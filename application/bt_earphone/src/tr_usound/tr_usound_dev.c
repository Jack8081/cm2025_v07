/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief tr_usound media
 */
#include "tr_usound.h"

#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif

struct _le_dev_t *tr_usound_dev_add(uint16_t handle)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();

    if(tr_usound) {
        for(int i = 0; i < ARRAY_SIZE(tr_usound->dev); i++) {
            if(tr_usound->dev[i].handle == 0) {
                tr_usound->dev[i].handle = handle;
                tr_usound->dev_num++;
                return &tr_usound->dev[i];
            }
        }
    }

    SYS_LOG_ERR("\n");
	return NULL;
}

int tr_usound_dev_del(uint16_t handle)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();

    if(tr_usound) {
        for(int i = 0; i < ARRAY_SIZE(tr_usound->dev); i++) {
            if(tr_usound->dev[i].handle == handle) {
                memset(&tr_usound->dev[i], 0, sizeof(struct _le_dev_t));
                tr_usound->dev_num--;
                return 0;
            }
        }
    }

    SYS_LOG_ERR("\n");
	return -EAGAIN;
}

int tr_usound_get_dev_num(void)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();

    if(tr_usound) {
        return tr_usound->dev_num;
    }

	return 0;
}

/*@brief add bt audio chan
 *
 *@param dir 0:source   1:sink
 *@return success chan : other null
 */
struct bt_audio_chan *tr_usound_dev_add_chan(uint16_t handle, uint8_t dir, uint8_t id)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();

    if(tr_usound) {
        for(int i = 0; i < ARRAY_SIZE(tr_usound->dev); i++) {
            if(tr_usound->dev[i].handle == handle) {
                if(dir) {
                    tr_usound->dev[i].chan[MIC_CHAN].handle = handle;
                    tr_usound->dev[i].chan[MIC_CHAN].id = id;
                    return &tr_usound->dev[i].chan[MIC_CHAN];
                }
                else {
                    tr_usound->dev[i].chan[SPK_CHAN].handle = handle;
                    tr_usound->dev[i].chan[SPK_CHAN].id = id;
                    return &tr_usound->dev[i].chan[SPK_CHAN];
                }
            }
        }
    }

    SYS_LOG_ERR("\n");
	return NULL;
}

/*@brief find bt audio dev
 *
 *@return find result or NULL;
 */
struct _le_dev_t *tr_usound_find_dev(uint16_t handle)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();

    if(tr_usound) {
        for(int i = 0; i < ARRAY_SIZE(tr_usound->dev); i++) {
            if(tr_usound->dev[i].handle == handle) {
                return &tr_usound->dev[i];
            }
        }
    }

	return NULL;
}

/*@brief find bt audio chan
 *
 *@param dir 0:source(SKP)   1:sink(MIC)
 *@return find result or NULL;
 */
struct bt_audio_chan *tr_usound_find_chan_by_dir(uint16_t handle, uint8_t dir)
{
    struct tr_usound_app_t *tr_usound = tr_usound_get_app();

    if(tr_usound) {
        for(int i = 0; i < ARRAY_SIZE(tr_usound->dev); i++) {
            if((handle == 0 && tr_usound->dev[i].handle && tr_usound->dev[i].chan[dir].handle) 
                    || ( handle && tr_usound->dev[i].handle == handle && tr_usound->dev[i].chan[dir].handle)) {
                return &tr_usound->dev[i].chan[dir];
            }
        }
    }
    return NULL;
}

struct bt_audio_chan *tr_usound_find_chan_by_id(uint16_t handle, uint8_t id)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();

    if(tr_usound) {
        for(int i = 0; i < ARRAY_SIZE(tr_usound->dev); i++) {
            if(tr_usound->dev[i].handle == handle) {
                if(tr_usound->dev[i].chan[MIC_CHAN].id == id) {
                    return &tr_usound->dev[i].chan[MIC_CHAN];
                }
                else if(tr_usound->dev[i].chan[SPK_CHAN].id == id) {
                    return &tr_usound->dev[i].chan[SPK_CHAN];
                }
            }
        }
    }

	return NULL;
}
/*@brief find bt audio dir by chan id
 *
 *@return dir 0:source(SKP)   1:sink(MIC)
 */
uint8_t tr_usound_get_chan_dir(uint16_t handle, uint8_t id)
{
	struct tr_usound_app_t *tr_usound = tr_usound_get_app();

    if(tr_usound) {
        for(int i = 0; i < ARRAY_SIZE(tr_usound->dev); i++) {
            if(tr_usound->dev[i].handle == handle) {
                if(tr_usound->dev[i].chan[MIC_CHAN].handle == handle && tr_usound->dev[i].chan[MIC_CHAN].id == id) {
                    return MIC_CHAN;
                }
                else if(tr_usound->dev[i].chan[SPK_CHAN].handle == handle && tr_usound->dev[i].chan[SPK_CHAN].id == id) {
                    return SPK_CHAN;
                }
            }
        }
    }

	return 0xff;
}
