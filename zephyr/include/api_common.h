/***************************************************************************
 * Copyright (c) 2021 Actions Semi Co., Ltd.
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
 * Description:    通用 API
 *
 * Change log:
 *	2020/12/20: Created by Rcmai.
 ****************************************************************************
 */

#ifndef __API_COMMON_H__
#define __API_COMMON_H__

#include "media_effect_param.h"
int check_prio(const uint8_t *name);
#define CK_PRIO() check_prio(__func__)
/*
 * struct defined
 * */
/**************************************************************
**	Description: 实现音频增益调节的 API  API_APP_GAIN_SET
**	Input:      
**    int dev_num: 设备号
**    int gain_level: 增益等级(0~20) 
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
int API_APP_GAIN_SET(int dev_num, int gain_level);


/**************************************************************
**	Description: 设置混音模式 API  API_APP_SET_MIX_MODE
**	Input:      
**    int mix_mode: 0为单声道模式， 1为立体声道模式, 2为安全模式
**  Output:
**		NULL
**
**	Return:  
**      NULL
** 
**	Note:
***************************************************************
**/
void API_APP_SET_MIX_MODE(int mix_mode);


/**************************************************************
**	Description: 获取混音模式 API  API_APP_GET_MIX_MODE
**	Input:      
**      NULL
**  Output:
**		mix_mode=0为单声道模式， mix_mode=1为立体声道模式 ， mix_mode=2为安全模式
**
**	Return:  
**      返回0为单声道模式， 1为立体声道模式, 2为安全模式
** 
**	Note:
***************************************************************
**/
int API_APP_GET_MIX_MODE(void);


/**************************************************************
**	Description: 获取设备增益等级的 API  API_APP_GET_GAIN_LEVEL
**	Input:      
**    int mic_dev_num: 设备号
**  Output:
**    int: 增益等级(0~20) 
**
**	Return:  
**    int result: 返回API 调用的结果，如果调用不成功，会把错误原因返回
**	    0~20：增益等级
**      -1： 失败
** 
**	Note:
***************************************************************
**/
int API_APP_GET_GAIN_LEVEL(int mic_dev_num);


/**************************************************************
**	Description: 静音接口 API  API_APP_MUTE_MIC
**	Input:      
**    int mic_dev_num: 设备号(0:设备0,1:设备1)
**    bool is_mute: true:静音 false:取消静音
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
int API_APP_MUTE_MIC(int mic_dev_num, bool is_mute);


/**************************************************************
**	Description: 获取上行数据能量值的 API  API_APP_GET_ENERGY_VAL
**	Input:      
**    int mic_dev_num: 设备号(0:设备0,1:设备1)
**    bool is_original: true:获取近端去除增益后的原始值 false:获取带增益的能量值
**  Output:
**		平均能量值(最大值2^15 - 1)
**
**	Return:  
**		平均能量值(最大值2^15 - 1)
** 
**	Note:
***************************************************************
**/
unsigned short API_APP_GET_ENERGY_VAL(int mic_dev_num, bool is_original);


/**************************************************************
**	Description: 实现降噪模式的开关设置的 API  API_APP_DENOISE_ONOFF
**	Input:      
**    int mode: ON/OFF
**    int level: 等级
**  Output:
**		NULL
**
**	Return:  
**    int result: 返回API 调用的结果，如果调用不成功，会把错误原因返回
**	    0： 成功
**      非0： 失败
** 
**	Note: not support
***************************************************************
**/
//int API_APP_DENOISE_ONOFF(int mode, int level);


/**************************************************************
**	Description: 实现EQ参数设置的 API  API_APP_EQ_PARAM
**	Input:      
**      int mode: 模式
**      char *eq: EQ参数指针
**  Output:
**		NULL
**
**	Return:  
**    int result: 返回API 调用的结果，如果调用不成功，会把错误原因返回
**	    0： 成功
**      非0： 失败
** 
**	Note: not support
***************************************************************
**/
//int API_APP_EQ_PARAM(int mode, char *eq);


/**************************************************************
**	Description: 设置低功耗模式的 API  API_APP_POWER_MODE_SET
**	Input:      
**    int mode: 0:sleep 1:work
**  Output:
**		NULL
**
**	Return:  
**    int result: 返回API 调用的结果，如果调用不成功，会把错误原因返回
**	    0： 成功
**      非0： 失败
** 
**	Note: not support
***************************************************************
**/
//int API_APP_POWER_MODE_SET(int mode);


/**************************************************************
**	Description: 获取连接状态的 API  API_APP_CONNECT_CONFIG
**	Input:      
**    int index: 获取设备的索引
**    struct conn_status *status: 结构体包含连接状态、信号质量rssi、连接设备addr
**  Output:
**    结构体包含连接状态,信号质量rssi,连接设备addr
**
**	Return:  
**    int result: 返回API 调用的结果，如果调用不成功，会把错误原因返回
**	    0： 成功
**      非0： 失败
** 
**	Note:
***************************************************************
**/
int API_APP_CONNECTION_CONFIG(int index, struct conn_status *status);


/**************************************************************
**	Description: 更新连接RSSI的 API  API_APP_CONNECTION_RSSI_UPDATA
**	Input:      
**    int index: 获取设备的索引
**  Output:
**    NULL
**
**	Return:  
**    int8_t result: 返回 指定index 的rssi
** 
**	Note:
***************************************************************
**/
int8_t API_APP_CONNECTION_RSSI_UPDATA(int index);


