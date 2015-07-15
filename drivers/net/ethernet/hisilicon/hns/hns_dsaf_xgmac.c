/*
 * Copyright (c) 2014-2015 Hisilicon Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/of_mdio.h>
#include "hns_dsaf_main.h"
#include "hns_dsaf_mac.h"
#include "hns_dsaf_xgmac.h"
#include "hns_dsaf_reg.h"

/**
 *hns_xgmac_tx_enable - xgmac port tx enable
 *@drv: mac driver
 *@value: value of enable
 */
static void hns_xgmac_tx_enable(struct mac_driver *drv, u32 value)
{
	dsaf_set_dev_bit(drv, XGMAC_MAC_ENABLE_REG, XGMAC_ENABLE_TX_B, !!value);
}

/**
 *hns_xgmac_rx_enable - xgmac port rx enable
 *@drv: mac driver
 *@value: value of enable
 */
static void hns_xgmac_rx_enable(struct mac_driver *drv, u32 value)
{
	dsaf_set_dev_bit(drv, XGMAC_MAC_ENABLE_REG, XGMAC_ENABLE_RX_B, !!value);
}

/**
 *hns_xgmac_enable - enable xgmac port
 *@drv: mac driver
 *@mode: mode of mac port
 */
static void hns_xgmac_enable(void *mac_drv, enum mac_commom_mode mode)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;
	struct dsaf_device *dsaf_dev
		= (struct dsaf_device *)dev_get_drvdata(drv->dev);
	u32 port = drv->mac_id;

	hns_dsaf_xge_core_srst_by_port(dsaf_dev, port, 1);
	mdelay(10);

	/*enable XGE rX/tX */
	if (MAC_COMM_MODE_TX == mode) {
		hns_xgmac_tx_enable(drv, 1);
	} else if (MAC_COMM_MODE_RX == mode) {
		hns_xgmac_rx_enable(drv, 1);
	} else if (MAC_COMM_MODE_RX_AND_TX == mode) {
		hns_xgmac_tx_enable(drv, 1);
		hns_xgmac_rx_enable(drv, 1);
	} else {
		dev_err(drv->dev, "error mac mode:%d\n", mode);
	}
}

/**
 *hns_xgmac_disable - disable xgmac port
 *@mac_drv: mac driver
 *@mode: mode of mac port
 */
static void hns_xgmac_disable(void *mac_drv, enum mac_commom_mode mode)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;
	struct dsaf_device *dsaf_dev
		= (struct dsaf_device *)dev_get_drvdata(drv->dev);
	u32 port = drv->mac_id;

	if (MAC_COMM_MODE_TX == mode) {
		hns_xgmac_tx_enable(drv, 0);
	} else if (MAC_COMM_MODE_RX == mode) {
		hns_xgmac_rx_enable(drv, 0);
	} else if (MAC_COMM_MODE_RX_AND_TX == mode) {
		hns_xgmac_tx_enable(drv, 0);
		hns_xgmac_rx_enable(drv, 0);
	}

	hns_dsaf_xge_core_srst_by_port(dsaf_dev, port, 0);
	mdelay(10);
}

/**
 *hns_xgmac_pma_fec_enable - xgmac PMA FEC enable
 *@drv: mac driver
 *@tx_value: tx value
 *@rx_value: rx value
 *return status
 */
static void hns_xgmac_pma_fec_enable(struct mac_driver *drv, u32 tx_value,
				     u32 rx_value)
{
	u32 origin = dsaf_read_dev(drv, XGMAC_PMA_FEC_CONTROL_REG);

	dsaf_set_bit(origin, XGMAC_PMA_FEC_CTL_TX_B, !!tx_value);
	dsaf_set_bit(origin, XGMAC_PMA_FEC_CTL_RX_B, !!rx_value);
	dsaf_write_dev(drv, XGMAC_PMA_FEC_CONTROL_REG, origin);
}

/* clr exc irq for xge*/
static inline void hns_xgmac_exc_irq_en(struct mac_driver *drv, u32 en)
{
	u32 clr_vlue = 0xfffffffful;
	u32 msk_vlue = en ? 0xfffffffful : 0; /*1 is en, 0 is dis*/

	dsaf_write_dev(drv, XGMAC_INT_STATUS_REG, clr_vlue);
	dsaf_write_dev(drv, XGMAC_INT_ENABLE_REG, msk_vlue);
}

/**
 *hns_xgmac_init - initialize XGE
 *@mac_drv: mac driver
 */
static void hns_xgmac_init(void *mac_drv)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;
	struct dsaf_device *dsaf_dev
		= (struct dsaf_device *)dev_get_drvdata(drv->dev);
	u32 port = drv->mac_id;

	hns_dsaf_xge_srst_by_port(dsaf_dev, port, 0);
	mdelay(100);
	hns_dsaf_xge_srst_by_port(dsaf_dev, port, 1);

	mdelay(100);
	hns_xgmac_exc_irq_en(drv, 0);

	hns_xgmac_pma_fec_enable(drv, 0x0, 0x0);

	hns_xgmac_disable(mac_drv, MAC_COMM_MODE_RX_AND_TX);
}

/**
 *hns_xgmac_config_pad_and_crc - set xgmac pad and crc enable the same time
 *@mac_drv: mac driver
 *@newval:enable of pad and crc
 */
static void hns_xgmac_config_pad_and_crc(void *mac_drv, u8 newval)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;
	u32 origin = dsaf_read_dev(drv, XGMAC_MAC_CONTROL_REG);

	dsaf_set_bit(origin, XGMAC_CTL_TX_PAD_B, !!newval);
	dsaf_set_bit(origin, XGMAC_CTL_TX_FCS_B, !!newval);
	dsaf_set_bit(origin, XGMAC_CTL_RX_FCS_B, !!newval);
	dsaf_write_dev(drv, XGMAC_MAC_CONTROL_REG, origin);
}

