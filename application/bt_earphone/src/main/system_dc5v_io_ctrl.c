#include "system_app.h"
#include "app_config.h"
#include <drivers/hrtimer.h>

/* DC5V_IO 时序定义:
 __   _____________   _   _   _   _   ____
  S|S|             | | | | | |R| | | |X|P
   |_|             |_| |_| |_| |_| |_|_|
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9

  pos | logic | 
  -----------------------
  0   | 1     | start
  1   | 0     | start
  2-8 | 1     | sync head
  9   | 0     | sync head
  10  | x     | b6
  11  | x     | b5
  12  | x     | b4
  13  | x     | b3
  14  | x     | reverse(b3)
  15  | x     | b2
  16  | x     | b1
  17  | x     | b0
  18  | x     | parity(b0 ~ b6)
  19  | 1     | stop
 */

#define DC5V_IO_CMD_BIT_TIME_US  25000  // 每位时间 (微秒)
#define DC5V_IO_SAMPLES_PER_BIT  5      // 每位采样数

/* 命令全部位数 (起始位、同步头、数据位、反转位、奇偶位、停止位)
 */
#define DC5V_IO_CMD_TOTAL_BITS   20

#define DC5V_IO_CMD_TOTAL_SAMPLES  (DC5V_IO_CMD_TOTAL_BITS  * DC5V_IO_SAMPLES_PER_BIT)
#define DC5V_IO_SAMPLE_TIME_US     (DC5V_IO_CMD_BIT_TIME_US / DC5V_IO_SAMPLES_PER_BIT)


/* 命令重复时间 (传输时间 + 间隔时间)
 */
#define DC5V_IO_CMD_REPEAT_TIME_MS  \
    ((DC5V_IO_CMD_TOTAL_BITS * DC5V_IO_CMD_BIT_TIME_US / 1000) + 0)


/* 命令有效检查 (起始位、同步头、停止位)
 */
#define DC5V_IO_CMD_CHECK_MASK   0xffc01
#define DC5V_IO_CMD_CHECK_VAL    0xbf801


/* 解出命令 ID (7-bit 有效数据)
 */
static inline u32_t DC5V_IO_CMD_ID_UNPACK(u32_t cmd_bits)
{
    u32_t  cmd_id = 
        (((cmd_bits >> 6) & 0xf) << 3) | 
        (((cmd_bits >> 2) & 0x7) << 0);

    return cmd_id;
}


/* 检查反转位 (bit3 的反转位)
 */
static inline bool DC5V_IO_CMD_CHECK_REVERSE(u32_t cmd_bits, u32_t cmd_id)
{
    u32_t  reverse = (((cmd_id >> 3) & 1) ^ 1);

    return (reverse == ((cmd_bits >> 5) & 1));
}


/* 检查奇偶位 (7-bit 有效数据)
 */
static inline bool DC5V_IO_CMD_CHECK_PARITY(u32_t cmd_bits, u32_t cmd_id)
{
    u32_t  parity = 0;
    int     i;

    for (i = 0; i < 7; i++)
    {
        parity ^= ((cmd_id >> i) & 1);
    }

    return (parity == ((cmd_bits >> 1) & 1));
}

typedef struct
{
    u8_t   sample_count;
    u8_t   sample_pos;
    u8_t   sample_data[(DC5V_IO_CMD_TOTAL_SAMPLES + 7) / 8];

    u16_t  threshold_mv;
    u32_t  last_cmd_id;
    u32_t  last_cmd_time;  // 最后有效命令时间戳
    u32_t  last_io_time;   // 最后通讯时间戳
    struct hrtimer timer;
    os_delayed_work work;
    
} system_dc5v_io_ctrl_t;

system_dc5v_io_ctrl_t*  system_dc5v_io_ctrl;

static inline void system_dc5v_io_sample_put(u32_t level)
{
    system_dc5v_io_ctrl_t*  p = system_dc5v_io_ctrl;

    int  i = p->sample_pos;

    if (level)
    {
        p->sample_data[i / 8] |= (1 << (i % 8));
    }
    else
    {
        p->sample_data[i / 8] &= ~(1 << (i % 8));
    }

    p->sample_pos = (i + 1) % DC5V_IO_CMD_TOTAL_SAMPLES;

    if (p->sample_count < DC5V_IO_CMD_TOTAL_SAMPLES)
    {
        p->sample_count += 1;
    }
}

