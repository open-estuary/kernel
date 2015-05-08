/*
 * Huawei Pv660/Hi1610 sas controller driver common function
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
#include "sal_dev.h"
#include "sal_io.h"
#include "sal_errhandler.h"
#include "sal_init.h"

#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/rtc.h>

#define TASK_NAME_LEN   12          
#define TIME_STRING_LEN   20
#define K_LOG_MSG_LENGTH 128  

#ifndef INVALID_VALUE32
#define INVALID_VALUE32 0xFFFFFFFF
#endif

extern SAS_SPINLOCK_T card_lock;

#ifndef TIMER_HZ_MSECONDS
#define TIMER_HZ_MSECONDS 1000
#endif

#define OSAL_MS_TO_JIFFIES(ms)   ((ms) * HZ / TIMER_HZ_MSECONDS)

struct sal_dev *sal_find_dev_by_slot_id(struct sal_card *card,
					struct sal_dev *exp_dev, u32 slot_id)
{
	unsigned long flag = 0;
	struct list_head *dev_node = NULL;
	struct list_head *tmp_node = NULL;
	struct sal_dev *dev = NULL;

	SAL_ASSERT(NULL != card, return NULL);
	SAL_ASSERT(NULL != exp_dev, return NULL);

	spin_lock_irqsave(&card->card_dev_list_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(dev_node, tmp_node, &card->active_dev_list) {
		dev = SAS_LIST_ENTRY(dev_node, struct sal_dev, dev_list);
		if ((dev->slot_id == slot_id)
		    && (dev->father_sas_addr == exp_dev->sas_addr)
		    && (dev->port->port_id == exp_dev->port->port_id)) {
			spin_unlock_irqrestore(&card->card_dev_list_lock, flag);
			return dev;
		}
	}
	spin_unlock_irqrestore(&card->card_dev_list_lock, flag);
	return NULL;
}

EXPORT_SYMBOL(sal_find_dev_by_slot_id);

struct sal_port *sal_find_port_by_chip_port_id(struct sal_card *card,
					       u32 chip_port_id)
{
	u32 i = 0;
	struct sal_port *port = NULL;

	for (i = 0; i < SAL_MAX_PHY_NUM; i++) {
		if (NULL == card->port[i])
			continue;

		if (SAL_PORT_FREE == card->port[i]->status)
			continue;

		if (SAL_PORT_LINK_DOWN == card->port[i]->status)
			continue;

		if (chip_port_id == card->port[i]->port_id) {
			port = card->port[i];
			return port;
		}
	}

	return NULL;
}

u32 sal_find_link_phy_in_port_by_rate(struct sal_port * sal_port)
{
	u32 i = 0;
	struct sal_card *tmp_card = NULL;
	struct sal_phy *tmp_phy = NULL;

	SAL_ASSERT(NULL != sal_port, return SAL_INVALID_PHYID);
	tmp_card = sal_port->card;

	for (i = 0; i < tmp_card->phy_num; i++)
		if (SAL_TRUE == sal_port->phy_id_list[i]) {
			tmp_phy = tmp_card->phy[i];
			if (sal_port->max_rate == tmp_phy->link_rate)
				return i;
		}

	return SAL_INVALID_PHYID;
}

struct sal_card *sal_find_card_by_host_id(u32 host_id)
{
	u32 i = 0;
	unsigned long flag = 0;
	struct sal_card *card = NULL;

	spin_lock_irqsave(&card_lock, flag);
	for (i = 0; i < SAL_MAX_CARD_NUM; i++) {
		if ((NULL == sal_global.sal_cards[i])
		    || ((struct sal_card *)SAL_INVALID_POSITION ==
			sal_global.sal_cards[i]))
			continue;

		if (host_id == sal_global.sal_cards[i]->host_id) {
			card = sal_global.sal_cards[i];
			break;
		}
	}
	spin_unlock_irqrestore(&card_lock, flag);

	return card;

}

u32 sal_first_phy_id_in_port(struct sal_card * card, u32 port_idx)
{
	u32 i = 0;
	u32 port_mask = 0;
	u32 ret = SAL_INVALID_PHYID;

	if (port_idx >= SAL_MAX_PHY_NUM) {
		SAL_TRACE(OSP_DBL_MAJOR, 241, "PortIndex:%d is invalid",
			  port_idx);
		return ERROR;
	}

	port_mask = card->config.port_bitmap[port_idx];

	for (i = 0; i < card->phy_num; i++)	
		if (port_mask & (1 << i)) {
			ret = i;
			break;
		}

	return ret;
}

u32 sal_comm_get_logic_id(u32 card_id, u32 phy_id)
{
	u32 i = 0;
	u32 phy_bit = 0;
	u32 logic_id = 0;
	u32 tmp_bit_map = 0;
	struct sal_card *tmp_card = NULL;

	tmp_card = sal_get_card(card_id);
	if (NULL == tmp_card) {
		SAL_TRACE(OSP_DBL_MAJOR, 223, "Get card:%d failed.", card_id);
		return SAL_INVALID_LOGIC_PORTID;
	}

	if (phy_id >= tmp_card->phy_num) {
		SAL_TRACE(OSP_DBL_MAJOR, 224,
			  "Card:%d phy:%d exceed max support phy num:%d",
			  card_id, phy_id, tmp_card->phy_num);
		sal_put_card(tmp_card);
		return SAL_INVALID_LOGIC_PORTID;
	}

	phy_bit = 1 << phy_id;
	for (i = 0; i < tmp_card->phy_num; i++) {
		tmp_bit_map = tmp_card->config.port_bitmap[i];
		if (phy_bit & tmp_bit_map) {
			logic_id = tmp_card->config.port_logic_id[i];
			sal_put_card(tmp_card);
			return logic_id;
		}
	}

	sal_put_card(tmp_card);
	return SAL_INVALID_LOGIC_PORTID;
}

s32 sal_comm_get_chip_info(void *argv_in, void *argv_out)
{
	s32 ret = ERROR;
	struct sal_card *tmp_card = NULL;
	u32 card_id = 0;

	SAL_REF(argv_out);	

	card_id = (u32) (u64) argv_in;

	tmp_card = sal_get_card(card_id);
	if (NULL == tmp_card) {
		SAL_TRACE(OSP_DBL_MAJOR, 523, "Get card failed by cardid:%d",
			  card_id);
		return ERROR;
	}

	if (NULL == tmp_card->ll_func.chip_op) {
		SAL_TRACE(OSP_DBL_MAJOR, 523, "Card:%d chip_op func is NULL",
			  card_id);
		sal_put_card(tmp_card);
		return ERROR;
	}

	ret =
	    tmp_card->ll_func.chip_op(tmp_card, SAL_CHIP_OPT_GET_VERSION, NULL);
	if (OK != ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 523, "Card:%d get chip info Failed",
			  tmp_card->card_no);
		sal_put_card(tmp_card);
		return ERROR;
	}

	SAL_TRACE(OSP_DBL_INFO, 519,
		  "Current Chip Firmware Ver:%s EEPROM Ver:%s ILA Ver:%s HW:%s",
		  tmp_card->chip_info.fw_ver, tmp_card->chip_info.eeprom_ver,
		  tmp_card->chip_info.ila_ver, tmp_card->chip_info.hw_ver);

	sal_put_card(tmp_card);
	return OK;
}

EXPORT_SYMBOL(sal_comm_get_chip_info);

/**
 * sal_comm_switch_phy - operate phy, start/stop/reset/... etc.
 * @sal_card:point to controller
 * @phy_id:phy id
 * @op_type:phy operation type
 * @phy_rate:phy operation rate 
 */
