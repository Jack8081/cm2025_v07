/*******************************************************************************
 * @file    hr_algo_data.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2021-5-25
 * @brief   hr algorithm data
*******************************************************************************/

#ifndef _HR_ALGO_DATA_H
#define _HR_ALGO_DATA_H

/******************************************************************************/
//includes
/******************************************************************************/
#include <hr_algo.h>

/******************************************************************************/
//constants
/******************************************************************************/

#define _data_rom_start			Load$$RW_RAM$$Base
#define _data_rom_end				Load$$RW_RAM$$Limit
#define _data_ram_start			Image$$RW_RAM$$Base
#define _data_ram_end				Image$$RW_RAM$$Limit

#define _bss_start					Image$$RW_RAM$$ZI$$Base 
#define _bss_end						Image$$RW_RAM$$ZI$$Limit

#define _heap_start					Image$$ARM_LIB_HEAP$$ZI$$Base 
#define _heap_end						Image$$ARM_LIB_HEAP$$ZI$$Limit

/******************************************************************************/
//typedef
/******************************************************************************/

/******************************************************************************/
//variable
/******************************************************************************/
extern char _data_rom_start[];
extern char _data_rom_end[];
extern char _data_ram_start[];
extern char _data_ram_end[];

extern char _bss_start[];
extern char _bss_end[];

extern char _heap_start[];
extern char _heap_end[];

/* Sensor algorithm api */
extern const hr_algo_api_t hr_algo_api;

/******************************************************************************/
//function
/******************************************************************************/

#endif  /* _HR_ALGO_DATA_H */

