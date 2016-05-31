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

#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem_cma_helper.h>

#include "hibmc_drm_drv.h"

/* ---------------------------------------------------------------------- */

static void hibmc_drm_fb_destroy(struct drm_framebuffer *fb)
{
	struct hibmc_drm_device *hidev =
		container_of(fb, struct hibmc_drm_device, fbdev.fb.fb);
	struct drm_gem_object *base = &hidev->fbdev.fb.obj->base;

	if (hidev->fbdev.fb.obj)
		drm_gem_object_unreference_unlocked(base);
	drm_framebuffer_cleanup(fb);
	kfree(fb);
}

static struct drm_framebuffer_funcs hibmc_drm_fb_funcs = {
	.destroy	= hibmc_drm_fb_destroy,
};

static int hibmc_drm_fb_init(struct drm_device *dev,
		      struct hibmc_framebuffer *gfb,
		      struct drm_mode_fb_cmd2 *mode_cmd,
		      struct drm_gem_cma_object *obj)
{
	int ret;

	drm_helper_mode_fill_fb_struct(&gfb->fb, mode_cmd);
	gfb->obj = obj;
	gfb->is_fbdev_fb = true;
	ret = drm_framebuffer_init(dev, &gfb->fb, &hibmc_drm_fb_funcs);
	if (ret) {
		DRM_ERROR("drm_framebuffer_init failed: %d\n", ret);
		return ret;
	}
	return 0;
}

static struct drm_gem_cma_object *hibmc_drm_gem_create_object(
						struct hibmc_drm_device *hidev,
						size_t size)
{
	struct drm_gem_cma_object *cma_obj;
	struct drm_gem_object *gem_obj;
	struct drm_device *drm = hidev->dev;
	int ret;

	cma_obj = devm_kzalloc(drm->dev, sizeof(*cma_obj), GFP_KERNEL);
	if (!cma_obj)
		return ERR_PTR(-ENOMEM);

	gem_obj = &cma_obj->base;

	ret = drm_gem_object_init(drm, gem_obj, size);
	if (ret)
		return ERR_PTR(ret);

	cma_obj->vaddr = hidev->fb_map;
	cma_obj->paddr = hidev->fb_base;
	return cma_obj;
}

static void hibmc_drm_gem_free_object(struct drm_gem_object *gem_obj)
{
	struct drm_gem_cma_object *cma_obj;

	cma_obj = to_drm_gem_cma_obj(gem_obj);
	drm_gem_object_release(gem_obj);
}

static struct fb_ops hibmc_drm_fb_ops = {
	.owner = THIS_MODULE,
	.fb_check_var = drm_fb_helper_check_var,
	.fb_set_par = drm_fb_helper_set_par,
	.fb_fillrect = drm_fb_helper_sys_fillrect,
	.fb_copyarea = drm_fb_helper_sys_copyarea,
	.fb_imageblit = drm_fb_helper_sys_imageblit,
	.fb_pan_display = drm_fb_helper_pan_display,
	.fb_blank = drm_fb_helper_blank,
	.fb_setcmap = drm_fb_helper_setcmap,
};

static int hibmc_drm_fb_probe(struct drm_fb_helper *helper,
			      struct drm_fb_helper_surface_size *sizes)
{
	struct hibmc_drm_device *hidev =
		container_of(helper, struct hibmc_drm_device, fbdev.helper);
	struct drm_device *dev = hidev->dev;
	struct fb_info *info;
	struct drm_framebuffer *fb;
	struct drm_mode_fb_cmd2 mode_cmd;
	struct drm_gem_cma_object *obj = NULL;
	int ret = 0;
	size_t size;
	unsigned int bytes_per_pixel;
	unsigned long offset;

	DRM_DEBUG_DRIVER("surface width(%d), height(%d) and bpp(%d)\n",
			 sizes->surface_width, sizes->surface_height,
			 sizes->surface_bpp);
	sizes->surface_depth = 32;

	bytes_per_pixel = DIV_ROUND_UP(sizes->surface_bpp, 8);

	mode_cmd.width = sizes->surface_width;
	mode_cmd.height = sizes->surface_height;
	mode_cmd.pitches[0] = mode_cmd.width * bytes_per_pixel;
	mode_cmd.pixel_format = drm_mode_legacy_fb_format(sizes->surface_bpp,
							  sizes->surface_depth);

	size = roundup(mode_cmd.pitches[0] * mode_cmd.height, PAGE_SIZE);

	obj = hibmc_drm_gem_create_object(hidev, size);
	if (IS_ERR(obj)) {
		DRM_ERROR("can't allocate gem object\n");
		return -ENOMEM;
	}


	/* init fb device */
	/*info = framebuffer_alloc(0, device);*/
	info = drm_fb_helper_alloc_fbi(helper);
	if (!info) {
		DRM_ERROR("can't allocate fb info\n");
		goto err_drm_gem_cma_free_object;
	}

