#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "objects.h"
#include "osdep_service.h"
#include "device_lock.h"
#include "semphr.h"
#include "tcm_heap.h"
#include <platform/platform_stdlib.h>
#include "hal_crypto.h"

#include "main.h"
#include "wc_mgr.h"



#ifndef CONFIG_INIT_NET
#define CONFIG_INIT_NET             1
#endif
#ifndef CONFIG_INTERACTIVE_MODE
#define CONFIG_INTERACTIVE_MODE     1
#endif

void main(void)
{
	//#if DEBUG_MAIN_LEVEL > 0
	//	vPortFree(pvPortMalloc(4)); // Init RAM heap
	//	fATST(NULL); // RAM/TCM/Heaps info
	//#endif
#if DEBUG_MAIN_LEVEL > 2
	ConfigDebugErr  = -1;
	ConfigDebugInfo = -1;
	ConfigDebugWarn = -1;
#endif
	//DBG_ERR_MSG_ON(_DBG_TCM_HEAP_) ;
	//DBG_WARN_MSG_ON(_DBG_TCM_HEAP_) ;
	//DBG_INFO_MSG_ON (_DBG_TCM_HEAP_);



#if defined(CONFIG_CPU_CLK)
	HalCpuClkConfig(0); // 0 - 166666666 Hz, 1 - 83333333 Hz, 2 - 41666666 Hz, 3 - 20833333 Hz, 4 - 10416666 Hz, 5 - 4000000 Hz
	HAL_LOG_UART_ADAPTER pUartAdapter;
	pUartAdapter.BaudRate = RUART_BAUD_RATE_38400;
	HalLogUartSetBaudRate(&pUartAdapter);
	SystemCoreClockUpdate();
	En32KCalibration();
#endif
#if DEBUG_MAIN_LEVEL > 1
	DBG_INFO_MSG_ON(_DBG_TCM_HEAP_); // On Debug TCM MEM
#endif

	if ( rtl_cryptoEngine_init() != 0 )
	{
		DiagPrintf("crypto engine init failed\r\n");
	}


	/* wlan intialization */
#if defined(CONFIG_WIFI_NORMAL) && defined(CONFIG_NETWORK)
	//wlan_network();
	xTaskCreate(wc_start, "wc_start", 4096, NULL, tskIDLE_PRIORITY + 1 + PRIORITIE_OFFSET, NULL);
#endif

	DBG_8195A("Main start\n");
	DiagPrintf("\nCLK CPU\t\t%d Hz\nRAM heap\t%d bytes\tTCM heap\t%d bytes\n",
			HalGetCpuClk(), xPortGetFreeHeapSize(), tcm_heap_freeSpace());

	//Enable Schedule, Start Kernel
#if defined(CONFIG_KERNEL) && !TASK_SCHEDULER_DISABLED
#ifdef PLATFORM_FREERTOS
	vTaskStartScheduler();
#endif
#else
	RtlConsolTaskRom(NULL);
#endif

	//    while(1);

}
