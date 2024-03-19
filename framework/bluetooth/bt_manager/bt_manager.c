/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager.
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <soc.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <srv_manager.h>

#include <bt_manager.h>
#include "bt_manager_inner.h"
#include <sys_event.h>
#include <btservice_api.h>
#include <dvfs.h>
#include <acts_bluetooth/host_interface.h>
#include <property_manager.h>
#include <drivers/bluetooth/bt_drv.h>
#include <drivers/adc.h>
#include <drivers/cfg_drv/dev_config.h>
#include <sys_wakelock.h>

#ifdef CONFIG_BT_TRANSCEIVER //CONFIG_BT_BR_TRANSCEIVER
#include "tr_bt_manager_config.c"
#endif

static struct bt_manager_context_t bt_manager_context;

static btmgr_base_config_t bt_manager_base_config;
static btmgr_pair_cfg_t bt_manager_pair_config;
static btmgr_tws_pair_cfg_t bt_manager_tws_pair_config;
static btmgr_feature_cfg_t bt_manager_feature_config;
static btmgr_sync_ctrl_cfg_t bt_manager_sync_ctrl_config;
static btmgr_reconnect_cfg_t bt_manager_reconnect_config;
static btmgr_ble_cfg_t ble_config;
static btmgr_rf_param_cfg_t bt_rf_param_config;
static btmgr_leaudio_cfg_t leaudio_config;


static const bt_mgr_event_strmap_t bt_manager_status_map[] =
{
    {BT_STATUS_LINK_NONE,                   STRINGIFY(BT_STATUS_LINK_NONE)},
    {BT_STATUS_WAIT_CONNECT,    			STRINGIFY(BT_STATUS_WAIT_CONNECT)},
    {BT_STATUS_PAIR_MODE,                   STRINGIFY(BT_STATUS_PAIR_MODE)},
    {BT_STATUS_CONNECTED,                   STRINGIFY(BT_STATUS_CONNECTED)},
    {BT_STATUS_DISCONNECTED,                STRINGIFY(BT_STATUS_DISCONNECTED)},
    {BT_STATUS_TWS_PAIRED,                  STRINGIFY(BT_STATUS_TWS_PAIRED)},
	{BT_STATUS_TWS_UNPAIRED,                STRINGIFY(BT_STATUS_TWS_UNPAIRED)},
    {BT_STATUS_TWS_PAIR_SEARCH,             STRINGIFY(BT_STATUS_TWS_PAIR_SEARCH)},
};

static const bt_mgr_event_strmap_t bt_manager_link_event_map[] =
{
    {BT_LINK_EV_ACL_CONNECT_REQ,            STRINGIFY(BT_LINK_EV_ACL_CONNECT_REQ)},
    {BT_LINK_EV_ACL_CONNECTED,    			STRINGIFY(BT_LINK_EV_ACL_CONNECTED)},
    {BT_LINK_EV_ACL_DISCONNECTED,           STRINGIFY(BT_LINK_EV_ACL_DISCONNECTED)},
    {BT_LINK_EV_GET_NAME,                   STRINGIFY(BT_LINK_EV_GET_NAME)},
    {BT_LINK_EV_HF_CONNECTED,               STRINGIFY(BT_LINK_EV_HF_CONNECTED)},
    {BT_LINK_EV_HF_DISCONNECTED,            STRINGIFY(BT_LINK_EV_HF_DISCONNECTED)},
	{BT_LINK_EV_A2DP_CONNECTED,             STRINGIFY(BT_LINK_EV_A2DP_CONNECTED)},
    {BT_LINK_EV_A2DP_DISCONNECTED,          STRINGIFY(BT_LINK_EV_A2DP_DISCONNECTED)},
	{BT_LINK_EV_AVRCP_CONNECTED,            STRINGIFY(BT_LINK_EV_AVRCP_CONNECTED)},
    {BT_LINK_EV_AVRCP_DISCONNECTED,         STRINGIFY(BT_LINK_EV_AVRCP_DISCONNECTED)},
    {BT_LINK_EV_SPP_CONNECTED,              STRINGIFY(BT_LINK_EV_SPP_CONNECTED)},
    {BT_LINK_EV_SPP_DISCONNECTED,           STRINGIFY(BT_LINK_EV_SPP_DISCONNECTED)},
    {BT_LINK_EV_HID_CONNECTED,              STRINGIFY(BT_LINK_EV_HID_CONNECTED)},
    {BT_LINK_EV_HID_DISCONNECTED,           STRINGIFY(BT_LINK_EV_HID_DISCONNECTED)},
    {BT_LINK_EV_SNOOP_ROLE,                 STRINGIFY(BT_LINK_EV_SNOOP_ROLE)},
};

static const bt_mgr_event_strmap_t bt_manager_service_event_map[] =
{
    {BTSRV_READY,                           STRINGIFY(BTSRV_READY)},
    {BTSRV_LINK_EVENT,    			        STRINGIFY(BTSRV_LINK_EVENT)},
    {BTSRV_DISCONNECTED_REASON,             STRINGIFY(BTSRV_DISCONNECTED_REASON)},
    {BTSRV_REQ_HIGH_PERFORMANCE,            STRINGIFY(BTSRV_REQ_HIGH_PERFORMANCE)},
    {BTSRV_RELEASE_HIGH_PERFORMANCE,        STRINGIFY(BTSRV_RELEASE_HIGH_PERFORMANCE)},
    {BTSRV_REQ_FLUSH_PROPERTY,              STRINGIFY(BTSRV_REQ_FLUSH_PROPERTY)},
    {BTSRV_CHECK_NEW_DEVICE_ROLE,           STRINGIFY(BTSRV_CHECK_NEW_DEVICE_ROLE)},
    {BTSRV_AUTO_RECONNECT_COMPLETE,         STRINGIFY(BTSRV_AUTO_RECONNECT_COMPLETE)},
    {BTSRV_SYNC_AOTO_RECONNECT_STATUS, 		STRINGIFY(BTSRV_SYNC_AOTO_RECONNECT_STATUS)},
};

const char *bt_manager_evt2str(int num, int max_num, const bt_mgr_event_strmap_t *strmap)
{
	int low = 0;
	int hi = max_num - 1;
	int mid;

	do {
		mid = (low + hi) >> 1;
		if (num > strmap[mid].event) {
			low = mid + 1;
		} else if (num < strmap[mid].event) {
			hi = mid - 1;
		} else {
			return strmap[mid].str;
		}
	} while (low <= hi);

	printk("evt num %d", num);

	return "Unknown";
}

#define BT_MANAGER_LINK_EVENTNUM_STRS (sizeof(bt_manager_link_event_map) / sizeof(bt_mgr_event_strmap_t))
const char *bt_manager_link_evt2str(int num)
{
	return bt_manager_evt2str(num, BT_MANAGER_LINK_EVENTNUM_STRS, bt_manager_link_event_map);
}

#define BT_MANAGER_STATUS_STRS (sizeof(bt_manager_status_map) / sizeof(bt_mgr_event_strmap_t))
const char *bt_manager_status_evt2str(int status)
{
	return bt_manager_evt2str(status, BT_MANAGER_STATUS_STRS, bt_manager_status_map);
}

