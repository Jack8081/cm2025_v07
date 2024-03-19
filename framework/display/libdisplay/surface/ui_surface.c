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
#include <display/ui_memsetcpy.h>
#include <ui_surface.h>
#ifdef CONFIG_DMA2D_HAL
#  include <dma2d_hal.h>
#endif
#ifdef CONFIG_TRACING
#  include <view_manager.h>
#endif

#define OPT_SWAP_COPY_W_2PX 1

#ifdef CONFIG_DMA2D_HAL
typedef struct surface_dma2d_context {
	hal_dma2d_handle_t hdma2d;
	uint8_t inited : 1;
	/* HARDWARE BUG:
	 * some IC dma2d's PSRAM bandwidth priority is higher than CPU's,
	 * which will affect ISR delay
	 */
	uint8_t fix_isr_latency : 1;
} surface_dma2d_context_t;
#endif

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 1
static void _surface_swapbuf(surface_t *surface);
static void _surface_swapbuf_wait_finish(surface_t *surface);

#ifdef CONFIG_DMA2D_HAL
static void _surface_dma2d_init(void);

static surface_dma2d_context_t dma2d_ctx;
#endif /* CONFIG_DMA2D_HAL */
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 1 */

static void _surface_invoke_draw_ready(surface_t *surface);
static void _surface_invoke_post_start(surface_t *surface, const ui_region_t *area, uint8_t flags);

surface_t *surface_create(uint16_t w, uint16_t h,
		uint32_t pixel_format, uint8_t buf_count, uint8_t swap_mode, uint8_t flags)
{
	surface_t *surface = NULL;

#if CONFIG_SURFACE_MAX_BUFFER_COUNT == 0
	if (swap_mode == SURFACE_SWAP_SINGLE_BUF)
		return -EINVAL;

	swap_mode = SURFACE_SWAP_ZERO_BUF;
#endif

	surface = mem_malloc(sizeof(*surface));
	if (surface == NULL) {
		return NULL;
	}

	memset(surface, 0, sizeof(*surface));
	surface->width = w;
	surface->height = h;
	surface->pixel_format = pixel_format;
	surface->swap_mode = swap_mode;
	surface->post_sync = (flags & SURFACE_POST_IN_SYNC_MODE) ? 1 : 0;

	if (surface->swap_mode == SURFACE_SWAP_ZERO_BUF) {
		surface->buffers[0] = mem_malloc(sizeof(graphic_buffer_t));
		if (surface->buffers[0] == NULL) {
			mem_free(surface);
			return NULL;
		}
	} else if (surface_set_min_buffer_count(surface, buf_count)) {
		surface_destroy(surface);
		return NULL;
	}

	atomic_set(&surface->refcount, 1);
	os_sem_init(&surface->complete_sem, 0, 1);
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 1 && defined(CONFIG_DMA2D_HAL)
	_surface_dma2d_init();
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 1 */

	return surface;
}

void surface_destroy(surface_t *surface)
{
	if (surface->buf_count > 0) {
		surface_set_max_buffer_count(surface, 0);
		assert(surface->buffers[0] == NULL);
	} else { /* SURFACE_SWAP_ZERO_BUF */
		while (atomic_get(&surface->post_pending_cnt) || surface->in_drawing) {
			SYS_LOG_DBG("%p wait post", surface);
			os_sem_take(&surface->complete_sem, OS_FOREVER);
		}

		if (surface->buffers[0]) {
			mem_free(surface->buffers[0]);
		}
	}

	if (atomic_dec(&surface->refcount) == 1) {
		mem_free(surface);
	}
}

int surface_set_min_buffer_count(surface_t *surface, uint8_t min_count)
{
	uint8_t buf_count;
	int res = 0;

	if (min_count > CONFIG_SURFACE_MAX_BUFFER_COUNT)
		return -EDOM;

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	if (min_count <= surface->buf_count)
		return 0;
	else if (surface->swap_mode == SURFACE_SWAP_SINGLE_BUF && min_count > 1)
		return -EINVAL;
	else if (surface->swap_mode == SURFACE_SWAP_ZERO_BUF && min_count > 0)
		return -EINVAL;

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 1
	_surface_swapbuf_wait_finish(surface);
#endif

	buf_count = surface->buf_count;

	for (int i = buf_count; i < min_count; i++) {
		surface->buffers[i] = graphic_buffer_create(surface->width,
				surface->height, surface->pixel_format,
				GRAPHIC_BUFFER_SW_MASK | GRAPHIC_BUFFER_HW_MASK);
		if (surface->buffers[i] == NULL) {
			SYS_LOG_ERR("alloc failed");
			res = -ENOMEM;
			break;
		}

		buf_count++;
	}

	if (buf_count != surface->buf_count) {
		/* make sure the front buffer index keep the same */
		os_sched_lock();
		surface->buf_count = buf_count;
		surface->back_idx = buf_count - 1;
		os_sched_unlock();

		/* invalidate the new dirty area */
		ui_region_set(&surface->dirty_area, 0, 0, surface->width - 1, surface->height - 1);

		SYS_LOG_DBG("buf count %d", surface->buf_count);
	}
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */

	return res;
}

