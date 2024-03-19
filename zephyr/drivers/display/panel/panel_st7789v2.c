/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2019 Marc Reilly
 * Copyright (c) 2019 PHYTEC Messtechnik GmbH
 * Copyright (c) 2020 Endian Technologies AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_st7789v2.h"
#include "panel_common.h"

#include <device.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <drivers/display.h>
#include <drivers/pwm.h>
#include <drivers/display/display_controller.h>
#include <drivers/display/display_engine.h>
#include <tracing/tracing.h>

#include <soc.h>
#include <board.h>

#define LOG_LEVEL CONFIG_DISPLAY_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(panel_st7789v2);

static const struct gpio_cfg reset_gpio_cfg = CONFIG_PANEL_RESET_GPIO;

#ifdef CONFIG_PANEL_BACKLIGHT_GPIO
static const struct gpio_cfg backlight_gpio_cfg = CONFIG_PANEL_BACKLIGHT_GPIO;
#endif

#ifdef CONFIG_PANEL_POWER_GPIO
static const struct gpio_cfg power_gpio_cfg = CONFIG_PANEL_POWER_GPIO;
#endif

#define ST7789V2_PIXEL_SIZE 2

static const PANEL_VIDEO_PORT_DEFINE(st7789v2_videoport);
static const PANEL_VIDEO_MODE_DEFINE(st7789v2_videomode, PIXEL_FORMAT_RGB_565);
//static void st7789v2_lcd_init(struct st7789v2_data *p_st7789v2);

struct k_timer st7789v2_timer;

struct st7789v2_data {
	const struct device *lcdc_dev;
	const struct display_callback *callback[2];
	struct display_buffer_descriptor refr_desc;
	struct k_sem write_sem;

#ifdef CONFIG_PANEL_TE_GPIO
	const struct device *te_gpio_dev;
	struct gpio_callback te_gpio_cb;
#endif

#ifdef CONFIG_PANEL_BACKLIGHT_GPIO
	const struct device *backlight_gpio_dev;
#elif defined(CONFIG_PANEL_BRIGHTNESS)
	const struct device *pwm_dev;
	u8_t backlight_brightness;
#endif

#ifdef CONFIG_PANEL_POWER_GPIO
	const struct device *power_gpio_dev;
	struct k_delayed_work timer;
#endif

#ifdef CONFIG_PM_DEVICE
	uint32_t pm_state;
#endif
};

struct st7789v2_data *g_data = NULL;


static void st7789v2_transmit(struct st7789v2_data *data, int cmd,
		const uint8_t *tx_data, size_t tx_count)
{
	display_controller_write_config(data->lcdc_dev, cmd, tx_data, tx_count);
}

static void st7789v2_exit_sleep(struct st7789v2_data *data)
{

	//st7789v2_transmit(data, ST7789V2_CMD_SLEEP_OUT, NULL, 0);

	//k_sleep(K_MSEC(120));
}

static void st7789v2_reset_display(struct st7789v2_data *data)
{
	LOG_DBG("Resetting display");

#ifdef CONFIG_PANEL_RESET_GPIO
	const struct device *gpio_dev = device_get_binding(reset_gpio_cfg.gpio_dev_name);

	if (gpio_pin_configure(gpio_dev, reset_gpio_cfg.gpion, GPIO_OUTPUT_ACTIVE | reset_gpio_cfg.flag)) {
		LOG_ERR("Couldn't configure enable pin");
		return;
	}
	gpio_pin_set(gpio_dev, reset_gpio_cfg.gpion, 0);
	k_sleep(K_MSEC(2));
	gpio_pin_set(gpio_dev, reset_gpio_cfg.gpion, 1);
	k_sleep(K_MSEC(2));
#endif
}

