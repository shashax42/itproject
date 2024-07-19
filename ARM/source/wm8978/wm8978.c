/********************************************************************************/
/* wm8978.c                                                                     */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#include "../../hwdefs.h"
#include "../prototype.h"


#define WM8978_ADDR			0x1A
//FREQ[0]
#define EQ1_80Hz			0x00
#define EQ1_105Hz			0x01
#define EQ1_135Hz			0x02
#define EQ1_175Hz			0x03
//FREQ[1]
#define EQ2_230Hz			0x00
#define EQ2_300Hz			0x01
#define EQ2_385Hz			0x02
#define EQ2_500Hz			0x03
//FREQ[2]
#define EQ3_650Hz			0x00
#define EQ3_850Hz			0x01
#define EQ3_1100Hz			0x02
#define EQ3_14000Hz			0x03
//FREQ[3]
#define EQ4_1800Hz			0x00
#define EQ4_2400Hz			0x01
#define EQ4_3200Hz			0x02
#define EQ4_4100Hz			0x03
//FREQ[4]
#define EQ5_5300Hz			0x00
#define EQ5_6900Hz			0x01
#define EQ5_9000Hz			0x02
#define EQ5_11700Hz			0x03

#define SYSTEM_PARA_SAVE_BASE		20

extern unsigned char I2C_CHANNEL;

_wm8978_obj wm8978set = {
      .mvol = 50,
      .cfreq = {0,0,0,0,0},
      .freqval = {0,0,0,0,0},
      .d3 = 0,
      .speakersw = 1,
      .saveflag = 0,
};


static unsigned short WM8978_REGVAL_TBL [58] =
{
      0x0000,0x0000,0x0000,0x0000,0x0050,0x0000,0x0140,0x0000,
      0x0000,0x0000,0x0000,0x00FF,0x00FF,0x0000,0x0100,0x00FF,
      0x00FF,0x0000,0x012C,0x002C,0x002C,0x002C,0x002C,0x0000,
      0x0032,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
      0x0038,0x000B,0x0032,0x0000,0x0008,0x000C,0x0093,0x00E9,
      0x0000,0x0000,0x0000,0x0000,0x0003,0x0010,0x0010,0x0100,
      0x0100,0x0002,0x0001,0x0001,0x0039,0x0039,0x0039,0x0039,
      0x0001,0x0001
};


void wm8978_read_para (_wm8978_obj *wm8978dev)
{
      wm8978dev->mvol = eeprom_read(SYSTEM_PARA_SAVE_BASE + 0);
      wm8978dev->cfreq[0] = eeprom_read(SYSTEM_PARA_SAVE_BASE + 1);
      wm8978dev->cfreq[1] = eeprom_read(SYSTEM_PARA_SAVE_BASE + 2);
      wm8978dev->cfreq[2] = eeprom_read(SYSTEM_PARA_SAVE_BASE + 3);
      wm8978dev->cfreq[3] = eeprom_read(SYSTEM_PARA_SAVE_BASE + 4);
      wm8978dev->cfreq[4] = eeprom_read(SYSTEM_PARA_SAVE_BASE + 5);
      wm8978dev->freqval[0] = eeprom_read(SYSTEM_PARA_SAVE_BASE + 6);
      wm8978dev->freqval[1] = eeprom_read(SYSTEM_PARA_SAVE_BASE + 7);
      wm8978dev->freqval[2] = eeprom_read(SYSTEM_PARA_SAVE_BASE + 8);
      wm8978dev->freqval[3] = eeprom_read(SYSTEM_PARA_SAVE_BASE + 9);
      wm8978dev->freqval[4] = eeprom_read(SYSTEM_PARA_SAVE_BASE + 10);
      wm8978dev->d3 = eeprom_read(SYSTEM_PARA_SAVE_BASE + 11);
      wm8978dev->speakersw = eeprom_read(SYSTEM_PARA_SAVE_BASE + 12);
      wm8978dev->saveflag = eeprom_read(SYSTEM_PARA_SAVE_BASE + 13);     
}


void wm8978_save_para (_wm8978_obj *wm8978dev)
{
      eeprom_write(SYSTEM_PARA_SAVE_BASE + 0,wm8978dev->mvol);
      eeprom_write(SYSTEM_PARA_SAVE_BASE + 1,wm8978dev->cfreq[0]);
      eeprom_write(SYSTEM_PARA_SAVE_BASE + 2,wm8978dev->cfreq[1]);
      eeprom_write(SYSTEM_PARA_SAVE_BASE + 3,wm8978dev->cfreq[2]);
      eeprom_write(SYSTEM_PARA_SAVE_BASE + 4,wm8978dev->cfreq[3]);
      eeprom_write(SYSTEM_PARA_SAVE_BASE + 5,wm8978dev->cfreq[4]);
      eeprom_write(SYSTEM_PARA_SAVE_BASE + 6,wm8978dev->freqval[0]);
      eeprom_write(SYSTEM_PARA_SAVE_BASE + 7,wm8978dev->freqval[1]);
      eeprom_write(SYSTEM_PARA_SAVE_BASE + 8,wm8978dev->freqval[2]);
      eeprom_write(SYSTEM_PARA_SAVE_BASE + 9,wm8978dev->freqval[3]);
      eeprom_write(SYSTEM_PARA_SAVE_BASE + 10,wm8978dev->freqval[4]);
      eeprom_write(SYSTEM_PARA_SAVE_BASE + 11,wm8978dev->d3);
      eeprom_write(SYSTEM_PARA_SAVE_BASE + 12,wm8978dev->speakersw);
      eeprom_write(SYSTEM_PARA_SAVE_BASE + 13,wm8978dev->saveflag);      
}


