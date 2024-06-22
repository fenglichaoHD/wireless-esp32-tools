/*
 * SPDX-FileCopyrightText: 2024 kerms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "web_uri_module.h"
#include "api_json_router.h"
#include "memory_pool.h"

#include <esp_http_server.h>
#include <esp_log.h>

#include <cJSON.h>
#include <errno.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <lwipopts.h>
#include <lwip/netdb.h>

#define TAG __FILE_NAME__


// Error message templates
#define ERROR_MSG_TEMPLATE(err_msg, err_code) "{\"error\":\"" err_msg "\", \"code\":" #err_code "}"

// Error messages
#define MSG_BAD_REQUEST_ERROR  ERROR_MSG_TEMPLATE("Bad json request", 2)
#define MSG_JSON_ERROR         ERROR_MSG_TEMPLATE("JSON parse error", 3)
#define MSG_SEND_JSON_ERROR    ERROR_MSG_TEMPLATE("JSON generation error", 3)
#define MSG_INTERNAL_ERROR     ERROR_MSG_TEMPLATE("Internal error", 3)
#define MSG_UNSUPPORTED_CMD    ERROR_MSG_TEMPLATE("Unsupported cmd", 4)
#define MSG_PROPERTY_ERROR     ERROR_MSG_TEMPLATE("Property error", 5)
#define MSG_BUSY_ERROR         ERROR_MSG_TEMPLATE("Resource busy", 6)

#define GET_FD_IDX(fd) ((fd) - LWIP_SOCKET_OFFSET)

#define WS_MODULE_ID 3

typedef struct ws_msg_t {
	api_json_req_t json;
	api_json_module_async_t async;
	httpd_handle_t hd;
	int fd;
	httpd_ws_frame_t ws_pkt;
	uint8_t delim[0];
	uint8_t payload[0]; /* size = static_buf_size - offsetof(this, delim) */
} ws_msg_t;

#define PAYLOAD_LEN memory_pool_get_buf_size() - sizeof(ws_msg_t)

struct ws_ctx_t {
	struct ws_client_info_t {
		httpd_handle_t hd;
		int fd; /* range 64 - max_socket ~ 64 */
	} clients[CONFIG_LWIP_MAX_SOCKETS];
	TaskHandle_t task_heartbeat;
	/* valid <client_count> fd are stored at beginning of the array
	 * use GET_FD_IDX to get the index in clients */
	uint8_t valid_fd[CONFIG_LWIP_MAX_SOCKETS];
	int8_t client_count;

	struct {
		SemaphoreHandle_t mutex;
		StaticSemaphore_t xMutexBuffer;
	} lock[CONFIG_LWIP_MAX_SOCKETS];
} ws_ctx;

static int ws_on_text_data(httpd_req_t *req, ws_msg_t *ws_msg);
static int ws_on_binary_data(httpd_req_t *req, ws_msg_t *ws_msg);
static int ws_on_socket_open(httpd_req_t *req);
static int ws_on_close(httpd_req_t *req, httpd_ws_frame_t *ws_pkt, void *msg);

/* send with lock per fd */
static inline int ws_send_frame_safe(httpd_handle_t hd, int fd, httpd_ws_frame_t *frame);

static void ws_async_resp(void *arg);
static void async_send_out_cb(void *arg, int module_status);
static void json_to_text(ws_msg_t *msg);

/* Heartbeat related */
static inline int8_t ws_add_fd(httpd_handle_t hd, int fd);
static inline void ws_rm_fd(int fd);

_Noreturn static void heartbeat_task(void *arg);



