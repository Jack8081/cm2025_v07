/*
 * Copyright (c) 2021  Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions SoC battery and charger control interface
 */

#include <stdlib.h>
#include "bat_charge_private.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(bat_ctl, CONFIG_LOG_DEFAULT_LEVEL);

/* PMU charger control register update */
static inline void pmu_chg_ctl_reg_write(uint32_t mask, uint32_t value)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
    bat_charge->bak_PMU_CHG_CTL = (bat_charge->bak_PMU_CHG_CTL & ~mask) | (value & mask);
	LOG_DBG("bak_PMU_CHG_CTL:0x%x", bat_charge->bak_PMU_CHG_CTL);
	sys_write32(bat_charge->bak_PMU_CHG_CTL, CHG_CTL_SVCC);
}

/* DC5V pulldown operation */
static void dc5v_pull_down_disable(struct k_work *work)
{
	LOG_DBG("DC5V pulldown disable");

	/* disable DC5V pulldown and set the minimal pulldown current */
    pmu_chg_ctl_reg_write((1 << CHG_CTL_SVCC_DC5VPD_EN) | CHG_CTL_SVCC_DC5VPD_SET_MASK,
        (0 << CHG_CTL_SVCC_DC5VPD_EN) | (0 << CHG_CTL_SVCC_DC5VPD_SET_SHIFT)
    );
}

/* enable DC5V pulldown and will last specified time in miliseconds to disable */
void dc5v_pull_down_enable(uint32_t current, uint32_t timer_ms)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    struct k_delayed_work *timer = &bat_charge->dc5v_pd_timer;

	/* When DC5V has already enable, cancel the delay work and reschedule new delay work */
	if (bat_charge->bak_PMU_CHG_CTL & (1 << CHG_CTL_SVCC_DC5VPD_EN)) {
		k_delayed_work_cancel(timer);
	}

	LOG_DBG("DC5V pulldown current:%d time:%dms", current, timer_ms);

    pmu_chg_ctl_reg_write((1 << CHG_CTL_SVCC_DC5VPD_EN) | CHG_CTL_SVCC_DC5VPD_SET_MASK,
        (1 << CHG_CTL_SVCC_DC5VPD_EN) | (current << CHG_CTL_SVCC_DC5VPD_SET_SHIFT));

	/* sumit a delay work to disable DC5V pull down */
	if ((int)timer_ms > 0)
		k_delayed_work_submit(timer, K_MSEC(timer_ms));
}

/* cancel DC5V state timer */
void dc5v_state_timer_delete(void)
{
    bat_charge_context_t*  bat_charge = bat_charge_get_context();

	k_delayed_work_cancel(&bat_charge->dc5v_check_status.state_timer);
}

/* get DC5V level by specified millivolt  */
uint32_t dc5v_get_low_level(uint32_t mv)
{
    if (mv >= 3100) {
        return DC5VLV_LEVEL_3_0_V;
    } else if (mv >= 2100) {
        return DC5VLV_LEVEL_2_0_V;
    } else if (mv >= 1100) {
        return DC5VLV_LEVEL_1_0_V;
    }

    return DC5VLV_LEVEL_0_2_V;
}

/* DC5V status check initialization */
void dc5v_check_status_init(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    dc5v_check_status_t *p = &bat_charge->dc5v_check_status;

    p->last_state = DC5V_STATE_INVALID;
}

/* DC5V pulldown to prevent charger box enter standby */
void dc5v_prevent_charger_box_standby(bool pull_down)
{
	bat_charge_context_ex_t *bt_charge_ex = bat_charge_get_context_ex();
    uint8_t standby_current =
        bt_charge_ex->cfg_ex.Charger_Box_Standby_Current;

	LOG_DBG("pull_down:%d standby_current:%d", pull_down, standby_current);

    if (pull_down) {
        uint8_t  dc5v_pd_current = DC5VPD_CURRENT_DISABLE;

        if (standby_current >= 7) {
            dc5v_pd_current = DC5VPD_CURRENT_15_MA;
        }
        else if (standby_current >= 4) {
            dc5v_pd_current = DC5VPD_CURRENT_7_5_MA;
        }
        else if (standby_current >= 1) {
            dc5v_pd_current = DC5VPD_CURRENT_2_5_MA;
        }

        if (dc5v_pd_current != DC5VPD_CURRENT_DISABLE) {
            dc5v_pull_down_enable(dc5v_pd_current, (uint32_t)-1);
        }
    } else {
        if (standby_current >= 1) {
			/* DC5V pull down disable */
			pmu_chg_ctl_reg_write((1 << CHG_CTL_SVCC_DC5VPD_EN) | CHG_CTL_SVCC_DC5VPD_SET_MASK,
				(0 << CHG_CTL_SVCC_DC5VPD_EN) | (0 << CHG_CTL_SVCC_DC5VPD_SET_SHIFT));
        }
    }
}

