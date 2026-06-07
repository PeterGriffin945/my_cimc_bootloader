#ifndef __USART_H__
#define __USART_H__

#include "gd32f4xx.h"
#include <stdio.h>

#define BUF_SIZE  65536  // 64KB，可接收最大 64KB 的固件

extern unsigned char rx_buf[BUF_SIZE];
extern unsigned int  rx_len;
extern unsigned char rx_done;

void usart_init(void);
void usart_set_baud(uint32_t baud);

#endif
