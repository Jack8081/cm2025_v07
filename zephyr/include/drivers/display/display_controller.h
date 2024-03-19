/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for display controller drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DISPLAY_DISPLAY_CONTROLLER_H_
#define ZEPHYR_INCLUDE_DRIVERS_DISPLAY_DISPLAY_CONTROLLER_H_

#include <kernel.h>
#include <device.h>
#include <zephyr/types.h>
#include <drivers/cfg_drv/dev_config.h>
#include "display_graphics.h"

/**
 * @brief Display Controller Interface
 * @defgroup display_controller_interface Display Controller Interface
 * @ingroup display_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#define DISPLAY_PORT_TYPE(major, minor) (((major) << 8) | (minor))
#define DISPLAY_PORT_TYPE_MAJOR(type) (((type) >> 8) & 0xFF)
#define DISPLAY_PORT_TYPE_MINOR(type) ((type) & 0xFF)

/**
 * @brief Enumeration with possible display major port type
 *
 */
#define DISPLAY_PORT_Unknown  (0)
#define DISPLAY_PORT_CPU      BIT(0)
#define DISPLAY_PORT_RGB      BIT(1)
#define DISPLAY_PORT_SPI      BIT(2)
#define DISPLAY_PORT_DSI      BIT(3)

/**
 * @brief Enumeration with possible display cpu port type
 *
 */
#define DISPLAY_CPU_I8080  (0) /* Intel 8080 */
#define DISPLAY_CPU_M6800  (1) /* Moto 6800 */

#define DISPLAY_PORT_CPU_I8080  DISPLAY_PORT_TYPE(DISPLAY_PORT_CPU, DISPLAY_CPU_I8080)
#define DISPLAY_PORT_CPU_M6800  DISPLAY_PORT_TYPE(DISPLAY_PORT_CPU, DISPLAY_CPU_M6800)

/**
 * @brief Enumeration with possible display spi port type
 *
 */
#define DISPLAY_SPI_3WIRE_TYPE1  (0)
#define DISPLAY_SPI_3WIRE_TYPE2  (1)
#define DISPLAY_SPI_4WIRE_TYPE1  (2)
#define DISPLAY_SPI_4WIRE_TYPE2  (3)
#define DISPLAY_SPI_QUAD         (4)
#define DISPLAY_SPI_QUAD_SYNC    (5)

#define DISPLAY_PORT_SPI_3WIRE_TYPE1  DISPLAY_PORT_TYPE(DISPLAY_PORT_SPI, DISPLAY_SPI_3WIRE_TYPE1)
#define DISPLAY_PORT_SPI_3WIRE_TYPE2  DISPLAY_PORT_TYPE(DISPLAY_PORT_SPI, DISPLAY_SPI_3WIRE_TYPE2)
#define DISPLAY_PORT_SPI_4WIRE_TYPE1  DISPLAY_PORT_TYPE(DISPLAY_PORT_SPI, DISPLAY_SPI_4WIRE_TYPE1)
#define DISPLAY_PORT_SPI_4WIRE_TYPE2  DISPLAY_PORT_TYPE(DISPLAY_PORT_SPI, DISPLAY_SPI_4WIRE_TYPE2)
#define DISPLAY_PORT_SPI_QUAD         DISPLAY_PORT_TYPE(DISPLAY_PORT_SPI, DISPLAY_SPI_QUAD)
#define DISPLAY_PORT_SPI_QUAD_SYNC    DISPLAY_PORT_TYPE(DISPLAY_PORT_SPI, DISPLAY_SPI_QUAD_SYNC)

/**
 * @struct display_videoport
 * @brief Structure holding display port configuration
 *
 */
struct display_videoport {
	/* port type */
	union {
		uint16_t type;
		struct {
			uint8_t minor_type;
			uint8_t major_type;
		};
	};