/* wakeup charger box */
dc5v_state_t dc5v_wake_charger_box(void)
{
    dc5v_state_t state = get_dc5v_current_state();
    CFG_Struct_Charger_Box *cfg = &(bat_charge_get_configs()->cfg_charger_box);
    uint32_t t;

    /* DC5V state under handling */
    if (state == DC5V_STATE_PENDING) {
        t = k_uptime_get_32();

        while (state == DC5V_STATE_PENDING) {
            k_sleep(K_MSEC(1));

            state = get_dc5v_current_state();

            /* maybe in DC5V UART communication ? */
            if (bat_get_diff_time(k_uptime_get_32(), t) > 1000) {
                return state;
            }
        }
    }

    if (!cfg->Enable_Charger_Box) {
        return state;
    }

    /* forbidden DC5V pulldown to wake up */
    if (cfg->DC5V_Pull_Down_Current == DC5VPD_CURRENT_DISABLE) {
        return state;
    }

    /* enable DC5V pulldown */
    dc5v_pull_down_enable (
        cfg->DC5V_Pull_Down_Current,
        cfg->DC5V_Pull_Down_Hold_Ms + 10
    );

    t = k_uptime_get_32();

    while (1) {
        uint32_t time_ms = bat_get_diff_time(k_uptime_get_32(), t);

        state = get_dc5v_current_state();

        /* already detected DC5V plug-in */
        if (state == DC5V_STATE_IN) {
            break;
        }

        /* wake up timeout */
        if (time_ms >= cfg->DC5V_Pull_Down_Hold_Ms +
                cfg->Charger_Wake_Delay_Ms + 50) {
            break;
        }

        k_sleep(K_MSEC(1));
    }

    return state;
}

static void dc5v_state_timer_handler(struct k_work *work)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    dc5v_check_status_t *p = &bat_charge->dc5v_check_status;
    CFG_Struct_Charger_Box *cfg = &(bat_charge_get_configs()->cfg_charger_box);
    bool pending = false;

    /* check charger box standby delay time from configuration */
    if (cfg->Enable_Charger_Box) {
        if (bat_charge->dc5v_state_pending_time
			< cfg->Charger_Standby_Delay_Ms) {
            pending = true;
        }
    }

	/* TODO */
#if 0
    /* check DC5V UART is under communication */
    if (dc5v_uart_operate(DC5V_UART_CHECK_IO_TIME, NULL, 500, 0)) {
        pending = true;
    }
#endif

	/* set the flag for the discharge detect timer (dc5v_state_timer_handler_ex) to run */
	if (pending) {
		bat_charge->dc5v_state_pending_time += 20;
		bat_charge->dc5v_state_exit = 0;
		bat_charge->dc5v_state_pending = 1;
	} else {
		bat_charge->dc5v_state_pending_time = 0;
		bat_charge->dc5v_state_exit = 1;
		bat_charge->dc5v_state_pending = 0;
	}

	/* sumit a delay work to check DC5V state */
	if (pending)
		k_delayed_work_submit(&p->state_timer, K_MSEC(20));
}

void dc5v_state_timer_handler_ex(struct k_work *work)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    dc5v_check_status_t *p = &bat_charge->dc5v_check_status;
	bool pending = false;

    /* check if DC5V UART can be created communication */
