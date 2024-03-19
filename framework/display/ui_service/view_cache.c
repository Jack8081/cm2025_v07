/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file view cache interface
 */

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "view_cache"
#include <os_common_api.h>
#include <ui_manager.h>
#include <view_cache.h>
#include <string.h>
#include <assert.h>

#define MAX_VIEW_CACHE		(sizeof(uint32_t) * 8)

typedef struct {
	const view_cache_dsc_t *dsc;
	uint32_t stat;
	uint8_t init_view; /* initial focused view */
	uint8_t rotate : 1;
	int8_t focus_idx;
	int8_t load_idx; /* the index that is loading */
} view_cache_ctx_t;

static int _view_cache_set_focus(uint8_t view_id);

static view_cache_ctx_t view_cache_ctx;
static OS_MUTEX_DEFINE(view_cache_mutex);

static int16_t _view_cache_get_view_id(uint8_t idx)
{
	if (idx < view_cache_ctx.dsc->num) {
		return view_cache_ctx.dsc->vlist[idx];
	} else if (idx < view_cache_ctx.dsc->num + 2) {
		return view_cache_ctx.dsc->cross_vlist[idx - view_cache_ctx.dsc->num];
	} else {
		return -1;
	}
}

static const void * _view_cache_get_presenter(uint8_t idx)
{
	if (idx < view_cache_ctx.dsc->num) {
		return view_cache_ctx.dsc->plist ? view_cache_ctx.dsc->plist[idx] : NULL;
	} else if (idx < view_cache_ctx.dsc->num + 2) {
		return view_cache_ctx.dsc->cross_plist[idx - view_cache_ctx.dsc->num];
	} else {
		return NULL;
	}
}

static int _view_cache_load(uint8_t idx, uint8_t create_flags)
{
	int16_t view_id;

	if (view_cache_ctx.stat & (1 << idx))
		return -EALREADY;

	view_id = _view_cache_get_view_id(idx);
	if (view_id >= 0) {
		view_cache_ctx.stat |= (1 << idx);
		return ui_view_create((uint8_t)view_id, _view_cache_get_presenter(idx), create_flags);
	}

	return -EINVAL;
}

static void _view_cache_unload(uint8_t idx)
{
	int16_t view_id;

	if (!(view_cache_ctx.stat & (1 << idx)))
		return;

	view_id = _view_cache_get_view_id(idx);
	assert(view_id >= 0);

	view_cache_ctx.stat &= ~(1 << idx);
	ui_view_delete((uint8_t)view_id);
}

static int _view_cache_set_attr(uint8_t idx, uint8_t attr)
{
	int16_t view_id;

	if (!(view_cache_ctx.stat & (1 << idx)))
		return -EINVAL;

	view_id = _view_cache_get_view_id(idx);
	assert(view_id >= 0);

	if (view_set_drag_attribute((uint8_t)view_id, attr))
		ui_view_set_drag_attribute((uint8_t)view_id, attr);

	return 0;
}

static int8_t _view_cache_get_main_idx(uint8_t view_id)
{
	int8_t idx;

	for (idx = view_cache_ctx.dsc->num - 1; idx >= 0; idx--) {
		if (view_cache_ctx.dsc->vlist[idx] == view_id) {
			return idx;
		}
	}

	return -1;
}

static int8_t _view_cache_rotate_main_idx(int8_t idx)
{
	while (idx < 0)
		idx += view_cache_ctx.dsc->num;

	while (idx >= view_cache_ctx.dsc->num)
		idx -= view_cache_ctx.dsc->num;

	return idx;
}

static bool _view_cache_main_idx_is_in_range(int8_t idx)
{
	int8_t min_idx = view_cache_ctx.focus_idx - CONFIG_VIEW_CACHE_LEVEL;
	int8_t max_idx = view_cache_ctx.focus_idx + CONFIG_VIEW_CACHE_LEVEL;

	if (view_cache_ctx.rotate) {
		if (view_cache_ctx.dsc->num <= (CONFIG_VIEW_CACHE_LEVEL * 2 + 1))
			return true;

		idx = _view_cache_rotate_main_idx(idx);
		if (min_idx < 0) {
			min_idx += view_cache_ctx.dsc->num;
			if (idx >= min_idx || idx <= max_idx)
				return true;
		} else if (max_idx >= view_cache_ctx.dsc->num) {
			max_idx -= view_cache_ctx.dsc->num;
			if (idx >= min_idx || idx <= max_idx)
				return true;
		} else if (idx >= min_idx && idx <= max_idx) {
			return true;
		}
	} else {
		if (idx >= UI_MAX(min_idx, 0) &&
			idx <= UI_MIN(max_idx, view_cache_ctx.dsc->num - 1))
			return true;
	}

	return false;
}

