/*
 * IOMMU API for ARM architected SMMU implementations.
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
 * Copyright (C) 2014 Hisilicon Limited
 *
 * Author: Lei Zhen <thunder.leizhen@huawei.com>
 *
 * This driver currently supports:
 *	- SMMUv3 implementations
 *	- Context fault reporting
 */

#define pr_fmt(fmt) "arm-smmu: " fmt

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iommu.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/mbi.h>

#include <linux/amba/bus.h>

#include <asm/pgalloc.h>

/* Maximum number of stream IDs assigned to a single device */
#define MAX_MASTER_STREAMIDS		MAX_PHANDLE_ARGS

/* Page table bits */
#define ARM_SMMU_PTE_XN			(((pteval_t)3) << 53)
#define ARM_SMMU_PTE_CONT		(((pteval_t)1) << 52)
#define ARM_SMMU_PTE_AF			(((pteval_t)1) << 10)
#define ARM_SMMU_PTE_SH_NS		(((pteval_t)0) << 8)
#define ARM_SMMU_PTE_SH_OS		(((pteval_t)2) << 8)
#define ARM_SMMU_PTE_SH_IS		(((pteval_t)3) << 8)
#define ARM_SMMU_PTE_PAGE		(((pteval_t)3) << 0)

#if PAGE_SIZE == SZ_4K
#define ARM_SMMU_PTE_CONT_ENTRIES	16
#elif PAGE_SIZE == SZ_64K
#define ARM_SMMU_PTE_CONT_ENTRIES	32
#else
#define ARM_SMMU_PTE_CONT_ENTRIES	1
#endif

#define ARM_SMMU_PTE_CONT_SIZE		(PAGE_SIZE * ARM_SMMU_PTE_CONT_ENTRIES)
#define ARM_SMMU_PTE_CONT_MASK		(~(ARM_SMMU_PTE_CONT_SIZE - 1))

/* Stage-1 PTE */
#define ARM_SMMU_PTE_AP_UNPRIV		(((pteval_t)1) << 6)
#define ARM_SMMU_PTE_AP_RDONLY		(((pteval_t)2) << 6)
#define ARM_SMMU_PTE_ATTRINDX_SHIFT	2
#define ARM_SMMU_PTE_nG			(((pteval_t)1) << 11)

/* Stage-2 PTE */
#define ARM_SMMU_PTE_HAP_FAULT		(((pteval_t)0) << 6)
#define ARM_SMMU_PTE_HAP_READ		(((pteval_t)1) << 6)
#define ARM_SMMU_PTE_HAP_WRITE		(((pteval_t)2) << 6)
#define ARM_SMMU_PTE_MEMATTR_OIWB	(((pteval_t)0xf) << 2)
#define ARM_SMMU_PTE_MEMATTR_NC		(((pteval_t)0x5) << 2)
#define ARM_SMMU_PTE_MEMATTR_DEV	(((pteval_t)0x1) << 2)

/* Configuration registers */
#define SMMU_IDR0			0x0
#define SMMU_IDR1			0x4
#define SMMU_IDR2			0x8
#define SMMU_IDR3			0xc
#define SMMU_IDR4			0x10
#define SMMU_IDR5			0x14
#define SMMU_IIDR			0x18
#define SMMU_AIDR			0x1c
#define SMMU_CR0			0x20
#define SMMU_CR0ACK			0x24
#define SMMU_CR1			0x28
#define SMMU_CR2			0x2c
#define SMMU_STATUSR			0x40
#define SMMU_GBPA			0x44
#define SMMU_AGBPA			0x48
#define SMMU_IRQ_CTRL			0x50
#define SMMU_IRQ_CTRLACK		0x54
#define SMMU_GERROR			0x60
#define SMMU_GERRORN			0x64
#define SMMU_GERROR_IRQ_CFG0		0x68
#define SMMU_GERROR_IRQ_CFG1		0x70
#define SMMU_GERROR_IRQ_CFG2		0x74
#define SMMU_STRTAB_BASE		0x80
#define SMMU_STRTAB_BASE_CFG		0x88
#define SMMU_CMDQ_BASE			0x90
#define SMMU_CMDQ_PROD			0x98
#define SMMU_CMDQ_CONS			0x9c
#define SMMU_EVENTQ_BASE		0xa0
#define SMMU_EVENTQ_IRQ_CFG0		0xb0
#define SMMU_EVENTQ_IRQ_CFG1		0xb8
#define SMMU_EVENTQ_IRQ_CFG2		0xbc
#define SMMU_PRIQ_BASE			0xc0
#define SMMU_PRIQ_IRQ_CFG0		0xd0
#define SMMU_PRIQ_IRQ_CFG1		0xd8
#define SMMU_PRIQ_IRQ_CFG2		0xdc
#define SMMU_GATOS_CTR			0x100
#define SMMU_GATOS_SID			0x108
#define SMMU_GATOS_ADDR			0x110
#define SMMU_GATOS_PAR			0x118
#define SMMU_VATOS_SEL			0x180

#define SMMU_EVENTQ_PROD		0x100a8
#define SMMU_EVENTQ_CONS		0x100ac
#define SMMU_PRIQ_PROD			0x100c8
#define SMMU_PRIQ_CONS			0x100cc

#define ACK_REG_OFFSET			(SMMU_CR0ACK - SMMU_CR0)

#define	FILED_MASK(hi, lo)		GENMASK_ULL(hi, lo)
#define	NR_MASK(bits)			FILED_MASK((bits) - 1, 0)
#define	FILED_GET(v, hi, lo)		((FILED_MASK(hi, lo) & (v)) >> (lo))
#define	FILED_SET(v, hi, lo)		(((u64)(v) << (lo)) & FILED_MASK(hi, lo))

#define IDR0_ST_LEVEL_MASK		(3 << 27)
#define IDR0_ST_LEVEL_2			(1 << 27)
#define IDR0_TERM_MODEL			(1 << 26)
#define IDR0_STALL_MODEL_STALL_MASK	(1 << 24)
#define IDR0_STALL_MODEL_STALL		(0 << 24)
#define IDR0_STALL_MODEL_BOTH		(0 << 24)
#define IDR0_STALL_MODEL_TERM		(1 << 24)
#define IDR0_STALL_MODEL_FORCED		(2 << 24)
#define IDR0_TTENDIAN_MASK		(3 << 21)
#define IDR0_TTENDIAN_BE		(3 << 21)
#define IDR0_VATOS			(1 << 20)
#define IDR0_CD2L			(1 << 19)
#define IDR0_VMID16			(1 << 18)
#define IDR0_VMW			(1 << 17)
#define IDR0_PRI			(1 << 16)
#define IDR0_ATOS			(1 << 15)
#define IDR0_SEV			(1 << 14)
#define IDR0_MSI			(1 << 13)
#define IDR0_ASID16			(1 << 12)
#define IDR0_ATS			(1 << 10)
#define IDR0_HYP			(1 << 9)
#define IDR0_DORMHINT			(1 << 8)
#define IDR0_HTTU_MASK			(3 << 6)
#define IDR0_HTTU_NONE			(0 << 6)
#define IDR0_HTTU_ACCESS		(1 << 6)
#define IDR0_HTTU_ACCESS_DIRTY		(2 << 6)
#define IDR0_BTM			(1 << 5)
#define IDR0_COHACC			(1 << 4)
#define IDR0_TTF_AARCH64_MASK		(2 << 2)
#define IDR0_TTF_AARCH64		(2 << 2)
#define IDR0_S1P			(1 << 1)
#define IDR0_S2P			(1 << 0)

#define IDR1_TABLES_PRESET		(1 << 30)
#define IDR1_QUEUES_PRESET		(1 << 29)
#define IDR1_REL			(1 << 28)
#define IDR1_ATTR_TYPES_OVR		(1 << 27)
#define IDR1_ATTR_PERMS_OVR		(1 << 26)
#define IDR1_CMDQS_GET(id)		FILED_GET(id, 25, 21)
#define IDR1_EVENTQS_GET(id)		FILED_GET(id, 20, 16)
#define IDR1_PRIQS_GET(id)		FILED_GET(id, 15, 11)
#define IDR1_SSIDSIZE_GET(id)		FILED_GET(id, 10, 6)
#define IDR1_SIDSIZE_GET(id)		FILED_GET(id, 5, 0)

#define IDR3_HAD			(1 << 2)

#define IDR5_GRAN64K			(1 << 6)
#define IDR5_GRAN16K			(1 << 5)
#define IDR5_GRAN4K			(1 << 4)
#define IDR5_OAS_GET(id)		FILED_GET(id, 2, 0)

#define AIDR_ARCH_REVISION_GET(id)	(FILED_GET(id, 7, 0) + 3)

#define CR0_VMW_MASK			(7 << 6)
#define CR0_VMW_EXACT			(0 << 6)
#define CR0_ATSCHK_FAST			(0 << 4)
#define CR0_ATSCHK_SAFE			(1 << 4)
#define CR0_CMDQEN			(1 << 3)
#define CR0_EVENTQEN			(1 << 2)
#define CR0_PRIQEN			(1 << 1)
#define CR0_SMMUEN			(1 << 0)

#define CR1_TABLE_MASK			FILED_MASK(11, 6)
#define CR1_TABLE_SH_ISH		(3 << 10)
#define CR1_TABLE_OC_WB			(1 << 8)
#define CR1_TABLE_IC_WB			(1 << 6)
#define CR1_QUEUE_MASK			FILED_MASK(5, 0)
#define CR1_QUEUE_SH_ISH		(3 << 4)
#define CR1_QUEUE_OC_WB			(1 << 2)
#define CR1_QUEUE_IC_WB			(1 << 0)

#define CR2_PTM				(1 << 2)
#define CR2_RECINVSID			(1 << 1)
#define CR2_E2H				(1 << 0)

#define IRQ_CTRL_EVENTQ_IRQEN		(1 << 2)
#define IRQ_CTRL_PRIQ_IRQEN		(1 << 1)
#define IRQ_CTRL_GERROR_IRQEN		(1 << 0)

#define GERROR_SFM_ERR			(1 << 8)
#define GERROR_MSI_GERROR_ABT_ERR	(1 << 7)
#define GERROR_MSI_PRIQ_ABT_ERR		(1 << 6)
#define GERROR_MSI_EVENTQ_ABT_ERR	(1 << 5)
#define GERROR_MSI_CMDQ_ABT_ERR		(1 << 4)
#define GERROR_PRIQ_ABT_ERR		(1 << 3)
#define GERROR_EVENTQ_ABT_ERR		(1 << 2)
#define GERROR_CMDQ_ERR			(1 << 0)

#define STRTAB_BASE_RA			((u64)0x1 << 62)
#define STRTAB_BASE_ADDR_MASK		FILED_MASK(47, 6)

