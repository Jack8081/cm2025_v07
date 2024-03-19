/*
 * Copyright (c) 1997-2015, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <device.h>
#include <soc.h>
#include <soc_anc.h>
#include <drivers/anc.h>
#include "anc_inner.h"
#include <soc_regs.h>

#define CONFIG_ANC2CPU_COMMAND_ENABLE

#define ANC_BOOTARGS_CPU_ADDRESS 0x106A600
#define ANC_BOOTARGS_SIZE    0x20


struct anc_bootargs *anc_bootargs = (struct anc_bootargs *)ANC_BOOTARGS_CPU_ADDRESS;

static struct acts_ringbuf ringbuf_cmd_cpu2anc  __in_section_unique(ANC_SHARE_RAM);
static u8_t dbuf_cmd_cpu2anc[0x200] __in_section_unique(ANC_SHARE_RAM);
static u8_t anc_data_buf[0x180];

static void anc_listent_work_func(struct k_work *work);
K_DELAYED_WORK_DEFINE(anc_listent_work, anc_listent_work_func);
static struct acts_ringbuf ringbuf_cmd_anc2cpu  __in_section_unique(ANC_SHARE_RAM);
static u8_t dbuf_cmd_anc2cpu[0x40] __in_section_unique(ANC_SHARE_RAM);


#ifdef CONFIG_ANC_DEBUG_PRINT
#include <acts_ringbuf.h>
struct acts_ringbuf *anc_dev_get_print_buffer(void);
int anc_print_all_buff(void);
static void anc_print_work_func(struct k_work *work);
static struct acts_ringbuf debug_buff  __in_section_unique(ANC_SHARE_RAM);
static u8_t debug_data_buff[0x100] __in_section_unique(ANC_SHARE_RAM);
K_DELAYED_WORK_DEFINE(anc_print_work, anc_print_work_func);
char anc_hex_buf[0x200];
#endif

//static void anc_acts_isr(void *arg);
static void anc_acts_irq_enable(void);
static void anc_acts_irq_disable(void);

DEVICE_DECLARE(anc_acts);

static int anc_acts_request_mem(struct device *dev, int type)
{
	return anc_soc_request_mem();
}

static int anc_acts_release_mem(struct device *dev, int type)
{
	return anc_soc_release_mem();
}

static int anc_acts_power_on(struct device *dev, char hr)
{
	int cnt = 0;
	struct anc_acts_data *anc_data = dev->data;

	if (anc_data->pm_status != ANC_STATUS_POWEROFF)
		return 0;

	if (anc_data->images.size == 0) {
		SYS_LOG_ERR("%s: no image loaded\n", __func__);
		return -EFAULT;
	}

	anc_bootargs->boot_on = 0;

	anc_init_clk(hr);

	/* assert anc wait signal */
	anc_do_wait();

	anc_soc_request_mem();

	/* clear all pending */
	clear_anc_all_irq_pending();

	/* enable anc irq */
	anc_acts_irq_enable();

	/* deassert anc wait signal */
	anc_undo_wait();

	anc_reset_enable();

	/*anc reg access by anc*/
	anc_soc_request_reg();

	while(!anc_bootargs->boot_on){
		os_sleep(1);
		cnt++;
		if(cnt == 10){
			SYS_LOG_ERR("anc boot time out");
			return -1;
		}
	}

	anc_data->pm_status = ANC_STATUS_POWERON;

	SYS_LOG_ERR("ANC power on\n");

	//anc_dump_info();

	return 0;
}

static int anc_acts_power_off(struct device *dev)
{
	//const struct anc_acts_config *anc_cfg = dev->config;
	struct anc_acts_data *anc_data = dev->data;

	/* assert anc wait signal */
	anc_do_wait();

	/* disable anc irq */
	anc_acts_irq_disable();

	/* assert reset anc module */
	acts_reset_peripheral_assert(RESET_ID_ANC);

	/* disable anc clock*/
	acts_clock_peripheral_disable(CLOCK_ID_ANC);

	/* deassert anc wait signal */
	anc_undo_wait();

	anc_soc_release_mem();

	anc_soc_release_reg();

	//anc_deinit_clk();

	anc_data->pm_status = ANC_STATUS_POWEROFF;

#ifdef CONFIG_ANC_DEBUG_PRINT
	anc_print_all_buff();
	acts_ringbuf_drop_all(&debug_buff);
#endif

	SYS_LOG_INF("ANC power off\n");
	return 0;
}


static int anc_acts_get_status(struct device *dev)
{
	struct anc_acts_data *anc_data = dev->data;

	return anc_data->pm_status;
}

