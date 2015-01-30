#ifndef _ACPI_IO_H_
#define _ACPI_IO_H_

#include <linux/mm.h>
#include <linux/io.h>

static inline void __iomem *acpi_os_ioremap(acpi_physical_address phys,
					    acpi_size size)
{
#ifdef CONFIG_ARM64
	if (!page_is_ram(phys >> PAGE_SHIFT))
		return ioremap(phys, size);
#endif

       return ioremap_cache(phys, size);
}

void __iomem *__init_refok
acpi_os_map_iomem(acpi_physical_address phys, acpi_size size);
void __ref acpi_os_unmap_iomem(void __iomem *virt, acpi_size size);
void __iomem *acpi_os_get_iomem(acpi_physical_address phys, unsigned int size);

int acpi_os_map_generic_address(struct acpi_generic_address *addr);
void acpi_os_unmap_generic_address(struct acpi_generic_address *addr);

#endif
