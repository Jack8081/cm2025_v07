#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <os_common_api.h>
#include <ui_mem.h>
#include "font_mempool.h"
#include "res_mempool.h"
#include "bitmap_font_api.h"
#ifndef FS_O_READ
#define FS_O_READ		0x01
#define FS_SEEK_SET     SEEK_SET
#define FS_SEEK_END     SEEK_END
#endif


#define DEF_HIGH_FREQ_BIN				"hfreq.bin"


#define LVGL_FONT_SIZE_OFFSET				14
#define LVGL_FONT_ASCENT_OFFSET				16
#define LVGL_FONT_DESCENT_OFFSET			18
#define LVGL_FONT_ADVANCE_OFFSET			30
#define LVGL_FONT_LOCA_FMT_OFFSET			34
#define LVGL_FONT_GLYFID_FMT_OFFSET			35
#define LVGL_FONT_ADW_FMT_OFFSET			36
#define LVGL_FONT_BBXY_LENGTH_OFFSET		38
#define LVGL_FONT_BBWH_LENGTH_OFFSET		39
#define LVGL_FONT_ADW_LENGTH_OFFSET			40


#define MAX_HIGH_FREQ_NUM					3500


typedef struct
{
	uint32_t data_offset;  //relative to cmap offset
	uint32_t range_start;
	uint16_t range_length;
	uint16_t glyf_id_offset;
	uint16_t entry_count;
	uint8_t sub_format;
	uint8_t align;
}cmap_sub_header_t;

typedef union
{
	int8_t bits7:7;
	int8_t bits6:6;
	int8_t bits5:5;
	int8_t bits4:4;
	int8_t bits3:3;
	int8_t bits2:2;
	int8_t xy;
}bbxy_t;

typedef enum
{
	CACHE_TYPE_NORMAL,
	CACHE_TYPE_HIGH_FREQ,
}bitmap_cache_type_e;

typedef struct
{
	glyph_metrics_t* metrics;
	uint8_t* data;
	uint32_t font_size;
	uint32_t unit_size;
	uint32_t last_cache_idx;
	uint16_t last_unicode_idx;
}high_freq_cache_t;

typedef struct{
	char magic[4];
	int32_t count;
	uint8_t width;
	uint8_t height;
	uint8_t reserved[6];
}emoji_font_header_t;


static cmap_sub_header_t** cmap_sub_headers;
static uint8_t* font_cmap_cache_buffer;
static uint8_t* cmap_sub_data;
static uint32_t metrics32[2];

static bitmap_cache_t* bitmap_cache;
static bitmap_font_t* opend_font;

static high_freq_cache_t high_freq_cache;
static uint16_t* high_freq_codes;

static uint32_t elapse_count=0;

static bitmap_emoji_font_t opend_emoji_font;
static int emoji_slot;
static int max_fonts;
static int font_cache_size;
static int lvgl_cmap_cache_size;

void bitmap_font_load_high_freq_chars(const uint8_t* file_path);

int _bitmap_cache_init(bitmap_cache_t* cache, uint32_t slot)
{
	uint8_t* cache_start;

	if(cache == NULL)
	{
		SYS_LOG_ERR("null font cache");
		return -1;
	}

	if(cache->inited == 1)
	{
		return 0;
	}

	cache_start = bitmap_font_get_cache_buffer() + font_cache_size*slot;
	if(cache_start == NULL)
	{
		SYS_LOG_ERR("no memory for font cache\n");
		return -1;
	}
	SYS_LOG_INF("cache_start %p, slot %d\n", cache_start, slot);
	cache->glyph_index = (uint32_t*)cache_start;

	memset(cache->glyph_index, 0, cache->cached_max*sizeof(uint32_t));
	cache->metrics = (glyph_metrics_t*)(cache_start + cache->cached_max*sizeof(uint32_t));
	cache->data = cache_start + cache->cached_max*sizeof(uint32_t) + cache->cached_max*sizeof(glyph_metrics_t);

	cache->inited = 1;

	SYS_LOG_INF("metrics_buf %p, data_buf %p\n", cache->metrics, cache->data);
	return 0;
}

int bitmap_font_init(void)
{
	int i;

	max_fonts = bitmap_font_get_max_fonts_num();
	font_cache_size = bitmap_font_get_font_cache_size();
	lvgl_cmap_cache_size = bitmap_font_get_cmap_cache_size();
	emoji_slot = max_fonts-1;

	bitmap_cache = (bitmap_cache_t*)mem_malloc(max_fonts*sizeof(bitmap_cache_t));
	opend_font = (bitmap_font_t*)mem_malloc(max_fonts*sizeof(bitmap_font_t));
	cmap_sub_headers = (cmap_sub_header_t**)mem_malloc(max_fonts*sizeof(cmap_sub_header_t*));

	for(i = 0;i < max_fonts; i++)
	{
		bitmap_cache[i].inited = 0;
	}
	memset(opend_font, 0, sizeof(bitmap_font_t)*max_fonts);
	memset(&opend_emoji_font, 0, sizeof(bitmap_emoji_font_t));
	font_cmap_cache_buffer = bitmap_font_get_cache_buffer() + font_cache_size*max_fonts;

	if(bitmap_font_get_high_freq_enabled())
	{
		high_freq_codes = (uint16_t*)(bitmap_font_get_cache_buffer() + font_cache_size*max_fonts + lvgl_cmap_cache_size*max_fonts);
		high_freq_cache.metrics = (glyph_metrics_t*)(bitmap_font_get_cache_buffer() + font_cache_size*max_fonts + lvgl_cmap_cache_size*max_fonts + MAX_HIGH_FREQ_NUM*sizeof(uint16_t));
		high_freq_cache.data = bitmap_font_get_cache_buffer() + font_cache_size*max_fonts + lvgl_cmap_cache_size*max_fonts + MAX_HIGH_FREQ_NUM*sizeof(uint16_t) + MAX_HIGH_FREQ_NUM*sizeof(glyph_metrics_t);
		high_freq_cache.font_size = 0;
	}
	return 0;
}

