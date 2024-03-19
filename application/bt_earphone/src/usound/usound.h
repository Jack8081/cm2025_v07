/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief usound function.
 */


#ifndef _USOUND_APP_H
#define _USOUND_APP_H

#ifdef CONFIG_SYS_LOG
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "usound"
#include <logging/sys_log.h>
#endif

#include <audiolcy_common.h>
#include <mem_manager.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <volume_manager.h>
#include <msg_manager.h>
#include <media_player.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <usb/class/usb_audio.h>
#include "usb_audio_inner.h"
#include "usb_hid_inner.h"
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <bt_manager.h>
#include <stream.h>
#include <property_manager.h>
#include <thread_timer.h>
#include <drivers/gpio.h>

#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"
#include "config_al.h"
#include "soc_pinmux.h"

#ifdef CONFIG_GUI
#include "../main/tr_app_view.h"
#include <api_common.h>
#endif

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
#include <dvfs.h>
#endif

#ifdef CONFIG_PLAYTTS
#include "tts_manager.h"
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

#define USOUND_VOLUME_LEVEL_MIN   (0)
#define USOUND_VOLUME_LEVEL_MAX   (16)

enum {
    MSG_USOUND_EVENT = MSG_APP_MESSAGE_START,
    MSG_SWITH_IOS,
};

enum {
    MSG_USOUND_STREAM_START = 1,
    MSG_USOUND_STREAM_STOP,
    MSG_USOUND_UPLOAD_STREAM_START,
    MSG_USOUND_UPLOAD_STREAM_STOP,
    MSG_USOUND_HOST_VOL_SYNC,
    MSG_USOUND_STREAM_MUTE,
    MSG_USOUND_STREAM_UNMUTE,
    MSG_USOUND_MUTE_SHORTCUT,

    MSG_USOUND_INCALL_RING,
    MSG_USOUND_INCALL_HOOK_UP,
    MSG_USOUND_INCALL_HOOK_HL,
    MSG_USOUND_INCALL_MUTE,
    MSG_USOUND_INCALL_UNMUTE,
    MSG_USOUND_INCALL_OUTGOING,
    MSG_USOUND_DEFINE_CMD_CONNECT_PAIRED_DEV,
};

enum USOUND_PLAY_STATUS {
    USOUND_STATUS_NULL      = 0x00,
    USOUND_STATUS_PLAYING   = 0x01,
    USOUND_STATUS_PAUSED    = 0x02,
};

enum USOUND_VOLUME_REQ_TYPE {
    USOUND_VOLUME_NONE      = 0x00,
    USOUND_VOLUME_DEC       = 0x01,
    USOUND_VOLUME_INC       = 0x02,
};

enum USOUND_MIC_CMD {
    MIC_CMD_NULL,
    MIC_CMD_STREAM_START,
    MIC_CMD_STREAM_STOP,
};

enum USOUND_PLAYBACK_REQ {
    PLAYBACK_REQ_NULL,
    PLAYBACK_REQ_STOP,
    PLAYBACK_REQ_START,
    PLAYBACK_REQ_RESTART,
};

enum USOUND_VOICE_STATE {
    VOICE_STATE_NULL,
    VOICE_STATE_INGOING,
    VOICE_STATE_OUTGOING,
    VOICE_STATE_ONGOING,
};

struct usound_app_t {
    struct thread_timer key_timer;
    struct thread_timer tts_monitor_timer;

    uint8_t     tts_playing             : 1;
    uint8_t     mic_mute                : 1;
    uint8_t     need_resume_key         : 1;
    uint8_t     capture_player_load     : 1;
    uint8_t     capture_player_run      : 1;
    uint8_t     playback_player_load    : 1;
    uint8_t     playback_player_run     : 1;

    media_player_t *playback_player;
    media_player_t *capture_player;

    uint8_t     dae_enalbe;
    uint8_t     cfg_dae_data[CFG_SIZE_BT_MUSIC_DAE_AL];
    uint8_t     multidae_enable;
    uint8_t     current_dae;
    uint8_t     dae_cfg_nums;
    uint8_t     curr_view_id;

    struct thread_timer host_vol_sync_timer;
    uint8_t     playing                 : 1;
    uint8_t     host_vol_sync           : 1;
    uint8_t     volume_req_type         : 2;
    uint8_t     host_vol_sync_tts       : 1;
    uint8_t     host_voice_state        : 2;
    uint8_t     host_hid_mic_mute       : 1;

    uint8_t     volume_req_level;
    uint8_t     current_volume_level;
    uint8_t     host_vol;
    uint8_t     host_mic_volume;
    uint8_t     host_mic_muted;

    io_stream_t usb_download_stream;
    io_stream_t usb_upload_stream;
};

int     iap2_protocol_init(void);
void    iap2_protocol_deinit(void);

void    usound_tts_event_proc(struct app_msg *msg);
void    usound_input_event_proc(struct app_msg *msg);
void    usound_view_init(uint8_t view_id, bool key_remap);
void    usound_view_deinit(void);
void    usound_event_notify(int event);

int     usound_capture_init(void);
int     usound_capture_start(void);
int     usound_capture_stop(void);
int     usound_capture_exit(void);

int     usound_playback_init(void);
int     usound_playback_start(void);
int     usound_playback_stop(void);
int     usound_playback_exit(void);

void    usound_event_proc(struct app_msg *msg);
void    usound_bt_event_proc(struct app_msg *msg);

struct usound_app_t *usound_get_app(void);

#endif  /* _USOUND_APP_H */

