/******************** (C) COPYRIGHT 2020 Actions-semi ********************
 * File Name          : hr_algo.s
 * Author             : Lark Team
 * Version            : V1.0
 * Date               : 2021-05-25
 * Description        : Include sensor algorithm bin. 
 *************************************************************************/
                .section .hr_rom,"a"
				.align 12
				.global __hr_algo_start
				.global __hr_algo_end
__hr_algo_start:
				.incbin "lark_dvb_hr.bin"
__hr_algo_end:
				.align 12
				.end
                PRESERVE8
                END
