/*
Cgi/template routines for the /wifi url.
 */

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


//#include <esp8266.h>
#include "FreeRTOS.h"
#include "wifi_conf.h"
#include "wifi_structures.h"

#include "platform.h"
#include "soc_rtl8710_httpd_func.h"
#include "cgiwifi_rtl.h"


//Enable this to disallow any changes in AP settings
//#define DEMO_MODE

//WiFi access point data
/*
typedef struct {
	char ssid[32];
	char bssid[8];
	int channel;
	char rssi;
	char enc;
} ApData;

//Scan result
typedef struct {
	char scanInProgress; //if 1, don't access the underlying stuff from the webpage.
	ApData **apData;
	int noAps;
} ScanResultData;

//Static scan status storage.
static ScanResultData cgiWifiAps;
 */
extern rtw_mode_t wifi_mode;
rtw_network_info_t wifi;





#define CONNTRY_IDLE 0
#define CONNTRY_WORKING 1
#define CONNTRY_SUCCESS 2
#define CONNTRY_FAIL 3

//Connection result var
static int connTryStatus=CONNTRY_IDLE;
//Temp store for new ap info.
static rtw_network_info_t stconf;

#if 0
//static os_timer_t resetTimer;

//Callback the code calls when a wlan ap scan is done. Basically stores the result in
//the cgiWifiAps struct.
void   wifiScanDoneCb(void *arg, STATUS status) {
	int n;
	struct bss_info *bss_link = (struct bss_info *)arg;
	httpd_printf("wifiScanDoneCb %d\n", status);
	if (status!=OK) {
		cgiWifiAps.scanInProgress=0;
		return;
	}

	//Clear prev ap data if needed.
	if (cgiWifiAps.apData!=NULL) {
		for (n=0; n<cgiWifiAps.noAps; n++) free(cgiWifiAps.apData[n]);
		free(cgiWifiAps.apData);
	}

	//Count amount of access points found.
	n=0;
	while (bss_link != NULL) {
		bss_link = bss_link->next.stqe_next;
		n++;
	}
	//Allocate memory for access point data
	cgiWifiAps.apData=(ApData **)malloc(sizeof(ApData *)*n);
	if (cgiWifiAps.apData==NULL) {
		printf("Out of memory allocating apData\n");
		return;
	}
	cgiWifiAps.noAps=n;
	httpd_printf("Scan done: found %d APs\n", n);

	//Copy access point data to the static struct
	n=0;
	bss_link = (struct bss_info *)arg;
	while (bss_link != NULL) {
		if (n>=cgiWifiAps.noAps) {
			//This means the bss_link changed under our nose. Shouldn't happen!
			//Break because otherwise we will write in unallocated memory.
			httpd_printf("Huh? I have more than the allocated %d aps!\n", cgiWifiAps.noAps);
			break;
		}
		//Save the ap data.
		cgiWifiAps.apData[n]=(ApData *)malloc(sizeof(ApData));
		if (cgiWifiAps.apData[n]==NULL) {
			httpd_printf("Can't allocate mem for ap buff.\n");
			cgiWifiAps.scanInProgress=0;
			return;
		}
		cgiWifiAps.apData[n]->rssi=bss_link->rssi;
		cgiWifiAps.apData[n]->channel=bss_link->channel;
		cgiWifiAps.apData[n]->enc=bss_link->authmode;
		strncpy(cgiWifiAps.apData[n]->ssid, (char*)bss_link->ssid, 32);
		strncpy(cgiWifiAps.apData[n]->bssid, (char*)bss_link->bssid, 6);

		bss_link = bss_link->next.stqe_next;
		n++;
	}
	//We're done.
	cgiWifiAps.scanInProgress=0;
}


//Routine to start a WiFi access point scan.
static void   wifiStartScan() {
	//	int x;
	if (cgiWifiAps.scanInProgress) return;
	cgiWifiAps.scanInProgress=1;
	wifi_station_scan(NULL, wifiScanDoneCb);
}

