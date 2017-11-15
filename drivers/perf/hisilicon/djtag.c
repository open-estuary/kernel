/*
 * Driver for Hisilicon Djtag r/w which use CPU sysctrl.
 *
 * Copyright (C) 2017 Hisilicon Limited
 * Author: Tan Xiaojun <tanxiaojun@huawei.com>
 *         Anurup M <anurup.m@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/acpi.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/idr.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/time64.h>

#include "djtag.h"

/*
 * Timeout of 100ms is used to retry on djtag access busy condition.
 * If djtag unavailable event after timeout, it means some serious
 * hardware condition where we would BUG_ON, as subsequent access
 * are also likely to FAIL.
 */
#define SC_DJTAG_TIMEOUT_US    (100 * USEC_PER_MSEC)

#define DJTAG_PREFIX "hisi-djtag-dev-"

/* for djtag v1 */
#define SC_DJTAG_MSTR_EN		0x6800
#define DJTAG_NOR_CFG			BIT(1)	/* normal RW mode */
#define DJTAG_MSTR_EN			BIT(0)
#define SC_DJTAG_MSTR_START_EN		0x6804
#define DJTAG_MSTR_START_EN		0x1
#define SC_DJTAG_DEBUG_MODULE_SEL	0x680c
#define SC_DJTAG_MSTR_WR		0x6810
#define DJTAG_MSTR_W			0x1
#define DJTAG_MSTR_R			0x0
#define SC_DJTAG_CHAIN_UNIT_CFG_EN	0x6814
#define CHAIN_UNIT_CFG_EN		0xFFFF
#define SC_DJTAG_MSTR_ADDR		0x6818
#define SC_DJTAG_MSTR_DATA		0x681c
#define SC_DJTAG_RD_DATA_BASE		0xe800

#define SC_DJTAG_V1_UNLOCK             0x3B
#define SC_DJTAG_V1_KERNEL_LOCK        0x3C

/* for djtag v2 */
#define SC_DJTAG_SEC_ACC_EN_EX		0xd800
#define DJTAG_SEC_ACC_EN_EX		0x1
#define SC_DJTAG_MSTR_CFG_EX		0xd818
#define DJTAG_MSTR_RW_SHIFT_EX		29
#define DJTAG_MSTR_RD_EX		(0x0 << DJTAG_MSTR_RW_SHIFT_EX)
#define DJTAG_MSTR_WR_EX		(0x1 << DJTAG_MSTR_RW_SHIFT_EX)
#define DEBUG_MODULE_SEL_SHIFT_EX	16
#define CHAIN_UNIT_CFG_EN_EX		0xFFFF
#define SC_DJTAG_MSTR_ADDR_EX		0xd810
#define SC_DJTAG_MSTR_DATA_EX		0xd814
#define SC_DJTAG_MSTR_START_EN_EX	0xd81c
#define DJTAG_MSTR_START_EN_EX		0x1
#define SC_DJTAG_RD_DATA_BASE_EX	0xe800
#define SC_DJTAG_OP_ST_EX		0xe828
#define DJTAG_OP_DONE_EX		BIT(8)

#define SC_DJTAG_V2_UNLOCK             0xAA
#define SC_DJTAG_V2_KERNEL_LOCK        0xAB
#define SC_DJTAG_V2_MODULE_SEL_MASK    0xFF00FFFF

#define DJTAG_SYSCTRL_ADDR_SIZE		0x10000

static DEFINE_IDR(djtag_hosts_idr);
static DEFINE_IDR(djtag_clients_idr);

struct hisi_djtag_ops {
	void (*djtag_read)(void __iomem *regs_base, u32 offset,
			   u32 mod_sel, u32 mod_mask, int chain_id, u32 *rval);
	void (*djtag_write)(void __iomem *regs_base, u32 offset,
			    u32 mod_sel, u32 mod_mask, u32 wval, int chain_id);
};

struct hisi_djtag_host {
	spinlock_t lock;
	int id;
	u32 scl_id;
	struct device dev;
	struct list_head client_list;
	void __iomem *sysctl_reg_map;
	struct device_node *of_node;
	acpi_handle acpi_handle;
	const struct hisi_djtag_ops *djtag_ops;
};

