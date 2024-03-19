/*
 * Copyright (c) 2021  Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BAT_CHARGE_PRIVATE_H__
#define __BAT_CHARGE_PRIVATE_H__

#include <kernel.h>
#include <config.h>
#include <soc.h>
#include <drivers/adc.h>
#include <drivers/power_supply.h>

#ifdef CONFIG_CFG_DRV
#include <drivers/cfg_drv/driver_config.h>
#endif

#define CHG_CTL_SVCC_DC5VPD_PWM                  (24)
#define CHG_CTL_SVCC_DC5VPD_EN                   (23)
#define CHG_CTL_SVCC_DC5VPD_SET_SHIFT            (21)
#define CHG_CTL_SVCC_DC5VPD_SET_MASK             (0x3 << CHG_CTL_SVCC_DC5VPD_SET_SHIFT)
#define CHG_CTL_SVCC_CV_3V3                      (19)
#define CHG_CTL_SVCC_CHGOP_SEL                   (18)
#define CHG_CTL_SVCC_BAT_PD_SHIFT                (16)
#define CHG_CTL_SVCC_BAT_PD_MASK                 (0x3 << CHG_CTL_SVCC_BAT_PD_SHIFT)
#define CHG_CTL_SVCC_CC_OFFSET_SHIFT             (11)
#define CHG_CTL_SVCC_CC_OFFSET_MASK              (0x1F << CHG_CTL_SVCC_CC_OFFSET_SHIFT)
#define CHG_CTL_SVCC_CHG_EN                      (10)
#define CHG_CTL_SVCC_CV_OFFSET_SHIFT             (4)
#define CHG_CTL_SVCC_CV_OFFSET_MASK              (0x3F << CHG_CTL_SVCC_CV_OFFSET_SHIFT)
#define CHG_CTL_SVCC_CHG_CURRENT_SHIFT           (0)
#define CHG_CTL_SVCC_CHG_CURRENT_MASK            (0xF << CHG_CTL_SVCC_CHG_CURRENT_SHIFT)

#define BDG_CTL_SVCC_CHARGEI_SET_SHIFT           (6)
#define BDG_CTL_SVCC_CHARGEI_SET_MASK            (0x1F << BDG_CTL_SVCC_CHARGEI_SET_SHIFT)

#define BAT_CHARGE_DRIVER_TIMER_MS               (50)

#define BAT_VOLT_CHECK_SAMPLE_SEC                (3)

#define BAT_ADC_SAMPLE_INTERVAL_MS               (100)
#define BAT_ADC_SAMPLE_BUF_TIME_SEC              BAT_VOLT_CHECK_SAMPLE_SEC
#define BAT_ADC_SAMPLE_BUF_SIZE                  (BAT_ADC_SAMPLE_BUF_TIME_SEC * 1000 / BAT_ADC_SAMPLE_INTERVAL_MS)

#define DC5V_DEBOUNCE_BUF_MAX                    (20)

#define DC5V_DEBOUNCE_MAX_TIME_MS                (DC5V_DEBOUNCE_BUF_MAX * BAT_CHARGE_DRIVER_TIMER_MS)

#define DC5V_DEBOUNCE_TIME_DEFAULT_MS            (300)

#define BAT_VOLTAGE_RESERVE                      (0x1fff)
#define BAT_CAP_RESERVE                          (101)
#define INDEX_VOL                                (0)
#define INDEX_CAP                                (INDEX_VOL + 1)
#define BAT_VOL_LSB_MV                           (3)

/* enumation for DC5V state  */
typedef enum
{
	DC5V_STATE_INVALID = 0xff,	/* invalid state */
	DC5V_STATE_OUT     = 0,		/* DC5V plug out */
	DC5V_STATE_IN      = 1,		/* DC5V plug in */
	DC5V_STATE_PENDING = 2,		/* DC5V pending */
	DC5V_STATE_STANDBY = 3,		/* DC5V standby */
} dc5v_state_t;

enum BAT_CHARGE_STATE {
	BAT_CHG_STATE_INIT = 0,   /* initial state */
	BAT_CHG_STATE_LOW,        /* low power state */
	BAT_CHG_STATE_PRECHARGE,  /* pre-charging state */
	BAT_CHG_STATE_NORMAL,     /* normal state */
	BAT_CHG_STATE_CHARGE,     /* charging state */
	BAT_CHG_STATE_FULL,       /* full power state */
};

struct acts_battery_info {
	struct device *adc_dev;
	struct adc_sequence bat_sequence;
	struct adc_sequence charge_sequence;
	struct adc_sequence dc5v_sequence;
	uint8_t bat_adcval[2];
	uint8_t charge_adcval[2];
	uint8_t dc5v_adcval[2];
	uint32_t timestamp;
};

