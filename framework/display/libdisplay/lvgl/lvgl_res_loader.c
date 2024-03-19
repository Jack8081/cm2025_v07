#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <os_common_api.h>
#include <res_manager_api.h>
#include <lvgl/lvgl_res_loader.h>
#ifdef CONFIG_UI_MANAGER
#include <ui_manager.h>
#endif


#define RES_PRELOAD_STACKSIZE	1536

#define MAX_RESOURCE_SETS		5

typedef enum
{
	PRELOAD_TYPE_IMMEDIATE,
	PRELOAD_TYPE_NORMAL,
	PRELOAD_TYPE_NORMAL_COMPACT,
	PRELOAD_TYPE_BEGIN_CALLBACK,
	PRELOAD_TYPE_END_CALLBACK,
}preload_type_e;

typedef struct _preload_default_t
{
	uint32_t scene_id;
	uint32_t* resource_id;
	uint32_t resource_num;
	void (*callback)(int32_t status, void* param);
	char* style_path;
	char* picture_path;
	char* text_path;
	struct _preload_default_t* next;
}preload_default_t;


static os_sem load_sem;
static os_sem preload_sem;
static os_mutex preload_mutex;
static uint32_t preload_running = 1;
static preload_param_t* param_list = NULL;
static preload_param_t* sync_param_list = NULL;

static lvgl_res_scene_t current_scene;
static lvgl_res_group_t current_group;
static lvgl_res_group_t current_subgrp;

static resource_info_t* res_info[MAX_RESOURCE_SETS];
static int32_t res_manager_inited = 0;

//static int32_t current_preload_count = 0;
static preload_default_t* preload_default_list = NULL;

static int screen_width;
static int screen_height;


static char __aligned(ARCH_STACK_PTR_ALIGN) __in_section_unique(ram.noinit.stack)
res_preload_stack[CONFIG_LVGL_RES_PRELOAD_STACKSIZE];

static void _res_preload_thread(void *parama1, void *parama2, void *parama3);


void lvgl_res_cache_clear(uint32_t force_clear)
{
	res_manager_clear_cache(force_clear);
}

int lvgl_res_loader_init(uint32_t screen_w, uint32_t screen_h)
{
	res_manager_init();
	res_manager_set_screen_size(screen_w, screen_h);

	/* create a thread to preload*/
#if 1
	os_sem_init(&load_sem, 0, 1);
	os_sem_init(&preload_sem, 0, 1);
	os_mutex_init(&preload_mutex);

	int tid = os_thread_create(res_preload_stack, RES_PRELOAD_STACKSIZE,
		_res_preload_thread,
		NULL, NULL, NULL,
		CONFIG_LVGL_RES_PRELOAD_PRIORITY, 0, 0);

	os_thread_name_set((os_tid_t)tid, "res_preload");
#endif
	memset(&current_scene, 0, sizeof(lvgl_res_scene_t));
	memset(&current_group, 0, sizeof(lvgl_res_group_t));
	memset(&current_subgrp, 0, sizeof(lvgl_res_group_t));
	memset(res_info, 0, sizeof(resource_info_t*)*MAX_RESOURCE_SETS);

	screen_width = screen_w;
	screen_height = screen_h;
	
	res_manager_inited = 1;
	return 0;
}

void lvgl_res_loader_deinit(void)
{
	uint32_t i;

	if(res_manager_inited == 0)
	{
		return;
	}

	for(i=0;i<MAX_RESOURCE_SETS;i++)
	{
		if(res_info[i] != NULL)
		{
			res_manager_close_resources(res_info[i]);
		}
		res_info[i] = NULL;
	}
}

void _res_file_close(resource_info_t* info, uint32_t force_close)
{
	uint32_t i;

	if(info == NULL)
	{
		SYS_LOG_ERR("null resource info when unload");
		return;
	}

	for(i=0;i<MAX_RESOURCE_SETS;i++)
	{
		if(res_info[i] == info)
		{
			if(res_info[i]->reference >= 1)
			{
				res_info[i]->reference--;
			}

			if(res_info[i]->reference == 0 && force_close)
			{
				res_info[i] = NULL;
				break;
			}
			else
			{
				return;
			}
		}
	}

	if(i >= MAX_RESOURCE_SETS)
	{
		SYS_LOG_ERR("resource info not found when unload");
		return;
	}

	res_manager_close_resources(info);
}

resource_info_t* _res_file_open(const char* style_path, const char* picture_path, const char* text_path, uint32_t force_ref)
{
	resource_info_t* info;
	uint32_t i;
	int32_t empty_slot = -1;
	int32_t expired_slot = -1;

	SYS_LOG_INF("current open %s\n", style_path);
	for(i=0;i<MAX_RESOURCE_SETS;i++)
	{
		SYS_LOG_INF("res set %d: %s, ref %d\n", i, res_info[i]->sty_path, res_info[i]->reference);
	}	

	for(i=0;i<MAX_RESOURCE_SETS;i++)
	{

		if(res_info[i] != NULL)
		{
			if(strcmp(style_path, res_info[i]->sty_path)==0)
			{
				if(force_ref)
				{
					res_info[i]->reference++;
				}

				if(strcmp(text_path, res_info[i]->str_path) != 0)
				{
					SYS_LOG_INF("testres: change str file %s to %s\n", res_info[i]->str_path, text_path);
					res_manager_set_str_file(res_info[i], text_path);
				}
				return res_info[i];
			}
			else if(res_info[i]->reference == 0)
			{
				expired_slot = i;
			}

		}
		else if(empty_slot == -1)
		{
			empty_slot = i;
		}
	}

	if(empty_slot == -1 && expired_slot == -1)
	{
		SYS_LOG_ERR("too many resources sets opened");
		for(i=0;i<MAX_RESOURCE_SETS;i++)
		{
			SYS_LOG_INF("res set %d: %s\n", i, res_info[i]->sty_path);
		}
		return NULL;
	}

	info = res_manager_open_resources(style_path, picture_path, text_path);
	if(info == NULL)
	{
		SYS_LOG_ERR("open resources failed");
		return NULL;
	}

	if(force_ref)
	{
		info->reference = 1;
	}

	if(empty_slot != -1)
	{
		res_info[empty_slot] = info;
	}
	else
	{
		_res_file_close(res_info[expired_slot], 1);
		res_info[expired_slot] = info;
	}

	return info;
}


int lvgl_res_load_scene(uint32_t scene_id, lvgl_res_scene_t* scene, const char* style_path, const char* picture_path, const char* text_path)
{
	resource_scene_t* res_scene;
	if(scene == NULL)
	{
		SYS_LOG_ERR("null scene param\n");
		return -1;
	}

	os_strace_u32(SYS_TRACE_ID_RES_SCENE_LOAD, (uint32_t)scene_id);
	scene->res_info = _res_file_open(style_path, picture_path, text_path, 1);
	if(scene->res_info == NULL)
	{
		SYS_LOG_ERR("resource info error\n");
		return -1;
	}

	res_scene = res_manager_load_scene(scene->res_info, scene_id);
	if(res_scene == NULL)
	{
		return -1;
	}

	scene->id = scene_id;
	scene->scene_data = res_scene;
	scene->background = lv_color_hex(res_scene->background);
	scene->x = res_scene->x;
	scene->y = res_scene->y;
	scene->width = res_scene->width;
	scene->height = res_scene->height;
	scene->transparence = res_scene->transparence;
	os_strace_end_call_u32(SYS_TRACE_ID_RES_SCENE_LOAD, (uint32_t)scene_id);
	return 0;
}

