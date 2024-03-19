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
#include <msg_manager.h>
#include <app_manager.h>
#include <mem_manager.h>
#include <string.h>
#include <ui_surface.h>
#include <display/display_composer.h>
#include "ui_service_inner.h"
#ifdef CONFIG_UISRV_VIEW_PAUSED_SNAPSHOT
#include <spress.h>
#include <ui_mem.h>
#include <memory/mem_cache.h>
#endif /* CONFIG_UISRV_VIEW_PAUSED_SNAPSHOT */

#ifdef CONFIG_UISRV_VIEW_PAUSED_SNAPSHOT

#define NUM_COMPRESS_ENTRIES (6)

typedef struct {
	uint8_t view_id;
	graphic_buffer_t *buffer;
} compress_entry_t;

static char compress_thread_stack[1536] __aligned(Z_THREAD_MIN_STACK_ALIGN);
#endif /* CONFIG_UISRV_VIEW_PAUSED_SNAPSHOT */

typedef struct {
	sys_slist_t view_list;
	ui_region_t disp_region;
	ui_region_t dirty_region;
	ui_view_animation_t animation;
	ui_view_context_t *pre_focus_view;
	ui_view_context_t *focused_view;
	ui_notify_cb_t notify_cb;
	ui_msgbox_popup_cb_t msgbox_popup_cb;
	ui_keyevent_cb_t keyevent_cb;
	uint32_t post_id;
	uint32_t dirty : 1;
	uint32_t mark_all_dirty : 1;
	os_mutex post_mutex;

#ifdef CONFIG_UISRV_VIEW_PAUSED_SNAPSHOT
	compress_entry_t compress_entries[NUM_COMPRESS_ENTRIES];
	os_sem compress_busy_sem;
	os_sem compress_free_sem;
	uint8_t compress_busy_idx;
	uint8_t compress_free_idx;
#endif
} view_manager_context_t;

extern view_entry_t __view_entry_table[];
extern view_entry_t __view_entry_end[];

static view_manager_context_t view_manager_context;

static void _view_post_cleanup(ui_view_context_t *view);

static view_manager_context_t *_view_manager_get_context(void)
{
	return &view_manager_context;
}

static void _view_manager_add_view(ui_view_context_t *view)
{
#ifdef CONFIG_SIMULATOR
	sys_snode_t* node;
#endif
	ui_view_context_t *cur_view;
	ui_view_context_t *pre_view = NULL;
	view_manager_context_t *view_manager = _view_manager_get_context();
#ifdef CONFIG_SIMULATOR
	SYS_SLIST_FOR_EACH_NODE_SAFE(&view_manager->view_list, cur_view, node) {
#else
	SYS_SLIST_FOR_EACH_CONTAINER(&view_manager->view_list, cur_view, node) {
#endif
		if (view->order <= cur_view->order) {
			if (pre_view) {
				sys_slist_insert(&view_manager->view_list, &pre_view->node, &view->node);
			} else {
				sys_slist_prepend(&view_manager->view_list, (sys_snode_t *)&view->node);
			}

			return;
		}

		pre_view = cur_view;
	}

	sys_slist_append(&view_manager->view_list, (sys_snode_t *)&view->node);
}

static ui_view_context_t *_view_manager_get_view_context(uint8_t view_id)
{
	ui_view_context_t *view;
#ifdef CONFIG_SIMULATOR
	sys_snode_t* node;
#endif
	view_manager_context_t *view_manager = _view_manager_get_context();
#ifdef CONFIG_SIMULATOR
	SYS_SLIST_FOR_EACH_NODE_SAFE(&view_manager->view_list, view, node) {
#else
	SYS_SLIST_FOR_EACH_CONTAINER(&view_manager->view_list, view, node) {
#endif
		if (view->entry->id == view_id)
			return view;
	}

	return NULL;
}

static view_entry_t *_view_manager_get_view_entry(uint8_t view_id)
{
	view_entry_t *view_entry;

	for (view_entry = __view_entry_table;
	     view_entry < __view_entry_end;
	     view_entry++) {

		if (view_entry->id == view_id) {
			return view_entry;
		}
	}

	return NULL;
}

bool view_manager_is_dirty(void)
{
	view_manager_context_t *view_manager = _view_manager_get_context();

	return view_manager->dirty;
}

void view_manager_set_dirty(bool dirty)
{
	view_manager_context_t *view_manager = _view_manager_get_context();

	view_manager->dirty = dirty ? 1 : 0;
}

void view_manager_mark_disp_dirty(void)
{
	view_manager_context_t *view_manager = _view_manager_get_context();

	os_mutex_lock(&view_manager->post_mutex, OS_FOREVER);
	memcpy(&view_manager->dirty_region, &view_manager->disp_region, sizeof(ui_region_t));
	view_manager_set_dirty(true);
	view_manager->mark_all_dirty = 1;
	os_mutex_unlock(&view_manager->post_mutex);
}

static bool _view_manager_add_dirty_region(const ui_region_t *dirty_region)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	ui_region_t region;

	if (ui_region_intersect(&region, dirty_region, &view_manager->disp_region) == false) {
		return false;
	}

	ui_region_merge(&view_manager->dirty_region, &view_manager->dirty_region, &region);

	SYS_LOG_DBG("dirty %d %d %d %d\n",
			view_manager->dirty_region.x1, view_manager->dirty_region.y1,
			view_manager->dirty_region.x2, view_manager->dirty_region.y2);

	return true;
}

view_data_t *view_get_data(uint8_t view_id)
{
	ui_view_context_t *view;

	view = _view_manager_get_view_context(view_id);
	if (view == NULL)
		return NULL;

	return &view->data;
}

static int _view_proc_callback(ui_view_context_t *view, uint8_t msg, void *msg_data)
{
	int res;

	os_strace_u32x2(SYS_TRACE_ID_VIEW_PROC_CB, view->entry->id, msg);
	res = view->entry->proc(view->entry->id, msg, msg_data ? msg_data : &view->data);
	os_strace_end_call_u32(SYS_TRACE_ID_VIEW_PROC_CB, view->entry->id);

	return res;
}

int view_notify_message(uint8_t view_id, uint8_t msg_id, void *msg_data)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);
	int res = -ENOMEM;

	if (!view)
		return -ESRCH;

	switch (msg_id) {
	case MSG_VIEW_FOCUS:
		SYS_LOG_INF("view %d focused", view_id);
#ifdef CONFIG_SURFACE_DOUBLE_BUFFER
		res = surface_set_min_buffer_count(view->data.surface, 2);
#endif
		view_gui_resume_draw(view, res ? false : true);
		break;

	case MSG_VIEW_DEFOCUS:
		SYS_LOG_INF("view %d defocused", view_id);
#ifdef CONFIG_SURFACE_DOUBLE_BUFFER
		surface_set_max_buffer_count(view->data.surface, 1);
#endif
		view_gui_pause_draw(view);
		break;

	case MSG_VIEW_UPDATE:
		view_gui_resume_draw(view, false);
		break;

	default:
		break;
	}

	_view_proc_callback(view, msg_id, msg_data);
	return 0;
}

