/*******************************************************************************
 * @file    sensor_dev.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2020-08-12
 * @brief   sensor testing module
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <sensor_hal.h>
#include <sensor_io.h>
#include <sensor_bus.h>
#include <zephyr.h>

/******************************************************************************/
//constants
/******************************************************************************/

/******************************************************************************/
//variables
/******************************************************************************/
__weak const sensor_dev_t sensor_dev[NUM_SENSOR] = {0};

static const char* sensor_name[NUM_SENSOR] = 
{
	"ACC", "GYRO", "MAG", "BARO", "TEMP", "HR", "GNSS", "OFFBODY",
};

static sensor_cb_t sensor_cb[NUM_SENSOR] = { NULL };
static void *sensor_cb_ctx[NUM_SENSOR] = { NULL };
static uint8_t sensor_en[NUM_SENSOR] = { 0 };
static uint8_t sensor_task[NUM_SENSOR] = { 0 };
static sensor_dat_t sensor_dat[NUM_SENSOR] = { 0 };

/******************************************************************************/
//functions
/******************************************************************************/
int sensor_hal_init(void)
{
	int id, ret = 0;
	const sensor_dev_t *dev;
	
	// init gpio
	for (id = 0; id < NUM_SENSOR; id ++) {
		dev = &sensor_dev[id];
		if (sensor_dev_is_valid(dev)) {
			// io init
			sensor_io_init(dev);
		}
	}
	
	// write config and probe
	for (id = 0; id < NUM_SENSOR; id ++) {
		dev = &sensor_dev[id];
		if (sensor_dev_is_valid(dev)) {
			// write init config
			sensor_dev_write_config(dev, CFG_INIT);
			
			// check chip id
			ret = sensor_dev_check_chipid(dev);
			if (ret != 0) {
				break;
			}
			
			// init device
			sensor_dev_init(dev);
			
			// self test
			sensor_dev_self_test(dev);
		}
	}
	
	return ret;
}

int sensor_hal_deinit(void)
{
	int id;
	const sensor_dev_t *dev;
	
	// sensor device deinit
	for (id = 0; id < NUM_SENSOR; id ++) {
		dev = &sensor_dev[id];
		if (sensor_dev_get_name(dev) != NULL) {
			// write off config
			sensor_dev_write_config(dev, CFG_OFF);
			
			// io deinit
			sensor_io_deinit(dev);
		}
	}
	
	return 0;
}

int sensor_hal_dump(void)
{
	int id, ret, idx, off;
	sensor_cfg_t cfg[4];
	const sensor_dev_t *dev;
	
	for (id = 0; id < NUM_SENSOR; id ++) {
		dev = &sensor_dev[id];
		if (sensor_dev_is_valid(dev)) {
			printf("[%s] dump reg\r\n", sensor_dev_get_name(dev));
			
			// read config
			ret = sensor_dev_read_config(dev, CFG_ON, cfg, 4);
			for (idx = 0; idx < ret; idx ++) {
				printf("[0x%x]=", cfg[idx].reg);
				for (off = 0; off < cfg[idx].len; off ++) {
					printf("0x%x ", cfg[idx].buf[off]);
				}
				printf("\r\n");
			}
			printf("\r\n");
		}
	}
		
	return 0;
}

static int sensor_is_wakeable(int id)
{
	return (id == ID_ACC) || (id == ID_HR);
}

int sensor_hal_suspend(void)
{
	int id;
	const sensor_dev_t *dev;
	
	for (id = 0; id < NUM_SENSOR; id ++) {
		dev = &sensor_dev[id];
		if (sensor_dev_is_valid(dev) && !sensor_is_wakeable(id) && sensor_en[id]) {
			// write off config
			printk("[%s] off\r\n", sensor_dev_get_name(dev));
			sensor_dev_write_config(dev, CFG_OFF);

			// io deinit
			sensor_io_deinit(dev);
		}
	}
	
	sensor_bus_task_wait(1);
	
	return 0;
}

int sensor_hal_resume(void)
{
	int id;
	const sensor_dev_t *dev;
	
	sensor_bus_task_wait(0);
	
	for (id = 0; id < NUM_SENSOR; id ++) {
		dev = &sensor_dev[id];
		if (sensor_dev_is_valid(dev) && !sensor_is_wakeable(id) && sensor_en[id]) {
			// io init
			sensor_io_init(dev);

			// write on config
			printk("[%s] on\r\n", sensor_dev_get_name(dev));
			sensor_dev_write_config(dev, CFG_ON);
		}
	}
	
	return 0;
}

const char *sensor_hal_get_type(int id)
{
	return sensor_name[id];
}

const char *sensor_hal_get_name(int id)
{
	return sensor_dev_get_name(&sensor_dev[id]);
}

void sensor_hal_add_callback(int id, sensor_cb_t cb, void *ctx)
{
	uint32_t key;
	
	key = irq_lock();
	sensor_cb[id] = cb;
	sensor_cb_ctx[id] = ctx;
	irq_unlock(key);
}

