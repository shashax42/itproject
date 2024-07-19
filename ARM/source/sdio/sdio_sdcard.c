/********************************************************************************/
/* sdio_sdcard.c                                                                */
/* STM32F407ZGT6                                                                */
/* (Lee ChangWoo HL2IRW  hl2irw@daum.net 010-8573-6860)                 	*/
/* stm32f4x_test								*/
/********************************************************************************/
#include "../../hwdefs.h"
#include "../prototype.h"
#include "sdio_sdcard.h"
#include <string.h>


SDIO_InitTypeDef SDIO_InitStructure;
SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
SDIO_DataInitTypeDef SDIO_DataInitStructure;




static unsigned char CardType = SDIO_STD_CAPACITY_SD_CARD_V1_1;
static unsigned int CSD_Tab[4],CID_Tab[4],RCA = 0;
static unsigned char DeviceMode = SD_DMA_MODE;
static unsigned char StopCondition = 0;
volatile SD_Error TransferError = SD_OK;
volatile unsigned char TransferEnd = 0;
SD_CardInfo SDCardInfo;
static unsigned int save_buff_size,save_rca;

__attribute__((aligned(4))) unsigned char SDIO_DATA_BUFFER [512];
__attribute__((aligned(4))) unsigned int *m_tempbuff;




SD_Error CmdError (void)
{
      SD_Error errorstatus = SD_OK;
      unsigned int timeout = SDIO_CMD0TIMEOUT;
      while (timeout--) {
	    if (SDIO_GetFlagStatus(SDIO_FLAG_CMDSENT) != RESET) break;
      }
      if (timeout == 0) return SD_CMD_RSP_TIMEOUT;
      SDIO_ClearFlag(SDIO_STATIC_FLAGS);
      return errorstatus;
}


SD_Error CmdResp1Error (unsigned char cmd)
{
      unsigned int status;
      while (1)	{
	    status = SDIO->STA;
	    if (status & ((1 << 0) | (1 << 2) | (1 << 6))) break;
      }
      if (SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET) {
 	 SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
	 return SD_CMD_RSP_TIMEOUT;
      }
      if (SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET) {
 	 SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
	 return SD_CMD_CRC_FAIL;
      }
      if (SDIO->RESPCMD != cmd) return SD_ILLEGAL_CMD;
      SDIO_ClearFlag(SDIO_STATIC_FLAGS);
      return (SD_Error)(SDIO->RESP1&SD_OCR_ERRORBITS);
}


SD_Error CmdResp2Error (void)
{
      SD_Error errorstatus = SD_OK;
      unsigned int status = 0;
      unsigned int timeout = SDIO_CMD0TIMEOUT;
      while (timeout--)	{
	    status = SDIO->STA;
	    if (status & ((1 << 0) | (1 << 2) | (1 << 6))) break;
      }
      if ((timeout == 0) || (status & (1 << 2))) {
	 errorstatus = SD_CMD_RSP_TIMEOUT;
	 SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
	 return errorstatus;
      }
      if (SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET) {
	 errorstatus = SD_CMD_CRC_FAIL;
	 SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
      }
      SDIO_ClearFlag(SDIO_STATIC_FLAGS);
      return errorstatus;
}


SD_Error CmdResp3Error (void)
{
      unsigned int status;
      while (1) {
	    status = SDIO->STA;
	    if (status & ((1 << 0) | (1 << 2) | (1 << 6))) break;
      }
      if (SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET) {
	 SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
	 return SD_CMD_RSP_TIMEOUT;
      }
      SDIO_ClearFlag(SDIO_STATIC_FLAGS);
      return SD_OK;
}


SD_Error CmdResp6Error (unsigned char cmd,unsigned short *prca)
{
      SD_Error errorstatus = SD_OK;
      unsigned int status;
      unsigned int rspr1;
      while (1) {
      	    status = SDIO->STA;
      	    if (status & ((1 << 0) | (1 << 2) | (1 << 6))) break;
      }
      if (SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET) {
      	 SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
      	 return SD_CMD_RSP_TIMEOUT;
      }
      if (SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET) {
      	 SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
      	 return SD_CMD_CRC_FAIL;
      }
      if (SDIO->RESPCMD != cmd) {
      	 return SD_ILLEGAL_CMD;
      }
      SDIO_ClearFlag(SDIO_STATIC_FLAGS);
      rspr1 = SDIO->RESP1;
      if (SD_ALLZERO == (rspr1 & (SD_R6_GENERAL_UNKNOWN_ERROR | SD_R6_ILLEGAL_CMD | SD_R6_COM_CRC_FAILED))) {
      	 *prca= (unsigned short)(rspr1 >> 16);
      	 return errorstatus;
      }
      if (rspr1 & SD_R6_GENERAL_UNKNOWN_ERROR) return SD_GENERAL_UNKNOWN_ERROR;
      if (rspr1 & SD_R6_ILLEGAL_CMD) return SD_ILLEGAL_CMD;
      if (rspr1 & SD_R6_COM_CRC_FAILED) return SD_COM_CRC_FAILED;
      return errorstatus;
}


SD_Error CmdResp7Error (void)
{
      SD_Error errorstatus = SD_OK;
      unsigned int status = 0;
      unsigned int timeout = SDIO_CMD0TIMEOUT;
      while (timeout--) {
	    status = SDIO->STA;
	    if (status & ((1 << 0) | (1 << 2) | (1 << 6)) )break;
      }
      if ((timeout == 0) || (status & (1 << 2))) {
	 errorstatus = SD_CMD_RSP_TIMEOUT;
	 SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
	 return errorstatus;
      }
      if (status & 1 << 6) {
	 errorstatus = SD_OK;
	 SDIO_ClearFlag(SDIO_FLAG_CMDREND);
      }
      return errorstatus;
}


unsigned char convert_from_bytes_to_power_of_two (unsigned short NumberOfBytes)
{
      unsigned char count = 0;
      while (NumberOfBytes != 1) {
	    NumberOfBytes >>= 1;
	    count++;
      }
      return count;
}


SD_Error FindSCR (unsigned short rca,unsigned int *pscr)
{
      unsigned int index = 0;
      save_rca = rca;
      SD_Error errorstatus = SD_OK;
      unsigned int tempscr[2] = {0,0};
      SDIO_CmdInitStructure.SDIO_Argument = (unsigned int)8;
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
      errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);
      if (errorstatus != SD_OK) return errorstatus;
      SDIO_CmdInitStructure.SDIO_Argument = (unsigned int)RCA << 16;
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
      errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
      if (errorstatus != SD_OK) return errorstatus;
      SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
      SDIO_DataInitStructure.SDIO_DataLength = 8;
      SDIO_DataInitStructure.SDIO_DataBlockSize = SDIO_DataBlockSize_8b;
      SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
      SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
      SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
      SDIO_DataConfig(&SDIO_DataInitStructure);
      SDIO_CmdInitStructure.SDIO_Argument = 0x0;
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SD_APP_SEND_SCR;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
      errorstatus = CmdResp1Error(SD_CMD_SD_APP_SEND_SCR);
      if (errorstatus != SD_OK) return errorstatus;
      while (!(SDIO->STA & (SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DBCKEND | SDIO_FLAG_STBITERR))) {
	    if (SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET) {
	       *(tempscr + index) = SDIO->FIFO;
	       index++;
	       if (index >= 2) break;
            }
      }
      if (SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET) {
	 SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
	 return SD_DATA_TIMEOUT;
      } else if (SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET) {
	 SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
	 return SD_DATA_CRC_FAIL;
      } else if (SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET) {
	 SDIO_ClearFlag(SDIO_FLAG_RXOVERR);
	 return SD_RX_OVERRUN;
      } else if (SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET) {
	 SDIO_ClearFlag(SDIO_FLAG_STBITERR);
	 return SD_START_BIT_ERR;
      }
      SDIO_ClearFlag(SDIO_STATIC_FLAGS);
      *(pscr + 1) = ((tempscr[0] & SD_0TO7BITS) << 24) | ((tempscr[0] & SD_8TO15BITS) << 8) | ((tempscr[0] & SD_16TO23BITS) >> 8) | ((tempscr[0] & SD_24TO31BITS) >> 24);
      *(pscr) = ((tempscr[1] & SD_0TO7BITS) << 24) | ((tempscr[1] & SD_8TO15BITS) << 8) | ((tempscr[1] & SD_16TO23BITS) >> 8) | ((tempscr[1] & SD_24TO31BITS) >> 24);
      return errorstatus;
}


