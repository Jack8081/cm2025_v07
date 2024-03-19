/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

//#define DT_DRV_COMPAT actions_acts_flash

#include <errno.h>
#include <disk/disk_access.h>
#include "spinand_acts.h"

#include <drivers/spinand.h>
#include <board.h>

#define ID_TBL_MAGIC			0x53648673
#define ID_TBL_ADDR				soc_boot_get_nandid_tbl_addr()

#ifndef CONFIG_SPINAND_LIB
//spinand code rom api address
#define SPINAND_API_ADDR	    0x00006800
#define p_spinand_api	((struct spinand_operation_api *)SPINAND_API_ADDR)
#endif

#define STA_NODISK		0x02	/* No medium in the drive */
#define STA_DISK_OK		0x08	/* Medium OK in the drive */

#define LIB_LOG_LEVEL DEBUG_LEVEL
#define HEAP_SIZE (14*1024)
uint32_t api_bss[HEAP_SIZE/4];

struct k_mutex mutex;

#define SECTOR_SIZE 512

#include <logging/log.h>
LOG_MODULE_REGISTER(spinand_acts, CONFIG_FLASH_LOG_LEVEL);


static struct nand_info system_spinand3 = {
    .base = 0x40034000,
    .bus_width = 4,
    .delay_chain = 0,
    .data = 1,
    .dma_base = 0x4001CA00, //DMA9
    .printf = (void *)printk,
    .loglevel = LIB_LOG_LEVEL,
};

static struct spinand_info spinand_acts_data = {
    .protect = 1,
};

#ifndef CONFIG_SPINAND_LIB
#define SPI_CTL                                     (0x00)
#define SPI_STAT                                    (0x04)
#define SPI_TXDAT                                   (0x08)
#define SPI_RXDAT                                   (0x0C)
#define SPI_TCNT                                    (0x10)

#define SPI_DELAY(s)						(s->spi->delay_chain)

enum
{
    DISABLE,
    SPIMODE_READ_ONLY,
    SPIMODE_WRITE_ONLY,
    SPIMODE_RW,
};

enum
{
    TWO_X_FOUR_X,
    DUAL_QUAD,
};

enum
{
    ONE_X_IO_0,
    ONE_X_IO_1,
    TWO_X_IO,
    FOUR_X_IO,
};

static inline uint32_t reg_read(struct spinand_info *si, uint32_t reg)
{
    return sys_read32(si->spi->base + reg);
}

static inline void reg_write(struct spinand_info *si, uint32_t value, uint32_t reg)
{
    sys_write32(value, si->spi->base + reg);
}

#define SPI0_BASE 0x40028000
#define SPI1_BASE 0x4002C000
#define SPI2_BASE 0x40030000
static inline int spi_controller_num(struct spinand_info *si)
{
    if (si->spi->base == (SPI0_BASE)) {
        return 0;
    } else if (si->spi->base == (SPI1_BASE)) {
        return 1;
    } else if (si->spi->base == (SPI2_BASE)) {
        return 2;
    } else {
        return 3;
    }
}

void SPI_SET_TX_CPU(struct spinand_info *sni)
{
    if (spi_controller_num(sni) == 0) {
        /* SPI0 */
        reg_write(sni, (reg_read(sni, SPI_CTL) | (SPI_DELAY(sni) << 16) | (1 << 8)), SPI_CTL);
    } else if (spi_controller_num(sni) == 1) {
        /* SPI1 */
        reg_write(sni, (reg_read(sni, SPI_CTL) | (SPI_DELAY(sni) << 16) | (1 << 15)), SPI_CTL);
    } else if (spi_controller_num(sni) == 2) {
        /* SPI2 */
        reg_write(sni, ((reg_read(sni, SPI_CTL) & ~(1U << 31 | 1 << 30 | 3 << 0)) | (1 << 15) | (1 << 5) | (0x7 << 16)), SPI_CTL);
    } else {
        /* SPI3 */
        reg_write(sni, (reg_read(sni, SPI_CTL) | (SPI_DELAY(sni) << 16)), SPI_CTL);
    }
}

