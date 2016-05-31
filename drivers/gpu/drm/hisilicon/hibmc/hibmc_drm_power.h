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

#ifndef HIBMC_DRM_POWER_H
#define HIBMC_DRM_POWER_H

#include "hibmc_drm_drv.h"

extern void hibmc_set_power_mode(struct hibmc_drm_device *hidev,
				 unsigned int power_mode);
extern void hibmc_set_current_gate(struct hibmc_drm_device *hidev,
				   unsigned int gate);
#endif
