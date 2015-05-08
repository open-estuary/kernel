/*
 * Huawei Pv660/Hi1610 sas controller driver interface
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

#ifndef __SALINTF_H__
#define __SALINTF_H__

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/errno.h>
#include <linux/kdev_t.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_eh.h>
#include <scsi/scsi_tcq.h>

#include "sal_types.h"
#include "sal_osintf.h"
#include "sal_protocol.h"
#include "sat_intf.h"


#define SAL_DEVSLOWIO_CNT 100
#define SAL_SLOWCIRCLE_NUM 10
#define SAL_MAX_DELDISK_NUM 2

#define SAL_MAX_DELAY_TIME 20	


#define SAL_GET_ALL_ENTRY_CNT(sgl)      (sgl)->num_sges_in_chain
#define SAL_SGL_CNT(sgl)				(sgl)->num_sges_in_sgl
#define SAL_GET_SGL_ENTRY(sgl,num)		&((sgl)->entrys[num])
#define SAL_GET_ENTRY_BUF(entry)		PRODUCT_REQ_GET_ENTRY_BUF(entry)

/* SCSI Task Attribute */
enum sal_io_task_attr {
	SAL_IO_TASK_SIMPLE = 0,
	SAL_IO_TASK_HEAD_OF_QUEUE,
	SAL_IO_TASK_ORDERED,
	SAL_IO_RESERVER,
	SAL_IO_TASK_ACA,
	SAL_IO_TASK_BUTT
};

enum sal_tm_type {
	SAL_TM_ABORT_TASK = 0x01,
	SAL_TM_ABORT_TASKSET = 0x02,
	SAL_TM_CLEAR_TASKSET = 0x04,
	SAL_TM_LUN_RESET = 0x08,
	SAL_TM_IT_NEXUS_RESET = 0x10,
	SAL_TM_CLEAR_ACA = 0x40,
	SAL_TM_QUERY_TASK = 0x80,
	SAL_TM_QUERY_TASK_SET = 0x81,
	SAL_TM_QUERY_UNIT_ATTENTION = 0x82,
	SAL_TM_TYPE_BUTT = 0xFF
};

#define SAL_TM_FUNCTION_COMPLETE            0x0
#define SAL_TM_INVALID_FRAME                 0x2
#define SAL_TM_FUNCTION_NOT_SUPPORTED       0x4
#define SAL_TM_FUNCTION_FAILED               0x5
#define SAL_TM_FUNCTION_SUCCEEDED           0x8
#define SAL_TM_INCORRECT_LOGICAL_UNIT       0x9
#define SAL_TM_OVERLAPPED_TAG               0xA


#define SAL_MSG_ACTIVE          (1 << 0)/* normal */
#define SAL_MSG_ABORTING        (1 << 1)/*error handling in SCSI mid layer */
#define SAL_MSG_ABORTED         (1 << 2)/*IO status after SCSI error handle */
#define SAL_MSG_TIMEOUT         (1 << 3)/*IO time out */
#define SAL_MSG_DEV_ABNORMAL    (1 << 4)/*device abnormal, can not done this IO to mid layer */
#define SAL_MSG_ERRHANDLING     (1 << 5)/*internal error handing */
#define SAL_MSG_ABORT_FINISH    (1 << 6)/*SCSI error handle finish,TM response(tag not found) */

enum sal_chip_op_type {
	SAL_CHIP_OPT_SOFTRESET,
	SAL_CHIP_OPT_ROUTE_CHECK,
	SAL_CHIP_OPT_CLOCK_CHECK,
	SAL_CHIP_OPT_LOOPBACK,
	SAL_CHIP_OPT_DUMP_LOG,
	SAL_CHIP_OPT_UPDATE,
	SAL_CHIP_OPT_GET_VERSION,
	SAL_CHIP_OPT_INTR_COAL,
	SAL_CHIP_OPT_POWER_DOWN,
	SAL_CHIP_OPT_PCIE_LOOPBACK,
	SAL_CHIP_OPT_MEM_TEST,
	SAL_CHIP_OPT_REQ_TIMEOUTCHECK
};

#define SAL_MML_PHY_CLOSE 	(0)
#define SAL_MML_PHY_OPEN	(1)
#define SAL_MML_PHY_RESET 	(2)
#define SAL_MML_PHY_QUERY 	(3)

enum sal_phy_op_type {
	SAL_PHY_OPT_CLOSE = 0,
	SAL_PHY_OPT_OPEN = 1,
	SAL_PHY_OPT_RESET = 2,
	SAL_PHY_OPT_GET_LINK = 3,	/* sascbb switchphy cardid phy_id <0:off 1:on 2: reset 3:getstate> */
	SAL_PHY_OPT_LOCAL_CONTROL,
	SAL_PHY_OPT_QUERY_ERRCODE,
	SAL_PHY_OPT_EMPHASIS_AMPLITUDE_OPER
};


