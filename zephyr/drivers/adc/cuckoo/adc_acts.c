/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions PMU ADC implementation
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <soc_atp.h>
#include <board_cfg.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <drivers/adc.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(pmuadc0, CONFIG_ADC_LOG_LEVEL);

/***************************************************************************************************
 * PMUADC_CTL
 */
#define PMUADC_CTL_REG_CHARGEI_SET                     (27)
#define PMUADC_CTL_REG_CHARGEI_SET_MASK                      (0x1F << PMUADC_CTL_REG_CHARGEI_SET)
#define PMUADC_CTL_REG_RVALUE_SET                     (22)
#define PMUADC_CTL_REG_RVALUE_SET_MASK                      (0x1F << PMUADC_CTL_REG_RVALUE_SET)

#define PMUADC_CTL_REG_R_SEL                           BIT(21)

#define PMUADC_CTL_REG_SENSOR_CUR                     (15)
#define PMUADC_CTL_REG_SENSOR_CUR_MASK                      (0x3F << PMUADC_CTL_REG_SENSOR_CUR)

#define PMUADC_CTL_PMUADC_EN                           BIT(14)
#define PMUADC_CTL_SCANMODE_EN                         BIT(13)
#define PMUADC_CTL_TEST_SARAD                          BIT(12)

#define PMUADC_CTL_REG_IBIAS_BUF_SHIFT                        (10)
#define PMUADC_CTL_REG_IBIAS_BUF_MASK                         (0x3 << PMUADC_CTL_REG_IBIAS_BUF_SHIFT)
#define PMUADC_CTL_REG_IBIAS_BUF(x)                           ((x) << PMUADC_CTL_REG_IBIAS_BUF_SHIFT)
#define PMUADC_CTL_REG_IBIAS_ADC_SHIFT                        (8)
#define PMUADC_CTL_REG_IBIAS_ADC_MASK                         (0x3 << PMUADC_CTL_REG_IBIAS_ADC_SHIFT)
#define PMUADC_CTL_REG_IBIAS_ADC(x)                           ((x) << PMUADC_CTL_REG_IBIAS_ADC_SHIFT)

#define PMUADC_CTL_REG_LRADC3_CHEN                                BIT(7)
#define PMUADC_CTL_REG_LRADC2_CHEN                                BIT(6)
#define PMUADC_CTL_REG_LRADC1_CHEN                                BIT(5)
#define PMUADC_CTL_REG_SVCC_CHEN                                  BIT(4)
#define PMUADC_CTL_REG_SENSOR_CHEN                                BIT(3)
#define PMUADC_CTL_REG_BATV_CHEN                                  BIT(2)
#define PMUADC_CTL_REG_DC5V_CHEN                                  BIT(1)
#define PMUADC_CTL_REG_CHARGI_CHEN                                BIT(0)

/***************************************************************************************************
 * CHARGI_DATA, 10BIT ADC
 */
#define CHARGI_DATA_CHARGI_SHIFT                              (0)
#define CHARGI_DATA_CHARGI_MASK                               (0x3FF << CHARGI_DATA_CHARGI_SHIFT)

/***************************************************************************************************
 * BATADC_DATA, 10BIT ADC
 */
#define BATADC_DATA_BATV_SHIFT                                (0)
#define BATADC_DATA_BATV_MASK                                 (0x3FF << BATADC_DATA_BATV_SHIFT)

/***************************************************************************************************
 * DC5VADC_DATA, 10BIT ADC
 */
#define DC5VADC_DATA_DC5V_SHIFT                               (0)
#define DC5VADC_DATA_DC5V_MASK                                (0x3FF << DC5VADC_DATA_DC5V_SHIFT)

/***************************************************************************************************
 * SENSADC_DATA, 10BIT ADC
 */
#define SENSADC_DATA_SENSOR_SHIFT                             (0)
#define SENSADC_DATA_SENSOR_MASK                              (0x3FF << SENSADC_DATA_SENSOR_SHIFT)

