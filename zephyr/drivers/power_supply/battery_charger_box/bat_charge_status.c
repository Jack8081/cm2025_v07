/*
 * Copyright (c) 2021  Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions SoC battery and charger status
 */

#include "bat_charge_private.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(bat_sts, CONFIG_LOG_DEFAULT_LEVEL);

/* period of detection when low power or pre-charge stage */
#define BAT_PRECHARGE_CHECK_PERIOD_SEC  60

/* timeout to check the battery is not existed */
#define BAT_NOT_EXIST_CHECK_TIME_SEC  300

/* sample the real voltage by PMU BATADC */
void bat_check_real_voltage(uint32_t sample_sec)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if (bat_charge->bat_charge_state == BAT_CHG_STATE_PRECHARGE ||
        bat_charge->bat_charge_state == BAT_CHG_STATE_CHARGE) {
        uint8_t  stop_mode = bat_charge->configs.cfg_charge.Charge_Stop_Mode;

		/**
		* According to the threshold of stop voltage to indicate the full status of battery.
		* It's better to stop charger function when sample battery voltage for accuracy.
		*/
        if (stop_mode == CHARGE_STOP_BY_VOLTAGE ||
            stop_mode == CHARGE_STOP_BY_VOLTAGE_AND_CURRENT) {

			/* enable DC5V pulldown to prevent from external charger box enter standby */
            dc5v_pull_down_enable(DC5VPD_CURRENT_15_MA,
				sample_sec * 1000 + BAT_CHARGE_DRIVER_TIMER_MS * 2);

			/* disable charger function */
            bat_charge_ctrl_disable();
        }
    } else {
        /* only disable charger function is enough */
        bat_charge_ctrl_disable();
    }

    /* update the wait time counter */
    bat_charge->bat_volt_check_wait_count =
        sample_sec * 1000 / BAT_CHARGE_DRIVER_TIMER_MS;

    bat_charge->need_check_bat_real_volt = 1;
}

void bat_charge_state_init(int bat_volt)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if (bat_volt < 0) {
        return;
    }

    /* check if battery is low power by configuration */
    if (bat_volt <= bat_charge->configs.cfg_charge.Precharge_Stop_Voltage)
    {
        LOG_DBG("low power cur_vol:%d conf_vol:%d",
				bat_volt, bat_charge->configs.cfg_charge.Precharge_Stop_Voltage);

        bat_charge->bat_charge_state = BAT_CHG_STATE_LOW;
    } else if (bat_volt >= bat_charge->configs.cfg_charge.Charge_Stop_Voltage) { /* check battery is full */
		LOG_DBG("full power cur_vol:%d conf_vol:%d",
				bat_volt, bat_charge->configs.cfg_charge.Charge_Stop_Voltage);

		/* Once DC5V plug in will start to charge */
        if (bat_charge->configs.cfg_features & SYS_FORCE_CHARGE_WHEN_DC5V_IN) {
           /* battery in normal state indicates will start to charge */
            bat_charge->bat_charge_state = BAT_CHG_STATE_NORMAL;
        } else {
            bat_charge->bat_charge_state = BAT_CHG_STATE_FULL;
        }
    } else {
        LOG_DBG("normal state cur_vol:%d", bat_volt);
        bat_charge->bat_charge_state = BAT_CHG_STATE_NORMAL;
    }
}

void bat_charge_reset_adjust_cv_offset(void)
{
    bat_charge_context_t*  bat_charge = bat_charge_get_context();

    bat_charge_adjust_cv_offset_t*  t = &bat_charge->adjust_cv_offset;

    memset(t, 0, sizeof(*t));
}

void bat_charge_state_start(uint32_t current_sel)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_context_ex_t *bat_charge_ex = bat_charge_get_context_ex();

    /* ajust CV_OFFSET when start to charge */
    if (!bat_charge->charge_state_started) {
        bat_charge_reset_adjust_cv_offset();
    }

    /* record the current when start */
    bat_charge_ex->charge_current_sel = current_sel;
    bat_charge_ex->charge_enable_pending = 1;

    bat_charge->need_check_bat_real_volt = 0;

    if (!bat_charge->charge_state_started) {
        bat_charge->charge_state_started = 1;
		if (bat_charge->callback)
        	bat_charge->callback(BAT_CHG_EVENT_CHARGE_START, NULL);
    }
}

