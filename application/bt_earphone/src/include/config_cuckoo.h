
#ifndef __CONFIG_H__
#define __CONFIG_H__

/*!
 * \file
 * \brief     配置文件
 * \details
 * \author
 * \date
 * \copyright Actions
 */

#include <input_manager_type.h>

#define BOARD_LARK                          (4)
#define BOARD_CUCKOO                        (5)          
#define BOARD_FPGA                          (8)

#define CFG_SUPPORT_AAP_SETTINGS     1
#define CFG_OPTIMIZE_BT_MUSIC_STUCK  0

//#define BOARD_TYPE  BOARD_FPGA
#define BOARD_TYPE  BOARD_CUCKOO

/*-----------------------------------------------------------------------------
 * 配置文件中必须使用以下定义的 cfg_xxx 数据类型
 *---------------------------------------------------------------------------*/
typedef signed char   cfg_int8;
typedef signed short  cfg_int16;
typedef signed int  cfg_int32;

typedef unsigned char   cfg_uint8;
typedef unsigned short  cfg_uint16;
typedef unsigned int  cfg_uint32;

/*---------------------------------------------------------------------------*/


/* 常用数值定义
 */
#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#define ENABLE   TRUE
#define DISABLE  FALSE
#define OK       TRUE
#define FAIL     FALSE
#define NONE     0


/* IC 型号定义
 */
#define IC_TYPE_LARK    (1 << 14)
#define IC_TYPE_CUCKOO  (1 << 15)
#define IC_TYPE_GL6189  (1 << 11)


/* 当前选用的 IC 型号
 */
#define CFG_IC_TYPE  IC_TYPE_CUCKOO


/* 最大配置数定义
 */
#define CFG_MAX_USER_VERSION_LEN    32
#define CFG_MAX_CASE_NAME_LEN       20
#define CFG_MAX_RESERVED_SIZE       255
#define CFG_MAX_GPIO_PINS           79
#define CFG_MAX_LRADC_KEYS          9
#define CFG_MAX_LRADC_COMBO_KEYS    3
#define CFG_MAX_GPIO_KEYS           4
#define CFG_MAX_KEY_FUNC_MAPS       40
#define CFG_MAX_COMBO_KEY_MAPS      8
#define CFG_MAX_LEDS                4
#define CFG_MAX_LED_DISPLAY_MODELS  30
#define CFG_MAX_VOICES              28
#define CFG_MAX_NUMERIC_VOICES      10
#define CFG_MAX_TONES               10
#define CFG_MAX_VOICE_NAME_LEN      9
#define CFG_MAX_VOICE_FMT_LEN       5
#define CFG_MAX_TONE_NAME_LEN       9
#define CFG_MAX_TONE_FMT_LEN        5
#define CFG_MAX_EVENT_NOTIFY        45
#define CFG_MAX_BATTERY_LEVEL       10
#define CFG_MAX_BT_DEV_NAME_LEN     30
#define CFG_MAX_BT_SUFFIX_LEN       10
#define CFG_MAX_BT_PIN_CODE_LEN     6
#define CFG_MAX_BT_SUPPORT_DEVICES  3
#define CFG_MAX_BT_MUSIC_VOLUME     16
#define CFG_MAX_BT_CALL_VOLUME      15
#define CFG_MAX_LINEIN_VOLUME       16
#define CFG_MAX_VOICE_VOLUME        16
#define CFG_MAX_CAP_TEMP_COMP       20
#define CFG_MAX_PEQ_BANDS           14
#define CFG_MAX_UUID_STR_LEN        38
#define CFG_MAX_ADC_NUM             4
#define CFG_MAX_ANC_FILE            18
#define CFG_MAX_ANC_FMT_LEN         5
#define CFG_MAX_RF_TABLE_LEN        4

#define CFG_GPIO_MFP_PIN_DEF(_gpio_no, _mfp_sel)  \
    (_gpio_no << 0) |  \
    (_mfp_sel << 8)

/*-----------------------------------------------------------------------------
 * 配置枚举类型定义
 * 类型必须以 CFG_XXX 命名
 *---------------------------------------------------------------------------*/


/* 配置分类定义
 */
enum CFG_CATEGORY
{
    CFG_CATEGORY_SYSTEM,          // 系统
    CFG_CATEGORY_UPGRADE,         // 固件升级设置
    CFG_CATEGORY_DISPLAY,         // 显示
    CFG_CATEGORY_KEY,             // 按键
    CFG_CATEGORY_AUDIO,           // 音频
    CFG_CATEGORY_VOLUME_GAIN,     // 音量及增益
    CFG_CATEGORY_BATTERY_CHARGE,  // 电池及充电
    CFG_CATEGORY_EVENT_NOTIFY,    // 事件通知
    CFG_CATEGORY_VOICE,           // 语音
    CFG_CATEGORY_TONE,            // 提示音
    CFG_CATEGORY_LINEIN,          // Linein
    CFG_CATEGORY_APP_MUSIC,       // 本地音乐
    CFG_CATEGORY_CARD,            // 存储卡
    CFG_CATEGORY_USB,             // USB
    CFG_CATEGORY_BT_MUSIC,        // 蓝牙音乐
    CFG_CATEGORY_BT_CALL,         // 蓝牙通话
    CFG_CATEGORY_IG_CALL,         // 智能语音
    CFG_CATEGORY_BLUETOOTH,       // 蓝牙管理
    CFG_CATEGORY_BLE,             // BLE 管理
    CFG_CATEGORY_LE_AUDIO,        // LE AUDIO 管理   
    CFG_CATEGORY_ASET,            // 音效调节
    CFG_CATEGORY_ASQT,            // 通话调节
    CFG_CATEGORY_ANC,            // 主动降噪
};


enum CFG_TYPE_BOOL
{
    YES = 1,  // 是
    NO  = 0,  // 否
};


enum CFG_TYPE_SYS_SUPPORT_FEATURES
{
    SYS_ENABLE_DC5V_IN_RESET          = (1 << 1),  // 允许 DC5V 接入时复位
    SYS_ENABLE_DC5VPD_WHEN_DETECT_OUT = (1 << 2),  // 允许 DC5V 检测拔出时下拉
    SYS_FRONT_CHARGE_DC5V_OUT_REBOOT  = (1 << 3),  // 前台充电 DC5V 拔出时重启
    SYS_FORCE_CHARGE_WHEN_DC5V_IN     = (1 << 4),  // DC5V 接入时总是开启充电 (即使电池满电)
};


enum CFG_TYPE_AUTO_POWOFF_MODE
{
    AUTO_POWOFF_MODE_UNCONNECTED,  // 无连接
    AUTO_POWOFF_MODE_NOT_PLAYING,  // 无播放
};

enum CFG_TYPE_USB_DEVICE_MODE
{
    USB_DEVICE_MODE_CHARGE_ONLY,  // 只充电
    USB_DEVICE_MODE_CHARGE_STUB,  // 充电和stub   
    USB_DEVICE_MODE_CHARGE_UAC,  // 充电和uac 
    USB_DEVICE_MODE_ADFU_ONLY,  // adfu
};

enum CFG_TYPE_GPIO_PIN
{
    GPIO_PIN_NONE = 0xFF,
	GPIO_NONE = GPIO_PIN_NONE,

    GPIO_PIN_0 = 0,
    GPIO_PIN_1,
    GPIO_PIN_2,
    GPIO_PIN_3,
    GPIO_PIN_4,
    GPIO_PIN_5,
    GPIO_PIN_6,
    GPIO_PIN_7,
    GPIO_PIN_8,
    GPIO_PIN_9,
    GPIO_PIN_10,
    GPIO_PIN_11,
    GPIO_PIN_12,
    GPIO_PIN_13,
    GPIO_PIN_14,
    GPIO_PIN_15,
    GPIO_PIN_16,
    GPIO_PIN_17,
    GPIO_PIN_18,
    GPIO_PIN_19,
    GPIO_PIN_20,
    GPIO_PIN_21,
    GPIO_PIN_22,
    GPIO_PIN_23,
    GPIO_PIN_24,
    GPIO_PIN_25,
    GPIO_PIN_26,
    GPIO_PIN_27,
    GPIO_PIN_28,
    GPIO_PIN_29,
    GPIO_PIN_30,
};


enum CFG_TYPE_GPIO_LEVEL
{
    GPIO_LEVEL_HIGH = 1,  // 高电平
    GPIO_LEVEL_LOW  = 0,  // 低电平
};


enum CFG_TYPE_GPIO_PULL
{
    CFG_GPIO_PULL_NONE   = 0x0,  // 无
    CFG_GPIO_PULL_UP     = 0x8,  // 上拉 50K
    CFG_GPIO_PULL_UP_10K = 0x1,  // 上拉 10K
    CFG_GPIO_PULL_DOWN   = 0x2,  // 下拉 100K
};

enum CFG_TYPE_UART_TX_PIN
{
    UART_TX_GPIO_NONE = GPIO_NONE,
    UART_TX_GPIO_0     = CFG_GPIO_MFP_PIN_DEF(0,  2),
    UART_TX_GPIO_1     = CFG_GPIO_MFP_PIN_DEF(1,  2),
    UART_TX_GPIO_5     = CFG_GPIO_MFP_PIN_DEF(5,  2),
    UART_TX_GPIO_7     = CFG_GPIO_MFP_PIN_DEF(7,  2),
    UART_TX_GPIO_9     = CFG_GPIO_MFP_PIN_DEF(9,  2),
    UART_TX_GPIO_10    = CFG_GPIO_MFP_PIN_DEF(10, 2),
    UART_TX_GPIO_18    = CFG_GPIO_MFP_PIN_DEF(18, 2),
    UART_TX_GPIO_20    = CFG_GPIO_MFP_PIN_DEF(20, 2),
    UART_TX_GPIO_21    = CFG_GPIO_MFP_PIN_DEF(21, 2),
};


enum CFG_TYPE_UART_RX_PIN
{
    UART_RX_GPIO_NONE = GPIO_NONE,
    UART_RX_GPIO_2     = CFG_GPIO_MFP_PIN_DEF(2,  2),
    UART_RX_GPIO_6     = CFG_GPIO_MFP_PIN_DEF(6,  2),
    UART_RX_GPIO_8     = CFG_GPIO_MFP_PIN_DEF(8,  2),
    UART_RX_GPIO_17    = CFG_GPIO_MFP_PIN_DEF(17, 2),
    UART_RX_GPIO_19    = CFG_GPIO_MFP_PIN_DEF(19, 2),
    UART_RX_GPIO_22    = CFG_GPIO_MFP_PIN_DEF(22, 2),

};


enum CFG_TYPE_ONOFF_LONG_PRESS_RESET
{
    ONOFF_LONG_PRESS_RESET_DISABLE = 0xFF,  // 禁止长按复位

    ONOFF_LONG_PRESS_RESET_8S  = 0,  // 8 秒
    ONOFF_LONG_PRESS_RESET_16S = 1,  // 16 秒
};


enum CFG_TYPE_ONOFF_PRESS_POWER_ON
{
    ONOFF_PRESS_POWER_ON_SHORT   = 0,     // 短按开机

    ONOFF_PRESS_POWER_ON_250_MS  = 250,   // 250 毫秒
    ONOFF_PRESS_POWER_ON_500_MS  = 500,   // 500 毫秒
    ONOFF_PRESS_POWER_ON_1000_MS = 1000,  // 1 秒
    ONOFF_PRESS_POWER_ON_1500_MS = 1500,  // 1.5 秒
    ONOFF_PRESS_POWER_ON_2000_MS = 2000,  // 2 秒
    ONOFF_PRESS_POWER_ON_3000_MS = 3000,  // 3 秒
    ONOFF_PRESS_POWER_ON_4000_MS = 4000,  // 4 秒
};

enum CFG_TYPE_ONOFF_PULLR_SEL
{
    ONOFF_PULLR_SEL_UP    = 0,  // 内部50K上拉，低电平为按键按下
    ONOFF_PULLR_SEL_DOWN  = 1,  // 内部100K下拉，高定平为按键按下
};

enum CFG_TYPE_BOOT_HOLD_KEY_FUNC
{
    BOOT_HOLD_KEY_FUNC_NONE,               // 无
    BOOT_HOLD_KEY_FUNC_ENTER_PAIR_MODE,    // 进入配对模式
    BOOT_HOLD_KEY_FUNC_TWS_PAIR_SEARCH,    // TWS 组对搜索
    BOOT_HOLD_KEY_FUNC_AUTO_SELECT,        // 自动选择 TWS 组对搜索或配对模式
    BOOT_HOLD_KEY_FUNC_CLEAR_PAIRED_LIST,  // 清除配对列表
};


#define CFG_LRADC_CTRL_DEF(_ctrl_no, _gpio_no, _mfp_sel)  \
    (_ctrl_no << 0) |  \
    (_gpio_no << 8) |  \
    (_mfp_sel << 16)


enum CFG_TYPE_LRADC_CTRL
{
    LRADC_CTRL_NONE = 0xFF,

    LRADC_CTRL_2_GPIO_0  = CFG_LRADC_CTRL_DEF(2, 0,  1),
    LRADC_CTRL_3_GPIO_3  = CFG_LRADC_CTRL_DEF(3, 3,  1),
    LRADC_CTRL_2_GPIO_8  = CFG_LRADC_CTRL_DEF(2, 8,  1),
    LRADC_CTRL_1_WIO_1   = CFG_LRADC_CTRL_DEF(1, 32, 1),
};


enum CFG_TYPE_LRADC_PULL_UP
{
    LRADC_PULL_UP_INTERNAL_50K = 0x8,  // 内部上拉 (50K)
    LRADC_PULL_UP_INTERNAL_10K = 0x1,  // 内部上拉 (10K)
    LRADC_PULL_UP_EXTERNAL     = 0x0,  // 外部上拉
};


enum CFG_TYPE_KEY_VALUE
{
    VKEY_NONE = KEY_RESERVED,

    VKEY_1 = KEY_F1,
    VKEY_2 = KEY_F2,
    VKEY_3 = KEY_F3,
    VKEY_4 = KEY_F4,
    VKEY_5 = KEY_F5,
    VKEY_6 = KEY_F6,

    LRADC_COMBO_VKEY_1 = KEY_COMBO_1,
    LRADC_COMBO_VKEY_2 = KEY_COMBO_2,
    LRADC_COMBO_VKEY_3 = KEY_COMBO_3,

    VKEY_PLAY = KEY_PAUSE_AND_RESUME,
    VKEY_VSUB = KEY_VOLUMEDOWN,
    VKEY_VADD = KEY_VOLUMEUP,
    VKEY_PREV = KEY_PREVIOUSSONG,
    VKEY_NEXT = KEY_NEXTSONG,
    VKEY_MODE = KEY_TBD,
    VKEY_MENU = KEY_MENU,

    VKEY_MEDIA     = KEY_F1,
    VKEY_NR        = KEY_F2,
    VKEY_SURROUND  = KEY_F3,
    VKEY_MIC_MUTE  = KEY_F4,

    VKEY_ROTARY_BTN  = KEY_F5,
    VKEY_ROTARY_FORD = KEY_PREVIOUSSONG,
    VKEY_ROTARY_BACK = KEY_NEXTSONG,

    TAP_KEY = KEY_TAP,
};


enum CFG_TYPE_KEY_EVENT
{
    KEY_EVENT_NONE            = 0,                      // 无
    KEY_EVENT_SINGLE_CLICK    = (KEY_TYPE_SHORT_UP >>16),   // 单击
    KEY_EVENT_DOUBLE_CLICK    = (KEY_TYPE_DOUBLE_CLICK >>16),   // 双击
    KEY_EVENT_TRIPLE_CLICK    = (KEY_TYPE_TRIPLE_CLICK >>16),   // 三击
    KEY_EVENT_QUAD_CLICK      = (KEY_TYPE_QUAD_CLICK >>16),   // 四击
    KEY_EVENT_QUINT_CLICK     = (KEY_TYPE_QUINT_CLICK >>16),   // 五击

    KEY_EVENT_REPEAT          = (KEY_TYPE_HOLD >>16),          // 重复按键
    KEY_EVENT_LONG_PRESS      = (KEY_TYPE_LONG1S >>16),        // 长按
    KEY_EVENT_LONG_UP         = (KEY_TYPE_LONG1S_UP >>16),     // 长按抬起
    KEY_EVENT_LONG_LONG_PRESS = (KEY_TYPE_LONG3S >>16),        // 超长按
    KEY_EVENT_LONG_LONG_UP    = (KEY_TYPE_LONG3S_UP >>16),     // 超长按抬起
    KEY_EVENT_VERY_LONG_PRESS = (KEY_TYPE_LONG6S >>16),  // 极长按
    KEY_EVENT_VERY_LONG_UP    = (KEY_TYPE_LONG6S_UP >>16),  // 极长按抬起

    KEY_EVENT_CUSTOMED_SEQUENCE_1 = (KEY_TYPE_CUSTOMED_SEQUENCE_1 >>16),  // 自定义按键序列 1
    KEY_EVENT_CUSTOMED_SEQUENCE_2 = (KEY_TYPE_CUSTOMED_SEQUENCE_2 >>16),  // 自定义按键序列 2
};


enum CFG_TYPE_SEQ1_KEY_EVENT
{
    SEQ1_KEY_EVENT_NONE         = KEY_EVENT_NONE,          // 无
    SEQ1_KEY_EVENT_SINGLE_CLICK = KEY_EVENT_SINGLE_CLICK,  // 单击
    SEQ1_KEY_EVENT_DOUBLE_CLICK = KEY_EVENT_DOUBLE_CLICK,  // 双击
    SEQ1_KEY_EVENT_TRIPLE_CLICK = KEY_EVENT_TRIPLE_CLICK,  // 三击
    SEQ1_KEY_EVENT_QUAD_CLICK   = KEY_EVENT_QUAD_CLICK,    // 四击
    SEQ1_KEY_EVENT_QUINT_CLICK  = KEY_EVENT_QUINT_CLICK,   // 五击
};


enum CFG_TYPE_SEQ2_KEY_EVENT
{
    SEQ2_KEY_EVENT_NONE            = KEY_EVENT_NONE,             // 无
    SEQ2_KEY_EVENT_LONG_PRESS      = KEY_EVENT_LONG_PRESS,       // 长按
    SEQ2_KEY_EVENT_LONG_UP         = KEY_EVENT_LONG_UP,          // 长按抬起
    SEQ2_KEY_EVENT_LONG_LONG_PRESS = KEY_EVENT_LONG_LONG_PRESS,  // 超长按
    SEQ2_KEY_EVENT_LONG_LONG_UP    = KEY_EVENT_LONG_LONG_UP,     // 超长按抬起
    SEQ2_KEY_EVENT_VERY_LONG_PRESS = KEY_EVENT_VERY_LONG_PRESS,  // 极长按
    SEQ2_KEY_EVENT_VERY_LONG_UP    = KEY_EVENT_VERY_LONG_UP,     // 极长按抬起
};


enum CFG_TYPE_CUSTOMED_KEY_SEQUENCE
{
    CUSTOMED_KEY_SEQUENCE_NONE = KEY_EVENT_NONE,  // 无

    CUSTOMED_KEY_SEQUENCE_1 = KEY_EVENT_CUSTOMED_SEQUENCE_1,  // 自定义按键序列 1
    CUSTOMED_KEY_SEQUENCE_2 = KEY_EVENT_CUSTOMED_SEQUENCE_2,  // 自定义按键序列 2
};


enum CFG_TYPE_KEY_DEVICE_TYPE
{
    KEY_DEVICE_TYPE_NONE = 0,  // 无

    KEY_DEVICE_TWS_UNPAIRED = (1 << 0),  // 未组对
    KEY_DEVICE_TWS_PAIRED   = (1 << 1),  // 已组对
    KEY_DEVICE_CHANNEL_L    = (1 << 2),  // L 左设备
    KEY_DEVICE_CHANNEL_R    = (1 << 3),  // R 右设备
};


enum CFG_TYPE_KEY_FUNC
{
    KEY_FUNC_NONE = 0,                           // 无

    KEY_FUNC_POWER_OFF,                          // 关机

    KEY_FUNC_ENTER_PAIR_MODE,                    // 进入配对模式
    KEY_FUNC_CLEAR_PAIRED_LIST,                  // 清除配对列表 (不限模式)
    KEY_FUNC_CLEAR_PAIRED_LIST_IN_PAIR_MODE,     // 清除配对列表 (配对模式下)
    KEY_FUNC_CLEAR_PAIRED_LIST_IN_FRONT_CHARGE,  // 清除配对列表 (前台充电时)
    KEY_FUNC_CLEAR_PAIRED_LIST_IN_UNLINKED,      // 清除配对列表 (未连接时)

    KEY_FUNC_TWS_PAIR_SEARCH,                    // TWS 组对搜索
    KEY_FUNC_START_RECONNECT,                    // 启动回连

    KEY_FUNC_PLAY_PAUSE,                         // 播放/暂停
    KEY_FUNC_PREV_MUSIC,                         // 上一曲 (不限状态)
    KEY_FUNC_NEXT_MUSIC,                         // 下一曲 (不限状态)

    KEY_FUNC_PREV_MUSIC_IN_PLAYING,              // 上一曲 (播放状态)
    KEY_FUNC_NEXT_MUSIC_IN_PLAYING,              // 下一曲 (播放状态)

    KEY_FUNC_PREV_MUSIC_IN_PAUSED,               // 上一曲 (暂停状态)
    KEY_FUNC_NEXT_MUSIC_IN_PAUSED,               // 下一曲 (暂停状态)

