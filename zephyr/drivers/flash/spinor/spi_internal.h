/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPI controller helper for GL5120
 */


#ifndef __SPI_H__
#define __SPI_H__

#include <soc.h>
#include <arch/common/sys_io.h>
#include "../spi_flash.h"

#define SPI_DELAY_LOOPS			5

/* spinor controller */
#define SSPI_CTL					0x00
#define SSPI_STATUS					0x04
#define SSPI_TXDAT					0x08
#define SSPI_RXDAT					0x0c
#define SSPI_BC						0x10
#define SSPI_SEED					0x14
#define SCACHE_ERR_ADDR				0x18
#define SNO_CACHE_ADDR				0x1C

#define SSPI_CTL_CLK_SEL_MASK		((unsigned int)1 << 31)
#define SSPI_CTL_CLK_SEL_CPU		((unsigned int)0 << 31)
#define SSPI_CTL_CLK_SEL_DMA		((unsigned int)1 << 31)

#define SSPI_CTL_MODE_MASK			(1 << 28)
#define SSPI_CTL_MODE_MODE3			(0 << 28)
#define SSPI_CTL_MODE_MODE0			(1 << 28)

#define SSPI_CTL_CRC_EN				(1 << 20)

#define SSPI_CTL_DELAYCHAIN_MASK	(0xf << 16)
#define SSPI_CTL_DELAYCHAIN_SHIFT	(16)

#define SSPI_CTL_RAND_MASK			(0xf << 12)
#define SSPI_CTL_RAND_PAUSE			(1 << 15)
#define SSPI_CTL_RAND_SEL			(1 << 14)
#define SSPI_CTL_RAND_TXEN			(1 << 13)
#define SSPI_CTL_RAND_RXEN			(1 << 12)

#define SSPI_CTL_IO_MODE_MASK		(0x3 << 10)
#define SSPI_CTL_IO_MODE_SHIFT		(10)
#define SSPI_CTL_IO_MODE_1X			(0x0 << 10)
#define SSPI_CTL_IO_MODE_2X			(0x2 << 10)
#define SSPI_CTL_IO_MODE_4X			(0x3 << 10)
#define SSPI_CTL_SPI_3WIRE			(1 << 9)
#define SSPI_CTL_AHB_REQ			(1 << 8)
#define SSPI_CTL_TX_DRQ_EN			(1 << 7)
#define SSPI_CTL_RX_DRQ_EN			(1 << 6)
#define SSPI_CTL_TX_FIFO_EN			(1 << 5)
#define SSPI_CTL_RX_FIFO_EN			(1 << 4)
#define SSPI_CTL_SS					(1 << 3)
#define SSPI_CTL_WR_MODE_MASK		(0x3 << 0)
#define SSPI_CTL_WR_MODE_DISABLE	(0x0 << 0)
#define SSPI_CTL_WR_MODE_READ		(0x1 << 0)
#define SSPI_CTL_WR_MODE_WRITE		(0x2 << 0)
#define SSPI_CTL_WR_MODE_READ_WRITE	(0x3 << 0)

#define SSPI_STATUS_BUSY			(1 << 6)
#define SSPI_STATUS_TX_FULL			(1 << 5)
#define SSPI_STATUS_TX_EMPTY		(1 << 4)
#define SSPI_STATUS_RX_FULL			(1 << 3)
#define SSPI_STATUS_RX_EMPTY		(1 << 2)


#define SSPI0_REGISTER				(SPI0_BASE)
#define SSPI1_REGISTER				(SPI1_BASE)
#define SSPI2_REGISTER				(SPI2_BASE)

/* spi1 registers bits */
#define SSPI1_CTL_MODE_MASK			(3 << 28)
#define SSPI1_CTL_MODE_MODE3		(3 << 28)
#define SSPI1_CTL_MODE_MODE0		(0 << 28)
#define SSPI1_CTL_AHB_REQ			(1 << 15)

#define SSPI1_STATUS_BUSY			(1 << 0)
#define SSPI1_STATUS_TX_FULL		(1 << 5)
#define SSPI1_STATUS_TX_EMPTY		(1 << 4)
#define SSPI1_STATUS_RX_FULL		(1 << 7)
#define SSPI1_STATUS_RX_EMPTY		(1 << 6)

