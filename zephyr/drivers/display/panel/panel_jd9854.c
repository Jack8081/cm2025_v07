/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2019 Marc Reilly
 * Copyright (c) 2019 PHYTEC Messtechnik GmbH
 * Copyright (c) 2020 Endian Technologies AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_jd9854.h"
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

#define LOG_LEVEL CONFIG_DISPLAY_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(panel_jd9854);

#ifdef CONFIG_PANEL_TE_GPIO
static const struct gpio_cfg te_gpio_cfg = CONFIG_PANEL_TE_GPIO;
#endif
static const struct gpio_cfg reset_gpio_cfg = CONFIG_PANEL_RESET_GPIO;
#ifdef CONFIG_PANEL_POWER_GPIO
static const struct gpio_cfg power_gpio_cfg = CONFIG_PANEL_POWER_GPIO;
#endif
#ifdef CONFIG_PANEL_BACKLIGHT_GPIO
static const struct gpio_cfg backlight_gpio_cfg = CONFIG_PANEL_BACKLIGHT_GPIO;
#endif

static const struct acts_pin_config jd9854_pin_config[] = { CONFIG_PANEL_MFP };

#define jd9854_PIXEL_SIZE 2

static const PANEL_VIDEO_PORT_DEFINE(gc9c01_videoport);
static const PANEL_VIDEO_MODE_DEFINE(gc9c01_videomode, PIXEL_FORMAT_RGB_565);

struct k_timer jd9854_timer;

struct jd9854_data {
	const struct device *lcdc_dev;
	const struct display_callback *callback[2];
	display_rect_t refresh_area;

#ifdef CONFIG_PANEL_TE_GPIO
	const struct device *te_gpio_dev;
	struct gpio_callback te_gpio_cb;
#endif

#ifdef CONFIG_PANEL_BACKLIGHT_GPIO
	const struct device *backlight_gpio_dev;
#else
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
	uint32_t blank_state;
	uint8_t blank_off_flag;
};

static void jd9854_transmit(struct jd9854_data *data, int cmd,
		const uint8_t *tx_data, size_t tx_count)
{
	display_controller_write_config(data->lcdc_dev, cmd, tx_data, tx_count);
}

static void jd9854_exit_sleep(struct jd9854_data *data)
{

	jd9854_transmit(data, jd9854_CMD_SLEEP_OUT, NULL, 0);

	k_sleep(K_MSEC(120));
}

static void jd9854_reset_display(struct jd9854_data *data)
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
	k_sleep(K_MSEC(100));
#endif
}

static int jd9854_blanking_on(const struct device *dev)
{
	struct jd9854_data *driver = (struct jd9854_data *)dev->data;

#ifdef CONFIG_PM_DEVICE
	if(driver->pm_state != PM_DEVICE_ACTION_RESUME && driver->pm_state != PM_DEVICE_ACTION_LATE_RESUME)
		return 0;
#endif
		if(driver->blank_state == 0)
		return 0;

#ifdef CONFIG_PANEL_BACKLIGHT_GPIO
	if (driver->backlight_gpio_dev)
		gpio_pin_set(driver->backlight_gpio_dev, backlight_gpio_cfg.gpion, 0);
#elif defined(CONFIG_PANEL_BRIGHTNESS)
	pwm_pin_set_cycles(driver->pwm_dev, CONFIG_PANEL_BACKLIGHT_PWM_CHAN, 255, 0, 1);
#endif
	jd9854_transmit(driver, jd9854_CMD_DISP_OFF, NULL, 0);

	driver->blank_state = 0;
	return 0;
}

static int jd9854_blanking_off(const struct device *dev)
{
	struct jd9854_data *driver = (struct jd9854_data *)dev->data;

#ifdef CONFIG_PM_DEVICE
	if(driver->pm_state != PM_DEVICE_ACTION_RESUME && driver->pm_state != PM_DEVICE_ACTION_LATE_RESUME)
		return 0;
#endif
	if(driver->blank_state == 1)
		return 0;

	jd9854_transmit(driver, jd9854_CMD_DISP_ON, NULL, 0);
#ifdef CONFIG_PANEL_BACKLIGHT_GPIO
	if (driver->backlight_gpio_dev)
		gpio_pin_set(driver->backlight_gpio_dev, backlight_gpio_cfg.gpion, 1);
#elif defined(CONFIG_PANEL_BRIGHTNESS)
	pwm_pin_set_cycles(driver->pwm_dev, CONFIG_PANEL_BACKLIGHT_PWM_CHAN, 255, driver->backlight_brightness, 1);
#endif

	driver->blank_state = 1;


	return 0;
}

static int jd9854_read(const struct device *dev,
			const uint16_t x,
			const uint16_t y,
			const struct display_buffer_descriptor *desc,
			void *buf)
{
	return -ENOTSUP;
}

static void jd9854_set_mem_area(struct jd9854_data *data, const uint16_t x,
				 const uint16_t y, const uint16_t w, const uint16_t h)
{
	uint16_t cmd_data[2];

	uint16_t ram_x = x;
	uint16_t ram_y = y;

	cmd_data[0] = sys_cpu_to_be16(ram_x);
	cmd_data[1] = sys_cpu_to_be16(ram_x + w - 1);
	jd9854_transmit(data, jd9854_CMD_CASET, (uint8_t *)&cmd_data[0], 4);

	cmd_data[0] = sys_cpu_to_be16(ram_y);
	cmd_data[1] = sys_cpu_to_be16(ram_y + h - 1);
	jd9854_transmit(data, jd9854_CMD_RASET, (uint8_t *)&cmd_data[0], 4);
}

static int jd9854_write(const struct device *dev,
			 const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	return -ENOTSUP;
}

static void *jd9854_get_framebuffer(const struct device *dev)
{
	return NULL;
}

static int jd9854_set_brightness(const struct device *dev,
			   const uint8_t brightness)
{
	struct jd9854_data *data = (struct jd9854_data *)dev->data;

	if(brightness > 255)
		return -EINVAL;
#ifdef CONFIG_PANEL_BACKLIGHT_GPIO
	return -ENOTSUP;
#else
	data->backlight_brightness = brightness;
#endif

	return 0;
}

static int jd9854_set_contrast(const struct device *dev,
			 const uint8_t contrast)
{
	return -ENOTSUP;
}

static void jd9854_get_capabilities(const struct device *dev,
			      struct display_capabilities *capabilities)
{
	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = jd9854_videomode.hactive;
	capabilities->y_resolution = jd9854_videomode.vactive;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
	capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
	capabilities->screen_info = SCREEN_INFO_VSYNC;
}

static int jd9854_set_pixel_format(const struct device *dev,
			     const enum display_pixel_format pixel_format)
{
	if (pixel_format == PIXEL_FORMAT_RGB_565) {
		return 0;
	}

	LOG_ERR("Pixel format change not implemented");
	return -ENOTSUP;
}