//This CGI is called from the bit of AJAX-code in wifi.tpl. It will initiate a
//scan for access points and if available will return the result of an earlier scan.
//The result is embedded in a bit of JSON parsed by the javascript in wifi.tpl.
int   cgiWiFiScan(HttpdConnData *connData) {
	int pos=(int)connData->cgiData;
	int len;
	char buff[1024];

	if (!cgiWifiAps.scanInProgress && pos!=0) {
		//Fill in json code for an access point
		if (pos-1<cgiWifiAps.noAps) {
			len=sprintf(buff, "{\"essid\": \"%s\", \"bssid\": \"" MACSTR "\", \"rssi\": \"%d\", \"enc\": \"%d\", \"channel\": \"%d\"}%s\n",
					cgiWifiAps.apData[pos-1]->ssid, MAC2STR(cgiWifiAps.apData[pos-1]->bssid), cgiWifiAps.apData[pos-1]->rssi,
					cgiWifiAps.apData[pos-1]->enc, cgiWifiAps.apData[pos-1]->channel, (pos-1==cgiWifiAps.noAps-1)?"":",");
			httpdSend(connData, buff, len);
		}
		pos++;
		if ((pos-1)>=cgiWifiAps.noAps) {
			len=sprintf(buff, "]\n}\n}\n");
			httpdSend(connData, buff, len);
			//Also start a new scan.
			wifiStartScan();
			return HTTPD_CGI_DONE;
		} else {
			connData->cgiData=(void*)pos;
			return HTTPD_CGI_MORE;
		}
	}

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/json");
	httpdEndHeaders(connData);

	if (cgiWifiAps.scanInProgress==1) {
		//We're still scanning. Tell Javascript code that.
		len=sprintf(buff, "{\n \"result\": { \n\"inProgress\": \"1\"\n }\n}\n");
		httpdSend(connData, buff, len);
		return HTTPD_CGI_DONE;
	} else {
		//We have a scan result. Pass it on.
		len=sprintf(buff, "{\n \"result\": { \n\"inProgress\": \"0\", \n\"APs\": [\n");
		httpdSend(connData, buff, len);
		if (cgiWifiAps.apData==NULL) cgiWifiAps.noAps=0;
		connData->cgiData=(void *)1;
		return HTTPD_CGI_MORE;
	}
}

//Temp store for new ap info.
static struct station_config stconf;

//This routine is ran some time after a connection attempt to an access point. If
//the connect succeeds, this gets the module in STA-only mode.
static void   resetTimerCb(void *arg) {
	int x=wifi_station_get_connect_status();
	if (x==STATION_GOT_IP) {
		//Go to STA mode. This needs a reset, so do that.
		httpd_printf("Got IP. Going into STA mode..\n");
		wifi_set_opmode(1);
		system_restart();
	} else {
		connTryStatus=CONNTRY_FAIL;
		httpd_printf("Connect fail. Not going into STA-only mode.\n");
		//Maybe also pass this through on the webpage?
	}
}



//Actually connect to a station. This routine is timed because I had problems
//with immediate connections earlier. It probably was something else that caused it,
//but I can't be arsed to put the code back :P
static void   reassTimerCb(void *arg) {
	int x;
	httpd_printf("Try to connect to AP....\n");
	wifi_station_disconnect();
	wifi_station_set_config(&stconf);
	wifi_station_connect();
	x=wifi_get_opmode();
	connTryStatus=CONNTRY_WORKING;
	if (x!=1) {
		//Schedule disconnect/connect
		os_timer_disarm(&resetTimer);
		os_timer_setfn(&resetTimer, resetTimerCb, NULL);
		os_timer_arm(&resetTimer, 15000, 0); //time out after 15 secs of trying to connect
	}
}
#endif

