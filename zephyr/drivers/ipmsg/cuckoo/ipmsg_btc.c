/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <errno.h>
#include <string.h>
#include <device.h>
#include <soc.h>
#include "soc_memctrl.h"
#include <drivers/ipmsg.h>
#include <sdfs.h>
#include <board_cfg.h>
#include <soc_atp.h>
#include <sys/byteorder.h>
#ifdef CONFIG_CFG_DRV
#include <config.h>
#include <drivers/cfg_drv/driver_config.h>
#endif


#define BTC_RAM_FUNC	__ramfunc

#define DEBUGSEL (0x40068400)
#define DEBUGOE  (0x40068420)
#define DEBUGIE  (0x40068410)

#define HOSC_CTL_HOSCI_CAP_SHIFT 24
#define HOSC_CTL_HOSCI_CAP(x)	 ((x) << HOSC_CTL_HOSCI_CAP_SHIFT)
#define HOSC_CTL_HOSCI_CAP_MASK  HOSC_CTL_HOSCI_CAP(0xff)
#define HOSC_CTL_HOSCO_CAP_SHIFT 16
#define HOSC_CTL_HOSCO_CAP(x)	 ((x) << HOSC_CTL_HOSCO_CAP_SHIFT)
#define HOSC_CTL_HOSCO_CAP_MASK  HOSC_CTL_HOSCO_CAP(0xff)
#define HOSC_CTL_HGMC_SHIFT		8
#define HOSC_CTL_HGMC_MASK		(0x3 << HOSC_CTL_HGMC_SHIFT)

/* CMU_MEMCLKEN1 */
#define BT_ROM_RAM_CLK_EN	BIT(24)

/* CMU_MEMCLKSRC0 */
#define BT_RAM_CLK_SRC		BIT(24)

/* CMU_S1CLKCTL */
#define RC64M_S1EN			BIT(2)
#define HOSC_S1EN			BIT(1)
#define RC4M_S1EN			BIT(0)

/* CMU_S1BTCLKCTL */
#define RC64M_S1BTEN		BIT(2)
#define HOSC_S1BTEN			BIT(1)
#define RC4M_S1BTEN			BIT(0)

/* CMU_DEVCLKEN1 */
#define BTHUB_RC32KEN		BIT(28)
#define BTHUB_RC64MEN		BIT(27)
#define BTHUB_HOSCEN  		BIT(26)
#define BTHUB_RC4MEN  		BIT(25)
#define BTHUB_LOSCEN  		BIT(24)

#define JTAG_CTL_BTSWEN		BIT(20)

/* DEBUGSEL */
#define DBGSE_SHIFT			0
#define DBGSE(x)			((x) << DBGSE_SHIFT)
#define DBGSE_BT_CONTROLLER DBGSE(0xd)


#if defined(CONFIG_BTC_FW_IN_NAND)
#define BT_RF_FILE         "/NAND:K/bt_rf.bin"
#define BT_TABLE_FILE      "/NAND:K/bttbl.bin"
#define BT_PATCH_FILE      "/NAND:K/bt_pth.bin"
#define BT_FCC_FILE        "/NAND:K/fcc.bin"
#define BT_NST_FILE        "/NAND:K/nst.bin" /*NoSignaling Test*/
#elif  defined(CONFIG_BTC_FW_IN_SD)
#define BT_RF_FILE         "/SD:K/bt_rf.bin"
#define BT_TABLE_FILE      "/SD:K/bttbl.bin"
#define BT_PATCH_FILE      "/SD:K/bt_pth.bin"
#define BT_FCC_FILE        "/SD:K/fcc.bin"
#define BT_NST_FILE        "/SD:K/nst.bin" /*NoSignaling Test*/
#elif  defined(CONFIG_BTC_FW_IN_NOR_EXT)
#define BT_RF_FILE         "/NOR:K/bt_rf.bin"
#define BT_TABLE_FILE      "/NOR:K/bttbl.bin"
#define BT_PATCH_FILE      "/NOR:K/bt_pth.bin"
#define BT_FCC_FILE        "/NOR:K/fcc.bin"
#define BT_NST_FILE        "/NOR:K/nst.bin" /*NoSignaling Test*/
#else
#define BT_RF_FILE         "bt_rf.bin"
#define BT_TABLE_FILE      "bttbl.bin"
#define BT_PATCH_FILE      "bt_pth.bin"
#define BT_FCC_FILE        "fcc.bin"
#define BT_NST_FILE        "nst.bin" /*NoSignaling Test*/
#endif

#define BT_RAM1_ADDR       (0x01206000)
#define BT_RAM_TABLE_ADDR  (0x01200000)
#define BT_RAM_PATCH_ADDR  (0x01201400)


