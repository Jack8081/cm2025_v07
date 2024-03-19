/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file adaptive latency interface
 */

#include "audiolcy_inner.h"


#define LCYAPT_PKTINFO_NUM      (24)
#define LCYAPT_ADJUST_DIFF_MS   (3)  // unit ms, new latency is disavowed when they differ too little


#define REGION_NUM              (9)   // divided region num, do not change if it is not necessary
#define TRANS_OPT_THRES_UP      (15)  // increase the minimum threshold if enable transfer optimization fail
#define TRANS_OPT_THRES_DOWN    (14)  // decrease the minimum threshold if phone send litter mtu packet

#define DOWN_TIMER_NUM          (3)
#define DOWN_TIMER_SPAN         (5)   // default 5
#define DOWN_TIMER_LAST_DIFF    (5)   // the last timer can be renewed by a lower latency within this value

#define STACCATO_DIS_FACTOR     (1)   // 0 ~ 5, smaller than cfg_factor


typedef struct
{
    u8_t  format;    // AAC or SBC, to avoid that player restarted by changing codec
    u8_t  is_valid;  // invalid: codec changed, dual phone switch, ...
    u8_t  latency;   // last recommended adaptive latency in ms

} audiolcyapt_global_t;

typedef enum {
    UPDATE_STA_NONE = 0,
    UPDATE_STA_NEED,        // need update apt lcy
    UPDATE_STA_PENDING,     // apt lcy is updating

} lcyapt_update_status_e;

typedef enum {
    LCYAPT_STA_NONE = 0,
    LCYAPT_STA_INIT,        // after init
    LCYAPT_STA_PREPLAY,     // playback started and lcyapt has not synced lcy
    LCYAPT_STA_LCYSYNC,     // after lcyapt sync lcys
    LCYAPT_STA_PLAYING,     // normal

} lcyapt_status_e;

typedef struct {
    u32_t time_span[LCYAPT_PKTINFO_NUM];  // time_span in us from last pkt received
    u8_t  frame_cnt[LCYAPT_PKTINFO_NUM];  // frames in a pkt

    u32_t last_pkt_time;    // time_stamp that pkt is received
    u8_t  last_frame_cnt;

    u8_t  windex;   // next write position
    u8_t  rindex;   // next read position
    u8_t  count;    // count of updated info

} lcyapt_source_t;

typedef struct {
    u32_t pkt_time_ave;     // average pkt time in a windows
    s32_t pkt_time_diff;    // difference between received_time cost and pkt_time in a windows

    u16_t result_factor;

    u8_t  cbig_thres;   // percentage of pkt_time, threshold of big ts, <= 200
    u8_t  cbig_count;   // count of continuous big ts
    u32_t cbig_sum;     // sum of continuous big ts
    u32_t cbig_max;     // the maximum of cbig_sum

    u32_t ts_region_total;
    u32_t ts_region_max;          // the biggest ts of all
    u16_t ts_region[REGION_NUM];  // 9 region in percentage: <50, <75, <90, 90~110, >110, >125, >150, >200, >300

    u8_t  factor_stat_flag;   //0-not init, 1-inited, 2-used
    u32_t factor_stat_ts;
    u16_t factor_last;
    u16_t factor_count;
    u32_t factor_delta_square_add;
    u16_t factor_delta_square_averages[8];
    u16_t factor_delta_square_weighting;

} lcyapt_result_t;

typedef struct {
    audiolcy_common_t *lcycommon;   // shared info of audiolcy

    lcyapt_source_t source_info;    // source info for analysis
    lcyapt_result_t result_info;    // result info after analysis

    u8_t  cfg_minimum;      // the minimum recommended adaptive latency in current codec
    u8_t  cfg_maximum;      // the maximum recommended adaptive latency could be raised to
    u8_t  cfg_factor;       // the bigger it is, the higher the priority of smoothly playing is
    u8_t  mtu_check;        // sbc codec, lower the cfg_minimum if pkt-size(mtu) is too small

    u8_t  pending_lcy;      // pre-saved lcy, pending for down-adjust timer
    u8_t  current_lcy;      // current using adaptive latency

    u8_t  need_calc;        // 1: source_info updated and need to recommend a new latency
    u8_t  update_status;    // 1: need update, 2: updating

    u32_t recommend_timer;  // unit second, time of last recommend latency
    u8_t  quick_calc;       // 0: normal speed, 1: quick speed, 2: pre-disable quick calc
    u8_t  quick_max;        // the biggest latency during quick speed calc
    u8_t  quick_count;      // count of recommended latency during quick speech calc

    u8_t  first_circle;     // the first circle after playing, might do sth special
    u8_t  status;           // lcyapt_status_e

    u8_t  staccato;         // 1: staccato occured, cooperated with Music_PLC
    u8_t  scc_imcrement;    // the increment latency by staccato
    u8_t  scc_count;        // count of staccato in a certain time
    u32_t scc_lcy_min;      // the minimum buffer latency after staccato
    u32_t scc_timer;        // unit ms

    u8_t  down_timer_num;
    u8_t  down_timer_lcy[DOWN_TIMER_NUM];    // unit ms, the latency to be adjusted to
    u32_t down_timer[DOWN_TIMER_NUM];        // unit second

} audiolcy_adaptive_t;


static audiolcyapt_global_t g_adaptive_latency = { 0, };

static u32_t audiolcyapt_down_timer_check(audiolcy_adaptive_t *lcyapt, u16_t cur_lcy_ms, u16_t new_lcy_ms);


static u8_t audiolcyapt_global_check_valid(u8_t format)
{
    audiolcyapt_global_t *g_lcyapt = &g_adaptive_latency;

    if (g_lcyapt->latency <= 0)
        return 0;

    if (g_lcyapt->is_valid && g_lcyapt->format == format)
        return 1;

    return 0;
}

void audiolcyapt_global_set_invalid(void)
{
    audiolcyapt_global_t *g_lcyapt = &g_adaptive_latency;

    /* last adaptive latency is invalid in those scene:
     * dual phone switch, codec change, ...
     */

    g_lcyapt->is_valid = 0;
    printk("lcyapt global invalid\n");
}

