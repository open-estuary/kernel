/*
 * Huawei Pv660/Hi1610 sas controller msg process
 *
 * Copyright 2015 Huawei.
 *
 * This file is licensed under GPLv2.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
*/


#include "sal_common.h"
#include "sal_io.h"


struct sal_msg_fsm sal_msg_fsm_table[SAL_MSG_ST_BUTT][SAL_MSG_EVENT_BUTT];

void sal_init_msg_ele(struct sal_msg *msg, struct sal_card *sal_card)
{
	msg->msg_state = SAL_MSG_ST_FREE;
	SAS_SPIN_LOCK_INIT(&msg->msg_state_lock);
	SAS_INIT_COMPLETION(&msg->compl);

	memset(&msg->status, 0, sizeof(struct sal_msg_status));
	memset(&msg->ini_cmnd, 0, sizeof(struct sal_ini_cmnd));
	memset(&msg->eh, 0, sizeof(struct sal_err_handle));
	memset(&msg->sgl_head, 0, sizeof(struct drv_sgl));
	memset(&msg->protocol, 0, sizeof(union sal_protocol));

	msg->card = sal_card;
	msg->dev = NULL;
	msg->related_msg = NULL;
	msg->send_time = 0;
	msg->tag_not_found = false;
	msg->transmit_mark = false;
	msg->tm_func = SAL_TM_TYPE_BUTT;	
	msg->data_direction = SAL_MSG_DATA_BUTT;
	msg->status.drv_resp = SAL_MSG_DRV_FAILED;	
	msg->status.io_resp = SAM_STAT_GOOD;
	msg->status.result = SAL_HOST_STATUS(DID_ERROR);	
	msg->ll_tag = 0;
	msg->type = SAL_MSG_BUTT;
	msg->io_type = SAL_MSG_TYPE_BUTT_IO;
	msg->done = NULL;
	msg->eh.event_retries = 0;

	return;

}

s32 sal_msg_rsc_init(struct sal_card * sal_card)
{
	s32 ret = ERROR;
	u32 alloc_msg_mem = 0;
	u32 i = 0;
	struct sal_msg *tmp_msg = NULL;
	unsigned long flag = 0;
	u32 max_io_num = 0;
	u32 list_num = 0;

	max_io_num = MIN(SAL_MAX_MSG_NUM, sal_card->config.max_active_io);
	alloc_msg_mem = sizeof(struct sal_msg) * max_io_num;

	sal_card->msg_mem = SAS_VMALLOC(alloc_msg_mem);
	if (NULL == sal_card->msg_mem) {
		SAL_TRACE(OSP_DBL_MAJOR, 378,
			  "card:%d alloc msg mem fail %d byte",
			  sal_card->card_no, alloc_msg_mem);
		ret = ERROR;
		goto fail_done;
	}
	memset(sal_card->msg_mem, 0, alloc_msg_mem);

	for (i = 0; i < SAL_MAX_MSG_LIST_NUM; i++) {
		SAS_SPIN_LOCK_INIT(&sal_card->card_msg_list_lock[i]);
		SAS_INIT_LIST_HEAD(&sal_card->free_msg_list[i]);
		SAS_INIT_LIST_HEAD(&sal_card->active_msg_list[i]);
	}

	for (i = 0; i < max_io_num; i++) {

		tmp_msg = &sal_card->msg_mem[i];

		sal_init_msg_ele(tmp_msg, sal_card);
		tmp_msg->msg_htag = i;

		list_num = i % SAL_MAX_MSG_LIST_NUM;

		spin_lock_irqsave(&sal_card->card_msg_list_lock[list_num],
				  flag);
		SAS_LIST_ADD_TAIL(&tmp_msg->host_list,
				  &sal_card->free_msg_list[list_num]);
		spin_unlock_irqrestore(&sal_card->card_msg_list_lock[list_num],
				       flag);
	}
	SAL_TRACE(OSP_DBL_INFO, 378, "card:%d alloc Msg mem:%d byte OK",
		  sal_card->card_no, alloc_msg_mem);

	ret = OK;
	return ret;

      fail_done:
	(void)sal_msg_rsc_remove(sal_card);
	return ret;
}

