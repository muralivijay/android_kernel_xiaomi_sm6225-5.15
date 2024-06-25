/*
 *  max77729_charger_iio.c
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define pr_fmt(fmt)	"[MAX77729-charger-iio] %s: " fmt, __func__

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/of.h>
#include <linux/qti_power_supply.h>
#include <linux/usb/typec/maxim/max77729_usbc.h>
#include "max77729_charger_iio.h"
#include <linux/iio/consumer.h>
#include <dt-bindings/iio/qti_power_supply_iio.h>

#define MAIN_ICL_MIN			650
#define MAIN_CHG_SUSPEND_VOTER	"MAIN_CHG_SUSPEND_VOTER"
#define MAIN_CHG_ENABLE_VOTER	"MAIN_CHG_ENABLE_VOTER"
#define MAIN_CHG_AICL_VOTER	"MAIN_CHG_AICL_VOTER"

extern struct max77729_charger_data *charger;
extern struct max77729_dev *max77729;
/*maxim glabal usbc struct for battery and usb psy status*/
extern struct max77729_usbc_platform_data *g_usbc_data;

int max77729_charger_iio_read_raw(struct iio_dev *indio_dev,
		struct iio_chan_spec const *chan, int *val1,
		int *val2, long mask)
{
	struct max77729_charger_data_iio *charger = iio_priv(indio_dev);
	struct max77729_charger_data *chg_data = charger->chg_data;
	int rc = 0;

	*val1 = 0;

	switch (chan->channel) {
	case PSY_IIO_SHUTDOWN_DELAY:
		*val1 = chg_data->shutdown_delay;
		break;
	case PSY_IIO_BATTERY_CHARGING_ENABLED:
		*val1 =!(get_client_vote_locked(chg_data->usb_icl_votable, MAIN_CHG_ENABLE_VOTER) == MAIN_ICL_MIN);
		break;
	case PSY_IIO_INPUT_SUSPEND:
		*val1 = (get_client_vote_locked(chg_data->usb_icl_votable, MAIN_CHG_SUSPEND_VOTER) == 0);
		break;
	case PSY_IIO_USB_REAL_TYPE:
		if(chg_data->pd_active)
			*val1 = POWER_SUPPLY_TYPE_USB_PD;
		else
			*val1 = chg_data->real_type;
		/* pr_debug("get REAL_TYPE %d\n", val->intval); */
		break;
	case PSY_IIO_PD_ACTIVE:
		*val1 = chg_data->pd_active;
		/* pr_debug("get PD_ACTIVE %d\n", charger->pd_active); */
		break;
	case PSY_IIO_TYPEC_CC_ORIENTATION:
		if (g_usbc_data)
			*val1 = g_usbc_data->cc_pin_status;
		else
			*val1 = 0;
		break;
	case PSY_IIO_TYPEC_MODE:
		*val1 = QTI_POWER_SUPPLY_TYPEC_NONE;
		if (g_usbc_data) {
			switch(g_usbc_data->cc_data->ccistat) {
				case NOT_IN_UFP_MODE:
					*val1 = QTI_POWER_SUPPLY_TYPEC_SINK;
					break;
				case CCI_1_5A:
					*val1 = QTI_POWER_SUPPLY_TYPEC_SOURCE_MEDIUM;
					break;
				case CCI_3_0A:
					*val1 = QTI_POWER_SUPPLY_TYPEC_SOURCE_HIGH;
					break;
				case CCI_500mA:
					*val1 = QTI_POWER_SUPPLY_TYPEC_SOURCE_DEFAULT;
				default:
					break;
			}
			if (g_usbc_data->plug_attach_done){
				if (g_usbc_data->acc_type == 1){

					*val1 = QTI_POWER_SUPPLY_TYPEC_SINK_AUDIO_ADAPTER;
				}
			} else {

				*val1 = QTI_POWER_SUPPLY_TYPEC_NONE;
			}
		}
		break;
	case PSY_IIO_CHARGING_ENABLED:
		*val1 = (chg_data->charge_mode == SEC_BAT_CHG_MODE_CHARGING?1 : 0);
		break;
	case PSY_IIO_CHARGE_IC_TYPE:
		*val1 = NOPMI_CHARGER_IC_MAXIM;
		break;
	default:
		pr_info("Unsupported max77729_charger IIO chan %d\n", chan->channel);
		rc = -EINVAL;
		break;
	}

	if (rc < 0) {
		pr_err("Couldn't read IIO channel %d, rc = %d\n",
			chan->channel, rc);
		return rc;
	}

	return IIO_VAL_INT;
}

