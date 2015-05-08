/*
 * Copyright (C) huawei
 * io error process function
 */


#include "higgs_common.h"
/*#include "higgs_dump.h"*/
#include "higgs_io.h"
#include "higgs_eh.h"
#include "higgs_dump.h"
#include "higgs_pv660.h"

#define  HIGGS_REQ_OCUPPY_TIMEOUT_THRESHOLD          4000	/* unit: ms */
#define  HIGGS_SAVE_CAHE_IOST_INTERVAL_THRESHOLD     1000	/* unit: ms */

struct higgs_req *higgs_get_req_by_iptt(struct higgs_card *ll_card, u32 iptt)
{
	struct higgs_req *find_abort_in_running = NULL;
	if (iptt >= HIGGS_MAX_REQ_NUM)
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_MAJOR, msecs_to_jiffies(1000), 171,
				     "Card:%d to get REQ by invalid iptt %d ",
				     ll_card->card_id, iptt);
	else		
		find_abort_in_running = ll_card->running_req[iptt];

	if (find_abort_in_running)
		return find_abort_in_running;
	else
		return NULL;
}

/*
 * find the iptt of abort msg,it is ok to call twice
 */
u32 higgs_look_up_iptt(struct higgs_card * ll_card, struct sal_msg * abort_msg)
{
	u32 ret_tag = HIGGS_MAX_REQ_NUM;
	u32 i;
	struct higgs_req *find_higgs_req = NULL;
	struct higgs_req_manager *req_manager = NULL;
	unsigned long flag = 0;

	HIGGS_ASSERT(NULL != ll_card, return HIGGS_MAX_REQ_NUM);
	req_manager = &ll_card->higgs_req_manager;
	for (i = 0; i < HIGGS_MAX_REQ_NUM; i++) {
		spin_lock_irqsave(&req_manager->req_lock, flag);
		find_higgs_req = ll_card->running_req[i];
		if (SAL_AND
		    (find_higgs_req, abort_msg == find_higgs_req->sal_msg)) {
			ret_tag = find_higgs_req->iptt;
			/*REQ status change to suspect*/
			(void)higgs_req_state_chg(find_higgs_req,
						  HIGGS_REQ_EVENT_ABORT_ST,
						  false);
			spin_unlock_irqrestore(&req_manager->req_lock, flag);
			break;
		}
		spin_unlock_irqrestore(&req_manager->req_lock, flag);
	}

	return ret_tag;
}

static s32 higgs_msg_done_fail(struct sal_msg *sal_msg)
{
	if (sal_msg && sal_msg->done) {
		sal_msg->status.drv_resp = SAL_MSG_DRV_FAILED;
		sal_msg->done(sal_msg);
	}
	return OK;
}

/*
 * aborted io which in running req return to upper levels,
 *do not care whether the io is aborted successfully
 */
static s32 higgs_abort_dev_not_found(struct higgs_req *mange_higgs_req)
{
	struct higgs_card *higgs_card = mange_higgs_req->higgs_card;
	u32 index = 0;
	struct higgs_req_manager *req_manager = NULL;
	unsigned long flag = 0;
	struct sal_msg *sal_msg = NULL;

	req_manager = &higgs_card->higgs_req_manager;
	for (index = 0; index < HIGGS_MAX_REQ_NUM; index++) {
		struct higgs_req *req = NULL;
		/*1.assign pstLoopReq */
		spin_lock_irqsave(&req_manager->req_lock, flag);
		req = higgs_card->running_req[index];
		/*  2. done to other */
		if (req && (INTER_CMD_TYPE_NONE == req->ctx.cmd_type) &&
		    req->ll_dev == mange_higgs_req->ll_dev) {
			/*return to SAL */
			HIGGS_TRACE(OSP_DBL_MINOR, 4738,
				    "card %d, dev %d, iptt %d dev abort sccess! send event = HIGGS_REQ_EVENT_ABORTCHIP_OK",
				    higgs_card->card_id,
				    mange_higgs_req->ll_dev->dev_id, req->iptt);
			sal_msg = req->sal_msg;
			req->sal_msg = NULL;
			(void)higgs_req_state_chg(req, HIGGS_REQ_EVENT_ABORTCHIP_OK, false);
		}
		spin_unlock_irqrestore(&req_manager->req_lock, flag);
		(void)higgs_msg_done_fail(sal_msg);
	}
	return OK;
}

/*
 * change the req status to suspect about the dev
 */

static s32 higgs_abort_dev_req_chg(struct higgs_card *higgs_card,
				   struct higgs_device *higgs_dev)
{
	u32 index = 0;
	struct higgs_req_manager *req_manager = NULL;
	unsigned long flag = 0;
	struct higgs_req *req = NULL;
	struct sal_dev *sal_dev = NULL;