/**
 * sal_get_msg -  get a free msg from free msg list
 * @sal_card: sas controller
 *
 */
struct sal_msg *sal_get_msg(struct sal_card *sal_card)
{
	struct sal_msg *msg_element = NULL;
	unsigned long flag = 0;
	u32 list_num = 0;
	struct list_head *list_node = NULL;

	list_num =
	    (u32) atomic_inc_return(&sal_card->msg_cnt) % SAL_MAX_MSG_LIST_NUM;

	spin_lock_irqsave(&sal_card->card_msg_list_lock[list_num], flag);
	if (unlikely(SAS_LIST_EMPTY(&sal_card->free_msg_list[list_num]))) {
		spin_unlock_irqrestore(&sal_card->card_msg_list_lock[list_num],
				       flag);

		SAL_TRACE_LIMIT(OSP_DBL_MAJOR, msecs_to_jiffies(1000), 1, 517,
				"Card:%d get msg failed from free list:%d",
				sal_card->card_no, list_num);

		return NULL;
	}
	list_node = sal_card->free_msg_list[list_num].next;
	SAS_LIST_DEL(list_node);
	SAS_LIST_ADD_TAIL(list_node, &sal_card->active_msg_list[list_num]);
	spin_unlock_irqrestore(&sal_card->card_msg_list_lock[list_num], flag);

    /*list_head is the frist member, so we use as sal_msg pointer directly */
	msg_element = (struct sal_msg *)list_node;	

	SAS_ATOMIC_INC(&sal_card->card_io_stat.active_io_cnt);

	sal_init_msg_ele(msg_element, sal_card);
	msg_element->send_time = jiffies;

	(void)sal_msg_state_chg(msg_element, SAL_MSG_EVENT_MSG_GET);

	return msg_element;
}

/**
 * sal_free_msg -  put msg back to free msg list
 * @sal_msg: pointer to msg
 *
 */
s32 sal_free_msg(struct sal_msg * msg)
{
	struct sal_card *sal_card;
	unsigned long flag = 0;
	u32 list_num = 0;

	SAL_ASSERT(NULL != msg, return ERROR);
	/* check whether msg state is correct or not  */
	SAL_ASSERT(msg->msg_state == SAL_MSG_ST_FREE, return ERROR);
	sal_card = msg->card;

	list_num = msg->msg_htag % SAL_MAX_MSG_LIST_NUM;

	spin_lock_irqsave(&sal_card->card_msg_list_lock[list_num], flag);
	SAS_LIST_DEL(&msg->host_list);
	SAS_LIST_ADD_TAIL(&msg->host_list, &sal_card->free_msg_list[list_num]);

	sal_init_msg_ele(msg, sal_card);

	spin_unlock_irqrestore(&sal_card->card_msg_list_lock[list_num], flag);

	SAS_ATOMIC_DEC(&sal_card->card_io_stat.active_io_cnt);

	return OK;
}

s32 sal_msg_print_aborting2aborted(struct sal_msg * msg)
{
	SAL_REF(msg);

	SAL_TRACE(OSP_DBL_INFO, 517,
		  "Msg:%p(uni id:0x%llx) change ST from aborting to aborted(%d)",
		  msg, msg->ini_cmnd.unique_id, msg->msg_state);
	return OK;
}

s32 sal_msg_print_aborted2tmo(struct sal_msg * msg)
{
	SAL_REF(msg);

	SAL_TRACE(OSP_DBL_INFO, 517,
		  "msg:%p(uni id:0x%llx) change ST from aborted to tmo(%d)",
		  msg, msg->ini_cmnd.unique_id, msg->msg_state);
	return OK;
}

s32 sal_msg_print_aborting2tmo(struct sal_msg * msg)
{
	SAL_REF(msg);

	SAL_TRACE(OSP_DBL_INFO, 517,
		  "msg:%p(uni id:0x%llx) change ST from aborting to tmo(%d)",
		  msg, msg->ini_cmnd.unique_id, msg->msg_state);
	return OK;
}

