/*
 * testProject.c
 *
 * Created: 09/04/2015 10:04:07 ص
 *  Author: mah.hussein
 */ 


/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

/* Drivers Inc files */
#include "lcd.h"
#include "usart_driver.h"
#include "adc.h"
#include "keypad.h"

/* Other Inc files */
#include <math.h>

#define		  toggleLed(PORTX,PINX)				(PORTX & (1<<PINX))?(PORTX &= ~(1<<PINX)):(PORTX |= (1<<PINX))

/* Task Prototypes */
void T_Button( void *pvParameters );
void T_TempCheck(void *pvParameters);
void T_Terminal( void *pvParameters );
void T_Lcd( void *pvParameters );
void T_Alarm( void *pvParameters );
void T_TempSensor( void *pvParameters );
void T_ButtonEventHanle( void *pvParameters );
void T_TermEventHanle( void *pvParameters );


/* Function Prototypes */
void port_init(void);
void calcTTemp(void);
void incrementSelectedDigit(void);
void nextDigit(void);
void calcDigits(void);


/* System States */
#define MAIN_STATE 			11
#define ALARMING_STATE 		22
#define KEY_CONFG_STATE 	33
#define TERM_CONFG_STATE 	44

/*Alarmin buzzer and leds*/
#define ALARM_PORT 			PORTD
#define ALARM_DIR 			DDRD
#define ALARM_MASK 			0b11100000

/* LCD Screens */
#define MAIN_SCREEN_LINE1 			  (unsigned char*)"C:    T:    AS:E"
#define MAIN_SCREEN_LINE2 			  (unsigned char*)"KC:B3 TC:G AC:B1"
#define KEY_CONFG_SCREEN_LINE1 	      (unsigned char*)"T    B9:OK B1:CN"
#define KEY_CONFG_SCREEN_LINE2        (unsigned char*)"B3:+ B6:N  B0:CL"
#define TERM_CONFG_SCREEN_LINE1 	  (unsigned char*)"  T       CL:L  "
#define TERM_CONFG_SCREEN_LINE2 	  (unsigned char*)"  OK:K    CN:C  "
#define ALARMING_SCREEN_LINE1 	      (unsigned char*)"!!! ALARMING !!!"
#define ALARMING_SCREEN_LINE2 	      (unsigned char*)" DA:B1     DA:S "
// 'G' 
/* LCD Events */
#define TTEMP_EDIT_EVENT				0x10
#define TTEMP_KEY_CONFG_EVENT			0x20
#define TTEMP_MAIN_EVENT			    0x30
#define TTEMP_TERM_CONFG_EVENT			0x40
#define TTEMP_ALARMING_EVENT			0x50
#define TTEMP_KEY_CLEAR_EVENT			0x60
#define TTEMP_TERM_CLEAR_EVENT			0x70
#define CTEMP_MAIN_READ_UPDATE_EVENT	0x80
#define ALARM_CHANGE_STATE_EVENT		0x90


#define NONE							0x00

/* System Event MasKs */
#define B3_MASK			0x01
#define B6_MASK			0x02
#define B9_MASK			0x04
#define B0_MASK			0x08
#define B1_MASK			0x10

#define AL_MASK			0x20

#define RX_G_MASK		0x01
#define RX_K_MASK		0x02
#define RX_L_MASK		0x04
#define RX_S_MASK		0x08
#define RX_C_MASK		0x10
#define RX_DIGIT_MASK	0x20

/* LCD Temperature Data Positions */
#define ALARM_STATE_MAIN_XPOS	15
#define ALARM_STATE_MAIN_YPOS	1
#define CTEMP_MAIN_XPOS			4
#define CTEMP_MAIN_YPOS			1
#define TTEMP_MAIN_XPOS			10
#define TTEMP_MAIN_YPOS			1
#define TTEMP_KEY_CONFG_XPOS	3
#define TTEMP_KEY_CONFG_YPOS	1
#define TTEMP_TERM_CONFG_XPOS	5
#define TTEMP_TERM_CONFG_YPOS	1


