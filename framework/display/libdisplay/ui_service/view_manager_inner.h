/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRAMEWORK_DISPLAY_UI_SERVICE_VIEW_MANAGER_INNNER_H_
#define FRAMEWORK_DISPLAY_UI_SERVICE_VIEW_MANAGER_INNNER_H_

#include <errno.h>
#include <ui_service.h>
#include <view_manager.h>

/**
* @cond INTERNAL_HIDDEN
*/

/**
 * @brief notify new message to a view
 *
 * @param view_id id of view
 * @param msg_id message id
 * @param msg_data message data
 *
 * @retval 0 on succsess else negative errno code.
 */
int view_notify_message(uint8_t view_id, uint8_t msg_id, void *msg_data);

/**
 * @brief Create a new view
 *
 * This routine Create a new view. After view created, it is hidden by default.
 *
 * @param view_id view id
 * @param presenter view presenter
 * @param flags create flags
 *
 * @retval 0 on succsess else negative errno code.
 */
int view_create(uint8_t view_id, const void *presenter, uint8_t flags);

/**
 * @brief Inflate a view layout
 *
 * This routine Inflate a view layout
 *
 * @param view_id init view id
 *
 * @retval 0 on succsess else negative errno code.
 */
int view_layout(uint8_t view_id);

/**
 * @brief view repaint
 *
 * This routine provide view refresh
 *
 * @param view_id id of view
 *
 * @retval 0 on succsess else negative errno code.
 */
int view_paint(uint8_t view_id);

/**
 * @brief view refresh
 *
 * This routine provide view refresh
 *
 * @param view_id id of view
 *
 * @retval 0 on succsess else negative errno code.
 */
int view_refresh(uint8_t view_id);

/**
 * @brief destory view
 *
 * This routine destory view, delete form ui manager view list
 * and release all resource for view.
 *
 * @param view_id id of view
 *
 * @retval 0 on succsess else negative errno code.
 */
int view_delete(uint8_t view_id);

/**
 * @brief view paused
 *
 * The view surface may become unreachable.
 *
 * @param view_id id of view
 *
 * @retval 0 on succsess else negative errno code.
 */
int view_pause(uint8_t view_id);

/**
 * @brief view resumed
 *
 * The view surface becomes reachable.
 *
 * @param view_id id of view
 *
 * @retval 0 on succsess else negative errno code.
 */
int view_resume(uint8_t view_id);

/**
 * @brief set view order
 *
 * This routine set view order, The lower the order value of the view,
 *  the higher the view
 *
 * @param view_id id of view
 * @param order of view
 *
 * @retval 0 on succsess else negative errno code.
 */
int view_set_order(uint8_t view_id, uint8_t order);

/**
 * @brief view set hide
 *
 * This routine provide view set hide
 *
 * @param view_id id of view
 * @hidden hidden or not
 *
 * @retval 0 on success else negative code.
 */
int view_set_hidden(uint8_t view_id, bool hidden);

/**
 * @brief Query view has painted or not after layout
 *
 * @param view_id id of view
 *
 * @return query result.
 */
bool view_is_painted(uint8_t view_id);

/**
 * @brief Query view surface buffer count
 *
 * @param view_id id of view
 *
 * @return query result.
 */
uint8_t view_get_buffer_count(uint8_t view_id);

/**
 * @brief set dirty region of view
 *
 * This routine provide set dirty region of view
 *
 * @param view pointer to view structure
 * @region dirty_region region
 * @flags surface flag SURFACE_FIRST_DRAW or SURFACE_LAST_DRAW
 *
 * @retval 0 on success else negative code
 */
int view_set_dirty_region(ui_view_context_t *view, const ui_region_t *dirty_region, uint32_t flags);

/**
 * @brief Initialize view manager
 *
 * @retval 0 on success else negative code
 */
int view_manager_init(void);

/**
 * @brief reset the focus among views
 *
 * This routine trt ti find which view should be focused and set the focus for it.
 *
 * @param reason_view the view that cause refocus. If set NULL, means always refocus.
 *
 * @retval N/A
 */
void view_manager_refocus(ui_view_context_t *reason_view);

/**
 * @brief query the view system is dirty
 *
 * @retval the result.
 */
bool view_manager_is_dirty(void);

/**
 * @brief mark the view system is dirty
 *
 * only when the view system is marked dirty, the dirty regions of screen will be refreshed.
 *
 * @retval N/A.
 */
void view_manager_set_dirty(bool dirty);

