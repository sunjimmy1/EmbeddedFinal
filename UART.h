/*
 * UART.h
 *
 *  Created on: Dec 16, 2022
 *      Author: jimmy
 */

#ifndef UART_H_
#define UART_H_

#include <msp430.h>
#include <stdint.h>
#include <stdio.h>

void configureUART();
void uart_init();
void port_init();
void transmit(char *TXData);
void Software_Trim();
void configureClocks();
void strreverse(char *begin, char *end);
void itoa(int value, char* str, int base);

#define MCLK_FREQ_MHZ 1                     // MCLK = 1MHz

#endif /* UART_H_ */
