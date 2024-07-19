/********************************************************************************/
/* random_disp.c                                                                */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#include "../../hwdefs.h"
#include "../prototype.h"
#include "../color.h"
#include <stdlib.h>


static const unsigned short a_color[8] = {
      reg565(0x000000), 
      reg565(0x0000FF), 
      reg565(0x00FF00), 
      reg565(0x00FFFF), 
      reg565(0xFF0000), 
      reg565(0xFF00FF), 
      reg565(0xFFFF00), 
      reg565(0xFFFFFF)
};


void random_display (void)
{
      unsigned short i = 0;
      unsigned short x_size = maxx;
      unsigned short y_size = maxy;
      unsigned short num_pixels = 0;
      short r_x0,r_x1,r_y0,r_y1;
      unsigned short a_color_index[8];
      for (i=0;i<8;i++) {
          a_color_index[i] = a_color[i];
      }  
      for (i=0;i<1000;i++) {
          /* Reload IWDG counter */
          IWDG_ReloadCounter();      	
          set_color(a_color_index[i & 7]);
          /* Calculate random positions */
          r_x0 = rand() % x_size - x_size / 2;
          r_y0 = rand() % y_size - y_size / 2;
          r_x1 = r_x0 + rand() % x_size;
          r_y1 = r_y0 + rand() % y_size;
          LCD_Fill(r_x0, r_y0, r_x1, r_y1,foreground_color);
          /* Clip rectangle to visible area and add the number of pixels (for speed computation) */
          if (r_x1 >= x_size) r_x1 = x_size - 1;
          if (r_y1 >= y_size) r_y1 = y_size - 1;
          if (r_x0 < 0) r_x0 = 0;
          if (r_y1 < 0) r_y1 = 0;
          num_pixels += (r_x1 - r_x0) * (r_y1 - r_y0);
      }
}
