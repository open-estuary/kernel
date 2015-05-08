/*
 * Copyright (C), 2008-2013, Hisilicon Technologies Co., Ltd.
 * declare of global register of chip
 */
 
#ifndef _HIGGS_PORT_REG_H
#define _HIGGS_PORT_REG_H

/* port_reg base address */
#define HISAS30HV100_PORT_REG_BASE                (0x800)
#define HISAS30HV100_PORT_REG_NUM                 (0x400)

/*
 * HiSAS30HV100 port_reg register define
 */
#define HISAS30HV100_PORT_REG_PHY_CFG_REG         (HISAS30HV100_PORT_REG_BASE + 0x0)	/* Phy config register */
#define HISAS30HV100_PORT_REG_HARD_PHY_LINK_RATE_REG  (HISAS30HV100_PORT_REG_BASE + 0x4)	/* PHY status inquire register*/
#define HISAS30HV100_PORT_REG_PHY_STATE_REG       (HISAS30HV100_PORT_REG_BASE + 0x8)	/* PHY status inquire register*/
#define HISAS30HV100_PORT_REG_PROG_PHY_LINK_RATE_REG  (HISAS30HV100_PORT_REG_BASE + 0xC)	/* programmable physics rate */
#define HISAS30HV100_PORT_REG_PHY_SUMBER_TIME_REG  (HISAS30HV100_PORT_REG_BASE + 0x10)	/* PHY sumber status wake up time */
#define HISAS30HV100_PORT_REG_PHY_CTRL_REG        (HISAS30HV100_PORT_REG_BASE + 0x14)	/* PHY controller register */
#define HISAS30HV100_PORT_REG_PHY_COMINI_TIME_REG  (HISAS30HV100_PORT_REG_BASE + 0x18)	/* COMINIT config time */
#define HISAS30HV100_PORT_REG_PHY_COMWAKE_TIME_REG  (HISAS30HV100_PORT_REG_BASE + 0x1C)	/* COMIWAKE config time */
#define HISAS30HV100_PORT_REG_PHY_COMSAS_TIME_REG  (HISAS30HV100_PORT_REG_BASE + 0x20)	/* COMSAS config time */
#define HISAS30HV100_PORT_REG_PHY_TRAIN_TIME_REG  (HISAS30HV100_PORT_REG_BASE + 0x24)	/* PHYCTRL train time config register */
#define HISAS30HV100_PORT_REG_PHY_RATE_CHG_REG    (HISAS30HV100_PORT_REG_BASE + 0x28)	/* PHYCTRL rate change register */
#define HISAS30HV100_PORT_REG_PHY_TIMER_SET_REG   (HISAS30HV100_PORT_REG_BASE + 0x2C)	/* PHYCTRL timer settint register */
#define HISAS30HV100_PORT_REG_PHY_RATE_NEGO_REG   (HISAS30HV100_PORT_REG_BASE + 0x30)	/* PHYCTRL speed negotiation and timer control register */
#define HISAS30HV100_PORT_REG_PHY_BIST_CHK_TIME_REG  (HISAS30HV100_PORT_REG_BASE + 0x34)	/* PHYCTRL BIST test time */
#define HISAS30HV100_PORT_REG_PHY_DWS_RESET_TIME_REG  (HISAS30HV100_PORT_REG_BASE + 0x38)	/* PHYCTRL DWS reset time register */
#define HISAS30HV100_PORT_REG_PHY_BIST_CTRL_REG   (HISAS30HV100_PORT_REG_BASE + 0x3C)	/* PHYCTRL BIST control register */
#define HISAS30HV100_PORT_REG_PHYCTRL_STATUS_REG  (HISAS30HV100_PORT_REG_BASE + 0x40)	/* PHYCTRL status machine register */
#define HISAS30HV100_PORT_REG_PHY_PCN_REG         (HISAS30HV100_PORT_REG_BASE + 0x44)	/* PHYCTRL config register */
#define HISAS30HV100_PORT_REG_PHY_SATA_CFG_REG    (HISAS30HV100_PORT_REG_BASE + 0x48)	/* SATA config register */
#define HISAS30HV100_PORT_REG_PHY_SATA_NEGO_TIME_REG  (HISAS30HV100_PORT_REG_BASE + 0x4C)	/* SATA config register */
#define HISAS30HV100_PORT_REG_PHY_EINJ_EN_REG     (HISAS30HV100_PORT_REG_BASE + 0x50)
#define HISAS30HV100_PORT_REG_PHY_EINJ_ERRCNT_REG  (HISAS30HV100_PORT_REG_BASE + 0x54)	/* PHY error inject config register */
#define HISAS30HV100_PORT_REG_PHY_EINJ_SPACE_H_REG  (HISAS30HV100_PORT_REG_BASE + 0x58)	/* PHY error inject config register */
#define HISAS30HV100_PORT_REG_PHY_EINJ_SPACE_L_REG  (HISAS30HV100_PORT_REG_BASE + 0x5C)	/* PHY error inject config register */
#define HISAS30HV100_PORT_REG_PHY_FFE_SET_REG     (HISAS30HV100_PORT_REG_BASE + 0x60)	/* PHY FFE config register */
#define HISAS30HV100_PORT_REG_PHY_DFE_SET_REG     (HISAS30HV100_PORT_REG_BASE + 0x64)	/* PHY DFE config register */
#define HISAS30HV100_PORT_REG_PHY_BIST_CODE_REG   (HISAS30HV100_PORT_REG_BASE + 0x68)	/* PHY BIST CODE config register */
#define HISAS30HV100_PORT_REG_RXOP_HW_OPREJ_REG   (HISAS30HV100_PORT_REG_BASE + 0x70)	/* OPAF hardware OPREJ register */
#define HISAS30HV100_PORT_REG_SSP_DEBUG_STATUS_REG  (HISAS30HV100_PORT_REG_BASE + 0x74)	/* SSP DEBUG register */
#define HISAS30HV100_PORT_REG_SL_DEBUG_STATUS0_REG  (HISAS30HV100_PORT_REG_BASE + 0x78)	/* SL DEBUG register */
#define HISAS30HV100_PORT_REG_SL_FSM_STATUS1_REG  (HISAS30HV100_PORT_REG_BASE + 0x7C)	/* SL status machine register */
#define HISAS30HV100_PORT_REG_SL_PRIM_STATUS_REG  (HISAS30HV100_PORT_REG_BASE + 0x80)	/* Primitive send status register */
#define HISAS30HV100_PORT_REG_SL_CFG_REG          (HISAS30HV100_PORT_REG_BASE + 0x84)	/* link level config register */
#define HISAS30HV100_PORT_REG_IDLE_DW_CFG_REG     (HISAS30HV100_PORT_REG_BASE + 0x88)	/* free Dword config register */
#define HISAS30HV100_PORT_REG_SL_TOUT_CFG_REG     (HISAS30HV100_PORT_REG_BASE + 0x8C)	/* Link level timeout register */
#define HISAS30HV100_PORT_REG_AIP_LIMIT_REG       (HISAS30HV100_PORT_REG_BASE + 0x90)	/* AIP timeout limit register */
#define HISAS30HV100_PORT_REG_SL_CONTROL_REG      (HISAS30HV100_PORT_REG_BASE + 0x94)	/* Link level control register */
#define HISAS30HV100_PORT_REG_RX_PRIMS_STATUS_REG  (HISAS30HV100_PORT_REG_BASE + 0x98)	/* receiving Primitive status register */
#define HISAS30HV100_PORT_REG_TX_ID_DWORD0_REG    (HISAS30HV100_PORT_REG_BASE + 0x9C)	/* sending IDENTIFY address frame dword0 */
#define HISAS30HV100_PORT_REG_TX_ID_DWORD1_REG    (HISAS30HV100_PORT_REG_BASE + 0xA0)	/* sending IDENTIFY address frame dword1 */
#define HISAS30HV100_PORT_REG_TX_ID_DWORD2_REG    (HISAS30HV100_PORT_REG_BASE + 0xA4)	/* sending IDENTIFY address frame dword2 */
#define HISAS30HV100_PORT_REG_TX_ID_DWORD3_REG    (HISAS30HV100_PORT_REG_BASE + 0xA8)	/* sending IDENTIFY address frame dword3 */
#define HISAS30HV100_PORT_REG_TX_ID_DWORD4_REG    (HISAS30HV100_PORT_REG_BASE + 0xAC)	/* sending IDENTIFY address frame dword4 */
#define HISAS30HV100_PORT_REG_TX_ID_DWORD5_REG    (HISAS30HV100_PORT_REG_BASE + 0xB0)	/* sending IDENTIFY address frame dword5 */
#define HISAS30HV100_PORT_REG_TX_ID_DWORD6_REG    (HISAS30HV100_PORT_REG_BASE + 0xB4)	/* sending IDENTIFY address frame dword6 */
#define HISAS30HV100_PORT_REG_TXID_AUTO_REG       (HISAS30HV100_PORT_REG_BASE + 0xB8)	/* auto-send IDENTIFY address frame register */
#define HISAS30HV100_PORT_REG_TX_RX_AF_STATUS_ENQ_REG  (HISAS30HV100_PORT_REG_BASE + 0xC0)	/* Tx/Rx address status enquery register */
#define HISAS30HV100_PORT_REG_RX_IDAF_DWORD0_REG  (HISAS30HV100_PORT_REG_BASE + 0xC4)	/* receive IDENTIFY address frame dword0 */
#define HISAS30HV100_PORT_REG_RX_IDAF_DWORD1_REG  (HISAS30HV100_PORT_REG_BASE + 0xC8)	/* receive IDENTIFY address frame dword1 */
#define HISAS30HV100_PORT_REG_RX_IDAF_DWORD2_REG  (HISAS30HV100_PORT_REG_BASE + 0xCC)	/* receive IDENTIFY address frame dword2 */
#define HISAS30HV100_PORT_REG_RX_IDAF_DWORD3_REG  (HISAS30HV100_PORT_REG_BASE + 0xD0)	/* receive IDENTIFY address frame dword3 */
#define HISAS30HV100_PORT_REG_RX_IDAF_DWORD4_REG  (HISAS30HV100_PORT_REG_BASE + 0xD4)	/* receive IDENTIFY address frame dword4 */
#define HISAS30HV100_PORT_REG_RX_IDAF_DWORD5_REG  (HISAS30HV100_PORT_REG_BASE + 0xD8)	/* receive IDENTIFY address frame dword5 */
#define HISAS30HV100_PORT_REG_RX_IDAF_DWORD6_REG  (HISAS30HV100_PORT_REG_BASE + 0xDC)	/* receive IDENTIFY address frame dword6 */
#define HISAS30HV100_PORT_REG_RXOP_CHECK_CFG_H_REG  (HISAS30HV100_PORT_REG_BASE + 0xFC)	/* receive OPAF hight 32 bit */
#define HISAS30HV100_PORT_REG_PW_MAG_REG          (HISAS30HV100_PORT_REG_BASE + 0x100)	/* power manager register */
#define HISAS30HV100_PORT_REG_RXOP_CHECK_CFG_L_REG  (HISAS30HV100_PORT_REG_BASE + 0x104)	/* receive OPAF lower 32 bit */
#define HISAS30HV100_PORT_REG_RXOP_ICT_RANGE_REG  (HISAS30HV100_PORT_REG_BASE + 0x108)	/* receive OPAF ICT range register */
#define HISAS30HV100_PORT_REG_STP_CON_CLOSE_REG   (HISAS30HV100_PORT_REG_BASE + 0x10C)	/* STP link close register */
#define HISAS30HV100_PORT_REG_PRIM_TOUT_CFG_REG   (HISAS30HV100_PORT_REG_BASE + 0x110)	/* Primitive timeout conifg register */
#define HISAS30HV100_PORT_REG_SSP_CONTROL_REG     (HISAS30HV100_PORT_REG_BASE + 0x114)	/* SSP link level control register */
#define HISAS30HV100_PORT_REG_ACK_NAK_BLC_REG     (HISAS30HV100_PORT_REG_BASE + 0x118)	/* ACK/NAK Balance inquire */
#define HISAS30HV100_PORT_REG_TXF_CRED_REG        (HISAS30HV100_PORT_REG_BASE + 0x11C)	/* send frame credit inquire register */
#define HISAS30HV100_PORT_REG_RXF_CRED_CONTROL_REG  (HISAS30HV100_PORT_REG_BASE + 0x120)	/*receive frame credit control register */
#define HISAS30HV100_PORT_REG_CRED_STATUS_ENQ_REG  (HISAS30HV100_PORT_REG_BASE + 0x124)	/* receive frame credit status enquiry register */
#define HISAS30HV100_PORT_REG_CON_CONTROL_REG     (HISAS30HV100_PORT_REG_BASE + 0x128)	/* link control register */
#define HISAS30HV100_PORT_REG_DONE_RECEVIED_TIME_REG  (HISAS30HV100_PORT_REG_BASE + 0x12C)	/* link colsed config register */
#define HISAS30HV100_PORT_REG_CON_CFG_DRIVER_REG  (HISAS30HV100_PORT_REG_BASE + 0x130)	/* link config register */
#define HISAS30HV100_PORT_REG_CON_STATUS_ENQ_REG  (HISAS30HV100_PORT_REG_BASE + 0x134)	/* link status enquiry register */
#define HISAS30HV100_PORT_REG_PORT_CC_STATUS_REG  (HISAS30HV100_PORT_REG_BASE + 0x138)	/* Port level close link time register */
#define HISAS30HV100_PORT_REG_PORT_MACHINE_STATUS_REG  (HISAS30HV100_PORT_REG_BASE + 0x13C)	/* Port level status machine register */
#define HISAS30HV100_PORT_REG_ARB_STATUS_REG      (HISAS30HV100_PORT_REG_BASE + 0x140)	/* arbitrate status register */
#define HISAS30HV100_PORT_REG_IT_NEXUS_STATUS_REG  (HISAS30HV100_PORT_REG_BASE + 0x144)	/* I_T Nexus Loss timer status register */
#define HISAS30HV100_PORT_REG_ACT_CON_TIMERS_STATUS_REG  (HISAS30HV100_PORT_REG_BASE + 0x148)	/* active link timer status register */
#define HISAS30HV100_PORT_REG_TX_FRM_FAIL_STATUS_REG  (HISAS30HV100_PORT_REG_BASE + 0x14C)	/* frame send FAIL status register */
#define HISAS30HV100_PORT_REG_RX_FRM_CHECK_EN_REG  (HISAS30HV100_PORT_REG_BASE + 0x150)	/* receive SSP frame check enable register */
#define HISAS30HV100_PORT_REG_RX_DATA_EXP_DOFFSET_REG  (HISAS30HV100_PORT_REG_BASE + 0x154)	/* Rx Data expect Data doffset register */
#define HISAS30HV100_PORT_REG_TX_NEXT_DATAOFFSET_REG  (HISAS30HV100_PORT_REG_BASE + 0x158)	/* the next Data frame to be send Data Offset */
#define HISAS30HV100_PORT_REG_RX_ERR_STATUS_REG   (HISAS30HV100_PORT_REG_BASE + 0x15C)	/* Rx frame check error info register */
#define HISAS30HV100_PORT_REG_RXFH_DWORD0_REG     (HISAS30HV100_PORT_REG_BASE + 0x160)	/* Rx SSP frame storage register dword0 */
#define HISAS30HV100_PORT_REG_RXFH_DWORD1_REG     (HISAS30HV100_PORT_REG_BASE + 0x164)	/* Rx SSP frame storage register dword1 */
#define HISAS30HV100_PORT_REG_RXFH_DWORD2_REG     (HISAS30HV100_PORT_REG_BASE + 0x168)	/* Rx SSP frame storage register dword2 */
#define HISAS30HV100_PORT_REG_RXFH_DWORD3_REG     (HISAS30HV100_PORT_REG_BASE + 0x16C)	/* Rx SSP frame storage register dword3 */
#define HISAS30HV100_PORT_REG_RXFH_DWORD4_REG     (HISAS30HV100_PORT_REG_BASE + 0x170)	/* Rx SSP frame storage register dword4 */
#define HISAS30HV100_PORT_REG_RXFH_DWORD5_REG     (HISAS30HV100_PORT_REG_BASE + 0x174)	/* Rx SSP frame storage register dword5 */
#define HISAS30HV100_PORT_REG_RX_XRDY_OFFSET_REG  (HISAS30HV100_PORT_REG_BASE + 0x178)	/* Rx XFER_RDY frame compensate register */
#define HISAS30HV100_PORT_REG_RX_XRDY_WLEN_REG    (HISAS30HV100_PORT_REG_BASE + 0x17C)	/* Rx XFER_RDY frame WLEN field */
#define HISAS30HV100_PORT_REG_RX_FRM_ICT_REG      (HISAS30HV100_PORT_REG_BASE + 0x180)	/* ICT number */
#define HISAS30HV100_PORT_REG_RX_SSP_BASE_INFO_REG  (HISAS30HV100_PORT_REG_BASE + 0x184)	/* receive SSP frame basic infomation */
#define HISAS30HV100_PORT_REG_RX_SMP_BASE_INFO_REG  (HISAS30HV100_PORT_REG_BASE + 0x188)	/* receive SMP frame basic infomation */
#define HISAS30HV100_PORT_REG_TX_FRM_STATUS_REG   (HISAS30HV100_PORT_REG_BASE + 0x18C)	/* transmit frame status register */
#define HISAS30HV100_PORT_REG_RX_SMP_ERR_INFO_REG  (HISAS30HV100_PORT_REG_BASE + 0x194)	/* receive SMP frame check error register */
#define HISAS30HV100_PORT_REG_TRANS_MACHINE_STATUS_REG  (HISAS30HV100_PORT_REG_BASE + 0x19C)	/* transmit machine register */
#define HISAS30HV100_PORT_REG_PHY_CONFIG0_REG     (HISAS30HV100_PORT_REG_BASE + 0x1A0)	/* Serdes congtrol register0 */
#define HISAS30HV100_PORT_REG_PHY_CONFIG1_REG     (HISAS30HV100_PORT_REG_BASE + 0x1A4)	/* Serdes congtrol register1 */
#define HISAS30HV100_PORT_REG_PHY_CONFIG2_REG     (HISAS30HV100_PORT_REG_BASE + 0x1A8)	/* Serdes congtrol register2 */
#define HISAS30HV100_PORT_REG_CHL_INT0_REG        (HISAS30HV100_PORT_REG_BASE + 0x1B0)	/* channel interrupt register0 */
#define HISAS30HV100_PORT_REG_CHL_INT1_REG        (HISAS30HV100_PORT_REG_BASE + 0x1B4)	/* channel interrupt register1 */
#define HISAS30HV100_PORT_REG_CHL_INT2_REG        (HISAS30HV100_PORT_REG_BASE + 0x1B8)	/* channel interrupt register2 */
#define HISAS30HV100_PORT_REG_CHL_INT0_MSK_REG    (HISAS30HV100_PORT_REG_BASE + 0x1BC)	/* channel interrupt register0 mask register */
#define HISAS30HV100_PORT_REG_CHL_INT1_MSK_REG    (HISAS30HV100_PORT_REG_BASE + 0x1C0)	/* channel interrupt register1 mask register */
#define HISAS30HV100_PORT_REG_CHL_INT2_MSK_REG    (HISAS30HV100_PORT_REG_BASE + 0x1C4)	/* channel interrupt register2 mask register */
#define HISAS30HV100_PORT_REG_CHL_INT_COAL_EN_REG  (HISAS30HV100_PORT_REG_BASE + 0x1D0)	/* interrupt converge  enable register */
#define HISAS30HV100_PORT_REG_CHL_RST_REG         (HISAS30HV100_PORT_REG_BASE + 0x1D4)	/* channel reset register */
#define HISAS30HV100_PORT_REG_ERR_STAT_EN_L_REG   (HISAS30HV100_PORT_REG_BASE + 0x1E0)	/* channel error type register */
#define HISAS30HV100_PORT_REG_ERR_STAT_EN_H_REG   (HISAS30HV100_PORT_REG_BASE + 0x1E4)	/* channel error type register */
#define HISAS30HV100_PORT_REG_ERR_CNT0_REG        (HISAS30HV100_PORT_REG_BASE + 0x1E8)	/* error type 0 statistic register */
#define HISAS30HV100_PORT_REG_ERR_CNT1_REG        (HISAS30HV100_PORT_REG_BASE + 0x1EC)	/* error type 1 statistic register */
#define HISAS30HV100_PORT_REG_ERR_CNT2_REG        (HISAS30HV100_PORT_REG_BASE + 0x1F0)	/* error type 2 statistic register */
#define HISAS30HV100_PORT_REG_ERR_CNT3_REG        (HISAS30HV100_PORT_REG_BASE + 0x1F4)	/* error type 3 statistic register */
#define HISAS30HV100_PORT_REG_TX_OP_DWORD0_REG    (HISAS30HV100_PORT_REG_BASE + 0x200)	/* transmit OPEN address frame dword0 */
#define HISAS30HV100_PORT_REG_TX_OP_DWORD1_REG    (HISAS30HV100_PORT_REG_BASE + 0x204)	/* transmit OPEN address frame dword1 */
#define HISAS30HV100_PORT_REG_TX_OP_DWORD2_REG    (HISAS30HV100_PORT_REG_BASE + 0x208)	/* transmit OPEN address frame dword2 */
#define HISAS30HV100_PORT_REG_TX_OP_DWORD3_REG    (HISAS30HV100_PORT_REG_BASE + 0x20C)	/* transmit OPEN address frame dword3 */
#define HISAS30HV100_PORT_REG_TX_OP_DWORD4_REG    (HISAS30HV100_PORT_REG_BASE + 0x210)	/* transmit OPEN address frame dword4 */
#define HISAS30HV100_PORT_REG_TX_OP_DWORD5_REG    (HISAS30HV100_PORT_REG_BASE + 0x214)	/* transmit OPEN address frame dword5 */
#define HISAS30HV100_PORT_REG_TX_OP_DWORD6_REG    (HISAS30HV100_PORT_REG_BASE + 0x218)	/* transmit OPEN address frame dword6 */
#define HISAS30HV100_PORT_REG_CON_CFG_HGC_REG     (HISAS30HV100_PORT_REG_BASE + 0x21C)	/* arbitrage config register */
#define HISAS30HV100_PORT_REG_CON_TIMER_CFG_REG   (HISAS30HV100_PORT_REG_BASE + 0x224)	/* link time config register */
#define HISAS30HV100_PORT_REG_IT_NEXUS_CFG_REG    (HISAS30HV100_PORT_REG_BASE + 0x228)	/* I_T Nexus Loss timer config register */
#define HISAS30HV100_PORT_REG_OPEN_ACC_REJ_REG    (HISAS30HV100_PORT_REG_BASE + 0x22C)	/* OPEN address frame Accept/Reject register */
#define HISAS30HV100_PORT_REG_TXFH_DWORD0_REG     (HISAS30HV100_PORT_REG_BASE + 0x230)	/* transmit SSP frame dword0 */
#define HISAS30HV100_PORT_REG_TXFH_DWORD1_REG     (HISAS30HV100_PORT_REG_BASE + 0x234)	/* transmit SSP frame dword1 */
#define HISAS30HV100_PORT_REG_TXFH_DWORD2_REG     (HISAS30HV100_PORT_REG_BASE + 0x238)	/* transmit SSP frame dword2 */
#define HISAS30HV100_PORT_REG_TXFH_DWORD3_REG     (HISAS30HV100_PORT_REG_BASE + 0x23C)	/* transmit SSP frame dword3 */
#define HISAS30HV100_PORT_REG_TXFH_DWORD4_REG     (HISAS30HV100_PORT_REG_BASE + 0x240)	/* transmit SSP frame dword4 */
#define HISAS30HV100_PORT_REG_TXFH_DWORD5_REG     (HISAS30HV100_PORT_REG_BASE + 0x244)	/* transmit SSP frame dword5 */
#define HISAS30HV100_PORT_REG_TX_FRM_MAX_SIZE_REG  (HISAS30HV100_PORT_REG_BASE + 0x248)	/* max SSP frame size */
#define HISAS30HV100_PORT_REG_TX_SMP_FRM_REQ_REG  (HISAS30HV100_PORT_REG_BASE + 0x250)	/* trasmit SMP frame request config register */
#define HISAS30HV100_PORT_REG_TX_SSP_FRM_REQ_REG  (HISAS30HV100_PORT_REG_BASE + 0x25C)	/* trasmit ssp frame request config register */
#define HISAS30HV100_PORT_REG_TRANS_TXRX_STATUS_REG  (HISAS30HV100_PORT_REG_BASE + 0x264)	/* receive frame status register */
#define HISAS30HV100_PORT_REG_IO_END_STATUS_REG   (HISAS30HV100_PORT_REG_BASE + 0x268)	/*  IO end status register*/
#define HISAS30HV100_PORT_REG_HGC_IO_TX_CONFIG_R_REG  (HISAS30HV100_PORT_REG_BASE + 0x270)	/* IO tranmit config register */
#define HISAS30HV100_PORT_REG_HGC_IO_CMD_CONFIG_R_REG  (HISAS30HV100_PORT_REG_BASE + 0x274)	/* command config register */
#define HISAS30HV100_PORT_REG_HGC_IO_CMD_BASEADDR_L_REG  (HISAS30HV100_PORT_REG_BASE + 0x278)	/* Command table base address register */
#define HISAS30HV100_PORT_REG_HGC_IO_CMD_BASEADDR_H_REG  (HISAS30HV100_PORT_REG_BASE + 0x27C)	/* Command table base address register */
#define HISAS30HV100_PORT_REG_HGC_IO_CMD_DATA_LENGTH_REG  (HISAS30HV100_PORT_REG_BASE + 0x280)	/* IO command data length register */
#define HISAS30HV100_PORT_REG_HGC_IO_DATA_PRDT_ADDR_L_REG  (HISAS30HV100_PORT_REG_BASE + 0x284)	/* data PRD Table address register */
#define HISAS30HV100_PORT_REG_HGC_IO_DATA_PRDT_ADDR_H_REG  (HISAS30HV100_PORT_REG_BASE + 0x288)	/* data PRD Table address register */
#define HISAS30HV100_PORT_REG_HGC_IO_SGL_LENGTH_REG  (HISAS30HV100_PORT_REG_BASE + 0x28C)	/* entry number in Data SGL and DIF SGL */
#define HISAS30HV100_PORT_REG_HGC_IO_DIF_PRDT_ADDR_L_REG  (HISAS30HV100_PORT_REG_BASE + 0x290)	/* DIF PRD Table address register */
#define HISAS30HV100_PORT_REG_HGC_IO_DIF_PRDT_ADDR_H_REG  (HISAS30HV100_PORT_REG_BASE + 0x294)	/* DIF PRD Table address register */
#define HISAS30HV100_PORT_REG_DMA_TX_DFX0_REG     (HISAS30HV100_PORT_REG_BASE + 0x29C)	/* Tx DMA DFX register0 */
#define HISAS30HV100_PORT_REG_DMA_TX_DFX1_REG     (HISAS30HV100_PORT_REG_BASE + 0x2A0)	/* Tx DMA  DFX register1 */
#define HISAS30HV100_PORT_REG_DMA_TX_DFX2_REG     (HISAS30HV100_PORT_REG_BASE + 0x2A4)	/* Tx DMA  DFX register2 */
#define HISAS30HV100_PORT_REG_DMA_RX_DFX0_REG     (HISAS30HV100_PORT_REG_BASE + 0x2A8)	/* Rx DMA DFX signal */
#define HISAS30HV100_PORT_REG_DMA_RX_DFX1_REG     (HISAS30HV100_PORT_REG_BASE + 0x2AC)	/* Rx DMA DFX signal */
#define HISAS30HV100_PORT_REG_DMA_RX_DFX2_REG     (HISAS30HV100_PORT_REG_BASE + 0x2B0)	/* Rx DMA DFX signal */
#define HISAS30HV100_PORT_REG_DMA_TX_ERR_STATUS_REG  (HISAS30HV100_PORT_REG_BASE + 0x2C4)	/* Tx DMA error status register */
#define HISAS30HV100_PORT_REG_DMA_TX_STATUS_REG   (HISAS30HV100_PORT_REG_BASE + 0x2D0)	/* DMA status register */
#define HISAS30HV100_PORT_REG_DMA_RX_ERR_STATUS_REG  (HISAS30HV100_PORT_REG_BASE + 0x2DC)	/* Rx DMA error status register */
#define HISAS30HV100_PORT_REG_HGC_DMA_TX_FAIL_PROC_CFG_REG  (HISAS30HV100_PORT_REG_BASE + 0x2E0)	/* Tx error DMA config register */
#define HISAS30HV100_PORT_REG_DMA_RX_STATUS_REG   (HISAS30HV100_PORT_REG_BASE + 0x2E8)	/* DMA status register */
#define HISAS30HV100_PORT_REG_DMA_TX_XFER_LGTH_REG  (HISAS30HV100_PORT_REG_BASE + 0x2F4)	/* Tx DMA transmit length register */
#define HISAS30HV100_PORT_REG_DMA_TX_XFER_OFFSET_REG  (HISAS30HV100_PORT_REG_BASE + 0x2F8)	/* Tx DMA transmit data offset register */
#define HISAS30HV100_PORT_REG_HGC_IO_FST_BURST_NUM_REG  (HISAS30HV100_PORT_REG_BASE + 0x2FC)
#define HISAS30HV100_PORT_REG_HGC_IO_RESP_CONFIG_R_REG  (HISAS30HV100_PORT_REG_BASE + 0x300)	/* response config register of IO */
#define HISAS30HV100_PORT_REG_HGC_IO_STATUS_BUF_ADDR_L_REG  (HISAS30HV100_PORT_REG_BASE + 0x304)	/* Status buffer memory address of io */
#define HISAS30HV100_PORT_REG_HGC_IO_STATUS_BUF_ADDR_H_REG  (HISAS30HV100_PORT_REG_BASE + 0x308)	/* Status buffer memory address of io */
#define HISAS30HV100_PORT_REG_RX_OPAF_DWORD0_REG  (HISAS30HV100_PORT_REG_BASE + 0x30C)	/* receive OPEN address frame dword0 */
#define HISAS30HV100_PORT_REG_RX_OPAF_DWORD1_REG  (HISAS30HV100_PORT_REG_BASE + 0x310)	/* receive OPEN address frame dword1 */
#define HISAS30HV100_PORT_REG_RX_OPAF_DWORD2_REG  (HISAS30HV100_PORT_REG_BASE + 0x314)	/* receive OPEN address frame dword2 */
#define HISAS30HV100_PORT_REG_RX_OPAF_DWORD3_REG  (HISAS30HV100_PORT_REG_BASE + 0x318)	/* receive OPEN address frame dword3 */
#define HISAS30HV100_PORT_REG_RX_OPAF_DWORD4_REG  (HISAS30HV100_PORT_REG_BASE + 0x31C)	/* receive OPEN address frame dword4 */
#define HISAS30HV100_PORT_REG_RX_OPAF_DWORD5_REG  (HISAS30HV100_PORT_REG_BASE + 0x320)	/* receive OPEN address frame dword5 */
#define HISAS30HV100_PORT_REG_RX_OPAF_DWORD6_REG  (HISAS30HV100_PORT_REG_BASE + 0x324)	/* receive OPEN address frame dword6 */
#define HISAS30HV100_PORT_REG_G_CSR_CFGR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x39C)	/* SATA CFGR register */
#define HISAS30HV100_PORT_REG_G_CSR_GCR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3A0)	/* SATA global control register */
#define HISAS30HV100_PORT_REG_G_CSR_BISTAFR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3A4)	/* SATA BIST Activate FIS register */
#define HISAS30HV100_PORT_REG_G_CSR_BISTCR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3A8)	/* SATA BIST control register */
#define HISAS30HV100_PORT_REG_G_CSR_BISTFCTR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3AC)	/* SATA BIST FIS count register */
#define HISAS30HV100_PORT_REG_G_CSR_BISTSR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3B0)	/* SATA BIST status register */
#define HISAS30HV100_PORT_REG_G_CSR_BISTDECR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3B4)	/* SATA BIST DW error count register */
#define HISAS30HV100_PORT_REG_G_CSR_DIAGNR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3B8)	/* SATA diagnouse register */
#define HISAS30HV100_PORT_REG_G_CSR_DIAGNR1_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3BC)	/* SATA diagnouse register1 */
#define HISAS30HV100_PORT_REG_G_CSR_GPARAM1R_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3C0)	/* SATA global parameter register */
#define HISAS30HV100_PORT_REG_G_CSR_GPARM2R_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3C4)	/* SATA global parameter register1 */
#define HISAS30HV100_PORT_REG_G_CSR_PPARAMR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3C8)	/* SATA PORT parameter register */
#define HISAS30HV100_PORT_REG_G_CSR_TESTR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3CC)	/* SATA TEST register */
#define HISAS30HV100_PORT_REG_G_CSR_VERSIONR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3D0)	/* SATA VERSION register */
#define HISAS30HV100_PORT_REG_P_CSR_TXTR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3D4)	/* SATA transmit status register */
#define HISAS30HV100_PORT_REG_P_CSR_RXTR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3D8)	/* SATA receive status register */
#define HISAS30HV100_PORT_REG_P_CSR_STMR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3DC)	/* SATA STP frame timer register */
#define HISAS30HV100_PORT_REG_P_CSR_PISR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3E0)	/* SATA Port interrupt status register */
#define HISAS30HV100_PORT_REG_P_CSR_PIER_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3E4)	/* SATA Port interrupt eneble register */
#define HISAS30HV100_PORT_REG_P_CSR_PCMDR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3E8)	/* SATA Port CMD register */
#define HISAS30HV100_PORT_REG_P_CSR_PSSTSR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3EC)	/* Port SATA Status register */
#define HISAS30HV100_PORT_REG_P_CSR_PSCTLR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3F0)	/* Port SATA control register */
#define HISAS30HV100_PORT_REG_P_CSR_PSERR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3F4)	/* SATA Error register */
#define HISAS30HV100_PORT_REG_P_CSR_PSDR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3F8)	/* SATA status word register */
#define HISAS30HV100_PORT_REG_P_CSR_PDMACR_HOST_REG  (HISAS30HV100_PORT_REG_BASE + 0x3FC)	/* SATA Port DMA control register */

