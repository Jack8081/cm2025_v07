/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <mem_manager.h>
#include <kernel.h>
#include <kernel_structs.h>
#include <init.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <string.h>

#include "ftstdlib.h"

#define FT_MEM_POOL_MAXSIZE_BLOCK_NUMER 400
#define FT_MEM_POOL_MAX_BLOCK_SIZE      1024
#define FT_MEM_POOL_MIN_BLOCK_SIZE      16
#define FT_MEM_POOL_ALIGN_SIZE         8

//K_HEAP_DEFINE(freetype_mem_pool, FT_MEM_POOL_MAXSIZE_BLOCK_NUMER * FT_MEM_POOL_MAX_BLOCK_SIZE);
static char __aligned(4) ft_mem_pool_buffer[FT_MEM_POOL_MAXSIZE_BLOCK_NUMER * FT_MEM_POOL_MAX_BLOCK_SIZE] __in_section_unique(RES_PSRAM_REGION);

Z_STRUCT_SECTION_ITERABLE(k_heap, freetype_mem_pool) = {
				.heap = {
					.init_mem = ft_mem_pool_buffer,
					.init_bytes = FT_MEM_POOL_MAXSIZE_BLOCK_NUMER * FT_MEM_POOL_MAX_BLOCK_SIZE,					
					},
};

typedef struct _mem_info
{
	void* ptr;
	size_t size;
	struct _mem_info* next;
}mem_info_t;


static mem_info_t* mem_info_list = NULL;
static size_t mem_total;

void _ft_add_mem_info(void* ptr, size_t size)
{
	mem_info_t* prev;
	mem_info_t* listp = mem_info_list;
	mem_info_t* item = mem_malloc(sizeof(mem_info_t));
	
	item->ptr = ptr;
	item->size = size;
	item->next = NULL;

	if(listp == NULL)
	{
		mem_info_list = item;
	}
	else
	{
		while(listp != NULL)
		{
			prev = listp;
			listp = listp->next;
		}
		prev->next = item;
	}

	mem_total += size;

}

void _ft_remove_mem_info(void* ptr)
{
	mem_info_t* prev;
	mem_info_t* listp = mem_info_list;
	mem_info_t* item;

	while(listp != NULL)
	{
		if(listp->ptr == ptr)
		{
			break;
		}
		prev = listp;
		listp = listp->next;
	}

	if(listp == NULL)
	{
		printf("testalloc remove mem info not found 0x%x\n", ptr);
		return;
	}

	item = listp;

	if(listp == mem_info_list)
	{
		mem_info_list = listp->next;
		mem_total -= item->size;

//		SYS_LOG_INF("testalloc free == item 0x%x, ptr 0x%x size %d total %d\n", item, item->ptr, item->size, mem_total);
		mem_free(item);
	}
	else
	{
		prev->next = listp->next;
		mem_total -= item->size;

//		SYS_LOG_INF("testalloc free == item 0x%x, ptr 0x%x size %d total %d\n", item, item->ptr, item->size, mem_total);
		mem_free(item);
	}

}

size_t _ft_get_mem_size(void* ptr)
{
	mem_info_t* prev;
	mem_info_t* listp = mem_info_list;
	mem_info_t* item;

	while(listp != NULL)
	{
		if(listp->ptr == ptr)
		{
			break;
		}
		prev = listp;
		listp = listp->next;
	}

	if(listp == NULL)
	{
		printf("testalloc get mem size not found 0x%x \n", ptr);
		return 0;
	}

	return listp->size;
}

void *ft_smalloc(size_t size)
{
	void *ptr;

	if(size % 4 != 0)
	{
		size = (size/4 + 1)*4;
	}
	
	ptr = k_heap_alloc(&freetype_mem_pool, size, K_NO_WAIT);
	if (ptr == NULL) {
		printf("heap alloc failed, size %d\n", size);
		errno = ENOMEM;
	}

	_ft_add_mem_info(ptr, size);
//	printf("testalloc alloc == ptr 0x%x size %d total %d\n", ptr, size, mem_total);
	
	return ptr;
}

void ft_sfree(void *ptr)
{
	_ft_remove_mem_info(ptr);
	k_heap_free(&freetype_mem_pool, ptr);
}

void *ft_srealloc(void *ptr, size_t requested_size)
{
	void *new_ptr;
	size_t copy_size;

	if (ptr == NULL) {
		return ft_smalloc(requested_size);
	}

	if (requested_size == 0) {
		ft_sfree(ptr);
		return NULL;
	}

	new_ptr = ft_smalloc(requested_size);
	if (new_ptr == NULL) {
		return NULL;
	}

	copy_size = _ft_get_mem_size(ptr);
	if (copy_size > requested_size)
	{
		copy_size = requested_size;
	}
	memcpy(new_ptr, ptr, copy_size);
	
	ft_sfree(ptr);

	return new_ptr;
}


void *ft_scalloc(size_t nmemb, size_t size)
{
	void *ptr;
	size_t total = nmemb*size;
/*
	if (size_mul_overflow(nmemb, size, &size)) {
		errno = ENOMEM;
		return NULL;
	}
*/
	printf("calloc total %d\n", total);
	ptr = ft_smalloc(total);
	
	if (ptr != NULL) {
		memset(ptr, 0, total);
	}

	return ptr;
}


