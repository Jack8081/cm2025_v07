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

#define BATWORK_STACKSIZE		(1024*2)

bat_charge_context_t bat_charge_context;

#define USE_LOW_PRIO_WORK_QUEUE 1

#if (USE_LOW_PRIO_WORK_QUEUE==0)
struct k_work_q bat_drv_q;
struct k_work_q *p_bat_drv_q = &bat_drv_q;
uint8_t bat_work_stack[BATWORK_STACKSIZE] __aligned(4);
#else
extern struct k_work_q k_sys_low_prio_work_q;
struct k_work_q *p_bat_drv_q = &k_sys_low_prio_work_q;
#endif


#ifdef CONFIG_ACTS_BATTERY_SUPPORT_INIT_CHARGER_NTC
uint8_t current_init_charge_stage1_NTC[5] = {0, 20, 40, 40, 0};
uint8_t current_init_charge_stage2_NTC[5] = {0, 50, 80, 60, 0};
uint8_t current_init_charge_stage3_NTC[5] = {0, 60, 80, 80, 0};
#endif

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
typedef struct {
    uint32_t bat_mix;
    uint32_t bat_max;
    int bat_temperature;
}CFG_NTC_BAT_TEMPERATURE;

CFG_NTC_BAT_TEMPERATURE bat_temperature_arr[29] = {
    {2724, 2800, -40},
    {2695, 2723, -35},
    {2655, 2694, -30},
    {2604, 2654, -25},
    {2540, 2603, -20},
    {2459, 2539, -15},
    {2362, 2458, -10},
    {2248, 2361, -5},
    {2117, 2247, 0},
    {1971, 2116, 5},
    {1814, 1970, 10},
    {1649, 1813, 15},
    {1483, 1648, 20},
    {1318, 1482, 25},
    {1160, 1317, 30},
    {1012, 1159, 35},
    {876, 1011, 40},
    {754, 875, 45},
    {646, 753, 50},
    {552, 645, 55},
    {471, 551, 60},
    {401, 470, 65},
    {341, 400, 70},
    {290, 340, 75},
    {247, 289, 80},
    {211, 246, 85},
    {180, 210, 90},
    {154, 179, 95},
    {0, 153, 100},
};
#endif

/* init battery adc sample buffer */
void bat_adc_sample_buf_init(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    int  buf_size = BAT_ADC_SAMPLE_BUF_SIZE;
    int  i;
	int bat_adc_value;

    /* check the CHG_EN status before sampling battery voltage */
    get_dc5v_current_state();

    /* sample BAT ADC data in 10ms */
    for (i = 0; i < buf_size; i++) {
		bat_adc_value = bat_adc_get_sample();
		if(bat_adc_value > 0) {
        	bat_charge->bat_adc_sample_buf[i] = (uint16_t)bat_adc_value;
			k_usleep(10000 / buf_size);
		}
    }

    bat_charge->bat_adc_sample_timer_count = 0;
    bat_charge->bat_adc_sample_index = 0;

}

/* put bat adc value to buffer */
void bat_adc_sample_buf_put(uint32_t bat_adc)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    int buf_size = BAT_ADC_SAMPLE_BUF_SIZE;

    bat_charge->bat_adc_sample_buf[bat_charge->bat_adc_sample_index] = (uint16_t)bat_adc;
    bat_charge->bat_adc_sample_index += 1;

    if (bat_charge->bat_adc_sample_index >= buf_size) {
        bat_charge->bat_adc_sample_index = 0;
    }
}

void bat_chargei_sample_buf_init(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

	memset(bat_charge->charge_current_ma_buf, 0, BAT_CHARGEI_SAMPLE_BUF_SIZE*2);
    bat_charge->bat_chargei_sample_timer_count = 0;
    bat_charge->bat_chargei_sample_index = 0;
}

void bat_chargei_sample_buf_put(uint32_t chargei_ma)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    int buf_size = BAT_CHARGEI_SAMPLE_BUF_SIZE;

    bat_charge->charge_current_ma_buf[bat_charge->bat_chargei_sample_index] = (uint16_t)chargei_ma;
    bat_charge->bat_chargei_sample_index += 1;

    if (bat_charge->bat_chargei_sample_index >= buf_size) {
        bat_charge->bat_chargei_sample_index = 0;
    }
}

uint16_t get_chargei_avg_value(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    int buf_size = BAT_CHARGEI_SAMPLE_BUF_SIZE;	
	uint16_t avg_val = 0;
	uint8_t i, cnt = 0;
	
	for(i=0; i<buf_size; i++) {
		if(bat_charge->charge_current_ma_buf[i] != 0) {
			cnt++;
			avg_val += bat_charge->charge_current_ma_buf[i];
		}
	}

	if(cnt!=0) {
		avg_val = avg_val/cnt;
	} else {
		avg_val = 0;
	}

	//LOG_INF("current avg: %dma", avg_val);

	return avg_val;
}

/* sort value in buffer */
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

    //get recent sample value
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

/* get current battery voltage in millivolt */
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

#if (defined CONFIG_SAMPLE_CASE_1) || (defined CONFIG_SAMPLE_CASE_XNT)
    if(bat_volt >= 4000)
    {
        percent = 100;
    }
    else if(bat_volt >= 3700 && bat_volt < 4000)
    {
        percent = 80;
    }
    else if(bat_volt >= 3500 && bat_volt < 3700)
    {
        percent = 60;
    }
    else if(bat_volt >= 3300 && bat_volt < 3500)
    {
        percent = 40;
    }
    else if(bat_volt >= 3100 && bat_volt < 3300)
    {
        percent = 20;
    }
    else
    {
        percent = 0;
    }
    LOG_INF("battery percent: %d", percent);

    return percent;
#endif

    if (bat_volt >= bat_charge->configs.cfg_charge.Charge_Stop_Voltage) {
        return 100;
    }

    for (i = CFG_MAX_BATTERY_LEVEL - 1; i > 0; i--) {
        if (bat_volt >= bat_charge->configs.cfg_bat_level.Level[i]) {
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
        percent += 100 * (bat_volt - level_begin) / ((level_end - level_begin) * CFG_MAX_BATTERY_LEVEL);
    }

	bat_charge->last_cap_detect = percent;

    //LOG_INF("battery percent: %d", percent);

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


/* get charge state format string */
const char* get_charge_state_str(uint8_t state)
{
    static const char* const  str[] =
    {
        "CHG_INIT",
        "CHG_LOW",
        "CHG_PRECHARGE",
        "CHG_NORMAL",
        "CHG_CHARGE",
        "CHG_FULL",
    };

    return str[state];
}

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
/**
**	get NTC format string 
**/
const char* get_NTC_state_str(NTC_temp_e state)
{
    static const char* const  str[] =
    {
        "NTC_INVALID",
        "NTC_HIGH_EX",
        "NTC_HIGH",
        "NTC_NORMAL",
        "NTC_LOW",
        "NTC_LOW_EX",
    };
	uint8_t index;

	if(state == NTC_TEMPERATURE_INVALID) {
		index = 0;
	} else {
		index = (uint8_t)state + 1;
	}
	
    return str[index];
}

void dump_ntc_cfg_info(void)
{
	uint8_t i;
	bat_charge_context_t *bat_charge = bat_charge_get_context();

	LOG_INF("NTC Config: ");
	LOG_INF("Enable: %d", bat_charge->configs.cfg_bat_ntc.NTC_Settings.Enable_NTC);
	LOG_INF("LRADC_CTRL: 0x%x", bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Ctrl);
	LOG_INF("LRADC_PU: %d", bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Pull_Up);
	LOG_INF("Test: %d", bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Value_Test);
	LOG_INF("Debounce: %ds", bat_charge->configs.cfg_bat_ntc.NTC_Settings.NTC_Det_Debounce_Time_S);

	for(i=0; i<5; i++) {
		LOG_INF("Range: 0x%x - 0x%x - %d%% - %dmv - %dmin", bat_charge->configs.cfg_bat_ntc.NTC_Ranges[i].ADC_Min, bat_charge->configs.cfg_bat_ntc.NTC_Ranges[i].ADC_Max, \
			bat_charge->configs.cfg_bat_ntc.NTC_Ranges[i].Adjust_Current_Percent,  (42000 + (bat_charge->configs.cfg_bat_ntc.NTC_Ranges[i].Adjust_CV_Level - CHARGE_VOLTAGE_4_20_V) * 125) / 10, \
			bat_charge->configs.cfg_bat_ntc.NTC_Ranges[i].Charge_Time_Limit);
	}
}


/* get a battery NTC adc value */
int bat_ntc_adc_get_sample(void)
{
	int ret;
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	struct device *dev = bat_charge->dev;
	struct acts_battery_info *bat = dev->data;

	if ((!dev) || (!bat->adc_dev)) {
		LOG_ERR("no device!!!");
		return -ENXIO;
	}

	ret = adc_read(bat->adc_dev, &bat->ntc_sequence);
	if (ret) {
		LOG_ERR("NTC ADC read error %d", ret);
		return -EIO;
	}

	ret = sys_get_le16(bat->ntc_sequence.buffer);

	if(bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Value_Test != 0) {
		LOG_INF("NTC ADC Value: 0x%x\n", ret);
	}

	return ret;
}

#ifdef CONFIG_CFG_DRV
static int bat_ntc_config_init(const struct device *dev, uint8_t *channel_id)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	int ret;
	unsigned int ctl_reg;
	bool read_err = false;

	*channel_id = 0xff;

	ret = cfg_get_by_key(ITEM_NTC_SETTING,
			&bat_charge->configs.cfg_bat_ntc.NTC_Settings, sizeof(CFG_Type_NTC_Settings));
	if (!ret) {
		LOG_INF("use default NTC config!");

		read_err = true;
		bat_charge->configs.cfg_bat_ntc.NTC_Settings.Enable_NTC = 0;
		bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Ctrl = 0;
		bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Pull_Up = 0;
		bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Value_Test = 0;

		memset(bat_charge->configs.cfg_bat_ntc.NTC_Ranges, 0,  sizeof(CFG_Type_NTC_Range)*5);
	}

	ret = cfg_get_by_key(ITEM_NTC_RANGES,
			&bat_charge->configs.cfg_bat_ntc.NTC_Ranges, sizeof(CFG_Type_NTC_Range)*5);
	if (!ret) {
		LOG_INF("use default NTC config!");

		read_err = true;
		bat_charge->configs.cfg_bat_ntc.NTC_Settings.Enable_NTC = 0;
		bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Ctrl = 0;
		bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Pull_Up = 0;
		bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Value_Test = 0;

		memset(bat_charge->configs.cfg_bat_ntc.NTC_Ranges, 0,  sizeof(CFG_Type_NTC_Range)*5);
	}

	dump_ntc_cfg_info();

	//config tool set not support NTC
    if(bat_charge->configs.cfg_bat_ntc.NTC_Settings.Enable_NTC == 0) {
		return 0;
    }

	if(!read_err) {
		uint8_t channel_no = bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Ctrl & 0xff;
		uint8_t gpio_no = (bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Ctrl & 0xff00) >> 8;
		uint8_t mfp_sel = (bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Ctrl & 0xff0000) >> 16;
		uint32_t pin_mode = 0;

        if (channel_no == LRADC_CTRL_NONE) {
		    return -ESRCH;
        }
		/* remap channel number to LRADC index */
		*channel_id = PMUADC_ID_LRADC1 + channel_no - 1;

		LOG_INF("gpio:%d, channel:%d, mfp:%d\n", gpio_no, channel_no, mfp_sel);

		acts_pinmux_get(gpio_no, &pin_mode);

		pin_mode = (pin_mode & ~GPIO_CTL_MFP_MASK) | (mfp_sel << GPIO_CTL_MFP_SHIFT);

		if(channel_no != 1) {
			//not LRADC1, use io pad pullup
			pin_mode = (pin_mode & ~GPIO_CTL_PULL_MASK) | (bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Pull_Up << GPIO_CTL_PULL_SHIFT);
		} else {
			//LRADC1, use PMUADC lradc1 100k pullup
			pin_mode = (pin_mode & ~GPIO_CTL_PULL_MASK);
		}
		
		acts_pinmux_set(gpio_no, pin_mode);

		if (gpio_no >= GPIO_MAX_PIN_NUM) {
			ctl_reg = WIO_REG_CTL(gpio_no - GPIO_MAX_PIN_NUM);
		} else {
			ctl_reg = GPIO_REG_CTL(GPIO_REG_BASE, gpio_no);
		}

		LOG_INF("NTC_IO_CTL: 0x%x\n\n", sys_read32(ctl_reg));
	}

	return 0;
}
#endif    // #ifdef CONFIG_CFG_DRV

