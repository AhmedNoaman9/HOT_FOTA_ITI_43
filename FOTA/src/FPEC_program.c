#include "STD_TYPES.h"
#include "BIT_MATH.h"

#include "FPEC_interface.h"
#include "FPEC_private.h"
#include "FPEC_config.h"

/**
 * @def		FPEC
 * @brief 	Casting Base Address of Flash Driver as Pointer to struct FLITF_t
 *
 */
//#define FPEC ((FLITF_t *)(0x40022000))
/**
 * @def 	HALF_WORD_LEN
 * @brief 	HNo of bytes of Half Word
 */
#define HALF_WORD_LEN  2
/**
 *	@def  	KEY1
 *  @brief 	KEY1 to unlock Flash to write on it
 */
#define KEY1 ( 0x45670123 )
/**
 *	@def  	KEY2
 *  @brief 	KEY2 to unlock Flash to write on it
 */
#define KEY2 ( 0xcdef89ab )


/* CR Register*/
/**
 * @def 	LOCK
 * @brief 	used to lock Flash
 */
#define LOCK 						0x00000080
/**
 * @def 	START
 * @brief 	used to trigger an ERASE operation when set
 */
#define START  						0x00000040
/**
 * @def 	MASS_ERASE
 * @brief 	used to earse all the Flash
 */
#define MASS_ERASE 					0x00000004
/**
 * @def 	PAGE_ERASE
 * @brief 	used to earse page
 */
#define PAGE_ERASE 					0x00000002
/**
 * @def 	PROGRAM_ENABLE
 * @brief 	used to enable  falsh programming
 */
#define PROGRAM_ENABLE  			0x00000001
/**
 * @def 	MER_RESET
 * @brief 	used to reset Mass Erase bit.
 */
#define MER_RESET					0x00001FFB
/* SR Register */
/**
 * @def 	EOP
 * @brief 	used to indicate that a Flash operation is completed
 */
#define EOP							0x00000020


void Flash_Lock(void){
	FPEC->CR |= LOCK;
}

void Flash_Unlock(void){

	FPEC->KEYR = (u32)KEY1;

	FPEC->KEYR = (u32)KEY2;

}

void Flash_ErasePage(u32 PageAddress){

	while(FPEC->SR & BUSY);
	FPEC->CR |= PAGE_ERASE;
	FPEC->AR  = PageAddress;
	FPEC->CR |= START;
	while(FPEC->SR & BUSY);
	//FPEC->FLASH_SR |= EOP;
	FPEC->CR &= ~PAGE_ERASE;
}

void FPEC_voidEraseBackUpArea(void)
{
	u8 i;

	for (i= 51;i<64;i++)
	{
		FPEC_voidFlashPageErase(i);
	}
}
void FPEC_voidEraseAppArea(void)
{
	u8 i;
	
	for (i= 12;i<64;i++)
	{
		FPEC_voidFlashPageErase(i);
	}
}



void FPEC_voidFlashPageErase(u8 Copy_u8PageNumber)
{
	/* Wait Busy Flag */
	while (GET_BIT(FPEC->SR,0) == 1);

	/* Check if FPEC is locked or not */
	if ( GET_BIT(FPEC->CR,7) == 1)
	{
		FPEC -> KEYR = 0x45670123;
		FPEC -> KEYR = 0xCDEF89AB;
	}
	
	/* Page Erase Operation */
	SET_BIT(FPEC->CR,1);

	/* Write Page address */
	FPEC->AR = (u32)(Copy_u8PageNumber * 1024) + 0x08000000 ;

	/* Start operation */
	SET_BIT(FPEC->CR,6);

	/* Wait Busy Flag */
	while (GET_BIT(FPEC->SR,0) == 1);

	/* EOP */
	SET_BIT(FPEC->SR,5);
	CLR_BIT(FPEC->CR,1);
}

void FPEC_voidFlashWrite(u32 Copy_u32Address, u16* Copy_u16Data, u8 Copy_u8Length)
{
	u8 i;
	volatile u16 Temp;

	while (GET_BIT(FPEC->SR,0) == 1);

	/* Check if FPEC is locked or not */
	if ( /* FPEC_CR->BitAccess.LOCK == 1 */ GET_BIT(FPEC->CR,7) == 1 )
	{
		FPEC -> KEYR = 0x45670123;
		FPEC -> KEYR = 0xCDEF89AB;
	}
	
	
	for (i = 0; i< Copy_u8Length; i++)
	{
		/* Write Flash Programming */
		SET_BIT(FPEC->CR,0);

		/* Half word operation */

		Temp = Copy_u16Data[i];
		*((volatile u16*)Copy_u32Address) = Copy_u16Data[i];
		Copy_u32Address += 2 ;

		/* Wait Busy Flag */
		while (GET_BIT(FPEC->SR,0) == 1);
	
		/* EOP */
		SET_BIT(FPEC->SR,5);
		CLR_BIT(FPEC->CR,0);
	}

}


void Flash_ProgramWrite(void * StartAddress,void * DataAddress,u32 DataLength){
	u32 Iterator=0;
	u16 temp;
	while(FPEC->SR & BUSY);
	FPEC->CR |= PROGRAM_ENABLE;
	while((DataLength - 2*Iterator) >= HALF_WORD_LEN ){
		( ( u16 *) StartAddress )[Iterator]  = ( ( u16 *) DataAddress )[Iterator];
		Iterator ++;
	}
	if((DataLength - 2*Iterator)){
		temp =(( (u8 *) DataAddress )[2*Iterator])  | 0xff00;
		( (u16 *) StartAddress )[Iterator] =temp ;
	}
	while(FPEC->SR & BUSY);
	FPEC->SR |= EOP;
	FPEC->CR &= ~PROGRAM_ENABLE;

}
