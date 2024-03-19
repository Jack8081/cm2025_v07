/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for display engine drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DISPLAY_DISPLAY_ENGINE_H_
#define ZEPHYR_INCLUDE_DRIVERS_DISPLAY_DISPLAY_ENGINE_H_

#include <kernel.h>
#include <device.h>
#include <zephyr/types.h>
#include <drivers/cfg_drv/dev_config.h>
#include "display_graphics.h"

/**
 * @brief Display Engine Interface
 * @defgroup display_engine_interface Display Engine Interface
 * @ingroup display_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/* open flags of display engine */
#define DISPLAY_ENGINE_FLAG_HIGH_PRIO  BIT(0)
#define DISPLAY_ENGINE_FLAG_POST       BIT(1) /* For display post */

/**
 * @enum display_engine_ctrl_type
 * @brief Enumeration with possible display engine ctrl command
 *
 */
enum display_engine_ctrl_type {
	DISPLAY_ENGINE_CTRL_DISPLAY_PORT = 0, /* configure display video port */
	DISPLAY_ENGINE_CTRL_DISPLAY_MODE,     /* configure display video mode */
	/* prepare display refreshing.
	 * arg1 is the callback function, arg2 must point to the display device structure
	 */
	DISPLAY_ENGINE_CTRL_DISPLAY_PREPARE_CB,
	/* start display refreshing
	 * arg1 is the callback function, arg2 must point to the display device structure
	 */
	DISPLAY_ENGINE_CTRL_DISPLAY_START_CB,
};

/**
 * @struct display_engine_capabilities
 * @brief Structure holding display engine capabilities
 *
 * @var uint8_t display_capabilities::num_overlays
 * Maximum number of overlays supported
 *
 * @var uint8_t display_capabilities::support_blend_fg
 * Blending constant fg color supported
 *
 * @var uint8_t display_capabilities::support_blend_b
 * Blending constant bg color supported
 *
 * @var uint32_t display_capabilities::supported_input_pixel_formats
 * Bitwise or of input pixel formats supported by the display engine
 *
 * @var uint32_t display_capabilities::supported_output_pixel_formats
 * Bitwise or of output pixel formats supported by the display engine
 *
 * @var uint32_t display_capabilities::supported_rotate_pixel_formats
 * Bitwise or of rotation pixel formats supported by the display engine
 *
 */
struct display_engine_capabilities {
	uint8_t num_overlays;
	uint8_t support_blend_fg : 1;
	uint8_t support_blend_bg : 1;
	uint32_t supported_input_pixel_formats;
	uint32_t supported_output_pixel_formats;
	uint32_t supported_rotate_pixel_formats;
};

/**
 * @struct display_engine_rotation
 * @brief Structure holding display engine rotation configuration
 *
 */
typedef struct display_engine_rotation {
	/* dst buffer */
	uint32_t dst_address; /* dst address at (0, row_start) */
	uint16_t dst_pitch;   /* dst bytes per row, must not less than outer_diameter * bytes_per_pixel(pixel_format) */

	/* src buffer */
	uint16_t src_pitch;   /* src bytes per row, must not less than outer_diameter * bytes_per_pixel(pixel_format) */
	uint32_t src_address; /* src address mapping to dest address at (0, row_start) */

	/* diplay pixel format both of src and dest */
	uint32_t pixel_format;

	/* line range [line_start, line_end) to do rotation */
	uint16_t line_start;
	uint16_t line_end;

	/* fill the inner hole with color  */
	display_color_t fill_color;
	uint16_t fill_enable : 1;

	/* 
	 * 1) the center both of the outer and inner circle is (outer_diameter/2, outer_diameter/2)
	 * 2) only the ring area decided by outer_radius_sq and inner_radius_sq will be rotated.
	 * 3) the pixles in the square area "outer_diameter x outer_diameter" but not inside the
	 *    ring area will be filled with fill_color if fill_enable set.
	 * 4) set inner_radius_sq to 0 and outer_radius_sq to (outer_diameter/2)^2 to
	 *    rotate the whole circular image area.
	 */
	/* width in pixels both of the src and dest, which decide a square area contains the circular image content */
	uint16_t outer_diameter;  /* outer circle diameter in pixels */
	uint32_t outer_radius_sq; /* outer circle radius square in .2 fixedpoint format */
	uint32_t inner_radius_sq; /* inner circle radius square in .2 fixedpoint format */

	/* assistant variables */
	uint32_t dst_dist_sq;   /* distance square between (0, row_start) and center (width/2, width/2) in .2 fixedpoint */
	int32_t src_coord_x;     /* src X coord in .12 fixedpoint mapping to dest coord (0, row_start) */
	int32_t src_coord_y;     /* src Y coord in .12 fixedpoint mapping to dest coord (0, row_start) */
	int32_t src_coord_dx_ax; /* src X coord diff in .12 fixedpoint along the dest X-axis */
	int32_t src_coord_dy_ax; /* src Y coord diff in .12 fixedpoint along the dest X-axis */
	int32_t src_coord_dx_ay; /* src X coord diff in .12 fixedpoint along the dest Y-axis */
	int32_t src_coord_dy_ay; /* src Y coord diff in .12 fixedpoint along the dest Y-axis */
} display_engine_rotation_t;

