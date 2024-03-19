#include <os_common_api.h>
#include <mem_manager.h>
#include <string.h>
#include "bitmap_font_api.h"

#define CMAP_CACHE_SIZE							1024*8

#ifdef CONFIG_BITMAP_FONT_USE_HIGH_FREQ_CACHE
#ifdef CONFIG_BITMAP_FONT_HIGH_FREQ_CACHE_SIZE
#define BITMAP_FONT_HIGH_FREQ_CACHE_SIZE		CONFIG_BITMAP_FONT_HIGH_FREQ_CACHE_SIZE
#else
#define BITMAP_FONT_HIGH_FREQ_CACHE_SIZE		1500*1024
#endif //CONFIG_BITMAP_FONT_HIGH_FREQ_CACHE_SIZE
#else
#define BITMAP_FONT_HIGH_FREQ_CACHE_SIZE		0
#endif //CONFIG_BITMAP_FONT_USE_HIGH_FREQ_CACHE

#ifdef CONFIG_BITMAP_PER_FONT_CACHE_SIZE
#define BITMAP_FONT_CACHE_SIZE				CONFIG_BITMAP_PER_FONT_CACHE_SIZE
#else
#define BITMAP_FONT_CACHE_SIZE				1024*64
#endif //CONFIG_BITMAP_PER_FONT_CACHE_SIZE

#ifdef CONFIG_BITMAP_FONT_MAX_OPENED_FONT
#define MAX_OPEND_FONT						CONFIG_BITMAP_FONT_MAX_OPENED_FONT
#else
#define MAX_OPEND_FONT						2
#endif //CONFIG_BITMAP_FONT_MAX_OPENED_FONT

#ifdef CONFIG_BITMAP_FONT_SUPPORT_EMOJI
#define MAX_EMOJI_FONT						1
#define MAX_EMOJI_NUM						100
#else
#define MAX_EMOJI_FONT						0
#define MAX_EMOJI_NUM						0
#endif  //CONFIG_BITMAP_FONT_SUPPORT_EMOJI

#define BITMAP_FONT_PSRAM_SIZE				(MAX_OPEND_FONT+MAX_EMOJI_FONT)*(BITMAP_FONT_CACHE_SIZE+CMAP_CACHE_SIZE)+MAX_EMOJI_NUM*sizeof(emoji_font_entry_t)+BITMAP_FONT_HIGH_FREQ_CACHE_SIZE


static char __aligned(4) bmp_font_cache_buffer[BITMAP_FONT_PSRAM_SIZE] __in_section_unique(BMPFONT_PSRAM_REGION);

uint8_t* bitmap_font_get_cache_buffer(void)
{
	return bmp_font_cache_buffer;
}

uint32_t bitmap_font_get_max_fonts_num(void)
{
	return (MAX_OPEND_FONT+MAX_EMOJI_FONT);
}

uint32_t bitmap_font_get_font_cache_size(void)
{
	return BITMAP_FONT_CACHE_SIZE;
}

uint32_t bitmap_font_get_max_emoji_num(void)
{
	return MAX_EMOJI_NUM;
}

uint32_t bitmap_font_get_cmap_cache_size(void)
{
	return CMAP_CACHE_SIZE;
}

void bitmap_font_cache_info_dump(void)
{
	SYS_LOG_DBG("checkmem bmp_font_cache_buffer %p, size %d, BITMAP_FONT_CACHE_SIZE %d, CMAP_CACHE_SIZE %d, CONFIG_BITMAP_PER_FONT_CACHE_SIZE %d\n", bmp_font_cache_buffer, BITMAP_FONT_PSRAM_SIZE, BITMAP_FONT_CACHE_SIZE, CMAP_CACHE_SIZE, CONFIG_BITMAP_PER_FONT_CACHE_SIZE);

}

bool bitmap_font_get_high_freq_enabled(void)
{
#ifdef CONFIG_BITMAP_FONT_USE_HIGH_FREQ_CACHE
	return true;
#else
	return false;
#endif
}
