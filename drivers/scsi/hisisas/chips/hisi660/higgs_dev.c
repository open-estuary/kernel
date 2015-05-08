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

#include "higgs_common.h"
#include "higgs_pv660.h"
#include "higgs_dev.h"

#define HIGGS_DEV_ITCT_VALID(itct, value)   \
    {((struct higgs_itct_info *)(itct))->base_info.valid = (value);}



/**
 * higgs_get_free_dev - get free device
 * @ll_card: per card object
 *
 */
struct higgs_device *higgs_get_free_dev(struct higgs_card *ll_card)
{
	struct higgs_device *ll_dev = NULL;
	unsigned long flag = 0;

	spin_lock_irqsave(&ll_card->card_free_dev_lock, flag);
	if (SAS_LIST_EMPTY(&ll_card->card_free_dev_list)) {
		spin_unlock_irqrestore(&ll_card->card_free_dev_lock, flag);
		return NULL;
	}

	ll_dev =
	    SAS_LIST_ENTRY(ll_card->card_free_dev_list.next,
			   struct higgs_device, list_node);
	SAS_LIST_DEL_INIT(&ll_dev->list_node);
	ll_dev->valid = HIGGS_DEVICE_VALID;
	spin_unlock_irqrestore(&ll_card->card_free_dev_lock, flag);

	return ll_dev;
}


u32 higgs_get_reg_dev_type(struct sal_dev * dev)
{
	u32 reg_dev_type = HIGGS_REG_DEVICE_INVALID;

	HIGGS_ASSERT(NULL != dev, return HIGGS_REG_DEVICE_INVALID);

	if ((SAL_MODE_DIRECT == dev->port->link_mode)
	    && (SAL_DEV_TYPE_STP == dev->dev_type))
		return HIGGS_REG_DEVICE_SATA;

	switch (dev->dev_type) {
	case SAL_DEV_TYPE_STP:
		reg_dev_type = HIGGS_REG_DEVICE_STP;
		break;
	case SAL_DEV_TYPE_SSP:
	case SAL_DEV_TYPE_SMP:
		reg_dev_type = HIGGS_REG_DEVICE_SAS;
		break;
	case SAL_DEV_TYPE_BUTT:
	default:
		HIGGS_TRACE(OSP_DBL_MINOR, 100,
			    "Disk:0x%llx unknown device type:%d", dev->sas_addr,
			    dev->dev_type);
	}

	return reg_dev_type;
}

void higgs_recycle_dev(struct higgs_card *ll_card, struct higgs_device *ll_dev)
{
	unsigned long flag = 0;

	HIGGS_ASSERT(NULL != ll_dev, return);
	HIGGS_ASSERT(NULL != ll_card, return);
	spin_lock_irqsave(&ll_card->card_free_dev_lock, flag);

	ll_dev->valid = HIGGS_DEVICE_INVALID;
	ll_dev->up_dev = NULL;
	ll_dev->dev_type = 0;
	ll_dev->err_info0 = 0;
	ll_dev->err_info1 = 0;
	ll_dev->err_info2 = 0;
	ll_dev->err_info3 = 0;
	ll_dev->last_print_err_jiff = 0;

	HIGGS_DEV_ITCT_VALID(&ll_card->io_hw_struct_info.
			     itct_base[ll_dev->dev_id], 0);

	SAS_LIST_ADD_TAIL(&ll_dev->list_node, &ll_card->card_free_dev_list);

	spin_unlock_irqrestore(&ll_card->card_free_dev_lock, flag);

	return;
}


void higgs_set_itct_info(struct higgs_itct_info *itct_info,
			 struct higgs_device *ll_dev)
{
	struct sal_dev *sal_dev = NULL;
	struct higgs_port *port = NULL;

	HIGGS_ASSERT(NULL != itct_info, return);
	HIGGS_ASSERT(NULL != ll_dev, return);

	sal_dev = (struct sal_dev *)ll_dev->up_dev;
	port = sal_dev->port->ll_port;

	memset(itct_info, 0, sizeof(struct higgs_itct_info));

	itct_info->base_info.dev_type = ll_dev->dev_type;
	itct_info->base_info.valid = 1;
	itct_info->base_info.break_reply_enable = 0;
	itct_info->base_info.awt_control = 1;

	/*0x08£º1.5Gbps;0x09£º3Gbps;0x0A£º6Gbps;0x0B£º12Gbps. */
	itct_info->base_info.max_conn_rate = sal_dev->link_rate_prot;
	itct_info->base_info.valid_link_num = 1;
	itct_info->base_info.port_id =
	    (u16) port->port_id & HIGGS_CHIP_PORT_ID_TO_DEV_MASK;
	itct_info->base_info.smp_timeout = 0;
	itct_info->base_info.max_burst_byte = 0;

