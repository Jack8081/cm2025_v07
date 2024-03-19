/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system bt operates
 */

#include <bt_manager.h>
#include <system_app.h>
#include "system_bt.h"

#ifdef CONFIG_LE_AUDIO_APP
static int leaudio_base_init(void);
static void leaudio_base_deinit(void);
#define DEFAULT_CONTEXTS	(BT_AUDIO_CONTEXT_UNSPECIFIED | \
				BT_AUDIO_CONTEXT_CONVERSATIONAL | \
				BT_AUDIO_CONTEXT_MEDIA)

#endif

#ifdef CONFIG_BT_TRANSCEIVER
#include "tr_system_bt_ops.c"
#endif

int sys_bt_init(void)
{
#ifdef CONFIG_BT_MANAGER
	bt_manager_init();

#ifdef CONFIG_BT_BLE
	sys_ble_advertise_init();
#endif
#endif

	return 0;
}

int sys_bt_deinit(void)
{
#ifdef CONFIG_LE_AUDIO_APP
	btmgr_leaudio_cfg_t* leaudio_config = bt_manager_leaudio_config();
	if (leaudio_config->leaudio_enable){
		leaudio_base_deinit();
	}
#endif

#ifdef CONFIG_BT_MANAGER
	bt_manager_deinit();
#endif

	return 0;
}

int sys_bt_ready(void)
{
	bt_manager_set_mailbox_ctx();

#ifdef CONFIG_LE_AUDIO_APP
	btmgr_leaudio_cfg_t* leaudio_config = bt_manager_leaudio_config();
	if (leaudio_config->leaudio_enable){
		leaudio_base_init();
	}
#endif

	return 0;
}


int sys_bt_restart_leaudio(void)
{
#ifdef CONFIG_LE_AUDIO_APP
	btmgr_leaudio_cfg_t* leaudio_config = bt_manager_leaudio_config();
	if (leaudio_config->leaudio_enable){
		leaudio_base_deinit();	
		leaudio_base_init();
	}
#endif

	return 0;
}


#ifdef CONFIG_LE_AUDIO_APP

