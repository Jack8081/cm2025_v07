/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio record.
*/

#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <audio_hal.h>
#include <audio_record.h>
#include <audiolcy_common.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <media_type.h>
#include <media_mem.h>
#include <ringbuff_stream.h>
#include <audio_device.h>



#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "audio record"


#ifdef CONFIG_FIXED_DMA_ACCESS_PSRAM
#define AUDIO_RECORD_PCM_BUFF_SIZE  1024
static uint8_t record_pcm_buff[1024] __aligned(4) __in_section_unique(audio.bss.input_pcm);
#else
#ifdef CONFIG_SOC_SERIES_CUCKOO
#define AUDIO_RECORD_PCM_BUFF_TIME  (5)
#else
#define AUDIO_RECORD_PCM_BUFF_TIME  (4)
#endif
#endif

extern void *ringbuff_stream_get_ringbuf_buf(io_stream_t handle);
extern uint32_t ringbuff_stream_get_ringbuf_size(io_stream_t handle);

static int _audio_record_data_full_count = 0;

static int _audio_record_request_more_data(void *priv_data, uint32_t reason)
{
	int ret = 0;
    struct audio_record_t *handle = (struct audio_record_t *)priv_data;
    if (!handle)
		return -1;
	
	int num = handle->pcm_buff_size / 2;
	uint8_t *buf = NULL;

	if (!handle->audio_stream || handle->paused)
		goto exit;

    if(handle->play_flag) {
        if(*handle->play_flag == 0) {
            goto exit;
        }
        if(handle->drop_cnt) {
            //printk("adc drop\n");
            handle->drop_cnt--;
            goto exit;
        }

        handle->play_flag = NULL;
    }

	if (reason == AIN_DMA_IRQ_HF) {
		buf = handle->pcm_buff;
	} else if (reason == AIN_DMA_IRQ_TC) {
		buf = handle->pcm_buff + num;
	}

	if (handle->pcm_buff == ringbuff_stream_get_ringbuf_buf(handle->audio_stream)) {
		ret = stream_write(handle->audio_stream, NULL, num);
		if (ret <= 0) {
			++_audio_record_data_full_count;
			if (_audio_record_data_full_count <= 5 || (_audio_record_data_full_count & 0x7F) == 0) {
				SYS_LOG_WRN("data full ret %d, drop old data, cnt %d", ret, _audio_record_data_full_count);
			}
			if (stream_read(handle->audio_stream, NULL, num) > 0) {
				ret = stream_write(handle->audio_stream, NULL, num);
				if (ret <= 0) {
					SYS_LOG_WRN("data full after drop data ret %d", ret);
				}
			}
		} else {
			_audio_record_data_full_count = 0;
		}
	} else {
		if (buf && handle->muted) {
			memset(buf, 0, num);
		}

		/**TODO: this avoid adc noise for first block*/
		if (buf && handle->first_frame) {
			memset(buf, 0, num);
			handle->first_frame = 0;
		}

		ret = stream_write(handle->audio_stream, buf, num);
		if (ret <= 0) {
			++_audio_record_data_full_count;
			if (_audio_record_data_full_count <= 5 || (_audio_record_data_full_count & 0x7F) == 0) {
				SYS_LOG_WRN("data full ret %d, cnt %d", ret, _audio_record_data_full_count);
			}
		} else {
			_audio_record_data_full_count = 0;
		}
	}

exit:
	return 0;
}

static int32_t latency_offset_avg = 12345678, latency_offset_avg_rest;

static void *_audio_record_init(struct audio_record_t *handle)
{
	audio_in_init_param_t ain_param = {0};

    latency_offset_avg = 12345678;
    latency_offset_avg_rest = 0;

	memset(&ain_param, 0, sizeof(audio_in_init_param_t));

	ain_param.sample_rate = handle->sample_rate;
	ain_param.adc_gain = handle->adc_gain;
	ain_param.input_gain = 0;
	ain_param.boost_gain = 0;
	ain_param.data_mode = handle->audio_mode;
	ain_param.data_width = handle->sample_bits;
	ain_param.dma_reload = 1;
	ain_param.reload_addr = handle->pcm_buff;
	ain_param.reload_len = handle->pcm_buff_size;
	ain_param.channel_type = handle->channel_type;

	ain_param.audio_device = audio_policy_get_record_channel_id(handle->stream_type, handle->ext_stream_type);

	ain_param.callback = _audio_record_request_more_data;
	ain_param.callback_data = handle;
	_audio_record_data_full_count = 0;

	if (handle->is_dest_directly) {
		ain_param.disable_discard = 1;
	}

	ain_param.audiopll_mode = handle->audiopll_mode;

    return hal_ain_channel_open(&(ain_param));
}

