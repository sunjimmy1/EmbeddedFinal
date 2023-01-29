/*
 * Temperature.h
 *
 *  Created on: Dec 16, 2022
 *      Author: jimmy
 */

#ifndef TEMPHUM_H_
#define TEMPHUM_H_

#define TEMPADDR 0x38
#define HDataLength   3
#define TDataLength   3

#include <I2C.h>
#include <stdint.h>

float *readTempData();

#endif /* TEMPHUM_H_ */
