#include "lvgl_tick.h"
#include <zephyr.h>

static uint32_t sys_cycle;
static uint32_t sys_time_ms;

uint32_t lvgl_tick_get()
{
	uint32_t cycle = k_cycle_get_32();

	sys_time_ms += k_cyc_to_ms_floor32(cycle - sys_cycle);
	sys_cycle = cycle;

	return sys_time_ms;
}
