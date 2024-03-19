/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_rm69090.h"
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
LOG_MODULE_REGISTER(panel_rm69090, CONFIG_DISPLAY_LOG_LEVEL);

#if defined(CONFIG_PANEL_BACKLIGHT_PWM_CHAN) || defined(CONFIG_PANEL_BACKLIGHT_GPIO)
#  define CONFIG_PANEL_BACKLIGHT_CTRL 1
#endif

struct rm69090_data {
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
static int _te_gpio_int_set_enable(struct rm69090_data *data, bool enabled);
#endif

//#define CONFIG_QSPI_GPIO_SIM

#ifdef CONFIG_QSPI_GPIO_SIM
#define PIN_CSX		(30)
#define PIN_SCL		(34)
#define PIN_SDA0	(14)
#define PIN_SDA1	(15)
#define PIN_SDA2	(16)
#define PIN_SDA3	(17)

static void gpio_out_init(int pin);
static void gpio_out(int pin, int level);

static void spi_write_data(uint8_t data);
static void spi_write_cmd(int dv_cmd, int cmd);
static void spi_write_565img_data(int data);

static void gpio_in_init(int pin);
static int gpio_in(int pin);
static int spi_read_data(void);

static uint32_t rm69090_read_id(void);
static void rm69090_address_set(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
static void rm69090_clear(uint32_t color32);
static void rm69090_write_buf(const struct display_buffer_descriptor *desc, const void *buf);
#endif /* CONFIG_QSPI_GPIO_SIM */

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

static const PANEL_VIDEO_PORT_DEFINE(rm69090_videoport);
static const PANEL_VIDEO_MODE_DEFINE(rm69090_videomode, PIXEL_FORMAT_RGB_565);


static void rm69090_transmit_begin(void)
{
#ifdef CONFIG_QSPI_GPIO_SIM
	if (rm69090_videoport.type == DISPLAY_PORT_SPI_QUAD) {
		gpio_out_init(PIN_CSX);//CS
		gpio_out_init(PIN_SCL);//SCL
		gpio_out_init(PIN_SDA0);//SDA0
		gpio_out_init(PIN_SDA1);//SDA1
		gpio_out_init(PIN_SDA2);//SDA2
		gpio_out_init(PIN_SDA3);//SDA3
	}
#endif
}

static void rm69090_transmit_end(void)
{
#ifdef CONFIG_QSPI_GPIO_SIM
	if (rm69090_videoport.type == DISPLAY_PORT_SPI_QUAD) {
		acts_pinmux_setup_pins(rm69090_pin_config, ARRAY_SIZE(rm69090_pin_config));
	}
#endif
}

static void rm69090_transmit(struct rm69090_data *data, int cmd,
		const uint8_t *tx_data, size_t tx_count)
{
#ifdef CONFIG_QSPI_GPIO_SIM
	if (rm69090_videoport.type == DISPLAY_PORT_SPI_QUAD) {
		gpio_out(PIN_CSX, 0);//CS=0

		if (cmd >= 0) {
			uint8_t cmd_h = 0x02;
			uint8_t cmd_l = cmd & 0xff;
			spi_write_cmd(cmd_h, cmd_l);
		}

		for (; tx_count > 0; tx_count--) {
			spi_write_data(*tx_data++);
		}

		gpio_out(PIN_CSX, 1);//CS=1
	} else
#endif
	{
		display_controller_write_config(data->lcdc_dev, (0x02 << 24) | (cmd << 8), tx_data, tx_count);
	}
}

static void rm69090_apply_brightness(struct rm69090_data *data, uint8_t brightness)
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
#else
	rm69090_transmit_begin();
	rm69090_transmit(data, RM69090_CMD_WRDISBV, &brightness, 1);
	rm69090_transmit_end();

	data->brightness = brightness;
#endif
}

static int rm69090_blanking_on(const struct device *dev)
{
	struct rm69090_data *driver = (struct rm69090_data *)dev->data;

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

	rm69090_apply_brightness(driver, 0);

	rm69090_transmit_begin();
	rm69090_transmit(driver, RM69090_CMD_DISPOFF, NULL, 0);
	rm69090_transmit_end();

	return 0;
}

static int rm69090_blanking_off(const struct device *dev)
{
	struct rm69090_data *driver = (struct rm69090_data *)dev->data;

#ifdef CONFIG_PM_DEVICE
	if (driver->pm_state != PM_DEVICE_STATE_ACTIVE &&
		driver->pm_state != PM_DEVICE_STATE_LOW_POWER)
		return -EPERM;
#endif

	if (driver->blanking_off == 1)
		return 0;

	LOG_INF("display blanking off");
	driver->blanking_off = 1;

	rm69090_transmit_begin();
	rm69090_transmit(driver, RM69090_CMD_DISPON, NULL, 0);
	rm69090_transmit_end();

	k_sleep(K_MSEC(80));

#ifdef CONFIG_PANEL_TE_GPIO
	_te_gpio_int_set_enable(driver, true);
#endif

	return 0;
}

static int rm69090_read(const struct device *dev,
			const uint16_t x,
			const uint16_t y,
			const struct display_buffer_descriptor *desc,
			void *buf)
{
	return -ENOTSUP;
}

static void rm69090_set_mem_area(struct rm69090_data *data, const uint16_t x,
				 const uint16_t y, const uint16_t w, const uint16_t h)
{
	uint16_t cmd_data[2];

	uint16_t ram_x = x + 12;
	uint16_t ram_y = y;

	cmd_data[0] = sys_cpu_to_be16(ram_x);
	cmd_data[1] = sys_cpu_to_be16(ram_x + w - 1);
	rm69090_transmit(data, RM69090_CMD_CASET, (uint8_t *)&cmd_data[0], 4);

	cmd_data[0] = sys_cpu_to_be16(ram_y);
	cmd_data[1] = sys_cpu_to_be16(ram_y + h - 1);
	rm69090_transmit(data, RM69090_CMD_RASET, (uint8_t *)&cmd_data[0], 4);
}

static int rm69090_write(const struct device *dev,
			 const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	struct rm69090_data *data = (struct rm69090_data *)dev->data;

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

	rm69090_transmit_begin();

	rm69090_set_mem_area(data, x, y, desc->width, desc->height);

	if (rm69090_videoport.type != DISPLAY_PORT_SPI_QUAD) {
		rm69090_transmit(data, RM69090_CMD_RAMWR, 0, 0);
	}

	rm69090_transmit_end();

	display_controller_set_source(data->lcdc_dev, DISPLAY_CONTROLLER_SOURCE_DMA, NULL);

	return display_controller_write_pixels(data->lcdc_dev, (0x32 << 24 | RM69090_CMD_RAMWR << 8), desc, buf);
}

static void *rm69090_get_framebuffer(const struct device *dev)
{
	return NULL;
}

static int rm69090_set_brightness(const struct device *dev,
			   const uint8_t brightness)
{
	struct rm69090_data *data = (struct rm69090_data *)dev->data;

	if (data->blanking_off == 0)
		return -EPERM;

	if (brightness == data->pending_brightness)
		return 0;

	LOG_INF("display set_brightness %u", brightness);
	data->pending_brightness = brightness;

	/* delayed set in TE interrupt handler */

	return 0;
}

static int rm69090_set_contrast(const struct device *dev,
			 const uint8_t contrast)
{
	return -ENOTSUP;
}

static void rm69090_get_capabilities(const struct device *dev,
			      struct display_capabilities *capabilities)
{
	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = rm69090_videomode.hactive;
	capabilities->y_resolution = rm69090_videomode.vactive;
	capabilities->vsync_period = 1000000u / rm69090_videomode.refresh_rate;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
	capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;

#ifdef CONFIG_PANEL_TE_GPIO
	capabilities->screen_info = SCREEN_INFO_VSYNC;
#endif
}

static int rm69090_set_pixel_format(const struct device *dev,
			     const enum display_pixel_format pixel_format)
{
	if (pixel_format == PIXEL_FORMAT_RGB_565) {
		return 0;
	}

	LOG_ERR("Pixel format change not implemented");
	return -ENOTSUP;
}

static int rm69090_set_orientation(const struct device *dev,
			    const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}

