/*
 * LCD.c
 * A library to control the NHD-0216AW-IB3 OLED display module
 *  Created on: Dec 16, 2022
 *      Author: jimmy
 */
#include <I2C.h>
#include <LCD.h>
void blocks()
//Turns on all of the pixels in the display
{
    int i;

    LCDcommand(0x01);
    __delay_cycles(200);

    for (i = 0; i < 15; i++)
    {
        LCDdata(0x1F);
    }

    LCDcommand(0xA0);
    for (i = 0; i < 15; i++)
    {
        LCDdata(0x1F);
    }
}

void display(char *line1, char *line2)
//Writes the given strings to the LCD to display
{
    int i;

    LCDcommand(0x01);
    __delay_cycles(200);
    for (i = 0; i < 15; i++)
    {
        LCDdata((uint8_t) line1[i]);
    }

    LCDcommand(0xA0);
    for (i = 0; i < 15; i++)
    {
        LCDdata((uint8_t) line2[i]);
    }
}

void LCDcommand(uint8_t data)
//Sends the given command to the LCD
{
    I2C_Master_WriteReg(0x3C, 0x00, &data, 1);
}

void LCDdata(uint8_t data)
//Writes the given character to the LCD
{
    I2C_Master_WriteReg(0x3C, 0x42, &data, 1);
}

void initLCD()
{
    LCDcommand(0x2A);  //function set (extended LCDcommand set)
    LCDcommand(0x71);  //function selection A, disable internal Vdd regualtor
    LCDdata(0x00);
    LCDcommand(0x28);  //function set (fundamental LCDcommand set)
    LCDcommand(0x08);  //display off, cursor off, blink off
    LCDcommand(0x2A);  //function set (extended LCDcommand set)
    LCDcommand(0x79);  //OLED LCDcommand set enabled
    LCDcommand(0xD5);  //set display clock divide ratio/oscillator frequency
    LCDcommand(0x70);  //set display clock divide ratio/oscillator frequency
    LCDcommand(0x78);  //OLED LCDcommand set disabled
    LCDcommand(0x09);  //extended function set (4-lines)
    LCDcommand(0x06);  //COM SEG direction
    LCDcommand(0x72);  //function selection B, disable internal Vdd regualtor
    LCDdata(0x00);     //ROM CGRAM selection
    LCDcommand(0x2A);  //function set (extended LCDcommand set)
    LCDcommand(0x79);  //OLED LCDcommand set enabled
    LCDcommand(0xDA);  //set SEG pins hardware configuration
    LCDcommand(0x00); //set SEG pins
    LCDcommand(0xDC);  //function selection C
    LCDcommand(0x00);  //function selection C
    LCDcommand(0x81);  //set contrast control
    LCDcommand(0x7F);  //set contrast control
    LCDcommand(0xD9);  //set phase length
    LCDcommand(0xF1);  //set phase length
    LCDcommand(0xDB);  //set VCOMH deselect level
    LCDcommand(0x40);  //set VCOMH deselect level
    LCDcommand(0x78);  //OLED LCDcommand set disabled
    LCDcommand(0x28);  //function set (fundamental LCDcommand set)
    LCDcommand(0x01);  //clear display
    LCDcommand(0x80);  //set DDRAM address to 0x00
    LCDcommand(0x0C);  //display ON
    __delay_cycles(200);
}

