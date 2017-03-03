/*
 * cgiflash_rtl.h
 *
 *  Created on: Feb 7, 2017
 *      Author: sharikov
 */

#ifndef CGIFLASH_RTL_H_
#define CGIFLASH_RTL_H_

#include "httpd.h"

#define SPI_FLASH_SEC_SIZE 4096
#define PAGELEN 256

typedef struct UploadState {
	uint32_t currentAddress;
	uint32_t len;
	uint32_t erasedAddress;
	uint32_t pageData[PAGELEN/4];  // aligned data buffer
	char *err;
	uint16_t pagePos;   // valid data count in pageData
	uint8_t  autoErase;
	uint8_t  state;
} UploadState_t;

typedef struct {
	//uint32_t start;  // start flash address
	uint32_t flash_size;

} CgiUploadFlashRtl_t;

int cgiUploadFirmware(HttpdConnData *connData);
int cgiRebootFirmware(HttpdConnData *connData);

#endif /* CGIFLASH_RTL_H_ */
