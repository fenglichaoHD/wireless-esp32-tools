/*
 * SPDX-FileCopyrightText: 2024 kerms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "web_uri_module.h"
#include "api_json_router.h"
#include "static_buffer.h"

#include <esp_http_server.h>
#include <esp_log.h>
#include <list.h>

#include <cJSON.h>
#include <errno.h>

#define TAG __FILE_NAME__
#define MSG_BUSY_ERROR "{\"error\":\"Resource busy\"}"
#define MSG_JSON_ERROR "{\"error\":\"JSON parse error\"}"
#define MSG_SEND_JSON_ERROR "{\"error\":\"JSON generation error\"}"
#define MSG_INTERNAL_ERROR "{\"error\":\"\"}"

typedef struct ws_msg_t {
	api_json_req_t json;
	api_json_module_async_t async;
	httpd_handle_t hd;
	int fd;
	httpd_ws_frame_t ws_pkt;
	uint8_t delim[0];
	uint8_t payload[0]; /* size = static_buf_size - offsetof(this, delim) */
} ws_msg_t;

#define PAYLOAD_LEN static_buffer_get_buf_size() - sizeof(ws_msg_t)

static int ws_on_text_data(httpd_req_t *req, ws_msg_t *ws_msg);
static int ws_on_binary_data(httpd_req_t *req, ws_msg_t *ws_msg);
static void ws_async_resp(void *arg);
static void async_send_out_cb(void *arg, int module_status);
static void json_to_text(ws_msg_t *msg);

static void test(void *arg)
{
	ESP_LOGE(TAG, "this is a test");
}

static esp_err_t ws_req_handler(httpd_req_t *req)
{
	if (unlikely(req->method == HTTP_GET)) {
		int sock_fd = httpd_req_to_sockfd(req);
		/**
		 * TODO: add socket to array
		 * */
		ESP_LOGI(TAG, "ws open: %d", sock_fd);
		return ESP_OK;
	}

	ESP_LOGI(TAG, "ws_handler: httpd_handle_t=%p, sockfd=%d, client_info:%d", req->handle,
	         httpd_req_to_sockfd(req), httpd_ws_get_fd_info(req->handle, httpd_req_to_sockfd(req)));

	int err = ESP_OK;
	httpd_ws_frame_t *ws_pkt;
	ws_msg_t *ws_msg;

	ws_msg = static_buffer_get(pdMS_TO_TICKS(10));
	if (unlikely(ws_msg == NULL)) {
		httpd_ws_frame_t resp_pkt;
		resp_pkt.type = HTTPD_WS_TYPE_TEXT;
		resp_pkt.len = strlen(MSG_BUSY_ERROR);
		resp_pkt.payload = (uint8_t *)MSG_BUSY_ERROR;
		resp_pkt.final = 1;
		httpd_ws_send_frame_async(req->handle, httpd_req_to_sockfd(req), &resp_pkt);
		goto end;
	}
	ws_pkt = &ws_msg->ws_pkt;
	ws_pkt->len = 0;

	/* get and check frame size */
	err = httpd_ws_recv_frame(req, ws_pkt, 0);
	if (unlikely(err != ESP_OK)) {
		ESP_LOGE(TAG, "ws recv len error");
		goto end;
	}
	ESP_LOGI(TAG, "frame len: %d, type: %d", ws_pkt->len, ws_pkt->type);
	if (unlikely(ws_pkt->len > PAYLOAD_LEN)) {
		ESP_LOGE(TAG, "frame len is too big");
		err = ESP_FAIL;
		goto end;
	}

	ws_pkt->payload = ws_msg->payload;
	/* read incoming data */
	err = httpd_ws_recv_frame(req, ws_pkt, ws_pkt->len);
	if (unlikely(err != ESP_OK)) {
		ESP_LOGE(TAG, "ws recv data error");
		goto end;
	}

	switch (ws_pkt->type) {
	case HTTPD_WS_TYPE_CONTINUE:
		break;
	case HTTPD_WS_TYPE_TEXT:
		return ws_on_text_data(req, ws_msg);
	case HTTPD_WS_TYPE_BINARY:
		return ws_on_binary_data(req, ws_msg);
	case HTTPD_WS_TYPE_CLOSE:
		/* Read the rest of the CLOSE frame and response */
		/* Please refer to RFC6455 Section 5.5.1 for more details */
		ws_pkt->len = 0;
		ws_pkt->type = HTTPD_WS_TYPE_CLOSE;
		err = httpd_ws_send_frame(req, ws_pkt);
		break;
	case HTTPD_WS_TYPE_PING:
		/* Now turn the frame to PONG */
		ws_pkt->type = HTTPD_WS_TYPE_PONG;
		err = httpd_ws_send_frame(req, ws_pkt);
		break;
	case HTTPD_WS_TYPE_PONG:
		err = ESP_OK;
		break;
	}

end:
	static_buffer_put(ws_msg);
	return err;
}