#define BT_MANAGER_SERVICE_EVENTNUM_STRS (sizeof(bt_manager_service_event_map) / sizeof(bt_mgr_event_strmap_t))
const char *bt_manager_service_evt2str(int num)
{
	return bt_manager_evt2str(num, BT_MANAGER_SERVICE_EVENTNUM_STRS, bt_manager_service_event_map);
}

struct bt_manager_context_t* bt_manager_get_context(void)
{
    struct bt_manager_context_t*  bt_manager = &bt_manager_context;

    return bt_manager;
}

btmgr_base_config_t * bt_manager_get_base_config(void)
{
    btmgr_base_config_t*  base_config = &bt_manager_base_config;

    return base_config;
}

btmgr_pair_cfg_t * bt_manager_get_pair_config(void)
{
    btmgr_pair_cfg_t*  pair_config = &bt_manager_pair_config;

    return pair_config;
}

btmgr_tws_pair_cfg_t * bt_manager_get_tws_pair_config(void)
{
    btmgr_tws_pair_cfg_t*  tws_pair_config = &bt_manager_tws_pair_config;

    return tws_pair_config;
}

btmgr_reconnect_cfg_t * bt_manager_get_reconnect_config(void)
{
    btmgr_reconnect_cfg_t*  reconnect_config = &bt_manager_reconnect_config;

    return reconnect_config;
}

btmgr_feature_cfg_t * bt_manager_get_feature_config(void)
{
    btmgr_feature_cfg_t*  feature_config = &bt_manager_feature_config;

    return feature_config;
}

btmgr_sync_ctrl_cfg_t * bt_manager_get_sync_ctrl_config(void)
{
    return &bt_manager_sync_ctrl_config;
}

btmgr_ble_cfg_t *bt_manager_ble_config(void)
{
    return &ble_config;
}

btmgr_rf_param_cfg_t *bt_manager_rf_param_config(void)
{
    return &bt_rf_param_config;
}

btmgr_leaudio_cfg_t *bt_manager_leaudio_config(void)
{
    return &leaudio_config;
}



static void bt_manager_set_config_info(void)
{
	struct btsrv_config_info cfg;
	btmgr_base_config_t * cfg_base = bt_manager_get_base_config();
	btmgr_pair_cfg_t *cfg_pair = bt_manager_get_pair_config();
	btmgr_tws_pair_cfg_t *cfg_tws_pair = bt_manager_get_tws_pair_config();
	btmgr_feature_cfg_t *cfg_feature = bt_manager_get_feature_config();
	btmgr_sync_ctrl_cfg_t *sync_ctrl_config = bt_manager_get_sync_ctrl_config();

    uint8_t i;
	
	memset(&cfg, 0, sizeof(cfg));
	cfg.max_conn_num = CONFIG_BT_MAX_BR_CONN;
	cfg.max_phone_num = bt_manager_config_connect_phone_num();
    cfg.cfg_device_num = cfg_feature->sp_device_num;
	cfg.pts_test_mode = bt_manager_config_pts_test() ? 1 : 0;

#ifdef CONFIG_BT_TRANSCEIVER
    cfg.br_tr_mode = bt_manager_config_br_tr() ? 1 : 0;
#endif

#ifdef CONFIG_LE_AUDIO_BQB
    cfg.leaudio_bqb_mode = bt_manager_config_leaudio_bqb() ? 1 : 0;
    cfg.leaudio_bqb_sr_mode = bt_manager_config_leaudio_sr_bqb() ? 1 : 0;
#endif
	cfg.volume_sync_delay_ms = bt_manager_config_volume_sync_delay_ms();
	cfg.tws_version = get_tws_current_versoin();
	cfg.tws_feature = get_tws_current_feature();
	cfg.pair_key_mode = cfg_tws_pair->pair_key_mode;
	cfg.power_on_auto_pair_search = cfg_tws_pair->power_on_auto_pair_search;
	cfg.default_state_discoverable = cfg_pair->default_state_discoverable;
    cfg.default_state_wait_connect_sec = cfg_pair->default_state_wait_connect_sec;
	cfg.bt_not_discoverable_when_connected = cfg_pair->bt_not_discoverable_when_connected;
	cfg.use_search_mode_when_tws_reconnect = cfg_tws_pair->use_search_mode_when_tws_reconnect;
	cfg.device_l_r_match = cfg_tws_pair->sp_device_l_r_match;
	cfg.sniff_enable = cfg_feature->sp_sniff_enable;
	cfg.idle_enter_sniff_time = cfg_feature->enter_sniff_time;
	cfg.sniff_interval = cfg_feature->sniff_interval;
	cfg.stop_another_when_one_playing = sync_ctrl_config->stop_another_when_one_playing;
	cfg.resume_another_when_one_stopped = sync_ctrl_config->resume_another_when_one_stopped;
	cfg.a2dp_status_stopped_delay_ms = sync_ctrl_config->a2dp_status_stopped_delay_ms;
    cfg.use_search_mode_when_tws_reconnect = cfg_tws_pair->use_search_mode_when_tws_reconnect;
    cfg.clear_link_key_when_clear_paired_list = cfg_feature->sp_clear_linkkey;
    cfg.cfg_support_tws = cfg_feature->sp_tws;

#ifdef CONFIG_BT_DOUBLE_PHONE_PREEMPTION_MODE
	cfg.double_preemption_mode = 1;
#else
	cfg.double_preemption_mode = 0;
#endif
	btif_base_set_config_info(&cfg);

    for(i = 0; i < BT_MANAGER_MAX_SCAN_MODE; i++){
	    btif_br_set_scan_param((struct bt_scan_parameter *)&cfg_base->scan_params[i]);
    }
}

static void bt_manager_btsrv_ready(void)
{
	struct bt_manager_context_t *bt_manager = bt_manager_get_context();
#ifdef CONFIG_BT_HID
	btmgr_reconnect_cfg_t *cfg_reconnect = bt_manager_get_reconnect_config();
#endif

#ifdef CONFIG_BT_A2DP
#ifdef CONFIG_BT_EARPHONE_SPEC
	btmgr_base_config_t *cfg_base = bt_manager_get_base_config();
#endif
#endif

	bt_manager_set_config_info();

#ifdef CONFIG_BT_A2DP
#ifdef CONFIG_BT_EARPHONE_SPEC
	bt_manager_a2dp_set_sbc_max_bitpool(cfg_base->ad2p_bitpool);
#endif
	bt_manager_a2dp_profile_start();
#endif
#ifdef CONFIG_BT_AVRCP
	bt_manager_avrcp_profile_start();
#endif
#ifdef CONFIG_BT_HFP_HF
	bt_manager_hfp_sco_start();
	bt_manager_hfp_profile_start();
#endif
#ifdef CONFIG_BT_HFP_AG
#ifdef CONFIG_BT_BR_TRANSCEIVER
	bt_manager_hfp_ag_profile_start();
#endif
#endif

#ifdef CONFIG_BT_SPP
	bt_manager_spp_profile_start();
#endif

#ifdef CONFIG_BT_HID
	bt_manager_hid_register_sdp();
	bt_manager_hid_profile_start(cfg_reconnect->hid_auto_disconnect_delay_sec,
								cfg_reconnect->hid_connect_operation_delay_ms);
	bt_manager_did_register_sdp();
#endif

#ifdef CONFIG_BT_BLE
#ifdef CONFIG_BT_TRANSCEIVER
#ifdef CONFIG_APP_TRANSMITTER
    if(sys_get_work_mode())
    {
        tr_bt_manager_ble_cli_init();
    }
    else
#endif
    {
        bt_manager_ble_init();
    }
#else
	bt_manager_ble_init();
#endif
#endif

#if (defined(CONFIG_BT_BLE_BQB) && defined(CONFIG_BT_TRANSCEIVER))
    if(sys_get_work_mode() == RECEIVER_MODE)
    {
        bt_manager_bqb_init();
    }
#endif

    /* start waiting connect after boot hold key */
    // bt_manager_start_wait_connect();
    
    bt_manager->bt_ready = 1;

    sys_event_send_message_new(MSG_BT_MGR_EVENT, BT_MGR_EVENT_READY, NULL, 0);
}