/***************************************************************************************************
 * SVCCADC_DATA, 10BIT ADC
 */
#define SVCCADC_DATA_SVCC_SHIFT                               (0)
#define SVCCADC_DATA_SVCC_MASK                                (0x3FF << SVCCADC_DATA_SVCC_SHIFT)

/***************************************************************************************************
 * LRADC_DATA, 10BIT ADC
 */
#define LRADC_DATA_LRADC_SHIFT                                (0)
#define LRADC_DATA_LRADC_MASK                                 (0x3FF << LRADC_DATA_LRADC_SHIFT)

/***************************************************************************************************
 * PMUADCDIG_CTL
 */
#define PMUADCDIG_CTL_ADCFIFOSEL                              BIT(7)
#define PMUADCDIG_CTL_ADC_DRQEN                               BIT(6)
#define PMUADCDIG_CTL_ADCFIFOLV_SHIFT                         (4)
#define PMUADCDIG_CTL_ADCFIFOLV_MASK                          (0x3 << PMUADCDIG_CTL_ADCFIFOLV_SHIFT)
#define PMUADCDIG_CTL_ADCFIFOLV(x)                            ((x) << PMUADCDIG_CTL_ADCFIFOLV_SHIFT)
#define PMUADCDIG_CTL_ADCFIFOEM                               BIT(3)
#define PMUADCDIG_CTL_ADCFIFOFU                               BIT(2)
#define PMUADCDIG_CTL_ADCFIFOER                               BIT(1)
#define PMUADCDIG_CTL_ADCFIFOEN                               BIT(0)

#define PMUADC_MAX_CHANNEL_ID                                 (7) 	/* MAX valid channel ID */
#define PMUADC_WAIT_TIMEOUT_MS                                (10) /* Timeout to wait for PMU ADC sampling */


#define PMUADC_WAIT_SAMPLE_US                            (400)

