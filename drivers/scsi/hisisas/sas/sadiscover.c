/*
 * Huawei Pv660/Hi1610 sas controller discover process
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


struct disc_card disc_card_table[DISC_MAX_CARD_NUM];
struct disc_event_intf disc_event_func_table[DISC_MAX_CARD_NUM];

void sal_disc_expander(struct disc_exp_data *exp);
void sal_check_new_disc(struct disc_port *port);
s32 sal_check_port_valid(struct disc_port *port);
s32 sal_check_atta_dev(struct disc_device_data *dev,
		       struct disc_device_data *attadev);
struct disc_device_data *sal_notify_new_dev(struct disc_device_data *dev,
					    struct disc_device_data *new_dev);
void sal_add_routing_entry(struct disc_exp_data *config_exp,
			   u8 phyid, u64 config_sas_addr);



void sal_hex_dump(char *hdr, u8 * buf, u32 len)
{
	u32 i = 0;
	u32 rec = 0;

	if (NULL == buf) {
		printk("buffer is null\n");
		return;
	}
	printk("%s\n", hdr);
	for (i = 0; i < len;) {
		if ((len - i) >= 4) {
			printk("%04x 0x%02x, 0x%02x, 0x%02x, 0x%02x \n",
			       i, buf[(u32) i], buf[(u32) (i + 1)],
			       buf[(u32) (i + 2)], buf[(u32) (i + 3)]);
			i += 4;
			continue;
		}

		if (0x00 == rec) {
			printk("%04x", i);
			rec++;
		}

		printk(" 0x%02x ", buf[i]);
		i++;
	}
	printk("\n");
}

/**
 * sal_get_smp_req_len - obtain a smp request cmd len
 * @func: smp request cmd type
 *
 */
u32 sal_get_smp_req_len(u8 func)
{
	u32 req_len = 0;

	switch (func) {
	case SMP_REPORT_GENERAL:
		req_len = sizeof(struct smp_report_general) + SAS_SMP_HEADR_LEN;
		break;
	case SMP_DISCOVER_REQUEST:
		req_len = sizeof(struct smp_discover) + SAS_SMP_HEADR_LEN;
		break;
	case SMP_REPORT_PHY_SATA:
		req_len =
		    sizeof(struct smp_report_phy_sata) + SAS_SMP_HEADR_LEN;
		break;
	case SMP_CONFIG_ROUTING_INFO:
		req_len =
		    sizeof(struct smp_config_routing_info) + SAS_SMP_HEADR_LEN;
		break;
	case SMP_PHY_CONTROL:
		req_len = sizeof(struct smp_phy_control) + SAS_SMP_HEADR_LEN;
		break;
	case SMP_DISCOVER_LIST:
		req_len = sizeof(struct smp_discover_list) + SAS_SMP_HEADR_LEN;
		break;

	default:
		req_len = 0;
		break;
	}
	return req_len;
}

/**
 * sal_get_smp_resp_len - obtain a smp respond msg len
 * @func: smp respond cmd type
 *
 */
u32 sal_get_smp_resp_len(u32 func)
{
	u32 rsp_len = 0;

	switch (func) {
	case SMP_REPORT_GENERAL:
		rsp_len =
		    sizeof(struct smp_report_general_rsp) + SAS_SMP_HEADR_LEN;
		break;
	case SMP_DISCOVER_REQUEST:
		rsp_len = sizeof(struct smp_discover_rsp) + SAS_SMP_HEADR_LEN;
		break;

	case SMP_REPORT_PHY_SATA:
		rsp_len =
		    sizeof(struct smp_report_phy_sata_rsp) + SAS_SMP_HEADR_LEN;
		break;
	case SMP_CONFIG_ROUTING_INFO:
		rsp_len =
		    sizeof(struct smp_config_routing_info_rsp) +
		    SAS_SMP_HEADR_LEN;
		break;
	case SMP_PHY_CONTROL:
		rsp_len = SAS_SMP_HEADR_LEN;
		break;
	default:
		rsp_len = 0;
		break;
	}
	return rsp_len;
}

/**
 * sal_reinit_exp - init a expander node resource
 * @exp: point to a expander
 *
 */
void sal_reinit_exp(struct disc_exp_data *exp)
{
	u32 i = 0;

	for (i = 0; i < DISC_MAX_EXPANDER_PHYS; i++) {
		exp->downstream_phy[i] = 0;
		exp->upstream_phy[i] = 0;
		exp->routing_attr[i] = SAS_BUTT_ROUTING;
		exp->curr_phy_idx[i] = 0;
		exp->phy_chg_cnt[i] = 0;
		exp->last_sas_addr[i] = 0;
	}

	exp->curr_downstream_exp = NULL;
	exp->device = NULL;
	exp->downstream_exp = NULL;
	exp->return_exp = NULL;
	exp->upstream_exp = NULL;
	exp->discovering_phy_id = 0;
	exp->send_smp_phy_id = 0;
	exp->flag = DISC_EXP_NOT_PROCESS;
	exp->num_of_downstream_phys = 0;
	exp->num_of_upstream_phys = 0;
	exp->phy_nums = 0;
	exp->exp_chg_cnt = 0;
	exp->retries = 0;
	exp->smp_retry_num = 0;
	exp->curr_upstream_phy_idx = 0;
	exp->curr_downstream_phy_idx = 0;
}


void sal_reinit_dev(struct disc_device_data *dev)
{
	dev->expander = NULL;
	dev->dev_type = NO_DEVICE;
	dev->link_rate = DISC_LINK_RATE_FREE;
	dev->valid_before = DISC_RESULT_FALSE;
	dev->valid_now = DISC_RESULT_FALSE;
	dev->reg_dev_flag = SAINI_NOT_REGDEV;
}

/**
 * sal_reinit_exp - obtain a expander node resource by flag
 * @port: point to the port
 * @status_flag: expander flag
 */
struct disc_exp_data *sal_get_exp_by_flag(struct disc_port *port,
					  u8 status_flag)
{
	unsigned long flag = 0;
	struct disc_card *card = NULL;
	struct disc_exp_data *expander = NULL;
	struct disc_exp_data *exp = NULL;
	struct list_head *list = NULL;
	struct list_head *list_tmp = NULL;

	SAL_ASSERT(NULL != port, return exp);

	card = (struct disc_card *)port->card;

	spin_lock_irqsave(&card->expander_lock, flag);
	if (SAS_LIST_EMPTY(&port->disc_expander)) {
		SAL_TRACE(OSP_DBL_MAJOR, 935,
			  "Card %d port %u expander list is empty",
			  card->card_no, port->port_id);
		exp = NULL;
	} else {
		SAS_LIST_FOR_EACH_SAFE(list, list_tmp, &port->disc_expander) {
			expander =
			    SAS_LIST_ENTRY(list, struct disc_exp_data,
					   exp_list);
			if (status_flag == expander->flag) {
				exp = expander;
				break;
			}
		}
	}

	spin_unlock_irqrestore(&card->expander_lock, flag);
	return exp;
}

/**
 * sal_find_dev_by_sas_addr - obtain dev by sas address
 * @sas_addr: sas address
 * @port: point to the port
 */
struct disc_device_data *sal_find_dev_by_sas_addr(u64 sas_addr,
						  struct disc_port *port)
{
	struct disc_card *card = NULL;
	struct list_head *list = NULL;
	struct list_head *list_tmp = NULL;
	struct disc_device_data *dev = NULL;
	struct disc_device_data *dev_return = NULL;
	unsigned long flag = 0;

	SAL_ASSERT(NULL != port, return NULL);
	card = (struct disc_card *)port->card;

	/*check if link table empty*/
	spin_lock_irqsave(&card->device_lock, flag);
	if (SAS_LIST_EMPTY(&port->disc_device)) {
		spin_unlock_irqrestore(&card->device_lock, flag);
		return NULL;
	}

	SAS_LIST_FOR_EACH_SAFE(list, list_tmp, &port->disc_device) {
		dev =
		    SAS_LIST_ENTRY(list, struct disc_device_data, device_link);
		if (sas_addr == dev->sas_addr) {
			dev_return = dev;
			break;
		}
	}
	spin_unlock_irqrestore(&card->device_lock, flag);
	return dev_return;
}

/**
 * sal_find_exp_by_sas_addr - obtain expander by sas address
 * @sas_addr: sas address
 * @port: point to the port
 */
struct disc_exp_data *sal_find_exp_by_sas_addr(struct disc_port *port,
					       u64 sas_addr)
{
	struct disc_card *card = NULL;
	struct list_head *list = NULL;
	struct list_head *list_tmp = NULL;
	struct disc_exp_data *exp = NULL;
	struct disc_exp_data *ret_exp = NULL;
	unsigned long flag = 0;

	SAL_ASSERT(NULL != port, return NULL);
	card = (struct disc_card *)port->card;

	spin_lock_irqsave(&card->expander_lock, flag);
	if (SAS_LIST_EMPTY(&port->disc_expander)) {
		spin_unlock_irqrestore(&card->expander_lock, flag);
		return NULL;
	}

	SAS_LIST_FOR_EACH_SAFE(list, list_tmp, &port->disc_expander) {
		exp = SAS_LIST_ENTRY(list, struct disc_exp_data, exp_list);
		if (sas_addr == exp->sas_addr) {
			ret_exp = exp;
			break;
		}
	}
	spin_unlock_irqrestore(&card->expander_lock, flag);
	return ret_exp;
}

void sal_add_exp_list(struct disc_exp_data *expander)
{
	unsigned long flag = 0;
	u8 card_id = 0;
	u32 port_id = 0;
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;

	SAL_ASSERT(NULL != expander, return);
	card_id = expander->card_id;
	port_id = expander->port_id;
	card = &disc_card_table[card_id];
	port = &card->disc_port[port_id];
	SAL_TRACE(OSP_DBL_INFO, 924,
		  "Card:%d port:%u add expander:0x%llx to expander list",
		  card_id, port_id, expander->sas_addr);

	spin_lock_irqsave(&card->expander_lock, flag);
	SAS_LIST_ADD_TAIL(&expander->exp_list, &port->disc_expander);
	spin_unlock_irqrestore(&card->expander_lock, flag);
}

void sal_clear_port_rsc(u8 card_id, u32 port_id)
{
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	struct list_head *list = NULL;
	struct list_head *list_tmp = NULL;
	struct disc_device_data *dev_tmp = NULL;
	struct disc_exp_data *exp_tmp = NULL;
	unsigned long flag = 0;
	unsigned long exp_flag = 0;

	SAL_ASSERT(card_id < DISC_MAX_CARD_NUM, return);
	SAL_ASSERT(port_id < DISC_MAX_PORT_NUM, return);

	card = &disc_card_table[card_id];
	port = &card->disc_port[port_id];

	SAL_TRACE(OSP_DBL_INFO, 903,
		  "Card:%d port:%u clr port rsc,remove all device from device list of Disc",
		  card_id, port_id);

	spin_lock_irqsave(&card->device_lock, flag);
	if (!SAS_LIST_EMPTY(&port->disc_device)) {
		SAS_LIST_FOR_EACH_SAFE(list, list_tmp, &port->disc_device) {
			dev_tmp = SAS_LIST_ENTRY(list,
						 struct disc_device_data,
						 device_link);
			sal_reinit_dev(dev_tmp);
			SAS_LIST_DEL_INIT(&dev_tmp->device_link);
			SAS_LIST_ADD_TAIL(&dev_tmp->device_link,
					  &card->free_device);
		}
	}
	spin_unlock_irqrestore(&card->device_lock, flag);

	spin_lock_irqsave(&card->expander_lock, exp_flag);
	if (!SAS_LIST_EMPTY(&port->disc_expander)) {
		SAS_LIST_FOR_EACH_SAFE(list, list_tmp, &port->disc_expander) {
			exp_tmp =
			    SAS_LIST_ENTRY(list, struct disc_exp_data,
					   exp_list);
			sal_reinit_exp(exp_tmp);
			SAS_LIST_DEL_INIT(&exp_tmp->exp_list);
			SAS_LIST_ADD_TAIL(&exp_tmp->exp_list,
					  &card->free_expander);
		}
	}
	spin_unlock_irqrestore(&card->expander_lock, exp_flag);

	return;
}

struct disc_exp_data *sal_get_free_expander(struct disc_card *card)
{
	unsigned long flag = 0;
	struct disc_exp_data *expander = NULL;

	SAL_ASSERT(NULL != card, return NULL);
	spin_lock_irqsave(&card->expander_lock, flag);
	if (SAS_LIST_EMPTY(&card->free_expander)) {
		SAL_TRACE(OSP_DBL_MAJOR, 923,
			  "Card:%d get free expander failed", card->card_no);
		spin_unlock_irqrestore(&card->expander_lock, flag);
		return NULL;
	}

	expander = SAS_LIST_ENTRY(card->free_expander.next,
				  struct disc_exp_data, exp_list);
	SAS_LIST_DEL_INIT(&expander->exp_list);
	spin_unlock_irqrestore(&card->expander_lock, flag);
	return expander;
}

void sal_combine_reg_dev(struct disc_port *port)
{
	struct disc_card *card = NULL;
	struct list_head *list = NULL;
	struct list_head *list_tmp = NULL;
	struct disc_device_data *dev = NULL;
	unsigned long flag = 0;
	struct disc_event_intf *event_func = NULL;
	s32 ret = ERROR;

	card = (struct disc_card *)port->card;

	event_func = &disc_event_func_table[card->card_no];

	spin_lock_irqsave(&card->device_lock, flag);
	if (SAS_LIST_EMPTY(&port->disc_device)) {
		spin_unlock_irqrestore(&card->device_lock, flag);
		return;
	}

	SAS_LIST_FOR_EACH_SAFE(list, list_tmp, &port->disc_device) {
		dev =
		    SAS_LIST_ENTRY(list, struct disc_device_data, device_link);
		if ((EXPANDER_DEVICE != dev->dev_type)
		    && (SAINI_NEED_REGDEV == dev->reg_dev_flag)) {
			ret = event_func->device_arrival(dev);	/*sal_disc_add_disk */
			if (ERROR == ret) {
				SAL_TRACE(OSP_DBL_MAJOR, 143,
					  "Card:%d port:%d add device:0x%llx failed",
					  dev->card_id, dev->port_id,
					  dev->sas_addr);

				SAL_TRACE(OSP_DBL_INFO, 925,
					  "Card:%d port:%u delete device:0x%llx from device list of Disc Port:%d",
					  card->card_no,
					  dev->port_id,
					  dev->sas_addr, port->port_id);
				SAS_LIST_DEL_INIT(&dev->device_link);
				sal_reinit_dev(dev);
				SAS_LIST_ADD_TAIL(&dev->device_link,
						  &card->free_device);
			} else {
				dev->valid_before = DISC_RESULT_TRUE;
				dev->valid_now = DISC_RESULT_FALSE;
			}
		}
	}
	spin_unlock_irqrestore(&card->device_lock, flag);
	return;
}


