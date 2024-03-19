/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA SDCARD backend interface
 */

#include <kernel.h>
#include <string.h>
#include <drivers/flash.h>
#include <partition/partition.h>
#include <mem_manager.h>
#include <ota_backend.h>
#include <ota_backend_temp_part.h>
#include <os_common_api.h>
#include <drivers/mmc/sd.h>

enum ota_backend_temp_type {
	OTA_BACKEND_SPINOR,
	OTA_BACKEND_SD,
	OTA_BACKEND_NAND,
};

#define OTA_BACKEND_SD_SECTOR_SIZE (512)
#define CONFIG_MMC_SDCARD_DEV_NAME "sd"
#define CONFIG_SPINAND_DEV_NAME "spinand"

struct ota_backend_temp_part {
	struct ota_backend backend;
	const struct device *dev;
	const char *dev_name;
	uint32_t offset;
	int size;
	int type; // refer to enum ota_backend_temp_type
};

int ota_backend_temp_part_read(struct ota_backend *backend, int offset, uint8_t *buf, int size)
{
	struct ota_backend_temp_part *ob_tp = CONTAINER_OF(backend,
		struct ota_backend_temp_part, backend);
	int err;
	uint32_t offs = ob_tp->offset + offset;
	uint32_t count, left;
	uint8_t *buffer;
	
	SYS_LOG_INF("dev %s: offset 0x%x, size 0x%x, buf %p",
		ob_tp->dev_name, offset, size, buf);

	if ((offset + size) > ob_tp->size) {
		SYS_LOG_ERR("offs 0x%x size 0x%x is too big, max size 0x%x",
			offset, size, ob_tp->size);

		return -EINVAL;
	}

	if (ob_tp->type == OTA_BACKEND_SD || ob_tp->type == OTA_BACKEND_NAND) {

		if ((ob_tp->offset % OTA_BACKEND_SD_SECTOR_SIZE)
			|| (offset % OTA_BACKEND_SD_SECTOR_SIZE)) {
			SYS_LOG_ERR("(%d %d) not align with %d",
				ob_tp->offset, offset, OTA_BACKEND_SD_SECTOR_SIZE);
			return -EINVAL;
		}

		count = size / OTA_BACKEND_SD_SECTOR_SIZE;
		left = size % OTA_BACKEND_SD_SECTOR_SIZE;
		offs /= OTA_BACKEND_SD_SECTOR_SIZE;

		if (count) {
			err = flash_read(ob_tp->dev, offs, buf, count);
			if (err) {
				SYS_LOG_ERR("read error %d, offs 0x%x, count %d",
					err, offs, count);
				return -EIO;
			}
		}
		if (left) {
			buffer = (uint8_t *)mem_malloc(OTA_BACKEND_SD_SECTOR_SIZE);
			if (!buffer) {
				SYS_LOG_ERR("can not malloc %d size", OTA_BACKEND_SD_SECTOR_SIZE);
				return -ENOMEM;
			}

			buf += (count * OTA_BACKEND_SD_SECTOR_SIZE);
			offs += count;

			err = flash_read(ob_tp->dev, offs, buffer, 1);
			if (err) {
				SYS_LOG_ERR("read error %d, offs 0x%x, count %d",
					err, offs, count);
				err = -EIO;
			}

			memcpy(buf, buffer, left);
			
			mem_free(buffer);
		}
	} else { 
		err = flash_read(ob_tp->dev, offs, buf, size);
		if (err) {
			SYS_LOG_ERR("read error %d, offs 0x%x, buf %p, size 0x%x",
				err, offset, buf, size);
			return -EIO;
		}
	}

	return err;
}

int ota_backend_temp_part_open(struct ota_backend *backend)
{
	struct ota_backend_temp_part *backend_temp_part = CONTAINER_OF(backend,
		struct ota_backend_temp_part, backend);

	SYS_LOG_INF("dev %s: open: type %d", backend_temp_part->dev_name, backend->type);

	return 0;
}

int ota_backend_temp_part_close(struct ota_backend *backend)
{
	struct ota_backend_temp_part *backend_temp_part = CONTAINER_OF(backend,
		struct ota_backend_temp_part, backend);

	SYS_LOG_INF("dev %s: close: type %d", backend_temp_part->dev_name, backend->type);

	return 0;
}

void ota_backend_temp_part_exit(struct ota_backend *backend)
{
	struct ota_backend_temp_part *backend_temp_part = CONTAINER_OF(backend,
		struct ota_backend_temp_part, backend);

	SYS_LOG_INF("dev %s: exit: type %d", backend_temp_part->dev_name, backend->type);

	mem_free(backend_temp_part);
}

const struct ota_backend_api ota_backend_api_temp_part = {
	.init = (void *)ota_backend_temp_part_init,
	.exit = ota_backend_temp_part_exit,
	.open = ota_backend_temp_part_open,
	.close = ota_backend_temp_part_close,
	.read = ota_backend_temp_part_read,
};

struct ota_backend *ota_backend_temp_part_init(ota_backend_notify_cb_t cb,
		struct ota_backend_temp_part_init_param *param)
{
	struct ota_backend_temp_part *backend_temp_part;
	const struct partition_entry *temp_part;
	const struct device *temp_part_dev;

	SYS_LOG_INF("init backend %s\n", param->dev_name);

	temp_part_dev = device_get_binding(param->dev_name);
	if (!temp_part_dev) {
		SYS_LOG_ERR("cannot found temp part device %s", param->dev_name);
		return NULL;
	}

	temp_part = partition_get_temp_part();
	if (temp_part == NULL) {
		SYS_LOG_ERR("cannot found temp partition to store ota fw");
		return NULL;
	}

	SYS_LOG_INF("temp partition offset 0x%x, size 0x%x\n",
		temp_part->offset, temp_part->size);

	backend_temp_part = mem_malloc(sizeof(struct ota_backend_temp_part));
	if (!backend_temp_part) {
		SYS_LOG_ERR("malloc failed");
		return NULL;
	}

	memset(backend_temp_part, 0x0, sizeof(struct ota_backend_temp_part));
	backend_temp_part->dev = temp_part_dev;
	backend_temp_part->dev_name = param->dev_name;
	backend_temp_part->offset = temp_part->offset;
	backend_temp_part->size = temp_part->size;

	if (!strcmp(param->dev_name, CONFIG_MMC_SDCARD_DEV_NAME)) {
		backend_temp_part->type = OTA_BACKEND_SD;
		if (sd_card_storage_init(backend_temp_part->dev)) {
			SYS_LOG_ERR("init sd device error");
			return NULL;
		}
	} else if (!strcmp(param->dev_name, CONFIG_SPINAND_DEV_NAME)) {
		backend_temp_part->type = OTA_BACKEND_NAND;
	} else {
		backend_temp_part->type = OTA_BACKEND_SPINOR;
	}

	ota_backend_init(&backend_temp_part->backend, OTA_BACKEND_TYPE_TEMP_PART,
			 (struct ota_backend_api *)&ota_backend_api_temp_part, cb);

	/* spinor backend detected, notify app */
	cb(&backend_temp_part->backend, OTA_BACKEND_UPGRADE_STATE, 1);

	return &backend_temp_part->backend;
}

