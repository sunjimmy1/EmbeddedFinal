//******************************************************************************
//   MSP430FR235x Demo - eUSCI_B1, I2C Master multiple byte TX/RX
//
//   Description: I2C master communicates to I2C slave sending and receiving
//   3 different messages of different length. I2C master will enter LPM0 mode
//   while waiting for the messages to be sent/receiving using I2C interrupt.
//   ACLK = NA, MCLK = SMCLK = DCO 16MHz.
//
//                                     /|\ /|\
//                   MSP430FR2355      4.7k |
//                 -----------------    |  4.7k
//            /|\ |             P1.3|---+---|-- I2C Clock (UCB1SCL)
//             |  |                 |       |
//             ---|RST          P1.2|-------+-- I2C Data (UCB1SDA)
//                |                 |
//                |                 |
//                |                 |
//                |                 |
//                |                 |
//                |                 |
//
//   Xiaodong Li
//   Texas Instruments Inc.
//   May 2020
//   Built with CCS V9.2
//******************************************************************************

#include <msp430.h> 
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

//******************************************************************************
// Pin Config ******************************************************************
//******************************************************************************

#define LED0_OUT    P1OUT
#define LED0_DIR    P1DIR
#define LED0_PIN    BIT0

#define LED1_OUT    P6OUT
#define LED1_DIR    P6DIR
#define LED1_PIN    BIT6

//******************************************************************************
// Example Commands ************************************************************
//******************************************************************************

#define SLAVE_ADDR  0x38

/* CMD_TYPE_X_SLAVE are example commands the master sends to the slave.
 * The slave will send example SlaveTypeX buffers in response.
 *
 * CMD_TYPE_X_MASTER are example commands the master sends to the slave.
 * The slave will initialize itself to receive MasterTypeX example buffers.
 * */

#define CMD_TYPE_0_SLAVE      0
#define CMD_TYPE_1_SLAVE      1
#define CMD_TYPE_2_SLAVE      2

#define CMD_TYPE_0_MASTER      3
#define CMD_TYPE_1_MASTER      4
#define CMD_TYPE_2_MASTER      5

#define HDataLength   3
#define TDataLength   3
#define TYPE_2_LENGTH   6

#define MAX_BUFFER_SIZE     20

/* MasterTypeX are example buffers initialized in the master, they will be
 * sent by the master to the slave.
 * SlaveTypeX are example buffers initialized in the slave, they will be
 * sent by the slave to the master.
 * */

uint8_t MasterType2[TYPE_2_LENGTH] = { 0x00, 0x40 };
uint8_t MasterType1[TDataLength] = { 8, 9 };
uint8_t MasterType0[HDataLength] = { 11 };

uint8_t HumidityData[HDataLength] = { 0 };
uint8_t TempData[TDataLength] = { 0 };
uint8_t SlaveType0[TYPE_2_LENGTH] = { 0 };

unsigned long RawTemp = 0;
float Temp = 0;
unsigned long RawHumidity = 0;
float Humidity = 0;
char tempStr[16] = { "                " };
char humStr[16] = { "                " };
;
//******************************************************************************
// General I2C State Machine ***************************************************
//******************************************************************************

typedef enum I2C_ModeEnum
{
    IDLE_MODE,
    NACK_MODE,
    TX_REG_ADDRESS_MODE,
    RX_REG_ADDRESS_MODE,
    TX_DATA_MODE,
    RX_DATA_MODE,
    SWITCH_TO_RX_MODE,
    SWITHC_TO_TX_MODE,
    TIMEOUT_MODE
} I2C_Mode;

/* Used to track the state of the software state machine*/
I2C_Mode MasterMode = IDLE_MODE;

/* The Register Address/Command to use*/
uint8_t TransmitRegAddr = 0;

/* ReceiveBuffer: Buffer used to receive data in the ISR
 * RXByteCtr: Number of bytes left to receive
 * ReceiveIndex: The index of the next byte to be received in ReceiveBuffer
 * TransmitBuffer: Buffer used to transmit data in the ISR
 * TXByteCtr: Number of bytes left to transfer
 * TransmitIndex: The index of the next byte to be transmitted in TransmitBuffer
 * */
