/*
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int tr_bt_manager_auto_get_connectable_device_num(void);

struct tr_bt_manager_auto_move_t {
	struct thread_timer tr_bt_mgr_auto_move_timer;
	struct thread_timer tr_bt_mgr_sleep_after_disconnect_timer;

	u8_t auto_move_stop_and_keep_flag:1;
	u8_t inquiry_reconnect_alter_flag:1;
	u8_t disconnect_mode_alter_flag:1;

	int prev_status;

#if (CONFIG_BT_TRANS_SUPPORT_DEV_NUMBER > 1)
	struct thread_timer multi_dev_pair_timer;
	u8_t multi_dev_pair_support;
#endif
};

static struct tr_bt_manager_auto_move_t tr_bt_mgr_auto;



static void tr_bt_manager_show_status_change(int prev_status, int curr_status)
{
	printk("bt status change:");
	switch (prev_status)
	{
		case BT_STATUS_NONE:
			printk("BT_STATUS_NONE  ---->  ");
			break;
		case BT_STATUS_WAIT_PAIR:
			printk("BT_STATUS_WAIT_PAIR  ---->  ");
			break;		
		case BT_STATUS_CONNECTED:
			printk("BT_STATUS_CONNECTED  ---->  ");
			break;
		case BT_STATUS_DISCONNECTED:
			printk("BT_STATUS_DISCONNECTED  ---->  ");
			break;
		case BT_STATUS_TWS_WAIT_PAIR:
			printk("BT_STATUS_TWS_WAIT_PAIR  ---->  ");
			break;
		case BT_STATUS_TWS_PAIRED:
			printk("BT_STATUS_TWS_PAIRED  ---->  ");
			break;
		case BT_STATUS_TWS_UNPAIRED:
			printk("BT_STATUS_TWS_UNPAIRED  ---->  ");
			break;
		case BT_STATUS_MASTER_WAIT_PAIR:
			printk("BT_STATUS_MASTER_WAIT_PAIR  ---->  ");
			break;
		//case BT_STATUS_PAUSED:
		//	printk("BT_STATUS_PAUSED  ---->  ");
		//	break;
		//case BT_STATUS_PLAYING:
		//	printk("BT_STATUS_PLAYING  ---->  ");
		//	break;
		case BT_STATUS_INCOMING:
			printk("BT_STATUS_INCOMING  ---->  ");
			break;
		case BT_STATUS_OUTGOING:
			printk("BT_STATUS_OUTGOING  ---->  ");
			break;
		case BT_STATUS_MULTIPARTY:
			printk("BT_STATUS_MULTIPARTY  ---->  ");
			break;
		case BT_STATUS_INQUIRY:
			printk("BT_STATUS_INQUIRY  ---->  ");
			break;
		case BT_STATUS_INQUIRY_COMPLETE:
			printk("BT_STATUS_INQUIRY_COMPLETE  ---->  ");
			break;

		case BT_STATUS_CONNECTING:
			printk("BT_STATUS_CONNECTING  ---->  ");
			break;

		case BT_STATUS_CONNECTED_INQUIRY_COMPLETE:
			printk("BT_STATUS_CONNECTED_INQUIRY_COMPLETE  ---->  ");
			break;

		default:
			printk("default state %d  ---->  ", prev_status);
			break;
	}

	switch (curr_status)
	{
		case BT_STATUS_NONE:
			printk("BT_STATUS_NONE\n");
			break;
		case BT_STATUS_WAIT_PAIR:
			printk("BT_STATUS_WAIT_PAIR\n");
			break;		
		case BT_STATUS_CONNECTED:
#if (CONFIG_BT_TRANS_SUPPORT_DEV_NUMBER > 1)
            tr_bt_mgr_auto.multi_dev_pair_support = 0;
#endif
			printk("BT_STATUS_CONNECTED\n");
			break;
		case BT_STATUS_DISCONNECTED:
#if (CONFIG_BT_TRANS_SUPPORT_DEV_NUMBER > 1)
            tr_bt_mgr_auto.multi_dev_pair_support = 0;
#endif
			printk("BT_STATUS_DISCONNECTED\n");
			break;
		case BT_STATUS_TWS_WAIT_PAIR:
			printk("BT_STATUS_TWS_WAIT_PAIR\n");
			break;
		case BT_STATUS_TWS_PAIRED:
			printk("BT_STATUS_TWS_PAIRED\n");
			break;
		case BT_STATUS_TWS_UNPAIRED:
			printk("BT_STATUS_TWS_UNPAIRED\n");
			break;
		case BT_STATUS_MASTER_WAIT_PAIR:
			printk("BT_STATUS_MASTER_WAIT_PAIR\n");
			break;
		//case BT_STATUS_PAUSED:
		//	printk("BT_STATUS_PAUSED\n");
		//	break;
		//case BT_STATUS_PLAYING:
		//	printk("BT_STATUS_PLAYING\n");
		//	break;
		case BT_STATUS_INCOMING:
			printk("BT_STATUS_INCOMING\n");
			break;
		case BT_STATUS_OUTGOING:
			printk("BT_STATUS_OUTGOING\n");
			break;
		case BT_STATUS_MULTIPARTY:
			printk("BT_STATUS_MULTIPARTY\n");
			break;
		case BT_STATUS_INQUIRY:
			printk("BT_STATUS_INQUIRY\n");
			break;
		case BT_STATUS_INQUIRY_COMPLETE:
			printk("BT_STATUS_INQUIRY_COMPLETE\n");
			break;

		case BT_STATUS_CONNECTING:
			printk("BT_STATUS_CONNECTING\n");
			break;

		case BT_STATUS_CONNECTED_INQUIRY_COMPLETE:
			printk("BT_STATUS_CONNECTED_INQUIRY_COMPLETE\n");
			break;

		default:
			printk("default state %d\n", curr_status);
			break;
	}
}

int tr_bt_manager_check_app_bt_relevant(void)
{
	if (memcmp(app_manager_get_current_app(), "tr_line_in", strlen("tr_line_in")) == 0)
	{
		return 1;
	}
	else if (memcmp(app_manager_get_current_app(), "tr_usound", strlen("tr_usound")) == 0)
	{
		return 1;
	}
	else if (memcmp(app_manager_get_current_app(), "tr_sd_mplayer", strlen("tr_sd_mplayer")) == 0)
	{
		return 1;
	}
	
	return 0;
}

void tr_bt_mgr_sleep_after_disconnect_timer_handle(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	if (tr_bt_mgr_auto.auto_move_stop_and_keep_flag == 1)
	{
		return;
	}
	
	//当前没有设备名额
	if(tr_bt_manager_auto_get_connectable_device_num() <= 0)
	{
		return;
	}

	thread_timer_stop(&tr_bt_mgr_auto.tr_bt_mgr_sleep_after_disconnect_timer);

	switch(CONFIG_BT_AUTO_CONNECT_DISCONNECT_MODE)
	{
		case DISCONNECT_MODE_ALWAYS_INQUIRY:
		{
			_api_bt_inquiry_and_deal_result(CONFIG_BT_AUTO_INQUIRY_NUM, CONFIG_BT_AUTO_INQUIRY_TIME, 0);
			break;
		}

		case DISCONNECT_MODE_RECONNECT_PAIRDLIST:
		{
			if( tr_bt_manager_startup_reconnect() == 0)
			{
				_api_bt_inquiry_and_deal_result(CONFIG_BT_AUTO_INQUIRY_NUM, CONFIG_BT_AUTO_INQUIRY_TIME, 0);
			}
			break;
		}

		case DISCONNECT_MODE_INQUIRY_RECONNECT_ALTER:
		{
			if(tr_bt_mgr_auto.disconnect_mode_alter_flag == 0)
			{
				tr_bt_mgr_auto.disconnect_mode_alter_flag = 1;
				_api_bt_inquiry_and_deal_result(CONFIG_BT_AUTO_INQUIRY_NUM, CONFIG_BT_AUTO_INQUIRY_TIME, 0);
			}
			else
			{
				tr_bt_mgr_auto.disconnect_mode_alter_flag = 0;
				if( tr_bt_manager_startup_reconnect() == 0)
				{
					_api_bt_inquiry_and_deal_result(CONFIG_BT_AUTO_INQUIRY_NUM, CONFIG_BT_AUTO_INQUIRY_TIME, 0);
				}
			}
			break;
		}

		case DISCONNECT_MODE_WAIT_RMT_RECONNECT:
		{
			bt_manager_set_connectable(true);
			break;
		}
		
		default:
			break;

	}

}



void tr_bt_manager_auto_move_when_poweron(void)
{
#ifdef CONFIG_BT_AUTO_CONNECT_SUPPORT_USB_BINDING_ADDR
/*
	u8_t binding_addr[6] = {0};
	bt_paired_info_t*  paired_list;
	u8_t ret, i;
	int connect_times = 0;
	u8_t paired_list_saved_binding_dev_flag = 0;

	ret = nvram_config_get("USB_BINDING_BT_ADDR", binding_addr, 6);
	if(ret != 6)
	{
		SYS_LOG_INF("BINDING BT ADDR IS NULL, WAIT AND RETURN!\n");
		return;
	}
	
	if (memcmp(binding_addr, "\x00\x00\x00\x00\x00\x00", BT_DEV_ADDR_LEN) == 0)
	{
		SYS_LOG_INF("BINDING BT ADDR IS WRONG, WAIT AND RETURN!\n");
		return;
	}*/
