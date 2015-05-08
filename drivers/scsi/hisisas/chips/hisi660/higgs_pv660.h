/*
 * copyright (C) huawei
 */

#ifndef HIGGS_PV660_H
#define HIGGS_PV660_H

#include "higgs_port_reg.h"
#include "higgs_global_reg.h"
/*lint -e46*/

struct higgs_dq_info {
	/*dw0*/
	union {
		struct {
			u32 abort_flag:2;
			u32 rsvd0:2;
			u32 t10_flds_prsnt:1;
			u32 rsponse_report:1;
			u32 tlr_ctrl:2;
			u32 phy_id:8;
			u32 force_phy:1;
			u32 port:3;
			u32 sata_reg_set:7;
			u32 pri:1;
			u32 mode:1;
			u32 cmd:3;
		} st;
		u32 chf_dw0;
	} dw0;
	/*dw1*/
	union {
		struct {
			u32 port_multiplier:4;
			u32 bist_activate:1;
			u32 atapi:1;
			u32 first_party_dma:1;
			u32 reset:1;
			u32 pir_present:1;
			u32 en_trans_layer_retry:1;
			u32 verify_data_rrans_length:1;
			u32 rsvd1:1;
			u32 ssp_pass_through:1;
			u32 ssp_frame_type:3;
			/* ssp_frame_type SSP¡£when Delivery Queue CMD[31:29] is "1"£¬
			   and SSP Pass through mode is not used 
			   000b£ºNon data transfer COMMAND Frame-(SSP Initiator only.)
			   001b£ºRead transfer COMMAND Frame-(SSP Initiator only.)
			   010b£ºWrite transfer COMMAND Frame-(SSP Initiator only.)
			   011b£ºTASK Frame-(SSP Initiator or target.)
			*/

			u32 device_id:16;
		} st;
		u32 chf_dw1;
	} dw1;
	union {
		struct {
			/*dw2*/
			u32 command_frame_length:9;
			u32 leave_affiliation_open:1;
			u32 rsvd2:5;
			u32 max_response_frame_length:9;
			u32 sg_mode:1;
			u32 first_burst:1;
			u32 rsvd3:6;
		} st;
		u32 chf_dw2;
	} dw2;
	/*dw3*/
	union {
		struct {
			u32 initiator_port_trans_tag:16;
			u32 target_port_trans_tag:16;
		} st;
		u32 chf_dw3;
	} dw3;
	/*dw4*/
	u32 data_trans_length;

	/*dw5*/
	u32 first_burst_num;
	/*dw6*/
	union {
		struct {
			u32 dif_prd_table_length:16;
			u32 prd_table_length:16;
		} st;
		u32 chf_dw6;
	} dw6;
	/*dw7*/
	union {
		struct {
			u32 rsvd4:15;
			u32 double_mode:1;
			u32 abort_iptt:16;
		} st;
		u32 chf_dw7;
	} dw7;
	/*dw8*/
	u32 command_table_address_lower;
	/*dw9*/
	u32 command_table_address_upper;
	/*dw10*/
	u32 status_buffer_address_lower;
	/*dw11*/
	u32 status_buffer_address_upper;
	/*dw12*/
	u32 prd_table_address_lower;
	/*dw13*/
	u32 prd_table_address_upper;
	/*dw14*/
	u32 dif_prd_table_address_lower;
	/*dw15*/
	u32 dif_prd_table_address_upper;
};

struct higgs_cq_info {
	union {
		struct {
			u32 iptt:16;	/* *IPTT number*/
			u32 rsvd2:1;
			u32 cmd_cmplt:1;
				 /**complete flag*/
			u32 err_rcrd_xfrd:1;
				 /**error frame*/
			u32 rspns_xfrd:1;
				 /**response had been sent*/
			u32 attention:1;
				 /**one bit of the caust register is set*/
			u32 cmd_rcvd:1;
				 /**receive command under target mode*/
			u32 slot_rst_cmplt:1;
			u32 rspns_good:1;
				 /**SSP command is correct*/
			u32 abort_status:3;
				 /**abort status*/
			u32 io_cfg_err:1;	/*DQ cfg err*/
			u32 rsvd0:4;
		} st;
		u32 cq_dw;
	} dw0;

};

#define HIGGS_RXQ_ATTN (1UL<<20)

struct higgs_iost_info {
	/*qw0*/
	u64 io_type:3;
	u64 io_dir:2;
	u64 cmd_tlr:2;
	u64 send_rpt:1;
	u64 phy_id:8;
	u64 target_ict:16;
	u64 force_phy:1;
	u64 tlr_cnt:4;
	u64 io_retry_cnt:6;
	u64 rsv0:7;
	u64 dif_fmt:1;
	u64 prd_dif_src:1;
	u64 sgl_mode:1;
	u64 pir_present:1;
	u64 first_burst:1;
	u64 ssp_pass_through:1;
	u64 io_slot_number:8;

	/*qw1*/
	u64 io_status:8;
	u64 io_ts:1;
	u64 io_rs:1;
	u64 io_ct:1;
	u64 max_response_frame_length:9;
	u64 rsv1:11;
	u64 chk_length:1;
	u64 xfer_tptt:16;
	u64 io_rt:1;
	u64 io_rd:1;
	u64 mis_cnt:8;
	u64 rsv2:6;

	/*qw2*/
	u64 xfer_offset:32;
	u64 xfer_length:32;

	/*qw3*/
	u64 status_buffer_address;
};

#define HIGGS_REG_DEVICE_STP                         0x00	/*stp device */
#define HIGGS_REG_DEVICE_SAS                         0x01	/*SSP OR SMP */
#define HIGGS_REG_DEVICE_SATA                        0x02	/*directed sata device */
#define HIGGS_REG_DEVICE_INVALID                     0xff	/*invalid device type */
#define HIGGS_I_T_NEXOUS_TIMEOUT                     500
#define HIGGS_REG_ITCT_INVALID_TIMER                 1

/*chip feature*/
#define CACHE_LINE_SIZE 128

