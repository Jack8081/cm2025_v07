/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager connect.
 */
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <sys_event.h>
#include <acts_bluetooth/host_interface.h>
#include <mem_manager.h>
#include <bt_manager.h>
#include "bt_manager_inner.h"
#include <property_manager.h>
#include "btservice_api.h"
#include "bt_porting_inner.h"

#ifdef CONFIG_BT_EARPHONE_SPEC
bool bt_manager_startup_reconnect(void)
{
	uint8_t phone_num;
    uint8_t tws_reconnect_mode;
	uint8_t i;
	uint8_t phone_cnt = 0;
	uint8_t tws_paired = 0;
	int cnt = 0;
	struct autoconn_info *info;
	struct bt_set_autoconn reconnect_param;
	btmgr_tws_pair_cfg_t *cfg_tws =  bt_manager_get_tws_pair_config();
    btmgr_pair_cfg_t * cfg_pair = bt_manager_get_pair_config();
	btmgr_feature_cfg_t *cfg_feature = bt_manager_get_feature_config();
	btmgr_reconnect_cfg_t *cfg_reconnect = bt_manager_get_reconnect_config();
    struct bt_manager_context_t*  bt_manager = bt_manager_get_context();

	info = bt_mem_malloc(sizeof(struct autoconn_info)*BT_MAX_AUTOCONN_DEV);
	if (info == NULL) {
		SYS_LOG_ERR("malloc failed");
		return false;
	}

	cnt = btif_br_get_auto_reconnect_info(info, BT_MAX_AUTOCONN_DEV);

    if(cnt > 0){
        for (i = 0; i < BT_MAX_AUTOCONN_DEV; i++){
            if(info[i].addr_valid &&
                ((info[i].tws_role == BTSRV_TWS_MASTER) || (info[i].tws_role == BTSRV_TWS_SLAVE))){
                tws_paired = 1;
                break;
            }
        }
    }

	cnt = (cnt > cfg_feature->sp_device_num) ? cfg_feature->sp_device_num : cnt;

    if( tws_paired){
		tws_reconnect_mode = BTSRV_TWS_RECONNECT_FAST_PAIR;
    }
	else if(cfg_tws->power_on_auto_pair_search){
		tws_reconnect_mode = BTSRV_TWS_RECONNECT_AUTO_PAIR;
    }
    else{
		tws_reconnect_mode = BTSRV_TWS_RECONNECT_NONE;
    }

	if(tws_reconnect_mode){
		if(tws_reconnect_mode == BTSRV_TWS_RECONNECT_FAST_PAIR){
            memcpy(reconnect_param.addr.val, info[i].addr.val, sizeof(bd_address_t));
		}
		else{
            btif_br_get_local_mac(&reconnect_param.addr);
            memset(&reconnect_param.addr.val[0], 0, 3);
		}
		if (tws_paired) {
            info[i].addr_valid = 0;
            cnt--;
		}
        reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
        reconnect_param.base_try = cfg_reconnect->reconnect_tws_times_by_startup;
        reconnect_param.profile_try = 0;
        reconnect_param.base_interval = cfg_reconnect->reconnect_tws_interval;
        reconnect_param.profile_interval = 0;
		reconnect_param.reconnect_mode = tws_reconnect_mode;
        reconnect_param.reconnect_phone_timeout = cfg_reconnect->reconnect_phone_timeout;
        reconnect_param.reconnect_tws_timeout = cfg_reconnect->reconnect_tws_timeout;
		bt_manager->auto_reconnect_startup = true;

		if (tws_reconnect_mode == BTSRV_TWS_RECONNECT_AUTO_PAIR)
		{
		    int times = (cfg_tws->wait_pair_search_time_sec * 1000) / 
		        (cfg_reconnect->reconnect_tws_interval * 100);

		    if (times > 250)
		        times = 250;

		    reconnect_param.base_try = times;
		}
        btif_br_auto_reconnect(&reconnect_param);

		SYS_LOG_INF("TWS: %x:%x:%x:%x:%x:%x",
			reconnect_param.addr.val[5],
			reconnect_param.addr.val[4],
			reconnect_param.addr.val[3],
			reconnect_param.addr.val[2],
			reconnect_param.addr.val[1],
			reconnect_param.addr.val[0]);
	}

	phone_cnt = 0;
	phone_num = bt_manager_config_connect_phone_num();

	if((cfg_reconnect->enable_auto_reconnect & (1 << 0)) != 0){
        for (i = 0; i < BT_MAX_AUTOCONN_DEV; i++) {
            if (info[i].addr_valid &&
                (info[i].a2dp || info[i].avrcp || info[i].hfp)) {
                if (phone_cnt == phone_num){
                    break;
                }
                memcpy(reconnect_param.addr.val, info[i].addr.val, sizeof(bd_address_t));
                reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
                reconnect_param.base_try = cfg_reconnect->reconnect_phone_times_by_startup;
                reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
                reconnect_param.base_interval = cfg_reconnect->reconnect_phone_interval;
                reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
                reconnect_param.reconnect_mode = BTSRV_TWS_RECONNECT_NONE;
				reconnect_param.reconnect_phone_timeout = cfg_reconnect->reconnect_phone_timeout;
				reconnect_param.reconnect_tws_timeout = cfg_reconnect->reconnect_tws_timeout;
                bt_manager->auto_reconnect_startup = true;
                btif_br_auto_reconnect(&reconnect_param);

                SYS_LOG_INF("PHONE:%x:%x:%x:%x:%x:%x",
                    reconnect_param.addr.val[5],
                    reconnect_param.addr.val[4],
                    reconnect_param.addr.val[3],
                    reconnect_param.addr.val[2],
                    reconnect_param.addr.val[1],
                    reconnect_param.addr.val[0]);

                if (info[i].tws_role == BTSRV_TWS_NONE) {
                    phone_cnt++;
                }
                cnt--;
                if (cnt == 0) {
                    break;
                }
            }
        }
    }

	bt_mem_free(info);

	if((phone_cnt == 0) && (cfg_pair->enter_pair_mode_when_paired_list_empty)){
		if(cfg_pair->clear_paired_list_when_enter_pair_mode){
			btif_br_clear_list(BTSRV_DEVICE_PHONE);
		}
		bt_manager->stop_reconnect_tws_in_pair_mode = 0;
		bt_manager->pair_mode_state = BT_PAIR_MODE_START_PAIR;
        bt_manager->enter_pair_mode_state_update = true;
		bt_manager_enter_pair_mode_ex();
	}
	return true;
}
#else
void bt_manager_startup_reconnect(void)
{
	uint8_t phone_num, master_max_index;
	int cnt, i, phone_cnt;
	struct autoconn_info *info;
	struct bt_set_autoconn reconnect_param;

	info = bt_mem_malloc(sizeof(struct autoconn_info)*BT_MAX_AUTOCONN_DEV);
	if (info == NULL) {
		SYS_LOG_ERR("malloc failed");
		return;
	}

	cnt = btif_br_get_auto_reconnect_info(info, BT_MAX_AUTOCONN_DEV);
	if (cnt == 0) {
		goto reconnnect_ext;
	}

	phone_cnt = 0;
	phone_num = bt_manager_config_connect_phone_num();

	cnt = (cnt > BT_TRY_AUTOCONN_DEV) ? BT_TRY_AUTOCONN_DEV : cnt;
	master_max_index = (phone_num > 1) ? phone_num : BT_MAX_AUTOCONN_DEV;

	for (i = 0; i < master_max_index; i++) {
		/* As tws master, connect first */
		if (info[i].addr_valid && (info[i].tws_role == BTSRV_TWS_MASTER) &&
			(info[i].a2dp || info[i].avrcp || info[i].hfp || info[i].hid)) {
			memcpy(reconnect_param.addr.val, info[i].addr.val, sizeof(bd_address_t));
			reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
			reconnect_param.base_try = BT_BASE_STARTUP_RECONNECT_TRY;
			reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
			reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
			reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
            reconnect_param.reconnect_phone_timeout = BT_RECONNECT_PHONE_TIMEOUT;
            reconnect_param.reconnect_tws_timeout = BT_RECONNECT_TWS_TIMEOUT;
			btif_br_auto_reconnect(&reconnect_param);

			info[i].addr_valid = 0;
			cnt--;
			break;
		}
	}

	for (i = 0; i < BT_MAX_AUTOCONN_DEV; i++) {
		if (info[i].addr_valid &&
			(info[i].a2dp || info[i].avrcp || info[i].hfp|| info[i].hid)) {
			if ((phone_cnt == phone_num) &&
				(info[i].tws_role == BTSRV_TWS_NONE)) {
				/* Config connect phone number */
				continue;
			}

			memcpy(reconnect_param.addr.val, info[i].addr.val, sizeof(bd_address_t));
			reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
			reconnect_param.base_try = BT_BASE_STARTUP_RECONNECT_TRY;
			reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
			reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
			reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
            reconnect_param.reconnect_phone_timeout = BT_RECONNECT_PHONE_TIMEOUT;
            reconnect_param.reconnect_tws_timeout = BT_RECONNECT_TWS_TIMEOUT;
			btif_br_auto_reconnect(&reconnect_param);

			if (info[i].tws_role == BTSRV_TWS_NONE) {
				phone_cnt++;
			}

			cnt--;
			if (cnt == 0 || info[i].tws_role == BTSRV_TWS_SLAVE) {
				/* Reconnect tow device,
				 *  or as tws slave, just reconnect master, not reconnect phone
				 */
				break;
			}
		}
	}

reconnnect_ext:
	bt_mem_free(info);
}
#endif

