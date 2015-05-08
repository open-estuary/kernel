/*
 * Huawei Pv660/Hi1610 sas controller device manage file
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

/*lint -e19 -e801   */

struct sal_ctrl sal_ctrl_table[SAL_MAX_CARD_NUM];

#if 0
/*****************************************************************************
 函 数 名  : 
 功能描述  : 测试校验函数
 输入参数  : void  
 输出参数  : 无
 返 回 值  : s32 
*****************************************************************************/
void SAL_CtrlMsgCheck(u32 card_id)
{
	struct sal_ctrl *card = NULL;
	unsigned long flag = 0;
	u32 cnt = 0;
	struct list_head *list_node = NULL;
	card = &sal_ctrl_table[card_id];

	spin_lock_irqsave(&card->stMsgLock, flag);
	SAS_LIST_FOR_EACH(list_node, &card->free_list) {
		cnt++;
	}
	SAS_LIST_FOR_EACH(list_node, &card->process_list) {
		cnt++;
	}
	spin_unlock_irqrestore(&card->stMsgLock, flag);
	if (cnt != SAL_QUEUE_COUNT) {
		SAL_TRACE(OSP_DBL_MAJOR, 517,
			  "Card %d msg count %d less than SAL_QUEUE_COUNT %d ",
			  card_id, cnt, SAL_QUEUE_COUNT);
	}
}
#endif


s32 sal_ctrl_raw_process(struct sal_ctrl *card, struct sal_ctrl_msg *msg)
{
	s32 ret = ERROR;	/* 默认error */
	SAL_REF(card);

	SAL_ASSERT(NULL != card, return ERROR);
	SAL_ASSERT(NULL != msg, return ERROR);
	SAL_ASSERT(NULL != msg->raw_func, return ERROR);

	ret = msg->raw_func(msg->argv_in, msg->argv_out);
	return ret;
}

/*
***intake of all ctrl message ***
*/
s32 sal_process_ctrl_msg(struct sal_ctrl * card, struct sal_ctrl_msg * msg)
{
	u32 card_id = 0;
	s32 ret = ERROR;	/* default error */

	SAL_ASSERT(NULL != card, return ERROR);
	SAL_ASSERT(NULL != msg, return ERROR);

	card_id = card->card_id;
	/*SAL_TRACE(OSP_DBL_DATA, 524, "Card:%d opcode:%d msg:%p argv_in:%p argv_out:%p raw_func %p",
	   card_id,
	   msg->op_code,
	   msg,
	   msg->argv_in,
	   msg->argv_out,
	   msg->raw_func
	   );
	 */
	ret = sal_ctrl_raw_process(card, msg);
	/* BEGIN:DTS2014071101736 modified by yongze ywx209873 */
	if (SAL_CTRL_HOST_REMOVE == msg->op_code) {
		SAL_TRACE(OSP_DBL_INFO, 525,
			  "Card:%d Enable to add task to ctrl", card_id);
		card->can_add = ENABLE_ADD_TO_CTRL;
		/* END:DTS2014071101736 modified by yongze ywx209873 */
	}

	if (msg->wait) {
		/*return the execute result in synchronism.*/
		msg->status = ret;

		/*wake up ,the resource will be released by the message initiator. */
		SAS_COMPLETE(&msg->compl);
	} else {      /*asynchronization, which will not be waited for ,it is  recycled by the processor. */
		(void)sal_free_ctrl_msg(card_id, msg);
	}

	return ret;
}

/*
***processed all the ctrl list.***
*/
s32 sal_process_ctrl_list(struct sal_ctrl * card)
{
	unsigned long flag = 0;
	struct sal_ctrl_msg *msg = NULL;
	struct list_head *list_node = NULL;

	for (;;) {		/*dispose all the ctrl list.*/
		spin_lock_irqsave(&card->ctrl_list_lock, flag);
		if (SAS_LIST_EMPTY(&card->process_list)) {
			list_node = NULL;
			msg = NULL;
		} else {
			list_node = card->process_list.next;
			msg =
			    SAS_LIST_ENTRY(list_node, struct sal_ctrl_msg,
					   list);
			SAS_LIST_DEL_INIT(&msg->list);	/*delete from dispose ctrl list. */
		}
		spin_unlock_irqrestore(&card->ctrl_list_lock, flag);
		if (NULL == msg)
			/*the list is null*/
			break;

		/* start process. */
		(void)sal_process_ctrl_msg(card, msg);
	}
	return OK;
}

/*
***hand out  and judge all the ctrl list.***
*/
s32 sal_card_ctrl_dispatcher(void *info)
{
	struct sal_ctrl *card = NULL;

	card = (struct sal_ctrl *)info;

	/*avoid waking up problem invalid. */
	SAS_SET_CURRENT_STATE(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		/*BEGIN:modefied by jianghong 00221128,2014/9/15,issue:DTS2014082908037  */
		(void)schedule_timeout((unsigned long)1 * HZ);
		SAS_SET_CURRENT_STATE(TASK_INTERRUPTIBLE);

		(void)sal_process_ctrl_list(card);
		/*END:modefied by jianghong 00221128,2014/9/15,issue :DTS2014082908037 */
	}
	__set_current_state(TASK_RUNNING);
	card->task = NULL;	/* quit out */
	SAL_TRACE(OSP_DBL_INFO, 526, "Card:%2d ctrl thread exit success",
		  card->card_id);
	return ERROR;
}

