/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr.h>
#include <init.h>
#include <sys/sys_heap.h>
#include <lvgl.h>
#include "lvgl_malloc.h"

#ifdef CONFIG_LV_MEM_CUSTOM

#define HEAP_BYTES (CONFIG_LV_MEM_SIZE_KILOBYTES * 1024)

__in_section_unique(lvgl.noinit.malloc) __aligned(4) static char lvgl_malloc_heap_mem[HEAP_BYTES];
__in_section_unique(lvgl.noinit.malloc) static struct sys_heap lvgl_malloc_heap;

void *lvgl_malloc(size_t size)
{
	return sys_heap_aligned_alloc(&lvgl_malloc_heap, sizeof(void *), size);
}

void *lvgl_realloc(void *ptr, size_t requested_size)
{
	return sys_heap_aligned_realloc(&lvgl_malloc_heap, ptr, sizeof(void *), requested_size);
}

void lvgl_free(void *ptr)
{
	sys_heap_free(&lvgl_malloc_heap, ptr);
}

static int lvgl_malloc_prepare(const struct device *unused)
{
	sys_heap_init(&lvgl_malloc_heap, lvgl_malloc_heap_mem, HEAP_BYTES);
	return 0;
}

SYS_INIT(lvgl_malloc_prepare, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

void lvgl_heap_dump(void)
{
	sys_heap_dump(&lvgl_malloc_heap);
}

#else /* CONFIG_LV_MEM_CUSTOM */

void lvgl_heap_dump(void)
{
	lv_mem_monitor_t monitor;

	lv_mem_monitor(&monitor);

	printk("            total: %u\n", monitor.total_size);
	printk("         free_cnt: %u\n", monitor.free_cnt);
	printk("        free_size: %u\n", monitor.free_size);
	printk("free_biggest_size: %u\n", monitor.free_biggest_size);
	printk("         used_cnt: %u\n", monitor.used_cnt);
	printk("         max_used: %u\n", monitor.max_used);
}

#endif /* CONFIG_LV_MEM_CUSTOM */
