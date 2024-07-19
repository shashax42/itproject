/********************************************************************************/
/* audio.c                                                                      */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#include "../../hwdefs.h"
#include "../prototype.h"
#include "../color.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define audio_file_address			(((unsigned char *)0x68000000))

void (*audio_play_function)(void);
void (*audio_stop_function)(void);

char *audio_file;

const char *music_extend [4] = {"MP3","APE","WAV","FLAC"};
unsigned char music_number,fnum,music_play,dp_page,display_line,display_line_flag,rflag;
__audiodev audiodev;
unsigned short wait_count;

FATFS *fs[_VOLUMES];
FIL *file;
FIL *ftemp;
UINT br,bw;
FILINFO fileinfo;
DIR dir;
unsigned char *fatbuf;
extern char _end;			/* Defined by the linker */
static char *heap_end;
volatile unsigned char play_key,play_audio,play_key_count;

volatile unsigned char audio_delay,audio_decode_flag,audio_read_flag,audio_run,audio_pause,audio_repeat,old_volume;
unsigned int audio_old_sec,audio_br = 0;

extern unsigned char old_ir;
extern volatile unsigned char volume,volume_flag;



#define SBRK_VERBOSE 			0


char *get_heap_end (void)
{
      return (char*) heap_end;
}


char *get_stack_top (void)
{
      return (char*) __get_MSP();
}


int _kill (int pid, int sig)
{
      pid = pid; sig = sig;		/* avoid warnings */
      errno = EINVAL;
      return -1;
}


void _exit (int status)
{
      lcd_printf(0,29,"_exit called with parameter %d ", status);
      while(1) {;}
}


int _getpid (void)
{
      return 1;
}


void *_sbrk (int incr)
{
      char *prev_heap_end;
#if SBRK_VERBOSE
      lcd_printf(0,29,"_sbrk called with incr %d ", incr);
#endif
      if (heap_end == 0) {
	 heap_end = &_end;
      }
      prev_heap_end = heap_end;
#if 1
      if (heap_end + incr > get_stack_top()) {
	  lcd_printf(0,29,"Heap and stack collision ");
         abort();
      }
#endif
      heap_end += incr;
      return (void *)prev_heap_end;
}


unsigned char char_upper (unsigned char c)
{
      if (c < 'A') return c;
      if (c >= 'a') return c - 0x20;else return c;
}


void audio_file_list (void)
{
      unsigned int index;
      foreground_color = YELLOW;
      lcd_printf(0,3 + display_line,"                                        ");
      if (display_line_flag == 1) {
		foreground_color = WHITE;
         lcd_printf(0,18,"play_list");
		lcd_printf(16,18,"back");
		lcd_printf(28,18,"memo");
		foreground_color = YELLOW;
         display_line_flag = 2;
      }
      index = (dp_page + display_line) * 128;
      lcd_printf(0,3 + display_line,"%2d: %s ",(dp_page + display_line + 1),(audio_file + index));
      display_line++;
      if (display_line >= 15) {
      	 display_line = 0;
      	 display_line_flag = 0;
      }
      foreground_color = WHITE;
}


unsigned char find_music (void)
{
      unsigned char res,i,find;
      unsigned int index;
      char *fn;
      find = 0;
#if _USE_LFN
      fileinfo.lfsize = _MAX_LFN * 2 + 1;
      fileinfo.lfname = (void *)malloc(fileinfo.lfsize);
#endif
      res = f_opendir(&dir,(const TCHAR *)"/audio");
      if (res == FR_OK) {
         while ((f_readdir(&dir, &fileinfo) == FR_OK) && fileinfo.fname[0]) {
#if _USE_LFN
      fn = *fileinfo.lfname ? fileinfo.lfname : fileinfo.fname;
#else
      fn = fileinfo.fname;;
#endif
	       i = strlen(fn);
	       if (strcasecmp(&fn[i-3],music_extend[0]) == 0 || strcasecmp(&fn[i-3],music_extend[1]) == 0 || strcasecmp(&fn[i-3],music_extend[2]) == 0 || strcasecmp(&fn[i-4],music_extend[3]) == 0) {
	       	  index = find * 128;
	       	  strcpy(audio_file + index,fn);
	          find++;
	       }
         }
         lcd_printf(30,1,"Find %d ",find);
         display_line = 0;
         display_line_flag = 1;
#if _USE_LFN         
         free(fileinfo.lfname);
#endif
	 return find;
      }
#if _USE_LFN 
      free(fileinfo.lfname);
#endif
      return 0;
}


void wm8978_volset (unsigned char vol)
{
      if (vol > 63) vol = 63;
      if (wm8978set.speakersw) WM8978_SPKvol_Set(vol);else WM8978_SPKvol_Set(0);
      WM8978_HPvol_Set(vol,vol);
}