void SPI_RESET_FIFO(struct spinand_info *sni)
{
    if (spi_controller_num(sni) == 0) {
        /* SPI0 */
        //Only bit7, 1 can be written.
        reg_write(sni, 0x82, SPI_STAT);
    } else {
        /* SPI3 */
        //Only bit2,3,8,9,11 can be written.
        reg_write(sni, 0xb0c, SPI_STAT);
    }
}

void spinand_basic_set_to_send_mode(struct spinand_info *sni)
{
    SPI_SET_TX_CPU(sni);
    SPI_RESET_FIFO(sni);
}

void SPI_SET_DUAL_MODE(struct spinand_info *sni, uint8_t mode)
{
    reg_write(sni, (reg_read(sni, SPI_CTL) & ~(1 << 27)) | ((mode) << 27), SPI_CTL);
}

void SPI_SET_IO_MODE(struct spinand_info *sni, uint8_t mode)
{
    reg_write(sni, (reg_read(sni, SPI_CTL) & ~(3 << 10)) | ((mode) << 10), SPI_CTL);
}

void SPI_WRITE_START(struct spinand_info *sni)
{
    reg_write(sni, (reg_read(sni, SPI_CTL) | (1 << 5)), SPI_CTL);
}

void SPI_SET_RW_MODE(struct spinand_info *sni, uint8_t mode)
{
    reg_write(sni, (reg_read(sni, SPI_CTL) & ~(3 << 0)) | ((mode) << 0), SPI_CTL);
}

void WAIT_SPI_TX_NOT_FULL(struct spinand_info *sni)
{
    while(reg_read(sni, SPI_STAT) & (1 << 5));
}

void WRITE_TX_FIFO(struct spinand_info *sni, uint32_t val)
{
    reg_write(sni, val, SPI_TXDAT);
}

void WAIT_SPI_TX_EMPTY(struct spinand_info *sni)
{
    while(!(reg_read(sni, SPI_STAT) & (1 << 4)));
}

void WAIT_SPI_BUS_IDLE(struct spinand_info *sni)
{
    if (spi_controller_num(sni) == 0) {
        /* SPI0 */
        while(reg_read(sni, SPI_STAT) & (1 << 6));
    } else {
        /* SPI1/2/3 */
        while(reg_read(sni, SPI_STAT) & (1 << 0));
    }
}

void SPI_REVERSE(struct spinand_info *sni)
{
    if (spi_controller_num(sni) == 0) {
        /* SPI0 */
        reg_write(sni, reg_read(sni, SPI_CTL) & ~(3 << 0 | 1U << 31 | 3 << 4 | 3 << 6 | 15 << 16), SPI_CTL);
    } else {
        /* SPI1/2/3 */
        reg_write(sni, reg_read(sni, SPI_CTL) & ~(3 << 0 | 1U << 31 | 3 << 4 | 3 << 6 | 15 << 16), SPI_CTL);
    }
}

void spinand_basic_send_data_with_byte(struct spinand_info *sni, uint8_t *send_buf, uint32_t len)
{
    spinand_basic_set_to_send_mode(sni);

    SPI_SET_DUAL_MODE(sni, TWO_X_FOUR_X);
    SPI_SET_IO_MODE(sni, ONE_X_IO_0);

    SPI_WRITE_START(sni);

    SPI_SET_RW_MODE(sni, SPIMODE_WRITE_ONLY);
    while (len-- > 0)
    {
        WAIT_SPI_TX_NOT_FULL(sni);
        WRITE_TX_FIFO(sni, *send_buf++);
    }
    WAIT_SPI_TX_EMPTY(sni);

    WAIT_SPI_BUS_IDLE(sni);
    SPI_REVERSE(sni);
}

