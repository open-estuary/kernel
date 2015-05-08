/*
 * Copyright (C) huawei
 * The program is used to process intrerrupt
 */

#include "higgs_common.h"
#include "higgs_pv660.h"
#include "higgs_phy.h"
#include "higgs_port.h"
#include "higgs_intr.h"
#include "higgs_init.h"
#ifdef _PCLINT_
#else

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)	/* < linux 3.19 */
#include <linux/irqchip/hisi-msi-gen.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#else
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#endif

#endif

extern void higgs_oq_int_process(struct higgs_card *higgs_card, u32 queue_id);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)	/* < linux 3.19 */

extern s32 ic_enable_msi(s32 hwirq, s32 * virq);
extern void ic_disable_msi(s32 iVIrq);
#endif

#if defined(PV660_ARM_SERVER)
extern void higgs_oper_led_by_event(struct higgs_card *ll_card,
				    struct higgs_led_event_notify
				    *led_event_data);
#endif

static s32 higgs_init_phy_irq(struct higgs_card *ll_card);
static s32 higgs_init_cq_irq(struct higgs_card *ll_card);
static s32 higgs_init_chip_fatal_irq(struct higgs_card *ll_card);
static void higgs_free_phy_irq(struct higgs_card *ll_card);
static void higgs_free_cq_irq(struct higgs_card *ll_card);
static void higgs_free_chip_fatal_irq(struct higgs_card *ll_card);
static void higgs_phy_down(struct higgs_card *ll_card, u32 phy_id);
static void higgs_phy_identify_timeout(struct higgs_card *ll_card, u32 phy_id);
static void higgs_phy_loss_dw_sync(struct higgs_card *ll_card, u32 phy_id);
static void higgs_check_address_frame_error(struct higgs_card *ll_card,
					    u32 phy_id);
static void higgs_bypass_chip_bug_mask_abnormal_intr(struct higgs_card *ll_card,
						     u32 phy_id);
static void higgs_bypass_chip_bug_unmask_abnormal_intr(struct higgs_card
						       *ll_card, u32 phy_id);
#if 0
static s32 higgs_sp804_timer_get_resource(struct higgs_card *ll_card);
static irqreturn_t higgs_sp804_timer_intr_handle(s32 irq_num, void *param);
static void higgs_sp804_timer_trigger(struct higgs_card *ll_card,
				      u32 timeout_us);
static void higgs_sp804_timer_clear_intr_status(struct higgs_card *ll_card);
static void higgs_sp804_timer_unmask_intr(struct higgs_card *ll_card);
static void higgs_sp804_timer_mask_intr(struct higgs_card *ll_card);
#endif


PHY_INT_ISR g_pfnIntIsr[MSI_PHY_BUTT][HIGGS_MAX_PHY_NUM] = {
	/* mask interrupt of the null callback function*/
	[MSI_PHY_CTRL_RDY] = {
			      MSI_INT_HANDLER_NAME(higgs_phy_ctrl_rdy, 0),
			      MSI_INT_HANDLER_NAME(higgs_phy_ctrl_rdy, 1),
			      MSI_INT_HANDLER_NAME(higgs_phy_ctrl_rdy, 2),
			      MSI_INT_HANDLER_NAME(higgs_phy_ctrl_rdy, 3),
			      MSI_INT_HANDLER_NAME(higgs_phy_ctrl_rdy, 4),
			      MSI_INT_HANDLER_NAME(higgs_phy_ctrl_rdy, 5),
			      MSI_INT_HANDLER_NAME(higgs_phy_ctrl_rdy, 6),
			      MSI_INT_HANDLER_NAME(higgs_phy_ctrl_rdy, 7),
			      },
	[MSI_PHY_DMA_RESP_ERR] = {
				  MSI_INT_HANDLER_NAME(higgs_phy_dma_err, 0),
				  MSI_INT_HANDLER_NAME(higgs_phy_dma_err, 1),
				  MSI_INT_HANDLER_NAME(higgs_phy_dma_err, 2),
				  MSI_INT_HANDLER_NAME(higgs_phy_dma_err, 3),
				  MSI_INT_HANDLER_NAME(higgs_phy_dma_err, 4),
				  MSI_INT_HANDLER_NAME(higgs_phy_dma_err, 5),
				  MSI_INT_HANDLER_NAME(higgs_phy_dma_err, 6),
				  MSI_INT_HANDLER_NAME(higgs_phy_dma_err, 7),
				  },
	[MSI_PHY_HOTPLUG_TOUT] = {
				  MSI_INT_HANDLER_NAME(higgs_phy_hot_plug_tout,
						       0),
				  MSI_INT_HANDLER_NAME(higgs_phy_hot_plug_tout,
						       1),
				  MSI_INT_HANDLER_NAME(higgs_phy_hot_plug_tout,
						       2),
				  MSI_INT_HANDLER_NAME(higgs_phy_hot_plug_tout,
						       3),
				  MSI_INT_HANDLER_NAME(higgs_phy_hot_plug_tout,
						       4),
				  MSI_INT_HANDLER_NAME(higgs_phy_hot_plug_tout,
						       5),
				  MSI_INT_HANDLER_NAME(higgs_phy_hot_plug_tout,
						       6),
				  MSI_INT_HANDLER_NAME(higgs_phy_hot_plug_tout,
						       7),
				  },
	[MSI_PHY_BCAST_ACK] = {
			       /*broadcast interrupt function*/
			       MSI_INT_HANDLER_NAME(higgs_phy_bcast, 0),
			       MSI_INT_HANDLER_NAME(higgs_phy_bcast, 1),
			       MSI_INT_HANDLER_NAME(higgs_phy_bcast, 2),
			       MSI_INT_HANDLER_NAME(higgs_phy_bcast, 3),
			       MSI_INT_HANDLER_NAME(higgs_phy_bcast, 4),
			       MSI_INT_HANDLER_NAME(higgs_phy_bcast, 5),
			       MSI_INT_HANDLER_NAME(higgs_phy_bcast, 6),
			       MSI_INT_HANDLER_NAME(higgs_phy_bcast, 7),
			       },
	[MSI_PHY_OOB_RESTART] = {
				 MSI_INT_HANDLER_NAME(higgs_phy_oob_restart, 0),
				 MSI_INT_HANDLER_NAME(higgs_phy_oob_restart, 1),
				 MSI_INT_HANDLER_NAME(higgs_phy_oob_restart, 2),
				 MSI_INT_HANDLER_NAME(higgs_phy_oob_restart, 3),
				 MSI_INT_HANDLER_NAME(higgs_phy_oob_restart, 4),
				 MSI_INT_HANDLER_NAME(higgs_phy_oob_restart, 5),
				 MSI_INT_HANDLER_NAME(higgs_phy_oob_restart, 6),
				 MSI_INT_HANDLER_NAME(higgs_phy_oob_restart, 7),
				 },
	[MSI_PHY_RX_HARDRST] = {
				MSI_INT_HANDLER_NAME(higgs_ph_rx_hard_rst, 0),
				MSI_INT_HANDLER_NAME(higgs_ph_rx_hard_rst, 1),
				MSI_INT_HANDLER_NAME(higgs_ph_rx_hard_rst, 2),
				MSI_INT_HANDLER_NAME(higgs_ph_rx_hard_rst, 3),
				MSI_INT_HANDLER_NAME(higgs_ph_rx_hard_rst, 4),
				MSI_INT_HANDLER_NAME(higgs_ph_rx_hard_rst, 5),
				MSI_INT_HANDLER_NAME(higgs_ph_rx_hard_rst, 6),
				MSI_INT_HANDLER_NAME(higgs_ph_rx_hard_rst, 7),
				},
	[MSI_PHY_STATUS_CHG] = {
				MSI_INT_HANDLER_NAME(higgs_phy_status_chg, 0),
				MSI_INT_HANDLER_NAME(higgs_phy_status_chg, 1),
				MSI_INT_HANDLER_NAME(higgs_phy_status_chg, 2),
				MSI_INT_HANDLER_NAME(higgs_phy_status_chg, 3),
				MSI_INT_HANDLER_NAME(higgs_phy_status_chg, 4),
				MSI_INT_HANDLER_NAME(higgs_phy_status_chg, 5),
				MSI_INT_HANDLER_NAME(higgs_phy_status_chg, 6),
				MSI_INT_HANDLER_NAME(higgs_phy_status_chg, 7),
				},
	[MSI_PHY_SL_PHY_ENABLED] = {
				    /*phy up interrupt function*/
				    MSI_INT_HANDLER_NAME(higgs_phy_up, 0),
				    MSI_INT_HANDLER_NAME(higgs_phy_up, 1),
				    MSI_INT_HANDLER_NAME(higgs_phy_up, 2),
				    MSI_INT_HANDLER_NAME(higgs_phy_up, 3),
				    MSI_INT_HANDLER_NAME(higgs_phy_up, 4),
				    MSI_INT_HANDLER_NAME(higgs_phy_up, 5),
				    MSI_INT_HANDLER_NAME(higgs_phy_up, 6),
				    MSI_INT_HANDLER_NAME(higgs_phy_up, 7),
				    },
	[MSI_PHY_INT_REG0] = {
			      /* phy down/identify tiemout/phy error interrupt function*/
			      MSI_INT_HANDLER_NAME(higgs_phy_abnormal, 0),
			      MSI_INT_HANDLER_NAME(higgs_phy_abnormal, 1),
			      MSI_INT_HANDLER_NAME(higgs_phy_abnormal, 2),
			      MSI_INT_HANDLER_NAME(higgs_phy_abnormal, 3),
			      MSI_INT_HANDLER_NAME(higgs_phy_abnormal, 4),
			      MSI_INT_HANDLER_NAME(higgs_phy_abnormal, 5),
			      MSI_INT_HANDLER_NAME(higgs_phy_abnormal, 6),
			      MSI_INT_HANDLER_NAME(higgs_phy_abnormal, 7),
			      },
	[MSI_PHY_INT_REG1] = {
			      MSI_INT_HANDLER_NAME(higgs_phy_int_reg1, 0),
			      MSI_INT_HANDLER_NAME(higgs_phy_int_reg1, 1),
			      MSI_INT_HANDLER_NAME(higgs_phy_int_reg1, 2),
			      MSI_INT_HANDLER_NAME(higgs_phy_int_reg1, 3),
			      MSI_INT_HANDLER_NAME(higgs_phy_int_reg1, 4),
			      MSI_INT_HANDLER_NAME(higgs_phy_int_reg1, 5),
			      MSI_INT_HANDLER_NAME(higgs_phy_int_reg1, 6),
			      MSI_INT_HANDLER_NAME(higgs_phy_int_reg1, 7),
			      },
};

