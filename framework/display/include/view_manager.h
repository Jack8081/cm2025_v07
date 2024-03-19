/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file view manager interface
 */

#ifndef __VIEW_MANGER_H__
#define __VIEW_MANGER_H__

/**
 * @defgroup ui_manager_apis app ui Manager APIs
 * @ingroup system_apis
 * @{
 */

#include <ui_region.h>
#include <sys/slist.h>
#include <input_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UI_VIEW_INV_ID  (255U)

#define TRANSMIT_ALL_KEY_EVENT  0xFFFFFFFF

#define UI_VIEW_ANIM_RANGE 1024

/* stand for all the msgboxes, used in closing all msgboxes when focused view closed */
#define UI_MSGBOX_ALL   (255U)

struct ui_view_anim_cfg;

typedef int (*ui_view_proc_t)(uint8_t view_id, uint8_t msg_id, void *msg_data);

typedef int (*ui_get_state_t)(void);

typedef bool (*ui_state_match_t)(uint32_t current_state, uint32_t match_state);

typedef void (*ui_notify_cb_t)(uint8_t view_id, uint8_t msg_id);

/**
 * @typedef ui_msgbox_popup_cb
 * @brief Callback API to popup/close the msgbox
 * @param msgbox_id msgbox id
 * @param closed close or popup the msgbox
 * @param focused_view_id the focused view when msgbox popup requested
 *
 * @retval N/A
 */
typedef void (*ui_msgbox_popup_cb_t)(uint8_t msgbox_id, bool closed, int16_t focused_view_id);

/**
 * @typedef ui_keyevent_cb
 * @brief Callback API to notify (gesture) event
 * @param view_id the focused view when event takes place
 * @param event event id
 *
 * @retval N/A
 */
typedef void (*ui_keyevent_cb_t)(uint8_t view_id, uint32_t event);

/**
 * @typedef ui_view_anim_path
 * @brief Callback API for setting view dragging animation
 * @param scroll_throw_vect indicated the drag speed, it uses similar algorithm as LVGL.
 * @param cfg pointer to structure ui_view_anim_cfg filled by callback
 *
 * @retval N/A
 */
typedef void (*ui_view_drag_anim_cb_t)(uint8_t view_id,
		const ui_point_t *scroll_throw_vect, struct ui_view_anim_cfg *cfg);

/**
 * @typedef ui_view_anim_path
 * @brief Callback API to compute the view move position
 * @param elaps the ratio of elapsed time to the duration. range [0, UI_VIEW_ANIM_RANGE]
 *
 * @retval the interpolation value of move ratio, range [0, UI_VIEW_ANIM_RANGE]
 */
typedef int32_t (*ui_view_anim_path_cb_t)(int32_t elaps);

/* notify the user the animation has stopped */
typedef void (*ui_view_anim_stop_cb_t)(uint8_t view_id, const ui_point_t *pos);

enum UI_VIEW_TYPE {
	UI_VIEW_Unknown = 0,
	UI_VIEW_LVGL,
	UI_VIEW_USER, /* user defined */

	NUM_VIEW_TYPES,
};

enum UI_VIEW_CREATE_FLAGS {
	UI_CREATE_FLAG_NO_PRELOAD = (1 << 0), /* ignore resource preload when created */
	UI_CREATE_FLAG_SHOW = (1 << 1), /* show the view by default */
	UI_CREATE_FLAG_SINGLE_BUF = (1 << 3), /* initial refresh mode is UI_REFR_SINGLE_BUF */
	UI_CREATE_FLAG_ZERO_BUF = (1 << 4), /* initial refresh mode is UI_REFR_ZERO_BUF */
};

enum UI_VIEW_FLAGS {
	UI_FLAG_HIDDEN       = (1 << 0),
	UI_FLAG_FOCUSED      = (1 << 1),
	UI_FLAG_PAUSED       = (1 << 2),

	UI_FLAG_INFLATED     = (1 << 3), /* layout inflated */
	UI_FLAG_PAINTED      = (1 << 4), /* first paint finished after layout */

	/* refresh mode changed */
	UI_FLAG_REFRESH_CHANGED = (1 << 5),
};

