/*
 * OF NUMA Parsing support.
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

#include <linux/ctype.h>
#include <linux/memblock.h>
#include <linux/module.h>
#include <linux/nodemask.h>
#include <linux/of.h>
#include <linux/of_fdt.h>

#include <asm/smp_plat.h>

/* define default numa node to 0 */
#define DEFAULT_NODE 0
#define OF_NUMA_PROP "numa-node-id"

/* Returns nid in the range [0..MAX_NUMNODES-1],
 * or NUMA_NO_NODE if no valid numa-node-id entry found
 * or DEFAULT_NODE if no numa-node-id entry exists
 */
static int of_numa_prop_to_nid(const __be32 *of_numa_prop, int length)
{
	int nid;

	if (!of_numa_prop)
		return DEFAULT_NODE;

	if (length != sizeof(*of_numa_prop)) {
		pr_warn("NUMA: Invalid of_numa_prop length %d found.\n",
				length);
		return NUMA_NO_NODE;
	}

	nid = of_read_number(of_numa_prop, 1);
	if (nid >= MAX_NUMNODES) {
		pr_warn("NUMA: Invalid numa node %d found.\n", nid);
		return NUMA_NO_NODE;
	}

	return nid;
}

/* Must hold reference to node during call */
static int of_get_numa_nid(struct device_node *device)
{
	int length;
	const __be32 *of_numa_prop;

	of_numa_prop = of_get_property(device, OF_NUMA_PROP, &length);

	return of_numa_prop_to_nid(of_numa_prop, length);
}

static int __init early_init_of_get_numa_nid(unsigned long node)
{
	int length;
	const __be32 *of_numa_prop;

	of_numa_prop = of_get_flat_dt_prop(node, OF_NUMA_PROP, &length);

	return of_numa_prop_to_nid(of_numa_prop, length);
}

/* Walk the device tree upwards, looking for a numa-node-id property */
int of_node_to_nid(struct device_node *device)
{
	struct device_node *parent;
	int nid = NUMA_NO_NODE;

	of_node_get(device);
	while (device) {
		const __be32 *of_numa_prop;
		int length;

		of_numa_prop = of_get_property(device, OF_NUMA_PROP, &length);
		if (of_numa_prop) {
			nid = of_numa_prop_to_nid(of_numa_prop, length);
			break;
		}

		parent = device;
		device = of_get_parent(parent);
		of_node_put(parent);
	}
	of_node_put(device);

	return nid;
}

void __init of_numa_set_node_info(unsigned int cpu, struct device_node *device)
{
	int nid = DEFAULT_NODE;

	if (device)
		nid = of_get_numa_nid(device);

	cpu_to_node_map[cpu] = nid;
}

/*
 * Even though we connect cpus to numa domains later in SMP
 * init, we need to know the node ids now for all cpus.
*/
static int __init early_init_parse_cpu_node(unsigned long node)
{
	int nid;

	const char *type = of_get_flat_dt_prop(node, "device_type", NULL);

	/* We are scanning "cpu" nodes only */
	if (type == NULL)
		return 0;
	else if (strcmp(type, "cpu") != 0)
		return 0;

	nid = early_init_of_get_numa_nid(node);

	if (nid == NUMA_NO_NODE)
		return -EINVAL;

	node_set(nid, numa_nodes_parsed);
	return 0;
}

static int __init early_init_parse_memory_node(unsigned long node)
{
	const __be32 *reg, *endp;
	int length;
	int nid;

	const char *type = of_get_flat_dt_prop(node, "device_type", NULL);

	/* We are scanning "memory" nodes only */
	if (type == NULL)
		return 0;
	else if (strcmp(type, "memory") != 0)
		return 0;

	nid = early_init_of_get_numa_nid(node);

	if (nid == NUMA_NO_NODE)
		return -EINVAL;

	reg = of_get_flat_dt_prop(node, "reg", &length);
	endp = reg + (length / sizeof(__be32));

	while ((endp - reg) >= (dt_root_addr_cells + dt_root_size_cells)) {
		u64 base, size;

		base = dt_mem_next_cell(dt_root_addr_cells, &reg);
		size = dt_mem_next_cell(dt_root_size_cells, &reg);
		pr_debug("NUMA-DT:  base = %llx , node = %u\n",
				base, nid);

		if (numa_add_memblk(nid, base, size) < 0)
			return -EINVAL;
	}

	return 0;
}

static int __init early_init_parse_distance_map_v1(unsigned long node,
		const char *uname)
{

	const __be32 *prop_dist_matrix;
	int length = 0, i, matrix_count;
	int nr_size_cells = OF_ROOT_NODE_SIZE_CELLS_DEFAULT;

	pr_info("NUMA: parsing numa-distance-map-v1\n");

	prop_dist_matrix =
		of_get_flat_dt_prop(node, "distance-matrix", &length);

	if (!length) {
		pr_err("NUMA: failed to parse distance-matrix\n");
		return  -ENODEV;
	}

	matrix_count = ((length / sizeof(__be32)) / (3 * nr_size_cells));

	if ((matrix_count * sizeof(__be32) * 3 * nr_size_cells) !=  length) {
		pr_warn("NUMA: invalid distance-matrix length %d\n", length);
		return -EINVAL;
	}

	for (i = 0; i < matrix_count; i++) {
		u32 nodea, nodeb, distance;

		nodea = dt_mem_next_cell(nr_size_cells, &prop_dist_matrix);
		nodeb = dt_mem_next_cell(nr_size_cells, &prop_dist_matrix);
		distance = dt_mem_next_cell(nr_size_cells, &prop_dist_matrix);
		numa_set_distance(nodea, nodeb, distance);
		pr_debug("NUMA-DT:  distance[node%d -> node%d] = %d\n",
				nodea, nodeb, distance);

		/* Set default distance of node B->A same as A->B */
		if (nodeb > nodea)
			numa_set_distance(nodeb, nodea, distance);
	}

	return 0;
}

static int __init early_init_parse_distance_map(unsigned long node,
		const char *uname)
{

	if (strcmp(uname, "distance-map") != 0)
		return 0;

	if (of_flat_dt_is_compatible(node, "numa-distance-map-v1"))
		return early_init_parse_distance_map_v1(node, uname);

	return -EINVAL;
}

/**
 * early_init_of_scan_numa_map - parse memory node and map nid to memory range.
 */
int __init early_init_of_scan_numa_map(unsigned long node, const char *uname,
				     int depth, void *data)
{
	int ret;

	ret = early_init_parse_cpu_node(node);

	if (!ret)
		ret = early_init_parse_memory_node(node);

	if (!ret)
		ret = early_init_parse_distance_map(node, uname);

	return ret;
}

/* DT node mapping is done already early_init_of_scan_memory */
int __init arm64_of_numa_init(void)
{
	return of_scan_flat_dt(early_init_of_scan_numa_map, NULL);
}
