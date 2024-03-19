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
#include <memory/mem_slabs.h>

static struct k_mem_slab ui_mem_slab[CONFIG_UI_SLAB_TOTAL_NUM] __in_section_unique(UI_PSRAM_REGION);
static uint8_t system_max_used[CONFIG_UI_SLAB_TOTAL_NUM] __in_section_unique(UI_PSRAM_REGION);
static uint32_t system_max_size[CONFIG_UI_SLAB_TOTAL_NUM] __in_section_unique(UI_PSRAM_REGION);

/* aligned to psram cache line size (32 bytes) */
#define SLAB_TOTAL_SIZE UI_ROUND_UP(CONFIG_UI_SLAB0_BLOCK_SIZE * CONFIG_UI_SLAB0_NUM_BLOCKS \
				+ CONFIG_UI_SLAB1_BLOCK_SIZE * CONFIG_UI_SLAB1_NUM_BLOCKS \
				+ CONFIG_UI_SLAB2_BLOCK_SIZE * CONFIG_UI_SLAB2_NUM_BLOCKS \
				+ CONFIG_UI_SLAB3_BLOCK_SIZE * CONFIG_UI_SLAB3_NUM_BLOCKS \
				+ CONFIG_UI_SLAB4_BLOCK_SIZE * CONFIG_UI_SLAB4_NUM_BLOCKS \
				+ CONFIG_UI_SLAB5_BLOCK_SIZE * CONFIG_UI_SLAB5_NUM_BLOCKS \
				+ CONFIG_UI_SLAB6_BLOCK_SIZE * CONFIG_UI_SLAB6_NUM_BLOCKS \
				+ CONFIG_UI_SLAB7_BLOCK_SIZE * CONFIG_UI_SLAB7_NUM_BLOCKS \
				+ CONFIG_UI_SLAB8_BLOCK_SIZE * CONFIG_UI_SLAB8_NUM_BLOCKS \
				+ CONFIG_UI_SLAB9_BLOCK_SIZE * CONFIG_UI_SLAB9_NUM_BLOCKS \
				+ CONFIG_UI_SLAB10_BLOCK_SIZE * CONFIG_UI_SLAB10_NUM_BLOCKS, 32)

/* aligned to psram cache line size (32 bytes) */
static char __aligned(32) ui_mem_slab_buffer[SLAB_TOTAL_SIZE] __in_section_unique(UI_PSRAM_REGION);

#define SLAB10_BLOCK_OFF 0

#define SLAB9_BLOCK_OFF \
		SLAB10_BLOCK_OFF + \
		CONFIG_UI_SLAB10_BLOCK_SIZE * CONFIG_UI_SLAB10_NUM_BLOCKS

#define SLAB8_BLOCK_OFF \
		SLAB9_BLOCK_OFF + \
		CONFIG_UI_SLAB9_BLOCK_SIZE * CONFIG_UI_SLAB9_NUM_BLOCKS

#define SLAB7_BLOCK_OFF \
		SLAB8_BLOCK_OFF + \
		CONFIG_UI_SLAB8_BLOCK_SIZE * CONFIG_UI_SLAB8_NUM_BLOCKS

#define SLAB6_BLOCK_OFF \
		SLAB7_BLOCK_OFF + \
		CONFIG_UI_SLAB7_BLOCK_SIZE * CONFIG_UI_SLAB7_NUM_BLOCKS

#define SLAB5_BLOCK_OFF \
		SLAB6_BLOCK_OFF + \
		CONFIG_UI_SLAB6_BLOCK_SIZE * CONFIG_UI_SLAB6_NUM_BLOCKS

#define SLAB4_BLOCK_OFF \
		SLAB5_BLOCK_OFF + \
		CONFIG_UI_SLAB5_BLOCK_SIZE * CONFIG_UI_SLAB5_NUM_BLOCKS

#define SLAB3_BLOCK_OFF \
		SLAB4_BLOCK_OFF + \
		CONFIG_UI_SLAB4_BLOCK_SIZE * CONFIG_UI_SLAB4_NUM_BLOCKS

#define SLAB2_BLOCK_OFF \
		SLAB3_BLOCK_OFF + \
		CONFIG_UI_SLAB3_BLOCK_SIZE * CONFIG_UI_SLAB3_NUM_BLOCKS

#define SLAB1_BLOCK_OFF \
		SLAB2_BLOCK_OFF + \
		CONFIG_UI_SLAB2_BLOCK_SIZE * CONFIG_UI_SLAB2_NUM_BLOCKS

