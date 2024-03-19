/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief display surface interface
 */

#ifndef ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_UI_SURFACE_H_
#define ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_UI_SURFACE_H_

#include <os_common_api.h>
#include "graphic_buffer.h"

/**
 * @defgroup ui_surface_apis UI Surface APIs
 * @ingroup system_apis
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum surface_event_id
 * @brief Enumeration with possible surface event
 *
 */
enum surface_event_id {
	/* ready to accept next draw, mainly for SURFACE_SWAP_ZERO_BUF */
	SURFACE_EVT_DRAW_READY = 0,
	/* check if current frame drawn dirty regions fully covers an area,
	 * required to optimize the swapbuf copy in double buffer case.
	 */
	SURFACE_EVT_DRAW_COVER_CHECK,

	SURFACE_EVT_POST_START,
};

/**
 * @enum surface_callback_id
 * @brief Enumeration with possible surface callback
 *
 */
enum surface_callback_id {
	SURFACE_CB_DRAW = 0,
	SURFACE_CB_POST,

	SURFACE_NUM_CBS,
};

enum surface_swap_mode {
	SURFACE_SWAP_DEFAULT = 0,
	SURFACE_SWAP_SINGLE_BUF, /* always use single buffer */
	SURFACE_SWAP_ZERO_BUF, /* always use zero buffer (direct post) */
};

enum surface_create_flags {
	/* surface post in sync mode. */
	SURFACE_POST_IN_SYNC_MODE = 0x1,
};

/**
 * @brief Enumeration with possible surface flags
 *
 */
enum {
	SURFACE_FIRST_DRAW = 0x01, /* first draw in one frame */
	SURFACE_LAST_DRAW  = 0x02, /* last draw in one frame */
};

/**
 * @struct surface_post_data
 * @brief Structure holding surface event data in callback SURFACE_CB_POST
 *
 */
typedef struct surface_post_data {
	uint8_t flags;
	const ui_region_t *area;
} surface_post_data_t;

/**
 * @struct surface_cover_check_data
 * @brief Structure holding surface event data in callback SURFACE_EVT_DRAW_COVER_CHECK
 *
 */
typedef struct surface_cover_check_data {
	const ui_region_t *area;
	bool covered; /* store the cover result with 'area' */
} surface_cover_check_data_t;

/**
 * @typedef surface_callback_t
 * @brief Callback API executed when surface changed
 *
 */
typedef void (*surface_callback_t)(uint32_t event, void *data, void *user_data);

/**
 * @struct surface
 * @brief Structure holding surface
 *
 */
typedef struct surface {
	graphic_buffer_t *buffers[CONFIG_SURFACE_MAX_BUFFER_COUNT];
	uint8_t buf_count;
	uint8_t back_idx;
	uint8_t front_idx;

	uint8_t swap_mode : 2;
	uint8_t post_sync : 1;

	uint16_t width;
	uint16_t height;
	uint32_t pixel_format;

	surface_callback_t callback[SURFACE_NUM_CBS];
	void *user_data[SURFACE_NUM_CBS];

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	ui_region_t dirty_area;
#endif

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 1
	uint8_t swap_pending;
#endif

	/* Take care of the last draw of the frame, since surface_end_draw()
	 * may be not called in the same thread as begin_draw()
	 */
	uint8_t in_drawing;
	uint8_t draw_flags;
	atomic_t post_pending_cnt;
	os_sem complete_sem;

	/* reference count (considering alloc/free and post pending count) */
	atomic_t refcount;
} surface_t;

/**
 * @brief Create surface
 *
 * @param w width in pixels
 * @param h height in pixels
 * @param pixel_format display pixel format, see enum display_pixel_format
 * @param buf_count number of buffers to allocate
 * @param swap_mode surface swap mode
 * @param flags surface create flags
 *
 * @return pointer to surface structure; NULL if failed.
 */
surface_t *surface_create(uint16_t w, uint16_t h,
		uint32_t pixel_format, uint8_t buf_count, uint8_t swap_mode, uint8_t flags);

/**
 * @brief Destroy surface
 *
 * @param surface pointer to surface structure
 *
 * @retval N/A.
 */
void surface_destroy(surface_t *surface);

/**
 * @brief surface begin a new frame to draw.
 *
 * Must call when current frame dirty regions are computed, and before
 * any surface_begin_draw().
 *
 * Must call swap mode is not SURFACE_SWAP_ZERO_BUF.
 *
 * @param surface pointer to surface structure
 *
 * @return N/A.
 */
void surface_begin_frame(surface_t *surface);

/**
 * @brief surface end current frame to draw
 *
 * @param surface pointer to surface structure
 *
 * @return N/A.
 */
