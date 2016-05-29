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

static int defx = 800;
static int defy = 600;

module_param(defx, int, 0444);
module_param(defy, int, 0444);
MODULE_PARM_DESC(defx, "default x resolution");
MODULE_PARM_DESC(defy, "default y resolution");

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

static int hibmc_connector_get_modes(struct drm_connector *connector)
{
	int count;

	count = drm_add_modes_noedid(connector, 800, 600);
	drm_set_preferred_mode(connector, defx, defy);
	return count;
}

static int hibmc_connector_mode_valid(struct drm_connector *connector,
				      struct drm_display_mode *mode)
{
	struct hibmc_drm_device *hiprivate =
	 container_of(connector, struct hibmc_drm_device, connector);
	unsigned long size = mode->hdisplay * mode->vdisplay * 4;

	/*
	* Make sure we can fit two framebuffers into video memory.
	* This allows up to 1600x1200 with 16 MB (default size).
	* If you want more try this:
	* 'qemu -vga std -global VGA.vgamem_mb=32 $otherargs'
	*/
	if (size * 2 > hiprivate->fb_size)
		return MODE_BAD;

	return MODE_OK;
}

static struct drm_encoder *
hibmc_connector_best_encoder(struct drm_connector *connector)
{
	int enc_id = connector->encoder_ids[0];

	/* pick the encoder ids */
	if (enc_id)
		return drm_encoder_find(connector->dev, enc_id);

	return NULL;
}

static enum drm_connector_status hibmc_connector_detect(struct drm_connector
						 *connector, bool force)
{
	return connector_status_connected;
}

static const struct drm_connector_helper_funcs
	hibmc_connector_connector_helper_funcs = {
	.get_modes = hibmc_connector_get_modes,
	.mode_valid = hibmc_connector_mode_valid,
	.best_encoder = hibmc_connector_best_encoder,
};

static const struct drm_connector_funcs hibmc_connector_connector_funcs = {
	.dpms = drm_atomic_helper_connector_dpms,
	.detect = hibmc_connector_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = drm_connector_cleanup,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

int hibmc_connector_init(struct hibmc_drm_device *hidev)
{
	struct drm_device *dev = hidev->dev;
	struct drm_connector *connector = &hidev->connector;
	int ret;

	ret = drm_connector_init(dev, connector,
				 &hibmc_connector_connector_funcs,
				 DRM_MODE_CONNECTOR_VIRTUAL);
	if (ret) {
		DRM_ERROR("failed to init connector\n");
		return ret;
	}
	drm_connector_helper_add(connector,
				 &hibmc_connector_connector_helper_funcs);

	return 0;
}

