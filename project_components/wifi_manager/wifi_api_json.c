#include "wifi_api_json.h"

#include "api_json_router.h"
#include "wifi_api.h"
#include "wifi_json_utils.h"

#include <stdio.h>

static int wifi_api_json_get_ap_info(api_json_req_t *req, api_json_resp_t *resp);

static int wifi_api_json_get_scan(api_json_req_t *req, api_json_resp_t *resp);

static int on_json_req(uint16_t cmd, api_json_req_t *req, api_json_resp_t *rsp)
{
	wifi_api_json_cmd_t wifi_cmd = cmd;
	switch (wifi_cmd) {
	case WIFI_API_JSON_GET_AP_INFO:
		return wifi_api_json_get_ap_info(req, rsp);
	case WIFI_API_JSON_CONNECT:

		break;
	case WIFI_API_JSON_GET_SCAN:
		return wifi_api_json_get_scan(req, rsp);
	case UNKNOWN:
	default:
		break;
	}

	printf("%d\n", cmd);

	return 0;
}

static int wifi_api_json_get_ap_info(api_json_req_t *req, api_json_resp_t *resp)
{
	wifi_api_ap_info_t ap_info;
	wifi_api_get_ap_info(&ap_info);
	resp->json = wifi_api_json_serialize_ap_info(&ap_info);
	return 0;
}

static int wifi_api_json_get_scan(api_json_req_t *req, api_json_resp_t *resp)
{
	wifi_api_ap_info_t ap_info[20];
	uint16_t max_count = 20;
	int err;

	err = wifi_api_get_scan_list(&max_count, ap_info);
	if (err == ESP_ERR_NOT_FINISHED) {
		resp->json = wifi_api_json_create_err_rsp(req->json, "Wi-Fi scan busy");
		return 1;
	}

	resp->json = wifi_api_json_serialize_scan_list(ap_info, max_count);
	return 0;
}


/* ****
 *  register module
 * */

static int wifi_api_json_init(api_json_module_cfg_t *cfg)
{
	cfg->on_req = on_json_req;
	cfg->module_id = 1;
	return 0;
}

API_JSON_MODULE_REGISTER(0x90, wifi_api_json_init);