void wm8978_eqset (_wm8978_obj *wmset,unsigned char eqx)
{
      switch (eqx) {
	 case 0:
	   WM8978_EQ1_Set(wmset->cfreq[eqx],wmset->freqval[eqx]);
	   break;
	 case 1:
	   WM8978_EQ2_Set(wmset->cfreq[eqx],wmset->freqval[eqx]);
	   break;
	 case 2:
	   WM8978_EQ3_Set(wmset->cfreq[eqx],wmset->freqval[eqx]);
	   break;
 	 case 3:
	   WM8978_EQ4_Set(wmset->cfreq[eqx],wmset->freqval[eqx]);
	   break;
         case 4:
	   WM8978_EQ5_Set(wmset->cfreq[eqx],wmset->freqval[eqx]);
	   break;
      }
}


void audio_start (void)
{
      audiodev.status |= 1 << 1;
      audiodev.status |= 1 << 0;
      I2S_Play_Start();
}


void audio_stop (void)
{
      audiodev.status &= ~(1 << 0);
      audiodev.status &= ~(1 << 1);
      I2S_Play_Stop();
}


void audio_init (void)
{
      unsigned short idx;
      for (idx=0;idx<_VOLUMES;idx++) {
          fs[idx] = (FATFS *)malloc(sizeof(FATFS));
          if (!fs[idx]) break;
      }
      file = (FIL *)malloc(sizeof(FIL));
      ftemp = (FIL *)malloc(sizeof(FIL));
      fatbuf = (unsigned char *)malloc(512);
      audio_file = (char *)audio_file_address;
      rtc_init();
      WM8978_Init();
      FSMC_SRAM_Init();
      memset(audio_file,0,32768);
      SD_Init();
      /* Reload IWDG counter */
      IWDG_ReloadCounter();
      wm8978_read_para(&wm8978set);
      if (wm8978set.saveflag != 0x0A) {
         wm8978set.mvol = 30;
         for (idx=0;idx<5;idx++) wm8978set.cfreq[idx] = 0;
         wm8978set.d3 = 0;
         for (idx=0;idx<5;idx++) wm8978set.freqval[idx] = 12;
         wm8978set.speakersw = 0;
         wm8978set.saveflag = 0x0A;
         wm8978_save_para(&wm8978set);
      }
      wm8978set.speakersw = 0;
      wm8978_volset(wm8978set.mvol);
      wm8978set.mvol = volume;
      for (idx=0;idx<5;idx++) {
	  wm8978_eqset(&wm8978set,idx);
      }
      WM8978_3D_Set(wm8978set.d3);
      foreground_color = WHITE;
      background_color = BLACK;
      LCD_Clear(background_color);
      lcd_printf(0,0,"mp3 player");
      lcd_printf(0,1," Audio Files ");
      f_mount(fs[0],"0:",1);
      fnum = find_music();
      music_number = 0;
      music_play = 0;
      WM8978_ADDA_Cfg(1,0);
      WM8978_Input_Cfg(0,0,0);
      WM8978_Output_Cfg(1,0);
}


void music_start (unsigned char flag)
{
      char pname[256];
      unsigned int index;
      unsigned char idx;
      if ((music_number < fnum) && (audio_play_function == NULL)) {
         if (flag) {
            if (flag == 1) {
               music_number++;
               if (music_number >= fnum) music_number = 0;
            } else {
               if (flag == 2) {
                  music_number+=rand()%fnum;
				  if (music_number >= fnum) music_number = music_number%fnum;
               }
            }
         }
         for (idx=19;idx<30;idx++) {
             lcd_printf(0,idx,"                                        ");
         } 
	 if (audiodev.file->fs) {
	    f_close(audiodev.file);
	    audiodev.file->fs = NULL;
	 }
         memset(pname,0,256);
         wm8978_volset(volume);
         index = music_number * 128;
         strcpy(pname,(const TCHAR *)"/audio/");
         strcat(pname,audio_file + index);
         idx = strlen(pname);
         if (strcasecmp(&pname[idx - 3],music_extend[0]) == 0) {	// MP3
	    lcd_printf(0,19,"MP3  file no %d ",music_number + 1);
            if (audio_play_function == NULL) audio_play_function = mp3_play;
            if (audio_stop_function == NULL) audio_stop_function = mp3_stop;
	    mp3_play_song((unsigned char *)pname);
	 } else if (strcasecmp(&pname[idx - 3],music_extend[2]) == 0) { // WAV
            lcd_printf(0,19,"WAV  file no %d ",music_number + 1);
            if (audio_play_function == NULL) audio_play_function = wave_play;
            if (audio_stop_function == NULL) audio_stop_function = wave_stop;
	    wav_play_song((unsigned char *)pname);
	 } else if (strcasecmp(&pname[idx - 4],music_extend[3]) == 0) { // FLAC
	    lcd_printf(0,19,"FLAC file no %d ",music_number + 1);
            if (audio_play_function == NULL) audio_play_function = flac_play;
            if (audio_stop_function == NULL) audio_stop_function = flac_stop;
	    flac_play_song((unsigned char *)pname);	 	
	 } else if (strcasecmp(&pname[idx - 3],music_extend[1]) == 0) { // APE
	    lcd_printf(0,19,"APE file no %d ",music_number + 1);
            if (audio_play_function == NULL) audio_play_function = ape_play;
            if (audio_stop_function == NULL) audio_stop_function = ape_stop;	 	
	    ape_play_song((unsigned char *)pname);
	    return;	    
	 } else {
            audio_run = 0;
            audio_read_flag = 0;
            audio_pause = 0;
            audio_play_function = NULL;
            audio_stop_function = NULL;
            music_play = 1;	 	
	    return;		 	
	 }
      }
}



