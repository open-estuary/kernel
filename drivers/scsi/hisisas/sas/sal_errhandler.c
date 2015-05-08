/*
 * Huawei Pv660/Hi1610 sas controller device process
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

//lint -e801

/*----------------------------------------------*
 * global variable type define.                                               *
 *----------------------------------------------*/
sal_err_io_handler_t sal_err_io_handler_table[SAL_MAX_ERRHANDLE_ACTION_NUM];
sal_err_io_handler_t sal_err_io_unhandler_table[SAL_MAX_ERRHANDLE_ACTION_NUM];	/*function which did not processed inside should be executed .  */

/*----------------------------------------------*
 * inner function type define.                                               *
 *----------------------------------------------*/
enum sal_eh_pre_abort sal_pre_abort_msg(struct sal_card *card, u64 unique_id,
					struct sal_msg **orgi_msg);
s32 sal_send_tm(struct sal_msg *tm_msg, struct sal_msg *sal_msg,
		enum sal_tm_type tm_type);

s32 sal_tm_single_io(struct sal_msg *orgi_msg, sal_eh_wait_tm_func_t wait_func);
void sal_iterate_list_and_mark(struct sal_card *sal_card,
			       struct sal_ll_notify *notify);
void sal_process_notify_event(struct sal_card *sal_card);
void sal_free_notify_event(struct sal_card *sal_card,
			   struct sal_ll_notify *notify);

/*----------------------------------------------*
 * outer function type define.                                               *
 *----------------------------------------------*/



/*----------------------------------------------*
 * enter into outer error handle, mutex area with inner error handle.          *
 *----------------------------------------------*/
s32 sal_enter_err_hand_mutex_area(struct sal_card *sal_card)
{
	u32 cnt = 0;
	s32 ret = ERROR;
	unsigned long flag = 0;

	/* outer want to enter into blue area,must be increased queue number 1 . */
	SAS_ATOMIC_INC(&sal_card->queue_mutex);
	/*judge mutex atom , 0: can execute ；1:need to wait. */
	for (cnt = 0; cnt < 100; cnt++) {

		spin_lock_irqsave(&sal_card->card_lock, flag);
		if (true == sal_card->eh_mutex_flag) {
			/*can enter into */
			sal_card->eh_mutex_flag = false;	/*avoid others enter into.  */
			ret = OK;
		}
		spin_unlock_irqrestore(&sal_card->card_lock, flag);

		if (OK == ret)
			/* waiting for critical area success.*/
			goto EXIT;

		/*wait for total 10s total and check per 100ms. */
		SAS_MSLEEP(100);
	}
	/* timeout*/

	SAL_TRACE(OSP_DBL_MAJOR, 71,
		  "Card:%d wait enter mutex area timeout. Queue mutex num:%d.",
		  sal_card->card_no, SAS_ATOMIC_READ(&sal_card->queue_mutex));
	ret = ERROR;

      EXIT:
	/*queue number decrease 1 */
	SAS_ATOMIC_DEC(&sal_card->queue_mutex);
	return ret;
}

/*----------------------------------------------*
 * leave outer error handle, mutex area with inner error handle.          *
 *----------------------------------------------*/
void sal_leave_err_hand_mutex_area(struct sal_card *sal_card)
{
	unsigned long flag = 0;

	spin_lock_irqsave(&sal_card->card_lock, flag);

	if (false == sal_card->eh_mutex_flag)
		sal_card->eh_mutex_flag = true;
	else
		SAL_TRACE(OSP_DBL_MAJOR, 71,
			  "Leave ErrHandler Mutex Area Error, MutexFlag:%d",
			  sal_card->eh_mutex_flag);

	spin_unlock_irqrestore(&sal_card->card_lock, flag);

	return;
}

/*----------------------------------------------*
 * lookup msg from error handle list, check if need to clear.          *
 *----------------------------------------------*/
enum sal_eh_pre_abort sal_pre_abort_msg_in_eh_notify(struct sal_card *card,
						     struct sal_msg *orgi_msg)
{
	enum sal_eh_pre_abort ret = SAL_PRE_ABORT_PASS;
	unsigned long notify_flag = 0;
	struct sal_ll_notify *notify = NULL;
	struct list_head *node = NULL;
	struct list_head *safe_node = NULL;
	struct list_head local_head;	/*local list, save msg from notify . */
	SAS_INIT_LIST_HEAD(&local_head);

	spin_lock_irqsave(&card->ll_notify_lock, notify_flag);
	SAS_LIST_FOR_EACH_SAFE(node, safe_node, &card->ll_notify_process_list) {
		notify =
		    SAS_LIST_ENTRY(node, struct sal_ll_notify, notify_list);
		if ((notify->op_code == SAL_ERR_HANDLE_ACTION_RESENDMSG)
		    && orgi_msg == notify->info) {
			/* Jaming inner */
			/* if the inner error handle is retrying, then it is cleared up by driver ,the function will return  SAL_PRE_ABORT_DONE and have finished process.*/
			SAS_LIST_DEL_INIT(&notify->notify_list);
			SAS_LIST_ADD_TAIL(&notify->notify_list, &local_head);
			ret = SAL_PRE_ABORT_DONE;	/*finished preprocess */
		} else
		    if ((notify->op_code == SAL_ERR_HANDLE_ACTION_HDRESET_RETRY)
			&& (orgi_msg->ini_cmnd.unique_id ==
			    notify->eh_info.unique_id)) {
			/* IO is in driver now. */
			/* IO is in driver,then it is deleted by driver，it means have finished process when the function return SAL_PRE_ABORT_DONE . */
            SAS_LIST_DEL_INIT(&notify->notify_list);
			SAS_LIST_ADD_TAIL(&notify->notify_list, &local_head);
			ret = SAL_PRE_ABORT_DONE;	/*finished preprocess */
		} else if ((notify->op_code == SAL_ERR_HANDLE_ACTION_LL_RETRY)
			   && (orgi_msg->ini_cmnd.unique_id ==
			       notify->eh_info.unique_id)) {
			/* IO is in driver now. */
			/* IO is in driver,then it is deleted by driver，it means have finished process when the function return SAL_PRE_ABORT_DONE . */
			SAS_LIST_DEL_INIT(&notify->notify_list);
			SAS_LIST_ADD_TAIL(&notify->notify_list, &local_head);
			ret = SAL_PRE_ABORT_DONE;	/*finished preprocess */
		} else
		    if ((notify->op_code == SAL_ERR_HANDLE_ACTION_ABORT_RETRY)
			&& (orgi_msg->ini_cmnd.unique_id ==
			    notify->eh_info.unique_id)) {
			/*IO may be in FW or driver, give up process inner and promise to process outer.*/
			SAS_LIST_DEL_INIT(&notify->notify_list);
			SAS_LIST_ADD_TAIL(&notify->notify_list, &local_head);
			ret = SAL_PRE_ABORT_PASS;	/* means it need to be go on processing abort outside. */
		}
	}
	spin_unlock_irqrestore(&card->ll_notify_lock, notify_flag);

	/* give up processing Notify inner, and replace into stLocalHead.*/
	SAS_LIST_FOR_EACH_SAFE(node, safe_node, &local_head) {
		notify =
		    SAS_LIST_ENTRY(node, struct sal_ll_notify, notify_list);
		SAS_LIST_DEL_INIT(&notify->notify_list);	/* pick up from local list.*/
		if (NULL != sal_err_io_unhandler_table[notify->op_code])
			(void)sal_err_io_unhandler_table[notify->
							 op_code] (notify->
								   info);
		/*After process, replace Notify event into free node. */
		sal_free_notify_event(card, notify);
		notify = NULL;
	}
	return ret;
}

/*----------------------------------------------*
 * lookup msg from active list, change state.          *
 *----------------------------------------------*/
struct sal_msg *sal_look_up_msg_and_st_chg(struct sal_card *card, u64 unique_id)
{
	unsigned long list_flag = 0;
	struct list_head *node = NULL;
	unsigned long state_flag = 0;
	struct sal_msg *orgi_msg = NULL;	//local
	u32 i = 0;

	for (i = 0; i < SAL_MAX_MSG_LIST_NUM; i++) {
		spin_lock_irqsave(&card->card_msg_list_lock[i], list_flag);
		SAS_LIST_FOR_EACH(node, &card->active_msg_list[i]) {
			orgi_msg =
			    SAS_LIST_ENTRY(node, struct sal_msg, host_list);
			if (unique_id == orgi_msg->ini_cmnd.unique_id) {
				spin_lock_irqsave(&orgi_msg->msg_state_lock,
						  state_flag);
				/*the IO did not return mid layer any more.*/
				orgi_msg->ini_cmnd.done = NULL;

				if ((SAL_MSG_ST_SEND == orgi_msg->msg_state)
				    || (SAL_MSG_ST_IN_JAM ==
					orgi_msg->msg_state)
				    || (SAL_MSG_ST_IN_ERRHANDING ==
					orgi_msg->msg_state)
				    || (SAL_MSG_ST_PENDING_IN_DRV ==
					orgi_msg->msg_state)
				    ) {
					SAL_TRACE(OSP_DBL_INFO, 827,
						  "Card:%d find msg:%p(unique Id:0x%llx,ST:%d) in node:%d",
						  card->card_no, orgi_msg,
						  orgi_msg->ini_cmnd.unique_id,
						  orgi_msg->msg_state, i);

					/*send -> aborting; in jam -> aborting; err hand -> aborting; pending -> aborting */
					(void)
					    sal_msg_state_chg_no_lock(orgi_msg,
								      SAL_MSG_EVENT_ABORT_START);

					spin_unlock_irqrestore(&orgi_msg->
							       msg_state_lock,
							       state_flag);
					spin_unlock_irqrestore(&card->
							       card_msg_list_lock
							       [i], list_flag);
					return orgi_msg;

				} else {
					/*IO abnormal */
					spin_unlock_irqrestore(&orgi_msg->
							       msg_state_lock,
							       state_flag);

					SAL_TRACE(OSP_DBL_MAJOR, 827,
						  "msg:%p(unique Id:0x%llx,ST:%d) is abnormal",
						  orgi_msg,
						  orgi_msg->ini_cmnd.unique_id,
						  orgi_msg->msg_state);
				}

			}
		}
		spin_unlock_irqrestore(&card->card_msg_list_lock[i], list_flag);
	}

	return NULL;
}

/*----------------------------------------------*
 * lookup msg from active list.          *
 *----------------------------------------------*/
void sal_search_msg_and_mark(struct sal_card *card, u64 unique_id)
{
	unsigned long list_flag = 0;
	unsigned long msg_flag = 0;
	struct list_head *node = NULL;
	struct sal_msg *orgi_msg = NULL;
	u32 i = 0;

	for (i = 0; i < SAL_MAX_MSG_LIST_NUM; i++) {
		spin_lock_irqsave(&card->card_msg_list_lock[i], list_flag);
		SAS_LIST_FOR_EACH(node, &card->active_msg_list[i]) {
			orgi_msg =
			    SAS_LIST_ENTRY(node, struct sal_msg, host_list);
			if (unique_id == orgi_msg->ini_cmnd.unique_id) {
				spin_lock_irqsave(&orgi_msg->msg_state_lock,
						  msg_flag);
				SAL_TRACE(OSP_DBL_INFO, 827,
					  "Card:%d mark msg:%p(Unique Id:0x%llx),it never return to mid layer",
					  card->card_no, orgi_msg, unique_id);

				orgi_msg->ini_cmnd.done = NULL;
				spin_unlock_irqrestore(&orgi_msg->
						       msg_state_lock,
						       msg_flag);
			}
		}
		spin_unlock_irqrestore(&card->card_msg_list_lock[i], list_flag);
	}
	return;
}

/*----------------------------------------------*
 * lookup SAL msg by abort.          *
 *----------------------------------------------*/
enum sal_eh_pre_abort sal_pre_abort_msg(struct sal_card *card, u64 unique_id,
					struct sal_msg **ret_orgi_msg)
{
	struct sal_msg *orgi_msg = NULL;
	enum sal_eh_pre_abort ret = SAL_PRE_ABORT_BUTT;

	SAL_ASSERT(NULL != card, return SAL_PRE_ABORT_FAIL);

	/* block inner error handle until inner error handle is over. */
	if (OK != sal_enter_err_hand_mutex_area(card)) {

		sal_search_msg_and_mark(card, unique_id);

		return SAL_PRE_ABORT_FAIL;
	}
	/* lookup raw msg , change state. */
	orgi_msg = sal_look_up_msg_and_st_chg(card, unique_id);
	if (NULL == orgi_msg) {
		/* didn't  find ,return handle success, and it didn't need to abort outside. */
		SAL_TRACE(OSP_DBL_INFO, 827,
			  "unique Id:0x%llx not found in card:%d", unique_id,
			  card->card_no);
		sal_leave_err_hand_mutex_area(card);
		return SAL_PRE_ABORT_NOT_FOUND;
	}
	/* delete inner error handle which is related with the current msg .*/
	ret = sal_pre_abort_msg_in_eh_notify(card, orgi_msg);
	if (SAL_PRE_ABORT_DONE == ret)
		SAL_TRACE(OSP_DBL_INFO, 796,
			  "msg:%p(unique id:0x%llx) is already in drv, so not need to process it",
			  orgi_msg, SAL_MSG_UNIID(orgi_msg));

	*ret_orgi_msg = orgi_msg;

	/*release from block inner error handle. */
	sal_leave_err_hand_mutex_area(card);
	return ret;
}

/*----------------------------------------------*
 * The function is used to end processing task management command.          *
 *----------------------------------------------*/
void sal_tm_done(struct sal_msg *sal_msg)
{
	SAL_ASSERT(NULL != sal_msg, return);

	SAL_TRACE(OSP_DBL_INFO, 796, "TM msg:%p(ST:%d) to dev:0x%llx come back",
		  sal_msg, sal_msg->msg_state, sal_msg->dev->sas_addr);
	SAS_COMPLETE(&sal_msg->compl);
	return;
}

/*----------------------------------------------*
 * The entrance to task manage command.          *
 *----------------------------------------------*/
s32 sal_send_tm(struct sal_msg * tm_msg,
		struct sal_msg * sal_msg, enum sal_tm_type tm_type)
{
	s32 ret = ERROR;
	struct sal_dev *dev = NULL;
	struct sal_card *card = NULL;

	dev = sal_msg->dev;
	SAL_ASSERT(NULL != dev, return ERROR);
	card = dev->card;
	SAL_ASSERT(NULL != card, return ERROR);

