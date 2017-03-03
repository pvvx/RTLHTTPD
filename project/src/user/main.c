/*
 *
 */

#include "platform_autoconf.h"
#include "platform_opts.h"
#include "autoconf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "hal_crypto.h"
#include "hal_log_uart.h"
#include "hal_misc.h"
#include "atcmd_user.h"
#include "wc_mgr.h"
#include "diag.h"
//#include "wdt_api.h"
//#include <osdep_service.h>
#include "hal_platform.h"
#include "rtl8195a_sys_on.h"

#ifdef CONFIG_WDG_ON_IDLE
#include "hal_peri_on.h"
#include "rtl8195a_peri_on.h"
#endif

#ifdef CONFIG_DEBUG_LOG
#define DEBUG_MAIN_LEVEL CONFIG_DEBUG_LOG
#else
#define DEBUG_MAIN_LEVEL 0
#endif

#ifndef CONFIG_INIT_NET
#define CONFIG_INIT_NET             1
#endif
#ifndef CONFIG_INTERACTIVE_MODE
#define CONFIG_INTERACTIVE_MODE     1
#endif

extern void wifi_init_thrd();

void main(void)
{
#if DEBUG_MAIN_LEVEL > 3
	 ConfigDebugErr  = -1;
	 ConfigDebugInfo =  ~(_DBG_SPI_FLASH_);//|_DBG_TCM_HEAP_);
	 ConfigDebugWarn = -1;
	 CfgSysDebugErr = -1;
	 CfgSysDebugInfo = -1;
	 CfgSysDebugWarn = -1;
#endif
	 if(HalGetCpuClk() != PLATFORM_CLOCK) {
#if	CPU_CLOCK_SEL_DIV5_3
		// 6 - 200000000 Hz, 7 - 10000000 Hz, 8 - 50000000 Hz, 9 - 25000000 Hz, 10 - 12500000 Hz, 11 - 4000000 Hz
		HalCpuClkConfig(CPU_CLOCK_SEL_VALUE);
		*((int *)(SYSTEM_CTRL_BASE+REG_SYS_SYSPLL_CTRL1)) |= (1<<17); // REG_SYS_SYSPLL_CTRL1 |= BIT_SYS_SYSPLL_DIV5_3
#else
		// 0 - 166666666 Hz, 1 - 83333333 Hz, 2 - 41666666 Hz, 3 - 20833333 Hz, 4 - 10416666 Hz, 5 - 4000000 Hz
		*((int *)(SYSTEM_CTRL_BASE+REG_SYS_SYSPLL_CTRL1)) &= ~(1<<17); // REG_SYS_SYSPLL_CTRL1 &= ~BIT_SYS_SYSPLL_DIV5_3
		HalCpuClkConfig(CPU_CLOCK_SEL_VALUE);
#endif
		HAL_LOG_UART_ADAPTER pUartAdapter;
		pUartAdapter.BaudRate = UART_BAUD_RATE_38400;
		HalLogUartSetBaudRate(&pUartAdapter);
		SystemCoreClockUpdate();
		En32KCalibration();
	}
	vPortFree(pvPortMalloc(4)); // Init RAM heap

#ifdef CONFIG_WDG_ON_IDLE
	HAL_PERI_ON_WRITE32(REG_SOC_FUNC_EN, HAL_PERI_ON_READ32(REG_SOC_FUNC_EN) & 0x1FFFFF);
	WDGInitial(CONFIG_WDG_ON_IDLE * 1000); // 10 s
	WDGStart();
#endif

#if (defined(CONFIG_CRYPTO_STARTUP) && (CONFIG_CRYPTO_STARTUP))
	 if(rtl_cryptoEngine_init() != 0 ) {
		 DBG_8195A("Crypto engine init failed!\n");
	 }
#endif

#if DEBUG_MAIN_LEVEL > 1
	fATST(); // RAM/TCM/Heaps info
#endif

	/* wlan intialization */
#if defined(CONFIG_WIFI_NORMAL) && defined(CONFIG_NETWORK)
//	xTaskCreate(wc_start, "wc_start", 4096, NULL, tskIDLE_PRIORITY + 1 + PRIORITIE_OFFSET, NULL);
	xTaskCreate(wifi_init_thrd, "wc_start", 1024, NULL, tskIDLE_PRIORITY + 1 + PRIORITIE_OFFSET, NULL);
#endif
	/*Enable Schedule, Start Kernel*/
#if defined(CONFIG_KERNEL) && !TASK_SCHEDULER_DISABLED
#ifdef PLATFORM_FREERTOS
	vTaskStartScheduler();
#endif
#else
	RtlConsolTaskRom(NULL);
#endif
}