enum sal_port_op_type {
	SAL_PORT_OPT_LOCAL_CONTROL = 0,
	SAL_PORT_OPT_DELETE,
	SAL_PORT_OPT_ALL_START_WORK,
	SAL_PORT_OPT_LED,
	SAL_PORT_OPT_REF_INC_OR_DEC,
	SAL_PORT_OPT_DETECT_CABLE_ONOFF,	/* check cable or fiber whether on port */
	SAL_PORT_OPT_BUTT
};


enum sal_port_op_type_by_cmd {
	SAL_PORT_OPT_BY_CMD_CLOSE = 0,
	SAL_PORT_OPT_BY_CMD_OPEN = 1,
	SAL_PORT_OPT_BY_CMD_RESET = 2,
	SAL_PORT_OPT_BY_CMD_GET_STATE = 3,
	SAL_PORT_OPT_BY_CMD_BUTT
};

enum sal_dev_op_type {
	SAL_DEV_OPT_REG,
	SAL_DEV_OPT_DEREG
};


enum sal_chip_update {
	SAL_UPDATE_EEPROM,
	SAL_UPDATE_FIRMWARE,
	SAL_UPDATE_ILA,
	SAL_UPDATE_ALL
};


#define SAL_CARD_INIT_PROCESS               0x0001	
#define SAL_CARD_ACTIVE                     0x0002	
#define SAL_CARD_REMOVE_PROCESS             0x0004	
#define SAL_CARD_RESETTING                  0x0008	
#define SAL_CARD_LOOPBACK                   0x0010	
#define SAL_CARD_FW_UPDATE_PROCESS          0x0020	
#define SAL_CARD_EEPROM_UPDATE_PROCESS      0x0040	
#define SAL_CARD_FATAL			            0x0100	
#define SAL_CARD_SLOWNESS					0x0200	

#define SAL_MAX_PHY_NUM     	32
#define SAL_MAX_CARD_NUM       	16	
#define SAL_IOSTAT_CNT_MAX      12

#define SAL_MAX_FRAME_SCSI_NUM  200	/*frame maximal scsi id */

#define SAL_MAX_CHIP_VER_STR_LEN 50

#define SAL_MAX_DEV_NUM     4096

#define SAL_MAX_EVENT_NUM   SAL_MAX_DEV_NUM

#define SAL_MAX_MSG_NUM     8192


#define SAL_MAX_WIRE_IN_OUT_NUM   128

#define SAL_MAX_SENSE_LEN   96	
#define SAL_MAX_CDB_LEN     16	

#define SAL_MAX_FIRMWARE_LEN      32	

/*
 *the number of periods of disable phy
*/
#define SAL_MAX_PHY_ERR_PERIOD             10


#define SAL_PHY_NEED_RESET_FOR_DFR 		0
#define SAL_PHY_NO_RESET_FOR_DFR 		1


#define SAL_NO_PHY_UP_IN_PORT  			0
#define SAL_PHY_UP_IN_PORT  			1


#define SAL_ALL_PORT_LED_NEED_OPT 1


#define SAL_GET_SASADDR_FROM_IDENTIFY(identify) \
                ((((u64)identify->sas_addr_hi[0]) << 56)| \
                 (((u64)identify->sas_addr_hi[1]) << 48)| \
                 (((u64)identify->sas_addr_hi[2]) << 40)| \
                 (((u64)identify->sas_addr_hi[3]) << 32)| \
                 (((u64)identify->sas_addr_lo[0]) << 24)| \
                 (((u64)identify->sas_addr_lo[1]) << 16)| \
                 (((u64)identify->sas_addr_lo[2]) << 8)| \
                 ((identify)->sas_addr_lo[3]))

/*
 *device type
*/
enum sal_dev_type {
	SAL_DEV_TYPE_SSP = 1,	/*sas disk ,also include ses on disk array */
	SAL_DEV_TYPE_STP,	/*sata disk */
	SAL_DEV_TYPE_SMP,	/*smp device -> expander device */
	SAL_DEV_TYPE_BUTT
};

enum sal_phy_config_state {
	SAL_PHY_CONFIG_OPEN,	
	SAL_PHY_CONFIG_CLOSE,	
	SAL_PHY_INSULATE_CLOSE,	
	SAL_PHY_CONFIG_BUTT
};