int bitmap_font_deinit(void)
{
	int32_t i;

	for(i=0;i<max_fonts;i++)
	{
#ifdef CONFIG_RES_MANAGER_USE_SDFS
		if(opend_font[i].font_fp != NULL)
#else
		if(opend_font[i].font_fp.filep != NULL)
#endif
		{
			bitmap_font_close(&opend_font[i]);
			memset(&opend_font[i], 0, sizeof(bitmap_font_t));
		}

		if(bitmap_cache[i].inited == 1)
		{
			bitmap_cache[i].inited = 0;
		}

	}

	if(cmap_sub_headers)
	{
		mem_free(cmap_sub_headers);
	}
	if(bitmap_cache)
	{
		mem_free(bitmap_cache);
	}
	if(opend_font)
	{
		mem_free(opend_font);
	}
	return 0;
}


bitmap_cache_t* bitmap_font_get_cache(bitmap_font_t* font)
{
	return font->cache;
}

bitmap_emoji_font_t* bitmap_emoji_font_open(const char* file_path)
{
	int ret;
	uint32_t i;
	uint32_t glyf_size;

	if(opend_emoji_font.inited == 1)
	{
		//suppose only 1 emoji font size used
		opend_emoji_font.ref_count++;
		SYS_LOG_INF("opend emoji font attr size %d, asc %d, desc %d, adw %d\n", opend_emoji_font.font_height, opend_emoji_font.ascent, opend_emoji_font.descent, opend_emoji_font.default_advance);
		return &opend_emoji_font;
	}

	memset(&opend_emoji_font, 0, sizeof(bitmap_emoji_font_t));
	opend_emoji_font.cache = &bitmap_cache[emoji_slot];
	memset(opend_emoji_font.cache, 0, sizeof(bitmap_cache_t));

	if ( file_path != NULL )
	{
		ret = res_fs_open(&opend_emoji_font.font_fp, file_path);
		if ( ret < 0 )
		{
			SYS_LOG_ERR(" open font file %s failed! \n", file_path);
			return NULL;
		}
	}

	//FIXME: read emoji font head and things
	emoji_font_header_t header;
	ret = res_fs_read(&opend_emoji_font.font_fp, &header, sizeof(emoji_font_header_t));
	if(ret < sizeof(emoji_font_header_t))
	{
		res_fs_close(&opend_emoji_font.font_fp);
		SYS_LOG_ERR(" read emoji font file header failed! \n");
		return NULL;
	}

	opend_emoji_font.bpp = 3;
	opend_emoji_font.glyf_list = (emoji_font_entry_t*)(bitmap_font_get_cache_buffer() + max_fonts*(font_cache_size+lvgl_cmap_cache_size));
	opend_emoji_font.glyf_count = header.count;
	opend_emoji_font.inited = 1;

	ret = res_fs_read(&opend_emoji_font.font_fp, opend_emoji_font.glyf_list, sizeof(emoji_font_entry_t)*header.count);
	if(ret < sizeof(emoji_font_entry_t)*header.count)
	{
		res_fs_close(&opend_emoji_font.font_fp);
		SYS_LOG_ERR(" read emoji font file entries failed! \n");
		return NULL;
	}

	uint32_t max_height = 0;
	uint32_t max_width = 0;
	for(i=0;i<opend_emoji_font.glyf_count;i++)
	{
		if(max_width < opend_emoji_font.glyf_list[i].width)
		{
			max_width = opend_emoji_font.glyf_list[i].width;
		}

		if(max_height < opend_emoji_font.glyf_list[i].height)
		{
			max_height = opend_emoji_font.glyf_list[i].height;
		}
	}

	max_width = ((max_width+3)/4)*4;

	opend_emoji_font.font_width = max_width;
	opend_emoji_font.font_height = max_height;
	opend_emoji_font.ascent = max_height;
	opend_emoji_font.descent = 0;
	opend_emoji_font.ref_count = 1;
	opend_emoji_font.default_advance = max_width;


	glyf_size = opend_emoji_font.font_width*opend_emoji_font.bpp;
	glyf_size = ((glyf_size+3)/4)*4;
	glyf_size *= opend_emoji_font.font_height;
	opend_emoji_font.cache->unit_size = glyf_size;
	opend_emoji_font.cache->cached_max = font_cache_size/(glyf_size + sizeof(uint32_t*) + sizeof(glyph_metrics_t));
	ret = _bitmap_cache_init(opend_emoji_font.cache, emoji_slot);
	if(ret < 0)
	{
		return NULL;
	}
	SYS_LOG_INF("opend_emoji_font cache %p, unit size %d, cached max %d\n", opend_emoji_font.cache->data, opend_emoji_font.cache->unit_size, opend_emoji_font.cache->cached_max);
	return &opend_emoji_font;
}

