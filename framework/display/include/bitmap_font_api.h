#ifndef __BITMAP_FONT_API_H__
#define __BITMAP_FONT_API_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include <fs/fs.h>
#include <stdint.h>

/*********************
 *      DEFINES
 *********************/
#define MAX_FONT_PATH_LEN			64
#ifdef	CONFIG_BITMAP_FONT_USE_HIGH_FREQ_CACHE
#define MAX_CACHED_GLYPH_BITMAPS	64
#else
#define MAX_CACHED_GLYPH_BITMAPS	200
#endif
#define CACHED_GLYPH_ID_BITS		4
#define MAX_CACHED_GLYPH_IDS		(1 << CACHED_GLYPH_ID_BITS)

#define USE_BSEARCH_IN_GLYPH_ID		1

/**********************
 *      TYPEDEFS
 **********************/

typedef enum
{
	BITMAP_FONT_FORMAT_4BPP,
	BITMAP_FONT_FORMAT_UNSUPPORTED,
}bitmap_font_format_e;

typedef struct
{
	uint32_t advance;
	uint32_t bbw;
	uint32_t bbh;
	int32_t bbx;
	int32_t bby;
}glyph_metrics_t;


typedef struct
{
	uint32_t current;
	uint32_t cached_total;
	uint32_t* glyph_index;
	glyph_metrics_t* metrics;
	uint8_t* data;
	uint32_t inited;
	uint32_t unit_size;
	uint32_t cached_max;
	/* last mapping */
	uint32_t last_unicode[MAX_CACHED_GLYPH_IDS];
	uint32_t last_glyph_id[MAX_CACHED_GLYPH_IDS];
#if MAX_CACHED_GLYPH_IDS > 1
	uint16_t last_unicode_idx;
	uint16_t curr_unicode_idx;
#endif

	uint32_t last_glyph_idx;
}bitmap_cache_t;

typedef struct{
	uint32_t unicode;
	uint16_t width;
	uint16_t height;
	uint32_t offset;
	uint8_t reserve[4];
}emoji_font_entry_t;

typedef struct
{
#ifdef CONFIG_RES_MANAGER_USE_SDFS
	struct sd_file* font_fp;
#else
	struct fs_file_t font_fp;
#endif
	bitmap_cache_t* cache;
	emoji_font_entry_t* glyf_list;
	uint32_t glyf_count;
	uint16_t font_width;
	uint16_t font_height;
	int16_t ascent;
	int16_t descent;
	uint32_t default_advance;
	uint32_t bpp;
	uint32_t inited;
	uint32_t ref_count;
}bitmap_emoji_font_t;

typedef struct
{
#ifdef CONFIG_RES_MANAGER_USE_SDFS
	struct sd_file* font_fp;
#else
	struct fs_file_t font_fp;
#endif
	bitmap_cache_t* cache;
	uint8_t font_path[MAX_FONT_PATH_LEN];
	uint16_t font_size;
	int16_t ascent;
	int16_t descent;
	uint16_t default_advance;
	uint32_t font_format;
	uint32_t loca_format;
	uint32_t glyfid_format;
	uint32_t adw_format;
	uint32_t bpp;
	uint32_t bbxy_length;
	uint32_t bbwh_length;
	uint32_t adw_length;
	uint32_t compress_alg;
	uint32_t cmap_offset;
	uint32_t loca_offset;
	uint32_t glyf_offset;
	uint32_t ref_count;
	uint8_t* cmap_sub_headers;
	uint32_t cmap_sub_count;
}bitmap_font_t;




/**********************
 * GLOBAL PROTOTYPES
 **********************/
int bitmap_font_init(void);
int bitmap_font_deinit(void);

bitmap_font_t* bitmap_font_open(const char* file_path);

void bitmap_font_close(bitmap_font_t* font);

uint8_t * bitmap_font_get_bitmap(bitmap_font_t* font, bitmap_cache_t* cache, uint32_t unicode);
glyph_metrics_t* bitmap_font_get_glyph_dsc(bitmap_font_t* font, bitmap_cache_t *cache, uint32_t unicode);

bitmap_cache_t* bitmap_font_get_cache(bitmap_font_t* font);

bitmap_emoji_font_t* bitmap_emoji_font_open(const char* file_path);
void bitmap_emoji_font_close(bitmap_emoji_font_t* emoji_font);
uint8_t * bitmap_font_get_emoji_bitmap(bitmap_emoji_font_t* font, uint32_t unicode);
glyph_metrics_t* bitmap_font_get_emoji_glyph_dsc(bitmap_emoji_font_t* font, uint32_t unicode);
void bitmap_font_load_high_freq_chars(const uint8_t* file_path);



/**********************
 *      MACROS
 **********************/
 
#ifdef __cplusplus
} /* extern "C" */
#endif	
#endif /*__BITMAP_FONT_API_H__*/