struct acts_battery_config {
	char *adc_name;
	uint8_t batadc_chan;
	uint8_t chargeadc_chan;
	uint8_t dc5v_chan;
	uint16_t debug_interval_sec;
};

typedef struct {
	uint8_t last_state;
	struct k_delayed_work charger_enable_timer;
	struct k_delayed_work state_timer;
	struct k_delayed_work state_timer_ex;
} dc5v_check_status_t;

typedef struct {
	uint8_t buf[5];
	uint8_t count;
	uint8_t index;
	s8_t  percent;
} bat_charge_current_percent_buf_t;

typedef struct {
	uint8_t buf[10];
	uint8_t count;
	uint8_t index;
} bat_charge_cv_state_buf_t;

typedef struct {
	uint8_t is_valid;
	uint8_t cv_offset;
	uint8_t adjust_end;
	int last_volt;
} bat_charge_adjust_cv_offset_t;

typedef struct {
	uint8_t is_valid;
	uint8_t charge_current;
	uint8_t stop_current_percent;
} bat_charge_adjust_current_t;

typedef struct {
	CFG_Struct_Battery_Charge cfg_charge;	/* battery charge configuration */
	CFG_Struct_Battery_Level cfg_bat_level;	/* battery quantity level */
	CFG_Struct_Battery_Low cfg_bat_low;		/* battery low power configuration */
	CFG_Struct_Charger_Box cfg_charger_box;	/* charger box configuration */
	uint16_t cfg_features;					/* CFG_TYPE_SYS_SUPPORT_FEATURES*/
} bat_charge_configs_t;

#ifndef CONFIG_CFG_DRV
#define BAT_CHARGE_CONFIG_DEFAULT { \
	{BAT_BACK_CHARGE_MODE, CHARGE_CURRENT_250_MA, CHARGE_VOLTAGE_4_20_V, CHARGE_STOP_BY_VOLTAGE_AND_CURRENT, \
	4200, CHARGE_STOP_CURRENT_5_MA, PRECHARGE_STOP_3_3_V, 60, 60, 120, 600, 400}, \
	{2800, 2966, 3132, 3298, 3464, 3630, 3796, 3962, 4128, 4300}, \
	{3000, 3300, 3300, 60}, \
	{0, DC5VPD_CURRENT_2_5_MA, 2000, 2000, 3800, 1000, 0, 0}, \
	SYS_ENABLE_DC5VPD_WHEN_DETECT_OUT | SYS_FORCE_CHARGE_WHEN_DC5V_IN, \
	}
#endif

struct report_deb_ctr {
	uint16_t rise;
	uint16_t decline;
	uint16_t times;
	uint8_t step;
};

typedef struct {
	struct device *dev;

	bat_charge_configs_t configs;

	struct k_delayed_work timer;

	uint8_t dc5v_debounce_buf[DC5V_DEBOUNCE_BUF_MAX];
	uint16_t bat_adc_sample_buf[BAT_ADC_SAMPLE_BUF_SIZE];

	uint8_t inited                   : 1;
	uint8_t charge_state_started     : 1;
	uint8_t charge_ctrl_enabled      : 1;
	uint8_t need_check_bat_real_volt : 1;
	uint8_t charge_near_full         : 1;
	uint8_t bat_volt_low             : 1;
	uint8_t bat_volt_low_ex          : 1;
	uint8_t bat_volt_too_low         : 1;
	uint8_t bat_exist_state          : 1;
	uint8_t dc5v_state_exit          : 1;
	uint8_t dc5v_state_ex_exit       : 1;
	uint8_t dc5v_state_pending 		 : 1;
	uint8_t dc5v_state_ex_pending    : 1;
	uint8_t enabled                  : 1;

	uint8_t dc5v_debounce_state;
	uint8_t bat_full_dc5v_last_state;
	uint8_t bat_charge_state;

	uint8_t bat_adc_sample_timer_count;
	uint8_t bat_adc_sample_index;

	uint16_t state_timer_count;
	uint16_t precharge_time_sec;

	uint16_t bat_volt_check_wait_count;
	uint16_t bat_real_volt;

	bat_charge_callback_t callback;

	dc5v_check_status_t dc5v_check_status;
	bat_charge_current_percent_buf_t charge_current_percent_buf;
	bat_charge_cv_state_buf_t charge_cv_state_buf;
	bat_charge_adjust_cv_offset_t adjust_cv_offset;

	struct k_delayed_work dc5v_pd_timer;

	bat_charge_adjust_current_t adjust_current;

	uint32_t cv_stage_1_begin_time;  /* in constant voltage stage1 (current level less than 50%)  */
	uint32_t cv_stage_2_begin_time;  /* in constant voltage stage2 (current level less than 20%)  */

	uint32_t dc5v_state_pending_time; /* DC5V state pending time */

	uint32_t bak_PMU_CHG_CTL; /* backup PMU_CHG_CTL as SVCC domain which take low effect */

	struct report_deb_ctr report_ctr[2];

	uint8_t last_cap_report; /* record last battery capacity report */
	uint16_t last_voltage_report; /* record last battery voltage report */
} bat_charge_context_t;

