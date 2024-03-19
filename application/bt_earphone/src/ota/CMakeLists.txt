# SPDX-License-Identifier: Apache-2.0

target_sources_ifdef(CONFIG_OTA_FOREGROUND app PRIVATE ota_app_foreground.c ota_tws_sppble_stream.c ota_tws_slave_stream.c ota_tws_transmit.c)
