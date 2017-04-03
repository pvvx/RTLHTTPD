/*
 * cgiflash_rtl.c
 *
 *  Created on: Feb 7, 2017
 *      Author: sharikov
 */

#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "objects.h"
#include "flash_api.h"

#include <stdint.h>
#include <string.h>
#include "platform.h"
//#include "espfs.h"

#include "flash_api.h"
#include "device_lock.h"
#include <platform/platform_stdlib.h>

#include "cgiflash_rtl.h"

#ifndef UPGRADE_FLAG_FINISH
#define UPGRADE_FLAG_FINISH     0x02
#endif

#define FLST_START 0
#define FLST_WRITE 1
#define FLST_DONE 2
#define FLST_ERROR 3


// MAC and WI-FI calibration data in flash
// must be sector aligned
#define CALIBRATION_START 0xa000
#define CALIBRATION_SIZE  SPI_FLASH_SEC_SIZE

#define EXCLUDE_AREA_START CALIBRATION_START
#define EXCLUDE_AREA_END  (CALIBRATION_START+CALIBRATION_SIZE)


int check_exclude_area(UploadState_t* state)
{
	int ret = state->pagePos;

	if (state->currentAddress >= EXCLUDE_AREA_END)
		return ret;
	if ((state->currentAddress+state->pagePos) <= EXCLUDE_AREA_START)
		return ret;

	// head not in exclude area, tail in exclude area
	if (state->currentAddress < EXCLUDE_AREA_START)
		return EXCLUDE_AREA_START - state->currentAddress;  // write head

	// whole buffer in exclude area
	if ((state->currentAddress+state->pagePos) < EXCLUDE_AREA_END)
		return 0;  // write none

	// head in exclude area, tail not in exclude area
	ret = state->pagePos;
	ret -= EXCLUDE_AREA_END - state->currentAddress;
	ret = -ret;  // write tail
	return ret;
}

/* aligned / unaligned flash write with auto-erase
 */
void write_buffer(UploadState_t* state)
{
	uint8_t  locked=0;
	int32_t  check_result;
	uint32_t vAddr;
	uint32_t vLen;
	uint32_t firstBlock, lastBlock;   // erase area
	int32_t  tmp;

	check_result =check_exclude_area(state);
	if (check_result) {
		vLen = abs(check_result);
		vAddr = state->currentAddress;
		if (check_result < 0)
			vAddr =	EXCLUDE_AREA_END;

		firstBlock = vAddr & ~(SPI_FLASH_SEC_SIZE-1);
		lastBlock = (vAddr + vLen -1) & ~(SPI_FLASH_SEC_SIZE-1);
		tmp=firstBlock;

		if (state->autoErase) {
			while (tmp <= lastBlock) {
				if (tmp != state->erasedAddress)
				{
					dbg("-E: %x\t1000", tmp);
					if (!locked) {
						device_mutex_lock(RT_DEV_LOCK_FLASH);
						locked=1;
					}
					flash_erase_sector(&flashobj, tmp);
					state->erasedAddress = tmp;
				}
				tmp+=SPI_FLASH_SEC_SIZE;
			}
		}

		if (!locked)
			device_mutex_lock(RT_DEV_LOCK_FLASH);
		if ((vLen & 3)==0) {
			dbg("PA: %x\t%x\t%x", vAddr, vLen, state->currentAddress);
			// aligned multi-page write (fast)
			flash_burst_write(&flashobj, vAddr, vLen,
					state->pageData + (vAddr - state->currentAddress));
		}
		else {
			dbg("PU: %x\t%x\t%x", vAddr, vLen, state->currentAddress);
			// unaligned multi-page write (slow)
			flash_stream_write(&flashobj, vAddr, vLen,
					state->pageData + (vAddr - state->currentAddress));
		}
		device_mutex_unlock(RT_DEV_LOCK_FLASH);
	}
	else
		dbg("--: %x", state->currentAddress);
	state->currentAddress+=state->pagePos;
	state->pagePos=0;
}