#define LRADCx_PENDING(ch, n) \
			if ((ch) & PMUADC_CTL_REG_LRADC##n##_CHEN) { \
				data->mdata.v.lradc##n##_voltage = pmuadc_reg->lradc##n##_data & LRADC_DATA_LRADC_MASK; \
				data->channels &= ~(BIT(n + 4)); \
				LOG_DBG("New LRADC%d voltage: 0x%x", n, data->mdata.v.lradc##n##_voltage); \
			}

#define LRADCx_PENDING_ALL(x) \
		{ \
			LRADCx_PENDING(x, 1); \
			LRADCx_PENDING(x, 2); \
			LRADCx_PENDING(x, 3); \
		}

/*
 * @struct acts_pmu
 * @brief Actions PMU controller hardware register
 */
struct acts_pmu {
	volatile uint32_t vout_ctl0;
	volatile uint32_t vout_ctl1_s1;
	volatile uint32_t vout_ctl1_s3; 
	volatile uint32_t pmu_det;
	volatile uint32_t dcdc_vc18_ctl;
	volatile uint32_t dcdc_vd12_ctl;
	volatile uint32_t pwrgate_dig;
	volatile uint32_t pwrgate_dig_ack;
	volatile uint32_t pwrgate_ram;
	volatile uint32_t pwrgate_ram_ack;
	volatile uint32_t pmu_intmask;
	volatile uint32_t pmuadc_ctl;
	volatile uint32_t dc5vadc_data;
	volatile uint32_t batadc_data;
	volatile uint32_t sensadc_data;
	volatile uint32_t svccadc_data;
	volatile uint32_t chgiadc_data;
	volatile uint32_t lradc1_data;
	volatile uint32_t lradc2_data;
	volatile uint32_t lradc3_data;	
};

/**
 * struct pmuadc_measure_data
 * @brief The measure result data from PMU ADC sampling.
 */
union pmuadc_measure_data {
	uint16_t data[PMUADC_MAX_CHANNEL_ID + 1];
	struct {
		uint16_t charging_current;
		uint16_t dc5v_voltage;
		uint16_t battery_voltage;
		uint16_t sensor_temperature;
		uint16_t svcc_voltage;
		uint16_t lradc1_voltage;
		uint16_t lradc2_voltage;
		uint16_t lradc3_voltage;
	} v;
};

/**
 * struct pmuadc_drv_data
 * @brief The meta data which related to Actions PMU ADC.
 */
struct pmuadc_drv_data {
	uint16_t channel_bitmap;
#ifndef CONFIG_ADC_ACTS_ALWAYS_ON
	struct k_sem completion; /* ADC sample synchronization completion semaphare */
#endif
	struct k_sem lock; /* ADC read lock */
	union pmuadc_measure_data mdata; /* measuared data */
	uint16_t channels; /* active channels */
	uint16_t sample_cnt; /* sample counter */
};

/**
 * struct pmuadc_config_data
 * @brief The hardware data that related to Actions PMU ADC
 */
struct pmuadc_config_data {
	uint32_t reg_base; /* PMU ADC controller register base address */
	uint8_t clk_id; /* LRADC devclk id */
	uint8_t clk_src; /* LRADC clock source */
	uint8_t clk_div; /* LRADC clock divisor */
	uint8_t debounce; /* PMU ADC debounce as the first sample data is not correct */
	void (*irq_config)(void); /* IRQ configuration function */
};

/* @brief get the base address of PMU register */
static inline struct acts_pmu *get_pmu_reg_base(const struct device *dev)
{
	const struct pmuadc_config_data *cfg = dev->config;
	return (struct acts_pmu *)cfg->reg_base;
}

/* @brief dump pmu dac controller register */
void pmudac_dump_register(const struct device *dev)
{
	struct acts_pmu *pmuadc_reg = get_pmu_reg_base(dev);

	LOG_INF("** pmudac contoller regster **");
	LOG_INF("         BASE: %08x", (uint32_t)pmuadc_reg);
	LOG_INF("          CTL: %08x", pmuadc_reg->pmuadc_ctl);
	LOG_INF("      INTMASK: %08x", pmuadc_reg->pmu_intmask);
	LOG_INF("  CHARGE_DATA: %08x", pmuadc_reg->chgiadc_data);
	LOG_INF("     BAT_DATA: %08x", pmuadc_reg->batadc_data);
	LOG_INF("    DC5V_DATA: %08x", pmuadc_reg->dc5vadc_data);
	LOG_INF("  SENSOR_DATA: %08x", pmuadc_reg->sensadc_data);
	LOG_INF("    SVCC_DATA: %08x", pmuadc_reg->svccadc_data);
	LOG_INF("  LRADC1_DATA: %08x", pmuadc_reg->lradc1_data);
	LOG_INF("  LRADC2_DATA: %08x", pmuadc_reg->lradc2_data);
	LOG_INF("  LRADC3_DATA: %08x", pmuadc_reg->lradc3_data);
	LOG_INF(" CMU_LRADCCLK: %08x", sys_read32(CMU_LRADCCLK));
}

static s16_t adc_sensor_offset, adc_dc5v_offset, adc_bat_offset;

/* @brief PMU ADC channel lock */
static inline void pmuadc_lock(const struct device *dev)
{
	struct pmuadc_drv_data *data = dev->data;
	k_sem_take(&data->lock, K_FOREVER);
}

/* @brief PMU ADC channel unlock */
static inline void pmuadc_unlock(const struct device *dev)
{
	struct pmuadc_drv_data *data = dev->data;
	k_sem_give(&data->lock);
}

#ifndef CONFIG_ADC_ACTS_ALWAYS_ON
/* @brief wait for PMU ADC sample IRQ completion */
static inline int pmuadc_wait_for_completion(const struct device *dev)
{
	struct pmuadc_drv_data *data = dev->data;
	return k_sem_take(&data->completion, K_MSEC(PMUADC_WAIT_TIMEOUT_MS));
}

/* @brief signal that PMU ADC sample data completly */
static inline void pmuadc_complete(const struct device *dev)
{
	struct pmuadc_drv_data *data = dev->data;
	k_sem_give(&data->completion);
}
#endif

/* @brief check the buffer size */
static int check_buffer_size(const struct adc_sequence *sequence,
			     uint8_t active_channels)
{
	uint32_t needed_buffer_size;

	needed_buffer_size = active_channels * sizeof(uint16_t);

	if (sequence->buffer_size < needed_buffer_size) {
		LOG_ERR("Provided buffer is too small (%u/%u)",
				sequence->buffer_size, needed_buffer_size);
		return -ENOMEM;
	}

	return 0;
}

/* @brief validate the selected PMU ADC channels */
static int pmuadc_check_channels(const struct device *dev,
			const struct adc_sequence *sequence)
{
	struct pmuadc_drv_data *data = dev->data;
	uint32_t channels = sequence->channels;
	uint8_t i, active_channels = 0;

	if (!channels) {
		LOG_ERR("null channels");
		return -EINVAL;
	}

	for (i = 0; i <= PMUADC_MAX_CHANNEL_ID; i++) {
		if (channels & BIT(i)) {
			if (!(data->channel_bitmap & BIT(i))) {
				LOG_ERR("ADC channel@%d has not setuped yet", i);
				return -ENXIO;
			} else {
				++active_channels;
			}
		}
	}

	return check_buffer_size(sequence, active_channels);
}

/* @brief enable specified PMU ADC channels to sample data */
static int pmuadc_enable_channels(const struct device *dev, uint16_t channels)
{
	struct pmuadc_drv_data *data = dev->data;
	struct acts_pmu *pmuadc_reg = get_pmu_reg_base(dev);
	uint8_t i;
	uint16_t ctl = 0;

	data->channels = channels;
	data->sample_cnt = 0;

	LOG_INF("Active channels:0x%x", data->channels);

#ifdef CONFIG_ADC_ACTS_ALWAYS_ON
	/* If enable new channels, need to disable PMUADC and then enable */
	if (pmuadc_reg->pmuadc_ctl & PMUADC_CTL_PMUADC_EN) {
		pmuadc_reg->pmuadc_ctl &= ~PMUADC_CTL_PMUADC_EN;
		//k_busy_wait(300);
	}
#endif

	/* enable PMU ADC channles */
	for (i = 0; i <= PMUADC_MAX_CHANNEL_ID; i++) {
		if (channels & BIT(i))
			ctl |= BIT(i);
	}

	pmuadc_reg->pmuadc_ctl |= ctl;

	if(channels & BIT(PMUADC_ID_LRADC1)) {
		//lradc1 is enabled, enable lradc1 pu100k
		pmuadc_reg->pmuadc_ctl |= PMUADC_CTL_REG_R_SEL;
	}

	/* enable PMU ADC function */
	pmuadc_reg->pmuadc_ctl |= PMUADC_CTL_PMUADC_EN;

	LOG_INF("pmuadc_ctl:0x%x", pmuadc_reg->pmuadc_ctl);

	return 0;
}

/* @brief disable all PMU ADC channels */
static int pmuadc_disable_channels(const struct device *dev)
{
	struct acts_pmu *pmuadc_reg = get_pmu_reg_base(dev);

	/* disable all ADC channels */
	//pmuadc_reg->pmuadc_ctl &= ~(PMUADC_CTL_PMUADC_EN | 0xFF);
	pmuadc_reg->pmuadc_ctl &= ~(PMUADC_CTL_REG_R_SEL | PMUADC_CTL_PMUADC_EN | 0xFF);
	//k_busy_wait(300);

	return 0;
}

#ifdef CONFIG_ADC_ACTS_ALWAYS_ON
static int pmuadc_wait_sample_complete(const struct device *dev, uint8_t ch_index)
{
	uint32_t timestamp = k_cycle_get_32();
	//struct acts_pmu *pmuadc_reg = get_pmu_reg_base(dev);

	while (k_cyc_to_us_floor32(k_cycle_get_32() - timestamp)
			> PMUADC_WAIT_SAMPLE_US) {
		break;
	}

	return 0;
}
#endif

/* @brief Implementation of the ADC driver API function: adc_channel_setup. */
static int pmuadc_channel_setup(const struct device *dev,
				  const struct adc_channel_cfg *channel_cfg)
{
	struct pmuadc_drv_data *data = dev->data;
	uint8_t channel_id = channel_cfg->channel_id;

	if (channel_id > PMUADC_MAX_CHANNEL_ID) {
		LOG_ERR("Invalid channel id %d", channel_id);
		return -EINVAL;
	}

	pmuadc_lock(dev);
	if (!(data->channel_bitmap & BIT(channel_id))) {
		data->channel_bitmap |= BIT(channel_id);
		LOG_INF("Enable PMU ADC channel@%d", channel_id);
#ifdef CONFIG_ADC_ACTS_ALWAYS_ON
		pmuadc_enable_channels(dev, BIT(channel_id));
		if (channel_id == PMUADC_ID_BATV) {
			pmuadc_wait_sample_complete(dev, BIT(PMUADC_ID_BATV));
		}
		//k_busy_wait(300);
#endif
	}
	pmuadc_unlock(dev);

	return 0;
}

static u16_t pmu_adc_cal(int32_t adcval, int32_t offset)
{
	adcval = adcval + offset;
	return (u16_t)adcval;
}
/* @brief start to read the PMU ADC measure data */
static int pmuadc_start_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	struct pmuadc_drv_data *data = dev->data;
	int ret = 0;
	uint32_t channels = sequence->channels;
	uint8_t i;

#ifdef CONFIG_ADC_ACTS_ALWAYS_ON
	struct acts_pmu *pmuadc_reg = get_pmu_reg_base(dev);

	if (channels & PMUADC_CTL_REG_BATV_CHEN) {
		data->mdata.v.battery_voltage = pmuadc_reg->batadc_data & BATADC_DATA_BATV_MASK;
		data->mdata.v.battery_voltage = pmu_adc_cal(data->mdata.v.battery_voltage, adc_bat_offset);
	}

	if (channels & PMUADC_CTL_REG_CHARGI_CHEN) {
		data->mdata.v.charging_current = pmuadc_reg->chgiadc_data & CHARGI_DATA_CHARGI_MASK;
	}
	
	if (channels & PMUADC_CTL_REG_DC5V_CHEN){
		data->mdata.v.dc5v_voltage = pmuadc_reg->dc5vadc_data & DC5VADC_DATA_DC5V_MASK;
		data->mdata.v.dc5v_voltage = pmu_adc_cal(data->mdata.v.dc5v_voltage, adc_dc5v_offset);
	}

	if (channels & PMUADC_CTL_REG_SENSOR_CHEN){
		data->mdata.v.sensor_temperature = pmuadc_reg->sensadc_data & SENSADC_DATA_SENSOR_MASK;
		data->mdata.v.sensor_temperature = pmu_adc_cal(data->mdata.v.sensor_temperature, adc_sensor_offset);
	}

	if (channels & PMUADC_CTL_REG_SVCC_CHEN) {
		data->mdata.v.svcc_voltage = pmuadc_reg->svccadc_data & SVCCADC_DATA_SVCC_MASK;
	}
	
	LRADCx_PENDING_ALL(channels);

#else
	ret = pmuadc_enable_channels(dev, channels);
	if (ret)
		return ret;

	ret = pmuadc_wait_for_completion(dev);
#endif /* CONFIG_ADC_ACTS_ALWAYS_ON */

	if (!ret) {
	    uint16_t* buf = sequence->buffer;
		for (i = 0; i <= PMUADC_MAX_CHANNEL_ID; i++) {
			if (channels & BIT(i)) {
				sys_put_le16(data->mdata.data[i], (uint8_t*)buf);
				buf += 1;
			}
		}
	}

#ifndef CONFIG_ADC_ACTS_ALWAYS_ON
	pmuadc_disable_channels(dev);
#endif

	return ret;
}

