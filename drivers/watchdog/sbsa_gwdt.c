/*
 * SBSA(Server Base System Architecture) Generic Watchdog driver
 *
 * Copyright (c) 2015, Linaro Ltd.
 * Author: Fu Wei <fu.wei@linaro.org>
 *         Suravee Suthikulpanit <Suravee.Suthikulpanit@amd.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * The SBSA Generic watchdog driver is compatible with the pretimeout
 * concept of Linux kernel.
 * The timeout and pretimeout are determined by WCV or WOR.
 * The first watch period is set by writing WCV directly, that can
 * support more than 10s timeout at the maximum system counter
 * frequency (400MHz).
 * When WS0 is triggered, the second watch period (pretimeout) is
 * determined by one of these registers:
 * (1)WOR: 32bit register, this gives a maximum watch period of
 * around 10s at the maximum system counter frequency. It's loaded
 * automatically by hardware.
 * (2)WCV: If the pretimeout value is greater then "max_wor_timeout",
 * it will be loaded in WS0 interrupt routine. If system is in
 * ws0_mode (reboot with watchdog enabled and WS0 == true), the ping
 * operation will only reload WCV.
 * More details about the hardware specification of this device:
 * ARM DEN0029B - Server Base System Architecture (SBSA)
 *
 * Kernel/API:                         P------------------| pretimeout
 *               |----------------------------------------T timeout
 * SBSA GWDT:                          P---WOR (or WCV)---WS1 pretimeout
 *               |-------WCV----------WS0~~~(ws0_mode)~~~~T timeout
 */

#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/watchdog.h>
#include <asm/arch_timer.h>

/* SBSA Generic Watchdog register definitions */
/* refresh frame */
#define SBSA_GWDT_WRR				0x000

/* control frame */
#define SBSA_GWDT_WCS				0x000
#define SBSA_GWDT_WOR				0x008
#define SBSA_GWDT_WCV_LO			0x010
#define SBSA_GWDT_WCV_HI			0x014

/* refresh/control frame */
#define SBSA_GWDT_W_IIDR			0xfcc
#define SBSA_GWDT_IDR				0xfd0

/* Watchdog Control and Status Register */
#define SBSA_GWDT_WCS_EN			BIT(0)
#define SBSA_GWDT_WCS_WS0			BIT(1)
#define SBSA_GWDT_WCS_WS1			BIT(2)

/**
 * struct sbsa_gwdt - Internal representation of the SBSA GWDT
 * @wdd:		kernel watchdog_device structure
 * @clk:		store the System Counter clock frequency, in Hz.
 * @ws0_mode:		indicate the system boot in the second stage timeout.
 * @max_wor_timeout:	the maximum timeout value for WOR (in seconds).
 * @refresh_base:	Virtual address of the watchdog refresh frame
 * @control_base:	Virtual address of the watchdog control frame
 */
struct sbsa_gwdt {
	struct watchdog_device	wdd;
	u32			clk;
	bool			ws0_mode;
	int			max_wor_timeout;
	void __iomem		*refresh_base;
	void __iomem		*control_base;
};

#define to_sbsa_gwdt(e) container_of(e, struct sbsa_gwdt, wdd)

#define DEFAULT_TIMEOUT		60 /* seconds, the 1st + 2nd watch periods*/
#define DEFAULT_PRETIMEOUT	30 /* seconds, the 2nd watch period*/

static unsigned int timeout;
module_param(timeout, uint, 0);
MODULE_PARM_DESC(timeout,
		 "Watchdog timeout in seconds. (>=0, default="
		 __MODULE_STRING(DEFAULT_TIMEOUT) ")");