#define to_hisi_djtag_client(d) container_of(d, struct hisi_djtag_client, dev)
#define to_hisi_djtag_driver(d) container_of(d, struct hisi_djtag_driver, \
					     driver)
#define MODULE_PREFIX "hisi_djtag:"

/*
 * hisi_djtag_lock_v1: djtag lock to avoid djtag access conflict
 * @reg_base:	djtag register base address
 *
 */
static void hisi_djtag_lock_v1(void __iomem *regs_base)
{
	u32 rd;

	/* Continuously poll to ensure the djtag is free */
	while (1) {
		rd = readl(regs_base + SC_DJTAG_DEBUG_MODULE_SEL);
		if (rd == SC_DJTAG_V1_UNLOCK) {
			writel(SC_DJTAG_V1_KERNEL_LOCK,
			       regs_base + SC_DJTAG_DEBUG_MODULE_SEL);
			udelay(10);
			rd = readl(regs_base + SC_DJTAG_DEBUG_MODULE_SEL);
			if (rd == SC_DJTAG_V1_KERNEL_LOCK)
				break;
		}
		udelay(10); /* 10us */
	}
}

static void hisi_djtag_prepare_v1(void __iomem *regs_base, u32 offset,
				  u32 mod_sel, u32 mod_mask)
{
	/* djtag master enable and support normal mode for R,W */
	writel(DJTAG_NOR_CFG | DJTAG_MSTR_EN, regs_base + SC_DJTAG_MSTR_EN);

	/* select module */
	writel(mod_sel, regs_base + SC_DJTAG_DEBUG_MODULE_SEL);
	writel(mod_mask & CHAIN_UNIT_CFG_EN,
	       regs_base + SC_DJTAG_CHAIN_UNIT_CFG_EN);

	/* address offset */
	writel(offset, regs_base + SC_DJTAG_MSTR_ADDR);
}

static void hisi_djtag_do_operation_v1(void __iomem *regs_base)
{
	u32 val;
	int ret;

	/* start to write to djtag register */
	writel(DJTAG_MSTR_START_EN, regs_base + SC_DJTAG_MSTR_START_EN);

	/* ensure the djtag operation is done */
	ret = readl_poll_timeout_atomic(regs_base + SC_DJTAG_MSTR_START_EN,
					val, !(val & DJTAG_MSTR_EN), 1,
					SC_DJTAG_TIMEOUT_US);
	/*
	 * Djtag access busy even after timeout indicate a serious hardware
	 * condition. BUG_ON as subsequent access are also likely to go wrong.
	 */
	BUG_ON(ret == -ETIMEDOUT);
}

/*
 * hisi_djtag_lock_v2: djtag lock to avoid djtag access conflict b/w kernel
 * and UEFI.
 * @reg_base:	djtag register base address
 *
 * Return - none.
 */
static void hisi_djtag_lock_v2(void __iomem *regs_base)
{
	u32 rd, wr, mod_sel;

	/* Continuously poll to ensure the djtag is free */
	while (1) {
		rd = readl(regs_base + SC_DJTAG_MSTR_CFG_EX);
		mod_sel = ((rd >> DEBUG_MODULE_SEL_SHIFT_EX) & 0xFF);
		if (mod_sel == SC_DJTAG_V2_UNLOCK) {
			wr = ((rd & SC_DJTAG_V2_MODULE_SEL_MASK) |
			      (SC_DJTAG_V2_KERNEL_LOCK <<
			       DEBUG_MODULE_SEL_SHIFT_EX));
			writel(wr, regs_base + SC_DJTAG_MSTR_CFG_EX);
			udelay(10); /* 10us */

			rd = readl(regs_base + SC_DJTAG_MSTR_CFG_EX);
			mod_sel = ((rd >> DEBUG_MODULE_SEL_SHIFT_EX) & 0xFF);
			if (mod_sel == SC_DJTAG_V2_KERNEL_LOCK)
				break;
		}
		udelay(10); /* 10us */
	}
}

/*
 * hisi_djtag_unlock_v2: djtag unlock
 * @reg_base:	djtag register base address
 *
 * Return - none.
 */