#endif


/**
**	put the data into debounce buffer and 
**	return true if all the data inside debounce buffer are the same. 
**/
static bool bat_charge_debounce(uint8_t *debounce_buf, int buf_size, uint8_t value)
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

/* detect dc5v */
void bat_charge_dc5v_detect(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	dc5v_check_status_t *p = &bat_charge->dc5v_check_status;
    uint8_t curr_state = get_dc5v_current_state();
    uint8_t last_state;

    static uint8_t last_unpending_state = DC5V_STATE_INVALID;

    int  debounce_buf_size =
        bat_charge->configs.cfg_charge.DC5V_Detect_Debounce_Time_Ms /
            BAT_CHARGE_DRIVER_TIMER_MS;

#ifdef CONFIG_ACTS_BATTERY_SUPPLY_CHARGER_BOX
    CFG_Struct_Charger_Box *cfg = &(bat_charge_get_configs()->cfg_charger_box);
#endif

    /* debounce operation */
    if (bat_charge->dc5v_debounce_state != DC5V_STATE_PENDING) {
        if (bat_charge_debounce(bat_charge->dc5v_debounce_buf,
                debounce_buf_size, curr_state) == false) {
			/* return if data in dc5v_debounce_buf are not the same */
            return ;
        }
    }

    if (bat_charge->dc5v_debounce_state == curr_state) {
        return ;
    }

    last_state = bat_charge->dc5v_debounce_state;
    bat_charge->dc5v_debounce_state = curr_state;

	LOG_INF("DC5V state change, last: %s, new: %s", get_dc5v_state_str(last_state), get_dc5v_state_str(curr_state));

	if(bat_charge->err_to_stop_charge != 0) {
		bat_charge->err_status_dc5v_change_flag = 1;
		LOG_INF("dc5v change, need restart from err stop!");	
	}

#ifdef CONFIG_ACTS_BATTERY_SUPPLY_CHARGER_BOX
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
#endif

    if (curr_state != DC5V_STATE_PENDING) {
        last_unpending_state = curr_state;
    }

	//dc5v charger deal after debounce
	if (curr_state == DC5V_STATE_IN) {
		if (last_state != DC5V_STATE_IN) {
			/* delay 500ms to enable charger */
			k_delayed_work_submit(&p->charger_enable_timer, K_MSEC(500));
		}
	} else if(curr_state == DC5V_STATE_OUT){
		if (last_state == DC5V_STATE_IN ||
			last_state == DC5V_STATE_INVALID) {
	
			/* disable charger enable timer */
            charger_enable_timer_delete();

			//just switch to little current, not close charger
			LOG_INF("DC5V out, switch to precharge current!");
			//pmu_chg_ctl_reg_write((0x1<<CHG_CTL_SVCC_CV_3V3), (0x1<<CHG_CTL_SVCC_CV_3V3));
			bat_charge_set_current(bat_charge->precharge_current_sel);
		}
	} else {
		; //empty
	}
	
    if (curr_state == DC5V_STATE_IN) {
		if (bat_charge->callback) {
        	bat_charge->callback(BAT_CHG_EVENT_DC5V_IN, NULL);
		}

        /* reset battery low power configuration */
        bat_charge->bat_volt_low     = 0;
        bat_charge->bat_volt_low_ex  = 0;
        bat_charge->bat_volt_too_low = 0;
    } else if (curr_state == DC5V_STATE_STANDBY) {
		if (bat_charge->callback) {
        	bat_charge->callback(BAT_CHG_EVENT_DC5V_STANDBY, NULL);
		}
    } else if ((last_state != DC5V_STATE_INVALID) &&
             (curr_state == DC5V_STATE_OUT)) {
		if (bat_charge->callback) {
        	bat_charge->callback(BAT_CHG_EVENT_DC5V_OUT, NULL);
		}
    }
}

/**
**	get battery init charge current
**/
void bat_get_initcharge_current(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_configs_t *configs = &bat_charge->configs;

	bat_charge->init_charge_current_ma_1 = bat_charge_get_current_ma(configs->cfg_charge.Init_Charge_Current_1);
	bat_charge->init_charge_current_ma_2 = bat_charge_get_current_ma(configs->cfg_charge.Init_Charge_Current_2);
	
	LOG_INF("init charge current: %dma, %dma", bat_charge->init_charge_current_ma_1, \
		bat_charge->init_charge_current_ma_2);
}

void bat_get_precharge_current(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_configs_t *configs = &bat_charge->configs;

    int cc_ma;
	int precharge_ma;

	cc_ma = bat_charge_get_current_ma(configs->cfg_charge.Charge_Current);

	precharge_ma = cc_ma * (configs->cfg_charge.Precharge_Current + 1) *5 / 100;

	bat_charge->precharge_current_sel = bat_charge_get_current_level(precharge_ma);

	if(bat_charge->precharge_current_sel < configs->cfg_charge.Precharge_Current_Min_Limit) {
		bat_charge->precharge_current_sel = configs->cfg_charge.Precharge_Current_Min_Limit;
		precharge_ma = bat_charge_get_current_ma(bat_charge->precharge_current_sel);
		//LOG_INF("Use precharge current min!");
	}

	LOG_INF("cc: %dma  pre: %dma  level: %d", cc_ma, precharge_ma, bat_charge->precharge_current_sel);
}


