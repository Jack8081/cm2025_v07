/*
 * Copyright (c) 1997-2015, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <device.h>
#include <soc.h>
#include <soc_dsp.h>
#include <drivers/dsp.h>
#include "dsp_inner.h"
#include <soc_regs.h>
#include <board_cfg.h>
#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

//#define CONFIG_DSP_DEBUG_GPIO_ENABLE
#define CONFIG_DSP_DEBUG_GPIO_SEL 0x7FF

static struct dsp_acts_config_entity dsp_cfg_entity __in_section_unique(DSP_SHARE_CONFIG_RAM);

#ifdef CONFIG_DSP_DEBUG_PRINT
#include <acts_ringbuf.h>
struct acts_ringbuf *dsp_dev_get_print_buffer(void);
static void dsp_print_work(struct k_work *work);
static struct acts_ringbuf debug_buff  __in_section_unique(DSP_SHARE_RAM);
unsigned int debug_info[1]  __in_section_unique(DSP_SHARE_RAM);
static u8_t debug_data_buff[0x200] __in_section_unique(DSP_SHARE_RAM);
K_DELAYED_WORK_DEFINE(print_work, dsp_print_work);
char hex_buf[0x200] __in_section_unique(dsp_hex_buf);

#ifdef CONFIG_SOFT_BREAKPOINT_DUMP_MEMORY

int dsp_dump_all_dsp_memory_flag;

#ifdef CONFIG_WATCHDOG
#include <watchdog_hal.h>
#endif

static void dump_dsp_memory(uint32_t start_addr, uint32_t dsp_addr, uint32_t bytes)
{
	uint32_t dump_addr = start_addr;
    uint32_t display_addr = dsp_addr;
	int32_t remain_bytes = bytes;
	uint32_t *p_u32_data;
	while (remain_bytes > 0) {
		p_u32_data = (uint32_t *)dump_addr;
		printk("[%08x] %08x %08x %08x %08x\n", display_addr, p_u32_data[0], p_u32_data[1], p_u32_data[2], p_u32_data[3]);
		dump_addr += 16;
        display_addr += 8; /* short */
		remain_bytes -= 16;
		k_busy_wait(1000);
		/**clear watchdog */
#ifdef CONFIG_WATCHDOG
		watchdog_clear();
#endif
	}
}

static void dump_all_dsp_memory_in_soft_breakpoint(void)
{
	printk("\n\ndsp trap into soft breakpoint, dump all memory:\n\n");

	//switch all ext memory to cpu access
	dsp_soc_release_addr(CPU_EXT_BASE);

	//dump dsp extern ram
	printk("dump dsp extern ram:\n");
	dump_dsp_memory(CPU_EXT_BASE, DSP_EXT_BASE, EXT_MEM_SIZE + 1);

	//dump share ram
	printk("dump share ram:\n");
	dump_dsp_memory(CPU_SHARE_BASE, DSP_SHARE_BASE, DSP_SHARE_MEM_SIZE + 1);

    //switch all ext memory to dsp access, so dsp can copy dtcm to ext memory
    dsp_soc_request_addr(CPU_EXT_BASE);

    //request dsp copy all dtcm to ext memory
    *(volatile uint32_t *)(DSP_DEBUG_REGION_REGISTER_BASE + 0) = 0x207c626b;
    *(volatile uint32_t *)(DSP_DEBUG_REGION_REGISTER_BASE + 4) = 0x6b627c20;

    while (1) {
        if ((*(volatile uint32_t *)(DSP_DEBUG_REGION_REGISTER_BASE + 0) == 0)
            && (*(volatile uint32_t *)(DSP_DEBUG_REGION_REGISTER_BASE + 4) == 0)) {
            break;
        } else {
            k_busy_wait(1000);
            /**clear watchdog */
#ifdef CONFIG_WATCHDOG
            watchdog_clear();
#endif
        }
    }

    //switch all memory to cpu access
    dsp_soc_release_mem(0);

	//dump dsp dtcm
	printk("dump dsp dtcm:\n");
    dump_dsp_memory(CPU_EXT_BASE, DSP_DTCM_BASE, (DTCM_SIZE + 1) * 2);

	//dump dsp ptcm
	printk("dump dsp ptcm:\n");
	dump_dsp_memory(CPU_PTCM_BASE, DSP_PTCM_BASE, (PTCM_SIZE + 1) * 2);

    //switch all memory to dsp access
	dsp_soc_request_mem(0);
}
#endif

