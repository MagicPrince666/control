#include <stdlib.h> 
#include <stdio.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <sys/socket.h> 
#include <time.h>
#include <unistd.h>
#include <errno.h> 
#include "CRC16.h"
#include "bufhead.h"
#include <pthread.h>//���߳�ͷ�ļ�

#define PORT 6666 
pthread_t pthread_id[2];//�߳�ID
pthread_mutex_t mut;

int sockfd;  
int err,nbytes=0;  
struct sockaddr_in addr_ser;  
struct hostent *host;
char sendline[1024],recvline[1024]; 
uint32_t readhead;//����Ϊ32λ���ⲻ��Ҫ�Ĵ���
uint16_t getcrc16; 
int len,type;


void *myThread1(void)//�߳�1 ��������
{
	time_t now;   
	int i;
	struct tm *timenow;
     	while(1)
    	{
        	/* �������� */
		printf("Input date len: ");
		scanf("%x",&len);//buffer����
		sendline[2]=len;
		if(len==9)
		{
			sendline[4]=3;
			time(&now);   
			timenow = localtime(&now);
			sendline[5]=((timenow->tm_year)%100/10)*10+(timenow->tm_year)%10;
			//sendline[5]=timenow->tm_year;//timenow->tm_year+1900;//ת��Ϊ��ǰ��
			sendline[6]=timenow->tm_mon+1;//�·�ת��Ϊ1-12
			sendline[7]=timenow->tm_mday;
			sendline[8]=timenow->tm_hour;
			sendline[9]=timenow->tm_min;
			sendline[10]=timenow->tm_sec;
			writeUInt(sendline,len);//д�ļ�ͷ
			getcrc16=crc16(sendline,len+2);
		
			sendline[len+2]=getcrc16;
			sendline[len+3]=getcrc16>>8;//CRC16У��
			printf("CRC16:%x \n",getcrc16);
			send(sockfd,sendline,len+4,0);
		}
		else if(len==0)
		{
			for(i=0;i<=255;i++)
			{
		 	   sendline[i]=i;
			}
			send(sockfd,sendline,256,0);
		}
		else
		{
			printf("Input type: ");
			scanf("%x",&type);//buffer[4]=0x01;//����
			sendline[4]=type;
		
			printf("Input parameter:\n");
			for(i=0;i<len-3;i++)
			{
		 	   scanf("%x",&type);//buffer[5]=0x01;//����
		 	   sendline[5+i]=type;
			}
			writeUInt(sendline,len);//д�ļ�ͷ
			getcrc16=crc16(sendline,len+2);
		
			sendline[len+2]=getcrc16;
			sendline[len+3]=getcrc16>>8;//CRC16У��
			printf("CRC16:%x \n",getcrc16);
			send(sockfd,sendline,len+4,0);
		}
		
		sleep(1);
      	}
      	pthread_exit(NULL);
}

void *myThread2(void)//�߳�2 ��������
{
	int i;
     	while(1)
     	{
        	//printf("waiting for server...\n");            
            	nbytes=recv(sockfd,recvline,1024,0);
            	if(nbytes)
            	{
            		printf("date len:%d\n",nbytes);
			
			printf("received date:\n");
		 	for(i=0;i<nbytes;i++)
				printf("0x%x ",(unsigned char)recvline[i]);//��ӡ����
			printf("\n");
			nbytes=0;
				
            	}
            	sleep(1);
      	}
      	pthread_exit(NULL);
}

int main(int argc,char **argv) 
{ 
	
	int ret=0;
	int err;
	
	/* ʹ��hostname��ѯhost ���� */
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
          
        sockfd=socket(AF_INET,SOCK_STREAM,0);  
        if(sockfd==-1)  
        {  
            printf("socket error\n");  
            return -1;  
        }  
          
        bzero(&addr_ser,sizeof(addr_ser));  
        addr_ser.sin_family=AF_INET;  
        addr_ser.sin_addr.s_addr=htonl(INADDR_ANY);  
        addr_ser.sin_port=htons(PORT);  
        addr_ser.sin_addr=*((struct in_addr *)host->h_addr); // IP��ַ
        err=connect(sockfd,(struct sockaddr *)(&addr_ser),sizeof(struct sockaddr));  
        if(err==-1)  
        {  
            printf("connect error\n");  
            return -1;  
        }  
          
        printf("connect with server...\n"); 
	/*��Ĭ�����Գ�ʼ��������*/
	pthread_mutex_init(&mut,NULL);
	
	/*�����߳�1*/
	ret = pthread_create(&pthread_id[0], NULL, (void*)myThread1, NULL);
        if (ret)
        {
           printf("Create pthread error!\n");
           return 1;
        }
        
	/*�����߳�2*/
    	ret = pthread_create(&pthread_id[1], NULL, (void*)myThread2, NULL);
    	if (ret)
    	{
        	printf("Create pthread error!\n");
        	return 1;
    	}
    	
    	if(pthread_id[0] !=0) {                   
                pthread_join(pthread_id[0],NULL);
                printf("server end!\n");
        }
        if(pthread_id[1] !=0) {                
                pthread_join(pthread_id[1],NULL);
                printf("uart end!\n");
        }
	   
	return 0;  
} 
