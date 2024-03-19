/*
 * Copyright (c) 2021 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager config.
*/

#ifndef __TR_BT_MANAGER_CONFIG_H__
#define __TR_BT_MANAGER_CONFIG_H__

// 蓝牙设备基本配置
#define CFG_KEY_TR_BRMGR_BASE "TR_BTMGR_BASE"
typedef struct 
{
    unsigned char   trans_work_mode;                              //蓝牙收发模式
} tr_btmgr_base_config_t;

#endif

