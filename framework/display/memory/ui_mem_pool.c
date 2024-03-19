/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief mem slab manager.
*/

#include <os_common_api.h>
#include <string.h>
#include <ui_mem.h>
#include <ui_math.h>
#include <memory/gui_text_cache.h>

#if CONFIG_UI_MEM_NUMBER_BLOCKS >= 256
#  error "CONFIG_UI_MEM_NUMBER_BLOCKS >= 256"
#endif

/* align to psram cache line size (32 bytes) */
#define UI_MEM_BLOCK_SIZE   UI_ROUND_UP(CONFIG_UI_MEM_BLOCK_SIZE, 32)
#define UI_MEM_SIZE         (CONFIG_UI_MEM_NUMBER_BLOCKS * UI_MEM_BLOCK_SIZE)

#ifdef CONFIG_SIMULATOR
static uint8_t ui_mem_base[UI_MEM_SIZE];
#else
static uint8_t __aligned(32) ui_mem_base[UI_MEM_SIZE] __in_section_unique(UI_PSRAM_REGION);

static struct k_spinlock alloc_spinlock;
/* store the allocated continuous block counts (max 255) from the index */
static uint8_t alloc_count[CONFIG_UI_MEM_NUMBER_BLOCKS];
#endif

int ui_memory_init(void)
{
	gui_text_cache_init();
	return 0;
}

void *ui_memory_alloc(uint32_t size)
{
#ifndef CONFIG_SIMULATOR
	uint8_t nbr_blks = 1;
	uint8_t free_blks = 0;
	uint8_t first_blk = 0;
	k_spinlock_key_t key;

	if (size == 0) {
		return NULL;
	}

	if (size > UI_MEM_BLOCK_SIZE) {
		nbr_blks = (size + UI_MEM_BLOCK_SIZE - 1) / UI_MEM_BLOCK_SIZE;
		if (nbr_blks > CONFIG_UI_MEM_NUMBER_BLOCKS) {
			return NULL;
		}
	}

	key = k_spin_lock(&alloc_spinlock);

	for (int i = 0; i < CONFIG_UI_MEM_NUMBER_BLOCKS;) {
		if (alloc_count[i] == 0) {
			if (free_blks == 0) {
				first_blk = i;
			}

			if (++free_blks == nbr_blks) {
				alloc_count[first_blk] = nbr_blks;
				break;
			}

			i++;
		} else {
			free_blks = 0;
			i += alloc_count[i];
			if (i + nbr_blks > CONFIG_UI_MEM_NUMBER_BLOCKS) {
				break;
			}
		}
	}

	k_spin_unlock(&alloc_spinlock, key);

	if (free_blks == nbr_blks) {
		return ui_mem_base + first_blk * UI_MEM_BLOCK_SIZE;
	}

	return NULL;
#else
	return malloc(size);
#endif
}

void ui_memory_free(void *ptr)
{
#ifndef CONFIG_SIMULATOR
	uint8_t *ptr8 = ptr;
	uint8_t blkidx;
	k_spinlock_key_t key;

	if (ptr8 < ui_mem_base || ptr8 >= ui_mem_base + UI_MEM_SIZE) {
		return;
	}

	blkidx = (uint32_t)(ptr8 - ui_mem_base) / UI_MEM_BLOCK_SIZE;

	key = k_spin_lock(&alloc_spinlock);

	__ASSERT(alloc_count[blkidx] > 0, "double-free for memory at %p", ptr8);

	alloc_count[blkidx] = 0;

	k_spin_unlock(&alloc_spinlock, key);	
#else
	free(ptr);
#endif
}

void ui_memory_dump_info(uint32_t index)
{
#ifndef CONFIG_SIMULATOR
	printk("heap at %p, block size %lu, count %u\n", ui_mem_base,
			UI_MEM_BLOCK_SIZE, CONFIG_UI_MEM_NUMBER_BLOCKS);

	for (int i = 0; i < CONFIG_UI_MEM_NUMBER_BLOCKS;) {
		if (alloc_count[i] > 0) {
			void *ptr = ui_mem_base + i * UI_MEM_BLOCK_SIZE;

			printk("ptr %p, %u blocks\n", ptr, alloc_count[i]);

			i += alloc_count[i];
		} else {
			i += 1;
		}
	}
#endif
}
