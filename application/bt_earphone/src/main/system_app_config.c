/*
 * Copyright (c) 2021Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app config
 */
#include <soc.h>
#include <mem_manager.h>
#include <property_manager.h>
#include "app_config.h"
#include "bt_manager.h"
#include "system_app.h"
#include "app_common.h"
#include "audio_policy.h"

#ifdef CONFIG_BT_TRANSCEIVER
#include "tr_system_app_config.c"
#endif

void system_enter_bqb_test_mode(uint8_t mode)
{
    btmgr_feature_cfg_t *cfg_feature = bt_manager_get_feature_config();
	uint8_t  enter_bqb_test_mode_flag = mode;

    if (mode == 0)
    {
        enter_bqb_test_mode_flag = cfg_feature->controller_test_mode;
    }
    SYS_LOG_INF("enter_bqb_test_mode:%d",enter_bqb_test_mode_flag);

    property_set(CFG_ENTER_BQB_TEST_MODE, &enter_bqb_test_mode_flag, sizeof(uint8_t));
	property_flush(CFG_ENTER_BQB_TEST_MODE);
    system_power_reboot(REBOOT_TYPE_GOTO_SYSTEM|REBOOT_REASON_GOTO_BQB);
}


void system_exit_bqb_test_mode(void)
{
    btmgr_feature_cfg_t *cfg_feature = bt_manager_get_feature_config();
    system_configs_t*  sys_configs = system_get_configs();
    CFG_Type_Auto_Quit_BT_Ctrl_Test*  cfg = &sys_configs->cfg_auto_quit_bt_ctrl_test;

    SYS_LOG_INF("%d, %d",cfg_feature->con_real_test_mode, cfg->Power_Off_After_Quit);

    if (cfg_feature->con_real_test_mode == DISABLE_TEST)
    {    
        return;
    }

    /* 退出后关机?
     */
    if (cfg->Power_Off_After_Quit)
    {		 
        system_power_off();
    }
    else
    {
        system_power_reboot(REBOOT_TYPE_GOTO_SYSTEM|REBOOT_REASON_NORMAL);
    }
    
}

static void btmgr_check_bqb_test_mode(void)
{
    btmgr_feature_cfg_t *cfg_feature = bt_manager_get_feature_config();
    uint8_t  enter_bqb_test_mode_flag;
    int ret;

	ret = property_get(CFG_ENTER_BQB_TEST_MODE, &enter_bqb_test_mode_flag, sizeof(uint8_t));
	if (ret < 0) {
		enter_bqb_test_mode_flag = 0;
	}

	if (enter_bqb_test_mode_flag == 0)
	{
		cfg_feature->con_real_test_mode = DISABLE_TEST;
	}
	else
	{
		cfg_feature->con_real_test_mode = cfg_feature->controller_test_mode;
	
		/* 清除标志
		 */
		enter_bqb_test_mode_flag = 0;
		property_set(CFG_ENTER_BQB_TEST_MODE, &enter_bqb_test_mode_flag, sizeof(uint8_t));
		property_flush(CFG_ENTER_BQB_TEST_MODE);			
	}


	if (!cfg_feature->enter_bqb_test_mode_by_key &&
		(cfg_feature->con_real_test_mode == DISABLE_TEST))
	{
        cfg_feature->con_real_test_mode = cfg_feature->controller_test_mode;
	}

	SYS_LOG_INF("con_real_test_mode:%d", cfg_feature->con_real_test_mode);    
}



/* 根据硬件设定设备声道添加蓝牙名称后缀?
 */
static void btmgr_add_suffix_name(btmgr_base_config_t * p, CFG_Struct_BT_Device* cfg_bt_dev)
{
    int  len = strlen(p->bt_device_name);

    u32_t  channel = audio_channel_get_config()->hw_sel_dev_channel;

    if (channel == AUDIO_DEVICE_CHANNEL_L)
    {
        strncpy
        (
            &p->bt_device_name[len], 
            cfg_bt_dev->Left_Device_Suffix, 
            sizeof(p->bt_device_name) - len
        );
    }
    else if (channel == AUDIO_DEVICE_CHANNEL_R)
    {
        strncpy
        (
            &p->bt_device_name[len], 
            cfg_bt_dev->Right_Device_Suffix, 
            sizeof(p->bt_device_name) - len
        );
    }

    if ((channel == AUDIO_DEVICE_CHANNEL_L) || (channel == AUDIO_DEVICE_CHANNEL_R))
    {
        bt_manager_set_bt_name(p->bt_device_name);
        SYS_LOG_INF("**********bt_device_name:%s*********",p->bt_device_name);
    }
}



