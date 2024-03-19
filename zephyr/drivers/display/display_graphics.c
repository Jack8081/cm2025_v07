/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <spicache.h>
#include <drivers/display/display_graphics.h>

bool display_format_is_opaque(uint32_t pixel_format)
{
	switch (pixel_format) {
	case PIXEL_FORMAT_ARGB_8888:
	case PIXEL_FORMAT_ARGB_8565:
	case PIXEL_FORMAT_ARGB_6666:
	case PIXEL_FORMAT_ABGR_6666:
	case PIXEL_FORMAT_A8:
	case PIXEL_FORMAT_A4:
	case PIXEL_FORMAT_A1:
	case PIXEL_FORMAT_A4_LE:
	case PIXEL_FORMAT_A1_LE:
		return false;
	default:
		return true;
	}
}

uint8_t display_format_get_bits_per_pixel(uint32_t pixel_format)
{
	switch (pixel_format) {
	case PIXEL_FORMAT_ARGB_8888:
		return 32;
	case PIXEL_FORMAT_RGB_565:
	case PIXEL_FORMAT_BGR_565:
	case PIXEL_FORMAT_RGB_565_LE:
		return 16;
	case PIXEL_FORMAT_RGB_888:
	case PIXEL_FORMAT_ARGB_8565:
	case PIXEL_FORMAT_ARGB_6666:
	case PIXEL_FORMAT_ABGR_6666:
		return 24;
	case PIXEL_FORMAT_A8:
		return 8;
	case PIXEL_FORMAT_A4:
	case PIXEL_FORMAT_A4_LE:
		return 4;
	case PIXEL_FORMAT_A1:
	case PIXEL_FORMAT_A1_LE:
	case PIXEL_FORMAT_MONO01:
	case PIXEL_FORMAT_MONO10:
		return 1;
	default:
		return 0;
	}
}

uint16_t display_buffer_get_bytes_per_line(const display_buffer_t *buffer)
{
	return buffer->desc.pitch * display_format_get_bits_per_pixel(buffer->desc.pixel_format) / 8;
}

int display_buffer_fill_color(const display_buffer_t *buffer, display_color_t color)
{
	uint8_t bits_per_pixel = display_format_get_bits_per_pixel(buffer->desc.pixel_format);
	uint16_t bytes_per_line = buffer->desc.pitch * bits_per_pixel / 8;
	uint16_t bytes_to_fill = buffer->desc.width * bits_per_pixel / 8;
	uint8_t *buf8 = (uint8_t *)(buffer->addr);
	int i;

	if (buffer->desc.pixel_format == PIXEL_FORMAT_RGB_565) {
		uint16_t *tmp_buf16 = (uint16_t *)buf8;
		uint16_t c16 = ((color.full >> 8) & 0xf800) |
				((color.full >> 5) & 0x07e0) | ((color.full >> 3) & 0x001f);

		for (i = buffer->desc.width; i > 0; i--) {
			*tmp_buf16++ = c16;
		}
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_BGR_565) {
		uint16_t *tmp_buf16 = (uint16_t *)buf8;
		uint16_t c16 = ((color.full << 8) & 0xf800) |
				((color.full >> 5) & 0x07e0) | ((color.full >> 19) & 0x001f);

		for (i = buffer->desc.width; i > 0; i--) {
			*tmp_buf16++ = c16;
		}
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_ARGB_8888) {
		uint32_t *tmp_buf32 = (uint32_t *)buf8;

		for (i = buffer->desc.width; i > 0; i--) {
			*tmp_buf32++ = color.full;
		}
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_RGB_888) {
		uint8_t *tmp_buf8 = (uint8_t *)buf8;

		for (i = buffer->desc.width; i > 0; i--) {
			*tmp_buf8++ = color.b;
			*tmp_buf8++ = color.g;
			*tmp_buf8++ = color.r;
		}
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_ARGB_6666) {
		uint8_t *tmp_buf8 = buf8 + 3;

		buf8[0] = ((color.g & 0x0c) << 4) | (color.b >> 2);
		buf8[1] = ((color.r & 0x3c) << 2) | (color.g >> 4);
		buf8[2] = ((color.a & 0xfc) << 0) | (color.r >> 6);

		for (i = buffer->desc.width - 1; i > 0; i--) {
			*tmp_buf8++ = buf8[0];
			*tmp_buf8++ = buf8[1];
			*tmp_buf8++ = buf8[2];
		}
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_ABGR_6666) {
		uint8_t *tmp_buf8 = buf8 + 3;

		buf8[0] = ((color.g & 0x0c) << 4) | (color.r >> 2);
		buf8[1] = ((color.b & 0x3c) << 2) | (color.g >> 4);
		buf8[2] = ((color.a & 0xfc) << 0) | (color.b >> 6);

		for (i = buffer->desc.width - 1; i > 0; i--) {
			*tmp_buf8++ = buf8[0];
			*tmp_buf8++ = buf8[1];
			*tmp_buf8++ = buf8[2];
		}
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_ARGB_8565) {
		uint8_t *tmp_buf8 = buf8 + 3;

		buf8[0] = ((color.g & 0x1c) << 3) | (color.b >> 3);
		buf8[1] = ((color.r & 0xf8) << 0) | (color.g >> 5);
		buf8[2] = color.a;

		for (i = buffer->desc.width - 1; i > 0; i--) {
			*tmp_buf8++ = buf8[0];
			*tmp_buf8++ = buf8[1];
			*tmp_buf8++ = buf8[2];
		}
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_A8) {
		uint8_t *tmp_buf8 = buf8;

		for (i = buffer->desc.width; i > 0; i--) {
			*tmp_buf8++ = color.a;
		}
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_A4 ||
		       buffer->desc.pixel_format == PIXEL_FORMAT_A4_LE) {
		uint8_t *tmp_buf8 = buf8 + 1;

		buf8[0] = (color.a & 0xf0) | (color.a >> 4);

		for (i = buffer->desc.width - 2; i > 0; i -= 2) {
			*tmp_buf8++ = buf8[0];
		}
	} else if (buffer->desc.pixel_format == PIXEL_FORMAT_A1 ||
		       buffer->desc.pixel_format == PIXEL_FORMAT_A1_LE) {
		uint8_t *tmp_buf8 = buf8 + 1;

		buf8[0] = (color.a & 0x80) ? 0xff : 0x00;

		for (i = buffer->desc.width - 8; i > 0; i -= 8) {
			*tmp_buf8++ = buf8[0];
		}
	} else {
		return -ENOTSUP;
	}

	/* point to the 2nd row */
	buf8 = (uint8_t *)(buffer->addr) + bytes_per_line;

	for (i = buffer->desc.height - 1; i > 0; i--) {
		memcpy(buf8, (uint8_t *)buffer->addr, bytes_to_fill);
		buf8 += bytes_per_line;
	}

	display_buffer_cpu_write_flush(buffer);
	return 0;
}

