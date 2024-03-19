/*******************************************************************************
 * @file    sensor_bus.c
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
#include <sensor_bus.h>
#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/i2cmt.h>
#include <drivers/spimt.h>

/******************************************************************************/
//constants
/******************************************************************************/

/******************************************************************************/
//variables
/******************************************************************************/
#ifdef CONFIG_SENSOR_TASK_CFG
static struct k_sem sensor_sem[NUM_BUS][2];
static __act_s2_sleep_data volatile int8_t sensor_stat[NUM_BUS][2] = { 0 };
static __act_s2_sleep_data uint32_t sensor_wait = 0;
#endif

static __act_s2_sleep_data struct device *spimt_dev[2] = { NULL };
static __act_s2_sleep_data struct device *i2cmt_dev[2] = { NULL };

/******************************************************************************/
//functions
/******************************************************************************/
#ifdef CONFIG_SENSOR_TASK_CFG

static void sensor_bus_callback(unsigned char *buf, int len, void *ctx)
{
	const sensor_dev_t *dev = (const sensor_dev_t*)ctx;
	
	// sensor status
	if (buf != NULL) {
		sensor_stat[dev->io.bus_type][dev->io.bus_id] = 0;
	} else {
		sensor_stat[dev->io.bus_type][dev->io.bus_id] = -1;
	}
	
	// sensor sync
	k_sem_give(&sensor_sem[dev->io.bus_type][dev->io.bus_id]);
}

static int sensor_bus_i2c_xfer(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len, uint8_t rd)
{
	i2c_task_t i2c_task, dat_task;
	uint32_t task_id, task_off, two_task, data_task_id;
	uint8_t wr_buf[16];
	int ret;
	
	if (dev->task == NULL) {
		return -1;
	}

	/* config task */
	two_task = 0;
	task_id = I2C_TASK_NUM - 1;
	data_task_id = task_id;
	task_off = dev->io.bus_id * I2C_TASK_NUM + task_id;
	i2c_task = *(i2c_task_t*)dev->task;
	i2c_task.irq_type = I2C_TASK_IRQ_CMPLT | I2C_TASK_IRQ_NACK;
	i2c_task.ctl.soft = 1;
	i2c_task.ctl.rwsel = rd;
	i2c_task.ctl.sdataddr = reg;
	i2c_task.ctl.rdlen_wsdat = len;
	i2c_task.dma.reload = 0;
	i2c_task.dma.addr = (uint32_t)buf;
	i2c_task.dma.len = len;
	i2c_task.trig.task = I2CMT0_TASK0 + task_off;
	
	// ignore data address
	if (reg == REG_NULL) {
		i2c_task.ctl.tdataddr = 1;
	} else {
		// process 2 bytes address
		if (dev->hw.adr_len > 1) {
			if (!rd) {
				if ((len + 1) > sizeof(wr_buf)) {
					return -2;
				}
				// write reg_h + wr_buf(reg_l + buf)
				wr_buf[0] = (reg & 0xff);
				if (len > 0) {
					memcpy(&wr_buf[1], buf, len);
				}
				i2c_task.ctl.sdataddr = (reg >> 8);
				i2c_task.dma.addr = (uint32_t)wr_buf;
				i2c_task.dma.len = len + 1;
			} else {
				two_task = 1;
				// read data in task2 (trigger by task3)
				dat_task = i2c_task;
				dat_task.ctl.soft = 0;
				dat_task.ctl.tdataddr = 1;
				dat_task.trig.chan = PPI_CH11;
				dat_task.trig.trig = I2CMT0_TASK0_CIP + task_off;
				dat_task.trig.task = I2CMT0_TASK0 + task_off - 1;
				data_task_id = task_id - 1;
				
				// write 2 bytes address in task3 (single write mode)
				i2c_task.irq_type = 0;
				i2c_task.ctl.rwsel = 0;
				i2c_task.ctl.swen = 1;
				i2c_task.ctl.sdataddr = (reg >> 8);
				i2c_task.ctl.rdlen_wsdat = (reg & 0xff);
				i2c_task.dma.len = 0;
			}
		}
	}
	
	/* start task */
	if (two_task) {
		i2c_register_callback(i2cmt_dev[dev->io.bus_id], data_task_id, sensor_bus_callback, (void*)dev);
		i2c_task_start(i2cmt_dev[dev->io.bus_id], data_task_id, &dat_task);
	} else {
		i2c_register_callback(i2cmt_dev[dev->io.bus_id], task_id, sensor_bus_callback, (void*)dev);
	}
	i2c_task_start(i2cmt_dev[dev->io.bus_id], task_id, &i2c_task);
	
	/* wait task */
	if (sensor_wait) {
		while(!i2c_task_get_data(dev->io.bus_id, data_task_id, -1, NULL));
		sensor_stat[dev->io.bus_type][dev->io.bus_id] = 0;
	} else {
		ret = k_sem_take(&sensor_sem[dev->io.bus_type][dev->io.bus_id], K_MSEC(20));
		if (ret) {
			sensor_stat[dev->io.bus_type][dev->io.bus_id] = -1;
//			printk("sensor: %s i2c timeout\n", dev->hw.name);
		}
	}
	
	/* stop task */
	if (two_task) {
		i2c_task_stop(i2cmt_dev[dev->io.bus_id], data_task_id);
	}
	i2c_task_stop(i2cmt_dev[dev->io.bus_id], task_id);
	i2c_register_callback(i2cmt_dev[dev->io.bus_id], task_id, NULL, NULL);

	return sensor_stat[dev->io.bus_type][dev->io.bus_id];
}

