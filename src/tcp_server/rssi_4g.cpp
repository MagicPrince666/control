#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "tcp_server.h"

int monitor_routine(int uart_fd, char *rebuff, int len)
{   
    //printf("in  monitor_routine\n");
    memset(rebuff,0,sizeof(char)*len);
    int level = 0;
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 10000;

    fd_set rfds;
    int retval=0;
    
    int lenght = 0;
    lenght = write(uart_fd, (char *)"AT+CSQ\r\n", 8);
    //printf("send AT+CSQ!\n");
    if(lenght < 0) {          
        printf(" write error ! \n");
        return -1;
    }

    while(1)
    {
        FD_ZERO(&rfds);
        FD_SET(uart_fd, &rfds);
        retval = select(uart_fd + 1, &rfds, NULL, NULL, &tv);
        if(retval<0)
        {  
            perror("4g select error\n");  
        }
        else
        {  
            if( retval && FD_ISSET(uart_fd, &rfds))
            {
                int rc = read(uart_fd, rebuff, len);
                if(rc > 0) 
				{
                    //printf("%s",rebuff);
                    char *str;
                    str = strstr(rebuff,"+CSQ");
                    level = atof(str + 6);
                    strenght = 0;
                    if(level >= 25) strenght |= 0x02<<4;             //2格信号
                    if(level > 1 && level < 25) strenght |= 0x01<<4; //1格信号
                    if(level <= 1) strenght |= 0x00<<4;              //0格信号
					//printf("RSSI = %s", rebuff);
					return rc;
				}

            }    
        } 
    }
    return 0;
}
