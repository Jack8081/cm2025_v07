# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(include)

add_subdirectory_ifdef(CONFIG_BT_MAIN_APP config)
add_subdirectory_ifdef(CONFIG_BT_MAIN_APP ui_key_map)

add_subdirectory_ifdef(CONFIG_BT_MAIN_APP common)
add_subdirectory_ifdef(CONFIG_BT_MAIN_APP led_display)

add_subdirectory_ifdef(CONFIG_TOOL usp)
add_subdirectory_ifdef(CONFIG_TOOL_ASET aset)
add_subdirectory_ifdef(CONFIG_TOOL stub)
add_subdirectory_ifdef(CONFIG_CONSUMER_EQ consumer_eq)