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
}

int wt_mdns_set_hostname(const char *hostname)
{
	mdns_hostname_set(hostname);

	/* TODO save to NVS */
	return 0;
}
