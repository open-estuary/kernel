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
#include "sal_init.h"

struct sal_dev_fsm sal_dev_fsm[SAL_DEV_ST_BUTT][SAL_DEV_EVENT_BUTT];


/**
 * sal_dev_fsm_init - device FSM(Finite State Machine) init
 *
 */
void sal_dev_fsm_init()
{
	u32 loop_event = 0;
	u32 loop_st = 0;

	memset(sal_dev_fsm, 0, sizeof(sal_dev_fsm));

	for (loop_st = 0; loop_st < SAL_DEV_ST_BUTT; loop_st++)
		for (loop_event = 0; loop_event < SAL_DEV_EVENT_BUTT;
		     loop_event++) {
			sal_dev_fsm[loop_st][loop_event].new_status =
			    SAL_DEV_ST_BUTT;
			sal_dev_fsm[loop_st][loop_event].act_func = NULL;
		}

	sal_dev_fsm[SAL_DEV_ST_FREE][SAL_DEV_EVENT_SMP_FIND].new_status =
	    SAL_DEV_ST_INIT;
	sal_dev_fsm[SAL_DEV_ST_FREE][SAL_DEV_EVENT_FIND].new_status =
	    SAL_DEV_ST_INIT;
	sal_dev_fsm[SAL_DEV_ST_INIT][SAL_DEV_EVENT_REG].new_status =
	    SAL_DEV_ST_IN_REG;
	sal_dev_fsm[SAL_DEV_ST_IN_REG][SAL_DEV_EVENT_REG_OK].new_status =
	    SAL_DEV_ST_REG;
	sal_dev_fsm[SAL_DEV_ST_IN_REG][SAL_DEV_EVENT_REG_FAILED].new_status =
	    SAL_DEV_ST_FLASH;
	sal_dev_fsm[SAL_DEV_ST_IN_REG][SAL_DEV_EVENT_SMP_REG_OK].new_status =
	    SAL_DEV_ST_ACTIVE;
	sal_dev_fsm[SAL_DEV_ST_REG][SAL_DEV_EVENT_REPORT_OK].new_status =
	    SAL_DEV_ST_ACTIVE;

	sal_dev_fsm[SAL_DEV_ST_REG][SAL_DEV_EVENT_REPORT_FAILED].new_status =
	    SAL_DEV_ST_ACTIVE;
	sal_dev_fsm[SAL_DEV_ST_ACTIVE][SAL_DEV_EVENT_MISS].new_status =
	    SAL_DEV_ST_IN_REMOVE;
	sal_dev_fsm[SAL_DEV_ST_IN_REMOVE][SAL_DEV_EVENT_DEREG].new_status =
	    SAL_DEV_ST_IN_DEREG;
	sal_dev_fsm[SAL_DEV_ST_IN_DEREG][SAL_DEV_EVENT_DEREG_OK].new_status =
	    SAL_DEV_ST_FLASH;
	sal_dev_fsm[SAL_DEV_ST_IN_DEREG][SAL_DEV_EVENT_DEREG_FAILED].
	    new_status = SAL_DEV_ST_FLASH;

	sal_dev_fsm[SAL_DEV_ST_FLASH][SAL_DEV_EVENT_SMP_FIND].new_status =
	    SAL_DEV_ST_INIT;
	sal_dev_fsm[SAL_DEV_ST_FLASH][SAL_DEV_EVENT_FIND].new_status =
	    SAL_DEV_ST_INIT;
	/*SMP device*/
	sal_dev_fsm[SAL_DEV_ST_FLASH][SAL_DEV_EVENT_REF_CLR].new_status = SAL_DEV_ST_FREE;
	sal_dev_fsm[SAL_DEV_ST_FLASH][SAL_DEV_EVENT_REPORT_OUT].new_status =
	    SAL_DEV_ST_RDY_FREE;
	sal_dev_fsm[SAL_DEV_ST_RDY_FREE][SAL_DEV_EVENT_REF_CLR].new_status =
	    SAL_DEV_ST_FREE;
}

