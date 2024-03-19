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
#include <bt_manager.h>
#include <audio_hal.h>
#include <audio_system.h>
#include <audio_track.h>
#include <audiolcy_common.h>
#include <media_type.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stream.h>
#ifdef CONFIG_TWS
#include <btservice_api.h>
#include "bluetooth_tws_observer.h"
#endif

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "audio_aps"

#define APS_ADJUST_PKT_INTERVAL    (300)

typedef struct {
    uint16_t  slow_step_time;
    uint16_t  fast_step_time;
} player_tws_step_t;

typedef struct {
    media_observer_t audio_observer;
    
    uint32_t adjust_time;/*最后一次调aps的系统时间*/
    uint32_t last_check_time;/*上一次检查aps的系统时间*/
    uint32_t debug_time;/*3秒钟打印一次aps信息*/
    int32_t max_latency;/*15个包中的最大水位值*/
	uint32_t start_play_cyc;/*单箱起始播放的cycle*/
    
    uint16_t first_pkt_seq;
    uint16_t last_level:8;
    uint16_t restarting:1;
    uint16_t sample_aligning:1;
    uint16_t simple_tws:1;
    uint16_t tws_start_decode:1;
    uint16_t tws_start_pcm_streaming:1;
    uint16_t reserved:3;
    
    int8_t check_count;
    
    os_timer align_timer;
    bt_tws_observer_t *bt_observer;
} audio_aps_snoop_t;

//一个采样点需要多少毫秒调节过来 * 采样率khz
//1000 / (两个level之间的采样率差) * 采样率
static const player_tws_step_t g_tws_step_44[16] =
{
    {841, 841},     /* APS_44KHZ_LEVEL_1 */
    {841, 1606},     /* APS_44KHZ_LEVEL_2 */
    {1606, 545},     /* APS_44KHZ_LEVEL_3 */
    {545, 2156},     /* APS_44KHZ_LEVEL_4 */
    {2156, 1913},     /* APS_44KHZ_LEVEL_5 */
    {1913, 2588},     /* APS_44KHZ_LEVEL_6 */
    {2588, 1134},     /* APS_44KHZ_LEVEL_7 */
    {1134, 1134},     /* APS_44KHZ_LEVEL_8 */
};
static const player_tws_step_t g_tws_step_48[16] =
{
    {819, 819},     /* APS_48KHZ_LEVEL_1 */
    {819, 2539},     /* APS_48KHZ_LEVEL_2 */
    {2539, 2448},     /* APS_48KHZ_LEVEL_3 */
    {2448, 2364},     /* APS_48KHZ_LEVEL_4 */
    {2364, 2285},     /* APS_48KHZ_LEVEL_5 */
    {2285, 3310},     /* APS_48KHZ_LEVEL_6 */
    {3310, 778},     /* APS_48KHZ_LEVEL_7 */
    {778, 778},     /* APS_48KHZ_LEVEL_8 */
};

extern void audio_aps_monitor_set_aps(void *audio_handle, uint8_t status, int level);
extern uint32_t get_sample_rate_hz(uint8_t fs_khz);

static void _tws_start_aligning(aps_monitor_info_t *handle, uint8_t level, int32_t adjust_time)
{
    audio_aps_snoop_t *aps_snoop = (audio_aps_snoop_t*)handle->tws_observer;
    
    handle->current_level = level;
    aps_snoop->sample_aligning = 1;
    hal_aout_channel_set_aps(handle->audio_track->audio_handle, handle->current_level, APS_LEVEL_AUDIOPLL);
    os_timer_start(&aps_snoop->align_timer, K_MSEC(adjust_time), K_MSEC(0));
}