/**
 *hns_xgmac_pausefrm_cfg - set pause param about xgmac
 *@mac_drv: mac driver
 *@newval:enable of pad and crc
 */
static void hns_xgmac_pausefrm_cfg(void *mac_drv, u32 rx_en, u32 tx_en)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;
	u32 origin = dsaf_read_dev(drv, XGMAC_MAC_PAUSE_CTRL_REG);

	dsaf_set_bit(origin, XGMAC_PAUSE_CTL_TX_B, !!tx_en);
	dsaf_set_bit(origin, XGMAC_PAUSE_CTL_RX_B, !!rx_en);
	dsaf_write_dev(drv, XGMAC_MAC_PAUSE_CTRL_REG, origin);
}

/**
 *hns_xgmac_set_rx_ignore_pause_frames - set rx pause param about xgmac
 *@mac_drv: mac driver
 *@enable:enable rx pause param
 */
static void hns_xgmac_set_rx_ignore_pause_frames(void *mac_drv, u32 enable)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	dsaf_set_dev_bit(drv, XGMAC_MAC_PAUSE_CTRL_REG,
			 XGMAC_PAUSE_CTL_RX_B, !!enable);
}

/**
 *hns_xgmac_set_tx_auto_pause_frames - set tx pause param about xgmac
 *@mac_drv: mac driver
 *@enable:enable tx pause param
 */
static void hns_xgmac_set_tx_auto_pause_frames(void *mac_drv, u16 enable)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	dsaf_set_dev_bit(drv, XGMAC_MAC_PAUSE_CTRL_REG,
			 XGMAC_PAUSE_CTL_TX_B, !!enable);

	/*if enable is not zero ,set tx pause time */
	if (enable)
		dsaf_write_dev(drv, XGMAC_MAC_PAUSE_TIME_REG, enable);
}

/**
 *hns_xgmac_get_id - get xgmac port id
 *@mac_drv: mac driver
 *@newval:xgmac max frame length
 */
static void hns_xgmac_get_id(void *mac_drv, u8 *mac_id)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	*mac_id = drv->mac_id;
}

/**
 *hns_xgmac_config_max_frame_length - set xgmac max frame length
 *@mac_drv: mac driver
 *@newval:xgmac max frame length
 */
static void hns_xgmac_config_max_frame_length(void *mac_drv, u16 newval)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	dsaf_write_dev(drv, XGMAC_MAC_MAX_PKT_SIZE_REG, newval);
}

