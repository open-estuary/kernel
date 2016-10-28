/* Hisilicon Hibmc SoC drm driver
 *
 * Based on the bochs drm driver.
 *
 * Copyright (c) 2016 Huawei Limited.
 *
 * Author:
 *	Rongrong Zou <zourongrong@huawei.com>
 *	Rongrong Zou <zourongrong@gmail.com>
 *	Jianhua Li <lijianhua@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef HIBMC_DRM_DRV_H
#define HIBMC_DRM_DRV_H

#include <drm/drmP.h>
#include <drm/drm_fb_helper.h>
#include <drm/ttm/ttm_bo_driver.h>
#include <drm/drm_gem.h>

struct hibmc_framebuffer {
	struct drm_framebuffer fb;
	struct drm_gem_object *obj;
	bool is_fbdev_fb;
};

struct hibmc_fbdev {
	struct drm_fb_helper helper;
	struct hibmc_framebuffer *fb;
	int size;
};

struct hibmc_drm_device {
	/* hw */
	void __iomem   *mmio;
	void __iomem   *fb_map;
	unsigned long  fb_base;
	unsigned long  fb_size;

	/* drm */
	struct drm_device  *dev;
	struct drm_plane plane;
	struct drm_crtc crtc;
	struct drm_encoder encoder;
	struct drm_connector connector;
	bool mode_config_initialized;

	/* ttm */
	struct {
		struct drm_global_reference mem_global_ref;
		struct ttm_bo_global_ref bo_global_ref;
		struct ttm_bo_device bdev;
		bool initialized;
	} ttm;

	/* fbdev */
	struct hibmc_fbdev *fbdev;
	bool mm_inited;
};

#define to_hibmc_framebuffer(x) container_of(x, struct hibmc_framebuffer, fb)

struct hibmc_bo {
	struct ttm_buffer_object bo;
	struct ttm_placement placement;
	struct ttm_bo_kmap_obj kmap;
	struct drm_gem_object gem;
	struct ttm_place placements[3];
	int pin_count;
};

static inline struct hibmc_bo *hibmc_bo(struct ttm_buffer_object *bo)
{
	return container_of(bo, struct hibmc_bo, bo);
}

static inline struct hibmc_bo *gem_to_hibmc_bo(struct drm_gem_object *gem)
{
	return container_of(gem, struct hibmc_bo, gem);
}

#define DRM_FILE_PAGE_OFFSET (0x100000000ULL >> PAGE_SHIFT)

int hibmc_plane_init(struct hibmc_drm_device *hidev);
int hibmc_crtc_init(struct hibmc_drm_device *hidev);
int hibmc_encoder_init(struct hibmc_drm_device *hidev);
int hibmc_connector_init(struct hibmc_drm_device *hidev);
int hibmc_fbdev_init(struct hibmc_drm_device *hidev);
void hibmc_fbdev_fini(struct hibmc_drm_device *hidev);

int hibmc_gem_create(struct drm_device *dev, u32 size, bool iskernel,
		     struct drm_gem_object **obj);
struct hibmc_framebuffer *
hibmc_framebuffer_init(struct drm_device *dev,
		       const struct drm_mode_fb_cmd2 *mode_cmd,
		       struct drm_gem_object *obj);

int hibmc_mm_init(struct hibmc_drm_device *hibmc);
void hibmc_mm_fini(struct hibmc_drm_device *hibmc);
int hibmc_bo_pin(struct hibmc_bo *bo, u32 pl_flag, u64 *gpu_addr);
void hibmc_gem_free_object(struct drm_gem_object *obj);
int hibmc_dumb_create(struct drm_file *file, struct drm_device *dev,
		      struct drm_mode_create_dumb *args);
int hibmc_dumb_mmap_offset(struct drm_file *file, struct drm_device *dev,
			   u32 handle, u64 *offset);
int hibmc_mmap(struct file *filp, struct vm_area_struct *vma);

extern const struct drm_mode_config_funcs hibmc_mode_funcs;

#endif
