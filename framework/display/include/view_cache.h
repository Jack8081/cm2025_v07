/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file view cache interface
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_VIEW_CACHE_H_
#define FRAMEWORK_DISPLAY_INCLUDE_VIEW_CACHE_H_

#include <stdint.h>

/**
 * @defgroup view_cache_apis View Cache APIs
 * @ingroup system_apis
 * @{
 */

enum VICE_CACHE_SLIDE_TYPE {
	LANDSCAPE,
	PORTRAIT,
};

enum VICE_CACHE_EVENT_TYPE {
	/* For serial load only */
	VIEW_CACHE_EVT_LOAD_BEGIN,
	VIEW_CACHE_EVT_LOAD_END,
	VIEW_CACHE_EVT_LOAD_CANCEL,
};

/**
 * @typedef view_cache_focus_cb_t
 * @brief Callback to notify view focus changed
 */
typedef void (*view_cache_focus_cb_t)(uint8_t view_id, bool focus);

/**
 * @typedef view_cache_event_cb_t
 * @brief Callback to notify view cache event
 */
typedef void (*view_cache_event_cb_t)(uint8_t event);

/**
 * @struct view_cache_dsc
 * @brief Structure to describe view cache
 */
typedef struct view_cache_dsc {
	uint8_t type; /* enum SLIDE_VIEW_TYPE */
	uint8_t rotate : 1; /* rotate sliding mode, only when 'num' >= 3 */
	uint8_t serial_load : 1; /* serial loading the views */
	/* limit cross view sliding only when init view passed in view_cache_init() focused */
	uint8_t cross_only_init_focused : 1;
	uint8_t num;  /* number of views in vlist */
	const uint8_t *vlist; /* main view list */
	const void **plist; /* main view presenter list */

	/* cross view list and their presenter list.
	 * set -1 to the corresponding index if the view does not exist.
	 *
	 * For LANDSCAPE, [0] is the UP view, [1] is the DOWN view
	 * For PORTRAIT, [0] is the LEFT view, [1] is the RIGHT view
	 */
	int16_t cross_vlist[2];
	const void *cross_plist[2];

	/* view focus changed callback */
	view_cache_focus_cb_t focus_cb;
	/* event callback */
	view_cache_event_cb_t event_cb;
} view_cache_dsc_t;

/**
 * @brief Initialize the view cache
 *
 * @param dsc pointer to an initialized structure view_cache_dsc
 * @param init_view initial focused view in vlist
 *
 * @retval 0 on success else negative code.
 */
int view_cache_init(const view_cache_dsc_t *dsc, uint8_t init_view);

/**
 * @brief Deinitialize the view cache
 *
 * @retval 0 on success else negative code.
 */
void view_cache_deinit(void);

/**
 * @brief Get focused view id
 *
 * @retval id of focused view on success else negative code.
 */
int16_t view_cache_get_focus_view(void);

/**
 * @brief Set focus to view
 *
 * Must not called during view sliding or when cross views are focused
 *
 * @view_id id of focus view, must in the vlist of view cache
 *
 * @retval 0 on success else negative code.
 */
int view_cache_set_focus_view(uint8_t view_id);

/**
 * @brief Dump the view cache
 *
 * @retval 0 on success else negative code.
 */
void view_cache_dump(void);

/**
 * @} end defgroup system_apis
 */
#endif /* FRAMEWORK_DISPLAY_INCLUDE_VIEW_CACHE_H_ */