void hns_xgmac_update_stats(void *mac_drv)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;
	struct mac_hw_stats *hw_stats = &drv->mac_cb->hw_stats;

	/* TX */
	hw_stats->tx_fragment_err
		= hns_mac_reg_read64(drv, XGMAC_TX_PKTS_FRAGMENT);
	hw_stats->tx_undersize
		= hns_mac_reg_read64(drv, XGMAC_TX_PKTS_UNDERSIZE);
	hw_stats->tx_under_min_pkts
		= hns_mac_reg_read64(drv, XGMAC_TX_PKTS_UNDERMIN);
	hw_stats->tx_64bytes = hns_mac_reg_read64(drv, XGMAC_TX_PKTS_64OCTETS);
	hw_stats->tx_65to127
		= hns_mac_reg_read64(drv, XGMAC_TX_PKTS_65TO127OCTETS);
	hw_stats->tx_128to255
		= hns_mac_reg_read64(drv, XGMAC_TX_PKTS_128TO255OCTETS);
	hw_stats->tx_256to511
		= hns_mac_reg_read64(drv, XGMAC_TX_PKTS_256TO511OCTETS);
	hw_stats->tx_512to1023
		= hns_mac_reg_read64(drv, XGMAC_TX_PKTS_512TO1023OCTETS);
	hw_stats->tx_1024to1518
		= hns_mac_reg_read64(drv, XGMAC_TX_PKTS_1024TO1518OCTETS);
	hw_stats->tx_1519tomax
		= hns_mac_reg_read64(drv, XGMAC_TX_PKTS_1519TOMAXOCTETS);
	hw_stats->tx_1519tomax_good
		= hns_mac_reg_read64(drv, XGMAC_TX_PKTS_1519TOMAXOCTETSOK);
	hw_stats->tx_oversize = hns_mac_reg_read64(drv, XGMAC_TX_PKTS_OVERSIZE);
	hw_stats->tx_jabber_err = hns_mac_reg_read64(drv, XGMAC_TX_PKTS_JABBER);
	hw_stats->tx_good_pkts = hns_mac_reg_read64(drv, XGMAC_TX_GOODPKTS);
	hw_stats->tx_good_bytes = hns_mac_reg_read64(drv, XGMAC_TX_GOODOCTETS);
	hw_stats->tx_total_pkts = hns_mac_reg_read64(drv, XGMAC_TX_TOTAL_PKTS);
	hw_stats->tx_total_bytes
		= hns_mac_reg_read64(drv, XGMAC_TX_TOTALOCTETS);
	hw_stats->tx_uc_pkts = hns_mac_reg_read64(drv, XGMAC_TX_UNICASTPKTS);
	hw_stats->tx_mc_pkts = hns_mac_reg_read64(drv, XGMAC_TX_MULTICASTPKTS);
	hw_stats->tx_bc_pkts = hns_mac_reg_read64(drv, XGMAC_TX_BROADCASTPKTS);
	hw_stats->tx_pfc_tc0 = hns_mac_reg_read64(drv, XGMAC_TX_PRI0PAUSEPKTS);
	hw_stats->tx_pfc_tc1 = hns_mac_reg_read64(drv, XGMAC_TX_PRI1PAUSEPKTS);
	hw_stats->tx_pfc_tc2 = hns_mac_reg_read64(drv, XGMAC_TX_PRI2PAUSEPKTS);
	hw_stats->tx_pfc_tc3 = hns_mac_reg_read64(drv, XGMAC_TX_PRI3PAUSEPKTS);
	hw_stats->tx_pfc_tc4 = hns_mac_reg_read64(drv, XGMAC_TX_PRI4PAUSEPKTS);
	hw_stats->tx_pfc_tc5 = hns_mac_reg_read64(drv, XGMAC_TX_PRI5PAUSEPKTS);
	hw_stats->tx_pfc_tc6 = hns_mac_reg_read64(drv, XGMAC_TX_PRI6PAUSEPKTS);
	hw_stats->tx_pfc_tc7 = hns_mac_reg_read64(drv, XGMAC_TX_PRI7PAUSEPKTS);
	hw_stats->tx_ctrl = hns_mac_reg_read64(drv, XGMAC_TX_MACCTRLPKTS);
	hw_stats->tx_1731_pkts = hns_mac_reg_read64(drv, XGMAC_TX_1731PKTS);
	hw_stats->tx_1588_pkts = hns_mac_reg_read64(drv, XGMAC_TX_1588PKTS);
	hw_stats->rx_good_from_sw
		= hns_mac_reg_read64(drv, XGMAC_RX_FROMAPPGOODPKTS);
	hw_stats->rx_bad_from_sw
		= hns_mac_reg_read64(drv, XGMAC_RX_FROMAPPBADPKTS);
	hw_stats->tx_bad_pkts = hns_mac_reg_read64(drv, XGMAC_TX_ERRALLPKTS);

	/* RX */
	hw_stats->rx_fragment_err
		= hns_mac_reg_read64(drv, XGMAC_RX_PKTS_FRAGMENT);
	hw_stats->rx_undersize
		= hns_mac_reg_read64(drv, XGMAC_RX_PKTSUNDERSIZE);
	hw_stats->rx_under_min
		= hns_mac_reg_read64(drv, XGMAC_RX_PKTS_UNDERMIN);
	hw_stats->rx_64bytes = hns_mac_reg_read64(drv, XGMAC_RX_PKTS_64OCTETS);
	hw_stats->rx_65to127
		= hns_mac_reg_read64(drv, XGMAC_RX_PKTS_65TO127OCTETS);
	hw_stats->rx_128to255
		= hns_mac_reg_read64(drv, XGMAC_RX_PKTS_128TO255OCTETS);
	hw_stats->rx_256to511
		= hns_mac_reg_read64(drv, XGMAC_RX_PKTS_256TO511OCTETS);
	hw_stats->rx_512to1023
		= hns_mac_reg_read64(drv, XGMAC_RX_PKTS_512TO1023OCTETS);
	hw_stats->rx_1024to1518
		= hns_mac_reg_read64(drv, XGMAC_RX_PKTS_1024TO1518OCTETS);
	hw_stats->rx_1519tomax
		= hns_mac_reg_read64(drv, XGMAC_RX_PKTS_1519TOMAXOCTETS);
	hw_stats->rx_1519tomax_good
		= hns_mac_reg_read64(drv, XGMAC_RX_PKTS_1519TOMAXOCTETSOK);
	hw_stats->rx_oversize = hns_mac_reg_read64(drv, XGMAC_RX_PKTS_OVERSIZE);
	hw_stats->rx_jabber_err = hns_mac_reg_read64(drv, XGMAC_RX_PKTS_JABBER);
	hw_stats->rx_good_pkts = hns_mac_reg_read64(drv, XGMAC_RX_GOODPKTS);
	hw_stats->rx_good_bytes = hns_mac_reg_read64(drv, XGMAC_RX_GOODOCTETS);
	hw_stats->rx_total_pkts = hns_mac_reg_read64(drv, XGMAC_RX_TOTAL_PKTS);
	hw_stats->rx_total_bytes
		= hns_mac_reg_read64(drv, XGMAC_RX_TOTALOCTETS);
	hw_stats->rx_uc_pkts = hns_mac_reg_read64(drv, XGMAC_RX_UNICASTPKTS);
	hw_stats->rx_mc_pkts = hns_mac_reg_read64(drv, XGMAC_RX_MULTICASTPKTS);
	hw_stats->rx_bc_pkts = hns_mac_reg_read64(drv, XGMAC_RX_BROADCASTPKTS);
	hw_stats->rx_pfc_tc0 = hns_mac_reg_read64(drv, XGMAC_RX_PRI0PAUSEPKTS);
	hw_stats->rx_pfc_tc1 = hns_mac_reg_read64(drv, XGMAC_RX_PRI1PAUSEPKTS);
	hw_stats->rx_pfc_tc2 = hns_mac_reg_read64(drv, XGMAC_RX_PRI2PAUSEPKTS);
	hw_stats->rx_pfc_tc3 = hns_mac_reg_read64(drv, XGMAC_RX_PRI3PAUSEPKTS);
	hw_stats->rx_pfc_tc4 = hns_mac_reg_read64(drv, XGMAC_RX_PRI4PAUSEPKTS);
	hw_stats->rx_pfc_tc5 = hns_mac_reg_read64(drv, XGMAC_RX_PRI5PAUSEPKTS);
	hw_stats->rx_pfc_tc6 = hns_mac_reg_read64(drv, XGMAC_RX_PRI6PAUSEPKTS);
	hw_stats->rx_pfc_tc7 = hns_mac_reg_read64(drv, XGMAC_RX_PRI7PAUSEPKTS);

	hw_stats->rx_unknown_ctrl
		= hns_mac_reg_read64(drv, XGMAC_RX_MACCTRLPKTS);
	hw_stats->tx_good_to_sw
		= hns_mac_reg_read64(drv, XGMAC_TX_SENDAPPGOODPKTS);
	hw_stats->tx_bad_to_sw
		= hns_mac_reg_read64(drv, XGMAC_TX_SENDAPPBADPKTS);
	hw_stats->rx_1731_pkts = hns_mac_reg_read64(drv, XGMAC_RX_1731PKTS);
	hw_stats->rx_symbol_err
		= hns_mac_reg_read64(drv, XGMAC_RX_SYMBOLERRPKTS);
	hw_stats->rx_fcs_err = hns_mac_reg_read64(drv, XGMAC_RX_FCSERRPKTS);
}

