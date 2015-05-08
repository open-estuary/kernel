/*
 * Huawei Pv660/Hi1610 sas controller discover control file
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
#include "sadiscctrl.h"

extern void sal_do_discover(struct disc_port *port);
static struct task_struct *disc_ctrl_thread = NULL;
static struct disc_event_ctrl event_ctrl;

/**
 * sal_discover - discover api function,when sas domain change,call it.
 * @port_context: port infomation
 *
 */
void sal_discover(struct disc_port_context *port_context)
{
	unsigned long flags = 0;
	struct disc_event *event = NULL;

	event = SAS_KMALLOC(sizeof(struct disc_event), GFP_ATOMIC);
	if (NULL == event) {
		SAL_TRACE(OSP_DBL_MAJOR, 61,
			  "malloc discover event memory failed");
		return;
	}

	memset(event, 0, sizeof(struct disc_event));

	SAS_INIT_LIST_HEAD(&event->event_list);
	event->card_no = port_context->card_no;
	event->port_no = (u8) port_context->port_id;
	event->port_rsc_idx = port_context->port_rsc_idx;
	event->link_rate = port_context->link_rate;
	event->disc_type = (enum sas_ini_disc_type)port_context->disc_type;
	event->local_addr = port_context->local_sas_addr;
	event->process_flag = SAL_TRUE;
	memcpy(&event->identify,
	       &port_context->sas_identify, sizeof(struct sas_identify_frame));

	SAL_TRACE(OSP_DBL_DATA, 62, "Card:%d/port:%d is discovering now...",
		  event->card_no, event->port_no);

	spin_lock_irqsave(&event_ctrl.event_lock, flags);
	SAS_LIST_ADD_TAIL(&event->event_list, &event_ctrl.event_head);

	event_ctrl.cnt++;
	spin_unlock_irqrestore(&event_ctrl.event_lock, flags);

	(void)SAS_WAKE_UP_PROCESS(disc_ctrl_thread);
	return;
}

void sal_notify_to_clr_disc_port_msg(u32 card_id, u32 port_id,
				     u8 port_rsc_index)
{
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	unsigned long flag = 0;
	struct list_head *temp_node = NULL;
	struct disc_msg *disc_msg_entry = NULL;
	struct disc_event *event = NULL;

	SAL_ASSERT(DISC_MAX_CARD_NUM > card_id, return);
	SAL_ASSERT(DISC_MAX_PORT_NUM > port_id, return);

	card = &disc_card_table[card_id];
	port = &card->disc_port[port_id];

	SAL_TRACE(OSP_DBL_INFO, 62,
		  "Card:%d/port:%d(index:%d) clear discover port msg.", card_id,
		  port_id, port_rsc_index);

	spin_lock_irqsave(&event_ctrl.event_lock, flag);
	SAS_LIST_FOR_EACH(temp_node, &event_ctrl.event_head) {
		event =
		    SAS_LIST_ENTRY(temp_node, struct disc_event, event_list);
		if ((event->port_no == port_id)
		    && (event->card_no == card_id)
		    && (event->port_rsc_idx == port_rsc_index)) {
			event->process_flag = SAL_FALSE;
		}
	}
	spin_unlock_irqrestore(&event_ctrl.event_lock, flag);

	spin_lock_irqsave(&port->disc_lock, flag);
	/*release all messages from the discover port*/
	SAS_LIST_FOR_EACH(temp_node, &port->disc_head) {
		disc_msg_entry =
		    SAS_LIST_ENTRY(temp_node, struct disc_msg, disc_list);
		if ((disc_msg_entry->port_rsc_idx == port_rsc_index)
		    || (disc_msg_entry->port_rsc_idx ==
			SAINI_UNKNOWN_PORTINDEX))
			disc_msg_entry->proc_flag = SAL_FALSE;
	}
	spin_unlock_irqrestore(&port->disc_lock, flag);

	return;
}

/**
 * sal_disc_thread - discover process thread
 * @parm: discover event infomation
 *
 */