/**
 * sal_init_dev_ele - device element init
 * @sal_dev:point to device
 *
 */
void sal_init_dev_ele(struct sal_dev *sal_dev)
{
	memset(sal_dev, 0, sizeof(struct sal_dev));

	SAS_INIT_LIST_HEAD(&sal_dev->dev_list);
	SAS_SPIN_LOCK_INIT(&sal_dev->dev_state_lock);
	SAS_SPIN_LOCK_INIT(&sal_dev->dev_lock);

	/*for slow io*/
	memset(&sal_dev->slow_io, 0, sizeof(sal_dev->slow_io));

	memset(&sal_dev->io_err_print_ctrl, 0,
	       sizeof(sal_dev->io_err_print_ctrl));
	SAS_SPIN_LOCK_INIT(&sal_dev->slow_io.slow_io_lock);
	SAS_SPIN_LOCK_INIT(&sal_dev->sata_dev_data.sata_lock);
	sal_dev->slow_io.last_chk_jiff = jiffies;
	sal_dev->slow_io.last_idle_start_ns = OSAL_CURRENT_TIME_NSEC;
	SAS_SPIN_LOCK_INIT(&sal_dev->slow_io.slow_io_lock);

	SAS_INIT_COMPLETION(&sal_dev->compl);
	SAS_ATOMIC_SET(&sal_dev->ref, 0);

	sal_dev->dev_status = SAL_DEV_ST_FREE;
	sal_dev->scsi_id = SAL_INVALID_SCSIID;
	sal_dev->mid_layer_eh = false;
	
	sal_dev->cnt_report_in = 0;
	sal_dev->cnt_destroy = 0;	
	return;
}

/**
 * sal_dev_init - device resource init
 * @sal_card:point to controller
 *
 */
s32 sal_dev_init(struct sal_card * sal_card)
{
	s32 ret = ERROR;
	struct sal_dev *dev = NULL;
	u32 i = 0;
	unsigned long flag = 0;
	u32 conf_len = 0;
	u32 alloc_dev_mem = 0;

	conf_len = MIN(SAL_MAX_DEV_NUM, sal_card->config.max_targets);
	alloc_dev_mem = conf_len * sizeof(struct sal_dev);

	sal_card->dev_mem = SAS_VMALLOC(alloc_dev_mem);
	if (NULL == sal_card->dev_mem) {
		SAL_TRACE(OSP_DBL_MAJOR, 378,
			  "card %d alloc dev mem fail %d byte",
			  sal_card->card_no, alloc_dev_mem);
		goto done;
	}

	memset(sal_card->dev_mem, 0, alloc_dev_mem);

	SAS_SPIN_LOCK_INIT(&sal_card->card_dev_list_lock);
	SAS_INIT_LIST_HEAD(&sal_card->free_dev_list);
	SAS_INIT_LIST_HEAD(&sal_card->active_dev_list);

	for (i = 0; i < conf_len; i++) {
		dev = &sal_card->dev_mem[i];
		sal_init_dev_ele(dev);

		spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
		SAS_LIST_ADD_TAIL(&dev->dev_list, &sal_card->free_dev_list);
		spin_unlock_irqrestore(&sal_card->card_dev_list_lock, flag);
	}
	SAL_TRACE(OSP_DBL_INFO, 378, "card:%d alloc dev mem:%d byte OK",
		  sal_card->card_no, alloc_dev_mem);

	ret = OK;

      done:
	return ret;
}

s32 sal_dev_remove(struct sal_card * sal_card)
{
	s32 ret = ERROR;

	SAS_INIT_LIST_HEAD(&sal_card->free_dev_list);
	SAS_INIT_LIST_HEAD(&sal_card->active_dev_list);

	if (NULL != sal_card->dev_mem) {
		SAS_VFREE(sal_card->dev_mem);
		sal_card->dev_mem = NULL;
	}
	ret = OK;
	return ret;
}