int32_t lvgl_res_set_pictures_regular(uint32_t scene_id, uint32_t group_id, uint32_t subgroup_id, uint32_t* id, uint32_t num,
												const uint8_t* style_path, const char* picture_path, const char* text_path)
{
	resource_info_t* info;
	int32_t ret;

	info = _res_file_open(style_path, picture_path, text_path, 0);
	if(info == NULL)
	{
		SYS_LOG_ERR("open resource info error\n");
		return -1;
	}

	ret = res_manager_set_pictures_regular(info, scene_id, group_id, subgroup_id, id, num);
	if(ret < 0)
	{
		SYS_LOG_ERR("set regular resource failed\n");
		return -1;
	}

	return ret;
}

int32_t lvgl_res_clear_regular_pictures(uint32_t scene_id, const uint8_t* style_path)
{
	int32_t ret;

	ret = res_manager_clear_regular_pictures(style_path, scene_id);
	if(ret < 0)
	{
		SYS_LOG_ERR("clear resource regular info failed\n");
		return -1;
	}
	return 0;
}

int lvgl_res_load_group_from_scene(lvgl_res_scene_t* scene, uint32_t id, lvgl_res_group_t* group)
{
	resource_group_t* res_group;

	if(scene == NULL || group == NULL)
	{
		SYS_LOG_ERR("null scene %p or null group %p\n", scene, group);
		return -1;
	}

	res_group = (resource_group_t*) res_manager_get_scene_child(scene->res_info, scene->scene_data, id);
	if(res_group == NULL)
	{
		return -1;
	}

	group->id = id;
	group->group_data = res_group;
	group->x = res_group->sty_data->x;
	group->y = res_group->sty_data->y;
	group->width = res_group->sty_data->width;
	group->height = res_group->sty_data->height;
	group->res_info = scene->res_info;

	return 0;
}

int lvgl_res_load_group_from_group(lvgl_res_group_t* group, uint32_t id, lvgl_res_group_t* subgroup)
{
	resource_group_t* res_group;

	if(group == NULL || subgroup == NULL)
	{
		SYS_LOG_ERR("null scene %p or null group %p\n", group, subgroup);
		return -1;
	}

	res_group = res_manager_get_group_child(group->res_info, group->group_data, id);
	if(res_group == NULL)
	{
		return -1;
	}

	subgroup->id = id;
	subgroup->group_data = res_group;
	subgroup->x = res_group->sty_data->x;
	subgroup->y = res_group->sty_data->y;
	subgroup->width = res_group->sty_data->width;
	subgroup->height = res_group->sty_data->height;
	subgroup->res_info = group->res_info;

	return 0;
}

static int _cvt_bmp2dsc(lv_img_dsc_t *dsc, resource_bitmap_t *bmp)
{
	if (bmp->sty_data->width <= 0 || bmp->sty_data->height <= 0)
		return -EINVAL;

	switch (bmp->sty_data->format) {
	case RESOURCE_BITMAP_FORMAT_ARGB8888:
		dsc->header.cf = LV_IMG_CF_ARGB_8888;
		break;
#ifdef CONFIG_LV_COLOR_DEPTH_16
	case RESOURCE_BITMAP_FORMAT_RGB565:
		dsc->header.cf = LV_IMG_CF_TRUE_COLOR;
		break;
	case RESOURCE_BITMAP_FORMAT_ARGB8565:
		dsc->header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
		break;
#endif
	case RESOURCE_BITMAP_FORMAT_RGB888:
	default:
		return -EINVAL;
	}

	dsc->header.always_zero = 0;
	dsc->header.w = bmp->sty_data->width;
	dsc->header.h = bmp->sty_data->height;
	dsc->data = bmp->buffer;
	dsc->data_size = bmp->sty_data->width * bmp->sty_data->height * bmp->sty_data->bytes_per_pixel;

	return 0;
}


int lvgl_res_load_pictures_from_scene(lvgl_res_scene_t* scene, const uint32_t* id, lv_img_dsc_t* img, lv_point_t* pt, uint32_t num)
{
	resource_bitmap_t* bitmap;
	uint32_t i;

	if(scene == NULL || id == NULL)
	{
		SYS_LOG_ERR("null param %p, %p, %p", scene, id, img);
		return -1;
	}

	os_strace_u32(SYS_TRACE_ID_RES_PICS_LOAD, (uint32_t)scene->scene_data);
	if(img != NULL)
	{
		for(i=0;i<num;i++)
		{
			img[i].data = NULL;
		}
	}

	for(i=0;i<num;i++)
	{
		bitmap = (resource_bitmap_t*)res_manager_get_scene_child(scene->res_info, scene->scene_data, id[i]);
		if(bitmap == NULL)
		{
			if(img != NULL)
			{
				lvgl_res_unload_pictures(img, num);
			}
			return -1;
		}

		if(img != NULL)
		{
			_cvt_bmp2dsc(&img[i], bitmap);
		}

		if(pt != NULL)
		{
			pt[i].x = bitmap->sty_data->x;
			pt[i].y = bitmap->sty_data->y;
		}
		res_manager_free_resource_structure(bitmap);
	}
	os_strace_end_call_u32(SYS_TRACE_ID_RES_PICS_LOAD, (uint32_t)scene->scene_data);
	return 0;
}


int lvgl_res_load_strings_from_scene(lvgl_res_scene_t* scene, const uint32_t* id, lvgl_res_string_t* str, uint32_t num)
{
	resource_text_t* text;
	uint32_t i;

	if(scene == NULL || id == NULL || str == NULL)
	{
		SYS_LOG_ERR("null param %p, %p, %p", scene, id, str);
		return -1;
	}
	os_strace_u32(SYS_TRACE_ID_RES_STRS_LOAD, (uint32_t)scene->scene_data);

	for(i=0;i<num;i++)
	{
		str[i].txt = NULL;
	}

	for(i=0;i<num;i++)
	{
		text = (resource_text_t*)res_manager_get_scene_child(scene->res_info, scene->scene_data, id[i]);
		if(text == NULL)
		{
			lvgl_res_unload_strings(str, i);
			return -1;
		}

		str[i].txt = text->buffer;
		str[i].color = lv_color_hex(text->sty_data->color);
		str[i].bgcolor = lv_color_hex(text->sty_data->color);
		str[i].x = text->sty_data->x;
		str[i].y = text->sty_data->y;
		str[i].width = text->sty_data->width;
		str[i].height = text->sty_data->height;
		switch(text->sty_data->align)
		{
		case 0:
			str[i].align = LV_TEXT_ALIGN_LEFT;
			break;
		case 2:
			str[i].align = LV_TEXT_ALIGN_CENTER;
			break;
		case 1:
			str[i].align = LV_TEXT_ALIGN_RIGHT;
			break;
		default:
			str[i].align = LV_TEXT_ALIGN_AUTO;
			break;
		}
		res_manager_free_resource_structure(text);
	}
	os_strace_end_call_u32(SYS_TRACE_ID_RES_STRS_LOAD, (uint32_t)scene->scene_data);
	return 0;
}

int lvgl_res_load_pictures_from_group(lvgl_res_group_t* group, const uint32_t* id, lv_img_dsc_t* img, lv_point_t* pt, uint32_t num)
{
	resource_bitmap_t* bitmap = NULL;
	uint32_t i;

	if(group == NULL || id == NULL )
	{
		SYS_LOG_ERR("null param %p, %p, %p", group, id, img);
		return -1;
	}

	os_strace_u32(SYS_TRACE_ID_RES_PICS_LOAD, (uint32_t)group->group_data);
	if(img != NULL)
	{
		for(i=0;i<num;i++)
		{
			img[i].data = NULL;
		}
	}

	for(i=0;i<num;i++)
	{
		bitmap = (resource_bitmap_t*)res_manager_get_group_child(group->res_info, group->group_data, id[i]);
		if(bitmap == NULL)
		{
			if(img != NULL)
			{
				lvgl_res_unload_pictures(img, num);
			}
			return -1;
		}

		if(img != NULL)
		{
			_cvt_bmp2dsc(&img[i], bitmap);
		}

		if(pt != NULL)
		{
			pt[i].x = bitmap->sty_data->x;
			pt[i].y = bitmap->sty_data->y;
		}

		res_manager_free_resource_structure(bitmap);
		bitmap = NULL;
	}
	os_strace_end_call_u32(SYS_TRACE_ID_RES_PICS_LOAD, (uint32_t)group->group_data);
	return 0;

}

