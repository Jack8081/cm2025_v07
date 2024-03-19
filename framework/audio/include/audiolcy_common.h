/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file audio latency common header
 */

#ifndef _AUDIOLCY_COMMON_H_
#define _AUDIOLCY_COMMON_H_

#include <os_common_api.h>


typedef enum {
    LCYMODE_NORMAL = 0,     // Normal Latency Mode (NLM)
    LCYMODE_LOW,            // Low Latency Mode (LLM)
    LCYMODE_ADAPTIVE,       // Adaptive Low Latency Mode (ALLM)

    LCYMODE_END,

} lcymode_e;

typedef enum {
    LCYCMD_INIT = 1,
    LCYCMD_DEINIT,
    LCYCMD_SET_TWSROLE,
    LCYCMD_SET_PKTINFO,
    LCYCMD_SET_START,
    LCYCMD_GET_FIXED,
    LCYCMD_APT_INVALID,
    LCYCMD_APT_SYNCLCY,
    LCYCMD_MAIN,
} lcycmd_e;


typedef struct {
    u8_t  Default_Low_Latency_Mode;   // YES: switch to (A)LLM as soon as powered on
    u8_t  Save_Latency_Mode;    // YES: save it to nvram when latency mode is changed

    u8_t  BM_Use_ALLM;          // YES: enable Adaptive Low Latency Mode(ALLM)
    u8_t  BM_ALLM_Factor;       // 0 ~ 5 level, adaptive latency would be heighten due to this value
    u8_t  BM_ALLM_Upper;        // 110 ~ 180ms, the maximum latency of ALLM

    u8_t  BM_PLC_Mode;          // music interpolation
    u8_t  BM_AAC_Decode_Delay_Optimize;

    u8_t  BS_Latency_Optimization;
    u8_t  BS_Use_Limiter_Not_AGC;
    u8_t  BS_PLC_NoDelay;       // enable no delay PLC algorithm to reduce about 7ms latency

    u16_t ALLM_recommend_period_s;    // unit second, the period of recommending a adaptive latency
    u32_t ALLM_scc_ignore_period_ms;  // unit ms, period to check staccato occured times
    u8_t  ALLM_scc_ignore_count;      // times of staccato in a period is less than this value, ignore it
    u8_t  ALLM_scc_clear_step;        // unit ms, the step to clear the latency increment by staccato
    u16_t ALLM_scc_clear_period_s;    // unit second, period to check and clear the increment by staccato
    u16_t ALLM_down_timer_period_s;   // unit second, the time to wait before down adjust the latency

} audiolcy_cfg_t;


typedef struct {
    u8_t  media_fromat;     // SBC_TYPE, AAC_TYPE, MSBC_TYPE, CVSD_TYPE
    u8_t  stream_type;      // AUDIO_STREAM_MUSIC, AUDIO_STREAM_VOICE
    u8_t  sample_rate_khz;
    u8_t  reserved;
    void *media_handle;     // media_player_t
    void *dsp_handle;

} audiolcy_init_param_t;


typedef struct {
    u16_t seq_no;
    u16_t pkt_len:12;
    u16_t frame_cnt:4;

    u32_t sysclk_cyc;    // received time of system cycle

} audiolcy_pktinfo_t;

/*!
 * \brief: initial latency configuration as soon as powered on
 * \n return latency mode
 */
u8_t audiolcy_config_init(audiolcy_cfg_t *lcy);

/*!
 * \brief: get latency mode
 */
u8_t audiolcy_get_latency_mode(void);

/*!
 * \brief: check if latency mode
 * \n 1: is (adaptive) low latency mode, 
 * \n 0: is normal latency mode
 */
u8_t audiolcy_is_low_latency_mode(void);

/*!
 * \brief: check the next lcy_mode would be switched to
 * \n return the next lcy_mode without switching
 */
u8_t audiolcy_check_latency_mode_switch_next(void);

/*!
 * \brief: set latency mode, check_switch_next before setting is suggested
 */
void audiolcy_set_latency_mode(u8_t mode, u8_t checked);

/*!
 * \brief: get latency threshold in current latency mode
 */
u32_t audiolcy_get_latency_threshold(u8_t format);
u32_t audiolcy_get_latency_threshold_min(u8_t format);

/*!
 * LCYCMD_INIT: audiolcy_init_param_t
 *      Initialize latency ctrl when playback opened for music or call
 *      Do it earlier than anyone use latency mode or threshold
 *
 * LCYCMD_DEINIT: param[0]=stream_type, param[1]=media_format
 *      Deinit latency ctrl when playback closed (with stream_type matched)
 *      Check media_format instead if stream_type=0
 *      Set both stream_type and media_format to 0 to deinit unconditionally
 *
 * LCYCMD_SET_TWSROLE: new role
 *
 * LCYCMD_SET_PKTINFO: audiolcy_pktinfo_t
 *      Newest received pktinfo, please called as soon as possible
 *
 * LCYCMD_SET_START: none
 *      Indicate palyback started
 *
 * LCYCMD_GET_FIXED: uint32_t *fixed_cost
 *      Get fixed cost part of delay to calc runtime latency
 *
 * LCYCMD_APT_INVALID: none
 *      Change ALLM global latency to invalid
 *
 * LCYCMD_APT_SYNCLCY: none
 *      ALLM, sync current latency to the other tws device
 *
 * LCYCMD_MAIN: none
 *      Main routine for audiolcy_ctrl, and it would decide what to do itself
 */
void audiolcy_cmd_entry(lcycmd_e cmd, void *param, u32_t size);

/*!
 * \brief: 
 * audiolcy_cmd_entry(LCYCMD_DEINIT, )
 */
void audiolcy_cmd_exit(u8_t stream_type, u8_t media_format);

#endif  // _AUDIOLCY_COMMON_H_