	tm_msg->related_msg = sal_msg;
	tm_msg->dev = dev;
	tm_msg->tm_func = tm_type;
	tm_msg->data_direction = SAL_MSG_DATA_NONE;
	if (SAL_DEV_TYPE_SSP == dev->dev_type) {
		tm_msg->type = SAL_MSG_PROT_SSP;
		tm_msg->done = sal_tm_done;
		if (NULL != card->ll_func.send_tm) {

			ret = card->ll_func.send_tm(tm_type, tm_msg);
			return ret;
		}

		return ERROR;
	} else if (SAL_DEV_TYPE_STP == dev->dev_type) {
		tm_msg->type = SAL_MSG_PROT_STP;
		tm_msg->protocol.stp.upper_msg = tm_msg;
		ret =
		    sat_abort_task_mgmt(&tm_msg->protocol.stp,
					&sal_msg->protocol.stp);
		return ret;
	} else {
		SAL_TRACE(OSP_DBL_MINOR, 796,
			  "Task manage,Device:0x%llx type:%d is incorrect",
			  dev->sas_addr, dev->dev_type);
		return ERROR;
	}
}

/*----------------------------------------------*
 * The entrance to task manage command.          *
 *----------------------------------------------*/
void sal_abort_single_io_done(struct sal_msg *sal_msg)
{
	SAL_ASSERT(NULL != sal_msg, return);
	SAL_ASSERT(sal_msg->msg_state != SAL_MSG_ST_FREE, return);

	SAL_TRACE(OSP_DBL_INFO, 796,
		  "Msg:%p(ST:%d,related msg:%p) to dev:0x%llx abort single io done",
		  sal_msg, sal_msg->msg_state, sal_msg->related_msg,
		  sal_msg->dev->sas_addr);
	/*wake up waiting */
	SAS_COMPLETE(&sal_msg->compl);
	return;
}

/*----------------------------------------------*
 * send abort LL/FW single IO command, didn't change v_pstOrgMsg state.    *
 *----------------------------------------------*/
s32 sal_abort_single_io(struct sal_msg * orginal_msg)
{
	s32 ret = ERROR;
	enum sal_cmnd_exec_result cmd_ret = SAL_CMND_EXEC_FAILED;	/*return value of called function.*/
	struct sal_dev *dev = 0;
	struct sal_card *card = NULL;
	struct sal_msg *msg = NULL;
	unsigned long dly_time = 0;

	SAL_ASSERT(NULL != orginal_msg, return ERROR);
	SAL_ASSERT(NULL != orginal_msg->dev, return ERROR);

	dev = orginal_msg->dev;
	card = dev->card;
	dly_time = sal_global.dly_time[card->card_no];

	msg = sal_get_msg(card);
	if (NULL == msg) {
		SAL_TRACE(OSP_DBL_MAJOR, 797, "Card:%d get req failed",
			  card->card_no);
		return ERROR;
	}

	(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_SMP_START);

	msg->dev = dev;
	msg->related_msg = orginal_msg;
	msg->done = sal_abort_single_io_done;

	if (NULL == card->ll_func.eh_abort_op) {
		SAL_TRACE(OSP_DBL_MINOR, 798,
			  "Card:%d pfnAbortMsg func is NULL", card->card_no);
		(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_SMP_SEND_FAIL);

		return ERROR;
	}

	cmd_ret = card->ll_func.eh_abort_op(SAL_EH_ABORT_OPT_SINGLE, msg);	/* Quark_EhAbortOperation */
	if (cmd_ret != SAL_CMND_EXEC_SUCCESS) {
		SAL_TRACE(OSP_DBL_MINOR, 798,
			  "Card:%d abort org msg:%p(unique id:0x%llx) failed,Result:%d",
			  card->card_no, orginal_msg,
			  orginal_msg->ini_cmnd.unique_id, cmd_ret);
		(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_SMP_SEND_FAIL);

		return ERROR;
	}
	/* waiting until abort iomb return, failed when it is timeout. */ 
	if (!SAS_WAIT_FOR_COMPLETION_TIME_OUT(&msg->compl,
					      msecs_to_jiffies
					      (SAL_ABORT_FW_TIMEOUT) +
					      dly_time)) {
		/* wait for abort fw until fail. */
		SAL_TRACE(OSP_DBL_MAJOR, 810,
			  "Card:%d dev:0x%llx abort org msg:%p(unique id:0x%llx,Htag:%d,ST:%d) in fw timeout",
			  card->card_no, dev->sas_addr, orginal_msg,
			  orginal_msg->ini_cmnd.unique_id, orginal_msg->ll_tag,
			  orginal_msg->msg_state);
		(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_SMP_WAIT_FAIL);

		return ERROR;
	}

	(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_SMP_WAIT_OK);

	/* waiting return IO which is abort. */
	ret =
	    sal_wait_msg_state(orginal_msg, SAL_ABORT_TIMEOUT,
			       SAL_MSG_ST_ABORTED);
	if (ERROR == ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 814,
			  "Card:%d wait org msg:%p(unique id:0x%llx,Htag:%d,ST:%d) back timeout",
			  card->card_no, orginal_msg,
			  orginal_msg->ini_cmnd.unique_id, orginal_msg->ll_tag,
			  orginal_msg->msg_state);
		return ERROR;
	}

	return OK;
}

/*----------------------------------------------*
 * clear notify in error handle list.    *
 *----------------------------------------------*/
void sal_clear_notify_in_err_handler_list(struct sal_card *sal_card,
					  struct sal_msg *sal_msg)
{
	unsigned long flag = 0;
	struct list_head *node = NULL;
	struct list_head *safe_node = NULL;
	struct sal_ll_notify *notify = NULL;

	spin_lock_irqsave(&sal_card->ll_notify_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(node, safe_node,
			       &sal_card->ll_notify_process_list) {
		notify =
		    SAS_LIST_ENTRY(node, struct sal_ll_notify, notify_list);
		if (sal_msg == notify->info) {
			/* didn't handle the error. */
			SAL_TRACE(OSP_DBL_INFO, 808,
				  "Card:%d ignore msg:%p(Htag:%d ST:%d),not handle it",
				  sal_card->card_no, sal_msg, sal_msg->ll_tag,
				  sal_msg->msg_state);

			notify->process_flag = false;
		}
	}
	spin_unlock_irqrestore(&sal_card->ll_notify_lock, flag);

	return;
}

/*----------------------------------------------*
 * task manage command state change.    *
 *----------------------------------------------*/
void sal_tm_cmd_st_change(struct sal_msg *tm_msg)
{
	unsigned long flag = 0;
	struct sal_card *sal_card = NULL;

	sal_card = tm_msg->card;
	SAL_ASSERT(NULL != sal_card, return);

	spin_lock_irqsave(&tm_msg->msg_state_lock, flag);
	if (SAL_MSG_ST_SEND == tm_msg->msg_state) {
		/*send -> tmo */
		(void)sal_msg_state_chg_no_lock(tm_msg,
						SAL_MSG_EVENT_SMP_WAIT_FAIL);

		spin_unlock_irqrestore(&tm_msg->msg_state_lock, flag);
	} else if (SAL_MSG_ST_IN_ERRHANDING == tm_msg->msg_state) {
		spin_unlock_irqrestore(&tm_msg->msg_state_lock, flag);

		/*didn't need to error handle for the IO (have been in critical area) */
		sal_clear_notify_in_err_handler_list(sal_card, tm_msg);
	} else {
		spin_unlock_irqrestore(&tm_msg->msg_state_lock, flag);

		SAL_TRACE(OSP_DBL_MAJOR, 808,
			  "Card:%d TM cmd:%p(ST:%d,Htag:%d) is abnormal",
			  sal_card->card_no, tm_msg, tm_msg->msg_state,
			  tm_msg->ll_tag);
	}

	return;
}

/*----------------------------------------------*
 * send abort LL/FW single IO command, didn't change v_pstOrgMsg state.    *
 *----------------------------------------------*/
s32 sal_tm_single_io(struct sal_msg * orgi_msg, sal_eh_wait_tm_func_t wait_func)
{
	struct sal_msg *tm_msg = NULL;
	struct sal_card *card = NULL;
	u64 unique_id = 0;
	s32 ret = ERROR;
	unsigned long card_flag = 0;
	enum sal_eh_wait_func_ret wait_smp_ret = SAL_EH_WAITFUNC_RET_TIMEOUT;

	SAL_REF(unique_id);	/*lint */
	SAL_ASSERT(NULL != orgi_msg, return ERROR);
	SAL_ASSERT(NULL != orgi_msg->card, return ERROR);
	SAL_ASSERT(NULL != wait_func, return ERROR);

	card = orgi_msg->card;
	unique_id = orgi_msg->ini_cmnd.unique_id;

	tm_msg = sal_get_msg(card);
	if (NULL == tm_msg) {
		SAL_TRACE(OSP_DBL_MAJOR, 808, "card:%d get msg failed",
			  card->card_no);
		return ERROR;
	}

	(void)sal_msg_state_chg(tm_msg, SAL_MSG_EVENT_SMP_START);
	/* abort IO info of maintaince in disk.  */
	ret = sal_send_tm(tm_msg, orgi_msg, SAL_TM_ABORT_TASK);
	if (OK != ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 809,
			  "card:%d send TM(tm msg:%p) to abort org msg:%p(uni id:0x%llx,ST:%d) failed",
			  card->card_no, tm_msg, orgi_msg, unique_id,
			  orgi_msg->msg_state);
		(void)sal_msg_state_chg(tm_msg, SAL_MSG_EVENT_SMP_SEND_FAIL);
		tm_msg = NULL;
		return ERROR;
	}
	/*start waiting TM return */
	wait_smp_ret = wait_func(tm_msg);	/* sal_default_tm_wait_func  or sal_inner_tm_wait_func  */
	if (SAL_EH_WAITFUNC_RET_TIMEOUT == wait_smp_ret || SAL_EH_WAITFUNC_RET_ABORT == wait_smp_ret) {	/*wait for tm response until fail */
		SAL_TRACE(OSP_DBL_MAJOR, 810,
			  "Card:%d abort disk:0x%llx org msg:%p(unique id:0x%llx),TM cmd:%p %s",
			  card->card_no, orgi_msg->dev->sas_addr, orgi_msg,
			  unique_id, tm_msg,
			  (wait_smp_ret ==
			   SAL_EH_WAITFUNC_RET_TIMEOUT ? "timeout" :
			   "aborted"));


		spin_lock_irqsave(&card->card_lock, card_flag);
		if ((SAL_CARD_ACTIVE & card->flag)
		    && (false == tm_msg->transmit_mark)
		    && (SAL_EH_WAITFUNC_RET_TIMEOUT == wait_smp_ret)) {
			/* terminate ahead of schedule, didn't reset host. */
			SAL_TRACE(OSP_DBL_INFO, 810,
				  "Card:%d is abnormal,need to reset it",
				  card->card_no);
			(void)sal_add_card_to_err_handler(card,
							  SAL_ERR_HANDLE_ACTION_RESET_HOST);
		}
		spin_unlock_irqrestore(&card->card_lock, card_flag);

		sal_tm_cmd_st_change(tm_msg);

		return ERROR;
	}

	if (tm_msg->tag_not_found) {
		SAL_TRACE(OSP_DBL_INFO, 810,
			  "Card:%d abort msg:%p(unique id:%llx,ST:%d) in disk:0x%llx OK(TM tag not found)",
			  card->card_no, orgi_msg, unique_id,
			  orgi_msg->msg_state, orgi_msg->dev->sas_addr);

		(void)sal_msg_state_chg(tm_msg, SAL_MSG_EVENT_SMP_WAIT_OK);
		/* special handle */
		return SAL_RETURN_MID_STATUS;
	}

	/* check result of TM processing. */
	if ((tm_msg->status.drv_resp != SAL_MSG_DRV_SUCCESS)
	    || ((tm_msg->status.io_resp != SAL_TM_FUNCTION_COMPLETE) && (tm_msg->status.io_resp != SAL_TM_FUNCTION_SUCCEEDED))) {	/*tm response 失败 */
		SAL_TRACE(OSP_DBL_MAJOR, 233,
			  "Card:%d dev:0x%llx abort msg:%p(unique id:%llx) failed(TM Drv Rsp:0x%x,IO Rsp:0x%x)",
			  card->card_no,
			  orgi_msg->dev->sas_addr,
			  orgi_msg,
			  unique_id,
			  tm_msg->status.drv_resp, tm_msg->status.io_resp);
		(void)sal_msg_state_chg(tm_msg, SAL_MSG_EVENT_SMP_WAIT_OK);

		return ERROR;
	}

	(void)sal_msg_state_chg(tm_msg, SAL_MSG_EVENT_SMP_WAIT_OK);

	/* ------------------------------- TM end.  */
	return OK;

}

/*----------------------------------------------*
 * The function of middle IO timeout error handle .    *
 *----------------------------------------------*/
