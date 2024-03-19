/*******************************************************************************
 * @file    sensor_algo.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2021-5-25
 * @brief   sensor algorithm api
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sensor_algo.h>
#include <sensor_algo_data.h>
#include <cwm_lib.h>

/******************************************************************************/
//constants
/******************************************************************************/
#define DBG(...)			os_api.dbgOutput(__VA_ARGS__)
//#define DBG(...)			do {} while (0)

#define PI						(3.14159265f)

/******************************************************************************/
//variables
/******************************************************************************/

/* CWM os api */
static sensor_os_api_t sensor_os_api = {0};
static OsAPI os_api = {0};

/* event handler */
static sensor_handler_t user_handler = NULL;

/******************************************************************************/
//functions
/******************************************************************************/

/* sensor algo save result */
static void sensor_algo_save(sensor_evt_t *evt)
{
	sensor_res_t *res = &sensor_algo_res;
	struct gnss_s *gnss = &res->gnss;
	struct sleeping_s *sleeping = &res->sleeping;
	struct pedometer_s *pedometer = &res->pedometer;
	struct biking_s *biking = &res->biking;
	sensor_datetime_t *dt;
	uint32_t val;
	
	/* save result */
	switch(evt->id) {
		case RAW_MAG:
			res->orientation = atan2f(evt->fData[0],evt->fData[1]) * (180 / PI) + 180;
			break;
		
		case RAW_BARO:
			res->pressure = evt->fData[0];
			break;
		
		case RAW_TEMP:
			res->temperature = evt->fData[0];
			break;
		
		case RAW_HEARTRATE:
			res->heart_rate = evt->fData[0];
			break;
		
		case RAW_GNSS:
			gnss->latitude = evt->fData[1];
			gnss->longitude = evt->fData[3];
			gnss->altitude = evt->fData[5];
			gnss->bearing = evt->fData[6];
			gnss->speed = evt->fData[7];
			break;
		
		case RAW_OFFBODY:
			res->onbody = (uint32_t)evt->fData[0];
			break;

		case ALGO_SEDENTARY:
			res->sedentary = (uint32_t)evt->fData[2];
			break;
		
		case ALGO_HANDUP:
			res->handup = (uint32_t)evt->fData[1];
			break;
		
		case ALGO_SLEEP:
			val = (uint32_t)evt->fData[0];
			if (val == 1) {
				sleeping->status = (uint32_t)evt->fData[2];
				switch (sleeping->status) {
					case 0:
						dt = &sleeping->time_enter;
						break;
					case 1:
						dt = &sleeping->time_shallow;
						break;
					case 2:
						dt = &sleeping->time_deep;
						break;
					case 3:
						dt = &sleeping->time_awake;
						break;
					case 4:
						dt = &sleeping->time_exit;
						break;
					case 12:
						dt = &sleeping->time_rem;
						break;
				}
				dt->month = (uint8_t)evt->fData[3];
				dt->day = (uint8_t)evt->fData[4];
				dt->hour = (uint8_t)evt->fData[5];
				dt->minute = (uint8_t)evt->fData[6];
			} else if (val == 2) {
				sleeping->duration_total = (uint32_t)evt->fData[1];
				sleeping->duration_shallow = (uint32_t)evt->fData[2];
				sleeping->duration_deep = (uint32_t)evt->fData[3];
				sleeping->duration_awake = (uint32_t)evt->fData[4];
				sleeping->duration_rem = (uint32_t)evt->fData[5];
			}
			break;
		
		case ALGO_ACTTYPEDETECT:
			res->activity_detect = (uint32_t)evt->fData[0];
			break;
		
		case ALGO_ACTIVITY_OUTPUT:
			val = (uint32_t)evt->fData[0];
			if ((val != MAIN_EXTRA) && (val != BIKING_EXTRA)) {
				res->activity_mode = (uint32_t)evt->fData[0];
			}
			if (val == BIKING_EXTRA) {
				biking->avg_speed = evt->fData[1];
				biking->max_speed = evt->fData[2];
			} else if (val == MAIN_EXTRA) {
				pedometer->avg_step_freq = evt->fData[1];
				pedometer->avg_step_len = evt->fData[2];
				pedometer->avg_pace = evt->fData[3];
				pedometer->max_step_freq = evt->fData[4];
				pedometer->max_pace = evt->fData[5];
				pedometer->vo2max = evt->fData[6];
				pedometer->hr_intensity = evt->fData[7];
				pedometer->hr_zone = evt->fData[8];
			} else if (val == OUTDOOR_BIKING) {
				biking->latitude = evt->fData[1];
				biking->longitude = evt->fData[3];
				biking->distance = evt->fData[5];
				biking->cur_speed = evt->fData[6];
				biking->elevation = evt->fData[7];
				biking->slope = evt->fData[8];
				biking->calories = evt->fData[9];
				biking->vo2max = evt->fData[10];
				biking->hr_intensity = evt->fData[11];
				biking->hr_zone = evt->fData[12];
			} else {
				pedometer->activity_type = (uint32_t)evt->fData[4];
				pedometer->total_steps = evt->fData[1];
				pedometer->total_distance = evt->fData[2];
				pedometer->total_calories = evt->fData[3];
				pedometer->cur_step_freq = evt->fData[5];
				pedometer->cur_step_len = evt->fData[6];
				pedometer->cur_pace = evt->fData[7];
				pedometer->slope = evt->fData[8];
				pedometer->elevation = evt->fData[9];
				pedometer->floors_up = evt->fData[10];
				pedometer->floors_down = evt->fData[11];
				pedometer->elevation_up = evt->fData[12];
				pedometer->elevation_down = evt->fData[13];
			}
			break;
	}
}

