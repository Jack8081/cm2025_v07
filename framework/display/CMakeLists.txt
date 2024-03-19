# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_UI_MANAGER)
set(ZEPHYR_CURRENT_LIBRARY acts_display)

add_library(acts_display INTERFACE)

zephyr_include_directories(
	include
)

target_include_directories(acts_display INTERFACE
	include
)

zephyr_library()

zephyr_library_sources(ui_math.c ui_region.c ui_manager.c)

if(CONFIG_UI_SERVICE)
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libdisplay/CMakeLists.txt)
      add_subdirectory(libdisplay)
  else()
      zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR})
      zephyr_library_import(display ${CMAKE_CURRENT_LIST_DIR}/libdisplay.a)
  endif()
endif()

zephyr_library_sources_ifdef(CONFIG_LED_MANAGER led_manager.c)
zephyr_library_sources_ifdef(CONFIG_SEG_LED_MANAGER seg_led_manager.c)
zephyr_library_sources_ifdef(CONFIG_SIMPLE_RES_MANAGER simple_res_manager.c)

add_subdirectory_ifdef(CONFIG_UI_MEMORY_MANAGER memory)
add_subdirectory_ifdef(CONFIG_LVGL_USE_VIRTUAL_DISPLAY lvgl)
add_subdirectory_ifdef(CONFIG_UI_SERVICE ui_service)

zephyr_library_link_libraries(acts_display)
target_link_libraries(acts_display INTERFACE zephyr_interface)

if(CONFIG_LVGL_USE_VIRTUAL_DISPLAY)
  zephyr_library_link_libraries(lvgl)
endif()

endif() # CONFIG_UI_MANAGER
