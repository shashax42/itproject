/********************************************************************************/
/* i2c_app.c                                                                    */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#include "../../hwdefs.h"
#include "../prototype.h"

#define ClockSpeed			200000

#define OwnAddress1			0x58
#define OwnAddress2			0x60

#define SEEP_PAGE0			0xA0
#define SEEP_PAGE1			0xA2


unsigned char I2C1_PORT = 1,I2C_CHANNEL = 0,i2c_error;
unsigned short i2c_temp;
unsigned int time_out;


void i2c_write (I2C_TypeDef *I2Cx, unsigned char device, unsigned char address, unsigned char buffer)
{
      i2c_error	= 0;
      time_out = 0;
      while (I2C_GetFlagStatus(I2Cx,I2C_FLAG_BUSY)) {
      	    time_out++;
      	    if (time_out >= 1000) break;
      }
restart_w1:
      I2C_GenerateSTART(I2Cx,ENABLE);
      time_out = 0;
      while (!I2C_CheckEvent(I2Cx,I2C_EVENT_MASTER_MODE_SELECT)) {
      	    time_out++;
      	    if (time_out >= 3000) {
      	       i2c_error = 1;
      	       init_i2c_24xx();
      	       return;
      	    }
      }
      device = device & 0xFFFE;
      I2C_Send7bitAddress(I2Cx,device, I2C_Direction_Transmitter);
      time_out = 0;
      while ((I2Cx->SR1 & 0x0002) != 0x0002) {
      	    time_out++;
      	    if (time_out >= 3000) {
      	       I2C_ClearFlag(I2Cx,I2C_FLAG_BUSY | I2C_FLAG_AF);
               goto restart_w1;
      	    }
      }
      device = I2Cx->SR2;
      I2C_SendData(I2Cx,address);
      time_out = 0;
      while (!I2C_CheckEvent(I2Cx,I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
      	    time_out++;
      	    if (time_out >= 3000) {
      	       i2c_error = 2;
      	       init_i2c_24xx();
      	       return;
      	    }
      }
      I2C_SendData(I2Cx,buffer);
      time_out = 0;
      while (!I2C_CheckEvent(I2Cx,I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
      	    time_out++;
      	    if (time_out >= 3000) {
      	       i2c_error = 3;
      	       init_i2c_24xx();
      	       return;
      	    }
      }
      I2C_GenerateSTOP(I2Cx,ENABLE);
      while (I2Cx->CR1 & 0x0200) {
      	    time_out++;
      	    if (time_out >= 1000) break;
      }
}


unsigned char i2c_read (I2C_TypeDef *I2Cx, unsigned char device, unsigned char address)
{
      unsigned char data;
      i2c_error	= 0;
      time_out = 0;
      while (I2C_GetFlagStatus(I2Cx,I2C_FLAG_BUSY)) {
      	    time_out++;
      	    if (time_out >= 1000) break;
      }
restart_r1:
      I2C_GenerateSTART(I2Cx,ENABLE);
      time_out = 0;
      while (!I2C_CheckEvent(I2Cx,I2C_EVENT_MASTER_MODE_SELECT)) {
      	    time_out++;
      	    if (time_out >= 3000) {
      	       i2c_error = 1;
      	       init_i2c_24xx();
      	       return 0;
      	    }
      }
      device = device & 0xFFFE;
      I2C_Send7bitAddress(I2Cx,device, I2C_Direction_Transmitter);
      time_out = 0;
      while ((I2Cx->SR1 & 0x0002) != 0x0002) {
      	    time_out++;
      	    if (time_out >= 3000) {
      	       I2C_ClearFlag(I2Cx,I2C_FLAG_BUSY | I2C_FLAG_AF);
               goto restart_r1;
      	    }
      }
      i2c_temp = I2Cx->SR2;
      I2C_SendData(I2Cx,address);
      time_out = 0;
      while (!I2C_CheckEvent(I2Cx,I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
      	    time_out++;
      	    if (time_out >= 3000) {
      	       i2c_error = 2;
      	       init_i2c_24xx();
      	       return 0;
      	    }
      }
      I2C_AcknowledgeConfig(I2Cx,DISABLE);
restart_r2:
      I2C_GenerateSTART(I2Cx,ENABLE);
      time_out = 0;
      while (!I2C_CheckEvent(I2Cx,I2C_EVENT_MASTER_MODE_SELECT)) {
      	    time_out++;
      	    if (time_out >= 3000) {
      	       i2c_error = 3;
      	       init_i2c_24xx();
      	       return 0;
      	    }
      }
      I2C_Send7bitAddress(I2Cx,device,I2C_Direction_Receiver);
      time_out = 0;
      while (!I2C_GetFlagStatus(I2Cx,I2C_FLAG_ADDR)) {
      	    time_out++;
      	    if (time_out >= 3000) {
      	       I2C_ClearFlag(I2Cx,I2C_FLAG_BUSY | I2C_FLAG_AF);
               goto restart_r2;
      	    }
      }
      I2C_AcknowledgeConfig(I2Cx,DISABLE);
      __disable_irq();
      device = I2Cx->SR2;
      I2C_GenerateSTOP(I2Cx,ENABLE);
      __enable_irq();
      time_out = 0;
      while ((I2C_GetLastEvent(I2Cx) & 0x0040) != 0x0040) {
      	    time_out++;
      	    if (time_out >= 5000) {
      	       i2c_error = 4;
      	       init_i2c_24xx();
      	       return 0;
      	    }
      }
      data = I2C_ReceiveData(I2Cx);
      time_out = 0;
      while ((I2Cx->CR1 & 0x200) == 0x200) {
      	    time_out++;
      	    if (time_out >= 3000) break;
      }
      I2C_AcknowledgeConfig(I2Cx,ENABLE);
      return data;
}


void I2C_LowLevel_Init (void)
{
      I2C_InitTypeDef I2C_InitStructure;
      if (I2C_CHANNEL == 0) {
         /* Enable I2C1 clock */
         RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
         if (I2C1_PORT == 0) {
            GPIO_PinAFConfig(GPIOB,GPIO_PinSource6,GPIO_AF_I2C1);
            GPIO_PinAFConfig(GPIOB,GPIO_PinSource7,GPIO_AF_I2C1);
            /* I2C1 SDA and SCL configuration */
            GPIO_Init_Pin(GPIOB,GPIO_Pin_6,GPIO_Speed_100MHz,GPIO_Mode_AF_OD);
            GPIO_Init_Pin(GPIOB,GPIO_Pin_7,GPIO_Speed_100MHz,GPIO_Mode_AF_OD);
         } else {
            GPIO_PinAFConfig(GPIOB,GPIO_PinSource8,GPIO_AF_I2C1);
            GPIO_PinAFConfig(GPIOB,GPIO_PinSource9,GPIO_AF_I2C1);
            /* I2C1 SDA and SCL configuration */
            GPIO_Init_Pin(GPIOB,GPIO_Pin_8,GPIO_Speed_100MHz,GPIO_Mode_AF_OD);
            GPIO_Init_Pin(GPIOB,GPIO_Pin_9,GPIO_Speed_100MHz,GPIO_Mode_AF_OD);
         }
         I2C_DeInit(I2C1);
         /* I2C1 configuration */
         I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
         I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
         I2C_InitStructure.I2C_OwnAddress1 = OwnAddress1;
         I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
         I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
         I2C_InitStructure.I2C_ClockSpeed = ClockSpeed;
         I2C_Init(I2C1, &I2C_InitStructure);
      } else {
         /* Enable I2C2 clock */
         RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
         /* I2C2 SDA and SCL configuration */
         GPIO_PinAFConfig(GPIOB,GPIO_PinSource10,GPIO_AF_I2C2);
         GPIO_PinAFConfig(GPIOB,GPIO_PinSource11,GPIO_AF_I2C2);
         GPIO_Init_Pin(GPIOB,GPIO_Pin_10,GPIO_Speed_100MHz,GPIO_Mode_AF_OD);
         GPIO_Init_Pin(GPIOB,GPIO_Pin_11,GPIO_Speed_100MHz,GPIO_Mode_AF_OD);
         I2C_DeInit(I2C2);
         /* I2C2 configuration */
         I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
         I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
         I2C_InitStructure.I2C_OwnAddress1 = OwnAddress2;
         I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
         I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
         I2C_InitStructure.I2C_ClockSpeed = ClockSpeed;
         I2C_Init(I2C2, &I2C_InitStructure);
      }
}


void u8_write (unsigned char device,unsigned char address,unsigned char data)
{
      /* Reload IWDG counter */
      IWDG_ReloadCounter();
      if (I2C_CHANNEL == 0) {
         i2c_write(I2C1,device,address,data);
      } else {
         i2c_write(I2C2,device,address,data);
      }
      wait_ms(4);
}


unsigned char u8_read (unsigned char device,unsigned char address)
{
      unsigned char ret_value;
      /* Reload IWDG counter */
      IWDG_ReloadCounter();
      if (I2C_CHANNEL == 0) {
         ret_value = i2c_read(I2C1,device,address);
      } else {
         ret_value = i2c_read(I2C2,device,address);
      }
      return ret_value;
}


unsigned char eeprom_read (unsigned char address)
{
      return u8_read(SEEP_PAGE0,address);
}


void eeprom_write (unsigned char address,unsigned char data)
{
      u8_write(SEEP_PAGE0,address,data);
}


void init_i2c_24xx (void)
{
      I2C_LowLevel_Init();
      if (I2C_CHANNEL == 0) {
         I2C_Cmd(I2C1, ENABLE);
         I2C_CHANNEL = 0;
      } else {
         I2C_Cmd(I2C2, ENABLE);
         I2C_CHANNEL = 1;
      }
}
