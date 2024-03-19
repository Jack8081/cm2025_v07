/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager.
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <bt_manager.h>
#include "bt_manager_inner.h"
#include <sys_event.h>
#include <btservice_api.h>
#include <shell/shell.h>
#include <acts_bluetooth/host_interface.h>
#include <property_manager.h>

static int get_auto_reconnect_phone_info(struct autoconn_info* info, uint8_t max_cnt)
{
	struct autoconn_info info_tmp[BT_MAX_AUTOCONN_DEV];
	int cnt, i;

	memset(info_tmp, 0, sizeof(info_tmp));
	cnt = btif_br_get_auto_reconnect_info(info_tmp, BT_MAX_AUTOCONN_DEV);
	if (cnt == 0) {
		return 0;
	}

	cnt = 0;
	for (i = 0; i < BT_MAX_AUTOCONN_DEV; i++) {

		if (cnt >= max_cnt){
			goto Exit;
		}

		if (info_tmp[i].addr_valid && info_tmp[i].tws_role == BTSRV_TWS_NONE) {

			if (cnt < max_cnt){
				memcpy((char *)&(info[cnt]), (char *)&(info_tmp[i]), (sizeof(struct autoconn_info)));
				cnt++;
			}
		}
	}

Exit:
	return cnt;
}

static int pts_connect_acl(const struct shell *shell, size_t argc, char *argv[])
{
	int cnt;
	struct autoconn_info info[1];

	memset(info, 0, sizeof(info));
	cnt = get_auto_reconnect_phone_info(info, 1);
	if (cnt == 0) {
		SYS_LOG_WRN("Never connect to pts dongle\n");
		return -1;
	}

	info[0].addr_valid = 1;
	info[0].tws_role = 0;
	info[0].a2dp = 0;
	info[0].hfp = 0;
	info[0].avrcp = 0;
	info[0].hfp_first = 0;
	btif_br_set_auto_reconnect_info(info, 1);

	btif_br_connect(&info[0].addr);
	return 0;
}

static int pts_connect_acl_a2dp_avrcp(const struct shell *shell, size_t argc, char *argv[])
{
	int cnt;
	struct autoconn_info info[1];

	memset(info, 0, sizeof(info));
#if defined(CONFIG_BT_BR_TRANSCEIVER) && defined(CONFIG_TR_BT_HOST_BQB) && defined(CONFIG_BT_TRANSCEIVER)
    bool bt_mgr_check_dev_aclconnected_a2dp_disconnect(void);
    if(bt_mgr_check_dev_aclconnected_a2dp_disconnect())
    {
        btif_a2dp_connect((sys_get_work_mode() == TRANSFER_MODE) ? 1 : 0, &info[0].addr);
    }
    cnt = btif_br_get_auto_reconnect_info(info, 1);
#else
	cnt = get_auto_reconnect_phone_info(info, 1);
#endif
	if (cnt == 0) {
		SYS_LOG_WRN("Never connect to pts dongle\n");
		return -1;
	}

	info[0].addr_valid = 1;
	info[0].tws_role = 0;
	info[0].a2dp = 1;
	info[0].hfp = 0;
	info[0].avrcp = 1;
	info[0].hfp_first = 0;
	btif_br_set_auto_reconnect_info(info, 1);
#ifdef CONFIG_BT_BR_TRANSCEIVER
        property_set(TR_CFG_AUTOCONN_INFO, (void *)info, sizeof(info));
        property_flush(NULL);
        tr_bt_manager_startup_reconnect();
#else
#ifdef CONFIG_PROPERTY
	property_flush(NULL);
#endif
#endif
	bt_manager_startup_reconnect();
	return 0;
}

static int pts_connect_acl_hfp(const struct shell *shell, size_t argc, char *argv[])
{
	int cnt;
	struct autoconn_info info[1];

	memset(info, 0, sizeof(info));
#ifdef CONFIG_BT_BR_TRANSCEIVER
    cnt = btif_br_get_auto_reconnect_info(info, 1);
#else
	cnt = get_auto_reconnect_phone_info(info, 1);
#endif
	if (cnt == 0) {
		SYS_LOG_WRN("Never connect to pts dongle\n");
		return -1;
	}

	info[0].addr_valid = 1;
	info[0].tws_role = 0;
	info[0].a2dp = 0;
	info[0].hfp = 1;
	info[0].avrcp = 0;
	info[0].hfp_first = 1;
	btif_br_set_auto_reconnect_info(info, 1);
#ifdef CONFIG_BT_BR_TRANSCEIVER
    property_set(TR_CFG_AUTOCONN_INFO, (void *)info, sizeof(info));
    property_flush(NULL);
    tr_bt_manager_startup_reconnect();
#else
#ifdef CONFIG_PROPERTY
	property_flush(NULL);
#endif
#endif
	bt_manager_startup_reconnect();
	return 0;
}

