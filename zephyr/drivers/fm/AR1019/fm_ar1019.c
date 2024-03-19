/*
 *fm module rda5820
 *
 *
 *
 */

#include <zephyr.h>
//#include <init.h>
#include <board.h>
#include <drivers/i2c.h>
#include <device.h>
#include <string.h>
//#include <misc/printk.h>
#include <drivers/fm.h>
#include <stdlib.h>
//#include <logging/sys_log.h>
#include "fm_ar1019.h"
#include <os_common_api.h>
#include <board_cfg.h>

#ifdef CONFIG_NVRAM_CONFIG
#include "drivers/nvram_config.h"
#endif



typedef struct {
	u8_t dev_addr;
	u8_t reg_addr;
} i2c_addr_t;



/* CHAN<9:0>*/
uint16_t Freq_RF;
/* 电台信号强度*/
uint16_t signal_stg;
/* 当前波段信息*/
uint8_t Set_Band;
/* 是否采用50K 步长*/
uint8_t Space50k;
/* 电台RSSI */
uint8_t RSSI2;
/* 电台RSSI */
uint8_t RSSI1;
/* 驱动层:   电台立体声信息  0~~立体声   1~~单声道*/
uint8_t ST_info;
/* 硬件seek 状态标识*/
uint8_t hardseek_flag;



extern int ar1019_rx_init(void);
int ar1019_set_mute(bool flag);


#if 1
const uint8_t ar1019_init_reg[36] = //32
{	
	0xFF, 0x7B, 0x5B, 0x11, 0xD4, 0xB9, 0xA0, 0x10, 0x07, 0x80, 0x28, 0xAB, 
	0x64, 0x00, 0x1e, 0xe7, 0x71, 0x41, 0x00, 0x80, 0x81, 0xc6, 0x4F, 0x55, 
	0x97, 0x0c, 0xb8, 0x45, 0xfc, 0x2d, 0x80, 0x97, 0x04, 0xE1, 0xDF, 0x6A
};
#else
const uint8_t ar1019_init_reg[36]=	//32.768
{ 
	0xFF, 0x7B, 0x5B, 0x11, 0xD4, 0xB9, 0xA0, 0x10, 0x07, 0x80, 0x28, 0xAB, 
	0x64, 0x00, 0x1e, 0xe7, 0x71, 0x41, 0x00, 0x7d, 0x82, 0xc6, 0x4F, 0x55, 
	0x97, 0x0c, 0xb8, 0x45, 0xfc, 0x2d, 0x80, 0x97, 0x04, 0xA1, 0xDF, 0x6A
};
const uint8_t ar1019_init_reg[36] =	//24M
{ 	
	0xFF, 0x7B, 0x5B, 0x11, 0xD4, 0xB9, 0xA0, 0x10, 0x07, 0x80, 0x28, 0xAB, 
	0x64, 0x00, 0x1f, 0x87, 0x71, 0x41, 0x00, 0x80, 0x81, 0xc6, 0x4F, 0x55, 
	0x97, 0x0c, 0xb8, 0x45, 0xfc, 0x2d, 0x80, 0x97, 0x04, 0xE1, 0xDF, 0x6A
};

const uint8_t ar1019_init_reg[36] =	//32.768
{ 	
	0xFF, 0x7B, 0x5B, 0x11, 0xD4, 0xB9, 0xA0, 0x10, 0x07, 0x80, 0x28, 0xAB, 
	0x64, 0x00, 0x1e, 0xe7, 0x71, 0x41, 0x00, 0x7d, 0x81, 0xc6, 0x4F, 0x55, 
	0x97, 0x0c, 0xb8, 0x45, 0xfc, 0x2d, 0x80, 0x97, 0x04, 0xE1, 0xDF, 0x6A
};
const uint8_t ar1019_init_reg[36] =	//12M
{ 	0xFF, 0x7B, 0x5B, 0x11, 0xD4, 0xB9, 0xA0, 0x10, 0x07, 0x80, 0x28, 0xAB, 
	0x64, 0x00, 0x1f, 0x07, 0x71, 0x41, 0x00, 0x80, 0x81, 0xc6, 0x4F, 0x55, 
	0x97, 0x0c, 0xb8, 0x45, 0xfc, 0x2d, 0x80, 0x97, 0x04, 0xE1, 0xDF, 0x6A
};
#endif

uint8_t ar1019_write_reg[36] = {0};

void ar_delay(u16_t ms)
{
	k_timeout_t time;

	time.ticks = ms;
	k_sleep(time);
}

u32_t i2c_write_dev(void *dev, i2c_addr_t *addr, u8_t *data, u8_t num_bytes)
{
	u8_t wr_addr[2];
	u32_t ret = 0;
	struct i2c_msg msgs[2];
	u8_t len;

	if (dev == NULL) {
		ret = -1;
		SYS_LOG_ERR("%d: i2c_gpio dev is not found\n", __LINE__);
		return ret;
	}

	if (addr->reg_addr == 0xff){
		msgs[0].buf = data;
		msgs[0].len = num_bytes;
		msgs[0].flags = I2C_MSG_WRITE;
		len = 1;
	} else{
		wr_addr[0] = addr->reg_addr;

		msgs[0].buf = wr_addr;
		msgs[0].len = 1;
		msgs[0].flags = I2C_MSG_WRITE;

		msgs[1].buf = data;
		msgs[1].len = num_bytes;
		msgs[1].flags = I2C_MSG_WRITE;

		len = 2;
	}
	
	return i2c_transfer(dev, &msgs[0], len, addr->dev_addr);
}