uint8_t ReceiveBuffer[MAX_BUFFER_SIZE] = { 0 };
uint8_t RXByteCtr = 0;
uint8_t ReceiveIndex = 0;
uint8_t TransmitBuffer[MAX_BUFFER_SIZE] = { 0 };
uint8_t TXByteCtr = 0;
uint8_t TransmitIndex = 0;

/* I2C Write and Read Functions */

/* For slave device with dev_addr, writes the data specified in *reg_data
 *
 * dev_addr: The slave device address.
 *           Example: SLAVE_ADDR
 * reg_addr: The register or command to send to the slave.
 *           Example: CMD_TYPE_0_MASTER
 * *reg_data: The buffer to write
 *           Example: MasterType0
 * count: The length of *reg_data
 *           Example: HDataLength
 *  */
I2C_Mode I2C_Master_WriteReg(uint8_t dev_addr, uint8_t reg_addr,
                             uint8_t *reg_data, uint8_t count);

/* For slave device with dev_addr, read the data specified in slaves reg_addr.
 * The received data is available in ReceiveBuffer
 *
 * dev_addr: The slave device address.
 *           Example: SLAVE_ADDR
 * reg_addr: The register or command to send to the slave.
 *           Example: CMD_TYPE_0_SLAVE
 * count: The length of data to read
 *           Example: HDataLength
 *  */
I2C_Mode I2C_Master_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t count);
void CopyArray(uint8_t *source, uint8_t *dest, uint8_t count,
               uint8_t startIndex);

void command(uint8_t data);
void data(uint8_t data);
void blocks();
void initOled();
void output(char *line1, char *line2);

void output(char *line1, char *line2)
{
    int i;

    command(0x01);
    __delay_cycles(200);
    for (i = 0; i < 16; i++)
    {
        data((uint8_t) line1[i]);
    }

    command(0xA0);
    for (i = 0; i < 16; i++)
    {
        data((uint8_t) line2[i]);
    }
}

void command(uint8_t data)
{
    I2C_Master_WriteReg(0x3C, 0x00, &data, 1);
}

void data(uint8_t data)
{
    I2C_Master_WriteReg(0x3C, 0x42, &data, 1);
}
I2C_Mode I2C_Master_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t count)
{
    /* Initialize state machine */
    MasterMode = TX_REG_ADDRESS_MODE;
    TransmitRegAddr = reg_addr;
    RXByteCtr = count;
    TXByteCtr = 0;
    ReceiveIndex = 0;
    TransmitIndex = 0;

    /* Initialize slave address and interrupts */
    UCB1I2CSA = dev_addr;
    UCB1IFG &= ~(UCTXIFG + UCRXIFG); // CTransmitRegAddrlear any pending interrupts
    UCB1IE &= ~UCRXIE;                       // Disable RX interrupt
    UCB1IE |= UCTXIE;                        // Enable TX interrupt

    UCB1CTLW0 |= UCTR + UCTXSTT;             // I2C TX, start condition
    __bis_SR_register(LPM0_bits + GIE);              // Enter LPM0 w/ interrupts

    return MasterMode;

}

I2C_Mode I2C_Master_WriteReg(uint8_t dev_addr, uint8_t reg_addr,
                             uint8_t *reg_data, uint8_t count)
{
    /* Initialize state machine */
    MasterMode = TX_REG_ADDRESS_MODE;
    TransmitRegAddr = reg_addr;

    //Copy register data to TransmitBuffer
    CopyArray(reg_data, TransmitBuffer, count, 0);

    TXByteCtr = count;
    RXByteCtr = 0;
    ReceiveIndex = 0;
    TransmitIndex = 0;

    /* Initialize slave address and interrupts */
    UCB1I2CSA = dev_addr;
    UCB1IFG &= ~(UCTXIFG + UCRXIFG);       // Clear any pending interrupts
    UCB1IE &= ~UCRXIE;                       // Disable RX interrupt
    UCB1IE |= UCTXIE;                        // Enable TX interrupt

    UCB1CTLW0 |= UCTR + UCTXSTT;             // I2C TX, start condition
    __bis_SR_register(LPM0_bits + GIE);              // Enter LPM0 w/ interrupts

    return MasterMode;
}

void CopyArray(uint8_t *source, uint8_t *dest, uint8_t count,
               uint8_t startIndex)
{
    uint8_t copyIndex = 0;
    for (copyIndex = 0; copyIndex < count; copyIndex++)
    {
        dest[copyIndex] = source[copyIndex + startIndex];
    }
}

