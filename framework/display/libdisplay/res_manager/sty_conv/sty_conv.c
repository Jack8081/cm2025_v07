#include <string.h>
#include <Stdio.h>
#include <stdlib.h>
#include "res_struct.h"

#pragma pack (1)

#define SCENE_ITEM_SIZE 3

#define PACK

#define MAX_RES_DATA_SIZE	1024*1024
#define MAX_DIFF_PATH_LEN	256


typedef struct _style_head
{
    uint32_t    magic;        /*0x18*/
    uint32_t    scene_sum;
    int8_t      reserve[8];
}style_head_t;

typedef struct _scene
{

    short  x;

    short  y;

    short  width;

    short  height;

    unsigned int background;     

    unsigned char  visible;

    unsigned char   opaque;

    unsigned short  transparence;
    
    unsigned int resource_sum;

    unsigned int child_offset;

    unsigned char direction;
    
    unsigned char keys[ 16 ];
}scene_t;

typedef struct resource_s
{

    uint8_t type;

    uint32_t id;

    uint32_t offset;
}resource_t;


typedef struct _resgroup_resource
{

    resource_t   	 resource;

    int16_t  absolute_x;

    int16_t  absolute_y;

    int16_t  x;

    int16_t  y;

    int16_t  width;

    int16_t  height;

    uint32_t	 background;

    uint8_t  visible;

    uint8_t   opaque;

    uint16_t  transparence;

    uint32_t  	 resource_sum;

    uint32_t child_offset;
}resgroup_resource_t;

typedef struct _string_resource
{

    resource_t   	 resource;

    int16_t  x;

    int16_t  y;

    int16_t  width;

    int16_t  height;

    uint32_t  	 foreground;

    uint32_t	 background;

    uint8_t   visible;

    uint8_t   text_align;

    uint8_t   mode;

    uint8_t   font_height;

    uint16_t str_id;

    uint8_t scroll;

    int8_t direction;

    uint16_t space;

    uint16_t pixel;
}string_resource_t;


typedef struct _picture_resource
{
    resource_t   	 resource;
    int16_t  x;
    int16_t  y;
    int16_t  width;
    int16_t  height;
    uint8_t  visible;  //0x10=rgb ?¨º? 0x11=argb
    uint16_t pic_id;
}picture_resource_t;

typedef struct _picregion_resource
{

    resource_t   	 resource;

    int16_t  x;

    int16_t  y;

    int16_t  width;

    int16_t  height;

    uint8_t   visible;

    uint16_t 	 frames;

    uint32_t pic_offset;
}picregion_resource_t;

typedef struct _picregion_frame
{

    uint32_t id;

    uint16_t  frame;

    int16_t  x;

    int16_t  y;

    int16_t  width;

    int16_t  height;

    uint16_t  pic_id;
}picregion_frame_t;

typedef struct
{
    uint8_t  magic[4];        //'R', 'E', 'S', 0x19
    uint16_t counts;
    uint16_t w_format;
    uint8_t brgb;
    uint8_t version;
	uint16_t id_start;
    uint8_t reserved[3];
    uint8_t ch_extend;
}res_head_t;

typedef struct
{
    uint32_t   offset;
    uint16_t   length;
    uint8_t    type;
    uint8_t    name[8];
    uint8_t    len_ext;
}res_entry_t;


FILE* picfp = NULL;
FILE* picfp_diff = NULL;
uint32_t basic_id_end = 0;
uint32_t diff_id_start = 0;

static int _read_head(char* sty_data, resource_info_t* info)
{
    style_head_t* head;
    int ret;
	uint32_t sty_size;

	head = (style_head_t*)sty_data;
	info->sum = head->scene_sum;
    //FIXME
	info->scenes = (int32_t*)(sty_data + sizeof(style_head_t));

    return 0;
}

