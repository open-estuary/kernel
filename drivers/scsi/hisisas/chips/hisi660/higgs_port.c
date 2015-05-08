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
#include "higgs_common.h"
#include "higgs_dev.h"
#include "higgs_peri.h"
#include "higgs_phy.h"
#include "higgs_port.h"

/*----------------------------------------------*
 *  * outer variable type define                                *                                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 *    outer function type define                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * inner function type define                                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * global variable                                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * module  variable                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * start work for all port.                                   *
 *----------------------------------------------*/
s32 higgs_all_port_start_work(struct sal_card *sal_card)
{
	/*parameter check */
	HIGGS_ASSERT(NULL != sal_card, return ERROR);

	/* config serdes parameter for all PHY */
	if (OK != higgs_init_serdes_param(sal_card->drv_data)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4611, "Card:%d init EQ param failed",
			    sal_card->card_no);
		return ERROR;
	}

	/* config identify address frame of all PHY*/
	if (OK != higgs_init_identify_frame(sal_card->drv_data)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4611,
			    "Card:%d init identify frame failed",
			    sal_card->card_no);
		return ERROR;
	}

	/*open PHY of port , according to product model and cable state in position. */
	higgs_open_phy_own_wire(sal_card);

	return OK;
}

/*----------------------------------------------*
 *convert sal-layer opcode into low-layer opcode.                                   *
 *----------------------------------------------*/
u8 higgs_port_ctrl_op_trans(enum sal_port_control sal_port_ctrl)
{
	u8 ll_op = 0;

	switch (sal_port_ctrl) {
	case SAL_PORT_CTRL_E_CLEAN_UP:
		ll_op = 0;	/*HIGGS_PORT_CTRL_CLEAN_UP; */
		break;
		/*below operation don't support in higgs layer  */
	case SAL_PORT_CTRL_E_SET_SMP_WIDTH:
	case SAL_PORT_CTRL_E_SET_PORT_RECY_TIME:
	case SAL_PORT_CTRL_E_ABORT_IO:
	case SAL_PORT_CTRL_E_SET_PORT_RESET_TIME:
	case SAL_PORT_CTRL_E_HARD_RESET:
	case SAL_PORT_CTRL_E_STOP_PORT_RECY_TIMER:
	default:
		ll_op = 0xff;
		break;
	}

	return ll_op;
}

#if 0
/*****************************************************************************
 函 数 名  : higgs_port_ref_count_opt
 功能描述  :
 输入参数  :
 输出参数  : ret
 返 回 值  : s32
*****************************************************************************/
s32 higgs_port_ref_count_opt(struct higgs_card * ll_card,
			     struct sal_port * v_pstSALPort,
			     enum sal_refcnt_op_type v_enOptype)
{
//      struct higgs_port *pstLLPort = NULL;
//      pstLLPort = (struct higgs_port *)v_pstSALPort->ll_port;

	HIGGS_REF(ll_card);
	HIGGS_REF(v_pstSALPort);
	switch (v_enOptype) {
	case SAL_REFCOUNT_INC_OPT:
		return OK;	//原子量都返回ok

	case SAL_REFCOUNT_DEC_OPT:
		return OK;
	case SAL_REFCOUNT_BUTT:
	default:
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_MAJOR, HZ, 4432,
				     "invalid optype:%d", v_enOptype);
		break;
	}
	return ERROR;
}
#endif

/*----------------------------------------------*
 * port operation function.                                   *
 *----------------------------------------------*/
s32 higgs_port_operation(struct sal_card * sal_card,
			 enum sal_port_op_type port_op, void *data)
{
	s32 ret = ERROR;
	u32 tmp_port_id = 0;
	u32 port_idx = 0;
	struct higgs_card *ll_card = NULL;
	u8 tmp_port_op_code = 0;
	struct sal_local_port_ctl *tmp = NULL;
	struct sal_led_op_param *led_param = NULL;
//    struct sal_ll_port_ref_op *pstPortRefOpt = NULL;
	u32 *logic_port_id = NULL;

	HIGGS_ASSERT(NULL != sal_card, return ERROR);
	if (SAL_PORT_OPT_ALL_START_WORK != port_op)
		HIGGS_ASSERT(NULL != data, return ERROR);

