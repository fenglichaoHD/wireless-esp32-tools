/*
 * SPDX-FileCopyrightText: 2024 kerms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef REQUEST_RUNNER_H_GUARD
#define REQUEST_RUNNER_H_GUARD

#include <stdint.h>

typedef struct req_send_out_cb_t {
	void (*cb)(void *arg, int status);
	void *arg; /* socket info */
} req_send_out_cb_t;

typedef struct req_module_cb_t {
	int (*helper_cb)(void *arg);
	void *arg;
} req_module_cb_t;

typedef struct req_task_cb_t {
	req_module_cb_t module;
	req_send_out_cb_t send_out;
	int status;
} req_task_cb_t;

int request_runner_init();

int req_queue_push_long_run(req_task_cb_t *req, uint32_t delay);
int req_queue_push_send_out(req_task_cb_t *req, uint32_t delay);


#endif //REQUEST_RUNNER_H_GUARD