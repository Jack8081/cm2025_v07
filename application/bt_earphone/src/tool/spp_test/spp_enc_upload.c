/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2020 Actions Semiconductor. All rights reserved.
 *
 *  \file       spp_mic_upload.c
 *  \brief      upload MIC record data during spp connection
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2020-8-1
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */

#include "abtp_inner.h"
#include "media_mem.h"
#include "ringbuff_stream.h"

enc_upload_context_t *g_enc_upload_ctx;

static int _enc_test_create_streams(enc_upload_context_t *ctx)
{
	int ret = 0;
    char *buffer;
    int buffer_size = 0x600;
    int buf_count = 0;
    int i;

	ctx->input_stream = ringbuff_stream_create_ext(
			media_mem_get_cache_pool(INPUT_PLAYBACK, AUDIO_STREAM_VOICE),
			media_mem_get_cache_pool_size(INPUT_PLAYBACK, AUDIO_STREAM_VOICE));
	if(!ctx->input_stream)
		goto exit; 
	ret = stream_open(ctx->input_stream, MODE_IN_OUT);
	if (ret)
		goto exit;

    ctx->output_stream = ringbuff_stream_create_ext(
			media_mem_get_cache_pool(OUTPUT_CAPTURE, AUDIO_STREAM_VOICE),
			media_mem_get_cache_pool_size(OUTPUT_CAPTURE, AUDIO_STREAM_VOICE));
	if(!ctx->output_stream)
		goto exit; 
	ret = stream_open(ctx->output_stream, MODE_IN_OUT);
	if (ret)
		goto exit;

    buffer = media_mem_get_cache_pool(TOOL_ASQT_DUMP_BUF, AUDIO_STREAM_VOICE);
    if(ctx->channel_bitmap == 0x4) {
        buffer_size = 0xc00;
        buf_count = 1;
    } else {
        if(ctx->channel_bitmap & 0x1) {
            buf_count++;
        }
        if(ctx->channel_bitmap & 0x2) {
            buf_count++;
        }
    }

    for(i=0; i<buf_count; i++) {
        ctx->dump_stream[i] = ringbuff_stream_create_ext(buffer, buffer_size);
    	if(!ctx->dump_stream[i])
    		goto exit;
    	ret = stream_open(ctx->dump_stream[i], MODE_IN_OUT);
    	if (ret)
    		goto exit;
        ctx->dump_buf[i] = stream_get_ringbuffer(ctx->dump_stream[i]);
        buffer += buffer_size;
    }

    return 0;

exit:
	SYS_LOG_ERR("stream create fail");
	return 	-1;
}

static int _enc_test_player_start(enc_upload_context_t *ctx)
{
    void *pbuf = NULL;
	media_init_param_t init_param;
	u8_t sample_rate = ctx->sample_rate/1000;

	memset(&init_param, 0, sizeof(media_init_param_t));
    ctx->pkthdr.status = 1;
    ctx->pkthdr.seq_no = 1;
    if(ctx->sample_rate == 8000) {
        init_param.format = CVSD_TYPE;
        init_param.capture_format = CVSD_TYPE;

        ctx->pkthdr.frame_len = 60;
        ctx->pkthdr.padding_len = 0;
    } else {
        init_param.format = MSBC_TYPE;
        init_param.capture_format = MSBC_TYPE;
        init_param.capture_bit_rate = 26;

        ctx->pkthdr.frame_len = 58;
        ctx->pkthdr.padding_len = 2;
    }
    
	init_param.type = MEDIA_SRV_TYPE_PLAYBACK_AND_CAPTURE;
	init_param.stream_type = AUDIO_STREAM_VOICE;
	init_param.efx_stream_type = AUDIO_STREAM_VOICE;
	init_param.sample_rate = sample_rate;
	init_param.input_stream = ctx->input_stream;
	init_param.output_stream = NULL;
	init_param.event_notify_handle = NULL;
	init_param.capture_sample_rate_input = sample_rate;
	init_param.capture_sample_rate_output = sample_rate;
	init_param.capture_input_stream = NULL;
	init_param.capture_output_stream = ctx->output_stream;
    init_param.support_tws = 1;
	init_param.dumpable = 1;
    init_param.sample_bits = 32;
	init_param.channels = 1;
	init_param.capture_channels_input = 2;

	ctx->player = media_player_open(&init_param);
	if (!ctx->player) {
		return -1;
	}

	pbuf = mem_malloc(CFG_SIZE_BT_CALL_QUALITY_AL);
	if(pbuf != NULL) {
		/* 通话aec参数 */
		app_config_read(CFG_ID_BT_CALL_QUALITY_AL, pbuf, 0, CFG_SIZE_BT_CALL_QUALITY_AL);
		media_player_update_aec_param(ctx->player, pbuf, CFG_SIZE_BT_CALL_QUALITY_AL);
		
		mem_free(pbuf);
	} else {
		SYS_LOG_ERR("malloc aec buf failed!\n");
	}

    media_player_set_effect_enable(ctx->player, false);
    media_player_set_upstream_dae_enable(ctx->player, false);
    media_player_set_upstream_cng_enable(ctx->player, false);

	media_player_play(ctx->player);
	media_player_set_hfp_connected(ctx->player, true);
    ctx->last_fill_time = k_cycle_get_32();
    return 0;
}

