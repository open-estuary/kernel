/*
 * Copyright (C) huawei
 * The program is used to process phy
 */


#include "higgs_common.h"
#include "higgs_pv660.h"
#include "higgs_phy.h"
#include "higgs_port.h"
/*#include "higgs_misc.h"*/
#include "higgs_intr.h"
#include "higgs_init.h"

#if !defined(FPGA_VERSION_TEST)
extern u32 SRE_SerdesLaneReset(u32 macro, u32 lane, u32 ds_cfg);
extern void SRE_SerdesEnableCTLEDFE(u32 macro, u32 lane, u32 ds_cfg);
#endif

#if defined(PV660_ARM_SERVER)
extern u32 higgs_comm_serdes_lane_reset(u32 node, u32 macro, u32 lane,
					u32 ds_cfg);
extern void higgs_comm_serdes_enable_ctledfe(u32 node, u32 macro, u32 lane,
					     u32 ds_cfg);
extern void higgs_oper_led_by_event(struct higgs_card *ll_card,
				    struct higgs_led_event_notify *led_event);

#endif


struct higgs_phy_serdes_param {
	bool tx_force;
	u8 tx_pre;
	u8 tx_main;
	u8 tx_post;
	u8 rx_clte;
};

#define  HIGGS_G3_CONFIG_TO_SERDES_PARAM(g3_config, serdes_param)\
	do {\
		serdes_param.tx_pre =  ((g3_config) >> 24) & 0x3F;\
		serdes_param.tx_main = ((g3_config) >> 16) & 0x3F;\
		serdes_param.tx_post = ((g3_config) >> 8)  & 0x3F;\
		serdes_param.rx_clte = ((g3_config) >> 0)  & 0x7;\
	}while (0)\

#define  HIGGS_REG_PHY_CONFIG2_TO_SERDES_PARAM(phy_config2, serdes_param)\
	do {\
		serdes_param.rx_clte = (u8)phy_config2.bits.cfg_rxcltepreset;\
		serdes_param.tx_force = phy_config2.bits.cfg_force_txdeemph ? true : false;\
		serdes_param.tx_pre  = (u8)((phy_config2.bits.cfg_phy_txdeemph >> 12) & 0x3F);\
		serdes_param.tx_main = (u8)((phy_config2.bits.cfg_phy_txdeemph >> 6) & 0x3F);\
		serdes_param.tx_post = (u8)((phy_config2.bits.cfg_phy_txdeemph >> 0) & 0x3F);\
	}while (0)\

#define  HIGGS_SERDES_PARAM_TO_REG_PHY_CONFIG2(serdes_param, phy_config2)\
	do {\
		phy_config2.bits.cfg_rxcltepreset = serdes_param.rx_clte;\
		phy_config2.bits.cfg_force_txdeemph = serdes_param.tx_force ? 0x1 : 0x0;\
		phy_config2.bits.cfg_phy_txdeemph = ((((u32)(serdes_param.tx_pre)) & 0x3F) << 12)\
			| ((((u32)(serdes_param.tx_main)) & 0x3F) << 6) | ((((u32)(serdes_param.tx_post)) & 0x3F) << 0);\
	}while (0)\

static s32 higgs_query_phy_err_code(struct sal_card *sal_card, u32 phy_id);

static s32 higgs_local_phy_control(struct sal_card *card,
				   u32 phy_id, u32 phy_op);

static u32 higgs_config_link_param(struct higgs_card *ll_card,
				   u32 phy_id, u32 phy_rate);

static s32 higgs_operate_serdes_param(struct higgs_card *ll_card,
				      struct sal_phy_ll_op_param *phy_op_info);

static s32 higgs_write_serdes_param(struct higgs_card *ll_card,
				    u32 phy_id,
				    struct higgs_phy_serdes_param
				    *serdes_param);

static s32 higgs_read_serdes_param(struct higgs_card *ll_card,
				   u32 phy_id,
				   struct higgs_phy_serdes_param *serdes_param);

static s32 higgs_notify_sal_phy_start_rsp(struct higgs_card *ll_card,
					  u32 phy_id);

static s32 higgs_config_opt_mode(struct higgs_card *ll_card, u32 phy_id);

static s32 higgs_config_tx_ffe_auto_negotiation(struct higgs_card *ll_card,
						u32 phy_id);

static s32 higgs_enable_phy(struct higgs_card *ll_card, u32 phy_id);

static s32 higgs_disable_phy(struct higgs_card *ll_card, u32 phy_id);

/*
 * operation phy 
 */
s32 higgs_phy_operation(struct sal_card *sal_card,
			enum sal_phy_op_type phy_opt, void *argv_in)
{
	s32 iret = ERROR;
	u32 phy_id = 0;
	struct higgs_card *ll_card = NULL;
	struct sal_phy_ll_op_param *ll_phy_opt_info = NULL;
	u64 ll_sasaddr = 0;

	HIGGS_ASSERT(NULL != sal_card, return ERROR);
	HIGGS_ASSERT(NULL != argv_in, return ERROR);

	ll_card = (struct higgs_card *)sal_card->drv_data;
	ll_phy_opt_info = (struct sal_phy_ll_op_param *)argv_in;
	phy_id = ll_phy_opt_info->phy_id;

	/* phy operation */
	switch (phy_opt) {
	case SAL_PHY_OPT_OPEN:
		ll_sasaddr = sal_card->phy[phy_id]->local_addr;
		iret =
		    higgs_start_phy(ll_card, phy_id, ll_sasaddr,
				    ll_phy_opt_info->phy_rate);
		break;
	case SAL_PHY_OPT_CLOSE:
		iret = higgs_stop_phy(ll_card, phy_id);
		break;
	case SAL_PHY_OPT_LOCAL_CONTROL:	/* PHY direct connect condition*/
		iret =
		    higgs_local_phy_control(sal_card, phy_id,
					    ll_phy_opt_info->op_code);
		break;
	case SAL_PHY_OPT_GET_LINK:
		break;
	case SAL_PHY_OPT_QUERY_ERRCODE:
		iret = higgs_query_phy_err_code(sal_card, phy_id);
		break;
	case SAL_PHY_OPT_EMPHASIS_AMPLITUDE_OPER:
		iret = higgs_operate_serdes_param(ll_card, ll_phy_opt_info);
		break;
	case SAL_PHY_OPT_RESET:
		iret = ERROR;
		break;
	default:
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_MAJOR, msecs_to_jiffies(3000),
				     4617, "Card:%d unknown phy opcode:%d ",
				     sal_card->card_no, phy_opt);
		iret = ERROR;
		break;
	}
	return iret;
}

/*
 * start phy 
 */
