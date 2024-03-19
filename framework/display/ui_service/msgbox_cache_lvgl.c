/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <msgbox_cache.h>
#include <lvgl.h>
#include <assert.h>

typedef struct {
	const uint8_t *ids;
	const msgbox_cache_popup_cb_t *cbs;
	uint8_t num;
	uint8_t en : 1;

	atomic_t free_popups;
	lv_obj_t * popup_objs[CONFIG_NUM_MSGBOX_POPUPS];
	uint8_t popup_ids[CONFIG_NUM_MSGBOX_POPUPS];
	uint8_t popup_view_ids[CONFIG_NUM_MSGBOX_POPUPS];
} msgbox_cache_ctx_t;

static void _msgbox_popup_handler(uint8_t msgbox_id, bool closed, int16_t focused_view_id);
static void _msgbox_delete_handler(lv_event_t * e);
static msgbox_cache_popup_cb_t _msgbox_find_cb(uint8_t msgbox_id);

static msgbox_cache_ctx_t msgbox_ctx;

int msgbox_cache_init(const uint8_t * ids, const msgbox_cache_popup_cb_t * cbs, uint8_t num)
{
	memset(&msgbox_ctx, 0, sizeof(msgbox_ctx));
	msgbox_ctx.ids = ids;
	msgbox_ctx.cbs = cbs;
	msgbox_ctx.num = num;
	msgbox_ctx.en = 1;
	atomic_set(&msgbox_ctx.free_popups, ARRAY_SIZE(msgbox_ctx.popup_objs));

	ui_manager_set_msgbox_popup_callback(_msgbox_popup_handler);
	return 0;
}

void msgbox_cache_deinit(void)
{
	msgbox_cache_close_all();
	ui_manager_set_msgbox_popup_callback(NULL);
}

void msgbox_cache_set_en(bool en)
{
	msgbox_ctx.en = en ? 1 : 0;
}

uint8_t msgbox_cache_num_popup_get(void)
{
	return ARRAY_SIZE(msgbox_ctx.popup_objs) - atomic_get(&msgbox_ctx.free_popups);
}

int msgbox_cache_popup(uint8_t msgbox_id)
{
	if (msgbox_ctx.en == 0) {
		return -EPERM;
	}

	if (_msgbox_find_cb(msgbox_id) == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&msgbox_ctx.free_popups) < 1) {
		return -EBUSY;
	}

	if (ui_msgbox_popup(msgbox_id)) {
		return -ENOMSG;
	}

	atomic_dec(&msgbox_ctx.free_popups);
	return 0;
}

void msgbox_cache_close_all(void)
{
	ui_msgbox_close(MSGBOX_ID_ALL);
}

void msgbox_cache_dump(void)
{
	int i;

	os_printk("msgbox:\n"
	          " id |   hwnd   | view\n"
	          "----+----------| \n");

	for (i = ARRAY_SIZE(msgbox_ctx.popup_objs) - 1; i >= 0; i--) {
		if (msgbox_ctx.popup_objs[i]) {
			os_printk(" %02x | %08x |  %02x\n",
				msgbox_ctx.popup_ids[i], msgbox_ctx.popup_objs[i],
				msgbox_ctx.popup_view_ids[i]);
		}
	}

	os_printk("\n");
}

static void _msgbox_popup_close_handler(uint8_t msgbox_id, int16_t focused_view_id)
{
	int i;

	for (i = ARRAY_SIZE(msgbox_ctx.popup_objs) - 1; i >= 0; i--) {
		if (msgbox_ctx.popup_objs[i] == NULL) {
			continue;
		}

		if (msgbox_id == msgbox_ctx.popup_ids[i]) {
			lv_obj_del(msgbox_ctx.popup_objs[i]);
			break;
		}

		if (msgbox_id == MSGBOX_ID_ALL &&
			(msgbox_ctx.popup_view_ids[i] == focused_view_id || focused_view_id < 0)) {
			lv_obj_del(msgbox_ctx.popup_objs[i]);
			break;
		}
	}
}

static void _msgbox_popup_handler(uint8_t msgbox_id, bool closed, int16_t focused_view_id)
{
	view_data_t * view_data;
	msgbox_cache_popup_cb_t popup_cb;
	int i;

	if (closed) {
		_msgbox_popup_close_handler(msgbox_id, focused_view_id);
		return;
	}

	popup_cb = _msgbox_find_cb(msgbox_id);
	assert(popup_cb != NULL);

	if (focused_view_id < 0) {
		SYS_LOG_ERR("no focused view to attach, ignore msgbox %d", msgbox_id);
		popup_cb(msgbox_id, -ESRCH);
		goto fail_exit;
	}

	for (i = ARRAY_SIZE(msgbox_ctx.popup_objs) - 1; i >= 0; i--) {
		if (msgbox_ctx.popup_objs[i] == NULL) {
			break;
		}
	}

	assert(i >= 0);

	msgbox_ctx.popup_objs[i] = popup_cb(msgbox_id, 0);
	if (msgbox_ctx.popup_objs[i] == NULL) {
		SYS_LOG_ERR("fail to create msgbox %d", msgbox_id);
		goto fail_exit;
	}

	msgbox_ctx.popup_ids[i] = msgbox_id;
	msgbox_ctx.popup_view_ids[i] = (uint8_t)focused_view_id;

	lv_obj_add_event_cb(msgbox_ctx.popup_objs[i],
			_msgbox_delete_handler, LV_EVENT_DELETE, NULL);

	/* Capture all the focus */
	view_data = view_get_data((uint8_t)focused_view_id);
	lv_obj_add_flag(lv_disp_get_layer_top(view_data->display), LV_OBJ_FLAG_CLICKABLE);

	SYS_LOG_INF("create msgbox %d (%p), attached to view %d",
			msgbox_id, msgbox_ctx.popup_objs[i], focused_view_id);
	return;
fail_exit:
	if (atomic_inc(&msgbox_ctx.free_popups) == ARRAY_SIZE(msgbox_ctx.popup_objs) - 1) {
		ui_manager_gesture_set_enabled(true);
	}
}

static void _msgbox_delete_handler(lv_event_t * e)
{
	view_data_t * view_data;
	lv_obj_t * target = lv_event_get_target(e);
	int i;

	for (i = ARRAY_SIZE(msgbox_ctx.popup_objs) - 1; i >= 0; i--) {
		if (msgbox_ctx.popup_objs[i] == target) {
			break;
		}
	}

	assert(i >= 0);
	msgbox_ctx.popup_objs[i] = NULL;

	SYS_LOG_INF("delete msgbox %d (%p)", msgbox_ctx.popup_ids[i], target);

	if (atomic_inc(&msgbox_ctx.free_popups) == ARRAY_SIZE(msgbox_ctx.popup_objs) - 1) {
		view_data = view_get_data(msgbox_ctx.popup_view_ids[i]);
		lv_obj_clear_flag(lv_disp_get_layer_top(view_data->display), LV_OBJ_FLAG_CLICKABLE);
		/* Enable system gesture */
		ui_manager_gesture_set_enabled(true);
	}
}

static msgbox_cache_popup_cb_t _msgbox_find_cb(uint8_t msgbox_id)
{
	msgbox_cache_popup_cb_t popup_cb = NULL;
	int i;

	for (i = msgbox_ctx.num - 1; i >= 0; i--) {
		if (msgbox_id == msgbox_ctx.ids[i]) {
			popup_cb = msgbox_ctx.cbs[i];
			break;
		}
	}

	return popup_cb;
}
