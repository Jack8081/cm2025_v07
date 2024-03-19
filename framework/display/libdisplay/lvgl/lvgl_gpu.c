/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <dma2d_hal.h>
#include <memory/mem_cache.h>
#include "lvgl_gpu.h"

/* optimize some copy corner cases that are unsupported by hardware */
#define OPT_COPY_CORNER_CASE 1

#ifdef CONFIG_LV_COLOR_DEPTH_32
#  define LVGL_DMA2D_COLOR_MODE HAL_DMA2D_ARGB8888
#  define LVGL_DMA2D_RB_SWAP    HAL_DMA2D_RB_REGULAR
#else
#  define LVGL_DMA2D_COLOR_MODE HAL_DMA2D_RGB565
#  define LVGL_DMA2D_RB_SWAP    HAL_DMA2D_RB_REGULAR
#endif

typedef struct lvgl_dma2d_context {
	hal_dma2d_handle_t hdma2d;

	uint8_t inited : 1;

	uint16_t alloc_seq;
	uint16_t event_seq; /* the last "alloc_seq" before the post */

	lvgl_gpu_event_cb_t event_cb;
	void *event_data;
	os_sem event_sem;
} lvgl_dma2d_context_t;

typedef struct lvgl_pending_cache {
	const uint8_t *buf_start;
	const uint8_t *buf_end;
} lvgl_pending_cache_t;

static lvgl_pending_cache_t pending_cache;

static lvgl_dma2d_context_t dma2d_ctx;

static void merge_pending_cache(
		const lv_color_t *buf, lv_coord_t buf_w, lv_coord_t w, lv_coord_t h)
{
	if (mem_is_cacheable(buf)) {
		const uint8_t *buf_start = (uint8_t *)buf;
		const uint8_t *buf_end = buf_start +
				sizeof(lv_color_t) * ((uint32_t)buf_w * (h - 1) + w);

		if (pending_cache.buf_start == NULL) {
			pending_cache.buf_start = buf_start;
			pending_cache.buf_end = buf_end;
		} else {
			if (pending_cache.buf_start > buf_start)
				pending_cache.buf_start = buf_start;
			if (pending_cache.buf_end < buf_end)
				pending_cache.buf_end = buf_end;
		}
	}
}

static void invalidate_pending_cache(void)
{
	if (pending_cache.buf_start) {
		mem_dcache_invalidate((void *)pending_cache.buf_start,
				pending_cache.buf_end - pending_cache.buf_start);
		mem_dcache_sync();
		pending_cache.buf_start = NULL;
	}
}

static void dma2d_xfer_cb(hal_dma2d_handle_t *hdma2d, uint16_t cmd_seq)
{
	if (dma2d_ctx.event_cb && cmd_seq == dma2d_ctx.event_seq) {
		dma2d_ctx.event_cb(0, dma2d_ctx.event_data);
		dma2d_ctx.event_cb = NULL;
		os_sem_give(&dma2d_ctx.event_sem);
	}
}

static int lvgl_gpu_init(void)
{
	int res = 0;

	if (dma2d_ctx.inited)
		return 0;

	res = hal_dma2d_init(&dma2d_ctx.hdma2d);
	if (res == 0) {
		/* register dma2d callback */
		hal_dma2d_register_callback(&dma2d_ctx.hdma2d,
				HAL_DMA2D_TRANSFERCOMPLETE_CB_ID, dma2d_xfer_cb);
		hal_dma2d_register_callback(&dma2d_ctx.hdma2d,
				HAL_DMA2D_TRANSFERERROR_CB_ID, dma2d_xfer_cb);

		/* initialize dma2d constant fields */
		dma2d_ctx.hdma2d.output_cfg.rb_swap = LVGL_DMA2D_RB_SWAP;
		dma2d_ctx.hdma2d.output_cfg.color_mode = LVGL_DMA2D_COLOR_MODE;
		dma2d_ctx.hdma2d.layer_cfg[0].alpha_mode = HAL_DMA2D_NO_MODIF_ALPHA;
		dma2d_ctx.hdma2d.layer_cfg[0].rb_swap = LVGL_DMA2D_RB_SWAP;
		dma2d_ctx.hdma2d.layer_cfg[0].color_mode = LVGL_DMA2D_COLOR_MODE;

		dma2d_ctx.inited = 1;
		dma2d_ctx.event_cb = NULL;
		dma2d_ctx.event_data = NULL;
		os_sem_init(&dma2d_ctx.event_sem, 0, 1);
	}

	return res;
}