/* FPGA*/
#if 0
#ifdef FPGA_VERSION_TEST
#define P660_CORE0_BASE                              0xb1000000ULL
#define P660_CORE1_BASE                              0xc1000000ULL
#else
#define P660_CORE0_BASE                              0xc1000000ULL
#define P660_CORE1_BASE                              0xb1000000ULL
#endif
#endif

/* unified address */
#if 0
#define P660_SAS_CORE0_BASE                          0xc1000000ULL	/* DSAF */
#define P660_SAS_CORE1_BASE                          0xb1000000ULL	/* PCIE */
#define P660_SAS_CORE0_ID							 0
#define P660_SAS_CORE1_ID							 1
#endif

#define P660_SAS_CORE_DSAF_BASE                          0xc1000000ULL
#define P660_SAS_CORE_DSAF_ID                        	 0
#define P660_SAS_CORE_PCIE_BASE                          0xb1000000ULL
#define P660_SAS_CORE_PCIE_ID                        	 1
#define P660_SAS_CORE_RANGE                              (0xffff)

#define P660_SAS_CORE_PCIE_DTS_INDEX                  	 0	/* DTS SAS index */
#define P660_SAS_CORE_DSAF_DTS_INDEX                  	 1

#define P660_HILINK_2_BASE                                (0xB2100000ULL)
#define P660_HILINK_5_BASE                                (0xB2180000ULL)
#define P660_HILINK_6_BASE                                (0xB2200000ULL)
#define P660_HILINK_RANGE                                 (0xfffff)

#ifdef PV660_ARM_SERVER
#define  P660_CPLD_LED_BASE                               (0x98000000)
#define  P660_CPLD_LED_RANGE                              (0xff)
#endif

#ifdef PV660_ARM_SERVER

/* CPU(PV660-1) address offset*/
#define P660_1_SAS_REG_BASE_OFFSET                       	0x40000000000ULL
#define P660_1_HILINK_REG_BASE_OFFSET                       0x40000000000ULL

#define P660_1_SAS_CORE_DSAF_BASE                          (0xc1000000ULL + P660_1_SAS_REG_BASE_OFFSET)
#define P660_1_SAS_CORE_DSAF_ID                        	 	2
#define P660_1_SAS_CORE_PCIE_BASE                          (0xb1000000ULL + P660_1_SAS_REG_BASE_OFFSET)
#define P660_1_SAS_CORE_PCIE_ID                        	 	3
#define P660_1_SAS_CORE_RANGE                              (0xffff)

#define P660_1_SAS_CORE_PCIE_DTS_INDEX                  	 2	/* DTS SAS1 index*/
#define P660_1_SAS_CORE_DSAF_DTS_INDEX                  	 3

#define P660_1_HILINK_2_BASE                                (0xB2100000ULL + P660_1_HILINK_REG_BASE_OFFSET)
#define P660_1_HILINK_5_BASE                                (0xB2180000ULL + P660_1_HILINK_REG_BASE_OFFSET)
#define P660_1_HILINK_6_BASE                                (0xB2200000ULL + P660_1_HILINK_REG_BASE_OFFSET)
#define P660_1_HILINK_RANGE                                 (0xfffff)

#endif
/*
 * 24 Dual-timer,DSAF SAS CORE use the third Timer, PCIE SAS CORE use the fourth Timer.
 * it is used the Timer0 in team
*/
#define P660_SP804_TIMER_ADDR_BASE                          0x80060000ULL
#define P660_SP804_TIMER_SAS_DSAF_ADDR_BASE                 0x80090000ULL
#define P660_SP804_TIMER_SAS_PCIE_ADDR_BASE                 0x800A0000ULL
#define P660_SP804_TIMER_LOAD_REG                           0x0
#define P660_SP804_TIMER_CONTROL_REG                        0x8
#define P660_SP804_TIMER_INTCLR_REG                         0xC
#define P660_SP804_TIMER_BGLOAD_REG                         0x18
#define P660_SP804_TIMER_US_TO_COUNT(us)                    ((us) * 2 / 5)	/* 400KHZ£¬2.5us */

#if defined(FPGA_VERSION_TEST)
#define VAR_CHECK_CARD (P660_SAS_CORE_PCIE_ID)
#elif defined(EVB_VERSION_TEST)
#define VAR_CHECK_CARD (P660_SAS_CORE_PCIE_ID)
#elif defined(C05_VERSION_TEST)
#define VAR_CHECK_CARD (P660_SAS_CORE_DSAF_ID)
#elif defined(PV660_ARM_SERVER)
#define VAR_CHECK_CARD (P660_SAS_CORE_DSAF_ID)
#endif

#define HIGGS_SAS_RESET_STATUS_MASK                  0x7FFFF
#define HIGGS_SAS_RESET_STATUS_RESET                 0x7FFFF
#define HIGGS_SAS_RESET_STATUS_DERESET               0x0

#define HIGGS_SLAVE_DSAF_SUBCTL_BASE                 (0x40000000000ULL + 0xc0000000ULL)

#define HIGGS_DSAF_SUBCTL_BASE                       0xc0000000ULL
#define HIGGS_DSAF_SUBCTL_RANGE                      (0xffff)
#define HIGGS_DSAF_SUB_CTRL_RESET_OFFSET             0xA60
#define HIGGS_DSAF_SUB_CTRL_RESET_VALUE              0x7FFFF
#define HIGGS_DSAF_SUB_CTRL_DERESET_OFFSET           0xA64
#define HIGGS_DSAF_SUB_CTRL_DERESET_VALUE            0x7FFFF
#define HIGGS_DSAF_SUB_CTRL_RESET_STATUS_OFFSET      0x5A30

#define HIGGS_SLAVE_PCIE_SUBCTL_BASE                 (0x40000000000ULL + 0xb0000000ULL)

#define HIGGS_PCIE_SUBCTL_BASE                       0xb0000000ULL
#define HIGGS_PCIE_SUBCTL_RANGE                      (0xffff)
#define HIGGS_PCIE_SUB_CTRL_RESET_OFFSET             0xA18
#define HIGGS_PCIE_SUB_CTRL_RESET_VALUE              0x7FFFF
#define HIGGS_PCIE_SUB_CTRL_DERESET_OFFSET           0xA1C
#define HIGGS_PCIE_SUB_CTRL_DERESET_VALUE            0x7FFFF
#define HIGGS_PCIE_SUB_CTRL_RESET_STATUS_OFFSET      0x5A0C

