/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file audio latency basic
 */

#include "audiolcy_inner.h"
#include "dsp_hal.h"

extern audiolcy_global_t audiolcy_global;
static audiolcy_ctrl_t  *g_audiolcy_ctrl = NULL;


audiolcy_ctrl_t *get_audiolcyctrl(void)
{
    return g_audiolcy_ctrl;
}

void put_audiolcyctrl(void)
{
    g_audiolcy_ctrl = NULL;
}

void set_audiolcyctrl(void *lcyctrl)
{
    g_audiolcy_ctrl = lcyctrl;
}

void audiolcy_ctrl_set_twsrole(u8_t new_role)
{
    audiolcy_ctrl_t *lcyctrl = get_audiolcyctrl();
    u8_t  old_role;

    if (lcyctrl) {
        old_role = lcyctrl->lcycommon.twsrole;
        if (old_role == new_role) {
            return ;
        }

        printk("lcyctrl twsrole: %d->%d\n", old_role, new_role);

        if (lcyctrl->lcyapt) {
            audiolcy_adaptive_twsrole_change(lcyctrl->lcyapt, old_role, new_role);
        }

        lcyctrl->lcycommon.twsrole = new_role;
    }
}

void audiolcy_ctrl_set_pktinfo(audiolcy_pktinfo_t *pktinfo)
{
    audiolcy_ctrl_t *lcyctrl = get_audiolcyctrl();

    if (lcyctrl) {
        if (lcyctrl->max_pkt_len < pktinfo->pkt_len) {
            lcyctrl->max_pkt_len = pktinfo->pkt_len;
        }

        if (lcyctrl->lcyapt) {
            audiolcyapt_info_collect(lcyctrl->lcyapt, pktinfo);
        }
    }
}

void audiolcy_ctrl_set_start(void)
{
    audiolcy_ctrl_t *lcyctrl = get_audiolcyctrl();

    if (lcyctrl)
    {
        if (lcyctrl->lcyapt) {
            audiolcy_adaptive_start_play(lcyctrl->lcyapt);
        }
    }
}

u32_t audiolcy_ctrl_get_hfp_fixedcost(void)
{
#if 0  // not sure yet
    audiolcy_ctrl_t *lcyctrl = get_audiolcyctrl();

    u32_t hfp_fixed_cost_us = 0;

    if (lcyctrl == NULL || lcyctrl->lcycommon.stream_type != AUDIO_STREAM_VOICE) {
        return 0;
    }

    // 1. delay of plc algorithm
    hfp_fixed_cost_us += 7500;

    // 2. delay of AGC algorithm
    hfp_fixed_cost_us += 10000;

    // us to samples
    return hfp_fixed_cost_us * lcyctrl->lcycommon.sample_rate_khz / 1000;
#else
    return 0;
#endif
}

void audiolcy_ctrl_get_fixedcost(u32_t *fixed)
{
    audiolcy_ctrl_t *lcyctrl = get_audiolcyctrl();

    u32_t fixed_cost_us = 0, fixed_cost_samples = 0;

    if (fixed == NULL) {
        return ;
    }

    if (lcyctrl == NULL) {
        *fixed = 0;
        return ;
    }

    // 1. fixed delay of IIR of DAC (differ from ICs), samples
    //fixed_cost_samples += 28;  // 0.63ms@44KHz, 0.58ms@44KHz, 1.75ms@16KHz

    // 2. time costed on transmission, us
    if (lcyctrl->lcycommon.stream_type == AUDIO_STREAM_MUSIC) {
        // a2dp tranfer cost is differ from mtu
        if (lcyctrl->max_pkt_len > 679) {
            fixed_cost_us += 5600;
        } else if (lcyctrl->max_pkt_len > 367) {
            fixed_cost_us += 3200;
        } else {
            fixed_cost_us += 1800;
        }
    } else {
        fixed_cost_us += 600;  // 0.6ms for hfp
    }

    // 3. fixed delay of hfp algorithm, samples
    fixed_cost_samples += audiolcy_ctrl_get_hfp_fixedcost();

    fixed_cost_samples += fixed_cost_us * lcyctrl->lcycommon.sample_rate_khz / 1000;
    *fixed = fixed_cost_samples;
}

