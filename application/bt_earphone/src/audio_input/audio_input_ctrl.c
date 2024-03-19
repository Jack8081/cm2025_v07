/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio_input event
 */

#include "audio_input.h"
#include "ui_manager.h"
#include "tts_manager.h"
#include "app_ui.h"
#include "system_notify.h"

#ifdef CONFIG_CONSUMER_EQ
#include "consumer_eq.h"
#endif

static int audio_input_tws_send(int receiver, int event, void* param, int len, bool sync_wait)
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

void audio_input_event_notify(int event)
{
	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE)
	{
		return;
	}

	sys_manager_event_notify(event, true);
}

static void audio_input_key_resume(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct audio_input_app_t *audio_input = audio_input_get_app();
	
	ui_key_filter(false);
	audio_input->need_resume_key = 0;
    audio_input->key_handled = 0;
}

static void audio_input_key_filter(int duration)
{
	struct audio_input_app_t *audio_input = audio_input_get_app();

	ui_key_filter(true);

	if (thread_timer_is_running(&audio_input->key_timer)) 
    {
		thread_timer_stop(&audio_input->key_timer);

		if(audio_input->need_resume_key)
			ui_key_filter(false);
	}

	audio_input->need_resume_key = 1;

	thread_timer_init(&audio_input->key_timer, audio_input_key_resume, NULL);
	thread_timer_start(&audio_input->key_timer, duration, 0);
	SYS_LOG_INF("time:%d", duration);
}

static void audio_input_volume_adjust(int adjust)
{
	int volume;
	int stream_type = audio_input_get_audio_stream_type(app_manager_get_current_app());

	volume = system_volume_get(stream_type) + adjust;
	system_volume_set(stream_type, volume, false);

	volume = system_volume_get(stream_type);

	if(volume >= CFG_MAX_BT_MUSIC_VOLUME)
	{
		audio_input_event_notify(SYS_EVENT_MAX_VOLUME);
	}
	else if(volume <= 0)
	{
		audio_input_event_notify(SYS_EVENT_MIN_VOLUME);
	}
}

int _audio_input_get_status(void)
{
	struct audio_input_app_t *audio_input = audio_input_get_app();

	if (!audio_input)
		return AUDIO_INPUT_STATUS_NULL;

	if (audio_input->playing)
		return AUDIO_INPUT_STATUS_PLAYING;
	else
		return AUDIO_INPUT_STATUS_PAUSED;
}

static int audio_input_key_func_proc(int keyfunc)
{
	struct audio_input_app_t *audio_input = audio_input_get_app();

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
			if (_audio_input_get_status() == AUDIO_INPUT_STATUS_PLAYING) {
				SYS_LOG_INF("pause\n");
				audio_input_stop_play();
				/*FIXME filter media stream*/
				audio_input_event_notify(SYS_EVENT_PLAY_PAUSE);
			} else {
				SYS_LOG_INF("play\n");
				audio_input_start_play();
				audio_input_event_notify(SYS_EVENT_PLAY_START);
				/*FIXME filter/unfilter media stream*/
			}
			audio_input_view_display();
			break;
		}

		case KEY_FUNC_ADD_MUSIC_VOLUME:
		{
			SYS_LOG_INF("up");
			audio_input_volume_adjust(1);
			break;
		}

		case KEY_FUNC_SUB_MUSIC_VOLUME:
		{
			SYS_LOG_INF("down");
			audio_input_volume_adjust(-1);
			break;
		}

		default:
		{
			SYS_LOG_ERR("unknow key func %d\n", keyfunc);
			break;
		}
	}

    audio_input->key_handled = 1;

	return 0;
}

void audio_input_input_event_proc(struct app_msg *msg)
{
	struct audio_input_app_t *audio_input = audio_input_get_app();
	int key_filter_time;

    /* 检查key是否已经屏蔽，防止两边同时按下处理两次按键
     */
    if(audio_input->need_resume_key)
        return;

    if(msg->cmd == KEY_FUNC_PLAY_PAUSE)
		key_filter_time = 500;
	else
		key_filter_time = 200;

    audio_input_tws_send(BTSRV_TWS_NONE, TWS_EVENT_REMOTE_KEY_PROC_BEGIN, &msg->cmd, 1, false);

	audio_input_key_filter(key_filter_time);

	if (audio_input_tws_send(BTSRV_TWS_MASTER, TWS_EVENT_BT_MUSIC_KEY_CTRL, &msg->cmd, 1, false) == 0)
		return;

	audio_input_key_func_proc(msg->cmd);
}

void audio_input_tws_event_proc(struct app_msg *msg)
{
	struct audio_input_app_t *audio_input = audio_input_get_app();
    tws_message_t* message = (tws_message_t*)msg->ptr;

	SYS_LOG_INF("tws cmd id %d %d", message->cmd_id, message->cmd_len);
	
	switch (message->cmd_id)
    {
        case TWS_EVENT_BT_MUSIC_KEY_CTRL:
		{
            /* 检查key是否已经处理，防止两边同时按下处理两次按键
             */
            if(!audio_input->key_handled)
                audio_input_key_func_proc(message->cmd_data[0]);
			break;
		}

        case TWS_EVENT_REMOTE_KEY_PROC_BEGIN:
		{
			int key_filter_time;
			if(message->cmd_data[0] == KEY_FUNC_PLAY_PAUSE)
				key_filter_time = 500;
			else
				key_filter_time = 200;
			audio_input_key_filter(key_filter_time);
			break;
		}

    }
}