void SPI_SET_RX_CPU(struct spinand_info *sni)
{
    if (spi_controller_num(sni) == 0) {
        /* SPI0 */
        reg_write(sni, (reg_read(sni, SPI_CTL) | (SPI_DELAY(sni) << 16) | (1 << 8)), SPI_CTL);
    } else if (spi_controller_num(sni) == 1) {
        /* SPI1 */
        reg_write(sni, (reg_read(sni, SPI_CTL) | (SPI_DELAY(sni) << 16) | (1 << 15)), SPI_CTL);
    } else if (spi_controller_num(sni) == 2) {
        /* SPI2 */
        reg_write(sni, ((reg_read(sni, SPI_CTL) & ~(1U << 31 | 1 << 30 | 3 << 0)) | (0x7 << 16) | (1 << 15)), SPI_CTL);
    } else {
        /* SPI3 */
        reg_write(sni, (reg_read(sni, SPI_CTL) | (SPI_DELAY(sni) << 16)), SPI_CTL);
    }
}

void spinand_basic_set_to_receive_mode(struct spinand_info *sni)
{
    SPI_SET_RX_CPU(sni);
    SPI_RESET_FIFO(sni);
}

void SPI_SET_READ_COUNT(struct spinand_info *sni, uint32_t val)
{
    reg_write(sni, val, SPI_TCNT);
}

void SPI_READ_START(struct spinand_info *sni)
{
    reg_write(sni, (reg_read(sni, SPI_CTL) | (1 << 4)), SPI_CTL);
}

void WAIT_SPI_RX_NOT_EMPTY(struct spinand_info *sni)
{
    if (spi_controller_num(sni) == 0) {
        /* SPI0 */
        while(reg_read(sni, SPI_STAT) & (1 << 2));
    } else {
        /* SPI1/2/3 */
        while(reg_read(sni, SPI_STAT) & (1 << 6));
    }
}

uint32_t READ_RX_FIFO(struct spinand_info *sni)
{
    return reg_read(sni, SPI_RXDAT);
}

void spinand_basic_receive_data_with_byte(struct spinand_info *sni, uint8_t *receive_buf, uint32_t len)
{
    spinand_basic_set_to_receive_mode(sni);
    //1xIO
    SPI_SET_DUAL_MODE(sni, TWO_X_FOUR_X);
    SPI_SET_IO_MODE(sni, ONE_X_IO_0);

    SPI_SET_READ_COUNT(sni, len);
    SPI_READ_START(sni);

    //Must be start read at last, otherwise nothing can be read!
    SPI_SET_RW_MODE(sni, SPIMODE_READ_ONLY);
    while (len-- > 0)
    {
        WAIT_SPI_RX_NOT_EMPTY(sni);
        *receive_buf++ = READ_RX_FIFO(sni);
    }

    SPI_REVERSE(sni);
}

void SPI_CS_LOW(struct spinand_info *sni)
{
    reg_write(sni, (reg_read(sni, SPI_CTL) & (~(1 << 3))), SPI_CTL);
}

void SPI_CS_HIGH(struct spinand_info *sni)
{
    reg_write(sni, (reg_read(sni, SPI_CTL) | (1 << 3)), SPI_CTL);
}

void spinand_basic_select_spinand(struct spinand_info *sni, uint8_t select)
{
    if (select)
        SPI_CS_LOW(sni);
    else
        SPI_CS_HIGH(sni);
}

#define SPINAND_FEATURE_SECURE_ADDRESS				0xB0
#define SPINAND_GET_FEATURE_COMMAND					0x0F
#define SPINAND_SET_FEATURE_COMMAND					0x1F

/*feature value should be A0/B0/C0/D0/F0H*/
uint8_t spinand_get_feature(struct spinand_info *sni, uint8_t feature)
{
    uint8_t status;
    uint8_t cmd_addr[2] = {SPINAND_GET_FEATURE_COMMAND, feature};

    spinand_basic_select_spinand(sni, 1);
    spinand_basic_send_data_with_byte(sni, cmd_addr, sizeof(cmd_addr));
    spinand_basic_receive_data_with_byte(sni, &status, 1);
    spinand_basic_select_spinand(sni, 0);

    return status;
}

