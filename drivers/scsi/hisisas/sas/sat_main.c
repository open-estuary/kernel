/*
 * Support for SATA devices on Serial Attached SCSI (SAS) Huawei Pv660/Hi1610 controllers
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
#include "sal_io.h"

/*SAT debug level,have higher priority than common debug level*/
u32 sat_dbg_level = OSP_DBL_BUTT;
extern s32(*sat_translation_table[SCSIOPC_MAX]) (struct stp_req *,
						 struct sat_ini_cmnd *,
						 struct sata_device *);
extern void sal_stp_done(struct sal_msg *msg);

/*get CDB legth for group code,group code is bit 5-7 of operation code,
 *group 3,6,and 7 is reserved,by default we assume them as 10 byte command*/
static u8 grp_code_to_len[] = { 6, 10, 10, 10, 16, 12, 10, 10 };

/*Functions provided by other modules*/

s32 sat_alloc_ncq_tag(struct sata_device *sata_dev, u8 * v_pucTag);
void sat_free_ncq_tag(struct sata_device *sata_dev, u8 v_ucTag);
void sat_identify_cb(struct stp_req *stp_request, u32 io_status);
s32 sat_send_identify(struct stp_req *stp_cmd);
void sal_adjust_queue_depth(u16 host_id, u16 chan_id, u16 target_id, u16 lun_id, s32 tags);

void *sat_get_req_buf(struct stp_req *stp_request)
{
	SAT_ASSERT(NULL != stp_request, return NULL);
	return stp_request->indirect_rsp_virt_addr;
}

/**
 * sat_get_new_msg - obtain a new sata io message
 * @sata_dev: sata device
 * @dma_size: dma size for new io message
 *
 */
struct stp_req *sat_get_new_msg(struct sata_device *sata_dev, u32 dma_size)
{
	struct sal_msg *msg = NULL;
	struct sal_dev *sal_dev = NULL;
	struct sal_card *sal_card = NULL;
	void *vir_addr = NULL;
	unsigned long phy_addr = 0;
	s32 ret = 0;

	sal_dev = (struct sal_dev *)sata_dev->upper_dev;
	SAT_ASSERT(NULL != sal_dev, return NULL);

	sal_card = sal_dev->card;
	SAT_ASSERT(NULL != sal_card, return NULL);

	msg = sal_get_msg(sal_card);
	if (NULL == msg) {
		SAT_TRACE(OSP_DBL_MAJOR, 0, "dev:0x%llx get msg failed",
			  sata_dev->dev_id);
		return NULL;
	}

	(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_IOSTART);

	msg->dev = sal_dev;
	msg->ini_cmnd.data_len = 0;
	msg->data_direction = SAL_MSG_DATA_NONE;
	msg->protocol.stp.upper_msg = (void *)msg;
	msg->type = SAL_MSG_PROT_STP;
    /*inner IO */
	msg->io_type = SAL_MSG_TYPE_INT_IO;

	if (0 != dma_size) {
		ret = sal_alloc_dma_mem(sal_card,
					&vir_addr,
					&phy_addr,
					dma_size, PCI_DMA_FROMDEVICE);
		if (ERROR == ret) {
			SAT_TRACE(OSP_DBL_MAJOR, 0,
				  "Dev:0x%llx Alloc DMA mem failed",
				  sal_dev->sas_addr);
			(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_SEND_FAIL);
			return NULL;
		}

		msg->protocol.stp.indirect_rsp_phy_addr = phy_addr;
		msg->protocol.stp.indirect_rsp_virt_addr = vir_addr;
		msg->protocol.stp.indirect_rsp_len = dma_size;
		msg->protocol.stp.sgl_cnt = 1;

		msg->ini_cmnd.data_len = dma_size;
		msg->data_direction = SAL_MSG_DATA_FROM_DEVICE;
	}

	return &msg->protocol.stp;
}

/**
 * sat_free_int_msg - release a sata io message
 * @stp_req: stp request message to be release
 *
 */
