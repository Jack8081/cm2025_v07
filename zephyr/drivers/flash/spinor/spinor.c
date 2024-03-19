/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPINOR Flash driver for LARK
 */
#include <irq.h>
#include "spi_internal.h"
#include "spimem.h"
#include "../spi_flash.h"

/* spinor parameters */
#define SPINOR_WRITE_PAGE_SIZE_BITS	8
#define SPINOR_ERASE_SECTOR_SIZE_BITS	12
#define SPINOR_ERASE_BLOCK_SIZE_BITS	16

#define SPINOR_WRITE_PAGE_SIZE		(1 << SPINOR_WRITE_PAGE_SIZE_BITS)
#define SPINOR_ERASE_SECTOR_SIZE	(1 << SPINOR_ERASE_SECTOR_SIZE_BITS)
#define SPINOR_ERASE_BLOCK_SIZE		(1 << SPINOR_ERASE_BLOCK_SIZE_BITS)

#define SPINOR_WRITE_PAGE_MASK		(SPINOR_WRITE_PAGE_SIZE - 1)
#define SPINOR_ERASE_SECTOR_MASK	(SPINOR_ERASE_SECTOR_SIZE - 1)
#define SPINOR_ERASE_BLOCK_MASK		(SPINOR_ERASE_BLOCK_SIZE - 1)

/* spinor commands */
#define	SPINOR_CMD_WRITE_PAGE		0x02	/* write one page */
#define	SPINOR_CMD_DISABLE_WRITE	0x04	/* disable write */
#define	SPINOR_CMD_READ_STATUS		0x05	/* read status1 */
#define	SPINOR_CMD_READ_STATUS2		0x35	/* read status2 */
#define	SPINOR_CMD_READ_STATUS3		0x15	/* read status3 */
#define	SPINOR_CMD_WRITE_STATUS		0x01	/* write status1 */
#define	SPINOR_CMD_WRITE_STATUS2	0x31	/* write status2 */
#define	SPINOR_CMD_WRITE_STATUS3	0x11	/* write status3 */
#define	SPINOR_CMD_ENABLE_WRITE		0x06	/* enable write */
#define	SPINOR_CMD_FAST_READ		0x0b	/* fast read */
#define SPINOR_CMD_ERASE_SECTOR		0x20	/* 4KB erase */
#define SPINOR_CMD_ERASE_BLOCK_32K	0x52	/* 32KB erase */
#define SPINOR_CMD_ERASE_BLOCK		0xd8	/* 64KB erase */
#define	SPINOR_CMD_READ_CHIPID		0x9f	/* JEDEC ID */
#define	SPINOR_CMD_DISABLE_QSPI		0xff	/* disable QSPI */

#define SPINOR_CMD_EN4B             0xB7	/* enter 4-byte address mode */

/* spinor 4byte address commands */
#define	SPINOR_CMD_WRITE_PAGE_4B	0x12	/* write one page by 4bytes address */
#define SPINOR_CMD_ERASE_SECTOR_4B	0x21	/* 4KB erase by 4bytes address */
#define SPINOR_CMD_ERASE_BLOCK_4B	0xdc	/* 64KB erase by 4bytes address */