int view_create(uint8_t view_id, const void *presenter, uint8_t flags)
{
	ui_view_context_t *view;
	view_entry_t * view_entry;
	int res = 0;

	os_strace_u32(SYS_TRACE_ID_VIEW_CREATE, view_id);

	view = _view_manager_get_view_context(view_id);
	if (view != NULL) {
		SYS_LOG_WRN("view %u already created\n", view_id);
		return -ENOENT;
	}

	view_entry = _view_manager_get_view_entry(view_id);
	if (!view_entry) {
		SYS_LOG_ERR("view %u not found\n", view_id);
		return -ENOENT;
	}

	view = mem_malloc(sizeof(ui_view_context_t));
	if (!view) {
		return -ENOMEM;
	}

	memset(view, 0, sizeof(ui_view_context_t));
	view->entry = view_entry;
	view->flags = (flags & UI_CREATE_FLAG_SHOW) ? 0 : UI_FLAG_HIDDEN;
	view->order = view_entry->default_order;

	view->region.x1 = 0;
	view->region.y1 = 0;
	view->region.x2 = view->entry->width - 1;
	view->region.y2 = view->entry->height - 1;
	view->data.presenter = presenter;

	res = view_gui_init(view, flags);
	if (res) {
		SYS_LOG_ERR("view %u gui init failed\n", view_id);
		mem_free(view);
		return res;
	}

	view_gui_clear_begin(view);

	if ((flags & UI_CREATE_FLAG_NO_PRELOAD) == 0) {
		res = _view_proc_callback(view, MSG_VIEW_PRELOAD, NULL);
		if (res) {
			SYS_LOG_ERR("view %u preload failed\n", view_id);
			view_gui_clear_end(view);
			view_gui_deinit(view);
			mem_free(view);
			return res;
		}
	} else {
		ui_service_send_self_msg(view_id, MSG_VIEW_LAYOUT);
	}

	_view_manager_add_view(view);

	if (flags & UI_CREATE_FLAG_SHOW)
		view_manager_refocus(view);

	os_strace_end_call_u32(SYS_TRACE_ID_VIEW_CREATE, view_id);

	SYS_LOG_INF("view %d created\n", view_id);
	return 0;
}

int view_layout(uint8_t view_id)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	ui_view_context_t *view = _view_manager_get_view_context(view_id);
	uint8_t has_layout;
	int res;

	if (!view) {
		return -ESRCH;
	}

	os_strace_u32(SYS_TRACE_ID_VIEW_LAYOUT, view_id);

	has_layout = (view->flags & UI_FLAG_INFLATED);

	/* layout() may be invoked several times by view implementation */
	if (has_layout == 0) {
		view_gui_clear_end(view);
	}

	/* After 1st inflation, view can be moved
	 *
	 * Move here before _view_proc_callback, since user may trigger view refresh
	 * here through gui API, like LVGL lv_refr_now()
	 */
	if (has_layout == 0) {
		view->flags |= UI_FLAG_INFLATED;
	}

	res = _view_proc_callback(view, MSG_VIEW_LAYOUT, NULL);
	if (res) {
		SYS_LOG_ERR("view %d layout failed, delete it\n", view_id);
		view_delete(view_id);
		return res;
	}

	/* FIXME: refresh the first frame here ? */
	view_gui_refresh(view);
	/* Make view implemention easier: notify the de-focused state */
	if ((view->flags & UI_FLAG_FOCUSED) == 0) {
		view_notify_message(view_id, MSG_VIEW_DEFOCUS, NULL);
	}

	if (has_layout == 0 && view_manager->notify_cb) {
		view_manager->notify_cb(view_id, MSG_VIEW_LAYOUT);
	}

	os_strace_end_call_u32(SYS_TRACE_ID_VIEW_LAYOUT, view_id);

	SYS_LOG_INF("view %d layout inflated\n", view_id);
	return 0;
}

int view_paint(uint8_t view_id)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	if (!view) {
		return -ESRCH;
	}

	return _view_proc_callback(view, MSG_VIEW_PAINT, NULL);
}

int view_refresh(uint8_t view_id)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	if (!view) {
		return -ESRCH;
	}

	view_gui_refresh(view);
	return 0;
}

int view_delete(uint8_t view_id)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	if (!view)
		return -ESRCH;

	os_strace_u32(SYS_TRACE_ID_VIEW_DELETE, view_id);

	/* Close all attached msgboxes. */
	view_manager_close_msgbox(UI_MSGBOX_ALL, view_id);

	_view_proc_callback(view, MSG_VIEW_DELETE, NULL);
	view_gui_deinit(view);

	os_sched_lock();
	if ((view->flags & UI_FLAG_HIDDEN) == 0) {
		gesture_manager_stop_process(input_dispatcher_get_pointer_dev(), true);
	}
	sys_slist_find_and_remove(&view_manager->view_list, &view->node);
	if (view == view_manager->focused_view) {
		view_manager_refocus(NULL);
		/* refresh only when new focused view exist */
		if (view_manager->focused_view)
			view_manager_mark_disp_dirty();
	}
	os_sched_unlock();

#ifdef CONFIG_UISRV_VIEW_PAUSED_SNAPSHOT
	if (view->snapshot)
		ui_memory_free(view->snapshot);
#endif

	mem_free(view);

	os_strace_end_call_u32(SYS_TRACE_ID_VIEW_DELETE, view_id);

	SYS_LOG_INF("view %d deleted\n", view_id);
	return 0;
}

bool view_is_paused(uint8_t view_id)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	return (view && (view->flags & UI_FLAG_PAUSED)) ? true : false;
}

int view_pause(uint8_t view_id)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	if (!view)
		return -ESRCH;

	os_strace_u32(SYS_TRACE_ID_VIEW_PAUSE, view_id);

	view->flags = (view->flags & ~UI_FLAG_PAINTED) | UI_FLAG_PAUSED;

#ifdef CONFIG_UISRV_VIEW_PAUSED_SNAPSHOT
	os_sem_take(&view_manager->compress_free_sem, OS_FOREVER);
	graphic_buffer_t *buf = surface_get_frontbuffer(view->data.surface);
	graphic_buffer_ref(buf);

	view_manager->compress_entries[view_manager->compress_free_idx].buffer = buf;
	view_manager->compress_entries[view_manager->compress_free_idx].view_id = view_id;
	if (++view_manager->compress_free_idx >= NUM_COMPRESS_ENTRIES)
		view_manager->compress_free_idx = 0;
	os_sem_give(&view_manager->compress_busy_sem);
#endif

	surface_set_max_buffer_count(view->data.surface, 0);
	view_gui_pause_draw(view);

	if (view == view_manager->focused_view) {
		view_manager_refocus(NULL);
		view_manager_mark_disp_dirty();
	}

	_view_proc_callback(view, MSG_VIEW_PAUSE, NULL);

#if 1
	if (view_manager->notify_cb) {
		os_strace_u32x2(SYS_TRACE_ID_VIEW_NOTIFY_CB, view_id, MSG_VIEW_PAUSE);
		view_manager->notify_cb(view_id, MSG_VIEW_PAUSE);
		os_strace_end_call_u32(SYS_TRACE_ID_VIEW_NOTIFY_CB, view_id);
	}
#endif

	os_strace_end_call_u32(SYS_TRACE_ID_VIEW_PAUSE, view_id);

	SYS_LOG_INF("view %d paused\n", view_id);
	return 0;
}

int view_resume(uint8_t view_id)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	if (!view)
		return -ESRCH;

	os_strace_u32(SYS_TRACE_ID_VIEW_RESUME, view_id);

	surface_set_min_buffer_count(view->data.surface, 1);