int btmgr_config_update_1st(void)
{
    int ret = 0;
    int i;
    u32_t  f;
    btmgr_base_config_t *base_config = bt_manager_get_base_config();
    btmgr_feature_cfg_t *feature_config = bt_manager_get_feature_config();
	
    CFG_Struct_BT_Device   cfg_bt_dev;
    CFG_Struct_BT_Manager  cfg_bt_mgr;
    CFG_Struct_BT_Link_Quality	link_quality;
    CFG_Struct_BT_Scan_Params cfg_scan_params;

    memset(base_config, 0, sizeof(btmgr_base_config_t));
    memset(feature_config, 0, sizeof(btmgr_feature_cfg_t));

    app_config_read(CFG_ID_BT_DEVICE,&cfg_bt_dev, 0, sizeof(CFG_Struct_BT_Device));
    app_config_read(CFG_ID_BT_MANAGER, &cfg_bt_mgr, 0, sizeof(CFG_Struct_BT_Manager));
	
	memcpy(base_config->bt_device_name, cfg_bt_dev.BT_Device_Name,sizeof(base_config->bt_device_name));
	memcpy(base_config->bt_address, cfg_bt_dev.BT_Address, sizeof(base_config->bt_address));
	base_config->use_random_bt_address = cfg_bt_dev.Use_Random_BT_Address;
	base_config->bt_device_class = cfg_bt_dev.BT_Device_Class;
	
	base_config->default_hosc_capacity = cfg_bt_dev.Default_HOSC_Capacity;
	base_config->force_default_hosc_capacity = cfg_bt_dev.Force_Default_HOSC_Capacity;
	base_config->bt_max_rf_tx_power = cfg_bt_dev.BT_Max_RF_TX_Power;
	base_config->ble_rf_tx_power = cfg_bt_dev.BLE_RF_TX_Power;
	base_config->ad2p_bitpool = cfg_bt_dev.A2DP_Bitpool;
	base_config->vendor_id = cfg_bt_dev.Vendor_ID;
	base_config->product_id = cfg_bt_dev.Product_ID;
	base_config->version_id = cfg_bt_dev.Version_ID;
	app_config_read(CFG_ID_BT_LINK_QUALITY, &link_quality, 0, sizeof(link_quality));
	base_config->link_quality.quality_pre_val	= link_quality.Quality_Pre_Value;
	base_config->link_quality.quality_diff		= link_quality.Quality_Diff;
	base_config->link_quality.quality_esco_diff = link_quality.Quality_ESCO_Diff;
	base_config->link_quality.quality_monitor	= link_quality.Quality_Monitor;

	app_config_read(CFG_ID_BT_SCAN_PARAMS, &cfg_scan_params, 0, sizeof(CFG_Struct_BT_Scan_Params));

	for(i=0; i< 7;i++)
	{
		base_config->scan_params[i].scan_mode = cfg_scan_params.Params[i].Scan_Mode;
		base_config->scan_params[i].inquiry_scan_window = cfg_scan_params.Params[i].Inquiry_Scan_Window;
		base_config->scan_params[i].inquiry_scan_interval = cfg_scan_params.Params[i].Inquiry_Scan_Interval;		
		base_config->scan_params[i].inquiry_scan_type = cfg_scan_params.Params[i].Inquiry_Scan_Type;
		base_config->scan_params[i].page_scan_window = cfg_scan_params.Params[i].Page_Scan_Window;
		base_config->scan_params[i].page_scan_interval = cfg_scan_params.Params[i].Page_Scan_Interval;
		base_config->scan_params[i].page_scan_type = cfg_scan_params.Params[i].Page_Scan_Type;
	}
	
	f = cfg_bt_mgr.Support_Features;

	/*蓝牙支持特性
	 */
	feature_config->sp_avdtp_aac 	 = (f & BT_SUPPORT_A2DP_AAC)			 ? 1 : 0;
	feature_config->sp_avrcp_vol_syn  = (f & BT_SUPPORT_AVRCP_VOLUME_SYNC)	 ? 1 : 0;
	feature_config->sp_hfp_vol_syn	 = (f & BT_SUPPORT_HFP_VOLUME_SYNC) 	 ? 1 : 0;
	feature_config->sp_hfp_bat_report = (f & BT_SUPPORT_HFP_BATTERY_REPORT)	 ? 1 : 0;
	feature_config->sp_hfp_3way_call  = (f & BT_SUPPORT_HFP_3WAY_CALL)		 ? 1 : 0;
	feature_config->sp_hfp_siri		 = (f & BT_SUPPORT_HFP_VOICE_ASSIST)	 ? 1 : 0;
	feature_config->sp_tws		     = (f & BT_SUPPORT_TWS)			 ? 1 : 0;
	feature_config->sp_linkkey_miss_reject = (f & BT_SUPPORT_LINKKEY_MISS_REJECT)   ? 1 : 0;
	if (f & BT_SUPPORT_A2DP_SCMS_T)
	{
		feature_config->sp_a2dp_cp = 2;
	}
	else if (f & BT_SUPPORT_A2DP_DTCP)
	{
		feature_config->sp_a2dp_cp = 1;
	}
	feature_config->sp_sniff_enable		  = (f & BT_SUPPORT_ENABLE_SNIFF)		   ? 1 : 0;
	feature_config->sp_hfp_enable_nrec		  = (f & BT_SUPPORT_ENABLE_NREC)		   ? 1 : 0;
	feature_config->sp_hfp_codec_negotiation   = (f & BT_SUPPORT_HFP_CODEC_NEGOTIATION) ? 1 : 0;
	feature_config->sp_clear_linkkey = (f & BT_SUPPORT_CLEAR_LINKKEY)   ? 1 : 0;

	feature_config->max_pair_list_num = cfg_bt_mgr.Paired_Device_Save_Number;
	feature_config->sp_device_num	 = cfg_bt_mgr.Support_Device_Number;

	if (f & BT_SUPPORT_DUAL_PHONE_DEV_LINK)
	{
		feature_config->sp_dual_phone = (cfg_bt_mgr.Support_Device_Number > 1) ? 1 : 0;
	}
	else if (cfg_bt_mgr.Support_Device_Number > 2){
		/*如果不支持双手机,则支持的设备数最高为2*/
		feature_config->sp_device_num	 = 2;
	}

	feature_config->controller_test_mode 	  = cfg_bt_mgr.Controller_Test_Mode;
	feature_config->enter_bqb_test_mode_by_key	= cfg_bt_mgr.Enter_BQB_Test_Mode_By_Key?1:0;

	feature_config->enter_sniff_time 		 = cfg_bt_mgr.Idle_Enter_Sniff_Time_Ms;
	feature_config->sniff_interval			 = cfg_bt_mgr.Sniff_Interval_Ms;


    /* 根据硬件设定设备声道添加蓝牙名称后缀?
     */
    btmgr_add_suffix_name(base_config, &cfg_bt_dev);	

#ifdef CONFIG_BT_TRANSCEIVER
	tr_btmgr_config_update_1st();
#endif
    return ret;
}