s32 sal_disc_thread(void *parm)
{
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	struct disc_event *event = NULL;
	struct disc_msg *disc_msg_entry = NULL;
	struct list_head *node = NULL;
	struct list_head *safe_node = NULL;
	struct disc_exp_data *exp_tmp = NULL;
	unsigned long flag = 0;
	unsigned long exp_flag = 0;
	unsigned long start_time = jiffies;

	SAL_REF(start_time);

	SAL_ASSERT(NULL != parm, return ERROR);
	event = (struct disc_event *)parm;

	SAL_TRACE(OSP_DBL_INFO, 64,
		  "Card:%d Port:%d disc thread start-up, hold on a moment...",
		  event->card_no, event->port_no);

	/* delay 10ms,then start discover */
	SAS_MSLEEP(SAL_LANUCH_DISC_INTERVAL);

	SAL_ASSERT(DISC_MAX_CARD_NUM > event->card_no, return ERROR);
	SAL_ASSERT(DISC_MAX_PORT_NUM > event->port_no, return ERROR);

	card = &disc_card_table[event->card_no];
	port = &card->disc_port[event->port_no];

	port->link_rate = event->link_rate;
	port->local_sas_addr = event->local_addr;
	memcpy(&port->identify, &event->identify,
	       sizeof(struct sas_identify_frame));

	spin_lock_irqsave(&card->expander_lock, exp_flag);
	if (!SAS_LIST_EMPTY(&port->disc_expander)) {
		SAS_LIST_FOR_EACH_SAFE(node, safe_node, &port->disc_expander) {
			exp_tmp =
			    SAS_LIST_ENTRY(node, struct disc_exp_data,
					   exp_list);
			exp_tmp->smp_retry_num = 0;
		}
	}
	spin_unlock_irqrestore(&card->expander_lock, exp_flag);

	spin_lock_irqsave(&port->disc_lock, flag);
	while (!SAS_LIST_EMPTY(&port->disc_head)) {
		disc_msg_entry =
		    SAS_LIST_ENTRY(port->disc_head.next, struct disc_msg,
				   disc_list);
		SAS_LIST_DEL_INIT(&disc_msg_entry->disc_list);
		spin_unlock_irqrestore(&port->disc_lock, flag);

		/* start do discover */
		if (SAL_TRUE == disc_msg_entry->proc_flag) {
			port->disc_type = disc_msg_entry->port_disc_type;
			sal_do_discover(port);
		} else {
			SAL_TRACE(OSP_DBL_MINOR, 64,
				  "Card:%d discard the Port:%d disc msg:%p",
				  event->card_no, event->port_no,
				  disc_msg_entry);
		}

		SAS_KFREE(disc_msg_entry);
		disc_msg_entry = NULL;

		spin_lock_irqsave(&port->disc_lock, flag);
	}
	spin_unlock_irqrestore(&port->disc_lock, flag);

	SAL_TRACE(OSP_DBL_INFO, 64,
		  "Card:%d Port:%d disc thread end (cost time:%d ms)",
		  event->card_no, event->port_no,
		  jiffies_to_msecs(jiffies - start_time));

	SAS_KFREE(event);
	SAS_ATOMIC_DEC(&port->ref_cnt);
	return OK;
}

/**
 * sal_disc_dispatcher - discover control thread, get msg from event linked list,
   then create a new thread to do the detial.
 * @parm: nothing
 *
 */