httpd_cgi_state cgiWiFiSetApSSID(HttpdConnData *connData) {
	char *buff;

	buff=rtw_zmalloc(128);
		if (buff ==NULL)
			return HTTPD_CGI_DONE;
	int len=httpdFindArg(connData->getArgs, "name", buff, sizeof(buff));
	if (len>0) {
		int i;
		for(i = 0; i < 32; i++) {
			char c = buff[i];
			if (c == 0) break;
			if (c < 32 || c >= 127) buff[i] = '_';
		}
		buff[i] = 0;

		info("Setting SSID to %s", buff);
/*
		struct softap_config wificfg;
		wifi_softap_get_config(&wificfg);
		sprintf((char *) wificfg.ssid, buff);
		wificfg.ssid_len = strlen((char *) wificfg.ssid);
		wifi_softap_set_config(&wificfg);
*/
	}
	vPortFree(buff);

	httpdRedirect(connData, "/wifi");
	return HTTPD_CGI_DONE;
}

//This cgi uses the routines above to connect to a specific access point with the
//given ESSID using the given password.
httpd_cgi_state cgiWiFiConnect(HttpdConnData *connData) {
	char *essid;
	char *passwd;
	//static os_timer_t reassTimer;

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	essid=rtw_zmalloc(128);
	if (essid ==NULL)
		return HTTPD_CGI_DONE;
	passwd=rtw_zmalloc(128);
	if (passwd ==NULL)
		goto cgiWiFiConnect_exit;

	httpdFindArg(connData->post->buff, "essid", essid, 128);
	httpdFindArg(connData->post->buff, "passwd", passwd, 128);

	strncpy((char*)stconf.ssid.val, essid, 32);
	stconf.ssid.val[32]=0x00;
	stconf.ssid.len=strlen(stconf.ssid.val);
	strncpy((char*)stconf.password, passwd, 64);
	stconf.password_len=strlen(passwd);
	httpd_printf("Try to connect to AP %s pw %s\n", essid, passwd);
	/*
	//Schedule disconnect/connect
	os_timer_disarm(&reassTimer);
	os_timer_setfn(&reassTimer, reassTimerCb, NULL);
//Set to 0 if you want to disable the actual reconnecting bit
#ifdef DEMO_MODE
	httpdRedirect(connData, "/wifi");
#else
	os_timer_arm(&reassTimer, 500, 0);
	httpdRedirect(connData, "connecting.html");
#endif
	 */
	httpdRedirect(connData, "connecting.html");

	vPortFree(passwd);

	cgiWiFiConnect_exit:
	vPortFree(essid);
	return HTTPD_CGI_DONE;
}


//This cgi uses the routines above to connect to a specific access point with the
//given ESSID using the given password.
httpd_cgi_state cgiWiFiSetMode(HttpdConnData *connData) {
	int len;
	char *buff;

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}
	buff=rtw_zmalloc(1024);
	if (buff ==NULL)
		return HTTPD_CGI_DONE;

	len=httpdFindArg(connData->getArgs, "mode", buff, sizeof(buff));
	if (len!=0) {
		httpd_printf("cgiWifiSetMode: %s\n", buff);
#ifndef DEMO_MODE
		//wifi_set_opmode(atoi(buff));
		//system_restart();
#endif
	}
	httpdRedirect(connData, "/wifi");
	vPortFree(buff);
	return HTTPD_CGI_DONE;
}


httpd_cgi_state cgiWiFiConnStatus(HttpdConnData *connData) {
	char *buff;
	int len, st;
	struct ip_info info;

	buff=rtw_zmalloc(1024);
	if (buff ==NULL)
		return HTTPD_CGI_DONE;

	st = wifi_is_connected_to_ap();

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/json");
	httpdEndHeaders(connData);
	if (connTryStatus==CONNTRY_IDLE) {
		len=sprintf(buff, "{\n \"status\": \"idle\"\n }\n");
	} else if (connTryStatus==CONNTRY_WORKING || connTryStatus==CONNTRY_SUCCESS) {
		if (st==RTW_SUCCESS) {
			wifi_get_ip_info(0, &info);
			len=sprintf(buff, "{\n \"status\": \"success\",\n \"ip\": \"%d.%d.%d.%d\" }\n", 
					(info.ip.addr>>0)&0xff, (info.ip.addr>>8)&0xff,
					(info.ip.addr>>16)&0xff, (info.ip.addr>>24)&0xff);
			//Reset into AP-only mode sooner.
			//os_timer_disarm(&resetTimer);
			//os_timer_setfn(&resetTimer, resetTimerCb, NULL);
			//os_timer_arm(&resetTimer, 1000, 0);
		} else {
			len=sprintf(buff, "{\n \"status\": \"working\"\n }\n");
		}
	} else {
		len=sprintf(buff, "{\n \"status\": \"fail\"\n }\n");
	}

	httpdSend(connData, buff, len);
	vPortFree(buff);
	return HTTPD_CGI_DONE;
}

