#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <os_common_api.h>
#include <lvgl/lvgl_freetype_font.h>


bool freetype_font_get_glyph_dsc_cb(const lv_font_t * lv_font, lv_font_glyph_dsc_t * dsc_out, uint32_t unicode, uint32_t unicode_next)
{	
	bbox_metrics_t* metric;
	lv_font_fmt_freetype_dsc_t * dsc = (lv_font_fmt_freetype_dsc_t *)(lv_font->user_data);
	
//	printf("dsc 0x%x, font 0x%x, unicode 0x%x\n", dsc, dsc->font, unicode);
	if(unicode < 0x20) 
	{
		dsc_out->adv_w = 0;
		dsc_out->box_h = 0;
		dsc_out->box_w = 0;
		dsc_out->ofs_x = 0;
		dsc_out->ofs_y = 0;
		dsc_out->bpp = 0;
		return true;
	}

	metric = freetype_font_get_glyph_dsc(dsc->font, dsc->cache, unicode);
#if 1	 
	 dsc_out->adv_w = metric->advance;
	 dsc_out->box_h = metric->bbh; 		/*Height of the bitmap in [px]*/
	 dsc_out->box_w = metric->bbw;		 /*Width of the bitmap in [px]*/
	 dsc_out->ofs_x = metric->bbx; 		/*X offset of the bitmap in [pf]*/
	 dsc_out->ofs_y = metric->bby;		  /*Y offset of the bitmap measured from the as line*/
//	 dsc_out->ofs_y = 0;
//	 printf("metrics %d %d %d %d\n", dsc_out->box_h, dsc_out->box_w, dsc_out->ofs_x, dsc_out->ofs_y);
	 /* see FT_Pixel_Mode: 1-FT_PIXEL_MODE_MONO; 2-FT_PIXEL_MODE_GRAY */
	 dsc_out->bpp = 8;		   /*Bit per pixel: 1/2/4/8*/
#else
	dsc_out->adv_w = 24;
	dsc_out->box_h = 24;		   /*Height of the bitmap in [px]*/
	dsc_out->box_w = 24; 		/*Width of the bitmap in [px]*/
	dsc_out->ofs_x = 0;		   /*X offset of the bitmap in [pf]*/
	dsc_out->ofs_y = 0;		 /*Y offset of the bitmap measured from the as line*/
	/* see FT_Pixel_Mode: 1-FT_PIXEL_MODE_MONO; 2-FT_PIXEL_MODE_GRAY */
	dsc_out->bpp = 8;		  /*Bit per pixel: 1/2/4/8*/
	printf("\n");
#endif
	 return true;
}

/* Get the bitmap of `unicode_letter` from `font`. */
static const uint8_t * freetype_font_get_glyph_bitmap_cb(const lv_font_t * font, uint32_t unicode)
{
	uint8_t* data;
	lv_font_fmt_freetype_dsc_t* font_dsc;

	if(font == NULL)
	{
		LV_LOG_ERROR("null font info, 0x%x\n", font);
		return NULL;
	}
	font_dsc = (lv_font_fmt_freetype_dsc_t*)font->user_data;

	data = freetype_font_get_bitmap(font_dsc->font, font_dsc->cache, unicode);
	return data;

}


int lvgl_freetype_font_init(void)
{
	int32_t ret;
	ret = freetype_font_init();
	if(ret != 0)
	{
		LV_LOG_ERROR("freetype library init failed\n");
	}
	return ret;
}

int lvgl_freetype_font_deinit(void)
{
	freetype_font_deinit();
	return 0;
}

int lvgl_freetype_font_open(lv_font_t* font, const char * font_path, uint32_t font_size)
{
	FT_Error error;

	lv_font_fmt_freetype_dsc_t * dsc = mem_malloc(sizeof(lv_font_fmt_freetype_dsc_t));
	LV_ASSERT_MEM(dsc);
	if(dsc == NULL) return -1;

	dsc->font = freetype_font_open(font_path, font_size);
	dsc->cache = freetype_font_get_cache(dsc->font);
	printf("dsc->font 0x%x\n", dsc->font);
	dsc->font_size = font_size;


	font->get_glyph_dsc = freetype_font_get_glyph_dsc_cb; 	   /*Set a callback to get info about gylphs*/
	font->get_glyph_bitmap = freetype_font_get_glyph_bitmap_cb;  /*Set a callback to get bitmap of a glyp*/

	font->user_data = dsc;
	font->line_height = (dsc->font->face->size->metrics.height >> 6);
	font->base_line = -(dsc->font->face->size->metrics.descender >> 6);  /*Base line measured from the top of line_height*/
	font->subpx = LV_FONT_SUBPX_NONE;

	printf("line height %d, base line %d\n", font->line_height, font->base_line);
	return FT_Err_Ok;
		
ERR_EXIT:
	if(dsc->font != NULL)
	{
		freetype_font_close(dsc->font);
	}

	mem_free(dsc);
	return -1;
}

void lvgl_freetype_font_close(lv_font_t* font)
{
    lv_font_fmt_freetype_dsc_t * dsc;

	if(font == NULL)
	{
		LV_LOG_ERROR("null font pointer\n");
		return;
	}	

	dsc = (lv_font_fmt_freetype_dsc_t*)font->user_data;
	freetype_font_close(dsc->font);
	mem_free(dsc);
}