bitmap_font_t* bitmap_font_open(const char* file_path)
{
	int32_t ret;
	uint8_t attr_tmp;
	uint32_t head_size;
	uint32_t cmap_size;
	uint32_t loca_size;
	uint32_t i;
	uint32_t cmap_sub_count;
	int32_t empty_slot = -1;
	bitmap_font_t* bmp_font = NULL;

	for(i=0;i<max_fonts;i++)
	{
#ifdef CONFIG_RES_MANAGER_USE_SDFS
		if(opend_font[i].font_fp != NULL)
#else
		if(opend_font[i].font_fp.filep != NULL)
#endif
		{
			if(strcmp(opend_font[i].font_path, file_path) == 0)
			{
				bmp_font = &opend_font[i];
				bmp_font->ref_count++;
				SYS_LOG_INF("\n opend font cmap_sub_headers %p\n", bmp_font->cmap_sub_headers);
				SYS_LOG_INF("opend font attr size %d, asc %d, desc %d, adw %d\n", bmp_font->font_size, bmp_font->ascent, bmp_font->descent, bmp_font->default_advance);
				SYS_LOG_INF("opend font attr adw len %d, bbxy len %d, bbwh len %d\n", bmp_font->adw_length, bmp_font->bbxy_length, bmp_font->bbwh_length);
				return bmp_font;
			}
		}
		else
		{
			if(empty_slot < 0)
			{
				empty_slot = i;
			}
		}
	}

	if(empty_slot >= 0)
	{
		bmp_font = &opend_font[empty_slot];
	}
	else
	{
		SYS_LOG_ERR("no available slot for bitmap font %s", file_path);
		return NULL;
	}

	memset(bmp_font, 0, sizeof(bitmap_font_t));
	bmp_font->cache = &bitmap_cache[empty_slot];
	memset(bmp_font->cache, 0, sizeof(bitmap_cache_t));

	if ( file_path != NULL )
	{
		ret = res_fs_open(&bmp_font->font_fp, file_path);

		if ( ret < 0 )
		{
			SYS_LOG_ERR(" open font file %s failed! \n", file_path);
			goto ERR_EXIT;
		}
	}

	ret = res_fs_read(&bmp_font->font_fp, &head_size, 4);
	if ( ret < 4 )
	{
		SYS_LOG_ERR( "read font head failed !!! " );
		goto ERR_EXIT;
	}

	res_fs_seek(&bmp_font->font_fp, 0, FS_SEEK_SET);
	ret = res_fs_read(&bmp_font->font_fp, &head_size, 4);
	bmp_font->cmap_offset = head_size;

	res_fs_seek(&bmp_font->font_fp, LVGL_FONT_SIZE_OFFSET, FS_SEEK_SET);
	ret = res_fs_read(&bmp_font->font_fp, &bmp_font->font_size, 2);

	res_fs_seek(&bmp_font->font_fp, LVGL_FONT_ASCENT_OFFSET, FS_SEEK_SET);
	ret = res_fs_read(&bmp_font->font_fp, &bmp_font->ascent, 2);

	res_fs_seek(&bmp_font->font_fp, LVGL_FONT_DESCENT_OFFSET, FS_SEEK_SET);
	ret = res_fs_read(&bmp_font->font_fp, &bmp_font->descent, 2);

	res_fs_seek(&bmp_font->font_fp, LVGL_FONT_ADVANCE_OFFSET, FS_SEEK_SET);
	ret = res_fs_read(&bmp_font->font_fp, &bmp_font->default_advance, 2);

	res_fs_seek(&bmp_font->font_fp, LVGL_FONT_LOCA_FMT_OFFSET, FS_SEEK_SET);
	ret = res_fs_read(&bmp_font->font_fp, &attr_tmp, 1);
	bmp_font->loca_format = attr_tmp;

	ret = res_fs_read(&bmp_font->font_fp, &attr_tmp, 1);
	bmp_font->glyfid_format = attr_tmp;

	ret = res_fs_read(&bmp_font->font_fp, &attr_tmp, 1);
	bmp_font->adw_format = attr_tmp;

	ret = res_fs_read(&bmp_font->font_fp, &attr_tmp, 1);
	bmp_font->bpp = attr_tmp;

	ret = res_fs_read(&bmp_font->font_fp, &attr_tmp, 1);
	bmp_font->bbxy_length = attr_tmp;

	ret = res_fs_read(&bmp_font->font_fp, &attr_tmp, 1);
	bmp_font->bbwh_length = attr_tmp;

	ret = res_fs_read(&bmp_font->font_fp, &attr_tmp, 1);
	bmp_font->adw_length = attr_tmp;

	ret = res_fs_read(&bmp_font->font_fp, &attr_tmp, 1);
	bmp_font->compress_alg = attr_tmp;

	if(bmp_font->adw_format == 1)
	{
		bmp_font->default_advance = bmp_font->default_advance >> 4;
	}

	SYS_LOG_INF("font attr size %d, asc %d, desc %d, adw %d\n", bmp_font->font_size, bmp_font->ascent, bmp_font->descent, bmp_font->default_advance);
	SYS_LOG_INF("font attr adw fmt %d, adw len %d, bbxy len %d, bbwh len %d\n", bmp_font->adw_format, bmp_font->adw_length, bmp_font->bbxy_length, bmp_font->bbwh_length);

	//FIXME: cache cmap
	res_fs_seek(&bmp_font->font_fp, bmp_font->cmap_offset, FS_SEEK_SET);
	ret = res_fs_read(&bmp_font->font_fp, &cmap_size, 4);
	bmp_font->loca_offset = bmp_font->cmap_offset + cmap_size;
	//get subtable count

	res_fs_seek(&bmp_font->font_fp, bmp_font->cmap_offset+8, FS_SEEK_SET);
	ret = res_fs_read(&bmp_font->font_fp, &cmap_sub_count, 4);
	cmap_sub_headers[empty_slot] = (cmap_sub_header_t*)(font_cmap_cache_buffer + empty_slot*lvgl_cmap_cache_size);
	bmp_font->cmap_sub_headers = (uint8_t*)cmap_sub_headers[empty_slot];
	bmp_font->cmap_sub_count = cmap_sub_count;

	res_fs_read(&bmp_font->font_fp, bmp_font->cmap_sub_headers, cmap_size-12);

	cmap_sub_data = (uint8_t*)bmp_font->cmap_sub_headers + cmap_sub_count*16;

	//read loca offset, loca table too big for cache
	res_fs_seek(&bmp_font->font_fp, bmp_font->loca_offset, FS_SEEK_SET);
	ret = res_fs_read(&bmp_font->font_fp, &loca_size, 4);
	bmp_font->glyf_offset = bmp_font->loca_offset + loca_size;
	bmp_font->ref_count = 1;

	uint32_t bmp_size = bmp_font->font_size*bmp_font->font_size/(8/bmp_font->bpp);
	bmp_size = ((bmp_size + 3)/4) * 4;
	bmp_font->cache->unit_size = bmp_size;
	bmp_font->cache->cached_max = font_cache_size/(bmp_size + sizeof(uint32_t*) + sizeof(glyph_metrics_t));
	SYS_LOG_INF("font attr per data size %d, max cached number %d\n", bmp_font->cache->unit_size, bmp_font->cache->cached_max);
	ret = _bitmap_cache_init(bmp_font->cache, empty_slot);
	if(ret < 0)
	{
		goto ERR_EXIT;
	}

	strcpy(bmp_font->font_path, file_path);
	return bmp_font;
ERR_EXIT:
	res_fs_close(&bmp_font->font_fp);

	memset(bmp_font, 0, sizeof(bitmap_font_t));
	return NULL;
}

