#include <diskio.h>
#include <disk/disk_access.h>
#include <ffconf.h>
#include <errno.h>
#include <string.h>
#include <init.h>
#include <os_common_api.h>
#include <mem_manager.h>
#include <memory/mem_cache.h>

#define SYS_LOG_DOMAIN "diskio_cache"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_INFO
/*#include <logging/sys_log.h>*/

#define DISKIO_TIMEOUT OS_FOREVER
#define DISKIO_CACHE_POOL_NUM 1

#define DISKIO_CACHE_POOL_SIZE 2048

#define DISKIO_CACHE_STACK_SIZE 1536

#define DISKIO_DIRECT_READ		0


/*static char __in_section_unique(diskio.cache.stack)__aligned(STACK_ALIGN) diskio_cache_thread_stack[1152];*/
static OS_THREAD_STACK_DEFINE(diskio_cache_thread_stack, DISKIO_CACHE_STACK_SIZE);


OS_MUTEX_DEFINE(diskio_cache_mutex);

struct  diskio_cache_item {
	u32_t cache_valid:1;
	u32_t busy_flag:1;
	u32_t write_valid:1;
	u32_t err_flag:1;
	u32_t disk_type:8;
	u32_t cache_sector;
	struct disk_info *disk;
	const char *pdrv;
	u8_t  cache_data[DISKIO_CACHE_POOL_SIZE];
};

enum {
	REQ_FLUSH,
	REQ_LOAD,
};

struct  diskio_cache_req {
	void *fifo_reserved;
	struct disk_info *req_disk;
	u8_t  req_type;
	u32_t req_sector;
	os_sem req_sem;
	const char *pdrv;
};

struct  diskio_cache_context {
	os_fifo cache_req_fifo;
	u32_t terminal:1;
	u32_t inited:1;
	u32_t cache_index:8;
	u32_t thread_id;
	struct  diskio_cache_item cache_pool[DISKIO_CACHE_POOL_NUM];
};

#ifdef CONFIG_FIXED_DMA_ACCESS_PSRAM
struct  diskio_cache_context diskio_cache __aligned(4)__in_section_unique(diskio.cache.pool);
#else
struct  diskio_cache_context diskio_cache __aligned(4);
#endif

static struct  diskio_cache_item *_diskio_find_cache_item(struct disk_info *disk, DWORD sector, UINT count)
{
	struct  diskio_cache_item *cache_item = NULL;
	u8_t cace_sector_cnt = DISKIO_CACHE_POOL_SIZE / disk->sector_size;

	for (int i = 0; i < DISKIO_CACHE_POOL_NUM; i++) {
		if (diskio_cache.cache_pool[i].cache_sector <= sector
			&& diskio_cache.cache_pool[i].disk == disk
			&& diskio_cache.cache_pool[i].cache_sector + cace_sector_cnt >= sector + count
			&& diskio_cache.cache_pool[i].cache_valid == 1){
			cache_item = &diskio_cache.cache_pool[i];
		}
	}

	return cache_item;
}

static int _diskio_cache_invalid(const char *pdrv, struct disk_info *disk, DWORD sector, UINT count)
{
	struct  diskio_cache_item *cache_item = NULL;
	u8_t cace_sector_cnt = DISKIO_CACHE_POOL_SIZE / disk->sector_size;
	u8_t cache_index = 0;

	for (int i = 0; i < DISKIO_CACHE_POOL_NUM; i++) {
		if (diskio_cache.cache_pool[i].cache_sector  +  cace_sector_cnt >= sector
			&& diskio_cache.cache_pool[i].cache_sector < sector + count
			&& diskio_cache.cache_pool[i].disk == disk
			&& diskio_cache.cache_pool[i].cache_valid == 1){
			cache_item = &diskio_cache.cache_pool[i];
			if (cache_item->write_valid) {
				if (disk_access_write(pdrv, cache_item->cache_data, cache_item->cache_sector,
							cace_sector_cnt)) {
					SYS_LOG_ERR("sector %d len %d\n", cache_item->cache_sector, cace_sector_cnt);
				}
			}
			cache_item->cache_valid = 0;
			cache_item->write_valid = 0;
			cache_index = i;
		}
	}
	return cache_index;
}

