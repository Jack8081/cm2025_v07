/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief le audio app ctrl
 */

#include "usound.h"
#include "ui_manager.h"
#include "tts_manager.h"
#include "app_ui.h"
#include "system_notify.h"

#ifdef CONFIG_CONSUMER_EQ
#include "consumer_eq.h"
#endif

void usound_event_notify(int event)
{
	sys_manager_event_notify(event, true);
}

static void usound_key_resume(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct usound_app_t *usound = usound_get_app();
	
	ui_key_filter(false);
	usound->need_resume_key = 0;
}

static void usound_key_filter(int duration)
{
	struct usound_app_t *usound = usound_get_app();

	ui_key_filter(true);

	if (thread_timer_is_running(&usound->key_timer)) 
    {
		thread_timer_stop(&usound->key_timer);

		if(usound->need_resume_key)
			ui_key_filter(false);
	}

	usound->need_resume_key = 1;

	thread_timer_init(&usound->key_timer, usound_key_resume, NULL);
	thread_timer_start(&usound->key_timer, duration, 0);
	SYS_LOG_INF("time:%d", duration);
}

static void usound_volume_adjust(int adjust)
{
	struct usound_app_t* usound = usound_get_app();
	int volume;
	int stream_type = AUDIO_STREAM_USOUND;

	if (usound->curr_view_id == BTCALL_VIEW)
		stream_type = AUDIO_STREAM_LE_AUDIO;

	//volume = system_volume_get(stream_type) + adjust;
	//system_volume_set(stream_type, volume, false);

	volume = system_volume_get(stream_type);

	if(volume >= CFG_MAX_BT_MUSIC_VOLUME) {
		usound_event_notify(SYS_EVENT_MAX_VOLUME);
	}
	else {
	    if (volume == 0) {
            usound_event_notify(SYS_EVENT_MIN_VOLUME);
            usb_audio_set_stream_mute(true);
        } else {
            usb_audio_set_stream_mute(false);
        }
	}
}

void usound_switch_mic_mute(void)
{
	struct usound_app_t *usound = usound_get_app();

	if (!usound->capture_player || !usound->capture_player_run) {
		SYS_LOG_INF("capture null\n");
		return;
	}

	usound->mic_mute = !usound->mic_mute;

	audio_system_mute_microphone(usound->mic_mute);
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    if (!usound->mic_mute)
    {
        SYS_LOG_INF("unmute");
        usound_event_notify(SYS_EVENT_MIC_MUTE_ON);
    }
    else
    {
        SYS_LOG_INF("mute");
        usound_event_notify(SYS_EVENT_MIC_MUTE_OFF);
    }
#else
	if (usound->mic_mute)
	{
		SYS_LOG_INF("mute");
		usound_event_notify(SYS_EVENT_MIC_MUTE_ON);
	}
	else
	{
		SYS_LOG_INF("unmute");
		usound_event_notify(SYS_EVENT_MIC_MUTE_OFF);
	}
#endif
}

/* 需要在配置工具中配置每个音效事件对应的语音
 */
uint8_t usound_get_dae_notify(uint8_t dae_notify_index)
{
    static const uint8_t dae_notify_array[] =
    {
        SYS_EVENT_DAE_DEFAULT,    // 默认音效
        SYS_EVENT_DAE_CUSTOM1,    // 自定义音效1
        SYS_EVENT_DAE_CUSTOM2,    // 自定义音效2
        SYS_EVENT_DAE_CUSTOM3,    // 自定义音效3
        SYS_EVENT_DAE_CUSTOM4,    // 自定义音效4
        SYS_EVENT_DAE_CUSTOM5,    // 自定义音效5
        SYS_EVENT_DAE_CUSTOM6,    // 自定义音效6
        SYS_EVENT_DAE_CUSTOM7,    // 自定义音效7
        SYS_EVENT_DAE_CUSTOM8,    // 自定义音效8
        SYS_EVENT_DAE_CUSTOM9,    // 自定义音效9
    };

    if (dae_notify_index >= ARRAY_SIZE(dae_notify_array))
        return SYS_EVENT_NONE;

    return dae_notify_array[dae_notify_index];
}

void usound_update_dae(void)
{
    struct usound_app_t *usound = (struct usound_app_t *)usound_get_app();

    if (!usound->playback_player) {
        return;
    }

#ifdef CONFIG_CONSUMER_EQ
    consumer_eq_patch_param(usound->cfg_dae_data, sizeof(usound->cfg_dae_data));
#endif

	media_player_update_effect_param(usound->playback_player, usound->cfg_dae_data, CFG_SIZE_BT_MUSIC_DAE_AL);
}

