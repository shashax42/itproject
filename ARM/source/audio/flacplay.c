/********************************************************************************/
/* flacplay.c                                                                   */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#include "../../hwdefs.h"
#include "../prototype.h"
#include <string.h>


__flacctrl *flacctrl;
volatile unsigned char flactransferend = 0;
volatile unsigned char flacwitchbuf = 0;
FLACContext *flac_fc = 0;
int flac_bytesleft;
unsigned char *flac_buffer = 0;
unsigned char *flac_decbuf0 = 0;
unsigned char *flac_decbuf1 = 0;
unsigned int flac_fptr = 0;

extern volatile unsigned char ms_delay;
extern volatile unsigned char volume;
extern volatile unsigned char audio_delay,audio_read_flag,audio_run,audio_pause,music_play;
extern unsigned int audio_old_sec;

extern int flac_seek_frame (u8 *buf,u32 size,FLACContext * fc);
extern int flac_decode_frame24 (FLACContext *fc, uint8_t *buf, int buf_size, s32 *wavbuf);
extern int flac_decode_frame16 (FLACContext *fc, uint8_t *buf, int buf_size, s16 *wavbuf);



unsigned char flac_init (FIL *fx,__flacctrl *fctrl,FLACContext *fc)
{
      FLAC_Tag *flactag;
      MD_Block_Head *flacblkh;
      unsigned char *buf;
      unsigned char endofmetadata = 0;
      int blocklength;
      unsigned int br;
      unsigned char res;
      buf = malloc(512);
      if (!buf) return 1;
      f_lseek(fx,0);
      f_read(fx,buf,4,&br);
      flactag = (FLAC_Tag *)buf;
      if (strncmp("fLaC",(char *)flactag->id,4) != 0) {
	 free(buf);
	 return 2;
      }
      while (!endofmetadata) {
	    f_read(fx,buf,4,&br);
            if (br < 4) break;
	    flacblkh = (MD_Block_Head *)buf;
	    endofmetadata = flacblkh->head & 0x80;
	    blocklength = ((unsigned int)flacblkh->size[0] << 16) | ((unsigned short)flacblkh->size[1] << 8) | (flacblkh->size[2]);
            if ((flacblkh->head & 0x7F) == 0) {
	       res = f_read(fx,buf,blocklength,&br);
               if (res != FR_OK) break;
               fc->min_blocksize = ((unsigned short)buf[0] << 8) | buf[1];
               fc->max_blocksize = ((unsigned short)buf[2] << 8) | buf[3];
               fc->min_framesize = ((unsigned int)buf[4] << 16) | ((unsigned short)buf[5] << 8) | buf[6];
               fc->max_framesize = ((unsigned int)buf[7] << 16) | ((unsigned short)buf[8] << 8) | buf[9];
               fc->samplerate = ((unsigned int)buf[10] << 12) | ((unsigned short)buf[11] << 4) | ((buf[12] & 0xF0) >> 4);
               fc->channels = ((buf[12] & 0x0E) >> 1) + 1;
               fc->bps = ((((unsigned short)buf[12] & 0x01) << 4) | ((buf[13] & 0xF0) >> 4)) + 1;
               fc->totalsamples = ((unsigned int)buf[14] << 24) | ((unsigned int)buf[15] << 16) | ((unsigned short)buf[16] << 8) | buf[17];
	       fctrl->samplerate = fc->samplerate;
	       fctrl->totsec = (fc->totalsamples / fc->samplerate);
            } else {
               if (f_lseek(fx,fx->fptr + blocklength) != FR_OK) {
		  free(buf);
		  return 3;
               }
	    }
      }
      free(buf);
      if (fctrl->totsec) {
	 fctrl->outsamples = fc->max_blocksize * 2;
	 fctrl->bps = fc->bps;
	 fctrl->datastart = fx->fptr;
	 fctrl->bitrate = ((fx->fsize - fctrl->datastart) * 8) / fctrl->totsec;
      } else return 4;
      return 0;
}


void flac_i2s_dma_tx_callback (void)
{
      unsigned short i;
      unsigned short size;
      if (DMA1_Stream4->CR & (1 << 19))	{
	 flacwitchbuf = 0;
	 if ((audiodev.status & 0x01) == 0) {
	    if (flacctrl->bps == 24) size = flacctrl->outsamples * 4;else size = flacctrl->outsamples * 2;
	    for (i=0;i<size;i++) audiodev.i2sbuf1[i] = 0;
	 }
      } else {
	 flacwitchbuf = 1;
	 if ((audiodev.status & 0x01) == 0) {
	    if (flacctrl->bps == 24) size = flacctrl->outsamples * 4;else size = flacctrl->outsamples * 2;
	    for (i=0;i<size;i++) audiodev.i2sbuf2[i] = 0;
	 }
      }
      flactransferend = 1;
}