bool bt_manager_manual_reconnect(void)
{
    struct bt_manager_context_t*  bt_manager = bt_manager_get_context();
    btmgr_feature_cfg_t *cfg_feature = bt_manager_get_feature_config();
#ifdef CONFIG_BT_EARPHONE_SPEC
    btmgr_reconnect_cfg_t *cfg_reconnect = bt_manager_get_reconnect_config();
#endif
    uint8_t role = bt_manager_tws_get_dev_role();
    uint8_t connect_dev = bt_manager_get_connected_dev_num();
	uint8_t cnt,i;
    uint8_t phone_num,phone_cnt;
    uint8_t tws_paired = 0;
	struct autoconn_info *info;
	struct bt_set_autoconn reconnect_param;

	SYS_LOG_INF("dev:%d role:%d search:%d",connect_dev,role,bt_manager_is_tws_pair_search());

    if ((connect_dev >= cfg_feature->sp_device_num)
		|| (role == BTSRV_TWS_SLAVE)
		|| (bt_manager_is_tws_pair_search() == true)){
        return false;
	}

	info = bt_mem_malloc(sizeof(struct autoconn_info)*BT_MAX_AUTOCONN_DEV);
	if (info == NULL) {
		SYS_LOG_ERR("malloc failed");
		return false;
	}

	cnt = btif_br_get_auto_reconnect_info(info, BT_MAX_AUTOCONN_DEV);
	if(cnt == 0){
	    bt_mem_free(info);
		return false;
	}

    bt_manager_end_pair_mode();

    btif_br_auto_reconnect_stop(BTSRV_STOP_AUTO_RECONNECT_ALL);

    for (i = 0; i < BT_MAX_AUTOCONN_DEV; i++){
        if(info[i].addr_valid &&
            ((info[i].tws_role == BTSRV_TWS_MASTER) || (info[i].tws_role == BTSRV_TWS_SLAVE))){
            tws_paired = 1;
            break;
        }
    }

	cnt = (cnt > cfg_feature->sp_device_num) ? cfg_feature->sp_device_num : cnt;

    if(tws_paired){
        memcpy(reconnect_param.addr.val, info[i].addr.val, sizeof(bd_address_t));
		info[i].addr_valid = 0;
		
    #ifdef CONFIG_BT_EARPHONE_SPEC
        reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
        reconnect_param.base_try = cfg_reconnect->reconnect_tws_times_by_startup;
        reconnect_param.profile_try = 0;
        reconnect_param.base_interval = cfg_reconnect->reconnect_tws_interval;
        reconnect_param.profile_interval = 0;
        reconnect_param.reconnect_phone_timeout = cfg_reconnect->reconnect_phone_timeout;
        reconnect_param.reconnect_tws_timeout = cfg_reconnect->reconnect_tws_timeout;
    #else
		reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
		reconnect_param.base_try = BT_BASE_STARTUP_RECONNECT_TRY;
		reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
        reconnect_param.reconnect_phone_timeout = BT_RECONNECT_PHONE_TIMEOUT;
        reconnect_param.reconnect_tws_timeout = BT_RECONNECT_TWS_TIMEOUT;
		reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
		reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
    #endif
		reconnect_param.reconnect_mode = BTSRV_TWS_RECONNECT_FAST_PAIR;
		cnt--;
		btif_br_auto_reconnect(&reconnect_param);
    }

	phone_cnt = 0;
	phone_num = bt_manager_config_connect_phone_num();

	for (i = 0; i < BT_MAX_AUTOCONN_DEV; i++) {
		if (info[i].addr_valid &&
			(info[i].a2dp || info[i].avrcp || info[i].hfp)) {
			if (phone_cnt == phone_num) {
                break;
			}

			memcpy(reconnect_param.addr.val, info[i].addr.val, sizeof(bd_address_t));
			
        #ifdef CONFIG_BT_EARPHONE_SPEC
            reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
            reconnect_param.base_try = cfg_reconnect->reconnect_phone_times_by_startup;
            reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
            reconnect_param.base_interval = cfg_reconnect->reconnect_phone_interval;
            reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
            reconnect_param.reconnect_mode = BTSRV_TWS_RECONNECT_NONE;
            reconnect_param.reconnect_phone_timeout = cfg_reconnect->reconnect_phone_timeout;
            reconnect_param.reconnect_tws_timeout = cfg_reconnect->reconnect_tws_timeout;
        #else
			reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
			reconnect_param.base_try = BT_BASE_STARTUP_RECONNECT_TRY;
			reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
			reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
			reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
			reconnect_param.reconnect_mode = BTSRV_TWS_RECONNECT_NONE;
			reconnect_param.reconnect_phone_timeout = BT_RECONNECT_PHONE_TIMEOUT;
			reconnect_param.reconnect_tws_timeout = BT_RECONNECT_TWS_TIMEOUT;
        #endif
			btif_br_auto_reconnect(&reconnect_param);

			if (info[i].tws_role == BTSRV_TWS_NONE) {
				phone_cnt++;
			}
			info[i].addr_valid = 0;
			cnt--;
			if (cnt == 0) {
				break;
			}
		}
	}

	bt_mem_free(info);

    if(bt_manager->bt_state == BT_STATUS_LINK_NONE){
        bt_manager_set_status(BT_STATUS_WAIT_CONNECT);
    }

    return true;
}

