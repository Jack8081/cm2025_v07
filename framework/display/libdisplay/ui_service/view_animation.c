/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file view manager interface
 */

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "view_manager"
#include <os_common_api.h>
#include <ui_math.h>
#include <string.h>
#ifdef CONFIG_DMA2D_HAL
#  include <dma2d_hal.h>
#endif
#include "ui_service_inner.h"
#include "view_manager_inner.h"

#ifndef CONFIG_SIMULATOR
__attribute__((__unused__))
#endif
static ui_point_t _animation_update_position_linear(ui_view_animation_t *animation)
{
	ui_point_t pos = animation->cfg.start;
	int32_t ratio = ui_map(animation->elapsed, 0, animation->cfg.duration, 0, UI_VIEW_ANIM_RANGE);

	if (animation->cfg.path_cb) {
		ratio = animation->cfg.path_cb(ratio);
	}

	pos.x += (ratio * (animation->cfg.stop.x - animation->cfg.start.x)) >> 10;
	pos.y += (ratio * (animation->cfg.stop.y - animation->cfg.start.y)) >> 10;

	return pos;
}

static ui_point_t _animation_update_position_cos(ui_view_animation_t *animation)
{
	ui_point_t pos = animation->cfg.start;
	uint32_t angle = ui_map(animation->elapsed, 0, animation->cfg.duration, 900, 1800);
	int32_t cos_value = -sw_cos30(angle) / UI_VIEW_ANIM_RANGE;
	int32_t ratio = ui_map(cos_value, 0, -sw_cos30(1800) / UI_VIEW_ANIM_RANGE, 0, UI_VIEW_ANIM_RANGE);

	if (animation->cfg.path_cb) {
		ratio = animation->cfg.path_cb(ratio);
	}

	pos.x += (ratio * (animation->cfg.stop.x - animation->cfg.start.x)) >> 10;
	pos.y += (ratio * (animation->cfg.stop.y - animation->cfg.start.y)) >> 10;

	return pos;
}
#ifndef CONFIG_SIMULATOR
__attribute__((__unused__))
#endif
static ui_point_t _animation_update_position_bezier(ui_view_animation_t *animation)
{
	ui_point_t pos = animation->cfg.start;
	uint32_t t = ui_map(animation->elapsed, 0, animation->cfg.duration, 0, UI_VIEW_ANIM_RANGE);
	int32_t ratio = ui_bezier3(t, 0, 900, 950, UI_VIEW_ANIM_RANGE);

	if (animation->cfg.path_cb) {
		ratio = animation->cfg.path_cb(ratio);
	}

	pos.x += (ratio * (animation->cfg.stop.x - animation->cfg.start.x)) >> 10;
	pos.y += (ratio * (animation->cfg.stop.y - animation->cfg.start.y)) >> 10;

	return pos;
}

static void _view_animation_cleanup(ui_view_animation_t *animation)
{
	animation->state = UI_ANIM_NONE;

	view_manager_post_anim_refocus();

	/* unlock forbid draw ui */
	ui_service_unlock_gui(UISRV_GUI_BOTH);

#ifdef CONFIG_DMA2D_HAL
	hal_dma2d_set_global_enabled(true);
#endif

	if (animation->cfg.stop_cb) {
		animation->cfg.stop_cb(animation->view_id, &animation->cfg.stop);
	}

	SYS_LOG_INF("stopped\n");
}

int view_animation_start(ui_view_animation_t *animation, uint8_t view_id,
		int16_t last_view_id, ui_point_t *last_view_offset,
		ui_view_anim_cfg_t *cfg, bool is_slide_anim)
{
	if (animation->state != UI_ANIM_NONE)
		return -EBUSY;

	if (cfg->duration == 0)
		return -EINVAL;

	animation->view_id = view_id;
	animation->last_view_id = last_view_id;
	animation->last_view_offset = *last_view_offset;

	memcpy(&animation->cfg, cfg, sizeof(*cfg));
	animation->start_time = os_uptime_get_32();
	animation->is_slide = is_slide_anim;
	animation->state = UI_ANIM_START;

	SYS_LOG_INF("view %d, last_view %d, last_view_offset(%d %d), start(%d %d), stop(%d %d), duration %d\n",
			animation->view_id, animation->last_view_id,
			animation->last_view_offset.x, animation->last_view_offset.y,
			animation->cfg.start.x, animation->cfg.start.y, animation->cfg.stop.x,
			animation->cfg.stop.y, animation->cfg.duration);

	return 0;
}

void view_animation_process(ui_view_animation_t *animation)
{
	if (animation->state == UI_ANIM_NONE)
		return;

	if (animation->state == UI_ANIM_START) {
		animation->state = UI_ANIM_RUNNING;
	}

	if (animation->state == UI_ANIM_RUNNING) {
		animation->elapsed = os_uptime_get_32() - animation->start_time;
		if (animation->elapsed > animation->cfg.duration) {
			animation->elapsed = animation->cfg.duration;
			animation->state = UI_ANIM_STOP;
		}

		ui_point_t position = _animation_update_position_cos(animation);
		view_set_pos(animation->view_id, position.x, position.y);

		if (animation->last_view_id >= 0) {
			ui_point_t last_position = position;
			last_position.x += animation->last_view_offset.x;
			last_position.y += animation->last_view_offset.y;

			view_set_pos(animation->last_view_id, last_position.x, last_position.y);
		}

		SYS_LOG_DBG("animation process view %d, last_view %d, pos(%d %d), elapsed %d\n",
			animation->view_id, animation->last_view_id, position.x, position.y, animation->elapsed);
	}

	if (animation->state == UI_ANIM_STOP) {
		_view_animation_cleanup(animation);
	}
}

void view_animation_stop(ui_view_animation_t *animation, bool forced)
{
	if (animation->state == UI_ANIM_NONE)
		return;

	if (!forced && animation->is_slide)
		return;

	/* Directly goto the stop position */
	if (animation->state == UI_ANIM_START || animation->state == UI_ANIM_RUNNING) {
		if (animation->is_slide) {
			view_set_pos(animation->view_id, animation->cfg.stop.x, animation->cfg.stop.y);

			if (animation->last_view_id >= 0) {
				ui_point_t last_position = animation->cfg.stop;
				last_position.x += animation->last_view_offset.x;
				last_position.y += animation->last_view_offset.y;

				view_set_pos(animation->last_view_id, last_position.x, last_position.y);
			}
		}

		_view_animation_cleanup(animation);
	}
}
