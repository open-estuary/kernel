/*
 * copyright (C) huawei
 */

#ifndef _HIGGS_INTR_H_
#define _HIGGS_INTR_H_

/*#define HIGGS_OQ_INT_MASK (0x0)*/
#define HIGGS_OQ_INT_MASK      (0xFFFFFFFF)
#define HIGGS_ENT_INT_SRC_MSK1 (0x0FFFFFFF)
#define HIGGS_ENT_INT_SRC_MSK2 (0xFFFFFFFF)

#define SAS_DSAF_MSI_INT_ID_START  (13184)
#define SAS_PCIE_MSI_INT_ID_START  (14080)
#define SAS_CQ_MSI_INT_ID_OFFSET   (80)
#define SAS_DSAF_MSI_CQ_INT(queue) ((queue) + SAS_DSAF_MSI_INT_ID_START + SAS_CQ_MSI_INT_ID_OFFSET)
#define SAS_PCIE_MSI_CQ_INT(queue) ((queue) + SAS_PCIE_MSI_INT_ID_START + SAS_CQ_MSI_INT_ID_OFFSET)

/* add by chenqilin for debug 2p arm server*/
#if defined(PV660_ARM_SERVER)
/* sas 0 */
#define SLAVE_SAS_DSAF_MSI_INT_ID_START  (21502)
#define SLAVE_SAS_DSAF_MSI_CQ_INT(queue) ((queue) + SLAVE_SAS_DSAF_MSI_INT_ID_START + SAS_CQ_MSI_INT_ID_OFFSET)
/* sas1 */
#define SLAVE_SAS_PCIE_MSI_INT_ID_START  (22272)
#define SLAVE_SAS_PCIE_MSI_CQ_INT(queue) ((queue) + SLAVE_SAS_PCIE_MSI_INT_ID_START + SAS_CQ_MSI_INT_ID_OFFSET)
#endif
/* end */

/* evb || c05 
#define SAS0_MSI_INT_ID_START  (13184)
#define SAS1_MSI_INT_ID_START  (14080)
#define MSI_DQ_ID_OFFSET (80)
#define SAS1_MSI_DQ_INT(queue) ((queue) + SAS1_MSI_INT_ID_START + MSI_DQ_ID_OFFSET)
#define SAS0_MSI_DQ_INT(queue) ((queue) + SAS0_MSI_INT_ID_START + MSI_DQ_ID_OFFSET)
#endif
*/

/*MSI interrupt number*/
/*#define MSI_HGC_XFER_RDY_INT_ID_OFFSET    (80)*/
#define MSI_HGC_ECC_INT_ID_OFFSET         (120)
#define MSI_HGC_AXI_INT_ID_OFFSET         (125)

#define HIGGS_PHY_ATTACH_CPU_ID  0

#define HIGGS_SAS_PORT_PHY_CHL_INTR_MASK0_ALL            0x003FFFFF
#define HIGGS_SAS_PORT_PHY_CHL_INTR_MASK1_ALL            0x0001FFFF
#define HIGGS_SAS_PORT_PHY_CHL_INTR_MASK2_ALL            0x000003FF

/* need check LOSS DWORD SYNC. START */
#define HIGGS_SAS_PORT_PHY_CHL_INTR_MASK0_DEFAULT        0x003CE3EE
/* need check LOSS DWORD SYNC. END */
#define HIGGS_SAS_PORT_PHY_CHL_INTR_MASK1_DEFAULT        0x00017FFF
#define HIGGS_SAS_PORT_PHY_CHL_INTR_MASK2_DEFAULT        0x0000032A

#define CHANNEL_INT_COUNT (10)
#define MSI_CHANNEL_OFFSET(PhyId) (CHANNEL_INT_COUNT * (PhyId))

typedef irqreturn_t(*PHY_INT_ISR) (s32 uiIsrNum, void *Param);

#if 0
typedef struct higgs_int_info {
	u32 irq_num;
	char intr_vec_name[HIGGS_MAX_INTRNAME_LEN];
	 irqreturn_t(*int_handler) (u32 irq, void *dev_id);
} struct higgs_int_info;
#endif

