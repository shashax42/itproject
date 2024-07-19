/********************************************************************************/
/* mp3play.c                                                                    */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#include "../../hwdefs.h"
#include "../prototype.h"
#include <string.h>
#include "mp3dec.h"


__mp3ctrl *mp3ctrl;
volatile unsigned char mp3transferend = 0;
volatile unsigned char mp3witchbuf = 0;


HMP3Decoder mp3decoder;
MP3FrameInfo mp3frameinfo;
unsigned char *mp3_buffer;
unsigned char *mp3_readptr;
int mp3_offset = 0;
int mp3_outofdata = 0;
int mp3_bytesleft = 0;

extern volatile unsigned char volume,volume_flag;
extern volatile unsigned char audio_delay,audio_decode_flag,audio_read_flag,audio_run,audio_pause,music_play;
extern unsigned int audio_old_sec,audio_br;



void mp3_fill_buffer (unsigned short *buf,unsigned short size,unsigned char nch)
{
      unsigned short i;
      unsigned short *p;
      while ((mp3transferend == 0) && (audio_pause == 0)){
            if (tick) {
               tick = 0;
               tick_process();
            }
      }
      mp3transferend = 0;
      if (mp3witchbuf == 0) {
	 //p = (unsigned short *)audiodev.i2sbuf1;	// POINTER
	 p = (void *)audiodev.i2sbuf1;	 
      } else {
	 //p = (unsigned short *)audiodev.i2sbuf2;	// POINTER
	 p = (void *)audiodev.i2sbuf2;	 
      }
      if (nch == 2) {
         for (i=0;i<size;i++) p[i] = buf[i];
      } else {
         for (i=0;i<size;i++) {
             p[2 * i] = buf[i];
             p[2 * i + 1] = buf[i];
         }
      }
}


void mp3_i2s_dma_tx_callback (void)
{
      unsigned short i;
      if (DMA1_Stream4->CR &(1 << 19)) {
	 mp3witchbuf = 0;
	 if ((audiodev.status & 0x01) == 0) {
	    for (i=0;i<2304*2;i++) audiodev.i2sbuf1[i] = 0;
	 }
      } else {
	 mp3witchbuf = 1;
	 if ((audiodev.status & 0x01) == 0) {
	    for (i=0;i<2304*2;i++)audiodev.i2sbuf2[i] = 0;
	 }
      }
      mp3transferend = 1;
}


unsigned char mp3_id3v1_decode (unsigned char *buf,__mp3ctrl *pctrl)
{
      ID3V1_Tag *id3v1tag;
      id3v1tag = (ID3V1_Tag *)buf;
      if (strncmp("TAG",(char *)id3v1tag->id,3) == 0) {
         if (id3v1tag->title[0]) strncpy((char *)pctrl->title,(char *)id3v1tag->title,30);
         if (id3v1tag->artist[0]) strncpy((char *)pctrl->artist,(char *)id3v1tag->artist,30);
      } else return 1;
      return 0;
}


unsigned char mp3_id3v2_decode (unsigned char *buf,unsigned int size,__mp3ctrl *pctrl)
{
      ID3V2_TagHead *taghead;
      ID3V23_FrameHead *framehead;
      unsigned int t;
      unsigned int tagsize;
      unsigned int frame_size;
      taghead = (ID3V2_TagHead *)buf;
      if (strncmp("ID3",(const char *)taghead->id,3) == 0) {
	 tagsize = ((unsigned int)taghead->size[0] << 21) | ((unsigned int)taghead->size[1] << 14) | ((unsigned short)taghead->size[2] << 7) | taghead->size[3];
	 pctrl->datastart = tagsize;
	 if (tagsize > size) tagsize = size;
	 if (taghead->mversion < 3) {
	    lcd_printf(0,29,"not supported mversion! ");
	    return 1;
	 }
	 t = 10;
	 while (t < tagsize) {
	       framehead = (ID3V23_FrameHead *)(buf + t);
	       frame_size = ((unsigned int)framehead->size[0] << 24) | ((unsigned int)framehead->size[1] << 16) | ((unsigned int)framehead->size[2] << 8) | framehead->size[3];
	       if (strncmp("TT2",(char *)framehead->id,3) == 0 || strncmp("TIT2",(char *)framehead->id,4) == 0) {
	          strncpy((char *)pctrl->title,(char *)(buf + t + sizeof(ID3V23_FrameHead) + 1),AUDIO_MIN(frame_size - 1,MP3_TITSIZE_MAX - 1));
	       }
	       if (strncmp("TP1",(char*)framehead->id,3) == 0 || strncmp("TPE1",(char *)framehead->id,4) == 0) {
	          strncpy((char *)pctrl->artist,(char *)(buf + t + sizeof(ID3V23_FrameHead) + 1),AUDIO_MIN(frame_size - 1,MP3_ARTSIZE_MAX - 1));
	       }
	       t += frame_size + sizeof(ID3V23_FrameHead);
	 }
      } else pctrl->datastart = 0;
      return 0;
}


