

#include "shell/shell.h"
#include "drivers/anc.h"
#include "anc_hal.h"
#include <media_mem.h>
#include "stream.h"
#include "ringbuff_stream.h"
#include "utils/acts_ringbuf.h"
#include "audio_system.h"
#include "soc_anc.h"
#include "drivers/audio/audio_out.h"
#include <stdlib.h>
#include "sdfs.h"

#define DATA_FRAME_SIZE 96


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

static int dump_len = 0;
static os_timer dump_timer;
static void *dump_ringbuf = NULL;
static io_stream_t dump_stream;

static int cmd_open_anc(const struct shell *shell,
			      size_t argc, char **argv)
{
	int ret = 0;
	if(argc != 2){
		SYS_LOG_ERR("usage: anc open <mode>");
		return 0;
	}

	ret = anc_dsp_open(ANC_SAMPLERATE_CONFIG);
	if(ret)
		return 0;

	anc_dsp_set_mode(atoi(argv[1]));

	return 0;
}


static int cmd_close_anc(const struct shell *shell,
			      size_t argc, char **argv)

{
	anc_dsp_close();
	return 0;
}

static void _anc_dump_data_timer_handler(os_timer *ttimer)
{
	static int cnt = 0;
	int ret = 0, i = 0;
	char data[DATA_FRAME_SIZE];

	if(dump_ringbuf ==NULL)
		return;

	for(i=0; i<5; i++){
		ret = acts_ringbuf_length(dump_ringbuf);
		if(ret < DATA_FRAME_SIZE)
			break;

		dump_len += acts_ringbuf_get(dump_ringbuf, data, DATA_FRAME_SIZE);
		printk("==============================\n");
		for(i=0; i<DATA_FRAME_SIZE; i++){
			if(i%8 == 0 && i!= 0)
				printk("\n");
			printk("%2x, ", *(data+i));
		}
		//printk("==============================\n");

	}

	cnt++;
	if(cnt == 1000){
		cnt = 0;
		SYS_LOG_INF("get %d bytes data total", dump_len);
	}

	return;
}

static int cmd_dump_data_start(const struct shell *shell,
			      size_t argc, char **argv)

{	char *dumpbuf;
	int bufsize;

	dumpbuf = media_mem_get_cache_pool(TOOL_ASQT_DUMP_BUF, AUDIO_STREAM_MUSIC);
	bufsize = media_mem_get_cache_pool_size(TOOL_ASQT_DUMP_BUF, AUDIO_STREAM_MUSIC);

	SYS_LOG_INF("buf %p, size 0x%x", dumpbuf, bufsize);

	dump_stream = ringbuff_stream_create_ext(dumpbuf, bufsize);

	dump_ringbuf = stream_get_ringbuffer(dump_stream);
	SYS_LOG_INF("ringbuf mcu addr %p", dump_ringbuf);


	dump_len = 0;
	os_timer_init(&dump_timer, _anc_dump_data_timer_handler, NULL);
	os_timer_start(&dump_timer, K_MSEC(2), K_MSEC(2));
	anc_dsp_dump_data(1, (uint32_t)dump_ringbuf, 0x7);
	return 0;
}

static int cmd_dump_data_stop(const struct shell *shell,
			      size_t argc, char **argv)

{
	os_timer_stop(&dump_timer);
	anc_dsp_dump_data(0, 0, 0);
	stream_close(dump_stream);
	stream_destroy(dump_stream);
	dump_ringbuf = NULL;
	return 0;
}

static int cmd_sample_rate_change(const struct shell *shell,
			      size_t argc, char **argv)

{
	struct device *anc_dev = NULL;
	struct device *aout_dev = NULL;
	uint32_t samplerete = 48000, dac_digctl = 0;

	anc_dev = (struct device *)device_get_binding(CONFIG_ANC_NAME);
	aout_dev = (struct device *)device_get_binding(CONFIG_AUDIO_OUT_ACTS_DEV_NAME);
	if(!aout_dev || !aout_dev)
	{
		SYS_LOG_ERR("get device failed\n");
		return -1;
	}

	if(anc_get_status(anc_dev) != ANC_STATUS_POWERON)
	{
		SYS_LOG_ERR("anc not open");
		return 0;
	}

	/*get dac samplerate*/
	audio_out_control(aout_dev, NULL, AOUT_CMD_GET_SAMPLERATE, &samplerete);
	if(samplerete == 0){
		SYS_LOG_ERR("get sample rate err");
	}

	/*get DAC_DIGCTL register value*/
	dac_digctl = sys_read32(AUDIO_DAC_REG_BASE + 0x0);

	SYS_LOG_INF("samplerate %d, dac_digctl 0x%x",samplerete, dac_digctl);

	return anc_fs_change(anc_dev, samplerete, dac_digctl);
}