void sensor_hal_remove_callback(int id)
{
	uint32_t key;
	
	key = irq_lock();
	sensor_cb[id] = NULL;
	sensor_cb_ctx[id] = NULL;
	irq_unlock(key);
}

static void sensor_init_data(int id, sensor_dat_t *dat, int evt)
{
	const sensor_dev_t *dev = &sensor_dev[id];

	dat->id = id;
	dat->evt = evt;
	dat->pd = dev->hw.timeout;
	dat->sz = dev->hw.data_len;
	dat->ts = k_uptime_get_32();
}

static void sensor_irq_callback(int pin, int id)
{
	if (sensor_cb[id] != NULL) {
		// sensor irq event
		sensor_init_data(id, &sensor_dat[id], EVT_IRQ);
		sensor_dat[id].cnt = 0;
		sensor_dat[id].buf = NULL;
		
		// callback
		sensor_cb[id](id, &sensor_dat[id], sensor_cb_ctx[id]);
	}
}

static void sensor_task_callback(uint8_t *buf, int len, void *ctx)
{
	int id = (int)ctx;
	
	if (sensor_cb[id] != NULL) {
		// sensor task event
		sensor_init_data(id, &sensor_dat[id], EVT_TASK);
		sensor_dat[id].cnt = len / sensor_dat[id].sz;
		sensor_dat[id].buf = buf;
		
		// callback
		sensor_cb[id](id, &sensor_dat[id], sensor_cb_ctx[id]);
	}
}

int sensor_hal_enable(int id)
{
	int ret, task_poll = 0;
	uint8_t buf[16];	
	const sensor_dev_t *dev = &sensor_dev[id];

	if (!sensor_dev_is_valid(dev)) {
		return -1;
	}
	
	if (sensor_en[id]) {
		return 0;
	}
	
	// write on config
	ret = sensor_dev_write_config(dev, CFG_ON);
	if (ret != 0) {
		return -1;
	}
	sensor_en[id] = 1;
	
#ifdef CONFIG_SENSOR_TASK_POLL
	if (dev->task && (dev->hw.data_reg != REG_NULL)) {
		task_poll = 1;
	}
#endif

	// start task
	if (task_poll) {
		sensor_task[id] = 1;
		
		// disable irq
		sensor_io_disable_irq(dev, sensor_irq_callback, id);
		
		/* start task */
		ret = sensor_bus_task_start(dev, sensor_task_callback, (void*)id);
		if (ret != 0) {
			task_poll = 0;
		}
	}

	// enable irq
	if (!task_poll) {
		sensor_task[id] = 0;
		
		// enable irq
		sensor_io_enable_irq(dev, sensor_irq_callback, id);
	}
	
	// read data to clear irq
	sensor_dev_get_data(dev, buf, sizeof(buf));
	
	return 0;
}

int sensor_hal_disable(int id)
{
	int ret = 0;
	const sensor_dev_t *dev = &sensor_dev[id];

	if (!sensor_dev_is_valid(dev)) {
		return -1;
	}
	
	if (!sensor_en[id]) {
		return 0;
	}
	
	// write off config
	ret = sensor_dev_write_config(dev, CFG_OFF);
	if (ret != 0) {
		return -1;
	}
	sensor_en[id] = 0;
	
	// disable irq
	sensor_io_disable_irq(dev, sensor_irq_callback, id);
	
	// stop task
	if (sensor_task[id]) {
		sensor_task[id] = 0;
		ret = sensor_bus_task_stop(dev);
	}
	
	return ret;
}

int sensor_hal_read(int id, uint16_t reg, uint8_t *buf, uint16_t len)
{
	return sensor_bus_read(&sensor_dev[id], reg, buf, len);
}

int sensor_hal_write(int id, uint16_t reg, uint8_t *buf, uint16_t len)
{
	return sensor_bus_write(&sensor_dev[id], reg, buf, len);
}

int sensor_hal_poll_data(int id, sensor_dat_t *dat, uint8_t *buf)
{
	const sensor_dev_t *dev = &sensor_dev[id];
	int ret = -1;
	uint16_t status;
	
	// check buffer
	if ((buf == NULL) && (dat->buf == NULL)) {
		return -1;
	}
	
	// check status
	if (dev->hw.sta_reg != REG_NULL) {
		ret = sensor_bus_read(dev, dev->hw.sta_reg, (uint8_t*)&status, dev->hw.reg_len);
		if((ret != 0) || !(status & dev->hw.sta_rdy)) {
			return -2;
		}
	}
	
	// init data
	sensor_init_data(id, dat, EVT_IRQ);
	dat->cnt = 1;
	if (buf != NULL) {
		dat->buf = buf;
	}
	
	// get data	
	return sensor_dev_get_data(dev, dat->buf, dat->sz);
}

int sensor_hal_get_value(int id, sensor_dat_t *dat, uint16_t idx, float *val)
{
	// check args
	if ((dat->buf == NULL) || (idx >= dat->cnt) || (val == NULL)) {
		return -1;
	}
	
	// convert data
	return sensor_dev_cvt_data(&sensor_dev[id], dat->buf + dat->sz * idx, dat->sz, val);
}
