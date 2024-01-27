#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_event.h>
#include <esp_netif.h>

#include "tcp_server.h"
#include "cmsis-dap/include/DAP.h"
#include "DAP_handle.h"
#include "wt_mdns_config.h"
#include "wt_storage.h"
#include "wifi_manager.h"

void app_main()
{
	wt_storage_init();
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(esp_netif_init());

	wifi_manager_init();
    DAP_Setup();

	wt_mdns_init();

    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 14, NULL);

    // DAP handle task
    xTaskCreate(DAP_Thread, "DAP_Task", 2048, NULL, 10, NULL);
}
