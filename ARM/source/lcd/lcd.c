/********************************************************************************/
/* lcd.c                                                                        */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#include "../../hwdefs.h"
#include "../prototype.h"
#include "../color.h"
#include "font.h"
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define	LCD_LED				PB_OUTPUT(15)

typedef struct
{
      volatile unsigned short LCD_REG;
      volatile unsigned short LCD_RAM;
} LCD_TypeDef;

#define LCD_BASE			((unsigned int)(0x6C000000 | 0x0000007E))
#define LCD				((LCD_TypeDef *) LCD_BASE)

#define L2R_U2D				0
#define L2R_D2U				1
#define R2L_U2D				2
#define R2L_D2U				3

#define U2D_L2R				4
#define U2D_R2L				5
#define D2U_L2R				6
#define D2U_R2L  			7

#define DFT_SCAN_DIR			L2R_U2D

#define SSD_HOR_RESOLUTION		480
#define SSD_VER_RESOLUTION		320

#define SSD_HOR_PULSE_WIDTH		1
#define SSD_HOR_BACK_PORCH		46
#define SSD_HOR_FRONT_PORCH		210

#define SSD_VER_PULSE_WIDTH		1
#define SSD_VER_BACK_PORCH		23
#define SSD_VER_FRONT_PORCH		22

#define SSD_HT				(SSD_HOR_RESOLUTION+SSD_HOR_BACK_PORCH+SSD_HOR_FRONT_PORCH)
#define SSD_HPS				(SSD_HOR_BACK_PORCH)
#define SSD_VT				(SSD_VER_RESOLUTION+SSD_VER_BACK_PORCH+SSD_VER_FRONT_PORCH)
#define SSD_VPS				(SSD_VER_BACK_PORCH)


unsigned short foreground_color,background_color,maxx,maxy,ax,ay,max_col,max_row,tx,ty,eng_mode,page;
char buff[256],dumy_data[32];
const unsigned char s_bit[8] = {0x80,0x40,0x20,0x10,8,4,2,1};
volatile unsigned char hangul_mode;


const unsigned char han_tbl [3][32] = {  // 3 * 32
        {0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 1, 2, 3, 4, 5, 0, 0, 6, 7, 8, 9,10,11, 0, 0,12,13,14,15,16,17, 0, 0,18,19,20,21, 0, 0},
        {0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16, 0,17,18,19,20,21,22,23,24,25,26,27, 0, 0}};
const unsigned short loc1[8] = {0,20,40,60,80,100,120,140};
const unsigned short loc2[4] = {160,182,204,226};
const unsigned short loc3[4] = {248,276,304,332};

const unsigned short hangul_chosung_tbl [19] = {
      0x8800, 0x8C00, 0X9000, 0x9400, 0x9800, 0x9C00, 0xA000, 0xA400,
      0xA800, 0xAC00, 0xB000, 0xB400, 0xB800, 0xBC00, 0xC000, 0xC400,
      0xC800, 0xCC00, 0xD000};

const unsigned short hangul_jungsung_tbl [21] = {
      0x8060, 0x8080, 0X80A0, 0x80C0, 0x80E0, 0x8140, 0x8160, 0x8180,
      0x81A0, 0x81C0, 0x81E0, 0x8240, 0x8260, 0x8280, 0x82A0, 0x82C0,
      0x82E0, 0x8340, 0x8360, 0x8380, 0x83A0};

const unsigned short hangul_jongsung_tbl[28] = {
      0x8001, 0x8002, 0x8003, 0x8004, 0x8005, 0x8006, 0x8007, 0x8008,
      0x8009, 0x800A, 0x800B, 0x800C, 0x800D, 0x800E, 0x800F, 0x8010,
      0x8011, 0x8013, 0x8014, 0x8015, 0x8016, 0x8017, 0x8018, 0x8019,
      0x801A, 0x801B, 0x801C, 0x801D};

volatile unsigned short delay_count,delay_tick;
_lcd_dev lcddev;



void delay_us (unsigned short delay)
{
      delay_count = 0;
      while (delay_count < delay) {
      	    for (delay_tick=0;delay_tick<9;delay_tick++);
      	    delay_count++;
      }
}


void LCD_WR_REG (volatile unsigned short regval)
{
      regval = regval;
      LCD->LCD_REG = regval;
}


void LCD_WR_DATA (volatile unsigned short data)
{
      data = data;
      LCD->LCD_RAM = data;
}


 unsigned short LCD_RD_DATA (void)
{
      volatile unsigned short ram;
      ram = LCD->LCD_RAM;
      return ram;
}


void LCD_WriteReg (unsigned short LCD_Reg, unsigned short LCD_RegValue)
{
      LCD->LCD_REG = LCD_Reg;
      LCD->LCD_RAM = LCD_RegValue;
}


