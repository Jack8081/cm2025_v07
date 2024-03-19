# SPDX-License-Identifier: Apache-2.0

# USB Device
if(CONFIG_USB_DEVICE)
	zephyr_include_directories(usb_audio_sink usb_audio_source usb_audio_sourcesink)
	zephyr_include_directories(usb_cdc_acm usb_hid)
	add_subdirectory_ifdef(CONFIG_USB_AUDIO_SINK usb_audio_sink)
	add_subdirectory_ifdef(CONFIG_USB_AUDIO_SOURCE usb_audio_source)
	add_subdirectory_ifdef(CONFIG_USB_AUDIO_SOURCESINK usb_audio_sourcesink)
	add_subdirectory_ifdef(CONFIG_USB_CDC_ACM usb_cdc_acm)
	add_subdirectory_ifdef(CONFIG_USB_DEVICE_HID usb_hid)
endif()

# USB Host
if(CONFIG_USB_HOST)
	zephyr_include_directories(disk)
	add_subdirectory_ifdef(CONFIG_DISK_ACCESS_USB disk)
endif()

# Hotplug
if(CONFIG_HOTPLUG_MANAGER)
	add_subdirectory(hotplug_manager)
	zephyr_include_directories(hotplug_manager)
endif()

target_sources(app PRIVATE tr_usound_main.c)
target_sources(app PRIVATE tr_usound_event.c)
target_sources(app PRIVATE tr_usound_view.c)
target_sources(app PRIVATE tr_usound_media.c)
target_sources(app PRIVATE tr_usound_media_profile.c)
target_sources(app PRIVATE tr_usound_ctrl.c)
target_sources(app PRIVATE tr_usound_dev.c)

