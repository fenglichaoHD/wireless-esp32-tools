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
#include <hal/gpio_types.h>
#include <driver/ledc.h>
#include <hal/gpio_hal.h>
#include <soc/ledc_periph.h>

#include "ssdp.h"
#include "wifi_api.h"

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
	struct {
		uint8_t is_endless_connect: 1;
		uint8_t auto_reconnect: 1;
		uint8_t do_fast_connect: 1; /* 0 delay connect on boot or just disconnected, else 5 seconds delay from each connection try */
		uint8_t is_sta_connected: 1;
		uint8_t reserved: 4;
	};
	TaskHandle_t delayed_stopAP_task;
	TaskHandle_t delayed_startAP_task;
	uint32_t try_connect_count;
	wifi_apsta_mode_e permanent_mode;
	wifi_mode_t mode;
	int ap_on_delay_tick;
	int ap_off_delay_tick;
} wifi_ctx_t;

static esp_netif_t *ap_netif;
static esp_netif_t *sta_netif;

static wifi_ctx_t ctx;

static void set_sta_cred(const char *ssid, const char *password);
static void disconn_handler(void);
static int set_default_sta_cred(void);
static int set_wifi_mode(wifi_apsta_mode_e mode);
static void handle_wifi_connected(); /* got IP */

static void wifi_led_init();
static void wifi_led_set_blink();
static void wifi_led_set_on();

void wifi_manager_init(void)
{
	esp_err_t err;
	uint8_t do_connect = 0;
	wifi_led_init();
	wifi_led_set_blink();

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

	ctx.is_sta_connected = false;
	ctx.ap_on_delay_tick = pdMS_TO_TICKS(5000);
	ctx.ap_off_delay_tick = pdMS_TO_TICKS(10000);
	ctx.delayed_stopAP_task = NULL;
	ctx.delayed_startAP_task = NULL;
	ctx.try_connect_count = 0;

	err = wifi_data_get_wifi_mode(&ctx.permanent_mode);
	ESP_LOGI(TAG, "use wifi mode: %d", ctx.permanent_mode);
	if (err) {
		ctx.permanent_mode = WIFI_AP_AUTO_STA_ON;
		ctx.mode = WIFI_MODE_APSTA;
	} else {
		ctx.mode = ctx.permanent_mode & 0x3; /* 0b1XX */
		if (ctx.mode == WIFI_MODE_STA || ctx.mode == WIFI_MODE_NULL) {
			ctx.mode = WIFI_MODE_APSTA;
		}
	}

	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(ctx.mode));
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

	wifi_credential_t cred;
	err = wifi_data_get_ap_credential(&cred);
	if (err || strlen(cred.password) < 8) {
		ESP_LOGI(TAG, "used default AP credential");
		strncpy((char *)cred.ssid, WIFI_DEFAULT_AP_SSID, 32);
		strncpy((char *)cred.password, WIFI_DEFAULT_AP_PASS, 64);
	}
	wifi_manager_set_ap_credential(&cred);

	if (set_default_sta_cred() == 0) {
		ESP_LOGI(TAG, "STA connect to saved cred");
		do_connect = 1;
		ctx.do_fast_connect = 1;
	}

	wifi_api_sta_ap_static_info_t static_info;
	err = wifi_data_get_static(&static_info);
	if (err == ESP_OK) {
		wifi_manager_sta_set_static_conf(&static_info);
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

	ssdp_init();
	ssdp_start();
}

void wifi_led_init()
{
#if WIFI_LED_ENABLE
	ledc_timer_config_t ledc_timer = {
		.duty_resolution = LEDC_TIMER_14_BIT, // resolution of PWM duty
		.freq_hz = 5,                      // frequency of PWM signal
		.speed_mode = LEDC_LOW_SPEED_MODE,           // timer mode
		.timer_num = LEDC_TIMER_0,            // timer index
		.clk_cfg = LEDC_USE_APB_CLK,              // Auto select the source clock
	};
	ledc_timer_config(&ledc_timer);

	ledc_channel_config_t ledc_channel = {
		.channel    = LEDC_CHANNEL_0,
		.duty       = 0,
		.gpio_num   = WIFI_LED_PIN,
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.hpoint     = 1,
		.timer_sel  = LEDC_TIMER_0,
		.flags.output_invert = 0
	};

	ledc_channel_config(&ledc_channel);
	ledc_fade_func_install(0);

	gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[WIFI_LED_PIN], PIN_FUNC_GPIO);
	gpio_set_direction(WIFI_LED_PIN, GPIO_MODE_OUTPUT_OD);
	gpio_set_direction(WIFI_LED_PIN, GPIO_FLOATING);
	esp_rom_gpio_connect_out_signal(WIFI_LED_PIN, ledc_periph_signal[LEDC_LOW_SPEED_MODE].sig_out0_idx + LEDC_CHANNEL_0,
	                                1, 0);
#endif
}


static void wifi_led_set_blink()
{
#if WIFI_LED_ENABLE
	ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (1 << 12), 0);
