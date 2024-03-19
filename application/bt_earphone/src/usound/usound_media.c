/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief usound media
 */

#include "usound.h"
#include "tts_manager.h"
#include "buffer_stream.h"
#include "ringbuff_stream.h"
#include "media_mem.h"
#include <acts_ringbuf.h>
#include "ui_manager.h"
#include "app_common.h"
#include <dvfs.h>
#include <soc.h>

static io_stream_t _usound_playback_create_input_stream(void)
{
    int size, ret;
    io_stream_t stream = NULL;

    size   = media_mem_get_cache_pool_size(INPUT_PLAYBACK, AUDIO_STREAM_USOUND);
    stream = ringbuff_stream_create_ext(
                media_mem_get_cache_pool(INPUT_PLAYBACK, AUDIO_STREAM_USOUND), size);
    if (!stream) {
        SYS_LOG_ERR("create ringbuff failed !\n");
        return NULL;
    }

    ret = stream_open(stream, MODE_IN_OUT);
    if (ret) {
        SYS_LOG_ERR("open stream failed !\n");
        stream_destroy(stream);
        return NULL;
    }

    SYS_LOG_INF("%p", stream);
    return stream;
}

int usound_playback_init(void)
{
    int threshold_us;
    io_stream_t input_stream        = NULL;
    media_init_param_t init_param   = {0};
    struct usound_app_t *usound     = usound_get_app();

    if (usound->playback_player) {
        SYS_LOG_INF(" playback player has initialized !\n");
        return -EALREADY;
    }

	if (!usound->capture_player) {
		SYS_LOG_INF("Clear leaudio ref");
		media_player_set_session_ref(NULL);
	}

#ifdef CONFIG_PLAYTTS
    tts_manager_wait_finished(false);
#endif

    input_stream = _usound_playback_create_input_stream();
    if (!input_stream)
    {
        SYS_LOG_ERR(" create input stream failed !\n");
        goto err_exit;
    }

    /* Drop origin garbege data */
    acts_ringbuf_fill(stream_get_ringbuffer(input_stream), 0, stream_get_space(input_stream));
    acts_ringbuf_drop_all(stream_get_ringbuffer(input_stream));
    
    memset(&init_param, 0, sizeof(media_init_param_t));

    init_param.type 				= MEDIA_SRV_TYPE_PLAYBACK;
    init_param.stream_type 			= AUDIO_STREAM_USOUND;
    init_param.efx_stream_type 		= AUDIO_STREAM_VOICE;
    init_param.format 		        = PCM_DSP_TYPE;
    init_param.sample_rate 			= 48;
    init_param.sample_bits 			= 32;
    init_param.input_stream 		= input_stream;
    init_param.output_stream 		= NULL;
    init_param.event_notify_handle 	= NULL;
    init_param.dumpable 			= 1;
    init_param.channels 			= 2;
    init_param.dsp_output 			= 1;
    init_param.uac_upload_flag 		= 0;
    init_param.dsp_latency_calc 	= 1;
    init_param.audiopll_mode 	    = 2;

    usound->usb_download_stream     = input_stream;
    usound->playback_player         = media_player_open(&init_param);
    if (!usound->playback_player) {
        goto err_exit;
    }
    
    SYS_LOG_INF(" %p\n", usound->playback_player);

    threshold_us = audiolcy_get_latency_threshold(init_param.format);
    media_player_set_audio_latency(usound->playback_player, threshold_us);
    media_player_set_effect_enable(usound->playback_player, usound->dae_enalbe);
    media_player_update_effect_param(usound->playback_player, usound->cfg_dae_data, CFG_SIZE_BT_MUSIC_DAE_AL);

    usb_audio_set_stream(usound->usb_download_stream);
    
    return 0;

err_exit:
    if (input_stream) {
        stream_close(input_stream);
        stream_destroy(input_stream);
    }
    
    if(usound->usb_download_stream) {
        usb_audio_set_stream(NULL);
        usound->usb_download_stream = NULL;
    }
    
    SYS_LOG_ERR("open failed\n");
    return -EINVAL;
}