struct LCDScreenStruct
{
	unsigned short threshTemp;
	unsigned short threshTempBackup;
	unsigned short currTemp;
	unsigned char alarmState;
	unsigned char ucSelectedDigit;
	unsigned char threshDigit1;
	unsigned char threshDigit2;
	unsigned char threshDigit3;
	unsigned char systemState;
	unsigned char eventType;
}lcdScreen;

EventGroupHandle_t egButtonEvents;
EventGroupHandle_t egTermEvents;
EventBits_t ebEventBits;
EventBits_t ebTermEventBits;
xSemaphoreHandle semLcdUpdate = NULL;

int main(void)
{
	port_init();
	LCD_Init();
	keypad_init();
	ADC0_Init();
	usart_init(9600);
	
	lcdScreen.currTemp = 31;
	lcdScreen.ucSelectedDigit = 1;
	lcdScreen.threshTemp = 52;
	lcdScreen.alarmState = 'E';
	lcdScreen.threshTempBackup = lcdScreen.threshTemp;
	calcDigits();
	lcdScreen.systemState = MAIN_STATE;
	lcdScreen.eventType = TTEMP_MAIN_EVENT;
	
	/* Creation of the Used Services */
	egButtonEvents = xEventGroupCreate();
	egTermEvents = xEventGroupCreate();
	vSemaphoreCreateBinary(semLcdUpdate);
	
	xTaskCreate(T_Terminal, ( char *)"T_Terminal", configMINIMAL_STACK_SIZE, NULL, 2, NULL );
	xTaskCreate(T_Button, ( char *)"T_Button", configMINIMAL_STACK_SIZE, NULL, 4, NULL );
	xTaskCreate(T_Lcd, ( char *)"T_Lcd", configMINIMAL_STACK_SIZE, NULL, 5, NULL );
	xTaskCreate(T_Alarm, ( char *)"T_Alarm", configMINIMAL_STACK_SIZE, NULL, 7, NULL );
	xTaskCreate(T_TempCheck, ( char *)"T_TempCheck", configMINIMAL_STACK_SIZE, NULL, 8, NULL );
	xTaskCreate(T_TempSensor, ( char *)"T_TempSensor", configMINIMAL_STACK_SIZE, NULL, 3, NULL );
	xTaskCreate(T_ButtonEventHanle, ( char *)"T_ButtonEventHanle", configMINIMAL_STACK_SIZE, NULL, 6, NULL );
	xTaskCreate(T_TermEventHanle, ( char *)"T_TermEventHanle", configMINIMAL_STACK_SIZE, NULL, 1, NULL );
	
	vTaskStartScheduler();
}

