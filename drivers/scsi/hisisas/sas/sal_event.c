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
/* temp modify */
#define PV660_ARM_SERVER
/* temporary code, to solve compile problem */
#define SASINI_INVALID_HOSTID    0xffffffff
#define SASINI_HBA_SASADDR       0x0807060504030201

enum sas_ini_dev_type {	
	SASINI_DEVICE_FREE = 0,	
    SASINI_DEVICE_SES,  
    SASINI_DEVICE_SAS,  
    SASINI_DEVICE_SATA, 
    SASINI_DEVICE_BUTT
};
enum sas_ini_intf_event {
	SASINI_EVENT_DEV_IN = 0,	
	SASINI_EVENT_DEV_OUT,	
	SASINI_EVENT__DEV_BUTT
};

enum sas_ini_event {
	SASINI_EVENT_DISK_IN = 0,
	SASINI_EVENT_DISK_OUT,	
	SASINI_EVENT_FRAME_IN,	
	SASINI_EVENT_FRAME_OUT,
	SASINI_EVENT_BUTT
};

/* temporary code, to solve compile problem */

/*----------------------------------------------*
 * global variable type define.                               *
 *----------------------------------------------*/
extern struct workqueue_struct *sal_wq;
/*----------------------------------------------*
 * inner variable type define.                             *
 *----------------------------------------------*/
sal_hw_event_cbfn_t sal_event_cb_table[SAL_MAX_HWEVENT];
/*----------------------------------------------*
 *  inner function type define.                         *
 *----------------------------------------------*/
s32 sal_phy_down(struct sal_port *port, struct sal_phy *phy,
		 struct sal_phy_event info);
s32 sal_phy_up(struct sal_port *port, struct sal_phy *phy,
	       struct sal_phy_event info);
s32 sal_phy_stop_rsp(struct sal_port *port, struct sal_phy *phy,
		     struct sal_phy_event info);
s32 sal_phy_bcast(struct sal_port *port, struct sal_phy *phy,
		  struct sal_phy_event info);
void sal_event_process(struct sal_card *sal_card);
enum sas_ini_event sal_get_chg_dly_event_type(u32 event, u32 dev_type);
u8 sal_check_wide_port_phy_rate(struct sal_card *sal_card, u32 phy_cnt,
				struct sal_phy_rate_chk_param *phy_rate);
void sal_check_wide_port(struct sal_card *sal_card, struct sal_port *sal_port);
s32 sal_set_phy_opt_info(struct sal_card *sal_card,
			 u8 phy_id, u32 phy_rate, enum sal_phy_op_type opt);

u32 sal_get_chg_delay_dev_type(struct sal_dev *device);

void sal_notify_report_dev_in_without_lock(u8 card_no, u32 scsi_id, u32 chan_id, u32 lun_id);
void sal_free_dev_rsc_without_lock(u8 card_no, u32 scsi_id, u32 chan_id, u32 lun_id);

enum sal_exec_cmpl_status sal_clear_ll_dev(struct sal_card *sal_card,
					   struct sal_dev *dev);

/*----------------------------------------------*
 * event process function initialization.                           *
 *----------------------------------------------*/
void sal_event_init(void)
{
	sal_event_cb_table[SAL_PHY_EVENT_DOWN] = sal_phy_down;
	sal_event_cb_table[SAL_PHY_EVENT_UP] = sal_phy_up;
	sal_event_cb_table[SAL_PHY_EVENT_BCAST] = sal_phy_bcast;
	sal_event_cb_table[SAL_PHY_EVENT_STOP_RSP] = sal_phy_stop_rsp;
}

/*----------------------------------------------*
 * event process function exit .                           *
 *----------------------------------------------*/
void sal_event_exit(void)
{
	sal_event_cb_table[SAL_PHY_EVENT_DOWN] = NULL;
	sal_event_cb_table[SAL_PHY_EVENT_UP] = NULL;
	sal_event_cb_table[SAL_PHY_EVENT_BCAST] = NULL;
	sal_event_cb_table[SAL_PHY_EVENT_STOP_RSP] = NULL;
}

/*----------------------------------------------*
 * handware event process function .                           *
 *----------------------------------------------*/
s32 sal_phy_event(struct sal_port *port,
		  struct sal_phy *phy, struct sal_phy_event phy_event_info)
{
	char *link_str[] = {
		"Free",
		"1.5G",
		"3.0G",
		"6.0G",
		"12.0G",
		"Unknown"
	};
	char *event_str[] = {
		"Unknown",
		"Link Down",
		"Link Up",
		"Bcast change",
		"StopAck",
	};
	link_str[0] = link_str[0];	/* for lint */
	event_str[0] = event_str[0];

	/*port can be NULL */
	SAL_ASSERT(NULL != phy, return ERROR);

	SAL_TRACE(OSP_DBL_DATA, 535,
		  "Card:%d/port:%d/phy:%d event:0x%0x(%s) rate:%d(%s)",
		  phy_event_info.card_id, phy_event_info.fw_port_id,
		  phy_event_info.phy_id, phy_event_info.event,
		  event_str[phy_event_info.event], phy_event_info.link_rate,
		  link_str[phy_event_info.link_rate]);

	if (OK !=
	    sal_event_cb_table[phy_event_info.event] (port, phy,
						      phy_event_info)) {
		SAL_TRACE(OSP_DBL_MAJOR, 536,
			  "card:%d/port:%d/phy:%d event:0x%x process failed",
			  phy_event_info.card_id, phy_event_info.fw_port_id,
			  phy_event_info.phy_id, phy_event_info.event);
		return ERROR;
	}

	return OK;
}

EXPORT_SYMBOL(sal_phy_event);

/*----------------------------------------------*
 * check if there is LINK UP PHY .                           *
 *----------------------------------------------*/
bool sal_is_all_phy_down(struct sal_port * port)
{
	u32 i = 0;
	struct sal_card *sal_card = NULL;

	sal_card = port->card;

	for (i = 0; i < sal_card->phy_num; i++)
		if (SAL_TRUE == port->phy_id_list[i])
			return SAL_FALSE;

	return SAL_TRUE;
}

/*----------------------------------------------*
 * set port speed function .                           *
 *----------------------------------------------*/
void sal_set_port_rate(struct sal_port *port)
{
	u32 i = 0;
	u8 max_rate = 0;
	u8 min_rate = 0;
	struct sal_phy *tmp_phy = NULL;
	struct sal_card *sal_card = NULL;

	sal_card = port->card;

	for (i = 0; i < sal_card->phy_num; i++)
		if (SAL_TRUE == port->phy_id_list[i]) {
			tmp_phy = ((struct sal_card *)(port->card))->phy[i];
			max_rate = MAX(tmp_phy->link_rate, max_rate);
			min_rate = MIN(tmp_phy->link_rate, min_rate);
		}

	port->max_rate = max_rate;
	port->min_rate = min_rate;
}

/*----------------------------------------------*
 * check if record the phy change , if yes , record it.             *
 *----------------------------------------------*/
void sal_phy_change_check(struct sal_phy *phy)
{
	u32 diff_sec = 1;	/* time interval within 1s, don't record phy change. */
	unsigned long cur_jiff = jiffies;

	if (time_after(cur_jiff,
		       phy->err_code.last_switch_jiff +
		       (unsigned long)diff_sec * HZ)) {

		SAL_TRACE(OSP_DBL_DATA, 550,
			  "PortId:%d PhyId:%d increase phy change 1 time",
			  phy->port_id, phy->phy_id);

		phy->err_code.phy_chg_cnt_per_period++;
	}
	return;
}

/*----------------------------------------------*
 * PHY DOWN process function .                           *
 *----------------------------------------------*/
s32 sal_phy_down(struct sal_port * port, struct sal_phy * phy,
		 struct sal_phy_event info)
{
	u8 phy_id = 0;
	struct sal_card *sal_card = NULL;

	SAL_REF(info);
	SAL_REF(sal_card);
	SAL_ASSERT(NULL != port, return ERROR);

	sal_card = port->card;
	phy_id = phy->phy_id;

	SAL_ASSERT(SAL_MAX_PHY_NUM > phy_id, return ERROR);

	phy->link_status = SAL_PHY_LINK_DOWN;
	phy->link_rate = SAL_LINK_RATE_FREE;
	phy->port_id = SAL_INVALID_PORTID;
	memset(&phy->remote_identify, 0, sizeof(struct sas_identify));

	port->phy_id_list[phy_id] = SAL_FALSE;

	SAL_TRACE(OSP_DBL_INFO, 550,
		  "Card:%d/Logic PortId:0x%x/Port(fw):%d/Phy id:%d link down",
		  sal_card->card_no, port->logic_id, port->port_id,
		  phy->phy_id);

	if (SAL_TRUE == sal_is_all_phy_down(port)) {
		port->status = SAL_PORT_LINK_DOWN;	/*port is closed.) */
		(void)sal_all_phy_down_handle(port);
	}

	sal_phy_change_check(phy);
	/*
	 *update max speed and min speed for port.
	 */
	sal_set_port_rate(port);

	/*light led */
	sal_comm_led_operation(sal_card);

	return OK;
}

/*----------------------------------------------*
 * After close phy , then process sal-layer.           *
 *----------------------------------------------*/
s32 sal_phy_stop_rsp(struct sal_port * port, struct sal_phy * phy,
		     struct sal_phy_event info)
{
	u8 phy_id = 0;
	struct sal_card *sal_card = NULL;

	SAL_REF(info);
	SAL_REF(sal_card);
	phy_id = phy->phy_id;
	SAL_ASSERT(SAL_MAX_PHY_NUM > phy_id, return ERROR);

	phy->port_id = SAL_INVALID_PORTID;
	phy->link_status = SAL_PHY_CLOSED;
	phy->link_rate = SAL_LINK_RATE_FREE;
	memset(&phy->remote_identify, 0, sizeof(struct sas_identify));

	if (NULL == port)
		/* The phy don't negotiate, don't have port. */
		return OK;

	port->phy_id_list[phy_id] = SAL_FALSE;

	sal_card = port->card;

	if (SAL_TRUE == sal_is_all_phy_down(port)) {
		port->status = SAL_PORT_LINK_DOWN;	/*port is closed. */
		(void)sal_all_phy_down_handle(port);
	}

	/*
	 *update max speed and min speed in port.
	 */
	sal_set_port_rate(port);

	/*light led*/
	sal_comm_led_operation(sal_card);

	return OK;
}

/*----------------------------------------------*
 * startup DISCOVERY.           *
 *----------------------------------------------*/
void sal_start_disc(struct sal_port *port, struct sal_phy *phy)
{
	struct disc_port_context disc_port_context;
	unsigned long flag = 0;
	struct sal_card *sal_card = NULL;
	enum sal_disc_type port_disc_type = SAL_INI_DISC_BUTT;

	SAL_ASSERT(NULL != port, return);

	sal_card = port->card;

	spin_lock_irqsave(&sal_card->card_lock, flag);
	if (SAL_INVALID_PORTID == port->port_id) {
		SAL_TRACE(OSP_DBL_MAJOR, 550,
			  "Card:%d Port(fw):%d(Logic PortId:0x%x/Phy id:%d) invalid,don't disc",
			  sal_card->card_no, port->port_id, port->logic_id,
			  phy->phy_id);
		spin_unlock_irqrestore(&sal_card->card_lock, flag);
		return;
	}


	if (port->port_need_do_full_disc == SAL_TRUE) {
		/*The port need to execute FULL discovery.  */
		port_disc_type = SAL_INI_FULL_DISC;
		port->port_need_do_full_disc = SAL_FALSE;
	} else {
		port_disc_type = SAL_INI_STEP_DISC;
	}

	if ((SAL_TRUE == sal_card->reseting)
	    || (SAL_CARD_REMOVE_PROCESS & sal_card->flag)) {
		/*it is reseting chip */
		SAL_TRACE(OSP_DBL_INFO, 550,
			  "Card:%d(Logic PortId:0x%x/Port(fw):%d/Phy id:%d) is resetting(sign:0x%x)/removing(flag:0x%x) now,don't disc",
			  sal_card->card_no, port->logic_id, port->port_id,
			  phy->phy_id, sal_card->reseting, sal_card->flag);
		spin_unlock_irqrestore(&sal_card->card_lock, flag);
		return;

	}

	disc_port_context.card_no = port->card->card_no;
	disc_port_context.port_id = port->port_id;
	disc_port_context.port_rsc_idx = (u8) port->index;
	disc_port_context.disc_type = port_disc_type;

	/*convert into DISCOVERY speed. */
	switch (phy->link_rate) {
	case SAL_LINK_RATE_1_5G:
		disc_port_context.link_rate = DISC_LINK_RATE_1_5_G;
		break;

	case SAL_LINK_RATE_3_0G:
		disc_port_context.link_rate = DISC_LINK_RATE_3_0_G;
		break;

	case SAL_LINK_RATE_6_0G:
		disc_port_context.link_rate = DISC_LINK_RATE_6_0_G;
		break;

	case SAL_LINK_RATE_12_0G:
		disc_port_context.link_rate = DISC_LINK_RATE_12_0_G;
		break;

	default:
		disc_port_context.link_rate = 0;
		spin_unlock_irqrestore(&sal_card->card_lock, flag);
		return;
	}

	disc_port_context.local_sas_addr = port->sas_addr;
	memcpy(&disc_port_context.sas_identify, &phy->remote_identify,
	       sizeof(struct sas_identify));
#if 0
	/* 防止IO失败 */
	port->ucJamFlag = SAL_PORT_NEED_JAM;
#endif

	/*schedule DISCOVERY module.  */
	sal_discover(&disc_port_context);
	spin_unlock_irqrestore(&sal_card->card_lock, flag);

	return;
}

/*----------------------------------------------*
 * SAL-layer interface card turn off led function.           *
 *----------------------------------------------*/
void sal_turn_off_all_led(struct sal_card *sal_card)
{
	struct sal_led_op_param led_para;

	memset(&led_para, 0, sizeof(struct sal_led_op_param));
	led_para.port_speed_led = SAL_LED_ALL_OFF;
	led_para.phy_id = 0;	/*phy_id:0 */


	if (delayed_work_pending(&sal_card->port_led_work))
		(void)cancel_delayed_work(&sal_card->port_led_work);

	flush_workqueue(sal_wq);

	/* Quark_PortOperation close all led. */
	if (OK !=
	    sal_card->ll_func.port_op(sal_card, SAL_PORT_OPT_LED,
				      (void *)(&led_para)))
		SAL_TRACE(OSP_DBL_MAJOR, 671, "Card:%d led all off failed",
			  sal_card->card_no);

	return;
}

EXPORT_SYMBOL(sal_turn_off_all_led);

/*----------------------------------------------*
 * SAL-layer interface card turn off led function.           *
 *----------------------------------------------*/
/*lint -e801 */
s32 sal_start_work(void *argv_in, void *argv_out)
{
	s32 ret = ERROR;
	u32 card_id = (u32) (u64) argv_in;
	struct sal_card *card = NULL;

	SAL_REF(argv_out);	//lint
	card = sal_get_card(card_id);
	if (NULL == card) {

		SAL_TRACE(OSP_DBL_MAJOR, 520, "Get card fail by card id:%d",
			  card_id);

		return ERROR;
	}

	if (NULL == card->ll_func.port_op) {
		SAL_TRACE(OSP_DBL_MAJOR, 671, "Card:%d port_op is null",
			  card->card_no);
		sal_put_card(card);
		return ERROR;
	}

	/* Quark_PortOperation */
	ret = card->ll_func.port_op(card, SAL_PORT_OPT_ALL_START_WORK, NULL);
	if (OK != ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 671, "Card:%d start work failed",
			  card->card_no);
		sal_put_card(card);
		return ERROR;
	}
	/* start up timer to light led .  */
	sal_comm_led_operation(card);
	sal_put_card(card);
	return ret;
	/*lint +e801 */
}

EXPORT_SYMBOL(sal_start_work);

/*----------------------------------------------*
 * BCAST process function.           *
 *----------------------------------------------*/
s32 sal_phy_bcast(struct sal_port * port, struct sal_phy * phy,
		  struct sal_phy_event info)
{
	struct sal_card *sal_card = NULL;

	SAL_REF(info);
	SAL_REF(sal_card);

	sal_card = port->card;

	SAL_TRACE(OSP_DBL_DATA, 550,
		  "Card:%d/Logic PortId:0x%x/Port(fw):%d/Phy id:%d broadcast",
		  sal_card->card_no, port->logic_id, port->port_id,
		  phy->phy_id);

	sal_start_disc(port, phy);
	return OK;
}

/*----------------------------------------------*
 *translate phy id by connect jbod.           *
 *----------------------------------------------*/
u32 sal_trans_phy_id_by_conn_jbod(struct sal_card * sal_card,
				  struct sal_port * sal_port,
				  struct sal_phy * sal_phy)
{
	u32 remote_lo_addr = 0;
	u32 ret_phy_id = sal_phy->phy_id;

	remote_lo_addr = *(u32 *) sal_phy->remote_identify.sas_addr_lo;

	if ((0x3d == ((remote_lo_addr & 0xff000000) >> 24))
	    && (SAS_EDGE_EXPANDER_DEVICE ==
		SAL_GET_DEVTYPE_FORMIDFRM(&sal_phy->remote_identify))) {
		/*port connect High-density frame */
		sal_port->is_conn_high_jbod = SAL_TRUE;

		switch (sal_card->chip_type) {
		case SAL_CHIP_TYPE_8072:	/*convert PHYID according to 8072 chip X8 port.*/
			if ((ret_phy_id >= 4) && (ret_phy_id <= 7))
				ret_phy_id = 0;	//map2:0x000f  ; map3:0x00f0
			else if ((ret_phy_id >= 8) && (ret_phy_id <= 11))
				ret_phy_id = 12;	//map0:0xf000 ; map1:0x0f00

			break;

		default:
			break;
		}
	}

	return ret_phy_id;
}

/*----------------------------------------------*
 *PHY UP process function.           *
 *----------------------------------------------*/
s32 sal_phy_up(struct sal_port * port, struct sal_phy * phy,
	       struct sal_phy_event info)
{
	struct sal_card *sal_card = NULL;
	u32 logic_port_id = 0;
	unsigned long flag = 0;
	u32 config_link_rate = 0;
	char *link_str[] = {
		"Free",
		"1.5G",
		"3.0G",
		"6.0G",
		"12.0G",
		"Unknown"
	};
	u32 trans_phy_id = 0;

	SAL_REF(link_str[0]);
	SAL_REF(config_link_rate);

	SAL_ASSERT(NULL != port, return ERROR);
	SAL_ASSERT(NULL != phy, return ERROR);

	sal_card = port->card;

	phy->phy_id = (u8) info.phy_id;
	phy->link_status = SAL_PHY_LINK_UP;
	phy->link_rate = info.link_rate;
	phy->port_id = (u8) info.fw_port_id;

	port->port_id = info.fw_port_id;
	port->phy_id_list[phy->phy_id] = SAL_TRUE;

	SAS_ATOMIC_SET(&port->port_in_use, SAL_PORT_RSC_IN_USE);	/*the port resource start to be used. */

	port->max_rate = MAX(phy->link_rate, port->max_rate);
	port->min_rate = MIN(phy->link_rate, port->min_rate);
	port->sas_addr = phy->local_addr;
	port->status = SAL_PORT_LINK_UP;
	port->link_mode = SAL_MODE_INDIRECT;	/*connect mode : default direct connect EXP */
	port->phy_up_in_port = SAL_PHY_UP_IN_PORT;	/*phy up event , it need to detect reliability in port opt. */

