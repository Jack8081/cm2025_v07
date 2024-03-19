/******************** (C) COPYRIGHT 2020 Actions-semi ********************
 * File Name          : sensor_algo.s
 * Author             : Lark Team
 * Version            : V1.0
 * Date               : 2021-05-25
 * Description        : Include sensor algorithm bin. 
 *************************************************************************/
                .section .sensor_rom,"a"
				.align 12
				.global __sensor_algo_start
				.global __sensor_algo_end
__sensor_algo_start:
				.incbin "lark_dvb_motion.bin"
__sensor_algo_end:
				.align 12
				.end
                PRESERVE8
                END
