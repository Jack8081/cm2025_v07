/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio_input app view
 */

#include "audio_input.h"
#include <input_manager.h>
#include "ui_manager.h"

const ui_key_func_t audio_input_func_table[] = {
	{ KEY_FUNC_PLAY_PAUSE, 0},
	{ KEY_FUNC_ADD_MUSIC_VOLUME, 0},
	{ KEY_FUNC_SUB_MUSIC_VOLUME, 0},
};

static ui_key_map_t* audio_input_keymap = NULL;

static int _audio_input_view_proc(u8_t view_id, u8_t msg_id, void* msg_data)
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

extern int _audio_input_get_status(void);

void audio_input_view_display(void)
{
	struct audio_input_app_t *app = audio_input_get_app();
	int disp_status = _audio_input_get_status();

    if (app->last_disp_status != disp_status)
    {
        SYS_LOG_INF("0x%x -> 0x%x", app->last_disp_status, disp_status);
        app->last_disp_status = disp_status;

        switch (disp_status)
        {
            case AUDIO_INPUT_STATUS_PLAYING:
                ui_event_led_display(UI_EVENT_BT_MUSIC_PLAY);
                break;
                
            case AUDIO_INPUT_STATUS_PAUSED:
                ui_event_led_display(UI_EVENT_BT_MUSIC_PAUSE);
                break;

            default:
                ui_event_led_display(UI_EVENT_BT_MUSIC_DISP_NONE);
                break;
        }
    }
}

static bool _audio_input_status_match(u32_t current_state, u32_t match_state)
{
	return true;
}

void audio_input_view_init(bool key_remap)
{
	struct audio_input_app_t* app = audio_input_get_app();
	ui_view_info_t  view_info;

	os_sched_lock();
	ui_key_filter(true);

	if (audio_input_keymap)
	{
		ui_key_map_delete(audio_input_keymap);
		ui_view_delete(BTMUSIC_VIEW);	
	}
	memset(&view_info, 0, sizeof(ui_view_info_t));

	view_info.view_proc = _audio_input_view_proc;
	audio_input_keymap = ui_key_map_create(audio_input_func_table, ARRAY_SIZE(audio_input_func_table));
	view_info.view_get_state = _audio_input_get_status;
	view_info.view_state_match = _audio_input_status_match;

	view_info.view_key_map = audio_input_keymap;
	view_info.order = 1;
	view_info.app_id = app_manager_get_current_app();

	ui_view_create(BTMUSIC_VIEW, &view_info);

	if (!key_remap) {
	    app->last_disp_status = -1;
		audio_input_view_display();
	}

	ui_key_filter(false);
    os_sched_unlock();
	SYS_LOG_INF("ok\n");
}

void audio_input_view_deinit(void)
{
	if (!audio_input_keymap)
		return;

	ui_key_map_delete(audio_input_keymap);
	audio_input_keymap = NULL;
	ui_view_delete(BTMUSIC_VIEW);

	SYS_LOG_INF("ok\n");
}

