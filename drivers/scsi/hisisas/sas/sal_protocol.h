/*
 * Huawei Pv660/Hi1610 sas protocol related data structure
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

#ifndef __SALPROTOL_H__
#define __SALPROTOL_H__


/* 
 *SAS device type definition. SAS spec(r.7) p206  
*/
#define SAS_NO_DEVICE                    0
#define SAS_END_DEVICE                   1
#define SAS_EDGE_EXPANDER_DEVICE         2
#define SAS_FANOUT_EXPANDER_DEVICE       3
#define SAS_SMP_HEADR_LEN                4

/*sas protocol defined device type*/
#define SAS_DEVTYPE_SATA_MASK            0x01
#define SAS_DEVTYPE_SMP_MASK             0x02
#define SAS_DEVTYPE_STP_MASK             0x04
#define SAS_DEVTYPE_SSP_MASK             0x08
#define SAS_DEVTYPE_SATA_SECELT          0x80


enum sas_device_type {
	NO_DEVICE = 0,
	END_DEVICE,
	EXPANDER_DEVICE,
	VIRTUAL_DEVICE,
	BUTT_DEVICE
};

/*SAS PROTOCOL P.487*/
#define SSP_RSP_DATAPRES_NO_DATA      0
#define SSP_RSP_DATAPRES_RSP_DATA     1
#define SSP_RSP_DATAPRES_SENSE_DATA   2
#define SSP_RSP_DATAPRES_RECV         3


struct sas_identify {
	u8 dev_type;
	/* b7   : reserved */
	/* b6-4 : device type */
	/* b3-0 : address frame type */
	u8 reserved;		/* reserved */
	/* b7-4 : reserved */
	/* b3-0 : reason SAS2 */
	u8 ini_ssp_stp_smp;
	/* b8-4 : reserved */
	/* b3   : SSP initiator port */
	/* b2   : STP initiator port */
	/* b1   : SMP initiator port */
	/* b0   : reserved */
	u8 tgt_ssp_stp_smp;
	/* b8-4 : reserved */
	/* b3   : SSP target port */
	/* b2   : STP target port */
	/* b1   : SMP target port */
	/* b0   : reserved */
	u8 dev_name[8];		/* reserved */

	u8 sas_addr_hi[4];	/* BE SAS address Lo */
	u8 sas_addr_lo[4];	/* BE SAS address Hi */

	u8 phy_iden;
	u8 zpsd_break;
	/* b7-3 : reserved */
	/* b2   : Inside ZPSDS Persistent */
	/* b1   : Requested Inside ZPSDS */
	/* b0   : Break Reply Capable */
	u8 resv[6];		/* reserved */
};

/* TASK information unit starts here */
struct sas_ssp_tm_info_unit {
	u8 lun[8];		/* 0x18 */
	u16 reserved4;		/* 0x20 */
	u8 mgmt_task_function;	/* 0x22 */
	u8 reserved5;		/* 0x23 */
	u16 mgmt_task_tag;	/* 0x24 */
	u16 reserved6;		/* 0x26 */
	u32 reserved7;		/* 0x28 */
	u32 reserved8;		/* 0x2C */
	u32 reserved9;		/* 0x30 */
};

#define DEFAULT_LUN_SIZE	(8)
#define DEFAULT_CDB_LEN		(16)

struct sas_ssp_open_frame {
	/*byte 0 */
	u8 frame_ctrl;
	/*byte 1 */
	u8 rate_ctrl;
	/*byte 2,3 */
	u8 ini_conn_tag[2];
	/*byte 4-11 */
	u8 dest_sas_addr[8];
	/*byte 12-19 */
	u8 src_sas_addr[8];
	/*byte 20 */
	u8 src_zone_grp;
	/*byte 21 */
	u8 path_way_blk_cnt;
	/*byte 22,23 */
	u8 awt[2];
	/*byte 24-27 */
	u8 compatible_feature[4];
};

struct sas_ssp_frame_header {
	/*byte 0 - 9 */
	u8 frame_type;
	u8 hash_dest_sas_addr[3];
	u8 rsv1;
	u8 hash_src_sas_addr[3];
	u8 rsv2[2];
	/*byte 10 */
	u8 byte10;		
	u8 byte11;		

	/*byte 12-23 */
	u8 reserved4[4];
	u16 ini_port_tag;
	u16 tgt_port_tag;
	u32 data_offset;

};

struct sas_ssp_xfer_rdy_iu {
	u32 data_offset;
	u32 data_len;
	u8 rsv1[4];
};

struct sas_ssp_cmnd_uint {
	u8 lun[DEFAULT_LUN_SIZE];	/* SCSI Logical Unit Number */
	u8 reserved1;		/* reserved */
	u8 task_attr;
	/* B7   : enabledFirstBurst */
	/* B6-3 : taskPriority */
	/* B2-0 : taskAttribute */
	u8 reserved2;		/* reserved */
	u8 add_cdb_len;
	/* B7-2 : additionalCdbLen */
	/* B1-0 : reserved */
	u8 cdb[DEFAULT_CDB_LEN];	/* The SCSI CDB up to 16 bytes length */
};


/* SCSI Sense data */
struct sal_sense_payload {
	u8 resp_code;
	u8 sns_segment;
	u8 sense_key;		/* sense key                                */
	u8 info[4];
	u8 add_sense_len;	/* 11 always                                */
	u8 cmd_specific[4];
	u8 add_sense_code;	/* additional sense code                    */
	u8 sense_qual;		/* additional sense code qualifier          */
	u8 fru;
	u8 key_specific[7];
};

#define SAL_IS_MODE_CMND(cdb0)  ((MODE_SELECT == (cdb0)) \
								||(MODE_SENSE == (cdb0))	\
								||(MODE_SENSE_10 == (cdb0)))


#endif
