/*
 * MCFG ACPI table parser.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/acpi.h>
#include <linux/ecam.h>

#include <asm/pci_x86.h> /* Temp hack before refactoring arch-specific calls */

#define	PREFIX	"MCFG: "

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