static int _parse_bitmap(char* sty_data, sty_picture_t* pic)
{
	int ret;
	res_entry_t res_entry;

	memset(&res_entry, 0, sizeof(res_entry_t));
	if(pic->id <= basic_id_end)
	{
		fseek(picfp, pic->id * sizeof(res_entry_t) , SEEK_SET);
		ret = fread(&res_entry, 1, sizeof(res_entry_t), picfp);
	}
	else
	{
		if(picfp_diff)
		{
			fseek(picfp_diff, (pic->id - diff_id_start + 1)*sizeof(res_entry_t), SEEK_SET);
			ret = fread(&res_entry, 1, sizeof(res_entry_t), picfp_diff);
		}
	}
	if (ret < sizeof(res_entry_t))
	{
		printf("cant find sty_id 0x%x\n", pic->sty_id);
		return -1;
	}

	switch (res_entry.type)
	{
	case RES_TYPE_PNG:
		pic->bytes_per_pixel = 3;
		pic->format = RESOURCE_BITMAP_FORMAT_ARGB8565;
		break;
	case RES_TYPE_PIC:
	case RES_TYPE_LOGO:
	case RES_TYPE_PIC_LZ4:
		pic->bytes_per_pixel = 2;
		pic->format = RESOURCE_BITMAP_FORMAT_RGB565;
		break;
	case RES_TYPE_PIC_RGB888:
		pic->bytes_per_pixel = 3;
		pic->format = RESOURCE_BITMAP_FORMAT_RGB888;
		break;
	case RES_TYPE_PIC_ARGB8888:
	case RES_TYPE_PIC_ARGB8888_LZ4:
		pic->bytes_per_pixel = 4;
		pic->format = RESOURCE_BITMAP_FORMAT_ARGB8888;
		break;
	case RES_TYPE_PNG_COMPRESSED:
	case RES_TYPE_PIC_COMPRESSED:
	case RES_TYPE_LOGO_COMPRESSED:
	case RES_TYPE_PIC_RGB888_COMPRESSED:
	case RES_TYPE_PIC_ARGB8888_COMPRESSED:
	default:
		printf("not supported sty_id 0x%x, id %d\n", pic->sty_id, pic->id);
		return -1;
	}

    if((res_entry.type == RES_TYPE_PIC_LZ4) || (res_entry.type == RES_TYPE_PIC_ARGB8888_LZ4))
    {
		pic->compress_size = ((res_entry.len_ext << 16) | res_entry.length) - 4;
    }	
    else
    {
    	pic->compress_size = 0;
    }
	pic->bmp_pos = res_entry.offset+4;
//	printf("pic 0x%x, pos %d, bpp %d\n", pic->sty_id, pic->bmp_pos, pic->bytes_per_pixel);
	return 0;
}

