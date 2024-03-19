#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr.h>
#include <os_common_api.h>
#include "freetype_font_api.h"
#include <logging/log.h>

static FT_Library ft_library;


static FT_Error  font_Face_Requester(FTC_FaceID  face_id,
                         FT_Library  library,
                         FT_Pointer  req_data,
                         FT_Face*    aface)
{
    *aface = face_id;

    return FT_Err_Ok;
}



#define FT_FONT_CACHE_SIZE					1024*64

#define FT_FONT_PSRAM_SIZE					1024*144


#define MAX_OPEND_FONT						2


static freetype_cache_t freetype_cache[MAX_OPEND_FONT];
static freetype_font_t opend_font[MAX_OPEND_FONT];

static char __aligned(4) ft_font_cache_buffer[FT_FONT_PSRAM_SIZE] __in_section_unique(FTFONT_PSRAM_REGION);


int _freetype_cache_init(freetype_cache_t* cache, uint32_t slot)
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

	cache_start = ft_font_cache_buffer + FT_FONT_CACHE_SIZE*slot;
	if(cache_start == NULL)
	{
		SYS_LOG_ERR("no memory for font cache\n");
		return -1;
	}
	memset(cache, 0, sizeof(freetype_cache_t));
	SYS_LOG_INF("cache_start %p\n", cache_start);
	cache->glyph_index = (uint32_t*)cache_start;
	memset(cache->glyph_index, 0, MAX_CACHE_NUM*sizeof(uint32_t));
	cache->metrics = (bbox_metrics_t*)(cache_start + MAX_CACHE_NUM*sizeof(uint32_t));
	cache->data = cache_start + MAX_CACHE_NUM*sizeof(uint32_t) + MAX_CACHE_NUM*sizeof(bbox_metrics_t);
	cache->inited = 1;

	SYS_LOG_INF("metrics_buf %p, data_buf %p\n", cache->metrics, cache->data);
	return 0;
}

int freetype_font_init(void)
{
	FT_Error error;
	printf("xxxxxxxx \n");
	error = FT_Init_FreeType(&ft_library);
	if ( error )
	{
		printf("Error in FT_Init_FreeType: %d\n", error);
		return error;
	}
	return FT_Err_Ok;

}

int freetype_font_deinit(void)
{
	int32_t i;

	for(i=0;i<MAX_OPEND_FONT;i++)
	{
		if(opend_font[i].face != NULL)
		{
			freetype_font_close(&opend_font[i]);
			memset(&opend_font[i], 0, sizeof(freetype_font_t));
		}
		
		if(freetype_cache[i].inited == 1)
		{
			freetype_cache[i].inited = 0;
		}		
		
	}

	return 0;
}


freetype_cache_t* freetype_font_get_cache(freetype_font_t* font)
{
	return font->cache;
}


freetype_font_t* freetype_font_open(const char* file_path, uint32_t font_size)
{
	int32_t ret;
	int32_t error;
	uint32_t i;
	int32_t empty_slot = -1;
	freetype_font_t* ft_font = NULL;

	for(i=0;i<MAX_OPEND_FONT;i++)
	{
		if(opend_font[i].face != NULL)
		{
			if((strcmp(opend_font[i].font_path, file_path) == 0) &&
				(opend_font[i].font_size == font_size))
			{
				ft_font = &opend_font[i];
				ft_font->ref_count++;
				return ft_font;
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
		ft_font = &opend_font[empty_slot];
	}
	else
	{
		SYS_LOG_ERR("no available slot for freetype font %s", file_path);
		return NULL;
	}

	ret = _freetype_cache_init(&freetype_cache[empty_slot], empty_slot);
	if(ret < 0)
	{
		SYS_LOG_ERR("init cache failed for freetype font");
		goto ERR_EXIT;
	}

	memset(ft_font, 0, sizeof(freetype_font_t));
	ft_font->cache = &freetype_cache[empty_slot];

	error = FT_New_Face(ft_library, file_path, 0, &ft_font->face);
	if ( error ) {
		SYS_LOG_ERR("Error in FT_New_Face: %d\n", error);
		return NULL;
	}

	error = FT_Select_Charmap(ft_font->face, FT_ENCODING_UNICODE);
	if (error)
	{
		SYS_LOG_ERR("freetype select charmap failed: %d\n", error);
		return NULL;
	}

	printf("%s %d\n", __FILE__, __LINE__);
	error = FT_Set_Pixel_Sizes(ft_font->face, 0,font_size);
	if ( error ) {
		SYS_LOG_ERR("Error in FT_Set_Char_Size: %d\n", error);
		return NULL;
	}
	printf("%s %d\n", __FILE__, __LINE__);

	ft_font->font_size = font_size;
	ft_font->ref_count = 1;

	strcpy(ft_font->font_path, file_path);
	return ft_font;
ERR_EXIT:

	memset(ft_font, 0, sizeof(freetype_font_t));
	return NULL;
}

void freetype_font_close(freetype_font_t* ft_font)
{
	int32_t i;
	
	if(ft_font != NULL)
	{
		for(i=0;i<MAX_OPEND_FONT;i++)
		{
			if(ft_font == &opend_font[i])
			{
				break;				
			}
		}

		if(i >= MAX_OPEND_FONT)
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
				FT_Done_Face(opend_font[i].face);
				memset(&opend_font[i], 0, sizeof(freetype_font_t));	

				if(freetype_cache[i].inited == 1)
				{
					freetype_cache[i].inited = 0;
				}				
			}
		}

	
	}
}

