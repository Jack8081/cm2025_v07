/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file soc_sleep.c  sleep  for Actions SoC
 */


#include <zephyr.h>
#include <soc.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <string.h>
#include <drivers/timer/system_timer.h>
#include <linker/linker-defs.h>


#if defined(CONFIG_BOARD_NANDBOOT) || !defined(CONFIG_SPI_FLASH_ACTS)
void  sys_norflash_power_ctrl(uint32_t is_powerdown)
{

}
#else
extern  void  sys_norflash_power_ctrl(uint32_t is_powerdown);
#endif

/* COREPLL RW fields range from 0 t0 7 */
//static uint32_t ram_power_backup;
/* Save and restore the registers */
static const uint32_t backup_regs_addr[] = {
	CMU_MEMCLKSRC0,
	CMU_SYSCLK,
	CMU_SPI0CLK,
	ADC_REF_LDO_CTL,
	AVDDLDO_CTL,
	PMUADC_CTL,
	WIO0_CTL,
	WIO1_CTL,
	CMU_MEMCLKEN0,
	CMU_MEMCLKEN1,
	CMU_DEVCLKEN0,
	RMU_MRCR0,
	NVIC_ISER0,
	NVIC_ISER1,
	CMU_GPIOCLKCTL,
};

struct sleep_wk_data {
	uint16_t wksrc;
	uint16_t wk_en_bit;
	const char *wksrc_msg;
};

struct sleep_wk_data wk_msg[] = {
	{SLEEP_WK_SRC_BT, 		IRQ_ID_BT,  	"BT" },
	{SLEEP_WK_SRC_GPIO, 	IRQ_ID_GPIO,    "GPIO" },
	{SLEEP_WK_SRC_PMU, 		IRQ_ID_PMU,  	"PMU" },
	{SLEEP_WK_SRC_T0,		IRQ_ID_TIMER0, 	"T0" },
	{SLEEP_WK_SRC_T1,		IRQ_ID_TIMER1,	"T1" },
	{SLEEP_WK_SRC_TWS,		IRQ_ID_TWS,     "TWS"},

};
#define SLEEP_WKSRC_NUM ARRAY_SIZE(wk_msg)

static volatile uint16_t g_sleep_wksrc_en, g_sleep_wksrc_src;
uint32_t  g_sleep_cycle;

void sys_s3_wksrc_set(enum S_WK_SRC_TYPE src)
{
	g_sleep_wksrc_en |= (1 << src);
}

void sys_s3_wksrc_clr(enum S_WK_SRC_TYPE src)
{
	g_sleep_wksrc_en &= ~(1 << src);
}

enum S_WK_SRC_TYPE sys_s3_wksrc_get(void)
{
	return g_sleep_wksrc_src;
}

static enum S_WK_SRC_TYPE sys_sleep_check_wksrc(void)
{
	int i;
	uint32_t wk_pd0, wk_pd1,wkbit;
	
	g_sleep_wksrc_src = 0;
	wk_pd0 = sys_read32(NVIC_ISPR0);
	wk_pd1 = sys_read32(NVIC_ISPR1);
	//printk("WK NVIC_ISPR0=0x%x\n", wk_pd0);
	//printk("WK NVIC_ISPR1=0x%x\n", wk_pd1);
	
	for(i = 0; i < SLEEP_WKSRC_NUM; i++){
		if((1<<wk_msg[i].wksrc) & g_sleep_wksrc_en){
			wkbit = wk_msg[i].wk_en_bit;
			if(wkbit >= 32){
				wkbit -= 32;
				if(wk_pd1 & (1<<wkbit)) {
					break;
				}
			}else{
				if(wk_pd0 & (1<<wkbit)) {
					break;
				}
			}
		}
	}
	
	if(i != SLEEP_WKSRC_NUM){
		g_sleep_wksrc_src = wk_msg[i].wksrc;
		printk("wksrc=%s\n", wk_msg[i].wksrc_msg);
	}else{
		printk("no wksrc\n");
		g_sleep_wksrc_src = 0;
	}

	return g_sleep_wksrc_src;
}

