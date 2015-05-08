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
#include "sal_init.h"


/*----------------------------------------------*
 * global struct variable type define and initialize.      *
 *----------------------------------------------*/
struct sal_global_info sal_global = {
	.sal_log_level = OSP_DBL_INFO,
	.sal_mod_init_ok = false,
	.quick_done = false,
	.support_4k = false,
	.enable_all_reg = 0,
	.sal_cards[0] = NULL,
	.init_failed_reason[0] = 0,
	.io_stat_threshold[0] = 0,
	.prd_type = 0,
	.fw_dump_idx[0] = 0,
	.dly_time[0] = 0,
};


 peripheral_operation_struct sal_peripheral_operation =
{
    .get_product_type         =   NULL,
    .get_dev_position        =   NULL,
    .get_cmos_time        =   NULL,
    .get_unique_dev_position        =   NULL,
    
    .get_card_type        =   NULL,
    .read_logic        =   NULL,
    .write_logic        =   NULL,
    
    .i2c_capi_do_send        =   NULL,
                                                  
    .i2c_api_do_recv        =   NULL,
                                                
    .i2c_simple_read        =   NULL,
                                                      
    .i2c_simple_write        =   NULL,
                                                    
    .register_int_handler        =   NULL,
    .unregister_int_handler        =   NULL,
    .serdes_lane_reset        =   NULL,

    .serdes_enable_ctledfe        =   NULL,
    .register_gpe_handler        =   NULL,

    .gpe_interrupt_clear        =   NULL,
    .get_bsp_version        =   NULL,
    .bsp_reboot        =   NULL,
    .read_slot_id        =   NULL,
    
};
EXPORT_SYMBOL(sal_peripheral_operation);
/**
 * temporary use our own sas layer, later we use libsas
 */
#define DRV_NAME		"hisi_pv660"
/* scsi host id, must be unique */
static int hisi_pv660_id = 0;
#define DRVAPI_FUNCTION_ID 0
#define DRV_SCSI_CMD_PORT_ID (0x5a5aa5a5) 

/** ASSERT define, return when ASSERT fail. */
#ifdef _PCLINT_
 #define DRV_ASSERT(expr, args...) 
