/*
 * Huawei Pv660/Hi1610 sata protocol related implementation
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


extern u32 sal_sg_copy_from_buffer(struct sal_msg *msg, u8 * buff, u32 len);
extern u32 sal_sg_copy_to_buffer(struct sal_msg *msg, u8 * buff, u32 len);


/*SCSI/ATA translation array*/
s32(*sat_translation_table[SCSIOPC_MAX]) (struct stp_req *,
					  struct sat_ini_cmnd *,
					  struct sata_device *) = {
0};

extern u32 sat_dbg_level;

extern void sat_set_sense_and_result(struct stp_req *req,
				     u8 sense_key, u16 sense_code);
extern s32 sat_send_req_to_dev(struct stp_req *req);
extern void sat_free_int_msg(struct stp_req *req);
extern void sat_io_completed(struct stp_req *req, s32 status);
extern void sat_set_dev_info(struct sata_device *sata_dev,
			     struct sata_identify_data *sata_iden_data);
extern void *sat_get_req_buf(struct stp_req *stp_request);
extern struct stp_req *sat_get_new_msg(struct sata_device *sata_dev,
				       u32 dma_size);
extern void sat_init_internal_command(struct stp_req *req,
				      struct stp_req *org_ata_cmd,
				      struct sata_device *sata_dev);

bool sat_field_check(u8 * criterion, u8 * src, u32 len)
{
	u32 bit_idx = 0;
	u32 ct_idx = 0;		
	u8 ct_mask = 0;		
	u32 sc_idx = 0;		
	u8 sc_mask = 0;		
	u32 bit_cnt = 0;	
	u8 ct_val = 0;
	u8 sc_val = 0;

	bit_cnt = len * 8;

	for (bit_idx = 0; bit_idx < bit_cnt; bit_idx++) {
		sc_idx = bit_idx / 8;
		sc_mask = 1 << (7 - bit_idx % 8);
		ct_idx = bit_idx / 4;
		ct_mask =
		    (1 << (7 - (bit_idx % 4) * 2)) | (1 <<
						      (6 - (bit_idx % 4) * 2));

		ct_val =
		    (criterion[ct_idx] & ct_mask) >> (6 - (bit_idx % 4) * 2);
		sc_val = (src[sc_idx] & sc_mask) >> (7 - bit_idx % 8);

		if (ct_val == 0x03)
			continue;
		if (ct_val != sc_val)
			return false;
	}
	return true;
}

void sat_hex_dump(char *hdr, u8 * buff, u32 len)
{
	u32 i = 0;
	u32 rec = 0;

	if (NULL == buff) {
		printk("buffer is null\n");
		return;
	}

	printk("%s\n", hdr);
	for (i = 0; i < len;) {
		if (4 < (len - i)) {
			printk("%04x 0x%02x, 0x%02x, 0x%02x, 0x%02x \n",
			       i, buff[i], buff[i + 1], buff[i + 2],
			       buff[i + 3]);
			i += 4;
			continue;
		}

		if (0x00 == rec) {
			printk("%04x", i);
			rec++;
		}

		printk(" 0x%02x ", buff[i]);
		i++;
	}

	printk("\n");
}

s32 sat_alloc_ncq_tag(struct sata_device *sata_dev, u8 * tag)
{
	u32 i = 0;
	unsigned long flag = 0;
	s32 ret = ERROR;

	spin_lock_irqsave(&sata_dev->sata_lock, flag);
	for (i = 0; i <= MAX_NCQ_TAG; i++) {
		if (0 == (((sata_dev->ncq_tag_bitmap) >> i) & 1)) {
			*tag = (u8) i;
			sata_dev->ncq_tag_bitmap |= (1 << i);

			ret = OK;
			break;
		}
	}
	spin_unlock_irqrestore(&sata_dev->sata_lock, flag);

	return ret;
}

void sat_free_ncq_tag(struct sata_device *sata_dev, u8 tag)
{
	unsigned long flag = 0;

	SAT_ASSERT(0 != (sata_dev->ncq_tag_bitmap & (1 << tag)));

	spin_lock_irqsave(&sata_dev->sata_lock, flag);
	sata_dev->ncq_tag_bitmap &= (~((u32) 1 << tag));
	spin_unlock_irqrestore(&sata_dev->sata_lock, flag);

	SAT_TRACE_FUNC_EXIT;
	return;
}

void sat_fill_identify_fis(struct sata_h2d *h2d_fis)
{
	memset(h2d_fis, 0, sizeof(struct sata_h2d));

	h2d_fis->header.fis_type = SATA_FIS_TYPE_H2D;
	h2d_fis->header.command = FIS_COMMAND_IDENTIFY_DEVICE;
	h2d_fis->header.pm_port = 0x80;	/*c bit set to 1,PM port set to 0 */
	/*all other fields are set to 0 */
}


/**
 * sat_send_identify - send identify device command to SATA device
 * in order to get identify information.
 * @req: command related stp request information
 *
 */
s32 sat_send_identify(struct stp_req *req)
{
	sat_fill_identify_fis(&req->h2d_fis);
	return sat_send_req_to_dev(req);
}


/**
 * sat_sata_err_to_sense - change sata err to scsi sense information
 * 
 * @req: command related stp request information
 * @ata_status: ata status information
 * @ata_err: ata err information
 * @sense_key: sense key
 * @sense_code: sense code
 * @direction: io direction
 *
 */
void sat_sata_err_to_sense(struct stp_req *req,
			   u8 ata_status,
			   u8 ata_err,
			   u8 * sense_key, u16 * sense_code, u32 direction)
{
	/* Dignostic request related */
	if (IS_DIGNOSTIC_REQ(req->org_msg->h2d_fis.header.command)) {
		*sense_key = SCSI_SNSKEY_HARDWARE_ERROR;
		*sense_code = SCSI_SNSCODE_LOGICAL_UNIT_FAILED_SELF_TEST;
	}
	/*
	 * Check for device fault case 
	 */
	if (ata_status & ATA_STATUS_MASK_DF) {
		*sense_key = SCSI_SNSKEY_HARDWARE_ERROR;
		*sense_code = SCSI_SNSCODE_INTERNAL_TARGET_FAILURE;
		return;
	}

	/*
	 * If status error bit it set, need to check the error register 
	 */
	if (ata_status & ATA_STATUS_MASK_ERR) {
		if (ata_err & ATA_ERROR_MASK_NM) {
			*sense_key = SCSI_SNSKEY_NOT_READY;
			*sense_code = SCSI_SNSCODE_MEDIUM_NOT_PRESENT;
		} else if (ata_err & ATA_ERROR_MASK_UNC) {
			/* If it's write command,we need to set sense code to WP */
			if (SCSI_IO_DMA_TO_DEV(direction)) {
				*sense_key = SCSI_SNSKEY_DATA_PROTECT;
				*sense_code = SCSI_SNSCODE_WRITE_PROTECTED;
			} else {
				*sense_key = SCSI_SNSKEY_MEDIUM_ERROR;
				*sense_code =
				    SCSI_SNSCODE_UNRECOVERED_READ_ERROR;
			}
		} else if (ata_err & ATA_ERROR_MASK_IDNF) {
			*sense_key = SCSI_SNSKEY_MEDIUM_ERROR;
			*sense_code = SCSI_SNSCODE_RECORD_NOT_FOUND;
		} else if (ata_err & ATA_ERROR_MASK_MC) {
			*sense_key = SCSI_SNSKEY_UNIT_ATTENTION;
			*sense_code = SCSI_SNSCODE_NOT_READY_TO_READY_CHANGE;
		} else if (ata_err & ATA_ERROR_MASK_MCR) {
			*sense_key = SCSI_SNSKEY_UNIT_ATTENTION;
			*sense_code =
			    SCSI_SNSCODE_OPERATOR_MEDIUM_REMOVAL_REQUEST;
		} else if (ata_err & ATA_ERROR_MASK_ICRC) {
			*sense_key = SCSI_SNSKEY_ABORTED_COMMAND;
			*sense_code = SCSI_SNSCODE_INFORMATION_UNIT_CRC_ERROR;
		} else if (ata_err & ATA_ERROR_MASK_ABRT) {
			*sense_key = SCSI_SNSKEY_ABORTED_COMMAND;
			*sense_code = SCSI_SNSCODE_NO_ADDITIONAL_INFO;
		} else {
			*sense_key = SCSI_SNSKEY_HARDWARE_ERROR;
			*sense_code = SCSI_SNSCODE_INTERNAL_TARGET_FAILURE;
		}
		return;
	} else {
		/* if no err, then no sense */
		*sense_key = SCSI_SNSKEY_NO_SENSE;
		*sense_code = SCSI_SNSCODE_NO_ADDITIONAL_INFO;
		return;
	}
}

void sat_dec_count_and_free_ncq_tag(struct sata_device *sata_dev,
				    struct stp_req *stp_request)
{
	unsigned long dev_flag = 0;

	spin_lock_irqsave(&sata_dev->sata_lock, dev_flag);
	sata_dev->pending_io--;
	if (SAT_IS_NCQ_REQ(stp_request->h2d_fis.header.command)) {
		sata_dev->pending_ncq_io--;
	} else {
		sata_dev->pending_non_ncq_io--;
	}
	spin_unlock_irqrestore(&sata_dev->sata_lock, dev_flag);

	if (SAT_IS_NCQ_REQ(stp_request->h2d_fis.header.command)) {
		sat_free_ncq_tag(sata_dev, stp_request->ncq_tag);
	}

	return;
}

void sat_disk_exec_complete(struct stp_req *stp_request, u32 io_status)
{
	struct sata_device *tmp_sata_dev = NULL;
	u8 sns_key = 0;
	u16 sns_code = 0;
	u8 *cdb = NULL;
	struct sal_msg *sal_msg = NULL;
	struct stp_req *org_stp_req = NULL;
	char *desc_str[] = {
		"successful",
		"failed",
		"busy",
		"complete",
		"queue full"
	};

	SAL_REF(desc_str[0]);	
	SAT_ASSERT(NULL != stp_request, return);

	sal_msg = stp_request->upper_msg;
	SAT_ASSERT(NULL != sal_msg, return);
	tmp_sata_dev = stp_request->dev;

	/* CDB */
	cdb = sal_msg->ini_cmnd.cdb;

	/*Decrease IO count and free NCQ tag(if it is NCQ command)first */
	sat_dec_count_and_free_ncq_tag(tmp_sata_dev, stp_request);

	/*delete this IO from list */
	SAS_LIST_DEL_INIT(&stp_request->io_list);

	if ((SAT_IO_SUCCESS != io_status)
	    && (!SAT_IS_ATA_PASSTHROUGH_CMND(cdb))) {
		SAT_TRACE(OSP_DBL_MAJOR, 4, "Device:0x%llx,IO status:%u(%s)",
			  tmp_sata_dev->dev_id, io_status, desc_str[io_status]);

		/*if org_msg not equal to current message,which means it is a internal IO */
		if (stp_request->org_msg != stp_request) {
			SAT_TRACE(OSP_DBL_INFO, 1,
				  "Org stp req:%p(upper msg:%p) ,Stp req:%p(upper msg:%p)",
				  stp_request->org_msg,
				  stp_request->org_msg->upper_msg, stp_request,
				  sal_msg);
			org_stp_req = stp_request->org_msg;

			sat_free_int_msg(stp_request);
			/* IO failed,Notify upper layer this information */
			sat_io_completed(org_stp_req, SAT_IO_FAILED);
			return;
		} else if (stp_request->scsi_cmnd.done != NULL) {
			/*Direct message,may be here we need to do some command specific process */
			sat_io_completed(stp_request, SAT_IO_FAILED);
			return;
		} else {
			/*Otherwise it is internal command like READ LOE EXT,use command specific
			   *error handle policy to handle this abnormal */
			SAT_TRACE(OSP_DBL_MAJOR, 1,
				  "msg:%p internal command without scsi cmd,handle it by its own policy",
				  sal_msg);
		}
	}

	/*OK,IO success,we need to judge whether we need to do necessary ATA error
	 *to SCSI error translation*/
	if ((SAT_D2H_VALID == stp_request->d2h_valid)
	    && SAT_IS_IO_FAILED(stp_request->d2h_fis.header.status)
	    && (!SAT_IS_ATA_PASSTHROUGH_CMND(cdb))) {
		SAT_TRACE(OSP_DBL_INFO, 4,
			  "Device:0x%llx IO failed,status:0x%x,error:0x%x",
			  tmp_sata_dev->dev_id,
			  stp_request->d2h_fis.header.status,
			  stp_request->d2h_fis.header.err);

		sat_sata_err_to_sense(stp_request,
				      stp_request->d2h_fis.header.status,
				      stp_request->d2h_fis.header.err,
				      &sns_key,
				      &sns_code,
				      ((NULL ==
					stp_request->org_msg->scsi_cmnd.
					done) ? PCI_DMA_NONE : stp_request->
				       org_msg->scsi_cmnd.data_dir)
		    );

		if (stp_request->org_msg != stp_request) {
			SAT_TRACE(OSP_DBL_MAJOR, 1,
				  "Current command failed(Org Stp Req:%p ,Stp Req:%p)",
				  stp_request->org_msg, stp_request);
			sat_set_sense_and_result(stp_request->org_msg, sns_key,
						 sns_code);
			org_stp_req = stp_request->org_msg;

			sat_free_int_msg(stp_request);

			sat_io_completed(org_stp_req, SAT_IO_SUCCESS);
			return;
		} else if (stp_request->scsi_cmnd.done != NULL) {
			SAT_TRACE(OSP_DBL_MAJOR, 1,
				  "org msg:%p(Org Stp Req:%p),msg:%p(Stp Req:%p),sence key:0x%x,sence code:0x%x",
				  stp_request->org_msg->upper_msg,
				  stp_request->org_msg, stp_request->upper_msg,
				  stp_request, sns_key, sns_code);
			sat_set_sense_and_result(stp_request->org_msg, sns_key,
						 sns_code);

			sat_io_completed(stp_request, SAT_IO_SUCCESS);
			return;
		}
		/*Otherwise it is internal command like READ LOE EXT,use command specific
		 *error handle policy to handle this abnormal*/

		/*OK,If it is NCQ command,we need to do error recovery(issue read log ext) */
		//1 TODO:NCQ ERROR HANDLING
	}

	/*Now use command specific Call Back */
	stp_request->compl_call_back(stp_request, io_status);
}

void sat_identify_cb(struct stp_req *stp_request, u32 io_status)
{
	struct stp_req *org_req = NULL;
	struct sata_device *dev = NULL;
	s32 ret = 0;

	org_req = stp_request->org_msg;
	dev = org_req->dev;

	/*If we get here,which means IO succeed,need to copy identify information to
	 *device struct*/
	sat_set_dev_info(dev,
			 (struct sata_identify_data *)
			 sat_get_req_buf(stp_request));
	sat_free_int_msg(stp_request);
	/*
	 *now identify information is valid,we can go to the next step
	 */
	if (NULL != sat_translation_table[org_req->scsi_cmnd.cdb[0]]) {
		ret =
		    sat_translation_table[org_req->scsi_cmnd.cdb[0]] (org_req,
								      &org_req->
								      scsi_cmnd,
								      dev);
		if (SAT_IO_SUCCESS == ret)
			return;

		sat_io_completed(org_req, ret);
		return;
	} else {
		/*
		 * Command not supported yet,return illegal request with ASC/ASCQ set to
		 * INVALID COMMAND OPERATION CODE to SCSI mid layer 
		 */
		sat_set_sense_and_result(org_req,
					 SCSI_SNSKEY_ILLEGAL_REQUEST,
					 SCSI_SNSCODE_INVALID_COMMAND);
		SAT_TRACE(OSP_DBL_INFO, 4,
			  "Device:0x%llx,get unsupported commnad:0x%x",
			  dev->dev_id, org_req->scsi_cmnd.cdb[0]);
		sat_io_completed(org_req, SAT_IO_SUCCESS);
		SAT_TRACE_FUNC_EXIT;
	}
	return;
}


/**
 * sat_inquiry_page_89 - fill in INQUIRY page 89(ATA information VPD page).
 * 
 * @sata_identify: sata identify info
 * @inquiry: inquiry info
 *
 */
void sat_inquiry_page_89(struct sata_identify_data *sata_identify, u8 * inquiry)
{
	/*
	 *  SAT revision 8, 10.3.5, p 83
	 */
	u8 *simple_data = (u8 *) sata_identify;
	/*
	 * When translating the fields, in some cases using the simple form of SATA 
	 * Identify Device data is easier. So we define it here.
	 * Both pstSimpleData and pSATAIdData points to the same data.
	 */
	inquiry[0] = 0x00;
	inquiry[1] = 0x89;	/* page code */

	/* Page length 0x238 */
	inquiry[2] = 0x02;
	inquiry[3] = 0x38;

	inquiry[4] = 0x0;	/* reserved */
	inquiry[5] = 0x0;	/* reserved */
	inquiry[6] = 0x0;	/* reserved */
	inquiry[7] = 0x0;	/* reserved */

	/* SAT Vendor Identification */
	inquiry[8] = 'H';	/* 8 bytes   */
	inquiry[9] = 'U';
	inquiry[10] = 'A';
	inquiry[11] = '_';
	inquiry[12] = 'W';
	inquiry[13] = 'E';
	inquiry[14] = 'I';
	inquiry[15] = ' ';

	/* SAT Product Idetification */
	inquiry[16] = 'O';	/* 16 bytes   */
	inquiry[17] = 'C';
	inquiry[18] = 'E';
	inquiry[19] = 'A';
	inquiry[20] = 'N';
	inquiry[21] = ' ';
	inquiry[22] = 'S';
	inquiry[23] = 'T';
	inquiry[24] = 'O';
	inquiry[25] = 'R';
	inquiry[26] = 'E';
	inquiry[27] = ' ';
	inquiry[28] = ' ';
	inquiry[29] = ' ';
	inquiry[30] = ' ';
	inquiry[31] = ' ';

	/* SAT Product Revision Level */
	inquiry[32] = '0';	/* 4 bytes   */
	inquiry[33] = '1';
	inquiry[34] = ' ';
	inquiry[35] = ' ';

	/* Signature, SAT revision8, Table88, p85 */

	inquiry[36] = 0x34;	/* SATA type */
	inquiry[37] = 0;	/*Set to 0 */
	inquiry[38] = 0;
	inquiry[39] = 0;

	inquiry[40] = 0x00;	/* LBA Low          */
	inquiry[41] = 0x00;	/* LBA Mid          */
	inquiry[42] = 0x00;	/* LBA High         */
	inquiry[43] = 0x00;	/* Device           */
	inquiry[44] = 0x00;	/* LBA Low Exp      */
	inquiry[45] = 0x00;	/* LBA Mid Exp      */
	inquiry[46] = 0x00;	/* LBA High Exp     */
	inquiry[47] = 0x00;	/* Reserved         */
	inquiry[48] = 0x00;	/* Sector Count     */
	inquiry[49] = 0x00;	/* Sector Count Exp */
	/* Reserved */
	inquiry[50] = 0x00;
	inquiry[51] = 0x00;
	inquiry[52] = 0x00;
	inquiry[53] = 0x00;
	inquiry[54] = 0x00;
	inquiry[55] = 0x00;
	inquiry[56] = 0xEC;	/* always ATA device,use IDENTIFY DEVICE */
	/* Reserved */
	inquiry[57] = 0x0;
	inquiry[58] = 0x0;
	inquiry[59] = 0x0;
	/* Identify Device */
	memcpy(&inquiry[60], simple_data, ATA_IDENTIFY_SIZE);
	return;
}


/**
 * sat_inquiry_page_83 - fill in INQUIRY page 83(device identification VPD page).
 * 
 * @sata_identify: sata identify info
 * @inquiry: inquiry info
 *
 */
void sat_inquiry_page_83(struct sata_identify_data *sata_identify, u8 * inquiry)
{

	/*
	   See SPC-4, 7.6.3, p 323
	   and SAT revision 8, 10.3.4, p 78
	 */

	/*
	   to do:
	   note: Depending on who we are and where we are located, addition ID descriptor
	   needs to be added. eg) If we have STP initiator port, we have ID descriptor like
	   SAT revision 8, Table 86, p82
	   As of 2/27/06, this implimentation assumes the simplest - no STP initiator port
	 */
	u16 *tmp_identify = (u16 *) (void *)sata_identify;
	/*
	   * When translating the fields, in some cases using the simple form of SATA 
	   * Identify Device data is easier. So we define it here.
	   * Both tmp_identify and pSATAIdData points to the same data.
	 */
	inquiry[0] = 0x00;
	inquiry[1] = 0x83;	/* page code */
	inquiry[2] = 0;		/* Reserved */

	/*
	   * If the ATA device returns word 87 bit 8 set to one in its IDENTIFY DEVICE
	   * data indicating that it supports the WORLD WIDE NAME field 
	   * (i.e., words 108-111), the SATL shall include an identification descriptor
	   * containing a logical unit name.
	 */
	if (SAT_IS_WWN_SUPPORTD(sata_identify)) {
		/* Fill in SAT Rev8 Table85 */
		/*
		 * Logical unit name derived from the world wide name.
		 */
		inquiry[3] = 12;	/* 15-3; page length, no addition ID descriptor assumed */

		/*
		 * Identifier descriptor 
		 */
		inquiry[4] = 0x01;	/* Code set: binary codes */
		inquiry[5] = 0x03;	/* Identifier type : NAA  */
		inquiry[6] = 0x00;	/* Reserved               */
		inquiry[7] = 0x08;	/* Identifier length      */

		/* Bit 4-7 NAA field, bit 0-3 MSB of IEEE Company ID */
		inquiry[8] = (u8) ((sata_identify->naming_authority) >> 8);
		/* IEEE Company ID */
		inquiry[9] = (u8) ((sata_identify->naming_authority) & 0xFF);
		/* IEEE Company ID */
		inquiry[10] = (u8) ((sata_identify->naming_authority1) >> 8);
		/* Bit 4-7 LSB of IEEE Company ID, bit 0-3 MSB of Vendor Specific ID */
		inquiry[11] = (u8) ((sata_identify->naming_authority1) & 0xFF);
		/* Vendor Specific ID  */
		inquiry[12] = (u8) ((sata_identify->unique_id_bit_16_31) >> 8);
		/* Vendor Specific ID  */
		inquiry[13] =
		    (u8) ((sata_identify->unique_id_bit_16_31) & 0xFF);
		/* Vendor Specific ID  */
		inquiry[14] = (u8) ((sata_identify->unique_id_bit_0_15) >> 8);
		/* Vendor Specific ID  */
		inquiry[15] = (u8) ((sata_identify->unique_id_bit_0_15) & 0xFF);

	} else {
		/* Fill in SAT Rev8 Table86 */
		/*
		 * Logical unit name derived from the model number and serial number.
		 */
		inquiry[3] = 72;	/* 75 - 3; page length */

		/*
		 * Identifier descriptor 
		 */
		inquiry[4] = 0x02;	/* Code set: ASCII codes */
		inquiry[5] = 0x01;	/* Identifier type : T10 vendor ID based */
		inquiry[6] = 0x00;	/* Reserved */
		inquiry[7] = 0x44;	/* 0x44, 68 Identifier length */

		/* Byte 8 to 15 is the vendor id string 'ATA     '. */
		inquiry[8] = 'A';
		inquiry[9] = 'T';
		inquiry[10] = 'A';
		inquiry[11] = ' ';
		inquiry[12] = ' ';
		inquiry[13] = ' ';
		inquiry[14] = ' ';
		inquiry[15] = ' ';
		/*
		 * Byte 16 to 75 is vendor specific id 
		 */
	/**< word 27 to 46 of identify device information, 40 ASCII chars */
		inquiry[16] = (u8) ((tmp_identify[27]) >> 8);
		inquiry[17] = (u8) ((tmp_identify[27]) & 0x00ff);
		inquiry[18] = (u8) ((tmp_identify[28]) >> 8);
		inquiry[19] = (u8) ((tmp_identify[28]) & 0x00ff);
		inquiry[20] = (u8) ((tmp_identify[29]) >> 8);
		inquiry[21] = (u8) ((tmp_identify[29]) & 0x00ff);
		inquiry[22] = (u8) ((tmp_identify[30]) >> 8);
		inquiry[23] = (u8) ((tmp_identify[30]) & 0x00ff);
		inquiry[24] = (u8) ((tmp_identify[31]) >> 8);
		inquiry[25] = (u8) ((tmp_identify[31]) & 0x00ff);
		inquiry[26] = (u8) ((tmp_identify[32]) >> 8);
		inquiry[27] = (u8) ((tmp_identify[32]) & 0x00ff);
		inquiry[28] = (u8) ((tmp_identify[33]) >> 8);
		inquiry[29] = (u8) ((tmp_identify[33]) & 0x00ff);
		inquiry[30] = (u8) ((tmp_identify[34]) >> 8);
		inquiry[31] = (u8) ((tmp_identify[34]) & 0x00ff);
		inquiry[32] = (u8) ((tmp_identify[35]) >> 8);
		inquiry[33] = (u8) ((tmp_identify[35]) & 0x00ff);
		inquiry[34] = (u8) ((tmp_identify[36]) >> 8);
		inquiry[35] = (u8) ((tmp_identify[36]) & 0x00ff);
		inquiry[36] = (u8) ((tmp_identify[37]) >> 8);
		inquiry[37] = (u8) ((tmp_identify[37]) & 0x00ff);
		inquiry[38] = (u8) ((tmp_identify[38]) >> 8);
		inquiry[39] = (u8) ((tmp_identify[38]) & 0x00ff);
		inquiry[40] = (u8) ((tmp_identify[39]) >> 8);
		inquiry[41] = (u8) ((tmp_identify[39]) & 0x00ff);
		inquiry[42] = (u8) ((tmp_identify[40]) >> 8);
		inquiry[43] = (u8) ((tmp_identify[40]) & 0x00ff);
		inquiry[44] = (u8) ((tmp_identify[41]) >> 8);
		inquiry[45] = (u8) ((tmp_identify[41]) & 0x00ff);
		inquiry[46] = (u8) ((tmp_identify[42]) >> 8);
		inquiry[47] = (u8) ((tmp_identify[42]) & 0x00ff);
		inquiry[48] = (u8) ((tmp_identify[43]) >> 8);
		inquiry[49] = (u8) ((tmp_identify[43]) & 0x00ff);
		inquiry[50] = (u8) ((tmp_identify[44]) >> 8);
		inquiry[51] = (u8) ((tmp_identify[44]) & 0x00ff);
		inquiry[52] = (u8) ((tmp_identify[45]) >> 8);
		inquiry[53] = (u8) ((tmp_identify[45]) & 0x00ff);
		inquiry[54] = (u8) ((tmp_identify[46]) >> 8);
		inquiry[55] = (u8) ((tmp_identify[46]) & 0x00ff);

	/**< word 10 to 19 of identify device information, 20 ASCII chars */
		inquiry[56] = (u8) ((tmp_identify[10]) >> 8);
		inquiry[57] = (u8) ((tmp_identify[10]) & 0x00ff);
		inquiry[58] = (u8) ((tmp_identify[11]) >> 8);
		inquiry[59] = (u8) ((tmp_identify[11]) & 0x00ff);
		inquiry[60] = (u8) ((tmp_identify[12]) >> 8);
		inquiry[61] = (u8) ((tmp_identify[12]) & 0x00ff);
		inquiry[62] = (u8) ((tmp_identify[13]) >> 8);
		inquiry[63] = (u8) ((tmp_identify[13]) & 0x00ff);
		inquiry[64] = (u8) ((tmp_identify[14]) >> 8);
		inquiry[65] = (u8) ((tmp_identify[14]) & 0x00ff);
		inquiry[66] = (u8) ((tmp_identify[15]) >> 8);
		inquiry[67] = (u8) ((tmp_identify[15]) & 0x00ff);
		inquiry[68] = (u8) ((tmp_identify[16]) >> 8);
		inquiry[69] = (u8) ((tmp_identify[16]) & 0x00ff);
		inquiry[70] = (u8) ((tmp_identify[17]) >> 8);
		inquiry[71] = (u8) ((tmp_identify[17]) & 0x00ff);
		inquiry[72] = (u8) ((tmp_identify[18]) >> 8);
		inquiry[73] = (u8) ((tmp_identify[18]) & 0x00ff);
		inquiry[74] = (u8) ((tmp_identify[19]) >> 8);
		inquiry[75] = (u8) ((tmp_identify[19]) & 0x00ff);
	}

}

