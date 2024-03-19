/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager a2dp profile.
 */
#define SYS_LOG_DOMAIN "bt manager"

//#include <logging/sys_log.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <shell/shell.h>
#include <stream.h>
#include <acts_ringbuf.h>
#include <bt_manager.h>
#include <sys_event.h>
#include <app_manager.h>
#include <power_manager.h>
#include <bt_manager_inner.h>
#include "audio_policy.h"
#include "app_switch.h"
#include "btservice_api.h"
#include "media_player.h"
#include <volume_manager.h>
#include <audio_system.h>
#include <msg_manager.h>
#include "api_bt_module.h"



/************************************************************************
 *  test shell
 * */
static u8_t ascii_trans(u8_t *data)
{
	if (48 <= *data && *data <= 57)    // number
	{
		return *data - 48;
	}
	else if ( 65 <= *data && *data <= 70)	//capital letter
	{
		return *data - 55;
	}
	else if ( 97 <= *data && *data <= 102)
	{
		return *data - 87;	//small letter
	}
	else
	{
		return 0;
	}
	
}

static u32_t char_dec_trans(u8_t len, u8_t *data)
{
	u32_t sum = 0;
	u8_t i;
	
	for (i = 0; i < len; i++)
	{
		sum = sum * 16 + ascii_trans(data + i);
	}

	return sum;
}

static void tr_bt_manager_inquiry_complete_result_proc(void * param, int dev_cnt)
{
   if(dev_cnt)
   {
       //inquiry complete event
//       inquiry_info_t * buf = (inquiry_info_t *)param;
//       api_bt_connect(buf->addr);
       API_BT_INQUIRY_FREE();
   }
   else
   {
       //single inquiry event
   }
}


static int shell_bt_manager_trs_inquiry(int argc, char *argv[])
{
    u8_t times;
    u8_t nums;
    u32_t filter_cod;
    int len;

	if (argc != 4)
	{
		return -1;
	}

	len = strlen(argv[1]);
	times = char_dec_trans(len, argv[1]);

	len = strlen(argv[2]);
	nums = char_dec_trans(len, argv[2]);
	
	len = strlen(argv[3]);
	filter_cod = char_dec_trans(len, argv[3]);
    SYS_LOG_INF("time %d , cod, %04x\n",times, filter_cod);
    api_bt_inquiry(nums, times, filter_cod, tr_bt_manager_inquiry_complete_result_proc);
    return 0;
	
}

static int shell_bt_manager_trs_init(int argc, char *argv[])
{
    tr_bt_manager_trs_init();
    return 0;
}

static int shell_bt_manager_trs_deinit(int argc, char *argv[])
{
    tr_bt_manager_trs_deinit();
    return 0;
}

