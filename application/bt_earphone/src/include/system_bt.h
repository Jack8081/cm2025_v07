/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system bt headfile
 */

#ifndef _SYSTEM_BT_H_
#define _SYSTEM_BT_H_

//#define ENABLE_LEAUDIO_MONO /* this second */

int sys_bt_init(void);

int sys_bt_deinit(void);

int sys_bt_ready(void);


int sys_bt_restart_leaudio(void);

#endif //_SYSTEM_BT_H_