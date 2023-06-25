/*
 * main.c
 *
 *  Created on: Oct 13, 2020
 *      Author: Ahmed
 */

#include "STD_TYPES.h"
#include "BIT_MATH.h"
#include "diag/Trace.h"
#include "RCC_interface.h"
#include "DIO_interface.h"
#include "STK_interface.h"
#include "USART_interface.h"
#include "FPEC_interface.h"

void Parser_voidParseRecord(u8* Copy_u8BufData);

#define SCB_VTOR   *((volatile u32*)0xE000ED08)
#define SCB_AIRCR *((volatile u32*) 0XE000ED0C)

#define WORD_SIZE                       4
#define PAGE_SIZE                       (u16)0x400
#define MARKER_PAGE						0x08002800
#define CLEAR_WORD						0xFFFFFFFF
#define APP_START_PAGE					0x08001000
#define APP_END_PAGE					0x0800FC00
#define BOOTLOADER_VECTOR_OFFSET		0x00000000

///////////////////////////////////////////////////////////

#define MAX_DATA_SIZE                 	1024

#define RECEIVED_OK                   	1
#define RECEIVED_NOK                  	2

#define APP_FOUND                     	0xAABBCCDD
#define FLASH_REQ                     	~0xAABBCCDD
#define FEATURE_CHANGE					0x012345678
#define NO_APP                        	0xFFFFFFFF

#define TRUE							0x25400454
#define FALSE							~0x25400454

#define RAM_MARKER_ADDRESS				0x20000000
#define RAM_FLAGF1_ADDR					0x20000004
#define RAM_FLAGF2_ADDR					0x20000008
#define RAM_FLAGF_VALUE_ENABLE				0x25400454
#define RAM_FLAGF_VALUE_DISABLE				~0x25400454
#define RAM_MARKER_VALUE				0x25400454

static void Erase_Application(void);
static void Reset_Sys(void);
static void Write_Marker_APP_exist(void);
static void Write_F1_Flag(void);
static void Write_F2_Flag(void);
static void Clear_ROM_Marker(void);

volatile u8 u8RecBuffer[100];
volatile u8 u8RecCounter = 0;
volatile u8 u8TimeOutFlag = 0;
volatile u16 u16TimerCounter = 0;
volatile u8 u8BLWriteReq = 1;

static u32 * ROM_Marker = (u32 *) (0x08002800);
static u32 * RAM_Marker = (u32 *) (0x20000000);
static u32 * F1_Flag = (u32 *) (RAM_FLAGF1_ADDR);
static u32 * F2_Flag = (u32 *) (RAM_FLAGF2_ADDR);

typedef void (*EntryPoint_t)(void);
EntryPoint_t * APP_EntryPoint = (EntryPoint_t *) (0x08001FFC);

typedef void (*Function_t)(void);
Function_t addr_to_call = 0;

void JumpToBL(void) {

#define SCB_VTOR   *((volatile u32*)0xE000ED08)

	SCB_VTOR = 0x08000000;

	addr_to_call = *(Function_t*) (0x08000004);
	addr_to_call();
}

void JumpToApp(void) {

#define SCB_VTOR   *((volatile u32*)0xE000ED08)

	SCB_VTOR = 0x08002C00;

	addr_to_call = *(Function_t*) (0x08002C04);
	addr_to_call();
}

void JumpToBackUp(void) {

#define SCB_VTOR   *((volatile u32*)0xE000ED08)

	//SCB_VTOR = 0x0800CC00; // right
	SCB_VTOR = 0x08002C00;
	addr_to_call = *((Function_t*) (0x08002C04));
	addr_to_call();
}

