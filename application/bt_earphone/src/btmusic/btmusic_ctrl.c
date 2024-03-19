/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt music ctrl
 */
#include "btmusic.h"
#include "ui_manager.h"
#include "tts_manager.h"
#include "app_ui.h"
#include "system_notify.h"
#include <user_comm/ap_status.h>

#ifdef CONFIG_CONSUMER_EQ
#include "consumer_eq.h"
#endif

extern int64_t btcall_exit_time;

int btmusic_tws_send(int receiver, int event, void* param, int len, bool sync_wait)
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

void btmusic_event_notify(int event)
{
	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE)
	{
		return;
	}

	sys_manager_event_notify(event, true);
}


void btmusic_ctrl_avdtp_filter(bool filter)
{
    struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();
    char avdtp_filter[2];
	
    if (filter)
    {
        if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER)
        {
            avdtp_filter[0] = filter;
            avdtp_filter[1] = btmusic->avdtp_resume;	            
			btmusic_tws_send(BTSRV_TWS_SLAVE, TWS_EVENT_AVDTP_FILTER, avdtp_filter, sizeof(avdtp_filter), true);
        }
        bt_music_stop_play();
        btmusic->avdtp_filter = 1;
    }
    else
    {
        btmusic->avdtp_filter = 0;
        if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER)
        {
            avdtp_filter[0] = filter;
            avdtp_filter[1] = btmusic->avdtp_resume;	            
			btmusic_tws_send(BTSRV_TWS_SLAVE, TWS_EVENT_AVDTP_FILTER, avdtp_filter, sizeof(avdtp_filter), true);            
        }
    }
}

int btmusic_ctrl_avdtp_is_filter(void)
{
    struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();
	return (btmusic->avdtp_filter >0)?1:0;
}



void bt_music_tws_avdtp_filter(bool filter)
{
	struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();
	
    if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER)
    {
        if (!btmusic_ctrl_avdtp_is_filter() && btmusic->avdtp_resume)
        {
            bt_manager_audio_stream_restore(BT_TYPE_BR);
		    btmusic->avdtp_resume = 0;
        }
        return;
    }

    SYS_LOG_INF("%d", filter);

    if (filter)
    {
        btmusic->tws_avdtp_filter = 1;
    }
    else
    {
        btmusic->tws_avdtp_filter = 0;
    }

    btmusic_ctrl_avdtp_filter(filter);
}


/* TWS 取消数据过滤
 */
void bt_music_tws_cancel_filter(void)
{
    struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();

    while (btmusic->tws_avdtp_filter > 0)
    {
        btmusic->tws_avdtp_filter -= 1;
        btmusic_ctrl_avdtp_filter(false);
    }
	SYS_LOG_INF("filter:%d, avdtp_resume: %d", btmusic_ctrl_avdtp_is_filter(), btmusic->avdtp_resume);	
	if (!btmusic_ctrl_avdtp_is_filter() && btmusic->avdtp_resume)
	{
		bt_manager_audio_stream_restore(BT_TYPE_BR);
		btmusic->avdtp_resume = 0;
	}
}

static void btmusic_tts_delay_resume(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();

	btmusic_ctrl_avdtp_filter(false);

	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_NONE)
	{
		if (!btmusic_ctrl_avdtp_is_filter() && btmusic->avdtp_resume)
		{
			bt_manager_audio_stream_restore(BT_TYPE_BR);
			btmusic->avdtp_resume = 0;
		}
	}
	SYS_LOG_INF("filter:%d, avdtp_resume: %d", btmusic_ctrl_avdtp_is_filter(), btmusic->avdtp_resume);	
}

static void btmusic_setup_delay_resume_timer(int duration)
{
	struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();
	
	if (thread_timer_is_running(&btmusic->monitor_timer))
	{
		thread_timer_stop(&btmusic->monitor_timer);
	}

	thread_timer_init(&btmusic->monitor_timer, btmusic_tts_delay_resume, NULL);
	thread_timer_start(&btmusic->monitor_timer, duration, 0);
}