void bt_manager_startup_reconnect_phone(void)
{
	uint8_t phone_num;
	int cnt, i, phone_cnt;
	struct autoconn_info *info;
	struct bt_set_autoconn reconnect_param;
#ifdef CONFIG_BT_EARPHONE_SPEC
	btmgr_reconnect_cfg_t *cfg_reconnect = bt_manager_get_reconnect_config();
#endif

	info = bt_mem_malloc(sizeof(struct autoconn_info)*BT_MAX_AUTOCONN_DEV);
	if (info == NULL) {
		SYS_LOG_ERR("malloc failed");
		return;
	}

	cnt = btif_br_get_auto_reconnect_info(info, BT_MAX_AUTOCONN_DEV);
	if (cnt == 0) {
		goto reconnnect_ext;
	}

	phone_cnt = 0;
	phone_num = bt_manager_config_connect_phone_num();

	cnt = (cnt > BT_TRY_AUTOCONN_DEV) ? BT_TRY_AUTOCONN_DEV : cnt;

	for (i = 0; i < BT_TRY_AUTOCONN_DEV; i++) {
		if (info[i].addr_valid &&
			(info[i].a2dp || info[i].avrcp || info[i].hfp)) {
			if ((phone_cnt == phone_num) ||
				(info[i].tws_role != BTSRV_TWS_NONE)) {
				/* Config connect phone number */
				continue;
			}

			memcpy(reconnect_param.addr.val, info[i].addr.val, sizeof(bd_address_t));

#ifdef CONFIG_BT_EARPHONE_SPEC
			reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
			reconnect_param.base_try = cfg_reconnect->reconnect_phone_times_by_startup;
			reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
			reconnect_param.base_interval = cfg_reconnect->reconnect_phone_interval;
			reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
			reconnect_param.reconnect_mode = BTSRV_TWS_RECONNECT_NONE;
			reconnect_param.reconnect_phone_timeout = cfg_reconnect->reconnect_phone_timeout;
			reconnect_param.reconnect_tws_timeout = cfg_reconnect->reconnect_tws_timeout;
#else
			reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
			reconnect_param.base_try = BT_BASE_STARTUP_RECONNECT_TRY;
			reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
			reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
			reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
			reconnect_param.reconnect_mode = BTSRV_TWS_RECONNECT_NONE;
			reconnect_param.reconnect_phone_timeout = BT_RECONNECT_PHONE_TIMEOUT;
			reconnect_param.reconnect_tws_timeout = BT_RECONNECT_TWS_TIMEOUT;
#endif
			btif_br_auto_reconnect(&reconnect_param);

			SYS_LOG_INF("PHONE:%x:%x:%x:%x:%x:%x",
				reconnect_param.addr.val[5],
				reconnect_param.addr.val[4],
				reconnect_param.addr.val[3],
				reconnect_param.addr.val[2],
				reconnect_param.addr.val[1],
				reconnect_param.addr.val[0]);
			
			phone_cnt++;
			cnt--;
			if (cnt == 0) {
				/* Reconnect tow device,
				 *  or as tws slave, just reconnect master, not reconnect phone
				 */
				break;
			}
		}
	}

reconnnect_ext:
	bt_mem_free(info);
}

