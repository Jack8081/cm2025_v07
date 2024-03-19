/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio policy.
*/

#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <audiolcy_common.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <audio_device.h>
#include <media_type.h>
#include <property_manager.h>
#ifdef CONFIG_TWS
#include <btservice_api.h>
#endif


#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "audio policy"

static const struct audio_policy_t *user_policy;
static u32_t a2dp_lantency_ms;

int audio_policy_get_out_channel_type(uint8_t stream_type)
{
	int out_channel = AUDIO_CHANNEL_DAC;

	if (user_policy)
		out_channel = user_policy->audio_out_channel;

	return out_channel;
}

int audio_policy_get_out_channel_id(uint8_t stream_type)
{
	int out_channel_id = AOUT_FIFO_DAC0;

	return out_channel_id;
}

int audio_policy_get_out_channel_mode(uint8_t stream_type)
{
#ifdef CONFIG_AUDIO_OUT_DAC_PCMBUF_SUPPORT
	int out_channel_mode = AUDIO_DMA_MODE;
#else
	int out_channel_mode = AUDIO_DMA_MODE | AUDIO_DMA_RELOAD_MODE;
#endif

	switch (stream_type) {
		case AUDIO_STREAM_TTS:
        case AUDIO_STREAM_TIP:
		case AUDIO_STREAM_LINEIN:
		case AUDIO_STREAM_SPDIF_IN:
		case AUDIO_STREAM_FM:
		case AUDIO_STREAM_I2SRX_IN:
		case AUDIO_STREAM_MIC_IN:
		case AUDIO_STREAM_MUSIC:
		case AUDIO_STREAM_LOCAL_MUSIC:
		case AUDIO_STREAM_USOUND:
		case AUDIO_STREAM_VOICE:
		case AUDIO_STREAM_LE_AUDIO:
		case AUDIO_STREAM_LE_AUDIO_MUSIC:
	    case AUDIO_STREAM_TR_USOUND:
		case AUDIO_STREAM_LE_AUDIO_GAME:
			break;
	}
	return out_channel_mode;
}

int audio_policy_get_out_pcm_channel_num(uint8_t stream_type)
{
	int channel_num = 0;

	switch (stream_type) {
	case AUDIO_STREAM_TTS:
    case AUDIO_STREAM_TIP:
		break;
	case AUDIO_STREAM_VOICE:
	case AUDIO_STREAM_LE_AUDIO:
		break;

	case AUDIO_STREAM_MIC_IN:
		break;
	default: /* decoder decided */
		channel_num = 0;
		break;
	}

#if (!defined CONFIG_SOC_SERIES_LARK && !defined CONFIG_SOC_SERIES_CUCKOO)
	/* FIXME: At Linkage mode, DACHNUM=1 will output stereo still, Needn't force stereo setting. */
	if ((user_policy && (user_policy->audio_out_channel & AUDIO_CHANNEL_I2STX))
	|| (user_policy && (user_policy->audio_out_channel & AUDIO_CHANNEL_SPDIFTX))) {
		channel_num = 2;
	}
#endif

	return channel_num;
}

int audio_policy_get_out_pcm_frame_size(uint8_t stream_type)
{
	int frame_size = 512;

	switch (stream_type) {
		case AUDIO_STREAM_TTS:
        case AUDIO_STREAM_TIP:
			frame_size = 4 * 960;
			break;
		case AUDIO_STREAM_USOUND:
		case AUDIO_STREAM_LINEIN:
		case AUDIO_STREAM_FM:
		case AUDIO_STREAM_I2SRX_IN:
		case AUDIO_STREAM_MIC_IN:
		case AUDIO_STREAM_SPDIF_IN:
		case AUDIO_STREAM_MUSIC:
		case AUDIO_STREAM_LOCAL_MUSIC:
			frame_size = 2048;
			break;
		case AUDIO_STREAM_VOICE:
			frame_size = 512;
			break;
		case AUDIO_STREAM_LE_AUDIO:
			frame_size = 4 * 640;
			break;
		case AUDIO_STREAM_LE_AUDIO_MUSIC:
	    case AUDIO_STREAM_TR_USOUND:
		case AUDIO_STREAM_LE_AUDIO_GAME:
			frame_size = 4 * 960;
			break;
		default:
			break;
	}

	return frame_size;
}

