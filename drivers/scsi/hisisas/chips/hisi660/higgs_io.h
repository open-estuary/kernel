/*
 * copyright (C) huawei
 */
 
#ifndef _HIGGS_IO_H_
#define _HIGGS_IO_H_

#define GET_HIGGS_CARD_BY_MSG(sal_msg)    \
    ((struct higgs_card *)((sal_msg)->card->drv_data))
#define GET_HIGGS_CARD_BY_DEV(sal_dev)		\
	((struct higgs_card *)((sal_dev)->card->drv_data))
#define GET_HIGGS_DEV_BY_MSG(sal_msg) \
    ((struct higgs_device *)((sal_msg)->dev->ll_dev))
#define GET_SCSI_CMD_BY_MSG(sal_msg) \
    ((sal_msg)->protocol.ssp.cdb[0])
#define GET_PORT_ID_BY_MSG(sal_msg) \
    ((sal_msg)->dev->port->port_id)

#define GET_HIGGS_CARD_BY_HIGGS_REQ(higgs_req)    \
    ((struct higgs_card *)((higgs_req)->higgs_card))

#define GET_HIGGS_CARD_BY_SAL_CARD(sal_card) \
    ((struct higgs_card *)(sal_card->drv_data))

/*DQ Config*/
#define SAS_MODE_TARGET 0	/*controller is target mode */
#define SAS_MODE_INITIATOR 1
#define SAS_SSP_PSS_THROUGH_DISABLE 0	/*frame header is creaded by logic */
#define SAS_SSP_PSS_THROUGH_ENABLE 1
#define SAS_SGE_MODE 0
#define SAS_SGL_MODE 1
#define HIGGS_INI_NO_DATA        			(0x0)	/*no data */
#define HIGGS_INI_DATA_FROM_DEVICE         	(0x1)	/*target to initiator */
#define HIGGS_INI_DATA_TO_DEVICE         	(0x2)	/*initiator to target */
#define HIGGS_INI_FRAME_TYPE_TASK         	(0x3)	/*task */
    enum higgs_tm_type {
	HIGGS_TM_ABORT_TASK = 0x01,
	HIGGS_TM_ABORT_TASKSET = 0x02,
	HIGGS_TM_CLEAR_TASKSET = 0x04,
	HIGGS_TM_LUN_RESET = 0x08,
	HIGGS_TM_IT_NEXUS_RESET = 0x10,
	HIGGS_TM_CLEAR_ACA = 0x40,
	HIGGS_TM_QUERY_TASK = 0x80,
	HIGGS_TM_QUERY_TASK_SET = 0x81,
	HIGGS_TM_QUERY_UNIT_ATTENTION = 0x82,
	HIGGS_TM_TYPE_BUTT = 0xFF
};

#define HIGGS_REQ_IS_TM(higgs_req)	\
	((INTER_CMD_TYPE_TASK == (higgs_req)->ctx.cmd_type))

#define HIGGS_DQ_RPT_GET(card,id)	\
	HIGGS_GLOBAL_REG_READ((card), HISAS30HV100_GLOBAL_REG_DLVRY_QUEUE_RD_PTR_0_REG+(u64)(id)*20)
#define HIGGS_DQ_WPT_GET(card,id)	\
	HIGGS_GLOBAL_REG_READ((card), HISAS30HV100_GLOBAL_REG_DLVRY_QUEUE_WRT_PTR_0_REG+(u64)(id)*20)
#define HIGGS_DQ_WPT_WRITE(card,id,val)	\
	HIGGS_GLOBAL_REG_WRITE((card), HISAS30HV100_GLOBAL_REG_DLVRY_QUEUE_WRT_PTR_0_REG+(u64)(id)*20,val)

#define HIGGS_CQ_WPT_GET(card,id)	\
	HIGGS_GLOBAL_REG_READ((card), HISAS30HV100_GLOBAL_REG_CMPLTN_QUEUE_WRT_PTR_0_REG+(u64)(id)*20)
