/********************************************************************************/
/* prototype.h                                                                  */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#include "fatfs/ff.h"
#include <stdlib.h>

// main.c
extern volatile unsigned short tick,jiffes;
extern void wait_ms (unsigned short delay);
extern char hex2dec (const char ch);
extern char dec2hex (const char ch);
extern void tick_process (void);

// serial.c
extern void serial_init (void);
extern void serial_check (void);

// i2c_app.c
extern void i2c_write (I2C_TypeDef *I2Cx, unsigned char device, unsigned char address, unsigned char buffer);
extern unsigned char eeprom_read (unsigned char address);
extern void eeprom_write (unsigned char address,unsigned char data);
extern void init_i2c_24xx (void);

// key.c
#define KEY0				PE_INPUT(4)
#define KEY1				PE_INPUT(3)
#define KEY2				PE_INPUT(2)
#define KEY_UP				PA_INPUT(0)
#define BEEP				PF_OUTPUT(8)

#define K_UP				0x08
#define K_DOWN				0x02
#define K_RIGHT				0x01
#define K_LEFT				0x04

extern volatile unsigned char key_value,beep_on;

extern void beep_control (unsigned char ctl);
extern unsigned char key_read (void);
extern void key_init (void);

// lcd.c
typedef struct
{
      unsigned short width;
      unsigned short height;
      unsigned short id;
      unsigned char dir;
      unsigned short wramcmd;
      unsigned short setxcmd;
      unsigned short setycmd;
} _lcd_dev;

extern unsigned short foreground_color,background_color,maxx,maxy,max_col,max_row,eng_mode;
extern volatile unsigned char hangul_mode;
extern _lcd_dev lcddev;

extern void delay_us (unsigned short delay);
extern void LCD_DrawPoint (unsigned short x, unsigned short y);
extern void LCD_Fast_DrawPoint (unsigned short x, unsigned short y, unsigned short color);
extern void LCD_SSD_BackLightSet (unsigned char pwm);
extern void LCD_Display_Dir (unsigned char dir);
extern void LCD_Set_Window (unsigned short sx, unsigned short sy, unsigned short width, unsigned short height);
extern void LCD_Clear (unsigned short color);
extern void LCD_Fill (short sx, short sy, short ex, short ey, unsigned short color);
extern void LCD_Color_Fill (short sx, short sy, short ex, short ey, unsigned short *color);
extern void LCD_DrawLine (short x1, short y1, short x2, short y2);
extern void LCD_DrawRectangle (short x1, short y1, short x2, short y2);
extern void LCD_Draw_Circle (short x0, short y0,unsigned short r);
extern void set_color (unsigned short color);
extern void set_background_color (unsigned short color);
extern void set_pixel (unsigned short x, unsigned short y, unsigned short color);
extern unsigned short uni_to_kssm2 (unsigned short wchar);
extern unsigned short uni_to_kssm (unsigned short wchar);
extern unsigned short ks5601_to_kssm (unsigned short wchar);
extern void put_engxy (unsigned short x,unsigned short y,char pdata);
extern void put_hanxy (unsigned short x,unsigned short y,unsigned short pdata);
extern void lcd_printf (unsigned short x,unsigned short y,char *form,...);
extern void LCD_Init (void);

// random_disp.c
extern void random_display (void);

// touch_panel.c
#define CT_MAX_TOUCH			5
#define TP_PRES_DOWN			0x80
#define TP_CATH_PRES			0x40

typedef struct
{
      unsigned short x[CT_MAX_TOUCH];
      unsigned short y[CT_MAX_TOUCH];
      unsigned char sta;
      float xfac;
      float yfac;
      short xoff;
      short yoff;
      unsigned char touchtype;
} _m_tp_dev;

extern _m_tp_dev tp_dev;
extern void TP_Adjust (void);
extern unsigned char TP_Scan (unsigned char tp);
extern unsigned char TP_Init (void);
extern void TP_Drow_Touch_Point (unsigned short x,unsigned short y,unsigned short color);
extern void TP_Drow_Touch_Point2 (unsigned short x,unsigned short y,unsigned short x2,unsigned short y2,unsigned short color);