int surface_set_max_buffer_count(surface_t *surface, uint8_t max_count)
{
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	uint8_t buf_count = surface->buf_count;

	if (max_count >= buf_count)
		return 0;

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 1
	_surface_swapbuf_wait_finish(surface);
#endif

	if (surface->post_sync) {
		/* synchronize */
		while (atomic_get(&surface->post_pending_cnt) || surface->in_drawing) {
			SYS_LOG_DBG("%p wait post", surface);
			os_sem_take(&surface->complete_sem, OS_FOREVER);
		}
	} else {
		while (atomic_get(&surface->post_pending_cnt) > 1) {
			SYS_LOG_DBG("%p wait post", surface);
			os_sem_take(&surface->complete_sem, OS_FOREVER);
		}

		while (surface->in_drawing) {
			os_sleep(1);
		}
	}

	/* make sure the front buffer index keep the same */
	os_sched_lock();

	/* keep front buffer whose content may be good */
	if (buf_count > 1 && surface->back_idx == 0) {
		graphic_buffer_t *tmp = surface->buffers[1];
		surface->buffers[1] = surface->buffers[0];
		surface->buffers[0] = tmp;
	}

	surface->back_idx = 0;
	surface->front_idx = 0;
	surface->buf_count = max_count;

	os_sched_unlock();

	for (int i = max_count; i < buf_count; i++) {
		graphic_buffer_destroy(surface->buffers[i]);
		surface->buffers[i] = NULL;
	}
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */

	return 0;
}

graphic_buffer_t *surface_get_backbuffer(surface_t *surface)
{
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	return (surface->buf_count > 0) ? surface->buffers[surface->back_idx] : NULL;
#else
	return NULL;
#endif
}

graphic_buffer_t *surface_get_frontbuffer(surface_t *surface)
{
	return surface->buffers[surface->front_idx];
}

int surface_set_swap_mode(surface_t *surface, uint8_t mode)
{
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	if (surface->swap_mode == mode)
		return 0;

	if (surface->swap_mode == SURFACE_SWAP_ZERO_BUF)
		return -EINVAL;

	switch (mode) {
	case SURFACE_SWAP_SINGLE_BUF:
		surface_set_max_buffer_count(surface, 1);
		break;
	case SURFACE_SWAP_ZERO_BUF: {
			graphic_buffer_t *fakebuf = mem_malloc(sizeof(graphic_buffer_t));
			if (fakebuf == NULL)
				return -EINVAL;

			surface_set_max_buffer_count(surface, 0);
			surface->buffers[0] = fakebuf;
		}
		break;
	default:
		break;
	}

	surface->swap_mode = mode;
	SYS_LOG_DBG("swap mode %d", mode);
	return 0;
#else
	return (mode == SURFACE_SWAP_SINGLE_BUF) ? -EINVAL : 0;
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */
}

void surface_begin_frame(surface_t *surface)
{
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 1
	if (surface->buf_count == 2) {
		surface_cover_check_data_t cover_check_data = {
			.area = &surface->dirty_area,
			.covered = ui_region_is_empty(&surface->dirty_area),
		};

		if (!cover_check_data.covered && surface->callback[SURFACE_CB_DRAW]) {
			surface->callback[SURFACE_CB_DRAW](SURFACE_EVT_DRAW_COVER_CHECK,
					&cover_check_data, surface->user_data[SURFACE_CB_DRAW]);
		}

		SYS_LOG_DBG("dirty (%d %d %d %d), covered %d",
			surface->dirty_area.x1, surface->dirty_area.y1,
			surface->dirty_area.x2, surface->dirty_area.y2, cover_check_data.covered);

		if (!cover_check_data.covered) {
			/* wait backbuf available */
			while (atomic_get(&surface->post_pending_cnt) >= surface->buf_count) {
				SYS_LOG_DBG("%p wait post", surface);
				os_sem_take(&surface->complete_sem, OS_FOREVER);
			}

			/* consider swap pending into post pending count */
			surface->swap_pending = 1;
			SYS_LOG_DBG("%p swap pending", surface);
			_surface_swapbuf(surface);
		}
	}
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 1 */

	ui_region_set(&surface->dirty_area, surface->width, surface->height, 0, 0);
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */
}

