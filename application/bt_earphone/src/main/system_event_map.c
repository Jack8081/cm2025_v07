/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system event map
 */
#include <sys_event.h>
#include <app_ui.h>


static const struct sys_event_ui_map sys_event_map[] = {
	{SYS_EVENT_POWER_ON,            UI_EVENT_POWER_ON,  true},
	{SYS_EVENT_POWER_OFF,           UI_EVENT_POWER_OFF,	true},
	{SYS_EVENT_STANDBY,             UI_EVENT_STANDBY, true},
	{SYS_EVENT_WAKE_UP,             UI_EVENT_WAKE_UP, true},

	{SYS_EVENT_BATTERY_LOW,         UI_EVENT_BATTERY_LOW, true},
	{SYS_EVENT_BATTERY_LOW_EX,      UI_EVENT_BATTERY_LOW_EX, true},
	{SYS_EVENT_BATTERY_TOO_LOW,     UI_EVENT_BATTERY_TOO_LOW, true},
	{SYS_EVENT_REPEAT_BAT_LOW,      UI_EVENT_REPEAT_BAT_LOW, true},
	
	{SYS_EVENT_CHARGE_START,        UI_EVENT_CHARGE_START, true},
	{SYS_EVENT_CHARGE_FULL,         UI_EVENT_CHARGE_FULL, true},
	{SYS_EVENT_FRONT_CHARGE_POWON,  UI_EVENT_FRONT_CHARGE_POWON, true},
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
	{SYS_EVENT_ENTER_PAIR_MODE,     UI_EVENT_ENTER_PAIRING_MODE, true},
#else
    {SYS_EVENT_ENTER_PAIR_MODE,     UI_EVENT_ENTER_PAIR_MODE, true},
#endif
	{SYS_EVENT_CLEAR_PAIRED_LIST,   UI_EVENT_CLEAR_PAIRED_LIST, true},

#if (!defined CONFIG_SAMPLE_CASE_1) && (!defined CONFIG_SAMPLE_CASE_XNT)
	{SYS_EVENT_BT_WAIT_PAIR,        UI_EVENT_BT_WAIT_CONNECT, true},
    {SYS_EVENT_BT_CONNECTED, 		UI_EVENT_BT_CONNECTED, true},
	{SYS_EVENT_2ND_CONNECTED, 		UI_EVENT_2ND_CONNECTED, true},	
	{SYS_EVENT_BT_DISCONNECTED, 	UI_EVENT_BT_DISCONNECTED, true},
	{SYS_EVENT_BT_UNLINKED, 		UI_EVENT_BT_UNLINKED, true},
#endif

#if (!defined CONFIG_SAMPLE_CASE_1) && (!defined CONFIG_SAMPLE_CASE_XNT)
	{SYS_EVENT_TWS_START_PAIR, 	    UI_EVENT_TWS_WAIT_PAIR, true},
#endif
	{SYS_EVENT_TWS_CONNECTED,       UI_EVENT_TWS_CONNECTED, true},
	{SYS_EVENT_TWS_DISCONNECTED,    UI_EVENT_TWS_DISCONNECTED, true},	
	
	{SYS_EVENT_MIN_VOLUME,          UI_EVENT_MIN_VOLUME, true},
	{SYS_EVENT_MAX_VOLUME, 	        UI_EVENT_MAX_VOLUME, true},
	{SYS_EVENT_PLAY_START,          UI_EVENT_BT_MUSIC_PLAY, true},	
	{SYS_EVENT_PLAY_PAUSE,          UI_EVENT_BT_MUSIC_PAUSE, true},
	{SYS_EVENT_PLAY_PREVIOUS,       UI_EVENT_PREV_MUSIC, true},	
	{SYS_EVENT_PLAY_NEXT,           UI_EVENT_NEXT_MUSIC, true},
	
	{SYS_EVENT_BT_CALL_OUTGOING,    UI_EVENT_BT_CALL_OUTGOING, true},
	{SYS_EVENT_BT_CALL_INCOMING,    UI_EVENT_BT_CALL_INCOMING, true},	
	{SYS_EVENT_BT_CALL_3WAYIN,      UI_EVENT_BT_CALL_3WAYIN, true},
	{SYS_EVENT_BT_CALL_REJECT,      UI_EVENT_BT_CALL_REJECT, true},	
	{SYS_EVENT_BT_CALL_ONGOING,     UI_EVENT_BT_CALL_ONGOING, true},
	{SYS_EVENT_BT_CALL_END,         UI_EVENT_BT_CALL_END, true},	
	{SYS_EVENT_SWITCH_CALL_OUT,     UI_EVENT_SWITCH_CALL_OUT, true},
	{SYS_EVENT_MIC_MUTE_ON,         UI_EVENT_MIC_MUTE_ON, true},	
	{SYS_EVENT_MIC_MUTE_OFF,        UI_EVENT_MIC_MUTE_OFF, true},

	{SYS_EVENT_SIRI_START,          UI_EVENT_VOICE_ASSIST_START, true},	
	{SYS_EVENT_SIRI_STOP,           UI_EVENT_VOICE_ASSIST_STOP, true},
	
	{SYS_EVENT_HID_PHOTO_SHOT,      UI_EVENT_HID_PHOTO_SHOT, true},

	{SYS_EVENT_ENTER_LINEIN,        UI_EVENT_ENTER_LINEIN, true},
	{SYS_EVENT_LINEIN_PLAY,         UI_EVENT_LINEIN_PLAY, true},	
	{SYS_EVENT_LINEIN_PAUSE,        UI_EVENT_LINEIN_PAUSE, true},	
	
	{SYS_EVENT_SEL_VOICE_LANG_1,    UI_EVENT_SEL_VOICE_LANG_1, true},
	{SYS_EVENT_SEL_VOICE_LANG_2,    UI_EVENT_SEL_VOICE_LANG_2, true},	
	
	{SYS_EVENT_ENTER_BQB_TEST_MODE, UI_EVENT_ENTER_BQB_TEST_MODE, true},

	{SYS_EVENT_CUSTOMED_1,          UI_EVENT_CUSTOMED_1, true},
	{SYS_EVENT_CUSTOMED_2,          UI_EVENT_CUSTOMED_2, true},	
	{SYS_EVENT_CUSTOMED_3,          UI_EVENT_CUSTOMED_3, true},
	{SYS_EVENT_CUSTOMED_4,          UI_EVENT_CUSTOMED_4, true},	
	{SYS_EVENT_CUSTOMED_5,          UI_EVENT_CUSTOMED_5, true},
	{SYS_EVENT_CUSTOMED_6,          UI_EVENT_CUSTOMED_6, true},	
	{SYS_EVENT_CUSTOMED_7,          UI_EVENT_CUSTOMED_7, true},
	{SYS_EVENT_CUSTOMED_8,          UI_EVENT_CUSTOMED_8, true},	
	{SYS_EVENT_CUSTOMED_9,          UI_EVENT_CUSTOMED_9, true},

	{SYS_EVENT_LOW_LATENCY_MODE,    UI_EVENT_LOW_LATENCY_MODE, true},
	{SYS_EVENT_NORMAL_LATENCY_MODE, UI_EVENT_NORMAL_LATENCY_MODE, true},	
	{SYS_EVENT_PRIVMA_TALK_START,   UI_EVENT_PRIVMA_TALK_START, true},
	{SYS_EVENT_DC5V_CMD_COMPLETE,   UI_EVENT_DC5V_CMD_COMPLETE, true},	
	{SYS_EVENT_NMA_COLLECTION,      UI_EVENT_NMA_COLLECTION, true},

	{SYS_EVENT_BT_MUSIC_DAE_SWITCH, UI_EVENT_BT_MUSIC_DAE_SWITCH, true},
	{SYS_EVENT_DAE_DEFAULT,         UI_EVENT_DAE_DEFAULT, true},
	{SYS_EVENT_DAE_CUSTOM1,         UI_EVENT_DAE_CUSTOM1, true},
#ifdef CONFIG_SAMPLE_CASE_XNT	
	{SYS_EVENT_DAE_CUSTOM2,         UI_EVENT_DAE_CUSTOM2, true},
	{SYS_EVENT_DAE_CUSTOM3,         UI_EVENT_DAE_CUSTOM3, true},	
	{SYS_EVENT_DAE_CUSTOM4,         UI_EVENT_DAE_CUSTOM4, true},
	{SYS_EVENT_DAE_CUSTOM5,	        UI_EVENT_DAE_CUSTOM5, true},	
	{SYS_EVENT_DAE_CUSTOM6,	        UI_EVENT_DAE_CUSTOM6, true},
	{SYS_EVENT_DAE_CUSTOM7,	        UI_EVENT_DAE_CUSTOM7, true},	
	{SYS_EVENT_DAE_CUSTOM8,         UI_EVENT_DAE_CUSTOM8, true},	
	{SYS_EVENT_DAE_CUSTOM9,         UI_EVENT_DAE_CUSTOM9, true},
#else
	{SYS_EVENT_DAE_CUSTOM2,         UI_EVENT_DAE_CUSTOM1, true},
	{SYS_EVENT_DAE_CUSTOM3,         UI_EVENT_DAE_CUSTOM1, true},	
	{SYS_EVENT_DAE_CUSTOM4,         UI_EVENT_DAE_CUSTOM1, true},
	{SYS_EVENT_DAE_CUSTOM5,	        UI_EVENT_DAE_CUSTOM1, true},	
	{SYS_EVENT_DAE_CUSTOM6,	        UI_EVENT_DAE_CUSTOM1, true},
	{SYS_EVENT_DAE_CUSTOM7,	        UI_EVENT_DAE_CUSTOM1, true},	
	{SYS_EVENT_DAE_CUSTOM8,         UI_EVENT_DAE_CUSTOM1, true},	
	{SYS_EVENT_DAE_CUSTOM9,         UI_EVENT_DAE_CUSTOM1, true},
#endif	
    {SYS_EVENT_NOMAL_MODE,          UI_EVENT_NOMAL_MODE, true},   
	{SYS_EVENT_ANC_MODE, 		    UI_EVENT_ANC_MODE, true},	
    {SYS_EVENT_TRANSPARENCY_MODE,   UI_EVENT_TRANSPARENCY_MODE, true}, 
    {SYS_EVENT_ANC_LEISURE_MODE,    UI_EVENT_ANC_LEISURE_MODE, true}, 
    {SYS_EVENT_ANC_WIND_NOISE_MODE, UI_EVENT_ANC_WIND_NOISE_MODE, true},     
	{SYS_EVENT_RING_HANDSET,        UI_EVENT_RING_HANDSET, true},	
	{SYS_EVENT_ALEXA_TONE_PLAY,     UI_EVENT_ALEXA_TONE_PLAY, true},		
	{SYS_EVENT_BT_CONNECTED_USB, 	UI_EVENT_BT_CONNECTED_USB, true},
	{SYS_EVENT_BT_DISCONNECTED_USB, UI_EVENT_BT_DISCONNECTED_USB, true},
	{SYS_EVENT_BT_CONNECTED_BR, 	UI_EVENT_BT_CONNECTED_BR, true},
	{SYS_EVENT_BT_DISCONNECTED_BR, 	UI_EVENT_BT_DISCONNECTED_BR, true},
	{SYS_EVENT_ENTER_PAIRING_MODE,  UI_EVENT_ENTER_PAIRING_MODE, true},

	{SYS_EVENT_XEAR_SURROUND_ON,	UI_EVENT_XEAR_SURROUND_ON,	false},
	{SYS_EVENT_XEAR_SURROUND_OFF,	UI_EVENT_XEAR_SURROUND_OFF,	false},
	{SYS_EVENT_XEAR_NR_ON,			UI_EVENT_XEAR_NR_ON,		false},
	{SYS_EVENT_XEAR_NR_OFF,			UI_EVENT_XEAR_NR_OFF,		false},

};

u8_t system_event_sys_2_ui(u8_t event)
{
	u8_t ui_event = 0;
	int map_size = ARRAY_SIZE(sys_event_map);

	for (int i = 0; i < map_size; i++) {
		if (sys_event_map[i].sys_event == event) {
			ui_event = sys_event_map[i].ui_event;
			break;
		}
	}
	return ui_event;
}


int system_event_map_init(void)
{
	sys_event_map_register(sys_event_map, ARRAY_SIZE(sys_event_map), 0);
	return 0;
}