static void btmusic_stop_hold(int stop_hold_ms, int need_resume)
{
    struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();
    if (stop_hold_ms > 0)
    {
		btmusic->avdtp_resume = need_resume?1:0;
        btmusic_ctrl_avdtp_filter(true);
		btmusic_setup_delay_resume_timer(stop_hold_ms);
        SYS_LOG_INF("stop_hold_ms %d", stop_hold_ms);	
    }
}


static void btmusic_key_resume(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();
	
	ui_key_filter(false);
	btmusic->need_resume_key = 0;
    btmusic->key_handled = 0;
}

static void btmusic_key_filter(int duration)
{
	struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();

	ui_key_filter(true);

	if (thread_timer_is_running(&btmusic->key_timer)) 
    {
		thread_timer_stop(&btmusic->key_timer);

		if(btmusic->need_resume_key)
			ui_key_filter(false);
	}

	btmusic->need_resume_key = 1;

	thread_timer_init(&btmusic->key_timer, btmusic_key_resume, NULL);
	thread_timer_start(&btmusic->key_timer, duration, 0);
	SYS_LOG_INF("time:%d", duration);
}

static void btmusic_volume_adjust(int adjust)
{
	int volume;

	volume = system_volume_get(AUDIO_STREAM_MUSIC) + adjust;
	system_volume_set(AUDIO_STREAM_MUSIC, volume, false);

	volume = system_volume_get(AUDIO_STREAM_MUSIC);

	if(volume >= CFG_MAX_BT_MUSIC_VOLUME)
	{
		btmusic_event_notify(SYS_EVENT_MAX_VOLUME);
	}
	else if(volume <= 0)
	{
		btmusic_event_notify(SYS_EVENT_MIN_VOLUME);
	}

}

/* 需要在配置工具中配置每个音效事件对应的语音
 */
