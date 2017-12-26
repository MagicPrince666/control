/***************************************************************

 *File : tcp_server.c
 *Auth : leo 
 *Date : 20170520
 *Mail : 846863428@qq.com 

***************************************************************/  

#include "tcp_server.h"
#include "uart.h"
#include "bufhead.h"
#include "log.h"
#include "CRC16.h"
#include "../../thread.h"
#include "rssi_4g.h"
#include "qxwz.h"
#include <net/if.h> 

//#define DEBUG_MCU
//#define DEBUG_APP

static int uart_fd = 0; 
unsigned char status_4g = 0;
char gps_to_mcu[32];  
static unsigned char Mac[6] = {0};
//static int fd_4g = 0;
unsigned char strenght;

char rtmp_add[128];

uint8_t w2m_ack[9] = {0};

int ser_sockfd;
struct sockaddr_in ser_addr;    
int client_fds[MAX_SOCK_CLI] = {0};


void get_mac(unsigned char *mac,char *net)
{
    struct   ifreq   ifreq; 
    int   sock;   
    
    if((sock=socket(AF_INET,SOCK_STREAM,0)) <0)
    { 
        perror( "socket"); 
    } 
    strcpy(ifreq.ifr_name,net); 
    if(ioctl(sock,SIOCGIFHWADDR,&ifreq) <0) 
    { 
        perror( "ioctl"); 
    }
	memcpy(Mac,ifreq.ifr_hwaddr.sa_data,6); 
}

uint8_t recvapp[1024];
int recv_len = 0;

