/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA upgrade interface
 */

#ifndef __OTA_UPGRADE_H__
#define __OTA_UPGRADE_H__

struct ota_upgrade_info;
struct ota_backend;

/** ota state */
enum ota_state
{
	OTA_INIT = 0,
	OTA_RUNNING,
	OTA_DONE,
	OTA_FAIL,
};

typedef void (*ota_notify_t)(int state, int old_state);
typedef void (*ota_ugrade_file_cb)(uint8_t file_id);

/** structure of ota upgrade param, Initialized by the user*/
struct ota_upgrade_param {
	const char *storage_name;
#ifdef CONFIG_OTA_MUTIPLE_STORAGE
	const char *storage_ext_name;
#endif
	ota_notify_t notify;
	ota_ugrade_file_cb file_cb;

	/* flags */
	unsigned int flag_use_recovery:1;	/* use recovery */
	unsigned int flag_use_recovery_app:1;	/* use recovery app */
	unsigned int no_version_control:1; /* OTA without version control */
	unsigned int flag_erase_part_for_upg:1; /* OTA upgrade erase part for upg*/
};

/**
 * @brief ota upgrade init funcion
 *
 * This routine calls init ota upgrade,called by ota app
 *
 * @param param init ota struct
 *
 * @return pointer to ota struct address.
 * @return NULL if init failed.
 */

struct ota_upgrade_info *ota_upgrade_init(struct ota_upgrade_param *param);

/**
 * @brief ota check and upgrade funcion
 *
 * This routine calls to do ota upgrade ,called by ota app
 *
 * @param ota pointer to the ota upgrade structure
 *
 * @return 0 if upgrade success.
 * @return others if init failed.
 */

int ota_upgrade_check(struct ota_upgrade_info *ota);

/**
 * @brief check the progress of ota
 *
 * This routine calls to check ota progress ,called by recovery_main
 *
 * @param ota pointer to the ota upgrade structure
 *
 * @return 0 if no in progress.
 * @return 1 if in progress.
 */

int ota_upgrade_is_in_progress(struct ota_upgrade_info *ota);

/**
 * @brief attach backend to ota
 *
 * This routine calls to attach backend to ota ,called by ota app
 *
 * @param ota pointer to the ota upgrade structure
 * @param backend pointer to backend structure
 * @return 0 if attach success.
 * @return others if attach fail.
 */

int ota_upgrade_attach_backend(struct ota_upgrade_info *ota, struct ota_backend *backend);

/**
 * @brief detach backend to ota
 *
 * This routine calls to detach backend to ota ,called by ota app for the end of ota
 *
 * @param ota pointer to the ota upgrade structure
 * @param backend pointer to backend structure
 */

void ota_upgrade_detach_backend(struct ota_upgrade_info *ota, struct ota_backend *backend);

#ifdef CONFIG_SPPBLE_DATA_CHAN
int tr_ota_read_data(struct ota_backend *backend, uint8_t *buf, int size);
int tr_ota_write_data(struct ota_backend *backend, uint8_t *buf, int size);
#endif
#endif /* __OTA_UPGRADE_H__ */
