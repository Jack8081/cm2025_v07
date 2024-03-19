/*
 * Copyright (c) 2021  Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions SoC battery and charger driver
 */

#include <stdlib.h>
#include <errno.h>
#include <kernel.h>
#include <sys/byteorder.h>
#include <drivers/power_supply.h>
#include <soc.h>
#include <board.h>

#include "bat_charge_private.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(bat_drv, CONFIG_LOG_DEFAULT_LEVEL);

#define DT_DRV_COMPAT actions_acts_batadc

#define CHG_EN_BATADC_ADJUST  0

bat_charge_context_ex_t bat_charge_context_ex;

bat_charge_context_t bat_charge_context;

static uint32_t bat_adc_get_sample_ex(void)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
    uint32_t adc_val = bat_adc_get_sample();

    /* calibrate BATADC deviation when charger enabled */
    if (bat_charge->bak_PMU_CHG_CTL & (1 << CHG_CTL_SVCC_CHG_EN)) {
        adc_val += CHG_EN_BATADC_ADJUST;
    }

    return adc_val;
}

void bat_adc_sample_buf_init(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    int  buf_size = BAT_ADC_SAMPLE_BUF_SIZE;
    int  i;

    /* check the CHG_EN status before sampling battery voltage */
    get_dc5v_current_state();

    /* sample BAT ADC data in 10ms */
    for (i = 0; i < buf_size; i++)
    {
        bat_charge->bat_adc_sample_buf[i] = bat_adc_get_sample_ex();

        k_usleep(10000 / buf_size);
    }

    bat_charge->bat_adc_sample_timer_count = 0;
    bat_charge->bat_adc_sample_index = 0;
}

void bat_adc_sample_buf_put(uint32_t bat_adc)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    int buf_size = BAT_ADC_SAMPLE_BUF_SIZE;

    bat_charge->bat_adc_sample_buf[bat_charge->bat_adc_sample_index] = bat_adc;
    bat_charge->bat_adc_sample_index += 1;

    if (bat_charge->bat_adc_sample_index >= buf_size)
    {
        bat_charge->bat_adc_sample_index = 0;
    }
}

static void bat_volt_selectsort(uint16_t *arr, uint16_t len)
{
	uint16_t i, j, min;

	for (i = 0; i < len-1; i++)	{
		min = i;
		for (j = i+1; j < len; j++) {
			if (arr[min] > arr[j])
				min = j;
		}
		/* swap */
		if (min != i) {
			arr[min] ^= arr[i];
			arr[i] ^= arr[min];
			arr[min] ^= arr[i];
		}
	}
}

/* get the average battery voltage during specified time */
uint32_t get_battery_voltage_by_time(uint32_t sec)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    int  buf_size = BAT_ADC_SAMPLE_BUF_SIZE;
    int  index = bat_charge->bat_adc_sample_index;
    int  adc_count = (sec * 1000 / BAT_ADC_SAMPLE_INTERVAL_MS);
    int  bat_adc, volt_mv, i;
    uint16_t tmp_buf[BAT_ADC_SAMPLE_BUF_SIZE] = {0};

    for (i = 0; i < adc_count; i++) {
        if (index > 0) {
            index -= 1;
        } else {
            index = buf_size - 1;
        }

        tmp_buf[i] = bat_charge->bat_adc_sample_buf[index];
    }

    /* sort the ADC data by ascending sequence */
	bat_volt_selectsort(tmp_buf, adc_count);

    /* get the last 4/5 position data */
    bat_adc = tmp_buf[adc_count * 4/5 - 1];

    volt_mv = bat_adc_get_voltage_mv(bat_adc);

    return volt_mv;
}

/* get battery voltage in millivolt */
uint32_t get_battery_voltage_mv(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    return bat_charge->bat_real_volt;
}

