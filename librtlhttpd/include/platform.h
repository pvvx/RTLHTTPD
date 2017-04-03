#ifndef PLATFORM_H
#define PLATFORM_H

#include "user_config.h"

#define ICACHE_FLASH_ATTR

typedef struct RtosConnType RtosConnType;
typedef RtosConnType* ConnTypePtr;

#include "platform_autoconf.h"
#include "autoconf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "soc_rtl8710_httpd_func.h"
#include "rtl8195a.h"
#include "logging.h"
#include "rtl8195a/rtl_libc.h"

#define httpd_strcasecmp(x1, x2) __httpd_strcasecmp(x1, x2)

#endif
