#include "wifi_event_handler.h"

#include <esp_netif.h>
#include <lwip/ip4_addr.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#define TAG __FILE_NAME__

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

typedef struct wifi_scan_ctx_t {
	wifi_ap_record_t *ap;
	wifi_event_scan_done_cb cb;
	uint16_t number;
} wifi_scan_ctx_t;

static wifi_scan_ctx_t scan_ctx = {
	.ap = NULL,
};

static void wifi_on_scan_done(wifi_event_sta_scan_done_t *event)
{
	int err;
	if (!scan_ctx.cb || !scan_ctx.ap) {
		scan_ctx.number = 0;
	} else if (event->status == 1) {
		/* error */
		scan_ctx.number = 0;
	} else if (event->number == 0) {
		/* no ap found on current channel */
		scan_ctx.number = 0;
	}

	err = esp_wifi_scan_get_ap_records(&scan_ctx.number, scan_ctx.ap);
	if (err) {
		esp_wifi_clear_ap_list();
	}

	int i;
	for (i = 0; i < scan_ctx.number; ++i) {
		if (scan_ctx.ap[i].rssi < -80)
			break;
	}

	scan_ctx.number = i;
	return scan_ctx.cb(scan_ctx.number, scan_ctx.ap);
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

	scan_ctx.cb = cb;
	scan_ctx.number = number;
	scan_ctx.ap = aps;
	return 0;
}

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
		scan_ctx.ap = NULL;
		scan_ctx.cb = NULL;
		break;
	}
	case WIFI_EVENT_STA_START:
		printf("event: WIFI_EVENT_STA_START\n");
		esp_wifi_connect();
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
		break;
	}
	case WIFI_EVENT_STA_DISCONNECTED: {
		wifi_event_sta_disconnected_t *event = event_data;
		uint8_t *m = event->bssid;
		printf("event: WIFI_EVENT_STA_DISCONNECTED ");
		printf("sta %02X:%02X:%02X:%02X:%02X:%02X disconnect reason %d\n",
		       m[0], m[1], m[2], m[3], m[4], m[5], event->reason);
		/* auto reconnect after disconnection */
		esp_wifi_connect();
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
		printf("%02X:%02X:%02X:%02X:%02X:%02X is connected\n",
		       m[0], m[1], m[2], m[3], m[4], m[5]);
		break;
	}
	default:
		break;
	}
}

