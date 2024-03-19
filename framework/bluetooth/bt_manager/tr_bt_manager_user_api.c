//#include <logging/sys_log.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <shell/shell.h>
#include <stream.h>
#include <acts_ringbuf.h>
#include <bt_manager.h>
#include <sys_event.h>
#include <app_manager.h>
#include <power_manager.h>
#include <bt_manager_inner.h>
#include "audio_policy.h"
#include "app_switch.h"
#include "btservice_api.h"
#include "media_player.h"
#include <volume_manager.h>
#include <audio_system.h>
#include <msg_manager.h>
#include "api_bt_module.h"
#include <property_manager.h>
#include "drivers/nvram_config.h"
#include "api_bt_module.h"
uint32_t bt_manager_user_api_get_status_transmitter(void)
{
    return (uint32_t)api_bt_get_status();
}


void bt_manager_user_api_switch_audio_source_transmitter(char *app)
{
	if(!tr_bt_manager_check_app_bt_relevant())
	{
		return;
	}
    app_switch(app, 0, true);
}


int bt_manager_user_api_get_last_dev_disconnect_reason_transmitter(void)
{
    return tr_bt_manager_get_disconnect_reason();
}

void bt_manager_trans_2nd_start(void);
void bt_manager_user_api_force_to_inquiry_transmitter(void)
{
   // bt_manager_trans_2nd_start();
}

void bt_manager_user_api_force_to_reconnect_pairlist_transmitter(void)
{
    tr_bt_manager_startup_reconnect();
}

void bt_manager_user_api_force_to_delete_pairlist_transmitter(void)
{
    api_bt_del_PDL(0xff);
}

