/********************************************************************************/
/* ext_sram.c                                                                   */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#include "../../hwdefs.h"
#include "../prototype.h"


#define Bank1_SRAM3_ADDR		((unsigned int)(0x68000000))


void FSMC_SRAM_Init (void)
{
      FSMC_NORSRAMInitTypeDef FSMC_NORSRAMInitStructure;
      FSMC_NORSRAMTimingInitTypeDef readWriteTiming;
      GPIO_InitTypeDef GPIO_InitStructure;
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOG, ENABLE);
      RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_FSMC,ENABLE);
      GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
      GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
      GPIO_Init(GPIOB, &GPIO_InitStructure);
      GPIO_InitStructure.GPIO_Pin = (3 << 0) | (3 << 4) | (0xFF << 8);
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
      GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
      GPIO_Init(GPIOD, &GPIO_InitStructure);
      GPIO_InitStructure.GPIO_Pin = (3 << 0) | (0x1FF << 7);
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
      GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
      GPIO_Init(GPIOE, &GPIO_InitStructure);
      GPIO_InitStructure.GPIO_Pin = (0x3F << 0) | (0xF << 12);
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
      GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
      GPIO_Init(GPIOF, &GPIO_InitStructure);
      GPIO_InitStructure.GPIO_Pin =(0x3F << 0) | GPIO_Pin_10;
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
      GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
      GPIO_Init(GPIOG, &GPIO_InitStructure);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource0,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource1,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource4,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource5,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource8,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource9,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource10,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource11,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource12,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource13,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource14,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource15,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource0,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource1,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource7,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource8,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource9,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource10,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource11,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource12,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource13,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource14,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource15,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOF,GPIO_PinSource0,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOF,GPIO_PinSource1,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOF,GPIO_PinSource2,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOF,GPIO_PinSource3,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOF,GPIO_PinSource4,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOF,GPIO_PinSource5,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOF,GPIO_PinSource12,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOF,GPIO_PinSource13,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOF,GPIO_PinSource14,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOF,GPIO_PinSource15,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOG,GPIO_PinSource0,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOG,GPIO_PinSource1,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOG,GPIO_PinSource2,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOG,GPIO_PinSource3,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOG,GPIO_PinSource4,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOG,GPIO_PinSource5,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOG,GPIO_PinSource10,GPIO_AF_FSMC);
      readWriteTiming.FSMC_AddressSetupTime = 0x00;
      readWriteTiming.FSMC_AddressHoldTime = 0x00;
      readWriteTiming.FSMC_DataSetupTime = 0x08;
      readWriteTiming.FSMC_BusTurnAroundDuration = 0x00;
      readWriteTiming.FSMC_CLKDivision = 0x00;
      readWriteTiming.FSMC_DataLatency = 0x00;
      readWriteTiming.FSMC_AccessMode = FSMC_AccessMode_A;
      FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM3;
      FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
      FSMC_NORSRAMInitStructure.FSMC_MemoryType =FSMC_MemoryType_SRAM;
      FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
      FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode =FSMC_BurstAccessMode_Disable;
      FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
      FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait=FSMC_AsynchronousWait_Disable;
      FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
      FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
      FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
      FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
      FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
      FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
      FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &readWriteTiming;
      FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &readWriteTiming;
      FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);
      FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM3, ENABLE);
}


void FSMC_SRAM_WriteBuffer (unsigned char *pBuffer,unsigned int WriteAddr,unsigned int n)
{
      for (;n!=0;n--) {
	  *(volatile unsigned char*)(Bank1_SRAM3_ADDR+WriteAddr) = *pBuffer;
	  WriteAddr++;
	  pBuffer++;
      }
}


void FSMC_SRAM_ReadBuffer (unsigned char *pBuffer,unsigned int ReadAddr,unsigned int n)
{
      for (;n!=0;n--) {
	  *pBuffer++ = *(volatile unsigned char*)(Bank1_SRAM3_ADDR + ReadAddr);
	  ReadAddr++;
      }
}


void fsmc_sram_test_write (unsigned int addr,unsigned char data)
{
      FSMC_SRAM_WriteBuffer(&data,addr,1);
}


unsigned char fsmc_sram_test_read (unsigned int addr)
{
      unsigned char data;
      FSMC_SRAM_ReadBuffer(&data,addr,1);
      return data;
}
