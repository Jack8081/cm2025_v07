/*
 * PA Module ACM8625
*/

#include <zephyr.h>
#include <board.h>
#include <drivers/i2c.h>
#include <device.h>
#include <stdlib.h>
#include <os_common_api.h>
#include <board_cfg.h>

#include <drivers/mt.h>
#include "mt_cm001x1.h"

#define CM001X1_I2C_ADDR_0     (0xA0)
#define CM001X1_I2C_ADDR_1     (0xB8)

#define CM001X1_I2C_ADDR       (CM001X1_I2C_ADDR_0 >> 1)

#define CM001X1_IOW_BASE_0     (0x10)
#define CM001X1_IOW_BASE_1     (0x20)
#define CM001X1_IOR_BASE_0     (0x30)

static int _mt_drv_i2c_readbytes(const struct device *i2c_dev, uint8_t reg_addr, uint8_t *pbuf, uint8_t size)
{
    struct i2c_msg msg[2];

    msg[0].flags = I2C_MSG_WRITE;
    msg[0].buf   = &reg_addr;
    msg[0].len   = 1;

    msg[1].flags = I2C_MSG_READ | I2C_MSG_RESTART;
    msg[1].buf   = pbuf;
    msg[1].len   = size;

    return i2c_transfer(i2c_dev, msg, 2, CM001X1_I2C_ADDR);
}

static int _mt_drv_i2c_writebytes(const struct device *i2c_dev, uint8_t reg_addr, uint8_t *pbuf, uint8_t size)
{
    struct i2c_msg msg[2];

    msg[0].flags = I2C_MSG_WRITE;
    msg[0].buf   = &reg_addr;
    msg[0].len   = 1;

    msg[1].flags = I2C_MSG_WRITE;
    msg[1].buf   = pbuf;
    msg[1].len   = size;

    return i2c_transfer(i2c_dev, msg, 2, CM001X1_I2C_ADDR);
}
#if 0
static uint8_t _pa_drv_i2c_readbyte(const struct device *i2c_dev, uint8_t reg_addr, uint8_t *reg_val)
{
    struct i2c_msg msg[2];

    msg[0].flags = I2C_MSG_WRITE;
    msg[0].buf   = &reg_addr;
    msg[0].len   = 1;

    msg[1].flags = I2C_MSG_READ | I2C_MSG_RESTART;
    msg[1].buf   = reg_val;
    msg[1].len   = 1;

    return i2c_transfer(i2c_dev, msg, 2, ACM8625S_I2C_ADDR);
}

static uint8_t _pa_drv_i2c_writebyte(const struct device *i2c_dev, uint8_t reg_addr, uint8_t reg_val)
{
    uint8_t val_buf[2];
    struct i2c_msg msg;

    val_buf[0] = reg_addr;
    val_buf[1] = reg_val;

    msg.flags = I2C_MSG_WRITE;
    msg.buf   = val_buf;
    msg.len   = 2;

    return i2c_transfer(i2c_dev, &msg, 1, ACM8625S_I2C_ADDR);
}
#endif

int mt_cm001x1_init(const struct device *mt_dev)
{
    printk("MT CM001X1 init successful !\n");

    return true;
}

int mt_cm001x1_ioctl(const struct device *mt_dev, struct mt_ioctl_info *p_mt_info)
{
    int ret = 0;
    struct mt_device_data *mt_data = mt_dev->data;

    if ((mt_dev == NULL) || (p_mt_info->size != MT_IO_BYTES_NUM))
    {
        printk("mt_cm001x1_ioctl, params are invalid !\n");
        return -EINVAL;
    }

    switch (p_mt_info->type)
    {
        case MT_IOW_BLUK_0:
            ret = _mt_drv_i2c_writebytes(mt_data->i2c_dev, CM001X1_IOW_BASE_0, p_mt_info->pbulk, p_mt_info->size);
            break;

        case MT_IOW_BLUK_1:
            ret = _mt_drv_i2c_writebytes(mt_data->i2c_dev, CM001X1_IOW_BASE_1, p_mt_info->pbulk, p_mt_info->size);
            break;

        case MT_IOR_BLUK_0:
            ret = _mt_drv_i2c_readbytes(mt_data->i2c_dev, CM001X1_IOR_BASE_0, p_mt_info->pbulk, p_mt_info->size);
            break;

        default:
            break;
    }

    return ret;
}