static inline u32_t system_dc5v_io_sample_get(int i)
{
    system_dc5v_io_ctrl_t*  p = system_dc5v_io_ctrl;

    i = (p->sample_pos + i) % DC5V_IO_CMD_TOTAL_SAMPLES;

    return ((p->sample_data[i / 8] >> (i % 8)) & 1);
}


static inline bool system_dc5v_io_check_start(void)
{
    system_dc5v_io_ctrl_t*  p = system_dc5v_io_ctrl;

    /* 检测起始位 (电平由高变低)
     */
    if (system_dc5v_io_sample_get(DC5V_IO_SAMPLES_PER_BIT - 1) == 1 &&
        system_dc5v_io_sample_get(DC5V_IO_SAMPLES_PER_BIT) == 0)
    {
        p->last_io_time = os_uptime_get_32();
        
        return true;
    }

    return false;
}

static inline u32_t system_dc5v_io_get_cmd_bits(void)
{
    u32_t  cmd_bits = 0;
#if 0
    int    i;

    for (i = 0; i < DC5V_IO_CMD_TOTAL_BITS; i++)
    {
        u32_t  bit_val = 0;
        int     j;

        /* 只取中间的采样
         */
        for (j = 1; j < DC5V_IO_SAMPLES_PER_BIT - 1; j++)
        {
            bit_val += system_dc5v_io_sample_get(i * DC5V_IO_SAMPLES_PER_BIT + j);
        }

        if (bit_val >= DC5V_IO_SAMPLES_PER_BIT - 2)
        {
            cmd_bits |= (1 << (DC5V_IO_CMD_TOTAL_BITS - 1 - i));
        }
        else if (bit_val == 0)
        {
            // cmd_bits |= (0 << (DC5V_IO_CMD_TOTAL_BITS - 1 - i));
        }
        else  // 错误采样?
        {
            return 0;
        }
    }

#else
    int  i = 0;
    int  j = 0;

    while (i < DC5V_IO_CMD_TOTAL_BITS &&
           j < DC5V_IO_CMD_TOTAL_SAMPLES)
    {
        u32_t  level = system_dc5v_io_sample_get(j);
        int     n = 1;

        j += 1;

        while (j < DC5V_IO_CMD_TOTAL_SAMPLES)
        {
            if (system_dc5v_io_sample_get(j) == level)
            {
                n += 1;
                j += 1;
            }
            else
            {
                break;
            }
        }

        n = (n + (DC5V_IO_SAMPLES_PER_BIT / 2)) / DC5V_IO_SAMPLES_PER_BIT;

        if (n == 0)
        {
            return 0;
        }

        if (n > (DC5V_IO_CMD_TOTAL_BITS - i))
        {
            n = (DC5V_IO_CMD_TOTAL_BITS - i);
        }

        i += n;

        if (level == 1)
        {
            cmd_bits |= (((1 << n) - 1) << (DC5V_IO_CMD_TOTAL_BITS - i));
        }
    }
#endif

    return cmd_bits;
}

#define DEBUG 1
#if DEBUG
#define p_debug(...)  	\
		do {	\
			printk(__VA_ARGS__);	\
		} while (0)
#else
#define p_debug(...)  	
#endif

