#ifndef _CRC16_H_
#define _CRC16_H_

#include <stdint.h> 
#define POLY        0x1021 
/** 
* Calculating CRC-16 in 'C' 
* @para addr, start of data 
* @para num, length of data 
* @para crc, incoming CRC 
*/  
uint16_t crc16(unsigned char *addr, int num)  
{  
        int i; 
        uint16_t crc=0 ;
        for (; num > 0; num--)              /* Step through bytes in memory */  
        {  
            crc = crc ^ (*addr++ << 8);     /* Fetch byte from memory, XOR into CRC top byte*/  
            for (i = 0; i < 8; i++)             /* Prepare to rotate 8 bits */  
            {  
                if (crc & 0x8000)            /* b15 is set... */  
                    crc = (crc << 1) ^ POLY;    /* rotate and XOR with polynomic */  
                else                          /* b15 is clear... */  
                    crc <<= 1;                  /* just rotate */  
            }                             /* Loop for 8 bits */  
            crc &= 0xFFFF;                  /* Ensure CRC remains 16-bit value */  
        }                               /* Loop until num=0 */  
        return(crc);                    /* Return updated CRC */  
}

#endif
