/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/flash.h>
#include <drivers/spi.h>
#include <logging/log.h>
#include <soc.h>
#include <board_cfg.h>
#include "spi_flash.h"
#include <linker/linker-defs.h>


LOG_MODULE_REGISTER(spi_flash_acts, CONFIG_FLASH_LOG_LEVEL);

#ifdef CONFIG_SPI_NOR_FLASH_4B_ADDRESS
extern int spinor_enter_4byte_address_mode(struct spinor_info *sni);
#endif

#if defined(CONFIG_SPI_FLASH_1_GPIO_CS_EN) && (CONFIG_SPI_FLASH_1_GPIO_CS_EN == 1)
static const struct device *spi_gpio_cs_dev;
#endif

#if (CONFIG_SPI_FLASH_SYNC_MULTI_DEV == 1)
static struct k_sem spi_flash_sync = Z_SEM_INITIALIZER(spi_flash_sync, 1, 1);
#endif

#ifdef SPINOR_RESET_FUN_ADDR
typedef void (*spi_reset_func)(struct spi_info *si);
__ramfunc void spi_flash_reset(struct spi_info *si)
{
	spi_reset_func func = (spi_reset_func)(SPINOR_RESET_FUN_ADDR);
	func(si);
}
#else
__ramfunc void spi_flash_reset(struct spi_info *si)
{
	p_spinor_api->continuous_read_reset((struct spinor_info *)si);
}
#endif

__ramfunc  void spi_flash_acts_prepare(struct spi_info *si)
{
	/* wait for spi ready */
	while(!(sys_read32(SPI_STA(si->base)) & SPI_STA_READY));

	spi_flash_reset(si);
}

__ramfunc int spi_flash_acts_read(const struct device *dev, off_t offset, void *data, size_t len)
{
    struct spinor_info *sni = DEV_DATA(dev);
    int ret = 0;
    size_t tmplen;

#if defined(CONFIG_SPI_FLASH_SYNC_MULTI_DEV) && (CONFIG_SPI_FLASH_SYNC_MULTI_DEV == 1)
    k_sem_take(&spi_flash_sync, K_FOREVER);
#endif
    tmplen = len;
    while(tmplen > 0) {
        if(tmplen <  0x8000)
            len = tmplen;
        else
            len = 0x8000;
    
#ifdef CONFIG_SPI_NOR_FLASH_4B_ADDRESS
        ret = spinor_4b_addr_op_api.read(sni, offset, data, len);
#else
#if defined(CONFIG_SPI_FLASH_NO_IRQ_LOCK) && (CONFIG_SPI_FLASH_NO_IRQ_LOCK == 1)
        ret = p_spinor_api->read(sni, offset, data, len);
#else
        uint32_t key = irq_lock();
        ret = p_spinor_api->read(sni, offset, data, len);
        irq_unlock(key);
#endif
#endif
        offset += len;
        data  = (void *)((unsigned int )data + len);
        tmplen -= len;
    }
#if defined(CONFIG_SPI_FLASH_SYNC_MULTI_DEV) && (CONFIG_SPI_FLASH_SYNC_MULTI_DEV == 1)
    k_sem_give(&spi_flash_sync);
#endif

    return ret;
}


__ramfunc int spi_flash_acts_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	struct spinor_info *sni = DEV_DATA(dev);
	int ret;

#if defined(CONFIG_SPI_FLASH_SYNC_MULTI_DEV) && (CONFIG_SPI_FLASH_SYNC_MULTI_DEV == 1)
	k_sem_take(&spi_flash_sync, K_FOREVER);
#endif

#ifdef CONFIG_SPI_NOR_FLASH_4B_ADDRESS
	ret = spinor_4b_addr_op_api.write(sni, offset, data, len);
#else
#if defined(CONFIG_SPI_FLASH_NO_IRQ_LOCK) && (CONFIG_SPI_FLASH_NO_IRQ_LOCK == 1)
	uint32_t flag = sni->spi.flag;
	uint32_t nor_flag = sni->flag;
	sni->flag |= SPINOR_FLAG_UNLOCK_IRQ_WAIT_READY; //unlock wait ready
	sni->spi.flag &= ~SPI_FLAG_NO_IRQ_LOCK;  //lock
	ret = p_spinor_api->write(sni, offset, data, len);
	sni->spi.flag = flag;
	sni->flag = nor_flag;
