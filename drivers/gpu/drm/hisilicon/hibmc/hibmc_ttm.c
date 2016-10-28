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

#include "hibmc_drm_drv.h"
#include <ttm/ttm_page_alloc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_atomic_helper.h>

static inline struct hibmc_drm_device *
hibmc_bdev(struct ttm_bo_device *bd)
{
	return container_of(bd, struct hibmc_drm_device, ttm.bdev);
}

static int
hibmc_ttm_mem_global_init(struct drm_global_reference *ref)
{
	return ttm_mem_global_init(ref->object);
}

static void
hibmc_ttm_mem_global_release(struct drm_global_reference *ref)
{
	ttm_mem_global_release(ref->object);
}

static int hibmc_ttm_global_init(struct hibmc_drm_device *hibmc)
{
	struct drm_global_reference *global_ref;
	int r;

	global_ref = &hibmc->ttm.mem_global_ref;
	global_ref->global_type = DRM_GLOBAL_TTM_MEM;
	global_ref->size = sizeof(struct ttm_mem_global);
	global_ref->init = &hibmc_ttm_mem_global_init;
	global_ref->release = &hibmc_ttm_mem_global_release;
	r = drm_global_item_ref(global_ref);
	if (r != 0) {
		DRM_ERROR("Failed setting up TTM memory accounting subsystem.\n"
			 );
		return r;
	}

	hibmc->ttm.bo_global_ref.mem_glob =
		hibmc->ttm.mem_global_ref.object;
	global_ref = &hibmc->ttm.bo_global_ref.ref;
	global_ref->global_type = DRM_GLOBAL_TTM_BO;
	global_ref->size = sizeof(struct ttm_bo_global);
	global_ref->init = &ttm_bo_global_init;
	global_ref->release = &ttm_bo_global_release;
	r = drm_global_item_ref(global_ref);
	if (r != 0) {
		DRM_ERROR("Failed setting up TTM BO subsystem.\n");
		drm_global_item_unref(&hibmc->ttm.mem_global_ref);
		return r;
	}
	return 0;
}

static void
hibmc_ttm_global_release(struct hibmc_drm_device *hibmc)
{
	if (!hibmc->ttm.mem_global_ref.release)
		return;

	drm_global_item_unref(&hibmc->ttm.bo_global_ref.ref);
	drm_global_item_unref(&hibmc->ttm.mem_global_ref);
	hibmc->ttm.mem_global_ref.release = NULL;
}

static void hibmc_bo_ttm_destroy(struct ttm_buffer_object *tbo)
{
	struct hibmc_bo *bo;

	bo = container_of(tbo, struct hibmc_bo, bo);

	drm_gem_object_release(&bo->gem);
	kfree(bo);
}

static bool hibmc_ttm_bo_is_hibmc_bo(struct ttm_buffer_object *bo)
{
	if (bo->destroy == &hibmc_bo_ttm_destroy)
		return true;
	return false;
}

static int
hibmc_bo_init_mem_type(struct ttm_bo_device *bdev, u32 type,
		       struct ttm_mem_type_manager *man)
{
	switch (type) {
	case TTM_PL_SYSTEM:
		man->flags = TTM_MEMTYPE_FLAG_MAPPABLE;
		man->available_caching = TTM_PL_MASK_CACHING;
		man->default_caching = TTM_PL_FLAG_CACHED;
		break;
	case TTM_PL_VRAM:
		man->func = &ttm_bo_manager_func;
		man->flags = TTM_MEMTYPE_FLAG_FIXED |
			TTM_MEMTYPE_FLAG_MAPPABLE;
		man->available_caching = TTM_PL_FLAG_UNCACHED |
			TTM_PL_FLAG_WC;
		man->default_caching = TTM_PL_FLAG_WC;
		break;
	default:
		DRM_ERROR("Unsupported memory type %u\n", type);
		return -EINVAL;
	}
	return 0;
}

void hibmc_ttm_placement(struct hibmc_bo *bo, int domain)
{
	u32 c = 0;
	u32 i;

	bo->placement.placement = bo->placements;
	bo->placement.busy_placement = bo->placements;
	if (domain & TTM_PL_FLAG_VRAM)
		bo->placements[c++].flags = TTM_PL_FLAG_WC |
		TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_VRAM;
	if (domain & TTM_PL_FLAG_SYSTEM)
		bo->placements[c++].flags = TTM_PL_MASK_CACHING |
		TTM_PL_FLAG_SYSTEM;
	if (!c)
		bo->placements[c++].flags = TTM_PL_MASK_CACHING |
		TTM_PL_FLAG_SYSTEM;

	bo->placement.num_placement = c;
	bo->placement.num_busy_placement = c;
	for (i = 0; i < c; ++i) {
		bo->placements[i].fpfn = 0;
		bo->placements[i].lpfn = 0;
	}
}

