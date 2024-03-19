# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_USB_DEVICE_STACK OR CONFIG_USB_HOST_STACK)

set(ZEPHYR_CURRENT_LIBRARY acts_framework_usb)

add_library(acts_framework_usb INTERFACE)

zephyr_include_directories(
	.
	include
)

target_include_directories(acts_framework_usb INTERFACE
	include
)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USB_DEVICE_STACK usb_device.c)

zephyr_library_sources_ifdef(CONFIG_USB_HOST_STACK usb_host.c)


if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/class/CMakeLists.txt)
    add_subdirectory_ifdef(CONFIG_USB_DEVICE_STACK class)
    add_subdirectory_ifdef(CONFIG_USB_HOST_STACK host)
else()
    zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR})
    if(CONFIG_MAGIC_TUNE)
        zephyr_library_import(usb_s ${CMAKE_CURRENT_LIST_DIR}/libusb_s.a)
    else()
        zephyr_library_import(usb ${CMAKE_CURRENT_LIST_DIR}/libusb.a)
    endif()
endif()

#
#  Xear Library
#
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/xear/lib/CMakeLists.txt)
    add_subdirectory(xear/lib)
else()
    zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR})
    zephyr_library_import(xear ${CMAKE_CURRENT_LIST_DIR}/libxear.a)
endif()

zephyr_library_sources_ifdef(CONFIG_XEAR_AUTHENTICATION usb_custom_handler.c)

zephyr_library_sources_ifdef(CONFIG_XEAR_AUTHENTICATION usb_custom_hid_info.c)
	

zephyr_library_sources_ifdef(CONFIG_XEAR_LED_CTRL xear/xear_led_ctrl.c)


zephyr_library_link_libraries(acts_framework_usb)

target_link_libraries(acts_framework_usb INTERFACE zephyr_interface)

endif() 
