/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_gc9c01.h"
#include "panel_common.h"

#include <soc.h>
#include <board.h>
#include <device.h>
#include <sys/byteorder.h>
#include <drivers/pwm.h>
#include <drivers/gpio.h>
#include <drivers/display.h>
#include <drivers/display/display_controller.h>
#include <drivers/display/display_engine.h>
#include <tracing/tracing.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(panel_gc9c01, CONFIG_DISPLAY_LOG_LEVEL);

#if defined(CONFIG_PANEL_BACKLIGHT_PWM_CHAN) || defined(CONFIG_PANEL_BACKLIGHT_GPIO)
#  define CONFIG_PANEL_BACKLIGHT_CTRL 1
#endif

struct gc9c01_data {
	const struct device *lcdc_dev;
	const struct display_callback *callback;
	struct display_buffer_descriptor refr_desc;
	struct k_sem write_sem;
	uint8_t transfering;
	uint8_t waiting;

#ifdef CONFIG_PANEL_TE_GPIO
	const struct device *te_gpio_dev;
	struct gpio_callback te_gpio_cb;
#endif

#ifdef CONFIG_PANEL_BACKLIGHT_PWM_CHAN
	const struct device *pwm_dev;
#elif defined(CONFIG_PANEL_BACKLIGHT_GPIO)
	const struct device *backlight_gpio_dev;
#endif

#ifdef CONFIG_PANEL_RESET_GPIO
	const struct device *reset_gpio_dev;
#endif

#ifdef CONFIG_PANEL_POWER_GPIO
	const struct device *power_gpio_dev;
#endif

#ifdef CONFIG_PM_DEVICE
	uint32_t pm_state;
	struct k_work resume_work;

	uint8_t pm_post_changed : 1;
#endif

	uint8_t blanking_off : 1;
#if CONFIG_PANEL_BACKLIGHT_CTRL
	/* delay 1 vsync to turn on backlight after blank off */
	uint8_t has_refreshed : 1;
	uint8_t has_refreshed_2 : 1;
#endif
	uint8_t brightness;
	uint8_t pending_brightness;
};


#ifdef CONFIG_PANEL_TE_GPIO
static int _te_gpio_int_set_enable(struct gc9c01_data *data, bool enabled);
#endif

#ifdef CONFIG_PANEL_RESET_GPIO
static const struct gpio_cfg reset_gpio_cfg = CONFIG_PANEL_RESET_GPIO;
#endif

#ifdef CONFIG_PANEL_TE_GPIO
static const struct gpio_cfg te_gpio_cfg = CONFIG_PANEL_TE_GPIO;
#endif

#ifdef CONFIG_PANEL_BACKLIGHT_GPIO
static const struct gpio_cfg backlight_gpio_cfg = CONFIG_PANEL_BACKLIGHT_GPIO;
#endif

#ifdef CONFIG_PANEL_POWER_GPIO
static const struct gpio_cfg power_gpio_cfg = CONFIG_PANEL_POWER_GPIO;
#endif

static const PANEL_VIDEO_PORT_DEFINE(gc9c01_videoport);
static const PANEL_VIDEO_MODE_DEFINE(gc9c01_videomode, PIXEL_FORMAT_RGB_565);


static void gc9c01_transmit(struct gc9c01_data *data, int cmd,
		const uint8_t *tx_data, size_t tx_count)
{
	if (gc9c01_videoport.type != DISPLAY_PORT_SPI_QUAD) {
		display_controller_write_config(data->lcdc_dev, cmd, tx_data, tx_count);
	} else {
		display_controller_write_config(data->lcdc_dev, (0x02 << 24) | (cmd << 8), tx_data, tx_count);
	}
}

static void gc9c01_transmit_p1(struct gc9c01_data *data, int cmd, uint8_t tx_data)
{
	gc9c01_transmit(data, cmd, &tx_data, 1);
}

static void gc9c01_apply_brightness(struct gc9c01_data *data, uint8_t brightness)
{
	if (data->brightness == brightness)
		return;

#if CONFIG_PANEL_BACKLIGHT_CTRL
	if (data->has_refreshed_2 || brightness == 0) {
	#ifdef CONFIG_PANEL_BACKLIGHT_PWM_CHAN
		pwm_pin_set_cycles(data->pwm_dev, CONFIG_PANEL_BACKLIGHT_PWM_CHAN, 255, brightness, 1);
	#else
		gpio_pin_set(data->backlight_gpio_dev, backlight_gpio_cfg.gpion, brightness ? 1 : 0);
	#endif
		data->brightness = brightness;
	}

	/* delay 1 vsync after blank off */
	if (data->has_refreshed)
		data->has_refreshed_2 = 1;
#endif
}

