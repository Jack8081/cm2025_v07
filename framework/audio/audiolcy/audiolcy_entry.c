/*!
 * \file      audio_entry.c
 * \brief     latency mode global interface
 * \details
 * \author
 * \date
 * \copyright Actions
 */

#include "audiolcy_inner.h"

audiolcy_global_t  audiolcy_global = { 0, };


/*!
 * \brief: initialize latency configuration as soon as powered on
 */
u8_t audiolcy_config_init(audiolcy_cfg_t *cfg)
{
    audiolcy_global_t  *p = &audiolcy_global;

    p->lcymode_switch_hold = 0;
    memcpy(&(p->cfg), cfg, sizeof(audiolcy_cfg_t));

    if (p->cfg.Save_Latency_Mode) {
        if (property_get(PROPERTY_LATENCY_MODE, (char*)&p->cur_lcy_mode, sizeof(u8_t)) < 0) {
            p->cur_lcy_mode = p->cfg.Default_Low_Latency_Mode;
        }
    } else {
        p->cur_lcy_mode = p->cfg.Default_Low_Latency_Mode;
    }

#if ALLM_INSTEAD_OF_LLM
    if (p->cur_lcy_mode == LCYMODE_LOW && p->cfg.BM_Use_ALLM) {
        p->cur_lcy_mode = LCYMODE_ADAPTIVE;
    }
#endif

    audiolcy_set_latency_mode(p->cur_lcy_mode, 1);

    SYS_LOG_INF("%d,%d_%d_%d", p->cur_lcy_mode, p->cfg.BM_Use_ALLM, p->cfg.BM_PLC_Mode, p->cfg.BS_Latency_Optimization);
    printk("allm cfg %ds_%ds, scc c %ds_%d, scc i %dms_%d\n", p->cfg.ALLM_down_timer_period_s, p->cfg.ALLM_recommend_period_s,\
        p->cfg.ALLM_scc_clear_period_s, p->cfg.ALLM_scc_clear_step, \
        p->cfg.ALLM_scc_ignore_period_ms, p->cfg.ALLM_scc_ignore_count);

    return p->cur_lcy_mode;
}

/*!
 * \brief: get latency mode
 */
u8_t audiolcy_get_latency_mode(void)
{
    audiolcy_ctrl_t *lcyctrl = get_audiolcyctrl();
    u8_t  lcy_mode = audiolcy_global.cur_lcy_mode;

    if (lcyctrl) {
        lcy_mode = lcyctrl->lcy_mode;
    }

    return lcy_mode;
}

/*!
 * \brief: check if latency mode
 * \n 1: is (adaptive) low latency mode, 
 * \n 0: is normal latency mode
 */
u8_t audiolcy_is_low_latency_mode(void)
{
    u8_t lcy_mode = audiolcy_get_latency_mode();
    return (lcy_mode == LCYMODE_NORMAL) ? 0 : 1;
}

/*!
 * \brief: check the next lcy_mode would be switched to
 * \n return the next lcy_mode without switching
 */
u8_t audiolcy_check_latency_mode_switch_next(void)
{
    audiolcy_ctrl_t *lcyctrl = get_audiolcyctrl();
    audiolcy_global_t *lcyglobal = &audiolcy_global;

    u8_t  next_lcymode, cur_lcymode = audiolcy_get_latency_mode();

    if (lcyctrl && lcyctrl->lcymode_switch_forbidden) {
        return cur_lcymode;
    }

    // holding, switch to the 1st mode (NLM)
    if (lcyglobal->lcymode_switch_hold > 0) {
        lcyglobal->lcymode_switch_hold = 0;
        return LCYMODE_NORMAL;
    }

    next_lcymode = (cur_lcymode + 1) % LCYMODE_END;

    // check if ALLM enabled
    if (next_lcymode == LCYMODE_ADAPTIVE && !lcyglobal->cfg.BM_Use_ALLM) {
        next_lcymode = (next_lcymode + 1) % LCYMODE_END;
    }

#if ALLM_INSTEAD_OF_LLM
    // no LLM, and NLM -> ALLM
    if (next_lcymode == LCYMODE_LOW && lcyglobal->cfg.BM_Use_ALLM) {
        next_lcymode = LCYMODE_ADAPTIVE;
    }
#else
    // custom switching rules, ALLM -> LLM, and then LLM -> NLM
    if (cur_lcymode == LCYMODE_ADAPTIVE && (cur_lcymode + 1) == LCYMODE_END) {
        lcyglobal->lcymode_switch_hold = LCYMODE_ADAPTIVE;
        next_lcymode = LCYMODE_LOW;
    }
#endif

    return next_lcymode;
}

/*!
 * \brief: set latency mode, checking before setting is suggested
 */
