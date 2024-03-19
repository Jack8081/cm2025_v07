/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief le audio app ctrl
 */

#include "leaudio.h"
#include "ui_manager.h"
#include "tts_manager.h"
#include "app_ui.h"
#include "system_notify.h"

#ifdef CONFIG_CONSUMER_EQ
#include "consumer_eq.h"
#endif

#ifdef CONFIG_XEAR_HID_PROTOCOL
#include "xear_hid_protocol.h"
#endif

#ifdef CONFIG_XEAR_HID_PROTOCOL
u8_t rXearSurround=0;
u8_t rXearNR=0;
#endif

static int leaudio_tws_send(int receiver, int event, void* param, int len, bool sync_wait)
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

void leaudio_event_notify(int event)
{
	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE)
	{
		return;
	}

	sys_manager_event_notify(event, true);
}

static void leaudio_key_resume(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();
	
	ui_key_filter(false);
	leaudio->need_resume_key = 0;
    leaudio->key_handled = 0;
}

static void leaudio_key_filter(int duration)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();

	ui_key_filter(true);

	if (thread_timer_is_running(&leaudio->key_timer)) 
    {
		thread_timer_stop(&leaudio->key_timer);

		if(leaudio->need_resume_key)
			ui_key_filter(false);
	}

	leaudio->need_resume_key = 1;

	thread_timer_init(&leaudio->key_timer, leaudio_key_resume, NULL);
	thread_timer_start(&leaudio->key_timer, duration, 0);
	SYS_LOG_INF("time:%d", duration);
}

static void leaudio_volume_adjust(int adjust)
{
	struct leaudio_app_t* leaudio = leaudio_get_app();
	int volume;
	int stream_type = AUDIO_STREAM_LE_AUDIO_MUSIC;

	if (leaudio->curr_view_id == BTCALL_VIEW)
		stream_type = AUDIO_STREAM_LE_AUDIO;

	volume = system_volume_get(stream_type) + adjust;
#ifdef CONFIG_LE_AUDIO_DOUBLE_SOUND_CARD
#ifdef CONFIG_BT_TRANSCEIVER
	tr_system_volume_set(stream_type, volume, false, false);
#endif
#else
	system_volume_set(stream_type, volume, false);
#endif

	volume = system_volume_get(stream_type);

	if(volume >= CFG_MAX_BT_MUSIC_VOLUME)
	{
		leaudio_event_notify(SYS_EVENT_MAX_VOLUME);
	}
	else if(volume <= 0)
	{
		leaudio_event_notify(SYS_EVENT_MIN_VOLUME);
	}
}

void leaudio_switch_mic_mute(void)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();
/*
#if (!defined CONFIG_SAMPLE_CASE_1) && (!defined CONFIG_SAMPLE_CASE_XNT)
	if (!leaudio->capture_player || !leaudio->capture_player_run) {
		SYS_LOG_INF("capture null\n");
		return;
	}
#else
	SYS_LOG_INF("init mic_mute:%d\n",leaudio->mic_mute);
#endif
*/
	SYS_LOG_INF("init mic_mute:%d\n",leaudio->mic_mute);

	leaudio->mic_mute ^= 1;

	audio_system_mute_microphone(leaudio->mic_mute);

	if (!leaudio->mic_mute)
	{
		SYS_LOG_INF("unmute");
		leaudio_event_notify(SYS_EVENT_MIC_MUTE_OFF);
	}
	else
	{
		SYS_LOG_INF("mute");
		leaudio_event_notify(SYS_EVENT_MIC_MUTE_ON);
	}
}

void leaudio_switch_mic_nr_flag(void)
{
	int nr_flag;

	nr_flag = audio_system_get_microphone_nr_flag();
	if (nr_flag <= 0) {
		nr_flag = 1;
	} else {
		nr_flag = 0;
	}

	audio_system_set_microphone_nr_flag(nr_flag);
}

/* 需要在配置工具中配置每个音效事件对应的语音
 */
uint8_t leaudio_get_dae_notify(uint8_t dae_notify_index)
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

void leaudio_multi_dae_tws_notify(uint8_t dae_index)
{
    uint8_t notify_id = SYS_EVENT_NONE;

    /* 可配置所有音效切换用同一个语音 */
	leaudio_event_notify(SYS_EVENT_BT_MUSIC_DAE_SWITCH);

    /* 也可以配置每种音效切换对应一个语音提示 */
    notify_id = leaudio_get_dae_notify(dae_index);

    leaudio_event_notify(notify_id);
}

