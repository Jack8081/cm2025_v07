/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file view stack interface
 */

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "view_stack"
#include <os_common_api.h>
#include <ui_manager.h>
#include <string.h>
#include <assert.h>
#include <view_stack.h>
#ifdef CONFIG_SIMULATOR
#include <simulator_config.h>
#endif

typedef struct {
	/* view cache descripter */
	const view_cache_dsc_t *cache;
	/* view group descripter */
	const view_group_dsc_t *group;
	/* presenter of the view, used only when cache == NULL */
	const void *presenter;
	/* init focused view id */
	uint8_t id;
} view_stack_data_t;

typedef struct {
	view_stack_data_t data[CONFIG_VIEW_STACK_LEVEL];
	uint8_t num;
	uint8_t init_view;
	uint8_t inited : 1;
} view_stack_ctx_t;

static view_stack_ctx_t view_stack;
static OS_MUTEX_DEFINE(mutex);

static int _view_stack_push(const view_stack_data_t *data);
static int _view_stack_jump(const view_stack_data_t *old_data,
		const view_stack_data_t *new_data);

int view_stack_init(void)
{
	int res = 0;

	os_mutex_lock(&mutex, OS_FOREVER);

	if (view_stack.inited) {
		res = -EALREADY;
		goto out_unlock;
	}

	view_stack.inited = 1;

out_unlock:
	os_mutex_unlock(&mutex);
	return res;
}

void view_stack_deinit(void)
{
	os_mutex_lock(&mutex, OS_FOREVER);

	view_stack.inited = 0;
	view_stack_clean();

	os_mutex_unlock(&mutex);
}

uint8_t view_stack_get_num(void)
{
	return view_stack.num;
}

int16_t view_stack_get_top(void)
{
	view_stack_data_t * top;
	int16_t focused = -1;

	os_mutex_lock(&mutex, OS_FOREVER);

	if (view_stack.num == 0) {
		goto out_unlock;
	}

	top = &view_stack.data[view_stack.num - 1];
	if (top->cache) {
		focused = view_cache_get_focus_view();
	} else {
		focused = top->id;
	}

out_unlock:
	os_mutex_unlock(&mutex);
	return focused;
}

void view_stack_clean(void)
{
	os_mutex_lock(&mutex, OS_FOREVER);

	if (view_stack.num > 0) {
		_view_stack_jump(&view_stack.data[view_stack.num - 1], NULL);
		view_stack.num = 0;
	}

	os_mutex_unlock(&mutex);
}

int view_stack_pop_until_first(void)
{
	int res = -ENODATA;

	os_mutex_lock(&mutex, OS_FOREVER);

	if (view_stack.num <= 1) {
		goto out_unlock;
	}

	if (view_stack.data[0].cache) {
		view_stack.data[0].id = view_stack.init_view;
	}

	res = _view_stack_jump(&view_stack.data[view_stack.num - 1], &view_stack.data[0]);
	view_stack.num = 1;

out_unlock:
	os_mutex_unlock(&mutex);
	return res;
}

int view_stack_pop(void)
{
	int res = -ENODATA;

	os_mutex_lock(&mutex, OS_FOREVER);

	if (view_stack.num <= 1) {
		goto out_unlock;
	}

	res = _view_stack_jump(&view_stack.data[view_stack.num - 1],
					&view_stack.data[view_stack.num - 2]);
	view_stack.num--;

out_unlock:
	os_mutex_unlock(&mutex);
	return res;
}

int view_stack_push_cache(const view_cache_dsc_t *dsc, uint8_t view_id)
{
	view_stack_data_t data = {
		.cache = dsc,
		.group = NULL,
		.presenter = NULL,
		.id = view_id,
	};

	return _view_stack_push(&data);
}

int view_stack_push_group(const view_group_dsc_t *dsc)
{
	view_stack_data_t data = {
		.cache = NULL,
		.group = dsc,
		.presenter = NULL,
		.id = dsc->vlist[dsc->idx],
	};

	return _view_stack_push(&data);
}

int view_stack_push_view(uint8_t view_id, const void *presenter)
{
	view_stack_data_t data = {
		.cache = NULL,
		.group = NULL,
		.presenter = presenter,
		.id = view_id,
	};

	return _view_stack_push(&data);
}

