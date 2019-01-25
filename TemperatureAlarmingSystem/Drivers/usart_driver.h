#ifndef _UART_H_
#define _UART_H_

#ifndef FCLK_SYSTEM
#define FCLK_SYSTEM 8000000UL
#endif

#include <avr/io.h>

void          usart_init(unsigned short int baudrate);
void          usart_putc(unsigned char data);
unsigned char usart_getc(void);
void          usart_puts(char *str);
#endif