/* Config pos from bt_table_config.txt */
#define BT_MAX_RF_POWER_POS_1 				0x22		/* tx_max_pwr_lvl */
#define BT_DEFAULT_TX_POWER					0x23
#define BT_MAX_RF_POWER_POS_2 				0x25		/* tws_max_pwr_lvl */
#define BT_CFG_WAKEUP_ADVANCE_POS			0x26
#define BT_BLE_RF_POWER_POS   				0x29		/* le_tx_pwr_lvl */
#define BT_SNIFF_ATTEMP_POS                 0x2D        /*sniff_attemp*/
#define BT_CFG_FIX_MAX_PWR_POS				0x40
#define BT_CFG_FIX_MAX_PWR_BIT				0x3
#define BT_CFG_DISABLE_3M_POS				0x41
#define BT_CFG_DISABLE_3M_BIT				0x1			/* Set to 1 */
#define BT_CFG_TWS_PRI_ENC_POS				0x41
#define BT_CFG_TWS_PRI_ENC_BIT				0x2			/* Set to 1 */
#define BT_CFG_SEL_32768_POS				0x42
#define BT_CFG_SEL_32768_BIT				0x0			/* Set to 1 */
#define BT_CFG_K192_BY_HOST_POS				0x42
#define BT_CFG_K192_BY_HOST_BIT				0x1
#define BT_CFG_FORCE_LIGHT_SLEEP_POS		0x42
#define BT_CFG_FORCE_LIGHT_SLEEP_BIT		0x5			/* Set to 0 */
#define BT_CFG_RF_BQB_SETTING_POS			0x43
#define BT_CFG_RF_BQB_SETTING_BIT			0x1			/* normal run set to 0, bqb mode rf set to 1, default value 0 */
#define BT_CFG_FORCE_UART_PRINT_POS			0x42
#define BT_CFG_FORCE_UART_PRINT_BIT			0x7			/* Set to 0 */
#define BT_CFG_TRANS_EN_POS					0x43
#define BT_CFG_TRANS_EN_BIT					0x5			/* Set to 1 */
#define BT_CFG_SEL_RC32K_POS				0x44
#define BT_CFG_SEL_RC32K_BIT				0x2			/* Set to 0 */

#define BT_CFG_RF_8_TO_10DB_1V8_POS			0x44
#define BT_CFG_RF_8_TO_10DB_1V8_BIT			0x4			/* Set to 1 */

#define BT_CFG_UART_BAUD_RATE_POS			0x4C		/* 0x4c, 0x4d,0x4e,0x4f Little-endian */
#define BT_CFG_UART_BAUD_RATE				2000000		/* 2M */
#define BT_CFG_EFUSE_SET_POS				0x43
#define BT_CFG_EFUSE_SET_BIT				0x4
#define BT_CFG_EFUSE_AVDD_IF_POS			0x50
#define BT_CFG_EFUSE_POWER_VER_POS			0x54
#define BT_CFG_EFUSE_RF_POS					0x58

#define BT_DCDC_MODE 0

#define EFUSE_POWER_VER 0
#define EFUSE_RF		1
#define EFUSE_AVDD_IF	2

#define EFUSE_RF_FOR_VER_C	4


struct ipmsg_btc_config {
	void *mem_base;
	uint32_t mem_size;
};

struct ipmsg_btc_data {
	ipmsg_callback_t btc_cb;
	ipmsg_callback_t tws0_cb;
	ipmsg_callback_t tws1_cb;
	ipmsg_pm_ctrl_callback_t pm_ctrl_cb;
};


static ipmsg_btc_init_param_t btc_set_param;
static struct btc_debug_config btc_usr_config;

DEVICE_DECLARE(btc);

#ifdef CONFIG_BT_CTRL_BLE_BQB
#define BT_ANT_SW_SEL	23
static const struct acts_pin_config btc_pin_cfg_bqb[] = {
	{56, BT_ANT_SW_SEL | GPIO_CTL_GPIO_OUTEN},
	{57, BT_ANT_SW_SEL | GPIO_CTL_GPIO_OUTEN},
};
#endif

struct btc_debug_config*  ipmsg_get_btc_debug_config(void)
{
	return &btc_usr_config;
}


static void ipmsg_btc_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct ipmsg_btc_data *data = dev->data;
	
	//clear irq pending
	sys_write32(1, PENDING_FROM_BT_CPU);

	if (data->btc_cb) {
		data->btc_cb(NULL, NULL);
	}
}

static void ipmsg_tws0_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct ipmsg_btc_data *data = dev->data;

	if (data->tws0_cb) {
		data->tws0_cb(NULL, NULL);
	}
}

static void ipmsg_tws1_isr(void *arg)
{
#ifndef CONFIG_IRQ_VECTOR_IN_SRAM
	struct device *dev = (struct device *)arg;
	struct ipmsg_btc_data *data = dev->data;

	if (data->tws1_cb) {
		data->tws1_cb(NULL, NULL);
	}
#endif
}