	union {
		/* cpu port */
		struct {
			uint8_t cs : 1; /* chip select */
			uint8_t lsb_first : 1;
			uint8_t bus_width; /* output interface buf width */
			uint8_t pclk_low_duration;  /* mem clock low level duration */
			uint8_t pclk_high_duration; /* mem clock high level duration */
		} cpu_mode;

		/* rgb port */
		struct {
			uint8_t lsb_first : 1;
			uint8_t bus_width; /* output interface buf width */
		} rgb_mode;

		/* spi port */
		struct {
			uint8_t cs : 1; /* chip select */
			uint8_t lsb_first : 1;
			uint8_t cpol : 1; /* (clk polarity), reused as sclk output inverse here */
			uint8_t cpha : 1; /* (clk phase) */
			uint8_t dual_lane : 1; /* dual data lane enable */
			uint8_t dcp_mode : 1; /* data compat mode enable */
			uint8_t rx_dummy_cycles; /* dummy cycles between read command and data */
			uint8_t rx_delay_chain_ns; /* delay chain in ns */
		} spi_mode;
	};
};

/* display timing flags */
enum display_flags {
	/* tearing effect sync flag */
	DISPLAY_FLAGS_TE_LOW      = BIT(0),
	DISPLAY_FLAGS_TE_HIGH     = BIT(1),
	/* horizontal sync flag */
	DISPLAY_FLAGS_HSYNC_LOW   = BIT(2),
	DISPLAY_FLAGS_HSYNC_HIGH  = BIT(3),
	/* vertical sync flag */
	DISPLAY_FLAGS_VSYNC_LOW   = BIT(4),
	DISPLAY_FLAGS_VSYNC_HIGH  = BIT(5),

	/* data enable flag */
	DISPLAY_FLAGS_DE_LOW   = BIT(6),
	DISPLAY_FLAGS_DE_HIGH  = BIT(7),
	/* drive data on pos. edge (rising edge) */
	DISPLAY_FLAGS_PIXDATA_POSEDGE = BIT(8),
	/* drive data on neg. edge (falling edge) */
	DISPLAY_FLAGS_PIXDATA_NEGEDGE = BIT(9),
	/* drive sync on pos. edge (rising edge) */
	DISPLAY_FLAGS_SYNC_POSEDGE    = BIT(10),
	/* drive sync on neg. edge (falling edge) */
	DISPLAY_FLAGS_SYNC_NEGEDGE    = BIT(11),
};

/**
 * @struct display_videomode
 * @brief Structure holding display mode configuration
 *
 */
struct display_videomode {
	uint32_t pixel_format;  /* see enum display_pixel_format */

	/* timing */
	uint32_t pixel_clk;     /* pixel clock in KHz */
	uint16_t refresh_rate;  /* refresh rate in Hz */

	uint16_t hactive;       /* hor. active video */
	uint16_t hfront_porch;  /* hor. front porch */
	uint16_t hback_porch;   /* hor. back porch */
	uint16_t hsync_len;     /* hor. sync pulse len */

	uint16_t vactive;       /* ver. active video */
	uint16_t vfront_porch;  /* ver. front porch */
	uint16_t vback_porch;   /* ver. back porch */
	uint16_t vsync_len;     /* ver. sync pulse len */

	/* timing flags */
	uint16_t flags; /* display flags */
};

/**
 * @enum display_controller_ctrl_type
 * @brief Enumeration with possible display controller ctrl commands
 *
 */
enum display_controller_ctrl_type {
	DISPLAY_CONTROLLER_CTRL_COMPLETE_CB = 0,
	DISPLAY_CONTROLLER_CTRL_VSYNC_CB, /* vsync or te callback */
	DISPLAY_CONTROLLER_CTRL_HOTPLUG_CB,
};

/**
 * @typedef display_controller_complete_t
 * @brief Callback API executed when display controller complete one frame
 *
 */
typedef void (*display_controller_complete_t)(void *arg);

/**
 * @typedef display_controller_vsync_t
 * @brief Callback API executed when display controller receive vsync or te signal
 *
 */
