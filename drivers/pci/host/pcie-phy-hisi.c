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
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_pci.h>
#include <linux/delay.h>

#include "pcie-designware.h"
#include "pcie-phy-hisi.h"

#define PCIE_SUBCTRL_MODE_REG                           0x2800
#define PCIE_SUBCTRL_SYS_STATE4_REG                     0x6818
#define PCIE_SLV_DBI_MODE                               0x0
#define PCIE_SLV_SYSCTRL_MODE                           0x1
#define PCIE_SLV_CONTENT_MODE                           0x2
#define PCIE_LTSSM_LINKUP_STATE                         0x11
#define PCIE_LTSSM_STATE_MASK                           0x3F

#define PCIE_CTRL_7_REG                                 0x114
#define PCIE_LTSSM_ENABLE_SHIFT                         BIT(11)
#define PCIE_SUBCTRL_RESET_REQ_REG                      0xA00
#define PCIE0_2_SUBCTRL_RESET_REQ_REG(port_id) \
	(PCIE_SUBCTRL_RESET_REQ_REG + (port_id << 3))
#define PCIE3_SUBCTRL_RESET_REQ_REG                     0xA68

#define PCIE_SUBCTRL_DRESET_REQ_REG                     0xA04
#define PCIE0_2_SUBCTRL_DRESET_REQ_REG(port_id) \
	(PCIE_SUBCTRL_DRESET_REQ_REG + (port_id << 3))
#define PCIE3_SUBCTRL_DRESET_REQ_REG                    0xA6C

#define PCIE_SUBCTRL_RESET_ST_REG                       0x5A00
#define PCIE0_2_SUBCTRL_RESET_ST_REG(port_id) \
	(PCIE_SUBCTRL_RESET_ST_REG + (port_id << 2))
#define PCIE3_SUBCTRL_RESET_ST_REG                      0x5A34

#define PCIE_SUBCTRL_SC_PCIE0_CLK_DIS_REG               0x304
#define PCIE_SUBCTRL_SC_PCIE0_2_CLK_DIS_REG(port_id) \
	(PCIE_SUBCTRL_SC_PCIE0_CLK_DIS_REG + (port_id << 3))
#define PCIE_SUBCTRL_SC_PCIE3_CLK_DIS_REG               0x324

#define PCIE_SUBCTRL_SC_PCIE0_CLK_ST_REG                0x5300
#define PCIE_SUBCTRL_SC_PCIE0_2_CLK_ST_REG(port_id) \
	(PCIE_SUBCTRL_SC_PCIE0_CLK_ST_REG + (port_id << 2))
#define PCIE_SUBCTRL_SC_PCIE3_CLK_ST_REG                0x5310

#define PCIE_SUBCTRL_SC_PCIE0_CLK_EN_REG                0x300
#define PCIE_SUBCTRL_SC_PCIE0_2_CLK_EN_REG(port_id) \
	(PCIE_SUBCTRL_SC_PCIE0_CLK_EN_REG + (port_id << 3))
#define PCIE_SUBCTRL_SC_PCIE3_CLK_EN_REG                0x320

#define PCIE_ASSERT_RESET_ON                            1
#define PCIE_DEASSERT_RESET_ON                          0
#define PCIE_CLOCK_ON                                   1
#define PCIE_CLOCK_OFF                                  0

#define PCIE_PCS_LOCAL_RESET_REQ                        0xAC0
#define PCIE_PCS_LOCAL_DRESET_REQ                       0xAC4
#define PCIE_PCS_RESET_REQ_REG                          0xA60
#define PCIE_PCS_RESET_REG_ST                           0x5A30
#define PCIE_PCS_LOCAL_DRESET_ST                        0x5A60
#define PCIE_PCS_LOCAL_RESET_ST                         0x5A60
#define PCIE_PCS_DRESET_REQ_REG                         0xA64
#define PCIE_M_PCS_IN15_CFG                             0x74
#define PCIE_M_PCS_IN13_CFG                             0x34
#define PCIE_PCS_RXDETECTED                             0xE4
#define PCIE_PCS_SERDES_STATUS                          0x8108

#define DS_API(lane) ((0x1FF6c + 8 * (15 - lane)) * 2)
#define PCIE_DFE_ENABLE_VAL 0x3851
#define HILINK0_MACRO_LRSTB_REG  0x2424
#define HILINK1_MACRO_LRSTB_REG  0x2524
#define HILINK2_MACRO_LRSTB_REG  0x2424
#define HILINK5_MACRO_LRSTB_REG  0x2624
#define SAS0_DSAF_CFG_BASE  0xc0000000

#define RX_CSR(lane, reg) (((0x4080 + (reg) * 0x0002 + (lane) * 0x0200)) * 2)
#define TX_CSR(lane, reg) (((0x4000 + (reg) * 0x0002 + (lane) * 0x0200)) * 2)
#define DSCLK_CSR(lane, reg) (((0x4100 + (reg) * 0x0002 + (lane) * 0x0200)) * 2)
#define FULL_RATE    0x0
#define HALF_RATE    0x1
#define QUARTER_RATE 0x2
#define OCTAL_RATE 0x3