static esp_err_t ws_req_handler(httpd_req_t *req)
{
	if (unlikely(req->method == HTTP_GET)) {
		return ws_on_socket_open(req);
	}
#ifdef WT_DEBUG_MODE
	ESP_LOGI(TAG, "ws_handler: httpd_handle_t=%p, sockfd=%d, client_info:%d, client_count: %d", req->handle,
	         httpd_req_to_sockfd(req), httpd_ws_get_fd_info(req->handle, httpd_req_to_sockfd(req)),
	         ws_ctx.client_count);
#endif

	int err = ESP_OK;
	httpd_ws_frame_t *ws_pkt;
	ws_msg_t *ws_msg;

	ws_msg = memory_pool_get(pdMS_TO_TICKS(10));
	if (unlikely(ws_msg == NULL)) {
		httpd_ws_frame_t resp_pkt;
		resp_pkt.type = HTTPD_WS_TYPE_TEXT;
		resp_pkt.len = strlen(MSG_BUSY_ERROR);
		resp_pkt.payload = (uint8_t *)MSG_BUSY_ERROR;
		resp_pkt.final = 1;
		ws_send_frame_safe(req->handle, httpd_req_to_sockfd(req), &resp_pkt);
		goto end;
	}
	ws_pkt = &ws_msg->ws_pkt;
	ws_pkt->len = 0;

	/* get and check frame size */
	err = httpd_ws_recv_frame(req, ws_pkt, 0);
	if (unlikely(err != ESP_OK)) {
		ESP_LOGE(TAG, "ws recv len error");
		return ws_on_close(req, ws_pkt, ws_msg);
	}
#ifdef WT_DEBUG_MODE
	ESP_LOGI(TAG, "frame len: %d, type: %d", ws_pkt->len, ws_pkt->type);
#endif
	if (unlikely(ws_pkt->len > PAYLOAD_LEN)) {
		ESP_LOGE(TAG, "frame len is too big");
		return ws_on_close(req, ws_pkt, ws_msg);
	}
	ws_pkt->payload = ws_msg->payload;
	switch (ws_pkt->type) {
	case HTTPD_WS_TYPE_CONTINUE:
		ESP_LOGE(TAG, "WS Continue not handled");
		goto end;
	case HTTPD_WS_TYPE_TEXT:
		/* read incoming data */
		err = httpd_ws_recv_frame(req, ws_pkt, ws_pkt->len);
		if (unlikely(err != ESP_OK)) {
			ESP_LOGE(TAG, "ws recv data error");
			return ws_on_close(req, ws_pkt, ws_msg);
		}
		return ws_on_text_data(req, ws_msg);
	case HTTPD_WS_TYPE_BINARY:
		return ws_on_binary_data(req, ws_msg);
	case HTTPD_WS_TYPE_CLOSE:
		if ((err = httpd_ws_recv_frame(req, ws_pkt, 126)) != ESP_OK) {
			ESP_LOGE(TAG, "Cannot receive the full CLOSE frame");
			goto end;
		}
		return ws_on_close(req, ws_pkt, ws_msg);
	case HTTPD_WS_TYPE_PING:
		/* Now turn the frame to PONG */
		ESP_LOGI(TAG, "PING received");
		if ((err = httpd_ws_recv_frame(req, ws_pkt, 126)) != ESP_OK) {
			ESP_LOGE(TAG, "Cannot receive the full PONG frame");
			goto end;
		}
		ws_pkt->type = HTTPD_WS_TYPE_PONG;
		err = ws_send_frame_safe(req->handle, httpd_req_to_sockfd(req), ws_pkt);
		if (err) {
			ESP_LOGE(TAG, "Cannot send PONG frame %s", esp_err_to_name(err));
		}
		goto end;
	case HTTPD_WS_TYPE_PONG:
		err = ESP_OK;
		goto end;
	default:
		err = ESP_FAIL;
		goto end;
	}
end:
	memory_pool_put(ws_msg);
	return err;
}

void ws_set_err_msg(httpd_ws_frame_t *p_frame, api_json_req_status_e ret);

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
	ws_msg->json.in = cJSON_ParseWithLength((char *)ws_pkt->payload, ws_pkt->len);
	if (unlikely(ws_msg->json.in == NULL)) {
		ws_pkt->payload = (uint8_t *)MSG_JSON_ERROR;
		ws_pkt->len = strlen(MSG_JSON_ERROR);
		goto put_buf;
	}

	ws_msg->json.out = NULL;
	ws_msg->json.out_flag = 0;
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
		ws_set_err_msg(ws_pkt, ret);
		goto end;
	} else if (ws_msg->json.out == NULL) {
		/* API exec ok */
		goto end;
	}

	/* api function returns something, send it to http client */
	json_to_text(ws_msg);

