/*
 * Copyright (C) huawei
 * The program is used to process io
 */
 
#include "higgs_common.h"

#include "higgs_io.h"
#include "higgs_eh.h"
#include "higgs_dump.h"
#include "higgs_pv660.h"
#include "higgs_init.h"
#include "higgs_phy.h"

/*struct higgs_card g_cardL;*/
bool printed_dq_cq = true;

#if defined(PV660_ARM_SERVER)

extern void higgs_oper_led_by_event(struct higgs_card *ll_card,
				    struct higgs_led_event_notify *led_event);

#endif

static inline bool higgs_sal_cmnd_has_data(struct sal_msg *sal_msg)
{
	return (SAL_MSG_DATA_FROM_DEVICE == sal_msg->data_direction ||
		SAL_MSG_DATA_TO_DEVICE == sal_msg->data_direction);
}

static s32 higgs_from_sas_addr_get_phy_id(struct sal_msg *sal_msg,
					  struct higgs_req *higgs_req);


/*
 * get dqid with devid
 */
u32 higgs_get_dqid_from_devid(struct higgs_card * ll_card, u32 devid)
{
	u32 queue_id = 0;
	if (0 == ll_card->higgs_can_dq) {
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_MAJOR, 60 * HZ, 123,
				     "HiggsCard CanDQ is 0");
		return queue_id;	/* CodeCC */
	}
	queue_id = devid % (ll_card->higgs_can_dq);

	HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, 60 * HZ, 1, "DevId:%d GetQueueId:%d",
			     devid, queue_id);
	return queue_id;
}

/*
 * check whether dq is full or not
 */
inline u32 higgs_dq_is_full(struct higgs_card * ll_card, u32 queue_id)
{
	struct higgs_dqueue *dqueue = NULL;

	dqueue = &ll_card->io_hw_struct_info.delivery_queue[queue_id];
	/* write pointer + 1 == read pointer */
	if ((dqueue->wpointer + 1) % (dqueue->dq_depth) ==
	    HIGGS_DQ_RPT_GET(ll_card, queue_id)) {
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_MAJOR, 1 * HZ, 1,
				     "Card:%d DQ:%d is Full pi %d",
				     ll_card->card_id, queue_id,
				     dqueue->wpointer);
		return HIGGS_TRUE;
	}
	HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, 60 * HZ, 1,
			     "Card:%d,DQ:%d not full, WP %d, RP %d",
			     ll_card->card_id, queue_id, dqueue->wpointer,
			     HIGGS_DQ_RPT_GET(ll_card, queue_id));

	return HIGGS_FALSE;
}

static void higgs_init_one_higgs_req(struct higgs_req *higgs_req)
{
	higgs_req->completion = NULL;
	higgs_req->sal_msg = NULL;
	higgs_req->ll_dev = NULL;
	higgs_req->higgs_card = NULL;
	higgs_req->proto_dispatch = NULL;
	higgs_req->cmd_table = NULL;
	higgs_req->dq_info = NULL;
	higgs_req->sge_info = NULL;

	higgs_req->ctx.cmd_type = INTER_CMD_TYPE_NONE;
	higgs_req->ctx.abort_dev_id = 0;
	higgs_req->ctx.abort_tag = 0xFFFF;

	higgs_req->valid_and_in_use = false;	/* For REQ dump  */

	higgs_req->abort_io_in_hdd_ok = false;
	higgs_req->abort_io_in_chip_ok = false;
	higgs_req->req_time_start = 0;
	higgs_req->ctx.sas_addr = 0;

	return;
}

/*
 * req status process when req is in suspect
 */
void higgs_req_suspect_proc(struct higgs_req *req, enum higgs_req_event event,
			    s32 * ret)
{
	struct higgs_card *ll_card = NULL;
	u32 iptt;

	HIGGS_ASSERT(NULL != req, return);
	iptt = req->iptt;
	ll_card = req->higgs_card;

	if (HIGGS_REQ_EVENT_SUSP_TMO == event) {
		HIGGS_REQ_RECLAIM(req, ll_card, iptt);
		*ret = OK;
	} else if (HIGGS_REQ_EVENT_TM_OK == event) {
		req->abort_io_in_hdd_ok = true;
		if (req->abort_io_in_hdd_ok && req->abort_io_in_chip_ok) {
			HIGGS_REQ_RECLAIM(req, ll_card, iptt);

			HIGGS_TRACE(DRV_LOG_INFO, 4738,
				    "card %d, tm cmd release mange iptt %d, success!!!",
				    ll_card->card_id, req->iptt);
		}
		*ret = OK;
	} else if (HIGGS_REQ_EVENT_ABORTCHIP_OK == event) {
		req->abort_io_in_chip_ok = true;
		if (req->abort_io_in_hdd_ok && req->abort_io_in_chip_ok) {
			HIGGS_REQ_RECLAIM(req, ll_card, iptt);

			HIGGS_TRACE(DRV_LOG_INFO, 4738,
				    "card %d, abort cmd release mange iptt %d, success!!!",
				    ll_card->card_id, req->iptt);
		}
		*ret = OK;
	}
	
}

/*
 * req status process
 * @need_lock:need lock or not
 */
s32 higgs_req_state_chg(struct higgs_req *req, enum higgs_req_event event,
			bool need_lock)
{
	struct higgs_card *ll_card = NULL;
	s32 ret = ERROR;
	u32 iptt;
	struct higgs_req_manager *req_manager = NULL;
	unsigned long flag = 0;

	HIGGS_ASSERT(NULL != req->higgs_card, return ERROR);

	iptt = req->iptt;
	ll_card = req->higgs_card;

	if (SAL_OR
	    (req->req_state > HIGGS_REQ_STATE_RUNNING,
	     event == HIGGS_REQ_EVENT_ABORT_ST)) {
		HIGGS_TRACE(DRV_LOG_INFO, 10,
			    "Higgs Req %d cur state %d, event %d", req->iptt,
			    req->req_state, event);
	}

	req_manager = &ll_card->higgs_req_manager;
	if (true == need_lock) {
		spin_lock_irqsave(&req_manager->req_lock, flag);
	}

	switch (req->req_state) {
	case HIGGS_REQ_STATE_FREE:
		if (event == HIGGS_REQ_EVENT_GET) {
			req->req_state = HIGGS_REQ_STATE_RUNNING;
			ret = OK;
		}
		break;
	case HIGGS_REQ_STATE_RUNNING:
		if (event == HIGGS_REQ_EVENT_COMPLETE) {
			HIGGS_REQ_RECLAIM(req, ll_card, iptt);
			ret = OK;
		} else if (event == HIGGS_REQ_EVENT_HALFDONE
			   || event == HIGGS_REQ_EVENT_ABORT_ST) {
			req->req_state = HIGGS_REQ_STATE_SUSPECT;
			req->req_time_start = jiffies;
			ret = OK;
		}
		break;
	case HIGGS_REQ_STATE_SUSPECT:
		higgs_req_suspect_proc(req, event, &ret);
		break;
	case HIGGS_REQ_STATE_BUTT:
	default:
		break;
	}

	if (true == need_lock)
		spin_unlock_irqrestore(&req_manager->req_lock, flag);

	if (ret == ERROR)
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_MAJOR, 10 * HZ, 1,
				     "Higgs Req %d cur state %d , event %d, no st change.",
				     req->iptt, req->req_state, event);
	return ret;
}

struct higgs_req *higgs_get_higgs_req(struct higgs_card *ll_card,
				      enum higgs_inter_cmd_type io_cmd_type)
{
	struct higgs_req_manager *req_manager = NULL;
	struct higgs_req *higgs_req = NULL;
	unsigned long flag = 0;

	HIGGS_ASSERT(NULL != ll_card, return NULL);
	req_manager = &ll_card->higgs_req_manager;
	spin_lock_irqsave(&req_manager->req_lock, flag);

	if (0 == req_manager->req_cnt || SAS_LIST_EMPTY(&req_manager->req_list)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1,
			    "Get Higgs Req Pool is Empty, ReqCount %d",
			    req_manager->req_cnt);
		spin_unlock_irqrestore(&req_manager->req_lock, flag);
		return NULL;
	}

	higgs_req = SAS_LIST_ENTRY(req_manager->req_list.next, struct higgs_req,
				   pool_entry);
	SAS_LIST_DEL_INIT(&higgs_req->pool_entry);
	req_manager->req_cnt--;
	higgs_init_one_higgs_req(higgs_req);

	higgs_req->higgs_card = ll_card;
	higgs_req->ctx.cmd_type = io_cmd_type;
	ll_card->running_req[higgs_req->iptt] = higgs_req;
	(void)higgs_req_state_chg(higgs_req, HIGGS_REQ_EVENT_GET, false);
	spin_unlock_irqrestore(&req_manager->req_lock, flag);

	HIGGS_ASSERT(higgs_req->iptt < ll_card->higgs_can_io, return NULL);

	return higgs_req;
}

/*
 * put req to req-queue
 */
void higgs_put_higgs_req(struct higgs_card *ll_card,
			 struct higgs_req *higgs_req)
{
	struct higgs_req_manager *req_manager = NULL;

	HIGGS_ASSERT(NULL != ll_card, return);
	req_manager = &ll_card->higgs_req_manager;
	SAS_LIST_ADD_TAIL(&higgs_req->pool_entry, &req_manager->req_list);
	req_manager->req_cnt++;
	higgs_init_one_higgs_req(higgs_req);	/* For REQ dump  */

	return;
}

struct higgs_cmd_table *higgs_get_command_table(struct higgs_card *ll_card,
						u32 iptt, OSP_DMA * dma)
{
	struct higgs_command_table_pool *pool = &ll_card->cmd_table_pool;
	struct higgs_cmd_table *ret = NULL;

	*dma = pool->table_entry[iptt].table_dma_addr;
	ret = pool->table_entry[iptt].table_addr;
	if (NULL == ret)
		return NULL;

	memset(&ret->open_addr_frame, 0, sizeof(ret->open_addr_frame));
	memset(ret->rsv1, 0, sizeof(ret->rsv1));
	memset(&ret->status_buff, 0, sizeof(ret->status_buff));
	memset(ret->rsv2, 0, sizeof(ret->rsv2));
	return ret;
}

struct higgs_sge_entry *higgs_get_prd_sge(struct higgs_card *ll_card, u32 iptt,
					  OSP_DMA * dma)
{
	struct higgs_sge_pool *pool = &ll_card->sge_pool;

	*dma = pool->sge_entry[iptt].sge_dma_addr;

	return pool->sge_entry[iptt].sge_addr;
}

void *higgs_get_dq_info(struct higgs_card *ll_card, u32 dq_id)
{
	struct higgs_dqueue *dqueue = NULL;
	struct higgs_dq_info *ret_dq_info = NULL;

	dqueue = &ll_card->io_hw_struct_info.delivery_queue[dq_id];

	if (higgs_dq_is_full(ll_card, dq_id)) {
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_MAJOR, 1 * HZ, 1,
				     "Card:%d DQ:%d is Full!", ll_card->card_id,
				     dq_id);
		return NULL;
	}

	ret_dq_info = &(dqueue->queue_base[dqueue->wpointer]);	
	memset(ret_dq_info, 0, sizeof(struct higgs_dq_info));
	HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, 60 * HZ, 1,
			     "Card:%d,DQ:%d get info %p, WP %d",
			     ll_card->card_id, dq_id, ret_dq_info,
			     dqueue->wpointer);

	return ret_dq_info;
}

static void higgs_dq_global_config(struct higgs_dq_info *dq_info)
{
	/*Global Configure */
	HIGGS_CHF_SSP_PASS(dq_info, SAS_SSP_PSS_THROUGH_DISABLE);
	HIGGS_CHF_MODE(dq_info, SAS_MODE_INITIATOR);
	HIGGS_CHF_SG_MODE(dq_info, SAS_SGE_MODE);

	return;
}

/*
 * both task and other ssp command call it to update write pointer,
 * notify to hardware
 */
void higgs_send_command_hw(struct higgs_card *ll_card,
			   struct higgs_req *higgs_req)
{
	struct higgs_dq_info *dq_info = higgs_req->dq_info;
	struct higgs_dqueue *dq_control = NULL;	/*it contain DQInfo, Write Point and so on */

	HIGGS_ASSERT(NULL != dq_info, return);
	HIGGS_ASSERT(higgs_req->queue_id < HIGGS_MAX_DQ_NUM, return);

	/* For REQ dump */
	higgs_req->valid_and_in_use = true;

	dq_control =
	    &ll_card->io_hw_struct_info.delivery_queue[higgs_req->queue_id];
	HIGGS_CHF_CMMT_LO(dq_info,
			  SAL_LOW_32_BITS(higgs_req->cmd_table_dma +
					  OFFSET_OF_FIELD(struct
							  higgs_cmd_table,
							  table)));
	HIGGS_CHF_CMMT_UP(dq_info,
			  SAL_HIGH_32_BITS(higgs_req->cmd_table_dma +
					   OFFSET_OF_FIELD(struct
							   higgs_cmd_table,
							   table)));