static int gc9c01_blanking_on(const struct device *dev)
{
	struct gc9c01_data *driver = (struct gc9c01_data *)dev->data;

#ifdef CONFIG_PM_DEVICE
	if (driver->pm_state != PM_DEVICE_STATE_ACTIVE &&
		driver->pm_state != PM_DEVICE_STATE_LOW_POWER)
		return -EPERM;
#endif

	if (driver->blanking_off == 0)
		return 0;

	LOG_INF("display blanking on");
	driver->blanking_off = 0;

#ifdef CONFIG_PANEL_BACKLIGHT_CTRL
	driver->has_refreshed = 0;
	driver->has_refreshed_2 = 0;
#endif

#ifdef CONFIG_PANEL_TE_GPIO
	_te_gpio_int_set_enable(driver, false);
#endif

	gc9c01_apply_brightness(driver, 0);
	gc9c01_transmit(driver, GC9C01_CMD_DISP_OFF, NULL, 0);

	return 0;
}

static int gc9c01_blanking_off(const struct device *dev)
{
	struct gc9c01_data *driver = (struct gc9c01_data *)dev->data;

#ifdef CONFIG_PM_DEVICE
	if (driver->pm_state != PM_DEVICE_STATE_ACTIVE &&
		driver->pm_state != PM_DEVICE_STATE_LOW_POWER)
		return -EPERM;
#endif

	if (driver->blanking_off == 1)
		return 0;

	LOG_INF("display blanking off");
	driver->blanking_off = 1;

	gc9c01_transmit(driver, GC9C01_CMD_DISP_ON, NULL, 0);

	k_sleep(K_MSEC(120));

#ifdef CONFIG_PANEL_TE_GPIO
	_te_gpio_int_set_enable(driver, true);
#endif

	return 0;
}

static int gc9c01_read(const struct device *dev,
			const uint16_t x,
			const uint16_t y,
			const struct display_buffer_descriptor *desc,
			void *buf)
{
	return -ENOTSUP;
}

static void gc9c01_set_mem_area(struct gc9c01_data *data, const uint16_t x,
				 const uint16_t y, const uint16_t w, const uint16_t h)
{
	uint16_t cmd_data[2];

	uint16_t ram_x = x;
	uint16_t ram_y = y;

	cmd_data[0] = sys_cpu_to_be16(ram_x);
	cmd_data[1] = sys_cpu_to_be16(ram_x + w - 1);
	gc9c01_transmit(data, GC9C01_CMD_CASET, (uint8_t *)&cmd_data[0], 4);

	cmd_data[0] = sys_cpu_to_be16(ram_y);
	cmd_data[1] = sys_cpu_to_be16(ram_y + h - 1);
	gc9c01_transmit(data, GC9C01_CMD_RASET, (uint8_t *)&cmd_data[0], 4);
}

static int gc9c01_write(const struct device *dev,
			 const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	struct gc9c01_data *data = (struct gc9c01_data *)dev->data;

	if (desc->pitch < desc->width)
		return -EDOM;

	if (k_is_in_isr()) {
		if (data->transfering) {
			LOG_WRN("last post not finished");
			return -ETIME;
		}
	} else {
		unsigned int key = irq_lock();
		data->waiting = data->transfering;
		irq_unlock(key);

		if (data->waiting) {
			k_sem_take(&data->write_sem, K_FOREVER);
			data->waiting = 0;
		}
	}

	data->transfering = 1;

	sys_trace_u32x4(SYS_TRACE_ID_LCD_POST,
			x, y, x + desc->width - 1, y + desc->height - 1);

	gc9c01_set_mem_area(data, x, y, desc->width, desc->height);

	if (gc9c01_videoport.type != DISPLAY_PORT_SPI_QUAD) {
		gc9c01_transmit(data, GC9C01_CMD_RAMWR, 0, 0);
	}

	display_controller_set_source(data->lcdc_dev, DISPLAY_CONTROLLER_SOURCE_DMA, NULL);

	return display_controller_write_pixels(data->lcdc_dev, (0x32 << 24 | GC9C01_CMD_RAMWR << 8), desc, buf);
}

static void *gc9c01_get_framebuffer(const struct device *dev)
{
	return NULL;
}

static int gc9c01_set_brightness(const struct device *dev,
			   const uint8_t brightness)
{
	struct gc9c01_data *data = (struct gc9c01_data *)dev->data;

	if (data->blanking_off == 0)
		return -EPERM;

	if (brightness == data->pending_brightness)
		return 0;

	LOG_INF("display set_brightness %u", brightness);
	data->pending_brightness = brightness;

	/* delayed set in TE interrupt handler */

	return 0;
}

static int gc9c01_set_contrast(const struct device *dev,
			 const uint8_t contrast)
{
	return -ENOTSUP;
}