int dsp_print_all_buff(void)
{
	int i = 0;
	int length;
	struct acts_ringbuf *debug_buf = &debug_buff;

#ifdef CONFIG_SOFT_BREAKPOINT_DUMP_MEMORY
	if (dsp_dump_all_dsp_memory_flag == 1) {
		dsp_dump_all_dsp_memory_flag = 0;

		dump_all_dsp_memory_in_soft_breakpoint();
	}
#endif

	if(!debug_buf)
		return 0;

	length = acts_ringbuf_length(debug_buf);

	if(length > acts_ringbuf_size(debug_buf)) {
		acts_ringbuf_drop_all(&debug_buff);
		length = 0;
	}

	if(length >= sizeof(hex_buf)){
		length = sizeof(hex_buf) - 2;
	}

	if(length){
		//int index = 0;
		acts_ringbuf_get(debug_buf, &hex_buf[0], length);
		for(i = 0; i < length / 2; i++){
			hex_buf[i] = hex_buf[2 * i];
		}
		hex_buf[i] = 0;

#ifdef CONFIG_ACTIONS_PRINTK_DMA			
		uart_dma_send_buf_ex(hex_buf,i);
#else
		printk("%s\n",hex_buf);
#endif	

		return acts_ringbuf_length(debug_buf);
	}
	return 0;
}
static void dsp_print_work(struct k_work *work)
{
	while(dsp_print_all_buff() > 0) {
		;
	}
	k_delayed_work_submit(&print_work, K_MSEC(2));
}
#endif

static void dsp_acts_isr(void *arg);
static void dsp_acts_irq_enable(void);
static void dsp_acts_irq_disable(void);

DEVICE_DECLARE(dsp_acts);

int wait_hw_idle_timeout(int usec_to_wait)
{
	do {
		if (dsp_check_hw_idle())
			return 0;
		if (usec_to_wait-- <= 0)
			break;
		k_busy_wait(1);
	} while (1);

	return -EAGAIN;
}

static int dsp_acts_suspend(struct device *dev)
{
	struct dsp_acts_data *dsp_data = dev->data;

	if (dsp_data->pm_status != DSP_STATUS_POWERON)
		return 0;

	/* already idle ? */
	if (!get_hw_idle()) {
		struct dsp_message message = { .id = DSP_MSG_SUSPEND, };

		k_sem_reset(&dsp_data->msg_sem);

		if (dsp_acts_send_message(dev, &message))
			return -EBUSY;

		/* wait message DSP_MSG_BOOTUP */
		if (k_sem_take(&dsp_data->msg_sem, SYS_TIMEOUT_MS(20))) {
			SYS_LOG_ERR("dsp image <%s> busy\n", dsp_data->images[DSP_IMAGE_MAIN].name);
			return -EBUSY;
		}

		/* make sure hardware is really idle */
		if (wait_hw_idle_timeout(10))
			SYS_LOG_ERR("DSP wait idle signal timeout\n");
	}

	dsp_do_wait();
	acts_clock_peripheral_disable(CLOCK_ID_DSP);
	acts_clock_peripheral_disable(CLOCK_ID_AUDDSPTIMER);

	dsp_data->pm_status = DSP_STATUS_SUSPENDED;
	SYS_LOG_ERR("DSP suspended\n");
	return 0;
}

static int dsp_acts_resume(struct device *dev)
{
	struct dsp_acts_data *dsp_data = dev->data;
	struct dsp_message message = { .id = DSP_MSG_RESUME, };

	if (dsp_data->pm_status != DSP_STATUS_SUSPENDED)
		return 0;

	acts_clock_peripheral_enable(CLOCK_ID_DSP);
	acts_clock_peripheral_enable(CLOCK_ID_AUDDSPTIMER);

	dsp_undo_wait();

	if (dsp_acts_send_message(dev, &message)) {
		SYS_LOG_ERR("DSP resume failed\n");
		return -EFAULT;
	}

	dsp_data->pm_status = DSP_STATUS_POWERON;
	SYS_LOG_ERR("DSP resumed\n");
	return 0;
}