void view_stack_dump(void)
{
	uint8_t i;
	int8_t idx;

	os_mutex_lock(&mutex, OS_FOREVER);

	os_printk("view stack:\n", view_stack.num);

	for (idx = view_stack.num - 1; idx >= 0; idx--) {
		if (view_stack.data[idx].cache) {
			os_printk("[%d] cache:\n", idx);

			os_printk("\t main:", view_stack.data[idx].cache->num);
			for (i = 0; i < view_stack.data[idx].cache->num; i++) {
				os_printk(" %d", view_stack.data[idx].cache->vlist[i]);
			}

			os_printk("\n\t cross: %d %d\n", view_stack.data[idx].cache->cross_vlist[0],
					view_stack.data[idx].cache->cross_vlist[1]);
		} else if (view_stack.data[idx].group) {
			os_printk("[%d] group:", idx, view_stack.data[idx].group->num);
			for (i = 0; i < view_stack.data[idx].group->num; i++) {
				os_printk(" %d", i, view_stack.data[idx].group->vlist[i]);
			}
			os_printk("\n");
		} else {
			os_printk("[%d] view: %d\n", idx, view_stack.data[idx].id);
		}
	}

	os_printk("\n");

	os_mutex_unlock(&mutex);
}

static int _view_stack_push(const view_stack_data_t *data)
{
	view_stack_data_t *old_data = NULL;
	int16_t focused_id = -1;
	int res = 0;

	os_mutex_lock(&mutex, OS_FOREVER);

	if (view_stack.inited == 0) {
		res = -ESRCH;
		goto out_unlock;
	}

	if (view_stack.num >= ARRAY_SIZE(view_stack.data)) {
		res = -ENOSPC;
		goto out_unlock;
	}

	if (view_stack.num > 0)
		old_data = &view_stack.data[view_stack.num - 1];
	if (old_data && old_data->cache)
		focused_id = view_cache_get_focus_view();

	res = _view_stack_jump(old_data, data);
	if (!res) {
		memcpy(&view_stack.data[view_stack.num++], data, sizeof(*data));
		if (old_data == NULL) {
			view_stack.init_view = data->id;
		} else if (focused_id >= 0) {
			old_data->id = (uint8_t)focused_id;
		}
	}

	assert(res == 0);

out_unlock:
	os_mutex_unlock(&mutex);
	return res;
}

static void _view_group_notify_cb(uint8_t view_id, uint8_t msg_id)
{
	view_stack_data_t * data;

	os_mutex_lock(&mutex, OS_FOREVER);

	data = &view_stack.data[view_stack.num - 1];
	if (view_stack.num < 1 || data->group == NULL)
		return;

	if (msg_id == MSG_VIEW_FOCUS || msg_id == MSG_VIEW_DEFOCUS) {
		bool focused = (msg_id == MSG_VIEW_FOCUS);

		if (data->group->focus_cb)
			data->group->focus_cb(view_id, focused);

		if (focused)
			data->id = view_id;
	}

	os_mutex_unlock(&mutex);
}

static int _view_stack_jump(const view_stack_data_t *old_data,
		const view_stack_data_t *new_data)
{
	int16_t old_view = -1;
	int16_t new_view = -1;
	int8_t i;
	int res = 0;

	if (old_data) {
		if (old_data->cache) {
			old_view = view_cache_get_focus_view();
			view_cache_deinit();
		} else if (old_data->group) {
			ui_manager_set_notify_callback(NULL);

			old_view = old_data->id;
			for (i = old_data->group->num - 1; i >= 0; i--) {
				if (old_data->group->vlist[i] != old_view)
					ui_view_delete(old_data->group->vlist[i]);
			}

			/* delete the focused view at last to avoid unexpected focus changes */
			ui_view_delete(old_view);
		} else {
			old_view = old_data->id;
			ui_view_delete(old_data->id);
		}
	}

	if (new_data) {
		if (new_data->cache) {
			res = view_cache_init(new_data->cache, new_data->id);
		} else if (new_data->group) {
			ui_manager_set_notify_callback(_view_group_notify_cb);

			ui_view_create(new_data->group->vlist[new_data->group->idx],
					new_data->group->plist ? new_data->group->plist[new_data->group->idx] : NULL,
					UI_CREATE_FLAG_SHOW);

			for (i = new_data->group->num - 1; i >= 0; i--) {
				if (i != new_data->group->idx) {
					ui_view_create(new_data->group->vlist[i],
							new_data->group->plist ? new_data->group->plist[i] : NULL,
							0);
				}
			}
		} else {
			res = ui_view_create(new_data->id, new_data->presenter, UI_CREATE_FLAG_SHOW);
		}

		new_view = new_data->id;
	}

	SYS_LOG_INF("from %d to %d (res=%d)", old_view, new_view, res);
	return res;
}
