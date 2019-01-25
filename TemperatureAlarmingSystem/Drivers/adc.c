#include "adc.h"

void ADC0_Init(void){
	ADMUX = 0b01000000;
	SFIOR &= 0b00011111;
	ADCSRA = 0b10000000;
}
unsigned short readADC0(void){
	unsigned short adcValue = 0;
	ADCSRA |= (1<<ADSC);
	while(ADCSRA & (1<<ADSC));
	adcValue = ADCL + ADCH*256;
	return adcValue;
}