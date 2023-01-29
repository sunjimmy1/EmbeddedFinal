#include <main.h>

float *tempData;
char tempHead[17] = { "Temperature:     " };
char humHead[17] = { "Humidity:        " };
char lightHead[17] = { "Light Level:     " };
char dataStr[17] = { "                 " };
int lightLevel = 0;
char tsString[20];
/**
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer
    //initClockTo16MHz();
    initGPIO();
    gpioInit();
    uart_init();
    PM5CTL0 &= ~LOCKLPM5;
    configureUART();
    initI2C();
    initLCD();
    adcInit();



    blocks();
    while (1)
    {
        tempData = readTempData();
        sprintf(dataStr, "%2.2f           ", tempData[1]);
        display(humHead, dataStr);//Hum
        __delay_cycles(800000);
        sprintf(dataStr, "%2.2f            ", tempData[0]);
        display(tempHead, dataStr);
        __delay_cycles(800000);
        _BIC_SR(GIE);
        lightLevel = readADCSingle();
        _BIS_SR(GIE);
        //itoa(lightLevel, tsString, 10);
        sprintf(tsString, "%i             ", lightLevel);
        transmit(tsString);
        sprintf(dataStr, "%i              ", lightLevel);
        display(lightHead, dataStr);
        __delay_cycles(100000);
    }
}
