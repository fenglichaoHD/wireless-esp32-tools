#ifndef API_JSON_MODULE_H_GUARD
#define API_JSON_MODULE_H_GUARD

#include "request_runner.h"
#include <cJSON.h>
#include <stdint.h>


typedef struct api_json_req_t {
	cJSON *in;
	cJSON *out;
} api_json_req_t;

typedef struct api_json_module_req_t {
	int (*func)(api_json_req_t *req);
	void *arg; /* request context (=api_json_req) */
} api_json_module_req_t;

typedef struct api_json_module_async_t {
	api_json_module_req_t module;
	req_task_cb_t req_task;
} api_json_module_async_t;

//typedef struct api_json_send_out_t {
//	void (*func)(void *arg);
//	void *arg; /* socket context */
//} api_json_send_out_t;


typedef enum api_json_req_status_e {
	API_JSON_OK = 0,
	API_JSON_ASYNC = 1,
	API_JSON_BAD_REQUEST = 2,
} api_json_req_status_e;

typedef int (*api_json_on_req)(uint16_t cmd, api_json_req_t *req, api_json_module_async_t *rsp);

typedef struct api_json_module_cfg_t {
	api_json_on_req on_req;
	uint8_t module_id;
} api_json_module_cfg_t;

typedef int (*api_json_init_func)(api_json_module_cfg_t *api_module);

void api_json_module_dump();

int api_json_module_add(api_json_init_func);

/**
 * @brief Register a module that will be init with PRI(priority) order.
 */
#define API_JSON_MODULE_REGISTER(PRI, INIT) \
  __attribute__((used, constructor(PRI))) void cons_ ## INIT(); \
  void cons_ ## INIT() { api_json_module_add(INIT); }

int api_json_module_call(uint8_t id, uint16_t cmd, api_json_req_t *in, api_json_module_async_t *out);

#endif //API_JSON_MODULE_H_GUARD