s32 sal_eh_abort_cmnd_process(void *argv_in, void *argv_out)
{
	u64 unique_id = 0;
	struct sal_card *card = NULL;
	struct sal_msg *orgi_msg = NULL;
	struct sal_eh_cmnd *eh_cmnd = NULL;
	enum sal_eh_pre_abort pre_abort_result = SAL_PRE_ABORT_BUTT;
	unsigned long card_lock_flag = 0;
	enum sal_dev_state dev_state = SAL_DEV_ST_BUTT;
	s32 ret = ERROR;

	SAL_REF(argv_out);

	eh_cmnd = (struct sal_eh_cmnd *)argv_in;
	SAL_ASSERT(NULL != eh_cmnd, return ERROR);

	unique_id = eh_cmnd->unique_id;

	card = sal_get_card(eh_cmnd->card_id);
	if (NULL == card) {
		SAL_TRACE(OSP_DBL_MAJOR, 308, "Get card failed by card id:%d",
			  eh_cmnd->card_id);
		return FAILED;
	}

	pre_abort_result = sal_pre_abort_msg(card, unique_id, &orgi_msg);
	if (unlikely(SAL_PRE_ABORT_DONE == pre_abort_result)) {

		sal_put_card(card);

		(void)sal_msg_state_chg(orgi_msg, SAL_MSG_EVENT_WAIT_OK);
		return SUCCESS;
	} else if (unlikely(SAL_PRE_ABORT_NOT_FOUND == pre_abort_result)) {

		sal_put_card(card);
		return SUCCESS;
	} else if (unlikely(SAL_PRE_ABORT_FAIL == pre_abort_result)) {

		sal_put_card(card);
		return FAILED;
	}

	SAL_ASSERT(pre_abort_result == SAL_PRE_ABORT_PASS, return FAILED);	/* finished preprocess  */
	SAL_ASSERT(NULL != orgi_msg, return FAILED);	

	SAL_ASSERT(NULL != orgi_msg->dev, return FAILED);

	SAL_TRACE(OSP_DBL_INFO, 827,
		  "Card:%d dev:0x%llx(scsi id:%d) on slot:%d is going to abort msg:%p(unique id:0x%llx,CDB[0]:0x%x,Htag:%d),time:%dms",
		  eh_cmnd->card_id, orgi_msg->dev->sas_addr, eh_cmnd->scsi_id,
		  orgi_msg->dev->slot_id, orgi_msg, unique_id,
		  orgi_msg->ini_cmnd.cdb[0], orgi_msg->ll_tag,
		  jiffies_to_msecs(jiffies - orgi_msg->send_time));


	spin_lock_irqsave(&card->card_lock, card_lock_flag);
	if (!SAL_IS_CARD_READY(card)) {
		spin_unlock_irqrestore(&card->card_lock, card_lock_flag);
		/* card exception. */
		SAL_TRACE(OSP_DBL_MAJOR, 910, "card:%d is not ready,flag:0x%x",
			  card->card_no, card->flag);

		(void)sal_msg_state_chg(orgi_msg, SAL_MSG_EVENT_WAIT_FAIL);

		sal_put_card(card);
		return FAILED;
	}
	spin_unlock_irqrestore(&card->card_lock, card_lock_flag);

	dev_state = sal_get_dev_state(orgi_msg->dev);
	if (SAL_DEV_ST_ACTIVE != dev_state) {
		SAL_TRACE(OSP_DBL_MAJOR, 910,
			  "Card:%d dev:0x%llx is not active(ST:0x%x)",
			  card->card_no, orgi_msg->dev->sas_addr, dev_state);

		(void)sal_msg_state_chg(orgi_msg, SAL_MSG_EVENT_WAIT_FAIL);

		sal_put_card(card);
		return FAILED;
	}

	sal_io_cnt_Sum(card, SAL_EHABORT_FREOM_SCSI);	/* error count*/

	/* ---------------  TM start ---------------- */
	ret = sal_tm_single_io(orgi_msg, sal_default_tm_wait_func);
	if (ERROR == ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 1233,
			  "Card:%d abort org msg:%p(unique id:0x%llx) in disk failed",
			  card->card_no, orgi_msg, unique_id);

		(void)sal_msg_state_chg(orgi_msg, SAL_MSG_EVENT_WAIT_FAIL);

		sal_put_card(card);
		return FAILED;
	} else if (SAL_RETURN_MID_STATUS == ret) {
		/*IO return ahead of shedule.*/
		ret =
		    sal_wait_msg_state(orgi_msg, SAL_ABORT_TIMEOUT,
				       SAL_MSG_ST_ABORTED);
		if (OK == ret) {
			SAL_TRACE(OSP_DBL_INFO, 1233,
				  "Card:%d org msg:%p(unique id:0x%llx ST:%d) already come back",
				  card->card_no, orgi_msg, unique_id,
				  orgi_msg->msg_state);
			(void)sal_msg_state_chg(orgi_msg,
						SAL_MSG_EVENT_WAIT_OK);
		}

		sal_put_card(card);
		return SUCCESS;
	}

	/* -------------  TM end------------------  */

	/* ------------Abort FW start---------------  */
	if (OK != sal_abort_single_io(orgi_msg)) {
		/* abort IO info of maintaince in FW.  */
		SAL_TRACE(OSP_DBL_MAJOR, 1233,
			  "Card:%d abort msg:%p(unique id:0x%llx) in fw failed",
			  card->card_no, orgi_msg, unique_id);

		/* aborting - > time out */
		(void)sal_msg_state_chg(orgi_msg, SAL_MSG_EVENT_WAIT_FAIL);

		sal_put_card(card);
		return FAILED;
	}
	/* -----------------Abort FW end --------------  */

	SAL_TRACE(OSP_DBL_INFO, 815,
		  "Card:%d abort msg:%p(unique id:0x%llx) succeed",
		  card->card_no, orgi_msg, unique_id);

	/*aborted -> free */
	(void)sal_msg_state_chg(orgi_msg, SAL_MSG_EVENT_WAIT_OK);

	sal_put_card(card);
	return SUCCESS;
}

/*----------------------------------------------*
 * Error handle abort command.    *
 *----------------------------------------------*/
s32 sal_eh_abort_command(struct drv_ini_cmd * ini_cmd)
{
	s32 ret = FAILED;
	struct sal_dev *dev = NULL;
	struct sal_eh_cmnd eh_cmnd;
	struct drv_ini_private_info *drv_info = NULL;

	SAL_ASSERT(NULL != ini_cmd, return FAILED);

	drv_info = (struct drv_ini_private_info *) ini_cmd->private_info;

	dev = (struct sal_dev *)drv_info->driver_data;	/* after Slave destroy，release dev. */
	if (NULL == dev) {
		SAL_TRACE(OSP_DBL_MAJOR, 827,
			  "Dev is NULL(unique number:0x%llx)", ini_cmd->cmd_sn);
		return FAILED;
	}

	if (NULL == dev->card) {
		SAL_TRACE(OSP_DBL_MAJOR, 827,
			  "Card is NULL,cmnd unique number:0x%llx Dev addr:%llx(sal dev:%p)",
			  ini_cmd->cmd_sn, dev->sas_addr, dev);

		return FAILED;
	}

	sal_get_dev(dev);	/* add reference. */

	memset(&eh_cmnd, 0, sizeof(struct sal_eh_cmnd));

	eh_cmnd.card_id = dev->card->card_no;
	eh_cmnd.unique_id = ini_cmd->cmd_sn;
	eh_cmnd.scsi_id = dev->scsi_id;

	SAL_TRACE(OSP_DBL_INFO, 827,
		  "=== Card:%d dev addr:0x%llx(sal dev:%p,scsi id:%d) is going to abort unique id:0x%llx by mid layer===",
		  eh_cmnd.card_id, dev->sas_addr, dev, eh_cmnd.scsi_id,
		  eh_cmnd.unique_id);


	if (103 == sal_simulate_io_fatal) {
	
		SAL_TRACE(OSP_DBL_MAJOR, 1,
			  "sal_simulate_io_fatal change from %d to 0, make abort fail",
			  sal_simulate_io_fatal);
		sal_simulate_io_fatal = 0;
		return SAL_CMND_EXEC_FAILED;
	}

	ret = sal_eh_abort_cmnd_process(&eh_cmnd, NULL);

	sal_put_dev(dev);
	return ret;
}

/*----------------------------------------------*
 * hard reset .    *
 *----------------------------------------------*/
s32 sal_hard_reset_smp(struct sal_dev * sal_dev,
		       struct disc_smp_req * smp_req,
		       u32 req_len,
		       struct disc_smp_req * smp_req_rsp,
		       u32 resp_len, sal_eh_wait_smp_func_t wait_func)
{
	s32 ret = ERROR;
	struct sal_dev *exp_dev = NULL;

	smp_req->smp_frame_type = SMP_REQUEST;
	smp_req->smp_function = SMP_PHY_CONTROL;
	smp_req->resp_len = 0;
	smp_req->req_len = 0;

	smp_req->req_type.phy_control.phy_identifier = (u8) sal_dev->slot_id;
	smp_req->req_type.phy_control.phy_operation = PHY_HARD_RESET;

	/*look for father equipment :expander */
	exp_dev =
	    sal_get_dev_by_sas_addr(sal_dev->port, sal_dev->father_sas_addr);
	if (NULL == exp_dev) {
		SAL_TRACE(OSP_DBL_MAJOR, 895,
			  "Port:%d can't find exp dev:0x%llx",
			  sal_dev->port->port_id, sal_dev->father_sas_addr);
		return ERROR;
	}

	/*judge equipment type, ，send SMP while it is expander. */
	if (SAL_DEV_TYPE_SMP != exp_dev->dev_type) {
		SAL_TRACE(OSP_DBL_MAJOR, 899,
			  "Dev:0x%llx is not smp dev(type:%d)",
			  sal_dev->father_sas_addr, exp_dev->dev_type);

		sal_put_dev(exp_dev);
		return ERROR;
	}

	if (SAL_DEV_ST_ACTIVE != exp_dev->dev_status) {
		SAL_TRACE(OSP_DBL_MAJOR, 896,
			  "Exp Dev:0x%llx is not Active(ST:%d)",
			  exp_dev->sas_addr, exp_dev->dev_type);
		sal_put_dev(exp_dev);
		return ERROR;
	}


	SAL_TRACE(OSP_DBL_INFO, 897, "it is going to hardreset dev:0x%llx",
		  exp_dev->sas_addr);
	ret =
	    sal_send_smp(exp_dev, (u8 *) smp_req, req_len, (u8 *) smp_req_rsp,
			 resp_len, wait_func);

	sal_put_dev(exp_dev);
	return ret;
}

/*----------------------------------------------*
 * link reset .    *
 *----------------------------------------------*/
s32 sal_link_reset_smp(struct sal_dev * sal_dev,
		       struct disc_smp_req * smp_req,
		       u32 req_len,
		       struct disc_smp_req * smp_req_rsp,
		       u32 resp_len, sal_eh_wait_smp_func_t wait_func)
{
	s32 ret = ERROR;
	struct sal_dev *exp_dev = NULL;

	sal_dev->sata_dev_data.sata_state = SATA_DEV_STATUS_INRECOVERY;

	smp_req->smp_frame_type = SMP_REQUEST;
	smp_req->smp_function = SMP_PHY_CONTROL;
	smp_req->resp_len = 0;
	smp_req->req_len = 0;

	smp_req->req_type.phy_control.phy_identifier = (u8) sal_dev->slot_id;
	smp_req->req_type.phy_control.phy_operation = PHY_LINK_RESET;

	/*look for father equipment :expander */
	exp_dev =
	    sal_get_dev_by_sas_addr(sal_dev->port, sal_dev->father_sas_addr);
	if (NULL == exp_dev) {
		SAL_TRACE(OSP_DBL_MAJOR, 895,
			  "Port:%d can't find exp dev:0x%llx",
			  sal_dev->port->port_id, sal_dev->father_sas_addr);

		return ERROR;
	}

	/*judge equipment type, ，send SMP while it is expander. */
	if (SAL_DEV_TYPE_SMP != exp_dev->dev_type) {
		SAL_TRACE(OSP_DBL_MAJOR, 899,
			  "Dev:0x%llx is not smp dev(type:%d)",
			  sal_dev->father_sas_addr, exp_dev->dev_type);
		sal_put_dev(exp_dev);
		return ERROR;
	}


	if (SAL_DEV_ST_ACTIVE != exp_dev->dev_status) {
		SAL_TRACE(OSP_DBL_MAJOR, 896,
			  "Exp Dev:0x%llx is not Active(ST:%d)",
			  exp_dev->sas_addr, exp_dev->dev_type);
		sal_put_dev(exp_dev);
		return ERROR;
	}


	SAL_TRACE(OSP_DBL_INFO, 900, "it is going to linkreset dev:0x%llx",
		  exp_dev->sas_addr);
	ret =
	    sal_send_smp(exp_dev, (u8 *) smp_req, req_len, (u8 *) smp_req_rsp,
			 resp_len, wait_func);
	if (OK == ret)
		/*restore the SATA device to normal state. */
		sal_dev->sata_dev_data.sata_state = SATA_DEV_STATUS_NORMAL;

	sal_put_dev(exp_dev);
	return ret;
}

/*----------------------------------------------*
 *  convert outer reset type into necessary reset type .    *
 *----------------------------------------------*/
enum sal_eh_dev_rst_type sal_get_dev_reset_type(struct sal_dev *sal_dev,
						enum sal_eh_dev_rst_type
						reset_type)
{
	enum sal_eh_dev_rst_type ret = SAL_EH_RESET_BUTT;

	if (SAL_EH_RESET_BY_DEVTYPE == reset_type) {
		/*judge reset by device type. */
		if (SAL_DEV_TYPE_SSP == sal_dev->dev_type)
			/*HARD RESET */
			ret = SAL_EH_DEV_HARD_RESET;
		else if (SAL_DEV_TYPE_STP == sal_dev->dev_type)
			/*LINK RESET */
			ret = SAL_EH_DEV_LINK_RESET;
	} else {
		ret = reset_type;
	}
	return ret;
}

/*----------------------------------------------*
 *  reset device , the funtion may be used for outer error handle, or inner error handle.   *
 *----------------------------------------------*/
s32 sal_reset_device(struct sal_dev * sal_dev,
		     enum sal_eh_dev_rst_type reset_type,
		     sal_eh_wait_smp_func_t wait_func)
{
	struct disc_smp_req *smp_req = NULL;
	struct disc_smp_req *smp_req_rsp = NULL;
	u32 req_len = 0;
	u32 resp_len = 0;
	s32 ret = ERROR;
	struct sal_port *port = NULL;
	enum sal_eh_dev_rst_type reset_action = SAL_EH_RESET_BUTT;

	SAL_ASSERT(NULL != sal_dev->port, return ERROR);

	port = sal_dev->port;
	if (SAL_PORT_LINK_UP != port->status) {
		SAL_TRACE(OSP_DBL_MINOR, 903,
			  "Card:%d Port:%d physical link is down(status:%d)",
			  sal_dev->card->card_no, port->port_id, port->status);
		return ERROR;
	}

	if (port->sas_addr == sal_dev->father_sas_addr) {
		/*direct connection device */
		SAL_TRACE(OSP_DBL_MAJOR, 902,
			  "unsupport direct dev, father sas addr:0x%llx",
			  sal_dev->father_sas_addr);
		return ERROR;
	} else {
		req_len = sal_get_smp_req_len(SMP_PHY_CONTROL);
		resp_len = sal_get_smp_req_len(SMP_PHY_CONTROL);

		smp_req = SAS_KMALLOC(req_len, GFP_KERNEL);
		if (NULL == smp_req) {
			SAL_TRACE(OSP_DBL_MAJOR, 902,
				  "Alloc SMP request memory failed,req len:%d",
				  req_len);
			return ERROR;
		}

		smp_req_rsp = SAS_KMALLOC(resp_len, GFP_KERNEL);
		if (NULL == smp_req_rsp) {
			SAL_TRACE(OSP_DBL_MAJOR, 903,
				  "Alloc SMP rsp memory failed,rsp len:%d",
				  resp_len);
			SAS_KFREE(smp_req);
			return ERROR;
		}

		memset(smp_req, 0, req_len);
		memset(smp_req_rsp, 0, resp_len);

		reset_action = sal_get_dev_reset_type(sal_dev, reset_type);

		if (SAL_EH_DEV_HARD_RESET == reset_action) {
			/*HARD RESET */
			ret =
			    sal_hard_reset_smp(sal_dev, smp_req, req_len,
					       smp_req_rsp, resp_len,
					       wait_func);
			if (ERROR == ret) {
				SAL_TRACE(OSP_DBL_MAJOR, 904,
					  "Card:%d Send Hard reset SMP to dev:0x%llx(father dev addr:0x%llx) failed",
					  sal_dev->card->card_no,
					  sal_dev->sas_addr,
					  sal_dev->father_sas_addr);

				SAS_KFREE(smp_req);
				SAS_KFREE(smp_req_rsp);
				return ERROR;
			}
		} else if (SAL_EH_DEV_LINK_RESET == reset_action) {
			/*LINK RESET */
			ret =
			    sal_link_reset_smp(sal_dev, smp_req, req_len,
					       smp_req_rsp, resp_len,
					       wait_func);
			if (ERROR == ret) {
				SAL_TRACE(OSP_DBL_MAJOR, 905,
					  "Card:%d Send link reset SMP to dev:0x%llx(father dev addr:0x%llx) failed",
					  sal_dev->card->card_no,
					  sal_dev->sas_addr,
					  sal_dev->father_sas_addr);

				SAS_KFREE(smp_req);
				SAS_KFREE(smp_req_rsp);
				return ERROR;
			}
		} else {
			SAL_TRACE(OSP_DBL_MAJOR, 906,
				  "invalid dev type:%d ,reset type:%d ",
				  sal_dev->dev_type, reset_action);

			SAS_KFREE(smp_req);
			SAS_KFREE(smp_req_rsp);
			return ERROR;
		}
	}

	SAS_KFREE(smp_req);
	SAS_KFREE(smp_req_rsp);
	return OK;

}

