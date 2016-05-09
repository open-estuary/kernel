/*
 * ACPI 5.1 based NUMA setup for ARM64
 * Lots of code was borrowed from arch/x86/mm/srat.c
 *
 * Copyright 2004 Andi Kleen, SuSE Labs.
 * Copyright (C) 2013-2016, Linaro Ltd.
 *		Author: Hanjun Guo <hanjun.guo@linaro.org>
 *
 * Reads the ACPI SRAT table to figure out what memory belongs to which CPUs.
 *
 * Called from acpi_numa_init while reading the SRAT and SLIT tables.
 * Assumes all memory regions belonging to a single proximity domain
 * are in one chunk. Holes between them will be included in the node.
 */

#define pr_fmt(fmt) "ACPI: NUMA: " fmt

#include <linux/acpi.h>
#include <linux/bitmap.h>
#include <linux/bootmem.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/memblock.h>
#include <linux/mmzone.h>
#include <linux/module.h>
#include <linux/topology.h>

#include <acpi/processor.h>
#include <asm/numa.h>

int acpi_numa __initdata;
static int cpus_in_srat;

struct __node_cpu_hwid {
	u32 node_id;    /* logical node containing this CPU */
	u64 cpu_hwid;   /* MPIDR for this CPU */
};

static struct __node_cpu_hwid early_node_cpu_hwid[NR_CPUS] = {
[0 ... NR_CPUS - 1] = {NUMA_NO_NODE, PHYS_CPUID_INVALID} };

static __init void bad_srat(void)
{
	pr_err("SRAT not used.\n");
	acpi_numa = -1;
}

static __init inline int srat_disabled(void)
{
	return acpi_numa < 0;
}

void __init acpi_numa_set_node_info(unsigned int cpu, u64 hwid)
{
	int nid = 0, i;

	for (i = 0; i < cpus_in_srat; i++) {
		if (hwid == early_node_cpu_hwid[i].cpu_hwid) {
			nid = early_node_cpu_hwid[i].node_id;
			break;
		}
	}

	cpu_to_node_map[cpu] = nid;
}

static int __init get_mpidr_in_madt(int acpi_id, u64 *mpidr)
{
	unsigned long madt_end, entry;
	struct acpi_table_madt *madt;
	acpi_size tbl_size;

	if (ACPI_FAILURE(acpi_get_table_with_size(ACPI_SIG_MADT, 0,
			(struct acpi_table_header **)&madt, &tbl_size)))
		return -ENODEV;

	entry = (unsigned long)madt;
	madt_end = entry + madt->header.length;

	/* Parse all entries looking for a match. */
	entry += sizeof(struct acpi_table_madt);
	while (entry + sizeof(struct acpi_subtable_header) < madt_end) {
		struct acpi_subtable_header *header =
			(struct acpi_subtable_header *)entry;

		if (header->type == ACPI_MADT_TYPE_GENERIC_INTERRUPT) {
			struct acpi_madt_generic_interrupt *gicc =
				container_of(header,
				struct acpi_madt_generic_interrupt, header);

			if ((gicc->flags & ACPI_MADT_ENABLED) &&
			    (gicc->uid == acpi_id)) {
				*mpidr = gicc->arm_mpidr;
				early_acpi_os_unmap_memory(madt, tbl_size);
				return 0;
			}
		}
		entry += header->length;
	}

	early_acpi_os_unmap_memory(madt, tbl_size);
	return -ENODEV;
}

/* Callback for Proximity Domain -> ACPI processor UID mapping */
void __init acpi_numa_gicc_affinity_init(struct acpi_srat_gicc_affinity *pa)
{
	int pxm, node;
	u64 mpidr;

	if (srat_disabled())
		return;

	if (pa->header.length < sizeof(struct acpi_srat_gicc_affinity)) {
		bad_srat();
		return;
	}

	if (!(pa->flags & ACPI_SRAT_GICC_ENABLED))
		return;

	if (cpus_in_srat >= NR_CPUS) {
		pr_warn_once("SRAT: cpu_to_node_map[%d] is too small, may not be able to use all cpus\n",
			     NR_CPUS);
		return;
	}

	pxm = pa->proximity_domain;
	node = acpi_map_pxm_to_node(pxm);

	if (node == NUMA_NO_NODE || node >= MAX_NUMNODES) {
		pr_err("SRAT: Too many proximity domains %d\n", pxm);
		bad_srat();
		return;
	}

	if (get_mpidr_in_madt(pa->acpi_processor_uid, &mpidr)) {
		pr_warn("SRAT: PXM %d with ACPI ID %d has no valid MPIDR in MADT\n",
			pxm, pa->acpi_processor_uid);
		bad_srat();
		return;
	}

	early_node_cpu_hwid[cpus_in_srat].node_id = node;
	early_node_cpu_hwid[cpus_in_srat].cpu_hwid =  mpidr;
	node_set(node, numa_nodes_parsed);
	acpi_numa = 1;
	cpus_in_srat++;
	pr_info("SRAT: PXM %d -> MPIDR 0x%Lx -> Node %d cpu %d\n",
		pxm, mpidr, node, cpus_in_srat);
}

/* Callback for parsing of the Proximity Domain <-> Memory Area mappings */
int __init acpi_numa_memory_affinity_init(struct acpi_srat_mem_affinity *ma)
{
	u64 start, end;
	int node, pxm;

	if (srat_disabled())
		return -EINVAL;

	if (ma->header.length != sizeof(struct acpi_srat_mem_affinity)) {
		bad_srat();
		return -EINVAL;
	}

	/* Ignore disabled entries */
	if (!(ma->flags & ACPI_SRAT_MEM_ENABLED))
		return -EINVAL;

	start = ma->base_address;
	end = start + ma->length;
	pxm = ma->proximity_domain;

	node = acpi_map_pxm_to_node(pxm);

	if (node == NUMA_NO_NODE || node >= MAX_NUMNODES) {
		pr_err("SRAT: Too many proximity domains.\n");
		bad_srat();
		return -EINVAL;
	}

	pr_info("SRAT: Node %u PXM %u [mem %#010Lx-%#010Lx]\n",
		node, pxm,
		(unsigned long long) start, (unsigned long long) end - 1);

	if (numa_add_memblk(node, start, (end - start)) < 0) {
		bad_srat();
		return -EINVAL;
	}

	return 0;
}

int __init arm64_acpi_numa_init(void)
{
	int ret;

	ret = acpi_numa_init();
	if (ret)
		return ret;

	return srat_disabled() ? -EINVAL : 0;
}
