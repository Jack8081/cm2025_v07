# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

target_sources(app PRIVATE led_ctrl.c)
target_sources(app PRIVATE led_display.c)
target_sources(app PRIVATE led_layer.c)