int btmgr_config_update_2nd(void)
{
    int ret = 0;
	int i;	
    btmgr_pair_cfg_t *pair_config = bt_manager_get_pair_config();
    btmgr_tws_pair_cfg_t *tws_pair_config = bt_manager_get_tws_pair_config();
    btmgr_sync_ctrl_cfg_t *sync_ctrl_config = bt_manager_get_sync_ctrl_config();
    btmgr_reconnect_cfg_t *reconnect_config = bt_manager_get_reconnect_config();
    btmgr_rf_param_cfg_t* rf_prarm_config = bt_manager_rf_param_config();
	
    memset(pair_config, 0, sizeof(btmgr_pair_cfg_t));
    memset(tws_pair_config, 0, sizeof(btmgr_tws_pair_cfg_t));
    memset(sync_ctrl_config, 0, sizeof(btmgr_sync_ctrl_cfg_t));
    memset(reconnect_config, 0, sizeof(btmgr_reconnect_cfg_t));
    memset(rf_prarm_config, 0, sizeof(btmgr_rf_param_cfg_t));

    CFG_Struct_BT_Pair     cfg_bt_pair;
    CFG_Struct_TWS_Pair    cfg_tws_pair;
    CFG_Struct_TWS_Advanced_Pair cfg_tws_advanced_pair;
    CFG_Struct_BT_Music_Volume_Sync  cfg_bt_music_vol_sync;
    CFG_Struct_BT_Two_Device_Play    cfg_two_dev_play;
    CFG_Struct_BT_Call_Volume_Sync   cfg_bt_call_vol_sync;
    CFG_Struct_BT_Auto_Reconnect cfg_reconnect;	
    CFG_Struct_Volume_Settings cfg_volume_settings;
    CFG_Struct_BT_HID_Settings cfg_hid_settings;
    CFG_Struct_BT_RF_Param_Table cfg_rf_param_table;


	/* 配对连接配置
	 */
	app_config_read(CFG_ID_BT_PAIR, &cfg_bt_pair, 0, sizeof(CFG_Struct_BT_Pair));

	pair_config->default_state_discoverable = cfg_bt_pair.Default_State_Discoverable ? 1 : 0;
	pair_config->default_state_wait_connect_sec = cfg_bt_pair.Default_State_Wait_Connect_Sec;
	pair_config->pair_mode_duration_sec = cfg_bt_pair.Pair_Mode_Duration_Sec;
	pair_config->disconnect_all_phones_when_enter_pair_mode = cfg_bt_pair.Disconnect_All_Phones_When_Enter_Pair_Mode ? 1 : 0;
	pair_config->clear_paired_list_when_enter_pair_mode = cfg_bt_pair.Clear_Paired_List_When_Enter_Pair_Mode ? 1 : 0;
	pair_config->clear_tws_When_key_clear_paired_list = cfg_bt_pair.Clear_TWS_When_Key_Clear_Paired_List ? 1 : 0;
	pair_config->enter_pair_mode_when_key_clear_paired_list = cfg_bt_pair.Enter_Pair_Mode_When_Key_Clear_Paired_List ? 1 : 0;
	pair_config->enter_pair_mode_when_paired_list_empty = cfg_bt_pair.Enter_Pair_Mode_When_Paired_List_Empty ? 1 : 0;
	pair_config->bt_not_discoverable_when_connected = cfg_bt_pair.BT_Not_Discoverable_When_Connected ? 1 : 0;

	app_config_read
	(
		CFG_ID_TWS_PAIR, 
		&cfg_tws_pair, 0, sizeof(CFG_Struct_TWS_Pair)
	);

	tws_pair_config->pair_key_mode = cfg_tws_pair.TWS_Pair_Key_Mode;
	tws_pair_config->match_mode = cfg_tws_pair.Match_Mode;
	tws_pair_config->match_name_length = cfg_tws_pair.Match_Name_Length;
	tws_pair_config->wait_pair_search_time_sec = cfg_tws_pair.TWS_Wait_Pair_Search_Time_Sec;
	tws_pair_config->power_on_auto_pair_search = cfg_tws_pair.TWS_Power_On_Auto_Pair_Search;

    /* check TWS pair match name length (not include suffix name) */
    {
        char origin_bt_name[CFG_MAX_BT_DEV_NAME_LEN];
        int  len;
        
        app_config_read(CFG_ID_BT_DEVICE, origin_bt_name, 
            offsetof(CFG_Struct_BT_Device, BT_Device_Name), CFG_MAX_BT_DEV_NAME_LEN
        );

        len = strlen(origin_bt_name);

        if (tws_pair_config->match_name_length > len) {
            tws_pair_config->match_name_length = len;
        }
    }

	app_config_read
	(
		CFG_ID_TWS_ADVANCED_PAIR,
		&cfg_tws_advanced_pair,0,sizeof(CFG_Struct_TWS_Advanced_Pair)
	);
	
	tws_pair_config->enable_advanced_pair_mode = cfg_tws_advanced_pair.Enable_TWS_Advanced_Pair_Mode? 1: 0;
	tws_pair_config->check_rssi_when_tws_pair_search = cfg_tws_advanced_pair.Check_RSSI_When_TWS_Pair_Search? 1: 0;
	tws_pair_config->use_search_mode_when_tws_reconnect = cfg_tws_advanced_pair.Use_Search_Mode_When_TWS_Reconnect? 1: 0;
	tws_pair_config->rssi_threshold = cfg_tws_advanced_pair.RSSI_Threshold;

	/* TWS 左右设备组对限制 */
	{
		u8_t  dev_channel = audio_channel_get_config()->hw_sel_dev_channel;
		
		tws_pair_config->tws_device_channel_l_r = dev_channel;
		tws_pair_config->sp_device_l_r_match    = (dev_channel != NONE) ? 1 : 0;
	}

	tws_pair_config->tws_pair_count			  = 0xff;
	tws_pair_config->tws_role_select 		  = 0;

	/* 音量同步配置
	 */
	app_config_read
	(
		CFG_ID_BT_MUSIC_VOLUME_SYNC, 
		&cfg_bt_music_vol_sync, 0, sizeof(CFG_Struct_BT_Music_Volume_Sync)
	);

	sync_ctrl_config->a2dp_volume_sync_when_playing = cfg_bt_music_vol_sync.Volume_Sync_Only_When_Playing? 1:0;
	sync_ctrl_config->a2dp_origin_volume_sync_to_remote = cfg_bt_music_vol_sync.Origin_Volume_Sync_To_Remote? 1:0;
	sync_ctrl_config->a2dp_origin_volume_sync_delay_ms = cfg_bt_music_vol_sync.Origin_Volume_Sync_Delay_Ms;
	sync_ctrl_config->a2dp_Playing_volume_sync_delay_ms = cfg_bt_music_vol_sync.Playing_Volume_Sync_Delay_Ms;
		
	app_config_read
	(
		CFG_ID_BT_CALL_VOLUME_SYNC, 
		&cfg_bt_call_vol_sync, 0, sizeof(CFG_Struct_BT_Call_Volume_Sync)
	);

	sync_ctrl_config->hfp_origin_volume_sync_to_remote = cfg_bt_call_vol_sync.Origin_Volume_Sync_To_Remote? 1:0;
	sync_ctrl_config->hfp_origin_volume_sync_delay_ms = cfg_bt_call_vol_sync.Origin_Volume_Sync_Delay_Ms;

    app_config_read
    (
        CFG_ID_VOLUME_SETTINGS, 
        &cfg_volume_settings, 0, sizeof(CFG_Struct_Volume_Settings)
    );

	sync_ctrl_config->bt_music_default_volume = cfg_volume_settings.BT_Music_Default_Volume;
	sync_ctrl_config->bt_call_default_volume = cfg_volume_settings.BT_Call_Default_Volume;
	sync_ctrl_config->bt_music_default_volume_ex = cfg_volume_settings.BT_Music_Default_Vol_Ex;
	
	/* 双设备播放配置
	 */
	app_config_read
	(
		CFG_ID_BT_TWO_DEVICE_PLAY, 
		&cfg_two_dev_play, 0, sizeof(CFG_Struct_BT_Two_Device_Play)
	);

	sync_ctrl_config->stop_another_when_one_playing = cfg_two_dev_play.Stop_Another_When_One_Playing? 1:0;
	sync_ctrl_config->resume_another_when_one_stopped = cfg_two_dev_play.Resume_Another_When_One_Stopped? 1:0;
	sync_ctrl_config->a2dp_status_stopped_delay_ms = cfg_two_dev_play.A2DP_Status_Stopped_Delay_Ms;


	app_config_read
	(
		CFG_ID_BT_AUTO_RECONNECT, 
		&cfg_reconnect, 0, sizeof(CFG_Struct_BT_Auto_Reconnect)
	);
	reconnect_config->enable_auto_reconnect = cfg_reconnect.Enable_Auto_Reconnect;
	reconnect_config->reconnect_phone_timeout = cfg_reconnect.Reconnect_Phone_Timeout;
	reconnect_config->reconnect_phone_interval = cfg_reconnect.Reconnect_Phone_Interval;
	reconnect_config->reconnect_phone_times_by_startup = cfg_reconnect.Reconnect_Phone_Times_By_Startup;
	reconnect_config->reconnect_tws_timeout = cfg_reconnect.Reconnect_TWS_Timeout;
	reconnect_config->reconnect_tws_interval = cfg_reconnect.Reconnect_TWS_Interval;
	reconnect_config->reconnect_tws_times_by_startup = cfg_reconnect.Reconnect_TWS_Times_By_Startup;
	reconnect_config->reconnect_times_by_timeout = cfg_reconnect.Reconnect_Times_By_Timeout;
	reconnect_config->enter_pair_mode_when_startup_reconnect_fail = cfg_reconnect.Enter_Pair_Mode_When_Startup_Reconnect_Fail;

    if(reconnect_config->reconnect_phone_interval <= reconnect_config->reconnect_phone_timeout){
        reconnect_config->reconnect_phone_interval = reconnect_config->reconnect_phone_timeout + 10;
	}
    if(reconnect_config->reconnect_tws_interval <= reconnect_config->reconnect_tws_timeout){
        reconnect_config->reconnect_tws_interval = reconnect_config->reconnect_tws_timeout + 10;
	}

    app_config_read
    (
        CFG_ID_BT_HID_SETTINGS, 
        &cfg_hid_settings, 0, sizeof(CFG_Struct_BT_HID_Settings)
    );
	
	reconnect_config->hid_auto_disconnect_delay_sec = cfg_hid_settings.HID_Auto_Disconnect_Delay_Sec;
	reconnect_config->hid_connect_operation_delay_ms = cfg_hid_settings.HID_Connect_Operation_Delay_Ms;

    app_config_read
    (
        CFG_ID_BT_RF_PARAM_TABLE, 
        &cfg_rf_param_table, 0, sizeof(CFG_Struct_BT_RF_Param_Table)
    );

	for(i=0; i < CFG_MAX_RF_TABLE_LEN; i++)
	{
		rf_prarm_config->param[i].addr = cfg_rf_param_table.Param_Table[i].Addr;
		rf_prarm_config->param[i].value = cfg_rf_param_table.Param_Table[i].Value;
		SYS_LOG_DBG("Addr:%x, Value:%x",rf_prarm_config->param[i].addr,rf_prarm_config->param[i].value);
	}
    return ret;
}


