/********************************************************************************/
/* key.c	                                                                */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#include "../../hwdefs.h"
#include "../prototype.h"

#define KEY_TIME			10
volatile unsigned char key_cnt1,key_cnt2,key_cnt3,key_cnt4,key_value,beep_on;
volatile unsigned short beep_cnt,beep_max;



void beep_control (unsigned char ctl)
{
      if (ctl == ON) {
         beep_on = 1;
         beep_cnt = 0;
      } else {
         beep_on = 0;
         beep_cnt = 0;
         BEEP = 0;
      }
}


void beep_check (void)
{
      if (beep_on) {
         beep_cnt++;
         if (beep_cnt < 200) {
            BEEP = 1;
         } else {
            BEEP = 0;
         }
         if (beep_cnt >= beep_max) {
            beep_control(OFF);
         }
      }
}


unsigned char key_read (void)
{
      if (KEY0 == RESET) {
         key_cnt1++;
         if (key_cnt1 >= KEY_TIME) {
            key_cnt1 = 0;
            if ((key_value & 0x01) == 0) {
               key_value |= 0x01;
               //beep_control(ON);
            }
         }
      } else {
         key_cnt1 = 0;
         key_value &= ~(0x01);
      }
      if (KEY1 == RESET) {
         key_cnt2++;
         if (key_cnt2 >= KEY_TIME) {
            key_cnt2 = 0;
            if ((key_value & 0x02) == 0) {
               key_value |= 0x02;
               //beep_control(ON);
            }
         }
      } else {
         key_cnt2 = 0;
         key_value &= ~(0x02);
      }
      if (KEY2 == RESET) {
         key_cnt3++;
         if (key_cnt3 >= KEY_TIME) {
            key_cnt3 = 0;
            if ((key_value & 0x04) == 0) {
               key_value |= 0x04;
               //beep_control(ON);
            }
         }
      } else {
         key_cnt3 = 0;
         key_value &= ~(0x04);
      }
      if (KEY_UP == SET) {
         key_cnt4++;
         if (key_cnt4 >= KEY_TIME) {
            key_cnt4 = 0;
            if ((key_value & 0x08) == 0) {
               key_value |= 0x08;
               //beep_control(ON);
            }
         }
      } else {
         key_cnt4 = 0;
         key_value &= ~(0x08);
      }
      beep_check();
      return key_value;
}


void key_init (void)
{
      /* Enable GPIO clocks */
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOF, ENABLE);
      GPIO_Init_Pin(GPIOE,GPIO_Pin_2,GPIO_Speed_100MHz,GPIO_Mode_IPU);		// KEY 2
      GPIO_Init_Pin(GPIOE,GPIO_Pin_3,GPIO_Speed_100MHz,GPIO_Mode_IPU);		// KEY 1
      GPIO_Init_Pin(GPIOE,GPIO_Pin_4,GPIO_Speed_100MHz,GPIO_Mode_IPU);		// KEY 0
      GPIO_Init_Pin(GPIOA,GPIO_Pin_0,GPIO_Speed_100MHz,GPIO_Mode_IPD);		// WAKEUP_KEY
      GPIO_Init_Pin(GPIOF,GPIO_Pin_8,GPIO_Speed_100MHz,GPIO_Mode_Out_PP);	// BUZZER
      BEEP = 0;
      beep_on = 0;
      beep_cnt = 0;
      beep_max = 500;
}