#define STRTAB_BASE_CFG_FMT_2LEVEL	((u32)0x1 << 16)
#define STRTAB_BASE_CFG_SPLIT_GET(v)	FILED_GET(v, 10, 6)
#define STRTAB_BASE_CFG_SPLIT_SET(v)	FILED_SET((v) - 6, 10, 6)
#define STRTAB_BASE_CFG_LOG2SIZE_GET(v)	FILED_GET(v, 5, 0)

#define CMDQ_BASE_RA			((u64)0x1 << 62)
#define CMDQ_BASE_ADDR_MASK		FILED_MASK(47, 5)
#define CMDQ_BASE_LOG2SIZE_GET(v)	FILED_GET(v, 4, 0)

#define CMDQ_CONS_ERR_GET(v)		FILED_GET(v, 30, 24)
#define CERROR_NONE			0x00
#define CERROR_ILL			0x01
#define CERROR_ABT			0x02

#define EVENTQ_BASE_WA			((u64)0x1 << 62)
#define EVENTQ_BASE_ADDR_MASK		FILED_MASK(47, 5)
#define EVENTQ_BASE_LOG2SIZE_GET(v)	FILED_GET(v, 4, 0)

#define EVENTQ_PROD_OVFLG		(1 << 31)
#define EVENTQ_CONS_OVACKFLG		(1 << 31)

#define LEVEL1_ENTRY_SIZE		8
#define ST_ENTRY_SIZE			64
#define CD_ENTRY_SIZE			64
#define CMDQ_ENTRY_SIZE			16
#define CMDQ_ENTRY_DATA_NR		(CMDQ_ENTRY_SIZE / sizeof(u32))
#define EVENTQ_ENTRY_SIZE		32
#define EVENTQ_ENTRY_DATA_NR		(EVENTQ_ENTRY_SIZE / sizeof(u32))
#define TBLSIZE_GET(bits, entry_size)	((entry_size) * ((u32)0x1 << (bits)))
#define STD_TBLSIZE_GET(bits)		TBLSIZE_GET(bits, LEVEL1_ENTRY_SIZE)
#define STE_TBLSIZE_GET(bits)		TBLSIZE_GET(bits, ST_ENTRY_SIZE)
#define CMDQ_TBLSIZE_GET(bits)		TBLSIZE_GET(bits, CMDQ_ENTRY_SIZE)
#define EVENTQ_TBLSIZE_GET(bits)	TBLSIZE_GET(bits, EVENTQ_ENTRY_SIZE)

#define SMMU_FEAT_COHERENT_WALK		(1 << 0)
#define SMMU_FEAT_STALL			(1 << 1)
#define SMMU_FEAT_TRANS_S1		(1 << 2)
#define SMMU_FEAT_TRANS_S2		(1 << 3)
#define SMMU_FEAT_ST_2LEVEL		(1 << 4)
#define SMMU_FEAT_CD_2LEVEL		(1 << 5)
#define SMMU_FEAT_SEV			(1 << 6)
#define SMMU_FEAT_PRI			(1 << 7)

#define RESUME_RETRY			(1 << 12)
#define RESUME_TERMINATE		(1 << 13)

#define CMD_SYNC_CS_SIG_SEV		(2 << 12)

#define STD_L2PTR_GET(v)		(FILED_MASK(47, 6) & (v))
#define STE_S1PTR_GET(v)		(FILED_MASK(47, 6) & (v))

#define STE_V				(1 << 0)
#define STE_CONFIG_BYPASS		(4 << 1)
#define STE_CONFIG_S1			(5 << 1)
#define STE_CONFIG_S2			(6 << 1)
#define STE_CONFIG_S1S2			(7 << 1)
#define STE_S1FMT_LINEAR		(0 << 4)
#define STE_S1FMT_4K			(1 << 4)
#define STE_S1FMT_64K			(2 << 4)
#define STE_DSS_TERM			(0 << 0)
#define STE_DSS_BYPASS_S1		(1 << 0)
#define STE_DSS_USE_SS0			(2 << 0)
#define STE_S1CIR_WBRA			(1 << 2)
#define STE_S1COR_WBRA			(1 << 4)
#define STE_S1CSH_IS			(3 << 6)

#define CDE_T0SZ_SET(v)			FILED_SET(v, 5, 0)
#define CDE_TG0_4K			((u64)0 << 6)
#define CDE_TG0_64K			((u64)1 << 6)
#define CDE_IR0_WBRAWA			((u64)1 << 8)
#define CDE_OR0_WBRAWA			((u64)1 << 10)
#define CDE_SH0_IS			((u64)3 << 12)
#define CDE_ENDI_BE			((u64)1 << 15)
#define CDE_EPD1			((u64)1 << 30)
#define CDE_V				((u64)1 << 31)
#define CDE_IPS_SHIFT			32
#define ADDR_SIZE_32			0
#define ADDR_SIZE_36			1
#define ADDR_SIZE_40			2
#define ADDR_SIZE_42			3
#define ADDR_SIZE_44			4
#define ADDR_SIZE_48			5
#define CDE_AFFD			((u64)1 << 35)
#define CDE_WXN				((u64)1 << 36)
#define CDE_UWXN			((u64)1 << 37)
#define CDE_TBI0			((u64)1 << 38)
#define CDE_TBI1			((u64)1 << 39)
#define CDE_PAN				((u64)1 << 40)
#define CDE_AA64			((u64)1 << 41)
#define CDE_S				((u64)1 << 44)
#define CDE_R				((u64)1 << 45)
#define CDE_A				((u64)1 << 46)
#define CDE_ASET			((u64)1 << 47)
#define CDE_ASID_SET(v)			FILED_SET(v, 63, 48)

#define MAIR_ATTR_SHIFT(n)		((n) << 3)
#define MAIR_ATTR_MASK			0xff
#define MAIR_ATTR_DEVICE		0x04
#define MAIR_ATTR_NC			0x44
#define MAIR_ATTR_WBRWA			0xff
#define MAIR_ATTR_IDX_NC		0
#define MAIR_ATTR_IDX_CACHE		1
#define MAIR_ATTR_IDX_DEV		2

#define CMD_PREFETCH_CONFIG		0x01
#define CMD_PREFETCH_ADDR		0x02
#define CMD_CFGI_STE			0x03
#define CMD_CFGI_STE_RANGE		0x04
#define CMD_CFGI_CD			0x05
#define CMD_CFGI_CD_ALL			0x06
#define CMD_TLBI_NH_ALL			0x10
#define CMD_TLBI_NH_ASID		0x11
#define CMD_TLBI_NH_VA			0x12
#define CMD_TLBI_NH_VAA			0x13
#define CMD_TLBI_EL3_ALL		0x18
#define CMD_TLBI_EL3_VA			0x1a
#define CMD_TLBI_EL2_ALL		0x20
#define CMD_TLBI_EL2_ASID		0x21
#define CMD_TLBI_EL2_VA			0x22
#define CMD_TLBI_EL2_VAA		0x23
#define CMD_TLBI_S12_VMALL		0x28
#define CMD_TLBI_S2_IPA			0x2a
#define CMD_TLBI_NSNH_ALL		0x30
#define CMD_ATC_INV			0x40
#define CMD_PRI_RESP			0x41
#define CMD_RESUME			0x44
#define CMD_STALL_TERM			0x45
#define CMD_SYNC			0x46

#define F_UUT				0x01
#define C_BAD_STREAMID			0x02
#define F_STE_FETCH			0x03
#define C_BAD_STE			0x04
#define F_BAD_ATS_TREQ			0x05
#define F_STREAM_DISABLED		0x06
#define F_TRANSL_FORBIDDEN		0x07
#define C_BAD_SUBSTREAMID		0x08
#define F_CD_FETCH			0x09
#define C_BAD_CD			0x0a
#define F_WALK_EABT			0x0b
#define F_TRANSLATION			0x10
#define F_ADDR_SIZE			0x11
#define F_ACCESS			0x12
#define F_PERMISSION			0x13
#define F_TLB_CONFLICT			0x20
#define F_CFG_CONFLICT			0x21
#define E_PAGE_REQUEST			0x24

#define F_TYPE_GET(evt)			FILED_GET((evt)->data[0], 7, 0)
#define F_SID_GET(evt)			((evt)->data[1])
#define F_STALL_GET(evt)		FILED_GET((evt)->data[2], 31, 31)
#define F_STAG_GET(evt)			FILED_GET((evt)->data[2], 15, 0)
#define F_CLASS_GET(evt)		FILED_GET((evt)->data[3], 9, 8)
#define F_CLASS_CD			0
#define F_CLASS_TTD			1
#define F_CLASS_IN			2
#define F_S2_GET(evt)			FILED_GET((evt)->data[3], 7, 7)
#define F_RNW_GET(evt)			FILED_GET((evt)->data[3], 3, 3)
#define F_IA_GET(evt)	\
	((evt)->data[4] | ((u64)(evt)->data[5] << 32))
#define F_IPA_GET(evt)	\
	(((evt)->data[6] & ~0xfff) | ((u64)((evt)->data[7] & 0xffff) << 32))

enum {
	SMMU_IRQ_GERROR,
	SMMU_IRQ_EVENTQ,
	SMMU_IRQ_PRIQ,
	SMMU_IRQ_SYNC,
	SMMU_IRQ_MAX,
};


struct arm_smmu_master_cfg {
	u32				sid;
	int				num_ssid;
	u32				*ssid;
};

struct arm_smmu_master {
	struct device_node		*of_node;
	struct rb_node			node;
	struct arm_smmu_master_cfg	cfg;
};

struct arm_smmu_cmd {
	u32 data[CMDQ_ENTRY_DATA_NR];
};

struct arm_smmu_event {
	u32 data[EVENTQ_ENTRY_DATA_NR];
};

struct arm_smmu_ste {
	u64 data[8];
};

struct arm_smmu_cde {
	u64 data[8];
};


struct arm_smmu_device {
	struct device			*dev;

	void __iomem			*base;
	unsigned long			size;

	u32				features;

	u32				std_span;
	u32				sidsize;
	u32				cmdq_mask;
	u32				eventq_mask;

	void				*strtab;
	struct arm_smmu_cmd		*cmdq;
	struct arm_smmu_event		*eventq;

	u32				options;
	int				version;

	u32				num_mapping_groups;

	unsigned long			input_size;
	unsigned long			s1_output_size;
	unsigned long			s2_output_size;

	u32				num_irqs;
	u32				irqs[SMMU_IRQ_MAX];

	struct list_head		list;
	struct rb_root			masters;
};

struct arm_smmu_cfg {
	u32				sid;
	struct arm_smmu_cde		*cde;
	pgd_t				*pgd;
};

enum arm_smmu_domain_stage {
	ARM_SMMU_DOMAIN_S1 = 0,
	ARM_SMMU_DOMAIN_S2,
	ARM_SMMU_DOMAIN_NESTED,
};

struct arm_smmu_domain {
	struct arm_smmu_device		*smmu;
	struct arm_smmu_cfg		cfg;
	enum arm_smmu_domain_stage	stage;
	spinlock_t			lock;
};

