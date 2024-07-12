#ifndef WIFI_JSON_UTILS_H_GUARD
#define WIFI_JSON_UTILS_H_GUARD

#include "wifi_api.h"

cJSON *wifi_api_json_serialize_ap_info(wifi_api_ap_info_t *ap_info, wifi_api_json_cmd_t cmd);
cJSON *wifi_api_json_serialize_scan_list(wifi_api_ap_info_t *aps_info, uint16_t count);
cJSON *wifi_api_json_serialize_ap_auto(wifi_apsta_mode_e mode, int ap_on_delay, int ap_off_delay);
cJSON *wifi_api_json_serialize_get_mode(wifi_apsta_mode_e mode, int status, int ap_on_delay, int ap_off_delay);


cJSON *wifi_api_json_create_err_rsp(cJSON *req, const char *msg);

cJSON *wifi_api_json_add_int_item(cJSON *root, const char *name, int item);
int wifi_api_json_utils_get_int(cJSON *req, const char *name, int *out_value);

#endif //WIFI_JSON_UTILS_H_GUARD