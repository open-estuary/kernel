/*
 * Copyright (C) huawei
 * dfm function
 */

#include "higgs_common.h"
#include "higgs_phy.h"
#include "higgs_pv660.h"
#include "higgs_dfm.h"
#include "higgs_intr.h"
#include "higgs_peri.h"

#define  HIGGS_PHY_IN_BITMAP(phy, bitmap)      ((((bitmap) >> (phy)) & 0x1) != 0)
static void higgs_mark_card_lpBak_flag(struct sal_card *sal_card);
static void higgs_un_mark_card_lpbak_flag(struct sal_card *sal_card);
static s32 higgs_phy_lpbak(struct higgs_card *ll_card, u32 phy_id,
			   u32 loop_type);
static s32 higgs_phy_inter_lpbak(struct higgs_card *ll_card, u32 phy_id);
static s32 higgs_phy_exter_lpbak(struct higgs_card *ll_card, u32 phy_id);
static s32 higgs_bitstream_start(struct sal_card *sal_card, void *info);
static s32 higgs_bitstr_end(struct sal_card *sal_card, void *info);
static s32 higgs_bitstr_query(struct sal_card *sal_card, void *info);
static s32 higgs_bitstr_clear(struct sal_card *sal_card, void *info);
static s32 higgs_bitstr_start_prbs_serdes_loop(struct higgs_card *ll_card,
					       u32 phy_id);
static s32 higgs_bitstr_start_cjpat_serdes_loop(struct higgs_card *ll_card,
						u32 phy_id);


/*
 * port loopback test
 * @argc_in: port_map, port infomation; loop_type, test type
 * @sal_card: card struct
 */
s32 higgs_port_loopback(struct sal_card *sal_card, void *argc_in)
{
	struct sal_loopback *loopback_info = NULL;
	struct higgs_card *ll_card = NULL;
	s32 ret = OK;
	u32 phy_id = 0;

	HIGGS_ASSERT(NULL != sal_card, return ERROR);
	ll_card = (struct higgs_card *)sal_card->drv_data;
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	loopback_info = (struct sal_loopback *)argc_in;
	HIGGS_ASSERT(NULL != loopback_info, return ERROR);

	/* set card flag to loopback test */
	higgs_mark_card_lpBak_flag(sal_card);

	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		if (!HIGGS_PHY_IN_BITMAP(phy_id, loopback_info->port_map))
			continue;

		if (OK !=
		    higgs_phy_lpbak(ll_card, phy_id,
				    loopback_info->loop_type)) {
			HIGGS_TRACE(OSP_DBL_MINOR, 4560,
				    "Card:%d phy:%d loop back test failed!",
				    sal_card->card_no, phy_id);
			ret = ERROR;
			break;
		}
	}

	/* clear card flag */
	higgs_un_mark_card_lpbak_flag(sal_card);

	return ret;
}

/*
 * bit stream opration entrance function
 * @sal_card, card struct
 * @v_enOper, bit stream test type
 * @v_pArgv, bit stram info
 */
s32 higgs_bit_stream_operation(struct sal_card * sal_card,
			       enum sal_bitstream_op_type oper, void *argv)
{
	s32 ret = ERROR;

	switch (oper) {
	case SAL_BITSTREAM_OPT_START:
		ret = higgs_bitstream_start(sal_card, argv);
		break;

	case SAL_BITSTREAM_OPT_END:
		ret = higgs_bitstr_end(sal_card, argv);
		break;

	case SAL_BITSTREAM_OPT_QUERY:
		ret = higgs_bitstr_query(sal_card, argv);
		break;

	case SAL_BITSTREAM_OPT_CLEAR:
		ret = higgs_bitstr_clear(sal_card, argv);
		break;

	case SAL_BITSTREAM_OPT_TX_ENABLE:
	case SAL_BITSTREAM_OPT_RX_ENABLE:
	case SAL_BITSTREAM_OPT_INSERT:
		ret = OK;
		break;
	case SAL_BITSTREAM_OPT_BUTT:
	default:
		HIGGS_TRACE(OSP_DBL_MAJOR, 4456, "Invalid opcode:%d", oper);
		ret = ERROR;
		break;
	}

	return ret;
}

/*
 * set card flag to loopback
 */