/* @brief Implementation of the ADC driver API function: adc_read. */
static int pmuadc_read(const struct device *dev,
			 const struct adc_sequence *sequence)
{
	int ret;

	ret = pmuadc_check_channels(dev, sequence);
	if (ret) {
		return ret;
	}
	
#ifndef CONFIG_ADC_ACTS_ALWAYS_ON
	pmuadc_lock(dev);
#endif

	ret = pmuadc_start_read(dev, sequence);

#ifndef CONFIG_ADC_ACTS_ALWAYS_ON
	pmuadc_unlock(dev);
#endif

	return ret;
}

#ifdef CONFIG_ADC_ASYNC
/* @brief Implementation of the ADC driver API function: adc_read_sync. */
static int pmuadc_read_async(const struct device *dev,
			       const struct adc_sequence *sequence,
			       struct k_poll_signal *async)
{
	return -ENOTSUP;
}
#endif /* CONFIG_ADC_ASYNC */

static const struct adc_driver_api pmuadc_driver_api = {
	.channel_setup = pmuadc_channel_setup,
	.read          = pmuadc_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async    = pmuadc_read_async,
#endif
};


/* @brief set the LRADC clock source and divisor */
static int pmuadc_clk_source_set(const struct device *dev)
{
	const struct pmuadc_config_data *cfg = dev->config;

	sys_write32(((cfg->clk_src & 0x3) << 0), CMU_LRADCCLK);
	LOG_INF("LRADCCLK:0x%08x", sys_read32(CMU_LRADCCLK));

	return 0;
}

