/*
 * SPDX-FileCopyrightText: 2024 kerms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "static_buffer.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define BUFFER_NR 4
#define BUFFER_SZ 2048

static uint8_t buf[BUFFER_NR][BUFFER_SZ];
static QueueHandle_t buf_queue = NULL;

int static_buffer_init()
{
	if (buf_queue != NULL)
		return 0;

	buf_queue = xQueueCreate(BUFFER_NR, sizeof(void *));
	if (buf_queue == NULL) {
		return 1;
	}
	for (int i = 0; i < BUFFER_NR; ++i) {
		uint8_t *buf_ptr = buf[i];
		if (xQueueSend(buf_queue, &buf_ptr, 0) != pdTRUE) {
			return 1;
		}
	}

	return 0;
}

void *static_buffer_get(uint32_t tick_wait)
{
	void *ptr = NULL;
	xQueueReceive(buf_queue, &ptr, tick_wait);
	return ptr;
}

void static_buffer_put(void *ptr)
{
	//printf("put buf %d\n", uxQueueMessagesWaiting(buf_queue));
	if (unlikely(xQueueSend(buf_queue, &ptr, 0) != pdTRUE)) {
		assert(0);
	}
}

uint32_t static_buffer_get_buf_size()
{
	return BUFFER_SZ;
}