/*
 *phy state machine definition
*/
enum sal_phy_status {
	SAL_PHY_LINK_DOWN = 0,	
	SAL_PHY_LINK_UP,	
	SAL_PHY_CLOSED,		
	SAL_PHY_LINK_BUTT
};

enum sal_wire_type {
	SAL_ELEC_CABLE = 0,	
	SAL_OPTICAL_CABLE,
	SAL_IDLE_WIRE,	
	SAL_WIRE_TYPE_BUTT
};

enum sal_cable_status {
	SAL_CABLE_IS_NOT_IN_PORT = 0,
	SAL_CABLE_IS_IN_PORT,	
	SAL_CABLE_IN_PORT_BUTT
};

/*
 *define hardware event 
*/
enum sal_phy_hw_event_type {
	SAL_PHY_EVENT_DOWN = 1,	/*phy down event */
	SAL_PHY_EVENT_UP,	/*phy up event */
	SAL_PHY_EVENT_BCAST,	/*phy broadcast event */
	SAL_PHY_EVENT_STOP_RSP,	/* stop phy response event */
	SAL_PHY_EVENT_BUTT
};

/*
 *link rate definition
*/
enum sal_link_rate {
	SAL_LINK_RATE_FREE = 0,	
	SAL_LINK_RATE_1_5G,	
	SAL_LINK_RATE_3_0G,	
	SAL_LINK_RATE_6_0G,	
	SAL_LINK_RATE_12_0G,	
	SAL_LINK_RATE_BUTT
};

/*
 *port state machine definition
*/
enum sal_port_status {
	SAL_PORT_FREE = 0,	
	SAL_PORT_LINK_UP,	
	SAL_PORT_LINK_DOWN,	
	SAL_PORT_LINK_BUTT
};

/*
 *link mode definition
*/
enum sal_link_mode {
	SAL_MODE_DIRECT = 1,	/* linked to end device */
	SAL_MODE_INDIRECT,	/* linked to expander device */
	SAL_MODE_BUTT
};


enum sal_port_mode {
	SAL_PORT_MODE_TGT = 1, 
	SAL_PORT_MODE_INI,     
	SAL_PORT_MODE_BOTH,   
	SAL_PORT_MODE_BUTT
};

enum sal_dev_state {
	SAL_DEV_ST_FREE = 0,	
	SAL_DEV_ST_INIT,	
	SAL_DEV_ST_IN_REG,
	SAL_DEV_ST_REG,		
	SAL_DEV_ST_ACTIVE,	
	SAL_DEV_ST_IN_REMOVE,	
	SAL_DEV_ST_IN_DEREG,	
	SAL_DEV_ST_FLASH,	
	SAL_DEV_ST_RDY_FREE,	
	SAL_DEV_ST_BUTT		
};

enum sal_dev_event {
	SAL_DEV_EVENT_SMP_FIND = 0,	
	SAL_DEV_EVENT_FIND,	
	SAL_DEV_EVENT_MISS,
	SAL_DEV_EVENT_REG,
	SAL_DEV_EVENT_DEREG,	
	SAL_DEV_EVENT_REG_OK = 5,
	SAL_DEV_EVENT_REG_FAILED,	
	SAL_DEV_EVENT_DEREG_OK,
	SAL_DEV_EVENT_DEREG_FAILED,
	SAL_DEV_EVENT_SMP_REG_OK,	
	SAL_DEV_EVENT_REPORT_OK,
	SAL_DEV_EVENT_REPORT_FAILED,
	SAL_DEV_EVENT_REPORT_OUT,
	SAL_DEV_EVENT_REF_CLR,
	SAL_DEV_EVENT_BUTT
};

enum sal_dif_block_size {
	SAL_DIF_BLOCK_SIZE_512B = 0,
	SAL_DIF_BLOCK_SIZE_520B,
	SAL_DIF_BLOCK_SIZE_4096B,
	SAL_DIF_BLOCK_SIZE_4160B,
	SAL_DIF_BLOCK_SIZE_BUTT
};

enum sal_dif_action {
	SAL_DIF_ACTION_INSERT = 0,
	SAL_DIF_ACTION_VERIY_AND_FORWARD,
	SAL_DIF_ACTION_VERIY_AND_DELETE,
	SAL_DIF_ACTION_VERIY_AND_REPLACE,
	SAL_DIF_ACTION_VERIYUDT_REPLACE_CRC,
	SAL_DIF_ACTION_REPLACEUDT_REPLACE_CRC,
	SAL_DIF_ACTION_BUTT
};

enum sal_eh_abort_op_type {
	SAL_EH_ABORT_OPT_SINGLE,
	SAL_EH_ABORT_OPT_DEV
};