static struct  diskio_cache_item *_diskio_new_cache_item(const char *pdrv, struct disk_info *disk, DWORD sector)
{
	struct  diskio_cache_item *cache_item = NULL;
	u8_t cace_sector_cnt = DISKIO_CACHE_POOL_SIZE / disk->sector_size;

	os_mutex_lock(&diskio_cache_mutex, OS_FOREVER);
	cache_item = &diskio_cache.cache_pool[diskio_cache.cache_index++];
	diskio_cache.cache_index = diskio_cache.cache_index % DISKIO_CACHE_POOL_NUM;

	if (cache_item->write_valid && cache_item->cache_valid) {
		if (disk_access_write(pdrv, cache_item->cache_data, cache_item->cache_sector,
					cace_sector_cnt)) {
			SYS_LOG_ERR("sector %d len %d\n", cache_item->cache_sector, cace_sector_cnt);
		}

	}

	cache_item->cache_valid = 1;
	cache_item->cache_sector = sector;
	cache_item->disk = disk;
	cache_item->write_valid = 0;
	cache_item->err_flag = 0;
	cache_item->busy_flag = 0;
	os_mutex_unlock(&diskio_cache_mutex);
	return cache_item;
}

static int _diskio_load_to_cache_req(const char *pdrv, struct disk_info *disk, DWORD sector)
{
	struct  diskio_cache_req cache_req;

	memset(&cache_req, 0, sizeof(cache_req));
	os_sem_init(&cache_req.req_sem, 0, 1);

	cache_req.pdrv = pdrv;
	cache_req.req_disk = disk;
	cache_req.req_sector = sector;
	cache_req.req_type = REQ_LOAD;

	os_fifo_put(&diskio_cache.cache_req_fifo, &cache_req);

	if (os_sem_take(&cache_req.req_sem, DISKIO_TIMEOUT) == -EAGAIN) {
		SYS_LOG_ERR("timeout");
		return -EAGAIN;
	}
	return 0;
}

static int _diskio_flush_cache_req(const char *pdrv)
{
	struct disk_info *disk = disk_access_get_di(pdrv);

	if ((disk == NULL) || (disk->ops == NULL))
		return -EINVAL;

	struct diskio_cache_req cache_req;

	memset(&cache_req, 0, sizeof(cache_req));
	os_sem_init(&cache_req.req_sem, 0, 1);

	cache_req.pdrv = pdrv;
	cache_req.req_disk = disk;
	cache_req.req_type = REQ_FLUSH;

	os_fifo_put(&diskio_cache.cache_req_fifo, &cache_req);

	if (os_sem_take(&cache_req.req_sem, DISKIO_TIMEOUT) == -EAGAIN) {
		SYS_LOG_ERR("timeout");
		return -EAGAIN;
	}
	return 0;
}

static void _diskio_cache_timeout_flush(void)
{
	int ret = 0;

	os_mutex_lock(&diskio_cache_mutex, OS_FOREVER);

	for (int i = 0; i < DISKIO_CACHE_POOL_NUM; i++) {
		if (diskio_cache.cache_pool[i].cache_valid == 1
			&& diskio_cache.cache_pool[i].write_valid == 1){
			struct  diskio_cache_item *cache_item = &diskio_cache.cache_pool[i];

			ret = disk_access_write(cache_item->pdrv, cache_item->cache_data,
						cache_item->cache_sector,
						DISKIO_CACHE_POOL_SIZE / cache_item->disk->sector_size);

			if (ret) {
				SYS_LOG_ERR("sector %d len %d\n", cache_item->cache_sector, DISKIO_CACHE_POOL_SIZE);
			} else {
				cache_item->write_valid = 0;
			}
			printk("diskio cache timeout flush\n");
		}
	}
	os_mutex_unlock(&diskio_cache_mutex);
}