static void _tws_stop_aligning(os_timer *ttimer)
{
    aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    audio_aps_snoop_t *aps_snoop = (audio_aps_snoop_t*)handle->tws_observer;

    if(!aps_snoop->sample_aligning) {
        return;
    }
    
    handle->current_level = handle->dest_level;
    aps_snoop->sample_aligning = 0;
    hal_aout_channel_set_aps(handle->audio_track->audio_handle, handle->current_level, APS_LEVEL_AUDIOPLL);
    os_timer_stop(&aps_snoop->align_timer);
    printk("stop align, level: %u, %u\n", handle->current_level, os_uptime_get_32());
}

static uint32_t _get_sdm_cnt_saved(void *aps_monitor)
{
    aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    uint32_t sdm_cnt_saved;

    if (!handle || !handle->audio_track) {
		return 0;
	}

    sdm_cnt_saved = hal_aout_channel_get_saved_sdm_cnt(handle->audio_track->audio_handle) & SDM_CNT_MASK;
    return sdm_cnt_saved;
}

static uint32_t _get_sdm_cnt_realtime(void *aps_monitor)
{
    aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    uint32_t sdm_cnt;

    if (!handle || !handle->audio_track) {
		return 0;
	}

    sdm_cnt = hal_aout_channel_get_sdm_cnt(handle->audio_track->audio_handle) & SDM_CNT_MASK;
    return sdm_cnt;
}

static int32_t _get_sdm_cnt_delta_time(void *aps_monitor, int32_t sdm_cnt)
{
    aps_monitor_info_t *handle = audio_aps_monitor_get_instance();

    if (!handle || !handle->audio_track) {
		return 0;
	}

    while(sdm_cnt > (int32_t)(SDM_CNT_MASK/2)) sdm_cnt -= SDM_CNT_MASK + 1;
    while(sdm_cnt < -(int32_t)(SDM_CNT_MASK/2)) sdm_cnt += SDM_CNT_MASK + 1;

    return sdm_cnt * 1000000ll / handle->audio_track->sdm_sample_rate;
}

static int32_t _set_dac_trigger_src(void *aps_monitor, audio_trigger_src src)
{
    aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    dac_ext_trigger_ctl_t trigger_ctl;

    if (!handle || !handle->audio_track) {
		return 0;
	}

    memset(&trigger_ctl, 0, sizeof(trigger_ctl));
#ifdef CONFIG_SOC_SERIES_LARK
#ifndef CONFIG_SOFT_SDM_CNT
    trigger_ctl.t.sdm_cnt_lock_en = 1;
#endif

    if(!handle->audio_track->dsp_fifo_src) {
        trigger_ctl.t.dac_fifo_trigger_en = 1;
    }
#endif //CONFIG_SOC_SERIES_LARK

#if defined(CONFIG_SOC_SERIES_CUCKOO) && !defined(CONFIG_SOFT_SDM_CNT)
    trigger_ctl.t.sdm_cnt_trigger_en = 1;
	trigger_ctl.t.sdm_cnt_lock_en = 1;
	trigger_ctl.t.dac_digital_trigger_en = 1;
#endif //CONFIG_SOC_SERIES_CUCKOO

    if(trigger_ctl.trigger_ctl == 0) {
        return 0;
    }

    hal_aout_trigger_src_control(handle->audio_track->audio_handle, &trigger_ctl);
    return hal_aout_channel_set_dac_trigger_src(handle->audio_track->audio_handle, src);
}

static int32_t _set_dac_enable(void *aps_monitor)
{
    aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    dac_ext_trigger_ctl_t trigger_ctl;

    if (!handle || !handle->audio_track) {
		return 0;
	}
    
    memset(&trigger_ctl, 0, sizeof(trigger_ctl));
#if defined(CONFIG_SOC_SERIES_CUCKOO) && !defined(CONFIG_SOFT_SDM_CNT)
    trigger_ctl.t.sdm_cnt_trigger_en = 1;
	trigger_ctl.t.dac_digital_trigger_en = 1;
#endif //CONFIG_SOC_SERIES_CUCKOO
    return hal_aout_channel_set_dac_enable(handle->audio_track->audio_handle, &trigger_ctl);
}