void handle_app(uint8_t *recvapp)
{
	if (recv_len >= 8)   //需要处理 
	{ 
		int count = 0; 
		if(recvapp[0] == 0xF5 && recvapp[1] == 0x15)//帧头正确
			count = recvapp[2] + 4;
		else{//帧头不对 查找下一个
			recv_len --;//记录未处理包个数
			uint8_t * my_buf = (uint8_t *)malloc(recv_len);
			memcpy(my_buf,recvapp + 1,recv_len);
			memcpy(recvapp,my_buf,recv_len);//舍弃第一个 后面数据整体往前移动
			free(my_buf);

			handle_app(recvapp);//递归调用 直到找到帧头
		}

		if(count <= recv_len)//长度没有问题
		{
		#ifdef DEBUG_APP
			printf("APP to M1:");
			for(int i = 0;i < count;i++)
				printf(" %x",recvapp[i]);//
			printf("\n");
		#endif
			unsigned char buffer_app[40];
			uint16_t getcrc16;
			getcrc16 = recvapp[count-2]|recvapp[count-1]<<8;
			if(getcrc16 == crc16(recvapp,count-2))
			{
				if(recvapp[4] == 0x6E) //app to mcu 心跳
				{				
					unsigned char buffer_mcu[40]; 
					buffer_mcu[0] = 0xF5;
					buffer_mcu[1] = 0x15;
					buffer_mcu[4] = 0x6E;
					buffer_mcu[5] = 0x01;
					buffer_mcu[6] = 1;

					buffer_mcu[7] = recvapp[7]; //填入原有信息	
					//bit3 4g 状态
					buffer_mcu[7] |= (status_4g & 0x01) << 3;//4g 状态
					//bit4-bit7 信号强度
					
					if(status_4g)
					{
						buffer_mcu[7] |= strenght;//加上信号格数
					}
					
					//GPS数据
					buffer_mcu[8] = 0x03;
					buffer_mcu[9] = 26;

					pthread_mutex_lock(&mut);
					gps_to_mcu[0] = gps_to_mcu[0] - 0x30;//ASCII码转化16进制
					memcpy(buffer_mcu+10,gps_to_mcu,26);
					pthread_mutex_unlock(&mut);

					buffer_mcu[2] = 34;
					buffer_mcu[3] = ~buffer_mcu[2];

					getcrc16 = crc16(buffer_mcu,36);
					buffer_mcu[36] = getcrc16;
					buffer_mcu[37] = getcrc16 >> 8;//CRC16

				#ifdef DEBUG_APP
					printf("app hart:");
					for(int j = 0; j < 38 ;j++)
						printf("%X ",buffer_mcu[j]);
					printf("\n");									
				#endif
				
					memcpy(recvapp,buffer_mcu,38);			
					count = 38;
					pthread_mutex_lock (&mut);
					write(uart_fd,(char *)recvapp,count);     //A8 to MCU
					pthread_mutex_unlock (&mut); 

				}
				else if(recvapp[4] >= 0x81 && recvapp[4] <= 0x85)//GPS控制命令	
				{
					int rtcm_fd;
					switch(recvapp[4])
					{
						case 0x81://精度控制
							switch(recvapp[5])
							{
								case 0:break;//不操作

								case 1://打开高精度定位
								printf("open rtcm gps\n");
								rtcm_contrl = 1;						
								break;

								case 2://关闭高精度
								printf("rtcm gps qiut\n");
								rtcm_contrl = 2;
								break;

								default:break;
							}
						break;
						
						case 0x82://用户名/密码
						memset(appkey,'\0',10);
						memset(appSecret,'\0',80);

						strncpy(appSecret,(char *)recvapp + 6,recvapp[5]);
						strncpy(appkey,(char *)recvapp + 7 + recvapp[5],recvapp[6+recvapp[5]]);
										
						rtcm_fd = open("/home/rtcm_info.txt",O_CREAT | O_WRONLY | O_TRUNC, 0666);
						/* 清空文件 */
						ftruncate(rtcm_fd,0);
						lseek(rtcm_fd, 0, SEEK_SET); 

						write(rtcm_fd,appSecret,strlen(appSecret));
						lseek(rtcm_fd,0,SEEK_END);
						write(rtcm_fd,"\n",1);
						printf("appSecret = %s\n",appSecret);
						
						lseek(rtcm_fd,0,SEEK_END);
						write(rtcm_fd,appkey,strlen(appkey));
						lseek(rtcm_fd,0,SEEK_END);
						write(rtcm_fd,"\n",1);
						printf("appkey = %s\n",appkey);
						
						lseek(rtcm_fd,0,SEEK_END);
						write(rtcm_fd,deviceId,strlen(deviceId));
						lseek(rtcm_fd,0,SEEK_END);
						write(rtcm_fd,"\n",1);
						printf("deviceId = %s\n",deviceId);

						lseek(rtcm_fd,0,SEEK_END);
						write(rtcm_fd,deviceType,strlen(deviceType));
						lseek(rtcm_fd,0,SEEK_END);
						write(rtcm_fd,"\n",1);
						printf("deviceType = %s\n",deviceType);
						close(rtcm_fd);
						break;

						case 0x83://服务ID/type
						memset(deviceId,'\0',32);
						memset(deviceType,'\0',32);

						strncpy(deviceId,(char *)recvapp + 6,recvapp[5]);
						strncpy(deviceType,(char *)recvapp + 7 + recvapp[5],recvapp[6+recvapp[5]]);
						
						rtcm_fd = open("/home/rtcm_info.txt",O_CREAT | O_WRONLY | O_TRUNC, 0666);
						/* 清空文件 */
						ftruncate(rtcm_fd,0);
						lseek(rtcm_fd, 0, SEEK_SET); 

						write(rtcm_fd,appSecret,strlen(appSecret));
						lseek(rtcm_fd,0,SEEK_END);
						write(rtcm_fd,"\n",1);
						printf("appSecret = %s\n",appSecret);

						lseek(rtcm_fd,0,SEEK_END);
						write(rtcm_fd,appkey,strlen(appkey));
						lseek(rtcm_fd,0,SEEK_END);
						write(rtcm_fd,"\n",1);
						printf("appkey = %s\n",appkey);

						lseek(rtcm_fd,0,SEEK_END);
						write(rtcm_fd,deviceId,strlen(deviceId));
						lseek(rtcm_fd,0,SEEK_END);
						write(rtcm_fd,"\n",1);
						printf("deviceId = %s\n",deviceId);

						lseek(rtcm_fd,0,SEEK_END);
						write(rtcm_fd,deviceType,strlen(deviceType));
						lseek(rtcm_fd,0,SEEK_END);
						write(rtcm_fd,"\n",1);
						printf("deviceType = %s\n",deviceType);
						close(rtcm_fd);					
						break;

						case 0x84://查询用户名是否有效	
						buffer_app[0] = 0xF6;
						buffer_app[1] = 0x16;
						buffer_app[2] = 6;
						buffer_app[3] = ~buffer_app[2];
						buffer_app[4] = 0x84;
						if(rtcm_status[0]&0x01)buffer_app[5] = 1;
						else buffer_app[5] = 2;
						buffer_app[6] = rtcm_status[1];
						buffer_app[7] = rtcm_status[2];

						getcrc16 = crc16(buffer_app,buffer_app[2] + 2);
						buffer_app[buffer_app[2] + 2] = getcrc16;
						buffer_app[buffer_app[2] + 3] = getcrc16 >> 8;//CRC16
						
						pthread_mutex_lock(&mut);
						for(int i = 0; i < MAX_SOCK_CLI; i++)//透传到APP    
						{    
							if(client_fds[i] != 0)       						  
								send(client_fds[i], buffer_app, buffer_app[2] + 4, 0);  
						}
						pthread_mutex_unlock(&mut);
						break;

						case 0x85://查询id和type是否有效	
						buffer_app[0] = 0xF6;
						buffer_app[1] = 0x16;
						buffer_app[2] = 6;
						buffer_app[3] = ~buffer_app[2];
						buffer_app[4] = 0x85;
						if(rtcm_status[0]&0x02)buffer_app[5] = 1;
						else buffer_app[5] = 2;
						buffer_app[6] = rtcm_status[1];
						buffer_app[7] = rtcm_status[2];

						getcrc16 = crc16(buffer_app,buffer_app[2] + 2);
						buffer_app[buffer_app[2] + 2] = getcrc16;
						buffer_app[buffer_app[2] + 3] = getcrc16 >> 8;//CRC16
						
						pthread_mutex_lock(&mut);
						for(int i = 0; i < MAX_SOCK_CLI; i++)//透传到APP    
						{    
							if(client_fds[i] != 0)       						  
								send(client_fds[i], buffer_app, buffer_app[2] + 4, 0);  
						}
						pthread_mutex_unlock(&mut);
						break;
					}
				}				

				if(recvapp[4] == 0x0A)//查询命令	
				{					
					switch(recvapp[5])//to M1
					{																	
						case 0x13:
							int j;
							time_t now;   
							struct tm *timenow;

							buffer_app[0] = 0xF6;
							buffer_app[1] = 0x16;
							buffer_app[2] = 0x29;
							buffer_app[3] = ~buffer_app[2];
							buffer_app[4] = 0x30;

							j = strlen(gps_to_mcu);
							pthread_mutex_lock(&mut);
							gps_to_mcu[0] = gps_to_mcu[0] - 0x30;//ASCII码转化16进制
							memcpy(buffer_app+5,gps_to_mcu,j);
							pthread_mutex_unlock(&mut);
							
							time(&now);   
							timenow = localtime(&now);
							j = j + 5;

							buffer_app[j++] = (unsigned char)(timenow->tm_mday)/10 + 0x30;
							buffer_app[j++] = (unsigned char)(timenow->tm_mday)%10 + 0x30;
							buffer_app[j++] = (unsigned char)(timenow->tm_mon+1)/10 + 0x30;
							buffer_app[j++] = (unsigned char)(timenow->tm_mon+1)%10 + 0x30;
							buffer_app[j++] = (unsigned char)(timenow->tm_year + 1900)%100/10 + 0x30;
							buffer_app[j++] = (unsigned char)(timenow->tm_year + 1900)%10 + 0x30;
							buffer_app[j++] = (unsigned char)(timenow->tm_hour)/10 + 0x30;
							buffer_app[j++] = (unsigned char)(timenow->tm_hour)%10 + 0x30;
							buffer_app[j++] = (unsigned char)(timenow->tm_min)/10 + 0x30;
							buffer_app[j++] = (unsigned char)(timenow->tm_min)%10 + 0x30;
							buffer_app[j++] = (unsigned char)(timenow->tm_sec)/10 + 0x30;
							buffer_app[j++] = (unsigned char)(timenow->tm_sec)%10 + 0x30;

							getcrc16 = crc16(buffer_app,buffer_app[2] + 2);
							buffer_app[buffer_app[2] + 2] = getcrc16;
							buffer_app[buffer_app[2] + 3] = getcrc16 >> 8;//CRC16

							pthread_mutex_lock(&mut);
							for(int i = 0; i < MAX_SOCK_CLI; i++)//透传到APP    
							{    
								if(client_fds[i] != 0)       						  
									send(client_fds[i], buffer_app, buffer_app[2] + 4, 0);  
							}
							pthread_mutex_unlock(&mut);
						break;
						
						case 0x14:
							get_mac(Mac,(char *)"eth0");
							buffer_app[0] = 0xF6;
							buffer_app[1] = 0x16;
							buffer_app[2] = 0x09;
							buffer_app[3] = ~buffer_app[2];
							buffer_app[4] = 0x31;
							memcpy(buffer_app + 5,Mac,6); 
							getcrc16 = crc16(buffer_app,11);		
							buffer_app[11] = getcrc16;
							buffer_app[12] = getcrc16 >> 8;//CRC16
									
							pthread_mutex_lock(&mut);
							for(int i = 0; i < MAX_SOCK_CLI; i++)//透传到APP    
							{    
								if(client_fds[i] != 0)       						  
									send(client_fds[i], buffer_app, 13, 0);  
							}
							pthread_mutex_unlock(&mut);							
						break;

						case 0x15://4G连接情况
							buffer_app[0] = 0xF6;
							buffer_app[1] = 0x16;
							buffer_app[2] = 0x04;
							buffer_app[3] = ~buffer_app[2];
							buffer_app[4] = 0x32;//类型

							buffer_app[5] = 0;
							buffer_app[5] |= status_4g & 0x01;//4g 状态
						
							if(status_4g)
							{											
								buffer_app[5] |= strenght;//加上信号格数
							}
							
							getcrc16 = crc16(buffer_app,6);		
							buffer_app[6] = getcrc16;
							buffer_app[7] = getcrc16 >> 8;//CRC16
					
							pthread_mutex_lock(&mut);
							for(int i = 0; i < MAX_SOCK_CLI; i++)//透传到APP    
							{    
								if(client_fds[i] != 0)       						  
									send(client_fds[i], buffer_app, 8, 0);  
							}
							pthread_mutex_unlock(&mut);
						break;

						case 0x16:
							buffer_app[0] = 0xF6;
							buffer_app[1] = 0x16;
							buffer_app[2] = 0x07;
							buffer_app[3] = ~buffer_app[2];
							buffer_app[4] = 0x33;
							buffer_app[5] = 1;//主版本号
							buffer_app[6] = 0;//主版本号
							buffer_app[7] = 0xa;//次版本号
							buffer_app[8] = 0;//次版本号
							
							getcrc16 = crc16(buffer_app,9);		
							buffer_app[9] = getcrc16;
							buffer_app[10] = getcrc16 >> 8;//CRC16										
									
							pthread_mutex_lock(&mut);
							for(int i = 0; i < MAX_SOCK_CLI; i++)//透传到APP    
							{    
								if(client_fds[i] != 0)       						  
									send(client_fds[i], buffer_app, 11, 0);  
							}
							pthread_mutex_unlock(&mut);										
						break;

						default:
							pthread_mutex_lock (&mut);
							write(uart_fd,(char *)recvapp,count);     //A8 to MCU
							pthread_mutex_unlock (&mut); 
						break; 
									
					} 
				}
				else
				{
					pthread_mutex_lock (&mut);
					write(uart_fd,(char *)recvapp,count);     //A8 to MCU
					pthread_mutex_unlock (&mut); 
				}

			}
			else
			{
				printf("CRC16 app: %x\nAPP date:",crc16(recvapp,recvapp[2]+2));//
				for(int i = 0; i < count; i++)					
					printf("%x ",recvapp[i]);
				printf("\n");
			}

			recv_len -= count;//记录APP未处理包个数
			if(recv_len)//包不为空 则移除已处理的包
			{
				uint8_t * my_buf = (uint8_t *)malloc(recv_len);
				memcpy(my_buf,recvapp + count,recv_len);
				memcpy(recvapp,my_buf,recv_len);//放入缓存
				free(my_buf);
			}
		}	
	} 		
}