u32_t audiolcy_ctrl_latency_change(u16_t lcy_ms)
{
    audiolcy_ctrl_t *lcyctrl = get_audiolcyctrl();

    if (lcyctrl) {
        if (lcy_ms == 0) {
            lcy_ms = audiolcy_get_latency_threshold(lcyctrl->lcycommon.media_format) / 1000; //us -> ms
        }

        // indicate by sending msg to media_service
        if (lcyctrl->media_handle) {
            media_player_set_audio_latency(lcyctrl->media_handle, lcy_ms * 1000); //ms -> us
        }
    }

    return lcy_ms;
}


void audiolcy_ctrl_drop_insert_data(u32_t new_lcy_ms)
{
    audiolcy_ctrl_t *lcyctrl = get_audiolcyctrl();
    audiolcy_common_t *lcycommon = &(lcyctrl->lcycommon);

    struct decoder_dspfunc_data_adjust  data_adjust;
    u16_t cur_lcy_samples = 0, new_lcy_samples = 0, adjust_samples = 0;
    u8_t  adjust_flag = 0;

    if (lcyctrl == NULL || lcyctrl->dsp_handle == NULL) {
        return ;
    }

    if (new_lcy_ms == 0) {
        new_lcy_ms = audiolcy_get_latency_threshold(lcycommon->media_format) / 1000; //us -> ms
    }

    //calc adjust frames by samples
    cur_lcy_samples = audio_aps_get_latency() / 1000; //us -> ms
    cur_lcy_samples = cur_lcy_samples * lcycommon->sample_rate_khz;
    new_lcy_samples = new_lcy_ms * lcycommon->sample_rate_khz;

    if (new_lcy_samples > cur_lcy_samples) {
        adjust_samples = new_lcy_samples - cur_lcy_samples;
        adjust_flag = 2;  //insert
    } else if (new_lcy_ms < cur_lcy_samples) {
        adjust_samples = cur_lcy_samples - new_lcy_samples;
        adjust_flag = 1;  //drop
    }

    // too litle, aps instead
    if (adjust_flag == 0 || adjust_samples < 128*5) {
        return ;
    }

    printk("lcyctrl data_adjust %d_%d\n", adjust_flag, adjust_samples);

    memset(&data_adjust, 0, sizeof(data_adjust));
    data_adjust.start_seq = 0;
    data_adjust.adj_flag = adjust_flag;
    data_adjust.adj_samples = adjust_samples;
    dsp_session_config_func(lcyctrl->dsp_handle, DSP_FUNCTION_DECODER, DSP_CONFIG_DATA_ADJUST, sizeof(data_adjust), &data_adjust);
}


void audiolcy_ctrl_latency_twssync_deal(void *param)
{
    audiolcy_ctrl_t *lcyctrl = get_audiolcyctrl();
    audiolcy_lcysync_t *sync_pkt = (audiolcy_lcysync_t *)param;

    if (lcyctrl == NULL || lcyctrl->lcycommon.twsrole != BTSRV_TWS_SLAVE) {
        return ;
    }

    lcyctrl->lcy_send_repeat = 0;
    if (sync_pkt->mode == LCYSYNC_REPEAT) {
        // the first lcysync pkt was received
        if ((lcyctrl->lcy_recv_repeat > 0) && (lcyctrl->lcysync.pkt_num == sync_pkt->pkt_num)) {
            printk("lcyctrl deal: repeat\n");
            lcyctrl->lcy_recv_repeat = 0;
            memset(&(lcyctrl->lcysync), 0, sizeof(audiolcy_lcysync_t));
            return ;
        }

        // the first lcysync pkt has not been received
        lcyctrl->lcy_recv_repeat = 0;
        sync_pkt->mode = LCYSYNC_SAVE;
        if (audiolcy_get_latency_mode() == LCYMODE_ADAPTIVE) {
            sync_pkt->mode = LCYSYNC_UPDATE;
        }
        printk("lcyctrl deal: repeat used %d_%d\n", sync_pkt->latency, sync_pkt->mode);
    } else {
        //lcyctrl->lcy_recv_repeat = 1;  // no repeat
        //memcpy(&(lcyctrl->lcysync), sync_pkt, sizeof(audiolcy_lcysync_t));
    }

    if (lcyctrl->lcyapt) {
        audiolcyapt_latency_adjust_slave(lcyctrl->lcyapt, sync_pkt->latency, sync_pkt->mode);
    }

    #if 0  // not supported yet
    // stop the current adjustment
    if (current_adjusting_flag && sync_pkt->adjust != 0) {
        printf("force stop %d\n", need_adjust_samples);
        adjust_flag = 0;
        need_adjust_samples = 0;
        adjust_status = STATUS_PLAYING;
    }

    // adjust according to master's info, 1:drop data, 2:insert data
    
    if (sync_pkt->adjust == 1) {
        drop_data_flag;
    } else if (sync_pkt->adjust == 2) {
        insert_data_flag;
    }

    if (current_adjusting_flag && sync_pkt->adjust != 0) {
        adjust_pkt_num = sync_pkt->pkt_num;
        need_adjust_samples = sync_pkt->samples;

        // start to adjust. 2:avoid pkt num re-begin
        if ((current_pkt_num >= adjust_pkt_num) \
         || (current_pkt_num <= 5 && adjust_pkt_num >= 65530))
        {
            adjust_status = STATUS_ADJUSTING;
        }
    }
    #endif
}