	if (SAL_INVALID_LOGIC_PORTID == port->logic_id) {

		trans_phy_id =
		    sal_trans_phy_id_by_conn_jbod(sal_card, port, phy);


		logic_port_id =
		    sal_comm_get_logic_id(sal_card->card_no, trans_phy_id);
		if (SAL_INVALID_LOGIC_PORTID != logic_port_id) {
			port->logic_id = logic_port_id;
		} else {	/*obtain  logic port id fail. */
			SAL_TRACE(OSP_DBL_MAJOR, 550,
				  "Card:%d phy:%d try to get logic port id failed",
				  sal_card->card_no, phy->phy_id);
			return ERROR;
		}
	}

	SAL_TRACE(OSP_DBL_INFO, 550,
		  "Card:%d/Logic PortId:0x%x/Port(fw):%d/Phy id:%2d link up(Rate:%d-%s)",
		  sal_card->card_no, port->logic_id, port->port_id, phy->phy_id,
		  phy->link_rate, link_str[phy->link_rate]);

/*cancel speed check. START */
#if 0
	config_link_rate = sal_card->config.phy_link_rate[phy->phy_id];
	/*如果速率达不到配置值，进行告警 */
	/*根据可靠性设计，进行一次PHY的复位操作 */
	if (phy->link_rate < sal_card->config.phy_link_rate[phy->phy_id]) {
		if (SAL_PHY_NEED_RESET_FOR_DFR == phy->phy_reset_flag) {
			SAL_TRACE(OSP_DBL_MINOR, 549,
				  "Card:%d phy:%2d link rate:0x%x(%s) less than config rate:0x%x(%s), restart it",
				  sal_card->card_no, phy->phy_id,
				  phy->link_rate, link_str[phy->link_rate],
				  config_link_rate, link_str[config_link_rate]);
			phy->phy_reset_flag = SAL_PHY_NO_RESET_FOR_DFR;
			(void)sal_set_phy_opt_info(sal_card,
						   phy->phy_id,
						   sal_card->config.
						   phy_link_rate[phy->phy_id],
						   SAL_PHY_OPT_RESET);
			return OK;
		}
	}
#else
	SAL_REF(config_link_rate);
#endif


	/*update disc port state. */
	sal_port_up_notify(sal_card->card_no, port->port_id);

	if (SAS_END_DEVICE == SAL_GET_DEVTYPE_FORMIDFRM(&phy->remote_identify))
		/*direct connect terminate device . */
		port->link_mode = SAL_MODE_DIRECT;

	/*phy change count */
	sal_phy_change_check(phy);

	/* add lock protect, avoid timer to add repeatedly.*/
	spin_lock_irqsave(&sal_card->card_lock, flag);
	if (!sal_timer_is_active((void *)&sal_card->port_op_timer))
		sal_add_timer((void *)&sal_card->port_op_timer,
			      (unsigned long)sal_card->card_no,
			      SAL_PORT_TIMER_DELAY, sal_wide_port_opt_func);

	spin_unlock_irqrestore(&sal_card->card_lock, flag);

	/*light led. */
	sal_comm_led_operation(sal_card);

	return OK;
}

/*----------------------------------------------*
 * PHY DOWN process function in a port.          *
 *----------------------------------------------*/
s32 sal_all_phy_down_handle(struct sal_port * port)
{
	struct sal_event *event = NULL;

	SAL_ASSERT(NULL != port, return ERROR);

	port->min_rate = 0;
	port->max_rate = 0;

	/*notify discover port the delete event. */
	sal_port_down_notify(port->card->card_no, port->port_id);

	event = sal_get_free_event(port->card);
	if (NULL == event) {
		SAL_TRACE(OSP_DBL_MAJOR, 541,
			  "Card:%d port:%d get free event node failed",
			  port->card->card_no, port->port_id);
		return ERROR;
	}

	memset(event, 0, sizeof(struct sal_event));
	event->time = jiffies;
	event->event = SAL_EVENT_DEL_PORT_DEV;
	event->info.port_event.port = port;

	SAL_TRACE(OSP_DBL_INFO, 542,
		  "Card:%d port:%d link down,notify to clr port rsc",
		  port->card->card_no, port->port_id);

	/*add event into event process queue. */
	sal_add_dev_event_thread(port->card, event);

	return OK;
}

EXPORT_SYMBOL(sal_all_phy_down_handle);

/*----------------------------------------------*
 * when unload driver, notify all port to end disc , set flag to avoid discover.     *
 *----------------------------------------------*/
s32 sal_stop_card_disc(u32 card_no)
{
	u32 i = 0;
	struct disc_port *port = NULL;
	struct disc_card *disc_card = NULL;
	unsigned long start_time = 0;
	s32 ret = ERROR;
	unsigned long disc_flag = 0;

	SAL_ASSERT(card_no < DISC_MAX_CARD_NUM, return ERROR);

	/*notify disc clear resource  */
	disc_card = &disc_card_table[card_no];

	for (i = 0; i < DISC_MAX_PORT_NUM; i++) {
		port = &disc_card->disc_port[i];
		sal_port_down_notify((u8) card_no, i);
	}

	/*wait all port disc end */
	for (i = 0; i < DISC_MAX_PORT_NUM; i++) {
		port = &disc_card->disc_port[i];
		start_time = jiffies;
		ret = ERROR;
		for (;;) {

			spin_lock_irqsave(&port->disc_lock, disc_flag);
			if (0 == SAS_ATOMIC_READ(&port->ref_cnt))
				/*The port disc end */
				ret = OK;

			spin_unlock_irqrestore(&port->disc_lock, disc_flag);

			if (OK == ret)
				break;

			SAS_MSLEEP(10);

			if (time_after(jiffies, start_time + 3 * HZ))
				SAL_TRACE_FRQLIMIT(OSP_DBL_MAJOR, 3, 308,
						   "Card:%d wait port:%d exit disc thread time out",
						   card_no, i);
		}
	}

	return OK;
}

EXPORT_SYMBOL(sal_stop_card_disc);


/*----------------------------------------------*
 * add preprocess event into event queue.     *
 *----------------------------------------------*/
void sal_add_dev_event_thread(struct sal_card *sal_card,
			      struct sal_event *sal_event)
{
	unsigned long flag = 0;

	if (NULL == sal_card->event_thread) {
		/* error handle thread is null. */
		SAL_TRACE(OSP_DBL_MAJOR, 559,
			  "Card:%d event-Q thread already stop",
			  sal_card->card_no);
		return;
	}

	spin_lock_irqsave(&sal_card->card_event_lock, flag);
	SAS_LIST_ADD_TAIL(&sal_event->list, &sal_card->event_active_list);
	spin_unlock_irqrestore(&sal_card->card_event_lock, flag);

	(void)SAS_WAKE_UP_PROCESS(sal_card->event_thread);
}

/*----------------------------------------------*
 * convert protocol defined device type into driver defined device type.     *
 *----------------------------------------------*/
enum sal_dev_type sal_get_dev_type(struct disc_device_data *disc_dev)
{
	enum sal_dev_type dev_type = SAL_DEV_TYPE_BUTT;

	if (SAS_DEVTYPE_SSP_MASK & disc_dev->tgt_proto)
		dev_type = SAL_DEV_TYPE_SSP;
	else if (SAS_DEVTYPE_SMP_MASK & disc_dev->tgt_proto)
		dev_type = SAL_DEV_TYPE_SMP;
	else if ((SAS_DEVTYPE_STP_MASK & disc_dev->tgt_proto)
		 || (SAS_DEVTYPE_SATA_MASK & disc_dev->tgt_proto))
		dev_type = SAL_DEV_TYPE_STP;
	else
		dev_type = SAL_DEV_TYPE_BUTT;

	if (SAS_DEVTYPE_SSP_MASK & disc_dev->ini_proto)
		dev_type = SAL_DEV_TYPE_SSP;

	return dev_type;
}

/*----------------------------------------------*
 * obtain device type.     *
 *----------------------------------------------*/
u8 sal_get_ini_dev_type(struct disc_device_data * disc_dev)
{

	if (disc_dev->ini_proto & SAS_DEVTYPE_SSP_MASK)
		return disc_dev->ini_proto;
	else
		return 0;
}

/*----------------------------------------------*
 * add disk interface for DISC module .     *
 *----------------------------------------------*/
s32 sal_disc_add_disk(struct disc_device_data * disc_dev)
{
	SAS_STRUCT_COMPLETION disc_cmpl;
	struct sal_card *card = NULL;
	struct sal_port *port = NULL;
	struct sal_event *event = NULL;
	struct sal_disk_info *disk_add = NULL;
	u32 result = SAL_ADD_DEV_FAILED;

	SAL_ASSERT(SAL_MAX_CARD_NUM > disc_dev->card_id, return ERROR);
	SAL_ASSERT(SAL_MAX_PHY_NUM > disc_dev->port_id, return ERROR);

	card = sal_get_card(disc_dev->card_id);
	SAL_ASSERT(NULL != card, return ERROR);

	port = sal_find_port_by_chip_port_id(card, disc_dev->port_id);
	if (NULL == port) {
		sal_put_card(card);
		SAL_TRACE(OSP_DBL_MAJOR, 563, "find port failed by port id:%d",
			  disc_dev->port_id);
		return ERROR;
	}

	event = sal_get_free_event(card);
	if (NULL == event) {
		SAL_TRACE(OSP_DBL_MAJOR, 564,
			  "Card:%d get free event node failed", card->card_no);
		sal_put_card(card);
		return ERROR;
	}

	memset(event, 0, sizeof(struct sal_event));

	disk_add = &event->info.disk_add;
	disk_add->sas_addr = disc_dev->sas_addr;
	disk_add->parent = disc_dev->father_sas_addr;
	disk_add->link_rate = disc_dev->link_rate;
	disk_add->slot_id = (u32) disc_dev->phy_id;
	disk_add->logic_id = port->logic_id;
	disk_add->card = card;
	disk_add->port = port;
	disk_add->virt_phy = SAL_FALSE;
	disk_add->status = NULL;
	if (VIRTUAL_DEVICE == disc_dev->dev_type)
		disk_add->virt_phy = SAL_TRUE;

	disk_add->type = sal_get_dev_type(disc_dev);
	if (disk_add->parent == disk_add->port->sas_addr)
		disk_add->direct_attatch = 1;
	else
		disk_add->direct_attatch = 0;

	disk_add->ini_type = sal_get_ini_dev_type(disc_dev);

	event->disc_compl = NULL;
	event->event = SAL_EVENT_ADD_DEV;
	event->time = jiffies;
	SAS_INIT_COMPLETION(&disc_cmpl);

	if (SAL_DEV_TYPE_SMP == disk_add->type) {
		event->disc_compl = &disc_cmpl;
		disk_add->status = &result;
	}

	/*add preprocess event into event process thread. */
	sal_add_dev_event_thread(card, event);
	disc_dev->reg_dev_flag = SAINI_FREE_REGDEV;

	if (SAL_DEV_TYPE_SMP == disk_add->type) {
		/* wait register end. */
		SAS_WAIT_FOR_COMPLETION(&disc_cmpl);
		if (SAL_ADD_DEV_SUCCEED != result) {
			sal_put_card(card);
			SAL_TRACE(OSP_DBL_MAJOR, 565,
				  "Card:%d port:%d add-dev event to event-Q for process,but failed",
				  disc_dev->card_id, disc_dev->port_id);
			return ERROR;
		} else {
			SAL_TRACE(OSP_DBL_DATA, 566,
				  "Card:%d port:%d disc-thread add SMP dev:0x%llx succeed",
				  disc_dev->card_id, disc_dev->port_id,
				  disc_dev->sas_addr);
		}
	} else {
		SAL_TRACE(OSP_DBL_DATA, 566,
			  "Card:%d port:%d disc-thread add disk:0x%llx(link rate:0x%x)",
			  disc_dev->card_id, disc_dev->port_id,
			  disc_dev->sas_addr, disc_dev->link_rate);
	}

	sal_put_card(card);
	return OK;

}

/*----------------------------------------------*
 * delete disk interface for DISC module .     *
 *----------------------------------------------*/
void sal_disc_del_disk(struct disc_device_data *disc_dev)
{
	struct sal_event *event = NULL;
	struct sal_card *card = NULL;
	struct sal_port *port = NULL;

	SAL_ASSERT(SAL_MAX_CARD_NUM > disc_dev->card_id, return);
	SAL_ASSERT(SAL_MAX_PHY_NUM > disc_dev->port_id, return);

	card = sal_get_card(disc_dev->card_id);
	if (NULL == card) {
		SAL_TRACE(OSP_DBL_MAJOR, 568, "get card failed by card id:%d",
			  disc_dev->card_id);
		return;
	}

	port = sal_find_port_by_chip_port_id(card, disc_dev->port_id);
	if (NULL == port) {
		sal_put_card(card);
		SAL_TRACE(OSP_DBL_MAJOR, 569, "get port failed by port id:%d",
			  disc_dev->port_id);
		return;
	}

	event = sal_get_free_event(card);
	if (NULL == event) {
		SAL_TRACE(OSP_DBL_MAJOR, 570,
			  "Card:%d get free event node failed", card->card_no);
		sal_put_card(card);
		return;
	}

	memset(event, 0, sizeof(struct sal_event));
	event->event = SAL_EVENT_DEL_DEV;
	event->time = jiffies;
	event->info.disk_del.sas_addr = disc_dev->sas_addr;
	event->info.disk_del.parent = disc_dev->father_sas_addr;
	event->info.disk_del.port = port;
	event->info.disk_del.card = card;
	sal_add_dev_event_thread(card, event);

	sal_put_card(card);
	return;
}

/*----------------------------------------------*
 *DISK DONE process.     *
 *----------------------------------------------*/
void sal_disc_done(u8 card_no, u32 port_no)
{
	struct sal_card *card = NULL;
	struct sal_port *port = NULL;
	struct sal_event *event = NULL;

	SAL_ASSERT(SAL_MAX_CARD_NUM > card_no, return);
	SAL_ASSERT(SAL_MAX_PHY_NUM > port_no, return);

	card = sal_get_card(card_no);
	if (NULL == card) {
		SAL_TRACE(OSP_DBL_MAJOR, 572, "get card failed by card id:%d",
			  card_no);
		return;
	}

	port = sal_find_port_by_chip_port_id(card, port_no);
	if (NULL == port) {
		SAL_TRACE(OSP_DBL_MAJOR, 573,
			  "can't find port by chip port id:%d", port_no);
		sal_put_card(card);
		return;
	}

	event = sal_get_free_event(card);
	if (NULL == event) {
		SAL_TRACE(OSP_DBL_MAJOR, 574,
			  "Card:%d get free event node failed", card->card_no);
		sal_put_card(card);
		return;
	}

	memset(event, 0, sizeof(struct sal_event));
	event->time = jiffies;
	event->info.port_event.port = port;
	event->event = SAL_EVENT_DISC_DONE;

	/*Disc process finish, wake up event thread to add device . */
	sal_add_dev_event_thread(card, event);
	sal_put_card(card);
	return;
}

/*----------------------------------------------*
 * if the device should be registered .     *
 *----------------------------------------------*/
bool sal_is_dev_should_be_reg(struct sal_card * sal_card,
			      struct sal_event * event)
{
	if (SAL_PORT_MODE_TGT == sal_card->config.work_mode) {
		/* if I am tgt */

		if ((event->info.disk_add.ini_type == 0)
		    && (event->info.disk_add.type == SAL_DEV_TYPE_SSP)) {
			/* if you are SSP target */
			SAL_TRACE(OSP_DBL_MINOR, 575,
				  "Card:%d(TGT) should reg dev:0x%llx(Ini attr:0x%x),"
				  "but it is SSP TGT,so don't reg",
				  sal_card->card_no,
				  event->info.disk_add.sas_addr,
				  event->info.disk_add.ini_type);
			return false;
		}
	} else if ((SAL_PORT_MODE_INI == sal_card->config.work_mode)
		   && (SAL_FALSE == event->info.disk_add.virt_phy)) {
		/* if I am ini */

		if (event->info.disk_add.ini_type & SAS_DEVTYPE_SSP_MASK) {
			/*if you are SSP initiator */
			SAL_TRACE(OSP_DBL_MINOR, 576,
				  "Card:%d(INI) should reg dev:0x%llx(Ini attr:0x%x),"
				  "but it is SSP INI,so don't reg",
				  sal_card->card_no,
				  event->info.disk_add.sas_addr,
				  event->info.disk_add.ini_type);
			return false;
		}
	}

	return true;
}

/*----------------------------------------------*
 * register SMP device finished, wake up waiter .     *
 *----------------------------------------------*/
void sal_disc_add_frame_complete(struct sal_event *event, u32 result)
{
	if ((SAL_DEV_TYPE_SMP == event->info.disk_add.type)
	    && (NULL != event->info.disk_add.status)) {
		*event->info.disk_add.status = result;
		SAS_COMPLETE(event->disc_compl);
	}

	return;
}

/*----------------------------------------------*
 *obtain free device node .     *
 *----------------------------------------------*/
struct sal_dev *sal_get_free_dev_node(struct sal_card *sal_card,
				      struct sal_port *port,
				      struct sal_disk_info *disk_info)
{
	struct sal_dev *dev = NULL;
	unsigned long flag = 0;
	u64 sas_addr = 0;
	u64 father_sas_addr = 0;
	u32 slot_id = 0;
	struct list_head *node = NULL;
	struct list_head *safe_node = NULL;

	SAL_ASSERT(NULL != sal_card, return NULL);
	SAL_ASSERT(NULL != disk_info, return NULL);

	sas_addr = disk_info->sas_addr;
	father_sas_addr = disk_info->parent;
	slot_id = disk_info->slot_id;

	/*traverse Active dev list, lookup if the device exist.*/
	spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(node, safe_node, &sal_card->active_dev_list) {
		dev = SAS_LIST_ENTRY(node, struct sal_dev, dev_list);
		if ((sas_addr == dev->sas_addr)
		    && (port->logic_id == dev->logic_id)
		    && (father_sas_addr == dev->father_sas_addr)
		    && (slot_id == dev->slot_id)
		    && (SAL_DEV_TYPE_SMP != dev->dev_type)) {

			if (SAL_DEV_ST_FLASH != sal_get_dev_state(dev)) {

				SAL_TRACE(OSP_DBL_MAJOR, 567,
					  "dev:0x%llx(ST:%d,slot:%d) is not correct,pls check!",
					  dev->sas_addr, dev->dev_status,
					  slot_id);

				spin_unlock_irqrestore(&sal_card->
						       card_dev_list_lock,
						       flag);
				return NULL;
			}

			/* disk device existed, state :flash -> init */
			(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_FIND);
			spin_unlock_irqrestore(&sal_card->card_dev_list_lock,
					       flag);
			return dev;
		}
	}
	spin_unlock_irqrestore(&sal_card->card_dev_list_lock, flag);

	dev = sal_get_free_dev(sal_card);
	if (NULL == dev) {
		SAL_TRACE(OSP_DBL_MAJOR, 567,
			  "get free dev node failed from free dev list");
		return NULL;
	}

	/*state migrate */
	if (SAL_DEV_TYPE_SMP == dev->dev_type)
		/*free -> init */
		(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_SMP_FIND);
	else
		/*free -> init */
		(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_FIND);

	return dev;
}

/*----------------------------------------------*
 * add disk process.     *
 *----------------------------------------------*/
struct sal_dev *sal_add_disk_process(struct sal_disk_info *disk_info)
{
	unsigned long flag = 0;
	struct sal_card *card = NULL;
	struct sal_port *port = NULL;
	struct sal_dev *dev = NULL;

	card = disk_info->card;
	port = disk_info->port;

	dev = sal_get_free_dev_node(card, port, disk_info);
	if (NULL == dev) {
		SAL_TRACE(OSP_DBL_MAJOR, 567,
			  "Card:%d get free dev node failed", card->card_no);
		return NULL;
	}