#else
	uint32_t key = irq_lock();
	ret = p_spinor_api->write(sni, offset, data, len);
	irq_unlock(key);
#endif
#endif

#if defined(CONFIG_SPI_FLASH_SYNC_MULTI_DEV) && (CONFIG_SPI_FLASH_SYNC_MULTI_DEV == 1)
	k_sem_give(&spi_flash_sync);
#endif

	return ret ;
}

__ramfunc int spi_flash_acts_erase(const struct device *dev, off_t offset, size_t size)
{
	struct spinor_info *sni = DEV_DATA(dev);
	int ret;

#if defined(CONFIG_SPI_FLASH_SYNC_MULTI_DEV) && (CONFIG_SPI_FLASH_SYNC_MULTI_DEV == 1)
	k_sem_take(&spi_flash_sync, K_FOREVER);
#endif

#ifdef CONFIG_SPI_NOR_FLASH_4B_ADDRESS
	ret = spinor_4b_addr_op_api.erase(sni, offset, size);
#else
#if defined(CONFIG_SPI_FLASH_NO_IRQ_LOCK) && (CONFIG_SPI_FLASH_NO_IRQ_LOCK == 1)
	uint32_t flag = sni->spi.flag;
	uint32_t nor_flag = sni->flag;
	sni->flag |= SPINOR_FLAG_UNLOCK_IRQ_WAIT_READY; //unlock wait ready
	sni->spi.flag &= ~SPI_FLAG_NO_IRQ_LOCK;  //lock
	ret = p_spinor_api->erase(sni, offset, size);
	sni->spi.flag = flag;
	sni->flag = nor_flag;
#else
	uint32_t key = irq_lock();
	ret = p_spinor_api->erase(sni, offset, size);
	irq_unlock(key);
#endif
#endif

#if defined(CONFIG_SPI_FLASH_SYNC_MULTI_DEV) && (CONFIG_SPI_FLASH_SYNC_MULTI_DEV == 1)
	k_sem_give(&spi_flash_sync);
#endif

	return ret ;
}

static inline void xspi_delay(void)
{
	volatile int i = 100000;

	while (i--)
		;
}

__ramfunc void xspi_nor_enable_status_qe(struct spinor_info *sni)
{
	uint16_t status;

	/* MACRONIX's spinor has different QE bit */
	if (XSPI_NOR_MANU_ID_MACRONIX == (sni->chipid & 0xff)) {
		status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
		if (!(status & 0x40)) {
			/* set QE bit to disable HOLD/WP pin function */
			status |= 0x40;
			p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS,
						(u8_t *)&status, 1);
		}

		return;
	}

	/* check QE bit */
	status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);

	if (!(status & 0x2)) {
		/* set QE bit to disable HOLD/WP pin function, for WinBond */
		status |= 0x2;
		p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS2,
					(u8_t *)&status, 1);

		/* check QE bit again */
		status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);

		if (!(status & 0x2)) {
			/* oh, let's try old write status cmd, for GigaDevice/Berg */
			status = ((status | 0x2) << 8) |
				 p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
			p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS,
						(u8_t *)&status, 2);
		}
	}

	xspi_delay();

}

static inline void xspi_setup_bus_width(struct spinor_info *sni, u8_t bus_width)
{
	struct acts_spi_reg *spi= (struct acts_spi_reg *)sni->spi.base;

	spi->ctrl = (spi->ctrl & ~(0x3 << 10)) | (((bus_width & 0x7) / 2 + 1) << 10);

	xspi_delay();
}

static inline void xspi_setup_delaychain(struct spinor_info *sni, u8_t ns)
{
	struct acts_spi_reg *spi= (struct acts_spi_reg *)sni->spi.base;
	spi->ctrl = (spi->ctrl & ~(0xF << 16)) | (ns << 16);

	xspi_delay();
}

#if IS_ENABLED(CONFIG_SPINOR_TEST_DELAYCHAIN)
extern int nor_test_delaychain(const struct device *dev);
#endif

#if IS_ENABLED(CONFIG_NOR_ACTS_DQ_MODE_ENABLE)
extern void nor_dual_quad_read_mode_try(struct spinor_info *sni);
#endif