/* sensor algo event handler */
static void sensor_algo_handler(SensorEVT_t *event)
{
	sensor_evt_t *evt = (sensor_evt_t*)event;
	
	// save result
	sensor_algo_save(evt);
	
	// call user handler
	if (user_handler != NULL) {
		user_handler(evt);
	}
}

/* Init sensor algo */
int sensor_algo_init(const sensor_os_api_t *api)
{
	SettingControl_t scl;
	char chipInfo[64];

	// init data section
  memcpy(_data_ram_start, _data_rom_start, (_data_rom_end - _data_rom_start));
	
	// init bss section
  memset(_bss_start, 0, (_bss_end - _bss_start));
	
	// init heap section
  memset(_heap_start, 0, (_heap_end - _heap_start));
	
	// config os api
	sensor_algo_set_os_api(api);

	//get lib version information
	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	CWM_SettingControl(SCL_GET_LIB_INFO, &scl);
	DBG("version:%d.%d.%d.%d product:%d\r\n", 
		scl.iData[1], scl.iData[2], scl.iData[3], scl.iData[4], scl.iData[5]);

	// config chip vendor
	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 0;
	scl.iData[2] = 0;
	scl.iData[3] = 0;
	CWM_SettingControl(SCL_CHIP_VENDOR_CONFIG, &scl);

	// set debug level
	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 0;
	CWM_SettingControl(SCL_LIB_DEBUG, &scl);

	// post-init
	CWM_LibPostInit(sensor_algo_handler);

	//get chip information
	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 1;
	scl.iData[2] = (int)chipInfo;
	scl.iData[3] = sizeof(chipInfo);
	scl.iData[4] = 0;
	scl.iData[5] = 0;
	scl.iData[6] = 0;
	CWM_SettingControl(SCL_GET_CHIP_INFO, &scl);
	DBG("have_security = %d.%d ret_buff_size = %d  chipInfo = %s\r\n", 
		scl.iData[5], scl.iData[6], scl.iData[4], chipInfo);
	DBG("chip_settings = %d, %d, %d\r\n", scl.iData[9], scl.iData[10], scl.iData[11]);

	//initial request_sensor to receive hardware_sensor on/off event
	CWM_Sensor_Enable(IDX_REQUEST_SENSOR);

	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = ACTIVITY_MODE_NORMAL;
	CWM_SettingControl(SCL_SET_ACTIVITY_MODE, &scl);

	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 2019;
	scl.iData[2] = 8;
	scl.iData[3] = 20;
	scl.iData[4] = 3;
	scl.iData[5] = 0;
	CWM_SettingControl(SCL_DATE_TIME, &scl);

	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 30;
	scl.iData[2] = 1;
	scl.iData[3] = 5;
	CWM_SettingControl(SCL_SEDENTARY, &scl);

//	CWM_Sensor_Enable(IDX_MAG);
//	CWM_Sensor_Enable(IDX_BARO);
//	CWM_Sensor_Enable(IDX_HEARTRATE);
//	CWM_Sensor_Enable(IDX_ALGO_ACTIVITY_OUTPUT);
	
	return 0;
}

/* Get os api */
sensor_os_api_t* sensor_algo_get_os_api(void)
{
	return &sensor_os_api;
}

