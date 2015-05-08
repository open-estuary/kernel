/*
 * copyright (C) huawei
 */

#ifndef _HIGGS_PHY_H
#define _HIGGS_PHY_H

#define HIGGS_ENDIAN_REVERT_32(x) \
	( ((((x) >> 24) & 0xff) << 0) \
	| ((((x) >> 16) & 0xff) << 8) \
	| ((((x) >> 8) & 0xff) << 16) \
	| ((((x) >> 0) & 0xff) << 24) \
	)

#define HIGGS_ENDIAN_REVERT_16(x) \
	( ((((x) >> 8) & 0xff) << 0) \
	| ((((x) >> 0) & 0xff) << 8) \
	)

#define     HIGGS_TGT_RESTRICTED            0x0
#define 	HIGGS_INI_SSP_STP_SMP  			0xe
#define 	HIGGS_MANAGE_PROTOCOL_IN 		0x10
#define     HIGGS_DEFAULT_SAS_ADDR          0xA002030405060708ULL

#define     HIGGS_12G_NEGO_QUERY_WINDOW     40	/* 12G rate negotiation query window,MS */

enum higgs_phy_cfg_rate {
	HIGGS_PHY_CFG_RATE_1_5_G = 1,
	HIGGS_PHY_CFG_RATE_3_0_G,
	HIGGS_PHY_CFG_RATE_6_0_G,
	HIGGS_PHY_CFG_RATE_12_0_G,
	HIGGS_PHY_CFG_RATE_BUTT
};

enum higgs_cfg_phy {
	HIGGS_CFG_PHY_DISABLE = 0,
	HIGGS_CFG_PHY_ENABLE,
	HIGGS_CFG_PHY_BUTT
};

enum higgs_cfg_phy_op_mode {
	HIGGS_CFG_PHY_OPT_MODE_OPTICAL = 0,
	HIGGS_CFG_PHY_OPT_MODE_DC,
	HIGGS_CFG_PHY_OPT_MODE_BUTT
};

enum higgs_cfg_prog_oob_phy_link_rate {
	HIGGS_CFG_PROG_OOB_PHY_LINK_RATE_1_5_G = 0x8,
	HIGGS_CFG_PROG_OOB_PHY_LINK_RATE_3_0_G,
	HIGGS_CFG_PROG_OOB_PHY_LINK_RATE_6_0_G,
	HIGGS_CFG_PROG_OOB_PHY_LINK_RATE_12_0_G,
	HIGGS_CFG_PROG_OOB_PHY_LINK_RATE_BUTT
};

enum higgs_cfg_prog_min_phy_link_rate {
	HIGGS_CFG_PROG_MIN_PHY_LINK_RATE_1_5_G = 0x8,
	HIGGS_CFG_PROG_MIN_PHY_LINK_RATE_3_0_G,
	HIGGS_CFG_PROG_MIN_PHY_LINK_RATE_6_0_G,
	HIGGS_CFG_PROG_MIN_PHY_LINK_RATE_12_0_G,
	HIGGS_CFG_PROG_MIN_PHY_LINK_RATE_BUTT
};

enum higgs_cfg_prog_max_phy_link_rate {
	HIGGS_CFG_PROG_MAX_PHY_LINK_RATE_1_5_G = 0x8,
	HIGGS_CFG_PROG_MAX_PHY_LINK_RATE_3_0_G,
	HIGGS_CFG_PROG_MAX_PHY_LINK_RATE_6_0_G,
	HIGGS_CFG_PROG_MAX_PHY_LINK_RATE_12_0_G =
	    HIGGS_CFG_PROG_MAX_PHY_LINK_RATE_6_0_G,
	HIGGS_CFG_PROG_MAX_PHY_LINK_RATE_BUTT
};

enum higgs_hard_phy_nego_link_rate {
	HIGGS_HARD_PHY_NEGO_LINK_RATE_1_5_G = 0x8,
	HIGGS_HARD_PHY_NEGO_LINK_RATE_3_0_G,
	HIGGS_HARD_PHY_NEGO_LINK_RATE_6_0_G,
	HIGGS_HARD_PHY_NEGO_LINK_RATE_12_0_G,
	HIGGS_HARD_PHY_NEGO_LINK_RATE_BUTT
};