void sat_free_int_msg(struct stp_req *stp_req)
{
	struct sal_msg *msg = NULL;
	struct sal_card *sal_card = NULL;
	unsigned long flag = 0;

	SAT_ASSERT(NULL != stp_req, return);
	msg = (struct sal_msg *)stp_req->upper_msg;
	SAT_ASSERT(NULL != msg, return);

	sal_card = msg->card;

	spin_lock_irqsave(&msg->msg_state_lock, flag);
	if (SAL_MSG_ST_SEND == msg->msg_state) {
		if (0 != stp_req->indirect_rsp_len) {
			sal_free_dma_mem(sal_card,
					 stp_req->indirect_rsp_virt_addr,
					 stp_req->indirect_rsp_phy_addr,
					 stp_req->indirect_rsp_len,
					 PCI_DMA_FROMDEVICE);
		}
		(void)sal_msg_state_chg_no_lock(msg, SAL_MSG_EVENT_DONE_MID);
	} else {
		SAT_TRACE(OSP_DBL_MAJOR, 0,
			  "Msg:%p(ST:%d,internal IO type:%d) come back,But not free rsc",
			  msg, msg->msg_state, msg->io_type);
	}
	spin_unlock_irqrestore(&msg->msg_state_lock, flag);

	/*send -> free; err handle -> pending; aborting -> aborted */
	(void)sal_msg_state_chg(msg, SAL_MSG_EVENT_DONE_UNUSE);

	return;
}

s32 sal_err_to_sat_err(enum sal_cmnd_exec_result sal_err)
{
	switch (sal_err) {
	case SAL_CMND_EXEC_SUCCESS:
		return SAT_IO_SUCCESS;
	case SAL_CMND_EXEC_BUSY:
		return SAT_IO_BUSY;
	case SAL_CMND_EXEC_FAILED:
	case SAL_CMND_EXEC_BUTT:
	default:
		return SAT_IO_FAILED;
	}

}

void sat_notify_io_done(struct sal_msg *stp_msg)
{

	sal_set_msg_result(stp_msg);
	/*
	 *IO done,just notify upper layer
	 */
	sal_msg_done(stp_msg);

	return;
}

void sat_io_completed(struct stp_req *stp_req, s32 status)
{
	struct sal_msg *sal_msg = NULL;

	SAT_ASSERT(NULL != stp_req, return);
	sal_msg = (struct sal_msg *)stp_req->upper_msg;
	SAT_ASSERT(NULL != sal_msg, return);

	/*
	 *IO complete handle,when status is set to SUCCESS,which means sense info 
	 *and command result are already set,just release resource here and then 
	 *notifiy upper layer,otherwise we need to set command result for current 
	 *command
	 */
	switch (status) {
	case SAT_IO_SUCCESS:
		sal_msg->status.drv_resp = SAL_MSG_DRV_SUCCESS;
		sat_notify_io_done(sal_msg);
		break;

	case SAT_IO_FAILED:
		sal_msg->status.drv_resp = SAL_MSG_DRV_FAILED;
		sat_notify_io_done(sal_msg);
		break;

	case SAT_IO_COMPLETE:
		sat_notify_io_done(sal_msg);
		break;
	case SAT_IO_BUSY:
	case SAT_IO_QUEUE_FULL:
		sal_msg->status.drv_resp = SAL_MSG_DRV_FAILED;
		sat_notify_io_done(sal_msg);
		break;

	default:
		/*should never happen */
		SAT_TRACE(OSP_DBL_MAJOR, 0, "disk:0x%llx io complete status:%u",
			  stp_req->dev->dev_id, status);
		SAS_DUMP_STACK();
		sal_msg->status.drv_resp = SAL_MSG_DRV_FAILED;
		sat_notify_io_done(sal_msg);
		break;
	}

	return;
}

/**
 * sal_dispatch_command - call chip driver to send stp io.
 * @stp_req: stp request io message to be send
 *
 */
enum sal_cmnd_exec_result sal_dispatch_command(struct stp_req *stp_req)
{
	struct sal_card *card = NULL;
	struct sal_msg *msg = NULL;

