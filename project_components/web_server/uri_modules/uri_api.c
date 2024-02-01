#include "web_uri_module.h"
#include "api_json_router.h"

#include <esp_http_server.h>
#include <esp_log.h>

#include <cJSON.h>

#define TAG __FILE_NAME__

static char buf[2048];
static api_json_req_t json_in;
static api_json_resp_t json_out;

static esp_err_t api_post_handler(httpd_req_t *req)
{
	int data_len;
	int err;
	uint32_t remaining = req->content_len;

	data_len = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));
	if (unlikely(data_len <= 0)) {
		ESP_LOGE(TAG, "httpd recv error");
		return ESP_FAIL;
	}

	ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
	ESP_LOGI(TAG, "%.*s", data_len, buf);
	ESP_LOGI(TAG, "====================================");

	/* Decode */
	json_in.json = cJSON_ParseWithLength(buf, data_len);
	if (unlikely(json_in.json == NULL)) {
		httpd_resp_set_status(req, HTTPD_400);
		httpd_resp_send(req, NULL, 0);
		return ESP_OK;
	}

	/* DEBUG print the actual decoded string */
	cJSON_PrintPreallocated(json_in.json, buf, sizeof(buf), 0);
	printf("json: %s\n", buf);

	err = api_json_route(&json_in, &json_out);
	if (err) {
		httpd_resp_set_status(req, HTTPD_400);
		httpd_resp_send(req, NULL, 0);
		return ESP_OK;
	}
	cJSON_Delete(json_in.json);

	if (json_out.json == NULL) {
		return httpd_resp_send(req, NULL, 0);
	}

	/* api function returns something, send back to http client */
	httpd_resp_set_type(req, HTTPD_TYPE_JSON);
	err = !cJSON_PrintPreallocated(json_out.json, buf, sizeof(buf) - 5, 0);
	cJSON_Delete(json_out.json);
	json_out.json = NULL;

	if (err) {
		httpd_resp_set_status(req, HTTPD_500);
		return httpd_resp_send(req, NULL, 0);
	}

	return httpd_resp_send(req, buf, strlen(buf));
}

/**
 * REGISTER MODULE
 * */

static const httpd_uri_t uri_api = {
	.uri       = "/api",
	.method    = HTTP_POST,
	.handler   = api_post_handler,
	.user_ctx  = NULL
};

int URI_API_INIT(const httpd_uri_t **uri_conf) {
	*uri_conf = &uri_api;
	api_json_router_init();
	return 0;
}

int URI_API_EXIT(const httpd_uri_t **uri_conf) {
	*uri_conf = &uri_api;
	return 0;
}

WEB_URI_MODULE_REGISTER(0x81, URI_API_INIT, URI_API_EXIT);
