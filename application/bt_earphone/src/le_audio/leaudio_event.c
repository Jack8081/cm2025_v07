/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief le audio app event.
 */

#include "leaudio.h"


typedef struct
{
	uint32_t enbale_share_info : 1;
	uint32_t dump_crc_error_pkt : 1;
	uint32_t recv_pkt_size : 10;
	uint32_t send_pkt_size : 10;
	uint32_t recv_pkt_triggle_enable : 1;
	uint32_t send_pkt_triggle_enable : 1;
	uint32_t recv_pkt_report_enable : 1;
	uint32_t send_pkt_report_enable : 1;

	uint16_t sink_handle;
	uint16_t source_handle;


	int32_t tx_anchor_offset_us;
	int32_t rx_anchor_offset_us;

	struct bt_dsp_cis_share_info *cis_share_info;
} cis_xfer_bypass_config_t;

cis_xfer_bypass_config_t cis_xfer_bypass_cfg __in_section_unique(DSP_BT_SHARE_RAM);

void leaudio_set_share_info(bool enable, bool update)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();

	if (update) {
		if (leaudio->set_share_info_flag == 1 && enable) {
			SYS_LOG_INF("update\n");
		} else {
			return;
		}
	} else {
		if (leaudio->set_share_info_flag == 1 && enable) {
			SYS_LOG_INF("already\n");
			return;
		}
	}

	if (!enable) {
		if (thread_timer_is_running(&leaudio->delay_play_timer)) {
			thread_timer_stop(&leaudio->delay_play_timer);
		}
	}

	SYS_LOG_INF("enable %d, 0x%p", enable, leaudio->cis_share_info);
	cis_xfer_bypass_cfg.enbale_share_info = enable;
	if (enable) {
		SYS_LOG_INF("cis_share_info %p,%p,%p,%p", 
            leaudio->cis_share_info->recvbuf, 
            leaudio->cis_share_info->recv_report_buf,
            leaudio->cis_share_info->sendbuf,
			leaudio->cis_share_info->send_report_buf);
		
		cis_xfer_bypass_cfg.cis_share_info = leaudio->cis_share_info;
		cis_xfer_bypass_cfg.dump_crc_error_pkt = CONFIG_DUMP_CRC_ERROR_PKT;
		cis_xfer_bypass_cfg.recv_pkt_size = MAX_RECV_PKT_SIZE;
		cis_xfer_bypass_cfg.send_pkt_size = MAX_SEND_PKT_SIZE;
		cis_xfer_bypass_cfg.recv_pkt_triggle_enable = 1;
		cis_xfer_bypass_cfg.send_pkt_triggle_enable = 0;
		if (!leaudio->playback_player_run && leaudio->capture_player_run)
			cis_xfer_bypass_cfg.send_pkt_triggle_enable = 1;
		if (leaudio->playback_player_run)
			cis_xfer_bypass_cfg.recv_pkt_report_enable = 1;
		else
			cis_xfer_bypass_cfg.recv_pkt_report_enable = 0;
		if (leaudio->capture_player_run)
			cis_xfer_bypass_cfg.send_pkt_report_enable = 1;
		else
			cis_xfer_bypass_cfg.send_pkt_report_enable = 0;

        SYS_LOG_INF("sink_audio_handle 0x%x, source_audio_handle 0x%x, send triggle %d",
            leaudio->sink_audio_handle,
            leaudio->source_audio_handle,
            cis_xfer_bypass_cfg.send_pkt_triggle_enable);

        cis_xfer_bypass_cfg.sink_handle = leaudio->sink_audio_handle;
        cis_xfer_bypass_cfg.source_handle = leaudio->source_audio_handle;
	}

	bt_manager_send_mailbox_sync(0x01, (uint8_t *)&cis_xfer_bypass_cfg, sizeof(cis_xfer_bypass_cfg));

	if (enable)
		leaudio->set_share_info_flag = 1;
	else
		leaudio->set_share_info_flag = 0;
}

