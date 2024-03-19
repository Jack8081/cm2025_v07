/*******************************************************************************
 * @file    hr_algo.c
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
#include <hr_algo.h>
#include <hr_algo_data.h>
#include "gh3011_example.h"

/******************************************************************************/
//constants
/******************************************************************************/
#define DBG(...)			os_api.dbgOutput(__VA_ARGS__)
//#define DBG(...)			do {} while (0)

/******************************************************************************/
//variables
/******************************************************************************/
/* hr os api */
static hr_os_api_t os_api = {0};

/******************************************************************************/
//functions
/******************************************************************************/
/* Init sensor algo */
int hr_algo_init(const hr_os_api_t *api)
{
	// init data section
  memcpy(_data_ram_start, _data_rom_start, (_data_rom_end - _data_rom_start));
	
	// init bss section
  memset(_bss_start, 0, (_bss_end - _bss_start));
	
	// init os api
	os_api = *api;
	
	return 0;
}

/* Get os api */
hr_os_api_t* hr_algo_get_os_api(void)
{
	return (hr_os_api_t*)&os_api;
}

/* Set os api */
int hr_algo_set_os_api(const hr_os_api_t *api)
{
	os_api = *api;
	return 0;
}

/* Start hr sensor */
int hr_algo_start(int mode)
{
	EMGh30xRunMode gh3011_mode;
	
	switch(mode) {
		case HB:
			gh3011_mode = GH30X_RUN_MODE_HB;
			break;
		case SPO2:
			gh3011_mode = GH30X_RUN_MODE_SPO2;
			break;
		case HRV:
			gh3011_mode = GH30X_RUN_MODE_HRV;
			break;
	}
	
	gh30x_module_init();
	GH30X_HBA_SCENARIO_CONFIG(0);
	gh30x_module_start(gh3011_mode);
	
	return 0;
}

/* Stop hr sensor */
int hr_algo_stop(void)
{
	gh30x_module_stop();
	return 0;
}

/* Process data through call-back handler */
int hr_algo_process(void)
{
	gh30x_int_msg_handler();
	return 0;
}
