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
#include <drivers/pa.h>

#if CONFIG_PA_ACM8625
#include "ACM8625/pa_acm8625.h"
#endif

static struct pa_device_data g_act_pa_data;

int pa_audio_init(const struct device *dev)
{
    const struct pa_driver_api  *pa_api  = dev->api;

    if (pa_api->api_init) {
        pa_api->api_init(dev);
    }

    return 0;
}

int pa_audio_format_config(const struct device *dev, const struct pa_i2s_format *p_i2s_fmt)
{
    int result = -1;
    const struct pa_driver_api *pa_api = dev->api;

    if (pa_api->api_config) {
        result = pa_api->api_config(dev, p_i2s_fmt);
    }

    return result;
}

int pa_audio_volume_set(const struct device *dev, uint8_t channel, uint8_t volume)
{
    int result = -1;
    const struct pa_driver_api *pa_api = dev->api;

    if (pa_api->api_volume) {
        result = pa_api->api_volume(dev, channel, volume);
    }

    return result;
}

int pa_audio_mute_set(const struct device *dev, uint8_t channel, bool mute)
{
    int result = -1;
    const struct pa_driver_api *pa_api = dev->api;

    if (pa_api->api_mute) {
        result = pa_api->api_mute(dev, channel, mute);
    }

    return result;
}

int pa_audio_state_get(const struct device *dev, struct pa_driver_state *pa_state)
{
    struct pa_device_data *pa_data = dev->data;

    pa_state = &pa_data->state;

    return 0;
}

static int acts_pa_init(const struct device *dev)
{
    struct pa_device_data       *pa_data = dev->data;

    pa_data->i2c_dev = (struct device *)device_get_binding(CONFIG_PA_I2C_NAME);
    if (pa_data->i2c_dev == NULL) {
        printk("acts_pa_init, binding [%s] err! \n", CONFIG_PA_I2C_NAME);
        return -EIO;
    }

    /* i2c master, standard mode(100kbps) */
    i2c_configure((const struct device *)pa_data->i2c_dev, I2C_MODE_MASTER | I2C_SPEED_SET(I2C_SPEED_STANDARD));
    /* i2c master, fast mode(400kbps) */
    // i2c_configure((const struct device *)pa_data->i2c_dev, I2C_MODE_MASTER | I2C_SPEED_SET(I2C_SPEED_FAST));

    /* Init pa state */
    pa_data->state.type     = I2S_TYPE_PHILIPS;
    pa_data->state.rate     = I2S_SAMPLE_RATE_48K;
    pa_data->state.width    = I2S_DATA_WIDTH_24;
    pa_data->state.volume_r = PA_AUDIO_VOLUME_MAX / 2;
    pa_data->state.volume_l = PA_AUDIO_VOLUME_MAX / 2;
    pa_data->state.mute_l   = false;
    pa_data->state.mute_r   = false;

    return 0;
}

static const struct pa_driver_api g_act_pa_driver_api =
{
    #ifdef CONFIG_PA_ACM8625
    .api_init   = pa_acm8625_audio_init,
    .api_config = pa_acm8625_audio_format_config,
    .api_volume = pa_acm8625_audio_volume_set,
    .api_mute   = pa_acm8625_audio_mute_set,
    #endif
};

DEVICE_DEFINE(pa, CONFIG_PA_DEV_NAME, acts_pa_init, NULL, &g_act_pa_data, NULL, \
                POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &g_act_pa_driver_api);
