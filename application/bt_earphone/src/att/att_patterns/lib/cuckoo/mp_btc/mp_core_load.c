#include "att_pattern_test.h"
#include "code_in_btram0.h"
#include "mp_btc_inner.h"

#define MP_BTC_NAME  "mp_btc.bin"

static int mp_btc_load_code(void)
{
#if 0
	unsigned int i, *p;
	p = (unsigned int *)0x01200000;
	for(i=0;i<sizeof(code_in_btram0)/4;i++)
	{
		*(p+i) = code_in_btram0[i];
	}

	udelay(5);

	return 0;
#else
	atf_dir_t dir;
	int ret_val;
	ret_val = read_atf_sub_file(NULL, 0x10000, (const u8_t *)MP_BTC_NAME, 0, -1, &dir);
	if (ret_val <= 0) {
		LOG_E("read_file %s fail, quit!\n", MP_BTC_NAME);
		return -EIO;
	}

	LOG_I("Load BTC code finish!\n");
	
	return 0;
#endif	
}

void mp_btc_mem_init(void)
{
	LOG_I("memsrc:%x\n", *((REG32)(CMU_MEMCLKSRC0)));
	*((REG32)(CMU_MEMCLKEN0)) = 0xffffffff;
	*((REG32)(CMU_MEMCLKEN1)) = 0xffffffff;
	//change bt ram to cpu
	*((REG32)(CMU_MEMCLKSRC0)) &= (~0x3f000000);

	LOG_I("memsrc:%x\n", *((REG32)(CMU_MEMCLKSRC0)));

	//clear btc memory
	memset((uint8_t *)0x1200000, 0, 64 * 1024);

}

int mp_btc_core_load(void)
{
	int ret_val;

	*((REG32)(MEMORYCTL)) |= 0x1<<4;   // bt cpu boot from btram0

	ret_val = mp_btc_load_code();

	*((REG32)(CMU_MEMCLKSRC0))	|=	0x1<<24;												// change BT ram clk source
	//*((REG32)(CMU_MEMCLKSRC0))  |=  (0x2<<20)|(0x2<<22)|(0x2<<24);	// change AUDIO DSP ram clk source
	udelay(5);
	//*((REG32)(CMU_DEVCLKEN))  |= 0x1f000000;

	return ret_val;
}

void mp_btc_hardware_init(void)
{
	   /******************for BT*********************************************/
	//*((REG32)(CMU_DEVCLKEN1)) |= (0x1F<<24);

	*((REG32)(CMU_S1CLKCTL)) |=(0x7<<0);
	*((REG32)(CMU_S1BTCLKCTL)) |=(0x7<<0);

	*((REG32)(CMU_MEMCLKSRC0)) = (*((REG32)(CMU_MEMCLKSRC0)) & (~                    (0x7<<5))) | (1<<5);
	*((REG32)(CMU_MEMCLKSRC0)) = (*((REG32)(CMU_MEMCLKSRC0)) & (~(0x7<<25))) | (1<<25);
	*((REG32)(CMU_MEMCLKSRC0)) |= (1<<24);
	                                                    
	*((REG32)(CMU_MEMCLKSRC1)) = (*((REG32)(CMU_MEMCLKSRC1)) & (~(0x3<<14))) | (2<<14);
	*((REG32)(CMU_MEMCLKSRC1)) = (*((REG32)(CMU_MEMCLKSRC1)) & (~(0x3<<12))) | (2<<12);
	*((REG32)(CMU_MEMCLKSRC1)) = (*((REG32)(CMU_MEMCLKSRC1)) & (~(0x3<<10))) | (2<<10);
	*((REG32)(CMU_MEMCLKSRC1)) = (*((REG32)(CMU_MEMCLKSRC1)) & (~(0x3<<8))) | (2<<8);
	*((REG32)(CMU_MEMCLKSRC1)) = (*((REG32)(CMU_MEMCLKSRC1)) & (~(0x3<<6))) | (2<<6);	

	LOG_I("release BT\n");

	*((REG32)(RMU_MRCR0)) &= (~(1<<14));					// reset BT reset
	udelay(10);
	*((REG32)(RMU_MRCR0)) |= (1<<14);					// release BT reset	

	udelay(5000);
}

void mp_btc_hardware_deinit(void)
{
	*((REG32)(RMU_MRCR0)) &= (~(1<<14));					//  BT reset
}


/* init mp btc */
void mp_btc_init(void)
{
	mp_btc_deinit();

	mp_btc_mem_init();

	ipmsg_btc_update_bt_table((void *)BT_RAM_TABLE_ADDR);

	//init code
	mp_btc_core_load();

	//init hardware
	mp_btc_hardware_init();

	//init ipmsg communicate
	mp_btc_ipmsg_init();
}

void mp_btc_deinit(void)
{
	mp_btc_hardware_deinit();
}




