/********************************************************************************/
/* serial.c                                                                     */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "hwdefs.h"

#define SOH				0x01
#define STX				0x02
#define ETX				0x03
#define EOT				0x04
#define ENQ				0x05
#define ACK				0x06
#define NAK				0x15

volatile unsigned short rxcnt1,txcnt1,maxtx1,rxcnt2,txcnt2,maxtx2,rxcnt3,txcnt3,maxtx3;
volatile unsigned char rxck1,rxck2,rxck3,rx_led,tx_led,this_id;
volatile unsigned int flash_address;
volatile unsigned int flash_para[64];


char rxbuff1[256],txbuff1[256],rxbuff2[256],txbuff2[256],rxbuff3[256],txbuff3[256];
unsigned char USART1_PORT,USART3_PORT;

extern void wm8978_volset (unsigned char vol);



void flash_read (void)
{
      unsigned int idx;
      FLASH_Unlock();
      flash_address = (unsigned int)0x08004000;
      for (idx=0;idx<64;idx++) {
          flash_para[idx] = (*(volatile unsigned int *)(flash_address + (idx * 4)));
      }
      FLASH_Lock();
}


void USART1_IRQHandler (void)
{
      if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
         /* Read one byte from the receive data register */
         rxbuff1[rxcnt1] = USART_ReceiveData(USART1);
         if (rxbuff1[0] == STX) {
            rxcnt1++;
            rx_led = 1;
         } else {
            rxcnt1 = 0;
         }
         rxck1 = 0;
         USART_ClearITPendingBit(USART1, USART_IT_RXNE);
      }
      if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET) {
         /* Write one byte to the transmit data register */
      	 if (txcnt1 < maxtx1) {
            USART_SendData(USART1, txbuff1[txcnt1]);
      	    txcnt1++;
      	 } else {
      	    /* Disable the USART1 Transmit interrupt */
      	    USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
      	    USART_ITConfig(USART1, USART_IT_TC, ENABLE);
      	 }
      	 USART_ClearITPendingBit(USART1, USART_IT_TXE);
      }
      if (USART_GetITStatus(USART1, USART_IT_TC) != RESET) {
      	 USART_ITConfig(USART1, USART_IT_TC, DISABLE);
      	 USART_ClearITPendingBit(USART1, USART_IT_TC);
         tx_led = 0;
      }
}


void USART2_IRQHandler (void)
{
      if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
         /* Read one byte from the receive data register */
         rxbuff2[rxcnt2] = USART_ReceiveData(USART2);
         if (rxbuff2[0] == STX) {
            rxcnt2++;
            rx_led = 1;
         } else {
            rxcnt2 = 0;
         }
         rxck2 = 0;
         USART_ClearITPendingBit(USART2, USART_IT_RXNE);
      }
      if (USART_GetITStatus(USART2, USART_IT_TXE) != RESET) {
         /* Write one byte to the transmit data register */
      	 if (txcnt2 < maxtx2) {
            USART_SendData(USART2, txbuff2[txcnt2]);
      	    txcnt2++;
      	 } else {
      	    /* Disable the USART2 Transmit interrupt */
      	    USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
      	    USART_ITConfig(USART2, USART_IT_TC, ENABLE);
      	 }
      	 USART_ClearITPendingBit(USART2, USART_IT_TXE);
      }
      if (USART_GetITStatus(USART2, USART_IT_TC) != RESET) {
      	 USART_ITConfig(USART2, USART_IT_TC, DISABLE);
      	 USART_ClearITPendingBit(USART2, USART_IT_TC);
         tx_led = 0;
         TXEN_485 = 0;
      }
}


void USART3_IRQHandler (void)
{
      if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
         /* Read one byte from the receive data register */
         rxbuff3[rxcnt3] = USART_ReceiveData(USART3);
         if (rxbuff3[0] == STX) {
            rxcnt3++;
            rx_led = 1;
         } else {
            rxcnt3 = 0;
         }
         rxck3 = 0;
         USART_ClearITPendingBit(USART3, USART_IT_RXNE);
      }
      if (USART_GetITStatus(USART3, USART_IT_TXE) != RESET) {
         /* Write one byte to the transmit data register */
      	 if (txcnt3 < maxtx3) {
            USART_SendData(USART3, txbuff3[txcnt3]);
      	    txcnt3++;
      	 } else {
      	    /* Disable the USART3 Transmit interrupt */
      	    USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
      	    USART_ITConfig(USART3, USART_IT_TC, ENABLE);
      	 }
      	 USART_ClearITPendingBit(USART3, USART_IT_TXE);
      }
      if (USART_GetITStatus(USART3, USART_IT_TC) != RESET) {
      	 USART_ITConfig(USART3, USART_IT_TC, DISABLE);
      	 USART_ClearITPendingBit(USART3, USART_IT_TC);
         tx_led = 0;
      }
}


