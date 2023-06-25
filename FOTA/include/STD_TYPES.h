/******************************************************/
/* Author    : Ahmed Assaf                            */
/* Date      : 14 SEP 2020                            */
/* Version   : V02                                    */
/* LOG       : V01 -> Initial Creation                */
/*           : V02 -> Add to singed data              */
/******************************************************/
#ifndef STD_TYPES_H
#define STD_TYPES_H

typedef unsigned char        u8 ;
typedef unsigned short int   u16;
typedef unsigned long  int   u32; 

typedef signed char        s8 ;
typedef signed short int   s16;
typedef signed long  int   s32; 

/**
 * @def     OK
 * @brief   used in Checking No Error
 */
#define OK 		0
/**
 * @def     OK
 * @brief   used in Checking Error
 */
#define NOK  	1
/**
 * @def     BUSY
 * @brief   used in Check Status
 */
#define BUSY	2
/**
 * @def     OK
 * @brief   used in Checking
 */
#define ON 		1
/**
 * @def     OFF
 * @brief   used in Checking
 */
#define OFF  	0
/**
 * @def     NULL
 */
#define NULL 							(void*)0

#endif
