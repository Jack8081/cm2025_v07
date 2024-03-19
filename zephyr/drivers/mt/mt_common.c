/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>
#include <board.h>
#include <drivers/i2c.h>

#include <device.h>
#include <stdlib.h>
#include <os_common_api.h>
#include <board_cfg.h>
#include <drivers/mt.h>
#include <drivers/gpio.h>

#if CONFIG_MAGIC_TUNE_CM001X1
#include "CM001X1/mt_cm001x1.h"
#endif

static struct mt_device_data g_act_mt_data;

int mt_init(const struct device *dev)
{
    const struct mt_driver_api  *mt_api  = dev->api;

    if (mt_api->api_init) {
        mt_api->api_init(dev);
    }
    acts_pinmux_set(GPIO_10, I2C_MFP_CFG(MFP0_I2C));
    acts_pinmux_set(GPIO_22, I2C_MFP_CFG(MFP0_I2C));
    return 0;
}

int mt_ioctl(const struct device *dev, struct mt_ioctl_info *p_mt_info)
{
    const struct mt_driver_api  *mt_api  = dev->api;

    if (mt_api->api_ioctl) {
        mt_api->api_ioctl(dev, p_mt_info);
    }

    return 0;
}

static int acts_mt_init(const struct device *dev)
{
    struct mt_device_data       *mt_data = dev->data;

    mt_data->i2c_dev = (struct device *)device_get_binding(CONFIG_MT_I2C_NAME);
    if (mt_data->i2c_dev == NULL) {
        printk("acts_mt_init, binding [%s] err! \n", CONFIG_MT_I2C_NAME);
        return -EIO;
    }

    /* i2c master, standard mode(100kbps) */
    //i2c_configure((const struct device *)mt_data->i2c_dev, I2C_MODE_MASTER | I2C_SPEED_SET(I2C_SPEED_STANDARD));
    /* i2c master, fast mode(400kbps) */
    i2c_configure((const struct device *)mt_data->i2c_dev, I2C_MODE_MASTER | I2C_SPEED_SET(I2C_SPEED_FAST));

    return 0;
}

static const struct mt_driver_api g_act_mt_driver_api =
{
    #ifdef CONFIG_MAGIC_TUNE_CM001X1
    .api_init  = mt_cm001x1_init,
    .api_ioctl = mt_cm001x1_ioctl,
    #endif
};

DEVICE_DEFINE(mt, CONFIG_MT_DEV_NAME, acts_mt_init, NULL, &g_act_mt_data, NULL, \
                POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &g_act_mt_driver_api);