#define HIGGS_CQ_RPT_GET(card,id)	\
	HIGGS_GLOBAL_REG_READ((card), HISAS30HV100_GLOBAL_REG_CMPLTN_QUEUE_RD_PTR_0_REG +(u64)(id)*20)
#define HIGGS_CQ_RPT_WRITE(card,id,val)	\
	HIGGS_GLOBAL_REG_WRITE((card), HISAS30HV100_GLOBAL_REG_CMPLTN_QUEUE_RD_PTR_0_REG+(u64)(id)*20, val)

#define HIGGS_DQ_LOCK(card, dq_id)	(&((card)->dq_lock[(dq_id)]))

#define HIGGS_REQ_RECLAIM(req, higgs_card, iptt) \
	do {												\
        req->req_state = HIGGS_REQ_STATE_FREE; 	\
        higgs_card->running_req[iptt] = NULL;\
        higgs_put_higgs_req(higgs_card, req);	\
	} while (0)\


/*for error infor*/
/*Dump error infor from memory*/
#define HIGGS_DumpMemory(addr, len) \
    do { \
        u32 i;   \
        u8 *p = (u8 *)addr;   \
        HIGGS_TRACE(OSP_DBL_MINOR, 4533,"%s:%d,Length is %d, The Memory 0x%x is: \n",__FUNCTION__,__LINE__,len, addr);   \
        HIGGS_TRACE(OSP_DBL_MINOR, 4533,"[%p] ", p); \
        for (i = 0; i < len;) { \
            HIGGS_TRACE(OSP_DBL_MINOR, 4533,"%02x ", p[i]);    \
            i++;    \
            if (!(i & 0xF)) { \
                HIGGS_TRACE(OSP_DBL_MINOR, 4533,"\n[%p] ",p + i); \
            }   \
        }   \
      } while (0)
/*sas protocol error type*/
enum sas_protocol_err {
	PROTO_ERROR_NONE = 0,	/*this not be used */
	PROTO_ERROR_TIMEOUT,
	PROTO_ERROR_OPEN_REJECT,
	PROTO_ERROR_I_T_NEXUS_LOSS,
	PROTO_ERROR_ACK_NAK_TIMEOUT,
	PROTO_ERROR_NAK_RECEIVED,
	PROTO_ERROR_COMMAND_RETRY,
	PROTO_ERROR_OPEN_BY_OTHER,
	PROTO_ERROR_TOTAL_NUM
};

extern enum sal_cmnd_exec_result higgs_send_msg(struct sal_msg *msg);
extern s32 higgs_send_tm_msg(enum sal_tm_type sal_type, struct sal_msg *msg);

extern struct higgs_req *higgs_get_higgs_req(struct higgs_card *higgs_card,
					     enum higgs_inter_cmd_type
					     io_cmd_type);
extern void higgs_put_higgs_req(struct higgs_card *higgs_card,
				struct higgs_req *higgs_req);
extern u32 higgs_get_dqid_from_devid(struct higgs_card *higgs_card, u32 devid);
extern void higgs_prepare_base_ssp(struct sal_msg *sal_msg,
				   struct higgs_req *higgs_req);

extern void higgs_register_dispatcher(void);
extern struct higgs_cmd_table *higgs_get_command_table(struct higgs_card
						       *higgs_card, u32 iptt,
						       OSP_DMA * dma);
void *higgs_get_dq_info(struct higgs_card *higgs_card, u32 dq_id);
void higgs_req_callback(struct higgs_req *higgs_req, u32 cmpl_entry);
void higgs_send_command_hw(struct higgs_card *higgs_card,
			   struct higgs_req *higgs_req);
s32 higgs_req_state_chg(struct higgs_req *req, enum higgs_req_event event,
			bool need_lock);
bool higgs_check_smp_resp_length(struct sal_msg *sal_msg, u8 smp_resp_length);

#endif