static u8_t audiolcyapt_global_set_latency(u8_t format, u8_t ms)
{
    audiolcyapt_global_t *g_lcyapt = &g_adaptive_latency;

    g_lcyapt->format = format;
    g_lcyapt->is_valid = 1;

    if (g_lcyapt->latency == ms) {
        return 0;
    }

    g_lcyapt->latency = ms;
    printk("lcyapt global set %d\n", g_lcyapt->latency);

    return g_lcyapt->latency;
}

static u32_t audiolcyapt_region_locate(lcyapt_result_t *p_out, u32_t ts)
{
    u8_t region_i;
    u32_t Thres10, Thres25, Thres50, abs_delta_ts;
    s32_t delta_ts = 0;

    Thres10 = p_out->pkt_time_ave / 10;  /* 10% */
    Thres25 = p_out->pkt_time_ave / 4;   /* 25% */
    Thres50 = p_out->pkt_time_ave / 2;   /* 50% */

    delta_ts = ts - p_out->pkt_time_ave;
    abs_delta_ts = abs(delta_ts);

    if (abs_delta_ts <= Thres10) {
        region_i = 3;   /* [90% ~ 110%] */
    } else {
        if (abs_delta_ts > (p_out->pkt_time_ave * 2)) {
            region_i = 8;   /* (300%, oo) */
        } else if (abs_delta_ts > p_out->pkt_time_ave) {
            region_i = 7;   /* (200%, 300%] */
        } else {
            if (abs_delta_ts > Thres50) {
                region_i = 3;   /* [0, 50%) || (150%, 200%] */
            } else if (abs_delta_ts > Thres25) {
                region_i = 2;   /* [50%, 75%) || (125%, 150%] */
            } else { 
                region_i = 1;   /* [75%, 90%) || (110%, 125%] */
            }

            if (delta_ts >= 0) {
                region_i += 3;
            } else {
                region_i = 3 - region_i;
            }
        }
    }

    /* avoid array bounds errors, current REGION_NUM is 9
     * please modify the divided region above as soon as REGION_NUM is changed
     */
    if (region_i >= REGION_NUM) {
        region_i = REGION_NUM - 1;
    }

    return region_i;
}


static void audiolcyapt_region_update(lcyapt_result_t *p_out, u32_t ts, u8_t clear)
{
    u8_t  region_i, cbig_end = 0;
    u32_t cbigThres;

    // clear the old region result
    if (clear > 0) {
        memset(p_out->ts_region, 0, sizeof(p_out->ts_region));
        p_out->ts_region_total = 0;
        p_out->ts_region_max = 0;
        p_out->cbig_max = 0;
        return ;
    }

    if (ts <= 0) {
        return ;
    }

    region_i = audiolcyapt_region_locate(p_out, ts);

    if (p_out->ts_region[region_i] < 0xffff) {
        p_out->ts_region[region_i]++;
        p_out->ts_region_total++;
    }

    if (p_out->ts_region_max < ts) {
        p_out->ts_region_max = ts;
    }

    cbigThres = p_out->cbig_thres * p_out->pkt_time_ave / 100;

    // keep checking continuous big ts
    if (p_out->cbig_count > 0) {
        p_out->cbig_count++;
        p_out->cbig_sum += ts;
        p_out->cbig_sum -= p_out->pkt_time_ave;

        if (p_out->cbig_max < p_out->cbig_sum) {
            p_out->cbig_max = p_out->cbig_sum;
        }

        // stop checking continuous big ts
        if (p_out->cbig_sum < cbigThres * 4 / 5) {
            cbig_end = 1;
        } else if (p_out->cbig_count >= 5 && ts <= cbigThres) {
            cbig_end = 1;
        }
    // found continuous big ts start
    } else if (region_i >= 5 && ts >= cbigThres) {
        p_out->cbig_sum = ts;
        p_out->cbig_count = 1;
        if (p_out->cbig_max < p_out->cbig_sum) {
            p_out->cbig_max = p_out->cbig_sum;
        }
    }

    if (p_out->cbig_max < p_out->ts_region_max) {
        p_out->cbig_max = p_out->ts_region_max;
    }

    if (cbig_end) {
        //printk("lcyapt cbig %d_%d\n", p_out->cbig_count, p_out->cbig_max);
        p_out->cbig_count = 0;
        p_out->cbig_sum = 0;
    }
}

static u32_t audiolcyapt_region_info_analyse(audiolcy_adaptive_t *lcyapt, u8_t force)
{
    lcyapt_source_t *p_in  = &(lcyapt->source_info);
    lcyapt_result_t *p_out = &(lcyapt->result_info);

    u8_t  i, cur_i, end_i;
    u32_t cur_ts = 0, recv_time_sum = 0, pkt_time_sum = 0;

    // too few new data
    if ((!force && p_in->count < 4) \
     || (force && p_in->count <= 0))
    {
        return 0;
    }

    // cur_i points to the oldest data while end_i shows the amount of new data
    end_i = p_in->count;
    p_in->count = 0;
    cur_i = p_in->rindex;
    if (end_i > LCYAPT_PKTINFO_NUM) {
        cur_i += (end_i - LCYAPT_PKTINFO_NUM) % LCYAPT_PKTINFO_NUM;
        end_i = LCYAPT_PKTINFO_NUM;
    }

    // sum of raw_music_data in us received in a windows
    for (i = 0; i < LCYAPT_PKTINFO_NUM; i++) {
        if (p_in->frame_cnt[i] == 0) {
            break;
        }

        pkt_time_sum  += p_in->frame_cnt[i];
        recv_time_sum += p_in->time_span[i];
    }

    pkt_time_sum = pkt_time_sum * 128 * 1000 / lcyapt->lcycommon->sample_rate_khz; // unit us
    p_out->pkt_time_ave = pkt_time_sum / i;
    p_out->pkt_time_diff = recv_time_sum - pkt_time_sum;

    for (i = 0; i < end_i; (i++, cur_i++)) {
        cur_i %= LCYAPT_PKTINFO_NUM;
        cur_ts = p_in->time_span[cur_i];

        // append new data info
        audiolcyapt_region_update(p_out, cur_ts, 0);
    }

    p_in->rindex = cur_i % LCYAPT_PKTINFO_NUM;  // correct next read position

    return end_i;
}