unsigned short LCD_ReadReg (unsigned short LCD_Reg)
{
      LCD_WR_REG(LCD_Reg);
      delay_us(5);
      return LCD_RD_DATA();
}


void LCD_WriteRAM_Prepare (void)
{
      LCD->LCD_REG = lcddev.wramcmd;
}


void LCD_WriteRAM (unsigned short RGB_Code)
{
      LCD->LCD_RAM = RGB_Code;
}


 unsigned short LCD_BGR2RGB (unsigned short c)
{
      unsigned short r,g,b,rgb;
      b = (c >> 0) & 0x1f;
      g = (c >> 5) & 0x3f;
      r = (c >> 11) & 0x1f;
      rgb = (b << 11) + (g << 5) + (r << 0);
      return (rgb);
}


void opt_delay (unsigned char i)
{
      while (i--);
}


void LCD_SetCursor (unsigned short Xpos, unsigned short Ypos)
{
      LCD_WR_REG(lcddev.setxcmd);
      LCD_WR_DATA(Xpos >> 8);
      LCD_WR_DATA(Xpos & 0xFF);
      LCD_WR_REG(lcddev.setycmd);
      LCD_WR_DATA(Ypos >> 8);
      LCD_WR_DATA(Ypos & 0xFF);
}


unsigned short LCD_ReadPoint (unsigned short x, unsigned short y)
{
      unsigned short r = 0,g = 0,b = 0;
      if (x >= lcddev.width || y >= lcddev.height) return 0;
      LCD_SetCursor(x,y);
      LCD_WR_REG(0x2E);
      r = LCD_RD_DATA();
      opt_delay(2);
      r = LCD_RD_DATA();
      opt_delay(2);
      b = LCD_RD_DATA();
      g = r & 0xFF;
      g <<= 8;
      return (((r >> 11) << 11) | ((g >> 10) << 5) | (b >> 11));
}


void LCD_DisplayOn (void)
{
      LCD_WR_REG(0x29);
}


void LCD_DisplayOff (void)
{
      LCD_WR_REG(0x28);
}


void LCD_Scan_Dir (unsigned char dir)
{
      unsigned short regval = 0;
      unsigned short dirreg = 0;
      unsigned short temp;
      if (lcddev.dir == 1) {
	 switch (dir) {
	    case 0:
	      dir = 6;
	      break;
	    case 1:
	      dir = 7;
	      break;
	    case 2:
	      dir = 4;
	      break;
	    case 3:
	      dir = 5;
	      break;
	    case 4:
	      dir = 1;
	      break;
	    case 5:
	      dir = 0;
	      break;
	    case 6:
	      dir = 3;
	      break;
	    case 7:
	      dir = 2;
	      break;
	 }
      }
      switch (dir) {
         case L2R_U2D:
           regval |= (0 << 7) | (0 << 6) | (0 << 5);
           break;
         case L2R_D2U:
           regval |= (1 << 7) | (0 << 6) | (0 << 5);
           break;
         case R2L_U2D:
           regval |= (0 << 7) | (1 << 6) | (0 << 5);
           break;
         case R2L_D2U:
           regval |= (1 << 7) | (1 << 6) | (0 << 5);
           break;
         case U2D_L2R:
           regval |= (0 << 7) | (0 << 6) | (1 << 5);
           break;
         case U2D_R2L:
           regval |= (0 << 7) | (1 << 6) | (1 << 5);
           break;
         case D2U_L2R:
           regval |= (1 << 7) | (0 << 6) | (1 << 5);
           break;
         case D2U_R2L:
           regval |= (1 << 7) | (1 << 6) | (1 << 5);
           break;
      }
      dirreg = 0x36;
      LCD_WriteReg(dirreg,regval);
      if (regval & 0x20) {
         if (lcddev.width < lcddev.height) {
            temp = lcddev.width;
            lcddev.width = lcddev.height;
            lcddev.height = temp;
         }
      } else {
         if (lcddev.width > lcddev.height) {
            temp = lcddev.width;
            lcddev.width = lcddev.height;
            lcddev.height = temp;
         }
      }
      LCD_WR_REG(lcddev.setxcmd);
      LCD_WR_DATA(0);
      LCD_WR_DATA(0);
      LCD_WR_DATA((lcddev.width - 1) >> 8);
      LCD_WR_DATA((lcddev.width - 1) & 0xFF);
      LCD_WR_REG(lcddev.setycmd);
      LCD_WR_DATA(0);
      LCD_WR_DATA(0);
      LCD_WR_DATA((lcddev.height - 1) >> 8);
      LCD_WR_DATA((lcddev.height - 1) & 0xFF);
}


