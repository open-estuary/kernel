/*
 * Huawei Pv660/Hi1610 sas controller io process
 *
 * Copyright 2015 Huawei. <chenjianmin@huawei.com>
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
#include "sal_dev.h"
#include "sal_io.h"
#include "sal_errhandler.h"
#include "sal_init.h"

void sal_smp_done(struct sal_msg *msg);

volatile u32 sal_simulate_io_fatal = 0;
EXPORT_SYMBOL(sal_simulate_io_fatal);

/**
 * sal_io_cnt_Sum -  io collect sum
 * @sal_card: point sas controller
 * @io_direct:collect type
 *
 */
void sal_io_cnt_Sum(struct sal_card *sal_card, u32 io_direct)
{
	unsigned long flag = 0;

	if (SAL_IO_STAT_ENABLE == sal_card->card_io_stat.stat_switch) {
		spin_lock_irqsave(&sal_card->card_io_stat.io_stat_lock, flag);
		switch (io_direct) {
		case SAL_QUEUE_FROM_SCSI:
			sal_card->card_io_stat.send_cnt++;
			break;

		case SAL_RESPONSE_TO_SCSI:
			sal_card->card_io_stat.response_cnt++;
			break;

		case SAL_EHABORT_FREOM_SCSI:
			sal_card->card_io_stat.eh_io_cnt++;
			break;

		default:
			SAL_TRACE_FRQLIMIT(OSP_DBL_MAJOR,
					   msecs_to_jiffies(3000), 809,
					   "Card:%d IO Direction:%d is invalid",
					   sal_card->card_no, io_direct);
			break;
		}

		spin_unlock_irqrestore(&sal_card->card_io_stat.io_stat_lock,
				       flag);
	}

	return;
}

/**
 * sal_abort_task_set_cb - deal with abort task set command response
 * @sal_card: point sas controller
 * @io_direct:collect type
 *
 */
void sal_abort_task_set_cb(struct sal_msg *msg)
{
	struct sal_dev *dev = NULL;

	SAL_ASSERT(NULL != msg, return);
	SAL_REF(dev);

	dev = msg->dev;
	SAL_ASSERT(NULL != dev, return);

	if (SAL_MSG_DRV_SUCCESS == msg->status.drv_resp)
		SAL_TRACE(OSP_DBL_DATA, 809,
			  "Msg:%p(ST:%d Htag:%d) to dev:0x%llx abort-task-set rsp OK",
			  msg, msg->msg_state, msg->ll_tag, dev->sas_addr);
	else
		SAL_TRACE(OSP_DBL_MAJOR, 809,
			  "Msg:%p(ST:%d Htag:%d) to dev:0x%llx abort-task-set rsp failed",
			  msg, msg->msg_state, msg->ll_tag, dev->sas_addr);

	/* send -> free */
	(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_SMP_WAIT_OK);
	return;
}

/**
 * sal_abort_task_set - send abort task set command to disk,do io clean
 * @sal_card: point sas controller
 * @sal_dev:disk device
 *
 */
s32 sal_abort_task_set(struct sal_card * sal_card, struct sal_dev * sal_dev)
{
	s32 ret = ERROR;
	struct sal_msg *msg = NULL;

	SAL_ASSERT(NULL != sal_dev, return ERROR);

	if (SAL_DEV_TYPE_SSP != sal_dev->dev_type) {
		/*not ssp device£¬need not send abort task set command */
		SAL_TRACE(OSP_DBL_DATA, 936,
			  "Card:%d dev:0x%llx type:%d,not need to send abort-task-set cmd",
			  sal_card->card_no, sal_dev->sas_addr,
			  sal_dev->dev_type);
		return ERROR;
	}

	if (SAL_PORT_MODE_INI != sal_card->config.work_mode) {
		SAL_TRACE(OSP_DBL_DATA, 936,
			  "Card:%d work mode:%d is not initiator",
			  sal_card->card_no, sal_card->config.work_mode);
		return ERROR;
	}

	if (NULL == sal_card->ll_func.send_tm) {
		SAL_TRACE(OSP_DBL_MAJOR, 936, "Card:%d TMReq func is NULL",
			  sal_card->card_no);
		return ERROR;
	}

	msg = sal_get_msg(sal_card);
	if (NULL == msg) {
		SAL_TRACE(OSP_DBL_MAJOR, 936, "Card:%d get msg failed",
			  sal_card->card_no);
		return ERROR;
	}

	(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_SMP_START);

	msg->type = SAL_MSG_PROT_SSP;
	msg->dev = sal_dev;
	msg->tm_func = SAL_TM_ABORT_TASKSET;
	msg->done = sal_abort_task_set_cb;

	ret = sal_card->ll_func.send_tm(SAL_TM_ABORT_TASKSET, msg);
	if (OK != ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 809,
			  "Card:%d send Abort Task Set failed",
			  sal_card->card_no);
		(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_SMP_SEND_FAIL);
		return ERROR;
	}

	return ret;

}

/**
 * sal_send_smp - send smp message
 * @dev: point to device
 * @req: smp request payload
 * @req_len:smp request payload len
 * @resp:response 
 * @resp_len1:response len
 * @wait_func: wait function
 *
 */
