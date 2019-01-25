#include "usart_driver.h"

void usart_init(unsigned short int baudrate);
void usart_putc( unsigned char data );
unsigned char usart_getc( void );
void usart_puts( char* str );

void usart_init(unsigned short int baudrate)
{
	//Set baud rate
	unsigned short int UBRRVAL=(FCLK_SYSTEM/(baudrate*16UL))-1;
	UBRRL=UBRRVAL; 			//low byte
	UBRRH=(UBRRVAL>>8); 	//high byte

	//Set data frame format: asynchronous mode,no parity, 1 stop bit, 8 bit size
	UCSRC=(1<<URSEL)|(0<<UMSEL)|(0<<UPM1)|(0<<UPM0)|(0<<USBS)|(0<<UCSZ2)|(1<<UCSZ1)|(1<<UCSZ0); 

	//Enable Transmitter and Receiver
	UCSRB=(1<<RXEN)|(1<<TXEN);
}
unsigned char usart_getc( void )
{
	unsigned char ucChar = 0;
	/* Wait for data to be received */
	if  (UCSRA & (1<<RXC)){
		ucChar = UDR;
	}
	/* Get and return received data from buffer */
	return ucChar;
}
void usart_putc( unsigned char data )
{
	/* Wait for empty transmit buffer */
	while ( !( UCSRA & (1<<UDRE)));
	/* Put data into buffer, sends the data */
	UDR = data;
}
void usart_puts( char* str )
{
	short int i=0;
	while(str[i])
	{
		usart_putc(str[i]);
		i++;
	}
}