static void bt_manager_notify_connected(struct bt_mgr_dev_info *info)
{
	if (info->is_tws || info->notify_connected) {
		/* Tws not notify in here, or already notify */
		return;
	}

	SYS_LOG_INF("btsrv connected:%s", (char *)info->name);
	struct bt_audio_report rep;
	rep.handle = info->hdl;
	bt_manager_audio_conn_event(BT_AUDIO_CONNECTED, &rep,sizeof(struct bt_audio_report));		
	
	info->notify_connected = 1;

	if (bt_manager_get_connected_dev_num() == 0) {
		bt_manager_update_phone_volume(info->hdl,1);
	}
	
	bt_manager_set_status(BT_STATUS_CONNECTED);

	if (info->snoop_role != BTSRV_SNOOP_SLAVE) {
		/* Advise not to set, just let phone make dicision. for snoop tws, can't call btif_br_set_phone_controler_role */
		//btif_br_set_phone_controler_role(&info->addr, CONTROLER_ROLE_MASTER);	/* Set phone controler as master */
		//btif_br_set_phone_controler_role(&info->addr, CONTROLER_ROLE_SLAVE);		/* Set phone controler as slave */
#ifdef CONFIG_BT_MAP_CLIENT
		/**only for get datatime*/
		if (btmgr_map_time_client_connect(&info->addr)) {
			SYS_LOG_INF("bt map time connected:%s", (char *)info->name);
		} else {
			SYS_LOG_INF("bt map time connected:%s failed\n", (char *)info->name);
		}
#endif
	}
}

static void bt_manager_check_disconnect_notify(struct bt_mgr_dev_info *info, uint8_t reason)
{
	struct bt_manager_context_t*  bt_manager = bt_manager_get_context();
    uint8_t phone_num = 0;

    if (info->force_disconnect_by_remote) {
        reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN;
    }

	if (info->is_tws) {
		/* Tws not notify in here */
		return;
	}

	if (info->notify_connected) {
		info->notify_connected = 0;
		bt_manager->dis_reason = reason;		/* Transfer to BT_STATUS_DISCONNECTED */
		bt_manager_set_status(BT_STATUS_DISCONNECTED);

		phone_num = bt_manager_get_connected_dev_num();
		if( phone_num == 0){
			bt_manager_set_status(BT_STATUS_WAIT_CONNECT);
		}
	}
	
	SYS_LOG_INF("reason:0x%x phone:%d", reason, bt_manager_get_connected_dev_num());
}

/* return 0: accept connect request, other: rejuect connect request
 * Direct callback from bt stack, can't do too much thing in this function.
 */
static int bt_manager_check_connect_req(struct bt_link_cb_param *param)
{
	if (param->new_dev) {
		SYS_LOG_INF("New connect request");
	} else {
		SYS_LOG_INF("%s connect request", param->is_tws ? "Tws" : "Phone");
	}

	return 0;
}

/* Sample code, just for reference */
#ifdef CONFIG_BT_DOUBLE_PHONE_EXT_MODE
#define SUPPORT_CHECK_DISCONNECT_NONACTIVE_DEV		1
#else
#define SUPPORT_CHECK_DISCONNECT_NONACTIVE_DEV		0
#endif

#if SUPPORT_CHECK_DISCONNECT_NONACTIVE_DEV
static void bt_mgr_check_disconnect_nonactive_dev(struct bt_mgr_dev_info *info)
{
	int i, phone_cnt = 0, tws_cnt = 0;
	struct bt_mgr_dev_info *exp_disconnect_info = NULL;
    struct bt_manager_context_t*  bt_manager = bt_manager_get_context();
	uint16_t hdl = btif_a2dp_get_active_hdl();

	for (i = 0; ((i < MAX_MGR_DEV) && bt_manager->dev[i].used); i++) {
		if (bt_manager->dev[i].is_tws) {
			tws_cnt++;
		} else {
			phone_cnt++;
			if (bt_manager->dev[i].hdl != hdl && bt_manager->dev[i].hdl != info->hdl) {
				exp_disconnect_info = &bt_manager->dev[i];
			}
		}
	}

	/* Tws paired */
	if (tws_cnt) {
		if (phone_cnt >= 2) {
			bt_manager_br_disconnect(&info->addr);
		}
		return;
	}

	if (phone_cnt >= 3) {
		if (exp_disconnect_info) {
			bt_manager_br_disconnect(&exp_disconnect_info->addr);
		}
	}
}
#endif


static bool bt_manager_on_acl_disconnected(struct bt_mgr_dev_info *dev_info, uint8_t reason)
{
    if (dev_info == NULL)
    {
        return false;
    }

    /* 保存播放状态用于超时断开后回连成功时恢复播放
     */
    if (bt_manager_is_timeout_disconnected(reason))
    {
        dev_info->timeout_disconnected = 1;

        if (!dev_info->need_resume_play)
        {
            dev_info->need_resume_play = dev_info->a2dp_status_playing;
        }
    }
    else
    {
        //dev_info->timeout_disconnected = 0;
        //dev_info->need_resume_play     = 0;
    }

    if (dev_info->need_resume_play)
    {
        SYS_LOG_INF("need_resume_play");
    }

    return true;
}