/* in usec */
int audio_policy_get_out_input_start_threshold(uint8_t stream_type, uint8_t exf_stream_type, uint8_t sample_rate, uint8_t channels, uint8_t tws_mode, uint8_t format)
{
	switch (stream_type) {
	case AUDIO_STREAM_MUSIC:
		if (exf_stream_type == AUDIO_STREAM_MUSIC) {
		#ifdef CONFIG_TWS
        #ifdef CONFIG_SNOOP_LINK_TWS
            if (user_policy) {
                if(btif_tws_get_dev_role() == BTSRV_TWS_NONE) {
                    switch(format) {
                    case AAC_TYPE:
                        return user_policy->player_config.BM_AAC_Playing_CacheData*1000;
                    case SBC_TYPE:
                        return user_policy->player_config.BM_SBC_Playing_CacheData*1000;
                    default:
                        return user_policy->player_config.BM_StartPlay_Normal*1000;
                    }
                } else {
                    return user_policy->player_config.BM_StartPlay_TWS*1000;
                }
            } else 
        #else
			if (!btif_tws_check_is_woodpecker() 
					&& btif_tws_get_dev_role() == BTSRV_TWS_SLAVE) {
				return 6*1000;
			} else 
        #endif  //end CONFIG_SNOOP_LINK_TWS
		#endif  //end CONFIG_TWS
			{
				if (audiolcy_is_low_latency_mode()) {
					return 15*1000;
				} else {
					return 0;
				}
			}
		} else if (exf_stream_type == AUDIO_STREAM_USOUND) {
			return 32*1000;
		} else if (exf_stream_type == AUDIO_STREAM_LINEIN) {
			/* For us281b linein tws is latency, need start with samll threshold */
			/* return 12 * 119; */
			return 6*1000;
		} else if (exf_stream_type == AUDIO_STREAM_FM) {
			return 32*1000;
		} else if (exf_stream_type == AUDIO_STREAM_LOCAL_MUSIC) {
			return 60*1000;
		}
	case AUDIO_STREAM_VOICE:
    #ifdef CONFIG_SNOOP_LINK_TWS
        if (user_policy) {
            if(btif_tws_get_dev_role() == BTSRV_TWS_NONE) {
                switch(format) {
                case CVSD_TYPE:
                    return user_policy->player_config.BS_CVSD_Playing_CacheData*1000;
                case MSBC_TYPE:
                    return user_policy->player_config.BS_MSBC_Playing_CacheData*1000;
                default:
                    return user_policy->player_config.BS_StartPlay_Normal*1000;
                }
            } else {
                return user_policy->player_config.BS_StartPlay_TWS*1000;
            }
        }
    #else
		if (audiolcy_is_low_latency_mode()) {
			return 0;
		} else {
			return 60*1000;
		}
    #endif  //end CONFIG_SNOOP_LINK_TWS
        break;
		
	case AUDIO_STREAM_USOUND:
	case AUDIO_STREAM_LINEIN:
	case AUDIO_STREAM_I2SRX_IN:
	case AUDIO_STREAM_SPDIF_IN:
	case AUDIO_STREAM_FM:
	case AUDIO_STREAM_LE_AUDIO:
	case AUDIO_STREAM_LE_AUDIO_MUSIC:
	case AUDIO_STREAM_TR_USOUND:
	case AUDIO_STREAM_LE_AUDIO_GAME:
		return 0;
	case AUDIO_STREAM_TTS:
    case AUDIO_STREAM_TIP:
	default:
		return 0;
	}
    return 0;
}

/* in usec */
int audio_policy_get_out_input_stop_threshold(uint8_t stream_type, uint8_t exf_stream_type, uint8_t sample_rate, uint8_t channels, uint8_t tws_mode)
{
	switch (stream_type) {
	case AUDIO_STREAM_MUSIC:
		if (exf_stream_type == AUDIO_STREAM_LINEIN) {
			return 0;
		}
		return 2*1000;
	case AUDIO_STREAM_VOICE:
	case AUDIO_STREAM_TTS:
    case AUDIO_STREAM_TIP:
	case AUDIO_STREAM_LE_AUDIO:
	case AUDIO_STREAM_LE_AUDIO_MUSIC:
	case AUDIO_STREAM_TR_USOUND:
	case AUDIO_STREAM_LE_AUDIO_GAME:
	default:
		return 0;
	}
}