static void gc9c01_get_capabilities(const struct device *dev,
			      struct display_capabilities *capabilities)
{
	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = gc9c01_videomode.hactive;
	capabilities->y_resolution = gc9c01_videomode.vactive;
	capabilities->vsync_period = 1000000u / gc9c01_videomode.refresh_rate;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
	capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;

#ifdef CONFIG_PANEL_TE_GPIO
	capabilities->screen_info = SCREEN_INFO_VSYNC;
#endif
}

static int gc9c01_set_pixel_format(const struct device *dev,
			     const enum display_pixel_format pixel_format)
{
	if (pixel_format == PIXEL_FORMAT_RGB_565) {
		return 0;
	}

	LOG_ERR("Pixel format change not implemented");
	return -ENOTSUP;
}

static int gc9c01_set_orientation(const struct device *dev,
			    const enum display_orientation orientation)
{
	struct gc9c01_data *data = (struct gc9c01_data *)dev->data;
	uint8_t access = 0;

	switch (orientation) {
	case DISPLAY_ORIENTATION_ROTATED_90:
		access = GC9C01_RAMACC_ROWCOL_SWAP | GC9C01_RAMACC_COL_INV;
		break;
	case DISPLAY_ORIENTATION_ROTATED_180:
		access = GC9C01_RAMACC_COL_INV | GC9C01_RAMACC_ROW_INV;
		break;
	case DISPLAY_ORIENTATION_ROTATED_270:
		access = GC9C01_RAMACC_ROWCOL_SWAP | GC9C01_RAMACC_ROW_INV;
		break;
	case DISPLAY_ORIENTATION_NORMAL:
	default:
		break;
	}

	/* This command has no effect when Tearing Effect output is already ON */
	gc9c01_transmit(data, GC9C01_CMD_RAMACC, &access, 1);

	LOG_INF("Changing display orientation %d", orientation);
	return 0;
}

static void gc9c01_reset_display(struct gc9c01_data *data)
{
	LOG_DBG("Resetting display");

#ifdef CONFIG_PANEL_RESET_GPIO
	gpio_pin_set(data->reset_gpio_dev, reset_gpio_cfg.gpion, 1);
#endif

#ifdef CONFIG_PANEL_POWER_GPIO
	gpio_pin_set(data->power_gpio_dev, power_gpio_cfg.gpion, 1);
#endif

#ifdef CONFIG_PANEL_RESET_GPIO
	k_sleep(K_MSEC(10));
	gpio_pin_set(data->reset_gpio_dev, reset_gpio_cfg.gpion, 0);
	k_sleep(K_MSEC(10));
#endif
}

static int gc9c01_set_te(struct gc9c01_data *data)
{
	if (gc9c01_videomode.flags & (DISPLAY_FLAGS_TE_HIGH | DISPLAY_FLAGS_TE_LOW)) {
		uint8_t tmp[2];

		tmp[0] = 0x10;
		gc9c01_transmit(data, 0x84, tmp, 1);

		tmp[0] = 0x02; /* pulse width: 3 line time */
		tmp[1] = (gc9c01_videomode.flags & DISPLAY_FLAGS_TE_LOW) ? 1 : 0;
		gc9c01_transmit(data, GC9C01_CMD_TECTRL, tmp, 2);

		sys_put_be16(CONFIG_PANEL_TE_SCANLINE, tmp);
		gc9c01_transmit(data, GC9C01_CMD_TESET, tmp, 2);

		tmp[0] = 0; /* only output vsync */
		gc9c01_transmit(data, GC9C01_CMD_TE_ON, tmp, 1);
	} else {
		gc9c01_transmit(data, GC9C01_CMD_TE_OFF, NULL, 0);
	}

	return 0;
}

