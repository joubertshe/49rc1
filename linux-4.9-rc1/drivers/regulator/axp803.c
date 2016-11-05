/*
 * AXP803 regulator driver
 *
 * Copyright (C) 2016 Jean-Francois Moine <moinejf@free.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/mfd/axp20x.h>
#include <linux/regulator/driver.h>
#include <linux/sunxi-rsb.h>

#include "axp-regulator.h"

enum {
	AXP803_DLDO1 = 0,
	AXP803_DLDO2,
	AXP803_DLDO3,
	AXP803_DLDO4,
	AXP803_ELDO1,
	AXP803_ELDO2,
	AXP803_ELDO3,
	AXP803_FLDO1,
	AXP803_FLDO2,
	AXP803_DCDC1,
	AXP803_DCDC2,
	AXP803_DCDC3,
	AXP803_DCDC4,
	AXP803_DCDC5,
	AXP803_DCDC6,
	AXP803_ALDO1,
	AXP803_ALDO2,
	AXP803_ALDO3,
	AXP803_LDO_IO0,
	AXP803_LDO_IO1,
	AXP803_RTC_LDO,
	AXP803_DC1SW,
};

/* AXP803 registers */
#define AXP803_FLDO1_V_OUT		0x1c
#define AXP803_FLDO2_V_OUT		0x1d
#define AXP803_DCDC1_V_OUT		0x20
#define AXP803_DCDC2_V_OUT		0x21
#define AXP803_DCDC3_V_OUT		0x22
#define AXP803_DCDC4_V_OUT		0x23
#define AXP803_DCDC5_V_OUT		0x24
#define AXP803_DCDC6_V_OUT		0x25

static const struct regulator_linear_range axp803_dldo2_ranges[] = {
	REGULATOR_LINEAR_RANGE(700000, 0, 26, 100000),
	REGULATOR_LINEAR_RANGE(3400000, 27, 31, 200000),
};

static const struct regulator_linear_range axp803_dcdc2_4_ranges[] = {
	REGULATOR_LINEAR_RANGE(500000, 0, 71, 10000),
	REGULATOR_LINEAR_RANGE(1220000, 72, 76, 20000),
};

static const struct regulator_linear_range axp803_dcdc5_ranges[] = {
	REGULATOR_LINEAR_RANGE(800000, 0, 33, 10000),
	REGULATOR_LINEAR_RANGE(1140000, 34, 69, 20000),
};

static const struct regulator_linear_range axp803_dcdc6_ranges[] = {
	REGULATOR_LINEAR_RANGE(600000, 0, 51, 10000),
	REGULATOR_LINEAR_RANGE(1120000, 52, 72, 20000),
};

