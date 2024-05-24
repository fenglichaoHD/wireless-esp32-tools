/*
 * SPDX-FileCopyrightText: 2024 kerms <kerms@niazo.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GLOBAL_MODULE_H_GUARD
#define GLOBAL_MODULE_H_GUARD

#include <stdint.h>

typedef int (*global_module_init_func)(void);

typedef struct global_module_t {
	global_module_init_func init;
	uint8_t module_id;
} global_module_t;

int global_module_reg(const global_module_t *g_mod);

int global_module_init();

#define GLOBAL_MODULE_REGISTER(NAME, GLOBAL_MODULE_CFG) \
  __attribute__((used, constructor)) void cons_G_MOD_ ## NAME(); \
  void cons_G_MOD_ ## NAME() { global_module_reg(GLOBAL_MODULE_CFG); }


#endif //GLOBAL_MODULE_H_GUARD