#define Width_40bit 0x3
#define Width_32bit 0x2
#define Width_20bit 0x1
#define Width_16bit 0x0

#define PCIE_CORE_NUM 8
#define LANE_NUM 8
static u8 serdes_tx_polarity[PCIE_CORE_NUM][LANE_NUM];
static u8 serdes_rx_polarity[PCIE_CORE_NUM][LANE_NUM];

enum pcie_mac_phy_rate_e {
	PCIE_GEN1 = 0,		/*PCIE 1.0 */
	PCIE_GEN2 = 1,		/*PCIE 2.0 */
	PCIE_GEN3 = 2		/*PCIE 3.0 */
};

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

static inline void hisi_pcie_pcs_writel(struct hisi_pcie *pcie,
					u32 val, u32 reg)
{
	writel(val, pcie->phy_base + reg);
}

static inline u32 hisi_pcie_pcs_readl(struct hisi_pcie *pcie, u32 reg)
{
	return readl(pcie->phy_base + reg);
}

static inline void hisi_pcie_serdes_writel(struct hisi_pcie *pcie,
					   u32 val, u32 reg)
{
	writel(val, pcie->serdes_base + reg);
}

static inline u32 hisi_pcie_serdes_readl(struct hisi_pcie *pcie, u32 reg)
{
	return readl(pcie->serdes_base + reg);
}

static void serdes_regbits_write(struct hisi_pcie *pcie, u32 reg,
				 u32 high, u32 low, u32 val)
{
	u32 max_val;
	u32 orign_val;
	u32 final_val;
	u32 mask;
	struct device *dev = pcie->pp.dev;

	if (high < low) {
		high ^= low;
		low ^= high;
		high ^= low;
	}

	max_val = (0x1 << (high - low + 1)) - 1;
	if (val > max_val) {
		dev_err(dev, "val:0x%x invalid\n", val);
		return;
	}
	orign_val = hisi_pcie_serdes_readl(pcie, reg);
	mask = (~(max_val << low)) & 0xffff;
	orign_val &= mask;
	final_val = orign_val | (val << low);
	hisi_pcie_serdes_writel(pcie, final_val, reg);
}

static u32 serdes_regbits_read(struct hisi_pcie *pcie, u32 reg,
			       u32 high, u32 low)
{
	u32 orign_val;
	u32 final_val;
	u32 mask;

	if (high < low) {
		high ^= low;
		low ^= high;
		high ^= low;
	}
	orign_val = hisi_pcie_serdes_readl(pcie, reg);
	orign_val >>= low;
	mask = (0x1 << (high - low + 1)) - 1;
	final_val = orign_val & mask;
	return final_val;
}

static void serdes_polarity_init(struct hisi_pcie *pcie, u32 lane)
{

	u32 val;
	u32 port_id = pcie->port_id;

	val = serdes_regbits_read(pcie, TX_CSR(lane, 2), 8, 8);
	if (val == 1)
		serdes_tx_polarity[port_id][lane] = val;
	val = serdes_regbits_read(pcie, RX_CSR(lane, 10), 8, 8);
	if (val == 1)
		serdes_rx_polarity[port_id][lane] = val;
}

static void serdes_ds_cfg_after_calib(struct hisi_pcie *pcie, u32 lane)
{
	/*Use AC couple mode (cap bypass mode) */
	serdes_regbits_write(pcie, RX_CSR(lane, 0), 13, 13, 0x0);
	serdes_regbits_write(pcie, RX_CSR(lane, 40), 9, 9, 0x0);

	/*Rx termination floating set */
	serdes_regbits_write(pcie, RX_CSR(lane, 27), 12, 12, 0x1);

	/*signal detection and read the result from Pin SQUELCH_DET and ALOS */
	serdes_regbits_write(pcie, RX_CSR(lane, 60), 15, 15, 0x1);

	serdes_regbits_write(pcie, RX_CSR(lane, 31), 0, 0, 0x0);
	serdes_regbits_write(pcie, RX_CSR(lane, 60), 14, 11, 0x5);
	serdes_regbits_write(pcie, RX_CSR(lane, 61), 10, 0, 2);
	serdes_regbits_write(pcie, RX_CSR(lane, 60), 14, 11, 5);
	serdes_regbits_write(pcie, RX_CSR(lane, 62), 10, 0, 0x28);
}