/*
***create control thread of SAL module.***
*/
s32 sal_card_ctrl_init(void)
{
	u32 card_id = 0;
	u32 i = 0;
	s32 j = 0;
	struct sal_ctrl *card = NULL;

	for (card_id = 0; card_id < SAL_MAX_CARD_NUM; card_id++) {	/* varialbe initialization.*/
		card = &sal_ctrl_table[card_id];
		card->card_id = card_id;
		/* BEGIN:DTS2014071101736 modified by yongze ywx209873 */
		card->can_add = ENABLE_ADD_TO_CTRL;
		/* END:DTS2014071101736 modified by yongze ywx209873 */
		SAS_INIT_LIST_HEAD(&card->free_list);
		SAS_INIT_LIST_HEAD(&card->process_list);
		SAS_SPIN_LOCK_INIT(&card->ctrl_list_lock);

		memset(card->msg, 0,
		       SAL_QUEUE_COUNT * sizeof(struct sal_ctrl_msg));

		/* add into free list */
		for (i = 0; i < SAL_QUEUE_COUNT; i++)
			SAS_LIST_ADD_TAIL(&card->msg[i].list, &card->free_list);

		card->task = NULL;
		card->task = OSAL_kthread_run(sal_card_ctrl_dispatcher,
					      (void *)card,
					      "sal_ctrl_%d", card->card_id);
		if (IS_ERR(card->task)) {
			SAL_TRACE(OSP_DBL_MAJOR, 527,
				  "init card:%d ctrl thread fail", card_id);
			for (j = (s32) card_id - 1; j >= 0; j--) {	/* delete the old thread. */
				(void)SAS_KTHREAD_STOP(sal_ctrl_table[j].task);
				sal_ctrl_table[j].task = NULL;
			}

			return ERROR;
		}
	}

	return OK;
}

/*
***remove control thread of SAL module.***
*/
void sal_card_ctrl_remove(void)
{
	u32 card_id = 0;

	for (card_id = 0; card_id < SAL_MAX_CARD_NUM; card_id++)
               /* delete the old thread. */
		if (sal_ctrl_table[card_id].task)
			(void)SAS_KTHREAD_STOP(sal_ctrl_table[card_id].task);

}

/*
***obtain ctrl msg from the free list.***
*/
struct sal_ctrl_msg *sal_get_ctrl_msg(u32 card_id)
{
	struct sal_ctrl_msg *result = NULL;
	struct sal_ctrl *card = NULL;
	unsigned long flag = 0;

	SAL_ASSERT(card_id < SAL_MAX_CARD_NUM, return NULL);

	card = &sal_ctrl_table[card_id];

	spin_lock_irqsave(&card->ctrl_list_lock, flag);
	if (!SAS_LIST_EMPTY(&card->free_list)) {	/* remove from free list.*/
		result =
		    SAS_LIST_ENTRY(card->free_list.next, struct sal_ctrl_msg,
				   list);
		SAS_LIST_DEL_INIT(&result->list);
		/* initialize message. */
		result->op_code = 0;
		result->argv_in = NULL;
		result->argv_out = NULL;
		result->status = ERROR;
		SAS_INIT_COMPLETION(&result->compl);
	}
	spin_unlock_irqrestore(&card->ctrl_list_lock, flag);
	return result;
}

/*
***replase the ctrl msg into the free list.***
*/
s32 sal_free_ctrl_msg(u32 card_id, struct sal_ctrl_msg * msg)
{
	struct sal_ctrl *card = NULL;
	unsigned long flag = 0;

	SAL_ASSERT(card_id < SAL_MAX_CARD_NUM, return ERROR);
	SAL_ASSERT(NULL != msg, return ERROR);

	card = &sal_ctrl_table[card_id];

	spin_lock_irqsave(&card->ctrl_list_lock, flag);
	SAS_LIST_ADD_TAIL(&msg->list, &card->free_list);
	spin_unlock_irqrestore(&card->ctrl_list_lock, flag);
	return OK;
}

/*
***add the ctrl msg into the process list, and wake up thread to process.***
*/
s32 sal_add_ctrl_msg(u32 card_id, struct sal_ctrl_msg * msg)
{
	struct sal_ctrl *card = NULL;
	unsigned long flag = 0;

	card = &sal_ctrl_table[card_id];
	spin_lock_irqsave(&card->ctrl_list_lock, flag);

        if ((DISABLE_ADD_TO_CTRL == card->can_add)
	    && ((msg->op_code != SAL_CTRL_HOST_ADD)
		&& (msg->op_code != SAL_CTRL_HOST_REMOVE))) {
		spin_unlock_irqrestore(&card->ctrl_list_lock, flag);
		return ERROR;
	}

	SAS_LIST_ADD_TAIL(&msg->list, &card->process_list);
	/*if task is remove, forbid adding task after ctrl thread. */
	if (msg->op_code == SAL_CTRL_HOST_REMOVE) {
		SAL_TRACE(OSP_DBL_INFO, 123,
			  "Card:%d Disable to add task to ctrl", card_id);
		card->can_add = DISABLE_ADD_TO_CTRL;
	}

	spin_unlock_irqrestore(&card->ctrl_list_lock, flag);

	(void)SAS_WAKE_UP_PROCESS(card->task);	/* wake up process thread.*/
	return OK;
}