void usound_multi_dae_adjust(uint8_t dae_index)
{
    struct usound_app_t *usound = (struct usound_app_t *)usound_get_app();

    uint8_t dae_id = CFG_ID_BT_MUSIC_DAE_AL;

	SYS_LOG_INF("%d\n", dae_index);

    if (dae_index > 0)
    {
        dae_id = CFG_ID_BTMUSIC_MULTIDAE_CUSTOM_AL + dae_index - 1;
        app_config_read(dae_id, &usound->dae_enalbe, CFG_SIZE_BT_MUSIC_DAE_AL, sizeof(cfg_uint8));
    }
    else
    {
        app_config_read
        (
            CFG_ID_BT_MUSIC_DAE,
            &usound->dae_enalbe, offsetof(CFG_Struct_BT_Music_DAE, Enable_DAE), sizeof(cfg_uint8)
        );
    }

    /* 已经播报语音 */
    if (app_config_read(dae_id, usound->cfg_dae_data, 0, CFG_SIZE_BT_MUSIC_DAE_AL) == 0)
    {
        SYS_LOG_ERR("no dae 0x%x\n", dae_id);
        usound->dae_enalbe = 0;
		usound->multidae_enable = 0;
        return;
    }

	usound->current_dae = dae_index;

	if (usound->playback_player)
	{
		media_player_set_effect_enable(usound->playback_player, usound->dae_enalbe);
		usound_update_dae();
	}

	property_set(BTMUSIC_MULTI_DAE, &usound->current_dae, sizeof(uint8_t));
}

static void usound_call_volume_adjust(int adjust)
{
    int volume;
    int cfg_volume_max = CFG_MAX_BT_CALL_VOLUME;

    volume = system_volume_get(AUDIO_STREAM_USOUND) + adjust;
    if (adjust > 0) {
        if (volume < cfg_volume_max) {
            volume += 1;
            system_volume_set(AUDIO_STREAM_USOUND, volume, false);
        }
    } else {
        if (volume > 0) {
            volume -= 1;
            system_volume_set(AUDIO_STREAM_USOUND, volume, false);
        }
    }

	/* incoming ignore max/min volume notify? */

	if(volume >= cfg_volume_max)
	{
		usound_event_notify(SYS_EVENT_MAX_VOLUME);
	}
	else if(volume <= 0)
	{
		usound_event_notify(SYS_EVENT_MIN_VOLUME);
	}
}

static void usound_switch_mic_nr_flag(void)
{
	int nr_flag;

	nr_flag = audio_system_get_microphone_nr_flag();
    nr_flag = !nr_flag; /* swap the noice reduce */
    
    SYS_LOG_INF("%d", nr_flag);
	audio_system_set_microphone_nr_flag(nr_flag);
}

int usound_set_plc_mode(int mode)
{
    int plc_mode = mode;
	struct usound_app_t *usound = usound_get_app();
    if(usound)
        media_player_set_lost_pkt_mode(usound->playback_player, (void*)&plc_mode);
    return 0;
}

