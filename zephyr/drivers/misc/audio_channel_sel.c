/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief I2C master driver for Actions SoC
 */

#include <errno.h>
#include <sys/__assert.h>
#include <stdbool.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/misc/audio_channel_sel.h>
#include <soc.h>
#include <drivers/adc.h>
#include <soc_pmu.h>
#include <drivers/gpio.h>
#include <os_common_api.h>

#ifdef CONFIG_CFG_DRV
#include <config.h>
#include <drivers/cfg_drv/dev_config.h>
#endif

int audio_acts_chan_sel(const struct device *dev, struct audio_chansel_data *data)
{
	if (data->sel_mode == AUDIO_CHANNEL_SEL_BY_LRADC)
	{
    	u32_t lradc_ctrl = data->Select_LRADC.LRADC_Ctrl;

		u32_t ctrl_no = lradc_ctrl & 0xff;
		u32_t gpio_no = (lradc_ctrl & 0xff00) >> 8;
		u32_t mfp_sel = (lradc_ctrl & 0xff0000) >> 16;
		u32_t pin_mode = 0;

		struct adc_channel_cfg channel_cfg = {0};

        if (lradc_ctrl == LRADC_CTRL_NONE)
            return 0;

		/* remap channel number to LRADC index */
		channel_cfg.channel_id = PMUADC_ID_LRADC1 + ctrl_no - 1;

		acts_pinmux_get(gpio_no, &pin_mode);
		
		pin_mode = (pin_mode & ~GPIO_CTL_MFP_MASK) | (mfp_sel << GPIO_CTL_MFP_SHIFT);
		pin_mode = (pin_mode & ~GPIO_CTL_PULL_MASK) | 
		    (data->Select_LRADC.LRADC_Pull_Up << GPIO_CTL_PULL_SHIFT);
		
		acts_pinmux_set(gpio_no, pin_mode);

		const struct device *adc;

		adc = device_get_binding(CONFIG_PMUADC_NAME);

		if (adc == NULL || adc_channel_setup(adc, &channel_cfg)) {
//			LOG_ERR("setup channel_id %d error", channel_cfg.channel_id);
			return 0;
		}

		struct adc_sequence sequence = { 0, };
		uint8_t adc_buf[4];

		sequence.channels = BIT(channel_cfg.channel_id);
		sequence.buffer = &adc_buf[0];
		sequence.buffer_size = sizeof(adc_buf);

		int ret;

		ret = adc_read(adc, &sequence);

		ret = ((uint16_t)adc_buf[1] << 8) | adc_buf[0];
		SYS_LOG_INF("0x%x", ret);

		return ret;
	}
	else if (data->sel_mode == AUDIO_CHANNEL_SEL_BY_GPIO)
	{
		u32_t pin = data->Select_GPIO.GPIO_Pin;
		u32_t pull = data->Select_GPIO.Pull_Up_Down;
		u32_t val;

        if (pin >= GPIO_MAX_PIN_NUM)
            return 0;

        val = sys_read32(GPIO_REG_CTL(GPIO_REG_BASE, pin));
        
        val = (val & ~GPIO_CTL_GPIO_OUTEN) | GPIO_CTL_GPIO_INEN;
        val = (val & ~GPIO_CTL_PULL_MASK) | (pull << GPIO_CTL_PULL_SHIFT);
        val = (val & ~GPIO_CTL_MFP_MASK) | (0 << GPIO_CTL_MFP_SHIFT);
        
        sys_write32(val, GPIO_REG_CTL(GPIO_REG_BASE, pin));

        /* need some delay after gpio pull and then read gpio level */
        k_busy_wait(100);

		val = !!(sys_read32(GPIO_REG_IDAT(GPIO_REG_BASE, pin)) & GPIO_BIT(pin));
		SYS_LOG_INF("%d", val);

		return val;
	}
	return 0;
}

int audio_acts_chan_sel_init(const struct device *dev)
{

	return 0;
}

const struct audio_chan_sel_driver_api audio_acts_chan_sel_driver_api = {
	.chan_sel = audio_acts_chan_sel,
};

DEVICE_DEFINE(audio_channel_sel, "audio_channel_sel",		\
		    &audio_acts_chan_sel_init, NULL,			\
		    NULL, NULL,	\
		    POST_KERNEL, CONFIG_AUDIO_CHAN_SEL_INIT_PRIORITY,	\
		    &audio_acts_chan_sel_driver_api);




