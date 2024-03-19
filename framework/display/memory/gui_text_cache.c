/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <sys/sys_heap.h>
#include <memory/gui_text_cache.h>

#if CONFIG_GUI_TEXT_IMG_CACHE_SIZE > 0

#ifdef CONFIG_SIMULATOR

void *gui_text_cache_malloc(size_t size)
{
	return malloc(size);
}

void gui_text_cache_free(void *ptr)
{
	free(ptr);
}

int gui_text_cache_init(void)
{
	return 0;
}

#else /* CONFIG_SIMULATOR */

#include <zephyr.h>

__in_section_unique(UI_PSRAM_REGION) __aligned(4) static char gui_text_cache_mem[CONFIG_GUI_TEXT_IMG_CACHE_SIZE];
__in_section_unique(UI_PSRAM_REGION) static struct sys_heap gui_text_cache_heap;

void *gui_text_cache_malloc(size_t size)
{
	return sys_heap_aligned_alloc(&gui_text_cache_heap, sizeof(void *), size);
}

void gui_text_cache_free(void *ptr)
{
	sys_heap_free(&gui_text_cache_heap, ptr);
}

int gui_text_cache_init(void)
{
	sys_heap_init(&gui_text_cache_heap, gui_text_cache_mem, CONFIG_GUI_TEXT_IMG_CACHE_SIZE);
	return 0;
}

#endif /* CONFIG_SIMULATOR */

#endif /* CONFIG_GUI_TEXT_IMG_CACHE_SIZE > 0 */
