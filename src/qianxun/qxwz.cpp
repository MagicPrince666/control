#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <net/if.h> 
#include <string.h>

#include "qxwz_rtcm.h"
#include "qxwz.h"
#include "../../thread.h"
#include "uart.h"
#include "tcp_server.h" //控制接口
#include "http.h"
#include "tcp_server.h"

extern int fd_gps;
unsigned char rtcm_status[3] = {0};
//#define _QXWZ_TEST_START_STOP

extern "C"
{

qxwz_account_info *p_account_info = NULL;
void  get_qxwz_sdk_account_info(void);

//根据差分账号获取差分数据流

void qxwz_rtcm_response_callback(qxwz_rtcm data){

    if(fd_gps >0 )//acm_send(fd_gps,data.buffer,data.length);//将RTCM返回数据发送给GPS处理  
    {
        pthread_mutex_lock(&mut);  
        #ifdef CROSS_COMPILE
        printf("send %d rtcm buf to gps modules\n", write(fd_gps,data.buffer,data.length));
        #else 
        printf("send %ld rtcm buf to gps modules\n", write(fd_gps,data.buffer,data.length));
        #endif                                    
        pthread_mutex_unlock(&mut);
/*
        printf("Rtcm hex:\n");
        for(int i = 0 ; i < data.length; i++)
        {
            printf("%x ",0xff&(*(data.buffer + i)));
            if(i > 0 && i%30 == 0)printf("\n"); 
        }          
        printf("\n"); 
*/        
    }            
}

void qxwz_status_response_callback(qxwz_rtcm_status code){
    
    QXLOGI("QXWZ_RTCM_STATUS:%d\n",code);
    rtcm_status[1] = code;
    rtcm_status[2] = code>>8;
    //用户名无效
    if(code == 1006 || code == 1019 || code == 1015 || code == 1016\
    || code == 2002 || code == 2003 || code == 2005 || code == 2006 || code == 2010)
    rtcm_status[0] |= 0x01;//无效

    //ID/type无效
    else if(code == 1017 || code == 1018)
    rtcm_status[0] |= 0x02;

	else if(code == QXWZ_STATUS_OPENAPI_ACCOUNT_TOEXPIRE){
		get_qxwz_sdk_account_info();
        rtcm_status[0] = 0;
	}
    else
    {
        rtcm_status[0] = 0;
    }
}

void  get_qxwz_sdk_account_info(void)
{
	p_account_info = getqxwzAccount();
    
	if(p_account_info->appkey != NULL) {
		//printf("appkey=%s\n",p_account_info->appkey);
	}
	if(p_account_info->deviceID != NULL) {
		//printf("deviceID=%s\n",p_account_info->deviceID);
	}
	if(p_account_info->deviceType != NULL) {
		//printf("deviceType=%s\n",p_account_info->deviceType);
	}

	if(p_account_info->NtripUserName != NULL) {
		//printf("NtripUserName=%s\n",p_account_info->NtripUserName);
	}
	if(p_account_info->NtripPassword != NULL) {
		//printf("NtripPassword=%s\n",p_account_info->NtripPassword);
	}
	//printf("expire_time=%ld\n",p_account_info->expire_time);
}

}

char appkey[10];
char appSecret[80];
char deviceId[32];
char deviceType[32];
int rtcm_contrl = 2;

#define POLY        0x1021 
/** 
* Calculating CRC-16 in 'C' 
* @para addr, start of data 
* @para num, length of data 
* @para crc, incoming CRC 
*/  
uint16_t mycrc16(unsigned char *addr, int num)  
{  
        int i; 
        uint16_t crc=0 ;
        for (; num > 0; num--)              /* Step through bytes in memory */  
        {  
            crc = crc ^ (*addr++ << 8);     /* Fetch byte from memory, XOR into CRC top byte*/  
            for (i = 0; i < 8; i++)             /* Prepare to rotate 8 bits */  
            {  
                if (crc & 0x8000)            /* b15 is set... */  
                    crc = (crc << 1) ^ POLY;    /* rotate and XOR with polynomic */  
                else                          /* b15 is clear... */  
                    crc <<= 1;                  /* just rotate */  
            }                             /* Loop for 8 bits */  
            crc &= 0xFFFF;                  /* Ensure CRC remains 16-bit value */  
        }                               /* Loop until num=0 */  
        return(crc);                    /* Return updated CRC */  
}