#define SLAB0_BLOCK_OFF \
		SLAB1_BLOCK_OFF + \
		CONFIG_UI_SLAB1_BLOCK_SIZE * CONFIG_UI_SLAB1_NUM_BLOCKS

static const struct slab_info slab_info[] = {
	{
		.slab = &ui_mem_slab[0],
		.slab_base = &ui_mem_slab_buffer[SLAB0_BLOCK_OFF],
		.block_num = CONFIG_UI_SLAB0_NUM_BLOCKS,
		.block_size = CONFIG_UI_SLAB0_BLOCK_SIZE,
	},

	{
		.slab = &ui_mem_slab[1],
		.slab_base = &ui_mem_slab_buffer[SLAB1_BLOCK_OFF],
		.block_num = CONFIG_UI_SLAB1_NUM_BLOCKS,
		.block_size = CONFIG_UI_SLAB1_BLOCK_SIZE,
	},

	{
		.slab = &ui_mem_slab[2],
		.slab_base = &ui_mem_slab_buffer[SLAB2_BLOCK_OFF],
		.block_num = CONFIG_UI_SLAB2_NUM_BLOCKS,
		.block_size = CONFIG_UI_SLAB2_BLOCK_SIZE,
	},

	{
		.slab = &ui_mem_slab[3],
		.slab_base = &ui_mem_slab_buffer[SLAB3_BLOCK_OFF],
		.block_num = CONFIG_UI_SLAB3_NUM_BLOCKS,
		.block_size = CONFIG_UI_SLAB3_BLOCK_SIZE,
	},

	{
		.slab = &ui_mem_slab[4],
		.slab_base = &ui_mem_slab_buffer[SLAB4_BLOCK_OFF],
		.block_num = CONFIG_UI_SLAB4_NUM_BLOCKS,
		.block_size = CONFIG_UI_SLAB4_BLOCK_SIZE,
	},

	{
		.slab = &ui_mem_slab[5],
		.slab_base = &ui_mem_slab_buffer[SLAB5_BLOCK_OFF],
		.block_num = CONFIG_UI_SLAB5_NUM_BLOCKS,
		.block_size = CONFIG_UI_SLAB5_BLOCK_SIZE,
	},

	{
		.slab = &ui_mem_slab[6],
		.slab_base = &ui_mem_slab_buffer[SLAB6_BLOCK_OFF],
		.block_num = CONFIG_UI_SLAB6_NUM_BLOCKS,
		.block_size = CONFIG_UI_SLAB6_BLOCK_SIZE,
	},

	{
		.slab = &ui_mem_slab[7],
		.slab_base = &ui_mem_slab_buffer[SLAB7_BLOCK_OFF],
		.block_num = CONFIG_UI_SLAB7_NUM_BLOCKS,
		.block_size = CONFIG_UI_SLAB7_BLOCK_SIZE,
	},

	{
		.slab = &ui_mem_slab[8],
		.slab_base = &ui_mem_slab_buffer[SLAB8_BLOCK_OFF],
		.block_num = CONFIG_UI_SLAB8_NUM_BLOCKS,
		.block_size = CONFIG_UI_SLAB8_BLOCK_SIZE,
	},

	{
		.slab = &ui_mem_slab[9],
		.slab_base = &ui_mem_slab_buffer[SLAB9_BLOCK_OFF],
		.block_num = CONFIG_UI_SLAB9_NUM_BLOCKS,
		.block_size = CONFIG_UI_SLAB9_BLOCK_SIZE,
	},

	{
		.slab = &ui_mem_slab[10],
		.slab_base = &ui_mem_slab_buffer[SLAB10_BLOCK_OFF],
		.block_num = CONFIG_UI_SLAB10_NUM_BLOCKS,
		.block_size = CONFIG_UI_SLAB10_BLOCK_SIZE,
	},
};

static const struct slabs_info ui_slab =
{
	.slab_num = CONFIG_UI_SLAB_TOTAL_NUM,
	.max_used = system_max_used,
	.max_size = system_max_size,
	.slab_flag = 0,
	.slabs = slab_info,
};

int ui_memory_init(void)
{
	memset(system_max_used, 0,sizeof(system_max_used));
	memset(system_max_size, 0, sizeof(system_max_size));
	mem_slabs_init((struct slabs_info *)&ui_slab);
	return 0;
}

void *ui_memory_alloc(uint32_t size)
{
	return mem_slabs_malloc((struct slabs_info *)&ui_slab, size);
}

void ui_memory_free(void *ptr)
{
	mem_slabs_free((struct slabs_info *)&ui_slab, ptr);
}

void ui_memory_dump_info(uint32_t index)
{
	mem_slabs_dump((struct slabs_info *)&ui_slab, index);
}
