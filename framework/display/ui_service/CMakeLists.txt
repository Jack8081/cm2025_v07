# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(gesture.c view_cache.c view_stack.c)
zephyr_library_sources_ifdef(CONFIG_LV_DISP_TOP_LAYER msgbox_cache_lvgl.c)
