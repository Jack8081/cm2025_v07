
#include <property_manager.h>
#include "os_common_api.h"
#include "bt_manager.h"
#include "app_manager.h"
#include "app_ui.h"
#include "consumer_eq.h"

#define CONSUMER_EQ_NAME  "consumer_eq"
#define SPEAKER_EQ_NAME  "speaker_eq"

OS_MUTEX_DEFINE(consumer_eq_mutex);

#define EFFECT_EQ_POS  (0)
#define EFFECT_EQ_NUM  (5)

#define CONSUME_EQ_POS  (5)
#define SPEAKER_EQ_POS  (15)

typedef struct
{
    short reserve_0[16];//只读
    peq_band_t peq_bands[20];
    short peq_bands_enable[20];//统一设置为1，for enable
    short reserve_2[140];//只读
}eq_info_t;


typedef struct
{
    consumer_eq_t consumer_eq;
    uint32_t version;
}consumer_eq_v_t;


typedef struct
{
    uint32_t first_time:1;
    uint32_t effect_eq_enable:1;
    uint32_t speaker_eq_enable:1;
    uint32_t reserved:29;
    speaker_eq_t *speaker_eq;
}eq_config_t;

static eq_config_t eq_config = {
    .first_time = 1,
    .effect_eq_enable = 1,
    .speaker_eq_enable = 1,
    .speaker_eq = NULL,
};


int32_t consumer_eq_get_param(consumer_eq_t *eq)
{
    int32_t ret;

    if(NULL == eq) {
        SYS_LOG_ERR("invalid param");
        return -1;
    }

    os_mutex_lock(&consumer_eq_mutex, OS_FOREVER);
    ret = property_get(CONSUMER_EQ_NAME, (char*)eq, sizeof(consumer_eq_t));
    os_mutex_unlock(&consumer_eq_mutex);
    
    if(ret != sizeof(consumer_eq_t)) {
        return -1;
    }
    
    return 0;
}

int32_t consumer_eq_set_param(consumer_eq_t *eq)
{
    int32_t ret;
    consumer_eq_v_t *consumer_eq_v;

    printk("sssssss line=%d, func=%s\n", __LINE__, __func__);
    consumer_eq_v = mem_malloc(sizeof(consumer_eq_v_t));
    if(NULL == consumer_eq_v) {
        SYS_LOG_ERR("malloc fail");
        return -1;
    }
    
    os_mutex_lock(&consumer_eq_mutex, OS_FOREVER);

    ret = property_get(CONSUMER_EQ_NAME, (char*)consumer_eq_v, sizeof(consumer_eq_v_t));
    if(ret != sizeof(consumer_eq_v_t)) {
        consumer_eq_v->version = 0;
    }

    consumer_eq_v->version++;
    memcpy(&consumer_eq_v->consumer_eq, eq, sizeof(consumer_eq_t));
    
    property_set(CONSUMER_EQ_NAME, (char*)consumer_eq_v, sizeof(consumer_eq_v_t));
    os_mutex_unlock(&consumer_eq_mutex);

    //Sync to another earphone
    bt_manager_tws_send_message(TWS_LONG_MSG_EVENT, TWS_EVENT_CONSUMER_EQ, (uint8_t*)consumer_eq_v, sizeof(consumer_eq_v_t));

    char *current_app = app_manager_get_current_app();
    if (current_app) {
        struct app_msg msg={0};
        msg.type = MSG_APP_UPDATE_MUSIC_DAE;
        send_async_msg(current_app, &msg);
    }

    mem_free(consumer_eq_v);
    return 0;
}