/*----------------------------------------------*
 * judge if the SATA device IO is cleared  absolutely.   *
 *----------------------------------------------*/
s32 sal_sata_dev_is_clean(struct sal_dev * sal_dev)
{
	if (SAL_DEV_TYPE_STP != sal_dev->dev_type) {
		/* Not SATA device , return true directly. */
		SAL_TRACE(OSP_DBL_MINOR, 907, "the dev is not SATA DEV.");
		return true;
	}

	if (0 != sal_dev->sata_dev_data.pending_io) {
		SAL_TRACE(OSP_DBL_MINOR, 908,
			  "SATA isn't clean,there is :%d IO left",
			  sal_dev->sata_dev_data.pending_io);
		return false;
	}

	return true;
}

/*----------------------------------------------*
 * clear all msg resource which is send to appointed disk IO. *
 *----------------------------------------------*/
void sal_clr_disk_all_io(struct sal_card *sal_card, struct sal_dev *sal_dev)
{
	unsigned long flag = 0;
	unsigned long msg_flag = 0;
	struct list_head *node = NULL;
	struct list_head *safe_node = NULL;
	struct sal_msg *sal_msg = NULL;
	u32 i = 0;

	for (i = 0; i < SAL_MAX_MSG_LIST_NUM; i++) {
		spin_lock_irqsave(&sal_card->card_msg_list_lock[i], flag);
		SAS_LIST_FOR_EACH_SAFE(node, safe_node,
				       &sal_card->active_msg_list[i]) {
			sal_msg = list_entry(node, struct sal_msg, host_list);
			if (sal_dev == sal_msg->dev) {
				/*The IO can't return mid layer. */
				SAL_TRACE(OSP_DBL_MAJOR, 308,
					  "Card:%d clr dev:0x%llx(sal dev:%p) msg:%p(unique id:0x%llx,cdb[0]:0x%x,ST:%d)",
					  sal_card->card_no, sal_dev->sas_addr,
					  sal_dev, sal_msg,
					  sal_msg->ini_cmnd.unique_id,
					  sal_msg->ini_cmnd.cdb[0],
					  sal_msg->msg_state);
				spin_lock_irqsave(&sal_msg->msg_state_lock,
						  msg_flag);
				sal_msg->ini_cmnd.done = NULL;
				spin_unlock_irqrestore(&sal_msg->msg_state_lock,
						       msg_flag);
			}
		}
		spin_unlock_irqrestore(&sal_card->card_msg_list_lock[i], flag);
	}

	return;
}

/*----------------------------------------------*
 * error handle reset dev process.  *
 *----------------------------------------------*/
s32 sal_eh_reset_dev_process(void *argv_in, void *argv_out)
{
	s32 ret = FAILED;
	struct sal_card *card = NULL;
	struct sal_eh_cmnd *eh_cmnd = NULL;
	struct sal_dev *sal_dev = NULL;

	SAL_REF(argv_out);

	eh_cmnd = (struct sal_eh_cmnd *)argv_in;
	SAL_ASSERT(NULL != eh_cmnd, return ERROR);

	/* start checking card state.*/  
	card = sal_get_card(eh_cmnd->card_id);
	if (NULL == card) {
		SAL_TRACE(OSP_DBL_MAJOR, 308, "Get card failed by card id:%d",
			  eh_cmnd->card_id);
		return SUCCESS;
	}

	sal_dev = sal_get_dev_by_scsi_id(card, eh_cmnd->scsi_id);
	if (NULL == sal_dev) {
		SAL_TRACE(OSP_DBL_MAJOR, 910, "Card:%d look up scsi id:%d fail",
			  card->card_no, eh_cmnd->scsi_id);
		sal_put_card(card);
		return SUCCESS;
	}
	/* IO which is sent to disk don't return to mid-layer.*/ 
	sal_clr_disk_all_io(card, sal_dev);

	if (!SAL_IS_CARD_READY(card)) {
		/* card exception  */
		SAL_TRACE(OSP_DBL_MAJOR, 910, "card:%d is not ready,flag:0x%x",
			  card->card_no, card->flag);
		sal_put_dev(sal_dev);
		sal_put_card(card);
		return SUCCESS;
	}

	if (SAL_DEV_ST_ACTIVE != sal_dev->dev_status) {
		/*device state is incorrect*/
		SAL_TRACE(OSP_DBL_MAJOR, 912,
			  "Card:%d dev:%llx is not active(ST:0x%x)",
			  card->card_no, sal_dev->sas_addr,
			  sal_dev->dev_status);

		sal_put_card(card);
		sal_put_dev(sal_dev);
		return SUCCESS;
	}

	SAL_TRACE(OSP_DBL_INFO, 913,
		  "Card:%d is going to reset device:0x%llx now...",
		  card->card_no, sal_dev->sas_addr);

	/*send reset SMP command, and wait for returning.*/
	if (OK !=
	    sal_reset_device(sal_dev, eh_cmnd->reset_type,
			     sal_default_smp_wait_func)) {
		SAL_TRACE(OSP_DBL_MAJOR, 914, "Card:%d reset dev:%llx failed.",
			  card->card_no, sal_dev->virt_sas_addr);
		sal_put_card(card);
		sal_put_dev(sal_dev);
		return FAILED;
	}

	/*abort dev IO */
	ret = sal_abort_dev_io(card, sal_dev);
	if (OK != ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 916,
			  "Card:%d abort dev:%llx io failed.", card->card_no,
			  sal_dev->virt_sas_addr);
		sal_put_dev(sal_dev);
		sal_put_card(card);
		return FAILED;
	}

	/*wait for restoring by reset device */
	SAS_MSLEEP(3000);

	if (SAL_DEV_TYPE_STP == sal_dev->dev_type) {
		/*SATA device  */
		if (false == sal_sata_dev_is_clean(sal_dev)) {
			SAL_TRACE(OSP_DBL_MAJOR, 918,
				  "Card:%d return reset dev:0x%llx failed to SCSI midlayer",
				  card->card_no, sal_dev->virt_sas_addr);
			sal_put_dev(sal_dev);
			sal_put_card(card);
			return FAILED;
		}
	}

	SAL_TRACE(OSP_DBL_INFO, 915,
		  "Card:%d reset dev:0x%llx(sal dev:%p ST:%d) success.",
		  card->card_no, sal_dev->virt_sas_addr, sal_dev,
		  sal_dev->dev_status);

	sal_put_card(card);
	sal_put_dev(sal_dev);
	return SUCCESS;
}

/*----------------------------------------------*
 * error handle device reset   *
 *----------------------------------------------*/
s32 sal_eh_device_reset(struct drv_ini_cmd * ini_cmd)
{
	s32 ret = FAILED;
	struct sal_dev *dev = NULL;
	struct sal_eh_cmnd eh_cmnd;
	struct drv_ini_private_info *drv_info = NULL;

	SAL_ASSERT(NULL != ini_cmd, return FAILED);

	drv_info = (struct drv_ini_private_info *) ini_cmd->private_info;
	dev = (struct sal_dev *)drv_info->driver_data;	/* after Slave destroy，release dev */
	if (NULL == dev) {
		SAL_TRACE(OSP_DBL_MAJOR, 827, "Dev is NULL(unique id:0x%llx)",
			  ini_cmd->cmd_sn);
		return FAILED;
	}

	if (NULL == dev->card) {
		SAL_TRACE(OSP_DBL_MAJOR, 827,
			  "Card is NULL(unique id:0x%llx),Dev addr:0x%llx(sal dev:%p)",
			  ini_cmd->cmd_sn, dev->sas_addr, dev);
		return FAILED;
	}

	sal_get_dev(dev);	/* add reference */
	memset(&eh_cmnd, 0, sizeof(struct sal_eh_cmnd));

	eh_cmnd.card_id = dev->card->card_no;
	eh_cmnd.unique_id = ini_cmd->cmd_sn;
	eh_cmnd.scsi_id = dev->scsi_id;
	eh_cmnd.reset_type = SAL_EH_RESET_BY_DEVTYPE;	/* reset by device type. */
	eh_cmnd.src_direction = SAL_SRC_FROM_FRAME;

	SAL_TRACE(OSP_DBL_INFO, 827,
		  "=== Card:%d is going to reset dev(scsi id:%d) by unique id:%llx by mid layer ===",
		  eh_cmnd.card_id, eh_cmnd.scsi_id, eh_cmnd.unique_id);
	/* step1. clear inner error handle.  */
	sal_clr_err_handler_notify_by_dev(dev->card, dev, true);

	/* step2. prevent inner error handle of the disk.*/
	if (OK != sal_enter_err_hand_mutex_area(dev->card))
		return FAILED;

	dev->mid_layer_eh = true;	/* in the way of outer process,  shield inner error handle.*/
	sal_leave_err_hand_mutex_area(dev->card);

	/* step3. start outer error handle of the disk.*/
	ret = sal_eh_reset_dev_process((void *)&eh_cmnd, NULL);

	/* step4. restore, handle the disk inside. */
	dev->mid_layer_eh = false;	/* in the way of outer process,  shield inner error handle.*/
	sal_put_dev(dev);
	return ret;
}

/*----------------------------------------------*
 * After reset chip, restore the IO which have changed to tmo into FREE state.   *
 *----------------------------------------------*/
void sal_recover_tmo_msg(struct sal_card *sal_card)
{
	unsigned long list_flag = 0;
	struct list_head *safe_node = NULL;	/* safe saved */
	struct list_head *node = NULL;
	struct list_head local_head;	/* local list */
	struct sal_msg *sal_msg = NULL;
	u32 i = 0;

	SAS_INIT_LIST_HEAD(&local_head);

	for (i = 0; i < SAL_MAX_MSG_LIST_NUM; i++) {
		spin_lock_irqsave(&sal_card->card_msg_list_lock[i], list_flag);
		SAS_LIST_FOR_EACH_SAFE(node, safe_node,
				       &sal_card->active_msg_list[i]) {
			sal_msg =
			    SAS_LIST_ENTRY(node, struct sal_msg, host_list);
			if (SAL_MSG_ST_TMO == sal_get_msg_state(sal_msg)) {
				SAS_LIST_DEL_INIT(&sal_msg->host_list);	/* delete from active list .*/
				SAS_LIST_ADD_TAIL(&sal_msg->host_list, &local_head);	/* put in temp list. */
			}
			sal_msg = NULL;	//run out of cycles
		}
		spin_unlock_irqrestore(&sal_card->card_msg_list_lock[i],
				       list_flag);
	}

	/* start process tmo msg in the local list,  don't need to add lock when traverse local list.*/
	SAS_LIST_FOR_EACH_SAFE(node, safe_node, &local_head) {
		sal_msg = SAS_LIST_ENTRY(node, struct sal_msg, host_list);
		SAS_LIST_DEL_INIT(&sal_msg->host_list);	/* fetch from local list. */

		SAL_TRACE(OSP_DBL_INFO, 827,
			  "Card:%d recovery tmo msg:%p(ST:%d uni id:%llx)",
			  sal_card->card_no, sal_msg, sal_msg->msg_state,
			  sal_msg->ini_cmnd.unique_id);

		/* TMO -> free */
		(void)sal_msg_state_chg(sal_msg, SAL_MSG_EVENT_CHIP_RESET_DONE);
		sal_msg = NULL;
	}

	return;
}

/*----------------------------------------------*
 * free all active io.   *
 *----------------------------------------------*/
void sal_free_all_active_io(struct sal_card *sal_card)
{
	unsigned long list_flag = 0;
	struct list_head *safe_node = NULL;	/* safe saved */
	struct list_head *node = NULL;
	struct list_head local_head;	/* local list */
	struct sal_msg *sal_msg = NULL;
	s32 enter_result = 0;
	u32 i = 0;

	SAL_ASSERT(NULL != sal_card, return);

	SAS_INIT_LIST_HEAD(&local_head);

	enter_result = sal_enter_err_hand_mutex_area(sal_card);
	if (OK != enter_result) {
		SAL_TRACE(OSP_DBL_MAJOR, 71, "Card:%d enter Mutex Area failed!",
			  sal_card->card_no);
		return;
	}

	for (i = 0; i < SAL_MAX_MSG_LIST_NUM; i++) {
		spin_lock_irqsave(&sal_card->card_msg_list_lock[i], list_flag);
		SAS_LIST_FOR_EACH_SAFE(node, safe_node,
				       &sal_card->active_msg_list[i]) {
			sal_msg =
			    SAS_LIST_ENTRY(node, struct sal_msg, host_list);

			if (NULL == sal_msg->ini_cmnd.done)
				continue;

			SAS_LIST_DEL_INIT(&sal_msg->host_list);
			SAS_LIST_ADD_TAIL(&sal_msg->host_list, &local_head);	/* put in temp list. */
			sal_msg = NULL;	//run out of cycles
		}
		spin_unlock_irqrestore(&sal_card->card_msg_list_lock[i],
				       list_flag);
	}

        /* start process tmo msg in the local list,  don't need to add lock when traverse local list.*/
	SAS_LIST_FOR_EACH_SAFE(node, safe_node, &local_head) {
		sal_msg = SAS_LIST_ENTRY(node, struct sal_msg, host_list);
		SAS_LIST_DEL_INIT(&sal_msg->host_list);
		SAL_TRACE(OSP_DBL_INFO, 888,
			  "Card:%d del msg:%p(unique id:0x%llx ST:%d)",
			  sal_card->card_no, sal_msg,
			  sal_msg->ini_cmnd.unique_id, sal_msg->msg_state);
		(void)sal_msg_state_chg(sal_msg, SAL_MSG_EVENT_CHIP_REMOVE);
		sal_msg = NULL;
	}


	sal_leave_err_hand_mutex_area(sal_card);

	return;
}