enum UI_VIEW_REFRESH_FLAGS {
	/* flags for view refresh */
	UI_REFR_FLAG_MOVED         = (1 << 0), /* refresh due to position changed */
	UI_REFR_FLAG_CHANGED       = (1 << 1), /* refresh due to content changed */
	UI_REFR_FLAG_FIRST_CHANGED = (1 << 2), /* first refresh in frame due to content changed */
	UI_REFR_FLAG_LAST_CHANGED  = (1 << 3), /* last refresh in frame due to content changed */
};

enum UI_VIEW_REFRESH_MODE {
	UI_REFR_DEFAULT = 0, /* the real buffer count depends on CONFIG_SURFACE_MAX_BUFFER_COUNT, but takes care of dirty region  */
	UI_REFR_SINGLE_BUF, /* always use single buffer even when view focused */
	UI_REFR_ZERO_BUF, /* always use zero buffer, post one part to screen while drawing another part, but sliding will be unavailable */
};

/* DOWN/UP/LEFT/RIGHT indicates the drop/move direction */
enum UI_VIEW_DRAG_ATTRIBUTE {
	UI_DRAG_NONE      = 0,
	UI_DRAG_DROPDOWN  = (1 << 0),
	UI_DRAG_DROPUP    = (1 << 1),
	UI_DRAG_DROPLEFT  = (1 << 2),
	UI_DRAG_DROPRIGHT = (1 << 3),
	UI_DRAG_MOVEDOWN  = (1 << 4),
	UI_DRAG_MOVEUP    = (1 << 5),
	UI_DRAG_MOVELEFT  = (1 << 6),
	UI_DRAG_MOVERIGHT = (1 << 7),
};

typedef struct {
    /** key value, which key is pressed */
	uint32_t key_val;

    /** key type, which key type of the pressed key */
	uint32_t key_type;

    /** app state, the state of app service to handle the message */
	uint32_t app_state;

    /** app msg, the message of app service will be send */
	uint32_t app_msg;

    /** key policy */
	uint32_t key_policy;
} ui_key_map_t;

typedef struct view_data {
	/* gui data */
	//void *context; /* gui context */
	void *display; /* gui display */
	void *surface; /* gui surface to draw on */

	/* custom data */
	const void *presenter; /* view presenter passed by app */
	void *user_data; /* application defined data */
} view_data_t;

typedef struct {
	ui_region_t     region;
	ui_view_proc_t  view_proc;
	ui_get_state_t  view_get_state;
	/** state match function */
	ui_state_match_t view_state_match;
	const ui_key_map_t	*view_key_map;
	void		*app_id;
	uint16_t	flags;
	uint16_t	order;
} ui_view_info_t;

/* app entry structure */
typedef struct view_entry {
	/** app id */
	const char *app_id;
	/** proc function of view */
	ui_view_proc_t  proc;
	/** get state function of view  */
	ui_get_state_t  get_state;
	/**key map of view */
	const ui_key_map_t	*key_map;
	/**view id */
	uint8_t id;
	/**default order */
	uint8_t default_order;
	/**view type */
	uint8_t type;
	/** view width */
	uint16_t width;
	/** view height */
	uint16_t height;
} view_entry_t;

/**
 * @brief Statically define and initialize view entry for view.
 *
 * The view entry define statically,
 *
 * Each view must define the view entry info so that the system wants
 * to find view to knoe the corresponding information
 *
 * @param app_id Name of the app.
 * @param view_proc view proc function.
 * @param view_get_state view get state function.
 * @param view_key_map key map of view .
 * @param view_id view id of view .
 * @param default_order default order of view .
 * @param view_w view width .
 * @param view_h view height.
 */
#ifdef CONFIG_SIMULATOR
#define VIEW_DEFINE(app_name, view_proc, view_get_state, view_key_map,\
		view_id, order, view_type, view_w, view_h)	\
	const struct view_entry __view_entry_##app_name	= {	\
		.app_id = #app_name,							\
		.proc = view_proc,								\
		.get_state = view_get_state,					\
		.key_map = view_key_map,		    			\
		.id = view_id,									\
		.default_order = order,							\
		.type = view_type,								\
		.width = view_w,								\
		.height = view_h,								\
	}
