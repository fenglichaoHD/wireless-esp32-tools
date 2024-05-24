/*
 * SPDX-FileCopyrightText: 2024 kerms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STATIC_BUFFER_H_GUARD
#define STATIC_BUFFER_H_GUARD

#include <stdint.h>

int memory_pool_init();

void *memory_pool_get(uint32_t tick_wait);

void memory_pool_put(void *ptr);

uint32_t memory_pool_get_buf_size();


#endif //STATIC_BUFFER_H_GUARD