end:
	cJSON_Delete(ws_msg->json.in);
put_buf:
	ws_send_frame_safe(req->handle, httpd_req_to_sockfd(req), ws_pkt);
	memory_pool_put(ws_msg);
	return err;
}

void ws_set_err_msg(httpd_ws_frame_t *p_frame, api_json_req_status_e ret)
{
	p_frame->final = 1;
	switch (ret) {
	case API_JSON_BAD_REQUEST:
		p_frame->len = strlen(MSG_BAD_REQUEST_ERROR);
		p_frame->payload = (uint8_t *)MSG_BAD_REQUEST_ERROR;
		break;
	case API_JSON_BUSY:
		p_frame->len = strlen(MSG_BUSY_ERROR);
		p_frame->payload = (uint8_t *)MSG_BUSY_ERROR;
		break;
	case API_JSON_UNSUPPORTED_CMD:
		p_frame->len = strlen(MSG_UNSUPPORTED_CMD);
		p_frame->payload = (uint8_t *)MSG_UNSUPPORTED_CMD;
		break;
	case API_JSON_PROPERTY_ERR:
		p_frame->len = strlen(MSG_PROPERTY_ERROR);
		p_frame->payload = (uint8_t *)MSG_PROPERTY_ERROR;
		break;
	default:
		p_frame->len = strlen(MSG_INTERNAL_ERROR);
		p_frame->payload = (uint8_t *)MSG_INTERNAL_ERROR;
		return;
	}
}

_Static_assert(sizeof(httpd_ws_frame_t) <= 16, "bin_data_internal padding not sufficient for httpd_ws_frame_t");

int ws_on_binary_data(httpd_req_t *req, ws_msg_t *ws_msg)
{
	(void) req;
	memory_pool_put(ws_msg);
	return 0;
}

int ws_on_socket_open(httpd_req_t *req)
{
	int sock_fd = httpd_req_to_sockfd(req);
	ESP_LOGI(TAG, "ws open: %d", sock_fd);
	return ws_add_fd(req->handle, sock_fd);
}

/**
 * @param msg must only used as generic ptr, truncated by ws_on_bin()
 * @return
 */
static int ws_on_close(httpd_req_t *req, httpd_ws_frame_t *ws_pkt, void *msg)
{
	/* Read the rest of the CLOSE frame and response */
	/* Please refer to RFC6455 Section 5.5.1 for more details */
	ws_pkt->len = 0;
	ws_pkt->type = HTTPD_WS_TYPE_CLOSE;
	ESP_LOGI(TAG, "ws %d closed", httpd_req_to_sockfd(req));
	ws_rm_fd(httpd_req_to_sockfd(req));
	int err = httpd_ws_send_frame_async(req, httpd_req_to_sockfd(req), ws_pkt);
	if (err) {
		ESP_LOGE(TAG, "on close %s", esp_err_to_name(err));
	}
	httpd_sess_trigger_close(req->handle, httpd_req_to_sockfd(req));
	memory_pool_put(msg);
	return err;
}

static void ws_async_resp(void *arg)
{
	ws_msg_t *req = arg;
	httpd_handle_t hd = req->hd;
	int fd = req->fd;
	int err;

	ESP_LOGI(TAG, "ws async fd : %d", fd);
	err = ws_send_frame_safe(hd, fd, &req->ws_pkt);
	if (unlikely(err)) {
		ESP_LOGE(TAG, "%s", esp_err_to_name(err));
	}
	memory_pool_put(req);
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
		/* msg queued, let callee release the buffer */
		return;
	}

	ESP_LOGE(TAG, "errno: %d, fd %p: %s", errno, req->hd, esp_err_to_name(err));

end:
	/* clean resources */
	memory_pool_put(req);
}

