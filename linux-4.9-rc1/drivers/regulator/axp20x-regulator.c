/*
 * AXP20x regulators driver.
 *
 * Copyright (C) 2013 Carlo Caione <carlo@caione.org>
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License. See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/err.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/mfd/axp20x.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>

#include "axp-regulator.h"

static const struct regulator_linear_range axp20x_ldo4_ranges[] = {
	REGULATOR_LINEAR_RANGE(1250000, 0x0, 0x0, 0),
	REGULATOR_LINEAR_RANGE(1300000, 0x1, 0x8, 100000),
	REGULATOR_LINEAR_RANGE(2500000, 0x9, 0x9, 0),
	REGULATOR_LINEAR_RANGE(2700000, 0xa, 0xb, 100000),
	REGULATOR_LINEAR_RANGE(3000000, 0xc, 0xf, 100000),
};

static const struct regulator_desc axp20x_regulators[] = {
	AXP_DESC(AXP20X, DCDC2, "dcdc2", "vin2", 700, 2275, 25,
		 AXP20X_DCDC2_V_OUT, 0x3f, AXP20X_PWR_OUT_CTRL, 0x10),
	AXP_DESC(AXP20X, DCDC3, "dcdc3", "vin3", 700, 3500, 25,
		 AXP20X_DCDC3_V_OUT, 0x7f, AXP20X_PWR_OUT_CTRL, 0x02),
	AXP_DESC_FIXED(AXP20X, LDO1, "ldo1", "acin", 1300),
	AXP_DESC(AXP20X, LDO2, "ldo2", "ldo24in", 1800, 3300, 100,
		 AXP20X_LDO24_V_OUT, 0xf0, AXP20X_PWR_OUT_CTRL, 0x04),
	AXP_DESC(AXP20X, LDO3, "ldo3", "ldo3in", 700, 3500, 25,
		 AXP20X_LDO3_V_OUT, 0x7f, AXP20X_PWR_OUT_CTRL, 0x40),
	AXP_DESC_RANGES(AXP20X, LDO4, "ldo4", "ldo24in", axp20x_ldo4_ranges,
			16, AXP20X_LDO24_V_OUT, 0x0f, AXP20X_PWR_OUT_CTRL,
			0x08),
	AXP_DESC_IO(AXP20X, LDO5, "ldo5", "ldo5in", 1800, 3300, 100,
		    AXP20X_LDO5_V_OUT, 0xf0, AXP20X_GPIO0_CTRL, 0x07,
		    AXP20X_IO_ENABLED, AXP20X_IO_DISABLED),
};

static const struct regulator_desc axp22x_regulators[] = {
	AXP_DESC(AXP22X, DCDC1, "dcdc1", "vin1", 1600, 3400, 100,
		 AXP22X_DCDC1_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL1, BIT(1)),
	AXP_DESC(AXP22X, DCDC2, "dcdc2", "vin2", 600, 1540, 20,
		 AXP22X_DCDC2_V_OUT, 0x3f, AXP22X_PWR_OUT_CTRL1, BIT(2)),
	AXP_DESC(AXP22X, DCDC3, "dcdc3", "vin3", 600, 1860, 20,
		 AXP22X_DCDC3_V_OUT, 0x3f, AXP22X_PWR_OUT_CTRL1, BIT(3)),
	AXP_DESC(AXP22X, DCDC4, "dcdc4", "vin4", 600, 1540, 20,
		 AXP22X_DCDC4_V_OUT, 0x3f, AXP22X_PWR_OUT_CTRL1, BIT(4)),
	AXP_DESC(AXP22X, DCDC5, "dcdc5", "vin5", 1000, 2550, 50,
		 AXP22X_DCDC5_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL1, BIT(5)),
	/* secondary switchable output of DCDC1 */
	AXP_DESC_SW(AXP22X, DC1SW, "dc1sw", NULL, AXP22X_PWR_OUT_CTRL2,
		    BIT(7)),
	/* LDO regulator internally chained to DCDC5 */
	AXP_DESC(AXP22X, DC5LDO, "dc5ldo", NULL, 700, 1400, 100,
		 AXP22X_DC5LDO_V_OUT, 0x7, AXP22X_PWR_OUT_CTRL1, BIT(0)),
	AXP_DESC(AXP22X, ALDO1, "aldo1", "aldoin", 700, 3300, 100,
		 AXP22X_ALDO1_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL1, BIT(6)),
	AXP_DESC(AXP22X, ALDO2, "aldo2", "aldoin", 700, 3300, 100,
		 AXP22X_ALDO2_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL1, BIT(7)),
	AXP_DESC(AXP22X, ALDO3, "aldo3", "aldoin", 700, 3300, 100,
		 AXP22X_ALDO3_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL3, BIT(7)),
	AXP_DESC(AXP22X, DLDO1, "dldo1", "dldoin", 700, 3300, 100,
		 AXP22X_DLDO1_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2, BIT(3)),
	AXP_DESC(AXP22X, DLDO2, "dldo2", "dldoin", 700, 3300, 100,
		 AXP22X_DLDO2_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2, BIT(4)),
	AXP_DESC(AXP22X, DLDO3, "dldo3", "dldoin", 700, 3300, 100,
		 AXP22X_DLDO3_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2, BIT(5)),
	AXP_DESC(AXP22X, DLDO4, "dldo4", "dldoin", 700, 3300, 100,
		 AXP22X_DLDO4_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2, BIT(6)),
	AXP_DESC(AXP22X, ELDO1, "eldo1", "eldoin", 700, 3300, 100,
		 AXP22X_ELDO1_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2, BIT(0)),
	AXP_DESC(AXP22X, ELDO2, "eldo2", "eldoin", 700, 3300, 100,
		 AXP22X_ELDO2_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2, BIT(1)),
	AXP_DESC(AXP22X, ELDO3, "eldo3", "eldoin", 700, 3300, 100,
		 AXP22X_ELDO3_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2, BIT(2)),
	/* Note the datasheet only guarantees reliable operation up to
	 * 3.3V, this needs to be enforced via dts provided constraints */
	AXP_DESC_IO(AXP22X, LDO_IO0, "ldo_io0", "ips", 700, 3800, 100,
		    AXP22X_LDO_IO0_V_OUT, 0x1f, AXP20X_GPIO0_CTRL, 0x07,
		    AXP22X_IO_ENABLED, AXP22X_IO_DISABLED),
	/* Note the datasheet only guarantees reliable operation up to
	 * 3.3V, this needs to be enforced via dts provided constraints */
	AXP_DESC_IO(AXP22X, LDO_IO1, "ldo_io1", "ips", 700, 3800, 100,
		    AXP22X_LDO_IO1_V_OUT, 0x1f, AXP20X_GPIO1_CTRL, 0x07,
		    AXP22X_IO_ENABLED, AXP22X_IO_DISABLED),
	AXP_DESC_FIXED(AXP22X, RTC_LDO, "rtc_ldo", "ips", 3000),
};

