/*
 * PCIe host controller driver for HiSilicon Hip05 SoC
 *
 * Copyright (C) 2015 HiSilicon Co., Ltd. http://www.hisilicon.com
 *
 * Author: Zhou Wang <wangzhou1@hisilicon.com>
 *         Dacai Zhu <zhudacai@hisilicon.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_pci.h>
#include <linux/platform_device.h>

#include "pcie-designware.h"

#define PCIE_SUBCTRL_MODE_REG                           0x2800
#define PCIE_SUBCTRL_SYS_STATE4_REG                     0x6818
#define PCIE_SLV_DBI_MODE                               0x0
#define PCIE_SLV_SYSCTRL_MODE                           0x1
#define PCIE_SLV_CONTENT_MODE                           0x2
#define PCIE_SLV_MSI_ASID                               0x10
#define PCIE_LTSSM_LINKUP_STATE                         0x11
#define PCIE_LTSSM_STATE_MASK                           0x3F
#define PCIE_MSI_ASID_ENABLE                            (0x1 << 12)
#define PCIE_MSI_ASID_VALUE                             (0x1 << 16)
#define PCIE_MSI_TRANS_ENABLE                           (0x1 << 12)
#define PCIE_MSI_TRANS_REG                              0x1c8
#define PCIE_MSI_LOW_ADDRESS                            0x1b4
#define PCIE_MSI_HIGH_ADDRESS                           0x1c4
#define PCIE_MSI_ADDRESS_VAL                            0xb7010040

#define PCIE_SYS_CTRL20_REG                             0x20
#define PCIE_CFG_BAR0BASE                               0x10
#define PCIE_DB2_ENABLE_SHIFT                           BIT(0)
#define PCIE_DBI_CS2_ENABLE                             0x1
#define PCIE_DBI_CS2_DISABLE                            0x0

#define to_hisi_pcie(x)	container_of(x, struct hisi_pcie, pp)

struct hisi_pcie {
	void __iomem *subctrl_base;
	void __iomem *reg_base;
	struct msi_controller *msi;
	u32 port_id;
	struct pcie_port pp;
};

static inline void hisi_pcie_subctrl_writel(struct hisi_pcie *pcie,
					    u32 val, u32 reg)
{
	writel(val, pcie->subctrl_base + reg);
}

static inline u32 hisi_pcie_subctrl_readl(struct hisi_pcie *pcie, u32 reg)
{
	return readl(pcie->subctrl_base + reg);
}

static inline void hisi_pcie_apb_writel(struct hisi_pcie *pcie,
					u32 val, u32 reg)
{
	writel(val, pcie->reg_base + reg);
}

static inline u32 hisi_pcie_apb_readl(struct hisi_pcie *pcie, u32 reg)
{
	return readl(pcie->reg_base + reg);
}

/*
 * Change mode to indicate the same reg_base to base of PCIe host configure
 * registers, base of RC configure space or base of vmid/asid context table
 */
static void hisi_pcie_change_apb_mode(struct hisi_pcie *pcie, u32 mode)
{
	u32 val;
	u32 bit_mask;
	u32 bit_shift;
	u32 port_id = pcie->port_id;
	u32 reg = PCIE_SUBCTRL_MODE_REG + 0x100 * port_id;

	if ((port_id == 1) || (port_id == 2)) {
		bit_mask = 0xc;
		bit_shift = 0x2;
	} else {
		bit_mask = 0x6;
		bit_shift = 0x1;
	}

	val = hisi_pcie_subctrl_readl(pcie, reg);
	val = (val & (~bit_mask)) | (mode << bit_shift);
	hisi_pcie_subctrl_writel(pcie, val, reg);
}

/* Configure vmid/asid table in PCIe host */
static void hisi_pcie_config_context(struct hisi_pcie *pcie)
{
	int i;

	hisi_pcie_change_apb_mode(pcie, PCIE_SLV_CONTENT_MODE);

	/*
	 * init vmid and asid tables for all PCIe devices as 0
	 * vmid table: 0 ~ 0x3ff, asid table: 0x400 ~ 0x7ff
	 */
	for (i = 0; i < 0x800; i++)
		hisi_pcie_apb_writel(pcie, 0x0, i * 4);

	hisi_pcie_change_apb_mode(pcie, PCIE_SLV_SYSCTRL_MODE);

	hisi_pcie_apb_writel(pcie, PCIE_MSI_ADDRESS_VAL, PCIE_MSI_LOW_ADDRESS);
	hisi_pcie_apb_writel(pcie, 0x0, PCIE_MSI_HIGH_ADDRESS);
	hisi_pcie_apb_writel(pcie, PCIE_MSI_ASID_ENABLE | PCIE_MSI_ASID_VALUE,
			     PCIE_SLV_MSI_ASID);
	hisi_pcie_apb_writel(pcie, PCIE_MSI_TRANS_ENABLE, PCIE_MSI_TRANS_REG);

	hisi_pcie_change_apb_mode(pcie, PCIE_SLV_DBI_MODE);
}

static int hisi_pcie_link_up(struct pcie_port *pp)
{
	u32 val;
	struct hisi_pcie *hisi_pcie = to_hisi_pcie(pp);

	val = hisi_pcie_subctrl_readl(hisi_pcie, PCIE_SUBCTRL_SYS_STATE4_REG +
				      0x100 * hisi_pcie->port_id);

	return ((val & PCIE_LTSSM_STATE_MASK) == PCIE_LTSSM_LINKUP_STATE);
}

