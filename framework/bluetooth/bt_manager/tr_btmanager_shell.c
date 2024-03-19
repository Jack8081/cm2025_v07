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
#include "bt_manager_audio.h"
#include "api_common.h"

#ifdef CONFIG_SHELL
#ifdef CONFIG_BT_TRANSCEIVER
static int shell_btmgr_start_scan(const struct shell *shell, size_t argc, char *argv[])
{
    u8_t c;
    if(argv[1] != NULL)
    {
        c = *argv[1] - '0';
        SYS_LOG_INF("%d\n", c);
        if(c < LE_SEARCH_MAX) 
            tr_bt_manager_audio_scan_mode(c);

    }
    return 0;
}

int tr_bt_manager_call_originate(uint16_t handle, uint8_t *remote_uri);
int tr_bt_manager_call_accept(struct bt_audio_call *call);
int tr_bt_manager_call_hold(struct bt_audio_call *call);
int tr_bt_manager_call_retrieve(struct bt_audio_call *call);
int tr_bt_manager_call_terminate(struct bt_audio_call *call, uint8_t reason);

static int shell_btmgr_originate(const struct shell *shell, size_t argc, char *argv[])
{
    if(argv[1] == NULL)
    {
        SYS_LOG_INF("actions:zs303ah\n");
        tr_bt_manager_call_originate(0, "actions:zs303ah");
    }
    else
    {
        SYS_LOG_INF("%s\n", argv[1]);
        tr_bt_manager_call_originate(0, argv[1]);
    }
    return 0;
}

static int shell_btmgr_accept(const struct shell *shell, size_t argc, char *argv[])
{
    SYS_LOG_INF("%d\n", tr_bt_manager_call_accept(NULL));
    return 0;
}

static int shell_btmgr_hold(const struct shell *shell, size_t argc, char *argv[])
{
    SYS_LOG_INF("%d\n", tr_bt_manager_call_hold(NULL));
    return 0;
}

static int shell_btmgr_retrieve(const struct shell *shell, size_t argc, char *argv[])
{
    SYS_LOG_INF("%d\n", tr_bt_manager_call_retrieve(NULL));
    return 0;
}

static int shell_btmgr_terminate(const struct shell *shell, size_t argc, char *argv[])
{
    u8_t c = 3;//BT_TBS_REASON_SERVER_END
    if(argv[1] != NULL)
    {
        c = *argv[1] - '0';
    }

    SYS_LOG_INF("reason %d\n", c);
    SYS_LOG_INF("%d\n", tr_bt_manager_call_terminate(NULL, c));
    return 0;
}

int tr_bt_manager_call_remote_originate(uint16_t handle, uint8_t *remote_uri);
int tr_bt_manager_call_remote_accept(struct bt_audio_call *call);
int tr_bt_manager_call_remote_alert(struct bt_audio_call *call);
int tr_bt_manager_call_remote_hold(struct bt_audio_call *call);
int tr_bt_manager_call_remote_retrieve(struct bt_audio_call *call);
int tr_bt_manager_call_remote_terminate(struct bt_audio_call *call);


static int shell_btmgr_rt_originate(const struct shell *shell, size_t argc, char *argv[])
{
    if(argv[1] == NULL)
    {
        SYS_LOG_INF("actions:zs303ah\n");
        tr_bt_manager_call_remote_originate(0, "actions:zs303ah");
    }
    else
    {
        SYS_LOG_INF("%s\n", argv[1]);
        tr_bt_manager_call_remote_originate(0, argv[1]);
    }
    return 0;
}

static int shell_btmgr_rt_alert(const struct shell *shell, size_t argc, char *argv[])
{
    SYS_LOG_INF("%d\n", tr_bt_manager_call_remote_alert(NULL));
    return 0;
}

static int shell_btmgr_rt_accept(const struct shell *shell, size_t argc, char *argv[])
{
    SYS_LOG_INF("%d\n", tr_bt_manager_call_remote_accept(NULL));
    return 0;
}

