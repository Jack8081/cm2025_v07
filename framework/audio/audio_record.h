/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio record.
*/
#ifndef __AUDIO_RECORD_H__
#define __AUDIO_RECORD_H__

#include <audio_system.h>
#include <audio_policy.h>
#include <stream.h>

/**
 * @defgroup audio_record_apis Auido Record APIs
 * @{
 */

/**
 * @brief Create New Record Instance.
 *
 * This routine Create new record instance.
 *
 * @param stream_type stream type of Record.
 * @param ext_stream_type extra stream type of Record to deside which audio device to record
 * @param sample_rate_input sample rate of dac 
 * @param sample_rate_output output sample rate of audio record 
 * @param format capture format 16bit/ 24bit
 * @param audio_mode_and_audiopll_mode_and_sample_bits
 *          bit[0~3]: audio mode of recoder , mono or stereo;
 *          bit[4-5]: audiopll mode, 0-default,1-integer-N,2-fractional-N
 *          bit[6-7]: sample bits, 0-16bits, 1-24bits
 * @param outer_stream recode data output stream
 *
 * @return handle of new recorder
 */
#define AUDIO_MODE_SHIFT   (0)      /* NOTE: Please don't modify it. */
#define AUDIO_MODE_MASK    (0xf<<0) /* NOTE: Please don't modify it. */
#define AUDPLL_MODE_SHIFT  (4)
#define AUDPLL_MODE_MASK   (0x3<<4)
#define SAMPLE_BITS_SHIFT  (6)
#define SAMPLE_BITS_MASK   (0x3<<6)
#define NO_APS_BITS_SHIFT  (8)
#define NO_APS_BITS_MASK   (0x1<<8)
struct audio_record_t *audio_record_create(uint8_t stream_type, uint8_t ext_stream_type,
                                    int sample_rate_input, int sample_rate_output,
									uint8_t format, uint32_t audio_mode_and_audiopll_mode_and_sample_bits, void *outer_stream);

/**
 * @brief Destory a Record Instance.
 *
 * This routine Destory a Record Instance 
 *
 * @param handle handle of record
 *
 * @return 0 excute successed , others failed
 */
int audio_record_destory(struct audio_record_t *handle);
/**
 * @brief Start Audio Record Capture
 *
 * This routine Start audio record capture, enable adc and dma.
 *
 * @param handle handle of record
 *
 * @return 0 excute successed , others failed
 */
int audio_record_start(struct audio_record_t *handle);
/**
 * @brief Stop Audio Record Capture
 *
 * This routine Stop audio record capture, disable adc and stop dma.
 *
 * @param handle handle of record
 *
 * @return 0 excute successed , others failed
 */
int audio_record_stop(struct audio_record_t *handle);
/**
 * @brief Pause Audio Record Capture
 *
 * This routine pause audio record capture, drop the caputre date
 *
 * @param handle handle of record
 *
 * @return 0 excute successed , others failed
 */
int audio_record_pause(struct audio_record_t *handle);
/**
 * @brief Resume Audio Record Capture
 *
 * This routine Resume audio record capture, out stream continue output capture data
 *
 * @param handle handle of record
 *
 * @return 0 excute successed , others failed
 */
int audio_record_resume(struct audio_record_t *handle);
/**
 * @brief Read Data From Audio Record
 *
 * This routine Read Data From Audio Record, user cam also read from out stream
 *
 * @param handle handle of record
 * @param buff buffer pointer of store read out data
 * @param num number of bytes want to read
 *
 * @return len of read datas
 */
int audio_record_read(struct audio_record_t *handle, uint8_t *buff, int num);
/**
 * @brief flush Audio Record Capture
 *
 * This routine flush Audio Record Capture, wait capture data read finished.
 *
 * @param handle handle of record
 *
 * @return 0 excute successed , others failed
 */
int audio_record_flush(struct audio_record_t *handle);
/**
 * @brief set capture gain
 *
 * This routine set capture gain, adjust capture gain for record.
 *
 * @param handle handle of record
 * @param volume gain level for capture.
 *
 * @return 0 excute successed , others failed
 */
int audio_record_set_volume(struct audio_record_t *handle, audio_input_gain *input_gain);
/**
 * @brief get capture volume
 *
 * This routine get capture volume.
 *
 * @param handle handle of record
 *
 * @return value of capture gain
 */
int audio_record_get_volume(struct audio_record_t *handle);
/**
 * @brief set capture sample rate
 *
 * This routine set capture sample rate.
 *
 * @param handle handle of record
 * @param sample_rate sample rate for capture.
 *
 * @return 0 excute successed , others failed
 */
int audio_record_set_samplerate(struct audio_record_t *handle, int sample_rate);
/**
 * @brief get capture sample rate
 *
 * This routine get capture sample rate.
 *
 * @param handle handle of record
 *
 * @return value of capture sample rate
 */
int audio_record_get_samplerate(struct audio_record_t *handle);
/**
 * @brief Get Audio Record Stream
 *
 * This routine Get Audio Record Stream
 *
 * @param handle handle of record
 *
 * @return handle of audio record stream
 */
io_stream_t audio_record_get_stream(struct audio_record_t *handle);

/**
 * @brief Set DAC play flag.
 *
 * This routine set the DAC play flag, then audio record will no store pcm data util DAC start play.
 *
 * @param      handle handle of record
 * @play_flag  play flag pointer
 *
 * @return 0 excute successed , others failed
 */
int audio_record_set_play_flag(struct audio_record_t *handle, uint16_t *play_flag);

/**
 * @brief record aps monitor deal.
 *
 * This routine deal record aps monitor.
 *
 * @param      handle handle of record
 * @latency_offset    current latency offset base latency threshold
 *
 * @return 0 excute successed , others failed
 */
int audio_record_aps_monitor(struct audio_record_t *handle, int32_t latency_offset);
int audio_record_aps_monitor_lite(struct audio_record_t *handle, int32_t latency_offset);

/**
 * @brief audio record is low latency mode or not.
 *
 * This routine return if audio record is low latency mode.
 *
 * @param      handle handle of record
 *
 * @return 1 is low latency mode, 0 is not low latency mode
 */
int audio_record_is_low_latency(struct audio_record_t *handle);

void audio_record_aps_param_reset(void);

/**
 * @} end defgroup audio_record_apis
 */
#endif



