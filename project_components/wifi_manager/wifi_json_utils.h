#ifndef WIFI_JSON_UTILS_H_GUARD
#define WIFI_JSON_UTILS_H_GUARD

#include "wifi_api.h"

cJSON *wifi_api_json_serialize_ap_info(wifi_api_ap_info_t *ap_info, wifi_api_json_cmd_t cmd);
cJSON *wifi_api_json_serialize_scan_list(wifi_api_ap_info_t *aps_info, uint16_t count);


cJSON *wifi_api_json_create_err_rsp(cJSON *req, const char *msg);

#endif //WIFI_JSON_UTILS_H_GUARD