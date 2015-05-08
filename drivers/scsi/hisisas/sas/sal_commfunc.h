/*
 * Huawei Pv660/Hi1610 sas controller driver common data structure and function
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

#ifndef _SAL_COMMFUNC_H_
#define _SAL_COMMFUNC_H_

#include "sal_msg.h"


#define MAX_CHAR_NUM_PER_CMD 32

#define SAL_WRITE_SPS_EEPROM_ALL            1
#define SAL_WRITE_SPS_EEPROM_CFG            2

#define SAL_SLOW_DISK_REPORT_DISABLE    0	
#define SAL_SLOW_DISK_REPORT_ENABLE     1	

#define SAL_SLOW_DISK_MAX_RESPTIME   300	
#define SAL_SLOW_DISK_MAX_SLOWPERIOD  100	

#define PMC_8305_SPS_MAX_PATCH_SIZE 		(32*1024)
#define SAL_SPS_PAGE_SIZE 						64
#define SAL_PMC_SPS_8305                    0x8305
#define SAL_PMC_SPS_8307                    0x8307
#define SAL_PMC_8305_SPS_MAX_CFG_SIZE 		   128	

#define SAL_PMC_BUSY_RETRY_TIMES                600000
#define SAL_OPR_PMC_SPS_MAX_RETRY_CNT 			10000
#define SAL_PMC_SPS_REG_DEVICE_ID_LOW 			0xBFA00002
#define SAL_PMC_SPS_REG_DEVICE_ID_HIGH 			0xBFA00003

#define SAL_PMC_SPS_COFIG_RESET_REG             0xBFA00450
#define SAL_PMC_SPS_INTERFACE_CONTROL_REG    	0xBFA00454
#define SAL_PMC_SPS_TRANS_DATA_REG              0xBFA00456
#define SAL_PMC_SPS_INTERRUPT_STATUS_REG        0xBFA00458
#define SAL_PMC_SPS_RECEIVE_DATA_REG            0xBFA0045A
#define SAL_PMC_SPS_WREEP_PROTECT_REG           0xBFA00105	

#define SAL_PMC_SPS_REG_LCLK_GEN 				0xBFA00027
#define SAL_PMC_SPS_REG_SSPL_LCLK_CLEAR_BASE 	0xBFA01091
#define SAL_BIT_MASK_LCLK_REQ_ASYNC 			0x80
#define SAL_BIT_MASK_LCLK_TIP 					0x1
#define SAL_BIT_MASK_SSPL_LCLK_CLEAR 			0x1
#define SAL_BIT_MASK_RX_DONE 					0x2
#define SAL_BIT_MASK_TX_DONE 					0x1

#define SAL_PMC_SPS_REG_INVALID_DWORD_BASE 		0xBFA04020
#define SAL_PMC_SPS_REG_DISPARITY_ERR_BASE 		0xBFA04024
#define SAL_PMC_SPS_REG_LOSS_DWORD_BASE 		0xBFA04028
#define SAL_PMC_SPS_REG_CODE_VIOLATION_BASE 	0xBFA04030

#define SAL_PMC_8305_SPS_ISTR_FILE_LENGTH 		0x80

#define SAL_ERROR_INFO_REG_DUMP  (0)
#define SAL_ERROR_INFO_ALL       (1)

#define SAL_WRITE_FIS_CMD    0xC3
#define SAL_READ_FIS_CMD     0xC2

#define SAL_REG_ADDR3(a)  (((a) & 0xFF000000) >> 24);	
#define SAL_REG_ADDR2(a)  (((a) & 0x00FF0000) >> 16);
#define SAL_REG_ADDR1(a)  (((a) & 0x0000FF00) >> 8);
#define SAL_REG_ADDR0(a)  ((a)& 0x000000FF);

#define SAL_DISABLE_SPS_WP  0x40	
#define SAL_ENABLE_SPS_WP   0xbf	

#define SAL_GET_SPS_ERR     0	
#define SAL_CLR_SPS_ERR     1	

#define SAL_GET_IO_STAT     1	
#define SAL_CLR_IO_STAT     2	

#define SAL_BIT_STREAM_TYPE_PRBS     0
#define SAL_BIT_STREAM_TYPE_CJTPAT   1

#define SAL_EXT_LOOP_BACK 	1
#define SAL_INT_LOOP_BACK 	2

#define ENABLE_ALL_REG     1
#define DISABLE_ALL_REG    0

#define SAL_SMP_LEN_INVALID 0

enum sal_port_control {
	SAL_PORT_CTRL_E_SET_SMP_WIDTH = 0x10,
	SAL_PORT_CTRL_E_SET_PORT_RECY_TIME,
	SAL_PORT_CTRL_E_ABORT_IO,
	SAL_PORT_CTRL_E_SET_PORT_RESET_TIME,
	SAL_PORT_CTRL_E_HARD_RESET,
	SAL_PORT_CTRL_E_CLEAN_UP,
	SAL_PORT_CTRL_E_STOP_PORT_RECY_TIMER
};

#define MAX_NUM_OF_INTEGER 0xffffffff	

#define SAL_PORT_RSC_UNUSE   0x0
#define SAL_PORT_RSC_IN_USE  0x1

#define SAL_GET_MAGIC_KEY(num)  (num & 0xffffff00)
#define SAL_GET_CARD_ID(num)    (num & 0xff)

#define SAL_MAX_SFP_PAGE_NUM   3
#define SAL_SFP_WHOLE_REGISTER_BYTES 256	
#define SAL_SFP_UPPER_PAGE_BYTES       128	
#define SAL_SFP_UPPER_PAGE_SHIFT       128	

#define SAL_SFP_UPPER_PAGE_0_SHIFT_TO_640       0	
#define SAL_SFP_UPPER_PAGE_1_SHIFT_TO_640       256
#define SAL_SFP_UPPER_PAGE_2_SHIFT_TO_640       384
#define SAL_SFP_UPPER_PAGE_3_SHIFT_TO_640       512
#define SAL_SFP_OUTPUT_BYTES_LENGTH             640

enum sal_phy_link_rate {
	SAL_PHY_LINK_RATE_1_5_G = 1,
	SAL_PHY_LINK_RATE_3_0_G,
	SAL_PHY_LINK_RATE_6_0_G,
	SAL_PHY_LINK_RATE_12_0_G,
	SAL_PHY_LINK_RATE_BUTT
};

#define SAL_PHY_RATE_IS_VALID(link_rate) ((link_rate == SAL_PHY_LINK_RATE_3_0_G) \
											|| (link_rate == SAL_PHY_LINK_RATE_6_0_G) \
											|| (link_rate == SAL_PHY_LINK_RATE_12_0_G))



struct sal_show_rsc_option {
	u32 port_id;
	u32 max_io;
	u32 active_io;
	u32 send_io;
	u32 resp_io;
	u32 eh_io;
	u32 max_dev;
	u32 active_dev;
	u32 ll_free_sgl_node;	
	u32 ll_active_sgl_node;
	u32 ll_free_req_node;	
};

struct sal_mml_send_smp_info {
	u32 card_id;
	u32 port_id;
	u32 sas_addr_hi;
	u32 sas_addr_lo;
	u64 sas_addr;
	u32 phy_id;
	u32 func;
	u32 phy_op;
};

struct sal_update_option {
	u32 card_id;
	char *file;
	u32 upd_op;

};

struct sal_phy_op_param {
	u32 card_id;
	u32 phy_id;
	u32 logic_port_id;
	u32 phy_rate;
	bool phy_rate_restore;	
	enum sal_phy_op_type op_type;
};

struct sal_phy_link_status_param {
	u32 card_id;
	u32 phy_id;
	u32 logic_port_id;
};

struct sal_phy_ll_op_param {
	u32 card_id;
	u32 phy_id;
	u32 phy_rate;
	u32 op_code;
	bool get_analog;	
	u8 pre_emphasis;
	u8 post_emphasis;
	u8 amplitidu;
};

struct sal_switch_port_info {
	u32 port_id;
	enum sal_port_op_type_by_cmd op_type;
};

struct sal_delay_to_do_param {
	u32 time;
	void *argv_in;
	void *argv_out;
	 s32(*raw_func) (void *, void *);

};

struct sal_set_delay_time_param {
	u32 card_id;
	u32 delay_time;
	u32 flag;
};

struct sal_set_dly_time_param {
	u32 port_id;
	u32 dly_time;
	u32 flag;
};

struct sal_disk_info_param {
	u32 host_id;
	u32 scsi_id;

};

struct sal_led_test_param {
	u32 port_id;
	u32 flag;
};

#define BCT_SWITCH_ON		(1)
#define BCT_SWITCH_OFF		(0)

struct sal_bct_switch_param {
	u32 card_id;
	bool bct_enable;
};

struct sal_get_sps_info_param {
	u32 *sps_ver;
	u16 *sps_type;
};

struct sal_get_chip_dump_info_param {
	u32 card_id;
	u32 type;
};

struct sal_update_sps_param {
	u32 host_id;
	u32 scsi_id;
	s8 *file;
	u32 type;
};

struct sal_port_err_sum_param {
	u32 port_id;
	u8 phy_offset;
};

struct sal_sps_h2d_fis {
	u8 fis_type;	
	u8 pm_port;		
	u8 command;		
	u8 reg_addr0;	

	u8 lba_low;		
	u8 lba_mid;		
	u8 lba_high;	
	u8 bit_operation;	

	u8 reg_addr3;	
	u8 lba_mid_exp;	
	u8 lba_high_exp;	
	u8 reg_addr1;		

	u8 reg_value;	
	u8 reg_addr2;	
	u8 reserved0;	
	u8 control;		

	u8 reserved1;		
	u8 reserved2;	
	u8 reserved3;	
	u8 reserved4;	
};

struct sal_phy_speed_param {
	u32 port_id;
	u32 phy_id;
};

struct sal_io_stat {
	u32 card_id;
	u32 port_id;
	u32 op_flag;
};

struct sal_io_stat_switch {
	u32 card_id;
	u32 switch_flag;
};


struct sal_bit_stream_err_code {
	u32 invalid_dw_cnt;	  
	u32 disparity_err_cnt;	 
	u32 loss_dw_sync_cnt;	 
	u32 code_vilation_cnt;	  
};

struct sal_bit_stream_op {
	u32 logic_port_id;
	u32 type;
	u32 op_code;

};

struct sal_simulate_io_err {
	u32 port_id;
	u32 host_id;
	u32 scsi_id;
	u32 io_err_type;
	u32 sense_key;
	u32 asc;
	u32 ascq;
	u32 err_cnt;
};

struct sal_bit_stream_info {
	u32 port_map;
	u32 bitstream_type;
	void *argv_out;
};

struct sal_port_loopback_test_param {
	u32 port_id;
	u32 loop_type;
	u32 test_time;
	 u32(*done) (u32 result);
};

struct sal_loopback {
	u32 port_map;
	u32 loop_type;

};

struct sal_phy_bit_err_insulate_param {
	u32 port_id;
	u32 flag;

};

struct sal_set_int_coal_param {
	u32 port_id;
	u32 icc0;
	u32 ict0;
};

struct sal_reg_op_param {
	u32 port_id;
	u32 bar;
	u32 reg_offset;
	u32 shift_offset;
	u32 val;
	enum sal_reg_op_type op_code;
};

struct sal_pcie_aer_status_param {
	u32 port_id;
	u32 pcie_ue_state;
	u32 pcie_ce_state;
	enum sal_reg_op_type reg_op;
};

struct sal_local_port_ctl {
	u32 port_id;
	enum sal_port_control op_code;
	u32 param0;
	u32 param1;

};

struct sal_sas_phy_analog_param {
	u32 uiCardid;
	u32 phy_id;
	bool get_analog;
	u8 pre_emphasis;
	u8 post_emphasis;
	u8 amplitidu;
};

struct sal_err_inject_param {
	u32 port_id;
	u32 phy_id;
	u32 err_type;
	u32 err_data;
};


struct sal_comm_config_param {
	u32 dev_miss_dly_time;
	u32 slow_disk_resp_time;	
	u32 slow_disk_interval;
	u32 slow_period;	
	u32 check_period;	
};

struct sal_ncq_switch_param {
	u32 port_id;
	void *disk_info;
	u32 op_code;
};

struct sal_slow_disk_param {
	u32 slow_disk_resp_time;
	u32 slow_disk_interval;
	u32 slow_period;	
	u32 check_period;	
};


#define MAX_FW_DUMP_INDEX	(3)

struct sal_global_info {
	u32 sal_log_level;
	struct sal_comm_config_param sal_comm_cfg;
	struct sal_card *sal_cards[SAL_MAX_CARD_NUM];
	u32 init_failed_reason[SAL_MAX_CARD_NUM];
	u32 sal_mod_init_ok;
	volatile u32 quick_done;
	volatile u32 support_4k;
	volatile u32 enable_all_reg;
	u32 io_stat_threshold[SAL_IOSTAT_CNT_MAX];
	u32 prd_type;
	u32 fw_dump_idx[SAL_MAX_CARD_NUM];	
	unsigned long dly_time[SAL_MAX_CARD_NUM];
};

struct sal_fault_limit {
	u32 bit_err_routine_time;
			      
	u32 bit_err_stop_threshold;
				
	u32 bit_err_routine_cnt;
			    
	u32 bit_err_interval_threshold;
				   
};

struct sal_local_phy_ctrl_param {
	u32 card_id;
	u32 phy_id;
	u32 op_code;
};

struct sal_sfp_get_info_param {
	u32 port_id;
	u32 page_sel;		
	u32 shift_offset;	
	u32 len;		
	void *argv_out;		
};

struct sal_config_port_speed_param {
	u32 port_id;
	u32 link_rate;

};

struct sal_ll_port_ref_op {
	u32 port_idx;
	enum sal_refcnt_op_type port_ref_op_type;
};

extern u32 sal_comm_get_logic_id(u32 card_id, u32 phy_id);

extern s32 sal_comm_get_chip_info(void *argv_in, void *argv_out);

extern s32 sal_comm_switch_phy(struct sal_card *card, u32 phy_id,
			       enum sal_phy_op_type op_type, u32 phy_rate);
extern s32 sal_do_switch_phy(struct sal_card *card, u32 phy_id,
			     enum sal_phy_op_type op_type, u32 phy_rate);

extern s32 sal_operate_phy(void *argv_in, void *argv_out);

extern s32 sal_comm_reset_chip(void *argv_in, void *argv_out);

extern struct sal_port *sal_find_port_by_chip_port_id(struct sal_card *card,
						      u32 chip_port_id);
extern struct sal_card *sal_find_card_by_host_id(u32 host_id);

extern u32 sal_find_link_phy_in_port_by_rate(struct sal_port *port);

extern void sal_wide_port_opt_func(unsigned long card_no);
extern s32 sal_comm_send_smp(void *argv_in, void *argv_out);
extern void sal_clr_phy_err_stat(struct sal_card *card, u32 phy_id);
extern struct sal_port *sal_find_port_by_logic_port_id(struct sal_card *card,
						       u32 logic_port_id);
extern s32 sal_alloc_dma_mem(struct sal_card *card,
			     void **va,
			     unsigned long *pa, u32 size, s32 direction);

extern void sal_free_dma_mem(struct sal_card *card,
			     void *va,
			     unsigned long pa, u32 size, s32 direction);
extern struct sal_dev *sal_find_dev_by_slot_id(struct sal_card *card,
					       struct sal_dev *dev,
					       u32 slot_id);

extern s32 sal_comm_dump_fw_info(u32 card_id, u32 type);
extern s32 sal_comm_clean_up_port_rsc(struct sal_card *card,
				      struct sal_port *port);
extern u32 sal_first_phy_id_in_port(struct sal_card *card, u32 port_idx);

extern s32 sal_wait_clr_port_rsc(struct sal_port *sal_port);

extern void sal_comm_led_operation(struct sal_card *card);

void sal_add_timer(void *timer, unsigned long data,
		   u32 time, sal_timer_cbfn_t timer_cb);

void sal_mod_timer(void *timer, unsigned long expires);

bool sal_timer_is_active(void *timer);

void sal_del_timer(void *timer);

#endif
