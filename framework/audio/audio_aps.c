/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio stream.
*/

#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <audio_hal.h>
#include <audio_system.h>
#include <audio_track.h>
#include <audiolcy_common.h>
#include <media_type.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stream.h>

#ifdef CONFIG_BLUETOOTH
#include <bt_manager.h>
#endif

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "audio_aps"

#define AUDIO_APS_ADJUST_INTERVAL            30
#define ABS(a) ((a)>0?(a):-(a))

static aps_monitor_info_t aps_monitor;

aps_monitor_info_t *audio_aps_monitor_get_instance(void)
{
	return &aps_monitor;
}

void audio_aps_monitor_set_aps(void *audio_handle, uint8_t status, int level)
{
	aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
	uint8_t aps_mode = (handle->audiopll_mode == 2)? (APS_LEVEL_AUDIOPLL_F): (APS_LEVEL_AUDIOPLL);
    uint8_t set_aps = false;

    if (status & APS_OPR_SET) {
       handle->dest_level = level;
    }

    if (status & APS_OPR_FAST_SET) {
		handle->dest_level = level;
		handle->current_level = level;
		set_aps = true;
    }

    if (handle->current_level > handle->dest_level) {
		handle->current_level--;
		set_aps = true;
    }
    if (handle->current_level < handle->dest_level) {
		handle->current_level++;
		set_aps = true;
    }

    if (set_aps) {
		SYS_LOG_DBG("adjust mode %d: %d\n", aps_mode, handle->current_level);
		hal_aout_channel_set_aps(audio_handle, handle->current_level, aps_mode);
    }
}

#ifdef CONFIG_AUDIO_APS_ADJUST_FINE
static uint8_t _monitor_aout_rate_fine(aps_monitor_info_t *handle, int32_t pcm_time, int32_t is_tws)
{
    int32_t max_threshold = (int32_t)handle->aps_increase_water_mark;
    int32_t max_diff_threshold = (int32_t)handle->aps_water_adjust_thres;
	int32_t aps_0ppm_threshold = (max_diff_threshold/3 < 6)? (6): (max_diff_threshold/3);
    uint8_t cur_level = handle->current_level;
    uint8_t adj_level = cur_level;
    int32_t time_diff_abs;
	uint32_t cur_time = k_uptime_get_32();

    ARG_UNUSED(is_tws);

    time_diff_abs = abs((int32_t)(pcm_time - max_threshold));

    if(handle->aps_fine_flag == 0) {
        max_diff_threshold = (max_diff_threshold < 18)? (18): (max_diff_threshold);
        if(time_diff_abs <= max_diff_threshold) {
            printk("aps fine start\n");
            handle->aps_fine_flag = 1;
            /* start adjust fine */
			if(handle->audiopll_mode == 2) {
            	adj_level = 16;
			} else {
            	adj_level = APS_LEVEL_0PPM;
			}
            handle->last_level = adj_level;
			handle->aps_fine_adjust_time = cur_time;
			handle->aps_water_adjust_last = pcm_time;
            return adj_level;
        }
    }

	if(handle->audiopll_mode == 2) {
		if (pcm_time > max_threshold + max_diff_threshold) {
			if (handle->last_level <= 16) {
				adj_level = 18;
			} else if (handle->last_level <= 21) {
				if (cur_time - handle->aps_fine_adjust_time > 300 && pcm_time > handle->aps_water_adjust_last) {
					adj_level = handle->last_level + 1;
				}
			}
		} else if (pcm_time < max_threshold - max_diff_threshold) {
			if (handle->last_level >= 16) {
				adj_level = 14;
			} else if (handle->last_level >= 11) {
				if (cur_time - handle->aps_fine_adjust_time > 300 && pcm_time < handle->aps_water_adjust_last) {
					adj_level = handle->last_level - 1;
				}
			}
		}
		//else if (time_diff_abs <= aps_0ppm_threshold) {
		//    adj_level = 16;
		//}
	} else {
		if (pcm_time > max_threshold + max_diff_threshold) {
			adj_level = APS_LEVEL_5;
		} else if (pcm_time < max_threshold - max_diff_threshold) {
			adj_level = APS_LEVEL_4;
		} else if (time_diff_abs <= aps_0ppm_threshold) {
			adj_level = APS_LEVEL_0PPM;
		}
	}

    if (adj_level != handle->last_level) {
        //printk("aps fine adjust to %d_%d_%d_%d\n", adj_level, handle->last_level, pcm_time, max_threshold);
        handle->last_level = adj_level;
		handle->aps_fine_adjust_time = cur_time;
		handle->aps_water_adjust_last = pcm_time;
        return adj_level;
    } else {
        return 0xFF;
    }
}
#endif