void main(void) {

	u8 Local_u8RecStatus;
	u16 Local_u8RecSize = 0;

	u8 Local_CommFlag = 0;
	u8 Local_Flag = 0;

	RCC_voidInitSysClock();
	RCC_voidEnableClock(RCC_APB2, 14); /* USART1 */
	RCC_voidEnableClock(RCC_APB2, 2); /* PortA  */
	RCC_voidEnableClock(RCC_APB2, 4); /* PortC  */
	RCC_voidEnableClock(RCC_AHB, 4); /* FPEC   */

	MGPIO_VidSetPinDirection(GPIOA_0, 9, 0b1010); /* TX AFPP */
	MGPIO_VidSetPinDirection(GPIOA_0, 10, 0b0100); /* RC Input Floating */

	MGPIO_VidSetPinDirection(GPIOC_2, 13, 0b0001); /* RC Input Floating */

	MGPIO_VidSetPinValue(GPIOC_2,13,HIGH);

	MUSART1_voidInit();
	MSTK_voidInit();

	//	MSTK_voidSetIntervalSingle(15000000, JumpToBL);
	while (1) {
		Local_CommFlag = 0;
		if ((*RAM_Marker == FLASH_REQ)) {
			*RAM_Marker = CLEAR_WORD;// ram marker cleared for future update requests from App
			Clear_ROM_Marker();	//if Marker wasn't rewritten -> Error in flashing
			SCB_VTOR = 0x08000000; // BL ReInvoked by APP so Vector table is reverted to BASE FLASH
			///////////////////////////////////////////////////////////////////////////////////////
			u8 buff[5] = {0};
			u8 i = 0 ;
			MGPIO_VidSetPinValue(GPIOC_2,13,HIGH);
			while (Local_CommFlag != 5) {

				//				Local_u8RecStatus = UART_Receive(buff,5);
				Local_u8RecStatus = MUSART1_u8Receive(&buff[i]);
				trace_printf("hs----%x\n", buff[i]);
				if(buff[i] == 'D') {
					Local_CommFlag++;
					i++ ;
				}
			}
			trace_printf("Handshake done\n");

			MGPIO_VidSetPinValue(GPIOC_2,13,HIGH);
			MUSART1_CharTransmit('d');
			MUSART1_CharTransmit('\n');
			MUSART1_CharTransmit('d');
			MUSART1_CharTransmit('\n');
			while (1){
				//				MGPIO_VidSetPinValue(GPIOC_2,13,LOW);
				//				Local_u8RecStatus = UART_Receive(u8RecBuffer, 100);
				Local_u8RecStatus = MUSART1_u8Receive(&(u8RecBuffer[u8RecCounter]));
				if (Local_u8RecStatus == 1) {
					//					trace_printf("%d", u8RecBuffer[u8RecCounter]);
					//					MUSART1_CharTransmit('X');
					//					MUSART1_CharTransmit('\n');
					//					MSTK_voidStopInterval();
					if (u8RecBuffer[u8RecCounter] == '!') {
						Local_u8RecSize++;
						trace_printf("Record %d Recieved\n",Local_u8RecSize);
						if (u8BLWriteReq == 1) {
							//FPEC_voidEraseBackUpArea();
							FPEC_voidEraseAppArea();
							u8BLWriteReq = 0;
						}
						/* Parse */
						Parser_voidParseRecord(u8RecBuffer);
						MUSART1_CharTransmit('d');
						MUSART1_CharTransmit('\n');
						u8RecCounter = 0;
					}

					else {
						u8RecCounter++;
					}
					if(u8RecBuffer[u8RecCounter] == '#') {
						MGPIO_VidSetPinValue(GPIOC_2,13,HIGH);
						MUSART1_CharTransmit('d');
						MUSART1_CharTransmit('\n');
						trace_printf("Flashing Done\n");
						//u8TimeOutFlag = 1;
						JumpToBackUp();
					}
					//		MSTK_voidSetIntervalSingle(15000000, JumpToApp);
				} else {

				}
			}
		}

		//	DNVIC_voidChangeVectorOffset(BOOTLOADER_VECTOR_OFFSET);
		else if ((*ROM_Marker == NO_APP) && (*RAM_Marker != FLASH_REQ)) {
			u8 buff[5] = {0};
			u8 i = 0 ;
			MGPIO_VidSetPinValue(GPIOC_2,13,HIGH);
			while (Local_CommFlag != 5) {

				//				Local_u8RecStatus = UART_Receive(buff,5);
				Local_u8RecStatus = MUSART1_u8Receive(&buff[i]);
				trace_printf("hs----%x\n", buff[i]);
				if(buff[i] == 'D') {
					Local_CommFlag++;
					i++ ;
				}
			}
			trace_printf("Handshake done\n");

			MGPIO_VidSetPinValue(GPIOC_2,13,HIGH);
			MUSART1_CharTransmit('d');
			MUSART1_CharTransmit('\n');
			MUSART1_CharTransmit('d');
			MUSART1_CharTransmit('\n');
			while (1){
				//				MGPIO_VidSetPinValue(GPIOC_2,13,LOW);
				//				Local_u8RecStatus = UART_Receive(u8RecBuffer, 100);
				Local_u8RecStatus = MUSART1_u8Receive(&(u8RecBuffer[u8RecCounter]));
				if (Local_u8RecStatus == 1) {
					if(u8RecBuffer[u8RecCounter] == '#') {
						MGPIO_VidSetPinValue(GPIOC_2,13,HIGH);
						MUSART1_CharTransmit('d');
						MUSART1_CharTransmit('\n');
						trace_printf("Flashing Done\n");
						//u8TimeOutFlag = 1;
						JumpToBackUp();
					}
					//					trace_printf("%d", u8RecBuffer[u8RecCounter]);
					//					MUSART1_CharTransmit('X');
					//					MUSART1_CharTransmit('\n');
					//					MSTK_voidStopInterval();
					if (u8RecBuffer[u8RecCounter] == '!') {
						Local_u8RecSize++;
						trace_printf("Record %d Recieved\n",Local_u8RecSize);
						if (u8BLWriteReq == 1) {
							//FPEC_voidEraseBackUpArea();
							FPEC_voidEraseAppArea();
							u8BLWriteReq = 0;
						}
						/* Parse */
						Parser_voidParseRecord(u8RecBuffer);
						MUSART1_CharTransmit('d');
						MUSART1_CharTransmit('\n');
						u8RecCounter = 0;
					}

					else {
						u8RecCounter++;
					}

					//		MSTK_voidSetIntervalSingle(15000000, JumpToApp);
				} else {

				}
			}
		}

		else if ((*ROM_Marker == APP_FOUND)
				&& (*RAM_Marker == FEATURE_CHANGE)) {

			if (*F1_Flag == TRUE) {

				Write_F1_Flag();

			}if (*F2_Flag == TRUE) {

				Write_F2_Flag();

			}

		}

	}
}

