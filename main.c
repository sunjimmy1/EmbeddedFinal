#include <main.h>
#include <I2C.h>

uint8_t tempReg[6] = { 0x00, 0x40 };
uint8_t HumidityData[HDataLength] = { 0 };
uint8_t TempData[TDataLength] = { 0 };

unsigned long RawTemp = 0;
float Temp = 0;
unsigned long RawHumidity = 0;
float Humidity = 0;
char tempStr[16] = { "                " };
char humStr[16] = { "                " };

/**
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer
     initClockTo16MHz();
     initGPIO();
     initI2C();
     initLCD();

     blocks();
     while (1)
     {
         I2C_Master_ReadReg(TEMPADDR, 0x00, 3);
         __no_operation();
         __delay_cycles(30000);
         I2C_Master_ReadReg(TEMPADDR, 0x71, 1);
         I2C_Master_WriteReg(TEMPADDR, 0xAC, tempReg, 2);
         __delay_cycles(30000);
         I2C_Master_ReadReg(TEMPADDR, 0x71, 6);
         CopyArray(getRXBuffer(), HumidityData, HDataLength, 1);
         CopyArray(getRXBuffer(), TempData, TDataLength, 3);
         RawHumidity = (HumidityData[2] >> 4) | (HumidityData[1] << 4)
                 | ((long) HumidityData[0] << 12);
         Humidity = (float) (RawHumidity >> 4);
         Humidity = (Humidity / 65536) * 100;

         RawTemp = ((long) (TempData[0] & 0x0F) << 16)
                 | ((long) TempData[1] << 8) | TempData[2];
         Temp = (float) (RawTemp >> 4);
         Temp = ((((Temp / 65536) * 200 - 50) * 9) / 5) + 32;
         sprintf(humStr, "Humidity: %2.2f%%", Humidity);
         sprintf(tempStr, "Temp: %2.2fF    ", Temp);
         display(humStr, tempStr);
         __no_operation();
         __delay_cycles(5000000);
     }
}
