/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __FM_H__
#define __FM_H__
#include <zephyr.h>
#include <device.h>



struct acts_fm_device_data {
	struct device *fm_dev;
	struct device *i2c_dev;
	u32_t current_freq;
	u8_t mute_flag:1;
};

typedef enum
{
    eFM_RECEIVER = 0,                //5800,5802,5804
    eFM_TRANSMITTER = 1                //5820,5820NS
}E_RADIO_WORK;


typedef enum                    
{
    FM_BAND_CHINA_US = 0,				//china/USA band   87500---108000,  step 100kHz
    FM_BAND_JAPAN,						//Japan band   76000---90000, step 100kHz
    FM_BAND_EUROPE,					//Europe band  87500---108000, step 50kHz
    FM_BAND_MAX						//播放用户电台时，播放模式可以为Band_MAX
} fm_band_e;

typedef enum
{
    FM_STEREO = 0,				//[0]立体声
    FM_MONO,					//[1]单声道        
} fm_channel_e;

typedef enum
{
    FM_SEEKDIR_UP = 0x10,			//向上搜台
    FM_SEEKDIR_DOWN = 0x20,       	//向下搜台
}fm_seekdir_e;

/*seek 状态枚举*/
typedef enum
{
    FM_SEEK_NOT_START = 0,             //未启动硬件搜台或者此轮硬件搜台未搜到有效台
    FM_SEEK_DOING,						//硬件搜台过程中
    FM_SEEK_COMPLETE,                  //此轮硬件搜台过程结束并搜到有效台
} fm_seek_status_e;

typedef enum
{
    FM_STATUS_PLAY = 0,                   // 正常播放状态
    FM_STATUS_PAUSE,                       //正常暂停状态(静音状态)
    FM_STATUS_ERROR,                      //硬件出错    
} fm_status_e;

typedef struct
{
	uint16_t                 antenna;       //是否有天线插入
    uint16_t			       freq;				//当前频率
    uint16_t                 intensity;		 	//当前频点信号强度
    fm_channel_e       channel;        	//0，立体声；非0，单声道
    fm_band_e      		band;               //当前波段 
    fm_status_e   		    status;				//当前所处状态    
} fm_status_t;


/*FM模组初始化*/
int fm_init(const struct device *dev,u8_t band, u8_t level, u16_t freq);

/*FM模组进入standby*/
int fm_standy(const struct device *dev);

/*设置频点开始播放*/
int fm_set_freq(const struct device *dev, uint16_t freq);

/* 设置电台频段*/
int fm_set_band(const struct device *dev, fm_band_e band);

/* 设置搜台门限*/
int fm_set_threshold(const struct device *dev, uint8_t level);

/* 设置音量*/
int fm_set_volume(const struct device *dev, uint8_t volume);

/*静音与解除静音*/
int fm_set_mute(const struct device *dev, bool enable);

/*获取当前状态信息*/
int fm_get_status(const struct device *dev, fm_status_t *status);

/*开启软件搜台过程*/
int fm_soft_seek(const struct device *dev, uint16_t freq, fm_seekdir_e direction);

/*开启硬件搜台过程*/
int fm_hard_seek(const struct device *dev, uint16_t freq, fm_seekdir_e direction);

/* 手动退出seek 过程,如果当前是hard seek，就退出hard seek，如果当前是soft seek，就退出soft seek*/
int fm_break_seek(const struct device *dev);

/* 获取获取当前fm的seek状态，如果当前是hard seek，就返回hard seek的状态，如果当前是soft seek，就返回soft seek*/
int fm_get_seek_status(const struct device *dev,fm_seek_status_e *seek_status);



struct fm_driver_api {
	/*void (*enable)(struct device *dev);
	void (*disable)(struct device *dev);
	int (*set_freq)(struct device *dev, u16_t freq);
	u16_t (*get_freq)(struct device *dev);
	int (*set_mute)(struct device *dev, bool mute);
	int (*check_freq)(struct device *dev, u16_t freq);*/
	
	int (*init)(const struct device *dev,u8_t band, u8_t level, u16_t freq);
	int (*standy)(const struct device *dev);
	int (*set_freq)(const struct device *dev, uint16_t freq);
	int (*set_band)(const struct device *dev, fm_band_e band);
	int (*set_threshold)(const struct device *dev, uint8_t level);
	int (*set_volume)(const struct device *dev, uint8_t volume);
	int (*set_mute)(const struct device *dev, bool enable);
	int (*get_status)(const struct device *dev, fm_status_t *status);
	int (*soft_seek)(const struct device *dev, uint16_t freq, fm_seekdir_e direction);
	int (*hard_seek)(const struct device *dev, uint16_t freq, fm_seekdir_e direction);
	int (*break_seek)(const struct device *dev);
	int (*get_seek_status)(const struct device *dev,fm_seek_status_e *seek_status);
};

#endif /* __FM_H__ */
