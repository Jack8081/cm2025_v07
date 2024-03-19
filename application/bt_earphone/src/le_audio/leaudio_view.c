/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief le audio app view
 */

#include "leaudio.h"
#include <input_manager.h>
#include "ui_manager.h"

const ui_key_func_t leaudio_func_table[] = {
	{ KEY_FUNC_PLAY_PAUSE, 0},
	{ KEY_FUNC_PREV_MUSIC, 0},
	{ KEY_FUNC_NEXT_MUSIC, 0},
	{ KEY_FUNC_ADD_MUSIC_VOLUME, 0},
	{ KEY_FUNC_SUB_MUSIC_VOLUME, 0},
	{ KEY_FUNC_SWITCH_MIC_MUTE, 0},
	{ KEY_FUNC_DAE_SWITCH, 0},
	{ KEY_FUNC_SWITCH_NR_FLAG, 0},
	{ KEY_FUNC_ACCEPT_INCOMING, 0},
	{ KEY_FUNC_REJECT_INCOMING, 0},
	{ KEY_FUNC_CALL_MUTE, 0},
	{ KEY_FUNC_CALL_UNMUTE, 0},
	{ KEY_FUNC_CUSTOMED_1, 0},
	{ KEY_FUNC_CUSTOMED_2, 0},
#ifdef CONFIG_LE_AUDIO_DOUBLE_SOUND_CARD
	{ KEY_FUNC_CUSTOMED_3, 0},
	{ KEY_FUNC_DOUBLE_SOUNDCARD_VOL_ADD, 0},
	{ KEY_FUNC_DOUBLE_SOUNDCARD_VOL_SUB, 0},
#endif
};

const ui_key_func_t leaudio_call_func_table[] = {
	{ KEY_FUNC_ACCEPT_CALL, 			BT_STATUS_INCOMING },
	{ KEY_FUNC_REJECT_CALL, 			BT_STATUS_INCOMING },
	{ KEY_FUNC_HANGUP_CALL, 			BT_STATUS_OUTGOING | BT_STATUS_ONGOING },
	{ KEY_FUNC_KEEP_CALL_RELEASE_3WAY,  BT_STATUS_MULTIPARTY | BT_STATUS_3WAYIN | BT_STATUS_3WAYOUT},
	{ KEY_FUNC_HOLD_CALL_ACTIVE_3WAY,   BT_STATUS_MULTIPARTY | BT_STATUS_3WAYIN | BT_STATUS_3WAYOUT},
	{ KEY_FUNC_HANGUP_CALL_ACTIVE_3WAY, BT_STATUS_MULTIPARTY | BT_STATUS_3WAYIN | BT_STATUS_3WAYOUT},
	{ KEY_FUNC_SWITCH_CALL_OUT, 		BT_STATUS_ONGOING | BT_STATUS_MULTIPARTY },
	{ KEY_FUNC_SWITCH_MIC_MUTE, 		BT_STATUS_ONGOING | BT_STATUS_MULTIPARTY | BT_STATUS_SIRI | BT_STATUS_3WAYIN | BT_STATUS_3WAYOUT},
	{ KEY_FUNC_ADD_CALL_VOLUME, 		BT_STATUS_HFP_ALL },
	{ KEY_FUNC_SUB_CALL_VOLUME, 		BT_STATUS_HFP_ALL },
	{ KEY_FUNC_STOP_VOICE_ASSIST, 		BT_STATUS_SIRI },
	{ KEY_FUNC_SWITCH_NR_FLAG, 			BT_STATUS_ONGOING },
};

static ui_key_map_t* leaudio_keymap = NULL;

static int _leaudio_view_proc(u8_t view_id, u8_t msg_id, void* msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_CREATE:
	{
		SYS_LOG_INF("CREATE\n");
		break;
	}
	case MSG_VIEW_DELETE:
	{
		SYS_LOG_INF(" DELETE\n");

		break;
	}
	case MSG_VIEW_PAINT:
	{
		break;
	}
	}
	return 0;
}

static int _leaudio_call_view_proc(u8_t view_id, u8_t msg_id, void* msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_CREATE:
	{
		SYS_LOG_INF("CREATE\n");
		break;
	}
	case MSG_VIEW_DELETE:
	{
		SYS_LOG_INF(" DELETE\n");

		break;
	}
	case MSG_VIEW_PAINT:
	{
		break;
	}
	}
	return 0;
}

