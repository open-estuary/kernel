/*
 *  Copyright (C) 2013-2014, Linaro Ltd.
 *	Author: Al Stone <al.stone@linaro.org>
 *	Author: Graeme Gregory <graeme.gregory@linaro.org>
 *	Author: Hanjun Guo <hanjun.guo@linaro.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation;
 */

#ifndef _ASM_ACPI_H
#define _ASM_ACPI_H

#include <asm/smp_plat.h>

/* Basic configuration for ACPI */
#ifdef	CONFIG_ACPI
#define acpi_strict 1	/* No out-of-spec workarounds on ARM64 */
extern int acpi_disabled;
extern int acpi_noirq;
extern int acpi_pci_disabled;

/* 1 to indicate PSCI 0.2+ is implemented */
static inline bool acpi_psci_present(void)
{
	return acpi_gbl_FADT.arm_boot_flags & ACPI_FADT_PSCI_COMPLIANT;
}

/* 1 to indicate HVC must be used instead of SMC as the PSCI conduit */
static inline bool acpi_psci_use_hvc(void)
{
	return acpi_gbl_FADT.arm_boot_flags & ACPI_FADT_PSCI_USE_HVC;
}

static inline void disable_acpi(void)
{
	acpi_disabled = 1;
	acpi_pci_disabled = 1;
	acpi_noirq = 1;
}

static inline void enable_acpi(void)
{
	acpi_disabled = 0;
	acpi_pci_disabled = 0;
	acpi_noirq = 0;
}

/* MPIDR value provided in GICC structure is 64 bits, but the
 * existing apic_id (CPU hardware ID) using in acpi processor
 * driver is 32-bit, to conform to the same datatype we need
 * to repack the GICC structure MPIDR.
 *
 * Only 32 bits of MPIDR are used:
 *
 * Bits [0:7] Aff0;
 * Bits [8:15] Aff1;
 * Bits [16:23] Aff2;
 * Bits [32:39] Aff3;
 */
static inline u32 pack_mpidr(u64 mpidr)
{
	return (u32) ((mpidr & 0xff00000000) >> 8) | mpidr;
}

/*
 * The ACPI processor driver for ACPI core code needs this macro
 * to find out this cpu was already mapped (mapping from CPU hardware
 * ID to CPU logical ID) or not.
 *
 * cpu_logical_map(cpu) is the mapping of MPIDR and the logical cpu,
 * and MPIDR is the cpu hardware ID we needed to pack.
 */
#define cpu_physical_id(cpu) pack_mpidr(cpu_logical_map(cpu))

/*
 * It's used from ACPI core in kdump to boot UP system with SMP kernel,
 * with this check the ACPI core will not override the CPU index
 * obtained from GICC with 0 and not print some error message as well.
 * Since MADT must provide at least one GICC structure for GIC
 * initialization, CPU will be always available in MADT on ARM64.
 */
static inline bool acpi_has_cpu_in_madt(void)
{
	return true;
}

static inline void arch_fix_phys_package_id(int num, u32 slot) { }
void __init acpi_smp_init_cpus(void);

#else
static inline void disable_acpi(void) { }
static inline bool acpi_psci_present(void) { return false; }
static inline bool acpi_psci_use_hvc(void) { return false; }
static inline void acpi_smp_init_cpus(void) { }
#endif /* CONFIG_ACPI */

#endif /*_ASM_ACPI_H*/