// ir_remocon.c
extern void ir_remocon_init (void);
extern void ir_process (void);

// touch_pad.c
extern unsigned char TPAD_Init (unsigned char psc);
extern unsigned char TPAD_Scan (unsigned char mode);

// dac1.c
extern void Dac1_Set_Voltage (unsigned short voltage);
extern void Dac1_Init (void);

// adc.c
extern unsigned short convert_voltage (unsigned short adc_data);
extern short cal_temperature (unsigned short adc_data);
extern void ADC_Config (void);
extern void adc_process (void);

// wm8978.c
typedef struct
{
      unsigned char mvol;		//0~63
      unsigned char cfreq[5];	        //0~3,[0]:80,105,135,175,[1]:230,300,385,500,[2]:650,850,1100,1400,[3]:1800,2400,3200,4100,[4]:5300,6900,9000,11700
      unsigned char freqval[5];
      unsigned char d3;
      unsigned char speakersw;
      unsigned char saveflag;
} __attribute__((packed)) _wm8978_obj;

extern _wm8978_obj wm8978set;

extern void wm8978_read_para (_wm8978_obj *wm8978dev);
extern void wm8978_save_para (_wm8978_obj *wm8978dev);
extern void WM8978_MIC_Gain (unsigned char gain);
extern void WM8978_LINEIN_Gain (unsigned char gain);
extern void WM8978_AUX_Gain (unsigned char gain);
extern void WM8978_ADDA_Cfg (unsigned char dacen,unsigned char adcen);
extern void WM8978_Input_Cfg (unsigned char micen,unsigned char lineinen,unsigned char auxen);
extern void WM8978_Output_Cfg (unsigned char dacen,unsigned char bpsen);
extern void WM8978_I2S_Cfg (unsigned char fmt,unsigned char len);
extern void WM8978_HPvol_Set (unsigned char voll,unsigned char volr);
extern void WM8978_SPKvol_Set (unsigned char volx);
extern void WM8978_3D_Set (unsigned char depth);
extern void WM8978_EQ_3D_Dir (unsigned char dir);
extern void WM8978_EQ1_Set (unsigned char cfreq,unsigned char gain);
extern void WM8978_EQ2_Set (unsigned char cfreq,unsigned char gain);
extern void WM8978_EQ3_Set (unsigned char cfreq,unsigned char gain);
extern void WM8978_EQ4_Set (unsigned char cfreq,unsigned char gain);
extern void WM8978_EQ5_Set (unsigned char cfreq,unsigned char gain);
extern unsigned char WM8978_Init (void);