#if 0 /* TODO */
    if (dc5v_uart_operate(DC5V_UART_CHECK_IO_TIME, NULL, 500, 0)) {
#else
	if (1) {
#endif
		pending = true;
    }

	if (pending) {
		bat_charge->dc5v_state_ex_exit = 0;
		bat_charge->dc5v_state_ex_pending = 1;
		k_delayed_work_submit(&p->state_timer_ex, K_MSEC(20));
	} else {
		bat_charge->dc5v_state_ex_exit = 1;
		bat_charge->dc5v_state_ex_pending = 0;
	}
}

void charger_enable_timer_delete(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

	k_delayed_work_cancel(&bat_charge->dc5v_check_status.charger_enable_timer);
}

static void charger_enable_timer_handler(struct k_work *work)
{
    charger_enable_timer_delete();

    pmu_chg_ctl_reg_write((1 << CHG_CTL_SVCC_CHG_EN),
			(1 << CHG_CTL_SVCC_CHG_EN));

    LOG_DBG("charger enabled");
}

/* convert from DC5V ADC value to voltage in millivolt */
uint32_t dc5v_adc_get_voltage_mv(uint32_t adc_val)
{
    uint32_t mv;

    if (!adc_val)
        return 0;

    mv = (adc_val + 1) * 12000 / 4096;

    return mv;
}

/* convert from battery ADC value to voltage in millivolt */
uint32_t bat_adc_get_voltage_mv(uint32_t adc_val)
{
    uint32_t mv;

    if (!adc_val)
        return 0;

    mv = (adc_val + 1) * 4800 / 4096;

    return mv;
}

/* get the chargei converts to current in milliampere */
uint32_t chargei_adc_get_current_ma(int adc_val)
{
    uint32_t mA;

    if (adc_val <= 0) {
        return 0;
    }

    mA = (adc_val + 1) >> 3;

    return mA;
}