static void gc9c01_lcd_init(struct gc9c01_data *p_gc9c01)
{
	gc9c01_transmit(p_gc9c01, 0x11, NULL, 0);
	k_sleep(K_MSEC(50));

	// internal reg enable
	gc9c01_transmit(p_gc9c01, GC9C01_CMD_INTERREG_EN1, NULL, 0);
	gc9c01_transmit(p_gc9c01, GC9C01_CMD_INTERREG_EN2, NULL, 0);

	//reg_en for 70\74
	gc9c01_transmit_p1(p_gc9c01, 0x80, 0x11);
	//reg_en for 7C\7D\7E
	gc9c01_transmit_p1(p_gc9c01, 0x81, 0x70);
	//reg_en for 90\93
	gc9c01_transmit_p1(p_gc9c01, 0x82, 0x09);
	//reg_en for 98\99
	gc9c01_transmit_p1(p_gc9c01, 0x83, 0x03);
	//reg_en for B5
	gc9c01_transmit_p1(p_gc9c01, 0x84, 0x20);
	//reg_en for B9\BE
	gc9c01_transmit_p1(p_gc9c01, 0x85, 0x42);
	//reg_en for C2~7
	gc9c01_transmit_p1(p_gc9c01, 0x86, 0xfc);
	//reg_en for C8\CB
	gc9c01_transmit_p1(p_gc9c01, 0x87, 0x09);
	//reg_en for EC
	gc9c01_transmit_p1(p_gc9c01, 0x89, 0x10);
	//reg_en for F0~3\F6
	gc9c01_transmit_p1(p_gc9c01, 0x8A, 0x4f);
	//reg_en for 60\63\64\66
	gc9c01_transmit_p1(p_gc9c01, 0x8C, 0x59);
	//reg_en for 68\6C\6E
	gc9c01_transmit_p1(p_gc9c01, 0x8D, 0x51);
	//reg_en for A1~3\A5\A7
	gc9c01_transmit_p1(p_gc9c01, 0x8E, 0xae);
	//reg_en for AC~F\A8\A9
	gc9c01_transmit_p1(p_gc9c01, 0x8F, 0xf3);

	gc9c01_transmit_p1(p_gc9c01, GC9C01_CMD_RAMACC, 0x00);
	// 565 frame
	gc9c01_transmit_p1(p_gc9c01, GC9C01_CMD_COLMOD, 0x05);
	// 2COL
	gc9c01_transmit_p1(p_gc9c01, GC9C01_CMD_INVERSION, 0x77);

	// rtn 60Hz
	const uint8_t rtn_data[] = { 0x01, 0x80, 0x00, 0x00, 0x00, 0x00 };
	gc9c01_transmit(p_gc9c01, 0x74, rtn_data, sizeof(rtn_data));

	// bvdd 3x
	gc9c01_transmit_p1(p_gc9c01, 0x98, 0x3E);
	// bvee -2x
	gc9c01_transmit_p1(p_gc9c01, 0x99, 0x3E);

	// VBP
	//gc9c01_transmit_p1(p_gc9c01, 0xC3, 0x2A);
	gc9c01_transmit_p1(p_gc9c01, 0xC3, 0x3A + 4);
	// VBN
	//gc9c01_transmit_p1(p_gc9c01, 0xC4, 0x18);
	gc9c01_transmit_p1(p_gc9c01, 0xC4, 0x16 + 4);

	const uint8_t data_0xA1A2[] = { 0x01, 0x04 };
	gc9c01_transmit(p_gc9c01, 0xA1, data_0xA1A2, sizeof(data_0xA1A2));
	gc9c01_transmit(p_gc9c01, 0xA2, data_0xA1A2, sizeof(data_0xA1A2));

	// IREF
	gc9c01_transmit_p1(p_gc9c01, 0xA9, 0x1C);

	const uint8_t data_0xA5[] = { 0x11, 0x09 }; //VDDMA,VDDML
	gc9c01_transmit(p_gc9c01, 0xA5, data_0xA5, sizeof(data_0xA5));

	// RTERM
	gc9c01_transmit_p1(p_gc9c01, 0xB9, 0x8A);
	//VBG_BUF, DVDD
	gc9c01_transmit_p1(p_gc9c01, 0xA8, 0x5E);
	gc9c01_transmit_p1(p_gc9c01, 0xA7, 0x40);
	//VDDSOU ,VDDGM
	gc9c01_transmit_p1(p_gc9c01, 0xAF, 0x73);
	//VREE,VRDD
	gc9c01_transmit_p1(p_gc9c01, 0xAE, 0x44);
	//VRGL ,VDDSF
	gc9c01_transmit_p1(p_gc9c01, 0xAD, 0x38);
	//VRGL ,VDDSF (adjust refresh rate about 60.1~60.6 fps)
	gc9c01_transmit_p1(p_gc9c01, 0xA3, 0x67);
	//VREG_VREF
	gc9c01_transmit_p1(p_gc9c01, 0xC2, 0x02);
	//VREG1A
	gc9c01_transmit_p1(p_gc9c01, 0xC5, 0x11);
	//VREG1B
	gc9c01_transmit_p1(p_gc9c01, 0xC6, 0x0E);
	//VREG2A
	gc9c01_transmit_p1(p_gc9c01, 0xC7, 0x13);
	//VREG2B
	gc9c01_transmit_p1(p_gc9c01, 0xC8, 0x0D);

	//bvdd ref_ad
	gc9c01_transmit_p1(p_gc9c01, 0xCB, 0x02);

	const uint8_t data_0x7C[] = { 0xB6, 0x26 };
	gc9c01_transmit(p_gc9c01, 0x7C, data_0x7C, sizeof(data_0x7C));

	//VGLO
	gc9c01_transmit_p1(p_gc9c01, 0xAC, 0x24);
	//EPF=2
	gc9c01_transmit_p1(p_gc9c01, 0xF6, 0x80);

	//gip start
	const uint8_t data_0xB5[] = { 0x09, 0x09 }; //VFP VBP
	gc9c01_transmit(p_gc9c01, 0xB5, data_0xB5, sizeof(data_0xB5));

	const uint8_t data_0x60[] = { 0x38, 0x0B, 0x5B, 0x56 }; //STV1&2
	gc9c01_transmit(p_gc9c01, 0x60, data_0x60, sizeof(data_0x60));

	const uint8_t data_0x63[] = { 0x3A, 0xE0, 0x5B, 0x56 }; //STV3&4
	gc9c01_transmit(p_gc9c01, 0x63, data_0x63, sizeof(data_0x63));

	const uint8_t data_0x64[] = { 0x38, 0x0D, 0x72, 0xDD, 0x5B, 0x56 }; //CLK_group1
	gc9c01_transmit(p_gc9c01, 0x64, data_0x64, sizeof(data_0x64));

	const uint8_t data_0x66[] = { 0x38, 0x11, 0x72, 0xE1, 0x5B, 0x56 }; //CLK_group1
	gc9c01_transmit(p_gc9c01, 0x66, data_0x66, sizeof(data_0x66));

	const uint8_t data_0x68[] = { 0x3B, 0x08, 0x08, 0x00, 0x08, 0x29, 0x5B }; //FLC&FLV 1~2
	gc9c01_transmit(p_gc9c01, 0x68, data_0x68, sizeof(data_0x68));

	const uint8_t data_0x6E[] = {
		0x00, 0x00, 0x00, 0x07, 0x01, 0x13, 0x11, 0x0B, 0x09, 0x16, 0x15, 0x1D,
		0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x1D, 0x15, 0x16, 0x0A,
		0x0C, 0x12, 0x14, 0x02, 0x08, 0x00, 0x00, 0x00,
	};
	gc9c01_transmit(p_gc9c01, 0x6E, data_0x6E, sizeof(data_0x6E));

	//gip end

	//SOU_BIAS_FIX
	gc9c01_transmit_p1(p_gc9c01, 0xBE, 0x11);

	const uint8_t data_0x6C[] = { 0xCC, 0x0C, 0xCC, 0x84, 0xCC, 0x04, 0x50 }; //precharge GATE
	gc9c01_transmit(p_gc9c01, 0x6C, data_0x6C, sizeof(data_0x6C));

	gc9c01_transmit_p1(p_gc9c01, 0x7D, 0x72);
	gc9c01_transmit_p1(p_gc9c01, 0x7E, 0x38);

	const uint8_t data_0x70[] = { 0x02, 0x03, 0x09, 0x05, 0x0C, 0x06, 0x09, 0x05, 0x0C, 0x06 };
	gc9c01_transmit(p_gc9c01, 0x70, data_0x70, sizeof(data_0x70));

	const uint8_t data_0x90[] = { 0x06, 0x06, 0x05, 0x06 };
	gc9c01_transmit(p_gc9c01, 0x90, data_0x90, sizeof(data_0x90));

	const uint8_t data_0x93[] = { 0x45, 0xFF, 0x00 };
	gc9c01_transmit(p_gc9c01, 0x93, data_0x93, sizeof(data_0x93));

	const uint8_t data_0xF0[] = { 0x46, 0x0B, 0x0F, 0x0A, 0x10, 0x3F };
	gc9c01_transmit(p_gc9c01, GC9C01_CMD_GAMSET1, data_0xF0, sizeof(data_0xF0));

	const uint8_t data_0xF1[] = { 0x52, 0x9A, 0x4F, 0x36, 0x37, 0xFF };
	gc9c01_transmit(p_gc9c01, GC9C01_CMD_GAMSET2, data_0xF1, sizeof(data_0xF1));

	const uint8_t data_0xF2[] = { 0x46, 0x0B, 0x0F, 0x0A, 0x10, 0x3F };
	gc9c01_transmit(p_gc9c01, GC9C01_CMD_GAMSET3, data_0xF2, sizeof(data_0xF2));

	const uint8_t data_0xF3[] = { 0x52, 0x9A, 0x4F, 0x36, 0x37, 0xFF };
	gc9c01_transmit(p_gc9c01, GC9C01_CMD_GAMSET4, data_0xF3, sizeof(data_0xF3));
}

