/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mem_manager.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <volume_manager.h>
#include <msg_manager.h>
#include <thread_timer.h>
#include <media_player.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"

#include "ap_record_private.h"
#include "tts_manager.h"
#include "buffer_stream.h"
#include "ringbuff_stream.h"
#include "media_mem.h"

#define RECORD_SAMPLE_RATE_KH 16
static io_stream_t _record_create_uploadstream(record_stream_init_param *user_param)
{
	int ret = 0;
	io_stream_t upload_stream = NULL;

	upload_stream = record_upload_stream_create(user_param);
	if (!upload_stream) {
		goto exit;
	}

	ret = stream_open(upload_stream, MODE_IN_OUT);
	if (ret) {
		stream_destroy(upload_stream);
		upload_stream = NULL;
		goto exit;
	}

exit:
	SYS_LOG_INF(" %p\n", upload_stream);
	return upload_stream;
}

void record_start_record(record_stream_init_param *rec_param)
{
	struct record_app_t *record_app = record_get_app();
	media_init_param_t init_param;
	io_stream_t upload_stream = NULL;

	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE)
		return;

	if (!record_app)
		return;

	if (!rec_param)
		return;

	if (!rec_param->stream_send_cb)
		return;

	if (record_app->media_opened)
		return;

	if (record_app->playing)
		return;

	if (AP_TTS_STATUS_PLAY == ap_tts_status_get())
	{
		SYS_LOG_INF("%d", __LINE__);
		memcpy(&record_app->user_param, rec_param, sizeof(record_stream_init_param));
		return;
	}

	record_app->playing = TRUE;

#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(true);
#endif

	if (record_app->player) {
		SYS_LOG_INF(" already open\n");
		return;
	}

	memset(&init_param, 0, sizeof(media_init_param_t));

	upload_stream = _record_create_uploadstream(rec_param);

	if (!upload_stream) {
		goto err_exit;
	}

	init_param.type = MEDIA_SRV_TYPE_CAPTURE;
	init_param.stream_type = AUDIO_STREAM_ASR;
    init_param.efx_stream_type = AUDIO_STREAM_DEFAULT;
	init_param.support_tws = 0;

	init_param.capture_bit_rate = 16;
#ifdef CONFIG_TUYA_APP
	init_param.capture_bit_rate = 32;
#endif

	init_param.capture_format = OPUS_TYPE;
	init_param.capture_sample_rate_input = 16;
	init_param.capture_sample_rate_output = 16;
	init_param.capture_channels_input = 1;
	init_param.capture_channels_output = 1;
	init_param.capture_input_stream = NULL;
	init_param.capture_output_stream = upload_stream;

	record_app->player = media_player_open(&init_param);
	if (!record_app->player) {
		SYS_LOG_ERR("open failed\n");
		goto err_exit;
	}

	record_app->record_upload_stream = upload_stream;

	media_player_play(record_app->player);
	memcpy (&record_app->user_param, rec_param, sizeof(record_stream_init_param));
	SYS_LOG_INF("sucessed %p ", record_app->player);
	record_app->media_opened = TRUE;

	return;
err_exit:

	if (upload_stream) {
		stream_close(upload_stream);
		stream_destroy(upload_stream);
	}
	SYS_LOG_INF("failed\n");
}

void record_stop_record(void)
{
	struct record_app_t *record_app = record_get_app();

	if (!record_app)
	{
		SYS_LOG_INF("%d", __LINE__);
		return;
	}

	if (!record_app->player)
	{
		SYS_LOG_INF("%d", __LINE__);
		return;
	}

	if (!record_app->media_opened)
	{
		SYS_LOG_INF("%d", __LINE__);
		return;
	}

	record_app->media_opened = FALSE;

	if (record_app->record_upload_stream)
		stream_close(record_app->record_upload_stream);

	media_player_stop(record_app->player);

	media_player_close(record_app->player);

	SYS_LOG_INF(" %p ok\n", record_app->player);

	record_app->player = NULL;

	if (record_app->record_upload_stream) {
		stream_destroy(record_app->record_upload_stream);
		record_app->record_upload_stream = NULL;
	}
	record_app->playing = FALSE;
}

void record_slave_status_set(u8_t status)
{
	char *fg_app = app_manager_get_current_app();
	u8_t ap_type = 0;
	
	if (!fg_app)
		return;

	if (memcmp(fg_app, "ap_record", strlen("ap_record")) == 0)
		ap_type = 1;

	if ((status) && (0 == ap_type))
	{
		app_switch_add_app(APP_ID_AP_RECORD);
		app_switch(APP_ID_AP_RECORD, APP_SWITCH_NEXT, false);
	}
	else if ((ap_type) && (0 == status))
	{
		app_switch_add_app(APP_ID_BTMUSIC);
		app_switch(APP_ID_BTMUSIC, APP_SWITCH_NEXT, false);
	}
}

void record_ap_exec(record_stream_init_param *user_param)
{
	u8_t status = 1;
	char *fg_app = app_manager_get_current_app();
	u8_t ap_type = 0;
	SYS_LOG_INF("%d", __LINE__);
	if (!fg_app)
		return;

	if (memcmp(fg_app, "ap_record", strlen("ap_record")) == 0)
		ap_type = 1;

	bt_manager_tws_send_message
	(
		TWS_USER_APP_EVENT, TWS_EVENT_AP_RECORD_STATUS,
		&status, sizeof(u8_t)
	);

	/* start record after in push-to-talk*/
	if (0 == ap_type)
	{
		SYS_LOG_INF("%d", __LINE__);
		app_switch_add_app(APP_ID_AP_RECORD);
		app_switch(APP_ID_AP_RECORD, APP_SWITCH_NEXT, false);
	}

	record_start_record(user_param);
}

void record_ap_exit(void)
{
	u8_t status = 0;
	char *fg_app = app_manager_get_current_app();
	u8_t ap_type = 0;
	SYS_LOG_INF("%d", __LINE__);

	if (!fg_app)
		return;

	if (memcmp(fg_app, "ap_record", strlen("ap_record")) == 0)
		ap_type = 1;

	bt_manager_tws_send_message
	(
		TWS_USER_APP_EVENT, TWS_EVENT_AP_RECORD_STATUS,
		&status, sizeof(u8_t)
	);

	record_stop_record();

	if (ap_type)
	{
		SYS_LOG_INF("%d", __LINE__);
		app_switch_add_app(APP_ID_BTMUSIC);
		app_switch(APP_ID_BTMUSIC, APP_SWITCH_NEXT, false);
	}
}

