/***************************************************************************
 * Copyright (c) 2018 Actions Semi Co., Ltd.
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

 * Author: <rcmai@actions-semi.com>
 *
 * Description:    蓝牙模块 API
 *
 * Change log:
 *	2020/12/20: Created by Rcmai.
 ****************************************************************************
 */

/**
**  蓝牙状态
**/
#ifndef __API_BT_MODULE_H__
#define __API_BT_MODULE_H__
#include "bt_manager.h"

typedef enum 
{	
	BT_A2DP_CONNECTED = 0x0,
	BT_A2DP_DISCONNECTED,			//0x1
	BT_AVRCP_CONNECTED,				//0x2
	BT_AVRCP_DISCONNECTED,		//0x3
	BT_CONNECT_FAIL,					//0x4
	BT_START_CONNECT,					//0x5
	BT_HFP_CONNECTED,         //0x6
	BT_HFP_DISCONNECTED      //0x7
}bt_state_e;

/**
**  AVRCP 的命令
**/
typedef enum
{
/* AVRCP CMD */
	AVRCP_PLAY = 0x44,				//0x44
	AVRCP_STOP,					      //0x45
	AVRCP_PAUSE,				      //0x46
	AVRCP_REWIND_PRESSED =0x48, 	//0x48
	AVRCP_FAST_FORWARD_PRESSED,   //0x49
	AVRCP_FORWARD = 0x4b,					//0x4b
	AVRCP_BACKWARD,               //0x4c	
	AVRCP_REWIND_RELEASE = 0xc8,  //0xc8	
	AVRCP_FAST_FORWARD_RELEASE   //0xc9
}bt_avrcp_cmd_e;


typedef enum
{
	A2DP_START = 0x00,
	A2DP_SUSPEND = 0x01,
	A2DP_BUZZER_MUTE =0x10,
	A2DP_BUZZER_UNMUTE = 0x11
}bt_a2dp_act_e;


typedef struct
{
	unsigned char reserved: 3;
	// role = 0:  模组作蓝牙A2DP src角色, 即模组作为发射端连接上了接收端（如连上了音箱）
	// role = 1:  模组作蓝牙A2DP snk角色, 即模组作为接收端被发射端连接上（如被手机连接上）
	unsigned char role: 1;
	// 表示这条连接在连接列表中的 index 号,有些函数可以通过该index来操作蓝牙连接，如 disconnect 等
	unsigned char linked_index: 4;
}bt_state_info_t;

typedef struct {
	u8_t hf_connected:1;
	u8_t a2dp_connected:1;
	u8_t avrcp_connected:1;
	u8_t spp_connected:3;
	u8_t hid_connected:1;
	
	u8_t avrcp_vol_sync:1;
}bt_dev_svr_info_t;

/**************************************************************
**	Description:	蓝牙模块硬件准备好的 event API  evt_bt_hwready(void)  
**	Input:      
**  	NULL  
**      
**  Output:
**  	NULL  
**  
**	Return:  
**    NULL 
** 
**	Note:
**      
***************************************************************/
typedef void (* evt_bt_hwready)(void);     


/**************************************************************
**	Description:	蓝牙连接的状态的 event API  evt_bt_hwready(void) ，当蓝牙状态发生变化时，会通过该函数调用上层回调函数来通知上层
**	Input:      
**  	unsigned char state：当前蓝牙的状态, 具体的状态描述见 bt_state_e
**		unsigned char state_info: 状态所附带的一些信息，具体的定义描述见 bt_state_info_t
**    unsigned char * bd_addr: 上述状态所对应的远端设备的蓝牙地址, 蓝牙地址的长度为6个字节，请在回调函数中需要把地址拷走
**      
**  Output:
**  	NULL  
**  
**	Return:  
**    NULL 
** 
**	Note:
**     
***************************************************************/
typedef void (* evt_bt_state)(unsigned char state, unsigned char state_info, unsigned char *bd_addr); 


/**************************************************************
**	Description:	蓝牙AVRCP 服务的 event API  (evt_bt_avrcp *) 
**	Input:      
**  	unsigned char avrcp_cmd： 远端蓝牙设备发过来的 avrcp命令
**		unsigned char data_base_index:  标识是连接列表中的哪个蓝牙设备发过来的avrcp命令, 命令具体描述见 bt_avrcp_cmd_e
**  
**	Return:  
**    NULL 
** 
**	Note:
**     
***************************************************************/
typedef void (* evt_bt_avrcp)(unsigned char avrcp_cmd, unsigned char data_base_index); 