int lvgl_res_load_strings_from_group(lvgl_res_group_t* group, const uint32_t* id, lvgl_res_string_t* str, uint32_t num)
{
	resource_text_t* text;
	uint32_t i;

	if(group == NULL || id== NULL || str == NULL)
	{
		SYS_LOG_ERR("null param %p, %p, %p", group, id, str);
		return -1;
	}


	os_strace_u32(SYS_TRACE_ID_RES_STRS_LOAD, (uint32_t)group->group_data);
	for(i=0;i<num;i++)
	{
		str[i].txt = NULL;
	}

	for(i=0;i<num;i++)
	{
		text = (resource_text_t*)res_manager_get_group_child(group->res_info, group->group_data, id[i]);
		if(text == NULL)
		{
			lvgl_res_unload_strings(str, i);
			return -1;
		}

		str[i].txt = text->buffer;
		str[i].color = lv_color_hex(text->sty_data->color);
		str[i].bgcolor = lv_color_hex(text->sty_data->bgcolor);
		str[i].x = text->sty_data->x;
		str[i].y = text->sty_data->y;
		str[i].width = text->sty_data->width;
		str[i].height = text->sty_data->height;
		switch(text->sty_data->align)
		{
		case 0:
			str[i].align = LV_TEXT_ALIGN_LEFT;
			break;
		case 2:
			str[i].align = LV_TEXT_ALIGN_CENTER;
			break;
		case 1:
			str[i].align = LV_TEXT_ALIGN_RIGHT;
			break;
		default:
			str[i].align = LV_TEXT_ALIGN_AUTO;
			break;
		}
		res_manager_free_resource_structure(text);
	}

	os_strace_end_call_u32(SYS_TRACE_ID_RES_STRS_LOAD, (uint32_t)group->group_data);
	return 0;

}

void lvgl_res_unload_scene_compact(uint32_t scene_id)
{
	os_strace_string(SYS_TRACE_ID_RES_UNLOAD, "scene_compact");

	res_manager_unload_scene(scene_id, NULL);

	os_strace_end_call_u32(SYS_TRACE_ID_RES_UNLOAD, scene_id);
}

void lvgl_res_unload_scene(lvgl_res_scene_t * scene)
{
	if(scene && scene->scene_data)
	{
		os_strace_string(SYS_TRACE_ID_RES_UNLOAD, "scene");

//		res_manager_unload_scene(0, scene->scene_data);
		if(scene->res_info != NULL)
		{
			_res_file_close(scene->res_info, 0);
		}

		os_strace_end_call_u32(SYS_TRACE_ID_RES_UNLOAD, scene->id);

		scene->id = 0;
		scene->scene_data = NULL;
	}
}

void lvgl_res_unload_group(lvgl_res_group_t* group)
{
	if(group && group->group_data)
	{
		os_strace_string(SYS_TRACE_ID_RES_UNLOAD, "group");

		res_manager_release_resource(group->group_data);
		group->id = 0;
		group->group_data = NULL;

		os_strace_end_call_u32(SYS_TRACE_ID_RES_UNLOAD, 1);
	}
}

void lvgl_res_unload_picregion(lvgl_res_picregion_t* picreg)
{
	if(picreg && picreg->picreg_data)
	{
		os_strace_string(SYS_TRACE_ID_RES_UNLOAD, "picregion");

		res_manager_release_resource(picreg->picreg_data);
		picreg->picreg_data = NULL;

		os_strace_end_call_u32(SYS_TRACE_ID_RES_UNLOAD, 1);
	}
}

void lvgl_res_unload_pictures(lv_img_dsc_t* img, uint32_t num)
{
	uint32_t i;

	os_strace_string(SYS_TRACE_ID_RES_UNLOAD, "pictures");

	for(i=0;i<num;i++)
	{
		if(img[i].data != NULL)
		{
			res_manager_free_bitmap_data((void *)img[i].data);
			img[i].data = NULL;
		}
	}

	os_strace_end_call_u32(SYS_TRACE_ID_RES_UNLOAD, num);
}

void lvgl_res_unload_strings(lvgl_res_string_t* str, uint32_t num)
{
	uint32_t i;

	os_strace_string(SYS_TRACE_ID_RES_UNLOAD, "strings");

	for(i=0;i<num;i++)
	{
		if(str[i].txt != NULL)
		{
			res_manager_free_text_data(str[i].txt);
			str[i].txt = NULL;
		}
	}

	os_strace_end_call_u32(SYS_TRACE_ID_RES_UNLOAD, num);
}

int lvgl_res_load_picregion_from_group(lvgl_res_group_t* group, const uint32_t id, lvgl_res_picregion_t* res_picreg)
{
	resource_picregion_t* picreg;

	if(group == NULL)
	{
		SYS_LOG_ERR("null param %p", group);
		return -1;
	}

	picreg = res_manager_get_group_child(group->res_info, group->group_data, id);
	if(picreg == NULL)
	{
		SYS_LOG_ERR("load pic region faild");
		return -1;
	}

	res_picreg->x = picreg->sty_data->x;
	res_picreg->y = picreg->sty_data->y;
	res_picreg->width = picreg->sty_data->width;
	res_picreg->height = picreg->sty_data->height;
	res_picreg->frames = picreg->sty_data->frames;
	res_picreg->picreg_data = picreg;
	res_picreg->res_info = group->res_info;
	return 0;
}

int lvgl_res_load_picregion_from_scene(lvgl_res_scene_t* scene, const uint32_t id, lvgl_res_picregion_t* res_picreg)
{
	resource_picregion_t* picreg;

	if(scene == NULL)
	{
		SYS_LOG_ERR("null param %p", scene);
		return -1;
	}

	picreg = res_manager_get_scene_child(scene->res_info, scene->scene_data, id);
	if(picreg == NULL)
	{
		SYS_LOG_ERR("load pic region faild");
		return -1;
	}

	res_picreg->x = picreg->sty_data->x;
	res_picreg->y = picreg->sty_data->y;
	res_picreg->width = picreg->sty_data->width;
	res_picreg->height = picreg->sty_data->height;
	res_picreg->frames = (uint32_t)picreg->sty_data->frames;
	res_picreg->picreg_data = picreg;
	res_picreg->res_info = scene->res_info;

	return 0;
}

int lvgl_res_load_pictures_from_picregion(lvgl_res_picregion_t* picreg, uint32_t start, uint32_t end, lv_img_dsc_t* img)
{
	uint32_t i;
	resource_bitmap_t* bitmap;

	if(picreg == NULL || img == NULL ||
		start >= picreg->frames ||
		start > end)
	{
		SYS_LOG_ERR("invalid param %p, %p, %d, %d", picreg, img, start, end);
		return -1;
	}

	os_strace_u32(SYS_TRACE_ID_RES_PICS_LOAD, (uint32_t)picreg->picreg_data);
	for(i=0; i<=end - start; i++)
	{
		img[i].data = NULL;
	}

	if(end >= picreg->frames)
	{
		end = picreg->frames-1;
	}

	for(i=start; i<=end; i++)
	{
		bitmap = res_manager_load_frame_from_picregion(picreg->res_info, picreg->picreg_data, i);
		if (bitmap == NULL)
		{
			lvgl_res_unload_pictures(img, i - start + 1);
			return -1;
		}

		if (_cvt_bmp2dsc(&img[i-start], bitmap))
		{
			lvgl_res_unload_pictures(img, i - start + 1);
			return -1;
		}

		res_manager_free_resource_structure(bitmap);
	}

	os_strace_end_call_u32(SYS_TRACE_ID_RES_PICS_LOAD, (uint32_t)picreg->picreg_data);
	return 0;
}