void *Tcp_server(void *arg)//线程1 
{     
	int i;	
	unsigned char recvline[1024];   
    fd_set server_fd_set;    
    int max_fd = -1;
	int ret;
	struct timeval tv;  //超时时间设置 

	while(tcp_control) 
	{ 
		//printf("tcp server\n");
		tv.tv_sec = 0;  //设置超时  
    	tv.tv_usec = 500000;
		FD_ZERO(&server_fd_set);    
    //    FD_SET(STDIN_FILENO, &server_fd_set);    
    //    if(max_fd < STDIN_FILENO)    
    //    {    
    //        max_fd = STDIN_FILENO;    
    //    }

		FD_SET(ser_sockfd,&server_fd_set);
		if(max_fd < ser_sockfd)  
		{
			max_fd = ser_sockfd;
		}      

		for(i =0 ; i < MAX_SOCK_CLI ; i++)
		{   
            if(client_fds[i] != 0)    
            {    
                FD_SET(client_fds[i], &server_fd_set);    
                if(max_fd < client_fds[i])    
                {    
                    max_fd = client_fds[i];    
                }    
            }
		}

		ret = select(max_fd + 1, &server_fd_set, NULL, NULL, &tv);    
        if(ret < 0)    
        {    
            perror("select error\n");    
            continue;    
        }    
        else if(ret == 0) //利用超时来处理数据   
        {    
			handle_app(recvapp);    
            continue;    
        }    
        else    
        {    
			//printf("=====================\r\n");  
            if(FD_ISSET(ser_sockfd, &server_fd_set))    
            {    
                //有新的连接请求    
                struct sockaddr_in client_address;    
                socklen_t address_len;    
                int client_sock_fd = accept(ser_sockfd, (struct sockaddr *)&client_address, &address_len);    
                printf("new connection client_sock_fd = %d\n", client_sock_fd);    
                if(client_sock_fd > 0)    
                {    
                    int index = -1;    
                    for(i = 0; i < MAX_SOCK_CLI; i++)    
                    {    
                        if(client_fds[i] == 0)    
                        {    
                            index = i;    
                            client_fds[i] = client_sock_fd;    
                            break;    
                        }    
                    }    
                    if(index >= 0)    
                    {    
                        printf("new client(%d) join %s:%d\n", index, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));    
                    }    
                    else    
                    {  
						//unsigned char uartbuf[1024];  
                        //bzero(uartbuf, BUFFER_SIZE);    
                        //strcpy((char *)uartbuf, "too much clients!\n");    
                        //send(client_sock_fd, uartbuf, BUFFER_SIZE, 0);    
						 
                        printf("client num is max, join fail %s:%d\n", \
						inet_ntoa(client_address.sin_addr), \
						ntohs(client_address.sin_port)); 
						close(client_sock_fd);   
                    }    
                }    
            }   

            for( i =0; i < MAX_SOCK_CLI; i++)    
            {    
                if(client_fds[i] !=0)    
                {    
                    if(FD_ISSET(client_fds[i], &server_fd_set))    
                    {    
						printf("client fd = %d send msg\r\n", client_fds[i]);
                        //处理某个客户端过来的消息                            
                        long byte_num = recv(client_fds[i], recvline, BUFFER_SIZE, 0);
						if (byte_num > 0)    
						{    
							if(recv_len + byte_num < 1024)//没有超出范围
							{
								memcpy(recvapp + recv_len,recvline,byte_num);//放入缓存
								recv_len += byte_num;
							}			
							else
							{
								printf("app pack distroy\n");			
								recv_len = 0 ;//丢弃
							}
							bzero(recvline, byte_num); //清空缓存 
						}
						else if(byte_num < 0)    
						{    
							printf("massage from client(%d) error!\n", i);    
						} 
						else    
						{    
							FD_CLR(client_fds[i], &server_fd_set);   
							client_fds[i] = 0;    
							printf("client(%d) quit!\n", i);    
						} 
                           
                    }    
                } 			   
            }    
        } 
	   //usleep(10000);
	   } 
	   LOG("tcp server exit\n");
	   printf("TCP server exit\n");
	   close(client_fds[0]);
	   pthread_exit(NULL);
}