static void leaudio_set_share_info_delay_play_proc(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	SYS_LOG_INF("exec");
	leaudio_set_share_info(true, false);
}

void leaudio_restore(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();
	struct bt_audio_chan *source_chan = &leaudio->source_chan;
	struct bt_audio_chan *sink_chan = &leaudio->sink_chan;
	int source_state, sink_state;

	SYS_LOG_INF("source_chan->handle:%d, sink_chan->handle:%d",source_chan->handle, sink_chan->handle);


	/* released or disconnected */
	if (!source_chan->handle && !sink_chan->handle) {
		return;
	}

	source_state = bt_manager_audio_stream_state(source_chan->handle,
					source_chan->id);
	sink_state = bt_manager_audio_stream_state(sink_chan->handle,
					sink_chan->id);

	/* both channels are stopped */
	if (source_state < BT_AUDIO_STREAM_ENABLED &&
	    sink_state < BT_AUDIO_STREAM_ENABLED) {
		return;
	}

	SYS_LOG_INF("\n");

	leaudio_init_capture();
	leaudio_init_playback();

	leaudio_set_share_info(true, false);

	if (sink_state == BT_AUDIO_STREAM_ENABLED) {
		leaudio_start_playback();
		leaudio->bt_sink_enabled = 1;
		bt_manager_audio_sink_stream_start_single(sink_chan);
	}

	if (source_state == BT_AUDIO_STREAM_STARTED) {
		leaudio_start_capture();
		leaudio->bt_source_started = 1;
	}

	if (sink_state == BT_AUDIO_STREAM_STARTED
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
           && !le_audio_is_in_background()
#endif
        ) {
		leaudio_start_playback();
	}
}

#if 0
static void leaudio_tx_start(uint16_t handle, uint8_t id, uint16_t audio_handle)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();

	if (leaudio->adc_to_bt_started) {
		return;
	}

	if (leaudio->bt_source_started) {
		SYS_LOG_INF("adc_to_bt_started");
		leaudio->adc_to_bt_started = 1;
	}
}

static void leaudio_rx_start(uint16_t handle, uint8_t id, uint16_t audio_handle)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();

	if (leaudio->bt_to_dac_started) {
		return;
	}

	if (leaudio->bt_sink_enabled) {
		SYS_LOG_INF("bt_to_dac_started");
		leaudio->bt_to_dac_started = 1;
	}
}
#endif

static int leaudio_handle_config_qos(struct bt_audio_qos_configured *qos)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();
	struct bt_audio_chan *chan;
	int dir;

	dir = bt_manager_audio_stream_dir(qos->handle, qos->id);

	if (dir == BT_AUDIO_DIR_SINK) {
		chan = &leaudio->sink_chan;
		chan->handle = qos->handle;
		chan->id = qos->id;
#if 0
		bt_manager_audio_stream_cb_register(chan, leaudio_rx_start,
						NULL);
#endif
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		chan = &leaudio->source_chan;
		chan->handle = qos->handle;
		chan->id = qos->id;
#if 0
		bt_manager_audio_stream_cb_register(chan, leaudio_tx_start,
						NULL);
#endif
	}

	SYS_LOG_INF("handle: %d, id: %d, dir: %d\n", qos->handle,
		qos->id, dir);

	return 0;
}

static void leaudio_handle_enable_share_info(struct leaudio_app_t *leaudio)
{
	uint32_t remain_period = 0;
	if (bt_manager_audio_get_recently_connectd(&remain_period)) {
		if (thread_timer_is_running(&leaudio->delay_play_timer)) {
			thread_timer_stop(&leaudio->delay_play_timer);
		}
		thread_timer_init(&leaudio->delay_play_timer, leaudio_set_share_info_delay_play_proc, NULL);
		thread_timer_start(&leaudio->delay_play_timer, remain_period, 0);
		SYS_LOG_INF("delay_play_proc set %d", remain_period);
	} else {
		leaudio_set_share_info(true, false);
	}
}

