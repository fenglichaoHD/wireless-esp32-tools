#ifndef WIFI_API_H_GUARD
#define WIFI_API_H_GUARD

#include <lwip/ip4_addr.h>

typedef struct wifi_api_ap_info_t {
	ip4_addr_t ip;
	ip4_addr_t gateway;
	ip4_addr_t netmask;
	char ssid[33];
	char mac[6];
	signed char rssi;
} wifi_api_ap_info_t;

void wifi_api_get_ap_info(wifi_api_ap_info_t *ap_info);


#endif //WIFI_API_H_GUARD