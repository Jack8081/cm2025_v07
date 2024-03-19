# SPDX-License-Identifier: Apache-2.0
zephyr_library()

zephyr_library_sources_ifdef(CONFIG_PA pa_common.c) 
add_subdirectory_ifdef(CONFIG_PA_ACM8625 ACM8625)