SD_Error IsCardProgramming (unsigned char *pstatus)
{
      volatile unsigned int respR1 = 0,status = 0;
      SDIO_CmdInitStructure.SDIO_Argument = (unsigned int)RCA << 16;
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
      status = SDIO->STA;
      while (!(status & ((1 << 0) | (1 << 6) | (1 << 2)))) status = SDIO->STA;
      if (SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET) {
	 SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
	 return SD_CMD_CRC_FAIL;
      }
      if (SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET) {
	 SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
	 return SD_CMD_RSP_TIMEOUT;
      }
      if (SDIO->RESPCMD != SD_CMD_SEND_STATUS) return SD_ILLEGAL_CMD;
      SDIO_ClearFlag(SDIO_STATIC_FLAGS);
      respR1 = SDIO->RESP1;
      *pstatus = (unsigned char)((respR1 >> 9) & 0x0000000F);
      return SD_OK;
}


SD_Error SDEnWideBus (unsigned char enx)
{
      SD_Error errorstatus = SD_OK;
      unsigned int scr[2] = {0,0};
      unsigned char arg = 0x00;
      if (enx) arg = 0x02;else arg = 0x00;
      if (SDIO->RESP1 & SD_CARD_LOCKED) return SD_LOCK_UNLOCK_FAILED;
      errorstatus = FindSCR(RCA,scr);
      if (errorstatus != SD_OK) return errorstatus;
      if ((scr[1] & SD_WIDE_BUS_SUPPORT) != SD_ALLZERO)	{
	 SDIO_CmdInitStructure.SDIO_Argument = (unsigned int)RCA << 16;
         SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
         SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
         SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
         SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
         SDIO_SendCommand(&SDIO_CmdInitStructure);
	 errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
	 if (errorstatus != SD_OK) return errorstatus;
	 SDIO_CmdInitStructure.SDIO_Argument = arg;
         SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_SD_SET_BUSWIDTH;
         SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
         SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
         SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
         SDIO_SendCommand(&SDIO_CmdInitStructure);
         errorstatus = CmdResp1Error(SD_CMD_APP_SD_SET_BUSWIDTH);
	 return errorstatus;
      } else return SD_REQUEST_NOT_APPLICABLE;
}


void SDIO_Register_Deinit (void)
{
      SDIO->POWER = 0x00000000;
      SDIO->CLKCR = 0x00000000;
      SDIO->ARG = 0x00000000;
      SDIO->CMD = 0x00000000;
      SDIO->DTIMER = 0x00000000;
      SDIO->DLEN = 0x00000000;
      SDIO->DCTRL = 0x00000000;
      SDIO->ICR = 0x00C007FF;
      SDIO->MASK = 0x00000000;
}


void SDIO_Clock_Set (unsigned char clkdiv)
{
      unsigned int tmpreg = SDIO->CLKCR;
      tmpreg &= 0xFFFFFF00;
      tmpreg |= clkdiv;
      SDIO->CLKCR = tmpreg;
}


SD_Error SD_PowerON (void)
{
      unsigned char i = 0;
      SD_Error errorstatus = SD_OK;
      unsigned int response = 0,count = 0,validvoltage = 0;
      unsigned int SDType = SD_STD_CAPACITY;
      SDIO_InitStructure.SDIO_ClockDiv = SDIO_INIT_CLK_DIV;
      SDIO_InitStructure.SDIO_ClockEdge = SDIO_ClockEdge_Rising;
      SDIO_InitStructure.SDIO_ClockBypass = SDIO_ClockBypass_Disable;
      SDIO_InitStructure.SDIO_ClockPowerSave = SDIO_ClockPowerSave_Disable;
      SDIO_InitStructure.SDIO_BusWide = SDIO_BusWide_1b;
      SDIO_InitStructure.SDIO_HardwareFlowControl = SDIO_HardwareFlowControl_Disable;
      SDIO_Init(&SDIO_InitStructure);
      SDIO_SetPowerState(SDIO_PowerState_ON);
      SDIO->CLKCR |= 1 << 8;
      for (i=0;i<74;i++) {
	  SDIO_CmdInitStructure.SDIO_Argument = 0x0;
          SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_GO_IDLE_STATE;
          SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_No;
          SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
          SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
          SDIO_SendCommand(&SDIO_CmdInitStructure);
	  errorstatus = CmdError();
	  if (errorstatus == SD_OK) break;
      }
      if (errorstatus) return errorstatus;
      SDIO_CmdInitStructure.SDIO_Argument = SD_CHECK_PATTERN;
      SDIO_CmdInitStructure.SDIO_CmdIndex = SDIO_SEND_IF_COND;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
      errorstatus = CmdResp7Error();
      if (errorstatus == SD_OK) {
	 CardType = SDIO_STD_CAPACITY_SD_CARD_V2_0;
	 SDType = SD_HIGH_CAPACITY;
      }
      SDIO_CmdInitStructure.SDIO_Argument = 0x00;
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
      errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
      if (errorstatus == SD_OK) {
	 while ((!validvoltage) && (count< SD_MAX_VOLT_TRIAL)) {
	       SDIO_CmdInitStructure.SDIO_Argument = 0x00;
               SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
               SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
               SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
               SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
               SDIO_SendCommand(&SDIO_CmdInitStructure);
	       errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
 	       if (errorstatus != SD_OK) return errorstatus;
               SDIO_CmdInitStructure.SDIO_Argument = SD_VOLTAGE_WINDOW_SD | SDType;
               SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SD_APP_OP_COND;
               SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
               SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
               SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
               SDIO_SendCommand(&SDIO_CmdInitStructure);
	       errorstatus = CmdResp3Error();
 	       if (errorstatus != SD_OK) return errorstatus;
	       response = SDIO->RESP1;
	       validvoltage = (((response >> 31) == 1) ? 1 : 0);
	       count++;
	 }
	 if (count >= SD_MAX_VOLT_TRIAL) {
	    errorstatus = SD_INVALID_VOLTRANGE;
	    return errorstatus;
	 }
	 if (response &= SD_HIGH_CAPACITY) {
	    CardType = SDIO_HIGH_CAPACITY_SD_CARD;
	 }
      } else {
	 while ((!validvoltage) && (count < SD_MAX_VOLT_TRIAL))	{
	       SDIO_CmdInitStructure.SDIO_Argument = SD_VOLTAGE_WINDOW_MMC;
               SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_OP_COND;
               SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
               SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
               SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
               SDIO_SendCommand(&SDIO_CmdInitStructure);
	       errorstatus = CmdResp3Error();
 	       if (errorstatus != SD_OK) return errorstatus;
	       response = SDIO->RESP1;
	       validvoltage = (((response >> 31) == 1) ? 1 : 0);
	       count++;
	 }
	 if (count >= SD_MAX_VOLT_TRIAL) {
	    errorstatus = SD_INVALID_VOLTRANGE;
	    return errorstatus;
	 }
	 CardType = SDIO_MULTIMEDIA_CARD;
      }
      return(errorstatus);
}


SD_Error SD_PowerOFF (void)
{
      SDIO_SetPowerState(SDIO_PowerState_OFF);
      return SD_OK;
}