s32 sal_comm_switch_phy(struct sal_card * sal_card, u32 phy_id,
			enum sal_phy_op_type op_type, u32 phy_rate)
{
	s32 ret = ERROR;
	unsigned long flag = 0;

	spin_lock_irqsave(&sal_card->card_lock, flag);
	if (!SAL_IS_CARD_READY(sal_card)) {
		spin_unlock_irqrestore(&sal_card->card_lock, flag);
		SAL_TRACE(OSP_DBL_MAJOR, 219,
			  "Card:%d is not active", sal_card->card_no);
		return ERROR;
	}
	spin_unlock_irqrestore(&sal_card->card_lock, flag);

	if ((sal_card->phy_num <= phy_id) || (SAL_MAX_PHY_NUM <= phy_id)) {
		SAL_TRACE(OSP_DBL_MAJOR, 220,
			  "Phy Id(%d) is greadter than Max phy num(%d)", phy_id,
			  sal_card->phy_num);
		return ERROR;
	}
	/* stop phy */
	if ((SAL_PHY_OPT_CLOSE == op_type)
	    && (SAL_PHY_CLOSED == sal_card->phy[phy_id]->link_status)) {
		SAL_TRACE(OSP_DBL_INFO, 221,
			  "Card:%d Phyid:%2d is already closed,status:0x%x",
			  sal_card->card_no, phy_id,
			  sal_card->phy[phy_id]->link_status);
		return OK;
	}

	/* start phy */
	if ((SAL_PHY_OPT_OPEN == op_type)
	    && (SAL_PHY_CLOSED != sal_card->phy[phy_id]->link_status)) {
		SAL_TRACE(OSP_DBL_INFO, 222,
			  "Card:%d Phyid:%2d is already open,status:0x%x",
			  sal_card->card_no, phy_id,
			  sal_card->phy[phy_id]->link_status);
		return OK;
	}

	/* other operations */
	ret = sal_do_switch_phy(sal_card, phy_id, op_type, phy_rate);
	return ret;
}

s32 sal_do_switch_phy(struct sal_card * sal_card, u32 phy_id,
		      enum sal_phy_op_type op_type, u32 phy_rate)
{
	s32 ret = ERROR;
	struct sal_phy_ll_op_param ll_phy_op_info;

