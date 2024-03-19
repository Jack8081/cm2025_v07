/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "ap_record_private.h"
#include "tts_manager.h"

void record_tts_event_proc(struct app_msg *msg)
{
	struct record_app_t *record_app = record_get_app();

	if (!record_app)
		return;

	switch (msg->value) {
	case TTS_EVENT_START_PLAY:
		ap_tts_status_set(AP_TTS_STATUS_PLAY);
		if (record_app->player) {
			record_stop_record();
		}
		break;
	case TTS_EVENT_STOP_PLAY:
		ap_tts_status_set(AP_TTS_STATUS_NONE);
		//if (record_app->player) {
			record_start_record(&record_app->user_param);
		//}
		break;
	default:
		break;
	}
}

void record_bt_event_proc(struct app_msg *msg)
{
	struct record_app_t *record_app = record_get_app();
	u8_t status = 1;

	if (!record_app)
		return;

	switch (msg->cmd) {
	case BT_TWS_CONNECTION_EVENT:
		bt_manager_tws_send_message
		(
			TWS_USER_APP_EVENT, TWS_EVENT_AP_RECORD_STATUS,
			&status, sizeof(u8_t)
		);
		break;

	default:
		break;
	}
}