void flac_get_curtime (FIL *fx,__flacctrl *tflacctrl)
{
      long long fpos = 0;
      if (fx->fptr > tflacctrl->datastart) fpos = fx->fptr - tflacctrl->datastart;
      tflacctrl->cursec = fpos * tflacctrl->totsec / (fx->fsize - tflacctrl->datastart);
}


unsigned int flac_file_seek (unsigned int pos)
{
      if (pos > audiodev.file->fsize) {
	 pos = audiodev.file->fsize;
      }
      f_lseek(audiodev.file,pos);
      return audiodev.file->fptr;
}


void flac_stop (void)
{
      audio_stop();
      f_close(audiodev.file);
      free(flac_fc);
      free(flacctrl);
      free(audiodev.file);
      free(audiodev.i2sbuf1);
      free(audiodev.i2sbuf2);
      free(flac_buffer);
      free(flac_decbuf0);
      free(flac_decbuf1);
      audio_run = 0;
      audio_read_flag = 0;
      audio_pause = 0;
      audio_play_function = NULL;
      audio_stop_function = NULL;
      music_play = 1;
}


unsigned char flac_play_song (unsigned char *fname)
{
      unsigned char res = 0;
      unsigned int br = 0;
      audio_old_sec = -1;
      flac_fc = malloc(sizeof(FLACContext));
      flacctrl = malloc(sizeof(__flacctrl));
      audiodev.file = (FIL *)malloc(sizeof(FIL));
      audiodev.file_seek = flac_file_seek;
      if (!flac_fc || !audiodev.file || !flacctrl) {
      	 res = 1;
      } else {
	 memset(flac_fc,0,sizeof(FLACContext));
	 res = f_open(audiodev.file,(char *)fname,FA_READ);
	 if (res == FR_OK) {
	    res = flac_init(audiodev.file,flacctrl,flac_fc);
            if (flac_fc->min_blocksize == flac_fc->max_blocksize && flac_fc->max_blocksize != 0) {
	       if (flac_fc->bps == 24) {
		  audiodev.i2sbuf1 = malloc(flac_fc->max_blocksize * 8);
		  audiodev.i2sbuf2 = malloc(flac_fc->max_blocksize * 8);
	       } else {
		  audiodev.i2sbuf1 = malloc(flac_fc->max_blocksize * 4);
		  audiodev.i2sbuf2 = malloc(flac_fc->max_blocksize * 4);
	       }
	       flac_buffer = malloc(flac_fc->max_framesize);
	       flac_decbuf0 = malloc(flac_fc->max_blocksize * 4);
	       flac_decbuf1 = malloc(flac_fc->max_blocksize * 4);
	    } else res += 1;
	 }
      }
      if (flac_buffer && audiodev.i2sbuf1 && audiodev.i2sbuf2 && flac_decbuf0 && flac_decbuf1 && res == 0) {
      	 lcd_printf(0,20,"File: %s ",fname);
         lcd_printf(0,21,"Blocksize: %d .. %d ", flac_fc->min_blocksize,flac_fc->max_blocksize);
	 lcd_printf(0,22,"Framesize: %d .. %d ",flac_fc->min_framesize,flac_fc->max_framesize);
	 lcd_printf(0,23,"Samplerate: %d Channels: %d ", flac_fc->samplerate,flac_fc->channels);
	 lcd_printf(0,24,"Bits per sample: %d ", flac_fc->bps);
	 lcd_printf(0,25,"Metadata length: %d ", flacctrl->datastart);
	 lcd_printf(0,26,"Total Samples: %lu ",flac_fc->totalsamples);
	 lcd_printf(0,27,"Bitrate: %d kbps ",flacctrl->bitrate);
         lcd_printf(0,28,"Time: %2d:%.2d ",flacctrl->totsec);
         lcd_printf(20,28,"Volume: %2d ",volume);
	 if (flacctrl->bps == 24) {
	    WM8978_I2S_Cfg(2,2);
	    I2S2_Init(I2S_Standard_Phillips,I2S_Mode_MasterTx,I2S_CPOL_Low,I2S_DataFormat_24b);
	    I2S2_TX_DMA_Init(audiodev.i2sbuf1,audiodev.i2sbuf2,flacctrl->outsamples * 2);
	    memset(audiodev.i2sbuf1,0,flac_fc->max_blocksize * 8);
	    memset(audiodev.i2sbuf2,0,flac_fc->max_blocksize * 8);
	 } else	{
	    WM8978_I2S_Cfg(2,0);
	    I2S2_Init(I2S_Standard_Phillips,I2S_Mode_MasterTx,I2S_CPOL_Low,I2S_DataFormat_16bextended);
	    I2S2_TX_DMA_Init(audiodev.i2sbuf1,audiodev.i2sbuf2,flacctrl->outsamples);
	    memset(audiodev.i2sbuf1,0,flac_fc->max_blocksize * 4);
	    memset(audiodev.i2sbuf2,0,flac_fc->max_blocksize * 4);
	 }
	 I2S2_SampleRate_Set(flac_fc->samplerate);
	 i2s_tx_callback = flac_i2s_dma_tx_callback;
	 f_read(audiodev.file,flac_buffer,flac_fc->max_framesize,&br);
	 flac_bytesleft = br;
	 audio_start();
	 //flac_fc->decoded0 = (int *)flac_decbuf0;// POINTER
	 //flac_fc->decoded1 = (int *)flac_decbuf1;// POINTER
	 flac_fc->decoded0 = (void *)flac_decbuf0;// POINTER
	 flac_fc->decoded1 = (void *)flac_decbuf1;// POINTER	 
	 flac_fptr = audiodev.file->fptr;
      } else res = 1;
      if (res == 0) {
         audio_read_flag = 1;
         audio_run = 1;
      } else {
         flac_stop();
      }
      return res;
}