static void hns_xgmac_clean_stats(void *mac_drv)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	dsaf_write_dev(drv, XGMAC_MAC_MIB_CONTROL_REG, 1);
}

/**
 *hns_xgmac_free - free xgmac driver
 *@mac_drv: mac driver
 */
static void hns_xgmac_free(void *mac_drv)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;
	struct dsaf_device *dsaf_dev
		= (struct dsaf_device *)dev_get_drvdata(drv->dev);

	u32 mac_id = drv->mac_id;

	hns_dsaf_xge_srst_by_port(dsaf_dev, mac_id, 0);

}

/**
 *hns_xgmac_get_info - get xgmac information
 *@mac_drv: mac driver
 *@mac_info:mac information
 */
static void hns_xgmac_get_info(void *mac_drv, struct mac_info *mac_info)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;
	u32 pause_time, pause_ctrl, port_mode, ctrl_val;

	ctrl_val = dsaf_read_dev(drv, XGMAC_MAC_CONTROL_REG);
	mac_info->pad_and_crc_en = dsaf_get_bit(ctrl_val, XGMAC_CTL_TX_PAD_B);
	mac_info->auto_neg = 0;

	pause_time = dsaf_read_dev(drv, XGMAC_MAC_PAUSE_TIME_REG);
	mac_info->tx_pause_time = pause_time;

	port_mode = dsaf_read_dev(drv, XGMAC_PORT_MODE_REG);
	mac_info->port_en = dsaf_get_field(port_mode, XGMAC_PORT_MODE_TX_M,
					   XGMAC_PORT_MODE_TX_S) &&
				dsaf_get_field(port_mode, XGMAC_PORT_MODE_RX_M,
					       XGMAC_PORT_MODE_RX_S);
	mac_info->duplex = 1;
	mac_info->speed = MAC_SPEED_10000;

	pause_ctrl = dsaf_read_dev(drv, XGMAC_MAC_PAUSE_CTRL_REG);
	mac_info->rx_pause_en = dsaf_get_bit(pause_ctrl, XGMAC_PAUSE_CTL_RX_B);
	mac_info->tx_pause_en = dsaf_get_bit(pause_ctrl, XGMAC_PAUSE_CTL_TX_B);
}

/**
 *hns_xgmac_get_pausefrm_cfg - get xgmac pause param
 *@mac_drv: mac driver
 *@rx_en:xgmac rx pause enable
 *@tx_en:xgmac tx pause enable
 */
static void hns_xgmac_get_pausefrm_cfg(void *mac_drv, u32 *rx_en, u32 *tx_en)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;
	u32 pause_ctrl;

	pause_ctrl = dsaf_read_dev(drv, XGMAC_MAC_PAUSE_CTRL_REG);
	*rx_en = dsaf_get_bit(pause_ctrl, XGMAC_PAUSE_CTL_RX_B);
	*tx_en = dsaf_get_bit(pause_ctrl, XGMAC_PAUSE_CTL_TX_B);
}

/**
 *hns_xgmac_get_link_status - get xgmac link status
 *@mac_drv: mac driver
 *@link_stat: xgmac link stat
 */
static void hns_xgmac_get_link_status(void *mac_drv, u32 *link_stat)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	*link_stat = dsaf_read_dev(drv, XGMAC_LINK_STATUS_REG);
}

/**
 *hns_xgmac_get_regs - dump xgmac regs
 *@mac_drv: mac driver
 *@cmd:ethtool cmd
 *@data:data for value of regs
 */
