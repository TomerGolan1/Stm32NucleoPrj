/*
 * cli.h
 *
 *  Created on: Nov 11, 2023
 *      Author: Tomer
 */

#ifndef INC_CLI_H_
#define INC_CLI_H_


void createCliTask();

int cliRecvChar(uint8_t ch);

#define RX_Q_SIZE 8
#define RX_Q_MASK (RX_Q_SIZE - 1)
#define RX_Q_FULL (RX_Q_SIZE - 2)

typedef struct CyclicQBuffer
{
	uint8_t buffer[64];
	uint32_t readerIndex;
	uint32_t writerIndex;
	uint32_t q_size;
	uint32_t isQueueFull;
}CyclicQBuffer_t;




void initCyclicQBuffer(CyclicQBuffer_t* ptr );
uint8_t getCharFromQueue();



#endif /* INC_CLI_H_ */