void audiolcy_ctrl_latency_twssync(u8_t mode, u16_t lcy_ms)
{
    audiolcy_ctrl_t *lcyctrl = get_audiolcyctrl();
    audiolcy_lcysync_t sync_pkt = { 0, };

    if (lcyctrl == NULL) {
        return ;
    }

    // send last lcysync pkt again
    if (mode == LCYSYNC_REPEAT) {
        if (lcyctrl->lcy_send_repeat == 0) {
            return ;
        }

        memcpy(&sync_pkt, &(lcyctrl->lcysync), sizeof(audiolcy_lcysync_t));
        lcyctrl->lcy_send_pktnum = 0;  // not used
        lcyctrl->lcy_recv_repeat = 0;

        if (lcyctrl->lcy_send_repeat == 1) {
            lcyctrl->lcy_send_repeat = 0;
            sync_pkt.mode = LCYSYNC_REPEAT;
            memset(&(lcyctrl->lcysync), 0, sizeof(audiolcy_lcysync_t));
        } else {
            lcyctrl->lcy_send_repeat = 1;  // up to twice
        }

        printk("lcyctrl sync repeat, %drest\n", lcyctrl->lcy_send_repeat);

        goto send_pkt;
    }

    lcyctrl->lcy_send_pktnum = 0;

    sync_pkt.latency = lcy_ms;
    sync_pkt.mode = mode;

    sync_pkt.adjust = 0;

    #if 0  // not supported yet
    // only sync lcy_ms when using SAVE mode
    if (sync_pkt.mode != LCYSYNC_SAVE)
    {
        if (insert_data_flag) {
            sync_pkt.adjust = 2;  // insert data
        } else if (drop_data_flag) {
            sync_pkt.adjust = 1;  // drop data
        }

        if (sync_pkt.adjust > 0) {
            sync_pkt.pkt_num = start_pkt_num;
            sync_pkt.samples = need_adjust_samples;
        }
    }
    #endif

    // markdown lcysync pkt info and repeat again
    lcyctrl->lcy_send_repeat = 0;   // 0:no repeat, >0:repeat times
    lcyctrl->lcy_recv_repeat = 0;
    memcpy(&(lcyctrl->lcysync), &sync_pkt, sizeof(audiolcy_lcysync_t));

send_pkt:
    printk("lcyctrl send pkt %d_%dms\n", sync_pkt.mode, sync_pkt.latency);
    tws_send_pkt_user_type((void *)&sync_pkt, sizeof(audiolcy_lcysync_t));
}

/*!
 * \brief: initialize latency ctrl when playback opened for music or call
 * \note : do it earlier than anyone use latency mode or threshold
 */
