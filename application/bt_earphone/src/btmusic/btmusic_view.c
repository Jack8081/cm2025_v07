/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt music app view
 */

#include "btmusic.h"
#include <input_manager.h>
#include "ui_manager.h"

#define BTMUSCI_STATUS_CONNECTED (1<<0)
#define BTMUSCI_STATUS_PLAYING   (1<<1)
#define BTMUSCI_STATUS_PAUSED   (1<<2)
#define BTMUSCI_STATUS_NOT_IN_SIRI  (1<<3)

const ui_key_func_t btmusic_func_table[] = {
	{ KEY_FUNC_PLAY_PAUSE, BTMUSCI_STATUS_CONNECTED | BTMUSCI_STATUS_NOT_IN_SIRI},
	{ KEY_FUNC_PREV_MUSIC, BTMUSCI_STATUS_CONNECTED | BTMUSCI_STATUS_NOT_IN_SIRI},
	{ KEY_FUNC_NEXT_MUSIC, BTMUSCI_STATUS_CONNECTED | BTMUSCI_STATUS_NOT_IN_SIRI},
	{ KEY_FUNC_PREV_MUSIC_IN_PLAYING, BTMUSCI_STATUS_CONNECTED | BTMUSCI_STATUS_PLAYING | BTMUSCI_STATUS_NOT_IN_SIRI},
	{ KEY_FUNC_NEXT_MUSIC_IN_PLAYING, BTMUSCI_STATUS_CONNECTED | BTMUSCI_STATUS_PLAYING | BTMUSCI_STATUS_NOT_IN_SIRI},
	{ KEY_FUNC_PREV_MUSIC_IN_PAUSED, BTMUSCI_STATUS_CONNECTED | BTMUSCI_STATUS_PAUSED | BTMUSCI_STATUS_NOT_IN_SIRI},
	{ KEY_FUNC_NEXT_MUSIC_IN_PAUSED, BTMUSCI_STATUS_CONNECTED | BTMUSCI_STATUS_PAUSED | BTMUSCI_STATUS_NOT_IN_SIRI},
	{ KEY_FUNC_ADD_MUSIC_VOLUME, 0},
	{ KEY_FUNC_SUB_MUSIC_VOLUME, 0},
	{ KEY_FUNC_ADD_MUSIC_VOLUME_IN_LINKED, BTMUSCI_STATUS_CONNECTED},
	{ KEY_FUNC_SUB_MUSIC_VOLUME_IN_LINKED, BTMUSCI_STATUS_CONNECTED},
	{ KEY_FUNC_DAE_SWITCH, 0},
    { KEY_FUNC_SWITCH_LANTENCY_MODE, 0},
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
	{ KEY_FUNC_SWITCH_MIC_MUTE, 0}
#endif
};

static ui_key_map_t* btmusic_keymap = NULL;

static int _btmusic_view_proc(u8_t view_id, u8_t msg_id, void* msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_CREATE:
	{
		SYS_LOG_INF("CREATE");
		break;
	}
	case MSG_VIEW_DELETE:
	{
		SYS_LOG_INF(" DELETE");

		break;
	}
	case MSG_VIEW_PAINT:
	{
		break;
	}
	}
	return 0;
}

void btmusic_view_display(void)
{
	struct btmusic_app_t *app = btmusic_get_app();
    u8_t disp_status = 0;
	
	if (bt_manager_get_connected_dev_num() > 0)
	{
        if (bt_manager_hfp_get_status() == BT_STATUS_SIRI) {
            disp_status = BT_STATUS_SIRI;
        } else {
            disp_status = bt_manager_a2dp_get_status();
        }
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
                
            case BT_STATUS_SIRI:
                ui_event_led_display(UI_EVENT_VOICE_ASSIST_START);
                break;

            default:
                ui_event_led_display(UI_EVENT_BT_MUSIC_DISP_NONE);
                break;
        }
    }
}

static int _btmusic_get_status(void)
{
	int play_status = bt_manager_a2dp_get_status();

	int status = 0;

	if (bt_manager_get_connected_dev_num())
	{
		status |= BTMUSCI_STATUS_CONNECTED;

		if (play_status == BT_STATUS_PLAYING)
			status |= BTMUSCI_STATUS_PLAYING;
		else
			status |= BTMUSCI_STATUS_PAUSED;

		if (bt_manager_hfp_get_status() != BT_STATUS_SIRI)
			status |= BTMUSCI_STATUS_NOT_IN_SIRI;
	}

	SYS_LOG_INF("status:%x", status);

	return status;
}

bool _btmusic_status_match(u32_t current_state, u32_t match_state)
{
	u32_t dev_type = match_state>>28;
	u32_t state = match_state&0xfffffff;

	if(ui_key_check_dev_type(dev_type))
	{
		SYS_LOG_INF("current:%x match:%x", current_state, state);
		return (current_state & state) == state;
	}
	return false;
}

void btmusic_view_init(bool key_remap)
{
	struct btmusic_app_t* app = btmusic_get_app();
	ui_view_info_t  view_info;
	
	ui_key_filter(true);
	
	if (btmusic_keymap)
	{
		ui_key_map_delete(btmusic_keymap);
		ui_view_delete(BTMUSIC_VIEW);	
	}
	memset(&view_info, 0, sizeof(ui_view_info_t));
	
	view_info.view_proc = _btmusic_view_proc;
	btmusic_keymap = ui_key_map_create(btmusic_func_table, ARRAY_SIZE(btmusic_func_table));
	view_info.view_key_map = btmusic_keymap;
	view_info.view_get_state = _btmusic_get_status;
	view_info.view_state_match = _btmusic_status_match;
	view_info.order = 1;
	view_info.app_id = APP_ID_BTMUSIC;
	ui_view_create(BTMUSIC_VIEW, &view_info);
	
	if (!key_remap) {
	    app->last_disp_status = -1;
		btmusic_view_display();
	}

	ui_key_filter(false);	
	SYS_LOG_INF("ok");
}

void btmusic_view_deinit(void)
{
	struct btmusic_app_t* app = btmusic_get_app();
	
	if (!btmusic_keymap)
		return;

    if (app->last_disp_status != 0) {
	    ui_event_led_display(UI_EVENT_BT_MUSIC_DISP_NONE);
	}
	ui_key_map_delete(btmusic_keymap);
	btmusic_keymap = NULL;
	ui_view_delete(BTMUSIC_VIEW);
	
	SYS_LOG_INF("ok");
}
