/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LVGL image loader interface
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_LVGL_ANIMIMG_LOADER_H_
#define FRAMEWORK_DISPLAY_INCLUDE_LVGL_ANIMIMG_LOADER_H_

/**
 * @defgroup lvgl_image_apis LVGL Image Loader APIs
 * @ingroup lvgl_apis
 * @{
 */

#include "lvgl_res_loader.h"

#ifdef __cplusplus
extern "C" {
#endif

struct lvgl_img_loader;

typedef struct lvgl_img_loader_fn {
	/* load a image */
	int (*load)(struct lvgl_img_loader * loader, uint16_t index, lv_img_dsc_t * dsc, lv_point_t * pos);
	/* (optional) unload a image */
	void (*unload)(struct lvgl_img_loader * loader, lv_img_dsc_t * dsc);
	/* (optional) preload a image */
	void (*preload)(struct lvgl_img_loader * loader, uint16_t index);
	/* get image count */
	uint16_t (*get_count)(struct lvgl_img_loader * loader);
} lvgl_img_loader_fn_t;

typedef struct lvgl_img_loader {
	const lvgl_img_loader_fn_t * fn;

	union {
		struct { /* picarray loader */
			const lv_img_dsc_t * dsc;
			const lv_point_t * pos;
			uint16_t count;
		} picarray;

		struct { /* picregion loader */
			lvgl_res_picregion_t * node;
		} picregion;

		struct { /* picgroup loader */
			lvgl_res_group_t * node;
			const uint32_t * ids;
			uint16_t count;
		} picgroup;

		 /* custom loader */
		void * user_data;
	};
} lvgl_img_loader_t;

/**
 * Set the image array resource
 *
 * @param loader pointer to structure lvgl_img_loader
 * @param dsc pointer to a series images
 * @param pos position array of the images
 * @param num number of images
 *
 * @retval 0 on success else negative code.
 */
int lvgl_img_loader_set_picarray(lvgl_img_loader_t * loader,
		const lv_img_dsc_t * dsc, const lv_point_t * pos, uint16_t num);

/**
 * Set the picture region resource
 *
 * @param loader pointer to structure lvgl_img_loader
 * @param picregion pointer to structure lvgl_res_picregion
 *
 * @retval 0 on success else negative code.
 */
int lvgl_img_loader_set_picregion(lvgl_img_loader_t * loader,
		lvgl_res_picregion_t * picregion);

/**
 * Set the picture group resource
 *
 * @param loader pointer to structure lvgl_img_loader
 * @param picgroup pointer to structure lvgl_res_group
 * @param ids picture ids in the picgroup
 * @param num number of pictures
 *
 * @retval 0 on success else negative code.
 */
int lvgl_img_loader_set_picgroup(lvgl_img_loader_t * loader,
		lvgl_res_group_t * picgroup, const uint32_t * ids, uint16_t num);

/**
 * Set the custom resource
 *
 * @param loader pointer to structure lvgl_img_loader
 * @param user_data custom data
 *
 * @retval 0 on success else negative code.
 */
int lvgl_img_loader_set_custom(lvgl_img_loader_t *loader,
		const lvgl_img_loader_fn_t * fn, void * user_data);

/**
 * Query the loader is initialized or not
 *
 * @param loader pointer to structure lvgl_img_loader
 *
 * @retval image count
 */
static inline bool lvgl_img_loader_is_inited(lvgl_img_loader_t * loader)
{
    return loader->fn != NULL;
}

/**
 * Get image count
 *
 * @param loader pointer to structure lvgl_img_loader
 *
 * @retval image count
 */
static inline uint16_t lvgl_img_loader_get_count(lvgl_img_loader_t * loader)
{
	return loader->fn->get_count(loader);
}

/**
 * load an image count
 *
 * @param loader pointer to structure lvgl_img_loader
 * @param index image index to load
 * @param dsc pointer to structure lv_img_dsc_t to store the image
 * @param pos pointer to structure lv_point_t to store the image position
 *
 * @retval 0 on success else negative code
 */
static inline int lvgl_img_loader_load(lvgl_img_loader_t * loader, uint16_t index, lv_img_dsc_t * dsc, lv_point_t * pos)
{
	return loader->fn->load(loader, index, dsc, pos);		
}

/**
 * Unload an image count
 *
 * @param loader pointer to structure lvgl_img_loader
 * @param index image index to unload
 * @param dsc pointer to an image dsc returned by load()
 *
 * @retval N/A
 */
static inline void lvgl_img_loader_unload(lvgl_img_loader_t * loader, lv_img_dsc_t * dsc)
{
	if (loader->fn->unload)
		loader->fn->unload(loader, dsc);		
}

/**
 * Preload an image count
 *
 * @param loader pointer to structure lvgl_img_loader
 * @param index image index to preload
 *
 * @retval N/A
 */
static inline void lvgl_img_loader_preload(lvgl_img_loader_t * loader, uint16_t index)
{
	if (loader->fn->preload)
		loader->fn->preload(loader, index);		
}

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* FRAMEWORK_DISPLAY_INCLUDE_LVGL_ANIMIMG_LOADER_H_ */