/*
***send ctrl message.***
*/
s32 sal_send_ctrl_msg(u32 card_id,
		      u32 op_code,
		      void *argv_in,
		      void *argv_out,
		      bool wait, s32(*raw_func) (void *, void *))
{
	s32 status = ERROR;
	struct sal_ctrl_msg *msg = NULL;

	if (sal_global.sal_mod_init_ok != true) {
		SAL_TRACE(OSP_DBL_MAJOR, 528, "sal module is not ready");
		return ERROR;
	}

	if (card_id >= SAL_MAX_CARD_NUM) {
		SAL_TRACE(OSP_DBL_MAJOR, 529,
			  "Card:%d is bigger than SAL_MAX_CARD_NUM:%d ",
			  card_id, SAL_MAX_CARD_NUM);
		return ERROR;
	}

	msg = sal_get_ctrl_msg(card_id);
	if (NULL == msg) {
		SAS_DUMP_STACK();
		SAL_TRACE(OSP_DBL_MAJOR, 530, "Card:%d get ctrl msg failed",
			  card_id);
		return ERROR;
	}

	/* variable initialization. */
	msg->op_code = op_code;
	msg->argv_in = argv_in;
	msg->argv_out = argv_out;
	msg->wait = wait;
	msg->raw_func = raw_func;	/* RAW type*/


	if (OK != sal_add_ctrl_msg(card_id, msg)) {
		(void)sal_free_ctrl_msg(card_id, msg);
		return ERROR;
	}


	if (wait) {		/*synchronize*/
		SAS_WAIT_FOR_COMPLETION(&msg->compl);

		status = msg->status;	

		/*put CTRL MSG into list again. */
		(void)sal_free_ctrl_msg(card_id, msg);
	} else {		/* asynchronous */
		status = OK;
	}
	return status;
}

/*
***transfer ctrl thread parameter asynchronous.***
*/
s32 sal_ctrl_async_func(void *argv_in, void *argv_out)
{
	struct sal_async_info *async_info = NULL;

	SAL_ASSERT(NULL != argv_in, return ERROR);
	SAL_REF(argv_out);

	async_info = (struct sal_async_info *)argv_in;

	if (0 != async_info->delay_time)
		SAS_MSLEEP(async_info->delay_time);

	(void)async_info->exec_func(async_info->argv_in, async_info->argv_out);

	if (NULL != async_info->argv_in)
		SAS_KFREE(async_info->argv_in);

	if (NULL != async_info->argv_out)
		SAS_KFREE(async_info->argv_out);

	SAS_KFREE(async_info);

	return OK;
}

/*
***pfnRawFunc should be in same KO.***
*/
s32 sal_send_raw_ctrl_wait(u32 card_id,
			   void *argv_in,
			   void *argv_out, s32(*raw_func) (void *, void *))
{
	s32 ret = ERROR;

	ret =
	    sal_send_ctrl_msg(card_id, SAL_CTRL_RAW, argv_in, argv_out, true,
			      raw_func);
	return ret;
}

void sal_send_raw_ctrl_no_wait(u32 card_id,
			       void *argv_in,
			       void *argv_out, s32(*raw_func) (void *, void *))
{
	(void)sal_send_ctrl_msg(card_id, SAL_CTRL_RAW, argv_in, argv_out, false,
				raw_func);
	return;
}

EXPORT_SYMBOL(sal_send_raw_ctrl_no_wait);
EXPORT_SYMBOL(sal_send_raw_ctrl_wait);

/*
***pfnRawFunc should be in same KO.***
*/
s32 sal_send_to_ctrl_wait(u32 card_id,
			  u32 op_code,
			  void *argv_in,
			  void *argv_out, s32(*raw_func) (void *, void *))
{
	s32 ret = ERROR;

	ret =
	    sal_send_ctrl_msg(card_id, op_code, argv_in, argv_out, true,
			      raw_func);
	return ret;
}
void sal_send_to_ctrl_no_wait(u32 card_id,
			      u32 op_code,
			      void *argv_in,
			      void *argv_out, s32(*raw_func) (void *, void *))
{
	(void)sal_send_ctrl_msg(card_id, op_code, argv_in, argv_out, false,
				raw_func);
	return;
}

EXPORT_SYMBOL(sal_send_to_ctrl_no_wait);
EXPORT_SYMBOL(sal_send_to_ctrl_wait);
/* END:DTS2014071101736 modified by yongze ywx209873 */
//lint +e19 +e801