#ifndef CONFIG_BT_EARPHONE_SPEC
static bool bt_manager_check_reconnect_enable(void)
{
	bool auto_reconnect = true;
	char temp[16];

	memset(temp, 0, sizeof(temp));

#ifdef CONFIG_PROPERTY
	if (property_get(CFG_AUTO_RECONNECT, temp, 16) > 0) {
		if (strcmp(temp, "false") == 0) {
			auto_reconnect = false;
		}
	}
#endif

	return auto_reconnect;
}
#endif

void bt_manager_sync_auto_reconnect_status(bd_address_t *addr)
{
	struct bt_set_autoconn reconnect_param;
	btmgr_reconnect_cfg_t *cfg_reconnect = bt_manager_get_reconnect_config();
	memcpy(&reconnect_param.addr,addr,sizeof(bd_address_t));

	SYS_LOG_INF("addr:%x:%x:%x:%x:%x:%x",
		addr->val[0],
        addr->val[1],
        addr->val[2],
        addr->val[3],
        addr->val[4],
        addr->val[5]);

    reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;
    reconnect_param.base_try = cfg_reconnect->reconnect_times_by_timeout;
    reconnect_param.reconnect_mode = BTSRV_TWS_RECONNECT_NONE;
    reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
    reconnect_param.base_interval = cfg_reconnect->reconnect_phone_interval;
    reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
    reconnect_param.reconnect_phone_timeout = cfg_reconnect->reconnect_phone_timeout;
    reconnect_param.reconnect_tws_timeout = cfg_reconnect->reconnect_tws_timeout;
	btif_br_auto_reconnect(&reconnect_param);
}