static void higgs_mark_card_lpBak_flag(struct sal_card *sal_card)
{
	unsigned long flag = 0;

	spin_lock_irqsave(&sal_card->card_lock, flag);
	sal_card->flag &= ~SAL_CARD_ACTIVE;
	sal_card->flag |= SAL_CARD_LOOPBACK;
	spin_unlock_irqrestore(&sal_card->card_lock, flag);
}

/*
 * set card flag to active
 */
static void higgs_un_mark_card_lpbak_flag(struct sal_card *sal_card)
{
	unsigned long flag = 0;

	spin_lock_irqsave(&sal_card->card_lock, flag);
	sal_card->flag &= ~SAL_CARD_LOOPBACK;
	sal_card->flag |= SAL_CARD_ACTIVE;
	spin_unlock_irqrestore(&sal_card->card_lock, flag);
}

/*
 * phy loopback test entrance
 * @loop_type loopback type, internal or external
 */
static s32 higgs_phy_lpbak(struct higgs_card *ll_card,
			   u32 phy_id, u32 loop_type)
{
	s32 ret = ERROR;

	switch (loop_type) {
	case SAL_INT_LOOP_BACK:
		/*internal loopback */
		ret = higgs_phy_inter_lpbak(ll_card, phy_id);
		break;

	case SAL_EXT_LOOP_BACK:
		/*external loopback */
		ret = higgs_phy_exter_lpbak(ll_card, phy_id);
		break;

	default:
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_MINOR, msecs_to_jiffies(3000),
				     4558, "Card:%d invalid loopback type:%d",
				     ll_card->sal_card->card_no, loop_type);
		break;
	}

	return ret;
}

/*
 * PHY internal loopback test
 * @v_pstLLCard, higgs card structure
 * @phy_id,	phy id
 */
static s32 higgs_phy_inter_lpbak(struct higgs_card *ll_card, u32 phy_id)
{
	U_PHY_BIST_CTRL phy_bist_ctrl;
	U_CHL_INT2 chl_int2;
	U_CHL_INT0_MSK chl_int0_msk_old;
	U_CHL_INT1_MSK chl_int1_msk_old;
	U_CHL_INT2_MSK chl_int2_msk_old;
	u64 sas_addr = 0;
	s32 ret = OK;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

	/* Stop phy */
	if (OK != higgs_stop_phy(ll_card, phy_id)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 171, "Card:%d Phy %d stop failed",
			    ll_card->card_id, phy_id);
		return ERROR;
	}

	SAS_MSLEEP(10);

	/* Preserve INT 0/1/2 mask */
	chl_int0_msk_old.u32 =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT0_MSK_REG);
	chl_int1_msk_old.u32 =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT1_MSK_REG);
	chl_int2_msk_old.u32 =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT2_MSK_REG);

	/* Mask all INT 0/1/2 */
	higgs_phy_mask_intr(ll_card, phy_id);

	/* Clear INT 0/1/2 status */
	higgs_phy_clear_intr_status(ll_card, phy_id);

	/* Config identify frame */
	sas_addr = ((u64) ll_card->card_cfg.phy_addr_high[phy_id] << 32)
	    | ((u64) ll_card->card_cfg.phy_addr_low[phy_id]);;
	(void)higgs_config_identify_frame(ll_card, phy_id, sas_addr);

	/* Start loop: Config PHY_BIST_CTRL */
	phy_bist_ctrl.u32 =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_PHY_BIST_CTRL_REG);
	phy_bist_ctrl.bits.cfg_loop_test = 0x1;	/* enable loop test  */
	phy_bist_ctrl.bits.cfg_loop_test_mode = 0x2;	/* link loop */
	phy_bist_ctrl.bits.cfg_bist_test = 0x0;
	phy_bist_ctrl.bits.cfg_bist_mode_sel = 0x0;
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_PHY_BIST_CTRL_REG,
			     phy_bist_ctrl.u32);

	SAS_MSLEEP(500);

	/* Check if succeeded */
	chl_int2.u32 =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_CHL_INT2_REG);
	ret = chl_int2.bits.sl_phy_enabled ? OK : ERROR;

	/* End loop: Clear PHY_BIST_CTRL */
	phy_bist_ctrl.u32 =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_PHY_BIST_CTRL_REG);
	phy_bist_ctrl.bits.cfg_loop_test = 0x0;
	phy_bist_ctrl.bits.cfg_loop_test_mode = 0x0;
	phy_bist_ctrl.bits.cfg_bist_test = 0x0;
	phy_bist_ctrl.bits.cfg_bist_mode_sel = 0x0;
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_PHY_BIST_CTRL_REG,
			     phy_bist_ctrl.u32);

	/* Clear INT 0/1/2 status */
	higgs_phy_clear_intr_status(ll_card, phy_id);

	/* Restore INT 0/1/2 mask */
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_CHL_INT0_MSK_REG,
			     chl_int0_msk_old.u32);
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_CHL_INT1_MSK_REG,
			     chl_int1_msk_old.u32);
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_CHL_INT2_MSK_REG,
			     chl_int2_msk_old.u32);

	/* Clear error statistic */
	sal_clr_phy_err_stat(ll_card->sal_card, phy_id);

	return ret;
}

