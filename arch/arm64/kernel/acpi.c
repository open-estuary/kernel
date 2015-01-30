/*
 *  ARM64 Specific Low-Level ACPI Boot Support
 *
 *  Copyright (C) 2013-2014, Linaro Ltd.
 *	Author: Al Stone <al.stone@linaro.org>
 *	Author: Graeme Gregory <graeme.gregory@linaro.org>
 *	Author: Hanjun Guo <hanjun.guo@linaro.org>
 *	Author: Tomasz Nowicki <tomasz.nowicki@linaro.org>
 *	Author: Naresh Bhat <naresh.bhat@linaro.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/acpi.h>
#include <linux/cpumask.h>
#include <linux/memblock.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/bootmem.h>
#include <linux/smp.h>

int acpi_noirq;			/* skip ACPI IRQ initialization */
int acpi_disabled;
EXPORT_SYMBOL(acpi_disabled);

int acpi_pci_disabled;		/* skip ACPI PCI scan and IRQ initialization */
EXPORT_SYMBOL(acpi_pci_disabled);

/*
 * __acpi_map_table() will be called before page_init(), so early_ioremap()
 * or early_memremap() should be called here to for ACPI table mapping.
 */
char *__init __acpi_map_table(unsigned long phys, unsigned long size)
{
	if (!phys || !size)
		return NULL;

	return early_memremap(phys, size);
}

void __init __acpi_unmap_table(char *map, unsigned long size)
{
	if (!map || !size)
		return;

	early_memunmap(map, size);
}

/*
 * acpi_boot_table_init() called from setup_arch(), always.
 *	1. find RSDP and get its address, and then find XSDT
 *	2. extract all tables and checksums them all
 *
 * We can parse ACPI boot-time tables such as MADT after
 * this function is called.
 */
void __init acpi_boot_table_init(void)
{
	/* If acpi_disabled, bail out */
	if (acpi_disabled)
		return;

	/* Initialize the ACPI boot-time table parser. */
	if (acpi_table_init())
		disable_acpi();
}

static int __init parse_acpi(char *arg)
{
	if (!arg)
		return -EINVAL;

	/* "acpi=off" disables both ACPI table parsing and interpreter */
	if (strcmp(arg, "off") == 0)
		disable_acpi();
	else if (strcmp(arg, "force") == 0) /* force ACPI to be enabled */
		enable_acpi();
	else
		return -EINVAL;	/* Core will print when we return error */

	return 0;
}
early_param("acpi", parse_acpi);