s32 sal_msg_print_aborting2send(struct sal_msg * msg)
{
	SAL_REF(msg);

	SAL_TRACE(OSP_DBL_INFO, 517,
		  "msg:%p(uni id:0x%llx) change ST from aborting to send(%d)",
		  msg, msg->ini_cmnd.unique_id, msg->msg_state);
	return OK;

}

s32 sal_msg_print_send2errhand(struct sal_msg * msg)
{
	SAL_REF(msg);

	SAL_TRACE(OSP_DBL_INFO, 517,
		  "msg:%p(uni id:0x%llx) change ST from send to err handing(Cur ST:%d)",
		  msg, msg->ini_cmnd.unique_id, msg->msg_state);
	return OK;

}

s32 sal_msg_print_errhand2send(struct sal_msg * msg)
{
	SAL_REF(msg);

	SAL_TRACE(OSP_DBL_INFO, 517,
		  "msg:%p(uni id:0x%llx) change ST from err handing to send(Cur ST:%d)",
		  msg, msg->ini_cmnd.unique_id, msg->msg_state);
	return OK;

}

s32 sal_msg_print_errhandle2pending(struct sal_msg * msg)
{
	SAL_REF(msg);

	SAL_TRACE(OSP_DBL_INFO, 517,
		  "msg:%p(uni id:0x%llx) chang ST from err handle to pending(Cur ST:%d)",
		  msg, msg->ini_cmnd.unique_id, msg->msg_state);
	return OK;

}

s32 sal_drv_done_msg(struct sal_msg * msg)
{
	msg->status.drv_resp = SAL_MSG_DRV_FAILED;

	sal_set_msg_result(msg);

	SAL_TRACE(OSP_DBL_INFO, 517,
		  "return msg:%p(uni id:0x%llx,ST:%d) failed to sas layer", msg,
		  msg->ini_cmnd.unique_id, msg->msg_state);
	sal_msg_done(msg);
	return OK;
}

s32 sal_msg_wake_up_waiter(struct sal_msg * msg)
{	
    /*wake up sas layer TM cmd */
	SAS_COMPLETE(&msg->compl);

	return OK;
}

s32 sal_msg_rsc_remove(struct sal_card * sal_card)
{
	if (sal_card->msg_mem) {
		SAS_VFREE(sal_card->msg_mem);
		sal_card->msg_mem = NULL;
	}
	return OK;
}

/**
 * sal_msg_state_chg_no_lock -  change the state of the msg without lock
 * @sal_msg: pointer to msg
 * @event: event that will change the msg state
 *
 */
s32 sal_msg_state_chg_no_lock(struct sal_msg * msg, enum sal_msg_event event)
{
	enum sal_msg_state old_state = SAL_MSG_ST_BUTT;
	enum sal_msg_state curr_state = SAL_MSG_ST_BUTT;
	sal_msg_st_chg_action_t act_func = NULL;
	s32 ret = ERROR;

	if (event >= SAL_MSG_EVENT_BUTT) {	
		SAL_TRACE(OSP_DBL_MAJOR, 517, "Msg event:%d is invalid",
			  (s32) event);
		return ERROR;
	}

	old_state = msg->msg_state;
	if (old_state >= SAL_MSG_ST_BUTT) {
		SAL_TRACE(OSP_DBL_MAJOR, 517,
			  "Msg:%p Old State:%d Event:%d is invalid", msg,
			  old_state, event);
		return ERROR;
	}

	curr_state = sal_msg_fsm_table[old_state][event].new_status;
	act_func = sal_msg_fsm_table[old_state][event].act_func;

	if (curr_state >= SAL_MSG_ST_BUTT) {
		SAL_TRACE(OSP_DBL_MAJOR, 517,
			  "tmp_msg %p old_state %d event %d New status %d is invalid",
			  msg, old_state, event, curr_state);
		return ERROR;
	}

	msg->msg_state = curr_state;
	ret = OK;

	if (act_func)
		ret = act_func(msg);

	return ret;
}

EXPORT_SYMBOL(sal_msg_state_chg_no_lock);

/**
 * sal_msg_state_chg -  change the state of the msg within lock
 * @sal_msg: pointer to msg
 * @event: event that will change the msg state
 *
 */