#ifdef CONFIG_LE_AUDIO_PHONE
static int leaudio_phone_cap_init(struct bt_audio_capabilities_2* caps)
{

#ifdef CONFIG_LE_AUDIO_BQB
    struct bt_audio_capability *cap;

    caps->source_cap_num = 1;
	caps->sink_cap_num = 1;
	/* Audio Source */
	cap = &caps->cap[0];
	cap->format = BT_AUDIO_CODEC_LC3;
	cap->channels = BT_AUDIO_CHANNEL_COUNTS_1;
	cap->frames = 1;

    cap->samples = bt_supported_sampling_from_khz(8) |
                bt_supported_sampling_from_khz(16) |
                bt_supported_sampling_from_khz(24) |
                bt_supported_sampling_from_khz(32) |
                bt_supported_sampling_from_khz(48);
    cap->octets_min = 26;
    cap->octets_max = 155;
    cap->durations = bt_supported_frame_durations_from_ms(10, false);
    cap->durations |= bt_supported_frame_durations_from_ms(7, false);


	cap->preferred_contexts = DEFAULT_CONTEXTS;
	caps->source_available_contexts = DEFAULT_CONTEXTS;
	caps->source_supported_contexts = DEFAULT_CONTEXTS;

	caps->source_locations = BT_AUDIO_LOCATIONS_FL;

	/* Audio Sink */
	cap = &caps->cap[1];
	cap->format = BT_AUDIO_CODEC_LC3;

	cap->channels = BT_AUDIO_CHANNEL_COUNTS_1;
	cap->frames = 1;

	cap->samples = bt_supported_sampling_from_khz(8) |
			bt_supported_sampling_from_khz(16) |
			bt_supported_sampling_from_khz(24) |
			bt_supported_sampling_from_khz(32) |
			bt_supported_sampling_from_khz(48);
	cap->octets_min = 26;
	cap->octets_max = 155;
	cap->durations = bt_supported_frame_durations_from_ms(10, false);
	cap->durations |= bt_supported_frame_durations_from_ms(7, false);

    caps->sink_locations = BT_AUDIO_LOCATIONS_FL;

	cap->preferred_contexts = DEFAULT_CONTEXTS;

#else
	struct bt_audio_capability *cap;
	btmgr_feature_cfg_t *cfg_feature = bt_manager_get_feature_config();
	CFG_Struct_LE_AUDIO_Advertising_Mode cfg_le_audio_adv_mode;
	uint8_t LC3_Interval;

	app_config_read(CFG_ID_LE_AUDIO_ADVERTISING_MODE, &cfg_le_audio_adv_mode, 0, sizeof(CFG_Struct_LE_AUDIO_Advertising_Mode));
	LC3_Interval = cfg_le_audio_adv_mode.LE_AUDIO_LC3_Interval;

	caps->source_cap_num = 1;
	caps->sink_cap_num = 1;

	/* Audio Source */
	cap = &caps->cap[0];
	cap->format = BT_AUDIO_CODEC_LC3;
	cap->channels = BT_AUDIO_CHANNEL_COUNTS_1;
	cap->frames = 1;

	if (LC3_Interval == LC3_INTERVAL_7500US) {
		cap->samples = bt_supported_sampling_from_khz(16);
		cap->octets_min = 30;
		cap->octets_max = 90;
		cap->durations = bt_supported_frame_durations_from_ms(7, false);
	} else {
		cap->samples = bt_supported_sampling_from_khz(16) |
				bt_supported_sampling_from_khz(24) |
				bt_supported_sampling_from_khz(32) |
				bt_supported_sampling_from_khz(48);
		cap->octets_min = 40;
		cap->octets_max = 120;
		cap->durations = bt_supported_frame_durations_from_ms(10, false);
	}
	
	
	cap->preferred_contexts = DEFAULT_CONTEXTS;
	caps->source_available_contexts = DEFAULT_CONTEXTS;
	caps->source_supported_contexts = DEFAULT_CONTEXTS;

	caps->source_locations = BT_AUDIO_LOCATIONS_FL;
	printk("source_locations BT_AUDIO_LOCATIONS_FL\n");

	/* Audio Sink */
	cap = &caps->cap[1];
	cap->format = BT_AUDIO_CODEC_LC3;
	
	if (cfg_feature->sp_tws) {
		cap->channels = BT_AUDIO_CHANNEL_COUNTS_1;
		cap->frames = 1;
	} else {
		cap->channels = BT_AUDIO_CHANNEL_COUNTS_2;
		cap->frames = 2;
	}

	if (LC3_Interval == LC3_INTERVAL_7500US) {
		cap->samples = bt_supported_sampling_from_khz(16);
		cap->octets_min = 30;
		cap->octets_max = 90;
		cap->durations = bt_supported_frame_durations_from_ms(7, false);
	} else {
		cap->samples = bt_supported_sampling_from_khz(16) |
				bt_supported_sampling_from_khz(24) |
				bt_supported_sampling_from_khz(32) |
				bt_supported_sampling_from_khz(48);
		cap->octets_min = 40;
		cap->octets_max = 120;
		cap->durations = bt_supported_frame_durations_from_ms(10, false);
	}

	cap->preferred_contexts = DEFAULT_CONTEXTS;


#endif

	return 0;
}
#else

