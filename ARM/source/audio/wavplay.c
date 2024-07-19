/********************************************************************************/
/* wavplay.c                                                                    */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#include "../../hwdefs.h"
#include "../prototype.h"
#include <string.h>


__wavctrl wavctrl;
volatile unsigned char wavtransferend = 0;
volatile unsigned char wavwitchbuf = 0;
unsigned int fillnum;

extern volatile unsigned char ms_delay;
extern volatile unsigned char volume;
extern volatile unsigned char audio_delay,audio_read_flag,audio_run,audio_pause,music_play;
extern unsigned int audio_old_sec;



unsigned char wav_decode_init (unsigned char *fname,__wavctrl *wavx)
{
      FIL *ftemp;
      unsigned char *buf;
      unsigned int br = 0;
      unsigned char res = 0;
      ChunkRIFF *riff;
      ChunkFMT *fmt;
      ChunkFACT *fact;
      ChunkDATA *data;
      ftemp = (FIL *)malloc(sizeof(FIL));
      buf = malloc(512);
      if (ftemp && buf) {
	 res = f_open(ftemp,(TCHAR *)fname,FA_READ);
	 if (res == FR_OK) {
	    f_read(ftemp,buf,512,&br);
	    riff = (ChunkRIFF *)buf;
	    if (riff->Format == 0x45564157) {
	       fmt = (ChunkFMT *)(buf + 12);
	       fact = (ChunkFACT *)(buf + 12 + 8 + fmt->ChunkSize);
	       if (fact->ChunkID == 0x74636166 || fact->ChunkID == 0x5453494C) wavx->datastart = 12 + 8 + fmt->ChunkSize + 8 + fact->ChunkSize;else wavx->datastart = 12 + 8 + fmt->ChunkSize;
	       data = (ChunkDATA *)(buf + wavx->datastart);
	       if (data->ChunkID == 0x61746164) {
		  wavx->audioformat = fmt->AudioFormat;
		  wavx->nchannels = fmt->NumOfChannels;
		  wavx->samplerate = fmt->SampleRate;
		  wavx->bitrate = fmt->ByteRate * 8;
		  wavx->blockalign = fmt->BlockAlign;
		  wavx->bps = fmt->BitsPerSample;
		  wavx->datasize = data->ChunkSize;
		  wavx->datastart = wavx->datastart + 8;
      	          lcd_printf(0,20,"File: %s ",fname);
		  lcd_printf(0,21,"Audioformat: %d Nchannels: %d",wavx->audioformat,wavx->nchannels);
		  lcd_printf(0,22,"Samplerate: %d ",wavx->samplerate);
		  lcd_printf(0,23,"Bitrate: %d ",wavx->bitrate);
		  lcd_printf(0,24,"Blockalign: %d ",wavx->blockalign);
		  lcd_printf(0,25,"Bps: %d ",wavx->bps);
		  lcd_printf(0,26,"Datasize: %d ",wavx->datasize);
		  lcd_printf(0,27,"Datastart: %d ",wavx->datastart);
                  lcd_printf(0,28,"Time: %2d:%.2d ",wavx->datasize / (wavx->bitrate / 8));
                  lcd_printf(20,28,"Volume: %2d ",volume);
               } else res = 3;
	    } else res = 2;
	 } else res = 1;
      }
      f_close(ftemp);
      free(ftemp);
      free(buf);
      return 0;
}


unsigned int wav_buffill (unsigned char *buf,unsigned short size,unsigned char bits)
{
      unsigned short readlen = 0;
      unsigned int bread;
      unsigned short i;
      unsigned char *p;
      if (bits == 24) {
	 readlen = (size / 4) * 3;
	 f_read(audiodev.file,audiodev.tbuf,readlen,(UINT *)&bread);
	 p = audiodev.tbuf;
	 for (i=0;i<size;) {
	     buf[i++] = p[1];
	     buf[i] = p[2];
	     i += 2;
	     buf[i++] = p[0];
	     p += 3;
	 }
	 bread = (bread * 4) / 3;
      } else {
	 f_read(audiodev.file,buf,size,(UINT *)&bread);
	 if (bread < size) {
	    for (i=bread;i<size-bread;i++)buf[i] = 0;
	 }
      }
      return bread;
}


void wav_i2s_dma_tx_callback (void)
{
      unsigned short i;
      if (DMA1_Stream4->CR & (1 << 19)) {
	 wavwitchbuf = 0;
	 if ((audiodev.status & 0x01) == 0) {
	    for (i=0;i<WAV_I2S_TX_DMA_BUFSIZE;i++) {
		audiodev.i2sbuf1[i] = 0;
	    }
	 }
      } else {
	 wavwitchbuf = 1;
	 if ((audiodev.status & 0x01) == 0) {
	    for (i=0;i<WAV_I2S_TX_DMA_BUFSIZE;i++) {
		audiodev.i2sbuf2[i] = 0;
	    }
	 }
      }
      wavtransferend = 1;
}


void wav_get_curtime (FIL *fx,__wavctrl *wavx)
{
      long long fpos;
      wavx->totsec = wavx->datasize / (wavx->bitrate / 8);
      fpos = fx->fptr-wavx->datastart;
      wavx->cursec = fpos * wavx->totsec / wavx->datasize;
}