void audiolcyapt_factor_statistics(audiolcy_adaptive_t *lcyapt, u16_t cur_factor)
{
    lcyapt_result_t *p_out = &(lcyapt->result_info);

    // total 100
    const uint8_t ts_region_factor_weighting_coeff[8] = {35, 22, 15, 11, 7, 5, 3, 2};

    u16_t ts_region_factor_delta, ts_region_factor_delta_square_avg, i;
    u32_t cur_timestamp, ts_region_factor_delta_square_weighting_total;
       
    cur_timestamp = os_uptime_get_32();
    if (p_out->factor_stat_flag == 0)
    {
        p_out->factor_stat_flag = 1;
        p_out->factor_last = cur_factor;
        p_out->factor_delta_square_add = 0;
        p_out->factor_stat_ts = cur_timestamp;
    }
    
    ts_region_factor_delta = abs((s16_t)p_out->factor_last - (s16_t)cur_factor);
    p_out->factor_delta_square_add += ts_region_factor_delta*ts_region_factor_delta;
    
    p_out->factor_count++;
    p_out->factor_last = cur_factor;
    
    if ((cur_timestamp - p_out->factor_stat_ts) >= (lcyapt->lcycommon->cfg->ALLM_down_timer_period_s * 1000))
    {
        ts_region_factor_delta_square_avg = p_out->factor_delta_square_add/p_out->factor_count;
        if (p_out->factor_stat_flag == 1)
        {
            p_out->factor_stat_flag = 2;
            for (i = 0; i < 8; i++)
            {
                p_out->factor_delta_square_averages[i] = ts_region_factor_delta_square_avg;
            }
        }
        else
        {
            for (i = 7; i >= 1; i--)
            {
                p_out->factor_delta_square_averages[i] = p_out->factor_delta_square_averages[i-1];
            }
            p_out->factor_delta_square_averages[0] = ts_region_factor_delta_square_avg;
        }

        //printf("lcyapt averages: ");

        ts_region_factor_delta_square_weighting_total = 0;
        for (i = 0; i < 8; i++)
        {
            /*
            if (i == 7)
                printf("%d\n", p_out->factor_delta_square_averages[i]);
            else
                printf("%d_", p_out->factor_delta_square_averages[i]);
            */
            ts_region_factor_delta_square_weighting_total +=
                p_out->factor_delta_square_averages[i] * ts_region_factor_weighting_coeff[i];
        }
        p_out->factor_delta_square_weighting = ts_region_factor_delta_square_weighting_total/100;
        //printf("lcyapt weighting: %d\n", p_out->factor_delta_square_weighting);

        p_out->factor_delta_square_add = 0;
        p_out->factor_count = 0;
        p_out->factor_stat_ts = cur_timestamp;
    }
}

static void audiolcyapt_info_clear(audiolcy_adaptive_t *lcyapt)
{
    memset(&(lcyapt->source_info), 0, sizeof(lcyapt_source_t));
    audiolcyapt_region_update(&(lcyapt->result_info), 0, 1);
}

static void audiolcyapt_info_analyse(audiolcy_adaptive_t *lcyapt, u8_t clear, u8_t force)
{
    lcyapt_result_t *p_out = &(lcyapt->result_info);

    u32_t factor = 100, cur_time_s, temp_value;

     // re-new our analysis result
    if (audiolcyapt_region_info_analyse(lcyapt, force) == 0) {
        return ;
    }

    temp_value = lcyapt->lcycommon->cfg->ALLM_recommend_period_s;
    // quickly recommend every 1 or 2 seconds
    if (lcyapt->quick_calc > 0) {
        temp_value = temp_value / 10;
        temp_value = (temp_value == 0) ? 1 : temp_value;
        temp_value = (temp_value  > 2) ? 2 : temp_value;
    }

    cur_time_s = os_uptime_get_32() / 1000;
    if (!force && cur_time_s < lcyapt->recommend_timer + temp_value) {
        return ;
    }

    // 1. continuous big time_span
    temp_value = (p_out->cbig_max > 0) ? p_out->cbig_max : p_out->ts_region_max;
    if (temp_value > 0) {
        factor = temp_value * 100 / p_out->pkt_time_ave;
    }

    // 2. impact of window received pkt cost difference
    if (p_out->pkt_time_diff > 0 && p_out->pkt_time_diff > p_out->pkt_time_ave) {
        factor += (p_out->pkt_time_diff * 100 / p_out->pkt_time_ave);
    }

    // 3. cfg factor
    factor += lcyapt->cfg_factor * 100 / 5;  // cfg_factor max value is 5

    // set flag to do recommendation
    p_out->result_factor = (u16_t)factor;
    lcyapt->need_calc = 1;

    if (lcyapt->quick_calc > 0) {
        clear = 0;  //don't clear data when quickly calc

        lcyapt->quick_count++;
        if (lcyapt->quick_count >= 10) {
            lcyapt->quick_calc = 2;
        }
    } else {
        audiolcyapt_factor_statistics(lcyapt, p_out->result_factor);
    }

    lcyapt->recommend_timer = cur_time_s;

    if (p_out->ts_region_total >= 1000) {
        clear = 1;
    }

    // clear all the ts_region info
    if (clear > 0) {
        audiolcyapt_region_update(p_out, 0, 1);
    }
}

void audiolcyapt_info_collect(void *handle, audiolcy_pktinfo_t *pktinfo)
{
    audiolcy_adaptive_t *lcyapt = (audiolcy_adaptive_t *)handle;
    lcyapt_source_t *p_in = &(lcyapt->source_info);
    u8_t temp_index = 0;
    static u8_t log_en = 0;

    if (pktinfo == NULL || lcyapt->lcycommon->twsrole == BTSRV_TWS_SLAVE) {
        return ;
    }

    if (p_in->last_pkt_time > 0)
    {
        p_in->frame_cnt[p_in->windex] = (lcyapt->lcycommon->media_format == SBC_TYPE) ? pktinfo->frame_cnt : 8;
        p_in->time_span[p_in->windex] = k_cyc_to_us_floor32(pktinfo->sysclk_cyc - p_in->last_pkt_time);
        temp_index = p_in->windex;
        p_in->windex = (p_in->windex + 1) % LCYAPT_PKTINFO_NUM;
        p_in->count++;
    }

    p_in->last_pkt_time = pktinfo->sysclk_cyc;

    if (p_in->time_span[temp_index] >= (lcyapt->result_info.cbig_thres * lcyapt->result_info.pkt_time_ave / 100)) {
        log_en = 5;
    }

    if (log_en > 0) {
        log_en -= 1;
/*        printk("lcyapt info_collect %d: %d, %d_%d_%u\n", p_in->windex, p_in->count, pktinfo->seq_no, \
            p_in->frame_cnt[temp_index], p_in->time_span[temp_index]);
*/
    }
}