static int sensor_bus_spi_xfer(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len, uint8_t rd)
{
	spi_task_t spi_task;
	uint32_t task_id, task_off;
	int ret;
	
	if (dev->task == NULL) {
		return -1;
	}

	/* config task */
	spi_task = *(spi_task_t*)dev->task;
	spi_task.irq_type = SPI_TASK_IRQ_CMPLT;
	spi_task.ctl.soft = 1;
	spi_task.ctl.rwsel = rd;
	spi_task.ctl.sbsh = (dev->hw.adr_len > 1);
	spi_task.ctl.wsdat = reg;
	if (rd) {
		if (dev->hw.adr_len > 1) {
			spi_task.ctl.wsdat |= (1 << 15);
		} else {
			spi_task.ctl.wsdat |= (1 << 7);
		}
	}
	spi_task.ctl.rdlen = len;
	spi_task.dma.reload = 0;
	spi_task.dma.addr = (uint32_t)buf;
	spi_task.dma.len = len;
	
	if (dev->io.bus_cs == 0) {
		task_id = 1;//SPI_TASK_NUM / 2 - 1;
	} else {
		task_id = SPI_TASK_NUM - 1;
	}
	task_off = dev->io.bus_id * SPI_TASK_NUM + task_id;
	spi_task.trig.task = SPIMT0_TASK0 + task_off;
	
	/* start task */
	spi_register_callback(spimt_dev[dev->io.bus_id], task_id, sensor_bus_callback, (void*)dev);
	spi_task_start(spimt_dev[dev->io.bus_id], task_id, &spi_task);
	
	/* wait task */
	if (sensor_wait) {
	#if IS_ENABLED(CONFIG_SPIMT_0) || IS_ENABLED(CONFIG_SPIMT_1)
		while(!spi_task_get_data(dev->io.bus_id, task_id, -1, NULL));
	#endif
		sensor_stat[dev->io.bus_type][dev->io.bus_id] = 0;
	} else {
		ret = k_sem_take(&sensor_sem[dev->io.bus_type][dev->io.bus_id], K_MSEC(20));
		if (ret) {
			sensor_stat[dev->io.bus_type][dev->io.bus_id] = -1;
//			printk("sensor: %s spi timeout\n", dev->hw.name);
		}
	}
	
	/* stop task */
	spi_task_stop(spimt_dev[dev->io.bus_id], task_id);
	spi_register_callback(spimt_dev[dev->io.bus_id], task_id, NULL, NULL);

	return 0;
}

#else

static int sensor_bus_i2c_read(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len)
{
	int ret = 0;
	uint8_t cmd[4];

	if (reg != REG_NULL) {
		if (dev->hw.adr_len > 1) {
			cmd[0] = (reg >> 8) & 0xff;
			cmd[1] = reg & 0xff;
		} else {
			cmd[0] = reg;
		}
		ret = i2c_write_read(i2cmt_dev[dev->io.bus_id], dev->hw.dev_addr, cmd, dev->hw.adr_len, buf, len);
	} else {
		ret = i2c_read(i2cmt_dev[dev->io.bus_id], buf, len, dev->hw.dev_addr);
	}
	
	return ret;
}