void sal_combine_dereg_dev(struct disc_port *port)
{
	struct disc_card *card = NULL;
	struct list_head *list = NULL;
	struct list_head *list_tmp = NULL;
	struct disc_device_data *dev = NULL;
	unsigned long flag = 0;
	struct disc_event_intf *event_func = NULL;

	card = (struct disc_card *)port->card;
	event_func = &disc_event_func_table[card->card_no];

	spin_lock_irqsave(&card->device_lock, flag);
	if (SAS_LIST_EMPTY(&port->disc_device)) {
		spin_unlock_irqrestore(&card->device_lock, flag);
		return;
	}

	SAS_LIST_FOR_EACH_SAFE(list, list_tmp, &port->disc_device) {
		dev =
		    SAS_LIST_ENTRY(list, struct disc_device_data, device_link);
		if ((END_DEVICE == dev->dev_type)
		    && (SAINI_FREE_REGDEV == dev->reg_dev_flag)
		    && (dev->valid_before == DISC_RESULT_TRUE)
		    && (dev->valid_now == DISC_RESULT_FALSE)) {
			SAL_TRACE(OSP_DBL_MAJOR, 906,
				  "Card:%d port:%d will send Event-Q to dereg dev:0x%llx and clr rsc in disc port",
				  card->card_no, port->port_id, dev->sas_addr);

			event_func->device_remove(dev);	/*sal_disc_del_disk */

			sal_reinit_dev(dev);
			SAS_LIST_DEL_INIT(&dev->device_link);
			SAS_LIST_ADD_TAIL(&dev->device_link,
					  &card->free_device);
		}
	}
	spin_unlock_irqrestore(&card->device_lock, flag);
	return;
}


void sal_combine_dereg_smp_dev(struct disc_port *port)
{
	struct disc_card *card = NULL;
	struct list_head *list = NULL;
	struct list_head *list_tmp = NULL;
	struct disc_device_data *dev = NULL;
	unsigned long flag = 0;
	struct disc_event_intf *event_func = NULL;
	struct disc_exp_data *exp_tmp = NULL;
	unsigned long exp_flag = 0;

	card = (struct disc_card *)port->card;
	event_func = &disc_event_func_table[card->card_no];

	spin_lock_irqsave(&card->device_lock, flag);
	if (SAS_LIST_EMPTY(&port->disc_device)) {
		spin_unlock_irqrestore(&card->device_lock, flag);
		return;
	}

	SAS_LIST_FOR_EACH_SAFE(list, list_tmp, &port->disc_device) {
		dev =
		    SAS_LIST_ENTRY(list, struct disc_device_data, device_link);
		if ((EXPANDER_DEVICE == dev->dev_type)
		    && (SAINI_FREE_REGDEV == dev->reg_dev_flag)
		    && (dev->valid_before == DISC_RESULT_TRUE)
		    && (dev->valid_now == DISC_RESULT_FALSE)) {
			SAL_TRACE(OSP_DBL_MAJOR, 906,
				  "Card:%d port:%d will send Event-Q to dereg SMP dev:0x%llx and clr rsc in disc port",
				  card->card_no, port->port_id, dev->sas_addr);

			event_func->device_remove(dev);	/*sal_disc_del_disk */

			/*if it is expander,then remove disc device and disc expander*/
			exp_tmp = (struct disc_exp_data *)dev->expander;

			spin_lock_irqsave(&card->expander_lock, exp_flag);
			sal_reinit_exp(exp_tmp);
			SAS_LIST_DEL_INIT(&exp_tmp->exp_list);
			SAS_LIST_ADD_TAIL(&exp_tmp->exp_list,
					  &card->free_expander);
			spin_unlock_irqrestore(&card->expander_lock, exp_flag);

			sal_reinit_dev(dev);
			SAS_LIST_DEL_INIT(&dev->device_link);
			SAS_LIST_ADD_TAIL(&dev->device_link,
					  &card->free_device);
		}
	}
	spin_unlock_irqrestore(&card->device_lock, flag);
	return;
}


void sal_smp_dev_done_st_chg(struct disc_port *port)
{
	struct disc_card *card = NULL;
	struct list_head *list = NULL;
	struct list_head *list_tmp = NULL;
	struct disc_device_data *dev = NULL;
	unsigned long flag = 0;

	card = (struct disc_card *)port->card;

	spin_lock_irqsave(&card->device_lock, flag);
	if (SAS_LIST_EMPTY(&port->disc_device)) {
		spin_unlock_irqrestore(&card->device_lock, flag);
		return;
	}

	SAS_LIST_FOR_EACH_SAFE(list, list_tmp, &port->disc_device) {
		dev =
		    SAS_LIST_ENTRY(list, struct disc_device_data, device_link);
		if ((EXPANDER_DEVICE == dev->dev_type)
		    && (SAINI_FREE_REGDEV == dev->reg_dev_flag)) {
			dev->valid_before = DISC_RESULT_TRUE;
			dev->valid_now = DISC_RESULT_FALSE;
		}
	}
	spin_unlock_irqrestore(&card->device_lock, flag);
	return;

}

/**
 * sal_notify_disc_done - notify discover done,success or failure
 * @port: point to the port
 * @disc_flag: success or failure
 */
