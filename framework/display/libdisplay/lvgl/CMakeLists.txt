# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_LVGL_USE_VIRTUAL_DISPLAY lvgl_virtual_display.c lvgl_view.c lvgl_input_dev.c lvgl_sw_gpu.c)
zephyr_library_sources_ifdef(CONFIG_LV_USE_GPU lvgl_gpu.c)
zephyr_library_sources_ifdef(CONFIG_LVGL_USE_BITMAP_FONT lvgl_bitmap_font.c)
zephyr_library_sources_ifdef(CONFIG_LVGL_USE_RES_MANAGER lvgl_res_loader.c)
zephyr_library_sources_ifdef(CONFIG_LVGL_USE_FREETYPE_FONT lvgl_freetype_font.c)

if(CONFIG_LVGL)
	target_link_libraries(display INTERFACE lvgl)
endif()