int cgiUploadFirmware(HttpdConnData *connData)
{
	CgiUploadFlashRtl_t *def=(CgiUploadFlashRtl_t*)connData->cgiArg;
	UploadState_t *state=(UploadState_t *)connData->cgiData;

	uint8_t *data;
	uint32_t dataLen;

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		if (state!=NULL) free(state);
		return HTTPD_CGI_DONE;
	}

	if (state==NULL) {
		//First call. Allocate and initialize state variable.
		dbg("Firmware upload cgi start.");
		state=malloc(sizeof(UploadState_t));
		if (state==NULL) {
			httpd_error("Can't allocate firmware upload struct!");
			return HTTPD_CGI_DONE;
		}
		memset(state, 0, sizeof(UploadState_t));
		state->state=FLST_START;
		connData->cgiData=state;
		state->err="Premature end";
	}

	data=connData->post->buff;
	dataLen=connData->post->buffLen;

	while (dataLen!=0) {
		if (state->state==FLST_START) {
			dbg("POST Header length: %d", connData->priv->headPos);
			for (int d=0; d<= connData->priv->headPos; d++)
			{
				if (connData->priv->head[d]!=0x00)
					httpd_printf("%c", connData->priv->head[d]);
				//else
				//	DiagPrintf("\n");
			}
			uint8_t *b;
			b = malloc(20);

			httpdGetHeader(connData, "x-start", b, 20);
			state->currentAddress = atoi(b);
			httpdGetHeader(connData, "x-erase", b, 20);
			state->autoErase = atoi(b) & 255;
			httpdGetHeader(connData, "Content-Length", b, 20);
			state->len = atoi(b);
			dbg("Start address=%d", state->currentAddress);
			dbg("Length=%d\tErase=%d", state->len, state->autoErase);
			if (state->len ==0)
				return HTTPD_CGI_DONE;

			state->pagePos=0;
			state->erasedAddress = (state->currentAddress - SPI_FLASH_SEC_SIZE) &
					~(SPI_FLASH_SEC_SIZE-1);
			state->state=FLST_WRITE;
			free(b);
		} else if (state->state==FLST_WRITE) {
			int32_t lenLeft= PAGELEN - state->pagePos;   // free space in page buffer
			if (state->len < lenLeft) {
				dbg("Last buffer");
				lenLeft=state->len; //last buffer can be a cut-off one
			}
			//See if we need to write the page.
			if (dataLen<lenLeft) {
				//Page isn't done yet. Copy data to buffer and exit.
				memcpy(&state->pageData[state->pagePos], data, dataLen);
				state->pagePos+=dataLen;
				state->len-=dataLen;
				dataLen=0;
			}
			else {
				//Finish page; take data we need from post buffer
				memcpy(&state->pageData[state->pagePos], data, lenLeft);
				data+=lenLeft;
				dataLen-=lenLeft;
				state->pagePos+=lenLeft;
				state->len-=lenLeft;
				write_buffer(state);
			}

			if (state->len==0)
				state->state=FLST_DONE;

		} else if (state->state==FLST_DONE) {
			warn("Huh? %d bogus bytes received after data received.", dataLen);
			//Ignore those bytes.
			dataLen=0;
		} else if (state->state==FLST_ERROR) {
			//Just eat up any bytes we receive.
			httpd_error("FLST_ERROR");
			dataLen=0;
		}
	}

	if (connData->post->len==connData->post->received) {
			//We're done! Format a response.
			info("Upload done. Sending response.");
			httpdStartResponse(connData, state->state==FLST_ERROR?400:200);
			httpdHeader(connData, "Content-Type", "text/plain");
			httpdEndHeaders(connData);
			if (state->state!=FLST_DONE) {
				httpdSend(connData, "Firmware image error:", -1);
				httpdSend(connData, state->err, -1);
				httpdSend(connData, "\n", -1);
			}
			free(state);
			return HTTPD_CGI_DONE;
		}

		return HTTPD_CGI_MORE;
}


int cgiRebootFirmware(HttpdConnData *connData)
{
	warn("Resetting...");

	// boot from flash
	HAL_WRITE32(SYSTEM_CTRL_BASE, 0x210, 0x211157); 	//mww 0x40000210 0x211157

	//ota_platform_reset();
	//wifi_off();

	// Set processor clock to default before system reset
	HAL_WRITE32(SYSTEM_CTRL_BASE, 0x14, 0x00000021);
	osDelay(50);
	// Cortex-M3 SCB->AIRCR
	HAL_WRITE32(0xE000ED00, 0x0C, (0x5FA << 16) |                             // VECTKEY
	                              (HAL_READ32(0xE000ED00, 0x0C) & (7 << 8)) | // PRIGROUP
	                              (1 << 2));                                  // SYSRESETREQ
	while(1) osDelay(1000);
	return HTTPD_CGI_DONE;
}