/*
 * PHY external loopback
 * @ll_card, Higgs card structure
 * @phy_id, phy id
 */
static s32 higgs_phy_exter_lpbak(struct higgs_card *ll_card, u32 phy_id)
{
	u8 link_status = 0;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

	/* PHY link status check */
	link_status = ll_card->phy[phy_id].up_ll_phy->link_status;
	if (SAL_PHY_LINK_UP == link_status)
		return OK;

	HIGGS_TRACE(OSP_DBL_MINOR, 171,
		    "Card:%d PHY %d is not link up, status = 0x%x",
		    ll_card->card_id, phy_id, link_status);

	/* clear bit error */
	sal_clr_phy_err_stat(ll_card->sal_card, phy_id);

	return ERROR;

}

/*
 * start bit stream test
 * @info, bit stream info
 */
static s32 higgs_bitstream_start(struct sal_card *sal_card, void *info)
{
	struct higgs_card *ll_card = NULL;
	struct sal_bit_stream_info *bitstr_info = NULL;
	struct sal_led_op_param led_param;
	U_CHL_INT0 chl_int0;
	u32 phy_id = 0;
	u32 bitstr_type = 0;
	u32 port_bitmap = 0;

	HIGGS_ASSERT(NULL != sal_card, return ERROR);
	ll_card = (struct higgs_card *)sal_card->drv_data;
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	bitstr_info = info;
	HIGGS_ASSERT(NULL != bitstr_info, return ERROR);
	bitstr_type = bitstr_info->bitstream_type;
	HIGGS_ASSERT((SAL_BIT_STREAM_TYPE_PRBS == bitstr_type)
		     || (SAL_BIT_STREAM_TYPE_CJTPAT == bitstr_type),
		     return ERROR);
	port_bitmap = bitstr_info->port_map;

	/* set loopback flag */
	higgs_mark_card_lpBak_flag(sal_card);

	/* Stop phy */
	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		if (!HIGGS_PHY_IN_BITMAP(phy_id, port_bitmap))
			continue;

		if (OK != higgs_wait_for_stop_phy(ll_card, phy_id)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 171,
				    "Card:%d phy %d wait stop failed",
				    ll_card->card_id, phy_id);

			/* stop test, return */
			higgs_un_mark_card_lpbak_flag(sal_card);
			return ERROR;
		}

		/* Turn off LED */
		memset(&led_param, 0, sizeof(led_param));
		led_param.phy_id = phy_id;
		led_param.port_speed_led = SAL_LED_OFF;
		(void)higgs_led_operation(sal_card, &led_param);
	}

	/* Start bitstream */
	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		if (0 == ((0x1 >> phy_id) & port_bitmap))
			continue;

		/* Disable err isolation */
		sal_card->phy[phy_id]->err_code.bit_err_enable = false;

		/* Clear INT 0 status */
		chl_int0.u32 =
		    HIGGS_PORT_REG_READ(ll_card, phy_id,
					HISAS30HV100_PORT_REG_CHL_INT0_REG);
		HIGGS_PORT_REG_WRITE(ll_card, phy_id,
				     HISAS30HV100_PORT_REG_CHL_INT0_REG,
				     chl_int0.u32);

		/* Set phy OOB link rate */
		(void)higgs_config_oob_link_rate(ll_card, phy_id,
						 HIGGS_CFG_PROG_OOB_PHY_LINK_RATE_12_0_G);

		/* Start bitstream */
		if (SAL_BIT_STREAM_TYPE_PRBS == bitstr_type)	/* PRBS */
			(void)higgs_bitstr_start_prbs_serdes_loop(ll_card,
								  phy_id);
		else		/* CJPAT */
			(void)higgs_bitstr_start_cjpat_serdes_loop(ll_card,
								   phy_id);
	}

	return OK;
}

