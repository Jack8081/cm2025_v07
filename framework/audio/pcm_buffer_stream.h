/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file pcm buffer stream interface
 */

#ifndef __PCM_BUFFER_STREAM_H__
#define __PCM_BUFFER_STREAM_H__

#include <stream.h>

/**
 * @defgroup pcm_buffer_stream_apis Buffer Stream APIs
 * @ingroup stream_apis
 * @{
 */
/** structure of buffer,
  * used to pass parama to pcm buffer stream
  */
typedef struct 
{
	/** audio out handle*/
	void *aout_handle;
    uint16_t sample_rate; //khz
    uint16_t buffer_size; //in bytes
	uint8_t channels;
    uint8_t frame_size;
    uint64_t (*get_play_time_us)(void);
    uint64_t (*get_bt_clk_us)(void *tws_observer);
}pcm_buffer_param_t;

/**
 * @brief create pcm buffer stream, return stream handle
 *
 * This routine provides create stream, and return stream handle.
 * and stream state is  STATE_INIT
 *
 * @param param create stream parama
 *
 * @return stream handle if create stream success
 * @return NULL  if create stream failed
 */
io_stream_t pcm_buffer_stream_create(pcm_buffer_param_t *param);

/**
 * @} end defgroup pcm_buffer_stream_apis
 */

#endif /* __PCM_BUFFER_STREAM_H__ */
