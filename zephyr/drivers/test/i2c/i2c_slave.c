#include "i2c_test.h"

const struct device *i2c_0;
int i2c_slave_write_requested_cb(struct i2c_slave_config *config)
{
	printk("go into write_requested\n");

	return 0;
}

int i2c_slave_read_requested_cb(struct i2c_slave_config *config, uint8_t *val)
{
	printk("go into read_requested\n");
	*val = 0x86;
	return 0;
}

int i2c_slave_write_received_cb(struct i2c_slave_config *config, uint8_t val)
{
	printk("go into write_received\n");
	printk("val:0x%x\n", val);
	return 0;
}

int i2c_slave_read_processed_cb(struct i2c_slave_config *config, uint8_t *val)
{
	printk("go into read_processed\n");
	*val = 0xae;
	return 0;
}

int i2c_slave_stop_cb(struct i2c_slave_config *config)
{
	printk("go into stop\n");

	return 0;
}

struct i2c_slave_callbacks slave_callbacks = {
    .write_requested = i2c_slave_write_requested_cb,
    .read_requested = i2c_slave_read_requested_cb,
    .write_received = i2c_slave_write_received_cb,
    .read_processed = i2c_slave_read_processed_cb,
    .stop = i2c_slave_stop_cb,
};

struct i2c_slave_config slave_config = {
    .address = I2C_SLAVE_ADDR,
    .callbacks = &slave_callbacks,
};

static int test_i2c_main(const struct device *arg)
{
    acts_pinmux_set(I2C_SCL, 0x4 | 1 << 5 | ((4 / 2) << 12) | 1 << 11);
    acts_pinmux_set(I2C_SDA, 0x4 | 1 << 5 | ((4 / 2) << 12) | 1 << 11);
    i2c_0 = device_get_binding("I2C_0");
    
    return 1;    
}


static int i2c_start_test(const struct shell *shell,
        size_t argc, char **argv)
{
    acts_pinmux_set(I2C_SCL, 0x4 | 1 << 5 | ((4 / 2) << 12) | 1 << 11);
    acts_pinmux_set(I2C_SDA, 0x4 | 1 << 5 | ((4 / 2) << 12) | 1 << 11);
	i2c_slave_register(i2c_0, &slave_config);
    printk("addr:%x\n", sys_read32(0x4004800c));
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(test_i2c_cmds,
        SHELL_CMD(start, NULL, "start test", i2c_start_test),
        SHELL_SUBCMD_SET_END     /* Array terminated. */
        );

SHELL_CMD_REGISTER(test_i2c, &test_i2c_cmds, "I2C commands", NULL);
SYS_INIT(test_i2c_main, APPLICATION, 10);