/* get the percent of battery capacity  */
uint32_t get_battery_percent(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    uint32_t bat_volt = get_battery_voltage_mv();
    int i;
    uint32_t level_begin;
    uint32_t level_end;
    uint32_t percent;

    if (bat_volt >= bat_charge->configs.cfg_charge.Charge_Stop_Voltage) {
        return 100;
    }

    for (i = CFG_MAX_BATTERY_LEVEL - 1; i > 0; i--) {
        if (bat_volt >= bat_charge->configs.cfg_bat_level.Level[i])
        {
            break;
        }
    }

    level_begin = bat_charge->configs.cfg_bat_level.Level[i];

    if (i < CFG_MAX_BATTERY_LEVEL - 1) {
        level_end = bat_charge->configs.cfg_bat_level.Level[i+1];
    } else {
        level_end = bat_charge->configs.cfg_charge.Charge_Stop_Voltage;
    }

    if (bat_volt > level_end) {
        bat_volt = level_end;
    }

    percent = 100 * i / CFG_MAX_BATTERY_LEVEL;

    if (bat_volt >  level_begin &&
        bat_volt <= level_end) {
        percent += 10 * (bat_volt - level_begin) / (level_end - level_begin);
    }

    return percent;
}

/* get the battery exist state */
bool battery_is_exist(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    return bat_charge->bat_exist_state;
}

/* get the flag of whether battery is full */
bool battery_is_full(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if (bat_charge->bat_charge_state == BAT_CHG_STATE_FULL) {
        return true;
    }

    return false;
}

/* get DC5V stat format string */
const char* get_dc5v_state_str(uint8_t state)
{
    static const char* const  str[] =
    {
        "OUT",
        "IN",
        "PENDING",
        "STANDBY",
    };

    return str[state];
}

/* put the data into debounce buffer and return true if all the data inside debounce buffer are the same. */
static bool bat_charge_dc5v_debounce(uint8_t *debounce_buf, int buf_size, uint8_t value)
{
    bool result = true;
    int i;

    for (i = 0; i < buf_size; i++) {
        if (debounce_buf[i] != value) {
            result = false;
        }

        if (i < buf_size - 1) {
            debounce_buf[i] = debounce_buf[i+1];
        } else {
            debounce_buf[i] = value;
        }
    }

    return result;
}

void bat_charge_dc5v_detect(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    uint8_t curr_state = get_dc5v_current_state();
    uint8_t last_state;

    static uint8_t last_unpending_state = DC5V_STATE_INVALID;

    int  debounce_buf_size =
        bat_charge->configs.cfg_charge.DC5V_Detect_Debounce_Time_Ms /
            BAT_CHARGE_DRIVER_TIMER_MS;

    CFG_Struct_Charger_Box *cfg = &(bat_charge_get_configs()->cfg_charger_box);

    /* debounce operation */
    if (bat_charge->dc5v_debounce_state != DC5V_STATE_PENDING) {
        if (bat_charge_dc5v_debounce(bat_charge->dc5v_debounce_buf,
                debounce_buf_size, curr_state) == false) {
			/* return if data in dc5v_debounce_buf are not the same */
            return ;
        }
    }

    if (bat_charge->dc5v_debounce_state == curr_state) {
        return ;
    }

    LOG_DBG("DC5V state:%s", get_dc5v_state_str(curr_state));

    last_state = bat_charge->dc5v_debounce_state;
    bat_charge->dc5v_debounce_state = curr_state;

    /* pulldown DC5V when the charger box plug-in to prevent from charger box enter standby */
    if (last_unpending_state == DC5V_STATE_INVALID ||
        last_unpending_state == DC5V_STATE_OUT) {
        if (curr_state == DC5V_STATE_IN ||
            curr_state == DC5V_STATE_STANDBY) {
            if (cfg->DC5V_Pull_Down_Current != DC5VPD_CURRENT_DISABLE) {
                dc5v_pull_down_enable (
                    cfg->DC5V_Pull_Down_Current,
                    cfg->DC5V_Pull_Down_Hold_Ms + 10
                );
            }
        }
    }

    if (curr_state != DC5V_STATE_PENDING) {
        last_unpending_state = curr_state;
    }

    if (curr_state == DC5V_STATE_IN) {
		if (bat_charge->callback)
        	bat_charge->callback(BAT_CHG_EVENT_DC5V_IN, NULL);

        /* reset battery low power configuration */
        bat_charge->bat_volt_low     = 0;
        bat_charge->bat_volt_low_ex  = 0;
        bat_charge->bat_volt_too_low = 0;
    } else if (curr_state == DC5V_STATE_STANDBY) {
		if (bat_charge->callback)
        	bat_charge->callback(BAT_CHG_EVENT_DC5V_STANDBY, NULL);
    } else if ((last_state != DC5V_STATE_INVALID) &&
             (curr_state == DC5V_STATE_OUT)) {
		if (bat_charge->callback)
        	bat_charge->callback(BAT_CHG_EVENT_DC5V_OUT, NULL);
    }
}

