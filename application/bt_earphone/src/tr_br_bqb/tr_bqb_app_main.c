#include "tr_bqb_app.h"
#include <sys_wakelock.h>

#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif

static struct tr_bqb_app_t *p_bqb_app = NULL;

/*
 ** bqb 播歌功能初始化
 */
static int tr_bqb_init(void)
{
    int ret = 0;

    p_bqb_app = app_mem_malloc(sizeof(struct tr_bqb_app_t));
    if (!p_bqb_app) {
        SYS_LOG_ERR("bqb malloc fail!");
        ret = -ENOMEM;
        return ret;
    }

    memset(p_bqb_app, 0, sizeof(struct tr_bqb_app_t));
    tr_bqb_view_init();
	api_bt_auto_move_keep_stop(1);
	bt_manager_set_connectable(true);
//    bt_manager_set_visual_mode_transmitter(TRANS_VISUAL_MODE_DISC_CON);

//    bt_manager_set_codec_info(0, BQB_SAMPLE_RATE);

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
    dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "media");
    p_bqb_app->need_resample = 1;
#endif
    sys_wake_lock(FULL_WAKE_LOCK);
   SYS_LOG_INF(" -- ok-- ");

    return ret;
}

/*
 ** 蓝牙推歌功能退出
 */
void tr_bqb_exit(void)
{
    SYS_LOG_INF("%s %d", __func__, __LINE__);

    if (!p_bqb_app) {
        goto exit;
    }

	thread_timer_stop(&p_bqb_app->monitor_timer);

  //  _bqb_store_play_state();

//    bqb_media_exit();

//    nvram_cache_flush(NULL);

    //释放资源
	tr_bqb_view_deinit();
    app_mem_free(p_bqb_app);
    p_bqb_app = NULL;
    sys_wake_unlock(FULL_WAKE_LOCK);
exit:
#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
	if (p_bqb_app->need_resample) {
		dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "media");
	}
#endif
    app_manager_thread_exit(app_manager_get_current_app());
}
/*
static void bqb_restore_play_state(u8_t init_play_state)
{
#ifdef CONFIG_ESD_MANAGER
    if(esd_manager_check_esd()) {
        esd_manager_restore_scene(TAG_PLAY_STATE,&init_play_state, 1);
    }
#endif

    SYS_LOG_INF("line_sta:%d", init_play_state);

    //进入bqb统一默认成暂停状态
    if (init_play_state != BQB_STATUS_PLAYING) {
        p_bqb_app->playing = 0;
		
    }
	//else{
        //p_bqb_app->playing = 1;
        //bqb_media_init();
    //}
	uint32_t curr_status = bt_manager_get_status_transmitter();
	if((curr_status == BT_STATUS_PAUSED) || (curr_status == BT_STATUS_PLAYING))
	{
		p_bqb_app->playing = 1;
        bqb_media_init();
	}
		
}
*/
/*
 ** bqb 应用主函数
 */
void tr_bqb_main_loop(void *parama1, void *parama2, void *parama3)
{
    struct app_msg msg = { 0 };
    int result = 0;
    bool terminated = false;
    printk("***************************************************\n");
    printk("********************* tr_br_bqb *******************\n");
    printk("***************************************************\n");
	
    result = tr_bqb_init();
    if (result != 0) {
        SYS_LOG_ERR("%s %d: bqb init fail!", __func__, __LINE__);
        while (1) {
            ; //empty
        }
    }

    while (!terminated) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			SYS_LOG_INF("type %d, value %d\n", msg.type, msg.value);
			switch (msg.type) {
			case MSG_INPUT_EVENT:
				tr_bqb_input_event_proc(&msg);
				break;
			case MSG_BT_EVENT:
				tr_bqb_bt_event_proc(&msg);
				break;
			case MSG_EXIT_APP:
				tr_bqb_exit();
				terminated = true;
				break;
			default:
				break;
			}
			if (msg.callback)
				msg.callback(&msg, 0, NULL);
		}
		if (!terminated)
			thread_timer_handle_expired();
	}


}

struct tr_bqb_app_t *tr_bqb_get_app(void)
{
    return p_bqb_app;
}

APP_DEFINE(tr_br_bqb, share_stack_area, sizeof(share_stack_area),CONFIG_APP_PRIORITY, 
    FOREGROUND_APP, NULL, NULL, NULL,
    tr_bqb_main_loop, NULL);