s32 higgs_start_phy(struct higgs_card * ll_card,
		    u32 phy_id, u64 ll_sasaddr, u32 phy_rate)
{
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

	HIGGS_TRACE(OSP_DBL_INFO, 4311,
		    "Card:%d is going to start phy:%d(rate:%d, sasaddr:0x%016llx)",
		    ll_card->card_id, phy_id, phy_rate, ll_sasaddr);

	if (OK != higgs_config_identify_frame(ll_card, phy_id, ll_sasaddr)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4617,
			    "Card:%d config identify frame failed with phy:%d(addr:0x%016llx)",
			    ll_card->card_id, phy_id, ll_sasaddr);
		return ERROR;
	}

	/*set Link */
	if (OK != higgs_config_link_param(ll_card, phy_id, phy_rate)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4617,
			    "Card:%d config link param failed with phy:%d(rate:%d)",
			    ll_card->card_id, phy_id, phy_rate);
		return ERROR;
	}

	/* line type is PT_MODE */
	(void)higgs_config_opt_mode(ll_card, phy_id);

	/*set TX FFE parameter*/
	(void)higgs_config_tx_ffe_auto_negotiation(ll_card, phy_id);

	/* phy start response process*/
	(void)higgs_notify_sal_phy_start_rsp(ll_card, phy_id);

	/* enable PHY*/
	(void)higgs_enable_phy(ll_card, phy_id);

	return OK;
}

/*
 * stop phy 
 */
s32 higgs_stop_phy(struct higgs_card * ll_card, u32 phy_id)
{
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

	HIGGS_TRACE(OSP_DBL_INFO, 4311, "Card:%d is going to stop phy:%d",
		    ll_card->card_id, phy_id);

	/* simulate phy stop response interrupt*/
	higgs_simulate_phy_stop_rsp_intr(ll_card, phy_id);

	HIGGS_MDELAY(2);	/* delay 2ms£¬wait PHY STOP RSP finish */

	/* disable PHY  */
	(void)higgs_disable_phy(ll_card, phy_id);

	/* stop serdes fw timer */
	(void)higgs_stop_serdes_fw_timer(ll_card, phy_id);

	/* reset serdes */
	HIGGS_TRACE(OSP_DBL_INFO, 187, "Card: %d phy %d to reset serdes lane",
		    ll_card->card_id, phy_id);

	higgs_serdes_lane_reset(ll_card, phy_id);

	return OK;
}

/*
 * check phy whether it is disabled 
 */

bool higgs_is_phy_disabled(struct higgs_card * ll_card, u32 phy_id)
{
	U_PHY_CFG phy_cfg;

	HIGGS_ASSERT(NULL != ll_card, return true);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return true);

	/* read PHY_CFG reg */
	phy_cfg.u32 =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_PHY_CFG_REG);

	return (HIGGS_CFG_PHY_DISABLE == phy_cfg.bits.cfg_phy_enabled);
}

void higgs_close_all_phy(struct higgs_card *ll_card)
{
	u32 phy_id = 0;

	HIGGS_ASSERT(NULL != ll_card, return);

	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++)
		(void)higgs_stop_phy(ll_card, phy_id);

	return;
}

/*
 * stop phy,waiting until phy is closed
 */
s32 higgs_wait_for_stop_phy(struct higgs_card * ll_card, u32 phy_id)
{
	unsigned long wait_end_time = 0;

	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

	if (SAL_PHY_CLOSED == ll_card->sal_card->phy[phy_id]->link_status) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4443,
			    "Card:%d Phy %d already closed!", ll_card->card_id,
			    phy_id);
		return OK;
	}

	if (OK != higgs_stop_phy(ll_card, phy_id)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4445, "Card:%d Phy %d stop failed!",
			    ll_card->card_id, phy_id);
		return ERROR;
	}

	ll_card->sal_card->phy[phy_id]->err_code.last_switch_jiff = jiffies;

	/* waiting phy is closed */
	wait_end_time = jiffies + msecs_to_jiffies(1000);
	while (!time_after(jiffies, wait_end_time)) {
		if (SAL_PHY_CLOSED ==
		    ll_card->sal_card->phy[phy_id]->link_status)
			return OK;

		SAS_MSLEEP(10);
	}

	HIGGS_TRACE(OSP_DBL_MAJOR, 4446, "Card:%d Phy %d wait ack timeout",
		    ll_card->card_id, phy_id);
	return ERROR;
}

void higgs_serdes_lane_reset(struct higgs_card *ll_card, u32 phy_id)
{
	u32 dsapi = 0;
#if defined(PV660_ARM_SERVER)
	u32 hilinkid;
#endif

	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return);

#if defined(EVB_VERSION_TEST)
	if (P660_SAS_CORE_PCIE_ID == ll_card->card_id) {
		dsapi = (phy_id % 4);	/* phy 4~7  <->  lane 0-3 */
		(void)SRE_SerdesLaneReset(6, dsapi, 6);	/* HiLink6 */
	}
#elif defined(C05_VERSION_TEST)
	if (P660_SAS_CORE_DSAF_ID == ll_card->card_id) {
		dsapi = phy_id;	/* phy 0~7  <->  lane 0-7 */
		(void)SRE_SerdesLaneReset(2, dsapi, 6);	/* HiLink2 */
	}
#elif defined(PV660_ARM_SERVER)
/* 1p arm server for linux 3.19,2P ARM SERVER is not support linux 3.19 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)	/* linux 3.19 */
	/* cpu node 0 sas0 */
	if (HIGGS_HILINK_TYPE_DSAF == ll_card->card_cfg.hilink_type) {
		dsapi = phy_id;	/* phy 0~7  <->  lane 0-7 */

		printk("dsaf: cpu_node_id=%d\n", ll_card->card_cfg.cpu_node_id);
		(void)higgs_comm_serdes_lane_reset(ll_card->card_cfg.cpu_node_id, 2, dsapi, 6);	/* HiLink2 */
	}

	/* cpu node 0 sas1 */
	if (HIGGS_HILINK_TYPE_PCIE == ll_card->card_cfg.hilink_type) {
		if (phy_id < 4)
			hilinkid = 5;
		else
			hilinkid = 6;

		dsapi = phy_id % 4;	/* phy 0~3  <-> hilink5 lane 0-3 , phy 4~7 : hilink6 lane 0-3 */

		printk("pcie: cpu_node_id=%d\n", ll_card->card_cfg.cpu_node_id);
		(void)higgs_comm_serdes_lane_reset(ll_card->card_cfg.cpu_node_id, hilinkid, dsapi,
						   6);
	}
