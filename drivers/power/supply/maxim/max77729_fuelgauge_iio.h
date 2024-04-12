/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2020-2021 The Linux Foundation. All rights reserved.
 */

#ifndef __MAX77729_FG_IIO_H
#define __MAX77729_FG_IIO_H

#include <linux/iio/iio.h>
#include <dt-bindings/iio/qti_power_supply_iio.h>
#include <linux/qti_power_supply.h>

struct max77729_fg_iio_channels {
	const char *datasheet_name;
	int channel_num;
	enum iio_chan_type type;
	long info_mask;
};

#define MAX77729_FG_IIO_CHAN(_name, _num, _type, _mask)		\
	{						\
		.datasheet_name = _name,		\
		.channel_num = _num,			\
		.type = _type,				\
		.info_mask = _mask,			\
	},

#define MAX77729_FG_CHAN_CURRENT(_name, _num)			\
	MAX77729_FG_IIO_CHAN(_name, _num, IIO_CURRENT,		\
		BIT(IIO_CHAN_INFO_PROCESSED))

static const struct max77729_fg_iio_channels max77729_fg_iio_psy_channels[] = {
	MAX77729_FG_CHAN_CURRENT("shutdown_delay", PSY_IIO_SHUTDOWN_DELAY)
	MAX77729_FG_CHAN_CURRENT("resistance", PSY_IIO_RESISTANCE)
	MAX77729_FG_CHAN_CURRENT("resistance_id", PSY_IIO_RESISTANCE_ID)
	MAX77729_FG_CHAN_CURRENT("soc_decimal", PSY_IIO_SOC_DECIMAL)
	MAX77729_FG_CHAN_CURRENT("soc_decimal_rate", PSY_IIO_SOC_DECIMAL_RATE)
	MAX77729_FG_CHAN_CURRENT("fastcharge_mode", PSY_IIO_FASTCHARGE_MODE)
	MAX77729_FG_CHAN_CURRENT("battery_type", PSY_IIO_BATTERY_TYPE)
	MAX77729_FG_CHAN_CURRENT("soh", PSY_IIO_SOH)
	MAX77729_FG_CHAN_CURRENT("charge_ic_type", PSY_IIO_CHARGE_IC_TYPE)
};

#endif