static void hns_xgmac_get_regs(void *mac_drv, void *data)
{
	u32 i = 0;
	struct mac_driver *drv = (struct mac_driver *)mac_drv;
	u32 *regs = data;
	u64 qtmp;

	/* base config registers */
	regs[0] = dsaf_read_dev(drv, XGMAC_INT_STATUS_REG);
	regs[1] = dsaf_read_dev(drv, XGMAC_INT_ENABLE_REG);
	regs[2] = dsaf_read_dev(drv, XGMAC_INT_SET_REG);
	regs[3] = dsaf_read_dev(drv, XGMAC_IERR_U_INFO_REG);
	regs[4] = dsaf_read_dev(drv, XGMAC_OVF_INFO_REG);
	regs[5] = dsaf_read_dev(drv, XGMAC_OVF_CNT_REG);
	regs[6] = dsaf_read_dev(drv, XGMAC_PORT_MODE_REG);
	regs[7] = dsaf_read_dev(drv, XGMAC_CLK_ENABLE_REG);
	regs[8] = dsaf_read_dev(drv, XGMAC_RESET_REG);
	regs[9] = dsaf_read_dev(drv, XGMAC_LINK_CONTROL_REG);
	regs[10] = dsaf_read_dev(drv, XGMAC_LINK_STATUS_REG);

	regs[11] = dsaf_read_dev(drv, XGMAC_SPARE_REG);
	regs[12] = dsaf_read_dev(drv, XGMAC_SPARE_CNT_REG);
	regs[13] = dsaf_read_dev(drv, XGMAC_MAC_ENABLE_REG);
	regs[14] = dsaf_read_dev(drv, XGMAC_MAC_CONTROL_REG);
	regs[15] = dsaf_read_dev(drv, XGMAC_MAC_IPG_REG);
	regs[16] = dsaf_read_dev(drv, XGMAC_MAC_MSG_CRC_EN_REG);
	regs[17] = dsaf_read_dev(drv, XGMAC_MAC_MSG_IMG_REG);
	regs[18] = dsaf_read_dev(drv, XGMAC_MAC_MSG_FC_CFG_REG);
	regs[19] = dsaf_read_dev(drv, XGMAC_MAC_MSG_TC_CFG_REG);
	regs[20] = dsaf_read_dev(drv, XGMAC_MAC_PAD_SIZE_REG);
	regs[21] = dsaf_read_dev(drv, XGMAC_MAC_MIN_PKT_SIZE_REG);
	regs[22] = dsaf_read_dev(drv, XGMAC_MAC_MAX_PKT_SIZE_REG);
	regs[23] = dsaf_read_dev(drv, XGMAC_MAC_PAUSE_CTRL_REG);
	regs[24] = dsaf_read_dev(drv, XGMAC_MAC_PAUSE_TIME_REG);
	regs[25] = dsaf_read_dev(drv, XGMAC_MAC_PAUSE_GAP_REG);
	regs[26] = dsaf_read_dev(drv, XGMAC_MAC_PAUSE_LOCAL_MAC_H_REG);
	regs[27] = dsaf_read_dev(drv, XGMAC_MAC_PAUSE_LOCAL_MAC_L_REG);
	regs[28] = dsaf_read_dev(drv, XGMAC_MAC_PAUSE_PEER_MAC_H_REG);
	regs[29] = dsaf_read_dev(drv, XGMAC_MAC_PAUSE_PEER_MAC_L_REG);
	regs[30] = dsaf_read_dev(drv, XGMAC_MAC_PFC_PRI_EN_REG);
	regs[31] = dsaf_read_dev(drv, XGMAC_MAC_1588_CTRL_REG);
	regs[32] = dsaf_read_dev(drv, XGMAC_MAC_1588_TX_PORT_DLY_REG);
	regs[33] = dsaf_read_dev(drv, XGMAC_MAC_1588_RX_PORT_DLY_REG);
	regs[34] = dsaf_read_dev(drv, XGMAC_MAC_1588_ASYM_DLY_REG);
	regs[35] = dsaf_read_dev(drv, XGMAC_MAC_1588_ADJUST_CFG_REG);

	regs[36] = dsaf_read_dev(drv, XGMAC_MAC_Y1731_ETH_TYPE_REG);
	regs[37] = dsaf_read_dev(drv, XGMAC_MAC_MIB_CONTROL_REG);
	regs[38] = dsaf_read_dev(drv, XGMAC_MAC_WAN_RATE_ADJUST_REG);
	regs[39] = dsaf_read_dev(drv, XGMAC_MAC_TX_ERR_MARK_REG);
	regs[40] = dsaf_read_dev(drv, XGMAC_MAC_TX_LF_RF_CONTROL_REG);
	regs[41] = dsaf_read_dev(drv, XGMAC_MAC_RX_LF_RF_STATUS_REG);
	regs[42] = dsaf_read_dev(drv, XGMAC_MAC_TX_RUNT_PKT_CNT_REG);
	regs[43] = dsaf_read_dev(drv, XGMAC_MAC_RX_RUNT_PKT_CNT_REG);
	regs[44] = dsaf_read_dev(drv, XGMAC_MAC_RX_PREAM_ERR_PKT_CNT_REG);
	regs[45] = dsaf_read_dev(drv, XGMAC_MAC_TX_LF_RF_TERM_PKT_CNT_REG);
	regs[46] = dsaf_read_dev(drv, XGMAC_MAC_TX_SN_MISMATCH_PKT_CNT_REG);
	regs[47] = dsaf_read_dev(drv, XGMAC_MAC_RX_ERR_MSG_CNT_REG);
	regs[48] = dsaf_read_dev(drv, XGMAC_MAC_RX_ERR_EFD_CNT_REG);
	regs[49] = dsaf_read_dev(drv, XGMAC_MAC_ERR_INFO_REG);
	regs[50] = dsaf_read_dev(drv, XGMAC_MAC_DBG_INFO_REG);

	regs[51] = dsaf_read_dev(drv, XGMAC_PCS_BASER_SYNC_THD_REG);
	regs[52] = dsaf_read_dev(drv, XGMAC_PCS_STATUS1_REG);
	regs[53] = dsaf_read_dev(drv, XGMAC_PCS_BASER_STATUS1_REG);
	regs[54] = dsaf_read_dev(drv, XGMAC_PCS_BASER_STATUS2_REG);
	regs[55] = dsaf_read_dev(drv, XGMAC_PCS_BASER_SEEDA_0_REG);
	regs[56] = dsaf_read_dev(drv, XGMAC_PCS_BASER_SEEDA_1_REG);
	regs[57] = dsaf_read_dev(drv, XGMAC_PCS_BASER_SEEDB_0_REG);
	regs[58] = dsaf_read_dev(drv, XGMAC_PCS_BASER_SEEDB_1_REG);
	regs[59] = dsaf_read_dev(drv, XGMAC_PCS_BASER_TEST_CONTROL_REG);
	regs[60] = dsaf_read_dev(drv, XGMAC_PCS_BASER_TEST_ERR_CNT_REG);
	regs[61] = dsaf_read_dev(drv, XGMAC_PCS_DBG_INFO_REG);
	regs[62] = dsaf_read_dev(drv, XGMAC_PCS_DBG_INFO1_REG);
	regs[63] = dsaf_read_dev(drv, XGMAC_PCS_DBG_INFO2_REG);
	regs[64] = dsaf_read_dev(drv, XGMAC_PCS_DBG_INFO3_REG);

	regs[65] = dsaf_read_dev(drv, XGMAC_PMA_ENABLE_REG);
	regs[66] = dsaf_read_dev(drv, XGMAC_PMA_CONTROL_REG);
	regs[67] = dsaf_read_dev(drv, XGMAC_PMA_SIGNAL_STATUS_REG);
	regs[68] = dsaf_read_dev(drv, XGMAC_PMA_DBG_INFO_REG);
	regs[69] = dsaf_read_dev(drv, XGMAC_PMA_FEC_ABILITY_REG);
	regs[70] = dsaf_read_dev(drv, XGMAC_PMA_FEC_CONTROL_REG);
	regs[71] = dsaf_read_dev(drv, XGMAC_PMA_FEC_CORR_BLOCK_CNT__REG);
	regs[72] = dsaf_read_dev(drv, XGMAC_PMA_FEC_UNCORR_BLOCK_CNT__REG);

	/* status registers */
#define hns_xgmac_cpy_q(p, q) \
	do {\
		*(p) = (u32)(q);\
		*((p) + 1) = (u32)((q) >> 32);\
	} while (0)

	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PKTS_FRAGMENT);
	hns_xgmac_cpy_q(&regs[73], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PKTS_UNDERSIZE);
	hns_xgmac_cpy_q(&regs[75], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PKTS_UNDERMIN);
	hns_xgmac_cpy_q(&regs[77], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PKTS_64OCTETS);
	hns_xgmac_cpy_q(&regs[79], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PKTS_65TO127OCTETS);
	hns_xgmac_cpy_q(&regs[81], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PKTS_128TO255OCTETS);
	hns_xgmac_cpy_q(&regs[83], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PKTS_256TO511OCTETS);
	hns_xgmac_cpy_q(&regs[85], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PKTS_512TO1023OCTETS);
	hns_xgmac_cpy_q(&regs[87], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PKTS_1024TO1518OCTETS);
	hns_xgmac_cpy_q(&regs[89], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PKTS_1519TOMAXOCTETS);
	hns_xgmac_cpy_q(&regs[91], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PKTS_1519TOMAXOCTETSOK);
	hns_xgmac_cpy_q(&regs[93], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PKTS_OVERSIZE);
	hns_xgmac_cpy_q(&regs[95], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PKTS_JABBER);
	hns_xgmac_cpy_q(&regs[97], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_GOODPKTS);
	hns_xgmac_cpy_q(&regs[99], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_GOODOCTETS);
	hns_xgmac_cpy_q(&regs[101], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_TOTAL_PKTS);
	hns_xgmac_cpy_q(&regs[103], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_TOTALOCTETS);
	hns_xgmac_cpy_q(&regs[105], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_UNICASTPKTS);
	hns_xgmac_cpy_q(&regs[107], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_MULTICASTPKTS);
	hns_xgmac_cpy_q(&regs[109], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_BROADCASTPKTS);
	hns_xgmac_cpy_q(&regs[111], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PRI0PAUSEPKTS);
	hns_xgmac_cpy_q(&regs[113], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PRI1PAUSEPKTS);
	hns_xgmac_cpy_q(&regs[115], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PRI2PAUSEPKTS);
	hns_xgmac_cpy_q(&regs[117], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PRI3PAUSEPKTS);
	hns_xgmac_cpy_q(&regs[119], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PRI4PAUSEPKTS);
	hns_xgmac_cpy_q(&regs[121], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PRI5PAUSEPKTS);
	hns_xgmac_cpy_q(&regs[123], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PRI6PAUSEPKTS);
	hns_xgmac_cpy_q(&regs[125], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_PRI7PAUSEPKTS);
	hns_xgmac_cpy_q(&regs[127], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_MACCTRLPKTS);
	hns_xgmac_cpy_q(&regs[129], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_1731PKTS);
	hns_xgmac_cpy_q(&regs[131], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_1588PKTS);
	hns_xgmac_cpy_q(&regs[133], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_FROMAPPGOODPKTS);
	hns_xgmac_cpy_q(&regs[135], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_FROMAPPBADPKTS);
	hns_xgmac_cpy_q(&regs[137], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_ERRALLPKTS);
	hns_xgmac_cpy_q(&regs[139], qtmp);

	/* RX */
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PKTS_FRAGMENT);
	hns_xgmac_cpy_q(&regs[141], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PKTSUNDERSIZE);
	hns_xgmac_cpy_q(&regs[143], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PKTS_UNDERMIN);
	hns_xgmac_cpy_q(&regs[145], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PKTS_64OCTETS);
	hns_xgmac_cpy_q(&regs[147], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PKTS_65TO127OCTETS);
	hns_xgmac_cpy_q(&regs[149], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PKTS_128TO255OCTETS);
	hns_xgmac_cpy_q(&regs[151], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PKTS_256TO511OCTETS);
	hns_xgmac_cpy_q(&regs[153], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PKTS_512TO1023OCTETS);
	hns_xgmac_cpy_q(&regs[155], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PKTS_1024TO1518OCTETS);
	hns_xgmac_cpy_q(&regs[157], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PKTS_1519TOMAXOCTETS);
	hns_xgmac_cpy_q(&regs[159], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PKTS_1519TOMAXOCTETSOK);
	hns_xgmac_cpy_q(&regs[161], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PKTS_OVERSIZE);
	hns_xgmac_cpy_q(&regs[163], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PKTS_JABBER);
	hns_xgmac_cpy_q(&regs[165], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_GOODPKTS);
	hns_xgmac_cpy_q(&regs[167], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_GOODOCTETS);
	hns_xgmac_cpy_q(&regs[169], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_TOTAL_PKTS);
	hns_xgmac_cpy_q(&regs[171], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_TOTALOCTETS);
	hns_xgmac_cpy_q(&regs[173], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_UNICASTPKTS);
	hns_xgmac_cpy_q(&regs[175], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_MULTICASTPKTS);
	hns_xgmac_cpy_q(&regs[177], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_BROADCASTPKTS);
	hns_xgmac_cpy_q(&regs[179], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PRI0PAUSEPKTS);
	hns_xgmac_cpy_q(&regs[181], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PRI1PAUSEPKTS);
	hns_xgmac_cpy_q(&regs[183], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PRI2PAUSEPKTS);
	hns_xgmac_cpy_q(&regs[185], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PRI3PAUSEPKTS);
	hns_xgmac_cpy_q(&regs[187], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PRI4PAUSEPKTS);
	hns_xgmac_cpy_q(&regs[189], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PRI5PAUSEPKTS);
	hns_xgmac_cpy_q(&regs[191], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PRI6PAUSEPKTS);
	hns_xgmac_cpy_q(&regs[193], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_PRI7PAUSEPKTS);
	hns_xgmac_cpy_q(&regs[195], qtmp);

	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_MACCTRLPKTS);
	hns_xgmac_cpy_q(&regs[197], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_SENDAPPGOODPKTS);
	hns_xgmac_cpy_q(&regs[199], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_TX_SENDAPPBADPKTS);
	hns_xgmac_cpy_q(&regs[201], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_1731PKTS);
	hns_xgmac_cpy_q(&regs[203], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_SYMBOLERRPKTS);
	hns_xgmac_cpy_q(&regs[205], qtmp);
	qtmp = hns_mac_reg_read64(drv, XGMAC_RX_FCSERRPKTS);
	hns_xgmac_cpy_q(&regs[207], qtmp);

	/* mark end of mac regs */
	for (i = 208; i < 214; i++)
		regs[i] = 0xaaaaaaaa;
}

