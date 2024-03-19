#define DT_DRV_COMPAT actions_acts_flash


#include <drivers/flash.h>
#include <drivers/spi.h>
#include <logging/log.h>
#include <soc.h>
#include "spi_flash.h"

#define SPINOR_FLAG_DATA_PROTECTION_SUPPORT     (1 << 16)
#define SPINOR_FLAG_DATA_PROTECTION_OPENED      (1 << 17)

/* NOR Flash vendors ID */
#define XSPI_NOR_MANU_ID_ALLIANCE	0x52	/* Alliance Semiconductor */
#define XSPI_NOR_MANU_ID_AMD		0x01	/* AMD */
#define XSPI_NOR_MANU_ID_AMIC		0x37	/* AMIC */
#define XSPI_NOR_MANU_ID_ATMEL		0x1f	/* ATMEL */
#define XSPI_NOR_MANU_ID_CATALYST	0x31	/* Catalyst */
#define XSPI_NOR_MANU_ID_ESMT		0x1c	/* ESMT */  /* change from 0x8c to 0x1c*/
//#define XSPI_NOR_MANU_ID_EON		0x1c	/* EON */   /* error */
#define XSPI_NOR_MANU_ID_FD_MICRO	0xa1	/* shanghai fudan microelectronics */
#define XSPI_NOR_MANU_ID_FIDELIX	0xf8	/* FIDELIX */
#define XSPI_NOR_MANU_ID_FMD		0x0e	/* Fremont Micro Device(FMD) */
#define XSPI_NOR_MANU_ID_FUJITSU	0x04	/* Fujitsu */
#define XSPI_NOR_MANU_ID_GIGADEVICE	0xc8	/* GigaDevice */
#define XSPI_NOR_MANU_ID_GIGADEVICE2	0x51	/* GigaDevice2 */
#define XSPI_NOR_MANU_ID_HYUNDAI	0xad	/* Hyundai */
#define XSPI_NOR_MANU_ID_INTEL		0x89	/* Intel */
#define XSPI_NOR_MANU_ID_MACRONIX	0xc2	/* Macronix (MX) */
#define XSPI_NOR_MANU_ID_NANTRONIC	0xd5	/* Nantronics */
#define XSPI_NOR_MANU_ID_NUMONYX	0x20	/* Numonyx, Micron, ST */
#define XSPI_NOR_MANU_ID_PMC		0x9d	/* PMC */
#define XSPI_NOR_MANU_ID_SANYO		0x62	/* SANYO */
#define XSPI_NOR_MANU_ID_SHARP		0xb0	/* SHARP */
#define XSPI_NOR_MANU_ID_SPANSION	0x01	/* SPANSION */
#define XSPI_NOR_MANU_ID_SST		0xbf	/* SST */
#define XSPI_NOR_MANU_ID_SYNCMOS_MVC	0x40	/* SyncMOS (SM) and Mosel Vitelic Corporation (MVC) */
#define XSPI_NOR_MANU_ID_TI		0x97	/* Texas Instruments */
#define XSPI_NOR_MANU_ID_WINBOND	0xda	/* Winbond */
#define XSPI_NOR_MANU_ID_WINBOND_NEX	0xef	/* Winbond (ex Nexcom) */
#define XSPI_NOR_MANU_ID_ZH_BERG	0xe0	/* ZhuHai Berg microelectronics (Bo Guan) */
#define XSPI_NOR_MANU_ID_XTX        0x0b    /* XTX */
#define XSPI_NOR_MANU_ID_BOYA       0x68    /* BOYA */
#define XSPI_NOR_MANU_ID_XMC        0x20    /* XMC */
#define XSPI_NOR_MANU_ID_PUYA       0x85    /* PUYA */
#define XSPI_NOR_MANU_ID_DS         0xff    /* Zbit Semiconductor */  /* FIX ME */


