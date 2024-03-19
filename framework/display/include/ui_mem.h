/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ui memory interface 
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_UI_MEM_H_
#define FRAMEWORK_DISPLAY_INCLUDE_UI_MEM_H_

/**
 * @defgroup view_cache_apis View Cache APIs
 * @ingroup system_apis
 * @{
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the ui surface memory
 *
 * @retval 0 on success else negative code.
 */
int ui_memory_init(void);

/**
 * @brief Alloc ui surface memory
 *
 * @param size allocation size in bytes
 *
 * @retval pointer to the allocation memory.
 */
void *ui_memory_alloc(uint32_t size);

/**
 * @brief Free ui surface memory
 *
 * @param ptr pointer to the allocated memory
 *
 * @retval N/A
 */
void ui_memory_free(void *ptr);

/**
 * @brief Dump ui surface memory allocation detail.
 *
 * @param index the slab index, ignored at present.
 *
 * @retval N/A
 */
void ui_memory_dump_info(uint32_t index);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* FRAMEWORK_DISPLAY_INCLUDE_UI_MEM_H_ */


