/*
 * Head file for hi655x regulator
 *
 * Copyright (c) 2015 Hisilicon.
 *
 * Chen Feng <puck.chen@hisilicon.com>
 * Fei  Wang <w.f@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __HISI_HI655X_REGULATOR_H__
#define __HISI_HI655X_REGULATOR_H__

struct hi655x_regulator_ctrl_regs {
	u32  enable_reg;
	u32  disable_reg;
	u32  status_reg;
};

struct hi655x_regulator_ctrl_data {
	int          shift;
	unsigned int val;
};

struct hi655x_regulator_vset_data {
	int          shift;
	unsigned int mask;
};

struct hi655x_regulator {
	struct hi655x_regulator_ctrl_regs ctrl_regs;
	u32 vset_reg;
	u32 ctrl_mask;
	u32 vset_mask;
	struct regulator_desc rdesc;
	struct regulator_dev *regdev;
};

#endif