void LCD_DrawPoint (unsigned short x, unsigned short y)
{
      if ((x >= maxx) || (y >= maxy)) return;
      LCD_SetCursor(x,y);
      LCD_WriteRAM_Prepare();
      LCD->LCD_RAM = foreground_color;
}


void LCD_Fast_DrawPoint (unsigned short x, unsigned short y, unsigned short color)
{
      if ((x >= maxx) || (y >= maxy)) return;
      LCD_WR_REG(lcddev.setxcmd);
      LCD_WR_DATA(x >> 8);
      LCD_WR_DATA(x & 0xFF);
      LCD_WR_REG(lcddev.setycmd);
      LCD_WR_DATA(y >> 8);
      LCD_WR_DATA(y & 0xFF);
      LCD->LCD_REG = lcddev.wramcmd;
      LCD->LCD_RAM = color;
}


void LCD_SSD_BackLightSet (unsigned char pwm)
{
      LCD_WR_REG(0xBE);
      LCD_WR_DATA(0x05);
      LCD_WR_DATA(pwm * 2.55);
      LCD_WR_DATA(0x01);
      LCD_WR_DATA(0xFF);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
}


void LCD_Display_Dir (unsigned char dir)
{
      if (dir == 0) {
	 lcddev.dir = 0;
	 lcddev.wramcmd = 0x2C;
	 lcddev.setxcmd = 0x2A;
	 lcddev.setycmd = 0x2B;
 	 lcddev.width = 320;
	 lcddev.height = 480;
      } else {
         lcddev.dir = 1;
	 lcddev.wramcmd = 0x2C;
	 lcddev.setxcmd = 0x2A;
	 lcddev.setycmd = 0x2B;
 	 lcddev.width = 480;
	 lcddev.height = 320;
      }
      LCD_Scan_Dir(DFT_SCAN_DIR);
      if (lcddev.dir == 0) {
         maxx = 320;
         maxy = 480;
      } else {
         maxx = 480;
         maxy = 320;
      }
      max_col = (maxx / 8);
      max_row = (maxy / 16);
}


void LCD_Set_Window (unsigned short sx, unsigned short sy, unsigned short width, unsigned short height)
{
      unsigned short twidth,theight;
      twidth = sx + width - 1;
      theight = sy + height - 1;
      LCD_WR_REG(lcddev.setxcmd);
      LCD_WR_DATA(sx >> 8);
      LCD_WR_DATA(sx & 0xFF);
      LCD_WR_DATA(twidth >> 8);
      LCD_WR_DATA(twidth & 0xFF);
      LCD_WR_REG(lcddev.setycmd);
      LCD_WR_DATA(sy >> 8);
      LCD_WR_DATA(sy & 0xFF);
      LCD_WR_DATA(theight >> 8);
      LCD_WR_DATA(theight & 0xFF);
}


void LCD_Clear (unsigned short color)
{
      unsigned int index = 0;
      unsigned int totalpoint = lcddev.width;
      totalpoint *= lcddev.height;
      LCD_SetCursor(0x00,0x0000);
      LCD_WriteRAM_Prepare();
      for (index=0;index<totalpoint;index++) {
	  LCD->LCD_RAM = color;
      }
}


void LCD_Fill (short sx, short sy, short ex, short ey, unsigned short color)
{
      unsigned short i,j;
      unsigned short xlen = 0;
      xlen = ex - sx + 1;
      for (i=sy;i<=ey;i++) {
	  LCD_SetCursor(sx,i);
	  LCD_WriteRAM_Prepare();
	  for (j=0;j<xlen;j++) LCD->LCD_RAM = color;
      }
}


void LCD_Color_Fill (short sx, short sy, short ex, short ey, unsigned short *color)
{
      unsigned short height,width;
      unsigned short i,j;
      width = ex - sx + 1;
      height = ey - sy + 1;
      for (i=0;i<height;i++) {
 	  LCD_SetCursor(sx,sy + i);
	  LCD_WriteRAM_Prepare();
	  for (j=0;j<width;j++) LCD->LCD_RAM = color[i * width + j];
      }
}


