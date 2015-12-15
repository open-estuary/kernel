/*
 * Device driver for regulators in hi655x IC
 *
 * Copyright (c) 2016 Hisilicon.
 *
 * Chen Feng <puck.chen@hisilicon.com>
 * Fei  Wang <w.f@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/bitops.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/mfd/hi655x-pmic.h>

struct hi655x_regulator {
	unsigned int disable_reg;
	unsigned int status_reg;
	unsigned int ctrl_regs;
	unsigned int ctrl_mask;
	struct regulator_desc rdesc;
};

/* LDO7 & LDO10 */
static const unsigned int ldo7_voltages[] = {
	1800000, 1850000, 2850000, 2900000,
	3000000, 3100000, 3200000, 3300000,
};

static const unsigned int ldo19_voltages[] = {
	1800000, 1850000, 1900000, 1750000,
	2800000, 2850000, 2900000, 3000000,
};

static const unsigned int ldo22_voltages[] = {
	 900000, 1000000, 1050000, 1100000,
	1150000, 1175000, 1185000, 1200000,
};

enum hi655x_regulator_id {
	HI655X_LDO0,
	HI655X_LDO1,
	HI655X_LDO2,
	HI655X_LDO3,
	HI655X_LDO4,
	HI655X_LDO5,
	HI655X_LDO6,
	HI655X_LDO7,
	HI655X_LDO8,
	HI655X_LDO9,
	HI655X_LDO10,
	HI655X_LDO11,
	HI655X_LDO12,
	HI655X_LDO13,
	HI655X_LDO14,
	HI655X_LDO15,
	HI655X_LDO16,
	HI655X_LDO17,
	HI655X_LDO18,
	HI655X_LDO19,
	HI655X_LDO20,
	HI655X_LDO21,
	HI655X_LDO22,
};

static int hi655x_is_enabled(struct regulator_dev *rdev)
{
	unsigned int value = 0;

	struct hi655x_regulator *regulator = rdev_get_drvdata(rdev);

	regmap_read(rdev->regmap, regulator->status_reg, &value);
	return (value & BIT(regulator->ctrl_mask));
}

static int hi655x_disable(struct regulator_dev *rdev)
{
	int ret = 0;

	struct hi655x_regulator *regulator = rdev_get_drvdata(rdev);

	ret = regmap_write(rdev->regmap, regulator->disable_reg,
			   BIT(regulator->ctrl_mask));
	return ret;
}

static struct regulator_ops hi655x_regulator_ops = {
	.enable = regulator_enable_regmap,
	.disable = hi655x_disable,
	.is_enabled = hi655x_is_enabled,
	.list_voltage = regulator_list_voltage_table,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
};

static struct regulator_ops hi655x_ldo_linear_ops = {
	.enable = regulator_enable_regmap,
	.disable = hi655x_disable,
	.is_enabled = hi655x_is_enabled,
	.list_voltage = regulator_list_voltage_linear,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
};

#define HI655X_LDO(_ID, vreg, vmask, ereg, dreg,                \
		   sreg, cmask, vtable) {                       \
	.rdesc = {                                              \
		.name           = #_ID,                         \
		.ops            = &hi655x_regulator_ops,        \
		.type           = REGULATOR_VOLTAGE,            \
		.id             = HI655X_##_ID,                 \
		.owner          = THIS_MODULE,                  \
		.n_voltages     = ARRAY_SIZE(vtable),           \
		.volt_table     = vtable,                       \
		.vsel_reg       = HI655X_BUS_ADDR(vreg),        \
		.vsel_mask      = vmask,                        \
		.enable_reg     = HI655X_BUS_ADDR(ereg),        \
		.enable_mask    = cmask,                        \
	},                                                      \
	.disable_reg = HI655X_BUS_ADDR(dreg),                   \
	.status_reg = HI655X_BUS_ADDR(sreg),                    \
	.ctrl_mask = cmask,                                     \
}

#define HI655X_LDO_LINEAR(_ID, vreg, vmask, ereg, dreg,         \
			  sreg, cmask, minv, nvolt, vstep) {    \
	.rdesc = {                                              \
		.name           = #_ID,                         \
		.ops            = &hi655x_ldo_linear_ops,       \
		.type           = REGULATOR_VOLTAGE,            \
		.id             = HI655X_##_ID,                 \
		.owner          = THIS_MODULE,                  \
		.min_uV         = minv,                         \
		.n_voltages     = nvolt,                        \
		.uV_step        = vstep,                        \
		.uV_step        = vstep,                        \
		.vsel_reg       = HI655X_BUS_ADDR(vreg),        \
		.vsel_mask      = vmask,                        \
		.enable_reg     = HI655X_BUS_ADDR(ereg),        \
		.enable_mask    = cmask,                        \
	},                                                      \
	.disable_reg = HI655X_BUS_ADDR(dreg),                   \
	.status_reg = HI655X_BUS_ADDR(sreg),                    \
	.ctrl_mask = cmask,                                     \
}

