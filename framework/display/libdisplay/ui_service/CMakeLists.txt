# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library_sources(
  ui_service.c
  view_manager.c
  view_manager_gui.c
  input_dispatcher.c
  view_animation.c
  gesture_manager.c
)

if(CONFIG_UI_INPUT_RECORDER)
  zephyr_library_sources(
    input_recorder.c
    input_recorder_buffer.c
    input_recorder_slide_fixedstep.c
  )

  zephyr_library_sources_ifdef(CONFIG_STREAM input_recorder_stream.c)
endif()