#define HIGGS_AXIMASTERCFG_CURR_PORT_STS_OFFSET      0x100

#define HIGGS_CHIPRESET_WAIT_RESETREQ                50	/*chip reset, wait REQ RESET,us */
#define HIGGS_CHIPRESET_WAIT_IDLE                    1000	/* chip reset,wait DMA/AXI IDLE,ms */

#define HIGGS_PHY_BIST_CODE_DEFAULT                  0xedba2564
#define HIGGS_PHY_BIST_CHK_TIME_DEFAULT              0x006403E8

#define HIGGS_ILLEGAL_IPTT_THRESHOLD                 3	/* illegal IPTT past 3 times,need reset chip*/

/***ITCT table***/
struct higgs_itct_base_info {
	u16 dev_type:2;		/*device type*/
	u16 valid:1;		/*whether valid*/
	u16 break_reply_enable:1;	/*break_reply,1:support,0:nonsupport*/
	u16 awt_control:1;	/* AWT control,1:support,0:nonsupport*/
	u16 max_conn_rate:4;	/*max connect rate*/
	u16 valid_link_num:4;	/*the number of valid link */
	u16 port_id:3;		/*portID */
	u16 smp_timeout;
	u32 max_burst_byte;
};

/***ITCT Port Layer Overall Ctrl timer***/
struct higgs_itct_ploctimer {
	u16 it_nexus_loss_time;
	u16 bus_inactive_limit_time;
	u16 max_connect_limit_time;
	u16 reject_open_limit_time;
};

/***ITCT misc***/
struct higgs_itct_misc {
	u16 curr_pathway:8;
	u16 current_trans_dir:2;
	u16 rsvd0:5;
	u16 awt_continue:1;
	u16 current_awt;
	u16 current_it_loss_timer;
	u16 tlr_enable:1;
	u16 catap:4;
	u16 cnt:5;
	u16 cpn:4;
	u16 cb:1;
	u16 rsvd1:1;
};

/***ITCT SATA***/
struct higgs_itct_sataset {
	u32 sata_active_reg;
	u32 rsvd2:9;
	u32 ata_status:8;
	u32 eb:1;
	u32 rpn:4;
	u32 rb:1;
	u32 sata_tx_ata_p:4;
	u32 tpn:4;
	u32 tb:1;
};

/***ITCT info***/
struct higgs_itct_info {
	struct higgs_itct_base_info base_info;
	u64 sas_addr;
	struct higgs_itct_ploctimer ploc_timerset;
	struct higgs_itct_misc misc_set;
	struct higgs_itct_sataset sata_set;
	u16 ncq_tag[32];
	union {
		struct {
			u16 non_ncq_iptt;
			u16 rsvd3[3];
		} st;
		u64 itct_dw13;
	};
	u64 rsvd4[2];
};

struct higgs_break_point_info {
	u8 data[128];
};

/*sge struct*/
struct higgs_sge_entry {
	u32 addr_low;/**the low 32bit of the address of data buffers,must 4 multiple byte*/
	u32 addr_hi; /**the hi 32bit of the address of data buffers,must 4 multiple byte*/
	u32 page_ctrol0;
	u32 page_ctrol1;
	u32 data_Len;/**the lenght of the data buffers,bytes*/
	u32 data_offset;/**the offset of the data*/

};

/* sge page struct */
struct higgs_sge_entry_page {
	struct higgs_sge_entry entry[SAL_MAX_DMA_SEGS];
};

/*sgl list struct*/
struct higgs_sgl_list {
	u32 next_sgl_addr_low;
	u32 next_sgl_addr_hi;
	u16 num_sge_in_chain;
	u16 num_sge_in_sgl;
	u32 flag;
	u32 serial_num_low;
	u32 serial_num_hi;
	struct higgs_sge_entry sgl_entry[64];
};

struct err_info_record {
	u32 err_info0;
	u32 err_info1;
	u32 err_info2;
	u32 err_info3;
};

#define MAX_SSP_RESP_DATA_SIZE 1024
struct sas_ssp_resp {
	u8 rsv1[8];
	u16 status_qualifier;
	u8 data_pres;
	u8 status;
	u8 rsv2[4];
	u32 sense_data_len;
	u32 resp_data_len;
	u8 data[MAX_SSP_RESP_DATA_SIZE];
};

#ifdef _PCLINT_
#define HIGGS_MAX_SMP_RESP_SIZE	(higgs_dispatcher.higgs_log_level)
#else

#define HIGGS_MAX_SMP_RESP_SIZE (1016)
#endif
#define HIGGS_MAX_RESPONSE_LENGTH   \
    (MAX(sizeof(struct sas_ssp_resp), HIGGS_MAX_SMP_RESP_SIZE))

/* Status Buffer */
struct status_buf {
	struct err_info_record err_info;
	union {
		struct sas_ssp_resp ssp_resp;
		u8 smp_resp[HIGGS_MAX_SMP_RESP_SIZE];
	} data;
};

struct protect_info {
	/*DWORD 0 */
	union {
		struct {
			u32 t10_insrt_en:1;
			u32 t10_rmv_en:1;
			u32 t10_rplc_en:1;
			u32 t10_chk_en:1;
			u32 chk_dsbl_md:1;
			u32 incr_lbrt:1;
			u32 incr_lbat:1;
			u32 prd_data_incl_t10:1;
			u32 rsv0:8;
			u32 app_proc_mode:1;
			u32 dif_fmt:1;
			u32 usr_dif_block_sz:2;
			u32 usr_bt_sz:12;
		} st;
		u32 pi_dw0;
	} dw0;