static int32_t _set_start_pkt_seq(void *aps_monitor, uint8_t start_decode, uint16_t first_seq)
{
	aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    audio_aps_snoop_t *aps_snoop = (audio_aps_snoop_t*)handle->tws_observer;

	if (!handle || !handle->audio_track) {
		return 0;
	}
    
	aps_snoop->first_pkt_seq = first_seq;
    aps_snoop->tws_start_decode = start_decode ? 1 : 0;
    return 0;
}

static int32_t _start_pcm_streaming(void *aps_monitor, uint16_t playtime_us)
{
	aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    audio_aps_snoop_t *aps_snoop = (audio_aps_snoop_t*)handle->tws_observer;

	if (!handle || !handle->audio_track) {
		return 0;
	}

    audio_track_start(handle->audio_track);
	aps_snoop->tws_start_pcm_streaming = 1;
	aps_snoop->start_play_cyc = k_cycle_get_32() + k_us_to_cyc_floor32(playtime_us);
    return 0;
}

static int32_t _start_playback(void *aps_monitor)
{
	aps_monitor_info_t *handle = audio_aps_monitor_get_instance();

	if (!handle || !handle->audio_track) {
		return 0;
	}
    
    return 0;
}

static int32_t _set_base_aps_level(void *aps_monitor, uint8_t level)
{
	aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    uint32_t flags;

	if (!handle || !handle->audio_track || (handle->dest_level == level)) {
		return 0;
	}

    flags = irq_lock();

    if(level == 0xFF) {
        level = handle->aps_default_level;
    }
    
    handle->current_level = level;
    handle->dest_level = level;
    _tws_stop_aligning(NULL);
    hal_aout_channel_set_aps(handle->audio_track->audio_handle, handle->current_level, APS_LEVEL_AUDIOPLL);
    
    irq_unlock(flags);
    
	//SYS_LOG_INF("%d", level);
    return 0;
}

static int32_t _notify_time_diff(void *aps_monitor, int32_t diff_time)
{
	aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    audio_aps_snoop_t *aps_snoop;
    const player_tws_step_t *tws_step;
    int32_t adjust_time;
    uint32_t flags;
    uint8_t new_level;
    uint16_t sample_rate;

	if (!handle || !handle->audio_track) {
		return 0;
	}

    aps_snoop = (audio_aps_snoop_t*)handle->tws_observer;
    sample_rate = handle->audio_track->output_sample_rate_hz / 1000;

    if(abs(diff_time) > 3000)
    {
        SYS_LOG_ERR("diff_time %d us", diff_time);
        aps_snoop->restarting = 1;
        bt_tws_trigger_restart(aps_snoop->bt_observer, 0);
        return 0;
    }
    
    //SYS_LOG_INF("diff_time %d us", diff_time);
    if((sample_rate % 11) == 0) {
        tws_step = g_tws_step_44;
    } else {
        tws_step = g_tws_step_48;
    }

    flags = irq_lock();

    /* 停止调节
     */
    _tws_stop_aligning(NULL);
    if(abs(diff_time) < 3) {
        irq_unlock(flags);
        return 0;
    }

    /* 确定adjust time
     */
    if(diff_time < 0) {
        new_level = handle->dest_level + 1;
        adjust_time = tws_step[handle->dest_level].fast_step_time * abs(diff_time) / 1000;
    } else {
        new_level = handle->dest_level - 1;
        adjust_time = tws_step[handle->dest_level].slow_step_time * abs(diff_time) / 1000;
    }

    //if(adjust_time > 100)
    //    adjust_time = 100;

    _tws_start_aligning(handle, new_level, adjust_time);

    irq_unlock(flags);

    printk("diff_time: %d us, adjust time: %d, level new: %d, cur: %d, base: %d\n",
        diff_time, adjust_time,
        new_level, handle->current_level, handle->dest_level);
    
    return 0;
}

