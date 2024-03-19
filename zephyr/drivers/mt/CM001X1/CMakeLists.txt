# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_MAGIC_TUNE_CM001X1 mt_cm001x1.c) 