	/*DWORD 1,2 */
	u32 lbrt_chk_val;	/* Logical Block Reference tag */
	u32 lbrt_gen_val;	/* Logical Block Reference tag Gen Value */
	/*DWORD 3 */
	union {
		struct {
			u32 lbat_chk_val:16;	/* Logical Block Application tag Check Value */
			u32 lbat_chk_mask:16;	/* Logical Block Application tag Check Mask */
		} st;
		u32 pi_dw3;
	} dw3;
	/*DWORD 4 */
	union {
		struct {
			u32 lbat_gen_val:16;	/* Logical Block Application tag Gen Value */
			u32 t10_chk_msk:8;	/*T10 Check Mask */
			u32 t10_prlc_msk:8;	/*T10 Replace Mask */
		} st;
		u32 pi_dw4;
	} dw4;
	/*DWORD 5,6 */
	union {
		struct {
			u32 crc_gen_seed_val:16;
			u32 crc_chk_seed_val:16;
		} st;
		u32 pi_dw5;
	} dw5;

};

struct ssp_cmd_table {
	struct sas_ssp_frame_header frame_header;
	union {
		struct {
			struct sas_ssp_cmnd_uint command_iu;
			struct protect_info pir;
		} command;
		struct sas_ssp_tm_info_unit task;
		struct sas_ssp_xfer_rdy_iu xfer_rdy;
		struct sas_ssp_resp resp;
	} data;
};

struct stp_cmd_table {
	u8 fis[20];		/* Command FIS */
	union {
		u8 fis_r[44];
		struct protect_info pir;
	};
	u8 atapi_cdb[32];	/* atapi CDB */
};

typedef struct disc_smp_req SMP_HIGGS_COMMAND_TABLE_S;

struct higgs_cmd_table {
	/*TODO: sal need to added */
	struct sas_ssp_open_frame open_addr_frame;
	u8 rsv1[4];
	struct status_buf status_buff;
	u8 rsv2[8];		/*used for alignment */
	union {
		struct ssp_cmd_table ssp_cmd_table;
		SMP_HIGGS_COMMAND_TABLE_S smp_cmd_table;
		struct stp_cmd_table stp_cmd_table;
	} table;

};

#define HIGGS_DQ_ENTRY_SIZE (sizeof(struct higgs_dq_info))
#define HIGGS_CQ_ENTRY_SIZE (sizeof(struct higgs_cq_info))
#define HIGGS_ITCT_ENTRY_SIZE (sizeof(struct higgs_itct_info))
#define HIGGS_IOST_ENTRY_SIZE (sizeof(struct higgs_iost_info))
#define HIGGS_BREAK_POINT_ENTRY_SIZE (sizeof(struct higgs_break_point_info))

enum cmd_in_command_header {
	SSP_PROTOCOL_CMD = 1,
	SMP_PROTOCOL_CMD = 2,
	STP_SATA_PROTOCOL_CMD = 3,
	SATA_PROTOCOL_CMD = 4,
	ABORT_CMD = 5,
	CMD_BUTT
};

 /*DQ*/
#define HIGGS_CHF_CMD(cmdheader,macro_cmd)  \
    {(cmdheader)->dw0.st.cmd = (macro_cmd);}
#define HIGGS_CHF_MODE(cmdheader,macro_mode)    \
    {(cmdheader)->dw0.st.mode = (macro_mode);}
#define HIGGS_CHF_PRI(cmdheader,macro_pri)	\
	{(cmdheader)->dw0.st.pri = (macro_pri);}
#define HIGGS_CHF_SATA_REG_SET(cmdheader,macro_set)	\
	{(cmdheader)->dw0.st.sata_reg_set = (macro_set);}
#define HIGGS_CHF_PORT(cmdheader,macro_port)	\
	{(cmdheader)->dw0.st.port = (macro_port);}
#define HIGGS_CHF_FORCE_PHY(cmdheader,macro_phy)	\
	{(cmdheader)->dw0.st.force_phy = (macro_phy);}
#define HIGGS_CHF_PHY_ID(cmdheader,id)	\
	{(cmdheader)->dw0.st.phy_id = (id);}
#define HIGGS_CHF_TLR_CTRL(cmdheader,macro_ctrl)	\
	{(cmdheader)->dw0.st.tlr_ctrl = (macro_ctrl);}
#define HIGGS_CHF_RSP_REPO(cmdheader,macro_report)  \
    {(cmdheader)->dw0.st.rsponse_report = (macro_report);}
#define HIGGS_CHF_T10_FLDS_PRSNT(cmdheader,macro_fmt)   \
    {(cmdheader)->dw0.st.t10_flds_prsnt = (macro_fmt);}
#define HIGGS_CHF_ABORT_FLAG(cmdheader,macro_flag)  \
    {(cmdheader)->dw0.st.abort_flag = (macro_flag);}
#define HIGGS_CHF_PORT_MUL(cmdheader,macro_multiplier)  \
    {(cmdheader)->dw1.st.port_multiplier = (macro_multiplier);}
#define HIGGS_CHF_BIST(cmdheader,macro_activate)  \
    {(cmdheader)->dw1.st.bist_activate = (macro_activate);}
#define HIGGS_CHF_ATAPI(cmdheader,macro_atapi)  \
    {(cmdheader)->dw1.st.atapi = (macro_atapi);}
#define HIGGS_CHF_FP_DMA(cmdheader,macro_dma)  \
    {(cmdheader)->dw1.st.first_party_dma = (macro_dma);}
#define HIGGS_CHF_REST(cmdheader,macro_reset)  \
    {(cmdheader)->dw1.st.reset = (macro_reset);}
#define HIGGS_CHF_PIR_PRESENT(cmdheader,present)  \
    {(cmdheader)->dw1.st.pir_present = (present);}
#define HIGGS_CHF_TRAN_RETRY(cmdheader,retry)  \
    {(cmdheader)->dw1.st.en_trans_layer_retry = (retry);}
#define HIGGS_CHF_VDT_LEN(cmdheader,len)  \
    {(cmdheader)->dw1.st.verify_data_rrans_length = (len);}
#define HIGGS_CHF_SSP_PASS(cmdheader,through)  \
    {(cmdheader)->dw1.st.ssp_pass_through = (through);}
#define HIGGS_CHF_SSP_FRAME(cmdheader,type)  \
    {(cmdheader)->dw1.st.ssp_frame_type = (type);}
#define HIGGS_CHF_DEVICE_ID(cmdheader,id)  \
    {(cmdheader)->dw1.st.device_id = (id);}