#else
#define VIEW_DEFINE(app_name, view_proc, view_get_state, view_key_map,\
		view_id, order, view_type, view_w, view_h)	\
	const struct view_entry __view_entry_##app_name	\
		__attribute__((__section__(".view_entry")))= {	\
		.app_id = #app_name,							\
		.proc = view_proc,								\
		.get_state = view_get_state,					\
		.key_map = view_key_map,		    			\
		.id = view_id,									\
		.default_order = order,							\
		.type = view_type,								\
		.width = view_w,								\
		.height = view_h,								\
	}
#endif
typedef struct {
	sys_snode_t node;

#ifdef CONFIG_UI_SERVICE
	const view_entry_t *entry;
	ui_region_t region;
	uint8_t flags;
	uint8_t refr_flags;
	uint8_t order;
	uint8_t drag_attr;
	ui_view_drag_anim_cb_t drag_anim_cb;

	view_data_t data;
	void *snapshot; /* pointer to the snapshot of the surface buffer */
#else
	void   *app_id;
	ui_view_info_t  info;
	uint8_t   view_id;
#endif
} ui_view_context_t;

enum UI_VIEW_ANIMATION_STATE {
	UI_ANIM_NONE,
	UI_ANIM_START,
	UI_ANIM_RUNNING,
	UI_ANIM_STOP,
};

/* DOWN/UP/LEFT/RIGHT indicates the slide direction */
enum UI_VIEW_SLIDE_ANIMATION_TYPE {
	UI_ANIM_SLIDE_IN_DOWN = 1,
	UI_ANIM_SLIDE_IN_UP,
	UI_ANIM_SLIDE_IN_RIGHT,
	UI_ANIM_SLIDE_IN_LEFT,
	UI_ANIM_SLIDE_OUT_UP,
	UI_ANIM_SLIDE_OUT_DOWN,
	UI_ANIM_SLIDE_OUT_LEFT,
	UI_ANIM_SLIDE_OUT_RIGHT,
};

typedef struct ui_view_anim_cfg {
	ui_point_t start;
	ui_point_t stop;
	uint16_t duration;
	ui_view_anim_path_cb_t path_cb;
	ui_view_anim_stop_cb_t stop_cb;
} ui_view_anim_cfg_t;

typedef struct {
	uint8_t state;
	uint8_t is_slide : 1;

	uint8_t view_id;
	int16_t last_view_id;
	ui_point_t last_view_offset;

	uint32_t start_time;
	uint16_t elapsed;

	ui_view_anim_cfg_t cfg;
} ui_view_animation_t;

enum UI_MSG_ID {
	MSG_VIEW_CREATE,  /* creating view, MSG_VIEW_PRELOAD or MSG_VIEW_LAYOUT will be sent to view */
	MSG_VIEW_PRELOAD, /* preload view resource */
	MSG_VIEW_LAYOUT,  /* view layout inflate, can be called by the view itself if required */
	MSG_VIEW_DELETE,  /* deleting view */
	MSG_VIEW_PAINT,   /* update the view normally driven by data changed */
	MSG_VIEW_REFRESH, /* refresh view surface to display */
	MSG_VIEW_FOCUS,   /* view becomes focused */
	MSG_VIEW_DEFOCUS, /* view becomes de-focused */
	MSG_VIEW_PAUSE,   /* view surface becomes unreachable */
	MSG_VIEW_RESUME,  /* view surface becomes reachable */
	MSG_VIEW_UPDATE,  /* resource reload completed */

	MSG_VIEW_SET_HIDDEN, /* 11 */
	MSG_VIEW_SET_ORDER,
	MSG_VIEW_SET_POS,
	MSG_VIEW_SET_DRAG_ATTRIBUTE,

	MSG_VIEW_SET_CALLBACK, /* 15 */

	MSG_VIEW_SET_DRAW_PERIOD, /* set draw period multiple of vsync(te) period */

	MSG_POST_DISPLAY, /* 17 */
	MSG_FORCE_POST_DISPLAY, /* force post display */
	MSG_RESUME_DISPLAY,

	/* msgbox */
	MSG_MSGBOX_POPUP, /* popup message box */
	MSG_MSGBOX_CLOSE, /* close message box */

	/* gesture setting */
	MSG_GESTURE_ENABLE, /* 22 */
	MSG_GESTURE_SET_DIR,
	MSG_GESTURE_WAIT_RELEASE,
	MSG_GESTURE_FORBID_DRAW,