s32 sal_send_smp(struct sal_dev * dev, u8 * req, u32 req_len,
		 u8 * resp, u32 resp_len1, sal_eh_wait_smp_func_t wait_func)
{
	s32 ret = ERROR;
	struct sal_card *card = NULL;
	struct sal_port *port = NULL;
	struct sal_msg *msg = NULL;
	enum sal_eh_wait_func_ret wait_smp_ret = SAL_EH_WAITFUNC_RET_TIMEOUT;

	SAL_ASSERT(NULL != wait_func, return ERROR);
	card = dev->card;
	port = dev->port;

	msg = sal_get_msg(card);
	if (NULL == msg) {
		SAL_TRACE(OSP_DBL_MAJOR, 827, "Card:%d get msg failed",
			  card->card_no);
		return ERROR;
	}

	(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_SMP_START);

	msg->dev = dev;
	msg->type = SAL_MSG_PROT_SMP;

	if (req_len <= SAL_SMP_DIRECT_PAYLOAD) {
		memcpy(msg->protocol.smp.smp_payload, req, req_len);
		msg->protocol.smp.smp_len = req_len;
		msg->protocol.smp.rsp_virt_addr = resp;
		msg->protocol.smp.rsp_len = resp_len1;
	} else {
		SAL_TRACE(OSP_DBL_MAJOR, 827,
			  "Card:%d indirect mode not support", card->card_no);
		(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_SMP_SEND_FAIL);
		return ERROR;
	}
	msg->done = sal_smp_done;

    /*higgs_send_smp */
	ret = card->ll_func.send_msg(msg);
	if (OK != ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 828,
			  "Card:%d send smp request:%p to dev:0x%llx failed",
			  card->card_no, msg, dev->sas_addr);
		(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_SMP_SEND_FAIL);
		return ERROR;
	}

    /*sal_default_smp_wait_func or sal_inner_eh_smp_wait_func */
	wait_smp_ret = wait_func(msg, port);
	if (SAL_EH_WAITFUNC_RET_PORTDOWN == wait_smp_ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 829,
			  "Card:%d port:%d is link down,exit", card->card_no,
			  port->port_id);
		/* send -> tmo */
		(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_SMP_WAIT_FAIL);
		
		/*wait clear port resource finish*/
		SAS_MSLEEP(100);
		return ERROR;
	} else if (SAL_EH_WAITFUNC_RET_TIMEOUT == wait_smp_ret
		   || SAL_EH_WAITFUNC_RET_ABORT == wait_smp_ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 829,
			  "Card:%d send smp:%p to dev:0x%llx %s,but need to abort it",
			  card->card_no, msg, dev->sas_addr,
			  ((SAL_EH_WAITFUNC_RET_TIMEOUT ==
			    wait_smp_ret) ? "timeout" : "aborted"));
		sal_abort_smp_request(card, msg);

		return ERROR;
	} else if (SAL_EH_WAITFUNC_RET_WAITOK == wait_smp_ret) {
		/* SMP response£¬check result*/
		if (SAL_MSG_DRV_FAILED == msg->status.drv_resp) {
			SAL_TRACE(OSP_DBL_MAJOR, 831,
				  "Card:%d send smp request:%p to dev:0x%llx rsp failed.",
				  card->card_no, msg, dev->sas_addr);
			ret = ERROR;
		} else
			ret = OK;

		/* send -> free */
		(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_SMP_WAIT_OK);
		return ret;
	} else {
		SAL_TRACE_LIMIT(OSP_DBL_MINOR, msecs_to_jiffies(3000), 2, 555,
				"Card:%d send smp:%p to dev:0x%llx ret %d. wait fail.",
				card->card_no,
				msg, dev->sas_addr, wait_smp_ret);
		return ERROR;
	}
}

/**
 * sal_send_disc_smp - send smp message with discover module format
 * @dev_data: point to device infomation
 * @smp_req: smp request
 * @req_len:smp request len
 * @smp_resp:response 
 * @resp_len:response len
 *
 */
s32 sal_send_disc_smp(struct disc_device_data * dev_data,
		      struct disc_smp_req * smp_req, u32 req_len,
		      struct disc_smp_response * smp_resp, u32 resp_len)
{
	struct sal_dev *sal_dev = NULL;
	struct sal_card *card = NULL;
	struct sal_port *port = NULL;

	SAL_ASSERT(NULL != dev_data, return ERROR);
	SAL_ASSERT(NULL != smp_req, return ERROR);
	SAL_ASSERT(NULL != smp_resp, return ERROR);
	SAL_ASSERT(SAL_MAX_CARD_NUM > dev_data->card_id, return ERROR);

	card = sal_get_card(dev_data->card_id);
	if (NULL == card) {
		SAL_TRACE(OSP_DBL_MAJOR, 832, "Card:%d is null",
			  dev_data->card_id);
		return ERROR;
	}

	port = sal_find_port_by_chip_port_id(card, dev_data->port_id);
	if (NULL == port) {
		SAL_TRACE(OSP_DBL_MAJOR, 833, "Can't find port by port id:%d",
			  dev_data->port_id);
		sal_put_card(card);
		return ERROR;
	}

	sal_dev = sal_get_dev_by_sas_addr(port, dev_data->sas_addr);
	if (NULL == sal_dev) {
		SAL_TRACE(OSP_DBL_MAJOR, 834,
			  "Card:%d Port:%d(logic id:0x%x) get dev failed by dev addr:0x%llx",
			  dev_data->card_id, port->port_id, port->logic_id,
			  dev_data->sas_addr);

		sal_put_card(card);
		return ERROR;
	}

	if (OK != sal_send_smp(sal_dev, (u8 *) smp_req,
			       req_len, (u8 *) smp_resp,
			       resp_len, sal_default_smp_wait_func)) {
		SAL_TRACE(OSP_DBL_MAJOR, 835,
			  "Card:%d Send SMP request to dev:0x%llx failed",
			  card->card_no, dev_data->sas_addr);
		sal_put_dev(sal_dev);
		sal_put_card(card);
		return ERROR;
	}

	sal_put_dev(sal_dev);
	sal_put_card(card);
	
	return OK;
}

u32 sal_get_io_len_type(u32 len)
{
	u32 i = 0;

	for (i = 0; i < SAL_IOSTAT_CNT_MAX; i++)
		if (len <= sal_global.io_stat_threshold[i])
			return i;

	return SAL_IOSTAT_CNT_MAX - 1;
}

/**
 * sal_save_io_info - save last 10 io infomations
 * @msg: device infomation
 *
 */
