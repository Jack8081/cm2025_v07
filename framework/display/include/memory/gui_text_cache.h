/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_GUI_LVGL_TEXT_CACHE_H_
#define ZEPHYR_LIB_GUI_LVGL_TEXT_CACHE_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_GUI_TEXT_IMG_CACHE_SIZE > 0

int gui_text_cache_init(void);

void *gui_text_cache_malloc(size_t size);

void gui_text_cache_free(void *ptr);

#else /* CONFIG_GUI_TEXT_IMG_CACHE_SIZE > 0 */

static inline int gui_text_cache_init(void)
{
	return 0;
}

static inline void *gui_text_cache_malloc(size_t size)
{
	return NULL;
}

static inline void gui_text_cache_free(void *ptr)
{
}

#endif /* CONFIG_GUI_TEXT_IMG_CACHE_SIZE > 0 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_GUI_LVGL_TEXT_CACHE_H_ */