void bat_charge_timer_handler(struct k_work *work)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    uint32_t  bat_adc;

    /* sample BATADC data */
    bat_charge->bat_adc_sample_timer_count += 1;

    if ((bat_charge->bat_adc_sample_timer_count %
            (BAT_ADC_SAMPLE_INTERVAL_MS / BAT_CHARGE_DRIVER_TIMER_MS)) == 0) {
        bat_adc = bat_adc_get_sample_ex();

        bat_adc_sample_buf_put(bat_adc);

        if (bat_charge->bat_adc_sample_timer_count >=
                (10000 / BAT_CHARGE_DRIVER_TIMER_MS)) {
            bat_charge->bat_adc_sample_timer_count = 0;
            LOG_DBG("state:%d CHG:0x%x dc5v:%d bat:%dmv current:%dma",
				bat_charge->bat_charge_state,
                sys_read32(CHG_CTL_SVCC), soc_pmu_get_dc5v_status(),
				bat_adc_get_voltage_mv(bat_adc),
                chargei_adc_get_current_ma(chargei_adc_get_sample()));
        }
    }

    /* DC5V detect */
    bat_charge_dc5v_detect();

    /* process the battery and charegr status */
    bat_charge_status_proc();

	k_delayed_work_submit(&bat_charge->timer, K_MSEC(BAT_CHARGE_DRIVER_TIMER_MS));
}

/* check if battery voltage is low power */
void bat_check_voltage_low(uint32_t bat_volt)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if (bat_volt <= bat_charge->configs.cfg_bat_low.Battery_Too_Low_Voltage) {
        if (!bat_charge->bat_volt_too_low) {
            bat_charge->bat_volt_low     = true;
            bat_charge->bat_volt_low_ex  = true;
            bat_charge->bat_volt_too_low = true;
			if (bat_charge->callback)
           		bat_charge->callback(BAT_CHG_EVENT_BATTERY_TOO_LOW, NULL);
        }
    } else {
        bat_charge->bat_volt_too_low = 0;
    }

    if (bat_volt <= bat_charge->configs.cfg_bat_low.Battery_Low_Voltage_Ex) {
        if (!bat_charge->bat_volt_low_ex) {
            bat_charge->bat_volt_low    = true;
            bat_charge->bat_volt_low_ex = true;

			if (bat_charge->callback)
            	bat_charge->callback(BAT_CHG_EVENT_BATTERY_LOW_EX, NULL);
        }
    } else {
		/* lower voltage configuration may inexistence */
        bat_charge->bat_volt_low_ex = 0;
    }

    if (bat_volt <= bat_charge->configs.cfg_bat_low.Battery_Low_Voltage) {
        if (!bat_charge->bat_volt_low) {
            bat_charge->bat_volt_low = true;
			if (bat_charge->callback)
            	bat_charge->callback(BAT_CHG_EVENT_BATTERY_LOW, NULL);
        }
    } else {
        bat_charge->bat_volt_low     = 0;
        bat_charge->bat_volt_low_ex  = 0;
        bat_charge->bat_volt_too_low = 0;
    }
}

/* check battery voltage change */
uint32_t bat_check_voltage_change(uint32_t bat_volt)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

	LOG_DBG("bat_volt:%d bat_real_volt:%d", bat_volt, bat_charge->bat_real_volt);

    /* battery voltage can not decrease during charging and pre-charging  */
    if (bat_charge->bat_charge_state == BAT_CHG_STATE_PRECHARGE ||
        bat_charge->bat_charge_state == BAT_CHG_STATE_CHARGE) {
        if (bat_volt < bat_charge->bat_real_volt) {
            bat_volt = bat_charge->bat_real_volt;
        }
    } else {
		/* battery voltage can not increase without charging */
        if (bat_volt > bat_charge->bat_real_volt) {
            bat_volt = bat_charge->bat_real_volt;
        }
    }

    if (bat_charge->bat_real_volt != bat_volt) {
        bat_charge->bat_real_volt = bat_volt;
    }

    if (bat_charge->dc5v_debounce_state != DC5V_STATE_IN) {
        /* check if battery is low power when DC5V not plug-in */
        bat_check_voltage_low(bat_volt);
    }

    return bat_volt;
}