static int sensor_bus_i2c_write(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len)
{
	int ret = 0, cnt;
	uint8_t cmd[4];
	struct i2c_msg msgs[2];

	if (reg != REG_NULL) {
		if (dev->hw.adr_len > 1) {
			cmd[0] = (reg >> 8) & 0xff;
			cmd[1] = reg & 0xff;
		} else {
			cmd[0] = reg;
		}
		cnt = 2;
		msgs[0].buf = cmd;
		msgs[0].len = dev->hw.adr_len;
		msgs[0].flags = I2C_MSG_WRITE;
		msgs[1].buf = buf;
		msgs[1].len = len;
		msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;
		if (buf == NULL) {
			cnt = 1;
			msgs[0].flags = I2C_MSG_WRITE;
		}
		ret = i2c_transfer(i2cmt_dev[dev->io.bus_id], msgs, cnt, dev->hw.dev_addr);
	} else {
		ret = i2c_write(i2cmt_dev[dev->io.bus_id], buf, len, dev->hw.dev_addr);
	}

	return ret;
}

static int sensor_bus_spi_read(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len)
{
	int ret = 0;
	uint8_t cmd[4];
	const struct spi_buf tx_bufs[] = { { .buf = cmd, .len = 1, },	};
	const struct spi_buf rx_bufs[] = { { .buf = buf, .len = len, }, };
	const struct spi_buf_set tx = {	.buffers = tx_bufs,	.count = ARRAY_SIZE(tx_bufs) };
	const struct spi_buf_set rx = {	.buffers = rx_bufs,	.count = ARRAY_SIZE(rx_bufs) };

	if (reg != REG_NULL) {
		cmd[0] = reg | (1 << 7); //rw=1
		ret = spi_transceive(spimt_dev[dev->io.bus_id], NULL, &tx, &rx);
	} else {
		ret = spi_read(spimt_dev[dev->io.bus_id], NULL, &rx);
	}

	return ret;
}

static int sensor_bus_spi_write(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len)
{
	int ret = 0;
	uint8_t cmd[4];
	const struct spi_buf tx_bufs[] = { { .buf = buf, .len = len, },	};
	const struct spi_buf_set tx = {	.buffers = tx_bufs,	.count = ARRAY_SIZE(tx_bufs) };
	const struct spi_buf tx2_bufs[] = { { .buf = cmd, .len = 1, }, { .buf = buf, .len = len, },	};
	const struct spi_buf_set tx2 = {	.buffers = tx2_bufs,	.count = ARRAY_SIZE(tx2_bufs) };

	if (reg != REG_NULL) {
		cmd[0] = reg & ~(1 << 7); //rw=0
		ret = spi_write(spimt_dev[dev->io.bus_id], NULL, &tx2);
	} else {
		ret = spi_write(spimt_dev[dev->io.bus_id], NULL, &tx);
	}

	return ret;
}

static int sensor_bus_i2c_xfer(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len, uint8_t rd)
{
	int ret = 0;
	
	if (rd) {
		ret = sensor_bus_i2c_read(dev, reg, buf, len);
	} else {
		ret = sensor_bus_i2c_write(dev, reg, buf, len);
	}
	
	return ret;
}

static int sensor_bus_spi_xfer(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len, uint8_t rd)
{
	int ret = 0;
	
	spi_cs_select(spimt_dev[dev->io.bus_id], dev->io.bus_cs);
	
	if (rd) {
		ret = sensor_bus_spi_read(dev, reg, buf, len);
	} else {
		ret = sensor_bus_spi_write(dev, reg, buf, len);
	}
	
	return ret;
}

#endif

static int sensor_bus_xfer(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len, uint8_t rd)
{
	int ret = 0;
	
	if (dev->hw.name == NULL) {
		return -1;
	}
	
	if (dev->io.bus_type == BUS_I2C) {
		ret = sensor_bus_i2c_xfer(dev, reg, buf, len, rd);
	} else if (dev->io.bus_type == BUS_SPI) {
		ret = sensor_bus_spi_xfer(dev, reg, buf, len, rd);
	}
	
	return ret;
}

int sensor_bus_read(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len)
{
	return sensor_bus_xfer(dev, reg, buf, len, 1);
}

int sensor_bus_write(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len)
{
	return sensor_bus_xfer(dev, reg, buf, len, 0);
}