PHY_INT_ISR g_pfnCQIntIsr[HIGGS_MAX_CQ_NUM] = {
	MSI_INT_HANDLER_NAME(higgs_oq_core_int, 0),
	MSI_INT_HANDLER_NAME(higgs_oq_core_int, 1),
	MSI_INT_HANDLER_NAME(higgs_oq_core_int, 2),
	MSI_INT_HANDLER_NAME(higgs_oq_core_int, 3),
};

PHY_INT_ISR g_pfnChipFatalIntIsr[MSI_CHIP_FATAL_BUTT] = {
	higgs_chip_fatal_hgc_ecc_int_proc,
	higgs_chip_fatal_hgc_axi_int_proc,  
};
#if 0
static struct of_device_id s_stClkSrcForSasDsaf = {
	.name = "",
	.type = "",
	.compatible = "arm,sp804-3",
	.data = NULL,
};

static struct of_device_id s_stClkSrcForSasPcie = {
	.name = "",
	.type = "",
	.compatible = "arm,sp804-4",
	.data = NULL,
};
#endif

/*
 * hardware interrupt close
 */
void higgs_close_all_intr(struct higgs_card *ll_card)
{
	u32 phy_id = 0;
	u32 reg_value = 0;

	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		/* clear all phy interrupt*/
		higgs_phy_clear_intr_status(ll_card, phy_id);

		/* mask all phy*/
		higgs_phy_mask_intr(ll_card, phy_id);
	}

	/*clear OQ interrupt status*/
	HIGGS_GLOBAL_REG_WRITE(ll_card, HISAS30HV100_GLOBAL_REG_OQ_INT_SRC_REG,
			       0xffffffff);

#if 0
	reg_value =
	    HIGGS_GLOBAL_REG_READ(v_pstLLCard,
				  HISAS30HV100_GLOBAL_REG_OQ_INT_SRC_MSK_REG);
	reg_value = reg_value | (~HIGGS_OQ_INT_MASK);
#else
	reg_value = HIGGS_OQ_INT_MASK;
#endif
	/* CodeCC */
	HIGGS_GLOBAL_REG_WRITE(ll_card,
			       HISAS30HV100_GLOBAL_REG_OQ_INT_SRC_MSK_REG,
			       reg_value);

	/* close the interrupt of event no.1*/
	reg_value =
	    HIGGS_GLOBAL_REG_READ(ll_card,
				  HISAS30HV100_GLOBAL_REG_ENT_INT_SRC_MSK1_REG);
	reg_value = reg_value | HIGGS_ENT_INT_SRC_MSK1;
	HIGGS_GLOBAL_REG_WRITE(ll_card,
			       HISAS30HV100_GLOBAL_REG_ENT_INT_SRC_MSK1_REG,
			       reg_value);

#if 0
	reg_value =
	    HIGGS_GLOBAL_REG_READ(v_pstLLCard,
				  HISAS30HV100_GLOBAL_REG_ENT_INT_SRC_MSK2_REG);
	reg_value = reg_value | HIGGS_ENT_INT_SRC_MSK2;
#else
	reg_value = HIGGS_ENT_INT_SRC_MSK2;
#endif
	/* CodeCC */
	HIGGS_GLOBAL_REG_WRITE(ll_card,
			       HISAS30HV100_GLOBAL_REG_ENT_INT_SRC_MSK2_REG,
			       reg_value);

	return;
}

/*
 * open hardware interrupt
 */
void higgs_open_all_intr(struct higgs_card *ll_card)
{
	u32 phy_id = 0;

	/*open phy interrupt*/
	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		/* clear interrupt status*/
		higgs_phy_clear_intr_status(ll_card, phy_id);

		/* unmask phy interrupt */
		higgs_phy_unmask_intr(ll_card, phy_id);

		/* phy down时, avoid to report Abnormal interrupt*/
		higgs_bypass_chip_bug_mask_abnormal_intr(ll_card, phy_id);
	}

	return;
}

/*
 * interrupt  initialization 
 */
s32 higgs_init_intr(struct higgs_card * ll_card)
{
	if (OK != higgs_init_phy_irq(ll_card))
		goto FAIL_PHY_INIT;

	if (OK != higgs_init_cq_irq(ll_card))
		goto FAIL_CQ_INIT;

	if (OK != higgs_init_chip_fatal_irq(ll_card))
		goto FAIL_FATAL_INIT;

#if 0
	if (OK != Higgs_InitSp804TimerIrq(higgs_card)) {
		goto FAIL_TIMER_INIT;
	}
#endif

	return OK;
#if 0
      FAIL_TIMER_INIT:
	higgs_free_chip_fatal_irq(ll_card);
#endif
      FAIL_FATAL_INIT:
	higgs_free_cq_irq(ll_card);
      FAIL_CQ_INIT:
	higgs_free_phy_irq(ll_card);
      FAIL_PHY_INIT:
	return ERROR;
}

/*
 * release interrupt  resources 
 */

void higgs_release_intr(struct higgs_card *ll_card)
{
	higgs_free_phy_irq(ll_card);
	higgs_free_cq_irq(ll_card);
	higgs_free_chip_fatal_irq(ll_card);
	return;
}

/*
 * INT0 interrupt process
 */
irqreturn_t higgs_phy_abnormal(u32 phy_id, void *ll_card)
{
	U_CHL_INT0 irq_val;
	u32 irq_mask_old = 0;
	struct higgs_card *card = NULL;

	HIGGS_ASSERT(NULL != ll_card, return IRQ_NONE);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return IRQ_NONE);
	card = (struct higgs_card *)ll_card;

	irq_mask_old =
	    HIGGS_PORT_REG_READ(card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT0_MSK_REG);
	HIGGS_PORT_REG_WRITE(card, phy_id,
			     HISAS30HV100_PORT_REG_CHL_INT0_MSK_REG,
			     HIGGS_SAS_PORT_PHY_CHL_INTR_MASK0_ALL);

	/*get interrupt status  */
	irq_val.u32 =
	    HIGGS_PORT_REG_READ(card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT0_REG);
	HIGGS_TRACE_LIMIT(OSP_DBL_INFO, msecs_to_jiffies(1000), 2, 4707,
			  "Card:%d phy %d abnormal, uiIrqVal = 0x%x",
			  card->card_id, phy_id, irq_val.u32);

	/* PHY DOWN*/
	if (irq_val.bits.phyctrl_not_rdy) {
		higgs_phy_down(card, phy_id);
	} else if (irq_val.bits.sl_idaf_tout_conf) {
		/* Identify Timeout*/
		higgs_phy_identify_timeout(card, phy_id);
	} else {
		/* loss dword sync*/
		if (irq_val.bits.phyctrl_dws_lost)
			higgs_phy_loss_dw_sync(card, phy_id);

		/*speed negotitation fail*/
		if (irq_val.bits.phyctrl_sn_fail_ngr)
			HIGGS_TRACE(OSP_DBL_MINOR, 4187,
				    "Card: %d phy %d link negotiation fail",
				    card->card_id, phy_id);

		/* process address frame error*/
		if (irq_val.bits.sl_idaf_fail_conf
		    || irq_val.bits.sl_opaf_fail_conf)
			higgs_check_address_frame_error(card, phy_id);

		/* PS_REQ fail*/
		if (irq_val.bits.sl_ps_fail)
			HIGGS_TRACE(OSP_DBL_MINOR, 4187,
				    "Card: %d phy %d PS_REQ fail",
				    card->card_id, phy_id);
	}

	/* clear interrupt status*/
	HIGGS_PORT_REG_WRITE(card, phy_id, HISAS30HV100_PORT_REG_CHL_INT0_REG,
			     irq_val.u32);

	if (irq_val.bits.phyctrl_not_rdy) {
		higgs_bypass_chip_bug_mask_abnormal_intr(card, phy_id);
	} else {

		HIGGS_PORT_REG_WRITE(card, phy_id,
				     HISAS30HV100_PORT_REG_CHL_INT0_MSK_REG,
				     irq_mask_old);
	}

	return IRQ_HANDLED;
}

/*
 * INT1 interrupt process
 */
irqreturn_t higgs_phy_int_reg1(u32 phy_id, void *ll_card)
{
	u32 irq_val1 = 0;
	u32 irq_val2 = 0;

	struct higgs_card *card = NULL;

	HIGGS_ASSERT(NULL != ll_card, return IRQ_NONE);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return IRQ_NONE);
	card = (struct higgs_card *)ll_card;

	irq_val1 =
	    HIGGS_PORT_REG_READ(card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT1_REG);
	irq_val2 =
	    HIGGS_PORT_REG_READ(card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT2_REG);

	/* INT1: all */
	if (irq_val1)
		HIGGS_TRACE(OSP_DBL_INFO, 4707,
			    "Card:%d phy %d, uiIrqVal1 = 0x%x", card->card_id,
			    phy_id, irq_val1);
		/* TODO: */

	/* INT2: Pattern Lock Lost Timeout */
	if (GET_BIT(irq_val2, 8))
		/* bit[8] */
		HIGGS_TRACE(OSP_DBL_MINOR, 4187,
			    "Card:%d phy %d pattern lock lost timeout",
			    card->card_id, phy_id);
		/* TODO: */