/* get the configration of battery and charger */
bat_charge_configs_t *bat_charge_get_configs(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    return &bat_charge->configs;
}

#ifdef CONFIG_CFG_DRV

#define BAT_CHARGE_CFG(x, prefix, item) \
			{ \
				ret = cfg_get_by_key(prefix##_##item, &val, sizeof((x).item)); \
				if (!ret) \
					return __LINE__; \
				LOG_DBG("%s:%d", #item, val); \
				(x).item = val; \
			}

static int bat_charge_config_init(void)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_context_ex_t *bat_charge_ex = bat_charge_get_context_ex();
	int ret, val;

	/* CFG_Struct_Battery_Charge */
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_Battery_Charge, Select_Charge_Mode);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_Battery_Charge, Charge_Current);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_Battery_Charge, Charge_Voltage);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_Battery_Charge, Charge_Stop_Mode);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_Battery_Charge, Charge_Stop_Voltage);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_Battery_Charge, Charge_Stop_Current);
	//BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_Battery_Charge, Precharge_Stop_Voltage);
	bat_charge->configs.cfg_charge.Precharge_Stop_Voltage = PRECHARGE_STOP_3_3_V; /* TODO */

	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_Battery_Charge, Battery_Check_Period_Sec);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_Battery_Charge, Charge_Check_Period_Sec);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_Battery_Charge, Charge_Full_Continue_Sec);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_Battery_Charge, Front_Charge_Full_Power_Off_Wait_Sec);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_Battery_Charge, DC5V_Detect_Debounce_Time_Ms);

	/* CFG_Struct_Battery_Low */
	BAT_CHARGE_CFG(bat_charge->configs.cfg_bat_low, ITEM_Battery_Low, Battery_Too_Low_Voltage);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_bat_low, ITEM_Battery_Low, Battery_Low_Voltage);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_bat_low, ITEM_Battery_Low, Battery_Low_Voltage_Ex);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_bat_low, ITEM_Battery_Low, Battery_Low_Prompt_Interval_Sec);

	/* CFG_Struct_Battery_Level */
	ret = cfg_get_by_key(ITEM_Battery_Level_Level,
			&bat_charge->configs.cfg_bat_level, sizeof(CFG_Struct_Battery_Level));
	if (!ret) {
		LOG_INF("use default battery level table");
		if (bat_charge->configs.cfg_charge.Charge_Voltage == CHARGE_VOLTAGE_4_35_V) {
			CFG_Struct_Battery_Level _cfg_bat_level = BOARD_BATTERY_VOLTAGE_4_35_V;
			bat_charge->configs.cfg_bat_level = _cfg_bat_level;
		} else {
			CFG_Struct_Battery_Level _cfg_bat_level = BOARD_BATTERY_VOLTAGE_4_20_V;
			bat_charge->configs.cfg_bat_level = _cfg_bat_level;
		}
	}

	/* CFG_Struct_Charger_Box */
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charger_box, ITEM_Charger_Box, Enable_Charger_Box);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charger_box, ITEM_Charger_Box, DC5V_Pull_Down_Current);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charger_box, ITEM_Charger_Box, DC5V_Pull_Down_Hold_Ms);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charger_box, ITEM_Charger_Box, Charger_Standby_Delay_Ms);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charger_box, ITEM_Charger_Box, Charger_Standby_Voltage);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charger_box, ITEM_Charger_Box, Charger_Wake_Delay_Ms);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charger_box, ITEM_Charger_Box, Enable_Battery_Recharge);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charger_box, ITEM_Charger_Box, Battery_Recharge_Threshold);

	ret = cfg_get_by_key(ITEM_Charger_Box_Charger_Box_Standby_Current,
			&bat_charge_ex->cfg_ex.Charger_Box_Standby_Current, sizeof(CFG_Type_Battery_Charge_Settings_Ex));
	if (!ret) {
		LOG_INF("use default standby current:%d", BOARD_CHARGER_BOX_STANDBY_CURRENT);
		bat_charge_ex->cfg_ex.Charger_Box_Standby_Current = BOARD_CHARGER_BOX_STANDBY_CURRENT;
	}

	bat_charge->configs.cfg_features = BOARD_BATTERY_SUPPORT_FEATURE;

	return 0;
}
#endif