/* Set os api */
int sensor_algo_set_os_api(const sensor_os_api_t *api)
{
	// config os api
	sensor_os_api = *api;
	os_api.malloc = malloc;
	os_api.free = free;
	os_api.GetTimeNs = api->get_timestamp_ns;
	os_api.dbgOutput = api->dbgOutput;
	user_handler = api->user_handler;
	
	// pre-init
	CWM_LibPreInit((OsAPI*)&os_api);
	return 0;
}

/* Control sensor algo */
int sensor_algo_control(uint32_t ctl_id, sensor_ctl_t *ctl)
{
	return CWM_SettingControl(ctl_id, (SettingControl_t*)ctl);
}

/* Enable sensor id */
int sensor_algo_enable(uint32_t id, uint32_t func)
{
	return CWM_Sensor_Enable(id);
}

/* Disable sensor id */
int sensor_algo_disable(uint32_t id, uint32_t func)
{
	return CWM_Sensor_Disable(id);
}

/* Input sensor raw data */
void sensor_algo_input(sensor_raw_t* dat)
{
	CWM_CustomSensorInput((CustomSensorData*)dat);
}

/* Input sensor fifo init */
void sensor_algo_input_fifo_init(int num)
{
	CWM_CustomSensorInput_Fifo_Init(num);
}

/* Input sensor fifo start */
int sensor_algo_input_fifo_start(uint32_t id, uint64_t timestamp, uint64_t delta)
{
	return CWM_CustomSensorInput_Fifo_Start(id, timestamp, delta);
}

/* Input sensor fifo end */
void sensor_algo_input_fifo_end(uint32_t id)
{
	CWM_CustomSensorInput_Fifo_End(id);
}

/* Get the duration time of the next event */
int64_t sensor_algo_get_next_duration(void)
{
	return CWM_GetNextActionDuration_ns();
}

/* Read sensor data and output through sensor call-back function */
int sensor_algo_process(uint32_t id)
{
	return CWM_process();
}

/* Read sensor algorithm result */
sensor_res_t* sensor_algo_get_result(void)
{
	return &sensor_algo_res;
}

/* Dump sensor event */
void sensor_algo_dump_event(sensor_evt_t *evt)
{
	switch(evt->id) {
		case REQ_SENSOR:
			DBG("%s: id: %d, on: %d\r\n", "req_sensor", (int)evt->fData[1], (int)evt->fData[2]);
			break;

		case RAW_ACCEL:
			DBG("%s: %.4f, %.4f, %.4f\r\n", "accel", evt->fData[0], evt->fData[1], evt->fData[2]);
			break;
		
		case RAW_GYRO:
			DBG("%s: %.4f, %.4f, %.4f\r\n", "gyro", evt->fData[0], evt->fData[1], evt->fData[2]);
			break;
		
		case RAW_MAG:
			DBG("%s: %.4f, %.4f, %.4f,%.4f\r\n", "mag", 
				evt->fData[0], evt->fData[1], evt->fData[2], evt->fData[3]);
			break;
		
		case RAW_BARO:
			DBG("%s: %.4f\r\n", "baro", evt->fData[0]);
			break;
		
		case RAW_TEMP:
			DBG("%s: %.4f\r\n", "temp", evt->fData[0]);
			break;
		
		case RAW_HEARTRATE:
			DBG("%s: %.4f, %.4f\r\n", "heart rate", evt->fData[0], evt->fData[1]);
			break;
		
		case RAW_GNSS:
			DBG("%s: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f\r\n", "gnss",
				evt->fData[0], evt->fData[1], evt->fData[2], evt->fData[3],
				evt->fData[4], evt->fData[5], evt->fData[6], evt->fData[7], evt->fData[8]);
			break;
		
		case RAW_OFFBODY:
			DBG("%s: %.4f, %.4f, %.4f\r\n", "offbody", evt->fData[0], evt->fData[1], evt->fData[2]);
			break;

		case ALGO_SEDENTARY:
			DBG("%s: %.4f, %.4f, %.4f\r\n", "sedentary", evt->fData[0], evt->fData[1], evt->fData[2]);
			break;
		
		case ALGO_HANDUP:
			DBG("%s: %.4f, %.4f, %.4f\r\n", "watch handup", evt->fData[0], evt->fData[1], evt->fData[2]);
			break;
		
		case ALGO_SLEEP:
			DBG("%s: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f\r\n", "sleep",
				evt->fData[0], evt->fData[1], evt->fData[2], evt->fData[3],
				evt->fData[4], evt->fData[5], evt->fData[6]);
			break;
		
		case ALGO_ACTTYPEDETECT:
			DBG("%s: %.4f, %.4f\r\n", "act type", evt->fData[0], evt->fData[1]);
			break;
		
		case ALGO_SHAKE:
			DBG("%s: %.4f, %.4f\r\n", "shake", evt->fData[0], evt->fData[1]);
			break;
		
		case ALGO_ACTIVITY_OUTPUT:
			DBG("%s: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, "\
				"%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f\r\n", "activity output",
				evt->fData[0], evt->fData[1], evt->fData[2], evt->fData[3],
				evt->fData[4], evt->fData[5], evt->fData[6], evt->fData[7],
				evt->fData[8], evt->fData[9], evt->fData[10], evt->fData[11],
				evt->fData[12], evt->fData[13], evt->fData[14], evt->fData[15]);
			break;
	}
}

