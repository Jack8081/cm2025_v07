/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TP Keyboard driver for Actions SoC
 */

#include <errno.h>
#include <kernel.h>
#include <string.h>
#include <init.h>
#include <irq.h>
#include <drivers/input/input_dev.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <board.h>
#include <logging/log.h>
#include <device.h>
#include <drivers/gpio.h>
#include <soc.h>
#include <string.h>
#include <drivers/i2c.h>
#include <soc_pmu.h>

#ifdef CONFIG_CFG_DRV
#include <config.h>
#include <drivers/cfg_drv/driver_config.h>
#endif

LOG_MODULE_REGISTER(tapkey, CONFIG_SYS_LOG_INPUT_DEV_LEVEL);

#define DT_DRV_COMPAT actions_tapkey

#define tp_slaver_addr             (0x4e >> 1)

//#define TAP_ISR_PIN		DT_GPIO_PIN_BY_IDX(DT_DRV_INST(0), isr_gpios, 0)
#define mode_pin (GPIO_PULL_UP | GPIO_INPUT | GPIO_INT_DEBOUNCE)
#define mode_pin_irq (GPIO_INT_EDGE_FALLING)
#define TAPKEY_PIN_MODE_DEFAULT (GPIO_INPUT)


struct acts_tapkey_data {
	struct device *i2c_dev;
	struct device *isr_dev;
	CFG_Type_Tap_Key_Control configs;
	struct gpio_callback key_gpio_cb;
	struct device *this_dev;
	u8_t past_status;
	input_notify_t notify;

	struct acts_pin_config irq_pinmux;

	struct k_delayed_work timer;
	struct k_delayed_work submit_timer;
};

struct acts_tapkey_config {
	//const struct acts_pin_config *pinmux;
	uint8_t pinmux_size;
};

static struct acts_tapkey_data tapkey_acts_ddata;

//static const struct acts_pin_config pins_tapkey[] = {FOREACH_PIN_MFP(0)};

static const struct acts_tapkey_config tapkey_acts_cdata;

static void da230_ctrl_init(struct acts_tapkey_data *tapkey);

static void tapkey_acts_poll(struct k_work *work)
{
	struct acts_tapkey_data *tapkey = CONTAINER_OF(work, struct acts_tapkey_data, timer);
	//struct device *dev = tapkey->this_dev;
	struct input_value val;

	val.keypad.code = KEY_PAUSE_AND_RESUME;
	val.keypad.type = EV_KEY;
	val.keypad.value = 1;

	tapkey->notify(NULL, &val);

	k_sleep(K_MSEC(20));

	val.keypad.value = 0;
	tapkey->notify(NULL, &val);

}

static void tapkey_acts_enable(const struct device *dev)
{
	struct acts_tapkey_data *tapkey = dev->data;
	struct acts_pin_config *pinconf = &tapkey->irq_pinmux;

	da230_ctrl_init(tapkey);

	gpio_pin_interrupt_configure(tapkey->isr_dev , (pinconf->pin_num % 32), GPIO_INT_EDGE_FALLING);//GPIO_INT_DISABLE

	LOG_DBG("enable tpkey");

}

static void tapkey_acts_disable(const struct device *dev)
{
	struct acts_tapkey_data *tapkey = dev->data;
	struct acts_pin_config *pinconf = &tapkey->irq_pinmux;

	gpio_pin_interrupt_configure(tapkey->isr_dev , (pinconf->pin_num % 32), GPIO_INT_DISABLE);//GPIO_INT_DISABLE

	LOG_DBG("disable tpkey");

}

static void tapkey_acts_inquiry(const struct device *dev, struct input_value *val)
{
	return;
}

static void tapkey_acts_register_notify(const struct device *dev, input_notify_t notify)
{
	struct acts_tapkey_data *tapkey = dev->data;

	LOG_DBG("register notify 0x%x", (uint32_t)notify);

	tapkey->notify = notify;

}