/**
 *hns_xgmac_get_ethtool_stats - get xgmac statistic
 *@mac_drv: mac driver
 *@cmd:ethtool cmd
 *@data:data for value of stats regs
 */
static void hns_xgmac_get_ethtool_stats(void *mac_drv,
					struct ethtool_stats *cmd, u64 *data)
{
	u64 *p = data;
	struct mac_driver *drv = (struct mac_driver *)mac_drv;
	struct mac_hw_stats *hw_stats = NULL;

	hw_stats = &drv->mac_cb->hw_stats;

	hns_xgmac_update_stats(drv);

	p[0] = hw_stats->tx_fragment_err;
	p[1] = hw_stats->tx_undersize;
	p[2] = hw_stats->tx_under_min_pkts;
	p[3] = hw_stats->tx_64bytes;
	p[4] = hw_stats->tx_65to127;
	p[5] = hw_stats->tx_128to255;
	p[6] = hw_stats->tx_256to511;
	p[7] = hw_stats->tx_512to1023;
	p[8] = hw_stats->tx_1024to1518;
	p[9] = hw_stats->tx_1519tomax;

	p[10] = hw_stats->tx_1519tomax_good;
	p[11] = hw_stats->tx_oversize;
	p[12] = hw_stats->tx_jabber_err;
	p[13] = hw_stats->tx_good_pkts;
	p[14] = hw_stats->tx_good_bytes;
	p[15] = hw_stats->tx_total_pkts;
	p[16] = hw_stats->tx_total_bytes;
	p[17] = hw_stats->tx_uc_pkts;
	p[18] = hw_stats->tx_mc_pkts;
	p[19] = hw_stats->tx_bc_pkts;