static void hisi_djtag_unlock_v2(void __iomem *regs_base)
{
	u32 rd, wr;

	rd = readl(regs_base + SC_DJTAG_MSTR_CFG_EX);

	wr = ((rd & SC_DJTAG_V2_MODULE_SEL_MASK) |
	      (SC_DJTAG_V2_UNLOCK << DEBUG_MODULE_SEL_SHIFT_EX));
	/*
	 * Release djtag module by writing to module
	 * selection bits of DJTAG_MSTR_CFG register
	 */
	writel(wr, regs_base + SC_DJTAG_MSTR_CFG_EX);
}

static void hisi_djtag_prepare_v2(void __iomem *regs_base, u32 offset,
				  u32 mod_sel, u32 mod_mask)
{
	/* djtag mster enable */
	writel(DJTAG_SEC_ACC_EN_EX, regs_base + SC_DJTAG_SEC_ACC_EN_EX);

	/* address offset */
	writel(offset, regs_base + SC_DJTAG_MSTR_ADDR_EX);
}

static void hisi_djtag_do_operation_v2(void __iomem *regs_base)
{
	u32 val;
	int ret;

	/* start to write to djtag register */
	writel(DJTAG_MSTR_START_EN_EX, regs_base + SC_DJTAG_MSTR_START_EN_EX);

	/* ensure the djtag operation is done */
	ret = readl_poll_timeout_atomic(regs_base + SC_DJTAG_MSTR_START_EN_EX,
					val, !(val & DJTAG_MSTR_START_EN_EX), 1,
					SC_DJTAG_TIMEOUT_US);
	/*
	 * Djtag access busy even after timeout indicate a serious hardware
	 * condition. BUG_ON as subsequent access are also likely to go wrong.
	 */
	BUG_ON(ret == -ETIMEDOUT);

	/* wait for the operation to complete */
	ret = readl_poll_timeout_atomic(regs_base + SC_DJTAG_OP_ST_EX,
					val, (val & DJTAG_OP_DONE_EX), 1,
					SC_DJTAG_TIMEOUT_US);

	/*
	 * Djtag access busy even after timeout indicate a serious hardware
	 * condition. BUG_ON as subsequent access are also likely to go wrong.
	 */
	BUG_ON(ret == -ETIMEDOUT);
}

/*
 * hisi_djtag_read_v1/v2: djtag read interface
 * @reg_base:	djtag register base address
 * @offset:	register's offset
 * @mod_sel:	module selection
 * @mod_mask:	mask to select specific modules for read
 * @chain_id:	which sub module to read
 * @rval:	value read from register
 *
 * Return - none.
 */
static void hisi_djtag_read_v1(void __iomem *regs_base, u32 offset, u32 mod_sel,
			       u32 mod_mask, int chain_id, u32 *rval)
{
	hisi_djtag_lock_v1(regs_base);

	hisi_djtag_prepare_v1(regs_base, offset, mod_sel, mod_mask);

	writel(DJTAG_MSTR_R, regs_base + SC_DJTAG_MSTR_WR);

	hisi_djtag_do_operation_v1(regs_base);

	*rval = readl(regs_base + SC_DJTAG_RD_DATA_BASE + chain_id * 0x4);

	/* Unlock djtag by setting module selection register to 0x3A */
	writel(SC_DJTAG_V1_UNLOCK, regs_base + SC_DJTAG_DEBUG_MODULE_SEL);
}

static void hisi_djtag_read_v2(void __iomem *regs_base, u32 offset, u32 mod_sel,
			       u32 mod_mask, int chain_id, u32 *rval)
{
	u32 val = DJTAG_MSTR_RD_EX |
		(mod_sel << DEBUG_MODULE_SEL_SHIFT_EX) |
		(mod_mask & CHAIN_UNIT_CFG_EN_EX);

	hisi_djtag_lock_v2(regs_base);

	hisi_djtag_prepare_v2(regs_base, offset, mod_sel, mod_mask);

	writel(val, regs_base + SC_DJTAG_MSTR_CFG_EX);

	hisi_djtag_do_operation_v2(regs_base);

	*rval = readl(regs_base + SC_DJTAG_RD_DATA_BASE_EX + chain_id * 0x4);

	hisi_djtag_unlock_v2(regs_base);
}

/*
 * hisi_djtag_write_v1/v2: djtag write interface
 * @reg_base:	djtag register base address
 * @offset:	register's offset
 * @mod_sel:	module selection
 * @mod_mask:	mask to select specific modules for write
 * @wval:	value to write to register
 * @chain_id:	which sub module to write
 *
 * Return - none.
 */
