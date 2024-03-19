/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief le audio app ctrl
 */

#include "tr_usound.h"
#include "ui_manager.h"
#include "tts_manager.h"
#include "app_ui.h"
#include "system_notify.h"

#ifdef CONFIG_CONSUMER_EQ
#include "consumer_eq.h"
#endif

static int tr_usound_tws_send(int receiver, int event, void* param, int len, bool sync_wait)
{
	int sender = bt_manager_tws_get_dev_role();
	
	if ((sender != BTSRV_TWS_NONE) && (sender != receiver))
	{
		if (sync_wait)
			bt_manager_tws_send_sync_message(TWS_USER_APP_EVENT, event, param, len, 100);
		else	
			bt_manager_tws_send_message(TWS_USER_APP_EVENT, event, param, len);

		return 0;
	}
	
	return -1;
}

void tr_usound_event_notify(int event)
{
	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE)
	{
		return;
	}

	sys_manager_event_notify(event, true);
}

static void tr_usound_key_resume(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();
	
	ui_key_filter(false);
	tr_usound->need_resume_key = 0;
    tr_usound->key_handled = 0;
}

static void tr_usound_key_filter(int duration)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();

	ui_key_filter(true);

	if (thread_timer_is_running(&tr_usound->key_timer)) 
    {
		thread_timer_stop(&tr_usound->key_timer);

		if(tr_usound->need_resume_key)
			ui_key_filter(false);
	}

	tr_usound->need_resume_key = 1;

	thread_timer_init(&tr_usound->key_timer, tr_usound_key_resume, NULL);
	thread_timer_start(&tr_usound->key_timer, duration, 0);
	SYS_LOG_INF("time:%d", duration);
}

static void tr_usound_volume_adjust(int adjust)
{
	struct tr_usound_app_t* tr_usound = tr_usound_get_app();
	int volume;
	int stream_type = AUDIO_STREAM_TR_USOUND;

	if (tr_usound->curr_view_id == BTCALL_VIEW)
		stream_type = AUDIO_STREAM_LE_AUDIO;

	volume = system_volume_get(stream_type) + adjust;
	system_volume_set(stream_type, volume, false);

	volume = system_volume_get(stream_type);

	if(volume >= CFG_MAX_BT_MUSIC_VOLUME)
	{
		tr_usound_event_notify(SYS_EVENT_MAX_VOLUME);
	}
	else if(volume <= 0)
	{
		tr_usound_event_notify(SYS_EVENT_MIN_VOLUME);
	}
}

void tr_usound_switch_mic_mute(void)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();

	if (!tr_usound->capture_player || !tr_usound->capture_player_run) {
		SYS_LOG_INF("capture null\n");
		return;
	}

	tr_usound->mic_mute ^= 1;

	audio_system_mute_microphone(tr_usound->mic_mute);

	if (tr_usound->mic_mute)
	{
		SYS_LOG_INF("mute");
		tr_usound_event_notify(SYS_EVENT_MIC_MUTE_ON);
	}
	else
	{
		SYS_LOG_INF("unmute");
		tr_usound_event_notify(SYS_EVENT_MIC_MUTE_OFF);
	}
}

/* 需要在配置工具中配置每个音效事件对应的语音
 */
uint8_t tr_usound_get_dae_notify(uint8_t dae_notify_index)
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

void tr_usound_multi_dae_tws_notify(uint8_t dae_index)
{
    uint8_t notify_id = SYS_EVENT_NONE;

    /* 可配置所有音效切换用同一个语音 */
	tr_usound_event_notify(SYS_EVENT_BT_MUSIC_DAE_SWITCH);

    /* 也可以配置每种音效切换对应一个语音提示 */
    notify_id = tr_usound_get_dae_notify(dae_index);

    tr_usound_event_notify(notify_id);
}

void tr_usound_update_dae(void)
{
    struct tr_usound_app_t *tr_usound = (struct tr_usound_app_t *)tr_usound_get_app();

    if (!tr_usound->playback_player) {
        return;
    }

#ifdef CONFIG_CONSUMER_EQ
    consumer_eq_patch_param(tr_usound->cfg_dae_data, sizeof(tr_usound->cfg_dae_data));
#endif

	media_player_update_effect_param(tr_usound->playback_player, tr_usound->cfg_dae_data, CFG_SIZE_BT_MUSIC_DAE_AL);
}