	spin_lock_irqsave(&dev->dev_lock, flag);
	dev->sas_addr = disk_info->sas_addr;
	dev->father_sas_addr = disk_info->parent;
	dev->link_rate_prot = disk_info->link_rate;
	dev->slot_id = disk_info->slot_id;
	dev->logic_id = port->logic_id;
	dev->card = card;
	dev->port = port;
	dev->virt_phy = disk_info->virt_phy;
	dev->dev_type = disk_info->type;
	dev->direct_attatch = disk_info->direct_attatch;
	dev->ini_type = disk_info->ini_type;
	dev->dif_ctrl.dif_switch = card->config.dif_switch;
	dev->dif_ctrl.block_size =
	    (enum sal_dif_block_size)card->config.dif_blk_size;
	dev->dif_ctrl.dif_action = (enum sal_dif_action)card->config.dif_action;
	spin_unlock_irqrestore(&dev->dev_lock, flag);

	return dev;
}

/*----------------------------------------------*
 *process add disk event  .     *
 *----------------------------------------------*/
s32 sal_handle_disk_add(struct sal_card * sal_card, struct sal_event * event)
{
	struct sal_dev *dev = NULL;
	unsigned long flag = 0;
	s32 ret = ERROR;
	unsigned long dly_time = 0;

	dly_time = sal_global.dly_time[sal_card->card_no];
	SAL_TRACE(OSP_DBL_INFO, 577,
		  "Card:%d handle add disk:0x%llx event,consume time:%dms",
		  sal_card->card_no, event->info.disk_add.sas_addr,
		  jiffies_to_msecs(jiffies - event->time));

	if ((SAL_PORT_LINK_UP != event->info.disk_add.port->status)
	    || (SAL_TRUE == sal_card->reseting)) {
		SAL_TRACE(OSP_DBL_MAJOR, 577,
			  "Card:%d port:%d is down(status:%d)/card resetting(flag:%d)",
			  sal_card->card_no, event->info.disk_add.port->port_id,
			  event->info.disk_add.port->status,
			  sal_card->reseting);

		sal_disc_add_frame_complete(event, SAL_ADD_DEV_FAILED);
		return ERROR;
	}
	if (false == sal_is_dev_should_be_reg(sal_card, event)) {
		/*the device forbid register.*/
		sal_disc_add_frame_complete(event, SAL_ADD_DEV_FAILED);
		return ERROR;
	}

	dev = sal_add_disk_process(&event->info.disk_add);
	if (NULL == dev) {
		SAL_TRACE(OSP_DBL_MAJOR, 578,
			  "Card:%d create dev struct failed",
			  sal_card->card_no);
		sal_disc_add_frame_complete(event, SAL_ADD_DEV_FAILED);
		return ERROR;
	}

	if (NULL == sal_card->ll_func.dev_op) {
		SAL_TRACE(OSP_DBL_MAJOR, 579, "Card:%d pfnAddDev func is NULL",
			  sal_card->card_no);
		sal_abnormal_put_dev(dev);
		sal_disc_add_frame_complete(event, SAL_ADD_DEV_FAILED);
		return ERROR;
	}

	SAL_TRACE(OSP_DBL_INFO, 580,
		  "Card:%d port:%d will add dev:0x%llx(ST:%d),event time:%d ms",
		  sal_card->card_no, event->info.port_event.port->port_id,
		  dev->sas_addr, dev->dev_status,
		  jiffies_to_msecs(jiffies - event->time));

	(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_REG);

	spin_lock_irqsave(&dev->dev_lock, flag);

	SAS_INIT_COMPLETION(&dev->compl);

	ret = sal_card->ll_func.dev_op(dev, SAL_DEV_OPT_REG);
	spin_unlock_irqrestore(&dev->dev_lock, flag);
	if (OK != ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 581,
			  "Card:%d port:%d Reg dev:0x%llx to FW failed",
			  sal_card->card_no,
			  event->info.port_event.port->port_id, dev->sas_addr);

		/*Notice:  SAL_DEV resource is releasing, LL DEV is responsible with LL layer. */
		(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_REG_FAILED);
		sal_abnormal_put_dev(dev);
	
		sal_disc_add_frame_complete(event, SAL_ADD_DEV_FAILED);
		return ERROR;
	}

	if (!SAS_WAIT_FOR_COMPLETION_TIME_OUT(&dev->compl,
					      msecs_to_jiffies
					      (SAL_ADD_DEV_TIMEOUT) +
					      dly_time)) {
		SAL_TRACE(OSP_DBL_MAJOR, 582,
			  "Card:%d port:%d Reg dev:0x%llx timeout",
			  sal_card->card_no,
			  event->info.port_event.port->port_id, dev->sas_addr);

		/*chip exception */
		SAL_TRACE(OSP_DBL_INFO, 616,
			  "Card:%d reg dev:0x%llx abnormal, it need to dump fw",
			  sal_card->card_no, dev->sas_addr);

		(void)sal_comm_dump_fw_info(sal_card->card_no,
					    SAL_ERROR_INFO_ALL);

		(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_REG_FAILED);


		spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
		SAS_LIST_DEL_INIT(&dev->dev_list);
		spin_unlock_irqrestore(&sal_card->card_dev_list_lock, flag);

		sal_disc_add_frame_complete(event, SAL_ADD_DEV_FAILED);


		sal_send_raw_ctrl_no_wait(sal_card->card_no,
					  (void *)(u64) sal_card->card_no,
					  NULL, sal_ctrl_reset_chip_by_card);
		return ERROR;
	}

	if (SAL_EXEC_SUCCESS == dev->compl_status) {
		/* send abort task set for SAS disk. */ 
		(void)sal_abort_task_set(sal_card, dev);

		if (SAL_DEV_TYPE_SMP == dev->dev_type)
			/* expander */
			(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_SMP_REG_OK);
		else
			(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_REG_OK);


		sal_disc_add_frame_complete(event, SAL_ADD_DEV_SUCCEED);
		 /*SES*/ if (SAL_TRUE == dev->virt_phy) {
			spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
			sal_add_disk_report(sal_card, dev);
			spin_unlock_irqrestore(&sal_card->card_dev_list_lock,
					       flag);
		}

		return OK;
	} else {
		SAL_TRACE(OSP_DBL_MAJOR, 583,
			  "Card:%d Reg dev:0x%llx failed,status:0x%x",
			  sal_card->card_no, dev->sas_addr, dev->compl_status);

		(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_REG_FAILED);

		sal_abnormal_put_dev(dev);

		sal_disc_add_frame_complete(event, SAL_ADD_DEV_FAILED);
		return ERROR;
	}

}

/*----------------------------------------------*
 *abort device IO.     *
 *----------------------------------------------*/
s32 sal_abort_dev_io(struct sal_card * sal_card, struct sal_dev * dev)
{
	unsigned long flag = 0;
	s32 ret = ERROR;
	unsigned long dly_time = 0;

	dly_time = sal_global.dly_time[sal_card->card_no];

	if (NULL == sal_card->ll_func.eh_abort_op) {
		SAL_TRACE(OSP_DBL_MAJOR, 584, "card:%d pfnAbortDevMsg is NULL",
			  sal_card->card_no);
		return ERROR;
	}

	SAL_TRACE(OSP_DBL_INFO, 585, "Card:%d is aborting disk:0x%llx io",
		  sal_card->card_no, dev->sas_addr);

	spin_lock_irqsave(&dev->dev_lock, flag);
	SAS_INIT_COMPLETION(&dev->compl);
	ret = sal_card->ll_func.eh_abort_op(SAL_EH_ABORT_OPT_DEV, dev);
	spin_unlock_irqrestore(&dev->dev_lock, flag);
	if (OK != ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 586,
			  "Card:%d abort disk:0x%llx io failed",
			  sal_card->card_no, dev->sas_addr);
		return ERROR;
	}

	if (!SAS_WAIT_FOR_COMPLETION_TIME_OUT(&dev->compl,
					      msecs_to_jiffies
					      (SAL_ABORT_DEV_IO_TIMEOUT) +
					      dly_time)) {
		SAL_TRACE(OSP_DBL_MAJOR, 587,
			  "Card:%d abort disk:0x%llx io timeout,need reset chip.",
			  sal_card->card_no, dev->sas_addr);

		(void)sal_add_card_to_err_handler(sal_card,
						  SAL_ERR_HANDLE_ACTION_RESET_HOST);

		return ERROR;
	}

	if (SAL_EXEC_SUCCESS != dev->compl_status) {
		SAL_TRACE(OSP_DBL_MAJOR, 588,
			  "Card:%d abort disk:0x%llx io failed,status:0x%x",
			  sal_card->card_no, dev->sas_addr, dev->compl_status);
		return ERROR;
	}

	return OK;
}

/*----------------------------------------------*
 * notify low-layer to delete device .     *
 *----------------------------------------------*/
enum sal_exec_cmpl_status sal_notify_ll_to_del_dev(struct sal_card *sal_card,
						   struct sal_dev *dev)
{
	unsigned long flag = 0;
	unsigned long dly_time = 0;
	s32 ret = ERROR;

	dly_time = sal_global.dly_time[sal_card->card_no];

	if (NULL == sal_card->ll_func.dev_op) {
		SAL_TRACE(OSP_DBL_MAJOR, 589, "Card:%d Dev Opt Func is NULL",
			  sal_card->card_no);
		return SAL_EXEC_FAILED;
	}

	spin_lock_irqsave(&dev->dev_lock, flag);
	SAS_INIT_COMPLETION(&dev->compl);
	ret = sal_card->ll_func.dev_op(dev, SAL_DEV_OPT_DEREG);
	spin_unlock_irqrestore(&dev->dev_lock, flag);
	if (OK != ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 589, "Deregister dev to FW failed");
		return SAL_EXEC_FAILED;
	}

	if (!SAS_WAIT_FOR_COMPLETION_TIME_OUT(&dev->compl,
					      msecs_to_jiffies
					      (SAL_DEL_DEV_TIMEOUT) +
					      dly_time)) {
		SAL_TRACE(OSP_DBL_MINOR, 590,
			  "Err! Card:%d Deregister disk:0x%llx timeout",
			  sal_card->card_no, dev->sas_addr);
		return SAL_EXEC_FAILED;
	}

	if (SAL_EXEC_RETRY == dev->compl_status) {
		SAL_TRACE(OSP_DBL_MAJOR, 591,
			  "Card:%d Deregister disk:0x%llx need to retry",
			  sal_card->card_no, dev->sas_addr);
		return SAL_EXEC_RETRY;
	} else if (SAL_EXEC_FAILED == dev->compl_status) {
		/*logout device fail */
		SAL_TRACE(OSP_DBL_MAJOR, 591,
			  "Card:%d Deregister disk:0x%llx failed",
			  sal_card->card_no, dev->sas_addr);
		return SAL_EXEC_FAILED;
	} else {
		return SAL_EXEC_SUCCESS;
	}

}

/*----------------------------------------------*
 * add device callback function.     *
 *----------------------------------------------*/
void sal_add_dev_cb(struct sal_dev *dev, u32 v_uiStatus)
{
	SAL_ASSERT(NULL != dev, return);

	if (SAL_STATUS_SUCCESS == v_uiStatus) {
		/*if register success , increase port disk count.*/
		SAS_ATOMIC_INC(&dev->port->dev_num);
		dev->compl_status = SAL_EXEC_SUCCESS;
	} else {
		dev->compl_status = SAL_EXEC_FAILED;
	}
	SAS_COMPLETE(&dev->compl);
	return;
}

EXPORT_SYMBOL(sal_add_dev_cb);

/*----------------------------------------------*
 * delete device callback function.     *
 *----------------------------------------------*/
void sal_del_dev_cb(struct sal_dev *dev, enum sal_status status)
{
	SAL_ASSERT(NULL != dev, return);

	if (SAL_STATUS_SUCCESS == status) {
		/*if register success , increase port disk count.*/
		SAS_ATOMIC_DEC(&dev->port->dev_num);
		dev->compl_status = SAL_EXEC_SUCCESS;

		/*state:(in dereg -> flash) */
		(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_DEREG_OK);
	} else if (SAL_STATUS_RETRY == status) {

		dev->compl_status = SAL_EXEC_RETRY;

	} else {
		dev->compl_status = SAL_EXEC_FAILED;

		/*state:(in dereg -> flash) */
		(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_DEREG_FAILED);
	}
	SAS_COMPLETE(&dev->compl);
	return;
}

EXPORT_SYMBOL(sal_del_dev_cb);

void sal_del_scsi_dev(u32 host_id,
			   u32 channel_id, u32 scsi_id, u32 lun_id)
{
	struct Scsi_Host *host;
	struct scsi_device *dev;

	host = (struct Scsi_Host *)scsi_host_lookup((u16) host_id);
	if (IS_ERR(host)) {
		SAL_TRACE(OSP_DBL_MAJOR, 58, "SCSI host [%d:%d:%d:%d] no exist",
			  host_id, channel_id, scsi_id, lun_id);
		return;
	}

	dev = scsi_device_lookup(host, channel_id, scsi_id, lun_id);

	if (dev) {
		scsi_remove_device(dev);
		scsi_device_put(dev);
		dev = NULL;
		SAL_TRACE(OSP_DBL_INFO, 59,
			  "scsi remove device [%d:%d:%d:%d] ok", host_id,
			  channel_id, scsi_id, lun_id);
	} else {
		SAL_TRACE(OSP_DBL_MAJOR, 60,
			  "scsi device [%d:%d:%d:%d] not found", host_id,
			  channel_id, scsi_id, lun_id);
	}

	scsi_host_put(host);
}

u32 sal_get_host_state(u32 host_id)
{
	struct Scsi_Host *host = NULL;
	u32 host_busy = 0;

	if (SASINI_INVALID_HOSTID == host_id)
		return SHOST_DEL;

	host = scsi_host_lookup((u16) host_id);
	if (NULL == host) {
		SAL_TRACE(OSP_DBL_MAJOR, 18, "Host %d is not exist", host_id);
		return SHOST_DEL;
	}

	host_busy = host->shost_state;
	scsi_host_put(host);

	return host_busy;
}

void sal_add_scsi_dev(u32 host_id, u32 channel_id, u32 scsi_id, u32 lun_id)
{
	struct Scsi_Host *host = NULL;
	struct scsi_device *scsi_device = NULL;
	u32 tmp_host_id = host_id;
	u32 tmp_scsi_id = scsi_id;
	u32 tmp_chan_id = channel_id;
	u32 tmp_lun_id = lun_id;
	s32 ret = ERROR;

	/*
	 *lookup HOST according to host id.
	 */
	host = (struct Scsi_Host *)scsi_host_lookup((u16) tmp_host_id);
	if (IS_ERR(host)) {
		SAL_TRACE(OSP_DBL_MAJOR, 53, "SCSI host %d:%d:%d:%d no exist",
			  host_id, channel_id, scsi_id, lun_id);
		return;
	}

	scsi_host_put(host);
	SAL_TRACE(OSP_DBL_INFO, 54, "scsi_add_device:hostname=%s"
		  " %d:%d:%d:%d", host->hostt->name,
		  host->host_no, tmp_chan_id, tmp_scsi_id, tmp_lun_id);
	scsi_device =
	    scsi_device_lookup(host, tmp_chan_id, tmp_scsi_id, tmp_lun_id);

	/*
	 *look up corresponding HOST, if there is DEVICE corresponding to the  SCSI ID 
	 */
	if (NULL == scsi_device) {
		ret =
		    scsi_add_device(host, tmp_chan_id, tmp_scsi_id, tmp_lun_id);
		if (0x00 != ret) {
			scsi_device = NULL;
			SAL_TRACE(OSP_DBL_MAJOR, 55,
				  "scsi_add_device %d failed, return %d",
				  tmp_scsi_id, ret);
			return;
		} else {
			SAL_TRACE(OSP_DBL_INFO, 56, "scsi_add_device success");
		}
	} else {
		scsi_device_put(scsi_device);
		SAL_TRACE(OSP_DBL_MAJOR, 57, "scsi_add_device exist");
	}
}

/*----------------------------------------------*
 * send delay report module .     *
 *----------------------------------------------*/
void sal_report_chg_delay_intf(struct sal_dev *device, u8 event_type)
{
#if 0 /* remove chg dly */    
	struct sas_ini_device_st_info dev_info;	/*是否考虑使用malloc 方式?? */
	u64 sas_addr = 0;
	u64 parent_sas_addr = 0;
	u64 mask = 0;
	char *desc_str[] = {
		"Disk in",
		"Disk out",
		"Frame in",
		"Frame out",
		"Unknown"
	};

	SAL_REF(desc_str[0]);	//lint
#endif

	if (SAL_INVALID_SCSIID == device->scsi_id) {
		SAL_TRACE(OSP_DBL_MAJOR, 622,
			  "Card:%d dev:0x%llx(ST:%d,sal dev:%p) scsi id invalid",
			  device->card->card_no, device->sas_addr,
			  device->dev_status, device);
		return;
	}
    
#if  0/* remove chg dly */
	memset(&dev_info, 0, sizeof(struct sas_ini_device_st_info));
	dev_info.card_no = device->card->card_no;
	dev_info.disk_info.chan_id = device->bus_id;
	dev_info.disk_info.dev_type = sal_get_chg_delay_dev_type(device);
	dev_info.disk_info.host_id = (u32) device->card->scsi_host.InitiatorNo;
	dev_info.disk_info.lun_id = device->lun_id;

	/*
	 *该PORTID是缓上报模块内部使用的 PORT ID
	 *缓上报处理需要以实际的SAS域作为缓上报的标准
	 *因此需要使用LogicID的后八位
	 *(不能使用chip port id，该值在复位芯片前后会发生变化)
	 */
	dev_info.disk_info.port_id = device->port->logic_id & 0xff;

	/*
	 *该ID是上报设备管理使用的ID，使用DRV API里标准的定义
	 */
	dev_info.disk_info.logic_id = device->port->logic_id;
	dev_info.disk_info.scsi_id = device->scsi_id;
	dev_info.disk_info.slot_id = device->slot_id;
	dev_info.disk_info.sas_addr = device->virt_sas_addr;
	dev_info.disk_info.parent_sas_addr = device->virt_father_addr;
	dev_info.disk_info.disk_fault = 1;

	sas_addr = device->virt_sas_addr;
	parent_sas_addr = device->virt_father_addr;
	mask = SAL_GET_WWN_MASK(dev_info.disk_info.dev_type);
	if (SASINI_HBA_SASADDR != dev_info.disk_info.parent_sas_addr)
		/*addby weixu 90005543 DTS2013041805307 V2的框地址可能是7F结尾 */
		dev_info.disk_info.report_parent_wwn =
		    SAL_GET_PARENT_WWN(parent_sas_addr, mask);
	/*end by weixu DTS2013041805307 */
	else
		dev_info.disk_info.report_parent_wwn = SASINI_HBA_SASADDR;

	/* 使用限制:磁盘框磁盘最大个数必须 <= 63 个
	   *(当前磁盘框最大磁盘个数为25),否则最后一个槽位的磁盘wwn和框的wwn会相同(3f)
	 */
	if (SASINI_DEVICE_SES == dev_info.disk_info.dev_type)
		/*SES*/
		    dev_info.disk_info.report_wwn =
		    (sas_addr & mask) | device->slot_id;
	else
		/*disk */
		dev_info.disk_info.report_wwn =
		    (parent_sas_addr & mask) | device->slot_id;

	dev_info.disk_info.event = sal_get_chg_dly_event_type(event_type,
							      dev_info.
							      disk_info.
							      dev_type);

	sasini_disk_chg_delay_event(&dev_info);
#else
	//enum sas_ini_dev_type dev_type;
	//enum sas_ini_event event;
	u32 host_id;
    u32 chan_id;
    u32 scsi_id;
    u32 lun_id;
    u8  card_no;
    u32 host_busy = 0;

	//dev_type = sal_get_chg_delay_dev_type(device);
    //event = sal_get_chg_dly_event_type(event_type, dev_type);

    host_id = (u32)device->card->host_id;
    chan_id = device->bus_id;
    scsi_id = device->scsi_id;
    lun_id = device->lun_id;
    card_no = device->card->card_no;

	printk("******** DEV REPORT: event_type=%d(0:IN, 1:OUT), (%d:%d:%d:%d) *********\n", 
        event_type, host_id, chan_id, scsi_id, lun_id);
    
    if (event_type == SASINI_EVENT_DEV_OUT) {
        /* out */
        sal_free_dev_rsc_without_lock(card_no, chan_id, scsi_id, lun_id);
        
		sal_del_scsi_dev(host_id, chan_id, scsi_id, lun_id);
    } else {
        host_busy = sal_get_host_state(host_id);
		if (SHOST_RUNNING == host_busy) {
            /* in */
            sal_add_scsi_dev(host_id, 0, scsi_id, 0);

			sal_notify_report_dev_in_without_lock(card_no, scsi_id, chan_id, lun_id);
            
        }
    }
	
#endif
}

