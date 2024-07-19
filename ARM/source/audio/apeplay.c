/********************************************************************************/
/* apeplay.c                                                                    */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#include "../../hwdefs.h"
#include "../prototype.h"
#include <string.h>


extern volatile unsigned char ms_delay;
extern volatile unsigned char volume;
__apectrl *apectrl;
extern filter_int *filterbuf64;
volatile unsigned char apetransferend = 0;
volatile unsigned char apewitchbuf = 0;

struct ape_ctx_t *apex;
int ape_nblocks;
unsigned long int ape_currentframe;
int ape_bytesconsumed;
int ape_bytesinbuffer;
int ape_firstbyte;
unsigned char *ape_readptr;
unsigned char *ape_buffer;
long int *ape_decoded0;
long int *ape_decoded1;
unsigned char ape_res;

extern volatile unsigned char audio_delay,audio_decode_flag,audio_read_flag,audio_run,audio_pause,music_play;
extern unsigned int audio_old_sec,audio_br;

extern int ape_parseheader (FIL *fx, struct ape_ctx_t *ape_ctx);
extern void init_frame_decoder (struct ape_ctx_t *ape_ctx,unsigned char *inbuffer,int *firstbyte,int *bytesconsumed);
extern int decode_chunk (struct ape_ctx_t *ape_ctx,unsigned char *inbuffer,int *firstbyte,int *bytesconsumed,int32_t *decoded0,int32_t *decoded1,int count);



void ape_i2s_dma_tx_callback (void)
{
      unsigned short i;
      if (DMA1_Stream4->CR & (1 << 19)) {
	 apewitchbuf = 0;
	 if ((audiodev.status & 0x01) == 0) {
	    for (i=0;i<APE_BLOCKS_PER_LOOP*4;i++) audiodev.i2sbuf1[i] = 0;
	 }
      } else {
	 apewitchbuf = 1;
	 if ((audiodev.status & 0x01) == 0) {
	    for (i=0;i<APE_BLOCKS_PER_LOOP*4;i++) audiodev.i2sbuf2[i] = 0;
	 }
      }
      apetransferend = 1;
}


void ape_fill_buffer (unsigned short *buf,unsigned short size)
{
      unsigned short i;
      unsigned short *p;
      while (apetransferend == 0) {
            if (tick) {
               tick = 0;
               tick_process();
            }
      }
      apetransferend = 0;
      if (apewitchbuf == 0) {
	 //p = (unsigned short *)audiodev.i2sbuf1;	// POINTER
	 p = (void *)audiodev.i2sbuf1;
      } else {
	 //p = (unsigned short *)audiodev.i2sbuf2;	// POINTER
	 p = (void *)audiodev.i2sbuf2;
      }
      for (i=0;i<size;i++) p[i] = buf[i];
}


void ape_get_curtime (FIL *fx,__apectrl *apectrlx)
{
      long long fpos = 0;
      if (fx->fptr > apectrlx->datastart) fpos = fx->fptr - apectrlx->datastart;
      apectrlx->cursec = fpos * apectrlx->totsec / (fx->fsize - apectrlx->datastart);
}


unsigned int ape_file_seek (unsigned int pos)
{
      UNUSED(pos);
      return audiodev.file->fptr;
}


void ape_stop (void)
{
      audio_stop();
      f_close(audiodev.file);
      free(filterbuf64);
      free(apectrl);
      free(apex->seektable);
      free(apex);
      free(ape_decoded0);
      free(ape_decoded1);
      free(audiodev.file);
      free(audiodev.i2sbuf1);
      free(audiodev.i2sbuf2);
      free(ape_buffer);
      audio_run = 0;
      audio_decode_flag = 0;
      audio_read_flag = 0;
      audio_pause = 0;
      audio_play_function = NULL;
      audio_stop_function = NULL;
      music_play = 1;
}