static int dsp_acts_kick(struct device *dev, uint32_t owner, uint32_t event, uint32_t params)
{
	struct dsp_acts_data *dsp_data = dev->data;
	const struct dsp_acts_config *dsp_cfg = dev->config;
	struct dsp_protocol_userinfo *dsp_userinfo = dsp_cfg->dsp_userinfo;
	struct dsp_message message = {
		.id = DSP_MSG_KICK,
		.owner = owner,
		.param1 = event,
		.param2 = params,
	};

	if (dsp_data->pm_status != DSP_STATUS_POWERON)
		return -EACCES;

	if (dsp_userinfo->task_state != DSP_TASK_SUSPENDED)
		return -EINPROGRESS;

	return dsp_acts_send_message(dev, &message);
}

static int dsp_acts_request_mem(struct device *dev, int type)
{
	return dsp_soc_request_mem(type);
}

static int dsp_acts_release_mem(struct device *dev, int type)
{
	return dsp_soc_release_mem(type);
}

/* synchronize cpu and dsp clocks, both base hw timer */
static int dsp_acts_synchronize_clock(struct device *dev)
{
	const struct dsp_acts_config *dsp_cfg = dev->config;
	volatile uint16_t *p_dsp_sync_clock_status = (volatile uint16_t *)(dsp_cfg->dsp_debuginfo + 14);
	volatile uint32_t *p_dsp_sync_clock_cycles = (volatile uint32_t *)(dsp_cfg->dsp_debuginfo + 15);
	uint32_t flags;
	uint16_t sync_clock_status, sync_clock_status_last;
	uint32_t sync_clock_cycles_cpu, sync_clock_cycles_cpu_end;
	uint32_t sync_clock_cycles_cpu_diff, sync_clock_cycles_cpu_diff_last = 0xffff0000;
	uint32_t sync_clock_cycles_dsp;
	uint32_t sync_clock_cycles_offset;
	uint32_t try_count = 0, match_count = 0;
	uint32_t jitter_cycles_threshold = 16;

	uint32_t log_cpu;
	uint32_t log_dsp;
	
	printk("synchronize cpu and dsp clocks start\n");
	
	flags = irq_lock();

	sync_clock_cycles_cpu = k_cycle_get_32();
	*p_dsp_sync_clock_cycles = sync_clock_cycles_cpu;
	*p_dsp_sync_clock_status = DSP_SYNC_CLOCK_START;
	sync_clock_status_last = DSP_SYNC_CLOCK_START;

	while (1) {
		sync_clock_status = *p_dsp_sync_clock_status;
		if (sync_clock_status_last == DSP_SYNC_CLOCK_START) {
			if (sync_clock_status == DSP_SYNC_CLOCK_REPLY) {
				sync_clock_cycles_dsp = *p_dsp_sync_clock_cycles;
				sync_clock_cycles_cpu_end = k_cycle_get_32();
				sync_clock_cycles_cpu_diff = sync_clock_cycles_cpu_end - sync_clock_cycles_cpu;
				
				if ((sync_clock_cycles_cpu_diff >= sync_clock_cycles_cpu_diff_last && 
				sync_clock_cycles_cpu_diff <= sync_clock_cycles_cpu_diff_last + jitter_cycles_threshold) ||
					(sync_clock_cycles_cpu_diff <= sync_clock_cycles_cpu_diff_last && 
				sync_clock_cycles_cpu_diff + jitter_cycles_threshold >= sync_clock_cycles_cpu_diff_last)) {
					match_count++;
				} else {
					match_count = 0;
				}
				try_count++;
				
				sync_clock_cycles_cpu_diff_last = sync_clock_cycles_cpu_diff;
				
				if (match_count >= 5) {
					sync_clock_cycles_offset = sync_clock_cycles_cpu - sync_clock_cycles_dsp;
					log_cpu = sync_clock_cycles_cpu;
					log_dsp = sync_clock_cycles_dsp;
					*p_dsp_sync_clock_cycles = sync_clock_cycles_offset;
					*p_dsp_sync_clock_status = DSP_SYNC_CLOCK_OFFSET;
					sync_clock_status_last = DSP_SYNC_CLOCK_OFFSET;
				} else {
					sync_clock_cycles_cpu = k_cycle_get_32();
					*p_dsp_sync_clock_cycles = sync_clock_cycles_cpu;
					*p_dsp_sync_clock_status = DSP_SYNC_CLOCK_START;
				}

				if (try_count > 20) {
					try_count = 0;
					match_count = 0;
					jitter_cycles_threshold += 16;
				}
			}
		} else {
			if (sync_clock_status == DSP_SYNC_CLOCK_CAL) {
				sync_clock_cycles_dsp = *p_dsp_sync_clock_cycles;
				sync_clock_cycles_cpu = k_cycle_get_32();
				*p_dsp_sync_clock_status = DSP_SYNC_CLOCK_NULL;
				break;
			}
		}
	}

	irq_unlock(flags);

	k_busy_wait(10); /* wait dsp recv dsp_sync_clock_null */

	printk("sync clock : 0x%x, 0x%x, %d, %d\n", sync_clock_cycles_cpu, sync_clock_cycles_dsp, \
		sync_clock_cycles_cpu_diff_last, jitter_cycles_threshold);

	return 0;
}

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
#ifdef DSP_DYNAMIC_FREQ_ADJ_ENABLE