/*----------------------------------------------*
 * delete disk report.     *
 *----------------------------------------------*/
void sal_del_disk_report(struct sal_card *sal_card, struct sal_dev *dev)
{
	unsigned long flag = 0;

	SAL_REF(sal_card);


	if ((SAL_DEV_TYPE_SMP == dev->dev_type)
	    && (SAL_DEV_ST_FLASH == sal_get_dev_state(dev))) {
		SAL_TRACE(OSP_DBL_INFO, 624,
			  "Card:%d port:%d smp dev:0x%llx out, ref count:%d",
			  sal_card->card_no, dev->port->port_id, dev->sas_addr,
			  SAS_ATOMIC_READ(&dev->ref));

		/*delete device from active list , then put in free list.*/
		sal_dev_check_put_last(dev, true);	/*called in devlist lock */
		return;
	} else if (SAL_DEV_ST_FLASH == sal_get_dev_state(dev)) {
		spin_lock_irqsave(&dev->dev_lock, flag);
		sal_report_chg_delay_intf(dev, SASINI_EVENT_DEV_OUT);
		spin_unlock_irqrestore(&dev->dev_lock, flag);
	} else {
		/* device is in wrong state. */
		SAL_TRACE(OSP_DBL_MAJOR, 625,
			  "Card:%d disk:0x%llx abnormal status:%d",
			  sal_card->card_no, dev->sas_addr, dev->dev_status);
	}

	return;
}

/*----------------------------------------------*
 * free smp dev.     *
 *----------------------------------------------*/
void sal_free_smp_dev(struct sal_dev *dev)
{
	if ((SAL_DEV_TYPE_SMP == dev->dev_type)
	    && (SAL_DEV_ST_FLASH == sal_get_dev_state(dev))) {
		SAL_TRACE(OSP_DBL_INFO, 624,
			  "Card:%d disk:0x%llx out, ref count:%d",
			  dev->card->card_no, dev->sas_addr,
			  SAS_ATOMIC_READ(&dev->ref));

		/*delete device from active list , then put in free list.*/
		sal_put_dev(dev);
	}

	return;
}

/*----------------------------------------------*
 * clear error handle notify by dev.     *
 *----------------------------------------------*/
void sal_clr_err_handler_notify_by_dev(struct sal_card *sal_card,
				       struct sal_dev *sal_dev, bool mutex_flag)
{
	unsigned long flag = 0;
	struct list_head *node = NULL;
	struct list_head *safe_node = NULL;
	struct sal_ll_notify *notify = NULL;
	struct sal_dev *attach_dev = NULL;
	struct sal_msg *sal_msg = NULL;

	if (true == mutex_flag) {
		if (OK != sal_enter_err_hand_mutex_area(sal_card))
			return;
	}

	spin_lock_irqsave(&sal_card->ll_notify_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(node, safe_node,
			       &sal_card->ll_notify_process_list) {
		notify =
		    SAS_LIST_ENTRY(node, struct sal_ll_notify, notify_list);
		if ((SAL_ERR_HANDLE_ACTION_RESENDMSG != notify->op_code)
		    && (SAL_ERR_HANDLE_ACTION_RESET_HOST != notify->op_code)) {

			if ((SAL_ERR_HANDLE_ACTION_RESET_DEV ==
			     notify->op_code)) {
				/*dev (hardreset ; linkreset) */
				attach_dev = (struct sal_dev *)notify->info;
				if ((attach_dev->father_sas_addr ==
				     sal_dev->sas_addr)
				    || (attach_dev == sal_dev))
					notify->process_flag = false;
			} else {
				/*msg (hardreset & retry;  abort & retry; LL retry) */
				sal_msg = (struct sal_msg *)notify->info;
				if (sal_msg->dev == sal_dev)
					notify->process_flag = false;
			}
		}
	}
	spin_unlock_irqrestore(&sal_card->ll_notify_lock, flag);

	if (true == mutex_flag)
		sal_leave_err_hand_mutex_area(sal_card);
	return;
}

EXPORT_SYMBOL(sal_clr_err_handler_notify_by_dev);

/*----------------------------------------------*
 * delete disk .     *
 *----------------------------------------------*/
s32 sal_handle_disk_del(struct sal_card * sal_card, struct sal_event * event)
{
	struct sal_dev *dev = NULL;
	struct sal_port *port = NULL;
	u64 sas_addr = 0;
	u32 i = 0;
	u32 ret = SAL_EXEC_FAILED;

	sas_addr = event->info.disk_del.sas_addr;
	port = event->info.disk_del.port;

	if (SAL_TRUE == sal_card->reseting) {
		SAL_TRACE(OSP_DBL_MINOR, 592,
			  "Card:%d port 0%x del dev 0x%llx in reset process",
			  sal_card->card_no, port->logic_id, sas_addr);
		return ERROR;
	}

	dev = sal_get_dev_by_sas_addr(port, sas_addr);
	if (NULL == dev) {
		SAL_TRACE(OSP_DBL_MAJOR, 592, "Card:%d disk:0x%llx not exist",
			  sal_card->card_no, sas_addr);
		return ERROR;
	}

	SAL_TRACE(OSP_DBL_INFO, 593,
		  "Card:%d is going to del dev addr:0x%llx(sal dev:%p),event time:%dms",
		  sal_card->card_no, dev->sas_addr, dev,
		  jiffies_to_msecs(jiffies - event->time));

	if (SAL_DEV_ST_ACTIVE != sal_get_dev_state(dev)) {
		/* device state exception  */
		SAL_TRACE(OSP_DBL_MAJOR, 593,
			  "Dev:0x%llx(sal dev:%p) ST:%d is abnormal",
			  dev->sas_addr, dev, dev->dev_status);
		sal_put_dev(dev);
		return ERROR;
	}

	SAL_TRACE(OSP_DBL_INFO, 593,
		  "Set dev:0x%llx(sal dev:%p) to miss (org ST:%d)",
		  dev->sas_addr, dev, dev->dev_status);
	/*migrate state: active -> in remove */
	(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_MISS);


	sal_clr_err_handler_notify_by_dev(sal_card, dev, true);

	for (i = 0; i < SAL_MAX_DELDISK_NUM; i++) {
		ret = sal_clear_ll_dev(sal_card, dev);
		if (SAL_EXEC_RETRY == ret) {
			SAL_TRACE(OSP_DBL_MAJOR, 597,
				  "Card:%d deregister dev:0x%llx(sal dev:%p) need to retry",
				  sal_card->card_no, dev->sas_addr, dev);
			continue;

		} else if (SAL_EXEC_FAILED == ret) {
			sal_put_dev(dev);
			return ERROR;
		} else {
			break;
		}
	}

	sal_free_smp_dev(dev);	/*if it is SMP device , then release.*/
	sal_put_dev(dev);
	return OK;

}


enum sal_exec_cmpl_status sal_clear_ll_dev(struct sal_card *sal_card,
					   struct sal_dev *dev)
{
	enum sal_exec_cmpl_status ret = SAL_EXEC_FAILED;

	if (OK != sal_abort_dev_io(sal_card, dev)) {
		SAL_TRACE(OSP_DBL_MAJOR, 596,
			  "card:%d abort dev:0x%llx(sal dev:%p) io failed",
			  sal_card->card_no, dev->sas_addr, dev);

		SAL_TRACE(OSP_DBL_INFO, 616,
			  "Card:%d abort dev:0x%llx IO abnormal, it need to dump fw",
			  sal_card->card_no, dev->sas_addr);

		(void)sal_comm_dump_fw_info(sal_card->card_no,
					    SAL_ERROR_INFO_ALL);

		return SAL_EXEC_FAILED;
	}

	/*migrate state: in remove -> in dereg */
	(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_DEREG);

	ret = sal_notify_ll_to_del_dev(sal_card, dev);
	if (SAL_EXEC_FAILED == ret) {
		/*logout device fail*/
		SAL_TRACE(OSP_DBL_MAJOR, 597,
			  "Card:%d deregister dev:0x%llx(sal dev:%p) failed",
			  sal_card->card_no, dev->sas_addr, dev);

		SAL_TRACE(OSP_DBL_INFO, 616,
			  "Card:%d dereg dev:0x%llx abnormal, it need to dump fw",
			  sal_card->card_no, dev->sas_addr);

		(void)sal_comm_dump_fw_info(sal_card->card_no,
					    SAL_ERROR_INFO_ALL);
		return SAL_EXEC_FAILED;
	} else {
		return ret;
	}
}

/*----------------------------------------------*
 * sal_set_port_dev_state print restrict.     *
 *----------------------------------------------*/
void sal_set_port_dev_state_print_ctrl(struct sal_card *sal_card,
				       struct sal_dev *dev,
				       enum sal_dev_event event)
{

	unsigned long timeout = 0;
	struct sal_port *sal_port = NULL;
	char *str[] = {
		"Free",
		"Init",
		"In Reg",
		"Reg OK",
		"Active",
		"remove",
		"In Dereg",
		"Flash",
		"rdy free",
		"unknown",
	};

	SAL_REF(str[0]);
	SAL_REF(event);
	SAL_REF(sal_card);

	sal_port = dev->port;
	timeout = sal_port->set_dev_state.last_print_time
	    + msecs_to_jiffies(1000);

	if (time_after(jiffies, timeout)) {
		sal_port->set_dev_state.last_print_time = jiffies;
		sal_port->set_dev_state.print_cnt = 0;
	}

	if (sal_port->set_dev_state.print_cnt < 5) {
		SAL_TRACE(OSP_DBL_INFO, 598,
			  "Card:%d set dev addr:0x%llx(orig state:%d-%s) new state by event:%d",
			  sal_card->card_no, dev->sas_addr, dev->dev_status,
			  str[dev->dev_status], event);

		sal_port->set_dev_state.print_cnt++;
	}

}

/*----------------------------------------------*
 * set device state of port.     *
 *----------------------------------------------*/
s32 sal_set_port_dev_state(struct sal_card *sal_card, struct sal_port *port,
			   enum sal_dev_event event)
{
	struct list_head *node = NULL;
	struct list_head *safe_node = NULL;
	struct sal_dev *dev = NULL;
	unsigned long flag = 0;
	u32 dev_cnt = 0; /*record the number of dev */ 
	char *str[] = {
		"Free",
		"Init",
		"In Reg",
		"Reg OK",
		"Active",
		"remove",
		"In Dereg",
		"Flash",
		"rdy free",
		"unknown",
	};

	SAL_REF(str[0]);
	SAL_REF(dev_cnt);

	spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(node, safe_node, &sal_card->active_dev_list) {
		dev = SAS_LIST_ENTRY(node, struct sal_dev, dev_list);
		if (dev->port == port) {
			/*look up DEVICE of the port.*/
			if ((SAL_DEV_ST_FLASH == sal_get_dev_state(dev))
			    && (SAL_DEV_EVENT_MISS == event)) {
				SAL_TRACE(OSP_DBL_INFO, 598,
					  "Card:%d port:%d dev addr:0x%llx(current state:%d-%s) flash",
					  sal_card->card_no, dev->port->port_id,
					  dev->sas_addr, dev->dev_status,
					  str[dev->dev_status]);
				continue;
			}
#if 0
			SAL_TRACE(OSP_DBL_INFO, 598,
				  "Card:%d set dev addr:0x%llx(orig state:%d-%s) new state by event:%d",
				  sal_card->card_no, dev->sas_addr,
				  dev->dev_status, str[dev->dev_status], event);
#else
			/*print restrict modify*/
			sal_set_port_dev_state_print_ctrl(sal_card, dev, event);
#endif
			dev_cnt++;

			(void)sal_dev_state_chg(dev, event);
		}
	}
	spin_unlock_irqrestore(&sal_card->card_dev_list_lock, flag);
	SAL_TRACE(OSP_DBL_INFO, 598,
		  "Card:%d Logic Port:0x%x(PortId:%d) fresh dev(num:%d) to new state by event:%d",
		  sal_card->card_no, port->logic_id, port->port_id, dev_cnt,
		  event);

	return OK;
}

/*----------------------------------------------*
 * callback function of ABORT DEVICE .     *
 *----------------------------------------------*/
void sal_abort_dev_io_cb(struct sal_dev *dev, enum sal_status v_enStatus)
{
	SAL_ASSERT(NULL != dev, return);
	SAL_ASSERT(NULL != dev->port, return);

	if (SAL_STATUS_SUCCESS == v_enStatus) {
		SAL_TRACE(OSP_DBL_INFO, 643,
			  "card:%d abort dev:0x%llx IO Rsp OK,status:0x%x",
			  dev->card->card_no, dev->sas_addr, v_enStatus);
		dev->compl_status = SAL_EXEC_SUCCESS;

	} else {
		SAL_TRACE(OSP_DBL_MAJOR, 643,
			  "card:%d abort dev:0x%llx IO failed,status:0x%x",
			  dev->card->card_no, dev->sas_addr, v_enStatus);

		if (SAL_STATUS_RETRY == v_enStatus)
			dev->compl_status = SAL_EXEC_RETRY;
		else
			dev->compl_status = SAL_EXEC_FAILED;

	}

	SAS_ATOMIC_DEC(&dev->port->pend_abort);	/* delete port */
	SAS_ATOMIC_DEC(&dev->card->pend_abort);	/* delete device */
	SAS_COMPLETE(&dev->compl);
	return;
}

EXPORT_SYMBOL(sal_abort_dev_io_cb);

/*----------------------------------------------*
 * all device IO of abort port .     *
 *----------------------------------------------*/
s32 sal_abort_port_dev_io(struct sal_card * sal_card, struct sal_port * port)
{
	struct list_head *node = NULL;
	struct list_head *safe_node = NULL;
	struct sal_dev *dev = NULL;
	unsigned long flag = 0;
	s32 wait_time = SAL_ABORT_DEV_IO_TIMEOUT;
	s32 ret = ERROR;

	if (NULL == sal_card->ll_func.eh_abort_op) {
		SAL_TRACE(OSP_DBL_MAJOR, 599,
			  "Card %d abort disk IO function is null",
			  sal_card->card_no);
		return ERROR;
	}

	SAS_ATOMIC_SET(&port->pend_abort, 0);

	spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(node, safe_node, &sal_card->active_dev_list) {
		dev = SAS_LIST_ENTRY(node, struct sal_dev, dev_list);
		if (dev->port == port) {
			/*look up device of the port.  */

			spin_lock_irqsave(&dev->dev_state_lock, flag);
			if (SAL_DEV_ST_FLASH == dev->dev_status) {
				spin_unlock_irqrestore(&dev->dev_state_lock,
						       flag);
				continue;
			}
			spin_unlock_irqrestore(&dev->dev_state_lock, flag);

			SAL_TRACE(OSP_DBL_INFO, 601,
				  "Card:%d abort port:%d disk:0x%llx IO",
				  sal_card->card_no, port->port_id,
				  dev->sas_addr);

			SAS_ATOMIC_INC(&port->pend_abort);

			ret =
			    sal_card->ll_func.eh_abort_op(SAL_EH_ABORT_OPT_DEV,
							  (void *)dev);
			if (SAL_CMND_EXEC_SUCCESS != ret) {
				SAS_ATOMIC_DEC(&port->pend_abort);
				SAL_TRACE(OSP_DBL_MAJOR, 602,
					  "Card:%d abort disk:0x%llx IO failed,continue",
					  sal_card->card_no, dev->sas_addr);
			}

		}
	}
	spin_unlock_irqrestore(&sal_card->card_dev_list_lock, flag);

	while (wait_time >= 0) {
		if (0 == SAS_ATOMIC_READ(&port->pend_abort))
			return OK;

		SAS_MSLEEP(1);
		wait_time--;
	}

	SAL_TRACE(OSP_DBL_MAJOR, 603,
		  "Card:%d Port:0x%x(FW PortId:%d) abort all dev io time out,"
		  "pending abort cmd rsp:%d", sal_card->card_no, port->logic_id,
		  port->port_id, SAS_ATOMIC_READ(&port->pend_abort));
	return ERROR;
}

/*----------------------------------------------*
 * release the timeout IO .     *
 *----------------------------------------------*/
void sal_clr_tmo_io_by_dev(struct sal_card *sal_card, struct sal_dev *sal_dev)
{
	unsigned long flag = 0;
	struct list_head *node = NULL;
	struct list_head *safe_node = NULL;
	struct sal_msg *sal_msg = NULL;
	struct list_head local_tmo_list;
	u32 i = 0;
	s32 enter_result = ERROR;


	enter_result = sal_enter_err_hand_mutex_area(sal_card);
	if (OK != enter_result) {
		SAL_TRACE(OSP_DBL_MAJOR, 71, "Card:%d enter Mutex Area failed!",
			  sal_card->card_no);
		return;
	}

	SAS_INIT_LIST_HEAD(&local_tmo_list);

	for (i = 0; i < SAL_MAX_MSG_LIST_NUM; i++) {
		spin_lock_irqsave(&sal_card->card_msg_list_lock[i], flag);
		SAS_LIST_FOR_EACH_SAFE(node, safe_node,
				       &sal_card->active_msg_list[i]) {
			sal_msg = list_entry(node, struct sal_msg, host_list);
			if ((sal_dev == sal_msg->dev)
			    && (SAL_MSG_ST_TMO == sal_msg->msg_state)) {
				SAS_LIST_DEL_INIT(&sal_msg->host_list);
				SAS_LIST_ADD_TAIL(&sal_msg->host_list,
						  &local_tmo_list);
			}
		}
		spin_unlock_irqrestore(&sal_card->card_msg_list_lock[i], flag);
	}

	/* traverse local tmo list */
	SAS_LIST_FOR_EACH_SAFE(node, safe_node, &local_tmo_list) {
		sal_msg = list_entry(node, struct sal_msg, host_list);
		SAS_LIST_DEL_INIT(&sal_msg->host_list);

		SAL_TRACE(OSP_DBL_MAJOR, 631,
			  "Card:%d dev:0x%llx(sal dev:%p ST:%d) free msg:%p(unique id:0x%llx) src",
			  sal_card->card_no, sal_dev->sas_addr, sal_dev,
			  sal_dev->dev_status, sal_msg,
			  sal_msg->ini_cmnd.unique_id);

		/* release resource*/
		(void)sal_msg_state_chg(sal_msg, SAL_MSG_EVENT_DONE_UNUSE);
	}

	sal_leave_err_hand_mutex_area(sal_card);
	return;
}

/*----------------------------------------------*
 * return the IO which have sent to disk and not returned to mid layer .     *
 *----------------------------------------------*/
void sal_clr_active_io_by_dev(struct sal_card *sal_card,
			      struct sal_dev *sal_dev)
{
	unsigned long flag = 0;
	struct list_head *node = NULL;
	struct list_head *safe_node = NULL;
	struct sal_msg *sal_msg = NULL;
	struct list_head active_list;
	u32 i = 0;
	s32 mutex_result = ERROR;

	SAS_INIT_LIST_HEAD(&active_list);