struct audio_record_t *audio_record_create(uint8_t stream_type, uint8_t ext_stream_type,
                                    int sample_rate_input, int sample_rate_output,
									uint8_t format, uint32_t audio_mode_and_audiopll_mode_and_sample_bits, void *outer_stream)
{
	struct audio_record_t *audio_record = NULL;
    audio_input_gain input_gain;
    int32_t reload_buff_size;
    int32_t hw_aec = 0;
	uint8_t audio_mode = (audio_mode_and_audiopll_mode_and_sample_bits & AUDIO_MODE_MASK) >> AUDIO_MODE_SHIFT;
	uint8_t audiopll_mode = (audio_mode_and_audiopll_mode_and_sample_bits & AUDPLL_MODE_MASK) >> AUDPLL_MODE_SHIFT;
	uint8_t sample_bits_sel = (audio_mode_and_audiopll_mode_and_sample_bits & SAMPLE_BITS_MASK) >> SAMPLE_BITS_SHIFT;
	uint8_t sample_bits = (sample_bits_sel == 1)? (24): (16);
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
	uint8_t no_aps_adjust = (audio_mode_and_audiopll_mode_and_sample_bits & NO_APS_BITS_MASK) >> NO_APS_BITS_SHIFT;
#endif
	audio_record = mem_malloc(sizeof(struct audio_record_t));
	if (!audio_record) {
		return NULL;
	}

	audio_record->stream_type = stream_type;
	audio_record->ext_stream_type = ext_stream_type;
	audio_record->audio_format = format;

	audio_record->audiopll_mode = AUDIOPLL_INTEGER_N;
#ifdef CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL
	if(no_aps_adjust) {
		audio_record->audiopll_mode	= AUDIOPLL_FRACTIONAL_N;
	}
#endif
	if (format == NAV_TYPE || format == PCM_DSP_TYPE) {
		audio_record->is_low_latency = 1; /* if yes, dma drm set to single mode */
		audio_record->is_dest_directly = 1; /* if yes, need't copy dma buffer to another larger buffer */
        if(stream_type == AUDIO_STREAM_USOUND) {
		    audio_record->is_low_latency = 0; /* if yes, dma drm set to single mode */
            audio_record->is_dest_directly = 0;
        }
	}
	if (audiopll_mode == 2) {
		audio_record->audiopll_mode	= AUDIOPLL_FRACTIONAL_N;
		SYS_LOG_INF("AUDIOPLL_FRACTIONAL_N");
	}

	audio_record->audio_mode = audio_mode;
	audio_record->output_sample_rate = sample_rate_output;
    if (audio_record->audio_mode == AUDIO_MODE_DEFAULT)
		audio_record->audio_mode = audio_policy_get_record_audio_mode(stream_type, ext_stream_type);

    if (audio_policy_get_record_channel_support_aec(audio_record->stream_type)
        && audio_policy_get_aec_reference_type() == AEC_REF_TYPE_HW){
		hw_aec = 1;
        audio_record->audio_mode++;
	}

	if (outer_stream) {
		audio_record->audio_stream = ringbuff_stream_create((struct acts_ringbuf *)outer_stream);
	} else {
		audio_record->audio_stream = ringbuff_stream_create_ext(
									media_mem_get_cache_pool(INPUT_PCM, stream_type),
									media_mem_get_cache_pool_size(INPUT_PCM, stream_type));
	}

	if (!audio_record->audio_stream) {
		SYS_LOG_ERR("audio_stream create failed");
		goto err_exit;
	}

#ifdef CONFIG_FIXED_DMA_ACCESS_PSRAM
	/* dma reload buff */
	if (audiolcy_is_low_latency_mode()) {
		audio_record->pcm_buff_size = (sample_rate_input <= 16) ? 256 : 512;
	} else {
		audio_record->pcm_buff_size = (sample_rate_input <= 16) ? 512 : 1024;
	}
	
	audio_record->pcm_buff = record_pcm_buff;
	
	if (audio_record->audio_mode == AUDIO_MODE_MONO) {
		audio_record->frame_size = 2;
	} else if (audio_record->audio_mode == AUDIO_MODE_STEREO) {
		audio_record->frame_size = 4;
	}
#else
	if (audio_record->is_dest_directly) {
		int i;
		audio_record->pcm_buff = ringbuff_stream_get_ringbuf_buf(audio_record->audio_stream);
		audio_record->pcm_buff_size = ringbuff_stream_get_ringbuf_size(audio_record->audio_stream);
		for (i = 0; i < audio_record->pcm_buff_size/sizeof(int); i++) {
			*((int32_t *)audio_record->pcm_buff + i) = 0x78788787;
		}
	} else {
#ifdef CONFIG_SOC_SERIES_CUCKOO
		audio_record->frame_size = 4 * audio_record->audio_mode; //24bits
#else
		audio_record->frame_size = 2 * audio_record->audio_mode;
#endif
		audio_record->pcm_buff_size = sample_rate_input * AUDIO_RECORD_PCM_BUFF_TIME * audio_record->frame_size;

		audio_record->pcm_buff = media_mem_get_cache_pool(INPUT_RELOAD_CAPTURE, stream_type);
		if(NULL == audio_record->pcm_buff) {
			audio_record->reload_buff_alloced = 1;
			audio_record->pcm_buff = mem_malloc(audio_record->pcm_buff_size);
			if (!audio_record->pcm_buff) {
				SYS_LOG_ERR("alloc fail %d", audio_record->pcm_buff_size);
				goto err_exit;
			}
		} else {
			reload_buff_size = media_mem_get_cache_pool_size(INPUT_RELOAD_CAPTURE, stream_type);
			if(audio_record->pcm_buff_size > reload_buff_size)
				audio_record->pcm_buff_size = reload_buff_size;
		}
	}
#endif

	audio_record->channel_type = audio_policy_get_record_channel_type(stream_type);
	audio_record->channel_mode = audio_policy_get_record_channel_mode(stream_type);
	audio_record->adc_gain = audio_policy_get_record_adc_gain(stream_type);
	//audio_record->input_gain = audio_policy_get_record_input_gain(stream_type);
	audio_record->sample_rate = sample_rate_input;
	audio_record->sample_bits = sample_bits;
	audio_record->first_frame = 1;

	audio_record->audio_handle = _audio_record_init(audio_record);
	if (!audio_record->audio_handle) {
		goto err_exit;
	}

	if (audio_record->is_low_latency) {
		uint8_t drq_level = 1;
		if (audio_mode == AUDIO_MODE_STEREO) {
			drq_level = 2;
		}
		hal_ain_channel_set_drq_level(audio_record->audio_handle, &drq_level);
	}

	if (stream_open(audio_record->audio_stream, MODE_IN_OUT)) {
		stream_destroy(audio_record->audio_stream);
		audio_record->audio_stream = NULL;
		SYS_LOG_ERR(" audio_stream open failed ");
		goto err_exit;
	}

	if (audio_system_register_record(audio_record)) {
		SYS_LOG_ERR(" audio_system_registy_track failed ");
		stream_close(audio_record->audio_stream);
		stream_destroy(audio_record->audio_stream);
		goto err_exit;
	}
	if (audiolcy_is_low_latency_mode()) {
		if (audio_record->stream_type == AUDIO_STREAM_VOICE) {
			if(sample_rate_input == 16) {
				memset(audio_record->pcm_buff, 0, audio_record->pcm_buff_size);
				stream_write(audio_record->audio_stream, audio_record->pcm_buff, 256);
				stream_write(audio_record->audio_stream, audio_record->pcm_buff, 256);
				stream_write(audio_record->audio_stream, audio_record->pcm_buff, 208);
			}
		}
	}

    if (hw_aec){
		hal_ain_set_aec_record_back(audio_record->audio_handle, 1);
	}
    
    audio_policy_get_record_input_gain(stream_type, &input_gain);
    audio_record_set_volume(audio_record, &input_gain);

	SYS_LOG_INF("stream_type : %d", audio_record->stream_type);
	SYS_LOG_INF("audio_format : %d", audio_record->audio_format);
	SYS_LOG_INF("audio_mode : %d", audio_record->audio_mode);
	SYS_LOG_INF("channel_type : %d ", audio_record->channel_type);
	SYS_LOG_INF("channel_id : %d ", audio_record->channel_id);
	SYS_LOG_INF("channel_mode : %d ", audio_record->channel_mode);
	SYS_LOG_INF("input_sr : %d ", audio_record->sample_rate);
	SYS_LOG_INF("output_sr : %d ", audio_record->output_sample_rate);
	SYS_LOG_INF("audio_handle : %p", audio_record->audio_handle);
	SYS_LOG_INF("audio_stream : %p", audio_record->audio_stream);
	SYS_LOG_INF("audio_stream ptr : %x", ((struct acts_ringbuf *)outer_stream)->cpu_ptr);
	return audio_record;

err_exit:

#ifndef CONFIG_FIXED_DMA_ACCESS_PSRAM
	if (audio_record->pcm_buff && audio_record->reload_buff_alloced)
		mem_free(audio_record->pcm_buff);
#endif

	mem_free(audio_record);

	return NULL;
}

