/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common utils for in/out audio drivers
 */
#ifndef __AUDIO_ACTS_UTILS_H__
#define __AUDIO_ACTS_UTILS_H__

#include <drivers/audio/audio_common.h>
#include <shell/shell_uart.h>

#define AUDIOPLL_APS_FRACTIONAL_MAX 31

/*
 * enum a_pll_series_e
 * @brief The series of audio pll
 */
typedef enum {
	AUDIOPLL_44KSR = 0, /* 44.1K sample rate seires */
	AUDIOPLL_48KSR /* 48K sample rate series */
} a_pll_series_e;

/*
 * enum a_pll_type_e
 * @brief The audio pll type selection
 */
typedef enum {
	AUDIOPLL_TYPE_0 = 0, /* AUDIO_PLL0 */
	AUDIOHOSC,                /* HOSC */
} a_pll_type_e;

/*
 * enum a_mclk_type_e
 * @brief The rate of MCLK in the multiple of sample rate
 * @note DAC MCLK is always 256FS, and the I2S MCLK depends on BCLK (MCLK = 4BCLK)
 */
typedef enum {
    MCLK_128FS = 128,
    MCLK_256FS = 256,
    MCLK_512FS = 512,
	MCLK_768FS = 768,
	MCLK_1536FS = 1536,
} a_mclk_type_e;

uint32_t audio_sr_khz_to_hz(audio_sr_sel_e sr_khz);
audio_sr_sel_e audio_sr_hz_to_Khz(uint32_t sr_hz);
int audio_get_pll_setting(audio_sr_sel_e sr_khz, uint8_t *clk_div, uint8_t *series);
int audio_get_pll_setting_i2s(uint16_t sr_khz, a_mclk_type_e mclk,
	uint8_t *div, uint8_t *series);
int audio_pll_check_config(a_pll_series_e series, uint8_t *index, a_pll_mode_e pll_mode);
int audio_get_pll_sample_rate(uint8_t clk_div, a_pll_type_e index);
int audio_get_pll_sample_rate_i2s(a_mclk_type_e mclk, uint8_t clk_div, a_pll_type_e index);
int audio_pll_get_aps(a_pll_type_e index);
int audio_pll_get_aps_f(a_pll_type_e index);
int audio_pll_set_aps(a_pll_type_e index, audio_aps_level_e level);
int audio_pll_set_aps_f(a_pll_type_e index, unsigned int level);


#endif /* __AUDIO_ACTS_UTILS_H__ */
