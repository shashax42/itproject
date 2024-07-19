/********************************************************************************/
/* touch_panel.c                                                                */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#include "../../hwdefs.h"
#include "../prototype.h"
#include "../color.h"
#include <stdlib.h>
#include <math.h>

#define PEN				PB_INPUT(1)  	// T_PEN
#define DOUT				PB_INPUT(2)   	// T_MISO
#define TDIN				PF_OUTPUT(11)  	// T_MOSI
#define TCLK				PB_OUTPUT(0)  	// T_SCK
#define TCS				PC_OUTPUT(13)  	// T_CS

#define ERR_RANGE			50
#define READ_TIMES			5
#define LOST_VAL			1
#define SAVE_ADDR_BASE 			40


_m_tp_dev tp_dev;
unsigned char CMD_RDX = 0xD0;
unsigned char CMD_RDY = 0x90;

extern volatile unsigned char time_led,rx_led,tx_led;




void TP_Adj_Info_Show (unsigned short x0,unsigned short y0,unsigned short x1,unsigned short y1,unsigned short x2,unsigned short y2,unsigned short x3,unsigned short y3,unsigned short fac)
{
      set_color(RED);
      lcd_printf(1,7,"Adjust_Info.");
      lcd_printf(1,8,"x0: %d y0:%d",x0,y0);
      lcd_printf(1,9,"x1: %d y1:%d",x1,y1);
      lcd_printf(1,10,"x2: %d y2:%d",x2,y2);
      lcd_printf(1,11,"x3: %d y3:%d",x3,y3);
      lcd_printf(1,12,"factor: %d ",fac);
}


void TP_Save_Adjdata (void)
{
      int temp;
      U32_REG u32_data;
      U16_REG u16_data;
      temp = tp_dev.xfac * 100000000;
      u32_data.reg = temp;
      eeprom_write(SAVE_ADDR_BASE + 0,u32_data.data[0]);
      eeprom_write(SAVE_ADDR_BASE + 1,u32_data.data[1]);
      eeprom_write(SAVE_ADDR_BASE + 2,u32_data.data[2]);
      eeprom_write(SAVE_ADDR_BASE + 3,u32_data.data[3]);
      temp = tp_dev.yfac * 100000000;
      u32_data.reg = temp;
      eeprom_write(SAVE_ADDR_BASE + 4,u32_data.data[0]);
      eeprom_write(SAVE_ADDR_BASE + 5,u32_data.data[1]);
      eeprom_write(SAVE_ADDR_BASE + 6,u32_data.data[2]);
      eeprom_write(SAVE_ADDR_BASE + 7,u32_data.data[3]);
      u16_data.reg = tp_dev.xoff;
      eeprom_write(SAVE_ADDR_BASE + 8,u16_data.data[0]);
      eeprom_write(SAVE_ADDR_BASE + 9,u16_data.data[1]);
      u16_data.reg = tp_dev.yoff;
      eeprom_write(SAVE_ADDR_BASE + 10,u16_data.data[0]);
      eeprom_write(SAVE_ADDR_BASE + 11,u16_data.data[1]);
      eeprom_write(SAVE_ADDR_BASE + 12,tp_dev.touchtype);
      eeprom_write(SAVE_ADDR_BASE + 13,0xAA);
}


unsigned char TP_Get_Adjdata (void)
{
      int tempfac;
      U32_REG u32_data;
      U16_REG u16_data;
      tempfac = eeprom_read(SAVE_ADDR_BASE + 13);
      if (tempfac == 0xAA) {
         u32_data.data[0] = eeprom_read(SAVE_ADDR_BASE + 0);
         u32_data.data[1] = eeprom_read(SAVE_ADDR_BASE + 1);
         u32_data.data[2] = eeprom_read(SAVE_ADDR_BASE + 2);
         u32_data.data[3] = eeprom_read(SAVE_ADDR_BASE + 3);
      	 tempfac = u32_data.reg;
	 tp_dev.xfac = (float)tempfac / 100000000;
         u32_data.data[0] = eeprom_read(SAVE_ADDR_BASE + 4);
         u32_data.data[1] = eeprom_read(SAVE_ADDR_BASE + 5);
         u32_data.data[2] = eeprom_read(SAVE_ADDR_BASE + 6);
         u32_data.data[3] = eeprom_read(SAVE_ADDR_BASE + 7);
      	 tempfac = u32_data.reg;
	 tp_dev.yfac = (float)tempfac / 100000000;
         u16_data.data[0] = eeprom_read(SAVE_ADDR_BASE + 8);
         u16_data.data[1] = eeprom_read(SAVE_ADDR_BASE + 9);
	 tp_dev.xoff = u16_data.reg;
         u16_data.data[0] = eeprom_read(SAVE_ADDR_BASE + 10);
         u16_data.data[1] = eeprom_read(SAVE_ADDR_BASE + 11);
	 tp_dev.yoff = u16_data.reg;
 	 tp_dev.touchtype = eeprom_read(SAVE_ADDR_BASE + 12);
	 if (tp_dev.touchtype) {
	    CMD_RDX = 0x90;
	    CMD_RDY = 0xD0;
	 } else {
	    CMD_RDX = 0xD0;
	    CMD_RDY = 0x90;
	 }
         return 1;
      }
      return 0;
}