	if (phy_id >= sal_card->phy_num) {
		SAL_TRACE(OSP_DBL_MAJOR, 212,
			  "phy:%d exceed max support phy num:%d", phy_id,
			  sal_card->phy_num);
		return ERROR;
	}

	if (NULL == sal_card->ll_func.phy_op) {
		SAL_TRACE(OSP_DBL_MAJOR, 213, "Phy:%d operate Func is NULL",
			  phy_id);
		return ERROR;
	}

	memset(&ll_phy_op_info, 0, sizeof(struct sal_phy_ll_op_param));
	ll_phy_op_info.card_id = sal_card->card_no;
	ll_phy_op_info.phy_id = phy_id;
	ll_phy_op_info.phy_rate = phy_rate;
	if (SAL_PHY_OPT_RESET == op_type) {
		sal_card->phy[phy_id]->err_code.last_switch_jiff = jiffies;

		ret =
		    sal_card->ll_func.phy_op(sal_card, SAL_PHY_OPT_CLOSE,
					     (void *)&ll_phy_op_info);
		if (OK != ret)
			return ret;

		SAS_MSLEEP(100);

		sal_card->phy[phy_id]->err_code.last_switch_jiff = jiffies;
		ret = sal_card->ll_func.phy_op(sal_card, SAL_PHY_OPT_OPEN, (void *)&ll_phy_op_info);

		return ret;
	}
	sal_card->phy[phy_id]->err_code.last_switch_jiff = jiffies;

	ret =
	    sal_card->ll_func.phy_op(sal_card, op_type,
				     (void *)&ll_phy_op_info);
	return ret;
}

s32 sal_operate_phy(void *argv_in, void *argv_out)
{
	s32 ret = ERROR;
	struct sal_card *tmp_card = NULL;
	struct sal_event *tmp_event = NULL;
	struct sal_phy_op_param *info = NULL;

	SAL_REF(argv_out);

	msleep(10);

	tmp_event = (struct sal_event *)argv_in;
	info = (struct sal_phy_op_param *)tmp_event->comm_str;

	tmp_card = sal_get_card(info->card_id);
	if (NULL == tmp_card) {
		SAL_TRACE(OSP_DBL_MAJOR, 228, "Get card failed by cardid:%d",
			  info->card_id);
		SAS_KFREE(tmp_event);
		return ERROR;
	}

	if (info->phy_rate_restore)
		info->phy_rate = tmp_card->config.phy_link_rate[info->phy_id];

	ret = sal_comm_switch_phy(tmp_card,
				  info->phy_id, info->op_type, info->phy_rate);

	SAS_KFREE(tmp_event);
	sal_put_card(tmp_card);
	return ret;
}

/**
 * sal_find_port_by_logic_port_id - get sal_port by logic port id
 * @card:point to controller
 * @logic_port_id:logic port id of the port
 */
struct sal_port *sal_find_port_by_logic_port_id(struct sal_card *card,
						u32 logic_port_id)
{
	u32 i = 0;
	struct sal_port *sal_port = NULL;

	for (i = 0; i < card->phy_num; i++) {
		if (NULL == card->port[i])
			continue;

		if (card->port[i]->logic_id == logic_port_id) {
			sal_port = card->port[i];
			break;
		}
	}

	return sal_port;
}

/**
 * sal_wait_clr_rsc_when_close_port - wait until all resource is cleared,when close port
 * @card:point to controller
 * @logic_port_id:logic port id of the port
 */
void sal_wait_clr_rsc_when_close_port(struct sal_card *card, u32 logic_port_id)
{
	struct sal_port *sal_port = NULL;

	sal_port = sal_find_port_by_logic_port_id(card, logic_port_id);
	if (NULL != sal_port)
		if (SAL_TRUE == sal_is_all_phy_down(sal_port))
			(void)sal_wait_clr_port_rsc(sal_port);

	return;
}

/**
 * sal_wait_clr_port_rsc - wait until all resource of the port is cleared
 * @sal_port:point to port
 */
s32 sal_wait_clr_port_rsc(struct sal_port * sal_port)
{
	unsigned long cost_time = 0;

	cost_time = jiffies + 3 * HZ;

	while (SAL_PORT_RSC_UNUSE != SAS_ATOMIC_READ(&sal_port->port_in_use)) {
		SAS_MSLEEP(10);

		if (time_after(jiffies, cost_time)) {
			SAL_TRACE(OSP_DBL_MAJOR, 213,
				  "Card:%d wait port:%d clr time out",
				  sal_port->card->card_no, sal_port->port_id);
			return ERROR;
		}
	}
	return OK;
}

/**
 * sal_stop_all_phy - stop all phy that belong to the sas controller
 * @sal_card:point to controller
 */
