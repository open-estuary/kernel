/*
 * NUMA support, based on the x86 implementation.
 *
 * Copyright (C) 2015 Cavium Inc.
 * Author: Ganapatrao Kulkarni <gkulkarni@cavium.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/acpi.h>
#include <linux/bootmem.h>
#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/memblock.h>
#include <linux/module.h>
#include <linux/mmzone.h>
#include <linux/nodemask.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/topology.h>

#include <asm/smp_plat.h>

struct pglist_data *node_data[MAX_NUMNODES] __read_mostly;
EXPORT_SYMBOL(node_data);
nodemask_t numa_nodes_parsed __initdata;
int cpu_to_node_map[NR_CPUS] = { [0 ... NR_CPUS-1] = NUMA_NO_NODE };

static int numa_off;
static int numa_distance_cnt;
static u8 *numa_distance;

static __init int numa_parse_early_param(char *opt)
{
	if (!opt)
		return -EINVAL;
	if (!strncmp(opt, "off", 3)) {
		pr_info("%s\n", "NUMA turned off");
		numa_off = 1;
	}
	return 0;
}
early_param("numa", numa_parse_early_param);

cpumask_var_t node_to_cpumask_map[MAX_NUMNODES];
EXPORT_SYMBOL(node_to_cpumask_map);

#ifdef CONFIG_DEBUG_PER_CPU_MAPS
/*
 * Returns a pointer to the bitmask of CPUs on Node 'node'.
 */
const struct cpumask *cpumask_of_node(int node)
{

	if (WARN_ON(node >= nr_node_ids))
		return cpu_none_mask;

	if (WARN_ON(node_to_cpumask_map[node] == NULL))
		return cpu_online_mask;

	return node_to_cpumask_map[node];
}
EXPORT_SYMBOL(cpumask_of_node);
#endif

static void map_cpu_to_node(unsigned int cpu, int nid)
{
	set_cpu_numa_node(cpu, nid);
	if (nid >= 0)
		cpumask_set_cpu(cpu, node_to_cpumask_map[nid]);
}

static void unmap_cpu_to_node(unsigned int cpu)
{
	int nid = cpu_to_node(cpu);

	if (nid >= 0)
		cpumask_clear_cpu(cpu, node_to_cpumask_map[nid]);
	set_cpu_numa_node(cpu, NUMA_NO_NODE);
}

void numa_clear_node(unsigned int cpu)
{
	unmap_cpu_to_node(cpu);
}

/*
 * Allocate node_to_cpumask_map based on number of available nodes
 * Requires node_possible_map to be valid.
 *
 * Note: cpumask_of_node() is not valid until after this is done.
 * (Use CONFIG_DEBUG_PER_CPU_MAPS to check this.)
 */
static void __init setup_node_to_cpumask_map(void)
{
	unsigned int cpu;
	int node;

	/* setup nr_node_ids if not done yet */
	if (nr_node_ids == MAX_NUMNODES)
		setup_nr_node_ids();

	/* allocate and clear the mapping */
	for (node = 0; node < nr_node_ids; node++) {
		alloc_bootmem_cpumask_var(&node_to_cpumask_map[node]);
		cpumask_clear(node_to_cpumask_map[node]);
	}

	for_each_possible_cpu(cpu)
		set_cpu_numa_node(cpu, NUMA_NO_NODE);

	/* cpumask_of_node() will now work */
	pr_debug("Node to cpumask map for %d nodes\n", nr_node_ids);
}

/*
 *  Set the cpu to node and mem mapping
 */
void numa_store_cpu_info(unsigned int cpu)
{
	map_cpu_to_node(cpu, numa_off ? 0 : cpu_to_node_map[cpu]);
}

/**
 * numa_add_memblk - Set node id to memblk
 * @nid: NUMA node ID of the new memblk
 * @start: Start address of the new memblk
 * @size:  Size of the new memblk
 *
 * RETURNS:
 * 0 on success, -errno on failure.
 */