	/* user message offset for specific view */
	MSG_VIEW_USER_OFFSET = 128,
};

enum UI_CALLBACK_ID {
	UI_CB_NOTIFY,
	UI_CB_MSGBOX,
	UI_CB_KEYEVENT,

	UI_NUM_CBS,
};

/**
 * @brief get view data
 *
 * This routine get view data.
 *
 * @param view_id id of view
 *
 * @return view data on success else NULL
 */
view_data_t *view_get_data(uint8_t view_id);

/**
 * @brief get view display
 *
 * This routine get view display.
 *
 * @param data view data
 *
 * @return view display on success else NULL
 */
static inline void *view_get_display(view_data_t *data)
{
	return data->display;
}

/**
 * @brief get view surface
 *
 * This routine get view gui surface.
 *
 * @param data view data
 *
 * @return view surface on success else NULL
 */
static inline void *view_get_surface(view_data_t *data)
{
	return data->surface;
}

/**
 * @brief get view data
 *
 * This routine get view data.
 *
 * @param view_id id of view
 *
 * @return view data on success else NULL
 */
static inline const void *view_get_presenter(view_data_t *data)
{
	return data->presenter;
}

/**
 * @cond INTERNAL_HIDDEN
 */

/**
 * @brief set view refresh mode
 *
 * This routine set view refresh mode
 *
 * @param view_id id of view
 * @param mode refresh mode, see enum UI_VIEW_REFRESH_MODE
 *
 * @retval 0 on succsess else negative errno code.
 */
int view_set_refresh_mode(uint8_t view_id, uint8_t mode);

/**
 * @brief enable view refresh to display or not
 *
 * This routine enable view refresh to display or not
 *
 * @param view_id id of view
 * @param enabled enable or not
 *
 * @retval 0 on succsess else negative errno code.
 */
int view_set_refresh_en(uint8_t view_id, bool enabled);

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief query view is hidden
 *
 * @param view_id id of view
 *
 * @retval the query result
 */
bool view_is_hidden(uint8_t view_id);

/**
 * @brief query view is visible (even partially) now or not
 *
 * A view to be visible, must not be hidden, and also partially at least on the screen.
 *
 * @param view_id id of view
 *
 * @retval the query result
 */
bool view_is_visible(uint8_t view_id);

/**
 * @brief Query view is focused or not
 *
 * @param view_id id of view
 *
 * @return query result.
 */
bool view_is_focused(uint8_t view_id);

/**
 * @brief Query view is focused or not
 *
 * @param view_id id of view
 *
 * @return query result.
 */
bool view_is_paused(uint8_t view_id);

/**
 * @brief get x coord of position of view
 *
 * @param view_id id of view
 *
 * @retval x coord
 */
int16_t view_get_x(uint8_t view_id);

/**
 * @brief get y coord of position of view
 *
 * @param view_id id of view
 *
 * @retval y coord
 */
int16_t view_get_y(uint8_t view_id);

/**
 * @brief get position of view
 *
 * @param view_id id of view
 * @param x pointer to store x coordinate
 * @param y pointer to store y coordinate
 *
 * @retval 0 on success else negative code
 */
int view_get_pos(uint8_t view_id, int16_t *x, int16_t *y);

/**
 * @brief get width of view
 *
 * @param view_id id of view
 *
 * @retval width on success else negative code
 */
int16_t view_get_width(uint8_t view_id);

/**
 * @brief get height of view
 *
 * @param view_id id of view
 *
 * @retval height on success else negative code
 */
int16_t view_get_height(uint8_t view_id);

/**
 * @brief get position of view
 *
 * @param view_id id of view
 * @param x pointer to store x coordinate
 * @param y pointer to store y coordinate
 *
 * @retval 0 on success else negative code
 */
int view_get_region(uint8_t view_id, ui_region_t *region);

/**
 * @brief set position x of view
 *
 * @param view_id id of view
 * @param x new x coordinate
 *
 * @retval 0 on success else negative code
 */
int view_set_x(uint8_t view_id, int16_t x);

/**
 * @brief set position y of view
 *
 * @param view_id id of view
 * @param y new y coordinate
 *
 * @retval 0 on success else negative code
 */
int view_set_y(uint8_t view_id, int16_t y);

