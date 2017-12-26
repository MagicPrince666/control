#include "uart_cdc.h"
#include "uart.h"
#include "http.h"
#include "../../thread.h"

typedef enum
{
    USBCMD_PAYLOAD_RETURN = 0x5AA569C0,     //Return
    USBCMD_PAYLOAD_NOTIFICATION,   			// Notification
    USBCMD_PAYLOAD_RAW_DATA                 // Data
}USBCMD_PAYLOAD_TYPE;


struct cmd cmd;
struct dat dat;

char filename[25] = "2016_0101_002223_001.JPG";

int  fd[2] = {0};

uint32_t Get_UInt(const void* buf)
{
	const unsigned char* usBuf = (unsigned char*)buf;
	uint32_t iRetValue = ((uint32_t)usBuf[3]) << 24;
	iRetValue += ((uint32_t)usBuf[2]) << 16;
	iRetValue += ((uint32_t)usBuf[1]) << 8;
	iRetValue += ((uint32_t)usBuf[0]);
	return iRetValue;
}

void * Thread_acm0 (void *arg) 
{ 
    char rxbuf0 [150]; 
    int count = 0;
    fd[0] = acm_open((char*)"/dev/ttyACM0");
    if(fd[1] <= 0)
    {
        fd[1] = acm_open((char*)"/dev/ttyACM2");
    }
     if(fd[0] <= 0)
    {
        printf("open acm0 error!\n");
        pthread_exit(NULL);
    }  
    char *name = NULL;

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    fd_set rfds;
	int retval=0;

    while(tcp_control)
    {		
		FD_ZERO(&rfds);
		FD_SET(fd[0], &rfds);
		
		retval = select(fd[0] + 1, &rfds, NULL, NULL, &tv);
		if(retval<0) 
			perror("acm0 select error\n");  
		else if( retval && FD_ISSET(fd[0], &rfds))
		{ 
            //count = acm_recv(fd[0], rxbuf0, 148);
            count = read(fd[0], rxbuf0, 148);
            if(count)//控制通道收到数据
            {
                if(count == 140)//收到140个字符
                {
                    cmd.lenght = Get_UInt(rxbuf0);//获取数据长度
                    cmd.type = Get_UInt(rxbuf0 + 4);//获取数据类型
                    cmd.massge = rxbuf0 + 8; 
                    name = (char*)strstr(cmd.massge,"<Name>");
                    strncpy(filename, name+6,24);           // 获取文件名
                    printf("filename:%s\n",filename);
                    time(&now);
                    shoot_time = (unsigned int)now;//获取拍摄时间
                    //acm_send(fd[0],(char*)"/?custom=1&cmd=8002&par=1",26);//发送回复 开始接收数据
                    write(fd[0], (char*)"/?custom=1&cmd=8002&par=1", 26);
                    //printf("start receve\n");
                    sleep(2);//2S之后再接收
                }
                else
                {
                    printf("%s\n\n",rxbuf0);
                    //printf("acm0 receive error!\n");
                }
                       
            }
        }
    }
    close(fd[0]);
    pthread_exit(NULL);
}

void * Thread_acm1 (void *arg)
{
    char *rxbuf1;
//    int fd_pic = 0; //图片文件句柄
    unsigned long count = 0;
    unsigned long pic_count = 0;
    //unsigned int buf_cnt;
    fd[1] = acm_open((char*)"/dev/ttyACM1");
    
    if(fd[1] <= 0)
    {
        fd[1] = acm_open((char*)"/dev/ttyACM3");
    }
    if(fd[1] <= 0)
    {
        printf("open acm1 error!\n");
        pthread_exit(NULL); 
    }  
   
    dat.status = 0;
    rxbuf1 = (char*)malloc(4*1024);//一个包最大为4096

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    fd_set rfds;
	int retval=0;

    while(tcp_control)
    {     	
		FD_ZERO(&rfds);
		FD_SET(fd[1], &rfds);
		
		retval = select(fd[1] + 1, &rfds, NULL, NULL, &tv);
        if(retval < 0)  
			perror("acm1 select error\n");  
		
        else if( retval && FD_ISSET(fd[1], &rfds))
		{
            count = read(fd[1], rxbuf1, 4*1024);
            if(count)
            {
                if(count == 8 ) //收到文件头 开始接收数据
                {
                    dat.lenght = Get_UInt(rxbuf1) - 8;//获取文件长度
                    printf("picture lenght = %u\n",dat.lenght);
                    dat.type = Get_UInt(rxbuf1 + 4);// 获取传输类型
                    printf("type=0x%x\n",dat.type);
                    dat.date = (char*)malloc(dat.lenght);//一个包最大为4096
                    //fd_pic = open(filename,O_CREAT|O_RDWR,0666);//可读可写权限
                    pic_count = 0;
                    //buf_cnt = 0;
                    //pthread_mutex_lock (&mut);
                   /*
                    while(pic_count < (dat.lenght))
                    {
                        //buf_cnt = acm_recv(fd[1], rxbuf1, 4*1024);  
                        buf_cnt = read(fd[1], rxbuf1, 4*1024);          
                        memcpy(dat.date + pic_count, rxbuf1, buf_cnt);//拷贝到内存
                        pic_count += buf_cnt; 
                        //printf("count = %lu\n",count);                
                    }
                    //printf("get buf:%lu\n",count);
                    dat.status = 1;//有图片要上传
                    */
                    //pthread_mutex_unlock (&mut);
                    //if(-1 == write(fd_pic,dat.date,dat.lenght)) printf("write error!\n");//写入文件
                    //close(fd_pic);//关闭文件
                    //count = 0;
                    //printf("receve finish!\n");
                }
                else
                {
                    if(pic_count < (dat.lenght))
                    {
                        //buf_cnt = acm_recv(fd[1], rxbuf1, 4*1024);  
                        //buf_cnt = read(fd[1], rxbuf1, 4*1024);          
                        memcpy(dat.date + pic_count, rxbuf1, count);//拷贝到内存
                        pic_count += count; 
                        printf("count = %lu\n",pic_count);                
                    }
                    if(pic_count == dat.lenght)
                    {
                        dat.status = 1;//有图片要上传
                        pic_count = 0;
                        printf("receve finish!\n");
                    }
                    
                    //printf("acm1 receive %d buf error!\n",count); 
                }
                              
            }
        }
    }
    free(rxbuf1);
    close(fd[1]);
    pthread_exit(NULL);
}  


