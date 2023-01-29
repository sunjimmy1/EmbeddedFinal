/*
 * ADC.h
 *
 *  Created on: Dec 17, 2022
 *      Author: jimmy
 */

#ifndef ADC_H_
#define ADC_H_

#include <msp430.h>

void adcInit();         // Setup ADC to do single, on demand samples (no input arguments)
int readADCSingle();    // Single Read ADC, no input arguments, returns current ADC reading
void gpioInit();

#endif /* ADC_H_ */
