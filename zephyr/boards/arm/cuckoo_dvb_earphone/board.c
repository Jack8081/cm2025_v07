/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief board init functions
 */

#include <kernel.h>
#include <init.h>
#include <device.h>
#include <soc.h>
#include "board.h"
#include <drivers/gpio.h>
#include <board_cfg.h>

#if 1
static const struct acts_pin_config board_pin_config[] = {
    /*UART0 */   
#if 0 //IS_ENABLED(CONFIG_UART_0)    
    /* uart0 tx */   
    PIN_MFP_SET(GPIO_10,  UART0_MFP_CFG),   
    /* uart0 rx */   
    PIN_MFP_SET(GPIO_11,  UART0_MFP_CFG),   
#endif   

#if 0 //IS_ENABLED(CONFIG_SPI_FLASH_0)
	/* SPI0 CS */
	PIN_MFP_SET(GPIO_0,   SPINOR_MFP_CFG),
	/* SPI0 MISO */
	PIN_MFP_SET(GPIO_1,   SPINOR_MFP_CFG),
	/* SPI0 CLK */
	PIN_MFP_SET(GPIO_2,   SPINOR_MFP_CFG),
	/* SPI0 MOSI */
	PIN_MFP_SET(GPIO_3,   SPINOR_MFP_CFG),
	/* SPI0 IO2 */
	PIN_MFP_SET(GPIO_6,   SPINOR_MFP_PU_CFG),
	/* SPI0 IO3 */
	PIN_MFP_SET(GPIO_7,   SPINOR_MFP_PU_CFG),
#endif

#if 0 //IS_ENABLED(CONFIG_ACTS_BATTERY_NTC)
	PIN_MFP_SET(GPIO_20,  BATNTC_MFP_CFG),
#endif


#if IS_ENABLED(CONFIG_I2C_0)
	/* I2C CLK*/
	PIN_MFP_SET(GPIO_21, I2C_MFP_CFG(MFP0_I2C)),
	/* I2C DATA*/
	PIN_MFP_SET(GPIO_22, I2C_MFP_CFG(MFP0_I2C)),
#endif

#if 0 //IS_ENABLED(CONFIG_CEC)
	PIN_MFP_SET(GPIO_12,  CEC_MFP_CFG),
#endif

#if IS_ENABLED(CONFIG_AUDIO_I2SRX_0)
	/*I2SRX0 mclk*/
	PIN_MFP_SET(GPIO_7,   I2SRX_MFP_CFG),
	/*I2SRX0 bclk*/
	PIN_MFP_SET(GPIO_5,   I2SRX_MFP_CFG),
	/*I2SRX0 lrclk*/
	PIN_MFP_SET(GPIO_8,   I2SRX_MFP_CFG),
	/*I2SRX0 din*/
	PIN_MFP_SET(GPIO_6,   I2SRX_MFP_CFG),
#endif

#if IS_ENABLED(CONFIG_AUDIO_I2STX_0)

    /*I2STX0 mclk*/
    PIN_MFP_SET(GPIO_20, I2STX_MFP_CFG),
    /*I2STX0 bclk*/
    PIN_MFP_SET(GPIO_18, I2STX_MFP_CFG),
    /*I2STX0 lrclk*/
    PIN_MFP_SET(GPIO_17, I2STX_MFP_CFG),
    /*I2STX0 dout*/
    PIN_MFP_SET(GPIO_19, I2STX_MFP_CFG),
#endif

};
#endif


#if IS_ENABLED(CONFIG_PWM)
/* Look at CONFIG_PWM_PIN_CHAN_MAP select the available pwm gpio */
static const struct pwm_acts_pin_config board_pwm_config[] = {
	PWM_PIN_MFP_SET(GPIO_7, 1, PWM_MFP_CFG),
	PWM_PIN_MFP_SET(GPIO_8, 2, PWM_MFP_CFG),
	PWM_PIN_MFP_SET(GPIO_18, 0, PWM_MFP_CFG),
};
#endif

