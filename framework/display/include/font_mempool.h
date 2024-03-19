/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file font memory interface 
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_FONT_MEMPOOL_H_
#define FRAMEWORK_DISPLAY_INCLUDE_FONT_MEMPOOL_H_

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
 * @brief Initialize the bitmap font pool memory
 *
 * @retval 0 on success else negative code.
 */
uint8_t* bitmap_font_get_cache_buffer(void);


/**
 * @brief get max fonts num
 */
uint32_t bitmap_font_get_max_fonts_num(void);

/**
 * @brief get font cache size
 *
 * @retval N/A
 */
uint32_t bitmap_font_get_font_cache_size(void);

/**
 * @brief get max emoji num
 *
 *
 * @retval N/A
 */
uint32_t bitmap_font_get_max_emoji_num(void);


uint32_t bitmap_font_get_cmap_cache_size(void);

/**
 * @brief dump cache size info
 *
 */
void bitmap_font_cache_info_dump(void);

/**
 * @brief get high freq config status
 *
 */
bool bitmap_font_get_high_freq_enabled(void);


#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* FRAMEWORK_DISPLAY_INCLUDE_FONT_MEMPOOL_H_ */


