/*
* Copyright (c) 2018 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/
#ifndef __DISK_ACCESS_USB_H__
#define __DISK_ACCESS_USB_H__

int _USB_disk_status(struct disk_info *disk);
int _USB_disk_initialize(struct disk_info *disk);
int _USB_disk_read(struct disk_info *disk, u8_t *buff, u32_t start_sector, u32_t sector_count);
int _USB_disk_write(struct disk_info *disk, const u8_t *buff, u32_t start_sector, u32_t sector_count);
int _USB_disk_ioctl(struct disk_info *disk, u8_t cmd, void *buff);

#endif /* __DISK_ACCESS_USB_H__ */
