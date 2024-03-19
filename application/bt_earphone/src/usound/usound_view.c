/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief usound app view
 */

#include "usound.h"
#include "ui_manager.h"
#include <input_manager.h>
#include <ui_manager.h>

const ui_key_func_t usound_func_table[] = {
    { KEY_FUNC_PLAY_PAUSE,          0 },
    { KEY_FUNC_PREV_MUSIC,          0 },
    { KEY_FUNC_NEXT_MUSIC,          0 },
    { KEY_FUNC_ADD_MUSIC_VOLUME,    0 },
    { KEY_FUNC_SUB_MUSIC_VOLUME,    0 },
    { KEY_FUNC_SWITCH_MIC_MUTE,     0 },
    { KEY_FUNC_DAE_SWITCH,          0 },
    { KEY_FUNC_SWITCH_NR_FLAG,      0 },
    { KEY_FUNC_ACCEPT_INCOMING,     0 },
    { KEY_FUNC_REJECT_INCOMING,     0 },
    { KEY_FUNC_CALL_MUTE,           0 },
    { KEY_FUNC_CALL_UNMUTE,         0 },
};

static ui_key_map_t* usound_keymap = NULL;

static int _usound_view_proc(u8_t view_id, u8_t msg_id, void* msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_CREATE:
		SYS_LOG_INF("CREATE\n");
		break;

	case MSG_VIEW_DELETE:
		SYS_LOG_INF(" DELETE\n");
		break;

	case MSG_VIEW_PAINT:
		break;
	}
	return 0;
}

static int _usound_get_status(void)
{
	int status = 0;
	SYS_LOG_INF("status:%x", status);
	return status;
}

static bool _usound_status_match(u32_t current_state, u32_t match_state)
{
	u32_t state = match_state&0xfffffff;

	SYS_LOG_INF("current:%x match:%x", current_state, state);
	return (current_state & state) == state;
}

void usound_view_init(uint8_t view_id, bool key_remap)
{
	struct usound_app_t* usound = usound_get_app();
	ui_view_info_t  view_info;

	os_sched_lock();
	ui_key_filter(true);

	if (usound_keymap)
	{
		ui_key_map_delete(usound_keymap);
		ui_view_delete(usound->curr_view_id);	
	}
	memset(&view_info, 0, sizeof(ui_view_info_t));

    view_info.view_proc = _usound_view_proc;
	usound_keymap = ui_key_map_create(usound_func_table, ARRAY_SIZE(usound_func_table));
	view_info.view_get_state = _usound_get_status;
	view_info.view_state_match = _usound_status_match;

	view_info.view_key_map = usound_keymap;
	view_info.order = 1;
	view_info.app_id = APP_ID_USOUND;

	ui_view_create(view_id, &view_info);
	usound->curr_view_id = view_id;

	ui_key_filter(false);
    os_sched_unlock();
	SYS_LOG_INF(" ok\n");
}

void usound_view_deinit(void)
{
	struct usound_app_t* usound = usound_get_app();

	if (!usound_keymap)
		return;

	ui_key_map_delete(usound_keymap);
	usound_keymap = NULL;
	ui_view_delete(usound->curr_view_id);

	SYS_LOG_INF("ok\n");
}