bool enc_data_start_capture(thread_timer_expiry_t expiry_fn, u32_t sample_rate, u32_t channel_bitmap)
{
    int ret;
    uint8_t asqt_tag_table[2];
    int count = 0;
    
    g_enc_upload_ctx = app_mem_malloc(sizeof(enc_upload_context_t));
    if (!g_enc_upload_ctx)
        return FALSE;	
    memset(g_enc_upload_ctx, 0, sizeof(enc_upload_context_t));

    g_enc_upload_ctx->channel_bitmap = channel_bitmap;
    g_enc_upload_ctx->sample_rate = sample_rate;

    g_enc_upload_ctx->spp_data = app_mem_malloc(MICDATA_PACKET_SIZE + sizeof(mic_pcm_frame_info_t));
    if (!g_enc_upload_ctx->spp_data)
        return FALSE;	

    g_enc_upload_ctx->pcm_frame_head = (mic_pcm_frame_info_t*)g_enc_upload_ctx->spp_data;
    g_enc_upload_ctx->pcm_frame_head->data_flag = 0x16;
    g_enc_upload_ctx->pcm_frame_head->payload_len = MICDATA_PACKET_SIZE + 8;
    g_enc_upload_ctx->pcm_frame_head->sample_rate = sample_rate;
    g_enc_upload_ctx->pcm_frame_head->sequence = 0;

    ret = _enc_test_create_streams(g_enc_upload_ctx);
    if (ret < 0)
        return FALSE;

    ret = _enc_test_player_start(g_enc_upload_ctx);
    if (ret < 0)
        return FALSE;
    
    if(g_enc_upload_ctx->channel_bitmap & 0x1) {
        asqt_tag_table[count] = MEDIA_DATA_TAG_MIC1;
        count++;
    }
    if(g_enc_upload_ctx->channel_bitmap & 0x2) {
        asqt_tag_table[count] = MEDIA_DATA_TAG_MIC2;
        count++;
    }
    if(g_enc_upload_ctx->channel_bitmap & 0x4) {
        asqt_tag_table[count] = MEDIA_DATA_TAG_AEC1;
        count++;
    }

    media_player_dump_data(g_enc_upload_ctx->player,
        count, asqt_tag_table, g_enc_upload_ctx->dump_buf);
    
    thread_timer_init(&g_enc_upload_ctx->ttimer, expiry_fn, NULL);
    thread_timer_start(&g_enc_upload_ctx->ttimer, 5, 5);
    return TRUE;
}

void enc_data_stop_capture(void)
{
    int i;
    
    if (NULL == g_enc_upload_ctx)
    {
        return;
    }

	if (g_enc_upload_ctx->input_stream) {
		stream_close(g_enc_upload_ctx->input_stream);
	}

    if (g_enc_upload_ctx->player)
    {
        thread_timer_stop(&g_enc_upload_ctx->ttimer);
        media_player_stop(g_enc_upload_ctx->player);
    	media_player_close(g_enc_upload_ctx->player);
    }
    
	if (g_enc_upload_ctx->output_stream) {
		stream_close(g_enc_upload_ctx->output_stream);
		stream_destroy(g_enc_upload_ctx->output_stream);
	}
	if (g_enc_upload_ctx->input_stream) {
		stream_destroy(g_enc_upload_ctx->input_stream);
	}

    for(i=0; i<2; i++) {
        if (g_enc_upload_ctx->dump_stream[i]) {
    		stream_close(g_enc_upload_ctx->dump_stream[i]);
    		stream_destroy(g_enc_upload_ctx->dump_stream[i]);
    	}
    }

    if(g_enc_upload_ctx->spp_data != NULL)
    {
        app_mem_free(g_enc_upload_ctx->spp_data);
        g_enc_upload_ctx->spp_data = NULL;
    }

    app_mem_free(g_enc_upload_ctx);
}