/**************************************************************
**	Description: 实现配置,状态信息同步的 API  API_APP_PASSTHROUGH_DATA
**	Input:      
**      int len: 辅助数据长度
**      char *data: 辅助数据
**  Output:
**		NULL
**
**	Return:  
**    int result: 返回API 调用的结果，如果调用不成功，会把错误原因返回
**	    0： 成功
**      非0： 失败
** 
**	Note: not support
***************************************************************
**/
//int API_APP_PASSTHROUGH_DATA(int len, char *data);


/**************************************************************
**	Description: 绑定蓝牙配对地址的 API  API_APP_SETPAIRADDR
**	Input:      
**      u8_t index: 绑定地址的索引 0：左声道蓝牙地址 1：右声道蓝牙地址
**      u8_t *param: 需要绑定的地址
**  Output:
**      NULL 
**
**	Return:  
**      NULL 
** 
**	Note:
***************************************************************
**/
void API_APP_SETPAIRADDR(u8_t index, u8_t *param);


/**************************************************************
**	Description: 获取绑定蓝牙配对地址的 API  API_APP_GETPAIRADDR
**	Input:      
**      u8_t index: 绑定地址的索引 0：左声道蓝牙地址 1：右声道蓝牙地址
**      u8_t *param: 存放获取到的绑定地址
**  Output:
**      绑定的地址
**
**	Return:  
**      NULL 
** 
**	Note:
***************************************************************
**/
void API_APP_GETPAIRADDR(u8_t index, u8_t *param);


/**************************************************************
**	Description: 启动le audio远端扫描、连接功能  API_APP_SCAN_WORK
**	Input:      
**      u8_t mode: 
**              0-回连绑定列表设备
**              1-搜索新设备并连接、配对为左声道；如果有新设备连接，则把新设备存到左声道绑定列表
**              2-搜索新设备并连接、配对为右声道；如果有新设备连接，则把新设备存到右声道绑定列表
**              3-搜索新设备并连接, 按连接顺序依次存到左、右声道绑定列表
**              4-停止当前搜索或回连
**              5-删除左、右声道绑定列表
**              6-删除左声道绑定列表  
**              7-删除右声道绑定列表
**  Output:
**		NULL
**
**	Return:  
**      NULL 
** 
**	Note:
***************************************************************
**/
void API_APP_SCAN_WORK(u8_t mode);


/**************************************************************
**	Description: 启动le audio近端广播功能  API_APP_ADV_WORK
**	Input:      
**      u8_t mode: 
**              0-开启广播模式为定向(绑定远端设备)广播
**              1-开启广播模式为不定向广播
**  Output:
**		NULL
**
**	Return:  
**      NULL 
** 
**	Note:
***************************************************************
**/
void API_APP_ADV_WORK(u8_t mode);


/**************************************************************
**	Description: 动态切换远端监听输出方式  API_APP_MOD_LISTEN_OUT_CHAN
**	Input:      
**      int out_chan: 
**              0-aout输出
**              1-i2s输出
**  Output:
**		NULL
**
**	Return:  
**      NULL 
** 
**	Note:
***************************************************************
**/
void API_APP_MOD_LISTEN_OUT_CHAN(int out_chan);


/**************************************************************
**	Description: 动态切换近端音效  API_APP_SET_MUSIC_EFX
**	Input:      
**      int efx_mode: 
**              0-默认音效
**              1-预置音效1
**              2-预置音效2
**              3-预置音效3
**              
**  Output:
**		NULL
**
**	Return:  
**      NULL 
** 
**	Note:
***************************************************************
**/
void API_APP_SET_PRE_MUSIC_EFX(int efx_mode);


/**************************************************************
**	Description: 动态修改近端PEQ  API_APP_MOD_PEQ
**	Input:      
**      int eq_point:EQ频点,btmusic(0~15共16个频点),leaudio(0~9共10个频点)
**      point_info_t *point_info:频点信息：
**                      short eq_fc: EQ频点的频率,btmusic(范围20~22000),leaduio(范围20~8000)
**                      short eq_gain:EQ频点的增益,范围-120~120,代表-12~12db,即eq_gain/10为真实增益
**                      short eq_qvalue:EQ频点的Q值(宽广度),范围3~300，代表0.3~30
**                      short eq_point_type:EQ频点的类型,(0:按钮disable 1:Peaking 2:High pass 3:Low pass 4:Low shelf 5:High shelf)
**                      short eq_point_en:EQ频点使能(0:disable 1:Speaker EQ 2:Mic EQ)
**      short peq_mod_type:修改peq的类型: BIT(0):修改频率 BIT(1):修改增益) BIT(2):修改Q值 BIT(3):修改频点类型 BIT(4):修改使能EQ频点
**              
**  Output:
**      NULL 
**
**	Return:  
**      int ret：ret = -1 输入频点超出范围而设置失败
**               ret = -2 其他原因设置失败   
**               ret = 0 设置成功
**               ret =  Error_mask > 0: 哪个BIT为1所对应的参数设置失败，例如：ret = 00000001, 即ret=BIT(0), 所以频率修改失败
** 
**	Note:
***************************************************************
**/
int API_APP_MOD_PEQ(int eq_point, point_info_t *point_info, short peq_mod_type);

int API_APP_MOD_MULTI_PEQ(point_info_t *point_info, int q_valid, int type_valid, int en_valid);

/**************************************************************
**	Description: 获取近端频点PEQ信息  API_APP_GET_PEQ
**	Input:      
**      int eq_point:EQ频点,btmusic(0~15共16个频点),leaudio(0~9共10个频点)
**      point_info:频点信息
**              
**  Output:
**	    包含频点信息的结构体	
**
**	Return:
**      ret = 0 获取成功  
**      ret = -1 输入频点超出范围而获取失败 
**      ret = -2 其他原因获取失败  
** 
**	Note:
***************************************************************
**/
int API_APP_GET_PEQ(int eq_point, point_info_t *point_info);  
#endif