s32 sal_msg_state_chg(struct sal_msg * msg, enum sal_msg_event event)
{
	unsigned long flag = 0;
	enum sal_msg_state old_state = SAL_MSG_ST_BUTT;
	enum sal_msg_state curr_state = SAL_MSG_ST_BUTT;
	sal_msg_st_chg_action_t act_func = NULL;
	s32 ret = ERROR;

	SAL_ASSERT(NULL != msg, return ERROR);

	if (event >= SAL_MSG_EVENT_BUTT) {
		SAL_TRACE(OSP_DBL_MAJOR, 517, "Msg event:%d is invalid",
			  (s32) event);
		return ERROR;
	}

	spin_lock_irqsave(&msg->msg_state_lock, flag);
	old_state = msg->msg_state;
	if (old_state >= SAL_MSG_ST_BUTT) {
		SAL_TRACE(OSP_DBL_MAJOR, 517,
			  "Msg:%p Old State:%d Event:%d is invalid", msg,
			  old_state, event);
		spin_unlock_irqrestore(&msg->msg_state_lock, flag);
		return ERROR;
	}

	curr_state = sal_msg_fsm_table[old_state][event].new_status;
	act_func = sal_msg_fsm_table[old_state][event].act_func;

	if (curr_state >= SAL_MSG_ST_BUTT) {
		SAL_TRACE(OSP_DBL_MAJOR, 517,
			  "Msg:%p Old State:%d Event:%d, New status:%d is invalid",
			  msg, old_state, event, curr_state);
		spin_unlock_irqrestore(&msg->msg_state_lock, flag);
		return ERROR;
	}

	msg->msg_state = curr_state;
	spin_unlock_irqrestore(&msg->msg_state_lock, flag);
	ret = OK;
    
	if (act_func)
		ret = act_func(msg);

	return ret;
}

EXPORT_SYMBOL(sal_msg_state_chg);


inline void sal_msg_copy_from_ini_cmnd(struct sal_msg *msg,
				       struct drv_ini_cmd * ini_cmd)
{
	unsigned long flag = 0;

	SAL_ASSERT(msg != NULL, return);

	spin_lock_irqsave(&msg->msg_state_lock, flag);
	msg->ini_cmnd.tag = ini_cmd->tag;
	msg->ini_cmnd.unique_id = ini_cmd->cmd_sn;
	msg->ini_cmnd.upper_cmd = (u64) ini_cmd->upper_data;
	msg->ini_cmnd.port_id = ini_cmd->port_id;

	memcpy(msg->ini_cmnd.cdb, ini_cmd->cdb, DEFAULT_CDB_LEN);
	memcpy(msg->ini_cmnd.lun, ini_cmd->lun, DRV_SCSI_LUN_LEN);

	msg->ini_cmnd.data_len = ini_cmd->data_len;
	msg->ini_cmnd.data_dir = ini_cmd->io_direction;
	msg->ini_cmnd.done = ini_cmd->done;	


	if (DRV_IO_DIRECTION_WRITE == ini_cmd->io_direction)
		msg->data_direction = SAL_MSG_DATA_TO_DEVICE;
	else if (DRV_IO_DIRECTION_READ == ini_cmd->io_direction)
		msg->data_direction = SAL_MSG_DATA_FROM_DEVICE;
	else
		msg->data_direction = SAL_MSG_DATA_NONE;

	if (ini_cmd->sgl)
		msg->ini_cmnd.sgl = ini_cmd->sgl;
	else
		msg->ini_cmnd.sgl = NULL;

	spin_unlock_irqrestore(&msg->msg_state_lock, flag);

	return;
}

inline void sal_msg_copy_to_ini_cmnd(struct sal_msg *msg, struct drv_ini_cmd * ini_cmd)
{
	struct drv_scsi_cmd_result *cmd_status = NULL;

	memset(ini_cmd, 0, sizeof(struct drv_ini_cmd));

	ini_cmd->cmd_sn = msg->ini_cmnd.unique_id;
	ini_cmd->done = msg->ini_cmnd.done;
	ini_cmd->upper_data = (void *)msg->ini_cmnd.upper_cmd;
	ini_cmd->remain_data_len = msg->status.res_id;
	ini_cmd->port_id = msg->ini_cmnd.port_id;

	cmd_status = &ini_cmd->result;

	cmd_status->status = (u32) msg->status.result;
	cmd_status->sense_data_len = (u16) msg->status.sense_len;
	memcpy(cmd_status->sense_data,
	       msg->status.sense,
	       MIN(msg->status.sense_len, SCSI_SENSE_DATA_LEN));

	return;
}