static unsigned int pretimeout;
module_param(pretimeout, uint, 0);
MODULE_PARM_DESC(pretimeout,
		 "Watchdog pretimeout in seconds. (>=0, default="
		 __MODULE_STRING(DEFAULT_PRETIMEOUT) ")");

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, S_IRUGO);
MODULE_PARM_DESC(nowayout,
		 "Watchdog cannot be stopped once started (default="
		 __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

/*
 * help functions for accessing 64bit WCV register
 */
static u64 sbsa_gwdt_get_wcv(struct watchdog_device *wdd)
{
	u32 wcv_lo, wcv_hi;
	struct sbsa_gwdt *gwdt = to_sbsa_gwdt(wdd);

	do {
		wcv_hi = readl_relaxed(gwdt->control_base + SBSA_GWDT_WCV_HI);
		wcv_lo = readl_relaxed(gwdt->control_base + SBSA_GWDT_WCV_LO);
	} while (wcv_hi != readl_relaxed(gwdt->control_base +
					 SBSA_GWDT_WCV_HI));

	return (((u64)wcv_hi << 32) | wcv_lo);
}

static void sbsa_gwdt_set_wcv(struct watchdog_device *wdd, unsigned int t)
{
	struct sbsa_gwdt *gwdt = to_sbsa_gwdt(wdd);
	u64 wcv;

	wcv = arch_counter_get_cntvct() + (u64)t * gwdt->clk;

	writel_relaxed(upper_32_bits(wcv),
		       gwdt->control_base + SBSA_GWDT_WCV_HI);
	writel_relaxed(lower_32_bits(wcv),
		       gwdt->control_base + SBSA_GWDT_WCV_LO);
}

/*
 * inline functions for reloading 64bit WCV register
 */
static inline void reload_pretimeout_to_wcv(struct watchdog_device *wdd)
{
	sbsa_gwdt_set_wcv(wdd, wdd->pretimeout);
}

static inline void reload_first_stage_to_wcv(struct watchdog_device *wdd)
{
	sbsa_gwdt_set_wcv(wdd, wdd->timeout - wdd->pretimeout);
}

/*
 * watchdog operation functions
 */
static int sbsa_gwdt_set_timeout(struct watchdog_device *wdd,
				 unsigned int timeout)
{
	wdd->timeout = timeout;

	return 0;
}

static int sbsa_gwdt_set_pretimeout(struct watchdog_device *wdd,
				    unsigned int pretimeout)
{
	struct sbsa_gwdt *gwdt = to_sbsa_gwdt(wdd);
	u32 wor;

	wdd->pretimeout = pretimeout;

	/* If ws0_mode == true, we won't touch WOR */
	if (!gwdt->ws0_mode) {
		if (!pretimeout)
			/*
			 * If pretimeout is 0, it gives driver a timeslot (1s)
			 * to update WCV after an explicit refresh
			 * (sbsa_gwdt_start)
			 */
			wor = gwdt->clk;
		else
			if (pretimeout > gwdt->max_wor_timeout)
				wor = U32_MAX;
			else
				wor = pretimeout * gwdt->clk;

		/* wtite WOR, that will cause an explicit watchdog refresh */
		writel_relaxed(wor, gwdt->control_base + SBSA_GWDT_WOR);
	}

	return 0;
}

static unsigned int sbsa_gwdt_get_timeleft(struct watchdog_device *wdd)
{
	struct sbsa_gwdt *gwdt = to_sbsa_gwdt(wdd);
	u64 timeleft = sbsa_gwdt_get_wcv(wdd) - arch_counter_get_cntvct();

	do_div(timeleft, gwdt->clk);

	return timeleft;
}

static int sbsa_gwdt_keepalive(struct watchdog_device *wdd)
{
	struct sbsa_gwdt *gwdt = to_sbsa_gwdt(wdd);

	if (gwdt->ws0_mode)
		reload_pretimeout_to_wcv(wdd);
	else
		reload_first_stage_to_wcv(wdd);

	return 0;
}

static int sbsa_gwdt_start(struct watchdog_device *wdd)
{
	struct sbsa_gwdt *gwdt = to_sbsa_gwdt(wdd);

	/* If ws0_mode == true, the watchdog is enabled */
	if (!gwdt->ws0_mode)
		/* writing WCS will cause an explicit watchdog refresh */
		writel_relaxed(SBSA_GWDT_WCS_EN,
			       gwdt->control_base + SBSA_GWDT_WCS);

	return sbsa_gwdt_keepalive(wdd);
}

static int sbsa_gwdt_stop(struct watchdog_device *wdd)
{
	struct sbsa_gwdt *gwdt = to_sbsa_gwdt(wdd);

	writel_relaxed(0, gwdt->control_base + SBSA_GWDT_WCS);
	/*
	 * Writing WCS has caused an explicit watchdog refresh.
	 * Both watchdog signals are deasserted, so clean ws0_mode flag.
	 */
	gwdt->ws0_mode = false;

	return 0;
}

static irqreturn_t sbsa_gwdt_interrupt(int irq, void *dev_id)
{
	struct sbsa_gwdt *gwdt = (struct sbsa_gwdt *)dev_id;
	struct watchdog_device *wdd = &gwdt->wdd;

	/* We don't use pretimeout, trigger WS1 now */
	if (!wdd->pretimeout)
		sbsa_gwdt_set_wcv(wdd, 0);

	/*
	 * The pretimeout is valid, go panic
	 * If pretimeout is greater then "max_wor_timeout",
	 * reload the right value to WCV, then panic
	 */
	if (wdd->pretimeout > gwdt->max_wor_timeout)
		reload_pretimeout_to_wcv(wdd);
	panic("SBSA Watchdog pre-timeout");

	return IRQ_HANDLED;
}

static struct watchdog_info sbsa_gwdt_info = {
	.identity	= "SBSA Generic Watchdog",
	.options	= WDIOF_SETTIMEOUT |
			  WDIOF_KEEPALIVEPING |
			  WDIOF_MAGICCLOSE |
			  WDIOF_PRETIMEOUT |
			  WDIOF_CARDRESET,
};

static struct watchdog_ops sbsa_gwdt_ops = {
	.owner		= THIS_MODULE,
	.start		= sbsa_gwdt_start,
	.stop		= sbsa_gwdt_stop,
	.ping		= sbsa_gwdt_keepalive,
	.set_timeout	= sbsa_gwdt_set_timeout,
	.set_pretimeout	= sbsa_gwdt_set_pretimeout,
	.get_timeleft	= sbsa_gwdt_get_timeleft,
};

static int sbsa_gwdt_probe(struct platform_device *pdev)
{
	void __iomem *rf_base, *cf_base;
	struct device *dev = &pdev->dev;
	struct watchdog_device *wdd;
	struct sbsa_gwdt *gwdt;
	struct resource *res;
	int ret, irq;
	u32 status;

	gwdt = devm_kzalloc(dev, sizeof(*gwdt), GFP_KERNEL);
	if (!gwdt)
		return -ENOMEM;
	platform_set_drvdata(pdev, gwdt);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	cf_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(cf_base))
		return PTR_ERR(cf_base);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	rf_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(rf_base))
		return PTR_ERR(rf_base);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(dev, "unable to get ws0 interrupt.\n");
		return irq;
	}

	/*
	 * Get the frequency of system counter from the cp15 interface of ARM
	 * Generic timer. We don't need to check it, because if it returns "0",
	 * system would panic in very early stage.
	 */
	gwdt->clk = arch_timer_get_cntfrq();
	gwdt->refresh_base = rf_base;
	gwdt->control_base = cf_base;
	gwdt->max_wor_timeout = U32_MAX / gwdt->clk;
	gwdt->ws0_mode = false;

	wdd = &gwdt->wdd;
	wdd->parent = dev;
	wdd->info = &sbsa_gwdt_info;
	wdd->ops = &sbsa_gwdt_ops;
	watchdog_set_drvdata(wdd, gwdt);
	watchdog_set_nowayout(wdd, nowayout);

	wdd->min_pretimeout = 0;
	wdd->min_timeout = 1;

	/*
	 * Because the maximum of gwdt->clk is 400MHz and the maximum of WCV is
	 * U64_MAX, so the result of (U64_MAX / gwdt->clk) is always greater
	 * than U32_MAX. And the maximum of "unsigned int" is U32_MAX on ARM64.
	 * So we set the maximum value of pretimeout and timeout below.
	 */
	wdd->max_pretimeout = U32_MAX - 1;
	wdd->max_timeout = U32_MAX;

	wdd->pretimeout = DEFAULT_PRETIMEOUT;
	wdd->timeout = DEFAULT_TIMEOUT;
	watchdog_init_timeouts(wdd, pretimeout, timeout, dev);

	status = readl_relaxed(gwdt->control_base + SBSA_GWDT_WCS);
	if (status & SBSA_GWDT_WCS_WS1) {
		dev_warn(dev, "System reset by WDT.\n");
		wdd->bootstatus |= WDIOF_CARDRESET;
	} else if (status == (SBSA_GWDT_WCS_WS0 | SBSA_GWDT_WCS_EN)) {
		gwdt->ws0_mode = true;
	}

	ret = devm_request_irq(dev, irq, sbsa_gwdt_interrupt, 0,
			       pdev->name, gwdt);
	if (ret) {
		dev_err(dev, "unable to request IRQ %d\n", irq);
		return ret;
	}

	ret = watchdog_register_device(wdd);
	if (ret)
		return ret;

	/* If ws0_mode == true, the line won't update WOR */
	sbsa_gwdt_set_pretimeout(wdd, wdd->pretimeout);

	/*
	 * If watchdog is already enabled, do a ping operation
	 * to keep system running
	 */
	if (status & SBSA_GWDT_WCS_EN)
		sbsa_gwdt_keepalive(wdd);

	dev_info(dev, "Initialized with %ds timeout, %ds pretimeout @ %u Hz%s\n",
		 wdd->timeout, wdd->pretimeout, gwdt->clk,
		 status & SBSA_GWDT_WCS_EN ?
			gwdt->ws0_mode ? " [second stage]" : " [enabled]" :
			"");

	return 0;
}