static void sys_set_wksrc_before_sleep(void)
{
	int i;
	uint32_t wk_en0, wk_en1;

	//printk("NVIC_ISPR0-1=0x%x,0x%x\n", sys_read32(NVIC_ISPR0), sys_read32(NVIC_ISPR1));
	printk("NVIC_ISER0-1=0x%x,0x%x\n", sys_read32(NVIC_ISER0), sys_read32(NVIC_ISER1));
	//printk("NVIC_IABR0-1=0x%x,0x%x\n", sys_read32(NVIC_IABR0), sys_read32(NVIC_IABR1));
	//printk("g_sleep_wksrc_en =0x%x\n", g_sleep_wksrc_en);
	sys_write32(sys_read32(NVIC_ISER0), NVIC_ICER0);
	sys_write32(sys_read32(NVIC_ISER1), NVIC_ICER1);
	sys_write32(0xffffffff, NVIC_ICPR0);
	sys_write32(0xffffffff, NVIC_ICPR1);
	
	if(g_sleep_wksrc_en){
		wk_en0 = wk_en1 = 0;
		for(i = 0; i < SLEEP_WKSRC_NUM; i++){
			if((1 << wk_msg[i].wksrc) & g_sleep_wksrc_en){
				printk("%d wksrc=%s \n",i, wk_msg[i].wksrc_msg);
				if(wk_msg[i].wk_en_bit >= 32){
					wk_en1 |=  1 << (wk_msg[i].wk_en_bit-32);
				}else{
					wk_en0 |=  1 << (wk_msg[i].wk_en_bit);
				}
			}
		}
		
		if(wk_en0) {
			sys_write32(wk_en0, NVIC_ISER0);
		}
		if(wk_en1) {
			sys_write32(wk_en1, NVIC_ISER1);
		}
	}
	//printk("NVIC_ISPR0-1=0x%x,0x%x\n", sys_read32(NVIC_ISPR0), sys_read32(NVIC_ISPR1));
	printk("NVIC_ISER0-1=0x%x,0x%x\n", sys_read32(NVIC_ISER0), sys_read32(NVIC_ISER1));
	//printk("NVIC_IABR0-1=0x%x,0x%x\n", sys_read32(NVIC_IABR0), sys_read32(NVIC_IABR1));

}

static struct sleep_wk_fun_data *g_wk_fun[SLEEP_WKSRC_NUM] ;

struct sleep_wk_cb {
	sleep_wk_callback_t wk_cb;
	enum S_WK_SRC_TYPE src;
};

static struct sleep_wk_cb g_syste_cb[SLEEP_WKSRC_NUM];
static int g_num_cb;

int sleep_register_wk_callback(enum S_WK_SRC_TYPE wk_src, struct sleep_wk_fun_data *fn_data)
{
	struct sleep_wk_fun_data *p;
	if(fn_data == NULL) {
		return -1;
	}
	
	p = g_wk_fun[wk_src];

	while (p) {
		if (p == fn_data) {
			return 0;
		}
		p = p->next;
	}
	
	fn_data->next = g_wk_fun[wk_src];
	g_wk_fun[wk_src] = fn_data;
	return 0;
}

#if 0
static void wakeup_system_callback(void)
{
	int i;
	for(i = 0; i < g_num_cb; i++){
		g_syste_cb[i].wk_cb(g_syste_cb[i].src);
	}
	g_num_cb = 0;
}
#endif

__sleepfunc static enum WK_CB_RC check_wk_run_sram_nor(uint16_t *wk_en_bit,
				struct sleep_wk_cb *cb, int *cb_num)
{
	int i;
	enum WK_CB_RC rc = WK_CB_SLEEP_AGAIN;
	enum WK_RUN_TYPE runt;
	uint32_t wk_pd0, wk_pd1,wkbit;
	bool b_nor_wk = false;
	struct sleep_wk_fun_data *p;
	wk_pd0 = sys_read32(NVIC_ISPR0);
	wk_pd1 = sys_read32(NVIC_ISPR1);
	*cb_num = 0;

