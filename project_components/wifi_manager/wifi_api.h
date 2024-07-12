#ifndef WIFI_API_H_GUARD
#define WIFI_API_H_GUARD

#include <lwip/ip4_addr.h>

#define WIFI_MODULE_ID 1

typedef enum wifi_api_json_cmd_t {
	UNKNOWN                       = 0,
	WIFI_API_JSON_STA_GET_AP_INFO = 1,
	WIFI_API_JSON_CONNECT         = 2,
	WIFI_API_JSON_GET_SCAN        = 3,
	WIFI_API_JSON_DISCONNECT      = 4,
	WIFI_API_JSON_AP_GET_INFO     = 5,
	WIFI_API_JSON_GET_MODE        = 6, /* req:{ }, ret:{mode, [delay_off], [delay_on]} */
	WIFI_API_JSON_SET_MODE        = 7, /* req:{mode, [delay_off], [delay_on]} */
	WIFI_API_JSON_SET_AP_CRED     = 8, /* ssid[32] + password[64] */
} wifi_api_json_cmd_t;

typedef struct wifi_api_ap_info_t {
	ip4_addr_t ip;
	ip4_addr_t gateway;
	ip4_addr_t netmask;
	char ssid[32+1];
	char password[64+1];
	char mac[6];
	signed char rssi;
} wifi_api_ap_info_t;

typedef struct wifi_api_ap_scan_info_t {
	ip4_addr_t ip;
	ip4_addr_t gateway;
	ip4_addr_t netmask;
	char ssid[32+1];
	char mac[6];
	signed char rssi;
} wifi_api_ap_scan_info_t;

typedef enum wifi_apsta_mode_e {
	/* permanent */
	WIFI_AP_AUTO_STA_ON = 0,

	WIFI_AP_STA_OFF     = 4, /* 100 */
	WIFI_AP_OFF_STA_ON  = 5, /* 101 */
	WIFI_AP_ON_STA_OFF  = 6, /* 110 */
	WIFI_AP_STA_ON      = 7, /* 111 */

	/* temporary */
	WIFI_AP_STOP        = 8,
	WIFI_AP_START       = 9,
	WIFI_STA_STOP       = 10,
	WIFI_STA_START      = 11,
} wifi_apsta_mode_e;

void wifi_api_sta_get_ap_info(wifi_api_ap_info_t *ap_info);

void wifi_api_ap_get_info(wifi_api_ap_info_t *ap_info);

typedef void (*wifi_api_scan_done_cb)(uint16_t found, wifi_api_ap_scan_info_t *aps, void *arg);

int wifi_api_trigger_scan(uint16_t *max_ap_count, wifi_api_scan_done_cb cb, void *cb_arg);

int wifi_api_get_scan_list(uint16_t *number, wifi_api_ap_scan_info_t *ap_info);

int wifi_api_connect(const char *ssid, const char *password);

int wifi_api_disconnect(void);


#endif //WIFI_API_H_GUARD