static int st7789v2_blanking_on(const struct device *dev)
{
	struct st7789v2_data *driver = (struct st7789v2_data *)dev->data;
#if 0
#ifdef CONFIG_PANEL_BACKLIGHT_GPIO
	if (driver->backlight_gpio_dev)
		gpio_pin_set(driver->backlight_gpio_dev, backlight_gpio_cfg.gpion, 0);
#elif defined(CONFIG_PANEL_BRIGHTNESS)
	pwm_pin_set_cycles(driver->pwm_dev, CONFIG_PANEL_BACKLIGHT_PWM_CHAN, 255, 0, 1);
#endif

	st7789v2_transmit(driver, ST7789V2_CMD_DISP_OFF, NULL, 0);
#endif
	return 0;
}

static int st7789v2_blanking_off(const struct device *dev)
{

#if 0
	struct st7789v2_data *driver = (struct st7789v2_data *)dev->data;

	st7789v2_transmit(driver, ST7789V2_CMD_DISP_ON, NULL, 0);

#ifdef CONFIG_PANEL_BACKLIGHT_GPIO
	if (driver->backlight_gpio_dev)
		gpio_pin_set(driver->backlight_gpio_dev, backlight_gpio_cfg.gpion, 1);
#elif defined(CONFIG_PANEL_BRIGHTNESS)
	pwm_pin_set_cycles(driver->pwm_dev, CONFIG_PANEL_BACKLIGHT_PWM_CHAN, 255, driver->backlight_brightness, 1);
#endif
#endif
	return 0;
}

static int st7789v2_read(const struct device *dev,
			const uint16_t x,
			const uint16_t y,
			const struct display_buffer_descriptor *desc,
			void *buf)
{
	return -ENOTSUP;
}

static void st7789v2_set_mem_area(struct st7789v2_data *data, const uint16_t x,
				 const uint16_t y, const uint16_t w, const uint16_t h)
{
	uint16_t cmd_data[2];

	uint16_t ram_x = x;
	uint16_t ram_y = y;

	cmd_data[0] = sys_cpu_to_be16(ram_x);
	cmd_data[1] = sys_cpu_to_be16(ram_x + w - 1);
	st7789v2_transmit(data, ST7789V2_CMD_CASET, (uint8_t *)&cmd_data[0], 4);

	cmd_data[0] = sys_cpu_to_be16(ram_y);
	cmd_data[1] = sys_cpu_to_be16(ram_y + h - 1);
	st7789v2_transmit(data, ST7789V2_CMD_RASET, (uint8_t *)&cmd_data[0], 4);
}

static void st7789v2_Clear(struct st7789v2_data *data, uint16_t color)
{
	struct display_buffer_descriptor desc = {
		.pixel_format = PIXEL_FORMAT_RGB_565,
		.width = 1,
		.height = 1,
		.pitch = 1,
	};
	int i;

	display_controller_set_source(data->lcdc_dev, DISPLAY_CONTROLLER_SOURCE_MCU, NULL);

	st7789v2_set_mem_area(data, 0, 0, 240, 320);
	st7789v2_transmit(data, ST7789V2_CMD_RAMWR, 0, 0);

	 for (i = 0; i < 240 * 320; i++) {
		display_controller_write_pixels(data->lcdc_dev, -1, &desc, &color);
	 }

}


static void st7789v2_Clear_dbg(struct st7789v2_data *data, uint16_t color)
{
    uint16_t dis_buf[60 * 52];

	struct display_buffer_descriptor desc = {
		.pixel_format = PIXEL_FORMAT_RGB_565,
		.width = 60,
		.height = 52,
		.pitch = 60,
	};
	int i;


    for (i = 0; i < 60 * 52; i++) {
        dis_buf[i] = 0xff00;
    }

	//display_controller_set_source(data->lcdc_dev, DISPLAY_CONTROLLER_SOURCE_DMA, NULL);
	display_controller_set_source(data->lcdc_dev, DISPLAY_CONTROLLER_SOURCE_MCU, NULL);

	st7789v2_set_mem_area(data, 0, 0, 60, 52);
	st7789v2_transmit(data, ST7789V2_CMD_RAMWR, 0, 0);

    display_controller_write_pixels(data->lcdc_dev, -1, &desc, dis_buf);

}