static void tpkey_acts_unregister_notify(const struct device *dev, input_notify_t notify)
{
	struct acts_tapkey_data *tapkey = dev->data;

	LOG_DBG("unregister notify 0x%x", (uint32_t)notify);

	tapkey->notify = NULL;

}

const struct input_dev_driver_api tapkey_acts_driver_api = {
	.enable = tapkey_acts_enable,
	.disable = tapkey_acts_disable,
	.inquiry = tapkey_acts_inquiry,
	.register_notify = tapkey_acts_register_notify,
	.unregister_notify = tpkey_acts_unregister_notify,
};

static u8_t da230_register_write(const struct device *dev, u8_t addr, u8_t data)
{
	u8_t cmd[6] = {0};

	cmd[0] = addr;
	cmd[1] = data;

	if(-1 == i2c_write(dev, cmd, 2, tp_slaver_addr)) {
		return -1;
	}

	return 0;
}

static u8_t da230_register_read(const struct device *dev, u8_t addr, u8_t *data, u8_t len)
{
	u8_t cmd[6] = {0};

	cmd[0] = addr;

	if(-1 == i2c_write(dev, cmd, 1, tp_slaver_addr)) {
		return -1;
	} else if(-1 == i2c_read(dev, data, len, tp_slaver_addr)) {//tp_slaver_addr
		return -1;
	}

	return 0;
}

static void da230_ctrl_init(struct acts_tapkey_data *tapkey)
{

	//u8_t  chip_id = 0;
	uint8_t cmd[6] = {0};
    uint8_t tap_ths = tapkey->configs.First_Tap_Sensitivity;

	//这里可能需要加一个中断管脚的上拉，不过没注意这是为了什么？

	// Soft Reset
	da230_register_write(tapkey->i2c_dev, 0x00, 0x24);

	k_sleep(K_MSEC(50));

	// RESOLUTION_RANGE: Wdt_en = 1, Wdt_time = 50ms, FS = 4g
	da230_register_write(tapkey->i2c_dev, 0x0F, 0x61);

	// CHIP_ID == 0x13
	da230_register_read(tapkey->i2c_dev, 0x01, cmd, 1);
	printk("CHIP_ID: 0x%02X", cmd[0]);

	// ENGINEERING_MODE ?
	da230_register_write(tapkey->i2c_dev, 0x7F, 0x83);

	da230_register_write(tapkey->i2c_dev, 0x7F, 0x69);

	da230_register_write(tapkey->i2c_dev, 0x7F, 0xbd);

	// MODE_BW: PWR_OFF = normal mode
	da230_register_write(tapkey->i2c_dev, 0x11, 0x14);


	// ODR_AXIS: ODR = 125Hz
	da230_register_write(tapkey->i2c_dev, 0x10, 0x07);

	// ???
	da230_register_write(tapkey->i2c_dev, 0x2F, 0x01);

	da230_register_write(tapkey->i2c_dev, 0x30, 0x62);

	da230_register_write(tapkey->i2c_dev, 0x31, 0x46);

	da230_register_write(tapkey->i2c_dev, 0x32, 0x32);

	// TAP_THS
	da230_register_write(tapkey->i2c_dev, 0x2B, tap_ths);

	// TAP_DUR: Tap_quiet = 20ms, Tap_shock = 50ms, Tap_dur = 50ms
	da230_register_write(tapkey->i2c_dev, 0x2A, 0x80);

	/* 中断电平锁存时间: 10 ms
	 */
	// INT_LATCH: Latch_int1 = non-latched
	da230_register_write(tapkey->i2c_dev, 0x21, 0x00);

	// INT_MAP1: Int1_s_tap = INT1
	da230_register_write(tapkey->i2c_dev, 0x19, 0x20);

	// INT_CONFIG: Int1_lvl
	cmd[0] = (tapkey->configs.INT1_Active_Level == GPIO_LEVEL_LOW) ? 0x01 : 0x00;
	da230_register_write(tapkey->i2c_dev, 0x20, cmd[0]);

	// INT_SET1: S_tap_int_en = enable
	da230_register_write(tapkey->i2c_dev, 0x16, 0x20);

	// close i2c pullup ?
	// da230_reg_write(0x8F, 0x90);
}