unsigned char mp3_get_info (unsigned char *pname,__mp3ctrl *pctrl)
{
      HMP3Decoder decoder;
      MP3FrameInfo frame_info;
      MP3_FrameXing *fxing;
      MP3_FrameVBRI *fvbri;
      FIL *fmp3;
      unsigned char *buf;
      unsigned int br;
      unsigned char res;
      int offset = 0;
      unsigned int p;
      short samples_per_frame;
      unsigned int totframes;
      fmp3 = malloc(sizeof(FIL));
      buf = malloc(5 * 1024);
      if (fmp3 && buf) {
	 f_open(fmp3,(const TCHAR *)pname,FA_READ);
	 res = f_read(fmp3,(unsigned char *)buf,5 * 1024,&br);
	 if (res == 0) {
	    mp3_id3v2_decode(buf,br,pctrl);
	    f_lseek(fmp3,fmp3->fsize - 128);
	    f_read(fmp3,(unsigned char *)buf,128,&br);
	    mp3_id3v1_decode(buf,pctrl);
	    decoder = MP3InitDecoder();
	    f_lseek(fmp3,pctrl->datastart);
	    f_read(fmp3,(unsigned char *)buf,5 * 1024,&br);
 	    offset = MP3FindSyncWord(buf,br);
	    if (offset >= 0 && MP3GetNextFrameInfo(decoder,&frame_info,&buf[offset]) == 0) {
	       p = offset + 4 + 32;
	       fvbri = (MP3_FrameVBRI *)(buf + p);
	       if (strncmp("VBRI",(char *)fvbri->id,4) == 0) {
		  if (frame_info.version == MPEG1) samples_per_frame = 1152;else samples_per_frame = 576;
 		  totframes = ((unsigned int)fvbri->frames[0] << 24) | ((unsigned int)fvbri->frames[1] << 16) | ((unsigned short)fvbri->frames[2] << 8)| fvbri->frames[3];
		  pctrl->totsec = totframes * samples_per_frame / frame_info.samprate;
	       } else {
		  if (frame_info.version == MPEG1) {
		     p = frame_info.nChans == 2 ? 32 : 17;
		     samples_per_frame = 1152;
		  } else {
		     p = frame_info.nChans == 2 ? 17 : 9;
		     samples_per_frame = 576;
		  }
		  p += offset + 4;
		  fxing = (MP3_FrameXing *)(buf + p);
		  if (strncmp("Xing",(char *)fxing->id,4) == 0 || strncmp("Info",(char *)fxing->id,4) == 0) {
		     if (fxing->flags[3] & 0x01) {
		        totframes = ((unsigned int)fxing->frames[0] << 24) | ((unsigned int)fxing->frames[1] << 16) | ((unsigned short)fxing->frames[2] << 8) | fxing->frames[3];
			pctrl->totsec = totframes * samples_per_frame / frame_info.samprate;
		     } else {
			pctrl->totsec = fmp3->fsize / (frame_info.bitrate / 8);
		     }
		  } else {
		     pctrl->totsec = fmp3->fsize / (frame_info.bitrate / 8);
		  }
	       }
	       pctrl->bitrate = frame_info.bitrate;
	       mp3ctrl->samplerate = frame_info.samprate;
	       if (frame_info.nChans == 2) mp3ctrl->outsamples = frame_info.outputSamps;else mp3ctrl->outsamples = frame_info.outputSamps * 2;
	    } else res = 0xFE;
	    MP3FreeDecoder(decoder);
         }
	 f_close(fmp3);
      } else res= 0xFF;
      free(fmp3);
      free(buf);
      return res;
}


