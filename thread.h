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


#define CROSS_COMPILE

extern char mac[];

extern pthread_t pthread_id[8];//线程ID
extern pthread_mutex_t mut;//声明互斥变量
extern int tcp_control;

//extern int fd_gps;