	LOG_ERR("Changing display orientation not implemented");
	return -ENOTSUP;
}

static void rm69090_reset_display(struct rm69090_data *data)
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
	k_sleep(K_MSEC(120));
#endif
}

static int rm69090_set_te(struct rm69090_data *data)
{
	if (rm69090_videomode.flags & (DISPLAY_FLAGS_TE_HIGH | DISPLAY_FLAGS_TE_LOW)) {
		uint8_t tmp[2];

		sys_put_be16(CONFIG_PANEL_TE_SCANLINE, tmp);
		rm69090_transmit(data, RM69090_CMD_STESL, tmp, 2);

		tmp[0] = 0x02; /* only output vsync */
		rm69090_transmit(data, RM69090_CMD_TEON, tmp, 1);
	} else {
		rm69090_transmit(data, RM69090_CMD_TEOFF, NULL, 0);
	}

	return 0;
}

static void lcd_cmd(struct rm69090_data* p_rm69090, uint8_t reg)
{
#ifdef CONFIG_QSPI_GPIO_SIM
	gpio_out(PIN_CSX, 0); // cs = 0
#endif

	rm69090_transmit(p_rm69090, reg, NULL, 0);

#ifdef CONFIG_QSPI_GPIO_SIM
	gpio_out(PIN_CSX, 1); // cs = 1
#endif
}

static void lcd_cmd_param(struct rm69090_data* p_rm69090, uint8_t reg, uint8_t data)
{
#ifdef CONFIG_QSPI_GPIO_SIM
	gpio_out(PIN_CSX, 0); // cs = 0
#endif

	rm69090_transmit(p_rm69090, reg, &data, 1);

#ifdef CONFIG_QSPI_GPIO_SIM
	gpio_out(PIN_CSX, 1); // cs = 1
#endif
}

static void lcd_cmd_param4_ext(struct rm69090_data* p_rm69090, uint8_t reg, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4)
{
	uint8_t data_array[4] = { data1, data2, data3, data4 };

#ifdef CONFIG_QSPI_GPIO_SIM
	gpio_out(PIN_CSX, 0); // cs = 0
#endif

	rm69090_transmit(p_rm69090, reg, data_array, 4);

#ifdef CONFIG_QSPI_GPIO_SIM
	gpio_out(PIN_CSX, 1); // cs = 1
#endif
}

#define lcd_rm69090_qspi_write_cmd(reg)       \
	lcd_cmd(p_rm69090, reg)
#define lcd_rm69090_qspi_cmd_param(reg, data) \
	lcd_cmd_param(p_rm69090, reg, data)
#define lcd_rm69090_qspi_cmd_param4_ext(reg, data1, data2, data3, data4) \
	lcd_cmd_param4_ext(p_rm69090, reg, data1, data2, data3, data4)

