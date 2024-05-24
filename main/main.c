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
#include "web_server.h"
#include "memory_pool.h"
#include "request_runner.h"
#include "uart_tcp_bridge.h"

#include <assert.h>

void app_main()
{
	assert(memory_pool_init() == 0); // static buffer
	assert(request_runner_init() == 0);
	wt_storage_init();
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	wt_mdns_init();

	wifi_manager_init();
    DAP_Setup();
	start_webserver();

    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 14, NULL);

    // DAP handle task
    xTaskCreate(DAP_Thread, "DAP_Task", 2048, NULL, 10, NULL);

	xTaskCreate(uart_bridge_task, "uart_server", 4096, NULL, 2, NULL);
}
