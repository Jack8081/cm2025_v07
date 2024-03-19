/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system ble advertise
 */

#include <bt_manager.h>
#include <thread_timer.h>
#include "system_app.h"
#include "app_config.h"
#include "list.h"
#include <ctype.h>
#include <user_comm/sys_comm.h>


typedef struct
{
    struct list_head  node;
    char  var_id;
    u8_t  used;
    u8_t  size;
    u8_t  data[1];
} ble_adv_var_t;

typedef struct 
{
	uint8_t type;
	uint8_t len;
	uint8_t data[29];
}adv_data_t;


typedef struct
{
    u8_t  adv_enabled;
    u8_t  adv_type;
    
}ble_adv_ctrl_t;

struct ble_adv_mngr
{
    u8_t inited;
	u8_t enabled;
    u8_t nesting;
    u8_t advert_state;
    u8_t advert_mode;
	u8_t adv_after_connected;
	u8_t limited;
	ble_adv_ctrl_t adv_ctrl;
	
	struct thread_timer check_adv_timer;
	struct thread_timer update_timer;
    struct list_head  var_list;
};


static struct ble_adv_mngr	ble_adv_info;


static int ble_adv_update(void);

void ble_adv_limit_set(u8_t value)
{
	struct ble_adv_mngr *p = &ble_adv_info;
	
	p->limited |= value;
	SYS_LOG_INF("p->limited %d", p->limited);

	return;
}

void ble_adv_limit_clear(u8_t value)
{
	struct ble_adv_mngr *p = &ble_adv_info;

	p->limited &= ~ value;
	SYS_LOG_INF("p->limited %d", p->limited);

	return;
}

static int ble_adv_set_variable(char var_id, void* data, int size)
{
	struct ble_adv_mngr *p = &ble_adv_info;
	ble_adv_var_t *var;
	u8_t used = 0;

	//SYS_LOG_INF("### set var '%c' : %s\n", var_id, data);
	//print_hex("data: ", data, size);
	
    list_for_each_entry(var, &p->var_list, node)
    {
        if (var->var_id == var_id)
        {
            if (var->size == size)
            {
                if (memcmp(var->data, data, size) == 0)
                {
                    return 0;
                }

                goto end;
            }

            used = var->used;

            list_del(&var->node);
            mem_free(var);
            
            break;
        }
    }

    var = mem_malloc(sizeof(ble_adv_var_t) + size);
	if(var == NULL)
	{
		SYS_LOG_ERR("malloc failed");
		return -1;
	}
    var->var_id = var_id;
    var->used   = used;
    var->size   = size;

    list_add_tail(&var->node, &p->var_list);

end:
    memcpy(var->data, data, size);

    if (var->used)
    {
		SYS_LOG_INF("### set var used.\n");
        ble_adv_update();
    }

	return 0;
}

static int ble_adv_get_variable(char var_id, void* data, int size)
{
	struct ble_adv_mngr *p = &ble_adv_info;
	ble_adv_var_t *var;

	//SYS_LOG_INF("### get id = '%c', data: %s", var_id, data);
	
    list_for_each_entry(var, &p->var_list, node)
    {
        if (var->var_id == var_id)
        {
            if (size > var->size)
            {
                size = var->size;
            }

            memcpy(data, var->data, size);
			//print_hex("data: ", data, size);

            /* 标记已使用该变量, 在重新设置变量时更新广播数据
             */
            var->used = 1;

            return size;
        }
    }

	//SYS_LOG_INF("### get null !!!\n");
	return 0;
}

static int ble_get_hex(void* buf, int size, u8_t* str)
{
    u8_t*  p = buf;
    int    i = 0;
    int    j = 0;
    
    while (i < size && str[j] != '\0')
    {
        char  ch1 = str[j];
        char  ch2 = str[j+1];

        if (ch1 == '%')
        {
            i += ble_adv_get_variable(ch2, &p[i], size - i);
            j += 2;
            
            continue;
        }
        
        if (isxdigit(ch1) &&
            isxdigit(ch2))
        {
            char  tmp_str[3];

            tmp_str[0] = ch1;
            tmp_str[1] = ch2;
            tmp_str[2] = '\0';

            p[i] = (u8_t)strtoul(tmp_str, NULL, 16);
            
            i += 1;
            j += 2;
        }
        else
        {
            j += 1;
        }
    }

    return i;
}