/* get the current DC5V state */
dc5v_state_t get_dc5v_current_state(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_context_ex_t *bt_charge_ex = bat_charge_get_context_ex();
    uint8_t state = DC5V_STATE_OUT;
    uint32_t dc5v_mv, bat_mv;
    dc5v_check_status_t *p = &bat_charge->dc5v_check_status;
    CFG_Struct_Charger_Box *cfg = &(bat_charge_get_configs()->cfg_charger_box);
	int ret;

	ret = dc5v_adc_get_sample();
	if (ret < 0)
		return DC5V_STATE_INVALID;

    dc5v_mv = dc5v_adc_get_voltage_mv(ret);

	ret = bat_adc_get_sample();
	if (ret < 0)
		return DC5V_STATE_INVALID;

    bat_mv  = bat_adc_get_voltage_mv(ret);

    if (cfg->Enable_Charger_Box &&
        cfg->Charger_Standby_Voltage >= 3800) {
        uint32_t volt = bat_charge->configs.cfg_charge.Charge_Voltage;

		volt = (4200 * 10 + (volt - CHARGE_VOLTAGE_4_20_V) * 125) / 10;

        if (dc5v_mv >= (volt + 100)) {
            state = DC5V_STATE_IN;
        }
    } else {
        if (soc_pmu_get_dc5v_status() && dc5v_mv >= 3800) {
            state = DC5V_STATE_IN;
        }
    }

    if (state == DC5V_STATE_IN) {
        if (p->last_state != DC5V_STATE_IN) {
            /* Once DC5V plug in, delay severial milliseconds to enable charger */
            if (!(bat_charge->bak_PMU_CHG_CTL & (1 << CHG_CTL_SVCC_CHG_EN))) {
				/* delay 500ms to enable charger */
				k_delayed_work_submit(&p->charger_enable_timer, K_MSEC(500));
            }

			/* TODO */
            //dc5v_uart_operate(DC5V_UART_SET_ENABLE, NULL, ENABLE, 0);

            dc5v_state_timer_delete();
        }
    } else {
        if (p->last_state == DC5V_STATE_IN ||
            p->last_state == DC5V_STATE_INVALID) {

			/* disable charger enable timer */
            charger_enable_timer_delete();

			/* disable charger anyway */
            if (bat_charge->bak_PMU_CHG_CTL & (1 << CHG_CTL_SVCC_CHG_EN)) {
                pmu_chg_ctl_reg_write((1 << CHG_CTL_SVCC_CHG_EN),
					(0 << CHG_CTL_SVCC_CHG_EN));
            }
        }

        if (p->last_state == DC5V_STATE_IN) {
            bool pending = false;

         	/* If DC5V drop down from high voltage, it is necessary to pulldown DC5V for quick power down */
            if (bat_charge->configs.cfg_features & SYS_ENABLE_DC5VPD_WHEN_DETECT_OUT) {
                dc5v_pull_down_enable(0, 20);
                pending = true;
            }

            /* TODO */
#if 0
            dc5v_uart_operate(DC5V_UART_SET_ENABLE, NULL, DISABLE, 500);

            if (dc5v_uart_operate(DC5V_UART_CHECK_IO_TIME, NULL, 500, 0)) {
                pending = YES;
            }
#endif
            if (cfg->Enable_Charger_Box) {
                pending = true;
            }

            if (pending) {
                dc5v_state_timer_delete();
				k_delayed_work_submit(&p->state_timer, K_MSEC(20));
                state = DC5V_STATE_PENDING;
            }
        }

        /* start to detect DC5V UART communication can be created */
        if ((bat_charge->dc5v_state_exit) && (dc5v_mv >= 2500) &&
            (bt_charge_ex->last_dc5v_mv < 2500)) {
            bool  pending = false;
#if 0
            dc5v_uart_operate(DC5V_UART_SET_ENABLE, NULL, ENABLE, 0);

			/* delay 500ms to disable DC5V UART if disconnected with charger box  */
            dc5v_uart_operate(DC5V_UART_SET_ENABLE, NULL, DISABLE, 500);

            if (dc5v_uart_operate(DC5V_UART_CHECK_IO_TIME, NULL, 500, 0)) {
                pending = true;
            }
#endif
            if (pending) {
                dc5v_state_timer_delete();
                k_delayed_work_submit(&p->state_timer_ex, K_MSEC(20));
                state = DC5V_STATE_PENDING;
            }
        }

		if ((bat_charge->dc5v_state_pending)
			|| (bat_charge->dc5v_state_ex_pending))
			state = DC5V_STATE_PENDING;

        if (state != DC5V_STATE_PENDING) {
            if (cfg->Enable_Charger_Box) {
                uint32_t volt = cfg->Charger_Standby_Voltage;

                if (volt > 3000) {
                    volt = 3000;
                }

                if (volt < 300) {
                    volt = 300;
                }

                if (dc5v_mv >= volt * 3/4) {
                    state = DC5V_STATE_STANDBY;
                } else {
                    state = DC5V_STATE_OUT;
                }
            } else {
                state = DC5V_STATE_OUT;
            }
        }
    }

    bt_charge_ex->last_dc5v_mv = dc5v_mv;

    if (p->last_state != state) {
        p->last_state = state;
        LOG_INF("dc5v:%dmv bat:%dmv dc5v_det:%d <%s>", dc5v_mv, bat_mv,
				soc_pmu_get_dc5v_status(), get_dc5v_state_str(state));
    }

    return state;
}

/* get DC5V debounce state */
extern dc5v_state_t get_dc5v_debounce_state(void)
{
    bat_charge_context_t*  bat_charge = bat_charge_get_context();

    return bat_charge->dc5v_debounce_state;
}