void TP_Write_Byte (unsigned char num)
{
      unsigned char count = 0;
      for (count=0;count<8;count++) {
	  if (num & 0x80) TDIN = 1;else TDIN = 0;
	  num <<= 1;
	  TCLK = 0;
	  delay_us(1);
	  TCLK = 1;
      }
}


unsigned short TP_Read_AD (unsigned char CMD)
{
      unsigned char count = 0;
      unsigned short Num = 0;
      TCLK = 0;
      TDIN = 0;
      TCS = 0;
      TP_Write_Byte(CMD);
      delay_us(6);	//ADS7846
      TCLK = 0;
      delay_us(1);
      TCLK = 1;
      delay_us(1);
      TCLK = 0;
      for (count=0;count<16;count++) {
	  Num <<= 1;
	  TCLK = 0;
	  delay_us(1);
 	  TCLK = 1;
 	  if (DOUT) Num++;
      }
      Num >>= 4;
      TCS = 1;
      return(Num);
}


unsigned short TP_Read_XOY (unsigned char xy)
{
      unsigned short i,j;
      unsigned short buf[READ_TIMES];
      unsigned short sum = 0;
      unsigned short temp;
      for (i=0;i<READ_TIMES;i++) buf[i] = TP_Read_AD(xy);
      for (i=0;i<READ_TIMES-1;i++) {
	  for (j=i+1;j<READ_TIMES;j++) {
	      if (buf[i] > buf[j]) {
		 temp = buf[i];
		 buf[i] = buf[j];
		 buf[j] = temp;
	      }
	  }
      }
      sum = 0;
      for (i=LOST_VAL;i<READ_TIMES-LOST_VAL;i++) sum += buf[i];
      temp = sum / (READ_TIMES - 2 * LOST_VAL);
      return temp;
}


unsigned char TP_Read_XY (unsigned short *x,unsigned short *y)
{
      unsigned short xtemp,ytemp;
      xtemp = TP_Read_XOY(CMD_RDX);
      ytemp = TP_Read_XOY(CMD_RDY);
      *x = xtemp;
      *y = ytemp;
      return 1;
}


unsigned char TP_Read_XY2 (unsigned short *x,unsigned short *y)
{
      unsigned short x1,y1;
      unsigned short x2,y2;
      unsigned char flag;
      flag = TP_Read_XY(&x1,&y1);
      if (flag == 0) return(0);
      flag = TP_Read_XY(&x2,&y2);
      if (flag == 0) return(0);
      if (((x2 <= x1 && x1 < x2 + ERR_RANGE) || (x1 <= x2 && x2 < x1 + ERR_RANGE)) && ((y2 <= y1 && y1 < y2 + ERR_RANGE) || (y1 <= y2 && y2 < y1 + ERR_RANGE))) {
         *x = (x1 + x2) / 2;
         *y = (y1 + y2) / 2;
         return 1;
      } else {
      	 return 0;
      }
}


unsigned char TP_Scan (unsigned char tp)
{
      if (PEN == 0) {
	 if (tp) {
	    TP_Read_XY2(&tp_dev.x[0],&tp_dev.y[0]);
	 } else {
	    if (TP_Read_XY2(&tp_dev.x[0],&tp_dev.y[0])) {
	       tp_dev.x[0] = tp_dev.xfac * tp_dev.x[0] + tp_dev.xoff;
	       tp_dev.y[0] = tp_dev.yfac * tp_dev.y[0] + tp_dev.yoff;
	    }
	 }
	 if ((tp_dev.sta & TP_PRES_DOWN) == 0) {
	    tp_dev.sta = TP_PRES_DOWN | TP_CATH_PRES;
	    tp_dev.x[4] = tp_dev.x[0];
	    tp_dev.y[4] = tp_dev.y[0];
	 }
      } else {
	 if (tp_dev.sta & TP_PRES_DOWN) {
	    tp_dev.sta &= ~(1 << 7);
	 } else {
	    tp_dev.x[4] = 0;
	    tp_dev.y[4] = 0;
	    tp_dev.x[0] = 0xffff;
	    tp_dev.y[0] = 0xffff;
	 }
      }
      return tp_dev.sta & TP_PRES_DOWN;
}


