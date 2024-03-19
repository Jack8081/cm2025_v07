/*
 * Copyright (c) 1997-2015, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ANC_IMAGE_H__
#define __ANC_IMAGE_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __packed
#ifdef __GNUC__
#define __packed __attribute__((__packed__))
#else
#define __packed
#endif
#endif

/* anc hardware memory resource */
#define CONFIG_PTCM_SIZE (0)
#define CONFIG_DTCM_SIZE (0x8000)

#define NUM_BANK_GROUPS (4)
#define NUM_BANKS_PER_GROUP (16)
#define NUM_BANKS (NUM_BANK_GROUPS * NUM_BANKS_PER_GROUP)
/********************** image structure ***************************************/

/*
 *                Image Layout
 * --------------------------------------------------------------
 * | magic (4 bytes, "yqhx")                                    |
 * |------------------------------------------------------------|
 * | version (4 byte)                                           |
 * |------------------------------------------------------------|
 * | f_type (1 byte)                                            |
 * |------------------------------------------------------------|
 * | f_subtype (1 byte)                                         |
 * -------------------------------------------------------------|
 * | f_reserved (reserved, 2 byte)                              |
 * -------------------------------------------------------------|
 * | f_version (4 byte)                                         |
 * |------------------------------------------------------------|
 * | entry point (4 bytes, anc word address)                    |
 * |------------------------------------------------------------|
 * | code & data section table pointer                          |
 * |------------------------------------------------------------|
 * | bank group 1 index 1 section table pointer                 |
 * |------------------------------------------------------------|
 * | bank group 1 index 2 section table pointer                 |
 * |------------------------------------------------------------|
 * |                 ......                                     |
 * |------------------------------------------------------------|
 * | bank group 1 index N1 section table pointer                |
 * |------------------------------------------------------------|
 * | bank group 2 index 1 section table pointer                 |
 * |------------------------------------------------------------|
 * | bank group 2 index 2 section table pointer                 |
 * |------------------------------------------------------------|
 * |                 ......                                     |
 * |------------------------------------------------------------|
 * | bank group 2 index N2 section table pointer                |
 * |------------------------------------------------------------|
 * | bank group n index 1 section table pointer                 |
 * |------------------------------------------------------------|
 * | bank group n index 2 section table pointer                 |
 * |------------------------------------------------------------|
 * |                 ......                                     |
 * |------------------------------------------------------------|
 * | bank group n index Nn section table pointer                |
 * |------------------------------------------------------------|
 * |                                                            |
 * |               section tables                               |
 * |                                                            |
 * --------------------------------------------------------------
 */

/*
 *                Image section table Layout
 * |------------------------------------------------------------|
 * | Section 1 size (4 bytes)                                   |
 * | Section 1 load address (4 bytes, anc word address)         |
 * | Section 1 raw data (8 x n bytes, padded to 8 bytes)        |
 * |------------------------------------------------------------|
 * | Section 2 size (4 bytes)                                   |
 * | Section 2 load address (4 bytes, anc word address)         |
 * | Section 2 raw data (8 x n bytes, padded to 8 bytes)        |
 * |------------------------------------------------------------|
 * |                 ......                                     |
 * |------------------------------------------------------------|
 * | Section N size (4 bytes)                                   |
 * | Section N load address (4 bytes, anc word address)         |
 * | Section N raw data (8 x n bytes, padded to 8 bytes)        |
 * |------------------------------------------------------------|
 * | 0x00000000 (end flag, 4 bytes)                             |
 * --------------------------------------------------------------
 */

/* section entry in image section table */
struct IMG_scnhdr {
	uint32_t size;
	uint32_t addr;
	uint8_t data[0];
} __packed;

#define IMG_SCNHDR struct IMG_scnhdr

#define IMAGE_MAGIC(a, b, c, d)                                            \
	(((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) |                       \
	 ((uint32_t)(c) << 8) | (d))

#define IMAGE_VERSION(major, minor, patchlevel)                           \
	(((uint32_t)(major) << 24) | ((uint32_t)(minor) << 16) |               \
	 ((uint32_t)(patchlevel) << 8))

#define IMG_VER_MAJOR(ver) (((ver) >> 24) & 0xFF)
#define IMG_VER_MINOR(ver) (((ver) >> 16) & 0xFF)
#define IMG_VER_PATCHLEVEL(ver) (((ver) >> 8) & 0xFF)

typedef struct IMG_filehdr {
	/* image format magic */
	uint32_t magic;
	/* image packing tool version */
	uint32_t version;

	/* image file information */
	uint8_t f_type; /* file (algorithm) type */
	uint8_t f_subtype;
	uint8_t f_reserved[2];
	uint32_t f_version; /* file (algorithm) version */

	/* anc program entry point */
	uint32_t entry_point;

	/* anc code & data sections */
	uint32_t code_scnptr;
	uint32_t data_scnptr;

	/* anc page miss and internal tcm */
	uint32_t bank_scnptr[NUM_BANKS][2];
} IMG_FILHDR;

#define IMG_FILHSZ sizeof(IMG_FILHDR)

//dtcm data head info
struct IMG_dtcmhdr {
	uint32_t anc_addr; //ANC Virtual destination address, move to address, word unit
	uint32_t mem_addr; //ANC virtual source address, data source address, word unit
	uint32_t length; //ANC data size, byte unit
	uint32_t reserved;
} __packed; //16 byte

#define IMG_DTCMHDR struct IMG_dtcmhdr

int load_anc_image(const void *image, size_t size, uint32_t *entry_point);
int load_anc_image_bank(const void *image, size_t size, unsigned int bank_no);

#ifdef __cplusplus
}
#endif

#endif /* __ANC_IMAGE_H__ */
