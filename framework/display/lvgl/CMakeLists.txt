# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_RES_MANAGER lvgl_img_loader.c)
zephyr_library_sources_ifdef(CONFIG_LVGL_USE_VIRTUAL_DISPLAY lvgl_memory.c)