void ack_MCU(uint8_t type,uint8_t statue)
{
	uint16_t getcrc16;
	w2m_ack[0] = 0xF5;
	w2m_ack[1] = 0x15;
	w2m_ack[2] = 0x05;
	w2m_ack[3] = ~w2m_ack[2];
	w2m_ack[4] = 0x6F;
	w2m_ack[5] = type;//ack type
	w2m_ack[6] = statue;//receice ok
	getcrc16 = crc16(w2m_ack,7);		
	w2m_ack[7] = getcrc16;
	w2m_ack[8] = getcrc16 >> 8;//CRC16

	pthread_mutex_lock (&mut);
	write(uart_fd,(char *)w2m_ack,9);     //A8 to MCU
	pthread_mutex_unlock (&mut); 
}

int power_4g = 0,wifi_reset = 0;
unsigned char buffer_mcu[32] = {0};	
int count_last = 0;
unsigned char buf_cache[1024];
uint8_t *my_buf;
char gps_time[40] = {0};

int send_app(uint8_t *buf)
{
	if(count_last >= 8)//需要处理
	{
		uint16_t getcrc16;
		int count = 0;
		if(buf[0] == 0xF6 && buf[1] == 0x16)//帧头正确
			count = buf[2] + 4;
		else{

			count_last --;//记录未处理包个数
			my_buf = (uint8_t *)malloc(count_last);
			memcpy(my_buf,buf + 1,count_last);
			memcpy(buf,my_buf,count_last);//舍弃第一个 后面数据整体往前挪
			free(my_buf);

			send_app(buf);//递归调用 直到找到帧头
		}
		if(count <= count_last)//包长度没问题 开始处理
		{
		#ifdef DEBUG_MCU
			printf("MCU to M1:");
			for(int i = 0;i < count;i++)
			{
				printf("%x ",buf[i]);
			}
			printf("\n");
		#endif
			//mcu control M1
			switch(buf[4])//to M1
			{
				case 0x40://4G 开关
					if(buf[5] == 0x01)//关闭
					{
						power_4g = open ("/sys/class/leds/PWRKEY_4G/brightness", O_RDWR);//4G 开关
						if(power_4g <= 0)printf ("Can't open /sys/class/leds/PWRKEY_4G/brightness !\n");
						else
						{
							if(-1 == write(power_4g,"0",1)) printf("write error!\n");
							sleep(1);
							if(-1 == write(power_4g,"1",1)) printf("write error!\n");
							sleep(5);
							if(-1 == write(power_4g,"0",1)) printf("write error!\n");
							sleep(1);
							if(-1 == write(power_4g,"1",1)) printf("write error!\n");
						}	
						close(power_4g);
					}
						
					if(buf[5] == 0x02)//打开
					{
						if(-1 == system("sh /home/4g-start.sh &"))
							printf("system() error!\n");			
					}
					getcrc16 = buf[count-2]|buf[count-1]<<8;
					if(getcrc16 == crc16(buf,count-2))
						ack_MCU(buf[4],0);//crc error
					else
						ack_MCU(buf[4],1);//ok	
				break;

				case 0x41://wifi开关
					wifi_reset = open ("/sys/class/leds/CPU_TO_WIFI_RESET/brightness", O_RDWR);//4G 开关
					if(wifi_reset <= 0)printf ("/sys/class/leds/CPU_TO_WIFI_RESET/brightness!\n");
					else
					{
						switch(buf[5])
						{
							case 0x01://关闭
								if(-1 == write(wifi_reset,"0",1)) printf("write error!\n");
								break;
							case 0x02://打开
								if(-1 == write(wifi_reset,"1",1)) printf("write error!\n");
								break;
						}
					}
					close(wifi_reset);	
					getcrc16 = buf[count-2]|buf[count-1]<<8;
					if(getcrc16 == crc16(buf,count-2))
						ack_MCU(buf[4],0);//crc error
					else
						ack_MCU(buf[4],1);//ok				
				break;

				case 0x42://查询M1
					switch(buf[5])
					{
						case 0x01://固件版本
							buffer_mcu[0] = 0xF5;
							buffer_mcu[1] = 0x15;
							buffer_mcu[2] = 0x07;
							buffer_mcu[3] = ~buffer_mcu[2];
							buffer_mcu[4] = 0x50;
							buffer_mcu[5] = 1;//主版本号
							buffer_mcu[6] = 0;//主版本号
							buffer_mcu[7] = 0xa;//次版本号
							buffer_mcu[8] = 0;//次版本号
							//strncpy(buffer_app + 5, gps, strlen(gps));
							getcrc16 = crc16(buffer_mcu,9);		
							buffer_mcu[9] = getcrc16;
							buffer_mcu[10] = getcrc16 >> 8;//CRC16
				
							pthread_mutex_lock(&mut);         //
							if(-1 == write(uart_fd,(char *)buffer_mcu,11)) 
							printf("Send to MCU error\n");//发送心跳到MCU 		
							pthread_mutex_unlock(&mut);       //	
							//printf("%X\n",buffer_mcu[4]);				
						break;

						case 0x02://mac地址
							get_mac(Mac,(char *)"eth0");
							buffer_mcu[0] = 0xF5;
							buffer_mcu[1] = 0x15;
							buffer_mcu[2] = 0x09;
							buffer_mcu[3] = ~buffer_mcu[2];
							buffer_mcu[4] = 0x51;
							memcpy(buffer_mcu + 5,Mac,6); 
							getcrc16 = crc16(buffer_mcu,11);		
							buffer_mcu[11] = getcrc16;
							buffer_mcu[12] = getcrc16 >> 8;//CRC16
							pthread_mutex_lock(&mut);         
							if(-1 == write(uart_fd,(char *)buffer_mcu,13)) 
							printf("Send to MCU error\n");//发送心跳到MCU 		
							pthread_mutex_unlock(&mut);       				
						break;

						case 0x03://GPS时间
							
							if(28 < strlen(gps_time))//GPS数据有效
							{
								buffer_mcu[0] = 0xF5;
								buffer_mcu[1] = 0x15;
								buffer_mcu[2] = 0x0a;
								buffer_mcu[3] = ~buffer_mcu[2];
								buffer_mcu[4] = 0x03;
								//0123456789012345678901234567890
								//$GNZDA,092124.000,29,06,2017,00,00*4D
								buffer_mcu[5] = (gps_time[26] - 0x30)*10 + (gps_time[27] - 0x30);//年
								buffer_mcu[6] = (gps_time[21] - 0x30)*10 + (gps_time[22] - 0x30);//月
								buffer_mcu[7] = (gps_time[18] - 0x30)*10 + (gps_time[19] - 0x30);//日
								buffer_mcu[8] = (gps_time[7] - 0x30)*10 + (gps_time[8] - 0x30);//时
								buffer_mcu[9] = (gps_time[9] - 0x30)*10 + (gps_time[10] - 0x30);//分
								buffer_mcu[10] = (gps_time[11] - 0x30)*10 + (gps_time[12] - 0x30);//秒
								buffer_mcu[11] = 0;

								printf("GPS time:");
								for(int i = 5;i < 11;i++)printf("%x ",buffer_mcu[i]);
								printf("\n");

								getcrc16 = crc16(buffer_mcu,12);		
								buffer_mcu[12] = getcrc16;
								buffer_mcu[13] = getcrc16 >> 8;//CRC16
								pthread_mutex_lock(&mut);         
								if(-1 == write(uart_fd,(char *)buffer_mcu,14))
									printf("Send to MCU error\n");//发送到MCU 		
								pthread_mutex_unlock(&mut); 
							}														
						break;

						default:break;
					}
					getcrc16 = buf[count-2]|buf[count-1]<<8;
					if(getcrc16 == crc16(buf,count-2))
						ack_MCU(buf[4],0);//crc error
					else
						ack_MCU(buf[4],1);//ok
				break;

				case 0x43://直播开关
					switch(buf[5])
					{
						case 0x01://关直播
							if(-1 == system("killall -9 RtmpPusher"))
								printf("system() error!\n");break;
						case 0x02://开直播
							char addr[256];
							sprintf(addr,"/home/RtmpPusher /dev/video0 default 1600 30 0 %s &",rtmp_add);
							printf("%s\n",addr);
							if(-1 == system(addr))
								printf("system() error!\n");break;
						default:break;
					}
					getcrc16 = buf[count-2]|buf[count-1]<<8;
					if(getcrc16 == crc16(buf,count-2))
						ack_MCU(buf[4],0);//crc error
					else
						ack_MCU(buf[4],1);//ok
				break;

				case 0x4F://rtmp流地址
					char address[128];
					int fd;
					
					memset(address,'\0',128);
					strncpy(address,(char *)buf + 6,buf[2] - 4);
					printf("rtmp IP address:%s\n",address);
					printf("recevl len :%d\n",buf[2]);
					printf("strlen :%d\n",strlen(address));
				
					fd = open("/home/rtmpaddress.txt",O_CREAT | O_WRONLY | O_TRUNC, 0666);
					if(fd)
					{
						/* 清空文件 */
						ftruncate(fd,0);
						lseek(fd, 0, SEEK_SET); 
						write(fd,address,strlen(address));
						close(fd);
					}
					

					if(buf[5] & 0x10)//立即生效
						strncpy(rtmp_add,address,128);

					getcrc16 = buf[count-2]|buf[count-1]<<8;
					if(getcrc16 == crc16(buf,count-2))
						ack_MCU(buf[4],0);//crc error
					else
						ack_MCU(buf[4],1);//ok
				break;	

				default://to app
					pthread_mutex_lock(&mut);
					for(int i = 0; i < MAX_SOCK_CLI; i++)//透传到APP    
					{    
						if(client_fds[i] != 0)       						  
							send(client_fds[i], buf, count, 0);  
					}
					pthread_mutex_unlock(&mut);
					break;									
			}

			count_last -= count;//记录weichuli包个数
			if(count_last)//包不为空 则移除已处理的包
			{
				my_buf = (uint8_t *)malloc(count_last);
				memcpy(my_buf,buf + count,count_last);
				memcpy(buf,my_buf,count_last);//放入缓存
				free(my_buf);
			}
		}
	}
	return 0;
}