static int bt_manager_link_event(void *param)
{
	int ret = 0;
	struct bt_mgr_dev_info *info;
	struct bt_link_cb_param *in_param = param;
	struct bt_manager_context_t*  bt_manager = bt_manager_get_context();

	info = bt_mgr_find_dev_info_by_hdl(in_param->hdl);
	if ((info == NULL) && (in_param->link_event != BT_LINK_EV_ACL_CONNECTED) &&
		(in_param->link_event != BT_LINK_EV_ACL_CONNECT_REQ)) {
		SYS_LOG_WRN("Already free %d", in_param->link_event);
		return ret;
	}

	SYS_LOG_INF("event: %s hdl: 0x%x", bt_manager_link_evt2str(in_param->link_event), in_param->hdl);
#ifdef CONFIG_BT_BR_TRANSCEIVER
    tr_bt_mgr_link_event(in_param);
#endif

	switch (in_param->link_event) {
	case BT_LINK_EV_ACL_CONNECT_REQ:
		ret = bt_manager_check_connect_req(in_param);
		break;
	case BT_LINK_EV_ACL_CONNECTED:
		info = bt_mgr_find_dev_info_by_hdl(in_param->hdl);
		if (!info){
			bt_manager->connected_acl_num++;
		}
		bt_mgr_add_dev_info(in_param->addr, in_param->hdl);
		bt_manager_audio_conn_event(BT_CONNECTED, &in_param->hdl,sizeof(in_param->hdl));		
		break;
	case BT_LINK_EV_ACL_DISCONNECTED:{
		info = bt_mgr_find_dev_info_by_hdl(in_param->hdl);
		if (info){
			bt_manager->connected_acl_num--;
		}		
		bt_manager_on_acl_disconnected(info, in_param->param);
		bt_manager_check_disconnect_notify(info, in_param->param);
		struct bt_disconn_report rep;
		rep.handle = in_param->hdl;
		rep.reason = in_param->param;	
		bt_manager_audio_conn_event(BT_DISCONNECTED,&rep, sizeof(rep));	
#ifdef CONFIG_BT_BR_TRANSCEIVER
        bt_manager_event_notify(BT_DISCONNECTED, NULL, 0);
#endif
		bt_mgr_free_dev_info(info);
		btmgr_poff_check_phone_disconnected();
	}
		break;
	case BT_LINK_EV_GET_NAME:
		info->name = in_param->name;
		info->is_tws = in_param->is_tws;
		if (info->is_tws){
			bt_manager_audio_conn_event(BT_TWS_CONNECTION_EVENT, &in_param->hdl,sizeof(in_param->hdl)); 
		}
#if SUPPORT_CHECK_DISCONNECT_NONACTIVE_DEV
		bt_mgr_check_disconnect_nonactive_dev(info);
#endif
		break;
	case BT_LINK_EV_HF_CONNECTED:
		info->hf_connected = 1;
		bt_manager_notify_connected(info);
		break;
	case BT_LINK_EV_HF_DISCONNECTED:
		info->hf_connected = 0;
		break;
	case BT_LINK_EV_A2DP_CONNECTED:
		info->a2dp_connected = 1;
		bt_manager_notify_connected(info);
		bt_manager_auto_reconnect_resume_play(in_param->hdl);
		break;
	case BT_LINK_EV_A2DP_DISCONNECTED:
		info->a2dp_connected = 0;
		break;
	case BT_LINK_EV_AVRCP_CONNECTED:
		info->avrcp_connected = 1;
		break;
	case BT_LINK_EV_AVRCP_DISCONNECTED:
		info->avrcp_connected = 0;
		break;
	case BT_LINK_EV_SPP_CONNECTED:
		info->spp_connected++;
		break;
	case BT_LINK_EV_SPP_DISCONNECTED:
		if (info->spp_connected) {
			info->spp_connected--;
		}
		break;
	case BT_LINK_EV_HID_CONNECTED:
		info->hid_connected = 1;
#ifdef CONFIG_BT_HID
		bt_manager_hid_connected_check_work(info->hdl);
#endif
		break;
	case BT_LINK_EV_HID_DISCONNECTED:
		info->hid_connected = 0;
		break;
	case BT_LINK_EV_SNOOP_ROLE:
		info->snoop_role = in_param->param&0x7;
		if (info->snoop_role == BTSRV_SNOOP_MASTER){
			bt_manager_tws_sync_phone_info(info->hdl, NULL);
		}
		btmgr_tws_poff_check_snoop_disconnect();
		break;
	default:
		break;
	}

	return ret;
}

/* Return 0: phone device; other : tws device
 * Direct callback from bt stack, can't do too much thing in this function.
 */
static int bt_manager_check_new_device_role(void *param)
{
#ifdef CONFIG_BT_EARPHONE_SPEC
    struct btsrv_check_device_role_s *cb_param = param;
    uint8_t pre_mac[3];
    uint8_t name[31];
    btmgr_base_config_t * cfg_base =  bt_manager_get_base_config();
    btmgr_tws_pair_cfg_t * cfg_tws =  bt_manager_get_tws_pair_config();
    uint8_t match_len = cfg_tws->match_name_length;
    bd_address_t bd_addr;

	if (bt_manager_config_get_tws_compare_high_mac()) {
		btif_br_get_local_mac(&bd_addr);
		memcpy(pre_mac,&bd_addr.val[3] ,3);
		if (cb_param->addr.val[5] != pre_mac[2] ||
			cb_param->addr.val[4] != pre_mac[1] ||
			cb_param->addr.val[3] != pre_mac[0]){
			return 0;
		}
	}

    memset(name, 0, sizeof(name));
    memcpy(name,cfg_base->bt_device_name,30);

    if(match_len > cb_param->len){
        match_len = cb_param->len;
    }
    if (memcmp(cb_param->name, name, match_len)) {
        return 0;
    }
    else{
        return 1;
    }
    return 0;
#else
	struct btsrv_check_device_role_s *cb_param = param;
	uint8_t pre_mac[3];

	if (bt_manager_config_get_tws_compare_high_mac()) {
		bt_manager_config_set_pre_bt_mac(pre_mac);
		if (cb_param->addr.val[5] != pre_mac[0] ||
			cb_param->addr.val[4] != pre_mac[1] ||
			cb_param->addr.val[3] != pre_mac[2]) {
			return 0;
		}
	}

#ifdef CONFIG_PROPERTY
	uint8_t name[33];
	memset(name, 0, sizeof(name));
	property_get(CFG_BT_NAME, name, 32);
	if (strlen(name) != cb_param->len || memcmp(cb_param->name, name, cb_param->len)) {
		return 0;
	}
#endif

	return 1;
#endif
}

static void bt_manager_auto_reconnect_complete(void)
{
    struct bt_manager_context_t*  bt_manager = bt_manager_get_context();
	btmgr_reconnect_cfg_t *cfg_reconnect = bt_manager_get_reconnect_config();

	if(bt_manager->auto_reconnect_startup){
		bt_manager->auto_reconnect_startup = 0;
        if (cfg_reconnect->enter_pair_mode_when_startup_reconnect_fail == 0){
            return;
        }

        if (btif_tws_get_dev_role() == BTSRV_TWS_SLAVE) {
            return;	
        }
		
        if (bt_manager_get_connected_dev_num() > 0){
            return;
        }
        bt_manager_enter_pair_mode();
	}
}

static int bt_manager_service_callback(btsrv_event_e event, void *param)
{
	int ret = 0;

    if(event == BTSRV_REQ_FLUSH_PROPERTY){
	    SYS_LOG_INF("event: %s %s",bt_manager_service_evt2str(event),(char *)param);
	}
	else{
	    SYS_LOG_INF("event: %s",bt_manager_service_evt2str(event));
	}

	switch (event) {
	case BTSRV_READY:
		bt_manager_btsrv_ready();
		break;
	case BTSRV_LINK_EVENT:
		ret = bt_manager_link_event(param);
		break;
	case BTSRV_DISCONNECTED_REASON:
		bt_manager_disconnected_reason(param);
		break;
	case BTSRV_REQ_HIGH_PERFORMANCE:
#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
		dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "btmanager");
#endif
	break;
	case BTSRV_RELEASE_HIGH_PERFORMANCE:
#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
		dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "btmanager");
