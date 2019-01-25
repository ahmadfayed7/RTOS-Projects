#ifndef _KEYPAD_H_
#define _KEYPAD_H_

#ifndef F_CPU
# define F_CPU 8000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>

#define NO_PRESSED_KEY 0xFF

#define KEYPAD_PORT PORTA
#define KEYPAD_DIR	DDRA
#define KEYPAD_PIN	PINA

#define KEYPAD_BTN3_ROW 0b00000100
#define KEYPAD_BTN3_COL 0b10000000

#define KEYPAD_BTN6_ROW 0b00001000
#define KEYPAD_BTN6_COL 0b10000000

#define KEYPAD_BTN9_ROW 0b00010000
#define KEYPAD_BTN9_COL 0b10000000

#define BTN_PIN	 	PIND
#define BTN_DIR	 	DDRD
#define BTN_PORT	PORTD
#define BTN0_MASK 	0b00000100
#define BTN1_MASK 	0b00001000

void keypad_init(void);
unsigned char keypad_getKey(void);

#endif