#else
 #define DRV_ASSERT(expr, args...) \
    do \
    {\
        if (unlikely(!(expr)))\
        {\
            printk(KERN_EMERG "BUG! (%s,%s,%d)%s (called from %p)\n", \
                   __FILE__, __FUNCTION__, __LINE__, \
                   # expr, __builtin_return_address(0)); \
            dump_stack();   \
            args; \
        } \
    } while (0)
#endif


/**fill 4 ID for SCSI device .*/
#define DRV_INI_FILL_DEVICE_ADDRESS(device, initiator, bus, target, function) \
    {\
        device.initiator_id = initiator; \
        device.bus_id = bus; \
        device.tgt_id   = target; \
        device.function_id = function; \
    }


s32 sal_queue_cmd(OSAL_SCSI_HOST_S* pHost, 
                                   OSAL_SCSI_CMND_S* scsi_cmd);

s32 sal_scsi_slave_alloc(OSAL_SCSI_DEVICE_S* dev);

void sal_scsi_slave_destroy(OSAL_SCSI_DEVICE_S* dev);

s32 sal_scsi_eh_reset_host(OSAL_SCSI_CMND_S* scsi_cmd);

s32 sal_scsi_eh_reset_device(OSAL_SCSI_CMND_S* scsi_cmd);

s32 sal_scsi_eh_abort_cmnd(OSAL_SCSI_CMND_S* scsi_cmd);

void sal_remove_all_dev(u8 card_id);

static struct scsi_host_template hisi_pv660_sht = {
	.module			= THIS_MODULE,
	.name			= DRV_NAME,
	.proc_name		= DRV_NAME,
	.queuecommand		= sal_queue_cmd,
	.target_alloc		= NULL,
	.slave_alloc		= sal_scsi_slave_alloc,
	.slave_configure	= NULL,
	.slave_destroy = sal_scsi_slave_destroy,
	.scan_finished		= NULL,
	.scan_start		= NULL,
	.change_queue_depth	= NULL,
	.bios_param		= NULL,
	.can_queue		= 1024,
	.cmd_per_lun		= 64,
	.this_id		= -1,
	.sg_tablesize		= SG_ALL,
	.use_clustering		= ENABLE_CLUSTERING,
	//.max_sectors		= SCSI_DEFAULT_MAX_SECTORS,
	.eh_host_reset_handler = sal_scsi_eh_reset_host,
	.eh_device_reset_handler = sal_scsi_eh_reset_device,
	.eh_abort_handler = sal_scsi_eh_abort_cmnd,
	.eh_bus_reset_handler = NULL,
	//.target_destroy		= sas_target_destroy,
	.ioctl			= NULL,
	.detect			= NULL,
	.bios_param		= NULL,
	.info			= NULL,
	//.use_blk_tags		= 1,
	//.track_queue_depth	= 1,
};

SAS_SPINLOCK_T card_lock;

EXPORT_SYMBOL(sal_global);

EXPORT_SYMBOL(card_lock);

//lint -e24 -e63 -e40 -e34 -e35
struct disc_event_intf disc_event_func = {
	.smp_frame_send = sal_send_disc_smp,
	.device_arrival = sal_disc_add_disk,
	.device_remove = sal_disc_del_disk,
	.discover_done = sal_disc_done,
};
//lint +e24 +e63 +e40 +e34 +e35

struct workqueue_struct *sal_wq = NULL;

s32 sal_initiator_handler(u32 op_code, void *argv_in, void *argv_out)
{
	s32 ret = ERROR;

	SAL_REF(argv_in);
	SAL_REF(argv_out);

	/*called function according to op_code.*/
	switch (op_code) {
	case DRV_INI_QUEUECMD:
		ret = sal_queue_cmnd((struct drv_ini_cmd *) argv_in);
		break;

	case DRV_INI_EH_ABORTCMD:
		ret = sal_eh_abort_command((struct drv_ini_cmd *) argv_in);
		break;

	case DRV_INI_EH_RESETDEVICE:
		ret = sal_eh_device_reset((struct drv_ini_cmd *) argv_in);
		break;

	case DRV_INI_EH_RESETHOST:
		ret = ERROR;
		break;

	case DRV_INI_SLAVE_ALLOC:
		ret =
		    sal_slave_alloc((struct drv_device_address *) argv_in,
				    (void **)argv_out);
		break;

	case DRV_INI_SLAVE_DESTROY:
		sal_slave_destroy((struct drv_device *) argv_in);
		ret = OK;
		break;
#if 0 /* begin: delete BDM intf */
		/*BEGIN 问题单号:DTS2013032808491 modified by chengwei 90006017 2013-03-28
		   * 新增接口实现
		 */
	case DRV_INI_INITIATOR_CTRL:
		ret = sal_initiator_ctrl(argv_in, argv_out);
		break;

	case DRV_INI_DEVICE_CTRL:
		ret = sal_device_ctrl(argv_in, argv_out);
		break;

		/*END 问题单号:DTS2013032808491 modified by chengwei 90006017 2013-03-28 */
#endif /* end: delete BDM intf */

	case DRV_INI_CHANGE_QUEUEDEPTH:
	case DRV_INI_CHANGE_QUEUETYPE:
		/* fall though */
	case DRV_INI_OPCODE_BUTT:
	default:
		SAL_TRACE_FRQLIMIT(OSP_DBL_MAJOR, 2000, 688,
				   "Unkown operation code:0x%x", op_code);
		ret = ERROR;
	}
	return ret;
}

s32 sal_scsi_eh_reset_host(OSAL_SCSI_CMND_S* scsi_cmd)
{
    /* no use and return 0 */
    return OK;
}

void sal_scsi_done(struct drv_ini_cmd* drv_cmd)
{
    OSAL_SCSI_CMND_S* scsi_cmd = NULL;

    DRV_ASSERT(drv_cmd, return );

    scsi_cmd = (OSAL_SCSI_CMND_S*)drv_cmd->upper_data;

    if(unlikely(NULL == scsi_cmd))
    {
        printk("Scsi cmnd is NULL.");
        return;
    }
    
    /**fill scsi_cmnd*/
    scsi_cmd->result = (s32)drv_cmd->result.status;

    /*fill SENSE BUFFER*/
    OSAL_MemCpy(scsi_cmd->sense_buffer, drv_cmd->result.sense_data,
                SCSI_SENSE_BUFFERSIZE);

    /*remain data length.*/
    OSAL_scsi_set_resid(scsi_cmd, (s32)drv_cmd->remain_data_len);

    /**call DONE function */
    scsi_cmd->scsi_done(scsi_cmd);

    return;
}

s32 sal_queue_cmd_lck(OSAL_SCSI_CMND_S* scsi_cmd, 
                          void (*done)(OSAL_SCSI_CMND_S*))
{
    struct drv_ini_cmd drv_cmd;
    s32 ret = ERROR;
    u32 cmd_len = 0;

    DRV_ASSERT((scsi_cmd && done), return ERROR);

    cmd_len = ((scsi_cmd->cmd_len) < (DRV_SCSI_CDB_LEN) ? \
                  (scsi_cmd->cmd_len) : (DRV_SCSI_CDB_LEN));

    OSAL_MemSet(&drv_cmd, 0, sizeof(struct drv_ini_cmd));

    /**fill SCSI_INI_CMD*/

    /*CDB lenth*/

    //Cmd.CmdLen = pCmnd->cmd_len;

    /*tcq tag */
    drv_cmd.tag = scsi_cmd->tag;

    /*data transfer orientation */
    drv_cmd.io_direction = (enum drv_io_direction)scsi_cmd->sc_data_direction;

    /*least transfer data*/
    drv_cmd.underflow = scsi_cmd->underflow;

    /*SCSI CDB*/
    OSAL_MemCpy(drv_cmd.cdb, scsi_cmd->cmnd, cmd_len);

    /*data length, unit:bytes.*/
    drv_cmd.data_len = OSAL_scsi_bufflen(scsi_cmd);

    /*driver device pointer*/
    drv_cmd.private_info = scsi_cmd->device->hostdata;

    //Cmd.SenseBuf = (OSP_CHAR *)scsi_cmd->sense_buffer;


    drv_cmd.upper_data = scsi_cmd;

    /*command SN*/
    drv_cmd.cmd_sn = (u64)scsi_cmd;


    drv_cmd.port_id = DRV_SCSI_CMD_PORT_ID;

    /**fill SCSI device address*/
    DRV_INI_FILL_DEVICE_ADDRESS(drv_cmd.addr,
                                (u16)scsi_cmd->device->host->host_no,
                                (u16)scsi_cmd->device->channel,
                                (u16)scsi_cmd->device->id,
                                DRVAPI_FUNCTION_ID
                                );
	#if 0
    API_LOG(DBG,LOG_ID_020E, "SCSI queue command, add:(%u:%u:%u:%u).",
            drv_cmd.addr.initiator_id, drv_cmd.addr.bus_id,
            drv_cmd.addr.tgt_id, drv_cmd.addr.function_id);
    #endif

    /*convert LUN from SCSI CMD.*/
    OSAL_int_to_scsilun(scsi_cmd->device->lun, (OSAL_SCSI_LUN_S*)drv_cmd.lun);



    drv_cmd.done = sal_scsi_done; /**use FRAME callback function */
    scsi_cmd->scsi_done = done;

    drv_cmd.sgl = (struct drv_sgl*)(void*)scsi_cmd->sdb.table.sgl;

	ret = sal_initiator_handler(DRV_INI_QUEUECMD, &drv_cmd, NULL);
    if (OK == ret) {
        scsi_cmd->host_scribble = drv_cmd.driver_data;   
    }

    return ret;
}

DEF_SCSI_QCMD(sal_queue_cmd);

s32 sal_scsi_slave_alloc(OSAL_SCSI_DEVICE_S* scsi_dev)
{
    struct drv_device_address device_addr;

    s32 ret = ERROR;

    DRV_ASSERT(scsi_dev, return ERROR);

    OSAL_MemSet(&device_addr, 0, sizeof(struct drv_device_address));

    /**fill SCSI device address.*/
    DRV_INI_FILL_DEVICE_ADDRESS(device_addr,
                                (u16)scsi_dev->host->host_no,
                                (u16)scsi_dev->channel,
                                (u16)scsi_dev->id,
                                DRVAPI_FUNCTION_ID
                                );

    //stDevice.ScsiDevice= (void *)pDev;

    printk("SCSI alloc slave, add:(%u:%u:%u:%u).",
            device_addr.initiator_id, device_addr.bus_id,
            device_addr.tgt_id, device_addr.function_id);

    /*call driver interface.*/

	ret = sal_initiator_handler(DRV_INI_SLAVE_ALLOC, &device_addr, &scsi_dev->hostdata);

    if (OK != ret) {
        printk("SCSI alloc slave ret %d.", ret);
    }

    
    return ret;
}

void sal_scsi_slave_destroy(OSAL_SCSI_DEVICE_S* scsi_dev)
{
    struct drv_device_address device_addr;
    s32 ret = ERROR;
    //u32 DrvType = 0;
    struct drv_device dev;

    REFERNCE_VAR(ret);
    DRV_ASSERT(scsi_dev, return );

    OSAL_MemSet(&device_addr, 0, sizeof(struct drv_device_address));
    OSAL_MemSet(&dev, 0, sizeof(struct drv_device));

   /**fill SCSI device address.*/
    DRV_INI_FILL_DEVICE_ADDRESS(device_addr,
                                (u16)scsi_dev->host->host_no,
                                (u16)scsi_dev->channel,
                                (u16)scsi_dev->id,
                                DRVAPI_FUNCTION_ID
                                );
    printk("SCSI destroy slave, add:(%u:%u:%u:%ud).",
            device_addr.initiator_id, device_addr.bus_id,
            device_addr.tgt_id, device_addr.function_id);

    /*called driver interface.*/
    //DrvType = DRVAPI_GetDrvTypeByIniID(device.InitiatorId);

    dev.addr   = &device_addr;
    dev.device = scsi_dev->hostdata;

	ret = sal_initiator_handler(DRV_INI_SLAVE_DESTROY, &dev, NULL);

    if (OK != ret) {
        printk("SCSI destroy slave ret %d\n", ret);
    }

    return;
}

s32 sal_scsi_eh_reset_device(OSAL_SCSI_CMND_S* scsi_cmd)
{
    struct drv_ini_cmd drv_cmd;
    s32 ret = ERROR;
    u32 cmd_len = 0;

    DRV_ASSERT(scsi_cmd, return FAILED);

    cmd_len = ((scsi_cmd->cmd_len) < (DRV_SCSI_CDB_LEN) ? \
                  (scsi_cmd->cmd_len) : (DRV_SCSI_CDB_LEN));

    OSAL_MemSet(&drv_cmd, 0, sizeof(struct drv_ini_cmd));

    /**fill SCSI_INI_CMD*/
    /*CDB length */

    //Cmd.cmd_len = scsi_cmd->cmd_len;

    /*tcq tag */
    drv_cmd.tag = scsi_cmd->tag;

    /*data transfer orientation */
    drv_cmd.io_direction = (enum drv_io_direction)scsi_cmd->sc_data_direction;

    /*least transfer data*/
    drv_cmd.underflow = scsi_cmd->underflow;

    /*SCSI CDB*/
    OSAL_MemCpy(drv_cmd.cdb, scsi_cmd->cmnd, cmd_len);

    /*data length, unit:bytes.*/
    drv_cmd.data_len = OSAL_scsi_bufflen(scsi_cmd);

    /*driver device pointer*/
    drv_cmd.private_info = scsi_cmd->device->hostdata;


    /*command SN*/
    drv_cmd.cmd_sn = (u64)scsi_cmd;


    drv_cmd.port_id = DRV_SCSI_CMD_PORT_ID;

    //Cmd.SenseBuf = (OSP_CHAR *)scsi_cmd->sense_buffer;

    drv_cmd.upper_data = scsi_cmd;

    drv_cmd.driver_data = scsi_cmd->host_scribble;

    /**fill SCSI device address*/
    DRV_INI_FILL_DEVICE_ADDRESS(drv_cmd.addr,
                                (u16)scsi_cmd->device->host->host_no,
                                (u16)scsi_cmd->device->channel,
                                (u16)scsi_cmd->device->id,
                                DRVAPI_FUNCTION_ID
                                );
    printk("SCSI queue command, add:(%u:%u:%u:%u).",
            drv_cmd.addr.initiator_id, drv_cmd.addr.bus_id,
            drv_cmd.addr.tgt_id, drv_cmd.addr.function_id);

    /*convert LUN from SCSI CMD.*/
    OSAL_int_to_scsilun(scsi_cmd->device->lun, (OSAL_SCSI_LUN_S*)drv_cmd.lun);


    drv_cmd.done = sal_scsi_done;

    ret = sal_initiator_handler(DRV_INI_EH_RESETDEVICE, &drv_cmd, NULL);
    
    
    if (OK != ret) {
		printk("SCSI Reset Device ret %d\n", ret);
    }
	
    return ret;
}

s32 sal_scsi_eh_abort_cmnd(OSAL_SCSI_CMND_S* scsi_cmd)
{
    struct drv_ini_cmd drv_cmd;
    s32 ret = FAILED;
    u32 cmd_len = 0;

    DRV_ASSERT(scsi_cmd, return FAILED);

    cmd_len = ((scsi_cmd->cmd_len) < (DRV_SCSI_CDB_LEN) ? \
                  (scsi_cmd->cmd_len) : (DRV_SCSI_CDB_LEN));

    OSAL_MemSet(&drv_cmd, 0, sizeof(struct drv_ini_cmd));

    /**fill SCSI_INI_CMD*/
    /*CDB lenth*/

    //Cmd.cmd_len = scsi_cmd->cmd_len;

    /*tcq tag*/
    drv_cmd.tag = scsi_cmd->tag;

    /*data transfer orientation */
    drv_cmd.io_direction = (enum drv_io_direction)scsi_cmd->sc_data_direction;

    /*least transfer data*/
    drv_cmd.underflow = scsi_cmd->underflow;

    /*SCSI CDB*/
    OSAL_MemCpy(drv_cmd.cdb, scsi_cmd->cmnd, cmd_len);

    /*data length, unit:bytes.*/
    drv_cmd.data_len = OSAL_scsi_bufflen(scsi_cmd);

    /*driver device pointer*/
    drv_cmd.private_info = scsi_cmd->device->hostdata;


    //Cmd.SenseBuf = (OSP_CHAR *)scsi_cmd->sense_buffer;

    drv_cmd.upper_data = scsi_cmd;

    drv_cmd.driver_data = scsi_cmd->host_scribble;

    /*command SN*/
    drv_cmd.cmd_sn = (u64)scsi_cmd;

   
    drv_cmd.port_id = DRV_SCSI_CMD_PORT_ID;

    /**fill SCSI device address*/
    DRV_INI_FILL_DEVICE_ADDRESS(drv_cmd.addr,
                                (u16)scsi_cmd->device->host->host_no,
                                (u16)scsi_cmd->device->channel,
                                (u16)scsi_cmd->device->id,
                                DRVAPI_FUNCTION_ID
                                );
    printk("SCSI queue command, add:(%u:%u:%u:%u).",
            drv_cmd.addr.initiator_id, drv_cmd.addr.bus_id,
            drv_cmd.addr.tgt_id, drv_cmd.addr.function_id);

    /*convert LUN from SCSI CMD*/
    OSAL_int_to_scsilun(scsi_cmd->device->lun, (OSAL_SCSI_LUN_S*)drv_cmd.lun);


    drv_cmd.done = sal_scsi_done; /*use FRAME callback function*/


	ret = sal_initiator_handler(DRV_INI_EH_ABORTCMD, &drv_cmd, NULL);

    printk("SCSI error handle abort command ret %d\n", ret);

    return ret;
}

/*----------------------------------------------*
 * return IO end in driver to mid-layer.                           *
 *----------------------------------------------*/
void sal_complete_all_io(struct sal_card *sal_card)
{
	if (SAL_PORT_MODE_TGT == sal_card->config.work_mode)
		return;
	/*release active IO */
	sal_free_all_active_io(sal_card);

	/*
	   * release inner IO resource.
	 */
	sal_free_left_io(sal_card);

	return;
}

EXPORT_SYMBOL(sal_complete_all_io);

/*----------------------------------------------*
 * obtain card struct corresponding to card number.                           *
 *----------------------------------------------*/
struct sal_card *sal_get_card(u32 card_no)
{
	unsigned long flags = 0;
	struct sal_card *sal_card = NULL;

	if (card_no >= SAL_MAX_CARD_NUM)
		return NULL;

	spin_lock_irqsave(&card_lock, flags);
	sal_card = sal_global.sal_cards[card_no];
	if ((NULL == sal_card)
	    || ((struct sal_card *)SAL_INVALID_POSITION == sal_card)
	    || (0 == SAS_ATOMIC_READ(&sal_card->ref_cnt))) {
		spin_unlock_irqrestore(&card_lock, flags);
		return NULL;
	}
	/* reference count increase. */
	SAS_ATOMIC_INC(&sal_card->ref_cnt);
	spin_unlock_irqrestore(&card_lock, flags);
	return sal_card;
}

EXPORT_SYMBOL(sal_get_card);

/*----------------------------------------------*
 * put card , which is corresponding to sal_get_card.                           *
 *----------------------------------------------*/
void sal_put_card(struct sal_card *sal_card)
{
	unsigned long flags = 0;

	SAL_ASSERT(NULL != sal_card, return);

	spin_lock_irqsave(&card_lock, flags);
	if (SAS_ATOMIC_DEC_AND_TEST(&sal_card->ref_cnt)) {
		SAL_TRACE(OSP_DBL_INFO, 725,
			  "it is going to free sal card:%d mem",
			  sal_card->card_no);

		/* release slot. */
		sal_global.sal_cards[sal_card->card_no] = NULL;

		/* release memory */
		memset(sal_card, 0, sizeof(struct sal_card));
		SAS_VFREE(sal_card);
	}
	spin_unlock_irqrestore(&card_lock, flags);

	return;
}

EXPORT_SYMBOL(sal_put_card);
#if 0 /* begin:delete function that is not used */
/*****************************************************************************
 函 数 名  : sal_get_card_num
 功能描述  : 生成驱动需要的CardNo
 输入参数  : struct pci_dev *dev
 		u8 *v_ucCardPos
 		u8 *card_no
 输出参数  : 无
 返 回 值  : u8
*****************************************************************************/
s32 sal_get_card_num(struct pci_dev * dev, u8 * card_pos, u8 * card_no)
{
	u8 prd_id = 0;
	u8 pos = 0;
	u8 cardno = 0;

	/*从BSP接口获取当前卡位置 */
       if(NULL == sal_peripheral_operation.get_dev_position)
       {
             pos = 10;
       }else if (OK != sal_peripheral_operation.get_dev_position(dev->bus->number, &pos)) {
		SAL_TRACE(OSP_DBL_MAJOR, 727,
			  "Get card positon failed, sas card bus id 0x%x",
			  dev->bus->number);
		return ERROR;
	}

	/*通过读取产品版本号判断确定CardNo的分配方式 */
        if(NULL == sal_peripheral_operation.get_product_type)
       {
             prd_id = SPANGEA_V2R1;
       }else if (OK != sal_peripheral_operation.get_product_type(&prd_id)) {
             SAL_TRACE(OSP_DBL_MAJOR, 727, "Get card product id failed");
             return ERROR;
	}

	/*BEGIN:问题单号:DTS2013051700014 add by jianghong,2013-7-4,
	   * 打开关闭端口PHY功能未正常实现
	 */
	sal_global.prd_type = prd_id;	/*全局变量记录产品类型 */
	/*END:问题单号:DTS2013051700014 add by jianghong,2013-7-4, */

	cardno = pos & 0x1f;	/* 低5位表示槽位号,槽位号与卡号一致 */
	if (SPANGEA_V2R1 == sal_global.prd_type) {
		SAL_TRACE(OSP_DBL_INFO, 429, "Pangea V2R1 Card position:0x%x",
			  pos);
		if (0x20 != pos)	/*V2R1板载的8072分配为0号Card */
			cardno = cardno + 1;	/* V2的外部接口卡编号从1开始 */
	}

	if (cardno >= SAL_MAX_CARD_NUM) {
		SAL_TRACE(OSP_DBL_MAJOR, 728,
			  "Card position:0x%x exceed than max support number",
			  cardno);
		return ERROR;
	}

	*card_pos = pos;
	*card_no = cardno;

	return OK;

}

EXPORT_SYMBOL(sal_get_card_num);
#endif /* end: delete function that is not used */

/*----------------------------------------------*
 * allocate card interface which is supported to driver by SAL.                           *
 *----------------------------------------------*/
struct sal_card *sal_alloc_card(u8 card_pos, u8 card_no, u32 mem_size,
				u32 phy_num)
{
	u32 i = 0;
	unsigned long flags = 0;
	struct sal_card *sal_card = NULL;
	u32 j = 0;
	u32 alloc_size = 0;

	/*judeg phy invalid or not*/
	if ((0 == phy_num) || (phy_num > SAL_MAX_PHY_NUM)) {
		SAL_TRACE(OSP_DBL_MAJOR, 726,
			  "phy num:%d not supported by sal", phy_num);
		return NULL;
	}

	SAL_TRACE(OSP_DBL_INFO, 729, "Alloc card in slot:%d", card_no);

	spin_lock_irqsave(&card_lock, flags);
	sal_card = sal_global.sal_cards[card_no];
	if (NULL != sal_card) {
		/* if card is null, it has been used.*/
		SAL_TRACE(OSP_DBL_MAJOR, 730,
			  "Card:%d visual addr:%p ref count:%d", card_no,
			  sal_card, SAS_ATOMIC_READ(&sal_card->ref_cnt));
		spin_unlock_irqrestore(&card_lock, flags);
		return NULL;

	} else {
		sal_global.sal_cards[card_no] =
		    (struct sal_card *)SAL_INVALID_POSITION;
	}
	spin_unlock_irqrestore(&card_lock, flags);

	alloc_size = (sizeof(struct sal_card) +
		      phy_num * (sizeof(struct sal_phy) +
				 sizeof(struct sal_port))
		      + mem_size);
	sal_card = SAS_VMALLOC(alloc_size);
	if (NULL == sal_card) {
		SAL_TRACE(OSP_DBL_MAJOR, 733,
			  "Card:%d alloc memory(size:%d) failed", card_no,
			  alloc_size);

		sal_global.sal_cards[card_no] = NULL;

		goto fail_done;
	}
	memset(sal_card, 0, alloc_size);
	/* set low-layer position */
	sal_card->drv_data = (u8 *)
	    ((u64) sal_card + sizeof(struct sal_card) +
	     phy_num * (u64) (sizeof(struct sal_phy) +
			      sizeof(struct sal_port)));

	/*Card id */
	sal_card->card_no = card_no;

	/* Card physical position*/
	sal_card->position = card_pos;


	for (i = 0; i < phy_num; i++) {

		sal_card->phy[i] = (struct sal_phy *)(u64)
		    ((u8 *) sal_card + sizeof(struct sal_card) +
		     (u64) i * sizeof(struct sal_phy));
		sal_card->phy[i]->phy_id = (u8) i;
		sal_card->phy[i]->port_id = SAL_INVALID_PORTID;

		sal_card->phy[i]->phy_err_flag = SAL_PHY_ERR_CODE_NO;
		sal_card->phy[i]->link_rate = SAL_LINK_RATE_FREE;
		sal_card->phy[i]->phy_reset_flag = SAL_PHY_NEED_RESET_FOR_DFR;
		sal_card->phy[i]->connect_wire_type = SAL_IDLE_WIRE;	/*phy default state is IDLE */ 

		sal_card->port[i] =
		    (struct sal_port *)(u64) ((u8 *) sal_card +
					      sizeof(struct sal_card) +
					      (u64) phy_num *
					      sizeof(struct sal_phy)
					      +
					      (u64) i *
					      sizeof(struct sal_port));

		sal_card->port[i]->card = sal_card;

		SAS_ATOMIC_SET(&sal_card->port[i]->dev_num, 0);
		SAS_ATOMIC_SET(&sal_card->port[i]->pend_abort, 0);
		SAS_ATOMIC_SET(&sal_card->port[i]->port_in_use,
			       SAL_PORT_RSC_UNUSE);
		sal_card->port[i]->port_need_do_full_disc = SAL_TRUE;
		sal_card->port[i]->port_id = SAL_INVALID_PORTID;
		sal_card->port[i]->index = i;
		sal_card->port[i]->logic_id = SAL_INVALID_LOGIC_PORTID;
		sal_card->port[i]->phy_up_in_port = SAL_NO_PHY_UP_IN_PORT;
		sal_card->port[i]->host_disc = false;
//        pstSalCard->port[i]->uiPortMapIndex = 0;

		/*record port cable insert or  pull out state  */
		sal_card->port_curr_wire_type[i] = SAL_IDLE_WIRE;	/*default state of port is IDLE */
		sal_card->port_last_wire_type[i] = SAL_IDLE_WIRE;	/*record insert or pull out state */

		for (j = 0; j < SAL_MAX_PHY_NUM; j++)
			sal_card->port[i]->phy_id_list[j] = false;

		/*initial every port link state flag. */
		sal_card->port[i]->status = SAL_PORT_FREE;
		sal_card->port[i]->is_conn_high_jbod = SAL_FALSE;
	}

	INIT_DELAYED_WORK(&sal_card->port_led_work, sal_port_led_func);
	SAS_SPIN_LOCK_INIT(&sal_card->card_lock);
	SAS_SemaInit(&sal_card->card_mutex, 1);

	sal_card->eh_thread = NULL;
	sal_card->event_thread = NULL;

	spin_lock_irqsave(&sal_card->card_lock, flags);
	sal_card->reseting = SAL_FALSE;	/*allow chip reset. */
	sal_card->phy_num = phy_num;
	sal_card->eh_mutex_flag = true;
	SAS_ATOMIC_SET(&sal_card->queue_mutex, 0);
	spin_unlock_irqrestore(&sal_card->card_lock, flags);

	SAS_ATOMIC_SET(&sal_card->msg_cnt, 0);
	SAS_ATOMIC_SET(&sal_card->port_ref_cnt, 0);

	/*global  Card Slot info */
	spin_lock_irqsave(&card_lock, flags);

	SAS_ATOMIC_SET(&sal_card->ref_cnt, 1);

	sal_global.sal_cards[card_no] = sal_card;
	spin_unlock_irqrestore(&card_lock, flags);
	return sal_card;

      fail_done:
	sal_global.init_failed_reason[card_no] = SAL_RSC_ALLOC_FAILED;
	return NULL;
}

EXPORT_SYMBOL(sal_alloc_card);

/*----------------------------------------------*
 * stop event module.                           *
 *----------------------------------------------*/
void sal_remove_event_module(struct sal_card *sal_card)
{
	if (NULL != sal_card->event_thread) {
		(void)SAS_KTHREAD_STOP(sal_card->event_thread);
		sal_card->event_thread = NULL;

		SAS_VFREE(sal_card->event_queue);
		sal_card->event_queue = NULL;
	}
}

/*----------------------------------------------*
 * initialize event module.                           *
 *----------------------------------------------*/
s32 sal_init_event_module(struct sal_card *sal_card)
{
	u32 routine_time = 1 * HZ;	/* time interval, unit :tick */
	u32 i = 0;
	struct sal_event *event = NULL;
	unsigned long flag = 0;
	u32 mem_size = SAL_MAX_EVENT_NUM * sizeof(struct sal_event);

	sal_card->event_queue = SAS_VMALLOC(mem_size);
	if (NULL == sal_card->event_queue) {
		SAL_TRACE(OSP_DBL_MAJOR, 378, "card:%d alloc Notify mem fail ",
			  sal_card->card_no);
		return ERROR;
	}
	memset(sal_card->event_queue, 0, mem_size);

	SAS_SPIN_LOCK_INIT(&sal_card->card_event_lock);
	SAS_INIT_LIST_HEAD(&sal_card->event_active_list);
	SAS_INIT_LIST_HEAD(&sal_card->event_free_list);

	for (i = 0; i < SAL_MAX_EVENT_NUM; i++) {
		/* add into free list*/
		event = &sal_card->event_queue[i];

		spin_lock_irqsave(&sal_card->card_event_lock, flag);
		SAS_LIST_ADD_TAIL(&event->list, &sal_card->event_free_list);
		spin_unlock_irqrestore(&sal_card->card_event_lock, flag);
	}

	/*create event process eventQ  thread */
	sal_card->event_thread = OSAL_kthread_run(sal_event_handler,
						  (void *)sal_card,
						  "sal_event/%d",
						  sal_card->card_no);
	if (IS_ERR(sal_card->event_thread)) {
		SAL_TRACE(OSP_DBL_MAJOR, 735,
			  "card:%d init event thread failed, ret:%ld",
			  sal_card->card_no, PTR_ERR(sal_card->event_thread));
		sal_card->event_thread = NULL;

		SAS_VFREE(sal_card->event_queue);
		sal_card->event_queue = NULL;
		return ERROR;
	}
	/*start timer of chip */
	sal_add_timer((void *)&sal_card->routine_timer,
		      (unsigned long)sal_card->card_no, routine_time,
		      sal_routine_timer);
	return OK;
}

/*----------------------------------------------*
 *  add card interface which is supported to driver by SAL.                            *
 *----------------------------------------------*/
s32 sal_add_mid_host(struct sal_card * sal_card)
{
	s32 ret = ERROR;

    struct Scsi_Host* scsi_host = NULL;

	scsi_host = scsi_host_alloc(&hisi_pv660_sht, sizeof(u64));
	if (NULL == scsi_host) {
	    printk("Alloc host failed.");
	    return ERROR;
	}

	sal_card->host_id = scsi_host->host_no;/* preserve host_no of scsi host */
	scsi_host->max_id       = sal_card->config.max_targets;
	scsi_host->this_id      = -1;
	scsi_host->unique_id    = hisi_pv660_id++;
	scsi_host->max_lun      = 8;
	scsi_host->max_channel  = 0;
	scsi_host->sg_tablesize = (u16)SAL_MAX_DMA_SEGS;
	scsi_host->can_queue    = (s32)(sal_card->config.max_can_queues);
	scsi_host->cmd_per_lun  = (s16)(sal_card->config.dev_queue_depth);
	scsi_host->unchecked_isa_dma = B_FALSE;
	scsi_host->max_cmd_len = 16;
	ret = scsi_add_host(scsi_host, sal_card->dev);
	if (OK != ret) {
	    printk("Add  host failed, return %d.", ret);
	}

	return ret;
}

/*----------------------------------------------*
 *  driver IO record timer ,which is used to update IOPS and width info.                   *
 *----------------------------------------------*/
void sal_io_stat_timer(unsigned long arg)
{
	u32 i = 0;
	u32 card_id = 0;
	unsigned long flags = 0;
	struct sal_card *sal_card = NULL;
	struct sal_drv_io_stat *io_stat = NULL;
	struct sal_port_io_stat *port_stat = NULL;

	card_id = (u8) arg;
	sal_card = sal_get_card(card_id);
	if (NULL == sal_card) {
		SAL_TRACE(OSP_DBL_MINOR, 4295, "Get card failed by id:%d",
			  card_id);
		return;
	}

	io_stat = &sal_card->card_io_stat;

	spin_lock_irqsave(&io_stat->io_stat_lock, flags);
	if (SAL_IO_STAT_DISABLE != io_stat->stat_switch) {
		/*
		   *    update IOPS and width info of current PORT
		 */
		for (i = 0; i < sal_card->phy_num; i++) {
			port_stat = &io_stat->port_io_stat[i];
			/* schedule Timer one time per second. */
			port_stat->iops = port_stat->total_io;
			port_stat->band_width = port_stat->total_len;
			/* re-start count*/
			port_stat->total_io = 0;
			port_stat->total_len = 0;
		}

	}
	spin_unlock_irqrestore(&io_stat->io_stat_lock, flags);

	sal_add_timer((void *)&sal_card->io_stat_timer,
		      (unsigned long)sal_card->card_no,
		      (1 * HZ), sal_io_stat_timer);
	sal_put_card(sal_card);
	return;
}

/*----------------------------------------------*
 *  initialize io state.                   *
 *----------------------------------------------*/
void sal_init_io_stat(struct sal_card *sal_card)
{
	memset(&sal_card->card_io_stat, 0, sizeof(struct sal_drv_io_stat));
	SAS_SPIN_LOCK_INIT(&sal_card->card_io_stat.io_stat_lock);

	/*initialize IO count info. */
	sal_card_rsc_init(sal_card);


	sal_add_timer(&sal_card->io_stat_timer,
		      (unsigned long)sal_card->card_no,
		      (1 * HZ), sal_io_stat_timer);
	return;
}

/*----------------------------------------------*
 *  card resource init.                   *
 *----------------------------------------------*/
void sal_card_rsc_init(struct sal_card *sal_card)
{
	unsigned long flag = 0;

	spin_lock_irqsave(&sal_card->card_io_stat.io_stat_lock, flag);

	SAS_ATOMIC_SET(&sal_card->card_io_stat.active_io_cnt, 0);
	sal_card->card_io_stat.send_cnt = 0;
	sal_card->card_io_stat.response_cnt = 0;
	sal_card->card_io_stat.eh_io_cnt = 0;
	sal_card->card_io_stat.stat_switch = SAL_IO_STAT_DISABLE;	/*default: forbid IO count */

	spin_unlock_irqrestore(&sal_card->card_io_stat.io_stat_lock, flag);

	return;
}

EXPORT_SYMBOL(sal_card_rsc_init);

/*----------------------------------------------*
 *   add card interface which is supported to driver by SAL.                    *
 *----------------------------------------------*/
//lint -e801
s32 sal_add_card(struct sal_card * sal_card)
{
	s32 ret = ERROR;

	SAL_ASSERT(NULL != sal_card, return ERROR);

	//OSAL_INIT_TIMER((OSAL_TIMER_LIST_S *) & sal_card->routine_timer);
	(void)osal_lib_init_timer((void *)&sal_card->routine_timer,
			     (unsigned long)100,
			     (unsigned long)sal_card->card_no,
			     sal_routine_timer);

	//OSAL_INIT_TIMER((OSAL_TIMER_LIST_S *) & sal_card->io_stat_timer);
	(void)osal_lib_init_timer((void *)&sal_card->io_stat_timer,
			     (unsigned long)1 * HZ,
			     (unsigned long)sal_card->card_no,
			     sal_io_stat_timer);

	/*
	OSAL_INIT_TIMER((OSAL_TIMER_LIST_S *) & sal_card->chip_op_timer);
	
	(void)osal_lib_init_timer((void *)&sal_card->chip_op_timer,
			     (unsigned long)100,
			     (unsigned long)sal_card->card_no,
			     sal_restore_dly_time_handler);
	*/
	//OSAL_INIT_TIMER((OSAL_TIMER_LIST_S *) & sal_card->port_op_timer);
	(void)osal_lib_init_timer((void *)&sal_card->port_op_timer,
			     (unsigned long)100,
			     (unsigned long)sal_card->card_no,
			     sal_wide_port_opt_func);


	ret = sal_add_mid_host(sal_card);
	if (OK != ret) {
		sal_global.init_failed_reason[sal_card->card_no] =
		    SAL_SCSI_HOST_ALLOC_FAILED;
		return ERROR;
	} else {
		SAL_TRACE(OSP_DBL_INFO, 735,
			  "Card:%d alloc host ok, Initiator No:%d",
			  sal_card->card_no, sal_card->host_id);
	}

	/* IO module */
	ret = sal_err_notify_rsc_init(sal_card);
	if (OK != ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 735, "card:%d init nofity rsc fail",
			  sal_card->card_no);
		sal_global.init_failed_reason[sal_card->card_no] =
		    SAL_IO_ROUTINE_INIT_FAILED;
		goto FAIL;
	}

	if (OK != sal_dev_init(sal_card)) {
		SAL_TRACE(OSP_DBL_MAJOR, 735, "card:%d init dev rsc fail",
			  sal_card->card_no);
		goto FAIL;
	}

	if (OK != sal_msg_rsc_init(sal_card)) {
		SAL_TRACE(OSP_DBL_MAJOR, 735, "card:%d init msg rsc fail",
			  sal_card->card_no);
		goto FAIL;
	}

	/* Disc module */
	ret = sal_init_disc_rsc(sal_card->card_no, &disc_event_func);
	if (OK != ret) {
		sal_global.init_failed_reason[sal_card->card_no] =
		    SAL_DISC_ROUTINE_INIT_FAILED;
		goto FAIL;
	}

	/* Event module */
	ret = sal_init_event_module(sal_card);
	if (OK != ret) {
		sal_global.init_failed_reason[sal_card->card_no] =
		    SAL_EVENT_ROUTINE_INIT_FAILED;
		goto FAIL;
	}
    
#if 0
	/*缓上报模块初始化 */
	ret = sasini_init_dev_chg_delay(sal_card->card_no,
					sal_card->config.max_targets,
					sal_card->config.dev_miss_dly_time,
					sal_notify_report_dev_in,
					sal_free_dev_rsc, sal_put_all_dev);
	if (OK != ret) {
		sal_global.init_failed_reason[sal_card->card_no] =
		    SAL_DEV_DELAY_INIT_FAILED;
		goto FAIL;
	}

	/* 线缆插拔事件处理模块 */
	ret = sal_wire_notify_rsc_init(sal_card);
	if (OK != ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 735, "card:%d init wire nofity fail",
			  sal_card->card_no);
		sal_global.init_failed_reason[sal_card->card_no] =
		    SAL_WIRE_ROUTINE_INIT_FAILED;
		goto FAIL;
	}