u32_t i2c_read_dev(void *dev, i2c_addr_t *addr, u8_t *data, u8_t num_bytes)
{
	u8_t wr_addr[2];
	u32_t ret = 0;
	struct i2c_msg msgs[2];
	u8_t len;

	if (dev == NULL) {
		ret = -1;
		SYS_LOG_ERR("%d: i2c_gpio dev is not found\n", __LINE__);
		return ret;
	}

	if (addr->reg_addr == 0xff){
		msgs[0].buf = data;
		msgs[0].len = num_bytes;
		msgs[0].flags = I2C_MSG_READ;	
		len = 1;	
	} else{
		wr_addr[0] = addr->reg_addr;

		msgs[0].buf = wr_addr;
		msgs[0].len = 1;
		msgs[0].flags = I2C_MSG_WRITE;

		msgs[1].buf = data;
		msgs[1].len = num_bytes;
		msgs[1].flags = I2C_MSG_READ | I2C_MSG_RESTART;
		len = 2;
	}
	return i2c_transfer(dev, &msgs[0], len, addr->dev_addr);
}

u8_t ar_WriteReg(u8_t Regis_Addr, u16_t Data)
{
	u8_t i;
	int result = 0;
	u8_t writebuffer[2];
	i2c_addr_t addr;
	const struct device *i2c = device_get_binding(CONFIG_FM_I2C_NAME);

	addr.reg_addr = Regis_Addr;
	addr.dev_addr = FM_I2C_DEV0_ADDRESS;
	writebuffer[1] = Data;
	writebuffer[0] = Data >> 8;

	for (i = 0; i < 10; i++) {
		if (i2c_write_dev((void *)i2c, &addr, writebuffer, 2) == 0) {
			break;
		}
	}
	if (i == 10) {
		SYS_LOG_INF("FM AR write failed\n");
		result = -1;
	}

	return result;
}

u16_t ar_ReadReg(u8_t Regis_Addr)
{
	u8_t Data[2] = {0};
	u8_t init_cnt = 0;
	u8_t ret = 0;
	u16_t tmp = 0;
	const struct device *i2c = device_get_binding(CONFIG_FM_I2C_NAME);

	i2c_addr_t addr;


	addr.reg_addr = Regis_Addr;
	addr.dev_addr = FM_I2C_DEV0_ADDRESS;

	for (init_cnt = 0; init_cnt < 10; init_cnt++) {
		ret = i2c_read_dev((void *)i2c, &addr, Data, 2);
		if (ret == 0) {
			break;
		}
	}

	if (init_cnt == 10) {
		Data[0] = 0xff;
		Data[1] = 0xff;
	}

	tmp = Data[0] << 8;
	tmp |= Data[1];

	return tmp;
}


u8_t ar_Write_Serial_Reg(int reg_addr, u8_t *buf, u8_t write_len)
{
	u8_t i;
	int result = 0;
	i2c_addr_t addr;
	const struct device *i2c = device_get_binding(CONFIG_FM_I2C_NAME);

	addr.reg_addr = reg_addr;
	addr.dev_addr = FM_I2C_DEV1_ADDRESS;

	for (i = 0; i < 10; i++) {
		if (i2c_write_dev((void *)i2c, &addr, buf, write_len) == 0) {
			break;
		}
	}
	if (i == 10) {
		SYS_LOG_INF("FM AR write failed\n");
		result = -1;
	}

	return result;
}

/*
 ********************************************************************************
 *             uint8 WaitSTC(void)
 *
 * Description : wait STC flag,tune or seek complete
 *
 * Arguments   :
 *
 * Returns     :  若STC标志置1，则返回1，否则，返回0
 *
 * Notes       :
 *
 ********************************************************************************
 */

int WaitSTC(void)
{
    uint16_t reg_val;
    uint8_t temp;
    uint8_t count = 0;

    ar_delay(50); //延时50ms，等待TUNE OR SEEK 结束
    do
    {
        reg_val = ar_ReadReg(0x13); // read status reg
        temp = (uint8_t)(reg_val |  0xdf); //STC
        if (temp == 0xff)
        {
            return 0; //STC =1
        }
        count++;
        ar_delay(10);
    } while (/*(temp != (uint8)0xff) && (count < 255)*/count < 255);
	
	printk("%s: error read REG[0x13] = 0x%x\n", __func__, reg_val);
	
    return -1;
}



/*
 ********************************************************************************
 *           uint8 TuneControl(uint8 mode)
 *
 * Description : tune 控制
 *
 * Arguments :   mode   0: clear tune;   1: start tune
 *
 * Returns :  成功:  返回1；失败:  返回0
 *
 * Notes :
 *
 ********************************************************************************
 */
uint8_t TuneControl(uint8_t mode)
{
	uint16_t value;
	uint8_t ret;

    if (mode == 0)
    {
        ar1019_write_reg[4] &= ~(0x01 << 1); //clear tune
    }
    else
    {
        ar1019_write_reg[4] |= (0x01 << 1); //start tune
    }

	value = (ar1019_write_reg[4]<<8 | ar1019_write_reg[5]);
	ret = ar_WriteReg(0x02, value);
	return ret;
}


/*
 ****************************************************************************************
 *           uint16 FreqToChan(uint16 freq, uint8 space)
 *
 * Description : 将频点转换成写寄存器的值
 *
 * Arguments :  freq:  需进行转换的频率( 实际频率khz 为单位的低2 字节)
 *			   space:  步进   0: 100K    1: 50K
 * Returns :	   CHAN<9:0>
 *
 * Notes :  各个频率值( 以1khz 为单位)  的最高bit  恒为1
 *
 * space 50K    CHAN<9:0> = FREQ ( 以50K 为单位)  -  69*20
 * else             CHAN<9:1> = FREQ ( 以100K 为单位) - 69*10
 *****************************************************************************************
 */