/**
 * sat_inquiry_standard - fill in INQUIRY standard page.
 * 
 * @sata_identify: sata identify info
 * @inquiry: inquiry info
 *
 */
void sat_inquiry_standard(struct sata_identify_data *sata_identify,
			  u8 * inquiry)
{
	/*
	 *  Assumption: Basic Task Mangement is supported
	 *  -> BQUE 1 and CMDQUE 0, SPC-4, Table96, p147
	 *
	 *
	 *  See SPC-4, 6.4.2, p 143
	 *  and SAT revision 8, 8.1.2, p 28
	 */

	/*
	 * Reject all other LUN other than LUN 0.
	 */
	inquiry[0] = 0x00;
	inquiry[1] = 0x00;
	inquiry[2] = 0x05;	/* SPC-3 */
	inquiry[3] = 0x2;	/* resp data format set to 2 */
	inquiry[4] = 0x1F;	/* 35 - 4 = 31; Additional length */
	inquiry[5] = 0x00;
	/* The following two are for task management. SAT Rev8, p20 */
	if (sata_identify->sata_cap & 0x100) {
		/* NCQ supported; multiple outstanding SCSI IO are supported */
		inquiry[6] = 0x00;	/* BQUE bit is not set */
		inquiry[7] = 0x02;	/* CMDQUE bit is set */
	} else {
		inquiry[6] = 0x80;	/* BQUE bit is set */
		inquiry[7] = 0x00;	/* CMDQUE bit is not set */
	}
	/*
	   * Vendor ID.
	 */
	inquiry[8] = 'A';	/* 8 bytes */
	inquiry[9] = 'T';
	inquiry[10] = 'A';
	inquiry[11] = ' ';
	inquiry[12] = ' ';
	inquiry[13] = ' ';
	inquiry[14] = ' ';
	inquiry[15] = ' ';

	/*
	   * Product ID
	 */
	/* when flipped by LL */
	inquiry[16] = sata_identify->model_num[1];
	inquiry[17] = sata_identify->model_num[0];
	inquiry[18] = sata_identify->model_num[3];
	inquiry[19] = sata_identify->model_num[2];
	inquiry[20] = sata_identify->model_num[5];
	inquiry[21] = sata_identify->model_num[4];
	inquiry[22] = sata_identify->model_num[7];
	inquiry[23] = sata_identify->model_num[6];
	inquiry[24] = sata_identify->model_num[9];
	inquiry[25] = sata_identify->model_num[8];
	inquiry[26] = sata_identify->model_num[11];
	inquiry[27] = sata_identify->model_num[10];
	inquiry[28] = sata_identify->model_num[13];
	inquiry[29] = sata_identify->model_num[12];
	inquiry[30] = sata_identify->model_num[15];
	inquiry[31] = sata_identify->model_num[14];

	/* when flipped */
	/*
	   * Product Revision level.
	 */

	/*
	 * If the IDENTIFY DEVICE data received in words 25 and 26 from the ATA
	 * device are four ASCII spaces (20h), do this translation.
	 */
	if ((sata_identify->firmware_ver[4] == 0x20) &&
	    (sata_identify->firmware_ver[5] == 0x20) &&
	    (sata_identify->firmware_ver[6] == 0x20) &&
	    (sata_identify->firmware_ver[7] == 0x20)) {
		/*firmware version */
		inquiry[32] = sata_identify->firmware_ver[1];
		inquiry[33] = sata_identify->firmware_ver[0];
		inquiry[34] = sata_identify->firmware_ver[3];
		inquiry[35] = sata_identify->firmware_ver[2];
	} else {
		/*firmware version */
		inquiry[32] = sata_identify->firmware_ver[5];
		inquiry[33] = sata_identify->firmware_ver[4];
		inquiry[34] = sata_identify->firmware_ver[7];
		inquiry[35] = sata_identify->firmware_ver[6];
	}
	return;

}


/**
 * sat_inquiry_cb - Identify information is valid,inquiry command callback process.
 * 
 * @stp_request: sata request info
 * @io_status: inquiry info
 *
 */
void sat_inquiry_cb(struct stp_req *stp_request, u32 io_status)
{
	u8 *cdb = NULL;
	struct sata_device *dev = NULL;
	struct stp_req *tmp_org_req = NULL;
	u8 *inq_buff = NULL;
	u32 copy_len = 0;
	u32 alloc_len = 0;
	u32 fill_len = 0;

	dev = stp_request->dev;
	tmp_org_req = stp_request->org_msg;
	cdb = tmp_org_req->scsi_cmnd.cdb;

	/* 
	 * If we get here,which means IO succeed,need to copy identify information to
	 * device struct
	 */
	sat_set_dev_info(dev,
			 (struct sata_identify_data *)
			 sat_get_req_buf(stp_request));

	/*free struct sal_msg resource */
	sat_free_int_msg(stp_request);

	alloc_len = ((cdb[3] << 8) | cdb[4]);	/*allocation length,byte 3 and byte 4 */
	copy_len = alloc_len;

	if (tmp_org_req->scsi_cmnd.data_len < copy_len)
		copy_len = tmp_org_req->scsi_cmnd.data_len;

	/*temp buffer used for inquiry */
	inq_buff = SAS_KMALLOC(SCSI_MAX_INQUIRY_BUFFER_SIZE, GFP_ATOMIC);
	if (NULL == inq_buff) {
		SAT_TRACE(OSP_DBL_MINOR, 5,
			  "Alloc resource for device 0x%llx failed",
			  dev->dev_id);

		sat_io_completed(tmp_org_req, SAT_IO_FAILED);
		return;
	}

	memset(inq_buff, 0, SCSI_MAX_INQUIRY_BUFFER_SIZE);

	/*
	 * if EVPD bit is not set,the page code must be set to 0,in this situation we
	 * execute inquiry stand.If page code is not set to 0,return fail to upper 
	 * layer
	 */
	if (!(cdb[1] & SCSI_EVPD_MASK)) {
		if (cdb[2]) {
			SAT_TRACE(OSP_DBL_INFO, 5,
				  "Alloc resource for device 0x%llx failed",
				  dev->dev_id);

			sat_set_sense_and_result(tmp_org_req,
						 SCSI_SNSKEY_ILLEGAL_REQUEST,
						 SCSI_SNSCODE_INVALID_FIELD_IN_CDB);
			sat_io_completed(tmp_org_req, SAT_IO_SUCCESS);
			SAS_KFREE(inq_buff);
			return;
		}
		sat_inquiry_standard(&dev->sata_identify, inq_buff);
		fill_len = inq_buff[4] + 5;
	} else {
		/* EVPD page */
		switch (cdb[2]) {
		case SCSI_INQUIRY_SUPPORTED_VPD_PAGE:
			inq_buff[0] = 0x00;
			inq_buff[1] = 0x00;	/* page code */
			inq_buff[2] = 0x00;	/* reserved */
			inq_buff[3] = 4;	/* last index(in this case, 6) - 3; page length */

			/* supported vpd page list */
			inq_buff[4] = 0x00;	/* page 0x00 supported */
			inq_buff[5] = 0x80;	/* page 0x80 supported */
			inq_buff[6] = 0x83;	/* page 0x83 supported */
			inq_buff[7] = 0x89;	/* page 0x89 supported */
			fill_len = inq_buff[3] + 4;	/*total page length */
			break;
		case SCSI_INQUIRY_UNIT_SERIAL_NUMBER_VPD_PAGE:
			inq_buff[0] = 0x00;	/*LUN id is always set to 0 */
			inq_buff[1] = 0x80;	/* page code */
			inq_buff[2] = 0x00;	/* reserved */
			inq_buff[3] = 0x14;	/* page length */
			inq_buff[4] = dev->sata_identify.serial_num[1];
			inq_buff[5] = dev->sata_identify.serial_num[0];
			inq_buff[6] = dev->sata_identify.serial_num[3];
			inq_buff[7] = dev->sata_identify.serial_num[2];
			inq_buff[8] = dev->sata_identify.serial_num[5];
			inq_buff[9] = dev->sata_identify.serial_num[4];
			inq_buff[10] = dev->sata_identify.serial_num[7];
			inq_buff[11] = dev->sata_identify.serial_num[6];
			inq_buff[12] = dev->sata_identify.serial_num[9];
			inq_buff[13] = dev->sata_identify.serial_num[8];
			inq_buff[14] = dev->sata_identify.serial_num[11];
			inq_buff[15] = dev->sata_identify.serial_num[10];
			inq_buff[16] = dev->sata_identify.serial_num[13];
			inq_buff[17] = dev->sata_identify.serial_num[12];
			inq_buff[18] = dev->sata_identify.serial_num[15];
			inq_buff[19] = dev->sata_identify.serial_num[14];
			inq_buff[20] = dev->sata_identify.serial_num[17];
			inq_buff[21] = dev->sata_identify.serial_num[16];
			inq_buff[22] = dev->sata_identify.serial_num[19];
			inq_buff[23] = dev->sata_identify.serial_num[18];
			fill_len = inq_buff[3] + 4;	/* total page length */
			break;
		case SCSI_INQUIRY_DEVICE_IDENTIFICATION_VPD_PAGE:
			sat_inquiry_page_83(&dev->sata_identify, inq_buff);
			fill_len = inq_buff[3] + 4;	/* total page length */
			break;
		case SCSI_INQUIRY_ATA_INFORMATION_VPD_PAGE:
			sat_inquiry_page_89(&dev->sata_identify, inq_buff);
			fill_len = ((u32) inq_buff[2] << 8) + inq_buff[3] + 4;	/* total page length */
			break;
		default:
			SAT_TRACE(OSP_DBL_MINOR, 5,
				  "Device 0x%llx get unsupportted page %d",
				  dev->dev_id, cdb[2]);

			sat_set_sense_and_result(tmp_org_req,
						 SCSI_SNSKEY_ILLEGAL_REQUEST,
						 SCSI_SNSCODE_INVALID_FIELD_IN_CDB);
			sat_io_completed(tmp_org_req, SAT_IO_SUCCESS);
			SAS_KFREE(inq_buff);
			return;
		}
	}

	if (fill_len < copy_len)
		copy_len = fill_len;

	if (copy_len !=
	    (u32) sal_sg_copy_from_buffer((struct sal_msg *)tmp_org_req->
					  upper_msg, inq_buff, copy_len)) {
		SAT_TRACE(OSP_DBL_MINOR, 5,
			  "Device:0x%llx copy form buffer failed", dev->dev_id);
		sat_io_completed(tmp_org_req, SAT_IO_FAILED);
		SAS_KFREE(inq_buff);
		return;
	}

	if (tmp_org_req->scsi_cmnd.data_len != copy_len) {
		SAT_TRACE(OSP_DBL_MINOR, 5,
			  "INI Cmnd data len:%d not equal to %d",
			  tmp_org_req->scsi_cmnd.data_len, copy_len);
	}

	sat_io_completed(tmp_org_req, SAT_IO_SUCCESS);
	SAS_KFREE(inq_buff);
	return;
}

/**
 * sat_inquiry - translate inquiry command from SCSI to ATA,
 * and Send to Device.
 * 
 * @req: sata request info
 * @scsi_cmd: sata request related scsi cmd
 * @sata_dev: sata device related
 *
 */
s32 sat_inquiry(struct stp_req * req,
		struct sat_ini_cmnd * scsi_cmd, struct sata_device * sata_dev)
{
	s32 ret = 0;
	u8 *cdb = NULL;
	struct stp_req *new_req = NULL;
	/*Need to send identify first */

	cdb = SAT_GET_SCSI_COMMAND(scsi_cmd);

	if (!(cdb[1] & SCSI_EVPD_MASK) && (0 != cdb[2])) {
		SAT_TRACE(OSP_DBL_MINOR, 5, "Disk:0x%llx invalid feild in CDB",
			  req->dev->dev_id);
		sat_set_sense_and_result(req,
					 SCSI_SNSKEY_ILLEGAL_REQUEST,
					 SCSI_SNSCODE_INVALID_FIELD_IN_CDB);
		return SAT_IO_COMPLETE;
	}

	new_req = sat_get_new_msg(sata_dev, ATA_IDENTIFY_SIZE);
	if (NULL == new_req) {
		SAT_TRACE(OSP_DBL_MINOR, 5,
			  "Alloc resource for device 0x%llx failed"
			  "command:0x%x", sata_dev->dev_id, cdb[0]);
		return SAT_IO_FAILED;
	}

	/*OK,send identify */
	sat_init_internal_command(new_req, req, sata_dev);
	new_req->compl_call_back = sat_inquiry_cb;

	ret = sat_send_identify(new_req);
	if (SAT_IO_SUCCESS != ret) {
		sat_free_int_msg(new_req);
		return ret;
	}

	return SAT_IO_SUCCESS;
}

/**
 * sat_test_unit_ready_cb - check power mode fis call back function, 
 * for scsi command TEST UNIT READY.
 * 
 * @stp_request: sata request info
 * @io_status: io status
 *
 */
void sat_test_unit_ready_cb(struct stp_req *stp_request, u32 io_status)
{
	/*SATA device should allways return a D2H fis to initiator */
	if (SAT_D2H_VALID == stp_request->d2h_valid) {
		/*secotr count 00h:standby state
		 *secotr count 40h:Active state,spun/spinning down
		 *secotr count 41h:Active state,spun/spinning up
		 *secotr count 80h:idle state
		 *secotr count FFh:Active state or idle state
		 */
		if (0 == stp_request->d2h_fis.data.sector_cnt) {	/*standby state */
			sat_set_sense_and_result(stp_request,
						 SCSI_SNSKEY_NOT_READY,
						 SCSI_SNSCODE_LOGICAL_UNIT_NOT_READY_INITIALIZING_COMMAND_REQUIRED);
			sat_io_completed(stp_request, SAT_IO_SUCCESS);
			return;
		}
		SAT_TRACE(OSP_DBL_DATA, 5, "Disk 0x%llx state normal",
			  stp_request->dev->dev_id);
		/*otherwise,we assume this disk in normal state */
		sat_io_completed(stp_request, SAT_IO_SUCCESS);
		return;
	} else {
		SAT_TRACE(OSP_DBL_MINOR, 5, "Disk 0x%llx D2H fis invalid",
			  stp_request->dev->dev_id);
		/*Ooops...Should never happen */
		sat_io_completed(stp_request, SAT_IO_FAILED);
		return;
	}

}

/**
 * sat_test_unit_ready - translate TEST UNIT READY command from SCSI to ATA,
 * and Send to Device.
 * 
 * @req: sata request info
 * @scsi_cmd: sata request related scsi cmd
 * @sata_dev: sata device related
 *
 */
s32 sat_test_unit_ready(struct stp_req * req,
			struct sat_ini_cmnd * scsi_cmd,
			struct sata_device * sata_dev)
{
	/*fill check power mode fis */
	memset(&req->d2h_fis, 0, sizeof(struct sata_h2d));
	req->h2d_fis.header.fis_type = SATA_FIS_TYPE_H2D;
	req->h2d_fis.header.command = FIS_COMMAND_CHECK_POWER_MODE;
	req->h2d_fis.header.pm_port = 0x80;	/*c bit set to 1,PM port set to 0 */
	/*all other fields are default to 0 */
	req->compl_call_back = sat_test_unit_ready_cb;
	return sat_send_req_to_dev(req);
}

/**
 * sat_read_capacity_16 - translate READ CPACITY(16) command from SCSI to ATA,
 * and Send to Device.
 * 
 * @req: sata request info
 * @scsi_cmd: sata request related scsi cmd
 * @sata_dev: sata device related
 *
 */
s32 sat_read_capacity_16(struct stp_req * req,
			 struct sat_ini_cmnd * scsi_cmd,
			 struct sata_device * sata_dev)
{
	u8 tmp_buff[SCSI_READ_CAPACITY16_PARA_LEN] = { 0 };
	u8 *cdb = NULL;
	u32 alloc_len = 0;
	u64 last_lba = 0;
	u32 copy_len = 0;
	u16 word_118 = 0;
	u16 word_117 = 0;
	struct sal_msg *sal_msg = NULL;

	sal_msg = (struct sal_msg *)req->upper_msg;

	cdb = SAT_GET_SCSI_COMMAND(scsi_cmd);
	/*check if CDB is invalid */
	if (((cdb[1] & 0x1f) != 0x10)	/*service action field should be set to 10h */
	    ||(*((u64 *) & cdb[2]) != 0)	/*Logical block address should be set to 0 */
	    ||(cdb[14] & 0x1)) {	/*PMI bit should be set to 0 */

		SAT_TRACE(OSP_DBL_MINOR, 5, "Disk 0x%llx check CDB failed",
			  req->dev->dev_id);

		sat_set_sense_and_result(req,
					 SCSI_SNSKEY_ILLEGAL_REQUEST,
					 SCSI_SNSCODE_INVALID_FIELD_IN_CDB);
		return SAT_IO_COMPLETE;
	}

	/*allocation length,byte 10,11,12 and 13 */
	alloc_len =
	    ((cdb[10] << 24) | (cdb[11] << 16) | (cdb[12] << 8) | cdb[13]);

	copy_len = SCSI_READ_CAPACITY16_PARA_LEN;
	if (alloc_len < copy_len)
		copy_len = alloc_len;

	if (SAT_GET_SCSI_DATA_BUFF_LEN(scsi_cmd) < copy_len)
		copy_len = SAT_GET_SCSI_DATA_BUFF_LEN(scsi_cmd);

	/*Okay...fill in temp buffer */
	if (sata_dev->dev_cap & SAT_DEV_CAP_48BIT) {
		last_lba = (((u64) sata_dev->sata_identify.max_lba_48_63 << 48)
			    | ((u64) sata_dev->sata_identify.
			       max_lba_32_47 << 32)
			    | ((u64) sata_dev->sata_identify.
			       max_lba_16_31 << 16)
			    | ((u64) sata_dev->sata_identify.max_lba_0_15));
		last_lba -= 1;
		tmp_buff[0] = (u8) ((last_lba >> 56) & 0xFF);	/* MSB */
		tmp_buff[1] = (u8) ((last_lba >> 48) & 0xFF);
		tmp_buff[2] = (u8) ((last_lba >> 40) & 0xFF);
		tmp_buff[3] = (u8) ((last_lba >> 32) & 0xFF);
		tmp_buff[4] = (u8) ((last_lba >> 24) & 0xFF);
		tmp_buff[5] = (u8) ((last_lba >> 16) & 0xFF);
		tmp_buff[6] = (u8) ((last_lba >> 8) & 0xFF);
		tmp_buff[7] = (u8) ((last_lba) & 0xFF);	/* LSB */
	} else {
		last_lba =
		    ((u32)
		     (sata_dev->sata_identify.user_addressable_sectors_hi << 16)
		     | (sata_dev->sata_identify.user_addressable_sectors_lo));
		/* LBA starts from zero */
		last_lba -= 1;
		tmp_buff[0] = 0;	/* MSB */
		tmp_buff[1] = 0;
		tmp_buff[2] = 0;
		tmp_buff[3] = 0;
		tmp_buff[4] = (u8) ((last_lba >> 24) & 0xFF);
		tmp_buff[5] = (u8) ((last_lba >> 16) & 0xFF);
		tmp_buff[6] = (u8) ((last_lba >> 8) & 0xFF);
		tmp_buff[7] = (u8) ((last_lba) & 0xFF);	/* LSB */

	}

	if (((sata_dev->sata_identify.word_104_107[2]) & 0x1000) == 0) {
		/*
		 * Set the block size, fixed at 512 bytes.
		 */
		tmp_buff[8] = 0x00;	/* MSB block size in bytes */
		tmp_buff[9] = 0x00;
		tmp_buff[10] = 0x02;
		tmp_buff[11] = 0x00;	/* LSB block size in bytes */
	} else {
		word_117 = sata_dev->sata_identify.word_112_126[5];
		word_118 = sata_dev->sata_identify.word_112_126[6];

		last_lba = ((u64) word_118 << 16) + word_117;
		last_lba *= 2;
		/* MSB block size in bytes */
		tmp_buff[8] = (u8) ((last_lba >> 24) & 0xFF);
		tmp_buff[9] = (u8) ((last_lba >> 16) & 0xFF);
		tmp_buff[10] = (u8) ((last_lba >> 8) & 0xFF);
		/* LSB block size in bytes */
		tmp_buff[11] = (u8) (last_lba & 0xFF);
	}

	/* fill in MAX LBA, which is used in SendDiagnostic_1() */
	sata_dev->sata_max_lba[0] = tmp_buff[0];	/* MSB */
	sata_dev->sata_max_lba[1] = tmp_buff[1];
	sata_dev->sata_max_lba[2] = tmp_buff[2];
	sata_dev->sata_max_lba[3] = tmp_buff[3];
	sata_dev->sata_max_lba[4] = tmp_buff[4];
	sata_dev->sata_max_lba[5] = tmp_buff[5];
	sata_dev->sata_max_lba[6] = tmp_buff[6];
	sata_dev->sata_max_lba[7] = tmp_buff[7];	/* LSB */

	if (copy_len !=
	    (u32) sal_sg_copy_from_buffer(sal_msg, tmp_buff, copy_len)) {
		SAT_TRACE(OSP_DBL_MINOR, 5, "Disk 0x%llx copy buffer failed",
			  req->dev->dev_id);
		return SAT_IO_FAILED;
	}

	if (SAT_GET_SCSI_DATA_BUFF_LEN(scsi_cmd) != copy_len) {
		SAT_TRACE(OSP_DBL_MINOR, 5,
			  "INI Cmnd data len:%d not equal to %d",
			  SAT_GET_SCSI_DATA_BUFF_LEN(scsi_cmd), copy_len);
	}

	sal_msg->status.io_resp = SAM_STAT_GOOD;
	sal_msg->status.drv_resp = SAL_MSG_DRV_SUCCESS;

	return SAT_IO_COMPLETE;
}

/**
 * sat_read_capacity_10 - translate READ CPACITY(10) command from SCSI to ATA,
 * and Send to Device.
 * 
 * @req: sata request info
 * @scsi_cmd: sata request related scsi cmd
 * @sata_dev: sata device related
 *
 */
s32 sat_read_capacity_10(struct stp_req * req,
			 struct sat_ini_cmnd * scsi_cmd,
			 struct sata_device * sata_dev)
{
	u8 tmp_buff[SCSI_READ_CAPACITY10_PARA_LEN] = { 0 };
	u8 *cdb = NULL;
	u64 last_lba = 0;
	u32 copy_len = 0;
	u16 word_118 = 0;
	u16 word_117 = 0;
	struct sal_msg *sal_msg = NULL;

	sal_msg = (struct sal_msg *)req->upper_msg;

