/*
 *	watchdog_core.c
 *
 *	(c) Copyright 2008-2011 Alan Cox <alan@lxorguk.ukuu.org.uk>,
 *						All Rights Reserved.
 *
 *	(c) Copyright 2008-2011 Wim Van Sebroeck <wim@iguana.be>.
 *
 *	This source code is part of the generic code that can be used
 *	by all the watchdog timer drivers.
 *
 *	Based on source code of the following authors:
 *	  Matt Domsch <Matt_Domsch@dell.com>,
 *	  Rob Radez <rob@osinvestor.com>,
 *	  Rusty Lynch <rusty@linux.co.intel.com>
 *	  Satyam Sharma <satyam@infradead.org>
 *	  Randy Dunlap <randy.dunlap@oracle.com>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *	Neither Alan Cox, CymruNet Ltd., Wim Van Sebroeck nor Iguana vzw.
 *	admit liability nor provide warranty for any of this software.
 *	This material is provided "AS-IS" and at no charge.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>	/* For EXPORT_SYMBOL/module stuff/... */
#include <linux/types.h>	/* For standard types */
#include <linux/errno.h>	/* For the -ENODEV/... values */
#include <linux/kernel.h>	/* For printk/panic/... */
#include <linux/watchdog.h>	/* For watchdog specific items */
#include <linux/init.h>		/* For __init/__exit/... */
#include <linux/idr.h>		/* For ida_* macros */
#include <linux/err.h>		/* For IS_ERR macros */
#include <linux/of.h>		/* For of_get_timeout_sec */

#include "watchdog_core.h"	/* For watchdog_dev_register/... */

static DEFINE_IDA(watchdog_ida);
static struct class *watchdog_class;

/*
 * Deferred Registration infrastructure.
 *
 * Sometimes watchdog drivers needs to be loaded as soon as possible,
 * for example when it's impossible to disable it. To do so,
 * raising the initcall level of the watchdog driver is a solution.
 * But in such case, the miscdev is maybe not ready (subsys_initcall), and
 * watchdog_core need miscdev to register the watchdog as a char device.
 *
 * The deferred registration infrastructure offer a way for the watchdog
 * subsystem to register a watchdog properly, even before miscdev is ready.
 */

static DEFINE_MUTEX(wtd_deferred_reg_mutex);
static LIST_HEAD(wtd_deferred_reg_list);
static bool wtd_deferred_reg_done;

static int watchdog_deferred_registration_add(struct watchdog_device *wdd)
{
	list_add_tail(&wdd->deferred,
		      &wtd_deferred_reg_list);
	return 0;
}

static void watchdog_deferred_registration_del(struct watchdog_device *wdd)
{
	struct list_head *p, *n;
	struct watchdog_device *wdd_tmp;

	list_for_each_safe(p, n, &wtd_deferred_reg_list) {
		wdd_tmp = list_entry(p, struct watchdog_device,
				     deferred);
		if (wdd_tmp == wdd) {
			list_del(&wdd_tmp->deferred);
			break;
		}
	}
}

static void watchdog_check_min_max_timeout(struct watchdog_device *wdd)
{
	/*
	 * Check that we have valid min and max pretimeout and timeout values,
	 * if not, reset them all to 0 (=not used or unknown)
	 */
	if (wdd->min_pretimeout > wdd->max_pretimeout ||
	    wdd->min_timeout > wdd->max_timeout ||
	    wdd->min_timeout < wdd->min_pretimeout ||
	    wdd->max_timeout < wdd->max_pretimeout) {
		pr_info("Invalid min or max timeouts, resetting to 0\n");
		wdd->min_pretimeout = 0;
		wdd->max_pretimeout = 0;
		wdd->min_timeout = 0;
		wdd->max_timeout = 0;
	}
}

/**
 * watchdog_init_timeouts() - initialize the pretimeout and timeout field
 * @pretimeout_parm: pretimeout module parameter
 * @timeout_parm: timeout module parameter
 * @dev: Device that stores the timeout-sec property
 *
 * Initialize the pretimeout and timeout field of the watchdog_device struct
 * with both the pretimeout and timeout module parameters (if they are valid) or
 * the timeout-sec property (only if they are valid and the pretimeout_parm or
 * timeout_parm is out of bounds). If one of them is invalid, then we keep
 * the old value (which should normally be the default timeout value).
 *
 * A zero is returned on success and -EINVAL for failure.
 */