static int anc_acts_send_command(struct device *dev, int type, void *data, int size)
{
	int ret, cnt, key;
	struct anc_acts_command cmd;
	//const struct anc_acts_config *anc_cfg = dev->config;
	struct anc_acts_data *anc_data = dev->data;

	if (anc_data->pm_status == ANC_STATUS_POWEROFF)
	{
		SYS_LOG_ERR("anc is off\n");
		return -1;
	}

	if(k_mutex_lock(&anc_data->mutex, SYS_TIMEOUT_MS(500))){
		SYS_LOG_ERR("timeout");
		return -1;
	}

	SYS_LOG_DBG("type %d, size %d", type, size);

	cnt = 0;
	ret = acts_ringbuf_space(&ringbuf_cmd_cpu2anc);
	while(ret < size + sizeof(struct anc_acts_command)){
		cnt++;
		os_sleep(1);
		ret = acts_ringbuf_space(&ringbuf_cmd_cpu2anc);
		if(cnt >= 400){
			SYS_LOG_ERR("wait timeout,%d", ret);
			goto errexit;
		}
	}

	/*put cmd*/
	cmd.id = type;
	cmd.data_size = size;
	memcpy(anc_data_buf, &cmd, sizeof(struct anc_acts_command));
	memcpy(anc_data_buf+sizeof(struct anc_acts_command), data, size);
	key = os_irq_lock();
	ret = acts_ringbuf_put(&ringbuf_cmd_cpu2anc, anc_data_buf, sizeof(struct anc_acts_command)+size);
	os_irq_unlock(key);
	if(ret != (sizeof(struct anc_acts_command) + size)){
		SYS_LOG_ERR("put err");
		goto errexit;
	}

	k_mutex_unlock(&anc_data->mutex);
	SYS_LOG_DBG("send cmd success");
	return 0;
errexit:
	k_mutex_unlock(&anc_data->mutex);
	return -1;
}
#if 0
static void anc_acts_isr(void *arg)
{
	/* clear irq pending bits */
	//clear_anc_irq_pending(DT_INST_IRQN(0));
	clear_anc_irq_pending(0);

	anc_acts_recv_message(arg);
}
#endif

static int anc_acts_init(const struct device *dev)
{
	//const struct anc_acts_config *anc_cfg = dev->config;
	struct anc_acts_data *anc_data = dev->data;

    SYS_LOG_INF("##################################################in");
	memset(anc_bootargs, 0, sizeof(struct anc_bootargs));

	memset(dbuf_cmd_cpu2anc, 0, sizeof(dbuf_cmd_cpu2anc));
	acts_ringbuf_init(&ringbuf_cmd_cpu2anc, dbuf_cmd_cpu2anc, sizeof(dbuf_cmd_cpu2anc));
	ringbuf_cmd_cpu2anc.dsp_ptr = mcu_to_anc_address(ringbuf_cmd_cpu2anc.cpu_ptr, 0);
	anc_bootargs->cmd2anc = mcu_to_anc_address(POINTER_TO_UINT(&ringbuf_cmd_cpu2anc), DATA_ADDR);
	SYS_LOG_INF("cpu2anc addr 0x%x", ringbuf_cmd_cpu2anc.dsp_ptr);
	SYS_LOG_INF("cpu2anc data addr %p", dbuf_cmd_cpu2anc);
	SYS_LOG_INF("cpu2anc data anc addr 0x%x", mcu_to_anc_address(POINTER_TO_UINT(dbuf_cmd_cpu2anc), DATA_ADDR));

	memset(dbuf_cmd_anc2cpu, 0, sizeof(dbuf_cmd_anc2cpu));
	acts_ringbuf_init(&ringbuf_cmd_anc2cpu, dbuf_cmd_anc2cpu, sizeof(dbuf_cmd_anc2cpu));
	ringbuf_cmd_anc2cpu.dsp_ptr = mcu_to_anc_address(ringbuf_cmd_anc2cpu.cpu_ptr, 0);
	anc_bootargs->cmd2cpu = mcu_to_anc_address(POINTER_TO_UINT(&ringbuf_cmd_anc2cpu), DATA_ADDR);
	k_delayed_work_submit(&anc_listent_work, K_MSEC(100));
	SYS_LOG_INF("anc2cpu addr 0x%x", ringbuf_cmd_anc2cpu.dsp_ptr);
	SYS_LOG_INF("anc2cpu data addr %p", dbuf_cmd_anc2cpu);

#ifdef CONFIG_ANC_DEBUG_PRINT
	memset(debug_data_buff, 0, sizeof(debug_data_buff));
	acts_ringbuf_init(&debug_buff, debug_data_buff, sizeof(debug_data_buff));
	debug_buff.dsp_ptr = mcu_to_anc_address(debug_buff.cpu_ptr, 0);
	anc_bootargs->debug_buf = mcu_to_anc_address(POINTER_TO_UINT(&debug_buff), DATA_ADDR);
	k_delayed_work_submit(&anc_print_work, K_MSEC(10));
    SYS_LOG_INF("debug_buff anc addr 0x%x", anc_bootargs->debug_buf);
#endif

	k_sem_init(&anc_data->sem, 0, 1);
	k_mutex_init(&anc_data->mutex);

	anc_data->pm_status = ANC_STATUS_POWEROFF;
	return 0;
}