static int usound_key_func_proc(int keyfunc)
{
    bool    update_flag = false;
	uint8_t dae_index;
	struct usound_app_t *usound = usound_get_app();

	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE) 
	{
		SYS_LOG_ERR("bug slave can not deal key func\n");
		return -1;
	}

	switch (keyfunc) 
	{
		case KEY_FUNC_PLAY_PAUSE:
			SYS_LOG_INF("pause\n");
			/*FIXME filter media stream*/
		    usb_hid_control_pause_play();
			usound_event_notify(SYS_EVENT_PLAY_PAUSE);
			break;

		case KEY_FUNC_PREV_MUSIC:
		case KEY_FUNC_PREV_MUSIC_IN_PLAYING:
		case KEY_FUNC_PREV_MUSIC_IN_PAUSED:
			SYS_LOG_INF("prev\n");
			/*FIXME filter medua stream*/
		    usb_hid_control_play_prev();
			usound_event_notify(SYS_EVENT_PLAY_PREVIOUS);
			break;

		case KEY_FUNC_NEXT_MUSIC:
		case KEY_FUNC_NEXT_MUSIC_IN_PLAYING:
		case KEY_FUNC_NEXT_MUSIC_IN_PAUSED:
			SYS_LOG_INF("next\n");
		    usb_hid_control_play_next();
			/*FIXME filter media stream*/
			usound_event_notify(SYS_EVENT_PLAY_NEXT);
			break;

		case KEY_FUNC_ADD_MUSIC_VOLUME:
			SYS_LOG_INF("vol up\n");
            if (usound->current_volume_level >= USOUND_VOLUME_LEVEL_MAX) {
                usound->volume_req_level = USOUND_VOLUME_LEVEL_MAX;
            } else if (usound->current_volume_level < USOUND_VOLUME_LEVEL_MAX) {
    		    usound->volume_req_level = usound->current_volume_level + 1;
    		    if (usound->volume_req_level > USOUND_VOLUME_LEVEL_MAX) {
    			    usound->volume_req_level = USOUND_VOLUME_LEVEL_MAX;
    		    }

                update_flag = true;
            } 

            if (update_flag) {
    		    system_volume_set(AUDIO_STREAM_USOUND, usound->volume_req_level, false);
                usound->volume_req_type      = USOUND_VOLUME_INC;
    		    usound->current_volume_level = usound->volume_req_level;
            }

		    usb_hid_control_volume_inc();
			usound_volume_adjust(1);
			break;

		case KEY_FUNC_SUB_MUSIC_VOLUME:
			SYS_LOG_INF("vol down\n");
            if (usound->current_volume_level >= USOUND_VOLUME_LEVEL_MAX) {
                usound->volume_req_level = USOUND_VOLUME_LEVEL_MAX - 1;
                update_flag = true;
            } else if (usound->current_volume_level > USOUND_VOLUME_LEVEL_MIN) {
                usound->volume_req_level = usound->current_volume_level - 1;

                update_flag = true;
		    }

            if (update_flag) {
		        system_volume_set(AUDIO_STREAM_USOUND, usound->volume_req_level, false);
                usound->volume_req_type      = USOUND_VOLUME_DEC;
		        usound->current_volume_level = usound->volume_req_level;
            }

		    usb_hid_control_volume_dec();
			usound_volume_adjust(-1);
			break;

		case KEY_FUNC_SWITCH_MIC_MUTE:
			usound_switch_mic_mute();
			break;

		case KEY_FUNC_DAE_SWITCH:
			if (usound->multidae_enable) {
				dae_index  = (usound->current_dae + 1) % usound->dae_cfg_nums;

				/* 设置音效参数 */
				usound_multi_dae_adjust(dae_index);
			}
			break;

        case KEY_FUNC_SWITCH_NR_FLAG:
            usound_switch_mic_nr_flag();
            break;

		case KEY_FUNC_ACCEPT_CALL:
			SYS_LOG_INF("accept call");
			break;

		case KEY_FUNC_REJECT_CALL:
			SYS_LOG_INF("reject call");
			break;

		case KEY_FUNC_HANGUP_CALL:
			SYS_LOG_INF("hangup call");
			break;

		case KEY_FUNC_KEEP_CALL_RELEASE_3WAY:
			SYS_LOG_INF("keep call release 3way");
			break;

		case KEY_FUNC_HOLD_CALL_ACTIVE_3WAY:
			SYS_LOG_INF("hold call active 3way");
			break;

    	case KEY_FUNC_HANGUP_CALL_ACTIVE_3WAY:
			SYS_LOG_INF("hangup call active 3way");
			break;

		case KEY_FUNC_SWITCH_CALL_OUT:
			SYS_LOG_INF("switch call out");
			break;

		case KEY_FUNC_ADD_CALL_VOLUME:
			SYS_LOG_INF("up");
			usound_call_volume_adjust(1);
			break;

		case KEY_FUNC_SUB_CALL_VOLUME:
			SYS_LOG_INF("down");
			usound_call_volume_adjust(-1);
			break;

		case KEY_FUNC_STOP_VOICE_ASSIST:
			SYS_LOG_INF("stop voice assist");
			break;

        case KEY_FUNC_ACCEPT_INCOMING:
			SYS_LOG_INF("accept incoming");
            if (usb_device_get_switch_os_flag()) {
                /* do nothing for switch device */
            } else {
                if (usound->host_voice_state != VOICE_STATE_NULL) {
                    usb_hid_answer();
                }
            }
			break;

		case KEY_FUNC_REJECT_INCOMING:
			SYS_LOG_INF("reject incoming");
            if (usb_device_get_switch_os_flag()) {
                /* do nothing for switch device */
            } else {
                if (usound->host_voice_state != VOICE_STATE_NULL) {
                    usb_hid_refuse_phone();
                }
            }
			break;
            
        case KEY_FUNC_CALL_MUTE:
            if (!usound->host_hid_mic_mute && usound->host_voice_state != VOICE_STATE_NULL)
            {
                usound->host_hid_mic_mute = 1;
                usb_hid_mute();
            }
            break;
            
        case KEY_FUNC_CALL_UNMUTE:
            if (usound->host_hid_mic_mute && usound->host_voice_state != VOICE_STATE_NULL)
            {
                usound->host_hid_mic_mute = 0;
                usb_hid_mute();
            }
            break;

		default:
			SYS_LOG_ERR("unknow key func %d\n", keyfunc);
			break;
	}

	return 0;
}

void usound_input_event_proc(struct app_msg *msg)
{
	struct usound_app_t *usound = usound_get_app();
	int key_filter_time;

    /* 检查key是否已经屏蔽，防止两边同时按下处理两次按键
     */
    if(usound->need_resume_key)
        return;

    if(msg->cmd == KEY_FUNC_PLAY_PAUSE)
		key_filter_time = 500;
	else
		key_filter_time = 200;

	usound_key_filter(key_filter_time);
	usound_key_func_proc(msg->cmd);
}

