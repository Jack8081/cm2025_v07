/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <lvgl/lvgl_img_loader.h>

/**********************
 *  STATIC PROTOTYPES
 **********************/

static int picarray_load(lvgl_img_loader_t * loader, uint16_t index, lv_img_dsc_t * dsc, lv_point_t * pos);
static uint16_t picarray_get_count(lvgl_img_loader_t *loader);

static int picregion_load(lvgl_img_loader_t * loader, uint16_t index, lv_img_dsc_t * dsc, lv_point_t * pos);
static void picregion_unload(lvgl_img_loader_t * loader, lv_img_dsc_t * dsc);
static void picregion_preload(lvgl_img_loader_t * loader, uint16_t index);
static uint16_t picregion_get_count(lvgl_img_loader_t *loader);

static int picgroup_load(lvgl_img_loader_t * loader, uint16_t index, lv_img_dsc_t * dsc, lv_point_t * pos);
static void picgroup_unload(lvgl_img_loader_t * loader, lv_img_dsc_t * dsc);
static void picgroup_preload(lvgl_img_loader_t * loader, uint16_t index);
static uint16_t picgroup_get_count(lvgl_img_loader_t * loader);

/**********************
 *  STATIC VARIABLES
 **********************/

static const lvgl_img_loader_fn_t picarray_loader_fn = {
	.load = picarray_load,
	.get_count = picarray_get_count,
};

static const lvgl_img_loader_fn_t picregion_loader_fn = {
	.load = picregion_load,
	.unload = picregion_unload,
	.preload = picregion_preload,
	.get_count = picregion_get_count,
};

static const lvgl_img_loader_fn_t picgroup_loader_fn = {
	.load = picgroup_load,
	.unload = picgroup_unload,
	.preload = picgroup_preload,
	.get_count = picgroup_get_count,
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int lvgl_img_loader_set_picarray(lvgl_img_loader_t * loader,
		const lv_img_dsc_t * dsc, const lv_point_t * pos, uint16_t num)
{
	memset(loader, 0, sizeof(*loader));
	loader->fn = &picarray_loader_fn;
	loader->picarray.dsc = dsc;
	loader->picarray.pos = pos;
	loader->picarray.count = num;
	return 0;
}

int lvgl_img_loader_set_picregion(lvgl_img_loader_t * loader,
		lvgl_res_picregion_t * picregion)
{
	memset(loader, 0, sizeof(*loader));
	loader->fn = &picregion_loader_fn;
	loader->picregion.node = picregion;
	return 0;
}

int lvgl_img_loader_set_picgroup(lvgl_img_loader_t * loader,
		lvgl_res_group_t * picgroup, const uint32_t * ids, uint16_t num)
{
	memset(loader, 0, sizeof(*loader));
	loader->fn = &picgroup_loader_fn;
	loader->picgroup.node = picgroup;
	loader->picgroup.ids = ids;
	loader->picgroup.count = num;
	return 0;
}

int lvgl_img_loader_set_custom(lvgl_img_loader_t *loader,
		const lvgl_img_loader_fn_t * fn, void * user_data)
{
	memset(loader, 0, sizeof(*loader));
	loader->fn = fn;
	loader->user_data = user_data;
	return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static int picarray_load(lvgl_img_loader_t * loader, uint16_t index, lv_img_dsc_t * dsc, lv_point_t * pos)
{
	if (pos && loader->picarray.pos) {
		pos->x = loader->picarray.pos[index].x;
		pos->y = loader->picarray.pos[index].y;
	}

	memcpy(dsc, &loader->picarray.dsc[index], sizeof(*dsc));
	return 0;
}

static uint16_t picarray_get_count(lvgl_img_loader_t * loader)
{
	return loader->picarray.count;
}

static int picregion_load(lvgl_img_loader_t * loader, uint16_t index, lv_img_dsc_t * dsc, lv_point_t * pos)
{
	int res = lvgl_res_load_pictures_from_picregion(loader->picregion.node, index, index, dsc);
	if (res == 0) {
		if (pos) {
			pos->x = loader->picregion.node->x;
			pos->y = loader->picregion.node->y;
		}
	}

	return res;
}

static void picregion_unload(lvgl_img_loader_t *loader, lv_img_dsc_t * dsc)
{
	if (dsc->data) {
		lvgl_res_unload_pictures(dsc, 1);
		dsc->data = NULL;
	}
}

static void picregion_preload(lvgl_img_loader_t *loader, uint16_t index)
{
	lvgl_res_preload_pictures_from_picregion(0, loader->picregion.node, index, index);
}

static uint16_t picregion_get_count(lvgl_img_loader_t *loader)
{
	return loader->picregion.node->frames;
}

static int picgroup_load(lvgl_img_loader_t *loader, uint16_t index, lv_img_dsc_t * dsc, lv_point_t * pos)
{
	return lvgl_res_load_pictures_from_group(loader->picgroup.node,
			loader->picgroup.ids + index, dsc, pos, 1);
}

static void picgroup_unload(lvgl_img_loader_t *loader, lv_img_dsc_t * dsc)
{
	if (dsc->data) {
		lvgl_res_unload_pictures(dsc, 1);
		dsc->data = NULL;
	}
}

static void picgroup_preload(lvgl_img_loader_t *loader, uint16_t index)
{
	lvgl_res_preload_pictures_from_group(0, loader->picgroup.node,
			loader->picgroup.ids + index, 1);
}

static uint16_t picgroup_get_count(lvgl_img_loader_t *loader)
{
	return loader->picgroup.count;
}
