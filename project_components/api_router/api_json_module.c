#include "api_json_module.h"

#include <stdint.h>
#include <stdio.h>
#include <esp_compiler.h>

#define TAG __FILE_NAME__

#define API_MODULE_MAX 10

typedef struct api_json_module_t {
	api_json_on_req on_req;
} api_json_module_t;

static api_json_module_t module_arr[API_MODULE_MAX] = {0};
static uint8_t module_count = 0;

void api_json_module_dump()
{
	for (int i = 0; i < API_MODULE_MAX; ++i) {
		printf("%d = %p\n", i, module_arr[i].on_req);
	}
}

int api_json_module_add(api_json_init_func func)
{
	api_json_module_cfg_t api_module;
	int err;

	api_module.module_id = -1;
	api_module.on_req = NULL;

	err = func(&api_module);
	if (err) {
		printf("module %p init failed\n", func);
		return 1;
	}

	if (api_module.module_id >= API_MODULE_MAX) {
		printf("module ID should be smaller than API_MODULE_MAX=%d\n", API_MODULE_MAX);
		return 1;
	}

	if (module_arr[api_module.module_id].on_req != NULL) {
		printf("module ID %d is already in use\n", api_module.module_id);
		return 1;
	}

	module_arr[api_module.module_id].on_req = api_module.on_req;
	module_count++;
	printf("%p is added\n", func);
	return 0;
}

int api_json_module_call(uint8_t id, uint16_t cmd, api_json_req_t *in, api_json_module_async_t *out)
{
	if (unlikely(id >= API_MODULE_MAX || module_arr[id].on_req == NULL)) {
		return API_JSON_BAD_REQUEST;
	}

	return module_arr[id].on_req(cmd, in, out);
}