static const struct regulator_linear_range axp806_dcdca_ranges[] = {
	REGULATOR_LINEAR_RANGE(600000, 0x0, 0x32, 10000),
	REGULATOR_LINEAR_RANGE(1120000, 0x33, 0x47, 20000),
};

static const struct regulator_linear_range axp806_dcdcd_ranges[] = {
	REGULATOR_LINEAR_RANGE(600000, 0x0, 0x2d, 20000),
	REGULATOR_LINEAR_RANGE(1600000, 0x2e, 0x3f, 100000),
};

static const struct regulator_linear_range axp806_cldo2_ranges[] = {
	REGULATOR_LINEAR_RANGE(700000, 0x0, 0x1a, 100000),
	REGULATOR_LINEAR_RANGE(3400000, 0x1b, 0x1f, 200000),
};

static const struct regulator_desc axp806_regulators[] = {
	AXP_DESC_RANGES(AXP806, DCDCA, "dcdca", "vina", axp806_dcdca_ranges,
			72, AXP806_DCDCA_V_CTRL, 0x7f, AXP806_PWR_OUT_CTRL1,
			BIT(0)),
	AXP_DESC(AXP806, DCDCB, "dcdcb", "vinb", 1000, 2550, 50,
		 AXP806_DCDCB_V_CTRL, 0x1f, AXP806_PWR_OUT_CTRL1, BIT(1)),
	AXP_DESC_RANGES(AXP806, DCDCC, "dcdcc", "vinc", axp806_dcdca_ranges,
			72, AXP806_DCDCC_V_CTRL, 0x7f, AXP806_PWR_OUT_CTRL1,
			BIT(2)),
	AXP_DESC_RANGES(AXP806, DCDCD, "dcdcd", "vind", axp806_dcdcd_ranges,
			64, AXP806_DCDCD_V_CTRL, 0x3f, AXP806_PWR_OUT_CTRL1,
			BIT(3)),
	AXP_DESC(AXP806, DCDCE, "dcdce", "vine", 1100, 3400, 100,
		 AXP806_DCDCB_V_CTRL, 0x1f, AXP806_PWR_OUT_CTRL1, BIT(4)),
	AXP_DESC(AXP806, ALDO1, "aldo1", "aldoin", 700, 3300, 100,
		 AXP806_ALDO1_V_CTRL, 0x1f, AXP806_PWR_OUT_CTRL1, BIT(5)),
	AXP_DESC(AXP806, ALDO2, "aldo2", "aldoin", 700, 3400, 100,
		 AXP806_ALDO2_V_CTRL, 0x1f, AXP806_PWR_OUT_CTRL1, BIT(6)),
	AXP_DESC(AXP806, ALDO3, "aldo3", "aldoin", 700, 3300, 100,
		 AXP806_ALDO3_V_CTRL, 0x1f, AXP806_PWR_OUT_CTRL1, BIT(7)),
	AXP_DESC(AXP806, BLDO1, "bldo1", "bldoin", 700, 1900, 100,
		 AXP806_BLDO1_V_CTRL, 0x0f, AXP806_PWR_OUT_CTRL2, BIT(0)),
	AXP_DESC(AXP806, BLDO2, "bldo2", "bldoin", 700, 1900, 100,
		 AXP806_BLDO2_V_CTRL, 0x0f, AXP806_PWR_OUT_CTRL2, BIT(1)),
	AXP_DESC(AXP806, BLDO3, "bldo3", "bldoin", 700, 1900, 100,
		 AXP806_BLDO3_V_CTRL, 0x0f, AXP806_PWR_OUT_CTRL2, BIT(2)),
	AXP_DESC(AXP806, BLDO4, "bldo4", "bldoin", 700, 1900, 100,
		 AXP806_BLDO4_V_CTRL, 0x0f, AXP806_PWR_OUT_CTRL2, BIT(3)),
	AXP_DESC(AXP806, CLDO1, "cldo1", "cldoin", 700, 3300, 100,
		 AXP806_CLDO1_V_CTRL, 0x1f, AXP806_PWR_OUT_CTRL2, BIT(4)),
	AXP_DESC_RANGES(AXP806, CLDO2, "cldo2", "cldoin", axp806_cldo2_ranges,
			32, AXP806_CLDO2_V_CTRL, 0x1f, AXP806_PWR_OUT_CTRL2,
			BIT(5)),
	AXP_DESC(AXP806, CLDO3, "cldo3", "cldoin", 700, 3300, 100,
		 AXP806_CLDO3_V_CTRL, 0x1f, AXP806_PWR_OUT_CTRL2, BIT(6)),
	AXP_DESC_SW(AXP806, SW, "sw", "swin", AXP806_PWR_OUT_CTRL2, BIT(7)),
};