// sdio_sdcard.c
typedef enum
{
      SD_CMD_CRC_FAIL			= (1), /* Command response received (but CRC check failed) */
      SD_DATA_CRC_FAIL			= (2), /* Data bock sent/received (CRC check Failed) */
      SD_CMD_RSP_TIMEOUT		= (3), /* Command response timeout */
      SD_DATA_TIMEOUT			= (4), /* Data time out */
      SD_TX_UNDERRUN			= (5), /* Transmit FIFO under-run */
      SD_RX_OVERRUN			= (6), /* Receive FIFO over-run */
      SD_START_BIT_ERR			= (7), /* Start bit not detected on all data signals in widE bus mode */
      SD_CMD_OUT_OF_RANGE		= (8), /* CMD's argument was out of range.*/
      SD_ADDR_MISALIGNED		= (9), /* Misaligned address */
      SD_BLOCK_LEN_ERR			= (10), /* Transferred block length is not allowed for the card or the number of transferred bytes does not match the block length */
      SD_ERASE_SEQ_ERR			= (11), /* An error in the sequence of erase command occurs.*/
      SD_BAD_ERASE_PARAM		= (12), /* An Invalid selection for erase groups */
      SD_WRITE_PROT_VIOLATION		= (13), /* Attempt to program a write protect block */
      SD_LOCK_UNLOCK_FAILED		= (14), /* Sequence or password error has been detected in unlock command or if there was an attempt to access a locked card */
      SD_COM_CRC_FAILED			= (15), /* CRC check of the previous command failed */
      SD_ILLEGAL_CMD			= (16), /* Command is not legal for the card state */
      SD_CARD_ECC_FAILED		= (17), /* Card internal ECC was applied but failed to correct the data */
      SD_CC_ERROR			= (18), /* Internal card controller error */
      SD_GENERAL_UNKNOWN_ERROR		= (19), /* General or Unknown error */
      SD_STREAM_READ_UNDERRUN		= (20), /* The card could not sustain data transfer in stream read operation. */
      SD_STREAM_WRITE_OVERRUN		= (21), /* The card could not sustain data programming in stream mode */
      SD_CID_CSD_OVERWRITE		= (22), /* CID/CSD overwrite error */
      SD_WP_ERASE_SKIP			= (23), /* only partial address space was erased */
      SD_CARD_ECC_DISABLED		= (24), /* Command has been executed without using internal ECC */
      SD_ERASE_RESET			= (25), /* Erase sequence was cleared before executing because an out of erase sequence command was received */
      SD_AKE_SEQ_ERROR			= (26), /* Error in sequence of authentication. */
      SD_INVALID_VOLTRANGE		= (27),
      SD_ADDR_OUT_OF_RANGE		= (28),
      SD_SWITCH_ERROR			= (29),
      SD_SDIO_DISABLED			= (30),
      SD_SDIO_FUNCTION_BUSY		= (31),
      SD_SDIO_FUNCTION_FAILED		= (32),
      SD_SDIO_UNKNOWN_FUNCTION		= (33),
      SD_INTERNAL_ERROR,
      SD_NOT_CONFIGURED,
      SD_REQUEST_PENDING,
      SD_REQUEST_NOT_APPLICABLE,
      SD_INVALID_PARAMETER,
      SD_UNSUPPORTED_FEATURE,
      SD_UNSUPPORTED_HW,
      SD_ERROR,
      SD_OK = 0
} SD_Error;


typedef struct
{
      unsigned char CSDStruct;			/* CSD structure */
      unsigned char SysSpecVersion;		/* System specification version */
      unsigned char Reserved1;			/* Reserved */
      unsigned char TAAC;			/* Data read access-time 1 */
      unsigned char NSAC;			/* Data read access-time 2 in CLK cycles */
      unsigned char MaxBusClkFrec;		/* Max. bus clock frequency */
      unsigned short CardComdClasses;		/* Card command classes */
      unsigned char RdBlockLen;			/* Max. read data block length */
      unsigned char PartBlockRead;		/* Partial blocks for read allowed */
      unsigned char WrBlockMisalign;		/* Write block misalignment */
      unsigned char RdBlockMisalign;		/* Read block misalignment */
      unsigned char DSRImpl;			/* DSR implemented */
      unsigned char Reserved2;			/* Reserved */
      unsigned int DeviceSize;			/* Device Size */
      unsigned char MaxRdCurrentVDDMin;		/* Max. read current @ VDD min */
      unsigned char MaxRdCurrentVDDMax;		/* Max. read current @ VDD max */
      unsigned char MaxWrCurrentVDDMin;		/* Max. write current @ VDD min */
      unsigned char MaxWrCurrentVDDMax;		/* Max. write current @ VDD max */
      unsigned char DeviceSizeMul;		/* Device size multiplier */
      unsigned char EraseGrSize;		/* Erase group size */
      unsigned char EraseGrMul;			/* Erase group size multiplier */
      unsigned char WrProtectGrSize;		/* Write protect group size */
      unsigned char WrProtectGrEnable;		/* Write protect group enable */
      unsigned char ManDeflECC;			/* Manufacturer default ECC */
      unsigned char WrSpeedFact;		/* Write speed factor */
      unsigned char MaxWrBlockLen;		/* Max. write data block length */
      unsigned char WriteBlockPaPartial;	/* Partial blocks for write allowed */
      unsigned char Reserved3;			/* Reserded */
      unsigned char ContentProtectAppli;	/* Content protection application */
      unsigned char FileFormatGrouop;		/* File format group */
      unsigned char CopyFlag;			/* Copy flag (OTP) */
      unsigned char PermWrProtect;		/* Permanent write protection */
      unsigned char TempWrProtect;		/* Temporary write protection */
      unsigned char FileFormat;			/* File Format */
      unsigned char ECC;			/* ECC code */
      unsigned char CSD_CRC;			/* CSD CRC */
      unsigned char Reserved4;			/* always 1*/
} SD_CSD;