s32 sal_disc_dispatcher(void *parm)
{
	unsigned long flags = 0;
	unsigned long disc_flag = 0;
	struct disc_event *event = NULL;
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	struct disc_msg *disc_msg = NULL;

	SAL_REF(parm);

	/*To avoid invalid wake up, set current set to TASK_INTERRUPTIBLE */
	SAS_SET_CURRENT_STATE(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		disc_msg = NULL;

		if (!event_ctrl.cnt) {
			schedule();
			SAS_SET_CURRENT_STATE(TASK_INTERRUPTIBLE);
			continue;
		}

		disc_msg = SAS_KMALLOC(sizeof(struct disc_msg), GFP_KERNEL);
		if (NULL == disc_msg) {
			SAL_TRACE(OSP_DBL_MAJOR, 64,
				  "Kmalloc disc msg mem failed");
			continue;
		}
		memset(disc_msg, 0, sizeof(struct disc_msg));

		spin_lock_irqsave(&event_ctrl.event_lock, flags);
		if (SAS_LIST_EMPTY(&event_ctrl.event_head)) {
			event_ctrl.cnt = 0;
			spin_unlock_irqrestore(&event_ctrl.event_lock, flags);
			SAS_KFREE(disc_msg);

			schedule();
			SAS_SET_CURRENT_STATE(TASK_INTERRUPTIBLE);
			continue;
		}

		event = SAS_LIST_ENTRY(event_ctrl.event_head.next,
				       struct disc_event, event_list);
		if ((event->card_no >= DISC_MAX_CARD_NUM)
		    || (event->port_no >= DISC_MAX_PORT_NUM)
		    || (SAL_FALSE == event->process_flag)) {

			SAL_TRACE(OSP_DBL_MAJOR, 65,
				  "Card:%d/PortId:%d disc event:%p(Proc Flag:%d) is invalid",
				  event->card_no, event->port_no, event,
				  event->process_flag);

			SAS_LIST_DEL_INIT(&event->event_list);
			event_ctrl.cnt--;
			spin_unlock_irqrestore(&event_ctrl.event_lock, flags);

			memset(event, 0, sizeof(struct disc_event));
			SAS_KFREE(event);
			SAS_KFREE(disc_msg);
			continue;
		}

		card = &disc_card_table[event->card_no];
		port = &card->disc_port[event->port_no];
		if (DISC_RESULT_FALSE == card->inited) {
			SAL_TRACE(OSP_DBL_MAJOR, 65, "Card:%d is not initiated",
				  event->card_no);

			SAS_LIST_DEL_INIT(&event->event_list);
			event_ctrl.cnt--;
			spin_unlock_irqrestore(&event_ctrl.event_lock, flags);

			memset(event, 0, sizeof(struct disc_event));
			SAS_KFREE(event);
			SAS_KFREE(disc_msg);

			schedule();
			SAS_SET_CURRENT_STATE(TASK_INTERRUPTIBLE);
			continue;
		}

		spin_lock_irqsave(&port->disc_lock, disc_flag);

		/*Port has been forbid to create a new discover thread*/ 
		if (PORT_STATUS_FORBID == port->disc_switch) {
			SAL_TRACE(OSP_DBL_MINOR, 65,
				  "Card:%d port:%d was forbidden to disc",
				  event->card_no, event->port_no);

			spin_unlock_irqrestore(&port->disc_lock, disc_flag);

			SAS_LIST_DEL_INIT(&event->event_list);
			event_ctrl.cnt--;
			spin_unlock_irqrestore(&event_ctrl.event_lock, flags);

			memset(event, 0, sizeof(struct disc_event));
			SAS_KFREE(event);
			SAS_KFREE(disc_msg);
			continue;
		}

		if ((DISC_STATUS_NOT_START != port->discover_status)
		    && (0 != SAS_ATOMIC_READ(&port->ref_cnt))) {
			/*current port is discovering, do not create new discover thread*/
			SAL_TRACE(OSP_DBL_DATA, 66,
				  "Card:%d port:%d is already discovering now.",
				  event->card_no, event->port_no);

			if (SAINI_FULL_DISC == event->disc_type) {
				/*full disc, no longer do step disc*/
				port->new_disc = DISC_RESULT_FALSE;
				SAS_INIT_LIST_HEAD(&disc_msg->disc_list);
				disc_msg->port_disc_type = event->disc_type;
				disc_msg->proc_flag = SAL_TRUE;
				disc_msg->port_rsc_idx = event->port_rsc_idx;
				SAS_LIST_ADD_TAIL(&disc_msg->disc_list,
						  &port->disc_head);
			} else {
				/*will step disc */
				port->new_disc = DISC_RESULT_TRUE;
				SAS_KFREE(disc_msg);
			}
			spin_unlock_irqrestore(&port->disc_lock, disc_flag);

			SAS_LIST_DEL_INIT(&event->event_list);
			event_ctrl.cnt--;
			spin_unlock_irqrestore(&event_ctrl.event_lock, flags);
			memset(event, 0, sizeof(struct disc_event));
			SAS_KFREE(event);

			continue;
		}

		port->discover_status = DISC_STATUS_STARTING;

		SAS_INIT_LIST_HEAD(&disc_msg->disc_list);
		disc_msg->port_disc_type = event->disc_type;
		disc_msg->proc_flag = SAL_TRUE;
		disc_msg->port_rsc_idx = event->port_rsc_idx;
		SAS_LIST_ADD_TAIL(&disc_msg->disc_list, &port->disc_head);

		SAS_ATOMIC_INC(&port->ref_cnt);

		spin_unlock_irqrestore(&port->disc_lock, disc_flag);

		SAS_LIST_DEL_INIT(&event->event_list);
		event_ctrl.cnt--;
		spin_unlock_irqrestore(&event_ctrl.event_lock, flags);

		SAL_TRACE(OSP_DBL_INFO, 67,
			  "Card:%d port:%d dispatcher discover event(type:%d <0-step; 1-full;>,ref count:%d)",
			  event->card_no, event->port_no, event->disc_type,
			  SAS_ATOMIC_READ(&port->ref_cnt));

		port->disc_thread =
		    OSAL_kthread_run(sal_disc_thread, (void *)(event),
				     "sal_disc_%d_%d", event->card_no,
				     event->port_no);
		if (IS_ERR(port->disc_thread)) {
			SAL_TRACE(OSP_DBL_MAJOR, 68,
				  "card:%d port:%d create discover thread failed",
				  event->card_no, event->port_no);
			memset(event, 0, sizeof(struct disc_event));
			SAS_KFREE(event);
			SAS_KFREE(disc_msg);
			port->disc_thread = NULL;
			SAS_ATOMIC_DEC(&port->ref_cnt);
		}

	}

	__set_current_state(TASK_RUNNING);
	disc_ctrl_thread = NULL;
	return 0;
}


