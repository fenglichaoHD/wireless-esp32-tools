#include <cJSON.h>

#include "wifi_json_utils.h"
#include "wifi_api.h"

static void wifi_api_json_set_header(cJSON *root, uint16_t cmd)
{
	cJSON_AddNumberToObject(root, "cmd", cmd);
	cJSON_AddNumberToObject(root, "module", WIFI_MODULE_ID);
}

cJSON *wifi_api_json_serialize_ap_info(wifi_api_ap_info_t *ap_info, wifi_api_json_cmd_t cmd)
{
	cJSON *root;

	root = cJSON_CreateObject();

	wifi_api_json_set_header(root, cmd);
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

cJSON *wifi_api_json_serialize_scan_list(wifi_api_ap_info_t *aps_info, uint16_t count)
{
	cJSON *root;
	char mac_str[18];

	root = cJSON_CreateObject();
	wifi_api_json_set_header(root, WIFI_API_JSON_GET_SCAN);
	cJSON *scan_list = cJSON_AddArrayToObject(root, "scan_list");

	for (int i = 0; i < count; ++i) {
		cJSON *ap = cJSON_CreateObject();
		cJSON_AddNumberToObject(ap, "rssi", aps_info[i].rssi);
		cJSON_AddStringToObject(ap, "ssid", aps_info[i].ssid);
		char *m = aps_info[i].mac;
		snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
		         m[0], m[1], m[2], m[3], m[4], m[5]);
		cJSON_AddStringToObject(ap, "mac", mac_str);
		cJSON_AddItemToArray(scan_list, ap);
	}

	return root;
}

cJSON *wifi_api_json_create_err_rsp(cJSON *req, const char *msg)
{
	cJSON *root;

	root = cJSON_Duplicate(req, 1);
	cJSON_AddStringToObject(root, "msg", msg);

	return root;
}