#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
void bat_charge_NTC_proc(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_configs_t *cfg = &bat_charge->configs;
	int ntc_adc;
	int i;
    static u8_t bat_ntc_adc_timer_count = 0;
	NTC_temp_e ntc_current_state;
    int debounce_buf_size =
        (bat_charge->configs.cfg_bat_ntc.NTC_Settings.NTC_Det_Debounce_Time_S * 1000)/BAT_NTC_SAMPLE_INTERVAL_MS;	

	if(cfg->cfg_bat_ntc.NTC_Settings.Enable_NTC == 0) {
		bat_charge->limit_cv_level = 0xff;
		return;
	}
    
    bat_ntc_adc_timer_count++;
    if((bat_ntc_adc_timer_count % (BAT_NTC_SAMPLE_INTERVAL_MS * 10 / BAT_CHARGE_DRIVER_TIMER_MS)) == 0)
    {
        bat_ntc_adc_timer_count = 0;
        ntc_adc = bat_ntc_adc_get_sample();
        if(ntc_adc < 0)
        {
            LOG_ERR("Bat NTC sample fail!");
            return;
        }

        uint32_t bat_ntc_volt_mv = 0;
        int bat_ntc_temperature = 0;

        if(bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Pull_Up == 0)
        {
            bat_ntc_volt_mv = ntc_adc * 3600 / 1024;
        }
        else
        {
            bat_ntc_volt_mv = ntc_adc * 3600 / 1024;
        }

        for(i = 0; i < ARRAY_SIZE(bat_temperature_arr); i++)
        {
            if(bat_ntc_volt_mv >= bat_temperature_arr[i].bat_mix && bat_ntc_volt_mv <= bat_temperature_arr[i].bat_max)
            {
                bat_ntc_temperature = bat_temperature_arr[i].bat_temperature;
                break;
            }
        }
        LOG_INF("BAT NTC:%dmv, ntc_adc:%x, bat_temperature:%d", bat_ntc_volt_mv, ntc_adc, bat_ntc_temperature);
    }

	if(bat_charge->dc5v_debounce_state != DC5V_STATE_IN) {
		//LOG_INF("No DC5V, not need NTC!");
		bat_charge->limit_cv_level = 0xff;
		return;
	}

	if((bat_charge->bat_charge_state != BAT_CHG_STATE_PRECHARGE) && \
		  (bat_charge->bat_charge_state != BAT_CHG_STATE_CHARGE) && \
		  (bat_charge->bat_charge_state != BAT_CHG_STATE_FULL)) {
		//LOG_INF("Current state not need NTC: %s", get_charge_state_str(bat_charge->bat_charge_state));
		bat_charge->limit_cv_level = 0xff;
		return;
	}	

    /* sample battery ntc data */
	bat_charge->bat_ntc_sample_timer_count++;

    if ((bat_charge->bat_ntc_sample_timer_count %
            (BAT_NTC_SAMPLE_INTERVAL_MS / BAT_CHARGE_DRIVER_TIMER_MS)) == 0) {

        ntc_adc = bat_ntc_adc_get_sample();
		if(ntc_adc < 0) {
			LOG_ERR("Bat NTC sample fail!");
			return;
		}

    	for (i = 0; i < ARRAY_SIZE(cfg->cfg_bat_ntc.NTC_Ranges); i++) {
        	if (cfg->cfg_bat_ntc.NTC_Ranges[i].ADC_Min < cfg->cfg_bat_ntc.NTC_Ranges[i].ADC_Max &&
            	ntc_adc >= cfg->cfg_bat_ntc.NTC_Ranges[i].ADC_Min &&
            	ntc_adc <= cfg->cfg_bat_ntc.NTC_Ranges[i].ADC_Max) {

				ntc_current_state = (NTC_temp_e)i; 

        		if (bat_charge_debounce(bat_charge->NTC_debounce_buf,
                		debounce_buf_size, (uint8_t)ntc_current_state) == false) {
					/* return if data in ntc_debounce_buf are not the same */
            		return ;
        		}

				if(ntc_current_state == bat_charge->ntc_debounce_state) {
					return;
				}

				LOG_INF("NTC state change, last: %s, new: %s", get_NTC_state_str(bat_charge->ntc_debounce_state), get_NTC_state_str(ntc_current_state));

				bat_charge->current_ntc_charge_time_limit = cfg->cfg_bat_ntc.NTC_Ranges[i].Charge_Time_Limit;
				bat_charge->current_ntc_begin_time = k_uptime_get_32();

				LOG_INF("time limit: %d, start: %d", bat_charge->current_ntc_charge_time_limit, bat_charge->current_ntc_begin_time);

				bat_charge->ntc_debounce_state = ntc_current_state;
					
            	bat_charge_limit_current(cfg->cfg_bat_ntc.NTC_Ranges[i].Adjust_Current_Percent);

				if(cfg->cfg_bat_ntc.NTC_Ranges[i].Adjust_CV_Level != 0xff) {
					bat_charge->limit_cv_level = cfg->cfg_bat_ntc.NTC_Ranges[i].Adjust_CV_Level;
					LOG_INF("limit cv level: %d", bat_charge->limit_cv_level);
				}
				
            	break;
        	}
    	}

		bat_charge->bat_ntc_sample_timer_count = 0;
    }

}
#endif

/* battery charge timer handle */
void bat_charge_timer_handler(struct k_work *work)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    uint32_t  bat_adc;
	uint32_t bat_charge_current;
	int bat_adc_value;
	uint32_t bat_volt_mv;

    /* sample BATADC data */
    bat_charge->bat_adc_sample_timer_count += 1;
	/* sample charge current */
	bat_charge->bat_chargei_sample_timer_count += 1;
	
	//get batadc sample
    if ((bat_charge->bat_adc_sample_timer_count %
            (BAT_ADC_SAMPLE_INTERVAL_MS / BAT_CHARGE_DRIVER_TIMER_MS)) == 0) {
        bat_adc_value = bat_adc_get_sample();
		if(bat_adc_value > 0) {
			bat_adc = (uint32_t)bat_adc_value;

			bat_volt_mv = bat_adc_get_voltage_mv(bat_adc);
			if((bat_volt_mv > BAT_VOLTAGE_SAMPLE_VALID_MIN) && (bat_volt_mv < BAT_VOLTAGE_SAMPLE_VALID_MAX)) {
				//LOG_INF("put: %d, %dmv\n\n", bat_adc, bat_volt_mv);
        		bat_adc_sample_buf_put(bat_adc);
			} else {
				LOG_INF("drop: %d, %dmv\n\n", bat_adc, bat_volt_mv);
			}

        	if (bat_charge->bat_adc_sample_timer_count >=
                	(BAT_CHARGER_DEBUG_INFO_OUT_MS / BAT_CHARGE_DRIVER_TIMER_MS)) {
           		bat_charge->bat_adc_sample_timer_count = 0;
                LOG_INF("state:%s CHG:0x%x BDG:0x%x dc5v:%d-%dmv chargei_d:0x%x bat:%dmv real_vol:%dmv percent:%d%% current:%dma",
					get_charge_state_str(bat_charge->bat_charge_state),
                	sys_read32(CHG_CTL_SVCC), sys_read32(BDG_CTL_SVCC), soc_pmu_get_dc5v_status(), dc5v_adc_get_voltage_mv(dc5v_adc_get_sample()), sys_read32(CHGIADC_DATA),
					bat_adc_get_voltage_mv(bat_adc),
					bat_charge->bat_real_volt,
					get_battery_percent(),
                	chargei_adc_get_current_ma(chargei_adc_get_sample()));
        	}
		} else {
			LOG_ERR("Sample BATADC Err!");
		}
    }

	//get chargei sample
    if ((bat_charge->bat_chargei_sample_timer_count %
            (BAT_CHARGEI_SAMPLE_INTERVAL_MS / BAT_CHARGE_DRIVER_TIMER_MS)) == 0) {
        bat_charge_current = chargei_adc_get_current_ma(chargei_adc_get_sample());

        bat_chargei_sample_buf_put(bat_charge_current);

		if((bat_charge->bat_charge_state == BAT_CHG_STATE_PRECHARGE) || (bat_charge->bat_charge_state == BAT_CHG_STATE_CHARGE)) {
			bat_charge->bat_real_charge_current = get_chargei_avg_value();
		}

		bat_charge->bat_chargei_sample_timer_count = 0;
    }			

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
	bat_charge_NTC_proc();
#endif

    /* DC5V detect */
    bat_charge_dc5v_detect();

    /* process the battery and charegr status */
    bat_charge_status_proc();

	//k_delayed_work_submit(&bat_charge->timer, K_MSEC(BAT_CHARGE_DRIVER_TIMER_MS));
	k_delayed_work_submit_to_queue(p_bat_drv_q, &bat_charge->timer, K_MSEC(BAT_CHARGE_DRIVER_TIMER_MS));
	
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
			if (bat_charge->callback) {
           		bat_charge->callback(BAT_CHG_EVENT_BATTERY_TOO_LOW, NULL);
			}
        }
    } else {
        bat_charge->bat_volt_too_low = 0;
    }

    if (bat_volt <= bat_charge->configs.cfg_bat_low.Battery_Low_Voltage_Ex) {
        if (!bat_charge->bat_volt_low_ex) {
            bat_charge->bat_volt_low    = true;
            bat_charge->bat_volt_low_ex = true;

			if (bat_charge->callback) {
            	bat_charge->callback(BAT_CHG_EVENT_BATTERY_LOW_EX, NULL);
			}
        }
    } else {
		/* lower voltage configuration may inexistence */
        bat_charge->bat_volt_low_ex = 0;
    }

    if (bat_volt <= bat_charge->configs.cfg_bat_low.Battery_Low_Voltage) {
        if (!bat_charge->bat_volt_low) {
            bat_charge->bat_volt_low = true;
			if (bat_charge->callback) {
            	bat_charge->callback(BAT_CHG_EVENT_BATTERY_LOW, NULL);
			}
        }
    } else {
        bat_charge->bat_volt_low     = 0;
        bat_charge->bat_volt_low_ex  = 0;
        bat_charge->bat_volt_too_low = 0;
    }
}

static uint32_t get_diff_time(uint32_t end_time, uint32_t begin_time)
{
    uint32_t  diff_time;
    
    if (end_time >= begin_time) {
        diff_time = (end_time - begin_time);
    } else {
        diff_time = ((uint32_t)-1 - begin_time + end_time + 1);
    }

    return diff_time;
}

/* permillage: 0 ~ 1100
 */
int get_battery_permillage(uint32_t mv)
{
    bat_charge_context_t*  bat_charge = bat_charge_get_context();

    uint32_t  level_begin;
    uint32_t  level_end;
    int     permillage;
    int     i;

    if (mv >= bat_charge->configs.cfg_charge.Charge_Stop_Voltage) {
        level_begin = bat_charge->configs.cfg_charge.Charge_Stop_Voltage;
        
        level_end = (4200 * 10 + (bat_charge->configs.cfg_charge.Charge_Voltage - 
            CHARGE_VOLTAGE_4_20_V) * 125) / 10;
        
        i = 10;
    } else {
        for (i = CFG_MAX_BATTERY_LEVEL - 1; i > 0; i--) {
            if (mv >= bat_charge->configs.cfg_bat_level.Level[i]) {
                break;
            }
        }
        
        level_begin = bat_charge->configs.cfg_bat_level.Level[i];
        
        if (i < CFG_MAX_BATTERY_LEVEL - 1) {
            level_end = bat_charge->configs.cfg_bat_level.Level[i+1];
        } else {
            level_end = bat_charge->configs.cfg_charge.Charge_Stop_Voltage;
        }
    }

    if (mv > level_end) {
        mv = level_end;
    }

    permillage = 1000 * i / CFG_MAX_BATTERY_LEVEL;

    if (mv >  level_begin && 
        mv <= level_end) {
        permillage += 100 * (mv - level_begin) / (level_end - level_begin);
    }

    return permillage;
}


/* permillage: 0 ~ 1100
 */