typedef void (*display_controller_vsync_t)(void *arg, uint32_t timestamp);

/**
 * @typedef display_controller_vsync_t
 * @brief Callback API executed when display controller receive vsync or te signal
 *
 */
typedef void (*display_controller_hotplug_t)(void *arg, int connected);

/**
 * @enum display_controller_source_type
 * @brief Enumeration with possible display controller intput source
 *
 */
enum display_controller_source_type {
	DISPLAY_CONTROLLER_SOURCE_MCU = 1,    /* MCU write */
	DISPLAY_CONTROLLER_SOURCE_ENGINE, /* display engine transfer */
	DISPLAY_CONTROLLER_SOURCE_DMA,    /* DMA transfer */
};

/**
 * @typedef display_controller_control_api
 * @brief Callback API to control display controller device
 * See display_controller_control() for argument description
 */
typedef int (*display_controller_control_api)(
		const struct device *dev, int cmd, void *arg1, void *arg2);

/**
 * @typedef display_controller_enable_api
 * @brief Callback API to enable display controller
 * See display_controller_enable() for argument description
 */
typedef int (*display_controller_enable_api)(const struct device *dev,
		const struct display_videoport *port);

/**
 * @typedef display_controller_disable_api
 * @brief Callback API to disable display controller
 * See display_controller_disable() for argument description
 */
typedef int (*display_controller_disable_api)(const struct device *dev);

/**
 * @typedef display_controller_set_mode_api
 * @brief Callback API to set display mode
 * See display_controller_set_mode() for argument description
 */
typedef int (*display_controller_set_mode_api)(const struct device *dev,
		const struct display_videomode *mode);

/**
 * @typedef display_controller_read_config
 * @brief Callback API to read display configuration
 * See display_controller_read_config() for argument description
 */
typedef int (*display_controller_read_config_api)(const struct device *dev,
		int32_t cmd, void *buf, uint32_t len);

/**
 * @typedef display_controller_write_config_api
 * @brief Callback API to write display configuration
 * See display_controller_write_config() for argument description
 */
typedef int (*display_controller_write_config_api)(const struct device *dev,
		int32_t cmd, const void *buf, uint32_t len);

/**
 * @typedef display_controller_read_pixels_api
 * @brief Callback API to read display image when source is MCU write
 * See display_controller_read_pixels() for argument description
 */
typedef int (*display_controller_read_pixels_api)(const struct device *dev,
		int32_t cmd, const struct display_buffer_descriptor *desc, void *buf);

/**
 * @typedef display_controller_write_pixels_api
 * @brief Callback API to write display image when source is MCU write
 * See display_controller_write_pixels() for argument description
 */
typedef int (*display_controller_write_pixels_api)(const struct device *dev,
		int32_t cmd, const struct display_buffer_descriptor *desc, const void *buf);

/**
 * @typedef display_controller_set_source_api
 * @brief Callback API to set display image input source device
 * See display_controller_set_source() for argument description
 */
typedef int (*display_controller_set_source_api)(const struct device *dev,
		enum display_controller_source_type source_type, const struct device *source_dev);

/**
 * @brief Display Controller driver API
 * API which a display controller driver should expose
 */
struct display_controller_driver_api {
	display_controller_control_api control;
	display_controller_enable_api enable;
	display_controller_disable_api disable;
	display_controller_set_mode_api set_mode;
	display_controller_set_source_api set_source;
	display_controller_read_config_api read_config;
	display_controller_write_config_api write_config;
	display_controller_read_pixels_api read_pixels;
	display_controller_write_pixels_api write_pixels;
};