int32_t _check_res_path(uint32_t group_id, uint32_t subgroup_id)
{
	int32_t ret;

	if(current_group.id != group_id && current_group.id != 0)
	{
		lvgl_res_unload_group(&current_subgrp);
		memset(&current_subgrp, 0, sizeof(lvgl_res_group_t));
		lvgl_res_unload_group(&current_group);
		memset(&current_group, 0, sizeof(lvgl_res_group_t));
	}

	if(current_subgrp.id != subgroup_id && current_subgrp.id != 0)
	{
		lvgl_res_unload_group(&current_subgrp);
		memset(&current_subgrp, 0, sizeof(lvgl_res_group_t));
	}

	if(current_group.id != group_id && group_id != 0 )
	{
		ret = lvgl_res_load_group_from_scene(&current_scene, group_id, &current_group);
		if(ret < 0)
		{
			memset(&current_group, 0, sizeof(lvgl_res_group_t));
			return -1;
		}
	}

	if(current_subgrp.id != subgroup_id && subgroup_id != 0)
	{
		ret = lvgl_res_load_group_from_group(&current_group, subgroup_id, &current_subgrp);
		if(ret < 0)
		{
			memset(&current_subgrp, 0, sizeof(lvgl_res_group_t));
			return -1;
		}
	}

	return 0;
}

int lvgl_res_load_pictures(uint32_t group_id, uint32_t subgroup_id, uint32_t* id, lv_img_dsc_t* img, lv_point_t* pt, uint32_t num)
{
	int32_t ret;

	if(current_scene.id == 0 || current_scene.scene_data == NULL)
	{
		SYS_LOG_ERR("invalid scene 0x%x, %p", current_scene.id, current_scene.scene_data);
		return -1;
	}

	ret = _check_res_path(group_id, subgroup_id);
	if(ret < 0)
	{
		return -1;
	}

	if(current_subgrp.id != 0)
	{
		ret = lvgl_res_load_pictures_from_group(&current_subgrp, id, img, pt, num);
		if(ret < 0)
		{
			return -1;
		}
	}
	else if(current_group.id != 0)
	{
		ret = lvgl_res_load_pictures_from_group(&current_group, id, img, pt, num);
		if(ret < 0)
		{
			return -1;
		}
	}
	else
	{
		ret = lvgl_res_load_pictures_from_scene(&current_scene, id, img, pt, num);
		if(ret < 0)
		{
			return -1;
		}
	}
	return 0;
}

int lvgl_res_load_strings(uint32_t group_id, uint32_t subgroup_id, uint32_t* id, lvgl_res_string_t* str, uint32_t num)
{
	int32_t ret;

	if(current_scene.id == 0 || current_scene.scene_data == NULL)
	{
		SYS_LOG_ERR("invalid scene 0x%x, %p", current_scene.id, current_scene.scene_data);
		return -1;
	}

	ret = _check_res_path(group_id, subgroup_id);
	if(ret < 0)
	{
		return -1;
	}

	if(current_subgrp.id != 0)
	{
		ret = lvgl_res_load_strings_from_group(&current_subgrp, id, str, num);
		if(ret < 0)
		{
			return -1;
		}
	}
	else if(current_group.id != 0)
	{
		ret = lvgl_res_load_strings_from_group(&current_group, id, str, num);
		if(ret < 0)
		{
			return -1;
		}
	}
	else
	{
		ret = lvgl_res_load_strings_from_scene(&current_scene, id, str, num);
		if(ret < 0)
		{
			return -1;
		}
	}
	return 0;

}

int lvgl_res_load_picregion(uint32_t group_id, uint32_t subgroup_id, uint32_t picreg_id, lvgl_res_picregion_t* res_picreg)
{
	int32_t ret;

	if(current_scene.id == 0 || current_scene.scene_data == NULL)
	{
		SYS_LOG_ERR("invalid scene 0x%x, %p", current_scene.id, current_scene.scene_data);
		return -1;
	}

	ret = _check_res_path(group_id, subgroup_id);
	if(ret < 0)
	{
		return -1;
	}

	if(current_subgrp.id != 0)
	{
		ret = lvgl_res_load_picregion_from_group(&current_subgrp, picreg_id, res_picreg);
		if(ret < 0)
		{
			return -1;
		}
	}
	else if(current_group.id != 0)
	{
		ret = lvgl_res_load_picregion_from_group(&current_group, picreg_id, res_picreg);
		if(ret < 0)
		{
			return -1;
		}
	}
	else
	{
		ret = lvgl_res_load_picregion_from_scene(&current_scene, picreg_id, res_picreg);
		if(ret < 0)
		{
			return -1;
		}
	}
	return 0;

}

static preload_param_t* _check_item_in_list(preload_param_t* sublist, resource_bitmap_t* bitmap)
{
	preload_param_t* item;

	if(sublist == NULL)
	{
		SYS_LOG_ERR("shouldnt happen");
		return NULL;
	}
	else
	{
		item = sublist;
		if(item->bitmap && item->bitmap->sty_data->id == bitmap->sty_data->id)
		{
			return NULL;
		}
		
		while(item->next != NULL)
		{
			if(item->bitmap && item->bitmap->sty_data->id == bitmap->sty_data->id)
			{
				return NULL;
			}
			item = item->next;
		}
		
		if(item->bitmap && item->bitmap->sty_data->id == bitmap->sty_data->id)
		{
			return NULL;
		}	
		//found tail		
		return item;
	}
}

static void _add_item_to_list(preload_param_t** sublist, preload_param_t* param)
{
	preload_param_t* item;

	if(*sublist == NULL)
	{
		*sublist = param;
	}
	else
	{
		item = *sublist;
		while(1)
		{
			if(item->next == NULL)
			{
				item->next = param;
				break;
			}
			item = item->next;
		}
	}
}

static void _add_item_to_preload_list(preload_param_t* param)
{
	preload_param_t* item;
	os_mutex_lock(&preload_mutex, OS_FOREVER);
	if(param_list == NULL)
	{
		param_list = param;
		os_sem_give(&preload_sem);
	}
	else
	{
		item = param_list;
		while(1)
		{
			if(item->next == NULL)
			{
				item->next = param;
				break;
			}
			item = item->next;
		}
	}


	os_mutex_unlock(&preload_mutex);

}

static void _add_item_to_loading_list(preload_param_t* param)
{
	preload_param_t* item;
	if(sync_param_list == NULL)
	{
		sync_param_list = param;
	}
	else
	{
		item = sync_param_list;
		while(1)
		{
			if(item->next == NULL)
			{
				item->next = param;
				break;
			}
			item = item->next;
		}
	}
}

void _clear_preload_list(uint32_t scene_id)
{
	preload_param_t* item;
	preload_param_t* found;
	os_mutex_lock(&preload_mutex, OS_FOREVER);

	if(param_list == NULL)
	{
		os_mutex_unlock(&preload_mutex);
		return;
	}

	item = param_list;
	if(scene_id == 0)
	{
		while(item != NULL)
		{
			param_list = item->next;
			if(item->preload_type == PRELOAD_TYPE_END_CALLBACK)
			{
				if(item->scene_id > 0)
				{
					res_manager_unload_scene(item->scene_id, NULL);
				}			
				if (item->callback)
					item->callback(LVGL_RES_PRELOAD_STATUS_CANCELED, item->param);
			}
			else
			{
				res_manager_free_resource_structure(item->bitmap);
			}
			res_array_free(item);
	//		_debug_sram_usage(0);
			item = param_list;
		}
	}
	else
	{
		while(item != NULL)
		{
			if(item->scene_id == scene_id)
			{
				if(item->preload_type == PRELOAD_TYPE_END_CALLBACK)
				{
					if(item->scene_id > 0)
					{
						res_manager_unload_scene(item->scene_id, NULL);
					}
					if (item->callback)
						item->callback(LVGL_RES_PRELOAD_STATUS_CANCELED, item->param);
				}
				else
				{
					res_manager_free_resource_structure(item->bitmap);
				}				

				if(param_list == item)
				{
					param_list = item->next;
				}
				found = item;
				item = found->next;
				res_array_free(found);
			}
			else
			{
				item = item->next;
			}
		}
	}
	
	os_mutex_unlock(&preload_mutex);
}