static void hisi_djtag_write_v1(void __iomem *regs_base, u32 offset,
				u32 mod_sel, u32 mod_mask, u32 wval,
				int chain_id)
{
	hisi_djtag_lock_v1(regs_base);

	hisi_djtag_prepare_v1(regs_base, offset, mod_sel, mod_mask);

	writel(DJTAG_MSTR_W, regs_base + SC_DJTAG_MSTR_WR);
	writel(wval, regs_base + SC_DJTAG_MSTR_DATA);

	hisi_djtag_do_operation_v1(regs_base);

	/* Unlock djtag by setting module selection register to 0x3A */
	writel(SC_DJTAG_V1_UNLOCK, regs_base + SC_DJTAG_DEBUG_MODULE_SEL);
}

static void hisi_djtag_write_v2(void __iomem *regs_base, u32 offset,
				u32 mod_sel, u32 mod_mask, u32 wval,
				int chain_id)
{
	u32 val = DJTAG_MSTR_WR_EX |
		(mod_sel << DEBUG_MODULE_SEL_SHIFT_EX) |
		(mod_mask & CHAIN_UNIT_CFG_EN_EX);
	hisi_djtag_lock_v2(regs_base);

	hisi_djtag_prepare_v2(regs_base, offset, mod_sel, mod_mask);

	writel(val, regs_base + SC_DJTAG_MSTR_CFG_EX);
	writel(wval, regs_base + SC_DJTAG_MSTR_DATA_EX);

	hisi_djtag_do_operation_v2(regs_base);

	hisi_djtag_unlock_v2(regs_base);
}

/*
 * hisi_djtag_writel - write registers via djtag
 * @client: djtag client handle
 * @offset:	register's offset
 * @mod_sel:	module selection
 * @mod_mask:	mask to select specific modules
 * @val:	value to write to register
 *
 * Return - none.
 */
void hisi_djtag_writel(struct hisi_djtag_client *client, u32 offset,
		       u32 mod_sel, u32 mod_mask, u32 val)
{
	struct hisi_djtag_host *host = client->host;
	void __iomem *reg_map = host->sysctl_reg_map;
	const struct hisi_djtag_ops *ops = host->djtag_ops;
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);
	ops->djtag_write(reg_map, offset, mod_sel, mod_mask, val, 0);
	spin_unlock_irqrestore(&host->lock, flags);
}

/*
 * hisi_djtag_readl - read registers via djtag
 * @client: djtag client handle
 * @offset:	register's offset
 * @mod_sel:	module type selection
 * @chain_id:	chain_id number, mostly is 0
 * @val:	register's value
 *
 * Return - none.
 */
void hisi_djtag_readl(struct hisi_djtag_client *client, u32 offset,
		      u32 mod_sel, int chain_id, u32 *val)
{
	struct hisi_djtag_host *host = client->host;
	void __iomem *reg_map = host->sysctl_reg_map;
	const struct hisi_djtag_ops *ops = host->djtag_ops;
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);
	ops->djtag_read(reg_map, offset, mod_sel, 0xffff, chain_id, val);
	spin_unlock_irqrestore(&host->lock, flags);
}

u32 hisi_djtag_get_sclid(struct hisi_djtag_client *client)
{
	return client->host->scl_id;
}

static const struct hisi_djtag_ops djtag_v1_ops = {
	.djtag_read  = hisi_djtag_read_v1,
	.djtag_write  = hisi_djtag_write_v1,
};

static const struct hisi_djtag_ops djtag_v2_ops = {
	.djtag_read  = hisi_djtag_read_v2,
	.djtag_write  = hisi_djtag_write_v2,
};

