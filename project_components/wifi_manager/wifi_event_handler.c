#include "wifi_event_handler.h"

#include <esp_netif.h>
#include <lwip/ip4_addr.h>
#include <esp_wifi.h>

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

void wifi_event_handler(void *handler_arg __attribute__((unused)),
                        esp_event_base_t event_base __attribute__((unused)),
                        int32_t event_id,
                        void *event_data)
{
	switch (event_id) {
	case WIFI_EVENT_WIFI_READY:
		printf("event: WIFI_EVENT_WIFI_READY\n");
		break;
	case WIFI_EVENT_SCAN_DONE:
		break;
	case WIFI_EVENT_STA_START:
		printf("event: WIFI_EVENT_STA_START\n");
		esp_wifi_connect();
		break;
	case WIFI_EVENT_STA_CONNECTED: {
		wifi_event_sta_connected_t *event = event_data;
		uint8_t *m = event->bssid;
		printf("event: WIFI_EVENT_STA_CONNECTED\n");
		printf("connected to %2X:%2X:%2X:%2X:%2X:%2X\n", m[0], m[1], m[2], m[3], m[4], m[5]);
#ifdef CONFIG_EXAMPLE_IPV6
		tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
#endif
		break;
	}
	case WIFI_EVENT_STA_DISCONNECTED: {
		wifi_event_sta_disconnected_t *event = event_data;
		uint8_t *m = event->bssid;
		printf("event: WIFI_EVENT_STA_DISCONNECTED\n");
		printf("sta %2X:%2X:%2X:%2X:%2X:%2X disconnect reason %d\n",
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
		printf("%2X:%2X:%2X:%2X:%2X:%2X is connected\n",
			   m[0], m[1], m[2], m[3], m[4], m[5]);
		break;
	}
	case WIFI_EVENT_AP_STADISCONNECTED: {
		wifi_event_ap_staconnected_t *event = event_data;
		uint8_t *m = event->mac;
		printf("event: WIFI_EVENT_AP_STADISCONNECTED\n");
		printf("%2X:%2X:%2X:%2X:%2X:%2X is connected\n",
		       m[0], m[1], m[2], m[3], m[4], m[5]);
		break;
	}
	default:
		break;
	}
}