/* NOR Flash vendors ID */
#define SPINOR_MANU_ID_ALLIANCE		0x52	/* Alliance Semiconductor */
#define SPINOR_MANU_ID_AMD			0x01	/* AMD */
#define SPINOR_MANU_ID_AMIC			0x37	/* AMIC */
#define SPINOR_MANU_ID_ATMEL		0x1f	/* ATMEL */
#define SPINOR_MANU_ID_CATALYST		0x31	/* Catalyst */
#define SPINOR_MANU_ID_ESMT			0x8c	/* ESMT */
#define SPINOR_MANU_ID_EON			0x1c	/* EON */
#define SPINOR_MANU_ID_FD_MICRO		0xa1	/* shanghai fudan microelectronics */
#define SPINOR_MANU_ID_FIDELIX		0xf8	/* FIDELIX */
#define SPINOR_MANU_ID_FMD			0x0e	/* Fremont Micro Device(FMD) */
#define SPINOR_MANU_ID_FUJITSU		0x04	/* Fujitsu */
#define SPINOR_MANU_ID_GIGADEVICE	0xc8	/* GigaDevice */
#define SPINOR_MANU_ID_GIGADEVICE2	0x51	/* GigaDevice2 */
#define SPINOR_MANU_ID_HYUNDAI		0xad	/* Hyundai */
#define SPINOR_MANU_ID_INTEL		0x89	/* Intel */
#define SPINOR_MANU_ID_MACRONIX		0xc2	/* Macronix (MX) */
#define SPINOR_MANU_ID_NANTRONIC	0xd5	/* Nantronics */
#define SPINOR_MANU_ID_NUMONYX		0x20	/* Numonyx, Micron, ST */
#define SPINOR_MANU_ID_PMC			0x9d	/* PMC */
#define SPINOR_MANU_ID_SANYO		0x62	/* SANYO */
#define SPINOR_MANU_ID_SHARP		0xb0	/* SHARP */
#define SPINOR_MANU_ID_SPANSION		0x01	/* SPANSION */
#define SPINOR_MANU_ID_SST			0xbf	/* SST */
#define SPINOR_MANU_ID_SYNCMOS_MVC	0x40	/* SyncMOS (SM) and Mosel Vitelic Corporation (MVC) */
#define SPINOR_MANU_ID_TI			0x97	/* Texas Instruments */
#define SPINOR_MANU_ID_WINBOND		0xda	/* Winbond */
#define SPINOR_MANU_ID_WINBOND_NEX	0xef	/* Winbond (ex Nexcom) */
#define SPINOR_MANU_ID_ZH_BERG		0xe0	/* ZhuHai Berg microelectronics (Bo Guan) */

//#define SPINOR_FLAG_UNLOCK_IRQ_WAIT_READY		(1 << 0)

#define NOR_DELAY_CHAIN 		    0x8

/* system XIP spinor */
static const struct spinor_info system_spi_nor = {
    .spi = {
        .base = SPI0_REG_BASE,
        .bus_width = 1,
        .delay_chain = NOR_DELAY_CHAIN,
        .flag = 0,
#if 0
        .dma_base= 0x4001C600, //DMA5
#endif
    }
};

static unsigned int spinor_read_status(struct spinor_info *sni, unsigned char cmd)
{
    if (!sni)
        sni = (struct spinor_info *)&system_spi_nor;

    return spimem_read_status(&sni->spi, cmd);
}

static int spinor_wait_ready(struct spinor_info *sni)
{
    unsigned char status;

    while (1) {
        status = spinor_read_status(sni, SPINOR_CMD_READ_STATUS);
        if (!(status & 0x1))
            break;
    }

    return 0;
}

static void spinor_write_data(struct spinor_info *sni, unsigned char cmd,
            unsigned int addr, int addr_len, const unsigned char *buf, int len)
{
    struct spi_info *si = &sni->spi;
    unsigned int key = 0;

    if (!(si->flag & SPI_FLAG_NO_IRQ_LOCK)) {
        key = irq_lock();
    }

    spimem_set_write_protect(si, 0);
    spimem_transfer(si, cmd, addr, addr_len, (unsigned char *)buf, len,
            0, SPIMEM_TFLAG_WRITE_DATA);
    if (!(si->flag & SPI_FLAG_NO_IRQ_LOCK)) {
        if (sni->flag & SPINOR_FLAG_UNLOCK_IRQ_WAIT_READY) {
            irq_unlock(key);
            spinor_wait_ready(sni);
        } else {
            spinor_wait_ready(sni);
            irq_unlock(key);
        }
    }else{
        spinor_wait_ready(sni);
    }
}

static int spinor_erase_internal(struct spinor_info *sni,
            unsigned char cmd, unsigned int addr)
{
    spinor_write_data(sni, cmd, addr, 4, 0, 0);
    return 0;
}

static int spinor_write_internal(struct spinor_info *sni,
            unsigned int addr, const unsigned char *buf, int len)
{
    spinor_write_data(sni, SPINOR_CMD_WRITE_PAGE_4B, addr, 4, buf, len);
    return 0;
}

static int spinor_read_internal(struct spinor_info *sni,
            unsigned int addr, unsigned char *buf, int len)
{
    spimem_read_page(&sni->spi, addr, 4, buf, len);
    return 0;
}

int spinor_read(struct spinor_info *sni, unsigned int addr, void *data, int len)
{
    if (!len)
        return 0;

    if (!sni)
        sni = (struct spinor_info *)&system_spi_nor;

    spinor_read_internal(sni, addr, data, len);

    return 0;
}

