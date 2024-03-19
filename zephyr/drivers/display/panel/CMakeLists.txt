# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_PANEL_GC9C01 panel_gc9c01.c)
zephyr_sources_ifdef(CONFIG_PANEL_JD9854 panel_jd9854.c)
zephyr_sources_ifdef(CONFIG_PANEL_RM69090 panel_rm69090.c)
zephyr_sources_ifdef(CONFIG_PANEL_ST7789V2 panel_st7789v2.c)
zephyr_sources_ifdef(CONFIG_PANEL_LARK_GC9307 panel_lark_gc9307.c)