#endif

	/*  IO count module*/
	sal_init_io_stat(sal_card);

	return OK;

      FAIL:

	(void)sal_dev_remove(sal_card);
	(void)sal_msg_rsc_remove(sal_card);
	(void)sal_err_notify_remove(sal_card);

	sal_release_disc_rsc(sal_card->card_no);
	sal_remove_event_module(sal_card);
	sal_del_timer((void *)&sal_card->routine_timer);

	return ERROR;

}

EXPORT_SYMBOL(sal_add_card);

/*----------------------------------------------*
 *   abort IO of all device in port.                    *
 *----------------------------------------------*/
s32 sal_abort_all_dev_io(struct sal_card * sal_card)
{
	struct list_head *node = NULL;
	struct sal_dev *dev = NULL;
	unsigned long flag = 0;
	s32 ret = ERROR;
	unsigned long dev_flag = 0;
	bool abort_this_dev;
	unsigned long wait_start_jiff;

	if (NULL == sal_card->ll_func.eh_abort_op) {
		SAL_TRACE(OSP_DBL_MAJOR, 599,
			  "Card %d abort disk IO function is null",
			  sal_card->card_no);
		return ERROR;
	}

	SAS_ATOMIC_SET(&sal_card->pend_abort, 0);

	spin_lock_irqsave(&sal_card->card_dev_list_lock, flag);
	SAS_LIST_FOR_EACH(node, &sal_card->active_dev_list) {
		dev = SAS_LIST_ENTRY(node, struct sal_dev, dev_list);
		abort_this_dev = true;
		sal_get_dev(dev);

		spin_lock_irqsave(&dev->dev_state_lock, dev_flag);
		if (dev->dev_status != SAL_DEV_ST_ACTIVE) {
			SAL_TRACE(OSP_DBL_INFO, 600,
				  "Card:%d disk:0x%llx not active(ST:%d)",
				  sal_card->card_no, dev->sas_addr,
				  dev->dev_status);
			abort_this_dev = false;
		}
		spin_unlock_irqrestore(&dev->dev_state_lock, dev_flag);

		if (true != abort_this_dev) {
			sal_put_dev_in_dev_list_lock(dev);
			continue;
		}

		SAL_TRACE(OSP_DBL_DATA, 601,
			  "Card:%d port:%d abort disk:0x%llx IO",
			  sal_card->card_no, dev->port->port_id, dev->sas_addr);

		SAS_ATOMIC_INC(&sal_card->pend_abort);
		ret = sal_card->ll_func.eh_abort_op(SAL_EH_ABORT_OPT_DEV, dev);
		if (SAL_CMND_EXEC_SUCCESS != ret) {
			SAS_ATOMIC_DEC(&sal_card->pend_abort);
			SAL_TRACE(OSP_DBL_MAJOR, 602,
				  "Card:%d abort disk:0x%llx IO failed,continue",
				  sal_card->card_no, dev->sas_addr);
		}
		/* run out of in the cycles*/
		sal_put_dev_in_dev_list_lock(dev);
	}
	spin_unlock_irqrestore(&sal_card->card_dev_list_lock, flag);

	wait_start_jiff = jiffies;
	while (jiffies_to_msecs(jiffies - wait_start_jiff) <=
	       SAL_ABORT_DEV_IO_TIMEOUT) {
		/* in the timeout time scopt.  */
		if (0 == SAS_ATOMIC_READ(&sal_card->pend_abort)) {
			SAL_TRACE(OSP_DBL_INFO, 603,
				  "Card:%d abort all dev over, cost %d ms",
				  sal_card->card_no,
				  jiffies_to_msecs(jiffies - wait_start_jiff));
			return OK;
		}
		SAS_MSLEEP(10);
	}
	SAL_TRACE(OSP_DBL_MAJOR, 603,
		  "Card:%d abort dev req:%d timeout,cost %d ms",
		  sal_card->card_no, SAS_ATOMIC_READ(&sal_card->pend_abort),
		  jiffies_to_msecs(jiffies - wait_start_jiff));
	return ERROR;
}

