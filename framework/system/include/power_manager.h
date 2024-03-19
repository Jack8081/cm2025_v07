/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file power manager interface
 */

#ifndef __POWER_MANAGER_H__
#define __POWER_MANAGER_H__
#include <drivers/power_supply.h>

/**
 * @defgroup power_manager_apis App power Manager APIs
 * @ingroup system_apis
 * @{
 */

/** battary state enum */
enum bat_state_e {
	/** no battery detacted*/
	BAT_STATUS_UNKNOWN = POWER_SUPPLY_STATUS_UNKNOWN,
	/** battery not exist */
	BAT_STATUS_NOTEXIST = POWER_SUPPLY_STATUS_BAT_NOTEXIST,
	/** battery discharge */
	BAT_STATUS_DISCHARGE = POWER_SUPPLY_STATUS_DISCHARGE,
	/** battery charging */
	BAT_STATUS_CHARGING = POWER_SUPPLY_STATUS_CHARGING,
	/** battery charged full */
	BAT_STATUS_FULL = POWER_SUPPLY_STATUS_FULL,
};
enum  dc5v_status_e{
	/** dc5v plug out */
	DC5V_STATUS_OUT = POWER_SUPPLY_STATUS_DC5V_OUT,
	/** dc5v plug in */
	DC5V_STATUS_IN = POWER_SUPPLY_STATUS_DC5V_IN,
	/** dc5v pending */
	DC5V_STATUS_PENDING = POWER_SUPPLY_STATUS_DC5V_PENDING,
	/** dc5v standby */
	DC5V_STATUS_STANDBY = POWER_SUPPLY_STATUS_DC5V_STANDBY
};

/** battery charge event enum */
typedef enum
{
	/** dc5v plug in event */
	POWER_EVENT_DC5V_IN = BAT_CHG_EVENT_DC5V_IN,
	/** dc5v plug out event */
	POWER_EVENT_DC5V_OUT = BAT_CHG_EVENT_DC5V_OUT,
	/** charger box enter standby */
	POWER_EVENT_DC5V_STANDBY = BAT_CHG_EVENT_DC5V_STANDBY,

	/** start charging */
	POWER_EVENT_CHARGE_START = BAT_CHG_EVENT_CHARGE_START,
	/** stop charging */
	POWER_EVENT_CHARGE_STOP = BAT_CHG_EVENT_CHARGE_STOP,
	/** charge full event */
	POWER_EVENT_CHARGE_FULL = BAT_CHG_EVENT_CHARGE_FULL,

	/** battery voltage changed */
	POWER_EVENT_VOLTAGE_CHANGE = BAT_CHG_EVENT_VOLTAGE_CHANGE,
	/** battery capacity changed */
	POWER_EVENT_CAP_CHANGE = BAT_CHG_EVENT_CAP_CHANGE,		

	/** battery power normal */
	POWER_EVENT_BATTERY_NORMAL = BAT_CHG_EVENT_BATTERY_NORMAL,
	/** battery power medium */
	POWER_EVENT_BATTERY_MEDIUM = BAT_CHG_EVENT_BATTERY_MEDIUM,
	/** battery power low */
	POWER_EVENT_BATTERY_LOW = BAT_CHG_EVENT_BATTERY_LOW,
	/** battery power lower */
	POWER_EVENT_BATTERY_LOW_EX = BAT_CHG_EVENT_BATTERY_LOW_EX,
	/** battery power too low */
	POWER_EVENT_BATTERY_TOO_LOW = BAT_CHG_EVENT_BATTERY_TOO_LOW,
	/** battery power full */
	POWER_EVENT_BATTERY_FULL = BAT_CHG_EVENT_BATTERY_FULL,		
} power_event_t;

/**
 * @brief get system battery capacity
 *
 *
 * @return battery capacity , The unit is the percentage
 */
int power_manager_get_battery_capacity(void);


/**
 * @brief check if is no power
 *
 *
 */
bool power_manager_check_is_no_power(void);

/**
 * @brief check battry if is full
 *
 *
 */
bool power_manager_check_bat_is_full(void);

/**
 * @brief get local system battery capacity
 *
 *
 * @return battery capacity , The unit is the percentage
 */
int power_manager_get_battery_capacity_local(void);

/**
 * @brief check battry if is exit
 *
 *
 */
bool power_manager_check_bat_exist(void);

/**
 * @brief get system battery vol
 *
 *
 * @return battery vol , The unit is mv
 */
int power_manager_get_battery_vol(void);

/**
 * @brief get system charge status
 *
 *
 * @return charget status, POWER_SUPPLY_STATUS_DISCHARGE, POWER_SUPPLY_STATUS_CHARGING... see power_supply_status
 */

int power_manager_get_charge_status(void);


/**
 * @brief get system dc5v status
 *
 *
 * @return dc5v status, 1 dc5v exist  0 dc5v not exit.
 */

int power_manager_get_dc5v_status(void);

/**
 * @brief get system dc5v voltage
 *
 *
 * @return dc5v voltage.
 */
int power_manager_get_dc5v_voltage(void);

/**
 * @brief get chargebox status
 *
 *
 * @return chargebox status.
 */
int power_manager_get_dc5v_status_chargebox(void);

/**
 * @brief control dc5v pull down
 *
 *
 * @return 0 success.
 */
int power_manger_set_dc5v_pulldown(void);

/**
 * @brief wake charger box
 *
 *
 * @return 0 success.
 */
int power_manger_wake_charger_box(void);

/**
 * @brief register system charge status changed callback
 *
 *
 */
int power_manager_init(void);


int power_manager_set_slave_battery_state(int capacity, int vol);
int power_manager_sync_slave_battery_state(void);

/**
 * @} end defgroup power_manager_apis
 */

#endif
