/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: wanghui<wanghui@actions-semi.com>
 *
 * Change log:
 *	2018/1/6: Created by wanghui.
 */

#include "tr_bqb_app.h"
//#include <app_message.h>

extern int tr_bt_manager_startup_reconnect(void);
void tr_bqb_input_event_proc(struct app_msg *msg)
{
    SYS_LOG_INF("receive cmd %d",msg->cmd);
    struct tr_bqb_app_t *bqb = tr_bqb_get_app();
    if(!bqb)
    {
        return;
    }

    switch (msg->cmd)
    {
        case MSG_BQB_PLAY_PAUSE_RESUME:
            tr_bqb_start_or_stop_record();
            break;
        case MSG_BQB_PLAY_VOLUP:
//            bqb_key_func_volume_ctrl(BQB_KEY_VOLUP);
            break;
        case MSG_BQB_PLAY_VOLDOWN:
//            bqb_key_func_volume_ctrl(BQB_KEY_VOLDOWN);
            break;
        case MSG_BQB_CONNECT_START:
            tr_bt_manager_startup_reconnect();
            break;
        case MSG_BQB_CONNECT_DISCONNECT_A2DP:
            SYS_LOG_INF("A2DP disconnected\n");
            bd_address_t bd;
            btif_avrcp_disconnect(&bd);
            btif_a2dp_disconnect(&bd);
            break;
        case MSG_BQB_CONNECT_DISCONNECT_ACL:
            api_bt_disconnect(-1);
            break;
        case MSG_BQB_CLEAR_LIST:
            api_bt_del_PDL(0xff);
            break;
        case MSG_BQB_MUTE:
//            bt_manager_engine_ctrl_transmitter(NULL, MSG_BTENGINE_SIM_RF_SHIELD, 0);
            break;
        default:
            break;
    }
}


void tr_bqb_bt_passthrough_cmd_proc(struct app_msg *msg)
{
    u8_t op_id = *(u8_t *)msg->ptr;
    SYS_LOG_INF("receive cmd %d",op_id);
    struct tr_bqb_app_t *bqb = tr_bqb_get_app();
    if (!bqb)
        return;

    switch(op_id)
    {
        case BT_AVRCP_OPERATION_ID_MUTE:
            SYS_LOG_INF("BT_AVRCP_OPERATION_ID_MUTE\n");
            break;
        
        case BT_AVRCP_OPERATION_ID_PLAY:
        case BT_AVRCP_OPERATION_ID_PAUSE:
            SYS_LOG_INF("BT_AVRCP_OPERATION_ID_PLAY_PAUSE 0x%x\n", op_id);
            break;
        
        case BT_AVRCP_OPERATION_ID_STOP:
            SYS_LOG_INF("BT_AVRCP_OPERATION_ID_STOP\n");
            break;
        
        case BT_AVRCP_OPERATION_ID_REWIND:
            SYS_LOG_INF("BT_AVRCP_OPERATION_ID_REWIND\n");
            break;
        
        case BT_AVRCP_OPERATION_ID_FAST_FORWARD:
            SYS_LOG_INF("BT_AVRCP_OPERATION_ID_FAST_FORWARD\n");
            break;
        
        case BT_AVRCP_OPERATION_ID_FORWARD:
            SYS_LOG_INF("BT_AVRCP_OPERATION_ID_FORWARD\n");
            break;
        case BT_AVRCP_OPERATION_ID_BACKWARD:
            SYS_LOG_INF("BT_AVRCP_OPERATION_ID_BACKWARD\n");
            break;
        
        case BT_AVRCP_OPERATION_ID_UNDEFINED:
            SYS_LOG_INF("BT_AVRCP_OPERATION_ID_UNDEFINED\n");
            break;

        case BT_AVRCP_OPERATION_ID_REWIND_RELEASE:
            SYS_LOG_INF("BT_AVRCP_OPERATION_ID_REWIND_RELEASE\n");
            break;

        case BT_AVRCP_OPERATION_ID_FAST_FORWARD_RELEASE:
            SYS_LOG_INF("BT_AVRCP_OPERATION_ID_FAST_FORWARD_RELEASE\n");
            break;
                
        default:
            SYS_LOG_INF("not support, op_id %d", op_id);
            break;

    }
}


void tr_bqb_bt_event_proc(struct app_msg *msg)
{
    SYS_LOG_INF("receive cmd %d",msg->cmd);
    struct tr_bqb_app_t *bqb = tr_bqb_get_app();
    if(!bqb)
    {
        return;
    }

	switch(msg->cmd) 
	{
		case BT_REQ_RESTART_PLAY:
			SYS_LOG_INF("BT_REQ_RESTART_PLAY\n");
            tr_bqb_stop_record();
            k_sleep(K_MSEC(200));
            tr_bqb_start_record();
			break;
		case BT_A2DP_CONNECTION_EVENT:
			SYS_LOG_INF("BT_A2DP_CONNECTION_EVENT\n");
            //tr_bqb_open_record();
            tr_bqb_start_record();
			break;

        case BT_A2DP_DISCONNECTION_EVENT:
			SYS_LOG_INF("BT_REQ_STOP_PLAY\n");
            tr_bqb_stop_record();
            break;

		//case BT_RMT_VOL_SYNC_EVENT://33
		//	break;

		case BT_AVRCP_PASS_THROUGH_CTRL_EVENT:
			SYS_LOG_INF("BT_AVRCP_PASS_THROUGH_CTRL_EVENT\n");
			tr_bqb_bt_passthrough_cmd_proc(msg);
			break;

        case BT_A2DP_STREAM_START_EVENT:
            SYS_LOG_INF("BT_A2DP_STREAM_START_EVENT\n");
            //tr_bqb_start_record();
            break;

        case BT_A2DP_STREAM_OPEN_EVENT:
            SYS_LOG_INF("BT_A2DP_STREAM_OPEN_EVENT\n");
            //tr_bqb_open_record();
            break;

        case BT_DISCONNECTED:
            tr_bqb_stop_record();
            SYS_LOG_INF("BT_DISCONNECTED\n");

            break;


		default:
			break;
	}
}