static int _parse_group(char* sty_data, int* pgroup_offset, resgroup_resource_t* group, unsigned char* res_data, int* pres_offset, uint32_t magic)
{
	int res_offset = *pres_offset;
	int group_offset = *pgroup_offset;
	unsigned char* buf;
	resource_t* resource;
	int count = 0;
	int i, j;
	int group_size;
	int bpp = 0;

	group_offset += group->child_offset;
	buf = (uint8_t*)sty_data + group_offset;
	resource = (resource_t*)buf;
	printf("group res sum %d\n", group->resource_sum);
	for(i=0;i<group->resource_sum;i++)
	{
	    if(resource->type == RESOURCE_TYPE_GROUP)
	    {
			//buf is now on group resource start
	    	sty_group_t* res = (sty_group_t*)(res_data + res_offset);
			resgroup_resource_t* group = (resgroup_resource_t*)resource;
			res->type = (uint32_t)resource->type;
			res->sty_id = (uint32_t)resource->id;
			res->x = (int16_t)group->x;
			res->y = (int16_t)group->y;
			res->width = (uint16_t)group->width;
			res->height = (uint16_t)group->height;
			res->backgroud = (uint32_t)group->background;
			res->opaque = (uint32_t)group->transparence;
			res->resource_sum = (uint32_t)group->resource_sum;
			
			res_offset += sizeof(sty_group_t);
			res->child_offset = (uint32_t)res_offset;

			magic += res->sty_id;
			printf("%d magic 0x%x\n", __LINE__, magic);
			_parse_group(sty_data, &group_offset, group, res_data, &res_offset, magic);
			res->offset = res_offset;
			magic -= res->sty_id;
	    }

	    if(resource->type == RESOURCE_TYPE_PICTURE)
	    {
	    	//buf is now on resource start
	    	sty_picture_t* res = (sty_picture_t*)(res_data + res_offset);
			picture_resource_t* pic = (picture_resource_t*)resource;
			res->type = (uint32_t)resource->type;
			res->sty_id = resource->id;
			res->x = (int16_t)pic->x;
			res->y = (int16_t)pic->y;
			res->width = (uint16_t)pic->width;
			res->height = (uint16_t)pic->height;
			res->format = (uint16_t)pic->visible;
			res->id = (uint32_t)pic->pic_id;
			res->magic = magic + res->sty_id;
			printf("0x%x, 0x%x\n", magic, res->sty_id);
			printf("%d magic 0x%x\n", __LINE__, res->magic);
			res_offset += sizeof(sty_picture_t);
			res->offset = res_offset;
			_parse_bitmap(sty_data, res);
	    }

	    if(resource->type == RESOURCE_TYPE_TEXT)
	    {
	    	//buf is now on resource start
	    	sty_string_t* res = (sty_string_t*)(res_data + res_offset);
			string_resource_t* str = (string_resource_t*)resource;
			res->type = (uint32_t)resource->type;
			res->sty_id = resource->id;
			res->x = (int16_t)str->x;
			res->y = (int16_t)str->y;
			res->width = (uint16_t)str->width;
			res->height = (uint16_t)str->height;
			res->color = (uint32_t)str->foreground;
			res->bgcolor = (uint32_t)str->background;
			res->align = (uint16_t)str->text_align;
			res->font_size = (uint16_t)str->font_height;
			res->id = (uint32_t)str->str_id;

			res_offset += sizeof(sty_string_t);
			res->offset = res_offset;
	    }

		if(resource->type == RESOURCE_TYPE_PICREGION)
		{
			sty_picregion_t* res = (sty_picregion_t*)(res_data + res_offset);
			picregion_resource_t* picreg = (picregion_resource_t*)resource;
			res->type = (uint32_t)resource->type;
			res->sty_id = resource->id;
			res->x = (int16_t)picreg->x;
			res->y = (int16_t)picreg->y;
			res->width = (uint16_t)picreg->width;
			res->height = (uint16_t)picreg->height;
			res->format = (uint16_t)picreg->visible;
			res->frames = (uint32_t)picreg->frames;

			res_offset += sizeof(sty_picregion_t);
			res->id_offset = res_offset;
			uint8_t* tmp = (res_data + res_offset);
			sty_picture_t* bitmap = (sty_picture_t*)tmp;
			buf = (char*)resource + picreg->pic_offset;
			printf("11111\n");
			for(j = 0; j < res->frames; j++)
			{
				bitmap->type = RESOURCE_TYPE_PICTURE;
				bitmap->sty_id = res->sty_id;
				res_offset += sizeof(sty_picture_t);
				bitmap->offset = res_offset;
				bitmap->x = res->x;
				bitmap->y = res->y;
				bitmap->width = res->width;
				bitmap->height = res->height;
				bitmap->id = (uint32_t)(buf[1]<<8 | buf[0]);
				bitmap->magic = magic + res->sty_id;
				_parse_bitmap(sty_data, bitmap);
				if(bpp == 0)
				{
					bpp = bitmap->bytes_per_pixel;
				}
				printf("w %d h%d\n", bitmap->width, bitmap->height);
				buf += 2;
				bitmap = (sty_picture_t*)(res_data + res_offset);
			}
			res->bytes_per_pixel = bpp;
			printf("22222\n");
			res->offset = res_offset;
			res->magic = magic + res->sty_id;
			printf("%d magic 0x%x\n", __LINE__, res->magic);

		}	

		group_offset += resource->offset;
		buf = (uint8_t*)sty_data + group_offset;
		resource = (resource_t*)buf;
	}

	*pres_offset = res_offset;
	return 0;	

}