void sal_notify_disc_done(struct disc_port *port, u8 disc_flag)
{
	struct disc_card *card = NULL;
	struct disc_event_intf *event_func = NULL;
	u8 card_id = 0;
	unsigned long flag = 0;
	struct list_head *list = NULL;
	struct list_head *list_tmp = NULL;
	struct disc_exp_data *exp = NULL;
	char *str[] = {
		"Successful",
		"Failed",
		"unknown"
	};

	SAL_ASSERT(NULL != port, return);
	card = (struct disc_card *)port->card;
	card_id = card->card_no;
	event_func = &disc_event_func_table[card_id];

	/*chip is resetting or port is invailed, return.*/
	if (PORT_STATUS_PERMIT != port->disc_switch) {
		SAL_TRACE(OSP_DBL_INFO, 905,
			  "Card:%d port:%d is not up,port status:%d", card_id,
			  port->port_id, port->disc_switch);
		port->disc_switch = PORT_STATUS_FORBID;
		/*must set discover status to not start*/
		port->discover_status = DISC_STATUS_NOT_START;
		port->new_disc = DISC_RESULT_FALSE;

		sal_clear_port_rsc(card_id, port->port_id);
		return;
	}

	SAL_TRACE(OSP_DBL_INFO, 906,
		  "Card:%d port:%u(status:0x%x) discover done with result:0x%x(%s)",
		  card_id,
		  port->port_id, port->disc_switch, disc_flag, str[disc_flag]);

	if (DISC_SUCESSED_DONE == disc_flag) {
		/*success, register device*/
		sal_combine_reg_dev(port);
	} else {
		/*failure,remove dievice and expander resource*/
		SAL_TRACE(OSP_DBL_MAJOR, 906,
			  "Card:%d Port:%d disc failed,go to clr rsc", card_id,
			  port->port_id);

		sal_combine_dereg_dev(port);

		sal_combine_dereg_smp_dev(port);

		sal_combine_reg_dev(port);

		sal_smp_dev_done_st_chg(port);
	}

	event_func->discover_done(card_id, port->port_id);	/*sal_disc_done */

	spin_lock_irqsave(&card->device_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(list, list_tmp, &port->disc_expander) {
		exp = SAS_LIST_ENTRY(list, struct disc_exp_data, exp_list);
		exp->retries = 0;
	}
	spin_unlock_irqrestore(&card->device_lock, flag);

	/*check if need to do a new discover*/
	sal_check_new_disc(port);
	return;
}

/**
 * sal_disc_del_dev - remove the device by sas address
 * @port: point to the port
 * @sas_addr: device sas address
 */
void sal_disc_del_dev(struct disc_port *port, u64 sas_addr)
{
	struct disc_card *card = NULL;
	struct list_head *list = NULL;
	struct list_head *pstListmp = NULL;
	struct disc_device_data *dev = NULL;
	unsigned long flag = 0;

	SAL_ASSERT(NULL != port, return);
	card = (struct disc_card *)port->card;

	spin_lock_irqsave(&card->device_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(list, pstListmp, &port->disc_device) {
		dev =
		    SAS_LIST_ENTRY(list, struct disc_device_data, device_link);
		if (sas_addr == dev->sas_addr) {
			SAL_TRACE(OSP_DBL_INFO, 925,
				  "Card:%d port:%u delete device:0x%llx from device list of Disc Port:%d",
				  card->card_no,
				  dev->port_id, sas_addr, port->port_id);
			SAS_LIST_DEL_INIT(&dev->device_link);
			sal_reinit_dev(dev);
			SAS_LIST_ADD_TAIL(&dev->device_link,
					  &card->free_device);
			break;
		}
	}
	spin_unlock_irqrestore(&card->device_lock, flag);
	return;
}

void sal_add_device_list(struct disc_device_data *device)
{
	unsigned long flag = 0;
	u8 card_id = 0;
	u32 port_id = 0;
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;

	SAL_ASSERT(NULL != device, return);
	card_id = device->card_id;
	port_id = device->port_id;
	card = &disc_card_table[card_id];
	port = &card->disc_port[port_id];
	
	spin_lock_irqsave(&card->device_lock, flag);
	SAS_LIST_ADD_TAIL(&device->device_link, &port->disc_device);
	spin_unlock_irqrestore(&card->device_lock, flag);
}

void sal_remove_exp_list(struct disc_port *port, u64 sas_addr)
{
	struct disc_card *card = NULL;
	struct list_head *list = NULL;
	struct list_head *list_tmp = NULL;
	struct disc_exp_data *exp = NULL;
	unsigned long flag = 0;

	SAL_ASSERT(NULL != port, return);
	card = (struct disc_card *)port->card;
	spin_lock_irqsave(&card->expander_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(list, list_tmp, &port->disc_expander) {
		exp = SAS_LIST_ENTRY(list, struct disc_exp_data, exp_list);
		if (sas_addr == exp->sas_addr) {
			sal_reinit_exp(exp);
			SAS_LIST_DEL_INIT(&exp->exp_list);
			SAS_LIST_ADD_TAIL(&exp->exp_list, &card->free_expander);
			break;
		}
	}
	spin_unlock_irqrestore(&card->expander_lock, flag);
}

struct disc_device_data *sal_get_free_device(struct disc_card *card)
{
	unsigned long flag = 0;
	struct disc_device_data *device = NULL;

	SAL_ASSERT(NULL != card, return NULL);
	spin_lock_irqsave(&card->device_lock, flag);
	if (SAS_LIST_EMPTY(&card->free_device)) {
		SAL_TRACE(OSP_DBL_MAJOR, 80,
			  "Err! Card %d get free device node failed",
			  card->card_no);
		spin_unlock_irqrestore(&card->device_lock, flag);
		return NULL;
	}

	device = SAS_LIST_ENTRY(card->free_device.next,
				struct disc_device_data, device_link);
	SAS_LIST_DEL_INIT(&device->device_link);
	spin_unlock_irqrestore(&card->device_lock, flag);
	return device;
}

/**
 * sal_add_port_dev - add device 
 * @port: point to the port
 */
struct disc_device_data *sal_add_port_dev(struct disc_port *port)
{
	s32 ret = ERROR;
	struct disc_card *card = NULL;
	struct sas_identify_frame *identify = NULL;
	struct disc_device_data *dev = NULL;
	struct disc_event_intf *event_func = NULL;

	SAL_ASSERT(NULL != port, return NULL);

	card = (struct disc_card *)port->card;
	event_func = &disc_event_func_table[card->card_no];
	dev = sal_get_free_device(card);
	if (NULL == dev) {
		SAL_TRACE(OSP_DBL_MAJOR, 920,
			  "Card:%d port:%d can't get free device node",
			  card->card_no, port->port_id);
		return NULL;
	}

	identify = &port->identify;
	/*add device info*/
	dev->dev_type = DISC_GET_DEVTYPE_FROM_IDENTIFY(identify->device_type);
	dev->ini_proto = identify->ini_proto;
	dev->tgt_proto = identify->tgt_proto;
	if (END_DEVICE == dev->dev_type) {
		/*for end device,phy id -> slot id is not change*/
#if 0
		dev->phy_id =
		    (u8) SAL_FindFirstPhyByFwPortId(card->card_no,
						    port->port_no);
#endif

	} else {
		dev->phy_id = identify->phy_identifier;
	}

	dev->card_id = card->card_no;
	dev->port_id = port->port_id;
	dev->father_sas_addr = port->local_sas_addr;
	dev->sas_addr = DISC_GET_SASADDR_FROM_IDENTIFY(identify);
	dev->link_rate = port->link_rate;
	dev->valid_before = DISC_RESULT_FALSE;
	dev->valid_now = DISC_RESULT_TRUE;

	SAL_TRACE(OSP_DBL_INFO, 82,
		  "Card:%d Port:%d discover add device:0x%llx(%s),Valid? Before:%s Now:%s",
		  card->card_no,
		  port->port_id,
		  dev->sas_addr,
		  (dev->dev_type ==
		   VIRTUAL_DEVICE) ? "virtual" : ((dev->dev_type ==
						   EXPANDER_DEVICE) ? "expander"
						  : "end"),
		  dev->valid_before ? "Yes" : "No",
		  dev->valid_now ? "Yes" : "No");

	sal_add_device_list(dev);
	dev->reg_dev_flag = SAINI_NEED_REGDEV;

    /*for end device, device register after discover done*/
	if (EXPANDER_DEVICE != dev->dev_type)
		return dev;

    /*sal_disc_add_disk */
	ret = event_func->device_arrival(dev);
	if (ERROR == ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 920,
			  "Card:%d port:%d add new device:0x%llx failed",
			  card->card_no, port->port_id, dev->sas_addr);
		sal_disc_del_dev(port, dev->sas_addr);

		ret = sal_check_port_valid(port);
		if (ERROR == ret) {
			port->discover_status = DISC_STATUS_NOT_START;
			sal_clear_port_rsc(card->card_no, port->port_id);
		}
		return NULL;
	}

	return dev;
}

/**
 * sal_check_dev_rate - device rate change check.
 * @port: point to the port
 */
s32 sal_check_dev_rate(struct disc_device_data * dev, u8 link_rate)
{
	s32 ret = ERROR;
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	struct disc_event_intf *event_func = NULL;

	SAL_ASSERT(NULL != dev, return ERROR);

	event_func = &disc_event_func_table[dev->card_id];
	card = &disc_card_table[dev->card_id];
	port = &card->disc_port[dev->port_id];

	if (link_rate == dev->link_rate)
		/*rate no change*/
		return OK;

	SAL_TRACE(OSP_DBL_MINOR, 85,
		  "Card:%d port:%d dev:0x%llx speed has changed(before:0x%x; current:0x%x)",
		  dev->card_id, dev->port_id, dev->sas_addr, dev->link_rate,
		  link_rate);

	/*end device,report device out,then report device in;
	  expander,report the lowest rate*/
	if (dev->dev_type != EXPANDER_DEVICE) {
		SAL_TRACE(OSP_DBL_MINOR, 85,
			  "Card:%d port:%d send Event-Q to dereg dev:0x%llx and then reg it by new speed after disc over",
			  dev->card_id, dev->port_id, dev->sas_addr);

		/*sal_disc_del_disk */
		event_func->device_remove(dev);

		dev->link_rate = link_rate;
		dev->reg_dev_flag = SAINI_NEED_REGDEV;
	} else {		
		if (link_rate < dev->link_rate) {
			SAL_TRACE(OSP_DBL_MINOR, 85,
				  "Card:%d port:%d send Event-Q to dereg smp dev:0x%llx and then reg it again by new speed",
				  dev->card_id, dev->port_id, dev->sas_addr);
			/*sal_disc_del_disk */
			event_func->device_remove(dev);

			dev->link_rate = link_rate;
			dev->reg_dev_flag = SAINI_NEED_REGDEV;

            /*sal_disc_add_disk */
			ret = event_func->device_arrival(dev);
			if (ERROR == ret) {
				SAL_TRACE(OSP_DBL_MAJOR, 85,
					  "Card:%d port:%d add device:0x%llx failed",
					  dev->card_id, dev->port_id,
					  dev->sas_addr);

				sal_disc_del_dev(port, dev->sas_addr);
				return ret;
			}
		}
	}
	return OK;
}


s32 sal_check_port_valid(struct disc_port * port)
{
	struct disc_card *card = NULL;
	char *port_status[] = {
		"Unknown",
		"disable",
		"enable",
	};

	port_status[0] = port_status[0];
	card = card;
	SAL_ASSERT(NULL != port, return ERROR);
	card = (struct disc_card *)port->card;

	if (PORT_STATUS_PERMIT != port->disc_switch) {
		SAL_TRACE(OSP_DBL_MAJOR, 1000,
			  "Card:%d Port:%d is in status:%d(%s)", card->card_no,
			  port->port_id, port->disc_switch,
			  port_status[port->disc_switch]);
		port->disc_switch = PORT_STATUS_FORBID;
		port->discover_status = DISC_STATUS_NOT_START;
		port->new_disc = DISC_RESULT_FALSE;
		return ERROR;
	}
	return OK;
}


s32 sal_check_port_info(struct disc_port * port)
{
	s32 ret = ERROR;
	struct disc_card *card = NULL;

	SAL_ASSERT(NULL != port, return ERROR);
	card = (struct disc_card *)port->card;

	ret = sal_check_port_valid(port);
	if (ERROR == ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 87,
			  "Card:%d Port:%d is invalid",
			  card->card_no, port->port_id);

		port->discover_status = DISC_STATUS_NOT_START;

		sal_clear_port_rsc(card->card_no, port->port_id);
		return ret;
	}

	return OK;
}

/**
 * sal_parse_report_gen_resp - deal SMP report general respone frame
 * @exp: point to the expander
 * @rg_rsp:point ro respone frame
 */
s32 sal_parse_report_gen_resp(struct disc_exp_data * exp,
			      struct smp_report_general_rsp * rg_rsp)
{
	u32 i = 0;
	u16 exp_chg_cnt = 0;

	SAL_ASSERT(NULL != exp, return ERROR);
	SAL_ASSERT(NULL != rg_rsp, return ERROR);

	exp->phy_nums = rg_rsp->phy_nums;
	for (i = 0; i < exp->phy_nums; i++) {
		exp->curr_phy_idx[i] = 0;
	}

	exp->long_resp = DISC_GET_LONGRESP_BIT(rg_rsp);
	exp->config_route_table = DISC_GET_CONFIGURABLE_BIT(rg_rsp);
	exp->configuring = DISC_GET_CONFIGURING_BIT(rg_rsp);
	exp->routing_idx = DISC_GET_ROUTING_INDEX(rg_rsp);
	exp_chg_cnt = DISC_GET_EXP_CHANGE_COUNT(rg_rsp);

	SAL_TRACE(OSP_DBL_INFO, 972,
		  "Card:%d port:%u expander:0x%llx,num of phy:%d,configuring:%d,configrable:%d,long response:0x%x,change times:%d",
		  exp->card_id,
		  exp->port_id,
		  exp->sas_addr,
		  exp->phy_nums,
		  exp->configuring,
		  exp->config_route_table,
		  exp->long_resp, (exp_chg_cnt - exp->exp_chg_cnt));

	exp->exp_chg_cnt = exp_chg_cnt;

	/*if this expander is configing route, wait and re discover*/
	if (DISC_RESULT_TRUE == exp->configuring) {
		exp->retries++;
		return ERROR;
	}
	return OK;
}

/**
 * sal_exe_report_gen - deal send/revice/parse report general frame
 * @dev: point to the device
 */
s32 sal_exe_report_gen(struct disc_device_data * dev)
{
	u32 req_len = 0;
	u32 resp_len = 0;
	s32 ret = ERROR;
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	struct disc_smp_req *smp_req = NULL;
	struct disc_smp_response *smp_resp = NULL;
	struct smp_report_general_rsp *rg_resp = NULL;
	struct disc_exp_data *exp = NULL;
	struct disc_event_intf *event_func = NULL;

	exp = (struct disc_exp_data *)dev->expander;
	req_len = sal_get_smp_req_len(SMP_REPORT_GENERAL);
	resp_len = sal_get_smp_resp_len(SMP_REPORT_GENERAL);
	card = &disc_card_table[dev->card_id];
	port = &card->disc_port[dev->port_id];
	event_func = &disc_event_func_table[dev->card_id];

	do {
		smp_req = SAS_KMALLOC(req_len, GFP_KERNEL);
		if (NULL == smp_req) {
			SAL_TRACE(OSP_DBL_MAJOR, 91,
				  "Card:%d Port:%d Alloc SMP request memory failed",
				  dev->card_id, dev->port_id);
			ret = ERROR;
			goto EXIT;
		}
		smp_resp = SAS_KMALLOC(resp_len, GFP_KERNEL);
		if (NULL == smp_resp) {
			SAL_TRACE(OSP_DBL_MAJOR, 92,
				  "Card:%d Port:%d Alloc SMP response memory failed",
				  dev->card_id, dev->port_id);
			ret = ERROR;
			goto EXIT;
		}

		memset(smp_req, 0, req_len);
		memset(smp_resp, 0, resp_len);
		smp_req->smp_frame_type = SMP_REQUEST;
		smp_req->smp_function = SMP_REPORT_GENERAL;
		smp_req->resp_len = 18;

        /* sal_send_disc_smp */
		ret = event_func->smp_frame_send(dev, smp_req, req_len, smp_resp, resp_len);
		if (ERROR == ret) {
			SAL_TRACE(OSP_DBL_MAJOR, 93,
				  "Card:%d port:%d send REPORT GENERAL to expander:0x%llx failed",
				  dev->card_id, dev->port_id, dev->sas_addr);

			ret = sal_check_port_valid(port);
			if (ERROR == ret) {
				port->discover_status = DISC_STATUS_NOT_START;
				sal_clear_port_rsc(dev->card_id, dev->port_id);
			}
			ret = ERROR;
			goto EXIT;
		}

		if (smp_resp->smp_result != SMP_FUNCTION_ACCEPTED) {
			SAL_TRACE(OSP_DBL_MINOR, 94,
				  "Card:%d port:%d expander:0x%llx report general response failed with 0x%x",
				  dev->card_id,
				  dev->port_id,
				  dev->sas_addr, smp_resp->smp_result);
			sal_hex_dump("Report general resp:", (u8 *) smp_resp,
				     resp_len);
			ret = ERROR;
			break;
		}

		/*parse report general response */
		rg_resp = &smp_resp->response.report_general_rsp;

		/*if Expander route auto configing, wait and retry*/
		ret = sal_parse_report_gen_resp(exp, rg_resp);
		if (OK == ret) {
			SAL_TRACE(OSP_DBL_DATA, 95,
				  "Card:%d port:%d report general to dev:0x%llx response OK",
				  dev->card_id, dev->port_id, dev->sas_addr);
			ret = OK;
			break;
		}
		SAL_TRACE(OSP_DBL_INFO, 96,
			  "Card:%d port:%d expander:0x%llx is configuring now,retry times:%d",
			  dev->card_id, dev->port_id, dev->sas_addr,
			  exp->retries);

		ret = ERROR;

		SAS_KFREE(smp_req);
		SAS_KFREE(smp_resp);
		smp_req = NULL;
		smp_resp = NULL;

		SAS_SET_CURRENT_STATE(TASK_INTERRUPTIBLE);
		schedule_timeout(DISC_CONFIGRING_RETRY_TIME);
	} while (exp->retries <= DISC_MAX_REPORTGEN_RETRIES);

      EXIT:
	if (NULL != smp_req) {
		SAS_KFREE(smp_req);
		smp_req = NULL;
	}

	if (NULL != smp_resp) {
		SAS_KFREE(smp_resp);
		smp_resp = NULL;
	}
	return ret;
}


void sal_add_exp_up_stream_phy(struct disc_exp_data *expander, u8 phy_id)
{
	u32 i = 0;
	u8 set = DISC_RESULT_FALSE;
	u8 up_stream_phy_id = 0;

	SAL_ASSERT(NULL != expander, return);
	SAL_TRACE(OSP_DBL_INFO, 995,
		  "Card:%d port:%u add expander:0x%llx upstream phy:%d",
		  expander->card_id,
		  expander->port_id, expander->sas_addr, phy_id);
	for (i = 0; i < expander->num_of_upstream_phys; i++) {
		if (expander->upstream_phy[i] == phy_id) {
			set = DISC_RESULT_TRUE;
			break;
		}
	}

	if (DISC_RESULT_FALSE == set) {
		up_stream_phy_id = expander->num_of_upstream_phys++;
		expander->upstream_phy[up_stream_phy_id] = phy_id;
	}

	return;
}

/**
 * sal_set_up_stream_exp - add upstream expander for certain device,
 *  and report add new device
 * @dev: point to the device
 * @attached_dev: device to be add 
 */
void sal_set_up_stream_exp(struct disc_device_data *dev,
			   struct disc_device_data *attached_dev)
{
	s32 ret = ERROR;
	struct disc_exp_data *exp = NULL;
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	struct disc_device_data *atta_dev = NULL;
	struct disc_event_intf *event_func = NULL;
	struct disc_exp_data *atta_exp = NULL;
	struct disc_exp_data *tmp_exp = NULL;

	exp = (struct disc_exp_data *)dev->expander;
	card = &disc_card_table[dev->card_id];
	port = &card->disc_port[dev->port_id];
	event_func = &disc_event_func_table[dev->card_id];

	sal_add_exp_up_stream_phy(exp, attached_dev->phy_id);

	exp->upstream_sas_addr = attached_dev->sas_addr;
 
	/*check attached device is new device*/
	atta_dev = sal_find_dev_by_sas_addr(attached_dev->sas_addr, port);
	if (NULL != atta_dev) {

		atta_dev->valid_now = DISC_RESULT_TRUE;

		/*double root Expander */
		tmp_exp = (struct disc_exp_data *)atta_dev->expander;
		if (tmp_exp->upstream_sas_addr == dev->sas_addr) {
			SAL_TRACE(OSP_DBL_INFO, 951,
				  "Card:%d port:%u has two root:"
				  "expander:0x%llx subtractive to subtractive expander:0x%llx",
				  dev->card_id, dev->port_id,
				  atta_dev->sas_addr, dev->sas_addr);
			tmp_exp->upstream_exp = exp;
			exp->upstream_exp = tmp_exp;
		}
		return;
	}

	/*add attached device to list*/
	atta_dev = sal_get_free_device(card);
	if (NULL == atta_dev) {
		SAL_TRACE(OSP_DBL_MAJOR, 102,
			  "Card:%d port:%d get device node failed",
			  dev->card_id, dev->port_id);
		return;
	}

	atta_dev->card_id = dev->card_id;
	atta_dev->port_id = dev->port_id;
	atta_dev->dev_type = attached_dev->dev_type;
	atta_dev->ini_proto = attached_dev->ini_proto;
	atta_dev->link_rate = attached_dev->link_rate;
	atta_dev->phy_id = attached_dev->phy_id;
	atta_dev->tgt_proto = attached_dev->tgt_proto;
	atta_dev->valid_before = DISC_RESULT_FALSE;
	atta_dev->valid_now = DISC_RESULT_TRUE;
	atta_dev->father_sas_addr = dev->sas_addr;
	atta_dev->sas_addr = attached_dev->sas_addr;
	atta_dev->reg_dev_flag = SAINI_NEED_REGDEV;

	sal_add_device_list(atta_dev);

	/*call driver: device arrival*/
	ret = event_func->device_arrival(atta_dev);
	if (ERROR == ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 103,
			  "Card:%d port:%d add device:0x%llx failed",
			  dev->card_id, dev->port_id, atta_dev->sas_addr);
		ret = sal_check_port_valid(port);
		if (ERROR == ret) {
			port->discover_status = DISC_STATUS_NOT_START;
			sal_clear_port_rsc(dev->card_id, dev->port_id);
			return;
		}
		sal_disc_del_dev(port, atta_dev->sas_addr);
		return;
	}

	/*add Expander list*/
	atta_exp = sal_get_free_expander(card);
	if (NULL == atta_exp) {
		SAL_TRACE(OSP_DBL_MAJOR, 2111,
			  "Card:%d has no more free expander node",
			  atta_dev->card_id);
		return;
	}

	atta_exp->device = atta_dev;
	atta_exp->card_id = atta_dev->card_id;
	atta_exp->port_id = atta_dev->port_id;
	atta_exp->num_of_upstream_phys = 0;
	atta_exp->discovering_phy_id = 0;
	atta_exp->send_smp_phy_id = 0;
	atta_exp->sas_addr = atta_dev->sas_addr;
	atta_exp->flag = DISC_EXP_UP_DISCOVERING;
	atta_exp->retries = 0;
	exp->upstream_exp = atta_exp;
	sal_add_exp_list(atta_exp);

	/*very important, device and expander relation;
	   expander:point to self;
	   device: point father device*/
	atta_dev->expander = (void *)atta_exp;
}

void sal_add_exp_down_stream_phy(struct disc_exp_data *expander, u8 phy_id)
{
	u32 i = 0;
	u8 set = DISC_RESULT_FALSE;
	u8 up_stream_phy_id = 0;

	SAL_ASSERT(NULL != expander, return);
	SAL_TRACE(OSP_DBL_INFO, 976,
		  "Card %d port %u add expander 0x%llx downstream phy %d",
		  expander->card_id,
		  expander->port_id, expander->sas_addr, phy_id);

	for (i = 0; i < expander->num_of_downstream_phys; i++) {
		if (expander->downstream_phy[i] == phy_id) {
			set = DISC_RESULT_TRUE;
			break;
		}
	}

	if (DISC_RESULT_FALSE == set) {
		up_stream_phy_id = expander->num_of_downstream_phys++;
		expander->downstream_phy[up_stream_phy_id] = phy_id;
	}

	return;
}