void mp3_get_curtime (FIL *fx,__mp3ctrl *mp3x)
{
      unsigned int fpos = 0;
      if (fx->fptr>mp3x->datastart) fpos = fx->fptr-mp3x->datastart;
      mp3x->cursec = fpos * mp3x->totsec / (fx->fsize-mp3x->datastart);
}


unsigned int mp3_file_seek (unsigned int pos)
{
      if (pos > audiodev.file->fsize) {
	 pos = audiodev.file->fsize;
      }
      f_lseek(audiodev.file,pos);
      return audiodev.file->fptr;
}


void mp3_stop (void)
{
      audio_stop();
      f_close(audiodev.file);
      MP3FreeDecoder(mp3decoder);
      free(mp3ctrl);
      free(mp3_buffer);
      free(audiodev.file);
      free(audiodev.i2sbuf1);
      free(audiodev.i2sbuf2);
      free(audiodev.tbuf);
      audio_run = 0;
      audio_decode_flag = 0;
      audio_read_flag = 0;
      audio_pause = 0;
      audio_play_function = NULL;
      audio_stop_function = NULL;
      music_play = 1;
}


unsigned char mp3_play_song (unsigned char *fname)
{
      unsigned char res;
      mp3decoder = 0;
      mp3_offset = 0;
      mp3_outofdata = 0;
      mp3_bytesleft = 0;
      audio_br = 0;
      mp3ctrl = malloc(sizeof(__mp3ctrl));
      mp3_buffer = malloc(MP3_FILE_BUF_SZ);
      audiodev.file = (FIL *)malloc(sizeof(FIL));
      audiodev.i2sbuf1 = malloc(2304 * 2);
      audiodev.i2sbuf2 = malloc(2304 * 2);
      audiodev.tbuf = malloc(2304 * 2);
      audiodev.file_seek = mp3_file_seek;
      if (!mp3ctrl || !mp3_buffer || !audiodev.file || !audiodev.i2sbuf1 || !audiodev.i2sbuf2 || !audiodev.tbuf) {
	 free(mp3ctrl);
	 free(mp3_buffer);
	 free(audiodev.file);
	 free(audiodev.i2sbuf1);
	 free(audiodev.i2sbuf2);
	 free(audiodev.tbuf);
	 return 1;
      }
      memset(audiodev.i2sbuf1,0,2304 * 2);
      memset(audiodev.i2sbuf2,0,2304 * 2);
      memset(mp3ctrl,0,sizeof(__mp3ctrl));
      res = mp3_get_info(fname,mp3ctrl);
      if (res == 0) {
      	 lcd_printf(0,20,"File: %s ",fname);
         lcd_printf(0,22,"Title: %s ",mp3ctrl->title);
         lcd_printf(0,24,"Artist: %s ",mp3ctrl->artist);
         lcd_printf(0,26,"Bitrate: %d bps ",mp3ctrl->bitrate);
         lcd_printf(0,27,"Samplerate: %d ", mp3ctrl->samplerate);
         lcd_printf(0,28,"Time: %2d:%.2d ",mp3ctrl->totsec / 60,mp3ctrl->totsec % 60);
         lcd_printf(20,28,"Volume: %2d ",volume);
         WM8978_I2S_Cfg(2,0);
	 I2S2_Init(I2S_Standard_Phillips,I2S_Mode_MasterTx,I2S_CPOL_Low,I2S_DataFormat_16bextended);
	 I2S2_SampleRate_Set(mp3ctrl->samplerate);
	 I2S2_TX_DMA_Init(audiodev.i2sbuf1,audiodev.i2sbuf2,mp3ctrl->outsamples);
	 i2s_tx_callback = mp3_i2s_dma_tx_callback;
	 mp3decoder = MP3InitDecoder();
	 res = f_open(audiodev.file,(char *)fname,FA_READ);
      }
      if (res == 0 && mp3decoder != 0) {
	 f_lseek(audiodev.file,mp3ctrl->datastart);
	 audio_start();
         audio_read_flag = 1;
         audio_run = 1;
      } else {
      	 mp3_stop();
      }
      return 0;
}