static int leaudio_handle_enable(struct bt_audio_report *rep)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();
	struct bt_audio_chan *source_chan = &leaudio->source_chan;
	struct bt_audio_chan *sink_chan = &leaudio->sink_chan;

	if (leaudio->tts_playing) {
		SYS_LOG_INF("tts\n");
		return 0;
	}

	/* context switch : call or others/music */
	SYS_LOG_INF("contexts %d", rep->audio_contexts);
	if (rep->audio_contexts & BT_AUDIO_CONTEXT_CALL) {
		SYS_LOG_INF("le audio call");
		leaudio_view_switch(BTCALL_VIEW);
	} else {
		SYS_LOG_INF("le audio music");
		leaudio_view_switch(BTMUSIC_VIEW);
	}

	if ((rep->handle == sink_chan->handle) &&
		(rep->id == sink_chan->id)) {
		SYS_LOG_INF("sink\n");
		leaudio_init_playback();
		leaudio_init_capture();

		leaudio_handle_enable_share_info(leaudio);

		/* start playback earlier */
		leaudio_start_playback();
		leaudio->bt_sink_enabled = 1;

		bt_manager_audio_sink_stream_start_single(sink_chan);
		return 0;
	}

	if ((rep->handle == source_chan->handle) &&
		(rep->id == source_chan->id)) {
		SYS_LOG_INF("source\n");
		leaudio_init_capture();
		leaudio_init_playback();

		leaudio_handle_enable_share_info(leaudio);

		return 0;
	}

	return -EINVAL;
}

static int leaudio_handle_start(struct bt_audio_report *rep)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();
	struct bt_audio_chan *source_chan = &leaudio->source_chan;

	if (leaudio->tts_playing) {
		SYS_LOG_INF("tts\n");
		return 0;
	}

	if ((rep->handle == source_chan->handle) &&
		(rep->id == source_chan->id)) {
		leaudio_start_capture();
		leaudio->bt_source_started = 1;
		return 0;
	}

	return 0;
}

static int leaudio_handle_stop(struct bt_audio_report *rep)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();
	struct bt_audio_chan *source_chan = &leaudio->source_chan;
	struct bt_audio_chan *sink_chan = &leaudio->sink_chan;

	if ((rep->handle == sink_chan->handle) &&
		(rep->id == sink_chan->id)) {
		leaudio_stop_playback();
	}

	if ((rep->handle == source_chan->handle) &&
		(rep->id == source_chan->id)) {
		leaudio->bt_source_started = 0;
		leaudio_stop_capture();
		return 0;
	}

	if (!leaudio->playback_player || !leaudio->playback_player_run) {
		SYS_LOG_INF("back to le audio music");
		leaudio_view_switch(BTMUSIC_VIEW);
	}

	return 0;
}

static int leaudio_handle_disable(struct bt_audio_report *rep)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();
	struct bt_audio_chan *sink_chan = &leaudio->sink_chan;

	if ((rep->handle == sink_chan->handle) &&
		(rep->id == sink_chan->id)) {
		leaudio->bt_sink_enabled = 0;
		bt_manager_audio_sink_stream_stop_single(sink_chan);
	}

	return 0;
}

static int leaudio_handle_release(struct bt_audio_report *rep)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();
	struct bt_audio_chan *source_chan = &leaudio->source_chan;
	struct bt_audio_chan *sink_chan = &leaudio->sink_chan;

	if ((rep->handle == sink_chan->handle) &&
		(rep->id == sink_chan->id)) {
		leaudio->bt_sink_enabled = 0;
		leaudio_stop_playback();
	}

	if ((rep->handle == source_chan->handle) &&
		(rep->id == source_chan->id)) {
		leaudio->bt_source_started = 0;
		leaudio_stop_capture();
	}

	if ((!leaudio->playback_player || !leaudio->playback_player_run)
		&& (!leaudio->capture_player || !leaudio->capture_player_run)) {
		leaudio_set_share_info(false, false);
		leaudio_exit_playback();
		leaudio_exit_capture();
	}

	if (!leaudio->playback_player || !leaudio->playback_player_run) {
		SYS_LOG_INF("back to le audio music");
		leaudio_view_switch(BTMUSIC_VIEW);
	}

	return 0;
}