SD_Error SD_InitializeCards (void)
{
      SD_Error errorstatus = SD_OK;
      unsigned short rca = 0x01;
      if (SDIO_GetPowerState() == SDIO_PowerState_OFF) {
         errorstatus = SD_REQUEST_NOT_APPLICABLE;
         return(errorstatus);
      }
      if (SDIO_SECURE_DIGITAL_IO_CARD != CardType) {
	 SDIO_CmdInitStructure.SDIO_Argument = 0x0;
         SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_ALL_SEND_CID;
         SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Long;
         SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
         SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
         SDIO_SendCommand(&SDIO_CmdInitStructure);
	 errorstatus = CmdResp2Error();
	 if (errorstatus != SD_OK) return errorstatus;
 	 CID_Tab[0] = SDIO->RESP1;
	 CID_Tab[1] = SDIO->RESP2;
	 CID_Tab[2] = SDIO->RESP3;
	 CID_Tab[3] = SDIO->RESP4;
      }
      if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == CardType) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == CardType) || (SDIO_SECURE_DIGITAL_IO_COMBO_CARD == CardType) || (SDIO_HIGH_CAPACITY_SD_CARD == CardType)) {
	 SDIO_CmdInitStructure.SDIO_Argument = 0x00;
         SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_REL_ADDR;
         SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
         SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
         SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
         SDIO_SendCommand(&SDIO_CmdInitStructure);
	 errorstatus = CmdResp6Error(SD_CMD_SET_REL_ADDR,&rca);
	 if (errorstatus != SD_OK) return errorstatus;
      }
      if (SDIO_MULTIMEDIA_CARD == CardType) {
	 SDIO_CmdInitStructure.SDIO_Argument = (unsigned int)(rca << 16);
         SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_REL_ADDR;
         SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
         SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
         SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
         SDIO_SendCommand(&SDIO_CmdInitStructure);
         errorstatus = CmdResp2Error();
	 if (errorstatus != SD_OK) return errorstatus;
      }
      if (SDIO_SECURE_DIGITAL_IO_CARD != CardType) {
	 RCA = rca;
         SDIO_CmdInitStructure.SDIO_Argument = (unsigned int)(rca << 16);
         SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_CSD;
         SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Long;
         SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
         SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
         SDIO_SendCommand(&SDIO_CmdInitStructure);
	 errorstatus = CmdResp2Error();
 	 if (errorstatus != SD_OK) return errorstatus;
	 CSD_Tab[0] = SDIO->RESP1;
	 CSD_Tab[1] = SDIO->RESP2;
	 CSD_Tab[2] = SDIO->RESP3;
	 CSD_Tab[3] = SDIO->RESP4;
      }
      return SD_OK;
}


SD_Error SD_GetCardInfo (SD_CardInfo *cardinfo)
{
      SD_Error errorstatus = SD_OK;
      unsigned char tmp = 0;
      cardinfo->CardType = (unsigned char)CardType;
      cardinfo->RCA = (unsigned short)RCA;
      tmp = (unsigned char)((CSD_Tab[0] & 0xFF000000) >> 24);
      cardinfo->SD_csd.CSDStruct = (tmp & 0xC0) >> 6;
      cardinfo->SD_csd.SysSpecVersion = (tmp & 0x3C) >> 2;
      cardinfo->SD_csd.Reserved1 = tmp & 0x03;
      tmp = (unsigned char)((CSD_Tab[0] & 0x00FF0000) >> 16);
      cardinfo->SD_csd.TAAC = tmp;
      tmp = (unsigned char)((CSD_Tab[0] & 0x0000FF00) >> 8);
      cardinfo->SD_csd.NSAC = tmp;
      tmp = (unsigned char)(CSD_Tab[0] & 0x000000FF);
      cardinfo->SD_csd.MaxBusClkFrec = tmp;
      tmp = (unsigned char)((CSD_Tab[1] & 0xFF000000) >> 24);
      cardinfo->SD_csd.CardComdClasses = tmp << 4;
      tmp = (unsigned char)((CSD_Tab[1] & 0x00FF0000) >> 16);
      cardinfo->SD_csd.CardComdClasses|= (tmp & 0xF0) >> 4;
      cardinfo->SD_csd.RdBlockLen  = tmp & 0x0F;
      tmp = (unsigned char)((CSD_Tab[1] & 0x0000FF00) >> 8);
      cardinfo->SD_csd.PartBlockRead = (tmp & 0x80) >> 7;
      cardinfo->SD_csd.WrBlockMisalign = (tmp & 0x40) >> 6;
      cardinfo->SD_csd.RdBlockMisalign = (tmp & 0x20) >> 5;
      cardinfo->SD_csd.DSRImpl = (tmp & 0x10) >> 4;
      cardinfo->SD_csd.Reserved2 = 0;
      if ((CardType == SDIO_STD_CAPACITY_SD_CARD_V1_1) || (CardType == SDIO_STD_CAPACITY_SD_CARD_V2_0) || (SDIO_MULTIMEDIA_CARD == CardType)) {
      	 cardinfo->SD_csd.DeviceSize = (tmp & 0x03) << 10;
       	 tmp = (unsigned char)(CSD_Tab[1] & 0x000000FF);
      	 cardinfo->SD_csd.DeviceSize|= (tmp) << 2;
      	 tmp = (unsigned char)((CSD_Tab[2] & 0xFF000000) >> 24);
      	 cardinfo->SD_csd.DeviceSize|= (tmp & 0xC0) >> 6;
      	 cardinfo->SD_csd.MaxRdCurrentVDDMin = (tmp & 0x38) >> 3;
      	 cardinfo->SD_csd.MaxRdCurrentVDDMax= (tmp & 0x07);
      	 tmp = (unsigned char)((CSD_Tab[2] & 0x00FF0000) >> 16);
      	 cardinfo->SD_csd.MaxWrCurrentVDDMin = (tmp & 0xE0) >> 5;
      	 cardinfo->SD_csd.MaxWrCurrentVDDMax= (tmp & 0x1C) >> 2;
      	 cardinfo->SD_csd.DeviceSizeMul= (tmp & 0x03) << 1;
      	 tmp = (unsigned char)((CSD_Tab[2] & 0x0000FF00) >> 8);
      	 cardinfo->SD_csd.DeviceSizeMul|= (tmp & 0x80) >> 7;
      	 cardinfo->CardCapacity = (cardinfo->SD_csd.DeviceSize + 1);
      	 cardinfo->CardCapacity *= (1 <<(cardinfo->SD_csd.DeviceSizeMul + 2));
      	 cardinfo->CardBlockSize = 1 <<(cardinfo->SD_csd.RdBlockLen);
      	 cardinfo->CardCapacity *= cardinfo->CardBlockSize;
      } else if (CardType == SDIO_HIGH_CAPACITY_SD_CARD) {
      	 tmp = (unsigned char)(CSD_Tab[1] & 0x000000FF);
      	 cardinfo->SD_csd.DeviceSize = (tmp & 0x3F) << 16;
      	 tmp = (unsigned char)((CSD_Tab[2] & 0xFF000000) >> 24);
      	 cardinfo->SD_csd.DeviceSize|= (tmp << 8);
      	 tmp = (unsigned char)((CSD_Tab[2] & 0x00FF0000) >> 16);
      	 cardinfo->SD_csd.DeviceSize|= (tmp);
      	 tmp = (unsigned char)((CSD_Tab[2] & 0x0000FF00) >> 8);
      	 cardinfo->CardCapacity = (long long)(cardinfo->SD_csd.DeviceSize + 1) * 512 * 1024;
      	 cardinfo->CardBlockSize = 512;
      }
      cardinfo->SD_csd.EraseGrSize = (tmp & 0x40) >> 6;
      cardinfo->SD_csd.EraseGrMul= (tmp & 0x3F) << 1;
      tmp = (unsigned char)(CSD_Tab[2] & 0x000000FF);
      cardinfo->SD_csd.EraseGrMul|= (tmp & 0x80) >> 7;
      cardinfo->SD_csd.WrProtectGrSize = (tmp & 0x7F);
      tmp = (unsigned char)((CSD_Tab[3] & 0xFF000000) >> 24);
      cardinfo->SD_csd.WrProtectGrEnable = (tmp & 0x80) >> 7;
      cardinfo->SD_csd.ManDeflECC= (tmp & 0x60) >> 5;
      cardinfo->SD_csd.WrSpeedFact = (tmp & 0x1C) >> 2;
      cardinfo->SD_csd.MaxWrBlockLen = (tmp & 0x03) << 2;
      tmp = (unsigned char)((CSD_Tab[3] & 0x00FF0000) >> 16);
      cardinfo->SD_csd.MaxWrBlockLen|= (tmp & 0xC0) >> 6;
      cardinfo->SD_csd.WriteBlockPaPartial= (tmp & 0x20) >> 5;
      cardinfo->SD_csd.Reserved3= 0;
      cardinfo->SD_csd.ContentProtectAppli= (tmp & 0x01);
      tmp = (unsigned char)((CSD_Tab[3] & 0x0000FF00) >> 8);
      cardinfo->SD_csd.FileFormatGrouop = (tmp & 0x80) >> 7;
      cardinfo->SD_csd.CopyFlag= (tmp & 0x40) >> 6;
      cardinfo->SD_csd.PermWrProtect = (tmp & 0x20) >> 5;
      cardinfo->SD_csd.TempWrProtect = (tmp & 0x10) >> 4;
      cardinfo->SD_csd.FileFormat = (tmp & 0x0C) >> 2;
      cardinfo->SD_csd.ECC= (tmp & 0x03);
      tmp = (unsigned char)(CSD_Tab[3] & 0x000000FF);
      cardinfo->SD_csd.CSD_CRC= (tmp & 0xFE) >> 1;
      cardinfo->SD_csd.Reserved4 = 1;
      tmp = (unsigned char)((CID_Tab[0] & 0xFF000000) >> 24);
      cardinfo->SD_cid.ManufacturerID = tmp;
      tmp = (unsigned char)((CID_Tab[0] & 0x00FF0000) >> 16);
      cardinfo->SD_cid.OEM_AppliID = tmp << 8;
      tmp = (unsigned char)((CID_Tab[0] & 0x000000FF00) >> 8);
      cardinfo->SD_cid.OEM_AppliID |= tmp;
      tmp = (unsigned char)(CID_Tab[0] & 0x000000FF);
      cardinfo->SD_cid.ProdName1 = tmp << 24;
      tmp = (unsigned char)((CID_Tab[1] & 0xFF000000) >> 24);
      cardinfo->SD_cid.ProdName1 |= tmp << 16;
      tmp = (unsigned char)((CID_Tab[1] & 0x00FF0000) >> 16);
      cardinfo->SD_cid.ProdName1 |= tmp << 8;
      tmp = (unsigned char)((CID_Tab[1] & 0x0000FF00) >> 8);
      cardinfo->SD_cid.ProdName1 |= tmp;
      tmp = (unsigned char)(CID_Tab[1] & 0x000000FF);
      cardinfo->SD_cid.ProdName2 = tmp;
      tmp = (unsigned char)((CID_Tab[2] & 0xFF000000) >> 24);
      cardinfo->SD_cid.ProdRev = tmp;
      tmp = (unsigned char)((CID_Tab[2] & 0x00FF0000) >> 16);
      cardinfo->SD_cid.ProdSN = tmp << 24;
      tmp = (unsigned char)((CID_Tab[2] & 0x0000FF00) >> 8);
      cardinfo->SD_cid.ProdSN |= tmp << 16;
      tmp = (unsigned char)(CID_Tab[2] & 0x000000FF);
      cardinfo->SD_cid.ProdSN |= tmp << 8;
      tmp = (unsigned char)((CID_Tab[3] & 0xFF000000) >> 24);
      cardinfo->SD_cid.ProdSN |= tmp;
      tmp = (unsigned char)((CID_Tab[3] & 0x00FF0000) >> 16);
      cardinfo->SD_cid.Reserved1 |= (tmp & 0xF0) >> 4;
      cardinfo->SD_cid.ManufactDate = (tmp & 0x0F) << 8;
      tmp = (unsigned char)((CID_Tab[3] & 0x0000FF00) >> 8);
      cardinfo->SD_cid.ManufactDate |= tmp;
      tmp = (unsigned char)(CID_Tab[3] & 0x000000FF);
      cardinfo->SD_cid.CID_CRC = (tmp & 0xFE) >> 1;
      cardinfo->SD_cid.Reserved2 = 1;
      return errorstatus;
}