/*feature value should be A0/B0/C0/D0/F0H*/
void spinand_set_feature(struct spinand_info *sni, uint8_t feature, uint8_t setting)
{
    uint8_t cmd_addr[3] = {SPINAND_SET_FEATURE_COMMAND, feature, setting};

    spinand_basic_select_spinand(sni, 1);
    spinand_basic_send_data_with_byte(sni, cmd_addr, sizeof(cmd_addr));
    spinand_basic_select_spinand(sni, 0);
}
#endif

static int spinand_acts_read(const struct device *dev, off_t offset, void *data, size_t len)
{
    //offset, len shuold align with 512B.
    struct spinand_info *sni = &spinand_acts_data;
    int ret = 0;

    //printk("read offset = %d; len = %d;\n", offset, len);

    k_mutex_lock(&mutex, K_FOREVER);
#ifndef CONFIG_SPINAND_LIB
    ret =  p_spinand_api->read(sni, offset, data, len);
#else
    ret = spinand_read(sni, offset, data, len);
#endif
    k_mutex_unlock(&mutex);

    return ret;
}

static int spinand_acts_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
    //offset, len shuold align with 512B.
    struct spinand_info *sni = &spinand_acts_data;
    int ret = 0;

    //printk("write offset = %d; len = %d;\n", offset, len);

    k_mutex_lock(&mutex, K_FOREVER);
#ifndef CONFIG_SPINAND_LIB
    ret = p_spinand_api->write(sni, offset, data, len);
#else
    ret = spinand_write(sni, offset, data, len);
#endif
    k_mutex_unlock(&mutex);

    return ret;
}

static int spinand_acts_erase(const struct device *dev, off_t offset, size_t size)
{
    //offset, len shuold align with 512B.
    struct spinand_info *sni = &spinand_acts_data;
    int ret = 0;

    printk("erase offset = %d; len = %d;\n", offset, size);

    k_mutex_lock(&mutex, K_FOREVER);
#ifndef CONFIG_SPINAND_LIB
    ret = p_spinand_api->erase(sni, offset, size);
#else
    ret = spinand_erase(sni, offset, size);
#endif
    k_mutex_unlock(&mutex);

    return ret;
}

#ifdef CONFIG_SPINAND_LIB
static int spinand_acts_flush(const struct device *dev, bool efficient)
{
    //offset, len shuold align with 512B.
    struct spinand_info *sni = &spinand_acts_data;
    int ret = 0;

    k_mutex_lock(&mutex, K_FOREVER);
    ret = spinand_flush(sni, efficient);
    k_mutex_unlock(&mutex);

    return ret;
}
#endif

int get_storage_params(struct spinand_info *sni, u8_t *id, struct FlashChipInfo **ChipInfo)
{
    int i, j;

    struct NandIdTblHeader *id_tbl_header = (struct NandIdTblHeader *)sni->id_tbl;
    struct FlashChipInfo *id_tbl = (struct FlashChipInfo *)((uint8_t *)sni->id_tbl + sizeof(struct NandIdTblHeader));

    if (id_tbl_header->magic != ID_TBL_MAGIC)
    {
        LOG_ERR("id tbl magic err! 0x%x \n", id_tbl_header->magic);
        return -1;
    }

    for (i = 0; i < id_tbl_header->num; i++)
    {
        for (j = 0; j < NAND_CHIPID_LENGTH / 2; j++)
        {
            /* skip compare the 0xff value */
            if ((id_tbl[i].ChipID[j] != 0xff) && (id_tbl[i].ChipID[j] != id[j]))
            {
                LOG_ERR("id not match; id_tbl[%d].ChipID[%d] = 0x%x; id[%d] = 0x%x\n", i, j, id_tbl[i].ChipID[j], j, id[j]);
                break;
            }
            LOG_DBG("id match; id_tbl[%d].ChipID[%d] = 0x%x; id[%d] = 0x%x\n", i, j, id_tbl[i].ChipID[j], j, id[j]);
        }

        if (j == NAND_CHIPID_LENGTH / 2)
        {
            *ChipInfo = &id_tbl[i];
            //LOG_DBG("get chipinfo.\n");
            return 0;
        }
    }

    return -1;
}

