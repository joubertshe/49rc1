/*
 * AXP regulators driver
 *
 * Copyright (C) 2016 Jean-Francois Moine <moinejf@free.fr>
 * Copyright (C) 2013 Carlo Caione <carlo@caione.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/mfd/axp20x.h>
#include <linux/sunxi-rsb.h>

#include "axp-regulator.h"

#define AXP20X_WORKMODE_DCDC2_MASK	BIT(2)
#define AXP20X_WORKMODE_DCDC3_MASK	BIT(1)
#define AXP22X_WORKMODE_DCDCX_MASK(x)	BIT(x)

#define AXP20X_FREQ_DCDC_MASK	0x0f

#define AXP22X_MISC_N_VBUSEN_FUNC	BIT(4)

const struct regulator_ops axp_ops_fixed = {
	.list_voltage		= regulator_list_voltage_linear,
};
EXPORT_SYMBOL_GPL(axp_ops_fixed);

const struct regulator_ops axp_ops_range = {
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_linear_range,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
};
EXPORT_SYMBOL_GPL(axp_ops_range);

const struct regulator_ops axp_ops = {
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_linear,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
};
EXPORT_SYMBOL_GPL(axp_ops);

const struct regulator_ops axp_ops_sw = {
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
};
EXPORT_SYMBOL_GPL(axp_ops_sw);

static const struct regulator_desc axp22x_drivevbus_regulator = {
	.name		= "drivevbus",
	.supply_name	= "drivevbus",
	.of_match	= of_match_ptr("drivevbus"),
	.regulators_node = of_match_ptr("regulators"),
	.type		= REGULATOR_VOLTAGE,
	.owner		= THIS_MODULE,
	.enable_reg	= AXP20X_VBUS_IPSOUT_MGMT,
	.enable_mask	= BIT(2),
	.ops		= &axp_ops_sw,
};

static int axp_set_dcdc_freq(struct device *dev,
					u32 dcdcfreq)
{
	struct axp20x_dev *axp20x = dev_get_drvdata(dev);
	unsigned int reg = AXP20X_DCDC_FREQ;
	u32 min, max, def, step;

	switch (axp20x->variant) {
	case AXP202_ID:
	case AXP209_ID:
		min = 750;
		max = 1875;
		def = 1500;
		step = 75;
		break;
	case AXP806_ID:
		/*
		 * AXP806 DCDC work frequency setting has the same range and
		 * step as AXP22X, but at a different register.
		 * Fall through to the check below.
		 * (See include/linux/mfd/axp20x.h)
		 */
		reg = AXP806_DCDC_FREQ_CTRL;
	case AXP221_ID:
	case AXP223_ID:
	case AXP809_ID:
	case AXP803_ID:
	case AXP813_ID:
		min = 1800;
		max = 4050;
		def = 3000;
		step = 150;
		break;
	default:
		dev_err(dev,
			"Setting DCDC frequency for unsupported AXP variant\n");
		return -EINVAL;
	}

	if (dcdcfreq == 0)
		dcdcfreq = def;

	if (dcdcfreq < min) {
		dcdcfreq = min;
		dev_warn(dev, "DCDC frequency too low. Set to %ukHz\n",
			 min);
	}

	if (dcdcfreq > max) {
		dcdcfreq = max;
		dev_warn(dev, "DCDC frequency too high. Set to %ukHz\n",
			 max);
	}

	dcdcfreq = (dcdcfreq - min) / step;

	return regmap_update_bits(axp20x->regmap, reg,
				  AXP20X_FREQ_DCDC_MASK, dcdcfreq);
}

static int axp_regulator_parse_dt(struct device *dev)
{
	struct device_node *np, *regulators;
	int ret;
	u32 dcdcfreq = 0;

	np = of_node_get(dev->of_node);
	if (!np)
		return 0;

	regulators = of_get_child_by_name(np, "regulators");
	if (!regulators) {
		dev_warn(dev, "regulators node not found\n");
	} else {
		of_property_read_u32(regulators, "x-powers,dcdc-freq", &dcdcfreq);
		ret = axp_set_dcdc_freq(dev, dcdcfreq);
		if (ret < 0) {
			dev_err(dev, "Error setting dcdc frequency: %d\n", ret);
			return ret;
		}

		of_node_put(regulators);
	}

	return 0;
}

static int axp_set_dcdc_workmode(struct regulator_dev *rdev,
				int id, u32 workmode)
{
	struct axp20x_dev *axp20x = rdev_get_drvdata(rdev);
	unsigned int reg = AXP20X_DCDC_MODE;
	unsigned int mask;

	switch (axp20x->variant) {
	case AXP202_ID:
	case AXP209_ID:
		if ((id != AXP20X_DCDC2) && (id != AXP20X_DCDC3))
			return -EINVAL;

		mask = AXP20X_WORKMODE_DCDC2_MASK;
		if (id == AXP20X_DCDC3)
			mask = AXP20X_WORKMODE_DCDC3_MASK;

		workmode <<= ffs(mask) - 1;
		break;

	case AXP806_ID:
		reg = AXP806_DCDC_MODE_CTRL2;
		/*
		 * AXP806 DCDC regulator IDs have the same range as AXP22X.
		 * Fall through to the check below.
		 * (See include/linux/mfd/axp20x.h)
		 */
	case AXP221_ID:
	case AXP223_ID:
	case AXP809_ID:
		if (id < AXP22X_DCDC1 || id > AXP22X_DCDC5)
			return -EINVAL;

		mask = AXP22X_WORKMODE_DCDCX_MASK(id - AXP22X_DCDC1);
		workmode <<= id - AXP22X_DCDC1;
		break;

	default:
		/* should not happen */
		WARN_ON(1);
		return -EINVAL;
	}

	return regmap_update_bits(rdev->regmap, reg, mask, workmode);
}

