/*
 * MCFG ACPI table parser.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/acpi.h>
#include <linux/dmi.h>
#include <linux/ecam.h>
#include <linux/pci.h>
#include <linux/pci-acpi.h>

#define	PREFIX	"MCFG: "

/*
 * raw_pci_read/write - raw ACPI PCI config space accessors.
 *
 * By defauly (__weak) these accessors are empty and should be overwritten
 * by architectures which support operations on ACPI PCI_Config regions,
 * see osl.c file.
 */

int __weak raw_pci_read(unsigned int domain, unsigned int bus,
			unsigned int devfn, int reg, int len, u32 *val)
{
	return PCIBIOS_DEVICE_NOT_FOUND;
}

int __weak raw_pci_write(unsigned int domain, unsigned int bus,
			 unsigned int devfn, int reg, int len, u32 val)
{
	return PCIBIOS_DEVICE_NOT_FOUND;
}

extern struct pci_mcfg_fixup __start_acpi_mcfg_fixups[];
extern struct pci_mcfg_fixup __end_acpi_mcfg_fixups[];

static struct pci_ops *pci_mcfg_check_quirks(struct acpi_pci_root *root)
{
	struct pci_mcfg_fixup *f;
	int bus_num = root->secondary.start;
	int domain = root->segment;

	/*
	 * First match against PCI topology <domain:bus> then use DMI or
	 * custom match handler.
	 */
	for (f = __start_acpi_mcfg_fixups; f < __end_acpi_mcfg_fixups; f++) {
		if ((f->domain == domain || f->domain == PCI_MCFG_DOMAIN_ANY) &&
		    (f->bus_num == bus_num || f->bus_num == PCI_MCFG_BUS_ANY) &&
		    (f->system ? dmi_check_system(f->system) : 0 ||
		     f->match ? f->match(f, root) : 0))
			return f->ops;
	}
	return NULL;
}

void __iomem *
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

/* Default generic PCI config accessors */
static struct pci_ops default_pci_mcfg_ops = {
	.map_bus = pci_mcfg_dev_base,
	.read = pci_generic_config_read,
	.write = pci_generic_config_write,
};

struct pci_ops *pci_mcfg_get_ops(struct acpi_pci_root *root)
{
	struct pci_ops *pci_mcfg_ops_quirk;

	/*
	 * Match against platform specific quirks and return corresponding
	 * PCI config space accessor set.
	 */
	pci_mcfg_ops_quirk = pci_mcfg_check_quirks(root);
	if (pci_mcfg_ops_quirk)
		return pci_mcfg_ops_quirk;

	return &default_pci_mcfg_ops;
}

int __init acpi_parse_mcfg(struct acpi_table_header *header)
{
	struct acpi_table_mcfg *mcfg;
	struct acpi_mcfg_allocation *cfg_table, *cfg;
	unsigned long i;
	int entries;

	if (!header)
		return -EINVAL;

	mcfg = (struct acpi_table_mcfg *)header;

	/* how many config structures do we have */
	free_all_mmcfg();
	entries = 0;
	i = header->length - sizeof(struct acpi_table_mcfg);
	while (i >= sizeof(struct acpi_mcfg_allocation)) {
		entries++;
		i -= sizeof(struct acpi_mcfg_allocation);
	}
	if (entries == 0) {
		pr_err(PREFIX "MCFG table has no entries\n");
		return -ENODEV;
	}

	cfg_table = (struct acpi_mcfg_allocation *) &mcfg[1];
	for (i = 0; i < entries; i++) {
		cfg = &cfg_table[i];
		if (acpi_mcfg_check_entry(mcfg, cfg)) {
			free_all_mmcfg();
			return -ENODEV;
		}

		if (pci_mmconfig_add(cfg->pci_segment, cfg->start_bus_number,
				 cfg->end_bus_number, cfg->address) == NULL) {
			pr_warn(PREFIX "no memory for MCFG entries\n");
			free_all_mmcfg();
			return -ENOMEM;
		}
	}

	return 0;
}

int pci_mmcfg_setup_map(struct acpi_pci_root_info *ci)
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

void pci_mmcfg_teardown_map(struct acpi_pci_root_info *ci)
{
	struct acpi_pci_root *root = ci->root;
	struct pci_mmcfg_region *cfg;

	cfg = pci_mmconfig_lookup(root->segment, root->secondary.start);
	if (!cfg)
		return;

	if (cfg->hot_added)
		pci_mmconfig_delete(root->segment, root->secondary.start,
				    root->secondary.end);
}

int __init __weak acpi_mcfg_check_entry(struct acpi_table_mcfg *mcfg,
					struct acpi_mcfg_allocation *cfg)
{
	return 0;
}

void __init __weak pci_mmcfg_early_init(void)
{

}

void __init __weak pci_mmcfg_late_init(void)
{
	struct pci_mmcfg_region *cfg;

	acpi_table_parse(ACPI_SIG_MCFG, acpi_parse_mcfg);

	if (list_empty(&pci_mmcfg_list))
		return;
	if (!pci_mmcfg_arch_init())
		free_all_mmcfg();

	list_for_each_entry(cfg, &pci_mmcfg_list, list)
		insert_resource(&iomem_resource, &cfg->res);
}
