# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

target_sources(app PRIVATE system_app_main.c)
target_sources(app PRIVATE system_app_launcher.c)
target_sources(app PRIVATE system_app_input.c)
target_sources(app PRIVATE system_app_view.c)
target_sources(app PRIVATE system_app_event.c)
target_sources(app PRIVATE system_audio_policy.c)
target_sources(app PRIVATE system_tts_policy.c)
target_sources(app PRIVATE system_event_map.c)
target_sources(app PRIVATE system_app_shell.c)
target_sources(app PRIVATE system_app_config.c)
target_sources(app PRIVATE system_key_tone.c)
target_sources(app PRIVATE system_app_poweroff.c)
target_sources(app PRIVATE system_app_poweron.c)
target_sources(app PRIVATE system_ble_advertise.c)
target_sources(app PRIVATE system_bt_ops.c)
target_sources(app PRIVATE tr_api_app.c)
zephyr_library_sources_ifdef(CONFIG_POWER_MANAGER
    system_dc5v_io_cmd.c
    system_dc5v_io_ctrl.c
    system_bat_charge.c
    system_dc5v_uart.c
    system_dc5v_status.c
)