static const struct of_device_id djtag_of_match[] = {
	/* for hip05 CPU die */
	{ .compatible = "hisilicon,hip05-cpu-djtag-v1",	.data = &djtag_v1_ops },
	/* for hip05 IO die */
	{ .compatible = "hisilicon,hip05-io-djtag-v1",	.data = &djtag_v1_ops },
	/* for hip06 CPU die */
	{ .compatible = "hisilicon,hip06-cpu-djtag-v1",	.data = &djtag_v1_ops },
	/* for hip06 IO die */
	{ .compatible = "hisilicon,hip06-io-djtag-v2",	.data = &djtag_v2_ops },
	/* for hip07 CPU die */
	{ .compatible = "hisilicon,hip07-cpu-djtag-v2",	.data = &djtag_v2_ops },
	/* for hip07 IO die */
	{ .compatible = "hisilicon,hip07-io-djtag-v2",	.data = &djtag_v2_ops },
	{},
};
MODULE_DEVICE_TABLE(of, djtag_of_match);

static const struct acpi_device_id djtag_acpi_match[] = {
	{ "HISI0201", (kernel_ulong_t)&djtag_v1_ops},
	{ "HISI0202", (kernel_ulong_t)&djtag_v2_ops},
	{ }
};
MODULE_DEVICE_TABLE(acpi, djtag_acpi_match);

static ssize_t show_modalias(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct hisi_djtag_client *client = to_hisi_djtag_client(dev);

	return sprintf(buf, "%s%s\n", MODULE_PREFIX, dev_name(&client->dev));
}
static DEVICE_ATTR(modalias, 0444, show_modalias, NULL);

static struct attribute *hisi_djtag_dev_attrs[] = {
	/* modalias helps coldplug:  modprobe $(cat .../modalias) */
	&dev_attr_modalias.attr,
	NULL
};
ATTRIBUTE_GROUPS(hisi_djtag_dev);

static struct device_type hisi_djtag_client_type = {
	.groups	= hisi_djtag_dev_groups,
};

static int hisi_djtag_device_probe(struct device *dev)
{
	struct hisi_djtag_driver *driver;
	struct hisi_djtag_client *client;
	int ret;

	client = to_hisi_djtag_client(dev);
	if (!client) {
		dev_err(dev, "could not find client\n");
		return -ENODEV;
	}

	driver = to_hisi_djtag_driver(dev->driver);
	if (!driver) {
		dev_err(dev, "could not find driver\n");
		return -ENODEV;
	}

	ret = driver->probe(client);
	if (ret < 0) {
		dev_err(dev, "client probe failed\n");
		return ret;
	}

	return 0;
}

static int hisi_djtag_device_remove(struct device *dev)
{
	struct hisi_djtag_driver *driver;
	struct hisi_djtag_client *client;
	int ret;

	client = to_hisi_djtag_client(dev);
	if (!client) {
		dev_err(dev, "could not find client\n");
		return -ENODEV;
	}

	driver = to_hisi_djtag_driver(dev->driver);
	if (!driver) {
		dev_err(dev, "could not find driver\n");
		return -ENODEV;
	}

	ret = driver->remove(client);
	if (ret < 0) {
		dev_err(dev, "client probe failed\n");
		return ret;
	}

	return 0;
}

static int hisi_djtag_device_match(struct device *dev,
				   struct device_driver *drv)
{
	/* Attempt an OF style match */
	if (of_driver_match_device(dev, drv))
		return true;

#ifdef CONFIG_ACPI
	if (acpi_driver_match_device(dev, drv))
		return true;
#endif
	return false;
}

struct bus_type hisi_djtag_bus = {
	.name		= "hisi-djtag",
	.match		= hisi_djtag_device_match,
	.probe		= hisi_djtag_device_probe,
	.remove		= hisi_djtag_device_remove,
};

static struct hisi_djtag_client *
hisi_djtag_client_alloc(struct hisi_djtag_host *host)
{
	struct hisi_djtag_client *client;

	client = kzalloc(sizeof(*client), GFP_KERNEL);
	if (!client)
		return NULL;

	client->host = host;
	client->dev.parent = &client->host->dev;
	client->dev.bus = &hisi_djtag_bus;
	client->dev.type = &hisi_djtag_client_type;

	return client;
}

static int hisi_djtag_get_client_id(struct hisi_djtag_client *client)
{
	return idr_alloc(&djtag_clients_idr, client, 0, 0, GFP_KERNEL);
}

static int hisi_djtag_set_client_name(struct hisi_djtag_client *client,
				      const char *device_name)
{
	char name[DJTAG_CLIENT_NAME_LEN];
	int id;

	id = hisi_djtag_get_client_id(client);
	if (id < 0)
		return id;

	client->id = id;
	snprintf(name, DJTAG_CLIENT_NAME_LEN, "%s%s_%d", DJTAG_PREFIX,
		 device_name, client->id);
	dev_set_name(&client->dev, "%s", name);

	return 0;
}

