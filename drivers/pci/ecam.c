/*
 * Arch agnostic direct PCI config space access via
 * ECAM (Enhanced Configuration Access Mechanism)
 *
 * Per-architecture code takes care of the mappings, region validation and
 * accesses themselves.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/mutex.h>
#include <linux/rculist.h>
#include <linux/ecam.h>

#include <asm/io.h>
#include <asm/pci_x86.h> /* Temp hack before refactoring arch-specific calls */

#define PREFIX "PCI: "

static DEFINE_MUTEX(pci_mmcfg_lock);

LIST_HEAD(pci_mmcfg_list);

static void __init pci_mmconfig_remove(struct pci_mmcfg_region *cfg)
{
	if (cfg->res.parent)
		release_resource(&cfg->res);
	list_del(&cfg->list);
	kfree(cfg);
}

void __init free_all_mmcfg(void)
{
	struct pci_mmcfg_region *cfg, *tmp;

	pci_mmcfg_arch_free();
	list_for_each_entry_safe(cfg, tmp, &pci_mmcfg_list, list)
		pci_mmconfig_remove(cfg);
}

void list_add_sorted(struct pci_mmcfg_region *new)
{
	struct pci_mmcfg_region *cfg;

	/* keep list sorted by segment and starting bus number */
	list_for_each_entry_rcu(cfg, &pci_mmcfg_list, list) {
		if (cfg->segment > new->segment ||
		    (cfg->segment == new->segment &&
		     cfg->start_bus >= new->start_bus)) {
			list_add_tail_rcu(&new->list, &cfg->list);
			return;
		}
	}
	list_add_tail_rcu(&new->list, &pci_mmcfg_list);
}

struct pci_mmcfg_region *pci_mmconfig_alloc(int segment, int start,
					    int end, u64 addr)
{
	struct pci_mmcfg_region *new;
	struct resource *res;

	if (addr == 0)
		return NULL;

	new = kzalloc(sizeof(*new), GFP_KERNEL);
	if (!new)
		return NULL;

	new->address = addr;
	new->segment = segment;
	new->start_bus = start;
	new->end_bus = end;

	res = &new->res;
	res->start = addr + PCI_MMCFG_BUS_OFFSET(start);
	res->end = addr + PCI_MMCFG_BUS_OFFSET(end + 1) - 1;
	res->flags = IORESOURCE_MEM | IORESOURCE_BUSY;
	snprintf(new->name, PCI_MMCFG_RESOURCE_NAME_LEN,
		 "PCI MMCONFIG %04x [bus %02x-%02x]", segment, start, end);
	res->name = new->name;

	return new;
}

struct pci_mmcfg_region *pci_mmconfig_add(int segment, int start,
					  int end, u64 addr)
{
	struct pci_mmcfg_region *new;

	new = pci_mmconfig_alloc(segment, start, end, addr);
	if (new) {
		mutex_lock(&pci_mmcfg_lock);
		list_add_sorted(new);
		mutex_unlock(&pci_mmcfg_lock);

		pr_info(PREFIX
		       "MMCONFIG for domain %04x [bus %02x-%02x] at %pR "
		       "(base %#lx)\n",
		       segment, start, end, &new->res, (unsigned long)addr);
	}

	return new;
}

struct pci_mmcfg_region *pci_mmconfig_lookup(int segment, int bus)
{
	struct pci_mmcfg_region *cfg;

	list_for_each_entry_rcu(cfg, &pci_mmcfg_list, list)
		if (cfg->segment == segment &&
		    cfg->start_bus <= bus && bus <= cfg->end_bus)
			return cfg;

	return NULL;
}

/* Delete MMCFG information for host bridges */
int pci_mmconfig_delete(u16 seg, u8 start, u8 end)
{
	struct pci_mmcfg_region *cfg;

	mutex_lock(&pci_mmcfg_lock);
	list_for_each_entry_rcu(cfg, &pci_mmcfg_list, list)
		if (cfg->segment == seg && cfg->start_bus == start &&
		    cfg->end_bus == end) {
			list_del_rcu(&cfg->list);
			synchronize_rcu();
			pci_mmcfg_arch_unmap(cfg);
			if (cfg->res.parent)
				release_resource(&cfg->res);
			mutex_unlock(&pci_mmcfg_lock);
			kfree(cfg);
			return 0;
		}
	mutex_unlock(&pci_mmcfg_lock);

	return -ENOENT;
}

int pci_mmconfig_inject(struct pci_mmcfg_region *cfg)
{
	struct pci_mmcfg_region *cfg_conflict;
	int err = 0;

	mutex_lock(&pci_mmcfg_lock);
	cfg_conflict = pci_mmconfig_lookup(cfg->segment, cfg->start_bus);
	if (cfg_conflict) {
		if (cfg_conflict->end_bus < cfg->end_bus)
			pr_info(FW_INFO "MMCONFIG for "
				"domain %04x [bus %02x-%02x] "
				"only partially covers this bridge\n",
				cfg_conflict->segment, cfg_conflict->start_bus,
				cfg_conflict->end_bus);
		err = -EEXIST;
		goto out;
	}

	if (pci_mmcfg_arch_map(cfg)) {
		pr_warn("fail to map MMCONFIG %pR.\n", &cfg->res);
		err = -ENOMEM;
		goto out;
	} else {
		list_add_sorted(cfg);
		pr_info("MMCONFIG at %pR (base %#lx)\n",
			&cfg->res, (unsigned long)cfg->address);

	}
out:
	mutex_unlock(&pci_mmcfg_lock);
	return err;
}