static int st7789v2_read_id(struct st7789v2_data *p_st7789v2)

{
    uint8_t data_id[4];
    display_controller_read_config(p_st7789v2->lcdc_dev, 0x04, data_id, sizeof(data_id));
    printk("%s,id:0x%X, 0x%X,0x%X,0x%X\n", __func__, data_id[0], data_id[1], data_id[2], data_id[3]);

}

static int st7789v2_write(const struct device *dev,
			 const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	struct st7789v2_data *data = (struct st7789v2_data *)dev->data;
    //printk("++++%s, x:%d, y:%d, buf_size:%d, format:%d, width:%d, height:%d, pitch:%d\n", __func__, x, y, desc->buf_size, desc->pixel_format, desc->width, desc->height, desc->pitch);

	if (desc->pitch < desc->width)
		return -EDOM;
    #if 0
	if (k_sem_take(&data->write_sem, K_MSEC(1000))) {
		LOG_ERR("last write not completed yet.");
		return -ETIME;
	}
    #endif

    if((desc->pitch > 240) || (desc->width > 240) || (desc->height > 320))
    {
        printk("params err!!\n");
        return 0;
    }

	//sys_trace_u32x4(SYS_TRACE_ID_LCD_POST,
			//x, y, x + desc->width - 1, y + desc->height - 1);

	if((x + desc->width > 240) || (y + desc->height > 320))
	{
		printk("buf overflow!!!\n");
		return 0;
	}

    st7789v2_set_mem_area(data, x, y, desc->width, desc->height);
    //st7789v2_set_mem_area(data, 0, 0, desc->width, desc->height);

	st7789v2_transmit(data, ST7789V2_CMD_RAMWR, 0, 0);

	if (display_controller_set_source(data->lcdc_dev, DISPLAY_CONTROLLER_SOURCE_DMA, NULL)) {
		display_controller_set_source(data->lcdc_dev, DISPLAY_CONTROLLER_SOURCE_MCU, NULL);
	}

	return display_controller_write_pixels(data->lcdc_dev, -1, desc, buf);
}


static void *st7789v2_get_framebuffer(const struct device *dev)
{
	return NULL;
}

static int st7789v2_set_brightness(const struct device *dev,
			   const uint8_t brightness)
{
//#if defined(CONFIG_PANEL_BRIGHTNESS)
	//data->backlight_brightness = brightness;
//#endif

	return -ENOTSUP;
}

static int st7789v2_set_contrast(const struct device *dev,
			 const uint8_t contrast)
{

	return -ENOTSUP;
}

static void st7789v2_get_capabilities(const struct device *dev,
			      struct display_capabilities *capabilities)
{
    memset(capabilities, 0, sizeof(struct display_capabilities));
    capabilities->x_resolution = st7789v2_videomode.hactive;
    capabilities->y_resolution = st7789v2_videomode.vactive;
    capabilities->vsync_period = 1000000u / st7789v2_videomode.refresh_rate;
    capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
    capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
    capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
	capabilities->screen_info = SCREEN_INFO_VSYNC;
}

static int st7789v2_set_pixel_format(const struct device *dev,
			     const enum display_pixel_format pixel_format)
{
	if (pixel_format == PIXEL_FORMAT_RGB_565) {
		return 0;
	}

	LOG_ERR("Pixel format change not implemented");
	return -ENOTSUP;
}