int audio_record_destory(struct audio_record_t *handle)
{
	if (!handle)
		return -EINVAL;

	if (audio_system_unregister_record(handle)) {
		SYS_LOG_ERR(" audio_system_unregisty_record failed ");
		return -ESRCH;
	}

	if (handle->audio_handle)
		hal_ain_channel_close(handle->audio_handle);

	if (handle->audio_stream)
		stream_destroy(handle->audio_stream);

#ifndef CONFIG_FIXED_DMA_ACCESS_PSRAM
	if (handle->pcm_buff && handle->reload_buff_alloced) {
		mem_free(handle->pcm_buff);
		handle->pcm_buff = NULL;
	}
#endif

	mem_free(handle);

	SYS_LOG_INF(" handle: %p ok ", handle);

	return 0;
}

int audio_record_start(struct audio_record_t *handle)
{
	if (!handle)
		return -EINVAL;

	hal_ain_channel_read_data(handle->audio_handle, handle->pcm_buff, handle->pcm_buff_size);

	hal_ain_channel_start(handle->audio_handle);

	return 0;
}

int audio_record_stop(struct audio_record_t *handle)
{
	if (!handle)
		return -EINVAL;

	if (handle->audio_stream)
		stream_close(handle->audio_stream);

	if (handle->audio_handle)
		hal_ain_channel_stop(handle->audio_handle);

	return 0;
}