extern void vect_isr_install(unsigned int irq, void (*isr_fun)(void));
static struct device *btc_dev;

BTC_RAM_FUNC static void ipmsg_tws1_isr_ram(void)
{
#ifdef CONFIG_IRQ_VECTOR_IN_SRAM
	struct ipmsg_btc_data *data;

	if (btc_dev) {
		data = btc_dev->data;
		if (data->tws1_cb) {
			data->tws1_cb(NULL, NULL);
		}
	}
#endif
}

static void ipmsg_btc_init_param(struct device *dev, void *param)
{
	memcpy(&btc_set_param, param, sizeof(btc_set_param));
}

static int sd_load(const char *filename, void *dst)
{
	struct sd_file *sdf;
	int ret;

	sdf = sd_fopen(filename);
    if (!sdf) {
		printk("open %s failed!\n", filename);
        return -EINVAL;
    }

    ret = sd_fread(sdf, dst, sdf->size);
	printk("%s size: %d, load: %d\n", filename, sdf->size, ret);

	if (ret == sdf->size) {
		ret = 0;
	} else {
		printk("load %s failed!\n", filename);
		ret = -EINVAL;
	}
	
	sd_fclose(sdf);
	return ret;
}

#ifdef CONFIG_SD_FS
extern bool bt_bqb_is_in_test(void);

