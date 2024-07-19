/********************************************************************************/
/* rtc.c                                                                        */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#include "../../hwdefs.h"
#include "../prototype.h"


volatile unsigned char rtc_alarm,rtc_wkup;
const unsigned char table_week [12] = {0,3,3,6,1,4,6,2,5,0,3,5};



ErrorStatus RTC_Set_Time (unsigned char hour,unsigned char min,unsigned char sec,unsigned char ampm)
{
      RTC_TimeTypeDef RTC_TimeTypeInitStructure;
      RTC_TimeTypeInitStructure.RTC_Hours = hour;
      RTC_TimeTypeInitStructure.RTC_Minutes = min;
      RTC_TimeTypeInitStructure.RTC_Seconds = sec;
      RTC_TimeTypeInitStructure.RTC_H12 = ampm;
      return RTC_SetTime(RTC_Format_BIN,&RTC_TimeTypeInitStructure);

}


ErrorStatus RTC_Set_Date (unsigned char year,unsigned char month,unsigned char date,unsigned char week)
{

      RTC_DateTypeDef RTC_DateTypeInitStructure;
      RTC_DateTypeInitStructure.RTC_Date = date;
      RTC_DateTypeInitStructure.RTC_Month = month;
      RTC_DateTypeInitStructure.RTC_WeekDay = week;
      RTC_DateTypeInitStructure.RTC_Year = year;
      return RTC_SetDate(RTC_Format_BIN,&RTC_DateTypeInitStructure);
}


void RTC_Get_Time (unsigned char *hour,unsigned char *min,unsigned char *sec,unsigned char *ampm)
{
      RTC_TimeTypeDef RTC_TimeTypeInitStructure;
      RTC_GetTime(RTC_Format_BIN,&RTC_TimeTypeInitStructure);
      *hour = RTC_TimeTypeInitStructure.RTC_Hours;
      *min = RTC_TimeTypeInitStructure.RTC_Minutes;
      *sec = RTC_TimeTypeInitStructure.RTC_Seconds;
      *ampm = RTC_TimeTypeInitStructure.RTC_H12;
}


void RTC_Get_Date (unsigned char *year,unsigned char *month,unsigned char *date,unsigned char *week)
{
      RTC_DateTypeDef RTC_DateStruct;
      RTC_GetDate(RTC_Format_BIN, &RTC_DateStruct);
      *year = RTC_DateStruct.RTC_Year;
      *month = RTC_DateStruct.RTC_Month;
      *date = RTC_DateStruct.RTC_Date;
      *week = RTC_DateStruct.RTC_WeekDay;
}


unsigned char rtc_init (void)
{
      RTC_InitTypeDef RTC_InitStructure;
      unsigned short retry = 0x1FFF;
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
      PWR_BackupAccessCmd(ENABLE);
      if (RTC_ReadBackupRegister(RTC_BKP_DR0) != 0x5050) {
	 RCC_LSEConfig(RCC_LSE_ON);
	 while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) {
	       retry++;
	       wait_ms(10);
	 }
	 if (retry == 0) return 1;
	 RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
	 RCC_RTCCLKCmd(ENABLE);
         RTC_InitStructure.RTC_AsynchPrediv = 0x7F;
         RTC_InitStructure.RTC_SynchPrediv = 0xFF;
         RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
         RTC_Init(&RTC_InitStructure);
	 RTC_Set_Time(23,59,56,RTC_H12_AM);
	 RTC_Set_Date(23,11,13,2);
	 RTC_WriteBackupRegister(RTC_BKP_DR0,0x5050);
     }
     return 0;
}