EXPORT_SYMBOL(sal_abort_all_dev_io);

/*----------------------------------------------*
 *   called the function to release scsi_host/tgt host resource when driver unload card.                    *
 *----------------------------------------------*/
void sal_remove_host(struct sal_card *sal_card)
{
    struct Scsi_Host* host;

	if ((SAL_PORT_MODE_INI == sal_card->config.work_mode)
	    || (SAL_PORT_MODE_BOTH == sal_card->config.work_mode)) {

		SAL_TRACE(OSP_DBL_INFO, 735,
			  "Card:%d is going to remove host Initiator No:%d",
			  sal_card->card_no, sal_card->host_id);
#if 0 /* remove sas_adpt.ko */        
		DRV_RemoveInitiator(sal_card->scsi_host.InitiatorNo);
#else
        host = (struct Scsi_Host *)scsi_host_lookup((u16)sal_card->host_id);
		if (IS_ERR(host)) {
            printk("scsi host %d is not exsit\n", sal_card->host_id);
			return;
        }
		scsi_remove_host(host);
		scsi_host_put(host);
#endif
	} else {
		/*release tgt host*/
	}

	SAL_TRACE(OSP_DBL_INFO, 735, "Card:%d remove host done",
		  sal_card->card_no);
	return;
}

EXPORT_SYMBOL(sal_remove_host);