int audio_policy_get_out_audio_mode(uint8_t stream_type)
{
	int audio_mode = AUDIO_MODE_STEREO;

	switch (stream_type) {
		/**283D tts is mono ,but zs285A tts is stereo*/
		/* case AUDIO_STREAM_TTS: */
		case AUDIO_STREAM_VOICE:
		case AUDIO_STREAM_LE_AUDIO:
		case AUDIO_STREAM_LOCAL_RECORD:
		case AUDIO_STREAM_GMA_RECORD:
		case AUDIO_STREAM_MIC_IN:
		audio_mode = AUDIO_MODE_MONO;
		break;
	}

	return audio_mode;
}

uint8_t audio_policy_get_out_effect_type(uint8_t stream_type,
			uint8_t efx_stream_type, bool is_tws)
{
	switch (stream_type) {
	case AUDIO_STREAM_LOCAL_MUSIC:
		if (is_tws)
			efx_stream_type = AUDIO_STREAM_TWS;
		break;
	case AUDIO_STREAM_MUSIC:
		if (is_tws && efx_stream_type == AUDIO_STREAM_LINEIN) {
			efx_stream_type = AUDIO_STREAM_LINEIN;
		}
		break;
	default:
		break;
	}

	return efx_stream_type;
}

uint8_t audio_policy_get_record_effect_type(uint8_t stream_type,
			uint8_t efx_stream_type)
{
	return efx_stream_type;
}

int audio_policy_get_record_audio_mode(uint8_t stream_type, uint8_t ext_stream_type)
{
	int audio_mode = AUDIO_MODE_STEREO;

	switch (stream_type) {
    case AUDIO_STREAM_VOICE:
	case AUDIO_STREAM_LE_AUDIO:
	case AUDIO_STREAM_LE_AUDIO_MUSIC:
	case AUDIO_STREAM_TR_USOUND:
	case AUDIO_STREAM_LE_AUDIO_GAME:
    case AUDIO_STREAM_LOCAL_RECORD:
    case AUDIO_STREAM_GMA_RECORD:
    case AUDIO_STREAM_MIC_IN:
		audio_mode = AUDIO_MODE_MONO;
		break;
	}
    
    if (user_policy && user_policy->record_configs_num) {
        int i;
        for(i=0; i<user_policy->record_configs_num; i++) {
            if(user_policy->record_configs[i].stream_type == ext_stream_type) {
                audio_mode = user_policy->record_configs[i].channels;
            }
        }
    }

    SYS_LOG_INF("audio_mode:%d\n", audio_mode);
	return audio_mode;
}

int audio_policy_get_record_channel_id(uint8_t stream_type, uint8_t ext_stream_type)
{
	int channel_id = AUDIO_ANALOG_MIC0;

	switch (stream_type) {
	case AUDIO_STREAM_USOUND:
	case AUDIO_STREAM_VOICE:
	case AUDIO_STREAM_LOCAL_RECORD:
	case AUDIO_STREAM_GMA_RECORD:
	case AUDIO_STREAM_MIC_IN:
	case AUDIO_STREAM_MUSIC:
	case AUDIO_STREAM_LOCAL_MUSIC:
	case AUDIO_STREAM_ASR:
	case AUDIO_STREAM_LE_AUDIO:
	case AUDIO_STREAM_LE_AUDIO_MUSIC:
	case AUDIO_STREAM_TR_USOUND:
	case AUDIO_STREAM_LE_AUDIO_GAME:
		channel_id = AUDIO_ANALOG_MIC0;
		break;
	case AUDIO_STREAM_LINEIN:
		channel_id = AUDIO_LINE_IN0;
		break;
	case AUDIO_STREAM_FM:
		channel_id = AUDIO_ANALOG_FM0;
		break;
	}

    if (user_policy && user_policy->record_configs_num) {
        int i;
        for(i=0; i<user_policy->record_configs_num; i++) {
            if(user_policy->record_configs[i].stream_type == ext_stream_type) {
                channel_id = user_policy->record_configs[i].audio_device;
            }
        }
    }
    
    SYS_LOG_INF("chan_id:%d\n", channel_id);
	return channel_id;
}

