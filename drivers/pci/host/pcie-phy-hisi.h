/*
 * PCIe host controller driver for Hisilicon Hip05 SoCs
 *
 * Copyright (C) 2015 Hisilicon Co., Ltd. http://www.hisilicon.com
 *
 * Author: Dongdong Liu <liudongdong3@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _PCIE_PHY_HISI_H
#define _PCIE_PHY_HISI_H

struct hisi_pcie {
	u8 __iomem *subctrl_base;
	u8 __iomem *reg_base;
	u8 __iomem *phy_base;
	u8 __iomem *serdes_base;
	struct msi_controller *msi;
	u32 port_id;
	struct pcie_port pp;
};

void hisi_pcie_establish_link(struct hisi_pcie *pcie);

#endif /* _PCIE_PHY_HISI_H */