static int gc9c01_register_callback(const struct device *dev,
					  const struct display_callback *callback)
{
	struct gc9c01_data *data = (struct gc9c01_data *)dev->data;

	if (data->callback == NULL) {
		data->callback = callback;
		return 0;
	}

	return -EBUSY;
}

static int gc9c01_unregister_callback(const struct device *dev,
					  const struct display_callback *callback)
{
	struct gc9c01_data *data = (struct gc9c01_data *)dev->data;

	if (data->callback == callback) {
		data->callback = NULL;
		return 0;
	}

	return -EINVAL;
}

static void gc9c01_prepare_de_transfer(void *arg, const display_rect_t *area)
{
	struct device *dev = arg;
	struct gc9c01_data *data = (struct gc9c01_data *)dev->data;

	data->refr_desc.pixel_format = PIXEL_FORMAT_RGB_565;
	data->refr_desc.width = area->w;
	data->refr_desc.height = area->h;
	data->refr_desc.pitch = area->w;

	sys_trace_u32x4(SYS_TRACE_ID_LCD_POST,
			area->x, area->y, area->x + area->w - 1, area->y + area->h - 1);

	data->transfering = 1;

	gc9c01_set_mem_area(data, area->x, area->y, area->w, area->h);

	if (gc9c01_videoport.type != DISPLAY_PORT_SPI_QUAD) {
		gc9c01_transmit(data, GC9C01_CMD_RAMWR, 0, 0);
	}
}