#define HIGGS_CHF_FIRST_BURST(cmdheader,burst)  \
    {(cmdheader)->dw2.st.first_burst = (burst);}
#define HIGGS_CHF_CMMF_LEN(cmdheader,len)  \
    {(cmdheader)->dw2.st.command_frame_length = (len);}
#define HIGGS_CHF_LA_OPEN(cmdheader,open)  \
    {(cmdheader)->dw2.st.leave_affiliation_open = (open);}
#define HIGGS_CHF_MAX_RESP_LEN(cmdheader,len)  \
    {(cmdheader)->dw2.st.max_response_frame_length = (len);}
#define HIGGS_CHF_SG_MODE(cmdheader,mode)  \
    {(cmdheader)->dw2.st.sg_mode = (mode);}
#define HIGGS_CHF_IPTT(cmdheader,tag)  \
    {(cmdheader)->dw3.st.initiator_port_trans_tag = (tag);}
#define HIGGS_CHF_TPTT(cmdheader,tag)  \
    {(cmdheader)->dw3.st.target_port_trans_tag = (tag);}
#define HIGGS_CHF_PRD_LEN(cmdheader,len)  \
    {(cmdheader)->dw6.st.prd_table_length = (len);}
#define HIGGS_CHF_DIF_PRD_LEN(cmdheader,len)  \
    {(cmdheader)->dw6.st.dif_prd_table_length = (len);}
#define HIGGS_CHF_DT_LEN(cmdheader,len)  \
    {(cmdheader)->data_trans_length = (len);}
#define HIGGS_CHF_CMMT_LO(cmdheader,addr_lower)  \
    {(cmdheader)->command_table_address_lower = (addr_lower);}
#define HIGGS_CHF_CMMT_UP(cmdheader,addr_upper)  \
    {(cmdheader)->command_table_address_upper = (addr_upper);}
#define HIGGS_CHF_FIRST_BURST_NUM(cmdheader,num)  \
    {(cmdheader)->first_burst_num = (num);}
#define HIGGS_CHF_DOUBLE_MODE(cmdheader,macro_mode)  \
    {(cmdheader)->dw7.st.double_mode = (macro_mode);}
#define HIGGS_CHF_ABORT_IPTT(cmdheader,iptt)  \
    {(cmdheader)->dw7.st.abort_iptt = (iptt);}
#define HIGGS_CHF_STATUS_BUFF_LO(cmdheader,addr_lower)  \
    {(cmdheader)->status_buffer_address_lower = (addr_lower);}
#define HIGGS_CHF_STATUS_BUFF_UP(cmdheader,addr_upper)  \
    {(cmdheader)->status_buffer_address_upper = (addr_upper);}
#define HIGGS_CHF_PRD_TABLE_LO(cmdheader,addr_lower)  \
    {(cmdheader)->prd_table_address_lower = (addr_lower);}
#define HIGGS_CHF_PRD_TABLE_UP(cmdheader,addr_upper)  \
    {(cmdheader)->prd_table_address_upper = (addr_upper);}
#define HIGGS_CHF_DIF_PRD_LO(cmdheader,addr_lower)  \
    {(cmdheader)->dif_prd_table_address_lower = (addr_lower);}
#define HIGGS_CHF_DIF_PRD_UP(cmdheader,addr_upper)  \
    {(cmdheader)->dif_prd_table_address_upper = (addr_upper);}
     /*CQ*/
     
#define HIGGS_CQ_DW(cq)		(((struct higgs_cq_info *)(void *)(cq))->dw0.cq_dw)
#define HIGGS_CQ_IPTT(cq)	(((struct higgs_cq_info *)(void *)(cq))->dw0.st.iptt)
#define HIGGS_CQ_ATTENTION(cq)  (((struct higgs_cq_info *)(void *)(cq))->dw0.st.attention)
#define HIGGS_CQ_CMD_CMPLT(cq)  (((struct higgs_cq_info *)(void *)(cq))->dw0.st.cmd_cmplt)
#define HIGGS_CQ_ERR_RCRD_XFRD(cq)  (((struct higgs_cq_info *)(void *)(cq))->dw0.st.err_rcrd_xfrd)
#define HIGGS_CQ_RSPNS_XFRD(cq)  (((struct higgs_cq_info *)(void *)(cq))->dw0.st.rspns_xfrd)
#define HIGGS_CQ_CMD_RCVD(cq)  (((struct higgs_cq_info *)(void *)(cq))->dw0.st.cmd_rcvd)
#define HIGGS_CQ_RSPNS_GOOD(cq)  (((struct higgs_cq_info *)(void *)(cq))->dw0.st.rspns_good)
#define HIGGS_CQ_ABORT_STATUS(cq)  (((struct higgs_cq_info *)(void *)(cq))->dw0.st.abort_status)
#define HIGGS_CQ_CFG_ERR(cq)  (((struct higgs_cq_info *)(void *)(cq))->dw0.st.io_cfg_err)
    enum io_abort_status {
	IO_ABORTED_STATUS = 0x0, /**/ IO_NOT_VALID_STATUS = 0x1,
	IO_NO_DEVICE_STATUS = 0x2,
	IO_COMPLETE_STATUS = 0x3,
	IO_ABORT_BUTT
};

/*COMMAND TABLE*/
#define HIGGS_CMT_OPEN_FRAME(cmdtable)    \
    (&(((struct higgs_cmd_table *)cmdtable)->open_addr_frame))
#define HIGGS_CMT_STATUS_BUFFER(cmdtable) \
    (&(((struct higgs_cmd_table *)cmdtable)->status_buff))
#define HIGGS_CMT_SSP_TABLE(cmdtable) \
    (&(((struct higgs_cmd_table *)cmdtable)->table))
#define HIGGS_CMT_SMP_TABLE(cmdtable) \
    HIGGS_CMT_SSP_TABLE(cmdtable)
#define HIGGS_CMT_STP_TABLE(cmdtable) \
    HIGGS_CMT_SSP_TABLE(cmdtable)

#define HIGGS_CMT_STATUS_ERROR_INFO(cmdtable) \
    (&(((struct higgs_cmd_table *)cmdtable)->status_buff.err_info))
