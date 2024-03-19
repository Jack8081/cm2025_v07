/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "usound_bqb.h"

static void usound_handle_disconnect(uint16_t *handle)
{
	struct usound_app_t *usound = usound_get_app();

	usound->bt_source_configured = 0;
	usound->bt_source_enabled = 0;
	usound->bt_source_started = 0;
	usound->usb_to_bt_started = 0;

	usound->bt_sink_configured = 0;
	usound->bt_sink_enabled = 0;
	usound->bt_to_usb_started = 0;

	usound->usb_source_pending = 0;
	usound->usb_sink_pending = 0;

	usound->bt_volume_connected = 0;
	usound->bt_mic_connected = 0;

	bt_manager_audio_stream_bandwidth_free(NULL);

	usound_get_app()->handle = 0;

	memset(&usound->source_chan, 0, sizeof(struct bt_audio_chan));
	memset(&usound->sink_chan, 0, sizeof(struct bt_audio_chan));
	memset(&usound->qoss, 0, sizeof(struct bt_audio_qoss_1));
}

static int usound_handle_connected(uint16_t *handle)
{
	usound_get_app()->handle = *handle;

	return 0;
}

void usound_bt_event_proc(struct app_msg *msg)
{
	SYS_LOG_INF("cmd: %d\n", msg->cmd);

	switch (msg->cmd) {
	case BT_CONNECTED:
		usound_handle_connected(msg->ptr);
		break;
	case BT_DISCONNECTED:
		usound_handle_disconnect(msg->ptr);
		break;

	default:
		break;
	}
}