static DEFINE_SPINLOCK(arm_smmu_devices_lock);
static LIST_HEAD(arm_smmu_devices);
static DEFINE_SPINLOCK(smmu_cmdq_lock);
static DEFINE_SPINLOCK(smmu_eventq_lock);
static DEFINE_SPINLOCK(smmu_ste_lock);

static void *get_std_addr(struct arm_smmu_device *smmu, u32 sid)
{
	return smmu->strtab + (LEVEL1_ENTRY_SIZE * (sid >> smmu->std_span));
}

static struct arm_smmu_ste *get_ste_addr(struct arm_smmu_device *smmu, u32 sid)
{
	u64 val;
	void *addr;
	struct arm_smmu_ste *ste;
	unsigned long flags;

	if (!(smmu->features & SMMU_FEAT_ST_2LEVEL)) {
		ste = (struct arm_smmu_ste *)smmu->strtab + sid;

		return ste;
	}

	addr = get_std_addr(smmu, sid);
	val = readq_relaxed(addr);
	if (unlikely(!val)) {
		struct page *tbl_page;

		tbl_page = alloc_page(GFP_KERNEL | __GFP_ZERO);
		if (!tbl_page) {
			pr_err("failed to allocate st memory\n");
			return NULL;
		}

		spin_lock_irqsave(&smmu_ste_lock, flags);
		val = readq_relaxed(addr);
		if (likely(!val)) {
			val = page_to_phys(tbl_page) | (smmu->std_span + 1);
			writeq_relaxed(val, addr);
			tbl_page = NULL;
		}
		spin_unlock_irqrestore(&smmu_ste_lock, flags);

		if (tbl_page)
			__free_page(tbl_page);
	}

	sid &= NR_MASK(smmu->std_span);
	ste = (struct arm_smmu_ste *)phys_to_virt(STD_L2PTR_GET(val)) + sid;

	return ste;
}

static void writel_and_wait(u32 val, void __iomem *addr)
{
	u32 ack;

	writel_relaxed(val, addr);
	dsb(sy);

	do {
		cpu_relax();
		ack = readl_relaxed(addr + ACK_REG_OFFSET);
	} while (ack != val);
}

static void cmd_write(struct arm_smmu_device *smmu,
		      struct arm_smmu_cmd *new_cmd)
{
	int i;
	u32 prod, cons;
	u32 idx_mask;
	unsigned long flags;
	struct arm_smmu_cmd *cmd_prod;
	void __iomem *base = smmu->base;

try_again:
	spin_lock_irqsave(&smmu_cmdq_lock, flags);

	idx_mask = smmu->cmdq_mask >> 1;

	/*
	 * There is no need to check command error here. When smmu detected
	 * command error, it's solely stop command consumption, software can
	 * continue to submit additional commands.
	 *
	 * Please refer clause 9.1 of SMMUv3 spec.
	 */
	prod = readl_relaxed(base + SMMU_CMDQ_PROD);
	cons = readl_relaxed(base + SMMU_CMDQ_CONS);

	/* cmdq is full */
	if (((prod & idx_mask) == (cons & idx_mask)) &&
		((prod & smmu->cmdq_mask) != (cons & smmu->cmdq_mask))) {
		spin_unlock_irqrestore(&smmu_cmdq_lock, flags);

		if (smmu->features & SMMU_FEAT_SEV)
			wfe();
		else
			cpu_relax();

		goto try_again;
	}

	cmd_prod = &smmu->cmdq[(prod & idx_mask)];
	for (i = 0; i < CMDQ_ENTRY_DATA_NR; i++)
		writel_relaxed(new_cmd->data[i], &cmd_prod->data[i]);
	dsb(sy);

	writel_relaxed((prod + 1) & smmu->cmdq_mask, base + SMMU_CMDQ_PROD);
	spin_unlock_irqrestore(&smmu_cmdq_lock, flags);
}

static void cmd_cfgi_ste(struct arm_smmu_device *smmu, u32 sid)
{
	struct arm_smmu_cmd new_cmd;

	new_cmd.data[0] = CMD_CFGI_STE;
	new_cmd.data[1] = sid;
	new_cmd.data[2] = 0;
	new_cmd.data[3] = 0;

	cmd_write(smmu, &new_cmd);
}

static void cmd_cfgi_ste_range(struct arm_smmu_device *smmu, u32 sid, u32 span)
{
	struct arm_smmu_cmd new_cmd;

	new_cmd.data[0] = CMD_CFGI_STE_RANGE;
	new_cmd.data[1] = sid;
	new_cmd.data[2] = span;
	new_cmd.data[3] = 0;

	cmd_write(smmu, &new_cmd);
}

#if 0
static void cmd_cfgi_cd(struct arm_smmu_device *smmu, u32 sid)
{
	struct arm_smmu_cmd new_cmd;

	new_cmd.data[0] = CMD_CFGI_CD;
	new_cmd.data[1] = sid;
	new_cmd.data[2] = 0;
	new_cmd.data[3] = 0;

	cmd_write(smmu, &new_cmd);
}

static void cmd_cfgi_cd_all(struct arm_smmu_device *smmu, u32 sid)
{
	struct arm_smmu_cmd new_cmd;

	new_cmd.data[0] = CMD_CFGI_CD_ALL;
	new_cmd.data[1] = sid;
	new_cmd.data[2] = 0;
	new_cmd.data[3] = 0;

	cmd_write(smmu, &new_cmd);
}
#endif

static void cmd_cfgi_all(struct arm_smmu_device *smmu)
{
	cmd_cfgi_ste_range(smmu, 0, 31);
}

#if 0
static void cmd_tlbi_nh_all(struct arm_smmu_device *smmu)
{
	struct arm_smmu_cmd new_cmd;

	new_cmd.data[0] = CMD_TLBI_NH_ALL;
	new_cmd.data[1] = 0;
	new_cmd.data[2] = 0;
	new_cmd.data[3] = 0;

	cmd_write(smmu, &new_cmd);
}
#endif

static void cmd_tlbi_nh_asid(struct arm_smmu_device *smmu, u32 asid)
{
	struct arm_smmu_cmd new_cmd;

	new_cmd.data[0] = CMD_TLBI_NH_ASID;
	new_cmd.data[1] = asid << 16;
	new_cmd.data[2] = 0;
	new_cmd.data[3] = 0;

	cmd_write(smmu, &new_cmd);
}

static void cmd_tlbi_nsnh_all(struct arm_smmu_device *smmu)
{
	struct arm_smmu_cmd new_cmd;

	new_cmd.data[0] = CMD_TLBI_NSNH_ALL;
	new_cmd.data[1] = 0;
	new_cmd.data[2] = 0;
	new_cmd.data[3] = 0;

	cmd_write(smmu, &new_cmd);
}

static void cmd_resume(struct arm_smmu_device *smmu,
			int resume, u32 sid, u16 stag)
{
	struct arm_smmu_cmd new_cmd;

	new_cmd.data[0] = resume | CMD_RESUME;
	new_cmd.data[1] = sid;
	new_cmd.data[2] = stag;
	new_cmd.data[3] = 0;

	cmd_write(smmu, &new_cmd);
}

#if 0
static void cmd_stall_term(struct arm_smmu_device *smmu, u32 sid)
{
	struct arm_smmu_cmd new_cmd;

	new_cmd.data[0] = CMD_STALL_TERM;
	new_cmd.data[1] = sid;
	new_cmd.data[2] = 0;
	new_cmd.data[3] = 0;

	cmd_write(smmu, &new_cmd);
}
#endif

static void cmd_wait_for_completion(struct arm_smmu_device *smmu)
{
	u32 prod, cons, prev;
	s32 remain;
	unsigned long flags;
	void __iomem *base = smmu->base;

	spin_lock_irqsave(&smmu_cmdq_lock, flags);
	prod = readl_relaxed(base + SMMU_CMDQ_PROD);
	cons = readl_relaxed(base + SMMU_CMDQ_CONS);
	spin_unlock_irqrestore(&smmu_cmdq_lock, flags);

	/*
	 * 1. (prod.wrap == cons.wrap) && (prod.index >= cons.index).
	 * 2. (prod.wrap != cons.wrap) && (prod.index <= cons.index).
	 */
	remain = (prod - cons) & smmu->cmdq_mask;

	/*
	 * SMMU maybe update SMMU_CMDQ_CONS in a batch fashion, and there maybe
	 * some other commands been added into command queue during waiting.
	 *
	 * To ensure the specified command have been consumed, meet any one of
	 * the following conditions :
	 * 1. Command queue become empty.
	 * 2. Current consume index exceed original product index.
	 */
	while ((prod & smmu->cmdq_mask) != (cons & smmu->cmdq_mask)) {
		if (smmu->features & SMMU_FEAT_SEV)
			wfe();
		else
			cpu_relax();

		prev = cons;
		prod = readl_relaxed(base + SMMU_CMDQ_PROD);
		cons = readl_relaxed(base + SMMU_CMDQ_CONS);
		remain -= (cons - prev) & (smmu->cmdq_mask >> 1);
		if (remain <= 0)
			break;
	}
}

static void cmd_sync(struct arm_smmu_device *smmu)
{
	struct arm_smmu_cmd new_cmd;

	new_cmd.data[0] = CMD_SYNC;
	new_cmd.data[1] = 0;
	new_cmd.data[2] = 0;
	new_cmd.data[3] = 0;

	if (smmu->features & SMMU_FEAT_SEV)
		new_cmd.data[0] |= CMD_SYNC_CS_SIG_SEV;

	cmd_write(smmu, &new_cmd);
	cmd_wait_for_completion(smmu);
}

static int event_read(struct arm_smmu_device *smmu, struct arm_smmu_event *evt)
{
	int i, overflow;
	u32 prod, cons;
	unsigned long flags;
	struct arm_smmu_event *evt_cons;
	void __iomem *base = smmu->base;

	spin_lock_irqsave(&smmu_eventq_lock, flags);

	prod = readl_relaxed(base + SMMU_EVENTQ_PROD);
	cons = readl_relaxed(base + SMMU_EVENTQ_CONS);

	/* eventq is empty */
	if ((prod & smmu->cmdq_mask) == (cons & smmu->cmdq_mask)) {
		spin_unlock_irqrestore(&smmu_eventq_lock, flags);
		return -EAGAIN;
	}

	evt_cons = &smmu->eventq[cons & (smmu->eventq_mask >> 1)];
	for (i = 0; i < EVENTQ_ENTRY_DATA_NR; i++)
		evt->data[i] = readl_relaxed(&evt_cons->data[i]);

	/*
	 * Read SMMU_EVENTQ_PROD again to minimize the occurrence that event
	 * queue state changed from normal to overflow during this short time.
	 */
	prod = readl_relaxed(base + SMMU_EVENTQ_PROD);
	overflow = (EVENTQ_PROD_OVFLG & prod) ^ (EVENTQ_CONS_OVACKFLG & cons);
	cons = ((cons + 1) & smmu->eventq_mask) | (EVENTQ_PROD_OVFLG & prod);
	writel_relaxed(cons, base + SMMU_EVENTQ_CONS);

	spin_unlock_irqrestore(&smmu_eventq_lock, flags);

	if (overflow)
		dev_warn(smmu->dev, "Event queue overflowed!\n");

	return 0;
}

