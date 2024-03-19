#include "i2c_test.h"

const struct device *i2c_dev;

static void i2c_test_read_sync(void)
{
    int i = 0;
    u8_t test_buf[64] = {0};

    acts_pinmux_set(I2C_SCL, 0x4 | 1 << 5 | ((4 / 2) << 12) | 1 << 11);

    i2c_configure(i2c_dev, 0x12);

    memset(test_buf, 0x0, 64);
    
    i2c_read(i2c_dev, test_buf, 60, I2C_SLAVE_ADDR);
    
    for(i = 0; i < 60; i++)
    {
        if(i % 16 == 0 && i != 0)
            printk("\n");
        printk("%x ", test_buf[i]);
    }

    return;
}

static void i2c_test_write_sync(void)
{
    u8_t test_buf[64] = {0};
    acts_pinmux_set(I2C_SCL, 0x4 | 1 << 5 | ((4 / 2) << 12) | 1 << 11);

    i2c_configure(i2c_dev, 0x12);

    memset(test_buf, 0x88, 64);
    
    i2c_write(i2c_dev, test_buf, 60, I2C_SLAVE_ADDR);
    
    return;
}


int i2c_write_async_func_t(void *cb_data, struct i2c_msg *msgs, uint8_t num_msgs, bool is_err)
{
    printk("write_async success\n");
    return 0;
}

int i2c_read_async_func_t(void *cb_data, struct i2c_msg *msgs, uint8_t num_msgs, bool is_err)
{
    printk("read_async success\n");
    return 0;
}

static void i2c_test_write_async(void)
{
    u8_t test_buf[64] = {0};
    acts_pinmux_set(I2C_SCL, 0x4 | 1 << 5 | ((4 / 2) << 12) | 1 << 11);

    i2c_configure(i2c_dev, 0x12);
    memset(test_buf, 0x88, 64);
    i2c_write_async(i2c_dev, test_buf, 60, I2C_SLAVE_ADDR, i2c_write_async_func_t, NULL);

    k_sleep(K_MSEC(30));
    return;
}

static void i2c_test_read_async(void)
{
    u8_t test_buf[64] = {0};
    acts_pinmux_set(I2C_SCL, 0x4 | 1 << 5 | ((4 / 2) << 12) | 1 << 11);

    i2c_configure(i2c_dev, 0x12);

    i2c_read_async(i2c_dev, test_buf, 60, I2C_SLAVE_ADDR, i2c_read_async_func_t, NULL);

    return;
}

static int test_i2c_main(const struct device *arg)
{
    acts_pinmux_set(I2C_SCL, 0x4 | 1 << 5 | ((4 / 2) << 12) | 1 << 11);
    acts_pinmux_set(I2C_SDA, 0x4 | 1 << 5 | ((4 / 2) << 12) | 1 << 11);
    i2c_dev = device_get_binding("I2C_0");
    return 0;
}

static int i2c_start_read_sync_test(const struct shell *shell,
        size_t argc, char **argv)
{
    i2c_test_read_sync();
    return 0;
}

static int i2c_start_write_sync_test(const struct shell *shell,
        size_t argc, char **argv)
{
    i2c_test_write_sync();
    return 0;
}

static int i2c_start_write_async_test(const struct shell *shell,
        size_t argc, char **argv)
{
    i2c_test_write_async();
    return 0;
}

static int i2c_start_read_async_test(const struct shell *shell,
        size_t argc, char **argv)
{
    i2c_test_read_async();
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(test_i2c_cmds,
        SHELL_CMD(start_read_sync, NULL, "start test", i2c_start_read_sync_test),
        SHELL_CMD(start_write_sync, NULL, "start test", i2c_start_write_sync_test),
        SHELL_CMD(start_read_async, NULL, "start test", i2c_start_read_async_test),
        SHELL_CMD(start_write_async, NULL, "start test", i2c_start_write_async_test),
        SHELL_SUBCMD_SET_END     /* Array terminated. */
        );

SHELL_CMD_REGISTER(test_i2c, &test_i2c_cmds, "I2C commands", NULL);
SYS_INIT(test_i2c_main, APPLICATION, 10);