void tx_enable1 (unsigned char max)
{
      maxtx1 = max;
      txcnt1 = 0;
      USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
      tx_led = 1;
}


void tx_enable2 (unsigned char max)
{
      TXEN_485 = 1;
      maxtx2 = max;
      txcnt2 = 0;
      USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
      tx_led = 1;
}


void tx_enable3 (unsigned char max)
{
      maxtx3 = max;
      txcnt3 = 0;
      USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
      tx_led = 1;
}


void receive_check1 (void)
{
      unsigned char bcc,idx;
      if (rxcnt1) {
         if ((rxbuff1[0] == STX) && (rxbuff1[5] == ETX) && ((rxbuff1[1] == this_id) || (rxbuff1[1] == 33))) {
	    bcc = rxbuff1[0];
	    for (idx=1;idx<6;idx++) bcc = bcc ^ rxbuff1[idx];
            if (bcc == rxbuff1[6]) {
	       if ((rxbuff1[2] == 0xFF) && (rxbuff1[3] == 0xFF) && (rxbuff1[4] == 0xFF)) {
	       	  wm8978_volset(0);
	       	  NVIC_SystemReset();
	       }
	    }
         }
         bzero(rxbuff1,rxcnt1);
         rxcnt1 = 0;
      }
      rx_led = 0;
}


void receive_check2 (void)
{
      unsigned char bcc,idx;
      if (rxcnt2) {
         if ((rxbuff2[0] == STX) && (rxbuff2[5] == ETX) && ((rxbuff2[1] == this_id) || (rxbuff2[1] == 33))) {
	    bcc = rxbuff2[0];
	    for (idx=1;idx<6;idx++) bcc = bcc ^ rxbuff2[idx];
            if (bcc == rxbuff2[6]) {
	       if ((rxbuff2[2] == 0xFF) && (rxbuff2[3] == 0xFF) && (rxbuff2[4] == 0xFF)) {
	       	  wm8978_volset(0);
	       	  NVIC_SystemReset();
	       }
	    }
         }
         bzero(rxbuff2,rxcnt2);
         rxcnt2 = 0;
      }
      rx_led = 0;
}


void receive_check3 (void)
{
      unsigned char bcc,idx;
      if (rxcnt3) {
         if ((rxbuff3[0] == STX) && (rxbuff3[5] == ETX) && ((rxbuff3[1] == this_id) || (rxbuff3[1] == 33))) {
	    bcc = rxbuff3[0];
	    for (idx=1;idx<6;idx++) bcc = bcc ^ rxbuff3[idx];
            if (bcc == rxbuff3[6]) {
	       if ((rxbuff3[2] == 0xFF) && (rxbuff3[3] == 0xFF) && (rxbuff3[4] == 0xFF)) {
	       	  wm8978_volset(0);
	       	  NVIC_SystemReset();
	       }
	    }
         }
         bzero(rxbuff3,rxcnt3);
         rxcnt3 = 0;
      }
      rx_led = 0;
}


void serial_check (void)
{
      if ((rxcnt1 != 0) && (rxck1 >= 3)) {
         rxck1 = 0;
         receive_check1();
      }
      if ((rxcnt2 != 0) && (rxck2 >= 3)) {
         rxck2 = 0;
         receive_check2();
      }
      if ((rxcnt3 != 0) && (rxck3 >= 3)) {
         rxck3 = 0;
         receive_check3();
      }
}