void _pause_preload(void)
{
	preload_running = 2;
}

void _resume_preload(void)
{
	preload_running = 1;
}

int lvgl_res_preload_pictures_from_scene(uint32_t scene_id, lvgl_res_scene_t* scene, const uint32_t* id, uint32_t num)
{
	int32_t i;
	preload_param_t* param;
	resource_bitmap_t* bitmap;

	if(scene == NULL || id == NULL || num == 0)
	{
		SYS_LOG_ERR("invalid param to preload %p, %p, %d", scene, id, num);
		return -1;
	}

	os_strace_u32(SYS_TRACE_ID_RES_PICS_PRELOAD, (uint32_t)scene->scene_data);
	for(i=0; i<num; i++)
	{
		bitmap = res_manager_preload_from_scene(scene->res_info, scene->scene_data, id[i]);
		if(bitmap == NULL)
		{
			SYS_LOG_ERR("preload %d bitmap error", id[i]);
			continue;
		}

		param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
		if(param == NULL)
		{
			SYS_LOG_ERR("no space for param for preload ");
			break;
		}
//		_debug_sram_usage(1);
		if(scene_id > 0)
		{
			param->preload_type = PRELOAD_TYPE_NORMAL_COMPACT;
			param->scene_id = scene_id;
		}
		else
		{
			param->preload_type = PRELOAD_TYPE_NORMAL;
		}
		param->bitmap = bitmap;
		param->next = NULL;
		param->res_info = scene->res_info;

		_add_item_to_preload_list(param);
	}

	os_strace_end_call_u32(SYS_TRACE_ID_RES_PICS_PRELOAD, (uint32_t)scene->scene_data);
	return 0;
}

int lvgl_res_preload_pictures_from_group(uint32_t scene_id, lvgl_res_group_t* group, const uint32_t* id, uint32_t num)
{
	int32_t i;
	preload_param_t* param;
	resource_bitmap_t* bitmap;

	if(group == NULL || id == NULL || num == 0)
	{
		SYS_LOG_ERR("invalid param to preload %p, %p, %d", group, id, num);
		return -1;
	}

	os_strace_u32(SYS_TRACE_ID_RES_PICS_PRELOAD, (uint32_t)group->group_data);
	for(i=0; i<num; i++)
	{
		bitmap = res_manager_preload_from_group(group->res_info, group->group_data, scene_id, id[i]);
		if(bitmap == NULL)
		{
			SYS_LOG_ERR("preload %d bitmap error", id[i]);
			continue;
		}

		param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
		if(param == NULL)
		{
			SYS_LOG_ERR("no space for param for preload");
			break;
		}
//		_debug_sram_usage(1);
		if(scene_id > 0)
		{
			param->preload_type = PRELOAD_TYPE_NORMAL_COMPACT;
			param->scene_id = scene_id;
		}
		else
		{
			param->preload_type = PRELOAD_TYPE_NORMAL;
		}
		param->bitmap = bitmap;
		param->next = NULL;
		param->res_info = group->res_info;

		_add_item_to_preload_list(param);
	}
	os_strace_end_call_u32(SYS_TRACE_ID_RES_PICS_PRELOAD, (uint32_t)group->group_data);

	return 0;
}

int lvgl_res_preload_pictures_from_picregion(uint32_t scene_id, lvgl_res_picregion_t* picreg, uint32_t start, uint32_t end)
{
	int32_t i;
	preload_param_t* param;
	resource_bitmap_t* bitmap;


	if(picreg == NULL || start > end)
	{
		SYS_LOG_ERR("invalid param to preload %p, %d, %d", picreg, start, end);
		return -1;
	}

	os_strace_u32(SYS_TRACE_ID_RES_PICS_PRELOAD, (uint32_t)picreg->picreg_data);
	for(i=start; i<=end; i++)
	{
		bitmap = res_manager_preload_from_picregion(picreg->res_info, picreg->picreg_data, i);
		if(bitmap == NULL)
		{
			SYS_LOG_ERR("preload %d bitmap error", i);
			continue;
		}

		param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
		if(param == NULL)
		{
			SYS_LOG_ERR("no space for param for preload");
			break;
		}
//		_debug_sram_usage(1);

		if(scene_id > 0)
		{
			param->preload_type = PRELOAD_TYPE_NORMAL_COMPACT;
			param->scene_id = scene_id;
		}
		else
		{
			param->preload_type = PRELOAD_TYPE_NORMAL;
		}
		param->bitmap = bitmap;
		param->next = NULL;
		param->res_info = picreg->res_info;

		_add_item_to_preload_list(param);
	}

	os_strace_end_call_u32(SYS_TRACE_ID_RES_PICS_PRELOAD, (uint32_t)picreg->picreg_data);
	return 0;

}

static void _res_preload_thread(void *parama1, void *parama2, void *parama3)
{
	preload_param_t* param_item;
	int32_t ret = 0;

	while(preload_running)
	{
		os_mutex_lock(&preload_mutex, OS_FOREVER);
		if(param_list == NULL)
		{
			os_mutex_unlock(&preload_mutex);
			os_sem_take(&preload_sem, OS_FOREVER);
			os_mutex_lock(&preload_mutex, OS_FOREVER);
		}

		if(param_list == NULL)
		{
			os_mutex_unlock(&preload_mutex);
			continue;
		}

		param_item = param_list;
		param_list = param_item->next;
		os_mutex_unlock(&preload_mutex);

		if(preload_running == 2)
		{
//			k_sleep(K_MSEC(50));
			continue;
		}

		if(param_item->preload_type == PRELOAD_TYPE_NORMAL)
		{
			ret = res_manager_preload_bitmap(param_item->res_info, param_item->bitmap);				
			res_manager_free_resource_structure(param_item->bitmap);
		}
		else if(param_item->preload_type == PRELOAD_TYPE_NORMAL_COMPACT)
		{
			ret = res_manager_preload_bitmap_compact(param_item->scene_id, param_item->res_info, param_item->bitmap);	
			res_manager_free_resource_structure(param_item->bitmap);
		}
		else if(param_item->preload_type == PRELOAD_TYPE_BEGIN_CALLBACK)
		{
			if (param_item->callback)
				param_item->callback(LVGL_RES_PRELOAD_STATUS_LOADING, param_item->param);

		}
		else if(param_item->preload_type == PRELOAD_TYPE_END_CALLBACK)
		{
			//user callback ,add at the end of preloaded pics to inform user about preload finish
			res_manager_preload_finish_check(param_item->scene_id);

			if (param_item->callback)
				param_item->callback(LVGL_RES_PRELOAD_STATUS_FINISHED, param_item->param);
//			_dump_sram_usage();
		}
		else
		{
			SYS_LOG_ERR("unknown preload type: %d\n", param_item->preload_type);
//			ret = res_manager_load_bitmap(param_item->res_info, param_item->bitmap);

			continue;
		}

		res_array_free(param_item);
//		_debug_sram_usage(0);
//		k_sleep(K_MSEC(5));
	}

}


void _dump_preload_list(void)
{
	preload_param_t* item = param_list;

	while(item)
	{
		printf("preload item scene 0x%x, type %d\n", item->scene_id, item->preload_type);
		item=item->next;
	}
}

int lvgl_res_preload_cancel(void)
{
	_clear_preload_list(0);
	return 0;
}