static void lvgl_gpu_clean_dcache_cb(lv_disp_drv_t * disp_drv)
{
	if (disp_drv->using_gpu == 0) {
		if (mem_is_cacheable(disp_drv->draw_buf->buf_act)) {
			/* clean the whole vdb cache to main memory */
			mem_dcache_clean(disp_drv->draw_buf->buf_act,
					disp_drv->draw_buf->size * sizeof(lv_color_t));
			mem_dcache_sync();
		}

		/* indicate the cache become clean */
		disp_drv->using_gpu = 1;
	} else {
		/* invalidate the cache */
		//invalidate_pending_cache();
	}
}

static void lvgl_gpu_wait_cb(lv_disp_drv_t * disp_drv)
{
	if (disp_drv->using_gpu == 1) {
		/* invalidate the cache */
		invalidate_pending_cache();
		/* wait gpu complete */
		hal_dma2d_poll_transfer(&dma2d_ctx.hdma2d, -1);
		/* next is cpu draw ? */
		disp_drv->using_gpu = 0;
	}
}

static int lvgl_gpu_fill_cb(lv_disp_drv_t * disp_drv,
		lv_color_t * dest, lv_coord_t dest_w, lv_color_t color, lv_opa_t opa,
		lv_coord_t fill_w, lv_coord_t fill_h)
{
	hal_dma2d_handle_t *dma2d = &dma2d_ctx.hdma2d;
	lv_color32_t c32 = { .full = lv_color_to32(color) };
	int res = 0;

	/* clean the wholde vdb cache */
	lvgl_gpu_clean_dcache_cb(disp_drv);

	/* configure the output */
	dma2d->output_cfg.mode = (opa > LV_OPA_MAX) ? HAL_DMA2D_R2M : HAL_DMA2D_M2M_BLEND_FG;
	dma2d->output_cfg.output_offset = dest_w - fill_w;
	res = hal_dma2d_config_output(dma2d);
	if (res < 0) {
		goto out_exit;
	}

	if (opa > LV_OPA_MAX) {
		/* start filling */
		res = hal_dma2d_start(dma2d, c32.full, (uint32_t)dest, fill_w, fill_h);
		if (res < 0) {
			goto out_exit;
		}
	} else {
		/* configure the background */
		dma2d->layer_cfg[0].input_offset = dest_w - fill_w;
		res = hal_dma2d_config_layer(dma2d, HAL_DMA2D_BACKGROUND_LAYER);
		if (res < 0) {
			goto out_exit;
		}

		/* start blending fg */
		c32.ch.alpha = opa;
		res = hal_dma2d_blending_start(dma2d, c32.full,
						(uint32_t)dest, (uint32_t)dest, fill_w, fill_h);
		if (res < 0) {
			goto out_exit;
		}
	}

	dma2d_ctx.alloc_seq = (uint16_t)res;
	merge_pending_cache(dest, dest_w, fill_w, fill_h);
out_exit:
	if (res < 0) {
		SYS_LOG_DBG("gpu fill failed (%p %d 0x%x %d %d)",
				dest, dest_w, c32.full, fill_w, fill_h);
	}
	return res;
}