void bitmap_emoji_font_close(bitmap_emoji_font_t* emoji_font)
{
	if(emoji_font != NULL)
	{
		if(opend_emoji_font.inited)
		{
			if(opend_emoji_font.ref_count > 1)
			{
				opend_emoji_font.ref_count--;
			}
			else
			{
				res_fs_close(&opend_emoji_font.font_fp);
				opend_emoji_font.cache->inited = 0;
				memset(&opend_emoji_font, 0, sizeof(bitmap_emoji_font_t));
			}
		}
	}
}

void bitmap_font_close(bitmap_font_t* bmp_font)
{
	int32_t i;

	if(bmp_font != NULL)
	{
		for(i=0;i<max_fonts;i++)
		{
			if(bmp_font == &opend_font[i])
			{
				break;
			}
		}

		if(i >= max_fonts)
		{
			return;
		}

		if(opend_font[i].font_path[0] != 0)
		{
			if(opend_font[i].ref_count > 1)
			{
				opend_font[i].ref_count--;
			}
			else
			{
				res_fs_close(&opend_font[i].font_fp);
				memset(&opend_font[i], 0, sizeof(bitmap_font_t));

				if(bitmap_cache[i].inited == 1)
				{
					bitmap_cache[i].inited = 0;
				}
			}
		}


	}
}

#if USE_BSEARCH_IN_GLYPH_ID > 0
static int unicode_to_glyph_compare(const void *key, const void *element)
{
	const uint16_t *key16 = key;
	const uint16_t *elem16 = element;

	return *key16 - *elem16;
}
#endif

uint32_t _get_glyf_id_cached(bitmap_cache_t* cache, uint8_t* cmap_headers, uint32_t cmap_sub_count, uint32_t unicode)
{
	int32_t i;
	uint32_t glyf_id;
	cmap_sub_header_t* sub_header;

#if MAX_CACHED_GLYPH_IDS > 1
	if (unicode == cache->last_unicode[cache->last_unicode_idx])
		return cache->last_glyph_id[cache->last_unicode_idx];
	for (i = cache->last_unicode_idx - 1; i >= 0; i--) {
		if (unicode == cache->last_unicode[i])
			return cache->last_glyph_id[i];
	}

	for (i = cache->last_unicode_idx + 1; i < MAX_CACHED_GLYPH_IDS; i++) {
		if (unicode == cache->last_unicode[i])
			return cache->last_glyph_id[i];
	}
#else
	if (unicode == cache->last_unicode[0])
		return cache->last_glyph_id[0];
#endif

	sub_header = (cmap_sub_header_t*)cmap_headers;
//	SYS_LOG_INF("cmap_sub_count %d, cmap_sub_headers 0x%x\n", cmap_sub_count, cmap_headers);
	for(i=0;i<cmap_sub_count;i++)
	{
		if((unicode >= sub_header->range_start)&&
			(unicode < sub_header->range_start+sub_header->range_length))
		{
			//zero mini
			//SYS_LOG_INF("unicode 0x%x, start 0x%0.4x, length 0x%0.4x\n", unicode, sub_header->range_start, sub_header->range_length);

			if(sub_header->sub_format == 2)
			{
				glyf_id = sub_header->glyf_id_offset + (unicode - sub_header->range_start);
				goto ok_exit;
			}
			else if(sub_header->sub_format == 3)
			{
				uint16_t delta = (uint16_t)(unicode - sub_header->range_start);
				uint8_t* value8 = (uint8_t*)cmap_headers + sub_header->data_offset - 12;
				uint16_t* value = (uint16_t*)value8;

#if USE_BSEARCH_IN_GLYPH_ID > 0
				uint16_t *ptr16 = bsearch(&delta, value, sub_header->entry_count, 2, unicode_to_glyph_compare);
				if (ptr16) {
					glyf_id = sub_header->glyf_id_offset + (uint32_t)(ptr16 - value);
					goto ok_exit;
				}
#else
				int j;
				for (j = 0; j < sub_header->entry_count; j++) {
					if(value[j] == delta) {
						glyf_id = sub_header->glyf_id_offset + j;
						goto ok_exit;
					}
				}
#endif
			}
			else
			{
				SYS_LOG_INF("unsupported subformat %d\n", sub_header->sub_format);
			}
		}
		//how much space do cmap_sub_header_t occupy in reality
		sub_header++;
	}

	SYS_LOG_DBG("glyf id not found: 0x%x\n", unicode);
	return 1;
ok_exit:
#if MAX_CACHED_GLYPH_IDS > 1
	cache->last_unicode[cache->curr_unicode_idx] = unicode;
	cache->last_glyph_id[cache->curr_unicode_idx] = glyf_id;
	cache->curr_unicode_idx = (cache->curr_unicode_idx + 1) & (MAX_CACHED_GLYPH_IDS - 1);
	cache->last_unicode_idx = cache->curr_unicode_idx;
#else
	cache->last_unicode[0] = unicode;
	cache->last_glyph_id[0] = glyf_id;
#endif

	return glyf_id;
}

uint32_t _get_glyf_loca(bitmap_font_t* font, uint32_t glyf_id)
{
	uint32_t loca_offset;
	uint32_t glyf_offset;
	uint16_t glyf_offset16;

	if(font->loca_format == 0)
	{
		loca_offset = font->loca_offset+12+2*glyf_id;
	}
	else
	{
		loca_offset = font->loca_offset+12+4*glyf_id;
	}

//	SYS_LOG_INF("loca_origin 0x%x, loca_offset 0x%x, font fp 0x%x\n", font->loca_offset, loca_offset, font->font_fp.filep);
	res_fs_seek(&font->font_fp, loca_offset, FS_SEEK_SET);
	if(font->loca_format == 0)
	{
		res_fs_read(&font->font_fp, &glyf_offset16, 2);
		glyf_offset = glyf_offset16;
	}
	else
	{
		res_fs_read(&font->font_fp, &glyf_offset, 4);
	}
	return glyf_offset;
}

