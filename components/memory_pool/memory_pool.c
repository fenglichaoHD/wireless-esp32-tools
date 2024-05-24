/*
 * SPDX-FileCopyrightText: 2024 kerms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "memory_pool.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define BUFFER_NR 8
#define BUFFER_SZ 2048

static uint8_t buf[BUFFER_NR][BUFFER_SZ];

/* TODO: use CAS */
static QueueHandle_t buf_queue = NULL;

int memory_pool_init()
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

void *memory_pool_get(uint32_t tick_wait)
{
	void *ptr = NULL;
	xQueueReceive(buf_queue, &ptr, tick_wait);
	return ptr;
}

void memory_pool_put(void *ptr)
{
	//printf("put buf %d\n", uxQueueMessagesWaiting(buf_queue));
	if (unlikely(xQueueSend(buf_queue, &ptr, 0) != pdTRUE)) {
		assert(0);
	}
}

inline uint32_t memory_pool_get_buf_size()
{
	return BUFFER_SZ;
}