static int jd9854_set_orientation(const struct device *dev,
			    const enum display_orientation orientation)
{
	struct jd9854_data *data = (struct jd9854_data *)dev->data;
	uint8_t access = 0;

	switch (orientation) {
	case DISPLAY_ORIENTATION_ROTATED_90:
		access = jd9854_RAMACC_ROWCOL_SWAP | jd9854_RAMACC_COL_INV;
		break;
	case DISPLAY_ORIENTATION_ROTATED_180:
		access = jd9854_RAMACC_COL_INV | jd9854_RAMACC_ROW_INV;
		break;
	case DISPLAY_ORIENTATION_ROTATED_270:
		access = jd9854_RAMACC_ROWCOL_SWAP | jd9854_RAMACC_ROW_INV;
		break;
	case DISPLAY_ORIENTATION_NORMAL:
	default:
		break;
	}


	jd9854_transmit(data, jd9854_CMD_RAMACC, &access, 1);

	//LOG_INF("Changing display orientation %d", orientation);
	return 0;
}

static int jd9854_set_te(const struct device *dev)
{
	struct jd9854_data *data = (struct jd9854_data *)dev->data;

#ifdef CONFIG_PANEL_TE_GPIO
	if (data->te_gpio_dev) {
		if (jd9854_videomode.flags & (DISPLAY_FLAGS_TE_LOW | DISPLAY_FLAGS_TE_HIGH)) {
			gpio_add_callback(data->te_gpio_dev, &data->te_gpio_cb);
		} else {
			gpio_remove_callback(data->te_gpio_dev, &data->te_gpio_cb);
		}
	}
#endif

	if (jd9854_videomode.flags & (DISPLAY_FLAGS_TE_HIGH | DISPLAY_FLAGS_TE_LOW)) {
		uint8_t tmp[2];

		tmp[0] = 0x10;
		jd9854_transmit(data, 0x84, tmp, 1);

		tmp[0] = 0x02;
		tmp[1] = (jd9854_videomode.flags & DISPLAY_FLAGS_TE_LOW) ? 1 : 0;
		jd9854_transmit(data, jd9854_CMD_TECTRL, tmp, 2);

		sys_put_be16(CONFIG_PANEL_TE_SCANLINE, tmp);
		jd9854_transmit(data, jd9854_CMD_TESET, tmp, 2);

		tmp[0] = 0;
		jd9854_transmit(data, jd9854_CMD_TE_ON, tmp, 1);
	} else {
		jd9854_transmit(data, jd9854_CMD_TE_OFF, NULL, 0);
	}

	return 0;
}

static void send_cmd(struct jd9854_data *p_jd9854, int cmd)
{
	jd9854_transmit(p_jd9854, cmd, NULL, 0);
}

static void send_dat(struct jd9854_data *p_jd9854, int dat)
{
	jd9854_transmit(p_jd9854, -1, &dat, 1);
}

static void send_formatted_dat(struct jd9854_data *p_jd9854, int dat)
{
	jd9854_transmit(p_jd9854, -2, &dat, 1);
}


void JD9854_Address_Set(struct jd9854_data *p_jd9854, int r_w,int x1,int y1,int x2,int y2)
{
	send_cmd(p_jd9854, 0x2a);
	send_dat(p_jd9854, x1>>8);
	send_dat(p_jd9854, x1);
	send_dat(p_jd9854, x2>>8);
	send_dat(p_jd9854, x2);
	send_cmd(p_jd9854, 0x2b);
	send_dat(p_jd9854, y1>>8);
	send_dat(p_jd9854, y1);
	send_dat(p_jd9854, y2>>8);
	send_dat(p_jd9854, y2);
	if(r_w==0)send_cmd(p_jd9854, 0x2c);
	else send_cmd(p_jd9854, 0x2e);
}

void JD9854_Clear(struct jd9854_data *p_jd9854, int Color)
{
	int i,color_r,color_g,color_b,color_s=0; 
	
	JD9854_Address_Set(p_jd9854, 0,0,0,319,384);
	color_r=Color>>16&0xff;
	color_g=Color>>8&0xff;
	color_b=Color&0xff;
	//------------------------------------------------------
		color_s=color_b |color_g<<8|color_r<<16;
	//------------------------------------------------------
	for(i=0;i<320*385;i++)
		send_formatted_dat(p_jd9854, color_s);
}