	p[20] = hw_stats->tx_pfc_tc0;
	p[21] = hw_stats->tx_pfc_tc1;
	p[22] = hw_stats->tx_pfc_tc2;
	p[23] = hw_stats->tx_pfc_tc3;
	p[24] = hw_stats->tx_pfc_tc4;
	p[25] = hw_stats->tx_pfc_tc5;
	p[26] = hw_stats->tx_pfc_tc6;
	p[27] = hw_stats->tx_pfc_tc7;
	p[28] = hw_stats->tx_ctrl;
	p[29] = hw_stats->tx_1731_pkts;

	p[30] = hw_stats->tx_1588_pkts;
	p[31] = hw_stats->rx_good_from_sw;
	p[32] = hw_stats->rx_bad_from_sw;
	p[33] = hw_stats->tx_bad_pkts;

	p[34] = hw_stats->rx_fragment_err;
	p[35] = hw_stats->rx_undersize;
	p[36] = hw_stats->rx_under_min;
	p[37] = hw_stats->rx_64bytes;
	p[38] = hw_stats->rx_65to127;
	p[39] = hw_stats->rx_128to255;

	p[40] = hw_stats->rx_256to511;
	p[41] = hw_stats->rx_512to1023;
	p[42] = hw_stats->rx_1024to1518;
	p[43] = hw_stats->rx_1519tomax;
	p[44] = hw_stats->rx_1519tomax_good;
	p[45] = hw_stats->rx_oversize;
	p[46] = hw_stats->rx_jabber_err;
	p[47] = hw_stats->rx_good_pkts;
	p[48] = hw_stats->rx_good_bytes;
	p[49] = hw_stats->rx_total_pkts;

	p[50] = hw_stats->rx_total_bytes;
	p[51] = hw_stats->rx_uc_pkts;
	p[52] = hw_stats->rx_mc_pkts;
	p[53] = hw_stats->rx_bc_pkts;
	p[54] = hw_stats->rx_pfc_tc0;
	p[55] = hw_stats->rx_pfc_tc1;
	p[56] = hw_stats->rx_pfc_tc2;
	p[57] = hw_stats->rx_pfc_tc3;
	p[58] = hw_stats->rx_pfc_tc4;
	p[59] = hw_stats->rx_pfc_tc5;

	p[60] = hw_stats->rx_pfc_tc6;
	p[61] = hw_stats->rx_pfc_tc7;
	p[62] = hw_stats->rx_unknown_ctrl;
	p[63] = hw_stats->tx_good_to_sw;
	p[64] = hw_stats->tx_bad_to_sw;
	p[65] = hw_stats->rx_1731_pkts;
	p[66] = hw_stats->rx_symbol_err;
	p[67] = hw_stats->rx_fcs_err;
}