int audio_record_pause(struct audio_record_t *handle)
{
	if (!handle)
		return -EINVAL;

	handle->paused = 1;

	if (handle->audio_handle)
		hal_ain_channel_stop(handle->audio_handle);

	if (handle->is_dest_directly) {
		int i;
		for (i = 0; i < handle->pcm_buff_size/sizeof(int); i++) {
			*((int32_t *)handle->pcm_buff + i) = 0x78788787;
		}
	}

	SYS_LOG_DBG("");

	return 0;
}

int audio_record_resume(struct audio_record_t *handle)
{
	if (!handle)
		return -EINVAL;

	SYS_LOG_DBG("");

	if (handle->is_dest_directly) {
		int i;
		for (i = 0; i < handle->pcm_buff_size/sizeof(int); i++) {
			*((int32_t *)handle->pcm_buff + i) = 0x78788787;
		}
	}

	hal_ain_channel_start(handle->audio_handle);

	handle->paused = 0;

	return 0;
}

int audio_record_read(struct audio_record_t *handle, uint8_t *buff, int num)
{
	if (!handle->audio_stream)
		return 0;

	return stream_read(handle->audio_stream, buff, num);
}

int audio_record_flush(struct audio_record_t *handle)
{
	if (!handle)
		return -EINVAL;

	/**TODO: wanghui wait record data empty*/

	return 0;
}

int audio_record_set_volume(struct audio_record_t *handle, audio_input_gain *input_gain)
{
	adc_gain gain;
	uint8_t i;

	if (!handle)
		return -EINVAL;

	for (i = 0; i < ADC_CH_NUM_MAX; i++)
#if CONFIG_AUDIO_IN_ADGAIN_SUPPORT
    {
        gain.ch_again[i] = input_gain->ch_again[i];
        gain.ch_dgain[i] = input_gain->ch_dgain[i];
    }

    SYS_LOG_INF("again: %d, %d, %d, %d, dgain: %d, %d, %d, %d,", 
                input_gain->ch_again[0], input_gain->ch_again[1], input_gain->ch_again[2], input_gain->ch_again[3],
                input_gain->ch_dgain[0], input_gain->ch_dgain[1], input_gain->ch_dgain[2], input_gain->ch_dgain[3]
                );

#else
    gain.ch_gain[i] = input_gain->ch_gain[i];

    SYS_LOG_INF("gain: %d, %d, %d, %d", 
                input_gain->ch_gain[0], input_gain->ch_gain[1], input_gain->ch_gain[2], input_gain->ch_gain[3]);
#endif

	hal_ain_channel_set_volume(handle->audio_handle, &gain);

	return 0;
}

int audio_record_get_volume(struct audio_record_t *handle)
{
	if (!handle)
		return -EINVAL;

	return 0;
}

