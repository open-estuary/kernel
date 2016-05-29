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

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drmP.h>

#include "hibmc_drm_drv.h"
#include "hibmc_drm_regs.h"
#include "hibmc_drm_power.h"

/* ---------------------------------------------------------------------- */

static int hibmc_plane_prepare_fb(struct drm_plane *plane,
		       const struct drm_plane_state *new_state)
{
	/* do nothing */
	return 0;
}

static void hibmc_plane_cleanup_fb(struct drm_plane *plane,
				   const struct drm_plane_state *old_state)
{
	/* do nothing */
}

static int hibmc_plane_atomic_check(struct drm_plane *plane,
				    struct drm_plane_state *state)
{
	struct drm_framebuffer *fb = state->fb;
	struct drm_crtc *crtc = state->crtc;
	struct drm_crtc_state *crtc_state;
	u32 src_x = state->src_x >> 16;
	u32 src_y = state->src_y >> 16;
	u32 src_w = state->src_w >> 16;
	u32 src_h = state->src_h >> 16;
	int crtc_x = state->crtc_x;
	int crtc_y = state->crtc_y;
	u32 crtc_w = state->crtc_w;
	u32 crtc_h = state->crtc_h;

	if (!crtc || !fb)
		return 0;

	crtc_state = drm_atomic_get_crtc_state(state->state, crtc);
	if (IS_ERR(crtc_state))
		return PTR_ERR(crtc_state);

	if (src_w != crtc_w || src_h != crtc_h) {
		DRM_ERROR("Scale not support!!!\n");
		return -EINVAL;
	}

	if (src_x + src_w > fb->width ||
	    src_y + src_h > fb->height)
		return -EINVAL;

	if (crtc_x < 0 || crtc_y < 0)
		return -EINVAL;

	if (crtc_x + crtc_w > crtc_state->adjusted_mode.hdisplay ||
	    crtc_y + crtc_h > crtc_state->adjusted_mode.vdisplay)
		return -EINVAL;

	return 0;
}

static void hibmc_plane_atomic_update(struct drm_plane *plane,
				      struct drm_plane_state *old_state)
{
	struct drm_plane_state	*state	= plane->state;
	u32 reg;
	unsigned int line_l;
	struct hibmc_drm_device *hidev =
		(struct hibmc_drm_device *)plane->dev->dev_private;

	/* now just support one plane */
	writel(0, hidev->mmio + HIBMC_CRT_FB_ADDRESS);
	reg = state->fb->width * (state->fb->bits_per_pixel >> 3);
	/* now line_pad is 16 */
	reg = PADDING(16, reg);

	line_l = state->fb->width * state->fb->bits_per_pixel / 8;
	line_l = PADDING(16, line_l);
	writel((HIBMC_CRT_FB_WIDTH_WIDTH(reg) & HIBMC_CRT_FB_WIDTH_WIDTH_MASK) |
	       (HIBMC_CRT_FB_WIDTH_OFFS(line_l) & HIBMC_CRT_FB_WIDTH_OFFS_MASK),
	       hidev->mmio + HIBMC_CRT_FB_WIDTH);

	/* SET PIXEL FORMAT */
	reg = readl(hidev->mmio + HIBMC_CRT_DISP_CTL);
	reg = reg & ~HIBMC_CRT_DISP_CTL_FORMAT_MASK;
	reg = reg | (HIBMC_CRT_DISP_CTL_FORMAT(state->fb->bits_per_pixel >> 4) &
		     HIBMC_CRT_DISP_CTL_FORMAT_MASK);
	writel(reg, hidev->mmio + HIBMC_CRT_DISP_CTL);
}

static void hibmc_plane_atomic_disable(struct drm_plane *plane,
				       struct drm_plane_state *old_state)
{
}

static const u32 channel_formats1[] = {
	DRM_FORMAT_RGB565, DRM_FORMAT_BGR565, DRM_FORMAT_RGB888,
	DRM_FORMAT_BGR888, DRM_FORMAT_XRGB8888, DRM_FORMAT_XBGR8888,
	DRM_FORMAT_RGBA8888, DRM_FORMAT_BGRA8888, DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ABGR8888
};

static struct drm_plane_funcs hibmc_plane_funcs = {
	.update_plane	= drm_atomic_helper_update_plane,
	.disable_plane	= drm_atomic_helper_disable_plane,
	.set_property = drm_atomic_helper_plane_set_property,
	.destroy = drm_plane_cleanup,
	.reset = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
};

static const struct drm_plane_helper_funcs hibmc_plane_helper_funcs = {
	.prepare_fb = hibmc_plane_prepare_fb,
	.cleanup_fb = hibmc_plane_cleanup_fb,
	.atomic_check = hibmc_plane_atomic_check,
	.atomic_update = hibmc_plane_atomic_update,
	.atomic_disable = hibmc_plane_atomic_disable,
};

int hibmc_plane_init(struct hibmc_drm_device *hidev)
{
	struct drm_device *dev = hidev->dev;
	struct drm_plane *plane = &hidev->plane;
	int ret = 0;

	/*
	 * plane init
	 * TODO: Now only support primary plane, overlay planes
	 * need to do.
	 */
	ret = drm_universal_plane_init(dev, plane, 1, &hibmc_plane_funcs,
				       channel_formats1,
				       ARRAY_SIZE(channel_formats1),
				       DRM_PLANE_TYPE_PRIMARY,
				       NULL);
	if (ret) {
		DRM_ERROR("fail to init plane!!!\n");
		return ret;
	}

	drm_plane_helper_add(plane, &hibmc_plane_helper_funcs);
	return 0;
}

