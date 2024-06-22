/*
 * SPDX-FileCopyrightText: 2024 kerms <kerms@niazo.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "wt_system_api.h"
#include "wt_system.h"
#include "api_json_module.h"
#include "wt_system_json_utils.h"


static int sys_api_json_get_fm_info(api_json_req_t *req)
{
	wt_fm_info_t info;
	wt_system_get_fm_info(&info);
	req->out = wt_sys_json_ser_fm_info(&info);
	return API_JSON_OK;
}

static int on_json_req(uint16_t cmd, api_json_req_t *req, api_json_module_async_t *async)
{
	wt_system_cmd_t ota_cmd = cmd;
	switch (ota_cmd) {
	default:
		break;
	case WT_SYS_GET_FM_INFO:
		return sys_api_json_get_fm_info(req);
	case WT_SYS_REBOOT:
		wt_system_reboot();
		return API_JSON_OK;
	}
	return API_JSON_UNSUPPORTED_CMD;
}


/* ****
 *  register module
 * */

static int wt_sys_json_init(api_json_module_cfg_t *cfg)
{
	cfg->on_req = on_json_req;
	cfg->module_id = SYSTEM_MODULE_ID;
	return 0;
}

API_JSON_MODULE_REGISTER(wt_sys_json_init)