	/*modify by SMMU */
	HIGGS_CHF_STATUS_BUFF_LO(dq_info,
				 SAL_LOW_32_BITS(higgs_req->cmd_table_dma +
						 OFFSET_OF_FIELD(struct
								 higgs_cmd_table,
								 status_buff)));
	HIGGS_CHF_STATUS_BUFF_UP(dq_info,
				 SAL_HIGH_32_BITS(higgs_req->cmd_table_dma +
						  OFFSET_OF_FIELD(struct
								  higgs_cmd_table,
								  status_buff)));

	dq_control->wpointer++;
	if (dq_control->wpointer >= dq_control->dq_depth)
		dq_control->wpointer = 0;


	HIGGS_ASSERT(higgs_req->queue_id < ll_card->higgs_can_dq);
	HIGGS_DQ_WPT_WRITE(ll_card, higgs_req->queue_id, dq_control->wpointer);
	HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, 60 * HZ, 4283,
			     "DQ %d Write WP is %d ,next info %p",
			     higgs_req->queue_id, dq_control->wpointer,
			     &(dq_control->queue_base[dq_control->wpointer]));

	{
		HIGGS_ASSERT((dq_info->status_buffer_address_lower % 16) == 0);
		HIGGS_ASSERT((dq_info->command_table_address_lower % 16) == 0);
		HIGGS_ASSERT((dq_info->prd_table_address_lower % 16) == 0);
		HIGGS_ASSERT((dq_info->dif_prd_table_address_lower % 16) == 0);
	}

	return;
}

/*
 * both task and other ssp command will call it.
 */
void higgs_prepare_base_ssp(struct sal_msg *sal_msg,
			    struct higgs_req *higgs_req)
{
	struct higgs_dq_info *dq_info = higgs_req->dq_info;

	/*global config */
	higgs_dq_global_config(dq_info);

	HIGGS_CHF_CMD(dq_info, SSP_PROTOCOL_CMD);	/*ssp protocol */
	HIGGS_CHF_MODE(dq_info, 1);	/*initiator mode */
	HIGGS_CHF_TLR_CTRL(dq_info, 0x2);	/*bit 6~7 = 0b10 , disable the transport retry */
	HIGGS_CHF_PORT(dq_info, GET_PORT_ID_BY_MSG(sal_msg));	/*which port issue */
	/*there we don't care the force phy */
	HIGGS_CHF_FORCE_PHY(dq_info, 0);
	HIGGS_CHF_PHY_ID(dq_info, 0x1);
	HIGGS_CHF_DEVICE_ID(dq_info, higgs_req->ll_dev->dev_id);
	HIGGS_CHF_RSP_REPO(dq_info, 1);	/*Response frame write to host memory */

	HIGGS_CHF_FIRST_BURST(dq_info, 0);	/*disable first burst */
	HIGGS_CHF_TRAN_RETRY(dq_info, 0);	/*disable transport layer retry */
	HIGGS_CHF_VDT_LEN(dq_info, 1);	/* need verify data length*/
	HIGGS_CHF_MAX_RESP_LEN(dq_info, HIGGS_MAX_RESPONSE_LENGTH >> 2);
	HIGGS_CHF_IPTT(dq_info, higgs_req->iptt);	/*iptt */
	/*for frame other than write data, the initiator should set it 0xFFFF */
	HIGGS_CHF_TPTT(dq_info, 0);

	return;
}

static s32 higgs_translate_sge(struct higgs_card *ll_card, struct sal_msg *msg,
			       struct higgs_sge_entry *higgs_sge,
			       u32 * use_sge_count, u32 * all_data_len)
{
	s32 ret = ERROR;
	struct drv_sge *sge = NULL;
	void *sgl_next = NULL;	
	OSP_DMA dma_addr = 0;
	s32 dma_dir = DMA_NONE;
	u32 i = 0;
	u32 tmp_all_data_len = 0;
	u32 higgs_sge_count = 0;	
	u32 mid_sge_sum_count = 0;	
	struct drv_sgl *sgl_used_in_dri = NULL;

	sgl_used_in_dri = drv_sgl_trans(msg->ini_cmnd.sgl,
					  (void *)msg->ini_cmnd.upper_cmd,
					  (u32) msg->ini_cmnd.port_id,
					  &msg->sgl_head);
	if (NULL == sgl_used_in_dri) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4283, " sgl pointer is NULL!");
		return ERROR;
	}

	if ((mid_sge_sum_count =
	     SAL_GET_ALL_ENTRY_CNT(sgl_used_in_dri)) > HIGGS_MAX_DMA_SEGS) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4360,
			    "mid sgl count:%d ,drv support max sgl:%d",
			    mid_sge_sum_count, HIGGS_MAX_DMA_SEGS);
		return ERROR;
	}

	dma_dir = sal_data_dir_to_pcie_dir(msg->data_direction);
	sgl_next = msg->ini_cmnd.sgl;
	for (;;) {
		for (i = 0; i < SAL_SGL_CNT(sgl_used_in_dri); i++) {
			if (higgs_sge_count >= mid_sge_sum_count) {
				HIGGS_TRACE(OSP_DBL_MAJOR, 4283,
					    "Sge Count:%d bigger than sum sge:%d",
					    higgs_sge_count, mid_sge_sum_count);
				ret = ERROR;
				goto FREE_RSC;
			}
			sge = &(sgl_used_in_dri->sge[i]);	/* get one sge */
			HIGGS_ASSERT(NULL != sge, return ERROR);
			HIGGS_ASSERT(NULL != sge->buff, return ERROR);
			//   1. map
			dma_addr =
			    higgs_dma_map_single_sge_buffer(ll_card, sge->buff,
							    sge->len,
							    dma_dir);

			/* fill each SGE */

			HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, 60 * HZ, 100,
					     "sg phys addr %llx",
					     (u64) dma_addr);

			if ((u64) higgs_sge <= 0xffffff0000000000) {
				HIGGS_TRACE(OSP_DBL_MAJOR, 4283,
					    "pstHiggsSge %p, uiLoop %d ,ll tag %d",
					    higgs_sge, i, msg->ll_tag);
				msleep(500);
			}

			higgs_sge->addr_low =
			    (u32) SAS_CPU_TO_LE32(SAL_LOW_32_BITS(dma_addr));
			higgs_sge->addr_hi =
			    (u32) SAS_CPU_TO_LE32(SAL_HIGH_32_BITS(dma_addr));
			higgs_sge->page_ctrol0 = 0;
			higgs_sge->page_ctrol1 = 0;
			higgs_sge->data_Len = (u32) SAS_CPU_TO_LE32(sge->len);	/*data length */
			higgs_sge->data_offset = 0;	/*ulDmaAddr is data addr, so the offset is zero */
			tmp_all_data_len += higgs_sge->data_Len;

			higgs_sge_count++;
			higgs_sge++;
		}

		sgl_next = sgl_used_in_dri->next_sgl;
		if (sgl_next && (higgs_sge_count < mid_sge_sum_count)) {
			memset(&msg->sgl_head, 0, sizeof(struct drv_sgl));

			sgl_used_in_dri = drv_sgl_trans(sgl_next,
							  (void *)msg->ini_cmnd.
							  upper_cmd,
							  (u32) msg->ini_cmnd.
							  port_id,
							  &msg->sgl_head);
			if (NULL == sgl_used_in_dri) {
				HIGGS_TRACE(OSP_DBL_MAJOR, 4283,
					    "sgl transfer failed!");
				ret = ERROR;
				goto FREE_RSC;
			}
		} else
			break;
	}

	*all_data_len = tmp_all_data_len;
	*use_sge_count = higgs_sge_count;

	return OK;

      FREE_RSC:

	return ret;

}

/*
 * write and read command sge parepare
 */
static s32 higgs_prepare_prdsge(struct higgs_card *ll_card,
				struct sal_msg *sal_msg,
				struct higgs_req *higgs_req,
				struct higgs_dq_info *dq_info)
{
	u32 data_sge_cnt = 0;
	u32 sum_data_len = 0;
	s32 ret = OK;

	HIGGS_CHF_PIR_PRESENT(dq_info, 0);
	HIGGS_CHF_T10_FLDS_PRSNT(dq_info, 0);
	HIGGS_CHF_PRD_LEN(dq_info, 0);
	HIGGS_CHF_PRD_TABLE_LO(dq_info, 0);
	HIGGS_CHF_PRD_TABLE_UP(dq_info, 0);
	if (SAS_SGE_MODE == dq_info->dw2.st.sg_mode) {
		u32 iptt = higgs_req->iptt;
		OSP_DMA dma;

		struct higgs_sge_entry *higgs_sge =
		    higgs_get_prd_sge(ll_card, iptt, &dma);

		ret =
		    higgs_translate_sge(ll_card, sal_msg, higgs_sge,
					&data_sge_cnt, &sum_data_len);
		if (ERROR == ret) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4283,
				    "ERROR: data SGE Is NULL");
			return ret;
		}

		HIGGS_CHF_PRD_LEN(dq_info, data_sge_cnt);
		HIGGS_CHF_DT_LEN(dq_info, sal_msg->ini_cmnd.data_len);
		HIGGS_CHF_DT_LEN(dq_info, sum_data_len);
		if (SAL_IS_RW_CMND(sal_msg->ini_cmnd.cdb[0])) {
			if (unlikely
			    (sum_data_len != sal_msg->ini_cmnd.data_len)) {
				HIGGS_TRACE_FRQLIMIT(OSP_DBL_MAJOR, 60 * HZ,
						     4283,
						     "REQ iptt %d, inicmd data len %d != sum sge len %d.\n",
						     iptt,
						     sal_msg->ini_cmnd.data_len,
						     sum_data_len);
				return ERROR;
			}
		}

		HIGGS_CHF_PRD_TABLE_LO(dq_info, SAL_LOW_32_BITS(dma));
		HIGGS_CHF_PRD_TABLE_UP(dq_info, SAL_HIGH_32_BITS(dma));

		HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, 60 * HZ, 4283,
				     "REQ iptt %d, SGE count %d, inicmd data len %d, sum sge len %d, Entry BaseAddr Virt:0x%llx, Dma:0x%llx",
				     iptt, data_sge_cnt,
				     sal_msg->ini_cmnd.data_len, sum_data_len,
				     (u64) higgs_sge, (u64) dma);
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, 60 * HZ, 4283,
				     "sge0 Low 0x%08x, High 0x%08x, Len %d",
				     higgs_sge->addr_low, higgs_sge->addr_hi,
				     higgs_sge->data_Len);
	} else {
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_MAJOR, 10 * HZ, 1,
				     "Card:%d unsupport Mode %d!",
				     ll_card->card_id, dq_info->dw2.st.sg_mode);
		return ERROR;
	}

	return OK;
}

/*transfer to the direction the chip support */
static u32 higgs_get_data_direction(struct sal_msg *sal_msg)
{
	u32 fw_direct = 0;

	switch (sal_msg->data_direction) {
	case SAL_MSG_DATA_TO_DEVICE:
		fw_direct = HIGGS_INI_DATA_TO_DEVICE;	/*2, WRITE */
		break;

	case SAL_MSG_DATA_FROM_DEVICE:
		fw_direct = HIGGS_INI_DATA_FROM_DEVICE;	/*1, READ */
		break;

	case SAL_MSG_DATA_BIDIRECTIONAL:	
	case SAL_MSG_DATA_NONE:
	case SAL_MSG_DATA_BUTT:
	default:
		fw_direct = HIGGS_INI_NO_DATA;	/*0, NO-DATA */
		break;
	}

	HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, 60 * HZ, 1,
			     "Cmd:0x%02x sal dir %d, Get Direct %d",
			     GET_SCSI_CMD_BY_MSG(sal_msg),
			     sal_msg->data_direction, fw_direct);

	return fw_direct;
}

/*
 * fill the ssp dq
 */
enum sal_cmnd_exec_result higgs_prepare_ssp(struct higgs_req *higgs_req)
{
	struct sal_msg *sal_msg = higgs_req->sal_msg;
	struct higgs_card *ll_card = higgs_req->higgs_card;
	struct higgs_dq_info *dq_info = higgs_req->dq_info;
	struct higgs_cmd_table *cmd_tab = higgs_req->cmd_table;
	struct sas_ssp_cmnd_uint *ssp_iu = NULL;
	u32 direct;
#if defined(PV660_ARM_SERVER)
	u32 phy_id;
#endif

	HIGGS_ASSERT(NULL != dq_info, return SAL_CMND_EXEC_FAILED);
	HIGGS_ASSERT(NULL != cmd_tab, return SAL_CMND_EXEC_FAILED);

	higgs_prepare_base_ssp(sal_msg, higgs_req);

	HIGGS_CHF_PRI(dq_info, 0);	/*normal priority */

	direct = higgs_get_data_direction(sal_msg);
	HIGGS_CHF_SSP_FRAME(dq_info, direct);