int _try_get_cached_index(bitmap_cache_t* cache, uint32_t glyf_id)
{
	int32_t cindex;

	if(cache == NULL)
	{
		SYS_LOG_ERR("null glyph cache\n");
		return -1;
	}

	if(cache->cached_total > 0)
	{
		if (glyf_id == cache->glyph_index[cache->last_glyph_idx]) {
			return cache->last_glyph_idx;
		}

		cindex = cache->last_glyph_idx + 1;
		for (; cindex < cache->cached_total; cindex++) {
			if (glyf_id == cache->glyph_index[cindex]) {
				cache->last_glyph_idx = cindex;
				return cindex;
			}
		}

		cindex = cache->last_glyph_idx - 1;
		for (; cindex >= 0; cindex--) {
			if (glyf_id == cache->glyph_index[cindex]) {
				cache->last_glyph_idx = cindex;
				return cindex;
			}
		}
	}

	return -1;
}

int _get_cache_index(bitmap_cache_t* cache, uint32_t glyf_id)
{
	if(cache == NULL)
	{
		SYS_LOG_ERR("null glyph cache\n");
		return -1;
	}

	if(cache->cached_total < cache->cached_max)
//	if(cache->cached_total < MAX_CACHED_GLYPH_BITMAPS)
	{
//		SYS_LOG_INF("current %d, current id %d\n", cache->current, cache->glyph_index[cache->current]);
		if(cache->glyph_index[cache->current] == 0)
		{
			cache->cached_total++;
			cache->glyph_index[cache->current] = glyf_id;
		}
		else
		{
			cache->cached_total++;
			cache->current++;
			cache->glyph_index[cache->current] = glyf_id;
		}
	}
	else
	{
		cache->current++;
		if(cache->current >= cache->cached_max)
//		if(cache->current > MAX_CACHED_GLYPH_BITMAPS)
		{
			cache->current = 0;
		}
		cache->glyph_index[cache->current] = glyf_id;
	}

	cache->last_glyph_idx = cache->current;

	return cache->current;
}

uint8_t* _get_glyph_cache(bitmap_cache_t* cache, uint32_t glyf_id)
{
	uint32_t cache_index;
	if(cache == NULL)
	{
		SYS_LOG_ERR("null glyph cache\n");
		return NULL;
	}

	cache_index = _try_get_cached_index(cache, glyf_id);
	return &(cache->data[cache_index*cache->unit_size]);
}

int32_t _check_high_freq_chars(uint16_t unicode)
{
	uint16_t key = unicode;

	if(unicode == high_freq_cache.last_unicode_idx)
	{
		return high_freq_cache.last_cache_idx;
	}
	uint16_t *ptr16 = bsearch(&key, high_freq_codes, MAX_HIGH_FREQ_NUM, 2, unicode_to_glyph_compare);
	if(ptr16)
	{
		int32_t cache_index = ptr16 - high_freq_codes;
		high_freq_cache.last_unicode_idx = unicode;
		high_freq_cache.last_cache_idx = cache_index;
		return cache_index;
	}
	else
	{
		return -1;
	}
}

uint32_t bitmap_font_get_elapse_count(void)
{
	return elapse_count;
}

uint8_t * bitmap_font_get_emoji_bitmap(bitmap_emoji_font_t* font, uint32_t unicode)
{
	uint8_t* data;

	data = _get_glyph_cache(font->cache, unicode);
	return data;
}


int32_t _search_emoji_glyf_id(bitmap_emoji_font_t* font, uint32_t unicode, uint32_t* glyf_id)
{
	emoji_font_entry_t* entry = font->glyf_list;
	int low, mid, high;

	low = 0;
	high = font->glyf_count-1;
	while(low <= high)
	{
		mid = (low+high)/2;
		if(entry[mid].unicode > unicode)
		{
			high = mid - 1;
		}
		else if(entry[mid].unicode < unicode)
		{
			low = mid + 1;
		}
		else
		{
			//found
			*glyf_id = mid;
			return 0;
		}
	}

	return -1;
}

glyph_metrics_t* _font_get_emoji_glyph_dsc(bitmap_emoji_font_t* font, uint32_t unicode, int32_t cache_index)
{
	int32_t glyf_id; //here refers to the index in glyf list
	uint32_t glyf_loca;
	uint32_t glyf_size;
	glyph_metrics_t* metric_item = NULL;
	emoji_font_entry_t* entry = NULL;
	uint8_t* data;
	int ret;

	if(font == NULL)
	{
		SYS_LOG_ERR("null font info %p\n", font);
		return NULL;
	}

	ret = _search_emoji_glyf_id(font, unicode, &glyf_id);
	if(ret < 0)
	{
		SYS_LOG_ERR("unknown emoji 0x%x\n", unicode);
		return NULL;
	}

	SYS_LOG_INF("emoji glyf id %d\n", glyf_id);
	metric_item = &(font->cache->metrics[cache_index]);
	data = _get_glyph_cache(font->cache, unicode);

	entry = font->glyf_list + glyf_id;
	metric_item->advance = entry->width;
	metric_item->bbx = 0;
	metric_item->bby = 0;
	metric_item->bbw = entry->width;
	metric_item->bbh = entry->height;
	glyf_size = entry->width*entry->height*font->bpp;
	glyf_loca = entry->offset;
	ret = res_fs_seek(&font->font_fp, glyf_loca, FS_SEEK_SET);
	if(ret < 0)
	{
		SYS_LOG_ERR("seek font file error\n");
		return NULL;
	}

	ret = res_fs_read(&font->font_fp, data, glyf_size);
	if(ret < glyf_size)
	{
		SYS_LOG_ERR("read font file error\n");
		return NULL;
	}
	return metric_item;

}

glyph_metrics_t* bitmap_font_get_emoji_glyph_dsc(bitmap_emoji_font_t* font, uint32_t unicode)
{
	int32_t cache_index;
	glyph_metrics_t* metric_item;

	if(font == NULL)
	{
		SYS_LOG_ERR("null font info, %p\n", font);
		return NULL;
	}

	cache_index = _try_get_cached_index(font->cache, unicode);
	if(cache_index > 0)
	{
		//found cached, fill data to glyph dsc

		return &font->cache->metrics[cache_index];
	}
	else
	{
		cache_index = _get_cache_index(font->cache, unicode);
		printf("cache_index %d\n", cache_index);
		metric_item = _font_get_emoji_glyph_dsc(font, unicode, cache_index);
		if(metric_item == NULL)
		{
			SYS_LOG_ERR("no metric item found for 0x%x\n", unicode);
			return NULL;
		}
		return metric_item;
	}

	return NULL;
}