	cdb = SAT_GET_SCSI_COMMAND(scsi_cmd);
	/*check if CDB is invalid */
	if ((*((u32 *) & cdb[2]) != 0)	/*Logical block address should be set to 0 */
	    ||(cdb[8] & 0x1)) {	/*PMI bit should be set to 0 */

		SAT_TRACE(OSP_DBL_MINOR, 5, "Disk 0x%llx check CDB failed",
			  req->dev->dev_id);
		sat_set_sense_and_result(req,
					 SCSI_SNSKEY_ILLEGAL_REQUEST,
					 SCSI_SNSCODE_INVALID_FIELD_IN_CDB);
		return SAT_IO_COMPLETE;
	}

	copy_len = SCSI_READ_CAPACITY10_PARA_LEN;
	if (SAT_GET_SCSI_DATA_BUFF_LEN(scsi_cmd) < copy_len) {
		copy_len = SAT_GET_SCSI_DATA_BUFF_LEN(scsi_cmd);
	}

	/*Okay...fill in temp buffer */
	if (sata_dev->dev_cap & SAT_DEV_CAP_48BIT) {
		if ((0 != sata_dev->sata_identify.max_lba_32_47)
		    || (0 != sata_dev->sata_identify.max_lba_48_63)) {
			tmp_buff[0] = 0xFF;	/* MSB */
			tmp_buff[1] = 0xFF;
			tmp_buff[2] = 0xFF;
			tmp_buff[3] = 0xFF;
		} else {
			last_lba =
			    (((u64) sata_dev->sata_identify.max_lba_16_31 << 16)
			     | ((u64) sata_dev->sata_identify.max_lba_0_15));
			last_lba -= 1;
			tmp_buff[0] = (u8) ((last_lba >> 24) & 0xFF);	/* MSB */
			tmp_buff[1] = (u8) ((last_lba >> 16) & 0xFF);
			tmp_buff[2] = (u8) ((last_lba >> 8) & 0xFF);
			tmp_buff[3] = (u8) ((last_lba) & 0xFF);
		}
	} else {
		last_lba =
		    ((u32) sata_dev->sata_identify.
		     user_addressable_sectors_hi << 16)
		    | (sata_dev->sata_identify.user_addressable_sectors_lo);
		/* LBA starts from zero */
		last_lba -= 1;
		tmp_buff[0] = (u8) ((last_lba >> 24) & 0xFF);
		tmp_buff[1] = (u8) ((last_lba >> 16) & 0xFF);
		tmp_buff[2] = (u8) ((last_lba >> 8) & 0xFF);
		tmp_buff[3] = (u8) ((last_lba) & 0xFF);	/* LSB */

	}

	if (((sata_dev->sata_identify.word_104_107[2]) & 0x1000) == 0) {
		/*
		 * Set the block size, fixed at 512 bytes.
		 */
		tmp_buff[4] = 0x00;	/* MSB block size in bytes */
		tmp_buff[5] = 0x00;
		tmp_buff[6] = 0x02;
		tmp_buff[7] = 0x00;	/* LSB block size in bytes */
	} else {
		word_117 = sata_dev->sata_identify.word_112_126[5];
		word_118 = sata_dev->sata_identify.word_112_126[6];

		last_lba = ((u64) word_118 << 16) + word_117;
		last_lba *= 2;
		/* MSB block size in bytes */
		tmp_buff[4] = (u8) ((last_lba >> 24) & 0xFF);
		tmp_buff[5] = (u8) ((last_lba >> 16) & 0xFF);
		tmp_buff[6] = (u8) ((last_lba >> 8) & 0xFF);
		/* LSB block size in bytes */
		tmp_buff[7] = (u8) (last_lba & 0xFF);
	}

	/* fill in MAX LBA, which is used in SendDiagnostic_1() */
	sata_dev->sata_max_lba[0] = 0;	/* MSB */
	sata_dev->sata_max_lba[1] = 0;
	sata_dev->sata_max_lba[2] = 0;
	sata_dev->sata_max_lba[3] = 0;
	sata_dev->sata_max_lba[4] = tmp_buff[0];
	sata_dev->sata_max_lba[5] = tmp_buff[1];
	sata_dev->sata_max_lba[6] = tmp_buff[2];
	sata_dev->sata_max_lba[7] = tmp_buff[3];	/* LSB */

	if (copy_len !=
	    (u32) sal_sg_copy_from_buffer(sal_msg, tmp_buff, copy_len)) {
		SAT_TRACE(OSP_DBL_MINOR, 5, "Disk:0x%llx copy buffer failed",
			  req->dev->dev_id);

		return SAT_IO_FAILED;
	}

	if (SAT_GET_SCSI_DATA_BUFF_LEN(scsi_cmd) != copy_len) {
		SAT_TRACE(OSP_DBL_MINOR, 5,
			  "INI Cmnd data len:%d not equal to %d",
			  SAT_GET_SCSI_DATA_BUFF_LEN(scsi_cmd), copy_len);
	}

	sal_msg->status.io_resp = SAM_STAT_GOOD;
	sal_msg->status.drv_resp = SAL_MSG_DRV_SUCCESS;

	return SAT_IO_COMPLETE;
}

/**
 * sat_mode_sense_6 - translate MODE SENSE(6) command from SCSI to ATA,
 * and Send to Device.
 * 
 * @req: sata request info
 * @scsi_cmd: sata request related scsi cmd
 * @sata_dev: sata device related
 *
 */
s32 sat_mode_sense_6(struct stp_req * req,
		     struct sat_ini_cmnd * scsi_cmd,
		     struct sata_device * sata_dev)
{
	u8 page_ctl = 0;
	u8 page_code = 0;
	u8 *cdb = NULL;
	u32 copy_len = 0;
	u32 alloc_len = 0;
	u8 tmp_buff[SCSI_MODE_SENSE6_MAX_PAGE_LEN] = { 0 };
	struct sal_msg *sal_msg = NULL;

	sal_msg = (struct sal_msg *)req->upper_msg;

	cdb = SAT_GET_SCSI_COMMAND(scsi_cmd);
	page_ctl = cdb[2] & SCSI_MODE_SENSE6_PC_MASK;
	page_code = cdb[2] & SCSI_MODE_SENSE6_PAGE_CODE_MASK;
	if ((0 != page_ctl)	/*only current values is supported */
	    ||(0 != cdb[3])) {	/*not support sub page code other than 0 */

		SAT_TRACE(OSP_DBL_MINOR, 5, "Disk 0x%llx check CDB failed",
			  req->dev->dev_id);
		sat_set_sense_and_result(req,
					 SCSI_SNSKEY_ILLEGAL_REQUEST,
					 SCSI_SNSCODE_INVALID_FIELD_IN_CDB);
		return SAT_IO_COMPLETE;
	}

	/*Assume DBD bit is set to 0!If DBD bit is set to 1,we will return block
	 *descriptor anyway*/
	switch (page_code) {	/*fill page info according to page number */

	case SCSI_MODESENSE_RETURN_ALL_PAGES:
		/*mode parameter header */
		tmp_buff[0] = SCSI_MODE_SENSE6_RETURN_ALL_PAGES_LEN - 1;
		tmp_buff[3] = 0x08;	/* block descriptor length */
		tmp_buff[10] = 0x02;	/* Block size is always 512 bytes */
		/* MODESENSE_READ_WRITE_ERROR_RECOVERY_PAGE */
		tmp_buff[12] = 0x01;	/* page code */
		tmp_buff[13] = 0x0A;	/* page length */
		tmp_buff[14] = 0x80;	/* AWRE is set */
		/* MODESENSE_CACHING */
		tmp_buff[24] = 0x08;	/* page code */
		tmp_buff[25] = 0x12;	/* page length */
		SAT_SET_DEFAULT_VALUE(!SAT_IS_LOOK_AHEAD_ENABLED
				      (&sata_dev->sata_identify), tmp_buff[36],
				      0x20, 0);
		/* MODESENSE_CONTROL_PAGE */
		tmp_buff[44] = 0x0A;	/* page code */
		tmp_buff[45] = 0x0A;	/* page length */
		tmp_buff[46] = 0x02;	/* only GLTSD bit is set */
		tmp_buff[47] = 0x12;	/* Queue Alogorithm modifier 1b and QErr 01b */
		tmp_buff[52] = 0xFF;	/* Busy Timeout Period */
		tmp_buff[53] = 0xFF;	/* Busy Timeout Period */
		/* MODESENSE_INFORMATION_EXCEPTION_CONTROL_PAGE */
		if (!SAT_IS_SMART_FEATURESET_SUPPORT(&sata_dev->sata_identify))
			break;
		tmp_buff[56] = 0x1C;	/* page code */
		tmp_buff[57] = 0x0A;	/* page length */
		SAT_SET_DEFAULT_VALUE(!SAT_IS_SMART_FEATURESET_ENABLED
				      (&sata_dev->sata_identify), tmp_buff[58],
				      0x08, 0);
		tmp_buff[59] = 0x06;
		break;
	case SCSI_MODESENSE_CONTROL_PAGE:
		tmp_buff[0] = SCSI_MODE_SENSE6_CONTROL_PAGE_LEN - 1;
		tmp_buff[3] = 0x08;	/* block descriptor length */
		tmp_buff[10] = 0x02;	/* Block size is always 512 bytes */
		tmp_buff[12] = 0x0A;	/* page code */
		tmp_buff[13] = 0x0A;	/* page length */
		tmp_buff[14] = 0x02;	/* only GLTSD bit is set */
		tmp_buff[15] = 0x12;	/* Queue Alogorithm modifier 1b and QErr 01b */
		tmp_buff[20] = 0xFF;	/* Busy Timeout Period */
		tmp_buff[21] = 0xFF;	/* Busy Timeout Period */
		break;
	case SCSI_MODESENSE_READ_WRITE_ERROR_RECOVERY_PAGE:
		tmp_buff[0] =
		    SCSI_MODE_SENSE6_READ_WRITE_ERROR_RECOVERY_PAGE_LEN - 1;
		tmp_buff[3] = 0x08;	/* block descriptor length */
		tmp_buff[10] = 0x02;	/* Block size is always 512 bytes */
		tmp_buff[12] = 0x01;	/* page code */
		tmp_buff[13] = 0x0A;	/* page length */
		tmp_buff[14] = 0x80;	/* AWRE is set */
		break;
	case SCSI_MODESENSE_CACHING:
		tmp_buff[0] = SCSI_MODE_SENSE6_CACHING_LEN - 1;
		tmp_buff[3] = 0x08;	/* block descriptor length */
		/* Fill-up direct-access device block-descriptor */
		tmp_buff[10] = 0x02;	/* Block size is always 512 bytes */
		/*Fill-up Caching mode page */
		tmp_buff[12] = 0x08;	/* page code */
		tmp_buff[13] = 0x12;	/* page length */
		SAT_SET_DEFAULT_VALUE(SAT_IS_WRITE_CACHE_ENABLED
				      (&sata_dev->sata_identify), tmp_buff[14],
				      0x04, 0);
		SAT_SET_DEFAULT_VALUE(!SAT_IS_LOOK_AHEAD_ENABLED
				      (&sata_dev->sata_identify), tmp_buff[24],
				      0x20, 0);
		break;
	case SCSI_MODESENSE_INFORMATION_EXCEPTION_CONTROL_PAGE:
		tmp_buff[0] =
		    SCSI_MODE_SENSE6_INFORMATION_EXCEPTION_CONTROL_PAGE_LEN - 1;
		tmp_buff[3] = 0x08;	/* block descriptor length */
		tmp_buff[10] = 0x02;	/* Block size is always 512 bytes */
		/* Fill-up informational-exceptions control mode page */
		tmp_buff[12] = 0x1C;	/* page code */
		tmp_buff[13] = 0x0A;	/* page length */
		SAT_SET_DEFAULT_VALUE(!SAT_IS_SMART_FEATURESET_ENABLED
				      (&sata_dev->sata_identify), tmp_buff[14],
				      0x08, 0);
		tmp_buff[15] = 0x06;
		break;
	case SCSI_MODESENSE_VENDOR_SPECIFIC_PAGE:
	default:
		SAT_TRACE(OSP_DBL_MINOR, 5,
			  "Disk 0x%llx don't support sense page 0x%x",
			  req->dev->dev_id, page_code);
		sat_set_sense_and_result(req, SCSI_SNSKEY_ILLEGAL_REQUEST,
					 SCSI_SNSCODE_INVALID_FIELD_IN_CDB);
		return SAT_IO_COMPLETE;
	}

	alloc_len = cdb[4];
	copy_len = tmp_buff[0] + 1;
	copy_len = MIN(copy_len, alloc_len);
	copy_len = MIN(copy_len, SAT_GET_SCSI_DATA_BUFF_LEN(scsi_cmd));

	if (copy_len !=
	    (u32) sal_sg_copy_from_buffer(sal_msg, tmp_buff, copy_len)) {
		SAT_TRACE(OSP_DBL_MINOR, 5,
			  "Device 0x%llx copy form buffer failed",
			  sata_dev->dev_id);
		return SAT_IO_FAILED;
	}

	if (SAT_GET_SCSI_DATA_BUFF_LEN(scsi_cmd) != copy_len) {
		SAT_TRACE(OSP_DBL_MINOR, 5,
			  "INI Cmnd data len:%d not equal to %d",
			  SAT_GET_SCSI_DATA_BUFF_LEN(scsi_cmd), copy_len);
	}

	sal_msg->status.io_resp = SAM_STAT_GOOD;
	sal_msg->status.drv_resp = SAL_MSG_DRV_SUCCESS;
	return SAT_IO_COMPLETE;
}

/**
 * sat_mode_sense_10 - translate MODE SENSE(10) command from SCSI to ATA,
 * and Send to Device.
 * 
 * @req: sata request info
 * @scsi_cmd: sata request related scsi cmd
 * @sata_dev: sata device related
 *
 */
s32 sat_mode_sense_10(struct stp_req * req,
		      struct sat_ini_cmnd * scsi_cmd,
		      struct sata_device * sata_dev)
{
	u8 page_ctl = 0;
	u8 page_code = 0;
	u8 long_lba = 0;
	u8 len_add = 0;
	u8 *cdb = NULL;
	u32 copy_len = 0;
	u32 alloc_len = 0;
	u8 tmp_buff[SCSI_MODE_SENSE10_MAX_PAGE_LEN] = { 0 };
	struct sal_msg *sal_msg = NULL;

	sal_msg = (struct sal_msg *)req->upper_msg;

	cdb = SAT_GET_SCSI_COMMAND(scsi_cmd);
	long_lba = cdb[1] & SCSI_MODE_SENSE10_LLBAA_MASK;
	page_code = cdb[2] & SCSI_MODE_SENSE10_PAGE_CODE_MASK;
	page_ctl = cdb[2] & SCSI_MODE_SENSE10_PC_MASK;
	SAT_SET_DEFAULT_VALUE(long_lba != 0, len_add, 8, 0);
	if ((0 != page_ctl)	/*only current values is supported */
	    ||(0 != cdb[3])) {	/*not support sub page code other than 0 */

		SAT_TRACE(OSP_DBL_MINOR, 5, "Disk 0x%llx check CDB failed",
			  req->dev->dev_id);
		sat_set_sense_and_result(req,
					 SCSI_SNSKEY_ILLEGAL_REQUEST,
					 SCSI_SNSCODE_INVALID_FIELD_IN_CDB);
		return SAT_IO_COMPLETE;
	}

	/*Assume DBD bit is set to 0!If DBD bit is set to 1,we will return block
	 *descriptor anyway*/
	switch (page_code) {	/*fill page info according to page number */

	case SCSI_MODESENSE_RETURN_ALL_PAGES:
		/*mode parameter header */
		tmp_buff[1] =
		    (u8) (SCSI_MODE_SENSE10_RETURN_ALL_PAGES_LEN + len_add - 2);
		SAT_SET_DEFAULT_VALUE(long_lba, tmp_buff[4], 0x1, 0);
		tmp_buff[7] = 0x08 + len_add;	/* block descriptor length */
		tmp_buff[14 + len_add] = 0x02;	/* Block size is always 512 bytes */
		/* MODESENSE_READ_WRITE_ERROR_RECOVERY_PAGE */
		tmp_buff[16 + 0 + len_add] = 0x01;	/* page code */
		tmp_buff[16 + 1 + len_add] = 0x0A;	/* page length */
		tmp_buff[16 + 2 + len_add] = 0x80;	/* AWRE is set */
		/* MODESENSE_CACHING_PAGE */
		tmp_buff[16 + 12 + len_add] = 0x08;	/* page code */
		tmp_buff[16 + 13 + len_add] = 0x12;	/* page length */
		SAT_SET_DEFAULT_VALUE(SAT_IS_WRITE_CACHE_ENABLED
				      (&sata_dev->sata_identify),
				      tmp_buff[16 + 14 + len_add], 0x04, 0);
		SAT_SET_DEFAULT_VALUE(!SAT_IS_LOOK_AHEAD_ENABLED
				      (&sata_dev->sata_identify),
				      tmp_buff[16 + 24 + len_add], 0x20, 0);
		/* MODESENSE_CONTROL_PAGE */
		tmp_buff[16 + 32 + len_add] = 0x0A;	/* page code */
		tmp_buff[16 + 33 + len_add] = 0x0A;	/* page length */
		tmp_buff[16 + 34 + len_add] = 0x02;	/* only GLTSD bit is set */
		tmp_buff[16 + 35 + len_add] = 0x12;	/* Queue Alogorithm modifier 1b and QErr 01b */
		tmp_buff[16 + 40 + len_add] = 0xFF;	/* Busy Timeout Period */
		tmp_buff[16 + 41 + len_add] = 0xFF;	/* Busy Timeout Period */
		/* MODESENSE_INFORMATION_EXCEPTION_CONTROL_PAGE */
		if (!SAT_IS_SMART_FEATURESET_SUPPORT(&sata_dev->sata_identify))
			break;

		tmp_buff[16 + 44 + len_add] = 0x1C;	/* page code */
		tmp_buff[16 + 45 + len_add] = 0x0A;	/* page length */
		SAT_SET_DEFAULT_VALUE(!SAT_IS_SMART_FEATURESET_ENABLED
				      (&sata_dev->sata_identify),
				      tmp_buff[16 + 46 + len_add], 0x08, 0);
		tmp_buff[16 + 47 + len_add] = 0x06;
		break;
	case SCSI_MODESENSE_CONTROL_PAGE:
		tmp_buff[1] = SCSI_MODE_SENSE10_CONTROL_PAGE_LEN + len_add - 2;
		SAT_SET_DEFAULT_VALUE(long_lba, tmp_buff[4], 0x1, 0);
		tmp_buff[7] = 0x08 + len_add;	/* block descriptor length */
		tmp_buff[14 + len_add] = 0x02;	/* Block size is always 512 bytes */
		tmp_buff[16 + 0 + len_add] = 0x0A;	/* page code */
		tmp_buff[16 + 1 + len_add] = 0x0A;	/* page length */
		tmp_buff[16 + 2 + len_add] = 0x80;	/* AWRE is set */
		tmp_buff[16 + 3 + len_add] = 0x12;	/* Queue Alogorithm modifier 1b and QErr 01b */
		tmp_buff[16 + 8 + len_add] = 0xFF;	/* Busy Timeout Period */
		tmp_buff[16 + 9 + len_add] = 0xFF;	/* Busy Timeout Period */
		break;
	case SCSI_MODESENSE_READ_WRITE_ERROR_RECOVERY_PAGE:
		tmp_buff[1] =
		    SCSI_MODE_SENSE10_READ_WRITE_ERROR_RECOVERY_PAGE_LEN +
		    len_add - 2;
		SAT_SET_DEFAULT_VALUE(long_lba, tmp_buff[4], 0x1, 0);
		tmp_buff[7] = 0x08 + len_add;	/* block descriptor length */
		tmp_buff[14 + len_add] = 0x02;	/* Block size is always 512 bytes */
		tmp_buff[16 + 0 + len_add] = 0x01;	/* page code */
		tmp_buff[16 + 1 + len_add] = 0x0A;	/* page length */
		tmp_buff[16 + 2 + len_add] = 0x80;	/* AWRE is set */
		break;
	case SCSI_MODESENSE_CACHING:
		tmp_buff[1] = SCSI_MODE_SENSE10_CACHING_LEN + len_add - 2;
		SAT_SET_DEFAULT_VALUE(long_lba, tmp_buff[4], 0x1, 0);
		tmp_buff[7] = 0x08 + len_add;	/* block descriptor length */
		tmp_buff[14 + len_add] = 0x02;	/* Block size is always 512 bytes */
		tmp_buff[16 + 0 + len_add] = 0x08;	/* page code */
		tmp_buff[16 + 1 + len_add] = 0x12;	/* page length */
		SAT_SET_DEFAULT_VALUE(SAT_IS_WRITE_CACHE_ENABLED
				      (&sata_dev->sata_identify),
				      tmp_buff[16 + 2 + len_add], 0x04, 0);
		SAT_SET_DEFAULT_VALUE(!SAT_IS_LOOK_AHEAD_ENABLED
				      (&sata_dev->sata_identify),
				      tmp_buff[16 + 12 + len_add], 0x20, 0);
		break;
	case SCSI_MODESENSE_INFORMATION_EXCEPTION_CONTROL_PAGE:
		tmp_buff[1] =
		    SCSI_MODE_SENSE10_INFORMATION_EXCEPTION_CONTROL_PAGE_LEN +
		    len_add - 2;
		SAT_SET_DEFAULT_VALUE(long_lba, tmp_buff[4], 0x1, 0);
		tmp_buff[7] = 0x08 + len_add;	/* block descriptor length */
		tmp_buff[14 + len_add] = 0x02;	/* Block size is always 512 bytes */
		tmp_buff[16 + 0 + len_add] = 0x1C;	/* page code */
		tmp_buff[16 + 1 + len_add] = 0x0A;	/* page length */
		SAT_SET_DEFAULT_VALUE(!SAT_IS_SMART_FEATURESET_ENABLED
				      (&sata_dev->sata_identify),
				      tmp_buff[16 + 2 + len_add], 0x08, 0);
		tmp_buff[16 + 3 + len_add] = 0x06;
		break;
	case SCSI_MODESENSE_VENDOR_SPECIFIC_PAGE:
	default:
		sat_set_sense_and_result(req,
					 SCSI_SNSKEY_ILLEGAL_REQUEST,
					 SCSI_SNSCODE_INVALID_FIELD_IN_CDB);
		return SAT_IO_COMPLETE;
	}

	alloc_len = ((cdb[7] << 8) | cdb[8]);
	copy_len = tmp_buff[1] + 2;
	copy_len = MIN(copy_len, alloc_len);
	copy_len = MIN(copy_len, SAT_GET_SCSI_DATA_BUFF_LEN(scsi_cmd));

	if (copy_len !=
	    (u32) sal_sg_copy_from_buffer(sal_msg, tmp_buff, copy_len)) {
		SAT_TRACE(OSP_DBL_MINOR, 5,
			  "Device 0x%llx copy form buffer failed",
			  sata_dev->dev_id);
		return SAT_IO_FAILED;
	}

	if (SAT_GET_SCSI_DATA_BUFF_LEN(scsi_cmd) != copy_len) {
		SAT_TRACE(OSP_DBL_MINOR, 5,
			  "INI Cmnd data len:%d not equal to %d",
			  SAT_GET_SCSI_DATA_BUFF_LEN(scsi_cmd), copy_len);

	}
	sal_msg->status.io_resp = SAM_STAT_GOOD;
	sal_msg->status.drv_resp = SAL_MSG_DRV_SUCCESS;

	return SAT_IO_COMPLETE;
}

/**
 * sat_calc_lba - calculate the LBA based on CDB type
 * 
 * @cdb: cdb of the SCSI command
 *
 */
u32 sat_calc_lba(u8 * cdb)
{
	switch (cdb[0]) {
		/*6bytes cdb */
	case SCSIOPC_READ_6:
	case SCSIOPC_WRITE_6:
		return (((cdb[1] & 0x1f) << (8 * 2))
			+ (cdb[2] << 8) + (cdb[3]));
	case SCSIOPC_READ_10:
	case SCSIOPC_READ_12:
	case SCSIOPC_WRITE_10:
	case SCSIOPC_WRITE_12:
	case SCSIOPC_VERIFY_10:
	case SCSIOPC_VERIFY_12:
	case SCSIOPC_WRITE_SAME_10:
	case SCSIOPC_WRITE_AND_VERIFY_10:
	case SCSIOPC_WRITE_AND_VERIFY_12:
		/*10/12bytes cdb */
		return ((cdb[2] << (8 * 3)) + (cdb[3] << (8 * 2))
			+ (cdb[4] << 8) + cdb[5]);
	case SCSIOPC_READ_16:
	case SCSIOPC_WRITE_16:
	case SCSIOPC_VERIFY_16:
	case SCSIOPC_WRITE_AND_VERIFY_16:
		/*16bytes cdb */

		return ((cdb[6] << (8 * 3)) + (cdb[7] << (8 * 2))
			+ (cdb[8] << 8) + cdb[9]);
	default:
		/*others */
		return 0;
	}
}


/**
 * sat_calc_trans_len - calculate the transfer length based on CDB type
 * 
 * @cdb: cdb of the SCSI command
 *
 */
