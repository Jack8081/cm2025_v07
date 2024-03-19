/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <board_cfg.h>
#include <drivers/audio/audio_common.h>
#include <soc.h>

/*Configure GPIO high resistance before sleep and restore GPIO after wakeup */
#define SLEEP_GPIO_REG_SET_HIGHZ      \
	GPION_CTL(5), /*lcd backlight enable*/			\
	/*sensor*/ \
	/*//GPION_CTL(18), not use defaut highz*/ 		\
	GPION_CTL(19), /*EN_NTC. user in sleep*/ 		\
	/*//GPION_CTL(20), not use defaut highz*/ 		\
	GPION_CTL(21), /*sensor irq ,use in sleep*/ 	\
	/*//GPION_CTL(24),  HR_PWR_EN ,use in sleep*/	\
	/*GPION_CTL(25),VDD1.8 eanble ,use in sleep*/	\
	GPION_CTL(33), /*GPS wake up Host ,use in sleep*/	\
	/*TP*/ \
	/*//GPION_CTL(26), not use defaut highz*/ 		\
	/* //GPION_CTL(27), not use£¬defaut highz*/ 		\
	GPION_CTL(32), /*tp irq*/				\
	/*I2CMT1*/ 											\
	GPION_CTL(51), /*use in sleep*/ \
	GPION_CTL(52), /*use in sleep*/ \
	/*i2c0*/  \
	GPION_CTL(57), /*not use in sleep*/  \
	GPION_CTL(58), /*not use in sleep*/  \
	/*i2c1*/  \
	GPION_CTL(59), /*not use in sleep*/  \
	GPION_CTL(60), /*not use in sleep*/	 \
	GPION_CTL(62), /*sensor GPS_PPS , not use in sleep*/ \
	GPION_CTL(63), /*sensor irq , not use in sleep*/  
	

#if IS_ENABLED(CONFIG_GPIOKEY)
	/* The GPIO MFP for GPIO KEY */
#if CONFIG_GPIOKEY_PRESSED_VOLTAGE_LEVEL
#define CONFIG_GPIOKEY_MFP    PIN_MFP_SET(GPIO_21,   GPIOKEY_MFP_CFG)
#else
#define CONFIG_GPIOKEY_MFP    PIN_MFP_SET(GPIO_21,   GPIOKEY_MFP_PU_CFG)
#endif
#endif

#define CONFIG_EAR_ISR_GPIO		GPIO_CFG_MAKE(CONFIG_GPIO_A_NAME, 22, GPIO_ACTIVE_LOW, 1)

#define BOARD_BATTERY_CAP_MAPPINGS      \
	{0, 3200},  \
	{5, 3300},  \
	{10, 3400}, \
	{20, 3550}, \
	{30, 3650}, \
	{40, 3750}, \
	{50, 3800}, \
	{60, 3850}, \
	{70, 3900}, \
	{80, 3950}, \
	{90, 4000}, \
	{100, 4050},

/*
 *  Each line represents a set of pwm channels {0, 3}, {0, 4}, {0, 14}, {0, 36}, {0, 49}
 *  The first numeric value of each set represents the pwm channel, and the second numeric value represents the pin number
 *       For example: 0 represents pwm channel 0 3 represents GPIO3
 *  The pwm driver scans the form to confirm if a channel has a gpio available
 */
#define CONFIG_PWM_PIN_CHAN_MAP      \
    {0, 4}, {0, 9}, {0, 18}, {0, 21},\
    {1, 5}, {1, 7}, {1, 10}, {1, 22},\
    {2, 1}, {2, 3}, {2, 8}, {2, 17}, {2, 20},\
    {3, 0}, {3, 2}, {3, 6}, {3, 19}

#define BOARD_BATTERY_VOLTAGE_4_35_V      \
	{{ 3100, 3671, 3746, 3808, 3840, 3883, 3929, 3994, 4074, 4183 }}

#define BOARD_BATTERY_VOLTAGE_4_20_V      \
	{{ 3100, 3500, 3600, 3660, 3710, 3760, 3810, 3880, 3960, 4050 }}	

#define BOARD_ADCKEY_KEY_MAPPINGS	\
	{KEY_NEXTSONG, 0, 0x180},	\
	{KEY_TBD, 0x180, 0x290},	\
	{KEY_MENU, 0x290, 0x400},	\
	{KEY_VOLUMEDOWN, 0x400, 0x570}, \
	{KEY_VOLUMEUP, 0x570, 0x670},

#define MUTIPLE_CLIK_KEY	\
	{KEY_NEXTSONG, KEY_TBD, KEY_MENU, KEY_VOLUMEDOWN, KEY_VOLUMEUP, KEY_PAUSE_AND_RESUME}

/* @brief The macro to define an audio device */
#define AUDIO_LINE_IN0 (AUDIO_DEV_TYPE_LINEIN | 0)
#define AUDIO_LINE_IN1 (AUDIO_DEV_TYPE_LINEIN | 1)
#define AUDIO_LINE_IN2 (AUDIO_DEV_TYPE_LINEIN | 2)
#define AUDIO_ANALOG_MIC0 (AUDIO_DEV_TYPE_AMIC | 0)
#define AUDIO_ANALOG_MIC1 (AUDIO_DEV_TYPE_AMIC | 1)
#define AUDIO_ANALOG_MIC2 (AUDIO_DEV_TYPE_AMIC | 2)
#define AUDIO_ANALOG_FM0 (AUDIO_DEV_TYPE_FM | 0)
#define AUDIO_DIGITAL_MIC0 (AUDIO_DEV_TYPE_DMIC | 0)
#define AUDIO_DIGITAL_MIC1 (AUDIO_DEV_TYPE_DMIC | 1)

/* config for battery NTC */
#define BATNTC_MFP_SEL          28
#define BATNTC_MFP_CFG (GPIO_CTL_MFP(BATNTC_MFP_SEL) | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))

struct board_pinmux_info{
	const struct acts_pin_config *pins_config;
	int pins_num;
};

struct pwm_acts_pin_config{
	unsigned int pin_num;
	unsigned int pin_chan;
	unsigned int mode;
};

struct board_pwm_pinmux_info{
	const struct pwm_acts_pin_config *pins_config;
	int pins_num;
};
extern void board_get_pwm_pinmux_info(struct board_pwm_pinmux_info *pinmux_info);

/* @brief Get the mapping relationship between the hardware inputs and audio devices */
int board_audio_device_mapping(audio_input_map_t *input_map);
int board_extern_pa_ctl(u8_t pa_class, bool is_on);

#endif /* __INC_BOARD_H */
