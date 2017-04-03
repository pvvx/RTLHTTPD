#ifndef PLATFORM_H
#define PLATFORM_H
#include "user_config.h"
#define  FREERTOS
#ifdef FREERTOS
//#include "esp_timer.h"
typedef struct RtosConnType RtosConnType;
typedef RtosConnType* ConnTypePtr;
#if 0
//Unfortunately, this does not always work... the latest esp32 sdk, for example, breaks this.
#define httpd_printf(fmt, ...) do {	\
	static const char flash_str[] ICACHE_RODATA_ATTR STORE_ATTR = fmt;	\
	printf(flash_str, ##__VA_ARGS__);	\
	} while(0)
#else
#ifndef __ets__
//#define httpd_printf(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define httpd_printf(fmt, ...) DiagPrintf(fmt, ##__VA_ARGS__) // rtl_printf(fmt, ##__VA_ARGS__)
#define strcasecmp(a, b) _stricmp(a, b)
#else
#define httpd_printf(fmt, ...) os_printf(fmt, ##__VA_ARGS__)
#endif
#endif
#else
#define printf(...) os_printf(__VA_ARGS__)
#define sprintf(str, ...) os_sprintf(str, __VA_ARGS__)
#define strcpy(a, b) os_strcpy(a, b)
#define strncpy(a, b, c) os_strncpy(a, b, c)
#define strcmp(a, b) os_strcmp(a, b)
#define strcasecmp(a, b) _stricmp(a, b, c)
#define strncmp(a, b, c) os_strncmp(a, b, c)
#define malloc(x) os_malloc(x)
#define free(x) os_free(x)
#define memset(x, a, b) os_memset(x, a, b)
#define memcpy(x, a, b) os_memcpy(x, a, b)
#define strcat(a, b) os_strcat(a, b)
#define strstr(a, b) os_strstr(a, b)
#define strlen(a) os_strlen(a)
#define memcmp(a, b, c) os_memcmp(a, b, c)
typedef struct espconn* ConnTypePtr;
#define httpd_printf(format, ...) os_printf(format, ##__VA_ARGS__)
#endif

#include "FreeRTOS.h"
#include <osdep_service.h>

#endif