__ramfunc int spi_flash_acts_init(const struct device *dev)
{
	struct spinor_info *sni = DEV_DATA(dev);
	uint32_t key;
	uint8_t status, status2, status3;

	sni->spi.prepare_hook = spi_flash_acts_prepare;

	key = irq_lock();
	sni->chipid = p_spinor_api->read_chipid(sni);

	printk("read spi nor chipid:0x%x\n", sni->chipid);

	status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
	status2 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	status3 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS3);

	printk("spinor status: {0x%02x 0x%02x 0x%02x}\n", status, status2, status3);

#if IS_ENABLED(CONFIG_NOR_ACTS_DQ_MODE_ENABLE)
	nor_dual_quad_read_mode_try(sni);
	printk("bus width : %d, and cache read use ", sni->spi.bus_width);

#else
	if(sni->spi.bus_width == 4) {
		printk("nor is 4 line mode\n");

		sni->spi.flag |= SPI_FLAG_SPI_4XIO;
		xspi_nor_enable_status_qe(sni);
		/* enable 4x mode */
		xspi_setup_bus_width(sni, 4);
	} else if(sni->spi.bus_width == 2) {
		printk("nor is 2 line mode\n");
		/* enable 2x mode */
		xspi_setup_bus_width(sni, 2);
	} else {
		sni->spi.bus_width = 1;
		printk("nor is 1 line mode\n");
		/* enable 1x mode */
		xspi_setup_bus_width(sni, 1);
	}
#endif
	/* setup SPI clock rate */
	clk_set_rate(CLOCK_ID_SPI0, MHZ(CONFIG_SPI_FLASH_FREQ_MHZ));

	/* configure delay chain */
	xspi_setup_delaychain(sni, sni->spi.delay_chain);

	/* check delay chain workable */
	sni->chipid = p_spinor_api->read_chipid(sni);

	printk("read again spi nor chipid:0x%x\n", sni->chipid);

#ifdef CONFIG_SPI_NOR_FLASH_4B_ADDRESS
	spinor_enter_4byte_address_mode(sni);
#endif

#if defined(CONFIG_SPI_FLASH_1_GPIO_CS_EN) && (CONFIG_SPI_FLASH_1_GPIO_CS_EN == 1)
	spi_gpio_cs_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(CONFIG_SPI_FLASH_1_GPIO_CS_PIN));
	if (!spi_gpio_cs_dev) {
		printk("failed to get gpio:%d device", CONFIG_SPI_FLASH_1_GPIO_CS_PIN);
		irq_unlock(key);
		return -1;
	}
	gpio_pin_configure(spi_gpio_cs_dev, CONFIG_SPI_FLASH_1_GPIO_CS_PIN % 32, GPIO_OUTPUT);
	gpio_pin_set(spi_gpio_cs_dev, CONFIG_SPI_FLASH_1_GPIO_CS_PIN % 32, 1);
	printk("use GPIO:%d as spi cs pin", CONFIG_SPI_FLASH_1_GPIO_CS_PIN);
#endif

	status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
	status2 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	status3 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS3);

	printk("spinor status: {0x%02x 0x%02x 0x%02x}\n", status, status2, status3);

#if IS_ENABLED(CONFIG_SPINOR_TEST_DELAYCHAIN)
	nor_test_delaychain(dev);
#endif

	irq_unlock(key);

	return 0;
}

#if defined(CONFIG_SPI_FLASH_1_GPIO_CS_EN) && (CONFIG_SPI_FLASH_1_GPIO_CS_EN == 1)
static void spi_flash_acts_cs_gpio(struct spi_info *si, int value)
{
	if (spi_gpio_cs_dev) {
		gpio_pin_set(spi_gpio_cs_dev, CONFIG_SPI_FLASH_1_GPIO_CS_PIN % 32, value ? true : false);
		k_busy_wait(1);
	}
}
#endif

#ifndef CONFIG_BOARD_NANDBOOT

typedef volatile unsigned int *REG32;

#define BROM_RES_OK            0
#define BROM_RES_FAIL          1

#define TIMER_CLK_FRE_KHZ       32

#define COUNT_TO_MSEC(x)    ((x) / TIMER_CLK_FRE_KHZ)
#define MSEC_TO_COUNT(x)    ((x) * TIMER_CLK_FRE_KHZ)

#define TIMER_MAX_VALUE     0xfffffffful

#define SPI0_CTL (SPI0_REG_BASE+0x0)
#define SPI0_STA (SPI0_REG_BASE+0x4)
#define SPI0_TXDAT (SPI0_REG_BASE+0x8)

unsigned int prv_timestamp_ms, overflow_ms;