uint16_t FreqToChan(uint16_t freq, uint8_t space)
{
    uint16_t Chan, tmp_freq;
    uint32_t temp;

    //实际频率值，以khz 为单位
    temp = (uint32_t) freq + 0x10000;

    //步进100K
    if (space == 0)
    {
        tmp_freq = (uint16_t)(temp / 100);
        //CHAN<9:1>
        Chan = tmp_freq - 69* 10 ;
        //CHAN<9:0>
        Chan = Chan*2;
    }
    //步进50K
    else
    {
        tmp_freq = (uint16_t)(temp/50);
        //CHAN<9:0>
        Chan = tmp_freq - 69*20;
    }

    return Chan;
}

/*
 ********************************************************************************
 *           uint16 ChanToFreq(uint16 chan, uint8 space)
 *
 * Description : 将读出的寄存器频率值转换成上层需要的频率
 *
 * Arguments :  chan:读出的频率值 CHAN<9:0>
 *                    space :  步进
 *
 * Returns :	   转换出来的实际频点值(以1KHz 为单位的低2   字节)
 *
 * Notes :
 *
 ********************************************************************************
 */
u16_t ChanToFreq(u16_t chan, u8_t space)
{
    u16_t Freq;
    u32_t temp;

    //步进100K  ，FREQ ( 以100K 为单位) = CHAN<9:1> +  69*10
    if (space == 0)
    {
        Freq = chan / 2 + 69* 10 ;
        //转换为以1khz  为单位
        temp = (u32_t)Freq * 100;
    }
    //步进50K ， FREQ ( 以50K 为单位) = CHAN<9:0> +  69*20

    else
    {
        Freq = chan + 69*20;
        //转换为以1khz  为单位
        temp = (u32_t)Freq * 50;
    }

    //最高位恒为1
    Freq = (u16_t)(temp & 0x0ffff);
    return Freq;
}





/*int ar1019_check_freq(struct fm_rx_device *dev, u16_t freq)
{
	u8_t value = 0;
	u16_t tmp;

	ar1019_set_mute(1);

	ar1019_set_freq(freq);
	ar_delay(100);

	tmp = ar_ReadReg(RDA_REGISTER_0b);

	value = (tmp >> 8) & 0x01;
	if (value == 1) {
		SYS_LOG_INF("freq : %d is value", freq);
	} else {
		SYS_LOG_INF("freq : %d is not value", freq);
	}

	return !value;
 }*/

int ar1019_set_mute(bool flag)
{
    int result;
	uint16_t value;
	 /**1: mute, 0: unmute**/
	if (flag)
        ar1019_write_reg[3] |= (0x01 << 1); //enable mute
	else
        ar1019_write_reg[3] &= ~(0x01 << 1); //release mute

	value = ar1019_write_reg[0x01*2]<<8 | ar1019_write_reg[0x01*2+1];
	result = ar_WriteReg(0x01, value);
	printk("ar1019_set_mute: read R01: 0x%x\n", ar_ReadReg(0x01));
    return result;
}












/*
 ********************************************************************************
 *             int  sFM_GetHardSeekflag(void* flag, void* null2, void* null3)
 *
 * Description : 获取硬件seek 过程是否结束标志
 *
 * Arguments : 保存结束标志的指针，1  硬件seek 结束
 *
 * Returns : 若获取成功，返回1;否则,返回0
 *
 * Notes :   flag :  bit0  此轮硬件seek 是否结束    1:  已结束   0:  未结束
 *                       bit1  此轮硬件seek 找到的是否有效台   1:  有效台
 ********************************************************************************
 */
int fm_ar1019_get_seek_status(const struct device *dev,fm_seek_status_e *seek_status)
{
    int result;
    u8_t* stc_flag;
	u16_t value = 0;
	printk("%s: fm_ar1019_get_seek_status %d\n", __FUNCTION__, __LINE__);
    stc_flag = (u8_t*) seek_status;
    value = ar_ReadReg(0x13);
	
    //STC Flag
    hardseek_flag = (value & 0x20) >> 5;
    //seek 过程结束
    if (hardseek_flag == 0x01)
    {
    	printk("Hard seek over, R13: 0x%x\n", value);
        *stc_flag = 0x01;
        if ((value & 0x10) == 0) //SF = 0
        {
            //seek 到有效台
            *stc_flag |= (0x01 << 1);
        }
    }
    //seek 过程未结束
    else
    {
        *stc_flag = 0;
    }
    result = 1;
		
    return result;
}


/*
 ********************************************************************************
 *             int  sFM_BreakSeek(void* null1, void* null2, void* null3);
 *
 * Description : 硬件seek 过程中，手动停止seek 过程
 *
 * Arguments :
 *
 * Returns : 成功停止，返回1;否则,返回0
 *
 * Notes :
 *
 ********************************************************************************
 */
int fm_ar1019_break_seek(const struct device *dev)
{
    int result;
	u16_t value = 0;

	value = ar_ReadReg(0x13);
    if (value != 0xffff)
    {
        //STC Flag
		hardseek_flag = (value & 0x20) >> 5;
		
        //仍处于硬件seek 过程中
        if (hardseek_flag == 0)
        {
            ar1019_write_reg[6] &= (u8_t) 0xbf; //stop seek
            value = ar1019_write_reg[6]<<8 | ar1019_write_reg[7];
			result = ar_WriteReg(0x03, value);
            if (result == 0)
            {
                hardseek_flag = 1;
                result = 1;
            }
            else
            {
                result = 0;
            }
        }
        //已经不处于硬件seek 过程，直接返回停止成功
        else
        {
            result = 1;
        }
    }
    else
    {
        result = 0;
    }
    return result;
}