static int lvgl_gpu_fill_mask_cb(lv_disp_drv_t * disp_drv,
		lv_color_t * dest, lv_coord_t dest_w, lv_color_t color, lv_opa_t opa,
		const void * mask, lv_coord_t mask_w, lv_img_cf_t mask_cf,
		lv_coord_t fill_w, lv_coord_t fill_h)
{
	hal_dma2d_handle_t *dma2d = &dma2d_ctx.hdma2d;
	lv_color32_t c32 = { .full = lv_color_to32(color) };
	int res = 0;

	/* clean the wholde vdb cache */
	lvgl_gpu_clean_dcache_cb(disp_drv);

	/* configure the output */
	dma2d->output_cfg.mode = HAL_DMA2D_M2M_BLEND;
	dma2d->output_cfg.output_offset = dest_w - fill_w;
	res = hal_dma2d_config_output(dma2d);
	if (res < 0) {
		goto out_exit;
	}

	/* configure the background */
	dma2d->layer_cfg[0].input_offset = dest_w - fill_w;
	res = hal_dma2d_config_layer(dma2d, HAL_DMA2D_BACKGROUND_LAYER);
	if (res < 0) {
		goto out_exit;
	}

	/* configure the foreground */
	dma2d->layer_cfg[1].input_offset = mask_w - fill_w;
	dma2d->layer_cfg[1].input_alpha = (c32.full & 0x00ffffff) | ((uint32_t)opa << 24);
	dma2d->layer_cfg[1].alpha_mode = HAL_DMA2D_COMBINE_ALPHA;
	switch (mask_cf) {
	case LV_IMG_CF_ALPHA_8BIT:
		dma2d->layer_cfg[1].color_mode = HAL_DMA2D_A8;
		break;
	case LV_IMG_CF_ALPHA_4BIT_LE:
		dma2d->layer_cfg[1].color_mode = HAL_DMA2D_A4_LE;
		break;
	case LV_IMG_CF_ALPHA_1BIT_LE:
		dma2d->layer_cfg[1].color_mode = HAL_DMA2D_A1_LE;
		break;
	default:
		return -EINVAL;
	}

	res = hal_dma2d_config_layer(dma2d, HAL_DMA2D_FOREGROUND_LAYER);
	if (res < 0) {
		goto out_exit;
	}

	/* start blending */
	res = hal_dma2d_blending_start(dma2d, (uint32_t)mask,
					(uint32_t)dest, (uint32_t)dest, fill_w, fill_h);
	if (res < 0) {
		goto out_exit;
	}

	dma2d_ctx.alloc_seq = (uint16_t)res;
	merge_pending_cache(dest, dest_w, fill_w, fill_h);
out_exit:
	if (res < 0) {
		SYS_LOG_DBG("gpu fill mask failed (%p %d 0x%x %p %d %d %d %d)",
				dest, dest_w, c32.full, mask, mask_w, mask_cf, fill_w, fill_h);
	}
	return res;
}

int lvgl_gpu_copy(lv_color_t * dest, lv_coord_t dest_w, const void * src,
		lv_coord_t src_w, lv_img_cf_t src_cf, lv_coord_t copy_w,
		lv_coord_t copy_h)
{
	hal_dma2d_handle_t *dma2d = &dma2d_ctx.hdma2d;
	int res = 0;

#if OPT_COPY_CORNER_CASE
	if (LVGL_DMA2D_COLOR_MODE == HAL_DMA2D_RGB565) {
		if (copy_w <= 3) {
			const lv_color_t * src_tmp = (lv_color_t * )src;
			lv_color_t * dest_tmp = dest;
			bool cached = mem_is_cacheable(dest);
			int j;

			if (cached)
				dest_tmp = mem_addr_to_uncache(dest);

			if (copy_w == 1) {
				for (j = copy_h; j > 0; j--) {
					dest_tmp[0] = src_tmp[0];
					dest_tmp += dest_w;
					src_tmp += src_w;
				}
			} else {
				for (j = copy_h; j > 0; j--) {
					dest_tmp[0] = src_tmp[0];
					dest_tmp[1] = src_tmp[1];
					dest_tmp += dest_w;
					src_tmp += src_w;
				}
			}

			if (cached)
				mem_writebuf_clean_all();

			return 0;
		}

		if (!(copy_w & 0x1) && (copy_w > 3) && !(src_w & 0x1) && ((uintptr_t)dest & 0x3)) {
			const lv_color_t * src_tmp = (lv_color_t * )src;
			lv_color_t * dest_tmp = dest;
			bool cached = mem_is_cacheable(dest);
			int j;

			if (cached)
				dest_tmp = mem_addr_to_uncache(dest);

			for (j = copy_h; j > 0; j--) {
				dest_tmp[0] = src_tmp[0];
				dest_tmp += dest_w;
				src_tmp += src_w;
			}

			if (cached)
				mem_writebuf_clean_all();

			dest++;
			src = (lv_color_t * )src + 1;
			copy_w--;
		}
	}
#endif /* OPT_COPY_CORNER_CASE */

	/* configure the output */
	dma2d->output_cfg.mode = HAL_DMA2D_M2M;
	dma2d->output_cfg.output_offset = dest_w - copy_w;
	res = hal_dma2d_config_output(dma2d);
	if (res < 0) {
		goto out_exit;
	}

	/* configure the foreground */
	dma2d->layer_cfg[1].input_offset = src_w - copy_w;
	dma2d->layer_cfg[1].alpha_mode = HAL_DMA2D_NO_MODIF_ALPHA;
	if (src_cf == LV_IMG_CF_ARGB_8888) {
		dma2d->layer_cfg[1].rb_swap = 0;
		dma2d->layer_cfg[1].color_mode = HAL_DMA2D_ARGB8888;
	} else {
		dma2d->layer_cfg[1].rb_swap = LVGL_DMA2D_RB_SWAP;
		dma2d->layer_cfg[1].color_mode = LVGL_DMA2D_COLOR_MODE;
	}

	res = hal_dma2d_config_layer(dma2d, HAL_DMA2D_FOREGROUND_LAYER);
	if (res < 0) {
		goto out_exit;
	}

	res = hal_dma2d_start(dma2d, (uint32_t)src, (uint32_t)dest,
						copy_w, copy_h);
	if (res < 0) {
		goto out_exit;
	}

	dma2d_ctx.alloc_seq = (uint16_t)res;
out_exit:
	if (res < 0) {
		SYS_LOG_DBG("gpu copy failed (%p %d %p %d %d %d %d)",
				dest, dest_w, src, src_w, src_cf, copy_w, copy_h);
	}
	return res;
}

