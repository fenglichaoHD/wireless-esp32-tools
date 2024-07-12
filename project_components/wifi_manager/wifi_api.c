#include "wifi_api.h"
#include "wifi_manager.h"
#include "wifi_configuration.h"
#include "wifi_storage.h"
#include <esp_wifi.h>

void wifi_api_sta_get_ap_info(wifi_api_ap_info_t *ap_info)
{
	wifi_ap_record_t ap_record;
	esp_wifi_sta_get_ap_info(&ap_record);
	strncpy(ap_info->ssid, (char *)ap_record.ssid, sizeof(ap_info->ssid));
	strncpy(ap_info->mac, (char *)ap_record.bssid, sizeof(ap_info->mac));
	ap_info->rssi = ap_record.rssi;
	ap_info->password[0] = '\0';

	esp_netif_t *sta_netif = wifi_manager_get_sta_netif();
	esp_netif_ip_info_t ip_info;
	esp_netif_get_ip_info(sta_netif, &ip_info);
	ip4_addr_set(&ap_info->ip, &ip_info.ip);
	ip4_addr_set(&ap_info->gateway, &ip_info.gw);
	ip4_addr_set(&ap_info->netmask, &ip_info.netmask);
}

void wifi_api_ap_get_info(wifi_api_ap_info_t *ap_info)
{
	wifi_credential_t credential;
	int err = wifi_data_get_ap_credential(&credential);
	if (err) {
		strncpy(ap_info->ssid, WIFI_DEFAULT_AP_SSID, strlen(WIFI_DEFAULT_AP_SSID)+1);
		strncpy(ap_info->password, WIFI_DEFAULT_AP_PASS, strlen(WIFI_DEFAULT_AP_PASS)+1);
	} else {
		strncpy(ap_info->ssid, credential.ssid, sizeof(ap_info->ssid));
		strncpy(ap_info->password, credential.password, sizeof(ap_info->password));
	}
	esp_wifi_get_mac(WIFI_IF_AP, (uint8_t *) &ap_info->mac);
	IP4_ADDR_EXPAND(&ap_info->ip, WIFI_DEFAULT_AP_IP);
	IP4_ADDR_EXPAND(&ap_info->gateway, WIFI_DEFAULT_AP_GATEWAY);
	IP4_ADDR_EXPAND(&ap_info->netmask, WIFI_DEFAULT_AP_NETMASK);
	ap_info->rssi = 0;
}

static int rssi_comp(const void *a, const void *b)
{
	const wifi_api_ap_scan_info_t *r1 = a;
	const wifi_api_ap_scan_info_t *r2 = b;
	return r2->rssi - r1->rssi;
}

/**
 * @brief blocking function
 */
int wifi_api_get_scan_list(uint16_t *number, wifi_api_ap_scan_info_t *ap_info)
{
	wifi_ap_record_t *records;
	int err;

	records = malloc(*number * sizeof(wifi_ap_record_t));
	if (records == NULL) {
		return ESP_ERR_NO_MEM;
	}

	err = wifi_manager_get_scan_list(number, records);
	if (err) {
		printf("get scan list err\n");
		free(records);
		return err;
	}

	for (int i = 0; i < *number; ++i) {
		printf("%d %d %s\n", i, records[i].rssi, records[i].ssid);
		strncpy(ap_info[i].ssid, (char *) records[i].ssid, sizeof(ap_info[i].ssid));
		strncpy(ap_info[i].mac, (char *) records[i].bssid, sizeof(ap_info[i].mac));
		ap_info[i].rssi = records[i].rssi;
	}
	free(records);

	qsort(ap_info, *number, sizeof(wifi_api_ap_scan_info_t), rssi_comp);
	return 0;
}

static wifi_api_scan_done_cb scan_done_cb;

static void wifi_manager_scan_done(uint16_t ap_found, wifi_ap_record_t *records, void *arg)
{
	wifi_api_ap_scan_info_t *ap_info;

	ap_info = malloc(ap_found * sizeof(wifi_api_ap_scan_info_t));
	for (int i = 0; i < ap_found; ++i) {
		strncpy(ap_info[i].ssid, (char *) records[i].ssid, sizeof(ap_info[i].ssid));
		strncpy(ap_info[i].mac, (char *) records[i].bssid, sizeof(ap_info[i].mac));
		ap_info[i].rssi = records[i].rssi;
	}
	printf("wifi api scan done\n");
	scan_done_cb(ap_found, ap_info, arg);
}

int wifi_api_connect(const char *ssid, const char *password)
{
	return wifi_manager_connect(ssid, password);
}

int wifi_api_disconnect(void)
{
	return wifi_manager_disconnect();
}