#ifdef CONFIG_UISRV_VIEW_PAUSED_SNAPSHOT
	if (view->snapshot) {
		graphic_buffer_t *buf = surface_get_frontbuffer(view->data.surface);
		void * bufptr = (void *)graphic_buffer_get_bufptr(buf, 0, 0);

		os_strace_u32x2(SYS_TRACE_ID_VIEW_DECOMPRESS, view_id, MSG_VIEW_PAUSE);
		unsigned int len = spr_decompress(view->snapshot, bufptr);
		mem_dcache_flush(bufptr, len);
		mem_dcache_sync();
		os_strace_end_call_u32(SYS_TRACE_ID_VIEW_DECOMPRESS, len);

		ui_memory_free(view->snapshot);
		view->snapshot = NULL;
	} else {
		view_gui_clear_begin(view);
		view_gui_clear_end(view);
	}
#else
	view_gui_clear_begin(view);
	view_gui_clear_end(view);
#endif /* CONFIG_UISRV_VIEW_PAUSED_SNAPSHOT */

	view_gui_resume_draw(view, true);
	_view_proc_callback(view, MSG_VIEW_RESUME, NULL);

	if (view_manager->notify_cb) {
		os_strace_u32x2(SYS_TRACE_ID_VIEW_NOTIFY_CB, view_id, MSG_VIEW_RESUME);
		view_manager->notify_cb(view_id, MSG_VIEW_RESUME);
		os_strace_end_call_u32(SYS_TRACE_ID_VIEW_NOTIFY_CB, view_id);
	}

	view->flags &= ~UI_FLAG_PAUSED;
	if ((view->flags & UI_FLAG_HIDDEN) == 0) {
		view_manager_refocus(view);
		if (view == view_manager->focused_view) {
			view_manager_mark_disp_dirty();
		}
	}

	os_strace_end_call_u32(SYS_TRACE_ID_VIEW_RESUME, view_id);

	SYS_LOG_INF("view %d resume\n", view_id);
	return 0;
}

int view_set_order(uint8_t view_id, uint8_t order)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	if (!view)
		return -ESRCH;

	sys_slist_find_and_remove(&view_manager->view_list, &view->node);

	view->order = order;

	_view_manager_add_view(view);
	view_manager_refocus(view);

	return 0;
}

int view_set_refresh_en(uint8_t view_id, bool enabled)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	if (!view) {
		return -ESRCH;
	}

	if (view->flags & UI_FLAG_PAUSED) {
		enabled = false;
	}

	if (enabled) {
		view_gui_resume_draw(view, false);
	} else {
		view_gui_pause_draw(view);
	}

	return 0;
}

int view_set_refresh_mode(uint8_t view_id, uint8_t mode)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	if (!view) {
		return -ESRCH;
	}

	SYS_LOG_INF("refresh mode %d", mode);

	os_strace_u32x3(SYS_TRACE_ID_VIEW_SETPROP, view_id, UI_FLAG_REFRESH_CHANGED, mode);

	switch (mode) {
	case UI_REFR_SINGLE_BUF:
		surface_set_swap_mode(view->data.surface, SURFACE_SWAP_SINGLE_BUF);
		break;
	case UI_REFR_ZERO_BUF:
		surface_set_swap_mode(view->data.surface, SURFACE_SWAP_ZERO_BUF);
		break;
	case UI_REFR_DEFAULT:
	default:
		surface_set_swap_mode(view->data.surface, SURFACE_SWAP_DEFAULT);
		break;
	}

	os_strace_end_call_u32(SYS_TRACE_ID_VIEW_SETPROP, view_id);

	return 0;
}

int view_set_dirty_region(ui_view_context_t *view, const ui_region_t *dirty_region, uint32_t flags)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	ui_region_t region;
	uint8_t swap_mode;
	bool do_cleanup = false;
	bool do_post = false;
	int ret;

	if (!view)
		return -ESRCH;

	swap_mode = surface_get_swap_mode(view->data.surface);
	if (swap_mode != SURFACE_SWAP_ZERO_BUF) {
		assert((flags & (SURFACE_FIRST_DRAW | SURFACE_LAST_DRAW)) ==
				(SURFACE_FIRST_DRAW | SURFACE_LAST_DRAW));
	}

	SYS_LOG_DBG("view %u dirty region (%d, %d, %d, %d)\n", view->entry->id,
		dirty_region->x1, dirty_region->y1, dirty_region->x2, dirty_region->y2);

	if ((view->flags & (UI_FLAG_INFLATED | UI_FLAG_PAINTED)) == UI_FLAG_INFLATED) {
		view->flags |= UI_FLAG_PAINTED;
		SYS_LOG_DBG("view %d painted\n", view->entry->id);
	}

	if (ui_service_is_draw_locked()) {
		_view_post_cleanup(view);
		return 0;
	}

	/* the view is invisible */
	if ((view->flags & (UI_FLAG_HIDDEN | UI_FLAG_PAUSED | UI_FLAG_INFLATED)) != UI_FLAG_INFLATED) {
		_view_post_cleanup(view);
		return 0;
	}

	region.x1 = dirty_region->x1 + view->region.x1;
	region.y1 = dirty_region->y1 + view->region.y1;
	region.x2 = dirty_region->x2 + view->region.x1;
	region.y2 = dirty_region->y2 + view->region.y1;

	os_mutex_lock(&view_manager->post_mutex, OS_FOREVER);
	ret = _view_manager_add_dirty_region(&region);
	if (ret) {
		if (view->refr_flags & UI_REFR_FLAG_CHANGED) {
			do_cleanup = true; /* cleanup previous changed */
		}

		view->refr_flags = UI_REFR_FLAG_CHANGED;
		if (flags & SURFACE_LAST_DRAW)
			view->refr_flags |= UI_REFR_FLAG_LAST_CHANGED;
		if (flags & SURFACE_FIRST_DRAW)
			view->refr_flags |= UI_REFR_FLAG_FIRST_CHANGED;

		do_post = (swap_mode == SURFACE_SWAP_ZERO_BUF) ||
			(gesture_is_scrolling() == false &&
				(flags & (SURFACE_FIRST_DRAW | SURFACE_LAST_DRAW)) ==
					(SURFACE_FIRST_DRAW | SURFACE_LAST_DRAW));

		view_manager_set_dirty(true); /* Set dirty flag */
	} else {
		do_cleanup = true; /* cleanup current, since the view is invisible */
	}

	os_mutex_unlock(&view_manager->post_mutex);

	if (do_cleanup) {
		/* no need to sync */
		_view_post_cleanup(view);
	}

	if (do_post) {
		view_manager_post_display();
	}

	return 0;
}

bool view_is_painted(uint8_t view_id)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	return (view && (view->flags & UI_FLAG_PAINTED)) ? true : false;
}

uint8_t view_get_buffer_count(uint8_t view_id)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	return view ? surface_get_buffer_count(view->data.surface) : 0;
}

int16_t view_get_x(uint8_t view_id)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	return view ? view->region.x1 : INT16_MIN;
}

int16_t view_get_y(uint8_t view_id)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	return view ? view->region.y1 : INT16_MIN;
}

int view_get_pos(uint8_t view_id, int16_t *x, int16_t *y)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	if (!view)
		return -ESRCH;

	if (x) *x = view->region.x1;
	if (y) *y = view->region.y1;

	return 0;
}