static void system_dc5v_io_timer_handler(struct hrtimer *ttimer, void *expiry_fn_arg)
{
    system_dc5v_io_ctrl_t*  p = system_dc5v_io_ctrl;

    u32_t  sample;
    u32_t  cmd_bits;
    u32_t  cmd_id;
#ifdef CONFIG_POWER_MANAGER
    sample = power_manager_get_dc5v_voltage();
#else
    sample = 0;
#endif
    sample = (sample >= p->threshold_mv) ? 1 : 0;

    //p_debug("%c", (sample) ? '~' : '_');
    
    system_dc5v_io_sample_put(sample);

    do
    {
        bool  repeated = false;
        u32_t  ts;
        
        const char*  fmt_str = "DC5V_IO: 0x%x, 0x%x%s\n";
        
        /* 检测起始位 (电平由高变低)
         */
        if (!system_dc5v_io_check_start())
        {
            break;
        }

        cmd_bits = system_dc5v_io_get_cmd_bits();

        /* 命令有效检查 (起始位、同步头、停止位)
         */
        if ((cmd_bits & DC5V_IO_CMD_CHECK_MASK) != DC5V_IO_CMD_CHECK_VAL)
        {
            break;
        }

        cmd_id = DC5V_IO_CMD_ID_UNPACK(cmd_bits);

        /* 检查反转位
         */
        if (!DC5V_IO_CMD_CHECK_REVERSE(cmd_bits, cmd_id))
        {
            printk(fmt_str, cmd_bits, cmd_id, ", ERR_REVERSE");
            break;
        }

        /* 检查奇偶位
         */
        if (!DC5V_IO_CMD_CHECK_PARITY(cmd_bits, cmd_id))
        {
            printk(fmt_str, cmd_bits, cmd_id, ", ERR_PARITY");
            break;
        }
        
        p_debug(fmt_str, cmd_bits, cmd_id, "");

        ts = os_uptime_get_32();

        /* 是否为重复命令?
         */
        if (p->last_cmd_id == cmd_id &&
            (ts - p->last_cmd_time) < DC5V_IO_CMD_REPEAT_TIME_MS + 100)
        {
            repeated = true;
        }
        
        p->last_cmd_id   = cmd_id;
        p->last_cmd_time = ts;
        p->last_io_time  = ts;

        if (repeated)  // 重复命令不用处理
        {
            break;
        }

        /* 通过msg将命令转到 main 中进行处理,
         */
        os_delayed_work_submit(&p->work, 0);
    }
    while (0);
}

static void _system_dc5v_io_submit_cmd(struct k_work *work)
{
    struct app_msg msg = {0};

    msg.type = MSG_APP_DC5V_IO_CMD;
	msg.cmd = system_dc5v_io_ctrl->last_cmd_id;
	send_async_msg("main", &msg);

}

int system_dc5v_io_ctrl_init(void)
{
    system_dc5v_io_ctrl_t*  p = system_dc5v_io_ctrl;

    CFG_Type_DC5V_IO_Comm_Settings  cfg;
    
    app_config_read
    (
        CFG_ID_CHARGER_BOX,
        &cfg,
        offsetof(CFG_Struct_Charger_Box, DC5V_IO_Comm_Settings),
        sizeof(CFG_Type_DC5V_IO_Comm_Settings)
    );

    if (cfg.DC5V_IO_Threshold_MV == 0)
    {
        return -1;
    }

    SYS_LOG_INF("DC5V_IO_Threshold_MV %d", cfg.DC5V_IO_Threshold_MV);
    
    system_dc5v_io_ctrl =
        (p = app_mem_malloc(sizeof(system_dc5v_io_ctrl_t)));

	if (!p) return NO;
	
    p->threshold_mv = cfg.DC5V_IO_Threshold_MV;

    os_delayed_work_init(&p->work, _system_dc5v_io_submit_cmd);
    
    hrtimer_init(&p->timer, system_dc5v_io_timer_handler, NULL);

    hrtimer_start(&p->timer, DC5V_IO_SAMPLE_TIME_US, DC5V_IO_SAMPLE_TIME_US);

    return YES;

}

/* 最后通讯时间戳
 */
u32_t system_dc5v_io_last_time(void)
{
    system_dc5v_io_ctrl_t*  p = system_dc5v_io_ctrl;

    return (p != NULL) ? p->last_io_time : 0;
}


void system_dc5v_io_ctrl_suspend(void)
{
    system_dc5v_io_ctrl_t*  p = system_dc5v_io_ctrl;

    if (p != NULL)
    {
        SYS_LOG_INF("");
        hrtimer_stop(&p->timer);
        
        p->sample_pos   = 0;
        p->sample_count = 0;
    }
}


void system_dc5v_io_ctrl_resume(void)
{
    system_dc5v_io_ctrl_t*  p = system_dc5v_io_ctrl;

    if (p != NULL)
    {
        SYS_LOG_INF("");
        hrtimer_start(&p->timer, DC5V_IO_SAMPLE_TIME_US, DC5V_IO_SAMPLE_TIME_US);
    }
}