/**
 * @typedef display_engine_prepare_display_t
 * @brief Callback API executed when display engine prepare refreshing the display
 *
 */
typedef void (*display_engine_prepare_display_t)(void *arg, const display_rect_t *area);

/**
 * @typedef display_engine_start_display_t
 * @brief Callback API executed when display engine start refreshing the display
 *
 */
typedef void (*display_engine_start_display_t)(void *arg);


/**
 * @typedef display_engine_instance_callback_t
 * @brief Callback API executed when any display engine instance transfer complete or error
 *
 */
typedef void (*display_engine_instance_callback_t)(int status, uint16_t cmd_seq, void *user_data);

/**
 * @typedef display_engine_control_api
 * @brief Callback API to control display engine device
 * See display_engine_control() for argument description
 */
typedef int (*display_engine_control_api)(const struct device *dev,
		int cmd, void *arg1, void *arg2);

/**
 * @typedef display_engine_open_api
 * @brief Callback API to open display engine
 * See display_engine_open() for argument description
 */
typedef int (*display_engine_open_api)(const struct device *dev, uint32_t flags);

/**
 * @typedef display_engine_close_api
 * @brief Callback API to close display engine
 * See display_engine_close() for argument description
 */
typedef int (*display_engine_close_api)(const struct device *dev, int inst);

/**
 * @typedef display_engine_get_capabilities_api
 * @brief Callback API to get display engine capabilities
 * See display_engine_get_capabilities() for argument description
 */
typedef void (*display_engine_get_capabilities_api)(const struct device *dev,
		struct display_engine_capabilities *capabilities);

/**
 * @typedef display_engine_register_callback_api
 * @brief Callback API to register instance callback
 * See display_engine_register_callback() for argument description
 */
typedef int (*display_engine_register_callback_api)(const struct device *dev,
		int inst, display_engine_instance_callback_t callback, void *user_data);

/**
 * @typedef display_engine_fill_api
 * @brief Callback API to fill color using display engine
 * See display_engine_fill() for argument description
 */
typedef int (*display_engine_fill_api)(const struct device *dev,
		int inst, display_buffer_t *dest, display_color_t color);

/**
 * @typedef display_engine_blit_api
 * @brief Callback API to blit using display engine
 * See display_engine_blit() for argument description
 */
typedef int (*display_engine_blit_api)(const struct device *dev,
		int inst, display_buffer_t *dest,
		display_buffer_t *src, display_color_t src_color);

/**
 * @typedef display_engine_blend_api
 * @brief Callback API to blend using display engine
 * See display_engine_blend() for argument description
 */
typedef int (*display_engine_blend_api)(const struct device *dev,
		int inst, display_buffer_t *dest,
		display_buffer_t *fg, display_color_t fg_color,
		display_buffer_t *bg, display_color_t bg_color);

/**
 * @typedef display_engine_rotate_api
 * @brief Callback API to rotate using display engine
 * See display_engine_rotate() for argument description
 */
typedef int (*display_engine_rotate_api)(const struct device *dev,
		int inst, display_engine_rotation_t *rotation_cfg);

/**
 * @typedef display_engine_compose_api
 * @brief Callback API to compose using display engine
 * See display_engine_compose() for argument description
 */
typedef int (*display_engine_compose_api)(const struct device *dev,
		int inst, display_buffer_t *target,
		const display_layer_t *ovls, int num_ovls, bool wait_vsync);

/**
 * @typedef display_engine_poll_api
 * @brief Callback API to poll complete of display engine
 * See display_engine_poll() for argument description
 */
typedef int (*display_engine_poll_api)(const struct device *dev,
		int inst, int timeout_ms);

/**
 * @brief Display Engine driver API
 * API which a display engine driver should expose
 */
struct display_engine_driver_api {
	display_engine_control_api control;
	display_engine_open_api open;
	display_engine_close_api close;
	display_engine_get_capabilities_api get_capabilities;
	display_engine_register_callback_api register_callback;
	display_engine_fill_api fill;
	display_engine_blit_api blit;
	display_engine_blend_api blend;
	display_engine_rotate_api rotate;
	display_engine_compose_api compose;
	display_engine_poll_api poll;
};

/**
 * @brief Control display engine
 *
 * @param dev Pointer to device structure
 * @param cmd Control command
 * @param arg1 Control command argument 1
 * @param arg2 Control command argument 2
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_engine_control(const struct device *dev, int cmd,
		void *arg1, void *arg2)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	return api->control(dev, cmd, arg1, arg2);
}

/**
 * @brief Open display engine
 *
 * The instance is not thread safe, and must be referred in the same thread.
 *
 * @param dev Pointer to device structure
 * @param flags flags to display engine
 *
 * @retval instance id on success else negative errno code.
 */
