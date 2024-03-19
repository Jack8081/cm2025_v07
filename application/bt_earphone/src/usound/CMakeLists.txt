# SPDX-License-Identifier: Apache-2.0

# USB Device
if(CONFIG_USB_DEVICE)
	zephyr_include_directories(usb_audio_sourcesink)
	zephyr_include_directories(usb_hid)
	add_subdirectory_ifdef(CONFIG_USB_AUDIO_SOURCESINK usb_audio_sourcesink)
	add_subdirectory_ifdef(CONFIG_USB_DEVICE_HID usb_hid)
endif()

# USB Host
if(CONFIG_USB_HOST)
	zephyr_include_directories(disk)
	add_subdirectory_ifdef(CONFIG_DISK_ACCESS_USB disk)
endif()

target_sources(app PRIVATE usound_main.c)
target_sources(app PRIVATE usound_event.c)
target_sources(app PRIVATE usound_view.c)
target_sources(app PRIVATE usound_media.c)
target_sources(app PRIVATE usound_ctrl.c)