static void jd9854_lcd_init(struct jd9854_data *p_jd9854)
{
	send_cmd(p_jd9854, 0x11);
	k_busy_wait(100*1000);

	send_cmd(p_jd9854, 0xDF);//password
	send_dat(p_jd9854, 0x98);
	send_dat(p_jd9854, 0x54);
	send_dat(p_jd9854, 0xEC);

//--------------PAGE0--------------------------
	send_cmd(p_jd9854, 0xDE);
	send_dat(p_jd9854, 0x00);

	//VCOM_SET
	send_cmd(p_jd9854, 0xB2);
	send_dat(p_jd9854, 0x01);
	send_dat(p_jd9854, 0x10); //VCOM

	//Gamma_Set
	send_cmd(p_jd9854, 0xB7);
	send_dat(p_jd9854, 0x10); //VGMP = +5.3V 0x14A
	send_dat(p_jd9854, 0x2C);
	send_dat(p_jd9854, 0x00); //VGSP = +0.0V
	send_dat(p_jd9854, 0x10); //VGMN = -5.3V 0x14A
	send_dat(p_jd9854, 0x2C);
	send_dat(p_jd9854, 0x00); //VGSN = -0.0V

	//DCDC_SEL
	send_cmd(p_jd9854, 0xBB);
	send_dat(p_jd9854, 0x01); //VGH = AVDD+VCI = 5.8V+3.1V= 8.9V ;VGL = -1*VGH = -8.9V; AVDD = 2xVCI = 3.1V*2 = 6.2V
	send_dat(p_jd9854, 0x1A); //AVDD_S = +5.8V (0x1D) ; AVEE = -1xAVDD_S = -5.8V
	send_dat(p_jd9854, 0x44);
	send_dat(p_jd9854, 0x44);
	send_dat(p_jd9854, 0x33);
	send_dat(p_jd9854, 0x33);

	//GATE_POWER
	send_cmd(p_jd9854, 0xCF);
	send_dat(p_jd9854, 0x20); //VGHO = +8V
	send_dat(p_jd9854, 0x50); //VGLO = -8V

	//SET_R_GAMMA
	send_cmd(p_jd9854, 0xC8);
	send_dat(p_jd9854, 0x7F);
	send_dat(p_jd9854, 0x64);
	send_dat(p_jd9854, 0x49);
	send_dat(p_jd9854, 0x2f);
	send_dat(p_jd9854, 0x22);
	send_dat(p_jd9854, 0x13);
	send_dat(p_jd9854, 0x17);
	send_dat(p_jd9854, 0x03);
	send_dat(p_jd9854, 0x20);
	send_dat(p_jd9854, 0x22);
	send_dat(p_jd9854, 0x25);
	send_dat(p_jd9854, 0x45);
	send_dat(p_jd9854, 0x34);
	send_dat(p_jd9854, 0x3d);
	send_dat(p_jd9854, 0x30);
	send_dat(p_jd9854, 0x28);
	send_dat(p_jd9854, 0x18);
	send_dat(p_jd9854, 0x10);
	send_dat(p_jd9854, 0x0E);
	send_dat(p_jd9854, 0x7F);
	send_dat(p_jd9854, 0x64);
	send_dat(p_jd9854, 0x49);
	send_dat(p_jd9854, 0x2F);
	send_dat(p_jd9854, 0x22);
	send_dat(p_jd9854, 0x13);
	send_dat(p_jd9854, 0x17);
	send_dat(p_jd9854, 0x03);
	send_dat(p_jd9854, 0x20);
	send_dat(p_jd9854, 0x22);
	send_dat(p_jd9854, 0x25);
	send_dat(p_jd9854, 0x45);
	send_dat(p_jd9854, 0x34);
	send_dat(p_jd9854, 0x3d);
	send_dat(p_jd9854, 0x30);
	send_dat(p_jd9854, 0x28);
	send_dat(p_jd9854, 0x18);
	send_dat(p_jd9854, 0x10);
	send_dat(p_jd9854, 0x0e);

	//-----------------------------
	// SET page4 TCON & GIP
	//------------------------------
	send_cmd(p_jd9854, 0xDE);
	send_dat(p_jd9854, 0x04);  // page4

	//SETSTBA
	send_cmd(p_jd9854, 0xB2);
	send_dat(p_jd9854, 0x14); //GAP = 1 ;SAP= 4
	send_dat(p_jd9854, 0x14);

	//SETRGBCYC1
	send_cmd(p_jd9854, 0xB8);
	send_dat(p_jd9854, 0x74); //- NEQ PEQ[1:0] -  RGB_INV_NP[2:0]
	send_dat(p_jd9854, 0x44); //- RGB_INV_PI[2:0] -   RGB_INV_I[2:0]
	send_dat(p_jd9854, 0x00); //RGB_N_T2[11:8],RGB_N_T1[11:8]
	send_dat(p_jd9854, 0x01); //RGB_N_T1[7:0],
	send_dat(p_jd9854, 0x01); //RGB_N_T2[7:0],
	send_dat(p_jd9854, 0x00); //RGB_N_T4[11:8],RGB_N_T3[11:8]
	send_dat(p_jd9854, 0x01); //RGB_N_T3[7:0],
	send_dat(p_jd9854, 0x01); //RGB_N_T4[7:0],
	send_dat(p_jd9854, 0x00); //RGB_N_T6[11:8],RGB_N_T5[11:8]
	send_dat(p_jd9854, 0x27); //RGB_N_T5[7:0],
	send_dat(p_jd9854, 0x99); //RGB_N_T6[7:0],
	send_dat(p_jd9854, 0x10); //RGB_N_T8[11:8],RGB_N_T7[11:8]
	send_dat(p_jd9854, 0xA3); //RGB_N_T7[7:0],
	send_dat(p_jd9854, 0x15); //RGB_N_T8[7:0],
	send_dat(p_jd9854, 0x11); //RGB_N_T10[11:8],RGB_N_T9[11:8]
	send_dat(p_jd9854, 0x1F); //RGB_N_T9[7:0],
	send_dat(p_jd9854, 0x91); //RGB_N_T10[7:0],
	send_dat(p_jd9854, 0x21); //RGB_N_T12[11:8],RGB_N_T11[11:8]
	send_dat(p_jd9854, 0x9B); //RGB_N_T11[7:0],
	send_dat(p_jd9854, 0x0D); //RGB_N_T12[7:0],
	send_dat(p_jd9854, 0x22); //RGB_N_T14[11:8],RGB_N_T13[11:8]
	send_dat(p_jd9854, 0x17); //RGB_N_T13[7:0],
	send_dat(p_jd9854, 0x89); //RGB_N_T14[7:0],
	send_dat(p_jd9854, 0x32); //RGB_N_T16[11:8],RGB_N_T15[11:8]
	send_dat(p_jd9854, 0x93); //RGB_N_T15[7:0],
	send_dat(p_jd9854, 0x05); //RGB_N_T16[7:0],
	send_dat(p_jd9854, 0x00); //RGB_N_T18[11:8],RGB_N_T17[11:8]
	send_dat(p_jd9854, 0x00); //RGB_N_T17[7:0],
	send_dat(p_jd9854, 0x00); //RGB_N_T18[7:0],

	//SETRGBCYC2
	send_cmd(p_jd9854, 0xB9);
	send_dat(p_jd9854, 0x00); //-,ENJDT,RGB_JDT2[2:0],ENP_LINE_INV,ENP_FRM_SEL[1:0],
	send_dat(p_jd9854, 0x22); //RGB_N_T20[11:8],RGB_N_T19[11:8],
	send_dat(p_jd9854, 0x08); //RGB_N_T19[7:0],
	send_dat(p_jd9854, 0x3A); //RGB_N_T20[7:0],
	send_dat(p_jd9854, 0x22); //RGB_N_T22[11:8],RGB_N_T21[11:8],
	send_dat(p_jd9854, 0x4B); //RGB_N_T21[7:0],
	send_dat(p_jd9854, 0x7D); //RGB_N_T22[7:0],
	send_dat(p_jd9854, 0x22); //RGB_N_T24[11:8],RGB_N_T23[11:8],
	send_dat(p_jd9854, 0x8D); //RGB_N_T23[7:0],
	send_dat(p_jd9854, 0xBF); //RGB_N_T24[7:0],
	send_dat(p_jd9854, 0x32); //RGB_N_T26[11:8],RGB_N_T25[11:8],
	send_dat(p_jd9854, 0xD0); //RGB_N_T25[7:0],
	send_dat(p_jd9854, 0x02); //RGB_N_T26[7:0],
	send_dat(p_jd9854, 0x33); //RGB_N_T28[11:8],RGB_N_T27[11:8],
	send_dat(p_jd9854, 0x12); //RGB_N_T27[7:0],
	send_dat(p_jd9854, 0x44); //RGB_N_T28[7:0],
	send_dat(p_jd9854, 0x00); //-,-,-,-,RGB_N_TA1[11:8],
	send_dat(p_jd9854, 0x0A); //RGB_N_TA1[7:0],
	send_dat(p_jd9854, 0x00); //RGB_N_TA3[11:8],RGB_N_TA2[11:8],
	send_dat(p_jd9854, 0x0A); //RGB_N_TA2[7:0],
	send_dat(p_jd9854, 0x0A); //RGB_N_TA3[7:0],
	send_dat(p_jd9854, 0x00); //RGB_N_TA5[11:8],RGB_N_TA4[11:8],
	send_dat(p_jd9854, 0x0A); //RGB_N_TA4[7:0],
	send_dat(p_jd9854, 0x0A); //RGB_N_TA5[7:0],
	send_dat(p_jd9854, 0x00); //RGB_N_TA7[11:8],RGB_N_TA6[11:8],
	send_dat(p_jd9854, 0x0A); //RGB_N_TA6[7:0],
	send_dat(p_jd9854, 0x0A); //RGB_N_TA7[7:0],

	//SETRGBCp_jd9854, YC3
	send_cmd(p_jd9854, 0xBA);
	send_dat(p_jd9854, 0x00);//-  -   -   -   -   -   -   -
	send_dat(p_jd9854, 0x00);//RGB_N_TA9[11:8],RGB_N_TA8[11:8]
	send_dat(p_jd9854, 0x07);//RGB_N_TA8[7:0],
	send_dat(p_jd9854, 0x07);//RGB_N_TA9[7:0],
	send_dat(p_jd9854, 0x00);//RGB_N_TA11[11:8],RGB_N_TA10[11:8]
	send_dat(p_jd9854, 0x07);//RGB_N_TA10[7:0],
	send_dat(p_jd9854, 0x07);//RGB_N_TA11[7:0],
	send_dat(p_jd9854, 0x00);//RGB_N_TA13[11:8],RGB_N_TA12[11:8]
	send_dat(p_jd9854, 0x07);//RGB_N_TA12[7:0],
	send_dat(p_jd9854, 0x07);//RGB_N_TA13[7:0],
	send_dat(p_jd9854, 0x00);//RGB_N_TC[11:8],RGB_N_TB[11:8]
	send_dat(p_jd9854, 0x01);//RGB_N_TB[7:0],
	send_dat(p_jd9854, 0x01);//RGB_N_TC[7:0],
	send_dat(p_jd9854, 0x00);//RGB_N_TE[11:8],RGB_N_TD[11:8]
	send_dat(p_jd9854, 0x0A);//RGB_N_TD[7:0],
	send_dat(p_jd9854, 0x01);//RGB_N_TE[7:0],
	send_dat(p_jd9854, 0x00);//- -   -   -   RGB_N_TF[11:8]
	send_dat(p_jd9854, 0x01);//RGB_N_TF[7:0],
	send_dat(p_jd9854, 0x30);//RGB_CHGEN_OFF[11:8],RGB_CHGEN_ON[11:8]
	send_dat(p_jd9854, 0x0A);//RGB_CHGEN_ON[7:0],
	send_dat(p_jd9854, 0x0E);//RGB_CHGEN_OFF[7:0],
	send_dat(p_jd9854, 0x30);//RES_MUX_OFF[11:8],RES_MUX_ON[11:8]
	send_dat(p_jd9854, 0x01);//RES_MUX_ON[7:0],
	send_dat(p_jd9854, 0x0E);//RES_MUX_OFF[7:0],
	send_dat(p_jd9854, 0x00);//- -   -   L2_COND1_INV[12:8],
	send_dat(p_jd9854, 0x00);//- -   -   L2_COND0_INV[12:8],
	send_dat(p_jd9854, 0x00);//L2_COND0_INV[7:0],
	send_dat(p_jd9854, 0x00);//L2_COND1_INV[7:0],

	//SETRGBCYC4
	send_cmd(p_jd9854, 0xBB);
	send_dat(p_jd9854, 0x10);
	send_dat(p_jd9854, 0x66);
	send_dat(p_jd9854, 0x66);
	send_dat(p_jd9854, 0xA0);
	send_dat(p_jd9854, 0x80);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x60);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0xC0);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x40);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);

	//SET_TCON
	send_cmd(p_jd9854, 0xBC);
	send_dat(p_jd9854, 0x18);//1  MUX_SEL =1:6 ,RSO = 360H
	send_dat(p_jd9854, 0x09);//2  LN_NO_MUL2 = 0:Gate line number=LN[10:0]*2 ,LN[10:8] = 0
	send_dat(p_jd9854, 0x81);//3  LN[7:0] =180*2 = 360
	send_dat(p_jd9854, 0x03);//4  PANEL[2:0] = dancing type 2
	send_dat(p_jd9854, 0x00);//5  VFP[11:8],SLT[11:8]
	send_dat(p_jd9854, 0xC4);//6  SLT[7:0] = 1/(60*(360+10+6))/4OSC(19MHZ)
	send_dat(p_jd9854, 0x08);//7  VFP[7:0] = 8
	send_dat(p_jd9854, 0x00);//8  HBP[11:8], VBP[11:8]
	send_dat(p_jd9854, 0x07);//9  VBP[7:0]
	send_dat(p_jd9854, 0x2C);//10 HBP[7:0]
	send_dat(p_jd9854, 0x00);//11 VFP_I[11:8],SLT_I[11:8]
	send_dat(p_jd9854, 0xC4);//12 SLT_I[7:0]
	send_dat(p_jd9854, 0x08);//13 VFP_I[7:0]
	send_dat(p_jd9854, 0x00);//14 HBP_I[11:8],VBP_I[11:8]
	send_dat(p_jd9854, 0x07);//15 VBP_I[7:0]
	send_dat(p_jd9854, 0x2C);//16 HBP_I[7:0]
	send_dat(p_jd9854, 0x82);//17 HBP_NCK[3:0],HFP_NCK[3:0]
	send_dat(p_jd9854, 0x00);//18 TCON_OPT1[15:8]
	send_dat(p_jd9854, 0x03);//19 TCON_OPT1[7:0]
	send_dat(p_jd9854, 0x00);//20 VFP_PI[11:8],SLT_PI[11:8]
	send_dat(p_jd9854, 0xC4);//21 SLT_PI[7:0]
	send_dat(p_jd9854, 0x08);//22 VFP_PI[7:0]
	send_dat(p_jd9854, 0x00);//23 HBP_PI[11:8],VBP_PI[11:8]
	send_dat(p_jd9854, 0x07);//24 VBP_PI[7:0]
	send_dat(p_jd9854, 0x2C);//25 HBP_PI[7:0]

	//-------------------GIP----------------------
	//SET_GIP_EQ
	send_cmd(p_jd9854, 0xC4);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x02);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x02);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x02);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);

	//SET_GIP_L
	send_cmd(p_jd9854, 0xC5);
	send_dat(p_jd9854, 0x00);//DUMMY
	send_dat(p_jd9854, 0x1F);//0
	send_dat(p_jd9854, 0x1F);//1
	send_dat(p_jd9854, 0x1F);//2
	send_dat(p_jd9854, 0x1F);//3 GAS
	send_dat(p_jd9854, 0x1F);//4 BGAS
	send_dat(p_jd9854, 0x1E);//5 RSTV
	send_dat(p_jd9854, 0x02);//6 CKV4
	send_dat(p_jd9854, 0xC5);//7 CKV2
	send_dat(p_jd9854, 0xC7);//8 SB
	send_dat(p_jd9854, 0xC9);//9
	send_dat(p_jd9854, 0xCB);//10
	send_dat(p_jd9854, 0xCD);//11
	send_dat(p_jd9854, 0xE1);//12
	send_dat(p_jd9854, 0xE3);//13
	send_dat(p_jd9854, 0xE5);//14
	send_dat(p_jd9854, 0x1F);//15
	send_dat(p_jd9854, 0x1F);//16
	send_dat(p_jd9854, 0x1F);//17
	send_dat(p_jd9854, 0x1F);//18
	send_dat(p_jd9854, 0x1F);//19
	send_dat(p_jd9854, 0x1F);//20
	send_dat(p_jd9854, 0x1F);//21
	send_dat(p_jd9854, 0x1F);//22
	send_dat(p_jd9854, 0x1F);//23
	send_dat(p_jd9854, 0x1F);//24
	send_dat(p_jd9854, 0x1F);//25

	//SET_GIP_R
	send_cmd(p_jd9854, 0xC6);
	send_dat(p_jd9854, 0x00);//DUMMY
	send_dat(p_jd9854, 0x1F);//0
	send_dat(p_jd9854, 0x1F);//1
	send_dat(p_jd9854, 0x1F);//2
	send_dat(p_jd9854, 0x1F);//3
	send_dat(p_jd9854, 0x1F);//4
	send_dat(p_jd9854, 0xDF);//5
	send_dat(p_jd9854, 0x00);//6
	send_dat(p_jd9854, 0xC4);//7
	send_dat(p_jd9854, 0xC6);//8
	send_dat(p_jd9854, 0xC8);//9   ASB
	send_dat(p_jd9854, 0xCA);//10  LSTV
	send_dat(p_jd9854, 0xCC);//11  CKV1
	send_dat(p_jd9854, 0xE0);//12  CKV3
	send_dat(p_jd9854, 0xE2);//13  CKH1
	send_dat(p_jd9854, 0xE4);//14  CKH2
	send_dat(p_jd9854, 0x1F);//15  CKH3
	send_dat(p_jd9854, 0x1F);//16  CKH4
	send_dat(p_jd9854, 0x1F);//17  CKH5
	send_dat(p_jd9854, 0x1F);//18  CKH6
	send_dat(p_jd9854, 0x1F);//19
	send_dat(p_jd9854, 0x1F);//20
	send_dat(p_jd9854, 0x1F);//21
	send_dat(p_jd9854, 0x1F);//22
	send_dat(p_jd9854, 0x1F);//23
	send_dat(p_jd9854, 0x1F);//24
	send_dat(p_jd9854, 0x1F);//25

	//SET_GIP_L_GS
	send_cmd(p_jd9854, 0xC7);
	send_dat(p_jd9854, 0x00);//DUMMY
	send_dat(p_jd9854, 0x1F);//0
	send_dat(p_jd9854, 0x1F);//1
	send_dat(p_jd9854, 0x1F);//2
	send_dat(p_jd9854, 0x1F);//3 GAS
	send_dat(p_jd9854, 0x1F);//4 BGAS
	send_dat(p_jd9854, 0xDE);//5 RSTV
	send_dat(p_jd9854, 0x00);//6 CKV4
	send_dat(p_jd9854, 0xC7);//7 CKV2
	send_dat(p_jd9854, 0xC5);//8 SB
	send_dat(p_jd9854, 0xCD);//9
	send_dat(p_jd9854, 0xCB);//10
	send_dat(p_jd9854, 0xC9);//11
	send_dat(p_jd9854, 0x21);//12
	send_dat(p_jd9854, 0x23);//13
	send_dat(p_jd9854, 0x25);//14
	send_dat(p_jd9854, 0x1F);//15
	send_dat(p_jd9854, 0x1F);//16
	send_dat(p_jd9854, 0x1F);//17
	send_dat(p_jd9854, 0x1F);//18
	send_dat(p_jd9854, 0x1F);//19
	send_dat(p_jd9854, 0x1F);//20
	send_dat(p_jd9854, 0x1F);//21
	send_dat(p_jd9854, 0x1F);//22
	send_dat(p_jd9854, 0x1F);//23
	send_dat(p_jd9854, 0x1F);//24
	send_dat(p_jd9854, 0x1F);//25

	//SET_GIP_R_GS
	send_cmd(p_jd9854, 0xC8);
	send_dat(p_jd9854, 0x00);//DUMMY
	send_dat(p_jd9854, 0x1F);//0
	send_dat(p_jd9854, 0x1F);//1
	send_dat(p_jd9854, 0x1F);//2
	send_dat(p_jd9854, 0x1F);//3
	send_dat(p_jd9854, 0x1F);//4
	send_dat(p_jd9854, 0x1F);//5
	send_dat(p_jd9854, 0x02);//6
	send_dat(p_jd9854, 0xC8);//7
	send_dat(p_jd9854, 0xC6);//8
	send_dat(p_jd9854, 0xC4);//9   ASB
	send_dat(p_jd9854, 0xCC);//10  LSTV
	send_dat(p_jd9854, 0xCA);//11  CKV1
	send_dat(p_jd9854, 0x20);//12  CKV3
	send_dat(p_jd9854, 0x22);//13  CKH1
	send_dat(p_jd9854, 0x24);//14  CKH2
	send_dat(p_jd9854, 0x1F);//15  CKH3
	send_dat(p_jd9854, 0x1F);//16  CKH4
	send_dat(p_jd9854, 0x1F);//17  CKH5
	send_dat(p_jd9854, 0x1F);//18  CKH6
	send_dat(p_jd9854, 0x1F);//19
	send_dat(p_jd9854, 0x1F);//20
	send_dat(p_jd9854, 0x1F);//21
	send_dat(p_jd9854, 0x1F);//22
	send_dat(p_jd9854, 0x1F);//23
	send_dat(p_jd9854, 0x1F);//24
	send_dat(p_jd9854, 0x1F);//25

	//SETGIP1
	send_cmd(p_jd9854, 0xC9);
	send_dat(p_jd9854, 0x00);//0
	send_dat(p_jd9854, 0x00);//1
	send_dat(p_jd9854, 0x00);//2
	send_dat(p_jd9854, 0x00);//3   L:GAS
	send_dat(p_jd9854, 0x00);//4   L:BGAS :VGH
	send_dat(p_jd9854, 0x20);//5   L:RSTV
	send_dat(p_jd9854, 0x00);//6   L:CKV4 :VGH
	send_dat(p_jd9854, 0x30);//7   L:CKV2 :VGH
	send_dat(p_jd9854, 0x30);//8   L:SB
	send_dat(p_jd9854, 0x30);//9   R:ASB
	send_dat(p_jd9854, 0x30);//10  R:LSTV
	send_dat(p_jd9854, 0x30);//11  R:CKV1 :VGH
	send_dat(p_jd9854, 0x30);//12  R:CKV3 :VGH
	send_dat(p_jd9854, 0x30);//13  R:CKH1 :VGH
	send_dat(p_jd9854, 0x30);//14  R:CKH2 :VGH
	send_dat(p_jd9854, 0x00);//15  R:CKH3 :VGH
	send_dat(p_jd9854, 0x00);//16  R:CKH4 :VGH
	send_dat(p_jd9854, 0x00);//17  R:CKH5 :VGH
	send_dat(p_jd9854, 0x00);//18  R:CKH6 :VGH
	send_dat(p_jd9854, 0x00);//19
	send_dat(p_jd9854, 0x00);//20
	send_dat(p_jd9854, 0x00);//21
	send_dat(p_jd9854, 0x00);//22
	send_dat(p_jd9854, 0x00);//23
	send_dat(p_jd9854, 0x00);//24
	send_dat(p_jd9854, 0x00);//25

	//SETGIP2
	send_cmd(p_jd9854, 0xCB);
	send_dat(p_jd9854, 0x01);//1  INIT_PORCH
	send_dat(p_jd9854, 0x10);//2  INIT_W
	send_dat(p_jd9854, 0x00);//3
	send_dat(p_jd9854, 0x10);//4
	send_dat(p_jd9854, 0x07);//5  STV_S0
	send_dat(p_jd9854, 0x89);//6
	send_dat(p_jd9854, 0x00);//7
	send_dat(p_jd9854, 0x0A);//8
	send_dat(p_jd9854, 0x80);//9  STV_NUM = 1 , STV_S1
	send_dat(p_jd9854, 0x00);//10
	send_dat(p_jd9854, 0x00);//11 STV1/0_W
	send_dat(p_jd9854, 0x00);//12 STV3/2_W
	send_dat(p_jd9854, 0x00);//13
	send_dat(p_jd9854, 0x07);//14
	send_dat(p_jd9854, 0x00);//15
	send_dat(p_jd9854, 0x00);//16
	send_dat(p_jd9854, 0x00);//17
	send_dat(p_jd9854, 0x20);//18
	send_dat(p_jd9854, 0x23);//19
	send_dat(p_jd9854, 0x90);//20 CKV_W
	send_dat(p_jd9854, 0x00);//21
	send_dat(p_jd9854, 0x08);//22 CKV_S0
	send_dat(p_jd9854, 0x05);//23 CKV0_DUM[7:0]
	send_dat(p_jd9854, 0x00);//24
	send_dat(p_jd9854, 0x00);//25
	send_dat(p_jd9854, 0x05);//26
	send_dat(p_jd9854, 0x10);//27
	send_dat(p_jd9854, 0x01);//28 //END_W
	send_dat(p_jd9854, 0x04);//29
	send_dat(p_jd9854, 0x06);//30
	send_dat(p_jd9854, 0x10);//31
	send_dat(p_jd9854, 0x10);//32

	//SET_GIP_ONOFF
	send_cmd(p_jd9854, 0xD1);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x03);
	send_dat(p_jd9854, 0x60);
	send_dat(p_jd9854, 0x30);
	send_dat(p_jd9854, 0x03);
	send_dat(p_jd9854, 0x18);
	send_dat(p_jd9854, 0x30);//CKV0_OFF[11:8]
	send_dat(p_jd9854, 0x25);//CKV0_ON[7:0]
	send_dat(p_jd9854, 0x0E);//CKV0_OFF[7:0]
	send_dat(p_jd9854, 0x30);
	send_dat(p_jd9854, 0x03);
	send_dat(p_jd9854, 0x18);
	send_dat(p_jd9854, 0x30);
	send_dat(p_jd9854, 0x03);
	send_dat(p_jd9854, 0x18);
	send_dat(p_jd9854, 0x30);
	send_dat(p_jd9854, 0x03);
	send_dat(p_jd9854, 0x18);

	//SET_GIP_ONOFF_WB
	send_cmd(p_jd9854, 0xD2);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x30);//STV_OFF[11:8]
	send_dat(p_jd9854, 0x25);//STV_ON[7:0]
	send_dat(p_jd9854, 0x0E);//STV_OFF[7:0]
	send_dat(p_jd9854, 0x32);
	send_dat(p_jd9854, 0xBC);
	send_dat(p_jd9854, 0x20);
	send_dat(p_jd9854, 0x32);
	send_dat(p_jd9854, 0xBC);
	send_dat(p_jd9854, 0x20);
	send_dat(p_jd9854, 0x32);
	send_dat(p_jd9854, 0xBC);
	send_dat(p_jd9854, 0x20);
	send_dat(p_jd9854, 0x32);
	send_dat(p_jd9854, 0xBC);
	send_dat(p_jd9854, 0x20);
	send_dat(p_jd9854, 0x30);
	send_dat(p_jd9854, 0x10);
	send_dat(p_jd9854, 0x20);
	send_dat(p_jd9854, 0x30);
	send_dat(p_jd9854, 0x10);
	send_dat(p_jd9854, 0x20);
	send_dat(p_jd9854, 0x30);
	send_dat(p_jd9854, 0x10);
	send_dat(p_jd9854, 0x20);
	send_dat(p_jd9854, 0x30);
	send_dat(p_jd9854, 0x10);
	send_dat(p_jd9854, 0x20);

	//SETGIP8_CKH1 CKH_ON/OFF_CKH0-CKH7_odd
	send_cmd(p_jd9854, 0xD4);
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00); //CKH_T2_ODD[11:8],CKH_T1_ODD[11:8]
	send_dat(p_jd9854, 0x03); //CKH_T1_ODD[7:0],
	send_dat(p_jd9854, 0x14); //CKH_T2_ODD[7:0],
	send_dat(p_jd9854, 0x00); //CKH_T4_ODD[11:8],CKH_T3_ODD[11:8]
	send_dat(p_jd9854, 0x03); //CKH_T3_ODD[7:0],
	send_dat(p_jd9854, 0x20); //CKH_T4_ODD[7:0],
	send_dat(p_jd9854, 0x00); //CKH_T6_ODD[11:8],CKH_T5_ODD[11:8]
	send_dat(p_jd9854, 0x27); //CKH_T5_ODD[7:0],
	send_dat(p_jd9854, 0x99); //CKH_T6_ODD[7:0],
	send_dat(p_jd9854, 0x10); //CKH_T8_ODD[11:8],CKH_T7_ODD[11:8]
	send_dat(p_jd9854, 0xA3); //CKH_T7_ODD[7:0],
	send_dat(p_jd9854, 0x15); //CKH_T8_ODD[7:0],
	send_dat(p_jd9854, 0x11); //CKH_T10_ODD[11:8],CKH_T9_ODD[11:8]
	send_dat(p_jd9854, 0x1F); //CKH_T9_ODD[7:0],
	send_dat(p_jd9854, 0x91); //CKH_T10_ODD[7:0],
	send_dat(p_jd9854, 0x21); //CKH_T12_ODD[11:8],CKH_T11_ODD[11:8]
	send_dat(p_jd9854, 0x9B); //CKH_T11_ODD[7:0],
	send_dat(p_jd9854, 0x0D); //CKH_T12_ODD[7:0],
	send_dat(p_jd9854, 0x22); //CKH_T14_ODD[11:8],CKH_T13_ODD[11:8]
	send_dat(p_jd9854, 0x17); //CKH_T13_ODD[7:0],
	send_dat(p_jd9854, 0x89); //CKH_T14_ODD[7:0],
	send_dat(p_jd9854, 0x32); //CKH_T16_ODD[11:8],CKH_T15_ODD[11:8]
	send_dat(p_jd9854, 0x93); //CKH_T15_ODD[7:0],
	send_dat(p_jd9854, 0x05); //CKH_T16_ODD[7:0],
	send_dat(p_jd9854, 0x00); //CKH_T18_ODD[11:8],CKH_T17_ODD[11:8]
	send_dat(p_jd9854, 0x00); //CKH_T17_ODD[7:0],
	send_dat(p_jd9854, 0x00); //CKH_T18_ODD[7:0],
	send_dat(p_jd9854, 0x00); //CKH_T20_ODD[11:8],CKH_T19_ODD[11:8]
	send_dat(p_jd9854, 0x00); //CKH_T19_ODD[7:0],
	send_dat(p_jd9854, 0x00); //CKH_T20_ODD[7:0],

	///-----------------------------------------------------------------------------------------
	//---------------- PAGE0 --------------
	send_cmd(p_jd9854, 0xDE);
	send_dat(p_jd9854, 0x00);
	// RAM_CTRL
	send_cmd(p_jd9854, 0xD7);
	send_dat(p_jd9854, 0x20);  //GM=1;RP=0;RM=0;DM=00
	send_dat(p_jd9854, 0x00);
	send_dat(p_jd9854, 0x00);

	//---------------- PAGE1 --------------
	send_cmd(p_jd9854, 0xDE);
	send_dat(p_jd9854, 0x01);

	////MCMD_CTRL
	send_cmd(p_jd9854, 0xCA);
	send_dat(p_jd9854, 0x01);

	//---------------- PAGE2 --------------
	send_cmd(p_jd9854, 0xDE);
	send_dat(p_jd9854, 0x02);

	//OSC DIV
	send_cmd(p_jd9854, 0xC5);
	send_dat(p_jd9854, 0x03); //FPS 60HZ (0x03) to 30HZ (0x0B) ,47HZ(0x0F),42HZ(0x0E)

	//---------------- PAGE0 --------------
	send_cmd(p_jd9854, 0xDE);
	send_dat(p_jd9854, 0x00);

	//Color Pixel Format
	send_cmd(p_jd9854, 0x3A);
	send_dat(p_jd9854, 0x55); //565

	//TE ON
	send_cmd(p_jd9854, 0x35);
	send_dat(p_jd9854, 0x00);

	send_cmd(p_jd9854, 0x20);

	//SLP OUT
	//send_cmd(p_jd9854, 0x11);     // SLPOUT

	send_cmd(p_jd9854, 0x29);     // SLPOUT