	for(i = 0; i < SLEEP_WKSRC_NUM; i++){
		if(!((1 << i) & g_sleep_wksrc_en))
			continue;

		wkbit = wk_en_bit[i];
		if(wkbit >= 32){
			wkbit -= 32;
			if(!(wk_pd1 & (1<<wkbit)))
				continue;

		}else{
			if(!(wk_pd0 & (1<<wkbit)))
				continue;
		}
		if(g_wk_fun[i]){
			p = g_wk_fun[i];
			do{
				if(p->wk_prep){
					runt = p->wk_prep(i);//
					if(runt == WK_RUN_IN_SRAM){
						if(WK_CB_RUN_SYSTEM == p->wk_cb(i)) //要求唤醒到系统继续运行
							rc = WK_CB_RUN_SYSTEM;
					}else if (runt == WK_RUN_IN_NOR) {// 唤醒nor 跑
						if(!b_nor_wk){//nor 退出idle
							b_nor_wk = true;
							sys_norflash_power_ctrl(0);
						}
						if(WK_CB_RUN_SYSTEM == p->wk_cb(i)) //要求唤醒到系统继续运行
							rc = WK_CB_RUN_SYSTEM;

					}else{
						rc = WK_CB_RUN_SYSTEM; /*系统跑*/
						if(*cb_num < SLEEP_WKSRC_NUM){
							cb[*cb_num].wk_cb = p->wk_cb;
							cb[*cb_num].src = i;
							(*cb_num)++;
						}
					}

				}else{
					rc = WK_CB_RUN_SYSTEM; /*系统跑*/
					if(*cb_num < SLEEP_WKSRC_NUM){
						cb[*cb_num].wk_cb = p->wk_cb;
						cb[*cb_num].src = i;
						(*cb_num)++;
					}
				}
				p = p->next;
			}while(p);
		}else{
			rc = WK_CB_RUN_SYSTEM; /*not wake up callback , system handle*/
		}

	}

	if(rc == WK_CB_SLEEP_AGAIN){
		sys_write32(0xffffffff, NVIC_ICPR0);
		sys_write32(0xffffffff, NVIC_ICPR1);
		if(b_nor_wk){ // 如果继续休眠，但是nor已经退出idle，重新进idle
			sys_norflash_power_ctrl(1);
		}
	}else{
		if(!b_nor_wk){// 如果系统跑，还没有退出idle，退出idle
			sys_norflash_power_ctrl(0);
		}
	}
	return rc;
}

static uint32_t s2_reg_backups[ARRAY_SIZE(backup_regs_addr)];

static void sys_pm_backup_registers(void)
{
	int i;

	//ram_power_backup = sys_read32(PWRGATE_RAM);

	for (i = 0; i < ARRAY_SIZE(backup_regs_addr); i++)
	{
		s2_reg_backups[i] = sys_read32(backup_regs_addr[i]);
		printk("0x%x 0x%x\n", backup_regs_addr[i], s2_reg_backups[i]);
	}
}

 static void sys_pm_restore_registers(void)
{
	int i;
	for (i = ARRAY_SIZE(backup_regs_addr) - 1; i >= 0; i--)
	{
		printk("0x%x 0x%x\n", backup_regs_addr[i], sys_read32(backup_regs_addr[i]));
		sys_write32(s2_reg_backups[i], backup_regs_addr[i]);
	}
}


typedef struct  {
	int32_t reg;
	const char *str;
}reg_t;

