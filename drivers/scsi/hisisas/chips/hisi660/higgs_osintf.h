/*
 * copyright (C) huawei
 * kernel interface encapsulation 
 */

#ifndef HIGGS_OSINTF_H_
#define HIGGS_OSINTF_H_

#ifndef _PCLINT_
#include "drv_common.h"
#endif

/* memory operation interface */
#define HIGGS_KMALLOC(size, flag) 		SAS_KMALLOC(size, flag)
#define HIGGS_KFREE(ptr) 				SAS_KFREE(ptr)
#define HIGGS_VMALLOC(size) 			SAS_VMALLOC(size)
#define HIGGS_VFREE(ptr) 				SAS_VFREE(ptr)

/* delay interface */
#define HIGGS_UDELAY(us)                udelay(us)
#define HIGGS_MDELAY(ms)                SAS_MDELAY(ms)

/* other */
typedef struct cpumask HIGGS_CPUMASK_S;
#define HIGGS_PLATFORM_GET_RESOURCE(dev, type, id)     platform_get_resource(dev, type, id)
#define HIGGS_DEVM_IOREMAP_RESOURCE(dev, res)          devm_ioremap_resource(dev, res)
#define HIGGS_DEVM_IOUNMAP(dev, addr)                  devm_iounmap(dev, addr)
#define HIGGS_KALLSYMS_LOOKUP_NAME(name)               kallsyms_lookup_name(name)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)	/* >= linux 3.19 */
#define HIGGS_IC_ENABLE_MSI(hwirq, virq)
#define HIGGS_IC_DISABLE_MSI(virq)
#else

#define HIGGS_IC_ENABLE_MSI(hwirq, virq)               ic_enable_msi(hwirq, virq)
#define HIGGS_IC_DISABLE_MSI(virq)                     ic_disable_msi(virq)
#endif

#define HIGGS_CPUMASK_OF(cpuid)						   cpumask_of(cpuid)

typedef struct of_device_id HIGGS_OF_DEVICE_ID_S;
typedef struct device_node HIGGS_DEVICE_NODE_S;
#define  HIGGS_FOR_EACH_MACHING_NODE_AND_MATCH(np, table, match)		for_each_matching_node_and_match(np, table, match)
#define  HIGGS_OF_DEVICE_IS_AVAILABLE(np)								of_device_is_available(np)
#define  HIGGS_OF_IOMAP(np, index)						of_iomap(np, index)
#define  HIGGS_IRQ_OF_PARSE_AND_MAP(np,index)		 	irq_of_parse_and_map(np, index)

#endif /* HIGGS_OSINTF_H_ */