#endif

	switch(CONFIG_BT_AUTO_CONNECT_STARTUP_MODE)
	{
		case STARTUP_MODE_ALWAYS_INQUIRY:
		{
			_api_bt_inquiry_and_deal_result(CONFIG_BT_AUTO_INQUIRY_NUM, CONFIG_BT_AUTO_INQUIRY_TIME, 0);
			break;
		}
		case STARTUP_MODE_RECONNECT_PAIRDLIST:
		{
#ifdef CONFIG_BT_AUTO_CONNECT_SUPPORT_USB_BINDING_ADDR
#else
			if( tr_bt_manager_startup_reconnect() == 0)
			{
				_api_bt_inquiry_and_deal_result(CONFIG_BT_AUTO_INQUIRY_NUM, CONFIG_BT_AUTO_INQUIRY_TIME, 0);
			}
			
#endif

			break;
		}

		default:
			break;
	}
}

void tr_bt_manager_auto_move_when_inquiry_complete(void)
{
	u8_t * addr[2];
	int dev_num;

	dev_num = tr_bt_manager_auto_get_connectable_device_num();
	if(dev_num <= 0)
	{
		SYS_LOG_INF("left connect_dev_num = %d, no more, cannot connect", dev_num);
		return;
	}
	
	dev_num = tr_bt_manager_inquiry_ctrl_select_dev(CONFIG_BT_AUTO_CONNECT_RSSI_THRESHOLD
				, CONFIG_BT_AUTO_CONNECT_DEV_FULL_NAME
				, addr
				, dev_num);
	if(dev_num > 0)
	{
		for(int i = 0; i < dev_num; i++)
		{
			api_bt_connect(addr[i]);
		}
	}
	else
	{
		switch(CONFIG_BT_AUTO_CONNECT_INQU_RESULT_FAIL_STANDARD_MODE)
		{
			case INQU_RESULT_FAIL_MODE_ALWAYS_INQUIRY:
			{
				_api_bt_inquiry_and_deal_result(CONFIG_BT_AUTO_INQUIRY_NUM, CONFIG_BT_AUTO_INQUIRY_TIME, 0);
				break;
			}

			case INQU_RESULT_FAIL_MODE_RECONNECT_PAIRDLIST:
			{
				if(tr_bt_manager_startup_reconnect() == 0)
				{
					_api_bt_inquiry_and_deal_result(CONFIG_BT_AUTO_INQUIRY_NUM, CONFIG_BT_AUTO_INQUIRY_TIME, 0);
				}
				break;
			}

			case INQU_RESULT_FAIL_MODE_INQUIRY_RECONNECT_ALTER:
			{
				if(tr_bt_mgr_auto.inquiry_reconnect_alter_flag == 0)
				{
					tr_bt_mgr_auto.inquiry_reconnect_alter_flag = 1;
					_api_bt_inquiry_and_deal_result(CONFIG_BT_AUTO_INQUIRY_NUM, CONFIG_BT_AUTO_INQUIRY_TIME, 0);
				}
				else
				{
					tr_bt_mgr_auto.inquiry_reconnect_alter_flag = 0;
					if(tr_bt_manager_startup_reconnect() == 0)
					{
						_api_bt_inquiry_and_deal_result(CONFIG_BT_AUTO_INQUIRY_NUM, CONFIG_BT_AUTO_INQUIRY_TIME, 0);
					}
				}
				break;
			}

			default:
				break;

		}
	}
}