int usound_playback_start(void)
{
    struct usound_app_t *usound = usound_get_app();

    if (!usound->playback_player) {
        SYS_LOG_INF("playback player is not ready !\n");
        return -EINVAL;
    }

    if (usound->playback_player_run) {
        SYS_LOG_INF("playback player is running !\n");
        return -EINVAL;
    }

    if (usound->capture_player_run) {
        //media_player_set_record_aps_monitor_enable(usound->capture_player, true);
    }

    /* For avoiding some noise */
    usb_audio_set_skip_ms(600);
    media_player_fade_in(usound->playback_player, 300);

    if (!usound->playback_player_load) {
        media_player_pre_play(usound->playback_player);
        media_player_play(usound->playback_player);

        usound->playback_player_load = 1;
    } else {
        acts_ringbuf_drop_all(stream_get_ringbuffer(usound->usb_download_stream));
        acts_ringbuf_fill_none(stream_get_ringbuffer(usound->usb_download_stream), stream_get_space(usound->usb_download_stream));
        SYS_LOG_INF("download stream len : %d\n", stream_get_length(usound->usb_download_stream));
        
        media_player_resume(usound->playback_player);
    }

    usound->playback_player_run = 1;
    
    SYS_LOG_INF("%p\n", usound->playback_player);

    return 0;
}

int usound_playback_stop(void)
{
    struct usound_app_t *usound = usound_get_app();

    if (!usound->playback_player || !usound->playback_player_run) {
        SYS_LOG_INF("playback player is not ready !\n");
        return -EINVAL;
    }

    media_player_pause(usound->playback_player);
    SYS_LOG_INF("%p\n", usound->playback_player);

    if (!usound->playback_player || !usound->playback_player_run) {
        usound_playback_exit();
    }
    else {
        usound->playback_player_run = 0;
    }
    
    return 0;
}

int usound_playback_exit(void)
{
    struct usound_app_t *usound = usound_get_app();

    if (!usound->playback_player) {
        SYS_LOG_INF("playback player is not ready !\n");
        return -EALREADY;
    }

    if (usound->playback_player_load) {
        media_player_stop(usound->playback_player);
    }

    media_player_close(usound->playback_player);
    SYS_LOG_INF("%p\n", usound->playback_player);
    
    if (usound->usb_download_stream) {
        stream_close(usound->usb_download_stream);
        stream_destroy(usound->usb_download_stream);
        usb_audio_set_stream(NULL);
        usound->usb_download_stream = NULL;
    }

    usound->playback_player      = NULL;
    usound->playback_player_load = 0;
    usound->playback_player_run  = 0;

    return 0;
}

static io_stream_t _usound_capture_create_upload_stream(void)
{
    int ret = 0;
    io_stream_t upload_stream = NULL;

    upload_stream = usb_audio_upload_stream_create();
    if (!upload_stream) {
        SYS_LOG_ERR("create stream failed !\n");
        return NULL;
    }

    ret = stream_open(upload_stream, MODE_IN_OUT);
    if (ret) {
        SYS_LOG_ERR("open stream failed !\n");
        stream_destroy(upload_stream);
        return NULL;
    }

    SYS_LOG_INF(" %p\n", upload_stream);
    return	upload_stream;
}