	SAL_ASSERT(NULL != stp_req, return ERROR);
	SAL_ASSERT(NULL != stp_req->upper_msg, return ERROR);

	msg = (struct sal_msg *)stp_req->upper_msg;
	card = msg->dev->card;
	msg->done = sal_stp_done;
	return card->ll_func.send_msg(msg);
}

/**
 * sat_send_req_to_dev - send stp prepare:TAG alloc ,deal with ncq and non-ncq io.
 * @stp_req: stp request io message to be send
 *
 */
s32 sat_send_req_to_dev(struct stp_req * stp_req)
{
	s32 ret = 0;
	enum sal_cmnd_exec_result result = SAL_CMND_EXEC_BUTT;
	struct sata_device *sata_dev = NULL;
	struct sata_h2d *h2dfis = NULL;
	unsigned long flag = 0;

	/*
	 *1,NCQ tag alloc
	 *2,NCQ and NON-NCQ mixture handle
	 */
	sata_dev = stp_req->dev;
	h2dfis = &stp_req->h2d_fis;

	if (SAT_IS_NCQ_REQ(h2dfis->header.command)) {
		 /*NCQ*/ if (sata_dev->pending_non_ncq_io != 0) {
			SAT_TRACE(OSP_DBL_MAJOR, 0,
				  "disk:0x%llx pending non-ncq io isn't 0",
				  stp_req->dev->dev_id);
			return SAT_IO_QUEUE_FULL;
		}

		if (OK != sat_alloc_ncq_tag(stp_req->dev, &stp_req->ncq_tag)) {
			SAT_TRACE(OSP_DBL_MAJOR, 0,
				  "disk:0x%llx alloc ncq tag failed,bitmap:0x%x",
				  stp_req->dev->dev_id,
				  stp_req->dev->ncq_tag_bitmap);
			return SAT_IO_QUEUE_FULL;
		}
		h2dfis->data.sector_cnt |= (u8) (stp_req->ncq_tag << 3);
	} else {
		/*NON-NCQ */
		if (sata_dev->pending_ncq_io != 0) {
			/*Check power mode and read log ext should always be sent to disk driver */
			if ((FIS_COMMAND_CHECK_POWER_MODE !=
			     h2dfis->header.command)
			    && (FIS_COMMAND_READ_LOG_EXT !=
				h2dfis->header.command)) {
				SAT_TRACE_LIMIT(OSP_DBL_MINOR,
						msecs_to_jiffies(3000), 1, 0,
						"disk:0x%llx(cmd:0x%x) is neither check power mode nor read log ext,Queue is full",
						stp_req->dev->dev_id,
						h2dfis->header.command);
				return SAT_IO_QUEUE_FULL;
			}
		} else if (sata_dev->pending_non_ncq_io != 0) {
			/*Device Reset can be set to disk */
			if (!SAT_IS_RESET_OR_DERESET_FIS(h2dfis)) {
				SAT_TRACE_LIMIT(OSP_DBL_MAJOR,
						msecs_to_jiffies(3000), 3,
						0x1001,
						"Sata cmd %p, msg %p send to disk:0x%llx is not reset cmd",
						stp_req, stp_req->upper_msg,
						stp_req->dev->dev_id);
				return SAT_IO_QUEUE_FULL;
			}

		}
	}

	result = sal_dispatch_command(stp_req);
	ret = sal_err_to_sat_err(result);
	if (ret != SAT_IO_SUCCESS) {
		/*free NCQ tag */
		SAT_TRACE(OSP_DBL_MAJOR, 9,
			  "Device:0x%llx dispatch command failed,ret:%d",
			  sata_dev->dev_id, ret);
		if (SAT_IS_NCQ_REQ(h2dfis->header.command))
			sat_free_ncq_tag(stp_req->dev, stp_req->ncq_tag);
	} else {
		spin_lock_irqsave(&sata_dev->sata_lock, flag);
		sata_dev->pending_io++;
		if (SAT_IS_NCQ_REQ(h2dfis->header.command))
			sata_dev->pending_ncq_io++;
		else
			sata_dev->pending_non_ncq_io++;
		spin_unlock_irqrestore(&sata_dev->sata_lock, flag);
	}

	return ret;
}


