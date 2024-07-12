#ifndef WIFI_MANAGER_H_GUARD
#define WIFI_MANAGER_H_GUARD

#include <esp_wifi_types.h>
#include "wifi_api.h"
#include "wifi_storage.h"

void wifi_manager_init();


void *wifi_manager_get_ap_netif();
void *wifi_manager_get_sta_netif();

typedef void (*wifi_manager_scan_done_cb)(uint16_t ap_found, wifi_ap_record_t *record, void *arg);
int wifi_manager_get_scan_list(uint16_t *number, wifi_ap_record_t *aps);
int wifi_manager_connect(const char *ssid, const char *password);
int wifi_manager_disconnect(void);
int wifi_manager_change_mode(wifi_apsta_mode_e mode);
int wifi_manager_get_mode(wifi_apsta_mode_e *mode, wifi_mode_t *status);
int wifi_manager_get_ap_auto_delay(int *ap_on_delay, int *ap_off_delay);
int wifi_manager_set_ap_auto_delay(int *ap_on_delay, int *ap_off_delay);
int wifi_manager_set_ap_credential(wifi_credential_t *cred);


#endif //WIFI_MANAGER_H_GUARD