unsigned char WM8978_Write_Reg (unsigned char reg,unsigned short val)
{
      unsigned char address,data;
      address = (reg << 1) | ((val >> 8) & 0x01);
      data = (val & 0xFF);
      if (I2C_CHANNEL == 0) {
         i2c_write(I2C1,(WM8978_ADDR << 1),address,data);
      } else {
         i2c_write(I2C2,(WM8978_ADDR << 1),address,data);
      }
      WM8978_REGVAL_TBL[reg] = val;
      return 0;
}


unsigned short WM8978_Read_Reg (unsigned char reg)
{
      return WM8978_REGVAL_TBL[reg];
}


void WM8978_MIC_Gain (unsigned char gain)
{
      gain &= 0x3F;
      WM8978_Write_Reg(45,gain);		//gain:0~63,-12dB~35.25dB,0.75dB/Step
      WM8978_Write_Reg(46,gain | 1 << 8);
}


void WM8978_LINEIN_Gain (unsigned char gain)
{
      unsigned short regval = 0;
      gain &= 0x07;
      regval=WM8978_Read_Reg(47);
      regval &= ~(7 << 4);
      WM8978_Write_Reg(47,regval | gain << 4);	//gain:0~7,0,1~7,-12dB~6dB,3dB/Step
      regval = WM8978_Read_Reg(48);
      regval &= ~(7 << 4);
      WM8978_Write_Reg(48,regval | gain << 4);
}


void WM8978_AUX_Gain (unsigned char gain)
{
      unsigned short regval = 0;
      gain &= 0x07;
      regval = WM8978_Read_Reg(47);
      regval &= ~(7 << 0);
      WM8978_Write_Reg(47,regval | gain << 0);	//gain:0~7,0,1~7,-12dB~6dB,3dB/Step
      regval = WM8978_Read_Reg(48);
      regval &= ~(7 << 0);
      WM8978_Write_Reg(48,regval | gain << 0);
}


void WM8978_ADDA_Cfg (unsigned char dacen,unsigned char adcen)
{
      unsigned short regval = 0;
      regval = WM8978_Read_Reg(3);
      if (dacen) regval |= 3 << 0;else regval &= ~(3 << 0);
      WM8978_Write_Reg(3,regval);
      regval = WM8978_Read_Reg(2);
      if (adcen) regval |= 3 << 0;else regval &= ~(3 << 0);
      WM8978_Write_Reg(2,regval);
}


void WM8978_Input_Cfg (unsigned char micen,unsigned char lineinen,unsigned char auxen)
{
      unsigned short regval = 0;
      regval = WM8978_Read_Reg(2);
      if (micen) regval |= 3 << 2;else regval &= ~(3 << 2);
      WM8978_Write_Reg(2,regval);
      regval = WM8978_Read_Reg(44);
      if (micen) regval |= 3 << 4 | 3 << 0;else regval &= ~(3 << 4 | 3 << 0);
      WM8978_Write_Reg(44,regval);
      if (lineinen) WM8978_LINEIN_Gain(5);else WM8978_LINEIN_Gain(0);
      if (auxen) WM8978_AUX_Gain(7);else WM8978_AUX_Gain(0);
}


void WM8978_Output_Cfg (unsigned char dacen,unsigned char bpsen)
{
      unsigned short regval = 0;
      if (dacen) regval |= 1 << 0;
      if (bpsen) {
	 regval |= 1 << 1;			//bpsen:Bypass,MIC,LINE IN,AUX
	 regval |= 5 << 2;
      }
      WM8978_Write_Reg(50,regval);
      WM8978_Write_Reg(51,regval);
}


void WM8978_I2S_Cfg (unsigned char fmt,unsigned char len)
{
      fmt &= 0x03;			//fmt:0,LSB;1,MSB;2,I2S;3,PCM/DSP;
      len &= 0x03;			//len:0,16;1,20;2,24;3,32;
      WM8978_Write_Reg(4,(fmt << 3) | (len << 5));
}


void WM8978_HPvol_Set (unsigned char voll,unsigned char volr)
{
      voll &= 0x3F;
      volr &= 0x3F;
      if (voll == 0) voll |= 1 << 6;
      if (volr == 0) volr |= 1 << 6;
      WM8978_Write_Reg(52,voll);
      WM8978_Write_Reg(53,volr | (1 << 8));
}