/* INT2: RX Eye Diag Done *//* bit[9] */
	if (GET_BIT(irq_val2, 9))
		HIGGS_TRACE(OSP_DBL_INFO, 4187,
			    "Card:%d phy %d RX eye diag done", card->card_id,
			    phy_id);
		/* TODO: */

	HIGGS_PORT_REG_WRITE(card, phy_id, HISAS30HV100_PORT_REG_CHL_INT1_REG,
			     irq_val1);
	HIGGS_PORT_REG_WRITE(card, phy_id, HISAS30HV100_PORT_REG_CHL_INT2_REG,
			     (0x1 << 8) | (0x1 << 9));

	return IRQ_HANDLED;
}

/*
 * INT1 interrupt process,phy up
 */
irqreturn_t higgs_phy_up(u32 phy_id, void *ll_card)
{
	U_CHL_INT2 irq_val;
	u32 chip_portid = HIGGS_INVALID_CHIP_PORT_ID;
	struct higgs_port *ll_port = NULL;
	struct higgs_phy *ll_phy = NULL;
	struct sal_phy *sal_phy = NULL;
	struct sal_port *sal_port = NULL;
	struct sal_phy_event phy_event_info;
	u32 higgs_rate = 0;
	u32 *identify_frm;	/*[ADDR_IDENTIFY_LEN] = { 0 };*/
	u32 i = 0, tmp_val = 0;
	struct higgs_card *card = NULL;
	irqreturn_t ret = IRQ_NONE;
	U_SL_CONTROL reg;
	u8 dev_type = 0;
#if defined(PV660_ARM_SERVER)
	struct higgs_led_event_notify led_event_data;
#endif

	HIGGS_ASSERT(NULL != ll_card, return IRQ_NONE);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return IRQ_NONE);

	card = (struct higgs_card *)ll_card;

	irq_val.u32 =
	    HIGGS_PORT_REG_READ(card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT2_REG);
	if (!irq_val.bits.sl_phy_enabled) {
		HIGGS_TRACE(OSP_DBL_MINOR, 4707,
			    "Card:%d phy %d phy not up, uiIrqVal:0x%x",
			    card->card_id, phy_id, irq_val.u32);
		ret = IRQ_NONE;
		goto END;
	}
	HIGGS_TRACE(OSP_DBL_INFO, 4707, "Card:%d phy %d phy up, uiIrqVal:0x%x",
		    card->card_id, phy_id, irq_val.u32);

	chip_portid =
	    (((HIGGS_GLOBAL_REG_READ
	       (card,
		HISAS30HV100_GLOBAL_REG_PHY_PORT_NUM_MA_REG)) >> (phy_id *
								  CFG_PHY_BIT_NUM))
	     & 0xF);
	if (HIGGS_INVALID_CHIP_PORT_ID == chip_portid) {
		HIGGS_TRACE(OSP_DBL_INFO, 4707,
			    "Card:%d Invalid chip PortId 0x%x on Phy %d up",
			    card->card_id, chip_portid, phy_id);
		ret = IRQ_NONE;
		goto END;
	} else {
		HIGGS_TRACE(OSP_DBL_INFO, 4707,
			    "Card:%d Chip PortId 0x%x on Phy %d up",
			    card->card_id, chip_portid, phy_id);
	}

	/* stop serdes fw timer */
	(void)higgs_stop_serdes_fw_timer(card, phy_id);

	ll_phy = &card->phy[phy_id];
	ll_port = higgs_get_port_by_port_id(card, chip_portid);
	if (NULL == ll_port) {
		HIGGS_TRACE(OSP_DBL_INFO, 4707,
			    "Card:%d Get free port, chip port id is %d",
			    card->card_id, chip_portid);
		ll_port = higgs_get_free_port(card);
		if (NULL == ll_port) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4708,
				    "Card:%d Get free port failed by port_id:%d of chip",
				    card->card_id, chip_portid);
			ret = IRQ_NONE;
			goto END;
		}
	}

	higgs_mark_ll_port_on_phy_up(card, ll_port, phy_id, chip_portid);

	sal_port = ll_port->up_ll_port;
	sal_phy = ll_phy->up_ll_phy;
	identify_frm = (u32 *) (u64) (&sal_phy->remote_identify);

	/* read 6 dword identify frame from 0xc4*/
	for (i = 0; i < HIGGS_ADDR_IDENTIFY_LEN; i++) {
		tmp_val =
		    HIGGS_PORT_REG_READ(card, phy_id,
					HISAS30HV100_PORT_REG_RX_IDAF_DWORD0_REG
					+ (u64) i * sizeof(u32));
		identify_frm[i] = HIGGS_SWAP_32(tmp_val);
	}


	HIGGS_TRACE(OSP_DBL_INFO, 4707,
		    "Card:%d phy %d, remote SasAddr = 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
		    card->card_id, phy_id,
		    sal_phy->remote_identify.sas_addr_hi[0],
		    sal_phy->remote_identify.sas_addr_hi[1],
		    sal_phy->remote_identify.sas_addr_hi[2],
		    sal_phy->remote_identify.sas_addr_hi[3],
		    sal_phy->remote_identify.sas_addr_lo[0],
		    sal_phy->remote_identify.sas_addr_lo[1],
		    sal_phy->remote_identify.sas_addr_lo[2],
		    sal_phy->remote_identify.sas_addr_lo[3]);


	/*direct connect sas，need to send notify primitive */
	dev_type = sal_phy->remote_identify.dev_type;
	HIGGS_TRACE(OSP_DBL_INFO, 4707,
		    "Card:%d phy %d, remote UcDevType = 0x%x", card->card_id,
		    phy_id, dev_type);

	/*check direct connect dev*/
	if (SAS_END_DEVICE == ((dev_type & 0x70) >> 4)) {
		HIGGS_TRACE(OSP_DBL_INFO, 4707,
			    "phy_id(%d), need send notify primi", phy_id);
		reg.u32 =
		    HIGGS_PORT_REG_READ(card, phy_id,
					HISAS30HV100_PORT_REG_SL_CONTROL_REG);
		/*Send the Notify Primitive */
		reg.bits.cfg_notify_en = 1;
		HIGGS_PORT_REG_WRITE(card, phy_id,
				     HISAS30HV100_PORT_REG_SL_CONTROL_REG,
				     reg.u32);
		SAS_MDELAY(1);
		/*Clear the Notify bit */
		reg.u32 =
		    HIGGS_PORT_REG_READ(card, phy_id,
					HISAS30HV100_PORT_REG_SL_CONTROL_REG);
		reg.bits.cfg_notify_en = 0;
		HIGGS_PORT_REG_WRITE(card, phy_id,
				     HISAS30HV100_PORT_REG_SL_CONTROL_REG,
				     reg.u32);
	}

	/* read sas global cfg 0x30 for link rate */
	higgs_rate =
	    HIGGS_GLOBAL_REG_READ(card,
				  HISAS30HV100_GLOBAL_REG_PHY_CONN_RATE_REG);
	higgs_rate = ((higgs_rate >> (phy_id * CFG_PHY_BIT_NUM)) & 0xf);

	/*notice SAL */
	memset(&phy_event_info, 0, sizeof(struct sal_phy_event));
	phy_event_info.link_rate =
	    (enum sal_link_rate)higgs_get_sal_phy_rate(higgs_rate);
	phy_event_info.event = SAL_PHY_EVENT_UP;
	phy_event_info.fw_port_id = chip_portid;
	phy_event_info.phy_id = phy_id;
	phy_event_info.card_id = card->card_id;

#if defined(PV660_ARM_SERVER)
	/*solve light oops*/
	led_event_data.phy_id = phy_id;
	led_event_data.event_val = LED_DISK_REBUILD;

	higgs_oper_led_by_event(ll_card, &led_event_data);

#endif

	(void)sal_phy_event(sal_port, sal_phy, phy_event_info);
	ret = IRQ_HANDLED;

      END:
	HIGGS_PORT_REG_WRITE(card, phy_id, HISAS30HV100_PORT_REG_CHL_INT2_REG,
			     PBIT6);

	if (irq_val.bits.sl_phy_enabled)
		higgs_bypass_chip_bug_unmask_abnormal_intr(card, phy_id);

	return ret;
}