#endif
}


static void wifi_led_set_on()
{
#if WIFI_LED_ENABLE
	ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (1 << 14) - 1, 0);
#endif
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
	err = wifi_data_get_sta_last_conn_cred(&credential);
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
	wifi_led_set_on();
	handle_wifi_connected();
	wifi_credential_t credential;
	memcpy(credential.ssid, ssid, 32);
	memcpy(credential.password, password, 64);
	ret = wifi_data_save_sta_ap_credential(&credential);
	if (ret) {
		ESP_LOGE(TAG, "nvs save error: %s", esp_err_to_name(ret));
	}
	return 0;
}

int wifi_manager_change_mode(wifi_apsta_mode_e mode)
{
	int err;
	if (ctx.permanent_mode == mode) {
		return 0;
	}
	err = xSemaphoreTake(ctx.lock, pdMS_TO_TICKS(2000));
	if (err != pdTRUE) {
		ESP_LOGE(TAG, "set mode take lock");
		return 1;
	}
	if (ctx.delayed_stopAP_task) {
		xTaskNotifyGive(ctx.delayed_stopAP_task);
	}
	if (ctx.delayed_startAP_task) {
		xTaskNotifyGive(ctx.delayed_startAP_task);
	}

	err = set_wifi_mode(mode);
	xSemaphoreGive(ctx.lock);
	if (err)
		return err;
	if (mode >= WIFI_AP_STOP){
		return ESP_OK;
	}
	return wifi_data_save_wifi_mode(mode);
}

int set_wifi_mode(wifi_apsta_mode_e mode)
{
	uint32_t new_mode = ctx.mode;
	printf("change mode: %d\n", mode);

	switch (mode) {
	default:
		return 1;
	case WIFI_AP_AUTO_STA_ON:
		if (ctx.is_sta_connected) {
			new_mode = WIFI_MODE_STA;
		} else {
			new_mode = WIFI_MODE_APSTA;
		}
		ctx.permanent_mode = mode;
		break;
	case WIFI_AP_STA_OFF:
	case WIFI_AP_ON_STA_OFF:
	case WIFI_AP_OFF_STA_ON:
	case WIFI_AP_STA_ON:
		ctx.permanent_mode = mode;
		new_mode = mode & (~WIFI_AP_STA_OFF);
		break;

	case WIFI_AP_STOP:
		new_mode &= ~WIFI_MODE_AP;
		break;
	case WIFI_AP_START:
		new_mode |= WIFI_MODE_AP;
		break;
	case WIFI_STA_STOP:
		new_mode &= ~WIFI_MODE_STA;
		break;
	case WIFI_STA_START:
		new_mode |= WIFI_MODE_STA;
		break;
	}

	printf("set mode to %lx\n", new_mode);
	if (new_mode == ctx.mode) {
		return 0;
	}

	int err = esp_wifi_set_mode(new_mode);
	if (err) {
		printf("set mode ret err %x\n", err);
		return err;
	}

	/* AP -> APSTA */
	if (!(ctx.mode & WIFI_MODE_STA) && (new_mode & WIFI_MODE_STA)) {
		printf("set mode reco\n");
		xSemaphoreGive(ctx.lock);
		disconn_handler();
	}
	ctx.mode = new_mode;
	printf("set mode ret %x\n", err);
	return err;
}

static void set_sta_cred(const char *ssid, const char *password)
{
	wifi_config_t wifi_config = {
		.sta = {
			.threshold.rssi = -80,
			.sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
		},
	};
	memcpy((char *)wifi_config.sta.ssid, ssid, 32);
	memcpy((char *)wifi_config.sta.password, password, 64);

	int err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
	if (err) {
		ESP_LOGE(TAG, "%s", esp_err_to_name(err));
	}
}

static void delayed_set_ap_stop(void *arg)
{
	uint32_t ret = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(30000));
	xSemaphoreTake(ctx.lock, pdMS_TO_TICKS(10000));
	/* timeout: connected to STA without disconnection: close AP */
	if (ret == 0) {
		set_wifi_mode(WIFI_AP_STOP);
		ctx.try_connect_count = 0;
	}

	ctx.delayed_stopAP_task = NULL;
	xSemaphoreGive(ctx.lock);
	vTaskDelete(NULL);
}

static void delayed_set_ap_start(void *arg)
{
	uint32_t ret = 0;

	if (!(ctx.try_connect_count > 5)) {
		ret = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10000));
	}
	xSemaphoreTake(ctx.lock, pdMS_TO_TICKS(10000));
	/* timeout: connected to STA without disconnection for 10 sec: close AP */
	/* or consecutive connect/disconnect */
	if (ret == 0 && ctx.permanent_mode == WIFI_AP_AUTO_STA_ON) {
		set_wifi_mode(WIFI_AP_START);
		ctx.try_connect_count = 0;
	}

	ctx.delayed_startAP_task = NULL;
	xSemaphoreGive(ctx.lock);
	vTaskDelete(NULL);
}