static void
hibmc_bo_evict_flags(struct ttm_buffer_object *bo, struct ttm_placement *pl)
{
	struct hibmc_bo *hibmcbo = hibmc_bo(bo);

	if (!hibmc_ttm_bo_is_hibmc_bo(bo))
		return;

	hibmc_ttm_placement(hibmcbo, TTM_PL_FLAG_SYSTEM);
	*pl = hibmcbo->placement;
}

static int hibmc_bo_verify_access(struct ttm_buffer_object *bo,
				  struct file *filp)
{
	struct hibmc_bo *hibmcbo = hibmc_bo(bo);

	return drm_vma_node_verify_access(&hibmcbo->gem.vma_node,
					  filp->private_data);
}

static int hibmc_ttm_io_mem_reserve(struct ttm_bo_device *bdev,
				    struct ttm_mem_reg *mem)
{
	struct ttm_mem_type_manager *man = &bdev->man[mem->mem_type];
	struct hibmc_drm_device *hibmc = hibmc_bdev(bdev);

	mem->bus.addr = NULL;
	mem->bus.offset = 0;
	mem->bus.size = mem->num_pages << PAGE_SHIFT;
	mem->bus.base = 0;
	mem->bus.is_iomem = false;
	if (!(man->flags & TTM_MEMTYPE_FLAG_MAPPABLE))
		return -EINVAL;
	switch (mem->mem_type) {
	case TTM_PL_SYSTEM:
		/* system memory */
		return 0;
	case TTM_PL_VRAM:
		mem->bus.offset = mem->start << PAGE_SHIFT;
		mem->bus.base = pci_resource_start(hibmc->dev->pdev, 0);
		mem->bus.is_iomem = true;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void hibmc_ttm_io_mem_free(struct ttm_bo_device *bdev,
				  struct ttm_mem_reg *mem)
{
}

static void hibmc_ttm_backend_destroy(struct ttm_tt *tt)
{
	ttm_tt_fini(tt);
	kfree(tt);
}

static struct ttm_backend_func hibmc_tt_backend_func = {
	.destroy = &hibmc_ttm_backend_destroy,
};

static struct ttm_tt *hibmc_ttm_tt_create(struct ttm_bo_device *bdev,
					  unsigned long size,
					  u32 page_flags,
					  struct page *dummy_read_page)
{
	struct ttm_tt *tt;

	tt = kzalloc(sizeof(*tt), GFP_KERNEL);
	if (!tt)
		return NULL;
	tt->func = &hibmc_tt_backend_func;
	if (ttm_tt_init(tt, bdev, size, page_flags, dummy_read_page)) {
		kfree(tt);
		return NULL;
	}
	return tt;
}

static int hibmc_ttm_tt_populate(struct ttm_tt *ttm)
{
	return ttm_pool_populate(ttm);
}

static void hibmc_ttm_tt_unpopulate(struct ttm_tt *ttm)
{
	ttm_pool_unpopulate(ttm);
}

struct ttm_bo_driver hibmc_bo_driver = {
	.ttm_tt_create		= hibmc_ttm_tt_create,
	.ttm_tt_populate	= hibmc_ttm_tt_populate,
	.ttm_tt_unpopulate	= hibmc_ttm_tt_unpopulate,
	.init_mem_type		= hibmc_bo_init_mem_type,
	.evict_flags		= hibmc_bo_evict_flags,
	.move			= NULL,
	.verify_access		= hibmc_bo_verify_access,
	.io_mem_reserve		= &hibmc_ttm_io_mem_reserve,
	.io_mem_free		= &hibmc_ttm_io_mem_free,
	.lru_tail		= &ttm_bo_default_lru_tail,
	.swap_lru_tail		= &ttm_bo_default_swap_lru_tail,
};

int hibmc_mm_init(struct hibmc_drm_device *hibmc)
{
	int ret;
	struct drm_device *dev = hibmc->dev;
	struct ttm_bo_device *bdev = &hibmc->ttm.bdev;

	ret = hibmc_ttm_global_init(hibmc);
	if (ret)
		return ret;

	ret = ttm_bo_device_init(&hibmc->ttm.bdev,
				 hibmc->ttm.bo_global_ref.ref.object,
				 &hibmc_bo_driver,
				 dev->anon_inode->i_mapping,
				 DRM_FILE_PAGE_OFFSET,
				 true);
	if (ret) {
		DRM_ERROR("Error initialising bo driver; %d\n", ret);
		return ret;
	}

	ret = ttm_bo_init_mm(bdev, TTM_PL_VRAM,
			     hibmc->fb_size >> PAGE_SHIFT);
	if (ret) {
		DRM_ERROR("Failed ttm VRAM init: %d\n", ret);
		return ret;
	}

	hibmc->mm_inited = true;
	return 0;
}

void hibmc_mm_fini(struct hibmc_drm_device *hibmc)
{
	if (!hibmc->mm_inited)
		return;

	ttm_bo_device_release(&hibmc->ttm.bdev);
	hibmc_ttm_global_release(hibmc);
	hibmc->mm_inited = false;
}

int hibmc_bo_create(struct drm_device *dev, int size, int align,
		    u32 flags, struct hibmc_bo **phibmcbo)
{
	struct hibmc_drm_device *hibmc = dev->dev_private;
	struct hibmc_bo *hibmcbo;
	size_t acc_size;
	int ret;

	hibmcbo = kzalloc(sizeof(*hibmcbo), GFP_KERNEL);
	if (!hibmcbo)
		return -ENOMEM;

	ret = drm_gem_object_init(dev, &hibmcbo->gem, size);
	if (ret) {
		kfree(hibmcbo);
		return ret;
	}

	hibmcbo->bo.bdev = &hibmc->ttm.bdev;

	hibmc_ttm_placement(hibmcbo, TTM_PL_FLAG_VRAM | TTM_PL_FLAG_SYSTEM);

	acc_size = ttm_bo_dma_acc_size(&hibmc->ttm.bdev, size,
				       sizeof(struct hibmc_bo));

	ret = ttm_bo_init(&hibmc->ttm.bdev, &hibmcbo->bo, size,
			  ttm_bo_type_device, &hibmcbo->placement,
			  align >> PAGE_SHIFT, false, NULL, acc_size,
			  NULL, NULL, hibmc_bo_ttm_destroy);
	if (ret)
		return ret;

	*phibmcbo = hibmcbo;
	return 0;
}

static inline u64 hibmc_bo_gpu_offset(struct hibmc_bo *bo)
{
	return bo->bo.offset;
}

int hibmc_bo_pin(struct hibmc_bo *bo, u32 pl_flag, u64 *gpu_addr)
{
	int i, ret;

	if (bo->pin_count) {
		bo->pin_count++;
		if (gpu_addr)
			*gpu_addr = hibmc_bo_gpu_offset(bo);
	}

	hibmc_ttm_placement(bo, pl_flag);
	for (i = 0; i < bo->placement.num_placement; i++)
		bo->placements[i].flags |= TTM_PL_FLAG_NO_EVICT;
	ret = ttm_bo_validate(&bo->bo, &bo->placement, false, false);
	if (ret)
		return ret;

	bo->pin_count = 1;
	if (gpu_addr)
		*gpu_addr = hibmc_bo_gpu_offset(bo);
	return 0;
}

int hibmc_bo_push_sysram(struct hibmc_bo *bo)
{
	int i, ret;

	if (!bo->pin_count) {
		DRM_ERROR("unpin bad %p\n", bo);
		return 0;
	}
	bo->pin_count--;
	if (bo->pin_count)
		return 0;

	if (bo->kmap.virtual)
		ttm_bo_kunmap(&bo->kmap);

	hibmc_ttm_placement(bo, TTM_PL_FLAG_SYSTEM);
	for (i = 0; i < bo->placement.num_placement ; i++)
		bo->placements[i].flags |= TTM_PL_FLAG_NO_EVICT;

	ret = ttm_bo_validate(&bo->bo, &bo->placement, false, false);
	if (ret) {
		DRM_ERROR("pushing to VRAM failed\n");
		return ret;
	}
	return 0;
}

int hibmc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct drm_file *file_priv;
	struct hibmc_drm_device *hibmc;

	if (unlikely(vma->vm_pgoff < DRM_FILE_PAGE_OFFSET))
		return -EINVAL;

	file_priv = filp->private_data;
	hibmc = file_priv->minor->dev->dev_private;
	return ttm_bo_mmap(filp, vma, &hibmc->ttm.bdev);
}

int hibmc_gem_create(struct drm_device *dev, u32 size, bool iskernel,
		     struct drm_gem_object **obj)
{
	struct hibmc_bo *hibmcbo;
	int ret;

