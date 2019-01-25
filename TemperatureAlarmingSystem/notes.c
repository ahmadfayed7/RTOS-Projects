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