	itct_info->sas_addr = sal_dev->sas_addr;

	itct_info->ploc_timerset.it_nexus_loss_time = HIGGS_I_T_NEXOUS_TIMEOUT;
	itct_info->ploc_timerset.bus_inactive_limit_time = 0xFF00;
	itct_info->ploc_timerset.max_connect_limit_time = 0xFF00;
	itct_info->ploc_timerset.reject_open_limit_time = 0xFF00;

	return;
}

/**
 * higgs_reg_dev - register device
 * @sal_dev: sal card object
 *
 */
s32 higgs_reg_dev(struct sal_dev * sal_dev)
{
	struct higgs_device *ll_dev = NULL;
	struct higgs_card *ll_card = NULL;
	struct higgs_itct_info *itct_infos = NULL;

	HIGGS_ASSERT(NULL != sal_dev, return ERROR);

	ll_card = (struct higgs_card *)sal_dev->card->drv_data;
	ll_dev = higgs_get_free_dev(ll_card);
	if (NULL == ll_dev) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 100,
			    "Card:%d get free LL dev failed", ll_card->card_id);
		return ERROR;
	}

	HIGGS_ASSERT(ll_dev->dev_id < HIGGS_MAX_DEV_NUM);
	ll_dev->up_dev = sal_dev;
	sal_dev->ll_dev = ll_dev;

	ll_dev->dev_type = higgs_get_reg_dev_type(sal_dev);
	itct_infos = &(ll_card->io_hw_struct_info.itct_base[ll_dev->dev_id]);

	higgs_set_itct_info(itct_infos, ll_dev);

	HIGGS_TRACE(OSP_DBL_INFO, 100,
		    "Reg dev addr:0x%llx,dev id:0x%x,%d reg (OK)",
		    sal_dev->sas_addr, ll_dev->dev_id, ll_dev->dev_id);

	SAS_ATOMIC_INC(&(sal_dev->port->dev_num));

	/*wake up sas layer */
	sal_add_dev_cb(sal_dev, SAL_STATUS_SUCCESS);
	return OK;
}

/**
 * higgs_dereg_dev - deregister device
 * @sal_dev: sal card object
 *
 */
s32 higgs_dereg_dev(struct sal_dev * sal_dev)
{
	struct higgs_device *ll_dev = NULL;
	struct higgs_card *ll_card = NULL;
	u32 reg_value = 0xffff;
	unsigned long flag = 0;

	HIGGS_ASSERT(NULL != sal_dev, return ERROR);
	ll_card = (struct higgs_card *)sal_dev->card->drv_data;
	ll_dev = (struct higgs_device *)sal_dev->ll_dev;
	if (NULL == ll_dev) {
		HIGGS_TRACE(OSP_DBL_MINOR, 100,
			    "Card:%d disk:0x%llx is not exit", ll_card->card_id,
			    sal_dev->sas_addr);
		return ERROR;
	}

	HIGGS_TRACE(OSP_DBL_INFO, 100,
		    "Card:%d Port(FW):%d will deregister dev:%llx(sal dev:%p,LL dev:%p)",
		    ll_card->card_id, sal_dev->port->port_id, sal_dev->sas_addr,
		    sal_dev, ll_dev);

	spin_lock_irqsave(&ll_card->card_lock, flag);
	reg_value =
	    HIGGS_GLOBAL_REG_READ(ll_card,
				  HISAS30HV100_GLOBAL_REG_CFG_AGING_TIME_REG);
	reg_value = reg_value | PBIT0;
	HIGGS_GLOBAL_REG_WRITE(ll_card,
			       HISAS30HV100_GLOBAL_REG_CFG_AGING_TIME_REG,
			       reg_value);
	spin_unlock_irqrestore(&ll_card->card_lock, flag);
	/*release local itct table ,
	  must keep value 1 at least 10 AXI cycle ,than set vale 0.*/
	HIGGS_UDELAY(HIGGS_REG_ITCT_INVALID_TIMER);	
	spin_lock_irqsave(&ll_card->card_lock, flag);
	reg_value =
	    HIGGS_GLOBAL_REG_READ(ll_card,
				  HISAS30HV100_GLOBAL_REG_CFG_AGING_TIME_REG);
	reg_value = reg_value & NBIT0;
	HIGGS_GLOBAL_REG_WRITE(ll_card,
			       HISAS30HV100_GLOBAL_REG_CFG_AGING_TIME_REG,
			       reg_value);
	spin_unlock_irqrestore(&ll_card->card_lock, flag);
	/*wake up sas layer */
	higgs_recycle_dev(ll_card, ll_dev);
	sal_del_dev_cb(sal_dev, SAL_STATUS_SUCCESS);

	HIGGS_TRACE(OSP_DBL_INFO, 100, "Card:%d deregister dev:0x%llx response",
		    ll_card->card_id, sal_dev->sas_addr);

	SAS_ATOMIC_DEC(&(sal_dev->port->dev_num));

	return OK;
}

