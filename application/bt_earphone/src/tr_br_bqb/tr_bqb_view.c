/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief tr_audio_input app view
 */

#include "tr_bqb_app.h"
#include "ui_manager.h"
#include <input_manager.h>
#include <ui_manager.h>

static const ui_key_map_t tr_bqb_keymap[] =
{
    { KEY_VOLUMEUP,     KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD, BQB_STATUS_ALL, MSG_BQB_PLAY_VOLUP}, 
    { KEY_VOLUMEDOWN,   KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD, BQB_STATUS_ALL, MSG_BQB_PLAY_VOLDOWN},  
    { KEY_POWER,        KEY_TYPE_SHORT_UP, BQB_STATUS_ALL, MSG_BQB_PLAY_PAUSE_RESUME},  
    { KEY_PREVIOUSSONG, KEY_TYPE_SHORT_UP, BQB_STATUS_ALL, MSG_BQB_CONNECT_START},  
    { KEY_NEXTSONG,     KEY_TYPE_SHORT_UP, BQB_STATUS_ALL, MSG_BQB_CONNECT_DISCONNECT_A2DP},  
    { KEY_NEXTSONG,     KEY_TYPE_LONG_DOWN, BQB_STATUS_ALL, MSG_BQB_CONNECT_DISCONNECT_ACL},  
    { KEY_NEXTSONG,     KEY_TYPE_DOUBLE_CLICK, BQB_STATUS_ALL, MSG_BQB_MUTE},      
    { KEY_MENU,         KEY_TYPE_LONG_DOWN, BQB_STATUS_ALL, MSG_BQB_CLEAR_LIST},
    {KEY_RESERVED,      0,  0, 0}
};

static int _tr_bqb_view_proc(u8_t view_id, u8_t msg_id, void* msg_data)
{
	switch (msg_id)
    {
        case MSG_VIEW_CREATE:
            SYS_LOG_INF("CREATE\n");
            break;
        case MSG_VIEW_DELETE:
            SYS_LOG_INF("DELETE\n");
            break;
        case MSG_VIEW_PAINT:
            break;
        default:
            break;
    }
    return 0;
}

void tr_bqb_view_init(void)
{
	ui_view_info_t  view_info;

	memset(&view_info, 0, sizeof(ui_view_info_t));

	view_info.view_proc = _tr_bqb_view_proc;
	view_info.view_key_map = tr_bqb_keymap;
	view_info.view_get_state = tr_bqb_get_status;
	view_info.order = 1;
	view_info.app_id = app_manager_get_current_app();

	ui_view_create(TR_BQB_VIEW, &view_info);

	SYS_LOG_INF(" ok\n");
}

void tr_bqb_view_deinit(void)
{
	ui_view_delete(TR_BQB_VIEW);

	SYS_LOG_INF("ok\n");
}