SD_Error SD_SelectDeselect (unsigned int addr)
{
      SDIO_CmdInitStructure.SDIO_Argument = addr;
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEL_DESEL_CARD;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
      return CmdResp1Error(SD_CMD_SEL_DESEL_CARD);
}


SD_Error SD_EnableWideBusOperation (unsigned int WideMode)
{
      SD_Error errorstatus = SD_OK;
      if (SDIO_MULTIMEDIA_CARD == CardType) {
         errorstatus = SD_UNSUPPORTED_FEATURE;
         return(errorstatus);
      }	else if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == CardType) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == CardType) || (SDIO_HIGH_CAPACITY_SD_CARD == CardType)) {
         if (SDIO_BusWide_8b == WideMode) {
            errorstatus = SD_UNSUPPORTED_FEATURE;
            return(errorstatus);
         } else {
	    errorstatus = SDEnWideBus(WideMode);
 	    if (SD_OK==errorstatus) {
	       SDIO->CLKCR &= ~(3 << 11);
	       SDIO->CLKCR |= WideMode;
	       SDIO->CLKCR |= 0 << 14;
	    }
	 }
      }
      return errorstatus;
}


SD_Error SD_SetDeviceMode (unsigned int Mode)
{
      SD_Error errorstatus = SD_OK;
      if ((Mode == SD_DMA_MODE) || (Mode == SD_POLLING_MODE)) DeviceMode = Mode;else errorstatus = SD_INVALID_PARAMETER;
      return errorstatus;
}


SD_Error SD_Init (void)
{
      NVIC_InitTypeDef NVIC_InitStructure;
      SD_Error errorstatus = SD_OK;
      unsigned char clkdiv = 0;
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_DMA2, ENABLE);
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_SDIO, ENABLE);
      RCC_APB2PeriphResetCmd(RCC_APB2Periph_SDIO, ENABLE);
      //PC8,9,10,11,12
      GPIO_Init_Pin(GPIOC,GPIO_Pin_8,GPIO_Speed_50MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOC,GPIO_Pin_9,GPIO_Speed_50MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOC,GPIO_Pin_10,GPIO_Speed_50MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOC,GPIO_Pin_11,GPIO_Speed_50MHz,GPIO_Mode_AF_PP_PU);
      GPIO_Init_Pin(GPIOC,GPIO_Pin_12,GPIO_Speed_50MHz,GPIO_Mode_AF_PP_PU);
      //PD2
      GPIO_Init_Pin(GPIOD,GPIO_Pin_2,GPIO_Speed_50MHz,GPIO_Mode_AF_PP_PU);
      //AF
      GPIO_PinAFConfig(GPIOC,GPIO_PinSource8,GPIO_AF_SDIO);
      GPIO_PinAFConfig(GPIOC,GPIO_PinSource9,GPIO_AF_SDIO);
      GPIO_PinAFConfig(GPIOC,GPIO_PinSource10,GPIO_AF_SDIO);
      GPIO_PinAFConfig(GPIOC,GPIO_PinSource11,GPIO_AF_SDIO);
      GPIO_PinAFConfig(GPIOC,GPIO_PinSource12,GPIO_AF_SDIO);
      GPIO_PinAFConfig(GPIOD,GPIO_PinSource2,GPIO_AF_SDIO);
      RCC_APB2PeriphResetCmd(RCC_APB2Periph_SDIO, DISABLE);
      SDIO_Register_Deinit();
      NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
      NVIC_Init(&NVIC_InitStructure);
      errorstatus = SD_PowerON();
      if (errorstatus == SD_OK) errorstatus = SD_InitializeCards();
      if (errorstatus == SD_OK) errorstatus = SD_GetCardInfo(&SDCardInfo);
      if (errorstatus == SD_OK) errorstatus = SD_SelectDeselect((unsigned int)(SDCardInfo.RCA << 16));
      if (errorstatus == SD_OK) errorstatus = SD_EnableWideBusOperation(SDIO_BusWide_4b);
      if ((errorstatus == SD_OK) || (SDIO_MULTIMEDIA_CARD == CardType)) {
	 if (SDCardInfo.CardType == SDIO_STD_CAPACITY_SD_CARD_V1_1 || SDCardInfo.CardType == SDIO_STD_CAPACITY_SD_CARD_V2_0) {
	    clkdiv = SDIO_TRANSFER_CLK_DIV + 2;		//12Mhz
	 } else clkdiv = SDIO_TRANSFER_CLK_DIV;		//24Mhz
	 SDIO_Clock_Set(clkdiv);
	 //errorstatus = SD_SetDeviceMode(SD_DMA_MODE);
	 errorstatus = SD_SetDeviceMode(SD_POLLING_MODE);
      }
      return errorstatus;
}