#define HIGGS_CMT_STATUS_SSP_RESP(cmdtable)   \
    (&(((struct higgs_cmd_table *)cmdtable)->status_buff.data))
#define HIGGS_CMT_STATUS_SMP_RESP(cmdtable)   \
    HIGGS_CMT_STATUS_SSP_RESP(cmdtable)
#define HIGGS_CMT_SSP_FRAME_HEADER(cmdtable)  \
    (&(((struct higgs_cmd_table *)cmdtable)->table.ssp_cmd_table.frame_header))
#define HIGGS_CMT_SSP_COMMAND_IU(cmdtable)    \
    (&(((struct higgs_cmd_table *)cmdtable)->table.ssp_cmd_table.data))
#define HIGGS_CMT_SSP_TASK_IU(cmdtable) HIGGS_CMT_SSP_COMMAND_IU(cmdtable)
#define HIGGS_CMT_SSP_XFERRDY_IU(cmdtable) HIGGS_CMT_SSP_COMMAND_IU(cmdtable)
#define HIGGS_CMT_SSP_RESPONSE_IU(cmdtable) HIGGS_CMT_SSP_COMMAND_IU(cmdtable)

/*Error Info*/
#define HIGGS_ERROR_INFO_DW0(err_info) (((struct err_info_record *)err_info)->err_info0)
#define HIGGS_ERROR_INFO_DW1(err_info) (((struct err_info_record *)err_info)->err_info1)
#define HIGGS_ERROR_INFO_DW2(err_info) (((struct err_info_record *)err_info)->err_info2)
#define HIGGS_ERROR_INFO_DW3(err_info) (((struct err_info_record *)err_info)->err_info3)

#define HIGGS_ERROR_INFO_OFFSET(n) (((u32)1UL) << ((u32)n))

enum higgs_io_err_type_e {
	HIGGS_IO_SUCC = 100,	/*complete successful io*/
	HIGGS_IO_DIF_ERROR = 1001,	/*error in DW0*/
	HIGGS_IO_UNDERFLOW,
	HIGGS_IO_OVERFLOW,
	HIGGS_IO_BUS_ERROR,	 /*bus error*/

	HIGGS_IO_UNEXP_XFER_RDY,	/* Unexpected XFER_RDY Error.*/
	/*1)    receive xfer_rdy while the io is not the write one;
	  2)    when the device registe,and the device is not support retransmit,
	        but the retry data frame was set to 1 in xfer_rdy
	*/
	HIGGS_IO_XFER_RDY_OFFSET,	/*Received XFER_RDY Offset Error.*/
	HIGGS_IO_XFR_RDY_LENGTH_OVERFLOW_ERR,
	HIGGS_IO_RX_BUFFER_ECC_ERR,
	HIGGS_IO_RX_DIF_CRC_ERR,
	HIGGS_IO_RX_DIF_APP_ERR,
	HIGGS_IO_RX_DIF_RPP_ERR,
	HIGGS_IO_RX_RESP_BUFFER_OVERFLOW_ERR,
	HIGGS_IO_RX_AXI_BUS_ERR,
	HIGGS_IO_RX_DATA_SGL_OVERFLOW_ERR,
	HIGGS_IO_RX_DIF_SGL_OVERFLOW_ERR,
	HIGGS_IO_RX_DATA_OFFSET_ERR,
	HIGGS_IO_RX_UNEXP_RX_DATA_ERR,
	HIGGS_IO_RX_DATA_OVERFLOW_ERR,
	HIGGS_IO_RX_UNEXP_RETRANS_RESP_ERR,

	HIGGS_IO_PHY_NOT_RDY = 2001,	/*error in DW1*/
	HIGGS_IO_OPEN_REJCT_WRONG_DEST_ERR,
	HIGGS_IO_OPEN_REJCT_ZONE_VIOLATION_ERR,
	HIGGS_IO_OPEN_REJCT_BY_OTHER_ERR,
	HIGGS_IO_OPEN_REJCT_AIP_TIMEOUT_ERR,
	HIGGS_IO_OPEN_REJCT_STP_BUSY_ERR,
	HIGGS_IO_OPEN_REJCT_PROTOCOL_NOT_SUPPORT_ERR,
	HIGGS_IO_OPEN_REJCT_RATE_NOT_SUPPORT_ERR,
	HIGGS_IO_OPEN_REJCT_BAD_DEST_ERR,
	HIGGS_IO_BREAK_RECEIVE_ERR,	/*the io requst is fail ,because of receiving break*/
	HIGGS_IO_LOW_PHY_POWER_ERR,
	HIGGS_IO_OPEN_REJCT_PATHWAY_BLOCKED_ERR,
	HIGGS_IO_OPEN_TIMEOUT_ERR,
	HIGGS_IO_OPEN_REJCT_NO_DEST_ERR,
	HIGGS_IO_OPEN_RETRY_ERR,
	HIGGS_IO_TX_BREAK_TIMEOUT_ERR,
	HIGGS_IO_TX_BREAK_REQUEST_ERR,
	HIGGS_IO_TX_BREAK_RECEIVE_ERR,
	HIGGS_IO_TX_CLOSE_TIMEOUT_ERR,
	HIGGS_IO_TX_CLOSE_NORMAL_ERR,
	HIGGS_IO_TX_CLOSE_PHYRESET_ERR,
	HIGGS_IO_TX_WITH_CLOSE_DWS_TIMEOUT_ERR,
	HIGGS_IO_TX_WITH_CLOSE_COMINIT_ERR,
	HIGGS_IO_TX_NAK_RECEIVE_ERR,
	HIGGS_IO_TX_ACK_NAK_TIMEOUT_ERR,
	HIGGS_IO_TX_CREDIT_TIMEOUT_ERR,
	HIGGS_IO_IPTT_CONFLICT_ERR,
	HIGGS_IO_TXFRM_TYPE_ERR,
	HIGGS_IO_TXSMP_LENGTH_ERR,