static int hisi_djtag_new_of_device(struct hisi_djtag_host *host,
				    struct device_node *node)
{
	struct hisi_djtag_client *client;
	int ret;

	client = hisi_djtag_client_alloc(host);
	if (!client) {
		dev_err(&host->dev, "DT: Client alloc fail!\n");
		return -ENOMEM;
	}

	client->dev.of_node = of_node_get(node);

	ret = hisi_djtag_set_client_name(client, node->name);
	if (ret < 0) {
		dev_err(&host->dev, "DT: Client set name fail!\n");
		goto fail;
	}

	ret = device_register(&client->dev);
	if (ret < 0) {
		dev_err(&client->dev,
			"DT: error adding new device, ret=%d\n", ret);
		idr_remove(&djtag_clients_idr, client->id);
		goto fail;
	}

	list_add(&client->next, &host->client_list);

	return 0;
fail:
	of_node_put(client->dev.of_node);
	kfree(client);
	return ret;
}

static acpi_status djtag_add_new_acpi_device(acpi_handle handle, u32 level,
					     void *data, void **return_value)
{
	struct acpi_device *acpi_dev;
	struct hisi_djtag_client *client;
	struct hisi_djtag_host *host = data;
	struct device *dev;
	const char *cid = NULL;
	int ret;

	if (acpi_bus_get_device(handle, &acpi_dev))
		return -ENODEV;

	dev = &acpi_dev->dev;
	client = hisi_djtag_client_alloc(host);
	if (!client) {
		dev_err(dev, "ACPI: Client alloc fail!\n");
		return -ENOMEM;
	}

	client->dev.fwnode = acpi_fwnode_handle(acpi_dev);

	cid = acpi_device_hid(acpi_dev);
	if (!cid) {
		dev_err(dev, "ACPI: could not read _CID!\n");
		ret = -EINVAL;
		goto fail;
	}

	ret = hisi_djtag_set_client_name(client, cid);
	if (ret < 0)
		goto fail;

	ret = device_register(&client->dev);
	if (ret < 0) {
		dev_err(dev, "ACPI: Error adding new device, ret=%d\n", ret);
		idr_remove(&djtag_clients_idr, client->id);
		goto fail;
	}

	list_add(&client->next, &host->client_list);

	return 0;

fail:
	kfree(client);
	return ret;
}

static void djtag_register_devices(struct hisi_djtag_host *host)
{
	struct device_node *node;

	if (host->of_node) {
		for_each_available_child_of_node(host->of_node, node) {
			if (of_node_test_and_set_flag(node, OF_POPULATED))
				continue;
			if (hisi_djtag_new_of_device(host, node))
				break;
		}
	} else if (host->acpi_handle) {
		acpi_handle handle = host->acpi_handle;
		acpi_status status;

		status = acpi_walk_namespace(ACPI_TYPE_DEVICE, handle, 1,
					     djtag_add_new_acpi_device, NULL,
					     host, NULL);
		if (ACPI_FAILURE(status)) {
			dev_err(&host->dev,
				"ACPI: Fail to read client devices!\n");
			return;
		}
	}
}

static int hisi_djtag_add_host(struct hisi_djtag_host *host)
{
	int ret;

	host->dev.bus = &hisi_djtag_bus;

	ret = idr_alloc(&djtag_hosts_idr, host, 0, 0, GFP_KERNEL);
	if (ret < 0) {
		dev_err(&host->dev, "No available djtag host ID'!s\n");
		return ret;
	}
	host->id = ret;

	/* Suffix the unique ID and set djtag hostname */
	dev_set_name(&host->dev, "djtag-host-%d", host->id);
	ret = device_register(&host->dev);
	if (ret < 0) {
		dev_err(&host->dev,
			"add_host dev register failed, ret=%d\n", ret);
		idr_remove(&djtag_hosts_idr, host->id);
		return ret;
	}

	djtag_register_devices(host);

	return 0;
}