void surface_end_frame(surface_t *surface)
{
}

int surface_begin_draw(surface_t *surface, uint8_t flags, graphic_buffer_t **drawbuf)
{
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	if (surface->buf_count > 0 && (flags & SURFACE_FIRST_DRAW)) {
		/* wait backbuf available */
		while (atomic_get(&surface->post_pending_cnt) >= surface->buf_count) {
			SYS_LOG_DBG("%p wait post", surface);
			os_sem_take(&surface->complete_sem, OS_FOREVER);
		}

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 1
		_surface_swapbuf_wait_finish(surface);
#endif
	}
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */

	surface->in_drawing = 1;
	surface->draw_flags = flags;
	*drawbuf = surface_get_backbuffer(surface);
	return 0;
}

void surface_end_draw(surface_t *surface, const ui_region_t *area, const void *buf)
{
#ifdef CONFIG_TRACING
	ui_view_context_t *view = surface->user_data[SURFACE_CB_POST];
	os_strace_end_call_u32(SYS_TRACE_ID_VIEW_DRAW, view->entry->id);
#endif /* CONFIG_TRACING */

	assert(surface->buffers[0] != NULL);

	if (surface->buf_count == 0) { /* post based draw part */
		graphic_buffer_init(surface->buffers[0], ui_region_get_width(area),
				ui_region_get_height(area), surface->pixel_format,
				GRAPHIC_BUFFER_HW_COMPOSER, ui_region_get_width(area), (void *)buf);

		_surface_invoke_post_start(surface, area, surface->draw_flags);
		surface->in_drawing = 0;
		return;
	}

	/* post based on frame */
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	ui_region_merge(&surface->dirty_area, &surface->dirty_area, area);

	_surface_invoke_draw_ready(surface);

	if (surface->draw_flags & SURFACE_LAST_DRAW) {
		/* make sure the swap buffer not interrupted by posting */
		os_sched_lock();

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 1
		if (surface->buf_count > 1) {
			surface->front_idx ^= 1;
			surface->back_idx ^= 1;
		}
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 1 */

		_surface_invoke_post_start(surface, &surface->dirty_area, SURFACE_FIRST_DRAW | SURFACE_LAST_DRAW);
		os_sched_unlock();
	}

	surface->in_drawing = 0;
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */
}

void surface_complete_one_post(surface_t *surface)
{
	atomic_dec(&surface->post_pending_cnt);
	SYS_LOG_DBG("%p post cplt %d", surface, atomic_get(&surface->post_pending_cnt));
	os_sem_give(&surface->complete_sem);

	if (surface->buf_count == 0) { /* post based draw part */
		_surface_invoke_draw_ready(surface);
	}

	if (atomic_dec(&surface->refcount) == 1) {
		mem_free(surface);
	}
}

static void _surface_invoke_draw_ready(surface_t *surface)
{
	if (surface->callback[SURFACE_CB_DRAW]) {
		surface->callback[SURFACE_CB_DRAW](SURFACE_EVT_DRAW_READY,
				NULL, surface->user_data[SURFACE_CB_DRAW]);
	}
}

static void _surface_invoke_post_start(surface_t *surface, const ui_region_t *area, uint8_t flags)
{
	surface_post_data_t data = {
		.flags = flags,
		.area = area,
	};

	atomic_inc(&surface->refcount);
	atomic_inc(&surface->post_pending_cnt);
	SYS_LOG_DBG("%p post inprog %d", surface, atomic_get(&surface->post_pending_cnt));

	if (surface->callback[SURFACE_CB_POST]) {
		surface->callback[SURFACE_CB_POST](SURFACE_EVT_POST_START,
				&data, surface->user_data[SURFACE_CB_POST]);
	}
}