	sal_dev = (struct sal_dev *)higgs_dev->up_dev;
	req_manager = &higgs_card->higgs_req_manager;
	for (index = 0; index < HIGGS_MAX_REQ_NUM; index++) {
		spin_lock_irqsave(&req_manager->req_lock, flag);
		req = higgs_card->running_req[index];
		if (req && (INTER_CMD_TYPE_NONE == req->ctx.cmd_type)
		    && (req->ll_dev == higgs_dev)) {
			HIGGS_TRACE(OSP_DBL_MINOR, 4738,
				    "card %d, devid = %d, orq iptt %d change to suspect!",
				    higgs_card->card_id, higgs_dev->dev_id,
				    req->iptt);
			req->ctx.sas_addr = sal_dev->sas_addr;
			(void)higgs_req_state_chg(req, HIGGS_REQ_EVENT_ABORT_ST, false);
		}
		spin_unlock_irqrestore(&req_manager->req_lock, flag);
	}

	return OK;
}
 
s32 higgs_process_rsp_abort(struct higgs_req * higgs_req)
{
	u32 cmpl_entry = higgs_req->cmpl_entry;
	u32 abort_status = 0;
	struct higgs_card *higgs_card = higgs_req->higgs_card;
	struct higgs_req *higgs_org_req = NULL;
	struct sal_msg *sal_msg = NULL;
	struct higgs_req_manager *req_manager = &higgs_card->higgs_req_manager;
	unsigned long flag = 0;

	u32 hgcinvld_dqe_info_reg = 0;

	/* dq cfg err */
	if (HIGGS_CQ_CFG_ERR(&cmpl_entry)) {
		/* DQ cfg err, direct return fail*/
		hgcinvld_dqe_info_reg =
		    HIGGS_GLOBAL_REG_READ(higgs_req->higgs_card,
					  HISAS30HV100_GLOBAL_REG_HGC_INVLD_DQE_INFO_REG);
		HIGGS_TRACE(OSP_DBL_INFO, 1,
			    "iptt: 0x%x, CMPLT: %d, ERR_XFRD: %d, RSPNS_GOOD: %d, cmpl_entry 0x%08X",
			    HIGGS_CQ_IPTT(&cmpl_entry),
			    HIGGS_CQ_CMD_CMPLT(&cmpl_entry),
			    HIGGS_CQ_ERR_RCRD_XFRD(&cmpl_entry),
			    HIGGS_CQ_RSPNS_GOOD(&cmpl_entry), cmpl_entry);
		HIGGS_TRACE(OSP_DBL_INFO, 1,
			    "Req iptt is %d,Delivery Queue Entry Error, HGC_INVLD_DQE_INFO_REG[0x%x]",
			    higgs_req->iptt, hgcinvld_dqe_info_reg);

		return higgs_process_abort_cfg_efg_err_rsp(higgs_req);
	}

	abort_status = HIGGS_CQ_ABORT_STATUS(&cmpl_entry);
	HIGGS_TRACE(OSP_DBL_MINOR, 4738, "Abort Rsp Tag:%d,status:0x%X",
		    higgs_req->iptt, abort_status);

	switch (abort_status) {
		/*abort single could be IO_ABORTED_STATUS and IO_NOT_VALID_STATUS*/
	case IO_ABORTED_STATUS:
		/*break;*/
	case IO_NOT_VALID_STATUS:

		spin_lock_irqsave(&req_manager->req_lock, flag);
		higgs_org_req =
		    higgs_get_req_by_iptt(higgs_req->higgs_card,
					  higgs_req->ctx.abort_tag);
		if (NULL != higgs_org_req) {
			HIGGS_TRACE(OSP_DBL_MINOR, 4738,
				    "card %d, abort cmd iptt %d not found mange iptt %d, sw return aborted",
				    higgs_card->card_id, higgs_req->iptt,
				    higgs_req->ctx.abort_tag);
			sal_msg = higgs_org_req->sal_msg;
			higgs_org_req->sal_msg = NULL;
			(void)higgs_req_state_chg(higgs_org_req,
						  HIGGS_REQ_EVENT_ABORTCHIP_OK,
						  false);
		}
		spin_unlock_irqrestore(&req_manager->req_lock, flag);
		(void)higgs_msg_done_fail(sal_msg);	/*aborted io, done fail*/
		break;
	case IO_NO_DEVICE_STATUS:
		break;
	case IO_COMPLETE_STATUS:
		if (HIGGS_ABORT_SINGLE_FLAG == higgs_req->ctx.inter_type) {
			/*abort single successfully,change status*/
			spin_lock_irqsave(&req_manager->req_lock, flag);
			higgs_org_req =
			    higgs_get_req_by_iptt(higgs_req->higgs_card,
						  higgs_req->ctx.abort_tag);
			if (NULL != higgs_org_req) {
				HIGGS_TRACE(OSP_DBL_MINOR, 4738,
					    "card %d, abort cmd iptt %d find mange iptt %d, complete! event = HIGGS_REQ_EVENT_ABORTCHIP_OK",
					    higgs_card->card_id,
					    higgs_req->iptt,
					    higgs_req->ctx.abort_tag);
				(void)higgs_req_state_chg(higgs_org_req,
							  HIGGS_REQ_EVENT_ABORTCHIP_OK,
							  false);
			}
			spin_unlock_irqrestore(&req_manager->req_lock, flag);
		}
		break;
	default:
		break;
	}

	if (HIGGS_ABORT_SINGLE_FLAG == higgs_req->ctx.inter_type) {
		/*abort single come back*/
		struct sal_msg *sal_msg = NULL;
		sal_msg = higgs_req->sal_msg;
		sal_msg->status.drv_resp = SAL_MSG_DRV_SUCCESS;
		sal_msg->status.io_resp = SAM_STAT_GOOD;
		sal_msg->done(sal_msg);
	} else if (HIGGS_ABORT_DEV_FLAG == higgs_req->ctx.inter_type) {
		struct sal_dev *pstSALDev = NULL;
		pstSALDev = (struct sal_dev *)higgs_req->ll_dev->up_dev;
		/*part of io is not come back,need to the incomplete tag done to sal*/
		(void)higgs_abort_dev_not_found(higgs_req);
		/*notice SAS layer operation is finish*/
		sal_abort_dev_io_cb(pstSALDev, SAL_STATUS_SUCCESS);
	} else {
		HIGGS_TRACE(OSP_DBL_MINOR, 171,
			    "Card:%d REQ invalid abort flag, iptt %d",
			    higgs_card->card_id, higgs_req->iptt);
	}

	higgs_req->sal_msg = NULL;
	/*release abort req itself*/
	(void)higgs_req_state_chg(higgs_req, HIGGS_REQ_EVENT_COMPLETE, true);
	return OK;
}