uint8_t audio_policy_get_record_channel_swap(uint8_t stream_type, uint8_t ext_stream_type)
{
	uint8_t swap = 0;

    if (user_policy && user_policy->record_configs_num) {
        int i;
        for(i=0; i<user_policy->record_configs_num; i++) {
            if(user_policy->record_configs[i].stream_type == ext_stream_type) {
                swap = user_policy->record_configs[i].dual_mic_swap;
            }
        }
    }
    
	return swap;
}

int32_t audio_policy_get_record_main_mic_gain(uint8_t stream_type, uint8_t ext_stream_type)
{
	int32_t main_mic_gain = 0;

    if (user_policy && user_policy->record_configs_num) {
        int i;
        for(i=0; i<user_policy->record_configs_num; i++) {
            if(user_policy->record_configs[i].stream_type == ext_stream_type) {
                main_mic_gain = user_policy->record_configs[i].main_mic_gain;
            }
        }
    }
    
	return main_mic_gain;
}

int audio_policy_is_master_mix_channel(uint8_t stream_type)
{
    if(stream_type == AUDIO_STREAM_LINEIN_MIX 
		|| stream_type == AUDIO_STREAM_USOUND_MIX){
        return true;
    }else{
        return false;
    }
}

int audio_policy_get_record_aec_block_size(uint8_t format_type)
{
	int block_size = 0;
/*	if (format_type == MSBC_TYPE) {
		block_size = 256 * 2 - 192;
	}else if (format_type == CVSD_TYPE){
		block_size = 256 * 2 - 128;
	}*/

	return block_size;
}

int audio_policy_get_record_channel_mix_channel(uint8_t stream_type)
{
    int mix_channel = false;

    switch (stream_type){
        case AUDIO_STREAM_LINEIN_MIX:
            mix_channel = true;
            break;
        case AUDIO_STREAM_USOUND_MIX:
            mix_channel = true;
            break;
    }

    return mix_channel;
}

int audio_policy_get_record_adc_gain(uint8_t stream_type)
{
	int gain = 0;

	return gain;
}

int audio_policy_get_record_input_gain(uint8_t stream_type, audio_input_gain *input_gain)
{
    assert(input_gain);

    memset(input_gain, 0, sizeof(audio_input_gain));
    switch (stream_type) {
    case AUDIO_STREAM_LINEIN:
        if (user_policy)
            *input_gain = user_policy->audio_in_linein_gain;
        break;
    case AUDIO_STREAM_FM:
        if (user_policy)
            *input_gain = user_policy->audio_in_fm_gain;
        break;
    case AUDIO_STREAM_USOUND:
    case AUDIO_STREAM_VOICE:
    case AUDIO_STREAM_LOCAL_RECORD:
    case AUDIO_STREAM_GMA_RECORD:
    case AUDIO_STREAM_MIC_IN:
	case AUDIO_STREAM_ASR:
	case AUDIO_STREAM_LE_AUDIO:
	case AUDIO_STREAM_LE_AUDIO_MUSIC:
	case AUDIO_STREAM_TR_USOUND:
	case AUDIO_STREAM_LE_AUDIO_GAME:
        if (user_policy)
            *input_gain = user_policy->audio_in_mic_gain;
        break;
    case AUDIO_STREAM_AI:
        if (user_policy)
            *input_gain = user_policy->audio_in_mic_gain;
        break;
    }

    return 0;
}

int audio_policy_get_record_channel_mode(uint8_t stream_type)
{
	int channel_mode =  AUDIO_DMA_MODE | AUDIO_DMA_RELOAD_MODE;

	switch (stream_type) {

	}

	return channel_mode;
}

int audio_policy_get_record_channel_support_aec(uint8_t stream_type)
{
	int support_aec = false;

	switch (stream_type) {
		case AUDIO_STREAM_VOICE:
		case AUDIO_STREAM_LE_AUDIO:
		case AUDIO_STREAM_LE_AUDIO_MUSIC:
		case AUDIO_STREAM_TR_USOUND:
		case AUDIO_STREAM_USOUND:
		case AUDIO_STREAM_LE_AUDIO_GAME:
		support_aec = true;
		break;
	}

    return support_aec;
}