static int shell_btmgr_rt_hold(const struct shell *shell, size_t argc, char *argv[])
{
    SYS_LOG_INF("%d\n", tr_bt_manager_call_remote_hold(NULL));
    return 0;
}

static int shell_btmgr_rt_retrieve(const struct shell *shell, size_t argc, char *argv[])
{
    SYS_LOG_INF("%d\n", tr_bt_manager_call_remote_retrieve(NULL));
    return 0;
}

static int shell_btmgr_rt_terminate(const struct shell *shell, size_t argc, char *argv[])
{
    SYS_LOG_INF("%d\n", tr_bt_manager_call_remote_terminate(NULL));
    return 0;
}

#ifdef CONFIG_APP_TRANSMITTER
int tr_bt_manager_ble_cli_send(uint16_t handle, u8_t * buf, u16_t len, s32_t timeout);
static int shell_btmgr_send_data(const struct shell *shell, size_t argc, char *argv[])
{
    struct conn_status status;
    if(API_APP_CONNECTION_CONFIG(0, &status) == 0)
    {
        SYS_LOG_INF("%d\n", tr_bt_manager_ble_cli_send(status.handle, argv[1], strlen(argv[1]), 0));
    }
    return 0;
}
#endif

static int shell_btmgr_mic_mute(const struct shell *shell, size_t argc, char *argv[])
{
    if(sys_get_work_mode())
        bt_manager_mic_client_mute();
    else
        bt_manager_mic_mute();

    return 0;
}
static int shell_btmgr_mic_unmute(const struct shell *shell, size_t argc, char *argv[])
{
    if(sys_get_work_mode())
        bt_manager_mic_client_unmute();
    else
        bt_manager_mic_unmute();

    return 0;
}

extern int atoi(const char* str);
void btcon_dump_print(void);
static int shell_btmgr_mic_set_gain(const struct shell *shell, size_t argc, char *argv[])
{
    if(argv[1] != NULL)
    {
        if(sys_get_work_mode())
            bt_manager_mic_client_gain_set((uint8_t)atoi(argv[1]));
        else
            bt_manager_mic_gain_set((uint8_t)atoi(argv[1]));
    }

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(acts_btbox_cmds,
	SHELL_CMD(scan, NULL, "scan start", shell_btmgr_start_scan),
	SHELL_CMD(tbs_originate, NULL, "originate", shell_btmgr_originate),
	SHELL_CMD(tbs_accept, NULL, "accept", shell_btmgr_accept),
	SHELL_CMD(tbs_hold, NULL, "hold", shell_btmgr_hold),
	SHELL_CMD(tbs_retrieve, NULL, "retrieve", shell_btmgr_retrieve),
	SHELL_CMD(tbs_terminate, NULL, "terminate", shell_btmgr_terminate),

	SHELL_CMD(tbs_rt_originate, NULL, "rt originate", shell_btmgr_rt_originate),
	SHELL_CMD(tbs_rt_alert, NULL, "rt alert", shell_btmgr_rt_alert),
	SHELL_CMD(tbs_rt_accept, NULL, "rt accept", shell_btmgr_rt_accept),
	SHELL_CMD(tbs_rt_hold, NULL, "rt hold", shell_btmgr_rt_hold),
	SHELL_CMD(tbs_rt_retrieve, NULL, "rt retrieve", shell_btmgr_rt_retrieve),
	SHELL_CMD(tbs_rt_terminate, NULL, "rt terminate", shell_btmgr_rt_terminate),

	SHELL_CMD(mics_mute, NULL, "mic mute", shell_btmgr_mic_mute),
	SHELL_CMD(mics_unmute, NULL, "mic unmute", shell_btmgr_mic_unmute),
	SHELL_CMD(mics_gain, NULL, "mic set gain", shell_btmgr_mic_set_gain),

#ifdef CONFIG_APP_TRANSMITTER
	SHELL_CMD(ble_send, NULL, "ble send data", shell_btmgr_send_data),
#endif

	SHELL_SUBCMD_SET_END
);

static int cmd_acts_btbox(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_REGISTER(btbox, &acts_btbox_cmds, "Btbox Application shell commands", cmd_acts_btbox);
#endif
#endif
