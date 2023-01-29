/*
 * TempHum.c
 *
 *  Created on: Dec 16, 2022
 *      Author: jimmy
 */

#include <TempHum.h>

uint8_t HumidityData[HDataLength] = { 0 };
uint8_t TempData[TDataLength] = { 0 };
uint8_t tempReg[2] = { 0x00, 0x40 };
float data[2];

void initTemp()
{
    I2C_Master_ReadReg(TEMPADDR, 0x71, 1);
    __no_operation();
}

float* readTempData()
{
    unsigned long RawTemp = 0;
    float Temp = 0;
    unsigned long RawHumidity = 0;
    float Humidity = 0;

    I2C_Master_WriteReg(TEMPADDR, 0xAC, tempReg, 2);
    __delay_cycles(1280000);

    I2C_Master_ReadReg(TEMPADDR, 0x71, 6);
    __delay_cycles(30000);

    CopyArray(getRXBuffer(), HumidityData, HDataLength, 1);
    CopyArray(getRXBuffer(), TempData, TDataLength, 3);
    RawHumidity = (HumidityData[2] >> 4) | (HumidityData[1] << 4)
            | ((long) HumidityData[0] << 12);
    Humidity = (float) (RawHumidity >> 4);
    Humidity = (Humidity / 65536) * 100;

    RawTemp = ((long) (TempData[0] & 0x0F) << 16) | ((long) TempData[1] << 8)
            | TempData[2];
    Temp = (float) (RawTemp >> 4);
    Temp = ((Temp / 65536) * 200 - 50) * (9.0 / 5.0) + 32;

    data[0] = Temp;
    data[1] = Humidity;

    return data;
}
