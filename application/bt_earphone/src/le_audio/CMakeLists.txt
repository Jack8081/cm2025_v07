# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories_ifdef(CONFIG_LE_AUDIO_BACKGROUD_IN_BTCALL .)

target_sources(app PRIVATE leaudio_main.c)
target_sources(app PRIVATE leaudio_event.c)
target_sources(app PRIVATE leaudio_view.c)
target_sources(app PRIVATE leaudio_media.c)
target_sources(app PRIVATE leaudio_media_profile.c)
target_sources(app PRIVATE leaudio_ctrl.c)

