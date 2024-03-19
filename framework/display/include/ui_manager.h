/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ui manager interface
 */

#ifndef __UI_MANGER_H__
#define __UI_MANGER_H__

#include <msg_manager.h>
#include <view_manager.h>
#include <input_manager.h>

#ifdef CONFIG_SEG_LED_MANAGER
#include <seg_led_manager.h>
#endif
#ifdef CONFIG_LED_MANAGER
#include <led_manager.h>
#endif

//#include "led_display.h"
/**
 * @defgroup ui_manager_apis app ui Manager APIs
 * @ingroup system_apis
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 */
/**
 * @brief dispatch key event to target view
 *
 * This routine dispatch key event to target view, ui manager will found the target
 * view by view_key_map
 *
 * @param key_event value of key event
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int ui_manager_dispatch_key_event(uint32_t key_event);

/**
 * INTERNAL_HIDDEN @endcond
 */
/**
 * @brief send ui message to target view
 *
 * This routine send ui message to target view which mark by view id
 *
 * @param view_id id of view
 * @param msg_id id of msg_id
 * @param msg_data param of message
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int ui_manager_send_async(uint8_t view_id, uint8_t msg_id, uint32_t msg_data);

/**
 * @brief handle app specific ui message in ui manger thread context
 *
 * This routine tries to handle app specific ui message in ui manger thread
 * context, by sending the msg to ui manger service whose msg type must larger
 * than MSG_APP_MESSAGE_START and msg callback should not be NULL. So the app
 * can handle message in msg callback.
 *
 * @param msg message
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int ui_manager_handle_appmsg_async(struct app_msg *msg);

/**
 * @brief dispatch ui message to target view
 *
 * This routine dispatch ui message to target view which mark by view id
 * if the target view not found in view list ,return failed. otherwise target
 * view will process this ui message.
 *
 * @param view_id id of view
 * @param msg_id id of msg_id
 * @param msg_data param of message
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int ui_message_dispatch(uint8_t view_id, uint8_t msg_id, uint32_t msg_data);

/**
 * @brief Create a new view
 *
 * This routine Create a new view. After view created, it is hidden by default.
 *
 * @param view_id init view id
 * @param presenter view presenter
 * @param flags create flags, see enum UI_VIEW_CREATE_FLAGS
 *
 * @retval 0 on invoiked success else negative errno code.
 */
#ifdef CONFIG_UI_SERVICE
int ui_view_create(uint8_t view_id, const void *presenter, uint8_t flags);
#else
int ui_view_create(uint8_t view_id, ui_view_info_t *info);
#endif

/**
 * @brief Inflate view layout
 *
 * This routine inflate view layout, called by view itself if required.
 *
 * @param view_id id of view
 *
 * @retval 0 on invoiked success else negative errno code.
 */
int ui_view_layout(uint8_t view_id);

/**
 * @brief update view
 *
 * @param view_id id of view
 *
 * @retval 0 on invoiked success else negative errno code.
 */
int ui_view_update(uint8_t view_id);

/**
 * @brief destory view
 *
 * This routine destory view, delete form ui manager view list
 * and release all resource for view.
 *
 * @param view_id id of view
 *
 * @retval 0 on invoiked success else negative errno code.
 */
int ui_view_delete(uint8_t view_id);

/**
 * @brief show view
 *
 * This routine show view, show form ui manager view list.
 *
 * @param view_id id of view
 *
 * @retval 0 on invoiked success else negative errno code.
 */
int ui_view_show(uint8_t view_id);

/**
 * @brief hide view
 *
 * This routine hide view, hide form ui manager view list.
 *
 * @param view_id id of view
 *
 * @retval 0 on invoiked success else negative errno code.
 */
int ui_view_hide(uint8_t view_id);

/**
 * @brief set view order
 *
 * @param view_id id of view
 *
 * @retval 0 on invoiked success else negative errno code.
 */
int ui_view_set_order(uint8_t view_id, uint16_t order);

/**
 * @brief set view drag attribute
 *
 * This routine set view drag attribute. If drag_attribute > 0, it implicitly make the view
 * visible, just like view_set_hidden(view_id, false) called.
 *

 * @param view_id id of view
 * @param drag_attribute drag attribute, see enum UI_VIEW_DRAG_ATTRIBUTE
 *
 * @retval 0 on success else negative errno code.
 */
int ui_view_set_drag_attribute(uint8_t view_id, int16_t drag_attribute);

/**
 * @brief paint view
 *
 * @param view_id id of view
 *
 * @retval 0 on invoiked success else negative errno code.
 */
int ui_view_paint(uint8_t view_id);

/**
 * @brief refresh view
 *
 * @param view_id id of view
 *
 * @retval 0 on invoiked success else negative errno code.
 */
int ui_view_refresh(uint8_t view_id);

/**
 * @brief pause view
 *
 * After view become paused, the view surface may become unreachable.
 * View is not paused by default after created.
 * A paused view is always hidden regardless of the hidden attribute.
 *
 * @param view_id id of view
 *
 * @retval 0 on invoiked success else negative errno code.
 */