/**
 * sal_attached_dev_is_exp - check attached device is expander or not,
 *  and then add to discovering list
 * @attached_dev: device to be check 
 */
void sal_attached_dev_is_exp(struct disc_device_data *attached_dev)
{
	u8 dev_type = 0;
	u8 card_id = 0;
	u32 port_id = 0;
	u64 attached_sas_addr = 0;
	u64 up_stream_sas_addr = 0;
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	struct disc_exp_data *attached_exp = NULL;
	struct disc_exp_data *upstream_exp = NULL;
	struct disc_device_data *up_stream_dev = NULL;

	SAL_ASSERT(NULL != attached_dev, return);

	card_id = attached_dev->card_id;
	port_id = attached_dev->port_id;
	card = &disc_card_table[card_id];
	port = &card->disc_port[port_id];
	dev_type = attached_dev->dev_type;
	attached_sas_addr = attached_dev->sas_addr;
	/*expander*/
	if (EXPANDER_DEVICE == dev_type) {
		up_stream_sas_addr = attached_dev->father_sas_addr;
		SAL_TRACE(OSP_DBL_INFO, 977,
			  "Card:%d port:%u expander:0x%llx upstream sas address is 0x%llx",
			  card_id, port_id,
			  attached_dev->sas_addr, up_stream_sas_addr);
		up_stream_dev =
		    sal_find_dev_by_sas_addr(up_stream_sas_addr, port);
		if (NULL == up_stream_dev)
			return;

		upstream_exp = (struct disc_exp_data *)up_stream_dev->expander;
		if (NULL == upstream_exp) {
			SAL_TRACE(OSP_DBL_MAJOR, 978,
				  "Card:%d port:%u up stream device:0x%llx is not expander",
				  card_id, port_id, up_stream_dev->sas_addr);
			return;
		}
		
		/*add expander to discovering list,and create relation*/
		attached_exp =
		    sal_find_exp_by_sas_addr(port, attached_sas_addr);
		if (NULL == attached_exp) {
			attached_exp = sal_get_free_expander(card);
			if (NULL == attached_exp)
				return;

			attached_exp->device = attached_dev;
			attached_exp->card_id = card_id;
			attached_exp->port_id = port_id;
			attached_exp->discovering_phy_id = 0;
			attached_exp->send_smp_phy_id = 0;
			attached_exp->flag = DISC_EXP_DOWN_DISCOVERING;
			attached_exp->sas_addr = attached_sas_addr;
			attached_exp->upstream_exp = upstream_exp;
			attached_exp->num_of_downstream_phys = 0;
			attached_exp->num_of_upstream_phys = 0;
			attached_exp->phy_nums = 0;
			attached_exp->curr_upstream_phy_idx = 0;
			attached_exp->curr_downstream_phy_idx = 0;
			attached_exp->retries = 0;
			upstream_exp->downstream_exp = attached_exp;

			sal_add_exp_list(attached_exp);

			attached_dev->expander = (void *)attached_exp;
		}

		attached_exp->flag = DISC_EXP_DOWN_DISCOVERING;

		/*add downstream phy for father expander*/
		if (upstream_exp->config_route_table) {
			SAL_TRACE(OSP_DBL_INFO, 979,
				  "Card:%d port:%u up stream expander:0x%llx configrable:%d",
				  card_id, port_id,
				  upstream_exp->sas_addr,
				  upstream_exp->config_route_table);
			sal_add_exp_down_stream_phy(upstream_exp,
						    upstream_exp->
						    discovering_phy_id);
		}

		/*add up sream phy for attached Expander*/
		sal_add_exp_up_stream_phy(attached_exp,
					  upstream_exp->discovering_phy_id);
		attached_exp->upstream_sas_addr = up_stream_sas_addr;
	}

	return;
}

struct disc_exp_data *sal_find_configurable_exp(struct disc_exp_data *exp)
{
	struct disc_exp_data *upstream_exp = NULL;
	struct disc_exp_data *tmp_exp = NULL;

	tmp_exp = exp->upstream_exp;
	while (tmp_exp) {
		/*if auto Expander,jump to next expander*/
		if (tmp_exp->config_route_table) {
			upstream_exp = tmp_exp;
			break;
		}

		tmp_exp = tmp_exp->upstream_exp;
	}

	return upstream_exp;
}

u32 sal_find_cur_down_stream_phy_index(struct disc_exp_data * exp)
{
	u32 i = DISC_PHYID_INVALID;
	u32 index = 0;
	u8 phy_id = 0;
	struct disc_exp_data *tmp_exp = NULL;
	struct disc_exp_data *downstream_exp = NULL;

	tmp_exp = exp;
	downstream_exp = tmp_exp->curr_downstream_exp;

	phy_id = downstream_exp->upstream_phy[0];
	for (i = 0; i < tmp_exp->num_of_downstream_phys; i++) {
		if (phy_id == downstream_exp->downstream_phy[i]) {
			index = i;
			break;
		}
	}

	return index;
}

/**
 * sal_check_and_config_up_exp - check if need to config upstream expander route,
 *  config it if need.
 * @up_exp: up stream expander
 * @exp: down stream expander
 */
void sal_check_and_config_up_exp(struct disc_exp_data *up_exp,
				 struct disc_exp_data *exp)
{
	struct disc_exp_data *config_exp = NULL;
	struct disc_exp_data *down_exp = NULL;
	u8 card_id = 0;
	u32 port_id = 0;
	struct disc_port *port = NULL;
	u32 tmp_phy_index = 0;

	SAL_ASSERT(NULL != up_exp, return);
	SAL_ASSERT(NULL != exp, return);

	down_exp = exp->curr_downstream_exp;
	card_id = exp->card_id;
	port_id = down_exp->port_id;
	port = &disc_card_table[card_id].disc_port[port_id];
	up_exp->downstream_exp = exp;
	config_exp = sal_find_configurable_exp(exp);

	if (NULL == config_exp) {
		down_exp->curr_upstream_phy_idx = 0;
		port->discover_status = DISC_STATUS_DOWNSTREAM;
		return;
	}

	up_exp->curr_downstream_exp = exp;
	tmp_phy_index = sal_find_cur_down_stream_phy_index(config_exp);
	config_exp->curr_downstream_phy_idx = tmp_phy_index;
	config_exp->return_exp = exp->return_exp;
	down_exp->curr_upstream_phy_idx = 0;

	sal_add_routing_entry(config_exp,
			      config_exp->downstream_phy[tmp_phy_index],
			      exp->cfg_sas_addr);
}

/**
 * sal_config_up_exp - check if need config upstream expander route
 * @exp: current expander
 */
void sal_config_up_exp(struct disc_exp_data *exp)
{
	u8 phy_id = 0;
	u8 card_id = 0;
	u32 port_id = 0;
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	struct disc_exp_data *down_exp = NULL;
	struct disc_exp_data *up_exp = NULL;

	card_id = exp->card_id;
	port_id = exp->port_id;
	card = &disc_card_table[card_id];
	port = &card->disc_port[port_id];
	down_exp = exp->curr_downstream_exp;

	exp->curr_downstream_phy_idx++;
	SAL_TRACE(OSP_DBL_INFO, 959,
		  "Card %d port %u current downstream phy index %u of expander 0x%llx",
		  card_id, port_id,
		  exp->curr_downstream_phy_idx, exp->sas_addr);

	if (NULL != down_exp) {
		down_exp->curr_upstream_phy_idx++;
		SAL_TRACE(OSP_DBL_INFO, 960,
			  "Card %d port %u current upstream phy index %u of downstream expander "
			  "0x%016llx", card_id, port_id,
			  down_exp->curr_upstream_phy_idx, down_exp->sas_addr);
		phy_id = (u8) down_exp->curr_upstream_phy_idx;
		if (phy_id < down_exp->num_of_upstream_phys) {
			sal_add_routing_entry(exp,
					      down_exp->upstream_phy[phy_id],
					      exp->cfg_sas_addr);
		} else {
			up_exp = exp->upstream_exp;
			if (NULL == up_exp) {
				down_exp->curr_upstream_phy_idx = 0;

				port->discover_status = DISC_STATUS_DOWNSTREAM;
				return;
			}

			sal_check_and_config_up_exp(up_exp, exp);
		}
	} else
		port->discover_status = DISC_STATUS_DOWNSTREAM;
}

/**
 * sal_parse_config_rout_info_resp - deal config routing info respond message
 * @device: point to respond frame expander
 * @smp_resp: smp respond message
 */
void sal_parse_config_rout_info_resp(struct disc_device_data *device,
				     struct disc_smp_response *smp_resp)
{
	u8 card_id = 0;
	u32 port_id = 0;
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	struct disc_exp_data *expander = NULL;

	SAL_ASSERT(smp_resp != NULL, return);
	card_id = device->card_id;
	port_id = device->port_id;
	card = &disc_card_table[card_id];
	port = &card->disc_port[port_id];
	expander = (struct disc_exp_data *)device->expander;

	if (NULL == expander) {
		SAL_TRACE(OSP_DBL_MAJOR, 961,
			  "Card %d port %d expander is null", card_id, port_id);
		return;
	}

	if (SMP_FUNCTION_ACCEPTED != smp_resp->smp_result) {
		SAL_TRACE(OSP_DBL_MAJOR, 962,
			  "Card %d port %u config routing table failed with 0x%x",
			  card_id, port_id, smp_resp->smp_result);

		/*config routing info request frame failure,
		   discover next expander*/
		port->discover_status = DISC_STATUS_DOWNSTREAM;
		return;
	}
	/*if need to config up expander*/
	sal_config_up_exp(expander);
}

/**
 * sal_add_routing_entry - send config routing info frame to expander,config route
 * @config_exp: point to expander
 * @phyid: phy id
 * @config_sas_addr: config sas address
 */
void sal_add_routing_entry(struct disc_exp_data *config_exp,
			   u8 phyid, u64 config_sas_addr)
{
	struct disc_exp_data *exp = NULL;
	struct disc_device_data *device = NULL;
	u8 routing_attribute = 0;
	u8 card_id = 0;
	u32 port_id = 0;
	u32 i = 0;
	u32 offset = 0;
	s32 ret = ERROR;
	u32 req_len = 0;
	u32 resp_len = 0;
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	struct disc_smp_req *smp_req = NULL;
	struct disc_smp_response *smp_resp = NULL;
	struct disc_event_intf *event_func = NULL;

	exp = config_exp;
	SAL_ASSERT(NULL != exp, return);
	device = exp->device;
	card_id = exp->card_id;
	port_id = exp->port_id;
	card = &disc_card_table[card_id];
	port = &card->disc_port[port_id];
	routing_attribute = exp->routing_attr[phyid];
	event_func = &disc_event_func_table[card_id];

	ret = sal_check_port_valid(port);
	if (ERROR == ret)
		return;

	/*bug check */
	if (SAS_TABLE_ROUTING != routing_attribute) {
		SAL_TRACE(OSP_DBL_MAJOR, 980,
			  "Card %d port %u expander 0x%llx phy %d routing attribute:%c",
			  card_id, port_id,
			  exp->sas_addr,
			  phyid,
			  (SAS_DIRECT_ROUTING == routing_attribute) ? 'D' :
			  (SAS_SUBTRUCTIVE_ROUTING ==
			   routing_attribute) ? 'S' : '?');

		return;
	}

	port->discover_status = DISC_STATUS_CONFIG_ROUTING;
	if (exp->curr_phy_idx[phyid] >= exp->routing_idx) {
		SAL_TRACE(OSP_DBL_MAJOR, 981,
			  "Card %d port %u current phy index %d is invalid",
			  card_id, port_id, exp->curr_phy_idx[phyid]);
		return;
	}

	SAL_TRACE(OSP_DBL_INFO, 982,
		  "Card %d port %u config routing phy %d of expander 0x%llx sas address 0x%llx",
		  card_id, port_id, phyid, exp->sas_addr, config_sas_addr);
	exp->cfg_sas_addr = config_sas_addr;

	req_len = sal_get_smp_req_len(SMP_CONFIG_ROUTING_INFO);
	resp_len = sal_get_smp_resp_len(SMP_CONFIG_ROUTING_INFO);
	smp_req = SAS_KMALLOC(req_len, GFP_KERNEL);
	if (NULL == smp_req) {
		SAL_TRACE(OSP_DBL_MAJOR, 116,
			  "Alloc SMP request memory failed");
		goto EXIT;
	}
	memset(smp_req, 0, req_len);

	smp_resp = SAS_KMALLOC(resp_len, GFP_KERNEL);
	if (NULL == smp_resp) {
		SAL_TRACE(OSP_DBL_MAJOR, 117,
			  "Alloc SMP request memory failed");
		goto EXIT;
	}
	memset(smp_resp, 0, resp_len);

	smp_req->smp_frame_type = SMP_REQUEST;
	smp_req->smp_function = SMP_CONFIG_ROUTING_INFO;
	smp_req->req_type.cfg_route_info.expander_route_idx[0] =
	    (exp->curr_phy_idx[phyid] >> 8) & 0xff;
	smp_req->req_type.cfg_route_info.expander_route_idx[1] =
	    exp->curr_phy_idx[phyid] & 0xff;
	smp_req->req_type.cfg_route_info.phy_identifier = phyid;
	smp_req->req_type.cfg_route_info.disable_exp_route_entry = 0;
	for (i = 0; i < SAS_ADDR_LENGTH; i++) {
		offset = ((SAS_ADDR_LENGTH - 1) - i) * 8;
		smp_req->req_type.cfg_route_info.route_sas_addr[i] =
		    (config_sas_addr >> offset) & 0xff;
	}

	ret = event_func->smp_frame_send(device,
					 smp_req, req_len, smp_resp, resp_len);
	if (ERROR == ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 118,
			  "Card %d port %d send DISCOVER LIST to expander 0x%llx failed",
			  device->card_id, device->port_id, device->sas_addr);
		ret = sal_check_port_valid(port);
		if (ERROR == ret) {
			port->discover_status = DISC_STATUS_NOT_START;
			sal_clear_port_rsc(device->card_id, device->port_id);
		}

		goto EXIT;
	}

	/*save PhyId */
	exp->send_smp_phy_id = phyid;
	exp->curr_phy_idx[phyid]++;

	/*parse config route info response frame*/
	sal_parse_config_rout_info_resp(device, smp_resp);

      EXIT:
	if (NULL != smp_req) {
		SAS_KFREE(smp_req);
		smp_req = NULL;
	}

	if (NULL != smp_resp) {
		SAS_KFREE(smp_resp);
		smp_resp = NULL;
	}

	return;
}

/**
 * sal_cfg_exp - config expander
 * @expander: point to expander
 * @source_sas_addr: sas address
 */
