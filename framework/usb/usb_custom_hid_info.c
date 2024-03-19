#include <kernel.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <btservice_api.h>
#include <api_common.h>
#include <usb/class/usb_hid.h>
#include <usb/usb_custom_handler.h>
#include <usb/usb_custom_hid_info.h>
#include <xear_authentication.h>
#include <fw_version.h>
#include <drivers/flash.h>
#include <board_cfg.h>
#include <device.h>


//#include <logging/sys_log.h>
#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "hid_info"
#endif

extern int tr_ble_srv_send_xear_data_other(u8_t *buf, u8_t packet_idx, u8_t packet_len);
//extern int tr_ble_srv_send_data_other(u8_t *buf, u8_t packet_idx, u8_t packet_len);
extern int tr_ble_cli_send_data_other(u16_t handle, u8_t *buf, u16_t data_len);
extern int usb_hid_ep_write(const u8_t *data, u32_t data_len, u32_t timeout);


#define MSB(word) 		(u8_t)(((word) >> 8) & 0x00FF)
#define LSB(word) 		(u8_t)(((word)&0x00FF))

enum {
	INFO_TARGET_RX = 0x00,
	INFO_TARGET_TX = 0x01,
};

static u8_t info_id;
static u8_t info_buf[64];
static u8_t info_len;
static struct k_delayed_work info_work;

static u32_t flash_addr, flash_len;

static void info_work_handle(struct k_work *work)
{
	struct conn_status status;

	API_APP_CONNECTION_CONFIG(0, &status);

	SYS_LOG_INF("status.connected(%d) info_len(%d)\n", status.connected, info_len);

	if( status.connected )
	{
		tr_ble_cli_send_data_other(status.handle, info_buf, info_len);
	}
}

static u8_t infoFwVersion(u8_t *buf, u8_t bufLen)
{
	u8_t i,j;
	struct fw_version *fw = (struct fw_version *)fw_version_get_current();
	// fw->version_name: "0.6.0.1_2305091556"  0.6.0.1_yymmddhhmm
	// translate string into BCD format (AA 01 00 06 00 01 FF 23 05 09 15 56)
	// AA, report Id
	// 01, info Id
	// 00 06 00 01, version(0.6.0.1)
	// FF, separator
	// 23 05 09, yymmdd(2023, May, 9th)
	// 15 56, hhmm(15:56)

	memset(buf, 0xFF, bufLen);
	buf[0] = HID_REPORT_ID_INFO;

	for( i=2,j=0; i<6; i++,j+=2 )
		buf[i] = fw->version_name[j] - 0x30;
	for( i=7,j=8; (i<bufLen) && fw->version_name[j]; i++,j+=2 )
		buf[i] = ((fw->version_name[j] - 0x30) << 4) | (fw->version_name[j+1] - 0x30);

	return i;
}

static u8_t infoMemRead(u8_t *in, u8_t *buf, u8_t bufLen)
{
	u8_t i, j;
	u32_t data;
	u32_t addr = ((u32_t)in[5]<<24) | ((u32_t)in[6]<<16) | ((u16_t)in[7]<<8) | in[8];

	SYS_LOG_INF("addr(%08x) width(%d)count(%d)\n", addr, in[3], in[4]);

	memset(buf, 0xFF, bufLen);
	buf[0] = HID_REPORT_ID_INFO;

	for( i=0,j=3; (i<in[4]) && ((j+in[3])<=bufLen); i++ )
	{
		// width: mdw/mdh/mdb
		switch(in[3])
		{
		case 4:
			data = *(volatile u32_t *)addr;
			buf[j++] = (data >> 24) & 0x000000FF;
			buf[j++] = (data >> 16) & 0x000000FF;
			buf[j++] = (data >> 8) & 0x000000FF;
			buf[j++] = data & 0x000000FF;
			break;
		case 2:
			data = *(volatile u16_t *)addr;
			buf[j++] = (data >> 8) & 0x000000FF;
			buf[j++] = data & 0x000000FF;
			break;
		case 1:
			buf[j++] = *(volatile u8_t *)addr;
			break;
		}
		addr += in[3];
	}
	buf[2] = i;  // actual data count

	return (3 + i*in[3]);
}