#else
	/* cpu node 0 sas0 */
	if (P660_SAS_CORE_DSAF_ID == ll_card->card_id) {
		dsapi = phy_id;	/* phy 0~7  <->  lane 0-7 */
		(void)higgs_comm_serdes_lane_reset(0, 2, dsapi, 6);	/* HiLink2 */
	}

	/* cpu node 0 sas1 */
	if (P660_SAS_CORE_PCIE_ID == ll_card->card_id) {
		if (phy_id < 4)
			hilinkid = 5;
		else
			hilinkid = 6;

		dsapi = phy_id % 4;	/* phy 0~3  <-> hilink5 lane 0-3 , phy 4~7 : hilink6 lane 0-3 */

		(void)higgs_comm_serdes_lane_reset(0, hilinkid, dsapi, 6);
	}

	/* cpu node 1 sas0 */
	if (P660_1_SAS_CORE_DSAF_ID == ll_card->card_id) {
		dsapi = phy_id;	/* phy 0~7  <->  lane 0-7 */
		(void)higgs_comm_serdes_lane_reset(1, 2, dsapi, 6);	/* HiLink2 */
	}

	/* cpu node 1 sas1 */
	if (P660_1_SAS_CORE_PCIE_ID == ll_card->card_id) {
		if (phy_id < 4)
			hilinkid = 5;
		else
			hilinkid = 6;

		dsapi = phy_id % 4;	/* phy 0~3  <-> hilink5 lane 0-3 , phy 4~7 : hilink6 lane 0-3 */

		(void)higgs_comm_serdes_lane_reset(1, hilinkid, dsapi, 6);
	}
#endif

#else
	HIGGS_REF(ll_card);
	HIGGS_REF(phy_id);
	HIGGS_REF(dsapi);	/* pclint */
#endif
	return;
}

void higgs_serdes_enable_ctledfe(struct higgs_card *ll_card, u32 phy_id)
{
	u32 dsapi = 0;
#if defined(PV660_ARM_SERVER)
	u32 hilinkid;
#endif

	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return);

#if defined(EVB_VERSION_TEST)
	if (P660_SAS_CORE_PCIE_ID == ll_card->card_id) {
		dsapi = (phy_id % 4);	/* phy 4~7 <-> lane 0-3 */
		SRE_SerdesEnableCTLEDFE(6, dsapi, 9);	/* HiLink6 */
	}
#elif defined(C05_VERSION_TEST)
	if (P660_SAS_CORE_DSAF_ID == ll_card->card_id) {
		dsapi = phy_id;	/* phy 0~7  <->  lane 0-7 */
		SRE_SerdesEnableCTLEDFE(2, dsapi, 9);	/* HiLink2 */
	}

#elif defined(PV660_ARM_SERVER)

/* 1p arm server for linux 3.19,2P ARM SERVER is not support linux 3.19 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)	/* linux 3.19 */

	/* cpu node 0 sas0 */
	if (HIGGS_HILINK_TYPE_DSAF == ll_card->card_cfg.hilink_type) {
		dsapi = phy_id;	/* phy 0~7  <->  lane 0-7 */
		printk("dsaf: cpu_node_id=%d\n", ll_card->card_cfg.cpu_node_id);
		(void)higgs_comm_serdes_enable_ctledfe(ll_card->card_cfg.cpu_node_id, 2, dsapi, 9);	/* HiLink2 */
	}

	/* cpu node 0 sas1 */
	if (HIGGS_HILINK_TYPE_PCIE == ll_card->card_cfg.hilink_type) {
		if (phy_id < 4)
			hilinkid = 5;
		else
			hilinkid = 6;

		dsapi = phy_id % 4;	/* phy 0~3  <-> hilink5 lane 0-3 , phy 4~7 : hilink6 lane 0-3 */
		printk("pcie: cpu_node_id=%d\n", ll_card->card_cfg.cpu_node_id);
		(void)higgs_comm_serdes_enable_ctledfe(ll_card->card_cfg.cpu_node_id, hilinkid,
						       dsapi, 9);
	}
#else
	/* cpu node 0 sas0 */
	if (P660_SAS_CORE_DSAF_ID == ll_card->card_id) {
		dsapi = phy_id;	/* phy 0~7  <->  lane 0-7 */
		(void)higgs_comm_serdes_enable_ctledfe(0, 2, dsapi, 9);	/* HiLink2 */
	}

	/* cpu node 0 sas1 */
	if (P660_SAS_CORE_PCIE_ID == ll_card->card_id) {
		if (phy_id < 4)
			hilinkid = 5;
		else
			hilinkid = 6;

		dsapi = phy_id % 4;	/* phy 0~3  <-> hilink5 lane 0-3 , phy 4~7 : hilink6 lane 0-3 */

		(void)higgs_comm_serdes_enable_ctledfe(0, hilinkid, dsapi, 9);
	}

	/* cpu node 1 sas0 */
	if (P660_1_SAS_CORE_DSAF_ID == ll_card->card_id) {
		dsapi = phy_id;	/* phy 0~7  <->  lane 0-7 */
		(void)higgs_comm_serdes_enable_ctledfe(1, 2, dsapi, 9);	/* HiLink2 */
	}

	/* cpu node 1 sas1 */
	if (P660_1_SAS_CORE_PCIE_ID == ll_card->card_id) {
		if (phy_id < 4)
			hilinkid = 5;
		else
			hilinkid = 6;

		dsapi = phy_id % 4;	/* phy 0~3  <-> hilink5 lane 0-3 , phy 4~7 : hilink6 lane 0-3 */

		(void)higgs_comm_serdes_enable_ctledfe(1, hilinkid, dsapi, 9);
	}
#endif

#else
	HIGGS_REF(ll_card);
	HIGGS_REF(phy_id);
	HIGGS_REF(dsapi);
#endif
	return;
}

s32 higgs_init_serdes_param(struct higgs_card * ll_card)
{
	u32 phy_id = 0;
	u32 g3config = 0;
	struct higgs_phy_serdes_param serdes_param;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);

#if 0
	/* for each PHY£¬set parameter */
	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		if (OK !=
		    higgs_read_serdes_param(ll_card, phy_id, &serdes_param)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4401,
				    "Card:%d phy %d read serdes param failed",
				    ll_card->card_id, phy_id);
			return ERROR;
		}

		serdes_param.tx_force = !ll_card->sal_card->config.bct_enable;

		g3_config = ll_card->card_cfg.phy_g3_cfg[phy_id];
		HIGGS_G3_CONFIG_TO_SERDES_PARAM(g3_config, serdes_param);

		if (OK !=
		    higgs_write_serdes_param(ll_card, phy_id, &serdes_param)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4401,
				    "Card:%d phy %d write serdes param failed",
				    ll_card->card_id, phy_id);
			return ERROR;
		}
	}