static void rm69090_lcd_init(struct rm69090_data *p_rm69090)
{
	lcd_rm69090_qspi_cmd_param(0xFE,0x01);
	lcd_rm69090_qspi_cmd_param(0x05,0x00);
	lcd_rm69090_qspi_cmd_param(0x06,0x72);
	lcd_rm69090_qspi_cmd_param(0x0D,0x00);
	lcd_rm69090_qspi_cmd_param(0x0E,0x81);//AVDD=6V
	lcd_rm69090_qspi_cmd_param(0x0F,0x81);
	lcd_rm69090_qspi_cmd_param(0x10,0x11);//AVDD=3VCI
	lcd_rm69090_qspi_cmd_param(0x11,0x81);//VCL=-VCI
	lcd_rm69090_qspi_cmd_param(0x12,0x81);
	lcd_rm69090_qspi_cmd_param(0x13,0x80);//VGH=AVDD
	lcd_rm69090_qspi_cmd_param(0x14,0x80);
	lcd_rm69090_qspi_cmd_param(0x15,0x81);//VGL=
	lcd_rm69090_qspi_cmd_param(0x16,0x81);
	lcd_rm69090_qspi_cmd_param(0x18,0x66);//VGHR=6V
	lcd_rm69090_qspi_cmd_param(0x19,0x88);//VGLR=-6V
	lcd_rm69090_qspi_cmd_param(0x5B,0x10);//VREFPN5 Regulator Enable
	lcd_rm69090_qspi_cmd_param(0x5C,0x55);
	lcd_rm69090_qspi_cmd_param(0x62,0x19);//Normal VREFN
	lcd_rm69090_qspi_cmd_param(0x63,0x19);//Idle VREFN
	lcd_rm69090_qspi_cmd_param(0x70,0x54);
	lcd_rm69090_qspi_cmd_param(0x74,0x0C);
	lcd_rm69090_qspi_cmd_param(0xC5,0x10);// NOR=IDLE=GAM1 // HBM=GAM2

	lcd_rm69090_qspi_cmd_param(0xFE,0x01);
	lcd_rm69090_qspi_cmd_param(0x25,0x03);
	lcd_rm69090_qspi_cmd_param(0x26,0x32);
	lcd_rm69090_qspi_cmd_param(0x27,0x0A);
	lcd_rm69090_qspi_cmd_param(0x28,0x08);
	lcd_rm69090_qspi_cmd_param(0x2A,0x03);
	lcd_rm69090_qspi_cmd_param(0x2B,0x32);
	lcd_rm69090_qspi_cmd_param(0x2D,0x0A);
	lcd_rm69090_qspi_cmd_param(0x2F,0x08);
	lcd_rm69090_qspi_cmd_param(0x30,0x43);//43: 15Hz

	lcd_rm69090_qspi_cmd_param(0x66,0x90);
	lcd_rm69090_qspi_cmd_param(0x72,0x1A);//OVDD  4.6V
	lcd_rm69090_qspi_cmd_param(0x73,0x13);//OVSS  -2.2V

	lcd_rm69090_qspi_cmd_param(0xFE,0x01);
	lcd_rm69090_qspi_cmd_param(0x6A,0x17);//RT4723  daz20013  0x17=-2.2

	lcd_rm69090_qspi_cmd_param(0x1B,0x00);
	//VSR power saving
	lcd_rm69090_qspi_cmd_param(0x1D,0x03);
	lcd_rm69090_qspi_cmd_param(0x1E,0x03);
	lcd_rm69090_qspi_cmd_param(0x1F,0x03);
	lcd_rm69090_qspi_cmd_param(0x20,0x03);
	lcd_rm69090_qspi_cmd_param(0xFE,0x01);
	lcd_rm69090_qspi_cmd_param(0x36,0x00);
	lcd_rm69090_qspi_cmd_param(0x6C,0x80);
	lcd_rm69090_qspi_cmd_param(0x6D,0x19);//VGMP VGSP turn off at idle mode

	lcd_rm69090_qspi_cmd_param(0xFE,0x04);
	lcd_rm69090_qspi_cmd_param(0x63,0x00);
	lcd_rm69090_qspi_cmd_param(0x64,0x0E);
	//Gamma1 - AOD/Normal
	lcd_rm69090_qspi_cmd_param(0xFE,0x02);
	lcd_rm69090_qspi_cmd_param(0xA9,0x30);//5.8V VGMPS
	lcd_rm69090_qspi_cmd_param(0xAA,0xB9);//2.5V VGSP
	lcd_rm69090_qspi_cmd_param(0xAB,0x01);
	//Gamma2 - HBM
	lcd_rm69090_qspi_cmd_param(0xFE,0x03);//page2
	lcd_rm69090_qspi_cmd_param(0xA9,0x30);//5.8V VGMP
	lcd_rm69090_qspi_cmd_param(0xAA,0x90);//2V VGSP
	lcd_rm69090_qspi_cmd_param(0xAB,0x01);
	//SW MAPPING
	lcd_rm69090_qspi_cmd_param(0xFE,0x0C);
	lcd_rm69090_qspi_cmd_param(0x07,0x1F);
	lcd_rm69090_qspi_cmd_param(0x08,0x2F);
	lcd_rm69090_qspi_cmd_param(0x09,0x3F);
	lcd_rm69090_qspi_cmd_param(0x0A,0x4F);
	lcd_rm69090_qspi_cmd_param(0x0B,0x5F);
	lcd_rm69090_qspi_cmd_param(0x0C,0x6F);
	lcd_rm69090_qspi_cmd_param(0x0D,0xFF);
	lcd_rm69090_qspi_cmd_param(0x0E,0xFF);
	lcd_rm69090_qspi_cmd_param(0x0F,0xFF);
	lcd_rm69090_qspi_cmd_param(0x10,0xFF);

	lcd_rm69090_qspi_cmd_param(0xFE,0x01);
	lcd_rm69090_qspi_cmd_param(0x42,0x14);
	lcd_rm69090_qspi_cmd_param(0x43,0x41);
	lcd_rm69090_qspi_cmd_param(0x44,0x25);
	lcd_rm69090_qspi_cmd_param(0x45,0x52);
	lcd_rm69090_qspi_cmd_param(0x46,0x36);
	lcd_rm69090_qspi_cmd_param(0x47,0x63);

	lcd_rm69090_qspi_cmd_param(0x48,0x41);
	lcd_rm69090_qspi_cmd_param(0x49,0x14);
	lcd_rm69090_qspi_cmd_param(0x4A,0x52);
	lcd_rm69090_qspi_cmd_param(0x4B,0x25);
	lcd_rm69090_qspi_cmd_param(0x4C,0x63);
	lcd_rm69090_qspi_cmd_param(0x4D,0x36);

	lcd_rm69090_qspi_cmd_param(0x4E,0x16);
	lcd_rm69090_qspi_cmd_param(0x4F,0x61);
	lcd_rm69090_qspi_cmd_param(0x50,0x25);
	lcd_rm69090_qspi_cmd_param(0x51,0x52);
	lcd_rm69090_qspi_cmd_param(0x52,0x34);
	lcd_rm69090_qspi_cmd_param(0x53,0x43);

	lcd_rm69090_qspi_cmd_param(0x54,0x61);
	lcd_rm69090_qspi_cmd_param(0x55,0x16);
	lcd_rm69090_qspi_cmd_param(0x56,0x52);
	lcd_rm69090_qspi_cmd_param(0x57,0x25);
	lcd_rm69090_qspi_cmd_param(0x58,0x43);
	lcd_rm69090_qspi_cmd_param(0x59,0x34);
	//Switch Timing Control
	lcd_rm69090_qspi_cmd_param(0xFE,0x01);
	lcd_rm69090_qspi_cmd_param(0x3A,0x00);
	lcd_rm69090_qspi_cmd_param(0x3B,0x00);
	lcd_rm69090_qspi_cmd_param(0x3D,0x12);
	lcd_rm69090_qspi_cmd_param(0x3F,0x37);
	lcd_rm69090_qspi_cmd_param(0x40,0x12);
	lcd_rm69090_qspi_cmd_param(0x41,0x0F);
	lcd_rm69090_qspi_cmd_param(0x37,0x0C);

	lcd_rm69090_qspi_cmd_param(0xFE,0x04);
	lcd_rm69090_qspi_cmd_param(0x5D,0x01);
	lcd_rm69090_qspi_cmd_param(0x75,0x08);
	//VSR Marping command---L
	lcd_rm69090_qspi_cmd_param(0xFE,0x04);
	lcd_rm69090_qspi_cmd_param(0x5E,0x0F);
	lcd_rm69090_qspi_cmd_param(0x5F,0x12);
	lcd_rm69090_qspi_cmd_param(0x60,0xFF);
	lcd_rm69090_qspi_cmd_param(0x61,0xFF);
	lcd_rm69090_qspi_cmd_param(0x62,0xFF);
	//VSR Marping command---R
	lcd_rm69090_qspi_cmd_param(0xFE,0x04);
	lcd_rm69090_qspi_cmd_param(0x76,0xFF);
	lcd_rm69090_qspi_cmd_param(0x77,0xFF);
	lcd_rm69090_qspi_cmd_param(0x78,0x49);
	lcd_rm69090_qspi_cmd_param(0x79,0xF3);
	lcd_rm69090_qspi_cmd_param(0x7A,0xFF);
	 //VSR1-STV
	lcd_rm69090_qspi_cmd_param(0xFE,0x04);
	lcd_rm69090_qspi_cmd_param(0x00,0x9D);
	lcd_rm69090_qspi_cmd_param(0x01,0x00);
	lcd_rm69090_qspi_cmd_param(0x02,0x00);
	lcd_rm69090_qspi_cmd_param(0x03,0x00);
	lcd_rm69090_qspi_cmd_param(0x04,0x00);
	lcd_rm69090_qspi_cmd_param(0x05,0x01);
	lcd_rm69090_qspi_cmd_param(0x06,0x01);
	lcd_rm69090_qspi_cmd_param(0x07,0x01);
	lcd_rm69090_qspi_cmd_param(0x08,0x00);
	//VSR2-SCK1
	lcd_rm69090_qspi_cmd_param(0xFE,0x04);
	lcd_rm69090_qspi_cmd_param(0x09,0xDC);
	lcd_rm69090_qspi_cmd_param(0x0A,0x00);
	lcd_rm69090_qspi_cmd_param(0x0B,0x02);
	lcd_rm69090_qspi_cmd_param(0x0C,0x00);
	lcd_rm69090_qspi_cmd_param(0x0D,0x08);
	lcd_rm69090_qspi_cmd_param(0x0E,0x01);
	lcd_rm69090_qspi_cmd_param(0x0F,0xCE);
	lcd_rm69090_qspi_cmd_param(0x10,0x16);
	lcd_rm69090_qspi_cmd_param(0x11,0x00);
	//VSR3-SCK2
	lcd_rm69090_qspi_cmd_param(0xFE,0x04);
	lcd_rm69090_qspi_cmd_param(0x12,0xDC);
	lcd_rm69090_qspi_cmd_param(0x13,0x00);
	lcd_rm69090_qspi_cmd_param(0x14,0x02);
	lcd_rm69090_qspi_cmd_param(0x15,0x00);
	lcd_rm69090_qspi_cmd_param(0x16,0x08);
	lcd_rm69090_qspi_cmd_param(0x17,0x02);
	lcd_rm69090_qspi_cmd_param(0x18,0xCE);
	lcd_rm69090_qspi_cmd_param(0x19,0x16);
	lcd_rm69090_qspi_cmd_param(0x1A,0x00);
	//VSR4-ECK1
	lcd_rm69090_qspi_cmd_param(0xFE,0x04);
	lcd_rm69090_qspi_cmd_param(0x1B,0xDC);
	lcd_rm69090_qspi_cmd_param(0x1C,0x00);
	lcd_rm69090_qspi_cmd_param(0x1D,0x02);
	lcd_rm69090_qspi_cmd_param(0x1E,0x00);
	lcd_rm69090_qspi_cmd_param(0x1F,0x08);
	lcd_rm69090_qspi_cmd_param(0x20,0x01);
	lcd_rm69090_qspi_cmd_param(0x21,0xCE);
	lcd_rm69090_qspi_cmd_param(0x22,0x16);
	lcd_rm69090_qspi_cmd_param(0x23,0x00);
	//VSR5-ECK2
	lcd_rm69090_qspi_cmd_param(0xFE,0x04);
	lcd_rm69090_qspi_cmd_param(0x24,0xDC);
	lcd_rm69090_qspi_cmd_param(0x25,0x00);
	lcd_rm69090_qspi_cmd_param(0x26,0x02);
	lcd_rm69090_qspi_cmd_param(0x27,0x00);
	lcd_rm69090_qspi_cmd_param(0x28,0x08);
	lcd_rm69090_qspi_cmd_param(0x29,0x02);
	lcd_rm69090_qspi_cmd_param(0x2A,0xCE);
	lcd_rm69090_qspi_cmd_param(0x2B,0x16);
	lcd_rm69090_qspi_cmd_param(0x2D,0x00);
	//VEN EM-STV
	lcd_rm69090_qspi_cmd_param(0xFE,0x04);
	lcd_rm69090_qspi_cmd_param(0x53,0x8A);
	lcd_rm69090_qspi_cmd_param(0x54,0x00);
	lcd_rm69090_qspi_cmd_param(0x55,0x03);
	lcd_rm69090_qspi_cmd_param(0x56,0x01);
	lcd_rm69090_qspi_cmd_param(0x58,0x01);
	lcd_rm69090_qspi_cmd_param(0x59,0x00);
	lcd_rm69090_qspi_cmd_param(0x65,0x76);
	lcd_rm69090_qspi_cmd_param(0x66,0x19);
	lcd_rm69090_qspi_cmd_param(0x67,0x00);

	lcd_rm69090_qspi_cmd_param(0xFE,0x07);
	lcd_rm69090_qspi_cmd_param(0x15,0x04);

	lcd_rm69090_qspi_cmd_param(0xFE,0x05);
	lcd_rm69090_qspi_cmd_param(0x4C,0x01);
	lcd_rm69090_qspi_cmd_param(0x4D,0x82);
	lcd_rm69090_qspi_cmd_param(0x4E,0x04);
	lcd_rm69090_qspi_cmd_param(0x4F,0x00);
	lcd_rm69090_qspi_cmd_param(0x50,0x20);
	lcd_rm69090_qspi_cmd_param(0x51,0x10);
	lcd_rm69090_qspi_cmd_param(0x52,0x04);
	lcd_rm69090_qspi_cmd_param(0x53,0x41);
	lcd_rm69090_qspi_cmd_param(0x54,0x0A);
	lcd_rm69090_qspi_cmd_param(0x55,0x08);
	lcd_rm69090_qspi_cmd_param(0x56,0x00);
	lcd_rm69090_qspi_cmd_param(0x57,0x28);
	lcd_rm69090_qspi_cmd_param(0x58,0x00);
	lcd_rm69090_qspi_cmd_param(0x59,0x80);
	lcd_rm69090_qspi_cmd_param(0x5A,0x04);
	lcd_rm69090_qspi_cmd_param(0x5B,0x10);
	lcd_rm69090_qspi_cmd_param(0x5C,0x20);
	lcd_rm69090_qspi_cmd_param(0x5D,0x00);
	lcd_rm69090_qspi_cmd_param(0x5E,0x04);
	lcd_rm69090_qspi_cmd_param(0x5F,0x0A);
	lcd_rm69090_qspi_cmd_param(0x60,0x01);
	lcd_rm69090_qspi_cmd_param(0x61,0x08);
	lcd_rm69090_qspi_cmd_param(0x62,0x00);
	lcd_rm69090_qspi_cmd_param(0x63,0x20);
	lcd_rm69090_qspi_cmd_param(0x64,0x40);
	lcd_rm69090_qspi_cmd_param(0x65,0x04);
	lcd_rm69090_qspi_cmd_param(0x66,0x02);
	lcd_rm69090_qspi_cmd_param(0x67,0x48);
	lcd_rm69090_qspi_cmd_param(0x68,0x4C);
	lcd_rm69090_qspi_cmd_param(0x69,0x02);
	lcd_rm69090_qspi_cmd_param(0x6A,0x12);
	lcd_rm69090_qspi_cmd_param(0x6B,0x00);
	lcd_rm69090_qspi_cmd_param(0x6C,0x48);
	lcd_rm69090_qspi_cmd_param(0x6D,0xA0);
	lcd_rm69090_qspi_cmd_param(0x6E,0x08);
	lcd_rm69090_qspi_cmd_param(0x6F,0x04);
	lcd_rm69090_qspi_cmd_param(0x70,0x05);
	lcd_rm69090_qspi_cmd_param(0x71,0x92);
	lcd_rm69090_qspi_cmd_param(0x72,0x00);
	lcd_rm69090_qspi_cmd_param(0x73,0x18);
	lcd_rm69090_qspi_cmd_param(0x74,0xA0);
	lcd_rm69090_qspi_cmd_param(0x75,0x00);
	lcd_rm69090_qspi_cmd_param(0x76,0x00);
	lcd_rm69090_qspi_cmd_param(0x77,0xE4);
	lcd_rm69090_qspi_cmd_param(0x78,0x00);
	lcd_rm69090_qspi_cmd_param(0x79,0x04);
	lcd_rm69090_qspi_cmd_param(0x7A,0x02);
	lcd_rm69090_qspi_cmd_param(0x7B,0x01);
	lcd_rm69090_qspi_cmd_param(0x7C,0x00);
	lcd_rm69090_qspi_cmd_param(0x7D,0x00);
	lcd_rm69090_qspi_cmd_param(0x7E,0x24);
	lcd_rm69090_qspi_cmd_param(0x7F,0x4C);
	lcd_rm69090_qspi_cmd_param(0x80,0x04);
	lcd_rm69090_qspi_cmd_param(0x81,0x0A);
	lcd_rm69090_qspi_cmd_param(0x82,0x02);
	lcd_rm69090_qspi_cmd_param(0x83,0xC1);
	lcd_rm69090_qspi_cmd_param(0x84,0x02);
	lcd_rm69090_qspi_cmd_param(0x85,0x18);
	lcd_rm69090_qspi_cmd_param(0x86,0x90);
	lcd_rm69090_qspi_cmd_param(0x87,0x60);
	lcd_rm69090_qspi_cmd_param(0x88,0x88);
	lcd_rm69090_qspi_cmd_param(0x89,0x02);
	lcd_rm69090_qspi_cmd_param(0x8A,0x09);
	lcd_rm69090_qspi_cmd_param(0x8B,0x0C);
	lcd_rm69090_qspi_cmd_param(0x8C,0x18);
	lcd_rm69090_qspi_cmd_param(0x8D,0x90);
	lcd_rm69090_qspi_cmd_param(0x8E,0x10);
	lcd_rm69090_qspi_cmd_param(0x8F,0x08);
	lcd_rm69090_qspi_cmd_param(0x90,0x00);
	lcd_rm69090_qspi_cmd_param(0x91,0x10);
	lcd_rm69090_qspi_cmd_param(0x92,0xA8);
	lcd_rm69090_qspi_cmd_param(0x93,0x00);
	lcd_rm69090_qspi_cmd_param(0x94,0x04);
	lcd_rm69090_qspi_cmd_param(0x95,0x0A);
	lcd_rm69090_qspi_cmd_param(0x96,0x00);
	lcd_rm69090_qspi_cmd_param(0x97,0x08);
	lcd_rm69090_qspi_cmd_param(0x98,0x10);
	lcd_rm69090_qspi_cmd_param(0x99,0x28);
	lcd_rm69090_qspi_cmd_param(0x9A,0x08);
	lcd_rm69090_qspi_cmd_param(0x9B,0x04);
	lcd_rm69090_qspi_cmd_param(0x9C,0x02);
	lcd_rm69090_qspi_cmd_param(0x9D,0x03);

	lcd_rm69090_qspi_cmd_param(0xFE,0x0C);
	lcd_rm69090_qspi_cmd_param(0x25,0x00);
	lcd_rm69090_qspi_cmd_param(0x31,0xEF);
	lcd_rm69090_qspi_cmd_param(0x32,0xE3);
	lcd_rm69090_qspi_cmd_param(0x33,0x00);
	lcd_rm69090_qspi_cmd_param(0x34,0xE3);
	lcd_rm69090_qspi_cmd_param(0x35,0xE3);
	lcd_rm69090_qspi_cmd_param(0x36,0x80);
	lcd_rm69090_qspi_cmd_param(0x37,0x00);
	lcd_rm69090_qspi_cmd_param(0x38,0x79);
	lcd_rm69090_qspi_cmd_param(0x39,0x00);
	lcd_rm69090_qspi_cmd_param(0x3A,0x00);
	lcd_rm69090_qspi_cmd_param(0x3B,0x00);
	lcd_rm69090_qspi_cmd_param(0x3D,0x00);
	lcd_rm69090_qspi_cmd_param(0x3F,0x00);
	lcd_rm69090_qspi_cmd_param(0x40,0x00);
	lcd_rm69090_qspi_cmd_param(0x41,0x00);
	lcd_rm69090_qspi_cmd_param(0x42,0x00);
	lcd_rm69090_qspi_cmd_param(0x43,0x01);

	if (rm69090_videomode.refresh_rate == 50) {
		lcd_rm69090_qspi_cmd_param(0xFE,0x01);
		lcd_rm69090_qspi_cmd_param(0x28,0x64);
	} else if (rm69090_videomode.refresh_rate == 30) {
		lcd_rm69090_qspi_cmd_param(0xFE,0x01);
		lcd_rm69090_qspi_cmd_param(0x25,0x07);
	}

	//lcd_rm69090_qspi_cmd_param(0xFE,0x04);
	//lcd_rm69090_qspi_cmd_param(0x05,0x08);

	lcd_rm69090_qspi_cmd_param(0xFE,0x01);
	lcd_rm69090_qspi_cmd_param(0x6e,0x0a); // 这两句把灭屏功耗降低一倍 (灭屏电流，LCD部分为110uA左右)

	lcd_rm69090_qspi_cmd_param(0xFE,0x00);
	lcd_rm69090_qspi_cmd_param(0x36,0x00); // 切记 RM69090 不能设置翻转 否则 撕裂

	lcd_rm69090_qspi_cmd_param(0x35,0x00);
	lcd_rm69090_qspi_cmd_param(0xC4,0x80);
	lcd_rm69090_qspi_cmd_param(0x51, p_rm69090->brightness); //lcd_rm69090_qspi_cmd_param(0x51,0xFF);
	lcd_rm69090_qspi_cmd_param4_ext(0x2A,0x00,0x0C,0x01,0xD1);
	lcd_rm69090_qspi_cmd_param4_ext(0x2B,0x00,0x00,0x01,0xC5);

	if (rm69090_videomode.pixel_format == PIXEL_FORMAT_RGB_565) {
		lcd_rm69090_qspi_cmd_param(0x3A,0x55);//0x55:RGB565   0x77:RGB888
	} else {
		lcd_rm69090_qspi_cmd_param(0x3A,0x77);//0x55:RGB565   0x77:RGB888
	}

	/* Sleep Out */
	lcd_rm69090_qspi_write_cmd(RM69090_CMD_SLPOUT);
	k_sleep(K_MSEC(120));
}