/*----------------------------------------------*
 *release remainder inner IO resource (release resource when unload driver).   *
 *----------------------------------------------*/
void sal_free_left_io(struct sal_card *sal_card)
{
	struct list_head *node = NULL;
	struct list_head *safe_node = NULL;
	struct sal_msg *sal_msg = NULL;
	unsigned long list_flag = 0;
	u32 i = 0;

	for (i = 0; i < SAL_MAX_MSG_LIST_NUM; i++) {
		spin_lock_irqsave(&sal_card->card_msg_list_lock[i], list_flag);
		SAS_LIST_FOR_EACH_SAFE(node, safe_node,
				       &sal_card->active_msg_list[i]) {
			sal_msg =
			    SAS_LIST_ENTRY(node, struct sal_msg, host_list);
			SAS_LIST_DEL_INIT(&sal_msg->host_list);

			SAL_TRACE(OSP_DBL_INFO, 888,
				  "Card:%d destroy msg:%p(ui id:0x%llx ST:%d)",
				  sal_card->card_no, sal_msg,
				  sal_msg->ini_cmnd.unique_id,
				  sal_msg->msg_state);

			if (0 != sal_msg->protocol.stp.indirect_rsp_len) {
				/*release SATA  inner IO resource. */
				sal_free_dma_mem(sal_card,
						 sal_msg->protocol.stp.
						 indirect_rsp_virt_addr,
						 sal_msg->protocol.stp.
						 indirect_rsp_phy_addr,
						 sal_msg->protocol.stp.
						 indirect_rsp_len,
						 PCI_DMA_FROMDEVICE);
			}

			SAS_ATOMIC_DEC(&sal_card->card_io_stat.active_io_cnt);
			SAS_LIST_ADD_TAIL(&sal_msg->host_list,
					  &sal_card->free_msg_list[i]);
		}
		spin_unlock_irqrestore(&sal_card->card_msg_list_lock[i],
				       list_flag);

	}

}

/*----------------------------------------------*
 * error handle re-send message.   *
 *----------------------------------------------*/
s32 sal_err_handle_resend_msg(void *info)
{
	struct sal_msg *sal_msg = NULL;
	enum sal_dev_state dev_state = SAL_DEV_ST_BUTT;
	struct sal_card *sal_card = NULL;

	SAL_ASSERT(info, return ERROR);
	sal_msg = (struct sal_msg *)info;
	SAL_ASSERT(sal_msg->dev, return ERROR);	

	sal_card = sal_msg->card;
	if (unlikely(sal_card->flag & SAL_CARD_SLOWNESS))

		return sal_add_msg_to_err_handler(sal_msg,
						  SAL_ERR_HANDLE_ACTION_RESENDMSG,
						  sal_msg->send_time);

	dev_state = sal_get_dev_state(sal_msg->dev);
	if (SAL_DEV_ST_ACTIVE == dev_state) {
		SAL_TRACE(OSP_DBL_INFO, 888,
			  "Card:%d will resend msg:%p(ST:%d,uni id:0x%llx,Cmd:0x%x,LBA:0x%02x%02x%02x%02x,sec num:0x%02x%02x) to dev:0x%llx(sal dev:%p)",
			  sal_msg->card->card_no, sal_msg, sal_msg->msg_state,
			  sal_msg->ini_cmnd.unique_id, sal_msg->ini_cmnd.cdb[0],
			  sal_msg->ini_cmnd.cdb[2], sal_msg->ini_cmnd.cdb[3],
			  sal_msg->ini_cmnd.cdb[4], sal_msg->ini_cmnd.cdb[5],
			  sal_msg->ini_cmnd.cdb[7], sal_msg->ini_cmnd.cdb[8],
			  sal_msg->dev->sas_addr, sal_msg->dev);
		return sal_start_io(sal_msg->dev, sal_msg);
	} else {
		/* execute in bule area, if the io abort at the same time, it will hang notify directly. */           
		return sal_add_msg_to_err_handler(sal_msg,
						  SAL_ERR_HANDLE_ACTION_RESENDMSG,
						  sal_msg->send_time);
	}
}

/*----------------------------------------------*
 * error handle jam fail .   *
 *----------------------------------------------*/
s32 sal_err_handle_jam_fail(void *info)
{
	struct sal_msg *sal_msg = NULL;

	SAL_ASSERT(info, return ERROR);

	sal_msg = (struct sal_msg *)info;
	SAL_TRACE(OSP_DBL_MAJOR, 888, "msg:%p(ST:%d,uni id:0x%llx) jam failed",
		  sal_msg, sal_msg->msg_state, sal_msg->ini_cmnd.unique_id);

	/* 1. aborting -> aborted;  2.in jam -> send; */
	(void)sal_msg_state_chg(sal_msg, SAL_MSG_EVENT_JAM_FAIL);
	return OK;
}

/*----------------------------------------------*
 * if error handle  fail ,and IO is in driver, then use the done IO .   *
 *----------------------------------------------*/
s32 sal_err_handle_fail_done_io(struct sal_msg * sal_msg)
{
	SAL_ASSERT(sal_msg, return ERROR);

	sal_msg->status.drv_resp = SAL_MSG_DRV_FAILED;

	SAL_ASSERT(sal_msg->done, return ERROR);

	SAL_TRACE(OSP_DBL_MAJOR, 888, "msg:%p(ST:%d,uni id:0x%llx) return",
		  sal_msg, sal_msg->msg_state, sal_msg->ini_cmnd.unique_id);

	sal_msg->done(sal_msg);
	return OK;
}

/*----------------------------------------------*
 *error handle retry fail  .   *
 *----------------------------------------------*/
s32 sal_err_handle_ll_retry_fail(void *info)
{
	struct sal_msg *sal_msg = NULL;
	unsigned long msg_flag = 0;

	SAL_ASSERT(info, return ERROR);
	sal_msg = (struct sal_msg *)info;

	spin_lock_irqsave(&sal_msg->msg_state_lock, msg_flag);
	if (SAL_MSG_ST_ABORTING == sal_msg->msg_state) {
		/* IO timeout ,aborting -> aborted */
		(void)sal_msg_state_chg_no_lock(sal_msg,
						SAL_MSG_EVENT_JAM_FAIL);

		spin_unlock_irqrestore(&sal_msg->msg_state_lock, msg_flag);
		return OK;
	}
	spin_unlock_irqrestore(&sal_msg->msg_state_lock, msg_flag);

	/*err handling -> send */
	(void)sal_msg_state_chg(sal_msg, SAL_MSG_EVENT_LL_RTY_HAND_OVER);	/* IO turn into send state */

	return sal_err_handle_fail_done_io(sal_msg);
}

/*----------------------------------------------*
 *error handle hard reset retry fail  .   *
 *----------------------------------------------*/
s32 sal_err_handle_hd_reset_retry_fail(void *info)
{
	struct sal_msg *sal_msg = NULL;
	unsigned long msg_flag = 0;

	SAL_ASSERT(info, return ERROR);

	sal_msg = (struct sal_msg *)info;

	spin_lock_irqsave(&sal_msg->msg_state_lock, msg_flag);
	if (SAL_MSG_ST_ABORTING == sal_msg->msg_state) {
		/* IO timeout ,aborting -> aborted */
		(void)sal_msg_state_chg_no_lock(sal_msg,
						SAL_MSG_EVENT_JAM_FAIL);

		spin_unlock_irqrestore(&sal_msg->msg_state_lock, msg_flag);
		return OK;
	}
	spin_unlock_irqrestore(&sal_msg->msg_state_lock, msg_flag);

	/*err handling -> send */
	(void)sal_msg_state_chg(sal_msg, SAL_MSG_EVENT_HDRST_RTY_HAND_OVER);	/* IO turn into send state */

	return sal_err_handle_fail_done_io(sal_msg);
}

/*----------------------------------------------*
 *add msg to jam and re-send .   *
 *----------------------------------------------*/
s32 sal_add_msg_to_jam_and_resend(struct sal_msg * sal_msg)
{
	unsigned long state_flag = 0;
	s32 ret = ERROR;

	spin_lock_irqsave(&sal_msg->msg_state_lock, state_flag);
	if (OK == sal_msg_state_chg_no_lock(sal_msg, SAL_MSG_EVENT_NEED_JAM)) {
		SAL_TRACE(OSP_DBL_INFO, 666,
			  "msg:%p(ST:%d,uni id:0x%llx) add jam and resend",
			  sal_msg, sal_msg->msg_state,
			  sal_msg->ini_cmnd.unique_id);
		(void)sal_add_msg_to_err_handler(sal_msg,
						 SAL_ERR_HANDLE_ACTION_RESENDMSG,
						 sal_msg->send_time);
		ret = OK;
	} else {
		SAL_TRACE(OSP_DBL_MAJOR, 666,
			  "msg:%p(ST:%d,uni id:0x%llx) add jam failed", sal_msg,
			  sal_msg->msg_state, sal_msg->ini_cmnd.unique_id);
		ret =
		    sal_msg_state_chg_no_lock(sal_msg,
					      SAL_MSG_EVENT_ADD_JAM_FAIL);
	}
	spin_unlock_irqrestore(&sal_msg->msg_state_lock, state_flag);

	return ret;
}

/*----------------------------------------------*
 *error handle abort , then retry .   *
 *----------------------------------------------*/
s32 sal_err_handle_abort_and_retry_fail(void *info)
{
	struct sal_msg *orgi_msg = NULL;
	unsigned long msg_flag = 0;

	SAL_ASSERT(NULL != info, return ERROR);
	orgi_msg = (struct sal_msg *)info;

	SAL_TRACE(OSP_DBL_INFO, 827,
		  "msg:%p(unique id:0x%llx,ST:%d) retry failed", orgi_msg,
		  orgi_msg->ini_cmnd.unique_id, orgi_msg->msg_state);

	spin_lock_irqsave(&orgi_msg->msg_state_lock, msg_flag);
	if (SAL_MSG_ST_ABORTING == orgi_msg->msg_state) {
		/*error handle in mid-layer ,aborting -> aborted */
		(void)sal_msg_state_chg_no_lock(orgi_msg,
						SAL_MSG_EVENT_JAM_FAIL);

		spin_unlock_irqrestore(&orgi_msg->msg_state_lock, msg_flag);
		return OK;
	}
	spin_unlock_irqrestore(&orgi_msg->msg_state_lock, msg_flag);

	/*pending -> free ; err handle -> send */
	(void)sal_msg_state_chg(orgi_msg, SAL_MSG_EVENT_CANCEL_ERR_HANDLE);
	return OK;
}

/*----------------------------------------------*
 *error handle abort , then retry .   *
 *----------------------------------------------*/
s32 sal_err_handle_abort_and_retry(void *info)
{
	s32 ret = ERROR;
	struct sal_msg *orgi_msg = NULL;
	struct sal_card *card = NULL;
	struct sal_dev *sal_dev = NULL;
	enum sal_dev_state state = SAL_DEV_ST_BUTT;
	unsigned long flag = 0;

	SAL_REF(card);

	SAL_ASSERT(NULL != info, return ERROR);
	orgi_msg = (struct sal_msg *)info;

	spin_lock_irqsave(&orgi_msg->msg_state_lock, flag);
	if (SAL_MSG_ST_IN_ERRHANDING != orgi_msg->msg_state) {
		spin_unlock_irqrestore(&orgi_msg->msg_state_lock, flag);

		SAL_TRACE(OSP_DBL_INFO, 827,
			  "msg:%p(unique id:0x%llx,ST:%d) is back and jam to resend",
			  orgi_msg, orgi_msg->ini_cmnd.unique_id,
			  orgi_msg->msg_state);

		/*IO have return, re-send pending -> in jam */
		ret = sal_add_msg_to_jam_and_resend(orgi_msg);
		return ret;
	}

	/*msg state change  :in err handing -> aborting */
	(void)sal_msg_state_chg_no_lock(orgi_msg, SAL_MSG_EVENT_ABORT_START);

	spin_unlock_irqrestore(&orgi_msg->msg_state_lock, flag);

	card = orgi_msg->card;

	/*add the judge of device state.  */
	sal_dev = orgi_msg->dev;
	state = sal_get_dev_state(sal_dev);
	if (SAL_DEV_ST_ACTIVE != state) {
		SAL_TRACE(OSP_DBL_MAJOR, 827,
			  "Dev:0x%llx isn't active(ST:%d),not handle msg:%p(unique id:0x%llx,ST:%d)",
			  sal_dev->sas_addr, state, orgi_msg,
			  orgi_msg->ini_cmnd.unique_id, orgi_msg->msg_state);
        /*
        1.aborting -> send; (IO not return, restore into original state)
        2.aborted -> free;  (IO have return, make IO done to mid-layer, and release resource.)  
       */
		(void)sal_msg_state_chg(orgi_msg,
					SAL_MSG_EVENT_CANCEL_ERR_HANDLE);
		return OK;
	}

	/* ---------------- TM start --------------- */
	ret = sal_tm_single_io(orgi_msg, sal_inner_tm_wait_func);
	if (ERROR == ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 1233,
			  "Card:%d abort orig msg:%p(unique id:0x%llx,ST:%d) on disk failed",
			  card->card_no, orgi_msg, orgi_msg->ini_cmnd.unique_id,
			  orgi_msg->msg_state);
        /*
        1.aborting -> send; (IO not return, restore into original state)
        2.aborted -> free;  (IO have return, make IO done to mid-layer, and release resource.)  
       */
		(void)sal_msg_state_chg(orgi_msg,
					SAL_MSG_EVENT_CANCEL_ERR_HANDLE);

		return ERROR;
	} else if (SAL_RETURN_MID_STATUS == ret) {
		/*IO may return , or not .  */
		ret =
		    sal_wait_msg_state(orgi_msg, SAL_ABORT_TIMEOUT,
				       SAL_MSG_ST_ABORTED);
		if (OK != ret) {
			SAL_TRACE(OSP_DBL_MAJOR, 814,
				  "Card:%d wait org msg:%p(unique id:0x%llx,Htag:%d,ST:%d) back timeout",
				  card->card_no, orgi_msg,
				  orgi_msg->ini_cmnd.unique_id,
				  orgi_msg->ll_tag, orgi_msg->msg_state);
		}

		/*(IO have return, make IO done to mid-layer, and release resource.)  */
		ret = sal_add_msg_to_jam_and_resend(orgi_msg);
		return ret;
	}

	/* -------------- TM end ----------------- */

	/* ---------------    Abort FW start ---------------- */
	if (OK != sal_abort_single_io(orgi_msg)) {
		SAL_TRACE(OSP_DBL_MAJOR, 1233,
			  "Card:%d abort msg:%p(unique id:0x%llx,ST:%d) in fw failed",
			  card->card_no, orgi_msg, orgi_msg->ini_cmnd.unique_id,
			  orgi_msg->msg_state);
        /*
        1.aborting -> send; (IO not return, restore into original state)
        2.aborted -> free;  (IO have return, make IO done to mid-layer, and release resource.)  
       */
		(void)sal_msg_state_chg(orgi_msg,
					SAL_MSG_EVENT_CANCEL_ERR_HANDLE);

		return ERROR;
	}
	/* ---------------  Abort FW end ---------------- */

	SAL_ASSERT(orgi_msg->msg_state == SAL_MSG_ST_ABORTED, return ERROR);

	/* IO have come back (aborting -> aborted)，hang Jam will re-send (aborted -> in jam) */
	ret = sal_add_msg_to_jam_and_resend(orgi_msg);
	return ret;
}