void sal_save_io_info(struct sal_msg *msg)
{
	u32 io_index = 0;
	struct sal_dev *sal_dev = NULL;
	struct sal_disk_io_stat *disk_io_stat = NULL;
	unsigned long flag = 0;

	sal_dev = msg->dev;
	disk_io_stat = &sal_dev->dev_io_stat;

	spin_lock_irqsave(&sal_dev->dev_lock, flag);
	disk_io_stat->total_time += jiffies - msg->send_time;
	io_index = disk_io_stat->total_back % 10;
	memcpy(disk_io_stat->last_10_ios[io_index].cdb,
	       &msg->ini_cmnd.cdb[0], DEFAULT_CDB_LEN);
	disk_io_stat->last_10_ios[io_index].start_jiff = msg->send_time;
	disk_io_stat->last_10_ios[io_index].back_jiff = jiffies;
	disk_io_stat->last_10_ios[io_index].ret_val = (u32) msg->status.result;
	disk_io_stat->total_back++;

	spin_unlock_irqrestore(&sal_dev->dev_lock, flag);

	return;
}

void sal_inner_io_stat(struct sal_msg *msg)
{
	struct sal_card *sal_card = NULL;
	u32 port_idx = 0;
	u32 idx = 0;
	struct sal_port_io_stat *port_stats = NULL;
	unsigned long flag = 0;

	sal_card = msg->card;

	SAL_ASSERT(SAL_DEV_ST_FREE != sal_get_dev_state(msg->dev), return);
	SAL_ASSERT(msg->dev->port, return);

	port_idx = msg->dev->port->logic_id & 0xff;
	if (port_idx >= sal_card->phy_num) {
		SAL_TRACE(OSP_DBL_DATA, 788,
			  "Card:%d logic port:0x%x is invalid",
			  msg->card->card_no, msg->dev->port->logic_id);
		return;
	}

	if (SAL_IO_STAT_ENABLE == sal_card->card_io_stat.stat_switch) {
		spin_lock_irqsave(&sal_card->card_io_stat.io_stat_lock, flag);

		port_stats = &sal_card->card_io_stat.port_io_stat[port_idx];

		/*stat total*/
		port_stats->total_io += 1;
		port_stats->total_len += msg->ini_cmnd.data_len;

		idx = sal_get_io_len_type(msg->ini_cmnd.data_len);

		port_stats->io_dist[idx] += 1;
		if ((SAL_MSG_DRV_SUCCESS == msg->status.drv_resp) ||
		    ((SAL_MSG_DRV_UNDERFLOW == msg->status.drv_resp) &&
		     (!SAL_IS_RW_CMND(msg->ini_cmnd.cdb[0])))) {
			if (SAL_MSG_PROT_SSP == msg->type)
				port_stats->ssp_io[idx] += 1;
			else if (SAL_MSG_PROT_STP == msg->type)
				port_stats->stp_io[idx] += 1;
		} else {
			port_stats->fail_io[idx] += 1;
		}

		if (SAL_IS_READ_CMND(msg->ini_cmnd.cdb[0]))
			port_stats->read_io[idx] += 1;
		else if (SAL_IS_WRITE_CMND(msg->ini_cmnd.cdb[0]))
			port_stats->write_io[idx] += 1;

		port_stats->time_dist[idx] += jiffies - msg->send_time;

		/*save last 10 CDBs */
		sal_save_io_info(msg);

		spin_unlock_irqrestore(&sal_card->card_io_stat.io_stat_lock,
				       flag);
	}

	return;
}
#if 0
/**
 * sal_drv_io_stat - stat io, include outer io¡¢inner io and io sum
 * @msg: device infomation
 *
 */
void sal_drv_io_stat(struct sal_msg *msg)
{
	sal_outer_io_stat(msg);

	sal_inner_io_stat(msg);

	sal_io_cnt_Sum(msg->card, SAL_RESPONSE_TO_SCSI);
}
#endif
/**
 * sal_add_io_to_jam - add io to jam
 * @msg: io infomation
 *
 */
s32 sal_add_io_to_jam(struct sal_msg *msg)
{
	struct sal_dev *dev = NULL;
	s32 ret = ERROR;
	enum sal_dev_state dev_state = SAL_DEV_ST_BUTT;
	unsigned long state_flag = 0;
	u32 card_flag = 0;

	SAL_ASSERT(NULL != msg, return ERROR);
	dev = msg->dev;
	SAL_ASSERT(NULL != dev, return ERROR);

	card_flag = sal_get_card_flag(msg->card);
	if (card_flag & SAL_CARD_REMOVE_PROCESS) {
		/*card is removing, do not add to JAM */
		SAL_TRACE_LIMIT(OSP_DBL_MINOR, msecs_to_jiffies(3000), 3, 555,
				"Card:%d is removing now,msg:%p(uni id:0x%llx) no need add to jam",
				msg->card->card_no, msg,
				msg->ini_cmnd.unique_id);

		return ERROR;
	}

	dev_state = sal_get_dev_state(dev);

	/*disk is offline, do not add to JAM*/ 
	if (SAL_DEV_ST_RDY_FREE == dev_state) {
		SAL_TRACE(OSP_DBL_MINOR, 788,
			  "Card:%d dev:0x%llx(sal dev:%p ST:%d) ready to report out,don't add msg:%p(uni id:0x%llx) to jam",
			  msg->card->card_no, dev->sas_addr, dev, dev_state,
			  msg, msg->ini_cmnd.unique_id);
		return ERROR;
	} else if (SAL_DEV_ST_ACTIVE == dev_state) {
		/*device is normal, do not add to JAM*/
		return ERROR;
	} else {
		spin_lock_irqsave(&msg->msg_state_lock, state_flag);
		if (SAL_OR(SAL_MSG_ST_SEND == msg->msg_state,
			   SAL_MSG_ST_ACTIVE == msg->msg_state)) {
			if (jiffies_to_msecs(jiffies - msg->send_time)
				>= msg->card->config.jam_tmo) {
				/*close jam, io save in driver,middle will retry*/
				msg->status.result = SAL_HOST_STATUS(DID_REQUEUE);
				ret = ERROR;
			} else {
				if (OK ==
				    sal_msg_state_chg_no_lock(msg,
							      SAL_MSG_EVENT_NEED_JAM)) {
					/* IO add to jam list */
					(void)sal_add_msg_to_err_handler(msg,
									 SAL_ERR_HANDLE_ACTION_RESENDMSG,
									 msg->send_time);
					ret = OK;
				}
			}
		}
		spin_unlock_irqrestore(&msg->msg_state_lock, state_flag);

		return ret;
	}

}