uint32_t bat_permillage_to_mv(int permillage)
{
    bat_charge_context_t*  bat_charge = bat_charge_get_context();

    uint32_t  mv;
    uint32_t  level_begin;
    uint32_t  level_end;
    int     i;

    if (permillage < 0) {
        permillage = 0;
    }
    
    i = permillage / 100;

    if (i >= 11) {
        mv = (4200 * 10 + (bat_charge->configs.cfg_charge.Charge_Voltage - 
            CHARGE_VOLTAGE_4_20_V) * 125) / 10;

        return mv;
    } else if (i == 10) {
        level_begin = bat_charge->configs.cfg_charge.Charge_Stop_Voltage;

        level_end = (4200 * 10 + (bat_charge->configs.cfg_charge.Charge_Voltage - 
            CHARGE_VOLTAGE_4_20_V) * 125) / 10;
    } else {
        level_begin = bat_charge->configs.cfg_bat_level.Level[i];
        
        if (i < CFG_MAX_BATTERY_LEVEL - 1) {
            level_end = bat_charge->configs.cfg_bat_level.Level[i+1];
        } else {
            level_end = bat_charge->configs.cfg_charge.Charge_Stop_Voltage;
        }
    }

    mv = level_begin;

    if (level_begin < level_end) {
        mv += (level_end - level_begin) * (permillage % 100) / 100;
    }

    return mv;
}


/* check battery real voltage change */
uint32_t bat_check_voltage_change(uint32_t bat_volt)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    uint32_t elapsed_sec = 0;
    uint32_t limit_mv;
    int     permillage;

    if (bat_charge->last_time_check_bat_volt != 0) {
		LOG_INF("last: %d, %d", bat_charge->last_time_check_bat_volt, k_uptime_get_32());
        elapsed_sec = get_diff_time(k_uptime_get_32(), 
            bat_charge->last_time_check_bat_volt) / 1000;
    }

	if((bat_volt > BAT_VOLTAGE_SAMPLE_VALID_MAX) || (bat_volt < BAT_VOLTAGE_SAMPLE_VALID_MIN)) {
		LOG_INF("err volt:%d\n\n\n\n\n\n", bat_volt);
	}

    permillage = get_battery_permillage(bat_volt);
    
    LOG_INF("vol:%dmv, permillage: %d, elapse_sec: %ds", bat_volt, permillage, elapsed_sec);

    if (bat_charge->bat_charge_state == BAT_CHG_STATE_INIT) {
        if (bat_charge->last_saved_bat_volt != 0) {
            bat_volt = bat_charge->last_saved_bat_volt;
            bat_charge->last_saved_bat_volt = 0;
        }
    }
    
    permillage = get_battery_permillage(bat_charge->bat_real_volt);

    LOG_INF("last real vol:%dmv, permillage: %d", bat_charge->bat_real_volt, permillage);

    if (bat_charge->bat_charge_state == BAT_CHG_STATE_PRECHARGE ||
        bat_charge->bat_charge_state == BAT_CHG_STATE_CHARGE) {
        if (elapsed_sec > 0 &&
            elapsed_sec < 3600) {
            /* Max ascend 2% per minute?
             */
            permillage += elapsed_sec * (BAT_REAL_VOLTAGE_SMOOTH_IN_CHARGE*10) / 60;
            
            limit_mv = bat_permillage_to_mv(permillage);

            if (bat_volt > limit_mv) {
				LOG_INF("Limit real volt increase speed when charge!");
                bat_volt = limit_mv;
            }
        }
        
        if (bat_volt < bat_charge->bat_real_volt) {
            bat_volt = bat_charge->bat_real_volt;
        }
    } else {
        if (elapsed_sec > 0 &&
            elapsed_sec < 3600) {
            /* Max descend 1% per minute?
             */
            if(permillage >= (elapsed_sec * (BAT_REAL_VOLTAGE_SMOOTH_IN_DISCHARGE*10) / 60)) {
            	permillage -= elapsed_sec * (BAT_REAL_VOLTAGE_SMOOTH_IN_DISCHARGE*10) / 60;
            	limit_mv = bat_permillage_to_mv(permillage);

            	if (bat_volt < limit_mv) {
					LOG_INF("Limit real volt reduce speed when dis-charge!");
                	bat_volt = limit_mv;
            	}
            }
        }
        
        if (bat_volt > bat_charge->bat_real_volt){
            bat_volt = bat_charge->bat_real_volt;
        }
    }
    
    bat_charge->last_time_check_bat_volt = k_uptime_get_32();

    if (bat_charge->bat_real_volt != bat_volt) {
		LOG_INF("bat_real_vol change: %d -- %d", bat_charge->bat_real_volt, bat_volt);
        bat_charge->bat_real_volt = bat_volt;		
    }

    if (bat_charge->dc5v_debounce_state != DC5V_STATE_IN) {
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

#define BAT_CHARGE_CFG(x, item_key, item) \
			{ \
				uint32_t val=0; \
				if (!cfg_get_by_key(item_key, &val, sizeof((x).item))) { \
					LOG_INF("item: 0x%x", item_key); \
					return __LINE__; \
				} \
				LOG_INF("%s:%d", #item, val); \
				(x).item = val; \
			}

/* get config item from config tool */
static int bat_charge_config_init(void)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	int ret;

	/* CFG_Struct_Battery_Charge */
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_SELECT_CHARGE_MODE, Select_Charge_Mode);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_CURRENT, Charge_Current);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_VOLTAGE, Charge_Voltage);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_STOP_MODE, Charge_Stop_Mode);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_STOP_VOLTAGE, Charge_Stop_Voltage);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_STOP_CURRENT, Charge_Stop_Current);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_INIT_CHARGE_ENABLE, Init_Charge_Enable);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_INIT_CHARGE_CURRENT1, Init_Charge_Current_1);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_INIT_CHARGE_VOLTAGE1, Init_Charge_Vol_1);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_INIT_CHARGE_CURRENT2, Init_Charge_Current_2);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_INIT_CHARGE_VOLTAGE2, Init_Charge_Vol_2);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_INIT_CHARGE_CURRENT3, Init_Charge_Current_3);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_INIT_CHARGE_VOLTAGE3, Init_Charge_Vol_3);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_PRECHARGE_ENABLE, Precharge_Enable);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_PRECHARGE_STOP_VOLTAGE, Precharge_Stop_Voltage);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_PRECHARGE_CURRENT, Precharge_Current);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_PRECHARGE_CURRENT_MIN_LIMIT, Precharge_Current_Min_Limit);

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_FAST_CHARGER
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_FAST_CHARGE_ENABLE, Fast_Charge_Enable);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_FAST_CHARGE_CURRENT, Fast_Charge_Current);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_FAST_CHARGE_VOLTAGE_THRESHOLD, Fast_Charge_Voltage_Threshold);
#endif

	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_ENABLE_BATTERY_RECHARGE, Enable_Battery_Recharge);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_BATTERY_RECHARGE_THRESHOLD_LOWTEMP, Recharge_Threshold_Low_Temperature);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_BATTERY_RECHARGE_THRESHOLD_NORTEMP, Recharge_Threshold_Normal_Temperature);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_BATTERY_RECHARGE_THRESHOLD_HIGHTEMP, Recharge_Threshold_High_Temperature);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_BATTERY_OVP_ENABLE, Bat_OVP_Enable);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_BATTERY_OVP_VOLTAGE_LOWTEMP, OVP_Voltage_Low_Temperature);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_BATTERY_OVP_VOLTAGE_NORTEMP, OVP_Voltage_Normal_Temperature);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_BATTERY_OVP_VOLTAGE_HIGHTEMP, OVP_Voltage_High_Temperature);	
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_BATTERY_CHARGE_TOTAL_TIME_LIMIT, Charge_Total_Time_Limit);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_BATTERY_CHECK_PERIOD_SEC, Battery_Check_Period_Sec);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_CHECK_PERIOD_SEC, Charge_Check_Period_Sec);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_FULL_CONTINUE_SEC, Charge_Full_Continue_Sec);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_FRONT_CHARGE_FULL_POWER_OFF_WAIT_SEC, Front_Charge_Full_Power_Off_Wait_Sec);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_DC5V_DETECT_DEBOUNCE_TIME_MS, DC5V_Detect_Debounce_Time_Ms);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charge, ITEM_CHARGE_BATTERY_DEFAULT_RDROP, Battery_Default_RDrop);

	/* CFG_Struct_Battery_Low */
	BAT_CHARGE_CFG(bat_charge->configs.cfg_bat_low, ITEM_BATTERY_TOO_LOW_VOLTAGE, Battery_Too_Low_Voltage);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_bat_low, ITEM_BATTERY_LOW_VOLTAGE, Battery_Low_Voltage);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_bat_low, ITEM_BATTERY_LOW_VOLTAGE_EX, Battery_Low_Voltage_Ex);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_bat_low, ITEM_BATTERY_LOW_PROMPT_INTERVAL_SEC, Battery_Low_Prompt_Interval_Sec);

	/* CFG_Struct_Battery_Level */
	ret = cfg_get_by_key(ITEM_BATTERY_LEVEL,
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

#ifdef CONFIG_ACTS_BATTERY_SUPPLY_CHARGER_BOX
	/* CFG_Struct_Charger_Box */
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charger_box, ITEM_CHARGEBOX_ENABLE_CHARGER_BOX, Enable_Charger_Box);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charger_box, ITEM_CHARGEBOX_DC5V_PULL_DOWN_CURRENT, DC5V_Pull_Down_Current);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charger_box, ITEM_CHARGEBOX_DC5V_PULL_DOWN_HOLD_MS, DC5V_Pull_Down_Hold_Ms);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charger_box, ITEM_CHARGEBOX_STANDBY_DELAY_MS, Charger_Standby_Delay_Ms);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charger_box, ITEM_CHARGEBOX_STANDBY_VOLTAGE, Charger_Standby_Voltage);
	BAT_CHARGE_CFG(bat_charge->configs.cfg_charger_box, ITEM_CHARGEBOX_WAKE_DELAY_MS, Charger_Wake_Delay_Ms);

	ret = cfg_get_by_key(ITEM_CHARGEBOX_BOX_STANDBY_CURRENT,
				&bat_charge->cfg_ex.Charger_Box_Standby_Current, sizeof(CFG_Type_Battery_Charge_Settings_Ex));
	if (!ret) {
		LOG_ERR("failed to get standby current:%d", ITEM_CHARGEBOX_BOX_STANDBY_CURRENT);
	}
