# SPDX-License-Identifier: Apache-2.0

zephyr_library()

add_subdirectory_ifdef(CONFIG_SOC_SERIES_LARK lark)
add_subdirectory_ifdef(CONFIG_SOC_SERIES_CUCKOO cuckoo)