void LCD_DrawLine (short x1, short y1, short x2, short y2)
{
      unsigned short t;
      int xerr = 0,yerr = 0,delta_x,delta_y,distance;
      int incx,incy,uRow,uCol;
      delta_x = x2 - x1;
      delta_y = y2 - y1;
      uRow = x1;
      uCol = y1;
      if (delta_x > 0) {
      	 incx = 1;
      } else {
      	 if (delta_x == 0) {
      	    incx = 0;
         } else {
            incx = -1;
            delta_x = -delta_x;
         }
      }
      if (delta_y > 0) {
      	 incy = 1;
      } else {
      	 if (delta_y == 0) {
      	    incy = 0;
	 } else {
	    incy = -1;
	    delta_y = -delta_y;
	 }
      }
      if (delta_x > delta_y) {
      	 distance = delta_x;
      } else {
      	 distance = delta_y;
      }
      for (t=0;t<=distance+1;t++ ) {
	  LCD_DrawPoint(uRow,uCol);
          xerr += delta_x;
	  yerr += delta_y;
	  if (xerr > distance) {
	     xerr -= distance;
	     uRow += incx;
	  }
	  if (yerr > distance) {
	     yerr -= distance;
	     uCol += incy;
	  }
      }
}


void LCD_DrawRectangle (short x1, short y1, short x2, short y2)
{
      LCD_DrawLine(x1,y1,x2,y1);
      LCD_DrawLine(x1,y1,x1,y2);
      LCD_DrawLine(x1,y2,x2,y2);
      LCD_DrawLine(x2,y1,x2,y2);
}


void LCD_Draw_Circle (short x0, short y0,unsigned short r)
{
      int a,b;
      int di;
      a = 0;
      b = r;
      di = 3 - (r << 1);
      while (a <= b) {
	    LCD_DrawPoint(x0 + a,y0 - b);
 	    LCD_DrawPoint(x0 + b,y0 - a);
	    LCD_DrawPoint(x0 + b,y0 + a);
	    LCD_DrawPoint(x0 + a,y0 + b);
	    LCD_DrawPoint(x0 - a,y0 + b);
 	    LCD_DrawPoint(x0 - b,y0 + a);
	    LCD_DrawPoint(x0 - a,y0 - b);
  	    LCD_DrawPoint(x0 - b,y0 - a);
	    a++;
	    if (di < 0) {
	       di += 4 * a + 6;
	    } else {
	       di += 10 + 4 * (a - b);
	       b--;
	    }
      }
}


void set_color (unsigned short color)
{
      foreground_color = color;
}


void set_background_color (unsigned short color)
{
      background_color = color;
}


void set_pixel (unsigned short x, unsigned short y, unsigned short color)
{
      unsigned short temp = foreground_color;
      foreground_color = color;
      //lcd_set_pixel(x,y);
      LCD_Fast_DrawPoint(x,y,color);
      foreground_color = temp;
}


void out_lcd (char wchar)
{
      int idx;
      for (idx=0;idx<8;idx++) {
          if ((s_bit[idx] & wchar) == 0) set_pixel(tx,ty,background_color); else set_pixel(tx,ty,foreground_color);
          tx++;
      }
}


unsigned short uni_to_kssm2 (unsigned short wchar)
{
      unsigned short cho_sung,jung_sung,jong_sung,unicode;
      unsigned short temp = 0x0000;
      unsigned short unicode_hangul_base = 0xAC00;
      unsigned short unicode_hangul_last = 0xD79F;
      unsigned short cho,jung,jong,result;
      temp = wchar;
      if ((temp < unicode_hangul_base) || (temp > unicode_hangul_last)) return temp;
      unicode = temp - unicode_hangul_base;
      cho_sung = unicode / (21 * 28);
      unicode = unicode % (21 * 28);
      jung_sung = unicode / 28;
      unicode = unicode % 28;
      jong_sung = unicode;
      cho = hangul_chosung_tbl[cho_sung];
      jung = hangul_jungsung_tbl[jung_sung];
      jong = hangul_jongsung_tbl[jong_sung];
      result = (cho | jung | jong);
      return result;
}


unsigned short uni_to_kssm (unsigned short wchar)
{
      unsigned short cho,joong,jong;
      unsigned short result = 0x8000;
      cho = 2 + (wchar - 0xAC00) / (21 * 28);
      joong = (wchar - 0xAC00) % (21 * 28) / 28;
      if (joong < 5) joong += 3;
      else if (joong < 11) joong += 5;
      else if (joong < 17) joong += 7;
      else joong += 9;
      jong  = (wchar - 0xAC00) % 28;
      if (jong < 17) jong++;
      else jong += 2;
      result |= cho << 10;
      result |= joong << 5;
      result |= jong;
      return result;
}


unsigned short ks5601_to_kssm (unsigned short wchar)
{
      unsigned char xh,xl;
      unsigned short i,result;
      xh = ((wchar >> 8) & 0xFF);
      xl = (wchar & 0x00FF);
      if ((xh >= 0xB0) && (xl >= 0xA0)) {
         xh = xh - 0xB0;
         xl = xl - 0xA0;
         i = xl + xh * 96;
         result = KS_CODE[i][0] << 8 | KS_CODE[i][1];
         return result;
      }
      return 0;
}