/*!
 * return 0: done (disavowed), return lcy_ms: global variable updated, need handle it
 */
static u8_t audiolcyapt_latency_set(audiolcy_adaptive_t *lcyapt, u16_t ms, u8_t staccato)
{
    u32_t cur_time;

    // last adaptive latency is updating, forbid new update
    if (lcyapt->update_status == UPDATE_STA_PENDING) {
        printk("lcyapt update pending %d\n", ms);
        return 0;
    }

    // slave accept the new latency unconditionally
    if (lcyapt->lcycommon->twsrole == BTSRV_TWS_SLAVE) {
        if(ms < lcyapt->cfg_minimum) {
            lcyapt->cfg_minimum = ms;
        } else if (ms > lcyapt->cfg_maximum) {
            lcyapt->cfg_maximum = ms;
        }
    // maste or normal device limit the new latency to the range
    } else {
        if(ms < lcyapt->cfg_minimum + lcyapt->scc_imcrement) {
            ms = lcyapt->cfg_minimum + lcyapt->scc_imcrement;
        } else if (ms > (lcyapt->cfg_maximum)) {
            ms = lcyapt->cfg_maximum;
        }

        if (lcyapt->pending_lcy == ms) {
            return 0;
        }
    }

    if (staccato && lcyapt->lcycommon->twsrole != BTSRV_TWS_SLAVE) {
        // staccato up adjust, clear all the down adjust timer
        audiolcyapt_down_timer_check(lcyapt, 0, 0);

        cur_time = os_uptime_get_32();

        if ((lcyapt->scc_timer == 0) \
         || (cur_time > lcyapt->scc_timer + lcyapt->lcycommon->cfg->ALLM_scc_ignore_period_ms))
        {
            lcyapt->scc_count = 0;
            lcyapt->scc_timer = cur_time;
        }

        lcyapt->scc_count++;

        // don't adjust if staccato is less and adaptive_factor is small
        if ((lcyapt->cfg_factor <= STACCATO_DIS_FACTOR) \
         && (lcyapt->scc_count < lcyapt->lcycommon->cfg->ALLM_scc_ignore_count))
        {
            printk("lcyapt scc dis %d, %d\n", lcyapt->scc_count, ms);
            return 0;
        }

        // to avoid the staccato occurs again?
        if ((ms > lcyapt->pending_lcy) && (ms - lcyapt->pending_lcy) >= 5)
        {
            lcyapt->scc_imcrement = (ms - lcyapt->cfg_minimum);
            lcyapt->scc_lcy_min = -1;
            printk("lcyapt scc start: %d, %d\n", lcyapt->scc_imcrement, lcyapt->scc_timer/1000);
        }
    }

    // pre-save the aptlcy, and save it when it takes effect
    lcyapt->pending_lcy = ms;
    lcyapt->staccato = staccato;
    lcyapt->update_status = UPDATE_STA_NEED;
    printk("lcyapt set: %d\n", lcyapt->pending_lcy);

    // adjust as soon as staccato
    if (lcyapt->staccato) {
        audiolcyapt_latency_adjust(lcyapt, 1, 1);
        return 0;
    }

    return lcyapt->pending_lcy;
}

/* return the current latency if the adaptive value is invalid
 */
u16_t audiolcyapt_latency_get(void *handle)
{
    audiolcy_adaptive_t *lcyapt = (audiolcy_adaptive_t *)handle;
    audiolcyapt_global_t *g_lcyapt = &g_adaptive_latency;
    u16_t ret_lcy = lcyapt->current_lcy;

    if (g_lcyapt->is_valid && g_lcyapt->format == lcyapt->lcycommon->media_format) {
        ret_lcy = g_lcyapt->latency;

        if (ret_lcy < lcyapt->cfg_minimum + lcyapt->scc_imcrement) {
            ret_lcy = lcyapt->cfg_minimum + lcyapt->scc_imcrement;
        } else if (ret_lcy > lcyapt->cfg_maximum) {
            ret_lcy = lcyapt->cfg_maximum;
        }
    }

    //printf("lcyapt get %dms, %d_%d_%d\n", ret_lcy, g_lcyapt->latency, g_lcyapt->format, g_lcyapt->is_valid);

    return ret_lcy;
}

static void audiolcyapt_thres_change(audiolcy_adaptive_t *lcyapt, s16_t min_dlt, s16_t max_dlt)
{
    if (min_dlt < 0 && (s16_t)(lcyapt->cfg_minimum + min_dlt) < 0) {
        lcyapt->cfg_minimum = 0;
    } else {
        lcyapt->cfg_minimum += min_dlt;
    }

    if (max_dlt < 0 && (s16_t)(lcyapt->cfg_maximum + max_dlt) < (s16_t)lcyapt->cfg_minimum) {
        lcyapt->cfg_maximum = lcyapt->cfg_minimum;
    } else {
        lcyapt->cfg_maximum += max_dlt;
    }

    if (lcyapt->cfg_maximum < 110) {
        lcyapt->cfg_maximum = 110;
    }
    if (lcyapt->cfg_maximum < lcyapt->cfg_minimum + 23) {
        lcyapt->cfg_maximum = lcyapt->cfg_minimum + 23;
    }

    // minimum threshold is raised, check current latency
    if (min_dlt > 0 && lcyapt->current_lcy < lcyapt->cfg_minimum) {
        audiolcyapt_latency_set(lcyapt, lcyapt->cfg_minimum, 0);
    }

    printk("lcyapt thres_changed %d_%d\n", lcyapt->cfg_minimum, lcyapt->cfg_maximum);
}


