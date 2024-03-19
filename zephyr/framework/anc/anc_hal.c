
#include <assert.h>
#include <errno.h>
#include <devicetree.h>
#include <string.h>
#include <mem_manager.h>
#include "anc_inner.h"
#include "drivers/anc.h"
#include "os_common_api.h"
#include "drivers/audio/audio_in.h"
#include "drivers/audio/audio_out.h"
#include "soc_regs.h"
#include "anc_hal.h"
#include "soc_anc.h"
#include "property_manager.h"
#include "sdfs.h"

#define ANC_CFG_DATA_SIZE	(368)
#define TEST_TOOL_DATA_OFFSET 	(376)
#define TEST_TOOL_DATA_SIZE	(128)

#define ANC_DSP_IMAGE "anc_dsp.dsp"
#define ANC_CFG_FF_FILE  "anccfgff.bin"
#define ANC_CFG_FB_FILE  "anccfgfb.bin"
#define ANC_CFG_TT_FILE  "anccfgtt.bin"

#define ANC_FB0_COF 	"ancFB0.cof"
#define ANC_FF0_COF 	"ancFF0.cof"
#define ANC_FB1_COF 	"ancFB1.cof"
#define ANC_FF1_COF 	"ancFF1.cof"
#define ANC_FB2_COF 	"ancFB2.cof"
#define ANC_FF2_COF 	"ancFF2.cof"
#define ANC_FB3_COF 	"ancFB3.cof"
#define ANC_FF3_COF 	"ancFF3.cof"
#define ANC_FB4_COF 	"ancFB4.cof"
#define ANC_FF4_COF 	"ancFF4.cof"

#define ANC_PFB0_COF 	"ancPFB0.cof"
#define ANC_PFF0_COF 	"ancPFF0.cof"

#define ANC_TT0_COF 	"ancTT0.cof"
#define ANC_TT1_COF	"ancTT1.cof"

static char cur_mode = -1;
static u8_t cur_anc_mode = 0xff;

static char *anc_mode_cof_map[ANC_MODE_MAX][ANC_FILTER_NUM] = {
	{ANC_FF0_COF, ANC_FB0_COF, ANC_PFB0_COF, ANC_PFF0_COF},
	{ANC_TT0_COF, NULL, NULL, NULL},
	{ANC_FF1_COF, ANC_FB1_COF, ANC_PFB0_COF, ANC_PFF0_COF},
	{ANC_FF2_COF, ANC_FB2_COF, ANC_PFB0_COF, ANC_PFF0_COF},
	{NULL, NULL, NULL, NULL},
};

static anc_info_t g_anc_info;

// static void anc_shutdown_work_func(struct k_work *work);
// K_DELAYED_WORK_DEFINE(anc_shutdown_work, anc_shutdown_work_func);
extern void anc_gain_convert(int gain, int *buf);

// static void anc_shutdown_work_func(struct k_work *work)
// {
// 	struct device *anc_dev = NULL;
// 	struct device *ain_dev = NULL;
// 	struct device *aout_dev = NULL;
// 	adc_anc_ctl_t adc_anc_ctl = {0};
// 	dac_anc_ctl_t dac_anc_ctl = {0};

// 	anc_dev = (struct device *)device_get_binding(CONFIG_ANC_NAME);
// 	ain_dev = (struct device *)device_get_binding(CONFIG_AUDIO_IN_ACTS_DEV_NAME);
// 	aout_dev = (struct device *)device_get_binding(CONFIG_AUDIO_OUT_ACTS_DEV_NAME);
// 	if(!aout_dev || !ain_dev || !anc_dev)
// 	{
// 		SYS_LOG_ERR("get device failed\n");
// 		return;
// 	}

// 	anc_poweroff(anc_dev);

// 	anc_release_image(anc_dev);

// 	adc_anc_ctl.is_open_anc = 0;
// 	audio_in_control(ain_dev, NULL, AIN_CMD_ANC_CONTROL, &adc_anc_ctl);
// 	dac_anc_ctl.is_open_anc = 0;
// 	audio_out_control(aout_dev, NULL, AOUT_CMD_ANC_CONTROL, &dac_anc_ctl);

// 	cur_scene = -1;
// 	cur_mode = -1;
// 	anc_filter_mode = 0xff;
// 	return;
// }