int32_t consumer_eq_patch_param(void *eq_info, int32_t size)
{
    eq_info_t *eq = (eq_info_t*)eq_info;
    int32_t ret;
    int32_t i;

    if((NULL == eq_info) || (size != sizeof(eq_info_t))) {
        return -1;
    }

    os_mutex_lock(&consumer_eq_mutex, OS_FOREVER);

    if(eq_config.effect_eq_enable) {
        ret = property_get(CONSUMER_EQ_NAME, (char*)&eq->peq_bands[CONSUME_EQ_POS], sizeof(consumer_eq_t));
        if(ret == sizeof(consumer_eq_t)) {
            for(i=CONSUME_EQ_POS; i<(CONSUMER_EQ_NUM+CONSUME_EQ_POS); i++) {
                eq->peq_bands_enable[i] = 1;
            }
        }
    } else {
        for(i=CONSUME_EQ_POS; i<(CONSUMER_EQ_NUM+CONSUME_EQ_POS); i++) {
            eq->peq_bands_enable[i] = 0;
        }
        for(i=EFFECT_EQ_POS; i<(EFFECT_EQ_NUM+EFFECT_EQ_POS); i++) {
            eq->peq_bands_enable[i] = 0;
        }
    }

    if(eq_config.speaker_eq_enable) {
        if(eq_config.speaker_eq) {
            memcpy(&eq->peq_bands[SPEAKER_EQ_POS], eq_config.speaker_eq, sizeof(speaker_eq_t));
            ret = sizeof(speaker_eq_t);
        } else {
            ret = property_get(SPEAKER_EQ_NAME, (char*)&eq->peq_bands[SPEAKER_EQ_POS], sizeof(speaker_eq_t));
        }
        
        if(ret == sizeof(speaker_eq_t)) {
            for(i=SPEAKER_EQ_POS; i<(SPEAKER_EQ_NUM+SPEAKER_EQ_POS); i++) {
                eq->peq_bands_enable[i] = 1;
            }
        }
    } else {
        for(i=SPEAKER_EQ_POS; i<(SPEAKER_EQ_NUM+SPEAKER_EQ_POS); i++) {
            eq->peq_bands_enable[i] = 0;
        }
    }
    
    os_mutex_unlock(&consumer_eq_mutex);
    return 0;
}

int32_t consumer_eq_tws_set_param(uint8_t *data_buf, int32_t len)
{
    int32_t ret;
    int32_t need_update = 0;
    consumer_eq_v_t *consumer_eq_v = (consumer_eq_v_t*)data_buf;
    consumer_eq_v_t *consumer_eq_l;

    if((NULL == consumer_eq_v) || (len != sizeof(consumer_eq_v_t))) {
        SYS_LOG_ERR("invalid param");
        return -1;
    }

    consumer_eq_l = mem_malloc(sizeof(consumer_eq_v_t));
    if(NULL == consumer_eq_l) {
        SYS_LOG_ERR("malloc fail");
        return -1;
    }
    
    os_mutex_lock(&consumer_eq_mutex, OS_FOREVER);
    ret = property_get(CONSUMER_EQ_NAME, (char*)consumer_eq_l, sizeof(consumer_eq_v_t));
    if(ret != sizeof(consumer_eq_v_t)) {
        consumer_eq_l->version = consumer_eq_v->version - 1;
    }

    printk("sssssss line=%d, func=%s, version: local: %u, remote: %u\n",
        __LINE__, __func__, consumer_eq_l->version, consumer_eq_v->version);
    if(consumer_eq_l->version == consumer_eq_v->version) {
        ; //do nothing
    } else if((consumer_eq_v->version - consumer_eq_l->version) < 0xFFFF) {
        // remote newer
        property_set(CONSUMER_EQ_NAME, (char*)consumer_eq_v, sizeof(consumer_eq_v_t));
        need_update = 1;
    } else if(eq_config.first_time){
        // local newer
        //Sync to another earphone
        eq_config.first_time = 0;
        bt_manager_tws_send_message(TWS_LONG_MSG_EVENT, TWS_EVENT_CONSUMER_EQ, (uint8_t*)consumer_eq_l, sizeof(consumer_eq_v_t));
    }
    
    os_mutex_unlock(&consumer_eq_mutex);

    if(need_update) {
        char *current_app = app_manager_get_current_app();
        if (current_app) {
            struct app_msg msg={0};
            msg.type = MSG_APP_UPDATE_MUSIC_DAE;
            send_async_msg(current_app, &msg);
        }
    }
    
    mem_free(consumer_eq_l);
    return 0;
}

