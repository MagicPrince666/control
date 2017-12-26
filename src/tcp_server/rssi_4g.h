#ifndef _UART_MCU_H
#define _UART_MCU_H

void *RSSI_4G(void *arg);
int monitor_routine(int uart_fd, char *rebuff, int len);

#endif