	*obj = NULL;

	size = PAGE_ALIGN(size);
	if (size == 0)
		return -EINVAL;

	ret = hibmc_bo_create(dev, size, 0, 0, &hibmcbo);
	if (ret) {
		if (ret != -ERESTARTSYS)
			DRM_ERROR("failed to allocate GEM object\n");
		return ret;
	}
	*obj = &hibmcbo->gem;
	return 0;
}

int hibmc_dumb_create(struct drm_file *file, struct drm_device *dev,
		      struct drm_mode_create_dumb *args)
{
	struct drm_gem_object *gobj;
	u32 handle;
	int ret;

	args->pitch = ALIGN(args->width * ((args->bpp + 7) / 8), 16);
	args->size = args->pitch * args->height;

	ret = hibmc_gem_create(dev, args->size, false,
			       &gobj);
	if (ret)
		return ret;

	ret = drm_gem_handle_create(file, gobj, &handle);
	drm_gem_object_unreference_unlocked(gobj);
	if (ret)
		return ret;

	args->handle = handle;
	return 0;
}

static void hibmc_bo_unref(struct hibmc_bo **bo)
{
	struct ttm_buffer_object *tbo;

	if ((*bo) == NULL)
		return;

	tbo = &((*bo)->bo);
	ttm_bo_unref(&tbo);
	*bo = NULL;
}

void hibmc_gem_free_object(struct drm_gem_object *obj)
{
	struct hibmc_bo *hibmcbo = gem_to_hibmc_bo(obj);

	hibmc_bo_unref(&hibmcbo);
}

static u64 hibmc_bo_mmap_offset(struct hibmc_bo *bo)
{
	return drm_vma_node_offset_addr(&bo->bo.vma_node);
}

int hibmc_dumb_mmap_offset(struct drm_file *file, struct drm_device *dev,
			   u32 handle, u64 *offset)
{
	struct drm_gem_object *obj;
	struct hibmc_bo *bo;

	obj = drm_gem_object_lookup(file, handle);
	if (!obj)
		return -ENOENT;

	bo = gem_to_hibmc_bo(obj);
	*offset = hibmc_bo_mmap_offset(bo);

	drm_gem_object_unreference_unlocked(obj);
	return 0;
}

/* ---------------------------------------------------------------------- */

static void hibmc_user_framebuffer_destroy(struct drm_framebuffer *fb)
{
	struct hibmc_framebuffer *hibmc_fb = to_hibmc_framebuffer(fb);

	drm_gem_object_unreference_unlocked(hibmc_fb->obj);
	drm_framebuffer_cleanup(fb);
	kfree(hibmc_fb);
}

static const struct drm_framebuffer_funcs hibmc_fb_funcs = {
	.destroy = hibmc_user_framebuffer_destroy,
};

struct hibmc_framebuffer *
hibmc_framebuffer_init(struct drm_device *dev,
		       const struct drm_mode_fb_cmd2 *mode_cmd,
		       struct drm_gem_object *obj)
{
	struct hibmc_framebuffer *hibmc_fb;
	int ret;