static inline unsigned int timer_cnt_get(void)
{
	//printk("prv_timestamp_ms %d overflow_ms %d\n", prv_timestamp_ms, overflow_ms);
    return sys_read32(T0_CNT);
}

__sleepfunc unsigned int timestamp_get_ms(void)
{
    unsigned int curent_ms;

    curent_ms = COUNT_TO_MSEC(timer_cnt_get());
    if (curent_ms < prv_timestamp_ms)
    {
        // roll over
        overflow_ms += COUNT_TO_MSEC(TIMER_MAX_VALUE);
    }
    prv_timestamp_ms = curent_ms;

    return overflow_ms + curent_ms;
}

__sleepfunc unsigned int wait_reg_status(unsigned int reg, unsigned int dat, unsigned int check, unsigned int ms)
{
    unsigned int res;
    unsigned int wait_start = timestamp_get_ms();
    res = BROM_RES_FAIL;

    while (1)
    {
        if((*((REG32)reg) & dat) == check)
        {
            res = BROM_RES_OK;
            break;
        }
        if ((timestamp_get_ms() - wait_start) > (ms))
        {
            break;
        }
    }
    return res;
}

__sleepfunc unsigned int spi_wait_trans_complete(void)
{
    unsigned int res;
    res = wait_reg_status(SPI0_STA,0x10,0x10,50);      //wait tx fifo empty
    res = wait_reg_status(SPI0_STA,0x40,0,50);         //wait transmit complete
    return res;
}

__sleepfunc int nor_send_command(unsigned char cmd)
{
    unsigned int res=0;

    *((REG32)(SPI0_CTL)) |= 1<<3;            // cs = 1
    *((REG32)(SPI0_CTL)) &= ~(1<<3);     // cs = 0

    *((REG32)(SPI0_CTL)) &= ~3;  // spi disable
    // write cmd
    *((REG32)(SPI0_CTL)) = ((*((REG32)(SPI0_CTL)) & (~0x03 )) | (0x2));   // spi write only
    *((REG32)(SPI0_TXDAT)) = cmd; // send one byte command
    res |= spi_wait_trans_complete();

    // disable SPI
    *((REG32)(SPI0_CTL)) &= (~0x03);
    // set cs high
    *((REG32)(SPI0_CTL)) |= 1<<3;            // cs = 1
    return res;
}

__sleepfunc void  sys_norflash_power_ctrl(uint32_t is_powerdown)
{
	volatile int i;
	u32_t spi_ctl_ori = sys_read32(SPI0_REG_BASE);
	prv_timestamp_ms = COUNT_TO_MSEC(timer_cnt_get());
    overflow_ms = 0;

	/* If spi mode is not the disable or write only mode, we need to disable firstly */
	if (((spi_ctl_ori & 0x3) != 0) && ((spi_ctl_ori & 0x3) != 2)) {
		sys_write32(sys_read32(SPI0_REG_BASE) & ~(3 << 0), SPI0_REG_BASE);
		for (i = 0; i < 5; i++) {
			;
		}
	}

	/* enable AHB interface for cpu write cmd */
	sys_write32(0xa013A, SPI0_REG_BASE);
	for(i = 0; i < 20; i++) {
		;
	}

	if (is_powerdown){
		/* 4x io need send 0xFF to exit the continuous mode */
		if (spi_ctl_ori & (0x3 << 10))
			nor_send_command(0xFF);
		nor_send_command(0xB9);
		soc_udelay(5); // max 3us
	} else {
		nor_send_command(0xAB);
		soc_udelay(40); // max 30us
	}
	
	/* set spi in disable mode */
	sys_write32(sys_read32(SPI0_REG_BASE) & ~(3 << 0), SPI0_REG_BASE);
	for (i = 0; i < 5; i++) {
		;
	}

	sys_write32(spi_ctl_ori, SPI0_REG_BASE);

}
#endif

#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
static void spi_flash_acts_pages_layout(
				const struct device *dev,
				const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	*layout = &(DEV_CFG(dev)->pages_layout);
	*layout_size = 1;
}
#endif /* IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT) */

#if IS_ENABLED(CONFIG_NOR_ACTS_DATA_PROTECTION_ENABLE)
extern int nor_write_protection(struct device *dev, bool enable);
#endif