    KEY_FUNC_ADD_MUSIC_VOLUME,                   // 音乐音量+
    KEY_FUNC_SUB_MUSIC_VOLUME,                   // 音乐音量-

    KEY_FUNC_ADD_MUSIC_VOLUME_IN_LINKED,         // 音乐音量+ (已连接时)
    KEY_FUNC_SUB_MUSIC_VOLUME_IN_LINKED,         // 音乐音量- (已连接时)

    KEY_FUNC_ADD_CALL_VOLUME,                    // 通话音量+
    KEY_FUNC_SUB_CALL_VOLUME,                    // 通话音量-

    KEY_FUNC_ACCEPT_CALL,                        // 接听来电
    KEY_FUNC_REJECT_CALL,                        // 拒接来电
    KEY_FUNC_HANGUP_CALL,                        // 挂断通话

    KEY_FUNC_KEEP_CALL_RELEASE_3WAY,             // 继续当前通话，挂断三方通话
    KEY_FUNC_HOLD_CALL_ACTIVE_3WAY,              // 保留当前通话，切换三方通话
    KEY_FUNC_HANGUP_CALL_ACTIVE_3WAY,            // 挂断当前通话，切换三方通话

    KEY_FUNC_SWITCH_CALL_OUT,                    // 切换通话输出
    KEY_FUNC_SWITCH_MIC_MUTE,                    // 打开/关闭 MIC 静音

    KEY_FUNC_DIAL_LAST_NO,                       // 回拨电话

    KEY_FUNC_START_VOICE_ASSIST,                 // 启动 Siri 等语音助手
    KEY_FUNC_STOP_VOICE_ASSIST,                  // 停止 Siri 等语音助手

    KEY_FUNC_HID_PHOTO_SHOT,                     // HID 拍照
    KEY_FUNC_HID_CUSTOM_KEY,                     // HID 自定义按键

    KEY_FUNC_ENTER_BQB_TEST_MODE,                // 进入 BQB 测试模式 (不限模式)
    KEY_FUNC_ENTER_BQB_TEST_IN_PAIR_MODE,        // 进入 BQB 测试模式 (配对模式下)

    KEY_FUNC_SWITCH_VOICE_LANG,                  // 切换语音语言 (不限模式)
    KEY_FUNC_SWITCH_VOICE_LANG_IN_PAIR_MODE,     // 切换语音语言 (配对模式下)
    KEY_FUNC_SWITCH_VOICE_LANG_IN_UNLINKED,      // 切换语音语言 (未连接时)

    KEY_FUNC_CUSTOMED_1,                         // 自定义功能 1
    KEY_FUNC_CUSTOMED_2,                         // 自定义功能 2
    KEY_FUNC_CUSTOMED_3,                         // 自定义功能 3
    KEY_FUNC_CUSTOMED_4,                         // 自定义功能 4
    KEY_FUNC_CUSTOMED_5,                         // 自定义功能 5
    KEY_FUNC_CUSTOMED_6,                         // 自定义功能 6
    KEY_FUNC_CUSTOMED_7,                         // 自定义功能 7
    KEY_FUNC_CUSTOMED_8,                         // 自定义功能 8
    KEY_FUNC_CUSTOMED_9,                         // 自定义功能 9

    KEY_FUNC_SWITCH_LOW_LATENCY_MODE,            // 切换低延迟模式
    KEY_FUNC_START_PRIVMA_TALK,                  // 开始APP智能语音上传
    KEY_FUNC_NMA_KEY_FIRST,                      // 云音乐按键定义单击功能
    KEY_FUNC_NMA_KEY_SECOND,                     // 云音乐按键定义双击功能
    KEY_FUNC_TMA_ONE_KEY_TO_REQUEST,             // 腾讯音乐娱乐一键直达

    KEY_FUNC_DAE_SWITCH,                         // 音效切换
    KEY_FUNC_TRANSPARENCY_MODE,                  // 通透模式
    KEY_FUNC_MANUAL_SWITCH_APP,                  // 手动切换活动应用
    KEY_FUNC_SWITCH_AUDIO_INPUT,                 // 切换到LINEIN应用
    KEY_FUNC_EXIT_BQB_TEST_MODE,                 // 退出 BQB 测试模式
    KEY_FUNC_SWITCH_NR_FLAG,                     // 切换降噪模式
    KEY_FUNC_DONGLE_ENTER_SEARCH,                // 进入搜索
    
    KEY_FUNC_ACCEPT_INCOMING,                    // 接听dongle端来电
    KEY_FUNC_REJECT_INCOMING,                    // 挂断/拒接dongle来电
    KEY_FUNC_CALL_MUTE,                          // 通话静音
    KEY_FUNC_CALL_UNMUTE,                        // 通话取消静音
    KEY_FUNC_DOUBLE_SOUNDCARD_VOL_SUB,           // 双声卡下控制软音量减
    KEY_FUNC_DOUBLE_SOUNDCARD_VOL_ADD,           // 双声卡下控制软音量加
    KEY_FUNC_SWITCH_LANTENCY_MODE,               // 选择延时模式
};

enum CFG_TYPE_LED_GPIO_PIN
{
    LED_GPIO_NONE = GPIO_NONE,  // GPIO_NONE

    LED_GPIO_0  = GPIO_PIN_0,   // GPIO_0   (PWM_3)
    LED_GPIO_1  = GPIO_PIN_1,   // GPIO_1   (PWM_2)
    LED_GPIO_2  = GPIO_PIN_2,   // GPIO_2   (PWM_3)
    LED_GPIO_3  = GPIO_PIN_3,   // GPIO_3   (PWM_2)
    LED_GPIO_4  = GPIO_PIN_4,   // GPIO_4   (PWM_0)
    LED_GPIO_5  = GPIO_PIN_5,   // GPIO_5   (PWM_1)
    LED_GPIO_6  = GPIO_PIN_6,   // GPIO_6   (PWM_3)
    LED_GPIO_7  = GPIO_PIN_7,   // GPIO_7   (PWM_1)
    LED_GPIO_8  = GPIO_PIN_8,   // GPIO_8   (PWM_2)
    LED_GPIO_9  = GPIO_PIN_9,   // GPIO_9   (PWM_0)
    LED_GPIO_10 = GPIO_PIN_10,  // GPIO_10  (PWM_1)
    LED_GPIO_11 = GPIO_PIN_11,  // GPIO_11
    LED_GPIO_12 = GPIO_PIN_12,  // GPIO_12
    LED_GPIO_13 = GPIO_PIN_13,  // GPIO_13
    LED_GPIO_14 = GPIO_PIN_14,  // GPIO_14
    LED_GPIO_15 = GPIO_PIN_15,  // GPIO_15
    LED_GPIO_16 = GPIO_PIN_16,  // GPIO_16
    LED_GPIO_17 = GPIO_PIN_17,  // GPIO_17 (PWM_2)
    LED_GPIO_18 = GPIO_PIN_18,  // GPIO_18 (PWM_0)
    LED_GPIO_19 = GPIO_PIN_19,  // GPIO_19 (PWM_3)
    LED_GPIO_20 = GPIO_PIN_20,  // GPIO_20 (PWM_2)
    LED_GPIO_21 = GPIO_PIN_21,  // GPIO_21 (PWM_0)
    LED_GPIO_22 = GPIO_PIN_22,  // GPIO_22 (PWM_1)
    LED_GPIO_23 = GPIO_PIN_23,  // GPIO_23
    LED_GPIO_24 = GPIO_PIN_24,  // GPIO_24
    LED_GPIO_25 = GPIO_PIN_25,  // GPIO_25
    LED_GPIO_26 = GPIO_PIN_26,  // GPIO_26
    LED_GPIO_27 = GPIO_PIN_27,  // GPIO_27
    LED_GPIO_28 = GPIO_PIN_28,  // GPIO_28
    LED_GPIO_29 = GPIO_PIN_29,  // GPIO_29
    LED_GPIO_30 = GPIO_PIN_30,  // GPIO_30
};


enum CFG_TYPE_LED_NO
{
    LED_NULL = 0,

    LED_1 = (1 << 0),
    LED_2 = (1 << 1),
    LED_3 = (1 << 2),
    LED_4 = (1 << 3),

    LED_RED   = (1 << 4),
    LED_GREEN = (1 << 5),
    LED_BLUE  = (1 << 6),
};


enum CFG_TYPE_LED_DISPLAY_MODEL
{
    LED_DISPLAY_MODEL_NONE = 0,

    LED_DISPLAY_MODEL_1,
    LED_DISPLAY_MODEL_2,
    LED_DISPLAY_MODEL_3,
    LED_DISPLAY_MODEL_4,
    LED_DISPLAY_MODEL_5,
    LED_DISPLAY_MODEL_6,
    LED_DISPLAY_MODEL_7,
    LED_DISPLAY_MODEL_8,
    LED_DISPLAY_MODEL_9,

    LED_DISPLAY_POWER_ON,
    LED_DISPLAY_POWER_OFF,
    LED_DISPLAY_STANDBY,
    LED_DISPLAY_CHARGE_START,
    LED_DISPLAY_CHARGE_FULL,
    LED_DISPLAY_BT_PAIR_MODE,
    LED_DISPLAY_BT_WAIT_CONNECT,
    LED_DISPLAY_BT_CONNECTED,
    LED_DISPLAY_BT_UNLINKED,
    LED_DISPLAY_BT_CALL_INCOMING,
    LED_DISPLAY_BT_CALL_ONGOING,
    LED_DISPLAY_KEY_PRESS,
    LED_DISPLAY_MUTE_ON,
    LED_DISPLAY_MUTE_OFF,
    LED_DISPLAY_BATTERY_LOW,
    LED_DISPLAY_BATTERY_TOO_LOW,
    LED_DISPLAY_MANUAL_SWITCH_APP_1,
    LED_DISPLAY_MANUAL_SWITCH_APP_2,
    LED_DISPLAY_ENTER_BQB_TEST,
    LED_DISPLAY_DONGLE_ENTER_SEARCH,
    LED_DISPLAY_DONGLE_DISCONNECTED,
    LED_DISPLAY_DONGLE_CONNECTED,
    LED_DISPLAY_DONGLE_ENTER_BQB_MODE,
};


enum CFG_TYPE_LED_OVERRIDE_MODE
{
    LED_OVERRIDE_NONE   = 0,  // 无

    LED_OVERRIDE_FRONT  = 3,  // 前
    LED_OVERRIDE_MIDDLE = 2,  // 中
    LED_OVERRIDE_BACK   = 1,  // 后
};

enum CFG_TYPE_I2STX_MCLK_PIN
{
    I2STX_MCLK_GPIO_NONE = GPIO_NONE,
    I2STX_MCLK_GPIO_3     = CFG_GPIO_MFP_PIN_DEF(3,  10),
    I2STX_MCLK_GPIO_7     = CFG_GPIO_MFP_PIN_DEF(7,  10),
    I2STX_MCLK_GPIO_10    = CFG_GPIO_MFP_PIN_DEF(10, 10),
    I2STX_MCLK_GPIO_20    = CFG_GPIO_MFP_PIN_DEF(20, 10),

};

enum CFG_TYPE_I2STX_BCLK_PIN
{
    I2STX_BCLK_GPIO_NONE = GPIO_NONE,
    I2STX_BCLK_GPIO_0    = CFG_GPIO_MFP_PIN_DEF(0,  10),
    I2STX_BCLK_GPIO_5    = CFG_GPIO_MFP_PIN_DEF(5,  10),
    I2STX_BCLK_GPIO_18   = CFG_GPIO_MFP_PIN_DEF(18, 10),
};

enum CFG_TYPE_I2STX_LRCLK_PIN
{
    I2STX_LRCLK_GPIO_NONE = GPIO_NONE,
    I2STX_LRCLK_GPIO_1    = CFG_GPIO_MFP_PIN_DEF(1,  10),
    I2STX_LRCLK_GPIO_4    = CFG_GPIO_MFP_PIN_DEF(4,  10),
    I2STX_LRCLK_GPIO_8    = CFG_GPIO_MFP_PIN_DEF(8,  10),
    I2STX_LRCLK_GPIO_17   = CFG_GPIO_MFP_PIN_DEF(17, 10),

};

enum CFG_TYPE_I2STX_DAT_PIN
{
    I2STX_DAT_GPIO_NONE  = GPIO_NONE,
    I2STX_DAT_GPIO_2     = CFG_GPIO_MFP_PIN_DEF(2,  10),
    I2STX_DAT_GPIO_6     = CFG_GPIO_MFP_PIN_DEF(6,  10),
    I2STX_DAT_GPIO_9     = CFG_GPIO_MFP_PIN_DEF(9,  10),
    I2STX_DAT_GPIO_19    = CFG_GPIO_MFP_PIN_DEF(19, 10),
    I2STX_DAT_GPIO_21    = CFG_GPIO_MFP_PIN_DEF(21, 10),
    I2STX_DAT_GPIO_22    = CFG_GPIO_MFP_PIN_DEF(22, 10),
};

enum CFG_TYPE_DMIC01_CLK_PIN
{
    DMIC01_CLK_GPIO_NONE  = GPIO_NONE,
    DMIC01_CLK_GPIO_6     = CFG_GPIO_MFP_PIN_DEF(6, 17),
    DMIC01_CLK_GPIO_30    = CFG_GPIO_MFP_PIN_DEF(30, 17),
};

enum CFG_TYPE_DMIC01_DAT_PIN
{
    DMIC01_DAT_GPIO_NONE = GPIO_NONE,
    DMIC01_DAT_GPIO_7    = CFG_GPIO_MFP_PIN_DEF(7, 17),
    DMIC01_DAT_GPIO_31   = CFG_GPIO_MFP_PIN_DEF(31, 17),
};

enum CFG_TYPE_VOLUME_GAIN
{
    VOLUME_GAIN_0  = 0x0,   // MUTE
    VOLUME_GAIN_1  = 0x26,  // -57.375  dB
    VOLUME_GAIN_2  = 0x2f,  // -54.000  dB
    VOLUME_GAIN_3  = 0x36,  // -51.375  dB
    VOLUME_GAIN_4  = 0x3d,  // -48.750  dB
    VOLUME_GAIN_5  = 0x47,  // -45.000  dB
    VOLUME_GAIN_6  = 0x4e,  // -42.375  dB
    VOLUME_GAIN_7  = 0x57,  // -39.000  dB  
    VOLUME_GAIN_8  = 0x5e,  // -36.375  dB   
    VOLUME_GAIN_9  = 0x63,  // -34.500  dB     
    VOLUME_GAIN_10 = 0x69,  // -32.250  dB
    VOLUME_GAIN_11 = 0x6c,  // -31.125  dB
    VOLUME_GAIN_12 = 0x70,  // -29.625  dB
    VOLUME_GAIN_13 = 0x75,  // -27.750  dB
    VOLUME_GAIN_14 = 0x78,  // -26.625  dB   
    VOLUME_GAIN_15 = 0x7b,  // -25.500  dB  
    VOLUME_GAIN_16 = 0x7e,  // -24.375  dB      
    VOLUME_GAIN_17 = 0x81,  // -23.250  dB
    VOLUME_GAIN_18 = 0x84,  // -22.125  dB
    VOLUME_GAIN_19 = 0x87,  // -21.000  dB
    VOLUME_GAIN_20 = 0x89,  // -20.250  dB
    VOLUME_GAIN_21 = 0x8b,  // -19.500  dB
    VOLUME_GAIN_22 = 0x8d,  // -18.750  dB
    VOLUME_GAIN_23 = 0x90,  // -17.625  dB
    VOLUME_GAIN_24 = 0x91,  // -17.250  dB
    VOLUME_GAIN_25 = 0x93,  // -16.500  dB
    VOLUME_GAIN_26 = 0x95,  // -15.750  dB
    VOLUME_GAIN_27 = 0x97,  // -15.000  dB
    VOLUME_GAIN_28 = 0x98,  // -14.625  dB
    VOLUME_GAIN_29 = 0x9a,  // -13.875  dB
    VOLUME_GAIN_30 = 0x9c,  // -13.125  dB
    VOLUME_GAIN_31 = 0x9d,  // -12.750  dB
    VOLUME_GAIN_32 = 0x9f,  // -12.000  dB
    VOLUME_GAIN_33 = 0xa0,  // -11.625  dB
    VOLUME_GAIN_34 = 0xa2,  // -10.875  dB
    VOLUME_GAIN_35 = 0xa3,  // -10.500  dB
    VOLUME_GAIN_36 = 0xa4,  // -10.125  dB
    VOLUME_GAIN_37 = 0xa6,  // -9.375  dB 
    VOLUME_GAIN_38 = 0xa7,  // -9.000  dB
    VOLUME_GAIN_39 = 0xa8,  // -8.625  dB
    VOLUME_GAIN_40 = 0xa9,  // -8.250  dB
    VOLUME_GAIN_41 = 0xaa,  // -7.875  dB
    VOLUME_GAIN_42 = 0xab,  // -7.500  dB
    VOLUME_GAIN_43 = 0xad,  // -6.750  dB
    VOLUME_GAIN_44 = 0xae,  // -6.375  dB
    VOLUME_GAIN_45 = 0xaf,  // -6.000  dB
    VOLUME_GAIN_46 = 0xb0,  // -5.625  dB
    VOLUME_GAIN_47 = 0xb1,  // -5.250  dB
    VOLUME_GAIN_48 = 0xb2,  // -4.875  dB
    VOLUME_GAIN_49 = 0xb3,  // -4.500  dB
    VOLUME_GAIN_50 = 0xb4,  // -4.125  dB
    VOLUME_GAIN_51 = 0xb5,  // -3.750  dB
    VOLUME_GAIN_52 = 0xb6,  // -3.375  dB
    VOLUME_GAIN_53 = 0xb7,  // -3.000  dB
    VOLUME_GAIN_54 = 0xb8,  // -2.625  dB
    VOLUME_GAIN_55 = 0xb9,  // -2.250  dB
    VOLUME_GAIN_56 = 0xba,  // -1.875  dB
    VOLUME_GAIN_57 = 0xbb,  // -1.500  dB
    VOLUME_GAIN_58 = 0xbc,  // -1.125  dB
    VOLUME_GAIN_59 = 0xbd,  // -0.750  dB
    VOLUME_GAIN_60 = 0xbe,  // -0.375  dB 
    VOLUME_GAIN_61 = 0xbf,  //  0.00   dB
    VOLUME_GAIN_62 = 0xc0,  //  0.375  dB
    VOLUME_GAIN_63 = 0xc1,  //  0.750  dB
    VOLUME_GAIN_64 = 0xc2,  //  1.125  dB
    VOLUME_GAIN_65 = 0xc3,  //  1.500  dB
    VOLUME_GAIN_66 = 0xc4,  //  1.875  dB
    VOLUME_GAIN_67 = 0xc5,  //  2.250  dB
    VOLUME_GAIN_68 = 0xc6,  //  2.625  dB
    VOLUME_GAIN_69 = 0xc7,  //  3.000  dB
    VOLUME_GAIN_70 = 0xc8,  //  3.375  dB
    VOLUME_GAIN_71 = 0xc9,  //  3.750  dB
    VOLUME_GAIN_72 = 0xca,  //  4.125  dB
    VOLUME_GAIN_73 = 0xcb,  //  4.500  dB
    VOLUME_GAIN_74 = 0xcc,  //  4.875  dB
    VOLUME_GAIN_75 = 0xcd,  //  5.250  dB
    VOLUME_GAIN_76 = 0xce,  //  5.625  dB
    VOLUME_GAIN_77 = 0xcf,  //  6.000  dB
    VOLUME_GAIN_78 = 0xd0,  //  6.375  dB
    VOLUME_GAIN_79 = 0xd1,  //  6.750  dB
    VOLUME_GAIN_80 = 0xd2,  //  7.125  dB
    VOLUME_GAIN_81 = 0xd3,  //  7.500  dB
    VOLUME_GAIN_82 = 0xd4,  //  7.875  dB 
};

enum CFG_TYPE_MIC_AGAIN
{
    MIC_AGAIN_0_0_DB    = 0x0,  // 0 dB
    MIC_AGAIN_6_0_DB    = 60,  // 6.0 dB
    MIC_AGAIN_12_0_DB   = 120,  // 12.0 dB
};

