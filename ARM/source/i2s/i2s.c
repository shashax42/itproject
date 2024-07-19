/********************************************************************************/
/* i2s.c                                                                        */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#include "../../hwdefs.h"
#include "../prototype.h"


void (*i2s_tx_callback)(void);
void (*i2s_rx_callback)(void);



void I2S2_Init (unsigned short I2S_Standard,unsigned short I2S_Mode,unsigned short I2S_Clock_Polarity,unsigned short I2S_DataFormat)
{
      I2S_InitTypeDef I2S_InitStructure;
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
      RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI2,ENABLE);
      RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI2,DISABLE);
      I2S_InitStructure.I2S_Mode = I2S_Mode;
      I2S_InitStructure.I2S_Standard = I2S_Standard;
      I2S_InitStructure.I2S_DataFormat = I2S_DataFormat;
      I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Disable;
      I2S_InitStructure.I2S_AudioFreq = I2S_AudioFreq_Default;
      I2S_InitStructure.I2S_CPOL = I2S_Clock_Polarity;
      I2S_Init(SPI2,&I2S_InitStructure);
      SPI_I2S_DMACmd(SPI2,SPI_I2S_DMAReq_Tx,ENABLE);
      I2S_Cmd(SPI2,ENABLE);
}


void I2S2ext_Init (unsigned short I2S_Standard,unsigned short I2S_Mode,unsigned short I2S_Clock_Polarity,unsigned short I2S_DataFormat)
{
      I2S_InitTypeDef I2S2ext_InitStructure;
      I2S2ext_InitStructure.I2S_Mode = I2S_Mode ^ (1 << 8);
      I2S2ext_InitStructure.I2S_Standard = I2S_Standard;
      I2S2ext_InitStructure.I2S_DataFormat = I2S_DataFormat;
      I2S2ext_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Disable;
      I2S2ext_InitStructure.I2S_AudioFreq = I2S_AudioFreq_Default;
      I2S2ext_InitStructure.I2S_CPOL = I2S_Clock_Polarity;
      I2S_FullDuplexConfig(I2S2ext,&I2S2ext_InitStructure);
      SPI_I2S_DMACmd(I2S2ext,SPI_I2S_DMAReq_Rx,ENABLE);
      I2S_Cmd(I2S2ext,ENABLE);
}


const unsigned short I2S_PSC_TBL[][5] = {
      {800,256,5,12,1},			//8Khz
      {1102,429,4,19,0},		//11.025Khz
      {1600,213,2,13,0},		//16Khz
      {2205,429,4,9,1},			//22.05Khz
      {3200,213,2,6,1},			//32Khz
      {4410,271,2,6,0},			//44.1Khz
      {4800,258,3,3,1},			//48Khz
      {8820,316,2,3,1},			//88.2Khz
      {9600,344,2,3,1},			//96Khz
      {17640,361,2,2,0},		//176.4Khz
      {19200,393,2,2,0},		//192Khz
};


unsigned char I2S2_SampleRate_Set (unsigned int samplerate)
{
      unsigned char i = 0;
      unsigned int tempreg = 0;
      samplerate /= 10;
      for (i=0;i<(sizeof(I2S_PSC_TBL)/10);i++) {
	  if (samplerate == I2S_PSC_TBL[i][0]) break;
      }
      RCC_PLLI2SCmd(DISABLE);
      if (i == (sizeof(I2S_PSC_TBL) / 10)) return 1;
      RCC_PLLI2SConfig((unsigned int)I2S_PSC_TBL[i][1],(unsigned int)I2S_PSC_TBL[i][2]);
      RCC->CR |= 1 << 26;
      while ((RCC->CR & 1 << 27) == 0);
      tempreg = I2S_PSC_TBL[i][3] << 0;
      tempreg |= I2S_PSC_TBL[i][4] << 8;
      tempreg |= 1 << 9;
      SPI2->I2SPR = tempreg;
      return 0;
}


