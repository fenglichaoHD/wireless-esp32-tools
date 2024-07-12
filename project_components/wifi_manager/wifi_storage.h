#ifndef WIFI_STORAGE_H_GUARD
#define WIFI_STORAGE_H_GUARD

#include <stdint-gcc.h>
#include "wifi_api.h"

typedef struct nvs_wifi_credential_t {
	char ssid[32];
	char password[64];
} wifi_credential_t;

int wifi_data_get_last_conn_cred(wifi_credential_t *ap_credential);

int wifi_save_ap_credential(wifi_credential_t *ap_credential);

int wifi_data_save_wifi_mode(wifi_apsta_mode_e mode);

#endif //WIFI_STORAGE_H_GUARD