unsigned int wav_file_seek (unsigned int pos)
{
      unsigned char temp;
      if (pos > audiodev.file->fsize) {
	 pos = audiodev.file->fsize;
      }
      if (pos < wavctrl.datastart) pos = wavctrl.datastart;
      if (wavctrl.bps == 16) temp = 8;
      if (wavctrl.bps == 24) temp = 12;
      if ((pos - wavctrl.datastart) % temp) {
	 pos += temp - (pos - wavctrl.datastart) % temp;
      }
      f_lseek(audiodev.file,pos);
      return audiodev.file->fptr;
}


void wave_stop (void)
{
      audio_stop();
      free(audiodev.tbuf);
      free(audiodev.i2sbuf1);
      free(audiodev.i2sbuf2);
      free(audiodev.file);
      audio_run = 0;
      audio_read_flag = 0;
      audio_pause = 0;
      audio_play_function = NULL;
      audio_stop_function = NULL;
      music_play = 1;
}


unsigned char wav_play_song (unsigned char *fname)
{
      unsigned char res;
      audio_old_sec = -1;
      audiodev.file = (FIL *)malloc(sizeof(FIL));
      audiodev.i2sbuf1 = malloc(WAV_I2S_TX_DMA_BUFSIZE);
      audiodev.i2sbuf2 = malloc(WAV_I2S_TX_DMA_BUFSIZE);
      audiodev.tbuf = malloc(WAV_I2S_TX_DMA_BUFSIZE);
      audiodev.file_seek = wav_file_seek;
      if (audiodev.file && audiodev.i2sbuf1 && audiodev.i2sbuf2 && audiodev.tbuf) {
	 res = wav_decode_init(fname,&wavctrl);
	 if (res == 0) {
	    if (wavctrl.bps == 16) {
	       WM8978_I2S_Cfg(2,0);
	       I2S2_Init(I2S_Standard_Phillips,I2S_Mode_MasterTx,I2S_CPOL_Low,I2S_DataFormat_16bextended);
	    } else if (wavctrl.bps == 24) {
	       WM8978_I2S_Cfg(2,2);
	       I2S2_Init(I2S_Standard_Phillips,I2S_Mode_MasterTx,I2S_CPOL_Low,I2S_DataFormat_24b);
	    }
	    I2S2_SampleRate_Set(wavctrl.samplerate);
	    I2S2_TX_DMA_Init(audiodev.i2sbuf1,audiodev.i2sbuf2,WAV_I2S_TX_DMA_BUFSIZE/2);
	    i2s_tx_callback = wav_i2s_dma_tx_callback;
	    audio_stop();
	    res = f_open(audiodev.file,(TCHAR *)fname,FA_READ);
	    if (res == 0) {
	       f_lseek(audiodev.file, wavctrl.datastart);
	       fillnum = wav_buffill(audiodev.i2sbuf1,WAV_I2S_TX_DMA_BUFSIZE,wavctrl.bps);
	       fillnum = wav_buffill(audiodev.i2sbuf2,WAV_I2S_TX_DMA_BUFSIZE,wavctrl.bps);
	       audio_start();
	    } else res = 1;
	 } else res = 1;
      } else res = 1;
      if (res == 0) {
         audio_read_flag = 1;
         audio_run = 1;
      } else {
      	 wave_stop();
      }
      return res;
}


void wave_play (void)
{
      if ((audio_read_flag) && (audio_pause == 0)) {
         if (wavtransferend) {
	    wavtransferend = 0;
 	    if (wavwitchbuf) fillnum = wav_buffill(audiodev.i2sbuf2,WAV_I2S_TX_DMA_BUFSIZE,wavctrl.bps);else fillnum = wav_buffill(audiodev.i2sbuf1,WAV_I2S_TX_DMA_BUFSIZE,wavctrl.bps);
 	    audio_read_flag = 0;
 	 }
      }
      if ((audiodev.status & (1 << 1)) == 0 || (fillnum != WAV_I2S_TX_DMA_BUFSIZE)) {
      	 if (audio_pause == 0) {
	    wave_stop();
	 }
      }
      if ((audiodev.status & (1 << 1)) && (audio_read_flag == 0) && (audio_pause == 0)) {
         audio_delay++;
         if (audio_delay >= 100) {
            audio_delay = 0;
	    wav_get_curtime(audiodev.file,&wavctrl);
	    audiodev.totsec = wavctrl.totsec;
	    audiodev.cursec = wavctrl.cursec;
	    audiodev.bitrate = wavctrl.bitrate;
	    audiodev.samplerate = wavctrl.samplerate;
	    audiodev.bps = wavctrl.bps;
            if (wavctrl.cursec != audio_old_sec) {
               audio_old_sec = wavctrl.cursec;
               lcd_printf(0,28,"Time: %2d:%.2d %2d:%.2d ",wavctrl.totsec / 60,wavctrl.totsec % 60,wavctrl.cursec / 60,wavctrl.cursec % 60);
            }
         }
         if (audiodev.status & 0x01) audio_read_flag = 1;
      }
}

