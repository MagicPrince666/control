#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

//#ifdef __cplusplus
//extern "C"
//{
//#endif

int init_cycle_buffer(void);
unsigned int fifo_get( char *buf, unsigned int len);
unsigned int fifo_put( char *buf, unsigned int len);

//#ifdef __cplusplus
//}
//#endif

#endif
