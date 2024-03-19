/*!
 * \file	  ap_role_switch.h
 * \brief	  
 * \details   
 * \author 
 * \date   
 * \copyright Actions
 */

#ifndef _AP_ROLE_SWITCH_H
#define _AP_ROLE_SWITCH_H

#include <bt_manager.h>

typedef enum
{
	APP_ROLE_SWITCH_GMA_ID = 0x10,
	APP_ROLE_SWITCH_TME_ID,
	APP_ROLE_SWITCH_NMA_ID,
	APP_ROLE_SWITCH_TUYA_ID,
	APP_ROLE_SWITCH_NV_ID,

	APP_ROLE_SWITCH_ID_MAX = 0xEF,
} AP_ROLE_SWITCH_ID;

typedef enum
{
	APP_ROLE_SWITCH_START_EVENT = 1,
	APP_ROLE_SWITCH_FINISH_EVENT ,

} AP_ROLE_SWITCH_EVENT;

void ap_role_switch_reg(struct tws_snoop_switch_cb_func *switch_cb);
u8_t ap_role_switch_access_receive(u8_t* buf, u16_t len);

#endif	// _AP_ROLE_SWITCH_H


