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

#include <drm/drm_atomic_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drmP.h>

#include "hibmc_drm_drv.h"
#include "hibmc_drm_regs.h"


static void hibmc_encoder_disable(struct drm_encoder *encoder)
{
}

static void hibmc_encoder_enable(struct drm_encoder *encoder)
{
}

static void hibmc_encoder_mode_set(struct drm_encoder *encoder,
				   struct drm_display_mode *mode,
				   struct drm_display_mode *adj_mode)
{
	u32 reg;
	struct drm_device *dev = encoder->dev;
	struct hibmc_drm_device *hidev = dev->dev_private;

	/* just open DISPLAY_CONTROL_HISILE register bit 3:0*/
	reg = readl(hidev->mmio + DISPLAY_CONTROL_HISILE);
	reg |= 0xf;
	writel(reg, hidev->mmio + DISPLAY_CONTROL_HISILE);
}

static int hibmc_encoder_atomic_check(struct drm_encoder *encoder,
				      struct drm_crtc_state *crtc_state,
				      struct drm_connector_state *conn_state)
{
	return 0;
}

static const struct drm_encoder_helper_funcs hibmc_encoder_helper_funcs = {
	.mode_set = hibmc_encoder_mode_set,
	.disable = hibmc_encoder_disable,
	.enable = hibmc_encoder_enable,
	.atomic_check = hibmc_encoder_atomic_check,
};

static const struct drm_encoder_funcs hibmc_encoder_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

int hibmc_encoder_init(struct hibmc_drm_device *hidev)
{
	struct drm_device *dev = hidev->dev;
	struct drm_encoder *encoder = &hidev->encoder;
	int ret;

	encoder->possible_crtcs = 0x1;
	ret = drm_encoder_init(dev, encoder, &hibmc_encoder_encoder_funcs,
			       DRM_MODE_ENCODER_DAC, NULL);
	if (ret) {
		DRM_ERROR("failed to init encoder\n");
		return ret;
	}

	drm_encoder_helper_add(encoder, &hibmc_encoder_helper_funcs);
	return 0;
}