void *Uart_Modules(void *arg)// receive date from uart
{
	int count = 0;
	unsigned char buf[1024];

	struct timeval tv;
    
	  
    int retval = 0;
	fd_set rfds;

    while(tcp_control)
    {	
		tv.tv_sec = 0;
    	tv.tv_usec = 100000;//超时设置
		FD_ZERO(&rfds);
    	FD_SET(uart_fd, &rfds);
		retval = select(uart_fd + 1, &rfds, NULL, NULL, &tv);
		if(retval < 0)
			perror("uart to mcu select error\n");
		else if(retval == 0)//接收超时 则开始处理数据
		{
			send_app(buf_cache);
			//printf("send buf\n");
		}
		else if( retval &&  FD_ISSET(uart_fd, &rfds) )//有数据要接收
		{
		 	count = read(uart_fd, buf, 1024);
					
			if(count_last + count < 1024)//没有超出范围
			{
				memcpy(buf_cache + count_last,buf,count);//放入缓存
				count_last += count;
			}			
			else
			{
				printf("back pack\n");			
				count_last = 0 ;//丢弃
			}				
		}	
    }
	
    close(uart_fd);
    pthread_exit(NULL);
}

void *Hart_Modules(void *arg)//send hart date to APP
{
	unsigned char buffer_mcu[40] = {0};//WIFI -> MCU
//	unsigned char buffer_app[10] = {0xF6,0x16,0x04,0xFB,0x15};//wifi -> app			
	uint16_t getcrc16;//
	int i = 0;
//	int j = 0;
	while(tcp_control)
	{
		buffer_mcu[0] = 0xF5;
		buffer_mcu[1] = 0x15;
		buffer_mcu[4] = 0x6E;
		buffer_mcu[5] = 0x01;
		buffer_mcu[6] = 1;

		buffer_mcu[7] = 0x0; //无客户端接入
		for(i = 0 ; i < MAX_SOCK_CLI ; i++)
		{
            if(client_fds[i] != 0)        
                buffer_mcu[7] |= 0x1;//bit0 手机APP加入夜视系统
		}	
		//bit1-bit2 00 正常运行 01 做好关机准备 10 做好待机准备 11 MTK固件升级
		buffer_mcu[7] |= 0x0<<1;
		//bit3 4g 状态
		buffer_mcu[7] |= (status_4g & 0x01) << 3;
		//bit4-bit7 信号强度
		//buffer_mcu[7] |= 1<<4;//信号格数
		if(status_4g)
		{
			buffer_mcu[7] |= strenght;//加上信号格数
		}

		//GPS数据
		buffer_mcu[8] = 0x03;
		buffer_mcu[9] = 26;

		pthread_mutex_lock(&mut);
		gps_to_mcu[0] = gps_to_mcu[0] - 0x30;//ASCII码转化16进制
		memcpy(buffer_mcu+10,gps_to_mcu,26);
		pthread_mutex_unlock(&mut);

		buffer_mcu[2] = 34;
		buffer_mcu[3] = ~buffer_mcu[2];

		getcrc16 = crc16(buffer_mcu,36);
		buffer_mcu[36] = getcrc16;
		buffer_mcu[37] = getcrc16 >> 8;//CRC16

		pthread_mutex_lock(&mut);        
		if(-1 == write(uart_fd,(char *)buffer_mcu,38)) printf("Send to MCU error\n");//发送心跳到MCU 		
		pthread_mutex_unlock(&mut);  

#ifdef DEBUG_MCU		
		printf("M1 to MCU:");
		for(i = 0; i < 38 ;i++)
			printf("%X ",buffer_mcu[i]);
		printf("\n\n");
#endif    
		/*
		j++;
		if(j >= 5)
		{
			j = 0;
			buffer_app[0] = 0xF6;
			buffer_app[1] = 0x16;
			buffer_app[2] = 0x04;
			buffer_app[3] = ~buffer_app[2];
			buffer_app[4] = 0x32;//类型

			buffer_app[5] = 0;
			buffer_app[5] |= (status_4g & 0x01) << 3;//4g 状态
			buffer_mcu[7] |= strenght;//加上信号格数

			getcrc16 = crc16(buffer_app,6);		
			buffer_app[6] = getcrc16;
			buffer_app[7] = getcrc16 >> 8;//CRC16
			for(i = 0; i < MAX_SOCK_CLI; i++)    
            {    
                if(client_fds[i] != 0)    
                {      
					pthread_mutex_lock(&mut);  
                    send(client_fds[i], buffer_app, 8, 0); 
					pthread_mutex_unlock(&mut);
                }    
            }		 		
		}
		*/		 
		sleep(1);
	}
	pthread_exit(NULL);
}