//	JD9854_Clear(p_jd9854, 0x00FFFFFF);

//	k_busy_wait(500*1000);

//	JD9854_Clear(p_jd9854, 0x00000000);

//	k_busy_wait(500*1000);

//	JD9854_Clear(p_jd9854, 0x000000FF);

//	k_busy_wait(500*1000);

//	JD9854_Clear(p_jd9854, 0x0000FF00);

//	k_busy_wait(500*1000);

//	JD9854_Clear(p_jd9854, 0x00FF0000);

//	k_busy_wait(500*1000);

//	printk("init finish\n");

}

static int jd9854_register_callback(const struct device *dev,
					  const struct display_callback *callback)
{
	struct jd9854_data *data = (struct jd9854_data *)dev->data;
	int i;

	for (i = 0; i < ARRAY_SIZE(data->callback); i++) {
		if (data->callback[i] == NULL) {
			data->callback[i] = callback;
			return 0;
		}
	}

	return -EBUSY;
}

static int jd9854_unregister_callback(const struct device *dev,
					  const struct display_callback *callback)
{
	struct jd9854_data *data = (struct jd9854_data *)dev->data;
	int i;

	for (i = 0; i < ARRAY_SIZE(data->callback); i++) {
		if (data->callback[i] == callback) {
			data->callback[i] = NULL;
			return 0;
		}
	}

	return -EINVAL;
}

