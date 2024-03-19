subdir-ccflags-y += -I${ZEPHYR_BASE}/samples/bt_box/src/include
subdir-ccflags-y += -I${ZEPHYR_BASE}/samples/bt_box/src/tool/
subdir-ccflags-y += -I${ZEPHYR_BASE}/samples/bt_box/src/btmusic/
subdir-ccflags-y += -I${ZEPHYR_BASE}/samples/bt_box/src/btcall/
subdir-ccflags-y += -I${ZEPHYR_BASE}/samples/bt_box/src/ota/
subdir-ccflags-y += -I${ZEPHYR_BASE}/samples/bt_box/src/le_audio/
subdir-ccflags-y += -I${ZEPHYR_BASE}/samples/bt_box/src/audio_input/
subdir-ccflags-y += -I${ZEPHYR_BASE}/samples/bt_box/src/tr_usound/
#subdir-ccflags-y += -I${ZEPHYR_BASE}/samples/bt_box/src/usound/

ifdef CONFIG_OTA_RECOVERY_APP
obj-y += ota_recovery/
else
obj-y += main/

obj-$(CONFIG_BT_MUSIC_APP) += btmusic/
obj-$(CONFIG_BT_CALL_APP) += btcall/
obj-$(CONFIG_TWS_APP)  += tws/
obj-$(CONFIG_OTA_APP) += ota/
obj-$(CONFIG_TOOL) += tool/
obj-$(CONFIG_LE_AUDIO_APP) += le_audio/
obj-$(CONFIG_AUDIO_INPUT_APP) += audio_input/
obj-$(CONFIG_TR_USOUND_APP) += tr_usound/

obj-$(CONFIG_CARD_READER_APP) += card_reader/
obj-$(CONFIG_CHARGER_APP) += charger/
obj-$(CONFIG_AP_USER_COMM) += user_comm/
obj-$(CONFIG_GMA_APP) += gma/
obj-$(CONFIG_TME_APP) += tme/
obj-$(CONFIG_NMA_APP) += netease/
obj-$(CONFIG_TUYA_APP) += tuya/
obj-$(CONFIG_USOUND_APP) += usound/

endif
