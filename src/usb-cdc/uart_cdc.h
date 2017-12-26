#ifndef _UART_CDC_H
#define _UART_CDC_H

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

struct cmd{
    uint32_t lenght;
    uint32_t type;
    char *massge;
};

struct dat{
    uint32_t lenght;
    uint32_t type;
    char status;
    char *date;
};

extern struct dat dat;
extern char filename[25];

void * Thread_acm0 (void *arg);
void * Thread_acm1 (void *arg);

#endif