	mutex_result = sal_enter_err_hand_mutex_area(sal_card);
	if (OK != mutex_result) {
		SAL_TRACE(OSP_DBL_MAJOR, 71, "Card:%d enter Mutex Area failed!",
			  sal_card->card_no);
		return;
	}

	for (i = 0; i < SAL_MAX_MSG_LIST_NUM; i++) {
		spin_lock_irqsave(&sal_card->card_msg_list_lock[i], flag);
		SAS_LIST_FOR_EACH_SAFE(node, safe_node,
				       &sal_card->active_msg_list[i]) {
			sal_msg = list_entry(node, struct sal_msg, host_list);
			if ((sal_dev == sal_msg->dev)
			    && (SAL_MSG_ST_SEND == sal_msg->msg_state)
			    && (NULL != sal_msg->ini_cmnd.done)) {
				SAS_LIST_DEL_INIT(&sal_msg->host_list);
				SAS_LIST_ADD_TAIL(&sal_msg->host_list,
						  &active_list);
			}
		}
		spin_unlock_irqrestore(&sal_card->card_msg_list_lock[i], flag);
	}

		/* traverse local active list */
	      SAS_LIST_FOR_EACH_SAFE(node, safe_node, &active_list) {
		sal_msg = list_entry(node, struct sal_msg, host_list);
		SAS_LIST_DEL_INIT(&sal_msg->host_list);
		SAL_TRACE(OSP_DBL_MAJOR, 631,
			  "Card:%d dev:0x%llx(sal dev:%p ST:%d) return msg:%p(unique id:0x%llx) to mid layer",
			  sal_card->card_no, sal_dev->sas_addr, sal_dev,
			  sal_dev->dev_status, sal_msg,
			  sal_msg->ini_cmnd.unique_id);
		/* return IO error. */
		sal_msg->status.drv_resp = SAL_MSG_DRV_FAILED;
		sal_msg->done(sal_msg);
	}

