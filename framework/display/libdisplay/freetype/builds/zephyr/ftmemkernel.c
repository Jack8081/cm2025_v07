/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>
#include <sys/mempool.h>

#include "ftstdlib.h"

#ifdef CONFIG_FREETYPE_MEM_POOL_KERNEL
K_MEM_POOL_DEFINE(freetype_mem_pool,
		CONFIG_FREETYPE_MEM_POOL_MIN_SIZE,
		CONFIG_FREETYPE_MEM_POOL_MAX_SIZE,
		CONFIG_FREETYPE_MEM_POOL_NUMBER_BLOCKS, 4);

void *ft_smalloc(size_t size)
{
	/* FIXME: workaround for realloc */
	void *ptr = k_mem_pool_malloc(&freetype_mem_pool, size + 4);

	if (ptr) {
		*(uint32_t *)ptr = size;
		ptr = (uint32_t *)ptr + 1;
	}

	return ptr;
}
#else
void *ft_smalloc(size_t size)
{
	/* FIXME: workaround for realloc */
	void *ptr = k_malloc(size + 4);

	if (ptr) {
		*(uint32_t *)ptr = size;
		ptr = (uint32_t *)ptr + 1;
	}

	return ptr;
}
#endif /* CONFIG_FREETYPE_MEM_POOL_KERNEL */

void ft_sfree(void *ptr)
{
	/* FIXME: workaround for realloc */
	ptr = (uint32_t *)ptr - 1;
	k_free(ptr);
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

	copy_size = *((uint32_t *)ptr - 1);
	if (copy_size > requested_size)
		copy_size = requested_size;

	memcpy(new_ptr, ptr, copy_size);
	ft_sfree(ptr);

	return new_ptr;
}

void *ft_scalloc(size_t nmemb, size_t size)
{
	void *ptr;

	if (size_mul_overflow(nmemb, size, &size)) {
		errno = ENOMEM;
		return NULL;
	}

	ptr = ft_smalloc(size);

	if (ptr != NULL) {
		memset(ptr, 0, size);
	}

	return ptr;
}