static int cmd_set_gain(const struct shell *shell,
			      size_t argc, char **argv)
{
	if(argc != 5){
		SYS_LOG_INF("usage: set_gain <mode> <filter> <gain> <save>");
		return 0;
	}

	anc_dsp_set_gain(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
	return 0;
}

// static int cmd_get_gain(const struct shell *shell,
// 			      size_t argc, char **argv)
// {

// }

static int cmd_set_para(const struct shell *shell,
			      size_t argc, char **argv)
{
	int ret;
	char *data = NULL;
	int mode, filter;
	char *anc_mode_cof_map[ANC_MODE_MAX][ANC_FILTER_NUM] = {
		{ANC_FF0_COF, ANC_FB0_COF, ANC_PFB0_COF, ANC_PFF0_COF},
		{ANC_TT0_COF, NULL, NULL, NULL},
		{ANC_FF1_COF, ANC_FB1_COF, ANC_PFB0_COF, ANC_PFF0_COF},
		{ANC_FF2_COF, ANC_FB2_COF, ANC_PFB0_COF, ANC_PFF0_COF},
	};

	if(argc != 4){
		SYS_LOG_INF("usage: set_para <mode> <filter> <save>");
		return 0;
	}

	mode = atoi(argv[1]);
	filter = atoi(argv[2]);

	if(anc_mode_cof_map[mode][filter] == NULL){
		SYS_LOG_INF("null");
	}

	if(sd_fmap(anc_mode_cof_map[mode][filter], (void **)&data, &ret))
	{
		SYS_LOG_ERR("cannot find anc cfg file \"%s\"", anc_mode_cof_map[mode][filter]);
		return 0;
	}

	data += 8;
	anc_dsp_set_filter_para(atoi(argv[1]), atoi(argv[2]), data, 368, atoi(argv[3]));

	return 0;
}

static int cmd_get_para(const struct shell *shell,
			      size_t argc, char **argv)
{
	int size = 0, ret = 0, i;
	char data[512];

	if(argc != 3){
		SYS_LOG_INF("usage: get_para <mode> <filter>");
		return 0;
	}

	memset(data, 0, 512);
	ret = anc_dsp_get_filter_para(atoi(argv[1]), atoi(argv[2]), data, &size);
	if(ret == 0){
		SYS_LOG_INF("get %d bytes param", size);
		for(i=0; i<size; i++){
			if(i%8 == 0 && i!= 0)
				printk("\n");
			printk("%2x, ", *(data+i));
		}
	}

	return 0;
}

static int cmd_send_cmd(const struct shell *shell,
			      size_t argc, char **argv)
{
	if(argc != 2){
		SYS_LOG_INF("usage: send_cmd <cmd>");
		return 0;
	}

	anc_dsp_send_command(atoi(argv[1]), 0, 0);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_acts_anc,
	SHELL_CMD(open, NULL, "init and open anc", cmd_open_anc),
	SHELL_CMD(close, NULL, "deinit and close anc", cmd_close_anc),
	SHELL_CMD(dump_start, NULL, "anc dsp start dump data", cmd_dump_data_start),
	SHELL_CMD(dump_stop, NULL, "anc dsp stop dump data", cmd_dump_data_stop),
	SHELL_CMD(sr_change, NULL, "set new sample rate", cmd_sample_rate_change),
	SHELL_CMD(set_gain, NULL, "set new sample rate", cmd_set_gain),
	SHELL_CMD(get_para, NULL, "set new sample rate", cmd_get_para),
	SHELL_CMD(send_cmd, NULL, "set new sample rate", cmd_send_cmd),
	SHELL_CMD(set_para, NULL, "set new sample rate", cmd_set_para),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(anc, &sub_acts_anc, "Actions anc commands", NULL);
