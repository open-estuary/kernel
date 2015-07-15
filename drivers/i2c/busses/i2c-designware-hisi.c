/*
 * Hisilicon I2C bus reset implementaion
 * Copyright (c) 2015, Hisilicon Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include "i2c-designware-core.h"

static void dw_i2c_hisi_reset(struct dw_i2c_dev *dev)
{
	struct dw_i2c_reset_cfg *cfg = &dev->reset_cfg;
	struct regmap *sysreg = cfg->sysreg;
	int id = cfg->id;
	int state;

	regmap_update_bits(sysreg, cfg->reset, BIT(id), BIT(id));
	regmap_read(sysreg, cfg->state, &state);
	if (state & BIT(id))
		regmap_update_bits(sysreg, cfg->dreset, BIT(id), BIT(id));
	else
		dev_err(dev->dev, "i2c reset failed\n");
}

int i2c_dw_reset_init(struct dw_i2c_dev *dev)
{
	struct device_node *np = dev->dev->of_node;
	struct dw_i2c_reset_cfg *reset_cfg = &dev->reset_cfg;
	struct regmap *sysreg;
	int cfg[4];
	int ret;

	sysreg = syscon_regmap_lookup_by_phandle(np, "hisilicon,sysreg-phandle");
	if (IS_ERR(sysreg))
		return PTR_ERR(sysreg);

	ret = of_property_read_u32_array(np, "hisilicon,reset-cfg", cfg, 4);
	if (ret)
		return ret;

	reset_cfg->sysreg = sysreg;
	reset_cfg->id = cfg[0];
	reset_cfg->reset = cfg[1];
	reset_cfg->state = cfg[2];
	reset_cfg->dreset = cfg[3];
	dev->dw_i2c_reset = dw_i2c_hisi_reset;
	return 0;
}

MODULE_AUTHOR("Kefeng Wang <wangkefeng.wang@huawei.com>");
MODULE_DESCRIPTION("Hisilicon I2C reset driver");
MODULE_LICENSE("GPL v2");
