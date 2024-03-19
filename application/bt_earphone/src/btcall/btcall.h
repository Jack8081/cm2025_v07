/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt call 
 */

#ifndef _BT_CALL_APP_H_
#define _BT_CALL_APP_H_

#ifdef CONFIG_SYS_LOG
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "btcall"
#endif

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <volume_manager.h>
#include <media_player.h>
#include <audio_system.h>
#include <audiolcy_common.h>
#include <thread_timer.h>

#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"
#include "system_notify.h"


#ifdef CONFIG_BLUETOOTH
#include <bt_manager.h>
#include "btservice_api.h"
#endif

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
#include "leaudio.h"
#endif
#ifdef CONFIG_BT_TRANSCEIVER
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
#include <system_app.h>
#endif
#endif

enum BTCALL_APP_EVENT_ID
{
	PHONENUM_PRE_PLAY_EVENT,
	RING_REPEATE_EVENT,
};

typedef enum
{
    SWITCH_SOURCE_MODE_NONE,
    SWITCH_SOURCE_MODE_IN,
    SWITCH_SOURCE_MODE_OUT,
    SWITCH_SOURCE_MODE_OUT_STEP2,
    SWITCH_SOURCE_MODE_OUT_STEP3,
}switch_source_mode_e;

typedef enum
{
    BTACLL_UAC_GAIN_0,  /* whole origin volume */
    BTACLL_UAC_GAIN_1,  /* 1/2  origin volume  */
    BTACLL_UAC_GAIN_2,  /* 1/4  origin volume  */
    BTACLL_UAC_GAIN_3,  /* 1/8  origin volume  */
    BTACLL_UAC_GAIN_4,  /* 1/16 origin volume  */
    BTACLL_UAC_GAIN_5,  /* 1/32 origin volume  */

    BTACLL_UAC_GAIN_UNKNOWN,
} btcall_uac_gain_e;

struct btcall_app_t {
    media_player_t *player;
    struct thread_timer key_timer;
    struct thread_timer ring_timer;
    struct thread_timer sco_check_timer;
    CFG_Struct_Incoming_Call_Prompt incoming_config;
    uint8_t ring_phone_num[MAX_PHONENUM];
    uint32_t enable_tts_in_calling  : 1;
    uint32_t disable_voice  : 1;
    uint32_t tts_start_stop  : 1;
    uint32_t phone_num_play  : 1;
    uint32_t ring_playing  : 1;
    uint32_t mic_mute : 1;
    uint32_t need_resume_play:1;
    uint32_t key_handled : 1;
    uint32_t sco_established : 1;
    uint32_t need_resume_key : 1;
    uint32_t siri_mode : 1;
    uint32_t hfp_ongoing : 1;
    uint32_t upload_stream_outer : 1;
    uint32_t is_tts_locked : 1;
    uint32_t app_exiting : 1;
    uint32_t switch_source_mode:4;  //switch_source_mode_e
    uint32_t hfp_3way_status;
    io_stream_t bt_stream;
    io_stream_t upload_stream;
    struct bt_audio_chan bt_chan;
    int32_t last_disp_status;
    CFG_Struct_BTSpeech_User_Settings user_settings;
#ifdef CONFIG_BT_TRANSCEIVER
#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    const struct device *gpio_dev;
    os_delayed_work aux_plug_in_delay_work;
#endif

#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL 
    uint8_t uac_gain;
#endif
#endif
};

void btcall_bt_event_proc(struct app_msg *msg);
void btcall_input_event_proc(struct app_msg *msg);
void btcall_tts_event_proc(struct app_msg *msg);
void btcall_app_event_proc(struct app_msg *msg);
int btcall_tws_event_proc(struct app_msg *msg);
void btcall_start_play(void);
void btcall_stop_play(void);
int btcall_handle_enable(struct bt_audio_report *rep);
void btcall_media_mutex_stop(void);
void btcall_media_mutex_resume(void);
void btcall_tws_slave_key_ctrl(uint8_t key_func);
void btcall_key_func_comm_ctrl(uint8_t key_func);

struct btcall_app_t *btcall_get_app(void);

void btcall_phonenum_pre_play(void);
void btcall_ring_start(struct thread_timer *ttimer, void *phone_num);
void btcall_ring_repeat(void);
int btcall_ring_check(void);
void btcall_ring_stop(void);
int btcall_ring_init(void);
int btcall_ring_deinit(void);
void btcall_led_display(int hfp_status);
void btcall_view_init(bool key_remap);
void btcall_view_deinit(void);

void btcall_key_filter(void);
void _btcall_sco_check(struct thread_timer *ttimer, void *expiry_fn_arg);

#endif  // _BT_CALL_APP_H_


