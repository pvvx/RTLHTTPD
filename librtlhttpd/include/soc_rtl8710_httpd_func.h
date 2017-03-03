/*
 * rtl_flash_wrapper.h
 *
 *  Created on: Jan 31, 2017
 *      Author: user003
 */

#ifndef ESPFS_RTL_FLASH_WRAPPER_H_
#define ESPFS_RTL_FLASH_WRAPPER_H_
//#include "rtl8195a/c_types.h"
#include "lwip/ip_addr.h"

#define ICACHE_FLASH_ATTR

#define SOFTAP_IF -1

struct ip_info {
    struct ip_addr ip;      /**< IP address */
    struct ip_addr netmask; /**< netmask */
    struct ip_addr gw;      /**< gateway */
};

typedef enum {
    SPI_FLASH_RESULT_OK,
    SPI_FLASH_RESULT_ERR,
    SPI_FLASH_RESULT_TIMEOUT
} SpiFlashOpResult;

/*typedef uint32_t uint32;
typedef int32_t  int32;
typedef uint8_t uint8;
typedef int8_t  int8;*/

//#define SPI_FLASH_SEC_SIZE      4096

SpiFlashOpResult spi_flash_read(uint32_t src_addr, uint32_t *des_addr, uint32_t size);

int wifi_get_ip_info(uint32_t dummy, struct ip_info *info);


#endif /* ESPFS_RTL_FLASH_WRAPPER_H_ */
