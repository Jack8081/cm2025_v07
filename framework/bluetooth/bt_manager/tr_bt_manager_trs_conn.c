/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager a2dp profile.
 */
#define SYS_LOG_DOMAIN "bt manager"

//#include <logging/sys_log.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <shell/shell.h>
#include <stream.h>
#include <acts_ringbuf.h>
#include <bt_manager.h>
#include <sys_event.h>
#include <app_manager.h>
#include <power_manager.h>
#include <bt_manager_inner.h>
#include "audio_policy.h"
#include "app_switch.h"
#include "btservice_api.h"
#include "media_player.h"
#include <volume_manager.h>
#include <audio_system.h>
#include <input_manager.h>
#include <acts_bluetooth/hci.h>


#define BT_MAX_AUTOCONN_DEV				3
#define BT_TRY_AUTOCONN_DEV				2
//#define BT_BASE_DEFAULT_RECONNECT_TRY	CONFIG_BT_AUTO_CONNECT_COUNTS
//#define BT_BASE_STARTUP_RECONNECT_TRY	CONFIG_BT_AUTO_CONNECT_PDL_COUNTS
#define BT_BASE_TIMEOUT_RECONNECT_TRY	15
#define BT_TWS_SLAVE_DISCONNECT_RETRY	10
//#define BT_BASE_RECONNECT_INTERVAL		6000
//#define BT_PROFILE_RECONNECT_TRY		CONFIG_BT_AUTO_CONNECT_PROFILE_COUNTS
//#define BT_PROFILE_RECONNECT_INTERVAL	3000

void tr_bt_manager_connected(void *addr)
{
    SYS_LOG_INF("\n");
    struct bt_set_autoconn reconnect_param;

	bt_manager_set_status(BT_STATUS_CONNECTING);
    bt_manager_event_notify(BT_CONNECTING_EVENT, NULL, 0);
	
    memset((u8_t *)&reconnect_param, 0, sizeof(reconnect_param));
    memcpy(reconnect_param.addr.val, addr, sizeof(bd_address_t));
    reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
    reconnect_param.base_try = BT_BASE_STARTUP_RECONNECT_TRY;
    reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
    reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
    reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
    reconnect_param.trs.a2dp = 1;//connect profile
    reconnect_param.trs.avrcp = 1;
    reconnect_param.trs.hfp = 1;
    btif_br_auto_reconnect(&reconnect_param);

	btif_br_set_connnectable(true);
	btif_br_set_discoverable(true);
}

void tr_bt_manager_disconnected_reason(void *param)
{
#ifndef CONFIG_UART_SLAVE
    struct bt_disconnect_reason *p_param = (struct bt_disconnect_reason *)param;
    struct bt_set_autoconn reconnect_param;

    SYS_LOG_INF("%d\n", p_param->reason);

    if (p_param->reason != BT_HCI_ERR_REMOTE_USER_TERM_CONN &&
            p_param->reason != BT_HCI_ERR_REMOTE_POWER_OFF &&
            p_param->reason != BT_HCI_ERR_LOCALHOST_TERM_CONN) {
        memcpy(reconnect_param.addr.val, p_param->addr.val, sizeof(bd_address_t));
        reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;

        reconnect_param.base_try = BT_BASE_DEFAULT_RECONNECT_TRY;

        if (p_param->tws_role == BTSRV_TWS_NONE &&
                p_param->reason == BT_HCI_ERR_CONN_TIMEOUT) {
            reconnect_param.base_try = BT_BASE_TIMEOUT_RECONNECT_TRY;
        }

        reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
        reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
        reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;

        reconnect_param.trs.a2dp = 1;//connect profile
        reconnect_param.trs.avrcp = 1;
        reconnect_param.trs.hfp = 1;
        btif_br_auto_reconnect(&reconnect_param);
    }
#endif
}
void tr_bt_manager_connect_stop(void)
{
    SYS_LOG_INF("\n");
	btif_br_auto_reconnect_stop(BTSRV_STOP_AUTO_RECONNECT_ALL);
}