typedef struct
{
      unsigned char ManufacturerID;		/* ManufacturerID */
      unsigned short OEM_AppliID;		/* OEM/Application ID */
      unsigned int ProdName1;			/* Product Name part1 */
      unsigned char ProdName2;			/* Product Name part2*/
      unsigned char ProdRev;			/* Product Revision */
      unsigned int ProdSN;			/* Product Serial Number */
      unsigned char Reserved1;			/* Reserved1 */
      unsigned short ManufactDate;		/* Manufacturing Date */
      unsigned char CID_CRC;			/* CID CRC */
      unsigned char Reserved2;			/* always 1 */
} SD_CID;


typedef struct
{
      SD_CSD SD_csd;
      SD_CID SD_cid;
      long long CardCapacity;
      unsigned int CardBlockSize;
      unsigned short RCA;
      unsigned char CardType;
} SD_CardInfo;



typedef enum
{
      SD_CARD_READY			= ((unsigned int)0x00000001),
      SD_CARD_IDENTIFICATION		= ((unsigned int)0x00000002),
      SD_CARD_STANDBY			= ((unsigned int)0x00000003),
      SD_CARD_TRANSFER			= ((unsigned int)0x00000004),
      SD_CARD_SENDING			= ((unsigned int)0x00000005),
      SD_CARD_RECEIVING			= ((unsigned int)0x00000006),
      SD_CARD_PROGRAMMING		= ((unsigned int)0x00000007),
      SD_CARD_DISCONNECTED		= ((unsigned int)0x00000008),
      SD_CARD_ERROR			= ((unsigned int)0x000000FF)
} SDCardState;


extern SD_CardInfo SDCardInfo;

extern SD_Error SD_Init (void);
extern void SDIO_Clock_Set (unsigned char clkdiv);
extern SD_Error SD_PowerON (void);
extern SD_Error SD_PowerOFF (void);
extern SD_Error SD_InitializeCards (void);
extern SD_Error SD_GetCardInfo (SD_CardInfo *cardinfo);
extern SD_Error SD_EnableWideBusOperation(unsigned int wmode);
extern SD_Error SD_SetDeviceMode (unsigned int mode);
extern SD_Error SD_SelectDeselect (unsigned int addr);
extern SD_Error SD_SendStatus (unsigned int *pcardstatus);
extern SDCardState SD_GetState (void);
extern SD_Error SD_ReadBlock (unsigned char *buf,long long addr,unsigned short blksize);
extern SD_Error SD_ReadMultiBlocks (unsigned char *buf,long long  addr,unsigned short blksize,unsigned int nblks);
extern SD_Error SD_WriteBlock (unsigned char *buf,long long addr,  unsigned short blksize);
extern SD_Error SD_WriteMultiBlocks (unsigned char *buf,long long addr,unsigned short blksize,unsigned int nblks);
extern SD_Error SD_ProcessIRQSrc (void);
extern void SD_DMA_Config(unsigned int *mbuf,unsigned int bufsize,unsigned int dir);
extern unsigned char SD_ReadDisk(unsigned char *buf,unsigned int sector,unsigned char cnt);
extern unsigned char SD_WriteDisk(unsigned char *buf,unsigned int sector,unsigned char cnt);

// i2s.c
extern void (*i2s_tx_callback)(void);
extern void (*i2s_rx_callback)(void);

extern void I2S2_Init (unsigned short I2S_Standard,unsigned short I2S_Mode,unsigned short I2S_Clock_Polarity,unsigned short I2S_DataFormat);
extern void I2S2ext_Init (unsigned short I2S_Standard,unsigned short I2S_Mode,unsigned short I2S_Clock_Polarity,unsigned short I2S_DataFormat);
extern unsigned char I2S2_SampleRate_Set (unsigned int samplerate);
extern void I2S2_TX_DMA_Init (unsigned char *buf0,unsigned char *buf1,unsigned short num);
extern void I2S2ext_RX_DMA_Init (unsigned char *buf0,unsigned char *buf1,unsigned short num);
extern void I2S_Play_Start (void);
extern void I2S_Rec_Start (void);
extern void I2S_Play_Stop (void);
extern void I2S_Rec_Stop (void);

