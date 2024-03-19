/*
* common define for all pattern
*/
#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>
#include "att_pattern_test.h"


//#define true       1
//#define false      0

#define PASSED     0
#define FAILED     1

#define	RESET_ID_DMA			0
#define	RESET_ID_ADC			17

#define	CLOCK_ID_DMA			0
#define	CLOCK_ID_ADC			17

#define act_write(reg, val)       		  ((*((volatile unsigned int *)(reg))) = (val))
#define act_read(reg)             		  (*((volatile unsigned int *)reg))
#define act_or(reg,val)					  ((*((volatile unsigned int *)(reg))) |= (val))
#define act_and(reg,val)				  ((*((volatile unsigned int *)(reg))) &= (val))
#define act_mask_or(reg,mask,val)		  ((*((volatile unsigned int *)(reg))) = (*((volatile unsigned int *)(reg))) & (~make) | (val))

typedef uintptr_t mem_addr_t;

static inline void sys_set_bit(mem_addr_t addr, unsigned int bit)
{
	uint32_t temp = *(volatile uint32_t *)addr;

	*(volatile uint32_t *)addr = temp | (1 << bit);
}

static inline void sys_clear_bit(mem_addr_t addr, unsigned int bit)
{
	uint32_t temp = *(volatile uint32_t *)addr;

	*(volatile uint32_t *)addr = temp & ~(1 << bit);
}

void delay(unsigned int count);
void att_clock_peripheral_enable(int clock_id);
void att_clock_peripheral_disable(int clock_id);
void att_reset_peripheral_assert(int reset_id);
void att_reset_peripheral_deassert(int reset_id);
void att_reset_peripheral(int reset_id);


#endif
