#include "keypad.h"

void keypad_init(void)
{
	/* ROW => o/p and COL => i/p */
	KEYPAD_DIR |= KEYPAD_BTN3_ROW;
	KEYPAD_DIR |= KEYPAD_BTN6_ROW;
	KEYPAD_DIR |= KEYPAD_BTN9_ROW;
	
	KEYPAD_DIR &= ~(KEYPAD_BTN3_COL);
	KEYPAD_DIR &= ~(KEYPAD_BTN6_COL);
	KEYPAD_DIR &= ~(KEYPAD_BTN9_COL);	
	
	/* Activating Pull-up Resistors */
	KEYPAD_PORT |= KEYPAD_BTN3_COL;
	KEYPAD_PORT |= KEYPAD_BTN6_COL;
	KEYPAD_PORT |= KEYPAD_BTN9_COL;
	
	KEYPAD_PORT |= KEYPAD_BTN3_ROW;
	KEYPAD_PORT |= KEYPAD_BTN6_ROW;
	KEYPAD_PORT |= KEYPAD_BTN9_ROW;
	
	/* Buttons */
	BTN_DIR &= ~(BTN0_MASK);
	BTN_DIR &= ~(BTN1_MASK);
	
	BTN_PORT &= ~(BTN0_MASK);
	BTN_PORT &= ~(BTN1_MASK);
}

unsigned char keypad_getKey(void)
{
	unsigned char key = NO_PRESSED_KEY;
	KEYPAD_PORT |= KEYPAD_BTN3_ROW;
	KEYPAD_PORT |= KEYPAD_BTN6_ROW;
	KEYPAD_PORT &= ~KEYPAD_BTN9_ROW;
	_delay_ms(1);
	if((BTN_PIN & BTN0_MASK) == BTN0_MASK){
		key =  '0';
		while((BTN_PIN & BTN0_MASK) == BTN0_MASK);
	}
	else if((BTN_PIN & BTN1_MASK) == BTN1_MASK){
		key =  '1';
		while((BTN_PIN & BTN1_MASK) == BTN1_MASK);
	}
	else if ((KEYPAD_PIN & KEYPAD_BTN9_COL) == 0)
	{
		key =  '9';
		while((KEYPAD_PIN & KEYPAD_BTN9_COL) == 0);
	}
	else
	{
		KEYPAD_PORT |= KEYPAD_BTN3_ROW;
		KEYPAD_PORT &= ~KEYPAD_BTN6_ROW;
		KEYPAD_PORT |= KEYPAD_BTN9_ROW;
		if ((KEYPAD_PIN & KEYPAD_BTN6_COL) == 0)
		{
			key =  '6';
			while((KEYPAD_PIN & KEYPAD_BTN6_COL) == 0);
		}
		else
		{
			KEYPAD_PORT &= ~KEYPAD_BTN3_ROW;
			KEYPAD_PORT |= KEYPAD_BTN6_ROW;
			KEYPAD_PORT |= KEYPAD_BTN9_ROW;
			if ((KEYPAD_PIN & KEYPAD_BTN3_COL) == 0)
			{
				key =  '3';
				while((KEYPAD_PIN & KEYPAD_BTN3_COL) == 0);
			}
		}
	}
	return key;
}