/* battery and charger function initialization */
static int bat_charge_init(struct device *dev)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_context_ex_t *bat_charge_ex = bat_charge_get_context_ex();
	bat_charge_configs_t *configs = &bat_charge->configs;

    uint32_t charge_voltage_mv;

    if (bat_charge->inited) {
		LOG_INF("already inited");
        return 0;
    }

	bat_charge->bak_PMU_CHG_CTL = sys_read32(CHG_CTL_SVCC);

#ifdef CONFIG_CFG_DRV
	int ret = bat_charge_config_init();
	if (ret) {
		LOG_ERR("config init error:%d", ret);
		return -ENOENT;
	}
#else
	bat_charge_configs_t configs_default = BAT_CHARGE_CONFIG_DEFAULT;
	memcpy(configs, &configs_default, sizeof(bat_charge_configs_t));
#endif
	bat_charge->last_voltage_report = BAT_VOLTAGE_RESERVE;
	bat_charge->last_cap_report = BAT_CAP_RESERVE;

	charge_voltage_mv =
			 (4200 * 10 + (configs->cfg_charge.Charge_Voltage - CHARGE_VOLTAGE_4_20_V) * 125) / 10;

	/* limitation to stop charging voltage */
    if (configs->cfg_charge.Charge_Stop_Voltage > (charge_voltage_mv - 20)) {
        configs->cfg_charge.Charge_Stop_Voltage = (charge_voltage_mv - 20);
    }

    bat_charge->dc5v_debounce_state = DC5V_STATE_INVALID;

    /* do not limit the electric current during initialization */
    bat_charge_ex->limit_current_percent = 100;

    /* DC5V detection debounce operations */
	if (configs->cfg_charge.DC5V_Detect_Debounce_Time_Ms > DC5V_DEBOUNCE_MAX_TIME_MS) {
		LOG_INF("DC5V detect debounce time:%d invalid and use default",
				configs->cfg_charge.DC5V_Detect_Debounce_Time_Ms);
		configs->cfg_charge.DC5V_Detect_Debounce_Time_Ms = DC5V_DEBOUNCE_TIME_DEFAULT_MS;
	}

	memset(bat_charge->dc5v_debounce_buf, DC5V_STATE_INVALID, sizeof(bat_charge->dc5v_debounce_buf));

    /* BATADC sample buffer initialization */
	memset(bat_charge->bat_adc_sample_buf, 0, sizeof(bat_charge->bat_adc_sample_buf));

    /* assume that the battery is already existed and will detect later */
    bat_charge->bat_exist_state = 1;

	/* init the battery and charger environment */
    bat_charge_ctrl_init();

    /* check and get the real voltage of battery firstly */
    bat_check_real_voltage(1);

    /* initialize the buffer for sampling battery voltage */
    bat_adc_sample_buf_init();

    bat_charge->bat_real_volt = get_battery_voltage_by_time(BAT_VOLT_CHECK_SAMPLE_SEC);

    LOG_INF("battery voltage:%dmv", bat_charge->bat_real_volt);

    if (get_dc5v_current_state() != DC5V_STATE_IN) {
        /* check if battery is low power when DC5V not plug-in */
        bat_check_voltage_low(bat_charge->bat_real_volt);
    }

	k_delayed_work_init(&bat_charge->timer, bat_charge_timer_handler);
	k_delayed_work_submit(&bat_charge->timer, K_MSEC(BAT_CHARGE_DRIVER_TIMER_MS));

    bat_charge->inited = true;
	bat_charge->enabled = 1;

	return 0;
}