/*DEXTI_SetCBF(BootLoaderExistingAppHandler);*/
//	switch (*ROM_Marker) {
//
//	case APP_FOUND:
//		/*Initialize stack pointer*/
//		asm ("ldr r1, =_estack\n"
//				"mov sp, r1");
//		(*APP_EntryPoint)();
//		break;
//
//	case NO_APP:
//
//		/* BACKUP CODE */
//		while (1) {
//			Com_Handler();
//		}
//		break;
//
//	case FLASH_REQ:
//		u8 Local_u8RecStatus;
//		u8 Local_u8RecSize = 0;
//
//		MSTK_voidInit();
//
//		MSTK_voidSetIntervalSingle(15000000, func);
//
//		while (u8TimeOutFlag == 0) {
//
//			Local_u8RecStatus = MUSART1_u8Receive(&(u8RecBuffer[u8RecCounter]));
//			if (Local_u8RecStatus == 1) {
//
//				MSTK_voidStopInterval();
//
//				if (u8RecBuffer[u8RecCounter] == '\n') {
//
//					if (u8BLWriteReq == 1) {
//						FPEC_voidEraseAppArea();
//						u8BLWriteReq = 0;
//					}
//
//					/* Parse */
//					Parser_voidParseRecord(u8RecBuffer);
//					MUSART1_voidTransmit("ok");
//					u8RecCounter = 0;
//				}
//
//				else {
//					u8RecCounter++;
//				}
//
//				MSTK_voidSetIntervalSingle(15000000, func);
//			}
//
//			else {
//
//			}
//		}
//
//		break;
//
//	case FEATURE_CHANGE:
//
//	if()
//
//}
//}
void Erase_Application(void) {

	u32 PagesIterator;
	Flash_Unlock();
	/*Erase Application pages*/
	for (PagesIterator = APP_START_PAGE; PagesIterator <= APP_END_PAGE;
			PagesIterator += PAGE_SIZE)
	{
		Flash_ErasePage(PagesIterator);
	}
}

