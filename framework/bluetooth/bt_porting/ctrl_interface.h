/*
 * Copyright(c) 2017-2027 Actions (Zhuhai) Technology Co., Limited,
 *                            All Rights Reserved.
 *
 * Johnny           2017-5-4        create initial version
 */

#ifndef __CTRL_INTERFACE_H
#define __CTRL_INTERFACE_H

/*******************************
 * type definitions
 *******************************/
struct le_link_time {
    uint64_t ce_cnt:39;
    uint32_t ce_interval_us;
    int32_t  ce_offs_us;
};

enum {
    CTRL_TIMER_TYPE_NATIVE = 0,
    CTRL_TIMER_TYPE_TWS = 1,
    CTRL_TIMER_TYPE_CIG = 2,
    CTRL_TIMER_TYPE_CIS = 3,
};

typedef struct bt_clock
{
    unsigned int bt_clk;  /* by 312.5us. the bit0 and bit1 are 0.  range is 0 ~ 0x0FFFFFFF.  */
    unsigned short bt_intraslot;  /* by 1us.  range is 0 ~ 1249.  */
} t_devicelink_clock;

typedef void (*ctrl_timer_cbk_t)(uint32_t cbk_param);
struct ctrl_timer_param {
	uint8_t type;
	uint8_t is_period:1;
	uint16_t handle;

	union {
		t_devicelink_clock bt_abs_clk;
		int32_t ce_offs_us;
	};

	ctrl_timer_cbk_t cbk;
	uint32_t cbk_param;
};

#if 1
// TODO:
inline unsigned char ctrl_get_le_link_time(unsigned short handle, struct le_link_time *link_time)
{
	return 0;
}

inline uint32_t ctrl_timer_request(struct ctrl_timer_param *ctimer_param)
{
	return 0;
}

inline void ctrl_timer_free(uint32_t ctimer_id)
{
}

inline void ctrl_timer_start(uint32_t ctimer_id)
{
}

inline void ctrl_timer_stop(uint32_t ctimer_id)
{
}
#endif

#endif
