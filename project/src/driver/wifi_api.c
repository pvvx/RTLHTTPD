/*
 * wifi_api.c
 *
 *  Created on: 01/2017
 *      Author: pvvx
 */
#include "FreeRTOS.h"
#include <autoconf.h>
#include "main.h"

#include <platform_opts.h>

#if CONFIG_EXAMPLE_WLAN_FAST_CONNECT
#error "Udnef CONFIG_EXAMPLE_WLAN_FAST_CONNECT!"
#endif
#ifndef USE_FLASH_EEP
#error "Define USE_FLASH_EEP!"
#endif

#include "task.h"
#include <platform/platform_stdlib.h>
#include <wifi/wifi_conf.h>
#include "flash_api.h"
#include <lwip_netconf.h>

#include "flash_eep.h"
#include "feep_config.h"

//=========================================
//==== Wlan Config ========================
#define DEF_WIFI_MODE 		RTW_MODE_STA_AP
#define DEF_WIFI_COUNTRY	RTW_COUNTRY_RU
#define DEF_WIFI_TX_PWR		RTW_TX_PWR_PERCENTAGE_100
#define DEF_WIFI_BGN		RTW_NETWORK_BGN		// rtw_network_mode_t
#define DEF_WIFI_ST_SLEEP	0 	// 0 - none, 1 - on
#define USE_NETBIOS			3	// 0 - off, 1 - ST, 2 - AP, 3 - AP+ST
//==== Interface 0 - wlan0 = AP ===========
#define DEF_AP_SSID			"RTL871X"
#define DEF_AP_PASSWORD		"0123456789"
#define DEF_AP_SECURITY		RTW_SECURITY_OPEN
#define DEF_AP_BEACON		100 // 100...6000 ms
#define DEF_AP_CHANNEL		1	// 1..14
#define DEF_AP_CHANNEL		1	// 1..14
#define DEF_AP_DHCP_MODE	1	// =0 dhcp off, =1 - dhcp on
#define DEF_AP_IP 			IP4ADDR(192,168,4,1)
#define DEF_AP_MSK			IP4ADDR(255,255,255,0)
#define DEF_AP_GW			IP4ADDR(192,168,4,1)
#define DEF_AP_DHCP_START	10
#define DEF_AP_DHCP_STOP	15
//==== Interface 1 - wlan1 = ST ===========
#define DEF_ST_SSID			"HOME_AP"
#define DEF_ST_PASSWORD		"0123456789"
#define DEF_ST_SECURITY		RTW_SECURITY_WPA_WPA2_MIXED
#define DEF_ST_BSSID		{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } // If bssid set is not ff.ff.ff.ff.ff.ff,
// station will connect to the router with both ssid[] and bssid[] matched.
#define DEF_ST_CHANNEL		1	// 1..14
#define DEF_ST_AUTORECONNECT   3 // 0 - none, 1..254 - count, 255 - all
#define DEF_ST_RECONNECT_PAUSE 5 // 5 sec
#define DEF_ST_DHCP_MODE	1 	// =0 dhcp off, =1 - dhcp on, =2 Static ip, =3 - auto
#define DEF_ST_IP			IP4ADDR(192,168,1,100)
#define DEF_ST_MSK			IP4ADDR(255,255,255,0)
#define DEF_ST_GW			IP4ADDR(192,168,1,1)
//==== Interface 2 - eth0 =================
#define DEF_EH_DHCP_MODE	1	// =0 dhcp off, =1 - dhcp on
#define DEF_EH_IP 			IP4ADDR(192,168,1,200)
#define DEF_EH_MSK			IP4ADDR(255,255,255,0)
#define DEF_EH_GW			IP4ADDR(192,168,1,1)
//=========================================
//==== FEEP_ID ===========================
#define FEEP_ID_GLOBAL_CFG	0x4347 // id:'GC'
#define FEEP_ID_WIFI_AP_CFG	0x5041 // id:'AP'
#define FEEP_ID_WIFI_ST_CFG	0x5453 // id:'ST'
#define FEEP_ID_AP_DHCP_CFG	0x4144 // id:'DA'
#define FEEP_ID_ST_DHCP_CFG	0x5344 // id:'DS'
#define FEEP_ID_UART_CFG	0x5530 // id:'0U', type: UART_LOG_CONF
#define FEEP_ID_LWIP_CFG	0x4C30 // id:'0L', type: struct atcmd_lwip_conf
#define FEEP_ID_DHCP_CFG	0x4430 // id:'0D', type: struct
//=========================================
#define IW_PASSPHRASE_MAX_SIZE		64
#define NDIS_802_11_LENGTH_SSID		32
#define IP4ADDR(a,b,c,d) (((unsigned int)((d) & 0xff) << 24) | \
                         ((unsigned int)((c) & 0xff) << 16) | \
                         ((unsigned int)((b) & 0xff) << 8)  | \
                          (unsigned int)((a) & 0xff))