enum sal_eh_dev_rst_type {
	SAL_EH_DEV_HARD_RESET = 10,
	SAL_EH_DEV_LINK_RESET,
	SAL_EH_RESET_BY_DEVTYPE,	
	SAL_EH_RESET_BUTT
};

/*
 *command execute result
*/
enum sal_cmnd_exec_result {
	SAL_CMND_EXEC_SUCCESS = 0,	/*success */
	SAL_CMND_EXEC_FAILED,	/*failed */
	SAL_CMND_EXEC_BUSY,	/*busy retry need */
	SAL_CMND_EXEC_BUTT
};

/* Protocol Type */
enum sal_msg_protocol {
	SAL_MSG_PROT_SSP = 0,
	SAL_MSG_PROT_STP,
	SAL_MSG_PROT_SMP,
	SAL_MSG_PROT_SSP_DIF,
	SAL_MSG_BUTT
};

enum sal_msg_data_direction {
	SAL_MSG_DATA_BIDIRECTIONAL,
	SAL_MSG_DATA_TO_DEVICE = 1,	
	SAL_MSG_DATA_FROM_DEVICE,
	SAL_MSG_DATA_NONE,	
	SAL_MSG_DATA_BUTT
};

enum sal_msg_drv_resp {
	SAL_MSG_DRV_SUCCESS = 0x800,
	SAL_MSG_DRV_NO_DEV,
	SAL_MSG_DRV_UNDERFLOW,
	SAL_MSG_DRV_FAILED,
	SAL_MSG_DRV_RETRY,
	SAL_MSG_DRV_BUTT
};

enum sal_eh_pre_abort {

	SAL_PRE_ABORT_NOT_FOUND = 1,
	SAL_PRE_ABORT_DONE,
	SAL_PRE_ABORT_FAIL,
	SAL_PRE_ABORT_PASS,
	SAL_PRE_ABORT_BUTT	
};

enum sal_common_op_type {
	SAL_COMMON_OPT_SHOW_IO = 1,
	SAL_COMMON_OPT_DEBUG_LEVEL,
	SAL_COMMON_OPT_GET_SFP_INFO,
	SAL_COMMON_OPT_READ_WIRE_EEPROM,
	SAL_COMMON_OPT_GETLL_RSC,	
	SAL_COMMON_OPT_INQUIRY_CMDTX_STATUS,
	SAL_COMMON_OPT_BUTT
};

enum sal_reg_op_type {
	SAL_REG_OPT_READ,
	SAL_REG_OPT_WRITE,
	SAL_REG_PCIE_OPT_READ,
	SAL_REG_PCIE_OPT_WRITE,
	SAL_REG_OPT_BUTT
};

enum sal_bitstream_op_type {
	SAL_BITSTREAM_OPT_START,
	SAL_BITSTREAM_OPT_END,
	SAL_BITSTREAM_OPT_RX_ENABLE,
	SAL_BITSTREAM_OPT_TX_ENABLE,
	SAL_BITSTREAM_OPT_QUERY,
	SAL_BITSTREAM_OPT_CLEAR,
	SAL_BITSTREAM_OPT_INSERT,
	SAL_BITSTREAM_OPT_BUTT
};

enum sal_led_op_type {
	SAL_LED_LOW_SPEED = 1,
	SAL_LED_HIGH_SPEED,
	SAL_LED_FAULT,
	SAL_LED_OFF,
	SAL_LED_ALL_OFF,
	SAL_LED_RESTORE,
	SAL_LED_BUTT
};

enum sal_gpio_op_type {
	SAL_GPIO_WRITE = 1,
	SAL_GPIO_READ,
	SAL_GPIO_EVENT_SETUP,
	SAL_GPIO_BUTT
};

#define SAL_LAST_TEN_IO_STAT 	10

#define SAL_SMP_DIRECT_PAYLOAD 	48

#define SAL_MAX_DEV_QUEUEDEPTH  16

enum sal_io_direction {
	SAL_QUEUE_FROM_SCSI,
	SAL_RESPONSE_TO_SCSI,
	SAL_EHABORT_FREOM_SCSI
};

#define SAL_MAX_MSG_LIST_NUM 16

enum sal_disc_type {
	SAL_INI_STEP_DISC = 0,
	SAL_INI_FULL_DISC,
	SAL_INI_DISC_BUTT
};

enum sal_refcnt_op_type {
	SAL_REFCOUNT_INC_OPT,	
	SAL_REFCOUNT_DEC_OPT,	
	SAL_REFCOUNT_BUTT
};

enum sal_exec_cmpl_status {
	SAL_EXEC_SUCCESS = 0,
	SAL_EXEC_FAILED,
	SAL_EXEC_RETRY
};