void SD_DMA_Config (unsigned int *mbuf,unsigned int bufsize,unsigned int dir)
{
      DMA_InitTypeDef DMA_InitStructure;
      save_buff_size = bufsize;
      while (DMA_GetCmdStatus(DMA2_Stream3) != DISABLE){}
      DMA_DeInit(DMA2_Stream3);
      DMA_InitStructure.DMA_Channel = DMA_Channel_4;
      DMA_InitStructure.DMA_PeripheralBaseAddr = (unsigned int)&SDIO->FIFO;
      DMA_InitStructure.DMA_Memory0BaseAddr = (unsigned int)mbuf;
      DMA_InitStructure.DMA_DIR = dir;
      DMA_InitStructure.DMA_BufferSize = 0;
      DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
      DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
      DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
      DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
      DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
      DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
      DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
      DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
      DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_INC4;
      DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_INC4;
      DMA_Init(DMA2_Stream3, &DMA_InitStructure);
      DMA_FlowControllerConfig(DMA2_Stream3,DMA_FlowCtrl_Peripheral);
      DMA_Cmd(DMA2_Stream3 ,ENABLE);
}


SD_Error SD_ReadBlock (unsigned char *buf,long long addr,unsigned short blksize)
{
      SD_Error errorstatus = SD_OK;
      unsigned char power;
      unsigned int count = 0;
      //unsigned int *tempbuff = (unsigned int *)buf;	// Pointer
      unsigned int *tempbuff = (void *)buf;	// Pointer      
      unsigned int timeout = SDIO_DATATIMEOUT;
      if (NULL == buf) return SD_INVALID_PARAMETER;
      SDIO->DCTRL = 0x0;
      if (CardType == SDIO_HIGH_CAPACITY_SD_CARD) {
	 blksize = 512;
	 addr >>= 9;
      }
      SDIO_DataInitStructure.SDIO_DataBlockSize = SDIO_DataBlockSize_1b;
      SDIO_DataInitStructure.SDIO_DataLength = 0;
      SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
      SDIO_DataInitStructure.SDIO_DPSM= SDIO_DPSM_Enable;
      SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToCard;
      SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
      SDIO_DataConfig(&SDIO_DataInitStructure);
      if (SDIO->RESP1 & SD_CARD_LOCKED) return SD_LOCK_UNLOCK_FAILED;
      if ((blksize > 0) && (blksize <= 2048) && ((blksize & (blksize - 1)) == 0)) {
	 power = convert_from_bytes_to_power_of_two(blksize);
	 SDIO_CmdInitStructure.SDIO_Argument = blksize;
         SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
         SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
         SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
         SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
         SDIO_SendCommand(&SDIO_CmdInitStructure);
	 errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);
	 if (errorstatus != SD_OK) return errorstatus;
      } else return SD_INVALID_PARAMETER;
      SDIO_DataInitStructure.SDIO_DataBlockSize = power << 4;
      SDIO_DataInitStructure.SDIO_DataLength = blksize;
      SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
      SDIO_DataInitStructure.SDIO_DPSM= SDIO_DPSM_Enable;
      SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
      SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
      SDIO_DataConfig(&SDIO_DataInitStructure);
      SDIO_CmdInitStructure.SDIO_Argument = addr;
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_READ_SINGLE_BLOCK;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
      errorstatus = CmdResp1Error(SD_CMD_READ_SINGLE_BLOCK);
      if (errorstatus != SD_OK) return errorstatus;
      if (DeviceMode == SD_POLLING_MODE) {
 	 //__disable_irq();
	 while (!(SDIO->STA & ((1 << 5) | (1 << 1) | (1 << 3) | (1 << 10) | (1 << 9)))) {
	       if (SDIO_GetFlagStatus(SDIO_FLAG_RXFIFOHF) != RESET) {
		  for (count=0;count<8;count++) {
		      *(tempbuff + count) = SDIO->FIFO;
		  }
		  tempbuff += 8;
		  timeout = 0x7FFFFF;
	       } else {
		  if (timeout == 0) return SD_DATA_TIMEOUT;
		  timeout--;
	       }
	 }
	 if (SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET) {
	    SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
	    return SD_DATA_TIMEOUT;
	 } else if (SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET) {
	    SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
	    return SD_DATA_CRC_FAIL;
	 } else if (SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET) {
	    SDIO_ClearFlag(SDIO_FLAG_RXOVERR);
	    return SD_RX_OVERRUN;
	 } else if (SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET) {
	    SDIO_ClearFlag(SDIO_FLAG_STBITERR);
	    return SD_START_BIT_ERR;
	 }
	 while (SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET) {
	       *tempbuff = SDIO->FIFO;
	       tempbuff++;
	 }
	 //__enable_irq();
	 SDIO_ClearFlag(SDIO_STATIC_FLAGS);
      } else if (DeviceMode == SD_DMA_MODE) {
 	 TransferError = SD_OK;
	 StopCondition = 0;
	 TransferEnd = 0;
	 SDIO->MASK |= (1 << 1) | (1 << 3) | (1 << 8) | (1 << 5) | (1 << 9);
	 SDIO->DCTRL |= 1 << 3;
 	 //SD_DMA_Config((unsigned int *)buf,blksize,DMA_DIR_PeripheralToMemory);	// Pointer
 	 SD_DMA_Config((void *)buf,blksize,DMA_DIR_PeripheralToMemory);	// Pointer 	 
 	 while (((DMA2->LISR & (1 << 27)) == RESET) && (TransferEnd == 0) && (TransferError == SD_OK) && timeout) timeout--;
	 if (timeout == 0) return SD_DATA_TIMEOUT;
	 if (TransferError != SD_OK) errorstatus = TransferError;
      }
      return errorstatus;
}