enum sal_cmnd_exec_result higgs_prepare_abort(struct higgs_req *higgs_req)
{
	struct higgs_dq_info *dq_info = higgs_req->dq_info;
	struct higgs_req_context *ctx = &higgs_req->ctx;
	struct sal_dev *sal_dev = (struct sal_dev *)(higgs_req->ll_dev->up_dev);

	HIGGS_ASSERT(NULL != dq_info, return SAL_CMND_EXEC_FAILED);
	HIGGS_ASSERT(NULL != sal_dev, return SAL_CMND_EXEC_FAILED);

	HIGGS_CHF_CMD(dq_info, ABORT_CMD);	/*abort cmd */
	HIGGS_CHF_PRI(dq_info, 0x1);	/*higher priority */
	/*Abort command used port id?? */
	HIGGS_CHF_PORT(dq_info, sal_dev->port->port_id);	/*which port issue */
	HIGGS_CHF_IPTT(dq_info, higgs_req->iptt);	/*iptt */

	/*Abort flag: 0x0,abort request; 0x1,abort device */
	HIGGS_CHF_ABORT_FLAG(dq_info, ctx->inter_type);
	/*If abort device use it */
	HIGGS_CHF_DEVICE_ID(dq_info, higgs_req->ll_dev->dev_id);
	/*If abort device single request use it */
	if (HIGGS_ABORT_SINGLE_FLAG == ctx->inter_type)
		HIGGS_CHF_ABORT_IPTT(dq_info, ctx->abort_tag);

	HIGGS_TRACE(OSP_DBL_INFO, 1,
		    "Manage iptt %d, Abort Flag:%d, DevId:%d, AbortIPTT:%d",
		    higgs_req->iptt, ctx->inter_type, higgs_req->ll_dev->dev_id,
		    ctx->abort_tag);

	return SAL_CMND_EXEC_SUCCESS;
}

enum sal_cmnd_exec_result higgs_send_abort(struct higgs_req *higgs_req)
{
	struct higgs_card *higgs_card = higgs_req->higgs_card;
	higgs_send_command_hw(higgs_card, higgs_req);

	return SAL_CMND_EXEC_SUCCESS;
}

enum sal_cmnd_exec_result higgs_internal_abort(struct higgs_card *higgs_card,
					       struct sal_msg *sal_msg,
					       struct sal_dev *sal_dev)
{
	struct higgs_req *higgs_req = NULL;
	struct higgs_device *higgs_dev = NULL;
	struct higgs_prot_dispatch *dispatch = NULL;
	enum sal_cmnd_exec_result ret = SAL_CMND_EXEC_FAILED;
	OSP_DMA command_table_dma;
	struct higgs_cmd_table *cmd_table = NULL;
	struct higgs_dq_info *dq_info = NULL;
	unsigned long flag = 0;
	u32 dqid = 0;