#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 1
#ifdef CONFIG_DMA2D_HAL
extern int soc_dvfs_opt(void);

static void _surface_dma2d_init(void)
{
	if (dma2d_ctx.inited)
		return;

	if (hal_dma2d_init(&dma2d_ctx.hdma2d) == 0) {
		/* initialize dma2d constant fields */
		dma2d_ctx.inited = 1;
		dma2d_ctx.fix_isr_latency = soc_dvfs_opt() ? 0 : 1;

		if (dma2d_ctx.fix_isr_latency) {
			dma2d_ctx.hdma2d.output_cfg.mode = HAL_DMA2D_M2M_BLEND;
			dma2d_ctx.hdma2d.output_cfg.rb_swap = HAL_DMA2D_RB_REGULAR;
			dma2d_ctx.hdma2d.layer_cfg[0].rb_swap = HAL_DMA2D_RB_REGULAR;
			dma2d_ctx.hdma2d.layer_cfg[0].alpha_mode = HAL_DMA2D_NO_MODIF_ALPHA;
			dma2d_ctx.hdma2d.layer_cfg[1].rb_swap = HAL_DMA2D_RB_REGULAR;
			dma2d_ctx.hdma2d.layer_cfg[1].alpha_mode = HAL_DMA2D_NO_MODIF_ALPHA;
			dma2d_ctx.hdma2d.layer_cfg[1].input_alpha = 0xFFFFFFFF;
		} else {
			dma2d_ctx.hdma2d.output_cfg.mode = HAL_DMA2D_M2M;
			dma2d_ctx.hdma2d.output_cfg.rb_swap = HAL_DMA2D_RB_REGULAR;
			dma2d_ctx.hdma2d.layer_cfg[1].rb_swap = HAL_DMA2D_RB_REGULAR;
			dma2d_ctx.hdma2d.layer_cfg[1].alpha_mode = HAL_DMA2D_NO_MODIF_ALPHA;
		}
	} else {
		SYS_LOG_ERR("dma2d init failed");
	}
}
#endif /* CONFIG_DMA2D_HAL */