/*
 ********************************************************************************
 *            int sFM_HardSeek(uint16 freq, uint8 direct, void* null3)
 *
 * Description : 启动FM 硬件搜台过程
 *
 * Arguments :
 param1: 硬件搜台方式，搜台起始频点
 param2: 搜索方向
 * Returns :  成功启动硬件搜台，返回1；否则返回0
 *
 * Notes :  搜台方向说明
 * bit0   ~~~  向上或向下搜台。0:   UP;    1: DOWN
 * bit1   ~~~  是否边界回绕。     0:   回绕;    1:  不回绕
 ********************************************************************************
 */

int fm_ar1019_hard_seek(const struct device *dev, uint16_t freq, fm_seekdir_e direction)
{
	int result;
	u8_t temp;
	u16_t TempFreq;
	u8_t chan_bit0;
	u16_t freq_tmp;
	u16_t value;

	printk("### freq = %d, dir = %d\n", freq, direction);
	
	//默认硬件seek	尚未启动
	hardseek_flag = 1;

	ar1019_set_mute(1);
	ar_delay(100);

	//tune 到起始频点
	TempFreq = FreqToChan(freq, Space50k); //将电台频率转换成寄存器需写入的值
	chan_bit0 = (u8_t)(TempFreq % 2);
	freq_tmp = (TempFreq >> (u8_t) 1); // CHAN<9:1>

	ar1019_write_reg[4] &= (u8_t) 0xf4; //disable tune
	ar1019_write_reg[4] |= (chan_bit0 << 3);
	ar1019_write_reg[4] |= (u8_t)(freq_tmp >> 8);
	ar1019_write_reg[5] = (u8_t)(freq_tmp % 256); //update chan
	value = ar1019_write_reg[4]<<8 | ar1019_write_reg[5];
	result = ar_WriteReg(0x02, value);
	ar_delay(1);

	ar1019_write_reg[6] &= (u8_t) ~(0x01 << 6); //disable seek
	value = ar1019_write_reg[6]<<8 | ar1019_write_reg[7];
	result = ar_WriteReg(0x03, value);
	ar_delay(1);
	if (result == 0)
	{
		temp = direction & 0x01;
		//seek down
		if (temp != 0)
		{
			//R3--bit15
			ar1019_write_reg[6] &= (u8_t) 0x7f;
		}
		//seek up
		else
		{
			ar1019_write_reg[6] |= (u8_t) 0x80;
		}

		temp = direction & 0x02;
		//不回绕
		if (temp != 0)
		{
			//R10--bit3
			ar1019_write_reg[21] &= (u8_t) 0xf7;
		}
		//回绕
		else
		{
			ar1019_write_reg[21] |= (u8_t) 0x08;
		}
		ar1019_write_reg[1] |= 0x01;
		ar1019_write_reg[6] |= 0x40; //start seek
		result = ar_Write_Serial_Reg(0x01, ar1019_write_reg +2, 20); //R1~R10
		ar_delay(1);
		if (result == 0)
		{
			for(int i=0; i<=0x1C; i++)
				printk("reg[%d]: 0x%x\n", i, ar_ReadReg(i));
			//硬件seek 已启动，未完成
			hardseek_flag = 0;
		}
	}

	ar_delay(1);
	result = WaitSTC();
	ar1019_set_mute(0);

	return result;
}



/*
 ********************************************************************************
 *             uint8 CheckStation(void)
 *
 * Description : 真台判断
 *
 * Arguments   :
 *
 * Returns     :  若搜到的电台为真台,返回1;否则,返回0
 *
 * Notes       :
 *
 ********************************************************************************
 */
uint8_t CheckStation(void)
{
//    uint8_t result;
    uint8_t temp;
    uint8_t tmp;
	uint16_t reg_val;
	uint8_t ReadBuffer[2];

    reg_val = WaitSTC();
    ReadBuffer[0] = (uint8_t)(reg_val >> 8);
	ReadBuffer[1] = (uint8_t)(reg_val % 256);
	if(reg_val != 0xffff)
    {
        temp = ReadBuffer[1] & 0x10; // 判断的是SF  这一位
        if (temp == 0)
        {
            Freq_RF = (uint16_t) ReadBuffer[0] * 4;
            tmp = (ReadBuffer[1] & (uint8_t) 0x80) >> 6;
            tmp += (ReadBuffer[1] & 0x01);
            //CHAN<9:0>
            Freq_RF += tmp;
            return 1;
        }
    }
    return 0;
}

/*
 ********************************************************************************
 * int fm_soft_seek(const struct device *dev, uint16_t freq, fm_seekdir_e direction)
 *
 * Description : FM  软件搜台，tune 单频点，判断是否有效电台
 *
 * Arguments :  软件搜台方式,需设置的频率值及搜索方向
 *
 * Returns : 电台为真台,则返回1;否则,返回0
 *
 * Notes :
 *
 ********************************************************************************
 */

