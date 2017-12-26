#include "ringbuffer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


#define BUFFSIZE 1024 //(40960*1024)
#define min(x, y) ((x) < (y) ? (x) : (y)) 


struct cycle_buffer {
	char *buf;
	unsigned int size;
	unsigned int in;
	unsigned int out;
	pthread_mutex_t lock;  
};
static struct cycle_buffer *fifo = NULL;  

 int init_cycle_buffer(void)  
{
    int size = BUFFSIZE, ret;  

    ret = size & (size - 1);  
    if (ret)  
    {
        printf("init_cycle_buffer 1 ret%d\n",ret);
        return ret;
    }
    if(fifo != NULL)
    {
        if(fifo->buf != NULL)
        {
            free(fifo->buf);
            fifo->buf = NULL;
        }
        free(fifo);
        fifo = NULL;
    }
    
    fifo = (struct cycle_buffer *) malloc(sizeof(struct cycle_buffer));  
    if (!fifo)  
    {
        printf("init_cycle_buffer 2\n");
        return -1;  
    }

    memset(fifo, 0, sizeof(struct cycle_buffer));  
    fifo->size = size;  
    fifo->in = fifo->out = 0;  
    int r = pthread_mutex_init(&(fifo->lock), NULL);  
    printf("LOCK INIT:%d\n", r);
    fifo->buf = (char *) malloc(size);  
    if (!fifo->buf)
    {
        printf("init_cycle_buffer 3\n");
        free(fifo);
        
    }
    else
        memset(fifo->buf, 0, size);  
    return 0;  
}  


static unsigned int _fifo_get( char *buf, unsigned int len)  
{  
    unsigned int l;  
    printf("fifo->in - fifo->out :%d\n", fifo->in - fifo->out);
    len = min(len, fifo->in - fifo->out);  
    printf("_fifo_get len :%d\n", len);
    l = min(len, fifo->size - (fifo->out & (fifo->size - 1)));  
    printf("_fifo_get l :%d\n", l);
    memcpy(buf, fifo->buf + (fifo->out & (fifo->size - 1)), l);  
    memcpy(buf + l, fifo->buf, len - l);  
    fifo->out += len;  
    return len;  
}  
unsigned int fifo_get(char* buf, unsigned len)
{
	pthread_mutex_lock(&(fifo->lock));  
	int _len = _fifo_get(buf,len);
	pthread_mutex_unlock(&(fifo->lock));  
	return _len;
}


static unsigned int _fifo_put( char *buf, unsigned int len)  
{  
    unsigned int l;  
    len = min(len, fifo->size - fifo->in + fifo->out);  
    printf("_fifo_put :%d\n", len);
    
    l = min(len, fifo->size - (fifo->in & (fifo->size - 1)));  
    printf("_fifo_put l :%d\n", l);
    
    memcpy(fifo->buf + (fifo->in & (fifo->size - 1)), buf, l);  
    printf("_fifo_put cpy 1\n");
    
    memcpy(fifo->buf, buf + l, len - l);  
    printf("_fifo_put cpy 2\n");
    
    fifo->in += len;  
    return len;  
}  
unsigned int fifo_put(char* buf, unsigned len)
{
    printf("fifo_put 1\n");
	printf("&fifo : %p \n", fifo);
	printf("&fifo->lock : %d \n", &(fifo->lock));
	pthread_mutex_lock(&(fifo->lock));  
	int _len = _fifo_put(buf,len);
	pthread_mutex_unlock(&(fifo->lock));  
	return _len;
}

unsigned int fifo_len()
{
    return fifo->in == fifo->out ;
}