void Write_F1_Flag(void) {

	u8 ramArray[PAGE_SIZE ];
	u8 *MarkerPage = (u8 *) (MARKER_PAGE);
	u16 Iterator;

	/*change marker value*/
	/*Move page to RAM*/
	for (Iterator = 0; Iterator < PAGE_SIZE ; Iterator++) {
		ramArray[Iterator] = MarkerPage[Iterator];
	}
	/*Erase Flash page*/
	Flash_Unlock();
	Flash_ErasePage(MARKER_PAGE);
	/*Change marker value and erase entry point old value in the ram*/
	*(u32 *) &(ramArray[1]) = FEATURE_CHANGE;/*Marker*/
	/*Flash again the marker page after editing*/
	Flash_ProgramWrite((void *) MARKER_PAGE, ramArray, PAGE_SIZE);

}

void Write_F2_Flag(void) {

	u8 ramArray[PAGE_SIZE ];
	u8 *MarkerPage = (u8 *) (MARKER_PAGE);
	u16 Iterator;

	/*change marker value*/
	/*Move page to RAM*/
	for (Iterator = 0; Iterator < PAGE_SIZE ; Iterator++) {
		ramArray[Iterator] = MarkerPage[Iterator];
	}
	/*Erase Flash page*/
	Flash_Unlock();
	Flash_ErasePage(MARKER_PAGE);
	/*Change marker value and erase entry point old value in the ram*/
	*(u32 *) &(ramArray[2]) = FEATURE_CHANGE;/*Marker*/
	/*Flash again the marker page after editing*/
	Flash_ProgramWrite((void *) MARKER_PAGE, ramArray, PAGE_SIZE);

	//	u8 ramArray[PAGE_SIZE];
	//	u8 *MarkerPage = (u8 *) (MARKER_PAGE);
	//	u16 Iterator;
	//
	//	/*change marker value*/
	//	/*Move page to RAM*/
	//	for (Iterator = 0; Iterator < PAGE_SIZE; Iterator++) {
	//		ramArray[Iterator] = MarkerPage[Iterator];
	//	}
	//	/*Erase Flash page*/
	//	Flash_Unlock();
	//	Flash_ErasePage(MARKER_PAGE);
	//	/*Change marker value and erase entry point old value in the ram*/
	//	*(u32 *) &(ramArray[2]) = FEATURE_CHANGE;/*Marker*/
	//
	//	/*Flash again the marker page after editing*/
	//	FPEC_voidFlashWrite((u32) MARKER_PAGE, (u16*)ramArray, PAGE_SIZE);

}

void Write_Marker_APP_exist(void) {
	u8 ramArray[PAGE_SIZE ];
	u8 *MarkerPage = (u8 *) (MARKER_PAGE);
	u16 Iterator;

	/*change marker value*/
	/*Move page to RAM*/
	for (Iterator = 0; Iterator < PAGE_SIZE ; Iterator++) {
		ramArray[Iterator] = MarkerPage[Iterator];
	}
	/*Erase Flash page*/
	Flash_Unlock();
	Flash_ErasePage(MARKER_PAGE);
	/*Change marker value and erase entry point old value in the ram*/
	*(u32 *) &(ramArray[0]) = APP_FOUND;/*Marker*/
	/*Flash again the marker page after editing*/
	Flash_ProgramWrite((void *) MARKER_PAGE, ramArray, PAGE_SIZE);
}

void Clear_ROM_Marker(void) {
	u8 ramArray[PAGE_SIZE ];
	u8 *MarkerPage = (u8 *) (MARKER_PAGE);
	u16 Iterator;

	/*change marker value*/
	/*Move page to RAM*/
	for (Iterator = 0; Iterator < PAGE_SIZE ; Iterator++) {
		ramArray[Iterator] = MarkerPage[Iterator];
	}
	/*Erase Flash page*/
	Flash_Unlock();
	Flash_ErasePage(MARKER_PAGE);
	/*Change marker value and erase entry point old value in the ram*/
	*(u32 *) &(ramArray[0]) = NO_APP;/*Marker*/
	/*Flash again the marker page after editing*/
	Flash_ProgramWrite((void *) MARKER_PAGE, ramArray, PAGE_SIZE);
}

void Reset_Sys(void) {
	u32 Delay;

	for (Delay = 0; Delay < 10000; Delay++) {
		asm ("NOP");
	}
	SCB_AIRCR = 0X05FA0000 | 0x04;
	//DNVIC_voidSysReset();

	while (1)
		;

}