/**************************************************************
**	Description:	蓝牙搜索结果 event API  (evt_bt_inquiry *) 
**	Input:      
**  	void* result：inquiry_dev_info_t * size 搜索结果返回给上层，注意上层需要在回调函数中把结果读走.
**      int     size;       返回查询结果数量   0:为查询过程返回(single inquiry result event)   其他为查询结束返回(inquiry complete event)
**	Return:  
**    NULL 
** 
**	Note:
**     
***************************************************************/
typedef void (* evt_bt_inquiry)(void* result, int size);     


/**************************************************************
**	Description:	蓝牙搜索完成 event API  (evt_bt_inquiry_finished *) 
**	Input:      
**  	int num： 把本次搜索到的设备个数返回给上层，并表示已搜索完成。
**      
**	Return:  
**    NULL 
** 
**	Note:
**     
***************************************************************/
typedef void (* evt_bt_inquiry_finished)(int num);


/**************************************************************
**	Description:	返回取消蓝牙连接结果的 event API  (evt_bt_connect_cancel *) 
**	Input:      
**  	bool result： 取消蓝牙连接的结果。
**      result = 1: 取消成功
**      result = 其他： 取消失败
**  Output:
**  	NULL  
**  
**	Return:  
**    NULL 
** 
**	Note:
**     
***************************************************************/
typedef void (* evt_bt_connect_cancel)(bool result);   


/**************************************************************
**	Description:	绝对音量变化时通知上层的 event API  (evt_bt_absvolume_changed *) 
**	Input:      
**  	int absvolume： 变化后最新的绝对音量的值
**  
**	Return:  
**    NULL 
** 
**	Note:
**     
***************************************************************/
typedef int (* evt_bt_absvolume_changed)(int absvolume);



//如下结构定义一些常用的事件回调函数，蓝牙模块有信息需要通知上层时，通过这里的函数进行回调
typedef struct
{
	evt_bt_state cb_state;
	evt_bt_hwready cb_hwready;
	evt_bt_avrcp cb_avrcp;
	evt_bt_absvolume_changed cb_absvolume;
}evt_bt_cb_t;



/**************************************************************
**	Description:	蓝牙模块的初始化的 API api_bt_init
**	Input:      
**    evt_bt_cb_t* callback: 模组初始化时会把事件回调函数表告知模块，这样模块有事件通话上层时就可以通过这些函数来通知
**  
**	Output:  
**    NULL
**
**  Return:
**    返回API 调用的结果，如果调用不成功，会把错误原因返回
**	    1： 成功
**      0或非1： 失败
**  
**	Note:
**    调用完初始化函数后，如果硬件ready ，会通过 (evt_bt_hwready *) 这个函数把硬件 ready 信号发出
***************************************************************
**/
int api_bt_init(evt_bt_cb_t* callback);


/**************************************************************
**	Description:	搜索蓝牙设备的 API  api_bt_inquiry
**	Input:      
**    int number: 搜索蓝牙设备的个数，达到这个个数后退出  0:释放上次搜索结果占用内存。
**    int timeout： 搜索的超时时间，单位为秒，如果搜不够个数，则到超时也退出  0:直接返回上次搜索结果
**    u32_t cod： 非0为 过滤搜索设备cod，0：不过滤
**    evt_bt_inquiry *evtcb: 搜索结果回调函数, 搜索结果会通过这个函数返回
** 
**  Output:
**		NULL
**
**	Return:  
**    int result: 返回API 调用的结果，如果调用不成功，会把错误原因返回
**	    0： 成功
**      非0： 失败
** 
**	Note:
***************************************************************
**/
int api_bt_inquiry(int number, int timeout, u32_t cod, evt_bt_inquiry evtcb);

/**************************************************************
**	Description:该宏释放api_bt_inquiry接口申请的内存	
***************************************************************/
#define API_BT_INQUIRY_FREE() api_bt_inquiry(0, 10, 0, NULL)

/**************************************************************
**	Description:	取消当前蓝牙设备搜索的 API  api_bt_inquiry_cancel
**	Input:      
**  	NULL  
**      
**  Output:
**  	NULL  
**  
**	Return:  
**    int result: 返回API 调用的结果，如果调用不成功，会把错误原因返回
**	    1： 成功
**      0或非1： 失败
** 
**	Note:
**  
***************************************************************/
int api_bt_inquiry_cancel(void);