	ret = hibmc_drm_fb_init(hidev->dev,
				&hidev->fbdev.fb, &mode_cmd, obj);
	if (ret) {
		DRM_ERROR("framebuffer init error\n");
		goto err_drm_gem_cma_free_object;
	}

	/* setup helper */
	fb = &hidev->fbdev.fb.fb;
	hidev->fbdev.helper.fb = fb;
	hidev->fbdev.helper.fbdev = info;

	strcpy(info->fix.id, "hibmcdrmfb");
	info->par = &hidev->fbdev.helper;
	info->flags = FBINFO_DEFAULT;
	info->fbops = &hibmc_drm_fb_ops;

	drm_fb_helper_fill_fix(info, fb->pitches[0], fb->depth);
	drm_fb_helper_fill_var(info, &hidev->fbdev.helper, sizes->fb_width,
			       sizes->fb_height);

	offset = info->var.xoffset * bytes_per_pixel;
	offset += info->var.yoffset * fb->pitches[0];

	dev->mode_config.fb_base = (resource_size_t)obj->paddr;
	info->screen_base = obj->vaddr + offset;
	info->fix.smem_start = (unsigned long)(obj->paddr + offset);
	info->screen_size = size;
	info->fix.smem_len = size;

	return 0;

err_drm_gem_cma_free_object:
	hibmc_drm_gem_free_object(&obj->base);
	return ret;
}

static int hibmc_fbdev_destroy(struct hibmc_drm_device *hidev)
{
	struct hibmc_framebuffer *gfb = &hidev->fbdev.fb;
	struct drm_fb_helper *fbh = &hidev->fbdev.helper;

	DRM_DEBUG_DRIVER("hibmc_fbdev_destroy\n");

	drm_fb_helper_unregister_fbi(fbh);
	drm_fb_helper_release_fbi(fbh);

	drm_framebuffer_unregister_private(&gfb->fb);
	if (gfb->obj) {
		drm_gem_object_unreference_unlocked(&gfb->obj->base);
		gfb->obj = NULL;
	}

	drm_framebuffer_cleanup(&gfb->fb);
	drm_fb_helper_fini(&hidev->fbdev.helper);

	return 0;
}

static const struct drm_fb_helper_funcs hibmc_fbdev_helper_funcs = {
	.fb_probe = hibmc_drm_fb_probe,
};

int hibmc_fbdev_init(struct hibmc_drm_device *hidev)
{
	int ret;
	struct fb_var_screeninfo *var;
	struct fb_fix_screeninfo *fix;

	drm_fb_helper_prepare(hidev->dev, &hidev->fbdev.helper,
			      &hibmc_fbdev_helper_funcs);

	/* Now just one crtc and one channel */
	ret = drm_fb_helper_init(hidev->dev,
				 &hidev->fbdev.helper, 1, 1);
	if (ret)
		return ret;

	ret = drm_fb_helper_single_add_all_connectors(&hidev->fbdev.helper);
	if (ret)
		goto fini;

	drm_helper_disable_unused_functions(hidev->dev);

	ret = drm_fb_helper_initial_config(&hidev->fbdev.helper, 16);
	if (ret)
		goto fini;

	hidev->fbdev.initialized = true;

	var = &hidev->fbdev.helper.fbdev->var;
	fix = &hidev->fbdev.helper.fbdev->fix;

	DRM_DEBUG("Member of info->var is :\n"
		 "xres=%d\n"
		 "yres=%d\n"
		 "xres_virtual=%d\n"
		 "yres_virtual=%d\n"
		 "xoffset=%d\n"
		 "yoffset=%d\n"
		 "bits_per_pixel=%d\n"
		 "...\n", var->xres, var->yres, var->xres_virtual,
		 var->yres_virtual, var->xoffset, var->yoffset,
		 var->bits_per_pixel);
	DRM_DEBUG("Member of info->fix is :\n"
		 "smem_start=%lx\n"
		 "smem_len=%d\n"
		 "type=%d\n"
		 "type_aux=%d\n"
		 "visual=%d\n"
		 "xpanstep=%d\n"
		 "ypanstep=%d\n"
		 "ywrapstep=%d\n"
		 "line_length=%d\n"
		 "accel=%d\n"
		 "capabilities=%d\n"
		 "...\n", fix->smem_start, fix->smem_len, fix->type,
		 fix->type_aux, fix->visual, fix->xpanstep,
		 fix->ypanstep, fix->ywrapstep, fix->line_length,
		 fix->accel, fix->capabilities);

	return 0;

fini:
	drm_fb_helper_fini(&hidev->fbdev.helper);
	return ret;
}

void hibmc_fbdev_fini(struct hibmc_drm_device *hidev)
{
	if (!hidev->fbdev.initialized)
		return;

	hibmc_fbdev_destroy(hidev);
	hidev->fbdev.initialized = false;
}