int fm_ar1019_soft_seek(const struct device *dev, uint16_t freq, fm_seekdir_e direction)
{
	int result;
//    u8_t temp;
    u16_t TempFreq;
    u8_t chan_bit0;
    u16_t freq_tmp;
	u16_t value;
	//切换到single_seek 模式
	ar1019_write_reg[28] |= 0x02;
	value = ar1019_write_reg[0x0e*2]<<8 | ar1019_write_reg[0x0e*2+1];
	result = ar_WriteReg(0x0e, value);
	ar_delay(1);


	ar1019_set_mute(1);
    ar_delay(200);

	if(direction == FM_SEEKDIR_DOWN)
	{
		//seek down
		if(Space50k == 0x01)
		{
			freq_tmp = freq +50;
		}
		else
		{
			freq_tmp = freq +100;
		}
	}
	else
	{
		//seek up
		if(Space50k == 0x01)
		{
			freq_tmp = freq -50;
		}
		else
		{
			freq_tmp = freq -100;
		}
	}
	
	//tune 到起始频点
    TempFreq = FreqToChan(freq_tmp, Space50k); //将电台频率转换成寄存器需写入的值
    chan_bit0 = (u8_t)(TempFreq % 2);
    freq_tmp = (TempFreq >> (u8_t) 1); // CHAN<9:1>

    ar1019_write_reg[4] &= (u8_t) 0xf4; //disable tune
    ar1019_write_reg[4] |= (chan_bit0 << 3);
    ar1019_write_reg[4] |= (u8_t)(freq_tmp >> 8);
    ar1019_write_reg[5] = (u8_t)(freq_tmp % 256); //update chan
	value = ar1019_write_reg[4]<<8 | ar1019_write_reg[5];
	result = ar_WriteReg(0x02, value);
	ar_delay(1);

	ar1019_write_reg[6] &= (u8_t) ~(0x01 << 6); //disable seek
	value = ar1019_write_reg[6]<<8 | ar1019_write_reg[7];
	result = ar_WriteReg(0x03, value);
	ar_delay(1);

	ar1019_write_reg[6] &= 0x7f; //设置SEEK direct
	ar1019_write_reg[6] |= (u8_t)(direction<<7); //设置SEEK direct
	ar1019_write_reg[6] |= (u8_t)(0x01<<6); //enable SEEK
	value = ar1019_write_reg[6]<<8 | ar1019_write_reg[7];
	result = ar_WriteReg(0x03, value);
	ar_delay(1);

	//已经启动single_seek
	if (result == 0)
	{
		//软件搜台，判断所设置频点是否有效电台
		if (CheckStation() == 1) // check station

		{
			//0x17700~~96MHz, 过滤96MHz 频点
			if( freq !=0x7700)
			{
				result = 1;
			}
			else
			{
				result = 0;
			}
		}
		else
		{
			result = 0;
		}

	}
	else
	{
		result = 0;
	}

	//切换回正常模式	
	ar1019_write_reg[28] &= (uint8_t)0xfd;
	value = ar1019_write_reg[0x0e*2]<<8 | ar1019_write_reg[0x0e*2+1];
	result = ar_WriteReg(0x0e, value);
	ar_delay(1);
	
	return result;
}



/*
 ********************************************************************************
 * int fm_get_status(const struct device *dev, fm_status_t *status)
 *
 * Description : 获取当前电台的相关信息，包括当前电台频率
 *                   信号强度值，立体声状态，当前波段
 *
 * Arguments : pstruct_buf   保存电台信息的结构体指针
 *                   mode==0,  正常播放( 非seek 过程中)  取信息
 *                   mode==1,  在硬件seek 过程中取信息
 * Returns : //是否读取状态成功,如果读取成功,则返回值为1
 * 否则,返回0
 * Notes :
 *
 ********************************************************************************
 */
int fm_ar1019_get_status(const struct device *dev, fm_status_t *status)
{
//当前状态 播放还是暂停状态 天线 此函数还未实现
	int result;
	uint16_t reg_val = 0;
	uint8_t ReadBuffer[2];

	fm_status_t * ptr_status = (fm_status_t *)status;
	uint8_t tmp;

	//硬件seek 过程中，直接取寄存器信息
	if(hardseek_flag == 1)
	{
		//read status reg
		reg_val = ar_ReadReg(0x13); // read status reg
	
	}
	//非硬件seek 过程中，需等待STC
	else
	{
		reg_val = WaitSTC();
	}
	ReadBuffer[0] = (uint8_t)(reg_val >> 8);
	ReadBuffer[1] = (uint8_t)(reg_val % 256);
	if(reg_val != 0xffff)
	{
		Freq_RF = (uint16_t)ReadBuffer[0] * 4;
		tmp = (uint8_t)((ReadBuffer[1] & 0x80)>>6);
		tmp +=(ReadBuffer[1] & 0x01);
		//CHAN<9:0>
		Freq_RF += tmp;
		ptr_status->freq = ChanToFreq(Freq_RF, Space50k);

		tmp = ReadBuffer[1] & 0x08;
		if(tmp==0)
		{
			//mono
			ST_info = 1;
		}
		else
		{
			//stereo
			ST_info = 0;
		}
		ptr_status->channel = (fm_channel_e)ST_info;
		
		//硬件搜台 暂不获取
		//STC Flag
       /* hardseek_flag = (ReadBuffer[1] & 0x20)>>5;
        if(mode!=1)
        {
            hardseek_flag = 1;
        }
        if(hardseek_flag==0)
        {
            ptr_buf->STC_flag = HARDSEEK_DOING;
        }
        else
        {
            ptr_buf->STC_flag = HARDSEEK_COMPLETE;
        }*/

		if(Set_Band == 0x10)
		{
			ptr_status->band = FM_BAND_JAPAN;
		}
		else
		{
			if(Space50k == 0x01)
			{
				ptr_status->band = FM_BAND_EUROPE;
			}
			else
			{
				ptr_status->band = FM_BAND_CHINA_US;
			}
		}


		reg_val = ar_ReadReg(0x12); // read status reg
		ReadBuffer[0] = (uint8_t)(reg_val >> 8);
		signal_stg = (uint16_t)((ReadBuffer[0] & 0xfe)>>1);
		ptr_status->intensity = signal_stg;
		result =1;
	}
	else
	{
		result = 0;
	}

	return result;
}



/*
 ************************************************* *******************************
 * int fm_set_mute(const struct device *dev, bool enable)
 *
 * Description : FM静音设置
 *
 * Arguments : 是否静音,0为取消静音,1为静音
 *
 * Returns : 若设置静音控制成功,则返回1,否则,返回0
 *
 * Notes :
 *
 ********************************************************************************
 */