void tr_usound_multi_dae_adjust(uint8_t dae_index)
{
    struct tr_usound_app_t *tr_usound = (struct tr_usound_app_t *)tr_usound_get_app();

    uint8_t dae_id = CFG_ID_BT_MUSIC_DAE_AL;

	SYS_LOG_INF("%d\n", dae_index);

    if (dae_index > 0)
    {
        dae_id = CFG_ID_BTMUSIC_MULTIDAE_CUSTOM_AL + dae_index - 1;
        app_config_read(dae_id, &tr_usound->dae_enalbe, CFG_SIZE_BT_MUSIC_DAE_AL, sizeof(cfg_uint8));
    }
    else
    {
        app_config_read
        (
            CFG_ID_BT_MUSIC_DAE,
            &tr_usound->dae_enalbe, offsetof(CFG_Struct_BT_Music_DAE, Enable_DAE), sizeof(cfg_uint8)
        );
    }

    /* 已经播报语音 */
    if (app_config_read(dae_id, tr_usound->cfg_dae_data, 0, CFG_SIZE_BT_MUSIC_DAE_AL) == 0)
    {
        SYS_LOG_ERR("no dae 0x%x\n", dae_id);
        tr_usound->dae_enalbe = 0;
		tr_usound->multidae_enable = 0;
        return;
    }

	tr_usound->current_dae = dae_index;

	if (tr_usound->playback_player)
	{
		media_player_set_effect_enable(tr_usound->playback_player, tr_usound->dae_enalbe);
		tr_usound_update_dae();
	}

	property_set(BTMUSIC_MULTI_DAE, &tr_usound->current_dae, sizeof(uint8_t));
}

static void tr_usound_call_volume_adjust(int adjust)
{
	int volume;

	volume = system_volume_get(AUDIO_STREAM_LE_AUDIO) + adjust;
	system_volume_set(AUDIO_STREAM_LE_AUDIO, volume, false);

	volume = system_volume_get(AUDIO_STREAM_LE_AUDIO);

	/* incoming ignore max/min volume notify? */

	if(volume >= CFG_MAX_BT_CALL_VOLUME)
	{
		tr_usound_event_notify(SYS_EVENT_MAX_VOLUME);
	}
	else if(volume <= 0)
	{
		tr_usound_event_notify(SYS_EVENT_MIN_VOLUME);
	}
}

int tr_usound_set_plc_mode(int mode)
{
    int plc_mode = mode;
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();
    if(tr_usound)
        media_player_set_lost_pkt_mode(tr_usound->playback_player, (void*)&plc_mode);
    return 0;
}