#if 0
static void print_scan_result( rtw_scan_result_t* record )
{
	//rtw_scan_result_my_t * record_my = (rtw_scan_result_my_t *)record;
    RTW_API_INFO( ( "%s\t ", ( record->bss_type == RTW_BSS_TYPE_ADHOC ) ? "Adhoc" : "Infra" ) );
    RTW_API_INFO( ( MAC_FMT, MAC_ARG(record->BSSID.octet) ) );
    RTW_API_INFO( ( " %d\t ", record->signal_strength ) );
    RTW_API_INFO( ( " 0x%x\t  ", record->channel) );
    RTW_API_INFO( ( " %d\t  ", record->wps_type ) );
    RTW_API_INFO( ( " 0x%x\t", record->security ) );
    /*RTW_API_INFO( ( "%s\t\t ", ( record_my->security == RTW_SECURITY_OPEN ) ? "Open" :
                                 ( record_my->security == RTW_SECURITY_WEP_PSK ) ? "WEP" :
                                 ( record_my->security == RTW_SECURITY_WPA_TKIP_PSK ) ? "WPA TKIP" :
                                 ( record_my->security == RTW_SECURITY_WPA_AES_PSK ) ? "WPA AES" :
                                 ( record_my->security == RTW_SECURITY_WPA2_AES_PSK ) ? "WPA2 AES" :
                                 ( record_my->security == RTW_SECURITY_WPA2_TKIP_PSK ) ? "WPA2 TKIP" :
                                 ( record_my->security == RTW_SECURITY_WPA2_MIXED_PSK ) ? "WPA2 Mixed" :
                                 ( record_my->security == RTW_SECURITY_WPA_WPA2_MIXED ) ? "WPA/WPA2 AES" :
                                 "Unknown" ) ); */

    RTW_API_INFO( ( " %s ", record->SSID.val ) );
    RTW_API_INFO( ( "\r\n" ) );
}
#endif
static rtw_scan_result_t *scanresult   = NULL;
static volatile uint8_t ap_scan_state =0;
static volatile uint16_t ApNum =0;

static rtw_result_t ap_scan_result_handler( rtw_scan_handler_result_t* malloced_scan_result)
{
	//static int ApNum = 0;
	if (malloced_scan_result->scan_complete != RTW_TRUE) {
		rtw_scan_result_t* record = &malloced_scan_result->ap_details;
		record->SSID.val[record->SSID.len] = 0; /* Ensure the SSID is null terminated */
		//print_scan_result(record);

		if ((malloced_scan_result->user_data) && (ApNum < AP_SCAN_LIST_SIZE)) {
			memcpy((void *)((char *)malloced_scan_result->user_data + ApNum*sizeof(rtw_scan_result_t)),
					(char *)record, sizeof(rtw_scan_result_t));

			++ApNum;
		}
	} else {
		ap_scan_state =0;
	}
	return RTW_SUCCESS;
}