s32 sal_stop_all_phy(struct sal_card * sal_card)
{
	u32 i = 0;
	struct sal_phy_ll_op_param ll_phy_op_info = { 0 };
	s32 ret = ERROR;

	if (NULL == sal_card->ll_func.phy_op) {
		SAL_TRACE(OSP_DBL_MINOR, 213, "Phy:%d operate Func is NULL", i);
		return ERROR;
	}

	for (i = 0; i < sal_card->phy_num; i++) {	
		sal_card->phy[i]->err_code.last_switch_jiff = jiffies;
		ll_phy_op_info.card_id = sal_card->card_no;
		ll_phy_op_info.phy_id = i;

		ret = sal_card->ll_func.phy_op(sal_card,
					       SAL_PHY_OPT_CLOSE,
					       (void *)&ll_phy_op_info);
		if (OK != ret) {
			SAL_TRACE(OSP_DBL_MAJOR, 213,
				  "Card:%d close phy:%d failed",
				  sal_card->card_no, i);
			return ret;
		}
	}

	for (i = 0; i < sal_card->phy_num; i++)
		if (OK != sal_wait_clr_port_rsc(sal_card->port[i])) {
			SAL_TRACE(OSP_DBL_MAJOR, 213,
				  "Card:%d wait port:%d clr time out",
				  sal_card->card_no, i);
			return ERROR;
		}

	return OK;
}