static int leaudio_restore_state(struct bt_audio_chan *chan)
{
	struct bt_audio_chan_info info;
	struct bt_audio_report rep;
	int state;
	int ret;

	ret = bt_manager_audio_stream_info(chan, &info);
	if (ret < 0) {
		return ret;
	}

	state = bt_manager_audio_stream_state(chan->handle, chan->id);
	if (state < 0) {
		return ret;
	}

	switch (state) {
	case BT_AUDIO_STREAM_ENABLED:
		rep.handle = chan->handle;
		rep.id = chan->id;
		rep.audio_contexts = info.contexts;
		leaudio_handle_enable(&rep);
		break;
	case BT_AUDIO_STREAM_STARTED:
		rep.handle = chan->handle;
		rep.id = chan->id;
		rep.audio_contexts = info.contexts;
		/* FIXME: sink_chan no need to stream_start again! */
		leaudio_handle_enable(&rep);
		leaudio_handle_start(&rep);
		break;
	default:
		break;
	}

	return 0;
}

static int leaudio_handle_restore(struct bt_audio_chan_restore *res)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();
	struct bt_audio_chan *chan;
	struct bt_audio_chan_info info;	
	uint8_t source, sink;
	uint8_t i;
	int dir;

	if (!res) {
		SYS_LOG_ERR("NULL");
		return -EINVAL;
	}

	/* NOTICE: should be 0/1/2 */
	if (res->count > 2) {
		SYS_LOG_WRN("count: %d", res->count);
	}

	SYS_LOG_INF("\n");

	leaudio->bt_sink_enabled = 0;
	leaudio_stop_playback();

	leaudio->bt_source_started = 0;
	leaudio_stop_capture();

	if ((!leaudio->playback_player || !leaudio->playback_player_run)
		&& (!leaudio->capture_player || !leaudio->capture_player_run)) {
		leaudio_set_share_info(false, false);
		leaudio_exit_playback();
		leaudio_exit_capture();
	}

	leaudio->adc_to_bt_started = 0;
	leaudio->bt_to_dac_started = 0;

	memset(&leaudio->source_chan, 0, sizeof(struct bt_audio_chan));
	memset(&leaudio->sink_chan, 0, sizeof(struct bt_audio_chan));

	source = sink = 0;

	for (i = 0; i < res->count; i++) {
		chan = &res->chans[i];

		dir = bt_manager_audio_stream_dir(chan->handle, chan->id);
		bt_manager_audio_stream_info(chan, &info);

		if (dir == BT_AUDIO_DIR_SINK) {
			sink = 1;
			leaudio->sink_chan.handle = chan->handle;
			leaudio->sink_chan.id = chan->id;
			leaudio->sink_audio_handle = info.audio_handle;
			SYS_LOG_INF("sink_audio_handle :%d", leaudio->sink_audio_handle );

		} else if (dir == BT_AUDIO_DIR_SOURCE) {
			source = 1;
			leaudio->source_chan.handle = chan->handle;
			leaudio->source_chan.id = chan->id;
			leaudio->source_audio_handle = info.audio_handle;
			SYS_LOG_INF("source_audio_handle :%d", leaudio->source_audio_handle );
			
		}

		SYS_LOG_INF("handle: %d, id: %d, dir: %d\n", chan->handle,
			chan->id, dir);
	}

	/* wait for tts complete */
	if (leaudio->tts_playing) {
		SYS_LOG_INF("tts\n");
		return 0;
	}

	if (source == 1) {
		SYS_LOG_INF("source\n");
		leaudio_restore_state(&leaudio->source_chan);
	}

	if (sink == 1) {
		SYS_LOG_INF("sink\n");
		leaudio_restore_state(&leaudio->sink_chan);
	}

	SYS_LOG_INF("done\n");

	return 0;
}