int max77729_charger_iio_write_raw(struct iio_dev *indio_dev,
		struct iio_chan_spec const *chan, int val1,
		int val2, long mask)
{
	struct max77729_charger_data_iio *charger = iio_priv(indio_dev);
        struct max77729_charger_data *chg_data = charger->chg_data;
	int rc = 0;
	static int last_shutdown_delay;

	switch (chan->channel) {
	case PSY_IIO_SHUTDOWN_DELAY:
		if (last_shutdown_delay != val1) {
			chg_data->shutdown_delay = val1;
			if (!chg_data->psy_batt)
				chg_data->psy_batt = power_supply_get_by_name("battery");
			if (chg_data->psy_batt)
				power_supply_changed(chg_data->psy_batt);
		}
		last_shutdown_delay = chg_data->shutdown_delay;
		break;
	case PSY_IIO_BATTERY_CHARGING_ENABLED:
		max77729_charger_unlock(chg_data);
		vote(chg_data->usb_icl_votable, MAIN_CHG_ENABLE_VOTER, !val1, MAIN_ICL_MIN);
		vote(chg_data->mainfcc_votable, MAIN_CHG_ENABLE_VOTER, !val1, 200);
		if (val1){
			max77729_set_topoff_current(chg_data, 500);
			max77729_set_topoff_time(chg_data, 1);
		} else {
			max77729_set_topoff_time(chg_data, 30);
			max77729_set_topoff_current(chg_data, 150);
		}
		break;
	case PSY_IIO_INPUT_SUSPEND:
		pr_err("%s: Set input suspend prop, value:%d\n",__func__,val1);
		vote(chg_data->usb_icl_votable, MAIN_CHG_SUSPEND_VOTER, !!val1, 0);
		if (val1) {
			vote(chg_data->chgctrl_votable, MAIN_CHG_SUSPEND_VOTER, true, SEC_BAT_CHG_MODE_BUCK_OFF);
		} else {
			vote(chg_data->chgctrl_votable, MAIN_CHG_SUSPEND_VOTER, false, SEC_BAT_CHG_MODE_BUCK_OFF);
			vote(chg_data->usb_icl_votable, MAIN_CHG_AICL_VOTER, false, 0);
		}
		break;
	case PSY_IIO_USB_REAL_TYPE:
		if (chg_data->real_type != val1){
			chg_data->real_type = val1;
			schedule_delayed_work(&chg_data->notify_work, msecs_to_jiffies(300));
			/* pr_debug("max77729_usb_set_property REAL_TYPE:%d\n", charger->real_type); */
		}
		break;
	case PSY_IIO_PD_ACTIVE:
		/* if (charger->pd_active != val->intval) { */
			chg_data->pd_active = val1;
			schedule_delayed_work(&chg_data->notify_work, msecs_to_jiffies(300));
			pr_debug("max77729_usb_set_property PD_ACTIVE:%d\n", chg_data->pd_active);
			if (chg_data->pd_active == 2){
				val1 = 1;
			} else {
				val1 = 0;
			}
			max77729_set_fast_charge_mode(chg_data, chg_data->pd_active);
		/* } */
		break;
	case PSY_IIO_CHARGING_ENABLED:
		chg_data->charge_mode =(val1 ? SEC_BAT_CHG_MODE_CHARGING: SEC_BAT_CHG_MODE_CHARGING_OFF);
		chg_data->misalign_cnt = 0;
		/* max77729_chg_set_mode_state(charger, charger->charge_mode); */
		vote(chg_data->chgctrl_votable, "charger-enable", true, chg_data->charge_mode);
	default:
		pr_err("Unsupported max77729_charger IIO chan %d\n", chan->channel);
		rc = -EINVAL;
		break;
	}
	if (rc < 0)
		pr_err("Couldn't write IIO channel %d, rc = %d\n",
			chan->channel, rc);
	return rc;
}

static int max77729_charger_iio_of_xlate(struct iio_dev *indio_dev,
				const struct of_phandle_args *iiospec)
{
	struct max77729_charger_data_iio *charger = iio_priv(indio_dev);
	struct iio_chan_spec *iio_chan = charger->iio_chan;
	int i;

	for (i = 0; i < ARRAY_SIZE(max77729_charger_iio_psy_channels);
					i++, iio_chan++)
		if (iio_chan->channel == iiospec->args[0])
			return i;

	return -EINVAL;
}