void leaudio_view_display(void)
{
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    return;
#endif
	struct leaudio_app_t *app = leaudio_get_app();
    uint8_t disp_status = 0;
	//if (bt_manager_get_connected_dev_num() > 0)
	{
        disp_status = bt_manager_media_get_status();
	}

    if (app->last_disp_status != disp_status)
    {
        SYS_LOG_INF("0x%x -> 0x%x", app->last_disp_status, disp_status);
        app->last_disp_status = disp_status;

        switch (disp_status)
        {
            case BT_STATUS_PLAYING:
                ui_event_led_display(UI_EVENT_BT_MUSIC_PLAY);
                break;
                
            case BT_STATUS_PAUSED:
                ui_event_led_display(UI_EVENT_BT_MUSIC_PAUSE);
                break;

            default:
                ui_event_led_display(UI_EVENT_BT_MUSIC_DISP_NONE);
                break;
        }
    }
}

void leaudio_call_view_display(void)
{

}

static int _leaudio_get_status(void)
{
	int status = 0;

	//if (bt_manager_get_connected_dev_num())
	{
		status = bt_manager_media_get_status();
	}

	SYS_LOG_INF("status:%x", status);

	return status;
}

static int _leaudio_call_get_status(void)
{
	int status = 0;

	//if (bt_manager_get_connected_dev_num())
	{
		status = bt_manager_call_get_status();
	}

	SYS_LOG_INF("status:%x", status);

	return status;
}

static bool _leaudio_status_match(u32_t current_state, u32_t match_state)
{
	//u32_t dev_type = match_state>>28;
	u32_t state = match_state&0xfffffff;

	//if(ui_key_check_dev_type(dev_type))
	{
		SYS_LOG_INF("current:%x match:%x", current_state, state);
		return (current_state & state) == state;
	}
	//return false;
}

static bool _leaudio_call_status_match(u32_t current_state, u32_t match_state)
{
	//u32_t dev_type = match_state>>28;
	u32_t cur_sta = current_state&0xfffffff;
	u32_t mat_sta = match_state&0xfffffff;

	// match status
	if ((cur_sta & mat_sta) == 0)
		return false;

#if 0
	if(!ui_key_check_dev_type(dev_type))
		return false;
#endif

	return true;
}

void leaudio_view_init(uint8_t view_id, bool key_remap)
{
	struct leaudio_app_t* leaudio = leaudio_get_app();
	ui_view_info_t  view_info;

	os_sched_lock();
	ui_key_filter(true);

	if (leaudio_keymap)
	{
		ui_key_map_delete(leaudio_keymap);
		ui_view_delete(leaudio->curr_view_id);	
	}
	memset(&view_info, 0, sizeof(ui_view_info_t));

	if (BTMUSIC_VIEW == view_id) {
		view_info.view_proc = _leaudio_view_proc;
		leaudio_keymap = ui_key_map_create(leaudio_func_table, ARRAY_SIZE(leaudio_func_table));
		view_info.view_get_state = _leaudio_get_status;
		view_info.view_state_match = _leaudio_status_match;
	} else {
		view_info.view_proc = _leaudio_call_view_proc;
		leaudio_keymap = ui_key_map_create(leaudio_call_func_table, ARRAY_SIZE(leaudio_func_table));
		view_info.view_get_state = _leaudio_call_get_status;
		view_info.view_state_match = _leaudio_call_status_match;
	}

	view_info.view_key_map = leaudio_keymap;
	view_info.order = 1;
	view_info.app_id = APP_ID_LE_AUDIO;

	ui_view_create(view_id, &view_info);
	leaudio->curr_view_id = view_id;

	if (!key_remap) {
	    leaudio->last_disp_status = -1;
		if (BTMUSIC_VIEW == view_id)
			leaudio_view_display();
		else
			leaudio_call_view_display();
	}

	ui_key_filter(false);
    os_sched_unlock();
	SYS_LOG_INF("ok %d\n", view_id);
}

void leaudio_view_switch(uint8_t view_id)
{
	struct leaudio_app_t* leaudio = leaudio_get_app();

	if (leaudio_keymap && leaudio->curr_view_id == view_id) {
		SYS_LOG_INF("already init");
		return;
	}

	leaudio_view_init(view_id, false);
	SYS_LOG_INF("ok %d\n", view_id);
}

void leaudio_view_deinit(void)
{
	struct leaudio_app_t* leaudio = leaudio_get_app();

	if (!leaudio_keymap)
		return;

	ui_key_map_delete(leaudio_keymap);
	leaudio_keymap = NULL;
	ui_view_delete(leaudio->curr_view_id);

	SYS_LOG_INF("ok\n");
}