#if 0
static void notify(struct device *dev, struct input_value *val)
{
	printk("type: %d, code:%d, value:%d)\n", val->keypad.type, val->keypad.code, val->keypad.value);

}

static void da230_test(const struct device *dev)
{
	struct acts_tapkey_data *tapkey = dev->data;
	const struct acts_tapkey_config *cfg = dev->config;
	int ret;

	tapkey_acts_register_notify(dev, notify);
	tapkey_acts_enable(dev);
	while(1);
}
#endif

static void _tabkey_irq_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct acts_tapkey_data *tapkey = &tapkey_acts_ddata;

    printk("%s\n", __func__);
	//sys_trace_void(SYS_TRACE_ID_TP_IRQ);
	k_delayed_work_submit(&tapkey->timer, K_MSEC(20));

	//sys_trace_end_call(SYS_TRACE_ID_TP_IRQ);
}

int tapkey_acts_init(const struct device *dev)
{
	struct acts_tapkey_data *tapkey = dev->data;
	//const struct acts_tapkey_config *cfg = dev->config;
	struct acts_pin_config *pinconf = &tapkey->irq_pinmux;
	int ret;

	ret = cfg_get_by_key(ITEM_TAP_KEY_CONTROL,&tapkey->configs, sizeof(tapkey->configs));
	if (!ret) {
		LOG_ERR("failed to get ITEM_TAP_KEY_CONTROL");
		return -ESRCH;
	}

	if(tapkey->configs.Tap_Ctrl_Select != TAP_CTRL_DA230)
		return -1;

    if(tapkey->configs.INT1_Pin != 0){
        pinconf->pin_num = tapkey->configs.INT1_Pin;
    }else{
        printk("tapkey:pinnum err\n");
    }

    pinconf->mode = TAPKEY_PIN_MODE_DEFAULT;
    if(tapkey->configs.INT1_Pull_Up_Down == CFG_GPIO_PULL_UP){
        pinconf->mode |= GPIO_PULL_UP;
    }
    else if(tapkey->configs.INT1_Pull_Up_Down == CFG_GPIO_PULL_UP_10K){
        pinconf->mode |= GPIO_PULL_UP_STRONG;
    }
    else if(tapkey->configs.INT1_Pull_Up_Down == CFG_GPIO_PULL_DOWN){
        pinconf->mode |= GPIO_PULL_DOWN;
    }else{
    }

    tapkey->isr_dev = (struct device *)device_get_binding(CONFIG_GPIO_PIN2NAME(pinconf->pin_num));
	if (!tapkey->isr_dev) {
		printk("can not access tapkey gpio device\n");
		return -1;
	}

    gpio_pin_configure(tapkey->isr_dev, (pinconf->pin_num % 32), pinconf->mode);
    gpio_init_callback(&tapkey->key_gpio_cb, _tabkey_irq_callback, BIT(pinconf->pin_num % 32));
    gpio_add_callback(tapkey->isr_dev, &tapkey->key_gpio_cb);
    //gpio_pin_interrupt_configure(tapkey->isr_dev, (pinconf->pin_num % 32), GPIO_INT_EDGE_BOTH);

	tapkey->i2c_dev = (struct device *)device_get_binding(CONFIG_TABKEY_I2C_NAME);
	if (!tapkey->i2c_dev) {
		printk("can not access right i2c device\n");
		return -1;
	}

	tapkey->this_dev = (struct device *)dev;

	k_delayed_work_init(&tapkey->timer, tapkey_acts_poll);

//	da230_test(dev);

	return 0;

}

#if IS_ENABLED(CONFIG_TAPKEY)
DEVICE_DEFINE(IS_ENABLED, CONFIG_TAPKEY_DEV_NAME, tapkey_acts_init,
			NULL, &tapkey_acts_ddata, &tapkey_acts_cdata, POST_KERNEL,
			60, &tapkey_acts_driver_api);
#endif