int diskio_cache_read(
	const char *pdrv,
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,		/* Start sector in LBA */
	UINT count		/* Number of sectors to read */)
{
	int ret = 0;
	struct  diskio_cache_item *cache_item = NULL;
	struct disk_info *disk = disk_access_get_di(pdrv);
	int cache_index = 0;

	if ((disk == NULL) || (disk->ops == NULL))
		return -EINVAL;

	if (!diskio_cache.inited) {
		return disk_access_read(pdrv, buff, sector, count);
	}
	
	if (DISKIO_DIRECT_READ || (count * disk->sector_size >= DISKIO_CACHE_POOL_SIZE)) {
		cache_index = _diskio_cache_invalid(pdrv, disk, sector, count);

		if (mem_is_cacheable(buff)) {
			UINT sum_read_count = 0;
			UINT read_count = 0;

			cache_item = &diskio_cache.cache_pool[cache_index];
			/*need to clear cache item*/
			cache_item->cache_valid = 0;
			cache_item->write_valid = 0;

			while (sum_read_count < count) {
				read_count = ((count - sum_read_count) > DISKIO_CACHE_POOL_SIZE / disk->sector_size) ?
					DISKIO_CACHE_POOL_SIZE / disk->sector_size : (count - sum_read_count);

				ret = disk_access_read(pdrv, cache_item->cache_data, sector + sum_read_count, read_count);
				memcpy(buff + sum_read_count * disk->sector_size, cache_item->cache_data, read_count * disk->sector_size);
				sum_read_count += read_count;
				mem_dcache_clean_all();
			}
		} else {
			ret = disk_access_read(pdrv, buff, sector, count);
		}

		return ret;
	}

try_to_read:
	os_mutex_lock(&diskio_cache_mutex, OS_FOREVER);
	cache_item = _diskio_find_cache_item(disk, sector, count);
	/**cache hit */
	if (cache_item) {
		cache_item->busy_flag = 1;
		if (!cache_item->err_flag) {
			memcpy(buff, cache_item->cache_data
					+ (sector - cache_item->cache_sector) * disk->sector_size,
					count * disk->sector_size);
			ret = 0;
		} else {
			cache_item->cache_valid = 0;
			ret = -EIO;
		}
		cache_item->busy_flag = 0;
	}
	os_mutex_unlock(&diskio_cache_mutex);
	/**cache miss */
	if (!cache_item) {
		_diskio_load_to_cache_req(pdrv, disk, sector);
		goto try_to_read;
	}

	return ret;
}

int diskio_cache_write(
	const char *pdrv,
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count		/* Number of sectors to write */)
{
	int ret = 0;
	struct  diskio_cache_item *cache_item = NULL;
	struct disk_info *disk = disk_access_get_di(pdrv);
	int cache_index = 0;

	if ((disk == NULL) || (disk->ops == NULL))
		return -EINVAL;

	if (!diskio_cache.inited) {
		return disk_access_write(pdrv, buff, sector, count);
	}
	
	if (count * disk->sector_size >= DISKIO_CACHE_POOL_SIZE) {
		cache_index = _diskio_cache_invalid(pdrv, disk, sector, count);

		if (mem_is_cacheable(buff)) {
			UINT sum_write_count = 0;
			UINT write_count = 0;

			cache_item = &diskio_cache.cache_pool[cache_index];
			/*need to clear cache item*/
			cache_item->cache_valid = 0;
			cache_item->write_valid = 0;

			while (sum_write_count < count) {
				write_count = ((count - sum_write_count) > DISKIO_CACHE_POOL_SIZE / disk->sector_size) ?
					DISKIO_CACHE_POOL_SIZE / disk->sector_size : (count - sum_write_count);

				memcpy(cache_item->cache_data, buff + sum_write_count * disk->sector_size, write_count * disk->sector_size);

				ret = disk_access_write(pdrv, cache_item->cache_data, sector + sum_write_count, write_count);
				sum_write_count += write_count;
			}
		} else {
			ret = disk_access_write(pdrv, buff, sector, count);
		}

		return ret;
	}

try_to_write:
	os_mutex_lock(&diskio_cache_mutex, OS_FOREVER);

	cache_item = _diskio_find_cache_item(disk, sector, count);
	/**cache hit */
	if (cache_item) {
		cache_item->busy_flag = 1;
		if (!cache_item->err_flag) {
			memcpy(cache_item->cache_data
					+ (sector - cache_item->cache_sector) * disk->sector_size,
					buff, count * disk->sector_size);
			cache_item->write_valid = 1;
			cache_item->pdrv = pdrv;
			ret = 0;
		} else {
			cache_item->cache_valid = 0;
			ret = -EIO;
		}
		cache_item->busy_flag = 0;
	}

	os_mutex_unlock(&diskio_cache_mutex);

	/**cache miss */
	if (!cache_item) {
		_diskio_load_to_cache_req(pdrv, disk, sector);
		goto try_to_write;
	}

	return ret;
}