static void ipmsg_btc_update_bt_table(void *table_addr)
{
	int err;
	uint32_t calib_val = 0;
	int in_bqb = 0; 
	uint8_t *start = table_addr;
    int opt = soc_dvfs_opt();
    int valid = 0;
	printk("opt:%d Befor set 0x41 = 0x%x 0x42 = 0x%x 0x43 = 0x%x 0x44 = 0x%x\n", opt, start[0x41], start[0x42], start[0x43], start[0x44]);

    err = soc_atp_get_rf_calib(EFUSE_AVDD_IF, &calib_val);
    printk("get efuse avdd ret:%d value:%x default %x\n", err,calib_val,start[BT_CFG_EFUSE_AVDD_IF_POS]);
    if (err == 0) {
        sys_put_le32(calib_val, &start[BT_CFG_EFUSE_AVDD_IF_POS]);
    }
    else{
        valid = -1;
    }

    err = soc_atp_get_rf_calib(EFUSE_POWER_VER, &calib_val);
    printk("get efuse power version ret:%d value:%x default %x\n", err,calib_val,start[BT_CFG_EFUSE_POWER_VER_POS]);
    if (err == 0) {
        sys_put_le32(calib_val, &start[BT_CFG_EFUSE_POWER_VER_POS]);
        valid = 0;
    }
    else{
        valid = -1;
    }

    err = soc_atp_get_rf_calib(EFUSE_RF, &calib_val);
    printk("get efuse rf ret:%d value:%x default %x\n", err,calib_val,start[BT_CFG_EFUSE_RF_POS]);
    if ((err == 0) && calib_val) {
        sys_put_le32(calib_val, &start[BT_CFG_EFUSE_RF_POS]);
        valid = 0;
    }
    else{
        valid = -1;
    }

    if(!valid){
        start[BT_CFG_EFUSE_SET_POS] = (start[BT_CFG_EFUSE_SET_POS] | BIT(BT_CFG_EFUSE_SET_BIT));
    }

#ifdef CONFIG_BT_CTRL_BQB
	in_bqb= bt_bqb_is_in_test()?1:0;
#endif

#if 0 
	if (!in_bqb){
        start[BT_CFG_RF_8_TO_10DB_1V8_POS] = (start[BT_CFG_RF_8_TO_10DB_1V8_POS] | BIT(BT_CFG_RF_8_TO_10DB_1V8_BIT));
		printk("rf 8~10db, use 1.8V \n");
	}else{
        start[BT_CFG_RF_8_TO_10DB_1V8_POS] = (start[BT_CFG_RF_8_TO_10DB_1V8_POS] & (~(0x1 << BT_CFG_RF_8_TO_10DB_1V8_BIT)));	/* Set 0 */
	}
#endif

    if (in_bqb){
        start[BT_CFG_DISABLE_3M_POS] = (start[BT_CFG_DISABLE_3M_POS] & (~(0x1 << BT_CFG_DISABLE_3M_BIT)));	/* Set 0 */
    }

	if (btc_usr_config.BTC_Log){
		start[BT_CFG_TWS_PRI_ENC_POS] = (start[BT_CFG_TWS_PRI_ENC_POS] & (~(0x1 << BT_CFG_TWS_PRI_ENC_BIT)));		/* Set 0 */	
	}else{
		start[BT_CFG_TWS_PRI_ENC_POS] = (start[BT_CFG_TWS_PRI_ENC_POS] | (0x1 << BT_CFG_TWS_PRI_ENC_BIT));		/* Set 1 */
	}


#ifdef CONFIG_IPMSG_BTC_SEL_32K
	start[BT_CFG_SEL_32768_POS] = (start[BT_CFG_SEL_32768_POS] | (0x1 << BT_CFG_SEL_32768_BIT));		/* Set 1 */
#else
	start[BT_CFG_SEL_32768_POS] = (start[BT_CFG_SEL_32768_POS] & (~(0x1 << BT_CFG_SEL_32768_BIT)));		/* Set 0 */
#endif

	start[BT_CFG_FORCE_LIGHT_SLEEP_POS] = (start[BT_CFG_FORCE_LIGHT_SLEEP_POS] & (~(0x1 << BT_CFG_FORCE_LIGHT_SLEEP_BIT)));		/* Set 0 */

#if 1
    if (1){//in_bqb
		start[BT_CFG_RF_BQB_SETTING_POS] = (start[BT_CFG_RF_BQB_SETTING_POS] | (0x1 << BT_CFG_RF_BQB_SETTING_BIT));		/* Set 1 normal mode*/		
    }else{
		start[BT_CFG_RF_BQB_SETTING_POS] = (start[BT_CFG_RF_BQB_SETTING_POS] & (~(0x1 << BT_CFG_RF_BQB_SETTING_BIT)));		/* Set 0 low power mode*/
    }
#endif

	if (btc_usr_config.BTC_Log != 1){
		start[BT_CFG_FORCE_UART_PRINT_POS] = (start[BT_CFG_FORCE_UART_PRINT_POS] & (~(0x1 << BT_CFG_FORCE_UART_PRINT_BIT)));			/* Set 0 */
	}else{
		start[BT_CFG_FORCE_UART_PRINT_POS] = (start[BT_CFG_FORCE_UART_PRINT_POS] | (0x1 << BT_CFG_FORCE_UART_PRINT_BIT));			/* Set 1 */
	}
#ifdef CONFIG_BT_TRANSMIT
	start[BT_CFG_TRANS_EN_POS] = (start[BT_CFG_TRANS_EN_POS] | (0x1 << BT_CFG_TRANS_EN_BIT));
#endif

	start[BT_CFG_SEL_RC32K_POS] = (start[BT_CFG_SEL_RC32K_POS] & (~(0x1 << BT_CFG_SEL_RC32K_BIT)));
	//start[BT_CFG_WAKEUP_ADVANCE_POS] = 0x04;

	if (btc_usr_config.BTC_Log == 2){
		start[BT_CFG_UART_BAUD_RATE_POS] = (uint8_t)(BT_CFG_UART_BAUD_RATE & 0xFF);
		start[BT_CFG_UART_BAUD_RATE_POS + 1] = (uint8_t)((BT_CFG_UART_BAUD_RATE >> 8) & 0xFF);
		start[BT_CFG_UART_BAUD_RATE_POS + 2] = (uint8_t)((BT_CFG_UART_BAUD_RATE >> 16) & 0xFF);
		start[BT_CFG_UART_BAUD_RATE_POS + 3] = (uint8_t)((BT_CFG_UART_BAUD_RATE >> 24) & 0xFF);
		printk("Bt uart rate %d\n", BT_CFG_UART_BAUD_RATE);
	}

#ifdef CONFIG_BT_ECC_ACTS
	start[BT_CFG_K192_BY_HOST_POS] = (start[BT_CFG_K192_BY_HOST_POS] | (0x1 << BT_CFG_K192_BY_HOST_BIT));
#else
	start[BT_CFG_K192_BY_HOST_POS] = (start[BT_CFG_K192_BY_HOST_POS] & (~(0x1 << BT_CFG_K192_BY_HOST_BIT)));
#endif

	if (btc_set_param.set_max_rf_power) {
		start[BT_MAX_RF_POWER_POS_1] = btc_set_param.bt_max_rf_tx_power;
		start[BT_MAX_RF_POWER_POS_2] = btc_set_param.bt_max_rf_tx_power;
		printk("max rf power %d\n", btc_set_param.bt_max_rf_tx_power);
	}

	if (btc_usr_config.BTC_Fix_TX_Power){
		start[BT_MAX_RF_POWER_POS_1] = btc_usr_config.BTC_Fix_TX_Power;
		start[BT_MAX_RF_POWER_POS_2] = btc_usr_config.BTC_Fix_TX_Power;
		start[BT_DEFAULT_TX_POWER] = btc_usr_config.BTC_Fix_TX_Power;
		start[BT_CFG_FIX_MAX_PWR_POS] = start[BT_CFG_FIX_MAX_PWR_POS] | (0x1 << BT_CFG_FIX_MAX_PWR_BIT);
		printk("After set 0x22 = %d 0x23 = %d 0x25 = %d\n", start[0x22], start[0x23], start[0x25]);
		printk("After set 0x40 = 0x%x\n", start[BT_CFG_FIX_MAX_PWR_POS]);
	}

	if (btc_set_param.set_ble_rf_power) {
		start[BT_BLE_RF_POWER_POS] = btc_set_param.ble_rf_tx_power;
		printk("ble rf tx power %d\n", start[BT_BLE_RF_POWER_POS]);
	}

    start[BT_SNIFF_ATTEMP_POS] = 8;

	printk("After set 0x41 = 0x%x 0x42 = 0x%x 0x43 = 0x%x 0x44 = 0x%x\n", start[0x41],start[0x42], start[0x43], start[0x44]);
}