static int serdes_ds_cfg(struct hisi_pcie *pcie, u32 lane)
{
	u32 reg_value;
	u64 reg_addr = 0;
	u32 port_id = pcie->port_id;
	void __iomem *addr;

	switch (port_id) {
	case 0:
		reg_addr = HILINK1_MACRO_LRSTB_REG;
		break;
	case 1:
		reg_addr = HILINK0_MACRO_LRSTB_REG;
		break;
	case 2:
		reg_addr = HILINK2_MACRO_LRSTB_REG;
		break;
	case 3:
		reg_addr = HILINK5_MACRO_LRSTB_REG;
		break;
	default:
		dev_info(pcie->pp.dev, "invalid port_id:%u\n", port_id);
		return -EINVAL;
	}

	/*lane reset */
	if (port_id != 2) {
		reg_value = hisi_pcie_subctrl_readl(pcie, reg_addr);
		reg_value |= (0x1 << lane);
		hisi_pcie_subctrl_writel(pcie, reg_value, reg_addr);
	} else {
		addr = ioremap_nocache(SAS0_DSAF_CFG_BASE + reg_addr, 0x4);
		reg_value = readl(addr);
		reg_value |= (0x1 << lane);
		writel(reg_value, addr);
		iounmap(addr);
	}
	udelay(10);
	/*Power up DSCLK */
	serdes_regbits_write(pcie, DSCLK_CSR(lane, 1), 10, 10, 0x1);
	/*Power up Tx */
	serdes_regbits_write(pcie, TX_CSR(lane, 0), 15, 15, 0x1);
	/*Power up Rx */
	serdes_regbits_write(pcie, RX_CSR(lane, 0), 15, 15, 0x1);
	udelay(100);

	/*Dsclk configuration start */
	serdes_regbits_write(pcie, DSCLK_CSR(lane, 0), 14, 14, 0);

	/* Choose whether bypass Tx phase interpolator */
	serdes_regbits_write(pcie, DSCLK_CSR(lane, 0), 10, 10, 0);

	/* Select Tx spine clock source */
	serdes_regbits_write(pcie, DSCLK_CSR(lane, 0), 13, 13, 0);

	udelay(200);

	if (serdes_tx_polarity[port_id][lane] == 1)
		serdes_regbits_write(pcie, TX_CSR(lane, 2), 8, 8, 1);

	/*Set Tx clock and data source */
	serdes_regbits_write(pcie, TX_CSR(lane, 2), 7, 6, 0x0);
	serdes_regbits_write(pcie, TX_CSR(lane, 2), 5, 4, 0x0);

	/*Set Tx align window dead band and Tx align mode */
	serdes_regbits_write(pcie, TX_CSR(lane, 2), 3, 3, 0x0);
	serdes_regbits_write(pcie, TX_CSR(lane, 2), 2, 0, 0x0);

	serdes_regbits_write(pcie, RX_CSR(lane, 10), 11, 10, HALF_RATE);
	serdes_regbits_write(pcie, RX_CSR(lane, 10), 14, 12, Width_40bit);
	serdes_regbits_write(pcie, TX_CSR(lane, 2), 11, 10, HALF_RATE);
	serdes_regbits_write(pcie, TX_CSR(lane, 2), 14, 12, Width_40bit);
	serdes_regbits_write(pcie, TX_CSR(lane, 1), 15, 10, 0x34);
	serdes_regbits_write(pcie, TX_CSR(lane, 1), 9, 5, 0x1b);

	/* Set Tx FIR coefficients */
	serdes_regbits_write(pcie, TX_CSR(lane, 0), 9, 5, 0);
	serdes_regbits_write(pcie, TX_CSR(lane, 0), 4, 0, 0);
	serdes_regbits_write(pcie, TX_CSR(lane, 1), 4, 0, 0);

	/*Set Tx tap pwrdnb according to settings of pre2/post2 tap setting */
	serdes_regbits_write(pcie, TX_CSR(lane, 10), 7, 0, 0xf6);

	/*Set Tx amplitude to 3 */
	serdes_regbits_write(pcie, TX_CSR(lane, 11), 2, 0, 0x3);

	/*TX termination calibration target resist value choice */
	serdes_regbits_write(pcie, TX_CSR(lane, 34), 1, 0, 0x2);

	/*select deemph from register  txdrv_map_sel */
	serdes_regbits_write(pcie, TX_CSR(lane, 34), 4, 4, 0x1);

	/*Set center phase offset according to rate mode */
	serdes_regbits_write(pcie, RX_CSR(lane, 12), 15, 8, 0x0);

	/*Rx termination calibration target resist value choice. 0-50Ohms */
	serdes_regbits_write(pcie, RX_CSR(lane, 31), 5, 4, 0x1);

	serdes_regbits_write(pcie, RX_CSR(lane, 39), 5, 3, 0x4);

	if (serdes_rx_polarity[port_id][lane] == 1)
		serdes_regbits_write(pcie, RX_CSR(lane, 10), 8, 8, 0x1);

	/*
	   Set CTLE/DFE parameters for common mode or PCIE 2.5Gbps&5Gbps
	   / gain = 777  /boost = 333  /CMB = 222  /RMB = 222  /Zero = 222
	   /SQH = 222
	 */
	serdes_regbits_write(pcie, RX_CSR(lane, 1), 15, 0, 0xbaa7);
	serdes_regbits_write(pcie, RX_CSR(lane, 2), 15, 0, 0x3aa7);
	serdes_regbits_write(pcie, RX_CSR(lane, 3), 15, 0, 0x3aa7);

	serdes_regbits_write(pcie, DSCLK_CSR(lane, 1), 9, 9, 0x1);
	return 0;
}

