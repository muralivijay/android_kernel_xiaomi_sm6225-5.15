
/*
 *  max77729_fuelgauge_iio.c
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define pr_fmt(fmt)	"[MAX77729-fg-iio] %s: " fmt, __func__

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/of.h>
#include <linux/qti_power_supply.h>
#include "max77729_fuelgauge_iio.h"
#include <linux/iio/consumer.h>
#include <dt-bindings/iio/qti_power_supply_iio.h>

extern struct max77729_fuelgauge_data *fuelgauge;
extern struct max77729_dev *max77729;

static int max77729_fg_iio_read_raw(struct iio_dev *indio_dev,
		struct iio_chan_spec const *chan, int *val1,
		int *val2, long mask)
{
	struct max77729_fuelgauge_data_iio *fuelgauge = iio_priv(indio_dev);
	struct max77729_fuelgauge_data *fg_data = fuelgauge->fg_data;
	struct max77729_dev *max77729 = fuelgauge->max77729;
	int rc = 0;

	*val1 = 0;

	switch (chan->channel) {
	case PSY_IIO_SHUTDOWN_DELAY:
		*val1 = fg_data->shutdown_delay;
		break;
	case PSY_IIO_RESISTANCE:
		*val1 = 0;
		break;
	case PSY_IIO_RESISTANCE_ID:
		if (fg_data->battery_data->battery_id == MAXIM_BATTERY_VENDOR_UNKNOWN) {
		   fg_data->battery_data->battery_id = fgauge_get_battery_id(max77729);
		}
		*val1 = fg_data->battery_data->battery_id;
		break;
	case PSY_IIO_SOC_DECIMAL:
		*val1 = max77729_fg_get_soc_decimal(fg_data);
		break;
	case PSY_IIO_SOC_DECIMAL_RATE:
		*val1 = max77729_fg_get_soc_decimal_rate(fg_data);
		break;
	case PSY_IIO_FASTCHARGE_MODE:
		*val1 = fg_data->is_fastcharge;
		break;
	case PSY_IIO_BATTERY_TYPE:
		switch (fgauge_get_battery_id(max77729)) {
			case MAXIM_BATTERY_VENDOR_NVT:
				*val1 = 3;		//"M376-NVT-5000mAh";
				break;
			case MAXIM_BATTERY_VENDOR_GY:
				*val1 = 1;		//"M376-GuanYu-5000mAh";
				break;
			case MAXIM_BATTERY_VENDOR_XWD:
				*val1 = 2;		//"M376-Sunwoda-5000mAh";
				break;
			default:
				*val1 = 0;		//"M376-unknown-5000mAh";
				break;
		}
		break;

	case PSY_IIO_SOH:
		*val1 = max77729_fg_read_SoH(fg_data);
		break;
	default:
		pr_info("Unsupported max77729_fg IIO chan %d\n", chan->channel);
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

static int max77729_fg_iio_write_raw(struct iio_dev *indio_dev,
		struct iio_chan_spec const *chan, int val1,
		int val2, long mask)
{
	struct max77729_fuelgauge_data_iio *fuelgauge = iio_priv(indio_dev);
        struct max77729_fuelgauge_data *fg_data = fuelgauge->fg_data;
	int rc = 0;

	switch (chan->channel) {
	case PSY_IIO_FASTCHARGE_MODE:
		fg_data->is_fastcharge = val1;
		break;

	default:
		pr_err("Unsupported MAX77729_FG IIO chan %d\n", chan->channel);
		rc = -EINVAL;
		break;
	}
	if (rc < 0)
		pr_err("Couldn't write IIO channel %d, rc = %d\n",
			chan->channel, rc);
	return rc;
}

static int max77729_fg_iio_of_xlate(struct iio_dev *indio_dev,
				const struct of_phandle_args *iiospec)
{
	struct max77729_fuelgauge_data_iio *fuelgauge = iio_priv(indio_dev);
	struct iio_chan_spec *iio_chan = fuelgauge->iio_chan;
	int i;

	for (i = 0; i < ARRAY_SIZE(max77729_fg_iio_psy_channels);
					i++, iio_chan++)
		if (iio_chan->channel == iiospec->args[0])
			return i;

	return -EINVAL;
}

static const struct iio_info max77729_fg_iio_info = {
	.read_raw	= max77729_fg_iio_read_raw,
	.write_raw	= max77729_fg_iio_write_raw,
	.of_xlate	= max77729_fg_iio_of_xlate,
};

static int max77729_fg_iio_probe(struct platform_device *pdev)
{
	struct max77729_fuelgauge_data_iio *fuelgauge_iio;
	struct iio_dev *indio_dev = NULL;
        struct iio_chan_spec *chan = NULL;
        int num_iio_channels = ARRAY_SIZE(max77729_fg_iio_psy_channels);
        int rc = 0, i = 0;

	if (!fuelgauge) {
		pr_err("Fuelgauge data not found!. defer probe\n");
			return -EPROBE_DEFER;
	}

	if (!max77729) {
		pr_err("Failed to get max77729_dev from parent. defer probe\n");
			return -EPROBE_DEFER;
	}

        pr_info("max77729_fg_init_iio start\n");

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*fuelgauge_iio));
	if (!indio_dev)
		return -ENOMEM;

	fuelgauge_iio = iio_priv(indio_dev);
	fuelgauge_iio->indio_dev = indio_dev;
	fuelgauge_iio->dev = &pdev->dev;
	fuelgauge_iio->fg_data = fuelgauge;
	fuelgauge_iio->max77729 = max77729;

	platform_set_drvdata(pdev, fuelgauge_iio);

        fuelgauge_iio->iio_chan = devm_kcalloc(fuelgauge_iio->dev, num_iio_channels,
                                sizeof(*fuelgauge_iio->iio_chan), GFP_KERNEL);
        if (!fuelgauge_iio->iio_chan)
                return -ENOMEM;

        fuelgauge_iio->int_iio_chans = devm_kcalloc(fuelgauge_iio->dev,
                                num_iio_channels,
                                sizeof(*fuelgauge_iio->int_iio_chans),
                                GFP_KERNEL);

        if (!fuelgauge_iio->int_iio_chans)
                return -ENOMEM;

	indio_dev->name = fuelgauge_iio->dev->of_node->name;
	indio_dev->dev.parent = fuelgauge_iio->dev;
	indio_dev->dev.of_node = fuelgauge_iio->dev->of_node;
	indio_dev->info = &max77729_fg_iio_info;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = fuelgauge_iio->iio_chan;
	indio_dev->num_channels = num_iio_channels;

        for (i = 0; i < num_iio_channels; i++) {
                fuelgauge_iio->int_iio_chans[i].indio_dev = indio_dev;
                chan = &fuelgauge_iio->iio_chan[i];
                fuelgauge_iio->int_iio_chans[i].channel = chan;
                chan->address = i;
                chan->channel = max77729_fg_iio_psy_channels[i].channel_num;
                chan->type = max77729_fg_iio_psy_channels[i].type;
                chan->datasheet_name =
                        max77729_fg_iio_psy_channels[i].datasheet_name;
                chan->extend_name =
                        max77729_fg_iio_psy_channels[i].datasheet_name;
                chan->info_mask_separate =
                        max77729_fg_iio_psy_channels[i].info_mask;
        }

	rc = devm_iio_device_register(fuelgauge_iio->dev, indio_dev);
	if (rc) {
		pr_err("Failed to register MAX77729_FG IIO device, rc=%d\n", rc);
		return -ENODEV;
	}

        pr_info("Successfully loaded max77729-fuelgauge iio driver");
	return 0;
}

static const struct of_device_id max77729_fg_iio_of_match[] = {
    { .compatible = "maxim,max77729-fuelgauge-iio", },
    {}
};
MODULE_DEVICE_TABLE(of, max77729_fg_iio_of_match);

static struct platform_driver max77729_fg_iio_driver = {
    .driver = {
        .name = "max77729-fuelgauge-iio",
	.owner = THIS_MODULE,
        .of_match_table = max77729_fg_iio_of_match,
    },
    .probe = max77729_fg_iio_probe,
};

static int __init max77729_fg_iio_init(void)
{
         pr_info("init max77729_fg_iio_driver\n");
        return platform_driver_register(&max77729_fg_iio_driver);
}
late_initcall_sync(max77729_fg_iio_init);

static void __exit max77729_fg_iio_exit(void)
{
        platform_driver_unregister(&max77729_fg_iio_driver);
}
module_exit(max77729_fg_iio_exit);

MODULE_AUTHOR("muralivijay@github");
MODULE_DESCRIPTION("Max77729 fuelgauge iio channel driver");
MODULE_LICENSE("GPL");
