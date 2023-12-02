/*
 * cli.c
 *
 *  Created on: Nov 11, 2023
 *      Author: Tomer
 */
#include "cmsis_os.h"
#include "cli.h"
#include "FreeRtos_CLI.h"
#include "i2c.h"
#include <stdio.h>



#define RX_BIT    0x01


CyclicQBuffer_t uartBuffer;
osThreadId Task1Handle;

void cliTask1(void const * argument);
void notifyCliTask( void );
static BaseType_t prvTestCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static BaseType_t i2cReadMem( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static BaseType_t i2cWriteMem( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );


static const CLI_Command_Definition_t xTestCommand =
{
    "test",
    "test: dump debug counter ",
    prvTestCommand,
    0
};

static const CLI_Command_Definition_t i2c_write_mem =
{
    "i2c_write_mem",
    "i2c_write_mem: write x bytes (chars) to memory starting from a chosen address in a EEprom ",
	i2cWriteMem,
    2
};


static const CLI_Command_Definition_t i2c_read_mem =
{
    "i2c_read_mem",
    "i2c_read_mem: Read x bytes starting from a chosen address in a EEprom ",
	i2cReadMem,
    2
};

  /* Create the thread(s) */
  /* definition and creation of Task1 */
void createCliTask()
{
	BaseType_t xReturned;
	initCyclicQBuffer(&uartBuffer);
	FreeRTOS_CLIRegisterCommand( &xTestCommand );
	FreeRTOS_CLIRegisterCommand( &i2c_read_mem );
	FreeRTOS_CLIRegisterCommand( &i2c_write_mem );
	//osThreadDef(Task1, cliTask1, osPriorityNormal, 0, 128);
	//Task1Handle = osThreadCreate(osThread(Task1), NULL);
    xReturned = xTaskCreate(
    				(TaskFunction_t )cliTask1,       /* Function that implements the task. */
                    "CLI_TASK",          /* Text name for the task. */
                    2048,      /* Stack size in words, not bytes. */
                    NULL,    /* Parameter passed into the task. */
					osPriorityNormal,/* Priority at which the task is created. */
                    &Task1Handle );      /* Used to pass out the created task's handle. */
}


void initCyclicQBuffer(CyclicQBuffer_t *ptr )
{
	ptr->readerIndex = 0;
	ptr->writerIndex = 0;
	ptr->q_size = RX_Q_SIZE;
	ptr->isQueueFull = 0;
}

int cliRxError = 0;

//int count=0;
//uint8_t buffer[8];

//This function is called from uart rx ISR
int cliRecvChar(uint8_t ch)
{
	uint32_t delta;
	delta = uartBuffer.writerIndex - uartBuffer.readerIndex ;
	if( delta < RX_Q_FULL )
	{
		uartBuffer.buffer[uartBuffer.writerIndex & RX_Q_MASK] = ch ;
		uartBuffer.writerIndex++;
		if(delta == 0)
		{
			notifyCliTask();
		}
	}
	else
	{
		cliRxError++;
		uartBuffer.isQueueFull = 1;
	}

	return 1; 					//check what to return
}

int readLine(char *pBuf, uint32_t max_len)
{
	uint32_t count = 0;
	uint8_t ch = 0;

	while (ch != 0x0d && ch != 0x0a &&  count < max_len)
	{
		ch = getCharFromQueue();
		if (count && (ch == 0x8 || ch == 0x7f))
		{
			count--;
		}
		else
		{
			pBuf[count++] = ch;
		}
		__io_putchar(ch);
	}
	__io_putchar('\r');
	__io_putchar('\n');
	pBuf[count -1] = 0;
	return count;
}

int count1 = 0;

char answer[248];
char buf[64]={0};
void cliTask1(void const * argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */

	//setvbuf( stdin, NULL, _IONBF, 0 );
	//scanf("%s",buf);
	int isMorePrints = 0;
	for(;;)
	{
		buf[0] = buf[1] = 0;
		readLine(buf, 63);
		do
		{
			isMorePrints = FreeRTOS_CLIProcessCommand(buf,answer,240);
			printf("%s \n",answer);
		}while(isMorePrints);
		printf("\n\r");


		//printf("%s \n",buf);
	}
  /* USER CODE END 5 */
}

/* The implementation of the transmit interrupt service routine. */
void notifyCliTask( void )
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;


   /* Notify the task that the transmission is complete by setting the TX_BIT
   in the task's notification value. */
   xTaskNotifyFromISR( Task1Handle,
                       RX_BIT,
                       eSetBits,
                       &xHigherPriorityTaskWoken );

   /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
   should be performed to ensure the interrupt returns directly to the highest
   priority task.  The macro used for this purpose is dependent on the port in
   use and may be called portEND_SWITCHING_ISR(). */
   portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}



