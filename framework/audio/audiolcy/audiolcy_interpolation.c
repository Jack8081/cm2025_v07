/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file music interpolation(mPLC) interface
 * Ensure enough data for continuous playing by inserting some data when it is closely empty
 */

#include "audiolcy_inner.h"
#include "dsp_hal.h"


#define RESUME_FRAME_COUNT          300
#define PAUSE_FRAME_COUNT           16  // 16 continuous empty frames(2 AAC pkt or 16 SBC frames), pause mplc and restart media

#define PERIOD_MAX_FRAME_COUNT      200 // mplc empty frame statistics period
#define PERIOD_PAUSE_FRAME_COUNT    (PAUSE_FRAME_COUNT * 3)

// one frame decode time cost
#define CODEC_SBC_MONO_FRAME_RUN_US      360
#define CODEC_SBC_STEREO_FRAME_RUN_US    380

#define CODEC_AAC_MONO_FRAME_RUN_US      2750
#define CODEC_AAC_STEREO_FRAME_RUN_US    3500

// postprocess time cost
#define AUDIOPP_128_MONO_FRAME_RUN_US    170
#define AUDIOPP_128_STEREO_FRAME_RUN_US  340

// interrupt & thread time cost
#define SYSTEM_INTERRUPT_SBC_TIME_US     1000
#define SYSTEM_INTERRUPT_AAC_TIME_US     1000

// cache miss time cost, maximum
#define CACHE_MISS_SBC_TIMER_US          500
#define CACHE_MISS_AAC_TIMER_US          500

// min pcmbuf guarantee
#define PCMBUF_MIN_LEVEL_TIME_US         500


typedef struct decoder_dspfunc_mplc_params  mplcdsp_param_t;

typedef struct {
    audiolcy_common_t *lcycommon;
    void *dsp_handle;

    mplcdsp_param_t dsp_param;

} audiolcy_musicplc_t;


static uint32_t audiolcymplc_check_threshold(audiolcy_musicplc_t *lcymplc)
{
    audiolcy_common_t *lcycommon = lcymplc->lcycommon;

#if 0  // not sure yet
    uint32_t enough_samples, enough_time_us = 0;

    //stereo, single device
    if (lcycommon->twsrole == 0)
    {
        if (lcycommon->media_format == SBC_TYPE) {
            enough_time_us += CODEC_SBC_STEREO_FRAME_RUN_US;
        } else {
            enough_time_us += CODEC_AAC_STEREO_FRAME_RUN_US;
        }

        enough_time_us += AUDIOPP_128_STEREO_FRAME_RUN_US;
    }
    //mono, tws device
    else
    {
        if (lcycommon->media_format == SBC_TYPE) {
            enough_time_us += CODEC_SBC_MONO_FRAME_RUN_US;
        } else {
            enough_time_us += CODEC_AAC_MONO_FRAME_RUN_US;
        }

        enough_time_us += AUDIOPP_128_MONO_FRAME_RUN_US;
    }

    if (lcycommon->media_format == SBC_TYPE) {
        enough_time_us += SYSTEM_INTERRUPT_SBC_TIME_US;
        enough_time_us += CACHE_MISS_SBC_TIMER_US;
    } else {
        enough_time_us += SYSTEM_INTERRUPT_AAC_TIME_US;
        enough_time_us += CACHE_MISS_AAC_TIMER_US;
    }

    enough_time_us += PCMBUF_MIN_LEVEL_TIME_US;
    enough_samples = (enough_time_us + 1000) * lcycommon->sample_rate_khz;

    return enough_samples;
#else
	// temporarily used
	if (lcycommon->media_format == SBC_TYPE) {
		return 6 * lcycommon->sample_rate_khz;
	} else {
    	return 9 * lcycommon->sample_rate_khz;
	}
#endif
}


void audiolcymplc_dsp_config(audiolcy_musicplc_t *lcymplc, mplcdsp_param_t *param)
{
    // tell dsp to enable or disable mplc
    if (lcymplc->dsp_handle == NULL) {
        return ;
    }

    printk("lcymplc config dsp %d,%d_%d, %d_%d_%d_%d\n", param->mode, param->samples_th1, param->samples_th2,\
        param->thres_pause, param->thres_resume, param->thres_pemframe, param->thres_period);

    dsp_session_config_func(
        lcymplc->dsp_handle, DSP_FUNCTION_DECODER, DSP_CONFIG_DECODER_MPLC, sizeof(mplcdsp_param_t), param
    );
}


void *audiolcy_musicplc_init(audiolcy_common_t *lcycommon, void *dsp_handle)
{
    audiolcy_musicplc_t *lcymplc = NULL;

    // mplc is available only when LLM or ALLM
    if (!audiolcy_is_low_latency_mode()) {
        return NULL;
    }

    // mplc is available only when SBC music or AAC music
    if ((lcycommon == NULL) || (lcycommon->cfg->BM_PLC_Mode == 0) \
     || (lcycommon->stream_type  != AUDIO_STREAM_MUSIC) \
     || (lcycommon->media_format != SBC_TYPE && lcycommon->media_format != AAC_TYPE))
    {
        return NULL;
    }

    lcymplc = mem_malloc(sizeof(audiolcy_musicplc_t));
    if (lcymplc == NULL) {
        return NULL;
    }
    memset(lcymplc, 0, sizeof(audiolcy_musicplc_t));
    lcymplc->lcycommon = lcycommon;
    lcymplc->dsp_handle = dsp_handle;

    // enable dsp mplc
    lcymplc->dsp_param.mode = lcycommon->cfg->BM_PLC_Mode;
    lcymplc->dsp_param.thres_pause = PAUSE_FRAME_COUNT;
    lcymplc->dsp_param.thres_pemframe = PERIOD_PAUSE_FRAME_COUNT;
    lcymplc->dsp_param.thres_period = PERIOD_MAX_FRAME_COUNT;
    lcymplc->dsp_param.thres_resume = RESUME_FRAME_COUNT;
    lcymplc->dsp_param.samples_th1 = audiolcymplc_check_threshold(lcymplc);
    lcymplc->dsp_param.samples_th2 = lcymplc->dsp_param.samples_th1 + 24;
    audiolcymplc_dsp_config(lcymplc, &(lcymplc->dsp_param));

    SYS_LOG_INF("ok");

    return lcymplc;
}

void audiolcy_musicplc_deinit(void *handle)
{
    audiolcy_musicplc_t *lcymplc = (audiolcy_musicplc_t *)handle;

    if (lcymplc) {
        // disable dsp mplc
        lcymplc->dsp_param.mode = 0;
        audiolcymplc_dsp_config(lcymplc, &(lcymplc->dsp_param));

        memset(lcymplc, 0, sizeof(audiolcy_musicplc_t));
        mem_free(lcymplc);
        SYS_LOG_INF("done");
    }
}