void bat_charge_state_stop(void)
{
    bat_charge_context_t*  bat_charge = bat_charge_get_context();

    /* disable charge */
    bat_charge_ctrl_disable();

    /* ajust bandgap base voltage */
    //sys_ctrl_adjust_bandgap(0); /* TODO */

    if (bat_charge->charge_state_started) {
        bat_charge->charge_state_started = 0;

		if (bat_charge->callback)
	        bat_charge->callback(BAT_CHG_EVENT_CHARGE_STOP, NULL);
    }
}

void bat_charge_state_check_volt(int bat_volt)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    uint32_t  period;

    if (bat_charge->dc5v_debounce_state == DC5V_STATE_IN &&
        bat_charge->bat_charge_state != BAT_CHG_STATE_FULL) {
        return;
    }

    period = bat_charge->configs.cfg_charge.Battery_Check_Period_Sec *
        1000 / BAT_CHARGE_DRIVER_TIMER_MS;

    if (bat_charge->state_timer_count < period) {
        bat_charge->state_timer_count += 1;

        /* check battery real voltage every BAT_VOLT_CHECK_SAMPLE_SEC seconds */
        if (bat_charge->state_timer_count == period) {
            bat_check_real_voltage(BAT_VOLT_CHECK_SAMPLE_SEC);
        }
        return;
    }

    if (bat_volt < 0) {
        return;
    }

    bat_charge->state_timer_count = 0;

    if (bat_charge->bat_charge_state == BAT_CHG_STATE_FULL) {
		/* if battery in full state and turns to low power state, consider that battery is not existed */
        if (bat_volt <= bat_charge->configs.cfg_charge.Precharge_Stop_Voltage) {
            LOG_DBG("battery inexistance");

            bat_charge->bat_exist_state  = 0;
            bat_charge->bat_charge_state = BAT_CHG_STATE_LOW;
        } else if (bat_volt < bat_charge->configs.cfg_charge.Charge_Stop_Voltage) {
			/* battery full state => not full state */
            LOG_DBG("not full");
            bat_charge->bat_charge_state = BAT_CHG_STATE_NORMAL;
        }
    }
}

void bat_charge_state_low(int bat_volt)
{
    bat_charge_context_t*  bat_charge = bat_charge_get_context();

	/* DC5V plug in and start pre-charge process */
    if (bat_charge->dc5v_debounce_state == DC5V_STATE_IN) {
        LOG_DBG("start precharge");

        bat_charge_state_start(bat_charge->configs.cfg_charge.Charge_Current);
        bat_charge->bat_charge_state = BAT_CHG_STATE_PRECHARGE;
    } else {
        bat_charge_state_check_volt(bat_volt);
    }
}

