#include <main.h>
#include <I2C.h>
#include <LCD.h>
#include <TempHum.h>
float *tempData;
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
        tempData = readTempData();
        sprintf(humStr, "Humidity: %2.2f%%", tempData[1]);
        sprintf(tempStr, "Temp: %2.2fF    ", tempData[0]);
        display(humStr, tempStr);
        __no_operation();
        __delay_cycles(5000000);
    }
}