	sal_leave_err_hand_mutex_area(sal_card);
	return;
}

/*----------------------------------------------*
 * add dev reference count after report disk in event .    *
 *----------------------------------------------*/
void sal_notify_report_dev_in(u8 card_no, u32 scsi_id, u32 chan_id, u32 lun_id)
{
	struct sal_card *card = NULL;
	unsigned long card_flag = 0;
	struct list_head *node = NULL;
	struct sal_dev *dev = NULL;

	SAL_REF(chan_id);
	SAL_REF(lun_id);
	SAL_ASSERT(card_no < SAL_MAX_CARD_NUM, return);

	card = sal_get_card(card_no);
	if (NULL == card) {
		SAL_TRACE(OSP_DBL_MAJOR, 631, "can't find card by cardid:%d",
			  card_no);
		return;
	}
	/* when report disk in event , the device must exist. */
	spin_lock_irqsave(&card->card_dev_list_lock, card_flag);
	SAS_LIST_FOR_EACH(node, &card->active_dev_list) {
		dev = SAS_LIST_ENTRY(node, struct sal_dev, dev_list);
		if (scsi_id != dev->scsi_id)
			continue;
		sal_get_dev(dev);
		dev->cnt_report_in++;
		dev = NULL;
		break;
	}
	spin_unlock_irqrestore(&card->card_dev_list_lock, card_flag);

	sal_put_card(card);
	return;
}

void sal_notify_report_dev_in_without_lock(u8 card_no, u32 scsi_id, u32 chan_id, u32 lun_id)
{
	struct sal_card *card = NULL;
	struct list_head *node = NULL;
	struct sal_dev *dev = NULL;

	SAL_REF(chan_id);
	SAL_REF(lun_id);
	SAL_ASSERT(card_no < SAL_MAX_CARD_NUM, return);

	card = sal_get_card(card_no);
	if (NULL == card) {
		SAL_TRACE(OSP_DBL_MAJOR, 631, "can't find card by cardid:%d",
			  card_no);
		return;
	}
	/* when report disk in event , the device must exist. */
	SAS_LIST_FOR_EACH(node, &card->active_dev_list) {
		dev = SAS_LIST_ENTRY(node, struct sal_dev, dev_list);
		if (scsi_id != dev->scsi_id)
			continue;
		sal_get_dev(dev);
		dev->cnt_report_in++;
		dev = NULL;
		break;
	}

	sal_put_card(card);
	return;
}

/*----------------------------------------------*
 * release dev node.    *
 *----------------------------------------------*/
void sal_free_dev_rsc_without_lock(u8 card_no, u32 scsi_id, u32 chan_id, u32 lun_id)
{
	struct sal_card *card = NULL;
	unsigned long flag = 0;
	unsigned long dev_flag = 0;
	struct list_head *node = NULL;
	struct list_head *safe_node = NULL;
	struct sal_dev *dev = NULL;
	bool real_out = false;

	SAL_REF(chan_id);
	SAL_REF(lun_id);
	SAL_ASSERT(SAL_MAX_CARD_NUM > card_no, return);

	card = sal_get_card(card_no);
	if (NULL == card) {
		SAL_TRACE(OSP_DBL_MAJOR, 631, "can't find card by cardid:%d",
			  card_no);
		return;
	}


	SAS_LIST_FOR_EACH_SAFE(node, safe_node, &card->active_dev_list) {
		dev = SAS_LIST_ENTRY(node, struct sal_dev, dev_list);
		if (scsi_id != dev->scsi_id) {
			dev = NULL;
			continue;
		}

		SAL_TRACE(OSP_DBL_INFO, 632,
			  "Card:%d dev:0x%llx(ref cnt:%d,%d,%d) out,ST:%d",
			  card->card_no, dev->sas_addr, dev->cnt_report_in,
			  dev->cnt_destroy, SAS_ATOMIC_READ(&dev->ref),
			  dev->dev_status);

		spin_lock_irqsave(&dev->dev_state_lock, dev_flag);
		if (SAL_DEV_ST_FLASH == dev->dev_status) {
			SAS_LIST_DEL_INIT(&dev->dev_list);
			spin_unlock_irqrestore(&dev->dev_state_lock, dev_flag);
			(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_REPORT_OUT);
			SAL_TRACE(OSP_DBL_INFO, 633,
				  "Card:%d dev:0x%llx(sal dev:%p) ready to report out and del dev from active list",
				  card_no, dev->sas_addr, dev);
			real_out = true;
		} else {

			spin_unlock_irqrestore(&dev->dev_state_lock, dev_flag);
		}
		break;
	}


	if (dev) {

		if (real_out) {

			sal_clean_dev_io_in_err_handle(card, dev);
			/* 
			   * 1. return the IO which have sent to disk and not returned to mid layer
			   * 2.release timeout IO resource.
			 */
			sal_clr_active_io_by_dev(card, dev);
			sal_clr_tmo_io_by_dev(card, dev);

		}

		/* report out , then count decrease 1 */
		sal_put_dev(dev);       
		sal_dev_check_put_last(dev, false);

	}
	sal_put_card(card);
	return;
}

/*----------------------------------------------*
 * release dev resource.    *
 *----------------------------------------------*/
void sal_free_dev_rsc(u8 card_no, u32 scsi_id, u32 chan_id, u32 lun_id)
{
	struct sal_card *card = NULL;
	unsigned long flag = 0;
	unsigned long dev_flag = 0;
	struct list_head *node = NULL;
	struct list_head *safe_node = NULL;
	struct sal_dev *dev = NULL;
	bool real_out = false;

	SAL_REF(chan_id);
	SAL_REF(lun_id);
	SAL_ASSERT(SAL_MAX_CARD_NUM > card_no, return);

	card = sal_get_card(card_no);
	if (NULL == card) {
		SAL_TRACE(OSP_DBL_MAJOR, 631, "can't find card by cardid:%d",
			  card_no);
		return;
	}


	spin_lock_irqsave(&card->card_dev_list_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(node, safe_node, &card->active_dev_list) {
		dev = SAS_LIST_ENTRY(node, struct sal_dev, dev_list);
		if (scsi_id != dev->scsi_id) {
			dev = NULL;
			continue;
		}

		SAL_TRACE(OSP_DBL_INFO, 632,
			  "Card:%d dev:0x%llx(ref cnt:%d,%d,%d) out,ST:%d",
			  card->card_no, dev->sas_addr, dev->cnt_report_in,
			  dev->cnt_destroy, SAS_ATOMIC_READ(&dev->ref),
			  dev->dev_status);

		spin_lock_irqsave(&dev->dev_state_lock, dev_flag);
		if (SAL_DEV_ST_FLASH == dev->dev_status) {
			SAS_LIST_DEL_INIT(&dev->dev_list);
			spin_unlock_irqrestore(&dev->dev_state_lock, dev_flag);
			(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_REPORT_OUT);
			SAL_TRACE(OSP_DBL_INFO, 633,
				  "Card:%d dev:0x%llx(sal dev:%p) ready to report out and del dev from active list",
				  card_no, dev->sas_addr, dev);
			real_out = true;
		} else {

			spin_unlock_irqrestore(&dev->dev_state_lock, dev_flag);
		}
		break;
	}
	spin_unlock_irqrestore(&card->card_dev_list_lock, flag);

	if (dev) {

		if (real_out) {

			sal_clean_dev_io_in_err_handle(card, dev);
			/* 
			   * 1. return the IO which have sent to disk and not returned to mid layer
			   * 2.release timeout IO resource.
			 */
			sal_clr_active_io_by_dev(card, dev);
			sal_clr_tmo_io_by_dev(card, dev);

		}

		/* report out , then count decrease 1 */
		sal_put_dev(dev);       
		sal_dev_check_put_last(dev, false);

	}
	sal_put_card(card);
	return;
}

/*----------------------------------------------*
 * release dev reference count in the active list.    *
 *----------------------------------------------*/
void sal_put_all_dev(u8 card_no)
{
	struct sal_card *card = NULL;
	unsigned long flag = 0;
	struct list_head *node = NULL;
	struct list_head *safe_node = NULL;
	struct sal_dev *dev = NULL;

	SAL_ASSERT(SAL_MAX_CARD_NUM > card_no, return);

	card = sal_get_card(card_no);
	if (NULL == card) {
		SAL_TRACE(OSP_DBL_MAJOR, 631, "can't find card by cardid:%d",
			  card_no);
		return;
	}

	spin_lock_irqsave(&card->card_dev_list_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(node, safe_node, &card->active_dev_list) {
		dev = SAS_LIST_ENTRY(node, struct sal_dev, dev_list);
		SAS_LIST_DEL_INIT(&dev->dev_list);	/* delete from active list .*/

		if (SAL_DEV_TYPE_SMP == dev->dev_type) {
			/* SMP device state change */
			(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_MISS);
			(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_DEREG);
			(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_DEREG_OK);
		} else {
			if (SAL_DEV_ST_ACTIVE == sal_get_dev_state(dev)) {
				(void)sal_dev_state_chg(dev,
							SAL_DEV_EVENT_MISS);
				(void)sal_dev_state_chg(dev,
							SAL_DEV_EVENT_DEREG);
				(void)sal_dev_state_chg(dev,
							SAL_DEV_EVENT_DEREG_OK);
				(void)sal_dev_state_chg(dev,
							SAL_DEV_EVENT_REPORT_OUT);
			} else if (SAL_DEV_ST_FLASH == sal_get_dev_state(dev)) {

				(void)sal_dev_state_chg(dev,
							SAL_DEV_EVENT_REPORT_OUT);
			}
		}

		/* release reference count.*/
		sal_put_dev_in_dev_list_lock(dev);
	}
	spin_unlock_irqrestore(&card->card_dev_list_lock, flag);

	sal_put_card(card);
	return;
}

/*----------------------------------------------*
 * delete dev device.    *
 *----------------------------------------------*/
s32 sal_del_port_dev(struct sal_card * sal_card, struct sal_port * port)
{
	struct list_head *node = NULL;
	struct list_head *safe_node = NULL;
	struct sal_dev *dev = NULL;
	SAS_ATOMIC send_num;
	unsigned long flag = 0;
	s32 wait_time = SAL_DEL_DEV_TIMEOUT;
	s32 ret = ERROR;

	if (NULL == sal_card->ll_func.dev_op) {
		SAL_TRACE(OSP_DBL_MAJOR, 604, "pfnDelDev func is NULL");
		return ERROR;
	}

	SAS_ATOMIC_SET(&send_num, 0);
	SAL_TRACE(OSP_DBL_INFO, 605, "Card:%d will del disk num:%d",
		  sal_card->card_no, SAS_ATOMIC_READ(&port->dev_num));

	spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(node, safe_node, &sal_card->active_dev_list) {
		dev = SAS_LIST_ENTRY(node, struct sal_dev, dev_list);
		if (dev->port == port) {
			spin_lock_irqsave(&dev->dev_state_lock, flag);
			if (SAL_DEV_ST_FLASH == dev->dev_status) {
				spin_unlock_irqrestore(&dev->dev_state_lock,
						       flag);
				continue;
			}
			spin_unlock_irqrestore(&dev->dev_state_lock, flag);

			SAS_ATOMIC_INC(&send_num);

			/*state change*/
			(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_DEREG);
			/* Quark_DevOperation */
			ret = sal_card->ll_func.dev_op(dev, SAL_DEV_OPT_DEREG);	/*logout low-layer device  */
			if (OK != ret) {
				SAS_ATOMIC_DEC(&send_num);
				SAL_TRACE(OSP_DBL_MAJOR, 607,
					  "Card:%d del disk:0x%llx dev failed",
					  sal_card->card_no, dev->sas_addr);
			}
		}
	}
	spin_unlock_irqrestore(&sal_card->card_dev_list_lock, flag);
	SAL_TRACE(OSP_DBL_INFO, 608, "Card:%d send request times:%d",
		  sal_card->card_no, SAS_ATOMIC_READ(&send_num));

	while (wait_time >= 0) {
		if (0 == SAS_ATOMIC_READ(&port->dev_num))
			return OK;

		SAS_MSLEEP(1);
		wait_time--;
	}
	SAL_TRACE(OSP_DBL_MAJOR, 609,
		  "Card:%d Port:0x%x(FW PortId:%d) delete all dev time out,remaining dev num:%d",
		  sal_card->card_no, port->logic_id, port->port_id,
		  SAS_ATOMIC_READ(&port->dev_num));

	return ERROR;
}

/*----------------------------------------------*
 * delete report process of PORT.    *
 *----------------------------------------------*/
void sal_del_port_report(struct sal_card *sal_card, struct sal_port *port)
{
	struct list_head *node = NULL;
	struct list_head *safe_node = NULL;
	struct sal_dev *dev = NULL;
	unsigned long flag = 0;

	spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(node, safe_node, &sal_card->active_dev_list) {
		dev = SAS_LIST_ENTRY(node, struct sal_dev, dev_list);
		if (dev->port == port)
			sal_del_disk_report(sal_card, dev);
	}
	spin_unlock_irqrestore(&sal_card->card_dev_list_lock, flag);

	return;
}

/*----------------------------------------------*
 * chip reset interface for frame and api.  *
 *----------------------------------------------*/
s32 sal_ctrl_reset_chip_by_card(void *argv_in, void *argv_out)
{
	s32 ret = ERROR;
	u32 card_id = 0;

	SAL_REF(argv_out);

	card_id = (u32) (u64) argv_in;

	SAL_TRACE(OSP_DBL_INFO, 313, "Card:%d ctrl thread => chip soft reset",
		  card_id);
	ret = sal_comm_reset_chip(&card_id, NULL);

	return ret;
}

/*----------------------------------------------*
 * check all dev in port.    *
 *----------------------------------------------*/
s32 sal_check_all_dev_in_port(struct sal_port * sal_port)
{
	if (0 == atomic_read(&sal_port->dev_num))
		return OK;

	SAL_TRACE(OSP_DBL_MAJOR, 609,
		  "Card:%d delete port:0x%x all dev failed,remaining dev num:%d",
		  sal_port->card->card_no, sal_port->logic_id,
		  atomic_read(&sal_port->dev_num));
	return ERROR;
}

/*----------------------------------------------*
 * clear error handle notify by port.    *
 *----------------------------------------------*/
void sal_clr_err_handler_notify_by_port(struct sal_card *sal_card,
					struct sal_port *sal_port)
{
	struct list_head *node = NULL;
	struct list_head *safe_node = NULL;
	struct sal_dev *dev = NULL;
	unsigned long flag = 0;

	if (OK != sal_enter_err_hand_mutex_area(sal_card))
		return;

	spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(node, safe_node, &sal_card->active_dev_list) {
		dev = SAS_LIST_ENTRY(node, struct sal_dev, dev_list);
		if (dev->port == sal_port) {
			spin_unlock_irqrestore(&sal_card->card_dev_list_lock,
					       flag);

			sal_clr_err_handler_notify_by_dev(sal_card, dev, false);

			spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
		}
	}
	spin_unlock_irqrestore(&sal_card->card_dev_list_lock, flag);

	sal_leave_err_hand_mutex_area(sal_card);
	return;
}

/*----------------------------------------------*
 * process port clear event arose by PORT.    *
 *----------------------------------------------*/
s32 sal_handle_port_del(struct sal_card * sal_card, struct sal_event * event)
{
	struct sal_port *port = NULL;
	s32 ret = ERROR;

	port = (struct sal_port *)event->info.port_event.port;
	SAL_TRACE(OSP_DBL_INFO, 611,
		  "Card:%d is going to delete port:%d device,time:%d ms",
		  sal_card->card_no, port->port_id,
		  jiffies_to_msecs(jiffies - event->time));

	(void)sal_set_port_dev_state(sal_card, port, SAL_DEV_EVENT_MISS);

	sal_clr_err_handler_notify_by_port(sal_card, port);

	ret = sal_comm_clean_up_port_rsc(sal_card, port);
	if (OK != ret) {	/* clear port resource fail */
		SAL_TRACE(OSP_DBL_MAJOR, 615,
			  "Card:%d clean up port:%d rsc failed",
			  sal_card->card_no, port->port_id);
		SAL_TRACE(OSP_DBL_INFO, 616,
			  "Card:%d is abnormal, it need to dump fw and reset for recovery",
			  sal_card->card_no);

		(void)sal_comm_dump_fw_info(sal_card->card_no,
					    SAL_ERROR_INFO_ALL);
	} else {

		ret = sal_check_all_dev_in_port(port);
	}

	if ((OK != ret) && (SAL_FALSE == sal_card->reseting)) {	/*chip exception , reset chip to modify. */
		sal_send_raw_ctrl_no_wait(sal_card->card_no,
					  (void *)(u64) sal_card->card_no, NULL,
					  sal_ctrl_reset_chip_by_card);
		return ret;
	}

	/*report disk event. */
	sal_del_port_report(sal_card, port);

	/* re-initialize port. */
	sal_re_init_port(sal_card, port);

	return ret;
}

/*----------------------------------------------*
 * initialize LL PORT.    *
 *----------------------------------------------*/
void sal_release_ll_port_rsc(struct sal_card *sal_card,
			     struct sal_port *sal_port)
{
	if (NULL != sal_port->card->ll_func.port_op)
		/* Quark_PortOperation */
		(void)sal_port->card->ll_func.port_op(sal_card,
						      SAL_PORT_OPT_DELETE,
						      (void *)&sal_port->index);

	return;
}

/*----------------------------------------------*
 * initialize  PORT.    *
 *----------------------------------------------*/
void sal_re_init_port(struct sal_card *sal_card, struct sal_port *port)
{
	u32 i = 0;
	unsigned long flag = 0;
	u32 chip_port_id = 0;

	SAL_ASSERT(NULL != port, return);

	SAL_TRACE(OSP_DBL_INFO, 610, "Card:%d reinit port:%d rsc(index:%d) ",
		  sal_card->card_no, port->port_id, port->index);

	spin_lock_irqsave(&sal_card->card_lock, flag);
	/*struct sal_port initialize. */
	chip_port_id = port->port_id;

	port->logic_id = SAL_INVALID_LOGIC_PORTID;
	port->port_id = SAL_INVALID_PORTID;
	port->status = SAL_PORT_FREE;
	port->port_need_do_full_disc = SAL_TRUE;
	port->is_conn_high_jbod = SAL_FALSE;

	/*notify clear disc port msg.  */
	sal_notify_to_clr_disc_port_msg(sal_card->card_no, chip_port_id,
					(u8) port->index);

	spin_unlock_irqrestore(&sal_card->card_lock, flag);

	port->set_dev_state.print_cnt = 0;
	port->set_dev_state.last_print_time = 0;
	port->free_all_dev.print_cnt = 0;
	port->free_all_dev.last_print_time = 0;

	for (i = 0; i < SAL_MAX_PHY_NUM; i++)
		port->phy_id_list[i] = SAL_FALSE;

	SAS_ATOMIC_SET(&port->dev_num, 0);
	SAS_ATOMIC_SET(&port->pend_abort, 0);

	sal_release_ll_port_rsc(sal_card, port);

	SAS_ATOMIC_SET(&port->port_in_use, SAL_PORT_RSC_UNUSE);
	return;
}

/*----------------------------------------------*
 * change device type define.    *
 *----------------------------------------------*/
u32 sal_get_chg_delay_dev_type(struct sal_dev * device)
{
	u32 dev_type = 0;

	switch (device->dev_type) {
	case SAL_DEV_TYPE_SSP:
		if (SAL_TRUE == device->virt_phy)
			dev_type = SASINI_DEVICE_SES;
		else
			dev_type = SASINI_DEVICE_SAS;

		break;

	case SAL_DEV_TYPE_STP:
		dev_type = SASINI_DEVICE_SATA;
		break;

	case SAL_DEV_TYPE_SMP:
		break;

	case SAL_DEV_TYPE_BUTT:
	default:
		SAL_TRACE(OSP_DBL_MAJOR, 621, "Disk:0x%llx unknown device type",
			  device->sas_addr);
		break;
	}

	return dev_type;
}

/*----------------------------------------------*
 * obtain event type of delay report event .  *
 *----------------------------------------------*/
enum sas_ini_event sal_get_chg_dly_event_type(u32 event, u32 dev_type)
{
	enum sas_ini_event ini_event = SASINI_EVENT_BUTT;

	if (SASINI_EVENT_DEV_IN == event) {
		ini_event = SASINI_EVENT_DISK_IN;
		if (SASINI_DEVICE_SES == dev_type)
			ini_event = SASINI_EVENT_FRAME_IN;
	} else {
		ini_event = SASINI_EVENT_DISK_OUT;
		if (SASINI_DEVICE_SES == dev_type)
			ini_event = SASINI_EVENT_FRAME_OUT;
	}

	return ini_event;
}

/*----------------------------------------------*
 * obtain and set device scsi id.    *
 *----------------------------------------------*/
s32 sal_set_scsi_id(struct sal_card * sal_card, struct sal_dev * dev)
{
	u32 i = 0;
	unsigned long flag = 0;
	s32 ret = ERROR;

	SAL_ASSERT(NULL != dev, return ERROR);

	if (SAL_INVALID_SCSIID != dev->scsi_id) {
		SAL_TRACE(OSP_DBL_DATA, 617,
			  "dev:0x%llx(scsi id:%d slot id:%d) is already in",
			  dev->sas_addr, dev->scsi_id, dev->slot_id);
		return OK;
	}


	 /*SES*/ if (SAL_TRUE == dev->virt_phy) {
		spin_lock_irqsave(&sal_card->card_lock, flag);
		for (i = 0; i < SAL_MAX_FRAME_SCSI_NUM; i++)
			if ((NULL == sal_card->scsi_id_ctl.dev_slot[i])
			    && (SAL_SCSI_SLOT_FREE ==
				sal_card->scsi_id_ctl.slot_use[i])) {
				sal_card->scsi_id_ctl.slot_use[i] =
				    SAL_SCSI_SLOT_USED;
				sal_card->scsi_id_ctl.dev_slot[i] = dev;
				break;
			}

		if (SAL_MAX_FRAME_SCSI_NUM == i) {
			SAL_TRACE(OSP_DBL_MAJOR, 618,
				  "Card:%d not find free dev slot for frame",
				  sal_card->card_no);
			ret = ERROR;
		} else {
			SAL_TRACE(OSP_DBL_DATA, 618,
				  "Card:%d dev:0x%llx get free scsi id:%d for frame",
				  sal_card->card_no, dev->sas_addr, i);
			dev->scsi_id = i;
			ret = OK;
		}
		spin_unlock_irqrestore(&sal_card->card_lock, flag);

		return ret;
	}

	/* disk */
	spin_lock_irqsave(&sal_card->card_lock, flag);
	for (i = SAL_MAX_FRAME_SCSI_NUM; i < SAL_MAX_DEV_NUM; i++)
		if ((NULL == sal_card->scsi_id_ctl.dev_slot[i])
		    && (SAL_SCSI_SLOT_FREE ==
			sal_card->scsi_id_ctl.slot_use[i])) {
			sal_card->scsi_id_ctl.slot_use[i] = SAL_SCSI_SLOT_USED;
			sal_card->scsi_id_ctl.dev_slot[i] = dev;
			break;
		}

	if (SAL_MAX_DEV_NUM == i) {
		SAL_TRACE(OSP_DBL_MAJOR, 619,
			  "Card:%d not find free dev slot for disk",
			  sal_card->card_no);
		ret = ERROR;
	} else {
		SAL_TRACE(OSP_DBL_DATA, 618,
			  "Card:%d dev:0x%llx get free scsi id:%d for disk",
			  sal_card->card_no, dev->sas_addr, i);
		dev->scsi_id = i;
		ret = OK;
	}
	spin_unlock_irqrestore(&sal_card->card_lock, flag);

	return ret;
}

/*----------------------------------------------*
 * set disk virtul SAS address and device.    *
 *----------------------------------------------*/
void sal_set_virt_sas_addr(struct sal_dev *dev)
{
	struct sal_port *port = NULL;
	struct sal_dev *exp = NULL;
	unsigned long flag = 0;
	u64 sas_addr = 0;

	port = dev->port;
	if (SAL_TRUE == dev->virt_phy) {
		sas_addr = dev->father_sas_addr;
		exp =
		    sal_get_dev_by_sas_addr_in_dev_list_lock(dev->port,
							     sas_addr);
		if (NULL == exp) {
			SAL_TRACE(OSP_DBL_MAJOR, 620,
				  "Err! Disk:0x%llx cannot get parent sasaddr",
				  sas_addr);
			return;
		}

		spin_lock_irqsave(&dev->dev_lock, flag);
		dev->virt_sas_addr = exp->sas_addr;
		dev->virt_father_addr = exp->father_sas_addr;

		if (port->sas_addr == exp->father_sas_addr)
			dev->virt_father_addr = SASINI_HBA_SASADDR;

		spin_unlock_irqrestore(&dev->dev_lock, flag);
		sal_put_dev(exp);
		return;
	} else if (SAL_MODE_DIRECT == port->link_mode) {
		spin_lock_irqsave(&dev->dev_lock, flag);
		dev->virt_sas_addr = dev->sas_addr;
		dev->virt_father_addr = SASINI_HBA_SASADDR;
		spin_unlock_irqrestore(&dev->dev_lock, flag);
		return;
	}

	spin_lock_irqsave(&dev->dev_lock, flag);
	dev->virt_sas_addr = dev->sas_addr;
	dev->virt_father_addr = dev->father_sas_addr;
	spin_unlock_irqrestore(&dev->dev_lock, flag);

	return;
}

/*----------------------------------------------*
 * disk report process.    *
 *----------------------------------------------*/
void sal_add_disk_report(struct sal_card *sal_card, struct sal_dev *dev)
{
	unsigned long flag = 0;

	/* set device SCSI ID */
	if (OK != sal_set_scsi_id(sal_card, dev)) {
		SAL_TRACE(OSP_DBL_MAJOR, 623,
			  "Card:%d disk:0x%llx get SCSI ID failed",
			  sal_card->card_no, dev->sas_addr);

		///TODO:状态迁移
		(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_REPORT_FAILED);
		return;
	}

	sal_set_virt_sas_addr(dev);

	spin_lock_irqsave(&dev->dev_lock, flag);
	if (SAL_DEV_TYPE_STP == dev->dev_type) {
		/*SATA disk */
		dev->sata_dev_data.dev_id = dev->sas_addr;
		dev->sata_dev_data.sata_state = SATA_DEV_STATUS_NORMAL;
		dev->sata_dev_data.ncq_enable = sal_card->config.ncq_switch;
		dev->sata_dev_data.upper_dev = dev;
	}
	spin_unlock_irqrestore(&dev->dev_lock, flag);

	/*state change */
	(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_REPORT_OK);

	sal_report_chg_delay_intf(dev, SASINI_EVENT_DEV_IN);

	return;
}

/*----------------------------------------------*
 * report disk in/out event to report module after disc.*
 *----------------------------------------------*/
s32 sal_handle_disc_done(struct sal_card * sal_card, struct sal_event * event)
{
	struct list_head *node = NULL;
	struct sal_dev *dev = NULL;
	struct sal_port *port = NULL;
	unsigned long flag = 0;
	unsigned long dev_flag = 0;

	if (SAL_PORT_MODE_INI != sal_card->config.work_mode)
		return ERROR;

	port = (struct sal_port *)event->info.port_event.port;

	spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
       SAS_LIST_FOR_EACH(node, &sal_card->active_dev_list)  {
        
		dev = SAS_LIST_ENTRY(node, struct sal_dev, dev_list);

		spin_lock_irqsave(&dev->dev_state_lock, dev_flag);
		if ((SAL_DEV_ST_REG == dev->dev_status)
		    && (dev->port == port)
		    && (dev->logic_id == port->logic_id)
		    && (SAL_DEV_TYPE_SMP != dev->dev_type)
		    && (SAL_TRUE != dev->virt_phy)) {
			spin_unlock_irqrestore(&dev->dev_state_lock, dev_flag);
			/* report disk in event to delay report module */
			sal_add_disk_report(sal_card, dev);
		} else if ((SAL_DEV_ST_FLASH == dev->dev_status)
			   && (dev->port == port)
			   && (dev->logic_id == port->logic_id)
			   && (SAL_DEV_TYPE_SMP != dev->dev_type)) {
			spin_unlock_irqrestore(&dev->dev_state_lock, dev_flag);
			/*  report disk out */
			sal_del_disk_report(sal_card, dev);
		} else {
			spin_unlock_irqrestore(&dev->dev_state_lock, dev_flag);
		}

     }
	spin_unlock_irqrestore(&sal_card->card_dev_list_lock, flag);

	return OK;
}

/*----------------------------------------------*
 * queue event process.*
 *----------------------------------------------*/
void sal_event_process(struct sal_card *sal_card)
{
	struct sal_event *sal_event = NULL;
	s32 ret = ERROR;
	unsigned long flag = 0;
	char *str[] = {
		"unknow",
		"Add dev",
		"del dev",
		"del port dev",
		"disc done",
		"phy event",
		"BUTT"
	};
	SAL_REF(str[0]);

	for (;;) {
		spin_lock_irqsave(&sal_card->card_event_lock, flag);
             if (SAS_LIST_EMPTY(&sal_card->event_active_list)) {
              /*list is null, quit.*/
			spin_unlock_irqrestore(&sal_card->card_event_lock,
					       flag);
			break;
		} else {
			sal_event =
			    SAS_LIST_ENTRY(sal_card->event_active_list.next,
					   struct sal_event, list);
			SAS_LIST_DEL_INIT(&sal_event->list);
			spin_unlock_irqrestore(&sal_card->card_event_lock,
					       flag);
		}

		/*power off/on */
		spin_lock_irqsave(&sal_card->card_lock, flag);
		if (SAL_OR((sal_card->flag & SAL_CARD_REMOVE_PROCESS),
			   (sal_card->flag & SAL_CARD_RESETTING))) {
			spin_unlock_irqrestore(&sal_card->card_lock, flag);

			SAL_TRACE(OSP_DBL_MAJOR, 626,
				  "Card:%d(flag:0x%x) is shutting down/resetting now",
				  sal_card->card_no, sal_card->flag);

			if (SAL_AND
			    ((SAL_EVENT_ADD_DEV == sal_event->event),
			     (sal_event->disc_compl))) {
				/*add disk event.*/
				*sal_event->info.disk_add.status =
				    SAL_ADD_DEV_FAILED;
				SAS_COMPLETE(sal_event->disc_compl);
			}

			sal_put_free_event(sal_card, sal_event);
			continue;
		}
		spin_unlock_irqrestore(&sal_card->card_lock, flag);

		switch (sal_event->event) {
		case SAL_EVENT_ADD_DEV:
			ret = sal_handle_disk_add(sal_card, sal_event);
			break;

		case SAL_EVENT_DEL_DEV:
			ret = sal_handle_disk_del(sal_card, sal_event);
			break;

		case SAL_EVENT_DEL_PORT_DEV:
			ret = sal_handle_port_del(sal_card, sal_event);
			break;

		case SAL_EVENT_DISC_DONE:
			ret = sal_handle_disc_done(sal_card, sal_event);
			break;

		case SAL_EVENT_PORT_REPAIR:

			ret =
			    sal_handle_all_port_rsc_clean(sal_card, sal_event);
			break;


		case SAL_EVENT_BUTT:
		default:
			SAL_TRACE(OSP_DBL_MAJOR, 627,
				  "Card:%d can't handle unknown event:%d",
				  sal_card->card_no, sal_event->event);
			break;
		}

		if (ERROR == ret)
			SAL_TRACE(OSP_DBL_MAJOR, 628,
				  "Card:%d process event:%d(%s) failed",
				  sal_card->card_no, sal_event->event,
				  str[sal_event->event]);

		sal_put_free_event(sal_card, sal_event);
	}

}

/*----------------------------------------------*
 * event process thread.*
 *----------------------------------------------*/
s32 sal_event_handler(void *param)
{
	struct sal_card *sal_card = NULL;


	sal_card = (struct sal_card *)param;
	/*avoid invalid wake up issue.*/
	SAS_SET_CURRENT_STATE(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		sal_event_process(sal_card);
		schedule();
		SAS_SET_CURRENT_STATE(TASK_INTERRUPTIBLE);
	}

	SAL_TRACE(OSP_DBL_INFO, 629, "Card:%d event thread exit",
		  sal_card->card_no);
	__set_current_state(TASK_RUNNING);
	sal_card->event_thread = NULL;

	return OK;
}

/*----------------------------------------------*
 * detect chip error code.*
 *----------------------------------------------*/
void sal_phy_err_code_inquery(struct sal_card *sal_card)
{
	u32 i = 0;
	struct sal_phy_ll_op_param op_param = { 0 };

	if (sal_card->flag & SAL_CARD_LOOPBACK)
		return;

	if (NULL == sal_card->ll_func.phy_op) {
		SAL_TRACE(OSP_DBL_MAJOR, 654, "phy_op is NULL!");
		return;
	}

	for (i = 0; i < sal_card->phy_num; i++) {
		if (NULL == sal_card->phy[i])
			continue;

		op_param.card_id = sal_card->card_no;
		op_param.phy_id = i;
		if (SAL_PHY_ERR_CODE_EXIST == sal_card->phy[i]->phy_err_flag) {
			if (ERROR ==
			    sal_card->ll_func.phy_op(sal_card,
						     SAL_PHY_OPT_QUERY_ERRCODE,
						     (void *)&op_param))
				/* Quark_InqueryChipErrCode */
				SAL_TRACE(OSP_DBL_MAJOR, 655,
					  "Card:%d check Phy:%d Err Code failed",
					  sal_card->card_no, i);

			sal_card->phy[i]->phy_err_flag = SAL_PHY_ERR_CODE_NO;
		}

	}

	return;
}

/*----------------------------------------------*
 * check phy error code process period. *
 *----------------------------------------------*/
void sal_check_phy_err_period(struct sal_card *sal_card, u8 phy_id)
{
	struct sal_phy *tmp_phy = NULL;

	tmp_phy = sal_card->phy[phy_id];
	if (tmp_phy->err_code.phy_err_pos >= SAL_MAX_PHY_ERR_PERIOD)
		tmp_phy->err_code.phy_err_pos = 0;

	return;
}

/*----------------------------------------------*
 * set error code process info according to the phy error code count.*
 *----------------------------------------------*/
void sal_set_phy_err_info_by_count(struct sal_card *sal_card, u8 phy_id)
{
	struct sal_phy *tmp_phy = NULL;
	u32 pos = 0;

	tmp_phy = sal_card->phy[phy_id];
	if ((tmp_phy->err_code.inv_dw_chg_cnt >=
	     sal_card->config.bit_err_interval_threshold)
	    || (tmp_phy->err_code.phy_chg_cnt_per_period >= 1)) {
		pos = tmp_phy->err_code.phy_err_pos;
		SAL_TRACE(OSP_DBL_MINOR, 646,
			  "Card:%d phy:%2d jiffies:%ld pos:%d "
			  "invalid dword:%d phy change:%d phy error",
			  sal_card->card_no, phy_id, jiffies, pos,
			  tmp_phy->err_code.inv_dw_chg_cnt,
			  tmp_phy->err_code.phy_chg_cnt_per_period);
		tmp_phy->err_code.phy_err[pos] = SAL_PHY_ERROR;
		tmp_phy->err_code.phy_err_time[pos] = jiffies;
		tmp_phy->err_code.phy_err_pos++;
	}
}

/*----------------------------------------------*
 * insulate PHY process.*
 *----------------------------------------------*/
void sal_insulate_phy_process(struct sal_card *sal_card, u8 phy_id)
{
	if (SAL_CARD_LOOPBACK & sal_card->flag)
		return;

	if (phy_id >= sal_card->phy_num) {
		SAL_TRACE(OSP_DBL_MAJOR, 644, "Card:%d phy:%d is invalid",
			  sal_card->card_no, phy_id);
		return;
	}
	/*        close PHY    */
	SAL_TRACE(OSP_DBL_MINOR, 645, "Card:%d phy:%d bit err overflow disable",
		  sal_card->card_no, phy_id);

	/*update PHY config state. */
	sal_card->phy[phy_id]->config_state = SAL_PHY_INSULATE_CLOSE;

	(void)sal_comm_switch_phy(sal_card, phy_id, SAL_PHY_OPT_CLOSE,
				  sal_card->config.phy_link_rate[phy_id]);
	return;
}

/*----------------------------------------------*
 * check phy insulate condition.*
 *----------------------------------------------*/
void sal_check_phy_disable(struct sal_card *sal_card, u8 phy_id)
{
	struct sal_bit_err *err_info = NULL;
	struct sal_phy *tmp_phy = NULL;
	u32 phy_err_cnt = 0;
	unsigned long first_phy_err = 0;
	unsigned long last_phy_err = 0;
	u32 phy_err_interval = 0;
	u32 i = 0;

	tmp_phy = sal_card->phy[phy_id];

	sal_check_phy_err_period(sal_card, phy_id);
	sal_set_phy_err_info_by_count(sal_card, phy_id);

	err_info = &tmp_phy->err_code;
	first_phy_err = err_info->phy_err_time[0];
	last_phy_err = err_info->phy_err_time[0];

	for (i = 0; i < SAL_MAX_PHY_ERR_PERIOD; i++)
		if (SAL_PHY_ERROR == err_info->phy_err[i]) {
			phy_err_cnt++;
			first_phy_err =
			    MIN(first_phy_err, err_info->phy_err_time[i]);
			last_phy_err =
			    MAX(last_phy_err, err_info->phy_err_time[i]);
		}

	phy_err_interval = jiffies_to_msecs(last_phy_err - first_phy_err);

	if ((phy_err_cnt >= sal_card->config.bit_err_stop_threshold) &&
	    (phy_err_interval <=
	     SAL_MS_PER_MIN * sal_card->config.bit_err_routine_cnt)) {

		for (i = 0; i < SAL_MAX_PHY_ERR_PERIOD; i++)
			err_info->phy_err[i] = SAL_PHY_OK;

		SAL_TRACE(OSP_DBL_MINOR, 648,
			  "Card:%d phy:%d bit error count:%d duration time:%dms",
			  sal_card->card_no, phy_id, phy_err_cnt,
			  phy_err_interval);
		if (sal_card->phy[phy_id]
		    && sal_card->phy[phy_id]->err_code.bit_err_enable)
			/* have opened insulate function.  */
			sal_insulate_phy_process(sal_card, phy_id);

	}

	return;
}

/*----------------------------------------------*
 * error code process function.*
 *----------------------------------------------*/
void sal_bit_err_proc(struct sal_card *sal_card)
{
	u32 phy_id = 0;
	struct sal_bit_err *bit_err_info = NULL;
	u32 period = 0;		/* time interval */

	sal_phy_err_code_inquery(sal_card);

	/*obtain error code count.*/
	for (phy_id = 0; phy_id < sal_card->phy_num; phy_id++) {
		if (SAL_PHY_CLOSED == sal_card->phy[phy_id]->link_status)
			continue;

		period = sal_card->config.bit_err_routine_time;
		bit_err_info = &sal_card->phy[phy_id]->err_code;

		if (time_before_eq
		    (jiffies,
		     (bit_err_info->last_chk_jiff +
		      (unsigned long)period * HZ)))

			continue;
		else

			bit_err_info->last_chk_jiff = jiffies;

		bit_err_info->loss_dw_synch_cnt +=
		    bit_err_info->loss_dw_syn_chg_cnt;
		bit_err_info->code_viol_cnt += bit_err_info->cod_vio_chg_cnt;
		bit_err_info->disparity_err_cnt +=
		    bit_err_info->disp_err_chg_cnt;
		bit_err_info->invalid_dw_cnt += bit_err_info->inv_dw_chg_cnt;
		bit_err_info->phy_chg_cnt +=
		    bit_err_info->phy_chg_cnt_per_period;

		sal_check_phy_disable(sal_card, (u8) phy_id);

		bit_err_info->loss_dw_syn_chg_cnt = 0;
		bit_err_info->cod_vio_chg_cnt = 0;
		bit_err_info->disp_err_chg_cnt = 0;
		bit_err_info->inv_dw_chg_cnt = 0;
		bit_err_info->phy_chg_cnt_per_period = 0;

	}

}

/*----------------------------------------------*
 * detect chip fault.*
 *----------------------------------------------*/
void sal_chip_fatal_examine(struct sal_card *sal_card)
{
	s32 ret = ERROR;
	unsigned long flag = 0;
	enum sal_chip_chk_ret chk_ret = SAL_ROUTINE_CHK_RET_BUTT;

	if ((SAL_CARD_RESETTING & sal_card->flag)
	    || (SAL_CARD_LOOPBACK & sal_card->flag)
	    || (SAL_CARD_INIT_PROCESS & sal_card->flag)
	    || (SAL_CARD_REMOVE_PROCESS & sal_card->flag)
	    || (SAL_CARD_FW_UPDATE_PROCESS & sal_card->flag)
	    || (SAL_CARD_EEPROM_UPDATE_PROCESS & sal_card->flag)
	    || (SAL_TRUE == sal_card->reseting))
		return;

	if (NULL == sal_card->ll_func.chip_op) {
		SAL_TRACE(OSP_DBL_MAJOR, 652, "chip_op is NULL!");
		return;
	}

	ret =
	    sal_card->ll_func.chip_op(sal_card, SAL_CHIP_OPT_ROUTE_CHECK,
				      &chk_ret);
	if (ERROR == ret)
		/*chip detect fail.  */
		return;

	if (SAL_ROUTINE_CHK_RET_OK != chk_ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 653,
			  "Card:%d chip fatal err happen, err type:%d",
			  sal_card->card_no, chk_ret);

		(void)sal_comm_dump_fw_info(sal_card->card_no,
					    SAL_ERROR_INFO_ALL);

		spin_lock_irqsave(&sal_card->card_lock, flag);
		if (!(SAL_CARD_REMOVE_PROCESS & sal_card->flag)) {
			if (SAL_ROUTINE_CHK_RET_CHIP_PD == chk_ret)
				return;
			else if (SAL_ROUTINE_CHK_RET_CHIP_RST == chk_ret)
				/*report chip reset event .  */
				(void)sal_add_card_to_err_handler(sal_card,
								  SAL_ERR_HANDLE_ACTION_RESET_HOST);

		}
		spin_unlock_irqrestore(&sal_card->card_lock, flag);
	}

	return;
}
#if 0
/*****************************************************************************
 函 数 名  : sal_get_avg_serv_time
 功能描述  : 计算本周期的平均服务时间
 输入参数  : struct sal_dev *dev  
              unsigned long circle_jiff
             入参合法性在调用函数已经保证
 输出参数  : 无
 返 回 值  : OSP_U32单位ms
*****************************************************************************/
u32 sal_get_avg_serv_time(struct sal_dev * dev, unsigned long circle_jiff)
{
	u64 idle_time_ns = 0;	/* 上次空闲到现在的时间，单位ns */
	u32 avg_serv_time_ms = 0;	/* 本周期内的平均服务响应时间 */
	u32 circle_ms = 0;

	circle_ms = jiffies_to_msecs(circle_jiff);
	/* 计算本周期的平均服务时间 */
	if (0 == dev->slow_io.pend_io) {	/* 还要加上尾巴部分的idle时间 */
		idle_time_ns = OSAL_CURRENT_TIME_NSEC - dev->slow_io.last_idle_start_ns;	/* ullLastIdleStartNs在被用之后，会被更新 */
		dev->slow_io.sum_idle_ns += idle_time_ns;	/* 本期累加 */
	}

	if (0 == dev->slow_io.sum_io && 0 == dev->slow_io.pend_io) {	/* 本周期内没有IO pend，也没有io返回，则平均服务时间为0 */
		avg_serv_time_ms = 0;
	} else if (0 == dev->slow_io.sum_io && dev->slow_io.pend_io) {
		/*         IF( on_flying > 0 && ios == 0 ) */
		/* svctm = busy_tm / on_flying;   // 要保证统计周期　>　平均延时阈值*并发  */
		/* 有io在服务，但是本周期内又没有IO返回 */
		avg_serv_time_ms =
		    (u32) (circle_ms - (dev->slow_io.sum_idle_ns / 1000000))
		    / dev->slow_io.pend_io;
	} else {
		if ((circle_ms - (dev->slow_io.sum_idle_ns / 1000000))
		    >= 1000 * 60 * 60)
			/* 数据不正确 */
			avg_serv_time_ms = 0;
		else
			avg_serv_time_ms =
			    (u32) (circle_ms -
				   (dev->slow_io.sum_idle_ns / 1000000))
			    / dev->slow_io.sum_io;
	}

	dev->slow_io.sum_io_snap = dev->slow_io.sum_io;
	dev->slow_io.pend_io_snap = dev->slow_io.pend_io;
	dev->slow_io.busy_time_snap_ms =
	    (u32) (circle_ms - (dev->slow_io.sum_idle_ns / 1000000));
	return avg_serv_time_ms;
}

/*****************************************************************************
 函 数 名  : sal_is_slow_disk
 功能描述  : 判断该盘是否慢盘
 输入参数  : struct sal_dev *dev  
               u32 curr_pos
             入参合法性在调用函数已经保证
 输出参数  : 无
 返 回 值  : void
*****************************************************************************/
bool sal_is_slow_disk(struct sal_dev * dev, u32 curr_pos)
{
	u32 i = 0;		/* 检查多少个周期 */
	u32 slow_circle_cnt = 0;	/* 慢周期的个数 */
	u32 check_pos = 0;
	u32 max_cnt = min((u32) SAL_DEVSLOWIO_CNT, sal_global.sal_comm_cfg.check_period);	/* 最大循环次数 */

	SAL_ASSERT(curr_pos < SAL_DEVSLOWIO_CNT, return false);	/* uiCurPos表示，最后一个，记录为慢周期的周期下标 */
	for (i = 0; i < max_cnt; i++) {
		check_pos = (curr_pos + SAL_DEVSLOWIO_CNT - i) % SAL_DEVSLOWIO_CNT;	/* 检查前N个周期 */
		SAL_ASSERT(check_pos < SAL_DEVSLOWIO_CNT, return false);	/* uiCurPos表示，最后一个，记录为慢周期的周期下标 */
		if (dev->slow_io.slow_jiff[check_pos])
			/* 是一个慢周期 */
			slow_circle_cnt++;

	}

	if (slow_circle_cnt >= sal_global.sal_comm_cfg.slow_period)
		/* 如果数组里面有足够的个慢周期了 */
		return true;
	else
		return false;

}

/*****************************************************************************
 函 数 名  : sal_process_slow_disk
 功能描述  : 记录慢IO并判断慢盘上报
 输入参数  : struct sal_dev *dev    
             入参合法性在调用函数已经保证
 输出参数  : 无
 返 回 值  : void
*****************************************************************************/
s32 sal_process_slow_disk(struct sal_dev * dev)
{
	u32 avg_serv_time_ms = 0;	/* 本周期内的平均服务响应时间 */
	struct sal_card *sal_card = NULL;
	unsigned long curr_jiff = jiffies;
	u32 interval_sec = sal_global.sal_comm_cfg.slow_disk_interval;	/* 单位秒 */
	unsigned long circle_jiff = 0;	/* 本次周期时间 */
	unsigned long flag = 0;
	u32 curr_pos = 0;
	u32 silents = 30;	/* 静默时间，单位秒，在这个时间内,只打印一次慢周期的信息 */

	SAL_REF(sal_card);

	sal_card = (struct sal_card *)(dev->card);
	SAL_ASSERT(NULL != sal_card, return ERROR);

	if (0 == sal_global.sal_comm_cfg.slow_disk_resp_time)
		/* uiSlowDiskRespTime单位为ms */
		return OK;

	circle_jiff = curr_jiff - dev->slow_io.last_chk_jiff;
	if (circle_jiff < ((unsigned long)interval_sec) * HZ)
		/* 还没到间隔时间,返回 */
		return OK;

	spin_lock_irqsave(&dev->slow_io.slow_io_lock, flag);	/* 锁住对stSlowIO结构的修改和获取 */
	/* 计算本周期的平均服务时间 */
	avg_serv_time_ms = sal_get_avg_serv_time(dev, circle_jiff);
	curr_pos = dev->slow_io.pos;

	if (avg_serv_time_ms > sal_global.sal_comm_cfg.slow_disk_resp_time) {	/* 是一个慢周期 */
		dev->slow_io.circle_cnt++;	/* 记录在两次打印期间的慢周期个数 */

		/*BEGIN 问题单号:  DTS2013092903793   modified by liujun 00248525 慢IO打印过多，冲日志 */
		if (time_after
		    (jiffies,
		     (unsigned long)silents * HZ +
		     dev->slow_io.last_print_slow_circle)
		    && (dev->slow_io.circle_cnt >= SAL_SLOWCIRCLE_NUM)) {
			/*END 问题单号:  DTS2013092903793   modified by liujun 00248525 */

			/* 达到了打印条件，不大量打印 */
			/* 打印，然后更新最后一次打印的时刻 */
			SAL_TRACE(OSP_DBL_MINOR, 2287,
				  "disk:0x%llx slow circle:%d io svctm:%d ms in %d ms, idle:%llu ns, svctm cfg:%d, Pos:%d,Circle Count:%d.",
				  dev->sas_addr, dev->slow_io.sum_io,
				  avg_serv_time_ms,
				  jiffies_to_msecs(circle_jiff),
				  dev->slow_io.sum_idle_ns,
				  sal_global.sal_comm_cfg.slow_disk_resp_time,
				  curr_pos, dev->slow_io.circle_cnt);

			dev->slow_io.last_print_slow_circle = jiffies;
			dev->slow_io.circle_cnt = 0;	//打印了之后，把两次打印之间，的静默条数归零
		}

		/* 记录为一个慢周期 */
		dev->slow_io.slow_jiff[curr_pos] = curr_jiff;	/* 记录为一个慢周期 */
		dev->slow_io.pos = (curr_pos + 1) % SAL_DEVSLOWIO_CNT;	/* 游标向后移 */

		/* 判断是否达到慢盘标准 */
		if (sal_is_slow_disk(dev, curr_pos)) {
			SAL_TRACE(OSP_DBL_INFO, 2163,
				  "Card:%d report slow disk:0x%llx[%d:%d:%d:%d]",
				  sal_card->card_no, dev->sas_addr,
				  sal_card->scsi_host.InitiatorNo, dev->chan_id,
				  dev->scsi_id, dev->lun_id);

			/*慢盘事件上报给框架 */
			sal_slow_disk_event_report(dev);

			/*BEGIN: 问题单号:DTS2013013007946  Modefied by jianghong 00221128,2013/2/23
			 *运行硬盘业务，多次升级sas fw后，发现上报慢盘
			 */
			sal_clear_slow_disk_record(dev);
			/*END: 问题单号:DTS2013013007946  Modefied by jianghong 00221128,2013/2/23 */
		}
	} else {		/* 不是慢周期 */
		dev->slow_io.slow_jiff[curr_pos] = 0;	/* 0表示这个周期不是慢周期 */
		dev->slow_io.pos = (curr_pos + 1) % SAL_DEVSLOWIO_CNT;	/* 游标向后移 */
	}

	/* 重新初始化IDLE时间等记录,pend_io 不清零 */
	dev->slow_io.last_idle_start_ns = OSAL_CURRENT_TIME_NSEC;
	dev->slow_io.sum_idle_ns = 0;
	dev->slow_io.sum_io = 0;

	/* 更新last 检查时间 */
	dev->slow_io.last_chk_jiff = curr_jiff;
	spin_unlock_irqrestore(&dev->slow_io.slow_io_lock, flag);

	return OK;
}
#endif

/*----------------------------------------------*
 * detect SAS clock fault.*
 *----------------------------------------------*/
void sal_check_card_clock_abnor(struct sal_card *sal_card)
{
	u32 i = 0;
	s32 ret = ERROR;
	u32 logic_id = 0;
	enum sal_card_clock clk_result = SAL_CARD_CLK_NORMAL;

	SAL_ASSERT(NULL != sal_card, return);

	if (SAL_CARD_CLK_ABNORMAL == sal_card->card_clk_abnormal_flag)
		return;

	/*judge card state. */
	if (!SAL_IS_CARD_READY(sal_card))
		return;

	if (time_before(jiffies, (sal_card->last_clk_chk_time + 10 * HZ)))
		return;

	sal_card->last_clk_chk_time = jiffies;

	for (i = 0; i < sal_card->phy_num; i++)
		if (SAL_PHY_LINK_UP == sal_card->phy[i]->link_status) {
			sal_card->card_clk_abnormal_num = 0;
			return;
		}

	if (NULL == sal_card->ll_func.chip_op) {
		SAL_TRACE(OSP_DBL_MAJOR, 652,
			  "Card %d pfnChipFaultCheck is NULL!",
			  sal_card->card_no);
		return;
	}

	/*Quark_ChipClockCheck */
	ret =
	    sal_card->ll_func.chip_op(sal_card, SAL_CHIP_OPT_CLOCK_CHECK,
				      &clk_result);
	if (ERROR == ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 652,
			  "Card %d pfnChipFaultCheck is failed!",
			  sal_card->card_no);
		return;
	}

	if (SAL_CARD_CLK_ABNORMAL == clk_result)
		sal_card->card_clk_abnormal_num++;
	else
		sal_card->card_clk_abnormal_num = 0;

	if (6 <= sal_card->card_clk_abnormal_num) {
		SAL_TRACE(OSP_DBL_MAJOR, 3001,
			  "Card %d clock is abnormal, notify chip fault.",
			  sal_card->card_no);

		sal_card->card_clk_abnormal_num = 0;
		sal_card->card_clk_abnormal_flag = SAL_CARD_CLK_ABNORMAL;

		/*notify chip fault */
		logic_id =
		    sal_comm_get_logic_id(sal_card->card_no, 0) & 0x00FFFFFF;

	}

	return;
}

#if 0
/*****************************************************************************
 函 数 名  : sal_check_card_slow_disk
 功能描述  : 检查整个卡带的慢盘
 输入参数  : void *v_pData  
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
s32 sal_check_card_slow_disk(struct sal_card * sal_card)
{
	unsigned long flag = 0;
	struct list_head *node = NULL;
	struct list_head *safe_node = NULL;
	struct sal_dev *dev = NULL;

	SAL_ASSERT(NULL != sal_card, return ERROR);

	/*加设备锁 */
	spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(node, safe_node, &sal_card->active_dev_list) {
		dev = SAS_LIST_ENTRY(node, struct sal_dev, dev_list);
		(void)sal_process_slow_disk(dev);
	}
	spin_unlock_irqrestore(&sal_card->card_dev_list_lock, flag);
	return OK;
}
#endif