#if 0
static  reg_t regs[] = {
	{HOSC_CTL, 	"HOSC_CTL"},
	{HOSCLDO_CTL, 	"HOSCLDO_CTL"},
	{RC64M_CTL, 	"RC64M_CTL"},
	//{CK96M_CTL, 	"CK96M_CTL"},
	{LOSC_CTL, 	"LOSC_CTL"},
	{RC4M_CTL, 	"RC4M_CTL"},
	//{CK128M_CTL, 	"CK128M_CTL"},
	{AVDDLDO_CTL, 	"AVDDLDO_CTL"},
	{COREPLL_CTL, 	"COREPLL_CTL"},
	{AUDIO_PLL0_CTL, 	"AUDIO_PLL0_CTL"},
	{AUDIO_PLL1_CTL, 	"AUDIO_PLL1_CTL"},
	{HOSCOK_CTL, 	"HOSCOK_CTL"},

	{CMU_SYSCLK,	"CMU_SYSCLK"},
	{CMU_DEVCLKEN0,	"CMU_DEVCLKEN0"},
	{CMU_DEVCLKEN1,	"CMU_DEVCLKEN1"},
	{CMU_SPI0CLK, "CMU_SPI0CLK"},
	{CMU_ANCCLK,	"CMU_ANCCLK"},
	{CMU_DSPCLK,	"CMU_DSPCLK"},
	{CMU_ANCDSPCLK,	"CMU_ANCDSPCLK"},
	{CMU_DACCLK,	"CMU_DACCLK"},
	{CMU_ADCCLK,	"CMU_ADCCLK"},
	{CMU_I2STXCLK,	"CMU_I2STXCLK"},
	{CMU_I2SRXCLK,	"CMU_I2SRXCLK"},
	{CMU_SPDIFTXCLK,	"CMU_SPDIFTXCLK"},
	{CMU_SPDIFRXCLK,	"CMU_SPDIFRXCLK"},
	{CMU_MEMCLKEN0,	"CMU_MEMCLKEN0"},
	{CMU_MEMCLKEN1,	"CMU_MEMCLKEN1"},
	{CMU_MEMCLKSRC0,	"CMU_MEMCLKSRC0"},
	{CMU_MEMCLKSRC1,	"CMU_MEMCLKSRC1"},
	{CMU_S1CLKCTL,	"CMU_S1CLKCTL"},
	{CMU_S1BTCLKCTL,	"CMU_S1BTCLKCTL"},
	{CMU_S2SCLKCTL,	"CMU_S2SCLKCTL"},
	{CMU_S3CLKCTL,	"CMU_S3CLKCTL"},
	//{CUM_GPIOCLKCTL,	"CUM_GPIOCLKCTL"},
	{CMU_PMUWKUPCLKCTL,	"CMU_PMUWKUPCLKCTL"},
	{CMU_SPIMT0CLK,	"CMU_SPIMT0CLK"},
	{CMU_SPIMT1CLK,	"CMU_SPIMT1CLK"},
	{CMU_I2CMT0CLK,	"CMU_I2CMT0CLK"},
	{CMU_I2CMT1CLK,	"CMU_I2CMT1CLK"},

	{RMU_MRCR0,	"RMU_MRCR0"},
	{RMU_MRCR1,	"RMU_MRCR1"},
	{DSP_VCT_ADDR,	"DSP_VCT_ADDR"},
	{DSP_STATUS_EXT_CTL,	"DSP_STATUS_EXT_CTL"},
	{ANC_VCT_ADDR,	"ANC_VCT_ADDR"},
	{ANC_STATUS_EXT_CTL,	"ANC_STATUS_EXT_CTL"},
	{CHIPVERSION,	"CHIPVERSION "},

	{VOUT_CTL0 ,	"VOUT_CTL0"},
	{VOUT_CTL1_S1,	"VOUT_CTL1_S1"},
	{VOUT_CTL1_S2,	"VOUT_CTL1_S2"},
	{VOUT_CTL1_S3,	"VOUT_CTL1_S3"},
	{PMU_DET,	"PMU_DET"},
	{DCDC_VC18_CTL,	"DCDC_VC18_CTL"},
	{DCDC_VD12_CTL,	"DCDC_VD12_CTL"},
	{DCDC_VDD_CTL,	"DCDC_VDD_CTL"},
	{PWRGATE_DIG,	"PWRGATE_DIG"},
	{PWRGATE_DIG_ACK,	"PWRGATE_DIG_ACK"},
	{PWRGATE_RAM,	"PWRGATE_RAM"},
	{PWRGATE_RAM_ACK,	"PWRGATE_RAM_ACK"},
	{PMU_INTMASK,	"PMU_INTMASK"},

	{CHG_CTL_SVCC,    	"CHG_CTL_SVCC"},
	{BDG_CTL_SVCC,    	"BDG_CTL_SVCC"},
	{SYSTEM_SET_SVCC, 	"SYSTEM_SET_SVCC"},
	{POWER_CTL_SVCC,  	"POWER_CTL_SVCC"},
	{WKEN_CTL_SVCC,   	"WKEN_CTL_SVCC"},
	{WAKE_PD_SVCC,    	"WAKE_PD_SVCC"},
	{PWM_CTL_SVCC,    	"PWM_CTL_SVCC"},
	{PWM_DUTY_SVCC,   	"PWM_DUTY_SVCC"},
	{PMUADC_CTL,      	"PMUADC_CTL"},
	{SPICACHE_CTL, "SPICACHE_CTL"},
};
#else
static  reg_t regs[] = {
	{CMU_SYSCLK,	"CMU_SYSCLK"},
	{VOUT_CTL1_S1,	"VOUT_CTL1_S1"},
	{VOUT_CTL1_S3,	"VOUT_CTL1_S3"},
	{CMU_MEMCLKEN0,	"CMU_MEMCLKEN0"},
	{CMU_MEMCLKEN1,	"CMU_MEMCLKEN1"},
	{CMU_MEMCLKSRC0,	"CMU_MEMCLKSRC0"},
	{CMU_MEMCLKSRC1,	"CMU_MEMCLKSRC1"},
	{JTAG_CTL,	        "JTAG_CTL"},
};
#endif

