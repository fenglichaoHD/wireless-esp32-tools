#include "wifi_api.h"
#include "wifi_manager.h"
#include <esp_wifi.h>

void wifi_api_get_ap_info(wifi_api_ap_info_t *ap_info)
{
	wifi_ap_record_t ap_record;
	esp_wifi_sta_get_ap_info(&ap_record);
	strncpy(ap_info->ssid, (const char *)ap_record.ssid, sizeof(ap_info->ssid));
	strncpy(ap_info->mac, (const char *)ap_record.bssid, sizeof(ap_info->mac));
	ap_info->rssi = ap_record.rssi;

	esp_netif_t *sta_netif = wifi_manager_get_sta_netif();
	esp_netif_ip_info_t ip_info;
	esp_netif_get_ip_info(sta_netif, &ip_info);
	ip4_addr_set(&ap_info->ip, &ip_info.ip);
	ip4_addr_set(&ap_info->gateway, &ip_info.gw);
	ip4_addr_set(&ap_info->netmask, &ip_info.netmask);
}