//******************************************************************************
// Device Initialization *******************************************************
//******************************************************************************

void initGPIO()
{
    //LEDs
    LED0_OUT &= ~LED0_PIN;
    LED0_DIR |= LED0_PIN;

    LED1_OUT &= ~LED1_PIN;
    LED1_DIR |= LED1_PIN;

    // I2C pins
    P4SEL0 |= BIT6 | BIT7;
    P4SEL1 &= ~(BIT6 | BIT7);

    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;
}

void initClockTo16MHz()
{
    // Configure one FRAM waitstate as required by the device datasheet for MCLK
    // operation beyond 8MHz _before_ configuring the clock system.
    FRCTL0 = FRCTLPW | NWAITS_1;

    // Clock System Setup
    __bis_SR_register(SCG0);                           // disable FLL
    CSCTL3 |= SELREF__REFOCLK;               // Set REFO as FLL reference source
    CSCTL0 = 0;                                   // clear DCO and MOD registers
    CSCTL1 &= ~(DCORSEL_7);             // Clear DCO frequency select bits first
    CSCTL1 |= DCORSEL_5;                               // Set DCO = 16MHz
    CSCTL2 = FLLD_0 + 487;                             // DCOCLKDIV = 16MHz
    __delay_cycles(3);
    __bic_SR_register(SCG0);                           // enable FLL
    while (CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1))
        ;         // FLL locked
}

void initI2C()
{
    UCB1CTLW0 = UCSWRST;                      // Enable SW reset
    UCB1CTLW0 |= UCMODE_3 | UCMST | UCSSEL__SMCLK | UCSYNC; // I2C master mode, SMCLK
    UCB1BRW = 160;                            // fSCL = SMCLK/160 = ~100kHz
    UCB1I2CSA = SLAVE_ADDR;                   // Slave Address
    UCB1CTLW0 &= ~UCSWRST;                   // Clear SW reset, resume operation
    UCB1IE |= UCNACKIE;
}

void initOled()
{
    command(0x2A);  //function set (extended command set)
    command(0x71);  //function selection A, disable internal Vdd regualtor
    data(0x00);
    command(0x28);  //function set (fundamental command set)
    command(0x08);  //display off, cursor off, blink off
    command(0x2A);  //function set (extended command set)
    command(0x79);  //OLED command set enabled
    command(0xD5);  //set display clock divide ratio/oscillator frequency
    command(0x70);  //set display clock divide ratio/oscillator frequency
    command(0x78);  //OLED command set disabled
    command(0x09);  //extended function set (4-lines)
    command(0x06);  //COM SEG direction
    command(0x72);  //function selection B, disable internal Vdd regualtor
    data(0x00);     //ROM CGRAM selection
    command(0x2A);  //function set (extended command set)
    command(0x79);  //OLED command set enabled
    command(0xDA);  //set SEG pins hardware configuration
    command(0x00); //set SEG pins ... NOTE: When using NHD-0216AW-XB3 or NHD_0216MW_XB3 change to (0x00)
    command(0xDC);  //function selection C
    command(0x00);  //function selection C
    command(0x81);  //set contrast control
    command(0x7F);  //set contrast control
    command(0xD9);  //set phase length
    command(0xF1);  //set phase length
    command(0xDB);  //set VCOMH deselect level
    command(0x40);  //set VCOMH deselect level
    command(0x78);  //OLED command set disabled
    command(0x28);  //function set (fundamental command set)
    command(0x01);  //clear display
    command(0x80);  //set DDRAM address to 0x00
    command(0x0C);  //display ON
    __delay_cycles(200);
}

void blocks()
{
    int i;

    command(0x01);
    __delay_cycles(200);

    for (i = 0; i < 15; i++)
    {
        data(0x1F);
    }

    command(0xA0);
    for (i = 0; i < 15; i++)
    {
        data(0x1F);
    }
}