static void jd9854_prepare_transfer(void *arg, const display_rect_t *area)
{
	struct device *dev = arg;
	struct jd9854_data *data = (struct jd9854_data *)dev->data;

	memcpy(&data->refresh_area, area, sizeof(*area));
	jd9854_set_mem_area(data, area->x, area->y, area->w, area->h);

	{
		display_controller_write_config(data->lcdc_dev, jd9854_CMD_RAMWR, 0, 0);
	}
	data->blank_off_flag = 1;
}

static void jd9854_start_transfer(void *arg)
{
	const struct device *dev = arg;
	struct jd9854_data *data = (struct jd9854_data *)dev->data;
	struct display_buffer_descriptor desc = {
		.width = data->refresh_area.w,
		.height = data->refresh_area.h,
		.pitch = data->refresh_area.w,
	};

	sys_trace_u32x4(SYS_TRACE_ID_LCD_POST,
			data->refresh_area.x, data->refresh_area.y,
			data->refresh_area.x + data->refresh_area.w - 1,
			data->refresh_area.y + data->refresh_area.h - 1);

	display_controller_write_pixels(data->lcdc_dev, &desc, NULL);
}

static void jd9854_complete_transfer(void *arg)
{
	const struct device *dev = arg;
	struct jd9854_data *data = (struct jd9854_data *)dev->data;
	int i;

	sys_trace_end_call(SYS_TRACE_ID_LCD_POST);

	for (i = 0; i < ARRAY_SIZE(data->callback); i++) {
		if (data->callback[i] && data->callback[i]->complete) {
			data->callback[i]->complete(data->callback[i]);
		}
	}
}