static const struct regulator_linear_range axp809_dcdc4_ranges[] = {
	REGULATOR_LINEAR_RANGE(600000, 0x0, 0x2f, 20000),
	REGULATOR_LINEAR_RANGE(1800000, 0x30, 0x38, 100000),
};

static const struct regulator_desc axp809_regulators[] = {
	AXP_DESC(AXP809, DCDC1, "dcdc1", "vin1", 1600, 3400, 100,
		 AXP22X_DCDC1_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL1, BIT(1)),
	AXP_DESC(AXP809, DCDC2, "dcdc2", "vin2", 600, 1540, 20,
		 AXP22X_DCDC2_V_OUT, 0x3f, AXP22X_PWR_OUT_CTRL1, BIT(2)),
	AXP_DESC(AXP809, DCDC3, "dcdc3", "vin3", 600, 1860, 20,
		 AXP22X_DCDC3_V_OUT, 0x3f, AXP22X_PWR_OUT_CTRL1, BIT(3)),
	AXP_DESC_RANGES(AXP809, DCDC4, "dcdc4", "vin4", axp809_dcdc4_ranges,
			57, AXP22X_DCDC4_V_OUT, 0x3f, AXP22X_PWR_OUT_CTRL1,
			BIT(4)),
	AXP_DESC(AXP809, DCDC5, "dcdc5", "vin5", 1000, 2550, 50,
		 AXP22X_DCDC5_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL1, BIT(5)),
	/* secondary switchable output of DCDC1 */
	AXP_DESC_SW(AXP809, DC1SW, "dc1sw", NULL, AXP22X_PWR_OUT_CTRL2,
		    BIT(7)),
	/* LDO regulator internally chained to DCDC5 */
	AXP_DESC(AXP809, DC5LDO, "dc5ldo", NULL, 700, 1400, 100,
		 AXP22X_DC5LDO_V_OUT, 0x7, AXP22X_PWR_OUT_CTRL1, BIT(0)),
	AXP_DESC(AXP809, ALDO1, "aldo1", "aldoin", 700, 3300, 100,
		 AXP22X_ALDO1_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL1, BIT(6)),
	AXP_DESC(AXP809, ALDO2, "aldo2", "aldoin", 700, 3300, 100,
		 AXP22X_ALDO2_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL1, BIT(7)),
	AXP_DESC(AXP809, ALDO3, "aldo3", "aldoin", 700, 3300, 100,
		 AXP22X_ALDO3_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2, BIT(5)),
	AXP_DESC_RANGES(AXP809, DLDO1, "dldo1", "dldoin", axp806_cldo2_ranges,
			32, AXP22X_DLDO1_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2,
			BIT(3)),
	AXP_DESC(AXP809, DLDO2, "dldo2", "dldoin", 700, 3300, 100,
		 AXP22X_DLDO2_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2, BIT(4)),
	AXP_DESC(AXP809, ELDO1, "eldo1", "eldoin", 700, 3300, 100,
		 AXP22X_ELDO1_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2, BIT(0)),
	AXP_DESC(AXP809, ELDO2, "eldo2", "eldoin", 700, 3300, 100,
		 AXP22X_ELDO2_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2, BIT(1)),
	AXP_DESC(AXP809, ELDO3, "eldo3", "eldoin", 700, 3300, 100,
		 AXP22X_ELDO3_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2, BIT(2)),
	AXP_DESC_IO(AXP809, LDO_IO0, "ldo_io0", "ips", 700, 3300, 100,
		    AXP22X_LDO_IO0_V_OUT, 0x1f, AXP20X_GPIO0_CTRL, 0x07,
		    AXP22X_IO_ENABLED, AXP22X_IO_DISABLED),
	AXP_DESC_IO(AXP809, LDO_IO1, "ldo_io1", "ips", 700, 3300, 100,
		    AXP22X_LDO_IO1_V_OUT, 0x1f, AXP20X_GPIO1_CTRL, 0x07,
		    AXP22X_IO_ENABLED, AXP22X_IO_DISABLED),
	AXP_DESC_FIXED(AXP809, RTC_LDO, "rtc_ldo", "ips", 1800),
	AXP_DESC_SW(AXP809, SW, "sw", "swin", AXP22X_PWR_OUT_CTRL2, BIT(6)),
};