void surface_end_frame(surface_t *surface);

/**
 * @brief surface begin to draw the current frame
 *
 * @param surface pointer to surface structure
 * @param flags draw flags
 * @param drawbuf store the pointer of draw buffer, will be NULL for SURFACE_SWAP_ZERO_BUF.
 *
 * @retval 0 on success else negative error code.
 */
int surface_begin_draw(surface_t *surface, uint8_t flags, graphic_buffer_t **drawbuf);

/**
 * @brief surface end to draw the current frame
 *
 * @param surface pointer to surface structure
 * @param area updated surface area
 * @param buf updated pixel data, must not be NULL for SURFACE_SWAP_ZERO_BUF
 *
 * @return N/A.
 */
void surface_end_draw(surface_t *surface, const ui_region_t *area, const void *buf);

/**
 * @brief Get pixel format of surface
 *
 * @param surface pointer to surface structure
 *
 * @return pixel format.
 */
static inline uint32_t surface_get_pixel_format(surface_t *surface)
{
	return surface->pixel_format;
}

/**
 * @brief Get width of surface
 *
 * @param surface pointer to surface structure
 *
 * @return width in pixels.
 */
static inline uint16_t surface_get_width(surface_t *surface)
{
	return surface->width;
}

/**
 * @brief Get height of surface
 *
 * @param surface pointer to surface structure
 *
 * @return height in pixels.
 */
static inline uint16_t surface_get_height(surface_t *surface)
{
	return surface->height;
}

/**
 * @brief Get stride of surface
 *
 * @param surface pointer to surface structure
 *
 * @return stride in pixels.
 */
static inline uint32_t surface_get_stride(surface_t *surface)
{
	return surface->buffers[0] ? graphic_buffer_get_stride(surface->buffers[0]) : surface->width;
}

/**
 * @brief Register surface callback
 *
 * @param surface pointer to surface structure
 * @param callback event callback
 * @param user_data user data passed in callback function
 *
 * @return N/A.
 */
static inline void surface_register_callback(surface_t *surface,
		int callback_id, surface_callback_t callback_fn, void *user_data)
{
	surface->callback[callback_id] = callback_fn;
	surface->user_data[callback_id] = user_data;
}

/**
* @cond INTERNAL_HIDDEN
*/

/**
 * @brief surface get buffer swap mode
 *
 * @param surface pointer to surface structure
 *
 * @retval surface swap mode.
 */
static inline uint8_t surface_get_swap_mode(surface_t *surface)
{
	return surface->swap_mode;
}

/**
 * @brief surface set buffer swap mode
 *
 * @param surface pointer to surface structure
 * @param mode swap mode
 *
 * @retval 0 on success else negative error code.
 */
int surface_set_swap_mode(surface_t *surface, uint8_t mode);

/**
 * @brief surface has posted one draw area
 *
 * @param surface pointer to surface structure
 *
 * @return N/A.
 */
void surface_complete_one_post(surface_t *surface);

/**
 * @brief Get allocated buffer count of surface
 *
 * @param surface pointer to surface structure
 *
 * @return buffer count of surface.
 */
static inline uint8_t surface_get_buffer_count(surface_t *surface)
{
	return surface->buf_count;
}

/**
 * @brief Set minumum buffer count of surface
 *
 * It will check current buffer count, if it is less than min_count,
 * buffers will be allocated.
 *
 * Must be called in the draw thread context.
 *
 * @param surface pointer to surface structure
 * @param min_count min buffer count
 *
 * @retval 0 on success else negative code.
 */
int surface_set_min_buffer_count(surface_t *surface, uint8_t min_count);

/**
 * @brief Set maximum buffer count of surface
 *
 * It will check current buffer count, if it is less than min_count,
 * buffers will be allocated.
 *
 * Must be called in the draw thread context.
 *
 * @param surface pointer to surface structure
 * @param max_count max buffer count
 *
 * @retval 0 on success else negative code.
 */
int surface_set_max_buffer_count(surface_t *surface, uint8_t max_count);

/**
 * @brief Get back/draw buffer of surface
 *
 * @param surface pointer to surface structure
 *
 * @return pointer of graphic buffer structure; NULL if failed.
 */
graphic_buffer_t *surface_get_backbuffer(surface_t *surface);

/**
 * @brief Get front/post buffer of surface
 *
 * @param surface pointer to surface structure
 *
 * @return pointer of graphic buffer structure; NULL if failed.
 */
graphic_buffer_t *surface_get_frontbuffer(surface_t *surface);

/**
* INTERNAL_HIDDEN @endcond
*/

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_UI_SURFACE_H_ */