static int st7789v2_set_orientation(const struct device *dev,
			    const enum display_orientation orientation)
{
	struct st7789v2_data *data = (struct st7789v2_data *)dev->data;
	uint8_t access = 0;

	switch (orientation) {
	case DISPLAY_ORIENTATION_ROTATED_90:
		access = ST7789V2_RAMACC_ROWCOL_SWAP | ST7789V2_RAMACC_COL_INV;
		break;
	case DISPLAY_ORIENTATION_ROTATED_180:
		access = ST7789V2_RAMACC_COL_INV | ST7789V2_RAMACC_ROW_INV;
		break;
	case DISPLAY_ORIENTATION_ROTATED_270:
		access = ST7789V2_RAMACC_ROWCOL_SWAP | ST7789V2_RAMACC_ROW_INV;
		break;
	case DISPLAY_ORIENTATION_NORMAL:
	default:
		break;
	}

	/* This command has no effect when Tearing Effect output is already ON */
	st7789v2_transmit(data, ST7789V2_CMD_RAMACC, &access, 1);

	LOG_INF("Changing display orientation %d", orientation);
	return 0;
}

static void st7789v2_lcd_init(struct st7789v2_data *p_st7789v2)
{
	static uint8_t tmp;
    printk("+++++++%s,%d, start+++++++++++++++\n", __func__, __LINE__);

	st7789v2_transmit(p_st7789v2, 0x11, NULL, 0);
	k_sleep(K_MSEC(120));

    /*Display Setting*/
	tmp = 0x00;
	st7789v2_transmit(p_st7789v2, 0x36, &tmp, 1);
	tmp = 0x05;
	st7789v2_transmit(p_st7789v2, 0x3a, &tmp, 1);

    /*ST7789S Frame rate setting*/
	const uint8_t data_0xb2[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
	st7789v2_transmit(p_st7789v2, 0xb2, data_0xb2, sizeof(data_0xb2));
	tmp = 0x35;
	st7789v2_transmit(p_st7789v2, 0xb7, &tmp, 1);

    /*ST7789S Power setting*/
	tmp = 0x2b;
	st7789v2_transmit(p_st7789v2, 0xbb, &tmp, 1);
	tmp = 0x2c;
	st7789v2_transmit(p_st7789v2, 0xc0, &tmp, 1);
	tmp = 0x01;
	st7789v2_transmit(p_st7789v2, 0xc2, &tmp, 1);
	tmp = 0x11;
	st7789v2_transmit(p_st7789v2, 0xc3, &tmp, 1);
	tmp = 0x20;
	st7789v2_transmit(p_st7789v2, 0xc4, &tmp, 1);
	tmp = 0x0f;
	st7789v2_transmit(p_st7789v2, 0xc6, &tmp, 1);
	const uint8_t data_0xd0[] = {0xa4, 0xa1};
	st7789v2_transmit(p_st7789v2, 0xd0, data_0xd0, sizeof(data_0xd0));

    /*ST7789S gamma setting*/
	const uint8_t data_0xe0[] = {0xd0,0x00,0x05,0x0e,0x15,0x0d,0x37,0x43,0x47,0x09,0x15,0x12,0x16,0x19};
	st7789v2_transmit(p_st7789v2, 0xe0, data_0xe0, sizeof(data_0xe0));

	const uint8_t data_0xe1[] = {0xd0,0x00,0x05,0x0d,0x0c,0x06,0x2d,0x44,0x40,0x0e,0x1c,0x18,0x16,0x19};
	st7789v2_transmit(p_st7789v2, 0xe1, data_0xe1, sizeof(data_0xe1));


	st7789v2_transmit(p_st7789v2, 0x29, NULL, 0);
    printk("+++++++%s,%d, end+++++++++++++++\n", __func__, __LINE__);

    k_sleep(K_MSEC(100));

	//gpio_pin_set(p_gc9c01->backlight_gpio_dev, ST7789V2_BACKLIGHT_PIN, 1);
//	pwm_pin_set_cycles(p_gc9c01->pwm_dev, ST7789V2_PWM_CHAN, 255, p_gc9c01->backlight_brightness, 1);
}

static int st7789v2_register_callback(const struct device *dev,
					  const struct display_callback *callback)
{
	struct st7789v2_data *data = (struct st7789v2_data *)dev->data;
	int i;

	for (i = 0; i < ARRAY_SIZE(data->callback); i++) {
		if (data->callback[i] == NULL) {
			data->callback[i] = callback;
			return 0;
		}
	}

	return -EBUSY;
}

static int st7789v2_unregister_callback(const struct device *dev,
					  const struct display_callback *callback)
{
	struct st7789v2_data *data = (struct st7789v2_data *)dev->data;
	int i;

	for (i = 0; i < ARRAY_SIZE(data->callback); i++) {
		if (data->callback[i] == callback) {
			data->callback[i] = NULL;
			return 0;
		}
	}

	return -EINVAL;
}

static void st7789v2_prepare_transfer(void *arg, const display_rect_t *area)
{
	struct device *dev = arg;
	struct st7789v2_data *data = (struct st7789v2_data *)dev->data;


	data->refr_desc.pixel_format = PIXEL_FORMAT_RGB_565;
	data->refr_desc.width = area->w;
	data->refr_desc.height = area->h;
	data->refr_desc.pitch = area->w;

	sys_trace_u32x4(SYS_TRACE_ID_LCD_POST,
			area->x, area->y, area->x + area->w - 1, area->y + area->h - 1);

	st7789v2_set_mem_area(data, area->x, area->y, area->w, area->h);

	if (st7789v2_videoport.type != DISPLAY_PORT_SPI_QUAD) {
		st7789v2_transmit(data, ST7789V2_CMD_RAMWR, 0, 0);
	}

	display_controller_set_source(data->lcdc_dev, DISPLAY_CONTROLLER_SOURCE_ENGINE, NULL);
}

static void st7789v2_start_transfer(void *arg)
{
	const struct device *dev = arg;
	struct st7789v2_data *data = (struct st7789v2_data *)dev->data;

	/* source from DE */
	//display_controller_write_pixels(data->lcdc_dev, (0x32 << 24 | 0x2c << 8),
			//&data->refr_desc, NULL);
}

static void st7789v2_complete_transfer(void *arg)
{
	const struct device *dev = arg;
	struct st7789v2_data *data = (struct st7789v2_data *)dev->data;
	int i;

	//sys_trace_end_call(SYS_TRACE_ID_LCD_POST);

	//k_sem_give(&data->write_sem);

	for (i = 0; i < ARRAY_SIZE(data->callback); i++) {
		if (data->callback[i] && data->callback[i]->complete) {
			data->callback[i]->complete(data->callback[i]);
		}
	}
}

DEVICE_DECLARE(st7789v2);

#ifdef CONFIG_PANEL_TE_GPIO
static void _te_gpio_callback_handler(struct k_work *work)
{
	const struct device *dev = DEVICE_GET(st7789v2);
	struct st7789v2_data *data = (struct st7789v2_data *)dev->data;
	uint32_t timestamp = k_cycle_get_32();
	int i;

	sys_trace_void(SYS_TRACE_ID_VSYNC);

	for (i = 0; i < ARRAY_SIZE(data->callback) ; i++) {
		if (data->callback[i] && data->callback[i]->vsync) {
			data->callback[i]->vsync(data->callback[i], timestamp);
		}
	}

	sys_trace_end_call(SYS_TRACE_ID_VSYNC);
}
#endif /* CONFIG_PANEL_TE_GPIO */

#ifdef CONFIG_PANEL_TE_GPIO
static int st7789v2_set_te(const struct device *dev)
{
	struct gc9c01_data *data = (struct st7789v2_data *)dev->data;

    k_timer_start(&st7789v2_timer, K_MSEC(20), K_MSEC(20));

	return 0;
}
#endif

static int st7789v2_init(const struct device *dev)
{
	struct st7789v2_data *data = (struct st7789v2_data *)dev->data;
	struct device *de_dev = NULL;

#ifdef CONFIG_PANEL_TE_GPIO
	 //gpio_init_callback(&data->te_gpio_cb, _te_gpio_callback_handler, BIT(te_gpio_cfg.gpion));
    k_timer_init(&st7789v2_timer, _te_gpio_callback_handler, NULL);
    k_timer_user_data_set(&st7789v2_timer, (void *)dev);

#endif

#ifdef CONFIG_PANEL_BACKLIGHT_GPIO
	data->backlight_gpio_dev = device_get_binding(backlight_gpio_cfg.gpio_dev_name);
	if (gpio_pin_configure(data->backlight_gpio_dev, backlight_gpio_cfg.gpion,
				GPIO_OUTPUT_ACTIVE | backlight_gpio_cfg.flag)) {
		LOG_ERR("Couldn't configure backlight pin");
		data->backlight_gpio_dev = NULL;
	}
	gpio_pin_set(data->backlight_gpio_dev, backlight_gpio_cfg.gpion, 0);
#elif defined(CONFIG_PANEL_BRIGHTNESS)
	data->pwm_dev = device_get_binding(CONFIG_PWM_NAME);
	if(data->pwm_dev == NULL) {
		LOG_ERR("can not find pwm device\n");
		return -EIO;
	}
	pwm_pin_set_cycles(data->pwm_dev, CONFIG_PANEL_BACKLIGHT_PWM_CHAN, 255, 0, 1);
	data->backlight_brightness = CONFIG_PANEL_BRIGHTNESS;
#endif

#ifdef CONFIG_PANEL_POWER_GPIO
		data->power_gpio_dev = device_get_binding(power_gpio_cfg.gpio_dev_name);
		if (gpio_pin_configure(data->power_gpio_dev, power_gpio_cfg.gpion,
					GPIO_OUTPUT_ACTIVE | power_gpio_cfg.flag)) {
			LOG_ERR("Couldn't configure power pin");
			data->power_gpio_dev = NULL;
		}
		gpio_pin_set(data->power_gpio_dev, power_gpio_cfg.gpion, 1);
#endif

	data->lcdc_dev = (struct device *)device_get_binding(CONFIG_LCDC_DEV_NAME);
	if (data->lcdc_dev == NULL) {
		LOG_ERR("Could not get LCD controller device");
		return -EPERM;
	}

	display_controller_enable(data->lcdc_dev, &st7789v2_videoport);
	display_controller_control(data->lcdc_dev,
			DISPLAY_CONTROLLER_CTRL_COMPLETE_CB, st7789v2_complete_transfer, (void *)dev);
	display_controller_set_mode(data->lcdc_dev, &st7789v2_videomode);

	de_dev = (struct device *)device_get_binding(CONFIG_DISPLAY_ENGINE_DEV_NAME);
	if (de_dev == NULL) {
		LOG_WRN("Could not get display engine device");
	} else {
		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_PORT, (void *)&st7789v2_videoport, NULL);
		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_MODE, (void *)&st7789v2_videomode, NULL);
		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_PREPARE_CB, st7789v2_prepare_transfer, (void *)dev);
		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_START_CB, st7789v2_start_transfer, (void *)dev);
	}

	k_sem_init(&data->write_sem, 1, 1);