/**
 * sal_done_io_add_jam - after io done,check device satus and discover status,
 * if need add io to jam list.
 * @msg: io infomation
 *
 */
s32 sal_done_io_add_jam(struct sal_msg * msg)
{
	s32 ret = ERROR;

	ret = sal_add_io_to_jam(msg);
	if (OK == ret)
		SAL_TRACE(OSP_DBL_INFO, 788,
			  "Card:%d add msg:%p(uni id:0x%llx,cdb[0]:0x%x,ST:%d) to dev addr:0x%llx(sal dev:%p ST:%d) to jam OK",
			  msg->card->card_no, msg, msg->ini_cmnd.unique_id,
			  msg->ini_cmnd.cdb[0], msg->msg_state,
			  msg->dev->sas_addr, msg->dev, msg->dev->dev_status);

	return ret;
}

/**
 * sal_check_dev_can_io - check device state,if state is active,then start io.
 * @msg: io infomation
 *
 */
inline s32 sal_check_dev_can_io(struct sal_dev * dev)
{
	s32 ret = ERROR;
	enum sal_dev_state dev_state = SAL_DEV_ST_BUTT;

	SAL_ASSERT(NULL != dev, return ERROR);
	dev_state = sal_get_dev_state(dev);

	if (SAL_DEV_ST_ACTIVE == dev_state)
		ret = OK;

	return ret;
}

s32 sal_queue_io_add_jam(struct sal_msg * msg)
{
	s32 ret = ERROR;

	ret = sal_add_io_to_jam(msg);
	if (OK == ret)
		SAL_TRACE(OSP_DBL_INFO, 788,
			  "Card:%d(flag:0x%x) dev:0x%llx(sal dev:%p ST:%d) is not active,add msg:%p(uni id:0x%llx,cdb[0]:0x%x,ST:%d) to jam OK",
			  msg->card->card_no, msg->card->flag,
			  msg->dev->sas_addr, msg->dev, msg->dev->dev_status,
			  msg, msg->ini_cmnd.unique_id, msg->ini_cmnd.cdb[0],
			  msg->msg_state);

	return ret;
}

/**
 * sal_msg_done - sas/sata io done callback function
 * @msg: io infomation
 *
 */
void sal_msg_done(struct sal_msg *msg)
{
	struct drv_ini_cmd ini_cmd;
	unsigned long flag = 0;

	SAL_ASSERT(NULL != msg, return);

	/*if io failure, then add io to jam*/
	if (unlikely(SAL_MSG_DRV_SUCCESS != msg->status.drv_resp))		
		if (OK == sal_done_io_add_jam(msg))
			return;

	//sal_drv_io_stat(msg);

	spin_lock_irqsave(&msg->msg_state_lock, flag);
	if (SAL_MSG_ST_SEND == msg->msg_state) {
		sal_msg_copy_to_ini_cmnd(msg, &ini_cmd);

		if (msg->status.result != SAL_HOST_STATUS(DID_OK))
			SAL_TRACE_LIMIT(OSP_DBL_MINOR, msecs_to_jiffies(3000),
					3, 555,
					"Msg:%p(Unique Id:0x%llx,CDB:0x%x,Htag:%d) to Dev:0x%llx(ST:%d) return to midlayer(result:0x%x)",
					msg, msg->ini_cmnd.unique_id,
					msg->ini_cmnd.cdb[0], msg->ll_tag,
					msg->dev->sas_addr,
					msg->dev->dev_status,
					msg->status.result);

		if (msg->ini_cmnd.done) {
			msg->ini_cmnd.done(&ini_cmd);
			/*to avoid io response message back repeat*/ 
			msg->ini_cmnd.done = NULL;
		} else {
			SAL_TRACE(OSP_DBL_MAJOR, 788,
				  "Card:%d msg:%p(uni id:0x%llx) to dev addr:0x%llx(sal dev:%p) done func is NULL",
				  msg->card->card_no, msg,
				  msg->ini_cmnd.unique_id, msg->dev->sas_addr,
				  msg->dev);
		}

		(void)sal_msg_state_chg_no_lock(msg, SAL_MSG_EVENT_DONE_MID);
	}
	spin_unlock_irqrestore(&msg->msg_state_lock, flag);

	(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_DONE_UNUSE);
	return;
}

/**
 * sal_ssp_done - ssp io done callback function
 * @msg: io infomation
 *
 */
void sal_ssp_done(struct sal_msg *msg)
{
	SAL_ASSERT(NULL != msg, return);

	sal_set_msg_result(msg);

	sal_msg_done(msg);
	return;
}

/**
 * sal_stp_done - stp io done callback function
 * @msg: io infomation
 *
 */
void sal_stp_done(struct sal_msg *msg)
{
	u32 sat_status = 0;

	SAL_ASSERT(NULL != msg, return);

	switch (msg->status.drv_resp) {
	case SAL_MSG_DRV_SUCCESS:
		sat_status = SAT_IO_SUCCESS;
		break;
	case SAL_MSG_DRV_FAILED:
		sat_status = SAT_IO_FAILED;
		break;
	case SAL_MSG_DRV_BUTT:
	case SAL_MSG_DRV_NO_DEV:
	case SAL_MSG_DRV_UNDERFLOW:
	case SAL_MSG_DRV_RETRY:
	default:
		SAL_TRACE(OSP_DBL_MAJOR, 789, "invalid io status:0x%x",
			  msg->status.drv_resp);
		sat_status = SAT_IO_FAILED;
		break;
	}

	sat_disk_exec_complete(&msg->protocol.stp, sat_status);
	return;
}

/**
 * sal_smp_done - smp io done callback function
 * @msg: io infomation
 *
 */