static struct hi655x_regulator regulators[] = {
	HI655X_LDO_LINEAR(LDO2, 0x72, 0x07, 0x29, 0x2a, 0x2b, 0x01,
			  2500000, 8, 100000),
	HI655X_LDO(LDO7, 0x78, 0x07, 0x29, 0x2a, 0x2b, 0x06, ldo7_voltages),
	HI655X_LDO(LDO10, 0x78, 0x07, 0x29, 0x2a, 0x2b, 0x01, ldo7_voltages),
	HI655X_LDO_LINEAR(LDO13, 0x7e, 0x07, 0x2c, 0x2d, 0x2e, 0x04,
			  1600000, 8, 50000),
	HI655X_LDO_LINEAR(LDO14, 0x7f, 0x07, 0x2c, 0x2d, 0x2e, 0x05,
			  2500000, 8, 100000),
	HI655X_LDO_LINEAR(LDO15, 0x80, 0x07, 0x2c, 0x2d, 0x2e, 0x06,
			  1600000, 8, 50000),
	HI655X_LDO_LINEAR(LDO17, 0x82, 0x07, 0x2f, 0x30, 0x31, 0x00,
			  2500000, 8, 100000),
	HI655X_LDO(LDO19, 0x84, 0x07, 0x2f, 0x30, 0x31, 0x02, ldo19_voltages),
	HI655X_LDO_LINEAR(LDO21, 0x86, 0x07, 0x2f, 0x30, 0x31, 0x04,
			  1650000, 8, 50000),
	HI655X_LDO(LDO22, 0x87, 0x07, 0x2f, 0x30, 0x31, 0x05, ldo22_voltages),
};

static struct of_regulator_match hi655x_regulator_match[] = {
	{ .name	= "ldo2", .driver_data = (void *)HI655X_LDO0, },
	{ .name	= "ldo7", .driver_data = (void *)HI655X_LDO7, },
	{ .name	= "ldo10", .driver_data = (void *)HI655X_LDO10, },
	{ .name	= "ldo13", .driver_data = (void *)HI655X_LDO13, },
	{ .name	= "ldo14", .driver_data = (void *)HI655X_LDO14, },
	{ .name = "ldo15", .driver_data = (void *)HI655X_LDO15, },
	{ .name	= "ldo17", .driver_data = (void *)HI655X_LDO17, },
	{ .name	= "ldo19", .driver_data = (void *)HI655X_LDO19, },
	{ .name	= "ldo21", .driver_data = (void *)HI655X_LDO21, },
	{ .name	= "ldo22", .driver_data = (void *)HI655X_LDO22, },
};

static int hi655x_regulator_probe(struct platform_device *pdev)
{
	unsigned int i;
	int ret;
	struct hi655x_regulator *regulator;
	struct hi655x_pmic *pmic;
	struct regulator_config config = { };
	struct device *dev = &pdev->dev;
	struct regulator_dev *rdev;
	struct device_node *np;

	np = of_get_child_by_name(dev->parent->of_node, "regulators");
	if (!np)
		return -ENODEV;

	ret = of_regulator_match(dev, np,
				 hi655x_regulator_match,
				 ARRAY_SIZE(hi655x_regulator_match));
	of_node_put(np);
	if (ret < 0) {
		dev_err(dev, "Error parsing regulator init data: %d\n", ret);
		return ret;
	}

	pmic = dev_get_drvdata(pdev->dev.parent);
	if (!pmic) {
		dev_err(dev, "no pmic in the regulator parent node\n");
		return -ENODEV;
	}

	regulator = devm_kzalloc(dev, sizeof(*regulator), GFP_KERNEL);
	if (!regulator)
		return -ENOMEM;

	platform_set_drvdata(pdev, regulator);

	for (i = 0; i < ARRAY_SIZE(regulators); i++) {
		config.dev = &pdev->dev;
		config.driver_data = regulator;
		config.regmap = pmic->regmap;
		config.of_node = hi655x_regulator_match[i].of_node;
		config.init_data = hi655x_regulator_match[i].init_data;

		/* register regulator with framework */
		rdev = devm_regulator_register(&pdev->dev,
					       &regulators[i].rdesc,
					       &config);
		if (IS_ERR(rdev)) {
			dev_err(&pdev->dev, "failed to register regulator %s\n",
				regulator->rdesc.name);
			return PTR_ERR(rdev);
		}
	}
	return 0;
}

static struct platform_driver hi655x_regulator_driver = {
	.driver = {
		.name	= "hi655x-regulator",
	},
	.probe	= hi655x_regulator_probe,
};
module_platform_driver(hi655x_regulator_driver);

MODULE_AUTHOR("Chen Feng <puck.chen@hisilicon.com>");
MODULE_DESCRIPTION("Hisilicon hi655x regulator driver");
MODULE_LICENSE("GPL v2");
