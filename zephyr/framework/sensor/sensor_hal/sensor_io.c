/*******************************************************************************
 * @file    sensor_io.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2020-08-12
 * @brief   sensor testing module
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <sensor_hal.h>
#include <sensor_io.h>
#include <soc.h>
#include <zephyr.h>
#include <drivers/gpio.h>

/******************************************************************************/
//constants
/******************************************************************************/
#define CFG_IRQ_ON	(GPIO_INT_ENABLE | GPIO_INT_EDGE | GPIO_INT_HIGH_1)
#define CFG_IRQ_OFF	(GPIO_INT_DISABLE)
/******************************************************************************/
//variables
/******************************************************************************/
static struct device *gpio_dev[4] =  { NULL };
static sensor_io_cb_t sensor_io_cb[NUM_SENSOR] = { NULL };
static struct gpio_callback sensor_gpio_cb[NUM_SENSOR] = { NULL };
static struct k_timer sensor_tm[NUM_SENSOR] = { 0 };

/******************************************************************************/
//functions
/******************************************************************************/
void sensor_io_init(const sensor_dev_t *dev)
{
    int pin, grp;

    // init int pin
    if (dev->io.int_io > 0) {
        grp = (dev->io.int_io / 32);
        pin = (dev->io.int_io % 32);
        gpio_pin_interrupt_configure(gpio_dev[grp], pin,	CFG_IRQ_OFF);
    }

    // power on
    if (dev->io.pwr_io > 0) {
        grp = (dev->io.pwr_io / 32);
        pin = (dev->io.pwr_io % 32);
        gpio_pin_configure(gpio_dev[grp], pin,	GPIO_OUTPUT);
        gpio_pin_set(gpio_dev[grp], pin, dev->io.pwr_val);
    }

    // reset
    if (dev->io.rst_io > 0) {
        grp = (dev->io.rst_io / 32);
        pin = (dev->io.rst_io % 32);
        gpio_pin_configure(gpio_dev[grp], pin,	GPIO_OUTPUT);
        gpio_pin_set(gpio_dev[grp], pin, dev->io.rst_val);
        k_msleep(dev->io.rst_lt);
        gpio_pin_set(gpio_dev[grp], pin, !dev->io.rst_val);
        k_msleep(dev->io.rst_ht);
    }
}

void sensor_io_deinit(const sensor_dev_t *dev)
{
    int pin, grp;

    // reset on
    if (dev->io.rst_io > 0) {
        grp = (dev->io.rst_io / 32);
        pin = (dev->io.rst_io % 32);
        gpio_pin_set(gpio_dev[grp], pin, dev->io.rst_val);
    }

    // power down
    if (dev->io.pwr_io > 0) {
        grp = (dev->io.pwr_io / 32);
        pin = (dev->io.pwr_io % 32);
        gpio_pin_set(gpio_dev[grp], pin, !dev->io.pwr_val);
    }
}

static void sensor_gpio_callback(const struct device *port,
                    struct gpio_callback *cb, gpio_port_pins_t pins)
{
    int idx;

    for (idx = 0; idx < NUM_SENSOR; idx ++) {
        if (cb == &sensor_gpio_cb[idx]) {
            sensor_io_cb[idx](pins, idx);
        }
    }
}

static void senosr_tm_callback(struct k_timer *timer)
{
    int id = (int)k_timer_user_data_get(timer);

    sensor_io_cb[id](id, id);
}

void sensor_io_enable_irq(const sensor_dev_t *dev, sensor_io_cb_t cb, int id)
{
    int pin, grp;
    k_timeout_t timeout;

    sensor_io_cb[id] = cb;

    // irq enable
    if (dev->io.int_io > 0) {
        grp = (dev->io.int_io / 32);
        pin = (dev->io.int_io % 32);
        acts_pinmux_set(dev->io.int_io, 0);
        gpio_pin_configure(gpio_dev[grp], pin, GPIO_INPUT);
        gpio_pin_interrupt_configure(gpio_dev[grp], pin, CFG_IRQ_ON);
        gpio_init_callback(&sensor_gpio_cb[id], sensor_gpio_callback, (1 << pin));
        gpio_add_callback(gpio_dev[grp], &sensor_gpio_cb[id]);
    } else {
        // timer enable
        if (dev->hw.timeout > 0) {
            timeout = Z_TIMEOUT_MS(dev->hw.timeout);
            k_timer_init(&sensor_tm[id], senosr_tm_callback, NULL);
            k_timer_user_data_set(&sensor_tm[id], (void*)id);
            k_timer_start(&sensor_tm[id], timeout, timeout);
        }
    }
}

void sensor_io_disable_irq(const sensor_dev_t *dev, sensor_io_cb_t cb, int id)
{
    int pin, grp;

    // irq disable
    if (dev->io.int_io > 0) {
        grp = (dev->io.int_io / 32);
        pin = (dev->io.int_io % 32);
        gpio_pin_configure(gpio_dev[grp], pin, GPIO_INPUT);
        gpio_pin_interrupt_configure(gpio_dev[grp], pin, CFG_IRQ_OFF);
        gpio_init_callback(&sensor_gpio_cb[id], sensor_gpio_callback, (1 << pin));
        gpio_remove_callback(gpio_dev[grp], &sensor_gpio_cb[id]);
        acts_pinmux_set(dev->io.int_io, dev->io.int_mfp);
    } else {
        // timer disable
        if (dev->hw.timeout > 0) {
            k_timer_stop(&sensor_tm[id]);
        }
    }

    sensor_io_cb[id] = NULL;
}

static int sensor_io_dev_init(const struct device *dev)
{
    ARG_UNUSED(dev);

    /* get device */
    gpio_dev[0] = (struct device*)device_get_binding("GPIOA");
    if (!gpio_dev[0]) {
        printk("[gpioa] get device failed!\n");
        return -1;
    }

    /* get device */
    gpio_dev[1] = (struct device*)device_get_binding("GPIOB");
    if (!gpio_dev[1]) {
        printk("[gpiob] get device failed!\n");
        return -1;
    }

    /* get device */
    gpio_dev[3] = (struct device*)device_get_binding("GPIOD");
    if (!gpio_dev[3]) {
        printk("[gpiod] get device failed!\n");
        return -1;
    }

    return 0;
}

SYS_INIT(sensor_io_dev_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

