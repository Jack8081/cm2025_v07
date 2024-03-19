/*
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct tr_bt_manager_inquiry_info_t {
    u8_t running : 1;
	struct thread_timer inquiry_info_free_timer;
	tr_bt_manager_inquiry_info_t * tr_inquiry_info;
};

static struct tr_bt_manager_inquiry_info_t tr_bt_mgr_inquiry_info;

int tr_bt_manager_inquiry_ctrl_select_dev(s8_t lowest_rssi, u8_t *name, u8_t **bd, int bd_num)
{
	int i, num = 0;
	int index = 100, last_index = 100;
	int rssi = -100, last_rssi = -100;
	u8_t zero_addr[6] = {0};

	if(!tr_bt_mgr_inquiry_info.tr_inquiry_info)
	{
		SYS_LOG_INF("there is no inquiry result saved\n");
		return 0;
	}
	
	for(i = 0; i < tr_bt_mgr_inquiry_info.tr_inquiry_info->index; i++)
	{
		if(tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[i].rssi > rssi
			&& memcmp(tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[i].addr, zero_addr, 6) != 0)
		{
			//需要按名字连接最强信号的设备
			if(memcmp(name, "NULL", sizeof(name)) != 0 
				&& memcmp(tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[index].name, name, sizeof(name)) != 0)
			{
				continue;
			}
				
			last_index = index;
			last_rssi = rssi;
			rssi = tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[i].rssi;
			index = i;
		}
	}  

	if(index != 100 && rssi >= lowest_rssi)
	{
		printk("*************************************************\n");
		printk("select dev %s, addr %02x:%02x:%02x:%02x:%02x:%02x\n", \
				tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[index].name,\
				tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[index].addr[5], \
				tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[index].addr[4], \
				tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[index].addr[3], \
				tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[index].addr[2], \
				tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[index].addr[1], \
				tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[index].addr[0]);
		printk("*************************************************\n");
		bd[0] = tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[index].addr;
		num++;
	}
	
	if(bd_num > 1 && last_index != 100 && last_rssi >= lowest_rssi)
	{
		printk("*************************************************\n");
		printk("select dev %s, addr %02x:%02x:%02x:%02x:%02x:%02x\n", \
				tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[last_index].name,\
				tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[last_index].addr[5], \
				tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[last_index].addr[4], \
				tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[last_index].addr[3], \
				tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[last_index].addr[2], \
				tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[last_index].addr[1], \
				tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[last_index].addr[0]);
		printk("*************************************************\n");
		bd[1] = tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info[last_index].addr;
		num++;
	}
	
	return num;
	
}
// 请查询接口 回掉调用
static void tr_bt_manager_inquiry_ctrl_complete_result_proc(void * param, int dev_cnt)
{
	if(dev_cnt == 0 || param == NULL)
	{
		SYS_LOG_INF("inquiry result is NULL\n");
		return;
	}
	
	if(tr_bt_mgr_inquiry_info.tr_inquiry_info)
	{
		mem_free(tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info);
		mem_free(tr_bt_mgr_inquiry_info.tr_inquiry_info);
		tr_bt_mgr_inquiry_info.tr_inquiry_info = NULL;
	}
	
	tr_bt_mgr_inquiry_info.tr_inquiry_info  = mem_malloc(sizeof(tr_bt_manager_inquiry_info_t));
	tr_bt_mgr_inquiry_info.tr_inquiry_info->index = dev_cnt;
	tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info = mem_malloc(dev_cnt * sizeof(inquiry_info_t));

	memset(tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info, 0, dev_cnt * sizeof(inquiry_info_t));
	memcpy(tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info, param, dev_cnt * sizeof(inquiry_info_t));

    tr_bt_mgr_inquiry_info.running = 1;
}

int _api_bt_inquiry_and_deal_result(int number, int timeout, u32_t cod)
{
    return api_bt_inquiry(number, timeout, cod, tr_bt_manager_inquiry_ctrl_complete_result_proc);
}

u8_t *tr_bt_manager_get_inquiry_name(u8_t *addr)
{
/*
	uint32_t cur_status;

	bt_manager_dev_info_for_transmitter_t *dev_info;
    cur_status = bt_manager_get_status();
    dev_info = bt_manager_get_a2dp_active_dev_transmitter();
*/
	/*检查已连接的设备名*/
/*	if(cur_status == BT_STATUS_PAUSED || cur_status == BT_STATUS_PLAYING)
	{
		if(memcmp(dev_info->bd_addr, addr, 6) == 0)
		{
			return dev_info->dev_name;
		}
	}
	*/
	return NULL;
	
}


/*自动检测，释放搜索中申请的内存*/
void tr_bt_manager_inquiry_free_work_handler(struct thread_timer *ttimer, void *expiry_fn_arg)
{
    int cur_status = bt_manager_get_status();

    if(!tr_bt_mgr_inquiry_info.running)
    {
        return;
    }
	
	if(cur_status != BT_STATUS_INQUIRY_COMPLETE)
	{
		if(tr_bt_mgr_inquiry_info.tr_inquiry_info)
		{
			mem_free(tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info);
			mem_free(tr_bt_mgr_inquiry_info.tr_inquiry_info);
			tr_bt_mgr_inquiry_info.tr_inquiry_info = NULL;
            tr_bt_mgr_inquiry_info.running = 0;
		}
	}

}

void tr_bt_manager_inquiry_timer_init(void)
{
	tr_bt_mgr_inquiry_info.tr_inquiry_info = NULL;
    tr_bt_mgr_inquiry_info.running = 0;
	thread_timer_init(&tr_bt_mgr_inquiry_info.inquiry_info_free_timer, tr_bt_manager_inquiry_free_work_handler, NULL);
	thread_timer_start(&tr_bt_mgr_inquiry_info.inquiry_info_free_timer, 100, 100);
}

void tr_bt_manager_inquiry_timer_deinit(void)
{
	if(tr_bt_mgr_inquiry_info.tr_inquiry_info)
	{
		mem_free(tr_bt_mgr_inquiry_info.tr_inquiry_info->dev_info);
		mem_free(tr_bt_mgr_inquiry_info.tr_inquiry_info);
		tr_bt_mgr_inquiry_info.tr_inquiry_info = NULL;

	}
    tr_bt_mgr_inquiry_info.running = 0;
	thread_timer_stop(&tr_bt_mgr_inquiry_info.inquiry_info_free_timer);
}

