/*
 * PCIe host controller driver for HiSilicon Hip05 SoC
 *
 * Copyright (C) 2015 HiSilicon Co., Ltd. http://www.hisilicon.com
 *
 * Author: Zhou Wang <wangzhou1@hisilicon.com>
 *         Dacai Zhu <zhudacai@hisilicon.com>
 *         Gabriele Paoloni <gabriele.paoloni@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef PCIE_HISI_H_
#define PCIE_HISI_H_

struct hisi_pcie {
#ifdef CONFIG_PCI_HISI_ACPI
	struct acpi_pci_root *root;
#endif /* CONFIG_ACPI_HOST_GENERIC */
	struct regmap *subctrl;
	void __iomem *reg_base;
	u32 port_id;
	struct pcie_port pp;
	struct pcie_soc_ops *soc_ops;
};

struct pcie_soc_ops {
	int (*hisi_pcie_link_up)(struct hisi_pcie *pcie);
};

static inline void hisi_pcie_apb_writel(struct hisi_pcie *pcie,
					u32 val, u32 reg)
{
	writel(val, pcie->reg_base + reg);
}

static inline u32 hisi_pcie_apb_readl(struct hisi_pcie *pcie, u32 reg)
{
	return readl(pcie->reg_base + reg);
}

#define to_hisi_pcie(x)	container_of(x, struct hisi_pcie, pp)

extern struct pcie_host_ops hisi_pcie_host_ops;

#define PCIE_LTSSM_LINKUP_STATE				0x11
#define PCIE_LTSSM_STATE_MASK				0x3F

#endif /* PCIE_HISI_H_ */