void sal_smp_done(struct sal_msg *msg)
{
	unsigned long flag = 0;

	SAL_ASSERT(NULL != msg, return);

	spin_lock_irqsave(&msg->msg_state_lock, flag);
	if (SAL_MSG_ST_ABORTING == msg->msg_state)
		/* aborting -> aborted */
		(void)sal_msg_state_chg_no_lock(msg, SAL_MSG_EVENT_DONE_UNUSE);

	spin_unlock_irqrestore(&msg->msg_state_lock, flag);

	SAS_COMPLETE(&msg->compl);
	return;
}

/**
 * sal_data_dir_to_pcie_dir - data transfer direction
 * @msg: io infomation
 *
 */
s32 sal_data_dir_to_pcie_dir(enum sal_msg_data_direction data_dir)
{
	s32 ret = DMA_NONE;

	switch (data_dir) {
	case SAL_MSG_DATA_TO_DEVICE:
		ret = DMA_TO_DEVICE;
		break;
	case SAL_MSG_DATA_FROM_DEVICE:
		ret = DMA_FROM_DEVICE;
		break;
	case SAL_MSG_DATA_NONE:
	case SAL_MSG_DATA_BUTT:
	case SAL_MSG_DATA_BIDIRECTIONAL:
	default:
		ret = DMA_NONE;
		break;
	}

	return ret;
}

EXPORT_SYMBOL(sal_data_dir_to_pcie_dir);


void sal_fill_ssp_iu(struct sal_msg *msg)
{
	struct sas_ssp_cmnd_uint *ssp_iu = NULL;

	SAL_ASSERT(NULL != msg, return);

	ssp_iu = &msg->protocol.ssp;

	memset(ssp_iu->lun, 0, DEFAULT_LUN_SIZE);
	memcpy(ssp_iu->cdb, msg->ini_cmnd.cdb, DEFAULT_CDB_LEN);
	ssp_iu->add_cdb_len = 0;

	switch (msg->ini_cmnd.tag) {
	case SIMPLE_QUEUE_TAG:
		ssp_iu->task_attr = SAL_IO_TASK_SIMPLE;
		break;
	case HEAD_OF_QUEUE_TAG:
		ssp_iu->task_attr = SAL_IO_TASK_HEAD_OF_QUEUE;
		break;
	case ORDERED_QUEUE_TAG:
		ssp_iu->task_attr = SAL_IO_TASK_ORDERED;
		break;
	default:
		ssp_iu->task_attr = SAL_IO_TASK_SIMPLE;
		break;
	}
}

void sal_set_msg_result(struct sal_msg *msg)
{
	SAL_ASSERT(NULL != msg, return);

	if (SAL_MSG_DRV_SUCCESS == msg->status.drv_resp) {
		if (SAM_STAT_GOOD == msg->status.io_resp)
			msg->status.result = SAL_HOST_STATUS(DID_OK);
		else
			/*check condition or busy or others */
			msg->status.result =
			    (s32) (SAL_HOST_STATUS(DID_OK) |
				   SAL_TGT_STATUS(msg->status.io_resp));
	} else if (SAL_MSG_DRV_NO_DEV == msg->status.drv_resp) {
		msg->status.result = SAL_HOST_STATUS(DID_NO_CONNECT);
	} else if (SAL_MSG_DRV_UNDERFLOW == msg->status.drv_resp) {
		if (SAL_IS_RW_CMND(msg->ini_cmnd.cdb[0]))
			msg->status.result = SAL_HOST_STATUS(DID_ERROR);
		else
			msg->status.result = SAL_HOST_STATUS(DID_OK);
	} else if (SAL_MSG_DRV_RETRY == msg->status.drv_resp) {
		msg->status.result = SAL_HOST_STATUS(DID_REQUEUE);
	} else {
		msg->status.result = SAL_HOST_STATUS(DID_ERROR);
	}
}


s32 sal_ssp_send(struct sal_msg *msg)
{
	enum sal_cmnd_exec_result ret = SAL_CMND_EXEC_FAILED;
	struct sal_card *card = NULL;

	SAL_ASSERT(NULL != msg, return ERROR);

	card = msg->card;

	msg->type = SAL_MSG_PROT_SSP;
	msg->done = sal_ssp_done;

	if (SAL_AND(sal_global.support_4k,
	     SAL_IS_MODE_CMND(msg->protocol.ssp.cdb[0]))) {
		SAL_TRACE_FRQLIMIT(OSP_DBL_MAJOR, msecs_to_jiffies(3000), 786,
				   "Card:%d block mode cmnd 0x%X",
				   card->card_no, msg->protocol.ssp.cdb[0]);
		msg->status.drv_resp = SAL_MSG_DRV_SUCCESS;
		msg->status.io_resp = SAM_STAT_GOOD;
		sal_set_msg_result(msg);
		sal_msg_done(msg);
		return OK;
	}

	if (NULL == card->ll_func.send_msg) {
		SAL_TRACE_FRQLIMIT(OSP_DBL_MAJOR, msecs_to_jiffies(3000), 788,
				   "Card:%d send_msg null", card->card_no);
		return ERROR;
	}

	ret = card->ll_func.send_msg(msg);
	if (SAL_CMND_EXEC_BUSY == ret) {
		msg->status.drv_resp = SAL_MSG_DRV_FAILED;

		sal_set_msg_result(msg);
		sal_msg_done(msg);
		return OK;
	} else if (SAL_CMND_EXEC_FAILED == ret) {
		msg->status.drv_resp = SAL_MSG_DRV_FAILED;

		sal_set_msg_result(msg);
		sal_msg_done(msg);
		return OK;
	} else {
		return OK;
	}
}