int spi_flash_acts_write_protection(const struct device *dev, bool enable)
{
#if IS_ENABLED(CONFIG_NOR_ACTS_DATA_PROTECTION_ENABLE)

#if defined(CONFIG_SPI_FLASH_SYNC_MULTI_DEV) && (CONFIG_SPI_FLASH_SYNC_MULTI_DEV == 1)
	k_sem_take(&spi_flash_sync, K_FOREVER);
#endif

	nor_write_protection(dev, enable);

#if defined(CONFIG_SPI_FLASH_SYNC_MULTI_DEV) && (CONFIG_SPI_FLASH_SYNC_MULTI_DEV == 1)
	k_sem_give(&spi_flash_sync);
#endif

#endif
	return 0;
}

static struct flash_driver_api spi_flash_nor_api = {
	.read = spi_flash_acts_read,
	.write = spi_flash_acts_write,
	.erase = spi_flash_acts_erase,
	.write_protection = spi_flash_acts_write_protection,
#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = spi_flash_acts_pages_layout,
#endif
};

/* system XIP spinor */
static struct spinor_info spi_flash_acts_data = {
	.spi = {
		.base = SPI0_REG_BASE,
		.bus_width = CONFIG_SPI_FLASH_BUS_WIDTH,
		.delay_chain = CONFIG_SPI_FLASH_DELAY_CHAIN,
#if (CONFIG_DMA_SPINOR_RESEVER_CHAN < CONFIG_DMA_0_PCHAN_NUM)
		.dma_base= (DMA_REG_BASE + 0x100 + (CONFIG_DMA_SPINOR_RESEVER_CHAN * 0x100)),
#endif
#if defined(CONFIG_SPI_FLASH_NO_IRQ_LOCK) && (CONFIG_SPI_FLASH_NO_IRQ_LOCK == 1)
		.flag = SPI_FLAG_NO_IRQ_LOCK,
#else
		.flag = 0,
#endif
	},
	.flag = 0,
};

static const struct spi_flash_acts_config spi_acts_config = {
#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
	.pages_layout = {
			.pages_count = CONFIG_SPI_FLASH_CHIP_SIZE/0x1000,
			.pages_size  = 0x1000,
		},
#endif
	.chip_size	 = CONFIG_SPI_FLASH_CHIP_SIZE,
	.page_size	 = 0x1000,
};

#if IS_ENABLED(CONFIG_SPI_FLASH_0)
DEVICE_DEFINE(spi_flash_acts, CONFIG_SPI_FLASH_NAME, &spi_flash_acts_init, NULL,
		    &spi_flash_acts_data, &spi_acts_config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &spi_flash_nor_api);
#endif

#if (CONFIG_SPI_FLASH_1 == 1)

/* system XIP spinor */
static struct spinor_info spi_flash_1_acts_data = {
	.spi = {
		.base = SPI0_REG_BASE,
		.bus_width = CONFIG_SPI_FLASH_1_BUS_WIDTH,
		.delay_chain = CONFIG_SPI_FLASH_1_DELAY_CHAIN,
#if (CONFIG_DMA_SPINOR_RESEVER_CHAN < CONFIG_DMA_0_PCHAN_NUM)
		.dma_base= (DMA_REG_BASE + 0x100 + (CONFIG_DMA_SPINOR_RESEVER_CHAN * 0x100)),
#endif
#if defined(CONFIG_SPI_FLASH_NO_IRQ_LOCK) && (CONFIG_SPI_FLASH_NO_IRQ_LOCK == 1)
		.flag = SPI_FLAG_NO_IRQ_LOCK,
#else
		.flag = 0,
#endif
#if defined(CONFIG_SPI_FLASH_1_GPIO_CS_EN) && (CONFIG_SPI_FLASH_1_GPIO_CS_EN == 1)
		.set_cs = spi_flash_acts_cs_gpio,
#endif
	},
	.flag = 0,
};

static const struct spi_flash_acts_config spi_flash_1_acts_config = {
#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
	.pages_layout = {
			.pages_count = CONFIG_SPI_FLASH_1_CHIP_SIZE/0x1000,
			.pages_size  = 0x1000,
		},
#endif
	.chip_size	 = CONFIG_SPI_FLASH_1_CHIP_SIZE,
	.page_size	 = 0x1000,
};

DEVICE_DEFINE(spi_flash_1_acts, CONFIG_SPI_FLASH_1_NAME, &spi_flash_acts_init, NULL,
		    &spi_flash_1_acts_data, &spi_flash_1_acts_config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &spi_flash_nor_api);
#endif

