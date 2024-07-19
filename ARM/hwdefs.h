/********************************************************************************/
/* hwdefs.h                                                                     */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#ifndef __HWDEFS_H
#define __HWDEFS_H

#ifndef __GNUC__
#define __GNUC__
#endif

#ifndef ARM_MATH_CM4
#define ARM_MATH_CM4
#endif

#ifndef __FPU_PRESENT
#define __FPU_PRESENT	1
#endif

#define assert_param(X)			{}

#define RAMFUNC				__attribute__ ((long_call, section(".ramfunc")))
#define UNUSED(x)			(void)(x)	//(void)(sizeof(x), 0)

#include <stm32f4xx.h>
#include <fundefs_stm32f4xx.h>
#include "userdefs.h"

#endif	/* __HWDEFS_H */