DEVICE_DECLARE(jd9854);

#ifdef CONFIG_PANEL_TE_GPIO
static void _te_gpio_callback_handler(const struct device *port,
		struct gpio_callback *cb, gpio_port_pins_t pins)
{
	const struct device *dev = DEVICE_GET(jd9854);
	struct jd9854_data *data = (struct jd9854_data *)dev->data;
	uint32_t timestamp = k_cycle_get_32();
	int i;

	sys_trace_void(SYS_TRACE_ID_VSYNC);

	if(data->blank_off_flag)
		jd9854_blanking_off(dev);

	for (i = 0; i < ARRAY_SIZE(data->callback) ; i++) {
		if (data->callback[i] && data->callback[i]->vsync) {
			data->callback[i]->vsync(data->callback[i], timestamp);
		}
	}

	sys_trace_end_call(SYS_TRACE_ID_VSYNC);
}
#endif /* CONFIG_PANEL_TE_GPIO */

static int jd9854_init(const struct device *dev)
{

	struct jd9854_data *data = (struct jd9854_data *)dev->data;
	struct device *de_dev = NULL;

	acts_pinmux_setup_pins(jd9854_pin_config, ARRAY_SIZE(jd9854_pin_config));

#ifdef CONFIG_PANEL_TE_GPIO
	data->te_gpio_dev = device_get_binding(te_gpio_cfg.gpio_dev_name);
	if (gpio_pin_configure(data->te_gpio_dev, te_gpio_cfg.gpion,
				GPIO_INPUT | GPIO_INT_EDGE_TO_ACTIVE | te_gpio_cfg.flag)) {
		LOG_ERR("Couldn't configure te pin");
		data->te_gpio_dev = NULL;
	} else {
		gpio_init_callback(&data->te_gpio_cb, _te_gpio_callback_handler, BIT(te_gpio_cfg.gpion));
	}
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
	data->pwm_dev = device_get_binding("PWM");
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
		gpio_pin_set(data->power_gpio_dev, power_gpio_cfg.gpion, 0);
#endif

	data->lcdc_dev = (struct device *)device_get_binding(CONFIG_LCDC_DEV_NAME);
	if (data->lcdc_dev == NULL) {
		LOG_ERR("Could not get LCD controller device");
		return -EPERM;
	}

	display_controller_enable(data->lcdc_dev, &jd9854_videoport);
	display_controller_control(data->lcdc_dev,
			DISPLAY_CONTROLLER_CTRL_COMPLETE_CB, jd9854_complete_transfer, (void *)dev);
	display_controller_set_mode(data->lcdc_dev, &jd9854_videomode);

	de_dev = (struct device *)device_get_binding(CONFIG_DISPLAY_ENGINE_DEV_NAME);
	if (de_dev == NULL) {
		LOG_WRN("Could not get display engine device");

		display_controller_set_source(data->lcdc_dev,
				DISPLAY_CONTROLLER_SOURCE_MCU, de_dev);
	} else {
		display_controller_set_source(data->lcdc_dev,
				DISPLAY_CONTROLLER_SOURCE_ENGINE, de_dev);

		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_PORT, (void *)&jd9854_videoport, NULL);
		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_MODE, (void *)&jd9854_videomode, NULL);
		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_PREPARE_CB, jd9854_prepare_transfer, (void *)dev);
		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_START_CB, jd9854_start_transfer, (void *)dev);
	}