static int _view_cache_load_main(int8_t idx, uint8_t create_flags)
{
	if (view_cache_ctx.rotate) {
		idx = _view_cache_rotate_main_idx(idx);
	} else if (idx < 0 || idx >= view_cache_ctx.dsc->num) {
		return -EINVAL;
	}

	return _view_cache_load(idx, create_flags);
}

static void _view_cache_unload_main(int8_t idx)
{
	if (view_cache_ctx.rotate) {
		if (_view_cache_main_idx_is_in_range(idx))
			return;

		idx = _view_cache_rotate_main_idx(idx);
	} else if (idx < 0 || idx >= view_cache_ctx.dsc->num) {
		return;
	}

	_view_cache_unload(idx);
}

static int _view_cache_set_attr_main(int8_t idx, uint8_t attr)
{
	if (view_cache_ctx.rotate) {
		idx = _view_cache_rotate_main_idx(idx);
	} else if (idx < 0 || idx >= view_cache_ctx.dsc->num) {
		return -EINVAL;
	}

	return _view_cache_set_attr(idx, attr);
}

static uint8_t _view_cache_decide_attr(int8_t idx)
{
	if (view_cache_ctx.dsc->type == LANDSCAPE) {
		if (idx == view_cache_ctx.focus_idx - 1)
			return UI_DRAG_MOVERIGHT;
		else if (idx == view_cache_ctx.focus_idx + 1)
			return UI_DRAG_MOVELEFT;
		else if (idx == view_cache_ctx.dsc->num)
			return UI_DRAG_DROPDOWN;
		else if (idx == view_cache_ctx.dsc->num + 1)
			return UI_DRAG_DROPUP;
		else
			return 0;
	} else {
		if (idx == view_cache_ctx.focus_idx - 1)
			return UI_DRAG_MOVEDOWN;
		else if (idx == view_cache_ctx.focus_idx + 1)
			return UI_DRAG_MOVEUP;
		else if (idx == view_cache_ctx.dsc->num)
			return UI_DRAG_DROPRIGHT;
		else if (idx == view_cache_ctx.dsc->num + 1)
			return UI_DRAG_DROPLEFT;
		else
			return 0;
	}
}

static void _view_cache_serial_load(void)
{
	int8_t focus_idx = view_cache_ctx.focus_idx;

	for(; view_cache_ctx.load_idx >= view_cache_ctx.dsc->num; view_cache_ctx.load_idx--) {
		if (!_view_cache_load(view_cache_ctx.load_idx, 0)) {
			_view_cache_set_attr(view_cache_ctx.load_idx, _view_cache_decide_attr(view_cache_ctx.load_idx));
			return;
		}
	}

	for (; view_cache_ctx.load_idx >= 0; view_cache_ctx.load_idx--) {
		if (_view_cache_main_idx_is_in_range(view_cache_ctx.load_idx) &&
			!_view_cache_load(view_cache_ctx.load_idx, 0)) {
			return;
		}
	}

	if (view_cache_ctx.dsc->event_cb) {
		view_cache_ctx.dsc->event_cb(VIEW_CACHE_EVT_LOAD_END);
	}

	_view_cache_set_attr_main(focus_idx - 1, _view_cache_decide_attr(focus_idx - 1));
	_view_cache_set_attr_main(focus_idx + 1, _view_cache_decide_attr(focus_idx + 1));
}

static void _view_cache_notify_cb(uint8_t view_id, uint8_t msg_id)
{
	os_mutex_lock(&view_cache_mutex, OS_FOREVER);

	if (view_cache_ctx.dsc == NULL) {
		goto out_unlock;
	}

	if (msg_id == MSG_VIEW_FOCUS || msg_id == MSG_VIEW_DEFOCUS) {
		bool focused = (msg_id == MSG_VIEW_FOCUS);

		if (focused && view_cache_ctx.load_idx < 0)
			_view_cache_set_focus(view_id);

		if (view_cache_ctx.dsc->focus_cb)
			view_cache_ctx.dsc->focus_cb(view_id, focused);
	}

	if (view_cache_ctx.load_idx >= 0 && msg_id == MSG_VIEW_LAYOUT) {
		_view_cache_serial_load();
	}

out_unlock:
	os_mutex_unlock(&view_cache_mutex);
}