void audio_aps_monitor_normal(aps_monitor_info_t *handle, int stream_length_us, uint8_t aps_max_level, uint8_t aps_min_level, uint8_t aps_level)
{
	void *audio_handle = handle->audio_track->audio_handle;
	uint32_t mid_threshold_us = 0;
	uint32_t diff_threshold_us = 0;
	/* printk("---in stream --- %d out stream %d\n",stream_length, stream_get_length(audio_track_get_stream(handle->audio_track))); */

	if (!handle->need_aps) {
		return;
	}

#ifdef CONFIG_AUDIO_APS_ADJUST_FINE
	if(handle->need_aps_fine == 1) {
		uint8_t ret = 0xFF;
		ret = _monitor_aout_rate_fine(handle, stream_length_us, 0);
		if(ret != 0xFF) {
			uint8_t aps_mode = (handle->audiopll_mode == 2)? (APS_LEVEL_AUDIOPLL_F): (APS_LEVEL_AUDIOPLL);
			handle->current_level = ret;
			hal_aout_channel_set_aps(handle->audio_track->audio_handle, handle->current_level, aps_mode);
		}
		return;
	}
#endif

	diff_threshold_us = (handle->aps_increase_water_mark - handle->aps_reduce_water_mark);
    mid_threshold_us = handle->aps_increase_water_mark - (diff_threshold_us / 2);

    switch (handle->aps_status) {
	case APS_STATUS_DEFAULT:
	    if (stream_length_us > handle->aps_increase_water_mark) {
		SYS_LOG_DBG("inc aps\n");
		audio_aps_monitor_set_aps(audio_handle, APS_OPR_FAST_SET, aps_max_level);
		handle->aps_status = APS_STATUS_INC;
	    } else if (stream_length_us < handle->aps_reduce_water_mark) {
		SYS_LOG_DBG("fast dec aps\n");
		audio_aps_monitor_set_aps(audio_handle, APS_OPR_FAST_SET, aps_min_level);
		handle->aps_status = APS_STATUS_DEC;
	    } else {
		audio_aps_monitor_set_aps(audio_handle, APS_OPR_ADJUST, 0);
	    }
	    break;

	case APS_STATUS_INC:
	    if (stream_length_us < handle->aps_reduce_water_mark) {
		SYS_LOG_DBG("fast dec aps\n");
		audio_aps_monitor_set_aps(audio_handle, APS_OPR_FAST_SET, aps_min_level);
		handle->aps_status = APS_STATUS_DEC;
	    } else if (stream_length_us <= mid_threshold_us) {
		SYS_LOG_DBG("default aps\n");
		audio_aps_monitor_set_aps(audio_handle, APS_OPR_SET, aps_level);
		handle->aps_status = APS_STATUS_DEFAULT;
	    } else {
		audio_aps_monitor_set_aps(audio_handle, APS_OPR_ADJUST, 0);
	    }
	    break;

	case APS_STATUS_DEC:
	    if (stream_length_us > handle->aps_increase_water_mark) {
		SYS_LOG_DBG("fast inc aps\n");
		audio_aps_monitor_set_aps(audio_handle, APS_OPR_FAST_SET,  aps_max_level);
		handle->aps_status = APS_STATUS_INC;
	    } else if (stream_length_us >= mid_threshold_us) {
		SYS_LOG_DBG("default aps\n");
		audio_aps_monitor_set_aps(audio_handle, APS_OPR_SET, aps_level);
		handle->aps_status = APS_STATUS_DEFAULT;
	    } else {
		audio_aps_monitor_set_aps(audio_handle, APS_OPR_ADJUST, 0);
	    }
	    break;
    }
}

static void audio_aps_get_average(int *average, int *average_left, int len)
{
    *average = *average * 15 + len + *average_left;
    *average_left = *average - ((*average / 16) * 16);
    *average /= 16;
}

