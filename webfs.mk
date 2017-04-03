THISDIR:=$(dir $(abspath $(lastword $(MAKEFILE_LIST))))

include paths.mk

USE_HEATSHRINK ?= yes
GZIP_COMPRESSION ?= no

HTML_PATH ?= html
webpages.espfs: $(HTML_PATH) librtlhttpd/espfs/mkespfsimage/mkespfsimage
	cd html; find . | $(THISDIR)librtlhttpd/espfs/mkespfsimage/mkespfsimage > $(THISDIR)$(BIN_DIR)/webpages.espfs; cd ..

librtlhttpd/espfs/mkespfsimage/mkespfsimage : librtlhttpd/espfs/mkespfsimage/Makefile
	$(MAKE) -C librtlhttpd/espfs/mkespfsimage mkespfsimage USE_HEATSHRINK="$(USE_HEATSHRINK)" GZIP_COMPRESSION="$(GZIP_COMPRESSION)"


clean:
	$(MAKE) -f librtlhttpd/espfs/mkespfsimage/Makefile clean
	rm -rf $(THISDIR)$(BIN_DIR)/webpages.espfs