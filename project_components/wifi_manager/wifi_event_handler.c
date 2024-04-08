#include "wifi_event_handler.h"

#include <esp_netif.h>
#include <lwip/ip4_addr.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#define TAG __FILE_NAME__

void event_on_connected(ip_event_got_ip_t *event);

void ip_event_handler(void *handler_arg __attribute__((unused)),
                      esp_event_base_t event_base __attribute__((unused)),
                      int32_t event_id,
                      void *event_data)
{
	switch (event_id) {
	case IP_EVENT_STA_GOT_IP: {
		ip_event_got_ip_t *event = event_data;
		printf("STA GOT IP : %s\n",
		       ip4addr_ntoa((const ip4_addr_t *) &event->ip_info.ip));
		event_on_connected(event);
		break;
	}
	case IP_EVENT_STA_LOST_IP:
		printf("sta lost ip\n");
		break;
	case IP_EVENT_AP_STAIPASSIGNED:
		printf("event STAIPASSIGNED\n");
		break;
	case IP_EVENT_GOT_IP6:
#ifdef CONFIG_EXAMPLE_IPV6
		xEventGroupSetBits(wifi_event_group, IPV6_GOTIP_BIT);
	printf("SYSTEM_EVENT_STA_GOT_IP6\r\n");

	char *ip6 = ip6addr_ntoa(&event->event_info.got_ip6.ip6_info.ip);
	printf("IPv6: %s\r\n", ip6);
#endif
	case IP_EVENT_ETH_GOT_IP:
	case IP_EVENT_ETH_LOST_IP:
	case IP_EVENT_PPP_GOT_IP:
	case IP_EVENT_PPP_LOST_IP:
	default:
		break;
	}
}

typedef struct wifi_event_ctx_t {
	/* WIFI scan */
	struct {
		wifi_ap_record_t *ap;
		wifi_event_scan_done_cb cb;
		uint16_t number;
	};
	/* WIFI connect */
	struct {
		wifi_event_connect_done_cb cb;
		void *arg;
		void (*disconn_handler)(void);
		uint8_t attempt;
	} conn;
	uint8_t is_connected;
} wifi_event_ctx_t;

static wifi_event_ctx_t event_ctx = {
	.ap = NULL,
	.is_connected = 0,
	.conn.disconn_handler = NULL,
};

static void wifi_on_scan_done(wifi_event_sta_scan_done_t *event)
{
	if (!event_ctx.cb || !event_ctx.ap) {
		event_ctx.number = 0;
	} else if (event->status == 1) {
		/* error */
		event_ctx.number = 0;
	} else if (event->number == 0) {
		/* no ap found on current channel */
		event_ctx.number = 0;
	}

	esp_wifi_scan_get_ap_records(&event_ctx.number, event_ctx.ap);
	esp_wifi_clear_ap_list();

	int i;
	for (i = 0; i < event_ctx.number; ++i) {
		if (event_ctx.ap[i].rssi < -80)
			break;
	}

	event_ctx.number = i;
	return event_ctx.cb(event_ctx.number, event_ctx.ap);
}


int wifi_event_trigger_scan(uint8_t channel, wifi_event_scan_done_cb cb, uint16_t number, wifi_ap_record_t *aps)
{
	int err;

	if (aps == NULL) {
		return 1;
	}

	wifi_scan_config_t config = {
		.bssid = NULL,
		.ssid = NULL,
		.show_hidden = 0,
		.scan_type = WIFI_SCAN_TYPE_ACTIVE,
		.scan_time.active.min = 10,
		.scan_time.active.max = 50,
		.channel = channel,
		.home_chan_dwell_time = 0,
	};

	err = esp_wifi_scan_start(&config, 0);
	if (err) {
		ESP_LOGE(TAG, "%s", esp_err_to_name(err));
		return err;
	}

	event_ctx.cb = cb;
	event_ctx.number = number;
	event_ctx.ap = aps;
	return 0;
}

/*
 * WIFI EVENT
 * */

static void reconnect_after_disco();

