
#ifndef __CONFIG_AL_H__
#define __CONFIG_AL_H__


#define CFG_ID_BTMUSIC_MULTIDAE_CUSTOM_AL   (0xF0)
#define CFG_ID_BT_MUSIC_DAE_AL              (0x60)
#define CFG_SIZE_BT_MUSIC_DAE_AL            (1024)

#define CFG_ID_BT_CALL_DAE_AL               (0x61)
#define CFG_SIZE_BT_CALL_DAE_AL             (512)

#define CFG_ID_BT_CALL_QUALITY_AL           (0x62)
#define CFG_SIZE_BT_CALL_QUALITY_AL         (296)

#define CFG_ID_LEAUDIO_CALL_DAE_AL          (0x63)
#define CFG_SIZE_LEAUDIO_CALL_DAE_AL        (CFG_SIZE_BT_CALL_DAE_AL)

#define CFG_ID_ANC_AL                       (0x80)

typedef struct  // <"自定义音效设置">
{
    cfg_uint8             dae_param[CFG_SIZE_BT_MUSIC_DAE_AL];
    cfg_uint8             Enable_DAE;      // <"启用数字音效", CFG_TYPE_BOOL>
    cfg_uint8             Test_Volume;     // <"测试音量", 0 ~ 16, slide_bar>

} CFG_Type_Multi_DAE_t;

#endif  // __CONFIG_AL_H__