	HIGGS_ASSERT(NULL != sal_dev, return ERROR);
	higgs_dev = sal_dev->ll_dev;
	HIGGS_ASSERT(NULL != higgs_dev, return ERROR);

	dispatch = &higgs_dispatcher.inter_abort_dispatch;
	higgs_req = higgs_get_higgs_req(higgs_card, INTER_CMD_TYPE_ABORT);
	if (NULL == higgs_req) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1, "Card[%d]Get Higgs Req Failed!",
			    higgs_card->card_id);
		return ret;
	}
	cmd_table =
	    higgs_get_command_table(higgs_card, higgs_req->iptt,
				    &command_table_dma);
	if (NULL == cmd_table) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1,
			    "iptt[0x%x] CommandTable Get Error!",
			    higgs_req->iptt);
		goto cmnd_table_fail;
	}

	if (NULL != sal_msg) {
		/*abort single msg */
		u16 abort_tag;
		/*save iptt to SAL msg */
		sal_msg->ll_tag = (u16) higgs_req->iptt;
		abort_tag =
		    (u16) higgs_look_up_iptt(higgs_card, sal_msg->related_msg);
		if (abort_tag >= HIGGS_MAX_REQ_NUM) {
			HIGGS_TRACE(OSP_DBL_MINOR, 4329,
				    "Card:%d managed msg:%p not be found",
				    higgs_card->card_id, sal_msg->related_msg);
			/*release manage IO*/
			(void)higgs_req_state_chg(higgs_req,
						  HIGGS_REQ_EVENT_COMPLETE,
						  true);
			/* abort is not send£¬need to wake up the waiter*/
			SAS_COMPLETE(&sal_msg->compl);
            /*can't find the right io,suggest the io come back successfully*/
			return SAL_CMND_EXEC_SUCCESS;
		}
		higgs_req->ctx.abort_tag = abort_tag;
		higgs_req->ctx.inter_type = HIGGS_ABORT_SINGLE_FLAG;
	} else {
		/*the req of the dev change to suspect*/
		(void)higgs_abort_dev_req_chg(higgs_card, higgs_dev);
		/* abort dev msg */
		higgs_req->ctx.abort_dev_id = (u16) higgs_dev->dev_id;
		higgs_req->ctx.inter_type = HIGGS_ABORT_DEV_FLAG;
	}

	dqid = higgs_get_dqid_from_devid(higgs_card, higgs_dev->dev_id);
	spin_lock_irqsave(HIGGS_DQ_LOCK(higgs_card, dqid), flag);

	/*init higgs req */
	higgs_req->queue_id = dqid;
	higgs_req->higgs_card = higgs_card;
	higgs_req->ll_dev = higgs_dev;
	higgs_req->proto_dispatch = dispatch;
	higgs_req->sal_msg = sal_msg;

	HIGGS_TRACE(OSP_DBL_INFO, 1,
		    "iptt[0x%x] abort type %d (single:0, dev:1), sal_msg %p",
		    higgs_req->iptt, higgs_req->ctx.inter_type, sal_msg);
	higgs_req->completion = higgs_req_callback;
	dq_info = higgs_get_dq_info(higgs_card, dqid);
	if (NULL == dq_info) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1,
			    "ITPP[0x%x] DQ Get Error!", higgs_req->iptt);
		goto fail_in_lock;
	}

	/*must be set, the prepare and send interface will use these */
	higgs_req->cmd_table = cmd_table;
	higgs_req->cmd_table_dma = command_table_dma;
	higgs_req->dq_info = dq_info;


	if (NULL != dispatch->command_prepare) {
		/* higgs_prepare_abort*/
		ret =
		    (enum sal_cmnd_exec_result)dispatch->
		    command_prepare(higgs_req);
		if (SAL_CMND_EXEC_SUCCESS != ret) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 1, "Card[%d]Prepare failed!",
				    higgs_card->card_id);
			goto fail_in_lock;
		}
	}
	/*higgs_send_abort*/
	HIGGS_TRACE(OSP_DBL_INFO, 100, "Begin Send Abort info,Dev[%d]",
		    higgs_dev->dev_id);
	ret = (enum sal_cmnd_exec_result)dispatch->command_send(higgs_req);
	HIGGS_TRACE(OSP_DBL_INFO, 100, "Complete Send Abort info,Dev[%d]",
		    higgs_dev->dev_id);
	if (SAL_CMND_EXEC_SUCCESS != ret) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1, "Card[%d]Dev[%d]Send failed!",
			    higgs_card->card_id, higgs_dev->dev_id);
		goto fail_in_lock;
	}
