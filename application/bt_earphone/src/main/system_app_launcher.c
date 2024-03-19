/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app launcher
 */

#include <os_common_api.h>
#include <string.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <app_manager.h>
#include <hotplug_manager.h>
#include "app_switch.h"
#include "app_defines.h"
#include "system_app.h"
#include "audio_system.h"
#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif
#ifdef CONFIG_STUB_DEV_USB
#include <usb/class/usb_stub.h>
#endif
#include <usb/usb_otg.h>

#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "system app launcher"
#endif

int system_app_launch(u8_t mode)
{
	char *default_app = APP_ID_BTMUSIC;

	/** add app to app switch manager*/
	app_switch_add_app(APP_ID_BTMUSIC);

#ifdef CONFIG_AUDIO_INPUT_APP
	app_switch_add_app(APP_ID_LINEIN);
	app_switch_add_app(APP_ID_I2SRX_IN);
#endif

#ifdef CONFIG_LE_AUDIO_APP
	btmgr_leaudio_cfg_t* leaudio_config = bt_manager_leaudio_config();
	if (leaudio_config->leaudio_enable){
		app_switch_add_app(APP_ID_LE_AUDIO);
		/** if support leaudio, default app: leaudio */
		default_app = APP_ID_LE_AUDIO;
	}
#endif

#ifdef CONFIG_TR_LINEIN_APP
    if(sys_get_work_mode())
    {
        app_switch_add_app(APP_ID_TR_LINEIN);
        default_app = APP_ID_TR_LINEIN;
    }
#endif

#ifdef CONFIG_TR_USOUND_APP
    if(sys_get_work_mode())
    {
        app_switch_add_app(APP_ID_TR_USOUND);
        default_app = APP_ID_TR_USOUND;
    }
#endif

#ifdef CONFIG_USOUND_APP
	app_switch_add_app(APP_ID_USOUND);
	//if (usb_phy_get_vbus()) {
    //    default_app = APP_ID_USOUND;
    //}
#endif

#ifdef CONFIG_BT_BR_TRANSCEIVER
    app_switch_add_app(APP_ID_TR_BR_BQB);
    default_app = APP_ID_TR_BR_BQB;
#endif

#ifdef CONFIG_BT_BR_TRANSCEIVER
	app_switch_add_app(APP_ID_TR_BQB_AG);
#endif

#ifdef CONFIG_LE_AUDIO_BQB
    if(sys_get_work_mode())
    {
        app_switch_add_app(APP_ID_USOUND_BQB);
        default_app = APP_ID_USOUND_BQB;
    }
#endif

#ifdef CONFIG_ESD_MANAGER
	if (esd_manager_check_esd()) {
		u8_t app_id = 0;
		u16_t volume_info = 0;

		esd_manager_restore_scene(TAG_APP_ID, &app_id, 1);
		default_app = app_switch_get_app_name(app_id);

	#ifdef CONFIG_AUDIO
		esd_manager_restore_scene(TAG_VOLUME, (u8_t *)&volume_info, 2);
		audio_system_set_stream_volume((volume_info >> 8) & 0xff, volume_info & 0xff);
	#endif

		/**
		 * if esd from btcall ,we switch btmusic first avoid call hangup when reset
		 * if hfp is always connected  device will auto switch to btcall later
		 */
		if (!strcmp(default_app, APP_ID_BTCALL)) {
			default_app = APP_ID_BTMUSIC;
		}
	}
#endif

	SYS_LOG_INF("default_app: %s\n", default_app);

	if (app_switch(default_app, APP_SWITCH_CURR, false) == 0xff) {
		app_switch(APP_ID_BTMUSIC, APP_SWITCH_CURR, false);
	}
	
	bt_manager_switch_active_app_init(app_manager_get_current_app());
	return 0;
}

int system_app_launch_init(void)
{
	const char *app_id_list_array[] = app_id_list;

	if (app_switch_init(app_id_list_array, ARRAY_SIZE(app_id_list_array)))
		return -EINVAL;

	return 0;
}