void sal_cfg_exp(struct disc_exp_data *expander, u64 source_sas_addr)
{
	u64 config_sas_addr = 0;
	u32 tmp_phy_index = 0;
	struct disc_exp_data *config_exp = NULL;
	struct disc_exp_data *upstream_exp = NULL;

	SAL_ASSERT(NULL != expander, return);

	upstream_exp = expander->upstream_exp;
	if (NULL == upstream_exp)
		return;

	upstream_exp->downstream_exp = expander;
	config_sas_addr = source_sas_addr;

	/*check if need to config up stream expander route table;*/
	config_exp = sal_find_configurable_exp(expander);
	if (NULL == config_exp)
		return;

	/*sas address is self,return*/
	if (config_exp->sas_addr == config_sas_addr) {
		SAL_TRACE(OSP_DBL_MAJOR, 983,
			  "Card %d port %u config sas address 0x%llx is config expander sas address",
			  config_exp->card_id, config_exp->port_id,
			  config_sas_addr);
		return;
	}

	upstream_exp->curr_downstream_exp = expander;
	tmp_phy_index = sal_find_cur_down_stream_phy_index(config_exp);
	if (DISC_PHYID_INVALID == tmp_phy_index) {
		SAL_TRACE(OSP_DBL_MAJOR, 120,
			  "Card %d expander 0x%llx find phy failed",
			  config_exp->card_id, config_exp->sas_addr);
		return;
	}
	config_exp->curr_downstream_phy_idx = tmp_phy_index;
	config_exp->return_exp = expander;

	sal_add_routing_entry(config_exp,
			      config_exp->downstream_phy[tmp_phy_index],
			      config_sas_addr);
	return;
}

/**
 * sal_up_stream_disc_resp - add up stream expander for device,
 *  then report add new divece
 * @exp: point to expander
 * @disc_resp: discover response
 */
void sal_up_stream_disc_resp(struct disc_exp_data *exp,
			     struct smp_discover_rsp *disc_resp)
{
	u8 dev_type = 0;
	u8 rout_attri = 0;
	u8 tmp_rate = 0;
	u32 att_sas_addr_hi = 0;
	u32 att_sas_addr_lo = 0;
	struct disc_device_data atta_dev;
	struct disc_device_data *dev = NULL;

	SAL_ASSERT(NULL != exp, return);
	SAL_ASSERT(NULL != disc_resp, return);
	dev = exp->device;
	dev_type = DISC_GET_ATTACHED_DEVTYPE(disc_resp);
	rout_attri = disc_resp->routing_attr & 0x0f;

	/*find root */
	if ((EXPANDER_DEVICE == dev_type)
	    && (SAS_SUBTRUCTIVE_ROUTING == rout_attri)) {
		atta_dev.card_id = exp->card_id;
		atta_dev.port_id = dev->port_id;
		atta_dev.dev_type = dev_type;
		atta_dev.ini_proto = disc_resp->attach_ini_proto & 0x0f;

		tmp_rate = DSIC_GET_DEV_RATE(disc_resp->link_rate);

		atta_dev.link_rate = MIN(tmp_rate, dev->link_rate);
		atta_dev.phy_id = disc_resp->phy_identifier;
		atta_dev.tgt_proto = disc_resp->attach_tgt_proto & 0x0f;
		att_sas_addr_hi = DISC_GET_ATTACHED_SASADDR_HI(disc_resp);
		att_sas_addr_lo = DISC_GET_ATTACHED_SASADDR_LO(disc_resp);
		atta_dev.sas_addr =
		    (((u64) att_sas_addr_hi) << 32) | att_sas_addr_lo;
		sal_set_up_stream_exp(dev, &atta_dev);
	}
}

/**
 * sal_down_stream_disc_resp - add up stream expander for device,
 *  then report add new divece
 * @exp: point to expander
 * @disc_resp: discover response frame
 */
void sal_down_stream_disc_resp(struct disc_exp_data *exp,
			       struct smp_discover_rsp *disc_resp)
{
	u8 phy_id = 0;
	u8 dev_type = 0;
	u8 virtual = 0;
	u8 rout_attri = 0;
	u8 phy_chg_cnt = 0;
	s32 ret = ERROR;
	u32 att_sas_addr_hi = 0;
	u32 att_sas_addr_lo = 0;
	struct disc_device_data atta_dev;
	struct disc_device_data *dev = NULL;
	struct disc_device_data *attached_dev = NULL;

	dev = exp->device;
	phy_id = disc_resp->phy_identifier;
	dev_type = DISC_GET_ATTACHED_DEVTYPE(disc_resp);
	virtual = DISC_GET_VIRTUAL_FROM_DISCOVERRESP(disc_resp);
	rout_attri = disc_resp->routing_attr & 0x0f;

	SAL_TRACE(OSP_DBL_MAJOR, 985,
		  "Card:%d port:%u expander:0x%llx phy:%d rout_attri:%d",
		  exp->card_id, exp->port_id, exp->sas_addr, phy_id,
		  rout_attri);

	if ((SAS_DIRECT_ROUTING == rout_attri)
	    && (EXPANDER_DEVICE == dev_type)) {
		SAL_TRACE(OSP_DBL_MAJOR, 985,
			  "Card:%d port:%u expander:0x%llx phy:%d direct routing can not attach expander",
			  exp->card_id, exp->port_id, exp->sas_addr, phy_id);
		return;
	}

	att_sas_addr_hi = DISC_GET_ATTACHED_SASADDR_HI(disc_resp);
	att_sas_addr_lo = DISC_GET_ATTACHED_SASADDR_LO(disc_resp);

	atta_dev.card_id = exp->card_id;
	atta_dev.port_id = exp->port_id;
	atta_dev.dev_type = dev_type;
	/*check SES device */
	if (virtual)
		atta_dev.dev_type = VIRTUAL_DEVICE;

	atta_dev.ini_proto = disc_resp->attach_ini_proto & 0x0f;
	atta_dev.link_rate = MIN(dev->link_rate, disc_resp->link_rate & 0x0f);
	atta_dev.phy_id = phy_id;
	atta_dev.tgt_proto = disc_resp->attach_tgt_proto & 0x0f;
	atta_dev.sas_addr = (((u64) att_sas_addr_hi) << 32) | att_sas_addr_lo;
	atta_dev.father_sas_addr = dev->sas_addr;

	ret = sal_check_atta_dev(dev, &atta_dev);
	if (ERROR == ret)
		return;

	phy_chg_cnt = disc_resp->phy_chg_cnt;
	exp->routing_attr[phy_id] = rout_attri;

	SAL_TRACE(OSP_DBL_INFO, 991,
		  "Card %d port %u expander 0x%llx phy %d:%c attached:0x%llx",
		  dev->card_id, dev->port_id,
		  dev->sas_addr, phy_id,
		  (SAS_TABLE_ROUTING == rout_attri) ? 'T' :
		  (SAS_DIRECT_ROUTING == rout_attri) ? 'D' :
		  (SAS_SUBTRUCTIVE_ROUTING == rout_attri) ? 'S' : '?',
		  atta_dev.sas_addr);

	/*if phy change,print*/
	if (phy_chg_cnt != exp->phy_chg_cnt[phy_id]) {
		SAL_TRACE(OSP_DBL_INFO, 124,
			  "Card %d port %u expander 0x%llx phy %d change %d times",
			  dev->card_id, dev->port_id,
			  dev->sas_addr, phy_id,
			  (phy_chg_cnt - exp->phy_chg_cnt[phy_id]));
	}
	exp->phy_chg_cnt[phy_id] = phy_chg_cnt;

	attached_dev = sal_notify_new_dev(dev, &atta_dev);
	if ((NULL != attached_dev)
	    && (SAS_TABLE_ROUTING == rout_attri))
		sal_attached_dev_is_exp(attached_dev);

	sal_cfg_exp(exp, atta_dev.sas_addr);
}

void sal_parse_disc_resp(struct disc_exp_data *exp,
			 struct smp_discover_rsp *disc_resp)
{
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	struct disc_device_data *dev = NULL;

	dev = exp->device;
	card = &disc_card_table[dev->card_id];
	port = &card->disc_port[dev->port_id];

	if (DISC_STATUS_UPSTREAM == port->discover_status)
		sal_up_stream_disc_resp(exp, disc_resp);
	else if (DISC_STATUS_DOWNSTREAM == port->discover_status)
		sal_down_stream_disc_resp(exp, disc_resp);
	else
		SAL_TRACE(OSP_DBL_MAJOR, 125,
			  "Err! the disc process is disordered");
}

/**
 * sal_exec_disc - parse discover response frame
 * @exp: point to expander
 */
s32 sal_exec_disc(struct disc_exp_data *exp)
{
	u32 i = 0;
	u32 req_len = 0;
	u32 resp_len = 0;
	s32 ret = ERROR;
	struct disc_smp_req *smp_req = NULL;
	struct disc_smp_response *smp_resp = NULL;
	struct smp_discover_rsp *disc_resp = NULL;
	struct disc_device_data *dev = NULL;
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	struct disc_event_intf *event_func = NULL;

	SAL_ASSERT(NULL != exp, return ERROR);

	dev = exp->device;
	card = &disc_card_table[dev->card_id];
	port = &card->disc_port[dev->port_id];
	req_len = sal_get_smp_req_len(SMP_DISCOVER_REQUEST);
	resp_len = sal_get_smp_resp_len(SMP_DISCOVER_REQUEST);
	event_func = &disc_event_func_table[dev->card_id];

	for (i = 0; i < exp->phy_nums; i++) {
		smp_req = SAS_KMALLOC(req_len, GFP_KERNEL);
		if (NULL == smp_req) {
			SAL_TRACE(OSP_DBL_MAJOR, 126,
				  "Err! Alloc SMP request memory failed");
			ret = ERROR;
			goto EXIT;
		}

		smp_resp = SAS_KMALLOC(resp_len, GFP_KERNEL);
		if (NULL == smp_resp) {
			SAL_TRACE(OSP_DBL_MAJOR, 127,
				  "Err! Alloc SMP response memory failed");
			ret = ERROR;
			goto EXIT;
		}
		memset(smp_req, 0, req_len);
		memset(smp_resp, 0, resp_len);

		exp->discovering_phy_id = (u8) i;
		smp_req->smp_frame_type = SMP_REQUEST;
		smp_req->smp_function = SMP_DISCOVER_REQUEST;
		smp_req->req_type.discover_req.phy_identifier = (u8) i;
		SAL_TRACE(OSP_DBL_INFO, 2077,
			  "Card:%d port:%u send discover request to phy:%d of expander:0x%llx",
			  exp->card_id, exp->port_id, i, exp->sas_addr);

		ret =
		    event_func->smp_frame_send(dev, smp_req, req_len, smp_resp,
					       resp_len);
		if (ERROR == ret) {
			SAL_TRACE(OSP_DBL_MAJOR, 129,
				  "Card:%d port:%d send DISCOVER to expander:0x%llx failed",
				  dev->card_id, dev->port_id, dev->sas_addr);
			ret = sal_check_port_valid(port);
			if (ERROR == ret) {
				port->discover_status = DISC_STATUS_NOT_START;
				sal_clear_port_rsc(dev->card_id, dev->port_id);
				ret = ERROR;
				goto EXIT;
			}
		}

		if (OK == ret) {
			disc_resp = &smp_resp->response.discover_rsp;
			sal_parse_disc_resp(exp, disc_resp);
		}

		memset(smp_req, 0, req_len);
		memset(smp_resp, 0, resp_len);
		SAS_KFREE(smp_req);
		SAS_KFREE(smp_resp);
		smp_resp = NULL;
		smp_req = NULL;
	}
      EXIT:
	if (NULL != smp_req) {
		SAS_KFREE(smp_req);
		smp_req = NULL;
	}

	if (NULL != smp_resp) {
		SAS_KFREE(smp_resp);
		smp_resp = NULL;
	}

	return ret;
}

/**
 * sal_up_stream_disc_list - parse discover response frame,
 * then check root expander
 * @exp: point to expander
 * @disc_list_resp: discover list response
 */
s32 sal_up_stream_disc_list(struct disc_exp_data * exp,
			    struct smp_discover_list_rsp * disc_list_resp)
{
	u32 phy_id = 0;
	u32 start_phy = 0;
	u32 end_phy = 0;
	u8 dev_type = 0;
	u8 rout_attri = 0;
	u32 att_sas_addr_hi = 0;
	u32 att_sas_addr_lo = 0;
	struct smp_discoverlist_short *short_resp = NULL;
	struct smp_discoverlist_short *short_resp_tmp = NULL;
	struct disc_device_data *dev = NULL;
	struct disc_device_data atta_dev;

	dev = exp->device;
	start_phy = disc_list_resp->start_phy_id;
	end_phy = start_phy + disc_list_resp->num_of_desc;
	SAL_TRACE(OSP_DBL_DATA, 130,
		  "Card:%d Port:%d Expander:0x%llx discover list resp,start phy:%2d,end phy:%2d,num of descr:%d",
		  dev->card_id,
		  dev->port_id,
		  exp->sas_addr,
		  start_phy, end_phy, disc_list_resp->num_of_desc);
	short_resp = &disc_list_resp->disc_desc.disc_short_resp;

	for (phy_id = start_phy; phy_id < end_phy; phy_id++) {
		short_resp_tmp = short_resp + (phy_id - start_phy);
		if (phy_id != short_resp_tmp->phy_id) {
			SAL_TRACE(OSP_DBL_MAJOR, 131,
				  "Card:%d Port:%d Logic Phy:%d and Phy Identifier:%d is not matched",
				  dev->card_id,
				  dev->port_id, phy_id, short_resp_tmp->phy_id);
			return ERROR;
		}

		if (SMP_FUNCTION_ACCEPTED != short_resp_tmp->func_result) {
			SAL_TRACE(OSP_DBL_MINOR, 132,
				  "Card:%d port:%d expander:0x%llx phy:%2d discover list failed:0x%x",
				  dev->card_id,
				  dev->port_id,
				  dev->sas_addr,
				  phy_id, short_resp_tmp->func_result);
			continue;
		}

		dev_type = DISC_GET_DEVTYPE_FROM_SHORTDESC(short_resp_tmp);
		rout_attri = DISC_GET_ROUTEATTR_FROM_SHORTDESC(short_resp_tmp);

		/*find root */
		if ((EXPANDER_DEVICE == dev_type)
		    && (SAS_SUBTRUCTIVE_ROUTING == rout_attri)) {
			atta_dev.card_id = exp->card_id;
			atta_dev.port_id = dev->port_id;
			atta_dev.dev_type = dev_type;
			atta_dev.ini_proto = short_resp->attached_ini & 0x0f;
			atta_dev.link_rate = short_resp->physic_rate & 0x0f;
			atta_dev.phy_id = (u8) phy_id;
			atta_dev.tgt_proto = short_resp->attached_tgt & 0x0f;
			att_sas_addr_hi =
			    DISC_GET_ADDRHI_FROM_SHORTDESC(short_resp);
			att_sas_addr_lo =
			    DISC_GET_ADDRLO_FROM_SHORTDESC(short_resp);
			atta_dev.sas_addr =
			    (((u64) att_sas_addr_hi) << 32) | att_sas_addr_lo;
			sal_set_up_stream_exp(dev, &atta_dev);
		}

	}