void leaudio_update_dae(void)
{
    struct leaudio_app_t *leaudio = (struct leaudio_app_t *)leaudio_get_app();

    if (!leaudio->playback_player) {
        return;
    }

#ifdef CONFIG_CONSUMER_EQ
    consumer_eq_patch_param(leaudio->cfg_dae_data, sizeof(leaudio->cfg_dae_data));
#endif

	media_player_update_effect_param(leaudio->playback_player, leaudio->cfg_dae_data, CFG_SIZE_BT_MUSIC_DAE_AL);
}

void leaudio_multi_dae_adjust(uint8_t dae_index)
{
    struct leaudio_app_t *leaudio = (struct leaudio_app_t *)leaudio_get_app();

    uint8_t dae_id = CFG_ID_BT_MUSIC_DAE_AL;

	SYS_LOG_INF("%d\n", dae_index);

    if (dae_index > 0)
    {
        dae_id = CFG_ID_BTMUSIC_MULTIDAE_CUSTOM_AL + dae_index - 1;
        app_config_read(dae_id, &leaudio->dae_enalbe, CFG_SIZE_BT_MUSIC_DAE_AL, sizeof(cfg_uint8));
    }
    else
    {
        app_config_read
        (
            CFG_ID_BT_MUSIC_DAE,
            &leaudio->dae_enalbe, offsetof(CFG_Struct_BT_Music_DAE, Enable_DAE), sizeof(cfg_uint8)
        );
    }

    /* 已经播报语音 */
    if (app_config_read(dae_id, leaudio->cfg_dae_data, 0, CFG_SIZE_BT_MUSIC_DAE_AL) == 0)
    {
        SYS_LOG_ERR("no dae 0x%x\n", dae_id);
        leaudio->dae_enalbe = 0;
		leaudio->multidae_enable = 0;
        return;
    }

	leaudio->current_dae = dae_index;

	if (leaudio->playback_player)
	{
		media_player_set_effect_enable(leaudio->playback_player, leaudio->dae_enalbe);
		leaudio_update_dae();
	}

	property_set(BTMUSIC_MULTI_DAE, &leaudio->current_dae, sizeof(uint8_t));
}

static void leaudio_call_volume_adjust(int adjust)
{
	int volume;

	volume = system_volume_get(AUDIO_STREAM_LE_AUDIO) + adjust;
	system_volume_set(AUDIO_STREAM_LE_AUDIO, volume, false);

	volume = system_volume_get(AUDIO_STREAM_LE_AUDIO);

	/* incoming ignore max/min volume notify? */

	if(volume >= CFG_MAX_BT_CALL_VOLUME)
	{
		leaudio_event_notify(SYS_EVENT_MAX_VOLUME);
	}
	else if(volume <= 0)
	{
		leaudio_event_notify(SYS_EVENT_MIN_VOLUME);
	}
}

int leaudio_set_plc_mode(int mode)
{
    int plc_mode = mode;
	struct leaudio_app_t *leaudio = leaudio_get_app();
    if(leaudio)
        media_player_set_lost_pkt_mode(leaudio->playback_player, (void*)&plc_mode);
    return 0;
}

static int leaudio_key_func_proc(int keyfunc)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();
#ifdef CONFIG_XEAR_HID_PROTOCOL
	u8_t cmd_buf[2];
#endif

#ifdef CONFIG_LE_AUDIO_DOUBLE_SOUND_CARD 
    static int volume = 8;