void TP_Drow_Touch_Point (unsigned short x,unsigned short y,unsigned short color)
{
      set_color(color);
      LCD_DrawLine(x - 2,y,x + 3,y);
      LCD_DrawLine(x,y - 2,x,y + 3);
      LCD_DrawPoint(x + 2,y + 2);
      LCD_DrawPoint(x - 2,y + 2);
      LCD_DrawPoint(x + 2,y - 2);
      LCD_DrawPoint(x - 2,y - 2);
      LCD_Draw_Circle(x,y,2);
}

void TP_Drow_Touch_Point2 (unsigned short x,unsigned short y,unsigned short x2,unsigned short y2,unsigned short color)
{
      set_color(color);
      LCD_DrawLine(x - 2,y,x + 3,y);
      LCD_DrawLine(x,y - 2,x,y + 3);
	  LCD_DrawLine(x,y,x2,y2);
      LCD_DrawPoint(x + 2,y + 2);
      LCD_DrawPoint(x - 2,y + 2);
      LCD_DrawPoint(x + 2,y - 2);
      LCD_DrawPoint(x - 2,y - 2);
      LCD_Draw_Circle(x,y,2);
}


void TP_Adjust (void)
{
      unsigned short pos_temp[4][2];
      unsigned char cnt = 0;
      unsigned short d1,d2;
      unsigned int tem1,tem2;
      double fac;
      unsigned short outtime = 0;
      cnt = 0;
      LCD_Clear(background_color);
      lcd_printf(1,5," Toutch_Adjust");
      TP_Drow_Touch_Point(20,20,RED);
      tp_dev.sta = 0;
      tp_dev.xfac = 0;
      while (1)	{
            if (tick) {
               tick = 0;      	
               /* Reload IWDG counter */
               IWDG_ReloadCounter();  
               serial_check();
               if (time_led == ON) LED_OUT0 = 0;else LED_OUT0 = 1;
               if ((tx_led) || (rx_led)) LED_OUT1 = 0;
               if ((tx_led == 0) && (rx_led == 0)) LED_OUT1 = 1;                  	
      	       TP_Scan(1);             
            }
	    if ((tp_dev.sta & 0xC0) == TP_CATH_PRES) {	
	       outtime = 0;
	       tp_dev.sta &= ~(1 << 6);
	       pos_temp[cnt][0] = tp_dev.x[0];
	       pos_temp[cnt][1] = tp_dev.y[0];
	       cnt++;
	       switch (cnt) {
	       	  case 1:
		    TP_Drow_Touch_Point(20,20,WHITE);
		    TP_Drow_Touch_Point(lcddev.width - 20,20,RED);
		    break;
		  case 2:
 		    TP_Drow_Touch_Point(lcddev.width - 20,20,WHITE);
		    TP_Drow_Touch_Point(20,lcddev.height - 20,RED);
	            break;
		  case 3:
 		    TP_Drow_Touch_Point(20,lcddev.height - 20,WHITE);
 		    TP_Drow_Touch_Point(lcddev.width - 20,lcddev.height - 20,RED);
		    break;
		  case 4:
		    tem1 = abs(pos_temp[0][0] - pos_temp[1][0]);
		    tem2 = abs(pos_temp[0][1] - pos_temp[1][1]);
		    tem1 *= tem1;
		    tem2 *= tem2;
		    d1 = sqrt(tem1 + tem2);
		    tem1 = abs(pos_temp[2][0] - pos_temp[3][0]);
		    tem2 = abs(pos_temp[2][1] - pos_temp[3][1]);
		    tem1 *= tem1;
		    tem2 *= tem2;
		    d2 = sqrt(tem1 + tem2);
		    fac = (float)d1 / d2;
		    if (fac < 0.95 || fac > 1.05 || d1 == 0 || d2 == 0) {
		       cnt = 0;
 		       TP_Drow_Touch_Point(lcddev.width - 20,lcddev.height - 20,WHITE);
   	 	       TP_Drow_Touch_Point(20,20,RED);
 		       TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac * 100);
 		       continue;
		    }
		    tem1 = abs(pos_temp[0][0] - pos_temp[2][0]);
		    tem2 = abs(pos_temp[0][1] - pos_temp[2][1]);
		    tem1 *= tem1;
		    tem2 *= tem2;
	            d1 = sqrt(tem1 + tem2);
		    tem1 = abs(pos_temp[1][0] - pos_temp[3][0]);
		    tem2 = abs(pos_temp[1][1] - pos_temp[3][1]);
		    tem1 *= tem1;
		    tem2 *= tem2;
		    d2 = sqrt(tem1 + tem2);
		    fac = (float)d1 / d2;
		    if (fac < 0.95 || fac > 1.05) {
		       cnt = 0;
 		       TP_Drow_Touch_Point(lcddev.width - 20,lcddev.height - 20,WHITE);
   	 	       TP_Drow_Touch_Point(20,20,RED);
 		       TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac * 100);
		       continue;
		    }
		    tem1 = abs(pos_temp[1][0] - pos_temp[2][0]);
		    tem2 = abs(pos_temp[1][1] - pos_temp[2][1]);
		    tem1 *= tem1;
		    tem2 *= tem2;
		    d1 = sqrt(tem1 + tem2);
		    tem1 = abs(pos_temp[0][0] - pos_temp[3][0]);
		    tem2 = abs(pos_temp[0][1] - pos_temp[3][1]);
		    tem1 *= tem1;
		    tem2 *= tem2;
		    d2 = sqrt(tem1 + tem2);
		    fac = (float)d1 / d2;
		    if (fac < 0.95 || fac > 1.05) {
		       cnt = 0;
 		       TP_Drow_Touch_Point(lcddev.width - 20,lcddev.height - 20,WHITE);
   	 	       TP_Drow_Touch_Point(20,20,RED);
 		       TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac * 100);
		       continue;
		    }
		    tp_dev.xfac = (float)(lcddev.width - 40) / (pos_temp[1][0] - pos_temp[0][0]);
		    tp_dev.xoff = (lcddev.width - tp_dev.xfac * (pos_temp[1][0] + pos_temp[0][0])) / 2;
		    tp_dev.yfac = (float)(lcddev.height - 40) / (pos_temp[2][1] - pos_temp[0][1]);
		    tp_dev.yoff = (lcddev.height - tp_dev.yfac * (pos_temp[2][1] + pos_temp[0][1])) / 2;
		    if (abs((int)tp_dev.xfac) > 2 || abs((int)tp_dev.yfac) > 2) {
		       cnt = 0;
 		       TP_Drow_Touch_Point(lcddev.width - 20,lcddev.height - 20,WHITE);
   	 	       TP_Drow_Touch_Point(20,20,RED);
		       lcd_printf(1,6,"TP Need readjust!");
		       tp_dev.touchtype = !tp_dev.touchtype;
		       if (tp_dev.touchtype) {
			  CMD_RDX = 0x90;
			  CMD_RDY = 0xD0;
		       } else {
			  CMD_RDX = 0xD0;
			  CMD_RDY = 0x90;
		       }
		       continue;
		    }
		    LCD_Clear(background_color);
		    lcd_printf(1,6,"Touch Screen Adjust OK!");
		    wait_ms(500);
		    TP_Save_Adjdata();
 		    LCD_Clear(background_color);
		    return;
	       }
	    }
	    wait_ms(10);
	    outtime++;
	    if (outtime > 1000) {
	       outtime = 0;	
               lcd_printf(1,5," Toutch_Adjust Time out");	       
	       wait_ms(500);	       
               set_color(WHITE);
               set_background_color(BLACK);
	       LCD_Clear(background_color);		       
	       TP_Get_Adjdata();
	       break;
	    }
      }
}


