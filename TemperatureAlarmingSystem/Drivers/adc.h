#ifndef _ADC_H_
#define _ADC_H_

#include <avr/io.h>

void ADC0_Init(void);
unsigned short readADC0(void);

#endif