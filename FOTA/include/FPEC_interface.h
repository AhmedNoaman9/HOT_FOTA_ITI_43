#ifndef FPEC_INTERFACE
#define FPEC_INTERFACE




void 	Flash_Lock(void);
void 	Flash_Unlock(void);

void FPEC_voidEraseAppArea(void);
void FPEC_voidEraseBackUpArea(void);

void Flash_ErasePage(u32);

void FPEC_voidFlashPageErase(u8 Copy_u8PageNumber);

void FPEC_voidFlashWrite(u32 Copy_u32Address, u16* Copy_u16Data, u8 Copy_u8Length);

void Flash_ProgramWrite(void * StartAddress,void * DataAddress,u32 DataLength);



#endif