struct sal_gpio_op_param {
	u32 phy_id;
	u32 val;
	u32 mask;
	u32 gpio_rsp_val;
};


struct sal_led_op_param {
	enum sal_led_op_type port_speed_led;	
	u32 phy_id;		
	enum sal_led_op_type all_port_speed_led[SAL_MAX_PHY_NUM];
	u32 all_port_op;	
};


struct sal_msg_status {
	enum sal_msg_drv_resp drv_resp;
	u32 io_resp;		

	s32 result;		
	u32 res_id;
	u32 sense_len;		
	u8 sense[SAL_MAX_SENSE_LEN];

};

/* SMP IO */
struct sal_msg_smp {

	u8 smp_payload[SAL_SMP_DIRECT_PAYLOAD];
	u32 smp_len;
	u8 *rsp_virt_addr;
	u32 rsp_len;

	/*SMP Buffer used for Indirect SMP */
	void *indirect_req_phy_addr;
	void *indirect_req_virt_addr;
	u32 indirect_req_len;
	void *indirect_rsp_phy_addr;
	void *indirect_rsp_virt_addr;
	u32 indirect_rsp_len;

};

union sal_protocol {
	struct sas_ssp_cmnd_uint ssp;
	struct sal_msg_smp smp;
	struct stp_req stp;
};


struct sal_dif_ctl {
	u32 dif_switch;		
	enum sal_dif_block_size block_size;	
	enum sal_dif_action dif_action;	
};

#define SAL_DISK_ERR_PRINT_TIME     			(10)	

struct sal_ll_io_err_print_ctrl {
	unsigned long last_printf_jiff;
	u32 last_ll_err_code;	
	u32 print_cnt;		
};

struct sal_config {
	u8 card_pos;
	u32 dev_miss_dly_time;
	u32 work_mode;
	u32 port_tmo;
	u32 smp_tmo;
	u32 jam_tmo;
	u32 num_inbound_queues;
	u32 num_outbound_queues;
	u32 ib_queue_num_elems;
	u32 ib_queue_elem_size;
	u32 ob_queue_num_elems;
	u32 ob_queue_elem_size;
	u32 dev_queue_depth;
	u32 max_targets;
	u32 max_can_queues;
	u32 max_active_io;
	u32 hw_int_coal_timer;
	u32 hw_int_coal_control;
	u32 ncq_switch;		/* SATA_NCQ_switch */
	/*DIF option */
	u32 dif_switch;
	u32 dif_blk_size;
	u32 dif_action;

	u32 phy_link_rate[SAL_MAX_PHY_NUM];
	u32 phy_addr_high[SAL_MAX_PHY_NUM];
	u32 phy_addr_low[SAL_MAX_PHY_NUM];
	u32 port_bitmap[SAL_MAX_PHY_NUM];
	u32 phy_g3_cfg[SAL_MAX_PHY_NUM];
	u32 port_logic_id[SAL_MAX_PHY_NUM];
	u32 bit_err_stop_threshold;
	u32 bit_err_interval_threshold;
	u32 bit_err_routine_time;
	u32 bit_err_routine_cnt;
	/* BCT */
	bool bct_enable;
};

/*
 *bit error information
*/
struct sal_bit_err {
	u32 bit_err_enable;	

	SAS_SPINLOCK_T err_code_lock;
	u32 phy_err_inv_dw;
	u32 phy_err_disp_err;	
	u32 phy_err_cod_viol;
	u32 phy_err_loss_dw_syn;

	u32 inv_dw_chg_cnt;	/*invalid dword count per period */
	u32 disp_err_chg_cnt;	/*disparity error count per period */
	u32 loss_dw_syn_chg_cnt;	/*loss dword synch count per period */
	u32 cod_vio_chg_cnt;	/*code violation count per period */
	u32 phy_chg_cnt_per_period;	/*phy change count per period */

	u32 invalid_dw_cnt;	/*invalid dword count */
	u32 disparity_err_cnt;	/*disparity error count */
	u32 loss_dw_synch_cnt;	/*loss dword synch count */
	u32 code_viol_cnt;	/*code violation count */
	u32 phy_chg_cnt;	/*phy change count */
	unsigned long last_chk_jiff;	

	unsigned long last_switch_jiff;	
	u32 phy_err_pos;	/*position of phy error */
	u32 phy_warn_pos;	/*position of phy warning */
	u8 phy_err[SAL_MAX_PHY_ERR_PERIOD];	/*phy error info */
	u8 phy_warn[SAL_MAX_PHY_ERR_PERIOD];
	unsigned long phy_err_time[SAL_MAX_PHY_ERR_PERIOD];	/*phy error time */
	unsigned long phy_warn_time[SAL_MAX_PHY_ERR_PERIOD];
};