static int djtag_host_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hisi_djtag_host *host;
	struct resource *res;
	int ret;

	host = kzalloc(sizeof(*host), GFP_KERNEL);
	if (!host)
		return -ENOMEM;

	if (dev->of_node) {
		const struct of_device_id *of_id;

		of_id = of_match_device(djtag_of_match, dev);
		if (!of_id) {
			ret = -EINVAL;
			goto fail;
		}

		host->djtag_ops = of_id->data;
		host->of_node = of_node_get(dev->of_node);
	} else if (ACPI_COMPANION(dev)) {
		const struct acpi_device_id *acpi_id;

		acpi_id = acpi_match_device(djtag_acpi_match, dev);
		if (!acpi_id) {
			ret = -EINVAL;
			goto fail;
		}
		host->djtag_ops = (struct hisi_djtag_ops *)acpi_id->driver_data;

		host->acpi_handle = ACPI_HANDLE(dev);
		if (!host->acpi_handle) {
			ret = -EINVAL;
			goto fail;
		}
	} else {
		ret = -EINVAL;
		goto fail;
	}

	/* Find the SCL ID */
	ret = device_property_read_u32(dev, "hisilicon,scl-id", &host->scl_id);
	if (ret < 0)
		goto fail;

	spin_lock_init(&host->lock);
	INIT_LIST_HEAD(&host->client_list);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "No reg resorces!\n");
		ret = -EINVAL;
		goto fail;
	}

	if (resource_size(res) != DJTAG_SYSCTRL_ADDR_SIZE) {
		dev_err(dev, "Incorrect djtag sysctrl address range!\n");
		ret = -EINVAL;
		goto fail;
	}

	host->sysctl_reg_map = devm_ioremap_resource(dev, res);
	if (IS_ERR(host->sysctl_reg_map)) {
		dev_err(dev, "Unable to map sysctl registers!\n");
		ret = -EINVAL;
		goto fail;
	}

	platform_set_drvdata(pdev, host);

	ret = hisi_djtag_add_host(host);
	if (ret) {
		dev_err(dev, "add host failed, ret=%d\n", ret);
		goto fail;
	}

	return 0;
fail:
	of_node_put(dev->of_node);
	kfree(host);
	return ret;
}

static int djtag_host_remove(struct platform_device *pdev)
{
	struct hisi_djtag_host *host;
	struct hisi_djtag_client *client, *tmp;
	struct list_head *client_list;

	host = platform_get_drvdata(pdev);
	client_list = &host->client_list;

	list_for_each_entry_safe(client, tmp, client_list, next) {
		list_del(&client->next);
		device_unregister(&client->dev);
		idr_remove(&djtag_clients_idr, client->id);
		of_node_put(client->dev.of_node);
		kfree(client);
	}

	device_unregister(&host->dev);
	idr_remove(&djtag_hosts_idr, host->id);
	of_node_put(host->of_node);
	kfree(host);

	return 0;
}

static struct platform_driver djtag_dev_driver = {
	.driver = {
		.name = "hisi-djtag",
		.of_match_table = djtag_of_match,
		.acpi_match_table = ACPI_PTR(djtag_acpi_match),
	},
	.probe = djtag_host_probe,
	.remove = djtag_host_remove,
};
module_platform_driver(djtag_dev_driver);

int hisi_djtag_register_driver(struct module *owner,
			       struct hisi_djtag_driver *driver)
{
	int ret;

	driver->driver.owner = owner;
	driver->driver.bus = &hisi_djtag_bus;

	ret = driver_register(&driver->driver);
	if (ret < 0)
		pr_err("%s register failed, ret=%d\n", __func__, ret);

	return ret;
}

void hisi_djtag_unregister_driver(struct hisi_djtag_driver *driver)
{
	driver->driver.bus = &hisi_djtag_bus;
	driver_unregister(&driver->driver);
}

static int __init hisi_djtag_init(void)
{
	int ret;

	ret = bus_register(&hisi_djtag_bus);
	if (ret) {
		pr_err("hisi  djtag init failed, ret=%d\n", ret);
		return ret;
	}

	return 0;
}
module_init(hisi_djtag_init);

static void __exit hisi_djtag_exit(void)
{
	bus_unregister(&hisi_djtag_bus);
}
module_exit(hisi_djtag_exit);

MODULE_DESCRIPTION("Hisilicon djtag driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