/* suspend battery and charger */
void bat_charge_suspend(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if (!bat_charge->inited
		|| !bat_charge->enabled) {
        return;
    }

    k_delayed_work_cancel(&bat_charge->timer);

	bat_charge->enabled = 0;
}

/* resume battery and charger */
void bat_charge_resume(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if (!bat_charge->inited
		|| bat_charge->enabled) {
        return;
    }

    bat_charge->dc5v_debounce_state = DC5V_STATE_INVALID;

    /* DC5V debounce buffer reset */
    memset(bat_charge->dc5v_debounce_buf, DC5V_STATE_INVALID, sizeof(bat_charge->dc5v_debounce_buf));

	bat_charge->bak_PMU_CHG_CTL = sys_read32(CHG_CTL_SVCC);

    /* check battery voltage */
    bat_check_real_voltage(1);

    /* initialize sample buffer */
    bat_adc_sample_buf_init();

    bat_charge->state_timer_count = 0;
	bat_charge->enabled = 1;

	k_delayed_work_submit(&bat_charge->timer, K_MSEC(BAT_CHARGE_DRIVER_TIMER_MS));
}

/* sample a DC5V ADC value */
int dc5v_adc_get_sample(void)
{
	int ret;
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	struct device *dev = bat_charge->dev;
	struct acts_battery_info *bat = dev->data;

	if ((!dev) || (!bat->adc_dev)) {
		LOG_ERR("no device!!!");
		return -ENXIO;
	}

	ret = adc_read(bat->adc_dev, &bat->dc5v_sequence);
	if (ret) {
		LOG_ERR("DC5V ADC read error %d", ret);
		return -EIO;
	}

	ret = sys_get_le16(bat->dc5v_sequence.buffer);

	//LOG_DBG("DC5V ADC:%d", ret);

	return ret;
}

/* sample a battery ADC value */
int bat_adc_get_sample(void)
{
	int ret;
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	struct device *dev = bat_charge->dev;
	struct acts_battery_info *bat = dev->data;

	if ((!dev) || (!bat->adc_dev)) {
		LOG_ERR("no device!!!");
		return -ENXIO;
	}

	ret = adc_read(bat->adc_dev, &bat->bat_sequence);
	if (ret) {
		LOG_ERR("battery ADC read error %d", ret);
		return -EIO;
	}

	ret = sys_get_le16(bat->bat_sequence.buffer);

	//LOG_DBG("battery ADC:%d", ret);

	return ret;
}

/* sample a chargei ADC value */
int chargei_adc_get_sample(void)
{
	int ret;
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	struct device *dev = bat_charge->dev;
	struct acts_battery_info *bat = dev->data;

	if ((!dev) || (!bat->adc_dev)) {
		LOG_ERR("no device!!!");
		return -ENXIO;
	}

	ret = adc_read(bat->adc_dev, &bat->charge_sequence);
	if (ret) {
		LOG_ERR("chargi ADC read error %d", ret);
		return -EIO;
	}

	ret = sys_get_le16(bat->charge_sequence.buffer);

	//LOG_DBG("chargi ADC:%d", ret);

	return ret;
}

static int battery_acts_get_property(struct device *dev,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
	{
		if (!bat_charge->bat_exist_state) {
			val->intval = POWER_SUPPLY_STATUS_BAT_NOTEXIST;
		} else if ((bat_charge->bat_charge_state == BAT_CHG_STATE_PRECHARGE)
			|| (bat_charge->bat_charge_state == BAT_CHG_STATE_NORMAL)
			|| (bat_charge->bat_charge_state == BAT_CHG_STATE_CHARGE)) {
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		} else if (bat_charge->bat_charge_state == BAT_CHG_STATE_FULL) {
			val->intval = POWER_SUPPLY_STATUS_FULL;
		}
		return 0;
	}
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 1;
		return 0;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = get_battery_voltage_mv();
		return 0;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = get_battery_percent();
		return 0;
	case POWER_SUPPLY_PROP_DC5V:
		val->intval = soc_pmu_get_dc5v_status();
		return 0;
	default:
		return -EINVAL;
	}
}

static void battery_acts_set_property(struct device *dev,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();

	switch (psp) {
		case POWER_SUPPLY_SET_PROP_FEATURE:
			bat_charge->configs.cfg_features = val->intval;
			LOG_INF("set new power supply feature:0x%x", val->intval);
			break;

		default:
			LOG_ERR("invalid psp cmd:%d", psp);
	}
}