int display_buffer_fill_image(const display_buffer_t *buffer, const void *data)
{
	uint8_t bits_per_pixel = display_format_get_bits_per_pixel(buffer->desc.pixel_format);
	uint16_t bytes_per_line = buffer->desc.pitch * bits_per_pixel / 8;
	uint16_t bytes_to_copy = buffer->desc.width * bits_per_pixel / 8;
	uint8_t *buf8 = (uint8_t *)buffer->addr;
	const uint8_t *data8 = data;

	for (int i = buffer->desc.height; i > 0; i--) {
		memcpy(buf8, data8, bytes_to_copy);
		buf8 += bytes_per_line;
		data8 += bytes_to_copy;
	}

	display_buffer_cpu_write_flush(buffer);
	return 0;
}

void display_buffer_cpu_write_flush(const display_buffer_t *buffer)
{
	uint16_t bytes_per_line = display_buffer_get_bytes_per_line(buffer);
	uint8_t cache_ops = 0;

	if (buf_is_psram(buffer->addr)) {
		cache_ops = SPI_CACHE_FLUSH;
	} else if (buf_is_psram_un(buffer->addr)) {
		cache_ops = SPI_WRITEBUF_FLUSH;
	}

	if (cache_ops > 0) {
		spi1_cache_ops(cache_ops, (void *)buffer->addr,
				bytes_per_line * buffer->desc.height);
		spi1_cache_ops_wait_finshed();
	}
}

void display_buffer_dev_write_flush(const display_buffer_t *buffer)
{
	uint16_t bytes_per_line = display_buffer_get_bytes_per_line(buffer);

	if (buf_is_psram(buffer->addr)) {
		spi1_cache_ops(SPI_CACHE_INVALIDATE, (void *)buffer->addr,
				bytes_per_line * buffer->desc.height);
		spi1_cache_ops_wait_finshed();
	}
}

void display_rect_intersect(display_rect_t *dest, const display_rect_t *src)
{
	int16_t x1 = MAX(dest->x, src->x);
	int16_t y1 = MAX(dest->y, src->y);
	int16_t x2 = MIN(dest->x + dest->w, src->x + src->w);
	int16_t y2 = MIN(dest->y + dest->h, src->y + src->h);

	dest->x = x1;
	dest->y = y1;
	dest->w = x2 - x1;
	dest->h = y2 - y1;
}

void display_rect_merge(display_rect_t *dest, const display_rect_t *src)
{
	int16_t x1 = MIN(dest->x, src->x);
	int16_t y1 = MIN(dest->y, src->y);
	int16_t x2 = MAX(dest->x + dest->w, src->x + src->w);
	int16_t y2 = MAX(dest->y + dest->h, src->y + src->h);

	dest->x = x1;
	dest->y = y1;
	dest->w = x2 - x1;
	dest->h = y2 - y1;
}