static int pts_hfp_cmd(const struct shell *shell, size_t argc, char *argv[])
{
	char *cmd;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts hfp_cmd xx");
		return -EINVAL;
	}

	cmd = argv[1];
	SYS_LOG_INF("hfp cmd:%s", cmd);

	/* AT cmd
	 * "ATA"			Answer call
	 * "AT+CHUP"		rejuet call
	 * "ATD1234567;"	Place a Call with the Phone Number Supplied by the HF.
	 * "ATD>1;"			Memory Dialing from the HF.
	 * "AT+BLDN"		Last Number Re-Dial from the HF.
	 * "AT+CHLD=0"	Releases all held calls or sets User Determined User Busy (UDUB) for a waiting call.
	 * "AT+CHLD=x"	refer HFP_v1.7.2.pdf.
	 * "AT+NREC=x"	Noise Reduction and Echo Canceling.
	 * "AT+BVRA=x"	Bluetooth Voice Recognition Activation.
	 * "AT+VTS=x"		Transmit DTMF Codes.
	 * "AT+VGS=x"		Volume Level Synchronization.
	 * "AT+VGM=x"		Volume Level Synchronization.
	 * "AT+CLCC"		List of Current Calls in AG.
	 * "AT+BTRH"		Query Response and Hold Status/Put an Incoming Call on Hold from HF.
	 * "AT+CNUM"		HF query the AG subscriber number.
	 * "AT+BIA="		Bluetooth Indicators Activation.
	 * "AT+COPS?"		Query currently selected Network operator.
	 */

	if (btif_pts_send_hfp_cmd(cmd)) {
		SYS_LOG_WRN("Not ready\n");
	}
	return 0;
}

static int pts_hfp_connect_sco(const struct shell *shell, size_t argc, char *argv[])
{
	if (btif_pts_hfp_active_connect_sco()) {
		SYS_LOG_WRN("Not ready\n");
	}

	return 0;
}

static int pts_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
#ifdef CONFIG_BT_BR_TRANSCEIVER
    bd_address_t bd;
    bt_dev_svr_info_t dev;
    memset(&dev, 0, sizeof(dev));

    api_bt_get_svr_status(NULL, &dev);
    if (dev.a2dp_connected || dev.avrcp_connected)
    {
        btif_avrcp_disconnect(&bd);
        btif_a2dp_disconnect(&bd);
    }
    else
    {
        btif_br_disconnect_device(BTSRV_DISCONNECT_ALL_MODE);
    }
#else
	btif_br_disconnect_device(BTSRV_DISCONNECT_ALL_MODE);
#endif
	return 0;
}

static int pts_a2dp_test(const struct shell *shell, size_t argc, char *argv[])
{
#ifdef CONFIG_BT_A2DP
	char *cmd;
	uint8_t value;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts a2dp xx");
		return -EINVAL;
	}

	cmd = argv[1];
	SYS_LOG_INF("a2dp cmd:%s", cmd);

	if (!strcmp(cmd, "delayreport")) {
		bt_manager_a2dp_send_delay_report(1000);	/* Delay report 100ms */
	} else if (!strcmp(cmd, "cfgerrcode")) {
		if (argc < 3) {
			SYS_LOG_INF("Used: pts a2dp cfgerrcode 0xXX");
		}

		value = strtoul(argv[2], NULL, 16);
		btif_pts_a2dp_set_err_code(value);
		SYS_LOG_INF("Set a2dp err code 0x%x", value);
	}
#endif

	return 0;
}