//******************************************************************************
// Main ************************************************************************
// Send and receive three messages containing the example commands *************
//******************************************************************************

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
    initClockTo16MHz();
    initGPIO();
    initI2C();

    initOled();
    blocks();
    while (1)
    {
        I2C_Master_ReadReg(SLAVE_ADDR, 0x00, 3);
        __no_operation();
        __delay_cycles(30000);
        I2C_Master_ReadReg(SLAVE_ADDR, 0x71, 1);
        I2C_Master_WriteReg(SLAVE_ADDR, 0xAC, MasterType2, 2);
        __delay_cycles(30000);
        I2C_Master_ReadReg(SLAVE_ADDR, 0x71, 6);
        CopyArray(ReceiveBuffer, HumidityData, HDataLength, 1);
        CopyArray(ReceiveBuffer, TempData, TDataLength, 3);
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
        output(humStr, tempStr);
        __no_operation();
        __delay_cycles(5000000);
    }
}

//******************************************************************************
// I2C Interrupt ***************************************************************
//******************************************************************************

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCI_B1_VECTOR
__interrupt void USCI_B1_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_B1_VECTOR))) USCI_B1_ISR (void)
#else
#error Compiler not supported!
#endif
{
    //Must read from UCB1RXBUF
    uint8_t rx_val = 0;
    switch (__even_in_range(UCB1IV, USCI_I2C_UCBIT9IFG))
    {
    case USCI_NONE:
        break;         // Vector 0: No interrupts
    case USCI_I2C_UCALIFG:
        break;         // Vector 2: ALIFG
    case USCI_I2C_UCNACKIFG:                // Vector 4: NACKIFG
        break;
    case USCI_I2C_UCSTTIFG:
        break;         // Vector 6: STTIFG
    case USCI_I2C_UCSTPIFG:
        break;         // Vector 8: STPIFG
    case USCI_I2C_UCRXIFG3:
        break;         // Vector 10: RXIFG3
    case USCI_I2C_UCTXIFG3:
        break;         // Vector 12: TXIFG3
    case USCI_I2C_UCRXIFG2:
        break;         // Vector 14: RXIFG2
    case USCI_I2C_UCTXIFG2:
        break;         // Vector 16: TXIFG2
    case USCI_I2C_UCRXIFG1:
        break;         // Vector 18: RXIFG1
    case USCI_I2C_UCTXIFG1:
        break;         // Vector 20: TXIFG1
    case USCI_I2C_UCRXIFG0:                 // Vector 22: RXIFG0
        rx_val = UCB1RXBUF;
        if (RXByteCtr)
        {
            ReceiveBuffer[ReceiveIndex++] = rx_val;
            RXByteCtr--;
        }

        if (RXByteCtr == 1)
        {
            UCB1CTLW0 |= UCTXSTP;
        }
        else if (RXByteCtr == 0)
        {
            UCB1IE &= ~UCRXIE;
            MasterMode = IDLE_MODE;
            __bic_SR_register_on_exit(CPUOFF);      // Exit LPM0
        }
        break;
    case USCI_I2C_UCTXIFG0:                 // Vector 24: TXIFG0
        switch (MasterMode)
        {
        case TX_REG_ADDRESS_MODE:
            UCB1TXBUF = TransmitRegAddr;
            if (RXByteCtr)
                MasterMode = SWITCH_TO_RX_MODE;   // Need to start receiving now
            else
                MasterMode = TX_DATA_MODE; // Continue to transmision with the data in Transmit Buffer
            break;

        case SWITCH_TO_RX_MODE:
            UCB1IE |= UCRXIE;              // Enable RX interrupt
            UCB1IE &= ~UCTXIE;             // Disable TX interrupt
            UCB1CTLW0 &= ~UCTR;            // Switch to receiver
            MasterMode = RX_DATA_MODE;    // State state is to receive data
            UCB1CTLW0 |= UCTXSTT;          // Send repeated start
            if (RXByteCtr == 1)
            {
                //Must send stop since this is the N-1 byte
                while ((UCB1CTLW0 & UCTXSTT))
                    ;
                UCB1CTLW0 |= UCTXSTP;      // Send stop condition
            }
            break;

        case TX_DATA_MODE:
            if (TXByteCtr)
            {
                UCB1TXBUF = TransmitBuffer[TransmitIndex++];
                TXByteCtr--;
            }
            else
            {
                //Done with transmission
                UCB1CTLW0 |= UCTXSTP;     // Send stop condition
                MasterMode = IDLE_MODE;
                UCB1IE &= ~UCTXIE;                       // disable TX interrupt
                __bic_SR_register_on_exit(CPUOFF);      // Exit LPM0
            }
            break;

        default:
            __no_operation();
            break;
        }
        break;
    default:
        break;
    }
}