/**
 * sal_wait_msg_state -  wait until the msg state change to the specified state
 * @org_msg: pointer to msg
 * @wait_time_ms: max wait time in ms
 * @wait_state: the specified state
 *
 */
s32 sal_wait_msg_state(struct sal_msg * org_msg, u32 wait_time_ms,
		       enum sal_msg_state wait_state)
{
	unsigned long flag = 0;
	enum sal_msg_state curr_state;
	s32 ret = ERROR;
	unsigned long start_wait = jiffies;

	for (;;) {
		spin_lock_irqsave(&org_msg->msg_state_lock, flag);
		curr_state = org_msg->msg_state;
		spin_unlock_irqrestore(&org_msg->msg_state_lock, flag);

		if (curr_state == wait_state) {
			ret = OK;
			break;
		}

		if (jiffies_to_msecs(jiffies - start_wait) > wait_time_ms) {
			ret = ERROR;
			break;
		}

		SAS_MSLEEP(10);
	}
	return ret;
}

/**
 * sal_msg_rsc_process -  process the msg
 * @sal_msg: pointer to msg
 *
 */
s32 sal_msg_rsc_process(struct sal_msg * msg)
{
	struct drv_ini_cmd ini_cmd;
	unsigned long flag = 0;

	SAL_ASSERT(SAL_MSG_ST_FREE == sal_get_msg_state(msg), return ERROR);

	SAL_TRACE(OSP_DBL_INFO, 517,
		  "msg:%p(uni id:0x%llx ST:%d) return to sal layer", msg,
		  msg->ini_cmnd.unique_id, msg->msg_state);


	spin_lock_irqsave(&msg->msg_state_lock, flag);
	if (msg->ini_cmnd.done) {
		msg->status.drv_resp = SAL_MSG_DRV_FAILED;
		sal_set_msg_result(msg);

		sal_msg_copy_to_ini_cmnd(msg, &ini_cmd);

		msg->ini_cmnd.done(&ini_cmd);
		msg->ini_cmnd.done = NULL;
	}
	spin_unlock_irqrestore(&msg->msg_state_lock, flag);

	(void)sal_free_msg(msg);

	return OK;
}