int32_t consumer_eq_on_tws_connect(void)
{
    int32_t ret;
    consumer_eq_v_t *consumer_eq_v;

    if(eq_config.first_time == 0) {
        return 0;
    }

    eq_config.first_time = 0;
    printk("sssssss line=%d, func=%s, on tws connect\n", __LINE__, __func__);
    consumer_eq_v = mem_malloc(sizeof(consumer_eq_v_t));
    if(NULL == consumer_eq_v) {
        SYS_LOG_ERR("malloc fail");
        return -1;
    }
    
    os_mutex_lock(&consumer_eq_mutex, OS_FOREVER);

    ret = property_get(CONSUMER_EQ_NAME, (char*)consumer_eq_v, sizeof(consumer_eq_v_t));
    if(ret == sizeof(consumer_eq_v_t)) {
        //Sync to another earphone
        printk("sssssss line=%d, func=%s, update to remote\n", __LINE__, __func__);
        bt_manager_tws_send_message(TWS_LONG_MSG_EVENT, TWS_EVENT_CONSUMER_EQ, (uint8_t*)consumer_eq_v, sizeof(consumer_eq_v_t));
    }

    os_mutex_unlock(&consumer_eq_mutex);

    mem_free(consumer_eq_v);
    return 0;
}

int32_t effect_eq_close(void)
{
    os_mutex_lock(&consumer_eq_mutex, OS_FOREVER);
    eq_config.effect_eq_enable = 0;
    os_mutex_unlock(&consumer_eq_mutex);

    //update effect
    char *current_app = app_manager_get_current_app();
    if (current_app) {
        struct app_msg msg={0};
        msg.type = MSG_APP_UPDATE_MUSIC_DAE;
        send_async_msg(current_app, &msg);
    }
    return 0;
}

int32_t speaker_eq_close(void)
{
    os_mutex_lock(&consumer_eq_mutex, OS_FOREVER);

    eq_config.speaker_eq_enable = 0;
    if(eq_config.speaker_eq) {
        mem_free(eq_config.speaker_eq);
        eq_config.speaker_eq = NULL;
    }

    os_mutex_unlock(&consumer_eq_mutex);


    //update effect
    char *current_app = app_manager_get_current_app();
    if (current_app) {
        struct app_msg msg={0};
        msg.type = MSG_APP_UPDATE_MUSIC_DAE;
        send_async_msg(current_app, &msg);
    }
    return 0;
}

int32_t speaker_eq_set_param(speaker_eq_t *eq, bool save)
{
    printk("sssssss line=%d, func=%s\n", __LINE__, __func__);
    os_mutex_lock(&consumer_eq_mutex, OS_FOREVER);

    if(save) {
        property_set_factory(SPEAKER_EQ_NAME, (char*)eq, sizeof(speaker_eq_t));
        eq_config.speaker_eq_enable = 1;
        if(eq_config.speaker_eq) {
            mem_free(eq_config.speaker_eq);
            eq_config.speaker_eq = NULL;
        }
    } else {
        if(!eq_config.speaker_eq) {
            eq_config.speaker_eq = mem_malloc(sizeof(speaker_eq_t));
        }
        if(eq_config.speaker_eq) {
            eq_config.speaker_eq_enable = 1;
            memcpy(eq_config.speaker_eq, eq, sizeof(speaker_eq_t));
        }
    }

    os_mutex_unlock(&consumer_eq_mutex);


    //update effect
    char *current_app = app_manager_get_current_app();
    if (current_app) {
        struct app_msg msg={0};
        msg.type = MSG_APP_UPDATE_MUSIC_DAE;
        send_async_msg(current_app, &msg);
    }

    return 0;
}