/**
 * REGISTER MODULE
 * */

static const httpd_uri_t uri_api = {
	.uri       = "/ws",
	.method    = HTTP_GET,
	.handler   = ws_req_handler,
	.user_ctx  = NULL,
	.is_websocket = true,
	.supported_subprotocol = NULL,
	.handle_ws_control_frames = true,
};

static int WS_REQ_INIT(const httpd_uri_t **uri_conf) {
	*uri_conf = &uri_api;
	return 0;
}

static int WS_REQ_EXIT(const httpd_uri_t **uri_conf) {
	*uri_conf = &uri_api;
	return 0;
}

WEB_URI_MODULE_REGISTER(101, WS_REQ_INIT, WS_REQ_EXIT)

int ws_on_text_data(httpd_req_t *req, ws_msg_t *ws_msg)
{
	int err = ESP_OK;
	int ret;
	httpd_ws_frame_t *ws_pkt;

	ws_pkt = &ws_msg->ws_pkt;
	ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
	ESP_LOGI(TAG, "%.*s", ws_pkt->len, ws_pkt->payload);
	ESP_LOGI(TAG, "====================================");
	ESP_LOGI(TAG, "heap min: %lu, cur: %lu", esp_get_minimum_free_heap_size(), esp_get_free_heap_size());

	/* Decode */
	ws_msg->json.in = cJSON_ParseWithLength((char *) ws_pkt->payload, ws_pkt->len);
	if (unlikely(ws_msg->json.in == NULL)) {
		ws_pkt->payload = (uint8_t *) MSG_JSON_ERROR;
		ws_pkt->len = strlen(MSG_JSON_ERROR);
		goto put_buf;
	}

	ws_msg->json.out = NULL;
	ret = api_json_route(&ws_msg->json, &ws_msg->async);
	if (ret == API_JSON_ASYNC) {
		ws_msg->hd = req->handle;
		ws_msg->fd = httpd_req_to_sockfd(req);
		ws_msg->async.req_task.send_out.cb = async_send_out_cb;
		ws_msg->async.req_task.send_out.arg = ws_msg;
		if (req_queue_push_long_run(&ws_msg->async.req_task, pdMS_TO_TICKS(20))) {
			/* queued failed */
			goto end;
		}
		/* ret, buf will be release latter in async send out */
		return ESP_OK;
	} else if (ret != API_JSON_OK) {
		ws_pkt->len = strlen(MSG_BUSY_ERROR);
		ws_pkt->payload = (uint8_t *)MSG_BUSY_ERROR;
		ws_pkt->final = 1;
		err = httpd_ws_send_frame_async(req->handle, httpd_req_to_sockfd(req), ws_pkt);
		goto end;
	} else if (ws_msg->json.out == NULL) {
		goto end;
	}

	/* api function returns something, send it to http client */
	json_to_text(ws_msg);

end:
	cJSON_Delete(ws_msg->json.in);
put_buf:
	httpd_ws_send_frame_async(req->handle, httpd_req_to_sockfd(req), ws_pkt);
	static_buffer_put(ws_msg);
	return err;
}

int ws_on_binary_data(httpd_req_t *req, ws_msg_t *ws_msg)
{
	static_buffer_put(ws_msg);
	return 0;
}

static void ws_async_resp(void *arg)
{
	ws_msg_t *req = arg;
	httpd_handle_t hd = req->hd;
	int fd = req->fd;
	int err;

	ESP_LOGI(TAG, "ws async fd : %d", fd);
	err = httpd_ws_send_frame_async(hd, fd, &req->ws_pkt);
	if (err) {
		ESP_LOGE(TAG, "%s", esp_err_to_name(err));
	}
	static_buffer_put(req);
}

void async_send_out_cb(void *arg, int module_status)
{
	ws_msg_t *req = arg;
	if (module_status != API_JSON_OK) {
		goto end;
	}

	int err;
	json_to_text(req);
	err = httpd_queue_work(req->hd, ws_async_resp, req);
	if (likely(err == ESP_OK)) {
		return;
	}

	ESP_LOGE(TAG, "errno: %d, fd %p: %s", errno, req->hd, esp_err_to_name(err));

end:
	/* clean resources */
	static_buffer_put(req);
}

void json_to_text(ws_msg_t *ws_msg)
{
	int err;
	httpd_ws_frame_t *ws_pkt = &ws_msg->ws_pkt;
	/* api function returns something, send it to http client */
	err = !cJSON_PrintPreallocated(ws_msg->json.out, (char *) ws_msg->payload, PAYLOAD_LEN - 5, 0);
	cJSON_Delete(ws_msg->json.out);
	if (unlikely(err)) {
		ws_pkt->len = strlen(MSG_SEND_JSON_ERROR);
		ws_pkt->payload = (uint8_t *) MSG_SEND_JSON_ERROR;
		ws_pkt->final = 1;
	}
	ws_pkt->len = strlen((char *) ws_pkt->payload);
};