enum CFG_TYPE_MIC_DGAIN
{
    MIC_DGAIN_0_0_DB    = 0x0,  // 0 dB
    MIC_DGAIN_1_0_DB    = 10,   // 1.023 dB
    MIC_DGAIN_2_0_DB    = 20,   // 2.046 dB
    MIC_DGAIN_3_0_DB    = 30,   // 3.069 dB
    MIC_DGAIN_4_0_DB    = 40,   // 4.092 dB
    MIC_DGAIN_5_0_DB    = 51,   // 5.115 dB
    MIC_DGAIN_6_0_DB    = 60,   // 6.0 dB
    MIC_DGAIN_7_0_DB    = 70,   // 7.023 dB
    MIC_DGAIN_8_0_DB    = 80,   // 8.046 dB
    MIC_DGAIN_9_5_DB    = 90,   // 9.069 dB
    MIC_DGAIN_10_0_DB   = 100,  // 10.092 dB
    MIC_DGAIN_11_1_DB   = 111,  // 11.115 dB
    MIC_DGAIN_12_0_DB   = 120,  // 12.0 dB
    MIC_DGAIN_13_0_DB   = 130,  // 13.023 dB
    MIC_DGAIN_14_0_DB   = 140,  // 14.046 dB
    MIC_DGAIN_15_0_DB   = 150,  // 15.069 dB
    MIC_DGAIN_16_0_DB   = 160,  // 16.092 dB
    MIC_DGAIN_17_1_DB   = 171,  // 17.115 dB
    MIC_DGAIN_18_0_DB   = 180,  // 18.0 dB
    MIC_DGAIN_19_0_DB   = 190,  // 19.023 dB
    MIC_DGAIN_20_0_DB   = 200,  // 20.046 dB
    MIC_DGAIN_21_5_DB   = 210,  // 21.069 dB
    MIC_DGAIN_22_0_DB   = 220,  // 22.092 dB
    MIC_DGAIN_23_1_DB   = 231,  // 23.115 dB
    MIC_DGAIN_24_0_DB   = 240,  // 24.0 dB
    MIC_DGAIN_25_0_DB   = 250,  // 25.023 dB
    MIC_DGAIN_26_0_DB   = 260,  // 26.046 dB
    MIC_DGAIN_27_5_DB   = 270,  // 27.069 dB
    MIC_DGAIN_28_0_DB   = 280,  // 28.092 dB
    MIC_DGAIN_29_1_DB   = 291,  // 29.115 dB
    MIC_DGAIN_30_0_DB   = 300,  // 30.0 dB
    MIC_DGAIN_31_0_DB   = 310,  // 31.023 dB
    MIC_DGAIN_32_0_DB   = 320,  // 32.046 dB
    MIC_DGAIN_33_0_DB   = 330,  // 33.069 dB
    MIC_DGAIN_34_0_DB   = 340,  // 34.092 dB
    MIC_DGAIN_35_1_DB   = 351,  // 35.115 dB
    MIC_DGAIN_36_0_DB   = 360,  // 36.0 dB
    MIC_DGAIN_37_0_DB   = 370,  // 37.023 dB
    MIC_DGAIN_38_0_DB   = 380,  // 38.046 dB
    MIC_DGAIN_39_5_DB   = 390,  // 39.069 dB
    MIC_DGAIN_40_0_DB   = 400,  // 40.092 dB
    MIC_DGAIN_41_1_DB   = 411,  // 41.115 dB
    MIC_DGAIN_42_0_DB   = 420,  // 42.0 dB
    MIC_DGAIN_43_0_DB   = 430,  // 43.023 dB
    MIC_DGAIN_44_0_DB   = 440,  // 44.046 dB
    MIC_DGAIN_45_5_DB   = 450,  // 45.069 dB
    MIC_DGAIN_46_0_DB   = 460,  // 46.092 dB
    MIC_DGAIN_47_1_DB   = 471,  // 47.115 dB
    MIC_DGAIN_48_1_DB   = 481,  // 48.138 dB
    MIC_DGAIN_49_1_DB   = 491,  // 49.161 dB

};

enum CFG_TYPE_LINEIN_DETECT_MODE
{
    LINEIN_DETECT_NONE,      // 不检测
    LINEIN_DETECT_BY_GPIO,   // GPIO 检测
    LINEIN_DETECT_BY_LRADC,  // LRADC 检测
};


enum CFG_TYPE_AUDIO_OUT_MODE
{
    AUDIO_OUT_MODE_DAC_DIFF     = 0,  // DAC 差分
    AUDIO_OUT_MODE_DAC_NODIRECT = 1,  // DAC 非直驱
    AUDIO_OUT_MODE_I2S          = 2,  // I2S
};


enum CFG_TYPE_AUXIN_OUT_MODE
{
    AUX_AA   = 0,  // AA 通路
    AUX_ADDA = 1,  // ADDA 通路
};


enum CFG_TYPE_SPEAKER_OUT_SELECT
{
    SPEAKER_OUT_ADAPTIVE = 2,  // 自适应
    SPEAKER_OUT_ENABLE   = 1,  // 打开
    SPEAKER_OUT_DISABLE  = 0,  // 关闭
};


enum CFG_TYPE_DMIC_GAIN_SELECT
{
    DMIC_GAIN_1X = 0,   // 1X(0db)
    DMIC_GAIN_2X = 1,   // 2X(6db)
    DMIC_GAIN_4X = 2,   // 4X(12db)
    DMIC_GAIN_8X = 3,   // 8X(18db)
    DMIC_GAIN_16X = 4,  // 16X(24db)
    DMIC_GAIN_32X = 5,  // 32X(30db)
    DMIC_GAIN_64X  = 6,  // 63X(36db)
    DMIC_GAIN_127X = 6,  // 127X(42db)
};

enum CFG_TYPE_ADC
{
    ADC_NONE    = 0,          // 无
    ADC_0       = (1 << 0),   // ADC0
    ADC_1       = (1 << 1),   // ADC1
};

enum CFG_TYPE_ADC_INPUT
{
    ADC_INPUT_NONE         = 0,           // 无
    ADC_INPUT_0N           = (1 << 0),    // INPUT_0N
    ADC_INPUT_0P           = (1 << 1),    // INPUT_0P
    ADC_INPUT_0NP_DIFF     = (ADC_INPUT_0N | ADC_INPUT_0P),    // INPUT_0NP_DIFF
    ADC_INPUT_1N           = (1 << 2),    // INPUT_1N
    ADC_INPUT_1P           = (1 << 3),    // INPUT_1P
    ADC_INPUT_1NP_DIFF     = (ADC_INPUT_1N | ADC_INPUT_1P),    // INPUT_1NP_DIFF
    ADC_INPUT_DMIC         = (0xFF),        // DMIC
};

enum CFG_TYPE_ADC0_INPUT
{
    ADC0_INPUT_NONE         = ADC_INPUT_NONE,       // 无
    ADC0_INPUT_0N           = ADC_INPUT_0N,         // INPUT_0N
    ADC0_INPUT_0P           = ADC_INPUT_0P,         // INPUT_0P
    ADC0_INPUT_0NP_DIFF     = ADC_INPUT_0NP_DIFF,   // INPUT_0NP_DIFF
    ADC0_INPUT_DMIC         = ADC_INPUT_DMIC,       // DMIC
};

enum CFG_TYPE_ADC1_INPUT
{
    ADC1_INPUT_NONE         = ADC_INPUT_NONE,      // 无
    ADC1_INPUT_1N           = ADC_INPUT_1N,        // INPUT_1N
    ADC1_INPUT_1P           = ADC_INPUT_1P,        // INPUT_1P
    ADC1_INPUT_1NP_DIFF     = ADC_INPUT_1NP_DIFF,  // INPUT_1NP_DIFF
    ADC1_INPUT_DMIC         = ADC_INPUT_DMIC,      // DMIC
};

enum CFG_TYPE_VMIC
{
    VMIC_0       =  0,     // VMIC0
    VMIC_1       =  1,     // VMIC1
    VMIC_NONE    =  0xFF,  // 无
};

enum CFG_TYPE_PA_SWING_SETTING
{
    PA_SWING_1_6_VPP = 1,  // 1.6 Vpp
    PA_SWING_2_0_VPP = 0,  // 2.0 Vpp
};


enum CFG_TYPE_CHANNEL_SELECT_MODE
{
    CHANNEL_SELECT_NORMAL_LR,      // (非 TWS) 正常左右声道
    CHANNEL_SELECT_MIX_LR,         // (非 TWS) 混合左右声道
    CHANNEL_SELECT_SWAP_LR,        // (非 TWS) 交换左右声道
    CHANNEL_SELECT_BOTH_L,         // (非 TWS) 双边左声道
    CHANNEL_SELECT_BOTH_R,         // (非 TWS) 双边右声道

    CHANNEL_SELECT_L_BY_TWS_PAIR,  // TWS 组对选择左声道
    CHANNEL_SELECT_R_BY_TWS_PAIR,  // TWS 组对选择右声道

    CHANNEL_SELECT_L_BY_GPIO,      // GPIO 选择左声道
    CHANNEL_SELECT_R_BY_GPIO,      // GPIO 选择右声道

    CHANNEL_SELECT_L_BY_LRADC,     // LRADC 选择左声道
    CHANNEL_SELECT_R_BY_LRADC,     // LRADC 选择右声道
};


enum CFG_TYPE_TWS_ALONE_AUDIO_CHANNEL
{
    TWS_ALONE_AUDIO_MIX_LR,    // 混合左右声道
    TWS_ALONE_AUDIO_ADAPTIVE,  // 单边自适应
    TWS_ALONE_AUDIO_SINGLE_L,  // 单边左声道
    TWS_ALONE_AUDIO_SINGLE_R,  // 单边右声道
};


enum CFG_TYPE_VOICE_ID
{
    VOICE_NONE = 0,  // 无

    VOICE_ID1,  // 语音 1
    VOICE_ID2,  // 语音 2
    VOICE_ID3,  // 语音 3
    VOICE_ID4,  // 语音 4
    VOICE_ID5,  // 语音 5

    VOICE_POWER_ON,         // 开机
    VOICE_POWER_OFF,        // 关机
    VOICE_BAT_LOW,          // 电量低
    VOICE_BAT_TOO_LOW,      // 电量不足
    VOICE_BT_PAIR_MODE,     // 配对模式
    VOICE_BT_WAIT_CONNECT,  // 等待连接
    VOICE_BT_CONNECTED,     // 蓝牙连接成功
    VOICE_2ND_CONNECTED,    // 第二设备连接成功
    VOICE_BT_DISCONNECTED,  // 蓝牙断开
    VOICE_TWS_WAIT_PAIR,    // 等待组对
    VOICE_TWS_CONNECTED,    // 组对成功
    VOICE_TWS_DISCONNECTED, // 组对断开
    VOICE_PLAY,             // 播放
    VOICE_PAUSE,            // 暂停
    VOICE_PREV_MUSIC,       // 上一曲
    VOICE_NEXT_MUSIC,       // 下一曲
    VOICE_MIN_VOLUME,       // 最小音量
    VOICE_MAX_VOLUME,       // 最大音量
    VOICE_NOMAL_MODE,       // 普通模式
    VOICE_ANC_MODE,         // 主动降噪模式   
    VOICE_LEISURE_MODE,     // ANC休闲模式
    VOICE_WIND_MODE,        // ANC风噪模式
    VOICE_TRANS_MODE,       // 通透模式
    VOICE_USB_MODE,         // USB模式
    VOICE_BT_MODE,          // BT模式
    VOICE_USB_CONNECTED,    // usb连接
    VOICE_USB_DISCONNECTED, // usb断开
    VOICE_BTT_CONNECTED,    // bt连接
    VOICE_BTT_DISCONNECTED, // bt断开
    VOICE_PAIR_MODE,        // 配对模式
    VOICE_MIC_ON,           // mic打开
    VOICE_MIC_OFF,          // mic关闭
};


enum CFG_TYPE_NUMERIC_VOICE_ID
{
    VOICE_NO_0 = '0',  // 数字 0
    VOICE_NO_1 = '1',  // 数字 1
    VOICE_NO_2 = '2',  // 数字 2
    VOICE_NO_3 = '3',  // 数字 3
    VOICE_NO_4 = '4',  // 数字 4
    VOICE_NO_5 = '5',  // 数字 5
    VOICE_NO_6 = '6',  // 数字 6
    VOICE_NO_7 = '7',  // 数字 7
    VOICE_NO_8 = '8',  // 数字 8
    VOICE_NO_9 = '9',  // 数字 9
};


enum CFG_TYPE_VOICE_LANGUAGE
{
    VOICE_LANGUAGE_1 = 0,  // 语音语言 1
    VOICE_LANGUAGE_2 = 1,  // 语音语言 2
};


enum CFG_TYPE_TONE_ID
{
    TONE_NONE = 0,  // 无

    TONE_ID1,  // 提示音 1
    TONE_ID2,  // 提示音 2
    TONE_ID3,  // 提示音 3
    TONE_ID4,  // 提示音 4
    TONE_ID5,  // 提示音 5
    TONE_ID6,  // 提示音 6
    TONE_ID7,  // 提示音 7

    TONE_KEY_SOUND,  // 按键音
    TONE_CALL_RING,  // 来电铃声
    TONE_WARNING,    // 警告

    TONE_TM_ENTER,   // 通透模式开
    TONE_TM_EXIT,    // 通透模式关
};


enum CFG_TYPE_SYS_EVENT
{
    UI_EVENT_NONE,            // 无

    UI_EVENT_POWER_ON,             // 开机
    UI_EVENT_POWER_OFF,           // 关机
    UI_EVENT_STANDBY,               // 待机
    UI_EVENT_WAKE_UP,               // 唤醒

    UI_EVENT_BATTERY_LOW,            // 电量低
    UI_EVENT_BATTERY_LOW_EX,      // 更低电量
    UI_EVENT_BATTERY_TOO_LOW,    // 电量不足
    UI_EVENT_REPEAT_BAT_LOW,      // 重复低电提示

    UI_EVENT_CHARGE_START,              // 开始充电
    UI_EVENT_CHARGE_FULL,                // 充电满
    UI_EVENT_FRONT_CHARGE_POWON,  // 前台充电开机
    UI_EVENT_CHARGE_STOP,              // 结束充电

    UI_EVENT_ENTER_PAIR_MODE,     // 进入配对模式
    UI_EVENT_CLEAR_PAIRED_LIST, // 清除配对列表

    UI_EVENT_BT_WAIT_CONNECT,     // 等待连接
    UI_EVENT_BT_CONNECTED,        // 蓝牙连接成功
    UI_EVENT_2ND_CONNECTED,      // 第二设备连接成功
    UI_EVENT_BT_DISCONNECTED,  // 蓝牙断开
    UI_EVENT_BT_UNLINKED,          // 蓝牙未连接

    UI_EVENT_TWS_WAIT_PAIR,     // TWS 等待组对
    UI_EVENT_TWS_CONNECTED,      // TWS 组对成功
    UI_EVENT_TWS_DISCONNECTED,// TWS 组对断开

    UI_EVENT_MIN_VOLUME,          // 最小音量
    UI_EVENT_MAX_VOLUME,          // 最大音量
    UI_EVENT_BT_MUSIC_PLAY,       // 蓝牙音乐播放
    UI_EVENT_BT_MUSIC_PAUSE,      // 蓝牙音乐暂停
    UI_EVENT_PREV_MUSIC,          // 上一曲
    UI_EVENT_NEXT_MUSIC,          // 下一曲

    UI_EVENT_BT_CALL_OUTGOING,    // 去电
    UI_EVENT_BT_CALL_INCOMING,    // 来电
    UI_EVENT_BT_CALL_3WAYIN,      // 三方来电
    UI_EVENT_BT_CALL_REJECT,      // 拒接来电
    UI_EVENT_BT_CALL_ONGOING,     // 通话开始
    UI_EVENT_BT_CALL_END,         // 通话结束
    UI_EVENT_SWITCH_CALL_OUT,     // 切换通话输出
    UI_EVENT_MIC_MUTE_ON,         // MIC 静音打开
    UI_EVENT_MIC_MUTE_OFF,        // MIC 静音关闭

    UI_EVENT_VOICE_ASSIST_START,  // Siri 等语音助手启动
    UI_EVENT_VOICE_ASSIST_STOP,   // Siri 等语音助手停止

    UI_EVENT_HID_PHOTO_SHOT,      // HID 拍照

    UI_EVENT_ENTER_LINEIN,        // 进入 Linein
    UI_EVENT_LINEIN_PLAY,         // Linein 播放
    UI_EVENT_LINEIN_PAUSE,        // Linein 暂停

    UI_EVENT_SEL_VOICE_LANG_1,    // 切换语音语言 1
    UI_EVENT_SEL_VOICE_LANG_2,    // 切换语音语言 2

    UI_EVENT_ENTER_BQB_TEST_MODE, // 进入 BQB 测试模式

    UI_EVENT_CUSTOMED_1,          // 自定义事件 1
    UI_EVENT_CUSTOMED_2,          // 自定义事件 2
    UI_EVENT_CUSTOMED_3,          // 自定义事件 3
    UI_EVENT_CUSTOMED_4,          // 自定义事件 4
    UI_EVENT_CUSTOMED_5,          // 自定义事件 5
    UI_EVENT_CUSTOMED_6,          // 自定义事件 6
    UI_EVENT_CUSTOMED_7,          // 自定义事件 7
    UI_EVENT_CUSTOMED_8,          // 自定义事件 8
    UI_EVENT_CUSTOMED_9,          // 自定义事件 9

    UI_EVENT_LOW_LATENCY_MODE,       // 低延迟模式
    UI_EVENT_NORMAL_LATENCY_MODE, // 正常延迟模式
    UI_EVENT_PRIVMA_TALK_START,     // 开始APP语音上传
    UI_EVENT_DC5V_CMD_COMPLETE,     // DC5V 通讯命令完成
    UI_EVENT_NMA_COLLECTION,           // 云音乐一键收藏

    UI_EVENT_BT_MUSIC_DAE_SWITCH, // 蓝牙音乐音效切换
    UI_EVENT_DAE_DEFAULT,         // 默认音效
    UI_EVENT_DAE_CUSTOM1,         // 自定义音效1
    UI_EVENT_DAE_CUSTOM2,         // 自定义音效2
    UI_EVENT_DAE_CUSTOM3,         // 自定义音效3
    UI_EVENT_DAE_CUSTOM4,         // 自定义音效4
    UI_EVENT_DAE_CUSTOM5,         // 自定义音效5
    UI_EVENT_DAE_CUSTOM6,         // 自定义音效6
    UI_EVENT_DAE_CUSTOM7,         // 自定义音效7
    UI_EVENT_DAE_CUSTOM8,         // 自定义音效8
    UI_EVENT_DAE_CUSTOM9,         // 自定义音效9

    UI_EVENT_NOMAL_MODE,          // 普通模式
    UI_EVENT_ANC_MODE,            // 主动降噪模式
    UI_EVENT_TRANSPARENCY_MODE,   // 通透模式
    UI_EVENT_ANC_LEISURE_MODE,    // 休闲模式
    UI_EVENT_ANC_WIND_NOISE_MODE, // 风噪模式

    UI_EVENT_RING_HANDSET,            // 寻找设备
    UI_EVENT_ALEXA_TONE_PLAY,      // 播放alexa喚醒提示音
    UI_EVENT_BT_CONNECTED_USB,       // usb蓝牙连接成功
    UI_EVENT_BT_DISCONNECTED_USB,    // usb蓝牙断开
    UI_EVENT_BT_CONNECTED_BR,        // br蓝牙连接成功
    UI_EVENT_BT_DISCONNECTED_BR,     // br蓝牙断开
    UI_EVENT_ENTER_PAIRING_MODE,     // 清列表进入配对模式

    UI_EVENT_XEAR_SURROUND_ON,
    UI_EVENT_XEAR_SURROUND_OFF,
    UI_EVENT_XEAR_NR_ON,
    UI_EVENT_XEAR_NR_OFF,

};


enum CFG_TYPE_EVENT_NOTIFY_OPTIONS
{
    EVENT_NOTIFY_OPTIONS_NONE = 0,  // 无

    EVENT_NOTIFY_TWS_UNPAIRED = (1 << 0),  // 未组对
    EVENT_NOTIFY_TWS_PAIRED   = (1 << 1),  // 已组对
    EVENT_NOTIFY_CHANNEL_L    = (1 << 2),  // L 左设备
    EVENT_NOTIFY_CHANNEL_R    = (1 << 3),  // R 右设备
    EVENT_NOTIFY_VOICE_FIRST  = (1 << 4),  // 先播报语音
    EVENT_NOTIFY_NO_WAIT_LED  = (1 << 5),  // 不等待 LED
};


enum CFG_TYPE_BAT_CHARGE_MODE
{
    BAT_BACK_CHARGE_MODE,   // 后台充电模式
    BAT_FRONT_CHARGE_MODE,  // 前台充电模式
};

enum CFG_TYPE_CHARGE_CURRENT
{
    CHARGE_CURRENT_10_MA  = 0x00,  // 10 mA
    CHARGE_CURRENT_20_MA  = 0x01,  // 20 mA
    CHARGE_CURRENT_30_MA  = 0x02,  // 30 mA
    CHARGE_CURRENT_40_MA  = 0x03,  // 40 mA
    CHARGE_CURRENT_60_MA  = 0x04,  // 60 mA
    CHARGE_CURRENT_80_MA  = 0x05,  // 80 mA
    CHARGE_CURRENT_100_MA  = 0x06,  // 100 mA
    CHARGE_CURRENT_120_MA  = 0x07,  // 120 mA
    CHARGE_CURRENT_140_MA  = 0x08,  // 140 mA
    CHARGE_CURRENT_150_MA = 0x09,  // 150 mA
    CHARGE_CURRENT_160_MA = 0x0A,  // 160 mA
    CHARGE_CURRENT_180_MA = 0x0B,  // 180 mA
    CHARGE_CURRENT_200_MA = 0x0C,  // 200 mA
    CHARGE_CURRENT_220_MA = 0x0D,  // 220 mA
    CHARGE_CURRENT_250_MA = 0x0E,  // 250 mA
    CHARGE_CURRENT_300_MA = 0x0F,  // 300 mA
};