/*
 *phy information
*/
struct sal_phy {
	u8 phy_id;		/*phy id for debug */
	u8 port_id;		/*chip port id of the the phy belong to */
	u8 link_status;		/*phy link status */
	u8 link_rate;		/*phy link rate */
	u64 local_addr;		/*phy sas address */
	struct sal_bit_err err_code;	/*phy error information */
	struct sas_identify remote_identify;	/*identiy information of remote device */
	u8 phy_err_flag;	/*0-no error, 1-has error */
	u32 phy_reset_flag;	/*0-no reset, 1-has reset */
	u32 connect_wire_type;	/*1-optical cable,0-electric cable */
	enum sal_phy_config_state config_state;	
};

struct port_set_dev_state_print {
	unsigned long last_print_time;	
	u32 print_cnt;		

};

struct port_free_all_dev {
	unsigned long last_print_time;	
	u32 print_cnt;		

};

/*
 * port information
 */
struct sal_port {
	struct sal_card *card;	
	enum sal_port_status status;	/*port status */
	enum sal_link_mode link_mode;	/*link mode;direct mode or not */
	u64 sas_addr;		/*local port sas address */
	u32 index;		
	u32 port_id;		/*port id from chip */
	u32 logic_id;		/*port logic id */
	SAS_ATOMIC dev_num;	/*number of device belong to port */
	SAS_ATOMIC pend_abort;	/*number of pending abort all request */
	SAS_ATOMIC port_in_use;	
	u8 phy_id_list[SAL_MAX_PHY_NUM];	/*phy list */
	u8 min_rate;		/*port minimal link rate */
	u8 max_rate;		/*port maximal link rate */
	u8 led_status;		/*led status */
	u8 host_disc;		/*should do discover? */
	u32 phy_up_in_port;	/*0:no phy up event, 1:has phy up event */
	void *ll_port;
	u32 port_need_do_full_disc;	
	struct port_set_dev_state_print set_dev_state;	
	struct port_free_all_dev free_all_dev;	
	u8 is_conn_high_jbod;
};


struct sal_err_handle {
	u32 retry_cnt;		
	u32 event_retries;	
};

struct sal_io_dfx {
	u32 io_err_type;
	u32 sense_key;
	u32 asc;
	u32 ascq;
	u32 err_cnt;
};

struct sal_io_stat_info {
	u8 cdb[16];
	u64 start_jiff;
	u64 back_jiff;
	u32 retry_times;
	u32 ret_val;
};

struct sal_disk_io_stat {
	u16 host_id;
	u32 scsi_id;
	u32 max_latency;
	u32 slow_ios;
	u64 fail_io;
	u64 total_send;
	u64 total_back;
	u64 total_read;
	u64 total_write;
	u64 total_time;
	u64 total_block;	
	struct sal_io_stat_info last_10_ios[SAL_LAST_TEN_IO_STAT];
};

struct sal_dev_slow_io {
	u32 sum_io_snap;	
	u32 pend_io_snap;	
	u32 busy_time_snap_ms;

	unsigned long last_print_slow_circle;	
	u32 circle_cnt;		

	unsigned long last_chk_jiff;
	u64 last_idle_start_ns;	
	u64 sum_idle_ns;	
	u32 sum_io;		
	u32 pend_io;	
	
	SAS_SPINLOCK_T slow_io_lock;
	
	u32 pos;
	
	unsigned long slow_jiff[SAL_DEVSLOWIO_CNT];
};

/*
 *sas layer device struct
*/
struct sal_dev {
	struct list_head dev_list;

	SAS_SPINLOCK_T dev_state_lock;	
	enum sal_dev_state dev_status;	/*device status in state machine */

	struct sal_dev_slow_io slow_io;	
	SAS_SPINLOCK_T dev_lock;	

	struct sata_device sata_dev_data;	/*sata device information */

	struct sal_card *card;	/*point to card struct */
	struct sal_port *port;	/*point to port struct */
	void *ll_dev;		/*point to lowlayer device */
	u64 sas_addr;		/*sas address */
	u64 father_sas_addr;	/*father sas address */
	u64 virt_sas_addr;	/*vitual sas address;ses virtual to smp */
	u64 virt_father_addr;	/*vitual father sas address */
	u32 logic_id;		/*device postion in topology */
	u32 scsi_id;		/*scsi id */
	u32 chan_id;		/*channel id */
	u32 bus_id;		/*bus id */
	u32 lun_id;		/*lun id */
	SAS_ATOMIC ref;		/*count using by outside */

