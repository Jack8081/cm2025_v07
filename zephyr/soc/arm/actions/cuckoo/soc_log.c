#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>

extern unsigned int debug_info[];

#define DEBUGSEL (0x40068400)
#define DEBUGOE  (0x40068420)

struct gpio_display_info {
	unsigned int cpu_pin_mask;
	unsigned int bt_pin_mask;
	unsigned char enable;
	unsigned char cpu_pin[DEBUG_CPU_GPIO_NUM];
};

struct gpio_display_info debug_ctrl;

static void gpio_high(int x)
{
	*((volatile unsigned int *)GPIO_REG_CTL(GPIO_REG_BASE, x)) = 0x3240;
	*((volatile unsigned int *)GPIO_REG_BSR(GPIO_REG_BASE, x)) = (1 << (x % 32));
}

static void gpio_low(int x)
{
	*((volatile unsigned int *)GPIO_REG_CTL(GPIO_REG_BASE, x)) = 0x3240;
	*((volatile unsigned int *)GPIO_REG_BRR(GPIO_REG_BASE, x)) = (1 << (x % 32));
}

static void gpio_display_val(int x, unsigned int val, int bits)
{
	unsigned int *gpio_ctrl = (unsigned int *)GPIO_REG_CTL(GPIO_REG_BASE, x);
	volatile register unsigned int *gpio_data = (unsigned int *)GPIO_REG_ODAT(GPIO_REG_BASE, x);
	register unsigned int reg_val = val;
	register int bits_cnt = bits;
	volatile register int i;
	register unsigned int x_mask = (1 << (x < 32 ? x : x - 32));

	*gpio_ctrl = 0x3240;
	while(bits_cnt-- > 0)
	{
		*gpio_data |= x_mask;
		if(reg_val & (1 << bits_cnt))
		{
			for(i = 3; i > 0; i--);
		}
		*gpio_data &= ~x_mask;
		if((bits_cnt & 0x7) == 0)
		{
			for(i = 1; i > 0; i--);
		}
	}
}

void debug_gpio_high(int index)
{
	if(index == -1) {
		return;
	}

	int x;
	unsigned int x_mask = 1 << index;

	if(!debug_ctrl.enable || !(debug_ctrl.cpu_pin_mask & x_mask)) {
		return;
	}
	
	x = __builtin_popcount(debug_ctrl.cpu_pin_mask & (x_mask - 1));
	x = debug_ctrl.cpu_pin[x];

	gpio_high(x);
}

void debug_gpio_low(int index)
{
	if(index == -1) {
		return;
	}

	int x;
	unsigned int x_mask = 1 << index;

	if(!debug_ctrl.enable || !(debug_ctrl.cpu_pin_mask & x_mask)) {
		return;
	}
	
	x = __builtin_popcount(debug_ctrl.cpu_pin_mask & (x_mask - 1));
	x = debug_ctrl.cpu_pin[x];

	gpio_low(x);
}


void debug_gpio_display(int index, unsigned int val, int bits, int base_status)
{
	if(index == -1) {
		return;
	}

	int x;
	unsigned int x_mask = 1 << index;

	if(!debug_ctrl.enable || !(debug_ctrl.cpu_pin_mask & x_mask)) {
		return;
	}
	
	x = __builtin_popcount(debug_ctrl.cpu_pin_mask & (x_mask - 1));
	x = debug_ctrl.cpu_pin[x];

	if(bits)
	{
		gpio_low(x);
		gpio_display_val(x, val, bits);
		gpio_low(x);
	}
	if(base_status)
	{
		gpio_high(x);
	}
}

void debug_gpio_enable(bool enable, int bt_mask, unsigned int valid_debug_signal, unsigned char *debug_pin)
{
	unsigned int i, count, val;
	
	if(enable) {
		debug_ctrl.enable = 1;
		debug_ctrl.cpu_pin_mask = (valid_debug_signal & DEBUG_CPU_GPIO_MASK) >> DEBUG_CPU_GPIO_SHIFT;
		debug_ctrl.bt_pin_mask = bt_mask;
		
		printk("debug_gpio_enable enabled:\n");
		
		if(debug_ctrl.cpu_pin_mask) {
			count = __builtin_popcount(debug_ctrl.cpu_pin_mask);
			memcpy(debug_ctrl.cpu_pin, debug_pin, count);
			
			printk("cpu_pin: ");
			for(i = 0; i < count; i++) {
				printk("%d, ", debug_ctrl.cpu_pin[i]);
			}
			printk("\n");
			if(debug_ctrl.cpu_pin_mask & (0xf << 12)) {
				debug_info[0] |= 0x1;
			}
		}
		if(debug_ctrl.bt_pin_mask) {
			const char bt_pin[6] = {7, 8, 19, 20, 21, 22};
			const char * bt_pin_name[6] = {"uart_32M", "irq", "sync_win", "sync_det", "rx", "tx"};
			count = __builtin_popcount(bt_mask);

			printk("bt_pin: ");
			val = 0;
			for(i = 0; i < count; i++) {
				if(debug_ctrl.bt_pin_mask & (1 << i)) {
					printk("%s(%d), ", bt_pin_name[i], bt_pin[i]);
					if(bt_pin[i] == 8) {
						sys_write32(0x00002027, 0x40068024);	//gpio8 used as GIO31 irq display
					} else {
						val |= 1 << bt_pin[i];
					}
				}
			}
			if(val & (3 << 21)) {
				sys_write32(0, JTAG_CTL);
			}
			sys_write32(0x0d, DEBUGSEL);
			sys_write32(val, DEBUGOE);
			printk("\n");
			printk("bt DEBUGOE %x\n", val);
		}
		
	} else {
		debug_ctrl.enable = 0;
		sys_write32(0, DEBUGOE);
		sys_write32(0, DEBUGSEL);
	}
}