static int leaudio_dongle_cap_init(struct bt_audio_capabilities_2* caps)
{
	struct bt_audio_capability *cap;
	btmgr_feature_cfg_t *cfg_feature = bt_manager_get_feature_config();
	CFG_Struct_LE_AUDIO_Advertising_Mode cfg_le_audio_adv_mode;
	uint8_t LC3_Interval;
	uint8_t RX_Music_2CH_Max_Bitrate;

	app_config_read(CFG_ID_LE_AUDIO_ADVERTISING_MODE, &cfg_le_audio_adv_mode, 0, sizeof(CFG_Struct_LE_AUDIO_Advertising_Mode));
	LC3_Interval = cfg_le_audio_adv_mode.LE_AUDIO_LC3_Interval;
	RX_Music_2CH_Max_Bitrate = cfg_le_audio_adv_mode.LE_AUDIO_LC3_RX_Music_2CH_Bitrate;

	caps->source_cap_num = 1;
	caps->sink_cap_num = 1;

	/* Audio Source */
	cap = &caps->cap[0];
	cap->format = BT_AUDIO_CODEC_LC3;
	cap->channels = BT_AUDIO_CHANNEL_COUNTS_1;
	cap->frames = 1;
	
	if (LC3_Interval == LC3_INTERVAL_7500US) {
		cap->samples = bt_supported_sampling_from_khz(16)|
				bt_supported_sampling_from_khz(32)|
				bt_supported_sampling_from_khz(48);
		cap->octets_min = 30;
		cap->octets_max = 74;
		cap->durations = bt_supported_frame_durations_from_ms(7, false);
	} else {
#if 1 /*FIXME decided by master */
		cap->samples = bt_supported_sampling_from_khz(16)|
				bt_supported_sampling_from_khz(32)|
				bt_supported_sampling_from_khz(48);
		cap->octets_min = 40;
		cap->octets_max = 98;
#endif
#if 0 /*FIXME fit to 48KHz, 78kbps*/
		cap->samples = bt_supported_sampling_from_khz(48);
		cap->octets_min = 98;
		cap->octets_max = 98;
#endif
#if 0 /*FIXME fit to 32KHz, 64kbps*/
		cap->samples = bt_supported_sampling_from_khz(32);
		cap->octets_min = 80;
		cap->octets_max = 80;
#endif
#if 0 /*FIXME fit to 16KHz, 32kbps*/
		cap->samples = bt_supported_sampling_from_khz(16);
		cap->octets_min = 40;
		cap->octets_max = 40;
#endif
		cap->durations = bt_supported_frame_durations_from_ms(10, false);
	}
		
	cap->preferred_contexts = DEFAULT_CONTEXTS;
	caps->source_available_contexts = DEFAULT_CONTEXTS;
	caps->source_supported_contexts = DEFAULT_CONTEXTS;

	caps->source_locations = BT_AUDIO_LOCATIONS_FL;
	printk("source_locations BT_AUDIO_LOCATIONS_FL\n");

	/* Audio Sink */
	cap = &caps->cap[1];
	cap->format = BT_AUDIO_CODEC_LC3;

	if (cfg_feature->sp_tws){ //TWS+LE AUDIO
		cap->channels = BT_AUDIO_CHANNEL_COUNTS_1;
		cap->frames = 1;
	}else{
#ifdef ENABLE_LEAUDIO_MONO
		cap->channels = BT_AUDIO_CHANNEL_COUNTS_1;
		cap->frames = 1;
#else
		cap->channels = BT_AUDIO_CHANNEL_COUNTS_2;
		cap->frames = 2;
#endif
	}
	
	cap->samples = bt_supported_sampling_from_khz(48);

	if (LC3_Interval == LC3_INTERVAL_7500US) {
#ifdef ENABLE_LEAUDIO_MONO
		cap->octets_min = 74;
		cap->octets_max = 74;
#else
		if (RX_Music_2CH_Max_Bitrate == LC3_BITRATE_MUSIC_2CH_192KBPS) {
			cap->octets_min = 90;
			cap->octets_max = 90;
		} else if (RX_Music_2CH_Max_Bitrate == LC3_BITRATE_MUSIC_2CH_184KBPS) {
			cap->octets_min = 86;
			cap->octets_max = 86;
		} else if (RX_Music_2CH_Max_Bitrate == LC3_BITRATE_MUSIC_2CH_176KBPS) {
			cap->octets_min = 83;
			cap->octets_max = 83;
		} else if (RX_Music_2CH_Max_Bitrate == LC3_BITRATE_MUSIC_2CH_160KBPS) {
			cap->octets_min = 75;
			cap->octets_max = 75;
		} else {
			cap->octets_min = 74;
			cap->octets_max = 74;
		}
#endif
		cap->durations = bt_supported_frame_durations_from_ms(7, false);
	} else {
		if (cfg_feature->sp_tws){ //TWS+LE AUDIO
			cap->octets_min = 120;
			cap->octets_max = 120;
		}else{
	#ifdef ENABLE_LEAUDIO_MONO
			cap->octets_min = 98;
			cap->octets_max = 98;
	#else

			if (RX_Music_2CH_Max_Bitrate == LC3_BITRATE_MUSIC_2CH_192KBPS) {
				cap->octets_min = 120;
				cap->octets_max = 120;
			} else if (RX_Music_2CH_Max_Bitrate == LC3_BITRATE_MUSIC_2CH_184KBPS) {
				cap->octets_min = 115;
				cap->octets_max = 115;
			} else if (RX_Music_2CH_Max_Bitrate == LC3_BITRATE_MUSIC_2CH_176KBPS) {
				cap->octets_min = 110;
				cap->octets_max = 110;
			} else if (RX_Music_2CH_Max_Bitrate == LC3_BITRATE_MUSIC_2CH_160KBPS) {
				cap->octets_min = 100;
				cap->octets_max = 100;
			} else {
				cap->octets_min = 99;
				cap->octets_max = 99;
			}

	#endif
		}
		cap->durations = bt_supported_frame_durations_from_ms(10, false);
	}

	cap->preferred_contexts = DEFAULT_CONTEXTS;

	return 0;
}
#endif



