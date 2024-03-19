/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt call view
 */

#include "btcall.h"
#include <input_manager.h>
#include "ui_manager.h"


const ui_key_func_t btcall_func_table[] = {
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

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
    { KEY_FUNC_ADD_MUSIC_VOLUME_IN_LINKED, BT_STATUS_HFP_ALL },
    { KEY_FUNC_SUB_MUSIC_VOLUME_IN_LINKED, BT_STATUS_HFP_ALL },
#endif
}; 

static ui_key_map_t* btcall_keymap = NULL;

static int _btcall_view_proc(u8_t view_id, u8_t msg_id, u32_t msg_data)
{
	switch (msg_id) {
		case MSG_VIEW_CREATE:
		{
			SYS_LOG_INF("CREATE\n");
			break;
		}
		case MSG_VIEW_DELETE:
		{
			SYS_LOG_INF("DELETE\n");
			break;
		}
		case MSG_VIEW_PAINT:
		{
			break;
		}
	}
    return 0;
}

u32_t _btcall_get_state(void)
{
	u32_t hfp_state;
	u32_t dev_type = 0;
	u32_t state;

	hfp_state = bt_manager_hfp_get_status();

	state = (dev_type << 28) | hfp_state;

	return state;
}

bool _btcall_state_match(u32_t current_state, u32_t match_state)
{
	u32_t cur_sta = current_state & 0xFFFFFFF;
	u32_t mat_sta = match_state & 0xFFFFFFF;
	u32_t mat_dev_type = match_state >> 28;
	int ret;

	SYS_LOG_INF("### current_state: 0x%08x, match_state: 0x%08x\n", current_state, match_state);

	// match status
	if((cur_sta & mat_sta) == 0)
		return false;

	/* match device type */
	ret = ui_key_check_dev_type(mat_dev_type);
	if(ret == NO)
		return false;

	return true;
}

/* 蓝牙通话状态 LED 显示
 */
void btcall_led_display(int hfp_status)
{
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    return;
#endif
    SYS_LOG_INF("0x%x", hfp_status);
    
	switch(hfp_status)
	{
		case BT_STATUS_HFP_NONE:
			ui_event_led_display(UI_EVENT_BT_CALL_DISP_NONE);
			break;

		case BT_STATUS_INCOMING:
			ui_event_led_display(UI_EVENT_BT_CALL_INCOMING);
			break;

		case BT_STATUS_OUTGOING:
			ui_event_led_display(UI_EVENT_BT_CALL_OUTGOING);
			break;

		case BT_STATUS_ONGOING:
		case BT_STATUS_MULTIPARTY:
			ui_event_led_display(UI_EVENT_BT_CALL_ONGOING);
			break;

		case BT_STATUS_SIRI:
			ui_event_led_display(UI_EVENT_VOICE_ASSIST_START);
			break;

		case BT_STATUS_3WAYIN:
			ui_event_led_display(UI_EVENT_BT_CALL_3WAYIN);
			break;

		default:
			break;
	}
}

void btcall_view_init(bool key_remap)
{
	struct btcall_app_t* btcall = btcall_get_app();
	ui_view_info_t  view_info;

	ui_key_filter(true);
	
	if (btcall_keymap)
	{
		ui_view_delete(BTCALL_VIEW);
		ui_key_map_delete(btcall_keymap);
	}
	
	memset(&view_info, 0, sizeof(ui_view_info_t));
	btcall_keymap = ui_key_map_create((ui_key_func_t *)btcall_func_table, (sizeof(btcall_func_table)/sizeof(ui_key_func_t)));
	view_info.view_proc = (ui_view_proc_t)_btcall_view_proc;
	view_info.view_key_map = btcall_keymap;
	view_info.view_get_state = (ui_get_state_t)_btcall_get_state;
	view_info.view_state_match = (ui_state_match_t)_btcall_state_match;
	view_info.order = 1;
	view_info.app_id = APP_ID_BTCALL;
	ui_view_create(BTCALL_VIEW, &view_info);

	if (!key_remap) {
	    btcall->last_disp_status = bt_manager_hfp_get_status();
		btcall_led_display(btcall->last_disp_status);
	}
	
	ui_key_filter(false);	
	SYS_LOG_INF("ok\n");
}

void btcall_view_deinit(void)
{
	struct btcall_app_t* btcall = btcall_get_app();
	
	if (!btcall_keymap)
		return;

    if (btcall->last_disp_status != BT_STATUS_HFP_NONE) {
        ui_event_led_display(UI_EVENT_BT_CALL_DISP_NONE);
    }
	ui_view_delete(BTCALL_VIEW);
	ui_key_map_delete(btcall_keymap);
	btcall_keymap = NULL;

	SYS_LOG_INF("ok\n");
}
