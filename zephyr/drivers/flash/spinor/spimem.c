/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief common code for SPI Flash (NOR & NAND & PSRAM)
 */

#include "spi_internal.h"
#include "spimem.h"

#define	SPIMEM_CMD_READ_CHIPID			0x9f	/* JEDEC ID */

#define	SPIMEM_CMD_FAST_READ			0x0b	/* fast read */
#define	SPIMEM_CMD_FAST_READ_X2			0x3b	/* data x2 */
#define	SPIMEM_CMD_FAST_READ_X4			0x6b	/* data x4 */
#define	SPIMEM_CMD_FAST_READ_X2IO		0xbb	/* addr & data x2 */
#define	SPIMEM_CMD_FAST_READ_X4IO		0xeb	/* addr & data x4 */

#define	SPIMEM_CMD_ENABLE_WRITE			0x06	/* enable write */
#define	SPIMEM_CMD_DISABLE_WRITE		0x04	/* disable write */

#define	SPIMEM_CMD_WRITE_PAGE			0x02	/* write one page */
#define SPIMEM_CMD_ERASE_BLOCK			0xd8	/* 64KB erase */
#define SPIMEM_CMD_CONTINUOUS_READ_RESET	0xff	/* exit quad continuous_read mode */

static void sys_memcpy_swap(void *dst, const void *src, int length)
{
    unsigned char *tmp_src = (unsigned char *)((unsigned int)src + (length - 1));
    unsigned char *tmp_dst = (unsigned char *)dst;

    for (; length > 0; length--) {
        *tmp_dst++ = *tmp_src--;
    }
}

static void _spimem_read_data(struct spi_info *si, void *buf, int len)
{
    if ((len > 16) && si->dma_base) {
        spi_read_data_by_dma(si, (unsigned char *)buf, len);
    } else {
        spi_read_data(si, (unsigned char *)buf, len);
    }
}

static void _spimem_write_data(struct spi_info *si, const void *buf, int len)
{
    if ((len > 16) && si->dma_base) {
        spi_write_data_by_dma(si, (const unsigned char *)buf, len);
    } else {
        spi_write_data(si, (const unsigned char *)buf, len);
    }
}

static void _spimem_write_byte(struct spi_info *si, unsigned char byte)
{
    _spimem_write_data(si, &byte, 1);
}

void spimem_set_cs(struct spi_info *si, int value)
{
    spi_set_cs(si, value);
}

void spimem_continuous_read_reset(struct spi_info *si)
{
    spimem_set_cs(si, 0);
    _spimem_write_byte(si, SPIMEM_CMD_CONTINUOUS_READ_RESET);
    _spimem_write_byte(si, SPIMEM_CMD_CONTINUOUS_READ_RESET);
    spimem_set_cs(si, 1);
}

unsigned int spimem_prepare_op(struct spi_info *si)
{
    unsigned int orig_spi_ctl;
    unsigned int use_3wire = 0;

    /* backup old SPI_CTL */
    orig_spi_ctl = spi_read(si, SSPI_CTL);


    if (!spi_is_randmomize_pause(si))
        spi_reset(si);

    if (spi_controller_num(si) == 0) {
        spi_write(si, SSPI_CTL, (orig_spi_ctl & SSPI_CTL_RAND_MASK) |
              SSPI_CTL_AHB_REQ | SSPI_CTL_IO_MODE_1X | SSPI_CTL_WR_MODE_DISABLE |
              SSPI_CTL_RX_FIFO_EN | SSPI_CTL_TX_FIFO_EN | SSPI_CTL_SS |
              8 << SSPI_CTL_DELAYCHAIN_SHIFT);
        use_3wire = orig_spi_ctl & SSPI_CTL_SPI_3WIRE;
    } else {
        spi_write(si, SSPI_CTL,
            SSPI1_CTL_AHB_REQ | SSPI_CTL_IO_MODE_1X | SSPI_CTL_WR_MODE_DISABLE |
            SSPI_CTL_RX_FIFO_EN | SSPI_CTL_TX_FIFO_EN | SSPI_CTL_SS |
            8 << SSPI_CTL_DELAYCHAIN_SHIFT);
    }

    if (use_3wire)
        spi_set_bits(si, SSPI_CTL, SSPI_CTL_SPI_3WIRE, SSPI_CTL_SPI_3WIRE);

    if (si->delay_chain != 0xff)
        spi_set_bits(si, SSPI_CTL, SSPI_CTL_DELAYCHAIN_MASK,
                 si->delay_chain << SSPI_CTL_DELAYCHAIN_SHIFT);

    if (si->flag & SPI_FLAG_SPI_MODE0) {
        if (spi_controller_num(si) == 0) {
            spi_set_bits(si, SSPI_CTL, SSPI_CTL_MODE_MASK,
                SSPI_CTL_MODE_MODE0);
        } else {
            spi_set_bits(si, SSPI_CTL, SSPI1_CTL_MODE_MASK,
                SSPI1_CTL_MODE_MODE0);
        }
    }

    if (si->set_clk && si->freq_khz)
        si->set_clk(si, si->freq_khz);

    if (si->prepare_hook)
        si->prepare_hook(si);

    return orig_spi_ctl;
}

void spimem_complete_op(struct spi_info *si, unsigned int orig_spi_ctl)
{
    /* restore old SPI_CTL */
    spi_write(si, SSPI_CTL, orig_spi_ctl);
    spi_delay();
}