void dump_reg(const char *promt)
{
	int i;

	printk("%s: reg dump\n", promt);
	
	for(i=0; i<ARRAY_SIZE(regs); i++) {
		printk("%s: 0x%08x\n", regs[i].str, sys_read32(regs[i].reg));
	}

	for(i=0;i<=30;i++) {
		printk("io%d: 0x%08x\n", i, sys_read32(GPIO_REG_BASE + 4*(i+1)));
	}
}



//static uint32_t pwgate_dis_bak;
static uint32_t pwgate_ram_bak;
void powergate_prepare_sleep(int isdeep)
{	
	//pwgate_dis_bak = sys_read32(PWRGATE_DIG);
	pwgate_ram_bak = sys_read32(PWRGATE_RAM);
	sys_write32(0x0008031f, PWRGATE_RAM);
	printk("PWRGATE_RAM: 0x%x 0x%x\n", sys_read32(PWRGATE_RAM), pwgate_ram_bak);
}

void powergate_prepare_wakeup(void)
{
	int count=0;
	//sys_write32(pwgate_dis_bak, PWRGATE_DIG);
	sys_write32(pwgate_ram_bak, PWRGATE_RAM);
	while ((sys_read32(PWRGATE_RAM_ACK)&0xff337) != (pwgate_ram_bak&0xff337))
	{
		count++;
	}
	printk("PWRGATE_RAM ack: 0x%x %d\n", sys_read32(PWRGATE_RAM_ACK), count);
}

static void soc_cmu_sleep_prepare(int isdeep)
{
	sys_pm_backup_registers();

	powergate_prepare_sleep(isdeep);
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk ;  // deepsleep
	/*spi0 clk switch to hosc*/
	//sys_write32(0x0, CMU_SPI0CLK);	
	sys_write32(sys_read32(CMU_MEMCLKSRC0)&~(0x1f<<5) , CMU_MEMCLKSRC0);// share ram clk switch to RC4M

	sys_write32(0x0, PMUADC_CTL);// close ADC
	sys_write32(0x0, CMU_GPIOCLKCTL); //select gpio clk RC32K

	sys_set_wksrc_before_sleep();
}


static void soc_cmu_sleep_exit(void)
{
	SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk ; 
	powergate_prepare_wakeup();
	sys_pm_restore_registers();	
}

static inline  unsigned int n_irq_lock(void)
{
	unsigned int key;
	unsigned int tmp;
	__asm__ volatile(
		"mov %1, %2;"
		"mrs %0, BASEPRI;"
		"msr BASEPRI, %1;"
		"isb;"
		: "=r"(key), "=r"(tmp)
		: "i"(_EXC_IRQ_DEFAULT_PRIO)
		: "memory");

	return key;
}

static inline  void n_irq_unlock(unsigned int key)
{
	__asm__ volatile(
		"msr BASEPRI, %0;"
		"isb;"
		:  : "r"(key) : "memory");
}