int spinor_write(struct spinor_info *sni, unsigned int addr,
            const void *data, int len)
{
    int unaligned_len, remaining, write_size;

    if (!len)
        return 0;

    if (!sni)
        sni = (struct spinor_info *)&system_spi_nor;

    /* unaligned write? */
    if (addr & SPINOR_WRITE_PAGE_MASK)
        unaligned_len = SPINOR_WRITE_PAGE_SIZE - (addr & SPINOR_WRITE_PAGE_MASK);
    else
        unaligned_len = 0;

    remaining = len;
    while (remaining > 0) {
        if (unaligned_len) {
            /* write unaligned page data */
            if (unaligned_len > len)
                write_size = len;
            else
                write_size = unaligned_len;
            unaligned_len = 0;
        } else if (remaining < SPINOR_WRITE_PAGE_SIZE)
            write_size = remaining;
        else
            write_size = SPINOR_WRITE_PAGE_SIZE;

        spinor_write_internal(sni, addr, data, write_size);

        addr += write_size;
        data = (unsigned char *)data+write_size;
        remaining -= write_size;
    }

    return 0;
}

int spinor_write_with_randomizer(struct spinor_info *sni, unsigned int addr, const void *data, int len)
{
    struct spi_info *si;
    u32_t key, page_addr, origin_spi_ctl;
    int wlen;
    unsigned char addr_len;
    unsigned char cmd;

    if (!sni)
        sni = (struct spinor_info *)&system_spi_nor;

    si = &sni->spi;

    key = irq_lock(); //ota diff upgrade, must be irq lock

    origin_spi_ctl = spi_read(si, SSPI_CTL);

    spimem_set_write_protect(si, 0);

    page_addr = (addr + len) & ~SPINOR_WRITE_PAGE_MASK;
    if ((addr & ~SPINOR_WRITE_PAGE_MASK) != page_addr) {
        /* data cross write page bound, need split */
        wlen = page_addr - addr;

        addr_len = 4;
        cmd = SPINOR_CMD_WRITE_PAGE_4B;

        spimem_transfer(si, cmd, addr, addr_len, (u8_t *)data, wlen,
                  0, SPIMEM_TFLAG_WRITE_DATA | SPIMEM_TFLAG_ENABLE_RANDOMIZE |
                  SPIMEM_TFLAG_PAUSE_RANDOMIZE);
        spinor_wait_ready(sni);

        data = (unsigned char *)data + wlen;
        len -= wlen;
        addr = page_addr;

        spimem_set_write_protect(si, 0);
    }

    addr_len = 4;
    cmd = SPINOR_CMD_WRITE_PAGE_4B;

    spimem_transfer(si, cmd, addr, addr_len, (u8_t *)data, len,
              0, SPIMEM_TFLAG_WRITE_DATA | SPIMEM_TFLAG_ENABLE_RANDOMIZE | SPIMEM_TFLAG_RESUME_RANDOMIZE);
    spinor_wait_ready(sni);

    spi_write(si, SSPI_CTL, origin_spi_ctl);
    spi_delay();

    irq_unlock(key);

    return 0;

}

int spinor_enter_4byte_address_mode(struct spinor_info *sni)
{
    printk("spinor enter 4-byte address mode\n");
    return spimem_transfer(&sni->spi, SPINOR_CMD_EN4B, 0, 0, NULL, 0, 0, 0);
}

int spinor_erase(struct spinor_info *sni, unsigned int addr, int size)
{
    int remaining, erase_size;
    unsigned char cmd;

    if (!size)
        return 0;

    if (addr & SPINOR_ERASE_SECTOR_MASK || size & SPINOR_ERASE_SECTOR_MASK)
        return -1;

    if (!sni)
        sni = (struct spinor_info *)&system_spi_nor;

    /* write aligned page data */
    remaining = size;
    while (remaining > 0) {
        if (addr & SPINOR_ERASE_BLOCK_MASK || remaining < SPINOR_ERASE_BLOCK_SIZE) {
            cmd = SPINOR_CMD_ERASE_SECTOR_4B;
            erase_size = SPINOR_ERASE_SECTOR_SIZE;
        } else {
            cmd = SPINOR_CMD_ERASE_BLOCK_4B;
            erase_size = SPINOR_ERASE_BLOCK_SIZE;
        }

        spinor_erase_internal(sni, cmd, addr);

        addr += erase_size;
        remaining -= erase_size;
    }

    return 0;
}

const struct spinor_operation_api spinor_4b_addr_op_api = {
    .read		= spinor_read,
    .write		= spinor_write,
    .erase		= spinor_erase,
};