void sat_set_dev_transfer_mode(struct sata_device *sata_dev)
{
	if ((SAT_DEV_SUPPORT_NCQ == (sata_dev->dev_cap & SAT_DEV_SUPPORT_NCQ))
	    && (SAT_NCQ_ENABLED == sata_dev->ncq_enable)) {
		sata_dev->dev_transfer_mode = SAT_TRANSFER_MOD_NCQ;
		return;
	}

	if (SAT_DEV_SUPPORT_DMA_EXT ==
	    (sata_dev->dev_cap & SAT_DEV_SUPPORT_DMA_EXT)) {
		sata_dev->dev_transfer_mode = SAT_TRANSFER_MOD_DMA_EXT;
		return;
	}

	if (SAT_DEV_SUPPORT_PIO_EXT ==
	    (sata_dev->dev_cap & SAT_DEV_SUPPORT_PIO_EXT)) {
		sata_dev->dev_transfer_mode = SAT_TRANSFER_MOD_PIO_EXT;
		return;
	}

	if (SAT_DEV_SUPPORT_DMA == (sata_dev->dev_cap & SAT_DEV_SUPPORT_DMA)) {
		sata_dev->dev_transfer_mode = SAT_TRANSFER_MOD_DMA;
		return;
	}

	/*default transfer mode is PIO */
	sata_dev->dev_transfer_mode = SAT_TRANSFER_MOD_PIO;
	return;
}

void sat_calc_max_lba(struct sata_device *device)
{
	struct sata_identify_data *identify = NULL;
	u8 virt_addr[8] = { 0 };
	u64 last_lba = 0;

	identify = &device->sata_identify;

	if (device->dev_cap & SAT_DEV_CAP_48BIT) {
		last_lba = (((u64) identify->max_lba_48_63 << 48)
			    | ((u64) identify->max_lba_32_47 << 32)
			    | ((u64) identify->max_lba_16_31 << 16)
			    | ((u64) identify->max_lba_0_15));
		/* LBA starts from zero */
		last_lba -= 1;
		virt_addr[0] = (u8) ((last_lba >> 56) & 0xFF);	/* MSB */
		virt_addr[1] = (u8) ((last_lba >> 48) & 0xFF);
		virt_addr[2] = (u8) ((last_lba >> 40) & 0xFF);
		virt_addr[3] = (u8) ((last_lba >> 32) & 0xFF);
		virt_addr[4] = (u8) ((last_lba >> 24) & 0xFF);
		virt_addr[5] = (u8) ((last_lba >> 16) & 0xFF);
		virt_addr[6] = (u8) ((last_lba >> 8) & 0xFF);
		virt_addr[7] = (u8) ((last_lba) & 0xFF);	/* LSB */
	} else {
		/* ATA Identify Device information word 60 - 61 */
		last_lba = ((u64) identify->user_addressable_sectors_hi << 16)
		    | ((u64) identify->user_addressable_sectors_lo);
		/* LBA starts from zero */
		last_lba -= 1;
		virt_addr[0] = 0;	/* MSB */
		virt_addr[1] = 0;
		virt_addr[2] = 0;
		virt_addr[3] = 0;
		virt_addr[4] = (u8) ((last_lba >> 24) & 0xFF);
		virt_addr[5] = (u8) ((last_lba >> 16) & 0xFF);
		virt_addr[6] = (u8) ((last_lba >> 8) & 0xFF);
		virt_addr[7] = (u8) ((last_lba) & 0xFF);	/* LSB */
	}

	/* fill in MAX LBA, which is used in SendDiagnostic_1() */
	device->sata_max_lba[0] = virt_addr[0];	/* MSB */
	device->sata_max_lba[1] = virt_addr[1];
	device->sata_max_lba[2] = virt_addr[2];
	device->sata_max_lba[3] = virt_addr[3];
	device->sata_max_lba[4] = virt_addr[4];
	device->sata_max_lba[5] = virt_addr[5];
	device->sata_max_lba[6] = virt_addr[6];
	device->sata_max_lba[7] = virt_addr[7];	/* LSB */

	return;
}