static int eventq_empty(struct arm_smmu_device *smmu)
{
	u32 prod, cons;
	unsigned long flags;
	void __iomem *base = smmu->base;

	spin_lock_irqsave(&smmu_eventq_lock, flags);
	prod = readl_relaxed(base + SMMU_EVENTQ_PROD);
	cons = readl_relaxed(base + SMMU_EVENTQ_CONS);
	spin_unlock_irqrestore(&smmu_eventq_lock, flags);

	return ((prod & smmu->cmdq_mask) == (cons & smmu->cmdq_mask));
}

static struct device *dev_get_master_dev(struct device *dev)
{
	if (dev_is_pci(dev)) {
		struct pci_bus *bus = to_pci_dev(dev)->bus;

		while (!pci_is_root_bus(bus))
			bus = bus->parent;
		return bus->bridge->parent;
	}

	return dev;
}

static struct arm_smmu_master *find_smmu_master(struct arm_smmu_device *smmu,
						struct device_node *dev_node)
{
	struct rb_node *node = smmu->masters.rb_node;

	while (node) {
		struct arm_smmu_master *master;

		master = container_of(node, struct arm_smmu_master, node);

		if (dev_node < master->of_node)
			node = node->rb_left;
		else if (dev_node > master->of_node)
			node = node->rb_right;
		else
			return master;
	}

	return NULL;
}

static struct arm_smmu_master_cfg *
find_smmu_master_cfg(struct arm_smmu_device *smmu, struct device *dev)
{
	struct arm_smmu_master *master;

	if (dev_is_pci(dev))
		return dev->archdata.iommu;

	master = find_smmu_master(smmu, dev->of_node);
	return master ? &master->cfg : NULL;
}

static int insert_smmu_master(struct arm_smmu_device *smmu,
			      struct arm_smmu_master *master)
{
	struct rb_node **new, *parent;

	new = &smmu->masters.rb_node;
	parent = NULL;
	while (*new) {
		struct arm_smmu_master *this
			= container_of(*new, struct arm_smmu_master, node);

		parent = *new;
		if (master->of_node < this->of_node)
			new = &((*new)->rb_left);
		else if (master->of_node > this->of_node)
			new = &((*new)->rb_right);
		else
			return -EEXIST;
	}

	rb_link_node(&master->node, parent, new);
	rb_insert_color(&master->node, &smmu->masters);
	return 0;
}

static int register_smmu_master(struct arm_smmu_device *smmu,
				struct device *dev,
				struct of_phandle_args *masterspec)
{
	struct arm_smmu_master *master;

	master = find_smmu_master(smmu, masterspec->np);
	if (master) {
		dev_err(dev,
			"rejecting multiple registrations for master device %s\n",
			masterspec->np->name);
		return -EBUSY;
	}

	if (masterspec->args_count > MAX_MASTER_STREAMIDS) {
		dev_err(dev,
			"reached maximum number (%d) of stream IDs for master device %s\n",
			MAX_MASTER_STREAMIDS, masterspec->np->name);
		return -ENOSPC;
	}

	master = devm_kzalloc(dev, sizeof(*master), GFP_KERNEL);
	if (!master)
		return -ENOMEM;

	master->of_node	= masterspec->np;
	master->cfg.sid = masterspec->args[0];

	return insert_smmu_master(smmu, master);
}

static struct arm_smmu_device *find_smmu_for_device(struct device *dev)
{
	struct arm_smmu_device *smmu;
	struct arm_smmu_master *master = NULL;
	struct device_node *dev_node = dev_get_master_dev(dev)->of_node;

	spin_lock(&arm_smmu_devices_lock);
	list_for_each_entry(smmu, &arm_smmu_devices, list) {
		master = find_smmu_master(smmu, dev_node);
		if (master)
			break;
	}
	spin_unlock(&arm_smmu_devices_lock);

	return master ? smmu : NULL;
}

static irqreturn_t arm_smmu_gerror_handler(int irq, void *dev)
{
	struct arm_smmu_device *smmu = dev;
	void __iomem *base = smmu->base;
	u32 gerror;
	static u32 gerrorn;

	gerror  = readl_relaxed(base + SMMU_GERROR);
	gerror ^= gerrorn;
	dev_warn(smmu->dev, "global error: 0x%x\n", gerror);

	if (gerror & GERROR_CMDQ_ERR) {
		u32 cons;
		struct arm_smmu_cmd *cmd;

		cons = readl_relaxed(base + SMMU_CMDQ_CONS);

		switch (CMDQ_CONS_ERR_GET(cons)) {
		case CERROR_ILL:
			cmd = &smmu->cmdq[(cons & (smmu->cmdq_mask >> 1))];
			pr_warn("bad cmd(0x%x): %#x, %#x, %#x, %#x\n", cons,
				readl_relaxed(&cmd->data[0]),
				readl_relaxed(&cmd->data[1]),
				readl_relaxed(&cmd->data[2]),
				readl_relaxed(&cmd->data[3]));

			/* replace bad cmd with cmd_sync to simply skip it */
			writel_relaxed(CMD_SYNC, &cmd->data[0]);
			writel_relaxed(0, &cmd->data[1]);
			writel_relaxed(0, &cmd->data[2]);
			writel_relaxed(0, &cmd->data[3]);
			dsb(ishst);
			break;

		case CERROR_NONE:
			break;

		default:
			panic("command error: 0x%x\n", cons);
			break;
		}
	}

	gerrorn ^= gerror;
	writel_relaxed(gerrorn, base + SMMU_GERRORN);

	return IRQ_HANDLED;
}

static void arm_smmu_print_fault(struct arm_smmu_device *smmu,
				 struct arm_smmu_event *evt)
{
	dev_err_ratelimited(smmu->dev,
		"Unhandled fault: %#x, %#x, %#x, %#x, %#x, %#x, %#x, %#x\n",
		evt->data[0], evt->data[1], evt->data[2], evt->data[3],
		evt->data[4], evt->data[5], evt->data[6], evt->data[7]);
}

static irqreturn_t arm_smmu_eventq_handler(int irq, void *dev)
{
	struct iommu_domain *domain = dev;
	struct arm_smmu_domain *smmu_domain = domain->priv;
	struct arm_smmu_device *smmu = smmu_domain->smmu;
	struct arm_smmu_event event, *evt = &event;
	int ret, flags, resume;
	u32 stalled;
	unsigned long iova;

consume_event:
	ret = event_read(smmu, evt);
	if (ret) {
		dev_err_ratelimited(smmu->dev,
			"speculated event fault IRQ without event recorded\n");
		return IRQ_HANDLED;
	}

	switch (F_TYPE_GET(evt)) {
	case F_TRANSLATION:
	case F_ADDR_SIZE:
	case F_ACCESS:
	case F_PERMISSION:
		stalled = F_STALL_GET(evt);
		break;
	default:
		stalled = 0;
		break;
	}

	if (!stalled) {
		arm_smmu_print_fault(smmu, evt);
		return IRQ_HANDLED;
	}

	if (F_CLASS_GET(evt) == F_CLASS_CD)
		goto unhandled;

	iova = F_S2_GET(evt) ?  F_IPA_GET(evt) : F_IA_GET(evt);
	flags = F_RNW_GET(evt) ? IOMMU_FAULT_READ : IOMMU_FAULT_WRITE;

	if (!report_iommu_fault(domain, smmu->dev, iova, flags)) {
		ret = IRQ_HANDLED;
		resume = RESUME_RETRY;
	} else {
unhandled:
		arm_smmu_print_fault(smmu, evt);
		ret = IRQ_NONE;
		resume = RESUME_TERMINATE;
	}

	/* Retry or terminate stalled transaction */
	cmd_resume(smmu, resume, F_SID_GET(evt), F_STAG_GET(evt));

	/*
	 * Event IRQ is only triggered when event queue transitions from empty
	 * to non-empty. So we should drain event queue before leave event IRQ
	 * handler.
	 */
	if (!eventq_empty(smmu))
		goto consume_event;

	return ret;
}

static void arm_smmu_flush_pgtable(struct arm_smmu_device *smmu, void *addr,
				   size_t size)
{
	unsigned long offset = (unsigned long)addr & ~PAGE_MASK;


	/* Ensure new page tables are visible to the hardware walker */
	if (smmu->features & SMMU_FEAT_COHERENT_WALK) {
		dsb(ishst);
	} else {
		/*
		 * If the SMMU can't walk tables in the CPU caches, treat them
		 * like non-coherent DMA since we need to flush the new entries
		 * all the way out to memory. There's no possibility of
		 * recursion here as the SMMU table walker will not be wired
		 * through another SMMU.
		 */
		dma_map_page(smmu->dev, virt_to_page(addr), offset, size,
				DMA_TO_DEVICE);
	}
}

static u32 size_to_field(unsigned long size, int shift)
{
	u32 val;

	/*
	 * 0b000 32-bits (<=32)
	 * 0b001 36-bits (33,34,35,36)
	 * 0b010 40-bits (37,38,39,40)
	 * 0b011 42-bits (41,42)
	 * 0b100 44-bits (43,44)
	 * 0b101 48-bits (>=45)
	 */
	if (size <= 32)
		val = ADDR_SIZE_32;
	else if (size <= 40)
		val = ((size - 33) >> 2) + ADDR_SIZE_36;
	else if (size <= 44)
		val = ((size - 41) >> 1) + ADDR_SIZE_42;
	else
		val = ADDR_SIZE_48;

	return val << shift;
}

static void arm_smmu_init_context_bank(struct arm_smmu_domain *smmu_domain)
{
	u64 val;
	struct arm_smmu_cfg *cfg = &smmu_domain->cfg;
	struct arm_smmu_device *smmu = smmu_domain->smmu;
	struct arm_smmu_cde *cde = cfg->cde;

	if (PAGE_SIZE == SZ_4K)
		val = CDE_TG0_4K;
	else
		val = CDE_TG0_64K;

	val |= CDE_T0SZ_SET(64 - smmu->input_size);
	val |= CDE_IR0_WBRAWA | CDE_OR0_WBRAWA | CDE_SH0_IS | CDE_EPD1 | CDE_V;

#ifdef __BIG_ENDIAN
	val |= CDE_ENDI_BE;
#endif

	val |= size_to_field(smmu->s1_output_size, CDE_IPS_SHIFT);
	val |= CDE_AFFD | CDE_AA64 | CDE_ASET | CDE_A | CDE_R;

	if (smmu->features & SMMU_FEAT_STALL)
		val |= CDE_S;

	writeq_relaxed(val, &cde->data[0]);
	writeq_relaxed(virt_to_phys(cfg->pgd), &cde->data[1]);

	arm_smmu_flush_pgtable(smmu, cfg->pgd, PTRS_PER_PGD * sizeof(pgd_t));

	/* MAIR0 */
	val = (MAIR_ATTR_NC << MAIR_ATTR_SHIFT(MAIR_ATTR_IDX_NC)) |
	      (MAIR_ATTR_WBRWA << MAIR_ATTR_SHIFT(MAIR_ATTR_IDX_CACHE)) |
	      (MAIR_ATTR_DEVICE << MAIR_ATTR_SHIFT(MAIR_ATTR_IDX_DEV));

	writeq_relaxed(val, &cde->data[3]);
}