	HIGGS_IO_RX_FRAME_CRC_ERR = 3001,	/*error in DW2*/
	HIGGS_IO_RX_FRAME_DONE_ERR,
	HIGGS_IO_RX_FRAME_ERRPRM_ERR,
	HIGGS_IO_RX_FRAME_NO_CREDIT_ERR,
	HIGGS_IO_RX_FRAME_OVERRUN_ERR,
	HIGGS_IO_RX_FRAME_NO_EOF_ERR,
	HIGGS_IO_LINK_BUF_OVERRUN_ERR,
	HIGGS_IO_RX_BREAK_TIMEOUT_ERR,
	HIGGS_IO_RX_BREAK_REQUEST_ERR,
	HIGGS_IO_RX_BREAK_RECEIVE_ERR,
	HIGGS_IO_RX_CLOSE_TIMEOUT_ERR,
	HIGGS_IO_RX_CLOSE_NORMAL_ERR,
	HIGGS_IO_RX_CLOSE_PHYRESET_ERR,
	HIGGS_IO_RX_WITH_CLOSE_DWS_TIMEOUT_ERR,
	HIGGS_IO_RX_WITH_CLOSE_COMINIT_ERR,
	HIGGS_IO_DATA_LENGTH0_ERR,
	HIGGS_IO_BAD_HASH_ERR,
	HIGGS_IO_RX_XRDY_ZERO_ERR,
	HIGGS_IO_RX_SSP_FRAME_LEN_ERR,
	HIGGS_IO_RX_NO_BALANCE_ERR,
	HIGGS_IO_RX_BAD_FRAME_TYPE_ERR,
	HIGGS_IO_RX_SMP_FRAME_LEN_ERR,
	HIGGS_IO_RX_SMP_RESP_TIMEOUT_ERR,

	HIGGS_IO_OTHERS_ERR = 4000,	 /*other error*/
	HIGGS_IO_PORT_NOT_READY_ERR,	/*Port not ready*/
	HIGGS_IO_CMD_NOT_CMPLETE = 4002,	/*CQ.cmd_cmplt = 0*/
	HIGGS_IO_NO_RESPONSE,	/*CQ.rspns_xfrd = 0*/
	HIGGS_IO_ERROR_TYPE_BUTT = 5000	/*unknow error type*/
};

enum err_info_dword0 {
	DW0_IO_TX_DIF_CRC_ERR = HIGGS_ERROR_INFO_OFFSET(0),
	DW0_IO_TX_DIF_APP_ERR = HIGGS_ERROR_INFO_OFFSET(1),
	DW0_IO_TX_DIF_RPP_ERR = HIGGS_ERROR_INFO_OFFSET(2),
	DW0_IO_TX_AXI_BUS_ERR = HIGGS_ERROR_INFO_OFFSET(3),
	DW0_IO_TX_DATA_SGL_OVER_FLOW_ERR = HIGGS_ERROR_INFO_OFFSET(4),
	DW0_IO_TX_DIF_SGL_OVER_FLOW_ERR = HIGGS_ERROR_INFO_OFFSET(5),
	DW0_IO_UNEXP_XFER_RDY_ERR = HIGGS_ERROR_INFO_OFFSET(6),
	DW0_IO_XFER_RDY_OFFSET_ERR = HIGGS_ERROR_INFO_OFFSET(7),
	DW0_IO_TX_DATA_UNDERFLOW_ERR = HIGGS_ERROR_INFO_OFFSET(8),
	DW0_IO_XFER_RDY_LENGTH_OVERFLOW_ERR = HIGGS_ERROR_INFO_OFFSET(9),
	/*Reserved */
	DW0_IO_RX_BUFFER_ECC_ERR = HIGGS_ERROR_INFO_OFFSET(16),
	DW0_IO_RX_DIF_CRC_ERR = HIGGS_ERROR_INFO_OFFSET(17),
	DW0_IO_RX_DIF_APP_ERR = HIGGS_ERROR_INFO_OFFSET(18),
	DW0_IO_RX_DIF_RPP_ERR = HIGGS_ERROR_INFO_OFFSET(19),
	DW0_IO_RX_RESP_BUFFER_OVERFLOW_ERR = HIGGS_ERROR_INFO_OFFSET(20),
	DW0_IO_RX_AXI_BUS_ERR = HIGGS_ERROR_INFO_OFFSET(21),
	DW0_IO_RX_DATA_SGL_OVERFLOW_ERR = HIGGS_ERROR_INFO_OFFSET(22),
	DW0_IO_RX_DIF_SGL_OVERFLOW_ERR = HIGGS_ERROR_INFO_OFFSET(23),
	DW0_IO_RX_DATA_OFFSET_ERR = HIGGS_ERROR_INFO_OFFSET(24),
	DW0_IO_RX_UNEXP_RX_DATA_ERR = HIGGS_ERROR_INFO_OFFSET(25),
	DW0_IO_RX_DATA_OVERFLOW_ERR = HIGGS_ERROR_INFO_OFFSET(26),
	DW0_IO_RX_DATA_UNDERFLOW_ERR = HIGGS_ERROR_INFO_OFFSET(27),
	DW0_IO_RX_UNEXP_RETRANS_RESP_ERR = HIGGS_ERROR_INFO_OFFSET(28),
	/*Reserved */
	DW0_IO_ERR_INFO_BUTT
};