u32 sat_calc_trans_len(u8 * cdb)
{
	switch (cdb[0]) {
	case SCSIOPC_READ_6:
	case SCSIOPC_WRITE_6:
		/*6bytes cdb */
		return cdb[4];
	case SCSIOPC_READ_10:
	case SCSIOPC_WRITE_10:
	case SCSIOPC_VERIFY_10:
	case SCSIOPC_WRITE_SAME_10:
	case SCSIOPC_WRITE_AND_VERIFY_10:
		/*10bytes cdb */
		return (cdb[7] << 8) + cdb[8];
	case SCSIOPC_READ_12:
	case SCSIOPC_WRITE_12:
	case SCSIOPC_VERIFY_12:
	case SCSIOPC_WRITE_AND_VERIFY_12:
		/*12bytes cdb */
		return ((cdb[6] << (8 * 3)) + (cdb[7] << (8 * 2))
			+ (cdb[8] << 8) + cdb[9]);
	case SCSIOPC_READ_16:
	case SCSIOPC_WRITE_16:
	case SCSIOPC_VERIFY_16:
	case SCSIOPC_WRITE_AND_VERIFY_16:
		/*16bytes cdb */
		return ((cdb[10] << (8 * 3)) + (cdb[11] << (8 * 2))
			+ (cdb[12] << 8) + cdb[13]);
	default:
		/*others */
		return 0;
	}
}

/**
 * sat_check_28bit_lba - check whether LBA and TL is beyond SAT_TR_LBA_LIMIT ,
 * based on SCSI command
 * 
 * @cdb: cdb of the SCSI command
 *
 */
s32 sat_check_28bit_lba(u8 * cdb)
{
	u64 result = 0;
	u64 lba = 0, len = 0;

	switch (cdb[0]) {
	case SCSIOPC_READ_6:
	case SCSIOPC_READ_10:
	case SCSIOPC_READ_12:
	case SCSIOPC_WRITE_6:
	case SCSIOPC_WRITE_10:
	case SCSIOPC_WRITE_12:
	case SCSIOPC_WRITE_AND_VERIFY_10:
	case SCSIOPC_WRITE_AND_VERIFY_12:
	case SCSIOPC_VERIFY_10:
	case SCSIOPC_VERIFY_12:
		lba = sat_calc_lba(cdb);
		len = sat_calc_trans_len(cdb);
		result = lba + len;

		/* overflow when add by each other */
		if (result < lba || result < len)
			return true;

		/* length beyond SAT_TR_LBA_LIMIT */
		if (result >= SAT_TR_LBA_LIMIT)
			return true;

		return false;
	case SCSIOPC_READ_16:
	case SCSIOPC_WRITE_16:
	case SCSIOPC_WRITE_AND_VERIFY_16:
	case SCSIOPC_VERIFY_16:
		/*get the start lba */
		lba = ((u64) cdb[2] << (8 * 7)) + ((u64) cdb[3] << (8 * 6))
		    + ((u64) cdb[4] << (8 * 5)) + ((u64) cdb[5] << (8 * 4))
		    + ((u64) cdb[6] << (8 * 3)) + ((u64) cdb[7] << (8 * 2))
		    + ((u64) cdb[8] << 8) + (u64) cdb[9];
		/*get transfer length */
		len = sat_calc_trans_len(cdb);
		result = lba + len;
		/* overflow when add by each other */
		if (result < lba || result < len)
			return true;

		/* length beyond SAT_TR_LBA_LIMIT */
		if (result >= SAT_TR_LBA_LIMIT)
			return true;

		/*normal */
		return false;
	default:
		/*default is ok */
		return false;
	}
}

void sat_fill_pio_fis(struct sata_h2d *fis, u8 * cdb)
{
	fis->header.fis_type = 0x27;	/* Reg host to device */
	fis->header.pm_port = 0x80;	/* C Bit is set */

	if (SAT_IS_READ_COMMAND(cdb[0]))
		fis->header.command = FIS_COMMAND_READ_SECTOR;	/* 0x20 */
	else
		fis->header.command = FIS_COMMAND_WRITE_SECTOR;	/* 0x20 */

	fis->header.features = 0;	/* FIS reserve */
	fis->data.lba_low_exp = 0;
	fis->data.lba_mid_exp = 0;
	fis->data.lba_high_exp = 0;
	fis->data.features_exp = 0;
	fis->data.sector_cnt_exp = 0;
	fis->data.reserved_4 = 0;
	fis->data.control = 0;	/* FIS HOB bit clear */
	fis->data.reserved_5 = 0;

	switch (cdb[0]) {
	case SCSIOPC_READ_6:
	case SCSIOPC_WRITE_6:
		fis->data.lba_low = cdb[3];	/* FIS LBA (7 :0 ) */
		fis->data.lba_mid = cdb[2];	/* FIS LBA (15:8 ) */
		fis->data.lab_high = (cdb[1]) & 0x1f;	/* FIS LBA (23:16) */
		fis->data.device = 0x40;	/* FIS LBA mode  */
		if (0 == cdb[4])
			/* temporary fix */
			fis->data.sector_cnt = 0xff;	/* FIS sector count (7:0) */
		else
			fis->data.sector_cnt = cdb[4];	/* FIS sector count (7:0) */

		break;
	case SCSIOPC_READ_10:
	case SCSIOPC_WRITE_10:
		fis->data.lba_low = cdb[5];	/* FIS LBA (7 :0 ) */
		fis->data.lba_mid = cdb[4];	/* FIS LBA (15:8 ) */
		fis->data.lab_high = cdb[3];	/* FIS LBA (23:16) */
		fis->data.device = (0x4 << 4) | (cdb[2] & 0xF);	/* FIS LBA (27:24) and FIS LBA mode  */
		fis->data.sector_cnt = cdb[8];	/* FIS sector count (7:0) */
		break;
	case SCSIOPC_READ_12:
	case SCSIOPC_WRITE_12:
		fis->data.lba_low = cdb[5];	/* FIS LBA (7 :0 ) */
		fis->data.lba_mid = cdb[4];	/* FIS LBA (15:8 ) */
		fis->data.lab_high = cdb[3];	/* FIS LBA (23:16) */
		fis->data.device = (0x4 << 4) | (cdb[2] & 0xF);	/* FIS LBA (27:24) and FIS LBA mode  */
		fis->data.sector_cnt = cdb[9];	/* FIS sector count (7:0) */
		break;
	case SCSIOPC_READ_16:
	case SCSIOPC_WRITE_16:
		fis->data.lba_low = cdb[9];	/* FIS LBA (7 :0 ) */
		fis->data.lba_mid = cdb[8];	/* FIS LBA (15:8 ) */
		fis->data.lab_high = cdb[7];	/* FIS LBA (23:16) */
		fis->data.device = (0x4 << 4) | (cdb[6] & 0xF);	/* FIS LBA (27:24) and FIS LBA mode  */
		fis->data.sector_cnt = cdb[13];	/* FIS sector count (7:0) */
		break;
	default:
		SAT_TRACE(OSP_DBL_MINOR, 5, "Invalid operation code 0x%x",
			  cdb[0]);
		break;
	}
	return;
}

void sat_fill_dma_fis(struct sata_h2d *fis, u8 * cdb)
{
	sat_fill_pio_fis(fis, cdb);
	if (SAT_IS_READ_COMMAND(cdb[0]))
		fis->header.command = FIS_COMMAND_READ_DMA;	/* 0xC8,all other field are the same */
	else
		fis->header.command = FIS_COMMAND_WRITE_DMA;	/* 0xC8,all other field are the same */

	return;
}

void sat_fill_pio_ext_fis(struct sata_h2d *fis, u8 * cdb)
{
	fis->header.fis_type = 0x27;	/* Reg host to device */
	fis->header.pm_port = 0x80;	/* C Bit is set */
	if (SAT_IS_READ_COMMAND(cdb[0]))
		fis->header.command = FIS_COMMAND_READ_SECTOR_EXT;	/* 0x24 */
	else
		fis->header.command = FIS_COMMAND_WRITE_SECTOR_EXT;	/* 0x24 */

	fis->header.features = 0;	/* FIS reserve */

	fis->data.features_exp = 0;	/* FIS reserve */
	fis->data.device = 0x40;	/* FIS LBA mode set */
	fis->data.reserved_4 = 0;
	fis->data.control = 0;	/* FIS HOB bit clear */
	fis->data.reserved_5 = 0;

	switch (cdb[0]) {
	case SCSIOPC_READ_6:
	case SCSIOPC_WRITE_6:
		fis->data.lba_low = cdb[3];	/* FIS LBA (7 :0 ) */
		fis->data.lba_mid = cdb[2];	/* FIS LBA (15:8 ) */
		fis->data.lab_high = (cdb[1]) & 0x1f;	/* FIS LBA (23:16) */
		fis->data.lba_low_exp = 0;	/* FIS LBA (31:24) */
		fis->data.lba_mid_exp = 0;	/* FIS LBA (39:32) */
		fis->data.lba_high_exp = 0;	/* FIS LBA (47:40) */
		if (0 == cdb[4]) {
			/* sector count is 256, 0x100 */
			fis->data.sector_cnt = 0;	/* FIS sector count (7:0) */
			fis->data.sector_cnt_exp = 0x01;	/* FIS sector count (15:8) */
		} else {
			fis->data.sector_cnt = cdb[4];	/* FIS sector count (7:0) */
			fis->data.sector_cnt_exp = 0;	/* FIS sector count (15:8) */
		}
		break;
	case SCSIOPC_READ_10:
	case SCSIOPC_WRITE_10:
		fis->data.lba_low = cdb[5];	/* FIS LBA (7 :0 ) */
		fis->data.lba_mid = cdb[4];	/* FIS LBA (15:8 ) */
		fis->data.lab_high = cdb[3];	/* FIS LBA (23:16) */
		fis->data.lba_low_exp = cdb[2];	/* FIS LBA (31:24) */
		fis->data.lba_mid_exp = 0;	/* FIS LBA (39:32) */
		fis->data.lba_high_exp = 0;	/* FIS LBA (47:40) */
		fis->data.sector_cnt = cdb[8];	/* FIS sector count (7:0) */
		fis->data.sector_cnt_exp = cdb[7];	/* FIS sector count (15:8) */
		break;
	case SCSIOPC_READ_12:
	case SCSIOPC_WRITE_12:
		fis->data.lba_low = cdb[5];	/* FIS LBA (7 :0 ) */
		fis->data.lba_mid = cdb[4];	/* FIS LBA (15:8 ) */
		fis->data.lab_high = cdb[3];	/* FIS LBA (23:16) */
		fis->data.lba_low_exp = cdb[2];	/* FIS LBA (31:24) */
		fis->data.lba_mid_exp = 0;	/* FIS LBA (39:32) */
		fis->data.lba_high_exp = 0;	/* FIS LBA (47:40) */
		fis->data.sector_cnt = cdb[9];	/* FIS sector count (7:0) */
		fis->data.sector_cnt_exp = cdb[8];	/* FIS sector count (15:8) */
		break;
	case SCSIOPC_READ_16:
	case SCSIOPC_WRITE_16:
		fis->data.lba_low = cdb[9];	/* FIS LBA (7 :0 ) */
		fis->data.lba_mid = cdb[8];	/* FIS LBA (15:8 ) */
		fis->data.lab_high = cdb[7];	/* FIS LBA (23:16) */
		fis->data.lba_low_exp = cdb[6];	/* FIS LBA (31:24) */
		fis->data.lba_mid_exp = cdb[5];	/* FIS LBA (39:32) */
		fis->data.lba_high_exp = cdb[4];	/* FIS LBA (47:40) */
		fis->data.sector_cnt = cdb[13];	/* FIS sector count (7:0) */
		fis->data.sector_cnt_exp = cdb[12];	/* FIS sector count (15:8) */
		break;
	default:
		SAT_TRACE(OSP_DBL_MINOR, 5, "Invalid operation code 0x%x",
			  cdb[0]);
		break;
	}
	return;
}

void sat_fill_dma_ext_fis(struct sata_h2d *fis, u8 * cdb)
{
	sat_fill_pio_ext_fis(fis, cdb);
	if (SAT_IS_READ_COMMAND(cdb[0]))
		fis->header.command = FIS_COMMAND_READ_DMA_EXT;	/* 0x25 */
	else
		fis->header.command = FIS_COMMAND_WRITE_DMA_EXT;	/* 0x25 */

	return;
}

void sat_fill_ncq_fis(struct sata_h2d *fis, u8 * cdb)
{
	/* Support 48-bit FPDMA addressing, use READ FPDMA QUEUE command */
	fis->header.fis_type = 0x27;	/* Reg host to device */
	fis->header.pm_port = 0x80;	/* C Bit is set */
	if (SAT_IS_READ_COMMAND(cdb[0]))
		fis->header.command = FIS_COMMAND_READ_FPDMA_QUEUED;	/* 0x60 */
	else
		fis->header.command = FIS_COMMAND_WRITE_FPDMA_QUEUED;

	fis->data.device = 0x40;	/* FIS FUA clear */
	fis->data.sector_cnt = 0;	/* Tag (7:3) set by LL layer */
	fis->data.sector_cnt_exp = 0;
	fis->data.reserved_4 = 0;
	fis->data.control = 0;	/* FIS HOB bit clear */
	fis->data.reserved_5 = 0;

	switch (cdb[0]) {
	case SCSIOPC_READ_10:
	case SCSIOPC_WRITE_10:
		fis->header.features = cdb[8];	/* FIS sector count (7:0) */
		fis->data.lba_low = cdb[5];	/* FIS LBA (7 :0 ) */
		fis->data.lba_mid = cdb[4];	/* FIS LBA (15:8 ) */
		fis->data.lab_high = cdb[3];	/* FIS LBA (23:16) */
		/* Check FUA bit */
		if (cdb[1] & SCSI_READ_WRITE_10_FUA_MASK)
			fis->data.device = 0xC0;	/* FIS FUA set */

		fis->data.lba_low_exp = cdb[2];	/* FIS LBA (31:24) */
		fis->data.lba_mid_exp = 0;	/* FIS LBA (39:32) */
		fis->data.lba_high_exp = 0;	/* FIS LBA (47:40) */
		fis->data.features_exp = cdb[7];	/* FIS sector count (15:8) */
		break;
	case SCSIOPC_READ_6:
	case SCSIOPC_WRITE_6:
		fis->data.lba_low = cdb[3];	/* FIS LBA (7 :0 ) */
		fis->data.lba_mid = cdb[2];	/* FIS LBA (15:8 ) */
		fis->data.lab_high = (cdb[1]) & 0x1f;	/* FIS LBA (23:16) */
		fis->data.lba_low_exp = 0;	/* FIS LBA (31:24) */
		fis->data.lba_mid_exp = 0;	/* FIS LBA (39:32) */
		fis->data.lba_high_exp = 0;	/* FIS LBA (47:40) */
		/*
		 * see Sbc,for READ (6), Transfer length equal to 0 means read 256
		 * but for READ(10), READ(12), READ(16),0 means read nothing
		 */
		if (0 == cdb[4]) {
			/* sector count is 256, 0x100 */
			fis->header.features = 0;	/* FIS sector count (7:0) */
			fis->data.features_exp = 0x01;	/* FIS sector count (15:8) */
		} else {
			fis->header.features = cdb[4];	/* FIS sector count (7:0) */
			fis->data.features_exp = 0;	/* FIS sector count (15:8) */
		}
		break;
	case SCSIOPC_READ_12:
	case SCSIOPC_WRITE_12:
		fis->header.features = cdb[9];	/* FIS sector count (7:0) */
		fis->data.lba_low = cdb[5];	/* FIS LBA (7 :0 ) */
		fis->data.lba_mid = cdb[4];	/* FIS LBA (15:8 ) */
		fis->data.lab_high = cdb[3];	/* FIS LBA (23:16) */
		/* Check FUA bit */
		if (cdb[1] & SCSI_READ_WRITE_12_FUA_MASK)
			fis->data.device = 0xC0;	/* FIS FUA set */

		fis->data.lba_low_exp = cdb[2];	/* FIS LBA (31:24) */
		fis->data.lba_mid_exp = 0;	/* FIS LBA (39:32) */
		fis->data.lba_high_exp = 0;	/* FIS LBA (47:40) */
		fis->data.features_exp = cdb[8];	/* FIS sector count (15:8) */
		break;
	case SCSIOPC_READ_16:
	case SCSIOPC_WRITE_16:
		fis->header.features = cdb[13];	/* FIS sector count (7:0) */
		fis->data.lba_low = cdb[9];	/* FIS LBA (7 :0 ) */
		fis->data.lba_mid = cdb[8];	/* FIS LBA (15:8 ) */
		fis->data.lab_high = cdb[7];	/* FIS LBA (23:16) */
		/* Check FUA bit */
		if (cdb[1] & SCSI_READ_WRITE_16_FUA_MASK)
			fis->data.device = 0xC0;	/* FIS FUA set */

		fis->data.lba_low_exp = cdb[6];	/* FIS LBA (31:24) */
		fis->data.lba_mid_exp = cdb[5];	/* FIS LBA (39:32) */
		fis->data.lba_high_exp = cdb[4];	/* FIS LBA (47:40) */
		fis->data.features_exp = cdb[12];	/* FIS sector count (15:8) */
		break;
	default:
		SAT_TRACE(OSP_DBL_MINOR, 5, "Invalid operation code:0x%x",
			  cdb[0]);
		break;
	}
	return;
}

s32 sat_fill_read_write_fis(struct stp_req * req,
			    u8 * cdb, struct sata_device * sata_dev)
{
	if (!(sata_dev->dev_cap & SAT_DEV_CAP_48BIT)) {
		if (true == sat_check_28bit_lba(cdb)) {
			SAT_TRACE(OSP_DBL_MINOR, 125,
				  "Disk 0x%llx check 28bit LBA failed",
				  sata_dev->dev_id);
			return ERROR;
		}
	}

	switch (sata_dev->dev_transfer_mode) {
	case SAT_TRANSFER_MOD_NCQ:
		sat_fill_ncq_fis(&req->h2d_fis, cdb);
		break;
	case SAT_TRANSFER_MOD_DMA_EXT:
		sat_fill_dma_ext_fis(&req->h2d_fis, cdb);
		break;
	case SAT_TRANSFER_MOD_PIO_EXT:
		sat_fill_pio_ext_fis(&req->h2d_fis, cdb);
		break;
	case SAT_TRANSFER_MOD_DMA:
		sat_fill_dma_fis(&req->h2d_fis, cdb);
		break;
	case SAT_TRANSFER_MOD_PIO:
		sat_fill_pio_fis(&req->h2d_fis, cdb);
		break;
	default:
		return ERROR;
	}
	return OK;
}

void sat_common_cb(struct stp_req *stp_request, u32 io_status)
{
	/*IO success,return success to upper layer */
	sat_io_completed(stp_request, SAT_IO_SUCCESS);
	return;
}

/**
 * sat_read_write - translate READ(6) READ(10) READ(12) READ(16) WRITE(6)
 * WRITE(10) WRITE(12) WRITE(16)  command from SCSI to ATA,
 * and Send to Device.
 * 
 * @req: sata request info
 * @scsi_cmd: sata request related scsi cmd
 * @sata_dev: sata device related
 *
 */
s32 sat_read_write(struct stp_req * req,
		   struct sat_ini_cmnd * scsi_cmd,
		   struct sata_device * sata_dev)
{
	u8 *cdb = NULL;

	cdb = SAT_GET_SCSI_COMMAND(scsi_cmd);
	if ((cdb[1] & SCSI_FUA_NV_MASK)
	    && !((SCSIOPC_READ_6 == cdb[0]) || (SCSIOPC_WRITE_6 == cdb[0]))) {
		/*read(6) write(6) has no FUA NV bit */
		SAT_TRACE(OSP_DBL_MINOR, 5, "Disk:0x%llx check CDB failed",
			  req->dev->dev_id);
		sat_set_sense_and_result(req,
					 SCSI_SNSKEY_ILLEGAL_REQUEST,
					 SCSI_SNSCODE_INVALID_FIELD_IN_CDB);
		return SAT_IO_COMPLETE;
	}

	/*Fill the fis */
	if (OK != sat_fill_read_write_fis(req, cdb, sata_dev)) {
		SAT_TRACE(OSP_DBL_MINOR, 5, "Device:0x%llx fill fis failed",
			  sata_dev->dev_id);
		return SAT_IO_FAILED;
	}

	req->compl_call_back = sat_common_cb;
	return sat_send_req_to_dev(req);
}

void sat_start_stop_unit_cb(struct stp_req *stp_request, u32 io_status)
{
	struct sata_h2d *tmp_fis = NULL;

	tmp_fis = &stp_request->h2d_fis;
	if ((FIS_COMMAND_FLUSH_CACHE == tmp_fis->header.command)
	    || (FIS_COMMAND_FLUSH_CACHE_EXT == tmp_fis->header.command)) {
		/*send a ATA STANDBY command */
		stp_request->d2h_valid = SAT_D2H_INVALID;
		tmp_fis->header.command = FIS_COMMAND_STANDBY;
		stp_request->compl_call_back = sat_start_stop_unit_cb;
		if (SAT_IO_SUCCESS != sat_send_req_to_dev(stp_request)) {
			sat_set_sense_and_result(stp_request,
						 SCSI_SNSKEY_ABORTED_COMMAND,
						 SCSI_SNSCODE_COMMAND_SEQUENCE_ERROR);
			sat_io_completed(stp_request, SAT_IO_SUCCESS);
		}
		return;
	}

	sat_io_completed(stp_request, SAT_IO_SUCCESS);
	return;
}

s32 sat_start_stop_unit(struct stp_req * req,
			struct sat_ini_cmnd * scsi_cmd,
			struct sata_device * sata_dev)
{
	u8 *cdb = NULL;
	cdb = SAT_GET_SCSI_COMMAND(scsi_cmd);

	/* according to SAT protocol,POWER CONDITION must be 0 */
	if (0 != (0xf0 & cdb[4])) {
		SAT_TRACE(OSP_DBL_MINOR, 123,
			  "Disk 0x%llx invalid feild in CDB", sata_dev->dev_id);
		sat_set_sense_and_result(req, SCSI_SNSKEY_ILLEGAL_REQUEST,
					 SCSI_SNSCODE_INVALID_FIELD_IN_CDB);
		return SAT_IO_COMPLETE;
	}

	if (cdb[4] & SCSI_LOEJ_MASK) {
		SAT_TRACE(OSP_DBL_MINOR, 5, "Disk 0x%llx cann't be ejected",
			  sata_dev->dev_id);
		sat_set_sense_and_result(req,
					 SCSI_SNSKEY_ILLEGAL_REQUEST,
					 SCSI_SNSCODE_INVALID_FIELD_IN_CDB);
		return SAT_IO_COMPLETE;
	}

	memset(&req->h2d_fis, 0, sizeof(req->h2d_fis));
	req->h2d_fis.header.fis_type = 0x27;	/* Reg host to device */
	req->h2d_fis.header.pm_port = 0x80;	/* C Bit is set */
	/*Fill in start stop unit fis */
	if (cdb[4] & SCSI_START_MASK) {
		req->h2d_fis.header.command = FIS_COMMAND_READ_VERIFY_SECTOR;
		if (sata_dev->dev_cap & SAT_DEV_CAP_48BIT)
			req->h2d_fis.header.command =
			    FIS_COMMAND_READ_VERIFY_SECTOR_EXT;

		req->h2d_fis.data.lba_low = 1;
		req->h2d_fis.data.sector_cnt = 1;
		req->h2d_fis.data.device = 0x40;
	} else {
		req->h2d_fis.header.command = FIS_COMMAND_FLUSH_CACHE;
		if (sata_dev->dev_cap & SAT_DEV_CAP_48BIT)
			req->h2d_fis.header.command =
			    FIS_COMMAND_FLUSH_CACHE_EXT;

	}
	req->compl_call_back = sat_start_stop_unit_cb;
	return sat_send_req_to_dev(req);
}

s32 sat_sync_cache(struct stp_req * req,
		   struct sat_ini_cmnd * scsi_cmd,
		   struct sata_device * sata_dev)
{
	/*immediate bit is ignored */
	memset(&req->h2d_fis, 0, sizeof(req->h2d_fis));
	req->h2d_fis.header.fis_type = 0x27;	/* Reg host to device */
	req->h2d_fis.header.pm_port = 0x80;	/* C Bit is set */
	/*Fill in start stop unit fis */
	req->h2d_fis.header.command = FIS_COMMAND_FLUSH_CACHE;
	if (sata_dev->dev_cap & SAT_DEV_CAP_48BIT)
		req->h2d_fis.header.command = FIS_COMMAND_FLUSH_CACHE_EXT;

	req->compl_call_back = sat_common_cb;
	return sat_send_req_to_dev(req);
}

void sat_ata_pass_through_cb(struct stp_req *stp_request, u32 io_status)
{
	u8 *cdb = NULL;
	u16 sense_code = 0;
	struct dsc_fmt_sns_data *dsc = NULL;
	struct sata_d2h *d2h = NULL;

	d2h = &stp_request->d2h_fis;
	cdb = stp_request->scsi_cmnd.cdb;
	if ((cdb[2] & 0x20)
	    || (SAT_IO_SUCCESS != io_status)
	    || (SAT_D2H_VALID == stp_request->d2h_valid)
	    || (0x1e == (cdb[1] & 0x1e))) {
		dsc = &stp_request->sense->dsc_fmt;
		dsc->resp_code = 0x72;
		dsc->add_sns_len = 14;
		dsc->add_sns[0] = 0x09;
		dsc->add_sns[1] = 0x0c;
		dsc->add_sns[2] = 0x0;
		if (SCSIOPC_ATA_PASSTHROUGH_16 == cdb[0])
			dsc->add_sns[2] = cdb[1] & 0x1;

		if ((SAT_D2H_VALID == stp_request->d2h_valid)
		    && SAT_IS_IO_FAILED(stp_request->d2h_fis.header.status)) {
			SAT_TRACE(OSP_DBL_MAJOR, 178, "status 0x%x err 0x%x",
				  stp_request->d2h_fis.header.status,
				  stp_request->d2h_fis.header.err);

			sat_sata_err_to_sense(stp_request,
					      stp_request->d2h_fis.header.
					      status,
					      stp_request->d2h_fis.header.err,
					      &dsc->sns_key, &sense_code,
					      stp_request->scsi_cmnd.data_dir);

			dsc->asc = (u8) ((sense_code >> 8) & 0xFF);
			dsc->ascq = (u8) (sense_code & 0xFF);

		}
		dsc->add_sns[3] = d2h->header.err;
		dsc->add_sns[5] = d2h->data.sector_cnt;
		dsc->add_sns[7] = d2h->data.lba_low;
		dsc->add_sns[9] = d2h->data.lba_mid;
		dsc->add_sns[11] = d2h->data.lab_high;
		dsc->add_sns[12] = d2h->data.device;
		dsc->add_sns[13] = d2h->header.status;
		if (dsc->add_sns[2] & 0x01) {
			dsc->add_sns[4] = d2h->data.sector_cnt_exp;
			dsc->add_sns[6] = d2h->data.lba_low_exp;
			dsc->add_sns[8] = d2h->data.lba_mid_exp;
			dsc->add_sns[10] = d2h->data.lba_high_exp;
		}
	}
	/*IO success,return success to upper layer */
	sat_io_completed(stp_request, SAT_IO_SUCCESS);
	return;
}