static void serdes_ds_calib_init(struct hisi_pcie *pcie, u32 lane)
{
	/*Power up eye monitor */
	serdes_regbits_write(pcie, RX_CSR(lane, 15), 15, 15, 0x1);

	/*Set RX, DSCLK and TX to auto calibration mode */
	serdes_regbits_write(pcie, RX_CSR(lane, 27), 15, 15, 0x0);
	serdes_regbits_write(pcie, DSCLK_CSR(lane, 9), 13, 13, 0x0);
	serdes_regbits_write(pcie, DSCLK_CSR(lane, 9), 12, 10, 0x0);
	serdes_regbits_write(pcie, TX_CSR(lane, 14), 5, 4, 0x0);

	/*Make sure Tx2Rx loopback is disabled */
	serdes_regbits_write(pcie, RX_CSR(lane, 0), 10, 10, 0x0);

	/* Set Rx to AC couple mode for calibration */
	serdes_regbits_write(pcie, RX_CSR(lane, 0), 13, 13, 0x0);

	/*ECOARSEALIGNSTEP (RX_CSR45 bit[6:4]) = 0x0;, */
	serdes_regbits_write(pcie, RX_CSR(lane, 45), 6, 4, 0x0);
}

static int serdes_ds_calib_exec(struct hisi_pcie *pcie, u32 lane)
{
	u32 check_count;
	struct device *dev = pcie->pp.dev;

	/*Start data slice calibration by rising edge of DSCALSTART */
	serdes_regbits_write(pcie, RX_CSR(lane, 27), 14, 14, 0x0);
	serdes_regbits_write(pcie, RX_CSR(lane, 27), 14, 14, 0x1);

	/*Check whether calibration complete */
	check_count = 0;
	do {
		check_count++;
		mdelay(1);
	} while (check_count <= 100 &&
		 (serdes_regbits_read(pcie, TX_CSR(lane, 26), 15, 15) == 0 ||
		  serdes_regbits_read(pcie, TX_CSR(lane, 26), 14, 14) == 0));

	if (check_count > 100) {
		dev_err(dev, "lane:%u calibration time out\n", lane);
		return -EINVAL;
	}

	/*Set the loss-of-lock detector to continuous running mode */
	serdes_regbits_write(pcie, DSCLK_CSR(lane, 15), 0, 0, 0x0);

	/*Start dsclk loss of lock detect */
	serdes_regbits_write(pcie, DSCLK_CSR(lane, 10), 9, 9, 0x0);
	serdes_regbits_write(pcie, DSCLK_CSR(lane, 10), 9, 9, 0x1);
	return 0;

}

static void serdes_ds_calib_adjust(struct hisi_pcie *pcie, u32 lane)
{
	int i;
	u32 val;

	/*Power down eye monitor */
	serdes_regbits_write(pcie, RX_CSR(lane, 15), 15, 15, 0x0);

	for (i = 1; i <= 6; i++) {
		serdes_regbits_write(pcie, RX_CSR(lane, 27), 5, 0, i);
		val = serdes_regbits_read(pcie, TX_CSR(lane, 42), 5, 0);
		val = val & 0x1f;

		switch (i) {
		case 1:
			serdes_regbits_write(pcie, RX_CSR(lane, 28),
					     14, 10, val);
			break;
		case 2:
			serdes_regbits_write(pcie, RX_CSR(lane, 28), 9, 5, val);
			break;
		case 3:
			serdes_regbits_write(pcie, RX_CSR(lane, 28), 4, 0, val);
			break;
		case 4:
			serdes_regbits_write(pcie, RX_CSR(lane, 29),
					     14, 10, val);
			break;
		case 5:
			serdes_regbits_write(pcie, RX_CSR(lane, 29), 9, 5, val);
			break;
		case 6:
			serdes_regbits_write(pcie, RX_CSR(lane, 29), 4, 0, val);
			break;
		default:
			break;
		}
	}
	serdes_regbits_write(pcie, DSCLK_CSR(lane, 9), 13, 13, 0x1);
}

