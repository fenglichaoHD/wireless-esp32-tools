/*
 * SPDX-FileCopyrightText: 2024 kerms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "request_runner.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <sys/cdefs.h>

static QueueHandle_t long_run_queue = NULL;
static QueueHandle_t send_out_queue = NULL;

_Noreturn static void req_long_task(void *arg);

_Noreturn static void req_send_out_task(void *arg);

_Noreturn void req_long_task(void *arg)
{
	req_task_cb_t *req;
	while (1) {
		if (unlikely(xQueueReceive(long_run_queue, &req, portMAX_DELAY) != pdTRUE)) {
			continue;
		}
		req->status = req->module.helper_cb(req->module.arg);

		/* if send out queue is busy, set status and let the cb to cancel send out
		 * */
		if (req_queue_push_send_out(req, pdMS_TO_TICKS(20)) != 0) {
			req->status = -1;
			req->send_out.cb(req->send_out.arg, req->status);
		}
	}
}

_Noreturn void req_send_out_task(void *arg)
{
	req_task_cb_t *req;
	while (1) {
		if (likely(xQueueReceive(send_out_queue, &req, portMAX_DELAY))) {
			req->send_out.cb(req->send_out.arg, req->status);
		}
	}
}

int request_runner_init()
{
	BaseType_t res;
	if (long_run_queue != NULL || send_out_queue != NULL) {
		return 0;
	}

	long_run_queue = xQueueCreate(2, sizeof(req_task_cb_t *));
	send_out_queue = xQueueCreate(4, sizeof(req_task_cb_t *));
	assert(long_run_queue != NULL && send_out_queue != NULL);

	res = xTaskCreate(req_long_task, "Ltask", 4 * 1024, NULL, 8, NULL);
	res &= xTaskCreate(req_send_out_task, "send out task", 4 * 1024, NULL, 9, NULL);
	assert(res == pdPASS);
	return 0;
}

int req_queue_push_long_run(req_task_cb_t *req, uint32_t delay)
{
	if (unlikely(xQueueSend(long_run_queue, &req, delay) != pdTRUE)) {
		return 1;
	}

	return 0;
}

int req_queue_push_send_out(req_task_cb_t *req, uint32_t delay)
{
	if (unlikely(xQueueSend(send_out_queue, &req, delay) != pdTRUE)) {
		return 1;
	}

	return 0;
}
