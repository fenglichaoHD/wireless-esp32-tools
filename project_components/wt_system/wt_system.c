/*
 * SPDX-FileCopyrightText: 2024 kerms <kerms@niazo.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wt_system.h"
#include "wt_system_api.h"
#include <esp_app_desc.h>
#include <string.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>

#define TAG "WT_SYS"

void wt_system_get_fm_info(wt_fm_info_t *fm_info)
{
	const esp_app_desc_t *app_desc = esp_app_get_description();
	strcpy(fm_info->fm_ver, app_desc->version);
	memcpy(fm_info->upd_date, FW_UPD_DATE, sizeof(fm_info->upd_date) - 1);
	fm_info->upd_date[sizeof(fm_info->upd_date) - 1] = '\0';
}

static void reboot_task(void *arg)
{
	ESP_LOGW(TAG, "reboot in 2 seconds");
	vTaskDelay(pdMS_TO_TICKS(2000));
	esp_restart();
}

void wt_system_reboot()
{
	xTaskCreatePinnedToCore(reboot_task, "reboot", 4096, NULL, 3, NULL, 0);
}

static int wt_system_init(void)
{
	return 0;
}

static const global_module_t module = {
	.init = wt_system_init,
	.module_id = SYSTEM_MODULE_ID
};

GLOBAL_MODULE_REGISTER(WT_SYSTEM, &module);
