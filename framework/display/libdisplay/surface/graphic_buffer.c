/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <assert.h>
#include <string.h>
#include <mem_manager.h>
#include <memory/mem_cache.h>
#include <ui_mem.h>
#include <graphic_buffer.h>

#define usage_sw(usage)       (usage & GRAPHIC_BUFFER_SW_MASK)
#define usage_sw_read(usage)  (usage & GRAPHIC_BUFFER_SW_READ)
#define usage_sw_write(usage) (usage & GRAPHIC_BUFFER_SW_WRITE)

#define usage_hw(usage)       (usage & GRAPHIC_BUFFER_HW_MASK)
#define usage_hw_read(usage)  (usage & (GRAPHIC_BUFFER_HW_TEXTURE | GRAPHIC_BUFFER_HW_COMPOSER))
#define usage_hw_write(usage) (usage & GRAPHIC_BUFFER_HW_RENDER)

#define usage_read(usage)     (usage & GRAPHIC_BUFFER_READ_MASK)
#define usage_write(usage)    (usage & GRAPHIC_BUFFER_WRITE_MASK)

int graphic_buffer_init(graphic_buffer_t *buffer, uint16_t w, uint16_t h,
		uint32_t pixel_format, uint32_t usage, uint16_t stride, void *data)
{
	memset(buffer, 0, sizeof(*buffer));
	buffer->width = w;
	buffer->height = h;
	buffer->stride = stride;
	buffer->bits_per_pixel = display_format_get_bits_per_pixel(pixel_format);
	buffer->pixel_format = pixel_format;
	buffer->data = data;
	buffer->usage = usage;

	atomic_set(&buffer->refcount, 1);
	return 0;
}

graphic_buffer_t *graphic_buffer_create_with_data(uint16_t w, uint16_t h,
		uint32_t pixel_format, uint32_t usage, uint16_t stride, void *data)
{
	graphic_buffer_t *buffer = mem_malloc(sizeof(*buffer));

	if (buffer == NULL) {
		return NULL;
	}

	if (graphic_buffer_init(buffer, w, h, pixel_format, usage, stride, data)) {
		mem_free(buffer);
		return NULL;
	}

	buffer->owner = GRAPHIC_BUFFER_OWN_HANDLE;

	return buffer;
}

graphic_buffer_t *graphic_buffer_create(uint16_t w, uint16_t h,
		uint32_t pixel_format, uint32_t usage)
{
#ifdef CONFIG_UI_MEMORY_MANAGER
	graphic_buffer_t *buffer = NULL;
	uint32_t stride, data_size;
	uint8_t bits_per_pixel;
	uint8_t tmp_bpp, multiple = 32;
	void *data;

	bits_per_pixel = display_format_get_bits_per_pixel(pixel_format);
	if (bits_per_pixel == 0 || bits_per_pixel > 32) {
		goto out_exit;
	}

	/* require bytes per row is 4 bytes align */
	tmp_bpp = bits_per_pixel;
	while ((tmp_bpp & 0x1) == 0) {
		tmp_bpp >>= 1;
		multiple >>= 1;
	}

	stride = UI_ROUND_UP(w, multiple);
	data_size = stride * h * bits_per_pixel / 8;

	data = ui_memory_alloc(data_size);
	if (data == NULL) {
		SYS_LOG_ERR("ui mem alloc failed");
		ui_memory_dump_info(-1);
		goto out_exit;
	}

	buffer = graphic_buffer_create_with_data(w, h, pixel_format, usage, stride, data);
	if (buffer == NULL) {
		ui_memory_free(data);
		goto out_exit;
	}

	buffer->owner |= GRAPHIC_BUFFER_OWN_DATA;

out_exit:
	return buffer;
#else
	return NULL;
#endif /* CONFIG_UI_MEMORY_MANAGER */
}

int graphic_buffer_ref(graphic_buffer_t *buffer)
{
	assert(buffer != NULL);
	assert(atomic_get(&buffer->refcount) >= 0);

	atomic_inc(&buffer->refcount);
	return 0;
}