/**************************************************************
**	Description:	连接蓝牙设备的 API api_bt_connect
**	Input:      
**    unsigned char * bt_addr: 要连接的蓝牙设备的蓝牙地址的首址，该参数为指针所指向的6个字节
**    
**	Return:  
**    int result: 返回API 调用的结果，如果调用不成功，会把错误原因返回
**	    1： 成功
**      -1： 失败，该设备已连接或正在连接，不要重复发起连接
**      -2： 已超过最大的可连接数，不能再建立新的连接
** 
**	Note:
**     1) 连接状态会通过 (evt_bt_state *) 这个函数来返回给上层
**     2) 如果想同时发起两个连接，可以在调用完1设备的连接后马上再调用第2个设备的连接，这样每个设备在连接上后都会返回相应的状态信息。
***************************************************************/
int api_bt_connect(unsigned char * bt_addr);


/**************************************************************
**	Description:	取消当前正在进行的蓝牙设备连接动作的 API  api_bt_connect_cancel
**	Input:      
**  	evt_bt_connect_cancel * evtcb: 回报是否取消成功的回调函数
**      
**  Output:
**  	NULL  
**  
**	Return:  
**    int result: 返回API 调用的结果，如果调用不成功，会把错误原因返回
**	    1： 成功
**      0或非1： 失败 (如果当前状态在连接中才能取消，如果已连接上，则不能取消，则该调用就会返回失败)
** 
**	Note:
**      如果在调用该接口已来不及取消，设备已连上，也可以再调用 api_bt_disconnect() 函数断开连接
***************************************************************/
int api_bt_connect_cancel(evt_bt_connect_cancel evtcb);


/**************************************************************
**	Description:	断开蓝牙设备连接的 API  api_bt_disconnect
**	Input:      
**  	int data_base_index: 蓝牙连接的索引号，该值是蓝牙连接时通过 (evt_bt_state *) 函数返回的index 值
**          data_base_index = -1 时，即要求把当前所有设备断开连接.
**	Return:  
**    int result: 返回API 调用的结果，如果调用不成功，会把错误原因返回
**	    1：成功调用
**      0: 没有指定的要断开的设备
**      -1： 其他错误
** 
**	Note:
**     断开连接的结果会导致蓝牙状态的变化，会通过 (evt_bt_state *)回调函数返回给上层
***************************************************************/
int api_bt_disconnect(int data_base_index);



/**************************************************************
**	Description:	获取蓝牙的绝对音量的 API  api_bt_get_absvolume
**	Input:      
**  	void *addr: 需要设置音量的蓝牙地址: 
							单设备连接时，可直接设置成NULL, 默认设置已连接的设备的音量;
							多设备连接时，必须写入地址，才能成功设置音量, 否则返回0xff;
** 
**	Return:  
**    int result: 设置成功，返回绝对音量的值 0-0x7f
				  设置不成功，AVRCP没有连接，返回0x80
				  设置不成功，远端音箱不支持音量同步，返回0x81
				  设置不成功，其他原因导致，反应0xff
** 
**	Note:
**     
***************************************************************/
int api_bt_get_absvolume (void *addr);


/**************************************************************
**	Description:	设置蓝牙的绝对音量的 API  api_bt_set_absvolume
**	Input:      
**  	void *addr: 需要设置音量的蓝牙地址: 
							单设备连接时，可直接设置成NULL, 默认设置已连接的设备的音量;
							多设备连接时，必须写入地址，才能成功设置音量, 否则返回0xff;
**  	int volume: 设置绝对音量的值， 0-127
** 
**	Return:  
**    int result: 设置成功，返回绝对音量的值 0-0x7f
				  设置不成功，AVRCP没有连接，返回0x80
				  设置不成功，远端音箱不支持音量同步，返回0x81
				  设置不成功，远程设备改变音量时不支持返回音量信息
				  设置不成功，其他原因导致，反应0xff

** 
**	Note:
**     
***************************************************************/
int api_bt_set_absvolume(void *addr, int volume);