int btmgr_config_update_3rd(void)
{
    int ret = 0;
    btmgr_ble_cfg_t* ble_config = bt_manager_ble_config();
	btmgr_leaudio_cfg_t* leaudio_config = bt_manager_leaudio_config();

    memset(ble_config, 0, sizeof(btmgr_ble_cfg_t));
    memset(leaudio_config, 0, sizeof(btmgr_leaudio_cfg_t));

    CFG_Struct_BLE_Manager cfg_ble_manager;
    CFG_Struct_BLE_Advertising_Mode_1 cfg_ble_adv_mode1;
    CFG_Struct_BLE_Advertising_Mode_2 cfg_ble_adv_mode2;	
    CFG_Struct_BLE_Connection_Param cfg_ble_connection_param;
	CFG_Struct_LE_AUDIO_Manager	cfg_leaudio_manager;

    /* BLE 管理配置
     */
    app_config_read(CFG_ID_BLE_MANAGER,&cfg_ble_manager,0,sizeof(CFG_Struct_BLE_Manager));
    app_config_read(CFG_ID_BLE_ADVERTISING_MODE_1,&cfg_ble_adv_mode1,0,sizeof(CFG_Struct_BLE_Advertising_Mode_1));
    app_config_read(CFG_ID_BLE_ADVERTISING_MODE_2,&cfg_ble_adv_mode2,0,sizeof(CFG_Struct_BLE_Advertising_Mode_2));
    app_config_read(CFG_ID_BLE_CONNECTION_PARAM,&cfg_ble_connection_param,0,sizeof(CFG_Struct_BLE_Connection_Param));

    ble_config->ble_enable = cfg_ble_manager.BLE_Enable?1:0;
    ble_config->use_advertising_mode_2_after_paired = cfg_ble_manager.Use_Advertising_Mode_2_After_Paired?1:0;
    ble_config->ble_address_type = cfg_ble_manager.BLE_Address_Type;

    ble_config->connection_param.interval_min_ms = cfg_ble_connection_param.Interval_Min_Ms;
    ble_config->connection_param.interval_max_ms = cfg_ble_connection_param.Interval_Max_Ms;
    ble_config->connection_param.latency = cfg_ble_connection_param.Latency;
    ble_config->connection_param.timeout_Ms = cfg_ble_connection_param.Timeout_Ms;

    /* LE AUDIO 管理配置
     */
    app_config_read(CFG_ID_LE_AUDIO_MANAGER,&cfg_leaudio_manager,0,sizeof(CFG_Struct_LE_AUDIO_Manager));	
    leaudio_config->leaudio_enable = cfg_leaudio_manager.LE_AUDIO_Enable?1:0;
    if (leaudio_config->leaudio_enable){
        leaudio_config->app_auto_switch = cfg_leaudio_manager.LE_AUDIO_APP_Auto_Switch?1:0;
    }
    leaudio_config->leaudio_dir_adv_enable = cfg_leaudio_manager.LE_AUDIO_Dir_Adv_Enable?1:0;
	
    return ret;
}

