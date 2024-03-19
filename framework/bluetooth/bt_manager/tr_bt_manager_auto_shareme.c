/*
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
#if (CONFIG_BT_TRANS_SUPPORT_DEV_NUMBER > 1)
static void _tr_bt_manager_2nd_device_timeout(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	if(tr_bt_mgr_auto.multi_dev_pair_support)
	{
		tr_bt_mgr_auto.multi_dev_pair_support = 0;
		SYS_LOG_INF("multi_dev_pair_support=%d", tr_bt_mgr_auto.multi_dev_pair_support);
	}
}

void tr_bt_manager_auto_2nd_device_start_stop(void)
{
    if(api_bt_get_disable_status())
    {
        tr_bt_mgr_auto.multi_dev_pair_support = 0;
    }

    if(api_bt_get_connect_dev_num() == 0)
    {
        api_bt_connect_cancel(NULL);
        _api_bt_inquiry_and_deal_result(CONFIG_BT_AUTO_INQUIRY_NUM, CONFIG_BT_AUTO_INQUIRY_TIME, 0);
        return;
    }

	tr_bt_mgr_auto.multi_dev_pair_support ^= 1;

	SYS_LOG_INF("multi_dev_pair_support=%d", tr_bt_mgr_auto.multi_dev_pair_support);

	if(tr_bt_mgr_auto.multi_dev_pair_support)
	{
#if (CONFIG_BT_TRANS_MULTI_DEV_PAIR_TIMEOUT > 0)
		thread_timer_init(&tr_bt_mgr_auto.multi_dev_pair_timer, _tr_bt_manager_2nd_device_timeout, NULL);
		thread_timer_start(&tr_bt_mgr_auto.multi_dev_pair_timer, CONFIG_BT_TRANS_MULTI_DEV_PAIR_TIMEOUT, 0);
#endif
		
		if(api_bt_get_connect_dev_num() < tr_bt_manager_config_connect_dev_num())
		{
			_api_bt_inquiry_and_deal_result(CONFIG_BT_AUTO_INQUIRY_NUM, CONFIG_BT_AUTO_INQUIRY_TIME, 0);
		}
		else
		{
			SYS_LOG_INF("connected %d devices, no more.", api_bt_get_connect_dev_num());
            tr_bt_mgr_auto.multi_dev_pair_support = 0;
		}
	}
	else
	{
#if (CONFIG_BT_TRANS_MULTI_DEV_PAIR_TIMEOUT > 0)
		thread_timer_stop(&tr_bt_mgr_auto.multi_dev_pair_timer);
#endif
		
		if(api_bt_get_connect_dev_num() > 0)
		{
			api_bt_connect_cancel(NULL);
			api_bt_inquiry_cancel();
		}
	}
}

#endif

int tr_bt_manager_auto_get_connectable_device_num(void)
{
#if (CONFIG_BT_TRANS_SUPPORT_DEV_NUMBER > 1)
	if(tr_bt_mgr_auto.multi_dev_pair_support)
	{
		return (tr_bt_manager_config_connect_dev_num() - api_bt_get_connect_dev_num());
	}
#endif

	return (1 - api_bt_get_connect_dev_num());
}

void tr_bt_manager_disconnect_all_devices(void)
{
	api_bt_disconnect(-1);
}


