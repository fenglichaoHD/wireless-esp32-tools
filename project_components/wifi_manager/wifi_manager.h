#ifndef WIFI_MANAGER_H_GUARD
#define WIFI_MANAGER_H_GUARD

#include <esp_wifi_types.h>

void wifi_manager_init();

int wifi_set_ap_cred(const char *ssid, const char *password);

int wifi_set_sta_cred(const char *ssid, const char *password);

void *wifi_manager_get_ap_netif();
void *wifi_manager_get_sta_netif();

typedef void (*wifi_manager_scan_done_cb)(uint16_t ap_found, wifi_ap_record_t *record, void *arg);
int wifi_manager_scan(uint16_t *number, wifi_ap_record_t *records, uint8_t is_async);
int wifi_manager_trigger_scan(wifi_manager_scan_done_cb cb, void *arg);
int wifi_manager_get_scan_list(uint16_t *number, wifi_ap_record_t *aps);



#endif //WIFI_MANAGER_H_GUARD