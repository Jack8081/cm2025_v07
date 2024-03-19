# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_MEDIA)

set(ZEPHYR_CURRENT_LIBRARY acts_media)

add_library(acts_media INTERFACE)

zephyr_include_directories(
	include
)

target_include_directories(acts_media INTERFACE
	include
)

zephyr_library()

zephyr_library_sources(media_mem.c)
zephyr_library_sources_ifdef(CONFIG_MEDIA_PLAYER media_player.c)
zephyr_library_sources_ifdef(CONFIG_MEDIA_SERVICE codec_config.c media_effect_param.c)

add_subdirectory_ifdef(CONFIG_SOC_SERIES_LARK libal)
add_subdirectory_ifdef(CONFIG_SOC_SERIES_CUCKOO libal_cuckoo)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libmedia/CMakeLists.txt)
	add_subdirectory_ifdef(CONFIG_MEDIA_SERVICE libmedia)
else()
	zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR})
    zephyr_library_import(${CONFIG_MEDIA_LIB_NAME} ${CMAKE_CURRENT_LIST_DIR}/lib${CONFIG_MEDIA_LIB_NAME}.a)
endif()

zephyr_library_link_libraries_ifdef(CONFIG_MEDIA_SERVICE ${CONFIG_MEDIA_LIB_NAME})

zephyr_library_link_libraries_ifdef(CONFIG_FAT_FILESYSTEM_ELM ELMFAT INTERFACE)

target_link_libraries(acts_media INTERFACE zephyr_interface)

endif()



