/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt call view
 */

#include "tr_bqb_ag.h"
#include "ui_manager.h"
#include <input_manager.h>
#include <ui_manager.h>

const ui_key_map_t tr_bqb_ag_keymap[] =
{
	{ KEY_VOLUMEUP,     KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD, 	BT_STATUS_HFP_ALL, 							MSG_TR_BQB_AG_VOLUP}, 
	{ KEY_VOLUMEDOWN,   KEY_TYPE_SHORT_UP | KEY_TYPE_LONG | KEY_TYPE_HOLD, 	BT_STATUS_HFP_ALL, 							MSG_TR_BQB_AG_VOLDOWN},  
	{ KEY_POWER,        KEY_TYPE_SHORT_UP, 									BT_STATUS_MULTIPARTY | BT_STATUS_3WAYIN, 	MSG_BT_HOLD_CURR_ANSWER_ANOTHER},  
	{ KEY_POWER,        KEY_TYPE_SHORT_UP, 									BT_STATUS_OUTGOING | BT_STATUS_ONGOING, 	MSG_BT_HANGUP_CALL}, 
	{ KEY_POWER,        KEY_TYPE_SHORT_UP, 									BT_STATUS_INCOMING, 						MSG_BT_ACCEPT_CALL},
	//{ KEY_POWER,        KEY_TYPE_SHORT_UP, 									BT_STATUS_SIRI, 							MSG_BT_SIRI_STOP},   
	{ KEY_POWER,        KEY_TYPE_LONG_DOWN,    								BT_STATUS_MULTIPARTY | BT_STATUS_3WAYIN, 	MSG_BT_HANGUP_ANOTHER},
	{ KEY_POWER,        KEY_TYPE_LONG_DOWN,     							BT_STATUS_INCOMING, 						MSG_BT_REJECT_CALL},    
	{ KEY_MENU,        	KEY_TYPE_SHORT_UP,     								BT_STATUS_MULTIPARTY | BT_STATUS_3WAYIN, 	MSG_BT_HANGUP_ANOTHER},
	{ KEY_MENU,        	KEY_TYPE_SHORT_UP,     								BT_STATUS_INCOMING, 						MSG_BT_REJECT_CALL}, 
	{ KEY_MENU,        	KEY_TYPE_SHORT_UP,     								BT_STATUS_HFP_ALL, 							MSG_NULL}, 
	{ KEY_NEXTSONG,    	KEY_TYPE_SHORT_UP,     								BT_STATUS_ONGOING | BT_STATUS_3WAYIN | BT_STATUS_MULTIPARTY, 		MSG_TR_BQB_AG_SWITCH_CALLOUT},
	{ KEY_PREVIOUSSONG, KEY_TYPE_SHORT_UP, 									BT_STATUS_ONGOING | BT_STATUS_MULTIPARTY | BT_STATUS_SIRI | BT_STATUS_3WAYIN, MSG_TR_BQB_AG_SWITCH_MICMUTE},
	{ KEY_POWER,       	KEY_TYPE_DOUBLE_CLICK, 								BT_STATUS_MULTIPARTY | BT_STATUS_3WAYIN, 	MSG_BT_HANGUP_CURR_ANSER_ANOTHER},          
	{ KEY_POWER,        KEY_TYPE_LONG_DOWN,   								BT_STATUS_HFP_ALL, 							MSG_NULL},
	{ KEY_RESERVED,     0,  0, 0}
};

static int _tr_bqb_ag_view_proc(u8_t view_id, u8_t msg_id,  void* msg_data)
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

void tr_bqb_ag_view_init(void)
{
	ui_view_info_t  view_info;

	memset(&view_info, 0, sizeof(ui_view_info_t));

	view_info.view_proc = _tr_bqb_ag_view_proc;
	view_info.view_key_map = tr_bqb_ag_keymap;
	view_info.view_get_state = bt_manager_hfp_get_status;
	view_info.order = 1;
	view_info.app_id = APP_ID_TR_BQB_AG;
	ui_view_create(TR_BQB_AG_VIEW, &view_info);

#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_clear_screen(LED_CLEAR_ALL);
#endif

	SYS_LOG_INF("ok\n");
}

void tr_bqb_ag_view_deinit(void)
{
	ui_view_delete(TR_BQB_AG_VIEW);
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_clear_screen(LED_CLEAR_ALL);
#endif
	SYS_LOG_INF("ok\n");
}