int graphic_buffer_unref(graphic_buffer_t *buffer)
{
	assert(buffer != NULL);
	assert(atomic_get(&buffer->refcount) > 0);

	if (atomic_dec(&buffer->refcount) != 1) {
		return 0;
	}

#ifdef CONFIG_UI_MEMORY_MANAGER
	if (buffer->owner & GRAPHIC_BUFFER_OWN_DATA) {
		ui_memory_free(buffer->data);
	}
#endif

	if (buffer->owner & GRAPHIC_BUFFER_OWN_HANDLE) {
		mem_free(buffer);
	}

	return 0;
}

const void *graphic_buffer_get_bufptr(graphic_buffer_t *buffer, int16_t x, int16_t y)
{
	assert(buffer);

	return buffer->data + ((int32_t)y * buffer->stride + x) * buffer->bits_per_pixel / 8;
}

int graphic_buffer_lock(graphic_buffer_t *buffer, uint32_t usage,
		const ui_region_t *rect, void **vaddr)
{
	assert(buffer);
	assert((usage & buffer->usage) == usage);

	if (usage_sw(usage) && !vaddr) {
		SYS_LOG_ERR("vaddr must provided");
		return -EINVAL;
	}

	if (usage_write(usage) && usage_write(buffer->lock_usage)) {
		SYS_LOG_ERR("multi-write disallowed");
		return -EINVAL;
	}

	if (usage_write(usage)) {
		if (rect) {
			memcpy(&buffer->lock_wrect, rect, sizeof(*rect));
		} else {
			buffer->lock_wrect.x1 = 0;
			buffer->lock_wrect.y1 = 0;
			buffer->lock_wrect.x2 = graphic_buffer_get_width(buffer) - 1;
			buffer->lock_wrect.y2 = graphic_buffer_get_height(buffer) - 1;
		}
	}

	if (vaddr) {
		*vaddr = buffer->data;
		if (rect) {
			*vaddr = (void *)graphic_buffer_get_bufptr(buffer, rect->x1, rect->y1);
		}
	}

	buffer->lock_usage |= usage;

	graphic_buffer_ref(buffer);

	return 0;
}

int graphic_buffer_unlock(graphic_buffer_t *buffer)
{
	void (*cache_ops)(const void *, uint32_t) = NULL;

	assert(buffer && buffer->lock_usage > 0);

	if (mem_is_cacheable(buffer->data) == false) {
		goto out_exit;
	}

	if (usage_sw_write(buffer->lock_usage) && usage_hw(buffer->usage)) {
		cache_ops = mem_dcache_clean;
	} else if (usage_hw_write(buffer->lock_usage) && usage_sw(buffer->usage)) {
		cache_ops = mem_dcache_invalidate;
	}

	if (cache_ops) {
		uint8_t *lock_vaddr = (uint8_t *)graphic_buffer_get_bufptr(buffer,
				buffer->lock_wrect.x1, buffer->lock_wrect.y1);
		uint16_t lock_w = ui_region_get_width(&buffer->lock_wrect);
		uint16_t lock_h = ui_region_get_height(&buffer->lock_wrect);
		uint16_t bytes_per_line = buffer->stride * buffer->bits_per_pixel / 8;
		uint16_t line_len = (lock_w * buffer->bits_per_pixel + 7) / 8;

		if (lock_w == buffer->width) {
			cache_ops(lock_vaddr, lock_h * bytes_per_line);
		} else {
			for (int i = lock_h; i > 0; i--) {
				cache_ops(lock_vaddr, line_len);
				lock_vaddr += bytes_per_line;
			}
		}
	}

out_exit:
	buffer->lock_usage &= ~GRAPHIC_BUFFER_WRITE_MASK;
	graphic_buffer_unref(buffer);
	return 0;
}

void graphic_buffer_destroy(graphic_buffer_t *buffer)
{
	graphic_buffer_unref(buffer);
}