// ext_sram.c
extern void FSMC_SRAM_Init (void);
extern void FSMC_SRAM_WriteBuffer (unsigned char *pBuffer,unsigned int WriteAddr,unsigned int NumHalfwordToWrite);
extern void FSMC_SRAM_ReadBuffer (unsigned char *pBuffer,unsigned int ReadAddr,unsigned int NumHalfwordToRead);
extern void fsmc_sram_test_write (unsigned int addr,unsigned char data);
extern unsigned char fsmc_sram_test_read (unsigned int addr);

// rtc.c
extern ErrorStatus RTC_Set_Time (unsigned char hour,unsigned char min,unsigned char sec,unsigned char ampm);
extern ErrorStatus RTC_Set_Date (unsigned char year,unsigned char month,unsigned char date,unsigned char week);
extern void RTC_Get_Time (unsigned char *hour,unsigned char *min,unsigned char *sec,unsigned char *ampm);
extern void RTC_Get_Date (unsigned char *year,unsigned char *month,unsigned char *date,unsigned char *week);
extern unsigned char rtc_init (void);
extern void RTC_Set_AlarmA (unsigned char week,unsigned char hour,unsigned char min,unsigned char sec);
extern void RTC_Set_WakeUp (unsigned int wksel,unsigned short cnt);
extern unsigned char RTC_Get_Week (unsigned short year,unsigned char month,unsigned char day);

// audio.c
#define T_BIN				0x00
#define T_LRC				0x10
#define T_NES				0x20
#define T_TEXT				0x30
#define T_C				0x31
#define T_H				0x32
#define T_WAV				0x40
#define T_MP3				0x41
#define T_APE				0x42
#define T_FLAC				0x43
#define T_BMP				0x50
#define T_JPG				0x51
#define T_JPEG				0x52
#define T_GIF				0x53
#define T_AVI				0x60


typedef struct
{
      unsigned char *i2sbuf1;
      unsigned char *i2sbuf2;
      unsigned char *tbuf;
      FIL *file;
      unsigned int (*file_seek)(unsigned int);
      volatile unsigned char status;
      unsigned char mode;
      unsigned char *path;
      unsigned char *name;
      unsigned short namelen;
      unsigned short curnamepos;
      unsigned int totsec;
      unsigned int cursec;
      unsigned int bitrate;
      unsigned int samplerate;
      unsigned short bps;
      unsigned short curindex;
      unsigned short mfilenum;
      unsigned short *mfindextbl;
} __attribute__((packed)) __audiodev;


//MP3
#define MP3_TITSIZE_MAX			40
#define MP3_ARTSIZE_MAX			40
#define MP3_FILE_BUF_SZ			(5 * 1024)


typedef struct
{
      unsigned char id[3];
      unsigned char title[30];
      unsigned char artist[30];
      unsigned char year[4];
      unsigned char comment[30];
      unsigned char genre;
} __attribute__((packed)) ID3V1_Tag;


typedef struct
{
      unsigned char id[3];
      unsigned char mversion;
      unsigned char sversion;
      unsigned char flags;
      unsigned char size[4];
} __attribute__((packed)) ID3V2_TagHead;


typedef struct
{
      unsigned char id[4];
      unsigned char size[4];
      unsigned short flags;
} __attribute__((packed)) ID3V23_FrameHead;


typedef struct
{
      unsigned char id[4];
      unsigned char flags[4];
      unsigned char frames[4];
      unsigned char fsize[4];
} __attribute__((packed)) MP3_FrameXing;


typedef struct
{
      unsigned char id[4];
      unsigned char version[2];
      unsigned char delay[2];
      unsigned char quality[2];
      unsigned char fsize[4];
      unsigned char frames[4];
} __attribute__((packed)) MP3_FrameVBRI;