int16_t view_get_width(uint8_t view_id)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	return view ? ui_region_get_width(&view->region) : -ESRCH;
}

int16_t view_get_height(uint8_t view_id)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	return view ? ui_region_get_height(&view->region) : -ESRCH;
}

int view_get_region(uint8_t view_id, ui_region_t *region)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	if (!view)
		return -ESRCH;

	ui_region_copy(region, &view->region);
	return 0;
}

static int _view_ctx_set_position(ui_view_context_t *view, int16_t x, int16_t y)
{
	view_manager_context_t *view_manager = _view_manager_get_context();

	x = UI_ALIGN(x, X_ALIGN_SIZE);
	y = UI_ALIGN(y, Y_ALIGN_SIZE);

	if (view->region.x1 != x || view->region.y1 != y) {
		bool not_hidden = (view->flags & (UI_FLAG_HIDDEN | UI_FLAG_PAUSED | UI_FLAG_INFLATED)) == UI_FLAG_INFLATED;
		bool dirty = false;

		os_mutex_lock(&view_manager->post_mutex, OS_FOREVER);

		if (not_hidden) {
			dirty = _view_manager_add_dirty_region(&view->region);
		}

		ui_region_set_pos(&view->region, x, y);

		if (not_hidden) {
			dirty |= _view_manager_add_dirty_region(&view->region);
		}

		if (dirty) {
			view->refr_flags |= UI_REFR_FLAG_MOVED;
			view_manager_set_dirty(true);
		}

		os_mutex_unlock(&view_manager->post_mutex);

		SYS_LOG_DBG("view %u pos (%d %d %d %d)\n", view->entry->id,
			view->region.x1, view->region.y1, view->region.x2, view->region.y2);
	}

	return 0;
}

int view_set_x(uint8_t view_id, int16_t x)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	if (!view)
		return -ESRCH;

	return _view_ctx_set_position(view, x, view->region.y1);
}

int view_set_y(uint8_t view_id, int16_t y)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	if (!view)
		return -ESRCH;

	return _view_ctx_set_position(view, view->region.x1, y);
}

int view_set_pos(uint8_t view_id, int16_t x, int16_t y)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	if (!view)
		return -ESRCH;

	return _view_ctx_set_position(view, x, y);
}

int view_set_drag_attribute(uint8_t view_id, uint8_t drag_attribute)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	ui_view_context_t *view = _view_manager_get_view_context(view_id);
	int16_t x = 0;
	int16_t y = 0;

	if (!view)
		return -ESRCH;

	switch(drag_attribute) {
	case UI_DRAG_DROPDOWN:
	case UI_DRAG_MOVEDOWN:
		y = view_manager->disp_region.y1 - ui_region_get_height(&view->region);
		break;
	case UI_DRAG_DROPUP:
	case UI_DRAG_MOVEUP:
		y = view_manager->disp_region.y2 + 1;
		break;
	case UI_DRAG_DROPRIGHT:
	case UI_DRAG_MOVERIGHT:
		x = view_manager->disp_region.x1 - ui_region_get_width(&view->region);
		break;
	case UI_DRAG_DROPLEFT:
	case UI_DRAG_MOVELEFT:
		x = view_manager->disp_region.x2 + 1;
		break;
	default:
		x = view->region.x1;
		y = view->region.y1;
		break;
	}

	view->drag_attr = drag_attribute;

	view_set_pos(view_id, x, y);

	if (drag_attribute) {
		view_set_hidden(view_id, false);
	}

	SYS_LOG_INF("view %u drag 0x%x\n", view_id, drag_attribute);

	return 0;
}

uint8_t view_get_drag_attribute(uint8_t view_id)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	return (view ? view->drag_attr : 0);
}

bool view_has_move_attribute(uint8_t view_id)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	return (view ? view->drag_attr >= UI_DRAG_MOVEDOWN : false);
}

int view_set_drag_anim_cb(uint8_t view_id, ui_view_drag_anim_cb_t drag_cb)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	if (!view)
		return -ESRCH;

	view->drag_anim_cb = drag_cb;
	return 0;
}

int view_set_hidden(uint8_t view_id, bool hidden)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	if (!view)
		return -ESRCH;

	if (hidden == !!(view->flags & UI_FLAG_HIDDEN))
		return 0;

	os_strace_u32x3(SYS_TRACE_ID_VIEW_SETPROP, view_id, UI_FLAG_HIDDEN, hidden);

	if (hidden) {
		view->flags |= UI_FLAG_HIDDEN;
	} else {
		view->flags &= (~UI_FLAG_HIDDEN);
	}

	view_manager_refocus(view);

	/* the whole view becomes dirty */
	if (view->flags & UI_FLAG_INFLATED) {
		os_mutex_lock(&view_manager->post_mutex, OS_FOREVER);
		if (_view_manager_add_dirty_region(&view->region))
			view_manager_set_dirty(true);
		os_mutex_unlock(&view_manager->post_mutex);
	}

	os_strace_end_call_u32(SYS_TRACE_ID_VIEW_SETPROP, view_id);

	return 0;
}

bool view_is_hidden(uint8_t view_id)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	return (view && !(view->flags & UI_FLAG_HIDDEN)) ? false : true;
}

static bool _view_ctx_is_visible(ui_view_context_t *view)
{
	view_manager_context_t *view_manager = _view_manager_get_context();

	if (view->flags & (UI_FLAG_HIDDEN | UI_FLAG_PAUSED)) {
		return false;
	}

	if (!ui_region_is_on(&view_manager->disp_region, &view->region)) {
		return false;
	}

	return true;
}

bool view_is_visible(uint8_t view_id)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	return _view_ctx_is_visible(view);
}

static int _view_ctx_focus(ui_view_context_t *view, bool focus, bool pre_anim)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	bool old_focus = (view->flags & UI_FLAG_FOCUSED) ? true : false;
	uint8_t view_id = view->entry->id;
	int ret = 0;

	if (focus != old_focus) {
		SYS_LOG_INF("view %d focus %d\n", view_id, focus);

		os_strace_u32x3(SYS_TRACE_ID_VIEW_SETPROP, view_id, UI_FLAG_FOCUSED, focus);

		if (focus) {
			ret = view_gui_set_focus(view);
			if (ret) {
				SYS_LOG_ERR("view %d gui focus failed\n", view_id);
				return ret;
			}

			assert((view->flags & UI_FLAG_PAUSED) == 0);
			view->flags |= UI_FLAG_FOCUSED;
		} else {
			view->flags &= (~UI_FLAG_FOCUSED);
		}

		ui_service_send_self_msg(view_id, focus ? MSG_VIEW_FOCUS : MSG_VIEW_DEFOCUS);

		if (!pre_anim && view_manager->notify_cb) {
			os_strace_u32x2(SYS_TRACE_ID_VIEW_NOTIFY_CB, view_id, focus ? MSG_VIEW_FOCUS : MSG_VIEW_DEFOCUS);
			view_manager->notify_cb(view_id, focus ? MSG_VIEW_FOCUS : MSG_VIEW_DEFOCUS);
			os_strace_end_call_u32(SYS_TRACE_ID_VIEW_NOTIFY_CB, view_id);
		}

		os_strace_end_call_u32(SYS_TRACE_ID_VIEW_SETPROP, view_id);
	}

	return 0;
}

bool view_is_focused(uint8_t view_id)
{
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	return (view && (view->flags & UI_FLAG_FOCUSED)) ? true : false;
}

