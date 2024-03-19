/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_GUI_LVGL_MALLOC_H_
#define ZEPHYR_LIB_GUI_LVGL_MALLOC_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_LV_MEM_CUSTOM

void *lvgl_malloc(size_t size);

void *lvgl_realloc(void *ptr, size_t requested_size);

void lvgl_free(void *ptr);

#endif /* CONFIG_LV_MEM_CUSTOM */

void lvgl_heap_dump(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_GUI_LVGL_MALLOC_H_ */
