#include <stdio.h>    
#include <stdlib.h>    
#include <netinet/in.h>    
#include <sys/socket.h>    
#include <arpa/inet.h>    
#include <string.h>    
#include <unistd.h>  
#include <netdb.h> 
#include <time.h>

#include "CRC16.h"
#include "bufhead.h"

#define BUFFER_SIZE 1024    
        
int main(int argc, const char * argv[])    
{    
    int sockfd; 
	unsigned char buffer[1024]; 
	struct hostent *host;    
    struct sockaddr_in server_addr;  
    uint32_t readhead;//定义为32位避免不必要的错误
    unsigned int getcrc16;
    time_t now;   
	int i;
	struct tm *timenow;
    int len,type;
    if(argc!=2) 
	{ 
		fprintf(stderr,"Usage:%s hostname \a\n",argv[0]); 
		exit(1); 
	} 

	if((host=gethostbyname(argv[1]))==NULL) 
	{ 
		fprintf(stderr,"Gethostname error\n"); 
		exit(1); 
	} 
       
        bzero(&server_addr,sizeof(server_addr));
        server_addr.sin_family = AF_INET;    
        server_addr.sin_port = htons(6666);   
        server_addr.sin_addr=*((struct in_addr *)host->h_addr); // 
        //server_addr.sin_addr.s_addr = inet_addr("192.168.3.129");    
        //bzero(&(server_addr.sin_zero), 8);    
        
        int server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);    
        if(server_sock_fd == -1)    
        {    
        perror("socket error");    
        return 1;    
        }    
        unsigned char recv_msg[BUFFER_SIZE];    
        unsigned char input_msg[BUFFER_SIZE];    
        
        if(connect(server_sock_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) == 0)    
        {    
        fd_set client_fd_set;    
        struct timeval tv;    
        
        
        while(1)    
        {    
            tv.tv_sec = 20;    
            tv.tv_usec = 0;    
            FD_ZERO(&client_fd_set);    
            FD_SET(STDIN_FILENO, &client_fd_set);    
            FD_SET(server_sock_fd, &client_fd_set);    
        
           select(server_sock_fd + 1, &client_fd_set, NULL, NULL, &tv);    
            if(FD_ISSET(STDIN_FILENO, &client_fd_set))    
            {    
               // 发送数据 
		        printf("Input date len: ");
		        scanf("%x",&len);//buffer长度
		        input_msg[2]=len;
		        if(len==9)
		        {
			        input_msg[4]=3;
			        time(&now);   
			        timenow = localtime(&now);
			        input_msg[5]=((timenow->tm_year)%100/10)*10+(timenow->tm_year)%10;
			        
			        input_msg[6]=timenow->tm_mon+1;//月份转化为1-12
			        input_msg[7]=timenow->tm_mday;
			        input_msg[8]=timenow->tm_hour;
			        input_msg[9]=timenow->tm_min;
			        input_msg[10]=timenow->tm_sec;
			        writeUInt(input_msg,len);//写文件头
			        getcrc16=crc16(input_msg,len+2);
		
			        input_msg[len+2]=getcrc16;
			        input_msg[len+3]=getcrc16>>8;//CRC16校验
			        printf("CRC16:%x \n",getcrc16);
			        send(sockfd,input_msg,len+4,0);
		        }
		        else if(len==0)
		        {
			        for(i=0;i<=255;i++)
			        {
		 	            input_msg[i]=i;
			        }
			        send(sockfd,input_msg,256,0);
		        }
		        else
		        {
			        printf("Input type: ");
			        scanf("%x",&type);//buffer[4]=0x01;//类型
			        input_msg[4]=type;
		
			        printf("Input parameter:\n");
			        for(i=0;i<len-3;i++)
			        {
		 	            scanf("%x",&type);//buffer[5]=0x01;//类型
		 	            input_msg[5+i]=type;
			        }
			        writeUInt(input_msg,len);//写文件头
			        getcrc16=crc16(input_msg,len+2);
		
			        input_msg[len+2]=getcrc16;
			        input_msg[len+3]=getcrc16>>8;//CRC16校验
			        printf("CRC16:%x \n",getcrc16);
			        send(server_sock_fd,input_msg,len+4,0);
		        } 
                
                //if(send(server_sock_fd, input_msg, BUFFER_SIZE, 0) == -1)    
                //{    
                //    perror("send massage error!\n");    
                //}   
                
            }    
            if(FD_ISSET(server_sock_fd, &client_fd_set))    
            {    
                bzero(recv_msg, BUFFER_SIZE);    
                long byte_num = recv(server_sock_fd, recv_msg, BUFFER_SIZE, 0);    
                if(byte_num > 0)    
                {    
                    if(byte_num > BUFFER_SIZE)    
                    {    
                        byte_num = BUFFER_SIZE;         
                    }
                    if(recv_msg[4] == 0x32)
                    {
                        printf("date len:%ld\n",byte_num);
                
                        printf("received date:\n");
                        for(i=0;i<byte_num;i++)
                            printf("%x ",(unsigned char)recv_msg[i]);//打印参数
                        printf("\n");  
                    }         
                    //recv_msg[byte_num] = '\0';    
                    //printf("server:%s\n", recv_msg);    
                }    
                else if(byte_num < 0)    
                {    
                    printf("receive massage error!\n");    
                }    
                else    
                {    
                    printf("server quit!\n");    
                    exit(0);    
                }    
            }    
            }            
    }    
    return 0;    
}   
/*
#include<stdio.h>    
#include<stdlib.h>    
#include<netinet/in.h>    
#include<sys/socket.h>    
#include<arpa/inet.h>    
#include<string.h>    
#include<unistd.h>  
#include <netdb.h> 
#include <time.h>
#include "CRC16.h"
#include "bufhead.h"

#define BUFFER_SIZE 1024    
        
int main(int argc, const char * argv[])    
{    
    int sockfd; 
	unsigned char buffer[1024]; 
	struct hostent *host;    
    struct sockaddr_in server_addr;  
    uint32_t readhead;//定义为32位避免不必要的错误
    unsigned int getcrc16;
    time_t now;   
	int i;
	struct tm *timenow;
    int len,type;
    if(argc!=2) 
	{ 
		fprintf(stderr,"Usage:%s hostname \a\n",argv[0]); 
		exit(1); 
	} 

	if((host=gethostbyname(argv[1]))==NULL) 
	{ 
		fprintf(stderr,"Gethostname error\n"); 
		exit(1); 
	} 
       
        bzero(&server_addr,sizeof(server_addr));
        server_addr.sin_family = AF_INET;    
        server_addr.sin_port = htons(6666);   
        server_addr.sin_addr=*((struct in_addr *)host->h_addr); // 
        //server_addr.sin_addr.s_addr = inet_addr("192.168.3.129");    
        //bzero(&(server_addr.sin_zero), 8);    
        
        int server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);    
        if(server_sock_fd == -1)    
        {    
        perror("socket error");    
        return 1;    
        }    
         char recv_msg[BUFFER_SIZE];    
         char input_msg[BUFFER_SIZE];    
        
        if(connect(server_sock_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) == 0)    
        {    
        fd_set client_fd_set;    
        struct timeval tv;    
        
        
        while(1)    
        {    
            tv.tv_sec = 20;    
            tv.tv_usec = 0;    
            FD_ZERO(&client_fd_set);    
            FD_SET(STDIN_FILENO, &client_fd_set);    
            FD_SET(server_sock_fd, &client_fd_set);    
        
           select(server_sock_fd + 1, &client_fd_set, NULL, NULL, &tv);    
            if(FD_ISSET(STDIN_FILENO, &client_fd_set))    
            {    
               bzero(input_msg, BUFFER_SIZE);    
            	fgets(input_msg, BUFFER_SIZE, stdin);    
            	if(send(server_sock_fd, input_msg, BUFFER_SIZE, 0) == -1)    
            	{    
                	perror("send message error!\n");    
            	} 
            }    
            if(FD_ISSET(server_sock_fd, &client_fd_set))    
            {    
                bzero(recv_msg, BUFFER_SIZE);    
                long byte_num = recv(server_sock_fd, recv_msg, BUFFER_SIZE, 0);    
                if(byte_num > 0)    
                {    
                    if(byte_num > BUFFER_SIZE)    
                    {    
                        byte_num = BUFFER_SIZE;         
                    }       
                    recv_msg[byte_num] = '\0';    
                    printf("server:%s\n", recv_msg);    
                }    
                else if(byte_num < 0)    
                {    
                    printf("receive massage error!\n");    
                }    
                else    
                {    
                    printf("server quit!\n");    
                    exit(0);    
                }    
            }    
            }    
        
    }    
    return 0;    
}  
*/ 