void bat_charge_state_precharge(int bat_volt)
{
    bat_charge_context_t*  bat_charge = bat_charge_get_context();
    uint32_t  period;

	/* adjust CV_OFFSET when starting to charge */
    if (bat_charge->dc5v_debounce_state != DC5V_STATE_IN) {
        bat_charge_state_stop();

        bat_charge->precharge_time_sec = 0;
        bat_charge->bat_exist_state    = 1;

        bat_charge->bat_charge_state = BAT_CHG_STATE_LOW;

		/* detect battery voltage after DV5V pluged out */
        bat_check_real_voltage(BAT_VOLT_CHECK_SAMPLE_SEC);
        return;
    }

    period = BAT_PRECHARGE_CHECK_PERIOD_SEC * 1000 / BAT_CHARGE_DRIVER_TIMER_MS;

    if (bat_charge->state_timer_count < period) {
        bat_charge->state_timer_count += 1;

        /* after some time in pre-charge state and disable charger to detect real battery voltage */
        if (bat_charge->state_timer_count == period) {
            bat_charge->precharge_time_sec += BAT_PRECHARGE_CHECK_PERIOD_SEC;

            bat_check_real_voltage(BAT_VOLT_CHECK_SAMPLE_SEC);
        }
        return;
    }

    if (bat_volt < 0) {
        return;
    }

    /* check battery low power */
    if (bat_volt <= bat_charge->configs.cfg_charge.Precharge_Stop_Voltage) {

		LOG_DBG("low power");

        if (bat_charge->bat_exist_state == 1) {
            /* if time under low power is too long and consider that battery is absent  */
            if (bat_charge->precharge_time_sec >= BAT_NOT_EXIST_CHECK_TIME_SEC) {
                bat_charge->precharge_time_sec = 0;
                bat_charge->bat_exist_state    = 0;

                LOG_DBG("battery inexistence");
            }
        }

        bat_charge->bat_charge_state = BAT_CHG_STATE_LOW;
    } else {
        LOG_DBG("battery normal");;

        bat_charge->precharge_time_sec = 0;
        bat_charge->bat_exist_state    = 1;

        bat_charge->bat_charge_state = BAT_CHG_STATE_NORMAL;
    }
}

void bat_charge_state_normal(int bat_volt)
{
    bat_charge_context_t*  bat_charge = bat_charge_get_context();

    /* DC5V plug in to start charging */
    if (bat_charge->dc5v_debounce_state == DC5V_STATE_IN) {
        LOG_DBG("charge");

        bat_charge_state_start(bat_charge->configs.cfg_charge.Charge_Current);

        bat_charge->bat_charge_state = BAT_CHG_STATE_CHARGE;
        bat_charge->charge_near_full = 0;
    } else {
        bat_charge_state_check_volt(bat_volt);
    }
}

/* initialize charger current percent buffer */
void bat_charge_current_percent_buf_init(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    bat_charge_current_percent_buf_t *p = &bat_charge->charge_current_percent_buf;

    memset(p, 0, sizeof(*p));

    p->percent = -1;
}

int bat_charge_current_percent_buf_get(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    bat_charge_current_percent_buf_t *p = &bat_charge->charge_current_percent_buf;
    int  i, v;

    if (p->count < ARRAY_SIZE(p->buf)) {
        return -1;
    }

    for (i = 1; i < ARRAY_SIZE(p->buf); i++) {
        if (p->buf[i] != p->buf[0]) {
            break;
        }
    }

    /* stable value */
    if (i >= ARRAY_SIZE(p->buf)) {
        return p->buf[0];
    }

    /* not stable value */
    for (i = 0, v = 0; i < ARRAY_SIZE(p->buf); i++) {
        v += p->buf[i];
    }

    /* handle in case of jumping from 5% to 0% */
    if (v < 5 * ARRAY_SIZE(p->buf) / 2) {
        return 0;
    }

    return -1;
}

void bat_charge_current_percent_buf_put(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    bat_charge_current_percent_buf_t *p = &bat_charge->charge_current_percent_buf;
    int  percent;

    if (!bat_charge->charge_ctrl_enabled) {
        return;
    }

    p->buf[p->index] = (uint8_t)get_charge_current_percent();

    p->index += 1;

    if (p->index >= ARRAY_SIZE(p->buf)) {
        p->index = 0;
    }

    if (p->count < ARRAY_SIZE(p->buf)) {
        p->count += 1;
    }

    percent = bat_charge_current_percent_buf_get();
    if (percent < 0) {
        return;
    }

    if (p->percent != percent) {
        LOG_DBG("bat_charge_current >= %d%%\n",
            get_charge_current_percent_ex(percent));

        p->percent = percent;
    }
}