int anc_dsp_open(anc_info_t *init_info)
{
	int ret = 0, i;
	uint32_t *anc_cfg = NULL;
	uint32_t *data = NULL;
	struct device *anc_dev = NULL;
	struct device *ain_dev = NULL;
	struct device *aout_dev = NULL;
	struct anc_imageinfo image;
	adc_anc_ctl_t adc_anc_ctl = {0};
	dac_anc_ctl_t dac_anc_ctl = {0};

	anc_dev = (struct device *)device_get_binding(CONFIG_ANC_NAME);
	ain_dev = (struct device *)device_get_binding(CONFIG_AUDIO_IN_ACTS_DEV_NAME);
	aout_dev = (struct device *)device_get_binding(CONFIG_AUDIO_OUT_ACTS_DEV_NAME);
	if(!aout_dev || !ain_dev || !anc_dev)
	{
		SYS_LOG_ERR("get device failed\n");
		return -ANC_ERR_OTHER;
	}

	if(anc_get_status(anc_dev) != ANC_STATUS_POWEROFF)
		return -ANC_ERR_OPEN;

	memset(&image, 0, sizeof(image));

	if(sd_fmap(ANC_DSP_IMAGE, (void **)&image.ptr, (int *)&image.size))
	{
		SYS_LOG_ERR("cannot find anc image \"%s\"", ANC_DSP_IMAGE);
		return -ANC_ERR_OTHER;
	}

	image.name = ANC_DSP_IMAGE;
	ret = anc_request_image(anc_dev, &image);
	if(ret)
	{
		SYS_LOG_ERR("anc request image err\n");
		return -ANC_ERR_OTHER;
	}

	adc_anc_ctl.is_open_anc = 1;
	audio_in_control(ain_dev, NULL, AIN_CMD_ANC_CONTROL, &adc_anc_ctl);
	if(init_info->aal){
		ret = 1;
		audio_in_control(ain_dev, NULL, AIN_CMD_ANC_AAL_CONTROL, &ret);
	}
	dac_anc_ctl.is_open_anc = 1;
	audio_out_control(aout_dev, NULL, AOUT_CMD_ANC_CONTROL, &dac_anc_ctl);

	ret = anc_poweron(anc_dev, init_info->rate);
	if(ret)
	{
		SYS_LOG_ERR("anc power on err\n");
		goto err;
	}

	data = app_mem_malloc(ANC_CFG_DATA_SIZE);
	if(data == NULL){
		SYS_LOG_ERR("no mem");
		goto err;
	}

	data[0] = init_info->ffmic;
	data[1] = init_info->fbmic;
	data[2] = init_info->speak;

	SYS_LOG_INF("ff %d, fb %d, speak %d, rate %d", data[0], data[1], data[2], init_info->rate);
	if(anc_send_command(anc_dev, ANC_COMMAND_POWERON, data, 12))
	{
		SYS_LOG_ERR("config err");
		app_mem_free(data);
		goto err;
	}

	cur_anc_mode = 0xff;
	for(i=0; i<ANC_FILTER_NUM; i++){
		if(anc_mode_cof_map[init_info->mode][i]){
			ret = property_get(anc_mode_cof_map[init_info->mode][i], (char*)data, ANC_CFG_DATA_SIZE);
			//SYS_LOG_INF("property %s, size %d", anc_mode_cof_map[init_info->mode][i], ret);
			if(ret == ANC_CFG_DATA_SIZE){
				SYS_LOG_INF("get %s from vram.", anc_mode_cof_map[init_info->mode][i]);
				//anc_cfg = data;
			}
			else{
				SYS_LOG_INF("get %s from sdfs.", anc_mode_cof_map[init_info->mode][i]);
				if(sd_fmap(anc_mode_cof_map[init_info->mode][i], (void **)&anc_cfg, &ret))
				{
					SYS_LOG_ERR("Not find %s from sdfs!", anc_mode_cof_map[init_info->mode][i]);
					continue;
				}

				anc_cfg = anc_cfg + 2;
                memcpy(data, anc_cfg, ANC_CFG_DATA_SIZE);
			}

            if((init_info->gain != 0) && (init_info->mode == ANC_MODE_ANCON)){
                if((i == ANC_FILTER_FF) || (i == ANC_FILTER_FB))
                    anc_gain_convert(init_info->gain, (int *)data);
            }

			cur_anc_mode = ((anct_data_t *)anc_cfg)->bMode;
			if(anc_send_command(anc_dev, ANC_COMMAND_ANCTDATA, data, ANC_CFG_DATA_SIZE))
			{
				app_mem_free(data);
				SYS_LOG_ERR("send anc cmd err\n");
				goto err;
			}
		}
	}

	g_anc_info = *init_info;
	app_mem_free(data);

	SYS_LOG_INF("---anc power on success");
	return ANC_OK;

err:
	SYS_LOG_ERR("---anc power on err");
	adc_anc_ctl.is_open_anc = 0;
	audio_in_control(ain_dev, NULL, AIN_CMD_ANC_CONTROL, &adc_anc_ctl);
	dac_anc_ctl.is_open_anc = 0;
	audio_out_control(aout_dev, NULL, AOUT_CMD_ANC_CONTROL, &dac_anc_ctl);
	if(init_info->aal){
		ret = 0;
		audio_in_control(ain_dev, NULL, AIN_CMD_ANC_AAL_CONTROL, &ret);
	}
	anc_poweroff(anc_dev);
	anc_release_image(anc_dev);
	return -ANC_ERR_OTHER;
}

