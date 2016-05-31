/* Hisilicon Hibmc SoC drm driver
 *
 * Based on the bochs drm driver.
 *
 * Copyright (c) 2016 Huawei Limited.
 *
 * Author:
 *	Rongrong Zou <zourongrong@huawei>
 *	Rongrong Zou <zourongrong@gmail.com>
 *	Jianhua Li <lijianhua@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <linux/io.h>
#include <drm/drmP.h>
#include <drm/drm_fb_helper.h>
#include "hibmc_drm_drv.h"
#include "hibmc_drm_regs.h"

/*
 * It can operate in one of three modes: 0, 1 or Sleep.
 */
void hibmc_set_power_mode(struct hibmc_drm_device *hidev,
				 unsigned int power_mode)
{
	unsigned int control_value = 0;
	void __iomem   *mmio = hidev->mmio;

	if (power_mode > HIBMC_PW_MODE_CTL_MODE_SLEEP)
		return;

	control_value = readl(mmio + HIBMC_POWER_MODE_CTRL);
	control_value &= ~HIBMC_PW_MODE_CTL_MODE_MASK;

	control_value |= HIBMC_PW_MODE_CTL_MODE(power_mode) &
			 HIBMC_PW_MODE_CTL_MODE_MASK;


    /* Set up other fields in Power Control Register */
	if (power_mode == HIBMC_PW_MODE_CTL_MODE_SLEEP) {
		control_value &= ~HIBMC_PW_MODE_CTL_OSC_INPUT_MASK;
		control_value |= HIBMC_PW_MODE_CTL_OSC_INPUT(0) &
				 HIBMC_PW_MODE_CTL_OSC_INPUT_MASK;
	} else {
		control_value &= ~HIBMC_PW_MODE_CTL_OSC_INPUT_MASK;
		control_value |= HIBMC_PW_MODE_CTL_OSC_INPUT(1) &
				 HIBMC_PW_MODE_CTL_OSC_INPUT_MASK;

	}
    /* Program new power mode. */
	writel(control_value, mmio + HIBMC_POWER_MODE_CTRL);
}

static unsigned int hibmc_get_power_mode(struct hibmc_drm_device *hidev)
{
	void __iomem   *mmio = hidev->mmio;

	return (readl(mmio + HIBMC_POWER_MODE_CTRL)&
		HIBMC_PW_MODE_CTL_MODE_MASK) >> HIBMC_PW_MODE_CTL_MODE_SHIFT;
}

void hibmc_set_current_gate(struct hibmc_drm_device *hidev, unsigned int gate)
{
	unsigned int gate_reg;
	unsigned int mode;
	void __iomem   *mmio = hidev->mmio;

	/* Get current power mode. */
	mode = hibmc_get_power_mode(hidev);

	switch (mode) {
	case HIBMC_PW_MODE_CTL_MODE_MODE0:
		gate_reg = HIBMC_MODE0_GATE;
		break;

	case HIBMC_PW_MODE_CTL_MODE_MODE1:
		gate_reg = HIBMC_MODE1_GATE;
		break;

	default:
		gate_reg = HIBMC_MODE0_GATE;
		break;
	}
	writel(gate, mmio + gate_reg);
}