SD_Error SD_ReadMultiBlocks (unsigned char *buf,long long addr,unsigned short blksize,unsigned int nblks)
{
      SD_Error errorstatus = SD_OK;
      unsigned char power;
      unsigned int count = 0;
      unsigned int timeout = SDIO_DATATIMEOUT;
      //m_tempbuff = (unsigned int *)buf;		// Pointer
      m_tempbuff = (void *)buf;		// Pointer      
      SDIO->DCTRL = 0x0;
      if (CardType == SDIO_HIGH_CAPACITY_SD_CARD) {
	 blksize = 512;
	 addr >>= 9;
      }
      SDIO_DataInitStructure.SDIO_DataBlockSize = 0;
      SDIO_DataInitStructure.SDIO_DataLength = 0;
      SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
      SDIO_DataInitStructure.SDIO_DPSM= SDIO_DPSM_Enable;
      SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToCard;
      SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
      SDIO_DataConfig(&SDIO_DataInitStructure);
      if (SDIO->RESP1 & SD_CARD_LOCKED) return SD_LOCK_UNLOCK_FAILED;
      if ((blksize > 0) && (blksize <= 2048) && ((blksize & (blksize - 1)) == 0)) {
	 power = convert_from_bytes_to_power_of_two(blksize);
	 SDIO_CmdInitStructure.SDIO_Argument = blksize;
	 SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
	 SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	 SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	 SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	 SDIO_SendCommand(&SDIO_CmdInitStructure);
	 errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);
	 if (errorstatus != SD_OK) return errorstatus;
      } else return SD_INVALID_PARAMETER;
      if (nblks > 1) {
 	 if (nblks * blksize > SD_MAX_DATA_LENGTH) return SD_INVALID_PARAMETER;
	 SDIO_DataInitStructure.SDIO_DataBlockSize = power << 4;
	 SDIO_DataInitStructure.SDIO_DataLength = nblks * blksize;
	 SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
	 SDIO_DataInitStructure.SDIO_DPSM= SDIO_DPSM_Enable;
	 SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
	 SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
	 SDIO_DataConfig(&SDIO_DataInitStructure);
         SDIO_CmdInitStructure.SDIO_Argument = addr;
	 SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_READ_MULT_BLOCK;
	 SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	 SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	 SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	 SDIO_SendCommand(&SDIO_CmdInitStructure);
 	 errorstatus = CmdResp1Error(SD_CMD_READ_MULT_BLOCK);
	 if (errorstatus != SD_OK) return errorstatus;
 	 if (DeviceMode == SD_POLLING_MODE) {
	    //__disable_irq();
	    while (!(SDIO->STA & ((1 << 5) | (1 << 1) | (1 << 3) | (1 << 8) | (1 << 9)))) {
	          if (SDIO_GetFlagStatus(SDIO_FLAG_RXFIFOHF) != RESET) {
		     for (count=0;count<8;count++) {
			 *(m_tempbuff + count) = SDIO->FIFO;
		     }
		     m_tempbuff += 8;
		     timeout = 0x7FFFFF;
		  } else {
		     if (timeout == 0) return SD_DATA_TIMEOUT;
		     timeout--;
		  }
	    }
	    if (SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET) {
	       SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
	       return SD_DATA_TIMEOUT;
	    } else if (SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET) {
	       SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
	       return SD_DATA_CRC_FAIL;
	    } else if (SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET) {
	       SDIO_ClearFlag(SDIO_FLAG_RXOVERR);
	       return SD_RX_OVERRUN;
	    } else if (SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET) {
	       SDIO_ClearFlag(SDIO_FLAG_STBITERR);
	       return SD_START_BIT_ERR;
	    }
   	    while (SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET) {
		  *m_tempbuff = SDIO->FIFO;
		  m_tempbuff++;
	    }
	    if (SDIO_GetFlagStatus(SDIO_FLAG_DATAEND) != RESET) {
	       if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == CardType) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == CardType) || (SDIO_HIGH_CAPACITY_SD_CARD == CardType)) {
		  SDIO_CmdInitStructure.SDIO_Argument = 0;
		  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_STOP_TRANSMISSION;
		  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		  SDIO_SendCommand(&SDIO_CmdInitStructure);
		  errorstatus = CmdResp1Error(SD_CMD_STOP_TRANSMISSION);
		  if (errorstatus != SD_OK) return errorstatus;
	       }
 	    }
	    //__enable_irq();
	    SDIO_ClearFlag(SDIO_STATIC_FLAGS);
 	 } else if (DeviceMode == SD_DMA_MODE) {
	    TransferError = SD_OK;
	    StopCondition = 1;
	    TransferEnd = 0;
	    SDIO->MASK |= (1 << 1) | (1 << 3) | (1 << 8) | (1 << 5) | (1 << 9);
	    SDIO->DCTRL |= 1 << 3;
	    //SD_DMA_Config((unsigned int *)buf,nblks * blksize,DMA_DIR_PeripheralToMemory);	// Pointer
	    SD_DMA_Config((void *)buf,nblks * blksize,DMA_DIR_PeripheralToMemory);	// Pointer	    
	    while (((DMA2->LISR & (1 << 27)) == RESET) && timeout) timeout--;
	    if (timeout == 0) return SD_DATA_TIMEOUT;
	    while ((TransferEnd == 0) && (TransferError == SD_OK));
	    if (TransferError != SD_OK) errorstatus = TransferError;
	 }
      }
      return errorstatus;
}