int audio_policy_get_record_channel_aec_tail_length(uint8_t stream_type, uint8_t sample_rate, bool in_debug)
{
	switch (stream_type) {
	case AUDIO_STREAM_VOICE:
	case AUDIO_STREAM_LE_AUDIO:
	case AUDIO_STREAM_LE_AUDIO_MUSIC:
	case AUDIO_STREAM_TR_USOUND:
	case AUDIO_STREAM_LE_AUDIO_GAME:
		if (in_debug) {
			return (sample_rate > 8) ? 32 : 64;
		} else if (user_policy) {
			return (sample_rate > 8) ?
				user_policy->voice_aec_tail_length_16k :
				user_policy->voice_aec_tail_length_8k;
		} else {
			return (sample_rate > 8) ? 48 : 96;
		}
	default:
		return 0;
	}
}

int audio_policy_get_record_max_pkt_in_queue(uint8_t stream_type, uint8_t format)
{
#ifdef CONFIG_SNOOP_LINK_TWS
	if (user_policy) {
        return user_policy->player_config.BS_MIC_Playing_PKTCNT;
	}
#endif
    return 3;
}

int audio_policy_is_out_channel_aec_reference(uint8_t stream_type)
{
	if(stream_type == AUDIO_STREAM_VOICE||
       stream_type == AUDIO_STREAM_LE_AUDIO||
	   stream_type == AUDIO_STREAM_LE_AUDIO_MUSIC||
	   stream_type == AUDIO_STREAM_TR_USOUND||
	   stream_type == AUDIO_STREAM_USOUND||
	   stream_type == AUDIO_STREAM_LE_AUDIO_GAME){
        return true;
    }else{
        return false;
    }
}

int audio_policy_get_out_channel_aec_reference_stream_type(uint8_t stream_type)
{
	if(stream_type == AUDIO_STREAM_VOICE||
       stream_type == AUDIO_STREAM_LE_AUDIO||
	   stream_type == AUDIO_STREAM_LE_AUDIO_MUSIC||
	   stream_type == AUDIO_STREAM_TR_USOUND||
	   stream_type == AUDIO_STREAM_USOUND||
	   stream_type == AUDIO_STREAM_LE_AUDIO_GAME){
        return AUDIO_MODE_MONO;
    }else{
        return 0;
    }
}

int audio_policy_get_channel_resample(uint8_t stream_type)
{
    int resample = false;

    switch (stream_type) {
	case AUDIO_STREAM_MUSIC:
	    /* resample = true; */
	break;
    }

    return resample;
}

int audio_policy_get_output_support_multi_track(uint8_t stream_type)
{
	int support_multi_track = false;

#ifdef CONFIG_AUDIO_SUBWOOFER
	switch (stream_type) {
		case AUDIO_STREAM_MUSIC:
		case AUDIO_STREAM_LINEIN:
		case AUDIO_STREAM_MIC_IN:
		case AUDIO_STREAM_USOUND:
			support_multi_track = true;
			break;
	}
#endif

	return support_multi_track;
}

int audio_policy_get_record_channel_type(uint8_t stream_type)
{
	int channel_type = AUDIO_CHANNEL_ADC;

	switch (stream_type) {
	case AUDIO_STREAM_SPDIF_IN:
		channel_type = AUDIO_CHANNEL_SPDIFRX;
		break;

	case AUDIO_STREAM_I2SRX_IN:
		channel_type = AUDIO_CHANNEL_I2SRX;
		break;
	}
	return channel_type;
}

int audio_policy_get_pa_volume(uint8_t stream_type, uint8_t volume_level)
{
	int pa_volume = volume_level;

	if (!user_policy)
		goto exit;

	switch (stream_type) {
		case AUDIO_STREAM_VOICE:
		case AUDIO_STREAM_LE_AUDIO:
            if (volume_level > user_policy->voice_max_volume) {
        		volume_level = user_policy->voice_max_volume;
        	}
			pa_volume = user_policy->voice_pa_table[volume_level];
			break;
#if 1
        case AUDIO_STREAM_USOUND:
			if (volume_level > 15) {
				volume_level = 15;
			}
			pa_volume = user_policy->usound_pa_table[volume_level];
			break;
#endif
        case AUDIO_STREAM_TTS:
            if (volume_level > user_policy->tts_max_volume) {
        		volume_level = user_policy->tts_max_volume;
        	} else if (volume_level < user_policy->tts_min_volume) {
        		volume_level = user_policy->tts_min_volume;
        	}

			pa_volume = user_policy->tts_pa_table[volume_level];
			break;
		case AUDIO_STREAM_LINEIN:
        case AUDIO_STREAM_FM:
        case AUDIO_STREAM_I2SRX_IN:
		case AUDIO_STREAM_MIC_IN:
		case AUDIO_STREAM_MUSIC:
		case AUDIO_STREAM_LOCAL_MUSIC:
		case AUDIO_STREAM_SPDIF_IN:
		case AUDIO_STREAM_TIP:
		case AUDIO_STREAM_LE_AUDIO_MUSIC:
		case AUDIO_STREAM_TR_USOUND:
        //case AUDIO_STREAM_USOUND:
		case AUDIO_STREAM_LE_AUDIO_GAME:
            if (volume_level > user_policy->music_max_volume) {
        		volume_level = user_policy->music_max_volume;
        	}
			pa_volume = user_policy->music_pa_table[volume_level];
			break;
	}

exit:
	return pa_volume;
}