void sat_set_dev_info(struct sata_device *sata_dev,
		      struct sata_identify_data *sata_id_data)
{
	struct sal_card *sal_card = NULL;
	struct sal_dev *sal_dev = NULL;
	u32 queue_depth = 0;

	char *capability_str[] = {
		"Unknown",
		"NCQ",
		"48Bit Addr",
		"NCQ and 48Bit Addr",
		"DMA",
		"Unknown",
		"48Bit Addr and DMA",
		"NCQ, 48Bit Addr and DMA"
	};

	SAT_ASSERT(NULL != sata_id_data, return);
	memcpy(&sata_dev->sata_identify, sata_id_data,
	       sizeof(struct sata_identify_data));

	/* Support NCQ, if Word 76 bit 8 is set */
	if (sata_id_data->sata_cap & 0x100)
		sata_dev->dev_cap |= SAT_DEV_CAP_NCQ;

	/* Support 48 bit addressing, if Word 83 bit 10 and Word 86 bit 10 are set */
	if ((sata_id_data->cmd_set_supported_1 & 0x400)
	    && (sata_id_data->cmd_set_feature_enabled_1 & 0x400))
		sata_dev->dev_cap |= SAT_DEV_CAP_48BIT;

	/* DMA Support, word49 bit8 */
	if ((sata_id_data->dma_lba_iod_ios_timer & 0x100)
	    /* word 88 bit0-6, bit8-14 Ultra DMA supported and enabled, */
	    &&
	    (((sata_id_data->
	       ultra_dma_mode & 0x7f) & ((sata_id_data->
					  ultra_dma_mode >> 8) & 0x7f))
	     /* word 63 bit0-2,bit8-10 Multiword DMA and supported and enabled */
	     || ((sata_id_data->word_62_74[1] & 0x7) &
		 ((sata_id_data->word_62_74[1] >> 8) & 0x7))))
		sata_dev->dev_cap |= SAT_DEV_CAP_DMA;

	sata_dev->iden_valid = SATA_DEV_IDENTIFY_VALID;
	sata_dev->queue_depth = sata_id_data->queue_depth;
	SAT_TRACE(OSP_DBL_INFO, 0,
		  "disk:0x%llx capability:0x%x(%s),Queue Depth:%d",
		  sata_dev->dev_id, sata_dev->dev_cap,
		  capability_str[sata_dev->dev_cap], sata_id_data->queue_depth);

	sal_dev = (struct sal_dev *)sata_dev->upper_dev;
	sal_card = sal_dev->card;

	if (SAT_NCQ_ENABLED == sata_dev->ncq_enable)
		queue_depth = sata_dev->queue_depth + 1;
	else
		queue_depth = 1;

	queue_depth = MIN(queue_depth, sal_card->config.dev_queue_depth);


	sal_adjust_queue_depth((u16)sal_card->host_id, (u16)sal_dev->bus_id, 
		(u16)sal_dev->scsi_id, 0, (s32)queue_depth);

	SAT_TRACE(OSP_DBL_INFO, 6,
		  "Card:%d change SATA dev:0x%llx queue depth to %d",
		  sal_card->card_no, sata_dev->dev_id, queue_depth);

	sat_set_dev_transfer_mode(sata_dev);
	SAT_TRACE_FUNC_EXIT;

	return;
}