static int lvgl_gpu_copy_cb(lv_disp_drv_t * disp_drv,
		lv_color_t * dest, lv_coord_t dest_w, const void * src,
		lv_coord_t src_w, lv_coord_t copy_w, lv_coord_t copy_h)
{
	int res = 0;

	/* clean the wholde vdb cache */
	lvgl_gpu_clean_dcache_cb(disp_drv);

	res = lvgl_gpu_copy(dest, dest_w, src, src_w, LV_IMG_CF_TRUE_COLOR, copy_w, copy_h);
	if (res >= 0) {
		merge_pending_cache(dest, dest_w, copy_w, copy_h);
	}

	return res;
}

static int lvgl_gpu_blend_cb(lv_disp_drv_t * disp_drv,
		lv_color_t * dest, lv_coord_t dest_w, const void * src,
		lv_coord_t src_w, lv_img_cf_t src_cf, lv_coord_t copy_w,
		lv_coord_t copy_h, lv_opa_t opa)
{
	hal_dma2d_handle_t *dma2d = &dma2d_ctx.hdma2d;
	int res = 0;

	/* clean the wholde vdb cache */
	lvgl_gpu_clean_dcache_cb(disp_drv);

	/* configure the output */
	dma2d->output_cfg.mode = HAL_DMA2D_M2M_BLEND;
	dma2d->output_cfg.output_offset = dest_w - copy_w;
	res = hal_dma2d_config_output(dma2d);
	if (res < 0) {
		goto out_exit;
	}

	/* configure the background */
	dma2d->layer_cfg[0].input_offset = dest_w - copy_w;
	res = hal_dma2d_config_layer(dma2d, HAL_DMA2D_BACKGROUND_LAYER);
	if (res < 0) {
		goto out_exit;
	}

	/* configure the foreground */
	dma2d->layer_cfg[1].input_offset = src_w - copy_w;
	dma2d->layer_cfg[1].input_alpha = (uint32_t)opa << 24;
	dma2d->layer_cfg[1].alpha_mode = HAL_DMA2D_COMBINE_ALPHA;
	if (src_cf == LV_IMG_CF_ARGB_8888) {
		dma2d->layer_cfg[1].rb_swap = 0;
		dma2d->layer_cfg[1].color_mode = HAL_DMA2D_ARGB8888;
	} else if (src_cf == LV_IMG_CF_ARGB_6666) {
		dma2d->layer_cfg[1].rb_swap = 0;
		dma2d->layer_cfg[1].color_mode = HAL_DMA2D_ARGB6666;
	} else {
		dma2d->layer_cfg[1].rb_swap = LVGL_DMA2D_RB_SWAP;
		dma2d->layer_cfg[1].color_mode = LVGL_DMA2D_COLOR_MODE;
	}

	res = hal_dma2d_config_layer(dma2d, HAL_DMA2D_FOREGROUND_LAYER);
	if (res < 0) {
		goto out_exit;
	}

	res = hal_dma2d_blending_start(dma2d, (uint32_t)src,
					(uint32_t)dest, (uint32_t)dest, copy_w, copy_h);
	if (res < 0) {
		goto out_exit;
	}

	dma2d_ctx.alloc_seq = (uint16_t)res;
	merge_pending_cache(dest, dest_w, copy_w, copy_h);
out_exit:
	if (res < 0) {
		SYS_LOG_DBG("gpu blend failed (%p %d %p %d %d %d %d %d)",
				dest, dest_w, src, src_w, src_cf, copy_w, copy_h, opa);
	}
	return res;
}

