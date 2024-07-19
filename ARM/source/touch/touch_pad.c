/********************************************************************************/
/* touch_pad.c                                                                  */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#include "../../hwdefs.h"
#include "../prototype.h"

#define TPAD_GATE_VAL 			100
#define TPAD_ARR_MAX_VAL		0xFFFF

volatile unsigned short tpad_default_val = 0;



void TPAD_Reset (void)
{
      GPIO_Init_Pin(GPIOA,GPIO_Pin_5,GPIO_Speed_100MHz,GPIO_Mode_Out_PP_PD);
      GPIO_ResetBits(GPIOA,GPIO_Pin_5);		// OUT 0
      delay_us(100);
      TIM_ClearITPendingBit(TIM2, TIM_IT_CC1 | TIM_IT_Update);
      TIM_SetCounter(TIM2,0);
      GPIO_Init_Pin(GPIOA,GPIO_Pin_5,GPIO_Speed_100MHz,GPIO_Mode_AF_PP);
}


unsigned short TPAD_Get_Val (void)
{
      TPAD_Reset();
      while (TIM_GetFlagStatus(TIM2, TIM_IT_CC1) == RESET) {
	    if (TIM_GetCounter(TIM2) > (TPAD_ARR_MAX_VAL - 500)) return TIM_GetCounter(TIM2);
      }
      return TIM_GetCapture1(TIM2);
}


unsigned short TPAD_Get_MaxVal (unsigned char n)
{
      unsigned short temp = 0;
      unsigned short res = 0;
      while (n--) {
	    temp = TPAD_Get_Val();
	    if (temp >res) res = temp;
      }
      return res;
}


unsigned char TPAD_Scan (unsigned char mode)
{
      static unsigned char key_en = 0;
      unsigned char res = 0;
      unsigned char sample = 3;
      unsigned short rval;
      if (mode)	{
         sample = 6;
	 key_en = 0;
      }
      rval = TPAD_Get_MaxVal(sample);
      if ((rval > (tpad_default_val + TPAD_GATE_VAL)) && (rval < (10 * tpad_default_val))) {
	 if ((key_en == 0) && (rval > (tpad_default_val + TPAD_GATE_VAL))) {
	    res = 1;
	 }
	 key_en = 3;
      }
      if (key_en) key_en--;
      return res;
}


void TIM2_CH1_Cap_Init (unsigned int arr,unsigned short psc)
{
      TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
      TIM_ICInitTypeDef TIM2_ICInitStructure;
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
      GPIO_PinAFConfig(GPIOA,GPIO_PinSource5,GPIO_AF_TIM2);
      GPIO_Init_Pin(GPIOA,GPIO_Pin_5,GPIO_Speed_100MHz,GPIO_Mode_AF_PP);
      TIM_TimeBaseStructure.TIM_Period = arr;
      TIM_TimeBaseStructure.TIM_Prescaler = psc;
      TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
      TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
      TIM_TimeBaseInit(TIM2,&TIM_TimeBaseStructure);
      TIM2_ICInitStructure.TIM_Channel = TIM_Channel_1;
      TIM2_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
      TIM2_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
      TIM2_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
      TIM2_ICInitStructure.TIM_ICFilter = 0x00;
      TIM_ICInit(TIM2, &TIM2_ICInitStructure);
      TIM_Cmd(TIM2,ENABLE );
}


unsigned char TPAD_Init (unsigned char psc)
{
      unsigned short buf[10];
      unsigned short temp;
      unsigned char i,j;
      TIM2_CH1_Cap_Init(TPAD_ARR_MAX_VAL,psc - 1);
      for (i=0;i<10;i++) {
	  buf[i] = TPAD_Get_Val();
	  wait_ms(10);
      }
      for (i=0;i<9;i++) {
	  for (j=i+1;j<10;j++) {
	      if (buf[i] > buf[j]) {
		 temp = buf[i];
		 buf[i] = buf[j];
		 buf[j] = temp;
	      }
	  }
      }
      temp = 0;
      for (i=2;i<8;i++) {
          temp += buf[i];
      }
      tpad_default_val = temp / 6;
      lcd_printf(1,2,"Tpad default: %d",tpad_default_val);
      if ((tpad_default_val) > (TPAD_ARR_MAX_VAL / 2)) return 1;
      return 0;
}