static int serdes_ds_calib(struct hisi_pcie *pcie, u32 lane)
{
	int ret;
	struct device *dev = pcie->pp.dev;

	/*First do initialization before calibration */
	serdes_ds_calib_init(pcie, lane);

	/*Then execute Ds hardware calibration */
	ret = serdes_ds_calib_exec(pcie, lane);
	if (ret)
		return ret;

	/*Finally do Ds hardware calibration adjustment */
	serdes_ds_calib_adjust(pcie, lane);

	/*Check calibration results */
	mdelay(1);
	/*Check out of lock when in ring VCO mode */
	if (serdes_regbits_read(pcie, DSCLK_CSR(lane, 0), 1, 1) != 0) {
		dev_err(dev, "lane%u DSCLK  is out of lock\n", lane);
		return -EINVAL;
	}
	dev_info(dev, "lane%u DSCLK  is locked\n", lane);
	return 0;
}

static void serdes_pma_init(struct hisi_pcie *pcie, u32 lane)
{
	/*PMA mode configure:0-PCIe, 1-SAS, 2-Normal Mode */
	serdes_regbits_write(pcie, TX_CSR(lane, 48), 13, 12, 0);

	serdes_regbits_write(pcie, RX_CSR(lane, 54), 15, 13, 0x0);
	serdes_regbits_write(pcie, RX_CSR(lane, 54), 11, 8, 0x9);
	serdes_regbits_write(pcie, RX_CSR(lane, 55), 15, 8, 0xa);
	serdes_regbits_write(pcie, RX_CSR(lane, 55), 7, 0, 0x0);
	serdes_regbits_write(pcie, RX_CSR(lane, 56), 15, 8, 0xa);
	serdes_regbits_write(pcie, RX_CSR(lane, 56), 7, 0, 0x14);

	/*Enable eye metric function */
	serdes_regbits_write(pcie, RX_CSR(lane, 15), 15, 15, 0x1);
	/*Set CDR_MODE. 0-Normal CDR(default), 1-SSCDR */
	serdes_regbits_write(pcie, RX_CSR(lane, 10), 6, 6, 0);
	serdes_regbits_write(pcie, DSCLK_CSR(lane, 0), 9, 5, 0x1f);
	serdes_regbits_write(pcie, RX_CSR(lane, 53), 15, 11, 0x4);

	serdes_regbits_write(pcie, TX_CSR(lane, 48), 11, 11, 0x0);
	/*Configure for pull up rxtx_status and enable pin_en */
	serdes_regbits_write(pcie, TX_CSR(lane, 48), 14, 14, 0x1);
	serdes_regbits_write(pcie, RX_CSR(lane, 60), 15, 15, 0x1);
	serdes_regbits_write(pcie, TX_CSR(lane, 48), 3, 3, 0x1);

}

static int pcie_lane_reset(struct hisi_pcie *pcie, u32 lane)
{
	u32 val;
	u64 reg_addr;
	u32 repeate_time = 0;
	struct device *dev = pcie->pp.dev;
	void __iomem *addr;

	/*disable CTLE/DFE */
	serdes_regbits_write(pcie, DS_API(lane), 7, 7, 0);
	serdes_regbits_write(pcie, DS_API(lane) + 4, 7, 4, 0);

	/*CTLE and DFE adaptation reset bit11 in the PUG control0 */
	serdes_regbits_write(pcie, DS_API(lane) + 4, 3, 3, 1);

	/*CTLE and DFE adaptation reset status */
	repeate_time = 1000;
	val = serdes_regbits_read(pcie, DS_API(lane) + 12, 3, 3);
	while ((1 != val) && repeate_time) {
		udelay(100);
		repeate_time--;
		val = serdes_regbits_read(pcie, DS_API(lane) + 12, 3, 3);
	}

	if (0 == repeate_time)
		dev_info(dev, "lane%u CTLE/DFE reset timeout\n", lane);

	/*CTLE and DFE adaptation reset release bit11 in the PUG control0 */
	serdes_regbits_write(pcie, DS_API(lane) + 4, 3, 3, 0);
	switch (pcie->port_id) {
	case 0:
		reg_addr = HILINK1_MACRO_LRSTB_REG;
		break;
	case 1:
		reg_addr = HILINK0_MACRO_LRSTB_REG;
		break;
	case 2:
		reg_addr = HILINK2_MACRO_LRSTB_REG;
		break;
	case 3:
		reg_addr = HILINK5_MACRO_LRSTB_REG;
		break;
	default:
		dev_info(pcie->pp.dev, "invalid port_id:%u\n", pcie->port_id);
		return -EINVAL;
	}

	/*lane reset */
	if (pcie->port_id != 2) {
		val = hisi_pcie_subctrl_readl(pcie, reg_addr);
		val &= (~(0x1UL << lane));
		hisi_pcie_subctrl_writel(pcie, val, reg_addr);
	} else {
		addr = ioremap_nocache(SAS0_DSAF_CFG_BASE + reg_addr, 0x4);
		val = readl(addr);
		val &= (~(0x1UL << lane));
		writel(val, addr);
		iounmap(addr);
	}

	udelay(10);
	return 0;
}