static int rm69090_register_callback(const struct device *dev,
					  const struct display_callback *callback)
{
	struct rm69090_data *data = (struct rm69090_data *)dev->data;

	if (data->callback == NULL) {
		data->callback = callback;
		return 0;
	}

	return -EBUSY;
}

static int rm69090_unregister_callback(const struct device *dev,
					  const struct display_callback *callback)
{
	struct rm69090_data *data = (struct rm69090_data *)dev->data;

	if (data->callback == callback) {
		data->callback = NULL;
		return 0;
	}

	return -EINVAL;
}

static int rm69090_flush(const struct device *dev, int timeout)
{
	struct rm69090_data *data = (struct rm69090_data *)dev->data;

	do {
		if (data->callback && data->callback->vsync) {
			data->callback->vsync(data->callback, k_cycle_get_32());
		}

		if (!data->transfering || timeout <= 0) {
			break;
		}

		do {
			k_sleep(K_MSEC(2));
			timeout -= 2;
		} while (data->transfering && timeout > 0);
	} while (1);

	return data->transfering ? -ETIMEDOUT : 0;
}

static void rm69090_prepare_de_transfer(void *arg, const display_rect_t *area)
{
	struct device *dev = arg;
	struct rm69090_data *data = (struct rm69090_data *)dev->data;

	data->refr_desc.pixel_format = PIXEL_FORMAT_RGB_565;
	data->refr_desc.width = area->w;
	data->refr_desc.height = area->h;
	data->refr_desc.pitch = area->w;

	sys_trace_u32x4(SYS_TRACE_ID_LCD_POST,
			area->x, area->y, area->x + area->w - 1, area->y + area->h - 1);

	data->transfering = 1;

	rm69090_transmit_begin();

	rm69090_set_mem_area(data, area->x, area->y, area->w, area->h);

	if (rm69090_videoport.type != DISPLAY_PORT_SPI_QUAD) {
		rm69090_transmit(data, RM69090_CMD_RAMWR, 0, 0);
	}

	rm69090_transmit_end();
}