/* @brief ADC core current bias setting */
static int pmuadc_bias_setting(const struct device *dev)
{
	struct acts_pmu *pmuadc_reg = get_pmu_reg_base(dev);

	uint32_t reg = pmuadc_reg->pmuadc_ctl;

	reg &= ~PMUADC_CTL_REG_IBIAS_BUF_MASK;
	reg |= PMUADC_CTL_REG_IBIAS_BUF(CONFIG_PMUADC_IBIAS_BUF_SEL);

	reg &= ~PMUADC_CTL_REG_IBIAS_ADC_MASK;
	reg |= PMUADC_CTL_REG_IBIAS_ADC(CONFIG_PMUADC_IBIAS_ADC_SEL);

	pmuadc_reg->pmuadc_ctl = reg;

	LOG_INF("PMUADC_reg: 0x%x\n", pmuadc_reg->pmuadc_ctl);

	return 0;
}

static void adc_cal_init(void)
{
	unsigned int offset;
	adc_bat_offset = 0;
	adc_sensor_offset = 0;
	adc_dc5v_offset = 0;

	LOG_INF("adc cal init start!\n");

	if (!soc_atp_get_pmu_calib(4, &offset)){
		LOG_DBG("get batadc cal=0x%x\n", offset);
		if(offset & 0x10) { 
			adc_bat_offset = (offset & 0x0f)*2;
		} else {
			adc_bat_offset = (-((offset & 0x0f)*2));
		}
	}

	if (!soc_atp_get_pmu_calib(5, &offset)){ 
		LOG_DBG("get sensoradc cal=0x%x\n", offset);
		if(offset & 0x10) {
			adc_sensor_offset = -(0x20-offset)*10;
		} else {
			adc_sensor_offset = offset*10;
		}

		adc_sensor_offset = -adc_sensor_offset*1000/6201;
	}

	adc_dc5v_offset = 0;

	LOG_INF("adc:bat=%d,sensor=%d,dc5v=%d\n", adc_bat_offset, adc_sensor_offset, adc_dc5v_offset);
}


