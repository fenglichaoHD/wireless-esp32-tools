#include "web_uri_module.h"

#include <esp_log.h>

#define URI_MODULE_MAX 8

#define TAG __FILE_NAME__

static uri_module_t module_arr[URI_MODULE_MAX];
static uint8_t module_count = 0;

int uri_module_init(httpd_handle_t server)
{
	int err;
	const httpd_uri_t *uri;

	for (int i = 0; i < module_count; ++i) {
		err = module_arr[i].init(&uri);
		ESP_LOGI(TAG, "uri %s init", uri->uri);
		if (err) {
			ESP_LOGE(TAG, "%d init error", i);
		}

		err = httpd_register_uri_handler(server, uri);
		if (err) {
			ESP_LOGE(TAG, "%d %s", i, esp_err_to_name(err));
		}
	}
	return 0;
}

int uri_module_exit(httpd_handle_t server)
{
	int err;
	const httpd_uri_t *uri;
	for (int i = 0; i < module_count; ++i) {
		module_arr[i].exit(&uri);
		err = httpd_unregister_uri_handler(server, uri->uri, uri->method);
		if (err) {
			ESP_LOGE(TAG, "%d %s", i, esp_err_to_name(err));
		}
	}
	return 0;
}

int uri_module_add(uri_module_func init, uri_module_func exit)
{
	ESP_LOGE(TAG, "adding module %p", init);

	if (module_count >= URI_MODULE_MAX) {
		ESP_LOGE(TAG, "too much module > URI_MODULE_MAX");
		return 1;
	}

	module_arr[module_count].exit = exit;
	module_arr[module_count].init = init;
	module_count++;
	return 0;
}