int diskio_cache_flush(const char *pdrv)
{
	return _diskio_flush_cache_req(pdrv);
}

int diskio_cache_invalid(const char *pdrv)
{
	struct  diskio_cache_item *cache_item = NULL;
	struct disk_info *disk = disk_access_get_di(pdrv);

	if ((disk == NULL) || (disk->ops == NULL))
		return -EINVAL;

	os_mutex_lock(&diskio_cache_mutex, OS_FOREVER);
	for (int i = 0; i < DISKIO_CACHE_POOL_NUM; i++) {
		if (diskio_cache.cache_pool[i].disk == disk
			&& diskio_cache.cache_pool[i].cache_valid == 1) {
			cache_item = &diskio_cache.cache_pool[i];
			cache_item->cache_valid = 0;
			cache_item->write_valid = 0;
		}
	}
	os_mutex_unlock(&diskio_cache_mutex);
	return 0;
}

static void _diskio_cache_thread_loop(void *p1, void *p2, void *p3)
{
	int ret = 0;
	struct  diskio_cache_context *diskio_cache_ctx = (struct  diskio_cache_context *)p1;

	while (!diskio_cache_ctx->terminal) {
		struct  diskio_cache_req  *cache_req = NULL;
		struct  diskio_cache_item *cache_item  = NULL;
		struct disk_info *disk = NULL;

		cache_req = os_fifo_get(&diskio_cache.cache_req_fifo, CONFIG_DISKIO_CACHE_TIMEOUT);
		if (!cache_req) {
			_diskio_cache_timeout_flush();
			continue;
		}
		os_mutex_lock(&diskio_cache_mutex, OS_FOREVER);
		switch (cache_req->req_type) {
		case REQ_LOAD:
		{
			cache_item = _diskio_new_cache_item(cache_req->pdrv, cache_req->req_disk, cache_req->req_sector);
			disk = cache_req->req_disk;
			if (cache_item) {
				ret = disk_access_read(cache_req->pdrv, cache_item->cache_data, cache_req->req_sector,
									 DISKIO_CACHE_POOL_SIZE / disk->sector_size);
				if (ret) {
					cache_item->err_flag = 1;
				}
				cache_item->cache_valid = 1;
			}

			break;
		}
		case REQ_FLUSH:
		{
			for (int i = 0; i < DISKIO_CACHE_POOL_NUM; i++) {
				if (diskio_cache.cache_pool[i].disk == cache_req->req_disk
					&& diskio_cache.cache_pool[i].cache_valid == 1
					&& diskio_cache.cache_pool[i].write_valid == 1) {

					cache_item = &diskio_cache.cache_pool[i];
					disk = cache_req->req_disk;

					ret = disk_access_write(cache_req->pdrv, cache_item->cache_data,
								cache_item->cache_sector,
								DISKIO_CACHE_POOL_SIZE / disk->sector_size);

					if (ret) {
						SYS_LOG_ERR("sector %d len %d\n", cache_item->cache_sector, DISKIO_CACHE_POOL_SIZE);
					} else {
						cache_item->write_valid = 0;
					}
				}
			}

			break;
		}
		default:
			break;
		}
		os_mutex_unlock(&diskio_cache_mutex);
		os_sem_give(&cache_req->req_sem);
	}
}
int diskio_cache_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	memset(&diskio_cache, 0, sizeof(struct diskio_cache_context));

	os_fifo_init(&diskio_cache.cache_req_fifo);

	diskio_cache.thread_id = os_thread_create((char *)diskio_cache_thread_stack,
											DISKIO_CACHE_STACK_SIZE,
											_diskio_cache_thread_loop,
											&diskio_cache, NULL, NULL,
											5, 0, OS_NO_WAIT);

	os_thread_name_set((struct k_thread *)diskio_cache.thread_id, "diskio_cache");

	diskio_cache.inited = 1;

	return 0;
}

int diskio_cache_deinit(void)
{
	memset(&diskio_cache, 0, sizeof(struct  diskio_cache_item));
	return 0;
}

SYS_INIT(diskio_cache_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);