int __init numa_add_memblk(int nid, u64 start, u64 size)
{
	int ret;

	ret = memblock_set_node(start, size, &memblock.memory, nid);
	if (ret < 0) {
		pr_err("NUMA: memblock [0x%llx - 0x%llx] failed to add on node %d\n",
			start, (start + size - 1), nid);
		return ret;
	}

	node_set(nid, numa_nodes_parsed);
	pr_info("NUMA: Adding memblock [0x%llx - 0x%llx] on node %d\n",
			start, (start + size - 1), nid);
	return ret;
}
EXPORT_SYMBOL(numa_add_memblk);

/* Initialize NODE_DATA for a node on the local memory */
static void __init setup_node_data(int nid, u64 start_pfn, u64 end_pfn)
{
	const size_t nd_size = roundup(sizeof(pg_data_t), SMP_CACHE_BYTES);
	u64 nd_pa;
	void *nd;
	int tnid;

	pr_info("Initmem setup node %d [mem %#010Lx-%#010Lx]\n",
			nid, start_pfn << PAGE_SHIFT,
			(end_pfn << PAGE_SHIFT) - 1);

	nd_pa = memblock_alloc_try_nid(nd_size, SMP_CACHE_BYTES, nid);
	nd = __va(nd_pa);

	/* report and initialize */
	pr_info("  NODE_DATA [mem %#010Lx-%#010Lx]\n",
		nd_pa, nd_pa + nd_size - 1);
	tnid = early_pfn_to_nid(nd_pa >> PAGE_SHIFT);
	if (tnid != nid)
		pr_info("    NODE_DATA(%d) on node %d\n", nid, tnid);

	node_data[nid] = nd;
	memset(NODE_DATA(nid), 0, sizeof(pg_data_t));
	NODE_DATA(nid)->node_id = nid;
	NODE_DATA(nid)->node_start_pfn = start_pfn;
	NODE_DATA(nid)->node_spanned_pages = end_pfn - start_pfn;
}

/**
 * numa_reset_distance - Reset NUMA distance table
 *
 * The current table is freed.
 * The next numa_set_distance() call will create a new one.
 */
void __init numa_reset_distance(void)
{
	size_t size;

	if (!numa_distance)
		return;

	size = numa_distance_cnt * numa_distance_cnt *
		sizeof(numa_distance[0]);

	memblock_free(__pa(numa_distance), size);
	numa_distance_cnt = 0;
	numa_distance = NULL;
}

static int __init numa_alloc_distance(void)
{
	size_t size;
	u64 phys;
	int i, j;

	size = nr_node_ids * nr_node_ids * sizeof(numa_distance[0]);
	phys = memblock_find_in_range(0, PFN_PHYS(max_pfn),
				      size, PAGE_SIZE);
	if (WARN_ON(!phys))
		return -ENOMEM;

	memblock_reserve(phys, size);

	numa_distance = __va(phys);
	numa_distance_cnt = nr_node_ids;

	/* fill with the default distances */
	for (i = 0; i < numa_distance_cnt; i++)
		for (j = 0; j < numa_distance_cnt; j++)
			numa_distance[i * numa_distance_cnt + j] = i == j ?
				LOCAL_DISTANCE : REMOTE_DISTANCE;

	pr_debug("NUMA: Initialized distance table, cnt=%d\n",
			numa_distance_cnt);

	return 0;
}

/**
 * numa_set_distance - Set NUMA distance from one NUMA to another
 * @from: the 'from' node to set distance
 * @to: the 'to'  node to set distance
 * @distance: NUMA distance
 *
 * Set the distance from node @from to @to to @distance.  If distance table
 * doesn't exist, one which is large enough to accommodate all the currently
 * known nodes will be created.
 *
 * If such table cannot be allocated, a warning is printed and further
 * calls are ignored until the distance table is reset with
 * numa_reset_distance().
 *
 * If @from or @to is higher than the highest known node or lower than zero
 * at the time of table creation or @distance doesn't make sense, the call
 * is ignored.
 * This is to allow simplification of specific NUMA config implementations.
 */