static int arm_smmu_init_domain_context(struct iommu_domain *domain,
					struct arm_smmu_device *smmu)
{
	int ret = 0;
	unsigned long flags;
	struct arm_smmu_domain *smmu_domain = domain->priv;
	struct arm_smmu_cfg *cfg = &smmu_domain->cfg;
	struct arm_smmu_cde *cde;

	cde = kzalloc(CD_ENTRY_SIZE, GFP_KERNEL);
	if (!cde)
		return -ENOMEM;

	spin_lock_irqsave(&smmu_domain->lock, flags);
	if (smmu_domain->smmu)
		goto out_unlock;

	cfg->cde = cde;

	if (smmu->features & SMMU_FEAT_TRANS_S1)
		smmu_domain->stage = ARM_SMMU_DOMAIN_S1;
	else
		smmu_domain->stage = ARM_SMMU_DOMAIN_S2;

	ACCESS_ONCE(smmu_domain->smmu) = smmu;
	arm_smmu_init_context_bank(smmu_domain);
	spin_unlock_irqrestore(&smmu_domain->lock, flags);

	ret = request_irq(smmu->irqs[SMMU_IRQ_EVENTQ], arm_smmu_eventq_handler,
			IRQF_SHARED, "arm-smmu eventq fault", domain);
	if (IS_ERR_VALUE(ret)) {
		dev_err(smmu->dev, "failed to request eventq IRQ\n");
	} else {
		u32 reg;

		reg = readl_relaxed(smmu->base + SMMU_IRQ_CTRL);
		if (!(reg & IRQ_CTRL_EVENTQ_IRQEN)) {
			reg |= IRQ_CTRL_EVENTQ_IRQEN;
			writel_and_wait(reg, smmu->base + SMMU_IRQ_CTRL);
		}
	}

	return 0;

out_unlock:
	spin_unlock_irqrestore(&smmu_domain->lock, flags);
	kfree(cde);
	return ret;
}

static void arm_smmu_destroy_domain_context(struct iommu_domain *domain)
{
	struct arm_smmu_domain *smmu_domain = domain->priv;
	struct arm_smmu_device *smmu = smmu_domain->smmu;
	struct arm_smmu_cfg *cfg = &smmu_domain->cfg;
	struct arm_smmu_ste *ste;

	if (!smmu)
		return;

	/* now StreamID is equal ASID */
	cmd_tlbi_nh_asid(smmu, cfg->sid);

	ste = get_ste_addr(smmu, cfg->sid);
	if (ste)
		writeq(0, &ste->data[0]);

	cmd_cfgi_ste(smmu, cfg->sid);

	kfree(cfg->cde);

	cfg->cde = NULL;
}

static int arm_smmu_domain_init(struct iommu_domain *domain)
{
	struct arm_smmu_domain *smmu_domain;
	pgd_t *pgd;

	/*
	 * Allocate the domain and initialise some of its data structures.
	 * We can't really do anything meaningful until we've added a
	 * master.
	 */
	smmu_domain = kzalloc(sizeof(*smmu_domain), GFP_KERNEL);
	if (!smmu_domain)
		return -ENOMEM;

	pgd = kcalloc(PTRS_PER_PGD, sizeof(pgd_t), GFP_KERNEL);
	if (!pgd)
		goto out_free_domain;
	smmu_domain->cfg.pgd = pgd;

	spin_lock_init(&smmu_domain->lock);
	domain->priv = smmu_domain;

	domain->geometry.aperture_start = 0;
	domain->geometry.aperture_end   = 0xffffffff;
	domain->geometry.force_aperture = true;

	return 0;

out_free_domain:
	kfree(smmu_domain);
	return -ENOMEM;
}

static void arm_smmu_free_ptes(pmd_t *pmd)
{
	pgtable_t table = pmd_pgtable(*pmd);

	__free_page(table);
}

static void arm_smmu_free_pmds(pud_t *pud)
{
	int i;
	pmd_t *pmd, *pmd_base = pmd_offset(pud, 0);

	pmd = pmd_base;
	for (i = 0; i < PTRS_PER_PMD; ++i) {
		if (pmd_none(*pmd))
			continue;

		arm_smmu_free_ptes(pmd);
		pmd++;
	}

	pmd_free(NULL, pmd_base);
}

static void arm_smmu_free_puds(pgd_t *pgd)
{
	int i;
	pud_t *pud, *pud_base = pud_offset(pgd, 0);

	pud = pud_base;
	for (i = 0; i < PTRS_PER_PUD; ++i) {
		if (pud_none(*pud))
			continue;

		arm_smmu_free_pmds(pud);
		pud++;
	}

	pud_free(NULL, pud_base);
}

static void arm_smmu_free_pgtables(struct arm_smmu_domain *smmu_domain)
{
	int i;
	struct arm_smmu_cfg *cfg = &smmu_domain->cfg;
	pgd_t *pgd, *pgd_base = cfg->pgd;

	/*
	 * Recursively free the page tables for this domain. We don't
	 * care about speculative TLB filling because the tables should
	 * not be active in any context bank at this point (SCTLR.M is 0).
	 */
	pgd = pgd_base;
	for (i = 0; i < PTRS_PER_PGD; ++i) {
		if (pgd_none(*pgd))
			continue;
		arm_smmu_free_puds(pgd);
		pgd++;
	}

	kfree(pgd_base);
}

static void arm_smmu_domain_destroy(struct iommu_domain *domain)
{
	struct arm_smmu_domain *smmu_domain = domain->priv;

	/*
	 * Free the domain resources. We assume that all devices have
	 * already been detached.
	 */
	arm_smmu_destroy_domain_context(domain);
	arm_smmu_free_pgtables(smmu_domain);
	kfree(smmu_domain);
}

static int arm_smmu_domain_add_master(struct arm_smmu_domain *smmu_domain,
				      struct arm_smmu_master_cfg *cfg)
{
	u64 val;
	struct arm_smmu_device *smmu = smmu_domain->smmu;
	struct arm_smmu_cde *cde = smmu_domain->cfg.cde;
	struct arm_smmu_ste *ste;
	unsigned long flags;

	ste = get_ste_addr(smmu, cfg->sid);
	if (!ste)
		return -ENOMEM;

	spin_lock_irqsave(&smmu_ste_lock, flags);

	val = readq_relaxed(&ste->data[0]);
	if (unlikely(val)) {
		spin_unlock_irqrestore(&smmu_ste_lock, flags);
		return 0;
	}

	val = readq_relaxed(&cde->data[0]);
	val |= CDE_ASID_SET(cfg->sid);
	writeq_relaxed(val, &cde->data[0]);

	val = virt_to_phys(cde) | STE_CONFIG_S1 | STE_V;
	writeq_relaxed(val, &ste->data[0]);
	val = STE_S1CIR_WBRA | STE_S1COR_WBRA | STE_S1CSH_IS;
	writeq_relaxed(val, &ste->data[1]);
	writeq_relaxed(0, &ste->data[2]);
	writeq_relaxed(0, &ste->data[3]);

	spin_unlock_irqrestore(&smmu_ste_lock, flags);

	dsb(sy);
	cmd_cfgi_ste(smmu, cfg->sid);
	cmd_sync(smmu);

	return 0;
}

static void arm_smmu_domain_remove_master(struct arm_smmu_domain *smmu_domain,
					  struct arm_smmu_master_cfg *cfg)
{
}

static int arm_smmu_attach_dev(struct iommu_domain *domain, struct device *dev)
{
	int ret;
	struct arm_smmu_domain *smmu_domain = domain->priv;
	struct arm_smmu_device *smmu, *dom_smmu;
	struct arm_smmu_master_cfg *cfg;

	smmu = dev_get_master_dev(dev)->archdata.iommu;
	if (!smmu) {
		dev_err(dev, "cannot attach to SMMU, is it on the same bus?\n");
		return -ENXIO;
	}

	/*
	 * Sanity check the domain. We don't support domains across
	 * different SMMUs.
	 */
	dom_smmu = ACCESS_ONCE(smmu_domain->smmu);
	if (!dom_smmu) {
		/* Now that we have a master, we can finalise the domain */
		ret = arm_smmu_init_domain_context(domain, smmu);
		if (IS_ERR_VALUE(ret))
			return ret;

		dom_smmu = smmu_domain->smmu;
	}

	if (dom_smmu != smmu) {
		dev_err(dev,
			"cannot attach to SMMU %s whilst already attached to domain on SMMU %s\n",
			dev_name(smmu_domain->smmu->dev), dev_name(smmu->dev));
		return -EINVAL;
	}

	/* Looks ok, so add the device to the domain */
	cfg = find_smmu_master_cfg(smmu_domain->smmu, dev);
	if (!cfg)
		return -ENODEV;

	return arm_smmu_domain_add_master(smmu_domain, cfg);
}

static void arm_smmu_detach_dev(struct iommu_domain *domain, struct device *dev)
{
	struct arm_smmu_domain *smmu_domain = domain->priv;
	struct arm_smmu_master_cfg *cfg;

	cfg = find_smmu_master_cfg(smmu_domain->smmu, dev);
	if (cfg)
		arm_smmu_domain_remove_master(smmu_domain, cfg);
}

static bool arm_smmu_pte_is_contiguous_range(unsigned long addr,
					     unsigned long end)
{
	return !(addr & ~ARM_SMMU_PTE_CONT_MASK) &&
		(addr + ARM_SMMU_PTE_CONT_SIZE <= end);
}

