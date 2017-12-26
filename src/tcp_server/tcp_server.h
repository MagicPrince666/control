/***************************************************************

 *File : tcp_server.h 
 *Auth : leo 
 *Date : 20170520
 *Mail : 846863428@qq.com 

***************************************************************/  
#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

//#include "CRC16.h"
#include <stdlib.h> 
#include <stdio.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <errno.h>    
#include <sys/socket.h>    
#include <arpa/inet.h>    
#include <string.h>    
#include <unistd.h> 
#include <pthread.h>//多线程头文件
#include <stdint.h> 
#include <linux/input.h>
#include <time.h>
#include <stdarg.h>
#include <syslog.h>

#define MAX_SOCK_CLI 5 //最大连接数为5
#define SERVER_PORT 6666    
#define BUFFER_SIZE 1024    

extern int ser_sockfd;
extern struct sockaddr_in ser_addr;    
extern int client_fds[MAX_SOCK_CLI];
extern unsigned char strenght;
extern unsigned char status_4g;
extern char gps_time[40];

//extern int uart_fd;
extern char gps_to_mcu[32];

void *Tcp_server(void *arg);
void *Uart_Modules(void *arg);
void *Input_Modules(void *arg);
void *Hart_Modules(void *arg);
int Init_Tcp_Server(void);
int send_gps_app(char *gps);//发送GPS到app


#endif