void __init numa_set_distance(int from, int to, int distance)
{
	if (!numa_distance)
		return;

	if (from >= numa_distance_cnt || to >= numa_distance_cnt ||
			from < 0 || to < 0) {
		pr_warn_once("NUMA: Warning: node ids are out of bound, from=%d to=%d distance=%d\n",
			    from, to, distance);
		return;
	}

	if ((u8)distance != distance ||
	    (from == to && distance != LOCAL_DISTANCE)) {
		pr_warn_once("NUMA: Warning: invalid distance parameter, from=%d to=%d distance=%d\n",
			     from, to, distance);
		return;
	}

	numa_distance[from * numa_distance_cnt + to] = distance;
}
EXPORT_SYMBOL(numa_set_distance);

int __node_distance(int from, int to)
{
	if (from >= numa_distance_cnt || to >= numa_distance_cnt)
		return from == to ? LOCAL_DISTANCE : REMOTE_DISTANCE;
	return numa_distance[from * numa_distance_cnt + to];
}
EXPORT_SYMBOL(__node_distance);

static int __init numa_register_nodes(void)
{
	int nid;
	struct memblock_region *mblk;

	/* Check that valid nid is set to memblks */
	for_each_memblock(memory, mblk)
		if (mblk->nid == NUMA_NO_NODE || mblk->nid >= MAX_NUMNODES)
			return -EINVAL;

	/* Finally register nodes. */
	for_each_node_mask(nid, numa_nodes_parsed) {
		unsigned long start_pfn, end_pfn;

		get_pfn_range_for_nid(nid, &start_pfn, &end_pfn);
		setup_node_data(nid, start_pfn, end_pfn);
		node_set_online(nid);
	}

	/* Setup online nodes to actual nodes*/
	node_possible_map = numa_nodes_parsed;

	return 0;
}

static int __init numa_init(int (*init_func)(void))
{
	int ret;

	nodes_clear(numa_nodes_parsed);
	nodes_clear(node_possible_map);
	nodes_clear(node_online_map);
	numa_reset_distance();

	ret = init_func();
	if (ret < 0)
		return ret;

	if (nodes_empty(numa_nodes_parsed))
		return -EINVAL;

	ret = numa_register_nodes();
	if (ret < 0)
		return ret;

	ret = numa_alloc_distance();
	if (ret < 0)
		return ret;

	setup_node_to_cpumask_map();

	/* init boot processor */
	cpu_to_node_map[0] = 0;
	map_cpu_to_node(0, 0);

	return 0;
}

/**
 * dummy_numa_init - Fallback dummy NUMA init
 *
 * Used if there's no underlying NUMA architecture, NUMA initialization
 * fails, or NUMA is disabled on the command line.
 *
 * Must online at least one node (node 0) and add memory blocks that cover all
 * allowed memory. It is unlikely that this function fails.
 */
static int __init dummy_numa_init(void)
{
	int ret;
	struct memblock_region *mblk;

	pr_info("%s\n", "No NUMA configuration found");
	pr_info("Faking a node at [mem %#018Lx-%#018Lx]\n",
	       0LLU, PFN_PHYS(max_pfn) - 1);

	for_each_memblock(memory, mblk) {
		ret = numa_add_memblk(0, mblk->base, mblk->size);
		if (unlikely(ret < 0))
			return ret;
	}

	numa_off = 1;
	return 0;
}

/**
 * arm64_numa_init - Initialize NUMA
 *
 * Try each configured NUMA initialization method until one succeeds.  The
 * last fallback is dummy single node config encomapssing whole memory and
 * never fails.
 */
void __init arm64_numa_init(void)
{
	int ret = -ENODEV;

	if (!numa_off)
		ret = numa_init(acpi_disabled ? arm64_of_numa_init : arm64_acpi_numa_init);

	if (ret)
		numa_init(dummy_numa_init);
}