static int shell_bt_manager_trs_conn(int argc, char *argv[])
{
	u8_t addr[6];
	u8_t i;
	s8_t j;
	u8_t len;
	u16_t bd_addr;

	if (argc != 2)
	{
		return -1;
	}

	len = strlen(argv[1]);
	if (len != 17)
	{
		return -1;
	}

	j = 5;
	for(i = 4; i < 10; i++)		//TxdData_Buffer 4~10
	{
		bd_addr = char_dec_trans(2, argv[1]+(i-4)*3);
		if (bd_addr > 255)
		{
			printk("bd_addr out of scope!\n");
			return -1;
		}
		
		addr[j--] = (u8_t)bd_addr;
	}

    printk("connect addr %02x:%02x:%02x:%02x:%02x:%02x!\n", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

    api_bt_connect(addr);
    return 0;
}


/*
static int  tr_shell_restart_bt_service(int argc, char *argv[])
{
	struct app_msg  msg = {0};
	tr_app_work_mode_e work_mode;
	u8_t appid;

	if (argc != 3)
	{
		return -1;
	}
	
	work_mode = strtoul(argv[1], NULL, 10);
	appid = strtoul(argv[2], NULL, 10);

	msg.type = MSG_TR_RESTART_BT_SERVICE;
	msg.cmd = work_mode;
	msg.value = appid;
	send_sync_msg("main", &msg);
	
	return 0;
}
*/
static int shell_bt_manager_get_pairlist(int argc, char *argv[])
{
    u8_t index;
    int len, ret;
    paired_dev_info_t *pdl, *dev;

	if (argc != 2)
	{
		return -1;
	}

	len = strlen(argv[1]);
	index = char_dec_trans(len, argv[1]);
    
    len = index < 8 ? 1 : 8;
    pdl = mem_malloc(sizeof(paired_dev_info_t) * len);
    memset(pdl, 0, sizeof(paired_dev_info_t) * len);
    dev = pdl;
    ret = api_bt_get_PDL(pdl, index);
    
    printk("=================pairplist================\n");
    if(index < 8)
    {
        printk("addr:%02x:%02x:%02x:%02x:%02x:%02x,name:%s\n", 
                pdl->addr[5],pdl->addr[4],pdl->addr[3],pdl->addr[2],pdl->addr[1] ,pdl->addr[0],pdl->name);
    }
    else
    {
        for(int i = 0; i < 8; i++)
        {
            printk("addr:%02x:%02x:%02x:%02x:%02x:%02x,name:%s\n", 
                pdl->addr[5],pdl->addr[4],pdl->addr[3],pdl->addr[2],pdl->addr[1] ,pdl->addr[0],pdl->name);
            pdl++;
        }
    }
    mem_free(dev);
    printk("=================  end   ================\n");
    return 0;
	
}

static int shell_bt_manager_del_pairlist(int argc, char *argv[])
{
    u8_t index;
    int len, ret;
	if(argc != 2)
	{
		return -1;
	}

	len = strlen(argv[1]);
	index = char_dec_trans(len, argv[1]);
    ret = api_bt_del_PDL(index);
    return 0;
}

static int shell_bt_manager_disconnect(int argc, char *argv[])
{
    u8_t index;
    int len, ret;
	if(argc != 2)
	{
		return -1;
	}

	len = strlen(argv[1]);
	index = char_dec_trans(len, argv[1]);
    SYS_LOG_INF("index:%d\n", index);
    ret = api_bt_disconnect(index);
    return 0;
}

static int shell_bt_manager_test(int argc, char *argv[])
{
    int len , enable;
 	len = strlen(argv[1]);
	enable = char_dec_trans(len, argv[1]);
    tr_btif_a2dp_mute(enable);

	return 0;
}

static int shell_bt_manager_status(int argc, char *argv[])
{
    int state = bt_manager_get_status();

	switch (state)
	{
		case BT_STATUS_NONE:
			printk("BT_STATUS_NONE\n");
			break;
		case BT_STATUS_WAIT_PAIR:
			printk("BT_STATUS_WAIT_PAIR\n");
			break;		
		case BT_STATUS_CONNECTED:
			printk("BT_STATUS_CONNECTED\n");
			break;
		case BT_STATUS_DISCONNECTED:
			printk("BT_STATUS_DISCONNECTED\n");
			break;
		case BT_STATUS_TWS_WAIT_PAIR:
			printk("BT_STATUS_TWS_WAIT_PAIR\n");
			break;
		case BT_STATUS_TWS_PAIRED:
			printk("BT_STATUS_TWS_PAIRED\n");
			break;
		case BT_STATUS_TWS_UNPAIRED:
			printk("BT_STATUS_TWS_UNPAIRED\n");
			break;
		case BT_STATUS_MASTER_WAIT_PAIR:
			printk("BT_STATUS_MASTER_WAIT_PAIR\n");
			break;
		case BT_STATUS_PAUSED:
			printk("BT_STATUS_PAUSED\n");
			break;
		case BT_STATUS_PLAYING:
			printk("BT_STATUS_PLAYING\n");
			break;
//		case BT_STATUS_INCOMING:
//			printk("BT_STATUS_INCOMING\n");
//			break;
//		case BT_STATUS_OUTGOING:
//			printk("BT_STATUS_OUTGOING\n");
//			break;
		case BT_STATUS_MULTIPARTY:
			printk("BT_STATUS_MULTIPARTY\n");
			break;
		case BT_STATUS_INQUIRY:
			printk("BT_STATUS_INQUIRY\n");
			break;
		case BT_STATUS_INQUIRY_COMPLETE:
			printk("BT_STATUS_INQUIRY_COMPLETE\n");
			break;

		case BT_STATUS_CONNECTING:
			printk("BT_STATUS_CONNECTING\n");
			break;

		case BT_STATUS_CONNECTED_INQUIRY_COMPLETE:
			printk("BT_STATUS_CONNECTED_INQUIRY_COMPLETE\n");
			break;

		default:
			printk("default state %d\n", state);
			break;
	}
    return 0;
}

static int shell_bt_manager_auto_stop(int argc, char *argv[])
{
    int len , enable;
 	len = strlen(argv[1]);
	enable = char_dec_trans(len, argv[1]);
    api_bt_auto_move_keep_stop(enable);

	return 0;
}

static int shell_bt_manager_auto_start(int argc, char *argv[])
{
    api_bt_auto_move_restart_from_stop();

	return 0;
}

static int shell_bt_manager_get_vol(int argc, char *argv[])
{
    int ret = 0;
	ret = api_bt_get_absvolume(NULL);
	printk("ret 0x%x\n", ret);
	return 0;
}

static int shell_btmgr_disconnect_reason(int argc, char *argv[])
{
	int reason;

	reason = bt_manager_user_api_get_last_dev_disconnect_reason_transmitter();
	printk("last dev disconnect reason is 0x%x\n", reason);
	
	return 0;
}

static int shell_btmgr_inquiry(int argc, char *argv[])
{
	bt_manager_user_api_force_to_inquiry_transmitter();
	
	return 0;
}

static int shell_btmgr_repdl(int argc, char *argv[])
{
	bt_manager_user_api_force_to_reconnect_pairlist_transmitter();
	
	return 0;
}

static int shell_btmgr_delpdl(int argc, char *argv[])
{
	bt_manager_user_api_force_to_delete_pairlist_transmitter();
	
	return 0;
}

static int shell_btmgr_get_status(int argc, char *argv[])
{
	int status;

	status = bt_manager_user_api_get_status_transmitter();
	printk("btsvr status is 0x%x\n", status);
	
	return 0;
}

static int shell_btmgr_change_source(int argc, char *argv[])
{
	if(argv[1] != NULL)	{
		bt_manager_user_api_switch_audio_source_transmitter(argv[1]);
	}
	return 0;
}

/*
static const struct shell_cmd bttrs_commands[] = {
	{ "trs_init", shell_bt_manager_trs_init, " bt_manager_trs_init" },
	{ "trs_deinit", shell_bt_manager_trs_deinit, "bt_manager_trs_deinit"},
	{ "trs_iquiry", shell_bt_manager_trs_inquiry, "bt_manager_trs_inquiry"},
	{ "trs_test", shell_bt_manager_test, "bt_manager_test"},
	{ "trs_connect", shell_bt_manager_trs_conn, "bt_manager_connected"},
	{ "tr_restart_bt", tr_shell_restart_bt_service, "tr_restart_bt mode appid" },
	{ "trs_get_pairlist", shell_bt_manager_get_pairlist, "shell_bt_manager_get_pairlist"},
	{ "trs_del_pairlist", shell_bt_manager_del_pairlist, "shell_bt_manager_del_pairlist"},
	{ "trs_disconnect", shell_bt_manager_disconnect, "shell_bt_manager_disconnect"},
	{ "trs_status", shell_bt_manager_status, "shell_bt_manager_status"},
	{ "trs_auto_stop", shell_bt_manager_auto_stop, "shell_bt_manager_auto_stop"},
	{ "trs_auto_start", shell_bt_manager_auto_start, "shell_bt_manager_auto_start"},
	{ "trs_get_vol", shell_bt_manager_get_vol, "shell_bt_manager_get_vol"},

    { "reason", shell_btmgr_disconnect_reason, "show btmgr status" },
    { "inquiry", shell_btmgr_inquiry, "show btmgr status" },
    { "reconnect", shell_btmgr_repdl, "show btmgr status" },
    { "delpdl", shell_btmgr_delpdl, "show btmgr status" },
    { "status", shell_btmgr_get_status, "show btmgr status" },
    { "changesource", shell_btmgr_change_source, "show btmgr status" },
    { NULL, NULL, NULL}
};*/


SHELL_STATIC_SUBCMD_SET_CREATE(acts_bttrs_cmds,
	SHELL_CMD(trs_init, NULL, "bt_manager_trs_init", shell_bt_manager_trs_init),
	SHELL_CMD(trs_deinit, NULL, "bt_manager_trs_deinit", shell_bt_manager_trs_deinit),
	SHELL_CMD(trs_iquiry, NULL, "bt_manager_trs_inquiry", shell_bt_manager_trs_inquiry),
	SHELL_CMD(trs_test, NULL, "bt_manager_test", shell_bt_manager_test),
	SHELL_CMD(trs_connect, NULL, "bt_manager_connected", shell_bt_manager_trs_conn),
	//SHELL_CMD(tr_restart_bt, NULL, "tr_restart_bt mode appid", tr_shell_restart_bt_service),
	SHELL_CMD(trs_get_pairlist, NULL, "shell_bt_manager_get_pairlist", shell_bt_manager_get_pairlist),
	SHELL_CMD(trs_del_pairlist, NULL, "shell_bt_manager_del_pairlist", shell_bt_manager_del_pairlist),

	SHELL_CMD(trs_disconnect, NULL, "shell_bt_manager_disconnect", shell_bt_manager_disconnect),
	SHELL_CMD(trs_status, NULL, "shell_bt_manager_status", shell_bt_manager_status),
	SHELL_CMD(trs_auto_stop, NULL, "shell_bt_manager_auto_stop", shell_bt_manager_auto_stop),
	SHELL_CMD(trs_auto_start, NULL, "shell_bt_manager_auto_start", shell_bt_manager_auto_start),
	SHELL_CMD(trs_get_vol, NULL, "shell_bt_manager_get_vol", shell_bt_manager_get_vol),
	SHELL_CMD(reason, NULL, "show btmgr status", shell_btmgr_disconnect_reason),

	SHELL_CMD(inquiry, NULL, "show btmgr status", shell_btmgr_inquiry),
	SHELL_CMD(reconnect, NULL, "show btmgr status", shell_btmgr_repdl),
	SHELL_CMD(delpdl, NULL, "show btmgr status", shell_btmgr_delpdl),
    SHELL_CMD(status, NULL, "show btmgr status", shell_btmgr_get_status),
    SHELL_CMD(changesource, NULL, "show btmgr status", shell_btmgr_change_source),

	SHELL_SUBCMD_SET_END
);

static int cmd_acts_bttrs(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_REGISTER(bttrs, &acts_bttrs_cmds, "bttrs Application shell commands", cmd_acts_bttrs);

//SHELL_REGISTER("bt_trs_btmanger", bttrs_commands);
