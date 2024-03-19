/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions SoC On/Off Key driver
 */

#include <errno.h>
#include <kernel.h>
#include <string.h>
#include <stdbool.h>
#include <init.h>
#include <irq.h>
#include <drivers/input/input_dev.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <board.h>
#include <soc_pmu.h>
#include <logging/log.h>
#include <device.h>
#include <drivers/gpio.h>
#include <soc.h>
#include <string.h>
#include <drivers/i2c.h>
#include <board_cfg.h>

#include "jsa1227.h"

LOG_MODULE_REGISTER(ear_inout, 2);

#define EAR_INOUT_PIN_MODE_DEFAULT (GPIO_INPUT | GPIO_PULL_UP | GPIO_INT_DEBOUNCE)

static const struct gpio_cfg _ear_isr_gpio_cfg = CONFIG_EAR_ISR_GPIO;

struct ear_inout_data {
	input_notify_t notify;
	struct device *i2c_dev;
	const  struct device *gpio_dev;
	const struct device *this_dev;
	struct gpio_callback ear_gpio_cb;
};

struct ear_inout_config {
	uint32_t irq_num;
};

static struct ear_inout_data _ear_inout_ddata;

static const struct ear_inout_config _ear_inout_cdata;

//jsa_calib_para_st jsa_calib_para;

static bool _i2c_write_bytes(const struct device *dev, uint16_t reg,
					uint8_t *data, uint16_t len, uint8_t regLen)
{
	int i2c_op_ret = 0;
	struct ear_inout_data *ear_inout = dev->data;
	uint8_t retrycnt = 10;
	uint8_t sendBuf[10];
	int lenT=0;

	if (regLen == 2) {
		sendBuf[0] = reg / 0x100;
		sendBuf[1] = reg % 0x100;
		lenT = 2;
	} else	{
		sendBuf[0]=reg % 0x100;
		lenT = 1;
	}

	memcpy(sendBuf + lenT, data, len);
	lenT += len;

	while (retrycnt--)
	{
		i2c_op_ret = i2c_write(ear_inout->i2c_dev, sendBuf, lenT, EAR_I2C_SLAVE_ADDR);
		if (0 != i2c_op_ret) {
			k_msleep(4);
			LOG_ERR("err = %d\n", i2c_op_ret);
		} else {
			return true;
		}
	}

	return false;
}

static bool _i2c_read_bytes(const struct device *dev, uint16_t reg, uint8_t *value, uint16_t len,uint8_t regLen)
{
	int i2c_op_ret = 0;
	struct ear_inout_data *ear_inout = dev->data;
	uint8_t retrycnt = 10;

	uint8_t sendBuf[4];
	uint32_t lenT=0;

	if (regLen==2) {
		sendBuf[0] = reg / 0x100;
		sendBuf[1] = reg % 0x100;
		lenT=2;
	} else {
		sendBuf[0] = reg % 0x100;
		lenT=1;
	}

  while (retrycnt--)
	{
		i2c_op_ret = i2c_write(ear_inout->i2c_dev, sendBuf, lenT, EAR_I2C_SLAVE_ADDR);
		if(0 != i2c_op_ret) {
			k_msleep(4);
			LOG_ERR("err = %d\n", i2c_op_ret);
			if (retrycnt == 1)
			{
				return false;
			}
		}
		else
		{
			break;
		}
	}

	retrycnt = 10;

	while (retrycnt--)
	{
		i2c_op_ret = i2c_read(ear_inout->i2c_dev, value, len, EAR_I2C_SLAVE_ADDR);
		if(0 != i2c_op_ret) {
			k_msleep(4);
			LOG_ERR("err = %d\n", i2c_op_ret);
		}
		else
		{
			return true;
		}
	}

  return false;
}

static uint8_t _jsa_read_1byte_data(const struct device *dev, uint8_t reg_addr, uint8_t flag) 
{
	uint8_t PXS_Data;
    uint16_t reg = reg_addr & 0x00ff;
	
	uint8_t readBuf[4];
    
    _i2c_read_bytes(dev, reg, readBuf, 1, 1);

	PXS_Data = readBuf[0] & flag;
	return PXS_Data;
}

static uint8_t _jsa_set_1byte_data(const struct device *dev, uint8_t reg_addr, uint8_t value, uint8_t flag)
{
    uint8_t writeBuf[2];
    writeBuf[0] = value & flag;
   _i2c_write_bytes(dev, (uint16_t)reg_addr, writeBuf, 1, 1);
	return 0;
}