int usound_capture_init(void)
{
    io_stream_t upload_stream     = NULL;
    media_init_param_t init_param = {0};
    struct usound_app_t *usound   = usound_get_app();

    if (usound->capture_player) {
        SYS_LOG_INF("capture player has initialized !\n");
        return -EALREADY;
    }

	if (!usound->playback_player) {
		SYS_LOG_INF("Clear leaudio ref");
		media_player_set_session_ref(NULL);
	}

    /* Wait until start_capture */
#ifdef CONFIG_PLAYTTS
    tts_manager_wait_finished(false);
#endif

    upload_stream = _usound_capture_create_upload_stream();
    if (!upload_stream) {
        SYS_LOG_ERR("create upload stream fail !\n");
        goto err_exit;
    }

    // SYS_LOG_INF("upload stream len :%d\n", stream_get_length(upload_stream));

    init_param.type                         = MEDIA_SRV_TYPE_CAPTURE;
    init_param.stream_type 	                = AUDIO_STREAM_USOUND; 
    init_param.efx_stream_type              = AUDIO_STREAM_VOICE;
    init_param.capture_format               = PCM_DSP_TYPE;
    init_param.capture_sample_rate_input 	= 32;
    init_param.capture_sample_rate_output 	= 32;
    init_param.capture_sample_bits 			= 24;
    init_param.capture_frame_size           = 120; // 160
    init_param.capture_input_stream         = NULL;
    init_param.latency_us                   = 10000;
   
    if (audio_policy_get_record_audio_mode(init_param.stream_type, init_param.efx_stream_type) == AUDIO_MODE_STEREO) {
        init_param.capture_channels_input   = 2;
    } else {
        init_param.capture_channels_input   = 1;
    }

    init_param.capture_output_stream        = upload_stream;
    init_param.capture_channels_output      = CONFIG_USB_AUDIO_UPLOAD_CHANNEL_NUM;
    init_param.capture_bit_rate 	        = 48;
    init_param.dsp_output                   = 1;
    init_param.audiopll_mode 	            = 2;
    init_param.dumpable     	            = 1;

    usound->usb_upload_stream   = upload_stream;
    usound->capture_player      = media_player_open(&init_param);
    if (!usound->capture_player) {
        SYS_LOG_ERR("media player open fail !\n");
        goto err_exit;
    }

    /* 设置上行音效参数 */
	{
		/* 音效开关 */
		CFG_Struct_LEAUDIO_Call_MIC_DAE usound_call_dae;
		void *pbuf = NULL;
		int size;
		const void *dae_param = NULL;
		const void *aec_param = NULL;

        app_config_read(CFG_ID_LEAUDIO_CALL_MIC_DAE, &usound_call_dae, 0, sizeof(CFG_Struct_LEAUDIO_Call_MIC_DAE));
		media_player_set_upstream_dae_enable(usound->capture_player, usound_call_dae.Enable_DAE);

        uint32_t BS_Develop_Value1;
        app_config_read(
    		CFG_ID_BTSPEECH_PLAYER_PARAM, &BS_Develop_Value1,
    		offsetof(CFG_Struct_BTSpeech_Player_Param, BS_Develop_Value1),
    		sizeof(BS_Develop_Value1));
        if((BS_Develop_Value1 & BTCALL_ALSP_POST_AGC) == 0) {
            media_player_set_downstream_agc_enable(usound->capture_player, false);
        }
        if((BS_Develop_Value1 & BTCALL_ALSP_PRE_ENC) == 0) {
            media_player_set_upstream_enc_enable(usound->capture_player, false);
        }
        if((BS_Develop_Value1 & BTCALL_ALSP_PRE_AI_ENC) == 0) {
            media_player_set_upstream_ai_enc_enable(usound->capture_player, false);
        }
        if((BS_Develop_Value1 & BTCALL_ALSP_PRE_CNG) == 0) {
            media_player_set_upstream_cng_enable(usound->capture_player, false);
        }

		size = (CFG_SIZE_LEAUDIO_CALL_DAE_AL > CFG_SIZE_BT_CALL_QUALITY_AL ? \
				CFG_SIZE_LEAUDIO_CALL_DAE_AL : CFG_SIZE_BT_CALL_QUALITY_AL);
		pbuf = mem_malloc(size);
		if(pbuf != NULL)
		{
            media_effect_get_param(AUDIO_STREAM_LE_AUDIO_MUSIC, AUDIO_STREAM_VOICE, &dae_param, &aec_param);

            if(!dae_param) {
    			/* 音效参数 */
    			app_config_read(CFG_ID_LEAUDIO_CALL_DAE_AL, pbuf, 0, CFG_SIZE_LEAUDIO_CALL_DAE_AL);
    			media_player_update_effect_param(usound->capture_player, pbuf, CFG_SIZE_LEAUDIO_CALL_DAE_AL);
            }

            if(!aec_param) {
    			/* 通话aec参数 */
    			app_config_read(CFG_ID_BT_CALL_QUALITY_AL, pbuf, 0, CFG_SIZE_BT_CALL_QUALITY_AL);
    			media_player_update_aec_param(usound->capture_player, pbuf, CFG_SIZE_BT_CALL_QUALITY_AL);
            }
			
			mem_free(pbuf);
		}
		else
		{
			SYS_LOG_ERR("malloc DAE buf failed!\n");
		}
	}
    media_player_pre_enable(usound->capture_player);
    SYS_LOG_INF("%p\n", usound->capture_player);

    return 0;

err_exit:
    if (upload_stream) {
        stream_close(upload_stream);
        stream_destroy(upload_stream);
    }

    SYS_LOG_ERR("open failed\n");
    return -EINVAL;
}

