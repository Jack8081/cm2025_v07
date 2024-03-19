#ifndef __I2C_TEST_H_
#define __I2C_TEST_H_

#include <string.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <drivers/i2c.h>
#include <zephyr.h>
#include "soc_pinmux.h"
#include <mem_manager.h>
#include <system_app.h>
#include "os_common_api.h"

#define I2C_SLAVE_ADDR 0x60

#define I2C_SCL 10
#define I2C_SDA 22

#endif
