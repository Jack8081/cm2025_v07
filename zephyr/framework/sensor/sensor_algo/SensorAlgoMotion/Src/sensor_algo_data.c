/*******************************************************************************
 * @file    sensor_algo_data.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2021-5-25
 * @brief   sensor algorithm data
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <sensor_algo.h>

/******************************************************************************/
//variables
/******************************************************************************/

/* Sensor algorithm api */
const sensor_algo_api_t sensor_algo_api __attribute__((used, section("RESET"))) = 
{
	.init = sensor_algo_init,
	.get_os_api = sensor_algo_get_os_api,
	.set_os_api = sensor_algo_set_os_api,
	.control = sensor_algo_control,
	.enable = sensor_algo_enable,
	.disable = sensor_algo_disable,
	.input = sensor_algo_input,
	.input_fifo_init = sensor_algo_input_fifo_init,
	.input_fifo_start = sensor_algo_input_fifo_start,
	.input_fifo_end = sensor_algo_input_fifo_end,
	.get_next_duration = sensor_algo_get_next_duration,
	.process = sensor_algo_process,
	.get_result = sensor_algo_get_result,
	.dump_event = sensor_algo_dump_event,
	.dump_result = sensor_algo_dump_result,
};

/* Sensor algorithm result */
sensor_res_t sensor_algo_res = {0};