#endif
		break;
	case BTSRV_REQ_FLUSH_PROPERTY:
#ifdef CONFIG_PROPERTY
		property_flush_req(param);
#endif
		break;
	case BTSRV_CHECK_NEW_DEVICE_ROLE:
		ret = bt_manager_check_new_device_role(param);
		break;
    case BTSRV_AUTO_RECONNECT_COMPLETE:
		bt_manager_auto_reconnect_complete();
		break;
	case BTSRV_SYNC_AOTO_RECONNECT_STATUS:
		bt_manager_sync_auto_reconnect_status(param);
		break;
	default:
		break;
	}

	return ret;
}

#ifndef CONFIG_BT_EARPHONE_SPEC
void bt_manager_record_halt_phone(void)
{
	uint8_t i, record = 0;
    struct bt_manager_context_t*  bt_manager = bt_manager_get_context();

	memset(bt_manager->halt_addr, 0, sizeof(bt_manager->halt_addr));
	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_manager->dev[i].used && !bt_manager->dev[i].is_tws &&
			(bt_manager->dev[i].a2dp_connected || bt_manager->dev[i].hf_connected)) {
			memcpy(&bt_manager->halt_addr[record], &bt_manager->dev[i].addr, sizeof(bd_address_t));
			record++;
		}
	}
}

void *bt_manager_get_halt_phone(uint8_t *halt_cnt)
{
    struct bt_manager_context_t*  bt_manager = bt_manager_get_context();

	*halt_cnt = MAX_MGR_DEV;
	return bt_manager->halt_addr;
}
#endif

int bt_manager_get_status(void)
{
    struct bt_manager_context_t*  bt_manager = bt_manager_get_context();

	return bt_manager->bt_state;
}

int bt_manager_get_connected_dev_num(void)
{
    struct bt_manager_context_t*  bt_manager = bt_manager_get_context();

	return bt_manager->connected_phone_num;
}

#ifdef CONFIG_TR_BT_HOST_BQB
bool bt_mgr_check_dev_aclconnected_a2dp_disconnect(void)
{
	int i;
    struct bt_manager_context_t*  bt_manager = bt_manager_get_context();
	for (i = 0; i < MAX_MGR_DEV; i++)
    {
		if (bt_manager->dev[i].used && !bt_manager->dev[i].a2dp_connected)
        {
			return true;
		}
	}
	return false;
}
#endif


int bt_manager_set_status(int state)
{
    struct bt_manager_context_t*  bt_manager = bt_manager_get_context();

	SYS_LOG_INF("status:%s", bt_manager_status_evt2str(state));

	switch (state) {
	case BT_STATUS_CONNECTED:
	{
		bt_manager->connected_phone_num++;
        bt_manager_check_phone_connected();

		/*if lea connected, then BR is 2nd device*/
		if (!bt_manager_audio_lea_addr() && bt_manager->connected_phone_num == 1) {
			bt_manager_sys_event_notify(SYS_EVENT_BT_CONNECTED);
		} else {
			bt_manager_sys_event_notify(SYS_EVENT_2ND_CONNECTED);
		}
#ifdef CONFIG_BT_BR_TRANSCEIVER
        bt_manager->connecting = 0;
        bt_manager->connecting_disconn_times =0;
#endif

		break;
	}
	case BT_STATUS_DISCONNECTED:
	{
		if (bt_manager->connected_phone_num > 0) {
			bt_manager->connected_phone_num--;
			if (bt_manager->dis_reason != 0x16) {
				bt_manager_sys_event_notify(SYS_EVENT_BT_DISCONNECTED);
			}

			/*if lea no connected, then send SYS_EVENT_BT_UNLINKED */
			if (!bt_manager_audio_lea_addr() && !bt_manager->connected_phone_num) {
				bt_manager_sys_event_notify(SYS_EVENT_BT_UNLINKED);
			}
		} 
#ifdef CONFIG_BT_BR_TRANSCEIVER
        if(bt_manager->connecting == 1)
        {
            bt_manager->connecting_disconn_times++;
            if(bt_manager->connecting_disconn_times == 1)
            {
                bt_manager->connecting = 0;
                bt_manager->bt_state = BT_STATUS_DISCONNECTED;
            }
        }
        else
        {
            bt_manager->bt_state = BT_STATUS_DISCONNECTED;
        }
#endif

        /* restart waiting connect when BT disconnected */
        if (bt_manager_tws_get_dev_role() != BTSRV_TWS_SLAVE &&
            bt_manager->poweroff_state == POFF_STATE_NONE)
        {
            bt_manager_start_wait_connect();
        }
		break;
	}
	case BT_STATUS_TWS_PAIRED:
	{
		if (!bt_manager->tws_mode) {
			bt_manager->tws_mode = 1;
#ifndef CONFIG_BT_EARPHONE_SPEC			
			if (btif_tws_get_dev_role() == BTSRV_TWS_MASTER) {
				bt_manager_sys_event_notify(SYS_EVENT_TWS_CONNECTED);
			}
#else
			bt_manager_sys_event_notify(SYS_EVENT_TWS_CONNECTED);
#endif			
			
			bt_manager_event_notify(BT_TWS_CONNECTION_EVENT, NULL, 0);
		}
		break;
	}
	case BT_STATUS_TWS_UNPAIRED:
	{
		if (bt_manager->tws_mode) {
			bt_manager->tws_mode = 0;
#ifdef CONFIG_BT_EARPHONE_SPEC	
			if (bt_manager_is_timeout_disconnected(bt_manager->tws_dis_reason)){
            	bt_manager_sys_event_notify(SYS_EVENT_TWS_DISCONNECTED);
			}
#endif			
			bt_manager_event_notify(BT_TWS_DISCONNECTION_EVENT, NULL, 0);

            /* restart waiting connect when TWS disconnected */
            if (bt_manager->poweroff_state == POFF_STATE_NONE)
            {
			    bt_manager_start_wait_connect();
			}
		}
		break;
	}

	case BT_STATUS_TWS_PAIR_SEARCH:
	{
		bt_manager_sys_event_notify(SYS_EVENT_TWS_START_PAIR);
		break;
	}
	case BT_STATUS_WAIT_CONNECT:
	{
		bt_manager_sys_event_notify(SYS_EVENT_BT_WAIT_PAIR);
		break;
	}

#ifdef CONFIG_BT_BR_TRANSCEIVER
    case BT_STATUS_INQUIRY:
    {
        bt_manager->inquiry = 1;
        break;
    }

    case BT_STATUS_INQUIRY_COMPLETE:
    {
        bt_manager->inquiry = 0;
        if((bt_manager->bt_state >= BT_STATUS_PAUSED) && (bt_manager->bt_state <= BT_STATUS_3WAYIN))
        {
            state = BT_STATUS_CONNECTED_INQUIRY_COMPLETE;
        }
        break;
    }

    case BT_STATUS_CONNECTING:
    {
        bt_manager->connecting = 1;
        bt_manager->connecting_disconn_times =0;
        break;
    }
#endif

	default:
		break;
	}

	bt_manager->bt_state = state;
    bt_manager_check_link_status();

	return 0;
}

