/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions Cuckoo family PMU public APIs
 */

#ifndef _SOC_PMU_H_
#define _SOC_PMU_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Macro of peripheral devices that PMU supervise the devices voltage level change.*/
#define PMU_DETECT_DEV_DC5V			(1)
#define PMU_DETECT_DEV_REMOTE		(2)
#define PMU_DETECT_DEV_ONOFF		(3)
#define PMU_DETECT_MAX_DEV			(3)

/* Max milliseconds of counter8hz */
#define PMU_COUTNER8HZ_MAX_MSEC     (8388607875) /* 0x3FFFFFF x 125 */

/**
 * enum pmu_hotplug_state
 * @brief Hotplug devices (e.g. DV5V) state change which detected by PMU.
 */
enum pmu_notify_state {
	PMU_NOTIFY_STATE_OUT = 0,
	PMU_NOTIFY_STATE_IN,
	PMU_NOTIFY_STATE_PRESSED,
	PMU_NOTIFY_STATE_LONG_PRESSED,
	PMU_NOTIFY_STATE_TIME_ON
};

typedef void (*pmu_notify_t)(void *cb_data, int state);

/**
 * struct notify_param_t
 * @brief Parameters for PMU notify register.
 */
struct detect_param_t {
	uint8_t detect_dev;
	pmu_notify_t notify;
	void *cb_data;
};

/* @brief Register the device that PMU start to monitor */
int soc_pmu_register_notify(struct detect_param_t *param);

/* @brief Unregister the device that PMU stop to monitor */
void soc_pmu_unregister_notify(uint8_t detect_dev);

/* @brief Get the wakeup source caused by system power up */
uint32_t soc_pmu_get_wakeup_source(void);

/* @brief return the wakeup setting by system startup */
uint32_t soc_pmu_get_wakeup_setting(void);


/* @brief get the current DC5V status of plug in or out  and if retval of status is 1 indicates that plug-in*/
bool soc_pmu_get_dc5v_status(void);

/* @brief lock DC5V charging for reading battery voltage */
void soc_pmu_read_bat_lock(void);

/* @brief unlock and restart DC5V charging */
void soc_pmu_read_bat_unlock(void);

/* @brief configure the long press on-off key time */
void soc_pmu_config_onoffkey_function(uint8_t val);

/* @brief configure the long press on-off key time */
void soc_pmu_config_onoffkey_time(uint8_t val);

/* @brief check if the onoff key has been pressed or not */
bool soc_pmu_is_onoff_key_pressed(void);

/* @brief set const voltage value */
void soc_pmu_set_const_voltage(uint8_t cv);

/* @brief set the max constant current */
void soc_pmu_set_max_current(uint16_t cur_ma);

/* @brief get the max constant current */
uint16_t soc_pmu_get_max_current(void);

/* @brief configure the long press on-off key reset/restart time */
void soc_pmu_config_onoffkey_reset_time(uint8_t val);

/* @brief configure the  on-off key pullr_sel  */
void soc_pmu_config_onoffkey_pullr_sel(uint8_t val);

/* @brief configure the  on-off key function source sel  */
void soc_pmu_config_onoffkey_source_sel(uint8_t val);

/* @brief get calibrate mul rc32k by hosc return hosc/rc32k */
uint32_t soc_rc32K_mutiple_hosc(void);

/* @brief get S1 VDD voltage  */
uint32_t soc_pmu_get_vdd_voltage(void);

/* @brief set S1 VDD voltage  */
void soc_pmu_set_vdd_voltage(uint32_t volt_mv);


#ifdef __cplusplus
}
#endif

#endif /* _SOC_PMU_H_ */