int usound_capture_start(void)
{
    struct usound_app_t *usound = usound_get_app();

    if (!usound->capture_player) {
        SYS_LOG_ERR("capture player is not ready !\n");
        return -EINVAL;
    }

    if (usound->capture_player_run) {
        SYS_LOG_INF("capture player is running !\n");
        //return -EINVAL;
    }

#ifdef CONFIG_PLAYTTS
    tts_manager_wait_finished(true);
#endif

    media_player_set_record_aps_monitor_enable(usound->capture_player, false);
    
    if (!usound->capture_player_load) {
        media_player_play(usound->capture_player);
        usound->capture_player_load = 1;
    } else {
        //media_player_resume(usound->capture_player);
    }
	
	usb_audio_upload_stream_set(NULL);
	k_msleep(50);
    usb_reset_upload_skip_ms();
    //usb_audio_set_upload_skip_ms(100);
	usb_audio_upload_stream_set(usound->usb_upload_stream);

    media_player_set_record_aps_monitor_enable(usound->capture_player, true);

    usound->capture_player_run = 1;
    SYS_LOG_INF("%p\n", usound->capture_player);

    return 0;
}

int usound_capture_stop(void)
{
    struct usound_app_t *usound = usound_get_app();

    if (!usound->capture_player || !usound->capture_player_run) {
        SYS_LOG_INF("capture player is not ready !\n");
        return -EINVAL;
    }

    media_player_set_record_aps_monitor_enable(usound->capture_player, false);

    //media_player_pause(usound->capture_player);
    SYS_LOG_INF("%p\n", usound->capture_player);
    
    if (!usound->capture_player || !usound->capture_player_run) {
        usound_capture_exit();
    }
    else {
        usound->capture_player_run = 0;
    }
    
    return 0;
}

int usound_capture_exit(void)
{
    struct usound_app_t *usound = usound_get_app();

    if (!usound->capture_player) {
        SYS_LOG_INF("capture player has exited\n");
        return -EALREADY;
    }

    if (usound->capture_player_load) {
        media_player_stop(usound->capture_player);
    }

    media_player_close(usound->capture_player);
    SYS_LOG_INF("%p\n", usound->capture_player);

    if (usound->usb_upload_stream) {
        stream_close(usound->usb_upload_stream);
        stream_destroy(usound->usb_upload_stream);
        usound->usb_upload_stream = NULL;
    }
    
    usound->capture_player      = NULL;
    usound->capture_player_load = 0;
    usound->capture_player_run  = 0;

    return 0;
}