static void handle_wifi_connected()
{
	ctx.is_sta_connected = true;
	if (ctx.delayed_startAP_task) {
		printf("clear start ap task");
		xTaskNotifyGive(ctx.delayed_startAP_task);
	}
	if (ctx.permanent_mode != WIFI_AP_AUTO_STA_ON) {
		return;
	}
	printf("stop ap task");
	xTaskCreate(delayed_set_ap_stop, "stop ap", 4096,
	            NULL, tskIDLE_PRIORITY + 1, &ctx.delayed_stopAP_task);
}

static void handle_wifi_disconnected()
{
	ctx.is_sta_connected = false;
	if (ctx.delayed_stopAP_task) {
		printf("clear stop ap task");
		xTaskNotifyGive(ctx.delayed_stopAP_task);
	}
	if (ctx.permanent_mode != WIFI_AP_AUTO_STA_ON) {
		return;
	}
	printf("start ap task");
	ctx.try_connect_count++;
	xTaskCreate(delayed_set_ap_start, "start ap", 4096,
	            NULL, tskIDLE_PRIORITY + 1, &ctx.delayed_startAP_task);
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
			if (ctx.conn.event) {
				/* sta connected */
				wifi_led_set_on();
				handle_wifi_connected();
			}
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
	wifi_led_set_blink();
	handle_wifi_disconnected();
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

int wifi_manager_get_ap_auto_delay(int *ap_on_delay, int *ap_off_delay)
{
	*ap_on_delay = pdTICKS_TO_MS(ctx.ap_on_delay_tick);
	*ap_off_delay = pdTICKS_TO_MS(ctx.ap_off_delay_tick);
	return 0;
}

int wifi_manager_set_ap_auto_delay(int *ap_on_delay, int *ap_off_delay)
{
	ctx.ap_on_delay_tick = pdMS_TO_TICKS(*ap_on_delay);
	ctx.ap_on_delay_tick = pdMS_TO_TICKS(*ap_off_delay);
	return wifi_manager_get_ap_auto_delay(ap_on_delay, ap_off_delay);
}

int wifi_manager_get_mode(wifi_apsta_mode_e *mode, wifi_mode_t *status)
{
	*mode = ctx.permanent_mode;
	*status = ctx.mode;
	return 0;
}

int wifi_manager_set_ap_credential(wifi_credential_t *cred)
{
	wifi_config_t ap_config = {0};
	wifi_mode_t mode;
	strncpy((char *)ap_config.ap.ssid, cred->ssid, 32);
	strncpy((char *)ap_config.ap.password, cred->password, 64);
	ap_config.ap.authmode = WIFI_AUTH_WPA2_WPA3_PSK;
	ap_config.ap.max_connection = 4;
	ap_config.ap.channel = 6;
	ap_config.ap.ssid_hidden = 0;

	if (esp_wifi_get_mode(&mode)) {
		return 1;
	}
	if (mode & WIFI_MODE_AP) {
		int err = esp_wifi_set_config(WIFI_IF_AP, &ap_config);
		if (err) {
			ESP_LOGE(TAG, "ap conf %s", esp_err_to_name(err));
		}
	}

	return 0;
}

int wifi_manager_sta_set_static_conf(wifi_api_sta_ap_static_info_t *static_info)
{
	if (static_info->static_ip_en) {
		esp_netif_ip_info_t ip_info;
		ip_info.ip.addr = static_info->ip.addr;
		ip_info.gw.addr = static_info->gateway.addr;
		ip_info.netmask.addr = static_info->netmask.addr;
		esp_netif_dhcpc_stop(wifi_manager_get_sta_netif());
		esp_netif_set_ip_info(wifi_manager_get_sta_netif(), &ip_info);
	} else {
		esp_netif_dhcpc_start(wifi_manager_get_sta_netif());
	}

	if (static_info->static_dns_en) {
		esp_netif_dns_info_t dns_info;
		dns_info.ip.type = ESP_IPADDR_TYPE_V4;
		dns_info.ip.u_addr.ip4.addr = static_info->dns_main.addr;
		esp_netif_set_dns_info(wifi_manager_get_sta_netif(), ESP_NETIF_DNS_MAIN, &dns_info);
		dns_info.ip.u_addr.ip4.addr = static_info->dns_backup.addr;
		esp_netif_set_dns_info(wifi_manager_get_sta_netif(), ESP_NETIF_DNS_BACKUP, &dns_info);
	} else {
		esp_netif_ip_info_t ip_info;
		esp_netif_dns_info_t dns_info;
		dns_info.ip.type = ESP_IPADDR_TYPE_V4;
		esp_netif_get_ip_info(wifi_manager_get_sta_netif(), &ip_info);
		dns_info.ip.u_addr.ip4.addr = ip_info.gw.addr;
		dns_info.ip.type = ESP_NETIF_DNS_MAIN;
		esp_netif_set_dns_info(wifi_manager_get_sta_netif(), ESP_NETIF_DNS_MAIN, &dns_info);
	}
	return 0;
}