#else
	HIGGS_REF(ll_card);
	HIGGS_REF(phy_id);
	HIGGS_REF(g3config);
	memset(&serdes_param, 0, sizeof(serdes_param));
	HIGGS_G3_CONFIG_TO_SERDES_PARAM(g3config, serdes_param);
#endif

	return OK;
}

s32 higgs_init_identify_frame(struct higgs_card * ll_card)
{
	u64 ll_sasaddr = HIGGS_DEFAULT_SAS_ADDR;
	u32 phy_id = 0;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);

	/* for each phy£¬set identify address frame parameter*/
	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		ll_sasaddr =
		    ((u64) ll_card->card_cfg.phy_addr_high[phy_id] << 32)
		    | ((u64) ll_card->card_cfg.phy_addr_low[phy_id]);
		if (OK !=
		    higgs_config_identify_frame(ll_card, phy_id, ll_sasaddr))
			HIGGS_TRACE(OSP_DBL_MINOR, 4617,
				    "Card:%d config identify frame failed with phy:%d(addr:0x%016llx)",
				    ll_card->card_id, phy_id, ll_sasaddr);
	}
	return OK;
}

s32 higgs_config_identify_frame(struct higgs_card * ll_card,
				u32 phy_id, u64 ll_sasaddr)
{
	struct sas_identify_frame identify_frame = { 0 };
	u32 *buffer = NULL;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

	memset(&identify_frame, 0, sizeof(identify_frame));
	identify_frame.tgt_proto = HIGGS_TGT_RESTRICTED;
	identify_frame.phy_identifier = (u8) phy_id;
	identify_frame.device_type = HIGGS_MANAGE_PROTOCOL_IN;
	if (SAL_PORT_MODE_INI == ll_card->card_cfg.work_mode)
		/* b3: SSP initiator port    b2: STP initiator port   b1: SMP initiator port */
		identify_frame.ini_proto = HIGGS_INI_SSP_STP_SMP;

	identify_frame.reason = HIGGS_IDENTIFY_ADDRESS_FRAME_REASON_POWER_ON;
	memcpy(&identify_frame.device_name[0], (void *)&ll_sasaddr,
	       sizeof(ll_sasaddr));

	memcpy(&identify_frame.sas_addr[0], (void *)&ll_sasaddr,
	       sizeof(ll_sasaddr));

	buffer = (u32 *) ((void *)&identify_frame);
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_TX_ID_DWORD0_REG,
			     HIGGS_ENDIAN_REVERT_32(buffer[0]));
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_TX_ID_DWORD1_REG, buffer[2]);
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_TX_ID_DWORD2_REG, buffer[1]);
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_TX_ID_DWORD3_REG, buffer[4]);
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_TX_ID_DWORD4_REG, buffer[3]);
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_TX_ID_DWORD5_REG,
			     HIGGS_ENDIAN_REVERT_32(buffer[5]));

	return OK;
}

void higgs_open_phy_own_wire(struct sal_card *sal_card)
{
	u32 phy_id = 0;
	struct sal_phy_ll_op_param ll_phy_opt_info = { 0 };

	HIGGS_ASSERT(NULL != sal_card, return);

	for (phy_id = 0; phy_id < sal_card->phy_num; phy_id++) {
		sal_card->phy[phy_id]->err_code.last_switch_jiff = jiffies;
		ll_phy_opt_info.phy_id = phy_id;
		ll_phy_opt_info.phy_rate =
		    sal_card->config.phy_link_rate[phy_id];
		if ((SAL_ELEC_CABLE == sal_card->phy[phy_id]->connect_wire_type)
		    || (SAL_OPTICAL_CABLE ==
			sal_card->phy[phy_id]->connect_wire_type))
			(void)higgs_phy_operation(sal_card, SAL_PHY_OPT_OPEN,
						  (void *)(&ll_phy_opt_info));
	}
	return;
}

void higgs_serdes_for_12g_timer_hander(unsigned long arg)
{
	u32 cardid = 0;
	u32 phy_id = 0;
	unsigned long flag = 0;
	struct sal_card *sal_card = NULL;
	struct higgs_card *higgs_card = NULL;

	phy_id = (u32) arg >> 16;
	cardid = (u32) 0x0000ffff & arg;
	sal_card = sal_get_card(cardid);
	HIGGS_ASSERT(NULL != sal_card, return);	/* CodeCC */

	higgs_card = (struct higgs_card *)sal_card->drv_data;
	HIGGS_ASSERT(NULL != higgs_card, return);

	HIGGS_TRACE(OSP_DBL_INFO, 171,
		    "Card:%d phy %d 12G serdes, enter timer Hander", cardid,
		    phy_id);

	spin_lock_irqsave(&higgs_card->card_lock, flag);
	/*card write reg under 3 status*/
	if ((sal_card->flag & SAL_CARD_REMOVE_PROCESS)
	    || (sal_card->flag & SAL_CARD_RESETTING)
	    || (sal_card->flag & SAL_CARD_FATAL)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 171,
			    "Card:%d phy %d 12G serdes, card status error, timer not execute",
			    cardid, phy_id);
		spin_unlock_irqrestore(&higgs_card->card_lock, flag);
		sal_put_card(sal_card);
		return;
	}

	if (higgs_card->phy[phy_id].run_serdes_fw_timer_en) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Card:%d phy %d 12G serdes, end TX training",
			    cardid, phy_id);
		(void)higgs_end_tx_training(higgs_card, phy_id);
		HIGGS_UDELAY(1);
		(void)higgs_clear_tx_training(higgs_card, phy_id);
	} else {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Card:%d phy %d 12G serdes, timer disable, not execute",
			    cardid, phy_id);
	}
	higgs_card->phy[phy_id].run_serdes_fw_timer_en = false;
	spin_unlock_irqrestore(&higgs_card->card_lock, flag);
	sal_put_card(sal_card);

	return;
}

/*
 * check dma tx rx
 */

