/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TR_APP_SWITCH_H_
#define TR_APP_SWITCH_H_

typedef enum
{
    RECEIVER_MODE = 0,
    TRANSFER_MODE
} tr_app_work_mode_e;


void sys_set_work_mode(tr_app_work_mode_e mode);
u8_t sys_get_work_mode(void);
void sys_set_switch_flag(void);
u8_t sys_get_clear_switch_flag(void);

bool tr_app_switch_is_in_current_work_mode(const char *app_name);
uint8_t tr_get_current_work_mode_active_app_num(void);
void tr_app_switch_list_show(void);
void tr_app_switch_transceiver_mode(s8_t transceiver_mode);

int tr_sys_work_mode_init(void);
#endif /* TR_APP_SWITCH_H_ */