void sal_msg_fsm_init(void)
{
	u32 event = 0;
	u32 st = 0;

	memset(sal_msg_fsm_table, 0, sizeof(sal_msg_fsm_table));

	for (st = 0; st < SAL_MSG_ST_BUTT; st++)
		for (event = 0; event < SAL_MSG_EVENT_BUTT; event++) {
			sal_msg_fsm_table[st][event].new_status =
			    SAL_MSG_ST_BUTT;
			sal_msg_fsm_table[st][event].act_func = NULL;
		}

	sal_msg_fsm_table[SAL_MSG_ST_FREE][SAL_MSG_EVENT_MSG_GET].new_status =
	    SAL_MSG_ST_ACTIVE;
	sal_msg_fsm_table[SAL_MSG_ST_ACTIVE][SAL_MSG_EVENT_IOSTART].new_status =
	    SAL_MSG_ST_SEND;

	sal_msg_fsm_table[SAL_MSG_ST_SEND][SAL_MSG_EVENT_DONE_MID].new_status = SAL_MSG_ST_DONE_CALLED;	
	sal_msg_fsm_table[SAL_MSG_ST_SEND][SAL_MSG_EVENT_ABORT_START].
	    new_status = SAL_MSG_ST_ABORTING;

	sal_msg_fsm_table[SAL_MSG_ST_SEND][SAL_MSG_EVENT_SEND_FAIL].new_status =
	    SAL_MSG_ST_FREE;
	sal_msg_fsm_table[SAL_MSG_ST_SEND][SAL_MSG_EVENT_SEND_FAIL].act_func =
	    sal_free_msg;

	sal_msg_fsm_table[SAL_MSG_ST_DONE_CALLED][SAL_MSG_EVENT_DONE_UNUSE].
	    new_status = SAL_MSG_ST_FREE;
	sal_msg_fsm_table[SAL_MSG_ST_DONE_CALLED][SAL_MSG_EVENT_DONE_UNUSE].act_func = sal_free_msg;	

	sal_msg_fsm_table[SAL_MSG_ST_ABORTING][SAL_MSG_EVENT_DONE_UNUSE].
	    new_status = SAL_MSG_ST_ABORTED;
	sal_msg_fsm_table[SAL_MSG_ST_ABORTING][SAL_MSG_EVENT_DONE_UNUSE].
	    act_func = sal_msg_print_aborting2aborted;

	sal_msg_fsm_table[SAL_MSG_ST_ABORTING][SAL_MSG_EVENT_JAM_FAIL].new_status = SAL_MSG_ST_ABORTED;	
	sal_msg_fsm_table[SAL_MSG_ST_ABORTING][SAL_MSG_EVENT_JAM_FAIL].
	    act_func = sal_msg_print_aborting2aborted;

	sal_msg_fsm_table[SAL_MSG_ST_ABORTING][SAL_MSG_EVENT_WAIT_FAIL].new_status = SAL_MSG_ST_TMO;	
	sal_msg_fsm_table[SAL_MSG_ST_ABORTING][SAL_MSG_EVENT_WAIT_FAIL].
	    act_func = sal_msg_print_aborting2tmo;

	sal_msg_fsm_table[SAL_MSG_ST_ABORTING][SAL_MSG_EVENT_CANCEL_ERR_HANDLE].new_status = SAL_MSG_ST_SEND;	
	sal_msg_fsm_table[SAL_MSG_ST_ABORTING][SAL_MSG_EVENT_CANCEL_ERR_HANDLE].
	    act_func = sal_msg_print_aborting2send;

	sal_msg_fsm_table[SAL_MSG_ST_ABORTED][SAL_MSG_EVENT_DONE_UNUSE].new_status = SAL_MSG_ST_ABORTED;

	sal_msg_fsm_table[SAL_MSG_ST_ABORTED][SAL_MSG_EVENT_CANCEL_ERR_HANDLE].
	    new_status = SAL_MSG_ST_FREE;
	sal_msg_fsm_table[SAL_MSG_ST_ABORTED][SAL_MSG_EVENT_CANCEL_ERR_HANDLE].
	    act_func = sal_msg_rsc_process;

	sal_msg_fsm_table[SAL_MSG_ST_ABORTED][SAL_MSG_EVENT_WAIT_OK].new_status = SAL_MSG_ST_FREE;
	sal_msg_fsm_table[SAL_MSG_ST_ABORTED][SAL_MSG_EVENT_WAIT_OK].act_func = sal_free_msg;

	sal_msg_fsm_table[SAL_MSG_ST_ABORTED][SAL_MSG_EVENT_WAIT_FAIL].new_status = SAL_MSG_ST_TMO;	
	sal_msg_fsm_table[SAL_MSG_ST_ABORTED][SAL_MSG_EVENT_WAIT_FAIL].act_func = sal_msg_print_aborted2tmo;	

	sal_msg_fsm_table[SAL_MSG_ST_TMO][SAL_MSG_EVENT_CHIP_REMOVE].new_status = SAL_MSG_ST_FREE;
	sal_msg_fsm_table[SAL_MSG_ST_TMO][SAL_MSG_EVENT_CHIP_REMOVE].act_func =
	    sal_free_msg;

	sal_msg_fsm_table[SAL_MSG_ST_TMO][SAL_MSG_EVENT_CHIP_RESET_DONE].new_status = SAL_MSG_ST_FREE;
	sal_msg_fsm_table[SAL_MSG_ST_TMO][SAL_MSG_EVENT_CHIP_RESET_DONE].
	    act_func = sal_free_msg;

	sal_msg_fsm_table[SAL_MSG_ST_TMO][SAL_MSG_EVENT_DONE_UNUSE].new_status = SAL_MSG_ST_FREE;	
	sal_msg_fsm_table[SAL_MSG_ST_TMO][SAL_MSG_EVENT_DONE_UNUSE].act_func =
	    sal_free_msg;

	sal_msg_fsm_table[SAL_MSG_ST_SEND][SAL_MSG_EVENT_CHIP_REMOVE].new_status = SAL_MSG_ST_SEND;
	sal_msg_fsm_table[SAL_MSG_ST_SEND][SAL_MSG_EVENT_CHIP_REMOVE].act_func = sal_drv_done_msg;

	sal_msg_fsm_table[SAL_MSG_ST_IN_JAM][SAL_MSG_EVENT_CHIP_REMOVE].new_status = SAL_MSG_ST_SEND;	
	sal_msg_fsm_table[SAL_MSG_ST_IN_JAM][SAL_MSG_EVENT_CHIP_REMOVE].act_func = sal_drv_done_msg;	

	sal_msg_fsm_table[SAL_MSG_ST_IN_JAM][SAL_MSG_EVENT_JAM_FAIL].new_status = SAL_MSG_ST_SEND;	
	sal_msg_fsm_table[SAL_MSG_ST_IN_JAM][SAL_MSG_EVENT_JAM_FAIL].act_func = sal_drv_done_msg;	

	sal_msg_fsm_table[SAL_MSG_ST_SEND][SAL_MSG_EVENT_NEED_JAM].new_status = SAL_MSG_ST_IN_JAM;	
	sal_msg_fsm_table[SAL_MSG_ST_ACTIVE][SAL_MSG_EVENT_NEED_JAM].new_status = SAL_MSG_ST_IN_JAM;

	sal_msg_fsm_table[SAL_MSG_ST_IN_JAM][SAL_MSG_EVENT_IOSTART].new_status = SAL_MSG_ST_SEND;	
	sal_msg_fsm_table[SAL_MSG_ST_IN_JAM][SAL_MSG_EVENT_ABORT_START].new_status = SAL_MSG_ST_ABORTING;	

	sal_msg_fsm_table[SAL_MSG_ST_SEND][SAL_MSG_EVENT_NEED_ERR_HANDLE].new_status = SAL_MSG_ST_IN_ERRHANDING;	
	sal_msg_fsm_table[SAL_MSG_ST_SEND][SAL_MSG_EVENT_NEED_ERR_HANDLE].
	    act_func = sal_msg_print_send2errhand;

	sal_msg_fsm_table[SAL_MSG_ST_IN_ERRHANDING][SAL_MSG_EVENT_CHIP_REMOVE].new_status = SAL_MSG_ST_SEND;	
	sal_msg_fsm_table[SAL_MSG_ST_IN_ERRHANDING][SAL_MSG_EVENT_CHIP_REMOVE].act_func = sal_drv_done_msg;	

	sal_msg_fsm_table[SAL_MSG_ST_IN_ERRHANDING][SAL_MSG_EVENT_ABORT_START].new_status = SAL_MSG_ST_ABORTING;	
	sal_msg_fsm_table[SAL_MSG_ST_IN_ERRHANDING][SAL_MSG_EVENT_HDRST_RTY_HAND_OVER].new_status = SAL_MSG_ST_SEND;	
	sal_msg_fsm_table[SAL_MSG_ST_IN_ERRHANDING][SAL_MSG_EVENT_LL_RTY_HAND_OVER].new_status = SAL_MSG_ST_SEND;	

	sal_msg_fsm_table[SAL_MSG_ST_IN_ERRHANDING][SAL_MSG_EVENT_DONE_UNUSE].new_status = SAL_MSG_ST_PENDING_IN_DRV;
	sal_msg_fsm_table[SAL_MSG_ST_IN_ERRHANDING][SAL_MSG_EVENT_DONE_UNUSE].
	    act_func = sal_msg_print_errhandle2pending;

	sal_msg_fsm_table[SAL_MSG_ST_PENDING_IN_DRV]
	    [SAL_MSG_EVENT_CANCEL_ERR_HANDLE].new_status = SAL_MSG_ST_FREE;
	sal_msg_fsm_table[SAL_MSG_ST_PENDING_IN_DRV]
	    [SAL_MSG_EVENT_CANCEL_ERR_HANDLE].act_func = sal_msg_rsc_process;

	sal_msg_fsm_table[SAL_MSG_ST_PENDING_IN_DRV][SAL_MSG_EVENT_ABORT_START].
	    new_status = SAL_MSG_ST_ABORTING;
	sal_msg_fsm_table[SAL_MSG_ST_PENDING_IN_DRV][SAL_MSG_EVENT_NEED_JAM].
	    new_status = SAL_MSG_ST_IN_JAM;

	sal_msg_fsm_table[SAL_MSG_ST_IN_ERRHANDING][SAL_MSG_EVENT_CANCEL_ERR_HANDLE].new_status = SAL_MSG_ST_SEND;	
	sal_msg_fsm_table[SAL_MSG_ST_IN_ERRHANDING]
	    [SAL_MSG_EVENT_CANCEL_ERR_HANDLE].act_func =
	    sal_msg_print_errhand2send;

	sal_msg_fsm_table[SAL_MSG_ST_ABORTED][SAL_MSG_EVENT_NEED_JAM].new_status = SAL_MSG_ST_IN_JAM;	

	sal_msg_fsm_table[SAL_MSG_ST_ABORTED][SAL_MSG_EVENT_ADD_JAM_FAIL].new_status = SAL_MSG_ST_TMO;	
	sal_msg_fsm_table[SAL_MSG_ST_ABORTED][SAL_MSG_EVENT_ADD_JAM_FAIL].
	    act_func = sal_msg_print_aborted2tmo;

	sal_msg_fsm_table[SAL_MSG_ST_ACTIVE][SAL_MSG_EVENT_SMP_START].
	    new_status = SAL_MSG_ST_SEND;

	sal_msg_fsm_table[SAL_MSG_ST_SEND][SAL_MSG_EVENT_SMP_SEND_FAIL].
	    new_status = SAL_MSG_ST_FREE;
	sal_msg_fsm_table[SAL_MSG_ST_SEND][SAL_MSG_EVENT_SMP_SEND_FAIL].
	    act_func = sal_free_msg;

	sal_msg_fsm_table[SAL_MSG_ST_SEND][SAL_MSG_EVENT_SMP_WAIT_OK].new_status = SAL_MSG_ST_FREE;
	sal_msg_fsm_table[SAL_MSG_ST_SEND][SAL_MSG_EVENT_SMP_WAIT_OK].act_func =
	    sal_free_msg;

	sal_msg_fsm_table[SAL_MSG_ST_SEND][SAL_MSG_EVENT_SMP_WAIT_FAIL].
	    new_status = SAL_MSG_ST_TMO;

	sal_msg_fsm_table[SAL_MSG_ST_TMO][SAL_MSG_EVENT_SMP_DONE].new_status =
	    SAL_MSG_ST_FREE;
	sal_msg_fsm_table[SAL_MSG_ST_TMO][SAL_MSG_EVENT_SMP_DONE].act_func =
	    sal_free_msg;

	sal_msg_fsm_table[SAL_MSG_ST_SEND][SAL_MSG_EVENT_SMP_DONE].new_status = SAL_MSG_ST_SEND;
	sal_msg_fsm_table[SAL_MSG_ST_SEND][SAL_MSG_EVENT_SMP_DONE].act_func =
	    sal_msg_wake_up_waiter;
}

/**
 * sal_get_msg_state -  get the current msg state
 * @sal_msg: pointer to msg
 *
 */
enum sal_msg_state sal_get_msg_state(struct sal_msg *msg)
{
	unsigned long flag = 0;
	enum sal_msg_state state = SAL_MSG_ST_BUTT;
	SAL_ASSERT(NULL != msg, return SAL_MSG_ST_BUTT);

	spin_lock_irqsave(&msg->msg_state_lock, flag);
	state = msg->msg_state;
	spin_unlock_irqrestore(&msg->msg_state_lock, flag);

	return state;
}