enum CFG_TYPE_CHARGE_VOLTAGE
{
    CHARGE_VOLTAGE_4_20_V = 0x5,  // 4.20 V
    CHARGE_VOLTAGE_4_25_V = 0x9,  // 4.25 V
    CHARGE_VOLTAGE_4_30_V = 0xd,  // 4.30 V (新型电池)
    CHARGE_VOLTAGE_4_35_V = 0x11,  // 4.35 V (新型电池)
    CHARGE_VOLTAGE_4_40_V = 0x15,  // 4.40 V (新型电池)
    CHARGE_VOLTAGE_4_45_V = 0x19,// 4.45 V (新型电池)
    CHARGE_VOLTAGE_4_475_V = 0x1B,// 4.475 V (新型电池)
    CHARGE_VOLTAGE_4_50_V = 0x1D,// 4.50 V (新型电池)   
};

enum CFG_TYPE_PRECHARGE_STOP_VOLTAGE
{
    PRECHARGE_STOP_3_0_V = 3000,  // 3.0 V
    PRECHARGE_STOP_3_1_V = 3100,  // 3.1 V
    PRECHARGE_STOP_3_2_V = 3200,  // 3.2 V
    PRECHARGE_STOP_3_3_V = 3300,  // 3.3 V
};


enum CFG_TYPE_PRECHARGE_CURRENT
{
    PRECHARGE_CURRENT_5_PERCENT   = 0,    // 5%
    PRECHARGE_CURRENT_10_PERCENT  = 1,    // 10%
	PRECHARGE_CURRENT_15_PERCENT  = 2,    // 15%
	PRECHARGE_CURRENT_20_PERCENT  = 3,    // 20%
	PRECHARGE_CURRENT_25_PERCENT  = 4,    // 25%
	PRECHARGE_CURRENT_30_PERCENT  = 5,    // 30%
	PRECHARGE_CURRENT_35_PERCENT  = 6,    // 35%
	PRECHARGE_CURRENT_40_PERCENT  = 7,    // 40%
	PRECHARGE_CURRENT_45_PERCENT  = 8,    // 45%
	PRECHARGE_CURRENT_50_PERCENT  = 9,    // 50%
};

enum CFG_TYPE_PRECHARGE_CURRENT_MIN_LIMIT
{
    PRECHARGE_CURRENT_MIN_10_MA  = 0x00,  // 10 mA
    PRECHARGE_CURRENT_MIN_20_MA  = 0x01,  // 20 mA
    PRECHARGE_CURRENT_MIN_30_MA  = 0x02,  // 30 mA
    PRECHARGE_CURRENT_MIN_40_MA  = 0x03,  // 40 mA
    PRECHARGE_CURRENT_MIN_60_MA  = 0x04,  // 60 mA
    PRECHARGE_CURRENT_MIN_80_MA  = 0x05,  // 80 mA
    PRECHARGE_CURRENT_MIN_100_MA  = 0x06,  // 100 mA
    PRECHARGE_CURRENT_MIN_120_MA  = 0x07,  // 120 mA
};

enum CFG_TYPE_FASTCHARGE_VOLTAGE_THRESHOLD
{
    FAST_CHARGE_3_4_V = 0x0,  // 3.4 V
    FAST_CHARGE_3_5_V = 0x1,  // 3.5 V
    FAST_CHARGE_3_6_V = 0x2,  // 3.6 V
    FAST_CHARGE_3_7_V = 0x3,  // 3.7 V
    FAST_CHARGE_3_8_V = 0x4,  // 3.8 V
    FAST_CHARGE_3_9_V = 0x5,  // 3.9 V
    FAST_CHARGE_4_0_V = 0x6,  // 4.0 V
    FAST_CHARGE_4_1_V = 0x7,  // 4.1 V
};

enum CFG_TYPE_CHARGE_STOP_CURRENT
{
    CHARGE_STOP_CURRENT_20_PERCENT = 0,  // 20%
    CHARGE_STOP_CURRENT_5_PERCENT  = 1,  // 5%

    CHARGE_STOP_CURRENT_30_MA,   // 30 mA
    CHARGE_STOP_CURRENT_20_MA,   // 20 mA
    CHARGE_STOP_CURRENT_16_MA,   // 16 mA
    CHARGE_STOP_CURRENT_12_MA,   // 12 mA
    CHARGE_STOP_CURRENT_8_MA,    // 8 mA
    CHARGE_STOP_CURRENT_6_4_MA,  // 6.4 mA
    CHARGE_STOP_CURRENT_5_MA,    // 5 mA
};


enum CFG_TYPE_CHARGE_STOP_MODE
{
    CHARGE_STOP_BY_VOLTAGE = 0,              // 阈值电压
    CHARGE_STOP_BY_CURRENT = 1,              // 阈值电流
    CHARGE_STOP_BY_VOLTAGE_AND_CURRENT = 2,  // 阈值电压和电流
};


enum CFG_TYPE_BATTERY_LOW_VOLTAGE
{
    BATTERY_LOW_3_0_V = 3000,  // 3.0 V
    BATTERY_LOW_3_1_V = 3100,  // 3.1 V
    BATTERY_LOW_3_2_V = 3200,  // 3.2 V
    BATTERY_LOW_3_3_V = 3300,  // 3.3 V
    BATTERY_LOW_3_4_V = 3400,  // 3.4 V
    BATTERY_LOW_3_5_V = 3500,  // 3.5 V
    BATTERY_LOW_3_6_V = 3600,  // 3.6 V
};


enum CFG_TYPE_DC5VPD_CURRENT
{
    DC5VPD_CURRENT_DISABLE = 0xff,  // 禁止下拉
    DC5VPD_CURRENT_2_5_MA  = 0x0,   // 2.5 mA
    DC5VPD_CURRENT_7_5_MA  = 0x1,   // 7.5 mA
    DC5VPD_CURRENT_15_MA   = 0x2,   // 15 mA
    DC5VPD_CURRENT_25_MA   = 0x3,   // 25 mA
};



enum CFG_TYPE_DC5VLV_LEVEL
{
    DC5VLV_LEVEL_0_2_V = 0,
    DC5VLV_LEVEL_0_5_V = 1,
    DC5VLV_LEVEL_1_0_V = 2,
    DC5VLV_LEVEL_1_5_V = 3,
    DC5VLV_LEVEL_2_0_V = 4,
    DC5VLV_LEVEL_2_5_V = 5,
    DC5VLV_LEVEL_3_0_V = 6,
    DC5VLV_LEVEL_4_5_V = 7,
};


enum CFG_TYPE_BAT_RECHARGE_THRESHOLD
{
    BAT_RECHARGE_3_4_V = 0x0,  // 3.4 V
    BAT_RECHARGE_3_5_V = 0x1,  // 3.5 V
    BAT_RECHARGE_3_6_V = 0x2,  // 3.6 V
    BAT_RECHARGE_3_7_V = 0x3,  // 3.7 V
    BAT_RECHARGE_3_8_V = 0x4,  // 3.8 V
    BAT_RECHARGE_3_9_V = 0x5,  // 3.9 V
    BAT_RECHARGE_4_0_V = 0x6,  // 4.0 V
    BAT_RECHARGE_4_1_V = 0x7,  // 4.1 V
};



enum CFG_TYPE_BT_SUPPORT_FEATURES
{
    BT_SUPPORT_A2DP_AAC              = (1 << 0),   // 支持 AAC 音频格式
    BT_SUPPORT_A2DP_DTCP             = (1 << 1),   // 支持 DTCP 内容保护
    BT_SUPPORT_A2DP_SCMS_T           = (1 << 2),   // 支持 SCMS-T 内容保护
    BT_SUPPORT_AVRCP_VOLUME_SYNC     = (1 << 3),   // 支持 AVRCP 音量同步
    BT_SUPPORT_HFP_VOLUME_SYNC       = (1 << 4),   // 支持 HFP 音量同步
    BT_SUPPORT_HFP_BATTERY_REPORT    = (1 << 5),   // 支持 HFP 电量上报
    BT_SUPPORT_HFP_3WAY_CALL         = (1 << 6),   // 支持 HFP 三方通话
    BT_SUPPORT_HFP_VOICE_ASSIST      = (1 << 7),  // 支持 Siri 等语音助手
    BT_SUPPORT_HFP_CODEC_NEGOTIATION = (1 << 8),  // 支持 HFP 音频格式协商
    BT_SUPPORT_ENABLE_NREC           = (1 << 9),  // 启用 NREC 手机端回音取消和噪声削弱
    BT_SUPPORT_TWS                   = (1 << 10),  // 支持 TWS功能
    BT_SUPPORT_ENABLE_SNIFF          = (1 << 11),  // 允许 Sniff 模式
    BT_SUPPORT_DUAL_PHONE_DEV_LINK   = (1 << 12),  // 支持双手机设备连接
    BT_SUPPORT_LINKKEY_MISS_REJECT   = (1 << 13),  // 取消配对后回连不弹出提示框
    BT_SUPPORT_CLEAR_LINKKEY         = (1 << 14),  // 清除配对列表同时清除 LINKKEY
};

enum CFG_TYPE_BT_CALL_RING_MODE
{
    BT_CALL_RING_MODE_DEFAULT = 0,  // 默认
    BT_CALL_RING_MODE_REMOTE  = 1,  // 已有来电铃声时不播放提示音
    BT_CALL_RING_MODE_LOCAL   = 2,  // 只播放本地提示音
};

enum CFG_TYPE_BT_AUTO_RECONNECT
{
    AUTO_RECONNECT_PHONE_BY_STARTUP = (1 << 0),  // 开机自动回连手机设备
    AUTO_RECONNECT_PHONE_BY_TIMEOUT = (1 << 1),  // 超时断开自动回连手机设备
};


enum CFG_TYPE_BT_CTRL_TEST_MODE
{
    BT_CTRL_DISABLE_TEST    = 0,  // DISABLE
    BT_CTRL_DUT_TEST        = 1,  // DUT_TEST
    BT_CTRL_LE_TEST         = 2,  // LE_TEST
    BT_CTRL_DUT_AND_LE_TEST = 3,  // DUT_TEST & LE_TEST
};


enum CFG_TYPE_BT_HID_KEY_TYPE
{
    BT_HID_KEY_TYPE_KEYBOARD         = 0x01,  // Keyboard
    BT_HID_KEY_TYPE_CUSTOMER_CONTROL = 0x03,  // Customer Control
};


enum CFG_TYPE_TWS_PAIR_KEY_MODE
{
    TWS_PAIR_KEY_MODE_ONE = 1,  // 单方按键组对模式
    TWS_PAIR_KEY_MODE_TWO = 2,  // 双方按键组对模式
};


enum CFG_TYPE_TWS_MATCH_MODE
{
    TWS_MATCH_NAME = 0,  // 名称匹配
    TWS_MATCH_ID   = 1,  // ID 匹配
};


enum CFG_TYPE_TWS_SYNC_MODE
{
    TWS_SYNC_KEY_TONE  = (1 << 0),  // 同步按键音
    TWS_SYNC_POWER_OFF = (1 << 1),  // 同步关机
};


enum CFG_TYPE_CAP_TEMP
{
    CAP_TEMP_NA   = 0x7F,        // NA
    CAP_TEMP_N_40 = 0x100 - 40,  // -40
    CAP_TEMP_N_35 = 0x100 - 35,  // -35
    CAP_TEMP_N_30 = 0x100 - 30,  // -30
    CAP_TEMP_N_25 = 0x100 - 25,  // -25
    CAP_TEMP_N_20 = 0x100 - 20,  // -20
    CAP_TEMP_N_15 = 0x100 - 15,  // -15
    CAP_TEMP_N_10 = 0x100 - 10,  // -10
    CAP_TEMP_N_5  = 0x100 - 5,   // -5
    CAP_TEMP_0    = 0,           // 0
    CAP_TEMP_P_5  = 5,           // +5
    CAP_TEMP_P_10 = 10,          // +10
    CAP_TEMP_P_15 = 15,          // +15
    CAP_TEMP_P_20 = 20,          // +20
    CAP_TEMP_P_25 = 25,          // +25
    CAP_TEMP_P_30 = 30,          // +30
    CAP_TEMP_P_35 = 35,          // +35
    CAP_TEMP_P_40 = 40,          // +40
    CAP_TEMP_P_45 = 45,          // +45
    CAP_TEMP_P_50 = 50,          // +50
    CAP_TEMP_P_55 = 55,          // +55
    CAP_TEMP_P_60 = 60,          // +60
    CAP_TEMP_P_65 = 65,          // +65
    CAP_TEMP_P_70 = 70,          // +70
    CAP_TEMP_P_75 = 75,          // +75
    CAP_TEMP_P_80 = 80,          // +80
};




enum CFG_TYPE_EXTERN_PA_FUNC
{
    EXTERN_PA_NONE   = 0,  // PA_NONE
    EXTERN_PA_ENABLE = 1,  // PA_ENABLE
    EXTERN_PA_MUTE   = 2,  // PA_MUTE
};


enum CFG_TYPE_TAP_CTRL_SELECT
{
    TAP_CTRL_NONE  = 0,  // NONE
    TAP_CTRL_DA230 = 1,  // DA230
};


enum CFG_TYPE_BT_SCAN_MODE
{
    CFG_DEFAULT_INQUIRY_PAGE_SCAN_MODE,
    CFG_FAST_PAGE_SCAN_MODE,
    CFG_FAST_PAGE_SCAN_MODE_EX,
    CFG_NORMAL_PAGE_SCAN_MODE,
    CFG_NORMAL_PAGE_SCAN_MODE_S3,
    CFG_NORMAL_PAGE_SCAN_MODE_EX,
    CFG_FAST_INQUIRY_PAGE_SCAN_MODE
};


enum CFG_TYPE_BLE_ADV_TYPE
{
    CFG_ADV_DISABLE              = 0xff,
    CFG_ADV_IND                  = 0x00,
    CFG_ADV_DIRECT_IND_HIGH_DUTY = 0x01,
    CFG_ADV_SCAN_IND             = 0x02,
    CFG_ADV_NONCONN_IND          = 0x03,
    CFG_ADV_DIRECT_IND_LOW_DUTY  = 0x04,
};


enum CFG_TYPE_BLE_ADDR_TYPE
{
    CFG_BLE_PUBLIC_DEVICE_ADDRESS          = 0,  // Public Device Address
    CFG_BLE_STATIC_DEVICE_ADDRESS          = 1,  // Static Device Address
    CFG_BLE_NON_RESOLVABLE_PRIVATE_ADDRESS = 2,  // Non-resolvable Private Address
};


enum CFG_TYPE_LE_AUDIO_WORK_MODE
{
    CFG_LE_AUDIO_WORK_COEXISTENCE  = 0x0,  // 共存模式
    CFG_LE_AUDIO_WORK_INDEPENDENCE = 0x1,  // 独立模式
};


enum CFG_TYPE_AAP_KEY_MAP
{
    AAP_KEY_MAP_NONE         = 0x0,  // 关闭
    AAP_KEY_MAP_VOICE_ASSIST = 0x1,  // 语音助手
    AAP_KEY_MAP_PLAY_PAUSE   = 0x2,  // 播放/暂停
    AAP_KEY_MAP_NEXT_MUSIC   = 0x3,  // 下一曲
    AAP_KEY_MAP_PREV_MUSIC   = 0x4,  // 上一曲
    AAP_KEY_MAP_NOISE_CTRL   = 0x5,  // 噪声控制
};


enum CFG_TYPE_DC5V_UART_SWITCH_VOLT
{
    DC5V_UART_SWITCH_VOLT_NA    = 0x0,  // NA
    DC5V_UART_SWITCH_VOLT_2_0_V = 0x4,	// 2.0 V
    DC5V_UART_SWITCH_VOLT_3_0_V = 0x5,	// 3.0 V
    DC5V_UART_SWITCH_VOLT_4_0_V = 0x6,	// 4.0 V
    DC5V_UART_SWITCH_VOLT_4_5_V = 0x7,  // 4.5 V
};


enum CFG_TYPE_UART_PARITY_BIT
{
    UART_PARITY_BIT_NONE = 0x0,  // 无
    UART_PARITY_BIT_ODD  = 0x4,  // 奇
    UART_PARITY_BIT_EVEN = 0x6,  // 偶
    UART_PARITY_BIT_0    = 0x7,  // 0
    UART_PARITY_BIT_1    = 0x5,  // 1
};


enum CFG_TYPE_ANALOG_GAIN_MAP
{
    ANALOG_GAIN_RUDUCE_NA   = 0x0,  // NA
    ANALOG_GAIN_RUDUCE_0DB  = 0x4,  // 0 db
    ANALOG_GAIN_RUDUCE_3DB  = 0x3,  // -3 db
    ANALOG_GAIN_RUDUCE_6DB  = 0x2,  // -6 db
    ANALOG_GAIN_RUDUCE_12DB = 0x1,  // -12 db
};


enum CFG_VOICE_SAMPLE_MODE
{
    VOICE_SAMPLE_MODE_FOLLOW_TTS = 0, // 提示音采样率跟随TTS
    VOICE_SAMPLE_MODE_BEST_QUALITY,   // 提示音采样率最好品质
};


enum CFG_MIC_SAMPLE_MODE
{
    MIC_SAMPLE_MODE_FIX = 0,    // 采样率固定16KHz
    MIC_SAMPLE_MODE_FOLLOW,     // 采样率跟随播放
};


enum CFG_TYPE_MIC_CHANNEL_SELECT
{
    MIC_CHANNEL_LEFT = 0,    // 左声道MIC
    MIC_CHANNEL_RIHGT,       // 右声道MIC
};


enum CFG_TYPE_BTMUSIC_PLC_MODE
{
    BMPLC_MODE_DISABLED = 0,    // 关闭
    BMPLC_MODE_EMPTY_FRAME,     // 静音帧补偿
    BMPLC_MODE_HISTORY_FRAME,   // 历史帧补偿
};


enum CFG_TYPE_BTCALL_LCY_OPTLEVEL
{
    BSLCY_OPT_DISABLED = 0x0,   // Disabled
    BSLCY_OPT_LEVEL_1  = 0x1,   // Level 1 (basic)
    BSLCY_OPT_LEVEL_2  = 0x3,   // Level 2
    BSLCY_OPT_LEVEL_3  = 0x5,   // Level 3
    BSLCY_OPT_LEVEL_4  = 0x7,   // Level 4
    BSLCY_OPT_LEVEL_5  = 0x9,   // Level 5
    BSLCY_OPT_LEVEL_6  = 0xb,   // Level 6
    BSLCY_OPT_LEVEL_7  = 0xd,   // Level 7
    BSLCY_OPT_LEVEL_8  = 0xf,   // Level 8
};


/*-----------------------------------------------------------------------------
 * 配置结构类型定义
 * 类型必须以 CFG_XXX 命名
 *---------------------------------------------------------------------------*/


typedef struct  // LRADC 按键
{
    cfg_uint8  Key_Value;  // 键值, CFG_TYPE_KEY_VALUE
    cfg_uint16  ADC_Min;    // ADC 阈值下限, 0x00 ~ 0xFFFF
    cfg_uint16  ADC_Max;    // ADC 阈值上限, 0x00 ~ 0xFFFF

} CFG_Type_LRADC_Key;


typedef struct  // GPIO 按键
{
    cfg_uint8   Key_Value;         // 键值,      CFG_TYPE_KEY_VALUE
    cfg_uint8   GPIO_Pin;          // GPIO 管脚, CFG_TYPE_GPIO_PIN
    cfg_uint8   Pull_Up_Down;      // 上下拉,    CFG_TYPE_GPIO_PULL
    cfg_uint8   Active_Level;      // 有效电平,  CFG_TYPE_GPIO_LEVEL
    cfg_uint16  Debounce_Time_Ms;  // 去抖时间 (毫秒), 0 ~ 500

} CFG_Type_GPIO_Key;


typedef struct  // 按键功能映射
{
    cfg_uint8   Key_Func;   // 按键功能, CFG_TYPE_KEY_FUNC
    cfg_uint8   Key_Value;  // 键值,     CFG_TYPE_KEY_VALUE
    cfg_uint8   LR_Device;  // 左右设备, CFG_TYPE_KEY_DEVICE_TYPE
    cfg_uint16  Key_Event;  // 按键消息, CFG_TYPE_KEY_EVENT
} CFG_Type_Key_Func_Map;


typedef struct  // 组合按键
{
    cfg_uint8   Key_Func;     // 按键功能, CFG_TYPE_KEY_FUNC
    cfg_uint8   Key_Value_1;  // 键值 1,   CFG_TYPE_KEY_VALUE
    cfg_uint8   Key_Value_2;  // 键值 2,   CFG_TYPE_KEY_VALUE
    cfg_uint8   LR_Device;    // 左右设备, CFG_TYPE_KEY_DEVICE_TYPE
    cfg_uint16  Key_Event;    // 按键消息, CFG_TYPE_KEY_EVENT
} CFG_Type_Combo_Key_Map;


typedef struct  // LED 驱动
{
    cfg_uint8  LED_No;        // LED 编号,  CFG_TYPE_LED_NO
    cfg_uint8  GPIO_Pin;      // GPIO 管脚, CFG_TYPE_LED_GPIO_PIN
    cfg_uint8  Active_Level;  // 有效电平,  CFG_TYPE_GPIO_LEVEL

} CFG_Type_LED_Drive;