static void infoMemWrite(u8_t *in, u8_t inLen)
{
	u8_t i, j;
	u32_t data;
	u32_t addr = ((u32_t)in[5]<<24) | ((u32_t)in[6]<<16) | ((u16_t)in[7]<<8) | in[8];

	SYS_LOG_INF("addr(%08x) width(%d)count(%d), inLen(%d)\n", addr, in[3], in[4], inLen);

	for( i=0,j=9; (i<in[4]) && ((j+in[3])<=inLen); i++ )
	{
		// width: mdw/mdh/mdb
		switch(in[3])
		{
		case 4:
			data = ((u32_t)in[j]<<24) | ((u32_t)in[j+1]<<16) | ((u16_t)in[j+2]<<8) | in[j+3];
			*(volatile u32_t *)addr = data;
			break;
		case 2:
			data =  ((u16_t)in[j]<<8) | in[j+1];
			*(volatile u16_t *)addr = data;
			break;
		case 1:
			*(volatile u8_t *)addr = in[j];
			break;
		}
		addr += in[3];
		j += in[3];
	}
}

static u8_t infoFlashRead(u8_t *buf, u8_t bufLen)
{
	int len;

	struct device *dev = (struct device *)device_get_binding(CONFIG_SPI_FLASH_NAME);//CONFIG_NVRAM_STORAGE_DEV_NAME

	if (!dev) {
		SYS_LOG_ERR("cannot found device \'%s\'\n",
			    CONFIG_NVRAM_STORAGE_DEV_NAME);
		return 0;
	}

	len = flash_len < (u32_t)(bufLen-3) ? flash_len : (bufLen-3);
	SYS_LOG_INF("addr(%08x) len(%d)\n", flash_addr, len);

	memset(buf, 0xFF, bufLen);
	if ( flash_read(dev, flash_addr, buf+3, len) < 0) {
		SYS_LOG_ERR("flash_read error");
		return 0;
	}
	buf[2] = len;  // actual data count
	buf[1] = info_id;
	buf[0] = HID_REPORT_ID_INFO;

	// update addr/len for next hid flash_read command
	flash_addr += len;
	flash_len -= len;

	return (3 + len);
}

/* ------------------------------------------------------------------------- */

static int getInfoReport(s32_t *len, u8_t **data)
{
	switch(info_id) {
	case INFO_ID_FW_VERSION:
	case INFO_ID_MEM_READ:
		if( info_buf[1] == HID_REPORT_ID_INFO )
		{
			// info_buf[1] == HID_REPORT_ID_INFO implies that
			// data is ready and stored in info_buf[].
			break;
		}
		return -ENOTSUP;

	case INFO_ID_FLASH_READ:
		if( flash_addr == 0xFFFFFFFF )
		{
			// read Rx flash
			if( info_buf[1] == HID_REPORT_ID_INFO )
			{
				break;
			}
			return -ENOTSUP;
		}
		else
		{
			infoFlashRead(info_buf, sizeof(info_buf));
		}
		break;

	default:
		return -ENOTSUP;
	}

	info_buf[1] = info_id;
	*len = sizeof(info_buf);
	*data = info_buf;
	return 0;
}

static int setInfoReport(s32_t len, u8_t *data)
{
	info_id = data[1];

	// for those command target to Rx, we send command to Rx.
	// command aim at Tx, just process here..
	switch(info_id)
	{
	case INFO_ID_XEAR_STATUS:
		len = 2;
		break;

	case INFO_ID_FW_VERSION:
		if( data[2] == INFO_TARGET_RX )
		{
			len = 2;
			break;
		}
		else
		{
			infoFwVersion(info_buf, sizeof(info_buf));
			info_buf[1] = HID_REPORT_ID_INFO;  // flag used to indicate data is ready
		}
		return 0;

	case INFO_ID_MEM_WRITE:
		if( data[2] == INFO_TARGET_RX )
		{
			len = 8 + data[3] * data[4];  // width * count
			break;
		}
		else
		{
			infoMemWrite(data, len);
		}
		return 0;

	case INFO_ID_MEM_READ:
		if( data[2] == INFO_TARGET_RX )
		{
			len = 8;
			break;
		}
		else
		{
			infoMemRead(data, info_buf, sizeof(info_buf));
			info_buf[1] = HID_REPORT_ID_INFO;  // flag used to indicate data is ready
		}
		return 0;

	case INFO_ID_FLASH_READ:
		if( data[2] == INFO_TARGET_RX )
		{
			flash_addr = 0xFFFFFFFF;   // used to indicate it is Rx flash read
			len = 10;
			break;
		}
		else
		{
			flash_addr = ((u32_t)data[3]<<24) | ((u32_t)data[4]<<16) | ((u16_t)data[5]<<8) | data[6];
			flash_len = ((u32_t)data[7]<<24) | ((u32_t)data[8]<<16) | ((u16_t)data[9]<<8) | data[10];
		}
		return 0;
	}

	hid_info_data_to_send(data+1, len); // len, not including report Id...
	return 0;
}