int fm_ar1019_set_mute(const struct device *dev, bool enable)
{
    int result;
	uint16_t value;
	
	 /**1: mute, 0: unmute**/
	
	if(enable)
        ar1019_write_reg[3] |= (0x01 << 1); //enable mute
	else
        ar1019_write_reg[3] &= ~(0x01 << 1); //release mute

	value = ar1019_write_reg[0x01*2]<<8 | ar1019_write_reg[0x01*2+1];
	result = ar_WriteReg(0x01, value);
	printk("ar1019_set_mute: read R01: 0x%x\n", ar_ReadReg(0x01));
    return result;
}

/*
 ************************************************* *******************************
 * int  sFM_SetVolume(uint8 value, void* null2, void* null3)
 *
 * Description : FM  音量设置
 *
 * Arguments : value---音量值 0~15
 *
 * Returns : 设置成功，返回1;  否则，返回0
 *
 * Notes :
 *
 ********************************************************************************
 */

int fm_ar1019_set_volume(const struct device *dev, uint8_t volume)
{
	int result = -1;
	uint16_t reg_val;
	uint16_t value;

	if(volume > 0x0F)
		volume = 0x0F;

	/* 先读出R6->volreg_sw位，决定设置哪个VOLUME */
	reg_val = ar_ReadReg(0x06);
	if(reg_val != 0xffff)
	{
		if(reg_val & 0x8000) // volreg_sw=1, in R6 (VOLUME[4:7], VOLUME2[0:3])
		{
			ar1019_write_reg[13] = 0;
			if(volume == 0)	// VOLUME
			{
				ar1019_write_reg[13] |= (0x0F<<4);
			}
			ar1019_write_reg[13] |= (volume<<0);	// VOLUME2
			value = ar1019_write_reg[0x06*2]<<8 | ar1019_write_reg[0x06*2+1];
			result = ar_WriteReg(0x06, value);
		}
		else	// volreg_sw=0, in R3(VOLUME[7:10]) & R14(VOLUME2[12:15])
		{
			if(volume == 0)
			{
				ar1019_write_reg[6] |= (0x07<<0);	// VOLUME
				ar1019_write_reg[7] |= (0x01<<7);	// VOLUME
			}
			else
			{
				ar1019_write_reg[6] &= (~(0x07<<0)); // VOLUME
				ar1019_write_reg[7] &= (~(0x01<<7)); // VOLUME
			}
			ar1019_write_reg[28] &= (~(0x0F<<4));
			ar1019_write_reg[28] |= (volume<<4);


			value = ar1019_write_reg[0x03*2]<<8 | ar1019_write_reg[0x03*2+1];
			result = ar_WriteReg(0x03, value);

			value = ar1019_write_reg[0x0e*2]<<8 | ar1019_write_reg[0x0e*2+1];
			result = ar_WriteReg(0x0e, value);
		}
	}

	return result;
}



/*
 ************************************************* *******************************
 * int fm_set_threshold(const struct device *dev, uint8_t level)
 *
 * Description : FM  搜台门限设置
 *
 * Arguments : level---搜台门限值
 *
 * Returns : 设置成功，返回1;  否则，返回0
 *
 * Notes :
 *
 ********************************************************************************
 */

int fm_ar1019_set_threshold(const struct device *dev, uint8_t level)
{
	int result;
	uint16_t value;
	
	ar1019_write_reg[7] &= (uint8_t) 0x80;
	ar1019_write_reg[7] |= level; //搜台门限
	value = ar1019_write_reg[0x03*2]<<8 | ar1019_write_reg[0x03*2+1];
	result = ar_WriteReg(0x03, value);
	
	return result;
}



/*
 ************************************************* *******************************
 * int fm_set_band(const struct device *dev, fm_band_e band)
 *
 * Description : FM  波段设置
 *
 * Arguments : band    0:  中国/ 美国波段 1: 日本波段  2: 欧洲波段
 *
 * Returns : 设置成功，返回1;  否则，返回0
 *
 * Notes :
 *
 ********************************************************************************
 */

int fm_ar1019_set_band(const struct device *dev, fm_band_e band)
{
	int result;
	uint8_t space_bak;
	uint16_t reg_val;

	space_bak = Space50k;
	Space50k = 0;

	if (band == FM_BAND_JAPAN)
	{
		Set_Band = 0x10;
	}
	else
	{
		Set_Band = 0;
		if (band == FM_BAND_EUROPE)
		{
			Space50k = 1;
		}
	}

	ar1019_write_reg[6] &= (uint8_t) 0xe7;
	ar1019_write_reg[6] |= Set_Band; //设置频段
	ar1019_write_reg[30] &= (uint8_t) 0xfe;
	ar1019_write_reg[30] |= Space50k; //设置步进
	
	reg_val = (ar1019_write_reg[6]<<8 | ar1019_write_reg[7]);
	result = ar_WriteReg(0x03, reg_val);

	reg_val = (ar1019_write_reg[30]<<8 | ar1019_write_reg[31]);
	result = ar_WriteReg(0x0f, reg_val);
	if (result == 0)
	{
		//设置频段失败，恢复space
		Space50k = space_bak;
	}
	return result;
}


/*
 ********************************************************************************
 *            int fm_set_freq(const struct device *dev, uint16_t freq)
 *
 * Description : 频点设置
 *
 * Arguments   :  需设置的频率值 CHAN <9:0>
 *
 * Returns     :   若频点设置成功，返回1;否则,返回0
 *
 * Notes       :
 *
 ********************************************************************************
 */
