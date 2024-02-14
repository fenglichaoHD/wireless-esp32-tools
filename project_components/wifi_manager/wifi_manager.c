#include "wifi_manager.h"
#include "wifi_configuration.h"
#include "wifi_event_handler.h"

#include <esp_err.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <lwip/ip4_addr.h>
#include <string.h>

#define TAG __FILENAME__

typedef struct wifi_scan_ctx_t {
	wifi_ap_record_t *ap;
	wifi_manager_scan_done_cb cb;
	void *arg;
	SemaphoreHandle_t lock;
	TaskHandle_t task;
	uint16_t total_aps;
	uint16_t nr_aps_in_channel;
	uint8_t status;
	uint8_t max_ap;
} wifi_scan_ctx_t;

static esp_netif_t *ap_netif;
static esp_netif_t *sta_netif;

static wifi_scan_ctx_t scan_ctx;

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

	strncpy((char *) ap_config.ap.ssid, WIFI_DEFAULT_AP_SSID, 32);
	strncpy((char *) ap_config.ap.password, WIFI_DEFAULT_AP_PASS, 64);
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

	scan_ctx.lock = xSemaphoreCreateBinary();
	scan_ctx.status = 0;
	xSemaphoreGive(scan_ctx.lock);
}

static void wifi_event_scan_channel_done(uint16_t number, wifi_ap_record_t *aps)
{
	scan_ctx.nr_aps_in_channel = number;
	scan_ctx.total_aps += number;
	if (scan_ctx.task) {
		xTaskNotifyGive(scan_ctx.task);
	}
}

static void scan_loop()
{
	uint32_t ret;
	uint16_t number;

	scan_ctx.total_aps = 0;

	for (int scan_channel = 1; scan_channel <= 13; ++scan_channel) {
		number = scan_ctx.max_ap - scan_ctx.total_aps;
		if (wifi_event_trigger_scan(scan_channel,
		                            wifi_event_scan_channel_done, number,
		                            &scan_ctx.ap[scan_ctx.total_aps])) {
			ESP_LOGE(TAG, "trigger scan %d error", scan_channel);
			goto end;
		}
		/* shadow wifi_event_scan_channel_done() called */
		vTaskDelay(100);

		ret = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));
		if (ret == 0) {
			/* timeout */
			ESP_LOGE(TAG, "scan channel %d timeout", scan_channel);
			goto end;
		}
	}

end:
	if (scan_ctx.cb) {
//	scan_ctx.cb(scan_ctx.total_aps, scan_ctx.ap);
	}
}

static void wifi_manager_scan_task(void *arg)
{
	scan_loop();
	free(scan_ctx.ap);
	xSemaphoreGive(scan_ctx.lock);
	vTaskDelete(NULL);
}

int wifi_manager_trigger_scan(wifi_manager_scan_done_cb cb, void *arg)
{
	if (xSemaphoreTake(scan_ctx.lock, pdMS_TO_TICKS(0)) != pdTRUE) {
		return 1;
	}
	/* TODO: let API allocate ? */
	scan_ctx.ap = malloc(sizeof(wifi_scan_ctx_t) * 32);
	if (scan_ctx.ap == NULL) {
		return 1;
	}

	scan_ctx.cb = cb;
	scan_ctx.arg = arg;
	scan_ctx.max_ap = 32;

	ulTaskNotifyTake(pdTRUE, 0);
	xTaskCreatePinnedToCore(wifi_manager_scan_task, "scan task", 4 * 1024,
							NULL, 7, &scan_ctx.task, 0);
	return 0;
}

int wifi_manager_get_scan_list(uint16_t *number, wifi_ap_record_t *aps)
{
	if (xSemaphoreTake(scan_ctx.lock, pdMS_TO_TICKS(0)) != pdTRUE) {
		return 1;
	}

	scan_ctx.ap = aps;
	scan_ctx.max_ap = *number;
	scan_ctx.cb = NULL;
	scan_ctx.task = xTaskGetCurrentTaskHandle();

	scan_loop();
	xSemaphoreGive(scan_ctx.lock);
	*number = scan_ctx.total_aps;
	return 0;
}

void *wifi_manager_get_ap_netif()
{
	return ap_netif;
}

void *wifi_manager_get_sta_netif()
{
	return sta_netif;
}
