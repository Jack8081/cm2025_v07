/*******************************************************************************
 * @file    hr_algo_data.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2021-5-25
 * @brief   sensor algorithm data
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <hr_algo.h>

/******************************************************************************/
//variables
/******************************************************************************/

/* HR algorithm api */
const hr_algo_api_t hr_algo_api __attribute__((used, section("RESET"))) = 
{
	.init = hr_algo_init,
	.get_os_api = hr_algo_get_os_api,
	.set_os_api = hr_algo_set_os_api,
	.start = hr_algo_start,
	.stop = hr_algo_stop,
	.process = hr_algo_process,
};