static int tr_usound_key_func_proc(int keyfunc)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();

	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE) 
	{
		SYS_LOG_ERR("bug slave can not deal key func\n");

		return -1;
	}

	switch (keyfunc) 
	{
		/* MUSIC VIEW */

		case KEY_FUNC_PLAY_PAUSE:
		{
			SYS_LOG_INF("pause\n");
			/*FIXME filter media stream*/
		    usb_hid_control_pause_play();
			tr_usound_event_notify(SYS_EVENT_PLAY_PAUSE);
			break;
		}

		case KEY_FUNC_PREV_MUSIC:
		case KEY_FUNC_PREV_MUSIC_IN_PLAYING:
		case KEY_FUNC_PREV_MUSIC_IN_PAUSED:
		{
			SYS_LOG_INF("prev\n");
			/*FIXME filter medua stream*/
		    usb_hid_control_play_prev();
			tr_usound_event_notify(SYS_EVENT_PLAY_PREVIOUS);
			break;
		}

		case KEY_FUNC_NEXT_MUSIC:
		case KEY_FUNC_NEXT_MUSIC_IN_PLAYING:
		case KEY_FUNC_NEXT_MUSIC_IN_PAUSED:
		{
			SYS_LOG_INF("next\n");
		    usb_hid_control_play_next();
			/*FIXME filter media stream*/
			tr_usound_event_notify(SYS_EVENT_PLAY_NEXT);
			break;
		}

		case KEY_FUNC_ADD_MUSIC_VOLUME:
		{
			SYS_LOG_INF("vol up\n");
		    tr_usound->volume_req_type = USOUND_VOLUME_INC;
		    tr_usound->volume_req_level = tr_usound->current_volume_level + 1;
		    if (tr_usound->volume_req_level > 16) {
			    tr_usound->volume_req_level = 16;
		    }
		    system_volume_set(AUDIO_STREAM_TR_USOUND, tr_usound->volume_req_level, false);
		    usb_hid_control_volume_inc();
		    tr_usound->current_volume_level = tr_usound->volume_req_level;
			tr_usound_volume_adjust(1);
			break;
		}

		case KEY_FUNC_SUB_MUSIC_VOLUME:
		{
			SYS_LOG_INF("vol down\n");
            tr_usound->volume_req_type = USOUND_VOLUME_DEC;
		    tr_usound->volume_req_level = tr_usound->current_volume_level - 1;
		    if (tr_usound->volume_req_level < 0) {
			    tr_usound->volume_req_level = 0;
		    }

		    system_volume_set(AUDIO_STREAM_TR_USOUND, tr_usound->volume_req_level, false);
		    usb_hid_control_volume_dec();
		    tr_usound->current_volume_level = tr_usound->volume_req_level;
			tr_usound_volume_adjust(-1);
			break;
		}

		case KEY_FUNC_SWITCH_MIC_MUTE:
		{
			tr_usound_switch_mic_mute();
			break;
		}

		case KEY_FUNC_DAE_SWITCH:
		{
			uint8_t dae_index;

			if (tr_usound->multidae_enable == 0)
				break;

			dae_index  = tr_usound->current_dae + 1;
			dae_index %= tr_usound->dae_cfg_nums;

			/* tws 通知另一方播报语音 */
			tr_usound_multi_dae_tws_notify(dae_index);

			/* tws 通知另一方切换音效 */
			tr_usound_tws_send(BTSRV_TWS_SLAVE, TWS_EVENT_SYNC_BT_MUSIC_DAE, &dae_index, sizeof(dae_index), false);

			/* 设置音效参数 */
			tr_usound_multi_dae_adjust(dae_index);

			break;
		}

        case KEY_FUNC_DONGLE_ENTER_SEARCH:
        {
            SYS_LOG_INF("enter search mode!\n");
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
            ui_event_led_display(UI_EVENT_CUSTOMED_3);
            tr_bt_manager_audio_scan_mode(2);
#else
            tr_bt_manager_audio_scan_mode(3);
#endif
            break;
        }


		/* CALL VIEW */

		case KEY_FUNC_ACCEPT_CALL:
		{
			SYS_LOG_INF("accept call");
			bt_manager_call_accept(NULL);
			break;
		}

		case KEY_FUNC_REJECT_CALL:
		{
			SYS_LOG_INF("reject call");
			bt_manager_call_terminate(NULL, 0);
			break;
		}

		case KEY_FUNC_HANGUP_CALL:
		{
			SYS_LOG_INF("hangup call");
			bt_manager_call_terminate(NULL, 0);
			break;
		}

		case KEY_FUNC_KEEP_CALL_RELEASE_3WAY:
		{
			SYS_LOG_INF("keep call release 3way");
			bt_manager_call_hangup_another_call();
			break;
		}

		case KEY_FUNC_HOLD_CALL_ACTIVE_3WAY:
		{
			SYS_LOG_INF("hold call active 3way");
			bt_manager_call_holdcur_answer_call();
			break;
		}

    	case KEY_FUNC_HANGUP_CALL_ACTIVE_3WAY:
		{
			SYS_LOG_INF("hangup call active 3way");
			bt_manager_call_hangupcur_answer_call();
			break;
		}

		case KEY_FUNC_SWITCH_CALL_OUT:
		{
			SYS_LOG_INF("switch call out");
			break;
		}

		case KEY_FUNC_ADD_CALL_VOLUME:
		{
			SYS_LOG_INF("up");
			tr_usound_call_volume_adjust(1);
			break;
		}

		case KEY_FUNC_SUB_CALL_VOLUME:
		{
			SYS_LOG_INF("down");
			tr_usound_call_volume_adjust(-1);
			break;
		}

		case KEY_FUNC_STOP_VOICE_ASSIST:
		{
			SYS_LOG_INF("stop voice assist");
			break;
		}

		default:
		{
			SYS_LOG_ERR("unknow key func %d\n", keyfunc);
			break;
		}
	}

    tr_usound->key_handled = 1;

	return 0;
}

void tr_usound_input_event_proc(struct app_msg *msg)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();
	int key_filter_time;

    /* 检查key是否已经屏蔽，防止两边同时按下处理两次按键
     */
    if(tr_usound->need_resume_key)
        return;

    if(msg->cmd == KEY_FUNC_PLAY_PAUSE)
		key_filter_time = 500;
	else
		key_filter_time = 200;

    tr_usound_tws_send(BTSRV_TWS_NONE, TWS_EVENT_REMOTE_KEY_PROC_BEGIN, &msg->cmd, 1, false);

	tr_usound_key_filter(key_filter_time);

	if (tr_usound_tws_send(BTSRV_TWS_MASTER, TWS_EVENT_BT_MUSIC_KEY_CTRL, &msg->cmd, 1, false) == 0)
		return;

	tr_usound_key_func_proc(msg->cmd);
}

void tr_usound_tws_event_proc(struct app_msg *msg)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();
    tws_message_t* message = (tws_message_t*)msg->ptr;

	SYS_LOG_INF("tws cmd id %d %d", message->cmd_id, message->cmd_len);
	
	switch (message->cmd_id)
    {
        case TWS_EVENT_BT_MUSIC_KEY_CTRL:
		{
            /* 检查key是否已经处理，防止两边同时按下处理两次按键
             */
            if(!tr_usound->key_handled)
                tr_usound_key_func_proc(message->cmd_data[0]);
			break;
		}

        case TWS_EVENT_REMOTE_KEY_PROC_BEGIN:
		{
			int key_filter_time;
			if(message->cmd_data[0] == KEY_FUNC_PLAY_PAUSE)
				key_filter_time = 500;
			else
				key_filter_time = 200;
			tr_usound_key_filter(key_filter_time);
			break;
		}

    }
}

