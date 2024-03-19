/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager.
*/

#ifndef __TR_BT_MANAGER_INNER_H__
#define __TR_BT_MANAGER_INNER_H__
#include <stream.h>
#include "btservice_api.h"

#define DEV_INQURY_INFO_NUM 20
#define BD_ADDR_LEN			6

/* 设备可见性和可连接性*/
#define TRANS_VISUAL_MODE_NODISC_NOCON         0 //设备不可见，不可连接
#define TRANS_VISUAL_MODE_NODISC_CON           1 //设备不可见，可连接
#define TRANS_VISUAL_MODE_DISC_NOCON           2 //设备可见，不可连接
#define TRANS_VISUAL_MODE_DISC_CON             3 //设备可见，可连接

/* 启动后动作模式
 * CONFIG_BT_AUTO_CONNECT_STARTUP_MODE
*/
#define STARTUP_MODE_ALWAYS_INQUIRY            1 //开机就搜索
#define STARTUP_MODE_RECONNECT_PAIRDLIST       2 //开机先回连配对列表

/* 断开后动作模式
 * CONFIG_BT_AUTO_CONNECT_DISCONNECT_MODE
*/
#define DISCONNECT_MODE_ALWAYS_INQUIRY            1 //搜索并连接
#define DISCONNECT_MODE_RECONNECT_PAIRDLIST       2 //回连配对列表
#define DISCONNECT_MODE_INQUIRY_RECONNECT_ALTER   3 //搜索与回连配对列表交替
#define DISCONNECT_MODE_WAIT_RMT_RECONNECT        4 //打开可连性，等待远程设备回连

/* 搜索完，没有符合条件的结果，动作模式
 * CONFIG_BT_AUTO_CONNECT_INQU_RESULT_FAIL_STANDARD_MODE
*/
#define INQU_RESULT_FAIL_MODE_ALWAYS_INQUIRY            1 //搜索并连接
#define INQU_RESULT_FAIL_MODE_RECONNECT_PAIRDLIST       2 //回连配对列表
#define INQU_RESULT_FAIL_MODE_INQUIRY_RECONNECT_ALTER   3 //搜索与回连配对列表交替




void _bt_manager_trs_event_notify(int event_type, void *param, int param_size);
void tr_bt_mgr_link_event(struct bt_link_cb_param *param);


int tr_bt_manager_startup_reconnect(void);

int bt_manager_hfp_ag_profile_start(void);
int bt_manager_hfp_ag_profile_stop(void);
bool tr_check_dev_first_passthrough_play(bd_address_t *addr);
void tr_bt_manager_set_disconnect_reason(int reason);
int tr_bt_manager_get_disconnect_reason(void);
#endif
