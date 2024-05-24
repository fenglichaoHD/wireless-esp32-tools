#ifndef WIFI_API_H_GUARD
#define WIFI_API_H_GUARD

#include <lwip/ip4_addr.h>

#define WIFI_MODULE_ID 1

typedef enum wifi_api_json_cmd_t {
	UNKNOWN = 0,
	WIFI_API_JSON_STA_GET_AP_INFO,
	WIFI_API_JSON_CONNECT,
	WIFI_API_JSON_GET_SCAN,
	WIFI_API_JSON_DISCONNECT,
	WIFI_API_JSON_AP_GET_INFO,
} wifi_api_json_cmd_t;

typedef struct wifi_api_ap_info_t {
	ip4_addr_t ip;
	ip4_addr_t gateway;
	ip4_addr_t netmask;
	char ssid[33];
	char mac[6];
	signed char rssi;
} wifi_api_ap_info_t;

void wifi_api_sta_get_ap_info(wifi_api_ap_info_t *ap_info);

void wifi_api_ap_get_info(wifi_api_ap_info_t *ap_info);

typedef void (*wifi_api_scan_done_cb)(uint16_t found, wifi_api_ap_info_t *aps, void *arg);

int wifi_api_trigger_scan(uint16_t *max_ap_count, wifi_api_scan_done_cb cb, void *cb_arg);

int wifi_api_get_scan_list(uint16_t *number, wifi_api_ap_info_t *ap_info);

int wifi_api_connect(const char *ssid, const char *password);

int wifi_api_disconnect(void);


#endif //WIFI_API_H_GUARD