static int bt_player_config_update(void)
{
    int ret = 0;
    CFG_Struct_Volume_Settings cfg_volume_settings;
    char tmp[4];

    app_config_read(CFG_ID_VOLUME_SETTINGS, &cfg_volume_settings, 0, sizeof(CFG_Struct_Volume_Settings));
    sprintf(tmp, "%d", cfg_volume_settings.BT_Music_Default_Volume);
    property_set(CFG_BTPLAY_VOLUME, tmp, strlen(tmp)+1);
    property_set(CFG_LEPLAY_VOLUME, tmp, strlen(tmp)+1);
    
    sprintf(tmp, "%d", cfg_volume_settings.BT_Call_Default_Volume);
    property_set(CFG_BTCALL_VOLUME, tmp, strlen(tmp)+1);	
    property_set(CFG_LECALL_VOLUME, tmp, strlen(tmp)+1);

    sprintf(tmp, "%d", cfg_volume_settings.Voice_Default_Volume);
    property_set(CFG_TONE_VOLUME, tmp, strlen(tmp)+1);

    return ret;
}

static int btmgr_configs_update(void)
{
    int update_config = 0;
    int ret;

    if (btmgr_config_update_1st() != 0)
        return -1;
    if (btmgr_config_update_2nd() != 0)
        return -1;
    if (btmgr_config_update_3rd() != 0)
        return -1;

    ret = property_get(CFG_UPDATED_CONFIG, (char*)&update_config, sizeof(update_config));
    if (ret > 0) {
        if (update_config == 1) {
            return 0;
        }
    }

    LOG_WRN("begin update config...");

    if (bt_player_config_update() != 0)
        return -1;

    LOG_WRN("end update config");

    update_config = 1;
    property_set(CFG_UPDATED_CONFIG, (char*)&update_config, sizeof(update_config));

    return 0;	
}