static void rm69090_start_de_transfer(void *arg)
{
	const struct device *dev = arg;
	struct rm69090_data *data = (struct rm69090_data *)dev->data;

	/* source from DE */
	display_controller_set_source(data->lcdc_dev, DISPLAY_CONTROLLER_SOURCE_ENGINE, NULL);
	display_controller_write_pixels(data->lcdc_dev, (0x32 << 24 | RM69090_CMD_RAMWR << 8),
			&data->refr_desc, NULL);
}

static void rm69090_complete_transfer(void *arg)
{
	const struct device *dev = arg;
	struct rm69090_data *data = (struct rm69090_data *)dev->data;

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

DEVICE_DECLARE(rm69090);

#ifdef CONFIG_PM_DEVICE
static void rm69090_apply_pm_post_change(struct rm69090_data *data)
{
	if (data->pm_post_changed == 0) {
		return;
	}

	lcd_cmd_param(data, 0xFE, 0x01);
	/*
	* 0x01 60Hz
	* 0x41 30Hz
	* 0x43 15Hz
	* 0x4B 5Hz
	* 0x7B 1Hz
	*/
	if (data->pm_state == PM_DEVICE_STATE_LOW_POWER) {
		lcd_cmd_param(data, 0x29, 0x4B);
	} else {
		lcd_cmd_param(data, 0x29, 0x01);
	}

	lcd_cmd_param(data, 0xFE, 0x00);
	data->pm_post_changed = 0;

	if (data->pm_state == PM_DEVICE_STATE_LOW_POWER) {
		rm69090_apply_brightness(data, CONFIG_PANEL_AOD_BRIGHTNESS);

#ifdef CONFIG_PANEL_TE_GPIO
		_te_gpio_int_set_enable(data, false);
#endif
	} else {
#ifdef CONFIG_PANEL_TE_GPIO
		_te_gpio_int_set_enable(data, true);
#endif
	}
}
#endif /* CONFIG_PM_DEVICE */

#ifdef CONFIG_PANEL_TE_GPIO
static int _te_gpio_int_set_enable(struct rm69090_data *data, bool enabled)
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
	const struct device *dev = DEVICE_GET(rm69090);
	struct rm69090_data *data = (struct rm69090_data *)dev->data;
	uint32_t timestamp = k_cycle_get_32();

	sys_trace_void(SYS_TRACE_ID_VSYNC);

	rm69090_apply_brightness(data, data->pending_brightness);
#ifdef CONFIG_PM_DEVICE
	rm69090_apply_pm_post_change(data);
#endif

	if (data->callback && data->callback->vsync) {
		data->callback->vsync(data->callback, timestamp);
	}

	sys_trace_end_call(SYS_TRACE_ID_VSYNC);
}
#endif /* CONFIG_PANEL_TE_GPIO */

