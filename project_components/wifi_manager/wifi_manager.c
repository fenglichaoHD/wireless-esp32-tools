#include "wifi_manager.h"
#include "wifi_configuration.h"
#include "wifi_event_handler.h"

#include <esp_err.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>

#include <lwip/ip4_addr.h>
#include <string.h>

#define TAG __FILENAME__

static esp_netif_t *ap_netif;
static esp_netif_t *sta_netif;

void wifi_manager_init(void)
{
	esp_err_t err;

	ap_netif = esp_netif_create_default_wifi_ap();
	assert(ap_netif);
	sta_netif = esp_netif_create_default_wifi_sta();
	assert(sta_netif);

	ESP_ERROR_CHECK(esp_netif_set_hostname(ap_netif, WIFI_DEFAULT_HOSTNAME));
	ESP_ERROR_CHECK(esp_netif_set_hostname(sta_netif, WIFI_DEFAULT_HOSTNAME));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL, NULL));

	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

	wifi_config_t ap_config = {0};
	size_t len;

	len = 32;
	strncpy((char *) ap_config.ap.ssid, WIFI_DEFAULT_AP_SSID, len);
	len = 64;
	strncpy((char *) ap_config.ap.password, WIFI_DEFAULT_AP_PASS, len);
	ap_config.ap.authmode = WIFI_AUTH_WPA2_WPA3_PSK;
	ap_config.ap.max_connection = 4;
	ap_config.ap.channel = 6;
	ap_config.ap.ssid_hidden = 0;
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));

	// TODO: scan once and connect to known wifi

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = WIFI_DEFAULT_STA_SSID,
			.password = WIFI_DEFAULT_STA_PASS,
			.sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

	/* TODO: Read from nvs */
	esp_netif_ip_info_t ip_info;
	IP4_ADDR(&ip_info.ip, 192, 168, 1, 1);
	IP4_ADDR(&ip_info.gw, 192, 168, 1, 1);
	IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

	err = esp_netif_dhcps_stop(ap_netif);
	if (err) {
		ESP_LOGE(TAG, "dhcps stop %s", esp_err_to_name(err));
	}
	err = esp_netif_set_ip_info(ap_netif, &ip_info);
	if (err) {
		ESP_LOGE(TAG, "net if set info %s", esp_err_to_name(err));
	}
	err = esp_netif_dhcps_start(ap_netif);
	if (err) {
		ESP_LOGE(TAG, "dhcps start %s", esp_err_to_name(err));
	}

	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_LOGI(TAG, "wifi started");
}

void *wifi_manager_get_ap_netif()
{
	return ap_netif;
}

void *wifi_manager_get_sta_netif()
{
	return sta_netif;
}


void test()
{
//	esp_netif_get_ip_info()
	wifi_sta_list_t list;
	wifi_ap_config_t ap_conf;
	wifi_ap_record_t ap_info;

	/* ESP_ERR_WIFI_NOT_CONNECT / ESP_ERR_WIFI_CONN */
	esp_wifi_sta_get_ap_info(&ap_info);

//	esp_wifi_ap_get_sta_list()
}