int anc_dsp_close(void)
{
	int ret;
	struct device *anc_dev = NULL;
	struct device *ain_dev = NULL;
	struct device *aout_dev = NULL;
	adc_anc_ctl_t adc_anc_ctl = {0};
	dac_anc_ctl_t dac_anc_ctl = {0};

	anc_dev = (struct device *)device_get_binding(CONFIG_ANC_NAME);
	ain_dev = (struct device *)device_get_binding(CONFIG_AUDIO_IN_ACTS_DEV_NAME);
	aout_dev = (struct device *)device_get_binding(CONFIG_AUDIO_OUT_ACTS_DEV_NAME);
	if(!aout_dev || !ain_dev || !anc_dev)
	{
		SYS_LOG_ERR("get device failed\n");
		return -ANC_ERR_OTHER;
	}

	if(anc_get_status(anc_dev) == ANC_STATUS_POWEROFF)
	{
		SYS_LOG_ERR("anc already close");
		return ANC_OK;
	}

	anc_send_command(anc_dev, ANC_COMMAND_POWEROFF, NULL, 0);

	/*wait dsp close*/
	os_sleep(10);

	anc_poweroff(anc_dev);

	anc_release_image(anc_dev);

	dac_anc_ctl.is_open_anc = 0;
	audio_out_control(aout_dev, NULL, AOUT_CMD_ANC_CONTROL, &dac_anc_ctl);

	adc_anc_ctl.is_open_anc = 0;
	audio_in_control(ain_dev, NULL, AIN_CMD_ANC_CONTROL, &adc_anc_ctl);
	if(g_anc_info.aal){
		ret = 0;
		audio_in_control(ain_dev, NULL, AIN_CMD_ANC_AAL_CONTROL, &ret);
	}

	memset(&g_anc_info, 0, sizeof(anc_info_t));

	return ANC_OK;
}

int anc_dsp_reopen(anc_info_t *init_info)
{
	struct device *anc_dev = NULL;
	anc_dev = (struct device *)device_get_binding(CONFIG_ANC_NAME);
	if(!anc_dev)
	{
		SYS_LOG_ERR("get device failed\n");
		return -ANC_ERR_OTHER;
	}

    /*close anc dsp before open*/
	anc_send_command(anc_dev, ANC_COMMAND_POWEROFF, NULL, 0);

	/*wait dsp close*/
	os_sleep(10);
	anc_poweroff(anc_dev);
	anc_release_image(anc_dev);

    return anc_dsp_open(init_info);
}

int anc_dsp_set_mode(anc_mode_e mode)
{
	int ret = 0, i;
	uint32_t *data = NULL;
	uint32_t *anc_cfg = NULL;
	struct device *anc_dev = NULL;

	SYS_LOG_INF("mode %d", mode);

	if(mode == cur_mode){
		SYS_LOG_INF("cur mode is you need");
		return 0;
	}

	if(mode < 0 || mode >= ANC_MODE_MAX){
		SYS_LOG_ERR("unknow mode");
		return -1;
	}

	anc_dev = (struct device *)device_get_binding(CONFIG_ANC_NAME);
	if(anc_dev == NULL){
		SYS_LOG_ERR("get device failed\n");
		return -1;
	}

	if(anc_get_status(anc_dev) != ANC_STATUS_POWERON){
		SYS_LOG_ERR("anc dsp off");
		return -1;
	}

	data = app_mem_malloc(ANC_CFG_DATA_SIZE);
	if(data == NULL){
		SYS_LOG_ERR("malloc err");
		return -1;
	}

	for(i=0; i<ANC_FILTER_NUM; i++){
		if(anc_mode_cof_map[mode][i]){
			ret = property_get(anc_mode_cof_map[mode][i], (char*)data, ANC_CFG_DATA_SIZE);
			SYS_LOG_INF("property %s, size %d", anc_mode_cof_map[mode][i], ret);
			if(ret == ANC_CFG_DATA_SIZE){
				SYS_LOG_INF("get property %s", anc_mode_cof_map[mode][i]);
				anc_cfg = data;
			}
			else{
				SYS_LOG_INF("get file %s", anc_mode_cof_map[mode][i]);
				if(sd_fmap(anc_mode_cof_map[mode][i], (void **)&anc_cfg, &ret))
				{
					SYS_LOG_ERR("cannot find anc cfg file \"%s\"", anc_mode_cof_map[mode][i]);
					continue;
				}

				anc_cfg = anc_cfg + 2;
			}

			// anc_filter_mode = ((anct_data_t *)anc_cfg)->bMode;
			if(anc_send_command(anc_dev, ANC_COMMAND_ANCTDATA, anc_cfg, ANC_CFG_DATA_SIZE))
			{
				app_mem_free(data);
				SYS_LOG_ERR("config anc err\n");
				return -1;
			}
		}
	}

	app_mem_free(data);
	cur_mode = mode;

	return 0;
}