void JSA_SET_CALIB_PARA(void)
{
    #if 0
//	U16 ps_data;
	
	//NVKEY_ReadFullKey(NVKEYID_IR_JSA, (U8 *)&jsa_calib_para, sizeof(jsa_calib_para));

	if (true == jsa_calib_para.flag) {

//		ps_data = JSA_Get_PS_Data();
		
//		if (ps_data > 500) {
			_jsa_set_1byte_data(JSA1227_CALIB_CTRL,0x1F,0XFF);
			//JSA_set_PGAvalue(jsa_calib_para.ps_pga_value);
			printk("Lange ps data > 500, calibration ps");
//		}
//		else {
//			JSA_Set_1Byte_Data(JSA1227_CALIB_CTRL,0x0F,0XFF);
//			ATE_calibration();
//			printk("Lange ps data <= 500, ate calibration ");
//		}
		
		//JSA_Set_HTH_Data(jsa_calib_para.ps_high_threshold);
		//JSA_Set_LTH_Data(jsa_calib_para.ps_low_threshold);
	}
	else {
		printk("Lange jsa calibration para invalid!");
	}
    #endif
}

void jsa_Init(const struct device *dev, bool iscalib)
{
	uint8_t id_value[2] = {0,0};
	
	id_value[1] = _jsa_read_1byte_data(dev, JSA1227_PROD_ID_H, 0XFF);
	id_value[0] = _jsa_read_1byte_data(dev, JSA1227_PROD_ID_L, 0XFF);
	printk("Lange JSA id 0X%x%x", id_value[1], id_value[0]);
	
	if (id_value[1] != JSA1227_PROD_ID_VAL_H || id_value[0] != JSA1227_PROD_ID_VAL_L) {
		printk("Lange JSA i2c comm fail!");
		return;
	}
	
	_jsa_set_1byte_data(dev, JSA1227_SYSTEM_CONTROL,0x00,0XFF);
	_jsa_set_1byte_data(dev, JSA1227_INT_CTRL,0x03,0XFF);
	_jsa_set_1byte_data(dev, JSA1227_WAIT_TIME,0x05,0XFF); 
	_jsa_set_1byte_data(dev, JSA1227_PS_GAIN,VCSEL_CURRENT_10mA|PGA_PS_X2,0XFF);	 
	_jsa_set_1byte_data(dev, JSA1227_PS_PULSE,(ITW_PS & 0x0F),0XFF);	
	_jsa_set_1byte_data(dev, JSA1227_PS_TIME,ICT_PS<<4 | (PSCONV & 0x0F),0XFF);
	_jsa_set_1byte_data(dev, JSA1227_PS_FILTER, 0xB0 | (NUM_AVG & 0x0F),0XFF);
	_jsa_set_1byte_data(dev, JSA1227_PS_PERSISTENCE,PRS_PS<<4 | 0x01,0XFF);
	_jsa_set_1byte_data(dev, JSA1227_PS_LOW_THRESHOLD_L,JSA1227_PS_Default_LOW_THRESHOLD & 0xFF,0XFF);
	_jsa_set_1byte_data(dev, JSA1227_PS_LOW_THRESHOLD_H,(JSA1227_PS_Default_LOW_THRESHOLD>>8)&0x0F,0XFF);
	_jsa_set_1byte_data(dev, JSA1227_PS_HIGH_THRESHOLD_L,JSA1227_PS_Default_HIGH_THRESHOLD & 0xFF,0XFF);
	_jsa_set_1byte_data(dev, JSA1227_PS_HIGH_THRESHOLD_H,(JSA1227_PS_Default_HIGH_THRESHOLD>>8)&0x0F,0XFF);
	_jsa_set_1byte_data(dev, JSA1227_PS_CALIBRATION_L,0x00,0XFF);
	_jsa_set_1byte_data(dev, JSA1227_PS_CALIBRATION_H,0x00,0XFF);
	_jsa_set_1byte_data(dev, JSA1227_MANU_CDAT_L,JSA1227_Default_CALIBRATION & 0xFF,0XFF);
	_jsa_set_1byte_data(dev, JSA1227_MANU_CDAT_H,(JSA1227_Default_CALIBRATION >> 8) & 0x01,0XFF);
	_jsa_set_1byte_data(dev, JSA1227_PS_PIPE_THRES,0x80,0XFF);
	_jsa_set_1byte_data(dev, JSA1227_CALIB_CTRL,0x1F,0XFF);
	_jsa_set_1byte_data(dev, JSA1227_SYSTEM_CONTROL,0x46,0XFF);  //  Wating time enalbe ; PS function enable
	_jsa_set_1byte_data(dev, JSA1227_INT_FLAG,0x00,0XFF);
	_jsa_set_1byte_data(dev, JSA1227_CALIB_STAT,0x00,0XFF);
 
	if (iscalib)	// У׼
		JSA_SET_CALIB_PARA();
}

static void ear_inout_enable(const struct device *dev)
{
	//struct ear_inout_data *ear_inout = dev->data;

   // jsa_Init(dev, 0);
	//gpio_pin_interrupt_configure(ear_inout->gpio_dev , _ear_isr_gpio_cfg.gpion, GPIO_INT_EDGE_FALLING);//GPIO_INT_DISABLE

	LOG_DBG("enable gear_inout");
}

static void ear_inout_disable(const struct device *dev)
{
	struct ear_inout_data *ear_inout = dev->data;

	gpio_pin_interrupt_configure(ear_inout->gpio_dev , _ear_isr_gpio_cfg.gpion, GPIO_INT_DISABLE);//GPIO_INT_DISABLE

	LOG_DBG("disable ear inout.");
}