/**
 * @brief set position of view
 *
 * @param view_id id of view
 * @param x new x coordinate
 * @param y new y coordinate
 *
 * @retval 0 on success else negative code
 */
int view_set_pos(uint8_t view_id, int16_t x, int16_t y);

/**
 * @brief set view drag attribute
 *
 * This routine set view drag attribute. If drag_attribute > 0, it implicitly make the view
 * visible, just like view_set_hidden(view_id, false) called.
 *
 * @param view_id id of view
 * @param drag_attribute drag attribute, see enum UI_VIEW_DRAG_ATTRIBUTE
 *
 * @retval 0 on succsess else negative errno code.
 */
int view_set_drag_attribute(uint8_t view_id, uint8_t drag_attribute);

/**
 * @brief get view drag attribute
 *
 * @param view_id id of view
 *
 * @retval drag attribute if view exist else negative code
 */
uint8_t view_get_drag_attribute(uint8_t view_id);

/**
 * @brief query view has move attribute (DRAG_ATTRIBUTE_MOVExx)
 *
 * @param view_id id of view
 *
 * @retval the query result
 */
bool view_has_move_attribute(uint8_t view_id);

/**
 * @brief set drag callback of view
 *
 * The callback will be called when the single dragged finished. During the callback,
 * the user can start the animation to some other position.
 *
 * @param view_id id of view
 * @param drag_cb callback for dragging one view, not involved in view switching
 *
 * @retval 0 on success else negative code
 */
int view_set_drag_anim_cb(uint8_t view_id, ui_view_drag_anim_cb_t drag_cb);

/**
 * @brief get diplay X resolution
 *
 * @retval X resolution in pixels
 */
int16_t view_manager_get_disp_xres(void);

/**
 * @brief get diplay Y resolution
 *
 * @retval Y resolution in pixels
 */
int16_t view_manager_get_disp_yres(void);

/**
 * @brief get dragged view
 *
 * This routine provide get dragged view.
 *
 * @param gesture gesture, see enum GESTURE_TYPE
 * @param towards_screen store the drag direction, whether dragged close to screen or away from screen.
 *
 * @retval view id on success else -1.
 */
int16_t view_manager_get_draggable_view(uint8_t gesture, bool *towards_screen);

/**
 * @brief get focused view
 *
 * This routine provide get focused view
 *
 * @retval view id of focused view on success else -1.
 */
int16_t view_manager_get_focused_view(void);

/**
 * @brief dump the view informations to the console.
 *
 * @retval N/A.
 */
void view_manager_dump(void);

/**
 * @brief refocus view pre animation
 *
 * @param view_id the view that will be focused.
 *
 * @retval N/A
 */
void view_manager_pre_anim_refocus(uint8_t view_id);

/**
 * @brief refocus view post animation
 *
 * @retval N/A
 */
void view_manager_post_anim_refocus(void);

/**
 * @brief get the start and stop points of the slide animation
 *
 * @param view_id the view to take the animation
 * @param start store the start point
 * @param stop store the stop point
 * @param animation_type animation type, see  enum UI_VIEW_ANIMATION_TYPE
 *
 * @retval 0 on success else negative code.
 */
int view_manager_get_slide_animation_points(uint8_t view_id,
		ui_point_t *start, ui_point_t *stop, uint8_t animation_type);

/**
 * @brief start the view animation on view switching
 *
 * @param view_id the view to take the animation
 * @param last_view_id the related view which will fade out if exists
 * @param animation_type animation type, see  enum UI_VIEW_ANIMATION_TYPE
 * @param cfg animation config
 *
 * @retval 0 on success else negative code.
 */
int view_manager_slide_animation_start(uint8_t view_id,
		int16_t last_view_id, uint8_t animation_type, ui_view_anim_cfg_t *cfg);

/**
 * @brief start the view animation caused on single long view dragging
 *
 * @param view_id the view to take the animation
 * @param runtime pointer to structure input_dev_runtime_t
 *
 * @retval 0 on success else negative code.
 */
int view_manager_drag_animation_start(uint8_t view_id, input_dev_runtime_t *runtime);

#ifdef __cplusplus
}
#endif

/**
 * @} end defgroup system_apis
 */
#endif /* __VIEW_MANGER_H__ */