/**
 * sal_get_free_dev - obtain a free device
 * @sal_card:point to controller
 *
 */
struct sal_dev *sal_get_free_dev(struct sal_card *sal_card)
{
	unsigned long flag = 0;
	struct sal_dev *sal_dev = NULL;

	spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
	if (!SAS_LIST_EMPTY(&sal_card->free_dev_list)) {
		sal_dev =
		    SAS_LIST_ENTRY(sal_card->free_dev_list.next, struct sal_dev,
				   dev_list);
		SAS_LIST_DEL_INIT(&sal_dev->dev_list);
		spin_unlock_irqrestore(&sal_card->card_dev_list_lock, flag);

		sal_init_dev_ele(sal_dev);

		/*device add to active list*/
		spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
		sal_card->active_dev_cnt++;
		SAS_LIST_ADD_TAIL(&sal_dev->dev_list,
				  &sal_card->active_dev_list);
		spin_unlock_irqrestore(&sal_card->card_dev_list_lock, flag);

		sal_get_dev(sal_dev);

		return sal_dev;
	}
	spin_unlock_irqrestore(&sal_card->card_dev_list_lock, flag);

	return NULL;
}

/**
 * sal_del_dev - device release to free list
 * @sal_card:point to controller
 * @sal_dev:device to release
 * @in_list_lock:in lock flag
 */
void sal_del_dev(struct sal_card *sal_card,
		 struct sal_dev *sal_dev, bool in_list_lock)
{
	unsigned long flag = 0;
	unsigned long card_flag = 0;
	struct sal_dev *slot_dev = NULL;

	SAL_ASSERT(NULL != sal_card, return);

	SAL_TRACE(OSP_DBL_DATA, 517,
		  "Dev addr:0x%llx will be put back to free list",
		  sal_dev->sas_addr);

	spin_lock_irqsave(&sal_card->card_lock, card_flag);
	if ((SAL_INVALID_SCSIID != sal_dev->scsi_id)
	    && (sal_dev->scsi_id < SAL_MAX_SCSI_ID_COUNT)) {
		slot_dev = sal_card->scsi_id_ctl.dev_slot[sal_dev->scsi_id];
		if (sal_dev != slot_dev) {
			SAL_TRACE(OSP_DBL_MINOR, 517,
				  "Card %d addr:0x%llx scsiid %d dev %p slotdev %p differ.",
				  sal_card->card_no, sal_dev->sas_addr,
				  sal_dev->scsi_id, sal_dev, slot_dev);
		} else {
			sal_card->scsi_id_ctl.dev_slot[sal_dev->scsi_id] = NULL;
			sal_card->scsi_id_ctl.slot_use[sal_dev->scsi_id] =
			    SAL_SCSI_SLOT_FREE;
		}
	}
	spin_unlock_irqrestore(&sal_card->card_lock, card_flag);

	/*delete from active list */
	if (false == in_list_lock)
		spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);

	sal_card->active_dev_cnt--;
	SAS_LIST_DEL_INIT(&sal_dev->dev_list);
	sal_init_dev_ele(sal_dev);
	SAS_LIST_ADD_TAIL(&sal_dev->dev_list, &sal_card->free_dev_list);

	if (false == in_list_lock)
		spin_unlock_irqrestore(&sal_card->card_dev_list_lock, flag);

	return;
}

/**
 * sal_dev_state_chg - change device state
 * @sal_dev:device to change
 * @event:event to change
 */
