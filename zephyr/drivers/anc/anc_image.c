/*
 * Copyright (c) 1997-2015, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <soc.h>
#include <zephyr.h>
#include <soc_anc.h>

#ifdef LOAD_IMAGE_FROM_FS
#include <sys/types.h>
#include <fs/fs_interface.h>
#include <fs/fat_fs.h>
#include <fs.h>
#endif /* LOAD_IMAGE_FROM_FS */

#include "anc_image.h"

static int load_scntable(const void *image, unsigned int offset, bool is_code)
{
	const struct IMG_scnhdr *scnhdr = (const struct IMG_scnhdr *)((uint32_t)image + offset);

	if (scnhdr->size == 0)
		return -ENODATA;

	for (; scnhdr->size > 0; ) {
		uint32_t cpu_addr = anc_to_mcu_address(scnhdr->addr, is_code);
		if (cpu_addr == UINT32_MAX) {
			printk("invalid address 0x%08x\n", scnhdr->addr);
			return -EFAULT;
		}

		// printk("load: offset=0x%08x, size=0x%08x, anc_addr=0x%08x, cpu_addr=0x%08x\n",
		// 	   (unsigned int)(scnhdr->data - (const uint8_t *)image),
		// 	   scnhdr->size, scnhdr->addr, cpu_addr);

		anc_soc_release_mem();

		memcpy((void*)cpu_addr, scnhdr->data, scnhdr->size);

		anc_soc_request_mem();
		scnhdr = (struct IMG_scnhdr*)&scnhdr->data[scnhdr->size];
	}

	return 0;
}


int load_anc_image(const void *image, size_t size, uint32_t *entry_point)
{
	const struct IMG_filehdr *filehdr = image;

	/* FIXME: use sys_get_le32 to handle unaligned 32bit access insead ? */
	if ((uint32_t)image & 0x3) {
		printk("image load address %p should be aligned to 4, "
			"could use __attribute__((__aligned__(x)))\n", image);
		return -EFAULT;
	}

	assert(size > sizeof(*filehdr));
	assert(size > filehdr->code_scnptr);
	assert(size > filehdr->code_scnptr);

	if (filehdr->magic != IMAGE_MAGIC('y', 'q', 'h', 'x')) {
		printk("invalid anc magic 0x%08x\n", filehdr->magic);
		return -EINVAL;
	}

	if (filehdr->code_scnptr) {
		if (load_scntable(image, filehdr->code_scnptr, true)) {
			printk("failed to load code\n");
			return -EINVAL;
		}
	}

	if (filehdr->data_scnptr) {
		if (load_scntable(image, filehdr->data_scnptr, false)) {
			printk("failed to load data\n");
			return -EINVAL;
		}
	}

	if (entry_point)
		*entry_point = filehdr->entry_point;

	return 0;
}