/*----------------------------------------------*
 * routine function realize.*
 *----------------------------------------------*/
s32 sal_routine_func(void *argv_in, void *argv_out)
{
	u32 card_id = 0;
	struct sal_card *sal_card = NULL;
	unsigned long flag = 0;

	SAL_REF(argv_out);

	card_id = (u32) (u64) argv_in;
	sal_card = sal_get_card(card_id);
	if (NULL == sal_card) {
		SAL_TRACE(OSP_DBL_MAJOR, 670,
			  "get sal card failed by cardid:%d", card_id);
		return ERROR;
	}

	spin_lock_irqsave(&sal_card->card_lock, flag);
	if (SAL_IS_CARD_READY(sal_card)) {
		spin_unlock_irqrestore(&sal_card->card_lock, flag);

		/*error code handle */
		sal_bit_err_proc(sal_card);
#if 0
		/* 慢盘检测 */
		(void)sal_check_card_slow_disk(sal_card);
#endif

		/*chip fault detect. */
		sal_chip_fatal_examine(sal_card);

		/*clock abnormal detect. */
		sal_check_card_clock_abnor(sal_card);

		(void)sal_card->ll_func.chip_op(sal_card,
						SAL_CHIP_OPT_REQ_TIMEOUTCHECK,
						NULL);
	} else {
		spin_unlock_irqrestore(&sal_card->card_lock, flag);
	}

	sal_put_card(sal_card);
	return OK;
}

/*----------------------------------------------*
 * routine thread.*
 *----------------------------------------------*/
void sal_routine_timer(unsigned long card_no)
{
	u32 period = 1 * HZ;	/* interval time, unit:tick */
	struct sal_card *sal_card = NULL;
	unsigned long flag = 0;

	sal_card = sal_get_card((u32) card_no);
	if (NULL == sal_card) {
		SAL_TRACE(OSP_DBL_MAJOR, 671, "Get card failed by cardid:%ld",
			  card_no);
		return;
	}

	spin_lock_irqsave(&sal_card->card_lock, flag);
	if (SAL_IS_CARD_READY(sal_card)) {
		spin_unlock_irqrestore(&sal_card->card_lock, flag);
		sal_send_raw_ctrl_no_wait(sal_card->card_no,
					  (void *)(u64) card_no,
					  NULL, sal_routine_func);
	} else {
		spin_unlock_irqrestore(&sal_card->card_lock, flag);
	}

	/*restart timer */
	sal_add_timer((void *)&sal_card->routine_timer,
		      (unsigned long)sal_card->card_no, period,
		      sal_routine_timer);

	sal_put_card(sal_card);
	return;
}

/*----------------------------------------------*
 * fill phy option parameter.*
 *----------------------------------------------*/