int watchdog_init_timeouts(struct watchdog_device *wdd,
			   unsigned int pretimeout_parm,
			   unsigned int timeout_parm,
			   struct device *dev)
{
	int ret = 0, length = 0;
	u32 timeouts[2] = {0};
	struct property *prop;

	watchdog_check_min_max_timeout(wdd);

	/*
	 * Backup the timeouts of wdd, and set them to the parameters,
	 * because watchdog_pretimeout_invalid uses wdd->timeout to validate
	 * the pretimeout_parm, and watchdog_timeout_invalid uses
	 * wdd->pretimeout to validate timeout_parm.
	 * if any of parameters is wrong, restore the default values before
	 * return.
	 */
	timeouts[0] = wdd->timeout;
	timeouts[1] = wdd->pretimeout;
	wdd->timeout = timeout_parm;
	wdd->pretimeout = pretimeout_parm;

	/*
	 * Try to get the pretimeout module parameter first.
	 * Note: zero is a valid value for pretimeout.
	 */
	if (watchdog_pretimeout_invalid(wdd, pretimeout_parm))
		ret = -EINVAL;

	/*
	 * Try to get the timeout module parameter,
	 * if it's valid and pretimeout is valid(ret == 0),
	 * assignment and return zero. Otherwise, try dtb.
	 */
	if (timeout_parm && !ret) {
		if (!watchdog_timeout_invalid(wdd, timeout_parm))
			return 0;
		ret = -EINVAL;
	}

	/*
	 * Either at least one of the module parameters is invalid,
	 * or timeout_parm is 0. Try to get the timeout_sec property.
	 */
	if (!dev || !dev->of_node) {
		wdd->timeout = timeouts[0];
		wdd->pretimeout = timeouts[1];
		return ret;
	}

	/*
	 * Backup default values to *_parms,
	 * timeouts[] will be used by of_property_read_u32_array.
	 */
	timeout_parm = timeouts[0];
	pretimeout_parm = timeouts[1];

	prop = of_find_property(dev->of_node, "timeout-sec", &length);
	if (prop && length > 0 && length <= sizeof(u32) * 2) {
		of_property_read_u32_array(dev->of_node,
					   "timeout-sec", timeouts,
					   length / sizeof(u32));
		wdd->timeout = timeouts[0];
		wdd->pretimeout = timeouts[1];

		if (length == sizeof(u32) * 2) {
			if (watchdog_pretimeout_invalid(wdd, timeouts[1]))
				goto error;
			ret = 0;
		} else {
			ret = -EINVAL;
		}

		if (!watchdog_timeout_invalid(wdd, timeouts[0]) &&
		    timeouts[0]) {
			if (ret) /* Only one value in "timeout-sec" */
				wdd->pretimeout = pretimeout_parm;
			return 0;
		}
	}

error:
	/* restore default values */
	wdd->timeout = timeout_parm;
	wdd->pretimeout = pretimeout_parm;

	return -EINVAL;
}
EXPORT_SYMBOL_GPL(watchdog_init_timeouts);

static int __watchdog_register_device(struct watchdog_device *wdd)
{
	int ret, id = -1, devno;

	if (wdd == NULL || wdd->info == NULL || wdd->ops == NULL)
		return -EINVAL;

	/* Mandatory operations need to be supported */
	if (wdd->ops->start == NULL || wdd->ops->stop == NULL)
		return -EINVAL;

	watchdog_check_min_max_timeout(wdd);

	/*
	 * Note: now that all watchdog_device data has been verified, we
	 * will not check this anymore in other functions. If data gets
	 * corrupted in a later stage then we expect a kernel panic!
	 */

	mutex_init(&wdd->lock);

	/* Use alias for watchdog id if possible */
	if (wdd->parent) {
		ret = of_alias_get_id(wdd->parent->of_node, "watchdog");
		if (ret >= 0)
			id = ida_simple_get(&watchdog_ida, ret,
					    ret + 1, GFP_KERNEL);
	}

	if (id < 0)
		id = ida_simple_get(&watchdog_ida, 0, MAX_DOGS, GFP_KERNEL);

	if (id < 0)
		return id;
	wdd->id = id;

	ret = watchdog_dev_register(wdd);
	if (ret) {
		ida_simple_remove(&watchdog_ida, id);
		if (!(id == 0 && ret == -EBUSY))
			return ret;

		/* Retry in case a legacy watchdog module exists */
		id = ida_simple_get(&watchdog_ida, 1, MAX_DOGS, GFP_KERNEL);
		if (id < 0)
			return id;
		wdd->id = id;

		ret = watchdog_dev_register(wdd);
		if (ret) {
			ida_simple_remove(&watchdog_ida, id);
			return ret;
		}
	}

	devno = wdd->cdev.dev;
	wdd->dev = device_create(watchdog_class, wdd->parent, devno,
					NULL, "watchdog%d", wdd->id);
	if (IS_ERR(wdd->dev)) {
		watchdog_dev_unregister(wdd);
		ida_simple_remove(&watchdog_ida, id);
		ret = PTR_ERR(wdd->dev);
		return ret;
	}

	return 0;
}