int system_app_config_init(void)
{
    system_app_context_t*  manager = system_app_get_context();
    system_configs_t*  sys_configs = system_get_configs();

    /* 需要先加载默认配置文件
     * 文件存放于 ROM 中时可以不使用缓存
     */
    app_config_load("defcfg.bin", NO, NO);

    /* 然后再加载用户配置文件
     */
    app_config_load("usrcfg.bin", NO, NO);

    /* 加载算法音效配置文件
     */
    app_config_load("alcfg.bin", NO, NO);

    /* 最后加载拓展配置文件（自定义音效）
     */
    app_config_load("extcfg.bin", NO, NO);

    CFG_Struct_User_Version   usr_ver;
    CFG_Struct_Platform_Case  plat_case;
	CFG_Struct_OTA_Settings ota_settings;
    
    app_config_read
    (
        CFG_ID_USER_VERSION, 
        &usr_ver, 0, sizeof(CFG_Struct_User_Version)
    );
    
    app_config_read
    (
        CFG_ID_PLATFORM_CASE, 
        &plat_case, 0, sizeof(CFG_Struct_Platform_Case)
    );

    app_config_read
    (
        CFG_ID_OTA_SETTINGS,
        &ota_settings,0,sizeof(CFG_Struct_OTA_Settings)
    );

    LOG_DBG("User_Ver  : %s",   usr_ver.Version);
    LOG_DBG("IC_Type   : 0x%x", plat_case.IC_Type);
    LOG_DBG("Board_Type: %d",   plat_case.Board_Type);
    LOG_DBG("Case_Type : %s %d.%d",plat_case.Case_Name,plat_case.Major_Version,plat_case.Minor_Version);
    LOG_INF("Firmware Version : %s", ota_settings.Version_Number);

    /* 系统配置
     */
    app_config_read
    (
        CFG_ID_SYSTEM_SETTINGS,
        &sys_configs->cfg_sys_settings, 0, sizeof(CFG_Struct_System_Settings)
    );

    app_config_read
    (
        CFG_ID_SYSTEM_MORE_SETTINGS,
        &sys_configs->cfg_sys_more_settings, 0, sizeof(CFG_Struct_System_More_Settings)
    );
	

    /* 不再支持 DC5V 接入时直接复位, 
     * 避免缓存的 VRAM 数据没有写入 FLASH 而丢失
     */
    sys_configs->cfg_sys_settings.Support_Features &= ~(SYS_ENABLE_DC5V_IN_RESET);


    /* ONOFF 按键配置
     */
    app_config_read
    (
        CFG_ID_ONOFF_KEY,
        &sys_configs->cfg_onoff_key, 0, sizeof(CFG_Struct_ONOFF_Key)
    );
    
    /* 按键音配置
     */
    app_config_read
    (
        CFG_ID_KEY_TONE,
        &sys_configs->cfg_key_tone, 0, sizeof(CFG_Struct_Key_Tone)
    );

    /* 事件通知列表
     */
    app_config_read
    (
        CFG_ID_EVENT_NOTIFY,
        &sys_configs->cfg_event_notify, 0, sizeof(CFG_Struct_Event_Notify)
    );

    /* TWS 同步配置
     */
    app_config_read
    (
        CFG_ID_TWS_SYNC,
        &sys_configs->cfg_tws_sync, 0, sizeof(CFG_Struct_TWS_Sync)
    );

    /* 充电配置
     */
    app_config_read
    (
        CFG_ID_BATTERY_CHARGE,
        &sys_configs->cfg_charge, 0, sizeof(CFG_Struct_Battery_Charge)
    );

    /* 充电盒配置
     */
    app_config_read
    (
        CFG_ID_CHARGER_BOX,
        &sys_configs->cfg_charger_box, 0, sizeof(CFG_Struct_Charger_Box)
    );

    app_config_read
    (
        CFG_ID_BATTERY_LOW,
        &sys_configs->bat_low_prompt_interval_sec,
        offsetof(CFG_Struct_Battery_Low, Battery_Low_Prompt_Interval_Sec),
        sizeof((((CFG_Struct_Battery_Low*)0)->Battery_Low_Prompt_Interval_Sec))
    );

    /* 敲击按键支持配置
     */
    CFG_Type_Tap_Key_Control  cfg_tap_key;
    app_config_read
    (
        CFG_ID_TAP_KEY, 
        &cfg_tap_key,
        offsetof(CFG_Struct_Tap_Key, Tap_Key_Control),
        sizeof(CFG_Type_Tap_Key_Control)
    );
    sys_configs->tap_key_tone = cfg_tap_key.Tap_Key_Tone;

    /* 语音语言
     */
    int ret = property_get(CFG_VOICE_LANGUAGE, &manager->select_voice_language, sizeof(u8_t));
    if (ret < 0) {
        manager->select_voice_language = sys_configs->cfg_sys_settings.Default_Voice_Language;
    }	

    app_config_read
    (
        CFG_ID_BT_MANAGER,
        &sys_configs->cfg_auto_quit_bt_ctrl_test,
        offsetof(CFG_Struct_BT_Manager, Auto_Quit_BT_Ctrl_Test),
        sizeof(CFG_Type_Auto_Quit_BT_Ctrl_Test)
    );


    audio_channel_init();
    system_audiolcy_config_init();

    if(!btmgr_configs_update()){
		manager->bt_manager_config_inited = true;
    }

    btmgr_check_bqb_test_mode();

    /* 开机长按键功能?
     */
    manager->boot_hold_key_func = BOOT_HOLD_KEY_FUNC_NONE;
    manager->boot_hold_key_begin_time = os_uptime_get_32();

    return 0;
}