irqreturn_t higgs_phy_bcast(u32 phy_id, void *ll_card)
{
	struct higgs_port *port = NULL;
	struct higgs_phy *phy = NULL;
	struct sal_port *sal_port = NULL;
	struct sal_phy *sal_phy = NULL;
	u32 chip_portid = 0;
	struct sal_phy_event phy_event_info;
	U_CHL_INT2 irq_val;
	struct higgs_card *card = NULL;
	irqreturn_t ret = IRQ_NONE;

	HIGGS_ASSERT(NULL != ll_card, return IRQ_NONE);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return IRQ_NONE);
	card = (struct higgs_card *)ll_card;

	irq_val.u32 =
	    HIGGS_PORT_REG_READ(card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT2_REG);
	if (!irq_val.bits.sl_rx_bcast_ack) {
		HIGGS_TRACE(OSP_DBL_MINOR, 4707,
			    "Card:%d phy %d no bcast, uiIrqVal = 0x%x",
			    card->card_id, phy_id, irq_val.u32);
		ret = IRQ_NONE;
		goto END;
	}
	HIGGS_TRACE(OSP_DBL_INFO, 4707, "Card:%d phy %d bcast, uiIrqVal = 0x%x",
		    card->card_id, phy_id, irq_val.u32);

	phy = &card->phy[phy_id];
	chip_portid = phy->up_ll_phy->port_id;

	HIGGS_TRACE(OSP_DBL_INFO, 4711,
		    "Card:%d/Port:%d/Phy:%d broadcast change.", card->card_id,
		    chip_portid, phy_id);
	port = higgs_get_port_by_port_id(card, chip_portid);
	if (NULL == port) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4712,
			    "Card:%d get port failed by port_id:%d of chip",
			    card->card_id, chip_portid);
		ret = IRQ_NONE;
		goto END;
	}
	sal_phy = phy->up_ll_phy;
	sal_port = port->up_ll_port;

	memset(&phy_event_info, 0, sizeof(struct sal_phy_event));
	phy_event_info.link_rate = SAL_LINK_RATE_BUTT;
	phy_event_info.event = SAL_PHY_EVENT_BCAST;
	phy_event_info.fw_port_id = chip_portid;
	phy_event_info.phy_id = phy_id;
	phy_event_info.card_id = card->card_id;

	(void)sal_phy_event(sal_port, sal_phy, phy_event_info);
	ret = IRQ_HANDLED;

      END:
	HIGGS_PORT_REG_WRITE(card, phy_id, HISAS30HV100_PORT_REG_CHL_INT2_REG,
			     PBIT2);

	return ret;
}


irqreturn_t higgs_phy_ctrl_rdy(u32 phy_id, void *ll_card)
{
	U_CHL_INT2 irq_val;
	struct higgs_card *card = NULL;
	irqreturn_t ret = IRQ_NONE;

	HIGGS_ASSERT(NULL != ll_card, return IRQ_NONE);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return IRQ_NONE);
	card = (struct higgs_card *)ll_card;

	irq_val.u32 =
	    HIGGS_PORT_REG_READ(card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT2_REG);
	if (!irq_val.bits.phyctrl_phy_rdy) {
		HIGGS_TRACE(OSP_DBL_MINOR, 4707,
			    "Card:%d phy %d no ctrl ready, uiIrqVal = 0x%x",
			    card->card_id, phy_id, irq_val.u32);
		ret = IRQ_NONE;
		goto END;
	}
	HIGGS_TRACE(OSP_DBL_INFO, 4707,
		    "Card:%d phy %d ctrl ready, uiIrqVal = 0x%x", card->card_id,
		    phy_id, irq_val.u32);

	/* 12G need set serdes parameter */
	(void)higgs_config_serdes_for_12g(card, phy_id);

	ret = IRQ_HANDLED;

      END:
	HIGGS_PORT_REG_WRITE(card, phy_id, HISAS30HV100_PORT_REG_CHL_INT2_REG,
			     PBIT0);

	return ret;
}

irqreturn_t higgs_phy_dma_err(u32 phy_id, void *ll_card)
{
	U_CHL_INT2 irq_val;
	struct higgs_card *card = NULL;
	irqreturn_t ret = IRQ_NONE;

	HIGGS_ASSERT(NULL != ll_card, return IRQ_NONE);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return IRQ_NONE);
	card = (struct higgs_card *)ll_card;

	irq_val.u32 =
	    HIGGS_PORT_REG_READ(card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT2_REG);
	if (!GET_BIT(irq_val.u32, 7)) {
		HIGGS_TRACE(OSP_DBL_MINOR, 4707,
			    "Card:%d phy %d no DMA error, uiIrqVal = 0x%x",
			    card->card_id, phy_id, irq_val.u32);
		ret = IRQ_NONE;
		goto END;
	}

	/* TODO: */
	HIGGS_TRACE(OSP_DBL_INFO, 4707,
		    "Card:%d phy %d DMA error, uiIrqVal = 0x%x", card->card_id,
		    phy_id, irq_val.u32);
	ret = IRQ_HANDLED;

      END:
	HIGGS_PORT_REG_WRITE(card, phy_id, HISAS30HV100_PORT_REG_CHL_INT2_REG,
			     PBIT7);

	return ret;
}

irqreturn_t higgs_phy_hot_plug_tout(u32 phy_id, void *ll_card)
{
	U_CHL_INT2 irq_val;
	struct higgs_card *card = NULL;
	irqreturn_t ret = IRQ_NONE;

	HIGGS_ASSERT(NULL != ll_card, return IRQ_NONE);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return IRQ_NONE);
	card = (struct higgs_card *)ll_card;
    
	irq_val.u32 =
	    HIGGS_PORT_REG_READ(card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT2_REG);
	if (!irq_val.bits.phyctrl_hotplug_tout) {
		HIGGS_TRACE(OSP_DBL_MINOR, 4707,
			    "Card:%d phy %d no hotplug timeout, uiIrqVal = 0x%x",
			    card->card_id, phy_id, irq_val.u32);
		ret = IRQ_NONE;
		goto END;
	}

	HIGGS_TRACE(OSP_DBL_INFO, 4707,
		    "Card:%d phy %d hotplug timeout, uiIrqVal = 0x%x",
		    card->card_id, phy_id, irq_val.u32);
	ret = IRQ_HANDLED;

      END:
	HIGGS_PORT_REG_WRITE(card, phy_id, HISAS30HV100_PORT_REG_CHL_INT2_REG,
			     PBIT1);
    
	return ret;
}

irqreturn_t higgs_phy_oob_restart(u32 phy_id, void *ll_card)
{
	U_CHL_INT2 irq_val;
	struct higgs_card *card = NULL;
	irqreturn_t ret = IRQ_NONE;

	HIGGS_ASSERT(NULL != ll_card, return IRQ_NONE);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return IRQ_NONE);
	card = (struct higgs_card *)ll_card;

	irq_val.u32 =
	    HIGGS_PORT_REG_READ(card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT2_REG);
	if (!irq_val.bits.phyctrl_oob_restart_ci) {
		HIGGS_TRACE(OSP_DBL_MINOR, 4707,
			    "Card:%d phy %d no OOB restart, uiIrqVal = 0x%x",
			    card->card_id, phy_id, irq_val.u32);
		ret = IRQ_NONE;
		goto END;
	}

	HIGGS_TRACE(OSP_DBL_INFO, 4707,
		    "Card:%d phy %d OOB restart, uiIrqVal = 0x%x",
		    card->card_id, phy_id, irq_val.u32);
	ret = IRQ_HANDLED;

      END:
	HIGGS_PORT_REG_WRITE(card, phy_id, HISAS30HV100_PORT_REG_CHL_INT2_REG,
			     PBIT3);

	return ret;
}

/*
 * phy hard reset interrupt
 */
irqreturn_t higgs_ph_rx_hard_rst(u32 phy_id, void *ll_card)
{
	U_CHL_INT2 irq_val;
	struct higgs_card *card = NULL;
	irqreturn_t ret = IRQ_NONE;

	HIGGS_ASSERT(NULL != ll_card, return IRQ_NONE);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return IRQ_NONE);
	card = (struct higgs_card *)ll_card;

	irq_val.u32 =
	    HIGGS_PORT_REG_READ(card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT2_REG);
	if (!irq_val.bits.sl_rx_hardrst) {
		HIGGS_TRACE(OSP_DBL_MINOR, 4707,
			    "Card:%d phy %d no RX hard reset, uiIrqVal = 0x%x",
			    card->card_id, phy_id, irq_val.u32);
		ret = IRQ_NONE;
		goto END;
	}

	/* TODO: */
	HIGGS_TRACE(OSP_DBL_INFO, 4707,
		    "Card:%d phy %d RX hard reset, uiIrqVal = 0x%x",
		    card->card_id, phy_id, irq_val.u32);
	ret = IRQ_HANDLED;

      END:
	HIGGS_PORT_REG_WRITE(card, phy_id, HISAS30HV100_PORT_REG_CHL_INT2_REG,
			     PBIT4);

	return ret;
}

irqreturn_t higgs_phy_status_chg(u32 phy_id, void *ll_card)
{
	U_CHL_INT2 irq_val;
	struct higgs_card *card = NULL;
	irqreturn_t ret = IRQ_NONE;
    
	HIGGS_ASSERT(NULL != ll_card, return IRQ_NONE);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return IRQ_NONE);
	card = (struct higgs_card *)ll_card;

	irq_val.u32 =
	    HIGGS_PORT_REG_READ(card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT2_REG);
	if (!irq_val.bits.phyctrl_status_chg) {
		HIGGS_TRACE(OSP_DBL_MINOR, 4707,
			    "Card:%d phy %d no status change, uiIrqVal = 0x%x",
			    card->card_id, phy_id, irq_val.u32);
		ret = IRQ_NONE;
		goto END;
	}

	/* TODO: */
	HIGGS_TRACE(OSP_DBL_INFO, 4707,
		    "Card:%d phy %d status change, uiIrqVal = 0x%x",
		    card->card_id, phy_id, irq_val.u32);
	ret = IRQ_HANDLED;

      END:
	HIGGS_PORT_REG_WRITE(card, phy_id, HISAS30HV100_PORT_REG_CHL_INT2_REG,
			     PBIT5);

	return ret;
}

irqreturn_t higgs_oq_core_int(u32 cq_id, void *param)
{
	struct higgs_card *ll_card = (struct higgs_card *)param;
	u32 oq_intr = 0;
	u32 queue_id = 0;

	oq_intr =
	    HIGGS_GLOBAL_REG_READ(ll_card,
				  HISAS30HV100_GLOBAL_REG_OQ_INT_SRC_REG);
	/* CodeCC */
#if 0
	uiOqIntr = uiOqIntr & (~HIGGS_OQ_INT_MASK);
#else
	oq_intr = oq_intr & HIGGS_OQ_INT_MASK;
#endif
	/* CodeCC */

	/* get CQ ID */
	queue_id = cq_id;

	HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, 60 * HZ, 4707,
			     "uiOqIntr:  0x%x uiCqId:0x%x", oq_intr, cq_id);
	HIGGS_GLOBAL_REG_WRITE(ll_card, HISAS30HV100_GLOBAL_REG_OQ_INT_SRC_REG,
			       (u32) (1 << queue_id));

