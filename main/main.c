#include <string.h>
#include <stdint.h>

#include "tcp_server.h"
#include "main/wifi_configuration.h"
#include "main/wifi_handle.h"
#include "cmsis-dap/include/DAP.h"
#include "DAP_handle.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "mdns.h"

static const char *MDNS_TAG = "server_common";

void mdns_setup() {
    // initialize mDNS
    int ret;
    ret = mdns_init();
    if (ret != ESP_OK) {
        ESP_LOGW(MDNS_TAG, "mDNS initialize failed:%d", ret);
        return;
    }

    // set mDNS hostname
    ret = mdns_hostname_set(MDNS_HOSTNAME);
    if (ret != ESP_OK) {
        ESP_LOGW(MDNS_TAG, "mDNS set hostname failed:%d", ret);
        return;
    }
    ESP_LOGI(MDNS_TAG, "mDNS hostname set to: [%s]", MDNS_HOSTNAME);

    // set default mDNS instance name
    ret = mdns_instance_name_set(MDNS_INSTANCE);
    if (ret != ESP_OK) {
        ESP_LOGW(MDNS_TAG, "mDNS set instance name failed:%d", ret);
        return;
    }
    ESP_LOGI(MDNS_TAG, "mDNS instance name set to: [%s]", MDNS_INSTANCE);
}

void app_main() {
    ESP_ERROR_CHECK(nvs_flash_init());

    wifi_init();
    DAP_Setup();

#if (USE_MDNS == 1)
    mdns_setup();
#endif

    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 14, NULL);

    // DAP handle task
    xTaskCreate(DAP_Thread, "DAP_Task", 2048, NULL, 10, NULL);
}
