#include <os_common_api.h>
#include <mem_manager.h>
#include <string.h>
#include "res_manager_api.h"

#ifdef CONFIG_RES_MANAGER_USE_SDFS
#include <sdfs.h>
#endif

#ifndef FS_O_READ
#define FS_O_READ		0x01
#define FS_SEEK_SET     SEEK_SET
#define FS_SEEK_END     SEEK_END
#define off_t			uint32_t
#define ssize_t 		int32_t
#endif


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


#ifndef CONFIG_RES_MEM_POOL_SCENE_MAX_BLOCK_NUMBER
#define RES_SCENE_POOL_MAX_BLOCK_NUMBER		160 //128 //96
#else
#define RES_SCENE_POOL_MAX_BLOCK_NUMBER		CONFIG_RES_MEM_POOL_SCENE_MAX_BLOCK_NUMBER
#endif
#define RES_TXT_POOL_MAX_BLOCK_NUMBER		10

#ifndef CONFIG_RES_MEM_POOL_MAX_BLOCK_NUMBER
#define RES_MEM_POOL_MAX_BLOCK_NUMBER 		2370
#else
#define RES_MEM_POOL_MAX_BLOCK_NUMBER		(CONFIG_RES_MEM_POOL_MAX_BLOCK_NUMBER-RES_TXT_POOL_MAX_BLOCK_NUMBER-RES_SCENE_POOL_MAX_BLOCK_NUMBER)
#endif
#define RES_MEM_POOL_MAX_BLOCK_SIZE      	1024
#define RES_MEM_POOL_MIN_BLOCK_SIZE      	16
#define RES_MEM_POOL_ALIGN_SIZE         	8

#define RES_ARRAY_MEM_DEBUG		1
#define RES_MEM_DEBUG			0
#if RES_MEM_DEBUG
#define DEBUG_PATTERN_SIZE		16
#endif

//K_HEAP_DEFINE(freetype_mem_pool, RES_MEM_POOL_MAX_BLOCK_NUMBER * RES_MEM_POOL_MAX_BLOCK_SIZE);
#ifndef CONFIG_SIMULATOR
static char __aligned(4) res_mem_pool_buffer[RES_MEM_POOL_MAX_BLOCK_NUMBER * RES_MEM_POOL_MAX_BLOCK_SIZE] __in_section_unique(RES_PSRAM_REGION);

STRUCT_SECTION_ITERABLE(k_heap, res_mem_pool) = {
				.heap = {
					.init_mem = res_mem_pool_buffer,
					.init_bytes = RES_MEM_POOL_MAX_BLOCK_NUMBER * RES_MEM_POOL_MAX_BLOCK_SIZE,
					},
};

static char __aligned(4) res_txt_pool_buffer[RES_TXT_POOL_MAX_BLOCK_NUMBER * RES_MEM_POOL_MAX_BLOCK_SIZE] __in_section_unique(RES_PSRAM_REGION);

STRUCT_SECTION_ITERABLE(k_heap, res_txt_pool) = {
				.heap = {
					.init_mem = res_txt_pool_buffer,
					.init_bytes = RES_TXT_POOL_MAX_BLOCK_NUMBER * RES_MEM_POOL_MAX_BLOCK_SIZE,
					},
};

static char __aligned(4) res_scene_pool_buffer[RES_SCENE_POOL_MAX_BLOCK_NUMBER * RES_MEM_POOL_MAX_BLOCK_SIZE] __in_section_unique(RES_PSRAM_REGION);

STRUCT_SECTION_ITERABLE(k_heap, res_scene_pool) = {
				.heap = {
					.init_mem = res_scene_pool_buffer,
					.init_bytes = RES_SCENE_POOL_MAX_BLOCK_NUMBER * RES_MEM_POOL_MAX_BLOCK_SIZE,
					},
};
#endif

typedef struct _mem_info
{
	void* ptr;
	size_t size;
	uint32_t type;
	struct _mem_info* next;
}mem_info_t;

static mem_info_t* mem_info_list[RES_MEM_POOL_TYPE_MAX];

static size_t mem_total[RES_MEM_POOL_TYPE_MAX];

static uint32_t* array_bitmap[RES_MEM_SIMPLE_TYPE_MAX];
static uint8_t* array_mem[RES_MEM_SIMPLE_TYPE_MAX];
static uint32_t array_unit_size[RES_MEM_SIMPLE_TYPE_MAX];
static uint8_t* array_offset[RES_MEM_SIMPLE_TYPE_MAX];
#ifdef RES_ARRAY_MEM_DEBUG
static uint32_t array_item_total[RES_MEM_SIMPLE_TYPE_MAX];
#endif