void higgs_check_dma_txrx(unsigned long data)
{
	u32 card_id = 0;
	u32 phy_id = 0;
	struct higgs_card *higgs_card = NULL;
	struct sal_card *sal_card = NULL;
	struct higgs_led_event_notify led_event;

	phy_id = (u32) data >> 16;
	card_id = (u32) 0x0000ffff & data;
	sal_card = sal_get_card(card_id);

	higgs_card = (struct higgs_card *)sal_card->drv_data;
	HIGGS_ASSERT(NULL != higgs_card, return);

	/*lighter when it is different to the status of phy between last time and current*/
	if (higgs_card->phy[phy_id].last_time_idle !=
	    higgs_card->phy[phy_id].phy_is_idle) {
		if (higgs_card->phy[phy_id].phy_is_idle == FALSE) {
			/*Higgs_AddEventToLedHandler(pstHiggsCard, LED_DISK_NORMAL_IO, uiPhyId); */
			led_event.phy_id = phy_id;
			led_event.event_val = LED_DISK_NORMAL_IO;
			higgs_oper_led_by_event(higgs_card, &led_event);
		} else {
			/*Higgs_AddEventToLedHandler(pstHiggsCard, LED_DISK_PRESENT,  uiPhyId); */
			led_event.phy_id = phy_id;
			led_event.event_val = LED_DISK_PRESENT;
			higgs_oper_led_by_event(higgs_card, &led_event);
		}
	}

	higgs_card->phy[phy_id].last_time_idle =
	    higgs_card->phy[phy_id].phy_is_idle;
	higgs_card->phy[phy_id].phy_is_idle = true;

	if (!sal_timer_is_active
	    ((void *)&higgs_card->phy[phy_id].phy_dma_status_timer))
		sal_add_timer((void *)&higgs_card->phy[phy_id].
			      phy_dma_status_timer,
			      (unsigned long)((phy_id & 0xff) << 16 |
					      higgs_card->card_id),
			      (u32) msecs_to_jiffies(1000),
			      higgs_check_dma_txrx);
	else
		sal_mod_timer((void *)&higgs_card->phy[phy_id].
			      phy_dma_status_timer, msecs_to_jiffies(1000));

	sal_put_card(sal_card);
}

s32 higgs_config_serdes_for_12g(struct higgs_card *ll_card, u32 phy_id)
{
#if defined (FPGA_VERSION_TEST) || defined(EVB_VERSION_TEST)
#define DS_API(lane)           ((0x1FF6c + 8*(15-(u64)lane))*2)

	u32 retrycnt = 20;
	u32 regdata = 0;
	struct higgs_card *higgs_card = NULL;
	u32 uphyid = 0;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	higgs_card = (struct higgs_card *)ll_card;
	uphyid = phy_id;

	while (retrycnt--) {
		regdata =
		    HIGGS_PORT_REG_READ(higgs_card, uphyid,
					HISAS30HV100_PORT_REG_HARD_PHY_LINK_RATE_REG);
		if ((regdata & 0xF00) == 0xb00) {
			u32 ds = (uphyid + 4) % 8;
			mdelay(15);
			/*control 1 */
			higgs_reg_write_part(higgs_card->hilink_base +
					     DS_API(ds) + 0, 6, 6, 1);
			/*control 0 */
			higgs_reg_write_part(higgs_card->hilink_base +
					     (u64) DS_API(ds) + 4, 4, 7, 0x5);
			break;
		}
		mdelay(2);
	}

	if ((regdata & 0xF00) == 0xb00) {
		mdelay(300);
		higgs_reg_write_part((u64)
				     HIGGS_PORT_REG_ADDR(higgs_card, uphyid,
							 HISAS30HV100_PORT_REG_PHY_CONFIG2_REG),
				     24, 24, 1);
	}
    
#elif defined(C05_VERSION_TEST) || defined(PV660_ARM_SERVER)

	unsigned long flag = 0;
	unsigned long wait_end_time = 0;
	U_HARD_PHY_LINK_RATE hard_phy_link_rate;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

	hard_phy_link_rate.u32 = 0;
	wait_end_time = jiffies + msecs_to_jiffies(HIGGS_12G_NEGO_QUERY_WINDOW);
	while (!time_after(jiffies, wait_end_time)) {
		if (higgs_phy_down_intr_come(ll_card, phy_id)) {
			HIGGS_TRACE(OSP_DBL_MINOR, 187,
				    "Card %d phy %d phy down come while ctrl ready",
				    ll_card->card_id, phy_id);
			higgs_phy_clear_down_intr_status(ll_card, phy_id);
		}

		/* check rate is 12G?*/
		hard_phy_link_rate.u32 =
		    HIGGS_PORT_REG_READ(ll_card, phy_id,
					HISAS30HV100_PORT_REG_HARD_PHY_LINK_RATE_REG);
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, msecs_to_jiffies(2), 187,
				     "Card %d phy %d neg_link_rate = 0x%x",
				     ll_card->card_id, phy_id,
				     hard_phy_link_rate.bits.
				     phyctrl_neg_link_rate);
		if (HIGGS_HARD_PHY_NEGO_LINK_RATE_12_0_G ==
		    hard_phy_link_rate.bits.phyctrl_neg_link_rate) {
			HIGGS_MDELAY(15);
			higgs_serdes_enable_ctledfe(ll_card, phy_id);
			break;
		}

		HIGGS_UDELAY(100);
	}
	HIGGS_TRACE(OSP_DBL_INFO, 187,
		    "Card %d phy %d neg_link_rate final = 0x%x",
		    ll_card->card_id, phy_id,
		    hard_phy_link_rate.bits.phyctrl_neg_link_rate);

	if (HIGGS_HARD_PHY_NEGO_LINK_RATE_12_0_G ==
	    hard_phy_link_rate.bits.phyctrl_neg_link_rate) {
		/*sal_del_timer((void *)&ll_card->phy[phy_id].phy_run_serdes_fw_timer);*/
		/* lock £¬avoid timer over add*/
		spin_lock_irqsave(&ll_card->card_lock, flag);
		if (!sal_timer_is_active
		    ((void *)&ll_card->phy[phy_id].phy_run_serdes_fw_timer)) {
			/*300ms*/
			sal_add_timer((void *)&ll_card->phy[phy_id].
				      phy_run_serdes_fw_timer,
				      (unsigned long)((phy_id & 0xff) << 16 |
						      ll_card->card_id),
				      (u32) msecs_to_jiffies(300),
				      higgs_serdes_for_12g_timer_hander);
			HIGGS_TRACE(OSP_DBL_INFO, 171,
				    "phy id %d negotiate add timer!", phy_id);
		} else {
			/*if timer is active£¬then delay 300ms*/
			sal_mod_timer((void *)&ll_card->phy[phy_id].
				      phy_run_serdes_fw_timer,
				      msecs_to_jiffies(300));
			HIGGS_TRACE(OSP_DBL_MINOR, 171,
				    "phy id %d negotiate mode timer!", phy_id);
		}

		ll_card->phy[phy_id].run_serdes_fw_timer_en = true;
		spin_unlock_irqrestore(&ll_card->card_lock, flag);
	}
#endif

	return OK;
}