void audio_aps_tws_notify_decode_err(uint16_t err_cnt)
{
    aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    audio_aps_snoop_t *aps_snoop = (audio_aps_snoop_t*)handle->tws_observer;

    if(NULL == handle->tws_observer) {
        return;
    }

    aps_snoop->restarting = 1;
	bt_tws_trigger_restart(aps_snoop->bt_observer, err_cnt);
}

static uint8_t _monitor_aout_rate(aps_monitor_info_t *handle, int32_t pcm_time, int32_t is_tws)
{
    audio_aps_snoop_t *aps_snoop = (audio_aps_snoop_t*)handle->tws_observer;
    int32_t max_threshold = (int32_t)handle->aps_increase_water_mark;
    int32_t trim_threshold = 1000;//max_threshold / 5;  /*微调阈值*/
    int8_t trim_flag = 0;
    uint8_t min_level = APS_LEVEL_1;
    uint8_t max_level = APS_LEVEL_8;
    uint8_t cur_level = handle->current_level;
    uint8_t adj_level = cur_level;
    int8_t max_check_count;
    uint32_t adjust_interval = 600;
    uint32_t cur_time = os_uptime_get_32();
    uint16_t sample_rate;
    int32_t time_diff_abs;

    sample_rate = handle->audio_track->output_sample_rate_hz / 1000;
    if((sample_rate == 8) || (sample_rate == 16)) {
        max_check_count = 2;
    } else {
        max_check_count = 2;
    }

    if(is_tws) {
        /* TWS场景需要预留最大和最小LEVEL做缓冲区调节
         */
        min_level++;
        max_level--;
    }

    time_diff_abs = abs((int32_t)(pcm_time - max_threshold));

    /* 差值小于trim_threshold，则在默认level上下一级微调
     */
    if(time_diff_abs <= trim_threshold) {
        if((cur_level >= (handle->aps_default_level - 1))
            && (cur_level <= (handle->aps_default_level + 1))) {
            min_level = handle->aps_default_level - 1;
            max_level = handle->aps_default_level + 1;

            trim_flag = 1;
        }
    }

    if((pcm_time > max_threshold) && (aps_snoop->check_count < max_check_count)) {
        aps_snoop->check_count++;
    } else if((pcm_time < max_threshold) && (aps_snoop->check_count > -max_check_count)) {
        aps_snoop->check_count--;
    }

    do
    {
        /* 设置最小等待时间避免TWS场景调节失败
         */
        if((cur_time - aps_snoop->adjust_time) < 220) {
            adj_level = cur_level;
            break;
        }
        
        if(aps_snoop->check_count >= max_check_count) {
            adj_level++;
            break;
        }
        if(aps_snoop->check_count <= -max_check_count) {
            if (adj_level > 0) {
                adj_level--;
            }
            break;
        }
        
        if(pcm_time > max_threshold) {
            adj_level++;
        } else if(pcm_time < max_threshold) {
            if (adj_level > 0) {
                adj_level--;
            }
        }

        if((cur_time - aps_snoop->adjust_time) < adjust_interval) {
            if(trim_flag == 1) {
                adj_level = cur_level;
                break;
            }

            /* 快速调节
             */
            if(((cur_level == max_level) && (adj_level < max_level)) ||
                ((cur_level == min_level) && (adj_level > min_level))) {
                //printk("aout aps6:%d_%d_%d\n", cur_level, adj_level, pcm_time);
            } else {
                adj_level = cur_level;
            }
        }
    }while(0);
    
    if(adj_level > max_level) {
        adj_level = max_level;
    }
    if(adj_level < min_level) {
        adj_level = min_level;
    }

    if(adj_level != cur_level) {
        /* 上升趋势调节
         */
        if((cur_level < handle->aps_default_level) && (adj_level > cur_level)) {
            adj_level = handle->aps_default_level;
        }

        /* 下降趋势调节
         */
        if((cur_level > handle->aps_default_level) && (adj_level < cur_level)) {
            adj_level = handle->aps_default_level;
        }
    } else {
        adj_level = 0xFF;
    }

    if(((adj_level != 0xFF) && (adj_level != aps_snoop->last_level))
        || ((cur_time - aps_snoop->debug_time) >= 3000)){
        if(adj_level != 0xFF) {
            aps_snoop->last_level = adj_level;
        }
        aps_snoop->debug_time = cur_time;
        printk("aps level: %u(%u-%u), time: %d, check count; %d/%d, interval: %u\n",
            aps_snoop->last_level, min_level, max_level,
            pcm_time,
            aps_snoop->check_count, max_check_count,
            cur_time - aps_snoop->adjust_time);
    }
    if(adj_level != 0xFF) {
        aps_snoop->adjust_time = cur_time;
        aps_snoop->check_count = 0;
    }
    
    return adj_level;
}