#if 0 //IS_ENABLED(CONFIG_AUDIO_I2STX_0)
static const struct acts_pin_config board_i2stx0_config[] = {
	/*I2STX0 mclk*/
	PIN_MFP_SET(GPIO_39, I2STX_MFP_CFG),
	/*I2STX0 bclk*/
	PIN_MFP_SET(GPIO_40, I2STX_MFP_CFG),
	/*I2STX0 lrclk*/
	PIN_MFP_SET(GPIO_41, I2STX_MFP_CFG),
	/*I2STX0 d0*/
	PIN_MFP_SET(GPIO_42, I2STX_MFP_CFG),
};
#endif

#if IS_ENABLED(CONFIG_ADCKEY)

#define CONFIG_ADCKEY_GPIO

#ifdef CONFIG_ADCKEY_GPIO
#define CONFIG_ADCKEY_GPIO_NUM (GPIO_32)
#else
#define CONFIG_ADCKEY_WIO_NUM  (WIO_1)
#define CONFIG_ADCKEY_WIO_MFP  (1)
#endif

#if 0
static void board_adckey_pinmux_init(void)
{
#ifdef CONFIG_ADCKEY_GPIO
	//acts_pinmux_set(CONFIG_ADCKEY_GPIO_NUM, ADCKEY_MFP_CFG);
#else
	//sys_write32(CONFIG_ADCKEY_WIO_MFP, WIO0_CTL + (CONFIG_ADCKEY_WIO_NUM * 4));
#endif
}
#endif

#endif

static int board_early_init(const struct device *arg)
{
#if 1
	//ARG_UNUSED(arg);

	printk("board early init start!\n");
	acts_pinmux_setup_pins(board_pin_config, ARRAY_SIZE(board_pin_config));

#if IS_ENABLED(CONFIG_ADCKEY)
	//board_adckey_pinmux_init();
#endif

#ifdef CONFIG_RTT_CONSOLE
	//jtag_set();
#endif

    sys_write32(0x00, JTAG_CTL);
#endif

    printk("board early init end!\n");

	return 0;
}

#if (CONFIG_ADCKEY)
#include <drivers/input/input_dev.h>

static int key_press_cnt;
static int key_invalid;
static struct input_value key_last_val;

static void board_key_input_notify(struct input_value *val)
{
	/* any adc key is pressed? */
	if (val->keypad.type != EV_KEY) {
		key_invalid = 1;
		return;
	}

	key_press_cnt++;

	/* save last adfu key value */
	if (key_press_cnt == 1) {
		key_last_val = *val;
		return;
	}

	/* key code must be same with last code and status is pressed */
	if (key_last_val.keypad.type != val->keypad.type ||
		key_last_val.keypad.code != val->keypad.code ||
		val->keypad.code == KEY_RESERVED) {
		key_invalid = 1;
	}
}

static void check_adfu_key(void)
{
	const struct device *adckey_dev;
	struct input_value val;
	int i;

	/* check adfu */
	adckey_dev = device_get_binding(CONFIG_INPUT_DEV_ACTS_ADCKEY_NAME);
	if (!adckey_dev) {
		return;
	}

	input_dev_enable(adckey_dev);

	for(i = 0; i < 8; i++) {
		k_msleep(10);
		input_dev_inquiry(adckey_dev, &val);
		board_key_input_notify(&val);
	}

	if (key_press_cnt > 0 && !key_invalid) {
		printk("key %d detected!\n", key_last_val.keypad.code);

        if(key_last_val.keypad.code == KEY_TBD) {
   
			/* adfu key is pressed, goto adfu! */
			printk("enter adfu!\n");
			sys_pm_reboot(REBOOT_TYPE_GOTO_ADFU);
        }

	}

	input_dev_disable(adckey_dev);
}
#endif

static int board_later_init(const struct device *arg)
{
	ARG_UNUSED(arg);
	
#if (CONFIG_ADCKEY)
	check_adfu_key();
#endif

	printk("%s %d: \n", __func__, __LINE__);
	return 0;
}

/* UART registers struct */
struct acts_uart_reg {
    volatile uint32_t ctrl;

    volatile uint32_t rxdat;