static struct dsp_freqadj_config_params s_config_params;
static int dsp_freqadj_flag = 2;
static int dsp_freqadj_enable_last = 0;

static int dsp_acts_freqadj_config(struct dsp_freqadj_config_params *config_params)
{
	memcpy(&s_config_params, config_params, sizeof(struct dsp_freqadj_config_params));
	SYS_LOG_INF("DSP freqadj %d\n", s_config_params.enable);
	if (s_config_params.enable)
		*(volatile uint16_t *)(DSP_FREQADJ_ENABLE_REGISTER_BASE) |= 0x01;
	else
		*(volatile uint16_t *)(DSP_FREQADJ_ENABLE_REGISTER_BASE) &= ~0x01;

	if (dsp_freqadj_enable_last == 1 && s_config_params.enable == 0 && dsp_freqadj_flag != 2) {
		//up freqadj forcely
		if (s_config_params.dsp_freqadj_up) {
			s_config_params.dsp_freqadj_up();
			SYS_LOG_INF("U forcely");
		}
		dsp_freqadj_flag = 2;
	}

	dsp_freqadj_enable_last = s_config_params.enable;

	return 0;
}

int dsp_acts_freqadj_set(struct device *dev, int flag)
{
	if (s_config_params.enable == 0 || flag == dsp_freqadj_flag)
		return 0;

	if (flag == 0) {
		//sleep freqadj
		if (s_config_params.dsp_freqadj_sleep)
			s_config_params.dsp_freqadj_sleep();
	} else if (flag == 1) {
		//low freqadj
		if (s_config_params.dsp_freqadj_down)
			s_config_params.dsp_freqadj_down();
	} else {
		//high freqadj
		if (s_config_params.dsp_freqadj_up)
			s_config_params.dsp_freqadj_up();
	}
	dsp_freqadj_flag = flag;

	return 0;
}

static int dsp_acts_power_on_freqadj(struct device *dev)
{
	dsp_freqadj_flag = 2;
	dsp_freqadj_enable_last = 0;
	memset(&s_config_params, 0, sizeof(s_config_params));

	dvfs_set_dsp_freqadj_config_cbk(dsp_acts_freqadj_config);

	return 0;
}

#endif
#endif

