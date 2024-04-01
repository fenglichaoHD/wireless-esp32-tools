#include "web_uri_module.h"
#include "list.h"

#include <esp_log.h>


#define URI_MODULE_MAX 8

#define TAG __FILE_NAME__

typedef struct uri_module_list_t {
	struct dl_list list;
	uri_module_t module;
} uri_module_list_t;

static uri_module_list_t module_arr[URI_MODULE_MAX];
static uint8_t module_count = 0;
static DEFINE_DL_LIST(list_head);

int uri_module_init(httpd_handle_t server)
{
	int err;
	const httpd_uri_t *uri;
	uri_module_list_t *item;
	uint8_t index = 0;

	dl_list_for_each(item, &list_head, uri_module_list_t, list) {
		err = item->module.init(&uri);
		ESP_LOGI(TAG, "uri %s init", uri->uri);
		if (err) {
			ESP_LOGE(TAG, "%d init error", index);
		}

		err = httpd_register_uri_handler(server, uri);
		if (err) {
			ESP_LOGE(TAG, "%d %s", index, esp_err_to_name(err));
		}
		index++;
	}
	return 0;
}

int uri_module_exit(httpd_handle_t server)
{

	return 0;
}

int uri_module_add(uint8_t priority, uri_module_func init, uri_module_func exit)
{
	ESP_LOGI(TAG, "adding module %p", init);

	if (module_count >= URI_MODULE_MAX) {
		ESP_LOGE(TAG, "too much module > URI_MODULE_MAX");
		return 1;
	}

	module_arr[module_count].module.exit = exit;
	module_arr[module_count].module.init = init;
	module_arr[module_count].module.priority = priority;

	uri_module_list_t *item;
	dl_list_for_each(item, &list_head, uri_module_list_t, list) {
		if (item->module.priority <= priority) {
			break;
		}
	}
	dl_list_add(&item->list, &module_arr[module_count].list);
	module_count++;
	return 0;
}