/**
 * watchdog_register_device() - register a watchdog device
 * @wdd: watchdog device
 *
 * Register a watchdog device with the kernel so that the
 * watchdog timer can be accessed from userspace.
 *
 * A zero is returned on success and a negative errno code for
 * failure.
 */

int watchdog_register_device(struct watchdog_device *wdd)
{
	int ret;

	mutex_lock(&wtd_deferred_reg_mutex);
	if (wtd_deferred_reg_done)
		ret = __watchdog_register_device(wdd);
	else
		ret = watchdog_deferred_registration_add(wdd);
	mutex_unlock(&wtd_deferred_reg_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(watchdog_register_device);

static void __watchdog_unregister_device(struct watchdog_device *wdd)
{
	int ret;
	int devno;

	if (wdd == NULL)
		return;

	devno = wdd->cdev.dev;
	ret = watchdog_dev_unregister(wdd);
	if (ret)
		pr_err("error unregistering /dev/watchdog (err=%d)\n", ret);
	device_destroy(watchdog_class, devno);
	ida_simple_remove(&watchdog_ida, wdd->id);
	wdd->dev = NULL;
}

/**
 * watchdog_unregister_device() - unregister a watchdog device
 * @wdd: watchdog device to unregister
 *
 * Unregister a watchdog device that was previously successfully
 * registered with watchdog_register_device().
 */

void watchdog_unregister_device(struct watchdog_device *wdd)
{
	mutex_lock(&wtd_deferred_reg_mutex);
	if (wtd_deferred_reg_done)
		__watchdog_unregister_device(wdd);
	else
		watchdog_deferred_registration_del(wdd);
	mutex_unlock(&wtd_deferred_reg_mutex);
}

EXPORT_SYMBOL_GPL(watchdog_unregister_device);

static int __init watchdog_deferred_registration(void)
{
	mutex_lock(&wtd_deferred_reg_mutex);
	wtd_deferred_reg_done = true;
	while (!list_empty(&wtd_deferred_reg_list)) {
		struct watchdog_device *wdd;

		wdd = list_first_entry(&wtd_deferred_reg_list,
				       struct watchdog_device, deferred);
		list_del(&wdd->deferred);
		__watchdog_register_device(wdd);
	}
	mutex_unlock(&wtd_deferred_reg_mutex);
	return 0;
}

static int __init watchdog_init(void)
{
	int err;

	watchdog_class = class_create(THIS_MODULE, "watchdog");
	if (IS_ERR(watchdog_class)) {
		pr_err("couldn't create class\n");
		return PTR_ERR(watchdog_class);
	}

	err = watchdog_dev_init();
	if (err < 0) {
		class_destroy(watchdog_class);
		return err;
	}

	watchdog_deferred_registration();
	return 0;
}

static void __exit watchdog_exit(void)
{
	watchdog_dev_exit();
	class_destroy(watchdog_class);
	ida_destroy(&watchdog_ida);
}

subsys_initcall_sync(watchdog_init);
module_exit(watchdog_exit);

MODULE_AUTHOR("Alan Cox <alan@lxorguk.ukuu.org.uk>");
MODULE_AUTHOR("Wim Van Sebroeck <wim@iguana.be>");
MODULE_DESCRIPTION("WatchDog Timer Driver Core");
MODULE_LICENSE("GPL");