	hibmc_fb = kzalloc(sizeof(*hibmc_fb), GFP_KERNEL);
	if (!hibmc_fb)
		return ERR_PTR(-ENOMEM);

	drm_helper_mode_fill_fb_struct(&hibmc_fb->fb, mode_cmd);
	hibmc_fb->obj = obj;
	ret = drm_framebuffer_init(dev, &hibmc_fb->fb, &hibmc_fb_funcs);
	if (ret) {
		DRM_ERROR("drm_framebuffer_init failed: %d\n", ret);
		kfree(hibmc_fb);
		return ERR_PTR(ret);
	}

	return hibmc_fb;
}

static struct drm_framebuffer *
hibmc_user_framebuffer_create(struct drm_device *dev,
			      struct drm_file *filp,
			      const struct drm_mode_fb_cmd2 *mode_cmd)
{
	struct drm_gem_object *obj;
	struct hibmc_framebuffer *hibmc_fb;

	DRM_DEBUG_DRIVER("%dx%d, format %c%c%c%c\n",
			 mode_cmd->width, mode_cmd->height,
			 (mode_cmd->pixel_format) & 0xff,
			 (mode_cmd->pixel_format >> 8)  & 0xff,
			 (mode_cmd->pixel_format >> 16) & 0xff,
			 (mode_cmd->pixel_format >> 24) & 0xff);

	obj = drm_gem_object_lookup(filp, mode_cmd->handles[0]);
	if (!obj)
		return ERR_PTR(-ENOENT);

	hibmc_fb = hibmc_framebuffer_init(dev, mode_cmd, obj);
	if (IS_ERR(hibmc_fb)) {
		drm_gem_object_unreference_unlocked(obj);
		return ERR_PTR((long)hibmc_fb);
	}
	return &hibmc_fb->fb;
}