	ll_card = (struct higgs_card *)sal_card->drv_data;
	if (NULL == ll_card) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4432, "Card:%d LL card is NULL!",
			    sal_card->card_no);
		return ERROR;
	}

	switch (port_op) {
	case SAL_PORT_OPT_LOCAL_CONTROL:
		tmp = (struct sal_local_port_ctl *)data;
		tmp_port_id = tmp->port_id;
		tmp_port_op_code = higgs_port_ctrl_op_trans(tmp->op_code);
		if (0xff == tmp_port_op_code) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4432,
				    "Card:%d port id:%d unknown opcode:%d",
				    sal_card->card_no, tmp_port_id,
				    tmp->op_code);
			return ERROR;
		}
		ret = higgs_dereg_port(sal_card, tmp_port_id, tmp_port_op_code);
		break;
	case SAL_PORT_OPT_DELETE:
		port_idx = *((u32 *) data);
		if (!(port_idx < HIGGS_MAX_PORT_NUM)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4432,
				    "Card:%d invalid port index:%d!",
				    sal_card->card_no, port_idx);
			return ERROR;
		}
		ret = higgs_del_port(sal_card->port[port_idx]);
		break;
	case SAL_PORT_OPT_ALL_START_WORK:
		ret = higgs_all_port_start_work(sal_card);
		break;
	case SAL_PORT_OPT_LED:

		led_param = (struct sal_led_op_param *)data;
		ret = higgs_led_operation(sal_card, led_param);
		break;
	case SAL_PORT_OPT_REF_INC_OR_DEC:
		ret = OK;	/*atomic operation return OK */
		break;
	case SAL_PORT_OPT_DETECT_CABLE_ONOFF:
		logic_port_id = (u32 *) data;
		ret = higgs_check_sfp_on_port(sal_card, logic_port_id);
		break;
	case SAL_PORT_OPT_BUTT:
	default:
		HIGGS_TRACE(OSP_DBL_MINOR, 4617, "Card:%d unknown opcode:%d ",
			    sal_card->card_no, port_op);
		break;
	}

	return ret;
}

/*----------------------------------------------*
 * delete port request function.                                   *
 *----------------------------------------------*/
s32 higgs_del_port(struct sal_port * sal_port)
{
	struct higgs_card *ll_card = NULL;
	struct higgs_port *tmp_port = NULL;

	HIGGS_ASSERT(NULL != sal_port, return ERROR);

	tmp_port = (struct higgs_port *)(sal_port->ll_port);
	ll_card = (struct higgs_card *)sal_port->card->drv_data;
	higgs_init_port_src(ll_card, tmp_port);
	return OK;
}

/*----------------------------------------------*
 * initialize PORT function.                                   *
 *----------------------------------------------*/
void higgs_init_port_src(struct higgs_card *ll_card, struct higgs_port *port)
{
	unsigned long flag = 0;

	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(NULL != port, return);
	spin_lock_irqsave(&ll_card->card_lock, flag);
	HIGGS_TRACE(OSP_DBL_INFO, 4422,
		    "Card:%d release LL port:%d(index:%d) rsc",
		    ll_card->card_id, port->port_id, port->index);
	port->status = HIGGS_PORT_FREE;
	port->port_id = HIGGS_INVALID_PORT_ID;
	port->last_phy = HIGGS_INVALID_PHY_ID;
	spin_unlock_irqrestore(&ll_card->card_lock, flag);
}

/*----------------------------------------------*
 * obtain free port function.                                   *
 *----------------------------------------------*/
struct higgs_port *higgs_get_free_port(struct higgs_card *ll_card)
{
	u32 i = 0;
	unsigned long flag = 0;

	HIGGS_ASSERT(NULL != ll_card, return NULL);