char buffer_4g[32]={0};
int len_4g = 32;
int fd_4g = 0;

void *Input_Modules(void *arg)//
{
	int keys_fd;   
	int gpio1_22;
	FILE* set_export;
	int gpio_in;
  	struct input_event t;
	char massge[10] = {0};  

	len_4g = 32;
  
  	keys_fd = open ("/dev/input/event0", O_RDONLY|O_NONBLOCK);  
  	if (keys_fd <= 0)  
    {  
		printf ("Can't open /dev/input/event0 device !\n");  
		pthread_exit(NULL); 
   	} 
	   
	gpio1_22 = open ("/sys/class/leds/WAKE_CPU_F06/brightness", O_RDWR);
	if(gpio1_22 <= 0)printf ("Can't open /sys/class/leds/WAKE_CPU_F06/brightness !\n");

	

	set_export = fopen ("/sys/class/gpio/export", "w");
	if(set_export == NULL)printf ("Can't open /sys/class/gpio/export!\n");
	else fprintf(set_export,"9");//gpio0_9 0*32+9
	fclose(set_export);
	
	//if(-1 == system("echo 9 > /sys/class/gpio/export"))
	//	printf("system() error!\n");

	gpio_in = open ("/sys/class/gpio/gpio9/direction", O_RDWR);
	if(gpio_in <= 0)printf ("Can't open /sys/class/gpio/gpio9/direction!\n");
	else write(gpio_in,"in",2);
	close(gpio_in);//设置成输入

	while(tcp_control)
	{	
		if (read (keys_fd, &t, sizeof (t)) == sizeof (t))  
		{  
			if (t.type == EV_KEY)  
				if (t.value == 0 || t.value == 1)  
				{  
					printf ("key %x %s\n", t.code, (t.value) ? "4g power off" : "4g power on");
					switch(t.code)
					{
						case 0x100://F06唤醒M1
						 	//t.value == 1 ? write(gpio1_22,"1",1) : write(gpio1_22,"0",1);
							break;
						case 0x101: //4G状态
							//t.value == 1 ? (status_4g = 0) : (status_4g = 1);
							break;
						default :  break;
					} 
				}  
		}
		gpio_in = open ("/sys/class/gpio/gpio9/value", O_RDWR);
		if(gpio_in <= 0)printf ("Can't open /sys/class/gpio/gpio9/value!\n");
		if(read(gpio_in,massge,8) == 0)
			printf("read status error!\n");
		else
			massge[0] == '1' ? (status_4g = 0) : (status_4g = 1);
		close(gpio_in);
		//printf("status_4g = %d\n",status_4g);
		if(status_4g == 0)
		{
			if(fd_4g > 0)
				close(fd_4g);
		}
		if(status_4g == 1 )
		{
			if(fd_4g > 0)
			{
				monitor_routine(fd_4g,buffer_4g,len_4g);
				//printf("strenght = 0x%x\n",strenght);
			}
			if(fd_4g == 0)
			{
				fd_4g = acm_open((char *)"/dev/ttyUSB2");
				if(fd_4g <= 0) 
				{
					printf("open 4g status error\nretry\n");
					sleep(10);
					fd_4g = acm_open((char *)"/dev/ttyUSB2");
				}
			}
		}
			
		sleep(1);
	}
	
	close(keys_fd);
	pthread_exit(NULL);
}

