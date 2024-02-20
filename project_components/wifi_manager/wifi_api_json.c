#include "wifi_api_json.h"

#include "api_json_module.h"
#include "wifi_api.h"
#include "wifi_json_utils.h"

#include <stdio.h>

static int wifi_api_json_get_ap_info(api_json_req_t *req);

static int wifi_api_json_get_scan(api_json_req_t *req);

static int on_async_call(void *arg)
{
	api_json_module_req_t *req = arg;
	return req->func(req->arg);
}

static int on_json_req(uint16_t cmd, api_json_req_t *req, api_json_module_async_t *async)
{
	wifi_api_json_cmd_t wifi_cmd = cmd;
	switch (wifi_cmd) {
	case WIFI_API_JSON_GET_AP_INFO:
		return wifi_api_json_get_ap_info(req);
	case WIFI_API_JSON_CONNECT:

		break;
	case WIFI_API_JSON_GET_SCAN:
		async->module.func = wifi_api_json_get_scan;
		async->module.arg = req;
		async->req_task.module.helper_cb = on_async_call;
		async->req_task.module.arg = &async->module;
		return API_JSON_ASYNC;
	case UNKNOWN:
	default:
		break;
	}

	printf("cmd %d\n", cmd);

	return 0;
}

static int wifi_api_json_get_ap_info(api_json_req_t *req)
{
	wifi_api_ap_info_t ap_info;
	wifi_api_get_ap_info(&ap_info);
	req->out = wifi_api_json_serialize_ap_info(&ap_info);
	return 0;
}

static int wifi_api_json_get_scan(api_json_req_t *req)
{
	wifi_api_ap_info_t ap_info[20];
	uint16_t max_count = 20;
	int err;

	printf("get scan\n");

	err = wifi_api_get_scan_list(&max_count, ap_info);
	if (err == ESP_ERR_NOT_FINISHED) {
		req->out = wifi_api_json_create_err_rsp(req->in, "Wi-Fi scan busy");
		return 1;
	}

	printf("scan ok\n");

	req->out = wifi_api_json_serialize_scan_list(ap_info, max_count);
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