/*  (void)Higgs_CheckCardVar(card);*/
	higgs_oq_int_process(ll_card, queue_id);
/*  (void)Higgs_CheckCardVar(card);*/

	return IRQ_HANDLED;
}

irqreturn_t higgs_chip_fatal_hgc_ecc_int_proc(s32 chip_fatal_id, void *param)
{
	struct higgs_card *ll_card = (struct higgs_card *)param;
	u32 chip_fatal_intr = 0;
	u32 reg_value = 0;

	HIGGS_REF(chip_fatal_id);

	HIGGS_TRACE(OSP_DBL_INFO, 4187,
		    "Chip Fatal HGC ECC Err Interrupt Happen !!!! ");

	chip_fatal_intr =
	    HIGGS_GLOBAL_REG_READ(ll_card,
				  HISAS30HV100_GLOBAL_REG_SAS_ECC_INTR_REG);

	/*1bit ECC error*/
	if (GET_BIT0(chip_fatal_intr)) {
		reg_value =
		    HIGGS_GLOBAL_REG_READ(ll_card,
					  HISAS30HV100_GLOBAL_REG_HGC_ECO_REG0_REG);

		HIGGS_TRACE(OSP_DBL_MINOR, 4187,
			    "Chip Error HgcDqeRam1BitEccErr FOUND ! Error Count is %d !",
			    reg_value);

		ll_card->chip_err_fatal_stat.hgc_dqe_ram_1bit_ecc_err_cnt =
		    reg_value;
	}
	/*mulbit ECC error*/
	if (GET_BIT1(chip_fatal_intr)) {
		ll_card->chip_err_fatal_stat.hgc_dqe_ram_mul_bit_ecc_err_cnt +=
		    1;

		reg_value =
		    HIGGS_GLOBAL_REG_READ(ll_card,
					  HISAS30HV100_GLOBAL_REG_HGC_DQE_ECC_ADDR_REG);
		reg_value = (reg_value & 0x0fff0000) >> 16;	/*bit[27:16]*/

		HIGGS_TRACE(OSP_DBL_MINOR, 4187,
			    "Chip Error HgcDqeRamMulBitEccErr FOUND ! RAM Address is 0x%08X ",
			    reg_value);

	}
	/*iost 1bit ECC error*/
	if (GET_BIT2(chip_fatal_intr)) {
		reg_value =
		    HIGGS_GLOBAL_REG_READ(ll_card,
					  HISAS30HV100_GLOBAL_REG_HGC_ECO_REG0_REG);

		HIGGS_TRACE(OSP_DBL_MINOR, 4187,
			    "Chip Error HgcIostRam1BitEccErr FOUND ! Error Count is %d !",
			    reg_value);

		ll_card->chip_err_fatal_stat.hgc_iost_ram_1bit_ecc_err_cnt =
		    reg_value;
	}
	/*mulbit ECC error*/
	if (GET_BIT3(chip_fatal_intr)) {
		ll_card->chip_err_fatal_stat.hgc_iost_ram_mul_bit_ecc_err_cnt +=
		    1;

		reg_value =
		    HIGGS_GLOBAL_REG_READ(ll_card,
					  HISAS30HV100_GLOBAL_REG_HGC_IOST_ECC_ADDR_REG);
		reg_value = (reg_value & 0x03ff0000) >> 16;	/*bit[25:16]*/

		HIGGS_TRACE(OSP_DBL_MINOR, 4187,
			    "Chip Error HgcIostRamMulBitEccErr FOUND ! RAM Address is 0x%08X ",
			    reg_value);
	}
	/*itct mulbit ECC error*/
	if (GET_BIT4(chip_fatal_intr)) {
		ll_card->chip_err_fatal_stat.hgc_itct_ram_mul_bit_ecc_err_cnt +=
		    1;

		reg_value =
		    HIGGS_GLOBAL_REG_READ(ll_card,
					  HISAS30HV100_GLOBAL_REG_HGC_ITCT_ECC_ADDR_REG);
		reg_value = (reg_value & 0x03ff0000) >> 16;	/*bit[25:16]*/

		HIGGS_TRACE(OSP_DBL_MINOR, 4187,
			    "Chip Error HgcItctRamMulBitEccErr FOUND ! RAM Address is 0x%08X ",
			    reg_value);
	}
	/*1bit ECC error*/
	if (GET_BIT5(chip_fatal_intr)) {
		reg_value =
		    HIGGS_GLOBAL_REG_READ(ll_card,
					  HISAS30HV100_GLOBAL_REG_HGC_ECO_REG0_REG);

		HIGGS_TRACE(OSP_DBL_MINOR, 4187,
			    "Chip Error HgcItctRam1BitEccErr FOUND ! Error Count is %d !",
			    reg_value);

		ll_card->chip_err_fatal_stat.hgc_itct_ram_1bit_ecc_err_cnt =
		    reg_value;
	}

	/*clear intr*/
	chip_fatal_intr = chip_fatal_intr | 0x3f;	/*bit0~bit5*/
	HIGGS_GLOBAL_REG_WRITE(ll_card,
			       HISAS30HV100_GLOBAL_REG_SAS_ECC_INTR_REG,
			       chip_fatal_intr);
    
	return IRQ_HANDLED;
}

irqreturn_t higgs_chip_fatal_hgc_axi_int_proc(s32 chip_fatal_id, void *param)
{
	struct higgs_card *ll_card = (struct higgs_card *)param;
	u32 reg_value = 0;
	u32 chip_fatal_intr = 0;

	HIGGS_REF(chip_fatal_id);

	HIGGS_TRACE(OSP_DBL_INFO, 4187,
		    "Chip Fatal HGC AXI Interrupt Happen !!!! ");

	chip_fatal_intr =
	    HIGGS_GLOBAL_REG_READ(ll_card,
				  HISAS30HV100_GLOBAL_REG_ENT_INT_SRC2_REG);

	reg_value =
	    HIGGS_GLOBAL_REG_READ(ll_card,
				  HISAS30HV100_GLOBAL_REG_HGC_AXI_FIFO_ERR_INFO_REG);

	if (GET_BIT28(chip_fatal_intr))
		/*axi_err_info [7:0]*/
		HIGGS_TRACE(OSP_DBL_MINOR, 4187,
			    "Chip Error AXI INT and FIFO overflow! axi_err_info=0x%08X",
			    reg_value & 0xff);

	if (GET_BIT29(chip_fatal_intr)) {      
		ll_card->chip_err_fatal_stat.hgc_fifo_ovfld_int_err_cnt += 1;

		/*fifo_err_info [10:8]*/
		HIGGS_TRACE(OSP_DBL_MINOR, 4187,
			    "Chip Error AXI INT and FIFO overflow! fifo_err_info=0x%08X",
			    (reg_value & 0x700) >> 8);
	}

	chip_fatal_intr = chip_fatal_intr | 0x30000000;	/*clear bit28~29*/
	HIGGS_GLOBAL_REG_WRITE(ll_card,
			       HISAS30HV100_GLOBAL_REG_ENT_INT_SRC2_REG,
			       chip_fatal_intr);

	return IRQ_HANDLED;
}

void higgs_phy_unmask_intr(struct higgs_card *ll_card, u32 phy_id)
{
	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return);

	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_CHL_INT0_MSK_REG,
			     HIGGS_SAS_PORT_PHY_CHL_INTR_MASK0_DEFAULT);
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_CHL_INT1_MSK_REG,
			     HIGGS_SAS_PORT_PHY_CHL_INTR_MASK1_DEFAULT);
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_CHL_INT2_MSK_REG,
			     HIGGS_SAS_PORT_PHY_CHL_INTR_MASK2_DEFAULT);
}

void higgs_phy_mask_intr(struct higgs_card *ll_card, u32 phy_id)
{
	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return);

	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_CHL_INT0_MSK_REG,
			     HIGGS_SAS_PORT_PHY_CHL_INTR_MASK0_ALL);
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_CHL_INT1_MSK_REG,
			     HIGGS_SAS_PORT_PHY_CHL_INTR_MASK1_ALL);
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_CHL_INT2_MSK_REG,
			     HIGGS_SAS_PORT_PHY_CHL_INTR_MASK2_ALL);
}

void higgs_phy_clear_intr_status(struct higgs_card *ll_card, u32 phy_id)
{
	u32 value = 0;

	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return);

	value =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT0_REG);
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_CHL_INT0_REG, value);

	value =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT1_REG);
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_CHL_INT1_REG, value);

	value =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT2_REG);
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_CHL_INT2_REG, value);
}

/*
 * check whether there are phy down interrupt set
 */
bool higgs_phy_down_intr_come(struct higgs_card * ll_card, u32 phy_id)
{
	U_CHL_INT0 irq_val;

	HIGGS_ASSERT(NULL != ll_card, return false);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return false);

	irq_val.u32 =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT0_REG);

	return irq_val.bits.phyctrl_not_rdy ? true : false;
}

void higgs_phy_clear_down_intr_status(struct higgs_card *ll_card, u32 phy_id)
{
	U_CHL_INT0 irq_val;

	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return);

	irq_val.u32 = 0;
	irq_val.bits.phyctrl_not_rdy = 0x1;
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_CHL_INT0_REG, irq_val.u32);
}

/*
 * simulate phy stop rsp intr
 */