#endif

	/* CFG_TYPE_SYS_SUPPORT_FEATURES */
	ret = cfg_get_by_key(ITEM_SYS_SETTINGS_SUPPORT_FEATURES, &bat_charge->configs.cfg_features, sizeof(cfg_uint16));
	if (!ret) {
		LOG_ERR("failed to get system features:%d", ITEM_SYS_SETTINGS_SUPPORT_FEATURES);
	}

	LOG_INF("Fast_Charge: %d", bat_charge->configs.cfg_charge.Fast_Charge_Enable);
	LOG_INF("Repeat_Charge: %d", bat_charge->configs.cfg_charge.Enable_Battery_Recharge);

	return 0;
}
#endif


void dump_bat_cfg_info(void)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_configs_t *configs = &bat_charge->configs;

	LOG_INF("Bat Config Start:");
	LOG_INF("Charge Mode: %s", (configs->cfg_charge.Select_Charge_Mode == BAT_BACK_CHARGE_MODE) ? "back" : "front");
	LOG_INF("CC: %dma, CV: %dmv", bat_charge_get_current_ma(configs->cfg_charge.Charge_Current), \
		(4200 * 10 + (configs->cfg_charge.Charge_Voltage - CHARGE_VOLTAGE_4_20_V) * 125) / 10);

	LOG_INF("Stop: %d, %dmv, %d", configs->cfg_charge.Charge_Stop_Mode, configs->cfg_charge.Charge_Stop_Voltage, \
		configs->cfg_charge.Charge_Stop_Current);

	LOG_INF("Init Charge: %d, %dma, %dmv, %dma, %dmv, %dma, %dmv", configs->cfg_charge.Init_Charge_Enable, \
		bat_charge_get_current_ma(configs->cfg_charge.Init_Charge_Current_1), \
		configs->cfg_charge.Init_Charge_Vol_1, \
		bat_charge_get_current_ma(configs->cfg_charge.Init_Charge_Current_2), \
		configs->cfg_charge.Init_Charge_Vol_2, \
		bat_charge_get_current_ma(configs->cfg_charge.Init_Charge_Current_3),
		configs->cfg_charge.Init_Charge_Vol_3);	

	LOG_INF("Pre-charge: %d, %dmv, %d, %d", configs->cfg_charge.Precharge_Enable, configs->cfg_charge.Precharge_Stop_Voltage, \
		configs->cfg_charge.Precharge_Current, \
		configs->cfg_charge.Precharge_Current_Min_Limit);	

	LOG_INF("Fast Charge: %d, %dma, %d", configs->cfg_charge.Fast_Charge_Enable, \
		bat_charge_get_current_ma(configs->cfg_charge.Fast_Charge_Current), \
		configs->cfg_charge.Fast_Charge_Voltage_Threshold);

	LOG_INF("Re-charge: %d, %dmv, %dmv, %dmv", configs->cfg_charge.Enable_Battery_Recharge, \
		configs->cfg_charge.Recharge_Threshold_Low_Temperature, \
		configs->cfg_charge.Recharge_Threshold_Normal_Temperature, \
		configs->cfg_charge.Recharge_Threshold_High_Temperature);

	LOG_INF("OVP: %d, %dmv, %dmv, %dmv", configs->cfg_charge.Bat_OVP_Enable, \
		configs->cfg_charge.OVP_Voltage_Low_Temperature, \
		configs->cfg_charge.OVP_Voltage_Normal_Temperature, \
		configs->cfg_charge.OVP_Voltage_High_Temperature);	

	LOG_INF("Total_Time_Limit: %dmin", configs->cfg_charge.Charge_Total_Time_Limit);
	LOG_INF("Check_Period: %ds", configs->cfg_charge.Battery_Check_Period_Sec);
	LOG_INF("Charge_Check_Period: %ds", configs->cfg_charge.Charge_Check_Period_Sec);
	LOG_INF("Full_Continue: %ds", configs->cfg_charge.Charge_Full_Continue_Sec);
	LOG_INF("Poff_Wait: %ds", configs->cfg_charge.Front_Charge_Full_Power_Off_Wait_Sec);
	LOG_INF("DC5V_Dbs: %dms", configs->cfg_charge.DC5V_Detect_Debounce_Time_Ms);
	LOG_INF("Bat_Rdrop: %dm", configs->cfg_charge.Battery_Default_RDrop);

	LOG_INF("Bat Config End!");
}


static int wait_special_ms(uint32_t wait_ms)
{
	uint32_t timestamp = k_cycle_get_32();

	while(1) {
		if (k_cyc_to_ms_floor32(k_cycle_get_32() - timestamp) >= wait_ms) {
			break;
		}
	}

	return 0;
}


static void adc_cal_init(void)
{
	unsigned int offset;
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	
	bat_charge->adc_bat_offset = 0;
    #if 1
	if (!soc_atp_get_pmu_calib(4, &offset)){
		LOG_INF("get batadc cal=0x%x\n", offset);
		if(offset & 0x10)
			bat_charge->adc_bat_offset = (offset & 0x0f)*2;
		else
			bat_charge->adc_bat_offset = (-((offset & 0x0f)*2));
	}
    #endif
	LOG_INF("get batadc off=%d\n", bat_charge->adc_bat_offset);
}

static uint16_t pmu_adc_cal(int32_t adcval, int32_t offset)
{
	adcval = adcval + offset;
	return (u16_t)adcval;
}


static int pmuadc_clk_source_set(void)
{
	sys_write32((CONFIG_PMUADC_CLOCK_SOURCE & 0x3) << 0 , CMU_LRADCCLK);
	LOG_INF("LRADCCLK:0x%08x", sys_read32(CMU_LRADCCLK));

	LOG_INF("hosc_ctl:0x%08x", sys_read32(HOSC_CTL));

	return 0;
}

static int pmuadc_reset_and_config(const struct device *dev)
{
#ifdef CONFIG_ACTS_BATTERY_SUPPORT_INIT_CHARGER_NTC
	const struct acts_battery_config *cfg = dev->config;
	bat_charge_context_t *bat_charge = bat_charge_get_context();
#endif

    uint32_t reg = sys_read32(PMUADC_CTL);
	uint32_t reg_disable, reg_enable;

	reg &= ~0x7FFF;
	reg &= ~(3<<10);
	reg |= (CONFIG_PMUADC_IBIAS_BUF_SEL<<10);

	reg &= ~(3<<8);
	reg |= (CONFIG_PMUADC_IBIAS_ADC_SEL<<8);	

	reg_disable = reg;
	reg_enable = reg | ((1<<14) | (1<<2));    //batadc en, pmuadc en

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_INIT_CHARGER_NTC
	if((cfg->ntc_chan >= PMUADC_ID_LRADC1) && (cfg->ntc_chan <= PMUADC_ID_LRADC3)) {
		LOG_INF("need enable NTC channel!");
		reg_enable |= (1<<cfg->ntc_chan);
	}

	//ntc adc channel number
	bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Ctrl &= (~(0xff));
	bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Ctrl |= (cfg->ntc_chan);

	LOG_INF("NTC channel: 0x%x", bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Ctrl);
#endif
	
	//ADC_EN=0
	sys_write32(reg_disable, PMUADC_CTL);
	wait_special_ms(1);
	//ADC_EN=1
	sys_write32(reg_enable, PMUADC_CTL);	

	return 0;
}


static int pmuadc_bias_setting(const struct device *dev)
{
    pmuadc_reset_and_config(dev);

	LOG_INF("PMUADC_CTL:0x%08x", sys_read32(PMUADC_CTL));
	return 0;
}

static void adc_config(const struct device *dev)
{
	pmuadc_clk_source_set();
	acts_clock_peripheral_enable(CLOCK_ID_LRADC);
	
	pmuadc_bias_setting(dev);
}

/* get a battery ADC value direct */
int bat_adc_get_sample_direct(void)
{
	int ret;
	int32_t adc_data;
	bat_charge_context_t *bat_charge = bat_charge_get_context();

	adc_data = sys_read32(BATADC_DATA) & 0x3FF;

	ret = (int)pmu_adc_cal(adc_data, bat_charge->adc_bat_offset);

	LOG_INF("bat_adc: 0x%x, adjust: 0x%x", adc_data, ret);

	return ret;
}


#ifdef CONFIG_ACTS_BATTERY_SUPPORT_INIT_CHARGER_NTC

/* get a NTC ADC value direct */
int bat_ntc_adc_get_direct(void)
{
	int ret;
	uint8_t ntc_channel_num;
	bat_charge_context_t *bat_charge = bat_charge_get_context();

	ntc_channel_num = bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Ctrl & 0xff;
	if((ntc_channel_num > PMUADC_ID_LRADC3) || (ntc_channel_num < PMUADC_ID_LRADC1)) {
		LOG_ERR("NTC channel number err!");
		return -1;
	}
	
	ret = sys_read32(LRADC1_DATA + 4*(ntc_channel_num-PMUADC_ID_LRADC1)) & 0x3FF;

	LOG_INF("ntc_adc: 0x%x", ret);

	return ret;
}
#endif

static void bat_wait_batadc_ok(void)
{
	int ret;
	uint32_t cnt = 0;
	uint32_t bat_mv;

	while(cnt < 100000) {
		ret = bat_adc_get_sample_direct();
		if(ret > 0) {
			bat_mv  = bat_adc_get_voltage_mv(ret);	
			if((bat_mv > BAT_VOLTAGE_SAMPLE_VALID_MIN) && (bat_mv < BAT_VOLTAGE_SAMPLE_VALID_MAX)) {
				LOG_INF("wait batadc ok: %dmv", bat_mv);
				break;
			}
		}
		cnt++;
	}
}

