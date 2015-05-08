/*
 * Huawei Pv660/Hi1610 sas controller msg data structure define
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

#ifndef _SAL_MSG_H_
#define _SAL_MSG_H_

enum sal_msg_state {
	SAL_MSG_ST_FREE = 0,	
	SAL_MSG_ST_ACTIVE,	
	SAL_MSG_ST_SEND,	
	SAL_MSG_ST_DONE_CALLED,	
	SAL_MSG_ST_IN_JAM,	
	SAL_MSG_ST_HELLO_WORLD = 5,	
	SAL_MSG_ST_ABORTING,	
	SAL_MSG_ST_ABORTED,
	SAL_MSG_ST_TMO,		
	SAL_MSG_ST_IN_ERRHANDING = 9,	
	SAL_MSG_ST_PENDING_IN_DRV,	
	SAL_MSG_ST_BUTT
};

enum sal_msg_event {
	SAL_MSG_EVENT_MSG_GET = 0,	
	SAL_MSG_EVENT_DONE_MID,	
	SAL_MSG_EVENT_IOSTART,
	SAL_MSG_EVENT_DONE_UNUSE,	
	SAL_MSG_EVENT_SEND_FAIL,	
	SAL_MSG_EVENT_CHIP_REMOVE = 5,	
	SAL_MSG_EVENT_LL_RTY_HAND_OVER,	
	SAL_MSG_EVENT_HDRST_RTY_HAND_OVER,	
	SAL_MSG_EVENT_NEED_JAM,	
	SAL_MSG_EVENT_ADD_JAM_FAIL,	
	SAL_MSG_EVENT_ABORT_START = 10,	
	SAL_MSG_EVENT_NEED_ERR_HANDLE = 15,	
	SAL_MSG_EVENT_CANCEL_ERR_HANDLE,	
	SAL_MSG_EVENT_JAM_FAIL,	
	SAL_MSG_EVENT_WAIT_OK,	
	SAL_MSG_EVENT_WAIT_FAIL,	
	SAL_MSG_EVENT_RTY_START,	
	SAL_MSG_EVENT_CHIP_RESET_DONE = 20,

	SAL_MSG_EVENT_SMP_START = 25,	
	SAL_MSG_EVENT_SMP_DONE,	
	SAL_MSG_EVENT_SMP_SEND_FAIL,	
	SAL_MSG_EVENT_SMP_WAIT_OK,	
	SAL_MSG_EVENT_SMP_WAIT_FAIL,	

	SAL_MSG_EVENT_BUTT
};

enum sal_msg_type {
	SAL_MSG_TYPE_EXT_IO = 1,	
	SAL_MSG_TYPE_INT_IO,	
	SAL_MSG_TYPE_BUTT_IO
};

#define SAL_MSG_UNIID(msg)	((msg)->ini_cmnd.unique_id)

struct sal_ini_cmnd {
	u16 tag;
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

struct sal_msg {
	struct list_head host_list;	
	u32 msg_htag;		
	SAS_SPINLOCK_T msg_state_lock;	
	enum sal_msg_state msg_state;
	struct sal_card *card;
	struct sal_dev *dev;	

	SAS_STRUCT_COMPLETION compl;	


	u16 ll_tag;		


	struct sal_ini_cmnd ini_cmnd;


	enum sal_msg_data_direction data_direction;	


	unsigned long send_time;	


	void *related_msg;	
	bool tag_not_found;
	bool transmit_mark;
	enum sal_tm_type tm_func;	

	void (*done) (struct sal_msg *);

	enum sal_msg_protocol type;
	union sal_protocol protocol;



	struct sal_msg_status status;	
	enum sal_msg_type io_type;

	struct sal_err_handle eh;

	struct drv_sgl sgl_head;
};

typedef s32(*sal_msg_st_chg_action_t) (struct sal_msg *);

struct sal_msg_fsm {
	enum sal_msg_state new_status;	
	sal_msg_st_chg_action_t act_func;	
};

extern void sal_msg_fsm_init(void);
extern s32 sal_msg_rsc_init(struct sal_card *sal_card);
extern s32 sal_msg_rsc_remove(struct sal_card *sal_card);
extern s32 sal_msg_state_chg(struct sal_msg *msg, enum sal_msg_event event);
extern s32 sal_msg_state_chg_no_lock(struct sal_msg *msg,
				     enum sal_msg_event event);

extern struct sal_msg *sal_get_msg(struct sal_card *sal_card);
extern s32 sal_free_msg(struct sal_msg *msg);
extern s32 sal_drv_done_msg(struct sal_msg *msg);
extern enum sal_msg_state sal_get_msg_state(struct sal_msg *msg);
extern void sal_msg_copy_from_ini_cmnd(struct sal_msg *msg,
				       struct drv_ini_cmd * ini_cmnd);
extern void sal_msg_copy_to_ini_cmnd(struct sal_msg *msg,
				     struct drv_ini_cmd * ini_cmnd);
extern s32 sal_wait_msg_state(struct sal_msg *orig_msg, u32 wait_time_ms,
			      enum sal_msg_state wait_state);

#endif