typedef struct
{
      unsigned char title[MP3_TITSIZE_MAX];
      unsigned char artist[MP3_ARTSIZE_MAX];
      unsigned int totsec;
      unsigned int cursec;
      unsigned int bitrate;
      unsigned int samplerate;
      unsigned short outsamples;
      unsigned int datastart;
} __attribute__((packed)) __mp3ctrl;


//FLAC
enum decorrelation_type {
      INDEPENDENT,
      LEFT_SIDE,
      RIGHT_SIDE,
      MID_SIDE,
};


typedef struct GetBitContext {
      const uint8_t *buffer, *buffer_end;
      int index;
      int size_in_bits;
} GetBitContext;


typedef struct FLACContext
{
      GetBitContext gb;
      int min_blocksize, max_blocksize;
      int min_framesize, max_framesize;
      int samplerate, channels;
      int blocksize;
      int bps, curr_bps;
      unsigned long samplenumber;
      unsigned long totalsamples;
      enum decorrelation_type decorrelation;
      int seektable;
      int seekpoints;
      int bitstream_size;
      int bitstream_index;
      int sample_skip;
      int framesize;
      int *decoded0;  // channel 0
      int *decoded1;  // channel 1
} FLACContext;


typedef struct
{
      unsigned char id[3];
} __attribute__((packed)) FLAC_Tag;


typedef struct
{
      unsigned char head;
      unsigned char size[3];
} __attribute__((packed)) MD_Block_Head;


typedef struct
{
      unsigned int totsec;
      unsigned int cursec;
      unsigned int bitrate;
      unsigned int samplerate;
      unsigned short outsamples;
      unsigned short bps;
      unsigned int datastart;
} __attribute__((packed)) __flacctrl;


//WAV
#define WAV_I2S_TX_DMA_BUFSIZE		15360


typedef struct
{
      unsigned int ChunkID;
      unsigned int ChunkSize;
      unsigned int Format;
} __attribute__((packed)) ChunkRIFF;


typedef struct
{
      unsigned int ChunkID;
      unsigned int ChunkSize ;
      unsigned short AudioFormat;
      unsigned short NumOfChannels;
      unsigned int SampleRate;
      unsigned int ByteRate;
      unsigned short BlockAlign;
      unsigned short BitsPerSample;
} __attribute__((packed)) ChunkFMT;


typedef struct
{
      unsigned int ChunkID;
      unsigned int ChunkSize ;
      unsigned int NumOfSamples;
} __attribute__((packed)) ChunkFACT;


typedef struct
{
      unsigned int ChunkID;
      unsigned int ChunkSize ;
} __attribute__((packed)) ChunkLIST;


typedef struct
{
      unsigned int ChunkID;
      unsigned int ChunkSize ;
} __attribute__((packed)) ChunkDATA;


typedef struct
{
      ChunkRIFF riff;
      ChunkFMT fmt;
      ChunkDATA data;
} __attribute__((packed)) __WaveHeader;


typedef struct
{
      unsigned short audioformat;
      unsigned short nchannels;
      unsigned short blockalign;
      unsigned int datasize;
      unsigned int totsec;
      unsigned int cursec;
      unsigned int bitrate;
      unsigned int samplerate;
      unsigned short bps;
      unsigned int datastart;
} __attribute__((packed)) __wavctrl;


// ape
typedef int32_t filter_int;
#define APE_FILE_BUF_SZ			(20 * 1024)
#define APE_BLOCKS_PER_LOOP		(2 * 1024)
#define APE_MIN_VERSION			3970
#define APE_MAX_VERSION			3990
#define PREDICTOR_HISTORY_SIZE		512
#define PREDICTOR_SIZE			50


struct predictor_t
{
      /* Filter histories */
      int32_t* buf;
      int32_t YlastA;
      int32_t XlastA;
      /* NOTE: The order of the next four fields is important for predictor-arm.S */
      int32_t YfilterB;
      int32_t XfilterA;
      int32_t XfilterB;
      int32_t YfilterA;
      /* Adaption co-efficients */
      int32_t YcoeffsA[4];
      int32_t XcoeffsA[4];
      int32_t YcoeffsB[5];
      int32_t XcoeffsB[5];
      int32_t historybuffer[PREDICTOR_HISTORY_SIZE + PREDICTOR_SIZE];
};


