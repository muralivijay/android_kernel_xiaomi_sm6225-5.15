/* Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt)	"%s: " fmt, __func__

#include <linux/errno.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_device.h>
#include <linux/regmap.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

/*
 * Define vibration periods: default(5sec), min(50ms), max(15sec) and
 * overdrive(30ms).
 */
#define QPNP_VIB_MIN_PLAY_MS		50
#define QPNP_VIB_PLAY_MS		1000
#define QPNP_VIB_MAX_PLAY_MS		15000
#define QPNP_VIB_OVERDRIVE_PLAY_MS	30

struct vib_pwm_chip {
	struct device		*dev;
	struct led_classdev	cdev;
	struct regmap		*regmap;
	struct mutex		lock;
	//struct hrtimer		overdrive_timer;
	//struct work_struct	overdrive_work;

	u16			base;
	int			state;
	int			effect_idx;
	u64			vib_play_ms;
	bool			vib_enabled;
	//bool			disable_overdrive;

	struct pwm_device       *pwm_dev;
	struct pwm_device       *pwm_dir;

	u64	                pre_period_ns;
	u64	                period_ns;
	u64	                duty_ns;

	u32                     en_gpio;
	u32                     en_gpio_flags;

	int			pwm_nums;
	const char		*label;
	u8			id;
		
	
};

static int qpnp_vibrator_play_on(struct vib_pwm_chip *chip)
{
	struct pwm_state pstate;
	int err;

	if (chip->pwm_dev == NULL) {
		printk("vib---exit---qpnp_vibrator_play_on\n");
		return -ENOMEM;
	}
	
	pwm_get_state(chip->pwm_dev, &pstate);
	pstate.enabled = true;
	pstate.polarity = PWM_POLARITY_NORMAL;

	if (chip->effect_idx == 1) {
		pstate.period = 35 * 1000000;
		pstate.duty_cycle = 15 * 1000000;
	} else if (chip->effect_idx == 2) {
		pstate.period = 50 * 1000000;
		pstate.duty_cycle = 25 * 1000000;
	} else if (chip->effect_idx == 3) {
		pstate.period = 60 * 1000000;
		pstate.duty_cycle = 30 * 1000000;
	} else {
		pstate.period = 50000;
		pstate.duty_cycle = 42500;
	}
	printk("vib--play on-chip->effect_idx=%d pstate.period=%d pstate.duty_cycle=%d\n", chip->effect_idx, pstate.period, pstate.duty_cycle);

	if (gpio_is_valid(chip->en_gpio)) {
		err = gpio_direction_output(chip->en_gpio, 1);
		if (err)
			printk("vib---en fail, ret=%d\n", err);
	}

	err = pwm_apply_state(chip->pwm_dev, &pstate);
	if (err) {
		printk("vib---Apply PWM state for vib failed, err=%d\n", err);
	}

	return err;

}

static int qpnp_vibrator_play_off(struct vib_pwm_chip *chip)
{
	struct pwm_state pstate;
	int err;

	printk("vib---qpnp_vibrator_play_off\n");

	if (chip->pwm_dev == NULL) {
		printk("vib---exit---qpnp_vibrator_play_on\n");
		return -ENOMEM;
	}
	
	pwm_get_state(chip->pwm_dev, &pstate);
	pstate.enabled = false;
	//pstate.period = 10000;
	pstate.duty_cycle = 0;

	if (gpio_is_valid(chip->en_gpio)) {
		err = gpio_direction_output(chip->en_gpio, 0);
		if (err)
			printk("vib---en fail, ret=%d\n", err);
	}

	err = pwm_apply_state(chip->pwm_dev, &pstate);
	if (err) {
		printk("vib---Apply PWM state for vib failed, err=%d\n", err);
	}

	return err;

}

/*
static enum hrtimer_restart vib_overdrive_timer(struct hrtimer *timer)
{
	struct vib_pwm_chip *chip = container_of(timer, struct vib_pwm_chip,
					     overdrive_timer);
	//schedule_work(&chip->overdrive_work);
	printk("vib---overdrive timer expired\n");
	return HRTIMER_NORESTART;
}
*/