void T_Lcd(void *pvParameters)
{
	unsigned char strDisp[4] = {0};
	portBASE_TYPE bSemGet;
	pvParameters = pvParameters;
	while(1)
	{
		bSemGet = xSemaphoreTake(semLcdUpdate,0xFFFF);
		if(bSemGet == pdTRUE)
		{
			switch (lcdScreen.eventType)
			{
				case TTEMP_EDIT_EVENT:
					switch (lcdScreen.ucSelectedDigit){
						case 1:
						LCD_DispCharXY(TTEMP_KEY_CONFG_XPOS,TTEMP_KEY_CONFG_YPOS,lcdScreen.threshDigit1+'0');
						break;
						case 2:
						LCD_DispCharXY(TTEMP_KEY_CONFG_XPOS-1,TTEMP_KEY_CONFG_YPOS,lcdScreen.threshDigit2+'0');
						break;
						case 3:
						LCD_DispCharXY(TTEMP_KEY_CONFG_XPOS-2,TTEMP_KEY_CONFG_YPOS,lcdScreen.threshDigit3+'0');
						break;
					}
					lcdScreen.eventType = NONE;
				break;
				case TTEMP_TERM_CLEAR_EVENT:
					LCD_DispCharsXY(TTEMP_TERM_CONFG_XPOS-2,TTEMP_TERM_CONFG_YPOS,(unsigned char *)"000",3);
					lcdScreen.eventType = NONE;
				break;
				case TTEMP_KEY_CLEAR_EVENT:
				LCD_DispCharsXY(TTEMP_KEY_CONFG_XPOS-2,TTEMP_KEY_CONFG_YPOS,(unsigned char *)"000",3);
				lcdScreen.eventType = NONE;
				break;
				case ALARM_CHANGE_STATE_EVENT:
					LCD_DispCharXY(ALARM_STATE_MAIN_XPOS,ALARM_STATE_MAIN_YPOS,lcdScreen.alarmState);
					lcdScreen.eventType = NONE;
				break;
				case TTEMP_KEY_CONFG_EVENT:
					lcdScreen.ucSelectedDigit = 1;
					LCD_DispStringXY(0,1,KEY_CONFG_SCREEN_LINE1);
					LCD_DispStringXY(0,2,KEY_CONFG_SCREEN_LINE2);
					getString(lcdScreen.threshTemp,strDisp);
					LCD_DispCharsXY(TTEMP_KEY_CONFG_XPOS-2,TTEMP_KEY_CONFG_YPOS,strDisp,3);
					lcdScreen.eventType = NONE;
				break;
				case TTEMP_MAIN_EVENT:
					LCD_DispStringXY(0,1,MAIN_SCREEN_LINE1);
					LCD_DispStringXY(0,2,MAIN_SCREEN_LINE2);
					getString(lcdScreen.threshTemp,strDisp);
					LCD_DispCharsXY(TTEMP_MAIN_XPOS-2,TTEMP_MAIN_YPOS,strDisp,3);
					getString(lcdScreen.currTemp,strDisp);
					LCD_DispCharsXY(CTEMP_MAIN_XPOS-2,CTEMP_MAIN_YPOS,strDisp,3);
					LCD_DispCharXY(ALARM_STATE_MAIN_XPOS,ALARM_STATE_MAIN_YPOS,lcdScreen.alarmState);
					lcdScreen.eventType = NONE;
				break;
				case TTEMP_TERM_CONFG_EVENT:
					LCD_DispStringXY(0,1,TERM_CONFG_SCREEN_LINE1);
					LCD_DispStringXY(0,2,TERM_CONFG_SCREEN_LINE2);
					getString(lcdScreen.threshTemp,strDisp);
					LCD_DispCharsXY(TTEMP_TERM_CONFG_XPOS-2,TTEMP_TERM_CONFG_YPOS,strDisp,3);
					lcdScreen.eventType = NONE;
				break;
				case TTEMP_ALARMING_EVENT:
					LCD_DispStringXY(0,1,ALARMING_SCREEN_LINE1);
					LCD_DispStringXY(0,2,ALARMING_SCREEN_LINE2);
					lcdScreen.eventType = NONE;
				break;
				case CTEMP_MAIN_READ_UPDATE_EVENT:
					if (lcdScreen.systemState == MAIN_STATE)
					{
						getString(lcdScreen.currTemp,strDisp);
						LCD_DispCharsXY(CTEMP_MAIN_XPOS-2,CTEMP_MAIN_YPOS,strDisp,3);
						LCD_DispCharXY(ALARM_STATE_MAIN_XPOS,ALARM_STATE_MAIN_YPOS,lcdScreen.alarmState);
						lcdScreen.eventType = NONE;
					}
				break;
			}
		}
	}
}
void T_ButtonEventHanle(void *pvParameters)
{
	pvParameters = pvParameters;
	while(1)
	{
		ebEventBits = xEventGroupWaitBits(egButtonEvents,B0_MASK|B1_MASK|B3_MASK|B6_MASK|B9_MASK|RX_G_MASK|RX_K_MASK|RX_L_MASK|RX_S_MASK,pdTRUE,pdFALSE,(short)100000 );
		
		if((ebEventBits&B3_MASK) == B3_MASK){
			switch (lcdScreen.systemState)
			{
				case MAIN_STATE:
					lcdScreen.systemState = KEY_CONFG_STATE;
					lcdScreen.eventType = TTEMP_KEY_CONFG_EVENT;
					lcdScreen.threshTempBackup = lcdScreen.threshTemp;
					xSemaphoreGive(semLcdUpdate);
					break;
				case KEY_CONFG_STATE:
					incrementSelectedDigit();
					calcTTemp();
					lcdScreen.eventType = TTEMP_EDIT_EVENT;
					xSemaphoreGive(semLcdUpdate);
					break;
			}
		}else if((ebEventBits&B6_MASK) == B6_MASK){
			switch (lcdScreen.systemState)
			{
				case KEY_CONFG_STATE:
					nextDigit();
					break;
			}
		}else if((ebEventBits&B9_MASK) == B9_MASK){
			switch (lcdScreen.systemState)
			{
				case KEY_CONFG_STATE:
					lcdScreen.systemState = MAIN_STATE;
					lcdScreen.eventType = TTEMP_MAIN_EVENT;
					lcdScreen.threshTempBackup = lcdScreen.threshTemp;
					xSemaphoreGive(semLcdUpdate);
					break;
			}
		}
		else if((ebEventBits&B0_MASK) == B0_MASK){
			switch (lcdScreen.systemState)
			{
				case KEY_CONFG_STATE:
					lcdScreen.threshDigit1 = 0;
					lcdScreen.threshDigit2 = 0;
					lcdScreen.threshDigit3 = 0;
					lcdScreen.threshTemp   = 0;
					lcdScreen.ucSelectedDigit = 1;
					lcdScreen.eventType = TTEMP_KEY_CLEAR_EVENT;
					xSemaphoreGive(semLcdUpdate);
					break;
			}
		}
		else if((ebEventBits&B1_MASK) == B1_MASK){
			switch (lcdScreen.systemState)
			{
				case KEY_CONFG_STATE:
					lcdScreen.systemState = MAIN_STATE;
					lcdScreen.eventType = TTEMP_MAIN_EVENT;
					lcdScreen.threshTemp = lcdScreen.threshTempBackup;
					calcDigits();
					xSemaphoreGive(semLcdUpdate);
					break;
				case ALARMING_STATE:
					lcdScreen.alarmState = 'D';
					xEventGroupClearBits(egButtonEvents, AL_MASK);
					lcdScreen.systemState = MAIN_STATE;
					lcdScreen.eventType = TTEMP_MAIN_EVENT;
					xSemaphoreGive(semLcdUpdate);
					break;
				case MAIN_STATE:
					xEventGroupClearBits(egButtonEvents, AL_MASK);
					if (lcdScreen.alarmState == 'E')
					{
						lcdScreen.alarmState = 'D';
					} 
					else
					{
						lcdScreen.alarmState = 'E';
					}
					lcdScreen.eventType = ALARM_CHANGE_STATE_EVENT;
					xSemaphoreGive(semLcdUpdate);
					break;
			}
		}
	}
	
}
void T_Button(void *pvParameters)
{
	unsigned char key = NO_PRESSED_KEY;
	pvParameters = pvParameters;
	while(1)
	{
		key = keypad_getKey();
		switch (key)
		{
			case '3':
				ebEventBits = xEventGroupSetBits(egButtonEvents, B3_MASK);
				vTaskDelay(100);
				break;
			case '6':
				ebEventBits = xEventGroupSetBits(egButtonEvents, B6_MASK);
				vTaskDelay(100);
				break;
			case '9':
				ebEventBits = xEventGroupSetBits(egButtonEvents, B9_MASK);
				vTaskDelay(100);
				break;
			case '0':
				ebEventBits = xEventGroupSetBits(egButtonEvents, B0_MASK);
				vTaskDelay(100);
				break;
			case '1':
				ebEventBits = xEventGroupSetBits(egButtonEvents, B1_MASK);
				vTaskDelay(100);
				break;
			case NO_PRESSED_KEY:
				vTaskDelay(100);
				break;
		}
			
	}
}
void T_TempSensor(void *pvParameters)
{
	unsigned short adcValue;
	unsigned char cTempStr[3]={0};
	pvParameters = pvParameters;
	while(1)
	{
		adcValue = readADC0();
		adcValue = (unsigned short)(((float)500/1024)*adcValue);
		if(adcValue != lcdScreen.currTemp)
		{
			lcdScreen.currTemp = adcValue;
			getString(lcdScreen.currTemp,cTempStr);
			lcdScreen.eventType = CTEMP_MAIN_READ_UPDATE_EVENT;
			xSemaphoreGive(semLcdUpdate);
		}
		vTaskDelay(150);
	}
}
void T_Alarm(void *pvParameters)
{
	pvParameters = pvParameters;
	while(1)
	{
		ebEventBits = xEventGroupWaitBits(egButtonEvents,AL_MASK,pdFALSE,pdTRUE,1000 );
		if((ebEventBits&AL_MASK) == AL_MASK){
			ALARM_PORT |= ALARM_MASK;
			vTaskDelay(500);
			ALARM_PORT &= ~(ALARM_MASK);
			vTaskDelay(500);
		}
	}
}
void T_TempCheck(void *pvParameters)
{
	pvParameters = pvParameters;
	while(1)
	{
		if(lcdScreen.currTemp >= lcdScreen.threshTemp && lcdScreen.alarmState == 'E' && lcdScreen.systemState != KEY_CONFG_STATE && lcdScreen.systemState != TERM_CONFG_STATE && lcdScreen.systemState != ALARMING_STATE){
			xEventGroupSetBits(egButtonEvents, AL_MASK);
			lcdScreen.systemState = ALARMING_STATE;
			lcdScreen.eventType = TTEMP_ALARMING_EVENT;
			xSemaphoreGive(semLcdUpdate);
		}
		 else if((lcdScreen.currTemp < lcdScreen.threshTemp || lcdScreen.alarmState == 'D') && lcdScreen.systemState == ALARMING_STATE){
			xEventGroupClearBits(egButtonEvents, AL_MASK);
			lcdScreen.systemState = MAIN_STATE;
			lcdScreen.eventType = TTEMP_MAIN_EVENT;
			xSemaphoreGive(semLcdUpdate);
		}
		vTaskDelay(200);
	}
}
void T_Terminal(void *pvParameters)
{
	unsigned char ucChar = 0;
	pvParameters = pvParameters;
	while(1)
	{
		ucChar = usart_getc();
		switch (ucChar)
		{
				case 'G':
					usart_putc('G');
					lcdScreen.ucSelectedDigit = 3;
					xEventGroupSetBits(egTermEvents, RX_G_MASK);
					break;
				case 'L':
					usart_putc('L');
					xEventGroupSetBits(egTermEvents, RX_L_MASK);
					break;
				case 'K':
					usart_putc('K');
					lcdScreen.ucSelectedDigit = 1;
					xEventGroupSetBits(egTermEvents, RX_K_MASK);
					break;
				case 'S':
					usart_putc('S');
					xEventGroupSetBits(egTermEvents, RX_S_MASK);
					break;
				case 'C':
					usart_putc('C');
					lcdScreen.ucSelectedDigit = 1;
					xEventGroupSetBits(egTermEvents, RX_C_MASK);
					break;
				default:
					if (ucChar >= '0' && ucChar <= '9' && lcdScreen.systemState == TERM_CONFG_STATE)
					{
						usart_putc(ucChar);
						switch (lcdScreen.ucSelectedDigit)
						{
							case 1:
								lcdScreen.threshDigit1 = ucChar - '0';
								lcdScreen.ucSelectedDigit = 3;
								calcTTemp();
								lcdScreen.eventType = TTEMP_TERM_CONFG_EVENT;
								xSemaphoreGive(semLcdUpdate);
							break;
							case 2:
								lcdScreen.threshDigit2 = ucChar - '0';
								lcdScreen.ucSelectedDigit--;
							break;
							case 3:
								lcdScreen.threshDigit3 = ucChar - '0';
								lcdScreen.ucSelectedDigit--;
							break;
						}
					}
					break;
		}
		vTaskDelay(100);
	}
}
void T_TermEventHanle(void *pvParameters)
{
	pvParameters = pvParameters;
	while(1)
	{
		ebTermEventBits = xEventGroupWaitBits(egTermEvents,RX_G_MASK|RX_K_MASK|RX_L_MASK|RX_S_MASK|RX_C_MASK|RX_DIGIT_MASK,pdTRUE,pdFALSE,(short)100000 );
		
		if((ebTermEventBits&RX_G_MASK) == RX_G_MASK){
			switch (lcdScreen.systemState)
			{
				case MAIN_STATE:
					lcdScreen.systemState = TERM_CONFG_STATE;
					lcdScreen.eventType = TTEMP_TERM_CONFG_EVENT;
					lcdScreen.threshTempBackup = lcdScreen.threshTemp;
					xSemaphoreGive(semLcdUpdate);
				break;
			}
		}
		else if((ebTermEventBits&RX_K_MASK) == RX_K_MASK){
			switch (lcdScreen.systemState)
			{
				case TERM_CONFG_STATE:
					lcdScreen.systemState = MAIN_STATE;
					lcdScreen.eventType = TTEMP_MAIN_EVENT;
					lcdScreen.threshTempBackup = lcdScreen.threshTemp;
					xSemaphoreGive(semLcdUpdate);
				break;
			}
		}
		else if((ebTermEventBits&RX_L_MASK) == RX_L_MASK){
			switch (lcdScreen.systemState)
			{
				case TERM_CONFG_STATE:
					lcdScreen.threshDigit1 = 0;
					lcdScreen.threshDigit2 = 0;
					lcdScreen.threshDigit3 = 0;
					lcdScreen.threshTemp   = 0;
					lcdScreen.eventType = TTEMP_TERM_CLEAR_EVENT;
					xSemaphoreGive(semLcdUpdate);
				break;
			}
		}
		else if((ebTermEventBits&RX_C_MASK) == RX_C_MASK){
			switch (lcdScreen.systemState)
			{
				case TERM_CONFG_STATE:
					lcdScreen.systemState = MAIN_STATE;
					lcdScreen.eventType = TTEMP_MAIN_EVENT;
					lcdScreen.threshTemp = lcdScreen.threshTempBackup;
					calcDigits();
					xSemaphoreGive(semLcdUpdate);
				break;
			}
			}else if((ebTermEventBits&RX_G_MASK) == RX_G_MASK){
				switch (lcdScreen.systemState)
				{
					case MAIN_STATE:
						lcdScreen.systemState = TERM_CONFG_STATE;
						lcdScreen.eventType = TTEMP_TERM_CONFG_EVENT;
						lcdScreen.threshTempBackup = lcdScreen.threshTemp;
						xSemaphoreGive(semLcdUpdate);
					break;
				}
			}else if((ebTermEventBits&RX_S_MASK) == RX_S_MASK){
				switch (lcdScreen.systemState)
				{
					case ALARMING_STATE:
						lcdScreen.alarmState = 'D';
						xEventGroupClearBits(egButtonEvents, AL_MASK);
						lcdScreen.systemState = MAIN_STATE;
						lcdScreen.eventType = TTEMP_MAIN_EVENT;
						xSemaphoreGive(semLcdUpdate);
					break;
				}
			}
		}
}

