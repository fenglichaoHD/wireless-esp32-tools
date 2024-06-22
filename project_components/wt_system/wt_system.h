/*
 * SPDX-FileCopyrightText: 2024 kerms <kerms@niazo.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WT_SYSTEM_H_GUARD
#define WT_SYSTEM_H_GUARD

#include "global_module.h"

typedef struct wt_fm_info_t {
	char fm_ver[32];
	char upd_date[11];
} wt_fm_info_t;

void wt_system_get_fm_info(wt_fm_info_t *fm_info);

/**
 * Trigger delayed reboot in 2 seconds
 */
void wt_system_reboot();


#endif //WT_SYSTEM_H_GUARD