s32 sal_dev_state_chg(struct sal_dev * sal_dev, enum sal_dev_event event)
{
	unsigned long flag = 0;
	enum sal_dev_state old_state = SAL_DEV_ST_BUTT;
	enum sal_dev_state cur_state = SAL_DEV_ST_BUTT;
	char *pstr[] = {
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
	char *event_str[] = {
		"SMP-Dev Find",
		"Dev Find",
		"Miss",
		"Reg Dev",
		"Dereg Dev",
		"Reg OK",
		"Reg Failed",
		"Dereg OK",
		"Dereg Failed",
		"SMP Reg OK",
		"Report OK",
		"Report failed",
		"Report out",
		"Free Dev",
		"Unknown"
	};

	SAL_REF(pstr[0]);
	SAL_REF(event_str[0]);

	if (event >= SAL_DEV_EVENT_BUTT) {
		SAL_TRACE(OSP_DBL_MAJOR, 517, "Dev event:%d is invalid",
			  (s32) event);
		return ERROR;
	}

	spin_lock_irqsave(&sal_dev->dev_state_lock, flag);
	old_state = sal_dev->dev_status;
	if (old_state >= SAL_DEV_ST_BUTT) {
		SAL_TRACE(OSP_DBL_MAJOR, 517,
			  "Dev addr:0x%llx(%p) Event:%d(%s) Old State:%d(%s) is invalid",
			  sal_dev->sas_addr,
			  sal_dev,
			  event, event_str[event], old_state, pstr[old_state]);
		spin_unlock_irqrestore(&sal_dev->dev_state_lock, flag);
		return ERROR;
	}

	cur_state = sal_dev_fsm[old_state][event].new_status;
	if (cur_state >= SAL_DEV_ST_BUTT) {
		SAL_TRACE(OSP_DBL_MAJOR, 517,
			  "Dev addr:0x%llx(%p) Old State:%d(%s) Event:%d(%s),New status:%d(%s) is invalid",
			  sal_dev->sas_addr,
			  sal_dev,
			  old_state,
			  pstr[old_state],
			  event, event_str[event], cur_state, pstr[cur_state]);
		spin_unlock_irqrestore(&sal_dev->dev_state_lock, flag);
		return ERROR;
	}
	sal_dev->dev_status = cur_state;
	spin_unlock_irqrestore(&sal_dev->dev_state_lock, flag);

	return OK;
}

EXPORT_SYMBOL(sal_dev_state_chg);


struct sal_dev *sal_get_dev_by_sas_addr(struct sal_port *port, u64 sas_addr)
{
	unsigned long flag = 0;
	struct list_head *node = NULL;
	struct sal_card *sal_card = NULL;
	struct sal_dev *dev = NULL;

	sal_card = port->card;
	spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
	SAS_LIST_FOR_EACH(node, &sal_card->active_dev_list) {
		dev = SAS_LIST_ENTRY(node, struct sal_dev, dev_list);
		if ((sas_addr == dev->sas_addr)
		    && (port->logic_id == dev->logic_id)
		    && (port->port_id == dev->port->port_id)) {
			sal_get_dev(dev);
			spin_unlock_irqrestore(&sal_card->card_dev_list_lock,
					       flag);
			return dev;
		}
	}
	spin_unlock_irqrestore(&sal_card->card_dev_list_lock, flag);
	return NULL;
}

struct sal_dev *sal_get_dev_by_sas_addr_in_dev_list_lock(struct sal_port *port,
							 u64 sas_addr)
{
	struct list_head *node = NULL;
	struct sal_card *sal_card = NULL;
	struct sal_dev *dev = NULL;

	sal_card = port->card;
	SAS_LIST_FOR_EACH(node, &sal_card->active_dev_list) {
		dev = SAS_LIST_ENTRY(node, struct sal_dev, dev_list);
		if ((sas_addr == dev->sas_addr)
		    && (port->logic_id == dev->logic_id)
		    && (port->port_id == dev->port->port_id)) {
			sal_get_dev(dev);
			return dev;
		}
	}
	return NULL;
}

enum sal_dev_state sal_get_dev_state(struct sal_dev *sal_dev)
{
	unsigned long flag = 0;
	enum sal_dev_state ret = SAL_DEV_ST_BUTT;

	SAL_ASSERT(NULL != sal_dev, return SAL_DEV_ST_BUTT);

	spin_lock_irqsave(&sal_dev->dev_state_lock, flag);
	ret = sal_dev->dev_status;
	spin_unlock_irqrestore(&sal_dev->dev_state_lock, flag);

	return ret;
}

EXPORT_SYMBOL(sal_get_dev_state);


void sal_get_dev(struct sal_dev *dev)
{
	if (NULL != dev)
		SAS_ATOMIC_INC(&dev->ref);

	return;
}


void sal_put_dev(struct sal_dev *dev)
{
	struct sal_card *card = NULL;

	SAL_ASSERT(NULL != dev, return);

	card = dev->card;

	if (SAS_ATOMIC_DEC_AND_TEST(&dev->ref)) {
		SAL_TRACE(OSP_DBL_DATA, 517,
			  "Dev addr:0x%llx(sal dev:%p) is free", dev->sas_addr,
			  dev);
		/* change state*/
		(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_REF_CLR);

		sal_del_dev(card, dev, false);
	}
	return;
}

/**
 * sal_put_dev_in_dev_list_lock - release device, you can not called it in card lock
 * @dev:device to release
 * 
 */
void sal_put_dev_in_dev_list_lock(struct sal_dev *dev)
{
	struct sal_card *card = NULL;

	SAL_ASSERT(NULL != dev, return);

	card = dev->card;

	if (SAS_ATOMIC_DEC_AND_TEST(&dev->ref)) {
		SAL_TRACE(OSP_DBL_DATA, 517,
			  "Dev addr:0x%llx(sal dev:%p) is free", dev->sas_addr,
			  dev);
		/* change state*/
		(void)sal_dev_state_chg(dev, SAL_DEV_EVENT_REF_CLR);

		sal_del_dev(card, dev, true);
	}
	return;
}


void sal_dev_check_put_last(struct sal_dev *dev, bool in_list_lock)
{
	if (SAL_DEV_TYPE_SMP == dev->dev_type) {
		if (in_list_lock)
			sal_put_dev_in_dev_list_lock(dev);
		else
			sal_put_dev(dev);
	} else {
		if ((sal_get_dev_state(dev) == SAL_DEV_ST_RDY_FREE)
		    && (dev->cnt_report_in == dev->cnt_destroy)) {
			if (in_list_lock)
				sal_put_dev_in_dev_list_lock(dev);
			else
				sal_put_dev(dev);
		}
	}
	return;
}

/**
 * sal_abnormal_put_dev - release abnormal device, you can not called it in card lock
 * @dev:device to release
 * 
 */
void sal_abnormal_put_dev(struct sal_dev *dev)
{
	SAL_ASSERT(NULL != dev, return);

	if (SAL_INVALID_SCSIID != dev->scsi_id)
		return;

	sal_put_dev(dev);

	return;
}

/**
 * sal_get_dev_by_scsi_id - obtain device by scsi id
 * @card: point to card controller
 * @scsi_id: scci id
 */
struct sal_dev *sal_get_dev_by_scsi_id(struct sal_card *card, u32 scsi_id)
{
	unsigned long flags = 0;
	struct sal_dev *dev = 0;

	if (scsi_id >= SAL_MAX_SCSI_ID_COUNT) {
		SAL_TRACE(OSP_DBL_MAJOR, 790, "scsi id is invalid");
		return NULL;
	}

	spin_lock_irqsave(&card->card_lock, flags);
	dev = card->scsi_id_ctl.dev_slot[scsi_id];
	if (NULL == dev) {
		SAL_TRACE(OSP_DBL_MAJOR, 791, "sal dev is NULL");
		spin_unlock_irqrestore(&card->card_lock, flags);
		return NULL;
	}

	sal_get_dev(dev);
	spin_unlock_irqrestore(&card->card_lock, flags);
	return dev;
}

void sal_adjust_queue_depth(u16 host_id, u16 chan_id, u16 target_id, u16 lun_id, s32 tags)
{
    struct scsi_device* scsi_dev = NULL;
    struct Scsi_Host* scsi_host = NULL;

    scsi_host = scsi_host_lookup(host_id);
    if ((NULL == scsi_host)) {
        return;
    }


    scsi_dev = scsi_device_lookup(scsi_host, chan_id,
                                      target_id, lun_id);
    if (NULL == scsi_dev) {
        scsi_host_put(scsi_host);
        return;
    }


    if (0 == scsi_device_online(scsi_dev)) {

        scsi_device_put(scsi_dev);
        scsi_host_put(scsi_host);
        return;
    }

	scsi_change_queue_depth(scsi_dev, tags);
    scsi_device_put(scsi_dev);
    scsi_host_put(scsi_host);

	return;
}

/**
 * sal_slave_alloc - slave alloc device register to scsi middle level
 * @cmd: device address infomation
 * @device: ponit to device
 */
s32 sal_slave_alloc(struct drv_device_address * cmd, void **device)
{
	unsigned long flag = 0;
	struct sal_dev *dev_info = NULL;
	struct sal_card *card = NULL;
	u32 scsi_id = 0;
	u32 queue_depth = 0;

	SAL_ASSERT(NULL != cmd, return ERROR);

	card = sal_find_card_by_host_id(cmd->initiator_id);
	if (NULL == card) {
		SAL_TRACE(OSP_DBL_MAJOR, 634, "Card not exist,host No:%d",
			  cmd->initiator_id);
		return ERROR;
	}

	spin_lock_irqsave(&card->card_lock, flag);
	if (card->flag & SAL_CARD_REMOVE_PROCESS) {
		spin_unlock_irqrestore(&card->card_lock, flag);
		SAL_TRACE(OSP_DBL_MINOR, 635, "Card:%d is shutting down",
			  card->card_no);
		return ERROR;
	}

	scsi_id = cmd->tgt_id;

	dev_info = card->scsi_id_ctl.dev_slot[scsi_id];
	if (NULL == dev_info) {
		SAL_TRACE(OSP_DBL_INFO, 636, "Card:%d dev(scsi id:%d) is NULL",
			  card->card_no, scsi_id);
		spin_unlock_irqrestore(&card->card_lock, flag);
		return ERROR;
	}

	dev_info->drv_info.driver_data = (void *)dev_info;


	*device = (void *)&(dev_info->drv_info);

	/* modify sata device queue depth */
	if (SAL_DEV_TYPE_STP == dev_info->dev_type) {
		if (SAT_NCQ_ENABLED == dev_info->sata_dev_data.ncq_enable)
			/*NCQ opened*/
			queue_depth = SAL_MAX_DEV_QUEUEDEPTH;
		else
			queue_depth = 1;

		queue_depth = MIN(queue_depth, card->config.dev_queue_depth);
	} else {
		queue_depth = card->config.dev_queue_depth;
	}
	spin_unlock_irqrestore(&card->card_lock, flag);


	sal_adjust_queue_depth((u16)cmd->initiator_id, (u16)cmd->bus_id, 
		(u16)cmd->tgt_id, (u16)cmd->function_id, (s32) queue_depth);

	SAL_TRACE(OSP_DBL_INFO, 636,
		  "Card:%d [%d:0:%d:0] slave alloc,change dev:0x%llx(ST:%d ref count:%d,%d,%d) queue depth to %d",
		  card->card_no,
		  cmd->initiator_id,
		  cmd->tgt_id,
		  dev_info->sas_addr,
		  dev_info->dev_status,
		  dev_info->cnt_report_in,
		  dev_info->cnt_destroy,
		  atomic_read(&dev_info->ref), queue_depth);

	sal_get_dev(dev_info);

	return OK;
}

/**
 * sal_slave_destroy -  scsi middle level delete device
 * @argv_in: slave destroy input parse
 * 
 */
void sal_slave_destroy(struct drv_device * argv_in)
{
	struct sal_dev *dev_info = NULL;
	struct sal_card *card = NULL;
	unsigned long flag = 0;
	u32 scsi_id = 0;
	u8 slot_use = 0;
	struct drv_device_address *device_addr = NULL;

	SAL_ASSERT(NULL != argv_in, return);

	device_addr = argv_in->addr;
	if (NULL == device_addr) {
		SAL_TRACE(OSP_DBL_MAJOR, 638, "Dev is NULL");
		return;
	}

	scsi_id = device_addr->tgt_id;

	card = sal_find_card_by_host_id(device_addr->initiator_id);
	if (NULL == card) {
		SAL_TRACE(OSP_DBL_MAJOR, 634, "Card not exist, host No:%d",
			  device_addr->initiator_id);
		return;
	}

	SAL_ASSERT(scsi_id < SAL_MAX_SCSI_ID_COUNT, return);

	argv_in->device = NULL;

	spin_lock_irqsave(&card->card_lock, flag);
	dev_info = card->scsi_id_ctl.dev_slot[scsi_id];
	slot_use = card->scsi_id_ctl.slot_use[scsi_id];
	spin_unlock_irqrestore(&card->card_lock, flag);
	if (dev_info && (SAL_SCSI_SLOT_USED == slot_use)) {
		dev_info->cnt_destroy++;
		SAL_TRACE(OSP_DBL_INFO, 640,
			  "Card:%d [%d:0:%d:0] is going to slave destroy,dev addr:%llx(sal dev:%p,ST:%d ref count:%d,%d,%d)",
			  card->card_no,
			  device_addr->initiator_id,
			  scsi_id,
			  dev_info->sas_addr,
			  dev_info,
			  dev_info->dev_status,
			  dev_info->cnt_report_in,
			  dev_info->cnt_destroy,
			  SAS_ATOMIC_READ(&dev_info->ref));
		sal_put_dev(dev_info);

		sal_dev_check_put_last(dev_info, false);
	} else {
		SAL_TRACE(OSP_DBL_MAJOR, 642,
			  "Card:%d node scsi id:%d dev pointer %p (used flag:%d) reused",
			  card->card_no, scsi_id, dev_info, slot_use);
	}

	return;
}

#if 0 /* begin: delete BDM intf */
/**
 * sal_initiator_ctrl -  called by BDM
 * @argv_in: input parse
 * @argv_out:output parse
 */
s32 sal_initiator_ctrl(void *argv_in, void *argv_out)
{
	s32 ret = ERROR;
	DRV_INITIATOR_CTRL_ARGIN_S *initiator_ctl = NULL;
	struct sal_card *sal_card = NULL;
	u32 host_id = 0;
	u32 *iout = NULL;
	u64 *llout = NULL;

	DRV_ASSERT(NULL != argv_in, return ERROR);
	DRV_ASSERT(NULL != argv_out, return ERROR);

	initiator_ctl = (DRV_INITIATOR_CTRL_ARGIN_S *) argv_in;
	host_id = (u32) initiator_ctl->initiator_id;

	sal_card = sal_find_card_by_host_id(host_id);
	if (NULL == sal_card) {
		SAL_TRACE(OSP_DBL_MAJOR, 642, "Can't find card by hostid:%d",
			  host_id);
		return ERROR;
	}

	if (SAL_CARD_REMOVE_PROCESS == sal_get_card_flag(sal_card)) {
		SAL_TRACE(OSP_DBL_MAJOR, 642, "Card:%d is removing now",
			  sal_card->card_no);
		return ERROR;

	}

	switch (initiator_ctl->cmd) {
	case DRV_INI_OP_GET_QUEUE_DEPTH:
		iout = (u32 *) argv_out;
		*iout = sal_card->config.max_can_queues;
		ret = OK;
		break;

	case DRV_INI_OP_GET_SGT_SIZE:
		iout = (u32 *) argv_out;
		*iout = SAL_MAX_DMA_SEGS;
		ret = OK;
		break;

	case DRV_INI_OP_GET_DMA_BOUNDARY:
		llout = (u64 *) argv_out;
		*llout = 0xFFFFFFFF;
		ret = OK;
		break;

	case DRV_INI_OP_GET_SECTORS:
		iout = (u32 *) argv_out;
		*iout = SAL_IO_MAX_SECTORS;
		ret = OK;
		break;

	case DRV_INI_OP_GET_SEGMENT_SIZE:
		iout = (u32 *) argv_out;
		*iout = SAL_MAX_SGL_SEGMENT;
		ret = OK;
		break;

	default:
		SAL_TRACE(OSP_DBL_MAJOR, 8,
			  "input wrong opcode 0x%x", initiator_ctl->cmd);
		ret = ERROR;
		break;
	}

	return ret;
}

/**
 * sal_device_ctrl -  called by BDM
 * @argv_in: input parse
 * @argv_out:output parse
 */
s32 sal_device_ctrl(void *argv_in, void *argv_out)
{
	DRV_DEVICE_CTRL_ARGIN_S *dev_ctrl = NULL;
	u32 *iout = NULL;
	struct sal_card *sal_card = NULL;
	u16 scsi_id = 0;
	unsigned long flag = 0;
	struct sal_dev *sal_dev = NULL;
	s32 ret = ERROR;
	u32 queue_depth = 0;

	DRV_ASSERT(NULL != argv_in, return ERROR);
	DRV_ASSERT(NULL != argv_out, return ERROR);

	iout = (u32 *) argv_out;
	dev_ctrl = (DRV_DEVICE_CTRL_ARGIN_S *) argv_in;

	sal_card = sal_find_card_by_host_id(dev_ctrl->addr->initiator_id);
	if (NULL == sal_card) {
		SAL_TRACE(OSP_DBL_MAJOR, 642, "Can't find card by hostid:%d",
			  dev_ctrl->addr->initiator_id);
		return ERROR;
	}

	if (SAL_CARD_REMOVE_PROCESS == sal_get_card_flag(sal_card)) {
		SAL_TRACE(OSP_DBL_MAJOR, 642, "Card:%d is removing now",
			  sal_card->card_no);
		return ERROR;

	}

	scsi_id = dev_ctrl->addr->tgt_id;
	if (scsi_id >= SAL_MAX_SCSI_ID_COUNT) {
		SAL_TRACE(OSP_DBL_MAJOR, 642, "scsi id:%d is invalid", scsi_id);
		return ERROR;
	}

	spin_lock_irqsave(&sal_card->card_lock, flag);
	sal_dev = sal_card->scsi_id_ctl.dev_slot[scsi_id];
	if (NULL == sal_dev) {
		SAL_TRACE(OSP_DBL_MAJOR, 2506, "Can't find dev by scsi:%d",
			  scsi_id);
		spin_unlock_irqrestore(&sal_card->card_lock, flag);
		return ERROR;
	}
	spin_unlock_irqrestore(&sal_card->card_lock, flag);

	switch (dev_ctrl->cmd) {
	case DRV_TGT_OP_GET_QUEUE_DEPTH:
		if (SAL_DEV_TYPE_STP == sal_dev->dev_type) {
			/*SATA*/ 
			if (SAT_NCQ_ENABLED == sal_dev->sata_dev_data.ncq_enable)
				queue_depth = SAL_MAX_DEV_QUEUEDEPTH;
			else
				queue_depth = 1;
			*iout =
			    MIN(queue_depth, sal_card->config.dev_queue_depth);
		} else {
			*iout = sal_card->config.dev_queue_depth;
		}

		ret = OK;
		break;

	default:
		SAL_TRACE(OSP_DBL_MAJOR, 2506,
			  "not support opcode:0x%x", dev_ctrl->cmd);
		ret = ERROR;
		break;
	}

	SAL_TRACE(OSP_DBL_DATA, 2506,
		  "Dev:0x%llx(scsi id:%d) opcode:%d out value:0x%x",
		  sal_dev->sas_addr, scsi_id,
		  dev_ctrl->cmd, *((u32 *) argv_out));
	return ret;
}

#endif /* end: delete BDM intf */