void sal_clean_err_handler_notify(struct sal_card *sal_card)
{
	unsigned long flag = 0;
	struct list_head *list_node = NULL;
	struct list_head *tmp_list_node = NULL;
	struct sal_ll_notify *tmp_notify = NULL;

	spin_lock_irqsave(&sal_card->ll_notify_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(list_node, tmp_list_node,
			       &sal_card->ll_notify_process_list) {
		tmp_notify =
		    SAS_LIST_ENTRY(list_node, struct sal_ll_notify,
				   notify_list);
		if (SAL_ERR_HANDLE_ACTION_RESENDMSG != tmp_notify->op_code) {
			tmp_notify->process_flag = false;
		}
	}
	spin_unlock_irqrestore(&sal_card->ll_notify_lock, flag);

	return;
}

s32 sal_reset_chip_func_exit(struct sal_card * sal_card)
{
	unsigned long flag = 0;

	spin_lock_irqsave(&sal_card->card_lock, flag);
	sal_card->reseting = SAL_FALSE;
	spin_unlock_irqrestore(&sal_card->card_lock, flag);

	return ERROR;
}
/**
 * sal_repair_operation - repair the sas controller
 * @sal_card:point to controller
 */
s32 sal_repair_operation(struct sal_card * sal_card)
{
	struct sal_event *tmp_event = NULL;
	u32 i = 0;
	unsigned long dead_time = 0;

	SAL_ASSERT(NULL != sal_card, return ERROR);

	tmp_event = sal_get_free_event(sal_card);
	if (NULL == tmp_event) {
		SAL_TRACE(OSP_DBL_MAJOR, 541,
			  "Card:%d get free event node failed",
			  sal_card->card_no);
		return ERROR;
	}

	memset(tmp_event, 0, sizeof(struct sal_event));
	tmp_event->time = jiffies;
	tmp_event->event = SAL_EVENT_PORT_REPAIR;	

	SAL_TRACE(OSP_DBL_INFO, 542, "Card:%d notify to repair all port rsc",
		  sal_card->card_no);

	sal_add_dev_event_thread(sal_card, tmp_event);

	dead_time = jiffies + 3 * HZ;
	/* wait until all port resource is cleared or timeout */
	for (i = 0; i < sal_card->phy_num; i++) {
		while (SAL_PORT_RSC_UNUSE !=
		       SAS_ATOMIC_READ(&sal_card->port[i]->port_in_use)) {
			SAS_MSLEEP(10);

            /* if port resource has not clear after timeout, then return error */
			if (time_after(jiffies, dead_time)) {	
				SAL_TRACE(OSP_DBL_MAJOR, 213,
					  "Card:%d wait port:%d portloop:%d clr time out",
					  sal_card->card_no,
					  sal_card->port[i]->port_id, i);
				return ERROR;
			}
		}
	}

	return OK;
}

/**
 * sal_wait_all_port_disc_over - wait until all ports of the sas controller
 * complete discover operation
 *
 * @card_no:card no/id of the sas controller
 */
void sal_wait_all_port_disc_over(u32 card_no)
{
	u32 i = 0;
	struct disc_port *port = NULL;
	struct disc_card *disc_card = NULL;
	unsigned long start_time = 0;

	SAL_ASSERT(card_no < DISC_MAX_CARD_NUM, return);

	disc_card = &disc_card_table[card_no];

	/* wait all port finish discover */
	for (i = 0; i < DISC_MAX_PORT_NUM; i++) {
		port = &disc_card->disc_port[i];
		start_time = jiffies;
		for (;;) {
			if (0 == SAS_ATOMIC_READ(&port->ref_cnt))
				/* this port discover is over */
				break;

			SAS_MSLEEP(10);

			if (time_after(jiffies, start_time + 3 * HZ))
				SAL_TRACE_FRQLIMIT(OSP_DBL_MAJOR, 3, 308,
						   "Card id:%d wait port:%d(ref count:%d) exit disc thread time out",
						   card_no, i,
						   SAS_ATOMIC_READ(&port->
								   ref_cnt));
		}
	}

	return;
}

void sal_init_rsc_after_chip_reset(struct sal_card *sal_card)
{
	u32 i = 0;

	for (i = 0; i < sal_card->phy_num; i++)
		sal_card->phy[i]->config_state = SAL_PHY_CONFIG_OPEN;

	return;
}

void sal_notify_disc_all_port_down(struct sal_card *sal_card)
{
	struct sal_port *port = NULL;
	u32 i = 0;

	for (i = 0; i < SAL_MAX_PHY_NUM; i++) {
		if (NULL == sal_card->port[i])
			continue;

		port = sal_card->port[i];
		if (SAL_PORT_LINK_UP == port->status)
			sal_port_down_notify(sal_card->card_no, port->port_id);
	}
}

/**
 * sal_comm_reset_chip - common interface used to reset sas controller
 *
 * @argv_in:argument value in
 * @argv_out:argument value out
 */
s32 sal_comm_reset_chip(void *argv_in, void *argv_out)
{
	s32 ret = ERROR;
	struct sal_card *tmp_card = NULL;
	u32 soft_mode = 0;
	u32 card_id = 0;
	u32 cnt;
	unsigned long flag = 0;
	unsigned long magic = 0;

	SAL_REF(argv_out);	
	SAL_ASSERT(NULL != argv_in, return ERROR);

	card_id = *((u32 *) argv_in);

	tmp_card = sal_get_card(card_id);
	if (NULL == tmp_card) {
		SAL_TRACE(OSP_DBL_MAJOR, 308, "Get card failed by card id:%d",
			  card_id);
		return ERROR;
	}

	if (SAL_CARD_ACTIVE != sal_get_card_flag(tmp_card)) {
		SAL_TRACE(OSP_DBL_MAJOR, 308,
			  "Card:%d is not active(flag:0x%x)", tmp_card->card_no,
			  tmp_card->flag);
		sal_put_card(tmp_card);
		return ERROR;
	}

	spin_lock_irqsave(&tmp_card->card_lock, flag);
	tmp_card->reseting = SAL_TRUE;
	tmp_card->verification_key = jiffies;	
	spin_unlock_irqrestore(&tmp_card->card_lock, flag);

	for (cnt = 0; cnt < tmp_card->phy_num; cnt++) {
		tmp_card->phy[cnt]->phy_reset_flag = SAL_PHY_NEED_RESET_FOR_DFR;
	}

	if (NULL == tmp_card->ll_func.chip_op) {
		SAL_TRACE(OSP_DBL_MAJOR, 308, "Low level function table is NULL!");
		ret = sal_reset_chip_func_exit(tmp_card);
		sal_put_card(tmp_card);
		return ret;
	}

	/* stop discover operation of all the port  */
	sal_notify_disc_all_port_down(tmp_card);
	sal_wait_all_port_disc_over(card_id);
    
	/* stop all phys */
	ret = sal_stop_all_phy(tmp_card);
	if (OK != ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 307, "Card:%d stop all phy failed",
			  card_id);
		(void)sal_repair_operation(tmp_card);
	}

	/* set flag to mark sas controller is resetting */
	spin_lock_irqsave(&tmp_card->card_lock, flag);
	tmp_card->flag &= ~SAL_CARD_ACTIVE;
	tmp_card->flag |= SAL_CARD_RESETTING;
	spin_unlock_irqrestore(&tmp_card->card_lock, flag);

	/* reset sas controller */
	soft_mode = SAL_SOFT_RESET_HW;
	ret =
	    tmp_card->ll_func.chip_op(tmp_card, SAL_CHIP_OPT_SOFTRESET,
				      &soft_mode);
	if (OK != ret) {	
		SAL_TRACE(OSP_DBL_MAJOR, 309, "reset card:%d failed", card_id);

		if (RETURN_OPERATION_FAILED == ret) {
			SAL_TRACE(OSP_DBL_MAJOR, 309,
				  "chip fatal err happen when resetting...");

			(void)sal_comm_dump_fw_info(tmp_card->card_no,
						    SAL_ERROR_INFO_ALL);

		}
		ret = sal_reset_chip_func_exit(tmp_card);
		sal_put_card(tmp_card);
		return ret;
	}

	/* clear all error handle notify, except JAM which need resend */
	sal_clean_err_handler_notify(tmp_card);

	/* free all tmo msg after chip reset */
	sal_recover_tmo_msg(tmp_card);
    
	/* init resource after chip reset */
	sal_init_rsc_after_chip_reset(tmp_card);

	spin_lock_irqsave(&tmp_card->card_lock, flag);
	tmp_card->flag &= ~SAL_CARD_RESETTING;
	tmp_card->flag |= SAL_CARD_ACTIVE;
	tmp_card->reseting = SAL_FALSE;
	spin_unlock_irqrestore(&tmp_card->card_lock, flag);

	/* start work, start all phys */
	ret = sal_start_work((void *)(u64) card_id, NULL);
	if (OK != ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 309, "Card:%d start work failed",
			  card_id);
		sal_put_card(tmp_card);
		return ERROR;
	}

	sal_send_raw_ctrl_no_wait(tmp_card->card_no,
				  (void *)(u64) tmp_card->card_no,
				  NULL, sal_comm_get_chip_info);

	sal_put_card(tmp_card);
	return ret;

}

