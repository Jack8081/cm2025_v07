#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_sources_ifdef(CONFIG_BOARD_LARK_DVB_WATCH	sensor_lark_dvb.c)
zephyr_sources_ifdef(CONFIG_BOARD_LARK_DVB_WATCH_EXT_NOR	sensor_lark_dvb_ext_nor.c)
