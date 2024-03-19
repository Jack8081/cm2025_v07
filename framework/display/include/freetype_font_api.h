#ifndef __FREETYPE_FONT_API_H__
#define __FREETYPE_FONT_API_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include <fs/fs.h>
#include <stdint.h>
#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_CACHE_H

/*********************
 *      DEFINES
 *********************/
 #define MAX_CACHE_NUM		80
 #define MAX_FONT_PATH_LEN	24

/**********************
 *      TYPEDEFS
 **********************/
typedef struct
{
	uint32_t advance;
	uint32_t bbw;
	uint32_t bbh;
	int32_t bbx;
	int32_t bby;
}bbox_metrics_t;


typedef struct
{
	uint32_t current;
	uint32_t cached_total;
	uint32_t* glyph_index;
	bbox_metrics_t* metrics;
	uint8_t* data;
	uint32_t inited;
}freetype_cache_t;

typedef struct
{
	FT_Face face;
	freetype_cache_t* cache;
	uint8_t font_path[MAX_FONT_PATH_LEN];
	uint16_t font_size;
	int16_t ascent;
	int16_t descent;
	uint16_t default_advance;
	uint32_t line_height;
	uint32_t ref_count;
}freetype_font_t;




/**********************
 * GLOBAL PROTOTYPES
 **********************/
int freetype_font_init(void);
int freetype_font_deinit(void);

freetype_font_t* freetype_font_open(const char* file_path, uint32_t font_size);

void freetype_font_close(freetype_font_t* font);

uint8_t * freetype_font_get_bitmap(freetype_font_t* font, freetype_cache_t* cache, uint32_t unicode);
bbox_metrics_t* freetype_font_get_glyph_dsc(freetype_font_t* font, freetype_cache_t *cache, uint32_t unicode);

freetype_cache_t* freetype_font_get_cache(freetype_font_t* font);

/**********************
 *      MACROS
 **********************/
 
#ifdef __cplusplus
} /* extern "C" */
#endif	
#endif /*__FREETYPE_FONT_API_H__*/