static int pts_avrcp_test(const struct shell *shell, size_t argc, char *argv[])
{
	char *cmd;
	uint8_t value;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts avrcp xx");
		return -EINVAL;
	}

	cmd = argv[1];
	SYS_LOG_INF("avrcp cmd:%s", cmd);

	if (!strcmp(cmd, "playstatus")) {
		btif_pts_avrcp_get_play_status();
	} else if (!strcmp(cmd, "pass")) {
		if (argc < 3) {
			SYS_LOG_INF("Used: pts avrcp pass 0xXX");
			return -EINVAL;
		}
		value = strtoul(argv[2], NULL, 16);
		btif_pts_avrcp_pass_through_cmd(value);
	} else if (!strcmp(cmd, "volume")) {
		if (argc < 3) {
			SYS_LOG_INF("Used: pts avrcp volume 0xXX");
			return -EINVAL;
		}
		value = strtoul(argv[2], NULL, 16);
		btif_pts_avrcp_notify_volume_change(value);
	} else if (!strcmp(cmd, "regnotify")) {
		btif_pts_avrcp_reg_notify_volume_change();
	} else if (!strcmp(cmd, "setvol")){
        value = atoi(argv[2]);
        btif_avrcp_set_absolute_volume(&value, 1);
    }

	return 0;
}

static uint8_t spp_chnnel;

static void pts_spp_connect_failed_cb(uint8_t channel)
{
	SYS_LOG_INF("channel:%d", channel);
	spp_chnnel = 0;
}

static void pts_spp_connected_cb(uint8_t channel, uint8_t *uuid)
{
	SYS_LOG_INF("channel:%d", channel);
	spp_chnnel = channel;
}

static void pts_spp_receive_data_cb(uint8_t channel, uint8_t *data, uint32_t len)
{
	SYS_LOG_INF("channel:%d data len %d", channel, len);
}

static void pts_spp_disconnected_cb(uint8_t channel)
{
	SYS_LOG_INF("channel:%d", channel);
	spp_chnnel = 0;
}

static const struct btmgr_spp_cb pts_spp_cb = {
	.connect_failed = pts_spp_connect_failed_cb,
	.connected = pts_spp_connected_cb,
	.disconnected = pts_spp_disconnected_cb,
	.receive_data = pts_spp_receive_data_cb,
};

static const uint8_t pts_spp_uuid[16] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, \
	0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x01, 0x11, 0x00, 0x00};

static void bt_pts_spp_connect(void)
{
	int cnt;
	struct autoconn_info info[3];

	memset(info, 0, sizeof(info));
	//cnt = btif_br_get_auto_reconnect_info(info, 1);
    cnt = get_auto_reconnect_phone_info(info, 1);

	if (cnt == 0) {
		SYS_LOG_WRN("Never connect to pts dongle\n");
		return;
	}

	spp_chnnel = bt_manager_spp_connect(&info[0].addr, (uint8_t *)pts_spp_uuid, (struct btmgr_spp_cb *)&pts_spp_cb);
	SYS_LOG_INF("channel:%d", spp_chnnel);
}

static int pts_spp_test(const struct shell *shell, size_t argc, char *argv[])
{
	char *cmd;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts spp xx");
		return -EINVAL;
	}

	cmd = argv[1];
	SYS_LOG_INF("spp cmd:%s", cmd);

	if (!strcmp(cmd, "register")) {
		bt_manager_spp_reg_uuid((uint8_t *)pts_spp_uuid, (struct btmgr_spp_cb *)&pts_spp_cb);
	} else if (!strcmp(cmd, "connect")) {
		bt_pts_spp_connect();
	} else if (!strcmp(cmd, "disconnect")) {
		if (spp_chnnel) {
			bt_manager_spp_disconnect(spp_chnnel);
		}
	}

	return 0;
}

static int pts_scan_test(const struct shell *shell, size_t argc, char *argv[])
{
	char *cmd;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts scan on/off");
		return -EINVAL;
	}

	cmd = argv[1];
	SYS_LOG_INF("scan cmd:%s", cmd);

	if (!strcmp(cmd, "on")) {
		bt_manager_set_user_visual(true,true,true,0);

	} else if (!strcmp(cmd, "off")) {
		bt_manager_set_user_visual(false,0,0,0);
	}

	return 0;
}

static int pts_clean_test(const struct shell *shell, size_t argc, char *argv[])
{
	char *cmd;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts clean xxx");
		return -EINVAL;
	}

	cmd = argv[1];
	SYS_LOG_INF("clean cmd:%s\n", cmd);

	if (!strcmp(cmd, "linkkey")) {
		btif_br_clean_linkkey();
	}

	return 0;
}