s32 sal_sat_send(struct sal_msg * msg)
{
	s32 ret = ERROR;

	SAL_ASSERT(NULL != msg, return ERROR);

	msg->protocol.stp.upper_msg = msg;
	msg->type = SAL_MSG_PROT_STP;

	msg->done = sal_stp_done;

	ret = sat_process_command(&msg->protocol.stp,
				  (struct sat_ini_cmnd *)(void *)(&msg->
								  ini_cmnd),
				  &msg->dev->sata_dev_data);
	/*SAT specific process */
	switch (ret) {
	case SAT_IO_SUCCESS:
		return OK;

	case SAT_IO_COMPLETE:
		sal_set_msg_result(msg);
		sal_msg_done(msg);
		return OK;

		/* Queue Full */
	case SAT_IO_QUEUE_FULL:
		return SCSI_MLQUEUE_DEVICE_BUSY;

	case SAT_IO_BUSY:
		return SCSI_MLQUEUE_HOST_BUSY;

	case SAT_IO_FAILED:
	default:
		msg->status.drv_resp = SAL_MSG_DRV_FAILED;
		sal_set_msg_result(msg);
		sal_msg_done(msg);
		return OK;
	}

}

/**
 * sal_simulate_io_result - simulate io failure handle
 * @msg: io infomation
 *
 */
s32 sal_simulate_io_result(struct sal_msg * msg)
{
	struct sal_io_dfx *io_dfx = NULL;
	struct sal_sense_payload sense_payload;
	u32 max_sense_len = SAL_MAX_SENSE_LEN;

	io_dfx = &(msg->dev->io_dfx);

	io_dfx->err_cnt--;

	SAL_TRACE(OSP_DBL_INFO, 2520,
		  "dev:0x%llx(scsi id:%d) sim one io error(type:0x%x),left num:%d",
		  msg->dev->sas_addr, msg->dev->scsi_id, io_dfx->io_err_type,
		  io_dfx->err_cnt);

	msg->status.result = (s32) SAL_HOST_STATUS(io_dfx->io_err_type);
	/*status.resp set success£¬aviod to do JAM */
	msg->status.drv_resp = SAL_MSG_DRV_SUCCESS;
	memset((void *)&sense_payload, 0, sizeof(struct sal_sense_payload));
	if (SAL_IO_NO_SENSE != io_dfx->sense_key) {
		sense_payload.sense_key = (u8) io_dfx->sense_key;
		sense_payload.add_sense_code = (u8) io_dfx->asc;
		sense_payload.sense_qual = (u8) io_dfx->ascq;
		memcpy((char *)&(msg->status.sense[0]),
		       (char *)&sense_payload,
		       MIN(max_sense_len,
			   (u32) sizeof(struct sal_sense_payload)));
	}

	sal_msg_done(msg);

	return OK;
}

#if 0 /* delete by chenqilin */
/**
 * sal_outer_io_stat - outer io statistic
 * @msg: io infomation
 *
 */
void sal_outer_io_stat(struct sal_msg *msg)
{
	sal_port_io_collect(msg);

	sal_disk_io_collect_stat(msg, SAL_IO_STAT_RESPONSE);

	return;
}

DRV_IOCNT_PORT_DIRECTION_E sal_msg_dir_to_frame_stat_dir(struct sal_msg * msg)
{
	DRV_IOCNT_PORT_DIRECTION_E ret = DRV_PORT_IOCNT_DIREC_BUTT;

	SAL_ASSERT(NULL != msg, return ret);

	switch (msg->data_direction) {
	case SAL_MSG_DATA_TO_DEVICE:
		ret = DRV_PORT_IOCNT_DIREC_SEND;
		break;
	case SAL_MSG_DATA_FROM_DEVICE:
		ret = DRV_PORT_IOCNT_DIREC_RECE;
		break;
	default:
	case SAL_MSG_DATA_BIDIRECTIONAL:
	case SAL_MSG_DATA_NONE:
	case SAL_MSG_DATA_BUTT:
		break;
	}

	if (SAL_MSG_DRV_SUCCESS != msg->status.drv_resp)
		ret = DRV_PORT_IOCNT_DIREC_THROW;

	return ret;
}

/**
 * sal_port_io_collect - collect port io
 * @msg: io infomation
 *
 */
void sal_port_io_collect(struct sal_msg *msg)
{
	DRV_IOCNT_PORT_EVENT_S data;

	SAL_ASSERT(NULL != msg, return);

	if (!DRV_IOCNT_PORT_IsStart())
		return;

	if (SAL_INVALID_LOGIC_PORTID == msg->dev->port->logic_id)
		return;

	memset(&data, 0, sizeof(DRV_IOCNT_PORT_EVENT_S));

	if (SAL_IS_READ_CMND(msg->ini_cmnd.cdb[0])
	    || SAL_IS_WRITE_CMND(msg->ini_cmnd.cdb[0])) {
		data.uiPortID = msg->dev->port->logic_id;
		data.eDirection = sal_msg_dir_to_frame_stat_dir(msg);
		data.ulIOSize = msg->ini_cmnd.data_len;
		data.ulRespTime =
		    (unsigned long)jiffies_to_msecs(jiffies - msg->send_time)
		    * (OSAL_PER_NS / 1000);
		(void)DRV_IOCNT_Port_Collect(&data);
	}

	return;
}

void sal_disk_io_collect_stat(struct sal_msg *msg, u8 type)
{
	DRV_IOCNT_DISK_EVENT_S data;

	SAL_ASSERT(NULL != msg, return);

	if (!DRV_IOCNT_DISK_IsStart())
		return;

	if (SAL_INVALID_LOGIC_PORTID == msg->dev->port->logic_id)
		return;

	memset(&data, 0, sizeof(DRV_IOCNT_DISK_EVENT_S));

	if (SAL_IS_READ_CMND(msg->ini_cmnd.cdb[0]))
		data.eCmdType = DRV_IOCNT_DISK_CMD_TYPE_READ;
	else if (SAL_IS_WRITE_CMND(msg->ini_cmnd.cdb[0]))
		data.eCmdType = DRV_IOCNT_DISK_CMD_TYPE_WRITE;
	else
		return;

	data.port_id = msg->dev->port->logic_id;
	data.host_id = msg->card->host_id;
	data.scsi_id = msg->dev->scsi_id;

	if (SAL_IO_STAT_REQUEST == type) {
		data.eIOType = DRV_IOCNT_DISK_IO_REQUEST;
		data.uiSectorNum = (u32) sal_comm_calc_sec_num(msg);
		data.uiIOSizeByte =
		    data.uiSectorNum * SAL_DISK_SECTOR_SIZE_512B;
		data.ulStartSector = (u32) sal_comm_calc_lba(msg);
		data.ulArriveTime = OSAL_CURRENT_TIME_NSEC;
	} else {
		data.eIOType = DRV_IOCNT_DISK_IO_FAIL;
		if ((SAL_MSG_DRV_SUCCESS == msg->status.drv_resp) &&
		    (SAM_STAT_GOOD == msg->status.io_resp))
			data.eIOType = DRV_IOCNT_DISK_IO_RESPONSE;

		data.ulResponseTime = OSAL_CURRENT_TIME_NSEC - msg->send_time;

	}

	(void)DRV_IOCNT_DISK_Collect(&data);

	return;
}
#endif 
/**
 * sal_start_io - start io,for ssp/stp io.
 * @msg: io infomation
 *
 */