/*----------------------------------------------*
 *  delete all the timer which is running.       *
 *----------------------------------------------*/
void sal_del_all_running_timer(struct sal_card *sal_card)
{
	SAL_TRACE(OSP_DBL_INFO, 739,
		  "Card:%d is going to del routine test timer",
		  sal_card->card_no);
	/*delete chip about timer */
	sal_del_timer((void *)&sal_card->routine_timer);

	SAL_TRACE(OSP_DBL_INFO, 739, "Card:%d is going to del IO stat timer",
		  sal_card->card_no);
	/*delete IO count timer */
	sal_del_timer((void *)&sal_card->io_stat_timer);

	/*delete light led timer */
	sal_del_timer((void *)&sal_card->port_op_timer);

	SAL_TRACE(OSP_DBL_INFO, 740, "Card:%d is going to del chip oper timer",
		  sal_card->card_no);


	//sal_del_timer((void *)&sal_card->chip_op_timer);

	SAL_TRACE(OSP_DBL_INFO, 740, "Card:%d is going to del led queue",
		  sal_card->card_no);


	if (delayed_work_pending(&sal_card->port_led_work))
		(void)cancel_delayed_work(&sal_card->port_led_work);


	flush_workqueue(sal_wq);

	return;
}

/*----------------------------------------------*
 *    called the function to logout low-layer chip when driver unload card.  *
 *----------------------------------------------*/