int fm_ar1019_set_freq(const struct device *dev, uint16_t freq)
{
	uint8_t result;
	uint8_t chan_bit0;
	uint16_t reg_val;
	uint16_t freq_tmp;

	printk("%s: enter ++, freq: %d\n", __FUNCTION__, freq);

	//ar_Write_Serial_Reg(0x07, ar1019_write_reg+0x07*2, 0x06);

	//ar1019_set_mute(1);
	//ar_delay(200);

	freq_tmp = FreqToChan(freq, Space50k); //将电台频率转换成寄存器需写入的值

	chan_bit0 = (uint8_t)(freq_tmp % 2);
	freq_tmp = (freq_tmp >> 1); // CHAN<9:1>
	ar1019_write_reg[4] &= (uint8_t) 0xfd; //clear tune
	ar1019_write_reg[6] &= (uint8_t) ~(0x01 << 6); //clear SEEK
	result = ar_Write_Serial_Reg(0x02, ar1019_write_reg+4, 0x04);
	ar_delay(1);
#if 1
	ar1019_write_reg[22] &= 0x7f; //Read Low-side LO Injection
	ar1019_write_reg[23] &= (uint8_t) 0xfa;
	reg_val = (ar1019_write_reg[22]<<8 | ar1019_write_reg[23]);
	result = ar_WriteReg(0x0b, reg_val);
	//result = ar_Write_Serial_Reg(0x0b, ar1019_write_reg+22, 0x02);
	ar_delay(1);
#endif
	//set band/space/chan,enable tune
	ar1019_write_reg[4] &= (uint8_t) 0xf6;
	ar1019_write_reg[4] |= (chan_bit0 << 3);
	ar1019_write_reg[4] |= (uint8_t)(freq_tmp >> 8);
	ar1019_write_reg[5] = (uint8_t)(freq_tmp % 256);
	result = TuneControl(1); //start tune
	ar_delay(1);

	result = WaitSTC(); //wait STC flag
	if(result != 0)
		printk("%s: ar1019_wait_stc error at line %d\n", __FUNCTION__, __LINE__);
#if 1
	reg_val = ar_ReadReg(0x12); //get RSSI (RSSI1)
	RSSI1 = (reg_val >> 9);
	printk("reg_val=0x%x, RSSI1=%d\n", reg_val, RSSI1);

	//clear tune
	result = TuneControl(0);
	ar_delay(1);

	ar1019_write_reg[22] |= (uint8_t) 0x80; //Read High-side LO Injection
	ar1019_write_reg[23] |= 0x05;
	reg_val = (ar1019_write_reg[22]<<8 | ar1019_write_reg[23]);
	result = ar_WriteReg(0x0b, reg_val);
	//result = ar_Write_Serial_Reg(0x0b, ar1019_write_reg+0x0b*2, 0x02);
	ar_delay(1);

	//enable tune
	result = TuneControl(1);
	ar_delay(1);

	result = WaitSTC();
	if(result != 0)
		printk("%s: ar1019_wait_stc error at line %d\n", __FUNCTION__, __LINE__);
	reg_val = ar_ReadReg(0x12); //get RSSI (RSSI2)
	RSSI2 = (reg_val >> 9);
	printk("reg_val=0x%x, RSSI2=%d\n", reg_val, RSSI2);

	//clear tune
	result = TuneControl(0);

	if (RSSI1 >= RSSI2) //Set D15,	Clear D0/D2
	{
		ar1019_write_reg[22] |= (uint8_t) 0x80;
		ar1019_write_reg[23] &= (uint8_t) 0xfa;
		printk("# Set D15,	Clear D0/D2\n");
	}
	else //Clear D15, Set D0/D2
	{
		ar1019_write_reg[22] &= 0x7f;
		ar1019_write_reg[23] |= 0x05;
		printk("# Clear D15, Set D0/D2\n");
	}
	reg_val = (ar1019_write_reg[22]<<8 | ar1019_write_reg[23]);
	result = ar_WriteReg(0x0b, reg_val);
	//result = ar_Write_Serial_Reg(0x0b, ar1019_write_reg+22, 0x02);

	//enable tune
	result = TuneControl(1);
	if (result == 0)
	{
		result = WaitSTC();
		if(result != 0)
			printk("%s: ar1019_wait_stc error at line %d\n", __FUNCTION__, __LINE__);
	}
#endif
	//for(int i=0; i<=0x1C; i++)
	//	printf("reg[%d]: 0x%x\n", i, ar_ReadReg(i));
	
	//ar_delay(10);
	//ar1019_set_mute(0);

	//reg_val = ar_ReadReg(0x02);
	//reg_val = reg_val & 0x1FF;
	//printk("### %s: set freq[%d], CHAN: %f, ret: %d\n", __FUNCTION__, freq_tmp, 69 + reg_val*0.1, result);

	return result;
}




/*
 ***** ***************************************************************************
 * int FM_Restore_Global_Data(uint8 band, uint8 level, uint16 freq)
 *
 * Description : 重新恢复FM驱动所需要的全局数组WriteBuffer的内容
 *
 * Arguments : 波段选择值,门限
 *                   band=0: 中国/美国电台，步进100KHz
 *                   band=1: 日本电台，步进100KHz
 *                   band=2: 欧洲电台，步进50KHz
 *
 * Returns : 若初始化成功,则返回1,否则,返回0
 *
 * Notes :
 *         由于FM驱动和录音codec所占用的空间是一致的，因此FM录音时FM驱动代码被覆盖，
 *         当退出FM驱动时，需要重新恢复WriteBuffer的内容，以便可以正确的设置相关模
 *         组的寄存器值
 *
 ********************************************************************************
 */
