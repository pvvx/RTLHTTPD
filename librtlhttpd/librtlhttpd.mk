
#httpd options
GZIP_COMPRESSION ?= no
COMPRESS_W_YUI ?= no
YUI-COMPRESSOR ?= /usr/bin/yui-compressor
USE_HEATSHRINK ?= yes
HTTPD_WEBSOCKETS ?= yes
#USE_OPENSDK ?= no
HTTPD_MAX_CONNECTIONS ?= 5
#For FreeRTOS
HTTPD_STACKSIZE ?= 4096


FREERTOS ?= yes

INCLUDES += librtlhttpd/include librtlhttpd/espfs librtlhttpd/lib/heatshrink

ADD_SRC_C += librtlhttpd/soc/soc_rtl8710_httpd_func.c
ADD_SRC_C += librtlhttpd/espfs/espfs.c librtlhttpd/espfs/heatshrink_decoder.c
#ADD_SRC_C += librtlhttpd/core/sha1.c 
ADD_SRC_C += librtlhttpd/core/base64.c librtlhttpd/core/auth.c
ADD_SRC_C += librtlhttpd/core/httpd-freertos.c librtlhttpd/core/httpd.c
ADD_SRC_C += librtlhttpd/core/httpdespfs.c
ADD_SRC_C += librtlhttpd/core/uptime.c
ADD_SRC_C += librtlhttpd/util/captdns.c
ADD_SRC_C += librtlhttpd/util/cgiwebsocket.c
ADD_SRC_C += librtlhttpd/util/cgiflash_rtl.c


CFLAGS += -DFREERTOS
CFLAGS += -DHTTPD_MAX_CONNECTIONS=$(HTTPD_MAX_CONNECTIONS)
CFLAGS += -DHTTPD_STACKSIZE=$(HTTPD_STACKSIZE)

ifeq ("$(GZIP_COMPRESSION)","yes")
CFLAGS		+= -DGZIP_COMPRESSION
endif

ifeq ("$(USE_HEATSHRINK)","yes")
CFLAGS		+= -DESPFS_HEATSHRINK
endif

ifeq ("$(HTTPD_WEBSOCKETS)","yes")
CFLAGS		+= -DHTTPD_WEBSOCKETS
endif