void sal_remove_chip_oper_handler(struct sal_card *sal_card)
{

	sal_card->ll_func.chip_op = NULL;
	sal_card->ll_func.phy_op = NULL;
	sal_card->ll_func.port_op = NULL;
	sal_card->ll_func.dev_op = NULL;
	sal_card->ll_func.eh_abort_op = NULL;
	sal_card->ll_func.send_msg = NULL;
}

/*----------------------------------------------*
 *    delete IO module , include error handle and jam module.  *
 *----------------------------------------------*/
void sal_remove_err_handler(struct sal_card *sal_card)
{
	SAL_REF(sal_card);
	/*  1. delete thread. */
	/*  2. release resource  */
	(void)sal_err_notify_remove(sal_card);
	return;
}

/*----------------------------------------------*
 *    delete much thread , assure no outer event .   *
 *----------------------------------------------*/
void sal_remove_threads(struct sal_card *sal_card)
{
	SAL_ASSERT(NULL != sal_card, return);
	/* forbid discover */
	(void)sal_stop_card_disc(sal_card->card_no);

	/* release error handle module*/
	sal_remove_err_handler(sal_card);
#if 0
	/*释放线缆插拔事件线程 */
	sal_wire_notify_remove(sal_card);
#endif
	/*release Disc */
	sal_release_disc_rsc(sal_card->card_no);
}