void bt_manager_set_stream_type(uint8_t stream_type)
{
#ifdef CONFIG_TWS
	bt_manager_tws_set_stream_type(stream_type);
#endif
}

void bt_manager_set_codec(uint8_t codec)
{
#ifdef CONFIG_TWS
	bt_manager_tws_set_codec(codec);
#endif
}

static void wait_connect_work_callback(struct k_work *work)
{
    bt_manager_end_wait_connect();
}

static void pair_mode_work_callback(struct k_work *work)
{
    struct bt_manager_context_t *bt_manager = bt_manager_get_context();

	bt_manager->pair_mode_work_valid = false;

    if((bt_manager->pair_mode_state != BT_PAIR_MODE_STATE_NONE) &&
		(bt_manager->pair_mode_state != BT_PAIR_MODE_RUNNING)){
	    bt_manager_enter_pair_mode_ex();
    }
    else{
        bt_manager_end_pair_mode();
    }
}

static void tws_pair_search_work_callback(struct k_work *work)
{
    bt_manager_tws_end_pair_search();
}

static void clear_paired_list_work(struct k_work *work)
{
    struct bt_manager_context_t *bt_manager = bt_manager_get_context();
	bt_manager->clear_paired_list_work_valid = false;

    if(bt_manager->clear_paired_list_state != BT_CLEAR_PAIRED_LIST_NONE){
	    bt_manager_clear_paired_list_ex();
    }
}

static void bt_manager_set_bt_drv_param(void)
{
#ifdef CONFIG_BT_EARPHONE_SPEC
	btdrv_init_param_t param;
	btmgr_base_config_t *cfg = bt_manager_get_base_config();
	btmgr_rf_param_cfg_t *rf_prarm_config = bt_manager_rf_param_config();

	param.set_hosc_cap = cfg->force_default_hosc_capacity;
	param.hosc_capacity = cfg->default_hosc_capacity;

	param.set_max_rf_power = 1;
	param.bt_max_rf_tx_power = cfg->bt_max_rf_tx_power;
	param.set_ble_rf_power = 1;
	param.ble_rf_tx_power = cfg->ble_rf_tx_power;
	memcpy(&param.rf_param, rf_prarm_config, sizeof(param.rf_param));

	SYS_LOG_INF("set_hosc_cap:%d, hosc_capacity:0x%x,bt_max_rf_tx_power:%d",
	param.set_hosc_cap, param.hosc_capacity, param.bt_max_rf_tx_power);

	btdrv_set_init_param(&param);
#endif
}

static void bt_manager_check_enter_bqb(void)
{
#ifdef CONFIG_BT_CTRL_BQB
	btmgr_feature_cfg_t *cfg_feature = bt_manager_get_feature_config();

	extern int bqb_init(int bqb_mode /*0:BR BQB Test, 1:BLE BQB Test,2:BR/BLE dual BQB Test*/);

	if (cfg_feature->con_real_test_mode != DISABLE_TEST){
		sys_wake_lock(FULL_WAKE_LOCK);
	}

	if (cfg_feature->con_real_test_mode == DUT_TEST) {
		bqb_init(0);
		SYS_LOG_INF("Enter BQB mode %d", cfg_feature->controller_test_mode);
	} else if (cfg_feature->con_real_test_mode == LE_TEST) {
		bqb_init(1);
		SYS_LOG_INF("Enter BQB mode %d", cfg_feature->controller_test_mode);
	}else if (cfg_feature->con_real_test_mode == DUT_LE_TEST){
		bqb_init(2);
		SYS_LOG_INF("Enter BQB mode %d", cfg_feature->controller_test_mode);
	}
#endif
}

#define BT_TEMP_COMP_CHECK_TIMER_MS 1000

static void bt_temp_comp_timer_handler(struct k_work* work)
{
	struct bt_manager_context_t* bt_manager = bt_manager_get_context();
	
    const struct device* adc_dev = device_get_binding(CONFIG_PMUADC_NAME);

    if (adc_dev == NULL)
        return;

    if (bt_manager->bt_temp_comp_stage == 0)
    {
        struct adc_channel_cfg adc_cfg = { 0, };
        
        adc_cfg.channel_id = PMUADC_ID_SENSOR;
        adc_channel_setup(adc_dev, &adc_cfg);
        
        bt_manager->bt_temp_comp_stage = 1;
        os_delayed_work_submit(&bt_manager->bt_temp_comp_timer, 1);
    }
    else
    {
        struct adc_sequence adc_data = { 0, };
        uint16_t adc_buf[2];
        int temp;
        
        adc_data.channels    = BIT(PMUADC_ID_SENSOR);
        adc_data.buffer      = adc_buf;
        adc_data.buffer_size = sizeof(adc_buf);
        
        adc_read(adc_dev, &adc_data);
        temp = (3869 - (6201 * adc_buf[0]) / 1000);

        if (bt_manager->bt_temp_comp_stage == 1)
        {
            SYS_LOG_INF("0x%x, %d.%d", adc_buf[0], temp / 10, temp % 10);
            
            bt_manager->bt_temp_comp_stage = 2;
            bt_manager->bt_comp_last_temp  = temp;
        }
        else if (abs(temp - bt_manager->bt_comp_last_temp) >= 150)
        {
            SYS_LOG_WRN("0x%x, %d.%d, DO_TEMP_COMP", adc_buf[0], temp / 10, temp % 10);
            bt_manager->bt_comp_last_temp = temp;

            bt_manager_bt_set_apll_temp_comp(true);
            bt_manager_bt_do_apll_temp_comp();
        }
        
        os_delayed_work_submit(&bt_manager->bt_temp_comp_timer, BT_TEMP_COMP_CHECK_TIMER_MS);
    }
}