/**
 * higgs_abort_port_dev_io - abort all io belong this port
 * @sal_dev: sal card object
 * @port: port object
 */
s32 higgs_abort_port_dev_io(struct sal_card * sal_card, struct sal_port * port)
{
	struct list_head *node = NULL;
	struct list_head *tmp = NULL;
	struct sal_dev *dev = NULL;
	unsigned long flag = 0;
	s32 wait_time = SAL_ABORT_DEV_IO_TIMEOUT;
	s32 ret = ERROR;

	if (NULL == sal_card->ll_func.eh_abort_op) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 100,
			    "Card %d abort disk IO function is null",
			    sal_card->card_no);
		return ERROR;
	}
	SAS_ATOMIC_SET(&port->pend_abort, 0);

	spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(node, tmp, &sal_card->active_dev_list) {
		dev = SAS_LIST_ENTRY(node, struct sal_dev, dev_list);

		if (dev->port == port) {
			spin_lock_irqsave(&dev->dev_state_lock, flag);
			if (SAL_DEV_ST_FLASH == dev->dev_status) {
				spin_unlock_irqrestore(&dev->dev_state_lock,
						       flag);
				continue;
			}
			spin_unlock_irqrestore(&dev->dev_state_lock, flag);

			HIGGS_TRACE(OSP_DBL_INFO, 601,
				    "Card:%d abort port:%d disk:0x%llx IO",
				    sal_card->card_no, port->port_id,
				    dev->sas_addr);

			SAS_ATOMIC_INC(&port->pend_abort);
			/*active -> in remove */
			(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_MISS);

			sal_clr_err_handler_notify_by_dev(sal_card, dev, true);

			ret =
			    sal_card->ll_func.eh_abort_op(SAL_EH_ABORT_OPT_DEV,
							  (void *)dev);
			if (SAL_CMND_EXEC_SUCCESS != ret) {
				SAS_ATOMIC_DEC(&port->pend_abort);
				HIGGS_TRACE(OSP_DBL_MAJOR, 100,
					    "Card:%d abort disk:0x%llx IO failed,continue",
					    sal_card->card_no, dev->sas_addr);
			}
		}
	}
	spin_unlock_irqrestore(&sal_card->card_dev_list_lock, flag);

	while (wait_time >= 0) {
		if (0 == SAS_ATOMIC_READ(&port->pend_abort)) {
			HIGGS_TRACE(OSP_DBL_INFO, 100, "All dev IO abort Ok");
			return OK;
		}
		HIGGS_TRACE_FRQLIMIT(OSP_DBL_MINOR, msecs_to_jiffies(300), 100,
				     "pending abort cmd rsp:%d",
				     SAS_ATOMIC_READ(&port->pend_abort));
		SAS_MSLEEP(10);
		wait_time--;
	}

	HIGGS_TRACE(OSP_DBL_MAJOR, 603,
		    "Card:%d Port:0x%x(FW PortId:%d) abort all dev io time out,"
		    "pending abort cmd rsp:%d", sal_card->card_no,
		    port->logic_id, port->port_id,
		    SAS_ATOMIC_READ(&port->pend_abort));
	return ERROR;
}

/**
 * higgs_dereg_port_dev - deregister all devices belong this port
 * @sal_dev: sal card object
 * @port: port object
 */