void bt_manager_disconnected_reason(void *param)
{
	struct bt_disconnect_reason *p_param = (struct bt_disconnect_reason *)param;
	struct bt_set_autoconn reconnect_param;
	btmgr_reconnect_cfg_t *cfg_reconnect = bt_manager_get_reconnect_config();

	SYS_LOG_INF("0x%x, tws_role %d reason 0x%x", 
	    p_param->conn_type, p_param->tws_role,p_param->reason
	);

#ifndef CONFIG_BT_EARPHONE_SPEC
	if (!bt_manager_check_reconnect_enable()) {
		SYS_LOG_WRN("Disable do reconnect\n");
		return;
	}

	if (p_param->tws_role == BTSRV_TWS_MASTER) {
		/* Just let slave device do reconnect */
		return;
	}
#endif


#ifdef CONFIG_BT_EARPHONE_SPEC
    if(p_param->reason != BT_HCI_ERR_REMOTE_POWER_OFF
        && p_param->reason != BT_HCI_ERR_LOCALHOST_TERM_CONN
        && p_param->reason != BT_HCI_ERR_REMOTE_USER_TERM_CONN
        && p_param->reason != BT_HCI_ERR_CONN_ALREADY_EXISTS){

        if(p_param->conn_type == BT_CONN_TYPE_BR_SNOOP){
			return;
        }

		if(p_param->tws_role == BTSRV_TWS_SLAVE){
            return;
		}

        memcpy(reconnect_param.addr.val, p_param->addr.val, sizeof(bd_address_t));
        reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;

		if (p_param->tws_role == BTSRV_TWS_NONE){
            if ((p_param->reason == BT_HCI_ERR_CONN_TIMEOUT || 
                    p_param->reason == BT_HCI_ERR_LL_RESP_TIMEOUT)
			    && ((cfg_reconnect->enable_auto_reconnect & (1 << 1)) == 0)){
			    return;
			}
			reconnect_param.base_try = cfg_reconnect->reconnect_times_by_timeout;
			reconnect_param.reconnect_mode = BTSRV_TWS_RECONNECT_NONE;
			reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
		    reconnect_param.base_interval = cfg_reconnect->reconnect_phone_interval;
		    reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
            reconnect_param.reconnect_phone_timeout = cfg_reconnect->reconnect_phone_timeout;
            reconnect_param.reconnect_tws_timeout = cfg_reconnect->reconnect_tws_timeout;
		}
        else{
            reconnect_param.reconnect_mode = BTSRV_TWS_RECONNECT_FAST_PAIR;
            reconnect_param.base_try = cfg_reconnect->reconnect_times_by_timeout;
            reconnect_param.profile_try = 0;
            reconnect_param.base_interval = cfg_reconnect->reconnect_tws_interval;
            reconnect_param.profile_interval = 0;
            reconnect_param.reconnect_phone_timeout = cfg_reconnect->reconnect_phone_timeout;
            reconnect_param.reconnect_tws_timeout = cfg_reconnect->reconnect_tws_timeout;
        }
		btif_br_auto_reconnect(&reconnect_param);
    }
#else
    if(p_param->reason != BT_HCI_ERR_REMOTE_USER_TERM_CONN
        && p_param->reason != BT_HCI_ERR_REMOTE_POWER_OFF
        &&p_param->reason != BT_HCI_ERR_LOCALHOST_TERM_CONN){
        memcpy(reconnect_param.addr.val, p_param->addr.val, sizeof(bd_address_t));
        reconnect_param.strategy = BTSRV_AUTOCONN_ALTERNATE;

		if(p_param->tws_role == BTSRV_TWS_SLAVE) {
			reconnect_param.base_try = BT_TWS_SLAVE_DISCONNECT_RETRY;
		} else {
			reconnect_param.base_try = BT_BASE_DEFAULT_RECONNECT_TRY;
		}

		if (p_param->tws_role == BTSRV_TWS_NONE &&
			p_param->reason == BT_HCI_ERR_CONN_TIMEOUT) {
			reconnect_param.base_try = BT_BASE_TIMEOUT_RECONNECT_TRY;
		}

		reconnect_param.profile_try = BT_PROFILE_RECONNECT_TRY;
		reconnect_param.base_interval = BT_BASE_RECONNECT_INTERVAL;
		reconnect_param.profile_interval = BT_PROFILE_RECONNECT_INTERVAL;
        reconnect_param.reconnect_phone_timeout = BT_RECONNECT_PHONE_TIMEOUT;
        reconnect_param.reconnect_tws_timeout = BT_RECONNECT_TWS_TIMEOUT;
		btif_br_auto_reconnect(&reconnect_param);
    }
#endif
}
static bool halted_phone = false;
void bt_manager_halt_phone(void)
{
#ifdef CONFIG_TWS
	/**only master need halt phone*/
#ifdef CONFIG_BT_TRANSCEIVER
    if(sys_get_work_mode() == RECEIVER_MODE)
#endif
	if (bt_manager_tws_get_dev_role() != BTSRV_TWS_MASTER) {
		return;
	}
#endif
    bt_manager_set_user_visual(1,0,0,BTSRV_SCAN_MODE_DEFAULT_INQUIRY_PAGE);
	btif_br_auto_reconnect_stop(BTSRV_STOP_AUTO_RECONNECT_ALL);
	btif_br_disconnect_device(BTSRV_DISCONNECT_PHONE_MODE);
	halted_phone = true;
	SYS_LOG_INF("");
}