#ifdef CONFIG_QSPI_GPIO_SIM

static void gpio_out_init(int pin)
{
	sys_write32(0x5060, GPIO_REG_CTL(GPIO_REG_BASE, pin));
}

static void gpio_out(int pin, int level)
{
	if (level) {
		sys_write32(1 << (pin & 31), GPIO_REG_BSR(GPIO_REG_BASE, pin));
	} else {
		sys_write32(1 << (pin & 31), GPIO_REG_BRR(GPIO_REG_BASE, pin));
	}
}

static void spi_write_data(uint8_t data)
{
	int i;

	for (i = 0; i < 8; i++) {
		gpio_out(PIN_SCL, 0);//SCL=0

		if (data & 0x80) gpio_out(PIN_SDA0, 1);
		else gpio_out(PIN_SDA0, 0);

		gpio_out(PIN_SCL, 1);//SCL=1
		data <<= 1;
	}
}

static void spi_write_cmd(int dv_cmd, int cmd)
{
	int i;

	cmd = cmd & 0xff;
	cmd = (cmd << 8) | (dv_cmd << 24);

	for (i = 0; i < 32; i++) {
		gpio_out(PIN_SCL, 0);//SCL=0

		if (cmd & 0x80000000) gpio_out(PIN_SDA0, 1);
		else gpio_out(PIN_SDA0, 0);

		gpio_out(PIN_SCL,1);//SCL=1
		cmd <<= 1;
	}
}

