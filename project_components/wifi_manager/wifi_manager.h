#ifndef WIFI_MANAGER_H_GUARD
#define WIFI_MANAGER_H_GUARD

void wifi_manager_init();

int wifi_set_ap_cred(const char *ssid, const char *password);

int wifi_set_sta_cred(const char *ssid, const char *password);

void *wifi_manager_get_ap_netif();
void *wifi_manager_get_sta_netif();

#endif //WIFI_MANAGER_H_GUARD