int16_t view_manager_get_focused_view(void)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	ui_view_context_t *view = view_manager->focused_view;

	return view ? view->entry->id : -1;
}

void view_manager_refocus(ui_view_context_t *reason_view)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	ui_view_context_t *view;
#ifdef CONFIG_SIMULATOR
	sys_snode_t* node;
#endif
	ui_view_context_t *focused_view = NULL;

	if (reason_view && reason_view != view_manager->focused_view) {
		uint16_t focus_order = view_manager->focused_view ? view_manager->focused_view->order : 0xFFFF;
		if (!_view_ctx_is_visible(reason_view) || reason_view->order >= focus_order) {
			return;
		}

		if (view_manager->focused_view)
			_view_ctx_focus(view_manager->focused_view, false, false);

		focused_view = reason_view;
	} else {
#ifdef CONFIG_SIMULATOR
		SYS_SLIST_FOR_EACH_NODE_SAFE(&view_manager->view_list, view, node) {
#else
		SYS_SLIST_FOR_EACH_CONTAINER(&view_manager->view_list, view, node) {
#endif
			if (focused_view == NULL && _view_ctx_is_visible(view)) {
				focused_view = view;
			} else {
				_view_ctx_focus(view, false, false);
			}
		}
	}

	if (focused_view) {
		_view_ctx_focus(focused_view, true, false);
		ui_service_set_view_origin(focused_view->region.x1, focused_view->region.y1);
	}

	view_manager->focused_view = focused_view;
}

void view_manager_pre_anim_refocus(uint8_t view_id)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	ui_view_context_t *new_focus_view = _view_manager_get_view_context(view_id);

	assert(new_focus_view != NULL);

	if (view_manager->focused_view != new_focus_view) {
		if (view_manager->focused_view)
			_view_ctx_focus(view_manager->focused_view, false, true);

		_view_ctx_focus(new_focus_view, true, true);
		view_manager->pre_focus_view = new_focus_view;
	}
}

void view_manager_post_anim_refocus(void)
{
	view_manager_context_t *view_manager = _view_manager_get_context();

	if (view_manager->pre_focus_view) {
		if (view_manager->notify_cb) {
			if (view_manager->focused_view)
				view_manager->notify_cb(view_manager->focused_view->entry->id, MSG_VIEW_DEFOCUS);

			view_manager->notify_cb(view_manager->pre_focus_view->entry->id, MSG_VIEW_FOCUS);
		}

		view_manager->focused_view = view_manager->pre_focus_view;
		view_manager->pre_focus_view = NULL;

		ui_service_set_view_origin(view_manager->focused_view->region.x1, view_manager->focused_view->region.y1);
	} else {
		view_manager_refocus(NULL);

#if 0
		/* long view may pause drawing during sliding due to double buffer
		 * allocation failure. when sliding completed, if the same long
		 * view focused, should resume drawing for it.
		 */
		if (view_manager->focused_view) {
			view_gui_resume_draw(view_manager->focused_view, false);
		}
#endif
	}
}

int16_t view_manager_get_draggable_view(uint8_t gesture, bool *towards_screen)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	ui_view_context_t *view;
#ifdef CONFIG_SIMULATOR
	sys_snode_t* node;
#endif
	uint8_t match_condition1 = 0;
	uint8_t match_condition2 = 0;
	uint8_t match_condition3 = 0;

	switch (gesture) {
	case GESTURE_DROP_DOWN:
		match_condition1 = UI_DRAG_DROPDOWN;
		match_condition2 = UI_DRAG_DROPUP; /* dragged away from screen */
		match_condition3 = UI_DRAG_MOVEDOWN;
		break;
	case GESTURE_DROP_UP:
		match_condition1 = UI_DRAG_DROPUP;
		match_condition2 = UI_DRAG_DROPDOWN; /* dragged away from screen */
		match_condition3 = UI_DRAG_MOVEUP;
		break;
	case GESTURE_DROP_RIGHT:
		match_condition1 = UI_DRAG_DROPRIGHT;
		match_condition2 = UI_DRAG_DROPLEFT; /* dragged away from screen */
		match_condition3 = UI_DRAG_MOVERIGHT;
		break;
	case GESTURE_DROP_LEFT:
		match_condition1 = UI_DRAG_DROPLEFT;
		match_condition2 = UI_DRAG_DROPRIGHT; /* dragged away from screen */
		match_condition3 = UI_DRAG_MOVELEFT;
		break;
	default:
		return -EINVAL;
	}