void RTC_Set_AlarmA (unsigned char week,unsigned char hour,unsigned char min,unsigned char sec)
{
      EXTI_InitTypeDef EXTI_InitStructure;
      RTC_AlarmTypeDef RTC_AlarmTypeInitStructure;
      RTC_TimeTypeDef RTC_TimeTypeInitStructure;
      NVIC_InitTypeDef NVIC_InitStructure;
      RTC_AlarmCmd(RTC_Alarm_A,DISABLE);
      RTC_TimeTypeInitStructure.RTC_Hours = hour;
      RTC_TimeTypeInitStructure.RTC_Minutes = min;
      RTC_TimeTypeInitStructure.RTC_Seconds = sec;
      RTC_TimeTypeInitStructure.RTC_H12 = RTC_H12_AM;
      RTC_AlarmTypeInitStructure.RTC_AlarmDateWeekDay = week;
      RTC_AlarmTypeInitStructure.RTC_AlarmDateWeekDaySel = RTC_AlarmDateWeekDaySel_WeekDay;
      RTC_AlarmTypeInitStructure.RTC_AlarmMask = RTC_AlarmMask_None;
      RTC_AlarmTypeInitStructure.RTC_AlarmTime = RTC_TimeTypeInitStructure;
      RTC_SetAlarm(RTC_Format_BIN,RTC_Alarm_A,&RTC_AlarmTypeInitStructure);
      RTC_ClearITPendingBit(RTC_IT_ALRA);
      EXTI_ClearITPendingBit(EXTI_Line17);
      RTC_ITConfig(RTC_IT_ALRA,ENABLE);
      RTC_AlarmCmd(RTC_Alarm_A,ENABLE);
      EXTI_InitStructure.EXTI_Line = EXTI_Line17;
      EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
      EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
      EXTI_InitStructure.EXTI_LineCmd = ENABLE;
      EXTI_Init(&EXTI_InitStructure);
      NVIC_InitStructure.NVIC_IRQChannel = RTC_Alarm_IRQn;
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x03;
      NVIC_InitStructure.NVIC_IRQChannelSubPriority =  0;
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
      NVIC_Init(&NVIC_InitStructure);
}


void RTC_Set_WakeUp (unsigned int wksel,unsigned short cnt)
{
      EXTI_InitTypeDef EXTI_InitStructure;
      NVIC_InitTypeDef NVIC_InitStructure;
      RTC_WakeUpCmd(DISABLE);
      RTC_WakeUpClockConfig(wksel);
      RTC_SetWakeUpCounter(cnt);
      RTC_ClearITPendingBit(RTC_IT_WUT);
      EXTI_ClearITPendingBit(EXTI_Line22);
      RTC_ITConfig(RTC_IT_WUT,ENABLE);
      RTC_WakeUpCmd(ENABLE);
      EXTI_InitStructure.EXTI_Line = EXTI_Line22;
      EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
      EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
      EXTI_InitStructure.EXTI_LineCmd = ENABLE;
      EXTI_Init(&EXTI_InitStructure);
      NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn;
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
      NVIC_Init(&NVIC_InitStructure);
}


void RTC_Alarm_IRQHandler (void)
{
      if (RTC_GetFlagStatus(RTC_FLAG_ALRAF) == SET) {
	 RTC_ClearFlag(RTC_FLAG_ALRAF);
	 rtc_alarm = 1;
      }
      EXTI_ClearITPendingBit(EXTI_Line17);
}


void RTC_WKUP_IRQHandler (void)
{
      if (RTC_GetFlagStatus(RTC_FLAG_WUTF) == SET) {
	 RTC_ClearFlag(RTC_FLAG_WUTF);
         rtc_wkup = 1;
      }
      EXTI_ClearITPendingBit(EXTI_Line22);
}


unsigned char RTC_Get_Week (unsigned short year,unsigned char month,unsigned char day)
{
      unsigned short temp2;
      unsigned char yearH,yearL;
      yearH = year / 100;
      yearL = year % 100;
      if (yearH > 19) yearL += 100;
      temp2 = yearL + yearL / 4;
      temp2 = temp2 % 7;
      temp2 = temp2 + day + table_week[month - 1];
      if (yearL % 4 == 0 && month < 3) temp2--;
      temp2 %= 7;
      if (temp2 == 0) temp2 = 7;
      return temp2;
}