void higgs_simulate_phy_stop_rsp_intr(struct higgs_card *ll_card, u32 phy_id)
{
	unsigned long flag = 0;

	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return);

	spin_lock_irqsave(&ll_card->card_lock, flag);
	ll_card->phy[phy_id].to_notify_phy_stop_rsp = true;
	spin_unlock_irqrestore(&ll_card->card_lock, flag);

	return;
}

void higgs_simulate_wire_hotplug_intr(struct higgs_card *ll_card, u32 port_id,
				      u32 timeout_ms)
{
	unsigned long flag = 0;

	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(port_id < HIGGS_MAX_PORT_NUM, return);

	spin_lock_irqsave(&ll_card->card_lock, flag);
	ll_card->port[port_id].to_notify_wire_hotplug = true;
	spin_unlock_irqrestore(&ll_card->card_lock, flag);

	return;
}

static s32 higgs_init_phy_irq(struct higgs_card *ll_card)
{
	u32 phy_id = 0;
	u32 irq_id = 0;
	u32 card_id = 0;
	s32 virtual_irq;
	struct higgs_card *higgs_card = NULL;
	struct higgs_int_info *int_info = NULL;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);

	higgs_card = (struct higgs_card *)ll_card;
	card_id = higgs_card->card_id;

	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		for (irq_id = 0; irq_id < MSI_PHY_BUTT; irq_id++) {
			int_info = &higgs_card->phy_irq[irq_id][phy_id];
			int_info->reg_succ = false;
		}
	}

	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		for (irq_id = 0; irq_id < MSI_PHY_BUTT; irq_id++) {
			int_info = &higgs_card->phy_irq[irq_id][phy_id];
			if (NULL == g_pfnIntIsr[irq_id][phy_id]) {
				HIGGS_TRACE(OSP_DBL_INFO, 4707,
					    "PhyID =%d offset =%d is null.",
					    phy_id, irq_id);
				continue;
			}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)	/* linux 3.19 */
			/* phy interrupt resouce start by interrupt table index=0 */

			int_info->irq_num =
			    higgs_card->card_cfg.intr_table[irq_id +
						   MSI_CHANNEL_OFFSET(phy_id)];
#else
			if (P660_SAS_CORE_DSAF_ID == card_id)
				int_info->irq_num =
				    irq_id + MSI_CHANNEL_OFFSET(phy_id) +
				    SAS_DSAF_MSI_INT_ID_START;
			else if (P660_SAS_CORE_PCIE_ID == card_id)
				int_info->irq_num =
				    irq_id + MSI_CHANNEL_OFFSET(phy_id) +
				    SAS_PCIE_MSI_INT_ID_START;
			else if (P660_1_SAS_CORE_DSAF_ID == card_id)
				int_info->irq_num =
				    irq_id + MSI_CHANNEL_OFFSET(phy_id) +
				    SLAVE_SAS_DSAF_MSI_INT_ID_START;
			else if (P660_1_SAS_CORE_PCIE_ID == card_id)
				int_info->irq_num =
				    irq_id + MSI_CHANNEL_OFFSET(phy_id) +
				    SLAVE_SAS_PCIE_MSI_INT_ID_START;
			else
				break;
#endif

			int_info->int_handler = g_pfnIntIsr[irq_id][phy_id];
			(void)snprintf(int_info->intr_vec_name,
				       HIGGS_MAX_INTRNAME_LEN - 1,
				       (char *)"Card%d_Phy_%d", card_id,
				       phy_id);

			/*enable hardware irq, get virtual irq */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)	/* linux 3.19 */
			virtual_irq = int_info->irq_num;
#else
			if (OK !=
			    HIGGS_IC_ENABLE_MSI((s32) int_info->irq_num,
						&virtual_irq)) {
				HIGGS_TRACE(OSP_DBL_INFO, 4707,
					    "Card:%d phy %d map hwirq %d to virq %d failed.",
					    higgs_card->card_id, phy_id,
					    int_info->irq_num, virtual_irq);
				goto FAIL;
			}
#endif
			HIGGS_TRACE_LIMIT(OSP_DBL_INFO, 1 * HZ, 3, 4796,
					  "Card:%d phy %d, offset %d, virq 0x%x.",
					  higgs_card->card_id, phy_id, irq_id,
					  virtual_irq);

			int_info->irq_num = (u32) virtual_irq;
			if (OK !=
			    SAS_REQUEST_IRQ((u32) virtual_irq,
					    int_info->int_handler,
					    (unsigned long)0,
					    int_info->intr_vec_name,
					    higgs_card)) {
				HIGGS_TRACE(OSP_DBL_MAJOR, 4415,
					    "Card:%d phy %d request %d IRQ failed",
					    higgs_card->card_id, phy_id,
					    int_info->irq_num);
				goto FAIL;
			}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)	/* linux 3.19 */
#if 0
			if (higgs_irq_set_affinity
			    (pstIntInfo->irq_num,
			     cpumask_of(HIGGS_PHY_ATTACH_CPU_ID)) < 0) {
				HIGGS_TRACE(OSP_DBL_MAJOR, 4415,
					    "Card:%d phy %d attach IRQ %d to CPU%d failed",
					    pstLLCard->card_id, uiPhyID,
					    pstIntInfo->irq_num,
					    HIGGS_PHY_ATTACH_CPU_ID);
				goto fail;
			}
#endif
#else
			if (OK !=
			    higgs_irq_set_affinity(int_info->irq_num,
						   HIGGS_CPUMASK_OF
						   (HIGGS_PHY_ATTACH_CPU_ID))) {
				HIGGS_TRACE(OSP_DBL_MAJOR, 4415,
					    "Card:%d phy %d attach IRQ %d to CPU%d failed",
					    higgs_card->card_id, phy_id,
					    int_info->irq_num,
					    HIGGS_PHY_ATTACH_CPU_ID);
				goto FAIL;
			}
#endif
			int_info->reg_succ = true;
		}
	}

	return OK;
      FAIL:
	higgs_free_phy_irq(higgs_card);
	return ERROR;
}

static s32 higgs_init_cq_irq(struct higgs_card *ll_card)
{
	u32 cq_id = 0;
	struct higgs_card *higgs_card = NULL;
	struct higgs_int_info *int_info = NULL;
	u32 card_id = 0;
	s32 virtual_irq;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	higgs_card = (struct higgs_card *)ll_card;
	card_id = higgs_card->card_id;

	/* 1. set all be uninitialization*/
	for (cq_id = 0; cq_id < HIGGS_MAX_CQ_NUM; cq_id++) {
		int_info = &higgs_card->cq_irq[cq_id];
		int_info->reg_succ = false;
	}
	/* 2. registe CQ interrupt*/
	for (cq_id = 0; cq_id < higgs_card->higgs_can_cq; cq_id++) {
		int_info = &higgs_card->cq_irq[cq_id];
		if (NULL == g_pfnCQIntIsr[cq_id]) {
			continue;
		}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)	/* linux 3.19 */
		/* cq interrupt number begin at index=80 */
		int_info->irq_num =
		    ll_card->card_cfg.intr_table[SAS_CQ_MSI_INT_ID_OFFSET + cq_id];
#else
		if (card_id == P660_SAS_CORE_DSAF_ID)
			int_info->irq_num = SAS_DSAF_MSI_CQ_INT(cq_id);
		else if (card_id == P660_SAS_CORE_PCIE_ID)
			int_info->irq_num = SAS_PCIE_MSI_CQ_INT(cq_id);
		else if (P660_1_SAS_CORE_DSAF_ID == card_id)
			int_info->irq_num = SLAVE_SAS_DSAF_MSI_CQ_INT(cq_id);
		else if (P660_1_SAS_CORE_PCIE_ID == card_id)
			int_info->irq_num = SLAVE_SAS_PCIE_MSI_CQ_INT(cq_id);
#endif
		int_info->int_handler = g_pfnCQIntIsr[cq_id];
		(void)snprintf(int_info->intr_vec_name,
			       HIGGS_MAX_INTRNAME_LEN - 1,
			       (char *)"Higgs_CQ_%d", cq_id);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)	/* linux 3.19 */
		virtual_irq = int_info->irq_num;
#else
		if (OK !=
		    HIGGS_IC_ENABLE_MSI((s32) int_info->irq_num,
					&virtual_irq)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4415,
				    "Card:%d map hwirq %d to virq %d failed",
				    higgs_card->card_id, int_info->irq_num,
				    virtual_irq);
			goto FAIL;
		}
#endif
		int_info->irq_num = (u32) virtual_irq;
		if (OK !=
		    SAS_REQUEST_IRQ((u32) virtual_irq, int_info->int_handler,
				    (unsigned long)0, int_info->intr_vec_name,
				    higgs_card)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4415,
				    "Card:%d request IRQ %d failed",
				    higgs_card->card_id, int_info->irq_num);
			goto FAIL;
		}
		int_info->reg_succ = true;
	}

	return OK;
      FAIL:
	higgs_free_cq_irq(higgs_card);
	return ERROR;
}