bool bat_charge_check_stop_current(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    bat_charge_current_percent_buf_t *p = &bat_charge->charge_current_percent_buf;
    bat_charge_adjust_current_t *adjust_current = &bat_charge->adjust_current;

    if (p->percent < 0) {
        return false;
    }

    if (bat_charge->configs.cfg_charge.Charge_Stop_Current ==
            CHARGE_STOP_CURRENT_20_PERCENT) {
        if (p->percent < 20) {
            return true;
        }
    } else if (bat_charge->configs.cfg_charge.Charge_Stop_Current ==
                CHARGE_STOP_CURRENT_5_PERCENT) {
        if (p->percent < 5) {
            return true;
        }
    } else {
        if (adjust_current->is_valid &&
            p->percent < adjust_current->stop_current_percent) {
            return true;
        }
    }

    return false;
}

/* CV state initialization */
void bat_charge_cv_state_buf_init(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    bat_charge_cv_state_buf_t *p = &bat_charge->charge_cv_state_buf;

    memset(p, 0, sizeof(*p));
}

/**
 * CV stage 1: constant voltage 1 (current less than 50%)
 * CV stage 2: constant voltage 2 (current less than 20%)
 */
int bat_charge_cv_state_buf_get(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    bat_charge_cv_state_buf_t *p = &bat_charge->charge_cv_state_buf;
    int i, v;

    if (p->count < ARRAY_SIZE(p->buf)) {
        return -1;
    }

    for (i = 0, v = 0; i < ARRAY_SIZE(p->buf); i++) {
        if (p->buf[i] == 0) {
            return 0;
        }

        v += p->buf[i];
    }

    if (v == 2 * ARRAY_SIZE(p->buf)) {
        return 2;
    }

    return 1;
}

void bat_charge_cv_state_buf_put(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    bat_charge_cv_state_buf_t *p = &bat_charge->charge_cv_state_buf;

    if (!bat_charge->charge_ctrl_enabled) {
        return;
    }

    p->buf[p->index] = (uint8_t)bat_charge_get_cv_state();

    p->index += 1;

    if (p->index >= ARRAY_SIZE(p->buf)) {
        p->index = 0;
    }

    if (p->count < ARRAY_SIZE(p->buf)) {
        p->count += 1;
    }
}

void bat_charge_check_cv_state(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    bat_charge_adjust_cv_offset_t *t = &bat_charge->adjust_cv_offset;

    bat_charge_adjust_current_t *adjust_current = &bat_charge->adjust_current;

    int  cv_stage;

    /* Do not check CV state if battery near power full */
    if (bat_charge->charge_near_full) {
        return;
    }

    bat_charge_cv_state_buf_put();

    cv_stage = bat_charge_cv_state_buf_get();

    /* CV stage1: constant voltage stage 1 (current less than 50%) */
    if (cv_stage == 1 &&
        adjust_current->is_valid == 0) {
        bat_charge_cv_state_buf_init();

        if (bat_charge->cv_stage_1_begin_time == 0) {
            LOG_DBG("cv_stage:%d", cv_stage);
            bat_charge->cv_stage_1_begin_time = k_uptime_get_32();
        } else {
			/* check charger current less than 80mA from configuration */
            if (bat_charge->configs.cfg_charge.Charge_Current <=
                    CHARGE_CURRENT_80_MA) {
             	/* if time in constant voltage stage 1 larger than 1 hour will enter stage 2 */
                if (bat_get_diff_time(k_uptime_get_32(),
                        bat_charge->cv_stage_1_begin_time) >
                        60 * 60 * 1000) {
                    cv_stage = 2;
                }
            }
        }
    }

    /* CV stage 2: constant voltage 2 (current less than 20%) */
    if (cv_stage == 2) {
        bat_charge_cv_state_buf_init();

        if (!t->adjust_end) {
            bat_charge_current_percent_buf_init();

            bat_charge_adjust_cv_offset();
            return ;
        }

        if (bat_charge->cv_stage_2_begin_time == 0) {
            LOG_DBG("cv stage:%d", cv_stage);
            bat_charge->cv_stage_2_begin_time = k_uptime_get_32();

            /* enable DC5V pulldown to prevent from charger box enter standby */
            dc5v_prevent_charger_box_standby(true);
        }

        bat_charge_adjust_current();
	}
}

