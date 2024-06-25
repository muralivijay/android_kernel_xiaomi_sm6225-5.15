/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2020-2021 The Linux Foundation. All rights reserved.
 */

#ifndef __MAX77729_CHARGER_IIO_H
#define __MAX77729_CHARGER_IIO_H

#include <linux/iio/iio.h>
#include <dt-bindings/iio/qti_power_supply_iio.h>
#include <linux/qti_power_supply.h>
#include "max77729_charger.h"

struct max77729_charger_data_iio {
        struct device           *dev;
        struct max77729_charger_data *chg_data;
        struct max77729_dev *max77729;
        struct iio_dev  *indio_dev;
        struct iio_chan_spec    *iio_chan;
        struct iio_channel      *int_iio_chans;
};

struct max77729_charger_iio_channels {
	const char *datasheet_name;
	int channel_num;
	enum iio_chan_type type;
	long info_mask;
};

#define MAX77729_CHARGER_IIO_CHAN(_name, _num, _type, _mask)		\
	{						\
		.datasheet_name = _name,		\
		.channel_num = _num,			\
		.type = _type,				\
		.info_mask = _mask,			\
	},

#define MAX77729_CHARGER_CHAN_CURRENT(_name, _num)			\
	MAX77729_CHARGER_IIO_CHAN(_name, _num, IIO_CURRENT,		\
		BIT(IIO_CHAN_INFO_PROCESSED))

static const struct max77729_charger_iio_channels max77729_charger_iio_psy_channels[] = {
	MAX77729_CHARGER_CHAN_CURRENT("shutdown_delay", PSY_IIO_SHUTDOWN_DELAY)
	MAX77729_CHARGER_CHAN_CURRENT("battery_charging_enabled", PSY_IIO_BATTERY_CHARGING_ENABLED)
	MAX77729_CHARGER_CHAN_CURRENT("input_suspend", PSY_IIO_INPUT_SUSPEND)
	MAX77729_CHARGER_CHAN_CURRENT("usb_real_type", PSY_IIO_USB_REAL_TYPE)
	MAX77729_CHARGER_CHAN_CURRENT("charge_pd_active", PSY_IIO_PD_ACTIVE)
	MAX77729_CHARGER_CHAN_CURRENT("typec_cc_orientation", PSY_IIO_TYPEC_CC_ORIENTATION)
	MAX77729_CHARGER_CHAN_CURRENT("typec_mode", PSY_IIO_TYPEC_MODE)
	MAX77729_CHARGER_CHAN_CURRENT("charging_enabled", PSY_IIO_CHARGING_ENABLED)
        MAX77729_CHARGER_CHAN_CURRENT("charge_ic_type", PSY_IIO_CHARGE_IC_TYPE)
};

#endif
