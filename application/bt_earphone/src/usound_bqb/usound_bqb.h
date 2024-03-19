/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USOUND_APP_H
#define _USOUND_APP_H

#ifdef CONFIG_SYS_LOG
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "leaudio"
#include <logging/sys_log.h>
#endif

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
#include <bt_manager.h>
#include <stream.h>
#include <sys_manager.h>
//#include <buffer_stream.h>
//#include <ringbuff_stream.h>
#include <media_mem.h>

#include <thread_timer.h>

#include "app_defines.h"
#include "app_ui.h"

#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif

enum {
	MSG_USOUND_EVENT = MSG_APP_MESSAGE_START,
};

enum {
	MSG_USOUND_STREAM_START = 1,
	MSG_USOUND_STREAM_STOP,
	MSG_USOUND_UPLOAD_STREAM_START,
	MSG_USOUND_UPLOAD_STREAM_STOP,
	MSG_USOUND_STREAM_VOLUME,
	MSG_USOUND_STREAM_MUTE,
	MSG_USOUND_STREAM_UNMUTE,
	MSG_USOUND_UPLOAD_STREAM_VOLUME,
	MSG_USOUND_UPLOAD_STREAM_MUTE,
	MSG_USOUND_UPLOAD_STREAM_UNMUTE,
};

struct usound_app_t {
	struct bt_audio_qoss_1 qoss;
 
	uint16_t handle;

	/*
	 * if usb sink start/stop is pending or new commands come
	 * when the last command is pending still.
	 */
	uint32_t usb_sink_pending : 1;
	uint32_t usb_sink_pending_start : 1;

	uint32_t usb_sink_started : 1;	/* usb audio sink started */
	uint32_t bt_source_configured : 1;	/* bt audio source configured */
	uint32_t bt_source_enabled : 1;	/* bt audio source enabled */
	uint32_t bt_source_started : 1;	/* bt audio source started */
	uint32_t usb_to_bt_started : 1;

	/*
	 * if usb source start/stop is pending or new commands come
	 * when the last command is pending still.
	 */
	uint32_t usb_source_pending : 1;
	uint32_t usb_source_pending_start : 1;

	uint32_t usb_source_started : 1; /* usb audio source started */
	uint32_t bt_sink_configured : 1; /* bt audio sink configured */
	uint32_t bt_sink_enabled : 1;	/* bt audio sink enabled */
	uint32_t bt_to_usb_started : 1;

	uint32_t capture_player_load : 1;
	uint32_t capture_player_run : 1;

	uint32_t playback_player_load : 1;
	uint32_t playback_player_run : 1;

	uint32_t bt_volume_connected : 1;
	uint32_t bt_volume_pending : 1;
	uint32_t bt_mute_pending : 1;

	uint32_t bt_mic_connected : 1;
	uint32_t bt_mic_gain_pending : 1;
	uint32_t bt_mic_mute_pending : 1;

	/* pending volume & mic */
	uint8_t bt_volume;
	uint8_t bt_mute;
	uint8_t bt_mic_gain;
	uint8_t bt_mic_mute;

	/* Audio Source: usound_stream -> source_stream -> BT */
	media_player_t *capture_player;
	io_stream_t usound_stream;
	io_stream_t source_stream;
	struct bt_audio_chan source_chan;

	/* Audio Sink: BT -> sink_stream -> usound_upload_stream */
	media_player_t *playback_player;
	io_stream_t usound_upload_stream;
	io_stream_t sink_stream;
	struct bt_audio_chan sink_chan;

	uint8_t *data;
};

struct usound_app_t *usound_get_app(void);
void usound_bt_event_proc(struct app_msg *msg);
void usound_event_proc(struct app_msg *msg);

int usound_init_capture(void);
int usound_start_capture(void);
int usound_stop_capture(void);
int usound_exit_capture(void);
int usound_init_playback(void);
int usound_start_playback(void);
int usound_stop_playback(void);
int usound_exit_playback(void);

#endif  /* _USOUND_APP_H */