#ifdef CONFIG_SIMULATOR
	SYS_SLIST_FOR_EACH_NODE_SAFE(&view_manager->view_list, view, node) { /* drop backward */
#else
	SYS_SLIST_FOR_EACH_CONTAINER(&view_manager->view_list, view, node) { /* drop backward */
#endif
		if (((view->drag_attr & match_condition2) && _view_ctx_is_visible(view))) {
			if (towards_screen) {
				*towards_screen = false;
			}
			return view->entry->id;
		}
	}
#ifdef CONFIG_SIMULATOR
	SYS_SLIST_FOR_EACH_NODE_SAFE(&view_manager->view_list, view, node) { /* drop forward */
#else
	SYS_SLIST_FOR_EACH_CONTAINER(&view_manager->view_list, view, node) { /* drop forward */
#endif
		if (((view->drag_attr & match_condition1) && !(view->flags & (UI_FLAG_HIDDEN | UI_FLAG_PAUSED)) &&
			!ui_region_is_on(&view_manager->disp_region, &view->region))) {
			if (towards_screen) {
				*towards_screen = true;
			}
			return view->entry->id;
		}
	}
#ifdef CONFIG_SIMULATOR
	SYS_SLIST_FOR_EACH_NODE_SAFE(&view_manager->view_list, view, node) { /* move */
#else
	SYS_SLIST_FOR_EACH_CONTAINER(&view_manager->view_list, view, node) { /* move */
#endif
		if (((view->drag_attr & match_condition3) && !(view->flags & (UI_FLAG_HIDDEN | UI_FLAG_PAUSED))/* &&
			!ui_region_is_on(&view_manager->disp_region, &view->region)*/)) {
			if (towards_screen) {
				*towards_screen = true;
			}
			return view->entry->id;
		}
	}

	return -1;
}

int view_manager_get_slide_animation_points(uint8_t view_id,
		ui_point_t *start, ui_point_t *stop, uint8_t animation_type)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	if (!view)
		return -ESRCH;

	ui_point_set(start, view->region.x1, view->region.y1);
	ui_point_set(stop, view->region.x1, view->region.y1);

	switch (animation_type) {
	case UI_ANIM_SLIDE_IN_DOWN:
		stop->y = view_manager->disp_region.y1;
		break;
	case UI_ANIM_SLIDE_IN_UP:
		stop->y = view_manager->disp_region.y1;
		break;
	case UI_ANIM_SLIDE_OUT_UP:
		stop->y = view_manager->disp_region.y1 - ui_region_get_height(&view_manager->disp_region);
		break;
	case UI_ANIM_SLIDE_OUT_DOWN:
		stop->y = view_manager->disp_region.y1 + ui_region_get_height(&view_manager->disp_region);
		break;
	case UI_ANIM_SLIDE_IN_RIGHT:
		stop->x = view_manager->disp_region.x1;
		break;
	case UI_ANIM_SLIDE_IN_LEFT:
		stop->x = view_manager->disp_region.x1;
		break;
	case UI_ANIM_SLIDE_OUT_LEFT:
		stop->x = view_manager->disp_region.x1 - ui_region_get_width(&view_manager->disp_region);
		break;
	case UI_ANIM_SLIDE_OUT_RIGHT:
		stop->x = view_manager->disp_region.x1 + ui_region_get_width(&view_manager->disp_region);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int view_manager_slide_animation_start(uint8_t view_id, int16_t last_view_id,
		uint8_t animation_type, ui_view_anim_cfg_t *cfg)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	ui_view_context_t *view = _view_manager_get_view_context(view_id);
	ui_point_t last_offset = { 0, 0 };

	if (!view)
		return -ESRCH;

	//ui_point_set(&cfg->start, view->region.x1, view->region.y1);
	//ui_point_set(&cfg->stop, 0, 0);

	switch (animation_type) {
	case UI_ANIM_SLIDE_IN_DOWN:
		//cfg->stop.y = view_manager->disp_region.y1;
		last_offset.y = view_manager_get_disp_yres();
		break;
	case UI_ANIM_SLIDE_IN_UP:
		//cfg->stop.y = view_manager->disp_region.y1;
		last_offset.y = -view_manager_get_disp_yres();
		break;
	case UI_ANIM_SLIDE_OUT_UP:
		//cfg->stop.y = view_manager->disp_region.y1 - ui_region_get_height(&view_manager->disp_region);
		last_offset.y = view_manager_get_disp_yres();
		break;
	case UI_ANIM_SLIDE_OUT_DOWN:
		//cfg->stop.y = view_manager->disp_region.y1 + ui_region_get_height(&view_manager->disp_region);
		last_offset.y = -view_manager_get_disp_yres();
		break;
	case UI_ANIM_SLIDE_IN_RIGHT:
		//cfg->stop.x = view_manager->disp_region.x1;
		last_offset.x = view_manager_get_disp_xres();
		break;
	case UI_ANIM_SLIDE_IN_LEFT:
		//cfg->stop.x = view_manager->disp_region.x1;
		last_offset.x = -view_manager_get_disp_xres();
		break;
	case UI_ANIM_SLIDE_OUT_LEFT:
		//cfg->stop.x = view_manager->disp_region.x1 - ui_region_get_width(&view_manager->disp_region);
		last_offset.x = view_manager_get_disp_xres();
		break;
	case UI_ANIM_SLIDE_OUT_RIGHT:
		//cfg->stop.x = view_manager->disp_region.x1 + ui_region_get_width(&view_manager->disp_region);
		last_offset.x = -view_manager_get_disp_xres();
		break;
	default:
		return -EINVAL;
	}

	if (view_animation_start(&view_manager->animation, view_id, last_view_id, &last_offset, cfg, true)) {
		if (cfg->stop.x != cfg->start.x || cfg->stop.y != cfg->start.y) {
			view_set_pos(view_id, cfg->stop.x, cfg->stop.y);
			if (last_view_id >= 0) {
				view_set_pos(last_view_id, cfg->stop.x + last_offset.x, cfg->stop.y+ last_offset.y);
			}
		}

		/* unlock forbid draw ui */
		ui_service_unlock_gui(UISRV_GUI_BOTH);
		return -EINVAL;
	}

	return 0;
}

int view_manager_drag_animation_start(uint8_t view_id, input_dev_runtime_t *runtime)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	ui_view_context_t *view = _view_manager_get_view_context(view_id);

	if (!view)
		return -ESRCH;

	/* Reset the origin here in case the user donot start the real animation */
	ui_service_set_view_origin(view->region.x1, view->region.y1);
	/* Unlock forbid draw ui in case the user donot start the real animation */
	ui_service_unlock_gui(UISRV_GUI_BOTH);

	if (view->drag_anim_cb) {
		ui_view_anim_cfg_t cfg;
		ui_point_t scroll_throw_vect;
		ui_region_t stop_region;

		memset(&cfg, 0, sizeof(cfg));
		cfg.start.x = view->region.x1;
		cfg.start.y = view->region.y1;
		cfg.stop = cfg.start;
		scroll_throw_vect.x = runtime->types.pointer.scroll_throw_vect.x;
		scroll_throw_vect.y = runtime->types.pointer.scroll_throw_vect.y;

		view->drag_anim_cb(view_id, &scroll_throw_vect, &cfg);
		if (cfg.duration > 0) {
			ui_region_copy(&stop_region, &view->region);
			ui_region_set_pos(&stop_region, cfg.stop.x, cfg.stop.y);
			ui_region_fit_in(&stop_region, &view_manager->disp_region);
			ui_point_set(&cfg.stop, stop_region.x1, stop_region.y1);

			if (cfg.stop.x == cfg.start.x && cfg.stop.y == cfg.start.y) {
				return 0;
			}

			ui_service_lock_gui(UISRV_GUI_INPUT);

			if (view_animation_start(&view_manager->animation, view_id, -1, NULL, &cfg, false)) {
				ui_service_unlock_gui(UISRV_GUI_INPUT);
				return -EBUSY;
			}
		}
	}

	return 0;
}

int view_manager_animation_process(void)
{
	view_manager_context_t *view_manager = _view_manager_get_context();

	view_animation_process(&view_manager->animation);
	return 0;
}

int view_manager_animation_stop(bool forced)
{
	view_manager_context_t *view_manager = _view_manager_get_context();

	view_animation_stop(&view_manager->animation, forced);
	return 0;
}

uint8_t view_manager_animation_get_state(void)
{
	view_manager_context_t *view_manager = _view_manager_get_context();

	return view_manager->animation.state;
}

static void _view_post_cleanup(ui_view_context_t *view)
{
	view_gui_post_cleanup(view->data.surface);
}

static void _view_surface_post_cleanup(void *surface)
{
	view_gui_post_cleanup(surface);
}

static bool _view_get_crop_and_frame(ui_view_context_t *view, ui_layer_t *layer)
{
	view_manager_context_t *view_manager = _view_manager_get_context();

	if (view->flags & (UI_FLAG_HIDDEN | UI_FLAG_PAUSED))
		return false;

	if (!(view->flags & UI_FLAG_PAINTED) && !(view->refr_flags & UI_REFR_FLAG_MOVED))
		return false;

	if (ui_region_intersect(&layer->frame, &view_manager->dirty_region, &view->region) == false) {
		return false;
	}

	layer->buffer = surface_get_frontbuffer(view->data.surface);
	if (layer->buffer == NULL) {
		return false;
	}

	if (surface_get_swap_mode(view->data.surface) == SURFACE_SWAP_ZERO_BUF) {
		if (ui_region_get_width(&layer->frame) != graphic_buffer_get_width(layer->buffer) ||
			ui_region_get_height(&layer->frame) != graphic_buffer_get_height(layer->buffer))
			return false;

		layer->buf_resident = 1;
		ui_region_set(&layer->crop, 0, 0, graphic_buffer_get_width(layer->buffer) - 1,
			graphic_buffer_get_height(layer->buffer) - 1);
	} else {
		layer->buf_resident = 0;
		ui_region_set(&layer->crop,
			layer->frame.x1 - view->region.x1, layer->frame.y1 - view->region.y1,
			layer->frame.x2 - view->region.x1, layer->frame.y2 - view->region.y1);
	}

	return true;
}

int view_manager_post_display(void)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	ui_layer_t layer[2];
	ui_layer_t *current_layer = &layer[1];
	uint32_t post_flags = POST_FULL_SCREEN_OPT;//FIRST_POST_IN_FRAME | LAST_POST_IN_FRAME;
	uint8_t layer_num = 0;
	ui_view_context_t *view, *top_view = NULL;
#ifdef CONFIG_SIMULATOR
	sys_snode_t* node;
#endif

	os_mutex_lock(&view_manager->post_mutex, OS_FOREVER);

#ifndef CONFIG_TP_TRAIL_DRAW
	if (!view_manager->dirty) {
		os_mutex_unlock(&view_manager->post_mutex);
		return 0;
	}
#endif

	if (view_manager->mark_all_dirty) {
		post_flags |= FIRST_POST_IN_FRAME | LAST_POST_IN_FRAME;
		view_manager->mark_all_dirty = 0;
	}

	/* round the dirty region. */
	display_composer_round(&view_manager->dirty_region);

	SYS_LOG_DBG("dirty region (%d %d %d %d)\n",
		view_manager->dirty_region.x1, view_manager->dirty_region.y1,
		view_manager->dirty_region.x2, view_manager->dirty_region.y2);

#ifdef CONFIG_SIMULATOR
	SYS_SLIST_FOR_EACH_NODE_SAFE(&view_manager->view_list, view, node) {
#else
	SYS_SLIST_FOR_EACH_CONTAINER(&view_manager->view_list, view, node) {
#endif
		if ((layer_num < 2) && _view_get_crop_and_frame(view, current_layer)) {
			current_layer->cleanup_cb = (view->refr_flags & UI_REFR_FLAG_CHANGED) ?
					_view_surface_post_cleanup : NULL;
			current_layer->cleanup_data = view->data.surface;
			current_layer->blending = DISPLAY_BLENDING_NONE;

			if (view->refr_flags & (UI_REFR_FLAG_FIRST_CHANGED | UI_REFR_FLAG_MOVED))
				post_flags |= FIRST_POST_IN_FRAME;
			if (view->refr_flags & (UI_REFR_FLAG_LAST_CHANGED | UI_REFR_FLAG_MOVED))
				post_flags |= LAST_POST_IN_FRAME;

			os_strace_u32x6(SYS_TRACE_ID_VIEW_POST, view_manager->post_id,
					view->entry->id, current_layer->frame.x1, current_layer->frame.y1,
					current_layer->frame.x2, current_layer->frame.y2);

			if (layer_num > 0) {
				/* apply blending to drop down/up/left/right */
				if (ui_region_is_on(&current_layer->frame, &layer[1].frame) == true) {
					uint16_t distance = 0, max_distance = 0;

					if (top_view->region.y1 < view_manager->disp_region.y1) {
						distance = top_view->region.y2 - view_manager->disp_region.y1;
						max_distance = top_view->region.y2 - top_view->region.y1;
					} else if (top_view->region.y2 > view_manager->disp_region.y2) {
						distance = view_manager->disp_region.y2 - top_view->region.y1;
						max_distance = top_view->region.y2 - top_view->region.y1;
					} else if (top_view->region.x1 < view_manager->disp_region.x1) {
						distance = top_view->region.x2 - view_manager->disp_region.x1;
						max_distance = top_view->region.x2 - top_view->region.x1;
					} else if (top_view->region.x2 > view_manager->disp_region.x2) {
						distance = view_manager->disp_region.x2 - top_view->region.x1;
						max_distance = top_view->region.x2 - top_view->region.x1;
					}

					if (distance < max_distance) {
						/* FIXME: use fixed alpha to avoide the bad hardware blending result on RGB-565 */
						if (graphic_buffer_get_pixel_format(layer[1].buffer) == PIXEL_FORMAT_ARGB_8888) {
							layer[1].color.a = 255 * distance / max_distance;
						} else {
							layer[1].color.a = 128;
						}

						layer[1].blending = DISPLAY_BLENDING_COVERAGE;
					} else {
						/* Skip the background layer full-covered by foreground layer */
						if (view->refr_flags & UI_REFR_FLAG_CHANGED) {
							_view_post_cleanup(view);
							view->refr_flags = 0;
						}
						continue;
					}
				}
			} else {
				top_view = view;
			}

		#ifdef CONFIG_TP_TRAIL_DRAW
			if (layer_num > 0) {
				void *vaddr = NULL;
				if (0 == graphic_buffer_lock(current_layer->buffer, GRAPHIC_BUFFER_SW_WRITE, NULL, &vaddr)) {
					input_dispatcher_draw_trail((int)vaddr,
						graphic_buffer_get_width(current_layer->buffer),
						graphic_buffer_get_height(current_layer->buffer),
						graphic_buffer_get_stride(current_layer->buffer),
						display_format_get_bits_per_pixel(
							graphic_buffer_get_pixel_format(current_layer->buffer)));
					graphic_buffer_unlock(current_layer->buffer);
				}
			}
		#endif

			SYS_LOG_DBG("[%d]view %d buf %p crop (%d %d %d %d) frame (%d %d %d %d)\n",
				layer_num, view->entry->id, current_layer->buffer,
				current_layer->crop.x1, current_layer->crop.y1,
				current_layer->crop.x2, current_layer->crop.y2,
				current_layer->frame.x1, current_layer->frame.y1,
				current_layer->frame.x2, current_layer->frame.y2);

			current_layer--;
			layer_num++;
		} else {
			/* make sure GUI donot block */
			if (view->refr_flags & UI_REFR_FLAG_CHANGED) {
				_view_post_cleanup(view);
			}
		}

		/* clear flags */
		view->refr_flags = 0;
	}

	if (layer_num > 0) {
		/* add a bg layer if the single layer donot cover the whole dirty region */
		if (layer_num == 1 &&
				(layer[1].frame.x1 > view_manager->dirty_region.x1 ||
				 layer[1].frame.x2 < view_manager->dirty_region.x2 ||
				 layer[1].frame.y1 > view_manager->dirty_region.y1 ||
				 layer[1].frame.y2 < view_manager->dirty_region.y2)) {
			layer[0].buffer = NULL;
			layer[0].color.full = 0; /* black */
			layer[0].cleanup_cb = NULL;
			layer[0].blending = DISPLAY_BLENDING_NONE;
			ui_region_copy(&layer[0].frame, &view_manager->dirty_region);
			layer_num++;

			SYS_LOG_DBG("[1]bg frame (%d %d %d %d)\n", layer[0].frame.x1,
				layer[0].frame.y1, layer[0].frame.x2, layer[0].frame.y2);
		}

		if (post_flags & LAST_POST_IN_FRAME) {
			view_manager->post_id++;
		}
	}

	if (view_manager->dirty) {
		view_manager_set_dirty(false);
		ui_region_set(&view_manager->dirty_region,
				view_manager->disp_region.x2, view_manager->disp_region.y2,
				view_manager->disp_region.x1, view_manager->disp_region.y1);
	}

	os_mutex_unlock(&view_manager->post_mutex);

	if (layer_num > 0) { /* post may be blocked */
		display_composer_post(&layer[2 - layer_num], layer_num, post_flags);
	}

	return 0;
}

void view_manager_resume_display(void)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	ui_view_context_t *view = view_manager->focused_view;

	if (view && (view->flags & UI_FLAG_INFLATED)) {
		view_gui_resume_draw(view, true);
		_view_proc_callback(view, MSG_VIEW_PAINT, NULL);
	}
}