/* Define the union U_PHY_CFG */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_phy_enabled:1;
		unsigned int cfg_sata_supp:1;
		unsigned int cfg_dc_opt_mode:1;
		unsigned int Reserved_0:29;
	} bits;

	/* Define an unsigned member*/
	unsigned int u32;

} U_PHY_CFG;

/* Define the union U_HARD_PHY_LINK_RATE */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int phyctrl_max_rate:4;
		unsigned int phyctrl_min_rate:4;
		unsigned int phyctrl_neg_link_rate:4;
		unsigned int phyctrl_att_device:2;
		unsigned int phyctrl_pwr_var:4;
		unsigned int phyctrl_csnw_var:3;
		unsigned int Reserved_1:11;
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_HARD_PHY_LINK_RATE;

/* Define the union U_PHY_STATE */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int phyctrl_state:30;
		unsigned int Reserved_2:2;
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PHY_STATE2;

/* Define the union U_PROG_PHY_LINK_RATE */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_prog_max_phy_link_rate:4;
		unsigned int cfg_prog_min_phy_link_rate:4;
		unsigned int cfg_prog_oob_phy_link_rate:4;
		unsigned int Reserved_3:20;
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PROG_PHY_LINK_RATE;

/* Define the union U_PHY_SUMBER_TIME */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_wakup_slum_time:16;
		unsigned int Reserved_4:16;
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PHY_SUMBER_TIME;

