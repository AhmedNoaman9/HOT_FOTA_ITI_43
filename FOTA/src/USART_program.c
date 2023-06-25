
#include "STD_TYPES.h"
#include "BIT_MATH.h"

#include "USART_interface.h"
#include "USART_private.h"
#include "USART_config.h"


#define USART_TC_FLAG					0x00000040
#define USART_RXNE_FLAG					0x00000020

typedef struct
{
	u8 *Data;
	u32 Position;
	u32 Size;
	u8 BufferState;
}DataBuffer_t;
static DataBuffer_t RxBuffer;

typedef void (*RxCbf_t)(void);

static RxCbf_t RxNotify;

#define STATE_IDLE	0
/**
 *	@def 	STATE_BUSY
 *  @brief 	Busy State
 */
#define STATE_BUSY	1

#define USART_RXNEIE_ENABLE				0x00000020

void MUSART1_voidInit(void)
{
	/*	baud rate = 9600		*/
	USART1 -> BRR = 0x341;

	SET_BIT((USART1-> CR[0]), 3);			/* Enabling Transmitter */
	SET_BIT((USART1-> CR[0]), 2);			/* Enabling Receiver */
	SET_BIT((USART1-> CR[0]), 13);			/* Enabling USART */

	USART1 -> SR = 0;						/* Clearing status register */
}

void MUSART1_CharTransmit(u8 arr)
{
	USART1 -> DR = arr;
	while((GET_BIT((USART1 -> SR), 6)) == 0);
}


void MUSART1_voidTransmit(u8 arr[])
{
	u8 i = 0;
	while(arr[i] != '\0'){
		USART1 -> DR = arr[i];
		while((GET_BIT((USART1 -> SR), 6)) == 0);
		i++;
	}

}

u8 UART_Receive(u8 *Buffer, u16 Length)
{
	u8 Local_Error=OK;
	SET_BIT(USART1 ->CR[0],5);
	if(Buffer && Length>0)
	{
		if(RxBuffer.BufferState==STATE_IDLE)
		{
			RxBuffer.Size=Length;
			RxBuffer.Position=0;
			RxBuffer.Data=Buffer;
			RxBuffer.BufferState=STATE_BUSY;
		}
		else
		{
			Local_Error=BUSY;
		}
	}
	else
	{
		Local_Error= NOK;
	}
	return Local_Error;
}


u8 MUSART1_u8Receive(u8* Copy_u8Data)
{
	u8 Local_u8Result = 1;

	if ( (GET_BIT((USART1 -> SR), 5)) == 1 )
	{
		*Copy_u8Data = USART1 -> DR;
	}
	else{
		Local_u8Result = 0;
	}

	return (Local_u8Result);
}

u8 UART_SetRxCbf(RxCbf_t RxCbf)
{
	u8 Local_Error=OK;
	RxNotify=RxCbf;
	return Local_Error;
}

void USART1_IRQHandler(void)
{
	if(USART1->SR & USART_RXNE_FLAG)
	{
		if(RxBuffer.BufferState==STATE_BUSY)
		{
			if(RxBuffer.Position != RxBuffer.Size)
			{
				USART1->SR &= ~USART_RXNE_FLAG;
				RxBuffer.Data[RxBuffer.Position]=USART1->DR;
				RxBuffer.Position++;
			}
			else
			{
				RxBuffer.Data=NULL;
				RxBuffer.Position=0;
				RxBuffer.Size=0;
				RxBuffer.BufferState=STATE_IDLE;
				//USART1->CR &= ~USART_RXNEIE_ENABLE;
				if(RxNotify)
				{
					RxNotify();
				}
				else
				{
					/*MISRA*/
				}
			}
		}
		else
		{
			/*MISRA*/
		}

	}
	else
	{
		/*MISRA*/
	}
}

