void sat_set_sense_and_result(struct stp_req *stp_cmd,
			      u8 sense_key, u16 sense_code)
{
	struct fix_fmt_sns_data *sense_data = NULL;
	struct sal_msg *sal_msg = NULL;

	sal_msg = (struct sal_msg *)stp_cmd->upper_msg;

	sense_data = (struct fix_fmt_sns_data *)sal_msg->status.sense;

	memset(sense_data, 0, sizeof(struct fix_fmt_sns_data));
	sense_data->resp_code = 0x70;	/*error type:Current,sense data format:fix */
	sense_data->sns_key = sense_key;
	sense_data->add_sns_len = 11;	/* Fixed size of sense data 18 bytes */
	sense_data->asc = (u8) ((sense_code >> 8) & 0xFF);
	sense_data->ascq = (u8) (sense_code & 0xFF);
	switch (sense_key) {
		/*
		 * set illegal request sense key specific error in cdb, no bit pointer
		 */
	case SCSI_SNSKEY_ILLEGAL_REQUEST:
		sense_data->sks[0] = 0xC8;
		break;
	default:
		break;
	}

	sal_msg->status.sense_len = sense_data->add_sns_len;

	/*if we return CHECK CONDITION to SCSI mid layer,the result field of SCSI
	 *command's host byte should be set to DID_OK,and status byte should be 
	 *set to CHECK CONDITION*/
	sal_msg->status.io_resp = SAM_STAT_CHECK_CONDITION;
	sal_msg->status.drv_resp = SAL_MSG_DRV_SUCCESS;

	SAT_TRACE_FUNC_EXIT;
}

void sat_init_command(struct stp_req *stp_cmd,
		      struct sat_ini_cmnd *scsi_cmnd,
		      struct sata_device *sata_dev)
{
	struct sal_msg *sal_msg = NULL;

	SAL_ASSERT(NULL != stp_cmd, return);
	SAL_ASSERT(NULL != scsi_cmnd, return);
	SAL_ASSERT(NULL != sata_dev, return);

	sal_msg = (struct sal_msg *)stp_cmd->upper_msg;
	/*initialize some field of ATA command */
	memcpy(&stp_cmd->scsi_cmnd, scsi_cmnd, sizeof(struct sat_ini_cmnd));

	stp_cmd->dev = sata_dev;
	stp_cmd->sense = (union sns_data *)sal_msg->status.sense;
	stp_cmd->ll_msg = stp_cmd;	/*point to it self */
	stp_cmd->org_msg = stp_cmd;	/*point to it self */

	SAS_INIT_LIST_HEAD(&stp_cmd->io_list);
}

void sat_init_internal_command(struct stp_req *stp_cmd,
			       struct stp_req *org_ata_cmd,
			       struct sata_device *sata_dev)
{
	/*initialize some field of ATA command */
	stp_cmd->dev = sata_dev;
	stp_cmd->ll_msg = stp_cmd;	/*point to it self */
	stp_cmd->org_msg = org_ata_cmd;

	org_ata_cmd->ll_msg = stp_cmd;	/*origin messge's low layer message
					   *should point to this command */

	SAS_INIT_LIST_HEAD(&stp_cmd->io_list);
}

s32 sat_process_command(struct stp_req *stp_cmd,
			struct sat_ini_cmnd *scsi_cmnd,
			struct sata_device *sata_dev)
{
	u8 control_byte = 0;
	u8 operation_code = 0;
	u8 group_num = 0;
	s32 ret = SAT_IO_SUCCESS;
	struct stp_req *new_req = NULL;
	char *sata_desc[] = {
		"invalid",
		"Normal",
		"In recovery"
	};

	SAT_ASSERT(stp_cmd != NULL, return SAT_IO_FAILED);
	SAT_ASSERT(scsi_cmnd != NULL, return SAT_IO_FAILED);
	SAT_ASSERT(sata_dev != NULL, return SAT_IO_FAILED);

	/*initialize each field of ATA command */
	sat_init_command(stp_cmd, scsi_cmnd, sata_dev);

	if (unlikely(sata_dev->sata_state != SATA_DEV_STATUS_NORMAL)) {
		SAT_TRACE(OSP_DBL_MAJOR, 11,
			  "disk:0x%llx state:%d isn't normal when CDB[0]:0x%x",
			  sata_dev->dev_id, sata_dev->sata_state,
			  stp_cmd->scsi_cmnd.cdb[0]);
		return SAT_IO_BUSY;
	}

