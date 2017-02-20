#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "objects.h"
#include "flash_api.h"
#include "osdep_service.h"
#include "device_lock.h"
#include "semphr.h"

#include "main.h"
//#include "main_test.h"
#if CONFIG_WLAN
#include "wifi_conf.h"
#include "wlan_intf.h"
#include "wifi_constants.h"
#include "wifi_lib.h"


#include <wlan/wlan_test_inc.h>
#include <wifi/wifi_conf.h>
#include <wifi/wifi_util.h>
#endif
#include "lwip_netconf.h"
#include <platform/platform_stdlib.h>
#include <dhcp/dhcps.h>
#include <lwip_netconf.h>
#include "tcpip.h"

#include "tcm_heap.h"

//#include "mDNS/mDNS.h"
//#include "mdns/example_mdns.h"


#include "soc_rtl8710_httpd_func.h"
//
#include "platform.h"
#include "httpd.h"
//#include "io.h"
#include "httpdespfs.h"

//#include "cgi.h"
//#include "cgiwifi.h"

#include "auth.h"

#include "captdns.h"
#include "webpages-espfs.h"

#include "cgiwebsocket.h"
#include "cgiflash_rtl.h"
//#include "cgi-test.h"





#ifndef CONFIG_INIT_NET
#define CONFIG_INIT_NET             1
#endif
#ifndef CONFIG_INTERACTIVE_MODE
#define CONFIG_INTERACTIVE_MODE     1
#endif

#include "espfsformat.h"
#include "espfs.h"


#include "hal_crypto.h"

#if CONFIG_LWIP_LAYER
extern struct netif xnetif[NET_IF_NUM];
#include "netbios/netbios.h"
#endif

#define DEBUG_MAIN_LEVEL CONFIG_DEBUG_LOG

rtw_mode_t wifi_mode = RTW_MODE_NONE;


extern init_done_ptr p_wlan_init_done_callback;
static rtw_ap_info_t ap = {0};
static rtw_network_info_t wifi = {
		{0},    // ssid
		{0},    // bssid
		0,      // security
		NULL,   // password
		0,      // password len
		-1      // key id
};
rtw_wifi_setting_t wifi_setting = {RTW_MODE_NONE, {0}, 0, RTW_SECURITY_OPEN, {0}};

_WEAK void connect_start(void)
{
	DiagPrintf("%s\n", __FUNCTION__);
}

_WEAK void connect_close(void)
{
	DiagPrintf("%s\n", __FUNCTION__);
}


// Decide starting flash address for storing application data
// User should pick address carefully to avoid corrupting image section

//#define FLASH_APP_BASE  0xFF000
#define FLASH_APP_BASE  0xd0000

static void init_wifi_struct(void)
{
	memset(wifi.ssid.val, 0, sizeof(wifi.ssid.val));
	memset(wifi.bssid.octet, 0, ETH_ALEN);
	//memset(password, 0, sizeof(password));
	wifi.ssid.len = 0;
	wifi.password = NULL;
	wifi.password_len = 0;
	wifi.key_id = -1;
	memset(ap.ssid.val, 0, sizeof(ap.ssid.val));
	ap.ssid.len = 0;
	ap.password = NULL;
	ap.password_len = 0;
	ap.channel = 1;
}

static void LoadWifiSetting(void)
{
    const char *ifname = WLAN0_NAME;

    if(rltk_wlan_running(WLAN1_IDX))
    {//STA_AP_MODE
    	ifname = WLAN1_NAME;
    }

    wifi_get_setting(ifname, &wifi_setting);
#if  DEBUG_MAIN_LEVEL > 4
    printf("%s: wifi_setting.ssid=%s\n", __FUNCTION__, wifi_setting.ssid);
    printf("%s: wifi_setting.channel=%d\n", __FUNCTION__, wifi_setting.channel);
    printf("%s: wifi_setting.security_type=%d\n", __FUNCTION__, wifi_setting.security_type);
    printf("%s: wifi_setting.password=%s\n", __FUNCTION__, wifi_setting.password);
#endif
}


#define GPIO_LED_PIN       PA_4
// PA4 - RTL00 red led:  1=off, 0=on
static gpio_t gpio_led;

void ioLed(int ena) {
	if (ena)
		gpio_write(&gpio_led, 0);
	else
		gpio_write(&gpio_led, 1);
}

//cause I can't be bothered to write an ioGetLed()
static uint8_t currLedState=0;

//Cgi that turns the LED on or off according to the 'led' param in the POST data
int cgiLed(HttpdConnData *connData) {
	int len;
	char buff[1024];

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	len=httpdFindArg(connData->post->buff, "led", buff, sizeof(buff));
	if (len!=0) {
		currLedState=atoi(buff);
		ioLed(currLedState);
	}

	httpdRedirect(connData, "led.tpl");
	return HTTPD_CGI_DONE;
}



//Template code for the led page.
int ICACHE_FLASH_ATTR tplLed(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return HTTPD_CGI_DONE;

	strcpy(buff, "Unknown");
	if (strcmp(token, "ledstate")==0) {
		//if (currLedState) {
		if (gpio_read(&gpio_led)==0) {
			strcpy(buff, "on");
		} else {
			strcpy(buff, "off");
		}
	}
	httpdSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}


