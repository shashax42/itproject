/********************************************************************************/
/* userdefs.h                                                                   */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#ifndef __USERDEFS_H
#define __USERDEFS_H

// user code
// PORT_A
#define TXD2				GPIO_Pin_2	// Port A, Serial
#define RXD2				GPIO_Pin_3	// Port A, Serial
#define TXD1				GPIO_Pin_9	// Port A, Serial
#define RXD1				GPIO_Pin_10	// Port A, Serial

// PORT_B
#define TXD3				GPIO_Pin_10	// Port B, Serial
#define RXD3				GPIO_Pin_11	// Port B, Serial

// PORT_C-E

// PORT F
#define LED0				GPIO_Pin_9	// Port F, OUTPUT
#define LED1				GPIO_Pin_10	// Port F, OUTPUT

// PORT G
#define TXEN				GPIO_Pin_8	// Port G, OUTPUT


// BITBAND    STM32F4X
#define BITBAND(address, bit_number)	((address & 0xF0000000) + 0x2000000 + ((address & 0xFFFFF) << 5) + (bit_number << 2))
#define MEM_ADDR(address)  		*((volatile unsigned long  *)(address))
#define BIT_ADDR(address, bit_number) 	MEM_ADDR(BITBAND(address, bit_number))

#define GPIOA_ODR_ADDRESS 		(GPIOA_BASE + 20)
#define GPIOB_ODR_ADDRESS 		(GPIOB_BASE + 20)
#define GPIOC_ODR_ADDRESS 		(GPIOC_BASE + 20)
#define GPIOD_ODR_ADDRESS 		(GPIOD_BASE + 20)
#define GPIOE_ODR_ADDRESS 		(GPIOE_BASE + 20)
#define GPIOF_ODR_ADDRESS 		(GPIOF_BASE + 20)
#define GPIOG_ODR_ADDRESS 		(GPIOG_BASE + 20)

#define GPIOA_IDR_ADDRESS 		(GPIOA_BASE + 16)
#define GPIOB_IDR_ADDRESS 		(GPIOB_BASE + 16)
#define GPIOC_IDR_ADDRESS 		(GPIOC_BASE + 16)
#define GPIOD_IDR_ADDRESS 		(GPIOD_BASE + 16)
#define GPIOE_IDR_ADDRESS 		(GPIOE_BASE + 16)
#define GPIOF_IDR_ADDRESS 		(GPIOF_BASE + 16)
#define GPIOG_IDR_ADDRESS 		(GPIOG_BASE + 16)

#define PA_OUTPUT(n)			BIT_ADDR(GPIOA_ODR_ADDRESS,n)
#define PA_INPUT(n)    			BIT_ADDR(GPIOA_IDR_ADDRESS,n)

#define PB_OUTPUT(n)			BIT_ADDR(GPIOB_ODR_ADDRESS,n)
#define PB_INPUT(n) 			BIT_ADDR(GPIOB_IDR_ADDRESS,n)

#define PC_OUTPUT(n)			BIT_ADDR(GPIOC_ODR_ADDRESS,n)
#define PC_INPUT(n) 			BIT_ADDR(GPIOC_IDR_ADDRESS,n)

#define PD_OUTPUT(n)			BIT_ADDR(GPIOD_ODR_ADDRESS,n)
#define PD_INPUT(n) 			BIT_ADDR(GPIOD_IDR_ADDRESS,n)

#define PE_OUTPUT(n)			BIT_ADDR(GPIOE_ODR_ADDRESS,n)
#define PE_INPUT(n) 			BIT_ADDR(GPIOE_IDR_ADDRESS,n)

#define PF_OUTPUT(n)			BIT_ADDR(GPIOF_ODR_ADDRESS,n)
#define PF_INPUT(n) 			BIT_ADDR(GPIOF_IDR_ADDRESS,n)

#define PG_OUTPUT(n)			BIT_ADDR(GPIOG_ODR_ADDRESS,n)
#define PG_INPUT(n) 			BIT_ADDR(GPIOG_IDR_ADDRESS,n)

#define LED_OUT0			PF_OUTPUT(9)
#define LED_OUT1			PF_OUTPUT(10)

#define TXEN_485			PG_OUTPUT(8)

#define ON				1
#define OFF				0


typedef union {
      unsigned long reg;
      unsigned char data[4];
} U32_REG;


typedef union {
      unsigned short reg;
      unsigned char data[2];
} U16_REG;

#endif	/* __USERDEFS_H */