	HIGGS_CHF_DT_LEN(dq_info, 0);
	HIGGS_CHF_CMMF_LEN(dq_info,
			   (((sizeof(struct sas_ssp_cmnd_uint) +
			      sizeof(struct sas_ssp_frame_header) + 3)
			     / (sizeof(u32))) & 0x1FF));	/*9 bit */

	/*the command header end, command table begin */
	ssp_iu = (struct sas_ssp_cmnd_uint *)HIGGS_CMT_SSP_COMMAND_IU(cmd_tab);
	(void)memset(ssp_iu, 0, sizeof(struct sas_ssp_cmnd_uint));

	(void)memcpy((void *)ssp_iu, &sal_msg->protocol.ssp,
		     sizeof(struct sas_ssp_cmnd_uint));

	HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, 60 * HZ, 1,
			     "scsi id %d CDB:%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
			     sal_msg->dev->scsi_id, ssp_iu->cdb[0],
			     ssp_iu->cdb[1], ssp_iu->cdb[2], ssp_iu->cdb[3],
			     ssp_iu->cdb[4], ssp_iu->cdb[5], ssp_iu->cdb[6],
			     ssp_iu->cdb[7], ssp_iu->cdb[8], ssp_iu->cdb[9]);

	/*WRITE, READ, or NONE-RW which has data */
	if (higgs_sal_cmnd_has_data(sal_msg)) {
		if (ERROR ==
		    higgs_prepare_prdsge(ll_card, sal_msg, higgs_req,
					 dq_info)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 1,
				    "Card:%d PrdSgePrepare failed",
				    ll_card->card_id);
			/*FIXME: release higgs req and dq,cmndtable */
			return SAL_CMND_EXEC_FAILED;
		}
	}
#if defined(PV660_ARM_SERVER)
	phy_id = higgs_from_sas_addr_get_phy_id(sal_msg, higgs_req);
	if (ERROR != phy_id) {
		ll_card->phy[phy_id].phy_is_idle = false;
	}
#endif

	return SAL_CMND_EXEC_SUCCESS;
}

/*
 * update write pointer send ssp
 */

enum sal_cmnd_exec_result higgs_send_ssp(struct higgs_req *higgs_req)
{
	struct sal_msg *sal_msg = higgs_req->sal_msg;
	struct higgs_card *ll_card = higgs_req->higgs_card;

	higgs_req->cmd = GET_SCSI_CMD_BY_MSG(sal_msg);

	higgs_send_command_hw(ll_card, higgs_req);
	return SAL_CMND_EXEC_SUCCESS;
}

/* check error of DW0 */
enum higgs_io_err_type_e higgs_check_io_error_dma(u32 error_dw)
{
	u32 loop_num = 0;
	u32 err_dw_shifted = 0;
	enum higgs_io_err_type_e err_type[32] = {
		HIGGS_IO_DIF_ERROR,	/* 0 */
		HIGGS_IO_DIF_ERROR,
		HIGGS_IO_DIF_ERROR,
		HIGGS_IO_BUS_ERROR,
		HIGGS_IO_OVERFLOW,
		HIGGS_IO_OVERFLOW,
		HIGGS_IO_UNEXP_XFER_RDY,
		HIGGS_IO_XFER_RDY_OFFSET,
		HIGGS_IO_UNDERFLOW,	/* 8 */
		HIGGS_IO_XFR_RDY_LENGTH_OVERFLOW_ERR,
		HIGGS_IO_ERROR_TYPE_BUTT,
		HIGGS_IO_ERROR_TYPE_BUTT,
		HIGGS_IO_ERROR_TYPE_BUTT,
		HIGGS_IO_ERROR_TYPE_BUTT,
		HIGGS_IO_ERROR_TYPE_BUTT,
		HIGGS_IO_ERROR_TYPE_BUTT,
		HIGGS_IO_RX_BUFFER_ECC_ERR,	/* 16 */
		HIGGS_IO_RX_DIF_CRC_ERR,
		HIGGS_IO_RX_DIF_APP_ERR,
		HIGGS_IO_RX_DIF_RPP_ERR,
		HIGGS_IO_RX_RESP_BUFFER_OVERFLOW_ERR,
		HIGGS_IO_RX_AXI_BUS_ERR,
		HIGGS_IO_RX_DATA_SGL_OVERFLOW_ERR,
		HIGGS_IO_RX_DIF_SGL_OVERFLOW_ERR,
		HIGGS_IO_RX_DATA_OFFSET_ERR,
		HIGGS_IO_RX_UNEXP_RX_DATA_ERR,
		HIGGS_IO_RX_DATA_OVERFLOW_ERR,
		HIGGS_IO_UNDERFLOW,	/* 27 */
		HIGGS_IO_RX_UNEXP_RETRANS_RESP_ERR,
		HIGGS_IO_ERROR_TYPE_BUTT,
		HIGGS_IO_ERROR_TYPE_BUTT,
		HIGGS_IO_ERROR_TYPE_BUTT
	};

	if (SAL_OR
	    (error_dw & DW0_IO_TX_DATA_UNDERFLOW_ERR,
	     error_dw & DW0_IO_RX_DATA_UNDERFLOW_ERR))
		error_dw = error_dw | (1 << 8);

	for (loop_num = 0; loop_num < 32; loop_num++) {
		err_dw_shifted = error_dw >> loop_num;
		if ((err_dw_shifted & 1) &&
		    (HIGGS_IO_ERROR_TYPE_BUTT != err_type[loop_num]))
			return err_type[loop_num];
	}

	return HIGGS_IO_ERROR_TYPE_BUTT;
}

/* check error of dw1 */
enum higgs_io_err_type_e higgs_check_io_error_tx(const u32 error_dw)
{
	u32 loop_num = 0;
	u32 err_dw_shifted = 0;
	enum higgs_io_err_type_e err_type[32] = { HIGGS_IO_ERROR_TYPE_BUTT,	/* 0 */
		HIGGS_IO_PHY_NOT_RDY,
		HIGGS_IO_OPEN_REJCT_WRONG_DEST_ERR,
		HIGGS_IO_OPEN_REJCT_ZONE_VIOLATION_ERR,
		HIGGS_IO_OPEN_REJCT_BY_OTHER_ERR,
		HIGGS_IO_ERROR_TYPE_BUTT,	/* 5 */
		HIGGS_IO_OPEN_REJCT_AIP_TIMEOUT_ERR,
		HIGGS_IO_OPEN_REJCT_STP_BUSY_ERR,
		HIGGS_IO_OPEN_REJCT_PROTOCOL_NOT_SUPPORT_ERR,
		HIGGS_IO_OPEN_REJCT_RATE_NOT_SUPPORT_ERR,
		HIGGS_IO_OPEN_REJCT_BAD_DEST_ERR,
		HIGGS_IO_BREAK_RECEIVE_ERR,
		HIGGS_IO_LOW_PHY_POWER_ERR,
		HIGGS_IO_OPEN_REJCT_PATHWAY_BLOCKED_ERR,
		HIGGS_IO_OPEN_TIMEOUT_ERR,
		HIGGS_IO_OPEN_REJCT_NO_DEST_ERR,
		HIGGS_IO_OPEN_RETRY_ERR,	/* 16 */
		HIGGS_IO_ERROR_TYPE_BUTT,
		HIGGS_IO_TX_BREAK_TIMEOUT_ERR,
		HIGGS_IO_TX_BREAK_REQUEST_ERR,
		HIGGS_IO_TX_BREAK_RECEIVE_ERR,
		HIGGS_IO_TX_CLOSE_TIMEOUT_ERR,
		HIGGS_IO_TX_CLOSE_NORMAL_ERR,
		HIGGS_IO_TX_CLOSE_PHYRESET_ERR,
		HIGGS_IO_TX_WITH_CLOSE_DWS_TIMEOUT_ERR,
		HIGGS_IO_TX_WITH_CLOSE_COMINIT_ERR,
		HIGGS_IO_TX_NAK_RECEIVE_ERR,
		HIGGS_IO_TX_ACK_NAK_TIMEOUT_ERR,
		HIGGS_IO_TX_CREDIT_TIMEOUT_ERR,
		HIGGS_IO_IPTT_CONFLICT_ERR,
		HIGGS_IO_TXFRM_TYPE_ERR,
		HIGGS_IO_TXSMP_LENGTH_ERR
	};

	for (loop_num = 0; loop_num < 32; loop_num++) {
		err_dw_shifted = error_dw >> loop_num;
		if ((err_dw_shifted & 1) &&
		    (HIGGS_IO_ERROR_TYPE_BUTT != err_type[loop_num]))
			return err_type[loop_num];
	}

	return HIGGS_IO_ERROR_TYPE_BUTT;
}

/* check error of DW2 */
enum higgs_io_err_type_e higgs_check_io_error_rx(const u32 error_dw)
{
	u32 loop_num = 0;
	u32 err_dw_shifted = 0;
	enum higgs_io_err_type_e err_type[32] = { HIGGS_IO_RX_FRAME_CRC_ERR,	/*0 */
		HIGGS_IO_RX_FRAME_DONE_ERR,
		HIGGS_IO_RX_FRAME_ERRPRM_ERR,
		HIGGS_IO_RX_FRAME_NO_CREDIT_ERR,
		HIGGS_IO_ERROR_TYPE_BUTT,	/*4 */
		HIGGS_IO_RX_FRAME_OVERRUN_ERR,
		HIGGS_IO_RX_FRAME_NO_EOF_ERR,
		HIGGS_IO_LINK_BUF_OVERRUN_ERR,
		HIGGS_IO_RX_BREAK_TIMEOUT_ERR,
		HIGGS_IO_RX_BREAK_REQUEST_ERR,
		HIGGS_IO_RX_BREAK_RECEIVE_ERR,
		HIGGS_IO_RX_CLOSE_TIMEOUT_ERR,
		HIGGS_IO_RX_CLOSE_NORMAL_ERR,
		HIGGS_IO_RX_CLOSE_PHYRESET_ERR,
		HIGGS_IO_RX_WITH_CLOSE_DWS_TIMEOUT_ERR,
		HIGGS_IO_RX_WITH_CLOSE_COMINIT_ERR,
		HIGGS_IO_DATA_LENGTH0_ERR,
		HIGGS_IO_BAD_HASH_ERR,
		HIGGS_IO_RX_XRDY_ZERO_ERR,
		HIGGS_IO_RX_SSP_FRAME_LEN_ERR,
		HIGGS_IO_ERROR_TYPE_BUTT,	/*20 */
		HIGGS_IO_RX_NO_BALANCE_ERR,
		HIGGS_IO_ERROR_TYPE_BUTT,
		HIGGS_IO_ERROR_TYPE_BUTT,
		HIGGS_IO_RX_BAD_FRAME_TYPE_ERR,	/* 24 */
		HIGGS_IO_RX_SMP_FRAME_LEN_ERR,
		HIGGS_IO_RX_SMP_RESP_TIMEOUT_ERR,
		HIGGS_IO_ERROR_TYPE_BUTT,
		HIGGS_IO_ERROR_TYPE_BUTT,
		HIGGS_IO_ERROR_TYPE_BUTT,
		HIGGS_IO_ERROR_TYPE_BUTT,
		HIGGS_IO_ERROR_TYPE_BUTT
	};

	for (loop_num = 0; loop_num < 32; loop_num++) {
		err_dw_shifted = error_dw >> loop_num;
		if ((err_dw_shifted & 1) &&
		    (HIGGS_IO_ERROR_TYPE_BUTT != err_type[loop_num]))
			return err_type[loop_num];
	}

	return HIGGS_IO_ERROR_TYPE_BUTT;
}