//This CGI is called from the bit of AJAX-code in wifi.tpl. It will initiate a
//scan for access points and if available will return the result of an earlier scan.
//The result is embedded in a bit of JSON parsed by the javascript in wifi.tpl.
httpd_cgi_state cgiWiFiScan(HttpdConnData *connData) {

	int pos=(int)connData->cgiData;
	int len, totlen;
	char *buff;

	buff=rtw_zmalloc(1024);
	if (buff ==NULL)
		return HTTPD_CGI_DONE;

	if (pos ==0) {
		httpdStartResponse(connData, 200);
		httpdHeader(connData, "Content-Type", "text/json");
		httpdEndHeaders(connData);
	}
	if (scanresult == NULL) {
		//Start a new scan.
		scanresult = rtw_zmalloc(AP_SCAN_LIST_SIZE * sizeof(rtw_scan_result_t));   // 32 * 0x40 = 2048bytes
		if (scanresult == NULL)
			goto wifi_scan_exit;

		ap_scan_state = 1;
		ApNum =0;
		wifi_scan_networks(ap_scan_result_handler, (void*)scanresult);
	}

	if (ap_scan_state ==1) {
		//We're still scanning. Tell Javascript code that.
		len=sprintf(buff, "{\n \"result\": { \n\"inProgress\":1\n }\n}\n");
		httpdSend(connData, buff, len);
	}
	else {
		//Scan completed. Send AP list
		if (pos==0) {
			len=sprintf(buff, "{\n \"result\": { \n\"inProgress\":0, \n\"APs\": [\n");
			httpdSend(connData, buff, len);
		}
		while (pos < ApNum) {
			len = sprintf(buff, "{\"essid\": \"%s\", \"bssid\": \"" MAC_FMT "\", \"rssi\": %d, \"enc\": %d, \"channel\": %d }%s\n",
					scanresult[pos].SSID.val, MAC_ARG(scanresult[pos].BSSID.octet),
					scanresult[pos].signal_strength,
					scanresult[pos].security,
					scanresult[pos].channel,
					(pos == ApNum-1)? "":",");
			buff[len]=0x00;
			//rtl_printf("%s\n", buff);
			if (httpdSend(connData, buff, len))
				connData->cgiData = (void*)pos+1;
			vPortFree(buff);
			return HTTPD_CGI_MORE;
		}
		len=sprintf(buff, "]\n}\n}\n");
		httpdSend(connData, buff, len);
		vPortFree(scanresult);
		scanresult = NULL;
	}

	wifi_scan_exit:
	vPortFree(buff);
	return HTTPD_CGI_DONE;
}

//Template code for the WLAN page.
httpd_cgi_state tplWlan(HttpdConnData *connData, char *token, void **arg) {
	char *buff;
	rtw_wifi_setting_t stconf;

	if (token==NULL) return HTTPD_CGI_DONE;

	buff=rtw_zmalloc(1024);
	if (!buff)
		return HTTPD_CGI_DONE;

	wifi_get_setting(WLAN0_NAME, &stconf);

	strcpy(buff, "Unknown");
	if (strcmp(token, "WiFiMode")==0) {
		if (wifi_mode==RTW_MODE_STA) strcpy(buff, "Client");
		if (wifi_mode==RTW_MODE_AP) strcpy(buff, "SoftAP");
		if (wifi_mode==RTW_MODE_STA_AP) strcpy(buff, "STA+AP");
		if (wifi_mode==RTW_MODE_PROMISC) strcpy(buff, "Promisc");
		if (wifi_mode==RTW_MODE_P2P) strcpy(buff, "P2P");
		if (wifi_mode==RTW_MODE_NONE) strcpy(buff, "None");
	} else if (strcmp(token, "currSsid")==0) {
		strcpy(buff, (char*)stconf.ssid);
	} else if (strcmp(token, "WiFiPasswd")==0) {
		strcpy(buff, (char*)stconf.password);
	} else if (strcmp(token, "WiFiapwarn")==0) {
		if ((wifi_mode!=RTW_MODE_STA_AP) && (wifi_mode!=RTW_MODE_STA)) {
			strcpy(buff, "<b>Can't scan in this mode.</b> Click <a href=\"setmode.cgi?mode=3\">here</a> to go to STA+AP mode.");
		} else {
			strcpy(buff, "Click <a href=\"setmode.cgi?mode=2\">here</a> to go to standalone AP mode.");
		}
	}
	httpdSend(connData, buff, -1);
	vPortFree(buff);
	return HTTPD_CGI_DONE;
}