/* 蓝牙自动连接动作
 */
void tr_bt_mgr_auto_move_timer_handle(struct thread_timer *ttimer, void *expiry_fn_arg)
{
#ifndef CONFIG_BT_TRANS_AUTO_INQUIRY_AND_CONNECT
	return ;
#endif

	int cur_status;

	if(sys_get_work_mode() != TRANSFER_MODE)
		return;

	if (tr_bt_mgr_auto.auto_move_stop_and_keep_flag == 1)
		return;

	//不是蓝牙相关应用就退出auto
	if(!tr_bt_manager_check_app_bt_relevant())
	{
		return;
	}

	//当前没有设备名额
	if(tr_bt_manager_auto_get_connectable_device_num() <= 0)
	{
		return;
	}

	cur_status = bt_manager_get_status();

	if(cur_status == tr_bt_mgr_auto.prev_status)
	{
		return;
	}
	else
	{
		tr_bt_manager_show_status_change(tr_bt_mgr_auto.prev_status, cur_status);
	}

	//开机动作(若第一个应用是udisk)
	/*
	if(bt_manager->transmitter.power_on == 1)
	{
		bt_manager->transmitter.power_on = 0;
		tr_bt_manager_auto_move_when_poweron();
		goto exit;
	}*/

	//开机动作
	if((tr_bt_mgr_auto.prev_status == 0) && (cur_status == BT_STATUS_WAIT_PAIR))
	{
		tr_bt_manager_auto_move_when_poweron();
		goto exit;
	}

	//搜索完成动作
	if((cur_status == BT_STATUS_INQUIRY_COMPLETE) 
		|| (cur_status == BT_STATUS_CONNECTED_INQUIRY_COMPLETE))
	{
		tr_bt_manager_auto_move_when_inquiry_complete();
		goto exit;
	}

	/*连接断开和连接未成功动作*/
	if(((tr_bt_mgr_auto.prev_status >= BT_STATUS_PAUSED
				&& tr_bt_mgr_auto.prev_status <= BT_STATUS_3WAYIN)
			|| tr_bt_mgr_auto.prev_status == BT_STATUS_CONNECTING
			|| tr_bt_mgr_auto.prev_status == BT_STATUS_CONNECTED_INQUIRY_COMPLETE)
		&& (cur_status == BT_STATUS_DISCONNECTED))
	{
		if (!thread_timer_is_running(&tr_bt_mgr_auto.tr_bt_mgr_sleep_after_disconnect_timer))
		{
			thread_timer_start(&tr_bt_mgr_auto.tr_bt_mgr_sleep_after_disconnect_timer, CONFIG_BT_AUTO_CONNECT_DISCONNECT_SLEEP_TIME*1000, 0);
		}
		goto exit;
	}

	//当前还有设备名额
	if(tr_bt_manager_auto_get_connectable_device_num() > 0 && cur_status < BT_STATUS_INQUIRY)
	{
		thread_timer_start(&tr_bt_mgr_auto.tr_bt_mgr_sleep_after_disconnect_timer, CONFIG_BT_AUTO_CONNECT_DISCONNECT_SLEEP_TIME*1000, 0);
		goto exit;
	}

exit:

	tr_bt_mgr_auto.prev_status = cur_status;
	return;
}