void *res_mem_alloc(uint32_t type, size_t size);
void res_mem_free(uint32_t type, void *ptr);
void res_mem_dump(void);

void res_mem_init(void)
{
	memset(mem_info_list ,0, sizeof(mem_info_t*)*RES_MEM_POOL_TYPE_MAX);
	memset(mem_total, 0, sizeof(size_t)*RES_MEM_POOL_TYPE_MAX);

	memset(array_bitmap, 0, sizeof(uint32_t*)*RES_MEM_SIMPLE_TYPE_MAX);
	memset(array_mem, 0, sizeof(uint8_t*)*RES_MEM_SIMPLE_TYPE_MAX);
#ifndef CONFIG_SIMULATOR
	os_printk("checkmem res_mem_pool_buffer %p\n", res_mem_pool_buffer);
	os_printk("checkmem res_txt_pool_buffer %p\n", res_txt_pool_buffer);
	os_printk("checkmem res_scene_pool_buffer %p\n", res_scene_pool_buffer);
#endif
#ifdef RES_ARRAY_MEM_DEBUG
	memset(array_item_total, 0, sizeof(uint32_t)*RES_MEM_SIMPLE_TYPE_MAX);
#endif
}

void res_mem_deinit(void)
{
#ifndef CONFIG_SIMULATOR
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
#endif
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
	case RES_MEM_SIMPLE_PRELOAD:
		return sizeof(preload_param_t);
	case RES_MEM_SIMPLE_COMPACT:
		return sizeof(compact_buffer_t);
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
#ifdef CONFIG_SIMULATOR
	return malloc(size);
#else
	uint8_t* ptr;
	uint32_t bit_offset;
	uint32_t i;
	uint32_t mask;
	uint32_t array_bitmap_size = _get_simple_bitmap_size(type);

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

	if(size > array_unit_size[type])
	{
		SYS_LOG_ERR("invalid array alloc type %d, size %d, unit size %d\n", type, size, array_unit_size[type]);
		return NULL;
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

	SYS_LOG_INF("ptr 0x%x, array_mem 0x%x, array_offset 0x%x\n", ptr, array_mem[type], array_offset[type]);

	if(res_mem_check()<0)
	{
		SYS_LOG_ERR("%s %d ptr 0x%x, array_mem 0x%x, array_offset 0x%x\n", __FILE__, __LINE__, ptr, array_mem[type], array_offset[type]);
		res_mem_dump();
	}
#endif

#ifdef RES_ARRAY_MEM_DEBUG
	array_item_total[type]++;
#endif
	return ptr;
#endif	
}

uint32_t res_array_free(void* ptr)
{
#ifdef CONFIG_SIMULATOR
	free(ptr);
	return 0;
#else
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
		SYS_LOG_ERR("unknown array type for %p\n", ptr);
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
			SYS_LOG_INF("invalid array pos when free type %d, ptr %p, array mem %p\n", type, ptr, array_mem[type]);
		}
	}
#endif	
	return 0;
}

void _add_mem_info(uint32_t type, void* ptr, size_t size)
{
	mem_info_t* prev = NULL;
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
	prev = mem_info_list[type];
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
		SYS_LOG_ERR("testalloc get mem size not found %p\n", ptr);
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
#ifndef CONFIG_SIMULATOR
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
#endif
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
			printk("\n check item 0x%x, ptr 0x%x, size 0x%x \n head:", item, check, item->size);
			for(i=0;i<DEBUG_PATTERN_SIZE;i++)
			{
				printk("0x%x, ", check[i]);
			}
			printk("\n");
			check = item->ptr + item->size - DEBUG_PATTERN_SIZE;
			printk(" tail:");
			for(i=0;i<DEBUG_PATTERN_SIZE;i++)
			{
				printk("0x%x, ", check[i]);
			}
			printk("\n");

			item = item->next;
		}
	}
#endif
}

void *res_mem_alloc(uint32_t type, size_t size)
{
#ifdef CONFIG_SIMULATOR
	return malloc(size);
#else
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
#endif
}

void res_mem_free(uint32_t type, void *ptr)
{
#ifdef CONFIG_SIMULATOR
	free(ptr);
#else
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
#endif
}