/*error info£¬analyse */
enum higgs_io_err_type_e higgs_get_error_detail(void *cmd_tab,
						struct higgs_req *higgs_req,
						u32 cmpl_entry)
{
	enum higgs_io_err_type_e ret = HIGGS_IO_ERROR_TYPE_BUTT;
	u8 *err_info = (u8 *) HIGGS_CMT_STATUS_ERROR_INFO(cmd_tab);
	u32 err_info0 = HIGGS_ERROR_INFO_DW0(err_info);
	u32 err_info1 = HIGGS_ERROR_INFO_DW1(err_info);
	u32 err_info2 = HIGGS_ERROR_INFO_DW2(err_info);
	u32 err_info3 = HIGGS_ERROR_INFO_DW3(err_info);
	u32 hgc_invld_dqe_Info_reg = 0;

	/* dq cfg err */
	if (HIGGS_CQ_CFG_ERR(&cmpl_entry)) {
		hgc_invld_dqe_Info_reg =
		    HIGGS_GLOBAL_REG_READ(higgs_req->higgs_card,
					  HISAS30HV100_GLOBAL_REG_HGC_INVLD_DQE_INFO_REG);
		if (0x800000 == (hgc_invld_dqe_Info_reg & 0xff0000)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 1, "Port Not Ready: Retry");
			return HIGGS_IO_PORT_NOT_READY_ERR;
		}
	}

	if (HIGGS_CQ_ERR_RCRD_XFRD(&cmpl_entry) == 0
	    && HIGGS_CQ_RSPNS_XFRD(&cmpl_entry)) {
		ret = HIGGS_IO_SUCC;
		goto CHECK_DONE;
	}

	/* DW 0 */
	ret = higgs_check_io_error_dma(err_info0);
	if (ret != HIGGS_IO_ERROR_TYPE_BUTT)
		goto CHECK_DONE;

	/* DW 1 */
	ret = higgs_check_io_error_tx(err_info1);
	if (ret != HIGGS_IO_ERROR_TYPE_BUTT)
		goto CHECK_DONE;

	/* DW 2 */
	ret = higgs_check_io_error_rx(err_info2);
	if (ret != HIGGS_IO_ERROR_TYPE_BUTT)
		goto CHECK_DONE;

	/* DW3 is reserved */
	HIGGS_REF(err_info3);

      CHECK_DONE:
	if (SAL_OR(HIGGS_IO_SUCC == ret, HIGGS_IO_ERROR_TYPE_BUTT == ret)) {
		if (!HIGGS_CQ_CMD_CMPLT(&cmpl_entry)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 1,
				    "CMD:0x%02x: DEV:%d iptt:0x%x:%d Complete Not Set!",
				    higgs_req->cmd, higgs_req->ll_dev->dev_id,
				    higgs_req->iptt, higgs_req->iptt);
			return HIGGS_IO_CMD_NOT_CMPLETE;	
		}

		if (!(HIGGS_CQ_RSPNS_XFRD(&cmpl_entry))) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 1,
				    "Request:0x%02x DEV:%d iptt:0x%x:%d Response Not Transfer:0x%x",
				    higgs_req->cmd, higgs_req->ll_dev->dev_id,
				    higgs_req->iptt, higgs_req->iptt,
				    cmpl_entry);
			return HIGGS_IO_NO_RESPONSE;	
		}
	}

	return ret;
}

/*fill resp in sal msg with response*/
static void higgs_fill_task_rsp(struct sal_msg *sal_msg,
				struct sas_ssp_resp *resp_iu)
{
	u32 resp_len = 0;
	u8 resp_code;
	/*The DATAPRES field must be RESPONSE_DATA */
	if (!(resp_iu->data_pres & 0x1)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1,
			    "The data_pres:0x%x Not RESPONSE_DATA",
			    resp_iu->data_pres);
		return;
	}
	resp_len = HIGGS_SWAP_32(resp_iu->resp_data_len);
	/*The SSP target port set the RESPONSE DATA LENGTH 0x4 */
	HIGGS_ASSERT(0x4 == resp_len, return);
	resp_code = resp_iu->data[3];

	sal_msg->status.drv_resp = SAL_MSG_DRV_SUCCESS;
	sal_msg->status.io_resp = resp_code;
	HIGGS_REF(resp_len);
	return;
}

void dump_sge_buffer(struct sal_msg *sal_msg)
{
	char tmp_buf[64];
	if (sal_msg->ini_cmnd.sgl == NULL)
		return;

	HIGGS_TRACE(OSP_DBL_INFO, 4682,
		    "show msg %p scsi id %d ,cdb0 0x%x sge buffer", sal_msg,
		    sal_msg->dev->scsi_id, sal_msg->ini_cmnd.cdb[0]
	    );
	{
		struct drv_sgl *sgl_used_in_dri = NULL;
		sgl_used_in_dri = drv_sgl_trans(sal_msg->ini_cmnd.sgl,
						  (void *)sal_msg->ini_cmnd.
						  upper_cmd,
						  (u32) sal_msg->ini_cmnd.
						  port_id, &sal_msg->sgl_head);
		if (NULL == sgl_used_in_dri) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4283,
				    " sgl pointer is NULL!");
			return;
		}

		(void)snprintf(tmp_buf, sizeof(tmp_buf), "SGE 0 buffer len %d",
			       sgl_used_in_dri->sge[0].len);
		higgs_hex_dump(tmp_buf, (u8 *) sgl_used_in_dri->sge[0].buff, 32,
			       1);
	}
}

/*check whether IO success with response IU */
void higgs_get_result_by_ssp_rsp_iu(struct sal_msg *sal_msg,
				    struct sas_ssp_resp *resp_iu)
{
	u32 sense_len = SAS_BE32_TO_CPU(resp_iu->sense_data_len);
	u8 status = resp_iu->status;
	sal_msg->status.drv_resp = SAL_MSG_DRV_SUCCESS;
	switch (resp_iu->data_pres) {
	case SSP_RSP_DATAPRES_NO_DATA:
		sal_msg->status.io_resp = status;
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, 60 * HZ, 1,
				     "sal msg %p %d and status %d", sal_msg,
				     resp_iu->data_pres, status);
		break;
	case SSP_RSP_DATAPRES_RSP_DATA:
		if (SAL_IS_RW_CMND(sal_msg->ini_cmnd.cdb[0])) {
			sal_msg->status.io_resp = SAM_STAT_CHECK_CONDITION;
			sal_msg->status.drv_resp = SAL_MSG_DRV_FAILED;
		} else {
			sal_msg->status.io_resp = status;
		}
		HIGGS_TRACE(OSP_DBL_INFO, 4682,
			    "SSP INI data pres rsp,msg:%p uni id:0x%llx,Status:0x%x",
			    sal_msg, SAL_MSG_UNIID(sal_msg), status);
		break;
	case SSP_RSP_DATAPRES_SENSE_DATA:
		sal_msg->status.io_resp = status;
		if (status != SAM_STAT_CHECK_CONDITION) {
			/*data pre not equal to status, print */
			HIGGS_TRACE_FRQLIMIT(OSP_DBL_MAJOR, HZ, 1,
					     "sal msg %p %d and status %d not match",
					     sal_msg, resp_iu->data_pres,
					     status);
		}
		sense_len = MIN(MAX_SSP_RESP_DATA_SIZE, sense_len);
		sense_len = MIN(SAL_MAX_SENSE_LEN, sense_len);
		sal_msg->status.sense_len = sense_len;
		memcpy(sal_msg->status.sense, resp_iu->data, sense_len);
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, HZ, 1,
				     "salmsg %p scsi id %d sas addr "
				     "0x%llx, sense len %d | %02X %02X %02X %02X %02X %02X %02X %02X | "
				     "%02X %02X %02X %02X %02X %02X %02X %02X",
				     sal_msg, sal_msg->dev->scsi_id,
				     sal_msg->dev->sas_addr, sense_len,
				     sal_msg->status.sense[0],
				     sal_msg->status.sense[1],
				     sal_msg->status.sense[2],
				     sal_msg->status.sense[3],
				     sal_msg->status.sense[4],
				     sal_msg->status.sense[5],
				     sal_msg->status.sense[6],
				     sal_msg->status.sense[7],
				     sal_msg->status.sense[8],
				     sal_msg->status.sense[9],
				     sal_msg->status.sense[10],
				     sal_msg->status.sense[11],
				     sal_msg->status.sense[12],
				     sal_msg->status.sense[13],
				     sal_msg->status.sense[14],
				     sal_msg->status.sense[15]);
		break;
	default:
		/* impossible */
		sal_msg->status.io_resp = SAM_STAT_CHECK_CONDITION;
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_MAJOR, HZ, 1,
				     "sal msg %p %d and status %d not match",
				     sal_msg, resp_iu->data_pres, status);
		break;
	}
}

static void higgs_print_error_info(struct sal_msg *sal_msg,
				   struct higgs_req *higgs_req,
				   enum higgs_io_err_type_e io_err_type)
{
	struct higgs_sge_entry *higgs_sge = NULL;
	OSP_DMA dma = 0;
	struct higgs_device *ll_dev;	/*LL DEV pointer, get device id */

	u8 *err_info = (u8 *) HIGGS_CMT_STATUS_ERROR_INFO(higgs_req->cmd_table);
	u32 err_info0 = HIGGS_ERROR_INFO_DW0(err_info);
	u32 err_info1 = HIGGS_ERROR_INFO_DW1(err_info);
	u32 err_info2 = HIGGS_ERROR_INFO_DW2(err_info);
	u32 err_info3 = HIGGS_ERROR_INFO_DW3(err_info);

	ll_dev = higgs_req->ll_dev;
	if (NULL == ll_dev) {
		/* abnormal req */
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_MAJOR, HZ, 1545,
				     "iptt %d higgs dev is null",
				     higgs_req->iptt);
		return;
	}

	if (ll_dev->err_info0 == err_info0
	    && ll_dev->err_info1 == err_info1
	    && ll_dev->err_info2 == err_info2
	    && ll_dev->err_info3 == err_info3) {
		if (time_before
		    (jiffies,
		     ll_dev->last_print_err_jiff +
		     IO_ERROR_PRINT_SILENCE_S * HZ))
			return;
	}

	ll_dev->err_info0 = err_info0;
	ll_dev->err_info1 = err_info1;
	ll_dev->err_info2 = err_info2;
	ll_dev->err_info3 = err_info3;
	ll_dev->last_print_err_jiff = jiffies;

	HIGGS_TRACE(OSP_DBL_MAJOR, 1645,
		    "sal msg %p, ll devid %d iptt %d cmpl_entry %08x, eIOErrorType %d, %08x,%08x,%08x,%08x",
		    sal_msg, ll_dev->dev_id, higgs_req->iptt,
		    higgs_req->cmpl_entry, io_err_type, err_info0, err_info1,
		    err_info2, err_info3);

	if (io_err_type == 5000
	    && (err_info0 | err_info1 | err_info2 | err_info3) == 0) {
		struct higgs_card *ll_card = higgs_req->higgs_card;
		u64 uiAllSizeCQ =
		    (u64) (ll_card->card_cfg.obqueue_num_elements)
		    * (u64) HIGGS_CQ_ENTRY_SIZE * (u64) 4;	/* 4 */
		u64 uiAllSizeDQ =
		    (u64) (ll_card->card_cfg.ibqueue_num_elements)
		    * (u64) HIGGS_DQ_ENTRY_SIZE * (u64) 4;
		if (printed_dq_cq == false) {
			printed_dq_cq = true;
			higgs_hex_dump("All DQ",
				       ll_card->io_hw_struct_info.all_dq_base,
				       (u32) uiAllSizeDQ, 4);
			higgs_hex_dump("All CQ",
				       ll_card->io_hw_struct_info.all_cq_base,
				       (u32) uiAllSizeCQ, 4);
		}
	}

	if (err_info0 == 0x08000000) {
		if (sal_msg->ini_cmnd.cdb[0] == 0x12
		    || sal_msg->ini_cmnd.cdb[0] == 0xA3
		    || sal_msg->ini_cmnd.cdb[0] == 0x1A)
			return;
	}

	higgs_hex_dump("ssp dq req in print error", (u8 *) (higgs_req->dq_info),
		       sizeof(struct higgs_dq_info), sizeof(u32));
	higgs_hex_dump("ssp cmd table in print error",
		       (u8
			*) (&((struct higgs_cmd_table *)higgs_req->cmd_table)->
			    table), 64, 1);

	higgs_sge =
	    higgs_get_prd_sge(higgs_req->higgs_card, higgs_req->iptt, &dma);
	higgs_hex_dump("pstHiggsSge 0", (u8 *) (higgs_sge),
		       sizeof(struct higgs_sge_entry), sizeof(u32));
}

/*
 * check whether the reason io failed is link error with error info
 */
static void tm_io_err_type_check(enum higgs_io_err_type_e err_type,
				 struct sal_msg *sal_msg)
{
	u32 tmp = (u32) err_type;
	HIGGS_ASSERT(sal_msg != NULL, return);

	switch (tmp) {
	case HIGGS_IO_PHY_NOT_RDY:
	case HIGGS_IO_OPEN_REJCT_WRONG_DEST_ERR:
	case HIGGS_IO_OPEN_REJCT_ZONE_VIOLATION_ERR:
	case HIGGS_IO_OPEN_REJCT_BY_OTHER_ERR:
	case HIGGS_IO_OPEN_REJCT_AIP_TIMEOUT_ERR:
	case HIGGS_IO_OPEN_REJCT_STP_BUSY_ERR:
	case HIGGS_IO_OPEN_REJCT_PROTOCOL_NOT_SUPPORT_ERR:
	case HIGGS_IO_OPEN_REJCT_RATE_NOT_SUPPORT_ERR:
	case HIGGS_IO_OPEN_REJCT_BAD_DEST_ERR:
	case HIGGS_IO_BREAK_RECEIVE_ERR:
	case HIGGS_IO_LOW_PHY_POWER_ERR:
	case HIGGS_IO_OPEN_REJCT_PATHWAY_BLOCKED_ERR:
	case HIGGS_IO_OPEN_TIMEOUT_ERR:
	case HIGGS_IO_OPEN_REJCT_NO_DEST_ERR:
	case HIGGS_IO_OPEN_RETRY_ERR:
	case HIGGS_IO_TX_NAK_RECEIVE_ERR:
	case HIGGS_IO_TX_ACK_NAK_TIMEOUT_ERR:
	case HIGGS_IO_RX_FRAME_CRC_ERR:
	case HIGGS_IO_RX_FRAME_DONE_ERR:
	case HIGGS_IO_RX_FRAME_ERRPRM_ERR:
	case HIGGS_IO_RX_FRAME_NO_CREDIT_ERR:
	case HIGGS_IO_ERROR_TYPE_BUTT:
	case HIGGS_IO_RX_FRAME_OVERRUN_ERR:
	case HIGGS_IO_RX_FRAME_NO_EOF_ERR:
		/* error info dw0:19 */
	case HIGGS_IO_RX_UNEXP_RETRANS_RESP_ERR:
	case HIGGS_IO_RX_RESP_BUFFER_OVERFLOW_ERR:
		sal_msg->transmit_mark = true;
		break;
	default:
		sal_msg->transmit_mark = false;
		break;
	}
}