/**************************************************************
**	Description:	获取指定设备远端蓝牙设备蓝牙名称的 API   api_bt_get_remote_name()
**	Input: 
**     unsigned char *addr: 制定要获取名称的设备地址。 
**     unsigned char * name: 用于存放蓝牙名称的 buffer，名字以 '\0' 结束
**     int bufsize:  上层开的用于存放蓝牙名称的 buffer 的长度，API中会根据这个长度判断，如果过长会截断并加上 '\0' 结束符。
**     
**	Return:  
**    int result: 返回实际名字的长度
**	    <=0： 获取名字失败 
** 
**	Note:
**      
***************************************************************/
int api_bt_get_remote_name(unsigned char *addr ,unsigned char * name, int bufsize);


/**************************************************************
**	Description:	A2DP 流控制的 API   api_bt_a2dp_action
**	Input:      
**  	int action:  需要执行的控制，详细的控制命令在 bt_a2dp_act_e 中定义
**    int data_base_index: 连接列表中的设备index, 表示要操作哪个设备中的流，1拖2时就会有2个流,需要通过这个来确定操作哪个流
**  
**	Return:  
**    int result: 返回API 调用的结果，如果调用不成功，会把错误原因返回
**	    1： 成功
**      0或非1： 失败 
** 
**	Note:
**      
***************************************************************/
int api_bt_a2dp_action(int action, int data_base_index);


/**************************************************************
**	Description:	让模块进入认证测试模式的 API  api_bt_enter_Qtest 
**	Input:      
**  	 int mod: 
**       0: 进入BQB 测试模式
**       1: 进入FCC 测试模式 
**  
**	Return:  
**    NULL
** 
**	Note:
**    注：BQB模式是直接蓝牙底层与蓝牙综测仪交互的，上层调用该接口后不再返回，测试完后需要重启
**        FCC模式是直接通过FCC工具用串口来控制进行测试，上层调用该接口后也是不返回，测试完后需要重启
***************************************************************/
void api_bt_enter_Qtest(int mod);



/**************************************************************
**	Description:	获取蓝牙配对列表信息的 API，api_bt_get_PDL
**	Input:      
**  	paired_dev_info_t * pdl: 用于存放需要获取的 pdl 的信息
**    int index:   需要获取PDL 中的哪个设备的信息, 从0开始，第1个设备 index=0 0xff 为全部配对列表
**  
**	Return:  
**    int result: 
**	    >0： 获取成功， result值就是PDL中总的记录个数，即记录已配对的设备数量
**      =0： 表示配对列表为空
**      =-1： 表示获取时出错
** 
**	Note:
**      
***************************************************************/
int api_bt_get_PDL(paired_dev_info_t * pdl, int index);




/**************************************************************
**	Description:	删除配对列表(PDL)指定index 项的 API，api_bt_del_PDL
**	Input:      
**    int index:   需要删除 PDL 中的哪个设备，从0开始，第1个设备 index=0  0xff删除全部配对列表
**  
**	Return:  
**    int result: 
**	    1： 成功删除
**      =0或非1的值： 删除失败
** 
**	Note:
**    如果想通过蓝牙地址来删除，可以先调用 api_bt_get_PDL()函数查到index号再调用 
***************************************************************/
int api_bt_del_PDL (int index);


/**************************************************************
**	Description:	获取蓝牙模块本地的蓝牙名称的 API  api_bt_get_local_name
**	Input:      
**  	unsigned char * buf:  用于存放取出的蓝牙名称的buffer的首址，该 buffer 的字节数至少32 + 1byte，由上层提供
**  
**
**	Return:  
**    result>0: 成功 返回name 长度
**    result<0: 失败
**	Note:
**
***************************************************************/
int api_bt_get_local_name(unsigned char *buf);


/**************************************************************
**	Description:	获取蓝牙模块本地的蓝牙设备信息的 API  api_bt_get_local_btaddr
**	Input:      
**  	unsigned char * addr:  用于存放取出的蓝牙地址的buffer的首址，该 buffer 为6个字节数组，由上层提供
**  
**	Return:  
**    result = 0: 成功
**    result > 0: 失败
**	Note:
**      
***************************************************************/
int api_bt_get_local_btaddr(unsigned char *addr);

/**************************************************************
**	Description:	设置蓝牙模块本地的蓝牙设备信息的 API  api_bt_set_local_btaddr
**	Input:      
**  	unsigned char * addr:  用于设置蓝牙地址的buffer的首址，该 buffer 为6个字节数组，由上层提供
**  
**	Return:  
**    result = 0: 成功
**    result > 0: 失败
**	Note:
**      
***************************************************************/
int api_bt_set_local_btaddr(unsigned char *addr);