void bt_manager_resume_phone(void)
{
	if(halted_phone) {
		bt_manager_set_user_visual(0,0,0,0);
		bt_manager_startup_reconnect_phone();
		halted_phone = false;
		SYS_LOG_INF("");
	}
}

#ifdef CONFIG_BT_BR_TRANSCEIVER
void bt_manager_set_connectable(bool connectable)
{
	btif_br_set_connnectable(connectable);
	btif_br_set_discoverable(connectable);
}
#endif

void bt_manager_set_user_visual(bool enable,bool discoverable, bool connectable, uint8_t scan_mode)
{
    struct bt_set_user_visual user_visual;
	user_visual.enable = enable;
	user_visual.discoverable = discoverable;
	user_visual.connectable = connectable;
	user_visual.scan_mode = scan_mode;
    btif_br_set_user_visual(&user_visual);
}

bool bt_manager_is_auto_reconnect_runing(void)
{
	return btif_br_is_auto_reconnect_runing();
}

void bt_manager_auto_reconnect_stop(void)
{
	if (btif_br_is_auto_reconnect_runing()) {
		btif_br_auto_reconnect_stop(BTSRV_STOP_AUTO_RECONNECT_ALL);
	}
}

int bt_manager_get_linkkey(struct bt_linkkey_info *info, uint8_t cnt)
{
	return btif_br_get_linkkey(info, cnt);
}

int bt_manager_update_linkkey(struct bt_linkkey_info *info, uint8_t cnt)
{
	return btif_br_update_linkkey(info, cnt);
}

int bt_manager_write_ori_linkkey(bd_address_t *addr, uint8_t *link_key)
{
	return btif_br_write_ori_linkkey(addr, link_key);
}

void bt_manager_disconnect_all_device(void)
{
	int time_out = 0;

	btif_br_disconnect_device(BTSRV_DISCONNECT_ALL_MODE);

	while (btif_br_get_connected_device_num() && time_out++ < 500) {
		os_sleep(10);
	}
}

void bt_manager_profile_disconnected_delay_proc(os_work* work)
{
    bt_mgr_dev_info_t* dev_info = 
		CONTAINER_OF(work, bt_mgr_dev_info_t, profile_disconnected_delay_work);

	/* if main profiles disconnected, but ACL not disconnected yet, will force disconnect
	 */
    if (btif_tws_get_dev_role() != BTSRV_TWS_SLAVE &&
        !dev_info->hf_connected &&
        !dev_info->a2dp_connected)
    {
        SYS_LOG_INF("");
        dev_info->force_disconnect_by_remote = true;
        btif_br_disconnect(&dev_info->addr);
    }
}