static u32_t audiolcyapt_staccato_timer_handle(audiolcy_adaptive_t *lcyapt)
{
    u32_t cur_time;

    if (lcyapt->scc_imcrement <= 0) {
        return 0;
    }

    cur_time = os_uptime_get_32();
    if (cur_time > lcyapt->scc_timer + lcyapt->lcycommon->cfg->ALLM_scc_clear_period_s * 1000) {
    #if 1
        if (lcyapt->scc_imcrement > 0) {
            lcyapt->scc_imcrement = 0;
        }
    #else
        if (lcyapt->scc_imcrement >= lcyapt->lcycommon.cfg.ALLM_scc_clear_step) {
            lcyapt->scc_imcrement -= lcyapt->lcycommon.cfg.ALLM_scc_clear_step;
        } else {
            lcyapt->scc_imcrement = 0;
        }
    #endif

        printk("lcyapt scc timer %d_%d\n", lcyapt->scc_imcrement, lcyapt->scc_lcy_min);

        lcyapt->scc_timer = cur_time;
        lcyapt->scc_lcy_min = -1;
    }

    return 0;
}

/* return the renewed timer index, no timer renewed when return -1
 */
static s32_t audiolcyapt_down_timer_update(audiolcy_adaptive_t *lcyapt, u16_t lcy)
{
    u8_t  i = 0;
    u32_t time_s = os_uptime_get_32() / 1000;

    for (i = 0; i < DOWN_TIMER_NUM; i++) {
        if (i > 0) {
            time_s = lcyapt->down_timer[i - 1] + lcyapt->lcycommon->cfg->ALLM_down_timer_period_s;
        }

        // add a new timer to record the lowest new_lcy_ms
        if (i == lcyapt->down_timer_num && lcyapt->down_timer_num < DOWN_TIMER_NUM) {
            lcyapt->down_timer_lcy[i] = lcy;
            lcyapt->down_timer[i] = time_s;
            lcyapt->down_timer_num++;
            printk("lcyapt timer add %d:%d_%d\n", i, lcyapt->down_timer_lcy[i], lcyapt->down_timer[i]);
            break;
        }

        // the last timer allows to be renewed by a lower latency
        if ((i == DOWN_TIMER_NUM - 1) && (lcy < lcyapt->down_timer_lcy[i])) {
            if (lcy + DOWN_TIMER_LAST_DIFF >= lcyapt->down_timer_lcy[i])
            {
                lcyapt->down_timer_lcy[i] = lcy;
                //lcyapt->down_timer[i] = time_s;  // renew the time?
                printk("lcyapt timer renew %d:%d_%d\n", i, lcyapt->down_timer_lcy[i], lcyapt->down_timer[i]);
                break;
            }
        }

        // check and renew the timer
        if (lcy >= lcyapt->down_timer_lcy[i]) {
            if (lcy >= lcyapt->down_timer_lcy[i] + 2)  // renew only when 2ms bigger
            {
                lcyapt->down_timer_lcy[i] = lcy;
                lcyapt->down_timer[i] = time_s;
                printk("lcyapt timer renew %d:%d_%d\n", i, lcyapt->down_timer_lcy[i], lcyapt->down_timer[i]);
            }
            lcyapt->down_timer_num = i + 1;  // the timer behind will be cleared
            break;
        }
    }

    return (i == DOWN_TIMER_NUM) ? -1 : i;
}

/*!
 * return, 0:timer[0] is not changed, >0:new latency value
 */
static u32_t audiolcyapt_down_timer_check(audiolcy_adaptive_t *lcyapt, u16_t cur_lcy_ms, u16_t new_lcy_ms)
{
    s32_t i, ret = 0, temp_ret = -1;
    u32_t delta, timer_num, timer_lcy;

    if ((new_lcy_ms > 0) && (new_lcy_ms < lcyapt->cfg_minimum + lcyapt->scc_imcrement)) {
        new_lcy_ms = lcyapt->cfg_minimum + lcyapt->scc_imcrement;
    } else if (new_lcy_ms > lcyapt->cfg_maximum) {
        new_lcy_ms = lcyapt->cfg_maximum;
    }

    // up adjust, clear all the down adjust timer (0 >= 0: clear all timers)
    if (new_lcy_ms >= cur_lcy_ms) {
        if (lcyapt->down_timer_num > 0) {
            printk("lcyapt timer stop %d_%d\n", cur_lcy_ms, new_lcy_ms);
        }
        lcyapt->down_timer_num = 0;
        memset(lcyapt->down_timer_lcy, 0, sizeof(lcyapt->down_timer_lcy));
        memset(lcyapt->down_timer, 0, sizeof(lcyapt->down_timer));
        return new_lcy_ms;
    }

    delta = cur_lcy_ms - new_lcy_ms;

    /* use as many timers as possible, but don't divide into multi timer any more when there are timers
     * the more timer_num is, the smoother latency is, and the slower it adjusts
     */
    timer_num = 1;
    if (lcyapt->down_timer_num == 0) {
        if (delta >= 20 && delta >= (DOWN_TIMER_SPAN * (DOWN_TIMER_NUM + 1))) {
            timer_num = DOWN_TIMER_NUM;
        } else if (delta >= 7 && delta >= DOWN_TIMER_SPAN) {
            timer_num = 2;

            if (delta > DOWN_TIMER_SPAN * 2) {
                timer_num = delta / DOWN_TIMER_SPAN;
            }

            if (DOWN_TIMER_NUM > 2 && timer_num > DOWN_TIMER_NUM) {
                timer_num = DOWN_TIMER_NUM -1;
            }

            timer_num = (timer_num > DOWN_TIMER_NUM) ? DOWN_TIMER_NUM : timer_num;
        }
    }

    // 3 or more timers, the first timer would down adjust bigger
    if (timer_num >= 3) {
        delta /= (timer_num + 1);
    } else {
        delta /= timer_num;
    }

    // check and renew timer(s)
    for (i = timer_num - 1; i >= 0; i--) {
        timer_lcy = new_lcy_ms + (delta * i);
        temp_ret = audiolcyapt_down_timer_update(lcyapt, timer_lcy);

        // temp_ret=0 means timer[0] is renewed(need update g_lcyapt)
        if (temp_ret == 0) {
            ret = timer_lcy;
        }
    }

    // clear the rest useless timer
    i = lcyapt->down_timer_num;
    if (i < DOWN_TIMER_NUM) {
        memset(&(lcyapt->down_timer_lcy[i]), 0, sizeof(u8_t)*(DOWN_TIMER_NUM - i));
        memset(&(lcyapt->down_timer[i]), 0, sizeof(u32_t)*(DOWN_TIMER_NUM - i));
    }

    return ret;
}