static int dsp_acts_power_on(struct device *dev, void *cmdbuf)
{
	struct dsp_acts_data *dsp_data = dev->data;
	const struct dsp_acts_config *dsp_cfg = dev->config;
	struct dsp_protocol_userinfo *dsp_userinfo = dsp_cfg->dsp_userinfo;
	uint32_t dsp_clk_source;
	uint32_t dsp_max_freq_mhz;
	int i;

	if (dsp_data->pm_status != DSP_STATUS_POWEROFF)
		return 0;

	if (dsp_data->images[DSP_IMAGE_MAIN].size == 0) {
		SYS_LOG_ERR("%s: no image loaded\n", __func__);
		return -EFAULT;
	}

	if (cmdbuf == NULL) {
		SYS_LOG_ERR("%s: must assign a command buffer\n", __func__);
		return -EINVAL;
	}

	memset((void *)&dsp_cfg_entity, 0, 0x80);

#ifdef CONFIG_DSP_DEBUG_GPIO_ENABLE
	*(volatile uint32_t *)(0x40068400) = 0x1f;
	*(volatile uint32_t *)(0x40068420) = CONFIG_DSP_DEBUG_GPIO_SEL;
#endif

#ifdef CONFIG_SOC_SERIES_CUCKOO
	dsp_clk_source = soc_freq_get_target_dsp_src();
	dsp_max_freq_mhz = soc_freq_get_target_dsp_freq_mhz();
#endif

#ifdef CONFIG_DSP_DEBUG_PRINT
	k_delayed_work_submit(&print_work, K_MSEC(2));
#endif
	dsp_init_clk();

	/* enable dsp clock*/
	acts_clock_peripheral_enable(CLOCK_ID_DSP);
	acts_clock_peripheral_enable(CLOCK_ID_AUDDSPTIMER);

	/* assert reset */
	acts_reset_peripheral_assert(RESET_ID_DSP);
#if defined(CONFIG_SOC_SERIES_CUCKOO) || defined(CONFIG_SOC_SERIES_CUCKOO_FPGA)
	acts_reset_peripheral_assert(RESET_ID_DSP_PART);
#endif

	/* set bootargs */
	dsp_data->bootargs.command_buffer = mcu_to_dsp_address((uint32_t)cmdbuf, DATA_ADDR);
	dsp_data->bootargs.sub_entry_point = dsp_data->images[1].entry_point;
#ifdef CONFIG_SOC_SERIES_CUCKOO
	dsp_data->bootargs.dsp_clk_source = dsp_clk_source;
	dsp_data->bootargs.dsp_max_freq_mhz = dsp_max_freq_mhz;
	SYS_LOG_INF("dsp src %d, max %d mhz\n", dsp_clk_source, dsp_max_freq_mhz);
#endif

	/* assert dsp wait signal */
	dsp_do_wait();

	dsp_acts_request_mem(dev, 0);

	/* intialize shared registers */
	dsp_cfg->dsp_mailbox->msg = MAILBOX_MSG(DSP_MSG_NULL, MSG_FLAG_ACK) ;
	dsp_cfg->dsp_mailbox->owner = 0;
	dsp_cfg->cpu_mailbox->msg = MAILBOX_MSG(DSP_MSG_NULL, MSG_FLAG_ACK) ;
	dsp_cfg->cpu_mailbox->owner = 0;

	dsp_userinfo->task_state = DSP_TASK_PRESTART;
	dsp_userinfo->error_code = DSP_NO_ERROR;

	/* set dsp vector_address */
	//set_dsp_vector_addr((unsigned int)DSP_ADDR);

	dsp_data->pm_status = DSP_STATUS_POWERON;

	/* clear all pending */
	clear_dsp_all_irq_pending();

	/* enable dsp irq */
	dsp_acts_irq_enable();

	/* deassert dsp wait signal */
	dsp_undo_wait();

	k_sem_reset(&dsp_data->msg_sem);
	/* deassert reset */
	acts_reset_peripheral_deassert(RESET_ID_DSP);
#if defined(CONFIG_SOC_SERIES_CUCKOO) || defined(CONFIG_SOC_SERIES_CUCKOO_FPGA)
	acts_reset_peripheral_deassert(RESET_ID_DSP_PART);
#endif

	/* wait message DSP_MSG_STATE_CHANGED.DSP_TASK_STARTED */
	if (k_sem_take(&dsp_data->msg_sem, SYS_TIMEOUT_MS(100))) {
		SYS_LOG_ERR("dsp image <%s> cannot boot up\n", dsp_data->images[DSP_IMAGE_MAIN].name);
		dsp_dump_info();
		goto cleanup;
	}

	if (dsp_data->need_sync_clock == 'C')
	{
		dsp_acts_synchronize_clock(dev);
	}

	/* FIXME: convert userinfo address from dsp to cpu here, since
	 * dsp will never touch it again.
	 */
	dsp_userinfo->func_table = dsp_to_mcu_address(dsp_userinfo->func_table, DATA_ADDR);
	for (i = 0; i < dsp_userinfo->func_size; i++) {
		volatile uint32_t addr = dsp_userinfo->func_table + i * 4;
		if (addr > 0)   /* NULL, no information provided */
			sys_write32(dsp_to_mcu_address(sys_read32(addr), DATA_ADDR), addr);
	}

	//SYS_LOG_ERR("DSP power on %d %p\n", dsp_userinfo->func_size, dsp_userinfo->func_table);

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
#ifdef DSP_DYNAMIC_FREQ_ADJ_ENABLE
	dsp_acts_power_on_freqadj(dev);
#endif
#endif

	return 0;
cleanup:
	dsp_acts_irq_disable();
	acts_clock_peripheral_disable(CLOCK_ID_DSP);
	acts_clock_peripheral_disable(CLOCK_ID_AUDDSPTIMER);
	acts_reset_peripheral_assert(RESET_ID_DSP);
#if defined(CONFIG_SOC_SERIES_CUCKOO) || defined(CONFIG_SOC_SERIES_CUCKOO_FPGA)
	acts_reset_peripheral_assert(RESET_ID_DSP_PART);
#endif

	return -ETIMEDOUT;
}

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
#ifdef DSP_DYNAMIC_FREQ_ADJ_ENABLE