void flac_play (void)
{
      unsigned char *p8 = 0;
      unsigned char *pbuf = 0;
      int consumed = 0;
      unsigned char res = 0;
      unsigned int br = 0;
      int i = 0;
      if (flac_bytesleft) {
	 if (flactransferend) {
	    if ((audio_read_flag) && (audio_pause == 0)) {
	       if ((audiodev.status & (1 << 1)) == 0) audio_read_flag = 0;
	       if (flac_fptr != audiodev.file->fptr) {
		  if (audiodev.file->fptr < flacctrl->datastart) {
	             f_lseek(audiodev.file,flacctrl->datastart);
		  }
		  f_read(audiodev.file,flac_buffer,flac_fc->max_framesize,&br);
		  flac_bytesleft = flac_seek_frame(flac_buffer,br,flac_fc);
		  if (flac_bytesleft >= 0) {
		     f_lseek(audiodev.file,audiodev.file->fptr-flac_fc->max_framesize + flac_bytesleft);
		     f_read(audiodev.file,flac_buffer,flac_fc->max_framesize,&br);
		  } else lcd_printf(0,29,"flac seek error: %d ",flac_bytesleft);
		  flac_bytesleft = br;
	       }
	       flactransferend = 0;
	       if (flacwitchbuf == 0) p8 = audiodev.i2sbuf1;else p8 = audiodev.i2sbuf2;
	       //if (flac_fc->bps == 24) res = flac_decode_frame24(flac_fc,flac_buffer,flac_bytesleft,(s32 *)p8);else res = flac_decode_frame16(flac_fc,flac_buffer,flac_bytesleft,(s16 *)p8);	// POINTER
	       if (flac_fc->bps == 24) res = flac_decode_frame24(flac_fc,flac_buffer,flac_bytesleft,(void *)p8);else res = flac_decode_frame16(flac_fc,flac_buffer,flac_bytesleft,(void *)p8);
	       if (res != 0) {
		  res = 1;
		  flac_stop();
	       }
	       if (flac_fc->bps == 24) {
		  pbuf = p8;
		  for (i=0;i<flac_fc->blocksize*8;) {
		      p8[i++] = pbuf[1];
		      p8[i] = pbuf[2];
		      i += 2;
		      p8[i++] = pbuf[0];
		      pbuf += 4;
		  }
	       }
	       consumed = flac_fc->gb.index / 8;
	       memmove(flac_buffer,&flac_buffer[consumed],flac_bytesleft - consumed);
	       flac_bytesleft -= consumed;
	       res = f_read(audiodev.file,&flac_buffer[flac_bytesleft],flac_fc->max_framesize - flac_bytesleft,&br);
	       if (res) {
	          res = 1;
		  flac_stop();
	       }
	       if (br > 0) {
		  flac_bytesleft += br;
	       }
	       flac_fptr = audiodev.file->fptr;
	       audio_read_flag = 0;
	    }
	    if ((audiodev.status & (1 << 1)) && (audio_read_flag == 0) && (audio_pause == 0)) {
               audio_delay++;
               if (audio_delay >= 100) {
                  audio_delay = 0;
	          flac_get_curtime(audiodev.file,flacctrl);
	          audiodev.totsec = flacctrl->totsec;
	          audiodev.cursec = flacctrl->cursec;
	          audiodev.bitrate = flacctrl->bitrate;
	          audiodev.samplerate = flacctrl->samplerate;
	          audiodev.bps = flacctrl->bps;
                  if (flacctrl->cursec != audio_old_sec) {
                     audio_old_sec = flacctrl->cursec;
		     lcd_printf(0,28,"Time: %2d:%.2d %2d:%.2d ",flacctrl->totsec / 60,flacctrl->totsec % 60,flacctrl->cursec / 60,flacctrl->cursec % 60);
	          }
  	       }
  	       if (audiodev.status & 0x01) audio_read_flag = 1;;
  	    }
  	 }
      } else {
         flac_stop();
      }
}