static void gc9c01_start_de_transfer(void *arg)
{
	const struct device *dev = arg;
	struct gc9c01_data *data = (struct gc9c01_data *)dev->data;

	/* source from DE */
	display_controller_set_source(data->lcdc_dev, DISPLAY_CONTROLLER_SOURCE_ENGINE, NULL);
	display_controller_write_pixels(data->lcdc_dev, (0x32 << 24 | GC9C01_CMD_RAMWR << 8),
			&data->refr_desc, NULL);
}

static void gc9c01_complete_transfer(void *arg)
{
	const struct device *dev = arg;
	struct gc9c01_data *data = (struct gc9c01_data *)dev->data;

	sys_trace_end_call(SYS_TRACE_ID_LCD_POST);

	data->transfering = 0;
	if (data->waiting)
		k_sem_give(&data->write_sem);

	if (data->callback && data->callback->complete) {
		data->callback->complete(data->callback);
	}

#if CONFIG_PANEL_BACKLIGHT_CTRL
	/* at least 1 part has refreshed */
	data->has_refreshed = 1;
#endif
}

DEVICE_DECLARE(gc9c01);

#ifdef CONFIG_PM_DEVICE
static void gc9c01_apply_pm_post_change(struct gc9c01_data *data)
{
	if (data->pm_post_changed == 0) {
		return;
	}

	data->pm_post_changed = 0;
}
#endif /* CONFIG_PM_DEVICE */

#ifdef CONFIG_PANEL_TE_GPIO
static int _te_gpio_int_set_enable(struct gc9c01_data *data, bool enabled)
{
	if (gpio_pin_interrupt_configure(data->te_gpio_dev, te_gpio_cfg.gpion,
				enabled ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE)) {
		LOG_ERR("Couldn't config te interrupt (en=%d)", enabled);
		return -ENODEV;
	}

	return 0;
}

static void _te_gpio_callback_handler(const struct device *port,
		struct gpio_callback *cb, gpio_port_pins_t pins)
{
	const struct device *dev = DEVICE_GET(gc9c01);
	struct gc9c01_data *data = (struct gc9c01_data *)dev->data;
	uint32_t timestamp = k_cycle_get_32();

	sys_trace_void(SYS_TRACE_ID_VSYNC);

	gc9c01_apply_brightness(data, data->pending_brightness);
#ifdef CONFIG_PM_DEVICE
	gc9c01_apply_pm_post_change(data);
#endif

	if (data->callback && data->callback->vsync) {
		data->callback->vsync(data->callback, timestamp);
	}

	sys_trace_end_call(SYS_TRACE_ID_VSYNC);
}
#endif /* CONFIG_PANEL_TE_GPIO */

static int gc9c01_panel_init(const struct device *dev)
{
	struct gc9c01_data *data = (struct gc9c01_data *)dev->data;

	gc9c01_reset_display(data);
	gc9c01_lcd_init(data);
	gc9c01_set_orientation(dev, CONFIG_PANEL_ORIENTATION / 90);
	gc9c01_set_te(data);

	/* Display On */
	gc9c01_blanking_off(dev);

	return 0;
}

static void gc9c01_resume(struct k_work *work);

