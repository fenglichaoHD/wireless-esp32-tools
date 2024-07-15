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
	cJSON_AddStringToObject(root, "dns_main", ip4addr_ntoa(&ap_info->dns_main));
	cJSON_AddStringToObject(root, "dns_backup", ip4addr_ntoa(&ap_info->dns_backup));
	cJSON_AddNumberToObject(root, "rssi", ap_info->rssi);
	cJSON_AddStringToObject(root, "ssid", ap_info->ssid);
	if (ap_info->password[0]) {
		cJSON_AddStringToObject(root, "password", ap_info->password);
	}

	char mac_str[18];
	char *m = ap_info->mac;
	snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
	         m[0], m[1], m[2], m[3], m[4], m[5]);
	cJSON_AddStringToObject(root, "mac", mac_str);

	return root;
}

cJSON *wifi_api_json_serialize_scan_list(wifi_api_ap_scan_info_t *aps_info, uint16_t count)
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

cJSON *wifi_api_json_serialize_ap_auto(wifi_apsta_mode_e mode, int ap_on_delay, int ap_off_delay)
{
	cJSON *root;

	root = cJSON_CreateObject();
	wifi_api_json_set_header(root, WIFI_API_JSON_GET_SCAN);
	cJSON_AddNumberToObject(root, "mode", mode);
	cJSON_AddNumberToObject(root, "ap_on_delay", ap_on_delay);
	cJSON_AddNumberToObject(root, "ap_off_delay", ap_off_delay);

	return root;
}

cJSON *wifi_api_json_create_err_rsp(cJSON *req, const char *msg)
{
	cJSON *root;

	root = cJSON_Duplicate(req, 1);
	cJSON_AddStringToObject(root, "err", msg);

	return root;
}

int wifi_api_json_utils_get_int(cJSON *req, const char *name, int *out_value)
{
	cJSON *item = cJSON_GetObjectItemCaseSensitive(req, name);

	if (!cJSON_IsNumber(item)) {
		return 1;
	}

	*out_value = item->valueint;
	return 0;
}

cJSON *wifi_api_json_serialize_get_mode(wifi_apsta_mode_e mode, int status, int ap_on_delay, int ap_off_delay)
{
	cJSON *root;

	root = cJSON_CreateObject();
	wifi_api_json_set_header(root, WIFI_API_JSON_GET_MODE);
	cJSON_AddNumberToObject(root, "mode", mode);
	cJSON_AddNumberToObject(root, "status", status);
	if (ap_on_delay >= 0 && ap_off_delay >= 0) {
		cJSON_AddNumberToObject(root, "ap_on_delay", ap_on_delay);
		cJSON_AddNumberToObject(root, "ap_off_delay", ap_off_delay);
	}

	return root;
}

cJSON *wifi_api_json_add_int_item(cJSON *root, const char *name, int item)
{
	cJSON_AddNumberToObject(root, name, item);
	return root;
}

int wifi_api_json_get_credential(cJSON *root, char *ssid, char *password)
{
	cJSON *json_ssid = cJSON_GetObjectItem(root, "ssid");
	cJSON *json_password = cJSON_GetObjectItem(root, "password");

	if (!cJSON_IsString(json_ssid) || !cJSON_IsString(json_password)) {
		return 1;
	}

	strncpy(ssid, cJSON_GetStringValue(json_ssid), 31);
	strncpy(password, cJSON_GetStringValue(json_password), 63);

	ssid[31] = '\0';
	password[64] = '\0';
	return 0;
}

cJSON *wifi_api_json_ser_static_info(wifi_api_sta_ap_static_info_t *info)
{
	cJSON *root;

	root = cJSON_CreateObject();
	wifi_api_json_set_header(root, WIFI_API_JSON_STA_GET_STATIC_INFO);
	cJSON_AddNumberToObject(root, "static_ip_en", info->static_ip_en);
	cJSON_AddNumberToObject(root, "static_dns_en", info->static_dns_en);

	cJSON_AddStringToObject(root, "ip", ip4addr_ntoa(&info->ip));
	cJSON_AddStringToObject(root, "gateway", ip4addr_ntoa(&info->gateway));
	cJSON_AddStringToObject(root, "netmask", ip4addr_ntoa(&info->netmask));
	cJSON_AddStringToObject(root, "dns_main", ip4addr_ntoa(&info->dns_main));
	cJSON_AddStringToObject(root, "dns_backup", ip4addr_ntoa(&info->dns_backup));

	return root;
}

int wifi_api_json_deser_static_conf(cJSON *root, wifi_api_sta_ap_static_info_t *static_info)
{
	cJSON *static_ip_en = cJSON_GetObjectItemCaseSensitive(root, "static_ip_en");
	cJSON *static_dns_en = cJSON_GetObjectItemCaseSensitive(root, "static_dns_en");

	if (!cJSON_IsNumber(static_ip_en) || !cJSON_IsNumber(static_dns_en)) {
		printf("en error\n");
		return 1;
	}

	static_info->static_dns_en = static_dns_en->valueint;
	static_info->static_ip_en = static_ip_en->valueint;

	cJSON *ip = cJSON_GetObjectItemCaseSensitive(root, "ip");
	cJSON *gateway = cJSON_GetObjectItemCaseSensitive(root, "gateway");
	cJSON *netmask = cJSON_GetObjectItemCaseSensitive(root, "netmask");
	cJSON *dns_main = cJSON_GetObjectItemCaseSensitive(root, "dns_main");
	cJSON *dns_backup = cJSON_GetObjectItemCaseSensitive(root, "dns_backup");

	if (cJSON_IsString(ip)) {
		ip4addr_aton(ip->valuestring, &static_info->ip);
	}
	if (cJSON_IsString(gateway)) {
		ip4addr_aton(gateway->valuestring, &static_info->gateway);
	}
	if (cJSON_IsString(netmask)) {
		ip4addr_aton(netmask->valuestring, &static_info->netmask);
	}
	if (cJSON_IsString(dns_main)) {
		ip4addr_aton(dns_main->valuestring, &static_info->dns_main);
	}
	if (cJSON_IsString(dns_backup)) {
		ip4addr_aton(dns_backup->valuestring, &static_info->dns_backup);
	}
	return 0;
}