/**************************************************************
**	Description:	返回当前连接的蓝牙设备状态
**	Input:      
**  	NULL  
**  
**	Return:  
**    enum BT_PLAY_STATUS: 返回API
**  BT_STATUS_NONE                  = 0x0100,
**  BT_STATUS_WAIT_PAIR             = 0x0200,
**	BT_STATUS_CONNECTED			= 0x0400,
**	BT_STATUS_DISCONNECTED		= 0x0800,
**	BT_STATUS_TWS_WAIT_PAIR         = 0x1000,
**	BT_STATUS_TWS_PAIRED            = 0x2000,
**	BT_STATUS_TWS_UNPAIRED          = 0x4000,
**	BT_STATUS_MASTER_WAIT_PAIR      = 0x8000,
**  BT_STATUS_PAUSED                = 0x0001,
**  BT_STATUS_PLAYING               = 0x0002,
**	BT_STATUS_INCOMING              = 0x0004,
**	BT_STATUS_OUTGOING              = 0x0008,
**	BT_STATUS_ONGOING               = 0x0010,
**	BT_STATUS_MULTIPARTY            = 0x0020,
**	BT_STATUS_SIRI                  = 0x0040,
**	BT_STATUS_SIRI_STOP             = 0xFFBF,
**	BT_STATUS_3WAYIN                = 0x0080,
**      
** 
**	Note:
**      
***************************************************************/
enum BT_PLAY_STATUS api_bt_get_status(void);

/**************************************************************
**	Description:	根据设备地址查询设备svr status
**	Input:      
**  	addr:   设备地址
**      dev_svr 保存返回状态结构体
**  
**	Return:  
**    int result: true 获取成功 fals 获取失败
** 
**	Note:
**      
***************************************************************/
bool api_bt_get_svr_status(void *addr, bt_dev_svr_info_t *dev_svr);

/**************************************************************
**	Description:	返回当前远端蓝牙设备连接数量
**	Input:      
**  	NULL  
**  
**	Return:  
**    int result: 返回API 调用的结果，返回当前蓝牙设备连接的数量
** 
**	Note:
**      
***************************************************************/
int api_bt_get_connect_dev_num(void);

/**************************************************************
**	Description:	temp 的 API  
**	Input:      
**  	u8_t enable:  1 mute enable;  0 mute disable
**  
**	Return: 无 
** 
**	Note:
**      
***************************************************************/
void api_bt_a2dp_mute(u8_t enable);

/**************************************************************
**	Description:	停止一切当前蓝牙自动动作
**	Input:      
**  	bool need_to_disconn:  TRUE: disconnect all connect;  FAlse: keep current connection
**  
**	Return: 无 
** 
**	Note:	停止动作包括：
				取消正在连接；
				取消搜索；
				设置不可连接，不可见；
				设置断开当前连接(可选)
**      
***************************************************************/
void api_bt_auto_move_keep_stop(bool need_to_disconn);

/**************************************************************
**	Description:	恢复蓝牙自动动作
**	Input:  无
**  
**	Return: 无 
** 
**	Note:    设置可连接，可见，并根据当前状态继续动作
			是否继续动作由CONFIG_BT_TRANS_AUTO_INQUIRY_AND_CONNECT决定
**      
***************************************************************/
void api_bt_auto_move_restart_from_stop(void);

/**************************************************************
**	Description:获取蓝牙接口是否允许操作状态
**	Input:  无
**  
**	Return: bool  1:disable  0:enable
** 
**	Note: disable  inquiry与connect接口将无法使用
**      
***************************************************************/
bool api_bt_get_disable_status(void);

/**************************************************************
**	Description: 根据蓝牙地址获取设备连接索引。
**	Input:      
**    unsigned char * addr: 要连接的蓝牙设备的蓝牙地址的首址，该参数为指针所指向的6个字节
**    
**	Return:  
**    u8_t : 返回API 设备连接的索引,从0开始,0xff返回失败,设备未连接
** 
***************************************************************/
u8_t api_bt_connect_index_by_addr(unsigned char *addr);

int api_bt_disallowed_connect(void *addr, u8_t disallowed);
#endif
