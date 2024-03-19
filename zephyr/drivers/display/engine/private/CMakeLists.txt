# SPDX-License-Identifier: Apache-2.0

set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../)

zephyr_library_named(${DRV_DISPLAY_ENGINE_LIB_NAME})

zephyr_library_sources_ifdef(CONFIG_DISPLAY_ENGINE de_lark.c)
