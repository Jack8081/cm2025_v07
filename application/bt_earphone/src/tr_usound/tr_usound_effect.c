#include "sdfs.h"

#define UPLOAD_CALL_EFX_DEFAULT "call_up.efx"

void tr_usound_mod_music_efx(int efx_mode)
{
	//struct tr_usound_app_t *tr_usound = tr_usound_get_app();
	media_player_t *player = NULL;
    void *aset_para_buf = NULL;
    int size = 0;
	player = media_player_get_current_dumpable_player();
    switch(efx_mode)
    {
        case 0:
	        sd_fmap(UPLOAD_CALL_EFX_DEFAULT, &aset_para_buf, &size);
            break;
        default:
            break;
    }
	if (!aset_para_buf || size != ASQT_PARA_BIN_SIZE)
    {
		SYS_LOG_ERR("not found");
        return;
    }

    //tr_usound->pre_music_efx = efx_mode;
#ifdef CONFIG_MOD_PEQ_PARAM_ONLY
    extern int _btcall_peq_param_file_modify(void *peq_param_base, int size);
    aset_para_buf += PEQ_PARAM_OFFST;
    _btcall_peq_param_file_modify(aset_para_buf, PEQ_PARAM_SIZE);
#else
	media_player_update_effect_param(player, aset_para_buf, ASQT_PARA_REAL_SIZE);
#endif
    //tr_usound_set_pre_music_efx_config(CFG_PRE_MUSIC_EFX, efx_mode);

	SYS_LOG_INF("buf:%p,size:%d, efx_mode:%d\n", aset_para_buf, size, efx_mode);
}

