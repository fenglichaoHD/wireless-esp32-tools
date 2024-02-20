/*
 * SPDX-FileCopyrightText: 2024 kerms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STATIC_BUFFER_H_GUARD
#define STATIC_BUFFER_H_GUARD

#include <stdint.h>

int static_buffer_init();

void *static_buffer_get(uint32_t tick_wait);

void static_buffer_put(void *ptr);

uint32_t static_buffer_get_buf_size();


#endif //STATIC_BUFFER_H_GUARD