static ssize_t qpnp_vib_show_effect(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *cdev = dev_get_drvdata(dev);
	struct vib_pwm_chip *chip = container_of(cdev, struct vib_pwm_chip,
						cdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", chip->effect_idx);
}

static ssize_t qpnp_vib_store_effect(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	/* At present, nothing to do with setting state */
	return count;
}

static struct device_attribute qpnp_vib_attrs = 
	__ATTR(effect, 0664, qpnp_vib_show_effect, qpnp_vib_store_effect);


static enum led_brightness qpnp_vib_brightness_get(struct led_classdev *cdev)
{
	struct vib_pwm_chip *chip = container_of(cdev, struct vib_pwm_chip,
						cdev);

	return chip->state;
}

static void qpnp_vib_brightness_set(struct led_classdev *cdev,
			enum led_brightness level)
{
	struct vib_pwm_chip *chip = container_of(cdev, struct vib_pwm_chip,
						cdev);
	int ret = 0;

	chip->state = level;

	if (chip->state) {
		ret = qpnp_vibrator_play_on(chip);
		if (ret < 0)
			pr_err("set vibrator-on failed, ret=%d\n", ret);
	} else {
		/*
		if (!chip->disable_overdrive) {
			hrtimer_cancel(&chip->overdrive_timer);
			cancel_work_sync(&chip->overdrive_work);
		}*/
		ret = qpnp_vibrator_play_off(chip);
	}

	pr_debug("vibrator state=%d\n", chip->state);
}

static int qpnp_vibrator_pwm_suspend(struct device *dev)
{
	struct vib_pwm_chip *chip = dev_get_drvdata(dev);

	//printk("vib---qpnp_vibrator_pwm_suspend\n");
	mutex_lock(&chip->lock);
	/*
	if (!chip->disable_overdrive) {
		hrtimer_cancel(&chip->overdrive_timer);
		//cancel_work_sync(&chip->overdrive_work);
	}*/
	qpnp_vibrator_play_off(chip);
	mutex_unlock(&chip->lock);

	return 0;
}
static SIMPLE_DEV_PM_OPS(qpnp_vibrator_pwm_pm_ops, qpnp_vibrator_pwm_suspend,
			NULL);

static int qpnp_vib_parse_dt(struct vib_pwm_chip *chip)
{
	struct device_node *node = chip->dev->of_node, *child_node;
	int rc = 0, id = 0;

	chip->pwm_nums = of_get_available_child_count(node);
	if (chip->pwm_nums == 0) {
		dev_err(chip->dev, "No vib child node defined\n");
		return -ENODEV;
	}

	chip->en_gpio = of_get_named_gpio_flags(node, "vib,en-gpio", 0, &chip->en_gpio_flags);
	printk("vib---vib,en-gpio=%d\n", chip->en_gpio);

	for_each_available_child_of_node(node, child_node) {
		rc = of_property_read_u32(child_node, "pwm-sources", &id);
		if (rc) {
			dev_err(chip->dev, "Get pwm-sources failed, rc=%d\n", rc);
			return rc;
		}

		chip->id = id;
		chip->label = of_get_property(child_node, "label", NULL) ? : child_node->name;
		printk("chip->label=%s ", chip->label);

		chip->pwm_dev = devm_of_pwm_get(chip->dev, child_node, NULL);
		if (IS_ERR(chip->pwm_dev)) {
			rc = PTR_ERR(chip->pwm_dev);
			if (rc != -EPROBE_DEFER)
				dev_err(chip->dev, "Get pwm device for %s failed, rc=%d\n",
							chip->label, rc);
			return rc;
		}
	}
	return 0;
}

static int qpnp_vibrator_pwm_probe(struct platform_device *pdev)
{
	struct vib_pwm_chip *chip;
	int ret;
	u32 base = 0;

	printk("vib---wj---qpnp_vibrator_pwm_probe\n");

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->dev = &pdev->dev;

	ret = qpnp_vib_parse_dt(chip);
	if (ret < 0) {
		printk("vib---couldn't parse device tree, ret=%d\n", ret);
		return ret;
	}

	chip->base = (uint16_t)base;
	chip->vib_play_ms = QPNP_VIB_PLAY_MS;
	mutex_init(&chip->lock);
	//INIT_WORK(&chip->overdrive_work, qpnp_vib_overdrive_work);

	//hrtimer_init(&chip->overdrive_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	//chip->overdrive_timer.function = vib_overdrive_timer;
	dev_set_drvdata(&pdev->dev, chip);

	chip->cdev.name = "vibrator";
	chip->cdev.brightness_get = qpnp_vib_brightness_get;
	chip->cdev.brightness_set = qpnp_vib_brightness_set;
	chip->cdev.max_brightness = 100;
	ret = devm_led_classdev_register(&pdev->dev, &chip->cdev);
	if (ret < 0) {
		printk("vib---Error in registering led class device, ret=%d\n", ret);
		goto fail;
	}
	
	ret = sysfs_create_file(&chip->cdev.dev->kobj,
			&qpnp_vib_attrs.attr);
	if (ret < 0) {
		dev_err(&pdev->dev, "vib---Error in creating sysfs file, ret=%d\n",
			ret);
		goto sysfs_fail;
	}
	
	//printk("vib---Vibrator PWM successfully registered: overdrive = %s\n",
	//	chip->disable_overdrive ? "disabled" : "enabled");
	return 0;

sysfs_fail:
	sysfs_remove_file(&chip->cdev.dev->kobj,
			&qpnp_vib_attrs.attr);
fail:
	mutex_destroy(&chip->lock);
	dev_set_drvdata(&pdev->dev, NULL);
	return ret;
}

static int qpnp_vibrator_pwm_remove(struct platform_device *pdev)
{
	struct vib_pwm_chip *chip = dev_get_drvdata(&pdev->dev);

	//printk("vib---wj---qpnp_vibrator_pwm_probe\n");
	/*
	if (!chip->disable_overdrive) {
		hrtimer_cancel(&chip->overdrive_timer);
		//cancel_work_sync(&chip->overdrive_work);
	}*/
	mutex_destroy(&chip->lock);
	dev_set_drvdata(&pdev->dev, NULL);

	return 0;
}

static const struct of_device_id vibrator_pwm_match_table[] = {
	{ .compatible = "qcom,lct-pwm-vibrator" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, vibrator_pwm_match_table);

static struct platform_driver qpnp_vibrator_pwm_driver = {
	.driver	= {
		.name		= "qcom,lct-pwm-vibrator",
		.of_match_table	= vibrator_pwm_match_table,
		.pm		= &qpnp_vibrator_pwm_pm_ops,
	},
	.probe	= qpnp_vibrator_pwm_probe,
	.remove	= qpnp_vibrator_pwm_remove,
};
module_platform_driver(qpnp_vibrator_pwm_driver);

MODULE_DESCRIPTION("QCOM QPNP Vibrator-PWM driver");
MODULE_LICENSE("GPL v2");