#define MRCR0_SPI3RESET             (7)
#define MRCR0_SPI2RESET             (6)
#define MRCR0_SPI1RESET             (5)
#define MRCR0_SPI0RESET             (4)

#define CMU_DEVCLKEN0_SPI3CLKEN     (7)
#define CMU_DEVCLKEN0_SPI2CLKEN     (6)
#define CMU_DEVCLKEN0_SPI1CLKEN     (5)
#define CMU_DEVCLKEN0_SPI0CLKEN     (4)

static inline void spi_delay(void)
{
    volatile int i = SPI_DELAY_LOOPS;

    while (i--)
        ;
}

static inline int spi_controller_num(struct spi_info *si)
{
    if (si->base == (SPI0_REG_BASE)) {
        return 0;
    } else if (si->base == (SPI1_REGISTER_BASE)) {
        return 1;
    } else {
        return 2;
    }
}

static inline unsigned int spi_read(struct spi_info *si, unsigned int reg)
{
    return sys_read32(si->base + reg);
}

static inline void spi_write(struct spi_info *si, unsigned int reg,
                    unsigned int value)
{
    sys_write32(value, si->base + reg);
}

static inline void spi_set_bits(struct spi_info *si, unsigned int reg,
                       unsigned int mask, unsigned int value)
{
    spi_write(si, reg, (spi_read(si, reg) & ~mask) | value);
}

static inline void spi_reset(struct spi_info *si)
{
    if (spi_controller_num(si) == 0) {
        /* SPI0 */
        sys_write32(sys_read32(RMU_MRCR0) & ~(1 << MRCR0_SPI0RESET), RMU_MRCR0);
        spi_delay();
        sys_write32(sys_read32(RMU_MRCR0) | (1 << MRCR0_SPI0RESET), RMU_MRCR0);
    } else if (spi_controller_num(si) == 1) {
        /* SPI1 */
        sys_write32(sys_read32(RMU_MRCR0) & ~(1 << MRCR0_SPI1RESET), RMU_MRCR0);
        spi_delay();
        sys_write32(sys_read32(RMU_MRCR0) | (1 << MRCR0_SPI1RESET), RMU_MRCR0);
    } else if (spi_controller_num(si) == 2) {
        /* SPI2 */
        sys_write32(sys_read32(RMU_MRCR0) & ~(1 << MRCR0_SPI2RESET), RMU_MRCR0);
        spi_delay();
        sys_write32(sys_read32(RMU_MRCR0) | (1 << MRCR0_SPI2RESET), RMU_MRCR0);
    }
}

static inline void spi_init_clk(struct spi_info *si)
{
    if (spi_controller_num(si) == 0) {
        /* SPI0 clock source: 32M HOSC， div 2*/
        sys_write32(0x01, CMU_SPI0CLK);

        /* enable SPI0 module clock */
        sys_write32(sys_read32(CMU_DEVCLKEN0)  | (1 << CMU_DEVCLKEN0_SPI0CLKEN), CMU_DEVCLKEN0);
    } else if (spi_controller_num(si) == 1) {
        /* SPI1 clock source: 32M HOSC div 2*/
        sys_write32(0x01, CMU_SPI1CLK);

        /* enable SPI1 module clock */
        sys_write32(sys_read32(CMU_DEVCLKEN0)  | (1 << CMU_DEVCLKEN0_SPI1CLKEN), CMU_DEVCLKEN0);
    } else if (spi_controller_num(si) == 2) {
        /* SPI2 clock source: 32M HOSC  div 2*/
        sys_write32(0x01, CMU_SPI2CLK);

        /* enable SPI0 module clock */
        sys_write32(sys_read32(CMU_DEVCLKEN0)  | (1 << CMU_DEVCLKEN0_SPI2CLKEN), CMU_DEVCLKEN0);
    }
}

static inline void spi_set_cs(struct spi_info *si, int value)
{
    if (si->set_cs)
        si->set_cs(si, value);
    else
        spi_set_bits(si, SSPI_CTL, SSPI_CTL_SS, value ? SSPI_CTL_SS : 0);
}