uint8_t * bitmap_font_get_bitmap(bitmap_font_t* font, bitmap_cache_t* cache, uint32_t unicode)
{
	uint32_t glyf_id;
	uint8_t* data;

	if(cache == NULL)
	{
		SYS_LOG_ERR("null cache info, %p\n", cache);
		return NULL;
	}

	if(unicode == 0)
	{
		unicode = 0x20;
	}


	if(bitmap_font_get_high_freq_enabled())
	{
		int cache_index;
		if(font->font_size == high_freq_cache.font_size)
		{
			cache_index = _check_high_freq_chars((uint16_t)unicode);
	//		SYS_LOG_INF("unicode 0x%x, cache_index %d\n", unicode, cache_index);
			if(cache_index >= 0)
			{
				return high_freq_cache.data+cache_index*high_freq_cache.unit_size;
			}
		}
	}
	
	glyf_id = _get_glyf_id_cached(cache, font->cmap_sub_headers, font->cmap_sub_count, unicode);
	data = _get_glyph_cache(cache, glyf_id);

	return data;
}


glyph_metrics_t* _font_get_glyph_dsc(bitmap_font_t* font, bitmap_cache_t* cache, high_freq_cache_t* hcache,  uint32_t glyf_id, int32_t cache_index, uint32_t load_cache_type)
{
	uint32_t glyf_loca;
	uint32_t metric_len;
	uint32_t metric_off;
	uint32_t bits_total;
	uint32_t bits_off;
	uint32_t off;
	uint32_t bmp_size;
	uint8_t* data = NULL;
	uint8_t* metrics;
	bbxy_t tmp;
	glyph_metrics_t* metric_item = NULL;
	int ret;

	if(font == NULL)
	{
		SYS_LOG_ERR("null font info, %p, %p\n", font, cache);
		return NULL;
	}


	glyf_loca = _get_glyf_loca(font, glyf_id);
	ret = res_fs_seek(&font->font_fp, glyf_loca+font->glyf_offset, FS_SEEK_SET);
	if(ret < 0)
	{
		SYS_LOG_ERR("seek font file error\n");
		return NULL;
	}


	bits_total = font->adw_length+2*font->bbxy_length+2*font->bbwh_length;
	metric_len = (bits_total+7)/8;

	metrics = (uint8_t*)metrics32;
	ret = res_fs_read(&font->font_fp, metrics, metric_len);
	if(ret < metric_len)
	{
		SYS_LOG_ERR("read font file error\n");
		return NULL;
	}
	metric_off = 0;

	if(cache != NULL)
	{
		metric_item = &(cache->metrics[cache_index]);
		data = _get_glyph_cache(cache, glyf_id);
	}
	else if(hcache != NULL)
	{
		metric_item = &(hcache->metrics[cache_index]);
		data = hcache->data+hcache->unit_size*cache_index;
	}

	if(font->adw_length <= 8)
	{
		if(font->adw_length == 0)
		{
			metric_item->advance = font->default_advance;
		}
		else
		{
			metric_item->advance = (uint32_t)(metrics[0]>>(8-font->adw_length));
		}
		bits_off = font->adw_length;
	}
	else
	{
		//suppose adw length less than 16
		if(font->adw_format == 1)
		{
			uint32_t adw_len = font->adw_length-4;
			if(adw_len <= 8)
			{
				metric_item->advance = (uint32_t)(metrics[0]>>(8-adw_len));
			}
			else
			{
				metric_item->advance = (uint32_t)((metrics[0]<<(adw_len-8))|(metrics[1]>>(16-adw_len)));
			}
			metric_off++;
		}
		else
		{
			metric_item->advance = (uint32_t)((metrics[0]<<(font->adw_length-8))|(metrics[1]>>(16-font->adw_length)));
			metric_off++;
		}
		bits_off = font->adw_length - 8;
	}

	if(font->bbxy_length <= 8 - bits_off)
	{
		tmp.xy = (metrics[metric_off]<<bits_off)>>(8-font->bbxy_length);
		switch(font->bbxy_length)
		{
		case 2:
			metric_item->bbx = tmp.bits2;
			break;
		case 3:
			metric_item->bbx = tmp.bits3;
			break;
		case 4:
			metric_item->bbx = tmp.bits4;
			break;
		case 5:
			metric_item->bbx = tmp.bits5;
			break;
		case 6:
			metric_item->bbx = tmp.bits6;
			break;
		case 7:
			metric_item->bbx = tmp.bits7;
			break;
		default:
			metric_item->bbx = tmp.xy;
			break;
		}
		bits_off = bits_off + font->bbxy_length;
		if(bits_off == 8)
		{
			bits_off = 0;
			metric_off++;
		}
	}
	else
	{
		uint8_t p1 = (metrics[metric_off]<<bits_off);
		p1 = p1>>bits_off;
		metric_off++;
		uint8_t p2 = (metrics[metric_off]>>(16- font->bbxy_length - bits_off));
		tmp.xy = p1<<(font->bbxy_length + bits_off - 8) | p2;
		switch(font->bbxy_length)
		{
		case 2:
			metric_item->bbx = tmp.bits2;
			break;
		case 3:
			metric_item->bbx = tmp.bits3;
			break;
		case 4:
			metric_item->bbx = tmp.bits4;
			break;
		case 5:
			metric_item->bbx = tmp.bits5;
			break;
		case 6:
			metric_item->bbx = tmp.bits6;
			break;
		case 7:
			metric_item->bbx = tmp.bits7;
			break;
		default:
			metric_item->bbx = tmp.xy;
			break;
		}
		bits_off = font->bbxy_length + bits_off - 8;
		if(bits_off == 8)
		{
			bits_off = 0;
			metric_off++;
		}
	}

	if(font->bbxy_length <= 8 - bits_off)
	{
		tmp.xy = (metrics[metric_off]<<bits_off)>>(8-font->bbxy_length);
		switch(font->bbxy_length)
		{
		case 2:
			metric_item->bby = tmp.bits2;
			break;
		case 3:
			metric_item->bby = tmp.bits3;
			break;
		case 4:
			metric_item->bby = tmp.bits4;
			break;
		case 5:
			metric_item->bby = tmp.bits5;
			break;
		case 6:
			metric_item->bby = tmp.bits6;
			break;
		case 7:
			metric_item->bby = tmp.bits7;
			break;
		default:
			metric_item->bby = tmp.xy;
			break;
		}
		bits_off = bits_off + font->bbxy_length;
		if(bits_off == 8)
		{
			bits_off = 0;
			metric_off++;
		}
	}
	else
	{
		uint8_t p1 = (metrics[metric_off]<<bits_off);
		p1 = p1>>bits_off;
		metric_off++;
		uint8_t p2 = (metrics[metric_off]>>(16- font->bbxy_length - bits_off));
		tmp.xy = p1<<(font->bbxy_length + bits_off - 8) | p2;
		switch(font->bbxy_length)
		{
		case 2:
			metric_item->bby = tmp.bits2;
			break;
		case 3:
			metric_item->bby = tmp.bits3;
			break;
		case 4:
			metric_item->bby = tmp.bits4;
			break;
		case 5:
			metric_item->bby = tmp.bits5;
			break;
		case 6:
			metric_item->bby = tmp.bits6;
			break;
		case 7:
			metric_item->bby = tmp.bits7;
			break;
		default:
			metric_item->bby = tmp.xy;
			break;
		}

		bits_off = font->bbxy_length + bits_off - 8;
		if(bits_off == 8)
		{
			bits_off = 0;
			metric_off++;
		}

	}

	if(font->bbwh_length <= 8 - bits_off)
	{
		uint8_t p1 = (metrics[metric_off]<<bits_off);
		metric_item->bbw = p1>>(8-font->bbwh_length);
//		metric_item->bbw = (metrics[metric_off]<<bits_off)>>bits_off;
		bits_off = bits_off + font->bbwh_length;
		if(bits_off == 8)
		{
			bits_off = 0;
			metric_off++;
		}

	}
	else
	{
		uint8_t p1 = (metrics[metric_off]<<bits_off);
		p1 = p1>>bits_off;
		metric_off++;
		uint8_t p2 = (metrics[metric_off]>>(16- font->bbwh_length - bits_off));
		metric_item->bbw = p1<<(font->bbwh_length + bits_off - 8) | p2;
		bits_off = font->bbwh_length + bits_off - 8;
		if(bits_off == 8)
		{
			bits_off = 0;
			metric_off++;
		}

	}

	if(font->bbwh_length <= 8 - bits_off)
	{
		uint8_t p1 = (metrics[metric_off]<<bits_off);
		metric_item->bbh = p1>>(8-font->bbwh_length);
		bits_off = bits_off + font->bbwh_length;
		if(bits_off == 8)
		{
			bits_off = 0;
			metric_off++;
		}
	}
	else
	{
		uint8_t p1 = (metrics[metric_off]<<bits_off);
		p1 = p1>>bits_off;
		metric_off++;
		uint8_t p2 = (metrics[metric_off]>>(16- font->bbwh_length - bits_off));
		metric_item->bbh = p1<<(font->bbwh_length + bits_off - 8) | p2;
		bits_off = font->bbwh_length + bits_off - 8;
		if(bits_off == 8)
		{
			bits_off = 0;
			metric_off++;
		}

	}


	off = 0;
	bmp_size = metric_item->bbw*metric_item->bbh;
	if(font->bpp == 4)
	{
		if(bmp_size%2 != 0)
		{
			bmp_size = bmp_size/2 + 1;
		}
		else
		{
			bmp_size = bmp_size/2;
		}
	}
	else if(font->bpp == 2)
	{
		if(bmp_size%4 != 0)
		{
			bmp_size = bmp_size/4 + 1;
		}
		else
		{
			bmp_size = bmp_size/4;
		}
	}
	else if(font->bpp == 1)
	{
		if(bmp_size%8 != 0)
		{
			bmp_size = bmp_size/8 + 1;
		}
		else
		{
			bmp_size = bmp_size/4;
		}
	}

	if(bits_off != 0)
	{
		res_fs_read(&font->font_fp, data+4, bmp_size);
		data[off] =  (metrics[metric_off]<<bits_off)|(data[4]>>(8-bits_off));
		off++;
		while(off < bmp_size)
		{
			data[off] = (data[off+3]<<bits_off)|(data[off+4]>>(8-bits_off));
			off++;
		}
	}
	else
	{
		res_fs_read(&font->font_fp, data, bmp_size);
	}

//	SYS_LOG_INF("adw %d x %d y %d w %d h %d\n", metric_item->advance, metric_item->bbx, metric_item->bby, metric_item->bbw, metric_item->bbh);
	return metric_item;

}