int view_cache_init(const view_cache_dsc_t *dsc, uint8_t init_view)
{
	int8_t init_idx;
	int res = 0;

	/* Also consider the cross views */
	if (dsc->num + 2 > MAX_VIEW_CACHE) {
		return -EDOM;
	}

	os_mutex_lock(&view_cache_mutex, OS_FOREVER);

	if (view_cache_ctx.dsc) {
		res = -EALREADY;
		goto out_unlock;
	}

	view_cache_ctx.dsc = dsc;

	init_idx = _view_cache_get_main_idx(init_view);
	if (init_idx < 0 || init_idx >= dsc->num) {
		view_cache_ctx.dsc = NULL;
		res = -EINVAL;
		goto out_unlock;
	}

	view_cache_ctx.init_view = init_view;
	view_cache_ctx.rotate = (view_cache_ctx.dsc->rotate && view_cache_ctx.dsc->num >= 3);

	ui_manager_set_notify_callback(_view_cache_notify_cb);

	if (dsc->serial_load == 0) {
		/* mark serial loading unnecessary*/
		view_cache_ctx.load_idx = -1;

		/* load focused view */
		_view_cache_load(init_idx, UI_CREATE_FLAG_SHOW);

		/* load cross views */
		if (!_view_cache_load(dsc->num, 0)) {
			_view_cache_set_attr(dsc->num, _view_cache_decide_attr(dsc->num));
		}
		if (!_view_cache_load(dsc->num + 1, 0)) {
			_view_cache_set_attr(dsc->num + 1, _view_cache_decide_attr(dsc->num + 1));
		}

		/* load other main views */
		_view_cache_set_focus(init_view);
	} else {
		view_cache_ctx.load_idx = dsc->num + 1;
		view_cache_ctx.focus_idx = init_idx;

		if (dsc->event_cb) {
			dsc->event_cb(VIEW_CACHE_EVT_LOAD_BEGIN);
		}

		/* load focused view */
		_view_cache_load(init_idx, UI_CREATE_FLAG_SHOW);
	}

out_unlock:
	SYS_LOG_INF("res=%d", res);
	os_mutex_unlock(&view_cache_mutex);
	return res;
}

void view_cache_deinit(void)
{
	int8_t idx;

	os_mutex_lock(&view_cache_mutex, OS_FOREVER);

	if (view_cache_ctx.dsc == NULL) {
		goto out_unlock;
	}

	ui_manager_set_notify_callback(NULL);

	if (view_cache_ctx.dsc->serial_load && view_cache_ctx.load_idx >= 0 &&
		view_cache_ctx.dsc->event_cb) {
		view_cache_ctx.dsc->event_cb(VIEW_CACHE_EVT_LOAD_CANCEL);
	}

	if (view_cache_ctx.focus_idx >= 0 && view_cache_ctx.dsc->focus_cb) {
		/* notify the defocus */
		view_cache_ctx.dsc->focus_cb((uint8_t)_view_cache_get_view_id(view_cache_ctx.focus_idx), false);
	}

	/* delte the main views first to avoid unexpected focus changes */
	for (idx = 0; idx < view_cache_ctx.dsc->num + 2; idx++) {
		_view_cache_unload(idx);
	}

	memset(&view_cache_ctx, 0, sizeof(view_cache_ctx));

out_unlock:
	SYS_LOG_INF("");
	os_mutex_unlock(&view_cache_mutex);
}

int16_t view_cache_get_focus_view(void)
{
	int16_t view_id = -1;

	os_mutex_lock(&view_cache_mutex, OS_FOREVER);

	if (view_cache_ctx.dsc) {
		view_id = view_cache_ctx.dsc->vlist[view_cache_ctx.focus_idx];
	}

	os_mutex_unlock(&view_cache_mutex);
	return view_id;
}