/*
 * tm response proc
 */
static s32 higgs_tm_response(struct sal_msg *sal_msg,
			     struct higgs_req *higgs_req, u32 cmpl_entry)
{
	struct sas_ssp_resp *resp_iu = NULL;
	void *cmd_tab = NULL;
	struct higgs_req *higgs_org_req;
	enum higgs_io_err_type_e io_err_type = HIGGS_IO_ERROR_TYPE_BUTT;
	struct higgs_req_manager *req_manager = NULL;
	unsigned long flag = 0;
	struct sal_dev *sal_dev = NULL;
	u32 loop_num = 0;

	HIGGS_TRACE(OSP_DBL_INFO, 1, "iptt: %d 0x%x come back, TM func %d",
		    higgs_req->iptt, higgs_req->iptt,
		    higgs_req->ctx.tm_function);

	HIGGS_REF(cmpl_entry);

	cmd_tab = higgs_req->cmd_table;
	if (NULL == cmd_tab)
		return ERROR;

	io_err_type = higgs_get_error_detail(cmd_tab, higgs_req, cmpl_entry);
	if (SAL_AND
	    (HIGGS_IO_SUCC != io_err_type, HIGGS_IO_UNDERFLOW != io_err_type))
		higgs_print_error_info(sal_msg, higgs_req, io_err_type);

	tm_io_err_type_check(io_err_type, sal_msg);
	if (HIGGS_IO_SUCC != io_err_type) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1,
			    "iptt: %d 0x%x come back, TM func %d, fail!",
			    higgs_req->iptt, higgs_req->iptt,
			    higgs_req->ctx.tm_function);
		return ERROR;
	}

	resp_iu = (struct sas_ssp_resp *)HIGGS_CMT_STATUS_SSP_RESP(cmd_tab);
	higgs_fill_task_rsp(sal_msg, resp_iu);

	if (higgs_req->sal_msg && higgs_req->sal_msg->done)
		higgs_req->sal_msg->done(higgs_req->sal_msg);
	else
		HIGGS_TRACE(OSP_DBL_MAJOR, 1,
			    "HiggsReqIPTT %d [0x%x]No SAL Msg!",
			    higgs_req->iptt, higgs_req->iptt);

	if (SAL_TM_FUNCTION_COMPLETE == sal_msg->status.io_resp
	    || SAL_TM_FUNCTION_SUCCEEDED == sal_msg->status.io_resp) {
		if (HIGGS_TM_ABORT_TASKSET == higgs_req->ctx.tm_function) {
			sal_dev = (struct sal_dev *)(higgs_req->ll_dev->up_dev);
			for (loop_num = 0; loop_num < HIGGS_MAX_REQ_NUM;
			     ++loop_num) {
				req_manager =
				    &higgs_req->higgs_card->higgs_req_manager;
				spin_lock_irqsave(&req_manager->req_lock, flag);
				higgs_org_req =
				    higgs_req->higgs_card->
				    running_req[loop_num];
				if (higgs_org_req
				    && (higgs_org_req->ctx.sas_addr ==
					sal_dev->sas_addr)) {
					HIGGS_TRACE(OSP_DBL_INFO, 1,
						    "ABTS cmd with iptt: %d success, manege iptt %d, event = HIGGS_REQ_EVENT_TM_OK",
						    higgs_req->iptt,
						    higgs_org_req->iptt);
					(void)higgs_req_state_chg(higgs_org_req,
								  HIGGS_REQ_EVENT_TM_OK,
								  false);
				}
				spin_unlock_irqrestore(&req_manager->req_lock,
						       flag);
			}

		} else {
			req_manager = &higgs_req->higgs_card->higgs_req_manager;
			spin_lock_irqsave(&req_manager->req_lock, flag);
			higgs_org_req =
			    higgs_get_req_by_iptt(higgs_req->higgs_card,
						  higgs_req->ctx.abort_tag);
			if (NULL != higgs_org_req) {
				HIGGS_TRACE(OSP_DBL_INFO, 1,
					    "TM cmd with iptt: %d success, manege iptt %d, event = HIGGS_REQ_EVENT_TM_OK",
					    higgs_req->iptt,
					    higgs_org_req->iptt);
				(void)higgs_req_state_chg(higgs_org_req,
							  HIGGS_REQ_EVENT_TM_OK,
							  false);
			}
			spin_unlock_irqrestore(&req_manager->req_lock, flag);
		}
	}

	(void)higgs_req_state_chg(higgs_req, HIGGS_REQ_EVENT_COMPLETE, true);
	return OK;
}

/*
 * notify sal to process with the error type of msg
 */
void higgs_handle_intr_msg_event(struct sal_msg *sal_msg,
				 enum sal_err_handle_action event_opcode)
{
	unsigned long flag = 0;

	HIGGS_ASSERT(NULL != sal_msg, return);
	HIGGS_ASSERT(NULL != sal_msg->dev, return);

	if (sal_msg->eh.event_retries > SAL_MAX_ERRHANDE_RETRIES_TIME) {
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, 10 * HZ, 2000,
				     "Card %d device 0x%llx msg %p cdb 0x%x retries %d times",
				     sal_msg->card->card_no,
				     sal_msg->dev->sas_addr, sal_msg,
				     sal_msg->ini_cmnd.cdb[0],
				     sal_msg->eh.event_retries);
		return;
	}

	spin_lock_irqsave(&sal_msg->msg_state_lock, flag);
	if (SAL_MSG_ST_SEND == sal_msg->msg_state) {
		if (OK ==
		    sal_msg_state_chg_no_lock(sal_msg,
					      SAL_MSG_EVENT_NEED_ERR_HANDLE)) {
			HIGGS_TRACE(OSP_DBL_INFO, 4749,
				    "Card:%d add msg:%p uni id:0x%llx,ST:%d,Tag:%d to err handler event code:%d",
				    sal_msg->card->card_no, sal_msg,
				    sal_msg->ini_cmnd.unique_id,
				    sal_msg->msg_state, sal_msg->ll_tag,
				    event_opcode);
			sal_msg->eh.event_retries++;
			(void)sal_add_msg_to_err_handler(sal_msg,
							 event_opcode,
							 sal_msg->send_time);
		}
	} else
		HIGGS_TRACE(OSP_DBL_MAJOR, 4749,
			    "Card:%d msg:%p uni id:0x%llx,ST:%d,Tag:%d is abnormal",
			    sal_msg->card->card_no, sal_msg,
			    sal_msg->ini_cmnd.unique_id, sal_msg->msg_state,
			    sal_msg->ll_tag);

	spin_unlock_irqrestore(&sal_msg->msg_state_lock, flag);

	return;
}

/* 
 * get sas addr from sas addr
 */
static s32 higgs_from_sas_addr_get_phy_id(struct sal_msg *sal_msg,
					  struct higgs_req *higgs_req)
{
	struct higgs_card *ll_card = NULL;
	u32 phy_id = 0;
	u32 start_phy = 0;
	u32 card_id = 0;
	u32 max_phy = 0;
	u64 sas_addr = sal_msg->dev->sas_addr;
	u64 tmp_sas_addr = 0;
	struct sas_identify_frame iden_frame;
	struct sas_identify_frame *iden_frame_ptr;
	ll_card = higgs_req->higgs_card;
	card_id = ll_card->card_id;

	max_phy = HIGGS_MAX_PHY_NUM;
	start_phy = 0;

	for (phy_id = start_phy; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		memcpy(&iden_frame,
		       &(ll_card->phy[phy_id].up_ll_phy->remote_identify),
		       sizeof(struct sas_identify));
		iden_frame_ptr = &iden_frame;
		tmp_sas_addr = DISC_GET_SASADDR_FROM_IDENTIFY(iden_frame_ptr);
		if (sas_addr == tmp_sas_addr)
			return phy_id;
	}

	return ERROR;
}


/*
 * parse error info and ssp response
 */