int audio_policy_get_da_volume(uint8_t stream_type, uint8_t volume_level)
{
	int da_volume = volume_level;

	if (!user_policy)
		goto exit;

	switch (stream_type) {
	case AUDIO_STREAM_VOICE:
	case AUDIO_STREAM_LE_AUDIO:
        if (volume_level > user_policy->voice_max_volume) {
    		volume_level = user_policy->voice_max_volume;
    	}
		da_volume = user_policy->voice_da_table[volume_level];
		break;
	case AUDIO_STREAM_USOUND:
		if (volume_level > 15) {
			volume_level = 15;
		}
		da_volume = user_policy->usound_da_table[volume_level];
		break;
    case AUDIO_STREAM_TTS:
        if (volume_level > user_policy->tts_max_volume) {
    		volume_level = user_policy->tts_max_volume;
    	} else if (volume_level < user_policy->tts_min_volume) {
    		volume_level = user_policy->tts_min_volume;
    	}
		da_volume = user_policy->tts_da_table[volume_level];
		break;
	case AUDIO_STREAM_LINEIN:
	case AUDIO_STREAM_FM:
	case AUDIO_STREAM_I2SRX_IN:
	case AUDIO_STREAM_MIC_IN:
	case AUDIO_STREAM_SPDIF_IN:
	case AUDIO_STREAM_MUSIC:
	case AUDIO_STREAM_LOCAL_MUSIC:
	case AUDIO_STREAM_TIP:
	case AUDIO_STREAM_LE_AUDIO_MUSIC:
	case AUDIO_STREAM_TR_USOUND:
	case AUDIO_STREAM_LE_AUDIO_GAME:
		da_volume = user_policy->music_da_table[volume_level];
		break;
	}

exit:
	return da_volume;
}

int audio_policy_get_volume_level_by_db(uint8_t stream_type, int volume_db)
{
	int volume_level = 0;
	int max_level = 0;
	const int *volume_table = NULL;

	if (!user_policy)
		goto exit;

	switch (stream_type) {
	case AUDIO_STREAM_VOICE:
	case AUDIO_STREAM_LE_AUDIO:
		volume_table = user_policy->voice_pa_table;
        max_level = user_policy->voice_max_volume;
		break;
	case AUDIO_STREAM_USOUND:
		volume_table = user_policy->usound_pa_table;
		max_level = 16;
		break;
    case AUDIO_STREAM_TTS:
    case AUDIO_STREAM_TIP:
        volume_table = user_policy->tts_pa_table;
        //max_level = user_policy->tts_max_volume;
        max_level = user_policy->music_max_volume;
		break;
	case AUDIO_STREAM_LINEIN:
	case AUDIO_STREAM_FM:
	case AUDIO_STREAM_I2SRX_IN:
	case AUDIO_STREAM_MIC_IN:
	case AUDIO_STREAM_SPDIF_IN:
	case AUDIO_STREAM_MUSIC:
	case AUDIO_STREAM_LOCAL_MUSIC:
	case AUDIO_STREAM_LE_AUDIO_MUSIC:
	case AUDIO_STREAM_TR_USOUND:
	case AUDIO_STREAM_LE_AUDIO_GAME:
    default:
        max_level = user_policy->music_max_volume;
		volume_table = user_policy->music_pa_table;
		break;
	}

	/* to 0.001 dB */
	volume_db *= 100;

	if (!volume_table)
		goto exit;
		
	if (volume_db == volume_table[max_level - 1]) {
		volume_level = max_level;
	} else {
		for (int i = 0; i < max_level; i++) {
			if (volume_db < volume_table[i]) {
				volume_level = i;
				break;
			}
		}
	}

exit:
	return volume_level;
}

