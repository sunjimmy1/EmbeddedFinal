#include <main.h>

float *tempData;
char tempHead[16] = { "Temperature:    " };
char humHead[16] = { "Humidity:       " };
char lightHead[16] = { "Light Level:    " };
char dataStr[16] = { "                " };
int lightLevel = 0;
/**
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer
    //initClockTo16MHz();
    initGPIO();
    //uart_init();
    //configureUART();
    initI2C();
    initLCD();
    adcInit();
    gpioInit();


    //blocks();
    while (1)
    {
        tempData = readTempData();
        sprintf(dataStr, "%2.2f%%              ", tempData[1]);
        display(humHead, dataStr);
        __delay_cycles(500000);
        sprintf(dataStr, "%2.2fF              ", tempData[0]);
        display(tempHead, dataStr);
        __delay_cycles(500000);
        _BIC_SR(GIE);
        lightLevel = readADCSingle();

        sprintf(dataStr, "%i                  ", lightLevel);
        display(lightHead, dataStr);

        __no_operation();
        initClockTo16MHz();
        __delay_cycles(50000);
    }
}