void put_eng (char cdata)
{
      unsigned short val,pi;
      if (cdata < 0x20) {
         if (cdata == 0x0D) ax = 0;
         if (cdata == 0x0A) {
            ay++;
            if (ay >= max_row) {
               ay -= 1;
               ax = 0;
            }
         }
      } else {
	 val = (unsigned short)cdata;
         ty = ay * 16;
         for (pi=0;pi<16;pi++) {
             tx = ax * 8;
             if (eng_mode == 0) {
                dumy_data[pi] = eng_font[val][pi];
             } else {
                if (page == 0) dumy_data[pi] = eng_font[val*2][pi];else dumy_data[pi] = eng_font[val*2+1][pi];
             }
             out_lcd(dumy_data[pi]);
             ty++;
         }
         ax++;
         if (ax >= max_col) {
            ax = 0;
            ay++;
            if (ay >= max_row) {
               ay -= 1;
            }
         }
      }
}


void put_han (unsigned short c)
{
      unsigned char i,first,mid,last,b1,b2,b3;
      unsigned short cc,hlx1,hlx2,hlx3;
      if (hangul_mode == 0) cc = ks5601_to_kssm(c);else cc = uni_to_kssm(c);
      first = ((cc & 0x7C00) >> 10);
      mid = ((cc & 0x03E0) >> 5);
      last = (cc & 0x001F);
      first = han_tbl[0][first];
      mid = han_tbl[1][mid];
      last = han_tbl[2][last];
      b1 = 0;
      b2 = 0;
      b3 = 0;
      switch (mid) {
      	 case 1:
      	 case 3:
      	 case 10:
           b3 = 0;
           break;
         case 5:
         case 7:
         case 12:
         case 15:
         case 17:
         case 20:
         case 21:
           b3 = 1;
           break;
         case 2:
         case 4:
         case 6:
         case 8:
         case 11:
         case 16:
           b3 = 2;
           break;
         case 9:
         case 13:
         case 14:
         case 18:
         case 19:
           b3 = 3;
           break;
      }
      switch (first) {
         case 1:
         case 16:
           if (last == 0) b2 = 0;else b2 = 2;
           break;
         default:
           if (last == 0) b2 = 1;else b2 = 3;
      }
      switch (mid) {
         case 1:
         case 2:
         case 3:
         case 4:
         case 5:
         case 6:
         case 7:
         case 8:
         case 21:
           if (last == 0) b1 = 0; else b1 = 5;
           break;
         case 9:
         case 13:
         case 19:
           if (last == 0) b1 = 1;else b1 = 6;
           break;
         case 14: case 18:
           if (last == 0) b1 = 2;else b1 = 6;
           break;
         case 10:
         case 11:
         case 12:
         case 20:
           if (last == 0) b1 = 3;else b1 = 7;
           break;
         case 15:
         case 16:
         case 17:
           if (last == 0) b1 = 4;else b1 = 7;
           break;
      }
      if ((mid == 0) && (last == 0)) b1 = 0;
      for (i=0;i<16;i++) {
          if (first != 0) {
             hlx1 = first + loc1[b1];
             dumy_data[i*2] = kor_font[hlx1][i*2];
             dumy_data[i*2+1] = kor_font[hlx1][i*2+1];
          }
          if (mid != 0) {
             hlx2 = mid + loc2[b2];
             dumy_data[i*2] = dumy_data[i*2] | kor_font[hlx2][i*2];
             dumy_data[i*2+1] = dumy_data[i*2+1] | kor_font[hlx2][i*2+1];
          }
          if (last != 0) {
             hlx3 = last + loc3[b3];
             dumy_data[i*2] = dumy_data[i*2] | kor_font[hlx3][i*2];
             dumy_data[i*2+1] = dumy_data[i*2+1] | kor_font[hlx3][i*2+1];
          }
      }
      ty = ay * 16;
      for (i=0;i<16;i++) {
          tx = ax * 8;
          out_lcd(dumy_data[i*2]);
          ty++;
      }
      ty = ay * 16;
      for (i=0;i<16;i++) {
          tx = (ax+1) * 8;
          out_lcd(dumy_data[i*2+1]);
          ty++;
      }
      ax += 2;
      if (ax >= max_col) {
         ax = 0;
         ay++;
         if (ay >= max_row) {
            ay -= 1;
         }
      }
}


void put_engxy (unsigned short x,unsigned short y,char pdata)
{
      ax = x;
      ay = y;
      put_eng(pdata);
}


