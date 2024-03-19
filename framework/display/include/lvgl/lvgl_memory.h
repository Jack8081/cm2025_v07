/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief lvgl memory
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_LVGL_MEMORY_H_
#define FRAMEWORK_DISPLAY_INCLUDE_LVGL_MEMORY_H_

/**
 * @defgroup lvgl_memory_apis LVGL Memory APIs
 * @ingroup lvgl_apis
 * @{
 */

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Allocate lvgl display draw buffers.
 *
 * @param disp_drv pointer to lvgl display driver structure.
 *
 * @retval 0 on success else negative code.
 */
int lvgl_allocate_draw_buffers(lv_disp_drv_t *disp_drv);

/**
 * @brief Free lvgl display draw buffers.
 *
 * @param disp_drv pointer to lvgl display driver structure.
 *
 * @retval 0 on success else negative code.
 */
int lvgl_free_draw_buffers(lv_disp_drv_t *disp_drv);

/**
 * @brief Clean cache of lvgl img buf.
 *
 * @param dsc pointer to an lvgl image descriptor.
 *
 * @retval N/A.
 */
void lvgl_img_buf_clean_cache(const lv_img_dsc_t *dsc);

/**
 * @brief Invalidate cache of lvgl img buf.
 *
 * @param dsc pointer to an lvgl image descriptor.
 *
 * @retval N/A.
 */
void lvgl_img_buf_invalidate_cache(const lv_img_dsc_t *dsc);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* FRAMEWORK_DISPLAY_INCLUDE_LVGL_MEMORY_H_ */
