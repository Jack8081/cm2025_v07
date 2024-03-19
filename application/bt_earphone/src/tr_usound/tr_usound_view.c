/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief tr_usound app view
 */

#include "tr_usound.h"
#include "ui_manager.h"
#include <input_manager.h>
#include <ui_manager.h>
#if 0
static const ui_key_map_t tr_usound_keymap[] = {

	{ KEY_VOLUMEUP,	KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD, USOUND_STATUS_ALL, MSG_USOUND_PLAY_VOLUP},
	{ KEY_VOLUMEDOWN,	KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD, USOUND_STATUS_ALL, MSG_USOUND_PLAY_VOLDOWN},
	{ KEY_POWER,		KEY_TYPE_SHORT_UP,							 USOUND_STATUS_ALL, MSG_USOUND_PLAY_PAUSE_RESUME},
	{ KEY_NEXTSONG,     KEY_TYPE_SHORT_UP,								USOUND_STATUS_ALL, MSG_USOUND_PLAY_NEXT},
	{ KEY_PREVIOUSSONG, KEY_TYPE_SHORT_UP,								USOUND_STATUS_ALL, MSG_USOUND_PLAY_PREV},

#ifdef CONFIG_BT_MIC_GAIN_ADJUST
	{ KEY_NEXTSONG,     KEY_TYPE_DOUBLE_CLICK,								USOUND_STATUS_ALL, MSG_USOUND_PLAY_GAIN_UP},
	{ KEY_PREVIOUSSONG, KEY_TYPE_DOUBLE_CLICK,								USOUND_STATUS_ALL, MSG_USOUND_PLAY_GAIN_DOWN},
#ifdef CONFIG_ENCODER_KNOB_INPUT
	{ ENCODER_KNOB_FORWARD,     ENCODER_KNOB_TYPE,								USOUND_STATUS_ALL, MSG_USOUND_PLAY_MIC2_GAIN_UP},
	{ ENCODER_KNOB_BACKWARD, ENCODER_KNOB_TYPE,								USOUND_STATUS_ALL, MSG_USOUND_PLAY_MIC2_GAIN_DOWN},
#else
	{ KEY_NEXTSONG,     KEY_TYPE_TRIPLE_CLICK,								USOUND_STATUS_ALL, MSG_USOUND_PLAY_MIC2_GAIN_UP},
	{ KEY_PREVIOUSSONG, KEY_TYPE_TRIPLE_CLICK,								USOUND_STATUS_ALL, MSG_USOUND_PLAY_MIC2_GAIN_DOWN},
#endif
#endif

#ifdef CONFIG_BT_MIC_SUPPORT_DENOISE
	{ KEY_PREVIOUSSONG,   KEY_TYPE_LONG_DOWN,								USOUND_STATUS_ALL, MSG_USOUND_PLAY_DENOISE_LEVEL_SET},
#endif
	{ KEY_POWER, KEY_TYPE_DOUBLE_CLICK,								USOUND_STATUS_ALL, MSG_USOUND_PLAY_KEY_SET_MIX_MODE},
	{ KEY_RESERVED,	0,	0,	0}
};
#endif

const ui_key_func_t tr_usound_func_table[] = {
	{ KEY_FUNC_PLAY_PAUSE, 0},
	{ KEY_FUNC_PREV_MUSIC, 0},
	{ KEY_FUNC_NEXT_MUSIC, 0},
	{ KEY_FUNC_ADD_MUSIC_VOLUME, 0},
	{ KEY_FUNC_SUB_MUSIC_VOLUME, 0},
	{ KEY_FUNC_SWITCH_MIC_MUTE, 0},
	{ KEY_FUNC_DAE_SWITCH, 0},
	{ KEY_FUNC_DONGLE_ENTER_SEARCH, 0},
};

static ui_key_map_t* tr_usound_keymap = NULL;

static int _tr_usound_view_proc(u8_t view_id, u8_t msg_id, void* msg_data)
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

void tr_usound_view_display(void)
{
    return;//
}

void tr_usound_call_view_display(void)
{

}

static int _tr_usound_get_status(void)
{
	int status = 0;

	//if (bt_manager_get_connected_dev_num())
	{
		status = bt_manager_media_get_status();
	}

	SYS_LOG_INF("status:%x", status);

	return status;
}

static bool _tr_usound_status_match(u32_t current_state, u32_t match_state)
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

void tr_usound_view_init(uint8_t view_id, bool key_remap)
{
	struct tr_usound_app_t* tr_usound = tr_usound_get_app();
	ui_view_info_t  view_info;

	os_sched_lock();
	ui_key_filter(true);

	if (tr_usound_keymap)
	{
		ui_key_map_delete(tr_usound_keymap);
		ui_view_delete(tr_usound->curr_view_id);	
	}
	memset(&view_info, 0, sizeof(ui_view_info_t));

    view_info.view_proc = _tr_usound_view_proc;
	tr_usound_keymap = ui_key_map_create(tr_usound_func_table, ARRAY_SIZE(tr_usound_func_table));
	view_info.view_get_state = _tr_usound_get_status;
	view_info.view_state_match = _tr_usound_status_match;

	view_info.view_key_map = tr_usound_keymap;
	view_info.order = 1;
	view_info.app_id = APP_ID_TR_USOUND;

	ui_view_create(view_id, &view_info);
	tr_usound->curr_view_id = view_id;

	if (!key_remap) {
	    tr_usound->last_disp_status = -1;
		tr_usound_view_display();
    }

	ui_key_filter(false);
    os_sched_unlock();
	SYS_LOG_INF(" ok\n");
}

void tr_usound_view_switch(uint8_t view_id)
{
	struct tr_usound_app_t* tr_usound = tr_usound_get_app();

	if (tr_usound_keymap && tr_usound->curr_view_id == view_id) {
		SYS_LOG_INF("already init");
		return;
	}

	tr_usound_view_init(view_id, false);
	SYS_LOG_INF("ok %d\n", view_id);
}

void tr_usound_view_deinit(void)
{
	struct tr_usound_app_t* tr_usound = tr_usound_get_app();

	if (!tr_usound_keymap)
		return;

	ui_key_map_delete(tr_usound_keymap);
	tr_usound_keymap = NULL;
	ui_view_delete(tr_usound->curr_view_id);

	SYS_LOG_INF("ok\n");
}