static int ble_adv_set_enable(bool enable)
{
	struct ble_adv_mngr *p = &ble_adv_info;

	if(p->advert_state != enable)
    //if(p->enabled != enable)
    {
        p->enabled = enable;
        ble_adv_update();
		SYS_LOG_INF("### set enable=%d\n", enable);
    }

    return 0;
}

static int ble_adv_set_mode(uint32_t mode)
{
	struct ble_adv_mngr *p = &ble_adv_info;

    if (mode != CFG_ID_BLE_ADVERTISING_MODE_2)
    {
        mode = CFG_ID_BLE_ADVERTISING_MODE_1;
    }
    
    if (p->advert_mode != CFG_ID_BLE_ADVERTISING_MODE_2)
    {
        p->advert_mode = CFG_ID_BLE_ADVERTISING_MODE_1;
    }
    
    if (p->advert_mode != mode)
    {
        p->advert_mode = mode;
        
        ble_adv_update();
    }

    return 0;
}

/* 检查打开或关闭 BLE 广播
 */
static bool ble_adv_check_enable(void)
{
    system_app_context_t*  manager = system_app_get_context();
    system_status_t *sys_status = &manager->sys_status;
	int tws_role = bt_manager_tws_get_dev_role();
	struct ble_adv_mngr *p = &ble_adv_info;
    bool enable;

    if (!bt_manager_is_ready())
    {
        return false;
    }
	
	/* 可根据相关状态动态控制打开或关闭 BLE 广播
	 */
	enable = p->adv_ctrl.adv_enabled;
	if(!enable)
	{
		goto end;
	}

	/* 是否配置 “蓝牙连接后才进行BLE广播” */
	if(p->adv_after_connected && bt_manager_get_connected_dev_num() < 1)
	{
		/* 前台充电延时关闭ble，不在这里关闭 */
		if(!sys_status->in_front_charge)
		{
#ifndef CONFIG_LE_AUDIO_APP
			/* 蓝牙断开，ble也要断开，并关闭广播 */
			if(bt_manager_ble_is_connected())
			{
				bt_manager_ble_disconnect(NULL);
			}
#endif			
			enable = false;
			goto end;
		}
	}

	int8_t ble_connected_num = 1;
#ifdef CONFIG_LE_AUDIO_APP
	ble_connected_num = 2;
#endif
	
	/* BLE 已连接关闭蓝牙 */
	if ((bt_manager_ble_is_connected() >= ble_connected_num) || (p->limited))
	{
        enable = false;
        goto end;
	}
	
    /* 副机在第一次尝试回连主机过程中禁止广播以避免和主机广播冲突?
     */
	if(btif_br_is_auto_reconnect_tws_first(BTSRV_TWS_SLAVE))
	{
        enable = false;
        goto end;
	}

    /* BLE 不可连接类型的广播? (例如 BLE 广播手机弹窗)
     */
	if (p->adv_ctrl.adv_type == BLE_ADV_NONCONN_IND)
	{
        /* 耳机从充电盒取出后关闭广播
         */
         if(!sys_status->in_charger_box)
         {
			 enable = false;
         }
		 else
		 {
			 /* 左右耳机都处于充电盒中时, 只由主机进行广播
			  */
			  if(tws_role==BTSRV_TWS_SLAVE && \
			  	sys_status->tws_remote_in_charger_box)
			  {
				  enable = false;
			  }
		 }
		 goto end;
	}
	
    /* BLE 可连接类型的广播?
     * 已组对连接的 TWS 副机不需要广播, 只由主机进行广播
     */
	if(tws_role == BTSRV_TWS_SLAVE)
	{
		enable = false;
		goto end;
	}

end:
	ble_adv_set_enable(enable);

	return enable;
}

/* 配置 BLE 广播模式
 */
