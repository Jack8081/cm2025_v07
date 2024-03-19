# SPDX-License-Identifier: Apache-2.0

zephyr_sources(hotplug_manager.c)
zephyr_include_directories(hotplug_usb)
add_subdirectory_ifdef(CONFIG_USB_HOTPLUG hotplug_usb)