glyph_metrics_t* bitmap_font_get_glyph_dsc(bitmap_font_t* font, bitmap_cache_t *cache, uint32_t unicode)
{
	uint32_t glyf_id;
	int32_t cache_index;
	glyph_metrics_t* metric_item;

	if((font == NULL) || (cache == NULL))
	{
		SYS_LOG_ERR("null font info, %p, %p\n", font, cache);
		return NULL;
	}

	if(unicode == 0)
	{
		unicode = 0x20;
	}


	if(bitmap_font_get_high_freq_enabled())
	{
		if(font->font_size == high_freq_cache.font_size)
		{
			cache_index = _check_high_freq_chars((uint16_t)unicode);
			if(cache_index >= 0)
			{
				return &(high_freq_cache.metrics[cache_index]);
			}
		}
	}
	
	glyf_id = _get_glyf_id_cached(cache, font->cmap_sub_headers, font->cmap_sub_count, unicode);
	
	cache_index = _try_get_cached_index(cache, glyf_id);
	if(cache_index >= 0)
	{
		//found cached, fill data to glyph dsc
		return &cache->metrics[cache_index];
	}
	else
	{
//		SYS_LOG_INF("no cache 0x%x\n", unicode);
		cache_index = _get_cache_index(cache, glyf_id);
		metric_item = _font_get_glyph_dsc(font, cache, NULL, glyf_id, cache_index, CACHE_TYPE_NORMAL);
		if(metric_item == NULL)
		{
			return NULL;
		}
//		SYS_LOG_INF("metrics %d %d %d %d\n", metric_item->bbx, metric_item->bby, metric_item->bbw, metric_item->bbh);
		//FIXME: no way to adjust vertical position of some letters automatically
		if(unicode == 0x4e00  || unicode == 0x2014 || unicode == 0xbbd2)
		{
			int32_t diff = metric_item->bby - metric_item->bbh;
			if(diff > font->font_size/2)
			{
				metric_item->bby -= font->font_size/3;
			}
		}
		return metric_item;
	}

}

