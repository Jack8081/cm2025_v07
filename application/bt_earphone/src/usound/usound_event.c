/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief usound event
 */

#include "usound.h"
#include "tts_manager.h"

#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif
#ifdef CONFIG_USOUND_DOUBLE_SOUND_CARD
extern int soundcard1_soft_vol;
extern int soundcard2_soft_vol;
#endif

static void usound_restore(struct thread_timer *ttimer, void *expiry_fn_arg)
{
    usound_playback_init();
    usound_capture_init();
    k_sleep(K_MSEC(10));
    usound_capture_start();
    usound_playback_start();
}

void usound_tts_event_proc(struct app_msg *msg)
{
	struct usound_app_t *usound = usound_get_app();

    SYS_LOG_INF("%d", msg->value);

	switch (msg->value) {
	case TTS_EVENT_START_PLAY:
		usound->tts_playing = 1;
		if (thread_timer_is_running(&usound->tts_monitor_timer)) {
			thread_timer_stop(&usound->tts_monitor_timer);
		}

		usound_playback_stop();
        usound_capture_stop();

		usound_playback_exit();
		usound_capture_exit();
		break;

	case TTS_EVENT_STOP_PLAY:
		usound->tts_playing = 0;
		if (thread_timer_is_running(&usound->tts_monitor_timer)) {
			thread_timer_stop(&usound->tts_monitor_timer);
		}

#ifdef CONFIG_PLAYTTS
        tts_manager_wait_finished(true);
#endif
		thread_timer_init(&usound->tts_monitor_timer, usound_restore, NULL);
		thread_timer_start(&usound->tts_monitor_timer, 1000, 0);
		break;
	}
}

static void _usound_host_vol_sync_cb(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct usound_app_t *usound = usound_get_app();
    
    SYS_LOG_INF("host sync vol:%d\n", usound->current_volume_level);
    system_volume_set(AUDIO_STREAM_USOUND, usound->current_volume_level, false);

    usound->volume_req_level = usound->current_volume_level;
    usound->host_vol_sync    = 0;

    /* play tts, doesn't play "max volume" tts when sync volume firstly */
    if (usound->host_vol_sync_tts) {
        if(usound->current_volume_level >= USOUND_VOLUME_LEVEL_MAX) {
    		usound_event_notify(SYS_EVENT_MAX_VOLUME);
    	}
    	else {
    	    if (usound->current_volume_level == 0) {
                usound_event_notify(SYS_EVENT_MIN_VOLUME);
                usb_audio_set_stream_mute(true);
            } else {
                usb_audio_set_stream_mute(false);
            }
    	}
    } else {
        usound->host_vol_sync_tts = 1;
    }
}

void usound_event_proc(struct app_msg *msg)
{
	struct usound_app_t *usound = usound_get_app();
    if (!usound) {
		return;
	}

	SYS_LOG_INF("\n--- cmd %d ---\n", msg->cmd);

	switch (msg->cmd) {
	case MSG_USOUND_STREAM_START:
        SYS_LOG_INF("Download stream start\n");
        usound_playback_init();
        k_sleep(K_MSEC(10));
        usound_playback_start();
        break;

	case MSG_USOUND_STREAM_STOP:
        SYS_LOG_INF("Download stream stop\n");
        usound_playback_stop();
        break;

	case MSG_USOUND_UPLOAD_STREAM_START: 
        SYS_LOG_INF("Upload stream start\n");
        usound_capture_init();
        k_sleep(K_MSEC(10));
        usound_capture_start();
        break;

	case MSG_USOUND_UPLOAD_STREAM_STOP:
        SYS_LOG_INF("Upload stream stop\n");
        usound_capture_stop();
        break;

    case MSG_USOUND_STREAM_MUTE:
        SYS_LOG_INF("Host Set Mute");
        usb_audio_set_stream_mute(1);
        usound->volume_req_type = USOUND_VOLUME_NONE;
        break;
	
    case MSG_USOUND_STREAM_UNMUTE:
        SYS_LOG_INF("Host Set Unmute");
        usb_audio_set_stream_mute(0);
        break;

    case MSG_USOUND_MUTE_SHORTCUT:
        break;

    case MSG_USOUND_HOST_VOL_SYNC:
        if (thread_timer_is_running(&usound->host_vol_sync_timer)) {
			thread_timer_stop(&usound->host_vol_sync_timer);
		}
		thread_timer_init(&usound->host_vol_sync_timer, _usound_host_vol_sync_cb, NULL);
		thread_timer_start(&usound->host_vol_sync_timer, 100, 0);
        usound->host_vol_sync = 1;
        break;

    case MSG_USOUND_INCALL_RING:
        SYS_LOG_INF("MSG_USOUND_INCALL_RING\n");
        usound->host_voice_state = VOICE_STATE_INGOING;
        break;

    case MSG_USOUND_INCALL_OUTGOING:
        SYS_LOG_INF("MSG_USOUND_INCALL_OUTGOING\n");
        usound->host_voice_state = VOICE_STATE_OUTGOING;
        break;

    case MSG_USOUND_INCALL_HOOK_UP:
        SYS_LOG_INF("MSG_USOUND_INCALL_HOOK_UP\n");
        usound->host_voice_state = VOICE_STATE_ONGOING;
        break;

    case MSG_USOUND_INCALL_HOOK_HL:
        SYS_LOG_INF("MSG_USOUND_INCALL_HOOK_HL\n");
        usound->host_voice_state = VOICE_STATE_NULL;
        break;

    case MSG_USOUND_INCALL_MUTE:
        SYS_LOG_INF("MSG_USOUND_INCALL_MUTE\n");
        break;

    case MSG_USOUND_INCALL_UNMUTE:
        SYS_LOG_INF("MSG_USOUND_INCALL_UNMUTE\n");
        break;

    default:
        break;
    }

#ifdef CONFIG_ESD_MANAGER
	esd_manager_save_scene(TAG_PLAY_STATE, &usound->playing, 1);
#endif
}

void usound_bt_event_proc(struct app_msg *msg)
{
    struct usound_app_t *usound = usound_get_app();
    if (!usound) {
		return;
	}

	SYS_LOG_INF("\n--- cmd %d ---\n", msg->cmd);

	switch (msg->cmd) {
    case BT_REQ_RESTART_PLAY:
        /* received cmd from tool_aset */
        usound_playback_stop();
        usound_playback_exit();

        usound_capture_stop();
        usound_capture_exit();

        usound_playback_init();
        usound_capture_init();
        k_sleep(K_MSEC(50));
        usound_playback_start();
        usound_capture_start();
        break;

    default:
        break;
    }
}