static int _parse_scene(char* sty_data, int scene_offset, scene_t* scene, unsigned char* res_data, int* pres_offset, uint32_t magic)
{
	int res_offset = *pres_offset;
	unsigned char* buf;
	resource_t* resource;
	int count = 0;
	int i, j;
	int group_size;
	int buf_offset;
	int bpp = 0;

	buf_offset = scene_offset + scene->child_offset;
	buf = (unsigned char*)sty_data + buf_offset;
	resource = (resource_t*)buf;
	printf("scene res sum %d\n", scene->resource_sum);
	for(i=0;i<scene->resource_sum;i++)
	{
	    if(resource->type == RESOURCE_TYPE_GROUP)
	    {
			//buf is now on group resource start
	    	sty_group_t* res = (sty_group_t*)(res_data + res_offset);
			resgroup_resource_t* group = (resgroup_resource_t*)resource;
			res->type = (uint32_t)resource->type;
			res->sty_id = (uint32_t)resource->id;
			res->x = (int16_t)group->x;
			res->y = (int16_t)group->y;
			res->width = (uint16_t)group->width;
			res->height = (uint16_t)group->height;
			res->backgroud = (uint32_t)group->background;
			res->opaque = (uint32_t)group->transparence;			
			res->resource_sum = (uint32_t)group->resource_sum;
			
			res_offset += sizeof(sty_group_t);
			res->child_offset = (uint32_t)res_offset;
			printf("buf_offset 0x%x, group child offset 0x%x, sum %d\n", buf_offset, group->child_offset, group->resource_sum);
			printf("group 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",group->absolute_x, group->absolute_y, group->x, group->y, group->visible, group->height);
			magic += res->sty_id;
			printf("%d magic 0x%x\n", __LINE__, magic);
			_parse_group(sty_data, &buf_offset, group, res_data, &res_offset, magic);
			res->offset = res_offset;
			magic -= res->sty_id;
	    }

	    if(resource->type == RESOURCE_TYPE_PICTURE)
	    {
	    	//buf is now on resource start
	    	sty_picture_t* res = (sty_picture_t*)(res_data + res_offset);
			picture_resource_t* pic = (picture_resource_t*)resource;
			res->type = (uint32_t)resource->type;
			res->sty_id = resource->id;
			res->x = (int16_t)pic->x;
			res->y = (int16_t)pic->y;
			res->width = (uint16_t)pic->width;
			res->height = (uint16_t)pic->height;
			res->format = (uint16_t)pic->visible;
			res->id = (uint32_t)pic->pic_id;
			_parse_bitmap(sty_data, res);
			res_offset += sizeof(sty_picture_t);
			res->offset = res_offset;
			res->magic = magic + res->sty_id;
			printf("0x%x, 0x%x\n", magic, res->sty_id);
			printf("%d magic 0x%x\n", __LINE__, res->magic);
			
	    }

	    if(resource->type == RESOURCE_TYPE_TEXT)
	    {
	    	//buf is now on resource start
	    	sty_string_t* res = (sty_string_t*)(res_data + res_offset);
			string_resource_t* str = (string_resource_t*)resource;
			res->type = (uint32_t)resource->type;
			res->sty_id = resource->id;
			res->x = (int16_t)str->x;
			res->y = (int16_t)str->y;
			res->width = (uint16_t)str->width;
			res->height = (uint16_t)str->height;
			res->color = (uint32_t)str->foreground;
			res->bgcolor = (uint32_t)str->background;
			res->align = (uint16_t)str->text_align;
			res->font_size = (uint16_t)str->font_height;
			res->id = (uint32_t)str->str_id;

			res_offset += sizeof(sty_string_t);
			res->offset = res_offset;
	    }

		if(resource->type == RESOURCE_TYPE_PICREGION)
		{
			sty_picregion_t* res = (sty_picregion_t*)(res_data + res_offset);
			picregion_resource_t* picreg = (picregion_resource_t*)resource;
			res->type = (uint32_t)resource->type;
			res->sty_id = resource->id;
			res->x = (int16_t)picreg->x;
			res->y = (int16_t)picreg->y;
			res->width = (uint16_t)picreg->width;
			res->height = (uint16_t)picreg->height;
			res->format = (uint16_t)picreg->visible;
			res->frames = (uint32_t)picreg->frames;

			res_offset += sizeof(sty_picregion_t);
			res->id_offset = res_offset;
			uint8_t* tmp = (res_data + res_offset);
			sty_picture_t* bitmap = (sty_picture_t*)tmp;
			buf = (char*)resource + picreg->pic_offset;
			printf("33333: 0x%x, id offset 0x%x\n", resource->id, res->id_offset);
			for(j = 0; j < res->frames; j++)
			{
				bitmap->type = RESOURCE_TYPE_PICTURE;
				bitmap->sty_id = res->sty_id;
				res_offset += sizeof(sty_picture_t);
				bitmap->offset = res_offset;
				bitmap->x = res->x;
				bitmap->y = res->y;
				bitmap->width = res->width;
				bitmap->height = res->height;
				bitmap->format = res->format;
				bitmap->id = (uint32_t)(buf[1]<<8 | buf[0]);
				bitmap->magic = magic + res->sty_id;
				_parse_bitmap(sty_data, bitmap);
				if(bpp == 0)
				{
					bpp = bitmap->bytes_per_pixel;
				}
				printf("w %d h%d\n", bitmap->width, bitmap->height);
				buf += 2;
				bitmap = (sty_picture_t*)(res_data + res_offset);
			}
			res->bytes_per_pixel = bpp;
			res->offset = res_offset;
			res->magic = magic + res->sty_id;
			printf("%d magic 0x%x\n", __LINE__, res->magic);
			
			printf("44444: offset 0x%x, id_offset 0x%x\n", res->offset, res->id_offset);
			
		}	

		buf_offset += resource->offset;
		buf = sty_data + buf_offset;
		resource = (resource_t*)buf;
	}

	*pres_offset = res_offset;
	return 0;
}
#if 1
int main(int argc, char* argv[])
{
    FILE* orip = NULL;
    FILE* newp = NULL;
    unsigned char* sty_data;
    unsigned char res_data[MAX_RES_DATA_SIZE];
    int res_size;
	int res_offset;
    resource_info_t info;
    resource_scene_t* scenes;
	int i;
	unsigned int* scene_item;
    uint32_t magic;
    int sty_size;
    int ret;
	char diff_path[MAX_DIFF_PATH_LEN];
	char* diff_ext;
	res_head_t head;

	printf("%s %d\n\n", __FILE__, __LINE__);

    if(argc < 3)
    {
        printf("usage: sty_conv [styfile_path] [picfile_path] [new_styfile_path]");
        return 0;
    }


	printf("%s %d\n", __FILE__, __LINE__);
    orip = fopen(argv[1], "rb");
    if(orip == NULL)
    {
        printf("open sty %s faild \n", argv[1]);
        return -1;
    }

	picfp = fopen(argv[2], "rb");
    if(picfp == NULL)
    {
        printf("open picres %s faild \n", argv[2]);
		fclose(orip);
        return -1;
    }
	fread(&head, 1, sizeof(res_head_t), picfp);
	basic_id_end = head.counts;
		
	memset(diff_path, 0, MAX_DIFF_PATH_LEN);
	strcpy(diff_path, argv[2]);
	diff_ext = strrchr(diff_path, '.');
	strcpy(diff_ext, "_OTA.res");
	picfp_diff = fopen(diff_path, "rb");
	if(picfp_diff == NULL)
	{
		printf("no diff pic res\n");
	}
	else
	{
		fread(&head, 1, sizeof(res_head_t), picfp_diff);
		diff_id_start = head.id_start;
		if(basic_id_end >= diff_id_start)
		{
			basic_id_end = head.id_start-1;
		}
	}


    fseek(orip, 0, SEEK_END);
    sty_size = ftell(orip);

    sty_data = (unsigned char*)malloc(sty_size);
	fseek(orip, 0, SEEK_SET);
    ret = fread(sty_data, 1, sty_size, orip);
    if(ret < sty_size)
    {
        printf("read sty %s failed\n", argv[1]);
        free(sty_data);
        fclose(orip);
		fclose(picfp);
        return -1;
    }
    fclose(orip);

    memset(&info, 0, sizeof(resource_info_t));
    _read_head(sty_data, &info);
	scenes = (resource_scene_t*)malloc(sizeof(resource_scene_t)*info.sum);
	
    res_size = 0;
	res_offset = 0;
	memset(res_data, 0, MAX_RES_DATA_SIZE);
	scene_item = info.scenes;
	printf("scene sum %d\n", info.sum);
	
    for(i=0;i<info.sum;i++)
    {
    	uint32_t scene_id = *scene_item;
		unsigned int offset = *( scene_item + 1 );
		unsigned int size = *( scene_item + 2 );
		scene_t* scene = (scene_t*)(sty_data+offset);
		scenes[i].scene_id = scene_id;
		scenes[i].x = scene->x;
		scenes[i].y = scene->y;
		scenes[i].background = scene->background;
		scenes[i].width = scene->width;
		scenes[i].height = scene->height;
		scenes[i].resource_sum = scene->resource_sum;
		scenes[i].direction = scene->direction;
		scenes[i].visible = scene->visible;
		scenes[i].transparence = scene->transparence;
		scenes[i].opaque = scene->opaque;

		printf("scene_id 0x%x, offset 0x%x, size %d\n", scene_id, offset, size);
		scenes[i].child_offset = res_offset;
		magic = scene_id;
		printf("%d magic 0x%x\n", magic);
        _parse_scene(sty_data, offset, scene, res_data, &res_offset, magic);
		scene_item = scene_item + SCENE_ITEM_SIZE;
    }

	if(argc == 4)
	{
		newp = fopen(argv[3], "wb");
	}
	else
	{
    	newp = fopen(argv[1], "wb");
	}
    if(newp == NULL)
    {
        free(scenes);
        free(sty_data);
        return -1;
    }

    fwrite(&info, 1, sizeof(resource_info_t), newp);
    fwrite(scenes, 1, sizeof(resource_scene_t)*info.sum, newp);
    fwrite(res_data, 1, res_offset, newp);
    fclose(newp);
	fclose(picfp);
	
	free(scenes);
	free(sty_data);
    return 0;
}
#endif