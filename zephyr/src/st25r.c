// Copyright © 2023 Vouch.io LLC

#define DT_DRV_COMPAT st_st25r

#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif

#include "st25r.h"

#if defined(CONFIG_ST25R500) && DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#error "The ST25R500 RFAL driver has no I2C transport; put the st,st25r node on SPI"
#endif

LOG_MODULE_REGISTER(ST25R, CONFIG_ST25R_LOG_LEVEL);

static int st25r_init_interface(const struct device *dev)
{
	int res;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	res = st25r_spi_init(dev);
	if (res) {
		return res;
	}
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	res = st25r_i2c_init(dev);
	if (res) {
		return res;
	}
#else
#error "BUS MACRO NOT DEFINED IN DTS"
#endif

	return 0;
}

static int st25r_init(const struct device *dev)
{
	const struct st25r_device_config *cfg = dev->config;

	if (st25r_init_interface(dev)) {
	    return -EINVAL;
	}

	if (cfg->reset_gpio.port) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio) ||
		    gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE) < 0) {
			LOG_ERR("Failed to configure reset pin");
			return -EIO;
		}
	}

	if (cfg->int_gpio.port) {
		if (st25r_init_interrupt(dev) < 0) {
			LOG_ERR("Failed to initialize interrupts");
			return -EIO;
		}
	} else {
	    LOG_ERR("No interrupt pin configured");
    }

	return 0;
}

#define ST25R_SPI(inst)                                                                           \
	(.spi = SPI_DT_SPEC_INST_GET(                                                              \
		 0, SPI_OP_MODE_MASTER | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_HOLD_ON_CS | SPI_LOCK_ON, 0),)

#define ST25R_I2C(inst) (.i2c = I2C_DT_SPEC_INST_GET(inst),)

#define ST25R_DEFINE(inst)									\
	static struct st25r_data st25r_data_##inst;						\
												\
	static const struct st25r_device_config st25r_device_config_##inst = {		\
		COND_CODE_1(DT_INST_ON_BUS(inst, i2c), ST25R_I2C(inst), ())			\
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), ST25R_SPI(inst), ())			\
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, irq_gpios, { 0 }),          \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, { 0 }),      \
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, st25r_init, NULL,						\
			      &st25r_data_##inst, &st25r_device_config_##inst, POST_KERNEL,	\
			      90, NULL);			\

DT_INST_FOREACH_STATUS_OKAY(ST25R_DEFINE)