int spimem_transfer(struct spi_info *si, unsigned char cmd, unsigned int addr,
                  int addr_len, void *buf, int length,
                  unsigned char dummy_len, unsigned int flag)
{

    unsigned int orig_spi_ctl, i, addr_be;
    unsigned int key = 0;

    /* address to big endian */
    if (addr_len > 0)
        sys_memcpy_swap(&addr_be, &addr, addr_len);

    if (!(si->flag & SPI_FLAG_NO_IRQ_LOCK)) {
        key = irq_lock();
    }
    orig_spi_ctl = spimem_prepare_op(si);
    if (!spi_is_randmomize_pause(si))
        spi_setup_randmomize(si, 0, 0);

    #ifdef NORDEBUG
    // SPI_SEED configuration, nor reset
        sys_write32(0x12345678, SPI0_SEED);
    #endif

    spimem_set_cs(si, 0);

    /* cmd & address & data all use multi io mode */
    if (flag & SPIMEM_TFLAG_MIO_CMD_ADDR_DATA)
        spi_setup_bus_width(si, si->bus_width);

    /* write command */
    _spimem_write_byte(si, cmd);

    /* address & data use multi io mode */
    if (flag & SPIMEM_TFLAG_MIO_ADDR_DATA)
        spi_setup_bus_width(si, si->bus_width);

    if (addr_len > 0)
        _spimem_write_data(si, &addr_be, addr_len);

    /* send dummy bytes */
    for (i = 0; i < dummy_len; i++)
        _spimem_write_byte(si, 0);

    /* only data use multi io mode */
    if (flag & SPIMEM_TFLAG_MIO_DATA)
        spi_setup_bus_width(si, si->bus_width);

    /* send or get data */
    if (length > 0) {
        if (flag & SPIMEM_TFLAG_WRITE_DATA) {
            if (flag & SPIMEM_TFLAG_RESUME_RANDOMIZE)
                spi_pause_resume_randmomize(si, 0);

            if (flag & SPIMEM_TFLAG_ENABLE_RANDOMIZE)
                spi_setup_randmomize(si, 1, 1);

            _spimem_write_data(si, buf, length);
        } else {
            if (flag & SPIMEM_TFLAG_RESUME_RANDOMIZE)
                spi_pause_resume_randmomize(si, 0);

            if (flag & SPIMEM_TFLAG_ENABLE_RANDOMIZE)
                spi_setup_randmomize(si, 0, 1);

            _spimem_read_data(si, buf, length);
        }
    }

    /* restore spi bus width to 1-bit */
    if (flag & SPIMEM_TFLAG_MIO_MASK)
        spi_setup_bus_width(si, 1);

    if (flag & SPIMEM_TFLAG_PAUSE_RANDOMIZE)
        spi_pause_resume_randmomize(si, 1);
    else if (flag & SPIMEM_TFLAG_ENABLE_RANDOMIZE)
        spi_setup_randmomize(si, 0, 0);

    spimem_set_cs(si, 1);

    if (!spi_is_randmomize_pause(si))
        spimem_complete_op(si, orig_spi_ctl);

    if (!(si->flag & SPI_FLAG_NO_IRQ_LOCK)) {
        irq_unlock(key);
    }

    return 0;
}

int spimem_write_cmd_addr(struct spi_info *si, unsigned char cmd,
                    unsigned int addr, int addr_len)
{
    return spimem_transfer(si, cmd, addr, addr_len, 0, 0, 0, 0);
}

int spimem_write_cmd(struct spi_info *si, unsigned char cmd)
{
    return spimem_write_cmd_addr(si, cmd, 0, 0);
}

void spimem_read_chipid(struct spi_info *si, void *chipid, int len)
{
    spimem_transfer(si, SPIMEM_CMD_READ_CHIPID, 0, 0, chipid, len, 0, 0);
}

unsigned char spimem_read_status(struct spi_info *si, unsigned char cmd)
{
    unsigned int status;

    spimem_transfer(si, cmd, 0, 0, &status, 1, 0, 0);

    return status;
}

void spimem_set_write_protect(struct spi_info *si, int protect)
{
    if (protect)
        spimem_write_cmd(si, SPIMEM_CMD_DISABLE_WRITE);
    else
        spimem_write_cmd(si, SPIMEM_CMD_ENABLE_WRITE);
}

void spimem_read_page(struct spi_info *si,
        unsigned int addr, int addr_len,
        void *buf, int len)
{
    unsigned char cmd;
    unsigned int flag, dlen =1;

    if (si->bus_width == 4) {
        if(si->flag & SPI_FLAG_SPI_NXIO) {
            cmd = SPIMEM_CMD_FAST_READ_X4IO;
            flag = SPIMEM_TFLAG_MIO_ADDR_DATA;
            dlen = 3;/*1byte mode & 2byte dummy*/
        }else{
            cmd = SPIMEM_CMD_FAST_READ_X4;
            flag = SPIMEM_TFLAG_MIO_DATA;
        }

    } else if (si->bus_width == 2) {
        if(si->flag & SPI_FLAG_SPI_NXIO){
            cmd = SPIMEM_CMD_FAST_READ_X2IO;
            flag = SPIMEM_TFLAG_MIO_ADDR_DATA;
        }else {
            cmd = SPIMEM_CMD_FAST_READ_X2;
            flag = SPIMEM_TFLAG_MIO_DATA;
        }
    } else {
        cmd = SPIMEM_CMD_FAST_READ;
        flag = 0;
    }

    /* change to 4 bytes address commands*/
    if (addr_len == 4)
        cmd++;

    spimem_transfer(si, cmd, addr, addr_len, buf, len, dlen, flag);
}

int spimem_erase_block(struct spi_info *si, unsigned int addr)
{
    spimem_set_write_protect(si, 0);
    spimem_write_cmd_addr(si, SPIMEM_CMD_ERASE_BLOCK, addr, 3);

    return 0;
}