/* create the regulators */
int axp_regulator_create(struct device *dev,
			 const struct axp_cfg *axp_cfg)
{
	struct regulator_dev *rdev;
	struct axp20x_dev *axp20x = dev_get_drvdata(dev);
	struct regulator_config config = {
		.dev = dev,
		.regmap = axp20x->regmap,
		.driver_data = axp20x,
	};
	int ret, i;
	u32 workmode;
	const char *dcdc1_name = NULL;
	const char *dcdc5_name = NULL;

	/* This only sets the dcdc freq. Ignore any errors */
	axp_regulator_parse_dt(dev);

	for (i = 0; i < axp_cfg->nregulators; i++) {
		const struct regulator_desc *desc = &axp_cfg->regulators[i];
		struct regulator_desc *new_desc;

		if (axp_cfg->skip_bitmap & (1 << i))
			continue;

		/*
		 * Regulators DC1SW and DC5LDO are connected internally,
		 * so we have to handle their supply names separately.
		 *
		 * We always register the regulators in proper sequence,
		 * so the supply names are correctly read. See the last
		 * part of this loop to see where we save the DT defined
		 * name.
		 */
		if (i == axp_cfg->dc1sw_ix && dcdc1_name) {
			new_desc = devm_kzalloc(dev, sizeof(*desc),
						GFP_KERNEL);
			*new_desc = *desc;
			new_desc->supply_name = dcdc1_name;
			desc = new_desc;
		}

		if (i == axp_cfg->dc5ldo_ix && dcdc5_name) {
			new_desc = devm_kzalloc(dev, sizeof(*desc),
						GFP_KERNEL);
			*new_desc = *desc;
			new_desc->supply_name = dcdc5_name;
			desc = new_desc;
		}

		rdev = devm_regulator_register(dev, desc, &config);
		if (IS_ERR(rdev)) {
			dev_err(dev, "Failed to register %s\n",
				axp_cfg->regulators[i].name);

			return PTR_ERR(rdev);
		}

		ret = of_property_read_u32(rdev->dev.of_node,
					   "x-powers,dcdc-workmode",
					   &workmode);
		if (!ret) {
			if (axp_set_dcdc_workmode(rdev, i, workmode))
				dev_err(dev, "Failed to set workmode on %s\n",
					rdev->desc->name);
		}

		/*
		 * Save AXP22X DCDC1 / DCDC5 regulator names for later.
		 */
		if (i == axp_cfg->dcdc1_ix)
			of_property_read_string(rdev->dev.of_node,
						"regulator-name",
						&dcdc1_name);
		if (i == axp_cfg->dcdc5_ix)
			of_property_read_string(rdev->dev.of_node,
						"regulator-name",
						&dcdc5_name);
	}

	if (axp_cfg->drivevbus) {
		/* Change N_VBUSEN sense pin to DRIVEVBUS output pin */
		regmap_update_bits(axp20x->regmap, AXP20X_OVER_TMP,
				   AXP22X_MISC_N_VBUSEN_FUNC, 0);
		rdev = devm_regulator_register(dev,
					       &axp22x_drivevbus_regulator,
					       &config);
		if (IS_ERR(rdev)) {
			dev_err(dev, "Failed to register drivevbus\n");
			return PTR_ERR(rdev);
		}
	}

	return 0;
}
EXPORT_SYMBOL_GPL(axp_regulator_create);

/* probe/remove RSB devices */
int axp_rsb_probe(struct sunxi_rsb_device *rdev,
		  struct axp20x_dev *axp20x,
		  const struct axp_cfg *axp_cfg)
{
	int ret;

	axp20x->dev = &rdev->dev;
	axp20x->irq = rdev->irq;
	dev_set_drvdata(&rdev->dev, axp20x);

	ret = axp20x_match_device(axp20x);
	if (ret)
		return ret;

	axp20x->regmap = devm_regmap_init_sunxi_rsb(rdev,
						    axp20x->regmap_cfg);
	if (IS_ERR(axp20x->regmap)) {
		ret = PTR_ERR(axp20x->regmap);
		dev_err(&rdev->dev, "regmap init failed: %d\n", ret);
		return ret;
	}

	ret = axp20x_device_probe(axp20x);
	if (ret < 0)
		return ret;

	return axp_regulator_create(&rdev->dev, axp_cfg);
}
EXPORT_SYMBOL_GPL(axp_rsb_probe);

int axp_rsb_remove(struct sunxi_rsb_device *rdev)
{
	struct axp20x_dev *axp20x = sunxi_rsb_device_get_drvdata(rdev);

	return axp20x_device_remove(axp20x);
}
EXPORT_SYMBOL_GPL(axp_rsb_remove);

MODULE_AUTHOR("Carlo Caione <carlo@caione.org>");
MODULE_DESCRIPTION("Regulator Module for AXP PMIC");
MODULE_LICENSE("GPL v2");