SD_Error SD_WriteBlock (unsigned char *buf,long long addr,unsigned short blksize)
{
      SD_Error errorstatus = SD_OK;
      unsigned char power = 0,cardstate = 0;
      unsigned int timeout = 0,bytestransferred = 0;
      unsigned int cardstatus = 0,count = 0,restwords = 0;
      unsigned int tlen = blksize;
      //unsigned int *tempbuff = (unsigned int *)buf;	// Pointer
      unsigned int *tempbuff = (void *)buf;	// Pointer      
      if (buf == NULL) return SD_INVALID_PARAMETER;
      SDIO->DCTRL= 0x0;
      SDIO_DataInitStructure.SDIO_DataBlockSize = 0;
      SDIO_DataInitStructure.SDIO_DataLength = 0;
      SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
      SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
      SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToCard;
      SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
      SDIO_DataConfig(&SDIO_DataInitStructure);
      if (SDIO->RESP1 & SD_CARD_LOCKED) return SD_LOCK_UNLOCK_FAILED;
      if (CardType == SDIO_HIGH_CAPACITY_SD_CARD) {
	 blksize = 512;
	 addr >>= 9;
      }
      if ((blksize > 0) && (blksize <= 2048) && ((blksize & (blksize - 1)) == 0)) {
	 power = convert_from_bytes_to_power_of_two(blksize);
	 SDIO_CmdInitStructure.SDIO_Argument = blksize;
	 SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
	 SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	 SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	 SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	 SDIO_SendCommand(&SDIO_CmdInitStructure);
	 errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);
	 if (errorstatus != SD_OK) return errorstatus;
      } else return SD_INVALID_PARAMETER;
      SDIO_CmdInitStructure.SDIO_Argument = (unsigned int)RCA << 16;
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
      errorstatus = CmdResp1Error(SD_CMD_SEND_STATUS);
      if (errorstatus != SD_OK) return errorstatus;
      cardstatus = SDIO->RESP1;
      timeout = SD_DATATIMEOUT;
      while (((cardstatus & 0x00000100) == 0) && (timeout> 0)) {
	    timeout--;
	    SDIO_CmdInitStructure.SDIO_Argument = (unsigned int)RCA << 16;
	    SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
	    SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	    SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	    SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	    SDIO_SendCommand(&SDIO_CmdInitStructure);
	    errorstatus = CmdResp1Error(SD_CMD_SEND_STATUS);
	    if (errorstatus != SD_OK) return errorstatus;
	    cardstatus = SDIO->RESP1;
      }
      if (timeout == 0) return SD_ERROR;
      SDIO_CmdInitStructure.SDIO_Argument = addr;
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_WRITE_SINGLE_BLOCK;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
      errorstatus = CmdResp1Error(SD_CMD_WRITE_SINGLE_BLOCK);
      if (errorstatus != SD_OK) return errorstatus;
      StopCondition = 0;
      SDIO_DataInitStructure.SDIO_DataBlockSize = power<<4;
      SDIO_DataInitStructure.SDIO_DataLength = blksize;
      SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
      SDIO_DataInitStructure.SDIO_DPSM= SDIO_DPSM_Enable;
      SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToCard;
      SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
      SDIO_DataConfig(&SDIO_DataInitStructure);
      timeout = SDIO_DATATIMEOUT;
      if (DeviceMode == SD_POLLING_MODE) {
	 //__disable_irq();
	 while (!(SDIO->STA & ((1 << 10) | (1 << 4) | (1 << 1) | (1 << 3) | (1 << 9)))) {
	       if (SDIO_GetFlagStatus(SDIO_FLAG_TXFIFOHE) != RESET) {
		  if ((tlen - bytestransferred) < SD_HALFFIFOBYTES) {
		     restwords = ((tlen - bytestransferred) % 4 == 0) ? ((tlen - bytestransferred) / 4) : ((tlen - bytestransferred) / 4 + 1);
		     for (count=0;count<restwords;count++,tempbuff++,bytestransferred+=4) {
			 SDIO->FIFO = *tempbuff;
		     }
		  } else {
		     for (count=0;count<8;count++) {
			 SDIO->FIFO = *(tempbuff + count);
		     }
		     tempbuff += 8;
		     bytestransferred += 32;
		  }
		  timeout = 0x3FFFFFFF;
	       } else {
		  if (timeout == 0) return SD_DATA_TIMEOUT;
		  timeout--;
	       }
	 }
	 if (SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET) {
	    SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
	    return SD_DATA_TIMEOUT;
	 } else if (SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET) {
	    SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
	    return SD_DATA_CRC_FAIL;
	 } else if (SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) != RESET) {
	    SDIO_ClearFlag(SDIO_FLAG_TXUNDERR);
	    return SD_TX_UNDERRUN;
	 } else if (SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET) {
	    SDIO_ClearFlag(SDIO_FLAG_STBITERR);
	    return SD_START_BIT_ERR;
	 }
	 //__enable_irq();
	 SDIO_ClearFlag(SDIO_STATIC_FLAGS);
      } else if (DeviceMode == SD_DMA_MODE) {
   	 TransferError = SD_OK;
	 StopCondition = 0;
	 TransferEnd = 0;
	 SDIO->MASK |= (1 << 1) | (1 << 3) | (1 << 8) | (1 << 4) | (1 << 9);
	 //SD_DMA_Config((unsigned int *)&buf,blksize,DMA_DIR_MemoryToPeripheral);		// Pointer
	 SD_DMA_Config((void *)&buf,blksize,DMA_DIR_MemoryToPeripheral);		// Pointer	 
 	 SDIO->DCTRL |= 1 << 3;
 	 while (((DMA2->LISR & (1 << 27)) == RESET) && timeout) timeout--;
	 if (timeout == 0) {
  	    SD_Init();
	    return SD_DATA_TIMEOUT;
 	 }
	 timeout = SDIO_DATATIMEOUT;
	 while ((TransferEnd == 0) && (TransferError == SD_OK) && timeout) timeout--;
 	 if (timeout == 0) return SD_DATA_TIMEOUT;
  	 if (TransferError != SD_OK) return TransferError;
      }
      SDIO_ClearFlag(SDIO_STATIC_FLAGS);
      errorstatus = IsCardProgramming(&cardstate);
      while ((errorstatus == SD_OK) && ((cardstate == SD_CARD_PROGRAMMING) || (cardstate == SD_CARD_RECEIVING))) {
	    errorstatus = IsCardProgramming(&cardstate);
      }
      return errorstatus;
}


SD_Error SD_WriteMultiBlocks (unsigned char *buf,long long addr,unsigned short blksize,unsigned int nblks)
{
      SD_Error errorstatus = SD_OK;
      unsigned char power = 0,cardstate = 0;
      unsigned int timeout = 0,bytestransferred = 0;
      unsigned int count = 0,restwords = 0;
      unsigned int tlen = nblks * blksize;
      //unsigned int *tempbuff = (unsigned int *)buf;		// Pointer
      unsigned int *tempbuff = (void *)buf;		// Pointer      
      if (buf == NULL) return SD_INVALID_PARAMETER;
      SDIO->DCTRL = 0x0;
      SDIO_DataInitStructure.SDIO_DataBlockSize = 0;
      SDIO_DataInitStructure.SDIO_DataLength = 0;
      SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
      SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
      SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToCard;
      SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
      SDIO_DataConfig(&SDIO_DataInitStructure);
      if (SDIO->RESP1 & SD_CARD_LOCKED) return SD_LOCK_UNLOCK_FAILED;
      if (CardType == SDIO_HIGH_CAPACITY_SD_CARD) {
	 blksize = 512;
	 addr >>= 9;
      }
      if ((blksize > 0) && (blksize <= 2048) && ((blksize & (blksize - 1)) == 0)) {
	 power = convert_from_bytes_to_power_of_two(blksize);
	 SDIO_CmdInitStructure.SDIO_Argument = blksize;
	 SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
	 SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	 SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	 SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	 SDIO_SendCommand(&SDIO_CmdInitStructure);
	 errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);
	 if (errorstatus != SD_OK) return errorstatus;
      } else return SD_INVALID_PARAMETER;
      if (nblks > 1) {
	 if (nblks * blksize > SD_MAX_DATA_LENGTH) return SD_INVALID_PARAMETER;
     	 if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == CardType) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == CardType) || (SDIO_HIGH_CAPACITY_SD_CARD == CardType)) {
	    SDIO_CmdInitStructure.SDIO_Argument = (unsigned int)RCA << 16;
	    SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
	    SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	    SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	    SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	    SDIO_SendCommand(&SDIO_CmdInitStructure);
	    errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
	    if (errorstatus != SD_OK) return errorstatus;
	    SDIO_CmdInitStructure.SDIO_Argument = nblks;
	    SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCK_COUNT;
	    SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	    SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	    SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	    SDIO_SendCommand(&SDIO_CmdInitStructure);
	    errorstatus = CmdResp1Error(SD_CMD_SET_BLOCK_COUNT);
	    if (errorstatus != SD_OK) return errorstatus;
	 }
	 SDIO_CmdInitStructure.SDIO_Argument = addr;
	 SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_WRITE_MULT_BLOCK;
	 SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	 SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	 SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	 SDIO_SendCommand(&SDIO_CmdInitStructure);
 	 errorstatus = CmdResp1Error(SD_CMD_WRITE_MULT_BLOCK);
	 if (errorstatus != SD_OK) return errorstatus;
         SDIO_DataInitStructure.SDIO_DataBlockSize = power << 4;
	 SDIO_DataInitStructure.SDIO_DataLength = nblks * blksize;
	 SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
	 SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
	 SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToCard;
	 SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
	 SDIO_DataConfig(&SDIO_DataInitStructure);
	 if (DeviceMode == SD_POLLING_MODE) {
	    timeout = SDIO_DATATIMEOUT;
	    //__disable_irq();
	    while (!(SDIO->STA & ((1 << 4) | (1 << 1) | (1 << 8) | (1 << 3) | (1 << 9)))) {
		  if (SDIO_GetFlagStatus(SDIO_FLAG_TXFIFOHE) != RESET) {
		     if ((tlen - bytestransferred) < SD_HALFFIFOBYTES) {
			restwords = ((tlen - bytestransferred) % 4 == 0) ? ((tlen - bytestransferred) / 4) : ((tlen - bytestransferred) / 4 + 1);
			for (count=0;count<restwords;count++,tempbuff++,bytestransferred+=4) {
			    SDIO->FIFO = *tempbuff;
			}
		     } else {
			for (count=0;count< SD_HALFFIFO;count++) {
			    SDIO->FIFO = *(tempbuff + count);
			}
			tempbuff += SD_HALFFIFO;
			bytestransferred += SD_HALFFIFOBYTES;
		     }
		     timeout = 0x3FFFFFFF;
		  } else {
		     if (timeout == 0) return SD_DATA_TIMEOUT;
		     timeout--;
		  }
	    }
	    if (SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET) {
	       SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
	       return SD_DATA_TIMEOUT;
	    } else if (SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)	{
	       SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
	       return SD_DATA_CRC_FAIL;
	    } else if (SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) != RESET) {
	       SDIO_ClearFlag(SDIO_FLAG_TXUNDERR);
	       return SD_TX_UNDERRUN;
	    } else if (SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET) {
	       SDIO_ClearFlag(SDIO_FLAG_STBITERR);
	       return SD_START_BIT_ERR;
	    }
	    if (SDIO_GetFlagStatus(SDIO_FLAG_DATAEND) != RESET)	{
	       if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == CardType) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == CardType) || (SDIO_HIGH_CAPACITY_SD_CARD == CardType)) {
		  SDIO_CmdInitStructure.SDIO_Argument = 0;
		  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_STOP_TRANSMISSION;
		  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		  SDIO_SendCommand(&SDIO_CmdInitStructure);
		  errorstatus = CmdResp1Error(SD_CMD_STOP_TRANSMISSION);
		  if (errorstatus != SD_OK) return errorstatus;
	       }
	    }
	    //__enable_irq();
	    SDIO_ClearFlag(SDIO_STATIC_FLAGS);
	 } else if (DeviceMode == SD_DMA_MODE) {
	    TransferError = SD_OK;
	    StopCondition = 1;
	    TransferEnd = 0;
	    SDIO->MASK |= (1 << 1) | (1 << 3) | (1 << 8) | (1 << 4) | (1 << 9);
	    //SD_DMA_Config((unsigned int *)buf,nblks * blksize,DMA_DIR_MemoryToPeripheral);	// Pointer
	    SD_DMA_Config((void *)buf,nblks * blksize,DMA_DIR_MemoryToPeripheral);	// Pointer	    
	    SDIO->DCTRL |= 1 << 3;
	    timeout = SDIO_DATATIMEOUT;
	    while (((DMA2->LISR & (1 << 27)) == RESET) && timeout) timeout--;
	    if (timeout == 0) {
  	       SD_Init();
	       return SD_DATA_TIMEOUT;
	    }
	    timeout = SDIO_DATATIMEOUT;
	    while ((TransferEnd == 0) && (TransferError == SD_OK) && timeout) timeout--;
	    if (timeout == 0) return SD_DATA_TIMEOUT;
	    if (TransferError != SD_OK) return TransferError;
	 }
      }
      SDIO_ClearFlag(SDIO_STATIC_FLAGS);
      errorstatus = IsCardProgramming(&cardstate);
      while ((errorstatus == SD_OK) && ((cardstate == SD_CARD_PROGRAMMING) || (cardstate == SD_CARD_RECEIVING))) {
	    errorstatus = IsCardProgramming(&cardstate);
      }
      return errorstatus;
}