int bat_channel_setup(const struct device *dev)
{
	struct acts_battery_info *bat = dev->data;
	const struct acts_battery_config *cfg = dev->config;
	struct adc_channel_cfg channel_cfg = {0};

	bat->adc_dev = (struct device *)device_get_binding(cfg->adc_name);
	if (!bat->adc_dev) {
		LOG_ERR("cannot found ADC device \'batadc\'\n");
		return -ENODEV;
	}
	
	channel_cfg.channel_id = cfg->batadc_chan;
	LOG_INF("setup batadc channel:%d", channel_cfg.channel_id);
	if (adc_channel_setup(bat->adc_dev, &channel_cfg)) {
		LOG_ERR("batadc channel setup error!");
		return -EFAULT;
	}

	channel_cfg.channel_id = cfg->chargeadc_chan;
	LOG_INF("setup chargei channel:%d", channel_cfg.channel_id);
	if (adc_channel_setup(bat->adc_dev, &channel_cfg)) {
		LOG_ERR("chargei channel setup error!");
		return -EFAULT;
	}

	channel_cfg.channel_id = cfg->dc5v_chan;
	LOG_INF("setup dc5v channel:%d", channel_cfg.channel_id);
	if (adc_channel_setup(bat->adc_dev, &channel_cfg)) {
		LOG_ERR("dc5v channel setup error!");
		return -EFAULT;
	}
	
#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
	
#ifdef CONFIG_CFG_DRV
	uint8_t channel_cfg_id;
	if (bat_ntc_config_init(dev, &channel_cfg_id)) {
		LOG_ERR("NTC config err!");
		return -ENXIO;
	}
	channel_cfg.channel_id = channel_cfg_id;
#else
	channel_cfg.channel_id = cfg->ntc_chan;
#endif
	
	if((channel_cfg.channel_id >= PMUADC_ID_LRADC1) && (channel_cfg.channel_id <= PMUADC_ID_LRADC3)) {
		LOG_INF("setup NTC channel:%d", channel_cfg.channel_id);
		if (adc_channel_setup(bat->adc_dev, &channel_cfg)) {
			LOG_ERR("NTC channel setup error!");
			return -EFAULT;
		}
	}
#endif
	
	bat->bat_sequence.channels = BIT(cfg->batadc_chan);
	bat->bat_sequence.buffer = &bat->bat_adcval;
	bat->bat_sequence.buffer_size = sizeof(bat->bat_adcval);

	bat->charge_sequence.channels = BIT(cfg->chargeadc_chan);
	bat->charge_sequence.buffer = &bat->charge_adcval;
	bat->charge_sequence.buffer_size = sizeof(bat->charge_adcval);
	
	bat->dc5v_sequence.channels = BIT(cfg->dc5v_chan);
	bat->dc5v_sequence.buffer = &bat->dc5v_adcval;
	bat->dc5v_sequence.buffer_size = sizeof(bat->dc5v_adcval);
	
#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
	if((channel_cfg.channel_id >= PMUADC_ID_LRADC1) && (channel_cfg.channel_id <= PMUADC_ID_LRADC3)) {
		bat->ntc_sequence.channels = BIT(channel_cfg.channel_id);
		bat->ntc_sequence.buffer = &bat->ntc_adcval;
		bat->ntc_sequence.buffer_size = sizeof(bat->ntc_adcval);
	}
#endif

	return 0;
}

/* battery and charger function initialization */
static int bat_charge_init(struct device *dev)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_configs_t *configs = &bat_charge->configs;
	int result;

    uint32_t charge_voltage_mv;
	uint16_t bat_mv_temp;

    if (bat_charge->inited) {
		LOG_INF("already inited");
        return 0;
    }

    LOG_INF("bat charge init start!");

	result = bat_channel_setup(dev);
	if(result != 0) {
		LOG_ERR("bat channel setup fail!");
		return -1;
	}

	bat_charge->last_voltage_report = BAT_VOLTAGE_RESERVE;
	bat_charge->last_cap_report = BAT_CAP_RESERVE;
	bat_charge->last_cap_detect = BAT_CAP_RESERVE;

	charge_voltage_mv =
			 (4200 * 10 + (configs->cfg_charge.Charge_Voltage - CHARGE_VOLTAGE_4_20_V) * 125) / 10;

	/* limitation to stop charging voltage */
    LOG_INF("Charge stop Vol: %d mv", configs->cfg_charge.Charge_Stop_Voltage);
    LOG_INF("Charge Vol: %d mv", charge_voltage_mv);
	
    if (configs->cfg_charge.Charge_Stop_Voltage > (charge_voltage_mv - 20)) {
        configs->cfg_charge.Charge_Stop_Voltage = (charge_voltage_mv - 20);
    }

	bat_get_precharge_current();

    //bat_charge->dc5v_debounce_state = DC5V_STATE_INVALID;

    /* do not limit the electric current during initialization */
    bat_charge->limit_current_percent = 100;

	bat_charge->limit_cv_level = configs->cfg_charge.Charge_Voltage;

    /* DC5V detection debounce operations */
	if (configs->cfg_charge.DC5V_Detect_Debounce_Time_Ms > DC5V_DEBOUNCE_MAX_TIME_MS) {
		LOG_INF("DC5V detect debounce time:%d invalid and use default",
				configs->cfg_charge.DC5V_Detect_Debounce_Time_Ms);
		configs->cfg_charge.DC5V_Detect_Debounce_Time_Ms = DC5V_DEBOUNCE_TIME_DEFAULT_MS;
	}

	memset(bat_charge->dc5v_debounce_buf, DC5V_STATE_INVALID, sizeof(bat_charge->dc5v_debounce_buf));

    /* BATADC sample buffer initialization */
	memset(bat_charge->bat_adc_sample_buf, 0, sizeof(bat_charge->bat_adc_sample_buf));
	
	/* chargei sample buffer init */
	bat_chargei_sample_buf_init();

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
	//NTC debounce 
	if (configs->cfg_bat_ntc.NTC_Settings.NTC_Det_Debounce_Time_S > NTC_DEBOUNCE_MAX_TIME_S) {
		LOG_INF("NTC detect debounce time:%d invalid and use default",
				configs->cfg_bat_ntc.NTC_Settings.NTC_Det_Debounce_Time_S);
		configs->cfg_bat_ntc.NTC_Settings.NTC_Det_Debounce_Time_S = NTC_DEBOUNCE_TIME_DEFAULT_S;
	}

	bat_charge->ntc_debounce_state = NTC_TEMPERATURE_INVALID;
	memset(bat_charge->NTC_debounce_buf, NTC_TEMPERATURE_INVALID, sizeof(bat_charge->NTC_debounce_buf));
#endif	

    /* assume that the battery is already existed and will detect later */
    bat_charge->bat_exist_state = 1;

	bat_charge->bat_error_state = 0;

	/* init the battery and charger environment */
    bat_charge_ctrl_init();

    /* check and get the real voltage of battery firstly */
    bat_check_real_voltage(1);

    /* initialize the buffer for sampling battery voltage */
    bat_adc_sample_buf_init();

	bat_mv_temp = (uint16_t)get_battery_voltage_by_time(BAT_VOLT_CHECK_SAMPLE_SEC);

    if (bat_charge->last_saved_bat_volt != 0) {
		if(abs(bat_charge->last_saved_bat_volt - bat_mv_temp) < BAT_INIT_VOLMV_DIFF_MAX) {
        	bat_charge->bat_real_volt = bat_charge->last_saved_bat_volt;
		} else {
			bat_charge->bat_real_volt = bat_mv_temp;
		}
    } else {
        bat_charge->bat_real_volt = bat_mv_temp;
    }

    bat_charge->last_time_check_bat_volt = k_uptime_get_32();

    LOG_INF("battery real voltage init:%dmv", bat_charge->bat_real_volt);

    if (get_dc5v_current_state() != DC5V_STATE_IN) {
        /* check if battery is low power when DC5V not plug-in */
        bat_check_voltage_low(bat_charge->bat_real_volt);
    } else {
		bat_charge->charge_begin_time = k_uptime_get_32();
		LOG_INF("charge begin time: 0x%x", bat_charge->charge_begin_time);
    }

#if (USE_LOW_PRIO_WORK_QUEUE==0)
	k_work_queue_start(p_bat_drv_q, (k_thread_stack_t *)bat_work_stack, BATWORK_STACKSIZE, 8, NULL);
#endif
	k_delayed_work_init(&bat_charge->timer, bat_charge_timer_handler);
	k_delayed_work_submit_to_queue(p_bat_drv_q, &bat_charge->timer, K_MSEC(BAT_CHARGE_DRIVER_TIMER_MS));


    bat_charge->inited = true;
	bat_charge->enabled = 1;

	LOG_INF("bat charge init end!");

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

	if (!bat_charge->inited) {
		bat_charge_init(bat_charge->dev);
		return;
	}

    if (bat_charge->enabled) {
        return;
    }

    bat_charge->dc5v_debounce_state = DC5V_STATE_INVALID;	
    bat_charge->bat_charge_state    = BAT_CHG_STATE_INIT;

    bat_charge->last_saved_bat_volt = 0;
    bat_charge->last_time_check_bat_volt = 0;

    /* DC5V debounce buffer reset */
    memset(bat_charge->dc5v_debounce_buf, DC5V_STATE_INVALID, sizeof(bat_charge->dc5v_debounce_buf));

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
	bat_charge->ntc_debounce_state = NTC_TEMPERATURE_INVALID;
	memset(bat_charge->NTC_debounce_buf, NTC_TEMPERATURE_INVALID, sizeof(bat_charge->NTC_debounce_buf));
#endif

	bat_charge->bak_PMU_CHG_CTL = sys_read32(CHG_CTL_SVCC);

    /* check battery voltage */
    bat_check_real_voltage(1);

    /* initialize sample buffer */
    bat_adc_sample_buf_init();

	/* chargei sample buffer init */
	bat_chargei_sample_buf_init();
	
    bat_charge->state_timer_count = 0;
	bat_charge->enabled = 1;

	//k_delayed_work_submit(&bat_charge->timer, K_MSEC(BAT_CHARGE_DRIVER_TIMER_MS));
	k_delayed_work_submit_to_queue(p_bat_drv_q, &bat_charge->timer, K_MSEC(BAT_CHARGE_DRIVER_TIMER_MS));
}