int bt_manager_init(void)
{
	int ret = 0;
	int status = BT_MANAGER_INIT_OK;
	struct bt_manager_context_t *bt_manager = bt_manager_get_context();
	btmgr_feature_cfg_t *cfg_feature = bt_manager_get_feature_config();
	btmgr_ble_cfg_t *cfg_ble = bt_manager_ble_config();
	struct btsrv_config_stack_param stack_param;

    if(bt_manager->inited){
        return 0;
    }

	memset(bt_manager, 0, sizeof(struct bt_manager_context_t));
	bt_manager_init_dev_volume();
	bt_manager_set_bt_drv_param();
	bt_manager_check_mac_name();

	os_delayed_work_init(&bt_manager->wait_connect_work, wait_connect_work_callback);
	os_delayed_work_init(&bt_manager->pair_mode_work, pair_mode_work_callback);
	os_delayed_work_init(&bt_manager->tws_pair_search_work, tws_pair_search_work_callback);
	os_delayed_work_init(&bt_manager->clear_paired_list_work, clear_paired_list_work);

	os_mutex_init(&bt_manager->poweroff_mutex);
	os_delayed_work_init(&bt_manager->poweroff_proc_work, btmgr_poff_state_proc_work);

	bt_manager->pair_status = BT_PAIR_STATUS_NONE;
	bt_manager_set_status(BT_STATUS_LINK_NONE);

	if (cfg_feature->con_real_test_mode != DISABLE_TEST)
	{
		bt_manager_check_enter_bqb();
		bt_manager->cur_bqb_mode = 1;
		return 0;
	}

	btif_base_register_processer();
#ifdef CONFIG_BT_HFP_HF
	btif_hfp_register_processer();
#endif
#ifdef CONFIG_BT_HFP_AG
#ifdef CONFIG_BT_BR_TRANSCEIVER
	btif_hfp_ag_register_processer();
#endif
#endif
#ifdef CONFIG_BT_A2DP
	btif_a2dp_register_processer();
#endif
#ifdef CONFIG_BT_AVRCP
	btif_avrcp_register_processer();
#endif
#ifdef CONFIG_BT_SPP
	btif_spp_register_processer();
#endif
#ifdef CONFIG_BT_PBAP_CLIENT
	btif_pbap_register_processer();
#endif
#ifdef CONFIG_BT_MAP_CLIENT
	btif_map_register_processer();
#endif
#if CONFIG_BT_HID
	btif_hid_register_processer();
#endif

#ifdef CONFIG_BT_BR_TRANSCEIVER
#else
#ifdef CONFIG_TWS
	btif_tws_register_processer();
#endif
#endif

#ifdef CONFIG_BT_LE_AUDIO
	btif_audio_register_processer();
#endif
#ifdef CONFIG_BT_BR_TRANSCEIVER
    tr_btif_trs_register_processer();
#endif

	stack_param.sniff_enable = cfg_feature->sp_sniff_enable;

	if(bt_manager_config_pts_test()){
		stack_param.linkkey_miss_reject = 0;
	}else{
		stack_param.linkkey_miss_reject = cfg_feature->sp_linkkey_miss_reject;
	}

#ifdef CONFIG_BT_BR_TRANSCEIVER
    stack_param.bt_br_tr_enable = 1;
#else
    stack_param.bt_br_tr_enable = 0;
#endif

#ifdef CONFIG_LE_AUDIO_BQB
    stack_param.leaudio_bqb_enable = 1;
    stack_param.leaudio_bqb_sr_enable = 0;
#else
    //stack_param.leaudio_bqb_enable = 0;
#endif

#ifdef CONFIG_LE_AUDIO_SR_BQB
    stack_param.leaudio_bqb_sr_enable = 1;
#else
    //stack_param.leaudio_bqb_sr_enable = 0;
#endif

	stack_param.max_pair_list_num = cfg_feature->max_pair_list_num;
	stack_param.le_adv_random_addr = (cfg_ble->ble_address_type == 0) ? 0 : 1;
	stack_param.avrcp_vol_sync = cfg_feature->sp_avrcp_vol_syn;
	stack_param.hfp_sp_codec_neg = cfg_feature->sp_hfp_codec_negotiation;
	stack_param.hfp_sp_voice_recg = cfg_feature->sp_hfp_siri;
	stack_param.hfp_sp_3way_call = cfg_feature->sp_hfp_3way_call;
	stack_param.hfp_sp_volume = cfg_feature->sp_hfp_vol_syn;
	stack_param.hfp_sp_ecnr = cfg_feature->sp_hfp_enable_nrec;
	stack_param.bt_version = 0x0C;				/* Bt verson 5.3 */
	stack_param.bt_sub_version = 0x0001;
	stack_param.classOfDevice = bt_manager_config_bt_class();
	memcpy(stack_param.bt_device_id, bt_manager_config_get_device_id(), sizeof(stack_param.bt_device_id));
	if (btif_start(bt_manager_service_callback, &stack_param) < 0){
		status = BT_MANAGER_INIT_SERVICE_FAIL;
		ret = -EACCES;
	} else {
#ifdef CONFIG_BT_BR_TRANSCEIVER
        tr_bt_manager_trs_init();
#else
#ifdef CONFIG_TWS
        bt_manager_tws_init();
#endif
#endif
#ifdef CONFIG_BT_HFP_HF
        bt_manager_hfp_init();
        bt_manager_hfp_sco_init();
#endif

#ifdef CONFIG_BT_HFP_AG
#ifdef CONFIG_BT_BR_TRANSCEIVER
        bt_manager_hfp_ag_init();
#endif
#endif
	}

#ifdef CONFIG_BT_ADV_MANAGER
	btmgr_adv_init();
#endif
	os_delayed_work_init(&bt_manager->bt_temp_comp_timer, bt_temp_comp_timer_handler);
	os_delayed_work_submit(&bt_manager->bt_temp_comp_timer, BT_TEMP_COMP_CHECK_TIMER_MS);

#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    bt_ui_monitor_init();
#endif

    SYS_LOG_INF("status:%d",status);
    bt_manager->inited = 1;	
	return ret;
}

void bt_manager_deinit(void)
{
	int time_out = 0;
    struct bt_manager_context_t*  bt_manager = bt_manager_get_context();
    struct bt_set_user_visual user_visual;

	if (bt_manager->cur_bqb_mode == 1 ){
		return;
	}

	if (!bt_manager->inited){
		return;
	}


	bt_manager_audio_stop(BT_TYPE_LE);
	bt_manager_audio_exit(BT_TYPE_LE);
	os_delayed_work_cancel(&bt_manager->wait_connect_work);
	os_delayed_work_cancel(&bt_manager->pair_mode_work);
	os_delayed_work_cancel(&bt_manager->tws_pair_search_work);
	os_delayed_work_cancel(&bt_manager->clear_paired_list_work);
	os_delayed_work_cancel(&bt_manager->poweroff_proc_work);
	os_delayed_work_cancel(&bt_manager->bt_temp_comp_timer);

    bt_manager->pair_status = BT_PAIR_STATUS_NODISC_NOCON;
    btif_br_update_pair_status(bt_manager->pair_status);
	user_visual.enable = 0;
	btif_br_set_user_visual(&user_visual);

	btif_br_auto_reconnect_stop(BTSRV_STOP_AUTO_RECONNECT_ALL);
	btif_br_disconnect_device(BTSRV_DISCONNECT_ALL_MODE);

	while (btif_br_get_connected_device_num() && time_out++ < 500) {
		os_sleep(10);
	}

#ifdef CONFIG_BT_SPP
	bt_manager_spp_profile_stop();
#endif

	bt_manager_a2dp_profile_stop();
	bt_manager_avrcp_profile_stop();
	bt_manager_hfp_profile_stop();
	bt_manager_hfp_sco_stop();
#ifdef CONFIG_BT_HFP_AG
#ifdef CONFIG_BT_BR_TRANSCEIVER
	bt_manager_hfp_ag_profile_stop();
#endif
#endif

#ifdef CONFIG_BT_HID
	bt_manager_hid_profile_stop();
#endif

#ifdef CONFIG_TWS
	bt_manager_tws_deinit();
#endif

#ifdef CONFIG_BT_ADV_MANAGER
	btmgr_adv_deinit();
#endif

#ifdef CONFIG_BT_BLE
	bt_manager_ble_deinit();
#endif

	bt_manager_save_dev_volume();
	bt_manager->bt_ready = 0;
#if 1
	/**
	 *  TODO: must clean btdrv /bt stack and bt service when bt manager deinit
	 *  enable this after all is work well.
	 */
	btif_stop();

	time_out = 0;
	while (srv_manager_check_service_is_actived(BLUETOOTH_SERVICE_NAME) && time_out++ < 500) {
        os_sleep(10);
	}

#endif
	SYS_LOG_INF("done");
	bt_manager->inited = 0; 

}

