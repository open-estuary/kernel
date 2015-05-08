/*
 * Huawei Pv660/Hi1610 sata protocol common data structure
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

#ifndef __SATCOMMON_H__
#define __SATCOMMON_H__

/*device status definition,normal means we can issus command to device freely;
 *in recovery means the device is currently not able to process new commands,
 *and the new comming commands should be add to waiting list or return failed to
 *upper layer*/
#define SATA_DEV_STATUS_NORMAL       0x1	/*normal state */
#define SATA_DEV_STATUS_INRECOVERY  0x2	/*in recovery state */

#define SATA_DEV_IDENTIFY_VALID      0x1
#define SATA_DEV_IDENTIFY_NOT_VALID  0x2

struct stp_req;
/*chip/vendor specific functions,this field is located in SATA device struct,it is
 *chip/vendor specific*/
struct chip_sat_intf {
	/*chip type */
	s32 type;

	/*send fis to chip */
	 s32(*send_fis) (struct stp_req * stp_request);
};

/*call back function for ATA command*/
typedef void (*sat_compl_cbfn_t) (struct stp_req * stp_request, u32 io_status);

/*SATA device definiton*/
struct sata_device {
	SAS_SPINLOCK_T sata_lock;
	s8 diag_pending;	/*is there any background diagnostic pending */
	u8 sata_max_lba[8];	/* MAXLBA is from read capacity */
	u32 dev_cap;		/*device capability,e.g,DMA support NCQ support etc */
	u32 dev_transfer_mode;	/*Transfer mode */
	u32 iden_valid;		/*whether identify information is valid */
	u16 pending_io;		/*number of pending IOs in the target */
	u16 pending_ncq_io;	/*number of pending NCQ IOs in the target */
	u16 pending_non_ncq_io;	/*number of pending non-NCQ IOs in the target */
	u16 queue_depth;	/*Queue depth of current device,get this from identify information */
	u32 sata_state;		/*current device state,e.g in-recovery,normal */
	u32 ncq_tag_bitmap;	/*NCQ tag bit map */
	struct sata_identify_data sata_identify;	/*identify information of current device */

	/*Following field should be intialized by upper layer */
	void *upper_dev;	/*point to upper layer device struct */
	u64 dev_id;		/*device identification(addr) */
	u32 chip_type;		/*chip type,use to fill in struct chip_sat_intf,which is chip
				   *specific functions */
	u32 ncq_enable;		/*NCQ enable or disable */
};

struct sat_ini_cmnd {
	u8 tag;
	u64 unique_id;		
	u64 upper_cmd;		
	u64 port_id;

	u8 cdb[DRV_SCSI_CDB_LEN];	
	u32 data_len;		
	u8 data_dir;		
	u8 lun[DRV_SCSI_LUN_LEN];		

	void *sgl;		

	void (*done) (struct drv_ini_cmd *);

};

struct stp_req {
	struct list_head io_list;	
	struct sata_h2d h2d_fis;	/*H2D fis send to SATA device */
	struct sata_d2h d2h_fis;	/*D2H fis offered by SATA device(on IO completition) */
	u32 d2h_valid;		/*whether D2H information is valid */
	union sns_data *sense;	/*ponit to sense data of current SCSI command */
	struct sat_ini_cmnd scsi_cmnd;	/*SCSI/BDM command */
	struct sata_device *dev;	/*which device this command belongs to */
	struct stp_req *ll_msg;	/*current executing comand(for traceing purpose) */
	struct stp_req *org_msg;	/*upper layer command(for traceing purpose) */
	u8 ncq_tag;		/*NCQ tag of current,used only for NCQ commands */
	u32 curr_lba;		/* current LBA for read and write */
	u32 ata_cmd;		/* ATA command */
	u32 org_transfer_len;	/* original tranfer length(tl) */
	sat_compl_cbfn_t compl_call_back;

	/*Following field should be intialized by upper layer */
	void *upper_msg;	/*point to upper layer message struct(struct sal_msg) */

	/*Inner SGL alloc */
	unsigned long indirect_rsp_phy_addr;
	void *indirect_rsp_virt_addr;
	u32 indirect_rsp_len;
	u32 sgl_cnt;
};