static __ramfunc void xspi_nor_write_status_general(struct spinor_info *sni, u16_t status_high, u16_t status_low)
{
	u32_t flags;
	u16_t status = (status_high << 8) | status_low;

	flags = irq_lock();
	p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS2, (u8_t *)&status_high, 1);
	p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS,  (u8_t *)&status_low,  1);
	p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS,  (u8_t *)&status, 2);
	irq_unlock(flags);
}

static bool data_protect_except_32kb_general(struct spinor_info *sni)
{
	u16_t status_low, status_high, status = 0;
	uint32_t flag = sni->spi.flag;
	uint32_t nor_flag = sni->flag;
	sni->flag |= SPINOR_FLAG_UNLOCK_IRQ_WAIT_READY; //unlock wait ready
	sni->spi.flag &= ~SPI_FLAG_NO_IRQ_LOCK;  //lock

	status_high = 0x40 | p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	status_low = 0x50;
	status = (status_high << 8) | status_low;

	/* P25Q16H: cmd 31H will overwrite config register */

	if (sni->chipid == 0x156085) {
		p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS, (u8_t *)&status, 2);
		return true;
	}

	xspi_nor_write_status_general(sni, status_high, status_low);

	sni->spi.flag = flag;
	sni->flag = nor_flag;


	return true;
}

static bool data_protect_except_32kb_for_puya(struct spinor_info *sni)
{
	u32_t chipid = sni->chipid;

	/* P25Q16H(P25Q16SH) */
	if (chipid == 0x156085)
		return data_protect_except_32kb_general(sni);

	return false;
}

static bool data_protect_except_32kb_for_ds(struct spinor_info *sni)
{
	//u32_t chipid = sni->chipid;

	/* ZB25VQ16 supports theoretically according to its datasheet, but we haven't tested yet */
	//if (chipid == 0x15605e)
	//	return data_protect_except_32kb_general(sni);

	return false;
}

static bool data_protect_except_32kb_for_winbond(struct spinor_info *sni)
{
	u32_t chipid = sni->chipid;

	/* W25Q16JV *//* note: there are 2 TIDs for this nor, and we're using 0x40 */
	if (chipid == 0x1540ef)
		return data_protect_except_32kb_general(sni);

	return false;
}

static bool data_protect_except_32kb_for_giga(struct spinor_info *sni)
{
	u32_t chipid = sni->chipid;

	/* GD25Q16CSIG */
	if (chipid == 0x1540c8)
		return data_protect_except_32kb_general(sni);

	return false;
}

static bool data_protect_except_32kb_for_xmc(struct spinor_info *sni)
{
	//u32_t chipid = sni->chipid;

	/* XM25QH16B supports theoretically according to its datasheet, but we haven't tested yet */
	//if (chipid == 0x154020)
	//	return data_protect_except_32kb_general(sni);

	return false;
}

static bool data_protect_except_32kb_for_esmt(struct spinor_info *sni)
{
	return false;
}

static bool data_protect_except_32kb_for_berg(struct spinor_info *sni)
{
	return false;
}

static bool data_protect_except_32kb_for_boya(struct spinor_info *sni)
{
	u32_t chipid = sni->chipid;

	/* BY25Q16BSSIG */
	if (chipid == 0x154068)
		return data_protect_except_32kb_general(sni);

	return false;
}

static bool data_protect_except_32kb_for_xtx(struct spinor_info *sni)
{
	u32_t chipid = sni->chipid;

	/* XT25W16B supports theoretically according to its datasheet, but we haven't tested yet */
	/* XT25F16BS, XT25W16B */
	//if ((chipid == 0x15400b) || (chipid == 0x15600b))
	/* XT25F16F & XT25F16B share the same chipid, and supports data_protection both,
	   XT25F16F max clock support above 90MHz, But XT25F16B we've found it unstable above 80MHz */
	/* XT25F16F */
	if (chipid == 0x15400b)
		return data_protect_except_32kb_general(sni);

	return false;
}

static bool data_protect_except_32kb_for_mxic(struct spinor_info *sni)
{
	return false;
}

