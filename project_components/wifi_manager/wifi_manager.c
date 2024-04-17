#include "wifi_manager.h"
#include "wifi_configuration.h"
#include "wifi_event_handler.h"
#include "wifi_storage.h"

#include <esp_err.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <lwip/ip4_addr.h>
#include <string.h>
#include <assert.h>

#define TAG __FILENAME__

typedef struct wifi_ctx_t {
	SemaphoreHandle_t lock;
	TaskHandle_t task;
	union {
		struct {
			wifi_ap_record_t *ap;
			wifi_manager_scan_done_cb cb;
			void *arg;
			uint16_t total_aps;
			uint16_t nr_aps_in_channel;
			uint8_t max_ap;
		} scan;
		struct {
			ip_event_got_ip_t *event;
			uint8_t need_unlock; /* used when trigger connection from wifi_manager instead of wifi_api */
		} conn;
	};
	uint8_t is_endless_connect:1;
	uint8_t auto_reconnect:1;
	uint8_t do_fast_connect:1; /* 0 delay connect on boot or just disconnected, else 5 seconds delay from each connection try */
	uint8_t reserved:5;
} wifi_ctx_t;

static esp_netif_t *ap_netif;
static esp_netif_t *sta_netif;

static wifi_ctx_t ctx;

static void set_sta_cred(const char *ssid, const char *password);
static void disconn_handler(void);
static int set_default_sta_cred(void);

void wifi_manager_init(void)
{
	esp_err_t err;
	uint8_t do_connect = 0;

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

	{
		wifi_config_t ap_config = {0};

		strncpy((char *) ap_config.ap.ssid, WIFI_DEFAULT_AP_SSID, 32);
		strncpy((char *) ap_config.ap.password, WIFI_DEFAULT_AP_PASS, 64);
		ap_config.ap.authmode = WIFI_AUTH_WPA2_WPA3_PSK;
		ap_config.ap.max_connection = 4;
		ap_config.ap.channel = 6;
		ap_config.ap.ssid_hidden = 0;
		ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
	}

	if (set_default_sta_cred() == 0) {
		ESP_LOGI(TAG, "STA connect to saved cred");
		do_connect = 1;
		ctx.do_fast_connect = 1;
	}

	/* TODO: Read from nvs */
	esp_netif_ip_info_t ip_info;
	IP4_ADDR_EXPAND(&ip_info.ip, WIFI_DEFAULT_AP_IP);
	IP4_ADDR_EXPAND(&ip_info.gw, WIFI_DEFAULT_AP_GATEWAY);
	IP4_ADDR_EXPAND(&ip_info.netmask, WIFI_DEFAULT_AP_NETMASK);

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
	esp_log_level_set("wifi", ESP_LOG_WARN);

	ctx.lock = xSemaphoreCreateBinary();
	ctx.is_endless_connect = 0;
	ctx.auto_reconnect = 1;
	xSemaphoreGive(ctx.lock);

	wifi_event_set_disco_handler(disconn_handler);
	if (do_connect) {
		disconn_handler();
	}
}

/**
 * @brief called by wifi_event_handler on scan done
 * */
static void wifi_event_scan_channel_done(uint16_t number, wifi_ap_record_t *aps)
{
	ctx.scan.nr_aps_in_channel = number;
	ctx.scan.total_aps += number;
	if (ctx.task) {
		xTaskNotifyGive(ctx.task);
	}
}

static int scan_loop()
{
	uint32_t ret;
	uint16_t number;

	ctx.scan.total_aps = 0;

	for (int scan_channel = 1; scan_channel <= 13; ++scan_channel) {
		number = ctx.scan.max_ap - ctx.scan.total_aps;
		if (wifi_event_trigger_scan(scan_channel,
		                            wifi_event_scan_channel_done, number,
		                            &ctx.scan.ap[ctx.scan.total_aps])) {
			ESP_LOGE(TAG, "trigger scan %d error", scan_channel);
			return 1;
		}
		/* shadow wifi_event_scan_channel_done() called */
		vTaskDelay(pdMS_TO_TICKS(100));
		ret = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));
		if (ret == 0) {
			/* timeout */
			ESP_LOGE(TAG, "scan channel %d timeout", scan_channel);
			return 1;
		}
	}

	return 0;
}

int wifi_manager_get_scan_list(uint16_t *number, wifi_ap_record_t *aps)
{
	int err;
	int broke_endless_connect = 0;
	if (xSemaphoreTake(ctx.lock, pdMS_TO_TICKS(0)) != pdTRUE) {
		if (ctx.is_endless_connect == 0) {
			return 1;
		}
		/* Is in connecting mode */
		ESP_LOGI(TAG, "deleting connecting %p", ctx.task);
		vTaskDelete(ctx.task);
		/* in case lock is released when deleting the task */
		xSemaphoreTake(ctx.lock, pdMS_TO_TICKS(0));
		esp_wifi_disconnect();
		ctx.is_endless_connect = 0;
		broke_endless_connect = 1;

	} else if (ctx.is_endless_connect == 1) {
		ESP_LOGI(TAG, "deleting delay %p", ctx.task);
		vTaskDelete(ctx.task);
		ctx.is_endless_connect = 0;
		broke_endless_connect = 1;
	}

	ctx.scan.ap = aps;
	ctx.scan.max_ap = *number;
	ctx.scan.cb = NULL;
	ctx.task = xTaskGetCurrentTaskHandle();

	err = scan_loop();
	xSemaphoreGive(ctx.lock);
	*number = ctx.scan.total_aps;

	if (broke_endless_connect) {
		if (set_default_sta_cred() == 0) {
			disconn_handler();
		}
	}
	return err;
}

