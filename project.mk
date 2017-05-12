#=============================================
# SDK CONFIG
#=============================================
#USE_AT = 1
#USE_FATFS = 1
#USE_SDIOH = 1
#USE_POLARSSL = 1
#USE_P2P_WPS = 1
#USE_GCC_LIB = 1
USE_MBED = 1

ifndef USE_AT
USE_NEWCONSOLE = 1
USE_WIFI_API = 1
endif

#RTOSDIR=freertos_v8.1.2
RTOSDIR=freertos_v9.0.0
LWIPDIR=lwip_v1.4.1

include $(SDK_PATH)sdkset.mk
#CFLAGS += -DDEFAULT_BAUDRATE=1562500
CFLAGS += -DLOGUART_STACK_SIZE=1024
#=============================================
# User Files
#=============================================
INCLUDES += project/inc/user
# bootloader
BOOT_C := project/src/rtl_boot_s.c
# openocd freertos helper
ADD_SRC_C += project/src/FreeRTOS-openocd.c
# user main
ADD_SRC_C += project/src/user/main.c
#ADD_SRC_C += project/src/user/user_init.c
# console
ADD_SRC_C += project/src/console/atcmd_user.c
#ADD_SRC_C += project/src/console/spi_tst.c
#ADD_SRC_C += project/src/console/wlan_tst.c
ADD_SRC_C += project/src/console/wifi_console.c
# httpd
ADD_SRC_C += project/src/user/http_server.c
ADD_SRC_C += project/src/user/cgi-test.c
ADD_SRC_C += project/src/user/cgiwifi_rtl.c
#ADD_SRC_C += project/src/user/wc_mgr.c
# components
include librtlhttpd/librtlhttpd.mk
#=============================================