	u32 cnt_report_in;	
	u32 cnt_destroy;

	SAS_STRUCT_COMPLETION compl;	/*device event completion sem */
	enum sal_exec_cmpl_status compl_status;	/*device event completion status */
	u32 slot_id;		/*slot id */
	u32 link_rate_prot;	/*protocol defined link rate */
	u32 virt_phy;		/*virtual phy or not */
	u8 direct_attatch;
	u8 ini_type;		
	enum sal_dev_type dev_type;	/*target device type */
	u32 phy_sector_size;	
	u64 phy_sector_num;	
	struct sal_ll_io_err_print_ctrl io_err_print_ctrl;
	bool mid_layer_eh;
	struct sal_dif_ctl dif_ctrl;	
	struct sal_io_dfx io_dfx;	
	struct sal_disk_io_stat dev_io_stat;
	struct drv_ini_private_info drv_info;
};

struct sal_chip_info {
	u8 fw_ver[SAL_MAX_FIRMWARE_LEN];
	u8 ila_ver[SAL_MAX_FIRMWARE_LEN];
	u8 eeprom_ver[SAL_MAX_FIRMWARE_LEN];
	u8 hw_ver[SAL_MAX_FIRMWARE_LEN];
	u8 chip_boot_mode[SAL_MAX_FIRMWARE_LEN];	

};

struct sal_update_op_info {
	enum sal_chip_update update_option;	
	struct file *file;	
};

struct sal_msg;

struct sal_ll_chip_func {
	s32(*chip_op) (struct sal_card *, enum sal_chip_op_type, void *);	
	s32(*phy_op) (struct sal_card *, enum sal_phy_op_type, void *);
	s32(*port_op) (struct sal_card *, enum sal_port_op_type, void *);
	s32(*dev_op) (struct sal_dev *, enum sal_dev_op_type);
	enum sal_cmnd_exec_result (*eh_abort_op) (enum sal_eh_abort_op_type, void *);	
	enum sal_cmnd_exec_result (*send_msg) (struct sal_msg *);	
	 s32(*send_tm) (enum sal_tm_type, struct sal_msg *);	
	 s32(*comm_ll_op) (struct sal_card *, enum sal_common_op_type, void *);	
	 s32(*reg_op) (struct sal_card *, enum sal_reg_op_type, void *);	
	 s32(*gpio_op) (struct sal_card *, enum sal_gpio_op_type, void *);
	 s32(*bitstream_op) (struct sal_card *, enum sal_bitstream_op_type, void *);
};

/*
 *maximal scsi id used by driver
*/
#define SAL_MAX_SCSI_ID_COUNT               SAL_MAX_DEV_NUM

/*
 *scsi id allocate information
*/
struct sal_scsi_id {
	struct sal_dev *dev_slot[SAL_MAX_SCSI_ID_COUNT];	/*SCSI ID-> DEVICE */
	u8 slot_use[SAL_MAX_SCSI_ID_COUNT];	/*used SCSI ID */
};

struct sal_port_io_stat {
	u32 iops;
	u32 band_width;
	u32 total_io;
	u32 total_len;
	unsigned long io_dist[SAL_IOSTAT_CNT_MAX];
	unsigned long time_dist[SAL_IOSTAT_CNT_MAX];
	unsigned long ssp_io[SAL_IOSTAT_CNT_MAX];
	unsigned long read_io[SAL_IOSTAT_CNT_MAX];
	unsigned long write_io[SAL_IOSTAT_CNT_MAX];
	unsigned long stp_io[SAL_IOSTAT_CNT_MAX];
	unsigned long fail_io[SAL_IOSTAT_CNT_MAX];
};

struct sal_drv_io_stat {

	SAS_SPINLOCK_T io_stat_lock;
	u32 stat_switch;	


	SAS_ATOMIC active_io_cnt;

	u32 send_cnt;		
	u32 response_cnt;	
	u32 eh_io_cnt;		

	struct sal_port_io_stat port_io_stat[SAL_MAX_PHY_NUM];
};

struct sal_ll_notify;
struct sal_event;

/*
 * sas controller data structure in sal layer
 */
struct sal_card {
	SAS_SPINLOCK_T card_lock;	
	SAS_SEMA_S card_mutex;	

	SAS_ATOMIC ref_cnt;	/* sal_card reference count */
	SAS_ATOMIC msg_cnt;	/* active io count */
	SAS_ATOMIC port_ref_cnt; /* port reference count */

	u8 card_no;		/* card unique id in sal layer */
	u8 position;	
	u32 flag;		/* card current state flag */
	u32 phy_num;	
	u32 chip_type;