/*----------------------------------------------*
 *wait end function after send reset dev.   *
 *----------------------------------------------*/
enum sal_eh_wait_func_ret sal_default_smp_wait_func(struct sal_msg *sal_msg,
						    struct sal_port *port)
{
	enum sal_eh_wait_func_ret ret = SAL_EH_WAITFUNC_RET_TIMEOUT;
	struct sal_card *card = NULL;
	unsigned long tmo_jiff = 0;

	card = sal_msg->card;
	tmo_jiff = jiffies + SAL_GET_ENCAP_WAITJIFF(msecs_to_jiffies(SAL_SMP_TIMEOUT * SAL_SMP_RETRY_TIMES), card);	//超时的jiff
	for (;;) {
		if (time_after_eq(jiffies, tmo_jiff)) {
			/* wait time end , not success. */
			ret = SAL_EH_WAITFUNC_RET_TIMEOUT;
			break;
		}
		if (SAL_PORT_LINK_UP != port->status) {
			/* tell waiter to  port down */
			ret = SAL_EH_WAITFUNC_RET_PORTDOWN;
			break;
		}
		if (!SAS_WAIT_FOR_COMPLETION_TIME_OUT(&sal_msg->compl,
						      msecs_to_jiffies
						      (SAL_SMP_TIMEOUT))) {
			/* timeout */ 
			ret = SAL_EH_WAITFUNC_RET_TIMEOUT;
			continue;
		} else {
			/* SMP return , check state that wait success.  */
			ret = SAL_EH_WAITFUNC_RET_WAITOK;
			break;
		}
	}
	return ret;
}

/*----------------------------------------------*
 *inner error handle, wait end function after send smp.   *
 *----------------------------------------------*/
enum sal_eh_wait_func_ret sal_inner_eh_smp_wait_func(struct sal_msg *sal_msg,
						     struct sal_port *port)
{
	enum sal_eh_wait_func_ret ret = SAL_EH_WAITFUNC_RET_TIMEOUT;
	struct sal_card *card = NULL;
	unsigned long tmo_jiff = 0;

	card = sal_msg->card;
	tmo_jiff = jiffies + SAL_GET_ENCAP_WAITJIFF(msecs_to_jiffies(SAL_SMP_TIMEOUT * SAL_SMP_RETRY_TIMES), card);	//超时的jiff
	for (;;) {
		if (time_after_eq(jiffies, tmo_jiff)) {
                      /* wait time end , not success. */
			ret = SAL_EH_WAITFUNC_RET_TIMEOUT;
			break;
		}

		if (SAS_ATOMIC_READ(&card->queue_mutex) != 0) {

			ret = SAL_EH_WAITFUNC_RET_ABORT;
			SAL_TRACE(OSP_DBL_MAJOR, 777,
				  "Card:%d dev:0x%llx smp msg:%p knowns someone is waiting for mutex area, cnt:%d",
				  card->card_no, sal_msg->dev->sas_addr,
				  sal_msg, SAS_ATOMIC_READ(&card->queue_mutex));
			break;
		}

		if (SAL_PORT_LINK_UP != port->status) {
                      /* tell waiter to  port down */
			ret = SAL_EH_WAITFUNC_RET_PORTDOWN;
			break;
		}

		if (!SAS_WAIT_FOR_COMPLETION_TIME_OUT(&sal_msg->compl,
						      msecs_to_jiffies
						      (SAL_SMP_TIMEOUT))) {
			/*  timeout */ 
			ret = SAL_EH_WAITFUNC_RET_TIMEOUT;
			continue;
		} else {
                      /* SMP return , check state that wait success.  */
			ret = SAL_EH_WAITFUNC_RET_WAITOK;
			break;
		}
	}
	return ret;
}

/*----------------------------------------------*
 * wait end function after send TM.   *
 *----------------------------------------------*/
enum sal_eh_wait_func_ret sal_default_tm_wait_func(struct sal_msg *sal_msg)
{
	enum sal_eh_wait_func_ret ret = SAL_EH_WAITFUNC_RET_TIMEOUT;
	struct sal_card *card = NULL;

	card = sal_msg->card;
	if (!SAS_WAIT_FOR_COMPLETION_TIME_OUT(&sal_msg->compl,
					      SAL_GET_ENCAP_WAITJIFF
					      (msecs_to_jiffies(SAL_TM_TIMEOUT),
					       card)))
		/*  timeout */ 
		ret = SAL_EH_WAITFUNC_RET_TIMEOUT;
	else
		ret = SAL_EH_WAITFUNC_RET_WAITOK;

	return ret;
}

/*----------------------------------------------*
 *inner error handle, wait end function after send TM.   *
 *----------------------------------------------*/
enum sal_eh_wait_func_ret sal_inner_tm_wait_func(struct sal_msg *sal_msg)
{
	enum sal_eh_wait_func_ret ret = SAL_EH_WAITFUNC_RET_TIMEOUT;
	struct sal_card *card = NULL;
	unsigned long tmo_jiff = 0;

	card = sal_msg->card;
	tmo_jiff = jiffies + SAL_GET_ENCAP_WAITJIFF(msecs_to_jiffies(SAL_TM_TIMEOUT), card);	//超时的jiff
	for (;;) {
		if (time_after_eq(jiffies, tmo_jiff)) {
			/* wait time end , not success. */
			ret = SAL_EH_WAITFUNC_RET_TIMEOUT;
			break;
		}

		if (SAS_ATOMIC_READ(&card->queue_mutex) != 0) {

			ret = SAL_EH_WAITFUNC_RET_ABORT;
			SAL_TRACE(OSP_DBL_MAJOR, 666,
				  "Card:%d dev:0x%llx tm msg:%p(relate msg:%p) knowns someone is waiting for mutex area, cnt:%d",
				  card->card_no, sal_msg->dev->sas_addr,
				  sal_msg, sal_msg->related_msg,
				  SAS_ATOMIC_READ(&card->queue_mutex));
			break;
		}

		if (!SAS_WAIT_FOR_COMPLETION_TIME_OUT
		    (&sal_msg->compl, msecs_to_jiffies(100))) {
			/*  timeout */ 
			ret = SAL_EH_WAITFUNC_RET_TIMEOUT;
			continue;
		} else {
			/* if wait success ,then quit cycles ,and return OK. */
			ret = SAL_EH_WAITFUNC_RET_WAITOK;
			break;
		}
	}
	return ret;
}

/*----------------------------------------------*
 * error handle, first reset disk ,then reset.   *
 *----------------------------------------------*/
s32 sal_err_handle_hd_reset_and_retry(void *info)
{
	s32 ret = FAILED;
	struct sal_msg *sal_msg = NULL;
	enum sal_dev_state dev_st = SAL_DEV_ST_BUTT;
	u32 card_id = 0;
	u32 wait_cnt = 0;

	SAL_REF(card_id);
	SAL_ASSERT(info, return ERROR);

	sal_msg = (struct sal_msg *)info;
	SAL_ASSERT(sal_msg->dev, return ERROR);

	card_id = sal_msg->card->card_no;

	ret =
	    sal_reset_device(sal_msg->dev, SAL_EH_DEV_HARD_RESET,
			     sal_inner_eh_smp_wait_func);

	if (OK == ret) {
		(void)sal_abort_dev_io(sal_msg->card, sal_msg->dev);
		for (wait_cnt = 0; wait_cnt < 10; wait_cnt++) {
			if (SAS_ATOMIC_READ(&sal_msg->card->queue_mutex) != 0) {
				SAL_TRACE(OSP_DBL_MAJOR, 777,
					  "Card:%d dev:0x%llx eh msg:%p knowns someone is waiting for mutex area,cnt:%d",
					  sal_msg->card->card_no,
					  sal_msg->dev->sas_addr, sal_msg,
					  SAS_ATOMIC_READ(&sal_msg->card->
							  queue_mutex));
				/*err handle -> send */
				(void)sal_msg_state_chg(sal_msg,
							SAL_MSG_EVENT_HDRST_RTY_HAND_OVER);
				return sal_err_handle_fail_done_io(sal_msg);
			} else {
				SAS_MSLEEP(100);	/* many times repeat wait ,then re-scan. */
			}
		}

		/*err handle -> send */
		(void)sal_msg_state_chg(sal_msg,
					SAL_MSG_EVENT_HDRST_RTY_HAND_OVER);

		dev_st = sal_get_dev_state(sal_msg->dev);
		if (SAL_DEV_ST_ACTIVE != dev_st) {
			SAL_TRACE(OSP_DBL_MAJOR, 666,
				  "dev:0x%llx(sal dev:%p) is not active(ST:%d)",
				  sal_msg->dev->sas_addr, sal_msg->dev, dev_st);
			return sal_err_handle_fail_done_io(sal_msg);
		}

		SAL_TRACE(OSP_DBL_INFO, 666,
			  "Card:%d hardreset dev:0x%llx over and resend msg:%p(ST:%d,uni id:0x%llx) to disk",
			  card_id, sal_msg->dev->sas_addr, sal_msg,
			  sal_msg->msg_state, sal_msg->ini_cmnd.unique_id);
		if (SAL_CMND_EXEC_SUCCESS !=
		    sal_msg->card->ll_func.send_msg(sal_msg)) {
			SAL_TRACE(OSP_DBL_MAJOR, 999,
				  "Card:%d hard reset dev:0x%llx but resend msg:%p(ST:%d,uni id:0x%llx) failed",
				  card_id, sal_msg->dev->sas_addr, sal_msg,
				  sal_msg->msg_state,
				  sal_msg->ini_cmnd.unique_id);
			/* re-send fail , then fail io done */
			return sal_err_handle_fail_done_io(sal_msg);
		}

		return OK;

	} else {
		SAL_TRACE(OSP_DBL_MAJOR, 999,
			  "Card:%d hardreset dev:0x%llx failed, return msg:%p(ST:%d,uni id:0x%llx) to sal layer",
			  card_id, sal_msg->dev->sas_addr, sal_msg,
			  sal_msg->msg_state, sal_msg->ini_cmnd.unique_id);

		/*err handle -> send */
		(void)sal_msg_state_chg(sal_msg,
					SAL_MSG_EVENT_HDRST_RTY_HAND_OVER);

		/* send reset dev fail  , then fail io done  */
		return sal_err_handle_fail_done_io(sal_msg);
	}

}

/*----------------------------------------------*
 * error handle re-send.   *
 *----------------------------------------------*/
s32 sal_err_handle_ll_resend(void *info)
{
	struct sal_msg *sal_msg = NULL;
	struct sal_dev *sal_dev = NULL;
	u32 card_id = 0;
	struct sal_card *sal_card = NULL;

	SAL_REF(card_id);
	SAL_ASSERT(NULL != info, return ERROR);

	sal_msg = (struct sal_msg *)info;

	SAL_ASSERT(NULL != sal_msg->dev, return ERROR);
	sal_dev = sal_msg->dev;
	card_id = sal_msg->card->card_no;

	/*err handling -> send */
	(void)sal_msg_state_chg(sal_msg, SAL_MSG_EVENT_LL_RTY_HAND_OVER);	/*IO turn into send state . */

	sal_card = sal_msg->card;
	if (unlikely(sal_card->flag & SAL_CARD_SLOWNESS))
		/* interface card process current command slowly, return mid-layer directly. */
		return sal_err_handle_fail_done_io(sal_msg);

	if (SAL_DEV_ST_ACTIVE != sal_get_dev_state(sal_dev)) {
		SAL_TRACE(OSP_DBL_MAJOR, 666,
			  "Card:%d dev:0x%llx(sal dev:%p) is not active(ST:%d), return msg:%p(uni id:0x%llx) to sal layer",
			  card_id, sal_dev->sas_addr, sal_dev,
			  sal_dev->dev_status, sal_msg,
			  sal_msg->ini_cmnd.unique_id);

		return sal_err_handle_fail_done_io(sal_msg);
	}

	SAL_TRACE(OSP_DBL_INFO, 666,
		  "Card:%d resend msg:%p(ST:%d,uni id:0x%llx,CMD:0x%x,LBA:0x%02x%02x%02x%02x,sec-num:0x%02x%02x) to dev:0x%llx",
		  card_id, sal_msg, sal_msg->msg_state,
		  sal_msg->ini_cmnd.unique_id, sal_msg->ini_cmnd.cdb[0],
		  sal_msg->ini_cmnd.cdb[2], sal_msg->ini_cmnd.cdb[3],
		  sal_msg->ini_cmnd.cdb[4], sal_msg->ini_cmnd.cdb[5],
		  sal_msg->ini_cmnd.cdb[7], sal_msg->ini_cmnd.cdb[8],
		  sal_msg->dev->sas_addr);

	if (SAL_CMND_EXEC_SUCCESS != sal_msg->card->ll_func.send_msg(sal_msg)) {
		SAL_TRACE(OSP_DBL_MAJOR, 999,
			  "Card:%d resend msg:%p(ST:%d,uni id:0x%llx) to dev:0x%llx failed",
			  card_id, sal_msg, sal_msg->msg_state,
			  sal_msg->ini_cmnd.unique_id, sal_msg->dev->sas_addr);
		/*IO send fail , then return IO error to sal-layer.  */
		return sal_err_handle_fail_done_io(sal_msg);
	}

	return OK;
}