typedef struct  // LED 显示模式
{
    cfg_uint8   Display_Model;      // 显示模式,            CFG_TYPE_LED_DISPLAY_MODEL
    cfg_uint8   Display_LEDs;       // 显示 LED,            CFG_TYPE_LED_NO
    cfg_uint8   Disable_LEDs;       // 关闭 LED,            CFG_TYPE_LED_NO
    cfg_uint8   Use_PWM_Control;    // 使用 PWM 控制模式,   CFG_TYPE_BOOL, 使用 PWM GPIO 才有效
    cfg_uint16  Delay_Time_Ms;      // 延迟亮灯时间 (毫秒), 0 ~ 10000
    cfg_uint16  ON_Time_Ms;         // 亮灯时间 (毫秒),     0 ~ 60000
    cfg_uint16  OFF_Time_Ms;        // 灭灯时间 (毫秒),     0 ~ 60000
    cfg_uint8   Flash_Count;        // 亮灭闪烁次数,        0 ~ 200, 设置为 0 时无限闪烁
    cfg_uint8   Loop_Count;         // 闪烁循环次数,        0 ~ 200, 设置为 0 时无限循环
    cfg_uint16  Loop_Wait_Time_Ms;  // 循环等待时间 (毫秒), 0 ~ 60000
    cfg_uint16  Breath_Time_Ms;     // 呼吸模式时间 (毫秒), 0 ~ 2000, 使用 PWM 控制模式才有效

} CFG_Type_LED_Display_Model;


typedef struct  // DMIC 管脚配置
{
    cfg_uint16  DMIC01_CLK;  // DMIC01 CLK 管脚, CFG_TYPE_DMIC01_CLK_PIN
    cfg_uint16  DMIC01_DAT;  // DMIC01 DAT 管脚, CFG_TYPE_DMIC01_DAT_PIN
} CFG_Type_DMIC_Select_GPIO;


typedef struct  // I2S 管脚配置
{
    cfg_uint16  I2S_MCLK;   // I2S MCLK 管脚,  CFG_TYPE_GPIO_PIN
    cfg_uint16  I2S_LRCLK;  // I2S LRCLK 管脚, CFG_TYPE_GPIO_PIN
    cfg_uint16  I2S_BCLK;   // I2S BCLK 管脚,  CFG_TYPE_GPIO_PIN
    cfg_uint16  I2S_DOUT;   // I2S DOUT 管脚,  CFG_TYPE_GPIO_PIN

} CFG_Type_I2S_Select_GPIO;



typedef struct  // GPIO 选择声道
{
    cfg_uint8  GPIO_Pin;      // GPIO 管脚, CFG_TYPE_GPIO_PIN
    cfg_uint8  Pull_Up_Down;  // 上下拉,    CFG_TYPE_GPIO_PULL
    cfg_uint8  Active_Level;  // 有效电平,  CFG_TYPE_GPIO_LEVEL

} CFG_Type_Channel_Select_GPIO;


typedef struct  // LRADC 选择声道
{
    cfg_uint32  LRADC_Ctrl;     // LRADC 控制器,   CFG_TYPE_LRADC_CTRL
    cfg_uint8   LRADC_Pull_Up;  // LRADC 上拉电阻, CFG_TYPE_LRADC_PULL_UP
    cfg_uint16  ADC_Min;        // ADC 阈值下限
    cfg_uint16  ADC_Max;        // ADC 阈值上限

} CFG_Type_Channel_Select_LRADC;


typedef struct  // GPIO 检测 Linein
{
    cfg_uint8  GPIO_Pin;      // GPIO 管脚, CFG_TYPE_GPIO_PIN
    cfg_uint8  Pull_Up_Down;  // 上下拉,    CFG_TYPE_GPIO_PULL
    cfg_uint8  Active_Level;  // 有效电平,  CFG_TYPE_GPIO_LEVEL

} CFG_Type_Linein_Detect_GPIO;


typedef struct  // LRADC 检测 Linein
{
    cfg_uint32  LRADC_Ctrl;     // LRADC 控制器,   CFG_TYPE_LRADC_CTRL
    cfg_uint8   LRADC_Pull_Up;  // LRADC 上拉电阻, CFG_TYPE_LRADC_PULL_UP
    cfg_uint16  ADC_Min;        // ADC 阈值下限
    cfg_uint16  ADC_Max;        // ADC 阈值上限

} CFG_Type_Linein_Detect_LRADC;


typedef struct  // 提示音编号名称
{
    cfg_uint8  Tone_ID;                           // 编号, CFG_TYPE_TONE_ID
    cfg_uint8  Tone_Name[CFG_MAX_TONE_NAME_LEN];  // 文件名

} CFG_Type_Tone_ID_Name;


typedef struct  // 语音编号名称
{
    cfg_uint8  Voice_ID;                            // 编号, CFG_TYPE_VOICE_ID
    cfg_uint8  Voice_Name[CFG_MAX_VOICE_NAME_LEN];  // 文件名

} CFG_Type_Voice_ID_Name;


typedef struct  // 数字语音编号名称
{
    cfg_uint8  Voice_ID;                            // 编号, CFG_TYPE_NUMERIC_VOICE_ID
    cfg_uint8  Voice_Name[CFG_MAX_VOICE_NAME_LEN];  // 文件名

} CFG_Type_Numeric_Voice_ID_Name;


typedef struct  // 事件通知
{
    cfg_uint8  Event_Type;    // 事件类型,     CFG_TYPE_SYS_EVENT
    cfg_uint8  LED_Display;   // LED 显示模式, CFG_TYPE_LED_DISPLAY_MODEL
    cfg_uint8  LED_Override;  // LED 显示覆盖, CFG_TYPE_LED_OVERRIDE_MODE, 多状态下靠前的显示优先
    cfg_uint8  Tone_Play;     // 提示音,       CFG_TYPE_TONE_ID
    cfg_uint8  Voice_Play;    // 播报语音,     CFG_TYPE_VOICE_ID
    cfg_uint8  Options;       // 选项,         CFG_TYPE_EVENT_NOTIFY_OPTIONS

} CFG_Type_Event_Notify;


typedef struct  // 温度补偿
{
    cfg_uint8  Cap_Temp;  // 温度 ℃, CFG_TYPE_CAP_TEMP
    cfg_int8   Cap_Comp;  // 补偿 pF, -10.0 ~ 10.0

} CFG_Type_Cap_Temp_Comp;




typedef struct  // MIC 增益
{
    cfg_uint16  ADC0_aGain;    // ADC0 模拟增益, CFG_TYPE_MIC_AGAIN
    cfg_uint16  ADC0_dGain;    // ADC0 数字增益, CFG_TYPE_MIC_DGAIN
    cfg_uint16  ADC1_aGain;    // ADC1 模拟增益, CFG_TYPE_MIC_AGAIN
    cfg_uint16  ADC1_dGain;    // ADC1 数字增益, CFG_TYPE_MIC_DGAIN
} CFG_Type_MIC_Gain;


typedef struct  // 外部 PA 控制
{
    cfg_uint8  PA_Function;   // 外部 PA 功能, CFG_TYPE_EXTERN_PA_FUNC
    cfg_uint8  GPIO_Pin;      // GPIO 管脚,    CFG_TYPE_GPIO_PIN
    cfg_uint8  Pull_Up_Down;  // 上下拉,       CFG_TYPE_GPIO_PULL
    cfg_uint8  Active_Level;  // 有效电平,     CFG_TYPE_GPIO_LEVEL

} CFG_Type_Extern_PA_Control;


typedef struct  // 敲击按键支持
{
    cfg_uint8   Tap_Ctrl_Select;           // 敲击支持选择,   CFG_TYPE_TAP_CTRL_SELECT
    cfg_uint8   SDA_Pin;                   // SDA 管脚,       CFG_TYPE_GPIO_PIN
    cfg_uint8   SCL_Pin;                   // SCL 管脚,       CFG_TYPE_GPIO_PIN
    cfg_uint8   I2C_Pull_Up;               // I2C 上拉,       CFG_TYPE_GPIO_PULL, 可选择上拉 10K
    cfg_uint8   I2C_Device_Address;        // I2C 设备地址
    cfg_uint32  I2C_Bitrate;               // I2C 比特率,     20000 ~ 250000
    cfg_uint8   INT1_Pin;                  // INT1 管脚,      CFG_TYPE_GPIO_PIN
    cfg_uint8   INT1_Pull_Up_Down;         // INT1 上下拉,    CFG_TYPE_GPIO_PULL
    cfg_uint8   INT1_Active_Level;         // INT1 有效电平,  CFG_TYPE_GPIO_LEVEL
    cfg_uint8   First_Tap_Sensitivity;     // 开始敲击灵敏度, 0 ~ 31, 配置为 13 左右, 值越小越灵敏
    cfg_uint8   Continue_Tap_Sensitivity;  // 连续敲击灵敏度, 0 ~ 31, 配置为 6 左右
    cfg_uint8   Enable_Single_Tap;         // 允许单击操作,   CFG_TYPE_BOOL
    cfg_uint8   Tap_Key_Tone;              // 敲击按键音,     CFG_TYPE_TONE_ID
    cfg_uint8   Support_INT_Wake_Up;       // 支持敲击唤醒,   CFG_TYPE_BOOL

} CFG_Type_Tap_Key_Control;



typedef struct  // SCAN 参数设置
{
    cfg_uint8   Scan_Mode;              // 模式, CFG_TYPE_BT_SCAN_MODE
    cfg_uint16  Inquiry_Scan_Window;    // InquiryScan 窗口
    cfg_uint16  Inquiry_Scan_Interval;  // InquiryScan 间隔
    cfg_uint8   Inquiry_Scan_Type;      // InquiryScan 类型, 0 ~ 1
    cfg_uint16  Page_Scan_Window;       // PageScan 窗口
    cfg_uint16  Page_Scan_Interval;     // PageScan 间隔
    cfg_uint8   Page_Scan_Type;         // PageScan 类型, 0 ~ 1

} CFG_Type_BT_Scan_Params;



typedef struct  // 自定义按键序列
{
    cfg_uint16  Key_Sequence;  // 按键序列, CFG_TYPE_CUSTOMED_KEY_SEQUENCE

    cfg_uint16  Key_Event_1;   // 消息 1, CFG_TYPE_SEQ1_KEY_EVENT
    cfg_uint16  Key_Event_2;   // 消息 2, CFG_TYPE_SEQ2_KEY_EVENT

} CFG_Type_Customed_Key_Sequence;


typedef struct  // 更多充电设置
{
    cfg_uint8  Charger_Box_Standby_Current;  // 充电盒待机电流 (mA), 0 ~ 10

} CFG_Type_Battery_Charge_Settings_Ex;


typedef struct  // DC5V_COM 通讯设置
{
    cfg_uint8   Enable_DC5V_UART_Comm_Mode;  // 启用 DC5V_COM 通讯模式, CFG_TYPE_BOOL
    cfg_uint8   DC5V_UART_Switch_Voltage;    // DC5V_COM 切换电压,      CFG_TYPE_DC5V_UART_SWITCH_VOLT
    cfg_uint8   Redirect_Console_Print;      // 重定向控制台打印,       CFG_TYPE_BOOL
    cfg_uint8   DC5V_UART_Parity_Select;     // DC5V_COM 奇偶校验位,    CFG_TYPE_UART_PARITY_BIT
    cfg_uint32  DC5V_UART_Comm_Baudrate;     // DC5V_COM 通讯波特率 (bps)

} CFG_Type_DC5V_UART_Comm_Settings;



typedef struct  // DC5V_IO 通讯设置
{
    cfg_uint16  DC5V_IO_Threshold_MV;  // DC5V_IO 阈值电压 (毫伏), 0 表示禁用该功能

} CFG_Type_DC5V_IO_Comm_Settings;

typedef struct  // 自动退出控制器测试模式
{
    cfg_uint16  Quit_Timer_Sec;        // 定时退出 (秒), 0 表示不自动退出
    cfg_uint8   Power_Off_After_Quit;  // 退出后关机, CFG_TYPE_BOOL

} CFG_Type_Auto_Quit_BT_Ctrl_Test;




typedef struct  // NTC 温度调节充电电流
{
    cfg_uint8   Enable_NTC;        // 启用 NTC 温度调节充电电流, CFG_TYPE_BOOL
    cfg_uint32  LRADC_Ctrl;        // LRADC 控制器,     CFG_TYPE_LRADC_CTRL
    cfg_uint8   LRADC_Pull_Up;     // LRADC 上拉电阻,   CFG_TYPE_LRADC_PULL_UP
    cfg_uint8   LRADC_Value_Test;  // LRADC 采样值测试, CFG_TYPE_BOOL, 通过串口打印 LRADC 采样值
    cfg_uint16  NTC_Det_Debounce_Time_S;  // NTC检测去抖时间(秒), 1 ~ 20
} CFG_Type_NTC_Settings;


typedef struct  // NTC 温度范围
{
    cfg_uint16  ADC_Min;                 // ADC 阈值下限
    cfg_uint16  ADC_Max;                 // ADC 阈值上限
    cfg_uint8   Adjust_Current_Percent;  // 调节充电电流 (百分比), 0 ~ 100
    cfg_uint8   Adjust_CV_Level;               // 调节CV等级, CFG_TYPE_CHARGE_VOLTAGE
    cfg_uint16  Charge_Time_Limit;       // 允许最长充电时间 (分钟), 0 表示该温度区间停止充电

} CFG_Type_NTC_Range;


typedef struct  // 低延时模式基础设置
{
    cfg_uint8   Default_Low_Latency_Mode;  // 默认低延迟模式, CFG_TYPE_BOOL
    cfg_uint8   Save_Low_Latency_Mode;     // 保存低延迟模式, CFG_TYPE_BOOL, 不保存时重新开机恢复默认模式

    cfg_uint16  SBC_Threshold;   // [音乐] SBC 格式播放延迟时间 ms,  0 ~ 150, 0 表示默认值 60, 非0时不能小于40
    cfg_uint16  AAC_Threshold;   // [音乐] AAC 格式播放延迟时间 ms,  0 ~ 150, 0 表示默认值 60, 非0时不能小于50

    cfg_uint16  MSBC_Threshold;  // [通话] MSBC 格式播放延迟时间 ms, 0 ~ 150, 0 表示默认值 60, 非0时不能小于30
    cfg_uint16  CVSD_Threshold;  // [通话] CVSD 格式播放延迟时间 ms, 0 ~ 150, 0 表示默认值 60, 非0时不能小于30

} CFG_Type_LLM_Basic;

typedef struct  // RF Param
{
    cfg_uint16  Addr;        // ADDR[0x0~0xff]
    cfg_uint16  Value;      // VAL[0x0~0xffff]
} CFG_Type_RF_Param;

enum CFG_TYPE_BTCALL_ALSP
{
    BTCALL_ALSP_POST_AGC      = (1 << 0),   // 后处理-AGC
    BTCALL_ALSP_PRE_ENC       = (1 << 1),   // 前处理-降噪
    BTCALL_ALSP_PRE_AI_ENC    = (1 << 2),   // 前处理-AI降噪
    BTCALL_ALSP_PRE_CNG       = (1 << 3),   // 前处理-舒适噪声
};

enum CFG_TYPE_BTCALL_NOISE_REDUCE
{
    BTCALL_NOISE_SPEAKER_NR      = (1 << 0),   // speaker nr
    BTCALL_NOISE_TAIL_NR         = (1 << 1),   // tail_nr
};

enum CFG_TYPE_LC3_INTERVAL
{
    LC3_INTERVAL_10000US = 0,   // 10ms
    LC3_INTERVAL_7500US,     	// 7.5ms
};

enum CFG_TYPE_LC3_BIT_WIDTH
{
    LC3_BIT_WIDTH_16 = 0,   // 16位
    LC3_BIT_WIDTH_24,     	// 24位
};

enum CFG_TYPE_LC3_MUSIC_2CH_BITRATE
{
    LC3_BITRATE_MUSIC_2CH_192KBPS = 0,  // 192kbps
    LC3_BITRATE_MUSIC_2CH_184KBPS,     	// 184kbps
	LC3_BITRATE_MUSIC_2CH_176KBPS,     	// 176kbps
	LC3_BITRATE_MUSIC_2CH_160KBPS,     	// 160kbps
	LC3_BITRATE_MUSIC_2CH_158KBPS,     	// 158kbps
	LC3_BITRATE_MUSIC_2CH_COUNT,		// 数目
};

enum CFG_TYPE_LC3_MUSIC_1CH_BITRATE
{
	LC3_BITRATE_MUSIC_1CH_96KBPS = 0,   // 96kbps
	LC3_BITRATE_MUSIC_1CH_80KBPS,   	// 80kbps
	LC3_BITRATE_MUSIC_1CH_78KBPS,     	// 78kbps
	LC3_BITRATE_MUSIC_1CH_COUNT,		// 数目
};

enum CFG_TYPE_LC3_CALL_1CH_BITRATE
{
	LC3_BITRATE_CALL_1CH_80KBPS = 0,    // 80kbps
	LC3_BITRATE_CALL_1CH_78KBPS,     	// 78kbps
	LC3_BITRATE_CALL_1CH_64KBPS,     	// 64kbps
	LC3_BITRATE_CALL_1CH_48KBPS,     	// 48kbps
	LC3_BITRATE_CALL_1CH_32KBPS,     	// 32kbps
	LC3_BITRATE_CALL_1CH_COUNT,			// 数目
};

enum CFG_TYPE_REF_GAIN_COEF26
{
    REF_GAIN_COEF26_0DB  = 0x4000000, // 0dB
    REF_GAIN_COEF26_1DB  = 0x47CED91, // 1dB
	REF_GAIN_COEF26_2DB  = 0x50918E2, // 2dB
	REF_GAIN_COEF26_3DB  = 0x5A65DF7, // 3dB
	REF_GAIN_COEF26_4DB  = 0x656D2E6, // 4dB
    REF_GAIN_COEF26_5DB  = 0x71CCEEE, // 5dB
	REF_GAIN_COEF26_6DB  = 0x8000000, // 6dB
	REF_GAIN_COEF26_7DB  = 0x8F9DB22, // 7dB
	REF_GAIN_COEF26_8DB  = 0xA1231C5, // 8dB
	REF_GAIN_COEF26_9DB  = 0xB4CBBEF, // 9dB
	REF_GAIN_COEF26_10DB = 0xCADA5CE, // 10dB
	REF_GAIN_COEF26_11DB = 0xE399DDF, // 11dB
	REF_GAIN_COEF26_12DB = 0x10000000, // 12dB
	REF_GAIN_COEF26_13DB = 0x11F3B645, // 13dB
	REF_GAIN_COEF26_14DB = 0x1424638B, // 14dB
	REF_GAIN_COEF26_15DB = 0x169977E1, // 15dB
	REF_GAIN_COEF26_16DB = 0x195B4B9F, // 16dB
	REF_GAIN_COEF26_17DB = 0x1C733BC2, // 17dB
	REF_GAIN_COEF26_18DB = 0x20000000, // 18dB
	REF_GAIN_COEF26_19DB = 0x23E76C8B, // 19dB
	REF_GAIN_COEF26_20DB = 0x2848C718, // 20dB
	REF_GAIN_COEF26_21DB = 0x2D32EFC4, // 21dB
	REF_GAIN_COEF26_22DB = 0x32B69741, // 22dB
	REF_GAIN_COEF26_23DB = 0x38E67787, // 23dB
	REF_GAIN_COEF26_24DB = 0x40000000, // 24dB
};

/*-----------------------------------------------------------------------------
 * 配置数据类定义
 * 类型必须以 CFG_XXX 命名
 * 类成员必须赋值
 *---------------------------------------------------------------------------*/


typedef struct  // 用户版本
{
    cfg_uint8  Version[CFG_MAX_USER_VERSION_LEN];  // 版本信息

} CFG_Struct_User_Version;


typedef struct  // 平台方案
{
    cfg_uint32  IC_Type;                           // IC 类型
    cfg_uint8   Board_Type;                        // 板型

    cfg_uint8   Case_Name[CFG_MAX_CASE_NAME_LEN];  // 方案名称

    cfg_uint8   Major_Version;                     // 主版本号
    cfg_uint8   Minor_Version;                     // 次版本号

} CFG_Struct_Platform_Case;


typedef struct  // 控制台串口
{
    cfg_uint16  TX_Pin;            // 输出管脚, CFG_TYPE_UART_TX_PIN
    cfg_uint16  RX_Pin;            // 输入管脚, CFG_TYPE_UART_RX_PIN
    cfg_uint32  Baudrate;          // 波特率 (bps)

    cfg_uint8   Print_Time_Stamp;  // 打印时间戳, CFG_TYPE_BOOL

} CFG_Struct_Console_UART;


typedef struct  // 系统设置
{
    cfg_uint16  Support_Features;                // 系统支持特性, CFG_TYPE_SYS_SUPPORT_FEATURES

    cfg_uint8   Auto_Power_Off_Mode;             // 自动关机模式, CFG_TYPE_AUTO_POWOFF_MODE

    cfg_uint16  Auto_Power_Off_Time_Sec;         // 自动关机时间 (秒), 0 ~ 900, 设置为 0 时禁止自动关机
    cfg_uint16  Auto_Standby_Time_Sec;           // 自动待机时间 (秒), 0 ~ 900, 设置为 0 时禁止自动待机

    cfg_uint8   Enable_Voice_Prompt_In_Calling;  // 通话中允许语音播报提示, CFG_TYPE_BOOL
    cfg_uint8   Default_Voice_Language;          // 默认语音语言, CFG_TYPE_VOICE_LANGUAGE

    cfg_uint8   Linein_Disable_Bluetooth;        // Linein 模式下禁用蓝牙功能, CFG_TYPE_BOOL

} CFG_Struct_System_Settings;



