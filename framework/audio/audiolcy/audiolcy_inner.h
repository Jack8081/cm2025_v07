/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file audio latency header
 */

#ifndef AUDIOLCY_H
#define AUDIOLCY_H

#define SYS_LOG_NO_NEWLINE

#ifdef  SYS_LOG_DOMAIN
#undef  SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "audio latency"

#include <stdlib.h>

#include <property_manager.h>
#include "media_player.h"
#include "bluetooth_tws_observer.h"

#include <audio_system.h>
#include <audio_policy.h>
#include <audiolcy_common.h>

#define PROPERTY_LATENCY_MODE        "LCY_MODE"

#define ALLM_INSTEAD_OF_LLM   (1)  // use ALLM instead of LLM if BM_Use_ALLM enabled

typedef enum {
    LCYSYNC_UPDATE = 1,  // update new target latency
    LCYSYNC_SAVE,        // don't change current target latency and save it if needed
    LCYSYNC_REPEAT,      // send twssync pkt once more, not used

    LCYSYNC_MAX,         // followers are reserved for other usage

} lcysync_mode_e;

typedef struct {
    audiolcy_cfg_t cfg;
    u8_t  cur_lcy_mode;
    u8_t  lcymode_switch_hold;  // holding now, next switch to the 1st mode

} audiolcy_global_t;

// no more than 16bytes
typedef struct {
    u16_t latency;    // new latency value in ms
    u8_t  mode;       // lcysync_mode_e

    u8_t  adjust;     // 1:drop data, 2:insert data
    u32_t samples;    // the samples to be inserted or dropped
    u16_t pkt_num;    // the pkt to start inserting or dropping datas

    u8_t  reserved[2];

} audiolcy_lcysync_t;

typedef struct {
    audiolcy_cfg_t *cfg;

    u8_t  media_format;
    u8_t  stream_type;
    u8_t  sample_rate_khz;
    u8_t  twsrole;          // 0xFF: unknown yet

} audiolcy_common_t;


typedef struct {
    audiolcy_common_t lcycommon;
    void *media_handle;     // media_player_t
    void *dsp_handle;
    void *lcyapt;           // ALLM handle, must init and run as long as Use_ALLM enabled
    void *lcymplc;          // music frame interpolation, enhance hearing when slight staccato

    u8_t  lcy_mode;     // it might be different from global->cur_lcy_mode
    u8_t  lcymode_switch_forbidden;

    u16_t max_pkt_len;

    u8_t  reserve;

    u8_t  lcysync_cb_set;       // 1: lcysync cb set successfully
    u8_t  lcy_send_repeat;      // times that lcy sync pkt need to send, <= 2
    u8_t  lcy_recv_repeat;      // times that lcy sync pkt has received, <= 1
    u16_t lcy_send_pktnum;      // the last pkt num that lcy sync
    audiolcy_lcysync_t lcysync; // backup for repeat

} audiolcy_ctrl_t;


audiolcy_ctrl_t *get_audiolcyctrl(void);


void audiolcy_ctrl_init(audiolcy_init_param_t *param);
void audiolcy_ctrl_deinit(u8_t stream_type, u8_t media_format);
void audiolcy_ctrl_main(void);

void audiolcy_ctrl_set_pktinfo(audiolcy_pktinfo_t *pktinfo);
void audiolcy_ctrl_set_twsrole(u8_t role);
void audiolcy_ctrl_set_start(void);
void audiolcy_ctrl_get_fixedcost(u32_t *fixed);

u32_t audiolcy_ctrl_latency_change(u16_t lcy_ms);
void audiolcy_ctrl_drop_insert_data(u32_t new_lcy_ms);
void audiolcy_ctrl_latency_twssync(u8_t mode, u16_t lcy_ms);

// ALLM interface
void audiolcyapt_global_set_invalid(void);
void audiolcyapt_info_collect(void *handle, audiolcy_pktinfo_t *pktinfo);
u32_t audiolcyapt_latency_adjust(void *handle, u8_t adjust_en, u8_t sync_en);
u32_t audiolcyapt_latency_adjust_slave(void *handle, u16_t lcy_ms, u8_t mode);
u16_t audiolcyapt_latency_get(void *handle);

void audiolcy_adaptive_start_play(void *handle);
void audiolcy_adaptive_twsrole_change(void *handle, u8_t old_role, u8_t new_role);
void audiolcy_adaptive_main(void *handle);
void *audiolcy_adatpive_init(audiolcy_common_t *lcycommon);
void audiolcy_adaptive_deinit(void *handle);

// Music PLC interface
void *audiolcy_musicplc_init(audiolcy_common_t *lcycommon, void *dsp_handle);
void audiolcy_musicplc_deinit(void *handle);

#endif  // AUDIOLCY_H