void audio_aps_monitor_normal_low_latency(aps_monitor_info_t *handle, int pcm_time)
{
	aps_monitor_info_t *aps_handle = (aps_monitor_info_t *)handle;
    
	aps_handle->len = pcm_time;

    audio_aps_monitor_normal(handle, aps_handle->len, aps_handle->aps_max_level, aps_handle->aps_min_level, aps_handle->aps_default_level);

    if(aps_handle->min_len > aps_handle->len) {
        aps_handle->min_len = aps_handle->len;
    }
    if(aps_handle->max_len < aps_handle->len) {
        aps_handle->max_len = aps_handle->len;
    }
	if((aps_handle->aps_count++ % 64) == 0) {
        int mark_adjust, min_lantency = audio_policy_get_a2dp_lantency_time();
        int expect_len = (aps_handle->max_len - aps_handle->min_len);
    
        if(aps_handle->max_len) {

            if(expect_len < min_lantency)
                expect_len = min_lantency;
            
            audio_aps_get_average(&aps_handle->average_expect_len, &aps_handle->average_expect_left, expect_len);
            audio_aps_get_average(&aps_handle->average_min_len, &aps_handle->average_min_left, aps_handle->min_len);
            audio_aps_get_average(&aps_handle->average_diff, &aps_handle->average_diff_left, ABS(aps_handle->min_len - aps_handle->average_min_len));
            audio_aps_get_average(&aps_handle->expect_diff, &aps_handle->expect_diff_left, ABS(aps_handle->min_len - aps_handle->last_expect_len));
            if(aps_handle->last_min_len) {
                if(ABS(aps_handle->last_min_len - aps_handle->min_len) > aps_handle->average_diff) {
                    aps_handle->boundary_count = 0;
                    audio_aps_get_average(&aps_handle->boundary_diff, &aps_handle->boundary_diff_left, ABS(aps_handle->last_min_len - aps_handle->min_len));
                } else if(++aps_handle->boundary_count >= 8){
                    aps_handle->boundary_count = 0;
                    audio_aps_get_average(&aps_handle->boundary_diff, &aps_handle->boundary_diff_left, 0);
                }
            }
            expect_len = aps_handle->average_expect_len + (aps_handle->average_diff * 2) + aps_handle->boundary_diff + aps_handle->expect_diff;
            
            mark_adjust = expect_len - aps_handle->min_len;
            
            aps_handle->aps_reduce_water_mark += mark_adjust;
            if(aps_handle->aps_reduce_water_mark < aps_handle->average_expect_len) {
                aps_handle->aps_reduce_water_mark = aps_handle->average_expect_len;
            } else if(aps_handle->aps_reduce_water_mark > aps_handle->average_min_len + aps_handle->average_expect_len) {
                aps_handle->aps_reduce_water_mark = aps_handle->average_min_len + aps_handle->average_expect_len;
            }
            aps_handle->aps_increase_water_mark = aps_handle->aps_reduce_water_mark + 5000;
            
            aps_handle->last_min_len = aps_handle->min_len;
            aps_handle->last_expect_len = expect_len;
        }
        
        printk("p(%d,%d,%d,%d,%d,%d,%d)(%d,%d)\n"
            , aps_handle->min_len, aps_handle->max_len, aps_handle->average_min_len, aps_handle->average_expect_len, aps_handle->average_diff, aps_handle->boundary_diff, aps_handle->expect_diff
            , expect_len, aps_handle->aps_reduce_water_mark);
        aps_handle->min_len = 20000000;
        aps_handle->max_len = 0;
	}
    
	return;
}
extern void audio_aps_monitor_slave(aps_monitor_info_t *handle, int stream_length_us, uint8_t aps_max_level, uint8_t aps_min_level, uint8_t slave_aps_level);
extern void audio_aps_monitor_master(aps_monitor_info_t *handle, int stream_length_us, uint8_t aps_max_level, uint8_t aps_min_level, uint8_t slave_aps_level);

/* Run interval DATA_PROCESS_PERIOD =  (4) */
void audio_aps_monitor(int pcm_time)
{
	aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
	static uint32_t s_time;

	/* Adjust aps every AUDIO_APS_ADJUST_INTERVAL */
	if ((k_uptime_get_32() - s_time) < handle->duration) {
		return;
	}

	s_time = k_uptime_get_32();

    if(audio_policy_get_a2dp_lantency_time())
    {
        audio_aps_monitor_normal_low_latency(handle, pcm_time);
    }
    else
    {
#ifdef CONFIG_TWS
	if (handle->role == BTSRV_TWS_NONE) {
		audio_aps_monitor_normal(handle, pcm_time, handle->aps_max_level, handle->aps_min_level, handle->aps_default_level);
	} else if (handle->role == BTSRV_TWS_MASTER) {
		audio_aps_monitor_master(handle, pcm_time, handle->aps_max_level, handle->aps_min_level, handle->aps_default_level);
	} else if (handle->role == BTSRV_TWS_SLAVE) {
		audio_aps_monitor_slave(handle, pcm_time, handle->aps_max_level, handle->aps_min_level, handle->current_level);
	}
#else
	audio_aps_monitor_normal(handle, pcm_time, handle->aps_max_level, handle->aps_min_level, handle->aps_default_level);
#endif
    }
}

