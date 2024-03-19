

#ifndef _ANC_HAL_H_
#define _ANC_HAL_H_


#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
	ANC_MODE_ANCON,
	ANC_MODE_TRANSPARENT,
	ANC_MODE_WIND,
	ANC_MODE_LEISURE,
	ANC_MODE_NORMAL,
	ANC_MODE_MAX,
}anc_mode_e;

typedef enum{
	ANC_FILTER_FF,
	ANC_FILTER_FB,
	ANC_FILTER_PFF,
	ANC_FILTER_PFB,
	ANC_FILTER_NUM,
}anc_filter_e;

typedef enum{
	ANC_SAMPLERATE_LOW,
	ANC_SAMPLERATE_HIGH,
	ANC_SAMPLERATE_CONFIG,
}anc_samplerate_e;

typedef enum {
	ANC_COMMAND_NONE,

	/*anc power on*/
	ANC_COMMAND_POWERON,
	/*this command type is use in ANCT test*/
	ANC_COMMAND_ANCTDATA,
	/*this command is used to notify anc dsp to start dump data, see function anc_dsp_dump_data()*/
	ANC_COMMAND_DUMPSTART,
	/*this command is used to notify anc dsp to stop dump data, see function anc_dsp_dump_data()*/
	ANC_COMMAND_DUMPSTOP,
	/*this command is used to notify anc dsp that the adc samplerate has change, it is invoked by audio driver*/
	ANC_COMMAND_SRCHANGE,
	/*this command is used to notify anc dsp to shutdown*/
	ANC_COMMAND_POWEROFF,
	/*this command is used to notify anc dsp to close ff and fb channel*/
	ANC_COMMAND_OFF_HY,
	/*this command is used to notify anc dsp to close ff channel*/
	ANC_COMMAND_OFF_FF,
	/*this command is used to notify anc dsp to close fb channel*/
	ANC_COMMAND_OFF_FB,
	/*this command is used to set ff, fb, play ff, play fb params, which are different between scenes*/
	ANC_COMMAND_SET_PARAM,
	/*this command is used to set which adc channel that ff channel use*/
	ANC_COMMAND_SET_FFMIC,
	/*this command is used to set which adc channel that fb channel use*/
	ANC_COMMAND_SET_FBMIC,
	/*this command is used to notify anc dsp to open ff and fb channel*/
	ANC_COMMAND_ON_HY,

	ANC_COMMAND_SET_GAIN,
	ANC_COMMAND_TEST_GAIN,
	ANC_COMMAND_TEST_PARAM,
	ANC_COMMAND_ANCTDAT1,
	ANC_COMMAND_ANCTDATA2,

	ANC_COMMAND_MAX,
}anc_command_e;

typedef enum{
	ANC_OK,
	ANC_ERR_OPEN = 1,
	ANC_ERR_OFF,
	ANC_ERR_MODE,
	ANC_ERR_RATE,

	ANC_ERR_OTHER,
}anc_error_e;

typedef struct
{
	u8_t bFlag;//[1]是FF，[2]是FB ，[3]是Music_FF，[4]Music_FB [5]EQ--通透
	u8_t bFilterTotal;//各模块滤波器个数，对应上面的数组顺序
     	u8_t bRate;//0:192k, 1:384k
     	u8_t bMode;//0:FF, 1:FB, 2:FF+FB, 3:FF+FF, 0:通透，这个表示当前ANC的工作模式设置
     	u8_t bAncSwitch; //1:ON    0:OFF
     	u8_t bModeEnable; //1:ON   0:OFF
     	u8_t bRev[2];//通讯传输字节对齐
     	u8_t reserve[360];
}anct_data_t;

typedef struct {
	u8_t mode;	/*anc init mode ,see @anc_mode_e*/
	u8_t rate;	/*anc sample rate 0:192k, 1:384k*/
	u8_t ffmic;	/*ffmic, 1:adc0, 2:adc1, 4:adc2*/
	u8_t fbmic;	/*fbmic, 1:adc0, 2:adc1, 4:adc2*/
	u8_t speak;	/*speak channel, 0:left, 1:right*/
	u8_t aal;	/*aal control, 0:disable, 1:enable*/
	u8_t status;	/*anc status, 0:anc off, 1:anc on*/
    int gain;  /*anc gain*/
}anc_info_t;

/**
 * @brief open anc dsp
 * @return 0 if success open,otherwise return none zero
 */
int anc_dsp_open(anc_info_t *init_info);

/**
 * @brief close anc dsp
 * @return 0 if success open,otherwise return none zero
 */
int anc_dsp_close(void);

/**
 * @brief reopen anc dsp
 * @return 0 if success open,otherwise return none zero
 */
int anc_dsp_reopen(anc_info_t *init_info);


/**
 * @brief set anc mode
 * @param mode anc mode .see @anc_mode_e
 * @return 0 success, -1 failed
 */
int anc_dsp_set_mode(anc_mode_e mode);

/**
 * @brief get anc mode
 * @return current anc mode, see @anc_mode_e
 */
int anc_dsp_get_mode(void);

/**
 * @brief set gain
 * @param mode anc mode .see @anc_mode_e
 * @param filter anc filter, see @anc_filter_e
 * @param gain filter gain, 10 is add 1 db, -10 is minus 1db
 * @param save whether save gain to nvram
 * @return 0 success, -1 failed
 */
int anc_dsp_set_gain(int mode, int filter, int gain, bool save);

/**
 * @brief get gain
 * @param mode anc mode .see @anc_mode_e
 * @param filter anc filter, see @anc_filter_e
 * @param gain[out] filter gain, 10 is add 1 db, -10 is minus 1db
 * @return 0 success, -1 failed
 */
int anc_dsp_get_gain(int mode, int filter, int *gain);

/**
 * @brief set filter param
 * @param mode anc mode .see @anc_mode_e
 * @param filter anc filter, see @anc_filter_e
 * @param para[in] filter param
 * @param size param size, is 368 bytes as define
 * @param save whether save param to nvram
 * @return 0 success, -1 failed
 */
int anc_dsp_set_filter_para(int mode, int filter, char *para, int size, bool save);

/**
 * @brief get filter param
 * @param mode anc mode .see @anc_mode_e
 * @param filter anc filter, see @anc_filter_e
 * @param para[out] filter param
 * @param size param size
 * @return 0 success, -1 failed
 */
int anc_dsp_get_filter_para(int mode, int filter, char *para, int *size);

/**
 * @brief get one frame pcm data frome anc dsp
 * @param start 1:start dump data; 0:stop dump data
 * @param ringbuf_addr address of ringbuf that dsp write data to
 * @param channels channels to dump
 * @return 0 if success,-1 if failed
 */
int anc_dsp_dump_data(int start, uint32_t ringbuf_addr, uint32_t channels);

/**
 * @brief send cmd to anc dsp
 * @param cmd anc command, see anc_command_e
 * @param data command data
 * @param size command data size
 * @return 0 success, -1 failed, -2 parameter sample rate not match
 */
int anc_dsp_send_command(int cmd, char *data, int size);

/**
 * @brief get anc info
 */
void anc_dsp_get_info(anc_info_t *info);


#ifdef __cplusplus
}
#endif

#endif