int ui_view_pause(uint8_t view_id);

/**
 * @brief resume view
 *
 * After view resumed, the view surface becomes reachable.
 *
 * @param view_id id of view
 *
 * @retval 0 on invoiked success else negative errno code.
 */
int ui_view_resume(uint8_t view_id);

/**
 * @brief set view position
 *
 * @param view_id id of view
 * @param x new coordinate X
 * @param y new coordinate Y
 *
 * @retval 0 on invoiked success else negative errno code.
 */
int ui_view_set_pos(uint8_t view_id, int16_t x, int16_t y);

/**
 * @brief send user message to view
 *
 * @param view_id id of view
 * @param msg_id user message id, must not less than MSG_VIEW_USER_OFFSET
 * @param msg_data user message data
 *
 * @retval 0 on invoiked success else negative errno code.
 */
int ui_view_send_user_msg(uint8_t view_id, uint8_t msg_id, void *msg_data);

/**
 * @brief popup a message box
 *
 * This routine will disable gesture implicitly, just like
 * ui_manager_gesture_set_enabled(false) is called. If the
 * registered popup callback failed, should enable the
 * gesture.
 *
 * @param msgbox_id user defined message box id
 *
 * @retval 0 on invoiked success else negative code.
 */
int ui_msgbox_popup(uint8_t msgbox_id);

/**
 * @brief close a message box
 *
 * @param msgbox_id user defined message box id.
 *
 * @retval 0 on invoiked success else negative code.
 */
int ui_msgbox_close(uint8_t msgbox_id);

/**
 * @brief ui manager init funcion
 *
 * This routine calls init ui manager ,called by main
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int ui_manager_init(void);

/**
 * @brief ui manager deinit funcion
 *
 * This routine calls deinit ui manager ,called by main
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int ui_manager_exit(void);

/**
 * @brief ui manager post display manually
 *
 * @note Nomally this routine is unnecessary to be invoked, sine the display
 *       posting (including the gui task execution) is driven by display TE
 *       signal. But in the case that gui task execution is required to do
 *       some cleanup, while the display suddenly blanked on and TE signal
 *       became inactive, this routine can be invoked to trigger gui task
 *       execution once.
 *
 * @retval 0 on succsess else negative code
 */
int ui_manager_post_display(void);

/**
 * @brief ui manager set gui draw period
 *
 * This routine set gui draw invoking period which is multiple of vsync(te)
 * period. By default, the multiple is 1.
 *
 * @param multiple period multiple of vsync(te) period
 *
 * @retval 0 on succsess else negative code
 */
int ui_manager_set_draw_period(uint8_t multiple);

/**
 * @brief ui manager register state notify callback
 *
 * The possible notify states is MSG_VIEW_FOCUS, MSG_VIEW_DEFOCUS, MSG_VIEW_PAUSE, MSG_VIEW_RESUME
 *
 * @param callback function called when view state changed takes place
 *
 * @retval 0 on succsess else negative code
 */
int ui_manager_set_notify_callback(ui_notify_cb_t callback);

/**
 * @brief ui manager register msgbox popup callback
 *
 * @param callback function called when a msgbox popup
 *
 * @retval 0 on succsess else negative code
 */
int ui_manager_set_msgbox_popup_callback(ui_msgbox_popup_cb_t callback);

/**
 * @brief ui manager register gesture/key event callback
 *
 * The registered callback will only handle the key events passed by
 * ui_manager_dispatch_key_event() (also CONFIG_UISRV_KEY_EVENT_BYPASS
 * is not configured.) and the (inner) gesture events.
 *
 * @param callback function called when gesture/key event take place
 *
 * @retval 0 on succsess else negative code
 */
int ui_manager_set_keyevent_callback(ui_keyevent_cb_t callback);

/**
 * @brief ui manager enable/disable gesture detection
 *
 * This routine enable/disable gesture detection. By default, gesture is enabled.
 *
 * @param en enabled flag
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int ui_manager_gesture_set_enabled(bool en);

/**
 * @brief ui manager set gesture direction filter
 *
 * This routine filter the gestures. By default, all are filtered in.
 *
 * @param filter gesture dir filter flags, like (1 << GESTURE_DROP_DOWN) | (1 << GESTURE_DROP_UP)
 *                 only the gestures filtered will be detected.
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int ui_manager_gesture_set_dir(uint8_t filter);

/**
 * @brief Ignore gesture detection until the next release
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int ui_manager_gesture_wait_release(void);

/**
 * @brief Enable/disable draw during gesture dragging and the following animation.
 *
 * If set true, the drawing will be forbidden during gesture and the following animation,
 * except that the view has not painted after layout.
 * If set false, the drawing will be disabled, but may affect the performace.
 *
 * By default (if the function never called), drawing is disabled.
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int ui_manager_gesture_forbid_draw(bool en);

/**
 * @} end defgroup system_apis
 */
#endif