static inline void spi_setup_bus_width(struct spi_info *si, unsigned char bus_width)
{
    spi_set_bits(si, SSPI_CTL, SSPI_CTL_IO_MODE_MASK,
             ((bus_width & 0x7) / 2 + 1) << SSPI_CTL_IO_MODE_SHIFT);
    spi_delay();
}

static inline void spi_setup_randmomize(struct spi_info *si, int is_tx, int enable)
{
    if (enable)
        spi_set_bits(si, SSPI_CTL, SSPI_CTL_RAND_TXEN | SSPI_CTL_RAND_RXEN,
                 is_tx ? SSPI_CTL_RAND_TXEN : SSPI_CTL_RAND_RXEN);
    else
        spi_set_bits(si, SSPI_CTL, SSPI_CTL_RAND_TXEN | SSPI_CTL_RAND_RXEN, 0);
}

static inline void spi_pause_resume_randmomize(struct spi_info *si, int is_pause)
{
    spi_set_bits(si, SSPI_CTL, SSPI_CTL_RAND_PAUSE, is_pause ? SSPI_CTL_RAND_PAUSE : 0);
}

static inline int spi_is_randmomize_pause(struct spi_info *si)
{
    return !!(spi_read(si, SSPI_CTL) & SSPI_CTL_RAND_PAUSE);
}


static inline void spi_wait_tx_complete(struct spi_info *si)
{
    if (spi_controller_num(si) == 0) {
        /* SPI0 */
        while (!(spi_read(si, SSPI_STATUS) & SSPI_STATUS_TX_EMPTY))
            ;

        /* wait until tx fifo is empty */
        while ((spi_read(si, SSPI_STATUS) & SSPI_STATUS_BUSY))
            ;
    } else {
        /* SPI1 & SPI2 & SPI3 */
        while (!(spi_read(si, SSPI_STATUS) & SSPI1_STATUS_TX_EMPTY))
            ;

        /* wait until tx fifo is empty */
        while ((spi_read(si, SSPI_STATUS) & SSPI1_STATUS_BUSY))
            ;
    }
}

static inline void spi_read_data(struct spi_info *si, unsigned char *buf,
                    int len)
{
    spi_write(si, SSPI_BC, len);

    /* switch to read mode */
    spi_set_bits(si, SSPI_CTL, SSPI_CTL_WR_MODE_MASK, SSPI_CTL_WR_MODE_READ);

    /* read data */
    while (len--) {
        if (spi_controller_num(si) == 0) {
            /* SPI0 */
            while (spi_read(si, SSPI_STATUS) & SSPI_STATUS_RX_EMPTY)
                ;
        } else {
            /* SPI1 & SPI2 & SPI3 */
            while (spi_read(si, SSPI_STATUS) & SSPI1_STATUS_RX_EMPTY)
                ;
        }

        *buf++ = spi_read(si, SSPI_RXDAT);
    }

    /* disable read mode */
    spi_set_bits(si, SSPI_CTL, SSPI_CTL_WR_MODE_MASK, SSPI_CTL_WR_MODE_DISABLE);
}

static inline void spi_write_data(struct spi_info *si,
                     const unsigned char *buf, int len)
{
    /* switch to write mode */
    spi_set_bits(si, SSPI_CTL, SSPI_CTL_WR_MODE_MASK, SSPI_CTL_WR_MODE_WRITE);

    /* write data */
    while (len--) {
        if (spi_controller_num(si) == 0) {
            /* SPI0 */
            while (spi_read(si, SSPI_STATUS) & SSPI_STATUS_TX_FULL)
                ;
        } else {
            /* SPI1 & SPI2 & SPI3 */
            while (spi_read(si, SSPI_STATUS) & SSPI1_STATUS_TX_FULL)
                ;
        }

        spi_write(si, SSPI_TXDAT, *buf++);
    }

    spi_delay();
    spi_wait_tx_complete(si);

    /* disable write mode */
    spi_set_bits(si, SSPI_CTL, SSPI_CTL_WR_MODE_MASK, SSPI_CTL_WR_MODE_DISABLE);
}