	spin_lock_irqsave(&ll_card->card_lock, flag);
	for (i = 0; i < HIGGS_MAX_PORT_NUM; i++) {
		HIGGS_TRACE(OSP_DBL_DATA, 4368,
			    "Card:%d Port index:%d Status:%d", ll_card->card_id,
			    i, ll_card->port[i].status);
		if (HIGGS_PORT_FREE == ll_card->port[i].status) {
			HIGGS_TRACE(OSP_DBL_DATA, 4368,
				    "get free port index:%d", i);
			ll_card->port[i].status = HIGGS_PORT_INVALID;

			spin_unlock_irqrestore(&ll_card->card_lock, flag);
			return (&ll_card->port[i]);
		}
	}
	spin_unlock_irqrestore(&ll_card->card_lock, flag);

	return NULL;
}

/*----------------------------------------------*
 * report port id .                                   *
 *----------------------------------------------*/

void higgs_notify_port_id(struct sal_card *sal_card)
{
	u32 i = 0;
	u32 port_logic_id = 0;

	for (i = 0; i < HIGGS_MAX_PORT_NUM; i++) {
		if (0 == sal_card->config.port_logic_id[i])
			continue;

		port_logic_id = sal_card->config.port_logic_id[i];

		//(void)sal_event_notify(port_logic_id, SAL_DRV_DEVT_IN);
	}

	return;
}

/*----------------------------------------------*
 * look up corresponding PORT according to PortId from chip.                                   *
 *----------------------------------------------*/
struct higgs_port *higgs_get_port_by_port_id(struct higgs_card *ll_card,
					     u32 chip_port_id)
{
	struct higgs_port *tmp_port = NULL;
	u32 i = 0;
	unsigned long flag = 0;

	spin_lock_irqsave(&ll_card->card_lock, flag);
	for (i = 0; i < HIGGS_MAX_PORT_NUM; i++) {
		if (HIGGS_PORT_FREE == ll_card->port[i].status)
			continue;

		if (HIGGS_PORT_DOWN == ll_card->port[i].status)
			continue;

		if (chip_port_id == ll_card->port[i].port_id) {
			tmp_port = &ll_card->port[i];	/* evaluate NULL here */
			break;
		}
	}
	spin_unlock_irqrestore(&ll_card->card_lock, flag);

	return tmp_port;
}

/*----------------------------------------------*
 * report port id to FRAME.                                   *
 *----------------------------------------------*/
s32 higgs_set_up_port_id(struct higgs_card * ll_card, u32 * logic_port_id)
{
	u32 drv_type;
	u32 board_type;
	u32 board_id;
	u32 phy_id;
	u32 tmp_port;
	u32 tmp_logic_port_id;
	struct sal_card *sal_card;
	struct sal_config *sal_cfg;
	struct higgs_config_info *ll_cfg_info;
	bool is_reported;

	/* parameter check */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(NULL != ll_card->sal_card, return ERROR);

	/* report port */
	sal_card = ll_card->sal_card;
	sal_cfg = &sal_card->config;
	ll_cfg_info = &ll_card->card_cfg;

	drv_type = 0;
	board_type = SAL_GET_BOARDTYPE(sal_card->position);
	board_id = SAL_GET_SLOTID(sal_card->position);

	/*traverse PHY */
	is_reported = false;
	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		sal_cfg->phy_addr_high[phy_id] =
		    ll_cfg_info->phy_addr_high[phy_id];
		sal_cfg->phy_addr_low[phy_id] =
		    ll_cfg_info->phy_addr_low[phy_id];
	}

	/* traverse PORT */
	for (tmp_port = 0; tmp_port < HIGGS_MAX_PORT_NUM; tmp_port++) {
		sal_cfg->port_bitmap[tmp_port] =
		    ll_cfg_info->port_bitmap[tmp_port];

		if (0 == (sal_cfg->port_bitmap[tmp_port] & 0xffff))
			continue;	

		tmp_logic_port_id =
		    DRV_BUILD_PORT_ID(DRV_FACTOR_PMC, drv_type, board_type,
				      board_id, tmp_port);
		sal_cfg->port_logic_id[tmp_port] = tmp_logic_port_id;

		HIGGS_TRACE(OSP_DBL_INFO, 4617,
			    "Card:%d set up logic port:0x%x succeed",
			    ll_card->card_id, tmp_logic_port_id);

		*logic_port_id = tmp_logic_port_id;

		if (!is_reported)
			is_reported = true;
	}

	return is_reported ? OK : ERROR;
}
#if 0 /* delete by chenqilin */

