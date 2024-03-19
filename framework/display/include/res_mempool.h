/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ui memory interface 
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_RES_MEMPOOL_H_
#define FRAMEWORK_DISPLAY_INCLUDE_RES_MEMPOOL_H_

/**
 * @defgroup view_cache_apis View Cache APIs
 * @ingroup system_apis
 * @{
 */
	
#ifndef CONFIG_SIMULATOR
#include <fs/fs.h>
#endif

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef off_t
#define off_t	uint32_t
#define ssize_t int32_t
#endif


/**
 * @brief Initialize the res manager pool memory
 *
 * @retval 0 on success else negative code.
 */
void res_mem_init(void);

/**
 * @brief Alloc res buffer memory
 *
 * @param type mempool type
 * @param size allocation size in bytes
 *
 * @retval pointer to the allocation memory.
 */
void *res_mem_alloc(uint32_t type, size_t size);

/**
 * @brief Free res buffer memory
 *
 * @param type mempool type 
 * @param ptr pointer to the allocated memory
 *
 * @retval N/A
 */
void res_mem_free(uint32_t type, void *ptr);

/**
 * @brief Dump res manger memory allocation detail.
 *
 *
 * @retval N/A
 */
void res_mem_dump(void);

/**
 * @brief Alloc fixed size res buffer memory
 *
 * @param type mem type
 * @param size allocation size in bytes
 *
 * @retval pointer to the allocation memory.
 */
void* res_array_alloc(int32_t type, size_t size);

/**
 * @brief Free fixed size res buffer memory
 *
 * @param type mem type 
 * @param ptr pointer to the allocated memory
 *
 * @retval 1 on success else ptr is not found
 */
uint32_t res_array_free(void* ptr);

/**
 * @brief check if auto searching enabled
 *
 *
 * @retval N/A
 */
int res_is_auto_search_files(void);

int res_fs_open(void* handle, const char* path);
void res_fs_close(void* handle);
int res_fs_seek(void* handle, off_t where, int whence);
off_t res_fs_tell(void* handle);
ssize_t res_fs_read(void* handle, void* buffer, size_t len);


#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* FRAMEWORK_DISPLAY_INCLUDE_RES_MEMPOOL_H_ */