int lvgl_res_preload_cancel_scene(uint32_t scene_id)
{
	_clear_preload_list(scene_id);

//	_dump_preload_list();
	return 0;
}

int _res_preload_pictures_from_picregion(uint32_t scene_id, lvgl_res_picregion_t* picreg, uint32_t start, uint32_t end, preload_param_t** sublist)
{
	int32_t i;
	preload_param_t* param;
	preload_param_t* tail;
	resource_bitmap_t* bitmap;
	int preload_count = 0;

	if(picreg == NULL || start > end)
	{
		SYS_LOG_ERR("invalid param to preload %p, %d, %d", picreg, start, end);
		return -1;
	}

	os_strace_u32(SYS_TRACE_ID_RES_PICS_PRELOAD, (uint32_t)picreg->picreg_data);
	for(i=start; i<=end; i++)
	{
		bitmap = res_manager_preload_from_picregion(picreg->res_info, picreg->picreg_data, i);
		if(bitmap == NULL)
		{
			SYS_LOG_ERR("preload %d bitmap error", i);
			continue;
		}
		tail = _check_item_in_list(*sublist, bitmap);
		if(tail == NULL)
		{
			//already in sublist, FIXME:only checked sublist for performance
			res_manager_release_resource(bitmap);
			continue;
		}

		param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
		if(param == NULL)
		{
			SYS_LOG_ERR("no space for param for preload");
			break;
		}
//		_debug_sram_usage(1);

		if(scene_id > 0)
		{
			param->preload_type = PRELOAD_TYPE_NORMAL_COMPACT;
			param->scene_id = scene_id;
		}
		else
		{
			param->preload_type = PRELOAD_TYPE_NORMAL;
		}
		param->bitmap = bitmap;
		param->next = NULL;
		param->res_info = picreg->res_info;

//		_add_item_to_list(sublist, param);
		tail->next = param;	
		preload_count++;
	}

	os_strace_end_call_u32(SYS_TRACE_ID_RES_PICS_PRELOAD, (uint32_t)picreg->picreg_data);
	return preload_count;

}



int _res_preload_group_compact(resource_info_t* info, uint32_t scene_id, uint32_t pargroup_id, resource_group_t* group, uint32_t* ptotal_size, uint32_t preload, preload_param_t** sublist)
{
	preload_param_t* param;
	preload_param_t* tail;
	resource_group_t* res_group;
	resource_bitmap_t* bitmap;
	lvgl_res_picregion_t picreg;
	uint32_t count = 0;
	uint32_t offset = 0;
	uint32_t total_size = *ptotal_size;	
	uint32_t buf_block_struct_size = res_manager_get_bitmap_buf_block_unit_size();
	int picreg_count = 0;

	if(info == NULL)
	{
		SYS_LOG_ERR("resources not opened\n");
		return -1;
	}

	while(1)
	{
		void* resource = res_manager_preload_next_group_child(info, group, &count, &offset, scene_id, pargroup_id);
		if(resource == NULL)
		{
			if(count >= group->sty_data->resource_sum)
			{
				//reach end of scene
				break;
			}

			//text or invalid param which shouldnt happen here
			continue;
		}
		
		uint32_t type = *((uint32_t*)(*((uint32_t*)(resource))));
//		SYS_LOG_INF("group 0x%x, resource type %d, count %d\n",group->sty_data->sty_id, type, count);

		switch(type)
		{
		case RESOURCE_TYPE_GROUP:
			res_group = (resource_group_t*)resource;
			if(res_group->sty_data->resource_sum > 0)
			{
				if(pargroup_id > 0)
				{
					//regular check only support 2 level depth, dont check regular after that
					_res_preload_group_compact(info, scene_id, 0xffffffff, (resource_group_t*)resource, &total_size, preload, sublist);
				}
				else
				{
					_res_preload_group_compact(info, scene_id, group->sty_data->sty_id, (resource_group_t*)resource, &total_size, preload, sublist);
				}
			}
			res_manager_release_resource(resource);
			break;
		case RESOURCE_TYPE_PICREGION:
			picreg.picreg_data = (resource_picregion_t*)resource;
			picreg.res_info = info;
			if(preload)
			{
				picreg_count = _res_preload_pictures_from_picregion(scene_id, &picreg, 0, picreg.picreg_data->sty_data->frames-1, sublist);
			}
			
			if(picreg_count > 0 && picreg.picreg_data->regular_info == 0)
			{
				total_size += (picreg.picreg_data->sty_data->width*picreg.picreg_data->sty_data->height*picreg.picreg_data->sty_data->bytes_per_pixel + buf_block_struct_size)*picreg_count;			
			}

			res_manager_release_resource(resource);
			break;
		case RESOURCE_TYPE_PICTURE:
			bitmap = (resource_bitmap_t*)resource;
			tail = _check_item_in_list(*sublist, bitmap);
			if(tail == NULL)
			{
				//already in sublist, FIXME:only checked sublist for performance
				res_manager_release_resource(resource);
				continue;
			}
			if(bitmap->regular_info == 0)
			{
				if(bitmap->sty_data->width == screen_width  && bitmap->sty_data->height == screen_height && bitmap->sty_data->bytes_per_pixel == 2)
				{
					total_size += buf_block_struct_size;
				}
				else
				{
					total_size += (bitmap->sty_data->width*bitmap->sty_data->height*bitmap->sty_data->bytes_per_pixel + buf_block_struct_size);
				}
			}
			if(preload)
			{
				param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
				if(param == NULL)
				{
					SYS_LOG_ERR("no space for param for preload");
					break;
				}
//				_debug_sram_usage(1);
				if(scene_id == 0)
				{
					param->preload_type = PRELOAD_TYPE_NORMAL;
				}
				else
				{
					param->preload_type = PRELOAD_TYPE_NORMAL_COMPACT;
					param->scene_id = scene_id;
				}
				param->bitmap = bitmap;
				param->next = NULL;
				param->res_info = info;
//				SYS_LOG_INF("add bitmap bitmap %p, 0x%x, format 0x%x\n", bitmap, bitmap->id, bitmap->format);
				_add_item_to_list(sublist, param);
			}
			break;
		default:
			//ignore text resource
			break;
		}
	}

	*ptotal_size = total_size;
	return 0;
}