int audio_record_set_sample_rate(struct audio_record_t *handle, int sample_rate)
{
	if (!handle)
		return -EINVAL;

	handle->sample_rate = sample_rate;

	return 0;
}

int audio_record_get_sample_rate(struct audio_record_t *handle)
{
	if (!handle)
		return -EINVAL;

	return handle->sample_rate;
}

io_stream_t audio_record_get_stream(struct audio_record_t *handle)
{
	if (!handle)
		return NULL;

	return handle->audio_stream;
}

int audio_record_set_play_flag(struct audio_record_t *handle, uint16_t *play_flag)
{
	if (!handle || !play_flag)
		return -EINVAL;

	handle->play_flag = play_flag;
    handle->drop_cnt = 1;
	return 0;
}

int audio_record_aps_monitor(struct audio_record_t *handle, int32_t latency_offset)
{
	uint8_t adj_level = 0xFF;
	uint32_t latency_offset_abs = abs(latency_offset);

    if (latency_offset >= 150) {
        adj_level = APS_LEVEL_4;
    } else if (latency_offset <= -150) {
        adj_level = APS_LEVEL_5;
    } else if (latency_offset_abs <= 20) {
        adj_level = APS_LEVEL_0PPM;
    }

	if (adj_level != 0xFF && adj_level != handle->last_aps_level) {
		//SYS_LOG_INF("ain aps set:%d\n", adj_level);
		handle->last_aps_level = adj_level;
		return hal_ain_channel_set_aps(handle->audio_handle, adj_level, APS_LEVEL_AUDIOPLL);
	} else {
		return 0;
	}
}

int audio_record_aps_monitor_lite(struct audio_record_t *handle, int32_t latency_offset)
{
	int32_t sumval;
	uint8_t adj_level = 0xFF;
	static int count;
	
    if(latency_offset_avg == 12345678) {//init
        latency_offset_avg = latency_offset;
        count = 0;
    }

	sumval = latency_offset_avg * 15 + latency_offset + latency_offset_avg_rest;
	latency_offset_avg = sumval / 16;
	latency_offset_avg_rest = sumval - (latency_offset_avg * 16);
#if 0
    if (latency_offset_avg >= 150) {
        adj_level = 42 - 32;
    } else if (latency_offset_avg <= -150) {
        adj_level = 54 - 32;
	} else if (latency_offset_avg >= 50) {
		adj_level = 44 - 32;
	} else if (latency_offset_avg <= -50) {
		adj_level = 52 - 32;
    } else {
        adj_level = 48 - 32;
    }
#else
    if (latency_offset_avg >= 2000) {
        adj_level = 45 - 32;
    } else if (latency_offset_avg <= -2000) {
#if 0
        if (count++ % 20 == 0) {
	        adj_level = 32 - 32;
    	} else {
	        adj_level = 63 - 32;
    	}
#else
        adj_level = 51 - 32;
#endif
    } else if (latency_offset_avg >= 1000) {
        /* add for interval */
    } else if (latency_offset_avg <= -1000) {
        /* add for interval */
    } else if (latency_offset_avg <= -500) {
        adj_level = 50 - 32;
    } else if (latency_offset_avg >= 500) {
        adj_level = 46 - 32;
    } else if (latency_offset_avg >= 250) {
        /* add for interval */
    } else if (latency_offset_avg <= -250) {
        /* add for interval */
    } else if (latency_offset_avg >= 100) {
		adj_level = 47 - 32;
	} else if (latency_offset_avg <= -100) {
		adj_level = 49 - 32;
    } else if (latency_offset_avg >= 20) {
        /* add for interval */
    } else if (latency_offset_avg <= -20) {
        /* add for interval */
    } else {
        adj_level = 48 - 32;
    }

#endif
    //if(debug_info[0] & 0x2) {
    //    printk("(%d, %d)", adj_level, latency_offset_avg);
    //}

	if (adj_level != 0xFF && adj_level != handle->last_aps_level) {
		handle->last_aps_level = adj_level;
		//printk("(%d, %d, %d)\n", adj_level, latency_offset_avg, latency_offset);
		return hal_ain_channel_set_aps(handle->audio_handle, adj_level, APS_LEVEL_AUDIOPLL_F);
	} else {
		return 0;
	}
}

int audio_record_is_low_latency(struct audio_record_t *handle)
{
	return handle->is_low_latency;
}

void audio_record_aps_param_reset(void)
{
    latency_offset_avg = 12345678;
    latency_offset_avg_rest = 0;
}