#if 0
	higgs_hex_dump("Abort DQ info", (void *)dq_info,
		       sizeof(struct higgs_dq_info), sizeof(u32));
	HIGGS_TRACE(OSP_DBL_INFO, 100, "Dump info Ok,Dev[%d]",
		    pstHiggsDev->dev_id);
#endif
	spin_unlock_irqrestore(HIGGS_DQ_LOCK(higgs_card, dqid), flag);

	return ret;

      fail_in_lock:
	spin_unlock_irqrestore(HIGGS_DQ_LOCK(higgs_card, dqid), flag);

      cmnd_table_fail:
	if (higgs_req) {
		(void)higgs_req_state_chg(higgs_req, HIGGS_REQ_EVENT_COMPLETE,
					  true);
		higgs_req = NULL;
	}
	return ret;
}

enum sal_cmnd_exec_result higgs_eh_abort_operation(enum sal_eh_abort_op_type
						   en_option, void *argv_in)
{
	struct higgs_card *higgs_card = NULL;
	struct sal_msg *sal_msg = NULL;
	struct sal_dev *sal_dev = NULL;
	enum sal_cmnd_exec_result ret = SAL_CMND_EXEC_FAILED;

	HIGGS_ASSERT(NULL != argv_in, return ERROR);
	switch (en_option) {
	case SAL_EH_ABORT_OPT_SINGLE:
		sal_msg = (struct sal_msg *)argv_in;
		higgs_card = GET_HIGGS_CARD_BY_MSG(sal_msg);
		ret = higgs_internal_abort(higgs_card, sal_msg, sal_msg->dev);
		break;
	case SAL_EH_ABORT_OPT_DEV:
		sal_dev = (struct sal_dev *)argv_in;
		higgs_card = GET_HIGGS_CARD_BY_DEV(sal_dev);
		ret = higgs_internal_abort(higgs_card, NULL, sal_dev);
		break;
	default:
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_MAJOR, HZ, 1,
				     "Unknown Ehoption[0x%x]", en_option);
		break;
	}
	return ret;
}

bool higgs_check_chip_fatal_error(struct sal_card * sal_card,
				  struct higgs_chip_err_fatal_stat *
				  chip_efstat)
{
	bool ret = false;
	struct higgs_chip_err_fatal_stat ll_chip_efstat;

	HIGGS_ASSERT(NULL != sal_card, return ERROR);
	HIGGS_ASSERT(NULL != chip_efstat, return ERROR);
	HIGGS_REF(sal_card);

	ll_chip_efstat = *chip_efstat;

	/*not trouble and error*/
	if (!HIGGS_INT_STAT_REG_FOUND_ERR(ll_chip_efstat))
		return ret;

	/*1.chip fatal error*/
	if (HIGGS_CHIP_HGC_FIFO_OVFLD_ERR(ll_chip_efstat)) {
		chip_efstat->hgc_fifo_ovfld_int_err_cnt = 0;
		ret = true;	/*request chip reset*/
	}

	/*2.process the error not from cq*/
	if (chip_efstat->hgc_iost_ram_mul_bit_ecc_err_cnt >
	    chip_efstat->hgc_iost_ram_mul_bit_ecc_err_rct) {
		/*HGC IOST RAM happen mulBit ECC error*/
		/*save current result*/
		chip_efstat->hgc_iost_ram_mul_bit_ecc_err_rct =
		    chip_efstat->hgc_iost_ram_mul_bit_ecc_err_cnt;

		HIGGS_TRACE(OSP_DBL_MINOR, 4187,
			    "Chip Error HgcIostRamMulBitEccErr FOUND ! The count is 0x%08X ",
			    chip_efstat->hgc_iost_ram_mul_bit_ecc_err_rct);
	}

	/*HGC Delevry Queue Entry RAM happen mulBit ECC error */
	if (chip_efstat->hgc_dqe_ram_mul_bit_ecc_err_cnt >
	    chip_efstat->hgc_dqe_ram_mul_bit_ecc_err_rct) {
		chip_efstat->hgc_dqe_ram_mul_bit_ecc_err_rct =
		    chip_efstat->hgc_dqe_ram_mul_bit_ecc_err_cnt;

		HIGGS_TRACE(OSP_DBL_MINOR, 4187,
			    "Chip Error HgcDqeRamMulBitEccErr FOUND ! The count is 0x%08X ",
			    chip_efstat->hgc_dqe_ram_mul_bit_ecc_err_rct);
	}

	/*HGC ITCT RAM happen mulBit ECC error*/
	if (chip_efstat->hgc_itct_ram_mul_bit_ecc_err_cnt >
	    chip_efstat->hgc_itct_ram_mul_bit_ecc_err_rct) {
		chip_efstat->hgc_itct_ram_mul_bit_ecc_err_rct =
		    chip_efstat->hgc_itct_ram_mul_bit_ecc_err_cnt;

		HIGGS_TRACE(OSP_DBL_MINOR, 4187,
			    "Chip Error HgcItctRamMulBitEccErr FOUND ! The count is 0x%08X ",
			    chip_efstat->hgc_itct_ram_mul_bit_ecc_err_rct);
	}

	return ret;
}

