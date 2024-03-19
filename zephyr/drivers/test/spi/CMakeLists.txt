# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_TEST_SPI_MASTER		spi_master.c)
zephyr_library_sources_ifdef(CONFIG_TEST_SPI_SLAVE		spi_slave.c)