#ifdef CONFIG_SOC_EP_MODE
static void ipmsg_btc_update_rf_table(void *start_addr)
{
	int i;
	uint16_t *start_int16_t = start_addr;
	int err;
	uint32_t calib_val = 0;
 	uint32_t ic_ver = sys_read32(CHIPVERSION) & 0xf;
	
 	if (ic_ver == 1){//C version
		err = soc_atp_get_rf_calib(EFUSE_RF_FOR_VER_C, &calib_val);
		printk("C Ver get efuse rf ret:%d value:%x\n", err,calib_val);
		if (err == 0) {
			if (calib_val == 0){
				sys_put_le16(0x6666, (uint8_t *)&start_int16_t[0x0E]);				
			}else{
				sys_put_le16(calib_val, (uint8_t *)&start_int16_t[0x0E]);
			}
			printk("Set rf[0x0E] (%p) value 0x%x\n",&start_int16_t[0x0E], start_int16_t[0x0E]);
		}
	}

	for (i=0; i<4; i++) {
		if (btc_set_param.rf_param[i].addr <= 0x2F) {
			sys_put_le16(btc_set_param.rf_param[i].value, (uint8_t *)&start_int16_t[btc_set_param.rf_param[i].addr]);
			printk("Set rf addr 0x%x(%p) value 0x%x\n", btc_set_param.rf_param[i].addr,
				&start_int16_t[btc_set_param.rf_param[i].addr], start_int16_t[btc_set_param.rf_param[i].addr]);
		}
	}
}
#endif
#endif

static int ipmsg_btc_load(struct device *dev, void *data, uint32_t size)
{
	uint32_t val;
    int opt;
	int err = 0;
	const struct ipmsg_btc_config *config = dev->config;

	if (size > config->mem_size) {
		return -EINVAL;
	}


	printk("MEMORYCTL2 0x%x \n",sys_read32(MEMORY_CTL2));


	val = sys_read32(CMU_MEMCLKSRC0) & ~BT_RAM_CLK_SRC;
	sys_write32(val, CMU_MEMCLKSRC0);

#ifdef CONFIG_SD_FS

#if defined(CONFIG_BT_FCC_TEST)
	#define BT_RAM_FCC_ADDR 	0x01200000
	sys_write32((sys_read32(MEMORYCTL) | (0x1<<4)), MEMORYCTL);	// bt cpu boot from btram0
	k_sleep(K_MSEC(1));
	sd_load(BT_FCC_FILE, (void *)BT_RAM_FCC_ADDR);  /* load bt config table */
	k_sleep(K_MSEC(1));
	return err;
#elif defined(CONFIG_BT_NS_TEST)
	#define BT_RAM_NST_ADDR 	0x01200000
	sys_write32((sys_read32(MEMORYCTL) | (0x1<<4)), MEMORYCTL);	// bt cpu boot from btram0
	k_sleep(K_MSEC(1));
	sd_load(BT_NST_FILE, (void *)BT_RAM_NST_ADDR);  /* load bt config table */
	k_sleep(K_MSEC(1));
	return err;
#endif


#ifdef CONFIG_BT_CTRL_RF_DEBUG
	err = sd_load(BT_RF_FILE, (void *)BT_RAM1_ADDR);
	if (err) {
		return -EINVAL;
	}
#endif
    opt = soc_dvfs_opt();
    printk("soc opt %d\n",opt);

	err = sd_load(BT_TABLE_FILE, (void *)BT_RAM_TABLE_ADDR);  /* load bt config table */

	ipmsg_btc_update_bt_table((void *)BT_RAM_TABLE_ADDR);
	
	err |= sd_load(BT_PATCH_FILE, (void *)BT_RAM_PATCH_ADDR);  /* load bt patch */

#ifdef CONFIG_SOC_EP_MODE
    ipmsg_btc_update_rf_table((void *)BT_RAM_PATCH_ADDR);
#endif

#else
	memcpy(config->mem_base, data, size);
#endif

	return err;
}