typedef struct {
	CFG_Type_Battery_Charge_Settings_Ex cfg_ex;
	uint8_t charge_current_sel;
	uint8_t charge_enable_pending;
	uint8_t limit_current_percent;
	uint16_t last_dc5v_mv;
} bat_charge_context_ex_t;

extern bat_charge_context_ex_t  bat_charge_context_ex;

static inline bat_charge_context_t *bat_charge_get_context(void)
{
    extern bat_charge_context_t bat_charge_context;

    return &bat_charge_context;
}

static inline bat_charge_context_ex_t *bat_charge_get_context_ex(void)
{
    extern bat_charge_context_ex_t bat_charge_context_ex;

    return &bat_charge_context_ex;
}

static inline uint32_t bat_get_diff_time(uint32_t end_time, uint32_t begin_time)
{
    uint32_t diff_time;

    if (end_time >= begin_time) {
        diff_time = (end_time - begin_time);
    } else {
        diff_time = ((uint32_t)-1 - begin_time + end_time + 1);
    }

    return diff_time;
}

/* get the battery average voltage in mv during by specified seconds */
uint32_t get_battery_voltage_by_time(uint32_t sec);

/* enable battery charging function */
void bat_charge_ctrl_enable(uint32_t current_sel, uint32_t voltage_sel);

/* check battery charging status */
void bat_charge_check_enable(void);

/* disable battery charging function */
void bat_charge_ctrl_disable(void);

/* set the time in seconds to sample the real battery voltage */
void bat_check_real_voltage(uint32_t sample_sec);

/* check and report the battery voltage changed infomation */
uint32_t bat_check_voltage_change(uint32_t bat_volt);

/* process according to the battery change status */
void bat_charge_status_proc(void);

/* charging current percent buffer initialization */
void bat_charge_current_percent_buf_init(void);

/* get the charging current precent value */
int get_charge_current_percent(void);

int get_charge_current_percent_ex(int percent);

/* buffer for constant voltage state initialization */
void bat_charge_cv_state_buf_init(void);

/* get the state of constant voltage */
int bat_charge_get_cv_state(void);

/* adjust the constant voltage value */
void bat_charge_adjust_cv_offset(void);

/* adjust the constant current value */
bool bat_charge_adjust_current(void);

/* get the DC5V state by string format */
const char *get_dc5v_state_str(uint8_t state);

/* get the charge current in milliamps */
int bat_charge_get_current_ma(int level);

/* get the charge current level */
int bat_charge_get_current_level(int ma);

/* battery and charger initialization */
void bat_charge_ctrl_init(void);

/* prevent the charger box to enter standby */
void dc5v_prevent_charger_box_standby(bool pull_down);

/* adjust bandgap */
void sys_ctrl_adjust_bandgap(uint8_t adjust);

/* sample a DC5V ADC value */
int dc5v_adc_get_sample(void);

/* sample a battery ADC value */
int bat_adc_get_sample(void);

/* sample a chargei ADC value */
int chargei_adc_get_sample(void);

/* get the current DC5V state */
dc5v_state_t get_dc5v_current_state(void);

/* get the configration of battery and charger */
bat_charge_configs_t *bat_charge_get_configs(void);

/* convert from DC5V ADC value to voltage in millivolt */
uint32_t dc5v_adc_get_voltage_mv(uint32_t adc_val);

/* convert from battery ADC value to voltage in millivolt */
uint32_t bat_adc_get_voltage_mv(uint32_t adc_val);

/* get the chargei converts to current in milliampere */
uint32_t chargei_adc_get_current_ma(int adc_val);

/* enable DC5V pulldown and will last specified time in miliseconds to disable */
void dc5v_pull_down_enable(uint32_t current, uint32_t timer_ms);

/* get the percent of battery capacity  */
uint32_t get_battery_percent(void);

#endif /* __BAT_CHARGE_PRIVATE_H__ */