typedef struct  // OTA 设置
{
    cfg_uint8   Enable_Dongle_OTA_Erase_VRAM;   // Dongle OTA擦除用户区, CFG_TYPE_BOOL
    cfg_uint8   Enable_APP_OTA_Erase_VRAM;      // 发射机或APP OTA擦除用户区, CFG_TYPE_BOOL
    cfg_uint8   Enable_Single_OTA_Without_TWS;  // 未组队时允许单耳OTA, CFG_TYPE_BOOL
    cfg_uint8   Enable_Ver_Diff;                // 左右耳固件版本不同时，允许TWS OTA, CFG_TYPE_BOOL
    cfg_uint8   Enable_Ver_Low;                 // 关闭版本控制，版本号自动加1, CFG_TYPE_BOOL
    cfg_uint8	  Enable_Poweroff;                // OTA完成后关机, CFG_TYPE_BOOL
    cfg_uint8   Version_Number[12];             // 固件版本号, 例如 1.6.8, 2.6.3.4

} CFG_Struct_OTA_Settings;

typedef struct  // 固件烧录设置
{
    cfg_uint8   Keep_User_VRAM_Data_When_UART_Upgrade;    // 烧录固件时保留用户区数据, CFG_TYPE_BOOL
    cfg_uint8   Keep_Factory_VRAM_Data_When_ATT_Upgrade;  // 烧录固件时保留工厂区数据, CFG_TYPE_BOOL

} CFG_Struct_Factory_Settings;




typedef struct  // ONOFF 按键
{
    cfg_uint8   Use_Inner_ONOFF_Key;                           // 使用内部软 ONOFF 按键, CFG_TYPE_BOOL
    cfg_uint8   Continue_Key_Function_After_Wake_Up;           // 按键唤醒后允许继续响应按键功能, CFG_TYPE_BOOL, (内部软 ONOFF 按键)

    cfg_uint8   Key_Value;                                     // ONOFF 键值, CFG_TYPE_KEY_VALUE
    cfg_uint16  Time_Press_Power_On;                           // 按下开机, CFG_TYPE_ONOFF_PRESS_POWER_ON,   (内部软 ONOFF 按键)
    cfg_uint8   Time_Long_Press_Reset;                         // 长按复位, CFG_TYPE_ONOFF_LONG_PRESS_RESET, (内部软 ONOFF 按键)
    cfg_uint8   Onoff_pullr_sel;                               // 内部上下拉电阻, CFG_TYPE_ONOFF_PULLR_SEL,     (内部软 ONOFF 按键)
    cfg_uint8   Boot_Hold_Key_Func;                            // 开机长按键功能, CFG_TYPE_BOOT_HOLD_KEY_FUNC
    cfg_uint16  Boot_Hold_Key_Time_Ms;                         // 开机长按键时间 (毫秒), 500 ~ 8000
    cfg_uint16  Debounce_Time_Ms;                              // 去抖时间 (毫秒),       0 ~ 100

    cfg_uint8   Reboot_After_Boot_Hold_Key_Clear_Paired_List;  // 开机长按键清除配对列表后自动重启, CFG_TYPE_BOOL

} CFG_Struct_ONOFF_Key;


typedef struct  // LRADC 按键
{
    CFG_Type_LRADC_Key  Key[CFG_MAX_LRADC_KEYS];  // 按键

    cfg_uint32  LRADC_Ctrl;                       // LRADC 控制器, CFG_TYPE_LRADC_CTRL

    cfg_uint8   LRADC_Pull_Up;                    // LRADC 上拉电阻, CFG_TYPE_LRADC_PULL_UP

    cfg_uint8   Use_LRADC_Key_Wake_Up;            // 使用 LRADC 按键唤醒, CFG_TYPE_BOOL
    cfg_uint8   LRADC_Value_Test;                 // LRADC 采样值测试, CFG_TYPE_BOOL, 通过串口打印 LRADC 采样值
    cfg_uint16  Debounce_Time_Ms;                 // 去抖时间 (毫秒),  0 ~ 100

} CFG_Struct_LRADC_Keys;


typedef struct  // GPIO 按键
{
    CFG_Type_GPIO_Key  Key[CFG_MAX_GPIO_KEYS];  // 按键

} CFG_Struct_GPIO_Keys;


typedef struct  // 敲击按键
{
    CFG_Type_Tap_Key_Control  Tap_Key_Control;  // 敲击按键支持

} CFG_Struct_Tap_Key;


typedef struct  // 按键响应门限参数
{
    cfg_uint16  Single_Click_Valid_Ms;    // 单击有效时间 (毫秒),     200 ~ 1000, 单击按下到抬起在该时间内有效
    cfg_uint16  Multi_Click_Interval_Ms;  // 多击间隔时间 (毫秒),     100 ~ 500
    cfg_uint16  Repeat_Start_Delay_Ms;    // 重复按键延迟时间 (毫秒), 0 ~ 1000,   按键按下该时间后开始重复按键
    cfg_uint16  Repeat_Interval_Ms;       // 重复按键间隔时间 (毫秒), 100 ~ 1000
    cfg_uint16  Long_Press_Time_Ms;       // 长按键时间 (毫秒),       500 ~ 5000
    cfg_uint16  Long_Long_Press_Time_Ms;  // 超长按键时间 (毫秒),     1000 ~ 10000
    cfg_uint16  Very_Long_Press_Time_Ms;  // 极长按键时间 (毫秒),     1500 ~ 20000

} CFG_Struct_Key_Threshold;


typedef struct  // 按键功能映射
{
    CFG_Type_Key_Func_Map  Map[CFG_MAX_KEY_FUNC_MAPS];  // 映射

} CFG_Struct_Key_Func_Maps;


typedef struct  // 组合按键映射
{
    CFG_Type_Combo_Key_Map  Map[CFG_MAX_COMBO_KEY_MAPS];  // 映射

} CFG_Struct_Combo_Key_Func_Maps;


typedef struct  // 自定义按键序列设置
{
    CFG_Type_Customed_Key_Sequence  Customed_Key_Sequence[2];  // 自定义按键序列


} CFG_Struct_Customed_Key_Sequence;


typedef struct  // LED 驱动
{
    CFG_Type_LED_Drive  LED[CFG_MAX_LEDS];  // LED

} CFG_Struct_LED_Drives;


typedef struct  // LED 显示模式
{
    CFG_Type_LED_Display_Model  Model[CFG_MAX_LED_DISPLAY_MODELS];  // 模式

} CFG_Struct_LED_Display_Models;


typedef struct  // 蓝牙音乐音量分级表
{
    cfg_uint16  Level[CFG_MAX_BT_MUSIC_VOLUME + 1];  // 分级, CFG_TYPE_VOLUME_GAIN

} CFG_Struct_BT_Music_Volume_Table;


typedef struct  // 蓝牙通话音量分级表
{
    cfg_uint16  Level[CFG_MAX_BT_CALL_VOLUME + 1];  // 分级, CFG_TYPE_VOLUME_GAIN

} CFG_Struct_BT_Call_Volume_Table;


typedef struct  // Linein 音量分级表
{
    cfg_uint16  Level[CFG_MAX_LINEIN_VOLUME + 1];  // 分级, CFG_TYPE_VOLUME_GAIN

} CFG_Struct_Linein_Volume_Table;


typedef struct  // 语音音量分级表
{
    cfg_uint16  Level[CFG_MAX_VOICE_VOLUME + 1];  // 分级, CFG_TYPE_VOLUME_GAIN

} CFG_Struct_Voice_Volume_Table;


typedef struct  // 音量设置
{
    cfg_uint8  Voice_Default_Volume;     // 语音默认音量, 固定语音音量大小使用该值,     0 ~ 16
    cfg_uint8  Voice_Min_Volume;         // 语音最小音量,     0 ~ 16
    cfg_uint8  Voice_Max_Volume;         // 语音最大音量,     0 ~ 16

    cfg_uint8  BT_Music_Default_Volume;  // 蓝牙音乐默认音量, 0 ~ 16
    cfg_uint8  BT_Call_Default_Volume;   // 蓝牙通话默认音量, 0 ~ 15
    cfg_uint8  BT_Music_Default_Vol_Ex;  // 蓝牙音乐默认音量 (用于不支持音量同步的设备), 0 ~ 16

    cfg_uint16 Tone_Volume_Decay;        // 按键音音量衰减, 0.0 ~ 30.0, 单位dB，仅按键音固定音量模式有效

} CFG_Struct_Volume_Settings;


typedef struct  // 音频设置
{
    cfg_uint8  Audio_Out_Mode;                            // 音频输出模式, CFG_TYPE_AUDIO_OUT_MODE

    CFG_Type_I2S_Select_GPIO  I2STX_Select_GPIO;          // I2S TX 管脚配置

    CFG_Type_I2S_Select_GPIO  I2SRX_Select_GPIO;          // I2S RX 管脚配置

    cfg_uint8  Channel_Select_Mode;                       // 声道选择模式, CFG_TYPE_CHANNEL_SELECT_MODE

    CFG_Type_Channel_Select_GPIO  Channel_Select_GPIO;    // GPIO 选择声道

    CFG_Type_Channel_Select_LRADC  Channel_Select_LRADC;  // LRADC 选择声道

    cfg_uint8  TWS_Alone_Audio_Channel;                   // TWS 未组对时声道选择, CFG_TYPE_TWS_ALONE_AUDIO_CHANNEL

    cfg_uint8  L_Speaker_Out;                             // 左声道喇叭输出, CFG_TYPE_SPEAKER_OUT_SELECT
    cfg_uint8  R_Speaker_Out;                             // 右声道喇叭输出, CFG_TYPE_SPEAKER_OUT_SELECT

    cfg_uint32  ADC_Bias_Setting;                         // ADC BIAS 设置

    cfg_uint32  DAC_Bias_Setting;                         // DAC BIAS 设置

    cfg_uint8  Keep_DA_Enabled_When_Play_Pause;           // 保持DAC打开状态, CFG_TYPE_BOOL
    cfg_uint8  Disable_PA_When_Reconnect;                 // 回连或组对时关闭 PA, CFG_TYPE_BOOL

    CFG_Type_Extern_PA_Control  Extern_PA_Control[2];     // 外部 PA 控制

    cfg_uint8  AntiPOP_Process_Disable;                   // 禁止ANTIPOP处理, CFG_TYPE_BOOL

    cfg_uint8  Pa_Gain;                                   // PA增益选择, 选择范围[0, 4], 不同模式下增益不同

    cfg_uint8  DMIC01_Channel_Aligning;                   // DMIC01 采样沿选择, 值为 0 表示 channel_1 是上升沿, channel_2 是下降沿, 值为 1 则相反

    CFG_Type_DMIC_Select_GPIO  DMIC_Select_GPIO;          // DMIC 管脚配置

    cfg_uint8  main_mic;                                  // 主mic录音通路选择,            CFG_TYPE_ADC
    cfg_uint8  sub_mic;                                   // 副mic录音通路选择,            CFG_TYPE_ADC

    cfg_uint8  adc0_input;                                // ADC0管脚配置,  CFG_TYPE_ADC0_INPUT
    cfg_uint8  adc1_input;                                // ADC1管脚配置,  CFG_TYPE_ADC1_INPUT

    cfg_uint8  ADC0_SEL_VMIC;                             // ADC0供电选择, CFG_TYPE_VMIC
    cfg_uint8  ADC1_SEL_VMIC;                             // ADC1供电选择, CFG_TYPE_VMIC

    cfg_uint8  Hw_Aec_Select;                             // 硬件aec，选择'无'则使用软件aec,            CFG_TYPE_ADC

} CFG_Struct_Audio_Settings;


typedef struct  // 提示音列表
{
    CFG_Type_Tone_ID_Name  Tone[CFG_MAX_TONES];         // 提示音

    cfg_uint8  Tone_Format_Name[CFG_MAX_TONE_FMT_LEN];  // 文件格式

} CFG_Struct_Tone_List;


typedef struct  // 按键音
{
    cfg_uint8  Key_Tone_Select;            // 选择按键音,   CFG_TYPE_TONE_ID
    cfg_uint8  Long_Key_Tone_Select;       // 长按提示音,   CFG_TYPE_TONE_ID
    cfg_uint8  Long_Long_Key_Tone_Select;  // 超长按提示音, CFG_TYPE_TONE_ID
    cfg_uint8  Very_Long_Key_Tone_Select;  // 极长按提示音, CFG_TYPE_TONE_ID

} CFG_Struct_Key_Tone;


typedef struct  // 语音列表
{
    CFG_Type_Voice_ID_Name  Voice[CFG_MAX_VOICES];        // 语音

    cfg_uint8  Voice_Format_Name[CFG_MAX_VOICE_FMT_LEN];  // 文件格式

} CFG_Struct_Voice_List;


typedef struct  // 数字语音列表
{
    CFG_Type_Numeric_Voice_ID_Name  Voice[CFG_MAX_NUMERIC_VOICES];  // 语音

} CFG_Struct_Numeric_Voice_List;


typedef struct  // 事件通知
{
    CFG_Type_Event_Notify  Notify[CFG_MAX_EVENT_NOTIFY];  // 通知

} CFG_Struct_Event_Notify;


typedef struct  // 电池充电
{
    cfg_uint8   Select_Charge_Mode;                     // 选择充电模式, CFG_TYPE_BAT_CHARGE_MODE, 后台充电模式将保持蓝牙正常工作状态

    cfg_uint8   Charge_Current;                         // 正常充电电流, CFG_TYPE_CHARGE_CURRENT

    cfg_uint8   Charge_Voltage;                         // 正常充电电压,     CFG_TYPE_CHARGE_VOLTAGE
    cfg_uint8   Charge_Stop_Mode;                       // 电池充满阈值选择, CFG_TYPE_CHARGE_STOP_MODE
    cfg_uint16  Charge_Stop_Voltage;                    // 电池充满阈值电压, 4.05 ~ 4.38, (小于等于充电电压 - 0.02V)
    cfg_uint8   Charge_Stop_Current;                    // 电池充满阈值电流, CFG_TYPE_CHARGE_STOP_CURRENT
    cfg_uint8  Init_Charge_Enable;                      //  开机预充使能, CFG_TYPE_BOOL
    cfg_uint8  Init_Charge_Current_1;                   // 开机预充一阶段电流, CFG_TYPE_CHARGE_CURRENT
    cfg_uint16  Init_Charge_Vol_1;                      // 开机预充一阶段阈值电压
    cfg_uint8  Init_Charge_Current_2;                   // 开机预充二阶段电流, CFG_TYPE_CHARGE_CURRENT
    cfg_uint16  Init_Charge_Vol_2;                      // 开机预充二阶段阈值电压
    cfg_uint8  Init_Charge_Current_3;                   // 开机预充三阶段电流, CFG_TYPE_CHARGE_CURRENT
    cfg_uint16  Init_Charge_Vol_3;                      // 开机预充三阶段阈值电压

    cfg_uint8  Precharge_Enable;                        //  预充使能, CFG_TYPE_BOOL	
	cfg_uint16  Precharge_Stop_Voltage;                 // 低电预充阈值电压, CFG_TYPE_PRECHARGE_STOP_VOLTAGE
    cfg_uint8   Precharge_Current;                      // 低电预充电流, CFG_TYPE_PRECHARGE_CURRENT
    cfg_uint8   Precharge_Current_Min_Limit;            // 低电预充电流最小限制, CFG_TYPE_PRECHARGE_CURRENT_MIN_LIMIT
    cfg_uint8   Fast_Charge_Enable;                     // 快速充电使能, CFG_TYPE_BOOL
    cfg_uint8   Fast_Charge_Current;                    // 快速充电电流, CFG_TYPE_CHARGE_CURRENT
    cfg_uint16  Fast_Charge_Voltage_Threshold;          // 快速充电截止电压, CFG_TYPE_FASTCHARGE_VOLTAGE_THRESHOLD
    cfg_uint8   Enable_Battery_Recharge;                // 启用电池复充功能,         CFG_TYPE_BOOL
    cfg_uint16  Recharge_Threshold_Low_Temperature;     // 低温时复充阈值电压
    cfg_uint16  Recharge_Threshold_Normal_Temperature;  // 常温时复充阈值电压
    cfg_uint16  Recharge_Threshold_High_Temperature;    // 高温时复充阈值电压  

    cfg_uint8   Bat_OVP_Enable;                         // 启用充电过压保护功能,         CFG_TYPE_BOOL
    cfg_uint16  OVP_Voltage_Low_Temperature;            // 低温时过压保护阈值电压
    cfg_uint16  OVP_Voltage_Normal_Temperature;         // 常温时过压保护阈值电压
    cfg_uint16  OVP_Voltage_High_Temperature;           // 高温时过压保护阈值电压  	
    
	cfg_uint16	Charge_Total_Time_Limit;                 // 充电总时间限制(分钟)	  

    cfg_uint16  Battery_Check_Period_Sec;               // 电量检测周期 (秒),     10 ~ 300
    cfg_uint16  Charge_Check_Period_Sec;                // 正常充电检测周期 (秒), 60 ~ 600
    cfg_uint16  Charge_Full_Continue_Sec;               // 充满延续时间 (秒),     10 ~ 1800, 充电至阈值后继续充电该时间以完全充满

    cfg_uint16  Front_Charge_Full_Power_Off_Wait_Sec;   // 前台模式充电满后关机等待时间 (秒), 5 ~ 300

    cfg_uint16  DC5V_Detect_Debounce_Time_Ms;           // DC5V 检测去抖时间 (毫秒), 0 ~ 1000

    cfg_uint16  Battery_Default_RDrop;                  // 	电池标称内阻 (毫欧)

} CFG_Struct_Battery_Charge;


typedef struct  // 充电盒设置
{
    cfg_uint8   Enable_Charger_Box;                             // 启用充电盒充电模式,       CFG_TYPE_BOOL
    cfg_uint8   DC5V_Pull_Down_Current;                         // DC5V 下拉唤醒充电盒,      CFG_TYPE_DC5VPD_CURRENT
    cfg_uint16  DC5V_Pull_Down_Hold_Ms;                         // DC5V 下拉保持时间 (毫秒), 0 ~ 2000
    cfg_uint16  Charger_Standby_Delay_Ms;                       // 充电盒待机延迟 (毫秒),    0 ~ 2000
    cfg_uint16  Charger_Standby_Voltage;                        // 充电盒待机电压,           0.5 ~ 4.0, 充电盒待机电压为自身电池电压时可配置为 3.8V
    cfg_uint16  Charger_Wake_Delay_Ms;                          // 充电盒唤醒延迟 (毫秒),    0 ~ 2000
    cfg_uint8   Charger_Box_Standby_Current;                    // 充电盒待机电流(mA), 0 ~ 10

    CFG_Type_DC5V_UART_Comm_Settings  DC5V_UART_Comm_Settings;  // DC5V_COM 通讯设置

    CFG_Type_DC5V_IO_Comm_Settings  DC5V_IO_Comm_Settings;      // DC5V_IO 通讯设置

} CFG_Struct_Charger_Box;


typedef struct  // 电量分级
{
    cfg_uint16  Level[CFG_MAX_BATTERY_LEVEL];  // 分级, 2.80 ~ 4.35

} CFG_Struct_Battery_Level;


typedef struct  // 低电电量
{
    cfg_uint16  Battery_Too_Low_Voltage;          // 电量不足, 3.00 ~ 3.80, 电量不足时会自动关机
    cfg_uint16  Battery_Low_Voltage;              // 电量低,   3.00 ~ 3.80
    cfg_uint16  Battery_Low_Voltage_Ex;           // 更低电量, 0.00 ~ 3.80

    cfg_uint16  Battery_Low_Prompt_Interval_Sec;  // 电量低提示间隔时间 (秒), 0 ~ 600, 设置为 0 时只提示一次

} CFG_Struct_Battery_Low;

typedef struct  // 温度调节充电电流
{
    CFG_Type_NTC_Settings  NTC_Settings;  // NTC 温度调节充电电流

    CFG_Type_NTC_Range  NTC_Ranges[5];    // NTC 温度范围

} CFG_Struct_NTC_Settings;


typedef struct  // 蓝牙设备
{
    cfg_uint8   BT_Device_Name[CFG_MAX_BT_DEV_NAME_LEN];     // 蓝牙设备名称

    cfg_uint8   Left_Device_Suffix [CFG_MAX_BT_SUFFIX_LEN];  // L 左设备名称后缀
    cfg_uint8   Right_Device_Suffix[CFG_MAX_BT_SUFFIX_LEN];  // R 右设备名称后缀

    cfg_uint8   BT_Address[6];                               // 蓝牙地址

    cfg_uint8   Use_Random_BT_Address;                       // 使用随机蓝牙地址, CFG_TYPE_BOOL, 随机生成低 3 字节蓝牙地址
    cfg_uint32  BT_Device_Class;                             // 蓝牙设备类型

    cfg_uint8   PIN_Code[CFG_MAX_BT_PIN_CODE_LEN];           // PIN Code, 在禁止 SSP 功能时使用

    cfg_uint8   Default_HOSC_Capacity;                       // 缺省频偏电容值 (pF), 0.0 ~ 24.0

    cfg_uint8   Force_Default_HOSC_Capacity;                 // 总是使用配置的频偏电容值, CFG_TYPE_BOOL

    cfg_uint8   BT_Max_RF_TX_Power;                          // 蓝牙最大发射功率, 0 ~ 43

    cfg_uint8   BLE_RF_TX_Power;                             // BLE 发射功率, 0 ~ 43

    cfg_uint8   A2DP_Bitpool;                                // A2DP Bitpool, 2 ~ 53

    cfg_uint16  Vendor_ID;                                   // 厂商 ID
    cfg_uint16  Product_ID;                                  // 产品 ID
    cfg_uint16  Version_ID;                                  // 版本 ID   

} CFG_Struct_BT_Device;