static void _surface_swapbuf_copy(graphic_buffer_t *backbuf, graphic_buffer_t *frontbuf, const ui_region_t *region)
{
	uint8_t *backptr = (uint8_t *)graphic_buffer_get_bufptr(backbuf, region->x1, region->y1);
	uint8_t *frontptr = (uint8_t *)graphic_buffer_get_bufptr(frontbuf, region->x1, region->y1);
	int res = -1;

	assert(backptr != NULL && frontptr != NULL);

#if OPT_SWAP_COPY_W_2PX
	if (ui_region_get_width(region) == 2) {
		int j;

		backptr = mem_addr_to_uncache(backptr);
		frontptr = mem_addr_to_uncache(frontptr);

		if (graphic_buffer_get_pixel_format(backbuf) == PIXEL_FORMAT_RGB_565) {
			uint16_t *backptr16 = (uint16_t *)backptr;
			uint16_t *frontptr16 = (uint16_t *)frontptr;
			for (j = ui_region_get_height(region); j > 0; j--) {
				backptr16[0] = frontptr16[0];
				backptr16[1] = frontptr16[1];
				backptr16 += graphic_buffer_get_stride(backbuf);
				frontptr16 += graphic_buffer_get_stride(backbuf);
			}
		} else {
			uint32_t *backptr32 = (uint32_t *)backptr;
			uint32_t *frontptr32 = (uint32_t *)frontptr;
			for (j = ui_region_get_height(region); j > 0; j--) {
				backptr32[0] = frontptr32[0];
				backptr32[1] = frontptr32[1];
				backptr32 += graphic_buffer_get_stride(backbuf);
				frontptr32 += graphic_buffer_get_stride(backbuf);
			}
		}

		mem_writebuf_clean_all();
		return;
	}
#endif /* OPT_SWAP_COPY_W_2PX */

#ifdef CONFIG_DMA2D_HAL
	if (dma2d_ctx.inited) {
		dma2d_ctx.hdma2d.output_cfg.output_offset =
			graphic_buffer_get_stride(backbuf) - ui_region_get_width(region);
		if (graphic_buffer_get_pixel_format(backbuf) == PIXEL_FORMAT_RGB_565) {
			dma2d_ctx.hdma2d.output_cfg.color_mode = HAL_DMA2D_RGB565;
			dma2d_ctx.hdma2d.layer_cfg[1].color_mode = HAL_DMA2D_RGB565;
			if (dma2d_ctx.fix_isr_latency) {
				dma2d_ctx.hdma2d.layer_cfg[0].color_mode = HAL_DMA2D_RGB565;
			}
		} else {
			dma2d_ctx.hdma2d.output_cfg.color_mode = HAL_DMA2D_ARGB8888;
			dma2d_ctx.hdma2d.layer_cfg[1].color_mode = HAL_DMA2D_ARGB8888;
			if (dma2d_ctx.fix_isr_latency) {
				dma2d_ctx.hdma2d.layer_cfg[0].color_mode = HAL_DMA2D_ARGB8888;
			}
		}
		dma2d_ctx.hdma2d.layer_cfg[1].input_offset = dma2d_ctx.hdma2d.output_cfg.output_offset;

		if (dma2d_ctx.fix_isr_latency) {
			/* FIXME: the alpha of ARGB888 is always 255 for surface buffer, so the blending is equal to copy ? */
			dma2d_ctx.hdma2d.layer_cfg[0].input_offset = dma2d_ctx.hdma2d.output_cfg.output_offset;

			if (!hal_dma2d_config_output(&dma2d_ctx.hdma2d) &&
				!hal_dma2d_config_layer(&dma2d_ctx.hdma2d, HAL_DMA2D_BACKGROUND_LAYER) &&
				!hal_dma2d_config_layer(&dma2d_ctx.hdma2d, HAL_DMA2D_FOREGROUND_LAYER)) {
				res = hal_dma2d_blending_start(&dma2d_ctx.hdma2d, (uint32_t)frontptr,
						(uint32_t)frontptr, (uint32_t)backptr,
						ui_region_get_width(region), ui_region_get_height(region));
			}
		} else {
			if (!hal_dma2d_config_output(&dma2d_ctx.hdma2d) &&
				!hal_dma2d_config_layer(&dma2d_ctx.hdma2d, HAL_DMA2D_FOREGROUND_LAYER)) {
				res = hal_dma2d_start(&dma2d_ctx.hdma2d, (uint32_t)frontptr, (uint32_t)backptr,
						ui_region_get_width(region), ui_region_get_height(region));
			}
		}
	}
#endif /* CONFIG_DMA2D_HAL */

	if (res < 0) {
		uint8_t bpp = graphic_buffer_get_bits_per_pixel(backbuf);
		uint16_t copy_bytes = ui_region_get_width(region) * bpp / 8;
		uint16_t bytes_per_line = graphic_buffer_get_stride(backbuf) * bpp / 8;

		backptr = mem_addr_to_uncache(backptr);

		if (copy_bytes == bytes_per_line) {
			ui_memcpy(backptr, mem_addr_to_uncache(frontptr),
					copy_bytes * ui_region_get_height(region));
			ui_memsetcpy_wait_finish(5000);
		} else {
			for (int j = ui_region_get_height(region); j > 0; j--) {
				mem_dcache_invalidate(frontptr, copy_bytes);
				memcpy(backptr, frontptr, copy_bytes);

				backptr += bytes_per_line;
				frontptr += bytes_per_line;
			}
		}

		mem_writebuf_clean_all();
	}
}

static void _surface_swapbuf(surface_t *surface)
{
	graphic_buffer_t *backbuf = surface->buffers[surface->back_idx];
	graphic_buffer_t *frontbuf = surface->buffers[surface->back_idx ^ 1];

#ifdef CONFIG_TRACING
	ui_view_context_t *view = surface->user_data[SURFACE_CB_POST];
	os_strace_u32(SYS_TRACE_ID_VIEW_SWAPBUF, view->entry->id);
#endif

	_surface_swapbuf_copy(backbuf, frontbuf, &surface->dirty_area);
	SYS_LOG_DBG("%p swap %p->%p", surface, frontbuf, backbuf);

#ifdef CONFIG_TRACING
	os_strace_end_call_u32(SYS_TRACE_ID_VIEW_SWAPBUF, view->entry->id);
#endif
}

static void _surface_swapbuf_wait_finish(surface_t *surface)
{
	if (surface->swap_pending) {
#ifdef CONFIG_DMA2D_HAL
		hal_dma2d_poll_transfer(&dma2d_ctx.hdma2d, -1);
#endif
		surface->swap_pending = 0;
	}
}

#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 1 */
