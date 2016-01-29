/*
 * pci_mcfg.c
 *
 * Common code to maintain the MCFG areas and mappings
 *
 * This has been extracted from arch/x86/pci/mmconfig-shared.c
 * and moved here so that other architectures can use this code.
 */

#include <linux/pci.h>
#include <linux/init.h>
#include <linux/dmi.h>
#include <linux/pci-acpi.h>
#include <linux/sfi_acpi.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/rculist.h>

#define PREFIX	"ACPI: "

static DEFINE_MUTEX(pci_mmcfg_lock);
LIST_HEAD(pci_mmcfg_list);

static void list_add_sorted(struct pci_mmcfg_region *new)
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

static struct pci_mmcfg_region *pci_mmconfig_alloc(int segment, int start,
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

/*
 * Map a pci_mmcfg_region, can be overrriden by arch
 */
int __weak pci_mmconfig_map_resource(struct device *dev,
	struct pci_mmcfg_region *mcfg)
{
	struct resource *tmp;
	void __iomem *vaddr;

	tmp = insert_resource_conflict(&iomem_resource, &mcfg->res);
	if (tmp) {
		dev_warn(dev, "MMCONFIG %pR conflicts with %s %pR\n",
			&mcfg->res, tmp->name, tmp);
		return -EBUSY;
	}

	vaddr =  ioremap(mcfg->res.start, resource_size(&mcfg->res));
	if (!vaddr) {
		release_resource(&mcfg->res);
		return -ENOMEM;
	}

	mcfg->virt = vaddr;
	return 0;
}

/*
 * Unmap a pci_mmcfg_region, can be overrriden by arch
 */
void __weak pci_mmconfig_unmap_resource(struct pci_mmcfg_region *mcfg)
{
	if (mcfg->virt) {
		iounmap(mcfg->virt);
		mcfg->virt = NULL;
	}
	if (mcfg->res.parent) {
		release_resource(&mcfg->res);
		mcfg->res.parent = NULL;
	}
}

/*
 * check if the mmconfig is enabled and configured
 */
int __weak pci_mmconfig_enabled(void)
{
	return 1;
}

/* Add MMCFG information for host bridges */
int pci_mmconfig_insert(struct device *dev, u16 seg, u8 start, u8 end,
			phys_addr_t addr)
{
	struct pci_mmcfg_region *cfg;
	int rc;

	if (!pci_mmconfig_enabled())
		return -ENODEV;
	if (start > end)
		return -EINVAL;

	mutex_lock(&pci_mmcfg_lock);
	cfg = pci_mmconfig_lookup(seg, start);
	if (cfg) {
		if (cfg->end_bus < end)
			dev_info(dev, FW_INFO
				 "MMCONFIG for "
				 "domain %04x [bus %02x-%02x] "
				 "only partially covers this bridge\n",
				  cfg->segment, cfg->start_bus, cfg->end_bus);
		rc = -EEXIST;
		goto err;
	}

	if (!addr) {
		rc = -EINVAL;
		goto err;
	}

	cfg = pci_mmconfig_alloc(seg, start, end, addr);
	if (cfg == NULL) {
		dev_warn(dev, "fail to add MMCONFIG (out of memory)\n");
		rc = -ENOMEM;
		goto err;
	}
	rc = pci_mmconfig_map_resource(dev, cfg);
	if (!rc) {
		list_add_sorted(cfg);
		dev_info(dev, "MMCONFIG at %pR (base %#lx)\n",
				 &cfg->res, (unsigned long)addr);
		return 0;
	} else {
		if (cfg->res.parent)
			release_resource(&cfg->res);
		kfree(cfg);
	}

err:
	mutex_unlock(&pci_mmcfg_lock);
	return rc;
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
			pci_mmconfig_unmap_resource(cfg);
			mutex_unlock(&pci_mmcfg_lock);
			kfree(cfg);
			return 0;
		}
	mutex_unlock(&pci_mmcfg_lock);

	return -ENOENT;
}

static int __init acpi_mcfg_check_entry(struct acpi_table_mcfg *mcfg,
					struct acpi_mcfg_allocation *cfg)
{
	int year;

	if (!config_enabled(CONFIG_X86))
		return 0;

	if (cfg->address < 0xFFFFFFFF)
		return 0;

	if (!strncmp(mcfg->header.oem_id, "SGI", 3))
		return 0;

	if (mcfg->header.revision >= 1) {
		if (dmi_get_date(DMI_BIOS_DATE, &year, NULL, NULL) &&
		    year >= 2010)
			return 0;
	}

	pr_err(PREFIX "MCFG region for %04x [bus %02x-%02x] at %#llx "
	       "is above 4GB, ignored\n", cfg->pci_segment,
	       cfg->start_bus_number, cfg->end_bus_number, cfg->address);
	return -EINVAL;
}

static int __init pci_parse_mcfg(struct acpi_table_header *header)
{
	struct acpi_table_mcfg *mcfg;
	struct acpi_mcfg_allocation *cfg_table, *cfg;
	unsigned long i;
	int entries;

	if (!header)
		return -EINVAL;

	mcfg = (struct acpi_table_mcfg *)header;

	/* how many config structures do we have */
	entries = 0;
	i = header->length - sizeof(struct acpi_table_mcfg);
	while (i >= sizeof(struct acpi_mcfg_allocation)) {
		entries++;
		i -= sizeof(struct acpi_mcfg_allocation);
	}
	if (entries == 0) {
		pr_err(PREFIX "MMCONFIG has no entries\n");
		return -ENODEV;
	}

	cfg_table = (struct acpi_mcfg_allocation *) &mcfg[1];
	for (i = 0; i < entries; i++) {
		cfg = &cfg_table[i];
		if (acpi_mcfg_check_entry(mcfg, cfg))
			return -ENODEV;

		if (pci_mmconfig_add(cfg->pci_segment, cfg->start_bus_number,
				   cfg->end_bus_number, cfg->address) == NULL) {
			pr_warn(PREFIX "no memory for MCFG entries\n");
			return -ENOMEM;
		}
	}

	return 0;
}

int __init pci_mmconfig_parse_table(void)
{
	return acpi_sfi_table_parse(ACPI_SIG_MCFG, pci_parse_mcfg);
}

void __weak __init pci_mmcfg_late_init(void)
{
	int err, n = 0;
	struct pci_mmcfg_region *cfg;

	err = pci_mmconfig_parse_table();
	if (err) {
		pr_err(PREFIX " Failed to parse MCFG (%d)\n", err);
		return;
	}

	list_for_each_entry(cfg, &pci_mmcfg_list, list) {
		pci_mmconfig_map_resource(NULL, cfg);
		n++;
	}

	pr_info(PREFIX " MCFG table loaded %d entries\n", n);
}