void audio_aps_monitor_master(aps_monitor_info_t *handle, int32_t pcm_time, uint8_t aps_max_level, uint8_t aps_min_level, uint8_t aps_level)
{
    audio_aps_snoop_t *aps_snoop = (audio_aps_snoop_t*)handle->tws_observer;
    uint8_t ret = 0xFF;
    int32_t role;
    uint32_t cur_time = os_uptime_get_32();

    if(NULL == handle->tws_observer) {
        return;
    }

    bt_tws_on_buffer_change(aps_snoop->bt_observer, pcm_time);
    if(handle->audio_track->stared == 0) {
        return;
    }

    do {
        if(aps_snoop->max_latency < pcm_time) {
            aps_snoop->max_latency = pcm_time;
        }

        if((cur_time - aps_snoop->last_check_time) >= APS_ADJUST_PKT_INTERVAL) {
            //printk("max_latency: %d\n", aps_snoop->max_latency);
            role = bt_tws_get_role(aps_snoop->bt_observer);
            if(role == BTSRV_TWS_NONE) {
                ret = _monitor_aout_rate(handle, aps_snoop->max_latency, 0);
            } else if(role == BTSRV_TWS_MASTER) {
                ret = _monitor_aout_rate(handle, aps_snoop->max_latency, 1);
            } else {
                uint32_t cur_time = os_uptime_get_32();
                if((cur_time - aps_snoop->adjust_time) >= 3000) {
                    aps_snoop->adjust_time = cur_time;
                    printk("aps level: %d, time: %d\n", handle->current_level, pcm_time);
                }
            }

            if(ret != 0xFF) {
                bt_tws_aps_change_request(aps_snoop->bt_observer, ret);
            }

            aps_snoop->last_check_time = cur_time;
            aps_snoop->max_latency = 0;
        }
    } while (0);

    if(!aps_snoop->restarting && !aps_snoop->simple_tws) {
        if(audio_track_get_fill_samples(handle->audio_track)) {
            printk("dac empty\n");
            aps_snoop->restarting = 1;
            bt_tws_trigger_restart(aps_snoop->bt_observer, 0);
        }
    }
}

void audio_aps_monitor_slave(aps_monitor_info_t *handle, int pcm_time, uint8_t aps_max_level, uint8_t aps_min_level, uint8_t slave_aps_level)
{
}

int32_t audio_tws_set_stream_info(
    uint8_t format, uint16_t first_seq, uint8_t sample_rate, uint32_t pkt_time_us, uint64_t play_time)
{
    aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    audio_aps_snoop_t *aps_snoop = (audio_aps_snoop_t*)handle->tws_observer;

    if(NULL == handle->tws_observer) {
        return 0;
    }

    aps_snoop->tws_start_decode = 0;
    aps_snoop->first_pkt_seq = first_seq;
    aps_snoop->adjust_time = os_uptime_get_32();
    aps_snoop->last_check_time = aps_snoop->adjust_time;
    aps_snoop->debug_time = aps_snoop->adjust_time;
    aps_snoop->check_count = 0;
    aps_snoop->last_level = 0xFF;
    return bt_tws_set_stream_info(aps_snoop->bt_observer,
        format, handle->audio_track->channels,
        first_seq, get_sample_rate_hz(sample_rate),
        handle->aps_increase_water_mark, pkt_time_us, play_time);
}

