#include <cJSON.h>

#include "wifi_json_utils.h"
#include "wifi_api.h"

cJSON *wifi_api_json_serialize_ap_info(wifi_api_ap_info_t *ap_info)
{
	cJSON *root;

	root = cJSON_CreateObject();

	cJSON_AddStringToObject(root, "ip", ip4addr_ntoa(&ap_info->ip));
	cJSON_AddStringToObject(root, "gateway", ip4addr_ntoa(&ap_info->gateway));
	cJSON_AddStringToObject(root, "netmask", ip4addr_ntoa(&ap_info->netmask));
	cJSON_AddNumberToObject(root, "rssi", ap_info->rssi);
	cJSON_AddStringToObject(root, "ssid", ap_info->ssid);

	char mac_str[18];
	char *m = ap_info->mac;
	snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
	         m[0], m[1], m[2], m[3], m[4], m[5]);
	cJSON_AddStringToObject(root, "mac", mac_str);

	return root;
}
