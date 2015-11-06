/*
 * PCIe host controller driver for HiSilicon Hip05 SoC
 *
 * Copyright (C) 2015 HiSilicon Co., Ltd. http://www.hisilicon.com
 *
 * Authors: Zhou Wang <wangzhou1@hisilicon.com>
 *          Gabriele Paoloni <gabriele.paoloni@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pci.h>
#include <linux/irqreturn.h>
#include "pcie-designware.h"
#include "pcie-hisi.h"

#define HIP05_PCIE_CLK_CTRL                             0x3
#define HIP05_PCS_SERDES_STATUS                         0x8108
#define HIP05_PCIE_CORE_RESET                           0x1

/*Check if the link is up*/
static int hisi_pcie_link_up_hip05(struct hisi_pcie *pcie)
{
	u32 val;

	val = hisi_pcie_subctrl_readl(pcie,
			PCIE_SUBCTRL_SYS_STATE4_REG + 0x100 * pcie->port_id);

	return ((val & PCIE_LTSSM_STATE_MASK) == PCIE_LTSSM_LINKUP_STATE);
}

/* Configure vmid/asid table in PCIe host */
static void hisi_pcie_config_context_hip05(struct hisi_pcie *pcie)
{
	int i;
	u32 val;

	/*
	 * enable to clean vmid and asid tables though apb bus
	 * */
	hisi_pcie_change_apb_mode(pcie, PCIE_SLV_SYSCTRL_MODE);

	val = hisi_pcie_apb_readl(pcie, PCIE_SYS_CTRL20_REG);
	/* enable ar channel */
	val |= PCIE_RD_TAB_SEL | PCIE_RD_TAB_EN;
	hisi_pcie_apb_writel(pcie, val, PCIE_SYS_CTRL20_REG);

	hisi_pcie_change_apb_mode(pcie, PCIE_SLV_CONTENT_MODE);
	for (i = 0; i < 0x800; i++)
		hisi_pcie_apb_writel(pcie, 0x0, i * 4);

	hisi_pcie_change_apb_mode(pcie, PCIE_SLV_SYSCTRL_MODE);
	/* enable aw channel */
	val &= (~PCIE_RD_TAB_SEL);
	val |= PCIE_RD_TAB_EN;
	hisi_pcie_apb_writel(pcie, val, PCIE_SYS_CTRL20_REG);

	hisi_pcie_change_apb_mode(pcie, PCIE_SLV_CONTENT_MODE);

	/*
	 * init vmid and asid tables for all PCIe devices as 0
	 * vmid table: 0 ~ 0x3ff, asid table: 0x400 ~ 0x7ff
	 */
	for (i = 0; i < 0x800; i++)
		hisi_pcie_apb_writel(pcie, 0x0, i * 4);

	hisi_pcie_change_apb_mode(pcie, PCIE_SLV_SYSCTRL_MODE);

	val = hisi_pcie_apb_readl(pcie, PCIE_SYS_CTRL20_REG);
	/* disable ar channel */
	val |= PCIE_RD_TAB_SEL;
	val &= (~PCIE_RD_TAB_EN);
	hisi_pcie_apb_writel(pcie, val, PCIE_SYS_CTRL20_REG);
	/* disable aw channel */
	val &= ((~PCIE_RD_TAB_SEL) & (~PCIE_RD_TAB_EN));
	hisi_pcie_apb_writel(pcie, val, PCIE_SYS_CTRL20_REG);

	hisi_pcie_apb_writel(pcie, pcie->msi_addr & 0xffffffff, PCIE_MSI_LOW_ADDRESS);
	hisi_pcie_apb_writel(pcie, pcie->msi_addr >> 32, PCIE_MSI_HIGH_ADDRESS);
	hisi_pcie_apb_writel(pcie, PCIE_MSI_ASID_ENABLE | PCIE_MSI_ASID_VALUE,
				 PCIE_SLV_MSI_ASID);
	hisi_pcie_apb_writel(pcie, PCIE_MSI_TRANS_ENABLE, PCIE_MSI_TRANS_REG);

	hisi_pcie_change_apb_mode(pcie, PCIE_SLV_DBI_MODE);
}

static void hisi_pcie_enable_ltssm_hip05(struct hisi_pcie *pcie, bool on)
{
	u32 val;

	hisi_pcie_change_apb_mode(pcie, PCIE_SLV_SYSCTRL_MODE);
	val = hisi_pcie_apb_readl(pcie, PCIE_CTRL_7_REG);
	if (on)
		val |= PCIE_LTSSM_ENABLE_SHIFT;
	else
		val &= ~(PCIE_LTSSM_ENABLE_SHIFT);
	hisi_pcie_apb_writel(pcie, val, PCIE_CTRL_7_REG);
	hisi_pcie_change_apb_mode(pcie, PCIE_SLV_DBI_MODE);
}