void audiolcy_set_latency_mode(u8_t mode, u8_t checked)
{
    audiolcy_global_t *lcyglobal = &audiolcy_global;
    audiolcy_ctrl_t   *lcyctrl = get_audiolcyctrl();
    u8_t  temp_lcy_mode;

    if (lcyctrl && lcyctrl->lcymode_switch_forbidden) {
        return ;
    }

    if (checked == 0) {
        temp_lcy_mode = audiolcy_check_latency_mode_switch_next();
        if (mode != temp_lcy_mode) {
            printk("lcy check error %d_%d\n", mode, temp_lcy_mode);
            mode = temp_lcy_mode;
        }
    }

    SYS_LOG_INF("%d -> %d", lcyglobal->cur_lcy_mode, mode);

    if (mode == lcyglobal->cur_lcy_mode) {
        return ;
    }

    lcyglobal->cur_lcy_mode = mode;

    if (lcyglobal->cfg.Save_Latency_Mode) {
        if ((property_get(PROPERTY_LATENCY_MODE, (char*)&temp_lcy_mode, sizeof(u8_t)) < 0) \
         || (temp_lcy_mode != lcyglobal->cur_lcy_mode))
        {
            property_set(PROPERTY_LATENCY_MODE, (char*)&lcyglobal->cur_lcy_mode, sizeof(u8_t));
        }
    }

    // playback opened
    if (lcyctrl) {
        u16_t new_lcy_ms = 0;
        lcyctrl->lcy_mode = lcyglobal->cur_lcy_mode;
        // 1.  indicate playback, drop or insert data
        new_lcy_ms = audiolcy_ctrl_latency_change(0);
        audiolcy_ctrl_drop_insert_data(new_lcy_ms);

        // 2. NLM, deint music plc, (A)LLM, init music plc
        if (lcyctrl->lcymplc && lcyctrl->lcy_mode == LCYMODE_NORMAL) {
            audiolcy_musicplc_deinit(lcyctrl->lcymplc);
            lcyctrl->lcymplc = NULL;
        } else if (lcyctrl->lcymplc == NULL && lcyctrl->lcy_mode != LCYMODE_NORMAL) {
            lcyctrl->lcymplc = audiolcy_musicplc_init(&(lcyctrl->lcycommon), lcyctrl->dsp_handle);
        }
    }
}


u32_t audiolcy_get_latency_threshold(u8_t format)
{
    audiolcy_ctrl_t *lcyctrl = get_audiolcyctrl();
    u8_t  lcymode = audiolcy_global.cur_lcy_mode;

    if (format == 0 && lcyctrl) {
        format = lcyctrl->lcycommon.media_format;
    }

    if (lcyctrl) {
        lcymode = lcyctrl->lcy_mode;
    }

    // ALLM, use adaptive lcy
    if (lcyctrl && lcyctrl->lcyapt && lcymode == LCYMODE_ADAPTIVE) {
        return audiolcyapt_latency_get(lcyctrl->lcyapt) * 1000; //ms -> us
    } else {
        return audio_policy_get_increase_threshold(lcymode, format);
    }
}

u32_t audiolcy_get_latency_threshold_min(u8_t format)
{
    audiolcy_ctrl_t *lcyctrl = get_audiolcyctrl();
    u8_t  lcymode = audiolcy_global.cur_lcy_mode;

    if (format == 0 && lcyctrl) {
        format = lcyctrl->lcycommon.media_format;
    }

    if (lcyctrl) {
        lcymode = lcyctrl->lcy_mode;
    }

    return audio_policy_get_reduce_threshold(lcymode, format);
}

void audiolcy_cmd_exit(u8_t stream_type, u8_t media_format)
{
    u8_t param[2];

    param[0] = stream_type;
    param[1] = media_format;
    audiolcy_cmd_entry(LCYCMD_DEINIT, (void *)param, sizeof(param));
}

void audiolcy_cmd_entry(u8_t cmd, void *param, u32_t size)
{
    switch (cmd) {
    case LCYCMD_INIT: {
        audiolcy_ctrl_init(param);
        break;
    }

    case LCYCMD_DEINIT: {
        u8_t *exit_param = (u8_t *)param;
        audiolcy_ctrl_deinit(exit_param[0], exit_param[1]);
        break;
    }

    case LCYCMD_SET_TWSROLE: {
        u32_t role = (u32_t)param;
        audiolcy_ctrl_set_twsrole(role);
        break;
    }

    case LCYCMD_SET_PKTINFO: {
        audiolcy_ctrl_set_pktinfo(param);
        break;
    }

    case LCYCMD_SET_START: {
        audiolcy_ctrl_set_start();
        break;
    }

    case LCYCMD_GET_FIXED: {
        audiolcy_ctrl_get_fixedcost(param);
        break;
    }

    case LCYCMD_APT_INVALID: {
        audiolcyapt_global_set_invalid();
        break;
    }

    case LCYCMD_APT_SYNCLCY: {
        u16_t aptlcy;
        if (audiolcy_get_latency_mode() == LCYMODE_ADAPTIVE) {
            aptlcy = audiolcy_get_latency_threshold(0) / 1000; //us -> ms
            audiolcy_ctrl_latency_twssync(LCYSYNC_UPDATE, aptlcy);
        }
        break;
    }

    case LCYCMD_MAIN: {
        audiolcy_ctrl_main();
        break;
    }

    default : {
        break;
    }
    }
}