/*----------------------------------------------*
 * clear IO in error handle when delete device .   *
 *----------------------------------------------*/
void sal_clean_dev_io_in_err_handle(struct sal_card *sal_card,
				    struct sal_dev *sal_dev)
{
	unsigned long list_flag = 0;
	struct list_head *safe_node = NULL;
	struct list_head *node = NULL;
	struct list_head local_head;	/*local list , save msg which is fetch from notify. */
	struct sal_ll_notify *notify = NULL;

	SAL_ASSERT(sal_dev, return);
	SAL_ASSERT(sal_card, return);

	SAL_TRACE(OSP_DBL_INFO, 633, "Card:%d will clean dev:0x%llx IO",
		  sal_card->card_no, sal_dev->sas_addr);

	SAS_INIT_LIST_HEAD(&local_head);
	
	if (OK != sal_enter_err_hand_mutex_area(sal_card))
		return;

	spin_lock_irqsave(&sal_card->ll_notify_lock, list_flag);
	SAS_LIST_FOR_EACH_SAFE(node, safe_node,
			       &sal_card->ll_notify_process_list) {
		notify =
		    SAS_LIST_ENTRY(node, struct sal_ll_notify, notify_list);

		if ((notify->eh_info.scsi_id == sal_dev->scsi_id)
		    || (notify->info == sal_dev)) {
			SAL_ASSERT(notify->eh_info.card_id == sal_card->card_no); /* bug check  */
			SAS_LIST_DEL_INIT(&notify->notify_list);
			SAS_LIST_ADD_TAIL(&notify->notify_list, &local_head);	/*put in local list.*/
		}
        
		notify = NULL;	/*use up cycle */
	}
	spin_unlock_irqrestore(&sal_card->ll_notify_lock, list_flag);

	/* start process jam io in the local list,  don't need to add lock when traverse local list.*/
	SAS_LIST_FOR_EACH_SAFE(node, safe_node, &local_head) {
		notify =
		    SAS_LIST_ENTRY(node, struct sal_ll_notify, notify_list);
		SAS_LIST_DEL_INIT(&notify->notify_list);	/* fetch from local list. */

		if (NULL != sal_err_io_unhandler_table[notify->op_code])
			(void)sal_err_io_unhandler_table[notify->
							 op_code] (notify->
								   info);

		sal_free_notify_event(sal_card, notify);	/*release resource , put in free list .  */
		notify = NULL;
	}

	sal_leave_err_hand_mutex_area(sal_card);

	return;
}

/*----------------------------------------------*
 * remove IO error handle moudle.    *
 *----------------------------------------------*/
void sal_err_notify_thread_remove(struct sal_card *sal_card)
{
	/* stop err handle thread */
	if (sal_card->eh_thread) {
		(void)SAS_KTHREAD_STOP(sal_card->eh_thread);
		SAL_TRACE(OSP_DBL_INFO, 888,
			  "Card:%d IO err handler thread exit...",
			  sal_card->card_no);
		sal_card->eh_thread = NULL;
	}
	return;
}

/*----------------------------------------------*
 * error notify thread init ,error handle thread.    *
 *----------------------------------------------*/
s32 sal_err_notify_thread_init(struct sal_card * sal_card)
{
	SAL_ASSERT(NULL == sal_card->eh_thread, return ERROR);

	sal_card->eh_thread = OSAL_kthread_run(sal_err_handler,
					       (void *)sal_card, "sal_eh/%d",
					       sal_card->card_no);
	if (IS_ERR(sal_card->eh_thread)) {
		SAL_TRACE(OSP_DBL_MAJOR, 890,
			  "card %d init error handler thread failed, ret %ld",
			  sal_card->card_no, PTR_ERR(sal_card->eh_thread));
		sal_card->eh_thread = NULL;
		return ERROR;
	}

	return OK;
}

/*----------------------------------------------*
 * error notify thread remove .    *
 *----------------------------------------------*/
s32 sal_err_notify_remove(struct sal_card * sal_card)
{
	unsigned long list_flag = 0;
	/* release the resource user first.*/
	sal_err_notify_thread_remove(sal_card);

	/* clear list , avoid obtain. */
	spin_lock_irqsave(&sal_card->ll_notify_lock, list_flag);
	SAS_INIT_LIST_HEAD(&sal_card->ll_notify_free_list);
	SAS_INIT_LIST_HEAD(&sal_card->ll_notify_process_list);
	spin_unlock_irqrestore(&sal_card->ll_notify_lock, list_flag);

	if (sal_card->notify_mem) {
		SAS_VFREE(sal_card->notify_mem);
		sal_card->notify_mem = NULL;
	}
	return OK;
}

/*----------------------------------------------*
 * IO error handle initialize .    *
 *----------------------------------------------*/
s32 sal_err_notify_rsc_init(struct sal_card * sal_card)
{
	s32 ret = ERROR;
	u32 notify_total_mem = 0;
	u32 i = 0;
	struct sal_ll_notify *notify = NULL;
	unsigned long flag = 0;
	u32 max_io_num = 0;

	max_io_num = MIN(SAL_MAX_MSG_NUM, sal_card->config.max_active_io);
	notify_total_mem = sizeof(struct sal_ll_notify) * max_io_num;

	/*IO error handle list node init */
	SAS_SPIN_LOCK_INIT(&sal_card->ll_notify_lock);
	SAS_INIT_LIST_HEAD(&sal_card->ll_notify_free_list);
	SAS_INIT_LIST_HEAD(&sal_card->ll_notify_process_list);

	sal_card->notify_mem = SAS_VMALLOC(notify_total_mem);
	if (NULL == sal_card->notify_mem) {
		SAL_TRACE(OSP_DBL_MAJOR, 378,
			  "card:%d alloc Notify mem fail %d byte",
			  sal_card->card_no, notify_total_mem);
		ret = ERROR;
		goto fail_done;
	}

	memset(sal_card->notify_mem, 0, notify_total_mem);

	for (i = 0; i < max_io_num; i++) {
		/* add into free list . */
		notify = &sal_card->notify_mem[i];

		spin_lock_irqsave(&sal_card->ll_notify_lock, flag);
		SAS_LIST_ADD_TAIL(&notify->notify_list,
				  &sal_card->ll_notify_free_list);
		spin_unlock_irqrestore(&sal_card->ll_notify_lock, flag);
	}
	SAL_TRACE(OSP_DBL_INFO, 378, "card:%d alloc Notify mem:%d byte OK",
		  sal_card->card_no, notify_total_mem);

	ret = sal_err_notify_thread_init(sal_card);
	if (OK != ret) {
		ret = ERROR;
		goto fail_done;
	}
	ret = OK;
	return ret;
      fail_done:
	(void)sal_err_notify_remove(sal_card);
	return ret;
}

/*----------------------------------------------*
 * get reset target dev, if inner error handle is disk , then operation dev .    *
 *----------------------------------------------*/
struct sal_dev *sal_get_reset_target_dev(struct sal_ll_notify *notify)
{
	if (SAL_ERR_HANDLE_ACTION_HDRESET_RETRY == notify->op_code)

		return ((struct sal_msg *)(notify->info))->dev;
	else if (SAL_ERR_HANDLE_ACTION_RESET_DEV == notify->op_code)
		return (struct sal_dev *)notify->info;
	else
		return NULL;
}

/*----------------------------------------------*
 * judge node in the list if need to process ,according to current handle node .   *
 *----------------------------------------------*/
void sal_mark_notify(struct sal_ll_notify *deal_notify,
		     struct sal_ll_notify *notify)
{
	struct sal_dev *deal_sal_dev = NULL;
	struct sal_dev *in_list_sal_dev = NULL;

	/* first kind of polymerization */
	if ((notify->info == deal_notify->info)
	    && (notify->op_code == deal_notify->op_code)) {

		notify->process_flag = false;
		return;
	}

	/* second kind of polymerization  */
	deal_sal_dev = sal_get_reset_target_dev(deal_notify);
	in_list_sal_dev = sal_get_reset_target_dev(notify);
	if ((deal_sal_dev != NULL)
	    && (in_list_sal_dev != NULL)
	    && (deal_sal_dev == in_list_sal_dev))

		notify->process_flag = false;

}

/*----------------------------------------------*
 * check if it is the same IO process in notify list .   *
 *----------------------------------------------*/
void sal_iterate_list_and_mark(struct sal_card *sal_card,
			       struct sal_ll_notify *deal_notify)
{
	unsigned long flag = 0;
	struct sal_ll_notify *notify = NULL;
	struct list_head *node = NULL;

	if (NULL == deal_notify->info)
		/*No need to process  */
		return;

	if (SAS_LIST_EMPTY(&sal_card->ll_notify_process_list))
		/*list is NULL */
		return;

	spin_lock_irqsave(&sal_card->ll_notify_lock, flag);
	SAS_LIST_FOR_EACH(node, &sal_card->ll_notify_process_list) {
		notify =
		    SAS_LIST_ENTRY(node, struct sal_ll_notify, notify_list);
		if (false == notify->process_flag)
			/*false , no need to process */
			continue;

		sal_mark_notify(deal_notify, notify);
		if (false == notify->process_flag) {
			SAL_TRACE(OSP_DBL_INFO, 937,
				  "Card:%d don't handle notify:%p(info:%p,opcode:%d) because cur notify:%p(info:%p,opcode:%d)",
				  sal_card->card_no, notify, notify->info,
				  notify->op_code, deal_notify,
				  deal_notify->info, deal_notify->op_code);
			notify = NULL;
		}
	}
	spin_unlock_irqrestore(&sal_card->ll_notify_lock, flag);

	return;
}

/*----------------------------------------------*
 * check if the error process go on processing .   *
 *----------------------------------------------*/
void sal_check_notify_event_process(struct sal_card *sal_card,
				    struct sal_ll_notify *notify)
{
	unsigned long flag = 0;
	u32 tmo_ms = SAL_DEFAULT_INNER_ERRHANDER_TMO;	
	struct sal_msg *sal_msg = NULL;
	struct sal_dev *dev = NULL;

	
	spin_lock_irqsave(&sal_card->card_lock, flag);
	if (!SAL_IS_CARD_READY(sal_card)
	    && (SAL_ERR_HANDLE_ACTION_RESENDMSG != notify->op_code)) {
		SAL_TRACE(OSP_DBL_INFO, 846,
			  "Card:%d is not active(flag:0x%x),notify opcode:%d(info:%p)",
			  sal_card->card_no, sal_card->flag, notify->op_code,
			  notify->info);
		spin_unlock_irqrestore(&sal_card->card_lock, flag);

		notify->process_flag = false;
		return;
	}
	spin_unlock_irqrestore(&sal_card->card_lock, flag);

	if ((SAL_ERR_HANDLE_ACTION_RESENDMSG == notify->op_code)
	    && (sal_card->config.jam_tmo == 0))
	
		tmo_ms = SAL_DEFAULT_INNER_ERRHANDER_TMO;
	else if ((SAL_ERR_HANDLE_ACTION_RESENDMSG == notify->op_code)
		 && (sal_card->config.jam_tmo <= SAL_MAX_JAM_TMO))
	
		tmo_ms = sal_card->config.jam_tmo;

	/* 2.if timeout , don't process. */
	if (jiffies_to_msecs(jiffies - notify->start_jiffies) >= tmo_ms) {
		SAL_TRACE(OSP_DBL_INFO, 846,
			  "Card:%d notify:%p(opcode:%d info:%p) is time out(JAM THRE:%d ms),not handle it",
			  sal_card->card_no, notify, notify->op_code,
			  notify->info, sal_card->config.jam_tmo);
		notify->process_flag = false;
	} else if (SAL_ERR_HANDLE_ACTION_HDRESET_RETRY == notify->op_code
		   || SAL_ERR_HANDLE_ACTION_ABORT_RETRY == notify->op_code) {
		sal_msg = (struct sal_msg *)notify->info;
		dev = sal_msg->dev;
		if (dev->mid_layer_eh) {

			SAL_TRACE(OSP_DBL_MINOR, 845,
				  "Dev:%p(addr:0x%llx) with opcode:%d Info:%p is doing by mid layer eh.",
				  dev, dev->sas_addr, notify->op_code,
				  notify->info);
			notify->process_flag = false;
		}
	} else if (SAL_ERR_HANDLE_ACTION_RESET_DEV == notify->op_code) {
		dev = (struct sal_dev *)notify->info;
		if (dev->mid_layer_eh) {

			SAL_TRACE(OSP_DBL_MINOR, 845,
				  "Dev:%p(addr:0x%llx) with opcode:%d Info:%p is doing by mid layer eh.",
				  dev, dev->sas_addr, notify->op_code,
				  notify->info);
			notify->process_flag = false;
		}
	}
	return;
}

/*----------------------------------------------*
 * process single notify event  .   *
 *----------------------------------------------*/
void sal_process_single_notify(struct sal_card *sal_card,
			       struct sal_ll_notify *notify)
{
	SAL_ASSERT(sal_card, return);
	SAL_ASSERT(notify, return);

	if (true == notify->process_flag) {
	
		sal_iterate_list_and_mark(sal_card, notify);

		if (NULL != sal_err_io_handler_table[notify->op_code])
			(void)sal_err_io_handler_table[notify->
						       op_code] (notify->info);
		else
			SAL_TRACE(OSP_DBL_MAJOR, 845,
				  "Card:%d IO Err op_code:%d func is NULL,not process",
				  sal_card->card_no, notify->op_code);
	} else {
		
		SAL_TRACE(OSP_DBL_INFO, 846,
			  "Card:%d(flag:0x%x) ignore IO err notify:%p(op_code:%d,info:%p),consume:%d ms",
			  sal_card->card_no, sal_card->flag, notify,
			  notify->op_code, notify->info,
			  jiffies_to_msecs(jiffies - notify->start_jiffies));

		if (NULL != sal_err_io_unhandler_table[notify->op_code])
			(void)sal_err_io_unhandler_table[notify->
							 op_code] (notify->
								   info);
		else
			SAL_TRACE(OSP_DBL_MAJOR, 845,
				  "Card:%d IO Err op_code:%d Unhandle func is NULL",
				  sal_card->card_no, notify->op_code);

	}
}