    volatile uint32_t txdat;

    volatile uint32_t stat;

    volatile uint32_t br;
} ;

void uart_poll_out_ch(int c)
{
	struct acts_uart_reg *uart = (struct acts_uart_reg*)UART0_REG_BASE;
	/* Wait for transmitter to be ready */
	while (uart->stat &  BIT(6));
	/* send a character */
	uart->txdat = (uint32_t)c;

}
/*for early printk*/
int arch_printk_char_out(int c)
{
	if ('\n' == c)
		uart_poll_out_ch('\r');
	uart_poll_out_ch(c);
	return 0;
}

static const audio_input_map_t board_audio_input_map[] =  {
	{AUDIO_LINE_IN0, ADC_CH_INPUT0P, ADC_CH_DISABLE, ADC_CH_INPUT0N, ADC_CH_DISABLE},
	{AUDIO_LINE_IN1, ADC_CH_INPUT0NP_DIFF, ADC_CH_INPUT1NP_DIFF, ADC_CH_DISABLE, ADC_CH_DISABLE},
	{AUDIO_LINE_IN2, ADC_CH_DISABLE, ADC_CH_INPUT1P, ADC_CH_DISABLE, ADC_CH_INPUT1N},
	{AUDIO_ANALOG_MIC0, ADC_CH_INPUT0NP_DIFF, ADC_CH_DISABLE, ADC_CH_DISABLE, ADC_CH_DISABLE},
	{AUDIO_ANALOG_MIC1, ADC_CH_INPUT0NP_DIFF, ADC_CH_DISABLE, ADC_CH_DISABLE, ADC_CH_DISABLE},
	{AUDIO_ANALOG_FM0, ADC_CH_INPUT0P, ADC_CH_DISABLE, ADC_CH_INPUT0N, ADC_CH_DISABLE},
	{AUDIO_DIGITAL_MIC0, ADC_CH_DMIC, ADC_CH_DMIC, ADC_CH_DISABLE, ADC_CH_DISABLE},
};

int board_audio_device_mapping(audio_input_map_t *input_map)
{
	int i;

	if (!input_map)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(board_audio_input_map); i++) {
		if (input_map->audio_dev == board_audio_input_map[i].audio_dev) {
			input_map->ch0_input = board_audio_input_map[i].ch0_input;
			input_map->ch1_input = board_audio_input_map[i].ch1_input;
			input_map->ch2_input = board_audio_input_map[i].ch2_input;
			input_map->ch3_input = board_audio_input_map[i].ch3_input;
			break;
		}
	}

	if (i == ARRAY_SIZE(board_audio_input_map)) {
		printk("can not find out audio dev 0x%x\n", input_map->audio_dev);
		return -ENOENT;
	}

	return 0;
}

#ifdef CONFIG_BOARD_EXTERNAL_PA_ENABLE

int board_extern_pa_ctl(u8_t pa_class, bool is_on)
{
	return 0;
}
#endif

void board_get_mmc0_pinmux_info(struct board_pinmux_info *pinmux_info)
{
#if IS_ENABLED(CONFIG_MMC_0)
	pinmux_info->pins_config = board_mmc0_config;
	pinmux_info->pins_num = ARRAY_SIZE(board_mmc0_config);
#endif
}

#if 0
void board_get_i2stx0_pinmux_info(struct board_pinmux_info *pinmux_info)
{
#if IS_ENABLED(CONFIG_AUDIO_I2STX_0)
	pinmux_info->pins_config = board_i2stx0_config;
	pinmux_info->pins_num = ARRAY_SIZE(board_i2stx0_config);
#endif
}
#endif
void board_get_pwm_pinmux_info(struct board_pwm_pinmux_info *pinmux_info)
{
#if IS_ENABLED(CONFIG_PWM)
	pinmux_info->pins_config = board_pwm_config;
	pinmux_info->pins_num = ARRAY_SIZE(board_pwm_config);
#endif
}


SYS_INIT(board_early_init, PRE_KERNEL_1, 5);

SYS_INIT(board_later_init, POST_KERNEL, 5);