s32 higgs_chip_fatal_check(struct sal_card * sal_card,
			   enum sal_chip_chk_ret * chip_chk_ret)
{
	struct higgs_card *ll_card = NULL;
#if 0
	u32 queue_id = 0;
	struct higgs_cq_info *pstCqInfo = NULL;
	u32 uiEntryId = 0;
	u32 uiDumpFlag = 1;
#endif

	HIGGS_ASSERT(NULL != sal_card, return ERROR);
	HIGGS_ASSERT(NULL != chip_chk_ret, return ERROR);
	ll_card = (struct higgs_card *)sal_card->drv_data;
	HIGGS_ASSERT(NULL != ll_card, return ERROR);

#if 1
	if (higgs_check_chip_fatal_error
	    (sal_card, &ll_card->chip_err_fatal_stat)) {
		/*fatal error need to reset chip */
		*chip_chk_ret = SAL_ROUTINE_CHK_RET_CHIP_RST;
		return OK;
	}
#else
	for (queue_id = 0; queue_id < pstLLCard->higgs_can_cq; queue_id++) {
		for (uiEntryId = 0;
		     uiEntryId < pstLLCard->io_hw_struct_info.entry_use_per_cq;
		     uiEntryId++) {
			pstCqInfo =
			    pstLLCard->io_hw_struct_info.
			    complete_queue[queue_id].queue_base + uiEntryId;

			if (!
			    ((pstCqInfo->dw0.cq_dw
			      && 0xffff) < pstLLCard->higgs_can_io)) {
				HIGGS_TRACE(OSP_DBL_MAJOR, 127,
					    "OOPS_DEBUG: Card %d CQ %d Entry %d abnormal, try to dump fwinfo",
					    pstLLCard->card_id, queue_id,
					    uiEntryId);
				(void)Higgs_DumpInfo(v_pstSALCard, &uiDumpFlag);
				goto END;
			}
		}
	}
      END:
#endif
	*chip_chk_ret = SAL_ROUTINE_CHK_RET_OK;
	return OK;

}

s32 higgs_analyze_io_status(u32 io_status)
{
	switch (io_status) {
	case 0x00:		/*IO initial status*/
	case 0x01:		/*wait TASK send*/
	case 0x11:		/*in TASK sending*/
	case 0x02:		/*wait command send*/
	case 0x12:		/*sending command*/
		/*report checked chip err*/
		return ERROR;
	default:
		break;
	}

	/*no chip error*/
	return OK;
}

void higgs_save_iost_from_cache(struct sal_card *sal_card)
{
	u32 reg_add_offset = 0;
	u32 i = 0;
	u32 reg_value = 0;
	struct higgs_card *higgs_card = NULL;

	HIGGS_ASSERT(sal_card != NULL, return);
	higgs_card = (struct higgs_card *)sal_card->drv_data;

	/*save current iost message in cache,by reading IOST_DFX(0x014C),get 64 io status message*/
	reg_add_offset = 0x014C;

	reg_value = higgs_read_glb_cfg_reg(sal_card, reg_add_offset);	/*fist is invalid data*/

	for (i = 0; i < HIGGS_MAX_IOST_CACHE_NUM; i++) {
		reg_value = higgs_read_glb_cfg_reg(sal_card, reg_add_offset);
		higgs_card->iost_cache_info.iost_cache_array[i] = reg_value;
	}

	return;
}

s32 higgs_chip_fatal_chkin_tm_timeout(struct sal_card * sal_card,
				      struct sal_msg * argv_in)
{
	struct higgs_card *higgs_card = NULL;
	struct higgs_iost_info *start_vir_addr = NULL;
	unsigned long cur_jiff = 0;
	u32 i = 0;
	u32 tm_iptt = 0;
	u32 iptt = 0;
	u32 iost_valflg = 0;
	u32 io_status = 0;
	bool chkflg = false;
	s32 ret = OK;

	HIGGS_ASSERT(sal_card != NULL, return OK);
	HIGGS_ASSERT(argv_in != NULL, return OK);
	higgs_card = (struct higgs_card *)sal_card->drv_data;

	/*get TM IPTT */
	tm_iptt = argv_in->ll_tag;
	HIGGS_ASSERT(tm_iptt < higgs_card->higgs_can_io, return ERROR);