#ifdef CONFIG_UISRV_VIEW_PAUSED_SNAPSHOT
static void _compress_thread_loop(void *p1, void *p2, void *p3)
{
	view_manager_context_t *view_manager = p1;

	while (1) {
		uint8_t view_id;
		graphic_buffer_t *buf;

		os_sem_take(&view_manager->compress_busy_sem, OS_FOREVER);
		buf = view_manager->compress_entries[view_manager->compress_busy_idx].buffer;
		view_id = view_manager->compress_entries[view_manager->compress_busy_idx].view_id;
		if (++view_manager->compress_busy_idx >= NUM_COMPRESS_ENTRIES)
			view_manager->compress_busy_idx = 0;
		os_sem_give(&view_manager->compress_free_sem);

		uint32_t buf_size = graphic_buffer_get_stride(buf) * graphic_buffer_get_height(buf) * 2;
		void *snapshot = ui_memory_alloc(buf_size);

		if (snapshot) {
			const void * bufptr = graphic_buffer_get_bufptr(buf, 0, 0);

			os_strace_u32(SYS_TRACE_ID_VIEW_COMPRESS, view_id);
			mem_dcache_invalidate(bufptr, buf_size);
			mem_dcache_sync();
			unsigned int idx_len = spr_compress_index(bufptr, snapshot, buf_size);
			unsigned int dat_len = spr_compress_data(bufptr, snapshot, buf_size);
			os_strace_end_call_u32(SYS_TRACE_ID_VIEW_COMPRESS, idx_len + dat_len);

			SYS_LOG_INF("view %d compress size: (idx %u, data %u), ratio %u\n",
					view_id, idx_len, dat_len, dat_len * 100 / buf_size);

			/* In case that view deleted */
			os_sched_lock();

			ui_view_context_t *view = _view_manager_get_view_context(view_id);
			if (view) {
				view->snapshot = snapshot;

#if 0
				if (view_manager->notify_cb) {
					os_strace_u32x2(SYS_TRACE_ID_VIEW_NOTIFY_CB, view_id, MSG_VIEW_PAUSE);
					view_manager->notify_cb(view_id, MSG_VIEW_PAUSE);
					os_strace_end_call_u32(SYS_TRACE_ID_VIEW_NOTIFY_CB, view_id);
				}
#endif
			} else {
				ui_memory_free(snapshot);
			}

			os_sched_unlock();
		}

		graphic_buffer_unref(buf);
	}
}
#endif /* CONFIG_UISRV_VIEW_PAUSED_SNAPSHOT */

