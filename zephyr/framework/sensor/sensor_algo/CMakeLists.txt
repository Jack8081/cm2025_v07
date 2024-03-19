#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_include_directories_ifdef(
  CONFIG_SENSOR_ALGO
  .
)

zephyr_sources_ifdef(
  CONFIG_SENSOR_ALGO
	hr_algo.S
	sensor_algo.S
	sensor_algo_sleep.S
)