s32 higgs_dereg_port_dev(struct sal_card * sal_card, struct sal_port * port)
{
	struct list_head *node = NULL;
	struct list_head *tmp = NULL;
	struct sal_dev *dev = NULL;
	unsigned long flag = 0;
	s32 wait_time = SAL_DEL_DEV_TIMEOUT;
	s32 ret = ERROR;

	if (NULL == sal_card->ll_func.dev_op) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 100, "pfnDelDev func is NULL");
		return ERROR;
	}

	HIGGS_TRACE(OSP_DBL_INFO, 100,
		    "Card:%d will del disk num:%d,Port:0x%x(FW PortId:%d)",
		    sal_card->card_no, SAS_ATOMIC_READ(&port->dev_num),
		    port->logic_id, port->port_id);

	spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
	SAS_LIST_FOR_EACH_SAFE(node, tmp, &sal_card->active_dev_list) {
		dev = SAS_LIST_ENTRY(node, struct sal_dev, dev_list);
		if (dev->port == port) {
			spin_lock_irqsave(&dev->dev_state_lock, flag);
			if (SAL_DEV_ST_FLASH == dev->dev_status) {
				spin_unlock_irqrestore(&dev->dev_state_lock,
						       flag);
				continue;
			}
			spin_unlock_irqrestore(&dev->dev_state_lock, flag);

			(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_DEREG);
			
			ret = sal_card->ll_func.dev_op(dev, SAL_DEV_OPT_DEREG);
			
			if (OK != ret) {
				SAS_ATOMIC_DEC(&port->dev_num);
				HIGGS_TRACE(OSP_DBL_MAJOR, 100,
					    "Card:%d del disk:0x%llx dev failed",
					    sal_card->card_no, dev->sas_addr);
			}
		}
	}
	spin_unlock_irqrestore(&sal_card->card_dev_list_lock, flag);
	HIGGS_TRACE(OSP_DBL_INFO, 100, "Card:%d send request times:%d",
		    sal_card->card_no, SAS_ATOMIC_READ(&port->dev_num));

	/*wait antil all devices already been deregister,retuen ok*/
	while (wait_time >= 0) {
		if (0 == SAS_ATOMIC_READ(&port->dev_num)) {
			HIGGS_TRACE(OSP_DBL_INFO, 100,
				    "Card:%d Port:0x%x(FW PortId:%d) del all OK",
				    sal_card->card_no, port->logic_id,
				    port->port_id);
			return OK;
		}

		SAS_MSLEEP(1);
		wait_time--;
	}
	HIGGS_TRACE(OSP_DBL_MAJOR, 100, "Card:%d Port:0x%x(FW PortId:%d) delete all dev \
              time out,remaining dev num:%d",
		    sal_card->card_no, port->logic_id, port->port_id, SAS_ATOMIC_READ(&port->dev_num));

	return ERROR;
}


struct sal_port *higgs_get_port_obj_from_port_id(struct sal_card *sal_card,
						 u32 port_id)
{
	u32 i = 0;
	unsigned long flag = 0;
	struct sal_port *port = NULL;

	spin_lock_irqsave(&sal_card->card_lock, flag);
	for (i = 0; i < SAL_MAX_PHY_NUM; i++) {
		port = sal_card->port[i];
		if (port->port_id == port_id) {
			spin_unlock_irqrestore(&sal_card->card_lock, flag);
			return port;
		}
	}
	spin_unlock_irqrestore(&sal_card->card_lock, flag);
	return NULL;

}

s32 higgs_dereg_port(struct sal_card * sal_card, u32 port_id, u8 port_op_code)
{
	struct sal_port *port = NULL;
	s32 ret = ERROR;

	HIGGS_ASSERT(NULL != sal_card, return ERROR);
	HIGGS_REF(port_op_code);

	port = higgs_get_port_obj_from_port_id(sal_card, port_id);
	if (NULL == port) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 100, "Card:%d PortId:%d not exist",
			    sal_card->card_no, port_id);
		return ERROR;
	}

	ret = higgs_abort_port_dev_io(sal_card, port);
	if (OK != ret) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 100, "Card:%d PortId:%d not exist",
			    sal_card->card_no, port_id);
		return ret;
	}

	ret = higgs_dereg_port_dev(sal_card, port);
	if (OK != ret) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 100, "Card:%d PortId:%d not exist",
			    sal_card->card_no, port_id);
		return ret;
	}
	return OK;
}

/**
 * higgs_dev_operation -  all device operation will call this function
 * @sal_dev: sal card object
 * @dev_opt: operation type
 */
s32 higgs_dev_operation(struct sal_dev * sal_dev, enum sal_dev_op_type dev_opt)
{

	s32 ret = ERROR;

	switch (dev_opt) {
	case SAL_DEV_OPT_REG:
		ret = higgs_reg_dev(sal_dev);
		break;

	case SAL_DEV_OPT_DEREG:
		ret = higgs_dereg_dev(sal_dev);
		break;
	default:
		HIGGS_TRACE(OSP_DBL_MAJOR, 4411, "invalid dev opcode:%d",
			    dev_opt);
		ret = ERROR;
		break;
	}

	return ret;
}


u64 higgs_get_dev_sas_addr_by_dev_id(struct higgs_card *ll_card, u32  dev_id)
{
	HIGGS_ASSERT(NULL != ll_card, return ~0x0ULL);
	HIGGS_ASSERT(dev_id < ll_card->higgs_can_dev, return ~0x0ULL);

	return ll_card->io_hw_struct_info.itct_base[dev_id].sas_addr;
}
