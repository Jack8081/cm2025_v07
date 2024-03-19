#ifndef _LV_FONT_BITMAP_H
#define _LV_FONT_BITMAP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <lvgl.h>
#include "freetype_font_api.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
 
typedef struct {
	freetype_font_t*  font;      /* handle to face object */
	freetype_cache_t* cache;
	uint32_t font_size;
}lv_font_fmt_freetype_dsc_t;


/**********************
 * GLOBAL PROTOTYPES
 **********************/
 
int lvgl_freetype_font_init(void);
int lvgl_freetype_font_open(lv_font_t* font, const char * font_path, uint32_t font_size);

/**********************
 *      MACROS
 **********************/
 
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_FONT_BITMAP_H*/
