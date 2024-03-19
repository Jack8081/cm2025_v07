#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "ui event"
	
#include <os_common_api.h>

#include <mem_manager.h>
#include <msg_manager.h>
#include <ui_manager.h>
#include "app_common.h"
#include "app_ui.h"
#include <tts_manager.h>


/*!
 * \brief 状态事件 LED 显示 (不播放提示音或语音)
 * \n  ui_event : CFG_TYPE_SYS_EVENT
 */
void ui_event_led_display(uint32_t ui_event)
{
	struct app_msg	msg = {0};

	if (ui_event != 0) {
		SYS_LOG_INF("ui_event %d\n", ui_event);
		memset(&msg,0,sizeof(struct app_msg));
		msg.type = MSG_APP_UI_EVENT;
		msg.cmd = ui_event;
		msg.reserve = 1;
    	send_async_msg("main", &msg);
	}
}



/*!
 * \brief 事件通知
 * \n  ui_event : CFG_TYPE_SYS_EVENT 
 * \n  wait_end : 是否等待通知结束 (播放完提示音和语音)
 */

void ui_event_notify(uint32_t ui_event, void* param, bool wait_end)
{
	struct app_msg	msg = {0};

	if (ui_event != 0)
	{
		SYS_LOG_INF("ui_event %d\n", ui_event);	
		memset(&msg,0,sizeof(struct app_msg));
		msg.type = MSG_APP_UI_EVENT;
		msg.cmd = ui_event;
		msg.ptr = param;
    	send_async_msg("main", &msg);
	}

	if (wait_end)
		tts_manager_wait_finished(true);
}


void sys_manager_event_notify(uint32_t sys_event, bool sync_wait)
{
    struct app_msg	msg = {0};

    msg.type = MSG_SYS_EVENT;
    msg.cmd = sys_event;
    msg.value = sync_wait? 1:0;
	
    send_async_msg("main", &msg);
}