uint32_t _ft_get_glyf_id_cached(FT_Face face, uint32_t unicode)
{
	uint32_t i;
	uint32_t glyf_id;
	
	glyf_id = FT_Get_Char_Index( face, unicode );

	return glyf_id;
}


int _ft_try_get_cached_index(freetype_cache_t* cache, uint32_t glyf_id)
{
	int32_t cindex;

	if(cache == NULL)
	{
		SYS_LOG_ERR("null glyph cache\n");
		return -1;
	}

	if(cache->cached_total > 0)
	{
		cindex = cache->current;
		while(cindex >= 0)
		{
			if(glyf_id != cache->glyph_index[cindex])
			{
				cindex--;
				if(cindex < 0)
				{
					break;
				}
			}
			else
			{
				break;
			}
		}

		if(cindex >= 0)
		{
			//cached found
			return cindex;
		}
		else
		{	
			if(cache->cached_total - cache->current > 0)
			{
				//ring buffer, maybe there's something in tail part
				cindex = MAX_CACHE_NUM-1;
				while(cindex > cache->current)
				{
					if(glyf_id != cache->glyph_index[cindex])
					{
						cindex--;
						if(cindex <= cache->current)
						{
							break;
						}
					}
					else
					{
						break;
					}					
				}

				if(cindex > cache->current)
				{
					//cached found
					return cindex;
				}
			}
		}
		
	}
	return -1;
}

int _ft_get_cache_index(freetype_cache_t* cache, uint32_t glyf_id)
{
	int32_t cindex;
	
	if(cache == NULL)
	{
		SYS_LOG_ERR("null glyph cache\n");
		return -1;
	}

	if(cache->cached_total < MAX_CACHE_NUM)
	{
//		SYS_LOG_INF("current %d, current id %d\n", cache->current, cache->glyph_index[cache->current]);
		if(cache->glyph_index[cache->current] == 0)
		{
			cache->cached_total++;
			cache->glyph_index[cache->current] = glyf_id;
			return cache->current;
		}
		else
		{
			cache->cached_total++;
			cache->current++;
			cache->glyph_index[cache->current] = glyf_id;
			return cache->current;
		}
	}
	else
	{
		cache->current++;
		if(cache->current > MAX_CACHE_NUM)
		{
			cache->current = 0;
		}
		cache->glyph_index[cache->current] = glyf_id;
		return cache->current;		
	}
}

uint8_t* _ft_get_glyph_cache(freetype_cache_t* cache, uint32_t font_size, uint32_t glyf_id)
{
	uint32_t cache_index;
	uint32_t bytes_per_glyph;
	if(cache == NULL)
	{
		SYS_LOG_ERR("null glyph cache\n");
		return NULL;
	}

	bytes_per_glyph = font_size*font_size;
	cache_index = _ft_try_get_cached_index(cache, glyf_id);
	if(cache_index >= 0)
	{
		return &(cache->data[cache_index*bytes_per_glyph]);
	}

	cache_index = _ft_get_cache_index(cache, glyf_id);
	if(cache_index < 0)
	{
		SYS_LOG_ERR("cant get cache index for unicode 0x%x", glyf_id);
		return NULL;
	}
	return &(cache->data[cache_index*bytes_per_glyph]);
}

