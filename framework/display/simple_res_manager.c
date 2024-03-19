#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <os_common_api.h>
#include <memory/mem_cache.h>
#include "res_manager_api.h"

#include <logging/log.h>


#define RES_MEM_DEBUG			0

#define SCENE_ITEM_SIZE 3

#define MAX_REGULAR_NUM			512

#define STY_PATH_MAX_LEN		24

#define COMPACT_BUFFER_MAX_PAD_SIZE		4*1024
#define COMPACT_BUFFER_MARGIN_SIZE		64

#define MAX_INNER_BITMAP_SIZE		64
#define MAX_ARRAY_INNER_SIZE		32*1024
#define MAX_ARRAY_INNER_BLOCK		32

#define MAX_PRELOAD_BITMAP_SIZE		32
#define MAX_ARRAY_PRELOAD_SIZE		16*1024	
#define MAX_ARRAY_PRELOAD_BLOCK		16

#define MAX_COMPACT_BITMAP_SIZE		16
#define MAX_ARRAY_COMPACT_SIZE		8*1024	
#define MAX_ARRAY_COMPACT_BLOCK		8

#define MAX_ARRAY_BUFFER_BLOCK		(MAX_ARRAY_INNER_BLOCK+MAX_ARRAY_PRELOAD_BLOCK+MAX_ARRAY_COMPACT_BLOCK)
#define MAX_ARRAY_BUFFER_SIZE		(MAX_ARRAY_INNER_SIZE+MAX_ARRAY_PRELOAD_SIZE+MAX_ARRAY_COMPACT_SIZE)


#define RES_SCENE_POOL_MAX_BLOCK_NUMBER		64
#ifndef CONFIG_RES_MEM_POOL_MAX_BLOCK_NUMBER
#define RES_MEM_POOL_MAX_BLOCK_NUMBER 		2370
#else
#define RES_MEM_POOL_MAX_BLOCK_NUMBER		(CONFIG_RES_MEM_POOL_MAX_BLOCK_NUMBER-RES_TXT_POOL_MAX_BLOCK_NUMBER-RES_SCENE_POOL_MAX_BLOCK_NUMBER)
#endif
#define RES_TXT_POOL_MAX_BLOCK_NUMBER		10
#define RES_MEM_POOL_MAX_BLOCK_SIZE      	1024
#define RES_MEM_POOL_MIN_BLOCK_SIZE      	16
#define RES_MEM_POOL_ALIGN_SIZE         	8

#define RES_ARRAY_MEM_DEBUG		1
#define RES_MEM_DEBUG			0
#if RES_MEM_DEBUG
#define DEBUG_PATTERN_SIZE		16
#endif


//以下宏定义资源图片的类型
typedef enum{
    RES_TYPE_INVALID                 = 0,
    RES_TYPE_PIC                     = 1,  // 16 bit picture (RGB_565, R[15:11], G[10:5], B[4:0])
    RES_TYPE_STRING                  = 2,  // string
    RES_TYPE_MSTRING,                      // multi-Language string
    RES_TYPE_LOGO                    = 4,  // BIN format picture
    RES_TYPE_PNG,                          // 24 bit picture with alpha (ARGB_8565, A[23:16], R[15:11], G[10:5], B[4:0])
    RES_TYPE_PIC_4,
    RES_TYPE_PIC_COMPRESSED          = 7,  // compressed 16 bit picture (compressed RGB_565)
    RES_TYPE_PNG_COMPRESSED,               // compressed 24 bit compressed picture with alpha (compressed ARGB_8565)
    RES_TYPE_LOGO_COMPRESSED,              // compressed BIN format picture

    RES_TYPE_PIC_RGB888              = 10, // 24 bit picture without alpha (RGB_888, R[23:16], G[16:8], B[7:0])
    RES_TYPE_PIC_ARGB8888,                 // 32 bit picture with alpha (ARGB_8888, , A[31:24], R[23:16], G[16:8], B[7:0])
    RES_TYPE_PIC_RGB888_COMPRESSED,        // compressed 24 bit picture without alpha (RGB_888)
    RES_TYPE_PIC_ARGB8888_COMPRESSED = 13, // compressed 32 bit picture with alpha (ARGB_8888)

    RES_TYPE_PIC_LZ4 = 14,		// 16 bit picture (RGB_565, R[15:11], G[10:5], B[4:0]) with LZ4 compressed
    RES_TYPE_PIC_ARGB8888_LZ4,	// 32 bit picture with alpha (ARGB_8888, , A[31:24], R[23:16], G[16:8], B[7:0]) with LZ4 compressed

    NUM_RES_TYPE,
}res_type_e;

/*style head*/
typedef struct
{
    uint32_t    scene_sum;
    uint32_t	reserve;
}style_head_t;