/*****************************************************************************
 函 数 名  : higgs_notify_sal_wire_hotplug
 功能描述  : 通知SAL 端口线缆拔插事件
 输入参数  : struct higgs_card *ll_card,
		u32 ll_port_id
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
s32 higgs_notify_sal_wire_hotplug(struct higgs_card * ll_card, u32 ll_port_id)
{
	u32 gpio_rsp_val = 0;
	u32 logic_port_id = 0;
	struct sal_card *sal_card = NULL;

	/* 参数检查 */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(ll_port_id < HIGGS_MAX_PORT_NUM, return ERROR);

	/* 调用SAL_AddEventToWireHandler通知SAL层  */
	sal_card = ll_card->sal_card;
	/* DTS2014122503568: 在PV660/Hi1610平台, GPIO值虽然无意义，但是SAL层要求不同端口的值有差异，否则邻近的事件可能会被过滤 */
#if 0
	uiReadGpioRspVal = 0xFFFFFFFF;	/* 在PV660/Hi1610平台, GPIO值废弃 */
#else
	gpio_rsp_val = 0xFFFFFF00 + ll_port_id;
#endif
	/* DTS2014122503568 */
	logic_port_id = sal_card->config.port_logic_id[ll_port_id];

	return sal_add_event_to_wire_handler(sal_card, gpio_rsp_val,
					     logic_port_id);
}
#endif

/*----------------------------------------------*
 * mark low-layer PORT state , when phy up.                                *
 *----------------------------------------------*/
void higgs_mark_ll_port_on_phy_up(struct higgs_card *ll_card,
				  struct higgs_port *ll_port,
				  u32 phy_id, u32 chip_port_id)
{
	unsigned long flag = 0;

	/* parameter check */
	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(NULL != ll_port, return);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return);
	HIGGS_ASSERT(chip_port_id < HIGGS_MAX_PORT_NUM, return);

	/* add lock */
	spin_lock_irqsave(&ll_card->card_lock, flag);

	ll_port->port_id = chip_port_id;
	ll_port->status = HIGGS_PORT_UP;
	ll_port->phy_bitmap |= ((u32) 0x1 << phy_id);

	/* unlock */
	spin_unlock_irqrestore(&ll_card->card_lock, flag);
}

/*----------------------------------------------*
 * mark low-layer PORT state , when phy down.                                *
 *----------------------------------------------*/
void higgs_mark_ll_port_on_phy_down(struct higgs_card *ll_card,
				    struct higgs_port *ll_port, u32 phy_id)
{
	unsigned long flag = 0;

	/* parameter check */
	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(NULL != ll_port, return);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return);

	/* add lock */
	spin_lock_irqsave(&ll_card->card_lock, flag);

	ll_port->phy_bitmap &= ~((u32) 0x1 << phy_id);
	if (0 == ll_port->phy_bitmap) {
		ll_port->port_id = HIGGS_INVALID_PORT_ID;
		ll_port->last_phy = phy_id;
		ll_port->status = HIGGS_PORT_DOWN;
	}

	/* unlock */
	spin_unlock_irqrestore(&ll_card->card_lock, flag);
}

/*----------------------------------------------*
 * mark low-layer PORT state , when StopPhyRsp.                                *
 *----------------------------------------------*/
void higgs_mark_ll_port_on_stop_phy_rsp(struct higgs_card *ll_card,
					struct higgs_port *ll_port, u32 phy_id)
{
	unsigned long flag = 0;

	/* parameter check */
	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(NULL != ll_port, return);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return);

	/* add lock */
	spin_lock_irqsave(&ll_card->card_lock, flag);

	ll_port->phy_bitmap &= ~((u32) 0x1 << phy_id);
	if (0 == ll_port->phy_bitmap) {
		ll_port->port_id = HIGGS_INVALID_PORT_ID;
		ll_port->last_phy = phy_id;
		ll_port->status = HIGGS_PORT_DOWN;
	}

	/* unlock */
	spin_unlock_irqrestore(&ll_card->card_lock, flag);
}
