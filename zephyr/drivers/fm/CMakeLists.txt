# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_FM fm_common.c)
add_subdirectory_ifdef(CONFIG_FM_AR1019 AR1019)
add_subdirectory_ifdef(CONFIG_FM_RDA5807 RDA5807)
#zephyr_library_sources_ifdef(CONFIG_FM_AR1019 fm_ar1019.c)
