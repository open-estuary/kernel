/*
 * copyright (C) huawei
 */

#ifndef _HIGGS_DUMP_H_
#define _HIGGS_DUMP_H_

#define HIGGS_GET_DUMP_LOG_TIMEOUT_JIFF(param)  ((param) + (20)*HZ)
#define HIGGS_MAX_DUMP_FILE_NAME_LEN 	120
#define HIGGS_DUMP_DIR_PATH  "/OSM/log/cur_debug/"

#define BUFFER_COUNT 64
#define HIGGS_FATAL_REG_FILE  		   	"Higgs_FatalReg_dump.bin"
#define HIGGS_ITCT_TB_FILE              "Higgs_itct_tb_dump.bin"
#define HIGGS_IOST_TB_IN_DDR_FILE       "Higgs_iost_tb_in_ddr_dump.bin"
#define HIGGS_IOST_TB_IN_CACHE_FILE     "Higgs_iost_tb_in_cache_dump.txt"
#define HIGGS_IO_ERR_STAT_FILE   	   	"Higgs_io_err_stat_dump.bin"
#define HIGGS_GLB_IO_ERR_STAT_FILE      "Higgs_global_io_err_stat_dump.bin"
#define HIGGS_REQ_CMDTBL_SGE_FILE 		"Higgs_req_cmdtbl_sge_dump.txt"
#define HIGGS_CMD_TB_FILE       	    "Higgs_command_table_log_dump.bin"
#define HIGGS_SGE_FILE 		   		    "Higgs_sge_log_dump.bin"
#define HIGGS_IO_BRK_TB_FILE 		   	"Higgs_io_break_tb_dump.bin"
#define HIGGS_SAS_PORT_CFG_FILE       	"Higgs_sas_port_cfg_dump.bin"
#define HIGGS_SAS_GLB_CFG_FILE 	   		"Higgs_sas_global_cfg_dump.bin"
#define HIGGS_PI_CI_INDEX_FILE    	    "Higgs_dqcq_ci_pi_index_dump.txt"
#define HIGGS_DQ_LOG_FILE           	"Higgs_dq_dump"
#define HIGGS_CQ_LOG_FILE           	"Higgs_cq_dump"

#define HIGGS_ARRAY_SIZE                128
#define HIGGS_SAS_GLB_REG_NUM           324
#define HIGGS_SAS_PORT_REG_NUM          62
#define HIGGS_DUMP_BUFFER_SIZE          (8 * 1024)	/* 8KB */

/* register type*/
enum higgs_reg_type {
	HIGGS_GLB_REG_TYPE,	/*SAS Global cfg*/
	HIGGS_PORT_REG_TYPE,	/*SAS Port cfg*/
	HIGGS_REG_BUTT
};

/* queue type */
enum higgs_queue_type {
	HIGGS_IS_DQ_TYPE,	/*DQ */
	HIGGS_IS_CQ_TYPE,	/*CQ */
	HIGGS_QUEUE_BUTT
};

extern s32 higgs_dump_info(struct sal_card *sal_card, void *in);
extern void higgs_hex_dump(char *hdr, u8 *buf, u32 len, u32 width);
extern u32 higgs_read_glb_cfg_reg(struct sal_card *sal_card, u32 reg_addr);

#endif
