#include "api_json_router.h"

#include <stdint.h>
#include <stdio.h>
#include <esp_compiler.h>

int api_json_router_init()
{
	api_json_module_dump();
	return 0;
}

int api_json_route(api_json_req_t *req, api_json_module_async_t *rsp)
{
	uint16_t cmd;
	uint8_t module_id;
	cJSON *cmd_json;
	cJSON *module_json;

	if (unlikely(req == NULL)) {
		return API_JSON_BAD_REQUEST;
	}

	cmd_json = cJSON_GetObjectItem(req->in, "cmd");
	module_json = cJSON_GetObjectItem(req->in, "module");

	if (!cJSON_IsNumber(cmd_json) || !cJSON_IsNumber(module_json)) {
		return API_JSON_BAD_REQUEST;
	}

	cmd = cmd_json->valueint;
	module_id = module_json->valueint;

	printf("cmd %d received\n", cmd);

	return api_json_module_call(module_id, cmd, req, rsp);
}