void mp3_play (void)
{
     unsigned char res;
      int err = 0;
      if ((audiodev.status & (1 << 1)) && (audio_decode_flag == 0) && (audio_pause == 0)) {
         if (audio_delay >= 100) {
            audio_delay = 0;
	    mp3_get_curtime(audiodev.file,mp3ctrl);
	    audiodev.totsec = mp3ctrl->totsec;
	    audiodev.cursec = mp3ctrl->cursec;
	    audiodev.bitrate = mp3ctrl->bitrate;
	    audiodev.samplerate = mp3ctrl->samplerate;
	    audiodev.bps = 16;
            if (mp3ctrl->cursec != audio_old_sec) {
               audio_old_sec = mp3ctrl->cursec;
               lcd_printf(0,28,"Time: %2d:%.2d %2d:%.2d ",mp3ctrl->totsec / 60,mp3ctrl->totsec % 60,mp3ctrl->cursec / 60,mp3ctrl->cursec % 60);
	       //if (audio_old_sec >= 20) mp3_stop();
            }
         }
         if (audiodev.status & 0x01) audio_decode_flag = 1;
      }
      if ((audio_decode_flag) && (audio_pause == 0)) {
         mp3_offset = MP3FindSyncWord(mp3_readptr,mp3_bytesleft);
	 if ((mp3_offset < 0) || ((audiodev.status & (1 << 1)) == 0)) {
	    mp3_outofdata = 1;
	    audio_read_flag = 1;
	    audio_decode_flag = 0;
	 } else {
	    mp3_readptr += mp3_offset;
	    mp3_bytesleft -= mp3_offset;
	    //err = MP3Decode(mp3decoder,&mp3_readptr,&mp3_bytesleft,(short *)audiodev.tbuf,0);	// POINTER
	    err = MP3Decode(mp3decoder,&mp3_readptr,&mp3_bytesleft,(void *)audiodev.tbuf,0);
	    if (err != 0) {
	       lcd_printf(0,29,"decode error: %d ",err);
	       mp3_stop();
	       audio_decode_flag = 0;
	       audio_read_flag = 0;
	    } else {
	       MP3GetLastFrameInfo(mp3decoder,&mp3frameinfo);
	       if (mp3ctrl->bitrate != (unsigned int)mp3frameinfo.bitrate) {
	          mp3ctrl->bitrate = mp3frameinfo.bitrate;
	       }
	       //mp3_fill_buffer((unsigned short *)audiodev.tbuf,mp3frameinfo.outputSamps,mp3frameinfo.nChans);	// POINTER
	       mp3_fill_buffer((void *)audiodev.tbuf,mp3frameinfo.outputSamps,mp3frameinfo.nChans);
	    }
	    if (mp3_bytesleft < MAINBUF_SIZE * 2) {
	       memmove(mp3_buffer,mp3_readptr,mp3_bytesleft);
	       f_read(audiodev.file,mp3_buffer + mp3_bytesleft,MP3_FILE_BUF_SZ - mp3_bytesleft,&audio_br);
	       if (audio_br < (unsigned int)(MP3_FILE_BUF_SZ - mp3_bytesleft)) {
		  memset(mp3_buffer + mp3_bytesleft + audio_br,0,MP3_FILE_BUF_SZ - mp3_bytesleft - audio_br);
	       }
	       mp3_bytesleft = MP3_FILE_BUF_SZ;
	       mp3_readptr = mp3_buffer;
	    }
	    audio_decode_flag = 0;
	 }
      }
      if ((audio_read_flag) && (audio_pause == 0)) {
	 mp3_readptr = mp3_buffer;
	 mp3_offset = 0;
	 mp3_outofdata = 0;
	 mp3_bytesleft = 0;
	 res = f_read(audiodev.file,mp3_buffer,MP3_FILE_BUF_SZ,&audio_br);
	 mp3_bytesleft += audio_br;
	 if ((res) || (audio_br == 0)) {
            audio_run  = 0;
	    mp3_stop();
	 } else {
	    audio_decode_flag = 1;
	 }
	 audio_read_flag = 0;
      }
}