int _res_preload_scene_compact(uint32_t scene_id, const uint32_t* resource_id, uint32_t resource_num, void (*callback)(int32_t, void *), void* user_data,
											const char* style_path, const char* picture_path, const char* text_path, bool async_preload)
{
	resource_info_t* info;
	preload_param_t* param;
	preload_param_t* sublist;
	preload_param_t* tail;
	resource_bitmap_t* bitmap;
	resource_scene_t* res_scene;
	resource_group_t* res_group;
	void* resource;
	lvgl_res_picregion_t picreg;
	uint32_t count = 0;
	uint32_t offset = 0;
	uint32_t total_size = 0;
	uint32_t buf_block_struct_size = res_manager_get_bitmap_buf_block_unit_size();
	int picreg_count = 0;

	info = _res_file_open(style_path, picture_path, text_path, 0);
	if(info == NULL)
	{
		SYS_LOG_ERR("resource info error");
		return-1;
	}

	os_strace_u32(SYS_TRACE_ID_RES_SCENE_PRELOAD_1, (uint32_t)scene_id);	
	res_scene = res_manager_load_scene(info, scene_id);
	if(res_scene == NULL)
	{
		return -1;
	}
	os_strace_end_call_u32(SYS_TRACE_ID_RES_SCENE_PRELOAD_1, (uint32_t)scene_id);
	os_strace_u32(SYS_TRACE_ID_RES_SCENE_PRELOAD_3, (uint32_t)scene_id);	
	sublist = NULL;
	param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
	if(param == NULL)
	{
		SYS_LOG_ERR("no space for param for preload");
		return -1;
	}
//	_debug_sram_usage(1);
	memset(param, 0, sizeof(preload_param_t));
	param->callback = callback;
	param->param = user_data;
	param->scene_id = scene_id;
	param->preload_type = PRELOAD_TYPE_BEGIN_CALLBACK;
	param->next = NULL;
	param->res_info = NULL;
	_add_item_to_list(&sublist, param);

	//preload group, be sure that scene compact buffer is inited already
	if(resource_id != NULL)
	{
		uint32_t i;
//		printf("resource_sum %d\n", resource_num);
		for(i=0;i<resource_num;i++)
		{
			resource = (void*)res_manager_preload_from_scene(info, res_scene, resource_id[i]);
			if(resource == NULL)
			{
				//somehow not found, just ignore it
				continue;
			}
			uint32_t type = *((uint32_t*)(*((uint32_t*)(resource))));
			switch(type)
			{
			case RESOURCE_TYPE_GROUP:
				res_group = (resource_group_t*)resource;
				if(res_group->sty_data->resource_sum > 0)
				{
					_res_preload_group_compact(info, scene_id, 0, (resource_group_t*)resource, &total_size, 1, &sublist);
				}
				res_manager_release_resource(resource);
				break;
			case RESOURCE_TYPE_PICREGION:
				picreg.picreg_data = (resource_picregion_t*)resource;
				picreg.res_info = info;
				picreg_count = _res_preload_pictures_from_picregion(scene_id, &picreg, 0, picreg.picreg_data->sty_data->frames-1, &sublist);
				if(picreg_count > 0 && picreg.picreg_data->regular_info == 0)
				{
					total_size += (picreg.picreg_data->sty_data->width*picreg.picreg_data->sty_data->height*picreg.picreg_data->sty_data->bytes_per_pixel + buf_block_struct_size)*picreg_count;
				}
				res_manager_release_resource(resource);
				break;
			case RESOURCE_TYPE_PICTURE:
				bitmap = (resource_bitmap_t*)resource;
				tail = _check_item_in_list(sublist, bitmap);
				if(tail == NULL)
				{
					//already in sublist, FIXME:only checked sublist for performance
					res_manager_release_resource(resource);
					continue;
				}				
				if(bitmap->regular_info == 0)
				{
					if(bitmap->sty_data->width == screen_width	&& bitmap->sty_data->height == screen_height && bitmap->sty_data->bytes_per_pixel == 2)
					{
						total_size += buf_block_struct_size;
					}
					else
					{
						total_size += (bitmap->sty_data->width*bitmap->sty_data->height*bitmap->sty_data->bytes_per_pixel + buf_block_struct_size);				
					}
				}

				param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
				if(param == NULL)
				{
					SYS_LOG_ERR("no space for param for preload");
					break;
				}
//				_debug_sram_usage(1);
				param->preload_type = PRELOAD_TYPE_NORMAL_COMPACT;
				param->bitmap = bitmap;
				param->scene_id = scene_id;
				param->next = NULL;
				param->res_info = info;
//				SYS_LOG_INF("add bitmap bitmap %p, %d\n", bitmap, bitmap->id);
//				_add_item_to_list(&sublist, param);
				tail->next = param;
				break;
			default:
				break;
			}
		}

		param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
		if(param == NULL)
		{
			SYS_LOG_ERR("no space for param for preload");
			return -1;
		}
//		_debug_sram_usage(1);
		memset(param, 0, sizeof(preload_param_t));
		param->callback = callback;
		param->param = user_data;
		param->scene_id = scene_id;
		param->preload_type = PRELOAD_TYPE_END_CALLBACK;
		param->next = NULL;

		_add_item_to_list(&sublist, param);

		if(total_size > 0)
		{
			res_manager_init_compact_buffer(scene_id, total_size);	
		}

		if(async_preload)
		{
			_add_item_to_preload_list(sublist);		
		}
		else
		{
			_add_item_to_loading_list(sublist);
		}
		
//		_dump_sram_usage();
		os_strace_end_call_u32(SYS_TRACE_ID_RES_SCENE_PRELOAD_3, (uint32_t)scene_id);

		SYS_LOG_INF("\n #### scene res 0x%x, total_size %d\n", scene_id, total_size);
		return 0;
	}

	//calculate scene bitmap buffer
//	_pause_preload();
	//fill preload list
	while(1)
	{
		void* resource = res_manager_preload_next_scene_child(info, res_scene, &count, &offset);
		if(resource == NULL)
		{
			if(count >= res_scene->resource_sum)
			{
				//reach end of scene
				break;
			}

			//text or invalid param which shouldnt happen here
			continue;
		}

		uint32_t type = *((uint32_t*)(*((uint32_t*)(resource))));
//		SYS_LOG_INF("scene 0x%x, resource type %d, count %d\n",scene_id, type, count);
		switch(type)
		{
		case RESOURCE_TYPE_GROUP:
			res_group = (resource_group_t*)resource;
			if(res_group->sty_data->resource_sum > 0)
			{
				_res_preload_group_compact(info, scene_id, 0, (resource_group_t*)resource, &total_size, 1, &sublist);
			}
			res_manager_release_resource(resource);
			break;
		case RESOURCE_TYPE_PICREGION:
			picreg.picreg_data = (resource_picregion_t*)resource;
			picreg.res_info = info;
			picreg_count = _res_preload_pictures_from_picregion(scene_id, &picreg, 0, picreg.picreg_data->sty_data->frames-1, &sublist);
			if(picreg_count > 0 && picreg.picreg_data->regular_info == 0)
			{
				total_size += (picreg.picreg_data->sty_data->width*picreg.picreg_data->sty_data->height*picreg.picreg_data->sty_data->bytes_per_pixel + buf_block_struct_size)*picreg_count;			
			}
//			SYS_LOG_INF("picreg format 0x%x\n", picreg.picreg_data->format);
			res_manager_release_resource(resource);
			break;
		case RESOURCE_TYPE_PICTURE:
			bitmap = (resource_bitmap_t*)resource;
			tail = _check_item_in_list(sublist, bitmap);
			if(tail == NULL)
			{
				//already in sublist, FIXME:only checked sublist for performance
				res_manager_release_resource(resource);
				continue;
			}			
			if(bitmap->regular_info == 0)
			{
				if(bitmap->sty_data->width == screen_width  && bitmap->sty_data->height == screen_height && bitmap->sty_data->bytes_per_pixel == 2)
				{
					total_size += buf_block_struct_size;
				}
				else
				{
					total_size += (bitmap->sty_data->width*bitmap->sty_data->height*bitmap->sty_data->bytes_per_pixel + buf_block_struct_size); 			
				}
			}

			param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
			if(param == NULL)
			{
				SYS_LOG_ERR("no space for param for preload");
				break;
			}
//			_debug_sram_usage(1);

			param->preload_type = PRELOAD_TYPE_NORMAL_COMPACT;
			param->bitmap = bitmap;
			param->scene_id = scene_id;
			param->next = NULL;
			param->res_info = info;
//			SYS_LOG_INF("add bitmap styid 0x%x, id %d, format %d\n", bitmap->sty_data->sty_id, bitmap->sty_data->id, bitmap->sty_data->format);
			tail->next = param;
			break;
		default:
			//ignore text resource
			break;
		}
	}

	param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
	if(param == NULL)
	{
		SYS_LOG_ERR("no space for param for preload");
		return -1;
	}
//	_debug_sram_usage(1);
	memset(param, 0, sizeof(preload_param_t));
	param->callback = callback;
	param->param = user_data;
	param->scene_id = scene_id;
	param->preload_type = PRELOAD_TYPE_END_CALLBACK;
	param->next = NULL;
	_add_item_to_list(&sublist, param);

	if(total_size > 0)
	{
		res_manager_init_compact_buffer(scene_id, total_size);
	}
	if(async_preload)
	{
		_add_item_to_preload_list(sublist); 	
	}
	else
	{
		_add_item_to_loading_list(sublist);
	}
//	_dump_sram_usage();
	os_strace_end_call_u32(SYS_TRACE_ID_RES_SCENE_PRELOAD_3, (uint32_t)scene_id);
	SYS_LOG_INF("\n ###  scene 0x%x, total_size %d\n", scene_id, total_size);
	return 0;
}