bool bt_manager_is_ready(void)
{
    struct bt_manager_context_t*  bt_manager = bt_manager_get_context();

	return (bt_manager->bt_ready == 1);
}

/* Return bt clock in ms */
uint32_t bt_manager_tws_get_bt_clock(void)
{
    return btif_tws_get_bt_clock();
}

int bt_manager_tws_set_irq_time(uint32_t bt_clock_ms)
{
    return btif_tws_set_irq_time(bt_clock_ms);
}

void bt_manager_wait_tws_ready_send_sync_msg(uint32_t wait_timeout)
{
	int ret;
	uint32_t start_time, stop_time;

	start_time = os_uptime_get_32();
	ret = btif_tws_wait_ready_send_sync_msg(wait_timeout);
	stop_time = os_uptime_get_32();

	if (ret || ((stop_time - start_time) > 10)) {
		SYS_LOG_WRN("Tws wait sync send thread %p(%d) ret %d time %d", os_current_get(),
					os_thread_priority_get(os_current_get()), ret, (stop_time - start_time));
	}
}

/* single_poweroff: 0: tws sync poweroff, 1: single device poweroff */
int bt_manager_proc_poweroff(uint8_t single_poweroff)
{
	struct bt_manager_context_t *bt_manager = bt_manager_get_context();
	int time_out = 0;
	struct app_msg	msg = {0};

	os_mutex_lock(&bt_manager->poweroff_mutex, OS_FOREVER);

	if (bt_manager->poweroff_state != POFF_STATE_NONE) {
		SYS_LOG_INF("Poff in process");
		bt_manager->local_later_poweroff = 1;
		os_mutex_unlock(&bt_manager->poweroff_mutex);
		return 0;
	}

	/*FIX btserver unready, but request poweroff, cause dead */
    if (bt_manager->inited && !bt_manager->bt_ready){
		SYS_LOG_INF("unready, Start poff single %d, ", single_poweroff);

		while (time_out++ < 300) {
			if (bt_manager->bt_ready){
				break;
			}
			os_sleep(10);
		}

		if (time_out >= 300){
			msg.type = MSG_BT_MGR_EVENT;
			msg.cmd = BT_MGR_EVENT_POWEROFF_RESULT;
			msg.reserve = BTMGR_LREQ_POFF_RESULT_OK;
			send_async_msg("main", &msg);
			os_mutex_unlock(&bt_manager->poweroff_mutex);			
			return 0;		
		}
	}

	SYS_LOG_INF("Start poff single %d", single_poweroff);

	if (bt_manager->cur_bqb_mode){
		bt_manager->poweroff_state = POFF_STATE_FINISH;
	}else{
		bt_manager->poweroff_state = POFF_STATE_START;
	}
	
	bt_manager->local_req_poweroff = 1;
	bt_manager->remote_req_poweroff = 0;
	bt_manager->single_poweroff = single_poweroff ? 1 : 0;
	bt_manager->local_later_poweroff = 0;
	bt_manager->local_pre_poweroff_done = 0;
	if (bt_manager->local_req_poweroff ||
		(bt_manager->remote_req_poweroff && (bt_manager->single_poweroff == 0)))
	{
		SYS_LOG_INF("leaudio stop");
		btmgr_adv_update_immediately();		
		bt_manager_audio_unblocked_stop(BT_TYPE_LE);
	}
	
	os_delayed_work_submit(&bt_manager->poweroff_proc_work, 0);

	os_mutex_unlock(&bt_manager->poweroff_mutex);
	return 0;
}


int bt_manager_is_poweroffing(void)
{
	struct bt_manager_context_t *bt_manager = bt_manager_get_context();

	return (bt_manager->poweroff_state != POFF_STATE_NONE)?1:0;
}


/* timeout disconnected
 */
bool bt_manager_is_timeout_disconnected(uint32_t reason)
{
    if (reason == 0x08 ||  // HCI_STATUS_CONNECTION_TIMEOUT
        reason == 0x04 ||  // HCI_STATUS_PAGE_TIMEOUT
        reason == 0x22 ||  // HCI_STATUS_LMP_OR_LL_RESPONSE_TIMEOUT
        reason == 0x10)    // HCI_STATUS_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED
    {
        return true;
    }

    return false;
}

int bt_manager_get_wake_lock(void)
{
#ifdef CONFIG_BT_BLE
	if (btif_base_get_wake_lock() || bt_manager_get_ble_wake_lock())
#else
	if (btif_base_get_wake_lock())
#endif
	{
		return 1;
	} else {
		return 0;
	}
}

int bt_manager_get_link_idle(void)
{
	if (btif_base_get_link_idle()){
		return 1;
	} else {
		return 0;
	}
}



int bt_manager_bt_set_apll_temp_comp(uint8_t enable)
{
	if (!bt_manager_is_ready()) {
		return -EIO;
	}

	return btif_bt_set_apll_temp_comp(enable);
}

int bt_manager_bt_do_apll_temp_comp(void)
{
	if (!bt_manager_is_ready()) {
		return -EIO;
	}

	return btif_bt_do_apll_temp_comp();
}

void bt_manager_dump_info(void)
{
	int i;
    struct bt_manager_context_t *bt_manager = bt_manager_get_context();

	printk("Bt manager info\n");
	printk("num %d, tws_mode %d, bt_state 0x%x, playing %d\n", bt_manager->connected_phone_num,
		bt_manager->tws_mode, bt_manager->bt_state, (bt_manager_a2dp_get_status() == BT_STATUS_PLAYING));
	for (i = 0; i < MAX_MGR_DEV; i++) {
		if (bt_manager->dev[i].used) {
			printk("Dev hdl 0x%x name %s, tws %d snoop role %d notify_connected %d\n",
				bt_manager->dev[i].hdl, bt_manager->dev[i].name, bt_manager->dev[i].is_tws,
				bt_manager->dev[i].snoop_role, bt_manager->dev[i].notify_connected);
			printk("\t a2dp %d avrcp %d hf %d spp %d hid %d\n", bt_manager->dev[i].a2dp_connected,
				bt_manager->dev[i].avrcp_connected, bt_manager->dev[i].hf_connected,
				bt_manager->dev[i].spp_connected, bt_manager->dev[i].hid_connected);
		}
	}

	printk("\n");
	btif_dump_brsrv_info();
}


int bt_manager_send_mailbox_sync(uint8_t msg_id, uint8_t *data, uint16_t len)
{
	return btdrv_send_mailbox_sync(msg_id, data, len);
}


int bt_manager_set_mailbox_ctx(void)
{
	uint32_t mailbox_ctx_params[2];
	mailbox_ctx_params[0] = 1;
	mailbox_ctx_params[1] = (uint32_t)btdrv_get_mailbox_ctx();
	SYS_LOG_INF("btc mailbox : 0x%x", mailbox_ctx_params[1]);
	btif_set_mailbox_ctx_to_controler(mailbox_ctx_params, ARRAY_SIZE(mailbox_ctx_params));
	return 0;
}

