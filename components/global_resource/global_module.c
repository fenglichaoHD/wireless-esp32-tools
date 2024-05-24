/*
 * SPDX-FileCopyrightText: 2024 kerms <kerms@niazo.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "global_module.h"
#include <stddef.h>
#include <esp_log.h>

#define TAG __FILE_NAME__
#define GLOBAL_MODULE_MAX 10

static global_module_t module_arr[GLOBAL_MODULE_MAX] = {0};
static uint8_t module_count = 0;

int global_module_reg(const global_module_t *g_mod)
{
	if (g_mod->module_id > GLOBAL_MODULE_MAX) {
		ESP_LOGE(TAG, "g_id > max");
		return 1;
	}

	if (module_arr[g_mod->module_id].init != NULL) {
		ESP_LOGE(TAG, "g_id.init not NULL");
		return 1;
	}

	ESP_LOGI(TAG, "g_id: %d", g_mod->module_id);
	module_arr[g_mod->module_id].module_id = g_mod->module_id;
	module_arr[g_mod->module_id].init = g_mod->init;
	module_count++;
	return 0;
}

int global_module_init()
{
	for (int i = 0; i < GLOBAL_MODULE_MAX; ++i) {
		if (module_arr[i].init) {
			module_arr[i].init();
		}
	}
	return 0;
}
