# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

target_sources(app PRIVATE audio_channel.c)
target_sources(app PRIVATE app_ui_event_notify.c)

