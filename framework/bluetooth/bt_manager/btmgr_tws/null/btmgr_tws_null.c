/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager tws null porting.
 */
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <bt_manager.h>
#include <sys_event.h>
#include "bt_manager_inner.h"
#include "btservice_api.h"

void bt_manager_tws_send_event(uint8_t event, uint32_t event_param)
{
}

void bt_manager_tws_send_event_sync(uint8_t event, uint32_t event_param)
{
}

void bt_manager_tws_notify_start_play(uint8_t media_type, uint8_t codec_id, uint8_t sample_rate)
{
}

void bt_manager_tws_notify_stop_play(void)
{
}

int bt_manager_tws_send_command(uint8_t *command, int command_len)
{
	return 0;
}

int bt_manager_tws_send_sync_command(uint8_t *command, int command_len)
{
	return 0;
}

void bt_manager_tws_sync_volume_to_slave(bd_address_t* bt_addr,uint32_t media_type, uint32_t music_vol)
{
}

void bt_manager_tws_cancel_wait_pair(void)
{
}

void bt_manager_tws_pair_search(void)
{
}

void bt_manager_tws_disconnect_and_wait_pair(void)
{
}

tws_runtime_observer_t *bt_manager_tws_get_runtime_observer(void)
{
	return NULL;
}

void bt_manager_tws_init(void)
{
}

int bt_manager_tws_get_dev_role(void)
{
	return BTSRV_TWS_NONE;
}

uint8_t bt_manager_tws_get_peer_version(void)
{
	return 0;
}

bool bt_manager_tws_check_is_woodpecker(void)
{
	return false;
}

bool bt_manager_tws_check_support_feature(uint32_t feature)
{
	return true;
}

void bt_manager_tws_set_stream_type(uint8_t stream_type)
{
}

void bt_manager_tws_set_codec(uint8_t codec)
{
}

void bt_manager_tws_channel_mode_switch(void)
{
}

void btmgr_tws_switch_snoop_link(void)
{
}