unsigned char TP_Init (void)
{
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOF, ENABLE);
      GPIO_Init_Pin(GPIOB,GPIO_Pin_1,GPIO_Speed_100MHz,GPIO_Mode_IPU);
      GPIO_Init_Pin(GPIOB,GPIO_Pin_2,GPIO_Speed_100MHz,GPIO_Mode_IPU);
      GPIO_Init_Pin(GPIOB,GPIO_Pin_0,GPIO_Speed_100MHz,GPIO_Mode_Out_PP);
      GPIO_Init_Pin(GPIOC,GPIO_Pin_13,GPIO_Speed_100MHz,GPIO_Mode_Out_PP);
      GPIO_Init_Pin(GPIOF,GPIO_Pin_11,GPIO_Speed_100MHz,GPIO_Mode_Out_PP);
      tp_dev.x[0] = 0;
      tp_dev.x[1] = 0;
      tp_dev.x[2] = 0;
      tp_dev.x[3] = 0;
      tp_dev.x[4] = 0;
      tp_dev.y[0] = 0;
      tp_dev.y[1] = 0;
      tp_dev.y[2] = 0;
      tp_dev.y[3] = 0;
      tp_dev.y[4] = 0;
      tp_dev.sta = 0;
      tp_dev.xfac = 0.0;
      tp_dev.yfac = 0.0;
      tp_dev.xoff = 0;
      tp_dev.yoff = 0;
      tp_dev.touchtype = 0;
      TP_Read_XY(&tp_dev.x[0],&tp_dev.y[0]);
      if (TP_Get_Adjdata()) {
         return 0;
      } else {
	 LCD_Clear(background_color);
	 TP_Adjust();
 	 TP_Save_Adjdata();
      }
      TP_Get_Adjdata();
      return 1;
}