static int leaudio_cap_init(void)
{
	struct bt_audio_capabilities_2 caps = {0};
	btmgr_feature_cfg_t *cfg_feature = bt_manager_get_feature_config();
	audio_manager_configs_t* cfg =  audio_channel_get_config();

#ifdef CONFIG_LE_AUDIO_PHONE
	leaudio_phone_cap_init(&caps);
#else
	leaudio_dongle_cap_init(&caps);
#endif

	caps.sink_available_contexts = DEFAULT_CONTEXTS;
	caps.sink_supported_contexts = DEFAULT_CONTEXTS;

	if (cfg_feature->sp_tws){ //TWS+LE AUDIO
		if (cfg->hw_sel_dev_channel == AUDIO_DEVICE_CHANNEL_L ||
			cfg->sw_sel_dev_channel == AUDIO_DEVICE_CHANNEL_L) {
			caps.sink_locations = BT_AUDIO_LOCATIONS_FL; /* | BT_AUDIO_LOCATIONS_FC*/
			printk("sink_locations BT_AUDIO_LOCATIONS_FL\n");
		} else if (cfg->hw_sel_dev_channel == AUDIO_DEVICE_CHANNEL_R ||
			cfg->sw_sel_dev_channel == AUDIO_DEVICE_CHANNEL_R) {
			caps.sink_locations = BT_AUDIO_LOCATIONS_FR; /* | BT_AUDIO_LOCATIONS_FC*/
			printk("sink_locations BT_AUDIO_LOCATIONS_FR\n");
		} else {
			uint8_t rank = 1;
			bt_manager_audio_get_rank_and_size(&rank, NULL);
			if (rank == 1) {
				caps.sink_locations = BT_AUDIO_LOCATIONS_FL;
				printk("sink_locations BT_AUDIO_LOCATIONS_FL\n");
			} else {
				caps.sink_locations = BT_AUDIO_LOCATIONS_FR;
				printk("sink_locations BT_AUDIO_LOCATIONS_FR\n");
			}
		}
	}else{
		#ifdef ENABLE_LEAUDIO_MONO
		caps.sink_locations = BT_AUDIO_LOCATIONS_FC;
		printk("sink_locations BT_AUDIO_LOCATIONS_FC\n");
		#else
		caps.sink_locations = BT_AUDIO_LOCATIONS_FL | BT_AUDIO_LOCATIONS_FR;
		printk("sink_locations BT_AUDIO_LOCATIONS_FL | BT_AUDIO_LOCATIONS_FR\n");
		#endif
	}

	return bt_manager_audio_server_cap_init((struct bt_audio_capabilities *)&caps);
}