	struct sal_phy *phy[SAL_MAX_PHY_NUM];	/* phy information */
	struct sal_port *port[SAL_MAX_PHY_NUM];
	struct delayed_work port_led_work;	/* led operation related delayed work */
	
	SAS_SPINLOCK_T card_dev_list_lock;	/* device lock */
	struct list_head free_dev_list;	/* head of free device list */
	struct list_head active_dev_list;/* head of active device list */
	struct sal_dev *dev_mem;	
	u32 active_dev_cnt;	
	SAS_ATOMIC pend_abort;	/* abort operation related device count  */

	SAS_SPINLOCK_T card_msg_list_lock[SAL_MAX_MSG_LIST_NUM]; /* msg lock */	
	struct list_head free_msg_list[SAL_MAX_MSG_LIST_NUM];	/* head of free msg list */
	struct list_head active_msg_list[SAL_MAX_MSG_LIST_NUM];	/* head of active msg list */
	struct sal_msg *msg_mem;	


	u32	host_id;	/* host id allocate from SCSI mid layer */
	struct sal_scsi_id scsi_id_ctl;	/* used to allocate scsi id */

	struct sal_drv_io_stat card_io_stat;

	u8 reseting;		/* whether sas controller is reseting  */

	struct task_struct *event_thread;	/* task that process event of sal layer */
	SAS_SPINLOCK_T card_event_lock;		/* event lock */
	struct list_head event_active_list;	/* head of active event list */
	struct list_head event_free_list;	/* head of free event list */
	struct sal_event *event_queue;	/* memory to store event data */

	struct task_struct *eh_thread;	/* task that process error handle notify in sal layer */
	SAS_SPINLOCK_T ll_notify_lock;  /* notify lock */
	struct list_head ll_notify_free_list;	/* head of free error handle notify list */
	struct list_head ll_notify_process_list;	/* head of pending error handle notify list */
	struct sal_ll_notify *notify_mem;	
	SAS_ATOMIC queue_mutex;	
	u32 eh_mutex_flag;	/* flag used for error handle in mid layer and our driver */

	u8 card_clk_abnormal_flag;	/* clock abnormal flag */
	u8 card_clk_abnormal_num;	/* clock abnormal count */
	unsigned long last_clk_chk_time;	/*last card clock routine check time */

	struct pci_dev *pci_device;
	struct device *dev;
	struct pci_device_id *dev_id;	
	struct sal_chip_info chip_info;	
	struct sal_config config;

	struct sal_ll_chip_func ll_func;	/* operation table that low level layer registered */
	void *drv_data;		/* pointer to memory for low level layer */

	struct osal_timer_list port_op_timer;
	struct osal_timer_list routine_timer;	
	struct osal_timer_list io_stat_timer;

	u32 update_percent;
	bool sim_chip_err_done;
	unsigned long verification_key;	

	struct task_struct *wire_event_thread;	/* task that process wire hot plug event */
	SAS_SPINLOCK_T card_wire_lock; /* wire lock */
	struct list_head wire_event_active_list;/* head of active wire event list */
	struct list_head wire_event_free_list;	/* head of free wire event list */
	struct sal_wire_event_notify *gpio_notify_mem;	
	u32 port_curr_wire_type[SAL_MAX_PHY_NUM];	/* 1-optical cable, 0-electric cable */
	u32 port_last_wire_type[SAL_MAX_PHY_NUM];	/* 1-optical cable, 0-electric cable */

};

typedef void (*sal_timer_cbfn_t) (unsigned long);

extern s32 sal_add_card(struct sal_card *sal_card);
extern s32 sal_event_notify(u32 logic_id, u32 event);
extern u32 sal_get_cardid_by_pcid_dev(struct pci_dev *pci_device);
extern s32 sal_special_str_to_ui(const s8 * cp, u32 base, u32 * result);
extern void sal_add_timer(void *timer, unsigned long data,
			  u32 time, sal_timer_cbfn_t timer_cb);
extern struct sal_card *sal_alloc_card(u8 card_pos, u8 card_no, u32 mem_size,
				       u32 phy_num);
extern struct sal_card *sal_get_card(u32 card_no);
extern void sal_put_card(struct sal_card *sal_card);
extern void sal_card_rsc_init(struct sal_card *sal_card);
extern void sal_remove_threads(struct sal_card *sal_card);
extern s32 sal_abort_all_dev_io(struct sal_card *sal_card);
extern void sal_remove_card(struct sal_card *sal_card);
extern s32 sal_data_dir_to_pcie_dir(enum sal_msg_data_direction data_dir);
extern void sal_notify_disc_shut_down(struct sal_card *sal_card);

#endif
