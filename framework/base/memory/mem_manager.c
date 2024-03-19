/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/**
 * @file
 * @brief mem manager.
*/

#include <mem_manager.h>
#include "mem_inner.h"

void * app_mem_malloc(unsigned int num_bytes)
{
#ifdef CONFIG_SIMULATOR
	return malloc(num_bytes);
#elif defined(CONFIG_APP_USED_MEM_SLAB)
#ifdef CONFIG_APP_USED_SYSTEM_SLAB
	return mem_slabs_malloc((struct slabs_info *)&sys_slab, num_bytes);
#else
	return mem_slabs_malloc((struct slabs_info *)&app_slab, num_bytes);
#endif
#elif defined(CONFIG_APP_USED_MEM_PAGE)
	return mem_page_malloc(num_bytes, __builtin_return_address(0));
#elif defined(CONFIG_APP_USED_MEM_POOL)
	return mem_pool_malloc(num_bytes);
#endif
}

void * app_mem_malloc_ext(unsigned int num_bytes, int malloc_tag)
{
#ifdef CONFIG_SIMULATOR
	return malloc(num_bytes);
#elif defined(CONFIG_APP_USED_MEM_SLAB)
#ifdef CONFIG_APP_USED_SYSTEM_SLAB
	return mem_slabs_malloc((struct slabs_info *)&sys_slab, num_bytes);
#else
	return mem_slabs_malloc((struct slabs_info *)&app_slab, num_bytes);
#endif
#elif defined(CONFIG_APP_USED_MEM_PAGE)
	return mem_page_malloc(num_bytes, (void *)malloc_tag);
#elif defined(CONFIG_APP_USED_MEM_POOL)
	return mem_pool_malloc(num_bytes);
#endif
}

void app_mem_free(void *ptr)
{
	if (ptr == NULL)
		return;

#ifdef CONFIG_SIMULATOR
	free(ptr);
#elif defined(CONFIG_APP_USED_MEM_SLAB)
#ifdef CONFIG_APP_USED_SYSTEM_SLAB
	mem_slabs_free((struct slabs_info *)&sys_slab, ptr);
#else
	mem_slabs_free((struct slabs_info *)&app_slab, ptr);
#endif
#elif defined(CONFIG_APP_USED_MEM_PAGE)
	mem_page_free(ptr, __builtin_return_address(0));
#elif defined(CONFIG_APP_USED_MEM_POOL)
	mem_pool_free(ptr);
#endif
}

void *mem_malloc_debug(unsigned int num_bytes, void *caller)
{
#ifdef CONFIG_SIMULATOR
	return malloc(num_bytes);
#elif defined(CONFIG_APP_USED_MEM_SLAB)
	return mem_slabs_malloc((struct slabs_info *)&sys_slab, num_bytes);
#elif defined(CONFIG_APP_USED_MEM_PAGE)
	return mem_page_malloc(num_bytes,caller);
#elif defined(CONFIG_APP_USED_MEM_POOL)
	return mem_pool_malloc(num_bytes);
#endif
}

void *mem_malloc(unsigned int num_bytes)
{
#ifdef CONFIG_SIMULATOR
	return malloc(num_bytes);
#else
	return mem_malloc_debug(num_bytes, __builtin_return_address(0));
#endif
}

void mem_free(void *ptr)
{
	if (ptr == NULL)
		return;

#ifdef CONFIG_SIMULATOR
	free(ptr);
#elif defined(CONFIG_APP_USED_MEM_SLAB)
	mem_slabs_free((struct slabs_info *)&sys_slab, ptr);
#elif defined(CONFIG_APP_USED_MEM_PAGE)
	mem_page_free(ptr, __builtin_return_address(0));
#elif defined(CONFIG_APP_USED_MEM_POOL)
	mem_pool_free(ptr);
#endif
}

void mem_manager_dump(void)
{
#ifdef CONFIG_SIMULATOR
	/* nothing to do */
#elif defined(CONFIG_APP_USED_MEM_SLAB)
	mem_slabs_dump((struct slabs_info *)&sys_slab, 0);
#elif defined(CONFIG_APP_USED_MEM_PAGE)
	mem_page_dump(1, NULL);
#elif defined(CONFIG_APP_USED_MEM_POOL)
	//mem_pool_dump();
#endif
}

void mem_manager_dump_ext(int dump_detail, const char* match_value)
{
#ifdef CONFIG_APP_USED_MEM_PAGE
	mem_page_dump(dump_detail, match_value);
#endif
}

int mem_manager_init(void)
{
#ifdef CONFIG_APP_USED_MEM_SLAB
	mem_slab_init(void);
#elif defined(CONFIG_APP_USED_MEM_PAGE)
	mem_page_init();
#endif

	return 0;
}