static void gpio_in_init(int pin)
{
	sys_write32(0x00a0, GPIO_REG_CTL(GPIO_REG_BASE, pin));
}

static int gpio_in(int pin)
{
	const struct device *gpio_dev = device_get_binding(CONFIG_GPIO_A_NAME);

	gpio_pin_configure(gpio_dev, pin, GPIO_INPUT);

	return gpio_pin_get(gpio_dev, pin);
}

static int spi_read_data(void)
{
	int i, data = 0;

	gpio_in_init(PIN_SDA0);

	for (i = 0; i < 8; i++){
		data <<= 1;
		gpio_out(PIN_SCL, 0);//SCL=0
		data |= gpio_in(PIN_SDA0);
		gpio_out(PIN_SCL, 1);//SCL=1
	}

	gpio_out_init(PIN_SDA0);

	return 0;
}

static uint32_t rm69090_read_id(void)
{
	uint32_t rt_data = 0;

	gpio_out(PIN_CSX, 0);
	spi_write_cmd(0x03, 0x04);
	rt_data = spi_read_data();
	rt_data <<= 8;
	rt_data |= spi_read_data();
	rt_data <<= 8;
	rt_data |= spi_read_data();
	rt_data <<= 8;
	rt_data |= spi_read_data();
	gpio_out(PIN_CSX, 1);

	return rt_data;
}

static void rm69090_address_set(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
	gpio_out(PIN_CSX, 0);
	x1 += 12;
	x2 += 12;
	spi_write_cmd(0x02, 0x2a);
	spi_write_data(x1 >> 8);
	spi_write_data(x1);
	spi_write_data(x2 >> 8);
	spi_write_data(x2);
	gpio_out(PIN_CSX, 1);

	gpio_out(PIN_CSX, 0);
	spi_write_cmd(0x02, 0x2b);
	spi_write_data(y1 >> 8);
	spi_write_data(y1);
	spi_write_data(y2 >> 8);
	spi_write_data(y2);
	gpio_out(PIN_CSX, 1);
}

static void spi_write_565img_data(int data)
{
	int i;

	for (i = 0; i < 4; i++){
		gpio_out(PIN_SCL, 0);//SCL=0

		if (data & 0x8000) gpio_out(PIN_SDA3, 1);
		else gpio_out(PIN_SDA3, 0);

		if (data & 0x4000) gpio_out(PIN_SDA2, 1);
		else gpio_out(PIN_SDA2, 0);

		if (data & 0x2000) gpio_out(PIN_SDA1, 1);
		else gpio_out(PIN_SDA1, 0);

		if (data & 0x1000) gpio_out(PIN_SDA0, 1);
		else gpio_out(PIN_SDA0, 0);

		gpio_out(PIN_SCL, 1);//SCL=1
		data <<= 4;
	}
}

static void rm69090_clear(uint32_t color32)
{
	int i, color_r, color_g, color_b, color_s = 0;

	rm69090_address_set(0, 0, 453, 453);
	//------------------------------------------------------
	color_r = (color32>>16) & 0xff;
	color_g = (color32>>8) & 0xff;
	color_b = color32 & 0xff;
	//------------------------------------------------------
	color_r = color_r >> 3;
	color_g = color_g >> 2;
	color_b = color_b >> 3;
	//------------------------------------------------------
	color_s = (color_r << 11) | (color_g << 5) | color_b;
	//------------------------------------------------------
	gpio_out(PIN_CSX, 0);

	spi_write_cmd(0x32, 0x2c);

	for (i = 0; i < 454*454; i++){
		spi_write_565img_data(color_s);
	}

	gpio_out(PIN_CSX, 1);
}

static void rm69090_write_buf(const struct display_buffer_descriptor *desc, const void *buf)
{
	int i, j;

	gpio_out(PIN_CSX, 0);

	spi_write_cmd(0x32, 0x2c);

	for (j = desc->height; j > 0; j--) {
		const uint16_t *buf16 = buf;
		for (i = desc->width; i > 0; i--) {
			spi_write_565img_data(*buf16++);
		}

		buf = (uint16_t *)buf + desc->pitch;
	}

	gpio_out(PIN_CSX, 1);
}

#endif /* CONFIG_QSPI_GPIO_SIM */

static int rm69090_panel_init(const struct device *dev)
{
	struct rm69090_data *data = (struct rm69090_data *)dev->data;

	rm69090_transmit_begin();

	rm69090_reset_display(data);
	rm69090_lcd_init(data);
	rm69090_set_te(data);

	rm69090_transmit_end();

	/* Display On */
	rm69090_blanking_off(dev);

	return 0;
}

static void rm69090_resume(struct k_work *work);