static u32_t audiolcyapt_down_timer_handle(audiolcy_adaptive_t *lcyapt)
{
    u32_t i, cur_time;

    if (lcyapt->down_timer_num == 0) {
        return 1;
    }

    if (lcyapt->quick_calc > 0)
    {
        memset(lcyapt->down_timer_lcy, 0, sizeof(lcyapt->down_timer_lcy));
        memset(lcyapt->down_timer, 0, sizeof(lcyapt->down_timer));
        lcyapt->down_timer_num = 0;
        return 1;
    }

    cur_time = (u32_t)os_uptime_get_32() / 1000;  // unit second

    // timer[0] goes off, clear timer[0] and re-sort other timer(s)
    if (lcyapt->down_timer[0] + lcyapt->lcycommon->cfg->ALLM_down_timer_period_s <= cur_time) {
        lcyapt->down_timer_num -= 1;

        printk("lcyapt timer gooff %d(%d)\n", lcyapt->down_timer_lcy[0], lcyapt->down_timer_num);

        for (i = 0; i < lcyapt->down_timer_num; i++) {
            lcyapt->down_timer_lcy[i] = lcyapt->down_timer_lcy[i+1];
            lcyapt->down_timer[i] = lcyapt->down_timer[i+1];
        }

        // clear the oldest timer(has been moved)
        lcyapt->down_timer_lcy[lcyapt->down_timer_num] = 0;
        lcyapt->down_timer[lcyapt->down_timer_num] = 0;

        return 1;
    }

    return 0;
}

u32_t audiolcyapt_latency_adjust_slave(void *handle, u16_t lcy_ms, u8_t mode)
{
    audiolcy_adaptive_t *lcyapt = (audiolcy_adaptive_t *)handle;

    // update the threshold and save to aptlcy_pre_set
    audiolcyapt_latency_set(lcyapt, lcy_ms, 0);
    lcyapt->update_status = UPDATE_STA_NONE;

    // save to the global variable
    audiolcyapt_global_set_latency(lcyapt->lcycommon->media_format, lcy_ms);
    lcyapt->current_lcy = lcy_ms;

    if (mode == LCYSYNC_SAVE) {
        return 0;
    }

    // set the new threshold
    audiolcy_ctrl_latency_change(lcy_ms);

    printk("lcyapt adjust slave -> %d ms\n", lcy_ms);

    return 0;
}

/* return 0: adjust done, return lcy_ms: delay handle
 */
u32_t audiolcyapt_latency_adjust(void *handle, u8_t adjust_en, u8_t sync_en)
{
    audiolcy_adaptive_t *lcyapt = (audiolcy_adaptive_t *)handle;
    audiolcy_common_t *lcycommon = lcyapt->lcycommon;

    u16_t aps_curlcy = audio_aps_get_latency() / 1000; //us -> ms
    u16_t old_ms, new_ms = lcyapt->pending_lcy;
    u8_t  lcy_updated = 0, timer_update = 0, syncmode = 0;

    if (lcycommon->twsrole == BTSRV_TWS_SLAVE) {
        return 0;
    }

    if (lcyapt->down_timer_num > 0 && lcyapt->quick_calc == 0) {
        // timer go off, need to check the next timer
        if (audiolcyapt_down_timer_handle(lcyapt) > 0) {
            timer_update = 1;
        } else { // wait for timer
            return new_ms;
        }
    }

    // clear the flag
    if (lcyapt->down_timer_num == 0) {
        lcyapt->update_status = UPDATE_STA_NONE;
    }

    old_ms = aps_curlcy;
    if (audiolcy_get_latency_mode() != LCYMODE_ADAPTIVE) {
        old_ms = audiolcyapt_latency_get(lcyapt);
    }

    // disavow the adjustment (new_ms is definitely within the range)
    if ((abs(new_ms - old_ms) < LCYAPT_ADJUST_DIFF_MS) \
     && (new_ms != lcyapt->cfg_minimum) && (old_ms != lcyapt->cfg_minimum))
    {
        goto adjust_end;
    }

    // save aptlcy to global variable
    audiolcyapt_global_set_latency(lcycommon->media_format, new_ms);
    lcyapt->current_lcy = new_ms;
    printk("lcyapt adjust (%d) %d -> %d ms\n", aps_curlcy, old_ms, new_ms);

    lcy_updated  = 1;  // global variable updated, sync to slave
    syncmode = LCYSYNC_UPDATE;
    // don't adjust in NLM or LLM
    if (audiolcy_get_latency_mode() != LCYMODE_ADAPTIVE) {
        syncmode = LCYSYNC_SAVE;
        goto adjust_end;
    }

    // set the new threshold
    audiolcy_ctrl_latency_change(new_ms);

    // enable quick aps, single device or master device
    // not supported yet

    // insert some mute data if latency is raised by staccato
    if (adjust_en && (new_ms > old_ms) && lcyapt->staccato)
    {
        #if 0  // drop data not supported yet
        lcyapt->update_status = UPDATE_STA_PENDING;

        if (cur_latency > 0) {
            cur_latency_samples = cur_latency;
        } else {
            cur_latency_samples = calc_latency();
        }
        adjust_flag = -1;
        adjust_pkt_num = cur_pkt_num;
        need_adjust_samples = abs(cur_target_latency_samples - cur_latency_samples);

        // single device, insert data immediately
        if (sync_en == 0 || lcycommon->twsrole == BTSRV_TWS_NONE) {
            lcyctrl_status = LCYCTRL_STATUS_DROPDATA;
        }
        // TWS devices, insert data together after some pkts?
        else if (sync_en && lcycommon->twsrole == BTSRV_TWS_MASTER)
        {
            //ctx->latency->adjust_pkt_num += LCYAPT_STACCATO_ADJSUT_DELAY; // delay adjust
            lcyctrl_status = LCYCTRL_STATUS_DROPDATA;         // adjust right now
        }
        #endif
    }

adjust_end:

    // clear staccato flag
    lcyapt->staccato = 0;

    // set the next timer latency
    if (timer_update > 0 && lcyapt->down_timer_num > 0) {
        audiolcyapt_latency_set(lcyapt, lcyapt->down_timer_lcy[0], 0);
    }

    // TWS devices, send latency sync pkt to slave device
    if (lcy_updated > 0 && sync_en && lcycommon->twsrole == BTSRV_TWS_MASTER) {
        audiolcy_ctrl_latency_twssync(syncmode, new_ms);
    }

    return 0;
}

