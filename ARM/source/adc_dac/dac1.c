/********************************************************************************/
/* dac1.c                                                                       */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#include "../../hwdefs.h"
#include "../prototype.h"



void Dac1_Set_Voltage (unsigned short voltage)
{
      double temp = voltage;	// 1mV Range 3.3V = 3300
      temp /= 1000;
      temp = temp * 4096 / 3.3;
      if (temp > 4095.0) temp = 4095.0;
      DAC_SetChannel1Data(DAC_Align_12b_R,temp);
}


void Dac1_Init (void)
{  
      DAC_InitTypeDef DAC_InitType;	
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC,ENABLE);
      GPIO_Init_Pin(GPIOA,GPIO_Pin_4,GPIO_Speed_100MHz,GPIO_Mode_ADC);   
      DAC_InitType.DAC_Trigger = DAC_Trigger_None;
      DAC_InitType.DAC_WaveGeneration = DAC_WaveGeneration_None;
      DAC_InitType.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
      DAC_InitType.DAC_OutputBuffer = DAC_OutputBuffer_Disable;
      DAC_Init(DAC_Channel_1,&DAC_InitType);
      DAC_Cmd(DAC_Channel_1,ENABLE);  
      DAC_SetChannel1Data(DAC_Align_12b_R,0);
}