static int spinand_get_chipid(struct spinand_info *sni, u32_t *chipid)
{
    int i = 0;

retry:
#ifndef CONFIG_SPINAND_LIB
    *chipid = p_spinand_api->read_chipid(sni);
#else
    *chipid = spinand_read_chipid(sni);
#endif
    LOG_DBG("nand id = 0x%x\n", *chipid);
    if (*chipid == 0x0 || *chipid == 0xffffffff) {
        if (i++ < 3) {
            LOG_ERR("Can't get spinand id, retry %d...\n", i);
            goto retry;
        } else {
            LOG_ERR("Can't get spinand id, Please check!\n");
            return -1;
        }
    }

    return 0;
}

static int spinand_get_delaychain(struct spinand_info *sni)
{
    uint32_t chipid = 0;
    struct FlashChipInfo *NandFlashInfo;

    /* setup SPI clock rate */
    clk_set_rate(CLOCK_ID_SPI3, MHZ(16));

    if (spinand_get_chipid(sni, &chipid) != 0) {
        LOG_ERR("spinand get chipid err!\n");
        return -EINVAL;
    }
    if (get_storage_params(sni, (u8_t *)&chipid, &NandFlashInfo) != 0) {
        LOG_ERR("Can't get flashinfo.\n");
        return -1;
    }

    return NandFlashInfo->delayChain;
}