s32 sal_start_io(struct sal_dev * dev, struct sal_msg * msg)
{
	s32 ret = ERROR;
	struct sal_card *card = NULL;

	/*1.active -> send; 2.in jam -> send; */
	(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_IOSTART);

	card = msg->card;

	if (unlikely(0 != dev->io_dfx.err_cnt))
		return sal_simulate_io_result(msg);

	if (SAL_DEV_TYPE_SSP == dev->dev_type) {
		sal_fill_ssp_iu(msg);
		ret = sal_ssp_send(msg);
	} else if (SAL_DEV_TYPE_STP == dev->dev_type) {
		ret = sal_sat_send(msg);
	} else {
		SAL_TRACE_FRQLIMIT(OSP_DBL_MAJOR, msecs_to_jiffies(3000), 818,
				   "card:%d dev:0x%llx type:%d not support",
				   card->card_no, dev->sas_addr, dev->dev_type);

		(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_SEND_FAIL);
		return ERROR;
	}

	if ((ERROR == ret)
	    || (SCSI_MLQUEUE_HOST_BUSY == ret)) {
		SAL_TRACE_LIMIT(OSP_DBL_MAJOR, msecs_to_jiffies(3000), 3, 819,
				"card:%d dev:0x%llx send IO failed / host busy,ret:0x%x",
				card->card_no, dev->sas_addr, ret);

		(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_SEND_FAIL);
		return SCSI_MLQUEUE_HOST_BUSY;
	} else if (SCSI_MLQUEUE_DEVICE_BUSY == ret) {
		SAL_TRACE_LIMIT(OSP_DBL_MAJOR, msecs_to_jiffies(3000), 3, 819,
				"card:%d dev:0x%llx busy", card->card_no,
				dev->sas_addr);
		
		(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_SEND_FAIL);
		return SCSI_MLQUEUE_DEVICE_BUSY;
	} else {
		return OK;
	}
}


s32 sal_queue_cmnd(struct drv_ini_cmd * ini_cmd)
{
	s32 ret = ERROR;
	struct sal_dev *dev = NULL;
	struct sal_card *sal_card = NULL;
	struct sal_msg *msg = NULL;
	enum sal_dev_state dev_state = SAL_DEV_ST_BUTT;
	struct drv_ini_private_info *drv_info = NULL;

#define SAL_PERFORMANCE_SWITCH 3

	SAL_ASSERT(NULL != ini_cmd, return ERROR);

	drv_info = (struct drv_ini_private_info *) ini_cmd->private_info;

	dev = (struct sal_dev *)drv_info->driver_data;
	if (NULL == dev) {
		SAL_TRACE_LIMIT(OSP_DBL_MINOR, msecs_to_jiffies(1000), 1, 555,
				"Dev is NULL(unique cmnd:0x%llx)",
				ini_cmd->cmd_sn);

		ini_cmd->result.status = SAL_HOST_STATUS(DID_ERROR);
		ini_cmd->done(ini_cmd);
		return OK;
	}

	sal_card = dev->card;
	if (NULL == sal_card) {
		SAL_TRACE_LIMIT(OSP_DBL_MINOR, msecs_to_jiffies(1000), 1, 555,
				"Card is NULL(unique cmnd:0x%llx,dev:0x%llx)",
				ini_cmd->cmd_sn, dev->sas_addr);

		ini_cmd->result.status = SAL_HOST_STATUS(DID_ERROR);
		ini_cmd->done(ini_cmd);
		return OK;
	}

	sal_get_dev(dev);

	if (unlikely(SAL_IS_CARD_NOT_ACTIVE(sal_card))) {
		SAL_TRACE_LIMIT(OSP_DBL_MINOR, msecs_to_jiffies(1000), 1, 555,
				"Card:%d isn't active now(flag:0x%x),let mid layer retry cmnd:0x%llx(cdb[0]:0x%x)",
				sal_card->card_no, sal_card->flag,
				ini_cmd->cmd_sn, ini_cmd->cdb[0]);

		ini_cmd->result.status = SAL_HOST_STATUS(DID_RESET);
		ini_cmd->done(ini_cmd);

		sal_put_dev(dev);
		return OK;
	}

	if (unlikely(sal_card->flag & SAL_CARD_SLOWNESS)) {
		SAL_TRACE_LIMIT(OSP_DBL_MINOR, msecs_to_jiffies(5000), 1, 555,
				"Card:%d is updating now(flag:0x%x),let mid-layer retry cmnd:0x%llx(cdb[0]:0x%x) later",
				sal_card->card_no, sal_card->flag,
				ini_cmd->cmd_sn, ini_cmd->cdb[0]);
		ini_cmd->result.status = SAL_HOST_STATUS(DID_REQUEUE);
		ini_cmd->done(ini_cmd);

		sal_put_dev(dev);
		return OK;
	}

	dev_state = dev->dev_status;
	if (unlikely((SAL_DEV_ST_RDY_FREE == dev_state)
		     || (SAL_DEV_ST_FREE == dev_state))) {
		SAL_TRACE_LIMIT(OSP_DBL_MINOR, msecs_to_jiffies(1000), 1, 555,
				"Card:%d dev:0x%llx(sal dev:%p) is offline(ST:%d),return cmnd:0x%llx(cdb[0]:0x%x)",
				sal_card->card_no, dev->sas_addr, dev,
				dev_state, ini_cmd->cmd_sn, ini_cmd->cdb[0]);

		ini_cmd->result.status = SAL_HOST_STATUS(DID_ERROR);
		ini_cmd->done(ini_cmd);

		sal_put_dev(dev);
		return OK;
	}

	if (sal_global.quick_done == SAL_PERFORMANCE_SWITCH) {
		SAL_TRACE_LIMIT(OSP_DBL_MINOR, msecs_to_jiffies(3000), 1, 822,
				"Card:%d(flag:0x%x) dev:0x%llx(ST:%d) performance test,quick done",
				sal_card->card_no, sal_card->flag,
				dev->sas_addr, dev->dev_status);

		ini_cmd->result.status = SAL_HOST_STATUS(DID_OK);
		ini_cmd->done(ini_cmd);

		sal_put_dev(dev);
		return OK;
	}

	msg = sal_get_msg(sal_card);
	if (unlikely(NULL == msg)) {
		SAL_TRACE_LIMIT(OSP_DBL_MAJOR, msecs_to_jiffies(1000), 1, 822,
				"Card:%d(flag:0x%x) dev:0x%llx get msg failed",
				sal_card->card_no, sal_card->flag,
				dev->sas_addr);
		sal_put_dev(dev);
		return SCSI_MLQUEUE_HOST_BUSY;
	}

	msg->dev = dev;

	/* copy scsi ini cmnd data to msg*/
	sal_msg_copy_from_ini_cmnd(msg, ini_cmd);

	sal_io_cnt_Sum(sal_card, SAL_QUEUE_FROM_SCSI);

	if (unlikely(SAL_DEV_ST_ACTIVE != dev->dev_status))
		if (OK == sal_queue_io_add_jam(msg)) {
			sal_put_dev(dev);
			return OK;
		} else {
			ini_cmd->result.status = (u32) msg->status.result;
			ini_cmd->done(ini_cmd);

			(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_IOSTART);
			(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_SEND_FAIL);

			sal_put_dev(dev);
			return OK;
		}

#if 0
	sal_disk_io_collect_stat(msg, SAL_IO_STAT_REQUEST);
#endif

	ret = sal_start_io(dev, msg);

	sal_put_dev(dev);
	return ret;
}