int32_t audio_tws_set_sync_policy(uint32_t type, uint8_t enable)
{
    aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    audio_aps_snoop_t *aps_snoop = (audio_aps_snoop_t*)handle->tws_observer;

    if(NULL == handle->tws_observer) {
        return 0;
    }

    return bt_tws_set_sync_policy(aps_snoop->bt_observer, type, enable);
}

int audio_tws_get_playback_first_seq(uint8_t *start_decode, uint8_t *start_play, uint16_t *first_seq, uint16_t *playtime_us)
{
    aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    audio_aps_snoop_t *aps_snoop = (audio_aps_snoop_t*)handle->tws_observer;

    if(NULL == handle->tws_observer) {
        return -1;
    }

    if(start_decode) {
        *start_decode = aps_snoop->tws_start_decode;
    }
    if(start_play) {
        *start_play = aps_snoop->tws_start_pcm_streaming;
    }
    if(first_seq) {
        *first_seq = aps_snoop->first_pkt_seq;
    }
	if(playtime_us) {
		uint32_t play_time = k_cyc_to_us_floor32(aps_snoop->start_play_cyc - k_cycle_get_32());

		if(play_time > 50000) {
			play_time = 0;
		}
		
		*playtime_us = play_time;
	}
    return 0;
}

int32_t audio_tws_set_pkt_info(uint16_t pkt_seq, uint16_t pkt_len, uint16_t frame_cnt, uint16_t pcm_len)
{
    aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    audio_aps_snoop_t *aps_snoop = (audio_aps_snoop_t*)handle->tws_observer;
    tws_pkt_info_t info;
#ifndef CONFIG_SYNC_BY_SDM_CNT
    uint32_t flags;
    uint32_t latency = 0;
#endif

    if(NULL == handle->tws_observer) {
        return 0;
    }
    
    info.pkt_seq = pkt_seq;
    info.pkt_len = pkt_len;
    info.frame_cnt = frame_cnt;
    info.samples = pcm_len;

    if(pcm_len != 0xFFFFU) {
        //if(handle->audio_track->stared == 0) {
        //    return 0;
        //}

#ifdef CONFIG_SYNC_BY_SDM_CNT
        info.sdm_cnt = handle->audio_track->samples_filled;
        handle->audio_track->samples_filled += info.samples * handle->audio_track->sdm_osr;
#else
        flags = irq_lock();
        info.pkt_bttime_us = bt_tws_get_bt_clk_us(aps_snoop->bt_observer);
        if(UINT64_MAX != info.pkt_bttime_us) {
            latency = audio_track_get_latency(handle->audio_track);
            info.pkt_bttime_us += latency;
        }
        irq_unlock(flags);
#endif

        //printk("seq: %u, time: %u, latency: %u\n", info.pkt_seq, (uint32_t)info.pkt_bttime_us, latency);
    } else {
        // received a new pkt, make a copy for audiolcy
        audiolcy_pktinfo_t lcypkt;

        lcypkt.seq_no = pkt_seq;
        lcypkt.pkt_len = pkt_len;
        lcypkt.frame_cnt = frame_cnt;
        lcypkt.sysclk_cyc = k_cycle_get_32();
        audiolcy_cmd_entry(LCYCMD_SET_PKTINFO, &lcypkt, sizeof(audiolcy_pktinfo_t));
    }

    return bt_tws_set_pkt_info(aps_snoop->bt_observer, &info);
}

uint64_t audio_tws_get_play_time_us(void)
{
    aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    audio_aps_snoop_t *aps_snoop;

    if(!handle || !handle->tws_observer) {
        return UINT64_MAX;
    }
    
    aps_snoop = (audio_aps_snoop_t*)handle->tws_observer;
    return bt_tws_get_play_time_us(aps_snoop->bt_observer);
}