static int pts_auth_test(const struct shell *shell, size_t argc, char *argv[])
{
	char *cmd;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts auth xxx");
		return -EINVAL;
	}

	cmd = argv[1];
	SYS_LOG_INF("auth cmd:%s", cmd);

	if (!strcmp(cmd, "register")) {
		btif_pts_register_auth_cb(true);
	} else if (!strcmp(cmd, "unregister")) {
		btif_pts_register_auth_cb(false);
	}

	return 0;
}

#ifdef CONFIG_BT_TRANSCEIVER
static int ag_answer_incoming_call(const struct shell *shell, int argc, char *argv[])
{
    if (argc != 1)
        return -EINVAL;

    printk("run cmd: %s\n", argv[0]);

#ifdef CONFIG_BT_BR_TRANSCEIVER
    bt_manager_hfp_ag_answer_call();
#else
    bt_manager_hfp_accept_call();
#endif
    return 0;
}

static int pts_hfp_disconnect_sco(const struct shell *shell, int argc, char *argv[])
{
	btif_pts_hfp_active_disconnect_sco();
	return 0;
}

static int pts_hungup(const struct shell *shell, int argc, char *argv[])
{
#ifdef CONFIG_BT_BR_TRANSCEIVER
    bt_manager_hfp_ag_hungup();
#else
    bt_manager_hfp_hangup_call();
#endif
    return 0;
}

static int ag_reject_incoming_call(const struct shell *shell, int argc, char *argv[])
{
    if (argc != 1)
        return -EINVAL;

    printk("run cmd: %s\n", argv[0]);
#ifdef CONFIG_BT_BR_TRANSCEIVER
    bt_manager_hfp_ag_reject_call();
#else
    bt_manager_hfp_reject_call();
#endif
    return 0;
}

static int pts_call_out(const struct shell *shell, int argc, char *argv[])
{
#ifdef CONFIG_BT_BR_TRANSCEIVER
    bt_manager_hfp_ag_start_call_out();
#else
    bt_manager_hfp_dial_last_number();
#endif
    return 0;
}

static int ag_clean_pairlist(const struct shell *shell, int argc, char *argv[])
{
    if (argc != 1)
        return -EINVAL;

    printk("run cmd: %s\n", argv[0]);
#ifdef CONFIG_BT_BR_TRANSCEIVER
    api_bt_del_PDL(0xff);
    property_flush(NULL);
#else
#endif

    return 0;
}
#endif

#if (defined(CONFIG_BT_BLE_BQB) && defined(CONFIG_BT_TRANSCEIVER))
static int pts_ble_bas_notify(const struct shell *shell, int argc, char *argv[])
{
    u8_t bas_level;

    if (argc != 2)
        return -EINVAL;

    printk("run cmd: %s %s\n", argv[0], argv[1]);

    bas_level = atoi(argv[1]);
    ble_bas_notify(bas_level);
    return 0;
}

static int pts_ble_cts_notify(const struct shell *shell, int argc, char *argv[])
{
    u8_t cts_level;

    if (argc != 2)
        return -EINVAL;

    printk("run cmd: %s %s\n", argv[0], argv[1]);

    cts_level = atoi(argv[1]);
    ble_cts_notify(cts_level);
    return 0;
}
static int pts_ble_hts_notify(const struct shell *shell, int argc, char *argv[])
{
    u8_t hts_level;

    if (argc != 2)
        return -EINVAL;

    printk("run cmd: %s %s\n", argv[0], argv[1]);

    hts_level = atoi(argv[1]);
    ble_hts_notify(hts_level);
    return 0;
}

static int pts_ble_hrs_notify(const struct shell *shell, int argc, char *argv[])
{
    u8_t hrs_level;

    if (argc != 2)
        return -EINVAL;

    printk("run cmd: %s %s\n", argv[0], argv[1]);

    hrs_level = atoi(argv[1]);
    ble_hrs_notify(hrs_level);
    return 0;
}
#endif

#if (defined(CONFIG_BT_BR_TRANSCEIVER) && defined(CONFIG_BT_TRANSCEIVER))
static int avrcp_track_notify(const struct shell *shell, int argc, char *argv[])
{
    u8_t track;
    if (argc != 2)
        return -EINVAL;

    printk("run cmd: %s %s\n", argv[0], argv[1]);

    track = atoi(argv[1]);
    btif_avrcp_sync_track(track);
    return 0;
}