static int arm_smmu_alloc_init_pte(struct arm_smmu_device *smmu, pmd_t *pmd,
				   unsigned long addr, unsigned long end,
				   unsigned long pfn, int prot, int stage)
{
	pte_t *pte, *start;
	pteval_t pteval = ARM_SMMU_PTE_PAGE | ARM_SMMU_PTE_AF | ARM_SMMU_PTE_XN;

	if (pmd_none(*pmd)) {
		/* Allocate a new set of tables */
		pgtable_t table = alloc_page(GFP_ATOMIC|__GFP_ZERO);

		if (!table)
			return -ENOMEM;

		arm_smmu_flush_pgtable(smmu, page_address(table), PAGE_SIZE);
		pmd_populate(NULL, pmd, table);
		arm_smmu_flush_pgtable(smmu, pmd, sizeof(*pmd));
	}

	if (stage == 1) {
		pteval |= ARM_SMMU_PTE_AP_UNPRIV | ARM_SMMU_PTE_nG;
		if (!(prot & IOMMU_WRITE) && (prot & IOMMU_READ))
			pteval |= ARM_SMMU_PTE_AP_RDONLY;

		if (prot & IOMMU_CACHE)
			pteval |= (MAIR_ATTR_IDX_CACHE <<
				   ARM_SMMU_PTE_ATTRINDX_SHIFT);
	} else {
		pteval |= ARM_SMMU_PTE_HAP_FAULT;
		if (prot & IOMMU_READ)
			pteval |= ARM_SMMU_PTE_HAP_READ;
		if (prot & IOMMU_WRITE)
			pteval |= ARM_SMMU_PTE_HAP_WRITE;
		if (prot & IOMMU_CACHE)
			pteval |= ARM_SMMU_PTE_MEMATTR_OIWB;
		else
			pteval |= ARM_SMMU_PTE_MEMATTR_NC;
	}

	/* If no access, create a faulting entry to avoid TLB fills */
	if (prot & IOMMU_NOEXEC)
		pteval |= ARM_SMMU_PTE_XN;

	if (!(prot & (IOMMU_READ | IOMMU_WRITE)))
		pteval &= ~ARM_SMMU_PTE_PAGE;

	pteval |= ARM_SMMU_PTE_SH_IS;
	start = pmd_page_vaddr(*pmd) + pte_index(addr);
	pte = start;

	/*
	 * Install the page table entries. This is fairly complicated
	 * since we attempt to make use of the contiguous hint in the
	 * ptes where possible. The contiguous hint indicates a series
	 * of ARM_SMMU_PTE_CONT_ENTRIES ptes mapping a physically
	 * contiguous region with the following constraints:
	 *
	 *   - The region start is aligned to ARM_SMMU_PTE_CONT_SIZE
	 *   - Each pte in the region has the contiguous hint bit set
	 *
	 * This complicates unmapping (also handled by this code, when
	 * neither IOMMU_READ or IOMMU_WRITE are set) because it is
	 * possible, yet highly unlikely, that a client may unmap only
	 * part of a contiguous range. This requires clearing of the
	 * contiguous hint bits in the range before installing the new
	 * faulting entries.
	 *
	 * Note that re-mapping an address range without first unmapping
	 * it is not supported, so TLB invalidation is not required here
	 * and is instead performed at unmap and domain-init time.
	 */
	do {
		int i = 1;

		pteval &= ~ARM_SMMU_PTE_CONT;

		if (arm_smmu_pte_is_contiguous_range(addr, end)) {
			i = ARM_SMMU_PTE_CONT_ENTRIES;
			pteval |= ARM_SMMU_PTE_CONT;
		} else if (pte_val(*pte) &
			   (ARM_SMMU_PTE_CONT | ARM_SMMU_PTE_PAGE)) {
			int j;
			pte_t *cont_start;
			unsigned long idx = pte_index(addr);

			idx &= ~(ARM_SMMU_PTE_CONT_ENTRIES - 1);
			cont_start = pmd_page_vaddr(*pmd) + idx;
			for (j = 0; j < ARM_SMMU_PTE_CONT_ENTRIES; ++j)
				pte_val(*(cont_start + j)) &=
					~ARM_SMMU_PTE_CONT;

			arm_smmu_flush_pgtable(smmu, cont_start,
					       sizeof(*pte) *
					       ARM_SMMU_PTE_CONT_ENTRIES);
		}

		do {
			*pte = pfn_pte(pfn, __pgprot(pteval));
		} while (pte++, pfn++, addr += PAGE_SIZE, --i);
	} while (addr != end);

	arm_smmu_flush_pgtable(smmu, start, sizeof(*pte) * (pte - start));
	return 0;
}

static int arm_smmu_alloc_init_pmd(struct arm_smmu_device *smmu, pud_t *pud,
				   unsigned long addr, unsigned long end,
				   phys_addr_t phys, int prot, int stage)
{
	int ret;
	pmd_t *pmd;
	unsigned long next, pfn = __phys_to_pfn(phys);

#ifndef __PAGETABLE_PMD_FOLDED
	if (pud_none(*pud)) {
		pmd = (pmd_t *)get_zeroed_page(GFP_ATOMIC);
		if (!pmd)
			return -ENOMEM;

		arm_smmu_flush_pgtable(smmu, pmd, PAGE_SIZE);
		pud_populate(NULL, pud, pmd);
		arm_smmu_flush_pgtable(smmu, pud, sizeof(*pud));

		pmd += pmd_index(addr);
	} else
#endif
		pmd = pmd_offset(pud, addr);

	do {
		next = pmd_addr_end(addr, end);
		ret = arm_smmu_alloc_init_pte(smmu, pmd, addr, next, pfn,
					      prot, stage);
		phys += next - addr;
	} while (pmd++, addr = next, addr < end);

	return ret;
}

static int arm_smmu_alloc_init_pud(struct arm_smmu_device *smmu, pgd_t *pgd,
				   unsigned long addr, unsigned long end,
				   phys_addr_t phys, int prot, int stage)
{
	int ret = 0;
	pud_t *pud;
	unsigned long next;

#ifndef __PAGETABLE_PUD_FOLDED
	if (pgd_none(*pgd)) {
		pud = (pud_t *)get_zeroed_page(GFP_ATOMIC);
		if (!pud)
			return -ENOMEM;

		arm_smmu_flush_pgtable(smmu, pud, PAGE_SIZE);
		pgd_populate(NULL, pgd, pud);
		arm_smmu_flush_pgtable(smmu, pgd, sizeof(*pgd));

		pud += pud_index(addr);
	} else
#endif
		pud = pud_offset(pgd, addr);

	do {
		next = pud_addr_end(addr, end);
		ret = arm_smmu_alloc_init_pmd(smmu, pud, addr, next, phys,
					      prot, stage);
		phys += next - addr;
	} while (pud++, addr = next, addr < end);

	return ret;
}

static int arm_smmu_handle_mapping(struct arm_smmu_domain *smmu_domain,
				   unsigned long iova, phys_addr_t paddr,
				   size_t size, int prot)
{
	int ret, stage;
	unsigned long end;
	phys_addr_t input_mask, output_mask;
	struct arm_smmu_device *smmu = smmu_domain->smmu;
	struct arm_smmu_cfg *cfg = &smmu_domain->cfg;
	pgd_t *pgd = cfg->pgd;
	unsigned long flags;

	if (!(smmu->features & SMMU_FEAT_TRANS_S1)) {
		stage = 2;
		output_mask = (1ULL << smmu->s2_output_size) - 1;
	} else {
		stage = 1;
		output_mask = (1ULL << smmu->s1_output_size) - 1;
	}

	if (!pgd)
		return -EINVAL;

	if (size & ~PAGE_MASK)
		return -EINVAL;

	input_mask = (1ULL << smmu->input_size) - 1;
	if ((phys_addr_t)iova & ~input_mask)
		return -ERANGE;

	if (paddr & ~output_mask)
		return -ERANGE;

	spin_lock_irqsave(&smmu_domain->lock, flags);
	pgd += pgd_index(iova);
	end = iova + size;
	do {
		unsigned long next = pgd_addr_end(iova, end);

		ret = arm_smmu_alloc_init_pud(smmu, pgd, iova, next, paddr,
					      prot, stage);
		if (ret)
			goto out_unlock;

		paddr += next - iova;
		iova = next;
	} while (pgd++, iova != end);

out_unlock:
	spin_unlock_irqrestore(&smmu_domain->lock, flags);

	return ret;
}

static int arm_smmu_map(struct iommu_domain *domain, unsigned long iova,
			phys_addr_t paddr, size_t size, int prot)
{
	struct arm_smmu_domain *smmu_domain = domain->priv;

	if (!smmu_domain)
		return -ENODEV;

	return arm_smmu_handle_mapping(smmu_domain, iova, paddr, size, prot);
}

static size_t arm_smmu_unmap(struct iommu_domain *domain, unsigned long iova,
			     size_t size)
{
	int ret;
	struct arm_smmu_domain *smmu_domain = domain->priv;

	ret = arm_smmu_handle_mapping(smmu_domain, iova, 0, size, 0);
	cmd_tlbi_nh_asid(smmu_domain->smmu, smmu_domain->cfg.sid);
	return ret ? 0 : size;
}

static phys_addr_t arm_smmu_iova_to_phys(struct iommu_domain *domain,
					 dma_addr_t iova)
{
	pgd_t *pgdp, pgd;
	pud_t pud;
	pmd_t pmd;
	pte_t pte;
	struct arm_smmu_domain *smmu_domain = domain->priv;
	struct arm_smmu_cfg *cfg = &smmu_domain->cfg;

	pgdp = cfg->pgd;
	if (!pgdp)
		return 0;

	pgd = *(pgdp + pgd_index(iova));
	if (pgd_none(pgd))
		return 0;

	pud = *pud_offset(&pgd, iova);
	if (pud_none(pud))
		return 0;

	pmd = *pmd_offset(&pud, iova);
	if (pmd_none(pmd))
		return 0;

	pte = *(pmd_page_vaddr(pmd) + pte_index(iova));
	if (pte_none(pte))
		return 0;

	return __pfn_to_phys(pte_pfn(pte)) | (iova & ~PAGE_MASK);
}

static bool arm_smmu_capable(enum iommu_cap cap)
{
	switch (cap) {
	case IOMMU_CAP_CACHE_COHERENCY:
		/*
		 * Return true here as the SMMU can always send out coherent
		 * requests.
		 */
		return true;
	case IOMMU_CAP_INTR_REMAP:
		return true; /* MSIs are just memory writes */
	case IOMMU_CAP_NOEXEC:
		return true;
	default:
		return false;
	}
}

static int __arm_smmu_get_pci_sid(struct pci_dev *pdev, u16 alias, void *data)
{
	*((u16 *)data) = alias;
	return 0; /* Continue walking */
}

static int arm_smmu_add_device(struct device *dev)
{
	struct arm_smmu_device *smmu;
	struct iommu_group *group;
	int ret;

	if (dev->archdata.iommu) {
		dev_warn(dev, "IOMMU driver already assigned to device\n");
		return -EINVAL;
	}

	smmu = find_smmu_for_device(dev);
	if (!smmu)
		return -ENODEV;

	group = iommu_group_alloc();
	if (IS_ERR(group)) {
		dev_err(dev, "Failed to allocate IOMMU group\n");
		return PTR_ERR(group);
	}

	if (dev_is_pci(dev)) {
		struct arm_smmu_master_cfg *cfg;
		struct pci_dev *pdev = to_pci_dev(dev);

		cfg = kzalloc(sizeof(*cfg), GFP_KERNEL);
		if (!cfg) {
			ret = -ENOMEM;
			goto out_put_group;
		}

		/*
		 * Assume Stream ID == Requester ID for now.
		 * We need a way to describe the ID mappings in FDT.
		 */
		pci_for_each_dma_alias(pdev, __arm_smmu_get_pci_sid,
				       &cfg->sid);
		dev->archdata.iommu = cfg;
	} else {
		dev->archdata.iommu = smmu;
	}

	ret = iommu_group_add_device(group, dev);

//	arch_setup_dma_ops(dev, 0,
//			dev->coherent_dma_mask,
//			(struct iommu_ops *)dev->bus->iommu_ops,
//			dev->archdata.dma_coherent);

out_put_group:
	iommu_group_put(group);
	return ret;
}