//res head
typedef struct
{
    uint8_t  magic[4];        //'R', 'E', 'S', 0x19
    uint16_t counts;
    uint16_t w_format;
    uint8_t brgb;
    uint8_t version;
    uint8_t reserved[5];
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

typedef struct _buf_block_s
{
	uint32_t source;
	uint32_t id;
	uint32_t size;
	uint32_t ref;
	uint32_t width;
	uint32_t height;
	uint32_t bytes_per_pixel;
	uint32_t format;
	uint8_t* addr;
	uint32_t regular_info;
	struct _buf_block_s* next;
}buf_block_t;

typedef struct
{
	compact_buffer_t* compact_buffer_list;
	buf_block_t* head;
}resource_buffer_t;

typedef struct _regular_info_s
{
	uint32_t scene_id;
	uint32_t magic;
	uint32_t num;
	uint32_t* id;
	struct _regular_info_s* next;	
}regular_info_t;

typedef struct _mem_info
{
	void* ptr;
	size_t size;
	uint32_t type;
	struct _mem_info* next;
}mem_info_t;


//K_HEAP_DEFINE(freetype_mem_pool, RES_MEM_POOL_MAX_BLOCK_NUMBER * RES_MEM_POOL_MAX_BLOCK_SIZE);
static char __aligned(4) res_mem_pool_buffer[RES_MEM_POOL_MAX_BLOCK_NUMBER * RES_MEM_POOL_MAX_BLOCK_SIZE] __in_section_unique(RES_PSRAM_REGION);

Z_STRUCT_SECTION_ITERABLE(k_heap, res_mem_pool) = {
				.heap = {
					.init_mem = res_mem_pool_buffer,
					.init_bytes = RES_MEM_POOL_MAX_BLOCK_NUMBER * RES_MEM_POOL_MAX_BLOCK_SIZE,					
					},
};

static char __aligned(4) res_txt_pool_buffer[RES_TXT_POOL_MAX_BLOCK_NUMBER * RES_MEM_POOL_MAX_BLOCK_SIZE] __in_section_unique(RES_PSRAM_REGION);

Z_STRUCT_SECTION_ITERABLE(k_heap, res_txt_pool) = {
				.heap = {
					.init_mem = res_txt_pool_buffer,
					.init_bytes = RES_TXT_POOL_MAX_BLOCK_NUMBER * RES_MEM_POOL_MAX_BLOCK_SIZE,					
					},
};

static char __aligned(4) res_scene_pool_buffer[RES_SCENE_POOL_MAX_BLOCK_NUMBER * RES_MEM_POOL_MAX_BLOCK_SIZE] __in_section_unique(RES_PSRAM_REGION);

Z_STRUCT_SECTION_ITERABLE(k_heap, res_scene_pool) = {
				.heap = {
					.init_mem = res_scene_pool_buffer,
					.init_bytes = RES_SCENE_POOL_MAX_BLOCK_NUMBER * RES_MEM_POOL_MAX_BLOCK_SIZE,					
					},
};

static mem_info_t* mem_info_list[RES_MEM_POOL_TYPE_MAX];

static size_t mem_total[RES_MEM_POOL_TYPE_MAX];

static uint32_t* array_bitmap[RES_MEM_SIMPLE_TYPE_MAX];
static uint8_t* array_mem[RES_MEM_SIMPLE_TYPE_MAX];
static uint32_t array_unit_size[RES_MEM_SIMPLE_TYPE_MAX];
static uint8_t* array_offset[RES_MEM_SIMPLE_TYPE_MAX];
#ifdef RES_ARRAY_MEM_DEBUG
static uint32_t array_item_total[RES_MEM_SIMPLE_TYPE_MAX];
#endif

static resource_buffer_t bitmap_buffer;
static resource_buffer_t text_buffer;
static resource_buffer_t scene_buffer;


void *res_mem_alloc(uint32_t type, size_t size);
void res_mem_free(uint32_t type, void *ptr);
void res_mem_dump(void);

void res_mem_init(void)
{
	memset(mem_info_list ,0, sizeof(mem_info_t*)*RES_MEM_POOL_TYPE_MAX);
	memset(mem_total, 0, sizeof(size_t)*RES_MEM_POOL_TYPE_MAX);

	memset(array_bitmap, 0, sizeof(uint32_t*)*RES_MEM_SIMPLE_TYPE_MAX);
	memset(array_mem, 0, sizeof(uint8_t*)*RES_MEM_SIMPLE_TYPE_MAX);

	printf("checkmem res_mem_pool_buffer %p\n", res_mem_pool_buffer);
	printf("checkmem res_txt_pool_buffer %p\n", res_txt_pool_buffer);
	printf("checkmem res_scene_pool_buffer %p\n", res_scene_pool_buffer);
#ifdef RES_ARRAY_MEM_DEBUG
	memset(array_item_total, 0, sizeof(uint32_t)*RES_MEM_SIMPLE_TYPE_MAX);
#endif
}

void res_mem_deinit(void)
{
	int32_t type;

	for(type=0;type<RES_MEM_SIMPLE_TYPE_MAX;type++)
	{
		if(array_mem[type] != NULL)
		{
			//res_mem_free(RES_MEM_POOL_BMP, array_mem);
			k_heap_free(&res_mem_pool, array_mem[type]);
			array_mem[type] = NULL;
		}

		if(array_bitmap[type] != NULL)
		{
			//res_mem_free(RES_MEM_POOL_BMP, array_bitmap);
			k_heap_free(&res_mem_pool, array_bitmap[type]);
			array_bitmap[type] = NULL;
		}
	}
}

uint32_t _get_simple_size(int32_t type)
{
	switch(type)
	{
	case RES_MEM_SIMPLE_INNER:
		return MAX_ARRAY_INNER_SIZE;
	case RES_MEM_SIMPLE_PRELOAD:
		return MAX_ARRAY_PRELOAD_SIZE;
	case RES_MEM_SIMPLE_COMPACT:
		return MAX_ARRAY_COMPACT_SIZE;
	default:
		return 0;
	}
}

uint32_t _get_simple_unit_size(int32_t type)
{
	switch(type)
	{
	case RES_MEM_SIMPLE_INNER:
		return sizeof(mem_info_t);
	default:
		return 0;
	}
}

uint32_t _get_simple_bitmap_size(int32_t type)
{
	switch(type)
	{
	case RES_MEM_SIMPLE_INNER:
		return MAX_INNER_BITMAP_SIZE;
	case RES_MEM_SIMPLE_PRELOAD:
		return MAX_PRELOAD_BITMAP_SIZE;
	case RES_MEM_SIMPLE_COMPACT:
		return MAX_COMPACT_BITMAP_SIZE;
	default:
		return 0;
	}
}

int32_t _get_array_type(void* ptr)
{
	int32_t type;
	for(type=0;type<RES_MEM_SIMPLE_TYPE_MAX;type++)
	{
		if(array_mem[type] != NULL)
		{
			if((uint8_t*)ptr >= array_mem[type] && (uint8_t*)ptr < array_mem[type]+_get_simple_size(type))
			{	
				return type;
			}
		}
	}

	return -1;
}


void* res_array_alloc(int32_t type, size_t size)
{
	uint8_t* ptr;
	uint32_t bit_offset;
	uint32_t i;
	uint32_t mask;
	uint32_t array_bitmap_size = _get_simple_bitmap_size(type);

	if(size > array_unit_size[type])
	{
		printf("invalid array alloc type %d, size %d, unit size %d\n", type, size, array_unit_size[type]);
	}
	
	if(array_mem[type] == NULL)
	{
		uint32_t array_size = _get_simple_size(type);
		array_mem[type] = ptr = k_heap_alloc(&res_mem_pool, array_size, K_NO_WAIT);
		if(array_mem[type] == NULL)
		{
			SYS_LOG_ERR("no buffer to preload\n");
			return NULL;
		}
		memset(array_mem[type], 0, array_size);
		array_unit_size[type] = _get_simple_unit_size(type);
		array_offset[type] = array_mem[type];
	}

	if(array_bitmap[type] == NULL)
	{
		array_bitmap[type] = (uint32_t*)k_heap_alloc(&res_mem_pool, array_bitmap_size, K_NO_WAIT);
		if(array_bitmap[type] == NULL)
		{
			SYS_LOG_ERR("no buffer to preload\n");
			return NULL;
		}	
		memset(array_bitmap[type], 0, array_bitmap_size);
	}

	for(i=0;i<array_bitmap_size/4;i++)
	{
		uint32_t check = array_bitmap[type][i]&0xffffffff;
		if( check != 0xffffffff)
		{
			bit_offset = 0;			
			while(check%2 != 0)
			{
				check = check>>1;
				bit_offset++;
			}
			break;
		}
	}

	if(i >= array_bitmap_size/4)
	{
#ifdef RES_ARRAY_MEM_DEBUG
		SYS_LOG_ERR("array mem full type %d, total %d\n", type, array_item_total[type]);
#else
		SYS_LOG_ERR("array mem full type %d,\n", type);
#endif
		return NULL;
	}

	mask = 1<<bit_offset;
	array_bitmap[type][i] = array_bitmap[type][i]|mask;
	ptr = array_offset[type] + (i*32+bit_offset)*array_unit_size[type];

#if RES_MEM_DEBUG

	printf("ptr 0x%x, array_mem 0x%x, array_offset 0x%x\n", ptr, array_mem[type], array_offset[type]);

	if(res_mem_check()<0)
	{
		printf("%s %d ptr 0x%x, array_mem 0x%x, array_offset 0x%x\n", __FILE__, __LINE__, ptr, array_mem[type], array_offset[type]);
		res_mem_dump();
	}
#endif		

#ifdef RES_ARRAY_MEM_DEBUG
	array_item_total[type]++;
#endif
	return ptr;
	
}

uint32_t res_array_free(void* ptr)
{
	uint32_t i;
	uint32_t bit_offset;
	uint32_t pos;
	uint32_t mask;
	int32_t type;
	uint32_t array_size;
	uint32_t array_bitmap_size;
	
	type = _get_array_type(ptr);
	if(type < 0)
	{
		printf("unknown array type for %p\n", ptr);
		return 0;
	}
	else
	{
		array_size = _get_simple_size(type);
		array_bitmap_size = _get_simple_bitmap_size(type);

#ifdef RES_ARRAY_MEM_DEBUG
		array_item_total[type]--;
#endif

		pos = (uint32_t)((uint8_t*)ptr - array_mem[type])/array_unit_size[type];
		i = pos/32;
		if(i < array_bitmap_size/4)
		{
			bit_offset = pos%32;
			mask = 1<<bit_offset;
			mask = ~mask;
			array_bitmap[type][i] = array_bitmap[type][i] & mask;
			
			return 1;
		}
		else
		{
			printf("invalid array pos when free type %d, ptr %p, array mem %p\n", type, ptr, array_mem[type]);
		}
	}
	
	return 0;
}

void _add_mem_info(uint32_t type, void* ptr, size_t size)
{
	mem_info_t* prev;
	mem_info_t* listp = NULL;
	mem_info_t* item;

	//item = mem_malloc(sizeof(mem_info_t));
	item = (mem_info_t*)res_array_alloc(RES_MEM_SIMPLE_INNER, sizeof(mem_info_t));
	if(item == NULL)
	{
		SYS_LOG_ERR("no ram for res mem info\n");
		return;
	}
	item->ptr = ptr;
	item->size = size;
	item->type = type;
	item->next = NULL;

	listp = mem_info_list[type];
	if(listp == NULL)
	{
		mem_info_list[type] = item;
	}
	else
	{
		while(listp != NULL)
		{
			prev = listp;
			listp = listp->next;
		}
		prev->next = item;
	}
	listp = mem_info_list[type];
	while(listp!=NULL)
	{
		listp = listp->next;
	}
#if RES_MEM_DEBUG
	mem_info_t* testitem = mem_info_list[type];
	uint8_t* check = testitem->ptr;
	uint32_t invalid = 0;
	uint32_t i;
	while(testitem != NULL)
	{
		for(i=0;i<DEBUG_PATTERN_SIZE;i++)
		{
			if(check[i] != 0x42)
			{
				invalid = 1;
				break;
			}
		}

		check = testitem->ptr + testitem->size - DEBUG_PATTERN_SIZE;
		for(i=0;i<DEBUG_PATTERN_SIZE;i++)
		{
			if(check[i] != 0x42)
			{
				invalid = 1;
				break;
			}		
		}

		if(invalid == 1)
		{
			break;
		}

		testitem = testitem->next;
	}

	if(invalid != 0)
	{
		res_mem_dump();
//		k_panic();
	}

#endif

	mem_total[type] += size;
//	SYS_LOG_INF("testalloc alloc == ptr 0x%x size %d type %d, total %d\n", ptr, size, type, mem_total[type]);

}

void _remove_mem_info(uint32_t type, void* ptr)
{
	mem_info_t* prev;
	mem_info_t* listp;
	mem_info_t* item;

	
	listp = mem_info_list[type];
	while(listp != NULL)
	{
		if(listp->ptr == ptr)
		{
			break;
		}
		prev = listp;
		listp = listp->next;
	}

	if(listp == NULL)
	{
		SYS_LOG_ERR("testalloc remove mem info not found %p\n", ptr);
		return;
	}
	
	item = listp;
	
#if RES_MEM_DEBUG
	mem_info_t* testitem = mem_info_list[type];
	uint8_t* check = testitem->ptr;
	uint32_t invalid = 0;
	uint32_t i;
	while(testitem != NULL)
	{
		for(i=0;i<DEBUG_PATTERN_SIZE;i++)
		{
			if(check[i] != 0x42)
			{
				invalid = 1;
				break;
			}
		}

		check = testitem->ptr + testitem->size - DEBUG_PATTERN_SIZE;
		for(i=0;i<DEBUG_PATTERN_SIZE;i++)
		{
			if(check[i] != 0x42)
			{
				invalid = 1;
				break;
			}		
		}

		if(invalid == 1)
		{
			break;
		}

		testitem = testitem->next;
	}

	if(invalid != 0)
	{
		res_mem_dump();
//		k_panic();
	}

#endif
	
	if(listp == mem_info_list[type])
	{
		mem_info_list[type] = listp->next;
		mem_total[type] -= item->size;
//		SYS_LOG_INF("testalloc free == item 0x%x, ptr 0x%x size %d type %d total %d\n", item, item->ptr, item->size, type, mem_total[type]);
		//mem_free(item);
		res_array_free(item);
	}
	else
	{
		prev->next = listp->next;
		mem_total[type] -= item->size;
//		SYS_LOG_INF("testalloc free == item 0x%x, ptr 0x%x size %d type %d total %d\n", item, item->ptr, item->size, type, mem_total[type]);
		//mem_free(item);
		res_array_free(item);
	}

}

size_t _get_mem_size(uint32_t type, void* ptr)
{
	mem_info_t* prev;
	mem_info_t* listp;
	void* real_ptr;

#if RES_MEM_DEBUG
	real_ptr = ptr - DEBUG_PATTERN_SIZE;
#else
	real_ptr = ptr;
#endif

	listp = mem_info_list[type];
	while(listp != NULL)
	{
		if(listp->ptr == real_ptr)
		{
			break;
		}
		prev = listp;
		listp = listp->next;
	}

	if(listp == NULL)
	{
		SYS_LOG_ERR("testalloc get mem size not found %p \n", ptr);
		return 0;
	}

#if RES_MEM_DEBUG
	return listp->size - DEBUG_PATTERN_SIZE*2;
#else
	return listp->size;
#endif
}

struct k_heap* _get_mem_heap(uint32_t type)
{
	switch(type)
	{
	case RES_MEM_POOL_BMP:
		return &res_mem_pool;
	case RES_MEM_POOL_TXT:
		return &res_txt_pool;
	case RES_MEM_POOL_SCENE:
		return &res_scene_pool;
	default:
		return NULL;
	}
}

int32_t res_mem_check(void)
{
#if RES_MEM_DEBUG
/*
	bool ret;
	ret = k_heap_validate(&res_mem_pool, NULL);
	if(ret == false)
	{
		return -1;
	}
	else
	{
		return 0;
	}
*/
#else
	return 0;
#endif

}

void res_mem_dump(void)
{
	int i;
	SYS_LOG_INF("res memory info:\n");
	for(i=0;i<RES_MEM_POOL_TYPE_MAX;i++)
	{
		SYS_LOG_INF("memtype %d, total %d\n", i, mem_total[i]);
	}

	SYS_LOG_INF("res array memory info:\n");
	for(i=0;i<RES_MEM_SIMPLE_TYPE_MAX;i++)
	{
		SYS_LOG_INF("arraytype %d, total %d\n", i, array_item_total[i]);
	}

#if RES_MEM_DEBUG	
	uint8_t* check = NULL;
	mem_info_t* item = NULL;
	uint32_t i;
	uint32_t type;

	for(type = 0; type < RES_MEM_POOL_TYPE_MAX; type++)
	{
		item = mem_info_list[type];
		while(item)
		{
			check = item->ptr;
			printf("\n check item 0x%x, ptr 0x%x, size 0x%x \n head:", item, check, item->size);
			for(i=0;i<DEBUG_PATTERN_SIZE;i++)
			{
				printf("0x%x, ", check[i]);
			}
			printf("\n");
			check = item->ptr + item->size - DEBUG_PATTERN_SIZE;
			printf(" tail:");
			for(i=0;i<DEBUG_PATTERN_SIZE;i++)
			{
				printf("0x%x, ", check[i]);
			}
			printf("\n");		

			item = item->next;
		}
	}
#endif
}

void *res_mem_alloc(uint32_t type, size_t size)
{
	void *ptr;
	struct k_heap* res_heap;

	if(type >= RES_MEM_POOL_TYPE_MAX)
	{
		SYS_LOG_ERR("invalid res mem type to alloc\n");
		return NULL;
	}

	if(size % 4 != 0)
	{
		size = (size/4 + 1)*4;
	}

#if RES_MEM_DEBUG
	size += DEBUG_PATTERN_SIZE*2;
#endif

	res_heap = _get_mem_heap(type);
	ptr = k_heap_alloc(res_heap, size, K_NO_WAIT);
	if (ptr == NULL) {
		SYS_LOG_ERR("heap alloc failed, size %d, type %d, total %d\n", size, type, mem_total[type]);
		res_mem_dump();
		return ptr;
	}

#if RES_MEM_DEBUG

	memset(ptr, 0x42, DEBUG_PATTERN_SIZE);
	memset(ptr+size-DEBUG_PATTERN_SIZE, 0x42, DEBUG_PATTERN_SIZE);
	
	_add_mem_info(type, ptr, size);
	SYS_LOG_INF("testalloc alloc == ptr 0x%x size %d type %d total %d\n", ptr, size, type, mem_total[type]);
	return ptr+DEBUG_PATTERN_SIZE;	
#else	
	_add_mem_info(type, ptr, size);
	return ptr;
#endif
}

void res_mem_free(uint32_t type, void *ptr)
{
	struct k_heap* res_heap;

	if(type >= RES_MEM_POOL_TYPE_MAX)
	{
		SYS_LOG_ERR("invalid res mem type to remove\n");
		return;
	}
	
#if RES_MEM_DEBUG
	void* real_ptr = ptr-DEBUG_PATTERN_SIZE;
	res_heap = _get_mem_heap(type);
	_remove_mem_info(type, real_ptr);	
	k_heap_free(res_heap, real_ptr);
#else
	res_heap = _get_mem_heap(type);
	_remove_mem_info(type, ptr);
	k_heap_free(res_heap, ptr);
#endif
}


void _resource_buffer_deinit(uint32_t force_clear)
{
	buf_block_t* item;
	buf_block_t* listp;

	if(force_clear)
	{
		listp = text_buffer.head;
		while(listp != NULL)
		{
			item = listp;
			listp = listp->next;
			res_mem_free(RES_MEM_POOL_TXT, item);
		}
		memset(&text_buffer, 0, sizeof(text_buffer));

		listp = scene_buffer.head;
		while(listp != NULL)
		{
			item = listp;
			listp = listp->next;
			res_mem_free(RES_MEM_POOL_SCENE, item);
		}
		memset(&scene_buffer, 0, sizeof(scene_buffer));
	}

}

void _resource_buffer_init(void)
{
	memset(&bitmap_buffer, 0, sizeof(bitmap_buffer));
	memset(&text_buffer, 0, sizeof(text_buffer));
	memset(&scene_buffer, 0, sizeof(scene_buffer));
}



uint8_t* _get_resource_scene_buffer(uint32_t source, uint32_t id, uint32_t size)
{
	buf_block_t* item = NULL;

	item = scene_buffer.head;
	while(item != NULL)
	{
		if((item->source == source)&&(item->id == id))
		{
			break;
		}
		item = item->next;
	}

	if(item != NULL)
	{
		//found loaded scene, which is not supposed to happen but fine
		item->ref ++;
		return item->addr;
	}
	else
	{
		item = (buf_block_t*)res_mem_alloc(RES_MEM_POOL_SCENE, sizeof(buf_block_t) + size);
		if(item == NULL)
		{
			SYS_LOG_ERR("error: scene buffer not enough for size %d", size);
			return NULL;
		}
		SYS_LOG_INF("item %p", item);
		item->source = source;
		item->id = id;
		item->size = sizeof(buf_block_t) + size;
		item->ref = 1;
		item->addr = (uint8_t*)item+sizeof(buf_block_t);
		item->next = scene_buffer.head;
		scene_buffer.head = item;

#if RES_MEM_DEBUG
		if(res_mem_check()<0)
		{
			SYS_LOG_INF("%s %d\n", __FILE__, __LINE__);
			res_mem_dump();
		}
#endif		
		return item->addr;
	}
}

uint8_t* _try_resource_scene_buffer(uint32_t source, uint32_t id)
{
	buf_block_t* item;

	item = scene_buffer.head;
	while(item != NULL)
	{
		if((item->source == source)&&(item->id == id))
		{
			break;
		}
		item = item->next;
	}

	if(item != NULL)
	{
		//found loaded text
		item->ref ++;
		return item->addr;
	}
	else
	{
		//not found, need to load from file
		return NULL;
	}
}

void _free_resource_scene_buffer(void* p)
{
	buf_block_t* item = scene_buffer.head;
	buf_block_t* prev = NULL;

	if(p == NULL)
	{
		return;
	}

	while(item != NULL)
	{
		if(item->addr == p)
		{
			break;
		}
		prev = item;
		item = item->next;
	}

	if(item == NULL)
	{
		SYS_LOG_ERR("scene not found");
		return;
	}
	else if((item != NULL)&&(item->ref > 1))
	{
		item->ref --;
		//FIXME
	}
	else
	{
		if(prev == NULL)
		{
			scene_buffer.head = item->next;
		}
		else
		{
			prev->next = item->next;
		}
		res_mem_free(RES_MEM_POOL_SCENE, (void*)item);

#if RES_MEM_DEBUG
		if(res_mem_check()<0)
		{
			SYS_LOG_INF("%s %d\n", __FILE__, __LINE__);
			res_mem_dump();
		}
#endif		
	}
}

uint8_t* _get_resource_text_buffer(uint32_t source, uint32_t id, uint32_t size)
{
	buf_block_t* item;

	item = text_buffer.head;
	while(item != NULL)
	{
		if((item->source == source)&&(item->id == id))
		{
			break;
		}
		item = item->next;
	}

	if(item != NULL)
	{
		//found loaded text, which is not supposed to happen but fine
		item->ref ++;
		return item->addr;
	}
	else
	{
		item = (buf_block_t*)res_mem_alloc(RES_MEM_POOL_TXT, sizeof(buf_block_t) + size);
		if(item == NULL)
		{
			SYS_LOG_ERR("error: resource buffer not enough for size 0x%x", size);
			return NULL;
		}
		item->source = source;
		item->id = id;
		item->size = sizeof(buf_block_t) + size;
		item->ref = 1;
		item->addr = (uint8_t*)item+sizeof(buf_block_t);
		item->next = text_buffer.head;
		text_buffer.head = item;
		return item->addr;
	}

}


uint8_t* _try_resource_text_buffer(uint32_t source, uint32_t id)
{
	buf_block_t* item;

	item = text_buffer.head;
	while(item != NULL)
	{
		if((item->source == source)&&(item->id == id))
		{
			break;
		}
		item = item->next;
	}

	if(item != NULL)
	{
		//found loaded text
		item->ref ++;
		return item->addr;
	}
	else
	{
		//not found, need to load from file
		return NULL;
	}
}

void _free_resource_text_buffer(void* p)
{
	buf_block_t* item = text_buffer.head;
	buf_block_t* prev = NULL;

	while(item != NULL)
	{
		if(item->addr == p)
		{
			break;
		}
		prev = item;
		item = item->next;
	}

	if(item == NULL)
	{
		SYS_LOG_ERR("resource text %p not found", p);
		return;
	}
	else if((item != NULL)&&(item->ref > 1))
	{
		item->ref --;
		//FIXME
		return;
	}
	else
	{
		if(prev == NULL)
		{
			text_buffer.head = item->next;
		}
		else
		{
			prev->next = item->next;
		}
		res_mem_free(RES_MEM_POOL_TXT, (void*)item);
	}
}





uint8_t* _get_resource_bitmap_buffer(uint32_t source, resource_bitmap_t* bitmap, uint32_t size, uint32_t force_ref)
{
	buf_block_t* item;

	item = bitmap_buffer.head;
	while(item != NULL)
	{
		if((item->source == source)&&(item->id == bitmap->sty_data->id))
		{
			break;
		}
		item = item->next;
	}

	if(item != NULL)
	{
		//found loaded bitmap, which is not supposed to happen but fine
		SYS_LOG_ERR("should not happen\n");
		if(force_ref >= 1)
		{
			item->ref ++;
		}
	
		return item->addr;
	}
	else
	{
		item = (buf_block_t*)res_mem_alloc(RES_MEM_POOL_BMP, sizeof(buf_block_t) + size);
		if(item == NULL)
		{
			SYS_LOG_ERR("error: resource buffer not enough for size %d\n", size);
			return NULL;
		}
		item->source = source;
		item->id = bitmap->sty_data->id;

		if(force_ref >= 1)
		{
			item->ref = 1;
		}
		else
		{
			item->ref = 0;
		}
		item->width = (uint32_t)bitmap->sty_data->width;
		item->height = (uint32_t)bitmap->sty_data->height;
		item->bytes_per_pixel = (uint32_t)bitmap->sty_data->bytes_per_pixel;
		item->format = (uint32_t)bitmap->sty_data->format;
		item->size = sizeof(buf_block_t) + size;
		item->addr = (uint8_t *)item + sizeof(buf_block_t);
		item->next = bitmap_buffer.head;
		bitmap_buffer.head = item;
		item->regular_info = bitmap->regular_info;
#if RES_MEM_DEBUG
		if(res_mem_check()<0)
		{
			SYS_LOG_INF("%s %d\n", __FILE__, __LINE__);
			res_mem_dump();
		}
#endif

		return item->addr;
	}

}

//uint8_t* _try_resource_bitmap_buffer(uint32_t source, uint32_t id, uint32_t* width, uint32_t* height,
//										uint32_t* bytes_per_pixel, uint32_t* format, uint32_t force_ref)
uint8_t* _try_resource_bitmap_buffer(uint32_t source, resource_bitmap_t* bitmap, uint32_t force_ref)
{
	buf_block_t* item;

	item = bitmap_buffer.head;
	while(item != NULL)
	{
		if((item->source == source)&&(item->id == bitmap->sty_data->id))
		{
			break;
		}
		item = item->next;
	}

	if(item != NULL)
	{
		//found loaded bitmap
		bitmap->regular_info = item->regular_info;
		if(force_ref == 1)
		{
			item->ref ++;
		}

//		SYS_LOG_INF("\n ### got cached bitmap id %d\n", id);
		return item->addr;
	}
	else
	{
		//not found, need to load from file
		return NULL;
	}
}

void _free_resource_bitmap_buffer(void *p)
{
	buf_block_t* item = bitmap_buffer.head;
	buf_block_t* prev = NULL;

#if RES_MEM_DEBUG
	if(res_mem_check()<0)
	{
		SYS_LOG_INF("%s %d item 0x%x \n", __FILE__, __LINE__, item);
		res_mem_dump();
	}
#endif		

	while(item != NULL)
	{
		if(item->addr == p)
		{
			break;
		}
		prev = item;
		item = item->next;
	}

	if(item == NULL)
	{
		SYS_LOG_ERR("resource bitmap not found %p", p);
		item = bitmap_buffer.head;
		while(item!=NULL)
		{
			SYS_LOG_INF("item in list %p \n", item->addr);
			item = item->next;
		}
		return;
	}
	else if((item != NULL)&&(item->ref > 1))
	{
		item->ref --;
		//FIXME
		return;
	}
	else if((item != NULL)&&(item->regular_info != NULL))
	{
		return;
	}
	else
	{
		if(prev == NULL)
		{
			bitmap_buffer.head = item->next;
		}
		else
		{
			prev->next = item->next;
		}

		res_mem_free(RES_MEM_POOL_BMP, (void*)item);

#if RES_MEM_DEBUG
		if(res_mem_check()<0)
		{
			SYS_LOG_INF("%s %d\n", __FILE__, __LINE__);
			res_mem_dump();
		}
#endif		
	}
}


//读取style 文件的头部信息, 返回值是该配置文件中scene 的总数
//缓存图片资源文件索引信息
static int _read_head( resource_info_t* info )
{
    style_head_t* head;
    int ret;
	uint32_t sty_size;

	fs_seek(&info->style_fp, 0, FS_SEEK_END);
	sty_size = fs_tell(&info->style_fp);
	info->sty_mem = (uint8_t*)res_mem_alloc(RES_MEM_POOL_SCENE, sty_size);
	if(info->sty_mem == NULL)
	{
		SYS_LOG_ERR("no enough mem for style data\n");
		return -1;
	}

	fs_seek(&info->style_fp, 0, FS_SEEK_SET);
	ret = fs_read(&info->style_fp, info->sty_mem, sty_size);
	if(ret < sty_size)
	{
		SYS_LOG_ERR("read style file error:%d\n", ret);
		free(info->sty_mem);
		info->sty_mem = NULL;
		return -1;
	}

	head = (style_head_t*)info->sty_mem;
	info->sum = head->scene_sum;
	info->scenes = (resource_scene_t*)(info->sty_mem + sizeof(style_head_t));
	info->sty_data = info->sty_mem + sizeof(style_head_t) + sizeof(resource_scene_t)*info->sum;

#if RES_MEM_DEBUG
	if(res_mem_check()<0)
	{
		SYS_LOG_INF("%s %d\n", __FILE__, __LINE__);
		res_mem_dump();
	}
#endif		

    return 0;
}

void res_manager_init(void)
{
	res_mem_init();
	_resource_buffer_init();
}

void res_manager_clear_cache(uint32_t force_clear)
{
	_resource_buffer_deinit(force_clear);
}

resource_info_t* res_manager_open_resources( const char* style_path, const char* picture_path, const char* text_path )
{
    int ret;
    resource_info_t* info;


    info = (resource_info_t*)mem_malloc( sizeof( resource_info_t ) );
    if ( info == NULL )
    {
        SYS_LOG_ERR( "error: malloc resource info failed !" );
        return NULL;
    }
    memset(info, 0, sizeof(resource_info_t));

    if ( style_path != NULL )
    {
		ret = fs_open(&info->style_fp, style_path, FS_O_READ);

	    if ( ret < 0 )
	    {
	        SYS_LOG_ERR(" open style file %s failed!", style_path);
	        goto ERR_EXIT;
	    }

/*
		info->sty_path = (uint8_t*)mem_malloc(strlen(style_path)+1);
		memset(info->sty_path, 0, strlen(style_path)+1);
		strcpy(info->sty_path, style_path);
*/
		info->sty_path = (uint8_t*)style_path;
    }
    else
    {
        LOG_WRN("style file path null");
    }

    if( picture_path != NULL)
    {
		ret = fs_open(&info->pic_fp, picture_path, FS_O_READ);
	    if ( ret < 0 )
	    {
	        SYS_LOG_ERR(" open picture resource file %s failed!", picture_path);
	        goto ERR_EXIT;
	    }
    }
    else
    {
    	LOG_WRN("picture resource file path null");
    }

    if( text_path != NULL)
    {
		ret = fs_open(&info->text_fp, text_path, FS_O_READ);
	    if ( ret < 0 )
	    {
	        SYS_LOG_ERR(" open text resource file %s failed!", text_path);
	        goto ERR_EXIT;
	    }
    }
    else
    {
    	LOG_WRN("text resource file path null");
    }

    _read_head( info );    //读取style头部信息，不做返回值判断，可能不存在样式文件。

    return info;

ERR_EXIT:

	fs_close(&info->style_fp);
	fs_close(&info->pic_fp);
	fs_close(&info->text_fp);
	mem_free(info);
	return NULL;
}

void res_manager_close_resources( resource_info_t* info )
{
	if(info == NULL)
	{
		SYS_LOG_ERR("null resource info pointer");
		return;
	}

	fs_close(&info->style_fp);
	fs_close(&info->pic_fp);
	fs_close(&info->text_fp);

	if(info->sty_mem != NULL)
	{
		res_mem_free(RES_MEM_POOL_SCENE, info->sty_mem);
		info->sty_mem = NULL;
		info->scenes = NULL;
	}

	if(info->pic_index != NULL)
	{
		res_mem_free(RES_MEM_POOL_BMP, info->pic_index);
		info->pic_index = NULL;
	}

	if(info->sty_path != NULL)
	{
//		mem_free(info->sty_path);
		info->sty_path = NULL;
	}
	mem_free(info);
}

resource_scene_t* _get_scene( resource_info_t* info, unsigned int* scene_item, uint32_t id )
{
	resource_scene_t* scene;
    unsigned int offset;
    unsigned int size;

	//FIXME: scene buffer should differ from bitmap buffer as in space range, 
	//so the mutex wont affect each other.
	os_strace_u32(SYS_TRACE_ID_RES_SCENE_PRELOAD_1, (uint32_t)id);
#if RES_MEM_DEBUG
	if(res_mem_check()<0)
	{
		SYS_LOG_INF("%s %d\n", __FILE__, __LINE__);
		res_mem_dump();
	}
#endif		

	
    offset = *( scene_item + 1 );
    size = *( scene_item + 2 );
	SYS_LOG_INF("sty data %p, scene size %d, offset 0x%x", info->sty_data, size, offset);	
    scene = ( resource_scene_t* )_try_resource_scene_buffer((uint32_t)info, id);
	if(scene != NULL)
	{
//		SYS_LOG_INF("scene cached buffer 0x%x, size 0x%x", scene, size);
//		SYS_LOG_INF("scene %d, %d, %d %d", scene->x, scene->y, scene->width, scene->height);
		os_strace_end_call_u32(SYS_TRACE_ID_RES_SCENE_PRELOAD_1, (uint32_t)id);
		return scene;
	}
	else
	{
		scene = ( resource_scene_t* )_get_resource_scene_buffer((uint32_t)info, id, size);
		if(scene == NULL)
		{
			SYS_LOG_ERR("error: cant get scene buffer");
			return NULL;
		}
	}	
	os_strace_end_call_u32(SYS_TRACE_ID_RES_SCENE_PRELOAD_1, (uint32_t)id);
	os_strace_u32(SYS_TRACE_ID_RES_SCENE_PRELOAD_2, (uint32_t)id);

	memcpy(scene, info->sty_data+offset, size);
	scene->scene_id = id;
	os_strace_end_call_u32(SYS_TRACE_ID_RES_SCENE_PRELOAD_2, (uint32_t)id);
//	SYS_LOG_INF("scene id 0x%x, scene_id 0x%x\n", id, scene->scene_id);
//	SYS_LOG_INF("scene %d, %d, %d %d", scene->x, scene->y, scene->width, scene->height);
	
#if RES_MEM_DEBUG
	if(res_mem_check()<0)
	{
		SYS_LOG_INF("%s %d\n", __FILE__, __LINE__);
		res_mem_dump();
	}
#endif		
	
    return scene;
}

resource_scene_t* res_manager_load_scene(resource_info_t* info, uint32_t scene_id)
{
    unsigned int i;
    resource_scene_t* scenes;

	
    if ( info == NULL )
    {
        SYS_LOG_ERR("resource info null");
        return NULL;
    }
    scenes = info->scenes;
	SYS_LOG_INF("scene id 0x%x, info sum %d", scene_id, info->sum);
    for ( i = 0; i < info->sum; i++ )
    {   	
//    	SYS_LOG_INF("scene_item 0x%x", scenes[i].scene_id);
        if ( scenes[i].scene_id == scene_id )
        {
        	
			return &scenes[i];
        }
    }

    SYS_LOG_ERR("cannot find scene id: 0x%x", scene_id);
    return NULL;
}

void _unload_scene(uint32_t id, resource_scene_t* scene)
{
//	_unload_scene_bitmaps(id);
	
	_free_resource_scene_buffer(scene);
}

void res_manager_unload_scene(uint32_t id, resource_scene_t* scene)
{
	if(scene != NULL)
	{
		_unload_scene(id, scene);
	}
	else if(id > 0)
	{
		//_unload_scene_bitmaps(id);
	}
}

void _unload_bitmap(resource_bitmap_t* bitmap)
{
	if(bitmap == NULL)
	{
		SYS_LOG_ERR("error: unload null resource");
	}
	if(bitmap->regular_info == NULL && bitmap->buffer != NULL)
	{
		_free_resource_bitmap_buffer(bitmap->buffer);
	}
}

void _unload_string(resource_text_t* text)
{
	if(text == NULL)
	{
		SYS_LOG_ERR("error: unload null resource");
		return;
	}
	_free_resource_text_buffer(text->buffer);
}

int32_t _load_bitmap(resource_info_t* info, resource_bitmap_t* bitmap, uint32_t force_ref)
{
	int32_t ret;
	int32_t bmp_size;
	int32_t compress_size = 0;
	uint8_t *compress_buf = NULL;

	if((info == NULL) || (bitmap == NULL))
	{
		SYS_LOG_ERR("error: invalid param to load bitmap");
		return -1;
	}

	os_strace_u32(SYS_TRACE_ID_RES_BMP_LOAD_0, (uint32_t)bitmap->sty_data->id);

#if RES_MEM_DEBUG
	if(res_mem_check()<0)
	{
		SYS_LOG_INF("%s %d\n", __FILE__, __LINE__);
		res_mem_dump();
	}
#endif		

	//bitmap->buffer = _try_resource_bitmap_buffer((uint32_t)info, bitmap->id, &bitmap->width, &bitmap->height, &bitmap->bytes_per_pixel, &bitmap->format, force_ref);
	bitmap->buffer = _try_resource_bitmap_buffer((uint32_t)info, bitmap, force_ref);
	if(bitmap->buffer != NULL)
	{
		//got bitmap from bitmap cache
		os_strace_end_call_u32(SYS_TRACE_ID_RES_BMP_LOAD_0, (uint32_t)bitmap->sty_data->id);
		return 0;
	}
	
	bmp_size = bitmap->sty_data->width * bitmap->sty_data->height * bitmap->sty_data->bytes_per_pixel;
//	SYS_LOG_ERR("styid 0x%x, id %d, wh %d, %d ,bmp_size %d\n", bitmap->sty_data->sty_id, bitmap->sty_data->id, bitmap->sty_data->width, bitmap->sty_data->height, bmp_size);
	bitmap->buffer = _get_resource_bitmap_buffer((uint32_t)info, bitmap, bmp_size, force_ref);
	if(bitmap->buffer == NULL)
	{
		SYS_LOG_ERR("error: no buffer to load bitmap");
		os_strace_end_call_u32(SYS_TRACE_ID_RES_BMP_LOAD_0, (uint32_t)bitmap->sty_data->id);
		return -1;
	}
	os_strace_end_call_u32(SYS_TRACE_ID_RES_BMP_LOAD_0, (uint32_t)bitmap->sty_data->id);	
	os_strace_u32(SYS_TRACE_ID_RES_BMP_LOAD_1, (uint32_t)bitmap->sty_data->id);
//	SYS_LOG_INF("\n pres_entry length %d, type %d, offset 0x%x \n", pres_entry->length, pres_entry->type, pres_entry->offset);
	fs_seek(&info->pic_fp, bitmap->sty_data->bmp_pos, FS_SEEK_SET);

	compress_size = bitmap->sty_data->compress_size;
	if (compress_size > 0)
	{
		ret = 0;
	}
	else
	{
		ret = fs_read(&info->pic_fp, bitmap->buffer, bmp_size);
		os_strace_end_call_u32(SYS_TRACE_ID_RES_BMP_LOAD_1, (uint32_t)bitmap->sty_data->id);	
	}
	
	if(ret < bmp_size)
	{
		SYS_LOG_ERR("bitmap read error %d, compress_buf %p, styid 0x%x, id %d, w %d, h %d, bmp_size %d, buffer %p\n", 
			ret, compress_buf, bitmap->sty_data->sty_id, bitmap->sty_data->id, bitmap->sty_data->width, bitmap->sty_data->height, bmp_size, bitmap->buffer);
	}

	
	
	if(force_ref <= 1)
	{
		mem_dcache_clean(bitmap->buffer, bmp_size);
	}
	
	
#if RES_MEM_DEBUG
	if(res_mem_check()<0)
	{
		SYS_LOG_INF("%s %d\n", __FILE__, __LINE__);
		res_mem_dump();
	}
#endif		

	return 0;
}

int _load_string(resource_info_t* info, resource_text_t* text)
{
	int ret;
    res_head_t res_head;
    res_entry_t res_entry;

	if((info == NULL) || (text == NULL))
	{
		SYS_LOG_INF("error: invalid param to load text");
		return -1;
	}
	text->buffer = _try_resource_text_buffer((uint32_t)info, text->sty_data->id);
	if(text->buffer != NULL)
	{
		//got string buffer from cache
		return 0;
	}

	fs_seek(&info->text_fp, 0, FS_SEEK_SET);
	fs_read(&info->text_fp, &res_head, sizeof(res_head));
    if (res_head.version != 2)
    {
        //support utf8 only for now
        SYS_LOG_ERR("error: unsupported encoding");
        return -1;
    }

    memset(&res_entry, 0, sizeof(res_entry_t));
	fs_seek(&info->text_fp,(int)(text->sty_data->id * (int)sizeof(res_entry_t)) , FS_SEEK_SET);
	ret = fs_read(&info->text_fp, &res_entry, sizeof(res_entry_t));
    if(ret < sizeof(res_entry_t))
    {
    	SYS_LOG_ERR("error: cannot find string resource");
    	return -1;
    }

//	SYS_LOG_INF("offset %d, length %d", res_entry.offset, res_entry.length);
	fs_seek(&info->text_fp, res_entry.offset, FS_SEEK_SET);

    text->buffer = _get_resource_text_buffer((uint32_t)info, text->sty_data->id, res_entry.length+1);
//	SYS_LOG_INF("text buffer 0x%x\n", text->buffer);
    memset(text->buffer, 0, res_entry.length+1);
	ret = fs_read(&info->text_fp, text->buffer, res_entry.length);
	if(ret < res_entry.length)
	{
		SYS_LOG_ERR("error: cannot read string resource");
		return -1;
	}

	return 0;
}


void* res_manager_get_scene_child(resource_info_t* res_info, resource_scene_t* scene, uint32_t id)
{
    uint32_t i;
    resource_t* resource;
	char* buf = NULL;   //地址不对齐，不能用数据类型指针。
	int ret;

    if ( scene == NULL )
    {
    	SYS_LOG_ERR("null scene");
        return NULL;
    }

    buf = (char*)res_info->sty_data + scene->child_offset;
	resource = (resource_t*)buf;

//	SYS_LOG_INF("resource sum %d, child offset %d, LF 0x%x\n", scene->resource_sum, scene->child_offset, id);
    for ( i = 0; i < scene->resource_sum; i++ )
    {
//	   	SYS_LOG_INF("resource id 0x%x, offset 0x%x",resource->id, resource->offset);
        if ( resource->id == id )
        {
            break;
        }

        buf = res_info->sty_data + resource->offset;
		resource = (resource_t*)buf;
    }

    if(i >= scene->resource_sum)
    {
    	SYS_LOG_DBG("error: cannot find resource 0x%x", id);
    	return NULL;
    }

//	SYS_LOG_INF("resource->type 0x%x", resource.type);
    if(resource->type == RESOURCE_TYPE_GROUP)
    {
		//buf is now on group resource start
    	resource_group_t* res = (resource_group_t*)res_array_alloc(RES_MEM_SIMPLE_INNER, sizeof(resource_group_t));
    	if(res == NULL)
    	{
    		SYS_LOG_ERR("error: no space for resource");
    		return NULL;
    	}
		sty_group_t* group = (sty_group_t*)resource;
		res->sty_data = group;		
		return res;
    }

    if(resource->type == RESOURCE_TYPE_PICTURE)
    {
    	//buf is now on resource start
    	resource_bitmap_t* res = (resource_bitmap_t*)res_array_alloc(RES_MEM_SIMPLE_INNER, sizeof(resource_bitmap_t));
    	if(res == NULL)
    	{
    		SYS_LOG_ERR("error: no space for resource");
    		return NULL;
    	}
		memset(res, 0, sizeof(resource_bitmap_t));		
		sty_picture_t* pic = (sty_picture_t*)resource;
		res->sty_data = pic;
		res->regular_info = NULL;
//		SYS_LOG_INF("pic w %d, h %d, id 0x%x", res->width, res->height, res->id);
    	ret = _load_bitmap(res_info, res, 1);
		if(ret < 0)
		{
			res_manager_release_resource(res);
			res = NULL;
		}
		return res;
    }

    if(resource->type == RESOURCE_TYPE_TEXT)
    {
    	//buf is now on resource start
    	resource_text_t* res = (resource_text_t*)res_array_alloc(RES_MEM_SIMPLE_INNER, sizeof(resource_text_t));
    	if(res == NULL)
    	{
    		SYS_LOG_ERR("error: no space for resource");
    		return NULL;
    	}
		memset(res, 0, sizeof(resource_text_t));
		sty_string_t* str = (sty_string_t*)resource;
		res->sty_data = str;
//		SYS_LOG_INF("str size %d, id 0x%x", res->font_size, res->id);
    	ret = _load_string(res_info, res);
		if(ret < 0)
		{
			res_manager_release_resource(res);
			res = NULL;
		}
		return res;
    }

	if(resource->type == RESOURCE_TYPE_PICREGION)
	{
		resource_picregion_t* res = (resource_picregion_t*)res_array_alloc(RES_MEM_SIMPLE_INNER, sizeof(resource_picregion_t));
		if(res == NULL)
		{
    		SYS_LOG_ERR("error: no space for resource");
    		return NULL;
		}
		sty_picregion_t* picreg = (sty_picregion_t*)resource;
		res->sty_data = picreg;
		res->regular_info = NULL;
		return res;
	}
	return NULL;
}

void* res_manager_get_group_child(resource_info_t* res_info, resource_group_t* resgroup, uint32_t id )
{
    unsigned int i;
	resource_t* resource;
	char*buf;
	int ret;

    if ( resgroup == NULL )
    {
        SYS_LOG_ERR("invalid group");
        return NULL;
    }

    buf = (char*)res_info->sty_data + resgroup->sty_data->child_offset;
	resource = (resource_t*)buf;

//	SYS_LOG_INF("resource sum %d, search id 0x%x\n", resgroup->sty_data->resource_sum, id);
    for ( i = 0; i < resgroup->sty_data->resource_sum; i++ )
    {
//    	SYS_LOG_INF("\n ### resource id 0x%x\n", resource->id);
        if ( resource->id == id )
        {
            break;
        }

        buf = res_info->sty_data + resource->offset;
		resource = (resource_t*)buf;
    }

    if(i >= resgroup->sty_data->resource_sum)
    {
    	SYS_LOG_ERR("cant find resource 0x%x", id);
    	return NULL;
    }

//	SYS_LOG_INF("resource.type 0x%x", resource.type);
    if(resource->type == RESOURCE_TYPE_GROUP)
    {
		//buf is now on group resource start
    	resource_group_t* res = (resource_group_t*)res_array_alloc(RES_MEM_SIMPLE_INNER, sizeof(resource_group_t));
    	if(res == NULL)
    	{
    		SYS_LOG_ERR("error: no space for resource");
    		return NULL;
    	}
		memset(res, 0, sizeof(resource_group_t));
		sty_group_t* group = (sty_group_t*)resource;
		res->sty_data = group;
		return res;

    }

    if(resource->type == RESOURCE_TYPE_PICTURE)
    {
    	//buf is now on resource start
    	resource_bitmap_t* res = (resource_bitmap_t*)res_array_alloc(RES_MEM_SIMPLE_INNER, sizeof(resource_bitmap_t));
    	if(res == NULL)
    	{
    		SYS_LOG_ERR("error: no space for resource");
    		return NULL;
    	}
		memset(res, 0, sizeof(resource_bitmap_t));		
		sty_picture_t* pic = (sty_picture_t*)resource;
		res->sty_data = pic;
//		SYS_LOG_INF("pic w %d, h %d, id 0x%x", res->width, res->height, res->id);
		res->regular_info = NULL;
    	ret = _load_bitmap(res_info, res, 1);
		if(ret < 0)
		{
			res_manager_release_resource(res);
			res = NULL;
		}
		return res;
    }

    if(resource->type == RESOURCE_TYPE_TEXT)
    {
    	//buf is now on resource start
    	resource_text_t* res = (resource_text_t*)res_array_alloc(RES_MEM_SIMPLE_INNER, sizeof(resource_text_t));
    	if(res == NULL)
    	{
    		SYS_LOG_ERR("error: no space for resource");
    		return NULL;
    	}
		memset(res, 0, sizeof(resource_text_t));
		sty_string_t* str = (sty_string_t*)resource;
		res->sty_data = str;
//		SYS_LOG_INF("str size %d, id 0x%x", res->font_size, res->id);
    	ret = _load_string(res_info, res);
		if(ret < 0)
		{
			res_manager_release_resource(res);
			res = NULL;
		}

		return res;
    }

	if(resource->type == RESOURCE_TYPE_PICREGION)
	{
		resource_picregion_t* res = (resource_picregion_t*)res_array_alloc(RES_MEM_SIMPLE_INNER, sizeof(resource_picregion_t));
		if(res == NULL)
		{
    		SYS_LOG_ERR("error: no space for resource");
    		return NULL;
		}
		sty_picregion_t* picreg = (sty_picregion_t*)resource;
		res->sty_data = picreg;
		res->regular_info = NULL;
		return res;
	}
	return NULL;
}

resource_bitmap_t* res_manager_load_frame_from_picregion(resource_info_t* info, resource_picregion_t* picreg, uint32_t frame)
{
	resource_bitmap_t* bitmap;
	uint8_t* buf;
	int32_t ret;

	if(info == NULL || picreg == NULL)
	{
        SYS_LOG_ERR("invalid picreg");
        return NULL;
	}

	bitmap = (resource_bitmap_t*)res_array_alloc(RES_MEM_SIMPLE_INNER, sizeof(resource_bitmap_t));
	if(bitmap == NULL)
	{
        SYS_LOG_ERR("no space for frame loading");
        return NULL;
	}
	buf = info->sty_data + picreg->sty_data->id_offset + sizeof(sty_picture_t)*frame;
	bitmap->sty_data = (sty_picture_t*)buf;
	bitmap->regular_info = picreg->regular_info;
	ret = _load_bitmap(info, bitmap, 1);
	if(ret < 0)
	{
		res_manager_release_resource(bitmap);
		bitmap = NULL;
	}
	return bitmap;
}


void res_manager_release_resource(void* resource)
{
	uint32_t type = *((uint32_t*)(*((uint32_t*)(resource))));

	if(type == RESOURCE_TYPE_GROUP)
	{
		res_array_free(resource);
	}
	else if(type == RESOURCE_TYPE_PICTURE)
	{
		resource_bitmap_t* bitmap = (resource_bitmap_t*)resource;
		_unload_bitmap(bitmap);
		res_array_free(resource);
	}
	else if(type == RESOURCE_TYPE_TEXT)
	{
		resource_text_t* text = (resource_text_t*)resource;
		_unload_string(text);
		res_array_free(resource);
	}
	else if(type == RESOURCE_TYPE_PICREGION)
	{
		resource_picregion_t* picreg = (resource_picregion_t*)resource;
		res_array_free(picreg);
	}
}

void res_manager_free_resource_structure(void* resource)
{
	if(resource != NULL)
	{
		res_array_free(resource);
	}
}

void res_manager_free_bitmap_data(void* data)
{
	if(data != NULL)
	{
		_free_resource_bitmap_buffer(data);
	}
}

void res_manager_free_text_data(void* data)
{
	if(data != NULL)
	{
		_free_resource_text_buffer(data);
	}
}

int32_t res_manager_preload_bitmap(resource_info_t* res_info, resource_bitmap_t* bitmap)
{
	int32_t ret;
	ret = _load_bitmap(res_info, bitmap, 0);
	if(ret < 0)
	{
		SYS_LOG_ERR("preload bitmap error 0x%x", bitmap->sty_data->id);
		return -1;
	}
	else
	{
		//SYS_LOG_INF("preload success 0x%x\n", bitmap->sty_data->id);
		return 0;
	}

}



int32_t res_manager_load_bitmap(resource_info_t* res_info, resource_bitmap_t* bitmap)
{
	int32_t ret;

	ret = _load_bitmap(res_info, bitmap, 1);
	if(ret < 0)
	{
		SYS_LOG_ERR("load bitmap error 0x%x", bitmap->sty_data->sty_id);
		return -1;
	}
	else
	{
		return 0;
	}

}