static int rm69090_init(const struct device *dev)
{
	struct rm69090_data *data = (struct rm69090_data *)dev->data;
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

	display_controller_enable(data->lcdc_dev, &rm69090_videoport);
	display_controller_control(data->lcdc_dev,
			DISPLAY_CONTROLLER_CTRL_COMPLETE_CB, rm69090_complete_transfer, (void *)dev);
	display_controller_set_mode(data->lcdc_dev, &rm69090_videomode);

	de_dev = (struct device *)device_get_binding(CONFIG_DISPLAY_ENGINE_DEV_NAME);
	if (de_dev == NULL) {
		LOG_WRN("Could not get display engine device");
	} else {
		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_PORT, (void *)&rm69090_videoport, NULL);
		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_MODE, (void *)&rm69090_videomode, NULL);
		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_PREPARE_CB, rm69090_prepare_de_transfer, (void *)dev);
		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_START_CB, rm69090_start_de_transfer, (void *)dev);
	}

	k_sem_init(&data->write_sem, 1, 1);
	data->blanking_off = 0;
	data->pending_brightness = CONFIG_PANEL_BRIGHTNESS;
#ifdef CONFIG_PM_DEVICE
	data->pm_state = PM_DEVICE_STATE_ACTIVE;
	k_work_init(&data->resume_work, rm69090_resume);
#endif

	rm69090_panel_init(dev);
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static void rm69090_enter_sleep(struct rm69090_data *data)
{
	rm69090_transmit_begin();
	rm69090_transmit(data, RM69090_CMD_SLPIN, NULL, 0);
	rm69090_transmit_end();
//	k_sleep(K_MSEC(120));
}

#ifndef CONFIG_PANEL_POWER_GPIO
static void rm69090_exit_sleep(struct rm69090_data *data)
{
	rm69090_transmit_begin();
	rm69090_transmit(data, RM69090_CMD_SLPOUT, NULL, 0);
	rm69090_transmit_end();
	k_sleep(K_MSEC(120));
}
#endif

static void rm69090_pm_notify(struct rm69090_data *data, uint32_t pm_action)
{
	if (data->callback && data->callback->pm_notify) {
		data->callback->pm_notify(data->callback, pm_action);
	}
}

static int rm69090_pm_eary_suspend(const struct device *dev)
{
	struct rm69090_data *data = (struct rm69090_data *)dev->data;

	if (data->pm_state != PM_DEVICE_STATE_ACTIVE &&
		data->pm_state != PM_DEVICE_STATE_LOW_POWER) {
		return -EPERM;
	}

	LOG_INF("panel early-suspend");

	rm69090_pm_notify(data, PM_DEVICE_ACTION_EARLY_SUSPEND);
	rm69090_blanking_on(dev);
	rm69090_enter_sleep(data);

#ifdef CONFIG_PANEL_POWER_GPIO
#ifdef CONFIG_PANEL_RESET_GPIO
	gpio_pin_set(data->reset_gpio_dev, reset_gpio_cfg.gpion, 1);
#endif
	gpio_pin_set(data->power_gpio_dev, power_gpio_cfg.gpion, 0);
#endif /* CONFIG_PANEL_POWER_GPIO */

	data->pm_state = PM_DEVICE_STATE_SUSPENDED;
	return 0;
}

static void rm69090_resume(struct k_work *work)
{
	const struct device *dev = DEVICE_GET(rm69090);
	struct rm69090_data *data = (struct rm69090_data *)dev->data;

	LOG_INF("panel resuming");

	data->pm_state = PM_DEVICE_STATE_ACTIVE;

#ifdef CONFIG_PANEL_POWER_GPIO
	rm69090_panel_init(dev);
#else
	rm69090_exit_sleep(dev->data);
	rm69090_blanking_off(dev);
#endif /* CONFIG_PANEL_POWER_GPIO */

	rm69090_pm_notify(data, PM_DEVICE_ACTION_LATE_RESUME);
	LOG_INF("panel active");
}

static int rm69090_pm_late_resume(const struct device *dev)
{
	struct rm69090_data *data = (struct rm69090_data *)dev->data;

	if (data->pm_state != PM_DEVICE_STATE_SUSPENDED &&
		data->pm_state != PM_DEVICE_STATE_OFF) {
		return -EPERM;
	}

	LOG_INF("panel late-resume");
	k_work_submit(&data->resume_work);
	return 0;
}

static int rm69090_pm_enter_low_power(const struct device *dev)
{
	struct rm69090_data *data = (struct rm69090_data *)dev->data;

	if (data->pm_state != PM_DEVICE_STATE_ACTIVE) {
		return -EPERM;
	}

	data->pm_state = PM_DEVICE_STATE_LOW_POWER;
	data->pm_post_changed = 1;
	rm69090_pm_notify(data, PM_DEVICE_ACTION_LOW_POWER);

	LOG_INF("panel enter low-power");
	return 0;
}

static int rm69090_pm_exit_low_power(const struct device *dev)
{
	struct rm69090_data *data = (struct rm69090_data *)dev->data;

	if (data->pm_state != PM_DEVICE_STATE_LOW_POWER) {
		return -EPERM;
	}

	data->pm_state = PM_DEVICE_STATE_ACTIVE;
	data->pm_post_changed = 1;
	rm69090_apply_pm_post_change(data);

	LOG_INF("panel exit low-power");
	return 0;
}

static int rm69090_pm_control(const struct device *dev, enum pm_device_action action)
{
	struct rm69090_data *data = (struct rm69090_data *)dev->data;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_EARLY_SUSPEND:
		if (soc_get_aod_mode()) {
			ret = rm69090_pm_enter_low_power(dev);
		} else {
			ret = rm69090_pm_eary_suspend(dev);
		}
		break;

	case PM_DEVICE_ACTION_LATE_RESUME:
		if (data->pm_state == PM_DEVICE_STATE_LOW_POWER) {
			ret = rm69090_pm_exit_low_power(dev);
		} else {
			ret = rm69090_pm_late_resume(dev);
		}
		break;

	case PM_DEVICE_ACTION_TURN_OFF:
		rm69090_pm_eary_suspend(dev);
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

static const struct display_driver_api rm69090_api = {
	.blanking_on = rm69090_blanking_on,
	.blanking_off = rm69090_blanking_off,
	.write = rm69090_write,
	.read = rm69090_read,
	.get_framebuffer = rm69090_get_framebuffer,
	.set_brightness = rm69090_set_brightness,
	.set_contrast = rm69090_set_contrast,
	.get_capabilities = rm69090_get_capabilities,
	.set_pixel_format = rm69090_set_pixel_format,
	.set_orientation = rm69090_set_orientation,
	.register_callback = rm69090_register_callback,
	.unregister_callback = rm69090_unregister_callback,
	.flush = rm69090_flush,
};

static struct rm69090_data rm69090_data;

#if IS_ENABLED(CONFIG_PANEL)
DEVICE_DEFINE(rm69090, CONFIG_LCD_DISPLAY_DEV_NAME, &rm69090_init,
			rm69090_pm_control, &rm69090_data, NULL, POST_KERNEL,
			60, &rm69090_api);
#endif