void *rtcm_gps (void *arg) //GPS精准定位
{ 
    //设置appKey和appSecret
    //apapKey申请详细见说明文档
    char *str = NULL;
    qxwz_config config;

#ifdef CROSS_COMPILE
//    get_MAC(mac,"eth0");//获取MAC地址;
//    config.deviceId = mac;
#endif

    //RTD	
    //config.appkey = (char*)"512600";
    //config.appSecret = (char*)"898faeb90fd2d3e07ebdeac2f35ef1c33303352862adbd769c3ccbfcf71f9e28";
    //config.deviceId=(char*)"F14_test1";
    //config.deviceType=(char*)"F14";
    int fd;

	fd = open("/home/rtcm_info.txt",O_RDWR);
	if(fd > 0)//文件存在 则读取帐号信息流地址
	{
        char buf[1024];
        int i,rem;
        read(fd,buf,1024);

		for(rem = i = 0;i < 1024;i++)
        {           
            if(buf[i] == '\n')
            {
                appSecret[i -rem] = '\0';
                i++;
                break;
            }
            appSecret[i - rem] = buf[i];
        }
		printf("appSecret = %s\n",appSecret);

        for(rem = i ;i < 1024;i++)
        {           
            if(buf[i] == '\n')
            {
                appkey[i - rem] = '\0';
                i++;
                break;
            }
            appkey[i - rem] = buf[i];
        }
		printf("appkey = %s\n",appkey);   

        for(rem = i ;i < 1024;i++)
        {           
            if(buf[i] == '\n')
            {
                deviceId[i -rem] = '\0';
                i++;
                break;
            }
            deviceId[i - rem] = buf[i];
        }
		printf("deviceId = %s\n",deviceId);

        for(rem = i ;i < 1024;i++)
        {           
            if(buf[i] == '\n')
            {
                deviceType[i -rem] = '\0';
                break;
            }
            deviceType[i - rem] = buf[i];
        }
		printf("deviceId = %s\n",deviceType);
        close(fd);
	}	
	else//不存在则创建
	{
		fd = open("/home/rtcm_info.txt",O_CREAT|O_RDWR,0666);
		
        strcpy(appSecret,"898faeb90fd2d3e07ebdeac2f35ef1c33303352862adbd769c3ccbfcf71f9e28");
        strcpy(appkey,"512600");
        strcpy(deviceId,"F14_test1");
        strcpy(deviceType,"F14");

        write(fd,appSecret,strlen(appSecret));
        lseek(fd,0,SEEK_END);
        write(fd,"\n",1);
		printf("appSecret = %s\n",appSecret);

        lseek(fd,0,SEEK_END);
        write(fd,appkey,strlen(appkey));
        lseek(fd,0,SEEK_END);
        write(fd,"\n",1);
		printf("appkey = %s\n",appkey);    

        lseek(fd,0,SEEK_END);
        write(fd,deviceId,strlen(deviceId));
        lseek(fd,0,SEEK_END);
        write(fd,"\n",1);
		printf("deviceId = %s\n",deviceId);

        lseek(fd,0,SEEK_END);
        write(fd,deviceType,strlen(deviceType));
        lseek(fd,0,SEEK_END);
        write(fd,"\n",1);
		printf("deviceType = %s\n",deviceType);
        close(fd);
	}	

    config.appkey = appkey;
    config.appSecret = appSecret;
    config.deviceId = deviceId;
    config.deviceType = deviceType;

    qxwz_setting(&config);

    //启动rtcm sdk
    qxwz_rtcm_start(qxwz_rtcm_response_callback,qxwz_status_response_callback);
    //demo测试10秒发送gga
    //每秒发送gga以获取最新的rtcm数据流
    //int fd_gps;
#ifdef CROSS_COMPILE
    fd_gps = acm_open((char*)"/dev/ttyS4");//arm 使用这条代码
#else
    fd_gps = acm_open((char*)"/dev/ttyUSB0");//PC使用此代码
#endif

    if(fd_gps <= 0)
    {
        printf("open GPS error!\n");
    }
    else  printf("open GPS sucess!\n");

    int count;
    char rxbuf[1024];
    char mbuf[1024];
    char *gps = (char*)malloc(128);
    int i;
    int len;

    double latitude = 0; //经度
    double longitude = 0; //纬度
    int status = 0,l_status = 0;
    char EORW=0,NORS=0,HORN=0;

    struct timeval tv;
    
    fd_set rfds;
    int retval=0;
    char rx_flag = 0;
    int buf_cnt = 0;

    while(tcp_control) 
    {
        tv.tv_sec = 1;
        tv.tv_usec = 0;
         
        FD_ZERO(&rfds);
        FD_SET(fd_gps, &rfds);	
		retval = select(fd_gps + 1, &rfds, NULL, NULL, &tv);
		if(retval<0) 
			perror("select error\n"); 
        if(retval == 0) 
        {
            //printf("ttys4\n");
            //write(fd_gps,"Hello 123\n\n",10);    
        }
		else if( retval && FD_ISSET(fd_gps, &rfds))
		{ 
            count = read(fd_gps, rxbuf, 1024);

            rx_flag = 0;  //接收未完成             
            
            if(buf_cnt + count > 1024)//包过大
            {
                buf_cnt = 0;  
                memset(mbuf,'\0',1024);
            }
            else
            {
                strncpy(mbuf + buf_cnt,rxbuf,count);
                buf_cnt += count;//统计字节数
            }

            if(NULL != strstr(mbuf,"OPEN*25"))//接收完整的包
            {
                rx_flag = 1;
                //printf("buf lenght :%d\n",buf_cnt);                   
            }
            //$GNZDA,092124.000,29,06,2017,00,00*4D
            //memset(rxbuf,'\0',1024);
            
            if(rx_flag == 1)
            {
                //printf("%s\n\n",mbuf); 

                str = strstr(mbuf,"$GNGGA");
                if(str != NULL)//截取$GNGGA信息
                {
                    for(i = 1 ;i < 100 ;i++)
                    {
                        if(str[i] == '$') 
                        {
                            gps[0] = '$';
                            gps[i] = '\0';
                            break;
                        }      
                        else
                            gps[i] = str[i];//分离出$GNGGA
                    }
                }
                str = strstr(mbuf,"$GNZDA");
                if(str != NULL)//截取$GNGGA信息
                {
                    for(i = 1 ;i < 40 ;i++)
                    {
                        if(str[i] == '$') 
                        {
                            gps_time[0] = '$';
                            gps_time[i] = '\0';
                            break;
                        }      
                        else
                            gps_time[i] = str[i];//分离出$GNGGA
                    }
                    //printf("%s",gps_time);
                }

                if(rtcm_contrl == 1)//上传到上位机
                {
                    char update[128];
                    int rem = 0;
                    int byle_count = 8;
                    uint16_t getcrc16;
                    
                    for(int i = 1 ;i < buf_cnt ;i++)
                    {
                        if(mbuf[i] == '$')//得到一个子帧 
                        {
                            rem ++;//子帧顺序           
                            update[7] = '$';
                            update[byle_count] = '\0';

                            update[0] = 0xf6;
                            update[1] = 0x16;
                            update[2] = byle_count - 4;
                            update[3] = ~update[2];
                            update[4] = 0x81;
                            update[5] = rem;
                            update[6] = strlen(update + 7) - 2;
                            //printf("%d %d %d %s",update[5],update[2],update[6],update + 7);

                            getcrc16 = mycrc16((unsigned char *)update,update[2] + 2 );		
							update[update[2] + 2] = getcrc16;
							update[update[2] + 3] = getcrc16 >> 8;//CRC16

                            pthread_mutex_lock(&mut);
                            for(int i = 0; i < MAX_SOCK_CLI; i++)//透传到APP    
                            {    
                                if(client_fds[i] != 0)       						  
                                    send(client_fds[i], update, update[2] + 4, 0);  
                            }
                            pthread_mutex_unlock(&mut);

                            byle_count = 8;
                            
                            usleep(50000);
                        }      
                        else
                        {
                            update[byle_count] = mbuf[i];//分离出一个字帧
                            byle_count ++;
                        }

                        if(rtcm_contrl == 2)break;
                    }
                          
                        update[7] = '$';
                        update[byle_count] = '\0';

                        update[0] = 0xf6;
                        update[1] = 0x16;
                        update[2] = byle_count - 4;
                        update[3] = ~update[2];
                        update[4] = 0x81;
                        update[5] = 0;//子帧顺序0 最后一帧
                        update[6] = strlen(update + 7) - 2;
                        //printf("%d %d %d %s",update[5],update[2],update[6],update + 7);

                        getcrc16 = mycrc16((unsigned char *)update,update[2] + 2 );		
						update[update[2] + 2] = getcrc16;
						update[update[2] + 3] = getcrc16 >> 8;//CRC16

                        pthread_mutex_lock(&mut);
                        for(int i = 0; i < MAX_SOCK_CLI; i++)//透传到APP    
                        {    
                            if(client_fds[i] != 0)       						  
                                send(client_fds[i], update, update[2] + 4, 0);  
                        }
                        pthread_mutex_unlock(&mut);
                           
                }
                buf_cnt = 0;
                memset(mbuf,'\0',1024);

                len = strlen(gps);

                //$GNGGA,085516.000,2233.9888,N,11352.5548,E,6,07,2.2,102.2,M,0.0,M,,*75
                if(len >= 48) //数据有效才传给SDK 
                {
                    printf("\nSend to RTCM:%s\n",gps); 
                    qxwz_rtcm_sendGGAWithGGAString(gps);//发送给千寻SDK
                    status = 0;
                    for( i = 0 ; i < len; i++)
                    {
                        if(gps[i] == ',') status++;//通过逗号确定位置
                        
                        if((status == 2) && (l_status != status))
                        {
                            l_status = status;
                            longitude = atof(gps + i + 1);//获取纬度信息
                        }
                        if((status == 3) && (l_status != status))
                        {
                            l_status = status;
                            NORS = *(gps + i +1);
                        }
                        if((status == 4) && (l_status != status))
                        {
                            l_status = status;
                            latitude = atof(gps + i + 1);//获取经度信息               
                        }
                        if((status == 5) && (l_status != status))
                        {
                            l_status = status;
                            EORW = *(gps + i +1);
                        }
                        if((status == 6) && (l_status != status))
                        {
                            l_status = status;
                            switch(atoi(gps + i + 1))
                            {
                                case 0:HORN = '0';break;//未定位
                                case 1:HORN = 'N';break;//普通精度
                                case 2:HORN = 'H';break;//高精度
                                case 6:HORN = '-';break;//正在估算
                            }
                            break;
                        }
                    }
                    memset(gps,'\0',128);
                    sprintf(gps,"%c,%c,%.6lf,%c,%.6lf",HORN,EORW,latitude,NORS,longitude);
                    if(status_4g)
                        post_gps_to_http(gps); 

                    if(HORN == 'N')
                        sprintf(gps_to_mcu,"1%c%4.6lf%c%5.6lf",EORW,latitude,NORS,longitude); 
                    else if(HORN == 'H')
                        sprintf(gps_to_mcu,"2%c%4.6lf%c%5.6lf",EORW,latitude,NORS,longitude); 
                    else
                        sprintf(gps_to_mcu,"0%c%4.6lf%c%5.6lf",EORW,latitude,NORS,longitude);                                         
                } 
                else
                {
                    memset(gps,'\0',128);
                    sprintf(gps_to_mcu,"000000.000000000000.000000"); //没有GPS数据  
                }   
            }
            else
            {
                sprintf(gps_to_mcu,"000000.000000000000.000000"); //没有GPS数据 
            } 
        }                        
		get_qxwz_sdk_account_info();
        //usleep(100000);
    }
    free(gps);
    QXLOGI("qxwz_rtcm_stop here\r\n");
    qxwz_rtcm_stop();
    QXLOGI("qxwz_rtcm_stop done\r\n");
    pthread_exit(NULL); 
} 