static void ble_adv_config_mode(void)
{
	CFG_Struct_BLE_Manager ble_mngr;
	
    /* 默认使用配置模式 1
     */
    uint32_t  mode = CFG_ID_BLE_ADVERTISING_MODE_1;

    app_config_read(CFG_ID_BLE_MANAGER,&ble_mngr,0,sizeof(CFG_Struct_BLE_Manager));

    /* 配对连接过后使用 BLE 广播模式 2,
     * 配对列表非空且不在配对模式?
     */
	if(ble_mngr.Use_Advertising_Mode_2_After_Paired)
	{
        if(bt_manager_get_connected_dev_num() > 0)
        {
            mode = CFG_ID_BLE_ADVERTISING_MODE_2;
        }
        else if (!bt_manager_is_pair_mode())
        {
			if(btif_br_check_phone_paired_info() == true)
			{
				mode = CFG_ID_BLE_ADVERTISING_MODE_2;
			}
        }
	}

	ble_adv_set_mode(mode);
}

/* %a 引用蓝牙地址 (小端 6 字节)
 * %A 引用蓝牙地址 (大端 6 字节)
 */
static void ble_adv_dev_addr(void)
{
    CFG_Struct_BT_Device   cfg_bt_dev;
    u8_t  data[6];
    int   i;
	
    app_config_read(CFG_ID_BT_DEVICE, &cfg_bt_dev, 0, sizeof(CFG_Struct_BT_Device));
	
    /* 蓝牙地址 (小端) */
    {
		memcpy(data, cfg_bt_dev.BT_Address, sizeof(data));
		
    	ble_adv_set_variable('a', data, sizeof(data));
    }

    /* 蓝牙地址 (大端) */
    {
        for (i = 0; i < 6; i++)
        {
            data[i] = cfg_bt_dev.BT_Address[5-i];
        }
    	
		ble_adv_set_variable('A', data, sizeof(data));
    }
}

/* %B 引用设备电量 (3 字节)
 *
 * data[0] : R 右设备电量百分比
 * data[1] : L 左设备电量百分比
 * data[2] : 充电盒电量百分比
 * bit-7   : 表示充电状态
 * 0xff    : 表示状态缺失
 */
static void ble_adv_bat_level(void)
{
    system_app_context_t*  manager = system_app_get_context();
    system_status_t *sys_status = &manager->sys_status;
	audio_manager_configs_t *audio_cfg;
    u8_t  data[3];
	u8_t  dev_channel;
	u8_t  tmp;

    /* 本机设备电量
     */
    #ifdef CONFIG_POWER_MANAGER  
    data[0] = power_manager_get_battery_capacity_local();
	#else
    data[0] = 100;
    #endif
    /* TWS 另一设备电量
     */
	if(bt_manager_tws_get_dev_role() >= BTSRV_TWS_MASTER)
	{
		data[1] = 0xFF;
	}
	else
	{
        /* 主机在第一次尝试回连副机过程中广播左右设备电量保持一致?
         * 第一次回连超时后将指示副机状态缺失
         */
		if(btif_br_is_auto_reconnect_tws_first(BTSRV_TWS_MASTER))
		{
            data[1] = data[0];
		}
		else  // TWS 另一设备状态缺失?
		{
			data[1] = 0xff;
		}
	}

	audio_cfg = audio_channel_get_config();
    dev_channel = (audio_cfg->hw_sel_dev_channel | audio_cfg->sw_sel_dev_channel);

    if (dev_channel == 0)
    {
        data[1] = data[0];
    }
    else if (dev_channel == AUDIO_DEVICE_CHANNEL_L)
    {
    	tmp = data[0];
		data[0] = data[1];
		data[1] = tmp;
    }

	data[2] = sys_status->charger_box_bat_level;

    ble_adv_set_variable('B', data, sizeof(data));
}

/* %C 引用充电盒打开/关闭状态 (1 字节)
 *
 * 0x00 : 表示充电盒已打开
 * 0x08 : 表示充电盒已关闭
 */
static void ble_adv_chg_box_state(void)
{
    system_app_context_t*  manager = system_app_get_context();
    system_status_t *sys_status = &manager->sys_status;
    u8_t  data = 0x08;

    if (sys_status->charger_box_opened &&
        !sys_status->chg_box_bat_low)
    {
        data = 0x00;
    }

    ble_adv_set_variable('C', &data, sizeof(data));
}

/* TMA 信息
 */