int view_manager_init(void)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	uint16_t x_res, y_res;

	if (display_composer_get_geometry(&x_res, &y_res, NULL)) {
		SYS_LOG_ERR("%s: cannot get display resolution", __func__);
		return -ENODEV;
	}

	memset(view_manager, 0,  sizeof(view_manager_context_t));

	os_mutex_init(&view_manager->post_mutex);
	sys_slist_init(&view_manager->view_list);
	ui_region_set(&view_manager->disp_region, 0, 0, x_res - 1, y_res - 1);
	ui_region_set(&view_manager->dirty_region, x_res - 1, y_res - 1, 0, 0);

#ifdef CONFIG_UISRV_VIEW_PAUSED_SNAPSHOT
	os_sem_init(&view_manager->compress_busy_sem, 0, NUM_COMPRESS_ENTRIES);
	os_sem_init(&view_manager->compress_free_sem, NUM_COMPRESS_ENTRIES, NUM_COMPRESS_ENTRIES);
	os_thread_create(compress_thread_stack, sizeof(compress_thread_stack),
			_compress_thread_loop, view_manager, NULL, NULL,
			CONFIG_LVGL_RES_PRELOAD_PRIORITY + 1, 0, 0);
#endif

	return 0;
}

int16_t view_manager_get_disp_xres(void)
{
	view_manager_context_t *view_manager = _view_manager_get_context();

	return ui_region_get_width(&view_manager->disp_region);
}

int16_t view_manager_get_disp_yres(void)
{
	view_manager_context_t *view_manager = _view_manager_get_context();

	return ui_region_get_height(&view_manager->disp_region);
}

int view_manager_set_callback(uint8_t id, void * callback)
{
	view_manager_context_t *view_manager = _view_manager_get_context();

	switch (id) {
	case UI_CB_NOTIFY:
		view_manager->notify_cb = callback;
		break;
	case UI_CB_MSGBOX:
		view_manager->msgbox_popup_cb = callback;
		break;
	case UI_CB_KEYEVENT:
		view_manager->keyevent_cb = callback;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int view_manager_popup_msgbox(uint8_t msgbox_id)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	int16_t focus_id;

	if (view_manager->msgbox_popup_cb) {
		/* stop animation first */
		os_sched_lock();
		gesture_manager_stop_process(input_dispatcher_get_pointer_dev(), true);

		focus_id = view_manager_get_focused_view();
		if (focus_id >= 0) {
			gesture_manager_set_enabled(false);
		}
		os_sched_unlock();

		view_manager->msgbox_popup_cb(msgbox_id, false, focus_id);
	}

	return 0;
}

int view_manager_close_msgbox(uint8_t msgbox_id, int16_t attached_view_id)
{
	view_manager_context_t *view_manager = _view_manager_get_context();

	if (view_manager->msgbox_popup_cb) {
		view_manager->msgbox_popup_cb(msgbox_id, true, attached_view_id);
	}

	return 0;
}

void view_manager_dispatch_key_event(uint32_t event)
{
	view_manager_context_t *view_manager = _view_manager_get_context();

	if (view_manager->keyevent_cb) {
		view_manager_context_t *view_manager = _view_manager_get_context();
		ui_view_context_t *view = view_manager->focused_view;

		view_manager->keyevent_cb(view ? view->entry->id : UI_VIEW_INV_ID, event);
	}
}

uint32_t view_manager_get_post_id(void)
{
	view_manager_context_t *view_manager = _view_manager_get_context();

	return view_manager->post_id;
}

void view_manager_dump(void)
{
	view_manager_context_t *view_manager = _view_manager_get_context();
	ui_view_context_t *view;
#ifdef CONFIG_SIMULATOR
	sys_snode_t* node;
#endif

	os_printk("  id |  handle  | format | order | flag | drag |      region       \n"
	          "-----+----------+--------+-------+------+------+-------------------\n");
#ifdef CONFIG_SIMULATOR
	SYS_SLIST_FOR_EACH_NODE_SAFE(&view_manager->view_list, view, node) {
#else
	SYS_SLIST_FOR_EACH_CONTAINER(&view_manager->view_list, view, node) {
#endif
		graphic_buffer_t *fb = surface_get_frontbuffer(view->data.surface);

		os_printk("%c %02x | %08x |   %02x   |  %02x   | %04x |  %02x  |%4d,%4d,%4d,%4d\n",
				(view->flags & UI_FLAG_FOCUSED) ? '*' : ' ',
				view->entry->id, fb ? (uint32_t)graphic_buffer_get_bufptr(fb, 0, 0) : 0,
				surface_get_pixel_format(view->data.surface), view->order, view->flags,
				view->drag_attr, view->region.x1, view->region.y1,
				view->region.x2, view->region.y2);
	}

	os_printk("\n");
}