int lvgl_res_preload_scene_compact(uint32_t scene_id, const uint32_t* resource_id, uint32_t resource_num, void (*callback)(int32_t, void *), void* user_data,
											const char* style_path, const char* picture_path, const char* text_path)
{
	int ret;
	ret = _res_preload_scene_compact(scene_id, resource_id, resource_num, callback, user_data, style_path, picture_path, text_path, 1);
	if(ret < 0)
	{
		SYS_LOG_ERR("preload failed 0x%x\n", scene_id);
		return -1;
	}

	return 0;
}

int lvgl_res_preload_scene_compact_serial(uint32_t scene_id, const uint32_t* resource_id, uint32_t resource_num, void (*callback)(int32_t, void *), void* user_data,
											const char* style_path, const char* picture_path, const char* text_path)
{
	int ret;
	preload_param_t* param_item;
	ret = _res_preload_scene_compact(scene_id, resource_id, resource_num, callback, user_data, style_path, picture_path, text_path, 0);
	if(ret < 0)
	{
		SYS_LOG_ERR("preload failed 0x%x\n", scene_id);
		return -1;
	}

	if(sync_param_list == NULL)
	{
		SYS_LOG_ERR("invalid loading list\n");
		return -1;
	}
	
	while(sync_param_list)
	{
		param_item = sync_param_list;
		sync_param_list = param_item->next;
		
		if(param_item->preload_type == PRELOAD_TYPE_NORMAL)
		{
			ret = res_manager_preload_bitmap(param_item->res_info, param_item->bitmap);			
			res_manager_free_resource_structure(param_item->bitmap);
		}
		else if(param_item->preload_type == PRELOAD_TYPE_NORMAL_COMPACT)
		{
			ret = res_manager_preload_bitmap_compact(param_item->scene_id, param_item->res_info, param_item->bitmap);
			res_manager_free_resource_structure(param_item->bitmap);	
		}
		else if(param_item->preload_type == PRELOAD_TYPE_BEGIN_CALLBACK)
		{
			//FIXME
		}
		else if(param_item->preload_type == PRELOAD_TYPE_END_CALLBACK)
		{
			//FIXME
		}
		else
		{
			SYS_LOG_ERR("unknown preload type: %d\n", param_item->preload_type);
			res_array_free(param_item);
			continue;
		}

		res_array_free(param_item);
	}	
	return 0;

}

void lvgl_res_scene_preload_default_cb_for_view(int32_t status, void *view_id)
{
	SYS_LOG_INF("preload view %u, status %d\n", (uint32_t)view_id, status);

	if (status == LVGL_RES_PRELOAD_STATUS_LOADING) {
		os_strace_u32(SYS_TRACE_ID_VIEW_PRELOAD, (uint32_t)view_id);
	} else {
		os_strace_end_call_u32(SYS_TRACE_ID_VIEW_PRELOAD, (uint32_t)view_id);
	}

#ifdef CONFIG_UI_MANAGER
	if (status == LVGL_RES_PRELOAD_STATUS_FINISHED) {
		ui_view_layout((uint32_t)view_id);
	}
#endif
}

void lvgl_res_scene_preload_default_cb_for_update(int32_t status, void *view_id)
{
	if (status == LVGL_RES_PRELOAD_STATUS_LOADING) {
		os_strace_u32(SYS_TRACE_ID_VIEW_UPDATE, (uint32_t)view_id);
	} else {
		os_strace_end_call_u32(SYS_TRACE_ID_VIEW_UPDATE, (uint32_t)view_id);
	}

#ifdef CONFIG_UI_MANAGER
	if (status == LVGL_RES_PRELOAD_STATUS_FINISHED) {
		ui_view_update((uint32_t)view_id);
	}
#endif	
}





int lvgl_res_preload_scene_compact_default_init(uint32_t scene_id, const uint32_t* resource_id, uint32_t resource_num, void (*callback)(int32_t, void *),
											const char* style_path, const char* picture_path, const char* text_path)
{
	preload_default_t* record;
	preload_default_t* item;
	int32_t i;

	record = preload_default_list;
	while(record != NULL)
	{
		if(record->scene_id == scene_id)
		{
			return 0;
		}
		record = record->next;
	}

	item = res_mem_alloc(RES_MEM_POOL_BMP ,sizeof(preload_default_t)+resource_num*sizeof(uint32_t));
	if(item == NULL)
	{
		SYS_LOG_ERR("alloc preload defautl data failed 0x%x\n", scene_id);
		return -1;
	}
	item->scene_id = scene_id;
	if(resource_id)
	{
		item->resource_id = (uint32_t*)((uint32_t)item+sizeof(preload_default_t));
	}
	else
	{
		item->resource_id = NULL;
	}
	item->resource_num = resource_num;
	item->callback = callback;
	//FIXME: hopefully these macros are constants
	item->style_path = (char*)style_path;
	item->picture_path = (char*)picture_path;
	item->text_path = (char*)text_path;
	item->next = NULL;
	
	for(i=0;i<resource_num;i++)
	{
		item->resource_id[i] = resource_id[i];
	}

	if(preload_default_list == NULL)
	{
		preload_default_list = item;
		return 0;
	}

	record = preload_default_list;
	while(record != NULL)
	{
		if(record->next == NULL)
		{
			record->next = item;
			return 0;
		} 
		else
		{
			record = record->next;
		}
	}

	return 0;
}

int lvgl_res_scene_is_loaded(uint32_t scene_id)
{
	return res_manager_scene_is_loaded(scene_id);
}

int lvgl_res_preload_scene_compact_default(uint32_t scene_id, uint32_t view_id, bool reload_update, bool load_serial)
{
	int ret = 0;
	preload_default_t* record = preload_default_list;

	while(record != NULL)
	{
		if(record->scene_id == scene_id)
		{
			if(reload_update)
			{
				view_set_refresh_en(view_id, 0);
				if(record->callback == NULL)
				{
					ret = lvgl_res_preload_scene_compact(scene_id, record->resource_id, record->resource_num, 
									lvgl_res_scene_preload_default_cb_for_update, (void*)view_id, 
									record->style_path, record->picture_path, record->text_path);
				}
				else
				{
					ret = lvgl_res_preload_scene_compact(scene_id, record->resource_id, record->resource_num, 
									record->callback, (void*)view_id, 
									record->style_path, record->picture_path, record->text_path);
				}
			}
			else if(!load_serial)
			{
				if(record->callback == NULL)
				{
					ret = lvgl_res_preload_scene_compact(scene_id, record->resource_id, record->resource_num, 
									lvgl_res_scene_preload_default_cb_for_view, (void*)view_id, 
									record->style_path, record->picture_path, record->text_path);
				}
				else
				{
					ret = lvgl_res_preload_scene_compact(scene_id, record->resource_id, record->resource_num, 
									record->callback, (void*)view_id, 
									record->style_path, record->picture_path, record->text_path);
				}
			}
			else
			{
				if(record->callback == NULL)
				{
					ret = lvgl_res_preload_scene_compact_serial(scene_id, record->resource_id, record->resource_num, 
									lvgl_res_scene_preload_default_cb_for_view, (void*)view_id, 
									record->style_path, record->picture_path, record->text_path);
				}
				else
				{
					ret = lvgl_res_preload_scene_compact_serial(scene_id, record->resource_id, record->resource_num, 
									record->callback, (void*)view_id, 
									record->style_path, record->picture_path, record->text_path);
				}
			}
		}
		record = record->next;
	}

	return ret;
}