/*
 * This function checks which regulators are part of poly-phase
 * output setups based on the registers settings.
 * Returns a bitmap of these regulators.
 */
static u32 axp20x_polyphase_slave(struct axp20x_dev *axp20x)
{
	u32 reg = 0, bitmap = 0;

	regmap_read(axp20x->regmap, AXP806_DCDC_MODE_CTRL2, &reg);

	if ((reg & GENMASK(7, 6)) == BIT(5))
		bitmap |= 1 << AXP806_DCDCE;
	if ((reg & GENMASK(7, 6)) == BIT(6))
		bitmap |= 1 << AXP806_DCDCB;
	if ((reg & GENMASK(7, 6)) == BIT(7))
		bitmap |= (1 << AXP806_DCDCB) | (1 << AXP806_DCDCC);

	return bitmap;
}

static int axp20x_regulator_probe(struct platform_device *pdev)
{
	struct device *dev = pdev->dev.parent;
	struct axp20x_dev *axp20x = dev_get_drvdata(dev);
	struct axp_cfg axp_cfg;

	axp_cfg.dcdc1_ix = -1;
	axp_cfg.dcdc5_ix = -1;
	axp_cfg.dc1sw_ix = -1;
	axp_cfg.dc5ldo_ix = -1;
	axp_cfg.drivevbus = false;
	axp_cfg.skip_bitmap = 0;

	switch (axp20x->variant) {
	case AXP202_ID:
	case AXP209_ID:
		axp_cfg.regulators = axp20x_regulators;
		axp_cfg.nregulators = AXP20X_REG_ID_MAX;
		break;
	case AXP221_ID:
	case AXP223_ID:
		axp_cfg.regulators = axp22x_regulators;
		axp_cfg.nregulators = AXP22X_REG_ID_MAX;
		axp_cfg.dcdc1_ix = AXP22X_DCDC1;
		axp_cfg.dcdc5_ix = AXP22X_DCDC5;
		axp_cfg.dc1sw_ix = AXP22X_DC1SW;
		axp_cfg.dc5ldo_ix = AXP22X_DC5LDO;
		axp_cfg.drivevbus = of_property_read_bool(dev->of_node,
						  "x-powers,drive-vbus-en");
		break;
	case AXP806_ID:
		axp_cfg.regulators = axp806_regulators;
		axp_cfg.nregulators = AXP806_REG_ID_MAX;

		/*
		 * The regulators which are slave in a poly-phase setup
		 * are skipped, as their controls are bound to the master
		 * regulator and won't work.
		 */
		axp_cfg.skip_bitmap |= axp20x_polyphase_slave(axp20x);
		break;
	case AXP809_ID:
		axp_cfg.regulators = axp809_regulators;
		axp_cfg.nregulators = AXP809_REG_ID_MAX;
		axp_cfg.dcdc1_ix = AXP809_DCDC1;
		axp_cfg.dcdc5_ix = AXP809_DCDC5;
		axp_cfg.dc1sw_ix = AXP809_DC1SW;
		axp_cfg.dc5ldo_ix = AXP809_DC5LDO;
		break;
	default:
		dev_err(dev, "Unsupported AXP variant: %ld\n",
			axp20x->variant);
		return -EINVAL;
	}

	return axp_regulator_create(dev, &axp_cfg);
}

static struct platform_driver axp20x_regulator_driver = {
	.probe	= axp20x_regulator_probe,
	.driver	= {
		.name		= "axp20x-regulator",
	},
};

module_platform_driver(axp20x_regulator_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Carlo Caione <carlo@caione.org>");
MODULE_DESCRIPTION("Regulator Driver for AXP20X PMIC");
MODULE_ALIAS("platform:axp20x-regulator");