static bool g_link_already = false;
static void ble_adv_tma_info(void)
{
	//int bt_status = bt_manager_get_status();

	/* TMA device status */
	{
		u8_t  data[2];
		if (1 <= bt_manager_get_connected_dev_num())
		{
			data[0] = 0x10;
			data[1] = 0x00;
			g_link_already = true;
		}
		else if (false == g_link_already)
		{
			data[0] = 0x01;
			data[1] = 0x00;
		}
		else
		{
			data[0] = 0x00;
			data[1] = 0x00;
		}
		ble_adv_set_variable('s', data, sizeof(data));
	}
	 /* TMA app cookie */
	{
		u8_t  data[6];
#if 0
		if (sizeof(data) != vdisk_read(
				VFD_TMA_COOKIE, (void *)data, sizeof(data)));
#endif
		memset(data, 0, sizeof(data));
		
		ble_adv_set_variable('c', data, sizeof(data));
	}
}

/* %N 引用蓝牙名称
 */
static void ble_adv_dev_name(void)
{
    CFG_Struct_BT_Device   cfg_bt_dev;
	
    app_config_read(CFG_ID_BT_DEVICE, &cfg_bt_dev, 0, sizeof(CFG_Struct_BT_Device));

    ble_adv_set_variable('N', cfg_bt_dev.BT_Device_Name, strlen(cfg_bt_dev.BT_Device_Name));
}

static void ble_adv_assign_mf_data(adv_data_t *t, CFG_Struct_BLE_Advertising_Mode_1* adv_mode)
{
	int  len = ble_get_hex
	(
		(void *)t->data, sizeof(t->data), adv_mode->Manufacturer_Specific_Data
	);

	if (len >= 2)
	{
		/* Manufacturer Specific Data
		 */
		t->type = BT_DATA_MANUFACTURER_DATA;
		t->len = len;

		if (memcmp(t->data, "\x4C\x00\x07\x19", 4) == 0)
		{
		  /* 跟海涛确认，aap有使用风险，不打开，
					 * encode做其它功能用的，可以不使用的
					 */
#if 0
			if (bt_manager_aap_is_enabled())
			{
				/* ble_adv_encode_ex in ble_encode.o */
				ble_adv_encode_ex(&t->val[len - 16]);
			}
#endif
		}
	}
}

static void ble_adv_assign_name(adv_data_t *t, CFG_Struct_BLE_Advertising_Mode_1* adv_mode)
{
    char *s = (char*)adv_mode->BLE_Device_Name;
	char *data = (char *)t->data;
    
    if (strlen(s) > 0)
    {
        int  i = 0;
        int  j = 0;

        while (i < sizeof(t->data) - 1 && s[j] != '\0')
        {
            if (s[j] == '%')
            {
                i += ble_adv_get_variable
                (
                    s[j+1], &data[i], sizeof(t->data) - 1 - i
                );
                j += 2;
            }
            else
            {
                data[i] = s[j];

                i += 1;
                j += 1;
            }
        }

        /* Complete Local Name
         */
        t->type = BT_DATA_NAME_COMPLETE;
        t->len  = strlen((char*)t->data);
    }
}

static void sys_ble_check_advertise(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	
	if(ble_adv_check_enable())
	{
        /* 配置 BLE 广播模式
         */
         ble_adv_config_mode();

        /* %a 引用蓝牙地址 (小端 6 字节)
         * %A 引用蓝牙地址 (大端 6 字节)
         */
         ble_adv_dev_addr();

        /* %B 引用设备电量 (3 字节)
         *
         * data[0] : R 右设备电量百分比
         * data[1] : L 左设备电量百分比
         * data[2] : 充电盒电量百分比
         * bit-7   : 表示充电状态
         * 0xff    : 表示状态缺失
         */
         ble_adv_bat_level();

		/* %C 引用充电盒打开/关闭状态 (1 字节)
		 *
		 * 0x00 : 表示充电盒已打开
		 * 0x08 : 表示充电盒已关闭
		 */
		 ble_adv_chg_box_state();

        /* 广播TMA信息
         */
         ble_adv_tma_info();

		/* %N 蓝牙名称
		 */
		 ble_adv_dev_name();

	}

}

