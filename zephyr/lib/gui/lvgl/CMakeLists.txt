# SPDX-License-Identifier: Apache-2.0

zephyr_interface_library_named(lvgl)

target_include_directories(lvgl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library()

zephyr_library_sources(lvgl.c lvgl_malloc.c)

zephyr_library_sources_ifdef(CONFIG_LV_TICK_CUSTOM lvgl_tick.c)
zephyr_library_sources_ifdef(CONFIG_LVGL_USE_FILESYSTEM lvgl_fs.c)
zephyr_library_sources_ifndef(CONFIG_LVGL_USE_VIRTUAL_DISPLAY lvgl_display.c)

zephyr_library_link_libraries(lvgl)