static int leaudio_qos_init(void)
{

#ifdef CONFIG_LE_AUDIO_BQB
    struct bt_audio_preferred_qos_default qos;
    uint32_t delay;

    qos.framing = BT_AUDIO_UNFRAMED_SUPPORTED;
    qos.phy = BT_AUDIO_PHY_2M;
    qos.rtn = 3;

	qos.latency = 10;
	delay = 10000;
	delay += 1000;

	qos.delay_min = delay;
	qos.delay_max = delay + 40000;
	qos.preferred_delay_min = delay;
	qos.preferred_delay_max = delay;

    bt_manager_audio_server_qos_init(true, &qos);

    delay = 3200;

	qos.delay_min = delay;
	qos.delay_max = delay + 40000;
	qos.preferred_delay_min = delay;
	qos.preferred_delay_max = delay;

	bt_manager_audio_server_qos_init(false, &qos);

#else
	struct bt_audio_preferred_qos_default qos;
	uint32_t delay;
	btmgr_feature_cfg_t *cfg_feature = bt_manager_get_feature_config();
	CFG_Struct_LE_AUDIO_Advertising_Mode cfg_le_audio_adv_mode;
	uint8_t LC3_Interval;

	app_config_read(CFG_ID_LE_AUDIO_ADVERTISING_MODE, &cfg_le_audio_adv_mode, 0, sizeof(CFG_Struct_LE_AUDIO_Advertising_Mode));
	LC3_Interval = cfg_le_audio_adv_mode.LE_AUDIO_LC3_Interval;

	qos.framing = BT_AUDIO_UNFRAMED_SUPPORTED;
	qos.phy = BT_AUDIO_PHY_2M;
	if (cfg_feature->sp_tws) {
		qos.rtn = 3;
	} else {
		qos.rtn = 4;
	}

	if (LC3_Interval == LC3_INTERVAL_7500US) {
		qos.latency = 8;
		delay = 7500;
	} else {
		qos.latency = 10;
		delay = 10000;
	}
	delay += 1000;

	/* FIXME: only support 1 presentation delay */
	qos.delay_min = delay;
	qos.delay_max = delay;
	qos.preferred_delay_min = delay;
#ifdef CONFIG_LE_AUDIO_PHONE
	qos.preferred_delay_max = 40000;
#else	
	qos.preferred_delay_max = delay;
#endif
	bt_manager_audio_server_qos_init(true, &qos);

	delay = 2000;

	/* FIXME: only support 1 presentation delay */
	qos.delay_min = delay;
	qos.delay_max = delay;
	qos.preferred_delay_min = delay;
#ifdef CONFIG_LE_AUDIO_PHONE
	qos.preferred_delay_max = 40000;
#else	
	qos.preferred_delay_max = delay;
#endif

	bt_manager_audio_server_qos_init(false, &qos);
#endif

	return 0;
}

static int leaudio_le_audio_init(void)
{
	struct bt_audio_role role = {0};
	btmgr_feature_cfg_t *cfg_feature = bt_manager_get_feature_config();
	btmgr_ble_cfg_t *cfg_ble = bt_manager_ble_config();
	
    if (cfg_ble->ble_enable){
		role.num_slave_conn = 2;
    }else{
		role.num_slave_conn = 1;
	}
	role.unicast_server = 1;
#ifdef CONFIG_LE_AUDIO_PHONE
	role.encryption = 1;
#endif
	role.microphone_device = 1;
	role.media_controller = 1;
	role.volume_renderer = 1;
	role.call_terminal = 1;

	if (cfg_feature->sp_tws) {
		role.set_member = 1;

		bt_manager_audio_get_sirk(role.set_sirk, sizeof(role.set_sirk));
		bt_manager_audio_get_rank_and_size(&role.set_rank, &role.set_size);
		role.sirk_enabled = 1;
	}
	
	role.disable_trigger = 1;

#ifdef CONFIG_LE_AUDIO_BQB
	role.num_slave_conn = 1;
    role.num_master_conn = 1;


	role.unicast_server = 1;

	role.encryption = 1;

	role.microphone_device = 1;
	role.media_controller = 1;
	role.volume_renderer = 1;

    role.media_device = 1;

	role.call_terminal = 1;
    role.call_gateway = 1;

	role.set_member = 1;
	role.set_size = 2;
	role.set_rank = 1;
	role.disable_trigger = 1;

#endif

	bt_manager_audio_init(BT_TYPE_LE, &role);

	leaudio_cap_init();
	leaudio_qos_init();

	bt_manager_audio_start(BT_TYPE_LE);

	return 0;
}

static int leaudio_le_audio_exit(void)
{
	bt_manager_audio_stop(BT_TYPE_LE);
	bt_manager_audio_exit(BT_TYPE_LE);
	return 0;
}



static int leaudio_base_init(void)
{
	CFG_Struct_LE_AUDIO_Manager cfg_le_audio_mgr;
	app_config_read(CFG_ID_LE_AUDIO_MANAGER, &cfg_le_audio_mgr, 0, sizeof(CFG_Struct_LE_AUDIO_Manager));
	
	bt_manager_set_leaudio_name(cfg_le_audio_mgr.LE_AUDIO_Device_Name);

#ifdef CONFIG_BT_TRANSCEIVER
    if(sys_get_work_mode())
        tr_leaudio_le_audio_init();
    else
        leaudio_le_audio_init();

#else
	leaudio_le_audio_init();
#endif
	SYS_LOG_INF("base init finished\n");

	return 0;
}


static void leaudio_base_deinit(void)
{
	leaudio_le_audio_exit();
	SYS_LOG_INF("base exit finished\n");
}
#endif