unsigned char ape_play_song (unsigned char *fname)
{
      unsigned int totalsamples;
      ape_res = 0;
      filterbuf64 = malloc(2816);
      apectrl = malloc(sizeof(__apectrl));
      apex = malloc(sizeof(struct ape_ctx_t));
      ape_decoded0 = malloc(APE_BLOCKS_PER_LOOP * 4);
      ape_decoded1 = malloc(APE_BLOCKS_PER_LOOP * 4);
      audiodev.file = (FIL*)malloc(sizeof(FIL));
      audiodev.i2sbuf1 = malloc(APE_BLOCKS_PER_LOOP * 4);
      audiodev.i2sbuf2 = malloc(APE_BLOCKS_PER_LOOP * 4);
      audiodev.file_seek = ape_file_seek;
      ape_buffer = malloc(APE_FILE_BUF_SZ);
      if (filterbuf64 && apectrl && apex && ape_decoded0 && ape_decoded1 && audiodev.file && audiodev.i2sbuf1 && audiodev.i2sbuf2 && ape_buffer) {
	 memset(apex,0,sizeof(struct ape_ctx_t));
	 memset(apectrl,0,sizeof(__apectrl));
	 memset(audiodev.i2sbuf1,0,APE_BLOCKS_PER_LOOP * 4);
	 memset(audiodev.i2sbuf2,0,APE_BLOCKS_PER_LOOP * 4);
	 f_open(audiodev.file,(char *)fname,FA_READ);
	 ape_res = ape_parseheader(audiodev.file,apex);
	 if (ape_res == 0) {
	    if ((apex->compressiontype > 3000) || (apex->fileversion < APE_MIN_VERSION) || (apex->fileversion > APE_MAX_VERSION /*|| apex->bps != 16*/)) {
	       ape_res = 1;
	    } else {
	       apectrl->bps = apex->bps;
	       apectrl->samplerate = apex->samplerate;
	       if (apex->totalframes > 1) totalsamples = apex->finalframeblocks + apex->blocksperframe * (apex->totalframes - 1);else totalsamples = apex->finalframeblocks;
	       apectrl->totsec = totalsamples / apectrl->samplerate;
	       apectrl->bitrate = (audiodev.file->fsize - apex->firstframe) * 8 / apectrl->totsec;
	       apectrl->outsamples = APE_BLOCKS_PER_LOOP * 2;
	       apectrl->datastart = apex->firstframe;
	    }
	 }
      }
      if (ape_res == 0) {
	 lcd_printf(0,20,"File: %s ",fname);
	 lcd_printf(0,22,"Samplerate: %d ", apectrl->samplerate);
	 lcd_printf(0,23,"Bits per sample: %d ",apectrl->bps);
	 lcd_printf(0,24,"First frame pos: %d ",apectrl->datastart);
	 lcd_printf(0,25,"Duration: %d ",apectrl->totsec);
	 lcd_printf(0,26,"Bitrate: %d kbps ",apectrl->bitrate);
         lcd_printf(0,28,"Time: %2d:%.2d ",apectrl->totsec / 60,apectrl->totsec % 60);
         lcd_printf(20,28,"Volume: %2d ",volume);
 	 WM8978_I2S_Cfg(2,0);
	 I2S2_Init(I2S_Standard_Phillips,I2S_Mode_MasterTx,I2S_CPOL_Low,I2S_DataFormat_16bextended);
	 I2S2_SampleRate_Set(apex->samplerate);
	 I2S2_TX_DMA_Init(audiodev.i2sbuf1,audiodev.i2sbuf2,APE_BLOCKS_PER_LOOP * 2);
	 i2s_tx_callback = ape_i2s_dma_tx_callback;
         ape_currentframe = 0;
	 f_lseek(audiodev.file,apex->firstframe);
	 ape_res = f_read(audiodev.file,ape_buffer,APE_FILE_BUF_SZ,(unsigned int *)&ape_bytesinbuffer);
	 ape_firstbyte = 3;
	 ape_readptr = ape_buffer;
	 audio_start();
         audio_read_flag = 1;
         audio_run = 1;
      } else {
      	 ape_stop();
      }
      return 0;
}


