/******************** (C) COPYRIGHT 2020 Actions-semi ********************
 * File Name          : sensor_algo_sleep.s
 * Author             : Lark Team
 * Version            : V1.0
 * Date               : 2021-05-25
 * Description        : Include sensor algorithm bin. 
 *************************************************************************/
                .section .sensor_sleep_rom,"a"
				.align 12
				.global __sensor_algo_sleep_start
				.global __sensor_algo_sleep_end
__sensor_algo_sleep_start:
				.incbin "lark_dvb_motion_sleep.bin"
__sensor_algo_sleep_end:
				.align 12
				.end
                PRESERVE8
                END
