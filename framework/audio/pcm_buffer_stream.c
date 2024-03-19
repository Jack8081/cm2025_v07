/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief dac pcm buffer stream
 */
#define SYS_LOG_DOMAIN "pcm_buffer_stream"
#include "stream.h"
#include "audio_hal.h"
#include "pcm_buffer_stream.h"
#include "bluetooth_tws_observer.h"

#define DMA_MIN_TRANSFER_SIZE (16)

typedef struct {
	void *aout_handle;
    char tmp_buffer[DMA_MIN_TRANSFER_SIZE];
    uint16_t sample_rate; //khz
	uint16_t pcm_buffer_size; //单声道样点数
    uint16_t buffer_size; //in bytes
	uint8_t channels;
    uint8_t frame_size;
    uint8_t tmp_filled;
    uint8_t first_frame;

    uint64_t (*get_play_time_us)(void);
    uint64_t (*get_bt_clk_us)(void *tws_observer);
} pcm_buffer_info_t;


static int32_t pcm_buffer_stream_open(io_stream_t handle, stream_mode mode)
{
    pcm_buffer_info_t *info = (pcm_buffer_info_t *)handle->data;

    if (!info)
        return -EACCES;

    handle->mode = mode;
    return 0;
}

static int32_t pcm_buffer_stream_read(io_stream_t handle, unsigned char *buf, int32_t num)
{
    return 0;
}

static int32_t pcm_buffer_stream_write(io_stream_t handle, uint8_t *buf, int32_t len)
{
    pcm_buffer_info_t *info = (pcm_buffer_info_t *)handle->data;
    int32_t to_fill = 0;
    int32_t size = len;

    if (!info)
        return -EACCES;

    if(info->tmp_filled > 0) {
        to_fill = DMA_MIN_TRANSFER_SIZE - info->tmp_filled;

        if(to_fill > len)
            to_fill = len;

        memcpy(info->tmp_buffer + info->tmp_filled, buf, to_fill);
        buf += to_fill;
        len -= to_fill;
        info->tmp_filled += to_fill;

        if(info->tmp_filled < DMA_MIN_TRANSFER_SIZE)
            goto EXIT;

        hal_aout_channel_write_data(info->aout_handle, info->tmp_buffer, DMA_MIN_TRANSFER_SIZE);
        info->tmp_filled = 0;
    }

    if(len < DMA_MIN_TRANSFER_SIZE) {
        memcpy(info->tmp_buffer, buf, len);
        info->tmp_filled = len;
        goto EXIT;
    }


    hal_aout_channel_write_data(info->aout_handle, buf, len);

EXIT:
    return size;
}

static int32_t pcm_buffer_stream_close(io_stream_t handle)
{
    pcm_buffer_info_t *info = (pcm_buffer_info_t *)handle->data;
    int32_t res = 0;

    if (!info)
        return -EACCES;

    return res;
}

static int32_t pcm_buffer_stream_destroy(io_stream_t handle)
{
    int32_t res = 0;
    pcm_buffer_info_t *info = (pcm_buffer_info_t *)handle->data;

    if (!info)
        return -EACCES;

    mem_free(info);
    handle->data = NULL;
    return res;
}

static int32_t pcm_buffer_stream_get_length(io_stream_t handle)
{
    pcm_buffer_info_t *info = (pcm_buffer_info_t *)handle->data;
    int32_t space;

    if (!info) {
        return -EACCES;
    }

    space = hal_aout_channel_get_buffer_space(info->aout_handle);
    return (info->pcm_buffer_size - space / info->channels) * info->frame_size + info->tmp_filled;
}

static int32_t pcm_buffer_stream_get_space(io_stream_t handle)
{
    pcm_buffer_info_t *info = (pcm_buffer_info_t *)handle->data;

    if (!info)
        return -EACCES;
    return info->buffer_size - pcm_buffer_stream_get_length(handle);
}

static int32_t pcm_buffer_stream_flush(io_stream_t handle)
{
    pcm_buffer_info_t *info = (pcm_buffer_info_t *)handle->data;
    int32_t to_fill = 0;

    if (!info)
        return -EACCES;

    if(info->tmp_filled > 0) {
        to_fill = DMA_MIN_TRANSFER_SIZE - info->tmp_filled;
        memset(info->tmp_buffer + info->tmp_filled, 0, to_fill);
    
        hal_aout_channel_write_data(info->aout_handle, info->tmp_buffer, DMA_MIN_TRANSFER_SIZE);
        info->tmp_filled = 0;
    }

    return 0;
}

static int32_t pcm_buffer_stream_init(io_stream_t handle, void *param)
{
    pcm_buffer_info_t *info = NULL;
    pcm_buffer_param_t *p = (pcm_buffer_param_t*)param;
    int32_t max_size;

    info = mem_malloc(sizeof(pcm_buffer_info_t));
    if (!info) {
        SYS_LOG_ERR("malloc failed\n");
        return -ENOMEM;
    }

    handle->data = info;
    info->aout_handle = p->aout_handle;
    info->sample_rate = p->sample_rate;
    info->buffer_size = p->buffer_size;
    info->channels = p->channels;
    info->frame_size = p->frame_size;
    info->first_frame = 1;
    info->get_play_time_us = p->get_play_time_us;
    info->get_bt_clk_us = p->get_bt_clk_us;
    info->pcm_buffer_size = hal_aout_channel_get_buffer_size(info->aout_handle) / info->channels;

    max_size = info->pcm_buffer_size * info->frame_size;
    if((info->buffer_size > max_size) || (info->buffer_size == 0)) {
        info->buffer_size = max_size;
    }
	return 0;
}

static const stream_ops_t pcm_buffer_stream_ops = {
    .init = pcm_buffer_stream_init,
    .open = pcm_buffer_stream_open,
    .read = pcm_buffer_stream_read,
    .seek = NULL,
    .tell = NULL,
    .get_length = pcm_buffer_stream_get_length,
    .get_space = pcm_buffer_stream_get_space,
    .write = pcm_buffer_stream_write,
    .flush = pcm_buffer_stream_flush,
    .close = pcm_buffer_stream_close,
    .destroy = pcm_buffer_stream_destroy,
};

io_stream_t pcm_buffer_stream_create(pcm_buffer_param_t *param)
{
    return stream_create(&pcm_buffer_stream_ops, param);
}

