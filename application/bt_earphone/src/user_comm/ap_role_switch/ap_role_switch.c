/*!
 * \file      ap_role_switch.c
 * \brief     
 * \details   
 * \author 
 * \date  
 * \copyright Actions
 */
#include <user_comm/ap_role_switch.h>

#ifdef CONFIG_GMA_APP
extern void gma_rx_sync_info_cb(uint8_t *data, uint16_t len);
#endif
#ifdef CONFIG_NV_APP
extern void nv_rx_sync_info_cb(uint8_t *data, uint16_t len);
#endif
#ifdef CONFIG_TUYA_APP
extern void tuya_rx_sync_info_cb(uint8_t *data, uint16_t len);
#endif

void ap_role_switch_reg(struct tws_snoop_switch_cb_func *switch_cb)
{
	bt_manager_tws_reg_snoop_switch_cb_func(switch_cb, true);
}

u8_t ap_role_switch_access_receive(u8_t* buf, u16_t len)
{
	if ((!buf) || (len <= 2))
		return 0;

	if (APP_ROLE_SWITCH_GMA_ID == buf[0])
	{
#ifdef CONFIG_GMA_APP
		gma_rx_sync_info_cb(buf, len);
#endif
	}
	else if (APP_ROLE_SWITCH_NV_ID == buf[0])
	{
#ifdef CONFIG_NV_APP
		nv_rx_sync_info_cb(buf, len);
#endif
	}
	else if (APP_ROLE_SWITCH_TUYA_ID == buf[0])
	{
#ifdef CONFIG_TUYA_APP
		tuya_rx_sync_info_cb(buf, len);
#endif
	}
	return 0;
}