//=========================================
//--- Wlan Config struct-------------------
typedef struct _wifi_config {
	unsigned char mode;			// rtw_mode_t
	unsigned char sleep;
	unsigned char country_code;	// rtw_country_code_t
	unsigned char tx_pwr;		// rtw_tx_pwr_percentage_t
	unsigned char bgn;			// 802.11 rtw_network_mode_t
	unsigned char flg;
} WIFI_CONFIG, *PWIFI_CONFIG;
//---- Interface 0 - wlan0 - AP - struct --
typedef struct _softap_config {
	unsigned char ssid[NDIS_802_11_LENGTH_SSID];
	unsigned char password[IW_PASSPHRASE_MAX_SIZE];
	rtw_security_t security_type;
	uint16 beacon_interval;		// Note: support 100 ~ 60000 ms, default 100
	unsigned char channel;		// 1..14
	unsigned char ssid_hidden;	// Note: default 0
} SOFTAP_CONFIG, *PSOFTAP_CONFIG;
//---- Interface 1 - wlan1 - ST - struct -
typedef struct _station_config {
	unsigned char ssid[NDIS_802_11_LENGTH_SSID];
	unsigned char password[IW_PASSPHRASE_MAX_SIZE];
	rtw_security_t security_type;
	unsigned char bssid[6];		// Note: If bssid set is not ff.ff.ff.ff.ff.ff,
	// station will connect to the router with both ssid[] and bssid[] matched.
	unsigned char autoreconnect;	// 0 - none, 1..254 - count, 255 - all
	unsigned char reconnect_pause; // in sec
// rtw_adaptivity_mode_t
} STATION_CONFIG, *PSTATION_CONFIG;
//--- LwIP Config -------------------------
struct lwip_conn_info {
	int32_t role; 				//client, server or seed
	unsigned int protocol; 		//tcp or udp
	unsigned int remote_addr; 	//remote ip
	unsigned int remote_port; 	//remote port
	unsigned int local_addr; 	//locale ip, not used yet
	unsigned int local_port; 	//locale port, not used yet
	unsigned int reserved; 		//reserve for further use
};
//--- DHCP Config -------------------------
typedef struct _dhcp_config {
	unsigned int ip;
	unsigned int mask;
	unsigned int gw;
	unsigned char mode; // =0 dhcp off, =1 - dhcp on, =2 Static ip, =3 - auto
} DHCP_CONFIG, *PDHCP_CONFIG;