static int dsp_acts_power_off_freqadj(struct device *dev)
{
	*(volatile uint16_t *)(DSP_FREQADJ_ENABLE_REGISTER_BASE) &= ~0x1;
	if (s_config_params.enable == 1 && dsp_freqadj_flag != 2) {
		//up freqadj forcely
		if (s_config_params.dsp_freqadj_up) {
			s_config_params.dsp_freqadj_up();
			SYS_LOG_INF("U forcely");
		}
		dsp_freqadj_flag = 2;
	}
	dsp_freqadj_enable_last = s_config_params.enable = 0;

	dvfs_set_dsp_freqadj_config_cbk(NULL);

	return 0;
}

#endif
#endif

static int dsp_acts_power_off(struct device *dev)
{
	const struct dsp_acts_config *dsp_cfg = dev->config;
	struct dsp_acts_data *dsp_data = dev->data;

	if (dsp_data->pm_status == DSP_STATUS_POWEROFF)
		return 0;

#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
#ifdef DSP_DYNAMIC_FREQ_ADJ_ENABLE
	dsp_acts_power_off_freqadj(dev);
#endif
#endif

	/* assert dsp wait signal */
	dsp_do_wait();

	/* disable dsp irq */
	dsp_acts_irq_disable();

	/* assert reset dsp module */
	acts_reset_peripheral_assert(RESET_ID_DSP);
#if defined(CONFIG_SOC_SERIES_CUCKOO) || defined(CONFIG_SOC_SERIES_CUCKOO_FPGA)
	acts_reset_peripheral_assert(RESET_ID_DSP_PART);
#endif

	/* disable dsp clock*/
	acts_clock_peripheral_disable(CLOCK_ID_DSP);
	acts_clock_peripheral_disable(CLOCK_ID_AUDDSPTIMER);

	/* deassert dsp wait signal */
	dsp_undo_wait();

	/* clear page mapping */
	clear_dsp_pageaddr();

	dsp_cfg->dsp_userinfo->task_state = DSP_TASK_DEAD;
	dsp_data->pm_status = DSP_STATUS_POWEROFF;
#ifdef CONFIG_DSP_DEBUG_PRINT
	if (!k_delayed_work_cancel(&print_work))
	{
		dsp_print_all_buff();
		acts_ringbuf_drop_all(&debug_buff);
	}
#endif

	dsp_deinit_clk();

	/* notify the system to lower VD12 */
	clk_set_rate(CLOCK_ID_DSP, 0);

	SYS_LOG_INF("DSP power off\n");
	return 0;
}

static int acts_request_userinfo(struct device *dev, int request, void *info)
{
	const struct dsp_acts_config *dsp_cfg = dev->config;
	struct dsp_protocol_userinfo *dsp_userinfo = dsp_cfg->dsp_userinfo;
	union {
		struct dsp_request_session *ssn;
		struct dsp_request_function *func;
	} req_info;

	switch (request) {
	case DSP_REQUEST_TASK_STATE:
		*(int *)info = dsp_userinfo->task_state;
		break;
	case DSP_REQUEST_ERROR_CODE:
		*(int *)info = dsp_userinfo->error_code;
		break;
	case DSP_REQUEST_SESSION_INFO:
		req_info.ssn = info;
		req_info.ssn->func_enabled = dsp_userinfo->func_enabled;
		req_info.ssn->func_runnable = dsp_userinfo->func_runnable;
		req_info.ssn->func_counter = dsp_userinfo->func_counter;
		req_info.ssn->info = (void *)dsp_to_mcu_address(dsp_userinfo->ssn_info, DATA_ADDR);
		break;
	case DSP_REQUEST_FUNCTION_INFO:
		req_info.func = info;
		if (req_info.func->id < dsp_userinfo->func_size)
			req_info.func->info = (void *)sys_read32(dsp_userinfo->func_table + req_info.func->id * 4);
		break;
	case DSP_REQUEST_USER_DEFINED:
    	{
            uint32_t index = *(uint32_t*)info;
            *(uint32_t*)info = *(volatile uint32_t *)(dsp_cfg->dsp_debuginfo + index);
            break;
    	}
	default:
		return -EINVAL;
	}

	return 0;
}