/* ------------------------------------------------------------------------- */
void hid_info_srv_rcv_data_handler(u8_t* buf, u8_t bufLen)
{
	u8_t dataLen = 0;

	SYS_LOG_INF("buf[0-3](%x %x %x %x) bufLen(%d)\n", buf[0], buf[1],buf[2],buf[3], bufLen);

	switch(buf[1])
	{
	case INFO_ID_FW_VERSION:
		dataLen = infoFwVersion(info_buf, sizeof(info_buf));
		break;

#ifdef CONFIG_XEAR_AUTHENTICATION
	case INFO_ID_XEAR_STATUS:
		xear_set_status(buf[2]);
		break;
#endif

	case INFO_ID_MEM_WRITE:
		infoMemWrite(buf, bufLen);
		break;

	case INFO_ID_MEM_READ:
		dataLen = infoMemRead(buf, info_buf, sizeof(info_buf));
		break;

	case INFO_ID_FLASH_READ:
		flash_addr = ((u32_t)buf[3]<<24) | ((u32_t)buf[4]<<16) | ((u16_t)buf[5]<<8) | buf[6];
		flash_len = ((u32_t)buf[7]<<24) | ((u32_t)buf[8]<<16) | ((u16_t)buf[9]<<8) | buf[10];
		dataLen = infoFlashRead(info_buf, sizeof(info_buf));
		break;
	}

	SYS_LOG_INF("info dataLen(%d)\n", dataLen);

	if( dataLen )
	{
		//tr_ble_srv_send_data_other(info_buf, 0, dataLen);
		tr_ble_srv_send_xear_data_other(info_buf, 0, dataLen);
	}
}

void hid_info_cli_rcv_data_handler(u8_t *buf, u8_t len)
{
	if( len > sizeof(info_buf) )
		len = sizeof(info_buf);

	memcpy(info_buf, buf, len);
	info_buf[1] = HID_REPORT_ID_INFO;  // flag used to indicate Rx Fw version is received
        usb_hid_ep_write(info_buf, 64, 50);
}

void hid_info_data_to_send(u8_t *buf, u8_t len)
{
	// buf[] including info_id, and data is in report format.

	info_len = (len < (sizeof(info_buf)-1)) ? len : (sizeof(info_buf)-1);
	memcpy( info_buf+1, buf, info_len );

	info_buf[0] = HID_REPORT_ID_INFO;
	info_len++;

	os_delayed_work_submit(&info_work, 200);
}

int hid_info_cmd_handler(struct usb_setup_packet *pSetup,
			 s32_t *len, u8_t **data)
{
	if( LSB(pSetup->wValue) == HID_REPORT_ID_INFO )
	{
		switch(pSetup->bRequest)
		{
		case HID_GET_REPORT:
			switch(MSB(pSetup->wValue))
			{
			case HID_REPORT_FEATURE:
				return getInfoReport(len, data);
			}
			break;

		case HID_SET_REPORT:
			switch(MSB(pSetup->wValue))
			{
			case HID_REPORT_FEATURE:
				setInfoReport(*len, *data);
				return 0;
			}
			break;
		}
	}

	return -ENOTSUP;
}

void hid_info_init(void)
{
	flash_addr = 0;
	flash_len = 0;

	os_delayed_work_init(&info_work, info_work_handle);
}
