#ifndef API_JSON_ROUTER_H_GUARD
#define API_JSON_ROUTER_H_GUARD

#include "api_json_module.h"

int api_json_router_init();

int api_json_route(api_json_req_t *req, api_json_resp_t *out);

#endif //API_JSON_ROUTER_H_GUARD