/* Define the union U_PHY_CTRL */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_stop_sntt:1;
		unsigned int cfg_reset_req:1;
		unsigned int Reserved_5:30;
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PHY_CTRL;

/* Define the union U_PHY_TRAIN_TIME */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_max_train_time:16;
		unsigned int cfg_max_train_lock_time:16;
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PHY_TRAIN_TIME;

/* Define the union U_PHY_RATE_CHG */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_rate_chg_delay_time:9;	/* [8..0] */
		unsigned int cfg_max_speed_neg_time:7;	/* [15..9] */
		unsigned int cfg_max_speed_lock_time:7;	/* [22..16] */
		unsigned int Reserved_6:9;	/* [31..23] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PHY_RATE_CHG;

/* Define the union U_PHY_TIMER_SET */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_hot_plug_time:20;	/* [19..0] */
		unsigned int cfg_bit_cell_time:8;	/* [27..20] */
		unsigned int Reserved_7:4;	/* [31..28] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PHY_TIMER_SET;

/* Define the union U_PHY_RATE_NEGO */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int Reserved_8:9;	/* [8..0] */
		unsigned int cfg_snw_support:3;	/* [11..9] */
		unsigned int cfg_comsas_wait_time:4;	/* [15..12] */
		unsigned int cfg_burst_time:16;	/* [31..16] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PHY_RATE_NEGO;

/* Define the union U_PHY_DWS_RESET_TIME */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_dws_reset_time:16;	/* [15..0] */
		unsigned int Reserved_9:16;	/* [31..16] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PHY_DWS_RESET_TIME;

/* Define the union U_PHY_BIST_CTRL */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_bist_mode_sel:3;	/* [2..0] */
		unsigned int cfg_loop_test_mode:2;	/* [4..3] */
		unsigned int cfg_bist_test:1;	/* [5] */
		unsigned int cfg_loop_test:1;	/* [6] */
		unsigned int Reserved_10:25;	/* [31..7] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PHY_BIST_CTRL;

/* Define the union U_PHYCTRL_STATUS */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int phyctrl_status:3;	/* [2..0] */
		unsigned int Reserved_11:29;	/* [31..3] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PHYCTRL_STATUS;

/* Define the union U_PHY_SATA_CFG */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_sata_rcspd_time:16;	/* [15..0] */
		unsigned int cfg_sata_awal_time:16;	/* [31..16] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PHY_SATA_CFG;

/* Define the union U_PHY_SATA_NEGO_TIME */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_sata_aligndly_time:16;	/* [15..0] */
		unsigned int Reserved_12:16;	/* [31..16] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PHY_SATA_NEGO_TIME;

/* Define the union U_PHY_EINJ_EN */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_einj_en:1;	/* [0] */
		unsigned int Reserved_13:31;	/* [31..1] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PHY_EINJ_EN;

/* Define the union U_RXOP_HW_OPREJ */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int sl_tx_hw_oprej_ret:1;	/* [0] */
		unsigned int sl_tx_hw_oprej_pro_ict:1;	/* [1] */
		unsigned int sl_tx_hw_oprej_pro_feat:1;	/* [2] */
		unsigned int sl_tx_hw_oprej_pro_ipcol:1;	/* [3] */
		unsigned int sl_tx_hw_oprej_con:1;	/* [4] */
		unsigned int sl_tx_hw_oprej_wdes:1;	/* [5] */
		unsigned int Reserved_16:26;	/* [31..6] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_RXOP_HW_OPREJ;

/* Define the union U_SSP_DEBUG_STATUS */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int ssp_tm_a:29;	/* [28..0] */
		unsigned int Reserved_18:3;	/* [31..29] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_SSP_DEBUG_STATUS;

/* Define the union U_SL_DEBUG_STATUS0 */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int sl_tm_b:19;	/* [18..0] */
		unsigned int Reserved_19:13;	/* [31..19] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_SL_DEBUG_STATUS0;

/* Define the union U_SL_PRIM_STATUS */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int sl_tx_bcast_retry:1;	/* [0] */
		unsigned int Reserved_22:1;	/* [1] */
		unsigned int Reserved_21:1;	/* [2] */
		unsigned int sl_psreq_exit:1;	/* [3] */
		unsigned int Reserved_20:28;	/* [31..4] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_SL_PRIM_STATUS;

/* Define the union U_SL_CFG */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_error_dis_en:1;	/* [0] */
		unsigned int cfg_scr_disable:1;	/* [1] */
		unsigned int cfg_clkskew_align_cnt:12;	/* [13..2] */
		unsigned int cfg_align_prim_type:2;	/* [15..14] */
		unsigned int cfg_notify_prim_type:2;	/* [17..16] */
		unsigned int cfg_en_break_reply:1;	/* [18] */
		unsigned int cfg_sata_align_cnt:7;	/* [25..19] */
		unsigned int Reserved_23:6;	/* [31..26] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_SL_CFG;

/* Define the union U_SL_TOUT_CFG */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_break_tout_limit:8;	/* [7..0] */
		unsigned int cfg_close_tout_limit:8;	/* [15..8] */
		unsigned int cfg_op_tout_limit:8;	/* [23..16] */
		unsigned int cfg_id_tout_limit:8;	/* [31..24] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_SL_TOUT_CFG;

/* Define the union U_AIP_LIMIT */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_aip_limit:16;	/* [15..0] */
		unsigned int cfg_aip_cnttimn:1;	/* [16] */
		unsigned int cfg_en_aip_limit:1;	/* [17] */
		unsigned int Reserved_24:14;	/* [31..18] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_AIP_LIMIT;

/* Define the union U_SL_CONTROL */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_notify_en:1;	/* [0] */
		unsigned int cfg_tri_notify_en:1;	/* [1] */
		unsigned int cfg_force_rx_badcrc:1;	/* [2] */
		unsigned int cfg_force_rx_goodcrc:1;	/* [3] */
		unsigned int cfg_force_tx_badcrc:1;	/* [4] */
		unsigned int cfg_force_rx_type:6;	/* [10..5] */
		unsigned int cfg_force_tx_type:6;	/* [16..11] */
		unsigned int cfg_txid_auto:1;	/* [17] */
		unsigned int Reserved_25:14;	/* [31..18] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_SL_CONTROL;

/* Define the union U_RX_PRIMS_STATUS */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int sl_rx_bcast_ses_ack:1;	/* [0] */
		unsigned int sl_rx_bcast_chg_ack:1;	/* [1] */
		unsigned int sl_rx_bcast_res1_ack:1;	/* [2] */
		unsigned int sl_rx_bcast_res2_ack:1;	/* [3] */
		unsigned int sl_rx_bcast_res3_ack:1;	/* [4] */
		unsigned int sl_rx_bcast_res4_ack:1;	/* [5] */
		unsigned int sl_rx_bcast_res_chg0_ack:1;	/* [6] */
		unsigned int sl_rx_bcast_res_chg1_ack:1;	/* [7] */
		unsigned int Reserved_26:24;	/* [31..8] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_RX_PRIMS_STATUS;

/* Define the union U_TXID_AUTO */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_tx_id:1;	/* [0] */
		unsigned int cfg_tx_3id:1;	/* [1] */
		unsigned int cfg_tx_hardrst:1;	/* [2] */
		unsigned int cfg_tx_bcast_ses_req:1;	/* [3] */
		unsigned int cfg_tx_bcast_chg_req:1;	/* [4] */
		unsigned int cfg_tx_bcast_res1_req:1;	/* [5] */
		unsigned int cfg_tx_bcast_res2_req:1;	/* [6] */
		unsigned int cfg_tx_bcast_res3_req:1;	/* [7] */
		unsigned int cfg_tx_bcast_res4_req:1;	/* [8] */
		unsigned int cfg_tx_bcast_res_chg0_req:1;	/* [9] */
		unsigned int cfg_tx_bcast_res_chg1_req:1;	/* [10] */
		unsigned int cfg_tx_break:1;	/* [11] */
		unsigned int Reserved_28:1;	/* [12] */
		unsigned int cfg_psreq_slu:1;	/* [13] */
		unsigned int cfg_psreq_par:1;	/* [14] */
		unsigned int Reserved_27:17;	/* [31..15] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_TXID_AUTO;

/* Define the union U_TX_RX_AF_STATUS_ENQ */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int sl_rxop_failtype:3;	/* [2..0] */
		unsigned int sl_rxid_failtype:3;	/* [5..3] */
		unsigned int sl_txid_err:1;	/* [6] */
		unsigned int Reserved_29:25;	/* [31..7] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_TX_RX_AF_STATUS_ENQ;

/* Define the union U_RXOP_CHECK_CFG_H */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_rxop_com_ftr:8;	/* [7..0] */
		unsigned int cfg_rxop_ftr:4;	/* [11..8] */
		unsigned int cfg_rxop_chk_en:1;	/* [12] */
		unsigned int cfg_rxop_feat_en:1;	/* [13] */
		unsigned int cfg_rxop_nzfeat_acc:1;	/* [14] */
		unsigned int cfg_rxop_nzfeat_sw:1;	/* [15] */
		unsigned int cfg_host_exp_en:1;	/* [16] */
		unsigned int cfg_error_chk_en:1;	/* [17] */
		unsigned int Reserved_31:14;	/* [31..18] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_RXOP_CHECK_CFG_H;

/* Define the union U_PW_MAG */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_acc_slu:1;	/* [0] */
		unsigned int cfg_acc_par:1;	/* [1] */
		unsigned int cfg_psreq_exit:1;	/* [2] */
		unsigned int cfg_ps_tout_limit:8;	/* [10..3] */
		unsigned int Reserved_33:21;	/* [31..11] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PW_MAG;

/* Define the union U_RXOP_ICT_RANGE */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_rxop_ict_lower:16;	/* [15..0] */
		unsigned int cfg_rxop_ict_upper:16;	/* [31..16] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_RXOP_ICT_RANGE;

/* Define the union U_STP_CON_CLOSE */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_stp_cc_count:16;	/* [15..0] */
		unsigned int cfg_auto_cc_stp:1;	/* [16] */
		unsigned int cfg_close_aff:1;	/* [17] */
		unsigned int cfg_sata_close_aff:1;	/* [18] */
		unsigned int Reserved_34:13;	/* [31..19] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_STP_CON_CLOSE;

/* Define the union U_PRIM_TOUT_CFG */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_done_tout_limit:8;	/* [7..0] */
		unsigned int cfg_acknak_tout_limit:8;	/* [15..8] */
		unsigned int cfg_cred_tout_limit:8;	/* [23..16] */
		unsigned int Reserved_35:8;	/* [31..24] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PRIM_TOUT_CFG;

/* Define the union U_SSP_CONTROL */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_rrdy_prim_type:2;	/* [1..0] */
		unsigned int cfg_nak_prim_type:2;	/* [3..2] */
		unsigned int cfg_done_tout_prim_type:2;	/* [5..4] */
		unsigned int cfg_done_norm_prim_type:2;	/* [7..6] */
		unsigned int cfg_suppress_ack:1;	/* [8] */
		unsigned int cfg_tx_frame_gap:4;	/* [12..9] */
		unsigned int cfg_tx_collate_prims:1;	/* [13] */
		unsigned int Reserved_36:18;	/* [31..14] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_SSP_CONTROL;

/* Define the union U_ACK_NAK_BLC */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int ssp_tx_balstatus:1;	/* [0] */
		unsigned int ssp_rx_balstatus:1;	/* [1] */
		unsigned int Reserved_37:30;	/* [31..2] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_ACK_NAK_BLC;

/* Define the union U_TXF_CRED */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int ssp_txf_cred:8;	/* [7..0] */
		unsigned int Reserved_38:24;	/* [31..8] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_TXF_CRED;

/* Define the union U_RXF_CRED_CONTROL */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_rxf_cred_ext_limit:8;	/* [7..0] */
		unsigned int cfg_rxf_cred_limit:16;	/* [23..8] */
		unsigned int cfg_rxf_cred_limit_update:1;	/* [24] */
		unsigned int cfg_rxf_cred_blocked:1;	/* [25] */
		unsigned int cfg_rxf_over_ext:1;	/* [26] */
		unsigned int Reserved_39:5;	/* [31..27] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_RXF_CRED_CONTROL;

/* Define the union U_CRED_STATUS_ENQ */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int ssp_rxf_cred_extended_cnt:8;	/* [7..0] */
		unsigned int ssp_rxf_cred_left:16;	/* [23..8] */
		unsigned int Reserved_40:8;	/* [31..24] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_CRED_STATUS_ENQ;

/* Define the union U_CON_CONTROL */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_open_acc_stp:1;	/* [0] */
		unsigned int cfg_open_acc_smp:1;	/* [1] */
		unsigned int cfg_open_acc_ssp:1;	/* [2] */
		unsigned int Reserved_41:29;	/* [31..3] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_CON_CONTROL;

/* Define the union U_DONE_RECEVIED_TIME */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_done_rcv_time:16;	/* [15..0] */
		unsigned int Reserved_42:16;	/* [31..16] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_DONE_RECEVIED_TIME;

/* Define the union U_CON_CFG_DRIVER */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_retry_limit:8;	/* [7..0] */
		unsigned int cfg_en_retry_pb:1;	/* [8] */
		unsigned int cfg_en_retry_br:1;	/* [9] */
		unsigned int cfg_en_retry_lp:1;	/* [10] */
		unsigned int cfg_en_retry_retry:1;	/* [11] */
		unsigned int cfg_en_retry_nodest:1;	/* [12] */
		unsigned int cfg_en_retry_tout:1;	/* [13] */
		unsigned int cfg_close_prim_type:2;	/* [15..14] */
		unsigned int cfg_con_timer_close_en:1;	/* [16] */
		unsigned int Reserved_43:15;	/* [31..17] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_CON_CFG_DRIVER;

/* Define the union U_CON_STATUS_ENQ */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int pl_co_fail_status:17;	/* [16..0] */
		unsigned int pl_co_src:1;	/* [17] */
		unsigned int pl_co_dest:1;	/* [18] */
		unsigned int pl_co_fail:1;	/* [19] */
		unsigned int pl_con_closed:1;	/* [20] */
		unsigned int pl_stp_cc_rdy:1;	/* [21] */
		unsigned int pl_co_req_busy:1;	/* [22] */
		unsigned int pl_smp_conn_open:1;	/* [23] */
		unsigned int pl_stp_conn_open:1;	/* [24] */
		unsigned int pl_ssp_conn_open:1;	/* [25] */
		unsigned int Reserved_44:6;	/* [31..26] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_CON_STATUS_ENQ;

/* Define the union U_PORT_CC_STATUS */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int pl_cc_status:8;	/* [7..0] */
		unsigned int Reserved_45:24;	/* [31..8] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PORT_CC_STATUS;

/* Define the union U_PORT_MACHINE_STATUS */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int pl_pm_curr_st:7;	/* [6..0] */
		unsigned int pl_oc_curr_st:6;	/* [12..7] */
		unsigned int Reserved_46:19;	/* [31..13] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PORT_MACHINE_STATUS;

/* Define the union U_ARB_STATUS */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int pl_txop_arb_wait_time:16;	/* [15..0] */
		unsigned int pl_txop_pb_cnt:8;	/* [23..16] */
		unsigned int Reserved_47:8;	/* [31..24] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_ARB_STATUS;

/* Define the union U_IT_NEXUS_STATUS */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int pl_it_nex_status:16;	/* [15..0] */
		unsigned int Reserved_48:16;	/* [31..16] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_IT_NEXUS_STATUS;

/* Define the union U_ACT_CON_TIMERS_STATUS */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int pl_bus_inact_cnt:16;	/* [15..0] */
		unsigned int pl_max_con_cnt:16;	/* [31..16] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_ACT_CON_TIMERS_STATUS;

/* Define the union U_RX_FRM_CHECK_EN */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_ssp_rx_chk_en:1;	/* [0] */
		unsigned int Reserved_49:31;	/* [31..1] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_RX_FRM_CHECK_EN;

/* Define the union U_RX_FRM_ICT */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int st_rx_frm_ict:16;	/* [15..0] */
		unsigned int Reserved_51:16;	/* [31..16] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_RX_FRM_ICT;

/* Define the union U_TX_FRM_STATUS */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int trans_tx_busy:1;	/* [0] */
		unsigned int Reserved_52:31;	/* [31..1] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_TX_FRM_STATUS;

/* Define the union U_TRANS_MACHINE_STATUS */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int st_hsh_curr_state:4;	/* [3..0] */
		unsigned int st_fcs_curr_state:10;	/* [13..4] */
		unsigned int st_fcr_curr_state:4;	/* [17..14] */
		unsigned int mt_sr_curr_state:11;	/* [28..18] */
		unsigned int Reserved_55:3;	/* [31..29] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_TRANS_MACHINE_STATUS;

/* Define the union U_PHY_CONFIG0 */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_sas_phy_config0:14;	/* [13..0] */
		unsigned int Reserved_57:18;	/* [31..14] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PHY_CONFIG0;

/* Define the union U_PHY_CONFIG2 */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_rxcltepreset:3;	/* [2..0] */
		unsigned int cfg_force_txdeemph:1;	/* [3] */
		unsigned int cfg_phy_txdeemph:18;	/* [21..4] */
		unsigned int Reserved_58:10;	/* [31..22] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_PHY_CONFIG2;

/* Define the union U_CHL_INT0 */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int phyctrl_not_rdy:1;	/* [0] */
		unsigned int phyctrl_reset_prob:1;	/* [1] */
		unsigned int phyctrl_sn_fail_ngr:1;	/* [2] */
		unsigned int phyctrl_dws_acquired:1;	/* [3] */
		unsigned int phyctrl_dws_lost:1;	/* [4] */
		unsigned int phyctrl_dws_reset:1;	/* [5] */
		unsigned int sl_rx_crc_fail:1;	/* [6] */
		unsigned int sl_syncfifo_ovrun:1;	/* [7] */
		unsigned int sl_rx_break_ack:1;	/* [8] */
		unsigned int sl_idaf_rxd_conf:1;	/* [9] */
		unsigned int sl_idaf_fail_conf:1;	/* [10] */
		unsigned int sl_idaf_tout_conf:1;	/* [11] */
		unsigned int sl_opaf_fail_conf:1;	/* [12] */
		unsigned int sl_opaf_nzfeat:1;	/* [13] */
		unsigned int ssp_done_tout:1;	/* [14] */
		unsigned int ssp_done_norm_rxd:1;	/* [15] */
		unsigned int mt_rx_buf_notrdy_ir:1;	/* [16] */
		unsigned int st_rx_buf_notrdy_ir:1;	/* [17] */
		unsigned int phyctrl_bist_check_err:1;	/* [18] */
		unsigned int phyctrl_bist_check_tout:1;	/* [19] */
		unsigned int phyctrl_bist_check_pass:1;	/* [20] */
		unsigned int sl_ps_fail:1;	/* [21] */
		unsigned int Reserved_59:10;	/* [31..22] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_CHL_INT0;

/* Define the union U_CHL_INT1 */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int ssp_done_cred_rxd:1;	/* [0] */
		unsigned int ssp_done_ackn_rxd:1;	/* [1] */
		unsigned int ssp_done_norm_txd:1;	/* [2] */
		unsigned int ssp_done_cred_txd:1;	/* [3] */
		unsigned int ssp_done_ackn_txd:1;	/* [4] */
		unsigned int pl_ack_timer_tout:1;	/* [5] */
		unsigned int pl_cred_timer_tout:1;	/* [6] */
		unsigned int pl_con_timer_tout:1;	/* [7] */
		unsigned int pl_bus_timer_tout:1;	/* [8] */
		unsigned int ssp_nak_rxd:1;	/* [9] */
		unsigned int ssp_ack_rxd:1;	/* [10] */
		unsigned int ssp_txf_cred_tout:1;	/* [11] */
		unsigned int ssp_txf_cred_blkd:1;	/* [12] */
		unsigned int ssp_rxf_cred_limit_reached:1;	/* [13] */
		unsigned int ssp_rxf_cred_exhausted:1;	/* [14] */
		unsigned int dmac_ram_ecc_bad_err:1;	/* [15] */
		unsigned int dmac_ram_ecc_1b_err:1;	/* [16] */
		unsigned int Reserved_61:15;	/* [31..17] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_CHL_INT1;

/* Define the union U_CHL_INT2 */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int phyctrl_phy_rdy:1;	/* [0] */
		unsigned int phyctrl_hotplug_tout:1;	/* [1] */
		unsigned int sl_rx_bcast_ack:1;	/* [2] */
		unsigned int phyctrl_oob_restart_ci:1;	/* [3] */
		unsigned int sl_rx_hardrst:1;	/* [4] */
		unsigned int phyctrl_status_chg:1;	/* [5] */
		unsigned int sl_phy_enabled:1;	/* [6] */
		unsigned int Reserved_62:25;	/* [31..7] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_CHL_INT2;

/* Define the union U_CHL_INT0_MSK */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int phyctrl_not_rdy_msk:1;	/* [0] */
		unsigned int phyctrl_reset_prob_msk:1;	/* [1] */
		unsigned int phyctrl_sn_fail_ngr_msk:1;	/* [2] */
		unsigned int phyctrl_dws_acquired_msk:1;	/* [3] */
		unsigned int phyctrl_dws_lost_msk:1;	/* [4] */
		unsigned int phyctrl_dws_reset_msk:1;	/* [5] */
		unsigned int sl_rx_crc_fail_msk:1;	/* [6] */
		unsigned int sl_syncfifo_ovrun_msk:1;	/* [7] */
		unsigned int sl_rx_break_ack_msk:1;	/* [8] */
		unsigned int sl_idaf_rxd_conf_msk:1;	/* [9] */
		unsigned int sl_idaf_fail_conf_msk:1;	/* [10] */
		unsigned int sl_idaf_tout_msk:1;	/* [11] */
		unsigned int sl_opaf_invalid_msk:1;	/* [12] */
		unsigned int sl_opaf_nzfeat_msk:1;	/* [13] */
		unsigned int ssp_done_tout_msk:1;	/* [14] */
		unsigned int ssp_done_norm_rxd_msk:1;	/* [15] */
		unsigned int mt_rx_buf_notrdy_ir_msk:1;	/* [16] */
		unsigned int st_rx_buf_notrdy_ir_msk:1;	/* [17] */
		unsigned int phyctrl_bist_check_err_msk:1;	/* [18] */
		unsigned int phyctrl_bist_check_tout_msk:1;	/* [19] */
		unsigned int phyctrl_bist_check_pass_msk:1;	/* [20] */
		unsigned int sl_ps_fail_msk:1;	/* [21] */
		unsigned int Reserved_63:10;	/* [31..22] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_CHL_INT0_MSK;

/* Define the union U_CHL_INT1_MSK */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int ssp_done_cred_rxd_msk:1;	/* [0] */
		unsigned int ssp_done_ackn_rxd_msk:1;	/* [1] */
		unsigned int ssp_done_norm_txd_msk:1;	/* [2] */
		unsigned int ssp_done_cred_txd_msk:1;	/* [3] */
		unsigned int ssp_done_ackn_txd_msk:1;	/* [4] */
		unsigned int pl_ack_timer_tout_msk:1;	/* [5] */
		unsigned int pl_cred_timer_tout_msk:1;	/* [6] */
		unsigned int pl_con_timer_tout_msk:1;	/* [7] */
		unsigned int pl_bus_timer_tout_msk:1;	/* [8] */
		unsigned int ssp_nak_rxd_msk:1;	/* [9] */
		unsigned int ssp_ack_rxd_msk:1;	/* [10] */
		unsigned int ssp_txf_cred_tout_msk:1;	/* [11] */
		unsigned int ssp_txf_cred_blkd_msk:1;	/* [12] */
		unsigned int ssp_rxf_cred_limit_reached_msk:1;	/* [13] */
		unsigned int ssp_rxf_cred_exhausted_msk:1;	/* [14] */
		unsigned int dmac_ram_ecc_bad_err_msk:1;	/* [15] */
		unsigned int dmac_ram_ecc_1b_err_msk:1;	/* [16] */
		unsigned int Reserved_64:15;	/* [31..17] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_CHL_INT1_MSK;

/* Define the union U_CHL_INT2_MSK */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int phyctrl_sas_rdy_msk:1;	/* [0] */
		unsigned int phyctrl_hotplug_tout_msk:1;	/* [1] */
		unsigned int sl_rx_bcast_ack_msk:1;	/* [2] */
		unsigned int phyctrl_oob_restart_ci_msk:1;	/* [3] */
		unsigned int sl_rx_hardrst_msk:1;	/* [4] */
		unsigned int phyctrl_status_chg_msk:1;	/* [5] */
		unsigned int sl_phy_enabled_msk:1;	/* [6] */
		unsigned int Reserved_65:25;	/* [31..7] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_CHL_INT2_MSK;

/* Define the union U_CHL_INT_COAL_EN */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int chl_int_coal_cnt_en:1;	/* [0] */
		unsigned int chl_int_coal_time_en:1;	/* [1] */
		unsigned int Reserved_66:30;	/* [31..2] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_CHL_INT_COAL_EN;

/* Define the union U_CHL_RST */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int rst_sas_phy_pd_n:1;	/* [0] */
		unsigned int rst_sas_phy_n:1;	/* [1] */
		unsigned int Reserved_68:30;	/* [31..2] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_CHL_RST;

/* Define the union U_CON_CFG_HGC */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_cont_awt:1;	/* [0] */
		unsigned int Reserved_71:31;	/* [31..1] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_CON_CFG_HGC;

/* Define the union U_CON_TIMER_CFG */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_bus_inact_time_limit:16;	/* [15..0] */
		unsigned int cfg_max_con_time_limit:16;	/* [31..16] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_CON_TIMER_CFG;

/* Define the union U_IT_NEXUS_CFG */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_it_nex_limit:16;	/* [15..0] */
		unsigned int cfg_rej_open_limit:16;	/* [31..16] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_IT_NEXUS_CFG;

/* Define the union U_OPEN_ACC_REJ */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_close_con:1;	/* [0] */
		unsigned int cfg_tx_open_accept:1;	/* [1] */
		unsigned int cfg_tx_open_reject:1;	/* [2] */
		unsigned int cfg_tx_open_reject_type:5;	/* [7..3] */
		unsigned int Reserved_73:24;	/* [31..8] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_OPEN_ACC_REJ;

/* Define the union U_TX_FRM_MAX_SIZE */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_tx_max_frm_size:11;	/* [10..0] */
		unsigned int Reserved_74:21;	/* [31..11] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_TX_FRM_MAX_SIZE;

/* Define the union U_TX_SMP_FRM_REQ */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_rxtimer_limit:8;	/* [7..0] */
		unsigned int Reserved_75:24;	/* [31..8] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_TX_SMP_FRM_REQ;

/* Define the union U_TX_SSP_FRM_REQ */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_tx_cmd:1;	/* [0] */
		unsigned int cfg_tx_task:1;	/* [1] */
		unsigned int cfg_tx_data:1;	/* [2] */
		unsigned int cfg_tx_smp:1;	/* [3] */
		unsigned int Reserved_77:28;	/* [31..4] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_TX_SSP_FRM_REQ;

/* Define the union U_TRANS_TXRX_STATUS */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int sl_opaf_rxd_conf:1;	/* [0] */
		unsigned int st_rx_xrdy_frm:1;	/* [1] */
		unsigned int dma_rx_io_end:1;	/* [2] */
		unsigned int trans_tx_complete:1;	/* [3] */
		unsigned int trans_tx_fail:1;	/* [4] */
		unsigned int dma_tx_error:1;	/* [5] */
		unsigned int Reserved_79:26;	/* [31..6] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_TRANS_TXRX_STATUS;

/* Define the union U_IO_END_STATUS */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int dmac_io_rx_end_iptt:16;	/* [15..0] */
		unsigned int dmac_io_rx_end_with_resp:1;	/* [16] */
		unsigned int dmac_io_rx_resp_good:1;	/* [17] */
		unsigned int dmac_io_rx_end_with_err:1;	/* [18] */
		unsigned int Reserved_81:13;	/* [31..19] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_IO_END_STATUS;

/* Define the union U_HGC_IO_TX_CONFIG_R */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_tx_iptt:16;	/* [15..0] */
		unsigned int cfg_verify_data_trans_len_en:1;	/* [16] */
		unsigned int cfg_dif_present:1;	/* [17] */
		unsigned int cfg_tx_sgl_mode:1;	/* [18] */
		unsigned int cfg_sgl_sge:1;	/* [19] */
		unsigned int cfg_retrans_xrdy_data:1;	/* [20] */
		unsigned int cfg_cur_tx_is_tlr:1;	/* [21] */
		unsigned int Reserved_82:10;	/* [31..22] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_HGC_IO_TX_CONFIG_R;

/* Define the union U_HGC_IO_CMD_CONFIG_R */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_pass_through:1;	/* [0] */
		unsigned int cfg_cmd_task_iu_size:9;	/* [9..1] */
		unsigned int cfg_ssp_first_burst_en:1;	/* [10] */
		unsigned int cfg_ssp_io_dir:2;	/* [12..11] */
		unsigned int cfg_cur_target_tlr_en:1;	/* [13] */
		unsigned int Reserved_84:18;	/* [31..14] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_HGC_IO_CMD_CONFIG_R;

/* Define the union U_HGC_IO_SGL_LENGTH */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_dif_sgl_length:16;	/* [15..0] */
		unsigned int cfg_data_sgl_length:16;	/* [31..16] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_HGC_IO_SGL_LENGTH;

/* Define the union U_DMA_TX_ERR_STATUS */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int dma_tx_crc_err:1;	/* [0] */
		unsigned int dma_tx_app_err:1;	/* [1] */
		unsigned int dma_tx_ref_err:1;	/* [2] */
		unsigned int dma_tx_sgl_overflow:1;	/* [3] */
		unsigned int dma_tx_resp_err:1;	/* [4] */
		unsigned int dma_tx_io_data_len_less_err:1;	/* [5] */
		unsigned int dma_tx_offset_err:1;	/* [6] */
		unsigned int Reserved_86:25;	/* [31..7] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_DMA_TX_ERR_STATUS;

/* Define the union U_DMA_TX_STATUS */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int dma_tx_busy:1;	/* [0] */
		unsigned int Reserved_88:31;	/* [31..1] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_DMA_TX_STATUS;

/* Define the union U_DMA_RX_ERR_STATUS */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int dma_rx_sgl_overflow:1;	/* [0] */
		unsigned int dma_rx_crc_err:1;	/* [1] */
		unsigned int dma_rx_app_err:1;	/* [2] */
		unsigned int dma_rx_ref_err:1;	/* [3] */
		unsigned int dma_rx_offset_err:1;	/* [4] */
		unsigned int dma_rx_resp_err:1;	/* [5] */
		unsigned int dma_rx_io_data_len_exceed_err:1;	/* [6] */
		unsigned int dma_rx_io_data_len_less_err:1;	/* [7] */
		unsigned int dmac_err_ram_num:2;	/* [9..8] */
		unsigned int dmac_ram_ecc_err_addr:5;	/* [14..10] */
		unsigned int Reserved_90:17;	/* [31..15] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_DMA_RX_ERR_STATUS;

/* Define the union U_HGC_DMA_TX_FAIL_PROC_CFG */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_dma_send_err_infor:1;	/* [0] */
		unsigned int cfg_dma_not_send_err_infor:1;	/* [1] */
		unsigned int Reserved_92:30;	/* [31..2] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_HGC_DMA_TX_FAIL_PROC_CFG;

/* Define the union U_DMA_RX_STATUS */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int dma_rx_err:1;	/* [0] */
		unsigned int dma_rx_busy:1;	/* [1] */
		unsigned int Reserved_93:30;	/* [31..2] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_DMA_RX_STATUS;

/* Define the union U_HGC_IO_RESP_CONFIG_R */
typedef union {
	/* Define the struct bits */
	struct {
		unsigned int cfg_ssp_send_response:1;	/* [0] */
		unsigned int cfg_max_resp_length:9;	/* [9..1] */
		unsigned int Reserved_96:22;	/* [31..10] */
	} bits;

	/* Define an unsigned member */
	unsigned int u32;

} U_HGC_IO_RESP_CONFIG_R;

#endif