static int hitCounter=0;

//Template code for the counter on the index page.
int tplCounter(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return HTTPD_CGI_DONE;

	if (strcmp(token, "counter")==0) {
		hitCounter++;
		sprintf(buff, "%d", hitCounter);
	}
	httpdSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}


//Broadcast the uptime in seconds every second over connected websockets
static void websocketBcast(void *arg) {
	static int ctr=0;
	char buff[128];

	DiagPrintf("RAM heap\t%d bytes\tTCM heap\t%d bytes\n",
			xPortGetFreeHeapSize(), tcm_heap_freeSpace());
	while(1) {
		ctr++;
		sprintf(buff, "Up for %d minutes %d seconds!\n", ctr/60, ctr%60);
		cgiWebsockBroadcast("/websocket/ws.cgi", buff, strlen(buff), WEBSOCK_FLAG_NONE);

		vTaskDelay(1000/portTICK_RATE_MS);
	}
}

//On reception of a message, send "You sent: " plus whatever the other side sent
static void myWebsocketRecv(Websock *ws, char *data, int len, int flags) {
	int i;
	char buff[128];
	sprintf(buff, "You sent: ");
	for (i=0; i<len; i++) buff[i+10]=data[i];
	buff[i+10]=0;
	cgiWebsocketSend(ws, buff, strlen(buff), WEBSOCK_FLAG_NONE);
}

//Websocket connected. Install reception handler and send welcome message.
static void myWebsocketConnect(Websock *ws) {
	ws->recvCb=myWebsocketRecv;
	cgiWebsocketSend(ws, "Hi, Websocket!", 14, WEBSOCK_FLAG_NONE);
}

//On reception of a message, echo it back verbatim
void myEchoWebsocketRecv(Websock *ws, char *data, int len, int flags) {
	DiagPrintf("EchoWs: echo, len=%d\n", len);
	cgiWebsocketSend(ws, data, len, flags);
}

//Echo websocket connected. Install reception handler.
void myEchoWebsocketConnect(Websock *ws) {
	DiagPrintf("EchoWs: connect\n");
	ws->recvCb=myEchoWebsocketRecv;
}