void sal_clr_phy_err_stat(struct sal_card *card, u32 phy_id)
{
	SAL_TRACE(OSP_DBL_INFO, 240, "card:%d clear phy:%2d err stat",
		  card->card_no, phy_id);

	card->phy[phy_id]->err_code.inv_dw_chg_cnt = 0;
	card->phy[phy_id]->err_code.invalid_dw_cnt = 0;
	card->phy[phy_id]->err_code.disparity_err_cnt = 0;
	card->phy[phy_id]->err_code.disp_err_chg_cnt = 0;
	card->phy[phy_id]->err_code.cod_vio_chg_cnt = 0;
	card->phy[phy_id]->err_code.code_viol_cnt = 0;
	card->phy[phy_id]->err_code.loss_dw_syn_chg_cnt = 0;
	card->phy[phy_id]->err_code.loss_dw_synch_cnt = 0;
	card->phy[phy_id]->err_code.phy_chg_cnt_per_period = 0;
	card->phy[phy_id]->err_code.phy_chg_cnt = 0;
}

EXPORT_SYMBOL(sal_clr_phy_err_stat);

s32 sal_comm_dump_fw_info(u32 card_id, u32 type)
{
	struct sal_card *card = NULL;
	s32 ret = ERROR;

	card = sal_get_card(card_id);
	if (NULL == card) {
		SAL_TRACE(OSP_DBL_MAJOR, 362, "get card failed by card id:%d",
			  card_id);
		return ERROR;
	}

	if (NULL != card->ll_func.chip_op) {

		ret =
		    card->ll_func.chip_op(card, SAL_CHIP_OPT_DUMP_LOG,
					  (void *)&type);
	} else {
		SAL_TRACE(OSP_DBL_MAJOR, 363, "card->ll_func.chip_op is NULL");
		ret = ERROR;
	}

	if (OK == ret)
		sal_global.fw_dump_idx[card_id] =
		    (sal_global.fw_dump_idx[card_id] + 1) % MAX_FW_DUMP_INDEX;

	sal_put_card(card);
	return ret;
}

s32 sal_fill_smp_frame(struct disc_smp_req * disc_smp_req,
		       u32 phy_id, u32 func, u32 phy_op)
{
	disc_smp_req->smp_frame_type = SMP_REQUEST;

	switch (func) {
	case SMP_DISCOVER_REQUEST:
		disc_smp_req->smp_function = SMP_DISCOVER_REQUEST;
		disc_smp_req->req_type.discover_req.phy_identifier =
		    (u8) phy_id;
		break;

	case SMP_REPORT_GENERAL:
		disc_smp_req->smp_function = SMP_REPORT_GENERAL;
		break;

	case SMP_REPORT_PHY_SATA:
		disc_smp_req->smp_function = SMP_REPORT_PHY_SATA;
		disc_smp_req->req_type.report_phy_sata.phy_identifier =
		    (u8) phy_id;
		break;

	case SMP_PHY_CONTROL:
		disc_smp_req->smp_function = SMP_PHY_CONTROL;
		disc_smp_req->req_type.phy_control.phy_identifier = (u8) phy_id;
		disc_smp_req->req_type.phy_control.phy_operation = (u8) phy_op;
		break;

	default:
		SAL_TRACE(OSP_DBL_MAJOR, 480, "Invalid func:0x%x", func);
		return ERROR;
	}

	return OK;
}

/**
 * sal_comm_send_smp - common interface used to send smp frame
 *
 * @argv_in:argument value in
 * @argv_out:argument value out
 */
