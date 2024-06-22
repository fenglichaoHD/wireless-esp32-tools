#ifndef API_JSON_MODULE_H_GUARD
#define API_JSON_MODULE_H_GUARD

#include "request_runner.h"
#include <cJSON.h>
#include <stdint.h>

typedef struct api_json_req_t {
	cJSON *in;
	cJSON *out;
	union {
		struct {
			uint8_t big_buffer: 1;
			uint8_t reserved: 7;
		};
		uint8_t out_flag;
	};
} api_json_req_t;

typedef struct api_json_module_req_t {
	int (*func)(api_json_req_t *req);
	void *arg; /* request context (=api_json_req) */
} api_json_module_req_t;

typedef struct api_json_module_async_t {
	api_json_module_req_t module;
	req_task_cb_t req_task;
} api_json_module_async_t;


typedef enum api_json_req_status_e {
	API_JSON_OK = 0,
	API_JSON_ASYNC = 1,
	API_JSON_BAD_REQUEST = 2,
	API_JSON_INTERNAL_ERR = 3,
	API_JSON_UNSUPPORTED_CMD = 4,
	API_JSON_PROPERTY_ERR = 5,
	API_JSON_BUSY = 6,
} api_json_req_status_e;

typedef int (*api_json_on_req)(uint16_t cmd, api_json_req_t *req, api_json_module_async_t *rsp);

typedef struct api_json_module_cfg_t {
	api_json_on_req on_req; /* input request callback */
	uint8_t module_id;
} api_json_module_cfg_t;

typedef int (*api_json_init_func)(api_json_module_cfg_t *api_module);

void api_json_module_dump();

int api_json_module_add(api_json_init_func);

#define API_JSON_MODULE_REGISTER(INIT) \
  __attribute__((used, constructor)) void cons_ ## INIT(); \
  void cons_ ## INIT() { api_json_module_add(INIT); }

int api_json_module_call(uint8_t id, uint16_t cmd, api_json_req_t *in, api_json_module_async_t *out);

#endif //API_JSON_MODULE_H_GUARD