void tr_bt_manager_disconnect(int index)
{
    SYS_LOG_INF("\n");
    tr_btif_br_disconnect_device(0, (void *)index);
}

int tr_bt_manager_startup_reconnect(void)
{
	u8_t phone_num, master_max_index;
	int cnt, i, phone_cnt, ret;
	struct autoconn_info *info;
	struct bt_set_autoconn reconnect_param;

/*
    if(api_bt_get_disable_status())
    {
        return 0;
    }
*/
	info = mem_malloc(sizeof(struct autoconn_info)*BT_MAX_AUTOCONN_DEV);
	if (info == NULL) {
		SYS_LOG_ERR("malloc failed");
		return 0;
	}

	cnt = btif_br_get_auto_reconnect_info(info, BT_MAX_AUTOCONN_DEV);
	ret = cnt;
	if (cnt == 0) {
		goto reconnnect_ext;
	}

	bt_manager_set_status(BT_STATUS_CONNECTING);
    bt_manager_event_notify(BT_CONNECTING_EVENT, NULL, 0);

	phone_cnt = 0;
	phone_num = bt_manager_config_connect_phone_num();

	cnt = (cnt > BT_TRY_AUTOCONN_DEV) ? BT_TRY_AUTOCONN_DEV : cnt;
	master_max_index = (phone_num > 1) ? phone_num : BT_MAX_AUTOCONN_DEV;

	for (i = 0; i < master_max_index; i++) {
		/* As tws master, connect first */
		if (info[i].addr_valid && (info[i].tws_role == BTSRV_TWS_MASTER) &&
			(info[i].a2dp || info[i].avrcp || info[i].hfp)) {
			memcpy(reconnect_param.addr.val, info[i].addr.val, sizeof(bd_address_t));
			reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
			reconnect_param.base_try = BT_BASE_STARTUP_RECONNECT_TRY;
			reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
			reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
			reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
			reconnect_param.trs.a2dp = info[i].a2dp;
			reconnect_param.trs.avrcp = info[i].avrcp;
			reconnect_param.trs.hfp = info[i].hfp;
            reconnect_param.reconnect_phone_timeout = BT_RECONNECT_PHONE_TIMEOUT;
            SYS_LOG_INF("333\n");
			btif_br_auto_reconnect(&reconnect_param);

			info[i].addr_valid = 0;
			cnt--;
			break;
		}
	}
    SYS_LOG_INF("444\n");

	for (i = 0; i < BT_MAX_AUTOCONN_DEV; i++) {
		if (info[i].addr_valid &&
			(info[i].a2dp || info[i].avrcp || info[i].hfp)) {
			if ((phone_cnt == phone_num) &&
				(info[i].tws_role == BTSRV_TWS_NONE)) {
				/* Config connect phone number */
                SYS_LOG_INF("i=%d\n",i);
				continue;
			}

			memcpy(reconnect_param.addr.val, info[i].addr.val, sizeof(bd_address_t));
			reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
			reconnect_param.base_try = BT_BASE_STARTUP_RECONNECT_TRY;
			reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
			reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
			reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
			reconnect_param.trs.a2dp = info[i].a2dp;
			reconnect_param.trs.avrcp = info[i].avrcp;
			reconnect_param.trs.hfp = info[i].hfp;
            reconnect_param.reconnect_phone_timeout = BT_RECONNECT_PHONE_TIMEOUT;
            SYS_LOG_INF("a2dp:%d,avrcp:%d,hfp:%d\n", info[i].a2dp, info[i].avrcp, info[i].hfp);
			btif_br_auto_reconnect(&reconnect_param);

			if (info[i].tws_role == BTSRV_TWS_NONE) {
				phone_cnt++;
			}

			cnt--;
			if (cnt == 0 || info[i].tws_role == BTSRV_TWS_SLAVE) {
				/* Reconnect tow device,
				 *	or as tws slave, just reconnect master, not reconnect phone
				 */
				break;
			}
		}
	}

reconnnect_ext:
	mem_free(info);
	return ret;
}