/*
 * PRBS bitstream test
 * @ll_card, Higgs card structure
 * @phy_id, phy id
 */
static s32 higgs_bitstr_start_prbs_serdes_loop(struct higgs_card *ll_card,
					       u32 phy_id)
{
	u32 bist_code = 0;
	u32 bist_chk_time = 0;
	U_PHY_BIST_CTRL phy_bist_ctrl;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

	/* BIST code */
	bist_code = HIGGS_PHY_BIST_CODE_DEFAULT;
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_PHY_BIST_CODE_REG,
			     bist_code);

	/* BIST check time */
	bist_chk_time = 0;
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_PHY_BIST_CHK_TIME_REG,
			     bist_chk_time);

	/* BIST ctrl */
	phy_bist_ctrl.u32 =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_PHY_BIST_CTRL_REG);
	phy_bist_ctrl.bits.cfg_loop_test = 0x0;
	phy_bist_ctrl.bits.cfg_loop_test_mode = 0x1;	/* serdes loop */
	phy_bist_ctrl.bits.cfg_bist_test = 0x1;	/* enable bist test */
	phy_bist_ctrl.bits.cfg_bist_mode_sel = 0x2;	/* PRBS31 */
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_PHY_BIST_CTRL_REG,
			     phy_bist_ctrl.u32);

	return OK;
}

/*
 * cjpat bitstream test
 * @ll_card, higgs card structure
 * @phy_id, phy id
 */
static s32 higgs_bitstr_start_cjpat_serdes_loop(struct higgs_card *ll_card,
						u32 phy_id)
{
	u32 bist_code = 0;
	u32 bist_chk_time = 0;
	U_PHY_BIST_CTRL phy_bist_ctrl;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

	/* BIST code */
	bist_code = HIGGS_PHY_BIST_CODE_DEFAULT;
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_PHY_BIST_CODE_REG,
			     bist_code);

	/* BIST check time */
	bist_chk_time = 0;
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_PHY_BIST_CHK_TIME_REG,
			     bist_chk_time);

	/* BIST ctrl */
	phy_bist_ctrl.u32 =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_PHY_BIST_CTRL_REG);
	phy_bist_ctrl.bits.cfg_loop_test = 0x0;
	phy_bist_ctrl.bits.cfg_loop_test_mode = 0x1;	/* serdes loop */
	phy_bist_ctrl.bits.cfg_bist_test = 0x1;	/* enable bist test */
	phy_bist_ctrl.bits.cfg_bist_mode_sel = 0x3;	/* PJPAT */
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_PHY_BIST_CTRL_REG,
			     phy_bist_ctrl.u32);

	return OK;
}

/*
 * stop bitstream test
 * @sal_card, sal card structure
 * @info, bit stream info
 */
static s32 higgs_bitstr_end(struct sal_card *sal_card, void *info)
{
	U_PHY_BIST_CTRL phy_bist_ctrl;
	struct higgs_card *ll_card = NULL;
	struct sal_bit_stream_info *bitstr_info = NULL;
	u32 phy_id = 0;
	u32 port_bitmap = 0;

	HIGGS_ASSERT(NULL != sal_card, return ERROR);
	ll_card = (struct higgs_card *)sal_card->drv_data;
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	bitstr_info = info;
	HIGGS_ASSERT(NULL != bitstr_info, return ERROR);
	port_bitmap = bitstr_info->port_map;

	/* Stop bitstream */
	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		if (!HIGGS_PHY_IN_BITMAP(phy_id, port_bitmap))
			continue;

		phy_bist_ctrl.u32 =
		    HIGGS_PORT_REG_READ(ll_card, phy_id,
					HISAS30HV100_PORT_REG_PHY_BIST_CTRL_REG);
		phy_bist_ctrl.bits.cfg_bist_mode_sel = 0x0;
		phy_bist_ctrl.bits.cfg_bist_test = 0x0;
		phy_bist_ctrl.bits.cfg_loop_test = 0x0;
		phy_bist_ctrl.bits.cfg_loop_test_mode = 0;
		HIGGS_PORT_REG_WRITE(ll_card, phy_id,
				     HISAS30HV100_PORT_REG_PHY_BIST_CTRL_REG,
				     phy_bist_ctrl.u32);
	}

	/* clear loopback flag */
	higgs_un_mark_card_lpbak_flag(sal_card);

	return OK;
}