s32 sat_ata_pass_through(struct stp_req * req,
			 struct sat_ini_cmnd * scsi_cmd,
			 struct sata_device * sata_dev)
{
	u8 *cdb = NULL;
	struct sata_h2d *tmp_fis = NULL;
	u32 multi_cnt = 0;

	cdb = SAT_GET_SCSI_COMMAND(scsi_cmd);
	/*protocol field is ignored */
	memset(&req->h2d_fis, 0, sizeof(req->h2d_fis));
	tmp_fis = &req->h2d_fis;
	tmp_fis->header.fis_type = 0x27;
	tmp_fis->header.pm_port = 0x80;	/*TODO:according to protocol,here need set C bit */
	if (SCSIOPC_ATA_PASSTHROUGH_16 == cdb[0]) {
		/*28bit ATA CMD */
		tmp_fis->header.command = cdb[14];	/* ATA CMD */
		tmp_fis->header.features = cdb[4];
		tmp_fis->data.device = cdb[13];
		tmp_fis->data.lba_low = cdb[8];	/* LBA low */
		tmp_fis->data.lba_mid = cdb[10];	/* LBA mid */
		tmp_fis->data.lab_high = cdb[12];	/* LBA high */
		tmp_fis->data.sector_cnt = cdb[6];	/*sector count */
		if (cdb[1] & 0x01) {
			/*48bit ATA CMD */
			tmp_fis->data.sector_cnt_exp = cdb[5];	/* LBA ext */
			tmp_fis->data.lba_low_exp = cdb[7];	/* LBA low */
			tmp_fis->data.lba_mid_exp = cdb[9];	/* LBA mid */
			tmp_fis->data.lba_high_exp = cdb[11];	/* LBA high */
			tmp_fis->data.features_exp = cdb[3];
		}
	} else {
		tmp_fis->header.command = cdb[9];	/* ATA CMD */
		tmp_fis->header.features = cdb[3];
		tmp_fis->data.device = cdb[8];
		tmp_fis->data.lba_low = cdb[5];	/* LBA low */
		tmp_fis->data.lba_mid = cdb[6];	/* LBA mid */
		tmp_fis->data.lab_high = cdb[7];	/* LBA high */
		tmp_fis->data.sector_cnt = cdb[4];	/*sector count */
	}

	if ((cdb[1] & 0xe0) && SAT_IS_MULTI_COMMAND(tmp_fis->header.command)) {
		sat_set_sense_and_result(req,
					 SCSI_SNSKEY_ILLEGAL_REQUEST,
					 SCSI_SNSCODE_INVALID_FIELD_IN_CDB);
		return SAT_IO_COMPLETE;
	}

	if (SAT_IS_MULTI_COMMAND(tmp_fis->header.command)) {
		multi_cnt = (sata_dev->sata_identify.word_54_59[5] & 0xff);
		if (multi_cnt != (1 << (cdb[1] >> 5))) {
			SAT_TRACE(OSP_DBL_MAJOR, 734,
				  "disk 0x%llx multicount %u,ingnored",
				  sata_dev->dev_id, multi_cnt);
		}
	}

	/*0x03:XFER  mode */
	if ((FIS_COMMAND_SET_FEATURES == tmp_fis->header.command)
	    && (0x03 == tmp_fis->header.features)) {
		sat_set_sense_and_result(req,
					 SCSI_SNSKEY_ILLEGAL_REQUEST,
					 SCSI_SNSCODE_INVALID_FIELD_IN_CDB);
		return SAT_IO_COMPLETE;
	}

	req->compl_call_back = sat_ata_pass_through_cb;
	return sat_send_req_to_dev(req);
}

void sat_log_sense_smart_return_status(struct stp_req *stp_request,
				       struct sat_ini_cmnd *ini_cmd)
{
	u8 log_page[11] = { 0 };
	u8 *cdb = NULL;
	u32 copy_len = 0;
	u32 need_len = 0;
	u32 buff_len = 0;
	s32 status = SAT_IO_SUCCESS;

	log_page[0] = 0x2F;	/* page code unspecified */
	log_page[1] = 0;	/* reserved */
	log_page[2] = 0;	/* page length */
	log_page[3] = 0x07;	/* page length */

	/*
	   SPC-4, 7.2.5, Table 211, p 255
	   no vendor specific field
	 */
	log_page[4] = 0;	/* Parameter Code */
	log_page[5] = 0;	/* Parameter Code unspecfied but to do: */
	log_page[6] = 0;	/* unspecified */
	log_page[7] = 0x03;	/* Parameter length, unspecified */

	/* SAT rev8, 10.2.3.1 Table 72, p 73 */
	if (stp_request->d2h_fis.data.lba_mid == 0x4F
	    || stp_request->d2h_fis.data.lab_high == 0xC2) {
		log_page[8] = 0;	/* Sense code */
		log_page[9] = 0;	/* Sense code qualifier */
	} else if (stp_request->d2h_fis.data.lba_mid == 0xF4
		   || stp_request->d2h_fis.data.lab_high == 0x2C) {
		log_page[8] = 0x5D;	/* Sense code */
		log_page[9] = 0x10;	/* Sense code qualifier */
	}

	/* Assumption: No support for SCT */
	log_page[10] = 0xFF;	/* Most Recent Temperature Reading */
	cdb = SAT_GET_SCSI_COMMAND(ini_cmd);
	buff_len = SAT_GET_SCSI_DATA_BUFF_LEN(ini_cmd);
	need_len = ((u32) cdb[7] << 8) + cdb[8];
	copy_len = MIN(buff_len, need_len);
	copy_len = MIN(copy_len, 11);
	status = SAT_IO_SUCCESS;

	if (copy_len !=
	    (u32) sal_sg_copy_from_buffer((struct sal_msg *)stp_request->
					  upper_msg, log_page, copy_len)) {
		SAT_TRACE(OSP_DBL_MAJOR, 734,
			  "disk 0x%llx copy buffer failed,length %u",
			  stp_request->dev->dev_id, copy_len);
		status = SAT_IO_FAILED;
	}

	sat_io_completed(stp_request, status);
	return;
}

void sat_fill_smart_read_log_page(u8 * log_page, u8 * sat_log_page)
{
	u8 self_test_exec_stat = 0;
	u32 i = 0;		/*log index in scsi cmd */
	u32 log_idx = 0;	/*index in ata log page */
	u32 desc_offset = 0;
	u32 tmp_idx = 24;
	log_idx = sat_log_page[508];
	/* SPC-4, 7.2.10, Table 216, 217, p 259 - 260 */
	log_page[0] = 0x10;	/* page code */
	log_page[1] = 0;
	log_page[2] = 0x01;	/* 0x190, page length */
	log_page[3] = 0x90;	/* 0x190, page length */

	i = 0;
	while (((log_idx + 21) != log_page[508])
	       && (i < 20)) {
		desc_offset = ((21 + log_idx - 1) % 21) * 24 + 2;	/*current descriptor */
		self_test_exec_stat = (log_page[desc_offset + 1] & 0xF0) >> 4;
		tmp_idx = 4 + i * 20;
		/* SPC-4, Table 217 */
		log_page[tmp_idx + 0] = 0;	/* Parameter Code */
		log_page[tmp_idx + 1] = (u8) (i + 1);	/* Parameter Code unspecfied but ... */
		log_page[tmp_idx + 2] = 3;	/* unspecified but ... */
		log_page[tmp_idx + 3] = 0x10;	/* Parameter Length */
		log_page[tmp_idx + 4] = 0 | ((log_page[desc_offset + 1] & 0xF0) >> 4);	/* Self Test Code and Self-Test Result */
		log_page[tmp_idx + 5] = 0;	/* self test number */
		log_page[tmp_idx + 6] = log_page[desc_offset + 3];	/* time stamp, MSB */
		log_page[tmp_idx + 7] = log_page[desc_offset + 2];	/* time stamp, LSB */

		log_page[tmp_idx + 8] = 0;	/* address of first failure MSB */
		log_page[tmp_idx + 9] = 0;	/* address of first failure */
		log_page[tmp_idx + 10] = 0;	 /* address of first failure */
		log_page[tmp_idx + 11] = 0;	 /* address of first failure */
		log_page[tmp_idx + 12] = log_page[desc_offset + 8];	/* address of first failure */
		log_page[tmp_idx + 13] = log_page[desc_offset + 7];	/* address of first failure */
		log_page[tmp_idx + 14] = log_page[desc_offset + 6];	/* address of first failure */
		log_page[tmp_idx + 15] = log_page[desc_offset + 5];	/* address of first failure LSB */

		/* SAT rev8 Table75, p 76 */
		switch (self_test_exec_stat) {
		case 0:
			log_page[tmp_idx + 16] = 0 | SCSI_SNSKEY_NO_SENSE;
			log_page[tmp_idx + 17] = 0;	
			log_page[tmp_idx + 18] =
			    SCSI_SNSCODE_NO_ADDITIONAL_INFO & 0xFF;
			break;
		case 1:	/*fall through */
		case 2:	/*fall through */
		case 3:
			log_page[tmp_idx + 16] =
			    0 | SCSI_SNSKEY_ABORTED_COMMAND;
			log_page[tmp_idx + 17] =
			    (SCSI_SNSCODE_DIAGNOSTIC_FAILURE_ON_COMPONENT_NN >>
			     8) & 0xFF;
			log_page[tmp_idx + 18] = 0x80 + self_test_exec_stat;
			break;
		case 4:	/*fall through */
		case 5:	/*fall through */
		case 6:
			log_page[tmp_idx + 16] = 0 | SCSI_SNSKEY_HARDWARE_ERROR;
			log_page[tmp_idx + 17] =
			    (SCSI_SNSCODE_DIAGNOSTIC_FAILURE_ON_COMPONENT_NN >>
			     8) & 0xFF;
			log_page[tmp_idx + 18] = 0x80 + self_test_exec_stat;
			break;
		case 7:
			log_page[tmp_idx + 16] = 0 | SCSI_SNSKEY_MEDIUM_ERROR;
			log_page[tmp_idx + 17] =
			    (SCSI_SNSCODE_DIAGNOSTIC_FAILURE_ON_COMPONENT_NN >>
			     8) & 0xFF;
			log_page[tmp_idx + 18] = 0x87;
			break;
		case 8:
			log_page[tmp_idx + 16] = 0 | SCSI_SNSKEY_HARDWARE_ERROR;
			log_page[tmp_idx + 17] =
			    (SCSI_SNSCODE_DIAGNOSTIC_FAILURE_ON_COMPONENT_NN >>
			     8) & 0xFF;
			log_page[tmp_idx + 18] = 0x88;
			break;
		case 9:	/* fall through */
		case 10:	/* fall through */
		case 11:	/* fall through */
		case 12:	/* fall through */
		case 13:	/* fall through */
		case 14:	/* fall through */
		case 15:
			log_page[tmp_idx + 16] = 0 | SCSI_SNSKEY_NO_SENSE;
			log_page[tmp_idx + 17] = 0;	
			log_page[tmp_idx + 18] =
			    SCSI_SNSCODE_NO_ADDITIONAL_INFO & 0xFF;
			break;
		default:
			SAL_TRACE(OSP_DBL_DATA, 970, "Impossible case");
			break;	
		}

		i++;
		log_idx--;
	}
}

void sat_log_sense_smart_read_log(struct stp_req *stp_request,
				  struct sat_ini_cmnd *ini_cmd)
{
	u8 *sat_log_info = NULL;
	u8 *log_buff = NULL;
	u8 *cdb = NULL;
	u32 copy_len = 0;
	u32 need_len = 0;
	u32 buff_len = 0;
	u32 log_idx = 0;
	s32 status = SAT_IO_SUCCESS;
	struct stp_req *org_stp_req = NULL;

	log_buff = SAS_KMALLOC(ATA_READ_LOG_LEN, GFP_ATOMIC);
	if (NULL == log_buff) {
		SAT_TRACE(OSP_DBL_MAJOR, 734, "disk 0x%llx alloc memory failed",
			  stp_request->dev->dev_id);
		org_stp_req = stp_request->org_msg;
		sat_free_int_msg(stp_request);
		sat_io_completed(org_stp_req, SAT_IO_FAILED);
		return;
	}

	memset(log_buff, 0, ATA_READ_LOG_LEN);

	sat_log_info = sat_get_req_buf(stp_request);
	SAT_ASSERT(NULL != sat_log_info, return);

	cdb = SAT_GET_SCSI_COMMAND(ini_cmd);
	buff_len = SAT_GET_SCSI_DATA_BUFF_LEN(ini_cmd);
	need_len = ((u32) cdb[7] << 8) + cdb[8];
	log_idx = sat_log_info[508];
	if (log_idx > ATA_MAX_SMART_READ_LOG_INDEX) {
		SAT_TRACE(OSP_DBL_MAJOR, 734,
			  "disk 0x%llx illegal max log index %u",
			  stp_request->dev->dev_id, log_idx);
		org_stp_req = stp_request->org_msg;
		sat_free_int_msg(stp_request);
		sat_io_completed(org_stp_req, SAT_IO_FAILED);

		SAS_KFREE(log_buff);
		return;
	}
	sat_fill_smart_read_log_page(log_buff, sat_log_info);
	copy_len = MIN(buff_len, need_len);
	copy_len = MIN(copy_len, ATA_READ_LOG_LEN);
	status = SAT_IO_SUCCESS;
	if (copy_len !=
	    (u32) sal_sg_copy_from_buffer((struct sal_msg *)stp_request->
					  upper_msg, log_buff, copy_len)) {
		SAT_TRACE(OSP_DBL_MAJOR, 734,
			  "disk 0x%llx copy buffer failed,length %u",
			  stp_request->dev->dev_id, copy_len);
		status = SAT_IO_FAILED;
	}
	org_stp_req = stp_request->org_msg;
	sat_free_int_msg(stp_request);
	sat_io_completed(org_stp_req, status);

	SAS_KFREE(log_buff);
	return;
}

void sat_fill_read_log_ext_page(u8 * log_page, u8 * sat_log_page)
{
	u8 self_test_exec_stat = 0;
	u32 i = 0;
	s32 log_idx = 0;
	u32 desc_offset = 0;
	u32 tmp_idx = 0;
	/* SPC-4, 7.2.10, Table 216, 217, p 259 - 260 */
	log_page[0] = 0x10;	/* page code */
	log_page[1] = 0;
	log_page[2] = 0x01;	/* 0x190, page length */
	log_page[3] = 0x90;	/* 0x190, page length */

	i = 0;
	while (((log_idx + 19) != ((sat_log_page[3] << 8) + sat_log_page[2]))
	       && (i < 20)) {
		desc_offset = ((19 + (u32) log_idx - 1) % 19) * 26 + 4;	/*current descriptor */
		self_test_exec_stat =
		    (sat_log_page[desc_offset + 1] & 0xF0) >> 4;
		tmp_idx = 4 + i * 20;
		/* SPC-4, Table 217 */
		log_page[tmp_idx + 0] = 0;	/* Parameter Code */
		log_page[tmp_idx + 1] = (u8) (i + 1);	/* Parameter Code unspecfied but ... */
		log_page[tmp_idx + 2] = 3;	/* unspecified but ... */
		log_page[tmp_idx + 3] = 0x10;	/* Parameter Length */
		log_page[tmp_idx + 4] = 0 | ((sat_log_page[desc_offset + 1] & 0xF0) >> 4);	/* Self Test Code and Self-Test Result */
		log_page[tmp_idx + 5] = 0;	/* self test number */
		log_page[tmp_idx + 6] = sat_log_page[desc_offset + 3];	/* time stamp, MSB */
		log_page[tmp_idx + 7] = sat_log_page[desc_offset + 2];	/* time stamp, LSB */

		log_page[tmp_idx + 8] = 0;	/* address of first failure MSB */
		log_page[tmp_idx + 9] = 0;	/* address of first failure */
		log_page[tmp_idx + 10] = sat_log_page[desc_offset + 10];	/* address of first failure */
		log_page[tmp_idx + 11] = sat_log_page[desc_offset + 9];	/* address of first failure */
		log_page[tmp_idx + 12] = sat_log_page[desc_offset + 8];	/* address of first failure */
		log_page[tmp_idx + 13] = sat_log_page[desc_offset + 7];	/* address of first failure */
		log_page[tmp_idx + 14] = sat_log_page[desc_offset + 6];	/* address of first failure */
		log_page[tmp_idx + 15] = sat_log_page[desc_offset + 5];	/* address of first failure LSB */

		/* SAT rev8 Table75, p 76 */
		switch (self_test_exec_stat) {
		case 0:
			log_page[tmp_idx + 16] = 0 | SCSI_SNSKEY_NO_SENSE;
			log_page[tmp_idx + 17] = 0;	
			log_page[tmp_idx + 18] =
			    SCSI_SNSCODE_NO_ADDITIONAL_INFO & 0xFF;
			break;
		case 1:	/*fall through */
		case 2:	/*fall through */
		case 3:
			log_page[tmp_idx + 16] =
			    0 | SCSI_SNSKEY_ABORTED_COMMAND;
			log_page[tmp_idx + 17] =
			    (SCSI_SNSCODE_DIAGNOSTIC_FAILURE_ON_COMPONENT_NN >>
			     8) & 0xFF;
			log_page[tmp_idx + 18] = 0x80 + self_test_exec_stat;
			break;
		case 4:	/*fall through */
		case 5:	/*fall through */
		case 6:
			log_page[tmp_idx + 16] = 0 | SCSI_SNSKEY_HARDWARE_ERROR;
			log_page[tmp_idx + 17] =
			    (SCSI_SNSCODE_DIAGNOSTIC_FAILURE_ON_COMPONENT_NN >>
			     8) & 0xFF;
			log_page[tmp_idx + 18] = 0x80 + self_test_exec_stat;
			break;
		case 7:
			log_page[tmp_idx + 16] = 0 | SCSI_SNSKEY_MEDIUM_ERROR;
			log_page[tmp_idx + 17] =
			    (SCSI_SNSCODE_DIAGNOSTIC_FAILURE_ON_COMPONENT_NN >>
			     8) & 0xFF;
			log_page[tmp_idx + 18] = 0x87;
			break;
		case 8:
			log_page[tmp_idx + 16] = 0 | SCSI_SNSKEY_HARDWARE_ERROR;
			log_page[tmp_idx + 17] =
			    (SCSI_SNSCODE_DIAGNOSTIC_FAILURE_ON_COMPONENT_NN >>
			     8) & 0xFF;
			log_page[tmp_idx + 18] = 0x88;
			break;
		case 9:	/* fall through */
		case 10:	/* fall through */
		case 11:	/* fall through */
		case 12:	/* fall through */
		case 13:	/* fall through */
		case 14:	/* fall through */
		case 15:
			log_page[tmp_idx + 16] = 0 | SCSI_SNSKEY_NO_SENSE;
			log_page[tmp_idx + 17] = 0;	
			log_page[tmp_idx + 18] =
			    SCSI_SNSCODE_NO_ADDITIONAL_INFO & 0xFF;
			break;
		default:
			SAL_TRACE(OSP_DBL_MAJOR, 971, "Impossible case");
			break;	
		}
		i++;
		log_idx--;
	}
}

void sat_log_sense_read_log_ext(struct stp_req *stp_request,
				struct sat_ini_cmnd *ini_cmd)
{
	u8 *sat_log_info = NULL;
	u8 *log_buff = NULL;
	u8 *cdb = NULL;
	u32 copy_len = 0;
	u32 need_len = 0;
	u32 buff_len = 0;
	u32 log_idx = 0;
	s32 status = SAT_IO_SUCCESS;
	struct stp_req *org_stp_req = NULL;

	log_buff = SAS_KMALLOC(ATA_READ_LOG_LEN, GFP_ATOMIC);
	if (NULL == log_buff) {
		SAT_TRACE(OSP_DBL_MAJOR, 734, "disk 0x%llx alloc memory failed",
			  stp_request->dev->dev_id);
		org_stp_req = stp_request->org_msg;
		sat_free_int_msg(stp_request);
		sat_io_completed(org_stp_req, SAT_IO_FAILED);

		return;
	}

	memset(log_buff, 0, ATA_READ_LOG_LEN);

	sat_log_info = sat_get_req_buf(stp_request);
	SAT_ASSERT(NULL != sat_log_info, return);

	cdb = SAT_GET_SCSI_COMMAND(ini_cmd);
	buff_len = SAT_GET_SCSI_DATA_BUFF_LEN(ini_cmd);
	need_len = ((u32) cdb[7] << 8) + cdb[8];
	log_idx = ((u32) sat_log_info[3] << 8) + sat_log_info[2];
	if (log_idx > ATA_MAX_LOG_INDEX) {
		SAT_TRACE(OSP_DBL_MAJOR, 734,
			  "disk 0x%llx illegal max log index %u",
			  stp_request->dev->dev_id, log_idx);
		org_stp_req = stp_request->org_msg;
		sat_free_int_msg(stp_request);
		sat_io_completed(org_stp_req, SAT_IO_FAILED);

		SAS_KFREE(log_buff);
		return;
	}
	sat_fill_read_log_ext_page(log_buff, sat_log_info);
	copy_len = MIN(buff_len, need_len);
	copy_len = MIN(copy_len, ATA_READ_LOG_LEN);
	status = SAT_IO_SUCCESS;
	if (copy_len !=
	    (u32) sal_sg_copy_from_buffer((struct sal_msg *)stp_request->
					  upper_msg, log_buff, copy_len)) {
		SAT_TRACE(OSP_DBL_MAJOR, 734,
			  "disk 0x%llx copy buffer failed,length %u",
			  stp_request->dev->dev_id, copy_len);
		status = SAT_IO_FAILED;
	}

	org_stp_req = stp_request->org_msg;
	sat_free_int_msg(stp_request);
	sat_io_completed(org_stp_req, status);

	SAS_KFREE(log_buff);
	return;
}

void sat_log_sense_cb(struct stp_req *stp_request, u32 io_status)
{
	struct stp_req *org_stp_req = NULL;

	switch (stp_request->h2d_fis.header.command) {
	case FIS_COMMAND_READ_LOG_EXT:
		sat_log_sense_read_log_ext(stp_request,
					   &stp_request->org_msg->scsi_cmnd);
		return;
	case FIS_COMMAND_SMART:
		if (0xd5 == stp_request->h2d_fis.header.features) {
			sat_log_sense_smart_read_log(stp_request,
						     &stp_request->org_msg->
						     scsi_cmnd);
			return;
		} else if (0xda == stp_request->h2d_fis.header.features) {
			sat_log_sense_smart_return_status(stp_request,
							  &stp_request->
							  scsi_cmnd);
			return;
		} else {
			break;
		}
	default:
		break;
	}
	org_stp_req = stp_request->org_msg;
	sat_free_int_msg(stp_request);
	sat_io_completed(org_stp_req, SAT_IO_FAILED);

	return;
}

s32 sat_support_log_page(struct stp_req * req,
			 struct sat_ini_cmnd * scsi_cmd,
			 struct sata_device * sata_dev)
{
	u8 *cdb = NULL;
	u8 log_page[7] = { 0 };
	u32 copy_len = 0;
	u32 alloc_len = 0;
	struct sal_msg *sal_msg = NULL;

	sal_msg = (struct sal_msg *)req->upper_msg;

	cdb = SAT_GET_SCSI_COMMAND(scsi_cmd);
	alloc_len = (cdb[7] << 8) + cdb[8];
	log_page[0] = 0;	/* page code */
	log_page[1] = 0;	/* reserved  */
	log_page[2] = 0;
	log_page[3] = 1;
	log_page[4] = 0;

	if (SAT_IS_SMART_FEATURESET_SUPPORT(&sata_dev->sata_identify)) {
		log_page[3] = 2;
		log_page[5] = 0x10;
		if (SAT_IS_SMART_SELFTEST_SUPPORT(&sata_dev->sata_identify)) {
			log_page[3] = 3;
			log_page[5] = 0x10;
			log_page[6] = 0x2F;
		}
	}
	copy_len = MIN(SAT_GET_SCSI_DATA_BUFF_LEN(scsi_cmd), 7);
	copy_len = MIN(alloc_len, copy_len);
	if (copy_len !=
	    (u32) sal_sg_copy_from_buffer(sal_msg, log_page, copy_len)) {
		SAT_TRACE(OSP_DBL_MAJOR, 734, "disk 0x%llx copy data failed",
			  sata_dev->dev_id);
		return SAT_IO_FAILED;
	}
	if (SAT_GET_SCSI_DATA_BUFF_LEN(scsi_cmd) != copy_len) {
		SAT_TRACE(OSP_DBL_MINOR, 5,
			  "INI Cmnd data len:%d not equal to %d",
			  SAT_GET_SCSI_DATA_BUFF_LEN(scsi_cmd), copy_len);
	}