void audio_process (void)
{
      if (play_key) play_key_count++;
      if ((play_key) && (play_key_count >= 50)) {
         play_key_count = 0;
      	 if (play_key & 0x01) {		// PLAY/PAUSE
      	    if (audio_run == 0) {
      	       if (play_audio == 0) {
      	          play_audio = 1;
		  music_play = 0;
      	          music_start(0);
      	       } else {
      	       	  if (music_play == 0) {
      	       	     if (music_number) music_number--;else music_number = fnum - 1;
      	       	     music_play = 1;
      	       	  }
      	       }
      	    } else {
      	       if (audio_pause) {
      	       	  audio_pause = 0;
      	       	  audio_start();
      	       } else {
      	       	  audio_pause = 1;
      	       	  audio_stop();
      	       }
            }
      	    play_key &= ~(0x01);
      	 }
      	 if (play_key & 0x02) {
      	    if (play_audio == 0) {
               music_number++;
               if (music_number >= fnum) music_number = 0;
      	       lcd_printf(0,20,"                                        ");
      	       lcd_printf(0,19,"Select file no %d  %s ",music_number + 1,audio_file + (music_number * 128));
      	    } else {
      	       if (audio_stop_function) audio_stop_function();
      	    }
      	    play_key &= ~(0x02);
         }
      	 if (play_key & 0x04) {
      	    if (play_audio == 0) {
      	       if (music_number) music_number--;else music_number = fnum - 1;
      	       lcd_printf(0,20,"                                        ");
      	       lcd_printf(0,19,"Select file no %d  %s ",music_number + 1,audio_file + (music_number * 128));
      	    } else {
      	       if (audio_stop_function) audio_stop_function();
      	       if (music_number) music_number--;else music_number = fnum - 1;
      	       if (music_number) music_number--;else music_number = fnum - 1;
      	    }
      	    play_key &= ~(0x04);
         }
      	 if (play_key & 0x08) {
      	    if (volume) volume--;else volume = 0;
      	    volume_flag = 1;
      	    lcd_printf(20,28,"Volume: %2d ",volume);
      	    play_key &= ~(0x08);
      	 }
      	 if (play_key & 0x10) {
      	    volume++;
      	    if (volume >= 63) volume = 63;
      	    volume_flag = 1;
      	    lcd_printf(20,28,"Volume: %2d ",volume);
      	    play_key &= ~(0x10);
         }
      	 if (play_key & 0x20) {
		 /*
      	    if (dp_page) {
      	       dp_page--;
               display_line = 0;
               display_line_flag = 1;
            }
			*/
      	    play_key &= ~(0x20);
         }
      	 if (play_key & 0x40) {
      	    /*if (dp_page) {
      	       dp_page = 0;
               display_line = 0;
               display_line_flag = 1;
            }*/
			if(audio_repeat){
				audio_repeat=0;
				lcd_printf(0,1,"            ");
				} 
			else {
				audio_repeat=1;
				lcd_printf(0,1,"repeat      ");
			}
      	    play_key &= ~(0x40);
         }
      	 if (play_key & 0x80) {
      	    /*if ((dp_page + 15) < (fnum)) {
      	       dp_page++;
               display_line = 0;
               display_line_flag = 1;
            }*/
			
			if(rflag){
				rflag=0;	
				lcd_printf(0,1,"              ");
			}
			else {
				rflag=1;
				lcd_printf(0,1,"random      ");
			}
      	    play_key &= ~(0x80);
         }
      }
      if (volume_flag) {
         volume_flag = 0;
         if (volume != old_volume) {
            old_volume = volume;
            wm8978_volset(volume);
         }
      }      
      if ((music_play) && (play_audio) && (audio_run == 0)) {
      	 wait_count++;
      	 if (wait_count >= 500) {
      	    wait_count = 0;
			if (rflag) music_start(2);
            if (audio_repeat) music_start(0);else music_start(1);
            music_play = 0;
         }
      }
}