#define SAT_TRACE_BASE_ID           5000	/*tarce base id of SAT module */

#define SAT_TRACE(dbg_lev,id,x,args...)                                   	\
do \
{ \
	if(unlikely(sat_dbg_level < dbg_lev))                                   	\
	{                                                                           \
	    printk(KERN_WARNING "[%d][%s,%d][%lu]" x "\n",id,__FUNCTION__,__LINE__,\
	    jiffies, ## args);                                                       \
	}                                                                           \
	else                                                                        \
	{                                                                           \
		SAL_TRACE(dbg_lev,(id+SAT_TRACE_BASE_ID),x,## args);           			\
	} \
} while(0)

#define SAT_TRACE_LIMIT(debug_level,interval,count,id,X,args...) \
do \
{ \
     static unsigned long last = 0; \
     static unsigned long local_count = 0; \
     if(local_count<count)	\
     {	\
	 	local_count++;	\
		SAL_TRACE(debug_level,id, X,## args);\
	 }	\
     if ( time_after_eq(jiffies,last+(interval))) \
     { \
       last = jiffies; \
       local_count = 0;	\
     } \
} while(0)


#ifdef SAT_DEBUG_TRACE_FUNC
#define SAT_TRACE_FUNC_ENTER                                                \
        printk(KERN_WARNING"enter function [%s,%d]",__FUNCTION__,__LINE__)
#define SAT_TRACE_FUNC_EXIT                                                \
        printk(KERN_WARNING"exit function [%s,%d]",__FUNCTION__,__LINE__)
#else
#define SAT_TRACE_FUNC_ENTER
#define SAT_TRACE_FUNC_EXIT
#endif


#define SAT_ASSERT(expr, args...) \
    do \
    {\
        if (unlikely(!(expr)))\
        {\
            printk(KERN_EMERG "BUG! (%s,%s,%d)%s (called from %p)\n", \
                   __FILE__, __FUNCTION__, __LINE__, \
                   # expr, __builtin_return_address(0)); \
            SAS_DUMP_STACK();   \
            args; \
        } \
    } while (0)


/*SAT IO status definition,they are return values of SAT send function*/
#define SAT_IO_SUCCESS 0x0	/*IO successfuly issued to LL layer,upper
				   *layer should return success to SCSI midlayer */
#define SAT_IO_FAILED  0x1	/*IO issue failed,upper layer should return 
				   *failed to upper layer */
#define SAT_IO_BUSY    0x2	/*BUSY,upper layer should retry this command */
#define SAT_IO_COMPLETE       0x3	/*IO execute complete */
#define SAT_IO_QUEUE_FULL        0x4	/*SAT can't send more commands to this device */

/*
 * seems that driver byte and is never used in scsi mid layer,so only field we 
 * need to fill is host byte and status byte
 */
#define SAT_SET_CMD_RESULT(result,host_byte,status_byte,driver_byte)                \
        (result |= (status_byte)|((host_byte)<<16)|((driver_byte)<<24))

#define SAT_GET_SCSI_COMMAND(scsi_cmnd)          (scsi_cmnd->cdb)
#define SAT_GET_SCSI_DIRECTION(scsi_cmnd)         (scsi_cmnd->data_dir)
#define SAT_GET_SCSI_DATA_BUFF_LEN(scsi_cmnd)      (scsi_cmnd->data_len)

#define SAT_SET_DEFAULT_VALUE(judge,toset,true_value,false_value) \
    ((judge) ?((toset)=(true_value)):((toset)=(false_value)))


#define SAT_SG_COPY_FROM_BUFFER(cmd,buf,len) 0
#define SAT_SG_COPY_TO_BUFFER(cmd,buf,len)  0

#define SAT_SET_SCSI_RESIDE(scsi_cmnd,reside) \
{ \
	scsi_cmnd->remain_data_len = reside;\
}

#define SAT_D2H_INVALID 0
#define SAT_D2H_VALID   1

#define LV_TEST     printk("=====%s,%d=======\n",__FUNCTION__,__LINE__);

extern void sat_disk_exec_complete(struct stp_req *stp_request, u32 io_status);
#endif