#define MSI_INT_HANDLER_NAME(origin, id) (origin##id)

#define MSI_INT_HANDLER(func, id)   \
    irqreturn_t MSI_INT_HANDLER_NAME(func, id)(s32 iIrqNum,void *Param) \
    {   \
		HIGGS_REF(iIrqNum);\
        return func(id, Param); \
    }

#if 1
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_bcast, 0) (s32 iIsrNum,
							     void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_bcast, 1) (s32 iIsrNum,
							     void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_bcast, 2) (s32 iIsrNum,
							     void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_bcast, 3) (s32 iIsrNum,
							     void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_bcast, 4) (s32 iIsrNum,
							     void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_bcast, 5) (s32 iIsrNum,
							     void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_bcast, 6) (s32 iIsrNum,
							     void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_bcast, 7) (s32 iIsrNum,
							     void *Param);
#endif
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_up, 0) (s32 iIsrNum,
							  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_up, 1) (s32 iIsrNum,
							  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_up, 2) (s32 iIsrNum,
							  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_up, 3) (s32 iIsrNum,
							  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_up, 4) (s32 iIsrNum,
							  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_up, 5) (s32 iIsrNum,
							  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_up, 6) (s32 iIsrNum,
							  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_up, 7) (s32 iIsrNum,
							  void *Param);

#if 1
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_abnormal, 0) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_abnormal, 1) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_abnormal, 2) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_abnormal, 3) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_abnormal, 4) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_abnormal, 5) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_abnormal, 6) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_abnormal, 7) (s32 iIsrNum,
								void *Param);

extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_ctrl_rdy, 0) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_ctrl_rdy, 1) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_ctrl_rdy, 2) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_ctrl_rdy, 3) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_ctrl_rdy, 4) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_ctrl_rdy, 5) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_ctrl_rdy, 6) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_ctrl_rdy, 7) (s32 iIsrNum,
								void *Param);

extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_dma_err, 0) (s32 iIsrNum,
							       void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_dma_err, 1) (s32 iIsrNum,
							       void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_dma_err, 2) (s32 iIsrNum,
							       void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_dma_err, 3) (s32 iIsrNum,
							       void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_dma_err, 4) (s32 iIsrNum,
							       void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_dma_err, 5) (s32 iIsrNum,
							       void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_dma_err, 6) (s32 iIsrNum,
							       void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_dma_err, 7) (s32 iIsrNum,
							       void *Param);

extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_hot_plug_tout,
					0) (s32 iIsrNum, void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_hot_plug_tout,
					1) (s32 iIsrNum, void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_hot_plug_tout,
					2) (s32 iIsrNum, void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_hot_plug_tout,
					3) (s32 iIsrNum, void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_hot_plug_tout,
					4) (s32 iIsrNum, void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_hot_plug_tout,
					5) (s32 iIsrNum, void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_hot_plug_tout,
					6) (s32 iIsrNum, void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_hot_plug_tout,
					7) (s32 iIsrNum, void *Param);

extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_oob_restart, 0) (s32 iIsrNum,
								   void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_oob_restart, 1) (s32 iIsrNum,
								   void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_oob_restart, 2) (s32 iIsrNum,
								   void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_oob_restart, 3) (s32 iIsrNum,
								   void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_oob_restart, 4) (s32 iIsrNum,
								   void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_oob_restart, 5) (s32 iIsrNum,
								   void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_oob_restart, 6) (s32 iIsrNum,
								   void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_oob_restart, 7) (s32 iIsrNum,
								   void *Param);

extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_ph_rx_hard_rst, 0) (s32 iIsrNum,
								  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_ph_rx_hard_rst, 1) (s32 iIsrNum,
								  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_ph_rx_hard_rst, 2) (s32 iIsrNum,
								  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_ph_rx_hard_rst, 3) (s32 iIsrNum,
								  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_ph_rx_hard_rst, 4) (s32 iIsrNum,
								  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_ph_rx_hard_rst, 5) (s32 iIsrNum,
								  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_ph_rx_hard_rst, 6) (s32 iIsrNum,
								  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_ph_rx_hard_rst, 7) (s32 iIsrNum,
								  void *Param);

extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_status_chg, 0) (s32 iIsrNum,
								  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_status_chg, 1) (s32 iIsrNum,
								  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_status_chg, 2) (s32 iIsrNum,
								  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_status_chg, 3) (s32 iIsrNum,
								  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_status_chg, 4) (s32 iIsrNum,
								  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_status_chg, 5) (s32 iIsrNum,
								  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_status_chg, 6) (s32 iIsrNum,
								  void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_status_chg, 7) (s32 iIsrNum,
								  void *Param);

extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_int_reg1, 0) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_int_reg1, 1) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_int_reg1, 2) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_int_reg1, 3) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_int_reg1, 4) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_int_reg1, 5) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_int_reg1, 6) (s32 iIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_phy_int_reg1, 7) (s32 iIsrNum,
								void *Param);

extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 0) (s32 ulIsrNum,
							       void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 1) (s32 ulIsrNum,
							       void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 2) (s32 ulIsrNum,
							       void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 3) (s32 ulIsrNum,
							       void *Param);

extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 4) (s32 ulIsrNum,
							       void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 5) (s32 ulIsrNum,
							       void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 6) (s32 ulIsrNum,
							       void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 7) (s32 ulIsrNum,
							       void *Param);

extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 8) (s32 ulIsrNum,
							       void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 9) (s32 ulIsrNum,
							       void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 10) (s32 ulIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 11) (s32 ulIsrNum,
								void *Param);

extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 12) (s32 ulIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 13) (s32 ulIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 14) (s32 ulIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 15) (s32 ulIsrNum,
								void *Param);

extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 16) (s32 ulIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 17) (s32 ulIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 18) (s32 ulIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 19) (s32 ulIsrNum,
								void *Param);

extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 20) (s32 ulIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 21) (s32 ulIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 22) (s32 ulIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 23) (s32 ulIsrNum,
								void *Param);

extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 24) (s32 ulIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 25) (s32 ulIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 26) (s32 ulIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 27) (s32 ulIsrNum,
								void *Param);

extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 28) (s32 ulIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 29) (s32 ulIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 30) (s32 ulIsrNum,
								void *Param);
extern irqreturn_t MSI_INT_HANDLER_NAME(higgs_oq_core_int, 31) (s32 ulIsrNum,
								void *Param);

#endif

/*chip fatat interrupt processing function*/
extern irqreturn_t higgs_chip_fatal_hgc_ecc_int_proc(s32 chip_fatal_id,
						     void *param);
extern irqreturn_t higgs_chip_fatal_hgc_axi_int_proc(s32 chip_fatal_id,
						     void *param);
/*
extern irqreturn_t Higgs_ChipFatalXferRdyIntProc(u32 uiChipFatalId, void* Param);
extern void Higgs_FreeChipFatalIrqInit(struct higgs_card* v_pstLLCard);
*/

extern s32 higgs_init_intr(struct higgs_card *ll_card);
extern void higgs_open_all_intr(struct higgs_card *ll_card);
extern void higgs_close_all_intr(struct higgs_card *ll_card);
extern void higgs_release_intr(struct higgs_card *ll_card);
extern void higgs_phy_clear_intr_status(struct higgs_card *ll_card, u32 phy_id);
extern void higgs_phy_mask_intr(struct higgs_card *ll_card, u32 phy_id);
extern void higgs_phy_unmask_intr(struct higgs_card *ll_card, u32 phy_id);
extern bool higgs_phy_down_intr_come(struct higgs_card *ll_card, u32 phy_id);
extern void higgs_phy_clear_down_intr_status(struct higgs_card *ll_card,
					     u32 phy_id);
extern void higgs_simulate_phy_stop_rsp_intr(struct higgs_card *ll_card,
					     u32 phy_id);
extern void higgs_simulate_wire_hotplug_intr(struct higgs_card *ll_card,
					     u32 port_id, u32 timeout_ms);

#endif
