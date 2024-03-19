/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_MSGBOX_H_
#define FRAMEWORK_DISPLAY_INCLUDE_MSGBOX_H_

#include <errno.h>
#include <ui_manager.h>

/* stand for all msgbox ids */
#define MSGBOX_ID_ALL UI_MSGBOX_ALL

/**
 * @typedef msgbox_cache_popup_cb_t
 * @brief Callback to popup a specific msgbox
 * 
 * This callback should return the GUI handle of the msgbox.
 * For LVGL, it is the root object of the msgbox widgets.
 *
 * @param msgbox_id     Message box ID.
 * @param err           Error code. 0 to create the popup else cancel the popup.
 *
 * @return the GUI handle of the msgbox if err == 0 else NULL
 */
typedef void * (*msgbox_cache_popup_cb_t)(uint8_t msgbox_id, int err);

#ifdef CONFIG_LV_DISP_TOP_LAYER

/**
 * @brief Initialize the msgbox cache
 *
 * @param ids array of msgbox ids
 * @param cbs array of msgbox popup callback
 * @param num number of msgbox ids
 *
 * @retval 0 on success else negative code.
 */
int msgbox_cache_init(const uint8_t * ids,
		const msgbox_cache_popup_cb_t * cbs, uint8_t num);

/**
 * @brief Deinitialize the msgbox cache
 *
 * @retval N/A.
 */
void msgbox_cache_deinit(void);

/**
 * @brief Enable msgbox popup or not
 *
 * @retval N/A.
 */
void msgbox_cache_set_en(bool en);

/**
 * @brief Popup a msgbox
 *
 * @param id msgbox id
 *
 * @retval 0 on success else negative code.
 */
int msgbox_cache_popup(uint8_t id);

/**
 * @brief Force close all the msgbox popups
 *
 * @retval N/A.
 */
void msgbox_cache_close_all(void);

/**
 * @brief Get number of popups at present
 *
 * @retval number of popups.
 */
uint8_t msgbox_cache_num_popup_get(void);

/**
 * @brief Dump popups
 *
 * @retval N/A
 */
void msgbox_cache_dump(void);

#else /* CONFIG_LV_DISP_TOP_LAYER */

static inline int msgbox_cache_init(const uint8_t * ids,
		const void ** cbs, uint8_t num)
{
	return -ENOSYS;
}

static inline void msgbox_cache_deinit(void) {}
static inline void msgbox_cache_set_en(bool en) {}
static inline int msgbox_cache_popup(uint8_t msgbox_id) { return -ENOSYS; }
static inline void msgbox_cache_close_all(void) { }
static inline uint8_t msgbox_cache_num_popup_get(void) { return 0; }
static inline void msgbox_cache_dump(void) { }

#endif /* CONFIG_LV_DISP_TOP_LAYER */

#endif /* FRAMEWORK_DISPLAY_INCLUDE_MSGBOX_H_ */