s32 sal_comm_send_smp(void *argv_in, void *argv_out)
{
	s32 ret = ERROR;
	struct sal_dev *dev = NULL;
	struct sal_port *port = NULL;
	u32 req_len = 0;
	u32 resp_len = 0;
	struct sal_card *card = NULL;
	struct sal_mml_send_smp_info *info = NULL;
	struct disc_smp_req smp_req;
	struct disc_smp_req smp_req_2;

	SAL_REF(argv_out);	

	memset(&smp_req, 0, sizeof(struct disc_smp_req));
	memset(&smp_req_2, 0, sizeof(struct disc_smp_req));

	SAL_ASSERT(NULL != argv_in, return ERROR);
	info = (struct sal_mml_send_smp_info *)argv_in;

	card = sal_get_card(info->card_id);
	if (NULL == card) {
		SAL_TRACE(OSP_DBL_MAJOR, 481, "get card failed by card id:%d",
			  info->card_id);
		return ERROR;
	}

	port = sal_find_port_by_logic_port_id(card, info->port_id);
	if (NULL == port) {
		SAL_TRACE(OSP_DBL_MAJOR, 481, "Card:%d find port:0x%x failed",
			  card->card_no, info->port_id);

		sal_put_card(card);
		return ERROR;
	}

	dev = sal_get_dev_by_sas_addr(port, info->sas_addr);
	if (NULL == dev) {
		SAL_TRACE(OSP_DBL_MAJOR, 482, "can't find dev by addr:0x%llx",
			  info->sas_addr);

		sal_put_card(card);
		return ERROR;
	}

	/* expander? */
	if (SAL_DEV_TYPE_SMP != dev->dev_type) {
		SAL_TRACE(OSP_DBL_MAJOR, 483,
			  "Card:%d device 0x%llx is not expander",
			  card->card_no, dev->sas_addr);

		sal_put_dev(dev);
		sal_put_card(card);
		return ERROR;
	}

	ret = sal_fill_smp_frame(&smp_req,
				 info->phy_id, info->func, info->phy_op);
	if (ERROR == ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 484, "fill SMP Frame failed");

		sal_put_dev(dev);
		sal_put_card(card);
		return ERROR;
	}

	req_len = sal_get_smp_req_len((u8) info->func);
	resp_len = sal_get_smp_req_len((u8) info->func);

	if ((SAL_SMP_LEN_INVALID == req_len)
	    || (SAL_SMP_LEN_INVALID == resp_len)) {
		SAL_TRACE(OSP_DBL_MAJOR, 485, "the func is invalid");

		sal_put_dev(dev);
		sal_put_card(card);
		return ERROR;
	}

	ret = sal_send_smp(dev,
			   (u8 *) & smp_req,
			   req_len,
			   (u8 *) & smp_req_2,
			   resp_len, sal_default_smp_wait_func);

	if (ERROR == ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 486, "send SMP Frame failed");

		sal_put_dev(dev);
		sal_put_card(card);
		return ERROR;
	}

	sal_put_dev(dev);
	sal_put_card(card);
	return OK;
}

s32 sal_alloc_dma_mem(struct sal_card * sal_card,
		      void **virt_addr,
		      unsigned long *phy_addr, u32 size, s32 direct)
{
	void *tmp_buff = NULL;
	unsigned long tmp_pa = 0;

	tmp_buff = SAS_KMALLOC(size, GFP_ATOMIC);
	if (NULL == tmp_buff) {
		SAL_TRACE(OSP_DBL_MAJOR, 4425, "Card:%d allocate memory failed",
			  sal_card->card_no);
		return ERROR;
	}

	memset(tmp_buff, 0, size);
	tmp_pa = (unsigned long)pci_map_single(sal_card->pci_device,
						   tmp_buff, size, direct);
	if (pci_dma_mapping_error(sal_card->pci_device, tmp_pa)) {
		SAL_TRACE(OSP_DBL_MAJOR, 4425, "Card:%d map single failed",
			  sal_card->card_no);

		SAS_KFREE(tmp_buff);
		return ERROR;
	}

	*phy_addr = tmp_pa;
	*virt_addr = tmp_buff;

	return OK;

}

void sal_free_dma_mem(struct sal_card *sal_card,
		      void *virt_addr,
		      unsigned long phy_addr, u32 size, s32 direct)
{
	SAL_ASSERT(NULL != sal_card, return);

	pci_unmap_single(sal_card->pci_device, phy_addr, size, direct);
	SAS_KFREE(virt_addr);
	return;
}

s32 sal_comm_clean_up_port_rsc(struct sal_card *sal_card, struct sal_port *port)
{
	s32 ret = ERROR;
	struct sal_local_port_ctl info;

	if (NULL == sal_card->ll_func.port_op) {
		SAL_TRACE(OSP_DBL_MAJOR, 313, "Card:%d Func port opt is NULL",
			  sal_card->card_no);
		return ERROR;
	}

	memset(&info, 0, sizeof(struct sal_local_port_ctl));
	info.port_id = port->port_id;
	info.op_code = SAL_PORT_CTRL_E_CLEAN_UP;
	info.param0 = 0;

	ret =
	    sal_card->ll_func.port_op(sal_card, SAL_PORT_OPT_LOCAL_CONTROL,
				      (void *)&info);
	return ret;
}

s32 osal_lib_init_timer(struct osal_timer_list *osal_timer, 
    unsigned long expires, unsigned long data, osal_timer_cb_t timer_cb)
{
    if (NULL == timer_cb || NULL == osal_timer) {
        printk("Input param invalid.");
        return ERROR;
    }

    if (NULL != osal_timer->std_timer.entry.next) {
        printk("Repeat init timer.");
        dump_stack();
        return ERROR;
    }
    
    init_timer(&osal_timer->std_timer);

    osal_timer->std_timer.data = data;
    osal_timer->std_timer.function = timer_cb;
    osal_timer->expires = OSAL_MS_TO_JIFFIES(expires);
    
    return OK;
}
EXPORT_SYMBOL(osal_lib_init_timer);