static void arm_smmu_remove_device(struct device *dev)
{
	if (dev_is_pci(dev))
		kfree(dev->archdata.iommu);

	dev->archdata.iommu = NULL;
	iommu_group_remove_device(dev);
}

static int arm_smmu_domain_get_attr(struct iommu_domain *domain,
				    enum iommu_attr attr, void *data)
{
	struct arm_smmu_domain *smmu_domain = domain->priv;

	switch (attr) {
	case DOMAIN_ATTR_NESTING:
		*(int *)data = (smmu_domain->stage == ARM_SMMU_DOMAIN_NESTED);
		return 0;
	default:
		return -ENODEV;
	}
}

static int arm_smmu_domain_set_attr(struct iommu_domain *domain,
				    enum iommu_attr attr, void *data)
{
	struct arm_smmu_domain *smmu_domain = domain->priv;

	switch (attr) {
	case DOMAIN_ATTR_NESTING:
		if (smmu_domain->smmu)
			return -EPERM;
		if (*(int *)data)
			smmu_domain->stage = ARM_SMMU_DOMAIN_NESTED;
		else
			smmu_domain->stage = ARM_SMMU_DOMAIN_S1;

		return 0;
	default:
		return -ENODEV;
	}
}

static struct iommu_ops arm_smmu_ops = {
	.capable		= arm_smmu_capable,
	.domain_init		= arm_smmu_domain_init,
	.domain_destroy		= arm_smmu_domain_destroy,
	.attach_dev		= arm_smmu_attach_dev,
	.detach_dev		= arm_smmu_detach_dev,
	.map			= arm_smmu_map,
	.unmap			= arm_smmu_unmap,
	.map_sg			= default_iommu_map_sg,
	.iova_to_phys		= arm_smmu_iova_to_phys,
	.add_device		= arm_smmu_add_device,
	.remove_device		= arm_smmu_remove_device,
	.domain_get_attr	= arm_smmu_domain_get_attr,
	.domain_set_attr	= arm_smmu_domain_set_attr,
	.pgsize_bitmap		= (SECTION_SIZE |
			   	   ARM_SMMU_PTE_CONT_SIZE |
				   PAGE_SIZE),
};

static void arm_smmu_device_reset(struct arm_smmu_device *smmu)
{
	void __iomem *base = smmu->base;
	u32 reg;

	reg = CR2_PTM | CR2_RECINVSID;
	writel_relaxed(reg, base + SMMU_CR2);

	writel_relaxed(0, base + SMMU_CMDQ_PROD);
	writel_relaxed(0, base + SMMU_CMDQ_CONS);
	writel_relaxed(0, base + SMMU_EVENTQ_PROD);
	writel_relaxed(0, base + SMMU_EVENTQ_CONS);

	reg = IRQ_CTRL_GERROR_IRQEN;
	writel_and_wait(reg, base + SMMU_IRQ_CTRL);

	reg = CR0_CMDQEN;
	writel_and_wait(reg, base + SMMU_CR0);

	/* Invalidate data structures and  TLBs */
	cmd_cfgi_all(smmu);
	cmd_sync(smmu);
	cmd_tlbi_nsnh_all(smmu);
	cmd_sync(smmu);

	reg |= CR0_EVENTQEN;
	writel_and_wait(reg, base + SMMU_CR0);

	reg |= CR0_SMMUEN;
	writel_and_wait(reg, base + SMMU_CR0);
}

static int arm_smmu_id_size_to_bits(int oas)
{
	switch (oas) {
	case 0:
		return 32;
	case 1:
		return 36;
	case 2:
		return 40;
	case 3:
		return 42;
	case 4:
		return 44;
	case 5:
	default:
		return 48;
	}
}

static int arm_smmu_device_cfg_probe(struct arm_smmu_device *smmu)
{
	unsigned long size;
	void __iomem *base = smmu->base;
	u32 id, id1, cr1, bits;
	u64 val;
	struct page *tbl_page;

	dev_notice(smmu->dev, "probing hardware configuration...\n");

	BUG_ON((SMMU_IRQ_CTRLACK - SMMU_IRQ_CTRL) != ACK_REG_OFFSET);

	/* AIDR */
	id = readl_relaxed(base + SMMU_AIDR);
	smmu->version = AIDR_ARCH_REVISION_GET(id);
	dev_notice(smmu->dev, "SMMUv%d with:\n", smmu->version);

	/* ID0 */
	id = readl_relaxed(base + SMMU_IDR0);
	if (id & IDR0_S1P) {
		smmu->features |= SMMU_FEAT_TRANS_S1;
		dev_notice(smmu->dev, "\tstage 1 translation\n");
	}

	if (id & IDR0_S2P) {
		smmu->features |= SMMU_FEAT_TRANS_S2;
		dev_notice(smmu->dev, "\tstage 2 translation\n");
	}

	if (!(smmu->features & (SMMU_FEAT_TRANS_S1 | SMMU_FEAT_TRANS_S2))) {
		dev_err(smmu->dev, "\tno translation support!\n");
		return -ENODEV;
	}

	if (id & IDR0_COHACC) {
		smmu->features |= SMMU_FEAT_COHERENT_WALK;
		dev_notice(smmu->dev, "\tcoherent table walk\n");
	}

	if ((id & IDR0_ST_LEVEL_MASK) == IDR0_ST_LEVEL_2) {
		smmu->features |= SMMU_FEAT_ST_2LEVEL;
		dev_notice(smmu->dev, "\t2-level stream table\n");
	}

	if (id & IDR0_CD2L) {
		smmu->features |= SMMU_FEAT_CD_2LEVEL;
		dev_notice(smmu->dev, "\t2-level CD table\n");
	}

	if (id & IDR0_PRI) {
		smmu->features |= SMMU_FEAT_PRI;
		dev_notice(smmu->dev, "\tpage request interface\n");
	}

	if (id & IDR0_SEV) {
		smmu->features |= SMMU_FEAT_SEV;
		dev_notice(smmu->dev, "\tcmd_sync sev\n");
	}

	if ((id & IDR0_STALL_MODEL_STALL_MASK) == IDR0_STALL_MODEL_STALL) {
		smmu->features |= SMMU_FEAT_STALL;
		dev_notice(smmu->dev, "\tstall supported\n");
	}

	/* ID1 */
	id1 = readl_relaxed(base + SMMU_IDR1);
	smmu->sidsize = IDR1_SIDSIZE_GET(id1);

	/* CR1 */
	cr1 = readl_relaxed(base + SMMU_CR1);

	if (id1 & IDR1_TABLES_PRESET) {
		val = readq_relaxed(base + SMMU_STRTAB_BASE);
		val &= STRTAB_BASE_ADDR_MASK;

		if (id1 & IDR1_REL)
			val += virt_to_phys(base);

		id = readl_relaxed(base + SMMU_STRTAB_BASE_CFG);
		bits = min_t(u32,
			STRTAB_BASE_CFG_LOG2SIZE_GET(id), smmu->sidsize);

		if (smmu->features & SMMU_FEAT_ST_2LEVEL) {
			bits -= STRTAB_BASE_CFG_SPLIT_GET(id);
			size = STD_TBLSIZE_GET(bits);
			smmu->std_span = STRTAB_BASE_CFG_SPLIT_GET(id);
		} else {
			size = STE_TBLSIZE_GET(bits);
			smmu->std_span = 0;
		}

		smmu->strtab = ioremap_nocache(val, size);
		if (!smmu->strtab) {
			pr_err("failed to ioremap table memory\n");
			return -ENOMEM;
		}
	} else {
		cr1 &= ~CR1_TABLE_MASK;
		cr1 |= CR1_TABLE_SH_ISH | CR1_TABLE_OC_WB | CR1_TABLE_IC_WB;
		writel_relaxed(cr1, base + SMMU_CR1);

		id = STRTAB_BASE_CFG_SPLIT_SET(PAGE_SHIFT) | smmu->sidsize;
		bits = smmu->sidsize;

		if (smmu->features & SMMU_FEAT_ST_2LEVEL) {
			id |= STRTAB_BASE_CFG_FMT_2LEVEL;
			bits -= STRTAB_BASE_CFG_SPLIT_GET(id);
			size = STD_TBLSIZE_GET(bits);
			smmu->std_span = STRTAB_BASE_CFG_SPLIT_GET(id);
		} else {
			size = STE_TBLSIZE_GET(bits);
			smmu->std_span = 0;
		}

		writel_relaxed(id, base + SMMU_STRTAB_BASE_CFG);
		dsb(sy);

		tbl_page = alloc_pages(GFP_KERNEL | __GFP_ZERO, get_order(size));
		if (!tbl_page) {
			pr_err("failed to allocate table memory\n");
			return -ENOMEM;
		}

		smmu->strtab = page_address(tbl_page);

		val = STRTAB_BASE_RA | page_to_phys(tbl_page);
		BUG_ON(val & (size - 1));
		writeq_relaxed(val, base + SMMU_STRTAB_BASE);
	}

	if (id1 & IDR1_QUEUES_PRESET) {
		val = readq_relaxed(base + SMMU_CMDQ_BASE);
		bits = min(CMDQ_BASE_LOG2SIZE_GET(val), IDR1_CMDQS_GET(id1));
		val &= CMDQ_BASE_ADDR_MASK;

		if (id1 & IDR1_REL)
			val += virt_to_phys(base);

		smmu->cmdq = ioremap_nocache(val, CMDQ_TBLSIZE_GET(bits));
		if (!smmu->cmdq) {
			pr_err("failed to ioremap cmdq memory\n");
			return -ENOMEM;
		}

		val = readq_relaxed(base + SMMU_EVENTQ_BASE);
		bits = min(EVENTQ_BASE_LOG2SIZE_GET(val), IDR1_EVENTQS_GET(id1));
		val &= EVENTQ_BASE_ADDR_MASK;

		smmu->eventq = ioremap_nocache(val, EVENTQ_TBLSIZE_GET(bits));
		if (!smmu->eventq) {
			pr_err("failed to ioremap eventq memory\n");
			return -ENOMEM;
		}
	} else {
		cr1 &= ~CR1_QUEUE_MASK;
		cr1 |= CR1_QUEUE_SH_ISH | CR1_QUEUE_OC_WB | CR1_QUEUE_IC_WB;
		writel_relaxed(cr1, base + SMMU_CR1);

		tbl_page = alloc_page(GFP_KERNEL | __GFP_ZERO);
		if (!tbl_page) {
			pr_err("failed to allocate cmdq memory\n");
			return -ENOMEM;
		}
		smmu->cmdq = page_address(tbl_page);
		dsb(sy);

		bits = __fls(PAGE_SIZE / CMDQ_ENTRY_SIZE);
		bits = min_t(u32, bits, IDR1_CMDQS_GET(id1));
		smmu->cmdq_mask = NR_MASK(bits + 1);
		val = CMDQ_BASE_RA | page_to_phys(tbl_page) | bits;
		writeq_relaxed(val, base + SMMU_CMDQ_BASE);

		tbl_page = alloc_page(GFP_KERNEL | __GFP_ZERO);
		if (!tbl_page) {
			pr_err("failed to allocate eventq memory\n");
			return -ENOMEM;
		}
		smmu->eventq = page_address(tbl_page);
		dsb(sy);

		bits = __fls(PAGE_SIZE / EVENTQ_ENTRY_SIZE);
		bits = min_t(u32, bits, IDR1_EVENTQS_GET(id1));
		smmu->eventq_mask = NR_MASK(bits + 1);
		val = EVENTQ_BASE_WA | page_to_phys(tbl_page) | bits;
		writeq_relaxed(val, base + SMMU_EVENTQ_BASE);
	}

	cr1 = readl_relaxed(base + SMMU_CR1);

	smmu->input_size = VA_BITS;
	smmu->s1_output_size = min_t(unsigned long, PHYS_MASK_SHIFT, VA_BITS);

	/* The stage-2 output mask is also applied for bypass */
	id = readl_relaxed(base + SMMU_IDR5);
	size = arm_smmu_id_size_to_bits(IDR5_OAS_GET(id));
	smmu->s2_output_size = min_t(unsigned long, PHYS_MASK_SHIFT, size);

	if ((PAGE_SIZE == SZ_4K && !(id & IDR5_GRAN4K)) ||
	    (PAGE_SIZE == SZ_64K && !(id & IDR5_GRAN64K)) ||
	    (PAGE_SIZE != SZ_4K && PAGE_SIZE != SZ_64K)) {
		dev_err(smmu->dev, "CPU page size 0x%lx unsupported\n",
			PAGE_SIZE);
		return -ENODEV;
	}

	dev_notice(smmu->dev,
		   "\t%lu-bit VA, %lu-bit IPA, %lu-bit PA\n",
		   smmu->input_size, smmu->s1_output_size,
		   smmu->s2_output_size);

	return 0;
}

