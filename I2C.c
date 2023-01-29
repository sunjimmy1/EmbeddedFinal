/*
 * I2C.c
 *
 *  Created on: Dec 15, 2022
 *      Author: jimmy
 */
#include <I2C.h>

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

uint8_t* getRXBuffer(){
    return ReceiveBuffer;
}

void initGPIO()
{
    // I2C pins
    P4SEL0 |= BIT6 | BIT7;
    P4SEL1 &= ~(BIT6 | BIT7);
    //PM5CTL0 &= ~LOCKLPM5;
    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
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
    UCB1CTLW0 &= ~UCSWRST;                   // Clear SW reset, resume operation
    UCB1IE |= UCNACKIE;
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
    __no_operation();
    return MasterMode;
}

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