int view_cache_set_focus_view(uint8_t view_id)
{
	const view_cache_dsc_t *dsc;
	int8_t focus_idx, idx;
	int res;

	SYS_LOG_INF("set focus view %u", view_id);

	os_mutex_lock(&view_cache_mutex, OS_FOREVER);

	dsc = view_cache_ctx.dsc;
	if (dsc == NULL) {
		res = -ESRCH;
		goto out_unlock;
	}

	focus_idx = _view_cache_get_main_idx(view_id);
	if (focus_idx < 0) {
		res = -EINVAL;
		goto out_unlock;
	}

	if (focus_idx == view_cache_ctx.focus_idx) {
		res = 0;
		goto out_unlock;
	}

	if (dsc->serial_load && view_cache_ctx.load_idx >= 0) {
		res = -EBUSY;
		goto out_unlock;
	}

	if (focus_idx == view_cache_ctx.focus_idx + 1 ||
		focus_idx == view_cache_ctx.focus_idx -1) {
		/* reset the position and focus */
		ui_view_set_pos(view_id, 0, 0);
		ui_view_hide(dsc->vlist[view_cache_ctx.focus_idx]);
		/* reset the view relation */
		res = _view_cache_set_focus(view_id);
		goto out_unlock;
	}

	/* unload all the main views */
	for (idx = view_cache_ctx.dsc->num - 1; idx >= 0; idx--) {
		_view_cache_unload(idx);
	}

	if (dsc->serial_load == 0) {
		/* load other main views */
		res = _view_cache_set_focus(view_id);
	} else {
		if (dsc->event_cb) {
			dsc->event_cb(VIEW_CACHE_EVT_LOAD_BEGIN);
		}

		view_cache_ctx.load_idx = dsc->num - 1;
		view_cache_ctx.focus_idx = focus_idx;

		/* load focused view */
		res = _view_cache_load(focus_idx, UI_CREATE_FLAG_SHOW);
	}

out_unlock:
	os_mutex_unlock(&view_cache_mutex);
	return res;
}

void view_cache_dump(void)
{
	uint8_t idx;

	if (view_cache_ctx.dsc == NULL)
		return;

	SYS_LOG_INF("main list (num=%d):", view_cache_ctx.dsc->num);

	for (idx = 0; idx < view_cache_ctx.dsc->num; idx ++) {
		char c = ' ';

		if (view_cache_ctx.stat & (1 << idx)) {
			c = (idx == view_cache_ctx.focus_idx) ? '*' : '+';
		}

		SYS_LOG_INF("%c %d", c, view_cache_ctx.dsc->vlist[idx]);
	}

	SYS_LOG_INF("cross list: %d, %d", view_cache_ctx.dsc->cross_vlist[0],
			view_cache_ctx.dsc->cross_vlist[1]);
}

static int _view_cache_set_focus(uint8_t view_id)
{
	int8_t focus_idx = _view_cache_get_main_idx(view_id);
	int8_t prefocus_idx = view_cache_ctx.focus_idx;

	if (focus_idx < 0) {
		return -EINVAL;
	}

	// save cur view, _view_cache_decide_attr() depends on the focus_idx.
	view_cache_ctx.focus_idx = focus_idx;

	if (view_cache_ctx.dsc->cross_only_init_focused) {
		uint8_t cross_attr[2] = { 0, 0 };

		if (view_id == view_cache_ctx.init_view) {
			cross_attr[0] = _view_cache_decide_attr(view_cache_ctx.dsc->num);
			cross_attr[1] = _view_cache_decide_attr(view_cache_ctx.dsc->num + 1);
		}

		_view_cache_set_attr(view_cache_ctx.dsc->num, cross_attr[0]);
		_view_cache_set_attr(view_cache_ctx.dsc->num + 1, cross_attr[1]);
	}

	// clear attr for old prev/next view
	_view_cache_set_attr_main(prefocus_idx - 1, 0);
	_view_cache_set_attr_main(prefocus_idx + 1, 0);

	// unload unused view
	_view_cache_unload_main(focus_idx - (CONFIG_VIEW_CACHE_LEVEL + 1));
	_view_cache_unload_main(focus_idx + (CONFIG_VIEW_CACHE_LEVEL + 1));

	// preload and show new cur view
	_view_cache_load_main(focus_idx, UI_CREATE_FLAG_SHOW);

	// preload and set attr for new prev/next view
	_view_cache_load_main(focus_idx - 1, 0);
	_view_cache_load_main(focus_idx + 1, 0);
	_view_cache_set_attr_main(focus_idx - 1, _view_cache_decide_attr(focus_idx - 1));
	_view_cache_set_attr_main(focus_idx + 1, _view_cache_decide_attr(focus_idx + 1));
#if CONFIG_VIEW_CACHE_LEVEL > 1
	_view_cache_load_main(focus_idx - CONFIG_VIEW_CACHE_LEVEL, 0);
	_view_cache_load_main(focus_idx + CONFIG_VIEW_CACHE_LEVEL, 0);
#endif

	return 0;
}