	sal_msg->status.io_resp = SAM_STAT_GOOD;
	sal_msg->status.drv_resp = SAL_MSG_DRV_SUCCESS;

	return SAT_IO_COMPLETE;

}

s32 sat_log_sense(struct stp_req * req,
		  struct sat_ini_cmnd * scsi_cmd, struct sata_device * sata_dev)
{
	u8 *cdb = NULL;
	u8 sense_key = SCSI_SNSKEY_NO_SENSE;
	u16 sense_code = SCSI_SNSCODE_NO_ADDITIONAL_INFO;
	struct stp_req *new_req = NULL;
	s32 ret = 0;

	cdb = SAT_GET_SCSI_COMMAND(scsi_cmd);
	switch (cdb[2] & SCSI_LOG_SENSE_PAGE_CODE_MASK) {
	case SCSI_LOGSENSE_SUPPORTED_LOG_PAGES:
		return sat_support_log_page(req, scsi_cmd, sata_dev);
	case SCSI_LOGSENSE_SELFTEST_RESULTS_PAGE:
		if (!SAT_IS_SMART_SELFTEST_SUPPORT(&sata_dev->sata_identify)) {
			sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
			sense_code = SCSI_SNSCODE_INVALID_FIELD_IN_CDB;
			break;	/*failure */
		}
		/*Need to send identify first */
		new_req = sat_get_new_msg(sata_dev, ATA_READ_LOG_LEN);
		if (NULL == new_req) {
			SAT_TRACE(OSP_DBL_MINOR, 5,
				  "Alloc resource for device 0x%llx failed"
				  "command 0x%x", sata_dev->dev_id,
				  SAT_GET_SCSI_COMMAND(scsi_cmd)[0]);
			return SAT_IO_FAILED;
		}
		sat_init_internal_command(new_req, req, sata_dev);
		memset(&new_req->h2d_fis, 0, sizeof(struct sata_h2d));
		new_req->h2d_fis.header.fis_type = SATA_FIS_TYPE_H2D;
		new_req->h2d_fis.header.pm_port = 0x80;
		new_req->h2d_fis.data.sector_cnt = 0x01;
		if (sata_dev->dev_cap & SAT_DEV_CAP_48BIT) {
			new_req->h2d_fis.header.command =
			    FIS_COMMAND_READ_LOG_EXT;
			new_req->h2d_fis.data.lba_low = 0x07;
		} else {
			new_req->h2d_fis.header.command = FIS_COMMAND_SMART;
			new_req->h2d_fis.header.features = 0xD5;
			new_req->h2d_fis.data.lba_low = 0x06;
			new_req->h2d_fis.data.lba_mid = 0x4F;
			new_req->h2d_fis.data.lab_high = 0xC2;
			new_req->h2d_fis.data.device = 0xA0;
		}
		new_req->compl_call_back = sat_log_sense_cb;

		ret = sat_send_req_to_dev(new_req);
		if (SAT_IO_SUCCESS != ret) {
			sat_free_int_msg(new_req);
			return ret;
		}
		return SAT_IO_SUCCESS;
	case SCSI_LOGSENSE_INFORMATION_EXCEPTIONS_PAGE:
		if (!SAT_IS_SMART_SELFTEST_SUPPORT(&sata_dev->sata_identify)) {
			sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
			sense_code = SCSI_SNSCODE_INVALID_FIELD_IN_CDB;
			break;	/* failure */
		}

		if (!SAT_IS_SMART_FEATURESET_ENABLED(&sata_dev->sata_identify)) {
			sense_key = SCSI_SNSKEY_ABORTED_COMMAND;
			sense_code =
			    SCSI_SNSCODE_ATA_DEVICE_FEATURE_NOT_ENABLED;
			break;	/*failure */
		}
		memset(&req->h2d_fis, 0, sizeof(struct sata_h2d));
		req->h2d_fis.header.fis_type = SATA_FIS_TYPE_H2D;
		req->h2d_fis.header.pm_port = 0x80;
		req->h2d_fis.header.command = FIS_COMMAND_SMART;
		req->h2d_fis.header.features = 0xDA;
		req->h2d_fis.data.lba_mid = 0x4F;
		req->h2d_fis.data.lab_high = 0xC2;
		req->compl_call_back = sat_log_sense_cb;
		return sat_send_req_to_dev(req);
	default:
		sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
		sense_code = SCSI_SNSCODE_INVALID_FIELD_IN_CDB;
		break;

	}

	sat_set_sense_and_result(req, sense_key, sense_code);
	return SAT_IO_COMPLETE;
}

void sat_fill_verify_fis(struct stp_req *req,
			 u8 * cdb, struct sata_device *sata_dev)
{
	struct sata_h2d *tmp_fis = NULL;
	tmp_fis = &req->h2d_fis;

	tmp_fis->header.fis_type = 0x27;
	tmp_fis->header.pm_port = 0x80;
	tmp_fis->header.features = 0;
	tmp_fis->data.features_exp = 0;
	tmp_fis->data.reserved_4 = 0;
	tmp_fis->data.control = 0;
	tmp_fis->data.reserved_5 = 0;

	if (sata_dev->dev_cap & SAT_DEV_CAP_48BIT) {
		tmp_fis->header.command = FIS_COMMAND_READ_VERIFY_SECTOR_EXT;
		tmp_fis->data.device = 0x40;
		switch (cdb[0]) {
		case SCSIOPC_VERIFY_10:
			tmp_fis->data.lba_low = cdb[5];	/* FIS LBA (7 :0 ) */
			tmp_fis->data.lba_mid = cdb[4];	/* FIS LBA (15:8 ) */
			tmp_fis->data.lab_high = cdb[3];	/* FIS LBA (23:16) */
			tmp_fis->data.lba_low_exp = cdb[2];	/* FIS LBA (31:24) */
			tmp_fis->data.lba_mid_exp = 0;	/* FIS LBA (39:32) */
			tmp_fis->data.lba_high_exp = 0;	/* FIS LBA (47:40) */
			tmp_fis->data.sector_cnt = cdb[8];	/* FIS sector count (7:0) */
			tmp_fis->data.sector_cnt_exp = cdb[7];	/* FIS sector count (15:8) */
			break;
		case SCSIOPC_VERIFY_12:
			tmp_fis->data.lba_low = cdb[5];	/* FIS LBA (7 :0 ) */
			tmp_fis->data.lba_mid = cdb[4];	/* FIS LBA (15:8 ) */
			tmp_fis->data.lab_high = cdb[3];	/* FIS LBA (23:16) */
			tmp_fis->data.lba_low_exp = cdb[2];	/* FIS LBA (31:24) */
			tmp_fis->data.lba_mid_exp = 0;	/* FIS LBA (39:32) */
			tmp_fis->data.lba_high_exp = 0;	/* FIS LBA (47:40) */
			tmp_fis->data.sector_cnt = cdb[9];	/* FIS sector count (7:0) */
			tmp_fis->data.sector_cnt_exp = cdb[8];	/* FIS sector count (15:8) */
			break;
		default:
			tmp_fis->data.lba_low = cdb[9];	/* FIS LBA (7 :0 ) */
			tmp_fis->data.lba_mid = cdb[8];	/* FIS LBA (15:8 ) */
			tmp_fis->data.lab_high = cdb[7];	/* FIS LBA (23:16) */
			tmp_fis->data.lba_low_exp = cdb[6];	/* FIS LBA (31:24) */
			tmp_fis->data.lba_mid_exp = cdb[5];	/* FIS LBA (39:32) */
			tmp_fis->data.lba_high_exp = cdb[4];	/* FIS LBA (47:40) */
			tmp_fis->data.sector_cnt = cdb[13];	/* FIS sector count (7:0) */
			tmp_fis->data.sector_cnt_exp = cdb[12];	/* FIS sector count (15:8) */
			break;
		}
	} else {
		tmp_fis->header.command = FIS_COMMAND_READ_VERIFY_SECTOR;
		tmp_fis->data.sector_cnt_exp = 0;
		tmp_fis->data.lba_high_exp = 0;
		tmp_fis->data.lba_mid_exp = 0;
		tmp_fis->data.lba_low_exp = 0;

		switch (cdb[0]) {
		case SCSIOPC_VERIFY_10:
			tmp_fis->data.lba_low = cdb[5];	/* FIS LBA (7 :0 ) */
			tmp_fis->data.lba_mid = cdb[4];	/* FIS LBA (15:8 ) */
			tmp_fis->data.lab_high = cdb[3];	/* FIS LBA (23:16) */
			tmp_fis->data.device = (0x04 << 4) | (cdb[2] & 0x0f);
			tmp_fis->data.sector_cnt = cdb[8];
			break;
		case SCSIOPC_VERIFY_12:
			tmp_fis->data.lba_low = cdb[5];	/* FIS LBA (7 :0 ) */
			tmp_fis->data.lba_mid = cdb[4];	/* FIS LBA (15:8 ) */
			tmp_fis->data.lab_high = cdb[3];	/* FIS LBA (23:16) */
			tmp_fis->data.device = (0x04 << 4) | (cdb[2] & 0x0f);
			tmp_fis->data.sector_cnt = cdb[9];
			break;
		default:	
			tmp_fis->data.lba_low = cdb[9];	/* FIS LBA (7 :0 ) */
			tmp_fis->data.lba_mid = cdb[8];	/* FIS LBA (15:8 ) */
			tmp_fis->data.lab_high = cdb[7];	/* FIS LBA (23:16) */
			tmp_fis->data.device = (0x04 << 4) | (cdb[6] & 0x0f);
			tmp_fis->data.sector_cnt = cdb[13];
			break;
		}
	}
}

void sat_verify_cb(struct stp_req *stp_request, u32 io_status)
{
	struct stp_req *org_req = NULL;
	struct stp_req *new_req = NULL;
	u32 max_len_per_time = 0;
	u32 req_last_lba = 0;
	u32 now_last_lba = 0;
	u32 next_begin_lba = 0;
	u32 remain_len = 0;
	u8 cdb[16] = { 0 };
	s32 ret = SAT_IO_FAILED;

	new_req = stp_request;
	org_req = new_req->org_msg;
	if (new_req == org_req) {
		sat_io_completed(org_req, SAT_IO_SUCCESS);
		return;
	}

	max_len_per_time = ATA_MAX_VERIFY_LEN(new_req->h2d_fis.header.command);
	req_last_lba = org_req->curr_lba + org_req->org_transfer_len - 1;
	now_last_lba = new_req->curr_lba + new_req->org_transfer_len - 1;

	if (now_last_lba >= req_last_lba) {
		sat_free_int_msg(new_req);
		sat_io_completed(org_req, SAT_IO_SUCCESS);
		return;
	}
	remain_len = req_last_lba - now_last_lba;
	next_begin_lba = now_last_lba + 1;
	memcpy(cdb, org_req->scsi_cmnd.cdb, 16);
	if (remain_len < max_len_per_time) {
		new_req->org_transfer_len = remain_len;
	} else {
		new_req->org_transfer_len = max_len_per_time;
		remain_len = 0;
	}
	new_req->curr_lba = next_begin_lba;

	switch (cdb[0]) {
	case SCSIOPC_VERIFY_10:
		cdb[2] = (u8) ((next_begin_lba & 0xFF000000) >> 24);
		cdb[3] = (u8) ((next_begin_lba & 0x00FF0000) >> 16);
		cdb[4] = (u8) ((next_begin_lba & 0x0000FF00) >> 8);
		cdb[5] = (u8) (next_begin_lba & 0x000000FF);
		cdb[7] = (u8) ((remain_len & 0x0000FF00) >> 8);
		cdb[8] = (u8) (remain_len & 0x000000FF);
		break;
	case SCSIOPC_VERIFY_12:
		cdb[2] = (u8) ((next_begin_lba & 0xFF000000) >> 24);
		cdb[3] = (u8) ((next_begin_lba & 0x00FF0000) >> 16);
		cdb[4] = (u8) ((next_begin_lba & 0x0000FF00) >> 8);
		cdb[5] = (u8) (next_begin_lba & 0x000000FF);
		cdb[6] = (u8) ((remain_len & 0xFF000000) >> 24);
		cdb[7] = (u8) ((remain_len & 0x00FF0000) >> 16);
		cdb[8] = (u8) ((remain_len & 0x0000FF00) >> 8);
		cdb[9] = (u8) (remain_len & 0x000000FF);
		break;
	default:
		cdb[6] = (u8) ((next_begin_lba & 0xFF000000) >> 24);
		cdb[7] = (u8) ((next_begin_lba & 0x00FF0000) >> 16);
		cdb[8] = (u8) ((next_begin_lba & 0x0000FF00) >> 8);
		cdb[9] = (u8) (next_begin_lba & 0x000000FF);
		cdb[10] = (u8) ((remain_len & 0xFF000000) >> 24);
		cdb[11] = (u8) ((remain_len & 0x00FF0000) >> 16);
		cdb[12] = (u8) ((remain_len & 0x0000FF00) >> 8);
		cdb[13] = (u8) (remain_len & 0x000000FF);
		break;
	}

	sat_fill_verify_fis(new_req, cdb, new_req->dev);

	ret = sat_send_req_to_dev(new_req);
	if (SAT_IO_SUCCESS != ret) {
		sat_free_int_msg(new_req);
		sat_io_completed(org_req, ret);
	}
	return;
}

/**
 * sat_verify - translate VERIFY(10) VERIFY(12) VERIFY(16) command from SCSI to ATA,
 * and Send to Device.
 * 
 * @req: sata request info
 * @scsi_cmd: sata request related scsi cmd
 * @sata_dev: sata device related
 *
 */
s32 sat_verify(struct stp_req * req,
	       struct sat_ini_cmnd * scsi_cmd, struct sata_device * sata_dev)
{
	u8 *cdb = NULL;
	u32 max_len_per_time = 0;
	struct stp_req *new_req = NULL;
	s32 ret = SAT_IO_FAILED;

	cdb = SAT_GET_SCSI_COMMAND(scsi_cmd);
	if (cdb[1] & SCSI_VERIFY_BYTCHK_MASK) {
		SAT_TRACE(OSP_DBL_MINOR, 5,
			  "Disk 0x%llx check CDB failed:BYTCHK",
			  req->dev->dev_id);
		sat_set_sense_and_result(req, SCSI_SNSKEY_ILLEGAL_REQUEST,
					 SCSI_SNSCODE_INVALID_FIELD_IN_CDB);
		return SAT_IO_COMPLETE;
	} else if (!(sata_dev->dev_cap & SAT_DEV_CAP_48BIT) &&
		   true == sat_check_28bit_lba(cdb)) {
		SAT_TRACE(OSP_DBL_MINOR, 5,
			  "Disk 0x%llx abnormal verify,invalid LBA or transfer length",
			  req->dev->dev_id);
		sat_set_sense_and_result(req,
					 SCSI_SNSKEY_ILLEGAL_REQUEST,
					 SCSI_SNSCODE_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE);
		return SAT_IO_COMPLETE;
	}

	req->curr_lba = sat_calc_lba(cdb);
	req->org_transfer_len = sat_calc_trans_len(cdb);

	if (0 == req->org_transfer_len) {
		SAT_TRACE(OSP_DBL_DATA, 1000, "Disk 0x%llx, verify length is 0",
			  sata_dev->dev_id);
		/* TODO: return what? */
		sat_set_sense_and_result(req,
					 SCSI_SNSKEY_ILLEGAL_REQUEST,
					 SCSI_SNSCODE_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE);
		return SAT_IO_COMPLETE;
	}

	sat_fill_verify_fis(req, cdb, sata_dev);

	req->ata_cmd = req->h2d_fis.header.command;
	max_len_per_time = ATA_MAX_VERIFY_LEN(req->h2d_fis.header.command);

	if (req->org_transfer_len > max_len_per_time) {
		new_req = sat_get_new_msg(sata_dev, 0);
		if (NULL == new_req) {
			SAT_TRACE(OSP_DBL_MINOR, 5,
				  "Alloc resource for device 0x%llx failed"
				  "command 0x%x", sata_dev->dev_id,
				  SAT_GET_SCSI_COMMAND(scsi_cmd)[0]);
			return SAT_IO_FAILED;
		}
		sat_init_internal_command(new_req, req, sata_dev);
		memcpy(&new_req->h2d_fis, &req->h2d_fis,
		       sizeof(struct sata_h2d));
		/*
		   According to ATA protocol,for READ VERIFY SECTOR(S) EXT,Count setted to 0000h
		   indicates 65536 sectors are to be verified.for READ VERIFY SECTOR(S),Count setted
		   to 00h indicates 256 sectors are to be verified.
		 */
		new_req->h2d_fis.data.sector_cnt = 0x00;
		new_req->h2d_fis.data.sector_cnt_exp = 0x00;
		new_req->compl_call_back = sat_verify_cb;
		new_req->curr_lba = req->curr_lba;
		new_req->org_transfer_len = max_len_per_time;

		ret = sat_send_req_to_dev(new_req);
		if (SAT_IO_SUCCESS != ret)
			sat_free_int_msg(new_req);

		return ret;
	} else {
		if (req->org_transfer_len == max_len_per_time) {
			req->h2d_fis.data.sector_cnt = 0x00;
			req->h2d_fis.data.sector_cnt_exp = 0x00;
		}
		req->compl_call_back = sat_verify_cb;
		return sat_send_req_to_dev(req);
	}

}

void sat_mode_select_cb(struct stp_req *stp_request, u32 io_status)
{
	struct stp_req *org_req = NULL;
	struct stp_req *sub_req = NULL;
	struct sata_h2d *org_fis = NULL;
	struct sata_h2d *sub_fis = NULL;
	s32 ret = SAT_IO_FAILED;

	sub_req = stp_request;
	org_req = sub_req->org_msg;
	org_fis = &org_req->h2d_fis;
	sub_fis = &sub_req->h2d_fis;

	/* no sub-IO */
	if (sub_req == org_req) {
		sat_io_completed(org_req, SAT_IO_SUCCESS);
		return;
	}

	/* if this sub-IO is enable/disable read look-ahead, then mode select complete */
	if ((sub_fis->header.features == 0xAA) ||
	    (sub_fis->header.features == 0x55)) {
		sat_free_int_msg(sub_req);
		sat_io_completed(org_req, SAT_IO_SUCCESS);
		return;
	}

	/* if need two sub-IO, the orignal fis has the second fis */
	memcpy(sub_fis, org_fis, sizeof(struct sata_h2d));
	ret = sat_send_req_to_dev(sub_req);
	if (SAT_IO_SUCCESS != ret) {
		sat_free_int_msg(sub_req);
		sat_io_completed(org_req, ret);
	}

}

s32 sat_ms_check_cdb(u8 * cdb, u32 * pllen, u8 * sense_key, u16 * sense_code)
{
	*pllen = 0;
	*sense_key = SCSI_SNSKEY_NO_SENSE;
	*sense_code = SCSI_SNSCODE_NO_ADDITIONAL_INFO;

	if (!(cdb[1] & SCSI_MODE_SELECT6_PF_MASK)) {
		*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
		*sense_code = SCSI_SNSCODE_INVALID_FIELD_IN_CDB;
		return ERROR;
	}

	switch (cdb[0]) {
	case SCSIOPC_MODE_SELECT_6:
		*pllen = cdb[4];
		break;
	case SCSIOPC_MODE_SELECT_10:
		*pllen = (cdb[7] << 8) + cdb[8];
		break;
	default:
		*pllen = 0;
		break;
	}
	return OK;
}

s32 sat_ms_header_and_desp(u8 scsi_opc,
			   struct sata_device * sata_dev,
			   u8 * param,
			   u32 len,
			   u8 * sense_key,
			   u16 * sense_code,
			   u32 * ret_desp_len, u32 * page_offset)
{
	u32 header_len = 0;
	u32 exp_desp_len = 0;	
	u32 exp_page_len = 0;	
	u32 desp_len = 0;
	u32 logic_block_len = 0;
	u8 long_lba = 0;

	*ret_desp_len = 0;
	*sense_key = SCSI_SNSKEY_NO_SENSE;
	*sense_code = SCSI_SNSCODE_NO_ADDITIONAL_INFO;
	*page_offset = 0;

	SAT_CHECK_AND_SET(SCSIOPC_MODE_SELECT_6 == scsi_opc, header_len, 4, 8);

	if (len < header_len) {
		*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
		*sense_code = SCSI_SNSCODE_PARAMETER_LIST_LENGTH_ERROR;
		SAT_TRACE(OSP_DBL_MINOR, 123,
			  "parameter list length %d shorter than expected header length %d",
			  len, header_len);
		return ERROR;
	}

	switch (scsi_opc) {
	case SCSIOPC_MODE_SELECT_6:
		exp_desp_len = 8;
		desp_len = param[3];
		break;
	case SCSIOPC_MODE_SELECT_10:
		long_lba = param[3] & SCSI_MODE_SELECT10_LONGLBA_MASK;
		SAT_CHECK_AND_SET(0 == long_lba, exp_desp_len, 8, 16);
		desp_len = (param[6] << 8) + param[7];
		break;
	default:
		return ERROR;
	}
	*ret_desp_len = desp_len;

	/* according to SPC, block descriptor length can be 0 */
	if (0 == desp_len) {
		SAT_TRACE(OSP_DBL_INFO, 123,
			  "Disk 0x%llx, block descriptor length is 0",
			  sata_dev->dev_id);
		return OK;
	} else if (exp_desp_len != desp_len) {
		*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
		*sense_code = SCSI_SNSCODE_INVALID_FIELD_PARAMETER_LIST;
		SAT_TRACE(OSP_DBL_MINOR, 123,
			  "block descriptor length %d not equal expected length %d",
			  desp_len, exp_desp_len);
		return ERROR;
	}

	if (len <= header_len + desp_len) {
		*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
		*sense_code = SCSI_SNSCODE_PARAMETER_LIST_LENGTH_ERROR;
		SAT_TRACE(OSP_DBL_MINOR, 123,
			  "parameter list length %d, first two part length %d",
			  len, header_len + desp_len);
		return ERROR;
	}

	logic_block_len = (u32) (param[header_len + 5] << 16) +
	    (u32) (param[header_len + 6] << 8) + (u32) (param[header_len + 7]);

	if (16 == desp_len) {
		logic_block_len = (u32) (param[header_len + 12] << 24) +
		    (u32) (param[header_len + 13] << 16) +
		    (u32) (param[header_len + 14] << 8) +
		    (u32) (param[header_len + 15]);
	}

	if (512 != logic_block_len) {
		*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
		*sense_code = SCSI_SNSCODE_INVALID_FIELD_PARAMETER_LIST;
		SAT_TRACE(OSP_DBL_MINOR, 123,
			  "Logical block length %d error", logic_block_len);
		return ERROR;
	}

	*page_offset = header_len + desp_len;
	switch (param[*page_offset] & 0x3f) {
	case SCSI_MODESELECT_CONTROL_PAGE:
	case SCSI_MODESELECT_READ_WRITE_ERROR_RECOVERY_PAGE:
	case SCSI_MODESELECT_INFORMATION_EXCEPTION_CONTROL_PAGE:
		exp_page_len = 12;
		break;
	case SCSI_MODESELECT_CACHING:
		exp_page_len = 20;
		break;
	default:
		*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
		*sense_code = SCSI_SNSCODE_INVALID_FIELD_PARAMETER_LIST;
		return ERROR;
	}

	if (len != header_len + exp_desp_len + exp_page_len) {
		*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
		*sense_code = SCSI_SNSCODE_PARAMETER_LIST_LENGTH_ERROR;
		SAT_TRACE(OSP_DBL_MINOR, 123,
			  "parameter list length %d not equal expected length %d",
			  len, header_len + exp_desp_len + exp_page_len);
		return ERROR;
	}
	return OK;
}

u8 sat_fill_mode_select_fis(struct sata_h2d * fis,
			    u8 * page, struct sata_device * sata_dev)
{
	u8 io_count = 0;

	fis->header.fis_type = 0x27;
	fis->header.pm_port = 0x80;

	switch (page[0] & 0x3f) {
	case SCSI_MODESELECT_CACHING:
		fis->header.command = FIS_COMMAND_SET_FEATURES;
		
		if (((page[2] & 0x04) >> 2) !=
		    ((sata_dev->sata_identify.
		      cmd_set_feature_enabled & 0x20) >> 5)) {
			io_count += 1;
			SAT_CHECK_AND_SET(page[2] & 0x04, fis->header.features, 0x02,	
					  0x82);	
		}
		
		if (((page[12] & 0x20) >> 5) ==
		    ((sata_dev->sata_identify.
		      cmd_set_feature_enabled & 0x40) >> 6)) {
			io_count += 1;
			SAT_CHECK_AND_SET(page[12] & 0x20, fis->header.features, 0x55,
					  0xAA);	
		}
		break;
	case SCSI_MODESELECT_INFORMATION_EXCEPTION_CONTROL_PAGE:
		
		if (((page[2] & 0x08) >> 3) ==
		    (sata_dev->sata_identify.cmd_set_feature_enabled & 0x01)) {
			io_count += 1;
			fis->header.command =
			    FIS_COMMAND_ENABLE_DISABLE_OPERATIONS;
			SAT_CHECK_AND_SET(page[2] & 0x08, fis->header.features,
					  0xD9, 0xD8);
			fis->data.lba_mid = 0x4F;
			fis->data.lab_high = 0xC2;
		}
		break;
	default:
		break;
	}
	SAT_TRACE(OSP_DBL_INFO, 123,
		  "Disk 0x%llx,SATA mode select, ATA command need to be send:%d",
		  sata_dev->dev_id, io_count);
	return io_count;
}

