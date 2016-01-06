/*
 * Code borrowed from powerpc/kernel/pci-common.c
 *
 * Copyright (C) 2003 Anton Blanchard <anton@au.ibm.com>, IBM
 * Copyright (C) 2014 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 */

#include <linux/acpi.h>
#include <linux/ecam.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/of_address.h>
#include <linux/of_pci.h>
#include <linux/of_platform.h>
#include <linux/pci-acpi.h>
#include <linux/slab.h>

#include <asm/pci-bridge.h>

/*
 * Called after each bus is probed, but before its children are examined
 */
void pcibios_fixup_bus(struct pci_bus *bus)
{
	/* nothing to do, expected to be removed in the future */
}

/*
 * We don't have to worry about legacy ISA devices, so nothing to do here
 */
resource_size_t pcibios_align_resource(void *data, const struct resource *res,
				resource_size_t size, resource_size_t align)
{
	return res->start;
}

/**
 * pcibios_enable_device - Enable I/O and memory.
 * @dev: PCI device to be enabled
 * @mask: bitmask of BARs to enable
 */
int pcibios_enable_device(struct pci_dev *dev, int mask)
{
	if (pci_has_flag(PCI_PROBE_ONLY))
		return 0;

	return pci_enable_resources(dev, mask);
}

/*
 * Try to assign the IRQ number from DT/ACPI when adding a new device
 */
int pcibios_add_device(struct pci_dev *dev)
{
	if (acpi_disabled)
		dev->irq = of_irq_parse_and_map_pci(dev, 0, 0);
#ifdef CONFIG_ACPI
	else
		acpi_pci_irq_enable(dev);
#endif

	return 0;
}

#ifdef CONFIG_ACPI
int pcibios_root_bridge_prepare(struct pci_host_bridge *bridge)
{
	struct acpi_pci_root *root = bridge->bus->sysdata;

	ACPI_COMPANION_SET(&bridge->dev, root->device);
	return 0;
}

void pcibios_add_bus(struct pci_bus *bus)
{
	acpi_pci_add_bus(bus);
}

void pcibios_remove_bus(struct pci_bus *bus)
{
	acpi_pci_remove_bus(bus);
}

static int __init pcibios_assign_resources(void)
{
	if (acpi_disabled)
		return 0;

	pci_assign_unassigned_resources();
	return 0;
}
/*
 * rootfs_initcall comes after subsys_initcall and fs_initcall_sync,
 * so we know acpi scan and PCI_FIXUP_FINAL quirks have both run.
 */
rootfs_initcall(pcibios_assign_resources);

static void __iomem *
pci_mcfg_dev_base(struct pci_bus *bus, unsigned int devfn, int offset)
{
	struct pci_mmcfg_region *cfg;

	cfg = pci_mmconfig_lookup(pci_domain_nr(bus), bus->number);
	if (cfg && cfg->virt)
		return cfg->virt +
			(PCI_MMCFG_BUS_OFFSET(bus->number) | (devfn << 12)) +
			offset;
	return NULL;
}

struct pci_ops pci_root_ops = {
	.map_bus = pci_mcfg_dev_base,
	.read = pci_generic_config_read,
	.write = pci_generic_config_write,
};

#ifdef CONFIG_PCI_MMCONFIG
static int pci_add_mmconfig_region(struct acpi_pci_root_info *ci)
{
	struct pci_mmcfg_region *cfg;
	struct acpi_pci_root *root;
	int seg, start, end, err;

	root = ci->root;
	seg = root->segment;
	start = root->secondary.start;
	end = root->secondary.end;

	cfg = pci_mmconfig_lookup(seg, start);
	if (cfg)
		return 0;

	cfg = pci_mmconfig_alloc(seg, start, end, root->mcfg_addr);
	if (!cfg)
		return -ENOMEM;

	err = pci_mmconfig_inject(cfg);
	return err;
}

static void pci_remove_mmconfig_region(struct acpi_pci_root_info *ci)
{
	struct acpi_pci_root *root = ci->root;
	struct pci_mmcfg_region *cfg;

	cfg = pci_mmconfig_lookup(root->segment, root->secondary.start);
	if (cfg)
		return;

	if (cfg->hot_added)
		pci_mmconfig_delete(root->segment, root->secondary.start,
				    root->secondary.end);
}
#else
static int pci_add_mmconfig_region(struct acpi_pci_root_info *ci)
{
	return 0;
}

static void pci_remove_mmconfig_region(struct acpi_pci_root_info *ci) { }
#endif

static int pci_acpi_root_init_info(struct acpi_pci_root_info *ci)
{
	return pci_add_mmconfig_region(ci);
}

static void pci_acpi_root_release_info(struct acpi_pci_root_info *ci)
{
	pci_remove_mmconfig_region(ci);
	kfree(ci);
}

static int pci_acpi_root_prepare_resources(struct acpi_pci_root_info *ci)
{
	struct resource_entry *entry, *tmp;
	int ret;

	ret = acpi_pci_probe_root_resources(ci);
	if (ret < 0)
		return ret;

	resource_list_for_each_entry_safe(entry, tmp, &ci->resources) {
		struct resource *res = entry->res;

		/*
		 * Special handling for ARM IO range
		 * TODO: need to move pci_register_io_range() function out
		 * of drivers/of/address.c for both used by DT and ACPI
		 */
		if (res->flags & IORESOURCE_IO) {
			unsigned long port;
			int err;
			resource_size_t length = res->end - res->start;

			err = pci_register_io_range(res->start, length);
			if (err) {
				resource_list_destroy_entry(entry);
				continue;
			}

			port = pci_address_to_pio(res->start);
			if (port == (unsigned long)-1) {
				resource_list_destroy_entry(entry);
				continue;
			}

			res->start = port;
			res->end = res->start + length - 1;

			if (pci_remap_iospace(res, res->start) < 0)
				resource_list_destroy_entry(entry);
		}
	}

	return 0;
}

static struct acpi_pci_root_ops acpi_pci_root_ops = {
	.pci_ops = &pci_root_ops,
	.init_info = pci_acpi_root_init_info,
	.release_info = pci_acpi_root_release_info,
	.prepare_resources = pci_acpi_root_prepare_resources,
};

/* Root bridge scanning */
struct pci_bus *pci_acpi_scan_root(struct acpi_pci_root *root)
{
	int node = acpi_get_node(root->device->handle);
	int domain = root->segment;
	int busnum = root->secondary.start;
	struct acpi_pci_root_info *info;
	struct pci_bus *bus;

	if (domain && !pci_domains_supported) {
		pr_warn("PCI %04x:%02x: multiple domains not supported.\n",
			domain, busnum);
		return NULL;
	}

	info = kzalloc_node(sizeof(*info), GFP_KERNEL, node);
	if (!info) {
		dev_err(&root->device->dev,
			"pci_bus %04x:%02x: ignored (out of memory)\n",
			domain, busnum);
		return NULL;
	}

	bus = acpi_pci_root_create(root, &acpi_pci_root_ops, info, root);

	/* After the PCI-E bus has been walked and all devices discovered,
	 * configure any settings of the fabric that might be necessary.
	 */
	if (bus) {
		struct pci_bus *child;

		list_for_each_entry(child, &bus->children, node)
			pcie_bus_configure_settings(child);
	}

	return bus;
}
#endif