u8_t dst_full_name[21];
static void set_dst_full_name(u8_t *name)
{
    if(name)
    {
        u8_t len = strlen(name);
        len = len > 20 ? 20: len;
        memcpy(dst_full_name, name, len);
        dst_full_name[len] = 0;
    }
}

static void tr_pts_inquiry_ctrl_complete_cb(void * param, int dev_cnt)
{
	if(dev_cnt != 0 || param == NULL)
	{
		SYS_LOG_INF("inquiry result is NULL\n");
		return;
	}
	inquiry_info_t *dev_info = (inquiry_info_t *)param;

    if((memcmp(dev_info->name, dst_full_name, strlen(dst_full_name)) == 0) || (dev_info->rssi >= -35))
    {
        struct autoconn_info info[3];
        memset(info, 0, sizeof(info));
        info[0].addr_valid = 1;
        info[0].tws_role = 0;
        info[0].a2dp = 0;
        info[0].hfp = 1;
        info[0].avrcp = 0;
        info[0].hfp_first = 1;
        memcpy(info[0].addr.val, dev_info->addr, 6);

        property_set(TR_CFG_AUTOCONN_INFO, (void *)info, sizeof(info));
        property_flush(NULL);
        api_bt_inquiry_cancel();
        tr_bt_manager_startup_reconnect();
        printk("****************************************\n");
        printk("PTS name: %s\n", dev_info->name);
        printk("****************************************\n");
    }
}

static int pts_inquiry_and_connect(const struct shell *shell, int argc, char *argv[])
{
	char *cmd;

	if (argc < 2) {
		SYS_LOG_INF("Used: pts hfp_cmd xx");
		return -EINVAL;
	}

	cmd = argv[1];
	SYS_LOG_INF("hfp cmd:%s\n", cmd);
    set_dst_full_name(cmd);
    api_bt_inquiry(5, 0x2f, 0, tr_pts_inquiry_ctrl_complete_cb);

    return 0;
}

static int pts_ag_call_in(const struct shell *shell, int argc, char *argv[])
{
#ifdef CONFIG_BT_BR_TRANSCEIVER
    bt_manager_hfp_ag_start_call_in();
#endif
    return 0;
}

static int pts_ag_call_inband(const struct shell *shell, int argc, char *argv[])
{
#ifdef CONFIG_BT_BR_TRANSCEIVER
    bt_manager_hfp_ag_start_call_in_band();
#endif
    return 0;
}

static int pts_ag_call_in_2(const struct shell *shell, int argc, char *argv[])
{
#ifdef CONFIG_BT_BR_TRANSCEIVER
    bt_manager_hfp_ag_start_call_in_2();
#endif
    return 0;
}

static int pts_ag_call_in_3(const struct shell *shell, int argc, char *argv[])
{
#ifdef CONFIG_BT_BR_TRANSCEIVER
    bt_manager_hfp_ag_start_call_in_3();
#endif
    return 0;
}

static int ag_answer_incoming_call_2(const struct shell *shell, int argc, char *argv[])
{
    if (argc != 1)
        return -EINVAL;

    printk("run cmd: %s\n", argv[0]);

#ifdef CONFIG_BT_BR_TRANSCEIVER
    bt_manager_hfp_ag_answer_call_2();
#else
    bt_manager_hfp_accept_call();
#endif
    return 0;
}


static int ag_ciev_service(const struct shell *shell, int argc, char *argv[])
{
    u8_t service;

    if (argc != 2)
        return -EINVAL;

    printk("run cmd: %s %s\n", argv[0], argv[1]);

    service = atoi(argv[1]);
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_SERVICE_IND, service ? 1 : 0);
    return 0;
}

static int ag_ciev_signal(const struct shell *shell, int argc, char *argv[])
{
    u8_t signal;

    if (argc != 2)
        return -EINVAL;

    printk("run cmd: %s %s\n", argv[0], argv[1]);

    signal = atoi(argv[1]);
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_SINGNAL_IND, signal);
    return 0;
}

static int ag_ciev_battchg(const struct shell *shell, int argc, char *argv[])
{
    u8_t battery;

    if (argc != 2)
        return -EINVAL;

    printk("run cmd: %s %s\n", argv[0], argv[1]);

    battery = atoi(argv[1]);
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_BATTERY_IND, battery);
    return 0;
}