static const struct regulator_desc axp803_regulators[] = {
	AXP_DESC(AXP803, DLDO1, "dldo1", "dldoin", 700, 3300, 100,
		 AXP22X_DLDO1_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2, BIT(3)),
	AXP_DESC_RANGES(AXP803, DLDO2, "dldo2", "dldoin", axp803_dldo2_ranges,
			31, AXP22X_DLDO2_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2,
			BIT(4)),
	AXP_DESC(AXP803, DLDO3, "dldo3", "dldoin", 700, 3300, 100,
		 AXP22X_DLDO3_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2, BIT(5)),
	AXP_DESC(AXP803, DLDO4, "dldo4", "dldoin", 700, 3300, 100,
		 AXP22X_DLDO4_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2, BIT(6)),
	AXP_DESC(AXP803, ELDO1, "eldo1", "eldoin", 700, 1900, 50,
		 AXP22X_ELDO1_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2, BIT(0)),
	AXP_DESC(AXP803, ELDO2, "eldo2", "eldoin", 700, 1900, 50,
		 AXP22X_ELDO2_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2, BIT(1)),
	AXP_DESC(AXP803, ELDO3, "eldo3", "eldoin", 700, 1900, 50,
		 AXP22X_ELDO3_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL2, BIT(2)),
	AXP_DESC(AXP803, FLDO1, "fldo1", "fldoin", 700, 1450, 50,
		 AXP803_FLDO1_V_OUT, 0x0f, AXP22X_PWR_OUT_CTRL3, BIT(2)),
	AXP_DESC(AXP803, FLDO2, "fldo2", "fldoin", 700, 1450, 50,
		 AXP803_FLDO2_V_OUT, 0x0f, AXP22X_PWR_OUT_CTRL3, BIT(3)),
	AXP_DESC(AXP803, DCDC1, "dcdc1", "vin1", 1600, 3400, 100,
		 AXP803_DCDC1_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL1, BIT(0)),
	AXP_DESC_RANGES(AXP803, DCDC2, "dcdc2", "vin2", axp803_dcdc2_4_ranges,
			76, AXP803_DCDC2_V_OUT, 0x7f, AXP22X_PWR_OUT_CTRL1,
			BIT(1)),
	AXP_DESC_RANGES(AXP803, DCDC3, "dcdc3", "vin3", axp803_dcdc2_4_ranges,
			76, AXP803_DCDC3_V_OUT, 0x7f, AXP22X_PWR_OUT_CTRL1,
			BIT(2)),
	AXP_DESC_RANGES(AXP803, DCDC4, "dcdc4", "vin4", axp803_dcdc2_4_ranges,
			76, AXP803_DCDC4_V_OUT, 0x7f, AXP22X_PWR_OUT_CTRL1,
			BIT(3)),
	AXP_DESC_RANGES(AXP803, DCDC5, "dcdc5", "vin5", axp803_dcdc5_ranges,
			69, AXP803_DCDC5_V_OUT, 0x7f, AXP22X_PWR_OUT_CTRL1,
			BIT(4)),
	AXP_DESC_RANGES(AXP803, DCDC6, "dcdc6", "vin6", axp803_dcdc6_ranges,
			72, AXP803_DCDC6_V_OUT, 0x7f, AXP22X_PWR_OUT_CTRL1,
			BIT(5)),
	AXP_DESC(AXP803, ALDO1, "aldo1", "aldoin", 700, 3300, 100,
		 AXP22X_ALDO1_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL3, BIT(5)),
	AXP_DESC(AXP803, ALDO2, "aldo2", "aldoin", 700, 3300, 100,
		 AXP22X_ALDO2_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL3, BIT(6)),
	AXP_DESC(AXP803, ALDO3, "aldo3", "aldoin", 700, 3300, 100,
		 AXP22X_ALDO3_V_OUT, 0x1f, AXP22X_PWR_OUT_CTRL3, BIT(7)),
	AXP_DESC_IO(AXP803, LDO_IO0, "ldo_io0", "ips", 700, 3300, 100,
		    AXP22X_LDO_IO0_V_OUT, 0x1f, AXP20X_GPIO0_CTRL, 0x07,
		    AXP22X_IO_ENABLED, AXP22X_IO_DISABLED),
	AXP_DESC_IO(AXP803, LDO_IO1, "ldo_io1", "ips", 700, 3300, 100,
		    AXP22X_LDO_IO1_V_OUT, 0x1f, AXP20X_GPIO1_CTRL, 0x07,
		    AXP22X_IO_ENABLED, AXP22X_IO_DISABLED),
	AXP_DESC_FIXED(AXP803, RTC_LDO, "rtc_ldo", "ips", 1800),
	/* secondary switchable output of DCDC1 */
	AXP_DESC_SW(AXP803, DC1SW, "dc1sw", NULL, AXP22X_PWR_OUT_CTRL2,
		    BIT(7)),
};

static const struct regmap_range axp803_writeable_ranges[] = {
	regmap_reg_range(AXP20X_DATACACHE(0), AXP20X_IRQ5_STATE),
	regmap_reg_range(AXP20X_DCDC_MODE, AXP22X_BATLOW_THRES1),
};