void nextDigit(void)
{
	if(lcdScreen.ucSelectedDigit == 3)
	{
		lcdScreen.ucSelectedDigit = 1;
	}
	else
	{
		lcdScreen.ucSelectedDigit++;
	}
}
void port_init(void)
{
	/* Alarming */
	ALARM_PORT &= ~(ALARM_MASK);
	ALARM_DIR |= (ALARM_MASK);
}
void calcTTemp(void)
{
	lcdScreen.threshTemp = lcdScreen.threshDigit1 + lcdScreen.threshDigit2*10+lcdScreen.threshDigit3*100;
}

void calcDigits(void)
{
	lcdScreen.threshDigit3 = lcdScreen.threshTemp/100;
	lcdScreen.threshDigit2 = lcdScreen.threshTemp/10 - (lcdScreen.threshTemp/100)*10;
	lcdScreen.threshDigit1 = lcdScreen.threshTemp - (lcdScreen.threshTemp/10)*10;
}
void incrementSelectedDigit(void){
	switch (lcdScreen.ucSelectedDigit)
	{
		case 1:
			if (lcdScreen.threshDigit1 == 9)
			{
				lcdScreen.threshDigit1 = 0;
			} 
			else 
			{
				lcdScreen.threshDigit1++;
			}
			break;
		case 2:
			if (lcdScreen.threshDigit2 == 9)
			{
				lcdScreen.threshDigit2 = 0;
			}
			else
			{
				lcdScreen.threshDigit2++;
			}
			break;
		case 3:
			if (lcdScreen.threshDigit3 == 9)
			{
				lcdScreen.threshDigit3 = 0;
			}
			else
			{
				lcdScreen.threshDigit3++;
			}
			break;
	}
}