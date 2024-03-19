#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_library_sources_ifdef(CONFIG_WDOG_ACTS watchdog_hal.c)