void *res_mem_realloc(uint32_t type, void *ptr, size_t requested_size)
{
	void *new_ptr;
	size_t copy_size;

	if(type >= RES_MEM_POOL_TYPE_MAX)
	{
		SYS_LOG_ERR("invalid res mem type to realloc\n");
		return NULL;
	}

#if RES_MEM_DEBUG
//	SYS_LOG_INF("testalloc ptr 0x%x realptr 0x%x, realloc %d\n",ptr, ptr-DEBUG_PATTERN_SIZE, requested_size);
#else
//	SYS_LOG_INF("testalloc type %d, ptr %p, realloc %d\n",type, ptr, requested_size);
#endif

	if (ptr == NULL) {
		return res_mem_alloc(type, requested_size);
	}

	if (requested_size == 0) {
		res_mem_free(type, ptr);
		return NULL;
	}

	new_ptr = res_mem_alloc(type, requested_size);
	if (new_ptr == NULL) {
		return NULL;
	}

	copy_size = _get_mem_size(type, ptr);
	if (copy_size > requested_size)
	{
		copy_size = requested_size;
	}
	memcpy(new_ptr, ptr, copy_size);

	res_mem_free(type, ptr);
	return new_ptr;
}

int res_is_auto_search_files(void)
{
#ifndef CONFIG_RES_MANAGER_DISABLE_AUTO_SEARCH_FILES
	return 1; 
#else
	return 0;
#endif
}

void res_mem_info(uint32_t type, uint32_t user_data)
{
#ifndef CONFIG_SIMULATOR
	mem_info_t* listp;
	uint32_t count=0;

	if(type >= RES_MEM_POOL_TYPE_MAX)
	{
		SYS_LOG_ERR("invalid res mem type to dump\n");
		return;
	}

	listp = mem_info_list[type];
	while(listp)
	{
		count++;
		listp = listp->next;
	}
#endif

	SYS_LOG_INF("\n res mem info data 0x%x, type %d, total_size %d\n", user_data, type, mem_total[type]);
}

int res_fs_open(void* handle, const char* path)
{
#ifdef CONFIG_RES_MANAGER_USE_SDFS
	struct sd_file** sdh = (struct sd_file**)handle;
	*sdh = sd_fopen(path);
	if(*sdh == NULL)
	{
		return -1;
	}
	else
	{
		return 0;
	}
#else
	int ret;
#ifdef CONFIG_SIMULATOR
	void** fath = (void**)handle;
#else
	struct fs_file_t* fath = (struct fs_file_t*)handle;
#endif
	ret = fs_open(fath, path, FS_O_READ);
	return ret;
#endif
}

void res_fs_close(void* handle)
{
#ifdef CONFIG_RES_MANAGER_USE_SDFS
	struct sd_file** sdh = (struct sd_file**)handle;
	if(*sdh == NULL)
	{
		SYS_LOG_ERR("Null file handle to close\n");
		return;
	}	
	sd_fclose(*sdh);
	*sdh = NULL;
#else
#ifdef CONFIG_SIMULATOR
	void** fath = (void**)handle;
#else
	struct fs_file_t* fath = (struct fs_file_t*)handle;
#endif
	fs_close(fath);	
#endif
}

int res_fs_seek(void* handle, off_t where, int whence)
{
	int ret;
#ifdef CONFIG_RES_MANAGER_USE_SDFS
	struct sd_file** sdh = (struct sd_file**)handle;
	if(*sdh == NULL)
	{
		SYS_LOG_ERR("Null file handle to seek\n");
		return -1;
	}
	ret = sd_fseek(*sdh, where, (unsigned char)whence);
#else
#ifdef CONFIG_SIMULATOR
	void** fath = (void**)handle;
#else
	struct fs_file_t* fath = (struct fs_file_t*)handle;
#endif
	ret = fs_seek(fath, where, whence);
#endif
	return ret;
}

off_t res_fs_tell(void* handle)
{
	off_t ret;
#ifdef CONFIG_RES_MANAGER_USE_SDFS
	struct sd_file** sdh = (struct sd_file**)handle;
	if(*sdh == NULL)
	{
		SYS_LOG_ERR("Null file handle to tell\n");
		return 0;
	}	
	ret = sd_ftell(*sdh);
#else
#ifdef CONFIG_SIMULATOR
	void** fath = (void**)handle;
#else
	struct fs_file_t* fath = (struct fs_file_t*)handle;
#endif
	ret = fs_tell(fath);
#endif
	return ret;
}

ssize_t res_fs_read(void* handle, void* buffer, size_t len)
{
	ssize_t ret;
#ifdef CONFIG_RES_MANAGER_USE_SDFS
	struct sd_file** sdh = (struct sd_file**)handle;
	if(*sdh == NULL)
	{
		SYS_LOG_ERR("Null file handle to read\n");
		return 0;
	}	
	ret = (ssize_t)sd_fread(*sdh, buffer, len);
#else
#ifdef CONFIG_SIMULATOR
	void** fath = (void**)handle;
#else
	struct fs_file_t* fath = (struct fs_file_t*)handle;
#endif
	ret = fs_read(fath, buffer, len);
#endif
	return ret;
}