int spinand_storage_ioctl(const struct device *dev, uint8_t cmd, void *buff)
{
    struct spinand_info *sni = &spinand_acts_data;
    uint32_t chipid = 0;
    struct FlashChipInfo *NandFlashInfo;

    switch (cmd) {
    case DISK_IOCTL_CTRL_SYNC:
#ifdef CONFIG_SPINAND_LIB
        spinand_acts_flush(dev, 1);
#endif
        break;
    case DISK_IOCTL_GET_SECTOR_COUNT:
        if (spinand_get_chipid(sni, &chipid) != 0) {
            LOG_ERR("spinand get chipid err!\n");
            return -EINVAL;
        }
        //printk("nand id = 0x%x\n", chipid);
        if (get_storage_params(sni, (u8_t *)&chipid, &NandFlashInfo) != 0) {
            LOG_ERR("Can't get flashinfo.\n");
            return -1;
        }
        *(uint32_t *)buff = NandFlashInfo->DefaultLBlkNumPer1024Blk * NandFlashInfo->SectorNumPerPage * NandFlashInfo->PageNumPerPhyBlk;
        break;
    case DISK_IOCTL_GET_SECTOR_SIZE:
        *(uint32_t *)buff = SECTOR_SIZE;
        break;
    case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
        *(uint32_t *)buff  = SECTOR_SIZE;
        break;
    case DISK_IOCTL_HW_DETECT:
        if (spinand_get_chipid(sni, &chipid) != 0) {
            *(uint8_t *)buff = STA_NODISK;
        } else {
            *(uint8_t *)buff = STA_DISK_OK;
        }
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

static int spinand_power_manager(bool on)
{
    if (CONFIG_SPINAND_USE_GPIO_POWER) {
        int ret = 0;
        const struct device *power_gpio_dev;
        uint8_t power_gpio = CONFIG_SPINAND_POWER_GPIO % 32;
        power_gpio_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(CONFIG_SPINAND_POWER_GPIO));

        if (!power_gpio_dev) {
            LOG_ERR("Failed to bind spinand power GPIO(%d:%s)",
            power_gpio, CONFIG_GPIO_PIN2NAME(CONFIG_SPINAND_POWER_GPIO));
            return -1;
        }

        ret = gpio_pin_configure(power_gpio_dev, power_gpio, GPIO_OUTPUT);
        if (ret) {
            LOG_ERR("Failed to config output GPIO:%d", power_gpio);
            return ret;
        }

        if (on) {
            /* power on spinand */
            gpio_pin_set(power_gpio_dev, power_gpio, CONFIG_SPINAND_GPIO_POWER_LEVEL);
            k_busy_wait(10*1000);
        } else {
            /* power off spinand */
            gpio_pin_set(power_gpio_dev, power_gpio, !CONFIG_SPINAND_GPIO_POWER_LEVEL);
        }

        //printk("GPIO64 = 0x%x \n", sys_read32(0x40068104));
        //printk("GPIO64 Output = 0x%x \n", sys_read32(0x40068208));
    }

    return 0;
}

static int spinand_env_init(struct spinand_info *sni)
{
    uint8_t feture;
    uint32_t chipid = 0;

    if (spinand_power_manager(1) != 0) {
        LOG_ERR("spinand power on failed!\n");
        return -1;
    }

    /* enable spi3 controller clock */
    acts_clock_peripheral_enable(CLOCK_ID_SPI3);

    /* reset spi3 controller */
    acts_reset_peripheral(RESET_ID_SPI3);

    if (sni->spi->delay_chain == 0) {
        sni->spi->delay_chain = spinand_get_delaychain(sni);
        if (sni->spi->delay_chain < 0) {
            LOG_ERR("spinand get delaychain failed!\n");
            return -1;
        }
        LOG_INF("spinand set delaychain %d.\n", sni->spi->delay_chain);
    }

    /* setup SPI clock rate */
    clk_set_rate(CLOCK_ID_SPI3, MHZ(100));

    //if use 4xIO, SIO2 & SIO3 do not need pull up, otherwise, SIO2 & SIO3 need setting pull up.
    if (sni->spi->bus_width == 4) {
        feture = spinand_get_feature(sni, 0xB0);
        spinand_set_feature(sni, 0xB0, feture | 0x1);
    }

    //For debug only.
    if (0) {
        LOG_DBG("MRCR0 = 0x%x \n", sys_read32(RMU_MRCR0));
        //bit7 for spi3
        LOG_DBG("CMU_DEVCKEN0 = 0x%x \n", sys_read32(CMU_DEVCLKEN0));
        LOG_DBG("CORE_PLL = 0x%x \n", sys_read32(COREPLL_CTL));
        LOG_DBG("SPI3CLK  = 0x%x \n", sys_read32(CMU_SPI3CLK));
        LOG_DBG("GOIO64   = 0x%x \n", sys_read32(0x40068104));
        LOG_DBG("GPIO64 Output = 0x%x \n", sys_read32(0x40068208));
        LOG_DBG("GPIO8  = 0x%x \n", sys_read32(0x40068024));
        LOG_DBG("GPIO9  = 0x%x \n", sys_read32(0x40068028));
        LOG_DBG("GPIO10 = 0x%x \n", sys_read32(0x4006802c));
        LOG_DBG("GPIO11 = 0x%x \n", sys_read32(0x40068030));
        LOG_DBG("GPIO12 = 0x%x \n", sys_read32(0x40068034));
        LOG_DBG("GPIO13 = 0x%x \n", sys_read32(0x40068038));
    }

    if (spinand_get_chipid(sni, &chipid) != 0) {
        LOG_ERR("spinand get chipid err!\n");
        return -1;
    }

    return 0;
}

static int spinand_acts_init(const struct device *dev)
{
    int ret = 0;
    //const struct spinand_acts_config *config = DEV_CFG(dev);
    struct spinand_info *sni = &spinand_acts_data;

    sni->bss = api_bss;
    memset(sni->bss, 0x0, HEAP_SIZE);
    sni->id_tbl = (void *)soc_boot_get_nandid_tbl_addr();
    sni->spi = &system_spinand3;

    if (spinand_env_init(sni) != 0) {
        LOG_ERR("spinand env init err.\n");
        return -1;
    }

#ifndef CONFIG_SPINAND_LIB
    ret = p_spinand_api->init(sni);
#else
    ret = spinand_init(sni);
#endif
    if (ret != 0) {
        LOG_ERR("SPINand rom driver init err.\n");
        return -1;
    }

    k_mutex_init(&mutex);

    return 0;
}

#ifdef CONFIG_PM_DEVICE
static u32_t bss_checksum;
#define CHECKSUM_XOR_VALUE      0xaa55
u32_t spinand_checksum(u32_t *buf, u32_t len)
{
    u32_t i;
    u32_t sum = 0;

    for (i = 0; i < len; i++)
    {
        sum += buf[i];
    }

    sum ^= (uint16_t)CHECKSUM_XOR_VALUE;

    return sum;
}

int spinand_pm_control(const struct device *device, enum pm_device_action action)
{
    int ret;
    u32_t tmp_cksum;
    struct spinand_info *sni = &spinand_acts_data;

    switch (action) {
    case PM_DEVICE_ACTION_RESUME:
        LOG_INF("spinand resume ...\n");
        tmp_cksum = spinand_checksum(api_bss, sizeof(api_bss)/4);
        if (bss_checksum != tmp_cksum) {
            LOG_ERR("SPINand resume err! api_bss is changed! suspend_checksum = 0x%x; resume_checksum = 0x%x \n", bss_checksum, tmp_cksum);
            return -1;
        }
        //LOG_INF("resume_checksum = 0x%x; sizeof(api_bss) = 0x%x \n", tmp_cksum, sizeof(api_bss));
        k_mutex_lock(&mutex, K_FOREVER);

        if (spinand_env_init(sni) != 0) {
            LOG_ERR("spinand env init err.\n");
            k_mutex_unlock(&mutex);
            return -1;
        }

#ifndef CONFIG_SPINAND_LIB
        ret = p_spinand_api->init(sni);
#else
        ret = spinand_pdl_init(sni);
#endif
        if (ret != 0) {
            LOG_ERR("SPINand rom driver init err.\n");
            k_mutex_unlock(&mutex);
            return -1;
        }

        k_mutex_unlock(&mutex);
        break;
    case PM_DEVICE_ACTION_SUSPEND:
        LOG_INF("spinand suspend ...\n");
        k_mutex_lock(&mutex, K_FOREVER);
        if (spinand_power_manager(0) != 0) {
            LOG_ERR("spinand power on failed!\n");
            k_mutex_unlock(&mutex);
            return -1;
        }
        k_mutex_unlock(&mutex);
        bss_checksum = spinand_checksum(api_bss, sizeof(api_bss)/4);
        //LOG_INF("suspend_checksum = 0x%x; sizeof(api_bss) = 0x%x \n", bss_checksum, sizeof(api_bss));
        break;
    case PM_DEVICE_ACTION_EARLY_SUSPEND:
        break;
    case PM_DEVICE_ACTION_LATE_RESUME:
        break;
    default:
        ret = -EINVAL;
    }
    return 0;
}
#else
#define adc_pm_control 	NULL
#endif

static struct flash_driver_api spinand_api = {
    .read = spinand_acts_read,
    .write = spinand_acts_write,
    .erase = spinand_acts_erase,
    .write_protection = NULL,
#ifdef CONFIG_SPINAND_LIB
    .flush = spinand_acts_flush,
#endif
};

#define SPINAND_ACTS_DEVICE_INIT(n)						\
    DEVICE_DEFINE(spinand, CONFIG_SPINAND_FLASH_NAME, \
            &spinand_acts_init, spinand_pm_control, \
            &spinand_acts_data, NULL, POST_KERNEL, \
            CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &spinand_api);

#if IS_ENABLED(CONFIG_SPINAND_3)
    SPINAND_ACTS_DEVICE_INIT(3)
#endif