	/*OK,check NACA and LINK,all fixed length CDBs have control byte as their
	 *last byte...since we don't support variable length CDB,we can get control
	 *byte as follows*/
	/*operation code,byte 0 of CDB */
	operation_code = stp_cmd->scsi_cmnd.cdb[0];
	group_num = (operation_code >> SCSI_GROUP_NUMBER_SHIFT);
	/*now..we know the control byte(the last byte of CDB is control byte) */
	control_byte = stp_cmd->scsi_cmnd.cdb[grp_code_to_len[group_num] - 1];

	/* According to SAT,NACA and Link bit should not be set,in fact link bit is
	   * obsolete now */
	if (control_byte & (SCSI_NACA_MASK | SCSI_LINK_MASK)) {
		/*NACA or LINK bit is set,retrun failed to upper layer */
		sat_set_sense_and_result(stp_cmd,
					 SCSI_SNSKEY_ILLEGAL_REQUEST,
					 SCSI_SNSCODE_INVALID_FIELD_IN_CDB);
		SAT_TRACE(OSP_DBL_DATA, 3,
			  "Device 0x%llx,get commnad 0x%x with NACA or"
			  " LINK bit set", sata_dev->dev_id,
			  stp_cmd->scsi_cmnd.cdb[0]);
		SAT_TRACE_FUNC_EXIT;
		return SAT_IO_COMPLETE;
	}

	/*Need to check whether Identify information is valid,if not valid,we send
	 *identify first.If current command is INQUIRY,because INQUIRY will cause a
	 *identify be sent to SATA device,we don't need to send identify here*/
	if ((sata_dev->iden_valid != SATA_DEV_IDENTIFY_VALID)
	    && (operation_code != SCSIOPC_INQUIRY)) {
		/*alloc new command and send identify */
		new_req = sat_get_new_msg(sata_dev, ATA_IDENTIFY_SIZE);
		if (NULL == new_req) {
			SAT_TRACE_LIMIT(OSP_DBL_MINOR, msecs_to_jiffies(3000),
					1, 5,
					"Alloc resource for device:0x%llx failed(cdb:0x%x)",
					sata_dev->dev_id,
					stp_cmd->scsi_cmnd.cdb[0]);
			return SAT_IO_FAILED;
		}

		SAT_TRACE(OSP_DBL_INFO, 9,
			  "Device:0x%llx identify info is invalid,pls send identify(stp:%p msg:%p) firstly",
			  sata_dev->dev_id, new_req, new_req->upper_msg);

		/*OK,send identify */
		sat_init_internal_command(new_req, stp_cmd, sata_dev);
		new_req->compl_call_back = sat_identify_cb;

		ret = sat_send_identify(new_req);
		if (SAT_IO_SUCCESS != ret) {
			SAT_TRACE(OSP_DBL_MINOR, 5,
				  "send identify to dev:0x%llx failed.",
				  sata_dev->dev_id);
			sat_free_int_msg(new_req);
			return ret;
		}

		return SAT_IO_SUCCESS;
	}

	/*Next,do the real ATA to scsi translation */
	if (NULL != sat_translation_table[operation_code]) {
		ret =
		    sat_translation_table[operation_code] (stp_cmd,
							   &stp_cmd->scsi_cmnd,
							   sata_dev);
		return ret;
	} else {
		/*Command not supported yet,return illegal request with ASC/ASCQ set to
		   *INVALID COMMAND OPERATION CODE to SCSI mid layer */
		sat_set_sense_and_result(stp_cmd,
					 SCSI_SNSKEY_ILLEGAL_REQUEST,
					 SCSI_SNSCODE_INVALID_COMMAND);
		SAT_TRACE(OSP_DBL_MAJOR, 4,
			  "Device:0x%llx unsupported commnad:0x%x",
			  sata_dev->dev_id, stp_cmd->scsi_cmnd.cdb[0]);
		SAT_TRACE_FUNC_EXIT;
		return SAT_IO_COMPLETE;
	}
}