static s32 higgs_ssp_resp(struct sal_msg *sal_msg, struct higgs_req *higgs_req,
			  u32 cmpl_entry)
{
	void *cmd_table = NULL;
	struct sas_ssp_resp *resp_iu = NULL;
	enum higgs_io_err_type_e io_err_type = HIGGS_IO_ERROR_TYPE_BUTT;
	bool need_done_sal = true;	
	u32 iptt = 0xFFFFFFFF;

#if defined(PV660_ARM_SERVER)
	u32 phy_id = 0;
	struct higgs_led_event_notify led_event;

#endif

	iptt = higgs_req->iptt;

	cmd_table = higgs_req->cmd_table;
	if (NULL == cmd_table) {
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_MAJOR, 3 * HZ, 1,
				     "Request:0x%02x iptt %d  0x%x cmd table is null",
				     higgs_req->cmd, iptt, cmpl_entry);
		return ERROR;
	}
	resp_iu = (struct sas_ssp_resp *)HIGGS_CMT_STATUS_SSP_RESP(cmd_table);

	if (HIGGS_CQ_RSPNS_XFRD(&cmpl_entry)) {
		higgs_get_result_by_ssp_rsp_iu(sal_msg, resp_iu);
		if (sal_msg->status.io_resp != SAM_STAT_GOOD) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 1645,
				    "SSP response IU IO fail: sal msg %p ll devid %d iptt %d eIOResp Code %d",
				    sal_msg, higgs_req->ll_dev->dev_id,
				    higgs_req->iptt, sal_msg->status.io_resp);
			goto DONE;
		}
	}

	io_err_type = higgs_get_error_detail(cmd_table, higgs_req, cmpl_entry);
	if (SAL_AND
	    (HIGGS_IO_SUCC != io_err_type, HIGGS_IO_UNDERFLOW != io_err_type)) {
		higgs_print_error_info(sal_msg, higgs_req, io_err_type);
	}

	switch (io_err_type) {
	case HIGGS_IO_SUCC:
		/*IO is ok in chip*/
		break;
	case HIGGS_IO_UNDERFLOW:
		sal_msg->status.drv_resp = SAL_MSG_DRV_UNDERFLOW;	
		break;
		/*error code of DW0*/
	case HIGGS_IO_DIF_ERROR:
	case HIGGS_IO_UNEXP_XFER_RDY:
	case HIGGS_IO_XFER_RDY_OFFSET:
	case HIGGS_IO_XFR_RDY_LENGTH_OVERFLOW_ERR:
	case HIGGS_IO_RX_DIF_CRC_ERR:
	case HIGGS_IO_RX_DIF_APP_ERR:
	case HIGGS_IO_RX_DIF_RPP_ERR:
	case HIGGS_IO_RX_DATA_OFFSET_ERR:
	case HIGGS_IO_RX_UNEXP_RX_DATA_ERR:
	case HIGGS_IO_RX_DATA_OVERFLOW_ERR:
		/*error code of DW1*/
	case HIGGS_IO_PHY_NOT_RDY:
	case HIGGS_IO_OPEN_REJCT_WRONG_DEST_ERR:
	case HIGGS_IO_OPEN_REJCT_BY_OTHER_ERR:
	case HIGGS_IO_OPEN_REJCT_AIP_TIMEOUT_ERR:
	case HIGGS_IO_OPEN_REJCT_STP_BUSY_ERR:
	case HIGGS_IO_BREAK_RECEIVE_ERR:
	case HIGGS_IO_OPEN_TIMEOUT_ERR:
	case HIGGS_IO_OPEN_REJCT_NO_DEST_ERR:
	case HIGGS_IO_TX_BREAK_TIMEOUT_ERR:
	case HIGGS_IO_TX_BREAK_REQUEST_ERR:
	case HIGGS_IO_TX_BREAK_RECEIVE_ERR:
	case HIGGS_IO_TX_CLOSE_NORMAL_ERR:
	case HIGGS_IO_TX_CLOSE_PHYRESET_ERR:
	case HIGGS_IO_TX_WITH_CLOSE_DWS_TIMEOUT_ERR:
	case HIGGS_IO_TX_WITH_CLOSE_COMINIT_ERR:
	case HIGGS_IO_TX_NAK_RECEIVE_ERR:
	case HIGGS_IO_TX_ACK_NAK_TIMEOUT_ERR:
	case HIGGS_IO_TX_CREDIT_TIMEOUT_ERR:
		/*error code of DW2*/
	case HIGGS_IO_RX_FRAME_CRC_ERR:
	case HIGGS_IO_RX_FRAME_DONE_ERR:
	case HIGGS_IO_RX_FRAME_ERRPRM_ERR:
	case HIGGS_IO_RX_FRAME_NO_CREDIT_ERR:
	case HIGGS_IO_RX_FRAME_OVERRUN_ERR:
	case HIGGS_IO_RX_FRAME_NO_EOF_ERR:
	case HIGGS_IO_LINK_BUF_OVERRUN_ERR:
	case HIGGS_IO_RX_BREAK_TIMEOUT_ERR:
	case HIGGS_IO_RX_BREAK_REQUEST_ERR:
	case HIGGS_IO_RX_BREAK_RECEIVE_ERR:
	case HIGGS_IO_RX_CLOSE_NORMAL_ERR:
	case HIGGS_IO_RX_CLOSE_PHYRESET_ERR:
	case HIGGS_IO_RX_XRDY_ZERO_ERR:
	case HIGGS_IO_RX_SSP_FRAME_LEN_ERR:
	case HIGGS_IO_RX_NO_BALANCE_ERR:
	case HIGGS_IO_RX_WITH_CLOSE_DWS_TIMEOUT_ERR:
	case HIGGS_IO_RX_WITH_CLOSE_COMINIT_ERR:
		need_done_sal = false;
		/*Notify SAL to process error: abort retry */
		higgs_handle_intr_msg_event(higgs_req->sal_msg,
					    SAL_ERR_HANDLE_ACTION_ABORT_RETRY);
		break;
	case HIGGS_IO_OPEN_RETRY_ERR:
		need_done_sal = false;
		/*Notify SAL to process error: hardreset retry */
		higgs_handle_intr_msg_event(higgs_req->sal_msg,
					    SAL_ERR_HANDLE_ACTION_HDRESET_RETRY);
		break;
		/* Port Not Ready */
	case HIGGS_IO_PORT_NOT_READY_ERR:
		sal_msg->status.drv_resp = SAL_MSG_DRV_RETRY;
		break;
	case HIGGS_IO_BUS_ERROR:
	case HIGGS_IO_OVERFLOW:
	case HIGGS_IO_RX_BUFFER_ECC_ERR:
	case HIGGS_IO_RX_RESP_BUFFER_OVERFLOW_ERR:
	case HIGGS_IO_RX_AXI_BUS_ERR:
	case HIGGS_IO_RX_DATA_SGL_OVERFLOW_ERR:
	case HIGGS_IO_RX_DIF_SGL_OVERFLOW_ERR:
	case HIGGS_IO_RX_UNEXP_RETRANS_RESP_ERR:
	case HIGGS_IO_OPEN_REJCT_ZONE_VIOLATION_ERR:
	case HIGGS_IO_OPEN_REJCT_PROTOCOL_NOT_SUPPORT_ERR:
	case HIGGS_IO_OPEN_REJCT_RATE_NOT_SUPPORT_ERR:
	case HIGGS_IO_OPEN_REJCT_BAD_DEST_ERR:
	case HIGGS_IO_LOW_PHY_POWER_ERR:
	case HIGGS_IO_OPEN_REJCT_PATHWAY_BLOCKED_ERR:
	case HIGGS_IO_TX_CLOSE_TIMEOUT_ERR:
	case HIGGS_IO_IPTT_CONFLICT_ERR:
	case HIGGS_IO_TXFRM_TYPE_ERR:
	case HIGGS_IO_TXSMP_LENGTH_ERR:
	case HIGGS_IO_RX_CLOSE_TIMEOUT_ERR:
	case HIGGS_IO_DATA_LENGTH0_ERR:
	case HIGGS_IO_BAD_HASH_ERR:
	case HIGGS_IO_RX_BAD_FRAME_TYPE_ERR:
	case HIGGS_IO_RX_SMP_FRAME_LEN_ERR:
	case HIGGS_IO_RX_SMP_RESP_TIMEOUT_ERR:
	case HIGGS_IO_ERROR_TYPE_BUTT:
	case HIGGS_IO_OTHERS_ERR:
	case HIGGS_IO_CMD_NOT_CMPLETE:	/*CQ.cmd_cmplt = 0*/
	case HIGGS_IO_NO_RESPONSE:	/*CQ.rspns_xfrd = 0*/
	default:
		sal_msg->status.drv_resp = SAL_MSG_DRV_FAILED;	/*IO failed in chip*/
		sal_msg->status.io_resp = SAM_STAT_CHECK_CONDITION;
		break;
	}

DONE:
	if (SAL_AND(need_done_sal, SAL_AND(sal_msg, sal_msg->done))) {
#if defined(PV660_ARM_SERVER)
		/*stop discover when CDB equal to 0x0*/
		if (sal_msg->ini_cmnd.cdb[0] == 0x00) {
			phy_id =
			    higgs_from_sas_addr_get_phy_id(sal_msg, higgs_req);
			if (ERROR != phy_id) {
				higgs_req->higgs_card->phy[phy_id].phy_is_idle =
				    TRUE;
				led_event.phy_id = phy_id;
				led_event.event_val = LED_DISK_PRESENT;
				higgs_oper_led_by_event(higgs_req->higgs_card,
							&led_event);

				higgs_check_dma_txrx(((phy_id & 0xff) << 16) |
						     (higgs_req->higgs_card->
						      card_id));
			}
		}
#endif
		sal_msg->done(sal_msg);
	} else {
		/*sal msg has no done function*/
		HIGGS_TRACE(OSP_DBL_MAJOR, 1,
			    "HiggsReqIPTT %d [0x%x]Not Done To sal SAL, bNeedDoneSal %d sal_msg %p",
			    iptt, iptt, need_done_sal, sal_msg);
	}

	if (SAL_OR
	    (HIGGS_IO_SUCC == io_err_type, HIGGS_IO_UNDERFLOW == io_err_type)) {
		(void)higgs_req_state_chg(higgs_req, HIGGS_REQ_EVENT_COMPLETE,
					  true);
		return OK;
	}

	HIGGS_TRACE(OSP_DBL_INFO, 1,
		    "req with HiggsReqIPTT %d failed, reason:%d change to suspect!",
		    iptt, io_err_type);
	(void)higgs_req_state_chg(higgs_req, HIGGS_REQ_EVENT_HALFDONE, true);

	return OK;
}

/*
 * fill the smp dq
 */
enum sal_cmnd_exec_result higgs_prepare_smp(struct higgs_req *higgs_req)
{
	u32 smp_req_len = 0;
	struct sal_msg_smp *smp_iu = NULL;
	struct sal_msg *sal_msg;
	struct higgs_dq_info *dq_info;
	struct higgs_cmd_table *cmd_tab;

	HIGGS_ASSERT(NULL != higgs_req, return SAL_CMND_EXEC_FAILED);
	sal_msg = higgs_req->sal_msg;
	dq_info = higgs_req->dq_info;
	cmd_tab = higgs_req->cmd_table;

	HIGGS_ASSERT(NULL != dq_info, return SAL_CMND_EXEC_FAILED);
	HIGGS_ASSERT(NULL != cmd_tab, return SAL_CMND_EXEC_FAILED);

	/*the command header begin */
	HIGGS_CHF_CMD(dq_info, SMP_PROTOCOL_CMD);	/*smp protocol */
	HIGGS_CHF_PRI(dq_info, 1);
	HIGGS_CHF_PORT(dq_info, GET_PORT_ID_BY_MSG(sal_msg));	/*which port issue */
	/*there we don't care the force phy */
	HIGGS_CHF_FORCE_PHY(dq_info, 0);
	HIGGS_CHF_PHY_ID(dq_info, 0x1);
	HIGGS_CHF_DEVICE_ID(dq_info, higgs_req->ll_dev->dev_id);	/*divice id */
	HIGGS_CHF_RSP_REPO(dq_info, 1);	/*Response frame write to host memory */
	HIGGS_CHF_FIRST_BURST(dq_info, 0);	/*disable first burst */
	HIGGS_CHF_TRAN_RETRY(dq_info, 0);	/*disable transport layer retry */
	HIGGS_CHF_MAX_RESP_LEN(dq_info, HIGGS_MAX_RESPONSE_LENGTH >> 2);	/*MAX Dword */

	smp_req_len = sal_msg->protocol.smp.smp_len;	/*get the request length */
	HIGGS_CHF_CMMF_LEN(dq_info, ((smp_req_len + 3) / (sizeof(u32))) & 0x1FF);	/*8 bit */

	HIGGS_CHF_IPTT(dq_info, higgs_req->iptt);	/*iptt */
	higgs_dq_global_config(dq_info);	/*global config */
	/*the command header end */

	/*command table begin */
	/*copy the request to command table */
	smp_iu = (struct sal_msg_smp *)HIGGS_CMT_SMP_TABLE(cmd_tab);
	(void)memcpy((void *)smp_iu, &sal_msg->protocol.smp,
		     sizeof(struct sal_msg_smp));
	/*command table end */

	return SAL_CMND_EXEC_SUCCESS;
}

/*
 * fill the smp dq
 */
enum sal_cmnd_exec_result higgs_send_smp(struct higgs_req *higgs_req)
{
	struct higgs_card *ll_card;

	HIGGS_ASSERT(NULL != higgs_req, return SAL_CMND_EXEC_FAILED);

	ll_card = higgs_req->higgs_card;
	higgs_send_command_hw(ll_card, higgs_req);

	return SAL_CMND_EXEC_SUCCESS;
}

/*
 * fill SAL return value with IU of driver when the IO success
 */
void higgs_fill_result_by_smp_rsp_iu(struct sal_msg *sal_msg,
				     struct disc_smp_response *resp_iu)
{
	u32 resp_len = 0;
	bool ret = false;
	if (SMP_FUNCTION_ACCEPTED == resp_iu->smp_result) {
		sal_msg->status.drv_resp = SAL_MSG_DRV_SUCCESS;
		sal_msg->status.io_resp = SAM_STAT_GOOD;
		if (NULL == sal_msg->protocol.smp.rsp_virt_addr) {
			HIGGS_TRACE(OSP_DBL_MINOR, 4533,
				    "Sal msg %p rsp_virt_addr is NULL!",
				    sal_msg);
			return;
		} else {
			ret =
			    higgs_check_smp_resp_length(sal_msg,
							((u8 *) resp_iu)[3]);
			if (ret) {
				resp_len = sizeof(u32);
				resp_len += (((u8 *) resp_iu)[3]) * sizeof(u32);
			} else {
				HIGGS_TRACE_FRQLIMIT(OSP_DBL_MINOR, 2 * HZ,
						     4533, "smp frame Resp %d",
						     ((u8 *) resp_iu)[3]);
				higgs_hex_dump("smp resp is :", (u8 *) resp_iu,
					       16, 1);
				sal_msg->status.drv_resp = SAL_MSG_DRV_FAILED;	
			}

			resp_len = MIN(resp_len, sal_msg->protocol.smp.rsp_len);
			HIGGS_TRACE(OSP_DBL_MINOR, 4533,
				    "Copy the %d byte data to buffer %p",
				    resp_len,
				    sal_msg->protocol.smp.rsp_virt_addr);
			memcpy(sal_msg->protocol.smp.rsp_virt_addr, resp_iu,
			       resp_len);
		}
	} else
		HIGGS_TRACE(OSP_DBL_MINOR, 4533, "Smp Func 0x%x ErrorCode %d",
			    resp_iu->smp_function, resp_iu->smp_result);
}

/*
 * get smp from cq and process
 */
