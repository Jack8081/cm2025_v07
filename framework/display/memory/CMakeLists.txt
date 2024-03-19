# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library_sources_ifdef(CONFIG_UI_MEM_USE_SLAB ui_mem_slab.c)
zephyr_library_sources_ifdef(CONFIG_UI_MEM_USE_POOL ui_mem_pool.c)
zephyr_library_sources_ifdef(CONFIG_RES_MANAGER res_mempool.c)
zephyr_library_sources_ifdef(CONFIG_BITMAP_FONT font_mempool.c)
zephyr_library_sources(gui_text_cache.c)