/*----------------------------------------------*
 * process notify event  .   *
 *----------------------------------------------*/
void sal_process_notify_event(struct sal_card *sal_card)
{
	unsigned long flag = 0;
	struct sal_ll_notify *notify = NULL;
	unsigned long start_jiffes = jiffies;

	SAL_ASSERT(NULL != sal_card, return);

	for (;;) {
		if (SAS_ATOMIC_READ(&sal_card->queue_mutex) != 0) {

			SAS_MSLEEP(10);
			start_jiffes = jiffies;	/*upate time */
			continue;
		}

		if (OK != sal_enter_err_hand_mutex_area(sal_card))
			/*entrance to critical area fail */
			break;

		spin_lock_irqsave(&sal_card->ll_notify_lock, flag);
		if (SAS_LIST_EMPTY(&sal_card->ll_notify_process_list)) {
			spin_unlock_irqrestore(&sal_card->ll_notify_lock, flag);
			sal_leave_err_hand_mutex_area(sal_card);
			break;
		}

		notify =
		    SAS_LIST_ENTRY(sal_card->ll_notify_process_list.next,
				   struct sal_ll_notify, notify_list);
		SAS_LIST_DEL_INIT(&notify->notify_list);
		spin_unlock_irqrestore(&sal_card->ll_notify_lock, flag);

		/*preprocess */
		sal_check_notify_event_process(sal_card, notify);

		/* process single notify */
		sal_process_single_notify(sal_card, notify);

		sal_free_notify_event(sal_card, notify);	

		sal_leave_err_hand_mutex_area(sal_card);

		if (time_after_eq(jiffies, start_jiffes + 3 * HZ)) {
			/*avoid watchdog with long-term use the thread. */
			SAS_MSLEEP(10);
			start_jiffes = jiffies;	/*update time */
		}

	}

	return;
}

/*----------------------------------------------*
 * error handle thread  .   *
 *----------------------------------------------*/
s32 sal_err_handler(void *data)
{
	struct sal_card *sal_card = NULL;

	/* will be not null */
	sal_card = (struct sal_card *)data;

	/*avoid wake up issue invalidly.  */
	SAS_SET_CURRENT_STATE(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		/*IO error handle  ,SAS/SATA event process */
		sal_process_notify_event(sal_card);

		schedule();
		SAS_SET_CURRENT_STATE(TASK_INTERRUPTIBLE);
	}
	__set_current_state(TASK_RUNNING);
	SAL_TRACE(OSP_DBL_INFO, 2, "Card:%d err handler thread exit",
		  sal_card->card_no);
	sal_card->eh_thread = NULL;
	return OK;
}

/*----------------------------------------------*
 * error handle reset dev  .   *
 *----------------------------------------------*/
s32 sal_err_handle_reset_dev(void *info)
{
	struct sal_dev *sal_dev = NULL;
	s32 ret = ERROR;

	SAL_ASSERT(NULL != info, return ERROR);
	sal_dev = (struct sal_dev *)info;

	if (SAL_DEV_ST_ACTIVE == sal_dev->dev_status) {
		SAL_TRACE(OSP_DBL_INFO, 2,
			  "Card:%d will reset dev:0x%llx by err handle",
			  sal_dev->card->card_no, sal_dev->sas_addr);
		ret =
		    sal_reset_device(sal_dev, SAL_EH_RESET_BY_DEVTYPE,
				     sal_inner_eh_smp_wait_func);

		if (OK != ret)
			/*add print when fail*/
			SAL_TRACE(OSP_DBL_INFO, 20,
				  "Card:%d reset dev:0x%llx fail",
				  sal_dev->card->card_no, sal_dev->sas_addr);
		else

			(void)sal_abort_dev_io(sal_dev->card, sal_dev);


	} else {
		SAL_TRACE(OSP_DBL_MAJOR, 2,
			  "Card:%d dev:0x%llx is not active(ST:%d)",
			  sal_dev->card->card_no, sal_dev->sas_addr,
			  sal_dev->dev_status);
	}

	return ret;
}
/*----------------------------------------------*
 * error handle reset host  .   *
 *----------------------------------------------*/
s32 sal_err_handle_reset_host(void *info)
{
	struct sal_card *sal_card = NULL;
	s32 ret = ERROR;
	u32 card_id = 0;

	SAL_ASSERT(NULL != info, return ERROR);

	sal_card = (struct sal_card *)info;

	if (SAL_TRUE == sal_card->reseting) {
		SAL_TRACE(OSP_DBL_MAJOR, 2,
			  "Card:%d(flag:0x%x) is just resetting now(%d)",
			  card_id, sal_card->flag, sal_card->reseting);
		return ERROR;
	}

	card_id = sal_card->card_no;
	SAL_TRACE(OSP_DBL_INFO, 2,
		  "Card:%d is going to reset chip by err handler", card_id);

	sal_send_raw_ctrl_no_wait(card_id, (void *)(u64) card_id, NULL,
				  sal_ctrl_reset_chip_by_card);
	SAL_TRACE(OSP_DBL_INFO, 2,
		  "Card:%d send card reset msg to ctrl, ret 0x%x", card_id,
		  ret);

	return ret;
}

/*----------------------------------------------*
 * IO error handle funtion initialization .   *
 *----------------------------------------------*/
void sal_init_err_handler_op(void)
{
	sal_err_io_handler_table[SAL_ERR_HANDLE_ACTION_RESENDMSG] =
	    sal_err_handle_resend_msg;
	sal_err_io_unhandler_table[SAL_ERR_HANDLE_ACTION_RESENDMSG] =
	    sal_err_handle_jam_fail;

	sal_err_io_handler_table[SAL_ERR_HANDLE_ACTION_HDRESET_RETRY] =
	    sal_err_handle_hd_reset_and_retry;
	sal_err_io_unhandler_table[SAL_ERR_HANDLE_ACTION_HDRESET_RETRY] = sal_err_handle_hd_reset_retry_fail;	//如果不做hard reset和retry,就把io fail done回去

	sal_err_io_handler_table[SAL_ERR_HANDLE_ACTION_LL_RETRY] = sal_err_handle_ll_resend;	//  LL层的重试
	sal_err_io_unhandler_table[SAL_ERR_HANDLE_ACTION_LL_RETRY] = sal_err_handle_ll_retry_fail;	//  不重试的处理

	sal_err_io_handler_table[SAL_ERR_HANDLE_ACTION_ABORT_RETRY] = sal_err_handle_abort_and_retry;	//内部abort然后重试
	sal_err_io_unhandler_table[SAL_ERR_HANDLE_ACTION_ABORT_RETRY] =
	    sal_err_handle_abort_and_retry_fail;

	sal_err_io_handler_table[SAL_ERR_HANDLE_ACTION_RESET_DEV] =
	    sal_err_handle_reset_dev;
	sal_err_io_unhandler_table[SAL_ERR_HANDLE_ACTION_RESET_DEV] = NULL;

	sal_err_io_handler_table[SAL_ERR_HANDLE_ACTION_RESET_HOST] =
	    sal_err_handle_reset_host;
	sal_err_io_unhandler_table[SAL_ERR_HANDLE_ACTION_RESET_HOST] = NULL;

	return;
}

/*----------------------------------------------*
 * IO error handle funtion logout.   *
 *----------------------------------------------*/
void sal_remove_err_handler_op(void)
{
	memset(sal_err_io_unhandler_table, 0,
	       sizeof(sal_err_io_unhandler_table));

	return;
}

/*----------------------------------------------*
 * free notify event .   *
 *----------------------------------------------*/
void sal_free_notify_event(struct sal_card *sal_card,
			   struct sal_ll_notify *notify)
{
	unsigned long flag = 0;

	SAL_ASSERT(NULL != sal_card, return);

	if (NULL == notify) {
		SAL_TRACE(OSP_DBL_MAJOR, 2, "notify is null");
		SAS_DUMP_STACK();
		return;
	}
	/*add msg into free list*/
	spin_lock_irqsave(&sal_card->ll_notify_lock, flag);
	SAS_LIST_ADD_TAIL(&notify->notify_list, &sal_card->ll_notify_free_list);
	spin_unlock_irqrestore(&sal_card->ll_notify_lock, flag);
	return;
}

/*----------------------------------------------*
 * get notify event .   *
 *----------------------------------------------*/
struct sal_ll_notify *sal_get_notify_event(struct sal_card *sal_card)
{
	unsigned long flag = 0;
	struct sal_ll_notify *notify = NULL;

	spin_lock_irqsave(&sal_card->ll_notify_lock, flag);
	if (!SAS_LIST_EMPTY(&sal_card->ll_notify_free_list)) {
		notify =
		    SAS_LIST_ENTRY(sal_card->ll_notify_free_list.next,
				   struct sal_ll_notify, notify_list);
		/*remove from free list. */
		SAS_LIST_DEL_INIT(&notify->notify_list);
		notify->op_code = SAL_ERR_HANDLE_ACTION_BUTT;
		notify->info = NULL;
		notify->start_jiffies = jiffies;
		notify->process_flag = true;
		memset(&notify->eh_info, 0, sizeof(struct sal_eh_cmnd));
	}
	spin_unlock_irqrestore(&sal_card->ll_notify_lock, flag);

	return notify;
}

/*----------------------------------------------*
 * add notify event .   *
 *----------------------------------------------*/
void sal_add_notify_event(struct sal_card *sal_card,
			  struct sal_ll_notify *notify)
{
	unsigned long flag = 0;

	SAL_ASSERT(NULL != sal_card, return);
	SAL_ASSERT(NULL != notify, return);

	spin_lock_irqsave(&sal_card->ll_notify_lock, flag);
	/*add msg into list */
	SAS_LIST_ADD_TAIL(&notify->notify_list,
			  &sal_card->ll_notify_process_list);
	spin_unlock_irqrestore(&sal_card->ll_notify_lock, flag);

	/*wake up thread  */
	(void)SAS_WAKE_UP_PROCESS(sal_card->eh_thread);

	return;
}

/*----------------------------------------------*
 * add msg which is needed to process into SAL error handle thread .   *
 *----------------------------------------------*/
s32 sal_add_msg_to_err_handler(struct sal_msg * sal_msg,
			       enum sal_err_handle_action op_code,
			       unsigned long record_jiff)
{
	struct sal_card *sal_card = NULL;
	struct sal_ll_notify *notify = NULL;

	SAL_ASSERT(NULL != sal_msg, return ERROR);
	SAL_ASSERT(NULL != sal_msg->dev, return ERROR);
	SAL_ASSERT(NULL != sal_msg->card, return ERROR);

	sal_card = sal_msg->card;

	notify = sal_get_notify_event(sal_card);
	if (NULL == notify) {
		SAL_TRACE(OSP_DBL_MAJOR, 2, "Card:%d get notify event failed",
			  sal_card->card_no);
		return ERROR;
	}

	sal_msg->eh.retry_cnt++;
	notify->start_jiffies = record_jiff;	/* update time  */
	notify->op_code = op_code;
	notify->info = (void *)sal_msg;
	notify->eh_info.card_id = sal_msg->card->card_no;
	notify->eh_info.scsi_id = sal_msg->dev->scsi_id;
	notify->eh_info.unique_id = sal_msg->ini_cmnd.unique_id;

	sal_add_notify_event(sal_card, notify);

	return OK;
}

EXPORT_SYMBOL(sal_add_msg_to_err_handler);

/*----------------------------------------------*
 * add dev which is needed to process into SAL error handle thread .   *
 *----------------------------------------------*/
s32 sal_add_dev_to_err_handler(struct sal_dev * sal_dev,
			       enum sal_err_handle_action op_code)
{
	struct sal_ll_notify *notify = NULL;
	struct sal_card *sal_card = NULL;

	SAL_ASSERT(NULL != sal_dev, return ERROR);

	sal_card = sal_dev->card;

	notify = sal_get_notify_event(sal_card);
	if (NULL == notify) {
		SAL_TRACE_FRQLIMIT(OSP_DBL_MAJOR, msecs_to_jiffies(3000), 2,
				   "Card:%d get notify event failed",
				   sal_card->card_no);
		return ERROR;
	}

	notify->op_code = op_code;
	notify->info = (void *)sal_dev;

	sal_add_notify_event(sal_card, notify);
	return OK;
}

EXPORT_SYMBOL(sal_add_dev_to_err_handler);

/*----------------------------------------------*
 * add card which is needed to process into SAL error handle thread .   *
 *----------------------------------------------*/
s32 sal_add_card_to_err_handler(struct sal_card * sal_card,
				enum sal_err_handle_action op_code)
{
	struct sal_ll_notify *notify = NULL;

	SAL_ASSERT(NULL != sal_card, return ERROR);

	notify = sal_get_notify_event(sal_card);
	if (NULL == notify) {
		SAL_TRACE_FRQLIMIT(OSP_DBL_MAJOR, msecs_to_jiffies(3000), 2,
				   "Card:%d get notify event failed",
				   sal_card->card_no);
		return ERROR;
	}

	notify->op_code = op_code;
	notify->info = (void *)sal_card;

	sal_add_notify_event(sal_card, notify);
	return OK;
}

EXPORT_SYMBOL(sal_add_card_to_err_handler);

/*----------------------------------------------*
 * abort smp request .   *
 *----------------------------------------------*/
void sal_abort_smp_request(struct sal_card *sal_card, struct sal_msg *sal_msg)
{
	s32 ret = ERROR;

	/* send -> aborting */
	(void)sal_msg_state_chg(sal_msg, SAL_MSG_EVENT_ABORT_START);

	ret = sal_abort_single_io(sal_msg);
	if (OK == ret) {
		/* aborted -> free */
		(void)sal_msg_state_chg(sal_msg, SAL_MSG_EVENT_WAIT_OK);
	} else {
		/* aborting -> tmo */
		(void)sal_msg_state_chg(sal_msg, SAL_MSG_EVENT_WAIT_FAIL);

		SAL_TRACE(OSP_DBL_INFO, 810,
			  "Card:%d will dump fw that is convenient for fixing problem",
			  sal_card->card_no);
		(void)sal_comm_dump_fw_info(sal_card->card_no,
					    SAL_ERROR_INFO_ALL);

		/* chip exception, reset chip  */
		if (SAL_FALSE == sal_card->reseting) {
			SAL_TRACE(OSP_DBL_INFO, 810,
				  "Card:%d is abnormal,need to reset it",
				  sal_card->card_no);

			sal_send_raw_ctrl_no_wait(sal_card->card_no,
						  (void *)(u64) sal_card->
						  card_no, NULL,
						  sal_ctrl_reset_chip_by_card);
		}

	}

	return;
}