static int ble_adv_update(void)
{
	struct ble_adv_mngr *p = &ble_adv_info;
	
	thread_timer_stop(&p->update_timer);

	thread_timer_start(&p->update_timer, 0, 0);

	return 0;
}

static int ble_adv_add_flag(uint8_t *ad_buf)
{
	int ad_len = 0;
	btmgr_ble_cfg_t *cfg_ble = bt_manager_ble_config();

	ad_buf[ad_len++] = 2;	/* Len */
	ad_buf[ad_len++] = BT_DATA_FLAGS;	/* Type */
	if (cfg_ble->ble_address_type) {
		ad_buf[ad_len++] = (BT_LE_AD_GENERAL);
	} else {
		/* 0: Public address, set BT_LE_AD_NO_BREDR, or trigger connect le will connect br */
		ad_buf[ad_len++] = (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR);
	}

	return ad_len;
}

void sys_ble_advertise(struct thread_timer *ttimer, void *expiry_fn_arg)
{
    CFG_Struct_BLE_Advertising_Mode_1 adv_mode;
    struct ble_advertising_params_t adv_param;
	struct ble_adv_mngr *p = &ble_adv_info;
	adv_data_t *adv_data;
    adv_data_t *t;
    uint8_t ad_buf[31], sd_buf[31];
	int ad_len, sd_len;
	int i, len;

	SYS_LOG_INF("### enter ++\n");

	thread_timer_stop(&p->update_timer);

	if(!p->inited || p->nesting)
	{
		return;
	}

    p->nesting = 1;

    /* 在连接后也关闭广播
     */
	if (!p->enabled/* || bt_manager_ble_is_connected()*/)
	{
		if (p->advert_state != false)
		{
			p->advert_state = false;
			
			bt_manager_ble_set_advertise_enable(false);
		}
		goto end;
	}

    /* BLE 广播配置 */
    {
        if (p->advert_mode != CFG_ID_BLE_ADVERTISING_MODE_2)
        {
            p->advert_mode = CFG_ID_BLE_ADVERTISING_MODE_1;
        }
		app_config_read(p->advert_mode,&adv_mode,0,sizeof(CFG_Struct_BLE_Advertising_Mode_1));

#if 0
		CFG_Struct_LE_AUDIO_Manager cfg_le_audio_mgr;
		app_config_read(CFG_ID_LE_AUDIO_MANAGER, &cfg_le_audio_mgr, 0, sizeof(CFG_Struct_LE_AUDIO_Manager));

		if (cfg_le_audio_mgr.LE_AUDIO_Enable){
			memset(adv_mode.BLE_Device_Name, 0, sizeof(adv_mode.BLE_Device_Name));
			memcpy(adv_mode.BLE_Device_Name, cfg_le_audio_mgr.LE_AUDIO_Device_Name, strlen(cfg_le_audio_mgr.LE_AUDIO_Device_Name));
		}
#endif
	
    }

    /* 禁止广播?
     */
	if (adv_mode.Advertising_Type== BLE_ADV_DISABLE)
	{
		if (p->advert_state != false)
		{
			p->advert_state = false;
			
			bt_manager_ble_set_advertise_enable(false);
		}
	
		goto end;
	}
	
    p->advert_state = true;

	adv_data = mem_malloc(4 *sizeof(adv_data_t));
	if(adv_data == NULL)
	{
		SYS_LOG_ERR("malloc failed");
		goto end;
	}
	memset(adv_data, 0, 4 *sizeof(adv_data_t));

	bt_manager_ble_set_advertise_enable(false);

    /* 清除变量使用标志 */
    {
        ble_adv_var_t* var;
        
        list_for_each_entry(var, &p->var_list, node)
        {
            var->used = 0;
        }
    }

    /* 厂商自定义数据 */
	ble_adv_assign_mf_data(&adv_data[0], &adv_mode);

    /* 服务 UUIDs (16-Bit) */
    {
        t = &adv_data[1];
		
        len = ble_get_hex((void *)t->data, sizeof(t->data), adv_mode.Service_UUIDs_16_Bit);

        if (len > 0 && (len % 2) == 0)
        {
            /* Complete List of 16-bit Service Class UUIDs
             */
            t->type = BT_DATA_UUID16_ALL;
            t->len = len;
        }
	}

    /* 服务 UUIDs (128-Bit) */
    {
        t = &adv_data[2];

        len = ble_get_hex((void *)t->data, sizeof(t->data), adv_mode.Service_UUIDs_128_Bit);

        if (len > 0 &&
            (len % 16) == 0)
        {
            /* Complete List of 128-bit Service Class UUIDs
             */
            t->type = BT_DATA_UUID128_ALL;
            t->len  = len;
        }
    }

    /* BLE 设备名称 */
	ble_adv_assign_name(&adv_data[3], &adv_mode);

    /* BLE 广播参数 */
    {
        memset(&adv_param,0,sizeof(adv_param));
    	adv_param.advertising_interval_min = adv_mode.Advertising_Interval_Ms *1000/625;
    	adv_param.advertising_interval_max = adv_mode.Advertising_Interval_Ms *1000/625;
    	adv_param.advertising_type = adv_mode.Advertising_Type;
    	bt_manager_ble_set_advertising_params(&adv_param);
    }

	ad_len = sd_len = 0;
	ad_len = ble_adv_add_flag(ad_buf);

	for(i=0; i<4; i++)
	{
        t = &adv_data[i];
		if(t->len >0 && ad_len +t->len +2 <=31 && sd_len==0)
		{
			ad_buf[ad_len] = t->len +1;		// len
			ad_len ++;
			ad_buf[ad_len] = t->type;	// type
			ad_len ++;
			memcpy(ad_buf +ad_len, t->data, t->len);
			ad_len += t->len;
		}
		else if(t->len >0 && sd_len +t->len +2 <=31)
		{
			sd_buf[sd_len] = t->len +1;		// len
			sd_len ++;
			sd_buf[sd_len] = t->type;	// type
			sd_len ++;
			memcpy(sd_buf +sd_len, t->data, t->len);
			sd_len += t->len;
		}
	}

	if(ad_len>0 || sd_len>0)
	{
		SYS_LOG_INF("### set adv data: ad_len=%d, sd_len=%d\n", ad_len, sd_len);
		print_hex("### ad_data: ", ad_buf, ad_len);
		print_hex("### sd_data: ", sd_buf, sd_len);
		/* set ble advertise ad & sd data */
		bt_manager_ble_set_adv_data(ad_buf, ad_len, sd_buf, sd_len);
		
		/* enable advertise */
		//bt_manager_ble_set_advertise_enable(true);
	}
	bt_manager_ble_set_advertise_enable(true);

	mem_free(adv_data);

end:
	p->nesting = 0;

}

