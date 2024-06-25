/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2020-2024 The Linux Foundation. All rights reserved.
 */

#ifndef __MAX77729_IIO_H
#define __MAX77729_IIO_H

#include <linux/iio/iio.h>

// ds28e16
enum maxim_ds_ext_iio_channels {
        MAXIM_DS_AUTHEN_RESULT,
        MAXIM_DS_ROMID,
        MAXIM_DS_STATUS,
        MAXIM_DS_PAGE0_DATA,
        MAXIM_DS_CHIP_OK,
        DS_MAX_CHANNELS,
};

static const char * const maxim_ds_ext_iio_chan_name[] = {
        [MAXIM_DS_AUTHEN_RESULT] = "ds_authen_result",
        [MAXIM_DS_ROMID] = "ds_romid",
        [MAXIM_DS_STATUS] = "ds_status",
        [MAXIM_DS_PAGE0_DATA] = "ds_page0_data",
        [MAXIM_DS_CHIP_OK] = "ds_chip_ok",
};

//maxim-charger
enum maxim_chg_ext_iio_channels {
        MAXIM_CHG_PD_ACTIVE,
        MAXIM_CHG_USB_REAL_TYPE,
        MAXIM_CHG_BATTERY_CHARGING_ENABLED,
        MAXIM_CHG_IC_TYPE,
};

static const char * const maxim_chg_ext_iio_chan_name[] = {
        [MAXIM_CHG_PD_ACTIVE] = "charge_pd_active",
        [MAXIM_CHG_USB_REAL_TYPE] = "usb_real_type",
        [MAXIM_CHG_BATTERY_CHARGING_ENABLED] = "battery_charging_enabled",
        [MAXIM_CHG_IC_TYPE] = "charge_ic_type",
};

//maxim-fg
enum maxim_fg_ext_iio_channels {
        MAXIM_FG_FASTCHARGE_MODE,
	MAXIM_FG_RESISTANCE_ID,
};

static const char * const maxim_fg_ext_iio_chan_name[] = {
        [MAXIM_FG_FASTCHARGE_MODE] = "fastcharge_mode",
	[MAXIM_FG_RESISTANCE_ID] = "resistance_id",
};

//maxim-nopmi
enum maxim_nopmi_ext_iio_channels {
        MAXIM_NOPMI_CHG_MTBF_CUR,
        MAXIM_NOPMI_CHG_FFC_DISABLE,
};

static const char * const maxim_nopmi_ext_iio_chan_name[] = {
        [MAXIM_NOPMI_CHG_MTBF_CUR] = "mtbf_cur",
        [MAXIM_NOPMI_CHG_FFC_DISABLE] = "ffc_disable"
};

#endif