s32 sal_disc_module_init(void)
{
	unsigned long flag = 0;

	SAS_INIT_LIST_HEAD(&event_ctrl.event_head);
	SAS_SPIN_LOCK_INIT(&event_ctrl.event_lock);

	spin_lock_irqsave(&event_ctrl.event_lock, flag);
	event_ctrl.cnt = 0;
	spin_unlock_irqrestore(&event_ctrl.event_lock, flag);

	disc_ctrl_thread = OSAL_kthread_run(sal_disc_dispatcher,
					    (void *)0, "sal_discctrl");
	if (IS_ERR(disc_ctrl_thread)) {
		SAL_TRACE(OSP_DBL_MAJOR, 69,
			  "init disc module control thread failed");
		disc_ctrl_thread = NULL;
		return ERROR;
	}

	SAL_TRACE(OSP_DBL_DATA, 70, "Disc module init ok.");
	return OK;
}


void sal_disc_module_remove(void)
{
	unsigned long flag = 0;
	struct disc_event *event = NULL;


	spin_lock_irqsave(&event_ctrl.event_lock, flag);
	event_ctrl.cnt = 0;
	spin_unlock_irqrestore(&event_ctrl.event_lock, flag);

	if (disc_ctrl_thread)
		(void)SAS_KTHREAD_STOP(disc_ctrl_thread);

    /*free all memory from event link table,
	  now the chip driver has remove,so this operation is safe*/
	while (!SAS_LIST_EMPTY(&event_ctrl.event_head)) {
		spin_lock_irqsave(&event_ctrl.event_lock, flag);
		event = SAS_LIST_ENTRY(event_ctrl.event_head.next,
				       struct disc_event, event_list);
		
		SAS_LIST_DEL_INIT(&event->event_list);
		spin_unlock_irqrestore(&event_ctrl.event_lock, flag);

		memset(event, 0, sizeof(struct disc_event));
		SAS_KFREE(event);
		event = NULL;
	}

	SAL_TRACE(OSP_DBL_INFO, 71, "disc module remove ok");
}