/**
 * sat_ms_mode_page - process Mode Page command,
 * 
 * @req: sata request info
 * @sata_dev: sata device related
 * @page: page
 * @sense_key: sense key
 * @sense_code: sense code
 * @send_ret: IO result
 *
 */
s32 sat_ms_mode_page(struct stp_req * req,
		     struct sata_device * sata_dev,
		     u8 * page,
		     u8 * sense_key, u16 * sense_code, s32 * send_ret)
{
	u8 page_code = 0;
	struct sata_h2d *tmp_fis = NULL;
	u8 ata_cmd_cnt = 0;
	struct stp_req *sub_req = NULL;

	*sense_key = SCSI_SNSKEY_NO_SENSE;
	*sense_code = SCSI_SNSCODE_NO_ADDITIONAL_INFO;

	tmp_fis = &req->h2d_fis;
	page_code = page[0] & 0x3f;

	memset(tmp_fis, 0, sizeof(struct sata_h2d));

	switch (page_code) {
	case SCSI_MODESELECT_CONTROL_PAGE:
		
		if ((page[1] != 0x0A) |
		    (page[2] & 0xF5) |
		    ((page[3] & 0xF6) != 0x12) |
		    (page[4] & 0x38) |
		    (page[5] & 0x47) |
		    (page[8] != 0xFF) |
		    (page[9] != 0xFF) | page[10] | page[11]) {
			*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
			*sense_code = SCSI_SNSCODE_INVALID_FIELD_PARAMETER_LIST;
			return ERROR;
		}
		*send_ret = SAT_IO_COMPLETE;
		return OK;
	case SCSI_MODESELECT_READ_WRITE_ERROR_RECOVERY_PAGE:
		
		if ((page[1] != 0x0A) |
		    ((page[2] & 0xDF) != 0x80) | page[10] | page[11]) {
			*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
			*sense_code = SCSI_SNSCODE_INVALID_FIELD_PARAMETER_LIST;
			return ERROR;
		}
		*send_ret = SAT_IO_COMPLETE;
		return OK;
	case SCSI_MODESELECT_CACHING:
		
		if ((page[1] != 0x12) |
		    (page[2] & 0xFB) |
		    (page[3]) |
		    (page[4]) |
		    (page[5]) |
		    (page[6]) |
		    (page[7]) |
		    (page[8]) |
		    (page[9]) |
		    (page[10]) |
		    (page[11]) |
		    (page[12] & 0xC1) | (page[13]) | (page[14]) | (page[15])) {
			*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
			*sense_code = SCSI_SNSCODE_INVALID_FIELD_PARAMETER_LIST;
			return ERROR;
		}
		ata_cmd_cnt = sat_fill_mode_select_fis(tmp_fis, page, sata_dev);

		if (0 == ata_cmd_cnt) {
			*send_ret = SAT_IO_COMPLETE;
			return OK;
		} else if (1 == ata_cmd_cnt) {
			req->compl_call_back = sat_mode_select_cb;
			*send_ret = sat_send_req_to_dev(req);
			return OK;
		}

		/* need two IO, get the first sub-IO */
		sub_req = sat_get_new_msg(sata_dev, 0);
		if (NULL == sub_req) {
			SAT_TRACE(OSP_DBL_MINOR, 5,
				  "Alloc resource for device 0x%llx failed",
				  sata_dev->dev_id);
			*send_ret = SAT_IO_FAILED;
			return OK;
		}
		sat_init_internal_command(sub_req, req, sata_dev);
		memcpy(&sub_req->h2d_fis, tmp_fis, sizeof(struct sata_h2d));
		sub_req->compl_call_back = sat_mode_select_cb;

		/* first send IO with WCE feature */
		SAT_CHECK_AND_SET(page[2] & 0x04,
				  sub_req->h2d_fis.header.features, 0x02, 0x82);

		*send_ret = sat_send_req_to_dev(sub_req);
		if (SAT_IO_SUCCESS != *send_ret) {
			sat_free_int_msg(sub_req);
		}
		return OK;

	case SCSI_MODESELECT_INFORMATION_EXCEPTION_CONTROL_PAGE:

		if ((page[1] != 0x0A) |
		    (page[2] & 0x84) |
		    ((page[3] & 0x0F) != 0x06) |
		    !(sata_dev->sata_identify.cmd_set_supported & 0x01)) {
			*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
			*sense_code = SCSI_SNSCODE_INVALID_FIELD_PARAMETER_LIST;
			return ERROR;
		}
		ata_cmd_cnt = sat_fill_mode_select_fis(tmp_fis, page, sata_dev);
		if (0 == ata_cmd_cnt) {
			*send_ret = SAT_IO_COMPLETE;
			return OK;
		}
		req->compl_call_back = sat_mode_select_cb;
		*send_ret = sat_send_req_to_dev(req);
		return OK;

	default:		
		return ERROR;
	}
}

/**
 * sat_mode_select - translate MODE SELECT command from SCSI to ATA,
 * and Send to Device.
 * 
 * @req: sata request info
 * @scsi_cmd: sata request related scsi cmd
 * @sata_dev: sata device related
 *
 */
s32 sat_mode_select(struct stp_req * req,
		    struct sat_ini_cmnd * scsi_cmd,
		    struct sata_device * sata_dev)
{
	u8 *cdb = NULL;
	u8 scsi_opc = 0;
	u8 *mode_param = NULL;
	struct sal_msg *upper_msg = NULL;
	u8 sense_key = SCSI_SNSKEY_NO_SENSE;
	u16 sense_code = SCSI_SNSCODE_NO_ADDITIONAL_INFO;
	u32 page_offset = 0;
	u32 desp_len = 0;
	u32 param_list_len = 0;
	s32 ret = SAT_IO_FAILED;

	cdb = SAT_GET_SCSI_COMMAND(scsi_cmd);
	scsi_opc = cdb[0];

	if (ERROR ==
	    sat_ms_check_cdb(cdb, &param_list_len, &sense_key, &sense_code)) {
		SAT_TRACE(OSP_DBL_MINOR, 143,
			  "Disk 0x%llx mode select,error field in CDB",
			  sata_dev->dev_id);
		sat_set_sense_and_result(req, sense_key, sense_code);
		return SAT_IO_COMPLETE;
	}

	if (0 == param_list_len) {
		SAT_TRACE(OSP_DBL_INFO, 1000,
			  "Disk 0x%llx, field parameter list length in CDB is 0",
			  sata_dev->dev_id);
		return SAT_IO_COMPLETE;
	}

	upper_msg = (struct sal_msg *)req->upper_msg;
	mode_param = SAS_KMALLOC(upper_msg->ini_cmnd.data_len, GFP_ATOMIC);

	if (0 == upper_msg->ini_cmnd.data_len) {
		SAT_TRACE(OSP_DBL_MINOR, 10000,
			  "Disk 0x%llx, mode select parameter list length is 0",
			  sata_dev->dev_id);

		sat_set_sense_and_result(req,
					 SCSI_SNSKEY_ILLEGAL_REQUEST,
					 SCSI_SNSCODE_PARAMETER_LIST_LENGTH_ERROR);
		return SAT_IO_COMPLETE;
	}

	if (NULL == mode_param ||
	    upper_msg->ini_cmnd.data_len !=
	    sal_sg_copy_to_buffer((struct sal_msg *)req->upper_msg, mode_param,
				  upper_msg->ini_cmnd.data_len)) {
		SAT_TRACE(OSP_DBL_MAJOR, 1234,
			  "Device 0x%llx Mode Select,get sgl content failed",
			  sata_dev->dev_id);
		if (NULL != mode_param)
			SAS_KFREE(mode_param);

		return SAT_IO_FAILED;
	}

	memset(mode_param, 0, upper_msg->ini_cmnd.data_len);

	if (ERROR == sat_ms_header_and_desp(scsi_opc,
					    sata_dev,
					    mode_param,
					    upper_msg->ini_cmnd.data_len,
					    &sense_key,
					    &sense_code,
					    &desp_len, &page_offset)) {
		SAT_TRACE(OSP_DBL_MINOR, 143,
			  "Disk 0x%llx mode select,error field in parameter list",
			  sata_dev->dev_id);
		sat_set_sense_and_result(req, sense_key, sense_code);
		SAS_KFREE(mode_param);
		return SAT_IO_COMPLETE;
	}

	if (0 == desp_len) {
		SAT_TRACE(OSP_DBL_INFO, 155,
			  "Disk 0x%llx mode select,block descrptor length is 0",
			  sata_dev->dev_id);
		SAS_KFREE(mode_param);
		return SAT_IO_COMPLETE;
	}

	if (ERROR == sat_ms_mode_page(req,
				      sata_dev,
				      mode_param + page_offset,
				      &sense_key, &sense_code, &ret)) {
		SAT_TRACE(OSP_DBL_MINOR, 143,
			  "Disk 0x%llx mode select,error field in mode page",
			  sata_dev->dev_id);
		sat_set_sense_and_result(req, sense_key, sense_code);
		SAS_KFREE(mode_param);
		return SAT_IO_COMPLETE;
	}

	SAS_KFREE(mode_param);
	return ret;
}

s32 sat_rb_check_cdb(u8 * cdb, u8 * sense_key, u16 * sense_code)
{
	u8 mode = 0;
	u8 buff_id = 0;
	u32 buff_offset = 0;
	u32 alloc_len = 0;
	*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
	*sense_code = SCSI_SNSCODE_INVALID_FIELD_IN_CDB;
	mode = cdb[1] & SCSI_READ_WRITE_BUFFER_MODE_MASK;

	buff_id = cdb[2];
	buff_offset = (cdb[3] << 16) | (cdb[4] << 8) | (cdb[5]);

	alloc_len = (cdb[6] << 16) | (cdb[7] << 8) | (cdb[8]);

	if (0 != buff_id)
		return ERROR;

	if (READ_WRITE_BUFFER_DATA_MODE == mode) {
		if ((0 != buff_offset) || (512 != alloc_len))
			return ERROR;
	} else if (READ_BUFFER_DESCRIPTOR_MODE == mode) {
		if (alloc_len < 4)
			return ERROR;
	} else {
		return ERROR;
	}
	return OK;
}

/**
 * sat_read_buffer - translate READ BUFFER command from SCSI to ATA,
 * and Send to Device.
 * 
 * @req: sata request info
 * @scsi_cmd: sata request related scsi cmd
 * @sata_dev: sata device related
 *
 */
s32 sat_read_buffer(struct stp_req * req,
		    struct sat_ini_cmnd * scsi_cmd,
		    struct sata_device * sata_dev)
{
	u8 mode = 0;
	u8 tmp_buff[4] = { 0 };
	u8 *cdb = NULL;
	struct sata_h2d *tmp_fis = NULL;
	s32 ret = 0;
	u8 sense_key = SCSI_SNSKEY_NO_SENSE;
	u16 sense_code = SCSI_SNSCODE_NO_ADDITIONAL_INFO;
	struct sal_msg *sal_msg = NULL;

	sal_msg = (struct sal_msg *)req->upper_msg;

	tmp_fis = &req->h2d_fis;
	cdb = SAT_GET_SCSI_COMMAND(scsi_cmd);
	mode = cdb[1] & SCSI_READ_WRITE_BUFFER_MODE_MASK;
	memset(tmp_fis, 0, sizeof(struct sata_h2d));

	if (ERROR == sat_rb_check_cdb(cdb, &sense_key, &sense_code)) {
		SAT_TRACE(OSP_DBL_MINOR, 143,
			  "Disk 0x%llx read buffer,error field in CDB",
			  sata_dev->dev_id);
		sat_set_sense_and_result(req, sense_key, sense_code);
		return SAT_IO_COMPLETE;
	}

	if (READ_WRITE_BUFFER_DATA_MODE == mode) {
		tmp_fis->header.fis_type = 0x27;
		tmp_fis->header.pm_port = 0x80;
		tmp_fis->header.command = FIS_COMMAND_READ_BUFFER;	
		tmp_fis->data.device = 0x40;
		req->compl_call_back = sat_common_cb;

		ret = sat_send_req_to_dev(req);
		return ret;
	}

	tmp_buff[0] = 0xFF;
	tmp_buff[1] = 0x00;
	tmp_buff[2] = 0x02;
	tmp_buff[3] = 0x00;

	if (sizeof(tmp_buff) !=
	    sal_sg_copy_from_buffer(sal_msg, tmp_buff, sizeof(tmp_buff)))
		return SAT_IO_FAILED;

	sal_msg->status.io_resp = SAM_STAT_GOOD;
	sal_msg->status.drv_resp = SAL_MSG_DRV_SUCCESS;
	return SAT_IO_COMPLETE;
}

s32 sat_wb_check_cdb(u8 * cdb, u8 * sense_key, u16 * sense_code)
{
	u8 mode = 0;
	u8 buff_id = 0;
	u32 buff_offset = 0;
	u32 param_list_len = 0;
	*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
	*sense_code = SCSI_SNSCODE_INVALID_FIELD_IN_CDB;
	mode = cdb[1] & SCSI_READ_WRITE_BUFFER_MODE_MASK;

	buff_id = cdb[2];
	buff_offset = (cdb[3] << 16) | (cdb[4] << 8) | (cdb[5]);

	param_list_len = (cdb[6] << 16) | (cdb[7] << 8) | (cdb[8]);

	if (READ_WRITE_BUFFER_DATA_MODE == mode) {
		if ((0 != buff_id) || (0 != buff_offset)
		    || (512 != param_list_len))
			return ERROR;
	} else {
		//Download microcodeÃ»ÓÐ×ö
		return ERROR;
	}
	return OK;
}

/**
 * sat_write_buffer - translate WRITE BUFFER command from SCSI to ATA,
 * and Send to Device.
 * 
 * @req: sata request info
 * @scsi_cmd: sata request related scsi cmd
 * @sata_dev: sata device related
 *
 */
s32 sat_write_buffer(struct stp_req * req,
		     struct sat_ini_cmnd * scsi_cmd,
		     struct sata_device * sata_dev)
{
	u8 *cdb = NULL;
	struct sata_h2d *tmp_fis = NULL;
	s32 ret = 0;
	u8 sense_key = SCSI_SNSKEY_NO_SENSE;
	u16 sense_code = SCSI_SNSCODE_NO_ADDITIONAL_INFO;

	tmp_fis = &req->h2d_fis;
	cdb = SAT_GET_SCSI_COMMAND(scsi_cmd);

	memset(tmp_fis, 0, sizeof(struct sata_h2d));

	if (ERROR == sat_wb_check_cdb(cdb, &sense_key, &sense_code)) {
		SAT_TRACE(OSP_DBL_MINOR, 143,
			  "Disk 0x%llx write buffer,error field in CDB",
			  sata_dev->dev_id);
		sat_set_sense_and_result(req, sense_key, sense_code);
		return SAT_IO_COMPLETE;
	}

	tmp_fis->header.fis_type = 0x27;
	tmp_fis->header.pm_port = 0x80;
	tmp_fis->header.command = FIS_COMMAND_WRITE_BUFFER;	
	tmp_fis->data.device = 0x40;
	req->compl_call_back = sat_common_cb;

	ret = sat_send_req_to_dev(req);

	return ret;
}

void sat_send_diagnostic_cb(struct stp_req *stp_request, u32 io_status)
{
	struct stp_req *org_req = NULL;
	struct stp_req *sub_req = NULL;
	struct sata_h2d *tmp_fis = NULL;
	struct sata_h2d *sub_fis = NULL;
	struct sata_device *tmp_sata_dev = NULL;
	s32 ret = SAT_IO_FAILED;

	sub_req = stp_request;
	org_req = sub_req->org_msg;
	tmp_fis = &org_req->h2d_fis;
	sub_fis = &sub_req->h2d_fis;
	tmp_sata_dev = sub_req->dev;

	switch (sub_fis->header.command) {
	case FIS_COMMAND_READ_VERIFY_SECTOR:
	case FIS_COMMAND_READ_VERIFY_SECTOR_EXT:
		tmp_fis->data.reserved_5++;
		if (tmp_fis->data.reserved_5 == 1) {
			sub_fis->data.lba_low = tmp_sata_dev->sata_max_lba[7];
			sub_fis->data.lba_mid = tmp_sata_dev->sata_max_lba[6];
			sub_fis->data.lab_high = tmp_sata_dev->sata_max_lba[5];
			SAT_CHECK_AND_SET(tmp_sata_dev->
					  dev_cap & SAT_DEV_CAP_48BIT,
					  sub_fis->data.lba_low_exp,
					  tmp_sata_dev->sata_max_lba[4], 0);

			SAT_CHECK_AND_SET(tmp_sata_dev->
					  dev_cap & SAT_DEV_CAP_48BIT,
					  sub_fis->data.lba_mid_exp,
					  tmp_sata_dev->sata_max_lba[3], 0);

			SAT_CHECK_AND_SET(tmp_sata_dev->
					  dev_cap & SAT_DEV_CAP_48BIT,
					  sub_fis->data.lba_high_exp,
					  tmp_sata_dev->sata_max_lba[2], 0);
		} else if (tmp_fis->data.reserved_5 == 2) {
			sub_fis->data.lba_low = 0x7F;
			sub_fis->data.lba_mid = 0;
			sub_fis->data.lab_high = 0;
			sub_fis->data.lba_low_exp = 0;
			sub_fis->data.lba_mid_exp = 0;
			sub_fis->data.lba_high_exp = 0;
		} else {
			sat_free_int_msg(sub_req);
			sat_io_completed(org_req, SAT_IO_SUCCESS);
			return;
		}
		ret = sat_send_req_to_dev(sub_req);
		if (SAT_IO_SUCCESS != ret) {
			sat_free_int_msg(sub_req);
			sat_io_completed(org_req, ret);
		}
		break;
	case FIS_COMMAND_SMART_EXECUTE_OFFLINE_IMMEDIATE:
		if (IS_BACKGROUND_SELF_TEST(sub_fis->data.lba_low)) {
			tmp_sata_dev->diag_pending = false;
			sat_free_int_msg(sub_req);
		} else {
			tmp_sata_dev->diag_pending = false;
			sat_io_completed(sub_req, SAT_IO_SUCCESS);
		}
		break;
	default:		
		sat_io_completed(sub_req, SAT_IO_SUCCESS);
		break;
	}
}

/**
 * sat_sd_check_cdb - check cdb of the send diagnostic command
 * 
 * @cdb: cdb of the send diagnostic command
 * @sense_key: sense key
 * @sense_code: sense code
 *
 */
s32 sat_sd_check_cdb(u8 * cdb, u8 * sense_key, u16 * sense_code)
{
	u8 cdb_std[10] = { 0x01, 0x51, 0xff, 0xf0, 0xff,
		0xff, 0x00, 0x00, 0x00, 0x00
	};

	*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
	*sense_code = SCSI_SNSCODE_INVALID_FIELD_IN_CDB;

	if (!sat_field_check(cdb_std, cdb, sizeof(cdb_std) / 2))
		return ERROR;

	return OK;
}

s32 sat_sd_self_test(struct stp_req * req,
		     struct sata_device * sata_dev, u8 * cdb, s32 * send_ret)
{
	struct sata_h2d *tmp_fis = NULL;
	struct sata_h2d *sub_fis = NULL;
	struct stp_req *sub_req = NULL;

	tmp_fis = &req->h2d_fis;
	memset(tmp_fis, 0, sizeof(struct sata_h2d));

	/* if self test is not support or disable, then need send 3 verify */
	sub_req = sat_get_new_msg(sata_dev, 0);
	if (NULL == sub_req) {
		SAT_TRACE(OSP_DBL_MINOR, 145,
			  "Alloc resource for device 0x%llx failed",
			  sata_dev->dev_id);
		*send_ret = SAT_IO_FAILED;
		return OK;
	}
	sat_init_internal_command(sub_req, req, sata_dev);
	sub_fis = &sub_req->h2d_fis;
	memset(sub_fis, 0, sizeof(struct sata_h2d));

	/* we do not send fis of the orignal IO, use reserve field 
	 * to trace verify progress 
	 */
	tmp_fis->data.reserved_5 = 0;

	sub_fis->header.fis_type = 0x27;
	sub_fis->header.pm_port = 0x80;
	SAT_CHECK_AND_SET(sata_dev->dev_cap & SAT_DEV_CAP_48BIT,
			  sub_fis->header.command,
			  FIS_COMMAND_READ_VERIFY_SECTOR_EXT,
			  FIS_COMMAND_READ_VERIFY_SECTOR);

	sub_fis->data.sector_cnt = 1;
	sub_fis->data.device = 0x40;
	sub_req->compl_call_back = sat_send_diagnostic_cb;

	*send_ret = sat_send_req_to_dev(sub_req);
	if (SAT_IO_SUCCESS != *send_ret)
		sat_free_int_msg(sub_req);

	return OK;
}

s32 sat_sd_non_self_test(struct stp_req * req,
			 struct sata_device * sata_dev,
			 u8 * cdb,
			 u8 * sense_key, u16 * sense_code, s32 * send_ret)
{
	s32 self_test_supported = false;
	s32 self_test_enabled = false;
	struct sata_h2d *tmp_fis = NULL;
	struct stp_req *sub_req = NULL;
	struct sata_h2d *sub_fis = NULL;
	u8 self_test_code = 0;

	/* Word 84 bit 1, ATA SMART EXECUTE OFF-LINE IMMEDIATE supported */
	SAT_CHECK_AND_SET(sata_dev->sata_identify.
			  cmd_set_feature_supported_ext & 0x02,
			  self_test_supported, true, false);
	/* Word 85 bit 0, ATA SMART EXECUTE OFF-LINE IMMEDIATE enabled */
	SAT_CHECK_AND_SET(sata_dev->sata_identify.
			  cmd_set_feature_enabled & 0x01, self_test_enabled,
			  true, false);

	if (!self_test_supported) {
		*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
		*sense_code = SCSI_SNSCODE_INVALID_FIELD_IN_CDB;
		return ERROR;
	} else if (!self_test_enabled) {
		*sense_key = SCSI_SNSKEY_ABORTED_COMMAND;
		*sense_code = SCSI_SNSCODE_ATA_DEVICE_FEATURE_NOT_ENABLED;
		return ERROR;
	}

	tmp_fis = &req->h2d_fis;
	memset(tmp_fis, 0, sizeof(struct sata_h2d));
	self_test_code = (cdb[1] & SCSI_SEND_DIAGNOSTIC_TEST_CODE_MASK) >> 5;

	switch (self_test_code) {
		/* for background self-test,return orignal IO, send sub-IO */
	case 1:
	case 2:
		sub_req = sat_get_new_msg(sata_dev, 0);
		if (NULL == sub_req) {
			SAT_TRACE(OSP_DBL_MINOR, 6,
				  "Alloc resource for device 0x%llx failed",
				  sata_dev->dev_id);
			*send_ret = SAT_IO_FAILED;
			return OK;
		}
		sub_req->dev = sata_dev;
		sub_req->ll_msg = sub_req;
		sub_req->org_msg = sub_req;	/* just return orignal msg */
		SAS_INIT_LIST_HEAD(&sub_req->io_list);
		sub_req->compl_call_back = sat_send_diagnostic_cb;

		sub_fis = &sub_req->h2d_fis;
		memset(sub_fis, 0, sizeof(struct sata_h2d));

		sub_fis->header.fis_type = 0x27;
		sub_fis->header.pm_port = 0x80;
		sub_fis->header.command =
		    FIS_COMMAND_SMART_EXECUTE_OFFLINE_IMMEDIATE;
		sub_fis->header.features = 0xD4;
		sub_fis->data.lba_low = self_test_code;
		sub_fis->data.lba_mid = 0x4F;
		sub_fis->data.lab_high = 0xC2;

		/* directly send sub-IO, return the orignal IO based on sub-IO result */
		*send_ret = sat_send_req_to_dev(sub_req);
		if (*send_ret == SAT_IO_SUCCESS) {
			sata_dev->diag_pending = true;
			*send_ret = SAT_IO_COMPLETE;
		} else {
			sat_free_int_msg(sub_req);
			*send_ret = SAT_IO_FAILED;
		}
		return OK;

	case 4:

		tmp_fis->header.fis_type = 0x27;
		tmp_fis->header.pm_port = 0x80;
		tmp_fis->header.command =
		    FIS_COMMAND_SMART_EXECUTE_OFFLINE_IMMEDIATE;
		tmp_fis->header.features = 0xD4;
		tmp_fis->data.lba_low = 0x7F;
		tmp_fis->data.lba_mid = 0x4F;
		tmp_fis->data.lab_high = 0xC2;

		req->compl_call_back = sat_send_diagnostic_cb;
		*send_ret = sat_send_req_to_dev(req);
		return OK;

	case 5:
	case 6:

	default:
		*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
		*sense_code = SCSI_SNSCODE_INVALID_FIELD_IN_CDB;
		return ERROR;
	}

}

/**
 * sat_send_diagnostic - translate SEND DIAGNOSTIC command from SCSI to ATA,
 * and Send to Device.
 * 
 * @req: sata request info
 * @scsi_cmd: sata request related scsi cmd
 * @sata_dev: sata device related
 *
 */
s32 sat_send_diagnostic(struct stp_req * req,
			struct sat_ini_cmnd * scsi_cmd,
			struct sata_device * sata_dev)
{
	u8 *cdb = NULL;
	struct sata_h2d *tmp_fis = NULL;
	s32 ret = 0;
	s32 tmp_ret = 0;
	u8 sense_key = SCSI_SNSKEY_NO_SENSE;
	u16 sense_code = SCSI_SNSCODE_NO_ADDITIONAL_INFO;