/* Dump sensor result */
void sensor_algo_dump_result(sensor_res_t *res)
{
	struct gnss_s *gnss = &res->gnss;
	struct sleeping_s *sleeping = &res->sleeping;
	struct pedometer_s *pedometer = &res->pedometer;
	struct biking_s *biking = &res->biking;
	
	DBG("\r\n");
	DBG("[base] orien=%.1f pres=%.1f alt=%.1f temp=%.1f hr=%.1f\r\n", 
		res->orientation, res->pressure, res->altitude, res->temperature, res->heart_rate);
	DBG("\r\n");
	
	DBG("[body] onbody=%d seden=%d handup=%d act_mode=%d act_detect=%d\r\n", 
		res->onbody, res->sedentary, res->handup, res->activity_mode, res->activity_detect);
	DBG("\r\n");
	
	DBG("[gnss] lat=%.1f lon=%.1f alt=%.1f bear=%.1f speed=%.1f\r\n", 
		gnss->latitude, gnss->longitude, gnss->altitude, gnss->bearing, gnss->speed);
	DBG("\r\n");
	
	DBG("[sleeping] sta=%d duration=%d enter=0x%x exit=0x%x\r\n", 
		sleeping->status, sleeping->duration_total, 
		*(uint32_t*)(&sleeping->time_enter), *(uint32_t*)(&sleeping->time_exit));
	DBG("  shallow: duration=%d time=0x%x\r\n", 
		sleeping->duration_shallow, *(uint32_t*)(&sleeping->time_shallow));
	DBG("     deep: duration=%d time=0x%x\r\n", 
		sleeping->duration_deep, *(uint32_t*)(&sleeping->time_deep));
	DBG("    awake: duration=%d time=0x%x\r\n", 
		sleeping->duration_awake, *(uint32_t*)(&sleeping->time_awake));
	DBG("      rem: duration=%d time=0x%x\r\n", 
		sleeping->duration_rem, *(uint32_t*)(&sleeping->time_rem));
	DBG("\r\n");
	
	DBG("[pedometer] steps=%.1f distance=%.1f calories=%.1f type=%d\r\n", 
		pedometer->total_steps, pedometer->total_distance, pedometer->total_calories, pedometer->activity_type);
	DBG(" step_freq: cur=%.1f avg=%.1f max=%.1f\r\n", 
		pedometer->cur_step_freq, pedometer->avg_step_freq, pedometer->max_step_freq);
	DBG("  step_len: cur=%.1f avg=%.1f\r\n", 
		pedometer->cur_step_len, pedometer->avg_step_len);
	DBG("      pace: cur=%.1f avg=%.1f max=%.1f\r\n", 
		pedometer->cur_pace, pedometer->avg_pace, pedometer->max_pace);
	DBG("    elevat: slope=%.1f elev=%.1f up=%.1f down=%.1f f-up=%.1f f-down=%.1f\r\n", 
		pedometer->slope, pedometer->elevation, 
		pedometer->elevation_up, pedometer->elevation_down, pedometer->floors_up, pedometer->floors_down);
	DBG("        hr: vo2max=%.1f hr_inten=%.1f hr_zone=%.1f\r\n", 
		pedometer->vo2max, pedometer->hr_intensity, pedometer->hr_zone);
	DBG("\r\n");
	
	DBG("[biking] lat=%.1f lon=%.1f distance=%.1f calories=%.1f\r\n", 
		biking->latitude, biking->longitude, biking->distance, biking->calories);
	DBG("  speed: cur=%.1f avg=%.1f max=%.1f\r\n", biking->cur_speed, biking->avg_speed, biking->max_speed);
	DBG(" elevat: slope=%.1f elev=%.1f\r\n", biking->slope, biking->elevation);
	DBG("        hr: vo2max=%.1f hr_inten=%.1f hr_zone=%.1f\r\n", 
		biking->vo2max, biking->hr_intensity, biking->hr_zone);
	DBG("\r\n");
}