s32 higgs_process_smp_rsp(struct higgs_req *higgs_req)
{
	void *cmd_tab = NULL;
	struct disc_smp_response *resp_iu = NULL;
	enum higgs_io_err_type_e io_err_type = HIGGS_IO_ERROR_TYPE_BUTT;
	u32 cmpl_entry = higgs_req->cmpl_entry;
	struct sal_msg *sal_msg = higgs_req->sal_msg;

	if (NULL == sal_msg) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1123,
			    "Request:0x%02x iptt %d 0x%x, no sal msg.",
			    higgs_req->cmd, higgs_req->iptt, cmpl_entry);
		return ERROR;
	}

	cmd_tab = higgs_req->cmd_table;
	if (NULL == cmd_tab) {
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_MAJOR, 3 * HZ, 1,
				     "Request[0x%02x] iptt %d [0x%x] cmd table is null.",
				     higgs_req->cmd, higgs_req->iptt,
				     cmpl_entry);
		return ERROR;
	}
	resp_iu =
	    (struct disc_smp_response *)HIGGS_CMT_STATUS_SSP_RESP(cmd_tab);

	io_err_type = higgs_get_error_detail(cmd_tab, higgs_req, cmpl_entry);
	if (io_err_type != HIGGS_IO_SUCC) {
	    HIGGS_TRACE(OSP_DBL_MINOR, 1645,
			    "SMP IO fail: sal msg %p ll devid %d iptt %d eIOErrorType %d",
			    sal_msg, higgs_req->ll_dev->dev_id, higgs_req->iptt,
			    io_err_type);
	}

	if (sal_simulate_io_fatal == 105) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1,
			    "simu smp result sal_simulate_io_fatal %d",
			    sal_simulate_io_fatal);
		resp_iu =
		    (struct disc_smp_response *)
		    HIGGS_CMT_STATUS_SSP_RESP(cmd_tab);
		((u8 *) resp_iu)[3] = 0x0;	
		sal_simulate_io_fatal = 0;	
	}
	
	if (HIGGS_IO_SUCC == io_err_type) {
		/*io success in chip, copy smp iu to SAL msg*/
		higgs_fill_result_by_smp_rsp_iu(sal_msg, resp_iu);
	} else {
		sal_msg->status.drv_resp = SAL_MSG_DRV_FAILED;	/*fail in chip*/
		sal_msg->status.io_resp = SAM_STAT_CHECK_CONDITION;
	}

	if (higgs_req->sal_msg && higgs_req->sal_msg->done)
		/*SAL_TMDone   sal_ssp_done    sal_smp_done*/
		higgs_req->sal_msg->done(higgs_req->sal_msg);
	else
		HIGGS_TRACE(OSP_DBL_MAJOR, 1235,
			    "smp HiggsReqIPTT %d [0x%x]No SAL Msg!",
			    higgs_req->iptt, higgs_req->iptt);

	if (HIGGS_IO_SUCC == io_err_type)
		(void)higgs_req_state_chg(higgs_req, HIGGS_REQ_EVENT_COMPLETE, true);
	else
		(void)higgs_req_state_chg(higgs_req, HIGGS_REQ_EVENT_HALFDONE, true);

	return OK;
}


/*
 * handle ssp response
 */
s32 higgs_process_ssp_rsp(struct higgs_req * higgs_req)
{
	u32 cmpl_entry = higgs_req->cmpl_entry;
	u32 hgc_invld_dqe_info_reg = 0;
	struct sal_msg *sal_msg = higgs_req->sal_msg;

	if (NULL == sal_msg) {
		/*sal msg was processed already*/
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, 1 * HZ, 1,
				     "SAL Msg is NULL, DEV[%d]iptt[%d]",
				     higgs_req->ll_dev->dev_id,
				     higgs_req->iptt);
		return OK;
	}

	if (HIGGS_CQ_CFG_ERR(&cmpl_entry)) {
		/* DQ config error, return*/
		static unsigned long last = 0;

		hgc_invld_dqe_info_reg =
		    HIGGS_GLOBAL_REG_READ(higgs_req->higgs_card,
					  HISAS30HV100_GLOBAL_REG_HGC_INVLD_DQE_INFO_REG);

		if (hgc_invld_dqe_info_reg & 0x800000) {	
		}

		if (time_after_eq(jiffies, last + (60 * HZ))) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 1,
				    "CMD[0x%02x]DEV[%d]iptt[0x%x]DQ CFG Error!",
				    higgs_req->cmd, higgs_req->ll_dev->dev_id,
				    higgs_req->iptt);
			sal_msg->status.drv_resp = SAL_MSG_DRV_FAILED;
			higgs_hex_dump("invalid ssp dq",
				       (u8 *) (higgs_req->dq_info),
				       sizeof(struct higgs_dq_info),
				       sizeof(u32));
			higgs_hex_dump("ssp cmd table",
				       (u8
					*) (&(((struct higgs_cmd_table *)
					       higgs_req->cmd_table)->table)),
				       64, 1);
			HIGGS_TRACE(OSP_DBL_MAJOR, 1,
				    "Req iptt is %d,Delivery Queue Entry Error, HGC_INVLD_DQE_INFO_REG[0x%x]",
				    higgs_req->iptt, hgc_invld_dqe_info_reg);
			last = jiffies;
		}
	}

	if (HIGGS_REQ_IS_TM(higgs_req))
		return higgs_tm_response(sal_msg, higgs_req, cmpl_entry);
	else
		return higgs_ssp_resp(sal_msg, higgs_req, cmpl_entry);
}

/*
 * callback to process IO response
 */
void higgs_req_callback(struct higgs_req *higgs_req, u32 cmpl_entry)
{
	if (sal_simulate_io_fatal == 1) {
		sal_simulate_io_fatal = 0;
		HIGGS_TRACE(OSP_DBL_MAJOR, 1, "set io iptt %d tmo",
			    higgs_req->iptt);
		return;
	} else if (sal_simulate_io_fatal == 102) {
		sal_simulate_io_fatal = 103;
		HIGGS_TRACE(OSP_DBL_MAJOR, 1,
			    "set io iptt %d tmo, sal_simulate_io_fatal %d",
			    higgs_req->iptt, sal_simulate_io_fatal);
		return;
	}

	higgs_req->cmpl_entry = cmpl_entry;
	if (NULL == higgs_req->ll_dev) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1, "HiggsReqIPTT[0x%x]Dev is NULL!",
			    higgs_req->iptt);
		return;
	}

	if (higgs_req->proto_dispatch->command_rsp)
		(void)higgs_req->proto_dispatch->command_rsp(higgs_req);

	return;
}

enum sal_cmnd_exec_result higgs_dispatch_higgs_req(struct higgs_card *ll_card,
						   struct higgs_prot_dispatch
						   *dispatch,
						   struct higgs_req *higgs_req)
{
	enum sal_cmnd_exec_result ret = SAL_CMND_EXEC_FAILED;

	if (NULL != dispatch->command_prepare) {
		/* higgs_prepare_ssp , higgs_prepare_smp*/
		ret =
		    (enum sal_cmnd_exec_result)dispatch->
		    command_prepare(higgs_req);
		if (SAL_CMND_EXEC_SUCCESS != ret) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 1, "Card[%d]Prepare failed!",
				    ll_card->card_id);
			return ret;
		}
	}
	/*fill command table and update write pointer */
	if (NULL != dispatch->command_send) {
		/* higgs_send_ssp , higgs_send_smp*/
		ret = dispatch->command_send(higgs_req);
		if (SAL_CMND_EXEC_SUCCESS != ret) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 1, "Card[%d]Send failed!",
				    ll_card->card_id);
			return ret;
		}
	}

	return ret;
}

/*
 * ssp,smp request send interface
 */
enum sal_cmnd_exec_result higgs_send_msg(struct sal_msg *msg)
{
	enum sal_cmnd_exec_result ret = SAL_CMND_EXEC_FAILED;
	struct higgs_req *higgs_req = NULL;
	struct higgs_card *higgs_card = NULL;
	struct higgs_device *higgs_dev = NULL;
	struct higgs_prot_dispatch *dispatch = NULL;
	OSP_DMA command_table_dma;
	struct higgs_cmd_table *cmd_tab = NULL;
	unsigned long flag = 0;
	u32 dq_id = 0;

	HIGGS_ASSERT(NULL != msg, return SAL_CMND_EXEC_FAILED);
	higgs_card = GET_HIGGS_CARD_BY_MSG(msg);
	higgs_dev = GET_HIGGS_DEV_BY_MSG(msg);
	HIGGS_ASSERT(NULL != higgs_card, return SAL_CMND_EXEC_FAILED);
	HIGGS_ASSERT(NULL != higgs_dev, return SAL_CMND_EXEC_FAILED);

	if (msg->type >= HIGGS_PROT_BUTT) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1,
			    "Card[%d]SAL msg Protocol[%d] Error!",
			    higgs_card->card_id, msg->type);
		ret = SAL_CMND_EXEC_FAILED;
		goto fail;
	}
	dispatch = &higgs_dispatcher.prot_dispatcher[msg->type];
	higgs_req = higgs_get_higgs_req(higgs_card, INTER_CMD_TYPE_NONE);
	if (NULL == higgs_req) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1, "Card[%d]Get Higgs Req Failed!",
			    higgs_card->card_id);
		ret = SAL_CMND_EXEC_BUSY;
		goto fail;
	}

	cmd_tab =
	    higgs_get_command_table(higgs_card, higgs_req->iptt,
				    &command_table_dma);
	if (NULL == cmd_tab) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1,
			    "iptt[0x%x] CommandTable Get Error!",
			    higgs_req->iptt);
		(void)higgs_req_state_chg(higgs_req, HIGGS_REQ_EVENT_COMPLETE,
					  true);
		higgs_req = NULL;
		ret = SAL_CMND_EXEC_BUSY;
		goto fail;
	}

	dq_id = higgs_get_dqid_from_devid(higgs_card, higgs_dev->dev_id);
	/*parepare the dq and sge list      *NOTE: the parepare and send must be lock! */
	spin_lock_irqsave(HIGGS_DQ_LOCK(higgs_card, dq_id), flag);
	/*must be set, the prepare and send interface will use these */
	higgs_req->queue_id = dq_id;
	higgs_req->cmd_table = cmd_tab;
	higgs_req->cmd_table_dma = command_table_dma;
	/*init higgs req */
	higgs_req->higgs_card = higgs_card;
	higgs_req->ll_dev = higgs_dev;
	higgs_req->proto_dispatch = dispatch;
	higgs_req->sal_msg = msg;
	higgs_req->completion = higgs_req_callback;

	higgs_req->dq_info = higgs_get_dq_info(higgs_card, dq_id);
	if (NULL == higgs_req->dq_info) {
		HIGGS_TRACE_FRQLIMIT(DRV_LOG_INFO, 2 * HZ, 1,
				     "ITPP[0x%x] Get DQ fail!",
				     higgs_req->iptt);
		spin_unlock_irqrestore(HIGGS_DQ_LOCK(higgs_card, dq_id), flag);
		ret = SAL_CMND_EXEC_BUSY;
		goto fail;
	}

	/*put iptt in sal msg */
	msg->ll_tag = (u16) higgs_req->iptt;
	ret = higgs_dispatch_higgs_req(higgs_card, dispatch, higgs_req);
	spin_unlock_irqrestore(HIGGS_DQ_LOCK(higgs_card, dq_id), flag);
	if (SAL_CMND_EXEC_SUCCESS != ret)
		goto fail;

	return ret;
    
fail:
	if (higgs_req) {
		(void)higgs_req_state_chg(higgs_req, HIGGS_REQ_EVENT_COMPLETE,
					  true);
		higgs_req = NULL;
		cmd_tab = NULL;
	}
	return ret;
}

/* Q index add one */
inline void higgs_qindex_plus(u32 * index, u32 qsize)
{
	(*index)++;
	if ((*index) >= qsize)
		(*index) = 0;
}

