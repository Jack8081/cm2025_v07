/**
 * @file
 *
 * @brief Public APIs for the audio channel select drivers.
 */

/*
 * Copyright (c) 2021 Action Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

enum AUDIO_CHANNEL_SEL_MODE {
    AUDIO_CHANNEL_SEL_BY_GPIO  = 0,
    AUDIO_CHANNEL_SEL_BY_LRADC = 1,
};

struct audio_chansel_data {
    uint8_t sel_mode;  /* AUDIO_CHANNEL_SEL_MODE */
	union {
		struct{
			uint8_t  GPIO_Pin;	  // GPIO 管脚, CFG_TYPE_GPIO_PIN
			uint8_t  Pull_Up_Down;  // 上下拉,    CFG_TYPE_GPIO_PULL
		} Select_GPIO;
		struct{
			uint8_t	LRADC_Pull_Up;	// LRADC 上拉电阻, CFG_TYPE_LRADC_PULL_UP
			uint32_t	LRADC_Ctrl; 	// LRADC 控制器,   CFG_TYPE_LRADC_CTRL
		} Select_LRADC;
	};
};


typedef int (*audio_api_chan_sel_t)(const struct device *dev, struct audio_chansel_data *data);

__subsystem struct audio_chan_sel_driver_api {
	audio_api_chan_sel_t chan_sel;
};

/**
 * @brief select channel for audio.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param audio_chansel_data choose two type way for the channel select.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed.
 */

static inline int audio_chan_sel(const struct device *dev, struct audio_chansel_data *data)
{
	const struct audio_chan_sel_driver_api *api =
		(const struct audio_chan_sel_driver_api *)dev->api;

	return api->chan_sel(dev, data);
}

#ifdef __cplusplus
}
#endif