static int gc9c01_init(const struct device *dev)
{
	struct gc9c01_data *data = (struct gc9c01_data *)dev->data;
	struct device *de_dev = NULL;

#ifdef CONFIG_PANEL_TE_GPIO
	data->te_gpio_dev = device_get_binding(te_gpio_cfg.gpio_dev_name);
	if (gpio_pin_configure(data->te_gpio_dev, te_gpio_cfg.gpion,
				GPIO_INPUT | te_gpio_cfg.flag)) {
		LOG_ERR("Couldn't configure te pin");
		return -ENODEV;
	}

	gpio_init_callback(&data->te_gpio_cb, _te_gpio_callback_handler, BIT(te_gpio_cfg.gpion));
	gpio_add_callback(data->te_gpio_dev, &data->te_gpio_cb);
#endif

#ifdef CONFIG_PANEL_BACKLIGHT_PWM_CHAN
	data->pwm_dev = device_get_binding(CONFIG_PWM_NAME);
	if (data->pwm_dev == NULL) {
		LOG_ERR("Couldn't find pwm device\n");
		return -ENODEV;
	}

	pwm_pin_set_cycles(data->pwm_dev, CONFIG_PANEL_BACKLIGHT_PWM_CHAN, 255, 0, 1);
#elif defined(CONFIG_PANEL_BACKLIGHT_GPIO)
	data->backlight_gpio_dev = device_get_binding(backlight_gpio_cfg.gpio_dev_name);
	if (gpio_pin_configure(data->backlight_gpio_dev, backlight_gpio_cfg.gpion,
				GPIO_OUTPUT_INACTIVE | backlight_gpio_cfg.flag)) {
		LOG_ERR("Couldn't configure backlight pin");
		return -ENODEV;
	}
#endif

#ifdef CONFIG_PANEL_RESET_GPIO
	data->reset_gpio_dev = device_get_binding(reset_gpio_cfg.gpio_dev_name);
	if (gpio_pin_configure(data->reset_gpio_dev, reset_gpio_cfg.gpion,
				GPIO_OUTPUT_ACTIVE | reset_gpio_cfg.flag)) {
		LOG_ERR("Couldn't configure reset pin");
		return -ENODEV;
	}
#endif

#ifdef CONFIG_PANEL_POWER_GPIO
	data->power_gpio_dev = device_get_binding(power_gpio_cfg.gpio_dev_name);
	if (gpio_pin_configure(data->power_gpio_dev, power_gpio_cfg.gpion,
				GPIO_OUTPUT_ACTIVE | power_gpio_cfg.flag)) {
		LOG_ERR("Couldn't configure power pin");
		return -ENODEV;
	}
#endif

	data->lcdc_dev = (struct device *)device_get_binding(CONFIG_LCDC_DEV_NAME);
	if (data->lcdc_dev == NULL) {
		LOG_ERR("Could not get LCD controller device");
		return -EPERM;
	}

	display_controller_enable(data->lcdc_dev, &gc9c01_videoport);
	display_controller_control(data->lcdc_dev,
			DISPLAY_CONTROLLER_CTRL_COMPLETE_CB, gc9c01_complete_transfer, (void *)dev);
	display_controller_set_mode(data->lcdc_dev, &gc9c01_videomode);

	de_dev = (struct device *)device_get_binding(CONFIG_DISPLAY_ENGINE_DEV_NAME);
	if (de_dev == NULL) {
		LOG_WRN("Could not get display engine device");
	} else {
		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_PORT, (void *)&gc9c01_videoport, NULL);
		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_MODE, (void *)&gc9c01_videomode, NULL);
		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_PREPARE_CB, gc9c01_prepare_de_transfer, (void *)dev);
		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_START_CB, gc9c01_start_de_transfer, (void *)dev);
	}

	k_sem_init(&data->write_sem, 1, 1);
	data->blanking_off = 0;
	data->pending_brightness = CONFIG_PANEL_BRIGHTNESS;
#ifdef CONFIG_PM_DEVICE
	data->pm_state = PM_DEVICE_STATE_ACTIVE;
	k_work_init(&data->resume_work, gc9c01_resume);
#endif

	gc9c01_panel_init(dev);
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static void gc9c01_enter_sleep(struct gc9c01_data *data)
{
	gc9c01_transmit(data, GC9C01_CMD_SLEEP_IN, NULL, 0);
//	k_sleep(K_MSEC(5));
}

#ifndef CONFIG_PANEL_POWER_GPIO
static void gc9c01_exit_sleep(struct gc9c01_data *data)
{
	gc9c01_transmit(data, GC9C01_CMD_SLEEP_OUT, NULL, 0);
	k_sleep(K_MSEC(120));
}
#endif

static void gc9c01_pm_notify(struct gc9c01_data *data, uint32_t pm_action)
{
	if (data->callback && data->callback->pm_notify) {
		data->callback->pm_notify(data->callback, pm_action);
	}
}