int audio_policy_get_aec_reference_type(void)
{
#ifdef CONFIG_AUDIO_VOICE_HARDWARE_REFERENCE
    return AEC_REF_TYPE_HW;
#else
    return AEC_REF_TYPE_SF;
#endif
}

/*FIXME:le audio tdb?*/
int audio_policy_get_aec_reference_sample_bits(void)
{
#ifdef CONFIG_SOC_SERIES_CUCKOO
	return 24;
#else
	return 16;
#endif
}

int audio_policy_get_aec_reference_gain_enable(uint8_t stream_type)
{
    bool reference_gain_enable = false;

	switch (stream_type) {
	case AUDIO_STREAM_VOICE:
		reference_gain_enable = true;
		break;
	case AUDIO_STREAM_LE_AUDIO:
		reference_gain_enable = true;
		break;
	case AUDIO_STREAM_LE_AUDIO_MUSIC:
	case AUDIO_STREAM_LE_AUDIO_GAME:
		reference_gain_enable = false;
		break;
    default:
        break;
	}

	return reference_gain_enable;
}

int audio_policy_get_max_volume(uint8_t stream_type)
{
    int volume;
    
	if (!user_policy)
		return 16;

	switch (stream_type) {
	case AUDIO_STREAM_VOICE:
	case AUDIO_STREAM_LE_AUDIO:
        volume = user_policy->voice_max_volume;
		break;
	case AUDIO_STREAM_TR_USOUND:
	case AUDIO_STREAM_USOUND:
		volume = 16;
		break;
    case AUDIO_STREAM_TTS:
    //case AUDIO_STREAM_TIP:
        volume = user_policy->tts_max_volume;
		break;
	default:
        volume = user_policy->music_max_volume;
		break;
	}

    return volume;
}

int audio_policy_get_min_volume(uint8_t stream_type)
{
    int volume;
    
	if (!user_policy)
		return 0;

	switch (stream_type) {
    case AUDIO_STREAM_TTS:
    //case AUDIO_STREAM_TIP:
        volume = user_policy->tts_min_volume;
		break;
	default:
        volume = 0;
		break;
	}

    return volume;
}

int audio_policy_check_tts_fixed_volume(void)
{
	if (user_policy)
		return user_policy->tts_fixed_volume;

	return 1;
}

int audio_policy_get_tone_fixed_volume_decay(void)
{
	if (user_policy)
		return user_policy->tone_fixed_volume_decay;

	return 0;
}

aset_volume_table_v2_t *audio_policy_get_aset_volume_table(void)
{
	if (user_policy)
		return user_policy->aset_volume_table;

	return NULL;
}

int audio_policy_check_save_volume_to_nvram(void)
{
	if (user_policy)
		return user_policy->volume_saveto_nvram;

	return 1;
}

/* in usec */
int audio_policy_get_reduce_threshold(u8_t low_lcymode, int format)
{
	if (format == NAV_TYPE) {
		return 1*1000;
	}

	if (low_lcymode) {
		if (format == MSBC_TYPE || format == CVSD_TYPE) {
			return 5*1000; 
		} else {
		#ifdef CONFIG_TWS
			if (btif_tws_get_dev_role() == BTSRV_TWS_NONE) {
				return 15*1000;
			} else {
				return 30*1000;
			}
		#else
			return 15*1000;
		#endif
		}
	} else {
		if (format == MSBC_TYPE || format == CVSD_TYPE) {
			return 30*1000;
		} else if (format == PCM_TYPE) {
			return 16*1000;
		} else {
			return 100*1000;
		}
	}
}