enum err_info_dword1 {
	DW1_IO_PHY_NOT_ENABLE_ERR = HIGGS_ERROR_INFO_OFFSET(1),
	DW1_IO_OPEN_REJCT_WRONG_DEST_ERR = HIGGS_ERROR_INFO_OFFSET(2),
	DW1_IO_OPEN_REJCT_ZONE_VIOLATION_ERR = HIGGS_ERROR_INFO_OFFSET(3),
	DW1_IO_OPEN_REJCT_BY_OTHER_ERR = HIGGS_ERROR_INFO_OFFSET(4),
	/*reserved */
	DW1_IO_OPEN_REJCT_AIP_TIMEOUT_ERR = HIGGS_ERROR_INFO_OFFSET(6),
	DW1_IO_OPEN_REJCT_STP_BUSY_ERR = HIGGS_ERROR_INFO_OFFSET(7),
	DW1_IO_OPEN_REJCT_PROTOCOL_NOT_SUPPORT_ERR = HIGGS_ERROR_INFO_OFFSET(8),
	DW1_IO_OPEN_REJCT_RATE_NOT_SUPPORT_ERR = HIGGS_ERROR_INFO_OFFSET(9),
	DW1_IO_OPEN_REJCT_BAD_DEST_ERR = HIGGS_ERROR_INFO_OFFSET(10),
	DW1_IO_BREAK_RECEIVE_ERR = HIGGS_ERROR_INFO_OFFSET(11),
	DW1_IO_LOW_PHY_POWER_ERR = HIGGS_ERROR_INFO_OFFSET(12),
	DW1_IO_OPEN_REJCT_PATHWAY_BLOCKED_ERR = HIGGS_ERROR_INFO_OFFSET(13),
	DW1_IO_OPEN_TIMEOUT_ERR = HIGGS_ERROR_INFO_OFFSET(14),
	DW1_IO_OPEN_REJCT_NO_DEST_ERR = HIGGS_ERROR_INFO_OFFSET(15),
	DW1_IO_OPEN_RETRY_ERR = HIGGS_ERROR_INFO_OFFSET(16),
	/*reserved */
	DW1_IO_TX_BREAK_TIMEOUT_ERR = HIGGS_ERROR_INFO_OFFSET(18),
	DW1_IO_TX_BREAK_REQUEST_ERR = HIGGS_ERROR_INFO_OFFSET(19),
	DW1_IO_TX_BREAK_RECEIVE_ERR = HIGGS_ERROR_INFO_OFFSET(20),
	DW1_IO_TX_CLOSE_TIMEOUT_ERR = HIGGS_ERROR_INFO_OFFSET(21),
	DW1_IO_TX_CLOSE_NORMAL_ERR = HIGGS_ERROR_INFO_OFFSET(22),
	DW1_IO_TX_CLOSE_PHYRESET_ERR = HIGGS_ERROR_INFO_OFFSET(23),
	DW1_IO_TX_WITH_CLOSE_DWS_TIMEOUT_ERR = HIGGS_ERROR_INFO_OFFSET(24),
	DW1_IO_TX_WITH_CLOSE_COMINIT_ERR = HIGGS_ERROR_INFO_OFFSET(25),
	DW1_IO_TX_NAK_RECEIVE_ERR = HIGGS_ERROR_INFO_OFFSET(26),
	DW1_IO_TX_ACK_NAK_TIMEOUT_ERR = HIGGS_ERROR_INFO_OFFSET(27),
	DW1_IO_TX_CREDIT_TIMEOUT_ERR = HIGGS_ERROR_INFO_OFFSET(28),
	DW1_IO_IPTT_CONFLICT_ERR = HIGGS_ERROR_INFO_OFFSET(29),
	DW1_IO_TXFRM_TYPE_ERR = HIGGS_ERROR_INFO_OFFSET(30),
	DW1_IO_TXSMP_LENGTH_ERR = HIGGS_ERROR_INFO_OFFSET(31),
	DW1_IO_ERR_INFO_BUTT
};

enum err_info_dword2 {
	DW2_IO_RX_FRAME_CRC_ERR = HIGGS_ERROR_INFO_OFFSET(0),
	DW2_IO_RX_FRAME_DONE_ERR = HIGGS_ERROR_INFO_OFFSET(1),
	DW2_IO_RX_FRAME_ERRPRM_ERR = HIGGS_ERROR_INFO_OFFSET(2),
	DW2_IO_RX_FRAME_NO_CREDIT_ERR = HIGGS_ERROR_INFO_OFFSET(3),
	/*reserved */
	DW2_IO_RX_FRAME_OVERRUN_ERR = HIGGS_ERROR_INFO_OFFSET(5),
	DW2_IO_RX_FRAME_NO_EOF_ERR = HIGGS_ERROR_INFO_OFFSET(6),
	DW2_IO_LINK_BUF_OVERRUN_ERR = HIGGS_ERROR_INFO_OFFSET(7),
	DW2_IO_RX_BREAK_TIMEOUT_ERR = HIGGS_ERROR_INFO_OFFSET(8),
	DW2_IO_RX_BREAK_REQUEST_ERR = HIGGS_ERROR_INFO_OFFSET(9),
	DW2_IO_RX_BREAK_RECEIVE_ERR = HIGGS_ERROR_INFO_OFFSET(10),
	DW2_IO_RX_CLOSE_TIMEOUT_ERR = HIGGS_ERROR_INFO_OFFSET(11),
	DW2_IO_RX_CLOSE_NORMAL_ERR = HIGGS_ERROR_INFO_OFFSET(12),
	DW2_IO_RX_CLOSE_PHYRESET_ERR = HIGGS_ERROR_INFO_OFFSET(13),
	DW2_IO_RX_WITH_CLOSE_DWS_TIMEOUT_ERR = HIGGS_ERROR_INFO_OFFSET(14),
	DW2_IO_RX_WITH_CLOSE_COMINIT_ERR = HIGGS_ERROR_INFO_OFFSET(15),
	DW2_IO_DATA_LENGTH0_ERR = HIGGS_ERROR_INFO_OFFSET(16),
	DW2_IO_BAD_HASH_ERR = HIGGS_ERROR_INFO_OFFSET(17),
	DW2_RX_XRDY_ZERO_ERR = HIGGS_ERROR_INFO_OFFSET(18),
	DW2_RX_SSP_FRAME_LEN_ERR = HIGGS_ERROR_INFO_OFFSET(19),

	/*reserved */
	DW2_IO_RX_NO_BALANCE_ERR = HIGGS_ERROR_INFO_OFFSET(21),
	/*reserved */
	DW2_RX_BAD_FRAME_TYPE_ERR = HIGGS_ERROR_INFO_OFFSET(24),
	DW2_RX_SMP_FRAME_LEN_ERR = HIGGS_ERROR_INFO_OFFSET(25),
	DW2_RX_SMP_RESP_TIMEOUT_ERR = HIGGS_ERROR_INFO_OFFSET(26),
	/*reserved */
	DW2_IO_ERR_INFO_BUTT
};

#endif