void put_hanxy (unsigned short x,unsigned short y,unsigned short pdata)
{
      ax = x;
      ay = y;
      if (pdata >= 0xAC00) {
         if (pdata >= 0xB0A1) {
            put_han(ks5601_to_kssm(pdata));
         } else {
            put_han(uni_to_kssm(pdata));
         }
      } else {
         put_han(pdata);
      }
}


void lput_char (char *str)
{
      unsigned char len,n;
      unsigned short hangul_char;
      len = strlen(str);
      for (n=0;n<len;n++) {
      	  if ((str[n] & 0x80) == 0) {
             put_eng(str[n]);
          } else {
             hangul_char = str[n];
             hangul_char = hangul_char << 8;
             hangul_char= hangul_char | str[n + 1];
             put_han(hangul_char);
             n++;
          }
      }
}


void lput_char_xy (unsigned short x,unsigned short y,char *str)
{
      unsigned char len,n;
      unsigned short hangul_char;
      ax = x;
      ay = y;
      len = strlen(str);
      for (n=0;n<len;n++) {
      	  if ((str[n] & 0x80) == 0) {
             put_eng(str[n]);
          } else {
             hangul_char = str[n];
             hangul_char = hangul_char << 8;
             hangul_char= hangul_char | str[n + 1];
             put_han(hangul_char);
             n++;
          }
      }
}


void lcd_printf (unsigned short x,unsigned short y,char *form,...)
{
      va_list argptr;
      ax = x;
      ay = y;
      va_start(argptr,form);
      vsprintf(buff,form,argptr);
      lput_char(buff);
      va_end(argptr);
}