EXPORT_SYMBOL(sal_remove_threads);


/*----------------------------------------------*
 *    notify discover module to stop start discover.  *
 *----------------------------------------------*/
void sal_notify_disc_shut_down(struct sal_card *sal_card)
{
	SAL_ASSERT(NULL != sal_card, return);
	sal_set_disc_card_false(sal_card->card_no);
}

EXPORT_SYMBOL(sal_notify_disc_shut_down);

/*----------------------------------------------*
 *    get card flag.  *
 *----------------------------------------------*/
u32 sal_get_card_flag(struct sal_card * sal_card)
{
	unsigned long irq_flag = 0;
	u32 ret = 0;

	spin_lock_irqsave(&sal_card->card_lock, irq_flag);
	ret = sal_card->flag;
	spin_unlock_irqrestore(&sal_card->card_lock, irq_flag);
	return ret;
}

/*----------------------------------------------*
 *    called the function to unload SAL when driver unload card.          *
 *----------------------------------------------*/
void sal_remove_card(struct sal_card *sal_card)
{
	SAL_ASSERT(NULL != sal_card, return);

	sal_del_all_running_timer(sal_card);	/* delete all runnig timer.*/

	sal_remove_event_module(sal_card);

	sal_complete_all_io(sal_card);


	sal_remove_all_dev(sal_card->card_no);

	/* when work mode is INI , then release scsi host to called SAL_RemoveHost function    */
	sal_remove_host(sal_card);

	/* release SAL DEV resource */
	(void)sal_dev_remove(sal_card);

	/* release SAL Msg resource */
	(void)sal_msg_rsc_remove(sal_card);

	/*logout option interface of low-layer chip */
	sal_remove_chip_oper_handler(sal_card);

}