static void hisi_pcie_set_db2_enable(struct hisi_pcie *pcie, u32 enable)
{
	u32 dbi_ctrl;

	hisi_pcie_change_apb_mode(pcie, PCIE_SLV_SYSCTRL_MODE);

	dbi_ctrl = hisi_pcie_apb_readl(pcie, PCIE_SYS_CTRL20_REG);
	if (enable)
		dbi_ctrl |= PCIE_DB2_ENABLE_SHIFT;
	else
		dbi_ctrl &= ~PCIE_DB2_ENABLE_SHIFT;
	hisi_pcie_apb_writel(pcie, dbi_ctrl, PCIE_SYS_CTRL20_REG);

	hisi_pcie_change_apb_mode(pcie, PCIE_SLV_DBI_MODE);
}

static void hisi_pcie_disabled_bar0(struct hisi_pcie *pcie)
{
	hisi_pcie_set_db2_enable(pcie, PCIE_DBI_CS2_ENABLE);
	hisi_pcie_apb_writel(pcie, 0, PCIE_CFG_BAR0BASE);
	hisi_pcie_set_db2_enable(pcie, PCIE_DBI_CS2_DISABLE);
}

static
int hisi_pcie_msi_host_init(struct pcie_port *pp, struct msi_controller *chip)
{
	struct device_node *msi_node;
	struct irq_domain *irq_domain;
	struct device_node *np = pp->dev->of_node;

	msi_node = of_parse_phandle(np, "msi-parent", 0);
	if (!msi_node) {
		dev_err(pp->dev, "failed to find msi-parent\n");
		return -ENODEV;
	}

	irq_domain = irq_find_host(msi_node);
	if (!irq_domain) {
		dev_err(pp->dev, "failed to find irq domain\n");
		return -ENODEV;
	}

	pp->irq_domain = irq_domain;

	return 0;
}

static struct pcie_host_ops hisi_pcie_host_ops = {
	.link_up = hisi_pcie_link_up,
	.msi_host_init = hisi_pcie_msi_host_init,
};

static int hisi_add_pcie_port(struct pcie_port *pp,
			      struct platform_device *pdev)
{
	int ret;
	u32 port_id;
	struct hisi_pcie *hisi_pcie = to_hisi_pcie(pp);

	if (of_property_read_u32(pdev->dev.of_node, "port-id", &port_id)) {
		dev_err(&pdev->dev, "failed to read port-id\n");
		return -EINVAL;
	}
	if (port_id > 3) {
		dev_err(&pdev->dev, "Invalid port-id: %d\n", port_id);
		return -EINVAL;
	}
	hisi_pcie->port_id = port_id;

	/*
	 * The default size of BAR0 in p660 host bridge is 0x10000000,
	 * which will bring problem when most resource has been allocated
	 * to BAR0 in host bridge.However, we need not use BAR0 in host bridge
	 * in RC mode. Here we just disable it
	 */
	hisi_pcie_disabled_bar0(hisi_pcie);

	pp->ops = &hisi_pcie_host_ops;

	hisi_pcie_config_context(hisi_pcie);

	ret = dw_pcie_host_init(pp);
	if (ret) {
		dev_err(&pdev->dev, "failed to initialize host\n");
		return ret;
	}

	return 0;
}

static int hisi_pcie_probe(struct platform_device *pdev)
{
	struct hisi_pcie *hisi_pcie;
	struct pcie_port *pp;
	struct resource *reg;
	struct resource *subctrl;
	int ret;

	hisi_pcie = devm_kzalloc(&pdev->dev, sizeof(*hisi_pcie), GFP_KERNEL);
	if (!hisi_pcie)
		return -ENOMEM;

	pp = &hisi_pcie->pp;
	pp->dev = &pdev->dev;

	subctrl = platform_get_resource_byname(pdev, IORESOURCE_MEM, "subctrl");
	hisi_pcie->subctrl_base = devm_ioremap_nocache(&pdev->dev,
					subctrl->start, resource_size(subctrl));
	if (IS_ERR(hisi_pcie->subctrl_base)) {
		dev_err(pp->dev, "cannot get subctrl base\n");
		return PTR_ERR(hisi_pcie->subctrl_base);
	}

	reg = platform_get_resource_byname(pdev, IORESOURCE_MEM, "rc_dbi");
	hisi_pcie->reg_base = devm_ioremap_resource(&pdev->dev, reg);
	if (IS_ERR(hisi_pcie->reg_base)) {
		dev_err(pp->dev, "cannot get rc_dbi base\n");
		return PTR_ERR(hisi_pcie->reg_base);
	}

	hisi_pcie->pp.dbi_base = hisi_pcie->reg_base;

	ret = hisi_add_pcie_port(pp, pdev);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, hisi_pcie);

	return 0;
}

static const struct of_device_id hisi_pcie_of_match[] = {
	{.compatible = "hisilicon,hip05-pcie",},
	{},
};

MODULE_DEVICE_TABLE(of, hisi_pcie_of_match);

static struct platform_driver hisi_pcie_driver = {
	.probe  = hisi_pcie_probe,
	.driver = {
		   .name = "hisi-pcie",
		   .of_match_table = hisi_pcie_of_match,
	},
};

module_platform_driver(hisi_pcie_driver);
