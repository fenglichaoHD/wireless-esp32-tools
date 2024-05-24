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

	//structure with TXT records
	mdns_txt_item_t serviceTxtData[] = {
#if defined CONFIG_IDF_TARGET_ESP32S3
		{"board", "esp32S3"},
#elif defined CONFIG_IDF_TARGET_ESP32C3
		{"board", "esp32c3"},
#elif defined CONFIG_IDF_TARGET_ESP32
		{"board", "esp32"},
#endif
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