/**
 * @brief mark the whole diplay region diry
 *
 * @retval N/A.
 */
void view_manager_mark_disp_dirty(void);

/**
 * @brief get the unique post sequence
 *
 * The post sequence will increase on every successful display post
 *
 * @retval the post sequence.
 */
uint32_t view_manager_get_post_id(void);

/**
 * @brief post the dirty regions to the display
 *
 * @retval 0 on success else negative code.
 */
int view_manager_post_display(void);

/**
 * @brief resume display, mark
 *
 * @retval N/A.
 */
void view_manager_resume_display(void);

/**
 * @brief process the started animation.
 *
 * @retval 0 on success else negative code.
 */
int view_manager_animation_process(void);

/**
 * @brief stop the started animation.
 *
 * @param forced force stop the animation
 *
 * @retval 0 on success else negative code.
 */
int view_manager_animation_stop(bool forced);

/**
 * @brief get the animation state.
 *
 * @retval the animation state.
 */
uint8_t view_manager_animation_get_state(void);

/**
 * @brief set callback
 *
 * This routine set callback
 *
 * @param id callback id
 * @param callback callback function
 *
 * @retval 0 on success else negative code.
 */
int view_manager_set_callback(uint8_t id, void * callback);

/**
 * @brief popup a msgbox
 *
 * @param msgbox_id message box id
 *
 * @retval 0 on success else negative code.
 */
int view_manager_popup_msgbox(uint8_t msgbox_id);

/**
 * @brief close a msgbox by id or all the msgbox attached to specific view id
 *
 * If msgbox_id is UI_MSGBOX_ALL, all the msgboxs attached to 'attached_view' will
 * be closed, otherwise only the specific msgbox_id will be closed.
 *
 * @param msgbox_id message box id
 * @param attached_view_id attached view id; will be ignored if msgbox_id is not UI_MSGBOX_ALL.
 *
 * @retval 0 on success else negative code.
 */
int view_manager_close_msgbox(uint8_t msgbox_id, int16_t attached_view_id);

/**
 * @brief displatch the key events.
 *
 * @retval N/A
 */
void view_manager_dispatch_key_event(uint32_t key_event);

/**
 * @brief Register the gui callback to view manager
 *
 * @param type enum UI_VIEW_TYPE
 * @param preferred_pixel_format perferred pixel format, HAL_PIXEL_FORMAT_x
 * @param callback callback to handle gui
 *
 * @retval 0 on success else negative code
 */
int view_manager_register_gui_callback(uint8_t type,
		uint32_t preferred_pixel_format, const ui_view_gui_callback_t *callback);

/**
 * @brief Handle the period task of the specific gui
 *
 * @retval N/A.
 */
void view_manager_gui_task_handler(void);

/**
 * @brief Initialize the view gui
 *
 * @param create_flags view create flags
 *
 * @retval 0 on success else negative code.
 */
int view_gui_init(ui_view_context_t *view, uint8_t create_flags);

/**
 * @brief Finalize the view gui
 *
 * @retval N/A
 */
void view_gui_deinit(ui_view_context_t *view);

/**
 * @brief Begine to clear the view surface buffer
 *
 * @retval N/A
 */
void view_gui_clear_begin(ui_view_context_t *view);

/**
 * @brief End to clear the view surface buffer
 *
 * @retval N/A
 */
void view_gui_clear_end(ui_view_context_t *view);

/**
 * @brief Make the view foreground and can receive the input events
 *
 * @retval 0 on success else negative code.
 */
int view_gui_set_focus(ui_view_context_t *view);

/**
 * @brief Refresh the view immediately
 *
 * @retval 0 on success else negative code.
 */
void view_gui_refresh(ui_view_context_t *view);

/**
 * @brief Pause the view draw/refresh
 *
 * @retval 0 on success else negative code.
 */
void view_gui_pause_draw(ui_view_context_t *view);

/**
 * @brief Resume the view view draw/refresh
 *
 * @param force_redraw force redraw all the view
 *
 * @retval 0 on success else negative code.
 */
void view_gui_resume_draw(ui_view_context_t *view, bool force_redraw);

/**
 * @brief Cleanup the view after post complete
 *
 * @retval N/A
 */
void view_gui_post_cleanup(void *surface);

/**
* INTERNAL_HIDDEN @endcond
*/

#endif /* FRAMEWORK_DISPLAY_UI_SERVICE_VIEW_MANAGER_INNNER_H_ */