void LCD_Init (void)
{
      FSMC_NORSRAMInitTypeDef FSMC_NORSRAMInitStructure;
      FSMC_NORSRAMTimingInitTypeDef readWriteTiming;
      FSMC_NORSRAMTimingInitTypeDef writeTiming;
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOG, ENABLE);
      RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_FSMC,ENABLE);
      GPIO_Init_Pin(GPIOB,GPIO_Pin_15,GPIO_Speed_100MHz,GPIO_Mode_Out_PP);
      //PD0,1,4,5,8,9,10,14,15 AF OUT
      GPIO_Init_Pin(GPIOD,GPIO_Pin_0,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOD,GPIO_Pin_1,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOD,GPIO_Pin_4,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOD,GPIO_Pin_5,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOD,GPIO_Pin_8,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOD,GPIO_Pin_9,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOD,GPIO_Pin_10,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOD,GPIO_Pin_14,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOD,GPIO_Pin_15,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      //PE7~15,AF OUT
      GPIO_Init_Pin(GPIOE,GPIO_Pin_7,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOE,GPIO_Pin_8,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOE,GPIO_Pin_9,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOE,GPIO_Pin_10,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOE,GPIO_Pin_11,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOE,GPIO_Pin_12,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOE,GPIO_Pin_13,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOE,GPIO_Pin_14,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOE,GPIO_Pin_15,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      //PF12,FSMC_A6
      GPIO_Init_Pin(GPIOF,GPIO_Pin_12,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      //PG12,FSMC_NE4
      GPIO_Init_Pin(GPIOG,GPIO_Pin_12,GPIO_Speed_100MHz,GPIO_Mode_AF_PP_PU);
      //AF
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource0,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource1,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource4,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource5,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource8,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource9,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource10,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource14,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource15,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource7,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource8,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource9,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource10,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource11,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource12,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource13,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource14,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOE,GPIO_PinSource15,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOF,GPIO_PinSource12,GPIO_AF_FSMC);
      GPIO_PinAFConfig(GPIOG,GPIO_PinSource12,GPIO_AF_FSMC);

      readWriteTiming.FSMC_AddressSetupTime = 0xF;	// ADDSET16 HCLK 1/168M = 6ns * 16 = 96ns
      readWriteTiming.FSMC_AddressHoldTime = 0x00;	// ADDHLD
      readWriteTiming.FSMC_DataSetupTime = 60;		// 60HCLK = 6 * 60 = 360ns
      readWriteTiming.FSMC_BusTurnAroundDuration = 0x00;
      readWriteTiming.FSMC_CLKDivision = 0x00;
      readWriteTiming.FSMC_DataLatency = 0x00;
      readWriteTiming.FSMC_AccessMode = FSMC_AccessMode_A;

      writeTiming.FSMC_AddressSetupTime =9;		// ADDSET 9HCLK = 54ns
      writeTiming.FSMC_AddressHoldTime = 0x00;
      writeTiming.FSMC_DataSetupTime = 8;		// 6ns * 9HCLK = 54ns
      writeTiming.FSMC_BusTurnAroundDuration = 0x00;
      writeTiming.FSMC_CLKDivision = 0x00;
      writeTiming.FSMC_DataLatency = 0x00;
      writeTiming.FSMC_AccessMode = FSMC_AccessMode_A;

      FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM4;
      FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
      FSMC_NORSRAMInitStructure.FSMC_MemoryType =FSMC_MemoryType_SRAM;			// FSMC_MemoryType_SRAM; SRAM
      FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;	// 16bit
      FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode =FSMC_BurstAccessMode_Disable;	// FSMC_BurstAccessMode_Disable;
      FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
      FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait=FSMC_AsynchronousWait_Disable;
      FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
      FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
      FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
      FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
      FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Enable;
      FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
      FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &readWriteTiming;
      FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &writeTiming;
      FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);
      FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM4, ENABLE);
      wait_ms(50);	// delay 50 ms
      LCD_WriteReg(0x0000,0x0001);
      wait_ms(50);	// delay 50 ms
      lcddev.id = LCD_ReadReg(0x0000);
      if (lcddev.id < 0xFF || lcddev.id == 0xFFFF || lcddev.id == 0x9300) {
	 LCD_WR_REG(0xD3);
	 lcddev.id = LCD_RD_DATA();	//dummy read
 	 lcddev.id = LCD_RD_DATA();
  	 lcddev.id = LCD_RD_DATA();
 	 lcddev.id <<= 8;
	 lcddev.id |= LCD_RD_DATA();
 	 if (lcddev.id != 0x9341) {
 	    LCD_WR_REG(0xBF);
	    lcddev.id = LCD_RD_DATA();	//dummy read
	    lcddev.id = LCD_RD_DATA();
	    lcddev.id = LCD_RD_DATA();
	    lcddev.id = LCD_RD_DATA();
	    lcddev.id <<= 8;
	    lcddev.id |= LCD_RD_DATA();
	    if (lcddev.id != 0x6804) {
	       LCD_WR_REG(0xD4);
	       lcddev.id = LCD_RD_DATA();//dummy read
	       lcddev.id = LCD_RD_DATA();
	       lcddev.id = LCD_RD_DATA();
	       lcddev.id <<= 8;
	       lcddev.id |= LCD_RD_DATA();
	       if (lcddev.id != 0x5310) {
		  LCD_WR_REG(0xDA00);
		  lcddev.id=LCD_RD_DATA();
		  LCD_WR_REG(0xDB00);
		  lcddev.id = LCD_RD_DATA();
		  lcddev.id <<= 8;
		  LCD_WR_REG(0xDC00);
		  lcddev.id |= LCD_RD_DATA();
		  if (lcddev.id == 0x8000) lcddev.id = 0x5510;
		  if (lcddev.id != 0x5510) {
		     LCD_WR_REG(0xA1);
		     lcddev.id = LCD_RD_DATA();
		     lcddev.id = LCD_RD_DATA();
		     lcddev.id <<= 8;
		     lcddev.id |= LCD_RD_DATA();
		     if (lcddev.id == 0x5761) lcddev.id=0x1963;
		  }
	       }
	    }
 	 }
      }
      FSMC_Bank1E->BWTR[6] &= ~(0xF << 0);	// ADDSET
      FSMC_Bank1E->BWTR[6] &= ~(0xF << 8);
      FSMC_Bank1E->BWTR[6] |= 3 << 0;		// ADDSET 3HCLK = 18ns
      FSMC_Bank1E->BWTR[6] |= 2 << 8;		// DATAST 6ns * 3HCLK = 18ns
      //lcddev.id 0x5310
      LCD_WR_REG(0xED);
      LCD_WR_DATA(0x01);
      LCD_WR_DATA(0xFE);
      LCD_WR_REG(0xEE);
      LCD_WR_DATA(0xDE);
      LCD_WR_DATA(0x21);
      LCD_WR_REG(0xF1);
      LCD_WR_DATA(0x01);
      LCD_WR_REG(0xDF);
      LCD_WR_DATA(0x10);
      //VCOMvoltage//
      LCD_WR_REG(0xC4);
      LCD_WR_DATA(0x8F);	//5f
      LCD_WR_REG(0xC6);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xE2);
      LCD_WR_DATA(0xE2);
      LCD_WR_DATA(0xE2);
      LCD_WR_REG(0xBF);
      LCD_WR_DATA(0xAA);
      LCD_WR_REG(0xB0);
      LCD_WR_DATA(0x0D);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x0D);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x11);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x19);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x21);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x2D);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x3D);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x5D);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x5D);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xB1);
      LCD_WR_DATA(0x80);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x8B);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x96);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xB2);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x02);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x03);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xB3);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xB4);
      LCD_WR_DATA(0x8B);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x96);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xA1);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xB5);
      LCD_WR_DATA(0x02);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x03);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x04);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xB6);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xB7);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x3F);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x5E);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x64);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x8C);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xAC);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xDC);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x70);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x90);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xEB);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xDC);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xB8);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xBA);
      LCD_WR_DATA(0x24);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xC1);
      LCD_WR_DATA(0x20);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x54);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xFF);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xC2);
      LCD_WR_DATA(0x0A);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x04);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xC3);
      LCD_WR_DATA(0x3C);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x3A);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x39);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x37);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x3C);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x36);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x32);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x2F);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x2C);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x29);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x26);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x24);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x24);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x23);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x3C);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x36);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x32);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x2F);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x2C);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x29);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x26);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x24);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x24);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x23);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xC4);
      LCD_WR_DATA(0x62);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x05);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x84);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xF0);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x18);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xA4);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x18);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x50);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x0C);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x17);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x95);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xF3);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xE6);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xC5);
      LCD_WR_DATA(0x32);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x44);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x65);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x76);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x88);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xC6);
      LCD_WR_DATA(0x20);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x17);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x01);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xC7);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xC8);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xC9);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xE0);
      LCD_WR_DATA(0x16);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x1C);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x21);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x36);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x46);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x52);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x64);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x7A);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x8B);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x99);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xA8);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xB9);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xC4);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xCA);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xD2);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xD9);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xE0);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xF3);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xE1);
      LCD_WR_DATA(0x16);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x1C);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x22);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x36);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x45);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x52);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x64);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x7A);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x8B);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x99);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xA8);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xB9);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xC4);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xCA);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xD2);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xD8);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xE0);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xF3);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xE2);
      LCD_WR_DATA(0x05);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x0B);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x1B);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x34);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x44);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x4F);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x61);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x79);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x88);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x97);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xA6);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xB7);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xC2);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xC7);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xD1);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xD6);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xDD);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xF3);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xE3);
      LCD_WR_DATA(0x05);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xA);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x1C);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x33);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x44);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x50);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x62);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x78);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x88);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x97);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xA6);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xB7);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xC2);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xC7);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xD1);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xD5);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xDD);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xF3);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xE4);
      LCD_WR_DATA(0x01);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x01);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x02);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x2A);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x3C);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x4B);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x5D);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x74);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x84);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x93);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xA2);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xB3);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xBE);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xC4);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xCD);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xD3);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xDD);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xF3);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xE5);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x02);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x29);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x3C);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x4B);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x5D);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x74);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x84);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x93);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xA2);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xB3);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xBE);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xC4);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xCD);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xD3);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xDC);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xF3);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xE6);
      LCD_WR_DATA(0x11);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x34);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x56);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x76);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x77);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x66);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x88);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x99);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xBB);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x99);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x66);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x55);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x55);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x45);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x43);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x44);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xE7);
      LCD_WR_DATA(0x32);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x55);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x76);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x66);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x67);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x67);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x87);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x99);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xBB);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x99);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x77);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x44);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x56);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x23);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x33);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x45);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xE8);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x99);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x87);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x88);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x77);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x66);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x88);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xAA);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0xBB);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x99);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x66);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x55);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x55);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x44);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x44);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x55);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xE9);
      LCD_WR_DATA(0xAA);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0x00);
      LCD_WR_DATA(0xAA);
      LCD_WR_REG(0xCF);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xF0);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x50);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xF3);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0xF9);
      LCD_WR_DATA(0x06);
      LCD_WR_DATA(0x10);
      LCD_WR_DATA(0x29);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0x3A);
      LCD_WR_DATA(0x55);	//66
      LCD_WR_REG(0x11);
      wait_ms(100);
      LCD_WR_REG(0x29);
      LCD_WR_REG(0x35);
      LCD_WR_DATA(0x00);
      LCD_WR_REG(0x51);
      LCD_WR_DATA(0xFF);
      LCD_WR_REG(0x53);
      LCD_WR_DATA(0x2C);
      LCD_WR_REG(0x55);
      LCD_WR_DATA(0x82);
      LCD_WR_REG(0x2c);
      LCD_Display_Dir(0);
      LCD_LED = 1;
      if (lcddev.dir == 0) {
         maxx = 320;
         maxy = 480;
      } else {
         maxx = 480;
         maxy = 320;
      }
      ax = 0;
      ay = 0;
      page = 0;
      eng_mode = 0;
      hangul_mode = 0;
      max_col = (maxx / 8);
      max_row = (maxy / 16);      
      random_display();       
      foreground_color = WHITE;
      background_color = BLACK;
      LCD_Clear(background_color);
      lcd_printf(0,0," ID: %.4X, xres %d yres %d",lcddev.id,lcddev.width,lcddev.height);
}