/* charger enable */
void bat_charge_ctrl_enable(uint32_t current_sel, uint32_t voltage_sel)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    bat_charge_adjust_cv_offset_t *t = &bat_charge->adjust_cv_offset;
    uint32_t adjust_volt = voltage_sel;

    bat_charge->charge_ctrl_enabled = 1;

    bat_charge_current_percent_buf_init();

    bat_charge_cv_state_buf_init();

	/* current limit percent is 0 to set minimal voltage 3.3V */
    if (voltage_sel < CHARGE_VOLTAGE_4_20_V) {
        adjust_volt = voltage_sel;
    } else if (t->is_valid) {
        adjust_volt = t->cv_offset;
    } else {
		/* adjust const voltage by efust  */
        LOG_DBG("before adjust_volt: 0x%x", adjust_volt);
        //sys_efuse_adjust_value(&adjust_volt, SYS_EFUSE_CV_OFFSET); /* TODO */
        LOG_DBG("after adjust_volt: 0x%x", adjust_volt);
    }

    LOG_DBG("cur:0x%x vol:0x%x adjust_vol:0x%x", current_sel, voltage_sel, adjust_volt);

    pmu_chg_ctl_reg_write(
        ((CHG_CTL_SVCC_CHG_CURRENT_MASK)|
        (CHG_CTL_SVCC_CV_OFFSET_MASK) |
        (0x1<<CHG_CTL_SVCC_CV_3V3)),
        ((current_sel << CHG_CTL_SVCC_CHG_CURRENT_SHIFT) |
        (adjust_volt << CHG_CTL_SVCC_CV_OFFSET_SHIFT) |
        (0x0<<CHG_CTL_SVCC_CV_3V3))
   );
}

void bat_charge_check_enable(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_context_ex_t *bt_charge_ex = bat_charge_get_context_ex();
    bat_charge_configs_t *cfg = bat_charge_get_configs();
    uint32_t current_sel;
    uint32_t voltage_sel;

    if (bt_charge_ex->charge_enable_pending) {
		/* check if charger is enabled */
        if (!(bat_charge->bak_PMU_CHG_CTL & (1 << CHG_CTL_SVCC_CHG_EN))) {
			LOG_DBG("charge not enabled yet");
            return ;
        }

        current_sel = bt_charge_ex->charge_current_sel;
        voltage_sel = cfg->cfg_charge.Charge_Voltage;

        /* limit max charger current */
        if (bt_charge_ex->limit_current_percent < 100) {
            int  level;
            int  ma;

            ma = bat_charge_get_current_ma(cfg->cfg_charge.Charge_Current);

            ma = ma * bt_charge_ex->limit_current_percent / 100;

            level = bat_charge_get_current_level(ma);

            if (level < cfg->cfg_charge.Charge_Current) {
				/* enter const voltage state and current less than 20%, moreover current limit precent  is 0 */
                if (bat_charge->cv_stage_2_begin_time == 0 ||
                    bt_charge_ex->limit_current_percent == 0) {
                    current_sel = level;
                }
            }

            /* if current limit percent is 0 to set the minimal charge voltage to be 3.3V */
            if (bt_charge_ex->limit_current_percent == 0) {
                voltage_sel = 0x0;
            }
        }

        bat_charge_ctrl_enable(current_sel, voltage_sel);

        bt_charge_ex->charge_enable_pending = 0;
	}
}

/* disable charger function */
void bat_charge_ctrl_disable(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_context_ex_t *bat_charge_ex = bat_charge_get_context_ex();

    bat_charge_ex->charge_enable_pending = 0;

    if (bat_charge->charge_ctrl_enabled)
        bat_charge->charge_ctrl_enabled = 0;

    /* setup the minimal charger voltage as 3.3V to stop charger function */
    pmu_chg_ctl_reg_write((0x1<<CHG_CTL_SVCC_CV_3V3), (0x1<<CHG_CTL_SVCC_CV_3V3));
}

/* configure the charge current */
void bat_charge_set_current(uint32_t current_sel)
{
	/* TODO: check whether the current setting is valid */

    pmu_chg_ctl_reg_write(CHG_CTL_SVCC_CHG_CURRENT_MASK,
        (current_sel << CHG_CTL_SVCC_CHG_CURRENT_SHIFT));
}

/* get charer current percent */
int get_charge_current_percent(void)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
    uint32_t current_sel = (bat_charge->bak_PMU_CHG_CTL & CHG_CTL_SVCC_CHG_CURRENT_MASK) >> CHG_CTL_SVCC_CHG_CURRENT_SHIFT;
	int ma1 = bat_charge_get_current_ma(current_sel);
	int ma2 = chargei_adc_get_current_ma(chargei_adc_get_sample());

	int percent = ma2 * 100 / ma1;

    return percent;
}

