# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_DISPLAY_CONTROLLER controller)
add_subdirectory_ifdef(CONFIG_DISPLAY_CONTROLLER panel)
add_subdirectory_ifdef(CONFIG_DISPLAY_ENGINE engine)

zephyr_sources(display_graphics.c)
zephyr_sources_ifdef(CONFIG_ST7789V		display_st7789v.c)

