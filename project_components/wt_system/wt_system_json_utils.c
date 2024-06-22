/*
 * SPDX-FileCopyrightText: 2024 kerms <kerms@niazo.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wt_system_json_utils.h"

static void wt_sys_json_add_header(cJSON *root, wt_system_cmd_t cmd)
{
	cJSON_AddNumberToObject(root, "cmd", cmd);
	cJSON_AddNumberToObject(root, "module", SYSTEM_MODULE_ID);
}

cJSON *wt_sys_json_ser_fm_info(wt_fm_info_t *info)
{
	cJSON *root = cJSON_CreateObject();
	wt_sys_json_add_header(root, WT_SYS_GET_FM_INFO);
	cJSON_AddStringToObject(root, "fm_ver", info->fm_ver);
	cJSON_AddStringToObject(root, "upd_date", info->upd_date);
	return root;
}