void *wifi_manager_get_ap_netif()
{
	return ap_netif;
}

void *wifi_manager_get_sta_netif()
{
	return sta_netif;
}


static void try_connect_done(void *arg, ip_event_got_ip_t *event)
{
	ctx.conn.event = event;
	if (ctx.task) {
		xTaskNotifyGive(ctx.task);
	}
	if (ctx.conn.need_unlock) {
		ctx.conn.need_unlock = 0;
		xSemaphoreGive(ctx.lock);
	}
}

int set_default_sta_cred()
{
	int err;
	wifi_credential_t credential;
	err = wifi_data_get_last_conn_cred(&credential);
	if (err) {
		return err;
	}

	set_sta_cred(credential.ssid, credential.password);
	return 0;
}

int wifi_manager_connect(const char *ssid, const char *password)
{
	if (xSemaphoreTake(ctx.lock, pdMS_TO_TICKS(0)) != pdTRUE) {
		if (ctx.is_endless_connect == 0) {
			return 1;
		}
		vTaskDelete(ctx.task);
		ctx.is_endless_connect = 0;
	}
	int ret;
	set_sta_cred(ssid, password);

	ctx.task = xTaskGetCurrentTaskHandle();
	ctx.auto_reconnect = 1;

	wifi_event_trigger_connect(2, try_connect_done, NULL);
	ret = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10000));

	xSemaphoreGive(ctx.lock);
	/* check connect done status */
	if (ret == 0 || ctx.conn.event == NULL) {
		ESP_LOGI(TAG, "conn error");
		/* try to connect to last connected sta */
		if (set_default_sta_cred() == 0) {
			disconn_handler();
		}
		return 1;
	}

	/* connection success: overwrite last connected credential */
	wifi_credential_t credential;
	memcpy(credential.ssid, ssid, 32);
	memcpy(credential.password, password, 64);
	ret = wifi_save_ap_credential(&credential);
	if (ret) {
		ESP_LOGE(TAG, "nvs save error: %s", esp_err_to_name(ret));
	}
	return 0;
}

static void set_sta_cred(const char *ssid, const char *password)
{
	wifi_config_t wifi_config = {
		.sta = {
			.threshold.rssi = -80,
			.sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
		},
	};
	memcpy((char *) wifi_config.sta.ssid, ssid, 32);
	memcpy((char *) wifi_config.sta.password, password, 64);

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
}

static void reconnection_task(void *arg)
{
	int err;

	ctx.is_endless_connect = 1;
	ctx.task = xTaskGetCurrentTaskHandle();

	do {
		ESP_LOGI(TAG, "reco task: try connect, task %p", xTaskGetCurrentTaskHandle());
		if (ctx.do_fast_connect) {
			ctx.do_fast_connect = 0;
			err = wifi_event_trigger_connect(3, try_connect_done, NULL);
		} else {
			err = wifi_event_trigger_connect(0, try_connect_done, NULL);
		}

		if (err) {
			ESP_LOGE(TAG, "trigger connect err: %s", esp_err_to_name(err));
			break;
		}
		ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(20000));
		if (ctx.conn.event || ctx.auto_reconnect == 0) {
			/* reconnection successful or stop reconnect */
			break;
		}

		/* long wait to not spam try to reconnect */
		xSemaphoreGive(ctx.lock);
		ESP_LOGI(TAG, "retry connection in 5 seconds");
		vTaskDelay(pdMS_TO_TICKS(5 * 1000));
		if (xSemaphoreTake(ctx.lock, pdMS_TO_TICKS(0)) != pdTRUE) {
			ESP_LOGE(TAG, "reconnection failed");
			break;
		}
	} while (1);

	ctx.is_endless_connect = 0;
	xSemaphoreGive(ctx.lock);
	vTaskDelete(NULL);
}

static void disconn_handler(void)
{
	/* disconnected
	 * 1. WI-FI AP is far away
	 * 2. WI-FI AP or AP device is closed
	 * 3. this device is ejected by AP
	 * */

	if (ctx.auto_reconnect == 0) {
		return;
	}

	if (xSemaphoreTake(ctx.lock, pdMS_TO_TICKS(0)) != pdTRUE) {
		ESP_LOGE(TAG, "disconn_handler failed");
		return;
	}

	ESP_LOGI(TAG, "start reconn task");
	ctx.do_fast_connect = 1;
	xTaskCreate(reconnection_task, "reconn task", 4 * 1024, NULL, 7, NULL);
}

/**
 * @brief kill reconnection task
 * @return
 */
int wifi_manager_disconnect(void)
{
	ctx.auto_reconnect = 0;
	return esp_wifi_disconnect();
}