	return OK;
}

/**
 * sal_check_atta_dev - check attached device infomation
 * @exp: point to expander
 * @disc_list_resp: discover list response
 */
s32 sal_check_atta_dev(struct disc_device_data * dev,
		       struct disc_device_data * attadev)
{
	struct disc_card *card = NULL;

	card = &disc_card_table[dev->card_id];

	if (0x00 == attadev->sas_addr) {
		SAL_TRACE(OSP_DBL_DATA, 987,
			  "Card:%d port:%u expander:0x%llx phy:%d attached SAS address is 0",
			  dev->card_id, dev->port_id, dev->sas_addr,
			  attadev->phy_id);
		return ERROR;
	}

	/*tgt type:unkonown:jump to next phy and discover */
	if ((DISC_TGT_IS_NODEVICE(attadev->tgt_proto))
	    && (DISC_INI_IS_NODEVICE(attadev->ini_proto))) {
		SAL_TRACE(OSP_DBL_MAJOR, 988,
			  "Card:%d port:%u expander:0x%llx phy:%d attached unknown device",
			  dev->card_id, dev->port_id, dev->sas_addr,
			  attadev->phy_id);
		return ERROR;
	}

	/*phy no device,jump to next phy and discover */
	if (NO_DEVICE == attadev->dev_type) {
		/*If there is a problem receiving the expected initial Register - Device to Host FIS, then the STP/SATA bridge
		   should use SATA_R_ERR to retry until the FIS is successfully received. In the DISCOVER response, the
		   ATTACHED SATA DEVICE bit is set to one and the ATTACHED SAS ADDRESS field is valid, but the ATTACHED DEVICE
		   TYPE field is set to 000b (i.e., no device attached) during this time */
		if (attadev->tgt_proto & 0x01) {
			SAL_TRACE(OSP_DBL_MAJOR, 136,
				  "Card:%d port:%u expander:0x%llx phy:%d attached SATA device D2H error",
				  dev->card_id, dev->port_id, dev->sas_addr,
				  attadev->phy_id);
			return ERROR;
		}

		SAL_TRACE(OSP_DBL_INFO, 2112,
			  "Card:%d port:%u expander:0x%llx phy:%d attached no device",
			  dev->card_id, dev->port_id, dev->sas_addr,
			  attadev->phy_id);
		return ERROR;
	}

	/*T-T:new attached expander is the father expander, discover next phy*/
	if (attadev->sas_addr == dev->father_sas_addr) {
		SAL_TRACE(OSP_DBL_INFO, 989,
			  "Card:%d port:%u expander:0x%llx phy:%d attached father addr:0x%llx",
			  dev->card_id, dev->port_id, dev->sas_addr,
			  attadev->phy_id, attadev->sas_addr);

		if ((EXPANDER_DEVICE == attadev->dev_type)
		    && (DISC_CHECK_EXP_EXP_ATTACHED
		     (attadev->phy_id, &card->disc_topo))) {
			SAL_TRACE(OSP_DBL_MAJOR, 139,
				  "Card:%d EXP-EXP attched error",
				  card->card_no);
		}
		return ERROR;
	}

	return OK;
}

/**
 * sal_notify_new_dev - check input attached device is in device list;
 * if need,report new device and add it to discover device list
 * @dev: point to device
 * @new_dev: new device
 */
struct disc_device_data *sal_notify_new_dev(struct disc_device_data *dev,
					    struct disc_device_data *new_dev)
{
	u8 tmp_rate = 0;
	s32 ret = ERROR;
	struct disc_device_data *attached_dev = NULL;
	struct disc_device_data *father_dev = NULL;
	struct disc_exp_data *expander = NULL;
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	struct disc_event_intf *event_func = NULL;

	SAL_ASSERT(NULL != dev, return NULL);
	SAL_ASSERT(NULL != new_dev, return NULL);

	card = &disc_card_table[dev->card_id];
	port = &card->disc_port[dev->port_id];
	event_func = &disc_event_func_table[dev->card_id];

	expander = (struct disc_exp_data *)dev->expander;

	/*father device rate*/
	father_dev = expander->device;
	tmp_rate = MIN(father_dev->link_rate, new_dev->link_rate);

	attached_dev = sal_find_dev_by_sas_addr(new_dev->sas_addr, port);
	if (NULL != attached_dev) {
		/*old device*/
		SAL_TRACE(OSP_DBL_DATA, 140,
			  "Card:%d Port:%d Old device:0x%llx already exist",
			  attached_dev->card_id, port->port_id,
			  attached_dev->sas_addr);
		
		attached_dev->valid_now = DISC_RESULT_TRUE;

		ret = sal_check_dev_rate(attached_dev, tmp_rate);
		if (ERROR == ret)
			return NULL;

		return attached_dev;
	}

	/*new device, add to device list*/
	attached_dev = sal_get_free_device(card);
	if (NULL == attached_dev) {
		SAL_TRACE(OSP_DBL_MAJOR, 990,
			  "Card:%d port:%u get free device node failed",
			  dev->card_id, dev->port_id);
		return NULL;
	}

	attached_dev->card_id = dev->card_id;
	attached_dev->port_id = dev->port_id;
	attached_dev->phy_id = new_dev->phy_id;
	attached_dev->dev_type = new_dev->dev_type;
	attached_dev->ini_proto = new_dev->ini_proto;
	attached_dev->tgt_proto = new_dev->tgt_proto;
	attached_dev->father_sas_addr = expander->sas_addr;
	attached_dev->sas_addr = new_dev->sas_addr;
	/*child device rate must <= father device rate*/
	attached_dev->link_rate = tmp_rate;
	attached_dev->valid_before = DISC_RESULT_FALSE;
	attached_dev->valid_now = DISC_RESULT_TRUE;
	attached_dev->reg_dev_flag = SAINI_NEED_REGDEV;

	/*end device:Expander point to father device*/
	if (EXPANDER_DEVICE != new_dev->dev_type)
		attached_dev->expander = (void *)expander;

	if ((EXPANDER_DEVICE == new_dev->dev_type)
	    && DISC_CHECK_PRI_PRI_ATTACHED(new_dev->phy_id, &card->disc_topo)) {
		SAL_TRACE(OSP_DBL_MAJOR, 142,
			  "Card:%d error topo is PRI-PRI attached",
			  dev->card_id);
		return NULL;
	}

	sal_add_device_list(attached_dev);

	if (EXPANDER_DEVICE != new_dev->dev_type)
		return attached_dev;

	/*sal_disc_add_disk */
	ret = event_func->device_arrival(attached_dev);
	if (ERROR == ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 143,
			  "Card:%d port:%d add SMP device:0x%llx failed",
			  dev->card_id, dev->port_id, new_dev->sas_addr);
		ret = sal_check_port_valid(port);
		if (ERROR == ret) {
			port->discover_status = DISC_STATUS_NOT_START;
			sal_clear_port_rsc(dev->card_id, dev->port_id);
			return NULL;
		}

		sal_disc_del_dev(port, new_dev->sas_addr);
		return NULL;
	}

	return attached_dev;
}

s32 sal_check_short_resp(struct disc_exp_data * exp,
			 struct smp_discoverlist_short * short_resp)
{
	u8 dev_type = 0;
	u8 rout_attri = 0;

	SAL_ASSERT(NULL != short_resp, return ERROR);
	SAL_ASSERT(NULL != exp, return ERROR);

	if (SMP_FUNCTION_ACCEPTED != short_resp->func_result) {
		SAL_TRACE(OSP_DBL_MINOR, 147,
			  "Card:%d port:%d expander:0x%llx phy:%d discover list failed with 0x%x",
			  exp->card_id, exp->port_id,
			  exp->sas_addr, short_resp->phy_id,
			  short_resp->func_result);
		return ERROR;
	}

	dev_type = DISC_GET_DEVTYPE_FROM_SHORTDESC(short_resp);
	rout_attri = DISC_GET_ROUTEATTR_FROM_SHORTDESC(short_resp);

	/*check: direct route must not connect to expander*/
	if ((SAS_DIRECT_ROUTING == rout_attri)
	    && (EXPANDER_DEVICE == dev_type)) {
		SAL_TRACE(OSP_DBL_MAJOR, 985,
			  "Card:%d port:%u expander:0x%llx phy:%d direct routing can not attach expander",
			  exp->card_id, exp->port_id,
			  exp->sas_addr, short_resp->phy_id);
		return ERROR;
	}
	return OK;
}

/**
 * sal_check_attach - check input attached device is new device;
 * if yes,add to device list and report to driver
 * @exp: point to expander father device
 * @new_dev: new device connect to father device
 */
void sal_check_attach(struct disc_exp_data *exp,
		      struct disc_device_data *new_dev)
{
	u8 rout_attri = 0;
	struct disc_device_data *attached_dev = NULL;
	struct disc_device_data *dev = NULL;

	dev = exp->device;
	rout_attri = exp->routing_attr[new_dev->phy_id];

	attached_dev = sal_notify_new_dev(dev, new_dev);
	if ((NULL != attached_dev)
	    && (SAS_TABLE_ROUTING == rout_attri))
		sal_attached_dev_is_exp(attached_dev);
}

/**
 * sal_down_stream_disc_list - in down stream,parse discover list response frame,
 * and disover the input expander.
 * @exp: point to expander
 * @disc_list_resp: discover list response
 */
void sal_down_stream_disc_list(struct disc_exp_data *exp,
			       struct smp_discover_list_rsp *disc_list_resp)
{
	u32 phy_id = 0;
	u32 start_phy = 0;
	u32 end_phy = 0;
	u8 dev_type = 0;
	u8 rout_attri = 0;
	u8 phy_chg_cnt = 0;
	u8 virtual = 0;
	u32 att_sas_addr_hi = 0;
	u32 att_sas_addr_lo = 0;
	s32 ret = ERROR;
	struct smp_discoverlist_short *short_resp = NULL;
	struct smp_discoverlist_short *short_resp_tmp = NULL;
	struct disc_device_data *dev = NULL;
	struct disc_device_data atta_dev;

	dev = exp->device;
	start_phy = disc_list_resp->start_phy_id;
	end_phy = start_phy + disc_list_resp->num_of_desc;
	SAL_TRACE(OSP_DBL_DATA, 149,
		  "Card:%d Port:%d Expander:0x%llx discover list resp start phy:%d,end phy:%d,num of descr:%d",
		  dev->card_id,
		  dev->port_id,
		  exp->sas_addr,
		  start_phy, end_phy, disc_list_resp->num_of_desc);
	short_resp = &disc_list_resp->disc_desc.disc_short_resp;
	for (phy_id = start_phy; phy_id < end_phy; phy_id++) {
		short_resp_tmp = short_resp + (phy_id - start_phy);
		if (phy_id != short_resp_tmp->phy_id) {
			SAL_TRACE(OSP_DBL_MAJOR, 150,
				  "Card:%d Port:%d Logic Phy:%d and Phy Identifier:%d is not matched.",
				  dev->card_id,
				  dev->port_id, phy_id, short_resp_tmp->phy_id);
			return;
		}

		if (ERROR == sal_check_short_resp(exp, short_resp_tmp))
			continue;

		exp->discovering_phy_id = (u8) phy_id;

		dev_type = DISC_GET_DEVTYPE_FROM_SHORTDESC(short_resp_tmp);
		virtual = DISC_GET_VIRTUAL_FROM_SHORTDESC(short_resp_tmp);
		rout_attri = DISC_GET_ROUTEATTR_FROM_SHORTDESC(short_resp_tmp);
		att_sas_addr_hi =
		    DISC_GET_ADDRHI_FROM_SHORTDESC(short_resp_tmp);
		att_sas_addr_lo =
		    DISC_GET_ADDRLO_FROM_SHORTDESC(short_resp_tmp);

		atta_dev.card_id = exp->card_id;
		atta_dev.port_id = dev->port_id;
		atta_dev.dev_type = dev_type;
		/*check SES device */
		if (virtual) {
			atta_dev.dev_type = VIRTUAL_DEVICE;
			
			SAL_TRACE(OSP_DBL_INFO, 985,
				  "Card:%d port:%u expander:0x%llx phy:%d route attr:%d virtual:%s",
				  exp->card_id,
				  exp->port_id,
				  exp->sas_addr,
				  phy_id, rout_attri, virtual ? "yes" : "no");
		}

		atta_dev.ini_proto = short_resp_tmp->attached_ini & 0x0f;
		atta_dev.link_rate = short_resp_tmp->logic_rate & 0x0f;
		atta_dev.phy_id = (u8) phy_id;
		atta_dev.tgt_proto = short_resp_tmp->attached_tgt & 0x0f;
		atta_dev.sas_addr =
		    (((u64) att_sas_addr_hi) << 32) | att_sas_addr_lo;
		atta_dev.father_sas_addr = dev->sas_addr;

		ret = sal_check_atta_dev(dev, &atta_dev);
		if (ERROR == ret)
			continue;

		exp->routing_attr[phy_id] = rout_attri;
		phy_chg_cnt = short_resp_tmp->phy_chg_cnt;

		if ((phy_chg_cnt != exp->phy_chg_cnt[phy_id])
		    || (atta_dev.sas_addr != exp->last_sas_addr[phy_id])) {
			SAL_TRACE(OSP_DBL_INFO, 153,
				  "Card:%d port:%u expander:0x%llx phy:%2d(%c) change:%d to %d times,current addr:0x%llx(last addr:0x%llx),link rate:0x%x",
				  exp->card_id,
				  exp->port_id,
				  exp->sas_addr,
				  phy_id,
				  (SAS_TABLE_ROUTING == rout_attri) ? 'T' :
				  (SAS_DIRECT_ROUTING == rout_attri) ? 'D' :
				  (SAS_SUBTRUCTIVE_ROUTING ==
				   rout_attri) ? 'S' : '?',
				  exp->phy_chg_cnt[phy_id], phy_chg_cnt,
				  atta_dev.sas_addr, exp->last_sas_addr[phy_id],
				  atta_dev.link_rate);
		}
		exp->last_sas_addr[phy_id] = atta_dev.sas_addr;
		exp->phy_chg_cnt[phy_id] = phy_chg_cnt;

		sal_check_attach(exp, &atta_dev);

		sal_cfg_exp(exp, atta_dev.sas_addr);
	}

}


void sal_parse_disc_list_resp(struct disc_exp_data *exp,
			      struct smp_discover_list_rsp *disc_list_resp)
{
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	struct disc_device_data *dev = NULL;

	SAL_ASSERT(NULL != exp, return);
	SAL_ASSERT(NULL != disc_list_resp, return);

	dev = exp->device;
	card = &disc_card_table[dev->card_id];
	port = &card->disc_port[dev->port_id];

