/* Copyright Statement:
 *
 * (C) 2017  Airoha Technology Corp. All rights reserved.
 *
 */


#ifndef _JSA_1227_H_
#define _JSA_1227_H_

#define EAR_I2C_SLAVE_ADDR 0X70		//(0x38<<1)

//======================================Register=======================================
#define JSA1227_SYSTEM_CONTROL              0x00  //Control of basic functions
#define JSA1227_INT_CTRL                    0x01  //Interrupt enable and auto/semi clear INT selector, duke 20190807
#define JSA1227_INT_FLAG                    0x02  //ALS and PS interrupt status and flag output, duke 20190807
#define JSA1227_WAIT_TIME                   0x03  //Waiting Time Setup
#define JSA1227_PS_GAIN                     0x06  //Gain control of PS 
#define JSA1227_PS_PULSE                    0x07  //VCSEL led pulse width
#define JSA1227_PSPD_CONFIG                 0x08  //PD number selective function
#define JSA1227_PS_TIME                     0x09  //resolution tuning of PS
#define JSA1227_PS_FILTER                   0x0A  //Hard ware Average filter of PS
#define JSA1227_PS_PERSISTENCE              0x0B  //Configure the Persist count of PS
#define JSA1227_PS_LOW_THRESHOLD_L          0x10  //Lower part of PS low threshold
#define JSA1227_PS_LOW_THRESHOLD_H          0x11  //Upper part of PS low threshold
#define JSA1227_PS_HIGH_THRESHOLD_L         0x12  //Lower part of PS high threshold
#define JSA1227_PS_HIGH_THRESHOLD_H         0x13  //Upper part of PS high threshold
#define JSA1227_PS_CALIBRATION_L            0x14  //Offset value to eliminate the Cross talk effect
#define JSA1227_PS_CALIBRATION_H            0x15  //Offset value to eliminate the Cross talk effect
#define JSA1227_PS_DATA_LOW                 0x1A  //Low byte for PS ADC channel output
#define JSA1227_PS_DATA_HIGH                0x1B  //High byte for PS ADC channel output
#define JSA1227_ERROR_FLAG                  0x17
#define JSA1227_CALIB_CTRL                  0x26
#define JSA1227_CALIB_STAT                  0x28
#define JSA1227_MANU_CDAT_L                 0x2A
#define JSA1227_MANU_CDAT_H                 0x2B
#define JSA1227_AUTO_CDAT_L                 0x2C
#define JSA1227_AUTO_CDAT_H                 0x2D
#define JSA1227_PS_PIPE_THRES               0x2E
#define JSA1227_PS_CALIBRATION              0xA0   
#define JSA1227_PROD_ID_L                   0xBC  //  0x11
#define JSA1227_PROD_ID_H                   0xBD  //  0x42
#define JSA1227_PROD_ID_VAL_L				0X11	// ID low bits
#define JSA1227_PROD_ID_VAL_H				0X42	// ID high bits
//======================================parameter==================================
#define VCSEL_CURRENT_5mA                   0x00
#define VCSEL_CURRENT_10mA                  0x10
#define VCSEL_CURRENT_15mA                  0x20
#define VCSEL_CURRENT_20mA                  0x30
#define PGA_PS_X1                           0x00
#define PGA_PS_X2                           0x01
#define PGA_PS_X4                           0x02
#define ITW_PS                              0x02  //  pulse Width  maxmum value during  0 ~ F
#define ICT_PS                              0x01  //  pulse count maxmum value is 0~F
#define PSCONV                              0x03  //  output data maxmum value is 0~F
#define NUM_AVG                             0x03  //  Average filer times that is maxmum value during 0~F
#define PRS_PS                              0x03  //
#define JSA1227_PS_Default_HIGH_THRESHOLD   600   //  defalue  High threshold
#define JSA1227_PS_Default_LOW_THRESHOLD    400   //  defalue  low threshold
#define JSA1227_Default_CALIBRATION         23    //  defalue  Xtalk  Minimun is 1,  maxmum is 511
#define MAX_HIGH_THRESHOLD                  900   //  to ensure not over set high threshold
#define MIN_HIGH_THRESHOLD                  300   //  to ensure not over set high threshold
#define MAX_LOW_THRESHOLD                   800   //  to ensure not over set low threshold
#define MIN_LOW_THRESHOLD                   200   //  to ensure not over set low threshold
#define JSA1227_Compensation                100   //  Pollution compensation value,  
#define JSA1227_ATE_XTALK_LOW_THRESHOLD     10   //  ATE X_TALK  LOWTHRESHOLD   ,  if the noise is lower than this value , the VCSEL  or Lens May exceed the specifications.
#define JSA1227_ATE_XTALK_HIGH_THRESHOLD    300   //  ATE X_TALK  LOWTHRESHOLD  ,  if  the Noise is higher than this value , the Lens may exceed the specifications.
#define JSA1227_INBOX_THRESHOLD             800
#define EEPROM_CT_L                           1
#define EEPROM_CT_H                           2
#define EEPROM_LT_L                           3
#define EEPROM_LT_H                           4
#define EEPROM_HT_L                           5
#define EEPROM_HT_H                           6
#define debug                                 0


/**************************************************************************************************
* Enumeration
**************************************************************************************************/
enum
{
	PSensor_TIMER_ID_0,
	PSensor_TIMER_ID_1,
	PSensor_TIMER_ID_2,
	PSensor_TIMER_ID_PGA,
	PSensor_TIMER_ID_HTH,
	PSensor_TIMER_ID_LTH,
	PSensor_TIMER_ID_6,
	PSensor_TIMER_ID_7,
	PSensor_TIMER_ID_8,
	PSensor_TIMER_ID_9,
};

typedef struct 
{
	uint16_t ps_data;
	uint8_t num;
	uint8_t H_data;
	uint8_t L_data;
} Calibration_PARA_STRU;

typedef struct
{
	uint16_t ps_pga_value;	// PGA
	uint16_t ps_high_threshold;	// PS high
	uint16_t ps_low_threshold;	// PS low
	uint8_t flag;
} jsa_calib_para_st;	// jsa calibration





#endif  //_JSA_1228

