/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef	_ACTIONS_SOC_ATP_H_
#define	_ACTIONS_SOC_ATP_H_

#ifndef _ASMLANGUAGE

/**
 * @brief atp get rf calib function
 *
 * This function is used to obtain RF calibration parameters
 *
 * @param id 0: get power table version bit
 *           1: get rf tx PGA offset
 *           2: get avdd_if adjust param
 *           3: get rf phase offset, reserved
 *           4: get rf.c 0x0E register for version C
 *          other value: invalid
 *
 * @param calib_val: return calib value
 * @return 0 if invoked succsess.
 * @return -1 if invoked failed.
 */
extern int soc_atp_get_rf_calib(int id, unsigned int *calib_val);

/**
 * @brief atp get pmu calib function
 *
 * This function is used to obtain pmu calibration parameters
 *
 * @param id 0: get Charger CV param
 *           1: get Charger CC param
 *           2: get 10ua bias current calib param
 *           3: get 100Kohm param
 *           4: get batadc param
 *           5: get sensoradc param
 *           6: get chargei calibration param
 *           7: get vd12 offset param
 *          other value:invalid
 * @param calib_val: return calib value
 * @return 0 if invoked succsess.
 * @return -1 if invoked failed.
 */
extern int soc_atp_get_pmu_calib(int id, unsigned int *calib_val);

/**
 * @brief atp get hosc calib function
 *
 * This function is used to obtain hosc calibration parameters
 *
 * @param id 0: get array0 param
 *           1: get array1 param
 *          other value:invalid
 * @param calib_val: return calib value
 * @return 0 if invoked succsess.
 * @return -1 if invoked failed.
 */
extern int soc_atp_get_hosc_calib(int id, unsigned int *calib_val);

/**
 * @brief atp get dac calib function
 *
 * This function is used to obtain dac calibration parameters
 *
 * @param id 0: get dac L offset param
 *           1: get dac R offset param
 *          other value:invalid
 * @param calib_val: return calib value
 * @return 0 if invoked succsess.
 * @return -1 if invoked failed.
 */
extern int soc_atp_get_dac_calib(int id, unsigned int *calib_val);


/**
 * @brief atp get fcc param
 *
 * This function is used to obtain fcc parameters
 *
 * @param id 0: get array0 param
 *           1: get array1 param
 *          other value:invalid
 * @param param_val: return param value
 * @return 0 if invoked succsess.
 * @return -1 if invoked failed.
 */
extern int soc_atp_get_fcc_param(int id, unsigned int *param_val);


/**
 * @brief atp get wafer info
 *
 * This function is used to obtain wafer info
 *
 * @param 
 * @param param_val: return param value
 *        0: TT/SS2 wafer; 1: FF2/FF3 wafer
 * @return 0 if invoked succsess.
 * @return -1 if invoked failed.
 */
extern int soc_atp_get_wafer_info(unsigned int *param_val);


#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_ATP_H_	*/
