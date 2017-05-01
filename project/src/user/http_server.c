#include "cgiflash_rtl.h"
#include "objects.h"
#include "flash_api.h"
#include "osdep_service.h"
#include "device_lock.h"
#include "semphr.h"

#include "main.h"
#include "bitband_io.h"

// espfs
#include "espfsformat.h"
#include "espfs.h"
// librtlhttpd
#include "cgiwebsocket.h"
#include "cgiwifi.h"
#include "cgi-test.h"
#include "httpdespfs.h"
#include "soc_rtl8710_httpd_func.h"
#include "netbios/netbios.h"
#include "captdns.h"

#include "http_server.h"


#define GPIO_LED_PIN       PA_4

CgiUploadFlashRtl_t uploadParams = {
		1048576
};

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
httpd_cgi_state cgiLed(HttpdConnData *connData) {
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
LOCAL int tplLed(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return HTTPD_CGI_DONE;

	strcpy(buff, "Unknown");
	if (strcmp(token, "ledstate")==0) {
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
LOCAL int tplCounter(HttpdConnData *connData, char *token, void **arg) {
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
void websocketBcast(void *arg) {
	static int ctr=0;
	char buff[128];

	debug_printf("RAM heap\t%d bytes\tTCM heap\t%d bytes\n",
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
	debug_printf("EchoWs: echo, len=%d\n", len);
	cgiWebsocketSend(ws, data, len, flags);
}

//Echo websocket connected. Install reception handler.
void myEchoWebsocketConnect(Websock *ws) {
	debug_printf("EchoWs: connect\n");
	ws->recvCb=myEchoWebsocketRecv;
}

HttpdBuiltInUrl builtInUrls[]=
{
		/*{"*", cgiRedirectApClientToHostname, "rtl.nonet"},*/
		//{"/", cgiRedirect, "/index.tpl"},
		ROUTE_REDIRECT("/", "/index.tpl"),
		//{"/index.tpl", cgiEspFsTemplate, tplCounter},
		ROUTE_TPL_FILE("/index.tpl", tplCounter, "/index.tpl"),

		//{"/led.tpl", cgiEspFsTemplate, tplLed},
		ROUTE_TPL_FILE("/led.tpl", tplLed, "/led.tpl"),
		//{"/led.cgi", cgiLed, NULL},
		ROUTE_CGI("/led.cgi", cgiLed),

		ROUTE_REDIRECT("/wifi", "/wifi/wifi.tpl"),
		ROUTE_REDIRECT("/wifi/", "/wifi/wifi.tpl"),
		ROUTE_TPL_FILE("/wifi/wifi.tpl", tplWlan, "/wifi/wifi.tpl"),
		ROUTE_CGI("/wifi/wifiscan.cgi", cgiWiFiScan),

		{"/wifi/connect.cgi", cgiWiFiConnect, NULL},
		{"/wifi/connstatus.cgi", cgiWiFiConnStatus, NULL},
		{"/wifi/setmode.cgi", cgiWiFiSetMode, NULL},


		//{"/websocket/ws.cgi", cgiWebsocket, myWebsocketConnect},
		ROUTE_WS("/websocket/ws.cgi", myWebsocketConnect),
		//{"/websocket/echo.cgi", cgiWebsocket, myEchoWebsocketConnect},
		ROUTE_WS("/websocket/echo.cgi", myEchoWebsocketConnect),

		//{"/flash/", cgiRedirect, "/flash/index.html"},
		ROUTE_REDIRECT("/flash", "/flash/index.html"),
		ROUTE_REDIRECT("/flash/", "/flash/index.html"),
		//{"/flash/upload", cgiUploadFirmware, &uploadParams},
		ROUTE_CGI_ARG("/flash/upload", cgiUploadFirmware, &uploadParams),
		//{"/flash/reboot", cgiRebootFirmware, NULL},
		ROUTE_CGI("/flash/reboot", cgiRebootFirmware),

		//{"/test", cgiRedirect, "/test/index.html"},
		ROUTE_REDIRECT("/test", "/test/index.html"),
		//{"/test/", cgiRedirect, "/test/index.html"},
		ROUTE_REDIRECT("/test/", "/test/index.html"),
		//{"/test/test.cgi", cgiTestbed, NULL},
		ROUTE_CGI("/test/test.cgi", cgiTestbed),

		//{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
		ROUTE_FILE("*", NULL),

		{NULL, NULL, NULL}
};


void user_start(void)
{
	info_printf("GPIO init\n");
	//HalPinCtrlRtl8195A(UART2,0,0);  // uart2 and pa_4 share the same pin
	// Init LED control pin
	gpio_init(&gpio_led, GPIO_LED_PIN);
	gpio_dir(&gpio_led, PIN_OUTPUT);    // Direction: Output
	gpio_mode(&gpio_led, PullNone);     // No pull
	gpio_write(&gpio_led, 1);           // 1=off, 0=on

	EspFsInitResult e = ESPFS_INIT_RESULT_NO_IMAGE;

	//netbios_init();
	captdnsInit();
	vTaskDelay(100);

	debug_printf("[Before espfsInit]: RAM heap\t%d bytes\tTCM heap\t%d bytes\n",
			xPortGetFreeHeapSize(), tcm_heap_freeSpace());
	e=espFsInit((void*)FLASH_APP_BASE);

	debug_printf("[After espfsInit]: RAM heap\t%d bytes\tTCM heap\t%d bytes\n",
			xPortGetFreeHeapSize(), tcm_heap_freeSpace());
	httpdInit(builtInUrls, 80);

	if (e==0)
		xTaskCreate(websocketBcast, "wsbcast", 300, NULL, 3, NULL);
	else
		error_printf("Espfs not found.\n");

	info_printf("[After httpdInit]: RAM heap\t%d bytes\tTCM heap\t%d bytes\n",
			xPortGetFreeHeapSize(), tcm_heap_freeSpace());
}


void user_init_thrd(void) {

	wifi_init();

	/* Initilaize the console stack */
	console_init();

	user_start();

	/* Kill init thread after all init tasks done */
	vTaskDelete(NULL);
}