int anc_dsp_get_mode(void)
{
	return g_anc_info.mode;
}

int anc_dsp_set_gain(int mode, int filter, int gain, bool save)
{
	int ret = 0;
	char *data = NULL, *sdfs_data = NULL;
	struct device *anc_dev = NULL;

	anc_dev = (struct device *)device_get_binding(CONFIG_ANC_NAME);
	if(anc_dev == NULL)
	{
		SYS_LOG_ERR("get anc device failed\n");
		return -ANC_ERR_OTHER;
	}

	if(anc_get_status(anc_dev) != ANC_STATUS_POWERON){
		SYS_LOG_ERR("anc dsp off");
		return -ANC_ERR_OFF;
	}

	if(mode != g_anc_info.mode){
		SYS_LOG_ERR("set mode %d, but cur mode %d", mode, g_anc_info.mode);
		return -ANC_ERR_OTHER;
	}

	if(anc_mode_cof_map[mode][filter] == NULL){
		SYS_LOG_ERR("null");
		return -ANC_ERR_OTHER;
	}

	data = app_mem_malloc(ANC_CFG_DATA_SIZE);
	if(data == NULL){
		SYS_LOG_ERR("malloc err");
		return -ANC_ERR_OTHER;
	}

    ret = property_get(anc_mode_cof_map[mode][filter], (char*)data, ANC_CFG_DATA_SIZE);
    SYS_LOG_INF("property %s, size %d", anc_mode_cof_map[mode][filter], ret);
    if(ret == ANC_CFG_DATA_SIZE){
        SYS_LOG_INF("get property %s", anc_mode_cof_map[mode][filter]);
    }
    else{
	    if(!sd_fmap(anc_mode_cof_map[mode][filter], (void **)&sdfs_data, &ret)){
		    SYS_LOG_INF("get file %s", anc_mode_cof_map[mode][filter]);
		    memcpy(data, sdfs_data+8, ANC_CFG_DATA_SIZE);
	    }
	    else{
		    SYS_LOG_ERR("no param found");
		    app_mem_free(data);
		    return -ANC_ERR_OTHER;
	    }
    }
	anc_gain_convert(gain, (int *)data);
	ret = anc_send_command(anc_dev, ANC_COMMAND_ANCTDATA, data, ANC_CFG_DATA_SIZE);

	if(save){
		ret = property_set_factory(anc_mode_cof_map[mode][filter], (char *)data, ANC_CFG_DATA_SIZE);
		if(ret){
			SYS_LOG_ERR("set property %s err", anc_mode_cof_map[mode][filter]);
		}
		SYS_LOG_INF("set property %s", anc_mode_cof_map[mode][filter]);
	}

	app_mem_free(data);

	return ret;
}

int anc_dsp_get_gain(int mode, int filter, int *gain)
{
	return 0;
}


int anc_dsp_set_filter_para(int mode, int filter, char *para, int size, bool save)
{
	int ret = 0;
	struct device *anc_dev = NULL;

	if(mode != g_anc_info.mode){
		SYS_LOG_ERR("set mode %d, but cur mode %d", mode, g_anc_info.mode);
		return -ANC_ERR_OTHER;
	}

	if(anc_mode_cof_map[mode][filter] == NULL){
		SYS_LOG_ERR("null");
		return -ANC_ERR_OTHER;
	}

	if(size != ANC_CFG_DATA_SIZE){
		SYS_LOG_ERR("para size not correct, %d", size);
		return -ANC_ERR_OTHER;
	}

	anc_dev = (struct device *)device_get_binding(CONFIG_ANC_NAME);
	if(anc_dev == NULL)
	{
		SYS_LOG_ERR("get anc device failed\n");
		return -ANC_ERR_OTHER;
	}

	if(anc_get_status(anc_dev) != ANC_STATUS_POWERON){
		SYS_LOG_ERR("anc dsp off");
		return -ANC_ERR_OFF;
	}

	ret = anc_send_command(anc_dev, ANC_COMMAND_ANCTDATA, para, ANC_CFG_DATA_SIZE);
	if(ret){
		return -ANC_ERR_OTHER;
	}

	if(save){
		SYS_LOG_INF("set property %s", anc_mode_cof_map[mode][filter]);
		ret = property_set_factory(anc_mode_cof_map[mode][filter], para, ANC_CFG_DATA_SIZE);
		if(ret){
			SYS_LOG_ERR("set property %s err", anc_mode_cof_map[mode][filter]);
			return -ANC_ERR_OTHER;
		}
	}

	return 0;
}

