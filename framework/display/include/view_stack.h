/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file view stack interface
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_VIEW_STACK_H_
#define FRAMEWORK_DISPLAY_INCLUDE_VIEW_STACK_H_

#include <stdint.h>
#include <stdbool.h>
#include "view_cache.h"

/**
 * @defgroup view_stack_apis View Stack APIs
 * @ingroup system_apis
 * @{
 */

/**
 * @typedef view_group_focus_cb_t
 * @brief Callback to notify view focus changed
 */
typedef void (*view_group_focus_cb_t)(uint8_t view_id, bool focus);

/**
 * @struct view_group_dsc
 * @brief Structure to describe view group layout
 */
typedef struct view_group_dsc {
	const uint8_t * vlist; /* view id list */
	const void ** plist; /* view presenter list */
	uint8_t num; /* number of view */
	uint8_t idx; /* index of focused view */

	/* view focus changed callback */
	view_group_focus_cb_t focus_cb;
} view_group_dsc_t;

/**
 * @brief Initialize the view stack
 *
 * @retval 0 on success else negative code.
 */
int view_stack_init(void);

/**
 * @brief Deinitialize the view stack
 *
 * @retval 0 on success else negative code.
 */
void view_stack_deinit(void);

/**
 * @brief Get the number of elements of the view stack
 *
 * @retval view id on success else negative code.
 */
uint8_t view_stack_get_num(void);

/**
 * @brief Query whether view stack is empty
 *
 * @retval query result.
 */
static inline bool view_stack_is_empty(void)
{
	return view_stack_get_num() == 0;
}

/**
 * @brief Get the top (focused) view id of the view stack
 *
 * @retval view id on success else negative code.
 */
int16_t view_stack_get_top(void);

/**
 * @brief Clean the view stack.
 *
 * @retval 0 on success else negative code.
 */
void view_stack_clean(void);

/**
 * @brief Pop all elements except the first from the view stack
 *
 * @retval 0 on success else negative code.
 */
int view_stack_pop_until_first(void);

/**
 * @brief Pop one element except the first from the view stack
 *
 * @retval 0 on success else negative code.
 */
int view_stack_pop(void);

/**
 * @brief Push view cache to the view stack
 *
 * @param dsc pointer to an initialized structure view_cache_dsc
 * @param view_id initial focused view id, must in the vlist of dsc
 *
 * @retval 0 on success else negative code.
 */
int view_stack_push_cache(const view_cache_dsc_t *dsc, uint8_t view_id);

/**
 * @brief Push view group to the view stack
 *
 * @param dsc pointer to an initialized structure view_group_dsc
 *
 * @retval 0 on success else negative code.
 */
int view_stack_push_group(const view_group_dsc_t *dsc);

/**
 * @brief Push view to the view stack
 *
 * @param view_id id of view
 * @param presenter presenter of view
 *
 * @retval 0 on success else negative code.
 */
int view_stack_push_view(uint8_t view_id, const void *presenter);

/**
 * @brief Dump the view stack
 *
 * @retval N/A.
 */
void view_stack_dump(void);

/**
 * @brief View group change focused view index
 *
 * @retval N/A.
 */
static inline void view_group_set_focus_idx(view_group_dsc_t *dsc, uint8_t idx)
{
	dsc->idx = idx;
}

/**
 * @} end defgroup system_apis
 */
#endif /* FRAMEWORK_DISPLAY_INCLUDE_VIEW_STACK_H_ */