static s32 higgs_init_chip_fatal_irq(struct higgs_card *ll_card)
{
	u32 i = 0;
	struct higgs_card *card = NULL;
	struct higgs_int_info *int_info = NULL;
	u32 card_id = 0;
	s32 virtual_irq = 0;
	u32 msi_int_id_start = 0;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	card = (struct higgs_card *)ll_card;
	card_id = card->card_id;

	if (card_id == P660_SAS_CORE_DSAF_ID) {
		msi_int_id_start = SAS_DSAF_MSI_INT_ID_START;
	} else if (card_id == P660_SAS_CORE_PCIE_ID) {
		msi_int_id_start = SAS_PCIE_MSI_INT_ID_START;
	} else if (P660_1_SAS_CORE_DSAF_ID == card_id) {
		msi_int_id_start = SLAVE_SAS_DSAF_MSI_INT_ID_START;
	} else if (P660_1_SAS_CORE_PCIE_ID == card_id) {
		msi_int_id_start = SLAVE_SAS_PCIE_MSI_INT_ID_START;
	} else {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4415, "Card:%d card_id ERROR !!",
			    card->card_id);
		return ERROR;
	}

	/* 1. Int info */
	for (i = 0; i < MSI_CHIP_FATAL_BUTT; i++) {
		int_info = &card->chip_fatal_irq[i];
		int_info->reg_succ = false;
	}
	/* 2. Chip Fatal function handle*/
	for (i = 0; i < MSI_CHIP_FATAL_BUTT; i++) {
		int_info = &card->chip_fatal_irq[i];
		if (NULL == g_pfnChipFatalIntIsr[i])
			continue;

		/* CodeCC */
#if 0
		switch (uiLoop) {
		case MSI_CHIP_FATAL_HGC_ECC:
			pstIntInfo->irq_num =
			    uiMsiIntIdStart + MSI_HGC_ECC_INT_ID_OFFSET;
			break;
		case MSI_CHIP_FATAL_HGC_AXI:
			pstIntInfo->irq_num =
			    uiMsiIntIdStart + MSI_HGC_AXI_INT_ID_OFFSET;
			break;
			//case MSI_CHIP_FATAL_HGC_ERR_XFER_RDY: break;
		default:
			HIGGS_TRACE(OSP_DBL_MAJOR, 4415,
				    "Card:%d ERROR MSI INT type of chip fatal !!",
				    card->card_id);
			goto FAIL;
		}
#else
		if (MSI_CHIP_FATAL_HGC_ECC == i) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)	/* linux 3.19 */
			/* fatal error interrupt number begin at index=112 */
			int_info->irq_num = ll_card->card_cfg.intr_table[112];
#else
			int_info->irq_num =
			    msi_int_id_start + MSI_HGC_ECC_INT_ID_OFFSET;
#endif
		} else if (MSI_CHIP_FATAL_HGC_AXI == i) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)	/* linux 3.19 */
			/* fatal error interrupt number begin at index=113 */
			int_info->irq_num = ll_card->card_cfg.intr_table[113];
#else
			int_info->irq_num =
			    msi_int_id_start + MSI_HGC_AXI_INT_ID_OFFSET;
#endif
		}
#endif
		/* CodeCC */

		int_info->int_handler = g_pfnChipFatalIntIsr[i];
		(void)snprintf(int_info->intr_vec_name,
			       HIGGS_MAX_INTRNAME_LEN - 1,
			       (char *)"Higgs_ChipFatal_%d", i);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)	/* linux 3.19 */
		virtual_irq = int_info->irq_num;
#else
		if (OK !=
		    HIGGS_IC_ENABLE_MSI((s32) int_info->irq_num,
					&virtual_irq)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4415,
				    "Card:%d map hwirq %d to virq %d failed",
				    card->card_id, int_info->irq_num,
				    virtual_irq);
			goto FAIL;
		}
#endif
		/* IRQ routine */
		int_info->irq_num = (u32) virtual_irq;
		if (OK !=
		    SAS_REQUEST_IRQ((u32) virtual_irq, int_info->int_handler,
				    (unsigned long)0, int_info->intr_vec_name,
				    card)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4415,
				    "Card:%d request %d IRQ failed",
				    card->card_id, int_info->irq_num);
			goto FAIL;
		}
		int_info->reg_succ = true;
	}
	return OK;
      FAIL:
	higgs_free_chip_fatal_irq(card);
	return ERROR;
}

static void higgs_free_phy_irq(struct higgs_card *ll_card)
{
	u32 phy_id = 0;
	u32 offset = 0;
	struct higgs_int_info *int_info = NULL;
	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		for (offset = 0; offset < MSI_PHY_BUTT; offset++) {
			int_info = &ll_card->phy_irq[offset][phy_id];
			if (true == int_info->reg_succ) {
				SAS_FREE_IRQ(int_info->irq_num, ll_card);
#if 1
				HIGGS_IC_DISABLE_MSI((s32) int_info->irq_num);
#endif
				int_info->reg_succ = false;
			}
		}
	}
}

static void higgs_free_cq_irq(struct higgs_card *ll_card)
{
	u32 cq_id = 0;
	struct higgs_int_info *int_info = NULL;
	for (cq_id = 0; cq_id < HIGGS_MAX_CQ_NUM; cq_id++) {
		int_info = &ll_card->cq_irq[cq_id];
		if (true == int_info->reg_succ) {
			SAS_FREE_IRQ(int_info->irq_num, ll_card);
#if 1
			HIGGS_IC_DISABLE_MSI((s32) int_info->irq_num);
#endif
			int_info->reg_succ = false;
		}
	}
}

static void higgs_free_chip_fatal_irq(struct higgs_card *ll_card)
{
	u32 i = 0;
	struct higgs_int_info *int_info = NULL;
	for (i = 0; i < MSI_CHIP_FATAL_BUTT; i++) {
		int_info = &ll_card->chip_fatal_irq[i];
		if (true == int_info->reg_succ) {

			SAS_FREE_IRQ(int_info->irq_num, ll_card);
#if 1
			HIGGS_IC_DISABLE_MSI((s32) int_info->irq_num);
#endif
			int_info->reg_succ = false;
		}
	}
}

#if 0
/*****************************************************************************
 函 数 名  : higgs_free_sp804_timer_irq
 功能描述  : 释放Sp804 Timer中断资源
 输入参数  : struct higgs_card *v_pstLLCard
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :
 修改历史      :
  1.日    期   : 2015年2月5日
    作    者   : z00171046
    修改内容   : 新生成函数
*****************************************************************************/
static void higgs_free_sp804_timer_irq(struct higgs_card *ll_card)
{
	struct higgs_int_info *int_info = &ll_card->sp804_timer_irq;

	if (int_info->reg_succ) {
		SAS_FREE_IRQ(int_info->irq_num, ll_card);
		memset(int_info, 0, sizeof(*int_info));
	}
	if (NULL != (void *)ll_card->sp804_timer_base) {
		SAS_IOUNMAP((void *)ll_card->sp804_timer_base);
		ll_card->sp804_timer_base = (u64) NULL;
	}
}
#endif

/*
 * avoid abnormal interrupt except phy down
 */
static void higgs_bypass_chip_bug_mask_abnormal_intr(struct higgs_card *ll_card,
						     u32 phy_id)
{
	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return);

	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_CHL_INT0_MSK_REG,
			     HIGGS_SAS_PORT_PHY_CHL_INTR_MASK0_ALL & ~PBIT0);
}

static void higgs_bypass_chip_bug_unmask_abnormal_intr(struct higgs_card
						       *ll_card, u32 phy_id)
{
	u32 value = 0;

	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return);

	value =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT0_REG);
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_CHL_INT0_REG,
			     value & ~PBIT0);

	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_CHL_INT0_MSK_REG,
			     HIGGS_SAS_PORT_PHY_CHL_INTR_MASK0_DEFAULT);
}

static void higgs_check_address_frame_error(struct higgs_card *ll_card,
					    u32 phy_id)
{
	U_TX_RX_AF_STATUS_ENQ unTxRxAfStatus;

	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return);

	/* check Address frame error*/
	unTxRxAfStatus.u32 =
	    (u32) HIGGS_PORT_REG_READ(ll_card, phy_id,
				      HISAS30HV100_PORT_REG_TX_RX_AF_STATUS_ENQ_REG);

	if (unTxRxAfStatus.bits.sl_txid_err)
		HIGGS_TRACE(OSP_DBL_MINOR, 4187,
			    "Card:%d phy %d TX identify frame error",
			    ll_card->card_id, phy_id);

	/*receive Identify err */
	if (unTxRxAfStatus.bits.sl_rxid_failtype)
		HIGGS_TRACE(OSP_DBL_MINOR, 4187,
			    "Card:%d phy %d RX identify frame error, type = 0x%x",
			    ll_card->card_id, phy_id,
			    unTxRxAfStatus.bits.sl_rxid_failtype);

	/* receive Open address frame error*/
	if (unTxRxAfStatus.bits.sl_rxop_failtype)
		HIGGS_TRACE(OSP_DBL_MINOR, 4187,
			    "Card:%d phy %d RX open frame error, type = 0x%x",
			    ll_card->card_id, phy_id,
			    unTxRxAfStatus.bits.sl_rxop_failtype);
	return;
}

static void higgs_phy_down(struct higgs_card *ll_card, u32 phy_id)
{
	s32 ret = 0;

	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return);

	if (higgs_is_phy_disabled(ll_card, phy_id)) {
		/* phy disable*/
		HIGGS_TRACE(OSP_DBL_INFO, 187,
			    "Card: %d phy %d ignore phy down while disabled",
			    ll_card->card_id, phy_id);
	} else {
		/* phy enable*/
		/*stop serdes fw timer */
		(void)higgs_stop_serdes_fw_timer(ll_card, phy_id);

		HIGGS_TRACE(OSP_DBL_INFO, 187,
			    "Card: %d phy %d to reset serdes lane",
			    ll_card->card_id, phy_id);
		higgs_serdes_lane_reset(ll_card, phy_id);

		/* PHY DOWN notice SAL */
		ret = higgs_notify_sal_phy_down(ll_card, phy_id);
		HIGGS_TRACE(OSP_DBL_INFO, 187,
			    "Card: %d phy %d notify sal phy down return %d",
			    ll_card->card_id, phy_id, ret);
	}

	return;
}

static void higgs_phy_identify_timeout(struct higgs_card *ll_card, u32 phy_id)
{
	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return);

	sal_intr_event2_reset_phy(ll_card->sal_card, phy_id);
}

