/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2013        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control module to the FatFs module with a defined API.        */
/*-----------------------------------------------------------------------*/
#include "../../hwdefs.h" 
#include "../prototype.h" 

#include "diskio.h"		/* FatFs lower layer API */
#include "ff.h"

#define SD_CARD				0
#define EX_FLASH			1
#define USB_DISK			2

#define FLASH_SECTOR_SIZE		512		
#define FLASH_SECTOR_NUM		(2048 * 12)

u16 FLASH_SECTOR_COUNT;			
#define FLASH_BLOCK_SIZE   		8  	


DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber (0..) */
)
{
	u8 res=0;
 	switch(pdrv)
	{
		case SD_CARD:		//SD
			res=SD_Init();	//SD_Init()
  			break;
		case EX_FLASH:		
			//W25QXX_Init();
			//FLASH_SECTOR_COUNT=FLASH_SECTOR_NUM;
 			break;
		case USB_DISK:		
	  		//res=!USBH_UDISK_Status();
			break;
		default:
			res=1;
	}
	if(res)return  STA_NOINIT;
	else return 0; 
}

unsigned char fs_disk_status;

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber (0..) */
)
{
	fs_disk_status = pdrv;
	return 0;
}


DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,		/* Sector address (LBA) */
	UINT count		/* Number of sectors to read (1..128) */
)
{
	u8 res=0;
	u8 tcnt=0;
    if (!count)return RES_PARERR;
	switch(pdrv)
	{
		case SD_CARD:
			res=SD_ReadDisk(buff,sector,count);
			while(res&&tcnt<10)
			{
				tcnt++;
				res=SD_Init();
				res=SD_ReadDisk(buff,sector,count);
			}
			break;
		case EX_FLASH:
			for(;count>0;count--)
			{
				//W25QXX_Read(buff,sector*FLASH_SECTOR_SIZE,FLASH_SECTOR_SIZE);
				//sector++;
				//buff+=FLASH_SECTOR_SIZE;
			}
			res=0;
			break;
		case USB_DISK:
			//res=USBH_UDISK_Read(buff,sector,count);
			break;
		default:
			res=1;
	}
    if(res==0x00)return RES_OK;
    else return RES_ERROR;
}


#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber (0..) */
	BYTE *buff,			/* Data to be written */
	DWORD sector,			/* Sector address (LBA) */
	UINT count			/* Number of sectors to write (1..128) */
)
{
	u8 res=0;
 	u8 tcnt=0;
   if (!count)return RES_PARERR;
 	switch(pdrv)
	{
		case SD_CARD:
			res=SD_WriteDisk((u8 *)buff,sector,count);
			while(res&&tcnt<10)
			{
				tcnt++;
				res=SD_Init();	
				res=SD_WriteDisk((u8 *)buff,sector,count);
			}
			break;
		case EX_FLASH:
			for(;count>0;count--)
			{
				//W25QXX_Write((u8*)buff,sector*FLASH_SECTOR_SIZE,FLASH_SECTOR_SIZE);
				//sector++;
				//buff+=FLASH_SECTOR_SIZE;
			}
			res=0;
			break;
		case USB_DISK:
			//res=USBH_UDISK_Write((u8*)buff,sector,count);
			break;
		default:
			res=1;
	}
    if(res==0x00)return RES_OK;
    else return RES_ERROR;
}
#endif



#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	if(pdrv==SD_CARD)
	{
	    switch(cmd)
	    {
		    case CTRL_SYNC:
				res = RES_OK;
		        break;
		    case GET_SECTOR_SIZE:
				*(DWORD*)buff = 512;
		        res = RES_OK;
		        break;
		    case GET_BLOCK_SIZE:
				*(WORD*)buff = SDCardInfo.CardBlockSize;
		        res = RES_OK;
		        break;
		    case GET_SECTOR_COUNT:
		        *(DWORD*)buff = SDCardInfo.CardCapacity/512;
		        res = RES_OK;
		        break;
		    default:
		        res = RES_PARERR;
		        break;
	    }
	}else if(pdrv==EX_FLASH)	
	{
	    switch(cmd)
	    {
		    case CTRL_SYNC:
				res = RES_OK;
		        break;
		    case GET_SECTOR_SIZE:
		        //*(WORD*)buff = FLASH_SECTOR_SIZE;
		        res = RES_OK;
		        break;
		    case GET_BLOCK_SIZE:
		        //*(WORD*)buff = FLASH_BLOCK_SIZE;
		        res = RES_OK;
		        break;
		    case GET_SECTOR_COUNT:
		        //*(DWORD*)buff = FLASH_SECTOR_COUNT;
		        res = RES_OK;
		        break;
		    default:
		        res = RES_PARERR;
		        break;
	    }
	}else if(pdrv==USB_DISK)
	{
	    switch(cmd)
	    {
		    case CTRL_SYNC:
				res = RES_OK;
		        break;
		    case GET_SECTOR_SIZE:
		        //*(WORD*)buff=512;
		        res = RES_OK;
		        break;
		    case GET_BLOCK_SIZE:
		        //*(WORD*)buff=512;
		        res = RES_OK;
		        break;
		    case GET_SECTOR_COUNT:
		        //*(DWORD*)buff=USBH_MSC_Param.MSCapacity;
		        res = RES_OK;
		        break;
		    default:
		        res = RES_PARERR;
		        break;
	    }
	}else res=RES_ERROR;
    return res;
}
#endif
vu8 cnt0=0;
vu8 cnt1=0;
vu8 fstype=0;

void ff_enter(FATFS *fs)
{
        fstype = fs->fs_type;	
	if(fs->drv!=2)
	{
		cnt0++;
	}else
	{
		cnt1++;
	}
}

void ff_leave(FATFS *fs)
{
        fstype = fs->fs_type;
	if(cnt0)
	{
		cnt0--;
	}
	if(cnt1)
	{
		cnt1--;
	}
}


DWORD get_fattime (void)
{
	u32 time=0;
	RTC_TimeTypeDef RTC_TimeStruct;
	RTC_DateTypeDef RTC_DateStruct;	
        RTC_GetTime(RTC_Format_BIN,&RTC_TimeStruct);
        RTC_GetDate(RTC_Format_BIN, &RTC_DateStruct);
	time=(RTC_DateStruct.RTC_Year)<<25;
	time|=(RTC_DateStruct.RTC_Month)<<21;	
	time|=(RTC_DateStruct.RTC_Date)<<16;	
	time|=(RTC_TimeStruct.RTC_Hours)<<11;		
	time|=(RTC_TimeStruct.RTC_Minutes)<<5;		
	time|=(RTC_TimeStruct.RTC_Seconds/2);		
	return time;
}

void *ff_memalloc (UINT size)
{
	return (void*)malloc(size);
}

void ff_memfree (void* mf)
{
	free(mf);
}




