static void ear_inout_register_notify(const struct device *dev, input_notify_t notify)
{
	struct ear_inout_data *ear_inout = dev->data;

	LOG_DBG("register notify 0x%x", (uint32_t)notify);

	ear_inout->notify = notify;
}

static void ear_inout_unregister_notify(const struct device *dev, input_notify_t notify)
{
	struct ear_inout_data *ear_inout = dev->data;

	LOG_DBG("unregister notify 0x%x", (uint32_t)notify);

	ear_inout->notify = NULL;
}


const struct input_dev_driver_api ear_inout_driver_api = {
	.enable = ear_inout_enable,
	.disable = ear_inout_disable,
	.register_notify = ear_inout_register_notify,
	.unregister_notify = ear_inout_unregister_notify,
};

static void _ear_report_key(int state)
{
	struct input_value val = {0};
	struct ear_inout_data *ear_inout = &_ear_inout_ddata;

	if (ear_inout->notify) {
		val.keypad.code = KEY_EAR;
		val.keypad.type = EV_KEY;

		val.keypad.value = state;
		LOG_DBG("report onoff key code:%d value:%d", val.keypad.code, state);
		ear_inout->notify(NULL, &val);
	}
}


static int _ear_i2c_async_write_cb(void *cb_data, struct i2c_msg *msgs,
					uint8_t num_msgs, bool is_err)
{
	if (is_err) {
		printk("ear i2c write err\n");
	}
	return 0;
}


static int _ear_i2c_async_read_cb(void *cb_data, struct i2c_msg *msgs,
					uint8_t num_msgs, bool is_err)
{
	if (!is_err) {
		uint8_t value = msgs->buf[0];
        if(value & (1 << 7)){  //por flag
            return 0;
        }

        if(value & (1 << 5)){   //OBJ flag
           _ear_report_key(1);  //near the ear
        }else{
           _ear_report_key(0);  //far of the ear
        }
	} else {
		printk("ear i2c read err\n");
	}
	return 0;
}

static void _ear_inout_state_process(struct device *i2c_dev)
{
	static uint8_t write_cmd[1] = {0};
	static uint8_t read_cmd[5] = {0};
	int ret = 0;

	sys_trace_void(SYS_TRACE_ID_TP_READ);

	write_cmd[0] = 0x02;
	ret = i2c_write_async(i2c_dev, write_cmd, 1, EAR_I2C_SLAVE_ADDR, _ear_i2c_async_write_cb, NULL);
	if (ret == -1)
		goto exit;

	ret = i2c_read_async(i2c_dev, read_cmd, 1, EAR_I2C_SLAVE_ADDR, _ear_i2c_async_read_cb, NULL);
	if (ret == -1)
		//goto exit;
exit:
	sys_trace_end_call(SYS_TRACE_ID_TP_READ);
	return;
}


static void _ear_irq_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct ear_inout_data *ear_inout = &_ear_inout_ddata;

    printk("%s\n", __func__);
	sys_trace_void(SYS_TRACE_ID_TP_IRQ);
    _ear_inout_state_process(ear_inout->i2c_dev);
	sys_trace_end_call(SYS_TRACE_ID_TP_IRQ);
}

static void _ear_irq_config(const struct device *dev)
{
	struct ear_inout_data *ear_inout = dev->data;

	ear_inout->gpio_dev = device_get_binding(_ear_isr_gpio_cfg.gpio_dev_name);

	gpio_pin_configure(ear_inout->gpio_dev , _ear_isr_gpio_cfg.gpion, EAR_INOUT_PIN_MODE_DEFAULT);
	gpio_init_callback(&ear_inout->ear_gpio_cb , _ear_irq_callback, BIT(_ear_isr_gpio_cfg.gpion));
	gpio_add_callback(ear_inout->gpio_dev , &ear_inout->ear_gpio_cb);

}

int _ear_inout_init(const struct device *dev)
{
	struct ear_inout_data *ear_inout = dev->data;

    jsa_Init(dev, 0);

	ear_inout->i2c_dev = (struct device *)device_get_binding(CONFIG_EARINOUT_I2C_NAME);
	if (!ear_inout->i2c_dev) {
		printk("can not access right i2c device\n");
		return -1;
	}

    _ear_irq_config(dev);

    //should invoke in enable.
    gpio_pin_interrupt_configure(ear_inout->gpio_dev , _ear_isr_gpio_cfg.gpion, GPIO_INT_EDGE_FALLING);//GPIO_INT_DISABLE

	ear_inout->this_dev = dev;
    return 0;
}

#if IS_ENABLED(CONFIG_EAR_INOUT)
DEVICE_DEFINE(ear_inout, CONFIG_INPUT_DEV_EAR_INOUT_NAME,
		    _ear_inout_init, NULL,
		    &_ear_inout_ddata, &_ear_inout_cdata,
		    POST_KERNEL, 60,
		    &ear_inout_driver_api);
#endif