/* get a DC5V adc value */
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

	return ret;
}

/* get a battery ADC value */
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

	return ret;
}

/* get a chargei ADC value */
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

	return ret;
}

/* check battery voltage when power on stage */
static bool bat_boot_stage_check_voltage(void)
{
	int ret;
	uint32_t bat_mv;
	uint8_t cnt;
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_configs_t *configs = &bat_charge->configs;	

	for(cnt=0;cnt<3;cnt++) {
		ret = bat_adc_get_sample_direct();
		if (ret < 0) {
			return false;
		}
	
		bat_mv	= bat_adc_get_voltage_mv(ret);
		
		if(cnt == 0) {
			LOG_INF("bat_adc: 0x%x, bat_mv: %dmv", ret, bat_mv);
		}
		
		if(bat_mv < configs->cfg_charge.Init_Charge_Vol_1) {
			bat_charge->init_charge_stage = 1;
			return false;
		} else if (bat_mv < configs->cfg_charge.Init_Charge_Vol_2) {
			bat_charge->init_charge_stage = 2;
			return false;			
		} else if (bat_mv < configs->cfg_charge.Init_Charge_Vol_3) {
			if(bat_mv >= 3500) {
				//need stop
				pmu_chg_ctl_reg_write((0x1<<CHG_CTL_SVCC_CV_3V3), (0x1<<CHG_CTL_SVCC_CV_3V3));
				wait_special_ms(200);
				//get ocv
				ret = bat_adc_get_sample_direct();
				bat_mv	= bat_adc_get_voltage_mv(ret);
				LOG_INF("OCV: %dmv", bat_mv);
				if(bat_mv < 3400) {
					//still in stage2
					bat_charge->init_charge_stage = 2;
					pmu_chg_ctl_reg_write((0x1<<CHG_CTL_SVCC_CV_3V3), (0x0<<CHG_CTL_SVCC_CV_3V3));
				} else {
					bat_charge->init_charge_stage = 3;
					pmu_chg_ctl_reg_write((0x1<<CHG_CTL_SVCC_CV_3V3), (0x0<<CHG_CTL_SVCC_CV_3V3));					
				}
			} else {
				//bat_charge->init_charge_stage = 3;
				bat_charge->init_charge_stage = 2;
			}
			return false;			
		} else {
			; //empty
		}
	}

	LOG_INF("Need exit init charge: %dmv, %dmv", bat_mv, configs->cfg_charge.Init_Charge_Vol_3);

	return true;
}


#ifdef CONFIG_ACTS_BATTERY_SUPPORT_INIT_CHARGER_NTC
int deal_for_initcharge_NTC(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_configs_t *cfg = &bat_charge->configs;
	int ntc_adc;
	int i;
	NTC_temp_e ntc_current_state;
    int debounce_buf_size =
        (bat_charge->configs.cfg_bat_ntc.NTC_Settings.NTC_Det_Debounce_Time_S * 1000)/BAT_NTC_SAMPLE_INTERVAL_MS;	

	if(cfg->cfg_bat_ntc.NTC_Settings.Enable_NTC == 0) {
		return 0xff;
	}

	//get temperature adc each 1s
    ntc_adc = bat_ntc_adc_get_direct();
	if(ntc_adc < 0) {
		LOG_ERR("Bat NTC sample fail!");
		return 0xff;
	}

    for (i = 0; i < ARRAY_SIZE(cfg->cfg_bat_ntc.NTC_Ranges); i++) {
        if (cfg->cfg_bat_ntc.NTC_Ranges[i].ADC_Min < cfg->cfg_bat_ntc.NTC_Ranges[i].ADC_Max &&
            ntc_adc >= cfg->cfg_bat_ntc.NTC_Ranges[i].ADC_Min &&
            ntc_adc <= cfg->cfg_bat_ntc.NTC_Ranges[i].ADC_Max) {
            
			ntc_current_state = (NTC_temp_e)i; 

        	if (bat_charge_debounce(bat_charge->NTC_debounce_buf,
                	debounce_buf_size, (uint8_t)ntc_current_state) == false) {
				/* return if data in ntc_debounce_buf are not the same */
            	return 0xff;
        	}

			if(ntc_current_state == bat_charge->ntc_debounce_state) {
				return 0xff;
			}

			LOG_INF("NTC state change, last: %s, new: %s", get_NTC_state_str(bat_charge->ntc_debounce_state), get_NTC_state_str(ntc_current_state));

			bat_charge->ntc_debounce_state = ntc_current_state;

            break;
        }
    }

	return i;
}
#endif

/* porting api , get battery property */
static int battery_acts_get_property(struct device *dev,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	int result = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
			if (!bat_charge->bat_exist_state) {
				val->intval = POWER_SUPPLY_STATUS_BAT_NOTEXIST;
			} else if (bat_charge->bat_error_state != 0) {
				val->intval = POWER_SUPPLY_STATUS_BAT_ERROR;
			} else if ((bat_charge->bat_charge_state == BAT_CHG_STATE_PRECHARGE)
				|| (bat_charge->bat_charge_state == BAT_CHG_STATE_CHARGE)) {
				val->intval = POWER_SUPPLY_STATUS_CHARGING;
			} else if (bat_charge->bat_charge_state == BAT_CHG_STATE_FULL) {
				val->intval = POWER_SUPPLY_STATUS_FULL;
			} else if(soc_pmu_get_dc5v_status() == false) {
				val->intval = POWER_SUPPLY_STATUS_DISCHARGE;
			} else {
				val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
			}
			break;
			
		case POWER_SUPPLY_PROP_ONLINE:
			val->intval = 1;
			break;
		
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			val->intval = get_battery_voltage_mv()*1000;
			break;
		
		case POWER_SUPPLY_PROP_CAPACITY:
			val->intval = get_battery_percent();
			break;
		
		case POWER_SUPPLY_PROP_DC5V:
			val->intval = soc_pmu_get_dc5v_status();
			if((bat_charge->dc5v_debounce_state == DC5V_STATE_OUT) && (val->intval == 0)) {
				//LOG_INF("power supply dc5v: out");
				val->intval = 0;
			} else {
				//LOG_INF("power supply dc5v: in");
				val->intval = 1;
			}
			break;

		case POWER_SUPPLY_PROP_DC5V_STATUS:
		{
#ifdef CONFIG_ACTS_BATTERY_SUPPLY_CHARGER_BOX
			int status = get_dc5v_current_state();
			if (status == DC5V_STATE_OUT) {
				val->intval = POWER_SUPPLY_STATUS_DC5V_OUT;
			} else if (status == DC5V_STATE_IN) {
				val->intval = POWER_SUPPLY_STATUS_DC5V_IN;
			} else if (status == DC5V_STATE_PENDING) {
				val->intval = POWER_SUPPLY_STATUS_DC5V_PENDING;
			} else if (status == DC5V_STATE_STANDBY) {
				val->intval = POWER_SUPPLY_STATUS_DC5V_STANDBY;
			} else {
				val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
			}
			break;
#else

			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
#endif
		}

		case POWER_SUPPLY_PROP_DC5V_VOLTAGE:
			val->intval = dc5v_adc_get_voltage_mv(dc5v_adc_get_sample());
			break;

#if 0
		case POWER_SUPPLY_PROP_ADC:
			val->intval = bat_adc_get_sample();
			break;

		case POWER_SUPPLY_PROP_CURRENT:
			val->intval = chargei_adc_get_current_ma(chargei_adc_get_sample());
			break;
#endif

		default:
			result = -EINVAL;
			break;
	}

	return result;
}

/* porting api , set battery property */
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

		case POWER_SUPPLY_SET_INIT_VOL:
			bat_charge->last_saved_bat_volt = (uint16_t)val->intval;
			LOG_INF("set bat int voltage: %dmv", bat_charge->last_saved_bat_volt);
			break;

#ifdef CONFIG_ACTS_BATTERY_SUPPLY_CHARGER_BOX
		case POWER_SUPPLY_SET_DC5V_PULLDOWM:
			dc5v_pull_down_enable(DC5VPD_CURRENT_2_5_MA, 20);
			break;

		case POWER_SUPPLY_WAKE_CHARGER_BOX:
		{
			int status = dc5v_wake_charger_box();
			if (status == DC5V_STATE_OUT) {
				val->intval = POWER_SUPPLY_STATUS_DC5V_OUT;
			} else if (status == DC5V_STATE_IN) {
				val->intval = POWER_SUPPLY_STATUS_DC5V_IN;
			} else if (status == DC5V_STATE_PENDING) {
				val->intval = POWER_SUPPLY_STATUS_DC5V_PENDING;
			} else if (status == DC5V_STATE_STANDBY) {
				val->intval = POWER_SUPPLY_STATUS_DC5V_STANDBY;
			} else {
				val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
			}
			break;
		}
#endif

		//case POWER_SUPPLY_SET_CONSUME_MA:
		//	bat_charge->bat_charge_system_consume_ma = (uint32_t)val->intval;
		//	bat_charge->use_config_system_consume = 1;
		//	LOG_INF("set consume current: %dma", bat_charge->bat_charge_system_consume_ma);			
		//	break;	

		//case POWER_SUPPLY_SET_BEFORE_ENTER_S4:
		//	bat_charger_set_before_enter_s4();
		//	break;

		default:
			LOG_ERR("invalid psp cmd:%d", psp);
			break;
	}
}

/* porting api , register callback function */
static void battery_acts_register_notify(struct device *dev, bat_charge_callback_t cb)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	int flag;

	flag = irq_lock();
	if ((bat_charge->callback == NULL) && cb) {
		bat_charge->callback = cb;
	} else {
		LOG_ERR("notify func already exist!\n");
	}
	irq_unlock(flag);
}

/* porting api , enable battery charge and status monitor */
static void battery_acts_enable(struct device *dev)
{
	bat_charge_resume();
}