static void battery_acts_register_notify(struct device *dev, bat_charge_callback_t cb)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	int flag;

	LOG_DBG("callback %p", cb);

	flag = irq_lock();
	if ((bat_charge->callback == NULL) && cb) {
		bat_charge->callback = cb;
	} else {
		LOG_ERR("notify func already exist!\n");
	}
	irq_unlock(flag);
}

static void battery_acts_enable(struct device *dev)
{
	LOG_DBG("enable battery detect");
	bat_charge_resume();
}

static void battery_acts_disable(struct device *dev)
{
	LOG_DBG("disable battery detect");
	bat_charge_suspend();
}

static const struct power_supply_driver_api battery_acts_driver_api = {
	.get_property = battery_acts_get_property,
	.set_property = battery_acts_set_property,
	.register_notify = battery_acts_register_notify,
	.enable = battery_acts_enable,
	.disable = battery_acts_disable,
};

#if DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay)

static int battery_acts_init(const struct device *dev)
{
	struct acts_battery_info *bat = dev->data;
	const struct acts_battery_config *cfg = dev->config;
	struct adc_channel_cfg channel_cfg = {0};
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	int ret;

	bat->adc_dev = (struct device *)device_get_binding(cfg->adc_name);
	if (!bat->adc_dev) {
		LOG_ERR("cannot found ADC device \'batadc\'\n");
		return -ENODEV;
	}

	channel_cfg.channel_id = cfg->batadc_chan;
	if (adc_channel_setup(bat->adc_dev, &channel_cfg)) {
		LOG_ERR("setup channel_id %d error", channel_cfg.channel_id);
		return -EFAULT;
	}

	channel_cfg.channel_id = cfg->chargeadc_chan;
	if (adc_channel_setup(bat->adc_dev, &channel_cfg)) {
		LOG_ERR("setup channel_id %d error", channel_cfg.channel_id);
		return -EFAULT;
	}

	channel_cfg.channel_id = cfg->dc5v_chan;
	if (adc_channel_setup(bat->adc_dev, &channel_cfg)) {
		LOG_ERR("setup channel_id %d error", channel_cfg.channel_id);
		return -EFAULT;
	}

	bat_charge->dev = (struct device *)dev;

	bat->bat_sequence.channels = BIT(cfg->batadc_chan);
	bat->bat_sequence.buffer = &bat->bat_adcval;
	bat->bat_sequence.buffer_size = sizeof(bat->bat_adcval);

	bat->charge_sequence.channels = BIT(cfg->chargeadc_chan);
	bat->charge_sequence.buffer = &bat->charge_adcval;
	bat->charge_sequence.buffer_size = sizeof(bat->charge_adcval);

	bat->dc5v_sequence.channels = BIT(cfg->dc5v_chan);
	bat->dc5v_sequence.buffer = &bat->dc5v_adcval;
	bat->dc5v_sequence.buffer_size = sizeof(bat->dc5v_adcval);

	ret = bat_charge_init(bat_charge->dev);
	if (ret)
		return ret;

	printk("battery initialized\n");

	return 0;
}

static struct acts_battery_info battery_acts_ddata;

static const struct acts_battery_config battery_acts_cdata = {
	.adc_name = DT_INST_PROP_BY_PHANDLE(0, io_channels, label),

	.batadc_chan = DT_IO_CHANNELS_INPUT_BY_IDX(DT_NODELABEL(batadc), 0),
	.chargeadc_chan = DT_IO_CHANNELS_INPUT_BY_IDX(DT_NODELABEL(batadc), 1),
	.dc5v_chan = DT_IO_CHANNELS_INPUT_BY_IDX(DT_NODELABEL(batadc), 2),

	.debug_interval_sec = DT_INST_PROP(0, debug_interval_sec),
};

DEVICE_DT_INST_DEFINE(0, battery_acts_init, NULL,
	    &battery_acts_ddata, &battery_acts_cdata, POST_KERNEL,
	    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &battery_acts_driver_api);

#endif // DT_NODE_HAS_STATUS