static void hisi_pcie_gen3_dfe_enable_hip05(struct hisi_pcie *pcie)
{
	u32 val;
	u32 lane;
	u32 port_id = pcie->port_id;
	u32 current_speed;

	if (port_id == 3)
		return;
	val = hisi_pcie_subctrl_readl(pcie,
					  PCIE_SUBCTRL_SYS_STATE4_REG +
					  0x100 * port_id);
	current_speed = (val >> 6) & 0x3;
	if (current_speed != PCIE_GEN3)
		return;
	for (lane = 0; lane < 8; lane++)
		hisi_pcie_serdes_writel(pcie,
				PCIE_DFE_ENABLE_VAL, DS_API(lane) + 4);

	dev_info(pcie->pp.dev, "enable DFE success!\n");
}

static void hisi_pcie_init_pcs_hip05(struct hisi_pcie *pcie)
{
	u32 i;
	u32 lane_num = pcie->pp.lanes;

	if (pcie->port_id <= 2) {
		hisi_pcie_serdes_writel(pcie, 0x212, 0xc088);

		hisi_pcie_pcs_writel(pcie, 0x2026044, 0x8020);
		hisi_pcie_pcs_writel(pcie, 0x2126044, 0x8060);
		hisi_pcie_pcs_writel(pcie, 0x2126044, 0x80c4);
		hisi_pcie_pcs_writel(pcie, 0x2026044, 0x80e4);
		hisi_pcie_pcs_writel(pcie, 0x4018, 0x80a0);
		hisi_pcie_pcs_writel(pcie, 0x804018, 0x80a4);
		hisi_pcie_pcs_writel(pcie, 0x11201100, 0x80c0);
		hisi_pcie_pcs_writel(pcie, 0x3, 0x15c);
		hisi_pcie_pcs_writel(pcie, 0x0, 0x158);
	} else {
		for (i = 0; i < lane_num; i++)
			hisi_pcie_pcs_writel(pcie, 0x46e000,
						 PCIE_M_PCS_IN15_CFG + (i << 2));
		for (i = 0; i < lane_num; i++)
			hisi_pcie_pcs_writel(pcie, 0x1001,
						 PCIE_M_PCS_IN13_CFG + (i << 2));

		hisi_pcie_pcs_writel(pcie, 0xffff, PCIE_PCS_RXDETECTED);
	}
}

void pcie_equalization_hip05(struct hisi_pcie *pcie)
{
	u32 val;

	hisi_pcie_apb_writel(pcie, 0x1400, 0x890);

	pcie_equalization_common(pcie);

	val = hisi_pcie_apb_readl(pcie, 0x80);
	val |= 0x80;
	hisi_pcie_apb_writel(pcie, val, 0x80);

	hisi_pcie_apb_writel(pcie, 0x44444444, 0x184);
	hisi_pcie_apb_writel(pcie, 0x44444444, 0x188);
	hisi_pcie_apb_writel(pcie, 0x44444444, 0x18c);
	hisi_pcie_apb_writel(pcie, 0x44444444, 0x190);
}


static void hisi_pcie_mode_set_hip05(struct hisi_pcie *pcie)
{
	hisi_pcie_change_apb_mode(pcie, PCIE_SLV_SYSCTRL_MODE);
	hisi_pcie_apb_writel(pcie, 0x4 << 28, PCIE_CORE_MODE_REG);
	hisi_pcie_change_apb_mode(pcie, PCIE_SLV_DBI_MODE);
}

static struct pcie_soc_ops hip05_ops = {
		&hisi_pcie_link_up_hip05,
		&hisi_pcie_config_context_hip05,
		&hisi_pcie_enable_ltssm_hip05,
		&hisi_pcie_gen3_dfe_enable_hip05,
		&hisi_pcie_init_pcs_hip05,
		&pcie_equalization_hip05,
		&hisi_pcie_mode_set_hip05,
		NULL,
		HIP05_PCIE_CORE_RESET,
		HIP05_PCIE_CLK_CTRL,
		HIP05_PCS_SERDES_STATUS,
		PCIE_HOST_HIP05
};

static const struct of_device_id hisi_pcie_of_match_hip05[] = {
	{
			.compatible = "hisilicon,hip05-pcie",
			.data	    = (void *) &hip05_ops,
	},
	{},
};


MODULE_DEVICE_TABLE(of, hisi_pcie_of_match);

static struct platform_driver hisi_pcie_driver_hip05 = {
	.driver = {
		   .name = "hisi-pcie-hip05",
		   .owner = THIS_MODULE,
		   .of_match_table = hisi_pcie_of_match_hip05,
	},
};

static int __init hisi_pcie_init_hip05(void)
{
	return platform_driver_probe(&hisi_pcie_driver_hip05, hisi_pcie_probe);
}
subsys_initcall(hisi_pcie_init_hip05);

MODULE_AUTHOR("Zhou Wang <wangzhou1@hisilicon.com>");
MODULE_AUTHOR("Dacai Zhu <zhudacai@hisilicon.com>");
MODULE_AUTHOR("Gabriele Paoloni <gabriele.paoloni@huawei.com>");
MODULE_LICENSE("GPL v2");