static inline int display_engine_open(const struct device *dev, uint32_t flags)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	return api->open(dev, flags);
}

/**
 * @brief Close display engine
 *
 * The caller must make sure all the commands belong to the inst have completed before close.
 *
 * @param dev Pointer to device structure
 * @param inst instance id return in open()
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_engine_close(const struct device *dev, int inst)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	return api->close(dev, inst);
}

/**
 * @brief Get display engine capabilities
 *
 * @param dev Pointer to device structure
 * @param capabilities Pointer to capabilities structure to populate
 */
static inline void display_engine_get_capabilities(const struct device *dev,
		struct display_engine_capabilities *capabilities)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	api->get_capabilities(dev, capabilities);
}

/**
 * @brief Register display engine instance callback
 *
 * This procedure will only succeed when display engine instance is not busy, and the
 * registered callback may be called in isr.
 *
 * @param dev Pointer to device structure
 * @param inst Instance id returned in open()
 * @param callback callback function
 * @param user_data callback parameter "user_data"
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_engine_register_callback(const struct device *dev,
		int inst, display_engine_instance_callback_t callback, void *user_data)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	return api->register_callback(dev, inst, callback, user_data);
}

/**
 * @brief Fill color
 *
 * @param dev Pointer to device structure
 * @param inst Instance id returned in open()
 * @param dest Pointer to the filling display framebuffer
 * @param color The filling color
 *
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
static inline int display_engine_fill(const struct device *dev,
		int inst, display_buffer_t *dest, display_color_t color)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	return api->fill(dev, inst, dest, color);
}

/**
 * @brief Blit source fb to destination fb
 *
 * @param dev Pointer to device structure
 * @param inst Instance id returned in open()
 * @param dest Pointer to the destination fb
 * @param src Pointer to the source fb
 * @param src_color Source color as color rgb values of pixel format A8 (A4/1 not allowed)
 *
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
static inline int display_engine_blit(const struct device *dev,
		int inst, display_buffer_t *dest,
		display_buffer_t *src, display_color_t src_color)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	return api->blit(dev, inst, dest, src, src_color);
}

/**
 * @brief Blending source fb over destination fb
 *
 * @param dev Pointer to device structure
 * @param inst Instance id returned in open()
 * @param dest Pointer to the destination fb
 * @param fg Pointer to the foreground fb, can be NULL
 * @param fg_color Foreground color
         1) used as fg plane color, when fg is NULL
         2) used as fg pixel rgb color component, when fg format is A8 (A4/1 not allowed)
         3) used as fg plane alpha, when fg format is not A8
 * @param bg Pointer to the background fb
 * @param bg Background color
         1) used as bg plane color, when fg is NULL
         2) used as bg pixel rgb color component, when fg format is A8 (A4/1 not allowed)
         3) used as bg plane alpha, when fg format is not A8
 * @param plane_alpha plane alpha applied to blending
 *
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
static inline int display_engine_blend(const struct device *dev,
		int inst, display_buffer_t *dest,
		display_buffer_t *fg, display_color_t fg_color,
		display_buffer_t *bg, display_color_t bg_color)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	return api->blend(dev, inst, dest, fg, fg_color, bg, bg_color);
}

/**
 * @brief Rotate image to a framebuffer
 *
 * @param dev Pointer to device structure
 * @param inst Instance id returned in open()
 * @param rotation_cfg Pointer to de_rotation_cfg structure
 *
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
static inline int display_engine_rotate(const struct device *dev,
		int inst, display_engine_rotation_t *rotation_cfg)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	if (api->rotate)
		return api->rotate(dev, inst, rotation_cfg);

	return -ENOSYS;
}

/**
 * @brief Composing layers to diplay device or target framebuffer
 *
 * @param dev Pointer to device structure
 * @param inst Instance id returned in open()
 * @param target Target buffer, set NULL if output to display device
 * @param ovls Array of overlays
 * @param num_ovls Number of overlays
 * @param wait_vsync Whether to delay the compostion until vsync arrive
 *
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
static inline int display_engine_compose(const struct device *dev,
		int inst, display_buffer_t *target,
		const display_layer_t *ovls, int num_ovls, bool wait_vsync)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	return api->compose(dev, inst, target, ovls, num_ovls, wait_vsync);
}

/**
 * @brief Polling display engine complete
 *
 * @param dev Pointer to device structure
 * @param inst Instance id returned in open()
 * @param timeout_ms Timeout duration in milliseconds
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_engine_poll(const struct device *dev,
		int inst, int timeout_ms)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	return api->poll(dev, inst, timeout_ms);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_DISPLAY_DISPLAY_ENGINE_H_ */
