/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief system app shell.
 */
#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <stdio.h>
#include <init.h>
#include <string.h>
#include <kernel.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <drivers/nvram_config.h>
#include <property_manager.h>
#include "system_app.h"
#include "bt_manager.h"

#ifdef CONFIG_SHELL
static int shell_input_key_event(const struct shell *shell, size_t argc, char *argv[])
{
	if (argv[1] != NULL) {
		u32_t key_event;
		key_event = strtoul (argv[1], (char **) NULL, 0);
		sys_event_report_input(key_event);
	}
	return 0;
}

static int shell_set_config(const struct shell *shell, size_t argc, char *argv[])
{
	int ret = 0;

	if (argc < 2) {
		ret = property_set(argv[1], argv[1], 0);
	} else {
		ret = property_set(argv[1], argv[2], strlen(argv[2]));
	}

	if (ret < 0) {
		ret = -1;
	} else {
		property_flush(NULL);
	}

	SYS_LOG_INF("set config %s : %s ok\n", argv[1], argv[2]);
	return 0;
}

static int shell_dump_bt_info(const struct shell *shell, size_t argc, char *argv[])
{
    bt_manager_dump_info();
    return 0;
}

#ifdef CONFIG_LE_AUDIO_APP
extern int leaudio_set_plc_mode(int mode);
#endif

#ifdef CONFIG_TR_USOUND_APP
extern int tr_usound_set_plc_mode(int mode);
#endif

#ifdef CONFIG_USOUND_APP
extern int usound_set_plc_mode(int mode);
#endif

static int shell_set_plc_mode(const struct shell *shell, size_t argc, char *argv[])
{
    int mode = atoi(argv[1]);
    void *tr_fg_app = app_manager_get_current_app();
    
    if (tr_fg_app && !strcmp(tr_fg_app, APP_ID_TR_USOUND))
    {
#ifdef CONFIG_TR_USOUND_APP
        tr_usound_set_plc_mode(mode); 
#endif
    }
    else if(tr_fg_app && !strcmp(tr_fg_app, APP_ID_LE_AUDIO))
    {
#ifdef CONFIG_LE_AUDIO_APP
        leaudio_set_plc_mode(mode);
#endif
    }
    else if(tr_fg_app && !strcmp(tr_fg_app, APP_ID_USOUND))
    {
#ifdef CONFIG_USOUND_APP
        usound_set_plc_mode(mode);
#endif
    }
    printk("plc mode:%d\n", mode);

	return 0;
}

#if 0
static int shell_key_map_setting(const struct shell *shell, size_t argc, char *argv[])
{
	CFG_Type_Key_Func_Map key_func_map[10];
	memset(&key_func_map, 0, sizeof(CFG_Type_Key_Func_Map));

	key_func_map[0].Key_Func = KEY_FUNC_PLAY_PAUSE;
	key_func_map[0].Key_Value = VKEY_VADD;
	key_func_map[0].LR_Device = KEY_DEVICE_CHANNEL_L;
	key_func_map[0].Key_Event = KEY_EVENT_SINGLE_CLICK;
	
	key_func_map[1].Key_Func = KEY_FUNC_PLAY_PAUSE;
	key_func_map[1].Key_Value = VKEY_VADD;
	key_func_map[1].LR_Device = KEY_DEVICE_CHANNEL_R;
	key_func_map[1].Key_Event = KEY_EVENT_DOUBLE_CLICK;

	ui_app_setting_key_map_update(2, &key_func_map);
	return 0;
}
#endif

#ifdef CONFIG_BT_TRANSCEIVER
static int tr_shell_cmd_applist(const struct shell *shell, size_t argc, char *argv[])
{
    tr_app_switch_list_show();
    return 0;
}

static int tr_shell_cmd_exec(const struct shell *shell, size_t argc, char *argv[])
{
    if(argv[1] != NULL) {
#ifdef CONFIG_ESD_MANAGER
        esd_manager_reset_finished();
#endif
        if (sys_get_work_mode() == RECEIVER_MODE)
        {
            if (strcmp(argv[1], app_manager_get_current_app())) {
                if ((!strcmp(APP_ID_LE_AUDIO, app_manager_get_current_app()))
                    && (bt_manager_switch_active_app_check() > 0)) {
                    extern int leaudio_playback_is_working(void);
                    if (leaudio_playback_is_working()) {
                        struct app_msg  msg = {0};
                        msg.type = MSG_BT_EVENT;
                        msg.cmd = BT_AUDIO_STREAM_PRE_STOP;
                        send_async_msg(APP_ID_LE_AUDIO, &msg);
                        os_sleep(120);
                    }
                }
#ifdef CONFIG_BT_MUSIC_APP
                else if ((!strcmp(APP_ID_BTMUSIC, app_manager_get_current_app()))
                    && (bt_manager_switch_active_app_check() > 0)) {
                    extern int bt_music_playback_is_working(void);
                    if (bt_music_playback_is_working()) {
                        struct app_msg  msg = {0};
                        msg.type = MSG_BT_EVENT;
                        msg.cmd = BT_AUDIO_STREAM_PRE_STOP;
                        send_async_msg(APP_ID_BTMUSIC, &msg);
                        os_sleep(120);
                    }
                }
#endif
                bt_manager_switch_active_app();
            }
        } else {
            app_switch(argv[1], 0, true);
        }
    }   
    return 0;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(acts_app_cmds,
#ifdef CONFIG_BT_TRANSCEIVER
	SHELL_CMD(list, NULL, "list app", tr_shell_cmd_applist),
	SHELL_CMD(exec, NULL, "exec app", tr_shell_cmd_exec),
#endif
	SHELL_CMD(set_config, NULL, "set system config", shell_set_config),
	SHELL_CMD(input, NULL, "input key event", shell_input_key_event),
	SHELL_CMD(btinfo, NULL, "dump bt info", shell_dump_bt_info),
	SHELL_CMD(set_plcmode, NULL, "set lost pkt mode", shell_set_plc_mode),

	//SHELL_CMD(keymap, NULL, "key map setting sample", shell_key_map_setting),

	SHELL_SUBCMD_SET_END
);

static int cmd_acts_app(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_REGISTER(app, &acts_app_cmds, "Application shell commands", cmd_acts_app);
#endif