static int gc9c01_pm_eary_suspend(const struct device *dev)
{
	struct gc9c01_data *data = (struct gc9c01_data *)dev->data;

	if (data->pm_state != PM_DEVICE_STATE_ACTIVE &&
		data->pm_state != PM_DEVICE_STATE_LOW_POWER) {
		return -EPERM;
	}

	LOG_INF("panel early-suspend");

	gc9c01_pm_notify(data, PM_DEVICE_ACTION_EARLY_SUSPEND);
	gc9c01_blanking_on(dev);
	gc9c01_enter_sleep(data);

#ifdef CONFIG_PANEL_POWER_GPIO
#ifdef CONFIG_PANEL_RESET_GPIO
	gpio_pin_set(data->reset_gpio_dev, reset_gpio_cfg.gpion, 1);
#endif
	gpio_pin_set(data->power_gpio_dev, power_gpio_cfg.gpion, 0);
#endif /* CONFIG_PANEL_POWER_GPIO */

	data->pm_state = PM_DEVICE_STATE_SUSPENDED;
	return 0;
}

static void gc9c01_resume(struct k_work *work)
{
	const struct device *dev = DEVICE_GET(gc9c01);
	struct gc9c01_data *data = (struct gc9c01_data *)dev->data;

	LOG_INF("panel resuming");

	data->pm_state = PM_DEVICE_STATE_ACTIVE;

#ifdef CONFIG_PANEL_POWER_GPIO
	gc9c01_panel_init(dev);
#else
	gc9c01_exit_sleep(dev->data);
	gc9c01_blanking_off(dev);
#endif /* CONFIG_PANEL_POWER_GPIO */

	gc9c01_pm_notify(data, PM_DEVICE_ACTION_LATE_RESUME);
	LOG_INF("panel active");
}

static int gc9c01_pm_late_resume(const struct device *dev)
{
	struct gc9c01_data *data = (struct gc9c01_data *)dev->data;

	if (data->pm_state != PM_DEVICE_STATE_SUSPENDED &&
		data->pm_state != PM_DEVICE_STATE_OFF) {
		return -EPERM;
	}

	LOG_INF("panel late-resume");
	k_work_submit(&data->resume_work);
	return 0;
}

static int gc9c01_pm_enter_low_power(const struct device *dev)
{
	struct gc9c01_data *data = (struct gc9c01_data *)dev->data;

	if (data->pm_state != PM_DEVICE_STATE_ACTIVE) {
		return -EPERM;
	}

	data->pm_state = PM_DEVICE_STATE_LOW_POWER;
	data->pm_post_changed = 1;
	gc9c01_pm_notify(data, PM_DEVICE_ACTION_LOW_POWER);

	LOG_INF("panel enter low-power");
	return 0;
}

static int gc9c01_pm_exit_low_power(const struct device *dev)
{
	struct gc9c01_data *data = (struct gc9c01_data *)dev->data;

	if (data->pm_state != PM_DEVICE_STATE_LOW_POWER) {
		return -EPERM;
	}

	data->pm_state = PM_DEVICE_STATE_ACTIVE;
	data->pm_post_changed = 1;
	gc9c01_apply_pm_post_change(data);

	LOG_INF("panel exit low-power");
	return 0;
}

static int gc9c01_pm_control(const struct device *dev, enum pm_device_action action)
{
	struct gc9c01_data *data = (struct gc9c01_data *)dev->data;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_EARLY_SUSPEND:
		if (soc_get_aod_mode()) {
			ret = gc9c01_pm_enter_low_power(dev);
		} else {
			ret = gc9c01_pm_eary_suspend(dev);
		}
		break;

	case PM_DEVICE_ACTION_LATE_RESUME:
		if (data->pm_state == PM_DEVICE_STATE_LOW_POWER) {
			ret = gc9c01_pm_exit_low_power(dev);
		} else {
			ret = gc9c01_pm_late_resume(dev);
		}
		break;

	case PM_DEVICE_ACTION_TURN_OFF:
		gc9c01_pm_eary_suspend(dev);
		data->pm_state = PM_DEVICE_STATE_OFF;
		LOG_INF("panel turn-off");
		break;

	case PM_DEVICE_ACTION_RESUME:
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_FORCE_SUSPEND:
	default:
		break;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static const struct display_driver_api gc9c01_api = {
	.blanking_on = gc9c01_blanking_on,
	.blanking_off = gc9c01_blanking_off,
	.write = gc9c01_write,
	.read = gc9c01_read,
	.get_framebuffer = gc9c01_get_framebuffer,
	.set_brightness = gc9c01_set_brightness,
	.set_contrast = gc9c01_set_contrast,
	.get_capabilities = gc9c01_get_capabilities,
	.set_pixel_format = gc9c01_set_pixel_format,
	.set_orientation = gc9c01_set_orientation,
	.register_callback = gc9c01_register_callback,
	.unregister_callback = gc9c01_unregister_callback,
};

static struct gc9c01_data gc9c01_data;

#if IS_ENABLED(CONFIG_PANEL)
DEVICE_DEFINE(gc9c01, CONFIG_LCD_DISPLAY_DEV_NAME, &gc9c01_init,
			gc9c01_pm_control, &gc9c01_data, NULL, POST_KERNEL,
			60, &gc9c01_api);
#endif