void bitmap_font_load_high_freq_chars(const uint8_t* file_path)
{
#ifndef CONFIG_SIMULATOR
	uint32_t glyf_id;
	uint32_t i;
	glyph_metrics_t* metric_item;
	int ret;
#ifdef CONFIG_RES_MANAGER_USE_SDFS
	struct sd_file* hffp;
#else
	struct fs_file_t hffp;
#endif
	bitmap_font_t* font = NULL;
	int unitsize = 0;

	char* high_freq_bin_path;
#ifndef	CONFIG_RES_MANAGER_USE_SDFS	
	char* bin_name;
#endif

	if(!bitmap_font_get_high_freq_enabled())
	{
		return;
	}


	if(file_path == NULL)
	{
		SYS_LOG_INF("null font path\n");
		return;
	}
	
	if(high_freq_cache.font_size != 0)
	{
		SYS_LOG_INF("high freq bin loaded\n");
		return;
	}


#ifdef CONFIG_RES_MANAGER_USE_SDFS
	high_freq_bin_path = DEF_HIGH_FREQ_BIN;
#else
	high_freq_bin_path = (char*)mem_malloc(strlen(file_path)+12);
	if(high_freq_bin_path == NULL)
	{
		SYS_LOG_INF("high freq bin load failed\n");
		return;
	}
	memset(high_freq_bin_path, 0, strlen(file_path)+12);
	strcpy(high_freq_bin_path, file_path);
	bin_name = strrchr(high_freq_bin_path, '/')+1;
	strcpy(bin_name, DEF_HIGH_FREQ_BIN);
#endif

#ifndef	CONFIG_RES_MANAGER_USE_SDFS
	memset(&hffp, 0, sizeof(struct fs_file_t));
#endif
	ret = res_fs_open(&hffp, high_freq_bin_path);
    if ( ret < 0 )
    {
        SYS_LOG_ERR(" open high freq file %s failed! %d \n", high_freq_bin_path, ret);
#ifndef CONFIG_RES_MANAGER_USE_SDFS
		mem_free(high_freq_bin_path);
#endif
		return;
    }	

	ret = res_fs_read(&hffp, high_freq_codes, sizeof(uint16_t)*MAX_HIGH_FREQ_NUM);
	if(ret < sizeof(uint16_t)*MAX_HIGH_FREQ_NUM)
	{
		SYS_LOG_ERR("read high freq codes failed\n");
		res_fs_close(&hffp);
#ifndef CONFIG_RES_MANAGER_USE_SDFS		
		mem_free(high_freq_bin_path);
#endif
		return;
	}
	res_fs_close(&hffp);
#ifndef CONFIG_RES_MANAGER_USE_SDFS
	mem_free(high_freq_bin_path);
#endif
	
	font = bitmap_font_open(file_path);
	if(font == NULL)
	{
		SYS_LOG_ERR("failed to open font file to load high freqency codes");
		return;
	}


	if(font->bpp == 2)
	{
		unitsize = (font->font_size*font->font_size)/4;
		unitsize = ((unitsize+3)/4)*4;
		high_freq_cache.unit_size = unitsize;
	}
	else if(font->bpp == 1)
	{
		unitsize = (font->font_size*font->font_size)/8;
		unitsize = ((unitsize+7)/8)*8;
		high_freq_cache.unit_size = unitsize;
	}
	else
	{
		SYS_LOG_INF("not support high freq cache of bpp %d yet", font->bpp);
		return;
	}
	SYS_LOG_INF("high freq fontsize %d, unitsize %d\n", high_freq_cache.font_size, high_freq_cache.unit_size);
	for(i=0;i<MAX_HIGH_FREQ_NUM;i++)
	{
		glyf_id = _get_glyf_id_cached(font->cache, font->cmap_sub_headers, font->cmap_sub_count, (uint32_t)high_freq_codes[i]);
		metric_item = _font_get_glyph_dsc(font, NULL, &high_freq_cache, glyf_id, i, CACHE_TYPE_HIGH_FREQ);
		if(metric_item == NULL)
		{
			SYS_LOG_ERR("high freq char not found 0x%x\n", high_freq_codes[i]);
			continue;
		}

		//FIXME: no way to adjust vertical position of some letters automatically
		if(high_freq_codes[i] == 0x4e00  || high_freq_codes[i] == 0x2014 || high_freq_codes[i] == 0xbbd2)
		{
			int32_t diff = metric_item->bby - metric_item->bbh;
			if(diff > font->font_size/2)
			{
				metric_item->bby -= font->font_size/3;
			}
		}

	}

	SYS_LOG_INF("high freq preview:\n");
	for(i=0;i<16;i++)
	{
		SYS_LOG_INF("0x%x, ", high_freq_codes[i]);
	}
	SYS_LOG_INF("\n");
	high_freq_cache.font_size = font->font_size;
	for(i='0';i<='9';i++)
	{
		bitmap_font_get_glyph_dsc(font, font->cache, i);
	}
	for(i='a';i<='z';i++)
	{
		bitmap_font_get_glyph_dsc(font, font->cache, i);
	}
	for(i='A';i<='Z';i++)
	{
		bitmap_font_get_glyph_dsc(font, font->cache, i);
	}

//	bitmap_font_get_glyph_dsc(font, font->cache, 0x2e);
	bitmap_font_get_glyph_dsc(font, font->cache, 0xff0c);
	bitmap_font_get_glyph_dsc(font, font->cache, 0x3002);
//	bitmap_font_close(font);

#else //SIMULATOR
	return;
#endif
}