void wifi_event_handler(void *handler_arg __attribute__((unused)),
                        esp_event_base_t event_base __attribute__((unused)),
                        int32_t event_id,
                        void *event_data)
{
	switch (event_id) {
	case WIFI_EVENT_WIFI_READY:
		printf("event: WIFI_EVENT_WIFI_READY\n");
		break;
	case WIFI_EVENT_SCAN_DONE: {
		wifi_event_sta_scan_done_t *event = event_data;
//		printf("event: WIFI_EVENT_SCAN_DONE: ok: %lu, nr: %u, seq: %u\n",
//		       event->status, event->number, event->scan_id);
		wifi_on_scan_done(event);
		event_ctx.ap = NULL;
		event_ctx.cb = NULL;
		break;
	}
	case WIFI_EVENT_STA_START:
		printf("event: WIFI_EVENT_STA_START\n");
		break;
	case WIFI_EVENT_STA_CONNECTED: {
		wifi_event_sta_connected_t *event = event_data;
		uint8_t *m = event->bssid;
		printf("event: WIFI_EVENT_STA_CONNECTED\n");
		printf("connected to %02X:%02X:%02X:%02X:%02X:%02X\n",
			   m[0], m[1], m[2], m[3], m[4], m[5]);
#ifdef CONFIG_EXAMPLE_IPV6
		tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
#endif
		event_ctx.is_connected = 1;
		break;
	}
	case WIFI_EVENT_STA_DISCONNECTED: {
		wifi_event_sta_disconnected_t *event = event_data;
		uint8_t *m = event->bssid;
		printf("event: WIFI_EVENT_STA_DISCONNECTED ");
		printf("sta %02X:%02X:%02X:%02X:%02X:%02X disconnect reason %d\n",
		       m[0], m[1], m[2], m[3], m[4], m[5], event->reason);
		event_ctx.is_connected = 0;
		reconnect_after_disco();
		break;
	}
	case WIFI_EVENT_AP_START:
		printf("event: WIFI_EVENT_AP_START\n");
		break;
	case WIFI_EVENT_AP_STACONNECTED: {
		wifi_event_ap_staconnected_t *event = event_data;
		uint8_t *m = event->mac;
		printf("event: WIFI_EVENT_AP_STACONNECTED\n");
		printf("%02X:%02X:%02X:%02X:%02X:%02X is connected\n",
			   m[0], m[1], m[2], m[3], m[4], m[5]);
		break;
	}
	case WIFI_EVENT_AP_STADISCONNECTED: {
		wifi_event_ap_staconnected_t *event = event_data;
		uint8_t *m = event->mac;
		printf("event: WIFI_EVENT_AP_STADISCONNECTED\n");
		printf("%02X:%02X:%02X:%02X:%02X:%02X is disconnected\n",
		       m[0], m[1], m[2], m[3], m[4], m[5]);
		break;
	}
	default:
		break;
	}
}

void wifi_event_set_disco_handler(void (*disconn_handler)(void))
{
	event_ctx.conn.disconn_handler = disconn_handler;
}

int wifi_event_trigger_connect(uint8_t attempt, wifi_event_connect_done_cb cb, void *arg)
{
	int err;
	event_ctx.conn.attempt = attempt;
	event_ctx.conn.cb = cb;
	event_ctx.conn.arg = arg;
	if (event_ctx.is_connected) {
		err = esp_wifi_disconnect();
	} else {
		err = esp_wifi_connect();
	}
	return err;
}

void event_on_connected(ip_event_got_ip_t *event)
{
	if (event_ctx.conn.cb) {
		event_ctx.conn.cb(event_ctx.conn.arg, event);
	}
	event_ctx.conn.attempt = 0;
	event_ctx.conn.cb = NULL;
	event_ctx.conn.arg = NULL;
}

void reconnect_after_disco()
{
	int err;

	ESP_LOGI(TAG, "reco... atempt %d", event_ctx.conn.attempt);

	if (event_ctx.conn.attempt == 0) {
		goto call;
	}

	err = esp_wifi_connect();
	if (err) {
		event_ctx.conn.attempt = 0;
		/* connect err: stop connect and call cb */
		ESP_LOGE(TAG, "wifi connect error %s", esp_err_to_name(err));
		goto call;
	}

	event_ctx.conn.attempt--;
	return;

call:
	if (event_ctx.conn.cb) {
		event_ctx.conn.cb(event_ctx.conn.arg, NULL);
	} else {
		/* disconnected from extern environment, notify */
		if (event_ctx.conn.disconn_handler) {
			event_ctx.conn.disconn_handler();
		}
	}
	event_ctx.conn.cb = NULL;
	event_ctx.conn.arg = NULL;
}
