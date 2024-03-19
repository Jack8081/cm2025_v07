/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager config.
*/
#define SYS_LOG_DOMAIN "bt manager"
#define SYS_LOG_LEVEL 4		/* 4, debug */
#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <bt_manager.h>
#include "bt_manager_inner.h"
#include <sys_event.h>
#include <btservice_api.h>

//static uint8_t default_pre_mac[3] = {0xF4, 0x4E, 0xFD};
static uint8_t default_pre_mac[3] = {0x50, 0xC0, 0xF0};


uint8_t bt_manager_config_get_tws_limit_inquiry(void)
{
	return 0;
}

uint8_t bt_manager_config_get_tws_compare_high_mac(void)
{
	return 1;
}

uint8_t bt_manager_config_get_tws_compare_device_id(void)
{
	return 0;
}

void bt_manager_config_set_pre_bt_mac(uint8_t *mac)
{
	memcpy(mac, default_pre_mac, sizeof(default_pre_mac));
}

void bt_manager_updata_pre_bt_mac(uint8_t *mac)
{
	memcpy(default_pre_mac, mac, sizeof(default_pre_mac));
}

bool bt_manager_config_enable_tws_sync_event(void)
{
	return false;
}

uint8_t bt_manager_config_connect_phone_num(void)
{
#ifdef CONFIG_BT_EARPHONE_SPEC
	btmgr_feature_cfg_t * cfg =  bt_manager_get_feature_config();
	if (cfg->sp_dual_phone){
		return 2;
	}
	return 1;
#else
#ifdef CONFIG_BT_DOUBLE_PHONE
#ifdef CONFIG_BT_DOUBLE_PHONE_EXT_MODE
	return 3;
#else
	return 2;
#endif
#else
	return 1;
#endif

#endif
}

bool bt_manager_config_support_a2dp_aac(void)
{
#ifdef CONFIG_BT_EARPHONE_SPEC
	btmgr_feature_cfg_t * cfg =  bt_manager_get_feature_config();
	if (cfg->sp_avdtp_aac){
		return true;
	}
	return false;
#else
#ifdef CONFIG_BT_A2DP_AAC
	return true;
#else
	return false;
#endif
#endif
}

int bt_manager_config_support_cp_scms_t(void)
{
#ifdef CONFIG_BT_EARPHONE_SPEC
	btmgr_feature_cfg_t * cfg =  bt_manager_get_feature_config();
	if (cfg->sp_a2dp_cp == 2){
		return 1;
	}
	return 0;
#else
	return 1;
#endif
}


bool bt_manager_config_support_battery_report(void)
{
#ifdef CONFIG_BT_EARPHONE_SPEC
	btmgr_feature_cfg_t * cfg =  bt_manager_get_feature_config();
	if (cfg->sp_hfp_bat_report){
		return true;
	}
	return false;
#else
	return true;
#endif
}




bool bt_manager_config_pts_test(void)
{
#ifdef  CONFIG_BT_PTS_TEST
	return true;
#else
	return false;
#endif
}

#ifdef CONFIG_BT_TRANSCEIVER
bool bt_manager_config_br_tr(void)
{
#ifdef  CONFIG_BT_BR_TRANSCEIVER
	return true;
#else
	return false;
#endif
}

bool bt_manager_config_leaudio_bqb(void)
{
#ifdef  CONFIG_LE_AUDIO_BQB
	return true;
#else
	return false;
#endif
}

bool bt_manager_config_leaudio_sr_bqb(void)
{
#ifdef  CONFIG_LE_AUDIO_SR_BQB
	return true;
#else
	return false;
#endif
}

#endif

uint16_t bt_manager_config_volume_sync_delay_ms(void)
{
#ifdef CONFIG_BT_EARPHONE_SPEC
	btmgr_sync_ctrl_cfg_t* cfg =  bt_manager_get_sync_ctrl_config();
	if (cfg->a2dp_volume_sync_when_playing){
		return cfg->a2dp_Playing_volume_sync_delay_ms;
	}else if (cfg->a2dp_origin_volume_sync_to_remote){
		return cfg->a2dp_origin_volume_sync_delay_ms;
	}else{
		return 3000;
	}
#else
	return 3000;
#endif
}

uint32_t bt_manager_config_bt_class(void)
{
#ifdef CONFIG_BT_EARPHONE_SPEC
	btmgr_base_config_t* cfg =  bt_manager_get_base_config();
	return cfg->bt_device_class;
#else
	return 0x240404;		/* Rendering,Audio, Audio/Video, Wearable Headset Device */
#endif
}

uint16_t *bt_manager_config_get_device_id(void)
{
#ifdef CONFIG_BT_EARPHONE_SPEC
	btmgr_base_config_t* cfg =  bt_manager_get_base_config();
	static uint16_t device_id[4];
	device_id[0] = cfg->vendor_id;
	device_id[1] = 0xFFFF;
	device_id[2] = cfg->product_id;
	device_id[3] = cfg->version_id;
#else
	static const uint16_t device_id[4] = {0x03E0, 0xFFFF, 0x0001, 0x0001};
#endif
	return (uint16_t *)device_id;
}

int bt_manager_config_expect_tws_connect_role(void)
{
//    if(btif_br_check_share_tws()){
//        return BTSRV_TWS_SLAVE;
//    }
	return BTSRV_TWS_NONE;		/* Decide tws role by bt service */
}

/* mac: High mac address in low address.
 * Return: 0: not use random address, other: use random address.
 */
uint8_t bt_manager_config_get_gen_mac_method(uint8_t *mac)
{
#ifdef CONFIG_BT_EARPHONE_SPEC
	btmgr_base_config_t *cfg = bt_manager_get_base_config();
	uint8_t i;

	for (i = 0; i < 6; i++) {
		mac[i] = cfg->bt_address[5 - i];
	}
	return cfg->use_random_bt_address;
#else
	memset(mac, 0, 6);
	return 1;
#endif
}