int anc_dsp_get_filter_para(int mode, int filter, char *para, int *size)
{
	int ret = 0;
	char *sdfs_data = NULL;

	if(anc_mode_cof_map[mode][filter] == NULL){
		SYS_LOG_ERR("null");
		return -ANC_ERR_OTHER;
	}

	if(!sd_fmap(anc_mode_cof_map[mode][filter], (void **)&sdfs_data, &ret)){
		SYS_LOG_INF("get file %s", anc_mode_cof_map[mode][filter]);
		/*copy data head*/
		memcpy(para, sdfs_data+8, 8);
		memcpy(para+8, sdfs_data+TEST_TOOL_DATA_OFFSET, TEST_TOOL_DATA_SIZE);
	}
	else{
		SYS_LOG_ERR("no param found");
		return -ANC_ERR_OTHER;
	}

	*size = TEST_TOOL_DATA_SIZE + 8;

	return ANC_OK;
}

int anc_dsp_dump_data(int start, uint32_t ringbuf_addr, uint32_t channels)
{
	uint32_t data[2];
	struct device *anc_dev = NULL;
	struct acts_ringbuf *ringbuf = NULL;

	anc_dev = (struct device *)device_get_binding(CONFIG_ANC_NAME);
	if(anc_dev == NULL)
	{
		SYS_LOG_ERR("get anc device failed\n");
		return -ANC_ERR_OTHER;
	}

	if(anc_get_status(anc_dev) != ANC_STATUS_POWERON)
	{
		SYS_LOG_ERR("anc not open");
		return -ANC_ERR_OTHER;
	}


	if(start){
		ringbuf = (struct acts_ringbuf *)ringbuf_addr;
		ringbuf->dsp_ptr = mcu_to_anc_address(ringbuf->cpu_ptr, 0);
		data[0] = mcu_to_anc_address(ringbuf_addr, 0);
		data[1] = channels;
		SYS_LOG_INF("addr 0x%x, 0x%x", data[0], ringbuf->dsp_ptr);
		if(anc_send_command(anc_dev, ANC_COMMAND_DUMPSTART, data, 8))
			return -ANC_ERR_OTHER;
	}
	else{
		if(anc_send_command(anc_dev, ANC_COMMAND_DUMPSTOP, NULL, 0))
			return -ANC_ERR_OTHER;
	}

	return ANC_OK;
}

int anc_dsp_send_command(int cmd, char *data, int size)
{
	struct device *anc_dev = NULL;

	anc_dev = (struct device *)device_get_binding(CONFIG_ANC_NAME);
	if(anc_dev == NULL)
	{
		SYS_LOG_ERR("get anc device failed\n");
		return -ANC_ERR_OTHER;
	}

	if(anc_get_status(anc_dev) != ANC_STATUS_POWERON){
		SYS_LOG_ERR("anc dsp off");
		return -ANC_ERR_OFF;
	}

	if(cmd == ANC_COMMAND_ANCTDATA){
		if(((anct_data_t *)data)->bRate != g_anc_info.rate){
			SYS_LOG_ERR("sample rate not match");
			return -ANC_ERR_RATE;
		}

		if((cur_anc_mode != 0xff) && (((anct_data_t *)data)->bMode != cur_anc_mode)){
			SYS_LOG_ERR("filter mode change, anc dsp should be reset, %d, %d", cur_anc_mode, ((anct_data_t*)data)->bMode);
			return -ANC_ERR_MODE;
		}

		cur_anc_mode = ((anct_data_t *)data)->bMode;
	}

	if(anc_send_command(anc_dev, cmd, data, size))
		return -ANC_ERR_OTHER;

	return ANC_OK;
}

void anc_dsp_get_info(anc_info_t *info)
{
	*info = g_anc_info;
}