/*funtion to handle oq interrupt*/
void higgs_oq_int_process(struct higgs_card *higgs_card, u32 queue_id)
{
	struct higgs_cqueue *cmpl_queue;
	struct higgs_cq_info *cmpl_entry;
	struct higgs_req *higgs_req = NULL;
	u32 cq_wptr, cq_rptr, cq_depth;
	u32 iptt = 0;
	u32 cq_entry_dword = 0;
	bool info_dumped = false;
	u32 dump_flag = 0;
	bool need_chip_reset = false;
	u64 all_size_cq = 0;
	u64 all_size_dq = 0;

	HIGGS_ASSERT(queue_id < HIGGS_MAX_CQ_NUM, return);

	cmpl_queue = &higgs_card->io_hw_struct_info.complete_queue[queue_id];
	cq_rptr = cmpl_queue->rpointer;
	cq_wptr = HIGGS_CQ_WPT_GET(higgs_card, queue_id);

	HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, 60 * HZ, 1,
			     "Card:%d, CQ %d, uiCqWptr %d, RPreg %d, uiCqRptr %d",
			     higgs_card->card_id, queue_id, cq_wptr,
			     HIGGS_CQ_RPT_GET(higgs_card, queue_id), cq_rptr);

	if (cq_wptr == cq_rptr)
		return;

	cq_depth = cmpl_queue->cq_depth;
	if (cq_wptr != (cq_wptr % cq_depth)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1, "Strange CQ PI %d, QueueDepth %d",
			    cq_wptr, cmpl_queue->cq_depth);
		cq_wptr = (cq_wptr % cq_depth);
	}

	for (; cq_wptr != cq_rptr; higgs_qindex_plus(&cq_rptr, cq_depth)) {
		cmpl_entry = &cmpl_queue->queue_base[cq_rptr];
		cq_entry_dword = HIGGS_CQ_DW(cmpl_entry);
		iptt = HIGGS_CQ_IPTT(cmpl_entry);

		HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, 60 * HZ, 1,
				     "Card:%d, CQ %d, CmplEntry 0x%08x, iptt %d",
				     higgs_card->card_id, queue_id,
				     cq_entry_dword, iptt);

		if (HIGGS_CQ_ATTENTION(cmpl_entry))
			HIGGS_TRACE_FRQLIMIT(OSP_DBL_MAJOR, 1, 10 * HZ,
					     "CmplEntry 0x%x attention Bit Set!",
					     cq_entry_dword);

		if (unlikely(iptt >= higgs_card->higgs_can_io)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 1,
				    "Card:%d, CQ %d, uiCqWptr %d, uiCqRptr %d, CmplEntry 0x%08x, iptt %d >= MAX %d",
				    higgs_card->card_id, queue_id, cq_wptr,
				    cq_rptr, cq_entry_dword, iptt,
				    higgs_card->higgs_can_io);
			if ((++higgs_card->ilegal_iptt_cnt) >=
			    HIGGS_ILLEGAL_IPTT_THRESHOLD)
				need_chip_reset = true;

			if (!info_dumped) {
				(void)higgs_dump_info(higgs_card->sal_card,
						      &dump_flag);
				info_dumped = true;
			}
			continue;
		}

		higgs_req = higgs_card->running_req[iptt];
		if (NULL == higgs_req) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 1,
				    "Req in[0x%x]is NULL, iptt %d CQ: %d uiCqWptr:%d, RPreg:%d, uiCqRptr %d",
				    cq_entry_dword, iptt, queue_id, cq_wptr,
				    HIGGS_CQ_RPT_GET(higgs_card, queue_id),
				    cq_rptr);

			all_size_cq =
			    (u64) (higgs_card->card_cfg.
				   obqueue_num_elements)
			    * (u64) HIGGS_CQ_ENTRY_SIZE * (u64) 4;	/* 4 */
			all_size_dq =
			    (u64) (higgs_card->card_cfg.
				   ibqueue_num_elements)
			    * (u64) HIGGS_DQ_ENTRY_SIZE * (u64) 4;
			if (printed_dq_cq == false) {
				printed_dq_cq = true;
				/* DQ */
				higgs_hex_dump("All DQ",
					       higgs_card->io_hw_struct_info.
					       all_dq_base, (u32) all_size_dq,
					       4);
				/* CQ */
				higgs_hex_dump("All CQ",
					       higgs_card->io_hw_struct_info.
					       all_cq_base, (u32) all_size_cq,
					       4);
			}

			continue;
		}

		if (higgs_req->iptt != iptt) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 1,
				    "The pstReq iptt[0x%x]is not equal to cmpl[0x%x]",
				    higgs_req->iptt, iptt);
			continue;
		}

		if (higgs_req_callback != higgs_req->completion) {
			HIGGS_TRACE(OSP_DBL_INFO, 1,
				    "iptt %d completion %p, higgs_req_callback %p",
				    higgs_req->iptt, higgs_req->completion,
				    higgs_req_callback);
			continue;
		}

		HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, 60 * HZ, 1,
				     "pstHiggsReq %p, iptt %d call func %p",
				     higgs_req,
				     higgs_req->iptt, higgs_req->completion);
		higgs_req->completion(higgs_req, cq_entry_dword);
		higgs_req->completion = NULL;
	}

	/* notify the chip when CQ finished */
	cmpl_queue->rpointer = cq_rptr;
	HIGGS_CQ_RPT_WRITE(higgs_card, queue_id, cq_rptr);

	if (unlikely(need_chip_reset)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1,
			    "Card:%d, illegal iptt count reach %d, to reset chip for recovery",
			    higgs_card->card_id, higgs_card->ilegal_iptt_cnt);
		(void)sal_add_card_to_err_handler(higgs_card->sal_card,
						  SAL_ERR_HANDLE_ACTION_RESET_HOST);
	}

	return;
}

/* prepare tm msg */
s32 higgs_prepare_tm_msg(struct higgs_req * higgs_req)
{
	struct sal_msg *sal_msg = higgs_req->sal_msg;
	struct higgs_dq_info *dq_info = higgs_req->dq_info;
	struct higgs_cmd_table *cmd_table = higgs_req->cmd_table;
	struct sas_ssp_tm_info_unit *task_iu = NULL;
	struct higgs_req_context *ctx = &higgs_req->ctx;

	HIGGS_ASSERT(NULL != dq_info, return SAL_CMND_EXEC_FAILED);
	HIGGS_ASSERT(NULL != cmd_table, return SAL_CMND_EXEC_FAILED);

	higgs_prepare_base_ssp(sal_msg, higgs_req);

	HIGGS_CHF_PRI(dq_info, 1);	/*higher priority */
	HIGGS_CHF_SSP_FRAME(dq_info, HIGGS_INI_FRAME_TYPE_TASK);	/*task */
	HIGGS_CHF_CMMF_LEN(dq_info, (((sizeof(struct sas_ssp_tm_info_unit) + sizeof(struct sas_ssp_frame_header) + 3) / sizeof(u32)) & 0x1FF));	/*9 bit */

	/*command table begin */
	task_iu =
	    (struct sas_ssp_tm_info_unit *)HIGGS_CMT_SSP_TASK_IU(cmd_table);
	(void)memcpy(&task_iu->lun[0], sal_msg->protocol.ssp.lun,
		     DEFAULT_LUN_SIZE);

	task_iu->mgmt_task_tag = HIGGS_SWAP_16(ctx->abort_tag);
	task_iu->mgmt_task_function = ctx->tm_function;

	return SAL_CMND_EXEC_SUCCESS;
}

/*
 * sal call it to send TM command
 */
s32 higgs_send_tm_msg(enum sal_tm_type sal_type, struct sal_msg * msg)
{
	struct higgs_req *higgs_req = NULL;
	struct higgs_card *higgs_card = NULL;
	struct higgs_device *higgs_dev = NULL;
	struct higgs_prot_dispatch *dispatch = NULL;
	enum higgs_tm_type tm_type = HIGGS_TM_TYPE_BUTT;
	u16 abort_tag = 0;
	enum sal_cmnd_exec_result ret;
	OSP_DMA command_table_dma;
	struct higgs_cmd_table *cmd_table = NULL;
	struct higgs_dq_info *dq_info = NULL;
	unsigned long flag = 0;
	u32 dq_id = 0;

	HIGGS_ASSERT(NULL != msg, return ERROR);
	higgs_card = GET_HIGGS_CARD_BY_MSG(msg);
	higgs_dev = GET_HIGGS_DEV_BY_MSG(msg);
	HIGGS_ASSERT(NULL != higgs_card, return ERROR);
	HIGGS_ASSERT(NULL != higgs_dev, return ERROR);

	if (SAL_DEV_TYPE_SSP != msg->dev->dev_type) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1, "DevType[%d]is not SSP!",
			    msg->dev->dev_type);
		return ERROR;
	}

	/* Get enTmType for SAL msg */
	tm_type = higgs_convert_sal_tm_type(sal_type);
	if (HIGGS_TM_TYPE_BUTT == tm_type) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4574, "the type is invalid:0x%x",
			    sal_type);
		return ERROR;
	}

	/* Tm command the type is SSP. */
	dispatch = &higgs_dispatcher.prot_dispatcher[msg->type];
	higgs_req = higgs_get_higgs_req(higgs_card, INTER_CMD_TYPE_TASK);
	if (NULL == higgs_req) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1, "Card[%d]Get Higgs Req Failed!",
			    higgs_card->card_id);
		return ERROR;
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

	/*The below code no need to spin lock !! */
	/*Get abort tag when TM type is ABORT/QUERY and return ok directly when abort tag is not found */
	if (HIGGS_TM_ABORT_TASK == tm_type || HIGGS_TM_QUERY_TASK == tm_type) {
		abort_tag =
		    (u16) higgs_look_up_iptt(higgs_card, msg->related_msg);
		if (abort_tag >= HIGGS_MAX_REQ_NUM) {
			msg->tag_not_found = true;
			msg->done(msg);
			(void)higgs_req_state_chg(higgs_req,
						  HIGGS_REQ_EVENT_COMPLETE,
						  true);
			return OK;
		}
		higgs_req->ctx.abort_tag = abort_tag;
	}

	/*parepare the dq and sge list      *NOTE: the parepare and send must be lock! */
	dq_id = higgs_get_dqid_from_devid(higgs_card, higgs_dev->dev_id);
	spin_lock_irqsave(HIGGS_DQ_LOCK(higgs_card, dq_id), flag);
	/*init higgs req */
	higgs_req->queue_id = dq_id;
	higgs_req->higgs_card = higgs_card;
	higgs_req->ll_dev = higgs_dev;
	higgs_req->proto_dispatch = dispatch;
	higgs_req->sal_msg = msg;
	higgs_req->completion = higgs_req_callback;

	dq_info = higgs_get_dq_info(higgs_card, dq_id);
	if (NULL == dq_info) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1,
			    "ITPP[0x%x] DQ Get Error!", higgs_req->iptt);
		goto cmnd_fail;
	}

	/*must be set, the prepare and send interface will use these */
	higgs_req->cmd_table = cmd_table;
	higgs_req->cmd_table_dma = command_table_dma;
	higgs_req->dq_info = dq_info;

	/*save iptt to SAL msg */
	msg->ll_tag = (u16) higgs_req->iptt;

	higgs_req->ctx.tm_function = tm_type;
	HIGGS_TRACE(OSP_DBL_INFO, 1,
		    "iptt %d sas addr %llx send TM, tm func %d, msg type %d, org iptt = %d",
		    higgs_req->iptt, msg->dev->sas_addr,
		    higgs_req->ctx.tm_function, msg->type,
		    higgs_req->ctx.abort_tag);

	(void)higgs_prepare_tm_msg(higgs_req);

	ret = (enum sal_cmnd_exec_result)dispatch->command_send(higgs_req);
	if (SAL_CMND_EXEC_SUCCESS != ret) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1, "Card[%d]Dev[%d]Send failed!",
			    higgs_card->card_id, higgs_dev->dev_id);
		goto cmnd_fail;
	}

	spin_unlock_irqrestore(HIGGS_DQ_LOCK(higgs_card, dq_id), flag);
	return OK;

      cmnd_fail:
	spin_unlock_irqrestore(HIGGS_DQ_LOCK(higgs_card, dq_id), flag);

      cmnd_table_fail:
	(void)higgs_req_state_chg(higgs_req, HIGGS_REQ_EVENT_COMPLETE, true);

	return ERROR;

}

/* register dispatcher used for IO process */
void higgs_register_dispatcher(void)
{
	struct higgs_prot_dispatch *protocol_handler =
	    higgs_dispatcher.prot_dispatcher;
	struct higgs_prot_dispatch *abort_handler =
	    &higgs_dispatcher.inter_abort_dispatch;

	protocol_handler[SAL_MSG_PROT_SSP].command_verify = NULL;
	protocol_handler[SAL_MSG_PROT_SSP].command_prepare = higgs_prepare_ssp;
	protocol_handler[SAL_MSG_PROT_SSP].command_send = higgs_send_ssp;
	protocol_handler[SAL_MSG_PROT_SSP].command_rsp = higgs_process_ssp_rsp;
	protocol_handler[SAL_MSG_PROT_SSP].command_error = NULL;

	protocol_handler[SAL_MSG_PROT_SMP].command_verify = NULL;
	protocol_handler[SAL_MSG_PROT_SMP].command_prepare = higgs_prepare_smp;
	protocol_handler[SAL_MSG_PROT_SMP].command_send = higgs_send_smp;
	protocol_handler[SAL_MSG_PROT_SMP].command_rsp = higgs_process_smp_rsp;
	protocol_handler[SAL_MSG_PROT_SMP].command_error = NULL;

	abort_handler->command_verify = NULL;
	abort_handler->command_prepare = higgs_prepare_abort;
	abort_handler->command_send = higgs_send_abort;
	abort_handler->command_rsp = higgs_process_rsp_abort;
	abort_handler->command_error = NULL;

	return;
}

/*
 * check whether response length in smp resp is correct.
 */
bool higgs_check_smp_resp_length(struct sal_msg * sal_msg, u8 smp_resp_length)
{
	bool ret = false;
	u8 smp_fuction;		/*byte1:SMP fuction */
	u8 resp_length;		/*byte2:Allocated Response Length */
	struct disc_smp_req *smp_req = NULL;
	smp_req = (struct disc_smp_req *)sal_msg->protocol.smp.smp_payload;

	smp_fuction = smp_req->smp_function;
	resp_length = smp_req->resp_len;

	switch (smp_fuction) {
		/* response lenth should be 00h, protocol limited*/
	case SMP_PHY_CONTROL:
	case SMP_CONFIG_ROUTING_INFO:
		if (0 == smp_resp_length)
			ret = true;
		break;
		/*response lenth and Allocated Response Length should be both 00h£¬
		  or response lenth and Allocated Response Length both no 00h */
	case SMP_REPORT_GENERAL:
	case SMP_DISCOVER_REQUEST:
	case SMP_REPORT_PHY_SATA:
		if ((0 == resp_length && 0 == smp_resp_length)
		    || (0 != resp_length && 0 != smp_resp_length))
			ret = true;
		break;
	default:
		if (0 != smp_resp_length)
			ret = true;
		break;

	}

	return ret;
}