typedef struct    // RF Param Table
{
    CFG_Type_RF_Param  Param_Table[CFG_MAX_RF_TABLE_LEN];  // RF Param

} CFG_Struct_BT_RF_Param_Table;

typedef struct  // 蓝牙管理
{
    cfg_uint32  Support_Features;                             // 蓝牙支持特性, CFG_TYPE_BT_SUPPORT_FEATURES

    cfg_uint8   Support_Device_Number;                        // 可同时连接设备个数, 1 ~ 3
    cfg_uint8   Paired_Device_Save_Number;                    // 已配对设备保存个数, 2 ~ 8

    cfg_uint8   Controller_Test_Mode;                         // 控制器测试模式, CFG_TYPE_BT_CTRL_TEST_MODE
    cfg_uint8   Enter_BQB_Test_Mode_By_Key;                   // 通过按键进入 BQB 测试模式, CFG_TYPE_BOOL

    CFG_Type_Auto_Quit_BT_Ctrl_Test  Auto_Quit_BT_Ctrl_Test;  // 自动退出控制器测试模式


    cfg_uint16  Idle_Enter_Sniff_Time_Ms;                     // 空闲进入 Sniff 模式时间 (毫秒), 2000 ~ 20000
    cfg_uint16  Sniff_Interval_Ms;                            // Sniff 周期 (毫秒), 100 ~ 500

} CFG_Struct_BT_Manager;


typedef struct  // 蓝牙配对连接
{
    cfg_uint8   Default_State_Discoverable;                  // 默认状态可被搜索发现,      CFG_TYPE_BOOL
    cfg_uint16  Default_State_Wait_Connect_Sec;              // 默认状态等待配对连接 (秒), 0 ~ 600, 设置为 0 时不限时间
    cfg_uint16  Pair_Mode_Duration_Sec;                      // 配对模式持续时间 (秒),     0 ~ 600, 设置为 0 时不限时间

    cfg_uint8   Disconnect_All_Phones_When_Enter_Pair_Mode;  // 进入配对模式时断开所有已连接手机设备, CFG_TYPE_BOOL

    cfg_uint8   Clear_Paired_List_When_Enter_Pair_Mode;      // 进入配对模式时清除配对列表, CFG_TYPE_BOOL
    cfg_uint8   Clear_TWS_When_Key_Clear_Paired_List;        // 按键清除配对列表同时清除 TWS 组对设备信息, CFG_TYPE_BOOL

    cfg_uint8   Enter_Pair_Mode_When_Key_Clear_Paired_List;  // 按键清除配对列表同时进入配对模式, CFG_TYPE_BOOL
    cfg_uint8   Enter_Pair_Mode_When_Paired_List_Empty;      // 配对列表为空时开机进入配对模式,   CFG_TYPE_BOOL

    cfg_uint8   BT_Not_Discoverable_When_Connected;          // 蓝牙已连接后关闭可见性, CFG_TYPE_BOOL

} CFG_Struct_BT_Pair;


typedef struct  // TWS 组对连接
{
    cfg_uint8   TWS_Pair_Key_Mode;              // 按键组对模式, CFG_TYPE_TWS_PAIR_KEY_MODE
    cfg_uint8   Match_Mode;                     // 匹配模式,     CFG_TYPE_TWS_MATCH_MODE
    cfg_uint8   Match_Name_Length;              // 名称匹配长度, 1 ~ 30
    cfg_uint16  TWS_Wait_Pair_Search_Time_Sec;  // 等待组对搜索时间 (秒), 5 ~ 600
    cfg_uint8   TWS_Power_On_Auto_Pair_Search;  // 未组对过时开机自动进行组对搜索, CFG_TYPE_BOOL

} CFG_Struct_TWS_Pair;




typedef struct  // TWS 高级组对设置
{
    cfg_uint8  Enable_TWS_Advanced_Pair_Mode;       // 启用 TWS 高级组对模式,  CFG_TYPE_BOOL
    cfg_uint8  Check_RSSI_When_TWS_Pair_Search;     // 组对搜索时判断信号强度, CFG_TYPE_BOOL
    cfg_int8   RSSI_Threshold;                      // 信号强度阈值,           -120 ~ 0
    cfg_uint8  Use_Search_Mode_When_TWS_Reconnect;  // TWS 回连时使用搜索模式, CFG_TYPE_BOOL

} CFG_Struct_TWS_Advanced_Pair;




typedef struct  // TWS 同步设置
{
    cfg_uint8  Sync_Mode;  // 同步模式, CFG_TYPE_TWS_SYNC_MODE

} CFG_Struct_TWS_Sync;


typedef struct  // 蓝牙自动回连
{
    cfg_uint8   Enable_Auto_Reconnect;                        // 启用自动回连, CFG_TYPE_BT_AUTO_RECONNECT

    cfg_uint16  Reconnect_Phone_Timeout;                      // 单次连接手机设备超时 (秒),      0.5 ~ 10.0
    cfg_uint16  Reconnect_Phone_Interval;                     // 重试回连手机设备间隔时间 (秒),  0.5 ~ 25.0, 包含连接超时
    cfg_uint8   Reconnect_Phone_Times_By_Startup;             // 开机回连手机设备尝试次数,       0 ~ 100, 设置为 0 时不限次数
    cfg_uint16  Reconnect_TWS_Timeout;                        // 单次连接 TWS 设备超时 (秒),     0.5 ~ 10.0
    cfg_uint16  Reconnect_TWS_Interval;                       // 重试回连 TWS 设备间隔时间 (秒), 0.5 ~ 25.0, 包含连接超时
    cfg_uint8   Reconnect_TWS_Times_By_Startup;               // 开机回连 TWS 设备尝试次数,      0 ~ 100, 设置为 0 时不限次数
    cfg_uint8   Reconnect_Times_By_Timeout;                   // 超时断开回连尝试次数,           0 ~ 100, 设置为 0 时不限次数

    cfg_uint8   Enter_Pair_Mode_When_Startup_Reconnect_Fail;  // 开机回连失败时进入配对模式, CFG_TYPE_BOOL

} CFG_Struct_BT_Auto_Reconnect;


typedef struct  // HID 设置
{
    cfg_uint16  HID_Auto_Disconnect_Delay_Sec;   // HID 操作后自动断开延迟时间 (秒), 0 ~ 600, 设置为 0 时不自动断开
    cfg_uint16  HID_Connect_Operation_Delay_Ms;  // HID 连接时操作延迟时间 (毫秒),   100 ~ 2000

    cfg_uint8   HID_Custom_Key_Type;             // HID 自定义按键类型, CFG_TYPE_BT_HID_KEY_TYPE
    cfg_uint8   HID_Custom_Key_Value;            // HID 自定义按键值

} CFG_Struct_BT_HID_Settings;



typedef struct  // 低延迟模式设置
{
    CFG_Type_LLM_Basic  LLM_Basic;            // 低延时模式基础设置

    cfg_uint8   BTMusic_Use_ALLM;             // [音乐] 使用自适应低延时模式, CFG_TYPE_BOOL, 使能后不同时期测量的延时可能会有差异
    cfg_uint8   ALLM_Factor;                  // [音乐] 自适应灵敏度, 0 ~ 5, 值越大自适应低延时越优先保证播放的稳定性
    cfg_uint8   ALLM_Upper;                   // [音乐] 自适应最高延时 ms, 110 ~ 180, 允许自适应低延时上调到的最高延时值

    cfg_uint8   BTMusic_PLC_Mode;             // [音乐] 使用补帧技术, CFG_TYPE_BTMUSIC_PLC_MODE
    cfg_uint8   AAC_Decode_Delay_Optimize;    // [音乐] 使用AAC解码延时优化, CFG_TYPE_BOOL, 使能后AAC解码延时会减少约11ms

    cfg_uint8   BTCall_Latency_Optimization;  // [通话] 使用优化选项, CFG_TYPE_BTCALL_LCY_OPTLEVEL
    cfg_uint8   BTCall_Use_Limiter_Not_AGC;   // [通话] 使用Limiter代替AGC, CFG_TYPE_BOOL
    cfg_uint8   BTCall_PLC_NoDelay;           // [通话] 使用无延迟丢包补偿算法, CFG_TYPE_BOOL

    cfg_uint8   Reserved[4];                  // 保留 

} CFG_Struct_Low_Latency_Settings;

typedef struct  // 自定义音效使能
{
    cfg_uint8  Enable;       // 是否打开自定义音效, CFG_TYPE_BOOL, 打开后可以在音效调节中自定义蓝牙音乐音效
    cfg_uint8  Cur_Dae_Num;  // 自定义音效数量
    cfg_uint8  Dae_Index;    // 当前音效序号

} CFG_Struct_BTMusic_Multi_Dae_Settings;


typedef struct  // 音量同步
{
    cfg_uint8   Volume_Sync_Only_When_Playing;  // 只在播放状态下同步音量, CFG_TYPE_BOOL
    cfg_uint8   Origin_Volume_Sync_To_Remote;   // 初始音量同步至远端设备, CFG_TYPE_BOOL, (连接时同步)
    cfg_uint16  Origin_Volume_Sync_Delay_Ms;    // 初始音量同步延迟时间 (毫秒), 2000 ~ 5000
    cfg_uint16  Playing_Volume_Sync_Delay_Ms;   // 播放音量同步延迟时间 (毫秒), 1000 ~ 3000

} CFG_Struct_BT_Music_Volume_Sync;


typedef struct  // 按键停顿时间
{
    cfg_uint16  Key_Pause_Stop_Hold_Ms;  // 按键暂停时停顿时间 (毫秒), 0 ~ 5000, 暂停后维持停止状态一段时间 (可过滤音乐淡出数据)
    cfg_uint16  Key_Prev_Next_Hold_Ms;   // 按键上下曲停顿时间 (毫秒), 0 ~ 5000

} CFG_Struct_BT_Music_Stop_Hold;


typedef struct  // 双手机播放控制
{
    cfg_uint8   Stop_Another_When_One_Playing;    // 开始播放时停止另一手机, CFG_TYPE_BOOL
    cfg_uint8   Resume_Another_When_One_Stopped;  // 停止播放时恢复另一手机, CFG_TYPE_BOOL
    cfg_uint16  A2DP_Status_Stopped_Delay_Ms;     // 停止播放状态延迟时间 (毫秒), 500 ~ 5000

} CFG_Struct_BT_Two_Device_Play;


typedef struct  // 音量同步
{
    cfg_uint8   Origin_Volume_Sync_To_Remote;  // 初始音量同步至远端设备, CFG_TYPE_BOOL, (开始通话时同步)
    cfg_uint16  Origin_Volume_Sync_Delay_Ms;   // 初始音量同步延迟时间 (毫秒), 1000 ~ 3000

} CFG_Struct_BT_Call_Volume_Sync;


typedef struct  // 来电提示
{
    cfg_uint16  Prompt_Interval_Ms;  // 来电提示间隔时间 (毫秒), 200 ~ 5000
    cfg_uint8   Play_Phone_Number;   // 播报来电号码, CFG_TYPE_BOOL

    cfg_uint8   BT_Call_Ring_Mode;   // 来电铃声模式, CFG_TYPE_BT_CALL_RING_MODE

} CFG_Struct_Incoming_Call_Prompt;


typedef struct  // 温度补偿
{
    cfg_uint8  Enable_Cap_Temp_Comp;                       // 启用温度补偿, CFG_TYPE_BOOL

    CFG_Type_Cap_Temp_Comp  Table[CFG_MAX_CAP_TEMP_COMP];  // 温度补偿

} CFG_Struct_Cap_Temp_Comp;


typedef struct  // Linein 检测
{
    cfg_uint8  Detect_Mode;                      // 检测方式, CFG_TYPE_LINEIN_DETECT_MODE

    CFG_Type_Linein_Detect_GPIO  Detect_GPIO;    // GPIO 检测 Linein

    CFG_Type_Linein_Detect_LRADC  Detect_LRADC;  // LRADC 检测 Linein

    cfg_uint16  Debounce_Time_Ms;                // 去抖时间 (毫秒),  0 ~ 1000

} CFG_Struct_Linein_Detect;


typedef struct  // 蓝牙音乐音效
{
    cfg_uint8  Enable_DAE;   // 音效使能, CFG_TYPE_BOOL
    cfg_uint8  Test_Volume;  // 测试音量, 0 ~ 16

} CFG_Struct_BT_Music_DAE;


typedef struct  // 蓝牙通话输出音效
{
    cfg_uint8  Enable_DAE;   // 音效使能, CFG_TYPE_BOOL
    cfg_uint8  Test_Volume;  // 测试音量, 0 ~ 16

} CFG_Struct_BT_Call_Out_DAE;


typedef struct  // 蓝牙通话 MIC 音效
{
    cfg_uint8  Enable_DAE;   // 音效使能, CFG_TYPE_BOOL
    cfg_uint8  Test_Volume;  // 测试音量, 0 ~ 16

} CFG_Struct_BT_Call_MIC_DAE;


typedef struct  // Linein 输出音效
{
    cfg_uint8  Enable_DAE;   // 音效使能, CFG_TYPE_BOOL
    cfg_uint8  Test_Volume;  // 测试音量, 0 ~ 16

} CFG_Struct_Linein_Out_DAE;


typedef struct  // 通话效果
{
    CFG_Type_MIC_Gain  MIC_Gain;  // MIC 增益

    cfg_uint8  Test_Volume;       // 测试音量, 0 ~ 15

} CFG_Struct_BT_Call_Quality;


typedef struct  // 播放器参数
{
    cfg_uint32  VP_Develop_Value1;       // 开发者模式参数 1
    cfg_uint8   VP_WaitData_Time;        // 数据不增加时持续等待时间
    cfg_uint8   VP_WaitData_Empty_Time;  // 数据为空时持续等待时间 (0 表示一直等待)
    cfg_uint8   VP_Max_Decode_Count;     // 最大解码次数
    cfg_uint16  VP_Max_PCMBUF_Sampels;   // PCMBUF 持续解码最大门限值
    cfg_uint16  VP_Het_PCMBUF_Sampels;   // PCMBUF 半空中断门限值
    cfg_uint16  VP_Hft_PCMBUF_Sampels;   // PCMBUF 半满中断门限值
    cfg_uint8   VP_Work_Frequency;       // 工作频率(包括系统正常工作的频率)
    cfg_uint8   VP_Module_Frequency;     // 模块频率(此模块所需频率增量)


} CFG_Struct_Voice_Player_Param;


typedef struct  // 播放器设置
{
    cfg_uint16  VP_StartPlay_Threshold;  // 普通模式开始播放门限值 Bytes

} CFG_Struct_Voice_User_Settings;


typedef struct  // 播放器参数
{
    cfg_uint32  WT_Develop_Value1;       // 开发者模式参数 1
    cfg_uint8   WT_WaitData_Time;        // 数据不增加时持续等待时间
    cfg_uint8   WT_WaitData_Empty_Time;  // 数据为空时持续等待时间 (0 表示一直等待)
    cfg_uint8   WT_Max_Decode_Count;     // 最大解码次数
    cfg_uint16  WT_Max_PCMBUF_Sampels;   // PCMBUF 持续解码最大门限值
    cfg_uint16  WT_Het_PCMBUF_Sampels;   // PCMBUF 半空中断门限值
    cfg_uint16  WT_Hft_PCMBUF_Sampels;   // PCMBUF 半满中断门限值
    cfg_uint8   WT_Work_Frequency;       // 工作频率(包括系统正常工作的频率)
    cfg_uint8   WT_Module_Frequency;     // 模块频率(此模块所需频率增量)

} CFG_Struct_Tone_Player_Param;


typedef struct  // 播放器设置
{
    cfg_uint16  WT_StartPlay_Threshold;  // 普通模式开始播放门限值 Bytes

} CFG_Struct_Tone_User_Settings;


typedef struct  // 播放器参数
{
    cfg_uint32  LI_Develop_Value1;       // 开发者模式参数 1
    cfg_uint8   LI_WaitData_Time;        // 数据不增加时持续等待时间
    cfg_uint8   LI_WaitData_Empty_Time;  // 数据为空时持续等待时间 (0 表示一直等待)
    cfg_uint8   LI_Max_Decode_Count;     // 最大解码次数
    cfg_uint16  LI_Max_PCMBUF_Sampels;   // PCMBUF 持续解码最大门限值
    cfg_uint16  LI_Het_PCMBUF_Sampels;   // PCMBUF 半空中断门限值
    cfg_uint16  LI_Hft_PCMBUF_Sampels;   // PCMBUF 半满中断门限值
    cfg_uint16  LI_StartPlay_Normal;     // 普通模式开始播放门限值 Bytes
    cfg_uint8   LI_Work_Frequency;       // 工作频率(包括系统正常工作的频率)
    cfg_uint8   LI_Module_Frequency;     // 模块频率(此模块所需频率增量)

} CFG_Struct_Linein_Player_Param;


typedef struct  // 播放器设置
{
    cfg_uint8   LI_INOUT_Mode;             // 模式选择, CFG_TYPE_AUXIN_OUT_MODE, AA 通路无法调节音量
    cfg_uint16  LI_Fadein_Continue_Time;   // 淡入持续时间 ms
    cfg_uint16  LI_Fadeout_Continue_Time;  // 淡出持续时间 ms
    cfg_uint16  LI_Playing_CacheData;      // 普通模式缓冲区播放数据量 Bytes

} CFG_Struct_Linein_User_Settings;


typedef struct  // 播放器参数
{
    cfg_uint32  BM_Develop_Value1;        // 开发者模式参数 1

    cfg_uint8   BM_WaitData_Time;         // 数据不增加时持续等待时间
    cfg_uint8   BM_WaitData_Empty_Time;   // 数据为空时持续等待时间 (0 表示一直等待)
    cfg_uint8   BM_Freq_TWS_Decrement;    // TWS 场景频率减量 Mhz
    cfg_uint8   BM_SBC_Max_Decode_Count;  // SBC 最大解码次数
    cfg_uint8   BM_AAC_Max_Decode_Count;  // AAC 最大解码次数
    cfg_uint8   BM_Max_PCMBUF_Time;       // PCMBUF 最大缓存时间
    cfg_uint16  BM_SBC_Max_Sleep_Time;    // SBC 最大睡眠时间 us
    cfg_uint16  BM_AAC_Max_Sleep_Time;    // AAC 最大睡眠时间 us
    cfg_uint16  BM_TWS_WPlay_Mintime;     // TWS 同时播放等待最小时间间隔 ms
    cfg_uint16  BM_TWS_WPlay_Maxtime;     // TWS 同时播放等待最大时间间隔 ms
    cfg_uint16  BM_TWS_WStop_Mintime;     // TWS 同时停止等待最小时间间隔 (中断时间) ms
    cfg_uint16  BM_TWS_WStop_Maxtime;     // TWS 同时停止等待最大时间间隔 (超时时间) ms
    cfg_uint16  BM_TWS_Sync_interval;     // TWS 播放过程中同步时间间隔 pkt
    cfg_uint16  BM_Het_PCMBUF_Sampels;    // PCMBUF 半空中断门限值
    cfg_uint16  BM_Hft_PCMBUF_Sampels;    // PCMBUF 半满中断门限值
    cfg_uint16  BM_StartPlay_Normal;      // 普通模式开始播放延迟时间 ms
    cfg_uint16  BM_StartPlay_TWS;         // TWS 模式开始播放延迟时间 ms
    cfg_uint8   BM_Work_Frequency_AAC;    // AAC工作频率(包括系统正常工作的频率)
    cfg_uint8   BM_Module_Frequency_AAC;  // AAC模块频率(此模块所需频率增量), 暂时不用
    cfg_uint8   BM_Work_Frequency_SBC;    // SBC工作频率(包括系统正常工作的频率)
    cfg_uint8   BM_Module_Frequency_SBC;  // SBC模块频率(此模块所需频率增量), 暂时不用

} CFG_Struct_BTMusic_Player_Param;


typedef struct  // 播放器设置
{
    cfg_uint8   BM_DataWidth;              // 音频输出位宽选择, 值为 2 表示 16bit 输出, 4 表示24bit 输出
    cfg_uint8   BM_ISpeech_PEQ_Enable;     // 语音识别场景下播歌 PEQ (CPU 不足时不能开启)
    cfg_uint16  BM_Fadein_Continue_Time;   // 淡入持续时间 ms
    cfg_uint16  BM_Fadeout_Continue_Time;  // 淡出持续时间 ms
    cfg_uint16  BM_SBC_Playing_CacheData;  // SBC 格式播放延迟 ms,  40 ~ 300
    cfg_uint16  BM_AAC_Playing_CacheData;  // AAC 格式播放延迟 ms,  50 ~ 300

} CFG_Struct_BTMusic_User_Settings;


