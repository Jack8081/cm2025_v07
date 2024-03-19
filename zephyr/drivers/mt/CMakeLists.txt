# SPDX-License-Identifier: Apache-2.0
zephyr_library()

zephyr_library_sources_ifdef(CONFIG_MAGIC_TUNE mt_common.c) 
add_subdirectory_ifdef(CONFIG_MAGIC_TUNE_CM001X1 CM001X1)