void ape_play (void)
{
      int blockstodecode;
      int ape_n;
      if (apetransferend == 0) return;
      if ((audio_read_flag == 1) && (audio_pause == 0)) {
         if ((ape_currentframe < apex->totalframes) && (ape_res == 0)) {
            if (ape_currentframe == (apex->totalframes - 1)) ape_nblocks = apex->finalframeblocks;else ape_nblocks = apex->blocksperframe;
            apex->currentframeblocks = ape_nblocks;
            init_frame_decoder(apex,ape_readptr,&ape_firstbyte,&ape_bytesconsumed);
            ape_readptr += ape_bytesconsumed;
            ape_bytesinbuffer -= ape_bytesconsumed;
            ape_currentframe++;
            audio_read_flag = 0;
            audio_decode_flag = 1;
         } else {
            ape_stop();
         }
      }
      if ((audio_decode_flag) && (audio_pause == 0)) {
         if (ape_nblocks > 0) {
            if (audio_decode_flag == 1) {
               blockstodecode = AUDIO_MIN(APE_BLOCKS_PER_LOOP,ape_nblocks);
               ape_res = decode_chunk(apex,ape_readptr,&ape_firstbyte,&ape_bytesconsumed,ape_decoded0,ape_decoded1,blockstodecode);
               if (ape_res != 0) {
      	          lcd_printf(0,29,"frame decode err ");
      	          ape_res = 1;
                  audio_read_flag = 1;
                  audio_decode_flag = 0;
               }
               ape_fill_buffer((unsigned short *)ape_decoded1,APE_BLOCKS_PER_LOOP * 2);
               ape_readptr += ape_bytesconsumed;
               ape_bytesinbuffer -= ape_bytesconsumed;
               if (ape_bytesconsumed > 4 * APE_BLOCKS_PER_LOOP) {
      	          ape_nblocks = 0;
      	          ape_res = 1;
      	          lcd_printf(0,29,"ape_bytesconsumed: %d ",ape_bytesconsumed);
               }
               if (ape_bytesinbuffer < 4 * APE_BLOCKS_PER_LOOP) {
      	          memmove(ape_buffer,ape_readptr,ape_bytesinbuffer);
      	          ape_res = f_read(audiodev.file,ape_buffer + ape_bytesinbuffer,APE_FILE_BUF_SZ - ape_bytesinbuffer,(unsigned int *)&ape_n);
      	          if (ape_res) {
      	             ape_res = 1;
                     audio_read_flag = 1;
                     audio_decode_flag = 0;
      	          }
      	          ape_bytesinbuffer += ape_n;
      	          ape_readptr = ape_buffer;
               }
               ape_nblocks -= blockstodecode;
               audio_decode_flag = 2;
            }
            if (audio_decode_flag == 2) {
               if ((audiodev.status & (1 << 1)) && (audio_pause == 0)) {
                  if (audio_delay >= 100) {
                     audio_delay = 0;
      	             ape_get_curtime(audiodev.file,apectrl);
      	             audiodev.totsec = apectrl->totsec;
      	             audiodev.cursec = apectrl->cursec;
      	             audiodev.bitrate = apectrl->bitrate;
      	             audiodev.samplerate = apectrl->samplerate;
      	             audiodev.bps = apectrl->bps;
      	             lcd_printf(0,28,"Time: %2d:%.2d %2d:%.2d ",apectrl->totsec / 60,apectrl->totsec % 60,apectrl->cursec / 60,apectrl->cursec % 60);
      	          }
      	          if (audiodev.status & 0x01) {
      	             audio_decode_flag = 1;
      	          }
               }
            }
            if (((audiodev.status & (1 << 1)) == 0) && (audio_pause == 0)) {
      	       ape_nblocks = 0;
      	       ape_res = 0;
               audio_read_flag = 1;
               audio_decode_flag = 0;
            }
         } else {
            audio_read_flag = 1;
            audio_decode_flag = 0;
         }
      }
}