u8_t btmusic_get_dae_notify(u8_t dae_notify_index)
{
    static const u8_t dae_notify_array[] =
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

void btmusic_multi_dae_tws_notify(u8_t dae_index)
{
    u8_t notify_id = SYS_EVENT_NONE;

    /* 可配置所有音效切换用同一个语音 */
	btmusic_event_notify(SYS_EVENT_BT_MUSIC_DAE_SWITCH);

    /* 也可以配置每种音效切换对应一个语音提示 */
    notify_id = btmusic_get_dae_notify(dae_index);

    btmusic_event_notify(notify_id);
}

void btmusic_update_dae(void)
{
    struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();

    if (!btmusic->player) {
        return;
    }

#ifdef CONFIG_CONSUMER_EQ
    consumer_eq_patch_param(btmusic->cfg_dae_data, sizeof(btmusic->cfg_dae_data));
#endif

	media_player_update_effect_param(btmusic->player, btmusic->cfg_dae_data, CFG_SIZE_BT_MUSIC_DAE_AL);
}

void btmusic_multi_dae_adjust(u8_t dae_index)
{
	struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();

    u8_t dae_id = CFG_ID_BT_MUSIC_DAE_AL;

	SYS_LOG_INF("%d\n", dae_index);

    if (dae_index > 0)
    {
        dae_id = CFG_ID_BTMUSIC_MULTIDAE_CUSTOM_AL + dae_index - 1;
        app_config_read(dae_id, &btmusic->dae_enalbe, CFG_SIZE_BT_MUSIC_DAE_AL, sizeof(cfg_uint8));
    }
    else
    {
        app_config_read
        (
            CFG_ID_BT_MUSIC_DAE,
            &btmusic->dae_enalbe, offsetof(CFG_Struct_BT_Music_DAE, Enable_DAE), sizeof(cfg_uint8)
        );
    }

    /* 已经播报语音 */
    if (app_config_read(dae_id, btmusic->cfg_dae_data, 0, CFG_SIZE_BT_MUSIC_DAE_AL) == 0)
    {
        SYS_LOG_ERR("no dae 0x%x\n", dae_id);
        btmusic->dae_enalbe = 0;
		btmusic->multidae_enable = 0;
        return;
    }

	btmusic->current_dae = dae_index;

	if (btmusic->player)
	{
		media_player_set_effect_enable(btmusic->player, btmusic->dae_enalbe);
		btmusic_update_dae();
	}

	property_set(BTMUSIC_MULTI_DAE, &btmusic->current_dae, sizeof(u8_t));
}

static int btmusic_key_func_proc(int keyfunc)
{
	struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();

	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE) 
	{
		SYS_LOG_ERR("bug slave can not deal key func\n");

		return -1;
	}

	switch (keyfunc) 
	{
		case KEY_FUNC_PLAY_PAUSE:
		{
			if (bt_manager_a2dp_get_status() == BT_STATUS_PLAYING)
			{
				bt_manager_media_pause();
				if (btmusic->cfg_bt_music_stop_hold.Key_Pause_Stop_Hold_Ms > 0){
					btmusic_stop_hold(btmusic->cfg_bt_music_stop_hold.Key_Pause_Stop_Hold_Ms, 1);
				}

				btmusic_event_notify(SYS_EVENT_PLAY_PAUSE);
			}
			else
			{
				bt_manager_media_play();
				btmusic_event_notify(SYS_EVENT_PLAY_START);

				if (btmusic->cfg_bt_music_stop_hold.Key_Pause_Stop_Hold_Ms > 0){
					btmusic_stop_hold(btmusic->cfg_bt_music_stop_hold.Key_Pause_Stop_Hold_Ms, 1);
				}
				bt_manager_audio_stream_restore(BT_TYPE_BR);
			}
			
			btmusic_view_display();
			break;
		}

		case KEY_FUNC_PREV_MUSIC:
		case KEY_FUNC_PREV_MUSIC_IN_PLAYING:
		case KEY_FUNC_PREV_MUSIC_IN_PAUSED:
		{
			bt_manager_media_play_previous();
			if (btmusic->cfg_bt_music_stop_hold.Key_Prev_Next_Hold_Ms > 0){
				btmusic_stop_hold(btmusic->cfg_bt_music_stop_hold.Key_Prev_Next_Hold_Ms,1);
			}
			btmusic_event_notify(SYS_EVENT_PLAY_PREVIOUS);
			break;
		}

		case KEY_FUNC_NEXT_MUSIC:
		case KEY_FUNC_NEXT_MUSIC_IN_PLAYING:
		case KEY_FUNC_NEXT_MUSIC_IN_PAUSED:
		{
			bt_manager_media_play_next();
			if (btmusic->cfg_bt_music_stop_hold.Key_Prev_Next_Hold_Ms > 0){
				btmusic_stop_hold(btmusic->cfg_bt_music_stop_hold.Key_Prev_Next_Hold_Ms, 1);
			}

			btmusic_event_notify(SYS_EVENT_PLAY_NEXT);
			break;
		}

		case KEY_FUNC_ADD_MUSIC_VOLUME:
		case KEY_FUNC_ADD_MUSIC_VOLUME_IN_LINKED:
		{
			btmusic_volume_adjust(1);
			break;
		}

		case KEY_FUNC_SUB_MUSIC_VOLUME:
		case KEY_FUNC_SUB_MUSIC_VOLUME_IN_LINKED:
		{
			btmusic_volume_adjust(-1);
			break;
		}

		case KEY_FUNC_DAE_SWITCH:
		{
			u8_t dae_index;

			if (btmusic->multidae_enable == 0)
				break;

			dae_index  = btmusic->current_dae + 1;
			dae_index %= btmusic->dae_cfg_nums;

			/* tws 通知另一方播报语音 */
			btmusic_multi_dae_tws_notify(dae_index);

			/* tws 通知另一方切换音效 */
			btmusic_tws_send(BTSRV_TWS_SLAVE, TWS_EVENT_SYNC_BT_MUSIC_DAE, &dae_index, sizeof(dae_index), false);

			/* 设置音效参数 */
			btmusic_multi_dae_adjust(dae_index);

			break;
		}
        case KEY_FUNC_SWITCH_LANTENCY_MODE:
		{
#ifdef CONFIG_BT_MUSIC_SWITCH_LANTENCY_SUPPORT
            bt_music_handle_switch_lantency();
#endif
			break;
		}
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
		case KEY_FUNC_SWITCH_MIC_MUTE:
		{
			SYS_LOG_INF("mute");
            btmusic->mic_mute ^= 1;

	        if (!btmusic->mic_mute)
	        {
		        SYS_LOG_INF("unmute");
		        btmusic_event_notify(SYS_EVENT_MIC_MUTE_ON);
	        }
	        else
	        {
		        SYS_LOG_INF("mute");
		        btmusic_event_notify(SYS_EVENT_MIC_MUTE_OFF);
	        }
			break;
		}
#endif

		default:
		{
			SYS_LOG_ERR("unknow key func %d\n", keyfunc);
			break;
		}
	}

    btmusic->key_handled = 1;

	return 0;
}

