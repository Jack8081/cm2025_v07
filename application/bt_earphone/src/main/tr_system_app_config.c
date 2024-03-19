/*
 * Copyright (c) 2021Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app config
 */

int tr_sys_work_mode_init(void);

void tr_btmgr_config_update_1st(void)
{
    tr_btmgr_base_config_t *tr_base_config = tr_bt_manager_get_base_config();
	
    CFG_Struct_TR_BT_Device   tr_cfg_bt_dev;

    memset(tr_base_config, 0, sizeof(btmgr_base_config_t));

    app_config_read(CFG_ID_TR_BT_DEVICE, &tr_cfg_bt_dev, 0, sizeof(CFG_Struct_TR_BT_Device));
	
    tr_base_config->trans_work_mode = tr_cfg_bt_dev.Trans_Work_Mode;

    tr_sys_work_mode_init();
}