int get_charge_current_percent_ex(int percent)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
    bat_charge_configs_t *cfg = bat_charge_get_configs();

    uint32_t  current_sel =
        (bat_charge->bak_PMU_CHG_CTL & CHG_CTL_SVCC_CHG_CURRENT_MASK) >> CHG_CTL_SVCC_CHG_CURRENT_SHIFT;

    if (current_sel != cfg->cfg_charge.Charge_Current) {
        int ma1 = bat_charge_get_current_ma(current_sel);
        int ma2 = bat_charge_get_current_ma(cfg->cfg_charge.Charge_Current);

        percent = ma1 * percent / ma2;
    }

    return percent;
}

/* get the CV state
 * const voltage state and current less than 50% return 1
 * const voltage state and current less than 20% return 2
 */
int bat_charge_get_cv_state(void)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
    uint32_t voltage_sel =
        (bat_charge->bak_PMU_CHG_CTL & CHG_CTL_SVCC_CV_OFFSET_MASK) >> CHG_CTL_SVCC_CV_OFFSET_SHIFT;
    int percent;
    uint32_t efuse_cv_4_2V = CHARGE_VOLTAGE_4_20_V;

	/* TODO */
   // sys_efuse_adjust_value(&efuse_cv_4_2V, SYS_EFUSE_CV_OFFSET);

    /* charger disable or limit current percent is 0 */
    if (voltage_sel < efuse_cv_4_2V) {
        LOG_DBG("vol:0x%x < cv_4_2V:0x%x",voltage_sel,efuse_cv_4_2V);
        return 0;
    }

    percent = get_charge_current_percent();

    if (percent > 75) {
        return 0;
    }

    percent = get_charge_current_percent_ex(percent);

    if (percent < 30) {
        return 2;
    } else if (percent < 50) {
        return 1;
    }

    return 0;
}

/* adjust CV_OFFSET */
void bat_charge_adjust_cv_offset(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    bat_charge_adjust_cv_offset_t *t = &bat_charge->adjust_cv_offset;
    uint32_t voltage_sel = bat_charge->configs.cfg_charge.Charge_Voltage;
    int  curr_volt = (int)get_battery_voltage_by_time(BAT_VOLT_CHECK_SAMPLE_SEC);

    int  diff_volt =
         4200*10 + (voltage_sel - CHARGE_VOLTAGE_4_20_V) * 125 - curr_volt*10;

    uint32_t cv_offset =
        (bat_charge->bak_PMU_CHG_CTL & CHG_CTL_SVCC_CV_OFFSET_MASK) >> CHG_CTL_SVCC_CV_OFFSET_SHIFT;

    if (curr_volt < t->last_volt) {
        /* voltage exception */
        t->adjust_end = 1;
    }
    else if (diff_volt >= 125)
    {
        if (cv_offset < 0x1f)
        {
            cv_offset += 1;
        } else {
            t->adjust_end = 1;
        }
    } else {
        t->adjust_end = 1;
    }

    LOG_DBG("curr_volt:%d diff_volt:%d cv_offset:0x%x", curr_volt, diff_volt, cv_offset);

    if (t->adjust_end) {
        LOG_WRN("adjust_end");
    }

    pmu_chg_ctl_reg_write (
        (CHG_CTL_SVCC_CV_OFFSET_MASK),
        (cv_offset << CHG_CTL_SVCC_CV_OFFSET_SHIFT)
	);

    if (t->last_volt < curr_volt) {
        t->last_volt = curr_volt;
    }

    t->is_valid  = 1;
    t->cv_offset = cv_offset;
}

