#include <string.h>
#include <stdint.h>
#include <sys/param.h>
#include <lwip/ip4_addr.h>

#ifdef CONFIG_EXAMPLE_IPV6
#include <lwip/ip6_addr.h>
#endif

#include "main/wifi_configuration.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#ifdef CONFIG_IDF_TARGET_ESP8266
    #define PIN_LED_WIFI_STATUS 15
#elif defined CONFIG_IDF_TARGET_ESP32
    #define PIN_LED_WIFI_STATUS 27
#elif defined CONFIG_IDF_TARGET_ESP32C3
    #define PIN_LED_WIFI_STATUS 10
#else
    #error unknown hardware
#endif

static EventGroupHandle_t wifi_event_group;
static esp_netif_t *sta_netif;
static int ssid_index = 0;

const int IPV4_GOTIP_BIT = BIT0;
#ifdef CONFIG_EXAMPLE_IPV6
const int IPV6_GOTIP_BIT = BIT1;
#endif

static void ssid_change();

static void event_handler(void *handler_arg __attribute__((unused)),
                             esp_event_base_t event_base __attribute__((unused)),
                             int32_t event_id,
                             void *event_data)
{
    switch (event_id) {
    case WIFI_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case WIFI_EVENT_STA_CONNECTED:
#ifdef CONFIG_EXAMPLE_IPV6
        /* enable ipv6 */
	    esp_netif_create_ip6_linklocal(sta_netif);
#endif
        break;
    case IP_EVENT_STA_GOT_IP: {
	    ip_event_got_ip_t *event = event_data;
//	    GPIO_SET_LEVEL_HIGH(PIN_LED_WIFI_STATUS);
        xEventGroupSetBits(wifi_event_group, IPV4_GOTIP_BIT);
        os_printf("SYSTEM EVENT STA GOT IP : %s\r\n", ip4addr_ntoa((const ip4_addr_t *) &event->ip_info.ip));
        break;
	}
    case WIFI_EVENT_STA_DISCONNECTED: {
	    wifi_event_sta_disconnected_t *event = event_data;
//	    GPIO_SET_LEVEL_LOW(PIN_LED_WIFI_STATUS);
	    os_printf("Disconnect reason : %d\r\n", event->reason);

#ifdef CONFIG_IDF_TARGET_ESP8266
        if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
            /*Switch to 802.11 bgn mode */
            esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
        }
#endif
        ssid_change();
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, IPV4_GOTIP_BIT);
#ifdef CONFIG_EXAMPLE_IPV6
        xEventGroupClearBits(wifi_event_group, IPV6_GOTIP_BIT);
#endif

#if (USE_UART_BRIDGE == 1)
        uart_bridge_close();
#endif
        break;
	}
    case IP_EVENT_GOT_IP6: {
#ifdef CONFIG_EXAMPLE_IPV6
	    ip_event_got_ip6_t *event = event_data;
        xEventGroupSetBits(wifi_event_group, IPV6_GOTIP_BIT);
        os_printf("SYSTEM_EVENT_STA_GOT_IP6\r\n");

	    char *ip6 = ip6addr_ntoa((ip6_addr_t *) &event->ip6_info);
        os_printf("IPv6: %s\r\n", ip6);
#endif
	}
    default:
        break;
    }
}

static void ssid_change() {
    if (ssid_index > WIFI_LIST_SIZE - 1) {
        ssid_index = 0;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
        },
    };

    strcpy((char *)wifi_config.sta.ssid, wifi_list[ssid_index].ssid);
    strcpy((char *)wifi_config.sta.password, wifi_list[ssid_index].password);
    ssid_index++;
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
}

static void wait_for_ip() {
#ifdef CONFIG_EXAMPLE_IPV6
    uint32_t bits = IPV4_GOTIP_BIT | IPV6_GOTIP_BIT;
#else
    uint32_t bits = IPV4_GOTIP_BIT;
#endif

    os_printf("Waiting for AP connection...\r\n");
    xEventGroupWaitBits(wifi_event_group, bits, false, true, portMAX_DELAY);
    os_printf("Connected to AP\r\n");
}

void wifi_init(void) {
//    GPIO_FUNCTION_SET(PIN_LED_WIFI_STATUS);
//    GPIO_SET_DIRECTION_NORMAL_OUT(PIN_LED_WIFI_STATUS);

	esp_netif_init();
	wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	sta_netif = esp_netif_create_default_wifi_sta();
	assert(sta_netif);

#if (USE_STATIC_IP == 1)
	esp_netif_dhcpc_stop(sta_netif);

	esp_netif_ip_info_t ip_info;
	esp_netif_set_ip4_addr(&ip_info.ip, DAP_IP_ADDRESS);
	esp_netif_set_ip4_addr(&ip_info.gw, DAP_IP_GATEWAY);
	esp_netif_set_ip4_addr(&ip_info.netmask, DAP_IP_NETMASK);
	esp_netif_set_ip_info(sta_netif, &ip_info);
#endif // (USE_STATIC_IP == 1)

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(
		WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(
		IP_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));

	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    // os_printf("Setting WiFi configuration SSID %s...\r\n", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ssid_change();
    ESP_ERROR_CHECK(esp_wifi_start());


    wait_for_ip();
}
