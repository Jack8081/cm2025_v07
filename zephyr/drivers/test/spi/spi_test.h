#ifndef __SPI_TEST_H_
#define __SPI_TEST_H_

#include <string.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <drivers/spi.h>
#include <zephyr.h>
#include "soc_pinmux.h"
#include <mem_manager.h>
#include <system_app.h>
#include "os_common_api.h"

#define SPI_CLK 4
#define SPI_MOSI 7
#define SPI_MISO 8
#define SPI_CS 9


#define TEST_SPI_BUF_LEN 64

#endif