static struct anc_acts_data anc_acts_data;

static const struct anc_acts_config anc_acts_config = {

};

const struct anc_driver_api anc_drv_api = {
	.poweron = anc_acts_power_on,
	.poweroff = anc_acts_power_off,
	.request_image = anc_acts_request_image,
	.release_image = anc_acts_release_image,
	.request_mem = anc_acts_request_mem,
	.release_mem = anc_acts_release_mem,
	.get_status = anc_acts_get_status,
	.send_command = anc_acts_send_command,
};

DEVICE_DEFINE(anc_acts, CONFIG_ANC_NAME,
		anc_acts_init, NULL, &anc_acts_data, &anc_acts_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &anc_drv_api);

/*FIXME*/
static void anc_acts_irq_enable(void)
{
	// IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), anc_acts_isr, DEVICE_GET(anc_acts), 0);

	// irq_enable(DT_INST_IRQN(0));
}

/*FIXME*/
static void anc_acts_irq_disable(void)
{
	// irq_disable(DT_INST_IRQN(0));
}

static void anc_listent_work_func(struct k_work *work)
{
	int ret = 0;
	struct anc_acts_command cmd;
	struct anc_acts_data *data = DEVICE_GET(anc_acts)->data;

	if(data->pm_status == ANC_STATUS_POWERON)
	{
		ret = acts_ringbuf_length((struct acts_ringbuf*)anc_bootargs->cmd2cpu);
		if(ret >= sizeof(struct anc_acts_command)){
			acts_ringbuf_get((struct acts_ringbuf*)anc_bootargs->cmd2cpu, &cmd, sizeof(struct anc_acts_command));

			switch(cmd.id){
				// case ANC_COMMAND_SCENE:
				// 	SYS_LOG_INF("ANC_COMMAND_SCENE");
				// break;
			}
		}
	}
	else{
		acts_ringbuf_reset((struct acts_ringbuf*)anc_bootargs->cmd2cpu);
	}

	k_delayed_work_submit(&anc_listent_work, K_MSEC(100));
}


#ifdef CONFIG_ANC_DEBUG_PRINT
#define DEV_DATA(dev) \
	((struct anc_acts_data * const)(dev)->data)

struct acts_ringbuf *anc_dev_get_print_buffer(void)
{
	struct anc_acts_data *data = DEV_DATA(DEVICE_GET(anc_acts));

	if(!data->bootargs.debug_buf){
		return NULL;
	}

	return (struct acts_ringbuf *)anc_to_mcu_address(data->bootargs.debug_buf,DATA_ADDR);
}

int anc_print_all_buff(void)
{
	int i = 0;
	int length;
	struct acts_ringbuf *debug_buf = &debug_buff;
	if(!debug_buf)
		return 0;

	length = acts_ringbuf_length(debug_buf);

	if(length > acts_ringbuf_size(debug_buf)) {
		acts_ringbuf_drop_all(&debug_buff);
		length = 0;
	}

	if(length >= sizeof(anc_hex_buf)){
		length = sizeof(anc_hex_buf) - 2;
	}

	if(length){
		acts_ringbuf_get(debug_buf, &anc_hex_buf[0], length);
		for(i = 0; i < length / 2; i++){
			anc_hex_buf[i] = anc_hex_buf[2 * i];
		}
		anc_hex_buf[i] = 0;

		printk("[ANC PRINT]%s %d %d %d \n",anc_hex_buf,debug_buf->head,debug_buf->tail,debug_buf->mask);

		return acts_ringbuf_length(debug_buf);
	}
	return 0;
}
static void anc_print_work_func(struct k_work *work)
{
	while(anc_print_all_buff() > 0) {
		;
	}
	k_delayed_work_submit(&anc_print_work, K_MSEC(2));
}

#endif