static void dsp_acts_isr(void *arg)
{
	/* clear irq pending bits */
	clear_dsp_irq_pending(IRQ_ID_DSP);

	dsp_acts_recv_message(arg);
}

static int dsp_acts_init(const struct device *dev)
{
	const struct dsp_acts_config *dsp_cfg = dev->config;
	struct dsp_acts_data *dsp_data = dev->data;

#ifdef CONFIG_DSP_DEBUG_PRINT
	acts_ringbuf_init(&debug_buff, debug_data_buff, sizeof(debug_data_buff));
	debug_buff.dsp_ptr = mcu_to_dsp_address(debug_buff.cpu_ptr, 0);
	dsp_data->bootargs.debug_buf = mcu_to_dsp_address(POINTER_TO_UINT(&debug_buff), DATA_ADDR);
	//k_delayed_work_submit(&print_work, K_MSEC(10));
#endif

	debug_info[0] = 0;	//= DSP_DEBUG_FLAG_DATA_IND;
	dsp_data->bootargs.debug_info_buf = mcu_to_dsp_address(POINTER_TO_UINT(debug_info), DATA_ADDR);;

	dsp_cfg->dsp_userinfo->task_state = DSP_TASK_DEAD;
	k_sem_init(&dsp_data->msg_sem, 0, 1);
	k_mutex_init(&dsp_data->msg_mutex);

	dsp_data->pm_status = DSP_STATUS_POWEROFF;
    memset(&dsp_data->images, 0, sizeof(dsp_data->images));
	memset(dsp_cfg->dsp_mailbox, 0, sizeof(struct dsp_protocol_mailbox));
	memset(dsp_cfg->cpu_mailbox, 0, sizeof(struct dsp_protocol_mailbox));
	memset(dsp_cfg->dsp_userinfo, 0, sizeof(struct dsp_protocol_userinfo));
	return 0;
}

static struct dsp_acts_data dsp_acts_data __in_section_unique(DSP_SHARE_RAM);

static const struct dsp_acts_config dsp_acts_config = {
	.dsp_mailbox = &dsp_cfg_entity.dsp_mailbox,
	.cpu_mailbox = &dsp_cfg_entity.cpu_mailbox,
	.dsp_userinfo = &dsp_cfg_entity.dsp_userinfo,
	.dsp_debuginfo = dsp_cfg_entity.dsp_debuginfo,
};

const struct dsp_driver_api dsp_drv_api = {
	.poweron = dsp_acts_power_on,
	.poweroff = dsp_acts_power_off,
	.suspend = dsp_acts_suspend,
	.resume = dsp_acts_resume,
	.kick = dsp_acts_kick,
	.register_message_handler = dsp_acts_register_message_handler,
	.unregister_message_handler = dsp_acts_unregister_message_handler,
	.request_image = dsp_acts_request_image,
	.release_image = dsp_acts_release_image,
	.request_mem = dsp_acts_request_mem,
	.release_mem = dsp_acts_release_mem,
	.send_message = dsp_acts_send_message,
	.request_userinfo = acts_request_userinfo,
};

DEVICE_DEFINE(dsp_acts, CONFIG_DSP_NAME,
		dsp_acts_init, NULL, &dsp_acts_data, &dsp_acts_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &dsp_drv_api);

static void dsp_acts_irq_enable(void)
{
	IRQ_CONNECT(IRQ_ID_DSP, CONFIG_DSP_IRQ_PRI, dsp_acts_isr, DEVICE_GET(dsp_acts), 0);

	irq_enable(IRQ_ID_DSP);
}

static void dsp_acts_irq_disable(void)
{
	irq_disable(IRQ_ID_DSP);
}

#ifdef CONFIG_DSP_DEBUG_PRINT
#define DEV_DATA(dev) \
	((struct dsp_acts_data * const)(dev)->data)

struct acts_ringbuf *dsp_dev_get_print_buffer(void)
{
	struct dsp_acts_data *data = DEV_DATA(DEVICE_GET(dsp_acts));

	if(!data->bootargs.debug_buf){
		return NULL;
	}

	return (struct acts_ringbuf *)dsp_to_mcu_address(data->bootargs.debug_buf,DATA_ADDR);
}
#endif