	cur_jiff = jiffies;

	/*save IOST message,time interval must over 1s*/
	if ((jiffies_to_msecs
	     (cur_jiff - higgs_card->iost_cache_info.save_cache_iost_jiff) >
	     HIGGS_SAVE_CAHE_IOST_INTERVAL_THRESHOLD)) {

		higgs_save_iost_from_cache(sal_card);

		higgs_card->iost_cache_info.save_cache_iost_jiff = cur_jiff;
	}

	/*search the iost in cache*/
	for (i = 0; i < HIGGS_MAX_IOST_CACHE_NUM; i++) {
		iptt = (higgs_card->iost_cache_info.iost_cache_array[i] & 0x1fff);	/*bit[12:0]*/
		iost_valflg = (higgs_card->iost_cache_info.iost_cache_array[i] & 0x8000) >> 15;	/*bit[15]*/
		io_status = ((higgs_card->iost_cache_info.iost_cache_array[i] & 0xff0000) >> 16);	/*bit[23:16]*/
		if ((tm_iptt == iptt) && (iost_valflg)) {
			/*analyze io_status */
			ret = higgs_analyze_io_status(io_status);

			chkflg = true;
			break;
		}
	}

	/*if the iost is not in cache£¬getting it from DDR */
	if (!chkflg) {
		start_vir_addr = higgs_card->io_hw_struct_info.iost_base;
		if (NULL == start_vir_addr) {
			HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, 60 * HZ, 1,
					     "IOST table Vir Addr NULL. ");
			return OK;
		}

		io_status = start_vir_addr[tm_iptt].io_status;

		/*analyze io_status */
		ret = higgs_analyze_io_status(io_status);
	}

	if (ERROR == ret) {
		if (true == argv_in->transmit_mark)
			ret = OK;
		else
			HIGGS_TRACE(OSP_DBL_MINOR, 4136,
				    "TM Timeout and Found Chip Fatal, iptt=%d, io_status=0x%08X ",
				    tm_iptt, io_status);
	}

	HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, 60 * HZ, 1,
			     "Informations When TM TimeOut: iptt=%d, io_status=0x%08X, ret=%d, transmit_mark=%d",
			     tm_iptt, io_status, ret, argv_in->transmit_mark);

	return ret;
}

/*
 * inquery chip err code
 */
s32 higgs_inquery_chip_err_code(struct sal_card * sal_card, u32 phy_id)
{
	struct sal_bit_err *sal_bit_err = NULL;
	unsigned long flag = 0;

	if (NULL == sal_card) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4524,
			    "ERROR: v_pstSALCard pointer is NULL!");
		return ERROR;
	}

	HIGGS_ASSERT(phy_id < SAL_MAX_PHY_NUM, return ERROR);
	sal_bit_err = &sal_card->phy[phy_id]->err_code;


	/*1.count err code by interrupt */
	spin_lock_irqsave(&sal_bit_err->err_code_lock, flag);
	sal_bit_err->inv_dw_chg_cnt +=
	    sal_card->phy[phy_id]->err_code.phy_err_inv_dw;
	sal_bit_err->disp_err_chg_cnt +=
	    sal_card->phy[phy_id]->err_code.phy_err_disp_err;
	sal_bit_err->cod_vio_chg_cnt +=
	    sal_card->phy[phy_id]->err_code.phy_err_cod_viol;
	sal_bit_err->loss_dw_syn_chg_cnt +=
	    sal_card->phy[phy_id]->err_code.phy_err_loss_dw_syn;

	sal_card->phy[phy_id]->err_code.phy_err_inv_dw = 0;
	sal_card->phy[phy_id]->err_code.phy_err_disp_err = 0;
	sal_card->phy[phy_id]->err_code.phy_err_cod_viol = 0;
	sal_card->phy[phy_id]->err_code.phy_err_loss_dw_syn = 0;
	spin_unlock_irqrestore(&sal_bit_err->err_code_lock, flag);

	/*log err code message*/
	HIGGS_TRACE(OSP_DBL_MINOR, 4533,
		    "Card:%d phy:%d error code sum(InvDw:%d Disp:%d CodeViol:%d LossDwSyn:%d)",
		    sal_card->card_no, phy_id, sal_bit_err->inv_dw_chg_cnt,
		    sal_bit_err->disp_err_chg_cnt, sal_bit_err->cod_vio_chg_cnt,
		    sal_bit_err->loss_dw_syn_chg_cnt);

	return OK;
}

/*
 * change tm type
 */
enum higgs_tm_type higgs_convert_sal_tm_type(enum sal_tm_type sal_type)
{
	enum higgs_tm_type ret = HIGGS_TM_TYPE_BUTT;