void bat_charge_state_charge(int bat_volt)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_context_ex_t *bat_charge_ex = bat_charge_get_context_ex();
    uint32_t period, stop_mode;
    bool stop_charge;

    bat_charge_adjust_cv_offset_t *t = &bat_charge->adjust_cv_offset;

    bat_charge_adjust_current_t *adjust_current = &bat_charge->adjust_current;

    /* DC5V plug out and stop charging */
    if (bat_charge->dc5v_debounce_state != DC5V_STATE_IN)
    {
        bat_charge_state_stop();

        bat_charge->bat_charge_state = BAT_CHG_STATE_NORMAL;

        /* enable DC5V pulldown to prevent charger box enter standby */
        if (bat_charge->cv_stage_2_begin_time != 0) {
            dc5v_prevent_charger_box_standby(false);
        }

        bat_charge->cv_stage_1_begin_time = 0;
        bat_charge->cv_stage_2_begin_time = 0;

        adjust_current->is_valid = 0;

		/* detect battery voltage when DC5V plug out  */
        bat_check_real_voltage(BAT_VOLT_CHECK_SAMPLE_SEC);
        return;
    }

    if (bat_charge->charge_near_full) {
        period = bat_charge->configs.cfg_charge.Charge_Full_Continue_Sec *
            		1000 / BAT_CHARGE_DRIVER_TIMER_MS;
    } else {
        period = bat_charge->configs.cfg_charge.Charge_Check_Period_Sec *
            		1000 / BAT_CHARGE_DRIVER_TIMER_MS;
    }

    if (bat_charge->state_timer_count < period) {
        bat_charge->state_timer_count += 1;

        /* check interval 1s  */
        if ((bat_charge->state_timer_count %
                (1000 / BAT_CHARGE_DRIVER_TIMER_MS)) == 0) {
            if (get_dc5v_current_state() != DC5V_STATE_PENDING) {
                bat_charge_current_percent_buf_put();
                bat_charge_check_cv_state();
            }
        }

		/* check the real voltage after charging for a while */
        if (bat_charge->state_timer_count == period) {
            bat_check_real_voltage(BAT_VOLT_CHECK_SAMPLE_SEC);
        }
        return;
    }

    if (bat_volt < 0) {
        return;
    }

    if (bat_charge->charge_near_full) {
        goto charge_full;
    }

    stop_mode = bat_charge->configs.cfg_charge.Charge_Stop_Mode;
    stop_charge = 0;

	/* limit current percent 0 */
    if (bat_charge_ex->limit_current_percent == 0) {
        stop_charge = 0;
    } else if (!t->adjust_end) {
		/* CV_OFFSET adjustment does not complete */
        stop_charge = 0;
    } else if (bat_charge->cv_stage_2_begin_time == 0) {
        stop_charge = 0;
    } else if (bat_get_diff_time(k_uptime_get_32(),
                bat_charge->cv_stage_2_begin_time) >
                60 * 60 * 1000) {
		/* in constant voltage stage2 (current level less than 20%)
		 * and moreover take at least 1 hour to stop charge.
		 */
        stop_charge = 1;
    } else if (stop_mode == CHARGE_STOP_BY_VOLTAGE) {
        if (bat_volt >= bat_charge->configs.cfg_charge.Charge_Stop_Voltage) {
            stop_charge = 1;
        }
    } else if (stop_mode == CHARGE_STOP_BY_CURRENT) {
        if (bat_charge_check_stop_current() == 1) {
            stop_charge = 1;
        }
    } else if (stop_mode == CHARGE_STOP_BY_VOLTAGE_AND_CURRENT) {
        if (bat_volt >= bat_charge->configs.cfg_charge.Charge_Stop_Voltage) {
            if (bat_charge_check_stop_current() == 1) {
                stop_charge = 1;
            }
        }
    }

    /* continue to charge if battery power is not full */
    if (!stop_charge) {
        if (adjust_current->is_valid) {
            bat_charge_state_start(adjust_current->charge_current);
        } else {
            bat_charge_state_start(bat_charge->configs.cfg_charge.Charge_Current);
        }

        bat_charge->state_timer_count = 0;
        bat_charge->charge_near_full  = 0;
        return;
    }

    if (!bat_charge->charge_near_full) {
        /* continue charging when near battery full */
        LOG_DBG("near full");

        if (adjust_current->is_valid) {
            bat_charge_state_start(adjust_current->charge_current);
        } else {
            bat_charge_state_start(bat_charge->configs.cfg_charge.Charge_Current);
        }

        bat_charge->state_timer_count = 0;
        bat_charge->charge_near_full  = true;
        return;
    }