typedef struct  // 播放器参数
{
    cfg_uint32  BS_Develop_Value1;         // 是否启用各算法模块, CFG_TYPE_BTCALL_ALSP

    cfg_uint8   BS_WaitData_Time;          // 数据不增加时持续等待时间
    cfg_uint8   BS_WaitData_Empty_Time;    // 数据为空时持续等待时间 (0 表示一直等待)
    cfg_uint8   BS_Max_Decode_Count;       // 最大解码次数
    cfg_uint8   BS_Max_PCMBUF_Time;        // PCMBUF 最大缓存时间
    cfg_uint16  BS_CVSD_Max_Sleep_Time;    // CVSD 最大睡眠时间 us
    cfg_uint16  BS_MSBC_Max_Sleep_Time;    // MSBC 最大睡眠时间 us
    cfg_uint16  BS_TWS_WPlay_Mintime;      // TWS 同时播放等待最小时间间隔 (中断时间) ms
    cfg_uint16  BS_TWS_WPlay_Maxtime;      // TWS 同时播放等待最大时间间隔 (超时时间) ms
    cfg_uint16  BS_TWS_WStop_Mintime;      // TWS 同时停止等待最小时间间隔 (中断时间) ms
    cfg_uint16  BS_TWS_WStop_Maxtime;      // TWS 同时停止等待最大时间间隔 (超时时间) ms
    cfg_uint16  BS_TWS_Sync_interval;      // TWS 播放过程中同步时间间隔 pkt
    cfg_uint16  BS_Het_PCMBUF_Sampels;     // PCMBUF 半空中断门限值
    cfg_uint16  BS_Hft_PCMBUF_Sampels;     // PCMBUF 半满中断门限值
    cfg_uint16  BS_StartPlay_Normal;       // 普通模式开始播放延迟 ms
    cfg_uint16  BS_StartPlay_TWS;          // TWS 模式开始播放延迟 ms
    cfg_uint8   BS_Work_Frequency_MSBC;    // MSBC工作频率(包括系统正常工作的频率)
    cfg_uint8   BS_Module_Frequency_MSBC;  // MSBC模块频率(此模块所需频率增量), 暂时不用
    cfg_uint8   BS_Work_Frequency_CVSD;    // CVSD工作频率(包括系统正常工作的频率)
    cfg_uint8   BS_Module_Frequency_CVSD;  // CVSD模块频率(此模块所需频率增量), 暂时不用
    cfg_uint8   BS_Module_Frequency_TMIC;  // 双MIC模块频率(此模块所需频率增量), 暂时不用
    cfg_uint8   BS_Module_Frequency_PLC;   // PLC模块频率(此模块所需频率增量), 暂时不用
    cfg_uint8   BS_MIC_Playing_PKTCNT;     // 播放过程中控制器队列缓存MIC包数 

} CFG_Struct_BTSpeech_Player_Param;


typedef struct  // 播放器设置
{
    cfg_uint8   BS_DataWidth;               // 音频输出位宽选择, 值为 2 表示 16bit 输出, 4 表示24bit 输出
    cfg_uint16  BS_Max_Out_Gain;            // 最大输出幅度, CFG_TYPE_VOLUME_GAIN
    cfg_uint16  BS_Fadein_Continue_Time;    // 淡入持续时间 ms
    cfg_uint16  BS_Fadeout_Continue_Time;   // 淡出持续时间 ms
    cfg_uint16  BS_CVSD_Playing_CacheData;  // CVSD 格式播放延迟 ms,  30 ~ 150
    cfg_uint16  BS_MSBC_Playing_CacheData;  // MSBC 格式播放延迟 ms,  30 ~ 150

} CFG_Struct_BTSpeech_User_Settings;


typedef struct  // 播放器参数
{
    cfg_uint32  IS_Develop_Value1;        // 开发者模式参数 1
    cfg_uint8   IS_AEC_Asr_NrLevel;       // 打断唤醒时的 level, 默认为 0 (aec mode 为 1 时才有效)
    cfg_uint8   IS_AEC_LowSkip_Enable;    // 打断唤醒和正常通话都有效, 默认为 1
    cfg_uint8   IS_AEC_ShiftProcess;      // AEC 处理后的数据移位操作
    cfg_uint8   IS_Work_Frequency;        // 普通场景的工作频率增量, 理论上只有 VAD
    cfg_uint8   IS_Module_Frequency_VAD;  // AEC模块频率(此模块所需频率增量)
    cfg_uint8   IS_Module_Frequency_AEC;  // AEC模块频率(此模块所需频率增量)
    cfg_uint8   IS_Module_Frequency_ASR;  // ASR模块频率(此模块所需频率增量)

} CFG_Struct_IGSpeech_Player_Param;


typedef struct  // 播放器设置
{
    CFG_Type_MIC_Gain  MIC_Gain;  // MIC 增益

} CFG_Struct_IGSpeech_User_Settings;


typedef struct  // BLE 管理
{
    cfg_uint8  BLE_Enable;                           // 启用 BLE 功能, CFG_TYPE_BOOL

    cfg_uint8  Use_Advertising_Mode_2_After_Paired;  // 配对连接过后使用 BLE 广播模式 2, CFG_TYPE_BOOL, 配对列表非空且不在配对模式

    cfg_uint8  BLE_Address_Type;                     // BLE 地址类型, CFG_TYPE_BLE_ADDR_TYPE

    cfg_uint8  Advertising_After_Connected;          // 经典蓝牙连接后才进行BLE广播, CFG_TYPE_BOOL
    
    cfg_uint8  Reserved[8];                          // 数据

} CFG_Struct_BLE_Manager;


typedef struct  // BLE 广播模式 1
{
    cfg_uint16  Advertising_Interval_Ms;                      // 广播间隔 (毫秒), 20 ~ 5000
    cfg_uint8   Advertising_Type;                             // 广播类型, CFG_TYPE_BLE_ADV_TYPE

    cfg_uint8   BLE_Device_Name[29];                          // BLE 设备名称

    cfg_uint8   Manufacturer_Specific_Data[59];               // 厂商自定义数据
    cfg_uint8   Service_UUIDs_16_Bit[59];                     // 服务 UUIDs (16-Bit)
    cfg_uint8   Service_UUIDs_128_Bit[CFG_MAX_UUID_STR_LEN];  // 服务 UUIDs (128-Bit)

} CFG_Struct_BLE_Advertising_Mode_1;


typedef struct  // BLE 广播模式 2
{
    cfg_uint16  Advertising_Interval_Ms;                      // 广播间隔 (毫秒), 20 ~ 5000
    cfg_uint8   Advertising_Type;                             // 广播类型, CFG_TYPE_BLE_ADV_TYPE

    cfg_uint8   BLE_Device_Name[29];                          // BLE 设备名称

    cfg_uint8   Manufacturer_Specific_Data[59];               // 厂商自定义数据
    cfg_uint8   Service_UUIDs_16_Bit[59];                     // 服务 UUIDs (16-Bit)
    cfg_uint8   Service_UUIDs_128_Bit[CFG_MAX_UUID_STR_LEN];  // 服务 UUIDs (128-Bit)

} CFG_Struct_BLE_Advertising_Mode_2;


typedef struct  // BLE 连接参数
{
    cfg_uint16  Interval_Min_Ms;  // 最小间隔 (毫秒), 8 ~ 1000
    cfg_uint16  Interval_Max_Ms;  // 最大间隔 (毫秒), 8 ~ 1000
    cfg_uint16  Latency;          // 延迟,            0 ~ 100
    cfg_uint16  Timeout_Ms;       // 超时 (毫秒),     500 ~ 10000

} CFG_Struct_BLE_Connection_Param;

typedef struct  // LE AUDIO 管理
{
    cfg_uint8  LE_AUDIO_Enable;            // 启用 LE AUDIO 功能, CFG_TYPE_BOOL
    cfg_uint8  LE_AUDIO_APP_Auto_Switch;   // 启用BR music与LEA 场景自动切换功能, CFG_TYPE_BOOL	
	
    cfg_uint8   LE_AUDIO_Device_Name[29];  // LE AUDIO 设备名称

    cfg_uint8  Reserved1;                  // 数据
    cfg_uint8  LE_AUDIO_Dir_Adv_Enable;    // 启用定向广播, CFG_TYPE_BOOL

    cfg_uint8   Reserved[32];              // 数据

} CFG_Struct_LE_AUDIO_Manager;


typedef struct  // LE AUDIO 广播模式
{
    cfg_uint8   LE_AUDIO_LC3_Interval;              // LC3周期, CFG_TYPE_LC3_INTERVAL
    cfg_uint8   LE_AUDIO_LC3_RX_Bit_Width;          // LC3解码位宽, CFG_TYPE_LC3_BIT_WIDTH
    cfg_uint8   LE_AUDIO_LC3_RX_Music_2CH_Bitrate;  // LC3立体声音乐最高码率, CFG_TYPE_LC3_MUSIC_2CH_BITRATE
    cfg_uint8   LE_AUDIO_LC3_RX_Music_1CH_Bitrate;  // LC3单声道音乐最高码率, CFG_TYPE_LC3_MUSIC_1CH_BITRATE
    cfg_uint8   LE_AUDIO_LC3_TX_Bit_Width;          // LC3编码位宽, CFG_TYPE_LC3_BIT_WIDTH
    cfg_uint8   LE_AUDIO_LC3_TX_Call_1CH_Bitrate;   // LC3通话最高码率, CFG_TYPE_LC3_CALL_1CH_BITRATE
    cfg_uint8   LE_AUDIO_Reserved2[16];             // 数据

} CFG_Struct_LE_AUDIO_Advertising_Mode;



typedef struct  // BLE 数据透传
{
    cfg_uint8   Enable_BLE_Pass_Through;             // 启用 BLE 数据透传, CFG_TYPE_BOOL

    cfg_uint8   Service_UUID[CFG_MAX_UUID_STR_LEN];  // 服务 UUID
    cfg_uint8   TX_RX_UUID[CFG_MAX_UUID_STR_LEN];    // TX/RX UUID

    cfg_uint16  RX_Buffer_Size;                      // RX 缓冲区大小, 128 ~ 4096

} CFG_Struct_BLE_Pass_Through;


typedef struct  // 链路质量监控
{
    cfg_uint8  Quality_Pre_Value;
    cfg_uint8  Quality_Diff;
    cfg_uint8  Quality_ESCO_Diff;
    cfg_uint8  Quality_Monitor;

} CFG_Struct_BT_Link_Quality;


typedef struct  // SCAN 参数设置
{
    CFG_Type_BT_Scan_Params  Params[7];  // SCAN 参数设置

} CFG_Struct_BT_Scan_Params;


typedef struct  // 本地播放设置
{
    cfg_uint8  Reserved;  // 数据

} CFG_Struct_App_Music;


typedef struct  // 存储卡设置
{
    cfg_uint8  Reserved;  // 数据

} CFG_Struct_Card_Settings;


typedef struct  // USB 设置
{
    cfg_uint8  Reserved;  // 数据

} CFG_Struct_USB_Settings;


typedef struct  // 用户保留配置
{
    cfg_uint8  String[128];               // 字符串

    cfg_uint8  Run_Console_Command[127];  // 执行控制台命令

} CFG_Struct_Usr_Reserved_Data;


typedef struct  // 系统更多配置
{
    cfg_uint8  Reserved;  // 数据

} CFG_Struct_Sys_Reserved_Data;

typedef struct  // 蓝牙调试
{
    cfg_uint8  BTC_Log;           // 蓝牙控制器LOG输出方式,  值为 0 表示关闭, 1 表示通过BTC UART 输出,2表示通过系统打印	 
    cfg_uint8  BTC_Log_TX_Pin;    // BTC UART 输出管脚
    cfg_uint8  BTC_Log_RX_Pin;    // BTC UART 输入管脚
    cfg_uint8  BTC_BQB_TX_Pin;    // BQB UART1 输出管脚
    cfg_uint8  BTC_BQB_RX_Pin;    // BQB UART1 输入管脚
    cfg_uint8  BTC_Fix_TX_Power;  // BTC 固定TX POWER, 值为0表示关闭，其它值表示固定TX power的 level值: 如配38,表示固定TX POWER为8db   
    cfg_uint8  BTC_Debug_Signal;  // BTC 调试信号, 值为0表示禁止，1表示使能
    cfg_uint8  Reserved[29];      // 数据

} CFG_Struct_BT_Debug;

typedef struct  // 系统设置更多配置
{
    cfg_uint8	Default_USB_Device_Mode;  // 默认usb设备模式, CFG_TYPE_USB_DEVICE_MODE
	
    cfg_uint8  Reserved;                // 数据

} CFG_Struct_System_More_Settings;

typedef struct  // 固件升级更多配置
{
    cfg_uint8  Reserved;  // 数据

} CFG_Struct_Upgrade_More_Settings;

typedef struct  // 显示更多配置
{
    cfg_uint8  Reserved;  // 数据

} CFG_Struct_Display_More_Settings;

typedef struct  // 按键更多配置
{
    cfg_uint8  Reserved;  // 数据

} CFG_Struct_Key_More_Settings;

typedef struct  // 语音更多配置
{
    cfg_uint8  Reserved;  // 数据

} CFG_Struct_Voice_More_Settings;

typedef struct  // 音频更多配置
{
    cfg_uint8  WirelessMic_Multi_Dae_Enable;       // 是否打开自定义无线麦音效, CFG_TYPE_BOOL, 打开后可以在音效调节中自定义无线麦音效
    cfg_uint8  WirelessMic_Multi_Dae_Cur_Dae_Num;  // 自定义无线麦音效数量
    cfg_uint8  WirelessMic_Multi_Dae_Dae_Index;    // 当前无线麦音效序号
    cfg_uint8  WirelessMic_Dae_Expand_Enable;      // 无线麦音效扩展使能, CFG_TYPE_BOOL, 打开后无线麦音效支持20个PEQ点
	cfg_uint8  Automute_Force_Enable;              // 强制使能Automute, CFG_TYPE_BOOL, 如果不强制使能，则由 CONFIG_AUDIO_DAC_0_AUTOMUTE 决定

    cfg_uint8  Reserved;                           // 数据

} CFG_Struct_Audio_More_Settings;

typedef struct  // 音量增益更多配置
{
    cfg_uint8  Reserved;  // 数据

} CFG_Struct_Volume_Gain_More_Settings;

typedef struct  // 蓝牙音乐更多配置
{
    cfg_uint8  Reserved;  // 数据

} CFG_Struct_BT_Music_More_Settings;

typedef struct  // 蓝牙通话更多配置
{
	cfg_uint8  BS_noise_reduce;  // 近端喇叭噪声控制使能, CFG_TYPE_BTCALL_NOISE_REDUCE

} CFG_Struct_BT_Call_More_Settings;

typedef struct  // 蓝牙发射管理
{
    cfg_uint8   Trans_Work_Mode;  // 蓝牙发射模式, CFG_TYPE_BOOL

} CFG_Struct_TR_BT_Device;

typedef struct  // BLE管理更多配置
{
    cfg_uint8  Reserved;  // 数据

} CFG_Struct_BLE_Manager_More_Settings;


typedef struct  // LE AUDIO通话输出音效
{
    cfg_uint8  Enable_DAE;   // 音效使能, CFG_TYPE_BOOL
    cfg_uint8  Test_Volume;  // 测试音量, 0 ~ 16

} CFG_Struct_LEAUDIO_Call_Out_DAE;

typedef struct  // LE AUDIO通话 MIC 音效
{
    cfg_uint8  Enable_DAE;   // 音效使能, CFG_TYPE_BOOL
    cfg_uint8  Test_Volume;  // 测试音量, 0 ~ 16

} CFG_Struct_LEAUDIO_Call_MIC_DAE;

#define CFG_ID_USER_VERSION             0x01
#define CFG_ID_PLATFORM_CASE            0x02
#define CFG_ID_CONSOLE_UART             0x03
#define CFG_ID_SYSTEM_SETTINGS          0x04
#define CFG_ID_OTA_SETTINGS             0x05
#define CFG_ID_FACTORY_SETTINGS         0x06
#define CFG_ID_ONOFF_KEY                0x07
#define CFG_ID_LRADC_KEYS               0x08
#define CFG_ID_GPIO_KEYS                0x09
#define CFG_ID_TAP_KEY                  0x0A
#define CFG_ID_KEY_THRESHOLD            0x0B
#define CFG_ID_KEY_FUNC_MAPS            0x0C
#define CFG_ID_COMBO_KEY_FUNC_MAPS      0x0D
#define CFG_ID_CUSTOMED_KEY_SEQUENCE    0x0E
#define CFG_ID_LED_DRIVES               0x0F
#define CFG_ID_LED_DISPLAY_MODELS       0x10
#define CFG_ID_BT_MUSIC_VOLUME_TABLE    0x11
#define CFG_ID_BT_CALL_VOLUME_TABLE     0x12
#define CFG_ID_LINEIN_VOLUME_TABLE      0x13
#define CFG_ID_VOICE_VOLUME_TABLE       0x14
#define CFG_ID_VOLUME_SETTINGS          0x15
#define CFG_ID_AUDIO_SETTINGS           0x16
#define CFG_ID_TONE_LIST                0x17
#define CFG_ID_KEY_TONE                 0x18
#define CFG_ID_VOICE_LIST               0x19
#define CFG_ID_NUMERIC_VOICE_LIST       0x1A
#define CFG_ID_EVENT_NOTIFY             0x1B
#define CFG_ID_BATTERY_CHARGE           0x1C
#define CFG_ID_CHARGER_BOX              0x1D
#define CFG_ID_BATTERY_LEVEL            0x1E
#define CFG_ID_BATTERY_LOW              0x1F
#define CFG_ID_NTC_SETTINGS             0x20
#define CFG_ID_BT_DEVICE                0x21
#define CFG_ID_BT_RF_PARAM_TABLE        0x22
#define CFG_ID_BT_MANAGER               0x23
#define CFG_ID_BT_PAIR                  0x24
#define CFG_ID_TWS_PAIR                 0x25
#define CFG_ID_TWS_ADVANCED_PAIR        0x26
#define CFG_ID_TWS_SYNC                 0x27
#define CFG_ID_BT_AUTO_RECONNECT        0x28
#define CFG_ID_BT_HID_SETTINGS          0x29
#define CFG_ID_LOW_LATENCY_SETTINGS     0x2A
#define CFG_ID_BTMUSIC_MULTI_DAE_SETTINGS  0x2B
#define CFG_ID_BT_MUSIC_VOLUME_SYNC     0x2C
#define CFG_ID_BT_MUSIC_STOP_HOLD       0x2D
#define CFG_ID_BT_TWO_DEVICE_PLAY       0x2E
#define CFG_ID_BT_CALL_VOLUME_SYNC      0x2F
#define CFG_ID_INCOMING_CALL_PROMPT     0x30
#define CFG_ID_CAP_TEMP_COMP            0x31
#define CFG_ID_LINEIN_DETECT            0x32
#define CFG_ID_BT_MUSIC_DAE             0x33
#define CFG_ID_BT_CALL_OUT_DAE          0x34
#define CFG_ID_BT_CALL_MIC_DAE          0x35
#define CFG_ID_LINEIN_OUT_DAE           0x36
#define CFG_ID_BT_CALL_QUALITY          0x37
#define CFG_ID_VOICE_PLAYER_PARAM       0x38
#define CFG_ID_VOICE_USER_SETTINGS      0x39
#define CFG_ID_TONE_PLAYER_PARAM        0x3A
#define CFG_ID_TONE_USER_SETTINGS       0x3B
#define CFG_ID_LINEIN_PLAYER_PARAM      0x3C
#define CFG_ID_LINEIN_USER_SETTINGS     0x3D
#define CFG_ID_BTMUSIC_PLAYER_PARAM     0x3E
#define CFG_ID_BTMUSIC_USER_SETTINGS    0x3F
#define CFG_ID_BTSPEECH_PLAYER_PARAM    0x40
#define CFG_ID_BTSPEECH_USER_SETTINGS   0x41
#define CFG_ID_IGSPEECH_PLAYER_PARAM    0x42
#define CFG_ID_IGSPEECH_USER_SETTINGS   0x43
#define CFG_ID_BLE_MANAGER              0x44
#define CFG_ID_BLE_ADVERTISING_MODE_1   0x45
#define CFG_ID_BLE_ADVERTISING_MODE_2   0x46
#define CFG_ID_BLE_CONNECTION_PARAM     0x47
#define CFG_ID_LE_AUDIO_MANAGER         0x48
#define CFG_ID_LE_AUDIO_ADVERTISING_MODE  0x49
#define CFG_ID_BLE_PASS_THROUGH         0x4A
#define CFG_ID_BT_LINK_QUALITY          0x4B
#define CFG_ID_BT_SCAN_PARAMS           0x4C
#define CFG_ID_APP_MUSIC                0x4D
#define CFG_ID_CARD_SETTINGS            0x4E
#define CFG_ID_USB_SETTINGS             0x4F
#define CFG_ID_USR_RESERVED_DATA        0x50
#define CFG_ID_SYS_RESERVED_DATA        0x51
#define CFG_ID_BT_DEBUG                 0x52
#define CFG_ID_SYSTEM_MORE_SETTINGS     0x53
#define CFG_ID_UPGRADE_MORE_SETTINGS    0x54
#define CFG_ID_DISPLAY_MORE_SETTINGS    0x55
#define CFG_ID_KEY_MORE_SETTINGS        0x56
#define CFG_ID_VOICE_MORE_SETTINGS      0x57
#define CFG_ID_AUDIO_MORE_SETTINGS      0x58
#define CFG_ID_VOLUME_GAIN_MORE_SETTINGS  0x59
#define CFG_ID_BT_MUSIC_MORE_SETTINGS   0x5A
#define CFG_ID_BT_CALL_MORE_SETTINGS    0x5B
#define CFG_ID_TR_BT_DEVICE             0x5C
#define CFG_ID_BLE_MANAGER_MORE_SETTINGS  0x5D
#define CFG_ID_LEAUDIO_CALL_OUT_DAE     0x5E
#define CFG_ID_LEAUDIO_CALL_MIC_DAE     0x5F


#endif  // __CONFIG_H__


