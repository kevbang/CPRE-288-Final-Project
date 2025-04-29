/*
 * acd.h
 * author: Kevin Tran
 */

#ifndef ADC_H_
#define ADC_H_

#include "lcd.h"
#include "Timer.h"
#include <math.h>


void adc_init();
uint16_t adc_read();


#endif /* ADC_H_ */