static void sbsa_gwdt_shutdown(struct platform_device *pdev)
{
	struct sbsa_gwdt *gwdt = platform_get_drvdata(pdev);

	sbsa_gwdt_stop(&gwdt->wdd);
}

static int sbsa_gwdt_remove(struct platform_device *pdev)
{
	struct sbsa_gwdt *gwdt = platform_get_drvdata(pdev);

	watchdog_unregister_device(&gwdt->wdd);

	return 0;
}

/* Disable watchdog if it is active during suspend */
static int __maybe_unused sbsa_gwdt_suspend(struct device *dev)
{
	struct sbsa_gwdt *gwdt = dev_get_drvdata(dev);

	if (watchdog_active(&gwdt->wdd))
		sbsa_gwdt_stop(&gwdt->wdd);

	return 0;
}

/* Enable watchdog and configure it if necessary */
static int __maybe_unused sbsa_gwdt_resume(struct device *dev)
{
	struct sbsa_gwdt *gwdt = dev_get_drvdata(dev);

	if (watchdog_active(&gwdt->wdd))
		sbsa_gwdt_start(&gwdt->wdd);

	return 0;
}

static const struct dev_pm_ops sbsa_gwdt_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sbsa_gwdt_suspend, sbsa_gwdt_resume)
};

static const struct of_device_id sbsa_gwdt_of_match[] = {
	{ .compatible = "arm,sbsa-gwdt", },
	{},
};
MODULE_DEVICE_TABLE(of, sbsa_gwdt_of_match);

static const struct platform_device_id sbsa_gwdt_pdev_match[] = {
	{ .name = "sbsa-gwdt", },
	{},
};
MODULE_DEVICE_TABLE(platform, sbsa_gwdt_pdev_match);

static struct platform_driver sbsa_gwdt_driver = {
	.driver = {
		.name = "sbsa-gwdt",
		.pm = &sbsa_gwdt_pm_ops,
		.of_match_table = sbsa_gwdt_of_match,
	},
	.probe = sbsa_gwdt_probe,
	.remove = sbsa_gwdt_remove,
	.shutdown = sbsa_gwdt_shutdown,
	.id_table = sbsa_gwdt_pdev_match,
};

module_platform_driver(sbsa_gwdt_driver);

MODULE_DESCRIPTION("SBSA Generic Watchdog Driver");
MODULE_AUTHOR("Fu Wei <fu.wei@linaro.org>");
MODULE_AUTHOR("Suravee Suthikulpanit <Suravee.Suthikulpanit@amd.com>");
MODULE_LICENSE("GPL v2");
