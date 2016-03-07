/*
 * PCIe host controller driver for HiSilicon HipXX SoCs
 *
 * Copyright (C) 2016 HiSilicon Co., Ltd. http://www.hisilicon.com
 *
 * Author: Dongdong Liu <liudongdong3@huawei.com>
 *         Gabriele Paoloni <gabriele.paoloni@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/interrupt.h>
#include <linux/acpi.h>
#include <linux/ecam.h>
#include <linux/pci.h>
#include <linux/pci-acpi.h>
#include "pcie-hisi.h"

#define GET_PCIE_LINK_STATUS  0x0

/* uuid 6d30f553-836c-408e-b6ad-45bccc957949 */
const u8 hisi_pcie_acpi_dsm_uuid[] = {
	0x53, 0xf5, 0x30, 0x6d, 0x6c, 0x83, 0x8e, 0x40,
	0xb6, 0xad, 0x45, 0xbc, 0xcc, 0x95, 0x79, 0x49
};

static const struct acpi_device_id hisi_pcie_ids[] = {
	{"HISI0080", 0},
	{"", 0},
};

static int hisi_pcie_get_addr(struct acpi_pci_root *root, const char *name,
				void __iomem **addr)
{
	struct acpi_device *device;
	u64 base;
	u64 size;
	u32 buf[4];
	int ret;

	device =  root->device;
	ret = fwnode_property_read_u32_array(&device->fwnode, name,
					buf, ARRAY_SIZE(buf));
	if (ret) {
		dev_err(&device->dev, "can't get %s\n", name);
		return ret;
	}

	base = ((u64)buf[0] << 32) | buf[1];
	size =  ((u64)buf[2] << 32) | buf[3];
	*addr = devm_ioremap(&device->dev, base, size);
	if (!(*addr)) {
		dev_err(&device->dev, "error with ioremap\n");
		return -ENOMEM;
	}

	return 0;
}


static int hisi_pcie_link_up_acpi(struct acpi_pci_root *root)
{
	u32 val;
	struct acpi_device *device;
	union acpi_object *obj;

	device = root->device;
	obj = acpi_evaluate_dsm(device->handle,
		hisi_pcie_acpi_dsm_uuid, 0,
		GET_PCIE_LINK_STATUS, NULL);

	if (!obj  ||  obj->type != ACPI_TYPE_INTEGER)  {
		dev_err(&device->dev, "can't get link status from _DSM\n");
		return 0;
	}
	val = obj->integer.value;

	return ((val & PCIE_LTSSM_STATE_MASK) == PCIE_LTSSM_LINKUP_STATE);

}

/*
 * Retrieve rc_dbi base and size from _DSD
 * Name (_DSD, Package () {
 *	ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
 *	Package () {
 *	Package () {"rc-dbi", Package () { 0x0, 0xb0080000, 0x0, 0x10000 }},
 *	}
 *	})
 */
static int hisi_pcie_init(struct acpi_pci_root *root)
{
	int ret;
	struct acpi_device *device;
	void __iomem *reg_base;

	device =  root->device;
	ret = hisi_pcie_get_addr(root, "rc-dbi", &reg_base);
	if (ret) {
		dev_err(&device->dev, "can't get rc-dbi\n");
		return ret;
	}

	root->sysdata = reg_base;
	return 0;
}

static int hisi_pcie_match(struct pci_mcfg_fixup *fixup,
			struct acpi_pci_root *root)
{
	int ret;
	struct acpi_device *device;

	device = root->device;
	ret = acpi_match_device_ids(device, hisi_pcie_ids);
	if (ret)
		return 0;

	ret = hisi_pcie_init(root);
	if (ret)
		dev_warn(&device->dev, "hisi pcie init fail\n");

	return 1;
}

static int hisi_pcie_acpi_valid_config(struct acpi_pci_root *root,
				struct pci_bus *bus, int dev)
{
	/* If there is no link, then there is no device */
	if (bus->number != root->secondary.start) {
		if (!hisi_pcie_link_up_acpi(root))
			return 0;
	}

	/* access only one slot on each root port */
	if (bus->number == root->secondary.start && dev > 0)
		return 0;

	/*
	 * do not read more than one device on the bus directly attached
	 * to RC's (Virtual Bridge's) DS side.
	 */
	if (bus->primary == root->secondary.start && dev > 0)
		return 0;

	return 1;
}

static int hisi_pcie_acpi_rd_conf(struct pci_bus *bus, u32 devfn, int where,
		int size, u32 *val)
{
	struct acpi_pci_root *root = bus->sysdata;
	void __iomem *reg_base = root->sysdata;

	if (hisi_pcie_acpi_valid_config(root, bus, PCI_SLOT(devfn)) == 0)
		return PCIBIOS_DEVICE_NOT_FOUND;

	if (bus->number == root->secondary.start)
		return hisi_pcie_common_cfg_read(reg_base, where, size, val);

	return pci_generic_config_read(bus, devfn, where, size, val);
}

static int hisi_pcie_acpi_wr_conf(struct pci_bus *bus, u32 devfn,
		int where, int size, u32 val)
{
	struct acpi_pci_root *root = bus->sysdata;
	void __iomem *reg_base = root->sysdata;

	if (hisi_pcie_acpi_valid_config(root, bus, PCI_SLOT(devfn)) == 0)
		return PCIBIOS_DEVICE_NOT_FOUND;

	if (bus->number == root->secondary.start)
		return hisi_pcie_common_cfg_write(reg_base, where, size, val);

	return pci_generic_config_write(bus, devfn, where, size, val);
}

struct pci_ops hisi_pcie_acpi_ops = {
	.map_bus = pci_mcfg_dev_base,
	.read = hisi_pcie_acpi_rd_conf,
	.write = hisi_pcie_acpi_wr_conf,
};


DECLARE_ACPI_MCFG_FIXUP(NULL, hisi_pcie_match, &hisi_pcie_acpi_ops,
	PCI_MCFG_DOMAIN_ANY, PCI_MCFG_BUS_ANY);