#endif
	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE) 
	{
		SYS_LOG_ERR("bug slave can not deal key func\n");

		return -1;
	}

	switch (keyfunc) 
	{
#ifdef CONFIG_XEAR_HID_PROTOCOL
		case KEY_FUNC_CUSTOMED_1:

			SYS_LOG_INF("AI NR\n");
			cmd_buf[0] = HID_XEAR_FEATURE_NR;
			cmd_buf[1] = ! rXearNR;
			if( tr_bt_manager_xear_set(cmd_buf[0], cmd_buf[1]) ==  0 )
			{
				rXearNR = ! rXearNR;
				
				if(rXearNR)
				{
					//NR on
					leaudio_event_notify(SYS_EVENT_CUSTOMED_1);
				}
				else
				{
					//NR off
					leaudio_event_notify(SYS_EVENT_CUSTOMED_2);
				}
			}
			else
			{
				SYS_LOG_INF("send XEAR key to TX failed!!\n");
			}

			break;

		case KEY_FUNC_CUSTOMED_2:

			SYS_LOG_INF("SURROUND\n");
			cmd_buf[0] = HID_XEAR_FEATURE_SURROUND;
			cmd_buf[1] = ! rXearSurround;
			if( tr_bt_manager_xear_set(cmd_buf[0], cmd_buf[1]) ==  0 )
			{
				rXearSurround = ! rXearSurround;

				if(rXearSurround)
				{
					//Surround on
					leaudio_event_notify(SYS_EVENT_CUSTOMED_3);
				}
				else
				{
					//Surround off
					leaudio_event_notify(SYS_EVENT_CUSTOMED_4);
				}
			}
			else
			{
				SYS_LOG_INF("send XEAR key to TX failed!!\n");
			}

			break;
#endif
		/* MUSIC VIEW */
		case KEY_FUNC_PLAY_PAUSE:
		{
			if (bt_manager_media_get_status() == BT_STATUS_PLAYING) {
				SYS_LOG_INF("pause\n");
				bt_manager_media_pause();
				/*FIXME filter media stream*/
#if (!defined CONFIG_SAMPLE_CASE_1) && (!defined CONFIG_SAMPLE_CASE_XNT)
				leaudio_event_notify(SYS_EVENT_PLAY_PAUSE);
#endif
			} else {
				SYS_LOG_INF("play\n");
				bt_manager_media_play();
#if (!defined CONFIG_SAMPLE_CASE_1) && (!defined CONFIG_SAMPLE_CASE_XNT)
				leaudio_event_notify(SYS_EVENT_PLAY_START);
#endif
				/*FIXME filter/unfilter media stream*/
			}
			leaudio_view_display();
			break;
		}

		case KEY_FUNC_PREV_MUSIC:
		case KEY_FUNC_PREV_MUSIC_IN_PLAYING:
		case KEY_FUNC_PREV_MUSIC_IN_PAUSED:
		{
			SYS_LOG_INF("prev");
			bt_manager_media_play_previous();
			/*FIXME filter media stream*/
			leaudio_event_notify(SYS_EVENT_PLAY_PREVIOUS);
			break;
		}

		case KEY_FUNC_NEXT_MUSIC:
		case KEY_FUNC_NEXT_MUSIC_IN_PLAYING:
		case KEY_FUNC_NEXT_MUSIC_IN_PAUSED:
		{
			SYS_LOG_INF("next");
			bt_manager_media_play_next();
			/*FIXME filter media stream*/
			leaudio_event_notify(SYS_EVENT_PLAY_NEXT);
			break;
		}

		case KEY_FUNC_ADD_MUSIC_VOLUME:
		{
			SYS_LOG_INF("up");
			leaudio_volume_adjust(1);
			break;
		}

		case KEY_FUNC_SUB_MUSIC_VOLUME:
		{
			SYS_LOG_INF("down");
			leaudio_volume_adjust(-1);
			break;
		}

		case KEY_FUNC_SWITCH_MIC_MUTE:
		{
			SYS_LOG_INF("mute");
			leaudio_switch_mic_mute();
			break;
		}

		case KEY_FUNC_SWITCH_NR_FLAG:
		{
			leaudio_switch_mic_nr_flag();
			break;
		}

		case KEY_FUNC_DAE_SWITCH:
		{
			uint8_t dae_index;
            SYS_LOG_INF("DAE enter -- multidae:%d\n",leaudio->multidae_enable);

			if (leaudio->multidae_enable == 0)
				break;

			dae_index  = leaudio->current_dae + 1;
			dae_index %= leaudio->dae_cfg_nums;

			/* tws 通知另一方播报语音 */
			//leaudio_multi_dae_tws_notify(dae_index);

			/* tws 通知另一方切换音效 */
			//leaudio_tws_send(BTSRV_TWS_SLAVE, TWS_EVENT_SYNC_BT_MUSIC_DAE, &dae_index, sizeof(dae_index), false);

			/* 设置音效参数 */
			leaudio_multi_dae_adjust(dae_index);

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
			leaudio_call_volume_adjust(1);
			break;
		}

		case KEY_FUNC_SUB_CALL_VOLUME:
		{
			SYS_LOG_INF("down");
			leaudio_call_volume_adjust(-1);
			break;
		}

		case KEY_FUNC_STOP_VOICE_ASSIST:
		{
			SYS_LOG_INF("stop voice assist");
			break;
		}

		case KEY_FUNC_ACCEPT_INCOMING:
		{
			SYS_LOG_INF("accept incoming");
            tr_bt_manager_call_accept(NULL);
			break;
		}

		case KEY_FUNC_REJECT_INCOMING:
		{
			SYS_LOG_INF("reject incoming");
            tr_bt_manager_call_terminate(NULL, 0);
			break;
		}

		case KEY_FUNC_CALL_MUTE:
		{
			SYS_LOG_INF("call mute");
            bt_manager_mic_mute();
			break;
		}

		case KEY_FUNC_CALL_UNMUTE:
		{
			SYS_LOG_INF("call unmute");
            bt_manager_mic_unmute();
			break;
		}
#ifdef CONFIG_LE_AUDIO_DOUBLE_SOUND_CARD
        case KEY_FUNC_DOUBLE_SOUNDCARD_VOL_ADD:
		{
			SYS_LOG_INF("double soundcard vol add");
	        volume += 1;
            if(volume >= 16)
                volume = 16;

            bt_manager_volume_set(volume);
			break;
		}

        case KEY_FUNC_DOUBLE_SOUNDCARD_VOL_SUB:
		{
			SYS_LOG_INF("double soundcard vol sub");
	        volume -= 1;
            if(volume <= 0)
                volume = 0;

            bt_manager_volume_set(volume);
			break;
		}

        case KEY_FUNC_CUSTOMED_3:
        {
            SYS_LOG_INF("double soundcard vol reset");
            volume = 8;

            bt_manager_volume_set(volume);
            break;
        }
#endif
		default:
		{
			SYS_LOG_ERR("unknow key func %d\n", keyfunc);
			break;
		}
	}

    leaudio->key_handled = 1;

	return 0;
}