/* in usec */
int audio_policy_get_increase_threshold(u8_t low_lcymode, int format)
{
#ifdef CONFIG_SNOOP_LINK_TWS
    int threshold = 0;

    if (user_policy) {
        switch(format) {
        case CVSD_TYPE:
            threshold = user_policy->player_config.BS_CVSD_Playing_CacheData;
            break;
        case MSBC_TYPE:
            threshold = user_policy->player_config.BS_MSBC_Playing_CacheData;
            break;
        case SBC_TYPE:
            if (low_lcymode) {
                threshold = user_policy->player_config.BM_SBC_Threshold_A_LLM;
            } else {
                threshold = user_policy->player_config.BM_SBC_Playing_CacheData;
            }
            break;
        case AAC_TYPE:
            if (low_lcymode) {
                threshold = user_policy->player_config.BM_AAC_Threshold_A_LLM;
            } else {
                threshold = user_policy->player_config.BM_AAC_Playing_CacheData;
            }
            break;
        case NAV_TYPE:
            threshold = 18;
            break;
        case PCM_DSP_TYPE:
            //threshold = 6;
#if (CONFIG_NRF_CHANNEL_NUM == 2)
            threshold = 6;
#else
            threshold = 4;
#endif
            break;
        default :
            break;
        }
    }

    if(0 == threshold)
        threshold = 100;
    return threshold*1000;
#else

	if (low_lcymode) {
		if (format == MSBC_TYPE || format == CVSD_TYPE) {
			return 15*1000;
		} else {
		#ifdef CONFIG_TWS
			if (btif_tws_get_dev_role() == BTSRV_TWS_NONE) {
				return 50*1000;
			} else {
				return 50*1000;
			}
		#else
			return 50*1000;
		#endif
		}
	} else {
		if (format == MSBC_TYPE || format == CVSD_TYPE) {
			return 50*1000;
		} else if (format == PCM_TYPE) {
			return 32*1000;
		} else {
			return 200*1000;
		}
	}

#endif  //end CONFIG_SNOOP_LINK_TWS
}

int audio_policy_check_snoop_tws_support(void)
{
#ifdef CONFIG_SNOOP_LINK_TWS
    return 1;
#else
    return 0;
#endif
}

int audio_policy_get_snoop_tws_sync_param(uint8_t format,
    uint16_t *negotiate_time, uint16_t *negotiate_timeout, uint16_t *sync_interval)
{
    assert(negotiate_time);
    assert(negotiate_timeout);
    assert(sync_interval);

#ifdef CONFIG_SNOOP_LINK_TWS
    if (user_policy) {
        switch(format) {
        case CVSD_TYPE:
        case MSBC_TYPE:
            *negotiate_time = user_policy->player_config.BS_TWS_WPlay_Mintime;
            *negotiate_timeout = user_policy->player_config.BS_TWS_WPlay_Maxtime;
            *sync_interval = user_policy->player_config.BS_TWS_Sync_interval;
            break;
        default:
            *negotiate_time = user_policy->player_config.BM_TWS_WPlay_Mintime;
            *negotiate_timeout = user_policy->player_config.BM_TWS_WPlay_Maxtime;
            *sync_interval = user_policy->player_config.BM_TWS_Sync_interval;
            break;
        }
    }
#endif

    if(0 == *negotiate_time) {
        *negotiate_time = 100;
    }
    if(0 == *negotiate_timeout) {
        *negotiate_timeout = 1000;
    }
    if(0 == *sync_interval) {
        *sync_interval = 100;
    }
    return 0;
}

int audio_policy_get_pcmbuf_time(uint8_t stream_type)
{
    int buffer_time = 0;

#ifdef CONFIG_SNOOP_LINK_TWS
    if (user_policy) {
        switch(stream_type) {
        case AUDIO_STREAM_MUSIC:
            buffer_time = user_policy->player_config.BM_Max_PCMBUF_Time;
            break;
        case AUDIO_STREAM_VOICE:
            buffer_time = user_policy->player_config.BS_Max_PCMBUF_Time;
            break;
        default:
            break;
        }
    }
#endif
    
    return buffer_time;
}

void audio_policy_set_a2dp_lantency_time(int low_latencey_ms)
{
    a2dp_lantency_ms = low_latencey_ms;
}

int audio_policy_get_a2dp_lantency_time(void)
{
	return a2dp_lantency_ms;
}

int audio_policy_get_automute_force(uint8_t stream_type)
{
	if (user_policy) {
		return user_policy->automute_force;
	}
	return 0;
}

int audio_policy_register(const struct audio_policy_t *policy)
{
	user_policy = policy;
	return 0;
}