	if (DISC_STATUS_UPSTREAM == port->discover_status)
		sal_up_stream_disc_list(exp, disc_list_resp);
	else if (DISC_STATUS_DOWNSTREAM == port->discover_status)
		sal_down_stream_disc_list(exp, disc_list_resp);
}

/**
 * sal_report_rev_chg - check all device in the port list, 
 * find change device infomation.
 * @port: point to port
 */
void sal_report_rev_chg(struct disc_port *port)
{
	struct disc_card *card = NULL;
	s32 ret = ERROR;
	unsigned long flag = 0;
	struct disc_device_data *dev_tmp = NULL;
	struct list_head *list = NULL;
	struct list_head *list_tmp = NULL;
	struct disc_event_intf *event_func = NULL;

	SAL_ASSERT(NULL != port, return);

	card = (struct disc_card *)port->card;
	event_func = &disc_event_func_table[card->card_no];

	ret = sal_check_port_valid(port);
	if (ERROR == ret) {
		SAL_TRACE(OSP_DBL_INFO, 1, "Err! the port is invalid");
		return;
	}

	spin_lock_irqsave(&card->device_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(list, list_tmp, &port->disc_device) {
		dev_tmp =
		    SAS_LIST_ENTRY(list, struct disc_device_data, device_link);

		 /*divice add is handle in discover,we just need to deal remove device*/
		if (dev_tmp->valid_now != dev_tmp->valid_before) {
			SAL_TRACE(OSP_DBL_INFO, 999,
				  "Card:%d port:%u disc over,device:0x%llx is valid? before:%s,now:%s",
				  card->card_no,
				  port->port_id,
				  dev_tmp->sas_addr,
				  dev_tmp->valid_before ? "Yes" : "no",
				  dev_tmp->valid_now ? "Yes" : "no");
		}

		if (DISC_RESULT_TRUE == dev_tmp->valid_before) {
			if (DISC_RESULT_FALSE == dev_tmp->valid_now) {
				/*device remove*/
				SAL_TRACE(OSP_DBL_DATA, 156,
					  "Card:%d port:%d send msg to Event-Q to delete device:0x%llx "
					  "parent:0x%llx phy_id:%d target:%d link_rate:%d",
					  card->card_no,
					  port->port_id,
					  dev_tmp->sas_addr,
					  dev_tmp->father_sas_addr,
					  dev_tmp->phy_id,
					  dev_tmp->tgt_proto,
					  dev_tmp->link_rate);
				sal_remove_exp_list(port, dev_tmp->sas_addr);

				event_func->device_remove(dev_tmp);
				sal_reinit_dev(dev_tmp);
				SAS_LIST_DEL_INIT(&dev_tmp->device_link);
				SAS_LIST_ADD_TAIL(&dev_tmp->device_link,
						  &card->free_device);
			} else {
				dev_tmp->valid_before = DISC_RESULT_TRUE;
				dev_tmp->valid_now = DISC_RESULT_FALSE;
			}
		} else {
			/*new device*/
			SAL_TRACE(OSP_DBL_DATA, 157,
				  "Card:%d port:%d send msg to Event-Q to add dev:0x%llx "
				  "father dev:0x%llx phy_id:%d target:%d rate:%d",
				  card->card_no,
				  port->port_id,
				  dev_tmp->sas_addr,
				  dev_tmp->father_sas_addr,
				  dev_tmp->phy_id,
				  dev_tmp->tgt_proto, dev_tmp->link_rate);
			dev_tmp->valid_before = DISC_RESULT_TRUE;
			dev_tmp->valid_now = DISC_RESULT_FALSE;
		}
	}
	spin_unlock_irqrestore(&card->device_lock, flag);
}

void sal_refresh_disc(struct disc_exp_data *exp, struct disc_port *port)
{
	SAL_ASSERT(exp, return);
	SAL_ASSERT(port, return);

	if (DISC_STATUS_UPSTREAM == port->discover_status)
		exp->flag = DISC_EXP_DOWN_DISCOVERING;
	else
		exp->flag = DISC_EXP_NOT_PROCESS;
}

/**
 * sal_down_stream_discovering - send/revice/parse discover list frame
 * @dev: point to device
 */
void sal_down_stream_discovering(struct disc_device_data *dev)
{
	u8 card_id = 0;
	u32 port_id = 0;
	s32 ret = ERROR;
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	struct disc_exp_data *exp = NULL;

	card_id = dev->card_id;
	port_id = dev->port_id;
	card = &disc_card_table[card_id];
	port = &card->disc_port[port_id];

	do {
		ret = sal_check_port_valid(port);
		if (ERROR == ret) {
			port->discover_status = DISC_STATUS_NOT_START;
			sal_clear_port_rsc(card_id, port_id);
			return;
		}

		/*obtain a expander from expander list*/
		exp = sal_get_exp_by_flag(port, DISC_EXP_DOWN_DISCOVERING);
		if (NULL == exp) {
			SAL_TRACE(OSP_DBL_DATA, 937,
				  "Card:%d port:%u has no more expander to do down stream",
				  card_id, port_id);
			break;
		}

		sal_refresh_disc(exp, port);

		SAL_TRACE(OSP_DBL_INFO, 936,
			  "Card:%d port:%u down stream,send report general to expander:0x%llx",
			  card_id, port_id, exp->sas_addr);

		SAL_ASSERT(NULL != exp->device, return);

		ret = sal_exe_report_gen(exp->device);
		if (ERROR == ret) {
			SAL_TRACE(OSP_DBL_MAJOR, 160,
				  "(downstream)Card:%d port:%d report general to dev:0x%llx failed",
				  card_id, port_id, exp->sas_addr);

			sal_launch_to_redo_disc(exp);

			sal_notify_disc_done(port, DISC_FAILED_DONE);
			return;
		}
		exp->smp_retry_num = 0;

		/*discover all phy in the expander*/
		sal_disc_expander(exp);
	} while (NULL != exp);

	sal_report_rev_chg(port);

	sal_notify_disc_done(port, DISC_SUCESSED_DONE);
}

void sal_down_stream_discover(struct disc_device_data *dev)
{
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	struct disc_exp_data *expander = NULL;
	struct disc_exp_data *up_expander = NULL;

	expander = (struct disc_exp_data *)dev->expander;
	card = &disc_card_table[dev->card_id];
	port = &card->disc_port[dev->port_id];

	port->discover_status = DISC_STATUS_DOWNSTREAM;
	SAL_TRACE(OSP_DBL_DATA, 938, "Card:%d port:%u down stream discover",
		  dev->card_id, dev->port_id);
	up_expander = expander->upstream_exp;

	/*check double root*/
	if ((NULL != up_expander)
	    && (up_expander->upstream_exp == expander)) {
		SAL_TRACE(OSP_DBL_MINOR, 939,
			  "Card %d port %u has two root:"
			  "expander 0x%llx subtractive to subtractive expander 0x%llx",
			  dev->card_id, dev->port_id,
			  expander->sas_addr, up_expander->sas_addr);

		expander->flag = DISC_EXP_DOWN_DISCOVERING;
		expander->discovering_phy_id = 0;

		up_expander->flag = DISC_EXP_DOWN_DISCOVERING;
		up_expander->discovering_phy_id = 0;

		up_expander->upstream_exp = NULL;
	} else {
		/*no double root */
		expander->flag = DISC_EXP_DOWN_DISCOVERING;
		expander->discovering_phy_id = 0;
	}

	sal_down_stream_discovering(dev);
}

s32 sal_exec_disc_list(struct disc_exp_data *exp)
{
	u8 phy_num = 0;
	u8 ucLoop = 0;
	u8 phy_num_tmp = 0;
	u32 req_len = 0;
	u32 resp_len = 0;
	s32 ret = ERROR;
	struct disc_smp_req *smp_req = NULL;
	struct disc_smp_response *smp_resp = NULL;
	struct smp_discover_list_rsp *disc_list_resp = NULL;
	struct disc_device_data *dev = NULL;
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	struct disc_event_intf *event_func = NULL;

	dev = exp->device;
	card = &disc_card_table[dev->card_id];
	port = &card->disc_port[dev->port_id];
	req_len = sal_get_smp_req_len(SMP_DISCOVER_LIST);
	event_func = &disc_event_func_table[dev->card_id];

	do {
		smp_req = SAS_KMALLOC(req_len, GFP_KERNEL);
		if (NULL == smp_req) {
			SAL_TRACE(OSP_DBL_MAJOR, 163,
				  "Alloc SMP request memory failed");
			ret = ERROR;
			goto EXIT;
		}
		memset(smp_req, 0, req_len);

		smp_req->smp_frame_type = SMP_REQUEST;
		smp_req->smp_function = SMP_DISCOVER_LIST;
		smp_req->resp_len = 255;
		smp_req->req_len = 6;
		smp_req->req_type.disc_list.start_phy_id = phy_num;
		phy_num_tmp =
		    exp->phy_nums - (ucLoop * DISCLIST_DESCRIPTOR_NUM_PER_REQ);
		if (phy_num_tmp < DISCLIST_DESCRIPTOR_NUM_PER_REQ)
			smp_req->req_type.disc_list.num_of_descritor =
			    phy_num_tmp;
		else
			smp_req->req_type.disc_list.num_of_descritor =
			    DISCLIST_DESCRIPTOR_NUM_PER_REQ;

		smp_req->req_type.disc_list.phy_filter = 0;
		smp_req->req_type.disc_list.desc_type = DISCLIST_DESCRIPTOR_SHORT;	/*short */

		resp_len =
		    48 + 24 * smp_req->req_type.disc_list.num_of_descritor;
		ucLoop++;
		phy_num =
		    phy_num + smp_req->req_type.disc_list.num_of_descritor;

		smp_resp = SAS_KMALLOC(resp_len, GFP_KERNEL);
		if (NULL == smp_resp) {
			SAL_TRACE(OSP_DBL_MAJOR, 164,
				  "Alloc SMP response memory failed");
			ret = ERROR;
			goto EXIT;
		}

		memset(smp_resp, 0, resp_len);

		ret =
		    event_func->smp_frame_send(dev, smp_req, req_len, smp_resp,
					       resp_len);
		if (ERROR == ret) {
			SAL_TRACE(OSP_DBL_MAJOR, 165,
				  "Card:%d port:%d send DISCOVER LIST to expander:0x%llx failed",
				  dev->card_id, dev->port_id, dev->sas_addr);

			ret = sal_check_port_valid(port);
			if (ERROR == ret) {
				SAL_TRACE(OSP_DBL_MAJOR, 166,
					  "Card:%d Port:%d is invalid,clr port rsc...",
					  card->card_no, port->port_id);
				port->discover_status = DISC_STATUS_NOT_START;
				sal_clear_port_rsc(dev->card_id, dev->port_id);
				goto EXIT;
			}
		}

		if (OK == ret) {
			disc_list_resp = &smp_resp->response.disc_list_rsp;
			sal_parse_disc_list_resp(exp, disc_list_resp);
		}

		memset(smp_req, 0, req_len);
		memset(smp_resp, 0, resp_len);
		SAS_KFREE(smp_req);
		SAS_KFREE(smp_resp);
		smp_req = NULL;
		smp_resp = NULL;
	} while (phy_num < exp->phy_nums);

      EXIT:
	if (NULL != smp_req) {
		SAS_KFREE(smp_req);
		smp_req = NULL;
	}

	if (NULL != smp_resp) {
		SAS_KFREE(smp_resp);
		smp_resp = NULL;
	}

	return ret;

}

void sal_disc_expander(struct disc_exp_data *exp)
{
	SAL_ASSERT(NULL != exp, return);

	if (exp->long_resp)	/*SAS 2.0 */
		sal_exec_disc_list(exp);
	else			/*SAS 1.1 */
		sal_exec_disc(exp);
}

/**
 * sal_up_stream_discovering - get a expander and do up stream discover
 * @dev: point to device
 */
void sal_up_stream_discovering(struct disc_device_data *device)
{
	u8 card_id = 0;
	u32 port_id = 0;
	s32 ret = ERROR;
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	struct disc_exp_data *exp = NULL;

	card_id = device->card_id;
	port_id = device->port_id;
	card = &disc_card_table[card_id];
	port = &card->disc_port[port_id];

	do {
		ret = sal_check_port_valid(port);
		if (ERROR == ret) {
			port->discover_status = DISC_STATUS_NOT_START;
			sal_clear_port_rsc(card_id, port_id);
			return;
		}

		exp = sal_get_exp_by_flag(port, DISC_EXP_UP_DISCOVERING);
		if (NULL == exp) {
			SAL_TRACE(OSP_DBL_DATA, 937,
				  "Card:%d port:%u has no more expander to do up stream",
				  card_id, port_id);
			break;
		}

		sal_refresh_disc(exp, port);

		SAL_TRACE(OSP_DBL_INFO, 936,
			  "Card:%d port:%u up stream,send report general to expander:0x%llx",
			  card_id, port_id, exp->sas_addr);

		SAL_ASSERT(NULL != exp->device, return);

		ret = sal_exe_report_gen(exp->device);
		if (ERROR == ret) {
			SAL_TRACE(OSP_DBL_MAJOR, 169,
				  "(upstream)Card:%d port:%d report general to expander:0x%llx failed",
				  card_id, port_id, exp->sas_addr);

			sal_launch_to_redo_disc(exp);

			sal_notify_disc_done(port, DISC_FAILED_DONE);
			return;
		}
		exp->smp_retry_num = 0;
		
		/*discover all phy in the expander*/
		sal_disc_expander(exp);
	} while (NULL != exp);

	sal_down_stream_discover(device);
}

/**
 * sal_up_stream_discover - find root expander from the input expander device
 * @device: point to device
 */
void sal_up_stream_discover(struct disc_device_data *device)
{
	u8 card_id = 0;
	u32 port_id = 0;
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;
	struct disc_exp_data *exp = NULL;

	card_id = device->card_id;
	port_id = device->port_id;
	SAL_TRACE(OSP_DBL_DATA, 932,
		  "Card:%d Port:%u up stream starting", card_id, port_id);
	card = &disc_card_table[card_id];
	port = &card->disc_port[port_id];
	port->discover_status = DISC_STATUS_UPSTREAM;
	exp = sal_find_exp_by_sas_addr(port, device->sas_addr);

	/*if the expander is not in the expander list,
	  add to the expander list*/ 
	if (NULL == exp) {
		exp = sal_get_free_expander(card);
		if (NULL == exp) {
			SAL_TRACE(OSP_DBL_MAJOR, 933,
				  "Card:%d get free expander node failed",
				  card_id);
			return;
		}

		exp->device = device;
		exp->card_id = card_id;
		exp->port_id = port_id;
		exp->num_of_upstream_phys = 0;
		exp->discovering_phy_id = 0;
		exp->send_smp_phy_id = 0;
		exp->sas_addr = device->sas_addr;
		exp->retries = 0;
		exp->upstream_exp = NULL;
		exp->flag = DISC_EXP_UP_DISCOVERING;
		exp->num_of_downstream_phys = 0;
		exp->phy_nums = 0;
		exp->curr_upstream_phy_idx = 0;
		exp->curr_downstream_phy_idx = 0;

		device->expander = (void *)exp;

		sal_add_exp_list(exp);
	}

	sal_up_stream_discovering(device);
}

void sal_do_discover(struct disc_port *port)
{
	s32 ret = ERROR;
	u64 sas_addr = 0;
	struct disc_card *card = NULL;
	struct disc_device_data *device = NULL;
	struct sas_identify_frame *identify = NULL;
	char *str[] = {
		"step",
		"full",
		"unknown"
	};

	SAL_REF(str[0]);
	SAL_ASSERT(NULL != port, return);

	card = (struct disc_card *)port->card;

	ret = sal_check_port_info(port);
	if (ERROR == ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 1, "port:%d is invalid",
			  port->port_id);
		port->new_disc = DISC_RESULT_FALSE;
		return;
	}

	port->new_disc = DISC_RESULT_FALSE;
	SAL_TRACE(OSP_DBL_INFO, 917,
		  "=== Card:%d port:%u start discovering(disc type:%s) ===",
		  card->card_no, port->port_id, str[port->disc_type]);

	/*full discover, clear discover port resource*/ 
	if (SAINI_FULL_DISC == port->disc_type)
		sal_clear_port_rsc(card->card_no, port->port_id);

	identify = &port->identify;
	sas_addr = DISC_GET_SASADDR_FROM_IDENTIFY(identify);

	device = sal_find_dev_by_sas_addr(sas_addr, port);
	if (NULL == device) {
		/*new device, add it to list*/
		device = sal_add_port_dev(port);
		if (NULL == device) {
			SAL_TRACE(OSP_DBL_MAJOR, 1,
				  "Card:%d add port:%d dev(addr:0x%llx) failed",
				  card->card_no, port->port_id, sas_addr);
			sal_notify_disc_done(port, DISC_FAILED_DONE);
			return;
		}
	} else {
	    /*device has in list,check rate*/
		ret = sal_check_dev_rate(device, port->link_rate);
		if (ERROR == ret) {
			SAL_TRACE(OSP_DBL_MAJOR, 1,
				  "Card:%d Port:%d check dev rate failed",
				  card->card_no, port->port_id);
			sal_notify_disc_done(port, DISC_FAILED_DONE);
			return;
		}

		device->valid_before = DISC_RESULT_TRUE;
		device->valid_now = DISC_RESULT_TRUE;
	}

	/*end device, discover is done*/
	if (END_DEVICE == device->dev_type) {
		SAL_TRACE(OSP_DBL_INFO, 929,
			  "Card:%d port:%u device:0x%llx is direct attached",
			  card->card_no, port->port_id, device->sas_addr);
		sal_notify_disc_done(port, DISC_SUCESSED_DONE);

		return;
	}

	/*find root expander */
	sal_up_stream_discover(device);

}