static int ag_clean_dial_memory(const struct shell *shell, int argc, char *argv[])
{
    if (argc != 1)
        return -EINVAL;

    printk("run cmd: %s\n", argv[0]);

    bt_manager_hfp_ag_dial_memory_clear();
    return 0;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(bt_mgr_pts_cmds,
	SHELL_CMD(connect_acl, NULL, "pts active connect acl", pts_connect_acl),
	SHELL_CMD(connect_acl_a2dp_avrcp, NULL, "pts active connect acl/a2dp/avrcp", pts_connect_acl_a2dp_avrcp),
	SHELL_CMD(connect_acl_hfp, NULL, "pts active connect acl/hfp", pts_connect_acl_hfp),
	SHELL_CMD(hfp_cmd, NULL, "pts hfp command", pts_hfp_cmd),
	SHELL_CMD(hfp_connect_sco, NULL, "pts hfp active connect sco", pts_hfp_connect_sco),
	SHELL_CMD(disconnect, NULL, "pts do disconnect", pts_disconnect),
	SHELL_CMD(a2dp, NULL, "pts a2dp test", pts_a2dp_test),
	SHELL_CMD(avrcp, NULL, "pts avrcp test", pts_avrcp_test),
	SHELL_CMD(spp, NULL, "pts spp test", pts_spp_test),
	SHELL_CMD(scan, NULL, "pts scan test", pts_scan_test),
	SHELL_CMD(clean, NULL, "pts scan test", pts_clean_test),
	SHELL_CMD(auth, NULL, "pts auth test", pts_auth_test),
#ifdef CONFIG_BT_TRANSCEIVER
	SHELL_CMD(answer, NULL, "answer", ag_answer_incoming_call),
	SHELL_CMD(hfp_disconn_sco, NULL, "pts hfp active disconnect sco", pts_hfp_disconnect_sco),
	SHELL_CMD(hungup, NULL, "pts hungup", pts_hungup),
	SHELL_CMD(reject, NULL, "reject", ag_reject_incoming_call),
	SHELL_CMD(call_out, NULL, "pts call out", pts_call_out),
	SHELL_CMD(clean_pairlist, NULL, "clean pairlist", ag_clean_pairlist),
#endif
#if (defined(CONFIG_BT_BLE_BQB) && defined(CONFIG_BT_TRANSCEIVER))
	SHELL_CMD(ble_bas_notify, NULL, "ble bas notify x", pts_ble_bas_notify),
	SHELL_CMD(ble_hts_notify, NULL, "ble hts indicate x", pts_ble_hts_notify),
	SHELL_CMD(ble_hrs_notify, NULL, "ble hrs indicate x", pts_ble_hrs_notify),
	SHELL_CMD(ble_cts_notify, NULL, "ble cts notify x", pts_ble_cts_notify),
#endif

#if (defined(CONFIG_BT_BR_TRANSCEIVER) && defined(CONFIG_BT_TRANSCEIVER))
    SHELL_CMD(inquiry_and_conn, NULL, "pts inquiry_and_con pts_name", pts_inquiry_and_connect),
    SHELL_CMD(ag_call_in, NULL, "pts call in", pts_ag_call_in),
    SHELL_CMD(ag_call_inband, NULL, "pts call ring inband", pts_ag_call_inband),
    SHELL_CMD(ag_call_in_2, NULL, "pts call in 2", pts_ag_call_in_2),
    SHELL_CMD(ag_call_in_3, NULL, "pts call in 3", pts_ag_call_in_3),
    SHELL_CMD(answer_2, NULL, "answer 2", ag_answer_incoming_call_2),
    SHELL_CMD(ciev_serv, NULL, "ciev_serv x", ag_ciev_service),
    SHELL_CMD(ciev_signal, NULL, "ciev_signal x", ag_ciev_signal),
    SHELL_CMD(ciev_battch, NULL, "ciev_battch x", ag_ciev_battchg),
    SHELL_CMD(clean_dial_memory, NULL, "clean dial memory", ag_clean_dial_memory),
    SHELL_CMD(track_notify, NULL, "pts track_notify x", avrcp_track_notify),
#endif

	SHELL_SUBCMD_SET_END
);

static int cmd_bt_pts(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_REGISTER(pts, &bt_mgr_pts_cmds, "Bluetooth manager pts test shell commands", cmd_bt_pts);