#ifdef CONFIG_PM_DEVICE
	data->pm_state = PM_DEVICE_ACTION_RESUME;
#endif

	st7789v2_reset_display(data);

	//st7789v2_blanking_on(dev);

	st7789v2_lcd_init(data);
	st7789v2_set_te(dev);
    g_data = data;

    #if 0
#ifdef CONFIG_PANEL_ORIENTATION
	st7789v2_set_orientation(dev, CONFIG_PANEL_ORIENTATION / 90);
#else
	st7789v2_set_orientation(dev, DISPLAY_ORIENTATION_NORMAL);
#endif
    #endif
	//st7789v2_exit_sleep(data);
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static void st7789v2_enter_sleep(struct st7789v2_data *data)
{
	st7789v2_transmit(data, ST7789V2_CMD_SLEEP_IN, NULL, 0);
//	k_sleep(K_MSEC(5));
}

#ifdef CONFIG_PANEL_POWER_GPIO
static void panel_st7789v2_res(struct k_work *work)
{
	struct st7789v2_data *st7789v2 = CONTAINER_OF(work, struct st7789v2_data, timer);
	const struct device *lcd_panel_dev = NULL;

	gpio_pin_set(st7789v2->power_gpio_dev, power_gpio_cfg.gpion, 0);
	lcd_panel_dev = device_get_binding(CONFIG_LCD_DISPLAY_DEV_NAME);
	st7789v2_init(lcd_panel_dev);
}
#endif