CgiUploadFlashRtl_t uploadParams = {
	.flash_size=1048576,

};
HttpdBuiltInUrl builtInUrls[]=
{
		{"*", cgiRedirectApClientToHostname, "rtl.nonet"},
		{"/", cgiRedirect, "/index.tpl"},
		{"/index.tpl", cgiEspFsTemplate, tplCounter},

		{"/led.tpl", cgiEspFsTemplate, tplLed},
		{"/led.cgi", cgiLed, NULL},

		{"/websocket/ws.cgi", cgiWebsocket, myWebsocketConnect},
		{"/websocket/echo.cgi", cgiWebsocket, myEchoWebsocketConnect},

		{"/flash/", cgiRedirect, "/flash/index.html"},
		{"/flash/upload", cgiUploadFirmware, &uploadParams},
		{"/flash/reboot", cgiRebootFirmware, NULL},

		{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
		{NULL, NULL, NULL}
};


static void start_httpd(void)
{
	EspFsInitResult e;

	//captdnsInit();

	e=espFsInit((void*)FLASH_APP_BASE);

	httpdInit(builtInUrls, 80);
	if (e==0)
		xTaskCreate(websocketBcast, "wsbcast", 512, NULL, tskIDLE_PRIORITY + 1 + PRIORITIE_OFFSET, NULL);
	else
		DiagPrintf("Espfs not found.\n");

	vTaskDelete(NULL);
}



void start_ap(void)
{
#if CONFIG_LWIP_LAYER
	struct ip_addr ipaddr;
	struct ip_addr netmask;
	struct ip_addr gw;
	struct netif * pnetif = &xnetif[0];
#endif
	int timeout = 10000/200;
	int ret = RTW_SUCCESS;
	DiagPrintf("WLAN_AP_ACTIVATE_\n");

#if CONFIG_LWIP_LAYER
	dhcps_deinit();
	IP4_ADDR(&ipaddr, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
	IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
	IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
	netif_set_addr(pnetif, &ipaddr, &netmask,&gw);
#ifdef CONFIG_DONT_CARE_TP
	pnetif->flags |= NETIF_FLAG_IPSWITCH;
#endif
#endif

	LoadWifiSetting();
	if(wifi_setting.mode != RTW_MODE_STA_AP) {
		wifi_off();
		vTaskDelay(20);
		if (wifi_on(RTW_MODE_AP) < 0)
		{
			DiagPrintf("ERROR: Wifi on failed!\n");
			ret = RTW_ERROR;
			goto exit;
		}
	}
	ap.channel = 1;
	ap.password = "0123456789";
	ap.password_len = strlen(ap.password);
	ap.security_type = RTW_SECURITY_OPEN; //RTW_SECURITY_WPA2_AES_PSK;
	memset(ap.ssid.val, 0, sizeof(ap.ssid.val));
	const char* ssid = "RTL8710";
	memcpy((void*)&ap.ssid.val[0], (void*)ssid, strlen(ssid));
	ap.ssid.len = strlen(ap.ssid.val);

	DiagPrintf("Starting AP ...\n");
	if((ret = wifi_start_ap((char*)ap.ssid.val, ap.security_type, (char*)ap.password, ap.ssid.len, ap.password_len, ap.channel) )< 0)
	{
		DiagPrintf("ERROR: Operation failed!\n");
		goto exit;
	}

	while(1) {
			char essid[33];

			if(wext_get_ssid(WLAN0_NAME, (unsigned char *) essid) > 0) {
				if(strcmp((const char *) essid, (const char *)ap.ssid.val) == 0) {
					DiagPrintf("%s started\n", ap.ssid.val);
					ret = RTW_SUCCESS;
					break;
				}
			}

			if(timeout == 0) {
				DiagPrintf("ERROR: Start AP timeout!\n");
				ret = RTW_TIMEOUT;
				break;
			}
			//vTaskDelay(1 * configTICK_RATE_HZ);
			vTaskDelay(200/portTICK_RATE_MS);
			timeout --;
		}
#if CONFIG_LWIP_LAYER
		//LwIP_UseStaticIP(pnetif);
		dhcps_init(pnetif);
		netbios_init();
#endif
#if  DEBUG_MAIN_LEVEL > 4
		LoadWifiSetting();
#endif
		exit:
#if CONFIG_INIC_CMD_RSP
	inic_c2h_wifi_info("ATWA", ret);
#endif
	init_wifi_struct( );

	if(xTaskCreate(start_httpd, ((const char*)"start_httpd"), 2048, NULL, tskIDLE_PRIORITY + 1 + PRIORITIE_OFFSET, NULL) != pdPASS)
		DiagPrintf("\n\r%s xTaskCreate(init_thread) failed", __FUNCTION__);

}


void main(void)
{
#if  DEBUG_MAIN_LEVEL > 4
	 ConfigDebugErr  = -1;
	 ConfigDebugInfo = ~_DBG_SPI_FLASH_;
	 ConfigDebugWarn = -1;
	 CfgSysDebugErr = -1;
	 CfgSysDebugInfo = -1;
	 CfgSysDebugWarn = -1;
#endif

	if(HalGetCpuClk() != PLATFORM_CLOCK) {
#if 0 // def CONFIG_CPU_CLK
		*((int *)0x40000074) &= ~(1<<17); // 0 - 166666666 Hz, 1 - 83333333 Hz, 2 - 41666666 Hz, 3 - 20833333 Hz, 4 - 10416666 Hz, 5 - 4000000 Hz
		HalCpuClkConfig(CPU_CLOCK_SEL_VALUE); // 0 - 166666666 Hz, 1 - 83333333 Hz, 2 - 41666666 Hz, 3 - 20833333 Hz, 4 - 10416666 Hz, 5 - 4000000 Hz
#else // 200 MHz
		HalCpuClkConfig(4); // 0 - 200000000 Hz, 1 - 10000000 Hz, 2 - 50000000 Hz, 3 - 25000000 Hz, 4 - 12500000 Hz, 5 - 4000000 Hz
		*((int *)0x40000074) |= (1<<17);
#endif
		HAL_LOG_UART_ADAPTER pUartAdapter;
		pUartAdapter.BaudRate = RUART_BAUD_RATE_38400;
		HalLogUartSetBaudRate(&pUartAdapter);
		SystemCoreClockUpdate();
		En32KCalibration();
	}

	if ( rtl_cryptoEngine_init() != 0 )
	{
		DiagPrintf("crypto engine init failed\r\n");
	}

	DiagPrintf("GPIO init\n");
	//HalPinCtrlRtl8195A(UART2,0,0);  // uart2 and pa_4 share the same pin
	// Init LED control pin
	gpio_init(&gpio_led, GPIO_LED_PIN);
	gpio_dir(&gpio_led, PIN_OUTPUT);    // Direction: Output
	gpio_mode(&gpio_led, PullNone);     // No pull
	gpio_write(&gpio_led, 1);           // 1=off, 0=on

	p_wlan_init_done_callback = start_ap;


	//example_mdns();

	/* wlan intialization */
#if defined(CONFIG_WIFI_NORMAL) && defined(CONFIG_NETWORK)
	wlan_network();
//	wifi_on(RTW_MODE_STA_AP);
#endif

	DBG_8195A("Main start\n");
	DiagPrintf("\nCLK CPU\t\t%d Hz\nRAM heap\t%d bytes\nTCM heap\t%d bytes\n",
			HalGetCpuClk(), xPortGetFreeHeapSize(), tcm_heap_freeSpace());

	//Enable Schedule, Start Kernel
#if defined(CONFIG_KERNEL) && !TASK_SCHEDULER_DISABLED
    #ifdef PLATFORM_FREERTOS
    vTaskStartScheduler();
    #endif
#else
    RtlConsolTaskRom(NULL);
#endif

    while(1);

}