struct ape_ctx_t
{
      /* Derived fields */
      uint32_t      junklength;
      uint32_t      firstframe;
      uint32_t      totalsamples;
      /* Info from Descriptor Block */
      char          magic[4];
      int16_t       fileversion;
      int16_t       padding1;
      uint32_t      descriptorlength;
      uint32_t      headerlength;
      uint32_t      seektablelength;
      uint32_t      wavheaderlength;
      uint32_t      audiodatalength;
      uint32_t      audiodatalength_high;
      uint32_t      wavtaillength;
      uint8_t       md5[16];
      /* Info from Header Block */
      uint16_t      compressiontype;
      uint16_t      formatflags;
      uint32_t      blocksperframe;
      uint32_t      finalframeblocks;
      uint32_t      totalframes;
      uint16_t      bps;
      uint16_t      channels;
      uint32_t      samplerate;
      /* Seektable */
      uint32_t*     seektable;        /* Seektable buffer */
      uint32_t      maxseekpoints;    /* Max seekpoints we can store (size of seektable buffer) */
      uint32_t      numseekpoints;    /* Number of seekpoints */
      int           seektablefilepos; /* Location in .ape file of seektable */
      /* Decoder state */
      uint32_t      acrc;
      int           frameflags;
      int           currentframeblocks;
      int           blocksdecoded;
      struct predictor_t predictor;
};

typedef struct
{
      unsigned int totsec;
      unsigned int cursec;
      unsigned int bitrate;
      unsigned int samplerate;
      unsigned short outsamples;
      unsigned short bps;
      unsigned int datastart;
} __attribute__((packed)) __apectrl;

extern __apectrl *apectrl;
extern __mp3ctrl *mp3ctrl;
extern __audiodev audiodev;
extern __flacctrl *flacctrl;

#define AUDIO_MIN(x,y)			((x) < (y) ? (x) : (y))

extern void (*audio_play_function)(void);
extern void (*audio_stop_function)(void);

extern void wm8978_volset (unsigned char vol);
extern void audio_init (void);
extern void audio_start (void);
extern void audio_stop (void);
extern void audio_process (void);
extern unsigned char check_ir_key (void);
extern void audio_file_list (void);

// mp3.c
extern void mp3_i2s_dma_tx_callback (void);
extern void mp3_fill_buffer (unsigned short *buf,unsigned short size,unsigned char nch);
extern unsigned char mp3_id3v1_decode (unsigned char *buf,__mp3ctrl *pctrl);
extern unsigned char mp3_id3v2_decode (unsigned char *buf,unsigned int size,__mp3ctrl *pctrl);
extern unsigned char mp3_get_info (unsigned char *pname,__mp3ctrl *pctrl);
extern unsigned char mp3_play_song (unsigned char *fname);
extern void mp3_play (void);
extern void mp3_stop (void);

// wavplay
extern unsigned char wav_decode_init (unsigned char *fname,__wavctrl *wavx);
extern unsigned int wav_buffill (unsigned char *buf,u16 size,unsigned char bits);
extern void wav_i2s_dma_tx_callback (void);
extern unsigned char wav_play_song (unsigned char *fname);
extern void wave_stop (void);
extern void wave_play (void);

// flacplay.c
extern unsigned char flac_init (FIL *fx,__flacctrl *fctrl,FLACContext *fc);
extern void flac_i2s_dma_tx_callback (void);
extern void flac_get_curtime (FIL *fx,__flacctrl *flacx);
extern unsigned char flac_play_song (unsigned char *fname);
extern void flac_stop (void);
extern void flac_play (void);

// apeplay.c
extern void ape_fill_buffer (unsigned short *buf,unsigned short size);
extern void ape_i2s_dma_tx_callback (void);
extern void ape_get_curtime (FIL *fx,__apectrl *apectrl);
extern unsigned char ape_play_song (unsigned char *fname);
extern void ape_play (void);
extern void ape_stop (void);