/*!
 * \brief 获取系统管理配置
 */
system_configs_t* system_get_configs(void)
{
    system_app_context_t*  manager = system_app_get_context();
    
    return &manager->sys_configs;
}


static bool system_notify_check_options(uint32_t options)
{
    audio_manager_configs_t*  cfg = audio_channel_get_config();
    int dev_role = bt_manager_tws_get_dev_role();

    /* 已组对?
     */
    if ((options & EVENT_NOTIFY_TWS_PAIRED) &&
        !(options & EVENT_NOTIFY_TWS_UNPAIRED))
    {
        if (dev_role < BTSRV_TWS_MASTER)
        {
            return false;
        }
    }

    /* 未组对?
     */
    if ((options & EVENT_NOTIFY_TWS_UNPAIRED) &&
        !(options & EVENT_NOTIFY_TWS_PAIRED))
    {
        if (dev_role >= BTSRV_TWS_MASTER)
        {
            return true;
        }
    }

    /* L 左设备标识?
     */
    if ((options & EVENT_NOTIFY_CHANNEL_L) &&
        !(options & EVENT_NOTIFY_CHANNEL_R))
    {
        if (cfg->hw_sel_dev_channel != AUDIO_DEVICE_CHANNEL_L &&
            cfg->sw_sel_dev_channel != AUDIO_DEVICE_CHANNEL_L)
        {
            return false;
        }
    }

    /* R 右设备标识?
     */
    if ((options & EVENT_NOTIFY_CHANNEL_R) &&
        !(options & EVENT_NOTIFY_CHANNEL_L))
    {
        if (cfg->hw_sel_dev_channel != AUDIO_DEVICE_CHANNEL_R &&
            cfg->sw_sel_dev_channel != AUDIO_DEVICE_CHANNEL_R)
        {
            return false;
        }
    }

    return true;
}