#ifdef CONFIG_PM_DEVICE
	data->pm_state = PM_DEVICE_ACTION_RESUME;
#endif
	data->blank_state = 0;
	data->blank_off_flag = 0;

	jd9854_reset_display(data);
	jd9854_blanking_on(dev);
	jd9854_lcd_init(data);

#ifdef CONFIG_PANEL_ORIENTATION
	jd9854_set_orientation(dev, CONFIG_PANEL_ORIENTATION / 90);
#else
	jd9854_set_orientation(dev, DISPLAY_ORIENTATION_NORMAL);
#endif

	jd9854_set_te(dev);

	//jd9854_exit_sleep(data);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static void jd9854_enter_sleep(struct jd9854_data *data)
{

	jd9854_transmit(data, jd9854_CMD_SLEEP_IN, NULL, 0);

//	k_sleep(K_MSEC(5));
}

#ifdef CONFIG_PANEL_POWER_GPIO
static void panel_jd9854_res(struct k_work *work)
{
	struct jd9854_data *jd9854 = CONTAINER_OF(work, struct jd9854_data, timer);
	const struct device *lcd_panel_dev = NULL;

	gpio_pin_set(jd9854->power_gpio_dev, power_gpio_cfg.gpion, 0);
	lcd_panel_dev = device_get_binding(CONFIG_LCD_DISPLAY_DEV_NAME);
	jd9854_init(lcd_panel_dev);
}
#endif