void wait4Notification()
{
	BaseType_t xResult;
	uint32_t ulNotifiedValue;

	/* Wait to be notified of an interrupt. */
	xResult = xTaskNotifyWait( pdFALSE,    /* Don't clear bits on entry. */
					   0xffffffff,        /* Clear all bits on exit. */
					   &ulNotifiedValue, /* Stores the notified value. */
					   portMAX_DELAY);
}

uint8_t getCharFromQueue()
{
	uint8_t ch;
	uint32_t delta;
	do
	{
		delta = uartBuffer.writerIndex - uartBuffer.readerIndex ;
		if( delta == 0 )
		{
			wait4Notification();
		}
	}while(delta == 0);

	ch = uartBuffer.buffer[uartBuffer.readerIndex & RX_Q_MASK] ;
	uartBuffer.readerIndex++ ;


	return ch;

}

static BaseType_t prvTestCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	sprintf(pcWriteBuffer,"cliRxError = %d \n", cliRxError);
	return pdFALSE;
}

static BaseType_t i2cReadMem( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	char *pcParameter1, *pcParameter2;
	BaseType_t xParameter1StringLength, xParameter2StringLength, xResult;
	uint32_t hexAddress , hexLength ;


	pcParameter1 = FreeRTOS_CLIGetParameter( pcCommandString, 1, &xParameter1StringLength);
	pcParameter2 = FreeRTOS_CLIGetParameter( pcCommandString, 2, &xParameter2StringLength );

	//first parameter is the start address , the second is the number of bytes to read
	/* Terminate the parameter names file names. */
	pcParameter1[ xParameter1StringLength ] = 0x00;
	pcParameter2[ xParameter2StringLength ] = 0x00;

	sscanf(pcParameter1, "%x", &hexAddress);
	sscanf(pcParameter2, "%x", &hexLength);

	i2cReadFromMemory(hexAddress , hexLength );

	pcWriteBuffer[0] = 0;




	//dont forget to write a string to pcWriteBuffer using sprintf

	return pdFALSE;
}



static BaseType_t i2cWriteMem( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	char *pcParameter1, *pcParameter2StrToWrite;
	BaseType_t xParameter1StringLength, xParameter2StringLength, xResult;
	uint32_t hexAddress , hexLength ;


	pcParameter1 = FreeRTOS_CLIGetParameter( pcCommandString, 1, &xParameter1StringLength);
	pcParameter2StrToWrite = FreeRTOS_CLIGetParameter( pcCommandString, 2, &xParameter2StringLength );

	//first parameter is the start address , the second is the number of bytes to read
	/* Terminate the parameter names file names. */
	pcParameter1[ xParameter1StringLength ] = 0x00;
	pcParameter2StrToWrite[ xParameter2StringLength ] = 0x00;

	sscanf(pcParameter1, "%x", &hexAddress);
	//parameter 2 is already ready

	i2cWriteToMemory(hexAddress , pcParameter2StrToWrite , xParameter2StringLength);

	pcWriteBuffer[0] = 0;




	//dont forget to write a string to pcWriteBuffer using sprintf

	return pdFALSE;
}