void btmusic_input_event_proc(struct app_msg *msg)
{
    struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();
	int key_filter_time;
    
    /* 检查key是否已经屏蔽，防止两边同时按下处理两次按键
     */
    if(btmusic->need_resume_key)
        return;
    
    if(msg->cmd == KEY_FUNC_PLAY_PAUSE)
		key_filter_time = 500;
	else
		key_filter_time = 200;

    btmusic_tws_send(BTSRV_TWS_NONE, TWS_EVENT_REMOTE_KEY_PROC_BEGIN, &msg->cmd, 1, false);

	btmusic_key_filter(key_filter_time);

	if (btmusic_tws_send(BTSRV_TWS_MASTER, TWS_EVENT_BT_MUSIC_KEY_CTRL, &msg->cmd, 1, false) == 0)
		return;

	btmusic_key_func_proc(msg->cmd);
}

void btmusic_tws_event_proc(struct app_msg *msg)
{
	struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();
    tws_message_t* message = (tws_message_t*)msg->ptr;

	SYS_LOG_INF("%d %d", message->cmd_id, message->cmd_len);
	
	switch (message->cmd_id)
    {
        case TWS_EVENT_BT_MUSIC_KEY_CTRL:
		{
            /* 检查key是否已经处理，防止两边同时按下处理两次按键
             */
            if(!btmusic->key_handled)
                btmusic_key_func_proc(message->cmd_data[0]);
			break;
		}

        case TWS_EVENT_REMOTE_KEY_PROC_BEGIN:
		{
			int key_filter_time;
			if(message->cmd_data[0] == KEY_FUNC_PLAY_PAUSE)
				key_filter_time = 500;
			else
				key_filter_time = 200;
			btmusic_key_filter(key_filter_time);
			break;
		}

        case TWS_EVENT_AVDTP_FILTER:
		{
			char avdtp_filter = message->cmd_data[0];
			btmusic->avdtp_resume = message->cmd_data[1];
			bt_music_tws_avdtp_filter(avdtp_filter);
			break;
		}

		case TWS_EVENT_SYNC_BT_MUSIC_DAE:
		{
			u8_t dae_index = message->cmd_data[0];

			if (btmusic->multidae_enable == 0 || dae_index >= btmusic->dae_cfg_nums)
				break;

			btmusic_multi_dae_adjust(dae_index);
			break;
		}

        case TWS_EVENT_BT_MUSIC_START:
        {
            SYS_LOG_INF("start");
            btcall_exit_time = os_uptime_get_32() - DELAY_MUSIC_TIME_AFTER_BTCALL_SLAVE;
            thread_timer_start(&btmusic->play_timer, 2, 0);
            break;
        }
    }

}

void btmusic_tts_event_proc(struct app_msg *msg)
{
	struct btmusic_app_t *btmusic = (struct btmusic_app_t *)btmusic_get_app();

	if (!btmusic)
		return;

	switch (msg->value)
	{
		case TTS_EVENT_START_PLAY:
		{
			btmusic->mutex_stop = 1;
			ap_tts_status_set(AP_TTS_STATUS_PLAY);
			if (btmusic->player) {
				bt_music_stop_play();
				SYS_LOG_INF("mutex stop %d", btmusic_ctrl_avdtp_is_filter());
			}
			break;
		}

		case TTS_EVENT_STOP_PLAY:
		{
			btmusic->mutex_stop = 0;
			ap_tts_status_set(AP_TTS_STATUS_NONE);
			SYS_LOG_INF("mutex resume %d", btmusic_ctrl_avdtp_is_filter());
			if (!btmusic_ctrl_avdtp_is_filter()) {
				bt_manager_audio_stream_restore(BT_TYPE_BR);
			}
			break;
		}
	}
}