/*
 * bitstream inquire entrance
 * @sal_card, sal card
 * @info, bitstream info
 */
static s32 higgs_bitstr_query(struct sal_card *sal_card, void *info)
{
	struct higgs_card *ll_card = NULL;
	struct sal_bit_stream_info *bitstr_info = NULL;
	U_CHL_INT0 chl_int0;
	u32 phy_id = 0;
	u32 bitstr_type = 0;
	u32 *prbs_val = NULL;
	struct sal_bit_stream_err_code *err_code_nfo = NULL;
	u32 port_bitmap = 0;

	HIGGS_ASSERT(NULL != sal_card, return ERROR);
	ll_card = (struct higgs_card *)sal_card->drv_data;
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	bitstr_info = info;
	HIGGS_ASSERT(NULL != bitstr_info, return ERROR);
	bitstr_type = bitstr_info->bitstream_type;
	HIGGS_ASSERT((SAL_BIT_STREAM_TYPE_PRBS == bitstr_type)
		     || (SAL_BIT_STREAM_TYPE_CJTPAT == bitstr_type),
		     return ERROR);
	HIGGS_ASSERT(NULL != bitstr_info->argv_out, return ERROR);
	port_bitmap = bitstr_info->port_map;

	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		if (!HIGGS_PHY_IN_BITMAP(phy_id, port_bitmap))
			continue;

		/* Check INT 0 */
		chl_int0.u32 =
		    HIGGS_PORT_REG_READ(ll_card, phy_id,
					HISAS30HV100_PORT_REG_CHL_INT0_REG);
		if (chl_int0.bits.phyctrl_bist_check_err) {
			ll_card->phy[phy_id].diag_result.result = 1;
			HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO,
					     msecs_to_jiffies(300), 172,
					     "Card:%d Phy %d BIST encounter err",
					     ll_card->card_id, phy_id);
		} else {
			ll_card->phy[phy_id].diag_result.result = 0;
			HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO,
					     msecs_to_jiffies(300), 172,
					     "Card:%d Phy %d BIST no err",
					     ll_card->card_id, phy_id);
		}

		if (SAL_BIT_STREAM_TYPE_PRBS == bitstr_type) {
			/* PRBS */
			prbs_val = (u32 *) bitstr_info->argv_out;
			*prbs_val += ll_card->phy[phy_id].diag_result.result;
		} else {
			err_code_nfo =
			    (struct sal_bit_stream_err_code *)bitstr_info->
			    argv_out;
			err_code_nfo->invalid_dw_cnt +=
			    ll_card->phy[phy_id].diag_result.result;
		}
	}

	return OK;
}

/*
 * bitstream clear
 * @sal_card, sal card
 * @info, bitstream info
 */
static s32 higgs_bitstr_clear(struct sal_card *sal_card, void *info)
{
	struct higgs_card *ll_card = NULL;
	struct sal_bit_stream_info *bitstr_info = NULL;
	U_CHL_INT0 chl_int0;
	u32 phy_id = 0;
	u32 port_bitmap = 0;

	HIGGS_ASSERT(NULL != sal_card, return ERROR);
	ll_card = (struct higgs_card *)sal_card->drv_data;
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	bitstr_info = info;
	HIGGS_ASSERT(NULL != bitstr_info, return ERROR);
	port_bitmap = bitstr_info->port_map;

	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		if (!HIGGS_PHY_IN_BITMAP(phy_id, port_bitmap))
			continue;

		/* Clear INT 0 status */
		chl_int0.u32 =
		    HIGGS_PORT_REG_READ(ll_card, phy_id,
					HISAS30HV100_PORT_REG_CHL_INT0_REG);
		HIGGS_PORT_REG_WRITE(ll_card, phy_id,
				     HISAS30HV100_PORT_REG_CHL_INT0_REG,
				     chl_int0.u32);
	}

	return OK;
}
