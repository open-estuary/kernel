#ifndef __ECAM_H
#define __ECAM_H
#ifdef __KERNEL__

#include <linux/types.h>
#include <linux/acpi.h>

/* "PCI MMCONFIG %04x [bus %02x-%02x]" */
#define PCI_MMCFG_RESOURCE_NAME_LEN (22 + 4 + 2 + 2)

struct pci_mmcfg_region {
	struct list_head list;
	struct resource res;
	u64 address;
	char __iomem *virt;
	u16 segment;
	u8 start_bus;
	u8 end_bus;
	char name[PCI_MMCFG_RESOURCE_NAME_LEN];
};

struct pci_mmcfg_region *pci_mmconfig_lookup(int segment, int bus);
struct pci_mmcfg_region *pci_mmconfig_alloc(int segment, int start,
						   int end, u64 addr);
int pci_mmconfig_inject(struct pci_mmcfg_region *cfg);
struct pci_mmcfg_region *pci_mmconfig_add(int segment, int start,
						 int end, u64 addr);
void list_add_sorted(struct pci_mmcfg_region *new);
void free_all_mmcfg(void);
int pci_mmconfig_delete(u16 seg, u8 start, u8 end);

/* Arch specific calls */
int pci_mmcfg_arch_init(void);
void pci_mmcfg_arch_free(void);
int pci_mmcfg_arch_map(struct pci_mmcfg_region *cfg);
void pci_mmcfg_arch_unmap(struct pci_mmcfg_region *cfg);

extern struct list_head pci_mmcfg_list;

#define PCI_MMCFG_BUS_OFFSET(bus)      ((bus) << 20)

#endif  /* __KERNEL__ */
#endif  /* __ECAM_H */
