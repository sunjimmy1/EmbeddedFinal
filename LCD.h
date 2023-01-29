/*
 * LCD.c
 *
 *  Created on: Dec 16, 2022
 *      Author: jimmy
 */

#ifndef LCD_C_
#define LCD_C_

void initLCD();
void LCDcommand(uint8_t cmd);
void LCDdata(uint8_t data);
void blocks();
void display(char *line1, char *line2);

#endif /* LCD_C_ */