void audio_aps_set_latency(int latency_us)
{
	aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
	handle->aps_increase_water_mark = latency_us;
	SYS_LOG_INF("%d", latency_us);
}

u32_t audio_aps_get_latency(void)
{
	aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
	return handle->aps_increase_water_mark;
}

u32_t audio_aps_get_samplerate_hz(u8_t is_48khz)
{
	aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
	u32_t sr_hz, aps_level;

	const u16_t srhz_table[] = {
		44000, 44024, 44062, 44084,
		44095, 44117, 44132, 44196,
		47879, 47916, 47965, 47980,
		48001, 48023, 48046, 48108
	};

	aps_level = handle->current_level;  // 0 - 7
	if (is_48khz) {
		aps_level += 8;
	}

	if (aps_level < ARRAY_SIZE(srhz_table)) {
		sr_hz = srhz_table[aps_level];
	} else {
		if (is_48khz) {
			sr_hz = 48000;
		} else {
			sr_hz = 44100;
		}
	}

	return sr_hz;
}

void audio_aps_monitor_init(uint32_t format_and_audiopll_mode, void *tws_observer, struct audio_track_t *audio_track)
{
	aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
	uint8_t format = (format_and_audiopll_mode & APS_FORMAT_MASK) >> APS_FORMAT_SHIFT;
	uint8_t audiopll_mode = (format_and_audiopll_mode & APS_AUDIOPLL_MODE_MASK) >> APS_AUDIOPLL_MODE_SHIFT;

	memset(handle, 0, sizeof(aps_monitor_info_t));
	handle->audio_track = audio_track;
	handle->aps_status = APS_STATUS_DEFAULT;
	handle->aps_min_level = APS_LEVEL_1;
	handle->aps_max_level = APS_LEVEL_8;
	handle->aps_default_level = APS_LEVEL_5;

	/* Default water mark: no aps adjustment */
	handle->aps_increase_water_mark = UINT32_MAX;
	handle->aps_reduce_water_mark = 0;
	handle->need_aps = 1;
	handle->aps_increase_water_mark = audiolcy_get_latency_threshold(format);
	handle->aps_reduce_water_mark = audiolcy_get_latency_threshold_min(format);
	handle->duration = AUDIO_APS_ADJUST_INTERVAL;
	handle->audiopll_mode = audiopll_mode;
    handle->average_expect_len = 100000;
    SYS_LOG_INF("tws_observer %p audio_track %p, water_mark: %u, %u, audiopll_mode %d\n",
        tws_observer, audio_track,
        handle->aps_increase_water_mark,
        handle->aps_reduce_water_mark,
		handle->audiopll_mode);

	switch (format) {
		case SBC_TYPE:
		{
            if(audio_policy_get_a2dp_lantency_time())
            {
                handle->aps_increase_water_mark = UINT32_MAX;
                handle->aps_reduce_water_mark = 0;
            }
            else if (audiolcy_is_low_latency_mode()) {
				handle->aps_min_level = APS_LEVEL_3;
				handle->aps_max_level = APS_LEVEL_6;
				handle->duration = 6;
			} 
#ifdef CONFIG_TWS
			if (tws_observer) {
				audio_aps_monitor_tws_init(tws_observer);
				audio_track_set_waitto_start(audio_track, true);
			}
#endif
            
			break;
		}
		case AAC_TYPE:
		{
            if(audio_policy_get_a2dp_lantency_time())
            {
                handle->aps_increase_water_mark = UINT32_MAX;
                handle->aps_reduce_water_mark = 0;
            }
            else if (audiolcy_is_low_latency_mode()) {
				handle->aps_min_level = APS_LEVEL_3;
				handle->aps_max_level = APS_LEVEL_6;
				handle->duration = 6;
			}
#ifdef CONFIG_TWS
			if (tws_observer) {
				audio_aps_monitor_tws_init(tws_observer);
				audio_track_set_waitto_start(audio_track, true);
			}
#endif
			break;
		}
		case MSBC_TYPE:
		case CVSD_TYPE:
		{
			if (audiolcy_is_low_latency_mode()) {
				handle->aps_min_level = APS_LEVEL_3;
				handle->aps_max_level = APS_LEVEL_6;
				handle->duration = 6;
			}
#ifdef CONFIG_TWS
			if (tws_observer) {
				audio_aps_monitor_tws_init(tws_observer);
				audio_track_set_waitto_start(audio_track, true);
			}
#endif
			break;
		}
		case ACT_TYPE:
		{
#ifdef CONFIG_SNOOP_LINK_TWS
            if (tws_observer) {
				audio_aps_monitor_tws_init(tws_observer);
				audio_track_set_waitto_start(audio_track, true);
			}
#endif
			break;
		}
		case PCM_TYPE:
		{
			handle->aps_min_level = APS_LEVEL_3;
			handle->aps_max_level = APS_LEVEL_6;
			if (audiolcy_is_low_latency_mode()) {
				handle->aps_default_level = APS_LEVEL_4;
			}
#ifdef CONFIG_TWS
			if (tws_observer) {
				audio_aps_monitor_tws_init(tws_observer);
				audio_track_set_waitto_start(audio_track, true);
			}
#endif
			break;
		}
		case NAV_TYPE:
		{
			if (audiopll_mode == 2) {
				handle->aps_min_level = 6;
				handle->aps_max_level = 26;
				handle->aps_default_level = 16;
				handle->aps_water_adjust_thres = 18;
			} else {
				handle->aps_min_level = APS_LEVEL_2;
				handle->aps_max_level = APS_LEVEL_6;
				if (audiolcy_is_low_latency_mode()) {
					handle->aps_default_level = APS_LEVEL_3;
				}
				handle->aps_water_adjust_thres = 18;
			}
			handle->duration = 6;
			audio_track_set_waitto_start(audio_track, true);
#ifdef CONFIG_AUDIO_APS_ADJUST_FINE
			handle->need_aps_fine = 1;
			handle->aps_water_adjust_thres = 18;
			handle->aps_water_adjust_last = handle->aps_increase_water_mark;
#endif
			handle->lea_restarting = 0;
			break;
		}
		case PCM_DSP_TYPE:
		{
			if (audiopll_mode == 2) {
				handle->aps_min_level = 6;
				handle->aps_max_level = 26;
				handle->aps_default_level = 16;
				handle->aps_water_adjust_thres = 200;
			} else {
				handle->aps_min_level = APS_LEVEL_2;
				handle->aps_max_level = APS_LEVEL_6;
				if (audiolcy_is_low_latency_mode()) {
					handle->aps_default_level = APS_LEVEL_3;
				}
				handle->aps_water_adjust_thres = 200;
			}
			handle->duration = 6;
			audio_track_set_waitto_start(audio_track, true);
#ifdef CONFIG_AUDIO_APS_ADJUST_FINE
			handle->need_aps_fine = 1;
			handle->aps_water_adjust_last = handle->aps_increase_water_mark;
#endif
			break;
		}
		default:
		{
			handle->need_aps = 0;
#ifdef CONFIG_TWS
			if (tws_observer) {
				audio_aps_monitor_tws_init(tws_observer);
				audio_track_set_waitto_start(audio_track, true);
			}
#endif
			break;
		}
	}

	handle->current_level = handle->aps_default_level;
	handle->last_level = handle->current_level;
	handle->dest_level = handle->current_level;

}

void audio_aps_notify_decode_err(uint16_t err_cnt)
{
#ifdef CONFIG_TWS
	audio_aps_tws_notify_decode_err(err_cnt);
#endif
}

void audio_lea_notify_decode_err(uint16_t err_cnt)
{
	aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
	if (handle->lea_restarting)
		return;

	handle->lea_restarting = 1;
	printk("[LEA]player restart\n");

	bt_manager_event_notify(BT_REQ_RESTART_PLAY, NULL, 0);
}

void audio_aps_monitor_deinit(int format, void *tws_observer, struct audio_track_t *audio_track)
{
#ifdef CONFIG_TWS
	/* Be consistent with audio_aps_monitor_init */
	switch (format) {
		case ACT_TYPE:
		case MSBC_TYPE:
		case CVSD_TYPE:
		case SBC_TYPE:
		case AAC_TYPE:
		default:
			if (tws_observer) {
				audio_aps_monitor_tws_deinit(tws_observer);
			}
			break;
	}
#endif
}