s32 higgs_stop_serdes_fw_timer(struct higgs_card * ll_card, u32 phy_id)
{
#if defined(C05_VERSION_TEST)
	unsigned long flag = 0;
#endif

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

#if !defined(C05_VERSION_TEST)
	HIGGS_REF(ll_card);
	HIGGS_REF(phy_id);
#else
	spin_lock_irqsave(&ll_card->card_lock, flag);

	/*stop serdes FW timer */
	ll_card->phy[phy_id].run_serdes_fw_timer_en = false;

	spin_unlock_irqrestore(&ll_card->card_lock, flag);
#endif

	return OK;
}

s32 higgs_clear_tx_training(struct higgs_card * ll_card, u32 phy_id)
{
	u32 value = 0;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

#if defined(FPGA_VERSION_TEST)
	HIGGS_REF(ll_card);
	HIGGS_REF(phy_id);
#else
	value =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_PHY_CONFIG2_REG);
	if (value & (0x1 << 24))
		HIGGS_PORT_REG_WRITE(ll_card, phy_id,
				     HISAS30HV100_PORT_REG_PHY_CONFIG2_REG,
				     value & ~(0x1 << 24));
#endif

	return OK;
}

s32 higgs_end_tx_training(struct higgs_card * ll_card, u32 phy_id)
{
	u32 value = 0;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

#if defined(FPGA_VERSION_TEST)
	HIGGS_REF(ll_card);
	HIGGS_REF(phy_id);
#else
	value =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_PHY_CONFIG2_REG);
	if (!(value & (0x1 << 24)))
		HIGGS_PORT_REG_WRITE(ll_card, phy_id,
				     HISAS30HV100_PORT_REG_PHY_CONFIG2_REG,
				     value | (0x1 << 24));
#endif

	return OK;
}

enum sal_link_rate higgs_get_sal_phy_rate(u32 higgs_rate)
{
	enum sal_link_rate link_rate = SAL_LINK_RATE_BUTT;

	switch (higgs_rate) {
	case HIGGS_PHY_RATE_1_5_G:
		link_rate = SAL_LINK_RATE_1_5G;
		break;
	case HIGGS_PHY_RATE_3_0_G:
		link_rate = SAL_LINK_RATE_3_0G;
		break;
	case HIGGS_PHY_RATE_6_0_G:
		link_rate = SAL_LINK_RATE_6_0G;
		break;
	case HIGGS_PHY_RATE_12_0_G:
		link_rate = SAL_LINK_RATE_12_0G;
		break;
	default:
		HIGGS_TRACE(OSP_DBL_MAJOR, 4645, "invalid phy link rate:%d",
			    higgs_rate);
		link_rate = SAL_LINK_RATE_BUTT;
		break;
	}

	return link_rate;
}

s32 higgs_config_oob_link_rate(struct higgs_card * ll_card, u32 phy_id,
			       u32 oob_link_rate)
{
	U_PROG_PHY_LINK_RATE unPhyLinkRate;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);
	HIGGS_ASSERT(oob_link_rate >= HIGGS_CFG_PROG_OOB_PHY_LINK_RATE_1_5_G
		     && oob_link_rate <=
		     HIGGS_CFG_PROG_OOB_PHY_LINK_RATE_12_0_G, return ERROR);

	unPhyLinkRate.u32 =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_PROG_PHY_LINK_RATE_REG);
	unPhyLinkRate.bits.cfg_prog_oob_phy_link_rate = oob_link_rate;
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_PROG_PHY_LINK_RATE_REG,
			     unPhyLinkRate.u32);

	return OK;
}

s32 higgs_notify_sal_phy_stop_rsp(struct higgs_card * ll_card, u32 phy_id)
{
	struct sal_phy_event phy_event_info;
	struct sal_phy *sal_phy = NULL;
	struct sal_port *sal_port = NULL;
	struct higgs_port *ll_port = NULL;
	u32 chip_port_id = 0;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

	/* SAL_PhyEvent notice sal */
	sal_phy = ll_card->phy[phy_id].up_ll_phy;
	chip_port_id = sal_phy->port_id;
	ll_port = higgs_get_port_by_port_id(ll_card, chip_port_id);

	if (NULL != ll_port) {
		higgs_mark_ll_port_on_stop_phy_rsp(ll_card, ll_port, phy_id);	/* set LLPort status  */
		sal_port = ll_port->up_ll_port;
	} else {
		sal_port = NULL;	/* SAL PORT can be null */
	}

	memset(&phy_event_info, 0, sizeof(struct sal_phy_event));
	phy_event_info.link_rate = SAL_LINK_RATE_FREE;
	phy_event_info.event = SAL_PHY_EVENT_STOP_RSP;
	phy_event_info.fw_port_id = chip_port_id;
	phy_event_info.phy_id = phy_id;
	phy_event_info.card_id = ll_card->card_id;

	(void)sal_phy_event(sal_port, sal_phy, phy_event_info);

	return OK;
}

s32 higgs_notify_sal_phy_down(struct higgs_card * ll_card, u32 phy_id)
{
	struct sal_phy_event phy_event_info;
	struct sal_phy *sal_phy = NULL;
	struct sal_port *sal_port = NULL;
	struct higgs_port *ll_port = NULL;
	u32 chip_port_id = 0;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

	/* SAL_PhyEvent notice sal */
	sal_phy = ll_card->phy[phy_id].up_ll_phy;
	chip_port_id = sal_phy->port_id;
	ll_port = higgs_get_port_by_port_id(ll_card, chip_port_id);

	if (NULL != ll_port) {
		higgs_mark_ll_port_on_phy_down(ll_card, ll_port, phy_id);	/* set LLPort status  */
		sal_port = ll_port->up_ll_port;
	} else {
		sal_port = NULL;
		return ERROR;
	}

	memset(&phy_event_info, 0, sizeof(struct sal_phy_event));
	phy_event_info.link_rate = SAL_LINK_RATE_FREE;
	phy_event_info.event = SAL_PHY_EVENT_DOWN;
	phy_event_info.fw_port_id = chip_port_id;
	phy_event_info.phy_id = phy_id;
	phy_event_info.card_id = ll_card->card_id;

	(void)sal_phy_event(sal_port, sal_phy, phy_event_info);

	return OK;
}