/*
*PHY LOSS DOWRD SYNC
*/
static void higgs_phy_loss_dw_sync(struct higgs_card *ll_card, u32 phy_id)
{
	struct sal_phy *sal_phy = NULL;

	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return);

	sal_phy = ll_card->phy[phy_id].up_ll_phy;

	sal_phy->err_code.phy_err_loss_dw_syn += 1;
}

#if 0
/*****************************************************************************
 函 数 名  : higgs_irq_set_affinity
 功能描述  : 中断绑定
 输入参数  : u32 v_uiIrq,
 	 	 	const struct cpumask *cpumask
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
static s32 higgs_irq_set_affinity(u32 irq, const HIGGS_CPUMASK_S * cpu_mask)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 16, 0)
	return irq_set_affinity(irq, cpu_mask);
#else
	typedef s32(*PFN_IRQ_SET_AFFINITY) (u32, const HIGGS_CPUMASK_S *, bool);
	PFN_IRQ_SET_AFFINITY irq_set_affinity = NULL;

	irq_set_affinity =
	    (PFN_IRQ_SET_AFFINITY)
	    HIGGS_KALLSYMS_LOOKUP_NAME("__irq_set_affinity");
	if (NULL == irq_set_affinity) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4707,
			    "Locate __irq_set_affinity position failed");
		return ERROR;
	}
	return irq_set_affinity(irq, cpu_mask, false);
#endif
}
#endif


MSI_INT_HANDLER(higgs_phy_up, 0)
    MSI_INT_HANDLER(higgs_phy_up, 1)
    MSI_INT_HANDLER(higgs_phy_up, 2)
    MSI_INT_HANDLER(higgs_phy_up, 3)
    MSI_INT_HANDLER(higgs_phy_up, 4)
    MSI_INT_HANDLER(higgs_phy_up, 5)
    MSI_INT_HANDLER(higgs_phy_up, 6)
    MSI_INT_HANDLER(higgs_phy_up, 7)

    MSI_INT_HANDLER(higgs_phy_bcast, 0)
    MSI_INT_HANDLER(higgs_phy_bcast, 1)
    MSI_INT_HANDLER(higgs_phy_bcast, 2)
    MSI_INT_HANDLER(higgs_phy_bcast, 3)
    MSI_INT_HANDLER(higgs_phy_bcast, 4)
    MSI_INT_HANDLER(higgs_phy_bcast, 5)
    MSI_INT_HANDLER(higgs_phy_bcast, 6)
    MSI_INT_HANDLER(higgs_phy_bcast, 7)

    MSI_INT_HANDLER(higgs_phy_ctrl_rdy, 0)
    MSI_INT_HANDLER(higgs_phy_ctrl_rdy, 1)
    MSI_INT_HANDLER(higgs_phy_ctrl_rdy, 2)
    MSI_INT_HANDLER(higgs_phy_ctrl_rdy, 3)
    MSI_INT_HANDLER(higgs_phy_ctrl_rdy, 4)
    MSI_INT_HANDLER(higgs_phy_ctrl_rdy, 5)
    MSI_INT_HANDLER(higgs_phy_ctrl_rdy, 6)
    MSI_INT_HANDLER(higgs_phy_ctrl_rdy, 7)

    MSI_INT_HANDLER(higgs_phy_dma_err, 0)
    MSI_INT_HANDLER(higgs_phy_dma_err, 1)
    MSI_INT_HANDLER(higgs_phy_dma_err, 2)
    MSI_INT_HANDLER(higgs_phy_dma_err, 3)
    MSI_INT_HANDLER(higgs_phy_dma_err, 4)
    MSI_INT_HANDLER(higgs_phy_dma_err, 5)
    MSI_INT_HANDLER(higgs_phy_dma_err, 6)
    MSI_INT_HANDLER(higgs_phy_dma_err, 7)

    MSI_INT_HANDLER(higgs_phy_hot_plug_tout, 0)
    MSI_INT_HANDLER(higgs_phy_hot_plug_tout, 1)
    MSI_INT_HANDLER(higgs_phy_hot_plug_tout, 2)
    MSI_INT_HANDLER(higgs_phy_hot_plug_tout, 3)
    MSI_INT_HANDLER(higgs_phy_hot_plug_tout, 4)
    MSI_INT_HANDLER(higgs_phy_hot_plug_tout, 5)
    MSI_INT_HANDLER(higgs_phy_hot_plug_tout, 6)
    MSI_INT_HANDLER(higgs_phy_hot_plug_tout, 7)

    MSI_INT_HANDLER(higgs_phy_oob_restart, 0)
    MSI_INT_HANDLER(higgs_phy_oob_restart, 1)
    MSI_INT_HANDLER(higgs_phy_oob_restart, 2)
    MSI_INT_HANDLER(higgs_phy_oob_restart, 3)
    MSI_INT_HANDLER(higgs_phy_oob_restart, 4)
    MSI_INT_HANDLER(higgs_phy_oob_restart, 5)
    MSI_INT_HANDLER(higgs_phy_oob_restart, 6)
    MSI_INT_HANDLER(higgs_phy_oob_restart, 7)

    MSI_INT_HANDLER(higgs_ph_rx_hard_rst, 0)
    MSI_INT_HANDLER(higgs_ph_rx_hard_rst, 1)
    MSI_INT_HANDLER(higgs_ph_rx_hard_rst, 2)
    MSI_INT_HANDLER(higgs_ph_rx_hard_rst, 3)
    MSI_INT_HANDLER(higgs_ph_rx_hard_rst, 4)
    MSI_INT_HANDLER(higgs_ph_rx_hard_rst, 5)
    MSI_INT_HANDLER(higgs_ph_rx_hard_rst, 6)
    MSI_INT_HANDLER(higgs_ph_rx_hard_rst, 7)

    MSI_INT_HANDLER(higgs_phy_abnormal, 0)
    MSI_INT_HANDLER(higgs_phy_abnormal, 1)
    MSI_INT_HANDLER(higgs_phy_abnormal, 2)
    MSI_INT_HANDLER(higgs_phy_abnormal, 3)
    MSI_INT_HANDLER(higgs_phy_abnormal, 4)
    MSI_INT_HANDLER(higgs_phy_abnormal, 5)
    MSI_INT_HANDLER(higgs_phy_abnormal, 6)
    MSI_INT_HANDLER(higgs_phy_abnormal, 7)
    MSI_INT_HANDLER(higgs_phy_status_chg, 0)
    MSI_INT_HANDLER(higgs_phy_status_chg, 1)
    MSI_INT_HANDLER(higgs_phy_status_chg, 2)
    MSI_INT_HANDLER(higgs_phy_status_chg, 3)
    MSI_INT_HANDLER(higgs_phy_status_chg, 4)
    MSI_INT_HANDLER(higgs_phy_status_chg, 5)
    MSI_INT_HANDLER(higgs_phy_status_chg, 6)
    MSI_INT_HANDLER(higgs_phy_status_chg, 7)

    MSI_INT_HANDLER(higgs_phy_int_reg1, 0)
    MSI_INT_HANDLER(higgs_phy_int_reg1, 1)
    MSI_INT_HANDLER(higgs_phy_int_reg1, 2)
    MSI_INT_HANDLER(higgs_phy_int_reg1, 3)
    MSI_INT_HANDLER(higgs_phy_int_reg1, 4)
    MSI_INT_HANDLER(higgs_phy_int_reg1, 5)
    MSI_INT_HANDLER(higgs_phy_int_reg1, 6)
    MSI_INT_HANDLER(higgs_phy_int_reg1, 7)

    MSI_INT_HANDLER(higgs_oq_core_int, 0)
    MSI_INT_HANDLER(higgs_oq_core_int, 1)
    MSI_INT_HANDLER(higgs_oq_core_int, 2)
    MSI_INT_HANDLER(higgs_oq_core_int, 3)
    MSI_INT_HANDLER(higgs_oq_core_int, 4)
    MSI_INT_HANDLER(higgs_oq_core_int, 5)
    MSI_INT_HANDLER(higgs_oq_core_int, 6)
    MSI_INT_HANDLER(higgs_oq_core_int, 7)
    MSI_INT_HANDLER(higgs_oq_core_int, 8)
    MSI_INT_HANDLER(higgs_oq_core_int, 9)
    MSI_INT_HANDLER(higgs_oq_core_int, 10)
    MSI_INT_HANDLER(higgs_oq_core_int, 11)
    MSI_INT_HANDLER(higgs_oq_core_int, 12)
    MSI_INT_HANDLER(higgs_oq_core_int, 13)
    MSI_INT_HANDLER(higgs_oq_core_int, 14)
    MSI_INT_HANDLER(higgs_oq_core_int, 15)
    MSI_INT_HANDLER(higgs_oq_core_int, 16)
    MSI_INT_HANDLER(higgs_oq_core_int, 17)
    MSI_INT_HANDLER(higgs_oq_core_int, 18)
    MSI_INT_HANDLER(higgs_oq_core_int, 19)
    MSI_INT_HANDLER(higgs_oq_core_int, 20)
    MSI_INT_HANDLER(higgs_oq_core_int, 21)
    MSI_INT_HANDLER(higgs_oq_core_int, 22)
    MSI_INT_HANDLER(higgs_oq_core_int, 23)
    MSI_INT_HANDLER(higgs_oq_core_int, 24)
    MSI_INT_HANDLER(higgs_oq_core_int, 25)
    MSI_INT_HANDLER(higgs_oq_core_int, 26)
    MSI_INT_HANDLER(higgs_oq_core_int, 27)
    MSI_INT_HANDLER(higgs_oq_core_int, 28)
    MSI_INT_HANDLER(higgs_oq_core_int, 29)
    MSI_INT_HANDLER(higgs_oq_core_int, 30)
    MSI_INT_HANDLER(higgs_oq_core_int, 31)
