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




inline char __httpd_tolower(const char c)
{
	return c | 0x20;
}

int __httpd_strcasecmp(const char *s1, const char *s2)
{
    const unsigned char *us1 = (const unsigned char *)s1;
    const unsigned char *us2 = (const unsigned char *)s2;

    while (__httpd_tolower(*us1) == __httpd_tolower(*us2)) {
            if (*us1++ == '\0')
                    return (0);
            us2++;
    }
return (__httpd_tolower(*us1) - __httpd_tolower(*us2));
}


/* src_addr and size 4-byte aligned
 * (SpicUserReadRtl8195A - unaligned read is broken)
 */
SpiFlashOpResult spi_flash_read(uint32_t src_addr, uint32_t *des_addr, uint32_t size)
{
#if ESPFS_IN_RAM
	memcpy((void*)des_addr, (void*)src_addr, size);
#else
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_burst_read(&flashobj, src_addr, size, (uint8_t *)des_addr);
	//flash_stream_read(&flashobj, src_addr, size, (uint8_t *)des_addr);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
#endif
	return SPI_FLASH_RESULT_OK;
}




extern rtw_mode_t wifi_mode;
extern struct netif xnetif[NET_IF_NUM];

void wifi_get_ip_info(uint32_t dummy, struct ip_info *info)
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
