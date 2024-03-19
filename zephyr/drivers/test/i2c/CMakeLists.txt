# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_TEST_I2C_MASTER		i2c_master.c)
zephyr_library_sources_ifdef(CONFIG_TEST_I2C_SLAVE		i2c_slave.c)