void enc_pcm_upload_handler(struct thread_timer *ttimer, void *expiry_fn_arg)
{
    enc_upload_context_t *ctx = g_enc_upload_ctx;
    uint32_t size, length;
    int index;

    //printk("in; %d, out: %d, dump: %d, %d, diff: %u\n",
    //    stream_get_length(ctx->input_stream),
    //    stream_get_length(ctx->output_stream),
    //    stream_get_length(ctx->dump_stream[0]),
    //    stream_get_length(ctx->dump_stream[1]),
    //    k_cyc_to_us_floor32(k_cycle_get_32() - ctx->last_fill_time));
    //drop output
    size = stream_get_length(ctx->output_stream);
    if(size > 0) {
        stream_read(ctx->output_stream, NULL, size);
    }

    //fill empty pkt
    while(k_cyc_to_us_floor32(k_cycle_get_32() - ctx->last_fill_time) >= 7500) {
        ctx->last_fill_time += k_us_to_cyc_floor32(7500);
        //printk("diff: %u\n", k_cyc_to_us_floor32(k_cycle_get_32() - ctx->last_fill_time));
        if(stream_get_space(ctx->input_stream) >= (sizeof(ctx->pkthdr) + 60)) {
            stream_write(ctx->input_stream, &ctx->pkthdr, sizeof(ctx->pkthdr) + 60);
            //printk("sssssss line=%d, func=%s, fill: %u\n", __LINE__, __func__, ctx->pkthdr.seq_no);
            ctx->pkthdr.seq_no++;
        }
    }

    size = MICDATA_PACKET_SIZE;
    while (1)
    {
        if((ctx->dump_stream[0] && stream_get_length(ctx->dump_stream[0]) < size)
            || (ctx->dump_stream[1] && stream_get_length(ctx->dump_stream[1]) < size)) {
            break;
        }

        index = 0;
        if(ctx->channel_bitmap & 0x1) {
            stream_read(ctx->dump_stream[index], ctx->pcm_frame_head->data, size);

            ctx->pcm_frame_head->channel = 0;
            length = spp_test_backend_write((u8_t*)ctx->pcm_frame_head, size + sizeof(mic_pcm_frame_info_t), 1);
            if ((size + sizeof(mic_pcm_frame_info_t)) != length)
            {
                SYS_LOG_ERR("Fail to upload mic0 data(%dB)", length);
            }
            //printk("send 0: %d\n", size);
            
            index++;
        }
        if(ctx->channel_bitmap & 0x2) {
            stream_read(ctx->dump_stream[index], ctx->pcm_frame_head->data, size);

            ctx->pcm_frame_head->channel = 1;
            length = spp_test_backend_write((u8_t*)ctx->pcm_frame_head, size + sizeof(mic_pcm_frame_info_t), 1);
            if ((size + sizeof(mic_pcm_frame_info_t)) != length)
            {
                SYS_LOG_ERR("Fail to upload mic0 data(%dB)", length);
            }
            //printk("send 1: %d\n", size);
            
            index++;
        }
        if(ctx->channel_bitmap & 0x4) {
            stream_read(ctx->dump_stream[index], ctx->pcm_frame_head->data, size);

            ctx->pcm_frame_head->channel = 2;
            length = spp_test_backend_write((u8_t*)ctx->pcm_frame_head, size + sizeof(mic_pcm_frame_info_t), 1);
            if ((size + sizeof(mic_pcm_frame_info_t)) != length)
            {
                SYS_LOG_ERR("Fail to upload mic0 data(%dB)", length);
            }
            
            index++;
        }

        ctx->pcm_frame_head->sequence++;
    }
}