s32 sal_set_phy_opt_info(struct sal_card * sal_card,
			 u8 phy_id, u32 phy_rate, enum sal_phy_op_type op_type)
{
	struct sal_event *event = NULL;
	struct sal_phy_op_param *op_info = NULL;

	event = SAS_KMALLOC(sizeof(struct sal_event), GFP_ATOMIC);
	if (NULL == event) {
		SAL_TRACE(OSP_DBL_MAJOR, 306,
			  "Card:%d allocate %ld byte memory failed",
			  sal_card->card_no, sizeof(struct sal_event));
		return ERROR;
	}

	memset(event, 0, sizeof(struct sal_event));


	op_info = (struct sal_phy_op_param *)event->comm_str;

	memset(op_info, 0, sizeof(struct sal_phy_op_param));
	op_info->phy_id = phy_id;
	op_info->card_id = sal_card->card_no;
	op_info->phy_rate_restore = false;
	op_info->phy_rate = phy_rate;
	op_info->op_type = op_type;

	sal_send_raw_ctrl_no_wait(op_info->card_id,
				  (void *)event, NULL, sal_operate_phy);
	return OK;
}

/*----------------------------------------------*
 * wide detect port phy rate..*
 *----------------------------------------------*/
u8 sal_check_wide_port_phy_rate(struct sal_card * sal_card, u32 phy_cnt,
				struct sal_phy_rate_chk_param * phy_rate)
{
	u32 i = 0;
	u32 phy_rate_sum[SAL_LINK_RATE_BUTT] = { 0 };
	u32 phy_num[SAL_LINK_RATE_BUTT] = { 0 };
	u32 phy_rate_final_sum = 0;
	u32 phy_final_num = 0;
	u32 phy_stable_rate = 0;
	char *link_str[] = {
		"Free",
		"1.5G",
		"3.0G",
		"6.0G",
		"12.0G",
		"Unknown"
	};
	SAL_REF(link_str[0]);


	for (i = 0; i < phy_cnt; i++) {
	
		if (SAL_LINK_RATE_12_0G <= phy_rate[i].phy_rate) {
			phy_rate_sum[SAL_LINK_RATE_12_0G] += 12;
			phy_num[SAL_LINK_RATE_12_0G]++;
		}

		if (SAL_LINK_RATE_6_0G <= phy_rate[i].phy_rate) {
			phy_rate_sum[SAL_LINK_RATE_6_0G] += 6;
			phy_num[SAL_LINK_RATE_6_0G]++;
		}

		if (SAL_LINK_RATE_3_0G <= phy_rate[i].phy_rate) {
			phy_rate_sum[SAL_LINK_RATE_3_0G] += 3;
			phy_num[SAL_LINK_RATE_3_0G]++;
		}
	}

	for (i = SAL_LINK_RATE_FREE; i < SAL_LINK_RATE_BUTT; i++)
	
		if ((phy_rate_final_sum < phy_rate_sum[i])
		    || ((phy_rate_final_sum == phy_rate_sum[i])
			&& (phy_final_num < phy_num[i]))) {
			phy_rate_final_sum = phy_rate_sum[i];
			phy_final_num = phy_num[i];
			phy_stable_rate = i;
		}

	SAL_TRACE(OSP_DBL_DATA, 672,
		  "Card:%d Port(First Phy:%d) current stable link rate:0x%x(%s)",
		  sal_card->card_no, phy_rate[0].phy_id, phy_stable_rate,
		  link_str[phy_stable_rate]);

	for (i = 0; i < phy_cnt; i++)
		if (phy_rate[i].phy_rate < phy_stable_rate) {

			SAL_TRACE(OSP_DBL_MINOR, 672,
				  "Card:%d phy:%d link rate:0x%x(%s) is less than port stable rate, close it",
				  sal_card->card_no, phy_rate[i].phy_id,
				  phy_rate[i].phy_rate,
				  link_str[phy_rate[i].phy_rate]);
			(void)sal_set_phy_opt_info(sal_card, phy_rate[i].phy_id,
						   sal_card->config.
						   phy_link_rate[phy_rate[i].
								 phy_id],
						   SAL_PHY_OPT_CLOSE);
		} else if (phy_rate[i].phy_rate > phy_stable_rate) {

			SAL_TRACE(OSP_DBL_MINOR, 673,
				  "Card:%d phy:%d link rate:0x%x(%s) is beyond than port stable rate, reduce phy rate",
				  sal_card->card_no, phy_rate[i].phy_id,
				  phy_rate[i].phy_rate,
				  link_str[phy_rate[i].phy_rate]);
			sal_card->phy[phy_rate[i].phy_id]->phy_reset_flag =
			    SAL_PHY_NO_RESET_FOR_DFR;
			(void)sal_set_phy_opt_info(sal_card, phy_rate[i].phy_id,
						   phy_stable_rate,
						   SAL_PHY_OPT_RESET);
		}

	return (u8) phy_stable_rate;
}

/*----------------------------------------------*
 * check wide port.*
 *----------------------------------------------*/
void sal_check_wide_port(struct sal_card *sal_card, struct sal_port *sal_port)
{
	u32 port_mask = 0;
	u32 i = 0;
	u32 phy_cnt = 0;
	struct sal_phy_rate_chk_param phy_rate_chk[SAL_MAX_PHY_NUM];
	struct sal_phy *sal_phy = NULL;
	u32 port_idx = sal_port->logic_id & 0xff;

	port_mask = sal_card->config.port_bitmap[port_idx];

	if (SAL_TRUE == sal_port->is_conn_high_jbod)	
		if (SAL_CHIP_TYPE_8072 == sal_card->chip_type)
			port_mask |=
			    sal_card->config.port_bitmap[(u32) (port_idx + 1)];


	for (i = 0; i < sal_card->phy_num; i++)
		if (port_mask & (1 << i)) {
			sal_phy = sal_card->phy[i];

			if (SAL_PHY_LINK_DOWN == sal_phy->link_status) {
				if (SAL_PHY_NEED_RESET_FOR_DFR ==
				    sal_phy->phy_reset_flag) {
					SAL_TRACE(OSP_DBL_MINOR, 549,
						  "Card:%d phy:%d link status is link down, need to reset it",
						  sal_card->card_no,
						  sal_phy->phy_id);

					sal_phy->phy_reset_flag =
					    SAL_PHY_NO_RESET_FOR_DFR;
					(void)sal_set_phy_opt_info(sal_card,
								   sal_phy->
								   phy_id,
								   sal_card->
								   config.
								   phy_link_rate
								   [sal_phy->
								    phy_id],
								   SAL_PHY_OPT_RESET);
				}
			} else if (SAL_PHY_CLOSED == sal_phy->link_status) {
				continue;
			} else {
				phy_rate_chk[phy_cnt].phy_rate =
				    sal_phy->link_rate;
				phy_rate_chk[phy_cnt].phy_id = sal_phy->phy_id;
				phy_rate_chk[phy_cnt].port_id =
				    sal_phy->port_id;
				phy_cnt++;
			}
		}

	if (0 != phy_cnt) {

#if 0
		sal_port->max_rate =
		    sal_check_wide_port_phy_rate(sal_card, phy_cnt,
						 phy_rate_chk);
#else
		phy_rate_chk[0].phy_id = 0;
		SAL_REF(phy_rate_chk[0].phy_id);
#endif

		sal_port->host_disc = true;
	}

}

/*----------------------------------------------*
 * light led according to port state.*
 *----------------------------------------------*/
void sal_port_led_execute(struct sal_card *sal_card)
{
	/*temp modify */
#if defined(PV660_ARM_SERVER)
	return;

#else

	u32 end_phy_id = 0;
	u32 start_phy_id = 0;
	u8 phy_cnt = 0;
	u8 link_rate = 0;
	u32 i = 0;
	u32 port_mask = 0;
	struct sal_phy *sal_phy = NULL;
	struct sal_led_op_param led_para;

	memset(&led_para, 0, sizeof(struct sal_led_op_param));

	for (i = 0; i < 4; i++) {

		port_mask = sal_card->config.port_bitmap[i];
		if (0 == port_mask)

			continue;


		phy_cnt = 0;

		start_phy_id = sal_first_phy_id_in_port(sal_card, i);

		end_phy_id = start_phy_id + 4;
		SAL_ASSERT(SAL_MAX_PHY_NUM > end_phy_id, return);

		link_rate = sal_card->phy[start_phy_id]->link_rate;
		for (start_phy_id = start_phy_id; start_phy_id < end_phy_id;
		     start_phy_id++) {
			sal_phy = sal_card->phy[start_phy_id];
			if (link_rate == sal_phy->link_rate)
				phy_cnt++;
		}


		if (SAL_LOGIC_PORT_WIDTH == phy_cnt) {

			if (SAL_LINK_RATE_12_0G == link_rate)
				led_para.all_port_speed_led[i] =
				    SAL_LED_HIGH_SPEED;
			else if (SAL_LINK_RATE_FREE == link_rate)
				led_para.all_port_speed_led[i] = SAL_LED_OFF;
			else
				led_para.all_port_speed_led[i] =
				    SAL_LED_LOW_SPEED;
		} else {

			SAL_TRACE(OSP_DBL_INFO, 534,
				  "(warning) red led,start phy id:%d",
				  start_phy_id);
			led_para.all_port_speed_led[i] = SAL_LED_FAULT;
		}
	}

	led_para.all_port_op = SAL_ALL_PORT_LED_NEED_OPT;
	/*Quark_PortOperation */
	(void)sal_card->ll_func.port_op(sal_card, SAL_PORT_OPT_LED,
					(void *)&led_para);

	return;
#endif
}

/*----------------------------------------------*
 * light led according to work queue.*
 *----------------------------------------------*/
void sal_port_led_func(struct work_struct *work)
{
	struct sal_card *sal_card = NULL;

	SAL_ASSERT(NULL != work, return);

	sal_card = container_of(work, struct sal_card, port_led_work.work);

	SAL_ASSERT(NULL != sal_card, return);

	if (!SAL_IS_CARD_READY(sal_card)) {
		SAL_TRACE(OSP_DBL_MINOR, 532,
			  "Card:%d(flag:0x%x) is not active", sal_card->card_no,
			  sal_card->flag);
		return;
	}

	SAL_TRACE(OSP_DBL_DATA, 532, "Card:%d ready to turn led.",
		  sal_card->card_no);

	/*light led according to port state. */
	sal_port_led_execute(sal_card);

	return;
}

/*----------------------------------------------*
 * wide port speed detect and adjust.*
 *----------------------------------------------*/
void sal_wide_port_opt_func(unsigned long timer_argv)
{
	u32 port_idx = 0;
	unsigned long flag = 0;
	struct sal_card *sal_card = NULL;
	u32 port_mask = 0;
	struct sal_port *sal_port = NULL;
	u32 phy_index = 0;
	u32 card_no = 0;
	bool port_should_do_disc[SAL_MAX_PHY_NUM];

	card_no = (u32) timer_argv;

	if (card_no >= SAL_MAX_CARD_NUM) {
		SAL_TRACE(OSP_DBL_MAJOR, 531, "Card id:%d invalid", card_no);
		return;
	}

	sal_card = sal_get_card(card_no);
	if (NULL == sal_card) {
		SAL_TRACE(OSP_DBL_MAJOR, 532, "get card failed by cardid:%d",
			  card_no);
		return;
	}

	for (port_idx = 0; port_idx < SAL_MAX_PHY_NUM; port_idx++)
		port_should_do_disc[port_idx] = false;

	spin_lock_irqsave(&sal_card->card_lock, flag);
	for (port_idx = 0; port_idx < sal_card->phy_num; port_idx++) {

#if 0
		/* 遍历物理端口 */
		port_mask = sal_card->config.port_bitmap[port_idx];
		if (0 == port_mask) {	/*配置文件中没有此端口 */
			continue;
		}
#else
		SAL_REF(port_mask);
#endif


		sal_port = sal_card->port[port_idx];
		if (SAL_PHY_UP_IN_PORT == sal_port->phy_up_in_port) {
			sal_card->port[port_idx]->phy_up_in_port =
			    SAL_NO_PHY_UP_IN_PORT;
			sal_check_wide_port(sal_card, sal_port);	/* wide detect. */
			if (true == sal_port->host_disc)

				port_should_do_disc[port_idx] = true;
		}
	}
	spin_unlock_irqrestore(&sal_card->card_lock, flag);

	/*after lookup , start discover. */
	for (port_idx = 0; port_idx < sal_card->phy_num; port_idx++) {
		if (true != port_should_do_disc[port_idx])
			continue;

		sal_port = sal_card->port[port_idx];
		phy_index = sal_find_link_phy_in_port_by_rate(sal_port);
		if (SAL_INVALID_PHYID != phy_index) {
			sal_start_disc(sal_port, sal_card->phy[phy_index]);
			sal_port->host_disc = false;
		} else {
			SAL_TRACE(OSP_DBL_MAJOR, 532,
				  "Card:%d port:%d invalid phy_id:%d", card_no,
				  port_idx, phy_index);
		}
	}

	sal_put_card(sal_card);
	return;
}

void sal_ll_port_ref_oper(struct sal_card *sal_card, struct sal_port *sal_port,
			  enum sal_refcnt_op_type ref_op_type)
{
	struct sal_ll_port_ref_op op;

	if (NULL != sal_port->card->ll_func.port_op) {
		memset(&op, 0, sizeof(struct sal_ll_port_ref_op));
		op.port_idx = sal_port->index;
		op.port_ref_op_type = ref_op_type;

		(void)sal_port->card->ll_func.port_op(sal_card,
						      SAL_PORT_OPT_REF_INC_OR_DEC,
						      (void *)&op);
	}

	return;
}

/*----------------------------------------------*
 * common entrance function of light led in sal-layer.*
 *----------------------------------------------*/
void sal_comm_led_operation(struct sal_card *sal_card)
{
	unsigned long flag = 0;

	spin_lock_irqsave(&sal_card->card_lock, flag);
	// sal_port_led_func
	if (!delayed_work_pending(&sal_card->port_led_work))
		queue_delayed_work(sal_wq, &sal_card->port_led_work,
				   (s32) msecs_to_jiffies(120));
	spin_unlock_irqrestore(&sal_card->card_lock, flag);
	return;
}

/*----------------------------------------------*
 * get free event.*
 *----------------------------------------------*/
struct sal_event *sal_get_free_event(struct sal_card *sal_card)
{
	unsigned long flag = 0;
	struct sal_event *info = NULL;

	SAL_ASSERT(NULL != sal_card, return NULL);

	spin_lock_irqsave(&sal_card->card_event_lock, flag);
	if (!SAS_LIST_EMPTY(&sal_card->event_free_list)) {
		info =
		    SAS_LIST_ENTRY(sal_card->event_free_list.next,
		    struct sal_event, list);

		SAS_LIST_DEL_INIT(&info->list);
		memset(info, 0, sizeof(struct sal_event));
	}
	spin_unlock_irqrestore(&sal_card->card_event_lock, flag);

	return info;
}

/*----------------------------------------------*
 * put free event.*
 *----------------------------------------------*/
void sal_put_free_event(struct sal_card *sal_card, struct sal_event *sal_event)
{
	unsigned long flag = 0;

       spin_lock_irqsave(&sal_card->card_event_lock, flag);
	SAS_LIST_ADD_TAIL(&sal_event->list, &sal_card->event_free_list);
	spin_unlock_irqrestore(&sal_card->card_event_lock, flag);

	return;
}
#if 0 /* delete no used function */
/*BEGIN 问题单号:DTS2013031400862 add by chengwei 90006017 重新触发disc*/
/*****************************************************************************
 函 数 名  : sal_direct_disk_link_reset
 功能描述  : 直连磁盘linkreset
 输入参数  :  
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
s32 sal_direct_disk_link_reset(struct sal_card * sal_card,
			       struct sal_port * sal_port)
{
	struct sal_local_phy_ctrl_param phy_info;
	u32 phy_id = 0;
	s32 ret = ERROR;

	/*找到本端口(fw)第一个呈link up状态的phy */
	phy_id =
	    sal_find_first_link_phy_in_port(sal_card, sal_port->logic_id,
					    sal_port->port_id);
	if (SAL_INVALID_PHYID == phy_id) {
		SAL_TRACE(OSP_DBL_MAJOR, 871,
			  "Card:%d port:0x%x can't find phy of link up",
			  sal_card->card_no, sal_port->logic_id);
		return ERROR;
	}

	memset(&phy_info, 0, sizeof(struct sal_local_phy_ctrl_param));
	phy_info.card_id = sal_card->card_no;
	phy_info.op_code = SAL_LOCAL_PHY_LINK_RESET;
	phy_info.phy_id = phy_id;

	ret = sal_comm_local_phy_control((void *)&phy_info, NULL);

	return ret;

}
#endif

/*----------------------------------------------*
 * interrupt event , modify exception  by linkreset phy. *
 *----------------------------------------------*/
void sal_intr_event2_reset_phy(struct sal_card *sal_card, u32 phy_id)
{
	s32 ret = ERROR;

	ret = sal_set_phy_opt_info(sal_card,
				   (u8) phy_id,
				   sal_card->config.phy_link_rate[phy_id],
				   SAL_PHY_OPT_RESET);

	if (OK != ret)
		SAL_TRACE(OSP_DBL_MAJOR, 244, "Card:%d reset phy:%d failed",
			  sal_card->card_no, phy_id);

	return;
}

EXPORT_SYMBOL(sal_intr_event2_reset_phy);

/*----------------------------------------------*
 * clear all port resource.*
 *----------------------------------------------*/
s32 sal_clr_all_port_rsc(struct sal_card * sal_card)
{
	u32 i = 0;

	for (i = 0; i < sal_card->phy_num; i++)
		if (SAL_PORT_FREE != sal_card->port[i]->status) {
			(void)sal_set_port_dev_state(sal_card,
						     sal_card->port[i],
						     SAL_DEV_EVENT_MISS);
			(void)sal_set_port_dev_state(sal_card,
						     sal_card->port[i],
						     SAL_DEV_EVENT_DEREG);
			(void)sal_set_port_dev_state(sal_card,
						     sal_card->port[i],
						     SAL_DEV_EVENT_DEREG_OK);


			sal_port_down_notify(sal_card->card_no,
					     sal_card->port[i]->port_id);

			sal_del_port_report(sal_card, sal_card->port[i]);

			sal_re_init_port(sal_card, sal_card->port[i]);
		}

	return OK;
}

EXPORT_SYMBOL(sal_clr_all_port_rsc);

/*----------------------------------------------*
 * handle all port resource.*
 *----------------------------------------------*/
s32 sal_handle_all_port_rsc_clean(struct sal_card * sal_card,
				  struct sal_event * event)
{
	s32 ret = ERROR;

	SAL_REF(event);

	SAL_TRACE(OSP_DBL_INFO, 577,
		  "Card:%d clear all port rsc,event time:%dms",
		  sal_card->card_no, jiffies_to_msecs(jiffies - event->time));

	ret = sal_clr_all_port_rsc(sal_card);
	return ret;
}

void sal_remove_all_dev(u8 card_id)
{
	struct list_head *node = NULL;
	struct list_head *safe_node = NULL;
	struct sal_dev *dev = NULL;
	unsigned long flag = 0;
	struct sal_card* sal_card;   
    
    sal_card = sal_get_card(card_id);
	/* report out all dev of this card */
	spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(node, safe_node, &sal_card->active_dev_list) {
		dev = SAS_LIST_ENTRY(node, struct sal_dev, dev_list);
		if (dev->card == sal_card) {
			spin_unlock_irqrestore(&sal_card->card_dev_list_lock,
					       flag);
            
        	/* report dev out */
        	sal_del_scsi_dev(dev->card->host_id, dev->bus_id, dev->scsi_id, dev->lun_id);
        
			spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
		}
	}
	spin_unlock_irqrestore(&sal_card->card_dev_list_lock, flag);
    sal_put_card(sal_card);
    
	/* put all sal dev */
    sal_put_all_dev(card_id);
    
	return;
}

EXPORT_SYMBOL(sal_remove_all_dev);