enum higgs_phy_pcn_pcb {
	HIGGS_PHY_PCN_PCB_1_5_G = 0x80800000,	/* G1 without SSC supported  */
	HIGGS_PHY_PCN_PCB_3_0_G = 0x80A00001,	/* G1 and G2 without SSC supported */
	HIGGS_PHY_PCN_PCB_6_0_G = 0x80A80000,	/* G1,G2, and G3 without SSC supported */
	HIGGS_PHY_PCN_PCB_12_0_G = 0x80AA0001	/* G1,G2,G3, and G4 without SSC supported */
};

enum higgs_identify_addr_frame_reason {
	HIGGS_IDENTIFY_ADDRESS_FRAME_REASON_UNKONWN = 0,
	HIGGS_IDENTIFY_ADDRESS_FRAME_REASON_POWER_ON = 1,
	HIGGS_IDENTIFY_ADDRESS_FRAME_REASON_HARD_RESET = 2,
	HIGGS_IDENTIFY_ADDRESS_FRAME_REASON_SMP_CONTROL_RESET = 3,
	HIGGS_IDENTIFY_ADDRESS_FRAME_REASON_LOSS_DWORD_SYNC = 4,
	HIGGS_IDENTIFY_ADDRESS_FRAME_REASON_MUX_RECEIVE = 5,
	HIGGS_IDENTIFY_ADDRESS_FRAME_REASON_I_T_NEXUS_LOSS_TIMER = 6,
	HIGGS_IDENTIFY_ADDRESS_FRAME_REASON_BREAK_TIMEOUT = 7,
	HIGGS_IDENTIFY_ADDRESS_FRAME_REASON_PHY_TEST_STOPPED = 8,
	HIGGS_IDENTIFY_ADDRESS_FRAME_REASON_EXPANDER_REDUCED_FUNC = 9,
	HIGGS_IDENTIFY_ADDRESS_FRAME_REASON_BUTT
};

extern s32 higgs_phy_operation(struct sal_card *sal_card,
			       enum sal_phy_op_type phy_opt, void *argc_in);

extern s32 higgs_start_phy(struct higgs_card *ll_card,
			   u32 phy_id, u64 ll_sasaddr, u32 phy_rate);

extern s32 higgs_stop_phy(struct higgs_card *ll_card, u32 phy_id);

extern void higgs_close_all_phy(struct higgs_card *ll_card);

extern s32 higgs_wait_for_stop_phy(struct higgs_card *ll_card, u32 phy_id);

extern s32 higgs_init_serdes_param(struct higgs_card *ll_card);

extern void higgs_open_phy_own_wire(struct sal_card *sal_card);

extern enum sal_link_rate higgs_get_sal_phy_rate(u32 higgs_rate);

extern s32 higgs_init_identify_frame(struct higgs_card *ll_card);

extern s32 higgs_config_identify_frame(struct higgs_card *ll_card,
				       u32 phy_id, u64 ll_sasaddr);

extern bool higgs_is_phy_disabled(struct higgs_card *ll_card, u32 phy_id);

extern s32 higgs_config_serdes_for_12g(struct higgs_card *ll_card, u32 phy_id);

extern void higgs_serdes_for_12g_timer_hander(unsigned long arg);

extern void higgs_serdes_lane_reset(struct higgs_card *ll_card, u32 phy_id);

extern void higgs_serdes_enable_ctledfe(struct higgs_card *ll_card, u32 phy_id);

extern s32 higgs_stop_serdes_fw_timer(struct higgs_card *ll_card, u32 phy_id);

extern s32 higgs_clear_tx_training(struct higgs_card *ll_card, u32 phy_id);

extern s32 higgs_end_tx_training(struct higgs_card *ll_card, u32 phy_id);

extern s32 higgs_config_oob_link_rate(struct higgs_card *ll_card, u32 phy_id,
				      u32 oob_link_rate);

extern void higgs_check_dma_txrx(unsigned long arg);
extern s32 higgs_notify_sal_phy_stop_rsp(struct higgs_card *ll_card,
					 u32 phy_id);

extern s32 higgs_notify_sal_phy_down(struct higgs_card *ll_card, u32 phy_id);

#endif