static int pcie_lane_release_reset(struct hisi_pcie *pcie, u32 lane)
{
	int ret;

	ret = serdes_ds_cfg(pcie, lane);
	if (ret)
		return ret;

	ret = serdes_ds_calib(pcie, lane);
	if (ret)
		return ret;

	serdes_ds_cfg_after_calib(pcie, lane);
	serdes_pma_init(pcie, lane);
	/*ctle enable */
	serdes_regbits_write(pcie, DS_API(lane), 7, 7, 1);
	return 0;
}

static void hisi_pcie_assert_serdes_reset(struct hisi_pcie *pcie)
{
	u32 lane_count = LANE_NUM;
	u32 lane;

	if (pcie->port_id == 3)
		lane_count = 4;

	for (lane = 0; lane < lane_count; lane++) {
		serdes_polarity_init(pcie, lane);
		(void)pcie_lane_reset(pcie, lane);
	}
}

static void hisi_pcie_deassert_serdes_reset(struct hisi_pcie *pcie)
{
	u32 lane_count = LANE_NUM;
	u32 lane;

	if (pcie->port_id == 3)
		lane_count = 4;

	for (lane = 0; lane < lane_count; lane++)
		(void)pcie_lane_release_reset(pcie, lane);

	mdelay(50);
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

static void hisi_pcie_enable_ltssm(struct hisi_pcie *pcie, bool on)
{
	u32 val;

	hisi_pcie_change_apb_mode(pcie, PCIE_SLV_SYSCTRL_MODE);

	val = hisi_pcie_apb_readl(pcie, PCIE_CTRL_7_REG);
	if (on)
		val |= (PCIE_LTSSM_ENABLE_SHIFT);
	else
		val &= ~(PCIE_LTSSM_ENABLE_SHIFT);
	hisi_pcie_apb_writel(pcie, val, PCIE_CTRL_7_REG);

	hisi_pcie_change_apb_mode(pcie, PCIE_SLV_DBI_MODE);
}

static void hisi_pcie_gen3_dfe_enable(struct hisi_pcie *pcie)
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

static int hisi_pcie_link_up(struct pcie_port *pp)
{
	u32 val;

	struct hisi_pcie *pcie = to_hisi_pcie(pp);

	val = hisi_pcie_subctrl_readl(pcie, PCIE_SUBCTRL_SYS_STATE4_REG +
				      0x100 * pcie->port_id);

	return ((val & PCIE_LTSSM_STATE_MASK) == PCIE_LTSSM_LINKUP_STATE);
}

static void hisi_pcie_core_reset_ctrl(struct hisi_pcie *pcie, bool reset_on)
{
	u32 reg_reset_ctrl;
	u32 reg_dereset_ctrl;
	u32 reg_reset_status;

	u32 reset_status;
	u32 reset_status_checked;

	unsigned long timeout;

	u32 port_id = pcie->port_id;

	if (port_id == 3) {
		reg_reset_ctrl = PCIE3_SUBCTRL_RESET_REQ_REG;
		reg_dereset_ctrl = PCIE3_SUBCTRL_DRESET_REQ_REG;
		reg_reset_status = PCIE3_SUBCTRL_RESET_ST_REG;
	} else {
		reg_reset_ctrl = PCIE0_2_SUBCTRL_RESET_REQ_REG(port_id);
		reg_dereset_ctrl = PCIE0_2_SUBCTRL_DRESET_REQ_REG(port_id);
		reg_reset_status = PCIE0_2_SUBCTRL_RESET_ST_REG(port_id);
	}

	if (reset_on) {
		/* if core is link up, stop the ltssm state machine first */
		if (hisi_pcie_link_up(&pcie->pp))
			hisi_pcie_enable_ltssm(pcie, 0);

		hisi_pcie_subctrl_writel(pcie, 0x1, reg_reset_ctrl);
	} else
		hisi_pcie_subctrl_writel(pcie, 0x1, reg_dereset_ctrl);

	/* wait a delay for 1s */
	timeout = jiffies + HZ * 1;

	do {
		reset_status = hisi_pcie_subctrl_readl(pcie, reg_reset_status);
		if (reset_on)
			reset_status_checked = ((reset_status & 0x01) != 1);
		else
			reset_status_checked = ((reset_status & 0x01) != 0);

	} while ((reset_status_checked) && (time_before(jiffies, timeout)));

	/* get a timeout error */
	if (reset_status_checked)
		dev_err(pcie->pp.dev,
			"error:pcie core reset or dereset failed!\n");
}

static void hisi_pcie_clock_ctrl(struct hisi_pcie *pcie, bool clock_on)
{
	u32 reg_clock_disable;
	u32 reg_clock_enable;
	u32 reg_clock_status;

	u32 clock_status;
	u32 clock_status_checked;

	unsigned long timeout;

	u32 port_id = pcie->port_id;

	if (port_id == 3) {
		reg_clock_disable = PCIE_SUBCTRL_SC_PCIE3_CLK_DIS_REG;
		reg_clock_enable = PCIE_SUBCTRL_SC_PCIE3_CLK_EN_REG;
		reg_clock_status = PCIE_SUBCTRL_SC_PCIE3_CLK_ST_REG;
	} else {
		reg_clock_disable =
		    PCIE_SUBCTRL_SC_PCIE0_2_CLK_DIS_REG(port_id);
		reg_clock_enable = PCIE_SUBCTRL_SC_PCIE0_2_CLK_EN_REG(port_id);
		reg_clock_status = PCIE_SUBCTRL_SC_PCIE0_2_CLK_ST_REG(port_id);
	}

	if (clock_on)
		hisi_pcie_subctrl_writel(pcie, 0x3, reg_clock_enable);
	else
		hisi_pcie_subctrl_writel(pcie, 0x3, reg_clock_disable);

	/* wait a delay for 1s */
	timeout = jiffies + HZ * 1;

	do {
		clock_status = hisi_pcie_subctrl_readl(pcie, reg_clock_status);
		if (clock_on)
			clock_status_checked = ((clock_status & 0x03) != 0x3);
		else
			clock_status_checked = ((clock_status & 0x03) != 0);

	} while ((clock_status_checked) && (time_before(jiffies, timeout)));

	/* get a timeout error */
	if (clock_status_checked)
		dev_err(pcie->pp.dev, "error:clock operation failed!\n");
}

static void hisi_pcie_assert_core_reset(struct hisi_pcie *pcie)
{
	hisi_pcie_core_reset_ctrl(pcie, PCIE_ASSERT_RESET_ON);
	hisi_pcie_clock_ctrl(pcie, PCIE_CLOCK_OFF);
}

static void hisi_pcie_deassert_core_reset(struct hisi_pcie *pcie)
{
	hisi_pcie_core_reset_ctrl(pcie, PCIE_DEASSERT_RESET_ON);
	hisi_pcie_clock_ctrl(pcie, PCIE_CLOCK_ON);
}

static void hisi_pcie_deassert_pcs_reset(struct hisi_pcie *pcie)
{
	u32 val;

	u32 hilink_reset_status;
	u32 pcs_local_status;

	u32 hilink_status_checked;
	u32 pcs_local_status_checked;

	unsigned long timeout;

	u32 port_id = pcie->port_id;

	val = 1 << port_id;
	hisi_pcie_subctrl_writel(pcie, val, PCIE_PCS_LOCAL_DRESET_REQ);

	val = 0xff << (port_id << 3);
	hisi_pcie_subctrl_writel(pcie, val, PCIE_PCS_DRESET_REQ_REG);

	/* wait a delay for 1s */
	timeout = jiffies + HZ * 1;

	/*read reset status,make sure pcs is deassert */
	do {
		pcs_local_status = hisi_pcie_subctrl_readl(pcie,
						PCIE_PCS_LOCAL_RESET_ST);
		pcs_local_status_checked = (pcs_local_status & (1 << port_id));
	} while ((pcs_local_status_checked) && (time_before(jiffies, timeout)));

	/* get a timeout error */
	if (pcs_local_status_checked)
		dev_err(pcie->pp.dev, "pcs deassert reset failed!\n");

	timeout = jiffies + HZ * 1;

	do {
		hilink_reset_status = hisi_pcie_subctrl_readl(pcie,
						PCIE_PCS_RESET_REG_ST);
		hilink_status_checked = (hilink_reset_status &
					 (0xff << (port_id << 3)));
	} while ((hilink_status_checked) && (time_before(jiffies, timeout)));

	if (hilink_status_checked)
		dev_err(pcie->pp.dev, "pcs deassert reset  failed!\n");
}

static void hisi_pcie_assert_pcs_reset(struct hisi_pcie *pcie)
{
	u32 reg;
	u32 hilink_reset_status;
	u32 pcs_local_reset_status;

	u32 hilink_status_checked;
	u32 pcs_local_status_checked;

	unsigned long timeout;

	u32 port_id = pcie->port_id;

	reg = 1 << port_id;
	hisi_pcie_subctrl_writel(pcie, reg, PCIE_PCS_LOCAL_RESET_REQ);

	reg = 0xff << (port_id << 3);
	hisi_pcie_subctrl_writel(pcie, reg, PCIE_PCS_RESET_REQ_REG);

	/* wait a delay for 1s */
	timeout = jiffies + HZ * 1;

	/*read reset status,make sure pcs is reset */
	do {
		pcs_local_reset_status = hisi_pcie_subctrl_readl(pcie,
						PCIE_PCS_LOCAL_RESET_ST);
		pcs_local_status_checked =
		    ((pcs_local_reset_status & (1 << port_id)) !=
		     (1 << port_id));

	} while ((pcs_local_status_checked) && (time_before(jiffies, timeout)));

	if (pcs_local_status_checked)
		dev_err(pcie->pp.dev, "pcs local reset status read failed\n");

	/* wait a delay for 1s */
	timeout = jiffies + HZ * 1;

	do {
		hilink_reset_status = hisi_pcie_subctrl_readl(pcie,
						PCIE_PCS_RESET_REG_ST);
		hilink_status_checked =
		    ((hilink_reset_status & (0xff << (port_id << 3))) !=
		     (0xff << (port_id << 3)));
	} while ((hilink_status_checked) && (time_before(jiffies, timeout)));

	if (hilink_status_checked)
		dev_err(pcie->pp.dev, "error:pcs assert reset failed\n");
}

static void hisi_pcie_init_pcs(struct hisi_pcie *pcie)
{
	u32 lane_num = pcie->pp.lanes;
	u32 i;

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

void pcie_equalization(struct hisi_pcie *pcie)
{
	u32 val;

	if (pcie->port_id <= 2) {
		hisi_pcie_apb_writel(pcie, 0x1400, 0x890);
		hisi_pcie_apb_writel(pcie, 0xfd7, 0x894);

		val = hisi_pcie_apb_readl(pcie, 0x80);
		val |= 0x80;
		hisi_pcie_apb_writel(pcie, val, 0x80);

		hisi_pcie_apb_writel(pcie, 0x0, 0x89c);
		hisi_pcie_apb_writel(pcie, 0xfc00, 0x898);
		hisi_pcie_apb_writel(pcie, 0x1, 0x89c);
		hisi_pcie_apb_writel(pcie, 0xbd00, 0x898);
		hisi_pcie_apb_writel(pcie, 0x2, 0x89c);
		hisi_pcie_apb_writel(pcie, 0xccc0, 0x898);
		hisi_pcie_apb_writel(pcie, 0x3, 0x89c);
		hisi_pcie_apb_writel(pcie, 0x8dc0, 0x898);
		hisi_pcie_apb_writel(pcie, 0x4, 0x89c);
		hisi_pcie_apb_writel(pcie, 0xfc0, 0x898);
		hisi_pcie_apb_writel(pcie, 0x5, 0x89c);
		hisi_pcie_apb_writel(pcie, 0xe46, 0x898);
		hisi_pcie_apb_writel(pcie, 0x6, 0x89c);
		hisi_pcie_apb_writel(pcie, 0xdc8, 0x898);
		hisi_pcie_apb_writel(pcie, 0x7, 0x89c);
		hisi_pcie_apb_writel(pcie, 0xcb46, 0x898);
		hisi_pcie_apb_writel(pcie, 0x8, 0x89c);
		hisi_pcie_apb_writel(pcie, 0x8c07, 0x898);
		hisi_pcie_apb_writel(pcie, 0x9, 0x89c);
		hisi_pcie_apb_writel(pcie, 0xd0b, 0x898);
		hisi_pcie_apb_writel(pcie, 0x103ff21, 0x8a8);

		hisi_pcie_apb_writel(pcie, 0x44444444, 0x184);
		hisi_pcie_apb_writel(pcie, 0x44444444, 0x188);
		hisi_pcie_apb_writel(pcie, 0x44444444, 0x18c);
		hisi_pcie_apb_writel(pcie, 0x44444444, 0x190);
	} else {
		hisi_pcie_apb_writel(pcie, 0x10e01, 0x890);
	}
}

void hisi_pcie_establish_link(struct hisi_pcie *pcie)
{
	u32 val;
	int count = 0;

	/* assert reset signals */
	hisi_pcie_assert_core_reset(pcie);
	hisi_pcie_assert_pcs_reset(pcie);
#ifndef CONFIG_P660_2P
	hisi_pcie_assert_serdes_reset(pcie);

	/* de-assert serdes reset */
	hisi_pcie_deassert_serdes_reset(pcie);
#endif
	/* de-assert phy reset */
	hisi_pcie_deassert_pcs_reset(pcie);

	/* de-assert core reset */
	hisi_pcie_deassert_core_reset(pcie);

	/* initialize phy */
	hisi_pcie_init_pcs(pcie);

	/* setup root complex */
	dw_pcie_setup_rc(&pcie->pp);

	pcie_equalization(pcie);

	/* assert LTSSM enable */
	hisi_pcie_enable_ltssm(pcie, 1);

	/* check if the link is up or not */
	while (!dw_pcie_link_up(&pcie->pp)) {
		mdelay(100);
		count++;
		if (count == 10) {
			val = hisi_pcie_pcs_readl(pcie, PCIE_PCS_SERDES_STATUS);
			dev_info(pcie->pp.dev,
				 "PCIe Link Failed! PLL Locked: 0x%x\n", val);
			return;
		}
	}

	dev_info(pcie->pp.dev, "Link up\n");

	/*add a 1s delay between linkup and enumeration,make sure
	   the EP device's configuration registers are prepared well */
	mdelay(999);

	hisi_pcie_gen3_dfe_enable(pcie);
}