bool bat_charge_adjust_current(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    uint32_t chg_current =


	(bat_charge->bak_PMU_CHG_CTL & CHG_CTL_SVCC_CHG_CURRENT_MASK) >> CHG_CTL_SVCC_CHG_CURRENT_SHIFT;

    bat_charge_adjust_current_t *adjust_current = &bat_charge->adjust_current;

    if (bat_charge->cv_stage_2_begin_time == 0) {
        return false;
    }

    switch (bat_charge->configs.cfg_charge.Charge_Stop_Current) {
        case CHARGE_STOP_CURRENT_20_PERCENT:
        case CHARGE_STOP_CURRENT_5_PERCENT:
        {
            adjust_current->is_valid = 0;

            adjust_current->charge_current =
                bat_charge->configs.cfg_charge.Charge_Current;
            break;
        }

        case CHARGE_STOP_CURRENT_30_MA:
        {
            adjust_current->is_valid = 1;
            adjust_current->charge_current = CHARGE_CURRENT_150_MA;
            adjust_current->stop_current_percent = 20;
            break;
        }

        case CHARGE_STOP_CURRENT_20_MA:
        {
            adjust_current->is_valid = 1;
            adjust_current->charge_current = CHARGE_CURRENT_100_MA;
            adjust_current->stop_current_percent = 20;
            break;
        }

        case CHARGE_STOP_CURRENT_16_MA:
        {
            adjust_current->is_valid = 1;
            adjust_current->charge_current = CHARGE_CURRENT_80_MA;
            adjust_current->stop_current_percent = 20;
            break;
        }

        case CHARGE_STOP_CURRENT_12_MA:
        {
            adjust_current->is_valid = 1;
            adjust_current->charge_current = CHARGE_CURRENT_60_MA;
            adjust_current->stop_current_percent = 20;
            break;
        }

        case CHARGE_STOP_CURRENT_8_MA:
        {
            adjust_current->is_valid = 1;
            adjust_current->charge_current = CHARGE_CURRENT_40_MA;
            adjust_current->stop_current_percent = 20;
            break;
        }

        case CHARGE_STOP_CURRENT_6_4_MA:
        {
            adjust_current->is_valid = 1;
            adjust_current->charge_current = CHARGE_CURRENT_30_MA;
            adjust_current->stop_current_percent = 20;
            break;
        }

        case CHARGE_STOP_CURRENT_5_MA:
        {
            adjust_current->is_valid = 1;
            adjust_current->charge_current = CHARGE_CURRENT_100_MA;
            adjust_current->stop_current_percent = 5;
            break;
        }
    }

    if (chg_current != adjust_current->charge_current) {
        chg_current = adjust_current->charge_current;

        LOG_DBG("chg_current:0x%x", chg_current);

        bat_charge_current_percent_buf_init();

        pmu_chg_ctl_reg_write (
            CHG_CTL_SVCC_CHG_CURRENT_MASK,
            (chg_current << CHG_CTL_SVCC_CHG_CURRENT_SHIFT)
        );

        return true;
    }

    return false;
}

static const struct {
    uint8_t   level;
    uint16_t  ma;
} bat_charge_current_table[] = {
    { CHARGE_CURRENT_10_MA,  10  },
    { CHARGE_CURRENT_20_MA,  20  },
    { CHARGE_CURRENT_30_MA,  30  },
    { CHARGE_CURRENT_40_MA,  40  },
	{ CHARGE_CURRENT_50_MA,  50  },
    { CHARGE_CURRENT_60_MA,  60  },
    { CHARGE_CURRENT_70_MA,  70  },
    { CHARGE_CURRENT_80_MA,  80  },
    { CHARGE_CURRENT_90_MA,  90  },
    { CHARGE_CURRENT_100_MA, 100 },
    { CHARGE_CURRENT_120_MA, 120 },
    { CHARGE_CURRENT_140_MA, 140 },
    { CHARGE_CURRENT_160_MA, 160 },
    { CHARGE_CURRENT_180_MA, 180 },
    { CHARGE_CURRENT_200_MA, 200 },
    { CHARGE_CURRENT_240_MA, 240 },
};

/* get the charger current in milliampere by specified level */
int bat_charge_get_current_ma(int level)
{
    int  i;

    for (i = ARRAY_SIZE(bat_charge_current_table) - 1; i > 0; i--) {
        if (level >= bat_charge_current_table[i].level) {
            break;
        }
    }

    return bat_charge_current_table[i].ma;
}