void serial_init (void)
{
      USART_InitTypeDef	USART_InitStructure;
      NVIC_InitTypeDef NVIC_InitStructure;
      USART1_PORT = 0;
      USART3_PORT = 0;
      rxcnt1 = 0;
      txcnt1 = 0;
      maxtx1 = 0;
      rxcnt2 = 0;
      txcnt2 = 0;
      maxtx2 = 0;
      rxcnt3 = 0;
      txcnt3 = 0;
      maxtx3 = 0;
      /* Enable the USART1 Interrupt */
      NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
      NVIC_Init(&NVIC_InitStructure);
      switch (USART1_PORT) {
         case 0:
           GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1);
           GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1);
           GPIO_Init_Pin(GPIOA,TXD1,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
           GPIO_Init_Pin(GPIOA,RXD1,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
           break;
         case 1:
           GPIO_PinAFConfig(GPIOB,GPIO_PinSource6,GPIO_AF_USART1);
           GPIO_PinAFConfig(GPIOB,GPIO_PinSource7,GPIO_AF_USART1);
           GPIO_Init_Pin(GPIOB,GPIO_Pin_6,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
           GPIO_Init_Pin(GPIOB,GPIO_Pin_7,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
           break;
      }
      /* Enable the USART2 Interrupt */
      NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
      NVIC_Init(&NVIC_InitStructure);
      GPIO_PinAFConfig(GPIOA,GPIO_PinSource2,GPIO_AF_USART2);
      GPIO_PinAFConfig(GPIOA,GPIO_PinSource3,GPIO_AF_USART2);
      GPIO_Init_Pin(GPIOA,TXD2,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOA,RXD2,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      /* Enable the USART3 Interrupt */
      NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
      NVIC_Init(&NVIC_InitStructure);
      switch (USART3_PORT) {
      	 case 0:
           GPIO_PinAFConfig(GPIOB,GPIO_PinSource10,GPIO_AF_USART3);
           GPIO_PinAFConfig(GPIOB,GPIO_PinSource11,GPIO_AF_USART3);
           /* Configure USART3 TX (PB10) */
           GPIO_Init_Pin(GPIOB,TXD3,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
           /* Configure USART3 TX (PB10) */
           GPIO_Init_Pin(GPIOB,RXD3,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
           break;
         case 1:
           GPIO_PinAFConfig(GPIOC,GPIO_PinSource10,GPIO_AF_USART3);
           GPIO_PinAFConfig(GPIOC,GPIO_PinSource11,GPIO_AF_USART3);
           /* Configure USART3 TX (PC10) */
           GPIO_Init_Pin(GPIOC,GPIO_Pin_10,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
           /* Configure USART3 RX (PC11) */
           GPIO_Init_Pin(GPIOC,GPIO_Pin_11,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
           break;
         case 2:
           GPIO_PinAFConfig(GPIOD,GPIO_PinSource8,GPIO_AF_USART3);
           GPIO_PinAFConfig(GPIOD,GPIO_PinSource9,GPIO_AF_USART3);
           /* Configure USART3 TX (PD8) */
           GPIO_Init_Pin(GPIOD,GPIO_Pin_8,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
           /* Configure USART3 RX (PD9) */
           GPIO_Init_Pin(GPIOD,GPIO_Pin_9,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
           break;
      }
      USART_InitStructure.USART_BaudRate = 2000000;
      USART_InitStructure.USART_WordLength = USART_WordLength_8b;
      USART_InitStructure.USART_StopBits = USART_StopBits_1;
      USART_InitStructure.USART_Parity = USART_Parity_No;
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
      USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
      USART_DeInit(USART1);
      /* Enable USART1 clock */
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
      /* Configure USART1 */
      USART_Init(USART1, &USART_InitStructure);
      /* Enable USART1 Receive and Transmit interrupts */
      USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
      /* Enable the USART1 */
      USART_Cmd(USART1, ENABLE);
      USART_DeInit(USART2);
      /* Enable USART2 clock */
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
      /* Configure USART2 */
      USART_Init(USART2, &USART_InitStructure);
      /* Enable USART2 Receive and Transmit interrupts */
      USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
      /* Enable the USART2 */
      USART_Cmd(USART2, ENABLE);
      USART_DeInit(USART3);
      /* Enable USART3 clock */
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
      /* Configure USART3 */
      USART_Init(USART3, &USART_InitStructure);
      /* Enable USART3 Receive and Transmit interrupts */
      USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
      /* Enable the USART3 */
      USART_Cmd(USART3, ENABLE);
      this_id = 1;
}