	tmp_fis = &req->h2d_fis;
	cdb = SAT_GET_SCSI_COMMAND(scsi_cmd);
	memset(tmp_fis, 0, sizeof(struct sata_h2d));

	if (ERROR == sat_sd_check_cdb(cdb, &sense_key, &sense_code)) {
		SAT_TRACE(OSP_DBL_MINOR, 143,
			  "Disk 0x%llx send diagnotic,error field in CDB",
			  sata_dev->dev_id);
		sat_set_sense_and_result(req, sense_key, sense_code);
		return SAT_IO_COMPLETE;
	}

	if (cdb[1] & SCSI_SEND_DIAGNOSTIC_SELFTEST_MASK)
		tmp_ret = sat_sd_self_test(req, sata_dev, cdb, &ret);
	else
		tmp_ret = sat_sd_non_self_test(req,
					       sata_dev,
					       cdb,
					       &sense_key, &sense_code, &ret);

	if (ERROR == tmp_ret) {
		SAT_TRACE(OSP_DBL_INFO, 144,
			  "Disk 0x%llx send diagnotic, check condition",
			  sata_dev->dev_id);
		sat_set_sense_and_result(req, sense_key, sense_code);
		return SAT_IO_COMPLETE;
	}

	return ret;
}

u64 sat_calc_dev_max_lba(struct sata_device * sata_dev)
{
	u64 dev_max_lba = 0;
	dev_max_lba = ((u64) sata_dev->sata_max_lba[0] << 56) |
	    ((u64) sata_dev->sata_max_lba[1] << 48) |
	    ((u64) sata_dev->sata_max_lba[2] << 40) |
	    ((u64) sata_dev->sata_max_lba[3] << 32) |
	    ((u64) sata_dev->sata_max_lba[4] << 24) |
	    ((u64) sata_dev->sata_max_lba[5] << 16) |
	    ((u64) sata_dev->sata_max_lba[6] << 8) |
	    ((u64) sata_dev->sata_max_lba[7]);
	return dev_max_lba;
}

void sat_write_same_cb(struct stp_req *stp_request, u32 io_status)
{
	struct stp_req *org_req = NULL;
	struct stp_req *sub_req = NULL;
	struct sata_h2d *sub_fis = NULL;
	s32 ret = 0;

	sub_req = stp_request;
	org_req = sub_req->org_msg;
	sub_fis = &sub_req->h2d_fis;
	if (sub_req->curr_lba ==
	    (org_req->curr_lba + (org_req->org_transfer_len - 1))) {
		sat_free_int_msg(sub_req);
		sat_io_completed(org_req, SAT_IO_SUCCESS);
		return;
	}

	sub_req->curr_lba++;

	sub_fis->data.lba_low_exp =
	    (u8) ((sub_req->curr_lba & 0xff000000) >> 24);
	sub_fis->data.lab_high = (u8) ((sub_req->curr_lba & 0x00ff0000) >> 16);
	sub_fis->data.lba_mid = (u8) ((sub_req->curr_lba & 0x0000ff00) >> 8);
	sub_fis->data.lba_low = (u8) (sub_req->curr_lba & 0x000000ff);

	ret = sat_send_req_to_dev(sub_req);
	if (SAT_IO_SUCCESS != ret) {
		sat_free_int_msg(sub_req);
		sat_io_completed(org_req, ret);
	}
	return;

}

s32 sat_ws_check_cdb(struct sata_device * sata_dev,
		     u8 * cdb, u8 * sense_key, u16 * sense_code)
{
	u64 req_max_lba = 0;
	u64 dev_max_lba = 0;
	u32 lba = 0;
	u32 trans_len = 0;

	dev_max_lba = sat_calc_dev_max_lba(sata_dev);
	lba = sat_calc_lba(cdb);
	trans_len = sat_calc_trans_len(cdb);

	req_max_lba = (u64) lba + trans_len;
	
	if (cdb[1] & 0x06) {
		*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
		*sense_code = SCSI_SNSCODE_INVALID_FIELD_IN_CDB;
		return ERROR;
	}

	if (!(sata_dev->dev_cap & SAT_DEV_CAP_48BIT)) {
		*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
		*sense_code = SCSI_SNSCODE_INVALID_FIELD_IN_CDB;
		return ERROR;
	}

	if (req_max_lba > dev_max_lba || req_max_lba > 0xffffffff) {
		*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
		*sense_code = SCSI_SNSCODE_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		return ERROR;
	}

	if (trans_len == 0 && dev_max_lba > 0xffffffff) {
		*sense_key = SCSI_SNSKEY_ILLEGAL_REQUEST;
		*sense_code = SCSI_SNSCODE_INVALID_FIELD_IN_CDB;
		return ERROR;
	}
	return OK;
}

/**
 * sat_write_same - translate WRITE SAME command from SCSI to ATA,
 * and Send to Device.
 * 
 * @req: sata request info
 * @scsi_cmd: sata request related scsi cmd
 * @sata_dev: sata device related
 *
 */
s32 sat_write_same(struct stp_req * req,
		   struct sat_ini_cmnd * scsi_cmd,
		   struct sata_device * sata_dev)
{
	u8 *cdb = NULL;
	struct sata_h2d *sub_fis = NULL;
	s32 ret = 0;
	u64 dev_max_lba = 0;
	struct stp_req *sub_req = NULL;
	struct sal_msg *sub_msg = NULL;
	struct sal_msg *org_msg = NULL;
	u8 sense_key = SCSI_SNSKEY_NO_SENSE;
	u16 sense_code = SCSI_SNSCODE_NO_ADDITIONAL_INFO;

	cdb = SAT_GET_SCSI_COMMAND(scsi_cmd);

	if (ERROR == sat_ws_check_cdb(sata_dev, cdb, &sense_key, &sense_code)) {
		SAT_TRACE(OSP_DBL_MINOR, 143,
			  "Disk 0x%llx Write Same,error field in CDB",
			  sata_dev->dev_id);
		sat_set_sense_and_result(req, sense_key, sense_code);
		return SAT_IO_COMPLETE;
	}
	dev_max_lba = sat_calc_dev_max_lba(sata_dev);
	req->curr_lba = sat_calc_lba(cdb);
	req->org_transfer_len = sat_calc_trans_len(cdb);

	if (0 == req->org_transfer_len)
		req->org_transfer_len = (u32) (dev_max_lba - req->curr_lba) + 1;

	sub_req = sat_get_new_msg(sata_dev, 0);
	if (NULL == sub_req) {
		SAT_TRACE(OSP_DBL_MINOR, 5,
			  "Alloc resource for device 0x%llx failed"
			  "command 0x%x", sata_dev->dev_id,
			  SAT_GET_SCSI_COMMAND(scsi_cmd)[0]);
		return SAT_IO_FAILED;
	}
	sat_init_internal_command(sub_req, req, sata_dev);
	sub_fis = &sub_req->h2d_fis;
	memset(sub_fis, 0, sizeof(struct sata_h2d));
	sub_req->curr_lba = req->curr_lba;

	sub_msg = (struct sal_msg *)sub_req->upper_msg;
	org_msg = (struct sal_msg *)req->upper_msg;

	sub_msg->ini_cmnd.data_len = org_msg->ini_cmnd.data_len;
	sub_msg->data_direction = org_msg->data_direction;
	sub_msg->ini_cmnd.sgl = org_msg->ini_cmnd.sgl;

	sub_fis->header.fis_type = 0x27;
	sub_fis->header.pm_port = 0x80;
	sub_fis->data.lba_low = cdb[5];
	sub_fis->data.lba_mid = cdb[4];
	sub_fis->data.lab_high = cdb[3];
	sub_fis->data.lba_low_exp = cdb[2];
	sub_fis->data.device = 0x40;

	if (sata_dev->dev_transfer_mode == SAT_TRANSFER_MOD_NCQ) {
		SAT_TRACE(OSP_DBL_INFO, 100, "NCQ");
		sub_fis->header.command = FIS_COMMAND_WRITE_FPDMA_QUEUED;
		sub_fis->header.features = 1;
	} else if (sata_dev->dev_transfer_mode == SAT_TRANSFER_MOD_DMA_EXT) {
		sub_fis->header.command = FIS_COMMAND_WRITE_DMA_EXT;
		sub_fis->data.sector_cnt = 1;
	} else {
		sub_fis->header.command = FIS_COMMAND_WRITE_SECTOR_EXT;
		sub_fis->data.sector_cnt = 1;
	}
	sub_req->compl_call_back = sat_write_same_cb;

	ret = sat_send_req_to_dev(sub_req);

	if (SAT_IO_SUCCESS != ret)
		sat_free_int_msg(sub_req);

	return ret;
}

void sat_check_power_mode_cb(struct stp_req *stp_request, u32 io_status)
{
	struct sata_device *tmp_sata_dev = NULL;
	struct sal_msg *tmp_msg = NULL;

	SAL_ASSERT(NULL != stp_request, return);

	tmp_msg = (struct sal_msg *)stp_request->upper_msg;

	if (SAT_IO_SUCCESS == io_status) {
		SAT_TRACE(OSP_DBL_INFO, 100,
			  "Disk 0x%llx check power mode response:0x%x(OK)",
			  stp_request->dev->dev_id, io_status);
		tmp_sata_dev = stp_request->dev;
		tmp_sata_dev->sata_state = SATA_DEV_STATUS_NORMAL;

		tmp_msg->status.drv_resp = SAL_MSG_DRV_SUCCESS;
		tmp_msg->status.io_resp = SAM_STAT_GOOD;
	} else {
		SAT_TRACE(OSP_DBL_MAJOR, 100,
			  "Disk 0x%llx check power mode response:0x%x(failed)",
			  stp_request->dev->dev_id, io_status);
		tmp_msg->status.drv_resp = SAL_MSG_DRV_FAILED;
	}
	(void)sal_msg_state_chg(tmp_msg, SAL_MSG_EVENT_SMP_DONE);

	return;
}

/**
 * sat_check_power_mode - send CHECK POWER MODE command 
 * 
 * @req: sata request info
 *
 */
s32 sat_check_power_mode(struct stp_req * req)
{
	struct sata_h2d *h2d_fis = NULL;
	s32 ret = ERROR;

	h2d_fis = &req->h2d_fis;
	memset(h2d_fis, 0, sizeof(struct sata_h2d));

	h2d_fis->header.features = 0;
	h2d_fis->header.command = FIS_COMMAND_CHECK_POWER_MODE;
	h2d_fis->header.fis_type = SATA_FIS_TYPE_H2D;
	h2d_fis->header.pm_port = SAT_H2D_CBIT_MASK;	/* C Bit is set */
	h2d_fis->data.control = 0;
	h2d_fis->data.device = 0;
	h2d_fis->data.features_exp = 0;
	h2d_fis->data.lab_high = 0;
	h2d_fis->data.lba_high_exp = 0;
	h2d_fis->data.lba_low = 0;
	h2d_fis->data.lba_low = 0;
	h2d_fis->data.lba_low_exp = 0;
	h2d_fis->data.lba_mid = 0;
	h2d_fis->data.lba_mid_exp = 0;
	h2d_fis->data.sector_cnt = 0;
	h2d_fis->data.sector_cnt_exp = 0;

	req->compl_call_back = sat_check_power_mode_cb;

	ret = sat_send_req_to_dev(req);
	if (SAT_IO_SUCCESS != ret) {
		SAT_TRACE(OSP_DBL_MAJOR, 234,
			  "Send check power mode command to disk:0x%llx failed",
			  req->dev->dev_id);
		return ERROR;
	}

	return OK;
}

void sat_de_reset_dev_cb(struct stp_req *stp_request, u32 io_status)
{
	struct sata_device *tmp_sata_dev = NULL;
	struct sal_msg *tm_msg = NULL;	

	tmp_sata_dev = stp_request->dev;
	tm_msg = (struct sal_msg *)stp_request->ll_msg->upper_msg;

	if (SAT_IO_SUCCESS == io_status) {
		
		tmp_sata_dev->sata_state = SATA_DEV_STATUS_NORMAL;
		SAT_TRACE(OSP_DBL_INFO, 100,
			  "Disk:0x%llx dereset response:0x%x(OK)",
			  stp_request->dev->dev_id, io_status);
		tm_msg->status.drv_resp = SAL_MSG_DRV_SUCCESS;
		tm_msg->status.io_resp = SAM_STAT_GOOD;

	} else {
		SAT_TRACE(OSP_DBL_INFO, 100,
			  "Disk:0x%llx dereset response:0x%x(failed)",
			  stp_request->dev->dev_id, io_status);
		tm_msg->status.drv_resp = SAL_MSG_DRV_FAILED;
	}
	(void)sal_msg_state_chg(tm_msg, SAL_MSG_EVENT_SMP_DONE);
	stp_request->ll_msg = NULL;	
	sat_free_int_msg(stp_request);
	return;
}

s32 sat_de_reset_dev(struct stp_req * org_stp_req,
		     struct sata_device * sata_dev)
{
	struct stp_req *new_req = NULL;
	struct sata_h2d *h2d_fis = NULL;
	s32 ret = ERROR;

	new_req = sat_get_new_msg(sata_dev, 0);
	if (NULL == new_req) {
		SAT_TRACE(OSP_DBL_MAJOR, 5,
			  "Alloc resource for device:0x%llx failed",
			  sata_dev->dev_id);
		return ERROR;
	}

	new_req->dev = sata_dev;
	new_req->ll_msg = org_stp_req;	
	new_req->org_msg = new_req;	

	SAS_INIT_LIST_HEAD(&new_req->io_list);

	h2d_fis = &new_req->h2d_fis;
	memset(h2d_fis, 0, sizeof(struct sata_h2d));

	h2d_fis->header.features = 0;
	h2d_fis->header.command = FIS_COMMAND_DEVICE_RESET;
	h2d_fis->header.fis_type = SATA_FIS_TYPE_H2D;
	h2d_fis->header.pm_port = 0;
	h2d_fis->data.control = 0;
	h2d_fis->data.device = 0;
	h2d_fis->data.features_exp = 0;
	h2d_fis->data.lab_high = 0;
	h2d_fis->data.lba_mid = 0;
	h2d_fis->data.lba_low = 0;
	h2d_fis->data.lba_high_exp = 0;
	h2d_fis->data.lba_mid_exp = 0;
	h2d_fis->data.lba_low_exp = 0;
	h2d_fis->data.sector_cnt = 0;
	h2d_fis->data.sector_cnt_exp = 0;
	h2d_fis->data.reserved_4 = 0;
	h2d_fis->data.reserved_5 = 0;
	new_req->compl_call_back = sat_de_reset_dev_cb;

	ret = sat_send_req_to_dev(new_req);
	if (SAT_IO_SUCCESS != ret) {
		SAT_TRACE(OSP_DBL_MAJOR, 234,
			  "Send dereset command to disk:0x%llx failed",
			  sata_dev->dev_id);
		sat_free_int_msg(new_req);
		return ERROR;
	}

	return OK;
}

void sat_reset_dev_cb(struct stp_req *stp_request, u32 io_status)
{
	struct sata_device *tmp_sata_dev = NULL;
	struct sal_msg *tmp_msg = NULL;
	s32 ret = 0;

	SAL_ASSERT(NULL != stp_request, return);

	tmp_sata_dev = stp_request->dev;
	tmp_msg = (struct sal_msg *)stp_request->upper_msg;

	if (SAT_IO_SUCCESS == io_status) {
		SAT_TRACE(OSP_DBL_INFO, 100,
			  "Disk:0x%llx reset response:0x%x(OK)",
			  stp_request->dev->dev_id, io_status);

		ret = sat_de_reset_dev(stp_request, tmp_sata_dev);
		if (OK != ret) {
			SAT_TRACE(OSP_DBL_MAJOR, 234,
				  "dereset device:0x%llx failed",
				  tmp_sata_dev->dev_id);
			tmp_msg->status.drv_resp = SAL_MSG_DRV_FAILED;
			(void)sal_msg_state_chg(tmp_msg,
						SAL_MSG_EVENT_SMP_DONE);

		}

	} else {
		SAT_TRACE(OSP_DBL_MAJOR, 234, "reset device:0x%llx rsp failed",
			  tmp_sata_dev->dev_id);
		tmp_msg->status.drv_resp = SAL_MSG_DRV_FAILED;
		(void)sal_msg_state_chg(tmp_msg, SAL_MSG_EVENT_SMP_DONE);

	}

	return;
}

s32 sat_reset_dev(struct stp_req * req)
{
	struct sata_h2d *h2d_fis = NULL;
	s32 ret = ERROR;

	h2d_fis = &req->h2d_fis;
	memset(h2d_fis, 0, sizeof(struct sata_h2d));

	h2d_fis->header.features = 0;
	h2d_fis->header.command = FIS_COMMAND_DEVICE_RESET;	/*any cmd */
	h2d_fis->header.fis_type = SATA_FIS_TYPE_H2D;
	h2d_fis->header.pm_port = 0;	/* C Bit is not set */
	h2d_fis->data.control = 0x4;	/* SRST bit is set  */
	h2d_fis->data.device = 0;
	h2d_fis->data.features_exp = 0;
	h2d_fis->data.lab_high = 0;
	h2d_fis->data.lba_high_exp = 0;
	h2d_fis->data.lba_low = 0;
	h2d_fis->data.lba_low_exp = 0;
	h2d_fis->data.lba_mid = 0;
	h2d_fis->data.lba_mid_exp = 0;
	h2d_fis->data.sector_cnt = 0;
	h2d_fis->data.sector_cnt_exp = 0;

	req->compl_call_back = sat_reset_dev_cb;

	ret = sat_send_req_to_dev(req);
	if (SAT_IO_SUCCESS != ret) {
		SAT_TRACE(OSP_DBL_MAJOR, 234,
			  "Send reset command to disk:0x%llx failed",
			  req->dev->dev_id);
		return ERROR;
	}

	return OK;

}

s32 sat_abort_task_mgmt(struct stp_req * tm_stp_req,
			struct stp_req * org_stp_req)
{
	s32 ret = 0;
	struct sata_device *tmp_sata_dev = NULL;

	SAL_ASSERT(NULL != tm_stp_req, return ERROR);
	SAL_ASSERT(NULL != org_stp_req, return ERROR);

	tmp_sata_dev = org_stp_req->dev;
	SAL_ASSERT(NULL != tmp_sata_dev, return ERROR);
	tmp_sata_dev->sata_state = SATA_DEV_STATUS_INRECOVERY;

	tm_stp_req->dev = tmp_sata_dev;
	tm_stp_req->ll_msg = tm_stp_req;
	tm_stp_req->org_msg = tm_stp_req;	

	SAS_INIT_LIST_HEAD(&tm_stp_req->io_list);

	if (SAT_IS_NCQ_REQ(org_stp_req->h2d_fis.header.command))
		ret = sat_check_power_mode(tm_stp_req);
	else
		ret = sat_reset_dev(tm_stp_req);

	return ret;
}

void sat_read_log_ext_cb(struct stp_req *sata_req, u32 io_status)
{
	struct sal_card *card = NULL;
	struct sal_dev *dev = NULL;
	struct sal_msg *tmp_msg = NULL;
	s32 ret = 0;
	unsigned long flag = 0;

	SAT_ASSERT(NULL != sata_req, return);

	tmp_msg = (struct sal_msg *)sata_req->upper_msg;

	dev = tmp_msg->dev;
	SAT_ASSERT(NULL != dev, return);

	card = dev->card;
	SAT_ASSERT(NULL != card, return);

	if (SAT_IO_SUCCESS == io_status) {
		SAT_TRACE(OSP_DBL_INFO, 205,
			  "Card:%d is aborting disk:0x%llx io", card->card_no,
			  dev->sas_addr);
		/* abort dev io */
		if (NULL != card->ll_func.eh_abort_op) {
			spin_lock_irqsave(&dev->dev_lock, flag);
			ret =
			    card->ll_func.eh_abort_op(SAL_EH_ABORT_OPT_DEV,
						      dev);
			spin_unlock_irqrestore(&dev->dev_lock, flag);
			if (ERROR == ret)
				SAT_TRACE(OSP_DBL_MAJOR, 5,
					  "card:%d abort dev:0x%llx io failed",
					  card->card_no, dev->sas_addr);
		} else {
			SAT_TRACE(OSP_DBL_MAJOR, 5,
				  "card:%d abort dev msg func is NULL",
				  card->card_no);
		}
	} else {
		SAT_TRACE(OSP_DBL_MAJOR, 5,
			  "card:%d read log ext to dev:0x%llx failed",
			  card->card_no, dev->sas_addr);
	}

	sat_free_int_msg(sata_req);
	return;
}

s32 sat_read_log_ext(struct sata_device * sata_dev)
{
	struct stp_req *new_req = NULL;
	struct sata_h2d *h2d_fis = NULL;
	s32 ret = 0;

	new_req = sat_get_new_msg(sata_dev, 0);
	if (NULL == new_req) {
		SAT_TRACE(OSP_DBL_MAJOR, 5,
			  "Alloc resource for device:0x%llx failed",
			  sata_dev->dev_id);
		return ERROR;
	}

	/*initialize some field of ATA command */
	new_req->dev = sata_dev;
	new_req->ll_msg = new_req;
	new_req->org_msg = new_req;	/* internal IO £¬point to it self */

	h2d_fis = &new_req->h2d_fis;
	memset(h2d_fis, 0, sizeof(struct sata_h2d));

	/* fill in FIS */
	h2d_fis->header.features = 0;
	h2d_fis->header.command = FIS_COMMAND_READ_LOG_EXT;
	h2d_fis->header.fis_type = SATA_FIS_TYPE_H2D;
	h2d_fis->header.pm_port = 0x80;	/* C Bit is set */
	h2d_fis->data.control = 0;
	h2d_fis->data.device = 0;
	h2d_fis->data.features_exp = 0;
	h2d_fis->data.lab_high = 0;
	h2d_fis->data.lba_high_exp = 0;
	h2d_fis->data.lba_low = 0x10;
	h2d_fis->data.lba_low_exp = 0;
	h2d_fis->data.lba_mid = 0;
	h2d_fis->data.lba_mid_exp = 0;
	h2d_fis->data.sector_cnt = 0x01;
	h2d_fis->data.sector_cnt_exp = 0;

	new_req->compl_call_back = sat_read_log_ext_cb;

	ret = sat_send_req_to_dev(new_req);
	if (SAT_IO_SUCCESS != ret) {
		SAT_TRACE(OSP_DBL_MAJOR, 234,
			  "Send read log ext to disk:0x%llx failed",
			  sata_dev->dev_id);

		sat_free_int_msg(new_req);
		return ERROR;
	}

	return OK;
}

void sat_init_trans_array(void)
{
	memset(sat_translation_table, 0, sizeof(sat_translation_table));

	sat_translation_table[SCSIOPC_INQUIRY] = sat_inquiry;
	sat_translation_table[SCSIOPC_TEST_UNIT_READY] = sat_test_unit_ready;
	sat_translation_table[SCSIOPC_READ_CAPACITY_10] = sat_read_capacity_10;
	sat_translation_table[SCSIOPC_READ_CAPACITY_16] = sat_read_capacity_16;
	sat_translation_table[SCSIOPC_MODE_SENSE_6] = sat_mode_sense_6;
	sat_translation_table[SCSIOPC_MODE_SENSE_10] = sat_mode_sense_10;
	sat_translation_table[SCSIOPC_READ_6] = sat_read_write;
	sat_translation_table[SCSIOPC_READ_10] = sat_read_write;
	sat_translation_table[SCSIOPC_READ_12] = sat_read_write;
	sat_translation_table[SCSIOPC_READ_16] = sat_read_write;
	sat_translation_table[SCSIOPC_WRITE_6] = sat_read_write;
	sat_translation_table[SCSIOPC_WRITE_10] = sat_read_write;
	sat_translation_table[SCSIOPC_WRITE_12] = sat_read_write;
	sat_translation_table[SCSIOPC_WRITE_16] = sat_read_write;
	sat_translation_table[SCSIOPC_START_STOP_UNIT] = sat_start_stop_unit;
	sat_translation_table[SCSIOPC_ATA_PASSTHROUGH_12] =
	    sat_ata_pass_through;
	sat_translation_table[SCSIOPC_ATA_PASSTHROUGH_16] =
	    sat_ata_pass_through;
	sat_translation_table[SCSIOPC_SYNCHRONIZE_CACHE_10] = sat_sync_cache;
	sat_translation_table[SCSIOPC_SYNCHRONIZE_CACHE_16] = sat_sync_cache;
	sat_translation_table[SCSIOPC_MODE_SELECT_6] = sat_mode_select;
	sat_translation_table[SCSIOPC_MODE_SELECT_10] = sat_mode_select;
	sat_translation_table[SCSIOPC_LOG_SENSE] = sat_log_sense;
	sat_translation_table[SCSIOPC_REQUEST_SENSE] = NULL;
	sat_translation_table[SCSIOPC_VERIFY_10] = sat_verify;
	sat_translation_table[SCSIOPC_VERIFY_12] = sat_verify;
	sat_translation_table[SCSIOPC_VERIFY_16] = sat_verify;
	sat_translation_table[SCSIOPC_READ_BUFFER] = sat_read_buffer;
	sat_translation_table[SCSIOPC_WRITE_BUFFER] = sat_write_buffer;
	sat_translation_table[SCSIOPC_SEND_DIAGNOSTIC] = sat_send_diagnostic;
	sat_translation_table[SCSIOPC_WRITE_SAME_10] = sat_write_same;
}