static const struct iio_info max77729_charger_iio_info = {
	.read_raw	= max77729_charger_iio_read_raw,
	.write_raw	= max77729_charger_iio_write_raw,
	.of_xlate	= max77729_charger_iio_of_xlate,
};

static int max77729_charger_iio_probe(struct platform_device *pdev)
{
	struct max77729_charger_data_iio *charger_iio;
	struct iio_dev *indio_dev = NULL;
        struct iio_chan_spec *chan = NULL;
        int num_iio_channels = ARRAY_SIZE(max77729_charger_iio_psy_channels);
        int rc = 0, i = 0;

	if (!charger) {
		pr_err("charger data not found!. defer probe\n");
			return -EPROBE_DEFER;
	}

	if (!max77729) {
		pr_err("Failed to get max77729_dev from parent. defer probe\n");
			return -EPROBE_DEFER;
	}

        pr_info("max77729_charger_init_iio start\n");

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*charger_iio));
	if (!indio_dev)
		return -ENOMEM;

	charger_iio = iio_priv(indio_dev);
	charger_iio->indio_dev = indio_dev;
	charger_iio->dev = &pdev->dev;
	charger_iio->chg_data = charger;
	charger_iio->max77729 = max77729;

	platform_set_drvdata(pdev, charger_iio);

        charger_iio->iio_chan = devm_kcalloc(charger_iio->dev, num_iio_channels,
                                sizeof(*charger_iio->iio_chan), GFP_KERNEL);
        if (!charger_iio->iio_chan)
                return -ENOMEM;

        charger_iio->int_iio_chans = devm_kcalloc(charger_iio->dev,
                                num_iio_channels,
                                sizeof(*charger_iio->int_iio_chans),
                                GFP_KERNEL);

        if (!charger_iio->int_iio_chans)
                return -ENOMEM;

	indio_dev->name = charger_iio->dev->of_node->name;
	indio_dev->dev.parent = charger_iio->dev;
	indio_dev->dev.of_node = charger_iio->dev->of_node;
	indio_dev->info = &max77729_charger_iio_info;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = charger_iio->iio_chan;
	indio_dev->num_channels = num_iio_channels;

        for (i = 0; i < num_iio_channels; i++) {
                charger_iio->int_iio_chans[i].indio_dev = indio_dev;
                chan = &charger_iio->iio_chan[i];
                charger_iio->int_iio_chans[i].channel = chan;
                chan->address = i;
                chan->channel = max77729_charger_iio_psy_channels[i].channel_num;
                chan->type = max77729_charger_iio_psy_channels[i].type;
                chan->datasheet_name =
                        max77729_charger_iio_psy_channels[i].datasheet_name;
                chan->extend_name =
                        max77729_charger_iio_psy_channels[i].datasheet_name;
                chan->info_mask_separate =
                        max77729_charger_iio_psy_channels[i].info_mask;
        }

	rc = devm_iio_device_register(charger_iio->dev, indio_dev);
	if (rc) {
		pr_err("Failed to register MAX77729_charger IIO device, rc=%d\n", rc);
		return -ENODEV;
	}

	if (rc < 0)
		pr_err("max77729_chg_test failed rc=%d\n", rc);

        pr_info("Successfully loaded max77729-charger iio driver");
	return 0;
}

static const struct of_device_id max77729_charger_iio_of_match[] = {
    { .compatible = "maxim,max77729-charger-iio", },
    {}
};
MODULE_DEVICE_TABLE(of, max77729_charger_iio_of_match);

static struct platform_driver max77729_charger_iio_driver = {
    .driver = {
        .name = "max77729-charger-iio",
	.owner = THIS_MODULE,
        .of_match_table = max77729_charger_iio_of_match,
    },
    .probe = max77729_charger_iio_probe,
};

static int __init max77729_charger_iio_init(void)
{
         pr_info("init max77729_charger_iio_driver\n");
        return platform_driver_register(&max77729_charger_iio_driver);
}
late_initcall_sync(max77729_charger_iio_init);

static void __exit max77729_charger_iio_exit(void)
{
        platform_driver_unregister(&max77729_charger_iio_driver);
}
module_exit(max77729_charger_iio_exit);

MODULE_AUTHOR("muralivijay@github");
MODULE_DESCRIPTION("Max77729 charger iio channel driver");
MODULE_LICENSE("GPL");
