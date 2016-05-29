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

#ifndef HIBMC_DRM_DE_H
#define HIBMC_DRM_DE_H

struct panel_pll {
	unsigned long M;
	unsigned long N;
	unsigned long OD;
	unsigned long POD;
};

#endif