static s32 higgs_config_opt_mode(struct higgs_card *ll_card, u32 phy_id)
{
	U_PHY_CFG phy_cfg;
	u32 cable_type = 0;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

	/* OPT MODE */
	phy_cfg.u32 =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_PHY_CFG_REG);
	cable_type = ll_card->sal_card->phy[phy_id]->connect_wire_type;
	if (SAL_OPTICAL_CABLE == cable_type) {
		phy_cfg.bits.cfg_dc_opt_mode = HIGGS_CFG_PHY_OPT_MODE_OPTICAL;	/* optical cable */
	} else if (SAL_ELEC_CABLE == cable_type) {
		phy_cfg.bits.cfg_dc_opt_mode = HIGGS_CFG_PHY_OPT_MODE_DC;	/* electric cable*/
	} else if (SAL_IDLE_WIRE == cable_type) {
		HIGGS_TRACE(OSP_DBL_MINOR, 4617,
			    "Card:%d no cable on phy:%d, default as electric cable",
			    ll_card->card_id, phy_id);
		phy_cfg.bits.cfg_dc_opt_mode = HIGGS_CFG_PHY_OPT_MODE_DC;	/* process as electric cable*/
	} else {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4617,
			    "Card:%d invalid cable type:0x%x on phy:%d",
			    ll_card->card_id, cable_type, phy_id);
		return ERROR;
	}
	HIGGS_PORT_REG_WRITE(ll_card, phy_id, HISAS30HV100_PORT_REG_PHY_CFG_REG,
			     phy_cfg.u32);

	return OK;
}

static s32 higgs_config_tx_ffe_auto_negotiation(struct higgs_card *ll_card,
						u32 phy_id)
{
	struct higgs_phy_serdes_param serdes_param;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

	if (OK != higgs_read_serdes_param(ll_card, phy_id, &serdes_param)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4401,
			    "Card:%d phy %d read serdes param failed",
			    ll_card->card_id, phy_id);
		return ERROR;
	}

	serdes_param.tx_force = !ll_card->sal_card->config.bct_enable;

	if (OK != higgs_write_serdes_param(ll_card, phy_id, &serdes_param)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4401,
			    "Card:%d phy %d write serdes param failed",
			    ll_card->card_id, phy_id);
		return ERROR;
	}

	return OK;
}

/*
 * enable phy
 */

static s32 higgs_enable_phy(struct higgs_card *ll_card, u32 phy_id)
{
	U_PHY_CFG reg_phy_cfg;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

	/* ENABLE PHY  */
	reg_phy_cfg.u32 =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_PHY_CFG_REG);
	reg_phy_cfg.bits.cfg_phy_enabled = HIGGS_CFG_PHY_ENABLE;
	HIGGS_PORT_REG_WRITE(ll_card, phy_id, HISAS30HV100_PORT_REG_PHY_CFG_REG,
			     reg_phy_cfg.u32);

	return OK;
}

/*
 * disable phy
 */
static s32 higgs_disable_phy(struct higgs_card *ll_card, u32 phy_id)
{
	U_PHY_CFG reg_phy_cfg;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

	/* DISABLE PHY  */
	reg_phy_cfg.u32 =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_PHY_CFG_REG);
	reg_phy_cfg.bits.cfg_phy_enabled = HIGGS_CFG_PHY_DISABLE;
	HIGGS_PORT_REG_WRITE(ll_card, phy_id, HISAS30HV100_PORT_REG_PHY_CFG_REG,
			     reg_phy_cfg.u32);

	return OK;
}

static s32 higgs_local_phy_control(struct sal_card *sal_card,
				   u32 phy_id, u32 phyop)
{
	HIGGS_TRACE(OSP_DBL_INFO, 4311,
		    "Card:%d currently does not support local phy control phy:%d(Opt:%d)",
		    sal_card->card_no, phy_id, phyop);

	/* TODO:  */
	HIGGS_REF(sal_card);
	HIGGS_REF(phyop);
	HIGGS_REF(phy_id);

	return ERROR;
}

/*
 * query phy err code
 */
static s32 higgs_query_phy_err_code(struct sal_card *sal_card, u32 phy_id)
{
	struct sal_bit_err *sal_biterr = NULL;
	unsigned long flag = 0;
 
	HIGGS_ASSERT(NULL != sal_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

	sal_biterr = &sal_card->phy[phy_id]->err_code;
 
	spin_lock_irqsave(&sal_biterr->err_code_lock, flag);

	/*statistics*/
	sal_biterr->inv_dw_chg_cnt +=
	    sal_card->phy[phy_id]->err_code.phy_err_inv_dw;
	sal_biterr->disp_err_chg_cnt +=
	    sal_card->phy[phy_id]->err_code.phy_err_disp_err;
	sal_biterr->cod_vio_chg_cnt +=
	    sal_card->phy[phy_id]->err_code.phy_err_cod_viol;
	sal_biterr->loss_dw_syn_chg_cnt +=
	    sal_card->phy[phy_id]->err_code.phy_err_loss_dw_syn;

	/* clear  */
	sal_card->phy[phy_id]->err_code.phy_err_inv_dw = 0;
	sal_card->phy[phy_id]->err_code.phy_err_disp_err = 0;
	sal_card->phy[phy_id]->err_code.phy_err_cod_viol = 0;
	sal_card->phy[phy_id]->err_code.phy_err_loss_dw_syn = 0;

	spin_unlock_irqrestore(&sal_biterr->err_code_lock, flag);

	HIGGS_TRACE(OSP_DBL_MINOR, 4533,
		    "Card:%d phy:%d error code sum(InvDw:%d Disp:%d CodeViol:%d LossDwSyn:%d)",
		    sal_card->card_no, phy_id, sal_biterr->inv_dw_chg_cnt,
		    sal_biterr->disp_err_chg_cnt, sal_biterr->cod_vio_chg_cnt,
		    sal_biterr->loss_dw_syn_chg_cnt);
	return OK;
}

static u32 higgs_config_link_param(struct higgs_card *ll_card,
				   u32 phy_id, u32 phy_rate)
{
#ifdef FPGA_VERSION_TEST
	HIGGS_REF(ll_card);
	HIGGS_REF(phy_id);
	HIGGS_REF(phy_rate);
	return OK;
#else
	/* evb and c05*/
	U_PROG_PHY_LINK_RATE phy_link_rate;
	u32 phy_pcn = 0;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

	phy_link_rate.u32 = HIGGS_PORT_REG_READ(ll_card, phy_id,
						HISAS30HV100_PORT_REG_PROG_PHY_LINK_RATE_REG);
	phy_pcn =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_PHY_PCN_REG);

	/* compute*/
	switch (phy_rate) {
	case HIGGS_PHY_CFG_RATE_1_5_G:
		HIGGS_TRACE(OSP_DBL_MINOR, 172,
			    "Card:%d 1.5G link not recommended phy:%d",
			    ll_card->card_id, phy_id);
		phy_link_rate.bits.cfg_prog_max_phy_link_rate =
		    HIGGS_CFG_PROG_MAX_PHY_LINK_RATE_1_5_G;
		phy_pcn = (u32) HIGGS_PHY_PCN_PCB_1_5_G;
		break;
	case HIGGS_PHY_CFG_RATE_3_0_G:
		phy_link_rate.bits.cfg_prog_max_phy_link_rate =
		    HIGGS_CFG_PROG_MAX_PHY_LINK_RATE_3_0_G;
		phy_pcn = (u32) HIGGS_PHY_PCN_PCB_3_0_G;
		break;
	case HIGGS_PHY_CFG_RATE_6_0_G:
		phy_link_rate.bits.cfg_prog_max_phy_link_rate =
		    HIGGS_CFG_PROG_MAX_PHY_LINK_RATE_6_0_G;
		phy_pcn = (u32) HIGGS_PHY_PCN_PCB_6_0_G;
		break;
	case HIGGS_PHY_CFG_RATE_12_0_G:
		phy_link_rate.bits.cfg_prog_max_phy_link_rate =
		    HIGGS_CFG_PROG_MAX_PHY_LINK_RATE_12_0_G;
		phy_pcn = (u32) HIGGS_PHY_PCN_PCB_12_0_G;
		break;
	default:
		return ERROR;
	}
	phy_link_rate.bits.cfg_prog_oob_phy_link_rate =
	    HIGGS_CFG_PROG_OOB_PHY_LINK_RATE_1_5_G;
	phy_link_rate.bits.cfg_prog_min_phy_link_rate =
	    HIGGS_CFG_PROG_MIN_PHY_LINK_RATE_1_5_G;

	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_PROG_PHY_LINK_RATE_REG,
			     phy_link_rate.u32);
	HIGGS_PORT_REG_WRITE(ll_card, phy_id, HISAS30HV100_PORT_REG_PHY_PCN_REG,
			     phy_pcn);

	return OK;