//=========================================
//--- Wlan Config Init-------------------
WIFI_CONFIG wifi_cfg = {
		.mode = DEF_WIFI_MODE,		// rtw_mode_t
		.sleep = DEF_WIFI_ST_SLEEP,
		.country_code = DEF_WIFI_COUNTRY,// rtw_country_code_t
		.tx_pwr = DEF_WIFI_TX_PWR,	// rtw_tx_pwr_percentage_t
		.bgn = DEF_WIFI_BGN,		// rtw_network_mode_t
		.flg = 0
};
//---- Interface 0 - wlan0 - AP - init ---
SOFTAP_CONFIG wifi_ap_cfg = {
		.ssid = DEF_AP_SSID,
		.password = DEF_AP_PASSWORD,
		.security_type = DEF_AP_SECURITY,
		.beacon_interval = DEF_AP_BEACON,
		.channel = DEF_AP_CHANNEL,
		.ssid_hidden = 0
};
DHCP_CONFIG wifi_ap_dhcp = {
		.ip = DEF_AP_IP,
		.mask = DEF_AP_MSK,
		.gw = DEF_AP_GW,
		.mode = 2
};
//---- Interface 1 - wlan1 - ST - init ---
STATION_CONFIG wifi_st_cfg = {
		.ssid = DEF_ST_SSID,
		.password = DEF_ST_PASSWORD,
		.bssid = DEF_ST_BSSID,
		.security_type = DEF_ST_SECURITY,
		.autoreconnect = DEF_ST_AUTORECONNECT,
		.reconnect_pause = DEF_ST_RECONNECT_PAUSE
};
DHCP_CONFIG wifi_st_dhcp = {
		.ip = DEF_ST_IP,
		.mask = DEF_ST_MSK,
		.gw = DEF_ST_GW,
		.mode = 3
};
/*
void wifi_start(void) {
	if (flash_read_cfg(NULL, FEEP_ID_WIFI_CFG, sizeof(WIFI_CONFIG))
			== sizeof(WIFI_CONFIG)) {
		flash_read_cfg(&wifi_cfg, FEEP_ID_WIFI_CFG, sizeof(WIFI_CONFIG));
	}
	if (wifi_cfg.mode >= RTW_MODE_MAX)
		wifi_cfg.mode = DEF_WIFI_MODE;
	if (wifi_cfg.mode <= RTW_MODE_STA_AP) {
		if (wifi_cfg.mode & RTW_MODE_AP) {
			if (flash_read_cfg(&wifi_cfg, FEEP_ID_WIFI_AP_CFG,
					sizeof(SOFTAP_CONFIG)) != sizeof(SOFTAP_CONFIG)) {
				wifi_cfg.mode &= (~RTW_MODE_AP);
				wifi_ap_cfg = NULL;
			}
		}
		if (wifi_cfg.mode & RTW_MODE_STA) {
			if (flash_read_cfg(&wifi_cfg, FEEP_ID_WIFI_ST_CFG,
					sizeof(STATION_CONFIG)) != sizeof(STATION_CONFIG)) {
				wifi_cfg.mode &= (~RTW_MODE_STA);
				wifi_st_cfg = NULL;
			}
		}
	}
	// Call back from wlan driver after wlan init done
	p_wlan_init_done_callback = wlan_init_done_callback;

	// Call back from application layer after wifi_connection success
	p_write_reconnect_ptr = wlan_write_reconnect_data_to_flash;

	wifi_on(wifi_cfg.mode);
	if (wifi_cfg.mode <= RTW_MODE_STA_AP && (wifi_cfg.mode & RTW_MODE_STA)) {
		if (wifi_st_cfg->autoconnect)
			wifi_set_autoreconnect(wifi_st_cfg->autoconnect);
	}
}
*/
typedef int (*wlan_init_done_ptr)(void);
typedef int (*write_reconnect_ptr)(uint8_t *data, uint32_t len);
//Function
#if CONFIG_AUTO_RECONNECT
extern void (*p_wlan_autoreconnect_hdl)(rtw_security_t, char*, int, char*, int,
		int);
#endif
extern wlan_init_done_ptr p_wlan_init_done_callback;
extern write_reconnect_ptr p_write_reconnect_ptr;
extern struct netif xnetif[NET_IF_NUM];

int wlan_init_done_callback(void)
{
//	wifi_disable_powersave();
#if CONFIG_AUTO_RECONNECT
//	wifi_set_autoreconnect(1); // (not work if lib_wlan_mp.a ?)
#endif
	info_printf("WiFi Init after %d ms\n", xTaskGetTickCount());
	return 0;
}

