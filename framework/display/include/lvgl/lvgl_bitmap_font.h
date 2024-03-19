#ifndef _LV_FONT_BITMAP_H
#define _LV_FONT_BITMAP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <lvgl.h>
#include <bitmap_font_api.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
 
typedef struct {
    bitmap_font_t*  font;      /* handle to face object */
	bitmap_cache_t* cache;
	bitmap_emoji_font_t* emoji_font;
}lv_font_fmt_bitmap_dsc_t;


/**********************
 * GLOBAL PROTOTYPES
 **********************/
 
int lvgl_bitmap_font_init(const char *def_font_path);
int lvgl_bitmap_font_deinit(void);
int lvgl_bitmap_font_open(lv_font_t* font, const char * font_path);
void lvgl_bitmap_font_close(lv_font_t* font);
int lvgl_bitmap_font_set_emoji_font(lv_font_t* lv_font, const char* emoji_font_path);
int lvgl_bitmap_font_get_emoji_dsc(const lv_font_t* lv_font, uint32_t unicode, lv_img_dsc_t* dsc);

/**********************
 *      MACROS
 **********************/
 
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_FONT_BITMAP_H*/