int Init_Tcp_Server(void)
{ 	    
	int err;
	int opt = 1;
	int fd;

	fd = open("/home/rtmpaddress.txt",O_RDWR);
	if(fd > 0)//文件存在 则读取rtmp流地址
	{
		read(fd,rtmp_add,128);
		printf("%s\n",rtmp_add);
	}	
	else//不存在则创建
	{
		fd = open("/home/rtmpaddress.txt",O_CREAT|O_RDWR,0666);
		strcpy(rtmp_add,"rtmp://baoqianli.8686c.com/live/3e571185d8b444c1aa7d2ecfc7c6d0c1");
		write(fd,rtmp_add,strlen(rtmp_add));
		printf("%s\n",rtmp_add);
	}	
	close(fd);
	

	uart_fd = acm_open((char *)"/dev/ttyS3");//打开与MCU通信端口
	if(uart_fd <= 0)
		printf("uart error!\n");
	
	ser_sockfd = socket(AF_INET,SOCK_STREAM,0);  
	if(ser_sockfd == -1)  
	{  
		printf("socket error:%s\n",strerror(errno));  
		return -1;
	}  
	setsockopt(ser_sockfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(&opt));  
	bzero(&ser_addr,sizeof(ser_addr));  
	ser_addr.sin_family = AF_INET;  
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
	ser_addr.sin_port = htons(SERVER_PORT);  
	err = bind(ser_sockfd,(struct sockaddr *)&ser_addr,sizeof(ser_addr));  
	if(err == -1)  
	{  
		printf("bind error:%s\n",strerror(errno));  
		return -1;  
	}  
		
	err = listen(ser_sockfd,5);  
	if(err == -1)  
	{  
		printf("listen error\n");  
		return -1;  
	}  
		
	printf("listen the port:%d\n",SERVER_PORT);      
	
	return 0; 
} 