/* get the charger current level by specified milliampere */
int bat_charge_get_current_level(int ma)
{
    int  i;

    for (i = ARRAY_SIZE(bat_charge_current_table) - 1; i > 0; i--) {
        if (ma >= bat_charge_current_table[i].ma) {
            break;
        }
    }

    if (i < ARRAY_SIZE(bat_charge_current_table) - 1) {
        int  diff1 = ma - bat_charge_current_table[i].ma;
        int  diff2 = ma - bat_charge_current_table[i+1].ma;

        if (abs(diff1) > abs(diff2))
        {
            i = i+1;
        }
    }

    return bat_charge_current_table[i].level;
}

/* limit charger current by specified percent (ranger from 0 ~ 100) */
void bat_charge_limit_current(uint32_t percent)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_context_ex_t *bat_charge_ex = bat_charge_get_context_ex();

    if (percent > 100) {
        percent = 100;
    }

    if (bat_charge_ex->limit_current_percent != percent) {
        LOG_DBG("percent:%d%%", percent);
		/* reset CV state calculate time if previous percent is 0 */
        if (bat_charge_ex->limit_current_percent == 0) {
            if (bat_charge->cv_stage_1_begin_time != 0) {
                bat_charge->cv_stage_1_begin_time = k_uptime_get_32();
            }

            if (bat_charge->cv_stage_2_begin_time != 0) {
                bat_charge->cv_stage_2_begin_time = k_uptime_get_32();
            }
        }

        bat_charge_ex->limit_current_percent = percent;

        if (bat_charge->charge_ctrl_enabled) {
            bat_charge_ex->charge_enable_pending = 1;
        }
    }
}

/* calibrate the charger internal contant current by efuse */
static int bat_charge_cc_calibration(uint8_t *cc_offset)
{
	/* TODO: impletement */
	return -ESRCH;
}

/* calibrate the current convert from charger to ADC by efuse */
static int bat_charge_chargiadc_calibration(uint8_t *chargi)
{
	/* TODO: impletement */
	return -ESRCH;
}

/* battery and charger control initialization */
void bat_charge_ctrl_init(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	uint8_t val, val_new;

	val = (bat_charge->bak_PMU_CHG_CTL & CHG_CTL_SVCC_CC_OFFSET_MASK) >> CHG_CTL_SVCC_CC_OFFSET_SHIFT;

	/* get the calibrated constant current from efuse */
	if (!bat_charge_cc_calibration(&val_new)) {
		LOG_DBG("new const current:%d", val_new);
		val = val_new;
	}

	LOG_INF("const current:%d", val);
	pmu_chg_ctl_reg_write(CHG_CTL_SVCC_CC_OFFSET_MASK, val << CHG_CTL_SVCC_CC_OFFSET_SHIFT);

	val = (sys_read32(BDG_CTL_SVCC) & BDG_CTL_SVCC_CHARGEI_SET_MASK) >> BDG_CTL_SVCC_CHARGEI_SET_SHIFT;

	/* get the calibrated charge convert value from efuse */
	if (!bat_charge_chargiadc_calibration(&val_new)) {
		LOG_DBG("new chargiadc:%d", val_new);
		val = val_new;
	}

	LOG_INF("charge adc convert ajustment:%d", val);
	sys_write32((sys_read32(BDG_CTL_SVCC) & ~BDG_CTL_SVCC_CHARGEI_SET_MASK)
				| (val << BDG_CTL_SVCC_CHARGEI_SET_SHIFT), BDG_CTL_SVCC);

	k_busy_wait(300);
	LOG_DBG("BDG_CTL_SVCC:0x%x", sys_read32(BDG_CTL_SVCC));

	/* set the charger current to avoid power supply insufficiently */
	LOG_INF("set charger const current:%d", bat_charge->configs.cfg_charge.Charge_Current);
    bat_charge_set_current(bat_charge->configs.cfg_charge.Charge_Current);

    /* check DC5V status */
    dc5v_check_status_init();

	k_delayed_work_init(&bat_charge->dc5v_pd_timer, dc5v_pull_down_disable);
	k_delayed_work_init(&bat_charge->dc5v_check_status.charger_enable_timer, charger_enable_timer_handler);
	k_delayed_work_init(&bat_charge->dc5v_check_status.state_timer, dc5v_state_timer_handler);
	k_delayed_work_init(&bat_charge->dc5v_check_status.state_timer_ex, dc5v_state_timer_handler_ex);
}