static int st7789v2_pm_control(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;
	struct st7789v2_data *data = (struct st7789v2_data *)dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_EARLY_SUSPEND:
		//st7789v2_pm_eary_suspend(dev);
		break;
	case PM_DEVICE_ACTION_LATE_RESUME:
		if (data->pm_state == PM_DEVICE_STATE_LOW_POWER) {
			st7789v2_blanking_off(dev);
#ifdef CONFIG_PANEL_TE_GPIO
			gpio_add_callback(data->te_gpio_dev, &data->te_gpio_cb);
#endif
			//st7789v2_pm_notify(data, PM_DEVICE_ACTION_LATE_RESUME);
			data->pm_state = PM_DEVICE_STATE_ACTIVE;
			LOG_WRN("panel active");
		}
		break;
	case PM_DEVICE_ACTION_RESUME:
		if (data->pm_state == PM_DEVICE_STATE_OFF) {
			LOG_WRN("panel resuming");
			//k_delayed_work_init(&data->timer, st7789v2_resume);
			k_delayed_work_submit(&data->timer, K_NO_WAIT);
			data->pm_state = PM_DEVICE_STATE_LOW_POWER;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_FORCE_SUSPEND:
	case PM_DEVICE_ACTION_TURN_OFF:
		//st7789v2_pm_eary_suspend(dev);
		if (data->pm_state == PM_DEVICE_STATE_LOW_POWER) {
			if (pm_device_is_busy(data->lcdc_dev)) {
				LOG_WRN("panel refuse suspend (lcdc busy)");
				ret = -EBUSY;
			} else {
				data->pm_state = PM_DEVICE_STATE_OFF;
				st7789v2_enter_sleep(data);
#ifdef CONFIG_PANEL_RESET_GPIO
				//gpio_pin_set(data->reset_gpio_dev, reset_gpio_cfg.gpion, 1);
#endif
#ifdef CONFIG_PANEL_POWER_GPIO
				gpio_pin_set(data->power_gpio_dev, power_gpio_cfg.gpion, 0);
#endif
				LOG_WRN("panel suspend");
			}
		}
		break;
	default:
		break;
	}

	return ret;
}

#endif /* CONFIG_PM_DEVICE */

static const struct display_driver_api st7789v2_api = {
	.blanking_on = st7789v2_blanking_on,
	.blanking_off = st7789v2_blanking_off,
	.write = st7789v2_write,
	.read = st7789v2_read,
	.get_framebuffer = st7789v2_get_framebuffer,
	.set_brightness = st7789v2_set_brightness,
	.set_contrast = st7789v2_set_contrast,
	.get_capabilities = st7789v2_get_capabilities,
	.set_pixel_format = st7789v2_set_pixel_format,
	.set_orientation = st7789v2_set_orientation,
	.register_callback = st7789v2_register_callback,
	.unregister_callback = st7789v2_unregister_callback,
};

static struct st7789v2_data st7789v2_data;

#if IS_ENABLED(CONFIG_PANEL)
DEVICE_DEFINE(st7789v2, CONFIG_LCD_DISPLAY_DEV_NAME, &st7789v2_init,
			st7789v2_pm_control, &st7789v2_data, NULL, POST_KERNEL,
			60, &st7789v2_api);
#endif
