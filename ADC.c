/*
 * ADC.c
 *
 *  Created on: Dec 17, 2022
 *      Author: jimmy
 */

#include <ADC.h>

void gpioInit(){
    // Configure ADC Pin
    P1SEL0 |= BIT1;
    P1SEL1 |= BIT1;
    //PM5CTL0 &= ~LOCKLPM5;
}

void adcInit(){
    ADCCTL0 |= ADCSHT_2 | ADCON;                             // ADCON, S&H=16 ADC clks
    ADCCTL1 |= ADCSHP;                                       // ADTBSSEL__SMCLK CCLK = MODOSC; sampling timer
    ADCCTL2 &= ~ADCRES;                                      // clear ADCRES in ADCCTL
    ADCCTL2 |= ADCRES_2;                                     // 12-bit conversion results
    ADCMCTL0 |= ADCINCH_1;                                   // A1 ADC input select; Vref=AVCC
    ADCIE |= ADCIE0;                                         // Enable ADC conv complete interrupt
    ADCIE &= ~ADCINIE_0;
}

int readADCSingle(){
    ADCCTL0 |= ADCENC | ADCSC;                           // Sampling and conversion start
//    __bis_SR_register(LPM0_bits | GIE);                  // LPM0, ADC_ISR will force exit
    while (!(ADCIFG & ADCIFG0));
    __no_operation();                                    // For debug only
    //while (!(ADCIFG & ADCIFG0));   // Wait for sample to be sampled and converted
    return ADCMEM0;//ADC_Result;
}

