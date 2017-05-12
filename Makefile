include userset.mk

all: ram_all
mp: ram_all_mp

.PHONY: ram_all
ram_all:
	@$(MAKE) -f $(SDK_PATH)sdkbuild.mk 
	@$(MAKE) -f $(SDK_PATH)flasher.mk genbin1 genbin23


.PHONY: ram_all_mp
ram_all_mp:
	@$(MAKE) -f $(SDK_PATH)sdkbuild.mk mp
	@$(MAKE) -f $(SDK_PATH)flasher.mk mp
	
.PHONY: clean  clean_all
clean:
	@$(MAKE) -f $(SDK_PATH)sdkbuild.mk clean
	@$(MAKE) -f webfs.mk clean

clean_all:
	@$(MAKE) -f $(SDK_PATH)sdkbuild.mk clean_all
	@$(MAKE) -f webfs.mk clean
	
.PHONY: flashburn runram reset test readfullflash flashwebfs
flashburn: 
	@$(MAKE) -f $(SDK_PATH)flasher.mk flashburn

flash_OTA:
	@$(MAKE) -f $(SDK_PATH)flasher.mk flash_OTA

flashwebfs:
	@$(MAKE) -f webfs.mk webpages.espfs
	@$(MAKE) -f $(SDK_PATH)flasher.mk flashespfs

.PHONY:	webfs
webfs:
	@$(MAKE) -f webfs.mk webpages.espfs
	
runram: 
	@$(MAKE) --f $(SDK_PATH)flasher.mk runram

reset: 
	@$(MAKE) -f $(SDK_PATH)flasher.mk reset 

readfullflash:
	@$(MAKE) -f $(SDK_PATH)flasher.mk readfullflash 

