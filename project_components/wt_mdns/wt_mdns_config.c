#include "wt_mdns_config.h"

#include <mdns.h>
#include <esp_log.h>

#define MDNS_TAG "wt_mdns"

void wt_mdns_init()
{
	ESP_ERROR_CHECK(mdns_init());

	/* TODO: read hostname from NVS */
	ESP_ERROR_CHECK(mdns_hostname_set(MDSN_DEFAULT_HOSTNAME));

	/* TODO: read instance description from NVS */
	ESP_ERROR_CHECK(mdns_instance_name_set(MDSN_INSTANCE_DESC));

	uint8_t mac[6];
	esp_netif_get_mac(esp_netif_get_default_netif(), mac);

	char value[64];
	snprintf(value, 63, "允斯调试器-%02X-%02X", mac[4], mac[5]);
	value[63] = '\0';

	//structure with TXT records
	mdns_txt_item_t serviceTxtData[] = {
		{"name", value},
	};

	//initialize service
	ESP_ERROR_CHECK(mdns_service_add("WebServer", "_http", "_tcp", 80,
									  serviceTxtData, sizeof(serviceTxtData)/sizeof(serviceTxtData[0])));
	ESP_ERROR_CHECK(mdns_service_subtype_add_for_host("WebServer", "_http", "_tcp", NULL, "_server") );
}

int wt_mdns_set_hostname(const char *hostname)
{
	mdns_hostname_set(hostname);

	/* TODO save to NVS */
	return 0;
}
