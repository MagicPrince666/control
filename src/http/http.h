/***************************************************************

 *File : http.h 
 *Auth : leo 
 *Date : 20170520
 *Mail : 846863428@qq.com 

***************************************************************/  
#ifndef _MY_HTTP_H  
#define _MY_HTTP_H 

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <pthread.h> 
#include <assert.h> 
#include <unistd.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h> 
#include <net/if.h>
#include <sys/ioctl.h>
  
#define MY_HTTP_DEFAULT_PORT 80 

extern unsigned int shoot_time;         //拍摄时间戳 
extern time_t now;   //当前时间 

char * http_get(const char *url);  
char * http_post(const char *url,const unsigned char * post_str ,long leng, char * filename);
char * f06_post(const char *url,const unsigned char *post_buf ,long leng ,char * filename);
char * post_gps(const char *url);
void get_MAC(char *mac,char *net);
int post_gps_to_http (char * gps);
int get_from_http (void);
void * post_to_http (void *arg);
  
#endif 