/* Start BT CPU */
static inline int ipmsg_btc_start(struct device *dev, uint32_t *send_id, uint32_t *recv_id)
{
	uint32_t val;
	volatile int loop = 400;

	ARG_UNUSED(dev);

	/* Start up not need wait */
	//sys_write32(0x00640015, HOSCLDO_CTL);
	//while(loop)loop--;
	sys_write32(0x0164800C, HOSCLDO_CTL);

	val = sys_read32(HOSC_CTL) & (HOSC_CTL_HOSCI_CAP_MASK | HOSC_CTL_HOSCO_CAP_MASK);
	val |= 0x7730;
	sys_write32(val, HOSC_CTL);

	if (btc_set_param.set_hosc_cap) {
		val = sys_read32(HOSCLDO_CTL);
		val &= 0xFFFF;
		val |= (btc_set_param.hosc_capacity << 16);
		sys_write32(val, HOSCLDO_CTL);
		while(loop)loop--;
		val |= (0x1 << 24);
		sys_write32(val, HOSCLDO_CTL);		/* bit16-bit23 same to HOSC_CTL bit16-bit23 */

		val = sys_read32(HOSC_CTL) & ~(HOSC_CTL_HOSCI_CAP_MASK | HOSC_CTL_HOSCO_CAP_MASK);
		val |= HOSC_CTL_HOSCI_CAP(btc_set_param.hosc_capacity) | HOSC_CTL_HOSCO_CAP(btc_set_param.hosc_capacity);
		sys_write32(val, HOSC_CTL);
	}

	printk("HOSC_CTL = 0x%x HOSCLDO_CTL = 0x%x\n", sys_read32(HOSC_CTL), sys_read32(HOSCLDO_CTL));

	printk("%x\n",RBUF_BASE);
	sys_write32(0x551, HOSCOK_CTL);


	val = sys_read32(CMU_MEMCLKSRC0) | BT_RAM_CLK_SRC;
	val = (val & ~(7 << 25)) | (1 << 25);
	val = (val & ~(7 << 5)) | (1 << 5);
	sys_write32(val, CMU_MEMCLKSRC0);

	val = sys_read32(CMU_MEMCLKEN0) | BT_ROM_RAM_CLK_EN;
	sys_write32(val, CMU_MEMCLKEN0);


	//val = sys_read32(CMU_S1CLKCTL) | RC64M_S1EN | HOSC_S1EN | RC4M_S1EN;
	val = sys_read32(CMU_S1CLKCTL) | HOSC_S1EN | RC4M_S1EN;
	sys_write32(val, CMU_S1CLKCTL);

#if defined(CONFIG_BT_FCC_TEST) || defined(CONFIG_BT_NS_TEST)
	val = sys_read32(CMU_S1BTCLKCTL) | RC64M_S1BTEN | HOSC_S1BTEN | RC4M_S1BTEN;
#else
	//val = sys_read32(CMU_S1BTCLKCTL) | RC64M_S1BTEN | HOSC_S1BTEN | RC4M_S1BTEN;
	val = sys_read32(CMU_S1BTCLKCTL) | HOSC_S1BTEN | RC4M_S1BTEN;
#endif
	sys_write32(val, CMU_S1BTCLKCTL);


	if (btc_usr_config.BTC_Debug_Signal == 1){
#ifdef CONFIG_BT_CTRL_BQB
		extern bool bt_bqb_is_in_test(void);
		if (!bt_bqb_is_in_test())
#endif
		{
			/*Debugsel need GPIO21, but GPIO21 be used by JTAG Group1, must disable JTAG Group1. */
			sys_write32((sys_read32(JTAG_CTL) & ~(1 << 4)), JTAG_CTL);

			sys_write32(DBGSE_BT_CONTROLLER, DEBUGSEL);
			sys_write32(0x3e0000, DEBUGOE);
		}
	}


	if (btc_usr_config.BTC_Log == 1)
	{
		#define BT_UART_MFP_SEL 12
		struct acts_pin_config btc_pin_cfg_uart[] = {
	  		{1, BT_UART_MFP_SEL | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
	  		{2, BT_UART_MFP_SEL | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
		};
		btc_pin_cfg_uart[0].pin_num = btc_usr_config.BTC_Log_TX_Pin	;	
		btc_pin_cfg_uart[1].pin_num = btc_usr_config.BTC_Log_RX_Pin	;	

		acts_pinmux_setup_pins(btc_pin_cfg_uart, ARRAY_SIZE(btc_pin_cfg_uart));

#ifndef CONFIG_SOC_EP_MODE
	#ifdef CONFIG_BT_CTRL_BLE_BQB
		acts_pinmux_setup_pins(btc_pin_cfg_bqb, ARRAY_SIZE(btc_pin_cfg_bqb));
	#endif
#endif
	}

	acts_reset_peripheral(RESET_ID_BT);

	irq_enable(IRQ_ID_BT);
	
	return 0;
}

static int ipmsg_btc_stop(struct device *dev)
{
	ARG_UNUSED(dev);

	// disable irq
	irq_disable(IRQ_ID_BT);

	// reset btc
	acts_reset_peripheral_assert(RESET_ID_BT);

	return 0;
}

BTC_RAM_FUNC static int ipmsg_btc_notify(struct device *dev)
{
	ARG_UNUSED(dev);
	volatile uint8_t i = 5;

	// send interrupt to btc
	sys_write32(0, INT_TO_BT_CPU);
	while(i--);
	sys_write32(1, INT_TO_BT_CPU);

	return 0;
}

static void ipmsg_btc_register_callback(struct device *dev,
					 ipmsg_callback_t cb, void *context)
{
	struct ipmsg_btc_data *data = dev->data;
	uint8_t irq = *((uint8_t *)context);

	switch (irq) {
	case IPMSG_BTC_IRQ:
		data->btc_cb = cb;
		break;
	case IPMSG_TWS0_IRQ:
		data->tws0_cb = cb;
		break;
	case IPMSG_TWS1_IRQ:
		data->tws1_cb = cb;
		break;
	case IPMSG_REG_PW_CTRL:
		data->pm_ctrl_cb = (ipmsg_pm_ctrl_callback_t)cb;
		break;
	default:
		printk("Unknown irq %d\n", irq);
		break;
	}
}

static void ipmsg_btc_clear_irq_pending(int32_t irq)
{
	if (irq < 32) {
		sys_write32((0x1 << irq), NVIC_ICPR0);
	} else {
		sys_write32((0x1 << (irq - 32)), NVIC_ICPR1);
	}
}

static void ipmsg_btc_irq_enable(struct device *dev, uint8_t irq)
{
	switch (irq) {
	case IPMSG_BTC_IRQ:
		irq_enable(IRQ_ID_BT);
		break;
	case IPMSG_TWS0_IRQ:
		ipmsg_btc_clear_irq_pending(IRQ_ID_TWS);
		irq_enable(IRQ_ID_TWS);
		break;
	case IPMSG_TWS1_IRQ:
		ipmsg_btc_clear_irq_pending(IRQ_ID_TWS1);
		irq_enable(IRQ_ID_TWS1);
		break;
	}
}

BTC_RAM_FUNC static void ipmsg_btc_irq_disable(struct device *dev, uint8_t irq)
{
	switch (irq) {
	case IPMSG_BTC_IRQ:
		irq_disable(IRQ_ID_BT);
		break;
	case IPMSG_TWS0_IRQ:
		irq_disable(IRQ_ID_TWS);
		break;
	case IPMSG_TWS1_IRQ:
		irq_disable(IRQ_ID_TWS1);
		break;
	}
}

static int btc_debug_config_init(void)
{
#ifdef CONFIG_CFG_DRV
	/* CFG_BT_Debug_BTC_Log */
	if (!cfg_get_by_key(ITEM_BTC_LOG, &btc_usr_config.BTC_Log, sizeof(cfg_uint8))) {
		printk("failed to get:%d", ITEM_BTC_LOG);
		goto Fail;
	}
	/* CFG_BT_Debug_BTC_Log_TX_Pin */
	if (!cfg_get_by_key(ITEM_BTC_LOG_TX_PIN, &btc_usr_config.BTC_Log_TX_Pin, sizeof(cfg_uint8))) {
		printk("failed to get:%d", ITEM_BTC_LOG_TX_PIN);
		goto Fail;
	}
	/* CFG_BT_Debug_BTC_Log_RX_Pin */
	if (!cfg_get_by_key(ITEM_BTC_LOG_RX_PIN, &btc_usr_config.BTC_Log_RX_Pin, sizeof(cfg_uint8))) {
		printk("failed to get:%d", ITEM_BTC_LOG_RX_PIN);
		goto Fail;
	}
	/* CFG_BT_Debug_BTC_BQB_TX_Pin */
	if (!cfg_get_by_key(ITEM_BTC_BQB_TX_PIN, &btc_usr_config.BTC_BQB_TX_Pin, sizeof(cfg_uint8))) {
		printk("failed to get:%d", ITEM_BTC_BQB_TX_PIN);
		goto Fail;
	}
	/* CFG_BT_Debug_BTC_BQB_RX_Pin */
	if (!cfg_get_by_key(ITEM_BTC_BQB_RX_PIN, &btc_usr_config.BTC_BQB_RX_Pin, sizeof(cfg_uint8))) {
		printk("failed to get:%d", ITEM_BTC_BQB_RX_PIN);
		goto Fail;
	}

	/* CFG_BT_Debug_BTC_Fix_TX_Power */
	if (!cfg_get_by_key(ITEM_BTC_FIX_TX_POWER, &btc_usr_config.BTC_Fix_TX_Power, sizeof(cfg_uint8))) {
		printk("failed to get:%d", ITEM_BTC_FIX_TX_POWER);
		goto Fail;
	}

	/* CFG_BT_Debug_BTC_Debug_Signal */
	if (!cfg_get_by_key(ITEM_BTC_DEBUG_SIGNAL, &btc_usr_config.BTC_Debug_Signal, sizeof(cfg_uint8))) {
		printk("failed to get:%d", ITEM_BTC_DEBUG_SIGNAL);
		goto Fail;
	}
	
	printk("BTC_Log(%d) Log T/Rx(%d,%d) BQB(%d,%d) fixTxPower(%d) dbgSignal(%d)\n",btc_usr_config.BTC_Log,
		btc_usr_config.BTC_Log_TX_Pin,btc_usr_config.BTC_Log_RX_Pin,
		btc_usr_config.BTC_BQB_TX_Pin,btc_usr_config.BTC_BQB_RX_Pin,
		btc_usr_config.BTC_Fix_TX_Power,btc_usr_config.BTC_Debug_Signal);
#else
	btc_usr_config.BTC_Log = 0;
	#ifdef CONFIG_BT_CTRL_DEBUG
		btc_usr_config.BTC_Log = 1;
	#else
		#ifdef CONFIG_BT_CTRL_LOG
		btc_usr_config.BTC_Log = 2;
		#endif /*CONFIG_BT_CTRL_LOG*/
	#endif /*CONFIG_BT_CTRL_DEBUG*/
	btc_usr_config.BTC_Log_TX_Pin = 34;
	btc_usr_config.BTC_Log_RX_Pin = 35;
	#define BT_FIX_POWER_LEVEL	38		/* 8db */
	btc_usr_config.BTC_Fix_TX_Power	= BT_FIX_POWER_LEVEL;
#endif
	return 0;
Fail:

	return -1;
}

static int ipmsg_btc_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	int i;
	for (i=0; i<4; i++) {
		btc_set_param.rf_param[i].addr =0xff;
	}
	btc_debug_config_init();

	/* btc irq init */
	IRQ_CONNECT(IRQ_ID_BT, CONFIG_BTC_IRQ_PRI,
			ipmsg_btc_isr, DEVICE_GET(btc), 0);

	/* tws irq init */
	IRQ_CONNECT(IRQ_ID_TWS, CONFIG_TWS_IRQ_PRI,
			ipmsg_tws0_isr, DEVICE_GET(btc), 0);
#ifdef CONFIG_SOC_EP_MODE
	IRQ_CONNECT_ORI(IRQ_ID_TWS1, CONFIG_TWS_IRQ_PRI,
			ipmsg_tws1_isr, DEVICE_GET(btc), 0);
	vect_isr_install(IRQ_ID_TWS1, ipmsg_tws1_isr_ram);
	btc_dev = (struct device *)DEVICE_GET(btc);
#else
	IRQ_CONNECT(IRQ_ID_TWS1, CONFIG_TWS_IRQ_PRI,
			ipmsg_tws1_isr, DEVICE_GET(btc), 0);
#endif

	return 0;
}


#ifdef CONFIG_PM_DEVICE
int ipmsg_btc_pm_control(const struct device *device, enum pm_device_action action)
{
	int ret;
	uint32_t val;
	struct ipmsg_btc_data *data = device->data;



	if (data && data->pm_ctrl_cb) {
		data->pm_ctrl_cb(0, action);
	}

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		val = sys_read32(HOSC_CTL) & (~(HOSC_CTL_HGMC_MASK | 0xf0));
		val |= 0x30;
		sys_write32(val, HOSC_CTL);
		printk("ACTIVE_STATE HOSC_CTL = 0x%x\n", sys_read32(HOSC_CTL));
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		val = sys_read32(HOSC_CTL) & (~(HOSC_CTL_HGMC_MASK | 0xf0));
		val |= (0x3 << HOSC_CTL_HGMC_SHIFT)|0x20;
		sys_write32(val, HOSC_CTL);
		printk("SUSPEND_STATE HOSC_CTL = 0x%x\n", sys_read32(HOSC_CTL));
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
#define ipmsg_btc_pm_control 	NULL
#endif

static const struct ipmsg_driver_api ipmsg_btc_driver_api = {
	.init_param = ipmsg_btc_init_param,
	.load = ipmsg_btc_load,
	.start = ipmsg_btc_start,
	.stop = ipmsg_btc_stop,
	.notify = ipmsg_btc_notify,
	.register_callback = ipmsg_btc_register_callback,
	.enable = ipmsg_btc_irq_enable,
	.disable = ipmsg_btc_irq_disable,
};

static const struct ipmsg_btc_config ipmsg_btc_cfg = {
	.mem_base = (void *)BTC_REG_BASE,
	.mem_size = 0x40000,
};

static struct ipmsg_btc_data ipmsg_btc_dat;

DEVICE_DEFINE(btc, CONFIG_BTC_NAME,
			&ipmsg_btc_init, ipmsg_btc_pm_control,
			&ipmsg_btc_dat, &ipmsg_btc_cfg,
			POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			&ipmsg_btc_driver_api);

