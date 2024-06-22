/*
 * SPDX-FileCopyrightText: 2024 kerms <kerms@niazo.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WT_SYSTEM_API_H_GUARD
#define WT_SYSTEM_API_H_GUARD

#define SYSTEM_MODULE_ID 0

typedef enum wt_system_cmd_t {
	WT_SYS_GET_FM_INFO = 1,
	WT_SYS_REBOOT = 2,
} wt_system_cmd_t;


#endif //WT_SYSTEM_API_H_GUARD