__ramfunc static void cpu_enter_sleep(void)
{
#if 1
	volatile int loop;
	uint32_t corepll_backup;

	//jtag_enable();

	sys_norflash_power_ctrl(1);/*nor enter deep power down */
	//sys_write32(sys_read32(RC128M_CTL)&0xfe, RC128M_CTL);

	corepll_backup = sys_read32(COREPLL_CTL);
	sys_write32(0x0, CMU_SYSCLK); /*cpu clk select rc4M*/	
	sys_write32(sys_read32(COREPLL_CTL) & ~(1 << 7), COREPLL_CTL);

	/*spi0 & spi1 cache disable*/
	sys_clear_bit(SPICACHE_CTL, 0); //bit0 disable spi 0 cache

	sys_clear_bit(AVDDLDO_CTL, 0);  /*disable avdd, corepll use must enable*/
	loop=100;
	while(loop)loop--;	
	

	/*enter sleep*/
	__asm__ volatile("cpsid	i");

	n_irq_unlock(0);

	__asm__ volatile("dsb");
	__asm__ volatile("wfi");

	n_irq_lock();

	__asm__ volatile("cpsie	i");
	
	sys_set_bit(AVDDLDO_CTL, 0);  /*enable avdd, for pll*/

	loop=300;
	while(loop)loop--; /*for avdd*/

	/*spi0 & spi1 cache enable*/
	sys_set_bit(SPICACHE_CTL, 0); //enable spi 0 cache
	
	sys_write32(corepll_backup, COREPLL_CTL);
		
	//sys_write32(sys_read32(RC128M_CTL)|1, RC128M_CTL);
	loop=200;
	while(loop--); /*for avdd*/

	if ((sys_read32(CMU_S1CLKCTL) & (1 << 2)) && ((sys_read32(CMU_SPI0CLK)&(3<<8)) == 0x300)) {
		uint32_t tmp;

		/* calibrate CK64M clock */
		sys_write32(0x8020140C, RC64M_CTL);

		/* wait calibration done */
		while(!(sys_read32(RC64M_CTL) & (1 << 24)))
		{
			;
		}

		tmp = sys_read32(RC64M_CTL) >> 25;
		tmp -= 3;

		sys_write32(0x80201000|(tmp<<4), RC64M_CTL);
	}

	sys_norflash_power_ctrl(0);//;//nor exit deep power down
	
#else
	k_busy_wait(2000000); //wati 2s

#endif

}

unsigned int soc_sleep_cycle(void)
{
	return g_sleep_cycle;
}


void soc_enter_deep_sleep(void)
{
	unsigned int i, num_cb;
	uint16_t wk_en_bit[SLEEP_WKSRC_NUM];
	struct sleep_wk_cb cb[SLEEP_WKSRC_NUM];

	for(i = 0; i < SLEEP_WKSRC_NUM; i++) // copy nor to sram, nor is not use in sleep func
		wk_en_bit[i] = wk_msg[i].wk_en_bit;

	sys_s3_wksrc_set(SLEEP_WK_SRC_BT);
	sys_s3_wksrc_set(SLEEP_WK_SRC_TWS);
	sys_s3_wksrc_set(SLEEP_WK_SRC_PMU);
	soc_cmu_sleep_prepare(1);
	dump_reg("dump");

	while(1){
		printk("enter sleep\n");
		cpu_enter_sleep();//wfi,enter to sleep

		if(WK_CB_RUN_SYSTEM == check_wk_run_sram_nor(wk_en_bit, cb, &num_cb))
			break;
	}

	g_num_cb = num_cb;
	memcpy(g_syste_cb, cb, num_cb*sizeof(struct sleep_wk_cb));

	soc_cmu_sleep_exit();
	sys_sleep_check_wksrc();

	//wakeup_system_callback();
}

void soc_enter_light_sleep(void)
{
	dump_reg("before");  	
	soc_cmu_sleep_prepare(0);	
	cpu_enter_sleep();//wfi,enter to sleep 
	soc_cmu_sleep_exit();	
	dump_reg("after");
}