int sys_ble_advertise_init(void)
{
    CFG_Struct_BLE_Advertising_Mode_1 adv_mode;
	struct ble_adv_mngr *p = &ble_adv_info;
	CFG_Struct_BLE_Manager ble_mngr;

    app_config_read(CFG_ID_BLE_MANAGER,&ble_mngr,0,sizeof(CFG_Struct_BLE_Manager));
    if (!ble_mngr.BLE_Enable)
    {
        return false;
    }

	p->inited = 1;
	p->adv_after_connected = ble_mngr.Advertising_After_Connected;

    if (p->var_list.next == NULL)
    {
        INIT_LIST_HEAD(&p->var_list);
    }

    app_config_read(CFG_ID_BLE_ADVERTISING_MODE_1,&adv_mode,0,sizeof(CFG_Struct_BLE_Advertising_Mode_1));

	/* 启用 BLE 功能时默认允许广播
	 */
	p->adv_ctrl.adv_enabled = 1;
	p->adv_ctrl.adv_type = adv_mode.Advertising_Type;

	thread_timer_init(&p->check_adv_timer, sys_ble_check_advertise, NULL);
	thread_timer_start(&p->check_adv_timer, adv_mode.Advertising_Interval_Ms, adv_mode.Advertising_Interval_Ms);

	thread_timer_init(&p->update_timer, sys_ble_advertise, NULL);	

	SYS_LOG_INF("### ble init.\n");
	return 0;
}

void sys_ble_advertise_deinit(void)
{
	struct ble_adv_mngr *p = &ble_adv_info;

	thread_timer_stop(&p->update_timer);
	thread_timer_stop(&p->check_adv_timer);
	
	SYS_LOG_INF("### ble deinit.\n");
}