charge_full:

    /* continue charging to make sure battery power full */
    LOG_DBG("charge full");

    /* disable DC5V pulldown */
    dc5v_prevent_charger_box_standby(false);

    /* disable charger */
    bat_charge_ctrl_disable();

    //sys_ctrl_adjust_bandgap(0); /* TODO */

	if (bat_charge->callback)
    	bat_charge->callback(BAT_CHG_EVENT_CHARGE_FULL, NULL);

    bat_charge->bat_full_dc5v_last_state = bat_charge->dc5v_debounce_state;

    bat_charge->bat_charge_state = BAT_CHG_STATE_FULL;

    bat_charge->cv_stage_1_begin_time = 0;
    bat_charge->cv_stage_2_begin_time = 0;

    adjust_current->is_valid = 0;
}

void bat_charge_state_full(int bat_volt)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if (bat_charge->bat_full_dc5v_last_state != bat_charge->dc5v_debounce_state) {
        if (bat_charge->dc5v_debounce_state == DC5V_STATE_IN) {
			/* DC5V plug in when battery power full */
			if (bat_charge->callback)
            	bat_charge->callback(BAT_CHG_EVENT_BATTERY_FULL, NULL);
        } else if (bat_charge->bat_full_dc5v_last_state == DC5V_STATE_IN) {
			/* DC5V plug out when battery power full */
			if (bat_charge->callback)
            	bat_charge->callback(BAT_CHG_EVENT_CHARGE_STOP, NULL);

            if (bat_charge->configs.cfg_features & SYS_FORCE_CHARGE_WHEN_DC5V_IN) {
				/* DC5V plug in when battery under normal state to enable charger */
                bat_charge->bat_charge_state = BAT_CHG_STATE_NORMAL;
            }
        }

        bat_charge->bat_full_dc5v_last_state = bat_charge->dc5v_debounce_state;
        bat_charge->state_timer_count = 0;
    } else {
        bat_charge_state_check_volt(bat_volt);
    }
}

static int bat_report_debounce(uint32_t last_val, uint32_t cur_val, struct report_deb_ctr *ctr)
{
	if ((cur_val  > last_val) && (cur_val - last_val >= ctr->step)) {
		ctr->rise++;
		ctr->decline = 0;
		LOG_DBG("RISE(%s) %d", cur_val>100 ? "VOL" : "CAP", ctr->rise);
	} else if ((cur_val  < last_val) && (last_val - cur_val >= ctr->step)) {
		ctr->decline++;
		ctr->rise = 0;
		LOG_DBG("DECLINE(%s) %d", cur_val>100 ? "VOL" : "CAP", ctr->decline);
	}

	if (ctr->decline >= ctr->times) {
		LOG_DBG("<DECLINE> %d>%d", last_val, cur_val);
		ctr->decline = 0;
		return 1;
	} else if (ctr->rise >= ctr->times) {
		LOG_DBG("<RISE> %d>%d", last_val, cur_val);
		ctr->rise = 0;
		return 1;
	} else {
		return 0;
	}
}

