/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app
 */

#ifndef _SYSTEM_NOTIFY_H_
#define _SYSTEM_NOTIFY_H_
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "config.h"
#include "app_ui.h"

typedef struct
{
    u8_t  set_led_display  : 1;
    u8_t  set_tone_play    : 1;
    u8_t  set_voice_play   : 1;
    
    u8_t  start_tone_play  : 1;
    u8_t  start_voice_play : 1;
    
    u8_t  end_led_display  : 1;
    u8_t  end_tone_play    : 1;
    u8_t  end_voice_play   : 1;

    u8_t  disable_tone     : 1;
    u8_t  disable_voice    : 1;

    u8_t  led_disp_only    : 1;
    u8_t  need_wait_end    : 1;
    u8_t  list_deleted     : 1;
    u8_t  is_tws_mode      : 1;

} system_notify_flags_t;


typedef struct
{
    bool   is_play_tone;
    u8_t   voice_list[1 + MAX_PHONENUM];
    int    voice_num;
} call_incoming_t;

typedef struct
{
    struct list_head  node;

    u8_t  layer_id;
    u8_t  app_id;
    u8_t  nesting;

    system_notify_flags_t  flags;
    unsigned int tag;

    call_incoming_t call_incoming;
    void  (*start_callback)(void);
    void  (*end_callback)(void);

    CFG_Type_Event_Notify  cfg_notify;
    
} system_notify_ctrl_t;

enum 
{
    START_PLAY_LED = 0,
    START_STOP_LED,
    START_PLAY_TONE,
    START_PLAY_VOICE
};

struct notify_sync
{
    u8_t  event_type;
    u8_t  event_stage;
    u64_t sync_time;
}__packed;

typedef struct notify_sync notify_sync_t;


struct notify_sync_phone_num
{
    u8_t  event_type;
    u8_t  event_stage;
    u64_t sync_time;
    u8_t  phone_num[1 + MAX_PHONENUM];
}__packed;

typedef struct notify_sync_phone_num notify_sync_phone_num_t;

CFG_Type_Event_Notify* system_notify_get_config(uint32_t event_id);
bool sys_check_notify_voice(uint32_t event_id);

/*!
 * \brief 是否允许播放提示音
 */
void sys_manager_enable_audio_tone(bool enable);

/*!
 * \brief 是否允许播放语音
 */
void sys_manager_enable_audio_voice(bool enable);

#endif //_SYSTEM_NOTIFY_H_