/* @brief PMU ADC initialization */
static int pmuadc_init(const struct device *dev)
{
	const struct pmuadc_config_data *cfg = dev->config;
	struct pmuadc_drv_data *data = dev->data;
	struct acts_pmu *pmuadc_reg = get_pmu_reg_base(dev);

	adc_cal_init();
	/* configure the LRADC clock source */
	pmuadc_clk_source_set(dev);

	acts_clock_peripheral_enable(cfg->clk_id);

	/* disable all ADC channels */
	pmuadc_reg->pmuadc_ctl &= ~(0xff << 0);
	pmuadc_reg->pmuadc_ctl &= ~(0x07 << 12);

	pmuadc_bias_setting(dev);

#ifndef CONFIG_ADC_ACTS_ALWAYS_ON
	k_sem_init(&data->completion, 0, 1);
#endif

	k_sem_init(&data->lock, 1, 1);

	if (cfg->irq_config) {
		cfg->irq_config();
	}
	
	return 0;
}


#ifdef CONFIG_PM_DEVICE
int adc_pm_control(const struct device *device, enum pm_device_action action)
{
	int ret;
	const struct pmuadc_config_data *cfg = device->config;

#ifdef CONFIG_ADC_ACTS_ALWAYS_ON
	static uint32_t pmuadc_ctl_bak;
	struct acts_pmu *pmuadc_reg = get_pmu_reg_base(device);
#endif

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		LOG_INF("adc wakeup\n");
		acts_clock_peripheral_enable(cfg->clk_id);
#ifdef CONFIG_ADC_ACTS_ALWAYS_ON
		pmuadc_reg->pmuadc_ctl = (pmuadc_ctl_bak & ~PMUADC_CTL_PMUADC_EN);
		LOG_DBG("%d PMUADC_CTL:0x%x", __LINE__, pmuadc_reg->pmuadc_ctl);
		//k_busy_wait(300);
		pmuadc_reg->pmuadc_ctl |= PMUADC_CTL_PMUADC_EN;
		LOG_DBG("%d PMUADC_CTL:0x%x", __LINE__, pmuadc_reg->pmuadc_ctl);
		//k_busy_wait(300);
#else
		irq_enable(IRQ_ID_LRADC);
#endif
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		LOG_INF("adc sleep\n");
#ifdef CONFIG_ADC_ACTS_ALWAYS_ON
		pmuadc_ctl_bak = pmuadc_reg->pmuadc_ctl;
		pmuadc_disable_channels(device);
		acts_clock_peripheral_disable(cfg->clk_id);
#else
		irq_disable(IRQ_ID_LRADC);
#endif
		break;
	case PM_DEVICE_ACTION_EARLY_SUSPEND:

		break;
	case PM_DEVICE_ACTION_LATE_RESUME:

		break;
	default:
		ret = -EINVAL;

	}
	return 0;
}
#else
#define adc_pm_control 	NULL
#endif

/* pmu adc driver data */
static struct pmuadc_drv_data pmuadc_drv_data0;

/* pmu adc config data */
static const struct pmuadc_config_data pmuadc_config_data0 = {
	.reg_base = PMU_REG_BASE,
	.clk_id = CLOCK_ID_LRADC,
	.clk_src = CONFIG_PMUADC_CLOCK_SOURCE,
	.clk_div = CONFIG_PMUADC_CLOCK_DIV,
	.debounce = CONFIG_PMUADC_DEBOUNCE,
	.irq_config = NULL,
};

DEVICE_DEFINE(pmuadc0, CONFIG_PMUADC_NAME, pmuadc_init, adc_pm_control,
	    &pmuadc_drv_data0, &pmuadc_config_data0,
	    PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &pmuadc_driver_api);

