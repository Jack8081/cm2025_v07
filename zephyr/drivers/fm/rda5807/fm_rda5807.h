
#ifndef _FM_RDA5807_H
#define _FM_RDA5807_H

#define FM_I2C_DEV0_ADDRESS 	(0x21 >> 1)
#define FM_I2C_DEV1_ADDRESS     (0x20 >> 1)




/*FM模组初始化*/
int fm_rda5807_init(const struct device *dev,u8_t band, u8_t level, u16_t freq);

/*FM模组进入standby*/
int fm_rda5807_standy(const struct device *dev);

/*设置频点开始播放*/
int fm_rda5807_set_freq(const struct device *dev, uint16_t freq);

/* 设置电台频段*/
int fm_rda5807_set_band(const struct device *dev, fm_band_e band);

/* 设置搜台门限*/
int fm_rda5807_set_threshold(const struct device *dev, uint8_t level);

/* 设置音量*/
int fm_rda5807_set_volume(const struct device *dev, uint8_t volume);

/*静音与解除静音*/
int fm_rda5807_set_mute(const struct device *dev, bool enable);

/*获取当前状态信息*/
int fm_rda5807_get_status(const struct device *dev, fm_status_t *status);

/*开启软件搜台过程*/
int fm_rda5807_soft_seek(const struct device *dev, uint16_t freq, fm_seekdir_e direction);

/*开启硬件搜台过程*/
int fm_rda5807_hard_seek(const struct device *dev, uint16_t freq, fm_seekdir_e direction);

/* 手动退出seek 过程,如果当前是hard seek，就退出hard seek，如果当前是soft seek，就退出soft seek*/
int fm_rda5807_break_seek(const struct device *dev);

/* 获取获取当前fm的seek状态，如果当前是hard seek，就返回hard seek的状态，如果当前是soft seek，就返回soft seek*/
int fm_rda5807_get_seek_status(const struct device *dev,fm_seek_status_e *seek_status);




/*************register address table**************/
#define  RDA_REGISTER_00     0x00
#define  RDA_REGISTER_01     0x01
#define  RDA_REGISTER_02     0x02
#define  RDA_REGISTER_03     0x03
#define  RDA_REGISTER_04     0x04
#define  RDA_REGISTER_05     0x05
#define  RDA_REGISTER_06     0x06
#define  RDA_REGISTER_07     0x07
#define  RDA_REGISTER_08     0x08
#define  RDA_REGISTER_09     0x09
#define  RDA_REGISTER_0a     0x0a
#define  RDA_REGISTER_0b     0x0b
#define  RDA_REGISTER_0c     0x0c
#define  RDA_REGISTER_0d     0x0d
#define  RDA_REGISTER_0e     0x0e
#define  RDA_REGISTER_0f     0x0f
#define  RDA_REGISTER_10     0x10
#define  RDA_REGISTER_11     0x11
#define  RDA_REGISTER_12     0x12
#define  RDA_REGISTER_13     0x13
#define  RDA_REGISTER_14     0x14
#define  RDA_REGISTER_15     0x15
#define  RDA_REGISTER_16     0x16
#define  RDA_REGISTER_17     0x17
#define  RDA_REGISTER_18     0x18
#define  RDA_REGISTER_19     0x19
#define  RDA_REGISTER_1a     0x1a
#define  RDA_REGISTER_1b     0x1b
#define  RDA_REGISTER_1c     0x1c

#endif