void audiolcyapt_latency_calcation(audiolcy_adaptive_t *lcyapt, u8_t log_en, u8_t force)
{
    lcyapt_result_t *p_out = &(lcyapt->result_info);

    u32_t sr_hz, us = 0, samples = 0, rem_lcy_ms, temp_value;
    u16_t allm_lcy_ms, cur_result_factor;
    u8_t  invalid_flag = 0;

    if (!force && lcyapt->need_calc == 0) {
        return ;
    }

    lcyapt->need_calc = 0;

    // 1. min pcmbuf guarantee, half a pkt time
    us += p_out->pkt_time_ave / 2;

    // 2. fixed cost
    audiolcy_ctrl_get_fixedcost(&samples);

    // 3. pattern factor recommend
    cur_result_factor = p_out->result_factor;
    if (p_out->factor_delta_square_weighting >= 2025) {
        cur_result_factor += 24;
    } else if (p_out->factor_delta_square_weighting >= 625) {
        cur_result_factor += 12;
    }

    us += p_out->pkt_time_ave * cur_result_factor / 100;

    // convert to time in ms
    rem_lcy_ms = (us + 700) / 1000; //+700: rounding
    sr_hz = audio_aps_get_samplerate_hz(lcyapt->lcycommon->sample_rate_khz == 48);
    rem_lcy_ms += (samples * 1000 / sr_hz);

    // recommemd a bigger lcy to avoid staccato in the beginning
    if (lcyapt->first_circle) {
        lcyapt->first_circle = 0;

        temp_value  = (p_out->pkt_time_ave * 80 / 100 + 700) / 1000;
        temp_value += lcyapt->cfg_minimum;
        if (rem_lcy_ms < temp_value) {
            rem_lcy_ms = temp_value;
        }
    }

    if (audiolcy_get_latency_mode() == LCYMODE_ADAPTIVE) {
        allm_lcy_ms = lcyapt->current_lcy;
    } else {
        // current is in NLM or LLM
        allm_lcy_ms = audiolcyapt_latency_get(lcyapt);
        if (allm_lcy_ms == lcyapt->current_lcy) {  // global apt_lcy invalid
            invalid_flag = 1;
            allm_lcy_ms = rem_lcy_ms;
        }
    }

    if (log_en) {
        temp_value = audio_aps_get_latency() / 1000;  // current latency, us -> ms
        printk("lcyapt calc(%d): %d->%d, %dHz, %dtimer, %d_%d_%d, %d_%d\n", temp_value, allm_lcy_ms, rem_lcy_ms, \
            sr_hz, lcyapt->down_timer_num, p_out->result_factor, p_out->cbig_max, p_out->pkt_time_ave, \
            samples, p_out->factor_delta_square_weighting);
    }

    if (lcyapt->quick_calc == 1) {
        if (rem_lcy_ms <= lcyapt->quick_max) {
            return;
        }
        lcyapt->quick_max = rem_lcy_ms;
    }

    // differ too little, disavow
    if ((invalid_flag == 0) && (lcyapt->down_timer_num == 0) \
     && (abs(rem_lcy_ms - allm_lcy_ms) < LCYAPT_ADJUST_DIFF_MS))
    {
        return ;
    }

    // disable down timer when quickly adjust
    if (lcyapt->quick_calc == 0) {
        rem_lcy_ms = audiolcyapt_down_timer_check(lcyapt, allm_lcy_ms, rem_lcy_ms);
    } else {
        // quick calc pre-disabled, disable it
        if (lcyapt->quick_calc == 2) {
            lcyapt->quick_calc = 0;
        }
    }

    if (rem_lcy_ms > 0) {  // save the lcy_ms
        rem_lcy_ms = audiolcyapt_latency_set(lcyapt, rem_lcy_ms, 0);
    }

    if (rem_lcy_ms > 0) {
        rem_lcy_ms = audiolcyapt_latency_adjust(lcyapt, 0, 1);
    }
}

static void audiolcyapt_mtu_check(audiolcy_adaptive_t *lcyapt)
{
    if (lcyapt->lcycommon->media_format != SBC_TYPE) {
        lcyapt->mtu_check = 0;
        return ;
    }

    if (lcyapt->mtu_check > 0 && lcyapt->source_info.count > 0) {
        lcyapt->mtu_check = 0;
        #if 0  // not used yet
        if (lcyapt->lcycommon.cfg.BM_Transfer_Opt) {
            // raise the minimum adaptive latency threshold
            if (max_pkt_length > 367) {
                audiolcyapt_thres_change(lcyapt, TRANS_OPT_THRES_UP, 0);
            }
        } else {
            // decrease the minimum adaptive latency threshold
            if (max_pkt_length <= 367) {
                audiolcyapt_thres_change(lcyapt, -TRANS_OPT_THRES_DOWN, 0);
            }
        }
        #endif
    }
}

static u8_t audiolcyapt_lcy_check(audiolcy_adaptive_t *lcyapt)
{
    u8_t stop_check = 1;

    // check global apt lcy and tws sync
    if (lcyapt->lcycommon->twsrole != BTSRV_TWS_SLAVE)
    {
        if (audiolcyapt_global_check_valid(lcyapt->lcycommon->media_format) == 0) {
            lcyapt->quick_calc = 1;
            lcyapt->quick_count = 0;
            lcyapt->quick_max = 0;

            if (audiolcy_get_latency_mode() == LCYMODE_ADAPTIVE) {
                stop_check = 0;
                if (lcyapt->source_info.count > 0) {
                    stop_check = 1;
                    lcyapt->first_circle = 1;
                    audiolcyapt_info_analyse(lcyapt, 0, 1);  // force analyse
                    audiolcyapt_latency_calcation(lcyapt, 1, 1);
                }
            }
        }
    }

    return stop_check;
}

/*!
 * this routine would run periodically
 */