static int leaudio_handle_switch(void)
{
	return bt_manager_audio_stream_restore(BT_TYPE_LE);
}

static void leaudio_handle_connect(uint16_t *handle)
{
	uint8_t type;
	uint8_t role;

	/*
	 * validity check
	 */
	type = bt_manager_audio_get_type(*handle);
	if (type != BT_TYPE_LE) {
		SYS_LOG_ERR("type: %d\n", type);
		return;
	}

	role = bt_manager_audio_get_role(*handle);
	if (role != BT_ROLE_SLAVE) {
		SYS_LOG_ERR("role: %d\n", role);
		return;
	}

#ifdef CONFIG_LE_AUDIO_SR_BQB
    leaudio_get_app()->handle = *handle;
#endif

}

static void leaudio_handle_disconnect(uint16_t *handle)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();
	uint16_t le_handle;

	/*
	 * validity check: filter other connections
	 */
	if (leaudio->source_chan.handle != 0) {
		le_handle = leaudio->source_chan.handle;
	} else if (leaudio->sink_chan.handle != 0) {
		le_handle = leaudio->sink_chan.handle;
	} else {
		SYS_LOG_WRN("empty\n");
		return;
	}

	if (*handle != le_handle) {
		SYS_LOG_WRN("skip %d %d\n", *handle, le_handle);
		return;
	}

	leaudio->bt_source_started = 0;
	leaudio->adc_to_bt_started = 0;

	leaudio->bt_sink_enabled = 0;
	leaudio->bt_to_dac_started = 0;

	memset(&leaudio->source_chan, 0, sizeof(struct bt_audio_chan));
	memset(&leaudio->sink_chan, 0, sizeof(struct bt_audio_chan));
}

#ifndef CONFIG_LE_AUDIO_DOUBLE_SOUND_CARD
static int leaudio_handle_volume_value(struct bt_volume_report *rep)
{
	struct leaudio_app_t* leaudio = leaudio_get_app();
	int new_volume = rep->value;
	int old_volume;
	int stream_type = AUDIO_STREAM_LE_AUDIO;

	if (leaudio->curr_view_id == BTMUSIC_VIEW){
		stream_type = AUDIO_STREAM_LE_AUDIO_MUSIC;
	}

	old_volume = system_volume_get(stream_type);
    SYS_LOG_INF("volume:%d, %d",new_volume, old_volume);

	system_player_volume_set(stream_type, new_volume);
	new_volume = system_volume_get(stream_type);			

	if (new_volume == old_volume)
		return 0;

	if (rep->from_phone){
		if(new_volume >= audio_policy_get_max_volume(stream_type))
		{
			leaudio_event_notify(SYS_EVENT_MAX_VOLUME);
		}
		else if(new_volume <= audio_policy_get_min_volume(stream_type))
		{
			leaudio_event_notify(SYS_EVENT_MIN_VOLUME);
		}

	}

	return 0;
}
#endif

static int leaudio_handle_volume_mute(struct bt_volume_report *rep)
{
	SYS_LOG_INF("\n");

	return audio_system_set_stream_mute(AUDIO_STREAM_LE_AUDIO_MUSIC, true);
}

static int leaudio_handle_volume_unmute(struct bt_volume_report *rep)
{
	SYS_LOG_INF("\n");

	return audio_system_set_stream_mute(AUDIO_STREAM_LE_AUDIO_MUSIC, false);
}

static int leaudio_handle_mic_mute(struct bt_mic_report *rep)
{
	SYS_LOG_INF("\n");

	return audio_system_mute_microphone(1);
}

static int leaudio_handle_mic_unmute(struct bt_mic_report *rep)
{
	SYS_LOG_INF("\n");

	return audio_system_mute_microphone(0);
}