SD_Error SD_ProcessIRQSrc (void)
{
      if (SDIO_GetFlagStatus(SDIO_FLAG_DATAEND) != RESET) {
	 if (StopCondition == 1) {
	    SDIO_CmdInitStructure.SDIO_Argument = 0;
	    SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_STOP_TRANSMISSION;
	    SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	    SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	    SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	    SDIO_SendCommand(&SDIO_CmdInitStructure);
	    TransferError = CmdResp1Error(SD_CMD_STOP_TRANSMISSION);
	 } else TransferError = SD_OK;
 	 SDIO->ICR |= 1 << 8;
	 SDIO->MASK &= ~((1 << 1) | (1 << 3) | (1 << 8) | (1 << 14) | (1 << 15) | (1 << 4) | (1 << 5) | (1 << 9));
 	 TransferEnd = 1;
	 return(TransferError);
      }
      if (SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET) {
	 SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
	 SDIO->MASK &= ~((1 << 1) | (1 << 3) | (1 << 8) | (1 << 14) | (1 << 15) | (1 << 4) | (1 << 5) | (1 << 9));
	 TransferError = SD_DATA_CRC_FAIL;
	 return(SD_DATA_CRC_FAIL);
      }
      if (SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET) {
	 SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
	 SDIO->MASK &= ~((1 << 1) | (1 << 3) | (1 << 8) | (1 << 14) | (1 << 15) | (1 << 4) | (1 << 5) | (1 << 9));
	 TransferError = SD_DATA_TIMEOUT;
	 return(SD_DATA_TIMEOUT);
      }
      if (SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET) {
	 SDIO_ClearFlag(SDIO_FLAG_RXOVERR);
	 SDIO->MASK &= ~((1 << 1) | (1 << 3) | (1 << 8) | (1 << 14) | (1 << 15) | (1 << 4) | (1 << 5) | (1 << 9));
	 TransferError = SD_RX_OVERRUN;
	 return(SD_RX_OVERRUN);
      }
      if (SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) != RESET) {
	 SDIO_ClearFlag(SDIO_FLAG_TXUNDERR);
	 SDIO->MASK &= ~((1 << 1) | (1 << 3) | (1 << 8) | (1 << 14) | (1 << 15) | (1 << 4) | (1 << 5) | (1 << 9));
	 TransferError = SD_TX_UNDERRUN;
	 return(SD_TX_UNDERRUN);
      }
      if (SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET) {
	 SDIO_ClearFlag(SDIO_FLAG_STBITERR);
	 SDIO->MASK &= ~((1 << 1) | (1 << 3) | (1 << 8) | (1 << 14) | (1 << 15) | (1 << 4) | (1 << 5) | (1 << 9));
	 TransferError = SD_START_BIT_ERR;
	 return(SD_START_BIT_ERR);
      }
      return(SD_OK);
}


void SDIO_IRQHandler (void)
{
      SD_ProcessIRQSrc();
}


SD_Error SD_SendStatus (unsigned int *pcardstatus)
{
      SD_Error errorstatus = SD_OK;
      if (pcardstatus == NULL) {
	 errorstatus = SD_INVALID_PARAMETER;
	 return errorstatus;
      }
      SDIO_CmdInitStructure.SDIO_Argument = (unsigned int)RCA << 16;
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
      errorstatus = CmdResp1Error(SD_CMD_SEND_STATUS);
      if (errorstatus != SD_OK) return errorstatus;
      *pcardstatus = SDIO->RESP1;
      return errorstatus;
}


SDCardState SD_GetState (void)
{
      unsigned int resp1 = 0;
      if (SD_SendStatus(&resp1) != SD_OK) return SD_CARD_ERROR;else return (SDCardState)((resp1 >> 9) & 0x0F);
}


unsigned char SD_ReadDisk (unsigned char *buf,unsigned int sector,unsigned char cnt)
{
      unsigned char sta = SD_OK;
      long long lsector = sector;
      unsigned char n;
      lsector <<= 9;
      if ((unsigned int)buf % 4 != 0) {
	 for (n=0;n<cnt;n++) {
	     sta = SD_ReadBlock(SDIO_DATA_BUFFER,lsector + 512 * n,512);
	     memcpy(buf,SDIO_DATA_BUFFER,512);
	     buf += 512;
	 }
      } else {
	 if (cnt == 1) sta = SD_ReadBlock(buf,lsector,512);else sta = SD_ReadMultiBlocks(buf,lsector,512,cnt);
      }
      return sta;
}


unsigned char SD_WriteDisk (unsigned char *buf,unsigned int sector,unsigned char cnt)
{
      unsigned char sta = SD_OK;
      unsigned char n;
      long long lsector = sector;
      lsector <<= 9;
      if ((unsigned int)buf % 4 != 0) {
	 for (n=0;n<cnt;n++) {
	     memcpy(SDIO_DATA_BUFFER,buf,512);
	     sta = SD_WriteBlock(SDIO_DATA_BUFFER,lsector + 512 * n,512);
	     buf += 512;
	 }
      } else {
      	 if (cnt == 1) sta = SD_WriteBlock(buf,lsector,512);else sta = SD_WriteMultiBlocks(buf,lsector,512,cnt);
      }
      return sta;
}