/* 获取事件通知配置
 */
CFG_Type_Event_Notify* system_notify_get_config(uint32_t event_id)
{
    system_app_context_t*  manager = system_app_get_context();

    CFG_Struct_Event_Notify*  cfg = &manager->sys_configs.cfg_event_notify;
    
    int  i;

    if (event_id == UI_EVENT_NONE)
    {
        return NULL;
    }

    for (i = 0; i < CFG_MAX_EVENT_NOTIFY; i++)
    {
        if (cfg->Notify[i].Event_Type == event_id)
        {
            if (system_notify_check_options(cfg->Notify[i].Options) == false)
            {
                continue;
            }
            break;
        }
    }

    if (i < CFG_MAX_EVENT_NOTIFY)
    {
        return &(cfg->Notify[i]);
    }

    return NULL;
}

/*!
 * \brief 检查事件通知是否会播报语音
 * \n  event_id : CFG_TYPE_SYS_EVENT
 */

bool sys_check_notify_voice(uint32_t event_id)
{
    CFG_Type_Event_Notify*  cfg_notify;
    char voice_name[CFG_MAX_VOICE_NAME_LEN + CFG_MAX_VOICE_FMT_LEN];

    cfg_notify =  system_notify_get_config(event_id);
    if(NULL == cfg_notify)
    {
        return false;
    }

    if (audio_voice_get_name_ex(cfg_notify->Voice_Play,voice_name))
    {
        return true;
    }

    return false;
}