void leaudio_bt_event_proc(struct app_msg *msg)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();

	SYS_LOG_INF("%d\n", msg->cmd);

	switch (msg->cmd) {
	case BT_CONNECTED:
		leaudio_handle_connect(msg->ptr);
		break;
	case BT_DISCONNECTED:
		leaudio_handle_disconnect(msg->ptr);
		break;
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
	case BT_AUDIO_DISCONNECTED:
		if(le_audio_is_in_background() && strcmp(app_manager_get_current_app(), APP_ID_BTCALL) == 0) {
			le_audio_background_exit();
			return;
		}
		break;
#endif
	case BT_AUDIO_STREAM_CONFIG_QOS:
		leaudio_handle_config_qos(msg->ptr);
		break;
	case BT_AUDIO_STREAM_ENABLE:
		leaudio_handle_enable(msg->ptr);
		break;
	case BT_AUDIO_STREAM_DISABLE:
		leaudio_handle_disable(msg->ptr);
		break;
	case BT_AUDIO_STREAM_START:
		leaudio_handle_start(msg->ptr);
		break;
	case BT_AUDIO_STREAM_STOP:
		leaudio_handle_stop(msg->ptr);
		break;
	case BT_AUDIO_STREAM_RELEASE:
		leaudio_handle_release(msg->ptr);
		break;
	case BT_AUDIO_STREAM_RESTORE:
		leaudio_handle_restore(msg->ptr);
		break;

	case BT_AUDIO_STREAM_PRE_STOP:
		leaudio_pre_stop_playback();
		break;

	case BT_VOLUME_VALUE:
#ifndef CONFIG_LE_AUDIO_DOUBLE_SOUND_CARD
		leaudio_handle_volume_value(msg->ptr);
#endif
		break;
	case BT_VOLUME_MUTE:
		leaudio_handle_volume_mute(msg->ptr);
		break;
	case BT_VOLUME_UNMUTE:
		leaudio_handle_volume_unmute(msg->ptr);
		break;
	case BT_MIC_MUTE:
		leaudio_handle_mic_mute(msg->ptr);
		break;
	case BT_MIC_UNMUTE:
		leaudio_handle_mic_unmute(msg->ptr);
		break;

	case BT_AUDIO_SWITCH:
		leaudio_handle_switch();
		break;

	case BT_REQ_RESTART_PLAY:
		leaudio_stop_playback();
		leaudio_stop_capture();

		if ((!leaudio->playback_player || !leaudio->playback_player_run)
			&& (!leaudio->capture_player || !leaudio->capture_player_run)) {
			leaudio_set_share_info(false, false);
			leaudio_exit_playback();
			leaudio_exit_capture();
		}

		leaudio_restore(NULL, NULL);
		break;

	default:
		break;
	}

	if (BTMUSIC_VIEW == leaudio->curr_view_id)
		leaudio_view_display();
	else
		leaudio_call_view_display();
}

void leaudio_tts_event_proc(struct app_msg *msg)
{
	struct leaudio_app_t *leaudio = leaudio_get_app();

	switch (msg->value) {
	case TTS_EVENT_START_PLAY:
		leaudio->tts_playing = 1;
		if (thread_timer_is_running(&leaudio->monitor_timer)) {
			thread_timer_stop(&leaudio->monitor_timer);
		}

		leaudio_stop_playback();
		leaudio_stop_capture();

		if ((!leaudio->playback_player || !leaudio->playback_player_run)
			&& (!leaudio->capture_player || !leaudio->capture_player_run)) {
			leaudio_set_share_info(false, false);
			leaudio_exit_playback();
			leaudio_exit_capture();
		}

		break;
	case TTS_EVENT_STOP_PLAY:
		leaudio->tts_playing = 0;
		if (thread_timer_is_running(&leaudio->monitor_timer)) {
			thread_timer_stop(&leaudio->monitor_timer);
		}
		thread_timer_init(&leaudio->monitor_timer, leaudio_restore, NULL);
		thread_timer_start(&leaudio->monitor_timer, 200, 0);
		break;
	}
}