void I2S2_TX_DMA_Init (unsigned char *buf0,unsigned char *buf1,unsigned short num)
{
      NVIC_InitTypeDef NVIC_InitStructure;
      DMA_InitTypeDef DMA_InitStructure;
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1,ENABLE);
      DMA_DeInit(DMA1_Stream4);
      while (DMA_GetCmdStatus(DMA1_Stream4) != DISABLE){}
      DMA_ClearITPendingBit(DMA1_Stream4,DMA_IT_FEIF4 | DMA_IT_DMEIF4 | DMA_IT_TEIF4 | DMA_IT_HTIF4 | DMA_IT_TCIF4);
      DMA_InitStructure.DMA_Channel = DMA_Channel_0;
      DMA_InitStructure.DMA_PeripheralBaseAddr = (unsigned int)&SPI2->DR;
      DMA_InitStructure.DMA_Memory0BaseAddr = (unsigned int)buf0;
      DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
      DMA_InitStructure.DMA_BufferSize = num;
      DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
      DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
      DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
      DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
      DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
      DMA_InitStructure.DMA_Priority = DMA_Priority_High;
      DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
      DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
      DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
      DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
      DMA_Init(DMA1_Stream4, &DMA_InitStructure);
      DMA_DoubleBufferModeConfig(DMA1_Stream4,(unsigned int)buf1,DMA_Memory_0);
      DMA_DoubleBufferModeCmd(DMA1_Stream4,ENABLE);
      DMA_ITConfig(DMA1_Stream4,DMA_IT_TC,ENABLE);
      NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream4_IRQn;
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
      NVIC_Init(&NVIC_InitStructure);
}


void I2S2ext_RX_DMA_Init (unsigned char *buf0,unsigned char *buf1,unsigned short num)
{

      NVIC_InitTypeDef NVIC_InitStructure;
      DMA_InitTypeDef DMA_InitStructure;
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1,ENABLE);
      DMA_DeInit(DMA1_Stream3);
      while (DMA_GetCmdStatus(DMA1_Stream3) != DISABLE){}
      DMA_ClearITPendingBit(DMA1_Stream3,DMA_IT_FEIF3 | DMA_IT_DMEIF3 | DMA_IT_TEIF3 | DMA_IT_HTIF3 | DMA_IT_TCIF3);
      DMA_InitStructure.DMA_Channel = DMA_Channel_3;
      DMA_InitStructure.DMA_PeripheralBaseAddr = (unsigned int)&I2S2ext->DR;
      DMA_InitStructure.DMA_Memory0BaseAddr = (unsigned int)buf0;
      DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
      DMA_InitStructure.DMA_BufferSize = num;
      DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
      DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
      DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
      DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
      DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
      DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
      DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
      DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
      DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
      DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
      DMA_Init(DMA1_Stream3, &DMA_InitStructure);
      DMA_DoubleBufferModeConfig(DMA1_Stream3,(unsigned int)buf1,DMA_Memory_0);
      DMA_DoubleBufferModeCmd(DMA1_Stream3,ENABLE);
      DMA_ITConfig(DMA1_Stream3,DMA_IT_TC,ENABLE);
      NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream3_IRQn;
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =0x00;
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
      NVIC_Init(&NVIC_InitStructure);
}


void DMA1_Stream4_IRQHandler (void)
{
      if (DMA_GetITStatus(DMA1_Stream4,DMA_IT_TCIF4) == SET) {
	 DMA_ClearITPendingBit(DMA1_Stream4,DMA_IT_TCIF4);
      	 i2s_tx_callback();
      }
}


void DMA1_Stream3_IRQHandler (void)
{
      if (DMA_GetITStatus(DMA1_Stream3,DMA_IT_TCIF3) == SET) {
	 DMA_ClearITPendingBit(DMA1_Stream3,DMA_IT_TCIF3);
      	 i2s_rx_callback();
      }
}


void I2S_Play_Start (void)
{
      DMA_Cmd(DMA1_Stream4,ENABLE);
}


void I2S_Play_Stop (void)
{
      DMA_Cmd(DMA1_Stream4,DISABLE);
}


void I2S_Rec_Start (void)
{
      DMA_Cmd(DMA1_Stream3,ENABLE);
}


void I2S_Rec_Stop (void)
{
      DMA_Cmd(DMA1_Stream3,DISABLE);
}