void WM8978_SPKvol_Set (unsigned char volx)
{
      volx &= 0x3F;
      if (volx == 0) volx |= 1 << 6;
      WM8978_Write_Reg(54,volx);
      WM8978_Write_Reg(55,volx | (1 << 8));
}


void WM8978_3D_Set (unsigned char depth)
{
      depth &= 0x0F;
      WM8978_Write_Reg(41,depth);
}


void WM8978_EQ_3D_Dir (unsigned char dir)
{
      unsigned short regval = 0;
      regval = WM8978_Read_Reg(0x12);
      if (dir) regval |= 1 << 8;else regval &= ~(1 << 8);
      WM8978_Write_Reg(18,regval);
}


void WM8978_EQ1_Set (unsigned char cfreq,unsigned char gain)
{
      unsigned short regval = 0;
      cfreq &= 0x03;
      if (gain > 24) gain = 24;
      gain = 24 - gain;
      regval = WM8978_Read_Reg(18);
      regval&=0x100;
      regval |= cfreq << 5;		//cfreq:0~3:80/105/135/175Hz
      regval |= gain;
      WM8978_Write_Reg(18,regval);
}


void WM8978_EQ2_Set (unsigned char cfreq,unsigned char gain)
{
      unsigned short regval = 0;
      cfreq &= 0x03;
      if (gain > 24) gain = 24;
      gain = 24 - gain;
      regval |= cfreq << 5;		//cfreq:0~3:230/300/385/500Hz
      regval |= gain;
      WM8978_Write_Reg(19,regval);
}


void WM8978_EQ3_Set (unsigned char cfreq,unsigned char gain)
{
      unsigned short regval = 0;
      cfreq &= 0x03;
      if (gain > 24) gain = 24;
      gain = 24 - gain;
      regval |= cfreq << 5;		//cfreq:0~3:650/850/1100/1400Hz
      regval |= gain;
      WM8978_Write_Reg(20,regval);
}


void WM8978_EQ4_Set (unsigned char cfreq,unsigned char gain)
{
      unsigned short regval = 0;
      cfreq &= 0x03;
      if (gain > 24) gain = 24;
      gain = 24 - gain;
      regval |= cfreq << 5;		//cfreq:0~3:1800/2400/3200/4100Hz
      regval |= gain;
      WM8978_Write_Reg(21,regval);
}


void WM8978_EQ5_Set (unsigned char cfreq,unsigned char gain)
{
       unsigned short regval = 0;
       cfreq &= 0x03;
       if (gain > 24) gain = 24;
       gain = 24 - gain;
       regval |= cfreq << 5;		//cfreq:0~3:5300/6900/9000/11700Hz
       regval |= gain;			//gain:0~24,-12~+12dB
       WM8978_Write_Reg(22,regval);
}


unsigned char WM8978_Init (void)
{
      unsigned char res;
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC, ENABLE);
      //PB12,13
      GPIO_Init_Pin(GPIOB,GPIO_Pin_12,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOB,GPIO_Pin_13,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      //PC2,3,6
      GPIO_Init_Pin(GPIOC,GPIO_Pin_2,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOC,GPIO_Pin_3,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOC,GPIO_Pin_6,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_PinAFConfig(GPIOB,GPIO_PinSource12,GPIO_AF_SPI2);	//PB12,AF5  I2S_LRCK
      GPIO_PinAFConfig(GPIOB,GPIO_PinSource13,GPIO_AF_SPI2);	//PB13,AF5  I2S_SCLK
      GPIO_PinAFConfig(GPIOC,GPIO_PinSource3,GPIO_AF_SPI2);	//PC3 ,AF5  I2S_DACDATA
      GPIO_PinAFConfig(GPIOC,GPIO_PinSource6,GPIO_AF_SPI2);	//PC6 ,AF5  I2S_MCK
      GPIO_PinAFConfig(GPIOC,GPIO_PinSource2,GPIO_AF_SPI2);	//PC2 ,AF6  I2S_ADCDATA  I2S2ext_SD
      res = WM8978_Write_Reg(0,0);
      if (res) return 1;
      WM8978_Write_Reg(1,0x1B);		//R1,MICEN,BIASEN,VMIDSEL[1:0]:11(5K)
      WM8978_Write_Reg(2,0x1B0);	//R2,ROUT1,LOUT1,BOOSTENR,BOOSTENL
      WM8978_Write_Reg(3,0x6C);		//R3,LOUT2,ROUT2,RMIX,LMIX
      WM8978_Write_Reg(6,0);		//R6,MCLK
      WM8978_Write_Reg(43,1 << 4);	//R43,INVROUT2
      WM8978_Write_Reg(47,1 << 8);	//R47,PGABOOSTL,MIC
      WM8978_Write_Reg(48,1 << 8);	//R48,PGABOOSTR,MIC
      WM8978_Write_Reg(49,1 << 1);	//R49,TSDEN
      WM8978_Write_Reg(10,1 << 3);	//R10,SOFTMUTE,128x,SNR
      WM8978_Write_Reg(14,1 << 3);	//R14,ADC 128x
      return 0;
}