void audiolcy_adaptive_main(void *handle)
{
    audiolcy_adaptive_t *lcyapt = (audiolcy_adaptive_t *)handle;
    audiolcy_common_t *lcycommon;

    if (lcyapt == NULL) {
        return ;
    }
	
    lcycommon = lcyapt->lcycommon;

    if (lcycommon->twsrole == BTSRV_TWS_SLAVE) {
        return ;
    }

    if (lcyapt->mtu_check > 0) {
        audiolcyapt_mtu_check(lcyapt);
    }

    if (lcyapt->scc_imcrement > 0) {
        audiolcyapt_staccato_timer_handle(lcyapt);
    }

    if (lcyapt->status != LCYAPT_STA_PLAYING)
    {
        if (lcyapt->status < LCYAPT_STA_LCYSYNC && lcycommon->twsrole != 0xFF) {
            // check latency and sync to slave if needed
            if (audiolcyapt_lcy_check(lcyapt) > 0) {
                if (lcyapt->status == LCYAPT_STA_PREPLAY) {
                    lcyapt->status  = LCYAPT_STA_PLAYING;
                } else {
                    lcyapt->status  = LCYAPT_STA_LCYSYNC;   // playing not started yet
                }
            }
        }

        return ;
    }

    // check collected info and then do analysis
    audiolcyapt_info_analyse(lcyapt, 0, 0);

    // do calculation and change current apt lcy if needed
    if (lcyapt->need_calc > 0) {
        audiolcyapt_latency_calcation(lcyapt, 1, 0);
    }

    // update apt lcy and check down timer
    if (lcyapt->down_timer_num > 0 || lcyapt->update_status == UPDATE_STA_NEED) {
        audiolcyapt_latency_adjust(lcyapt, 1, 1);
    }
}

void audiolcy_adaptive_start_play(void *handle)
{
    audiolcy_adaptive_t *lcyapt = (audiolcy_adaptive_t *)handle;
    if (lcyapt == NULL) {
        return ;
    }

    if (lcyapt->lcycommon->twsrole == BTSRV_TWS_SLAVE) {
        lcyapt->status = LCYAPT_STA_PLAYING;
        return ;
    }

    if (lcyapt->status == LCYAPT_STA_PLAYING) {
        return ;
    }

    if (lcyapt->status == LCYAPT_STA_LCYSYNC) {
        lcyapt->status  = LCYAPT_STA_PLAYING;
    } else {
        lcyapt->status  = LCYAPT_STA_PREPLAY;
    }

    printk("lcyapt set start %d\n", lcyapt->status);
}

void audiolcy_adaptive_twsrole_change(void *handle, u8_t old_role, u8_t new_role)
{
    audiolcy_adaptive_t *lcyapt = (audiolcy_adaptive_t *)handle;

    // audiolcy_adaptive_start_play before tws_role comfirmed, possibly, slave was playing in PREPLAY status
    if (old_role == BTSRV_TWS_SLAVE && lcyapt->status == LCYAPT_STA_PREPLAY) {
        lcyapt->status = LCYAPT_STA_PLAYING;
    }

    // switch to slave device, clear the old source info to avoid mistaken used
    if (new_role == BTSRV_TWS_SLAVE) {
        audiolcyapt_info_clear(lcyapt);
    }
}

void *audiolcy_adatpive_init(audiolcy_common_t *lcycommon)
{
    audiolcy_adaptive_t *lcyapt;

    // ALLM is available only when SBC music or AAC music
    if ((lcycommon == NULL) || (lcycommon->cfg->BM_Use_ALLM == 0) \
     || (lcycommon->stream_type  != AUDIO_STREAM_MUSIC) \
     || (lcycommon->media_format != SBC_TYPE && lcycommon->media_format != AAC_TYPE))
    {
        return NULL;
    }

    lcyapt = mem_malloc(sizeof(audiolcy_adaptive_t));
    if (lcyapt == NULL) {
        return NULL;
    }

    memset(lcyapt, 0, sizeof(audiolcy_adaptive_t));
    lcyapt->lcycommon = lcycommon;

    // get cfg latency of LLM (current might be NLM)
    lcyapt->cfg_minimum = audio_policy_get_increase_threshold(1, lcycommon->media_format) / 1000; //us -> ms
    lcyapt->cfg_maximum = lcycommon->cfg->BM_ALLM_Upper;
    lcyapt->cfg_factor  = lcycommon->cfg->BM_ALLM_Factor;

    // default cfg value 60ms used
    if (lcyapt->cfg_minimum == 0) {
        lcyapt->cfg_minimum = 60;
    }

    // maximum is cfg 110 ~ 180
    if (lcyapt->cfg_maximum < 110) {
        lcyapt->cfg_maximum = 140;
    }

    audiolcyapt_thres_change(lcyapt, 0, 0);

    // dual phone switch, last adaptive latency is invalid
    //if ()
    //{
    //    audiolcyapt_global_set_invalid();
    //}

    // ts that bigger than this percentage of pkt time is big ts, default 150%
    lcyapt->result_info.cbig_thres = 150;
    // the bigger Adaptive_Factor is, the lower cbig_thres is, maximum is 200%
    lcyapt->result_info.cbig_thres += ((5 - lcyapt->cfg_factor) * 10);  //min 150
    if (lcyapt->result_info.cbig_thres > 200) {
        lcyapt->result_info.cbig_thres = 200;
    }

    // use last adaptive lcy
    lcyapt->current_lcy = audiolcyapt_latency_get(lcyapt);
    if (lcyapt->current_lcy < lcyapt->cfg_minimum) {
        lcyapt->current_lcy = lcyapt->cfg_minimum;
    }

    if (lcycommon->media_format == SBC_TYPE) {
        lcyapt->mtu_check = 1;  // need check pkt_size (mtu)
    }

    if (lcyapt->status != LCYAPT_STA_PREPLAY) {
        lcyapt->status = LCYAPT_STA_INIT;
    }

    SYS_LOG_INF("%dms,%d_%d", lcyapt->current_lcy, lcyapt->cfg_factor, lcyapt->result_info.cbig_thres);

    return lcyapt;
}

void audiolcy_adaptive_deinit(void *handle)
{
    if (handle) {
        memset(handle, 0, sizeof(audiolcy_adaptive_t));
        mem_free(handle);
        SYS_LOG_INF("done");
    }
}