/**
 *hns_xgmac_get_strings - get xgmac strings name
 *@mac_drv: mac driver
 *@stringset: type of values in data,0-selftest,1-ststistics,5-dump regs
 *@data:data for value of string name
 */
static void hns_xgmac_get_strings(void *mac_drv, u32 stringset, u8 *data)
{
	char *buff = (char *)data;

	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_bad_pkts_minto64");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_good_pkts_minto64");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_total_pkts_minto64");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_pkts_64");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_pkts_65TO127");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_pkts_128TO255");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_pkts_256TO511");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_pkts_512TO1023");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_pkts_1024TO1518");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_pkts_1519TOmax");
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_good_pkts_1519TOmax");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_good_pkts_untralmax");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_bad_pkts_untralmax");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_good_pkts_ALL");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_good_byte_ALL");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_total_pkt");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_total_byt");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_UC_pkt");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_MC_pkt");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_BC_pkt");
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_pause_frame_num");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_pfc_per_1pause_framer");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_pfc_per_2pause_framer");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_pfc_per_3pause_framer");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_pfc_per_4pause_framer");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_pfc_per_5pause_framer");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_pfc_per_6pause_framer");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_pfc_per_7pause_framer");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_MAC_CTROL_FRAME");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_1731_PKTS");
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_1588_PKTS");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_GOOD_PKT_FROM_DSAF");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_BAD_PKT_FROM_DSAF");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_BAD_PKT_64TOMAX");
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_NOT_WELL_PKT");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_GOOD_WELL_PKT");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_TOTAL_PKT");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_PKT_64");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_PKT_65TO128");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_PKT_128TO256");
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_PKT_256TO512");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_PKT_512TO1024");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_PKT_1024TO1518");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_PKT_1519TOMAX");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_GOOD_PKT_1519TOMAX");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_GOOD_PKT_UNTRAMAX");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_BAD_PKT_UNTRAMAX");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_GOOD_PKT");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_GOOD_BYT");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_PKT");
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_BYT");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_UC_PKT");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_MC_PKT");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_BC_PKT");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_PAUSE_FRAME_NUM");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_PFC_PER_1PAUSE_FRAME");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_PFC_PER_2PAUSE_FRAME");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_PFC_PER_3PAUSE_FRAME");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_PFC_PER_4PAUSE_FRAME");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_PFC_PER_5PAUSE_FRAME");
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_PFC_PER_6PAUSE_FRAME");
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_PFC_PER_7PAUSE_FRAME");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_MAC_CONTROL");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_GOOD_PKT_TODSAF");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_TX_BAD_PKT_TODSAF");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_1731_PKT");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_SYMBOL_ERR_PKT");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "XGMAC_RX_FCS_PKT");
}

/**
 *hns_xgmac_get_sset_count - get xgmac string set count
 *@stringset: type of values in data
 *return xgmac string set count
 */
static int hns_xgmac_get_sset_count(int stringset)
{
	if (stringset == ETH_SS_STATS)
		return ETH_XGMAC_MIB_NUM;

	return 0;
}

/**
 *hns_xgmac_get_regs_count - get xgmac regs count
 *return xgmac regs count
 */
static int hns_xgmac_get_regs_count(void)
{
	return ETH_XGMAC_DUMP_NUM;
}

void *hns_xgmac_config(struct hns_mac_cb *mac_cb, struct mac_params *mac_param)
{
	struct mac_driver *mac_drv;

	mac_drv = devm_kzalloc(mac_cb->dev, sizeof(*mac_drv), GFP_KERNEL);
	if (!mac_drv)
		return NULL;

	mac_drv->mac_init = hns_xgmac_init;
	mac_drv->mac_enable = hns_xgmac_enable;
	mac_drv->mac_disable = hns_xgmac_disable;

	mac_drv->dsaf_id = mac_param->dsaf_id;
	mac_drv->mac_id = mac_param->mac_id;
	mac_drv->mac_mode = mac_param->mac_mode;
	mac_drv->io_base = mac_param->vaddr;
	mac_drv->dev = mac_param->dev;
	mac_drv->mac_cb = mac_cb;

	mac_drv->mac_reset = NULL;
	mac_drv->set_mac_addr = NULL;
	mac_drv->set_an_mode = NULL;
	mac_drv->config_loopback = NULL;
	mac_drv->config_pad_and_crc = hns_xgmac_config_pad_and_crc;
	mac_drv->config_half_duplex = NULL;
	mac_drv->set_rx_ignore_pause_frames =
		hns_xgmac_set_rx_ignore_pause_frames;
	mac_drv->mac_get_id = hns_xgmac_get_id;
	mac_drv->mac_free = hns_xgmac_free;
	mac_drv->adjust_link = NULL;
	mac_drv->set_tx_auto_pause_frames = hns_xgmac_set_tx_auto_pause_frames;
	mac_drv->config_max_frame_length = hns_xgmac_config_max_frame_length;
	mac_drv->mac_pausefrm_cfg = hns_xgmac_pausefrm_cfg;
	mac_drv->autoneg_stat = NULL;
	mac_drv->get_info = hns_xgmac_get_info;
	mac_drv->get_pause_enable = hns_xgmac_get_pausefrm_cfg;
	mac_drv->get_link_status = hns_xgmac_get_link_status;
	mac_drv->get_regs = hns_xgmac_get_regs;
	mac_drv->get_ethtool_stats = hns_xgmac_get_ethtool_stats;
	mac_drv->get_sset_count = hns_xgmac_get_sset_count;
	mac_drv->get_regs_count = hns_xgmac_get_regs_count;
	mac_drv->get_strings = hns_xgmac_get_strings;
	mac_drv->update_stats = hns_xgmac_update_stats;
	mac_drv->clean_stats = hns_xgmac_clean_stats;

	return (void *)mac_drv;
}
