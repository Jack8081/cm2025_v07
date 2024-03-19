#ifndef _SYSTEM_DC5V_IO_CMD_H_
#define _SYSTEM_DC5V_IO_CMD_H_

/* 产测命令完成后进行提示, 
 * 选择是否在通讯断开后关机 (从产测板盒中取出)
 */
void system_dc5v_io_cmd_complete(bool wait_powoff);

/* 产测进入 BQB 测试模式, 重启后自动进入
 * bqb_mode : CFG_TYPE_BT_CTRL_TEST_MODE
 */
void dc5v_io_cmd_enter_bqb_mode(u32_t bqb_mode);

/* 产测清除配对列表
 * clear_tws : 是否清除tws
 */
void dc5v_io_cmd_clear_paired_list(u32_t clear_tws);

#define TWS_PAIR_NONE      0
#define TWS_PAIR_PENDING   1
#define TWS_PAIR_OK        2
#define TWS_PAIR_ABORT     3
/* 产测 TWS 组对, 将放入同一产测板盒的两只耳机进行组对,
 * 多个产测板盒可以设置不同组对命令, 对应不同的组对匹配关键字,
 * 可以避免不同板盒内的耳机错误配对问题,
 * 耳机组对成功后从产测板盒取出自动关机
 */
int dc5v_io_cmd_enter_tws_pair_mode(u32_t keyword);

void dc5v_io_cmd_enter_pair_mode(void);

/* 充电盒开盖, 开盖时可以不发送开盖命令,
 * 而直接发送电量命令替代开盖命令以能快速获取充电盒电池电量
 */
void dc5v_io_cmd_chg_box_opened(u32_t bat_level);

/* 充电盒关盖
 */
void dc5v_io_cmd_chg_box_closed(void);

/* 充电盒自身电池低电
 */
void dc5v_io_cmd_chg_box_bat_low(void);

/* pressed 可定义充电盒按键操作
 * 1 : 短按
 * 2 : 长按
 * 3 : 双击
 * 4 : 超长按
 */
void dc5v_io_cmd_chg_box_key_pressed(u32_t pressed);

void dc5v_io_cmd_enter_ota_mode(void);

void dc5v_io_cmd_search_ble_adv(void);

void check_dc5v_io_cmd_search_ble_adv_finish_timer(struct thread_timer *ttimer, void *expiry_fn_arg);

#endif