static const struct regmap_range axp803_volatile_ranges[] = {
	regmap_reg_range(AXP20X_PWR_INPUT_STATUS, AXP20X_PWR_OP_MODE),
	regmap_reg_range(AXP20X_IRQ1_EN, AXP20X_IRQ5_STATE),
	regmap_reg_range(AXP22X_GPIO_STATE, AXP22X_GPIO_STATE),
	regmap_reg_range(AXP20X_FG_RES, AXP20X_FG_RES),
};

static const struct regmap_access_table axp803_writeable_table = {
	.yes_ranges	= axp803_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp803_writeable_ranges),
};

static const struct regmap_access_table axp803_volatile_table = {
	.yes_ranges	= axp803_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(axp803_volatile_ranges),
};

static const struct regmap_config axp803_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.wr_table	= &axp803_writeable_table,
	.volatile_table	= &axp803_volatile_table,
	.max_register	= AXP22X_BATLOW_THRES1,
	.cache_type	= REGCACHE_RBTREE,
};

#define INIT_REGMAP_IRQ(_irq, _off, _bit)			\
	[AXP809_IRQ_##_irq] = { .reg_offset = (_off), .mask = BIT(_bit) }

static const struct regmap_irq axp803_regmap_irqs[] = {
	INIT_REGMAP_IRQ(PEK_RIS_EDGE,	        4, 6),
	INIT_REGMAP_IRQ(PEK_FAL_EDGE,	        4, 5),
	INIT_REGMAP_IRQ(PEK_SHORT,		4, 4),
	INIT_REGMAP_IRQ(PEK_LONG,		4, 3),
	INIT_REGMAP_IRQ(PEK_OVER_OFF,		4, 2),
	INIT_REGMAP_IRQ(GPIO1_INPUT,		4, 1),
	INIT_REGMAP_IRQ(GPIO0_INPUT,		4, 0),
};

static const struct regmap_irq_chip axp803_regmap_irq_chip = {
	.name			= "axp803",
	.status_base		= AXP20X_IRQ1_STATE,
	.ack_base		= AXP20X_IRQ1_STATE,
	.mask_base		= AXP20X_IRQ1_EN,
	.mask_invert		= true,
	.init_ack_masked	= true,
	.irqs			= axp803_regmap_irqs,
	.num_irqs		= ARRAY_SIZE(axp803_regmap_irqs),
	.num_regs		= 6,
};

static const struct axp_cfg axp803_cfg = {
	.regulators = axp803_regulators,
	.nregulators= ARRAY_SIZE(axp803_regulators),
	.dcdc1_ix = AXP803_DCDC1,
	.dcdc5_ix = -1,
	.dc1sw_ix = AXP803_DC1SW,
	.dc5ldo_ix = -1,
};

static struct axp20x_dev axp803_dev = {
	.regmap_cfg = &axp803_regmap_config,
	.regmap_irq_chip = &axp803_regmap_irq_chip,
};

static int axp803_rsb_probe(struct sunxi_rsb_device *rdev)
{
	return axp_rsb_probe(rdev, &axp803_dev, &axp803_cfg);
}

static const struct of_device_id axp803_of_match[] = {
	{ .compatible = "x-powers,axp803", .data = (void *) AXP803_ID },
	{ },
};
MODULE_DEVICE_TABLE(of, axp803_of_match);

static struct sunxi_rsb_driver axp803_rsb_driver = {
	.driver = {
		.name	= "axp803-rsb",
		.of_match_table	= of_match_ptr(axp803_of_match),
	},
	.probe	= axp803_rsb_probe,
	.remove	= axp_rsb_remove,
};
module_sunxi_rsb_driver(axp803_rsb_driver);

MODULE_DESCRIPTION("AXP803 RSB driver");
MODULE_AUTHOR("Jean-Francois Moine <moinejf@free.fr>");
MODULE_LICENSE("GPL v2");
