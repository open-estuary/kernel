/*
 * PCIe host controller driver for Hisilicon Hip05 SoCs
 *
 * Copyright (C) 2015 Hisilicon Co., Ltd. http://www.hisilicon.com
 *
 * Author: Zhou Wang <wangzhou1@xxxxxxxxxxxxx>
 *         Dacai Zhu <zhudacai@xxxxxxxxxxxxx>
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
#include "pcie-phy-hisi.h"

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
	 * init vmid and asid tables for all PCIes device as 0
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

static int hisi_pcie_link_up(struct pcie_port *pp)
{
	u32 val;

	struct hisi_pcie *hisi_pcie = to_hisi_pcie(pp);

	val = hisi_pcie_subctrl_readl(hisi_pcie, PCIE_SUBCTRL_SYS_STATE4_REG +
				      0x100 * hisi_pcie->port_id);

	return ((val & PCIE_LTSSM_STATE_MASK) == PCIE_LTSSM_LINKUP_STATE);
}

static
int hisi_pcie_msi_host_init(struct pcie_port *pp, struct msi_controller *chip)
{
	struct device_node *msi_node;
	struct irq_domain *irq_domain;
	struct device_node *np = pp->dev->of_node;

	msi_node = of_parse_phandle(np, "msi-parent", 0);
	if (!msi_node) {
		pr_err("failed to find msi-parent\n");
		return -ENODEV;
	}

	/* FIX ME: we replace of_pci_find_msi_chip_by_node by irq_find_host
	 * to get irq_domain directly here.
	 *
	 * Why we use irq_find_host is that in v3.19 it did not support
	 * of_pci_find_msi_chip_by_node to get msi controller in ITS driver.
	 * If we use of_pci_find_msi_chip_by_node, we will get NULL, then
	 * kernel crashes :(
	 */
	irq_domain = irq_find_host(msi_node);
	if (!irq_domain) {
		pr_err("failed to find irq domain\n");
		return -ENODEV;
	}

	pp->irq_domain = irq_domain;

	return 0;
}

static void hisi_pcie_host_init(struct pcie_port *pp)
{
	struct hisi_pcie *pcie = to_hisi_pcie(pp);

	hisi_pcie_establish_link(pcie);
	hisi_pcie_config_context(pcie);

	/*
	 * The default size of BAR0 in p660 host bridge is 0x10000000,
	 * which will bring problem when most resource has been allocated
	 * to BAR0 in host bridge.However, we need not use BAR0 in host bridge
	 * in RC mode. Here we just disable it
	 */
	hisi_pcie_disabled_bar0(pcie);
}

static int hisi_pcie_cfg_write(struct pcie_port *pp, int where, int  size,
				u32 val)
{
	u32 reg_val;
	u32 reg;
	struct hisi_pcie *pcie = to_hisi_pcie(pp);
	void *walker = &reg_val;

	walker += (where & 0x3);
	reg = where & ~0x3;
	if (size == 4)
		hisi_pcie_apb_writel(pcie, val, reg);
	else if (size == 2) {
		reg_val = hisi_pcie_apb_readl(pcie, reg);
		*(u16 __force *) walker = val;
		hisi_pcie_apb_writel(pcie, reg_val, reg);
	} else if (size == 1) {
		reg_val = hisi_pcie_apb_readl(pcie, reg);
		*(u8 __force *) walker = val;
		hisi_pcie_apb_writel(pcie, reg_val, reg);
	} else
		return PCIBIOS_BAD_REGISTER_NUMBER;

	return PCIBIOS_SUCCESSFUL;
}

static struct pcie_host_ops hisi_pcie_host_ops = {
	.link_up = hisi_pcie_link_up,
	.msi_host_init = hisi_pcie_msi_host_init,
	.host_init = hisi_pcie_host_init,
	.wr_own_conf = hisi_pcie_cfg_write,
};

static int __init hisi_add_pcie_port(struct pcie_port *pp,
				     struct platform_device *pdev)
{
	int ret;
	u32 port_id;
	struct resource busn;

	struct hisi_pcie *hisi_pcie = to_hisi_pcie(pp);

	if (of_property_read_u32(pdev->dev.of_node, "port-id", &port_id)) {
		dev_err(&pdev->dev, "failed to read port-id\n");
		return -EINVAL;
	}
	if (port_id > 7) {
		dev_err(&pdev->dev, "Invalid port-id\n");
		return -EINVAL;
	}

	hisi_pcie->port_id = port_id % 4;

	if (of_pci_parse_bus_range(pdev->dev.of_node, &busn)) {
		dev_err(&pdev->dev, "failed to parse bus-ranges\n");
		return -EINVAL;
	}

	pp->root_bus_nr = busn.start;
	pp->ops = &hisi_pcie_host_ops;

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
	struct resource *phy;
	struct resource *serdes;
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
		dev_err(pp->dev, "cannot get reg base\n");
		return PTR_ERR(hisi_pcie->reg_base);
	}

	hisi_pcie->pp.dbi_base = hisi_pcie->reg_base;

	phy = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pcs");
	hisi_pcie->phy_base = devm_ioremap_resource(&pdev->dev, phy);
	if (IS_ERR(hisi_pcie->phy_base))
		return PTR_ERR(hisi_pcie->phy_base);

	serdes = platform_get_resource_byname(pdev, IORESOURCE_MEM, "serdes");
	hisi_pcie->serdes_base = devm_ioremap_resource(&pdev->dev, serdes);
	if (IS_ERR(hisi_pcie->serdes_base))
		return PTR_ERR(hisi_pcie->serdes_base);

	ret = hisi_add_pcie_port(pp, pdev);
	if (ret < 0)
		return ret;

	platform_set_drvdata(pdev, hisi_pcie);

	return ret;
}

static const struct of_device_id hisi_pcie_of_match[] = {
	{.compatible = "hisilicon,hip05-pcie",},
	{},
};

MODULE_DEVICE_TABLE(of, hisi_pcie_of_match);

static struct platform_driver hisi_pcie_driver = {
	.driver = {
		   .name = "hisi-pcie",
		   .of_match_table = hisi_pcie_of_match,
		   },
};

static int __init hisi_pcie_init(void)
{
	return platform_driver_probe(&hisi_pcie_driver, hisi_pcie_probe);
}
subsys_initcall(hisi_pcie_init);

MODULE_AUTHOR("Zhou Wang <wangzhou1@xxxxxxxxxx>");
MODULE_AUTHOR("Dacai Zhu <zhudacai@xxxxxxxxxx>");
MODULE_LICENSE("GPL v2");