static int arm_smmu_device_dt_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct arm_smmu_device *smmu;
	struct device *dev = &pdev->dev;
	struct rb_node *node;
	struct of_phandle_args masterspec;
	int i, err;
	u32 reg;
	const __be32 *prop;
	int len;

	smmu = devm_kzalloc(dev, sizeof(*smmu), GFP_KERNEL);
	if (!smmu) {
		dev_err(dev, "failed to allocate arm_smmu_device\n");
		return -ENOMEM;
	}
	smmu->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	smmu->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(smmu->base))
		return PTR_ERR(smmu->base);
	smmu->size = resource_size(res);

	reg = readl_relaxed(smmu->base + SMMU_IDR0);
	if (reg & IDR0_MSI) {
		/*
		 * alloc a group of LPIs
		 * smmu->irqs[SMMU_IRQ_GERROR] = irq0;
		 * smmu->irqs[SMMU_IRQ_EVENTQ] = irq1;
		 * smmu->irqs[SMMU_IRQ_PRIQ] = irq2;

		 * writel SMMU_GERROR_IRQ_CFG0, SMMU_GERROR_IRQ_CFG1
		 * writel SMMU_EVENTQ_IRQ_CFG0, SMMU_EVENTQ_IRQ_CFG1
		 * write SMMU_PRIQ_IRQ_CFG0, SMMU_PRIQ_IRQ_CFG1
		 * how to get SH and MemAttr from GIC module?
		 * how to get MSI addr and data from GIC module?
		 */
	} else {
		int irq;

		irq = mbi_parse_irqs(dev, NULL);
		if (irq > 0) {
			smmu->irqs[SMMU_IRQ_EVENTQ] = irq + 0;
			smmu->irqs[SMMU_IRQ_GERROR] = irq + 1;
			smmu->irqs[SMMU_IRQ_SYNC]   = irq + 2;
			smmu->num_irqs = 3;
		} else {
			static const char *name[] = {"GERROR", "EVENTQ", "PRIQ"};

			smmu->num_irqs = SMMU_IRQ_EVENTQ + 1;
			if (smmu->features & SMMU_FEAT_PRI)
				smmu->num_irqs++;

			for (i = 0; i < smmu->num_irqs; i++) {
				irq =  platform_get_irq_byname(pdev, name[i]);
				if (irq < 0) {
					dev_err(dev, "failed to get %s IRQ\n", name[i]);
					return -ENODEV;
				}
				smmu->irqs[i] = irq;
			}
		}
	}

	err = arm_smmu_device_cfg_probe(smmu);
	if (err)
		return err;

	i = 0;
	smmu->masters = RB_ROOT;
	while (!of_parse_phandle_with_args(dev->of_node, "mmu-masters",
					   "#stream-id-cells", i,
					   &masterspec)) {
		err = register_smmu_master(smmu, dev, &masterspec);
		if (err) {
			dev_err(dev, "failed to add master %s\n",
				masterspec.np->name);
			goto out_put_masters;
		}

		i++;
	}
	dev_notice(dev, "registered %d master devices\n", i);

	/*
	 * 0, pure bypass
	 * 1, cahceable, WBRAWA
	 * 2, non-cacheable
	 * 3, device, nGnRE
	 */
	prop = (__be32 *)of_get_property(dev->of_node, "smmu-cb-memtype", &len);
	for (i = 0; prop && (i < (len / 4) - 1); i += 2) {
		u64 sid;
		struct arm_smmu_ste *ste;

		sid = of_read_number(&prop[i], 1);

		if (sid >= ((u64)1 << smmu->sidsize)) {
			dev_err(dev, "invalid StreamID 0x%llx\n", sid);
			goto out_put_masters;
		}

		ste = get_ste_addr(smmu, sid);
		if (!ste) {
			dev_err(dev, "Allocate STE failed 0x%llx\n", sid);
			goto out_put_masters;
		}

		ste->data[0] = STE_CONFIG_BYPASS | STE_V;
		ste->data[1] = ((u64)0xbf000 << 32); /* Data, Priv, NS, ISH */
		ste->data[2] = 0;
		ste->data[3] = 0;

		switch (of_read_number(&prop[i + 1], 1)) {
		case 0:
			ste->data[1] = 0;
			break;

		case 1:
			ste->data[1] |= ((u64)0x1df << 32);
			break;

		case 2:
			ste->data[1] |= ((u64)0x15 << 32);
			break;

		case 3:
			ste->data[1] |= ((u64)0x11 << 32);
			break;
		}
	}

	err = request_irq(smmu->irqs[SMMU_IRQ_GERROR],
			  arm_smmu_gerror_handler,
			  IRQF_SHARED,
			  "arm-smmu global fault",
			  smmu);
	if (err) {
		dev_err(dev, "failed to request gerror IRQ\n");
		goto out_free_irqs;
	}

	INIT_LIST_HEAD(&smmu->list);
	spin_lock(&arm_smmu_devices_lock);
	list_add(&smmu->list, &arm_smmu_devices);
	spin_unlock(&arm_smmu_devices_lock);

	arm_smmu_device_reset(smmu);
	return 0;

out_free_irqs:
	while (i--)
		free_irq(smmu->irqs[i], smmu);

out_put_masters:
	for (node = rb_first(&smmu->masters); node; node = rb_next(node)) {
		struct arm_smmu_master *master
			= container_of(node, struct arm_smmu_master, node);
		of_node_put(master->of_node);
	}

	return err;
}

static int arm_smmu_device_remove(struct platform_device *pdev)
{
	int i;
	struct device *dev = &pdev->dev;
	struct arm_smmu_device *curr, *smmu = NULL;
	struct rb_node *node;
	u32 reg;

	spin_lock(&arm_smmu_devices_lock);
	list_for_each_entry(curr, &arm_smmu_devices, list) {
		if (curr->dev == dev) {
			smmu = curr;
			list_del(&smmu->list);
			break;
		}
	}
	spin_unlock(&arm_smmu_devices_lock);

	if (!smmu)
		return -ENODEV;

	for (node = rb_first(&smmu->masters); node; node = rb_next(node)) {
		struct arm_smmu_master *master
			= container_of(node, struct arm_smmu_master, node);
		of_node_put(master->of_node);
	}

	for (i = 0; i < smmu->num_irqs; ++i)
		free_irq(smmu->irqs[i], smmu);

	reg = readl_relaxed(smmu->base + SMMU_CR0);
	reg &= ~(CR0_CMDQEN | CR0_EVENTQEN);
	writel_and_wait(reg, smmu->base + SMMU_CR0);
	reg &= ~CR0_SMMUEN;
	writel_and_wait(reg, smmu->base + SMMU_CR0);

	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id arm_smmu_of_match[] = {
	{ .compatible = "arm,smmu-v3", },
	{ },
};
MODULE_DEVICE_TABLE(of, arm_smmu_of_match);
#endif

static struct platform_driver arm_smmu_driver = {
	.driver	= {
		.owner		= THIS_MODULE,
		.name		= "arm-smmu",
		.of_match_table	= of_match_ptr(arm_smmu_of_match),
	},
	.probe	= arm_smmu_device_dt_probe,
	.remove	= arm_smmu_device_remove,
};

static int __init arm_smmu_init(void)
{
	int ret;

	ret = platform_driver_register(&arm_smmu_driver);
	if (ret)
		return ret;

	/* Oh, for a proper bus abstraction */
	if (!iommu_present(&platform_bus_type))
		bus_set_iommu(&platform_bus_type, &arm_smmu_ops);

#ifdef CONFIG_ARM_AMBA
	if (!iommu_present(&amba_bustype))
		bus_set_iommu(&amba_bustype, &arm_smmu_ops);
#endif

#ifdef CONFIG_PCI
	if (!iommu_present(&pci_bus_type))
		bus_set_iommu(&pci_bus_type, &arm_smmu_ops);
#endif

	return 0;
}

static void __exit arm_smmu_exit(void)
{
	return platform_driver_unregister(&arm_smmu_driver);
}

subsys_initcall(arm_smmu_init);
module_exit(arm_smmu_exit);

MODULE_DESCRIPTION("IOMMU API for ARM architected SMMUv3 implementations");
MODULE_AUTHOR("Lei Zhen <thunder.leizhen@huawei.com>");
MODULE_LICENSE("GPL v2");