void tr_bt_manager_auto_move_init(void)
{
#ifndef CONFIG_BT_TRANS_AUTO_INQUIRY_AND_CONNECT
	return ;
#endif

	tr_bt_mgr_auto.auto_move_stop_and_keep_flag = 0;
	tr_bt_mgr_auto.prev_status = 0;
	tr_bt_mgr_auto.inquiry_reconnect_alter_flag = 0;
	tr_bt_mgr_auto.disconnect_mode_alter_flag = 0;

	thread_timer_init(&tr_bt_mgr_auto.tr_bt_mgr_sleep_after_disconnect_timer, tr_bt_mgr_sleep_after_disconnect_timer_handle, NULL);

	thread_timer_init(&tr_bt_mgr_auto.tr_bt_mgr_auto_move_timer, tr_bt_mgr_auto_move_timer_handle, NULL);
	thread_timer_start(&tr_bt_mgr_auto.tr_bt_mgr_auto_move_timer, 100, 100);
	
}

void tr_bt_manager_auto_move_deinit(void)
{
#ifndef CONFIG_BT_TRANS_AUTO_INQUIRY_AND_CONNECT
	return ;
#endif

	thread_timer_stop(&tr_bt_mgr_auto.tr_bt_mgr_auto_move_timer);
	thread_timer_stop(&tr_bt_mgr_auto.tr_bt_mgr_sleep_after_disconnect_timer);
}