#endif
}

/*
 * write and read the eq parameter of the phy
 */
static s32 higgs_operate_serdes_param(struct higgs_card *ll_card,
				      struct sal_phy_ll_op_param
				      *ll_phy_opt_info)
{
	u32 phy_id = 0;
	struct higgs_phy_serdes_param serdes_param;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(NULL != ll_phy_opt_info, return ERROR);

	phy_id = ll_phy_opt_info->phy_id;
	memset(&serdes_param, 0, sizeof(serdes_param));

	/*
	 * 660 serdes parameter£º TX: force, pre, main, post; RX: CLTE
	 * SAL:PreEmphasis, PostEmphasis, Amplitude,map
	 * PreEmphasis <-> pre
	 * Amplitude <-> main
	 * PostEmphsis <-> post
	 */
	/* read parameter*/
	if (OK != higgs_read_serdes_param(ll_card, phy_id, &serdes_param)) {
		HIGGS_TRACE(OSP_DBL_INFO, 4311,
			    "Card:%d phy %d read serdes param failed",
			    ll_card->card_id, phy_id);
		return ERROR;
	}

	if (ll_phy_opt_info->get_analog) {
		ll_phy_opt_info->pre_emphasis = serdes_param.tx_pre;
		ll_phy_opt_info->amplitidu = serdes_param.tx_main;
		ll_phy_opt_info->post_emphasis = serdes_param.tx_post;
	} else {		/* write*/
		serdes_param.tx_pre = ll_phy_opt_info->pre_emphasis;
		serdes_param.tx_main = ll_phy_opt_info->amplitidu;
		serdes_param.tx_post = ll_phy_opt_info->post_emphasis;
		serdes_param.rx_clte = serdes_param.rx_clte;	/* RX CLTE*/

		if (OK !=
		    higgs_write_serdes_param(ll_card, phy_id, &serdes_param)) {
			HIGGS_TRACE(OSP_DBL_INFO, 4311,
				    "Card:%d phy:%d write serdes param failed",
				    ll_card->card_id, phy_id);
			return ERROR;
		}
	}

	return OK;
}

static s32 higgs_write_serdes_param(struct higgs_card *ll_card,
				    u32 phy_id,
				    struct higgs_phy_serdes_param *serdes_Param)
{
	U_PHY_CONFIG2 reg_phy_config2;
	struct higgs_phy_serdes_param ll_serdes_param;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);
	HIGGS_ASSERT(NULL != serdes_Param, return ERROR);

#if defined(FPGA_VERSION_TEST)
	HIGGS_REF(ll_card);
	HIGGS_REF(phy_id);
	HIGGS_REF(serdes_Param);
	reg_phy_config2.u32 = 0;
	HIGGS_REF(reg_phy_config2);	/* pclint */
#else
	/* read reg*/
	memcpy(&ll_serdes_param, serdes_Param, sizeof(ll_serdes_param));
	reg_phy_config2.u32 =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_PHY_CONFIG2_REG);
	HIGGS_SERDES_PARAM_TO_REG_PHY_CONFIG2(ll_serdes_param, reg_phy_config2);
	HIGGS_PORT_REG_WRITE(ll_card, phy_id,
			     HISAS30HV100_PORT_REG_PHY_CONFIG2_REG,
			     reg_phy_config2.u32);
#endif

	return OK;
}

static s32 higgs_read_serdes_param(struct higgs_card *ll_card,
				   u32 phy_id,
				   struct higgs_phy_serdes_param *serdes_Param)
{
	U_PHY_CONFIG2 reg_phy_config2;
	struct higgs_phy_serdes_param ll_serdes_param;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);
	HIGGS_ASSERT(NULL != serdes_Param, return ERROR);

#if defined(FPGA_VERSION_TEST)
	HIGGS_REF(ll_card);
	HIGGS_REF(phy_id);
	HIGGS_REF(serdes_Param);
	reg_phy_config2.u32 = 0;
	HIGGS_REF(reg_phy_config2);	/* pclint */
#else

	reg_phy_config2.u32 =
	    HIGGS_PORT_REG_READ(ll_card, phy_id,
				HISAS30HV100_PORT_REG_PHY_CONFIG2_REG);
	HIGGS_REG_PHY_CONFIG2_TO_SERDES_PARAM(reg_phy_config2, ll_serdes_param);
	memcpy(serdes_Param, &ll_serdes_param, sizeof(ll_serdes_param));
#endif

	return OK;
}

static s32 higgs_notify_sal_phy_start_rsp(struct higgs_card *ll_card,
					  u32 phy_id)
{
	struct sal_phy *sal_phy = NULL;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

	/* chang status, PHY is open,the status of link up is knew in phy up*/
	sal_phy = ll_card->phy[phy_id].up_ll_phy;
	sal_phy->link_status = SAL_PHY_LINK_DOWN;

	return OK;
}
