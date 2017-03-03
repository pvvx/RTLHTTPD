#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "objects.h"
#include "flash_api.h"
#include "osdep_service.h"
#include "device_lock.h"

#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip_netconf.h"
#include <wifi/wifi_conf.h>
#include <lwip_netconf.h>


//#include "espfs_flash_wrapper.h"
#include "soc_rtl8710_httpd_func.h"



/* src_addr and size 4-byte aligned
 * (SpicUserReadRtl8195A - unaligned read is broken)
 */
SpiFlashOpResult spi_flash_read(uint32_t src_addr, uint32_t *des_addr, uint32_t size)
{
#if	CONFIG_DEBUG_LOG > 4
	DiagPrintf("spi_flash_read(%p, %p, %d)\n", src_addr, des_addr, size);
#endif
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_burst_read(&flashobj, src_addr, size, (uint8_t *)des_addr);
	//flash_stream_read(&flashobj, src_addr, size, (uint8_t *)des_addr);

	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	return SPI_FLASH_RESULT_OK;
}




extern rtw_mode_t wifi_mode;
extern struct netif xnetif[NET_IF_NUM];

wifi_get_ip_info(uint32_t dummy, struct ip_info *info)
{
	u8_t num=0;
	int devnum = 0;
	struct netif * pnetif = &xnetif[0];

	if(wifi_mode == RTW_MODE_STA_AP)
	{
		pnetif = &xnetif[1];
	}
	info->ip.addr = pnetif->ip_addr.addr;
	info->netmask.addr = pnetif->netmask.addr;
	info->gw.addr = pnetif->gw.addr;
}