uint64_t audio_tws_get_bt_clk_us(void *tws_observer)
{
    if(tws_observer) {
        tws_runtime_observer_t *observer = (tws_runtime_observer_t *)tws_observer;
        return observer->get_bt_clk_us();
    } else {
        aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
        audio_aps_snoop_t *aps_snoop;

        if(!handle || !handle->tws_observer) {
            return UINT64_MAX;
        }
        
        aps_snoop = (audio_aps_snoop_t*)handle->tws_observer;
        return bt_tws_get_bt_clk_us(aps_snoop->bt_observer);
    }
}

static audio_aps_snoop_t audio_aps_snoop;

void audio_aps_monitor_tws_init(void *tws_observer)
{
	aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    audio_aps_snoop_t *aps_snoop = &audio_aps_snoop;
    uint8_t stream_type = AUDIO_STREAM_DEFAULT;

    assert(tws_observer);

    memset(aps_snoop, 0, sizeof(audio_aps_snoop_t));
    aps_snoop->audio_observer.set_start_pkt_seq   = _set_start_pkt_seq;
	aps_snoop->audio_observer.start_pcm_streaming = _start_pcm_streaming;
	aps_snoop->audio_observer.start_playback   = _start_playback;
	aps_snoop->audio_observer.set_base_aps_level   = _set_base_aps_level;
	aps_snoop->audio_observer.notify_time_diff   = _notify_time_diff;
	aps_snoop->audio_observer.get_sdm_cnt_saved = _get_sdm_cnt_saved;
    aps_snoop->audio_observer.get_sdm_cnt_realtime = _get_sdm_cnt_realtime;
	aps_snoop->audio_observer.get_sdm_cnt_delta_time = _get_sdm_cnt_delta_time;
	aps_snoop->audio_observer.set_dac_trigger_src = _set_dac_trigger_src;
	aps_snoop->audio_observer.set_dac_en = _set_dac_enable;
	
    if(!handle){
        return;
    }

    if(handle->audio_track) {
        stream_type = handle->audio_track->stream_type;
    }

    if((stream_type == AUDIO_STREAM_TTS) || (stream_type == AUDIO_STREAM_TIP)) {
        aps_snoop->bt_observer = simple_tws_observer_init(&aps_snoop->audio_observer, tws_observer);
        aps_snoop->simple_tws = 1;
    } else {
    	aps_snoop->bt_observer = bt_tws_observer_init(&aps_snoop->audio_observer, tws_observer);
    }
    
    handle->tws_observer = aps_snoop;
	handle->role = BTSRV_TWS_MASTER;
    handle->duration = 1;

    hal_aout_channel_enable_sample_cnt(handle->audio_track->audio_handle, true);
    hal_aout_channel_reset_sample_cnt(handle->audio_track->audio_handle);

    os_timer_init(&aps_snoop->align_timer, _tws_stop_aligning, NULL);
	audio_aps_monitor_set_aps(handle->audio_track->audio_handle, APS_OPR_FAST_SET, handle->aps_default_level);
}

void audio_aps_monitor_tws_deinit(void *tws_observer)
{
	aps_monitor_info_t *handle = audio_aps_monitor_get_instance();

	if (handle->tws_observer) {
        audio_aps_snoop_t *aps_snoop = (audio_aps_snoop_t*)handle->tws_observer;

        bt_tws_observer_deinit(aps_snoop->bt_observer);
		handle->tws_observer = NULL;

        os_timer_stop(&aps_snoop->align_timer);
	}
}

void audio_tws_pre_init(void)
{
    tws_runtime_observer_t *tws_observer;

    tws_observer = bt_manager_tws_get_runtime_observer();
    if(tws_observer) {
        bt_tws_observer_pre_init(tws_observer);
    }
}