void bat_charge_report(uint32_t cur_voltage, uint8_t cur_cap)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	struct device *dev = bat_charge->dev;
	struct acts_battery_info *bat = dev->data;
	const struct acts_battery_config *cfg = dev->config;
	bat_charge_event_para_t para = {0};

	bat_charge->report_ctr[INDEX_CAP].times = 600;
	bat_charge->report_ctr[INDEX_CAP].step = 5; /* % */

	if (cur_voltage <= 3400) {
		bat_charge->report_ctr[INDEX_VOL].times = 50;
		bat_charge->report_ctr[INDEX_VOL].step = BAT_VOL_LSB_MV*3; /* mv */
	} else if (cur_voltage <= 3700) {
		bat_charge->report_ctr[INDEX_VOL].step = BAT_VOL_LSB_MV*5; /* mv */
		bat_charge->report_ctr[INDEX_VOL].times = 150;
	} else {
		bat_charge->report_ctr[INDEX_VOL].step = BAT_VOL_LSB_MV*6; /* mv */
		bat_charge->report_ctr[INDEX_VOL].times = 350;
	}

	if (bat_report_debounce(bat_charge->last_voltage_report, cur_voltage, &bat_charge->report_ctr[INDEX_VOL])
		|| (bat_charge->last_voltage_report == BAT_VOLTAGE_RESERVE)) {

		para.voltage_val = cur_voltage;

		if (bat_charge->callback)
        	bat_charge->callback(BAT_CHG_EVENT_VOLTAGE_CHANGE, &para);

		bat_charge->last_voltage_report = cur_voltage;

		LOG_DBG("current voltage: %d", cur_voltage);
	}

	if (bat_report_debounce(bat_charge->last_cap_report, cur_cap, &bat_charge->report_ctr[INDEX_CAP])
		|| (bat_charge->last_cap_report == BAT_CAP_RESERVE)) {
		para.cap = cur_cap;

		if (bat_charge->callback)
        	bat_charge->callback(BAT_CHG_EVENT_CAP_CHANGE, &para);

		bat_charge->last_cap_report = cur_cap;

		LOG_DBG("current cap: %d", cur_cap);
	}

	if (cfg->debug_interval_sec) {
		if ((k_uptime_get_32() - bat->timestamp) > (cfg->debug_interval_sec * 1000)) {
			printk("** battery voltage:%dmV capacity:%d%% **\n", cur_voltage, cur_cap);
			bat->timestamp = k_uptime_get_32();
		}
	}
}

/* battery and charger status process */
void bat_charge_status_proc(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    int last_state = bat_charge->bat_charge_state;
    int bat_volt = -1;

    /* check the battery real voltage */
    if (bat_charge->need_check_bat_real_volt) {
        if (bat_charge->bat_volt_check_wait_count > 0) {
            bat_charge->bat_volt_check_wait_count -= 1;
        }

		/* wait some time after disable charger */
        if (bat_charge->bat_volt_check_wait_count == 0) {
			/* within a short time after disable charger, get the last 2 seconds sample voltage */
            bat_volt = get_battery_voltage_by_time(2);

            bat_charge->need_check_bat_real_volt = 0;

            /* check voltage change */
            bat_volt = bat_check_voltage_change(bat_volt);

			/* In case of battery voltage or capacity percent changed to notify user */
			bat_charge_report(bat_volt, get_battery_percent());
        }
    }

    switch (bat_charge->bat_charge_state)
    {
        case BAT_CHG_STATE_INIT:
        {
            bat_charge_state_init(bat_volt);
            break;
        }

        case BAT_CHG_STATE_LOW:
        {
            bat_charge_state_low(bat_volt);
            break;
        }

        case BAT_CHG_STATE_PRECHARGE:
        {
            bat_charge_state_precharge(bat_volt);
            break;
        }

        case BAT_CHG_STATE_NORMAL:
        {
            bat_charge_state_normal(bat_volt);
            break;
        }

        case BAT_CHG_STATE_CHARGE:
        {
            bat_charge_state_charge(bat_volt);
            break;
        }

        case BAT_CHG_STATE_FULL:
        {
            bat_charge_state_full(bat_volt);
            break;
        }
    }

    if (bat_charge->bat_charge_state != last_state) {
        bat_charge->state_timer_count = 0;
    }

    bat_charge_check_enable();
}