EXPORT_SYMBOL(sal_remove_card);

/*----------------------------------------------*
 *    initialize global varible resource of SAL layer.    *
 *----------------------------------------------*/
void sal_init_global_var(void)
{
	u32 i;
	memset(sal_global.init_failed_reason, SAL_SLOT_NO_CARD,
	       sizeof(sal_global.init_failed_reason));
	for (i = 0; i < SAL_MAX_CARD_NUM; i++)
		sal_global.sal_cards[i] = NULL;
	/*max value of io_stat cnt is 12 */
	sal_global.io_stat_threshold[0] = 512;
	sal_global.io_stat_threshold[1] = 4 * 1024;
	sal_global.io_stat_threshold[2] = 8 * 1024;
	sal_global.io_stat_threshold[3] = 16 * 1024;
	sal_global.io_stat_threshold[4] = 32 * 1024;
	sal_global.io_stat_threshold[5] = 64 * 1024;
	sal_global.io_stat_threshold[6] = 128 * 1024;
	sal_global.io_stat_threshold[7] = 256 * 1024;
	sal_global.io_stat_threshold[8] = 512 * 1024;
	sal_global.io_stat_threshold[9] = 1024 * 1024;
	sal_global.io_stat_threshold[10] = 2 * 1024 * 1024;
	sal_global.io_stat_threshold[11] = 4 * 1024 * 1024;

}

/*----------------------------------------------*
 *    SAL module load function.    *
 *----------------------------------------------*/

#ifndef _PCLINT_
static
#endif
s32 __init sal_module_init(void)
{
	s32 ret = ERROR;


	SAS_SPIN_LOCK_INIT(&card_lock);
	/*initialize SAL resource  */
	sal_init_global_var();




	memset(sal_global.fw_dump_idx, 0x00, sizeof(sal_global.fw_dump_idx));

	/*register mml */
#ifdef SUPPORT_MML

	//  SAL_InitMML();//only for debug arm server
#endif

	/*IO error handle function and array initialize*/
	sal_init_err_handler_op();

	/*event handle function initialize  */
	sal_event_init();



	/* initialize dev state machine  */
	sal_dev_fsm_init();

	/*initialize msg state machine */
	sal_msg_fsm_init();

	/* initialize Disc resource */
	ret = sal_disc_module_init();
	if (OK != ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 70, "Init disc module fail ret 0x%x",
			  ret);
		return -ENOMEM;
	}
#if 0
	/* 初始化驱动IO统计资源 */
	SAL_InitDrvIOStat();
#endif

	//(void)sal_comm_set_debug_level(sal_global.sal_log_level);

	/*initialize  SCSI to ATA array*/
	sat_init_trans_array();

	/*control thread initialize */
	ret = sal_card_ctrl_init();
	if (OK != ret) {
		SAL_TRACE(OSP_DBL_MAJOR, 743,
			  "sal module of Ctrl thread init failed");
		return ret;
	}

	sal_global.sal_mod_init_ok = true;

	/*create unattached work queue. */
	sal_wq = create_singlethread_workqueue("sal_layer");
	if (!sal_wq) {
		SAL_TRACE(OSP_DBL_MAJOR, 743,
			  "Sal module create a single work queue failed");
		return ERROR;
	}

	SAL_TRACE(OSP_DBL_INFO, 744,
		  "sal module init ,soft ver:%s (complied at:%s %s)",
		  sal_sw_ver, sal_compile_date, sal_compile_time);
	return 0;
}

/*----------------------------------------------*
 *    SAL module unload function.    *
 *----------------------------------------------*/
#ifndef _PCLINT_
static
#endif
void __exit sal_module_exit(void)
{

	/* destory work queue. */
	destroy_workqueue(sal_wq);

	sal_global.sal_mod_init_ok = false;


	//SAL_UnregCliDebugCmd();//only for debug arm server


#ifdef SUPPORT_MML

	//SAL_ExitMML();//only for debug arm server
#endif
	/*stop discover module  */
	sal_disc_module_remove();

	/*event handle function logout*/
	sal_event_exit();

	/*remove IO err handler func after err handler thread */
	sal_remove_err_handler_op();
#if 0
	/* 注销驱动IO统计 */
	SAL_FreeDrvIOStat();

	/*销毁IO pool */
	kmem_cache_destroy(g_pstIOCachep);
#endif

	sal_card_ctrl_remove();

	SAL_TRACE(OSP_DBL_INFO, 745, "sal module exit");
}

#if 0
/*****************************************************************************
 函 数 名  : sal_get_cardid_by_pcid_dev
 功能描述  : 从pcie dev得到slot号
 输入参数  : struct pci_dev             *dev  
             const struct pci_device_id *dev_id  
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
u32 sal_get_cardid_by_pcid_dev(struct pci_dev *dev)
{
	u8 card_pos = 0;
	u8 card_no = 0;

	/*从BSP接口获取当前卡位置 */
        if(NULL == sal_peripheral_operation.get_dev_position)
       {
               card_pos = 10;
       }else if (OK != sal_peripheral_operation.get_dev_position(dev->bus->number, &card_pos)) {
		SAL_TRACE(OSP_DBL_MAJOR, 4618,
			  "Get card positon failed by pcie bus id:0x%x",
			  dev->bus->number);
		return 0xFF;
	}
	/* 低5位表示槽位号,槽位号与卡号一致 */
	card_no = card_pos & 0x1f;
	if (card_no >= SAL_MAX_CARD_NUM) {
		SAL_TRACE(OSP_DBL_MAJOR, 4619,
			  "Sas card position:0x%x exceed than max support number",
			  card_no);
		return 0xFF;
	}
	return card_no;
}

EXPORT_SYMBOL(sal_get_cardid_by_pcid_dev);
#endif


/*----------------------------------------------*
 *   convert string to number.    *
 *----------------------------------------------*/
s32 sal_special_str_to_ui(const s8 * cp, u32 base, u32 * result)
{
	u32 value = 0;

	if (!base) {
		base = 10;
		if (*cp == '0') {
			if ('\0' == *(cp + 1)) {
				*result = 0;
				return OK;
			}

			base = 8;
			cp++;
			if (SAL_LOWERANDX(cp)) {
				cp++;
				base = 16;
			}
		}
	} else if (base == 16) {
		if (SAL_LOWERAND0(cp))
			cp += 2;
	}

	if ('\0' == *cp)
		return ERROR;

	while (isxdigit(*cp)
	       &&
	       ((value =
		 (isdigit(*cp) ? *cp - '0' : (TOLOWER(*cp) - 'a') + 10)) <
		base)) {
		*result = *result * base + value;
		cp++;
	}

	if ('\0' == *cp)
		return OK;

	return ERROR;
}

EXPORT_SYMBOL(sal_special_str_to_ui);

module_init(sal_module_init);
module_exit(sal_module_exit);
MODULE_LICENSE("GPL");

//lint -e24 -e40 -e63 -e35 -e10 -e34 -e785
//lint +e24 +e40 +e63 +e35 +e10 +e34 +e785

//lint +e679 +e19


