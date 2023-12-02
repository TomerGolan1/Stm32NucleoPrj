/*
 * i2c.c
 *
 *  Created on: Nov 25, 2023
 *      Author: Tomer
 */

#include "cmsis_os.h"
#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "i2c.h"

static I2C_HandleTypeDef *pI2cHal = NULL;

void updateI2cHalPtr(void *p)
{
	pI2cHal = (I2C_HandleTypeDef *)p;
	//HAL_I2C_Mem_Write(pI2cHal, 0xA0, 0x520, 2, "hello", 5, 1000000);  //for test
}

#define I2C_DEVICE_ADDR 0xA0
void i2cReadFromMemory(uint32_t address, uint32_t length)
{
	int i , k ;
	int numLines = (length >> 4) +1 ;
	HAL_StatusTypeDef retVal = HAL_ERROR;
	uint32_t timeout = 1000000;
	uint8_t buffer[20];
	if (pI2cHal)
	{
		buffer[0] = address >> 8;		//MSByte
		buffer[1] = address & 0xff;		//LSByte
		retVal = HAL_I2C_Master_Transmit(pI2cHal , I2C_DEVICE_ADDR ,  buffer , 2 , timeout);
		retVal = HAL_I2C_Master_Receive(pI2cHal, I2C_DEVICE_ADDR, buffer, 16, timeout);
	}
	for(i=0 ; i< numLines ; i++)
	{
		printf("0x%02x:" , address);
		for(k=0 ; k<16 && length>0 ; k++)
		{
			printf(" 0x%02x ", buffer[16*i + k] );
			address++;
			length--;
		}
		printf("\n");
	}
    //printf("Address: 0x%x: %x \n", address buffer[3]);
}


void i2cWriteToMemory(uint32_t address, uint8_t *data , uint8_t length)
{
	uint32_t timeout = 1000000;
	HAL_StatusTypeDef retVal = HAL_ERROR;
	if (pI2cHal)
	{
		retVal = HAL_I2C_Mem_Write( pI2cHal, I2C_DEVICE_ADDR, address, 2 , data , length , timeout);

	}
	//return retVal;
}