#define DMA_CTL		(0x00)
#define DMA_START	(0x04)
#define DMA_SADDR	(0x08)
#define DMA_DADDR	(0x10)
#define DMA_BC		(0x18)
#define DMA_RC		(0x1c)
//#define DMA_PD		(0x04)

static inline void spi_read_data_by_dma(struct spi_info *si,
                 const unsigned char *buf, int len)
{
    spi_write(si, SSPI_BC, len);

    /* switch to dma read mode */
    spi_set_bits(si, SSPI_CTL, SSPI_CTL_CLK_SEL_MASK | SSPI_CTL_RX_DRQ_EN | SSPI_CTL_WR_MODE_MASK,
        SSPI_CTL_CLK_SEL_DMA | SSPI_CTL_RX_DRQ_EN | SSPI_CTL_WR_MODE_READ);

   if (spi_controller_num(si) == 0) {
       /* SPI0 */
       sys_write32(0x200087, si->dma_base + DMA_CTL);
    } else if (spi_controller_num(si) == 1) {
       /* SPI1 */
       sys_write32(0x200088, si->dma_base + DMA_CTL);
    } else if (spi_controller_num(si) == 2) {
       /* SPI2 */
       sys_write32(0x200089, si->dma_base + DMA_CTL);
    } else {
       /* SPI2 */
       sys_write32(0x20008a, si->dma_base + DMA_CTL);
    }

    sys_write32(si->base + SSPI_RXDAT, si->dma_base + DMA_SADDR);

    sys_write32((unsigned int)buf, si->dma_base + DMA_DADDR);
    sys_write32(len, si->dma_base + DMA_BC);

    /* start dma */
    sys_write32(1, si->dma_base + DMA_START);

    while (sys_read32(si->dma_base + DMA_START) & 0x1) {
        /* wait */
    }

    spi_delay();
    spi_wait_tx_complete(si);

    spi_set_bits(si, SSPI_CTL, SSPI_CTL_CLK_SEL_MASK | SSPI_CTL_RX_DRQ_EN | SSPI_CTL_WR_MODE_MASK,
        SSPI_CTL_CLK_SEL_CPU | SSPI_CTL_WR_MODE_DISABLE);
}

static inline void spi_write_data_by_dma(struct spi_info *si,
                  const unsigned char *buf, int len)
{
    /* switch to dma write mode */
    spi_set_bits(si, SSPI_CTL, SSPI_CTL_CLK_SEL_MASK | SSPI_CTL_TX_DRQ_EN | SSPI_CTL_WR_MODE_MASK,
        SSPI_CTL_CLK_SEL_DMA | SSPI_CTL_TX_DRQ_EN | SSPI_CTL_WR_MODE_WRITE);

    if (spi_controller_num(si) == 0) {
       /* SPI0 */
       sys_write32(0x208700, si->dma_base + DMA_CTL);
    } else if (spi_controller_num(si) == 1) {
       /* SPI1 */
       sys_write32(0x208800, si->dma_base + DMA_CTL);
    } else if (spi_controller_num(si) == 2) {
       /* SPI2 */
       sys_write32(0x208900, si->dma_base + DMA_CTL);
    } else {
       /* SPI3 */
       sys_write32(0x208a00, si->dma_base + DMA_CTL);
    }

    sys_write32((unsigned int)buf, si->dma_base + DMA_SADDR);
    sys_write32(si->base + SSPI_TXDAT, si->dma_base + DMA_DADDR);
    sys_write32(len, si->dma_base + DMA_BC);

    /* start dma */
    sys_write32(1, si->dma_base + DMA_START);

    while (sys_read32(si->dma_base + DMA_START) & 0x1) {
        /* wait */
    }

    spi_delay();
    spi_wait_tx_complete(si);

    spi_set_bits(si, SSPI_CTL, SSPI_CTL_CLK_SEL_MASK | SSPI_CTL_TX_DRQ_EN | SSPI_CTL_WR_MODE_MASK,
        SSPI_CTL_CLK_SEL_CPU | SSPI_CTL_WR_MODE_DISABLE);
}

#endif	/* __SPI_H__ */