/* porting api , disable battery charge and status monitor */
static void battery_acts_disable(struct device *dev)
{
	bat_charge_suspend();
}

static const struct power_supply_driver_api battery_acts_driver_api = {
	.get_property = battery_acts_get_property,
	.set_property = battery_acts_set_property,
	.register_notify = battery_acts_register_notify,
	.enable = battery_acts_enable,
	.disable = battery_acts_disable,
};


/* battery driver init */
static int battery_acts_init(const struct device *dev)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_configs_t *configs = &bat_charge->configs;

	uint32_t bat_mv;
	uint32_t total_ms;
	uint32_t cnt;
	uint32_t dc5v_out_cnt;
	int ret;
#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC	
	uint8_t channel_cfg_id;
#endif

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_INIT_CHARGER_NTC
	int ntc_new_state;
	uint8_t ntc_current_index = 0xff;
#endif	
	
	memset(bat_charge, 0, sizeof(bat_charge_context_t));

	bat_charge->dev = (struct device *)dev;

	bat_charge->bak_PMU_CHG_CTL = sys_read32(CHG_CTL_SVCC);

	bat_boot_open_charger();

	LOG_INF("bat drv get config!");
	
#ifdef CONFIG_CFG_DRV
	ret = bat_charge_config_init();
	if (ret) {
		LOG_ERR("config init error:%d", ret);
		return -ENOENT;
	}

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC	
	if (bat_ntc_config_init(dev, &channel_cfg_id)) {
		LOG_ERR("NTC config err!");
		return -ENXIO;
	}

	LOG_INF("NTC channel id: %d\n", channel_cfg_id);
#endif	

#else
	CFG_Struct_Battery_Charge cfg_charge = BAT_CHARGE_CFG_DEFAULT;
	CFG_Struct_Battery_Level cfg_level = BAT_LEVEL_CFG_DEFAULT;
	CFG_Struct_Battery_Low cfg_low = BAT_LOW_CFG_DEFAULT;
#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
	CFG_Type_NTC_Settings cfg_ntc_setting = BAT_NTC_SETTING_DEFAULT;
	CFG_Type_NTC_Range cfg_ntc_range0 = BAT_NTC_RANGE_0;
	CFG_Type_NTC_Range cfg_ntc_range1 = BAT_NTC_RANGE_1;
	CFG_Type_NTC_Range cfg_ntc_range2 = BAT_NTC_RANGE_2;
	CFG_Type_NTC_Range cfg_ntc_range3 = BAT_NTC_RANGE_3;
	CFG_Type_NTC_Range cfg_ntc_range4 = BAT_NTC_RANGE_4;
#endif
	
	memcpy(&configs->cfg_charge, &cfg_charge, sizeof(CFG_Struct_Battery_Charge));
	memcpy(&configs->cfg_bat_level, &cfg_level, sizeof(CFG_Struct_Battery_Level));
	memcpy(&configs->cfg_bat_low, &cfg_low, sizeof(CFG_Struct_Battery_Low));

	dump_bat_cfg_info();
	
#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
	memcpy(&configs->cfg_bat_ntc.NTC_Settings, &cfg_ntc_setting, sizeof(CFG_Type_NTC_Settings));
	memcpy(&configs->cfg_bat_ntc.NTC_Ranges[0], &cfg_ntc_range0, sizeof(CFG_Type_NTC_Range));
	memcpy(&configs->cfg_bat_ntc.NTC_Ranges[1], &cfg_ntc_range1, sizeof(CFG_Type_NTC_Range));
	memcpy(&configs->cfg_bat_ntc.NTC_Ranges[2], &cfg_ntc_range2, sizeof(CFG_Type_NTC_Range));
	memcpy(&configs->cfg_bat_ntc.NTC_Ranges[3], &cfg_ntc_range3, sizeof(CFG_Type_NTC_Range));
	memcpy(&configs->cfg_bat_ntc.NTC_Ranges[4], &cfg_ntc_range4, sizeof(CFG_Type_NTC_Range));
	
	dump_ntc_cfg_info();
	
	bat_charge->ntc_debounce_state = NTC_TEMPERATURE_INVALID;
#endif
	configs->cfg_features = SYS_FORCE_CHARGE_WHEN_DC5V_IN;
#endif

	adc_cal_init();
	
	adc_config(dev);

	bat_wait_batadc_ok();

	if(!soc_pmu_get_dc5v_status()) {
		//no dc5v, check bat adc
		LOG_INF("power on, no dc5v!");
		for(cnt=0; cnt<3; cnt++) {
			ret = bat_adc_get_sample_direct();
			if (ret < 0) {
				return -EFAULT;
			}
	
    		bat_mv  = bat_adc_get_voltage_mv(ret);
			LOG_INF("%d, bat_mv: %d", cnt, bat_mv);
			if(bat_mv < CONFIG_ACTS_BATTERY_POWERON_MIN_VOL_WITHOUT_DC5V) {
				//power off
				LOG_INF("bat too low, poweroff: %d, %d", CONFIG_ACTS_BATTERY_POWERON_MIN_VOL_WITHOUT_DC5V, bat_mv);
				//sys_pm_poweroff need do devices power off, many device is not init now
				while(1) {
					sys_write32(0, POWER_CTL_SVCC);
				}
			}
			bat_charge->dc5v_debounce_state = DC5V_STATE_OUT;
		}
	} else {
		LOG_INF("power on, with dc5v!");
		
		if(configs->cfg_charge.Init_Charge_Enable == YES) {
			LOG_INF("init charge start!");
			bat_get_initcharge_current();

			bat_charge->init_charge_stage_last = 0;
			bat_charge->init_charge_stage = 0;

			total_ms = 0;
			dc5v_out_cnt = 0;
			
#ifdef CONFIG_ACTS_BATTERY_SUPPORT_INIT_CHARGER_NTC
			//NTC debounce 
			if (configs->cfg_bat_ntc.NTC_Settings.NTC_Det_Debounce_Time_S > NTC_DEBOUNCE_MAX_TIME_S) {
				LOG_INF("NTC detect debounce time:%d invalid and use default",
								configs->cfg_bat_ntc.NTC_Settings.NTC_Det_Debounce_Time_S);
				configs->cfg_bat_ntc.NTC_Settings.NTC_Det_Debounce_Time_S = NTC_DEBOUNCE_TIME_DEFAULT_S;
			}
			
			bat_charge->ntc_debounce_state = NTC_TEMPERATURE_INVALID;
			memset(bat_charge->NTC_debounce_buf, NTC_TEMPERATURE_INVALID, sizeof(bat_charge->NTC_debounce_buf));
#endif				
			
			//init charge, without stop
			while(1) {
				if(bat_boot_stage_check_voltage()) {
					LOG_INF("exit init charge!");
					break;
				}

				watchdog_clear();
				
#ifndef CONFIG_ACTS_BATTERY_SUPPORT_INIT_CHARGER_NTC
				if(bat_charge->init_charge_stage != bat_charge->init_charge_stage_last) {
					if(bat_charge->init_charge_stage_last < bat_charge->init_charge_stage) {
						LOG_INF("enter init charge stage: %d", bat_charge->init_charge_stage);
						bat_init_charge_set(0xff);
						bat_charge->init_charge_stage_last = bat_charge->init_charge_stage;
					} else {
						LOG_ERR("init charge vol can not decrease!");
					}
				}
#else
				if((bat_charge->init_charge_stage != bat_charge->init_charge_stage_last) || \
						(ntc_current_index != 0xff)) {
					LOG_INF("enter init charge stage: %d", bat_charge->init_charge_stage);
					bat_init_charge_set(ntc_current_index);
				
					bat_charge->init_charge_stage_last = bat_charge->init_charge_stage;
				}
#endif
								
#ifdef CONFIG_ACTS_BATTERY_SUPPORT_INIT_CHARGER_NTC
				ntc_new_state = deal_for_initcharge_NTC();
				if(ntc_new_state != 0xff) {
					ntc_current_index = 4 - ntc_new_state;
				} else {
					ntc_current_index = 0xff;
				}
#endif			

				wait_special_ms(BAT_INIT_CHARGE_INTERVAL_MS);

				total_ms += BAT_INIT_CHARGE_INTERVAL_MS;

                if((total_ms%(5*BAT_INIT_CHARGE_INTERVAL_MS)) == 0) {
					LOG_INF("Total_ms: %d", total_ms);
                }

				if(total_ms > 3600000) {  // 1 hour
					LOG_INF("init charge too long, reboot!");
					sys_write32(0x5f, WD_CTL);
					while (1) {
						;
					}
				}
				if(!soc_pmu_get_dc5v_status()) {
					dc5v_out_cnt++;
					if(dc5v_out_cnt >= 2) {
						LOG_INF("dc5v out in init charge, power off!");
						while(1) {
							sys_write32(0, POWER_CTL_SVCC);
						}						
					}
				} else {
					dc5v_out_cnt = 0;
				}
			}
		}
		
		bat_charge->dc5v_debounce_state = DC5V_STATE_IN;
	}
	
	LOG_INF("bat driver init ok,so enable peripheral_power!\r\n");
	
	return 0;
}

static struct acts_battery_info battery_acts_ddata;

static const struct acts_battery_config battery_acts_cdata = {
	.adc_name = CONFIG_PMUADC_NAME,
	.batadc_chan = PMUADC_ID_BATV,
	.chargeadc_chan = PMUADC_ID_CHARGI,
    .dc5v_chan = PMUADC_ID_DC5V,
#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
    .ntc_chan = PMUADC_ID_LRADC2,
#endif
	.debug_interval_sec = CONFIG_BATTERY_DEBUG_INTERVAL_SEC,
};

#if IS_ENABLED(CONFIG_ACTS_BATTERY)
DEVICE_DEFINE(battery, CONFIG_ACTS_BATTERY_DEV_NAME, battery_acts_init, NULL,
	    &battery_acts_ddata, &battery_acts_cdata, PRE_KERNEL_2,
	    1, &battery_acts_driver_api);
#endif
