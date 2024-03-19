/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * References:
 *
 * https://www.microbit.co.uk/device/screen
 * https://lancaster-university.github.io/microbit-docs/ubit/display/
 */

#include <zephyr.h>
#include <init.h>
#include <board.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>

#include <device.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include <misc/printk.h>
#include <os_common_api.h>
#include <drivers/fm.h>
#include <board_cfg.h>

#if CONFIG_FM_AR1019
#include "AR1019/fm_ar1019.h"
#endif

#if CONFIG_FM_RDA5807
#include "RDA5807/fm_rda5807.h"
#endif



static struct acts_fm_device_data fm_dev_data;
//static os_delayed_work fm_delaywork;

/*FM模组初始化*/
int fm_init(const struct device *dev,u8_t band, u8_t level, u16_t freq)
{
	
	const struct fm_driver_api *api = dev->api;
	int result = -1;

	if (api->init)
		result = api->init(dev,band,level,freq);

	return result;
}

/*FM模组进入standby*/
int fm_standy(const struct device *dev)
{
	const struct fm_driver_api *api = dev->api;
	int result = -1;

	if (api->standy)
		result = api->standy(dev);

	return result;
}

/*设置频点开始播放*/
int fm_set_freq(const struct device *dev, uint16_t freq)
{
	const struct fm_driver_api *api = dev->api;
	int result = -1;

	if (api->set_freq)
		result = api->set_freq(dev,freq);

	return result;
}

/* 设置电台频段*/
int fm_set_band(const struct device *dev, fm_band_e band)
{
	const struct fm_driver_api *api = dev->api;
	int result = -1;

	if (api->set_band)
		result = api->set_band(dev,band);

	return result;
}

/* 设置搜台门限*/
int fm_set_threshold(const struct device *dev, uint8_t level)
{
	const struct fm_driver_api *api = dev->api;
	int result = -1;

	if (api->set_threshold)
		result = api->set_threshold(dev,level);

	return result;
}

/* 设置音量*/
int fm_set_volume(const struct device *dev, uint8_t volume)
{
	const struct fm_driver_api *api = dev->api;
	int result = -1;

	if (api->set_volume)
		result = api->set_volume(dev,volume);

	return result;
}

/*静音与解除静音*/
int fm_set_mute(const struct device *dev, bool enable)
{
	const struct fm_driver_api *api = dev->api;
	int result = -1;

	if (api->set_mute)
		result = api->set_mute(dev,enable);

	return result;
}

/*获取当前状态信息*/
int fm_get_status(const struct device *dev, fm_status_t *status)
{
	const struct fm_driver_api *api = dev->api;
	int result = -1;

	if (api->get_status)
		result = api->get_status(dev,status);

	return result;
}

/*开启软件搜台过程*/
int fm_soft_seek(const struct device *dev, uint16_t freq, fm_seekdir_e direction)
{
	const struct fm_driver_api *api = dev->api;
	int result = -1;

	if (api->soft_seek)
		result = api->soft_seek(dev,freq,direction);

	return result;
}

/*开启硬件搜台过程*/
int fm_hard_seek(const struct device *dev, uint16_t freq, fm_seekdir_e direction)
{
	const struct fm_driver_api *api = dev->api;
	int result = -1;

	if (api->hard_seek)
		result = api->hard_seek(dev,freq,direction);

	return result;
}

/* 手动退出seek 过程,如果当前是hard seek，就退出hard seek，如果当前是soft seek，就退出soft seek*/
int fm_break_seek(const struct device *dev)
{
	const struct fm_driver_api *api = dev->api;
	int result = -1;

	if (api->break_seek)
		result = api->break_seek(dev);
	
	return result;
}

/* 获取获取当前fm的seek状态，如果当前是hard seek，就返回hard seek的状态，如果当前是soft seek，就返回soft seek*/
int fm_get_seek_status(const struct device *dev,fm_seek_status_e *seek_status)
{
	const struct fm_driver_api *api = dev->api;
	int result = -1;

	if (api->get_seek_status)
		result = api->get_seek_status(dev,seek_status);

	return result;
}

/*static void fm_init_delaywork(os_work *work)
{
	//rda5807_rx_init();
	//ar1019_rx_init();
	struct acts_fm_device_data *fm_ar1019 = dev->data;
	fm_ar1019->i2c_dev = device_get_binding(CONFIG_FM_I2C_NAME);
	if (!fm_ar1019->i2c_dev) {
		// SYS_LOG_ERR("Device not found\n");
		printk("i2c_device_binding failed!!!\n");
		return -1;
	}
	i2c_configure(fm_ar1019->i2c_dev, I2C_SPEED_STANDARD|I2C_MODE_MASTER);
}*/



static int acts_fm_init(const struct device *dev)
{
	printk("%s\n", __func__);
	/*os_delayed_work_init(&fm_delaywork, fm_init_delaywork);
	os_delayed_work_submit(&fm_delaywork, 0);*/
	
	struct acts_fm_device_data *fm_ar1019 = dev->data;
	fm_ar1019->i2c_dev = (struct device *)device_get_binding(CONFIG_FM_I2C_NAME);
	if (!fm_ar1019->i2c_dev) {
		// SYS_LOG_ERR("Device not found\n");
		printk("i2c_device_binding failed!!!\n");
		return -1;
	}
	i2c_configure(fm_ar1019->i2c_dev, I2C_SPEED_STANDARD|I2C_MODE_MASTER);

	return 0;
}

const struct fm_driver_api fm_acts_driver_api = {
#if CONFIG_FM_AR1019
    .init= fm_ar1019_init,
	.standy= fm_ar1019_standy,
	.set_freq = fm_ar1019_set_freq,
	.set_band = fm_ar1019_set_band,
	.set_threshold = fm_ar1019_set_threshold,
	.set_volume = fm_ar1019_set_volume,
	.set_mute = fm_ar1019_set_mute,
	.get_status = fm_ar1019_get_status,
	.soft_seek = fm_ar1019_soft_seek,
	.hard_seek = fm_ar1019_hard_seek,
	.break_seek = fm_ar1019_break_seek,
	.get_seek_status = fm_ar1019_get_seek_status,
#endif
#if CONFIG_FM_RDA5807
    .init= fm_rda5807_init,
	.standy= fm_rda5807_standy,
	.set_freq = fm_rda5807_set_freq,
	.set_band = fm_rda5807_set_band,
	.set_threshold = fm_rda5807_set_threshold,
	.set_volume = fm_rda5807_set_volume,
	.set_mute = fm_rda5807_set_mute,
	.get_status = fm_rda5807_get_status,
	.soft_seek = fm_rda5807_soft_seek,
	.hard_seek = fm_rda5807_hard_seek,
	.break_seek = fm_rda5807_break_seek,
	.get_seek_status = fm_rda5807_get_seek_status,
#endif

};


DEVICE_DEFINE(fm, CONFIG_FM_DEV_NAME, acts_fm_init, NULL,
	&fm_dev_data, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &fm_acts_driver_api);

