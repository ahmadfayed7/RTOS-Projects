#include "lcd.h"

void LCD_Init(void)
{
	LCD_DATA_PORT_DIR |= LCD_DATA_MASK;
	LCD_DATA_PORT &= (~LCD_DATA_MASK);

	LCD_CONTROL_PORT_DIR |= LCD_CONTROL_MASK;
	LCD_CONTROL_PORT &= (~LCD_CONTROL_MASK);
	
	clearRS();
	
	/* Init command */
	setD4();
	setD5();
	
	setEN();
	clearEN();
	_delay_ms(5);
	setEN();
	clearEN();
	_delay_us(100);
	setEN();
	clearEN();
	_delay_ms(5);

	clearD4();
	setEN();
	clearEN();
	_delay_us(40);

    LCD_SendCmd(0x28);// 4 bit mode, 1/16 duty, 5x8 font, 2lines
    LCD_SendCmd(0x0C);// display on
    LCD_SendCmd(0x06);// entry mode advance cursor
    LCD_SendCmd(0x01);// clear display and reset cursor
	_delay_ms(5);
	// Clear LCD
	//LCD_ClearAll();
}

void LCD_DispChar(unsigned char data)
{
	setRS();
	clearRW();

	if(data & 0x80) setD7(); else clearD7();
	if(data & 0x40) setD6(); else clearD6();
	if(data & 0x20) setD5(); else clearD5();
	if(data & 0x10) setD4(); else clearD4();
	setEN();
	clearEN();
	if(data & 0x08) setD7(); else clearD7();
	if(data & 0x04) setD6(); else clearD6();
	if(data & 0x02) setD5(); else clearD5();
	if(data & 0x01) setD4(); else clearD4();
	setEN();
	clearEN();
	_delay_us(40);
}
void LCD_DispCharXY(unsigned char x, unsigned char y,unsigned char data)
{
	LCD_GotoXY(x,y);
	LCD_DispChar(data);
}
void LCD_DispCharsXY(unsigned char x, unsigned char y,unsigned char *str, unsigned char len){
	LCD_GotoXY(x,y);
	LCD_DispChars(str,len);
}
void LCD_SendCmd(unsigned char cmd)
{
		clearRS();
		clearRW();

	    if(cmd & 0x80) setD7(); else clearD7();
	    if(cmd & 0x40) setD6(); else clearD6();
	    if(cmd & 0x20) setD5(); else clearD5();
	    if(cmd & 0x10) setD4(); else clearD4();
	    setEN();
		clearEN();
	    if(cmd & 0x08) setD7(); else clearD7();
	    if(cmd & 0x04) setD6(); else clearD6();
	    if(cmd & 0x02) setD5(); else clearD5();
	    if(cmd & 0x01) setD4(); else clearD4();
		setEN();
		clearEN();
	    _delay_us(40);
}

void LCD_ClearAll(void)
{
	LCD_SendCmd(0x01);
}
/* Pos, Line */
/* Line: 1/2 */
/* Pos: 0/15 */
void LCD_GotoXY(unsigned char x, unsigned char y)
{
	if(y == 1)
	{
		LCD_SendCmd(0x80 + x);
	}
	else if( y == 2)
	{
		LCD_SendCmd(0xC0 + x);
	}
	else if(y == 3)
	{
		LCD_SendCmd(0x94 + x);
	}
	else if( y == 4)
	{
		LCD_SendCmd(0xD4 + x);
	}

}
void LCD_DispStringXY(unsigned char x, unsigned char y,unsigned char *str)
{
	LCD_GotoXY(x,y);
	LCD_DispString(str);
}
void LCD_DispString(unsigned char *str)
{
	unsigned char count = 0;
	while(str[count] != '\0')
		LCD_DispChar(str[count++]);
}
void LCD_DispChars(unsigned char *str, unsigned char len)
{
	unsigned char i = 0;
	for (i=0;i<len;i++)
	{
		LCD_DispChar(str[i]);
	}
}
void toString(unsigned short num,unsigned char * str,unsigned char dispSize){
	unsigned char i = 0;
	for (i=0;i<dispSize;i++)
	{
		str[i] = ((num/(long)pow(10,(dispSize-i-1)))-(num/(long)pow(10,(dispSize-i)))*10)+'0';
	}
}
void getString(unsigned short num,unsigned char * str){
	str[0] = (num/100)+'0';
	str[1] = (num/10)-(num/100)*10+'0';
	str[2] = num-(num/10)*10+'0';
}