void json_to_text(ws_msg_t *ws_msg)
{
	int err;
	httpd_ws_frame_t *ws_pkt = &ws_msg->ws_pkt;
	/* api function returns something, send it to http client */
	err = !cJSON_PrintPreallocated(ws_msg->json.out, (char *)ws_pkt->payload, PAYLOAD_LEN - 5, 0);
	cJSON_Delete(ws_msg->json.out);
	if (unlikely(err)) {
		ws_pkt->len = strlen(MSG_SEND_JSON_ERROR);
		ws_pkt->payload = (uint8_t *)MSG_SEND_JSON_ERROR;
		ws_pkt->final = 1;
	}
	ws_pkt->len = strlen((char *)ws_pkt->payload);
}


/* Clients array manipulation function
 * */
static inline int8_t ws_add_fd(httpd_handle_t hd, int fd)
{
	if (ws_ctx.client_count >= CONFIG_LWIP_MAX_SOCKETS) {
		return 1;
	}

	uint8_t idx = GET_FD_IDX(fd);
	ws_ctx.clients[idx].fd = fd;
	ws_ctx.clients[idx].hd = hd;
	ws_ctx.valid_fd[ws_ctx.client_count] = fd;
	ws_ctx.client_count++;
	return 0;
}

/**
 * @brief replace the fd and hd to be removed by the last item
 * all front items are valid
 */
static inline void ws_rm_fd(int fd)
{
	for (int i = 0; i < ws_ctx.client_count; ++i) {
		if (ws_ctx.valid_fd[i] != fd) {
			continue;
		}
		ws_ctx.valid_fd[i] = ws_ctx.valid_fd[ws_ctx.client_count - 1];
		ws_ctx.client_count--;
		return;
	}
}

static inline int ws_send_frame_safe(httpd_handle_t hd, int fd, httpd_ws_frame_t *frame)
{
	int err;
	xSemaphoreTake(ws_ctx.lock[GET_FD_IDX(fd)].mutex, portMAX_DELAY);
	err = httpd_ws_send_frame_async(hd, fd, frame);
	xSemaphoreGive(ws_ctx.lock[GET_FD_IDX(fd)].mutex);
	return err;
}

static inline void ws_broadcast_heartbeat()
{
	static httpd_ws_frame_t ws_pkt = {
		.len = 0,
		.payload = NULL,
		.type = HTTPD_WS_TYPE_TEXT,
	};

	int err;
	for (int i = 0; i < ws_ctx.client_count; ++i) {
		uint8_t idx = GET_FD_IDX(ws_ctx.valid_fd[i]);
		err = ws_send_frame_safe(ws_ctx.clients[idx].hd, ws_ctx.clients[idx].fd, &ws_pkt);
		if (err) {
			ws_rm_fd(ws_ctx.clients[idx].fd);
			httpd_sess_trigger_close(ws_ctx.clients[idx].hd, ws_ctx.clients[idx].fd);
			ESP_LOGE(TAG, "hb send err: %s", esp_err_to_name(err));
		}
	}
}

_Noreturn
void heartbeat_task(void *arg)
{
	(void)arg;
	while (1) {
		vTaskDelay(pdMS_TO_TICKS(2000));
		ws_broadcast_heartbeat();
	}
};


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

static int WS_REQ_INIT(const httpd_uri_t **uri_conf)
{
	*uri_conf = &uri_api;
	xTaskCreate(heartbeat_task, "hb task", 2048, NULL, 3, &ws_ctx.task_heartbeat);

	ws_ctx.client_count = 0;
	for (int i = 0; i < CONFIG_LWIP_MAX_SOCKETS; ++i) {
		ws_ctx.lock[i].mutex = xSemaphoreCreateMutexStatic(&ws_ctx.lock[i].xMutexBuffer);
	}
	return 0;
}

static int WS_REQ_EXIT(const httpd_uri_t **uri_conf)
{
	*uri_conf = &uri_api;
	vTaskDelete(ws_ctx.task_heartbeat);
	ws_ctx.task_heartbeat = NULL;
	return 0;
}

WEB_URI_MODULE_REGISTER(101, WS_REQ_INIT, WS_REQ_EXIT)