u32 sal_sg_copy_from_buffer(struct sal_msg * sal_msg, u8 * buff, u32 len)
{
	u32 i = 0;
	void *sgl_next = NULL;
	struct drv_sge *sge = NULL;
	u32 offset = 0;
	struct drv_sgl *sgl_used_in_driver = NULL;

	SAL_ASSERT(NULL != sal_msg->ini_cmnd.sgl, return 0);
	sgl_used_in_driver = drv_sgl_trans(sal_msg->ini_cmnd.sgl,
					     (void *)sal_msg->ini_cmnd.upper_cmd,
					     (u32) sal_msg->ini_cmnd.port_id,
					     &sal_msg->sgl_head);

	SAL_ASSERT(NULL != sgl_used_in_driver, return 0);

	/* find all sgl */
	for (;;) {
		for (i = 0; i < SAL_SGL_CNT(sgl_used_in_driver); i++) {
			/*find all sge in one sgl*/
			sge = &(sgl_used_in_driver->sge[i]);
			memcpy(sge->buff, buff + offset,
			       MIN(sge->len, len - offset));
			offset += MIN(sge->len, len - offset);
			if (0 == len - offset)
				return offset;
		}

		sgl_next = sgl_used_in_driver->next_sgl;
		if (NULL != sgl_next) {
			memset(&sal_msg->sgl_head, 0, sizeof(struct drv_sgl));

			sgl_used_in_driver = drv_sgl_trans(sgl_next,
							     (void *)sal_msg->ini_cmnd.upper_cmd,
							     (u32) sal_msg->ini_cmnd.port_id,
							     &sal_msg->sgl_head);
			SAL_ASSERT(NULL != sgl_used_in_driver, return 0);

			sgl_next = NULL;

		} else {
			break;
		}
	}

	return offset;
}

EXPORT_SYMBOL(sal_sg_copy_from_buffer);

u32 sal_sg_copy_to_buffer(struct sal_msg * sal_msg, u8 * buff, u32 len)
{
	u32 i = 0;
	void *sgl_next = NULL;
	struct drv_sge *sge = NULL;
	u32 offset = 0;
	struct drv_sgl *sgl_used_in_driver = NULL;

	SAL_ASSERT(NULL != sal_msg->ini_cmnd.sgl, return 0);

	sgl_used_in_driver = drv_sgl_trans(sal_msg->ini_cmnd.sgl,
					     (void *)sal_msg->ini_cmnd.upper_cmd,
					     (u32) sal_msg->ini_cmnd.port_id,
					     &sal_msg->sgl_head);
	SAL_ASSERT(NULL != sgl_used_in_driver, return 0);

	for (;;) {
		for (i = 0; i < SAL_SGL_CNT(sgl_used_in_driver); i++) {
			sge = &(sgl_used_in_driver->sge[i]);

			memcpy(buff + offset, sge->buff,
			       MIN(sge->len, len - offset));
			offset += MIN(sge->len, len - offset);

		}

		sgl_next = sgl_used_in_driver->next_sgl;
		if (NULL != sgl_next) {
			memset(&sal_msg->sgl_head, 0, sizeof(struct drv_sgl));

			sgl_used_in_driver = drv_sgl_trans(sgl_next,
							     (void *)sal_msg->ini_cmnd.upper_cmd,
							     (u32) sal_msg->ini_cmnd.port_id,
							     &sal_msg->sgl_head);
			SAL_ASSERT(NULL != sgl_used_in_driver, return 0);

			sgl_next = NULL;
		} else {
			break;
		}
	}

	return offset;
}

EXPORT_SYMBOL(sal_sg_copy_to_buffer);