static int sensor_bus_i2c_task_start(int bus_id, i2c_task_t *task, sensor_bus_cb_t cb, void *ctx)
{
	uint32_t task_id;

	// get task id
	task_id = ((task->trig.task - SPI_TASK_NUM * 2) % I2C_TASK_NUM);
	
	// add task callback
	i2c_register_callback(i2cmt_dev[bus_id], task_id, cb, ctx);
	
	// start task
	return i2c_task_start(i2cmt_dev[bus_id], task_id, task);
}

static int sensor_bus_i2c_task_stop(int bus_id, i2c_task_t *task)
{
	uint32_t task_id;
	
	// get task id
	task_id = ((task->trig.task - SPI_TASK_NUM * 2) % I2C_TASK_NUM);
	
	// remove task callback
	i2c_register_callback(i2cmt_dev[bus_id], task_id, NULL, NULL);
	
	// stop task
	return i2c_task_stop(i2cmt_dev[bus_id], task_id);
}

static int sensor_bus_spi_task_start(int bus_id, spi_task_t *task, sensor_bus_cb_t cb, void *ctx)
{
	uint32_t task_id;

	// get task id
	task_id = (task->trig.task % SPI_TASK_NUM);
	
	// add task callback
	spi_register_callback(spimt_dev[bus_id], task_id, cb, ctx);
	
	// start task
	return spi_task_start(spimt_dev[bus_id], task_id, task);
}

static int sensor_bus_spi_task_stop(int bus_id, spi_task_t *task)
{
	uint32_t task_id;
	
	// get task id
	task_id = (task->trig.task % SPI_TASK_NUM);
		
	// remove task callback
	spi_register_callback(spimt_dev[bus_id], task_id, NULL, NULL);
	
	// stop task
	return spi_task_stop(spimt_dev[bus_id], task_id);
}

int sensor_bus_task_start(const sensor_dev_t *dev, sensor_bus_cb_t cb, void *ctx)
{
	int ret = -1;
	
	if ((dev->task == NULL) || (cb == NULL)) {
		return -1;
	}
	
	if (dev->io.bus_type == BUS_I2C) {
		ret = sensor_bus_i2c_task_start(dev->io.bus_id, (i2c_task_t*)dev->task, cb, ctx);
	} else if (dev->io.bus_type == BUS_SPI) {
		ret = sensor_bus_spi_task_start(dev->io.bus_id, (spi_task_t*)dev->task, cb, ctx);
	}
	
	return ret;
}

int sensor_bus_task_wait(int enable)
{
	sensor_wait = enable;
	return 0;
}

int sensor_bus_task_stop(const sensor_dev_t *dev)
{
	int ret = -1;
	
	if (dev->task == NULL) {
		return -1;
	}

	if (dev->io.bus_type == BUS_I2C) {
		ret = sensor_bus_i2c_task_stop(dev->io.bus_id, (i2c_task_t*)dev->task);
	} else if (dev->io.bus_type == BUS_SPI) {
		ret = sensor_bus_spi_task_stop(dev->io.bus_id, (spi_task_t*)dev->task);
	}
	
	return ret;
}

static int sensor_bus_init(const struct device *dev)
{
#ifdef CONFIG_SENSOR_TASK_CFG
	uint32_t i, j;
#endif
	
	ARG_UNUSED(dev);

	/* get device */
	spimt_dev[0] = (struct device*)device_get_binding("SPIMT_0");
	if (!spimt_dev[0]) {
		//printk("[spimt0] get device failed!\n");
	}

	/* get device */
	i2cmt_dev[0] = (struct device*)device_get_binding("I2CMT_0");
	if (!i2cmt_dev[0]) {
		printk("[i2cmt0] get device failed!\n");
		return -1;
	}

	/* get device */
	i2cmt_dev[1] = (struct device*)device_get_binding("I2CMT_1");
	if (!i2cmt_dev[1]) {
		printk("[i2cmt1] get device failed!\n");
		return -1;
	}
	
#ifdef CONFIG_SENSOR_TASK_CFG
	/* init sem */
	for (i = 0; i < NUM_BUS; i ++) {
		for (j = 0; j < 2; j ++) {
			k_sem_init(&sensor_sem[i][j], 0, UINT_MAX);
		}
	}
#endif

	return 0;
}

SYS_INIT(sensor_bus_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

