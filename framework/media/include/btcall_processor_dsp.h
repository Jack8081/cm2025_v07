/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BTCALL_PROCESSOR_DSP_H__
#define __BTCALL_PROCESSOR_DSP_H__

#include "dsp_hal_defs.h"


typedef struct hfp_dae_para {
	short  pcm_debugmode;   //0:上传pcm, 1:不上传；2:清0
	short  hfp_connect_mod;// 0 没接通   1 接通
	short  change_flag;   //音效change标志
	short  mic_channel;       //   0: 左声道，   1: 右声道, 2: 双声道
	uint16_t pp_da_volume_l;
    uint16_t pp_da_volume_r;
	uint16_t  sample_rate;            //采样率，输入声音的采样率
	uint16_t  upstream_enc_enable:1;
	uint16_t  upstream_dae_enable:1;
	uint16_t  upstream_cng_enable:1;
    uint16_t  upstream_ai_enable:1;
    uint16_t  upstream_agc_enable:1;
    uint16_t  downstream_dae_enable:1;
    uint16_t  downstream_agc_enable:1;
    uint16_t  dual_mic_swap:1; //双mic数据交换位置
    uint16_t  channels:3; //声道数
    uint16_t  downstream_speaker_nr_enable:1;
    uint16_t  downstream_tail_nr_enable:1;
    uint16_t  reserve_bits:3;
    int32_t main_mic_gain;
	short  reserve[12];

    short  dae_para_info_array[256];
	short  aec_para[148];
    short  *dmvp_para;
    RINGBUF_TYPE ai_param_buf;
} hfp_dae_para_t;

#endif /* __BTCALL_PROCESSOR_DSP_H__ */