static int jd9854_pm_control(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;
	struct jd9854_data *data = (struct jd9854_data *)dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_EARLY_SUSPEND:
#ifdef CONFIG_PANEL_TE_GPIO
		gpio_remove_callback(data->te_gpio_dev, &data->te_gpio_cb);
#endif
		jd9854_blanking_on(dev);
		//data->blank_state = 0;
		break;
	case PM_DEVICE_ACTION_LATE_RESUME:
		//jd9854_blanking_off(dev);
		break;
	case PM_DEVICE_ACTION_RESUME:
		if(CONFIG_PANEL_SLEEP_SW) {
#ifdef CONFIG_PANEL_POWER_GPIO
			k_delayed_work_init(&data->timer, panel_jd9854_res);
			k_delayed_work_submit(&data->timer, K_NO_WAIT);
#endif
		} else {
			jd9854_exit_sleep(data);
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_FORCE_SUSPEND:
	default:
		if(CONFIG_PANEL_SLEEP_SW) {
#ifdef CONFIG_PANEL_POWER_GPIO
			gpio_pin_set(data->power_gpio_dev, power_gpio_cfg.gpion, 1);
#endif
		} else {
			jd9854_enter_sleep(data);
		}
		break;
	}
	
	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static const struct display_driver_api jd9854_api = {
	.blanking_on = jd9854_blanking_on,
	.blanking_off = jd9854_blanking_off,
	.write = jd9854_write,
	.read = jd9854_read,
	.get_framebuffer = jd9854_get_framebuffer,
	.set_brightness = jd9854_set_brightness,
	.set_contrast = jd9854_set_contrast,
	.get_capabilities = jd9854_get_capabilities,
	.set_pixel_format = jd9854_set_pixel_format,
	.set_orientation = jd9854_set_orientation,
	.register_callback = jd9854_register_callback,
	.unregister_callback = jd9854_unregister_callback,
};

static struct jd9854_data jd9854_data;

DEVICE_DEFINE(jd9854, CONFIG_LCD_DISPLAY_DEV_NAME, &jd9854_init,
			jd9854_pm_control, &jd9854_data, NULL, POST_KERNEL,
			60, &jd9854_api);