void audiolcy_ctrl_init(audiolcy_init_param_t *param)
{
    audiolcy_ctrl_t *lcyctrl = get_audiolcyctrl();
    audiolcy_common_t *lcycommon;

    if (param->stream_type != AUDIO_STREAM_MUSIC && param->stream_type != AUDIO_STREAM_VOICE)
        return ;

    if (lcyctrl) {
        if (lcyctrl->lcycommon.stream_type == param->stream_type) {
            lcyctrl->lcycommon.media_format = param->media_fromat;
            lcyctrl->lcycommon.sample_rate_khz = param->sample_rate_khz;
            return ;
        }
        audiolcy_ctrl_deinit(lcyctrl->lcycommon.stream_type, 0);
    }

    lcyctrl = mem_malloc(sizeof(audiolcy_ctrl_t));
    if (lcyctrl == NULL) {
        SYS_LOG_INF("malloc fail");
        return ;
    }

    set_audiolcyctrl(lcyctrl);
    memset(lcyctrl, 0, sizeof(audiolcy_ctrl_t));

    lcyctrl->lcy_mode = audiolcy_global.cur_lcy_mode;
    lcyctrl->media_handle = param->media_handle;  // media player handle
    lcyctrl->dsp_handle = param->dsp_handle;
    lcycommon = &lcyctrl->lcycommon;
    lcycommon->cfg = &(audiolcy_global.cfg);
    lcycommon->stream_type = param->stream_type;
    lcycommon->media_format = param->media_fromat;  // AAC_TYPE(6), SBC_TYPE(10)
    lcycommon->sample_rate_khz = param->sample_rate_khz;
    lcycommon->twsrole = 0xFF;  // unknown yet

    if (lcycommon->stream_type == AUDIO_STREAM_MUSIC) {
        if (audiolcy_is_low_latency_mode()) {
            // in LLM or ALLM, music PLC init
            if (lcycommon->cfg->BM_PLC_Mode > 0) {
                lcyctrl->lcymplc = audiolcy_musicplc_init(lcycommon, lcyctrl->dsp_handle);
            }
        }

        // ALLM init
        if (lcycommon->cfg->BM_Use_ALLM) {
            lcyctrl->lcyapt = audiolcy_adatpive_init(lcycommon);
        }
    } else if (lcycommon->stream_type == AUDIO_STREAM_VOICE) {
        // lock to NLM on calling
        lcyctrl->lcy_mode = LCYMODE_NORMAL;
        lcyctrl->lcymode_switch_forbidden = 1;
    }

    SYS_LOG_INF("%d,%d_%d_%d", lcyctrl->lcy_mode, lcycommon->stream_type, lcycommon->media_format, lcycommon->sample_rate_khz);
}

/*!
 * \brief: deinit latency ctrl when playback closed
 * \n deinit when stream_type matched, check media_format instead if stream_type=0
 * \n set both stream_type and media_format to 0 to deinit unconditionally
 */
void audiolcy_ctrl_deinit(u8_t stream_type, u8_t media_format)
{
    audiolcy_ctrl_t *lcyctrl = get_audiolcyctrl();

    if (lcyctrl) {
        // check stream_type or media_format to avoid mistaken deinit by others
        if (stream_type > 0 && stream_type != lcyctrl->lcycommon.stream_type) {
            return ;
        }
        if (stream_type == 0 && media_format > 0 && media_format != lcyctrl->lcycommon.media_format) {
            return ;
        }

        if (lcyctrl->lcymplc) {
            audiolcy_musicplc_deinit(lcyctrl->lcymplc);
        }

        if (lcyctrl->lcyapt) {
            audiolcy_adaptive_deinit(lcyctrl->lcyapt);
        }

        memset(lcyctrl, 0, sizeof(audiolcy_ctrl_t));
        mem_free(lcyctrl);
        put_audiolcyctrl();
        SYS_LOG_INF("done");
    }
}

void audiolcy_ctrl_main(void)
{
    audiolcy_ctrl_t *lcyctrl = get_audiolcyctrl();
    if (lcyctrl == NULL) {
        return ;
    }

    // register lcy sync deal callback
    if (lcyctrl->lcysync_cb_set == 0) {
        if (tws_send_pkt_user_cb_set(audiolcy_ctrl_latency_twssync_deal) > 0) {
            lcyctrl->lcysync_cb_set = 1;
        }
    }

    if (lcyctrl->lcyapt) {
        audiolcy_adaptive_main(lcyctrl->lcyapt);
    }
}