static int lvgl_gpu_rotate_config_cb(lv_disp_drv_t * disp_drv,
		lv_coord_t outer_diameter, lv_coord_t inner_diameter, uint16_t angle,
		lv_color_t fill_color)
{
	hal_dma2d_handle_t *dma2d = &dma2d_ctx.hdma2d;
	int res = 0;

	/* configure the rotation */
	dma2d->rotation_cfg.input_offset = 0;
	dma2d->rotation_cfg.rb_swap = LVGL_DMA2D_RB_SWAP;
	dma2d->rotation_cfg.color_mode = LVGL_DMA2D_COLOR_MODE;
	dma2d->rotation_cfg.angle = angle;
	dma2d->rotation_cfg.outer_diameter = outer_diameter;
	dma2d->rotation_cfg.inner_diameter = inner_diameter;
	dma2d->rotation_cfg.fill_enable = 1;
	dma2d->rotation_cfg.fill_color = lv_color_to32(fill_color);

	res = hal_dma2d_config_rotation(dma2d);
	if (res < 0) {
		SYS_LOG_DBG("gpu rotate prepare failed (%u %u %u 0x%x)",
				outer_diameter, inner_diameter, angle, fill_color.full);
	}

	return res;
}

static int lvgl_gpu_rotate_start_cb(lv_disp_drv_t * disp_drv,
		lv_color_t * dest, lv_coord_t dest_w, const lv_color_t * src,
		lv_coord_t row_start, lv_coord_t row_count)
{
	hal_dma2d_handle_t *dma2d = &dma2d_ctx.hdma2d;
	int res = 0;

	/* clean the wholde vdb cache */
	lvgl_gpu_clean_dcache_cb(disp_drv);

	/* configure the output */
	dma2d->output_cfg.mode = HAL_DMA2D_M2M_ROTATE;
	dma2d->output_cfg.output_offset = dest_w - dma2d->rotation_cfg.outer_diameter;

	res = hal_dma2d_config_output(dma2d);
	if (res < 0) {
		goto out_exit;
	}

	res = hal_dma2d_rotation_start(dma2d, (uint32_t)src, (uint32_t)dest, row_start, row_count);
	if (res < 0) {
		goto out_exit;
	}

	dma2d_ctx.alloc_seq = (uint16_t)res;
	merge_pending_cache(dest, dest_w, dma2d->rotation_cfg.outer_diameter, row_count);
out_exit:
	if (res < 0) {
		SYS_LOG_DBG("gpu rotate failed (%p %d %d %d)",
				dest, dest_w, row_start, row_count);
	}
	return res;
}

void lvgl_gpu_insert_marker(lvgl_gpu_event_cb_t event_cb, void *user_data)
{
	bool completed = false;

	unsigned int key = irq_lock();

	if (hal_dma2d_get_state(&dma2d_ctx.hdma2d) != HAL_DMA2D_STATE_BUSY) {
		completed = true;
	} else {
		dma2d_ctx.event_seq = dma2d_ctx.alloc_seq;
		dma2d_ctx.event_cb = event_cb;
		dma2d_ctx.event_data = user_data;
	}

	os_sem_reset(&dma2d_ctx.event_sem);

	irq_unlock(key);

	if (completed) {
		event_cb(0, user_data);
	}
}

void lvgl_gpu_wait_marker_finish(void)
{
	if (dma2d_ctx.event_cb) {
		os_sem_take(&dma2d_ctx.event_sem, OS_FOREVER);
		dma2d_ctx.event_cb = NULL;
	}
}

int set_lvgl_gpu_cb(lv_disp_drv_t *disp_drv)
{
	if (lvgl_gpu_init()) {
		SYS_LOG_ERR("gpu init failed");
		return -ENODEV;
	}

	disp_drv->gpu_fill_cb = lvgl_gpu_fill_cb;
	disp_drv->gpu_fill_mask_cb = lvgl_gpu_fill_mask_cb;
	disp_drv->gpu_copy_cb = lvgl_gpu_copy_cb;
	disp_drv->gpu_blend_cb = lvgl_gpu_blend_cb;
	disp_drv->gpu_rotate_config_cb = lvgl_gpu_rotate_config_cb;
	disp_drv->gpu_rotate_start_cb = lvgl_gpu_rotate_start_cb;
	disp_drv->gpu_wait_cb = lvgl_gpu_wait_cb;
	disp_drv->clean_dcache_cb = lvgl_gpu_clean_dcache_cb;

	return 0;
}