/* 蓝牙停止所有动作
 */
void tr_bt_manager_auto_move_keep_stop(bool need_to_disconn)
{
	tr_bt_mgr_auto.auto_move_stop_and_keep_flag = 1;

	bt_manager_set_connectable(false);

	api_bt_connect_cancel(NULL);
    k_sleep(K_MSEC(10));

    if(sys_get_work_mode())
    {
        api_bt_inquiry_cancel();
    }

	if(need_to_disconn)
	{
		printk("dissconnect all dev\n");
		api_bt_disconnect(-1);
	}

    k_sleep(K_MSEC(10));
	printk("+++++++++++++++++tr_bt_manager_auto_move_stop_all++++++++++++++++++++++\n");

}

void tr_bt_manager_auto_move_restart_from_stop(void)
{
	bt_manager_set_connectable(true);

	printk("+++++++++++++++++tr_bt_manager_auto_move_restart++++++++++++++++++++++\n");
	tr_bt_manager_show_status_change(tr_bt_mgr_auto.prev_status, bt_manager_get_status());
	
	if(tr_bt_mgr_auto.prev_status == bt_manager_get_status())
	{
		SYS_LOG_INF("prev status and curr status is same\n");
		if(tr_bt_mgr_auto.prev_status == BT_STATUS_WAIT_PAIR)
			tr_bt_mgr_auto.prev_status = 0;
		else if(tr_bt_mgr_auto.prev_status == BT_STATUS_INQUIRY_COMPLETE)
			tr_bt_mgr_auto.prev_status = BT_STATUS_INQUIRY;
		else if(tr_bt_mgr_auto.prev_status == BT_STATUS_CONNECTED_INQUIRY_COMPLETE)
			tr_bt_mgr_auto.prev_status = BT_STATUS_PAUSED;
		else if(tr_bt_mgr_auto.prev_status == BT_STATUS_DISCONNECTED)
			tr_bt_mgr_auto.prev_status = BT_STATUS_PAUSED;
	}

	tr_bt_mgr_auto.auto_move_stop_and_keep_flag = 0;

}