static int FM_Restore_Global_Data(u8_t band, u8_t level, u16_t freq)
{
    u8_t chan_bit0;
    u16_t freq_tmp;
	

    //band = band;
    //level = level;

    freq = FreqToChan(freq, Space50k);

    chan_bit0 = (u8_t)(freq % 2);
    freq_tmp = (freq >> 1); // CHAN<9:1>

    ar1019_write_reg[1] |= (0x01 << 0); //FM power up, Enable

    ar1019_write_reg[4] &= (u8_t) 0xfd; //clear tune
    ar1019_write_reg[6] &= (u8_t) ~(0x01 << 6); //clear SEEK

    //set band/space/chan,enable tune
    ar1019_write_reg[4] &= (u8_t) 0xf6;
    ar1019_write_reg[4] |= (chan_bit0 << 3);
    ar1019_write_reg[4] |= (u8_t)(freq_tmp >> 8);
    ar1019_write_reg[5] = (u8_t)(freq_tmp % 256);

    return 1;

}


/*
 ***** ***************************************************************************
 * int  FM_Init(uint8 band, uint8 level, void* null3)
 *
 * Description : FM初始化,设置搜索门限,波段,步进等
 *
 * Arguments : 波段选择值,门限
 *                   band=0: 中国/美国电台，步进100KHz
 *                   band=1: 日本电台，步进100KHz
 *                   band=2: 欧洲电台，步进50KHz
 *
 * Returns : 若初始化成功,则返回1,否则,返回0
 *
 * Notes :
 *
 ********************************************************************************
 */
int fm_ar1019_init(const struct device *dev,u8_t band, u8_t level, u16_t freq)
{
    int result;
	u16_t value;

	printk("%s: enter %d\n", __FUNCTION__, __LINE__);
    memcpy(ar1019_write_reg, ar1019_init_reg, sizeof(ar1019_init_reg));
    Space50k = 0;
    if (band == FM_BAND_JAPAN)
    {
        /* 76MHz~90MHz*/
        Set_Band = 0x10;
    }
    else
    {
        /*87.5MHz~108MHz*/
        Set_Band = 0;
        if (band == FM_BAND_EUROPE)
        {
            /* 欧洲电台，步进50k */
            Space50k = 0x01;
        }
    }

	ar1019_write_reg[6] &= (uint8_t) ~(0x01 << 6); //初始化,disable SEEK
	
    ar1019_write_reg[6] &= (uint8_t) 0xe7;
    ar1019_write_reg[6] |= Set_Band; //设置频段
    ar1019_write_reg[30] &= (uint8_t) 0xfe;
    ar1019_write_reg[30] |= Space50k; //设置步进

    ar1019_write_reg[7] &= (uint8_t) 0x80;
    ar1019_write_reg[7] |= level; //搜台门限<6:0>

    ar1019_write_reg[1] &= ~(0x01 << 0); //Enable bit = 0

	value = ar1019_write_reg[0x00]<<8 | ar1019_write_reg[0x00+1];
	
    if (freq != 0)
    {
        return FM_Restore_Global_Data(band, level, freq);
    }

    result = ar_WriteReg(0x00, value);	//FM standby  ， 写2 个字节寄存器内容，只设置了FM_Enable
    if(result == 0)
	{
		result = ar_Write_Serial_Reg(0x01, &ar1019_write_reg[0x01*2], 17*2); //init, from R1 to R17
		if(result == 0)
		{
            ar1019_write_reg[1] |= (0x01 << 0); //FM power up, Enable
			value = ar1019_write_reg[0x00]<<8 | ar1019_write_reg[0x00+1];
            result = ar_WriteReg(0x00, value);
		}
	}
	if(result == 0)
	{
        result = WaitSTC(); //wait STC
	}
	printk("%s: return %d\n", __FUNCTION__, result);
    return result;
}

/*
 ********************************************************************************
 *             int sFM_Standby(void* null1, void* null2, void* null3)
 *
 * Description : FM standby
 *
 * Arguments : NULL
 *
 * Returns : 若设置standby成功,则返回1,否则,返回0
 *
 * Notes :
 *
 ********************************************************************************
 */
/*FM模组进入standby*/
int fm_ar1019_standy(const struct device *dev)
{
    u16_t value;
	int result;
    ar1019_write_reg[1] &= ~(0x01 << 0); //FM Enable bit = 0
    value = ar1019_write_reg[0x00]<<8 | ar1019_write_reg[0x00+1];
    result = ar_WriteReg(0x00, value);
    return result;
}


/*extern int ar1019_rx_init(void)
{
	struct fm_rx_device *ar1019_rx = &ar1019_info.ar1019_rx_dev;

	printk("%s\n", __func__);

	ar1019_rx->i2c = device_get_binding(CONFIG_FM_I2C_NAME);
	if (!ar1019_rx->i2c) {
		// SYS_LOG_ERR("Device not found\n");
		printk("i2c_device_binding failed!!!\n");
		return -1;
	}

	// union dev_config i2c_cfg = {0};
	// i2c_cfg.raw = 0;
	// i2c_cfg.bits.is_master_device = 1;
	// i2c_cfg.bits.speed = I2C_SPEED_STANDARD;
	// i2c_configure(ar1019_rx->i2c, i2c_cfg.raw);
	i2c_configure(ar1019_rx->i2c, I2C_SPEED_STANDARD|I2C_MODE_MASTER);

	ar1019_rx->init = fm_ar1019_init;
	ar1019_rx->set_freq = fm_ar1019_set_freq;
	ar1019_rx->set_mute = fm_ar1019_set_mute;
	ar1019_rx->check_inval_freq = ar1019_check_freq;
	ar1019_rx->deinit = fm_ar1019_standy;
	

	fm_receiver_device_register(ar1019_rx);

	return 0;
}*/

//SYS_INIT(rda5820_tx_init, POST_KERNEL, 80);