uint8_t * freetype_font_get_bitmap(freetype_font_t* font, freetype_cache_t* cache, uint32_t unicode)
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

	glyf_id = _ft_get_glyf_id_cached(font->face, unicode);
	data = _ft_get_glyph_cache(cache, font->font_size, glyf_id);

	return data;
}


bbox_metrics_t* freetype_font_get_glyph_dsc(freetype_font_t* font, freetype_cache_t *cache, uint32_t unicode)
{
	uint32_t glyf_id;
	uint32_t bytes_per_glyph;
	uint8_t* data;
	int32_t cache_index;
	uint8_t* glyph_buf;
	uint8_t* metrics;	
	int error;
	FT_Face face;

	if((font == NULL) || (cache == NULL))
	{
		SYS_LOG_ERR("null font info, %p, %p\n", font, cache);
		return NULL;
	}

	if(unicode == 0)
	{
		unicode = 0x20;
	}

	face = font->face;
	glyf_id = _ft_get_glyf_id_cached(face, unicode);
	if(glyf_id  < 0)
	{
		SYS_LOG_ERR("unknown unicode 0x%x", unicode);
		return NULL;
	}

	cache_index = _ft_try_get_cached_index(cache, glyf_id);
	if(cache_index >= 0)
	{
		//found cached, fill data to glyph dsc
		return &cache->metrics[cache_index];
	}

	cache_index = _ft_get_cache_index(cache, glyf_id);
	if(cache_index < 0)
	{
		SYS_LOG_ERR("cant get cache index for unicode 0x%x", unicode);
		return NULL;
	}

	bytes_per_glyph = font->font_size*font->font_size;
	data = &(cache->data[cache_index*bytes_per_glyph]);
	error = FT_Load_Glyph(
		 face,			/* handle to face object */
		 glyf_id,	/* glyph index			 */
		 FT_LOAD_DEFAULT );  /* load flags, see below *///FT_LOAD_MONOCHROME|FT_LOAD_NO_AUTOHINTING|FT_LOAD_NO_HINTING
	if ( error )
	{
		SYS_LOG_ERR("Error in FT_Load_Glyph: %d\n", error);
		return error;
	}

	error = FT_Render_Glyph( face->glyph,	 /* glyph slot	*/
		 FT_RENDER_MODE_NORMAL ); /* render mode */ //
	if ( error )
	{
		SYS_LOG_ERR("Error in FT_Render_Glyph: %d\n", error);
		return error;
	}
//	printf("pitch %d, width %d, rows %d\n", face->glyph->bitmap.pitch, face->glyph->bitmap.width, face->glyph->bitmap.rows);
	memcpy(data, face->glyph->bitmap.buffer, face->glyph->bitmap.pitch*face->glyph->bitmap.rows);

	cache->metrics[cache_index].advance = (face->glyph->metrics.horiAdvance >> 6);
	cache->metrics[cache_index].bbh = face->glyph->bitmap.rows; 		/*Height of the bitmap in [px]*/
	cache->metrics[cache_index].bbw = face->glyph->bitmap.width;		 /*Width of the bitmap in [px]*/
	cache->metrics[cache_index].bbx = face->glyph->bitmap_left; 		/*X offset of the bitmap in [pf]*/
	cache->metrics[cache_index].bby = face->glyph->bitmap_top - face->glyph->bitmap.rows;		  /*Y offset of the bitmap measured from the as line*/

//	SYS_LOG_INF("unicode 0x%x, adw %d x %d y %d w %d h %d\n", unicode, cache->metrics[cache_index].advance, cache->metrics[cache_index].bbx, cache->metrics[cache_index].bby, cache->metrics[cache_index].bbw, cache->metrics[cache_index].bbh);
	return &cache->metrics[cache_index];
	
}