	switch (sal_type) {
	case SAL_TM_ABORT_TASK:
		ret = HIGGS_TM_ABORT_TASK;
		break;
	case SAL_TM_ABORT_TASKSET:
		ret = HIGGS_TM_ABORT_TASKSET;
		break;
	case SAL_TM_CLEAR_TASKSET:
		ret = HIGGS_TM_CLEAR_TASKSET;
		break;
	case SAL_TM_LUN_RESET:
		ret = HIGGS_TM_LUN_RESET;
		break;
	case SAL_TM_IT_NEXUS_RESET:
		ret = HIGGS_TM_IT_NEXUS_RESET;
		break;
	case SAL_TM_CLEAR_ACA:
		ret = HIGGS_TM_CLEAR_ACA;
		break;
	case SAL_TM_QUERY_TASK:
		ret = HIGGS_TM_QUERY_TASK;
		break;
	case SAL_TM_QUERY_TASK_SET:
		ret = HIGGS_TM_QUERY_TASK_SET;
		break;
	case SAL_TM_QUERY_UNIT_ATTENTION:
		ret = HIGGS_TM_QUERY_UNIT_ATTENTION;
		break;
	case SAL_TM_TYPE_BUTT:
	default:
		ret = HIGGS_TM_TYPE_BUTT;
		break;
	}
	return ret;
}

/*
 * check chip clock error
 */
s32 higgs_chip_clock_check(struct sal_card * sal_card,
			   enum sal_card_clock * sas_clk_result)
{

	HIGGS_ASSERT(NULL != sal_card, return ERROR);
	HIGGS_ASSERT(NULL != sas_clk_result, return ERROR);
	HIGGS_REF(sal_card);
	HIGGS_REF(sas_clk_result);

	return OK;
}

/*
 * check req timeout
 */
void higgs_req_timeout_check(struct sal_card *sal_card)
{
	u32 i;
	unsigned long flag = 0;
	struct higgs_req *find_higgs_req = NULL;
	struct higgs_card *higgs_card = NULL;
	struct higgs_req_manager *req_manager = NULL;

	HIGGS_ASSERT(NULL != sal_card, return);
	higgs_card = GET_HIGGS_CARD_BY_SAL_CARD(sal_card);
	req_manager = &higgs_card->higgs_req_manager;
	for (i = 0; i < HIGGS_MAX_REQ_NUM; i++) {
		/*1,search in running req*/
		spin_lock_irqsave(&req_manager->req_lock, flag);
		find_higgs_req = higgs_card->running_req[i];
		if (find_higgs_req
		    && (HIGGS_REQ_STATE_SUSPECT == find_higgs_req->req_state)
		    &&
		    jiffies_to_msecs((jiffies -
				      find_higgs_req->req_time_start)) >
		    HIGGS_REQ_OCUPPY_TIMEOUT_THRESHOLD) {

			HIGGS_TRACE(DRV_LOG_INFO, 10,
				    "Higgs Req %d cur state %d, Time out! send event=HIGGS_REQ_EVENT_SUSP_TMO",
				    find_higgs_req->iptt,
				    find_higgs_req->req_state);
			(void)higgs_req_state_chg(find_higgs_req,
						  HIGGS_REQ_EVENT_SUSP_TMO,
						  false);
		}

		spin_unlock_irqrestore(&req_manager->req_lock, flag);
	}

}

/*
 * process the result of aborting error
 */
s32 higgs_process_abort_cfg_efg_err_rsp(struct higgs_req *higgs_req)
{
	struct higgs_card *higgs_card = higgs_req->higgs_card;

	if (HIGGS_ABORT_SINGLE_FLAG == higgs_req->ctx.inter_type) {
		struct sal_msg *sal_msg = NULL;
		sal_msg = higgs_req->sal_msg;
		sal_msg->status.drv_resp = SAL_MSG_DRV_SUCCESS;
		sal_msg->status.io_resp = SAM_STAT_GOOD;
		sal_msg->done(sal_msg);
	} else if (HIGGS_ABORT_DEV_FLAG == higgs_req->ctx.inter_type) {
		struct sal_dev *sal_dev = NULL;
		sal_dev = (struct sal_dev *)higgs_req->ll_dev->up_dev;
		sal_abort_dev_io_cb(sal_dev, SAL_STATUS_SUCCESS);
	} else {
		HIGGS_TRACE(OSP_DBL_MINOR, 171,
			    "Card:%d REQ invalid abort flag, iptt %d",
			    higgs_card->card_id, higgs_req->iptt);
	}

	higgs_req->sal_msg = NULL;
	/*release the req of aborting itself*/
	(void)higgs_req_state_chg(higgs_req, HIGGS_REQ_EVENT_COMPLETE, true);
	return OK;
}