rtw_result_t wifi_run_ap(void)
{
	rtw_result_t ret = RTW_NOTAP;
	if(wifi_cfg.mode & RTW_MODE_AP) {
		LwIP_DHCP(1, DHCP_STOP);
		info_printf("Starting AP ...\n");
		ret = wifi_start_ap(
				wifi_ap_cfg.ssid,			//char  *ssid,
				wifi_ap_cfg.security_type,	//rtw_security_t ecurity_type,
				wifi_ap_cfg.password, 		//char *password,
				strlen(wifi_ap_cfg.ssid),			//int ssid_len,
				strlen(wifi_ap_cfg.password), 	//int password_len,
				wifi_ap_cfg.channel);			//int channel
		if(ret != RTW_SUCCESS) {
			error_printf("Start AP failed!\n\n");;
		} else {
			int timeout = 10000/200;
			while(1) {
				char essid[33];
				if(wext_get_ssid(WLAN1_NAME, (unsigned char *) essid) > 0) {
					if(strcmp((const char *) essid, (const char *)wifi_ap_cfg.ssid) == 0) {
						info_printf("AP '%s' started after %d ms\n", wifi_ap_cfg.ssid, xTaskGetTickCount());
#if CONFIG_LWIP_LAYER
						netif_set_addr(&xnetif[1], &wifi_ap_dhcp.ip , &wifi_ap_dhcp.mask, &wifi_ap_dhcp.gw);
#ifdef CONFIG_DONT_CARE_TP
						pnetiff->flags |= NETIF_FLAG_IPSWITCH;
#endif
						dhcps_init(&xnetif[1]);
#endif
						ret = RTW_SUCCESS;
						break;
					}
				}
				if(timeout == 0) {
					error_printf("Start AP timeout!\n");
					ret = RTW_TIMEOUT;
					break;
				}
				vTaskDelay(200/portTICK_RATE_MS);
				timeout --;
			}
		}
	}
	return ret;
}

rtw_result_t wifi_run_st(void)
{
	rtw_result_t ret = RTW_NOTSTA;
	if(wifi_cfg.mode & RTW_MODE_STA) {
#if CONFIG_AUTO_RECONNECT
		p_wlan_autoreconnect_hdl = NULL;
#endif
		LwIP_DHCP(0, DHCP_STOP);
#if CONFIG_AUTO_RECONNECT
		if(wifi_st_cfg.autoreconnect) {
			ret = wifi_config_autoreconnect(1, wifi_st_cfg.autoreconnect, wifi_st_cfg.reconnect_pause);
			if(ret != RTW_SUCCESS)
				warning_printf("ERROR: Operation failed! Error=%d\n", ret);
		}
#endif
		info_printf("Connected to AP\n");
		ret = wifi_connect(wifi_st_cfg.ssid,
				wifi_st_cfg.security_type,
				wifi_st_cfg.password,
				strlen(wifi_st_cfg.ssid),
				strlen(wifi_st_cfg.password),
				-1,	NULL);
		if(ret != RTW_SUCCESS) {
			error_printf("ERROR: Operation failed! Error=%d\n", ret);
		} else {
			// Start DHCPClient
			LwIP_DHCP(0, DHCP_START);
			info_printf("Got IP\n");
	#if CONFIG_WLAN_CONNECT_CB
			//	extern void connect_start(void);
			connect_start();
	#endif
		}
	};
	return ret;
}

void wifi_init_thrd(void)
{
	/* Initilaize the LwIP stack */
	debug_printf("\nLwIP Init\n");
	LwIP_Init();
	if(wifi_cfg.mode != RTW_MODE_NONE) {
		dhcps_deinit(); // stop dhcp server and client
#if CONFIG_WIFI_IND_USE_THREAD
		wifi_manager_init();
#endif
		// Call back from wlan driver after wlan init done
		p_wlan_init_done_callback = wlan_init_done_callback;
		// Call back from application layer after wifi_connection success
//		p_write_reconnect_ptr = wlan_write_reconnect_data_to_flash;
		debug_printf("\nWiFi_on(%d)\n", wifi_cfg.mode);
		//		wifi_on(RTW_MODE_STA_AP); // RTW_MODE_STA); // RTW_MODE_STA_AP);
		if (wifi_on(wifi_cfg.mode) < 0) {
			error_printf("ERROR: Wifi on failed!\n");
			goto exit_fail;
		}
		wifi_run_ap();
		wifi_run_st();
		//	wifi_config_autoreconnect(1,1,1);
#if CONFIG_INTERACTIVE_MODE
		/* Initial uart rx swmaphore*/
		vSemaphoreCreateBinary(uart_rx_interrupt_sema);
		xSemaphoreTake(uart_rx_interrupt_sema, 1/portTICK_RATE_MS);
		start_interactive_mode();
#endif
		httpd_start();
	}
exit_fail:
	/* Initilaize the console stack */
	console_init();
	/* Kill init thread after all init tasks done */
	vTaskDelete(NULL);
}
