/*
 * I2C.h
 *
 *  Created on: Dec 15, 2022
 *      Author: jimmy
 */

#ifndef I2C_H_
#define I2C_H_
#include <msp430.h>
#include <stdint.h>

#define MAX_BUFFER_SIZE     20

typedef enum I2C_ModeEnum
{
    IDLE_MODE,
    NACK_MODE,
    TX_REG_ADDRESS_MODE,
    RX_REG_ADDRESS_MODE,
    TX_DATA_MODE,
    RX_DATA_MODE,
    SWITCH_TO_RX_MODE,
    SWITCH_TO_TX_MODE,
    TIMEOUT_MODE
} I2C_Mode;


uint8_t* getRXBuffer();
void initGPIO();
void initClockTo16MHz();
void initI2C();
I2C_Mode I2C_Master_WriteReg(uint8_t dev_addr, uint8_t reg_addr,
                             uint8_t *reg_data, uint8_t count);
I2C_Mode I2C_Master_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t count);
void CopyArray(uint8_t *source, uint8_t *dest, uint8_t count,
               uint8_t startIndex);
#endif /* I2C_H_ */