static bool xspi_nor_data_protect_except_32kb(struct spinor_info *sni)
{
	u8_t manu_chipid = sni->chipid & 0xff;
	bool ret_val = false;

	switch (manu_chipid) {
	case XSPI_NOR_MANU_ID_MACRONIX:
		ret_val = data_protect_except_32kb_for_mxic(sni);
		break;

	case XSPI_NOR_MANU_ID_XTX:
		ret_val = data_protect_except_32kb_for_xtx(sni);
		break;

	case XSPI_NOR_MANU_ID_BOYA:
		ret_val = data_protect_except_32kb_for_boya(sni);
		break;

	case XSPI_NOR_MANU_ID_ZH_BERG:
		ret_val = data_protect_except_32kb_for_berg(sni);
		break;

	case XSPI_NOR_MANU_ID_ESMT:
		ret_val = data_protect_except_32kb_for_esmt(sni);
		break;

	case XSPI_NOR_MANU_ID_XMC:
		ret_val = data_protect_except_32kb_for_xmc(sni);
		break;

	case XSPI_NOR_MANU_ID_GIGADEVICE:
		ret_val = data_protect_except_32kb_for_giga(sni);
		break;

	case XSPI_NOR_MANU_ID_WINBOND_NEX:
		ret_val = data_protect_except_32kb_for_winbond(sni);
		break;

	case XSPI_NOR_MANU_ID_PUYA:
		ret_val = data_protect_except_32kb_for_puya(sni);
		break;

	case XSPI_NOR_MANU_ID_DS:
		ret_val = data_protect_except_32kb_for_ds(sni);
		break;

	default :
		printk("ID Error: 0x%x\n", manu_chipid);
		ret_val = false;
		break;
	}

	return ret_val;
}

static void xspi_nor_data_protect_disable(struct spinor_info *sni)
{
	u16_t status_low = 0, status_high = 0, status = 0;
	u32_t chipid = sni->chipid;

	status_low = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
	status_high = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	status = (status_high << 8) | status_low;

	if (chipid == 0x156085) {
		status &= 0xbfc3;
		p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS, (u8_t *)&status, 2);
    } else {
		status_high &= 0xbf;
		status_low &= 0xc3;
		xspi_nor_write_status_general(sni, status_high, status_low);
	}
}

void xspi_nor_dump_info(struct xspi_nor_info *sni)
{
#ifdef CONFIG_XSPI_NOR_ACTS_DUMP_INFO

	u32_t chipid, status, status2, status3;

	status3 = 0x00;
		chipid = p_spinor_api->read_chipid(sni);
		printk("SPINOR: chipid: 0x%02x 0x%02x 0x%02x",
			chipid & 0xff,
			(chipid >> 8) & 0xff,
			(chipid >> 16) & 0xff);

		status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
		status2 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
		status3 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS3);

		printk("SPINOR: status: 0x%02x 0x%02x 0x%02x", status, status2, status3);

#endif//CONFIG_XSPI_NOR_ACTS_DUMP_INFO

}

int nor_write_protection(struct device *dev, bool enable)
{

	struct spinor_info *sni = (struct spinor_info *)(dev)->data;

	if (sni->flag & SPINOR_FLAG_DATA_PROTECTION_SUPPORT) {
		xspi_nor_dump_info(sni);

		if (enable) {
			if ((sni->flag & SPINOR_FLAG_DATA_PROTECTION_OPENED) == 0) {
				if (xspi_nor_data_protect_except_32kb(sni) == true) {
					sni->flag |= SPINOR_FLAG_DATA_PROTECTION_OPENED;
					printk("data protect enable is true\n");
				} else {
					sni->flag &= ~SPINOR_FLAG_DATA_PROTECTION_OPENED;
					printk("data protect enable is false\n");
				}
			}
		} else {
			if (sni->flag & SPINOR_FLAG_DATA_PROTECTION_OPENED) {
				xspi_nor_data_protect_disable(sni);
				sni->flag &= ~SPINOR_FLAG_DATA_PROTECTION_OPENED;
				printk("data protect disable\n");
			}
		}

		xspi_nor_dump_info(sni);
	}



	return 0;
}