s32 osal_lib_add_timer(struct osal_timer_list *osal_timer)
{
    if (NULL == osal_timer) {
        printk("Input param invalid."); 
        return ERROR;
    }

    if (NULL != osal_timer->std_timer.entry.next) {
        printk("Repeat add timer.");
        dump_stack();
        return ERROR;
    }

    osal_timer->std_timer.expires = osal_timer->expires + jiffies;
    add_timer(&osal_timer->std_timer);

    return OK;
}
EXPORT_SYMBOL(osal_lib_add_timer);

s32 osal_lib_mod_timer(struct osal_timer_list *timer, unsigned long expire_time )
{
    if (NULL == timer) {
        printk("Input param invalid.");
        return ERROR;
    }
    /* expire_time: time unit is ms */
    mod_timer(&timer->std_timer, jiffies + OSAL_MS_TO_JIFFIES(expire_time));
    
    return OK;
}
EXPORT_SYMBOL(osal_lib_mod_timer);

s32 osal_lib_del_timer(struct osal_timer_list *osal_timer )
{
    if (NULL == osal_timer) {
        printk("Input param invalid.");
        return ERROR;
    }
    
    del_timer(&osal_timer->std_timer);
    
    return OK;
}
EXPORT_SYMBOL(osal_lib_del_timer);

s32 osal_lib_del_timer_sync(struct osal_timer_list *osal_timer )
{
	if (NULL == osal_timer )
	{
		printk("Input param invalid.");
		return ERROR;
	}

	del_timer_sync(&osal_timer->std_timer);
	
	return OK;
}
EXPORT_SYMBOL(osal_lib_del_timer_sync);

bool osal_lib_is_timer_activated(struct osal_timer_list *osal_timer )
{
    if (NULL == osal_timer) {
        printk("Input param invalid.");
        return false;
    }

    return timer_pending(&osal_timer->std_timer);
}
EXPORT_SYMBOL(osal_lib_is_timer_activated);


void sal_add_timer(void *timer, unsigned long data,
		   u32 time, sal_timer_cbfn_t timer_cb)
{
	unsigned long expire = 0;

	SAL_ASSERT(NULL != timer, return);

	expire = ((unsigned long)time * 1000) / HZ;	

	(void)osal_lib_init_timer((struct osal_timer_list *) timer, expire, data,
			     timer_cb);
	(void)osal_lib_add_timer((struct osal_timer_list *) timer);

	return;
}

EXPORT_SYMBOL(sal_add_timer);

void sal_mod_timer(void *timer, unsigned long expires)
{

	unsigned long expire_time = 0;

	expire_time = ((unsigned long)expires * 1000) / HZ;
	SAL_ASSERT(NULL != timer, return);

	(void)osal_lib_mod_timer((struct osal_timer_list *) timer, expire_time);
}

EXPORT_SYMBOL(sal_mod_timer);

bool sal_timer_is_active(void *timer)
{
	return osal_lib_is_timer_activated((struct osal_timer_list *) timer);
}

EXPORT_SYMBOL(sal_timer_is_active);

void sal_del_timer(void *timer)
{

	if (OK != osal_lib_del_timer_sync((struct osal_timer_list *) timer)) {

		SAL_TRACE(OSP_DBL_MAJOR, 768, "Delete timer:%p failed", timer);
	}

}

EXPORT_SYMBOL(sal_del_timer);

void get_date(char *date)
{
    struct timex  txc;
    struct rtc_time tm;
    
    do_gettimeofday(&(txc.time));
    rtc_time_to_tm(txc.time.tv_sec,&tm);

    (void)snprintf(date, TIME_STRING_LEN, "%4d-%02d-%02d %02d:%02d:%02d",
        tm.tm_year + 1900,tm.tm_mon + 1, tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);

    return;
}

void log_kproc(int module_id, int level, int msg_code, const char *funcName, int line, const char * format, ...)            
{
    char* log_msg = NULL;
    va_list vptr;
    static unsigned int seq_num = 0;

    char log_msg_u[K_LOG_MSG_LENGTH];    
    char   sys_date[TIME_STRING_LEN];
    char   task_name[TASK_NAME_LEN];     

    get_date(sys_date);

    if (in_interrupt()){
        (void)strncpy(task_name, "ISR", TASK_NAME_LEN - 1);
    } else {
        (void)snprintf(task_name, TASK_NAME_LEN, "%d", current->pid);
    }

    log_msg = log_msg_u;

    va_start(vptr, format);
    (void)vsnprintf(log_msg, K_LOG_MSG_LENGTH, format, vptr);
    va_end(vptr);

    seq_num++;
    
    printk("%d,%d,%d,%s,%s,%s,%d,%s\n", 
		module_id, msg_code, seq_num, sys_date, task_name, funcName, line, log_msg);

    return;
}

EXPORT_SYMBOL(log_kproc);


