/*
 * i2c.h
 *
 *  Created on: Nov 25, 2023
 *      Author: Tomer
 */

#ifndef INC_I2C_H_
#define INC_I2C_H_




void i2cReadFromMemory(uint32_t address, uint32_t length);

void i2cWriteToMemory(uint32_t address, uint8_t *data , uint8_t length);

void updateI2cHalPtr(void *p);

#endif /* INC_I2C_H_ */
