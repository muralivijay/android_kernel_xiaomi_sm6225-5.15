/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2020-2021 The Linux Foundation. All rights reserved.
 */

#ifndef __MAX77729_CHARGER_IIO_H
#define __MAX77729_CHARGER_IIO_H

#include <linux/iio/iio.h>
#include <dt-bindings/iio/qti_power_supply_iio.h>
#include <linux/qti_power_supply.h>

#if 0 //Enable it when not use nopmi_chg
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
	MAX77729_CHARGER_CHAN_CURRENT("pd_active", PSY_IIO_PD_ACTIVE)
	MAX77729_CHARGER_CHAN_CURRENT("typec_cc_orientation", PSY_IIO_TYPEC_CC_ORIENTATION)
	MAX77729_CHARGER_CHAN_CURRENT("typec_mode", PSY_IIO_TYPEC_MODE)
	MAX77729_CHARGER_CHAN_CURRENT("charging_enabled", PSY_IIO_CHARGING_ENABLED)
};
#endif

enum fg_ext_iio_channels {
	FG_FASTCHARGE_MODE,
	FG_RESISTANCE_ID,
};

static const char * const fg_ext_iio_chan_name[] = {
	[FG_FASTCHARGE_MODE] = "fastcharge_mode",
	[FG_RESISTANCE_ID] = "resistance_id",
};

enum max77729_chg_ext_iio_channels {
	MAX77729_CHG_MTBF_CUR,
	MAX77729_CHG_USB_REAL_TYPE,
};

static const char * const max77729_chg_ext_iio_chan_name[] = {
	[MAX77729_CHG_MTBF_CUR] = "mtbf_cur",
	[MAX77729_CHG_USB_REAL_TYPE] = "usb_real_type",
};

enum ds_ext_iio_channels {
	DS_CHIP_OK,
};

static const char * const ds_ext_iio_chan_name[] = {
	[DS_CHIP_OK] = "ds_chip_ok",
};

enum main_iio_channels {
	MAIN_BATTERY_CHARGING_ENABLED,
};

static const char * const main_iio_chan_name[] = {
	[MAIN_BATTERY_CHARGING_ENABLED] = "battery_charge_enabled",
};

#endif
