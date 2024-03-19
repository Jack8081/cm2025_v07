# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

SET(LIB_NAME display)
SET(ZEPHYR_CURRENT_LIBRARY ${LIB_NAME})
SET(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/../")

zephyr_include_directories(./)
zephyr_library_named(${LIB_NAME})

zephyr_library_compile_options(-O3)

target_include_directories(${LIB_NAME} INTERFACE
	.
	lvgl
	ui_service
	freetype/include
	decompress
)

add_subdirectory(ui_service)
add_subdirectory_ifdef(CONFIG_RES_MANAGER res_manager)
add_subdirectory_ifdef(CONFIG_BITMAP_FONT bitmap_font)
add_subdirectory(surface)

if(CONFIG_FREETYPE_FONT)
  add_subdirectory(freetype)
endif()

if(CONFIG_LVGL)
  add_subdirectory(lvgl)
endif()

add_subdirectory(decompress)

target_link_libraries(${LIB_NAME} INTERFACE zephyr_interface)

if(CONFIG_LVGL)
zephyr_library_link_libraries(lvgl)
endif()