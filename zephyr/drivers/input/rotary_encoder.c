#include <errno.h>
#include <kernel.h>
#include <init.h>
#include <soc.h>
#include <sys_event.h>
#include <drivers/gpio.h>
#include <board_cfg.h>
#include <drivers/input/input_dev.h>

#define gpio_pin_a 5
#define gpio_pin_b 6

struct {
	struct k_timer timer;
	const struct device *gpio_dev;
	input_notify_t notify;
	int aValue;
} encoder_info;


//vsw_dn : clockwise
//vsw_up : Counterclockwise

enum {
	vsw_nu = 0,
	vsw_up = KEY_PREVIOUSSONG,
	vsw_dn = KEY_NEXTSONG,
};

static uint8_t const vsw[4][4] = {
	{vsw_nu, vsw_dn, vsw_up, vsw_nu},
	{vsw_up, vsw_nu, vsw_nu, vsw_dn},
	{vsw_dn, vsw_nu, vsw_nu, vsw_up},
	{vsw_nu, vsw_up, vsw_dn, vsw_nu}
};

static void encoder_acts_poll(struct k_timer *timer)
{
	int val_a,val_b, val_ab;
	struct input_value val;

	val_a = gpio_pin_get(encoder_info.gpio_dev, gpio_pin_a);
	val_b = gpio_pin_get(encoder_info.gpio_dev, gpio_pin_b);

	val_ab = (val_a << 1) | (val_b);

	if( vsw[encoder_info.aValue][val_ab] != vsw_nu )
	{
		val.keypad.value = 0;
		val.keypad.code = vsw[encoder_info.aValue][val_ab];
		val.keypad.type = EV_KEY;
		encoder_info.notify(NULL,&val);
	}
	encoder_info.aValue = val_ab;
}

static void encoder_acts_enable(const struct device *dev)
{
	int val_a,val_b;

	k_timer_start(&(encoder_info.timer), K_MSEC(10), K_MSEC(10));

	val_a = gpio_pin_get(encoder_info.gpio_dev, gpio_pin_a);
	val_b = gpio_pin_get(encoder_info.gpio_dev, gpio_pin_b);
	encoder_info.aValue = (val_a << 1) | (val_b);
}

static void encoder_acts_disable(const struct device *dev)
{
	k_timer_stop(&(encoder_info.timer));
}

static int encoder_acts_init(const struct device *dev)
{
	encoder_info.gpio_dev = device_get_binding(CONFIG_GPIO_A_NAME);

	gpio_pin_configure(encoder_info.gpio_dev, gpio_pin_a, GPIO_PULL_UP | GPIO_INPUT);
	gpio_pin_configure(encoder_info.gpio_dev, gpio_pin_b, GPIO_PULL_UP | GPIO_INPUT);

	k_timer_init(&(encoder_info.timer), encoder_acts_poll, NULL);
	//k_timer_user_data_set(&(encoder_info.timer), dev);

	return 0;
}

static void encoder_acts_register_notify(const struct device *dev, input_notify_t notify)
{
	encoder_info.notify = notify;
}

static void encoder_acts_unregister_notify(const struct device *dev, input_notify_t notify)
{
	encoder_info.notify = NULL;
}

static const struct input_dev_driver_api encoder_acts_api = {
	.enable = encoder_acts_enable,
	.disable = encoder_acts_disable,
	.register_notify = encoder_acts_register_notify,
	.unregister_notify = encoder_acts_unregister_notify,
};

#if IS_ENABLED(CONFIG_ROTARY_ENCODER)
DEVICE_DEFINE(encoder_acts, CONFIG_ROTARY_ENCODER_NAME,
		encoder_acts_init, NULL,
		NULL, NULL,
		POST_KERNEL, 60,
		&encoder_acts_api);
#endif