void sal_init_port(struct disc_port *port)
{
	u32 i = 0;

	SAL_ASSERT(NULL != port, return);

	SAS_INIT_LIST_HEAD(&port->disc_device);
	SAS_INIT_LIST_HEAD(&port->disc_expander);

	SAS_INIT_LIST_HEAD(&port->disc_head);
	SAS_SPIN_LOCK_INIT(&port->disc_lock);

	port->disc_thread = NULL;
	port->new_disc = DISC_RESULT_FALSE;
	port->link_rate = DISC_LINK_RATE_FREE;
	port->disc_switch = PORT_STATUS_FORBID;
	port->discover_status = DISC_STATUS_NOT_START;

	SAS_ATOMIC_SET(&port->ref_cnt, 0);

	for (i = 0; i < DISC_MAX_DEVICE_NUM; i++) {
		port->abnormal[i].sas_addr = 0;
		port->abnormal[i].reset_cnt = 0;
	}
}

/**
 * sal_reg_event_func - discover event register function 
 * @card_no: card number, the same as driver
 * @disc_event: point to discover event
 */
s32 sal_reg_event_func(u8 card_no, struct disc_event_intf *disc_event)
{
	SAL_ASSERT(NULL != disc_event, return RETURN_PARAM_ERROR);
	SAL_ASSERT(DISC_MAX_CARD_NUM > card_no, return RETURN_PARAM_ERROR);

	memset(&disc_event_func_table[card_no], 0,
	       sizeof(struct disc_event_intf));

	if ((NULL == disc_event->device_arrival)
	    || (NULL == disc_event->device_remove)
	    || (NULL == disc_event->discover_done)
	    || (NULL == disc_event->smp_frame_send)) {
		SAL_TRACE(OSP_DBL_MAJOR, 900,
			  "Err! Card %d register function is null,register failed",
			  card_no);
		return ERROR;
	}

	memcpy(&disc_event_func_table[card_no],
	       disc_event, sizeof(struct disc_event_intf));

	return OK;
}

/**
 * sal_init_disc_rsc - discover resource init. 
 * @card_no: card number, the same as driver
 * @disc_event: point to discover event
 */
s32 sal_init_disc_rsc(u8 card_no, struct disc_event_intf * disc_event)
{
	struct disc_card *disc_card = NULL;
	struct disc_port *disc_port = NULL;
	struct disc_device_data *free_dev = NULL;
	struct disc_exp_data *free_exp = NULL;
	u32 dev_mem_len = 0;
	u32 exp_mem_len = 0;
	u32 i = 0;
	s32 ret = ERROR;
	u32 port_no = 0;

	SAL_ASSERT(NULL != disc_event, return RETURN_PARAM_ERROR);
	SAL_ASSERT(card_no < DISC_MAX_CARD_NUM, return RETURN_PARAM_ERROR);

	if (DISC_RESULT_TRUE == disc_card_table[card_no].inited) {
		SAL_TRACE(OSP_DBL_MAJOR, 177, "Card:%d already inited",
			  card_no);
		return ERROR;
	}

	memset(&disc_card_table[card_no], 0, sizeof(struct disc_card));

	disc_card = &disc_card_table[card_no];

	disc_card->inited = DISC_RESULT_TRUE;
	disc_card->card_no = card_no;

	dev_mem_len = sizeof(struct disc_device_data) * DISC_MAX_DEVICE_NUM;
	disc_card->device_mem = SAS_KMALLOC(dev_mem_len, GFP_ATOMIC);
	if (NULL == disc_card->device_mem) {
		SAL_TRACE(OSP_DBL_MAJOR, 901,
			  "Card %d vmalloc device memory failed", card_no);
		goto ERROR_EXIT;
	}

	disc_card->disc_card_flag |= DISCARD_MALLOC_DEVICE_MEM;

	/*alloc expander list resource*/
	exp_mem_len = sizeof(struct disc_exp_data) * DISC_MAX_EXP_NUM;
	disc_card->expander_mem = SAS_KMALLOC(exp_mem_len, GFP_ATOMIC);
	if (NULL == disc_card->expander_mem) {
		SAL_TRACE(OSP_DBL_MAJOR, 902,
			  "Card %d vmalloc expander memory failed", card_no);

		goto ERROR_EXIT;
	}

	disc_card->disc_card_flag |= DISCARD_MALLOC_EXPANDER_MEM;

	SAS_SPIN_LOCK_INIT(&disc_card->device_lock);
	SAS_INIT_LIST_HEAD(&disc_card->free_device);

	SAS_SPIN_LOCK_INIT(&disc_card->expander_lock);
	SAS_INIT_LIST_HEAD(&disc_card->free_expander);

	memset(disc_card->device_mem, 0, dev_mem_len);
	free_dev = (struct disc_device_data *)disc_card->device_mem;
	for (i = 0; i < DISC_MAX_DEVICE_NUM; i++) {
		free_dev->valid_before = DISC_RESULT_FALSE;
		free_dev->valid_now = DISC_RESULT_FALSE;
		SAS_LIST_ADD_TAIL(&free_dev->device_link,
				  &disc_card->free_device);
		free_dev++;
	}

	memset(disc_card->expander_mem, 0, exp_mem_len);
	free_exp = (struct disc_exp_data *)disc_card->expander_mem;
	for (i = 0; i < DISC_MAX_EXP_NUM; i++) {
		SAS_LIST_ADD_TAIL(&free_exp->exp_list,
				  &disc_card->free_expander);
		free_exp++;
	}

	/*init port infomation*/
	for (port_no = 0; port_no < DISC_MAX_PORT_NUM; port_no++) {
		disc_port = &disc_card->disc_port[port_no];

		disc_port->card = (void *)disc_card;
		disc_port->port_id = port_no;

		sal_init_port(disc_port);
	}

	/*register event*/
	ret = sal_reg_event_func(card_no, disc_event);
	if (OK == ret)
		disc_card->disc_card_flag |= DISCARD_REGISTER_INTF_SUCESS;

	SAL_TRACE(OSP_DBL_DATA, 181,
		  "Card:%d expphy0:%d expphy1:%d priphy0:%d priphy1:%d",
		  card_no,
		  disc_card->disc_topo.exp_phy_id_0,
		  disc_card->disc_topo.exp_phy_id_1,
		  disc_card->disc_topo.pri_phy_id_0,
		  disc_card->disc_topo.pri_phy_id_1);
	return OK;

      ERROR_EXIT:
	sal_release_disc_rsc(card_no);
	return ERROR;
}

/**
 * sal_release_disc_rsc - discover resource release. 
 * @card_no: card number
 */
void sal_release_disc_rsc(u8 card_no)
{
	struct disc_card *card = NULL;
	struct disc_event_intf *event_func = NULL;

	SAL_ASSERT(card_no < DISC_MAX_CARD_NUM, return);

	SAL_TRACE(OSP_DBL_INFO, 182, "Card:%d release disc rsc", card_no);

	if (DISC_RESULT_FALSE == disc_card_table[card_no].inited) {
		SAL_TRACE(OSP_DBL_MAJOR, 182, "Card:%d was not inited",
			  card_no);
		return;
	}

	card = &disc_card_table[card_no];
	event_func = &disc_event_func_table[card_no];

	SAS_INIT_LIST_HEAD(&card->free_device);
	SAS_INIT_LIST_HEAD(&card->free_expander);

	/*release device memory*/
	if (card->disc_card_flag & DISCARD_MALLOC_DEVICE_MEM) {
		SAS_KFREE(card->device_mem);
		card->device_mem = NULL;
	}

	/*release expander memory*/
	if (card->disc_card_flag & DISCARD_MALLOC_EXPANDER_MEM) {
		SAS_KFREE(card->expander_mem);
		card->expander_mem = NULL;
	}

	if (card->disc_card_flag & DISCARD_REGISTER_INTF_SUCESS)
		memset(event_func, 0, sizeof(struct disc_event_intf));

	card->inited = DISC_RESULT_FALSE;
	return;
}

void sal_set_disc_card_false(u8 card_no)
{
	SAL_ASSERT(card_no < DISC_MAX_CARD_NUM, return);

	disc_card_table[card_no].inited = DISC_RESULT_FALSE;
}

void sal_port_down_notify(u8 card_id, u32 port_id)
{
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;

	SAL_ASSERT(card_id < DISC_MAX_CARD_NUM, return);
	SAL_ASSERT(port_id < DISC_MAX_PORT_NUM, return);

	card = &disc_card_table[card_id];
	port = &card->disc_port[port_id];

	SAL_TRACE(OSP_DBL_INFO, 911,
		  "Card:%d port:%u is down,notify to stop disc progress",
		  card_id, port_id);

	/*set port status to invalid, to stop the discovering */
	port->disc_switch = PORT_STATUS_FORBID;

	if (DISC_STATUS_NOT_START == port->discover_status)
		sal_clear_port_rsc(card_id, port_id);

	return;
}

void sal_port_up_notify(u8 card_id, u32 port_id)
{
	struct disc_card *card = NULL;
	struct disc_port *port = NULL;

	SAL_ASSERT(card_id < DISC_MAX_CARD_NUM, return);
	SAL_ASSERT(port_id < DISC_MAX_PORT_NUM, return);

	card = &disc_card_table[card_id];
	port = &card->disc_port[port_id];

	port->disc_switch = PORT_STATUS_PERMIT;

	return;
}

/**
 * sal_disc_redo - discover again
 * @disc_port: port to discover
 */
void sal_disc_redo(struct disc_port *disc_port)
{
	struct disc_msg *disc_msg = NULL;

	disc_msg = SAS_KMALLOC(sizeof(struct disc_msg), GFP_ATOMIC);
	if (NULL == disc_msg) {
		SAL_TRACE(OSP_DBL_MAJOR, 911, "Kmalloc mem failed");
		return;
	}

	SAL_TRACE(OSP_DBL_INFO, 911, "Port:%d add msg to discover list",
		  disc_port->port_id);

	memset(disc_msg, 0, sizeof(struct disc_msg));

	disc_msg->port_disc_type = SAINI_STEP_DISC;
	disc_msg->port_rsc_idx = SAINI_UNKNOWN_PORTINDEX;
	disc_msg->proc_flag = SAL_TRUE;

	SAS_INIT_LIST_HEAD(&disc_msg->disc_list);
	SAS_LIST_ADD_TAIL(&disc_msg->disc_list, &disc_port->disc_head);
	return;
}

/**
 * sal_check_new_disc - check if need to do a new discover,
 * if need,to it.
 * @port: port to discover
 */
void sal_check_new_disc(struct disc_port *port)
{
	unsigned long flag = 0;

	SAL_ASSERT(NULL != port, return);

	spin_lock_irqsave(&port->disc_lock, flag);
	if (DISC_RESULT_TRUE == port->new_disc) {
		sal_disc_redo(port);

	} else {
		if (list_empty(&port->disc_head))
			port->discover_status = DISC_STATUS_NOT_START;
	}
	spin_unlock_irqrestore(&port->disc_lock, flag);
	return;
}

void sal_launch_to_redo_disc(struct disc_exp_data *exp_dev)
{
	struct disc_port *disc_port = NULL;
	struct disc_card *disc_card = NULL;
	unsigned long flag = 0;

	SAL_ASSERT(exp_dev->card_id < DISC_MAX_CARD_NUM, return);
	SAL_ASSERT(exp_dev->port_id < DISC_MAX_PORT_NUM, return);

	disc_card = &disc_card_table[exp_dev->card_id];
	disc_port = &disc_card->disc_port[exp_dev->port_id];
	exp_dev->smp_retry_num++;

	if (exp_dev->smp_retry_num > DISC_MAX_REDO_DISC_NUM) {
		SAL_TRACE(OSP_DBL_MINOR, 169,
			  "Card:%d port:%d expander:0x%llx disc retry %d times",
			  disc_card->card_no, disc_port->port_id,
			  exp_dev->sas_addr, exp_dev->smp_retry_num);
		return;
	}
	spin_lock_irqsave(&disc_port->disc_lock, flag);
	sal_disc_redo(disc_port);
	spin_unlock_irqrestore(&disc_port->disc_lock, flag);

	return;
}

