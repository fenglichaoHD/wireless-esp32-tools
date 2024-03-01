#include "web_uri_module.h"
#include "api_json_router.h"
#include "request_runner.h"
#include "static_buffer.h"

#include <esp_http_server.h>
#include <esp_log.h>

#include <cJSON.h>

#define TAG __FILE_NAME__

#define HTTPD_STATUS_503 "503 Busy"

typedef struct post_request_t {
	api_json_req_t json;
	api_json_module_async_t async;
	httpd_req_t *req_out;
	char buf[0];
} post_request_t;

static void async_send_out_cb(void *arg, int module_status);
static int uri_api_send_out(httpd_req_t *req, post_request_t *post_req);


static esp_err_t api_post_handler(httpd_req_t *req)
{
	uint32_t buf_len;
	int data_len;
	int err;
	post_request_t *post_req;
	char *buf;
	uint32_t remaining = req->content_len;

	buf_len = static_buffer_get_buf_size() - sizeof(post_request_t);
	if (unlikely(buf_len < remaining)) {
		ESP_LOGE(TAG, "req size %lu > buf_len %lu", remaining, buf_len);
		return ESP_FAIL;
	}

	post_req = static_buffer_get(pdMS_TO_TICKS(20));
	if (unlikely(post_req == NULL)) {
		ESP_LOGE(TAG, "static buf busy");
		return ESP_FAIL;
	}
	buf = post_req->buf;

	data_len = httpd_req_recv(req, buf, buf_len);
	if (unlikely(data_len <= 0)) {
		ESP_LOGE(TAG, "httpd recv error");
		err = ESP_FAIL;
		goto put_buf;
	}

	ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
	ESP_LOGI(TAG, "%.*s", data_len, buf);
	ESP_LOGI(TAG, "====================================");
	ESP_LOGI(TAG, "heap min: %lu, cur: %lu", esp_get_minimum_free_heap_size(), esp_get_free_heap_size());

	/* Decode */
	post_req->json.in = cJSON_ParseWithLength(buf, data_len);
	if (unlikely(post_req->json.in == NULL)) {
		httpd_resp_set_status(req, HTTPD_400);
		goto end;
	}

	post_req->json.out = NULL;
	err = api_json_route(&post_req->json, &post_req->async);
	if (err == API_JSON_ASYNC) {
		httpd_req_async_handler_begin(req, &post_req->req_out);
		post_req->async.req_task.send_out.cb = async_send_out_cb;
		post_req->async.req_task.send_out.arg = post_req;
		if (req_queue_push_long_run(&post_req->async.req_task, pdMS_TO_TICKS(20))) {
			/* queued failed */
			httpd_req_async_handler_complete(post_req->req_out);
			httpd_resp_set_status(req, HTTPD_STATUS_503);
			goto end;
		}
		return ESP_OK;
	} else if (unlikely(err != API_JSON_OK)) {
		httpd_resp_set_status(req, HTTPD_400);
		goto end;
	}

	if (post_req->json.out == NULL) {
		goto end;
	}

	/* api function returns something, send back to http client */
	err = uri_api_send_out(req, post_req);
	goto put_buf;

end:
	cJSON_Delete(post_req->json.in);
	err = httpd_resp_send(req, NULL, 0);
	if (unlikely(err)) {
		ESP_LOGE(TAG, "resp_send err: %s", esp_err_to_name(err));
	}
put_buf:
	static_buffer_put(post_req);
	return err;
}

int uri_api_send_out(httpd_req_t *req, post_request_t *post_req)
{
	int err;
	char *buf;
	uint32_t buf_len;

	buf = post_req->buf;
	buf_len = static_buffer_get_buf_size() - sizeof(post_request_t);

	httpd_resp_set_type(req, HTTPD_TYPE_JSON);
	err = !cJSON_PrintPreallocated(post_req->json.out, buf, buf_len - 5, 0);
	cJSON_Delete(post_req->json.out);
	cJSON_Delete(post_req->json.in);
	if (unlikely(err)) {
		httpd_resp_set_status(req, HTTPD_500);
		return httpd_resp_send(req, NULL, 0);
	}

	return httpd_resp_send(req, buf, strlen(buf));
}

void async_send_out_cb(void *arg, int module_status)
{
	post_request_t *req = arg;
	if (module_status != API_JSON_OK) {
		httpd_sess_trigger_close(req->req_out->handle,
		                         httpd_req_to_sockfd(req->req_out->handle));
	}

	uri_api_send_out(req->req_out, req);

	/* clean resources */
	httpd_req_async_handler_complete(req->req_out);
	static_buffer_put(req);
};

/**
 * REGISTER MODULE
 * */

static const httpd_uri_t uri_api = {
	.uri       = "/api",
	.method    = HTTP_POST,
	.handler   = api_post_handler,
	.user_ctx  = NULL
};

static int URI_API_INIT(const httpd_uri_t **uri_conf) {
	*uri_conf = &uri_api;
	api_json_router_init();
	return 0;
}

static int URI_API_EXIT(const httpd_uri_t **uri_conf) {
	*uri_conf = &uri_api;
	return 0;
}

WEB_URI_MODULE_REGISTER(0x81, URI_API_INIT, URI_API_EXIT)