void leaudio_input_event_proc(struct app_msg *msg)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();
	int key_filter_time;

    /* 检查key是否已经屏蔽，防止两边同时按下处理两次按键
     */
    if(leaudio->need_resume_key)
        return;

    if(msg->cmd == KEY_FUNC_PLAY_PAUSE)
		key_filter_time = 500;
#ifdef CONFIG_LE_AUDIO_DOUBLE_SOUND_CARD
	else if( (msg->cmd == KEY_FUNC_DOUBLE_SOUNDCARD_VOL_ADD) ||
			(msg->cmd == KEY_FUNC_DOUBLE_SOUNDCARD_VOL_SUB) )
	{
		// for rotary encoder to response quickly
		key_filter_time = 50;
	}
	//else if((msg->cmd == KEY_FUNC_CUSTOMED_1) || (msg->cmd == KEY_FUNC_CUSTOMED_2))
	//{
	//	key_filter_time = 1500;
	//}
#endif
	else
		key_filter_time = 200;

    leaudio_tws_send(BTSRV_TWS_NONE, TWS_EVENT_REMOTE_KEY_PROC_BEGIN, &msg->cmd, 1, false);

	leaudio_key_filter(key_filter_time);

	if (leaudio_tws_send(BTSRV_TWS_MASTER, TWS_EVENT_BT_MUSIC_KEY_CTRL, &msg->cmd, 1, false) == 0)
		return;

	leaudio_key_func_proc(msg->cmd);
}

void leaudio_tws_event_proc(struct app_msg *msg)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();
    tws_message_t* message = (tws_message_t*)msg->ptr;

	SYS_LOG_INF("tws cmd id %d %d", message->cmd_id, message->cmd_len);
	
	switch (message->cmd_id)
    {
        case TWS_EVENT_BT_MUSIC_KEY_CTRL:
		{
            /* 检查key是否已经处理，防止两边同时按下处理两次按键
             */
            if(!leaudio->key_handled)
                leaudio_key_func_proc(message->cmd_data[0]);
			break;
		}

        case TWS_EVENT_REMOTE_KEY_PROC_BEGIN:
		{
			int key_filter_time;
			if(message->cmd_data[0] == KEY_FUNC_PLAY_PAUSE)
				key_filter_time = 500;
			else
				key_filter_time = 200;
			leaudio_key_filter(key_filter_time);
			break;
		}

    }
}