/**
 * @brief Control display controller
 *
 * @param dev Pointer to device structure
 * @param cmd Control command
 * @param arg1 Control command argument 1
 * @param arg2 Control command argument 2
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_controller_control(const struct device *dev,
		int cmd, void *arg1, void *arg2)
{
	struct display_controller_driver_api *api =
		(struct display_controller_driver_api *)dev->api;

	return api->control(dev, cmd, arg1, arg2);
}

/**
 * @brief Turn display controller on
 *
 * @param dev Pointer to device structure
 * @param port Pointer to display_videoport structure, which must be static defined,
 *           since display controller will still refer it until next api call.
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_controller_enable(const struct device *dev,
		const struct display_videoport *port)
{
	struct display_controller_driver_api *api =
		(struct display_controller_driver_api *)dev->api;

	return api->enable(dev, port);
}

/**
 * @brief Turn display controller off
 *
 * @param dev Pointer to device structure
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_controller_disable(const struct device *dev)
{
	struct display_controller_driver_api *api =
		(struct display_controller_driver_api *)dev->api;

	return api->disable(dev);
}

/**
 * @brief Set display video mode
 *
 * @param dev Pointer to device structure
 * @param mode Pointer to display_mode_set structure, which must be static defined,
 *           since display controller will still refer it until next api call.
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_controller_set_mode(const struct device *dev,
		const struct display_videomode *mode)
{
	struct display_controller_driver_api *api =
		(struct display_controller_driver_api *)dev->api;

	return api->set_mode(dev, mode);
}

/**
 * @brief Set display controller image input source
 *
 * This routine configs the image input source device, which should be a display
 * accelerator 2D device.
 *
 * @param dev Pointer to device structure
 * @param source_type source type
 * @param source_dev Pointer to structure of source device
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_controller_set_source(const struct device *dev,
		enum display_controller_source_type source_type, const struct device *source_dev)
{
	struct display_controller_driver_api *api =
		(struct display_controller_driver_api *)dev->api;

	return api->set_source(dev, source_type, source_dev);
}

/**
 * @brief Read configuration data from display
 *
 * @param dev Pointer to device structure
 * @param cmd Read reg command, ignored for negative value
 * @param buf Pointer to data array
 * @param len Length of data to read
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_controller_read_config(const struct device *dev,
		int32_t cmd, void *buf, uint32_t len)
{
	struct display_controller_driver_api *api =
		(struct display_controller_driver_api *)dev->api;

	return api->read_config(dev, cmd, buf, len);
}

/**
 * @brief Write configuration data to display
 *
 * @param dev Pointer to device structure
 * @param cmd Write reg command, ignored for negative value
 * @param buf Pointer to data array
 * @param len Length of data to write
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_controller_write_config(const struct device *dev,
		int32_t cmd, const void *buf, uint32_t len)
{
	struct display_controller_driver_api *api =
		(struct display_controller_driver_api *)dev->api;

	return api->write_config(dev, cmd, buf, len);
}

/**
 * @brief Read image data from display
 *
 * @param dev Pointer to device structure
 * @param cmd Read pixel command, ignored for negative value
 * @param desc Pointer to a structure describing the buffer layout
 * @param buf Pointer to buffer array
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_controller_read_pixels(const struct device *dev,
		int32_t cmd, const struct display_buffer_descriptor *desc, void *buf)
{
	struct display_controller_driver_api *api =
		(struct display_controller_driver_api *)dev->api;

	return api->read_pixels(dev, cmd, desc, buf);
}

/**
 * @brief Write image data to display
 *
 * This routine may return immediately without waiting complete, So the caller must make
 * sure the previous write_pixels() has completed by listening to complete() callback
 * registered by control() with cmd DISPLAY_CONTROLLER_CTRL_COMPLETE_CB.
 *
 * @param dev Pointer to device structure
 * @param cmd Write pixel command, ignored for negative value
 * @param desc Pointer to a structure describing the buffer layout
 * @param buf Pointer to buffer array
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_controller_write_pixels(const struct device *dev,
		int32_t cmd, const struct display_buffer_descriptor *desc, const void *buf)
{
	struct display_controller_driver_api *api =
		(struct display_controller_driver_api *)dev->api;

	return api->write_pixels(dev, cmd, desc, buf);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_DISPLAY_DISPLAY_CONTROLLER_H_ */
