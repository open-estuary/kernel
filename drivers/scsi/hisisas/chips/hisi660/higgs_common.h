/*
 * copyright (C) huawei
 * higgs data structure definition
 */
 
#ifndef HIGGS_COMMON_H
#define HIGGS_COMMON_H

#define PV660_ARM_SERVER

#include "sal_common.h"
#include "sal_dev.h"
#include "sal_io.h"
#include "sal_errhandler.h"
#include "sal_init.h"
#include "higgs_osintf.h"
/* #include "higgs_intr.h" */

typedef dma_addr_t OSP_DMA;
typedef phys_addr_t OSP_PHY_ADDR;
struct higgs_dq_info;
struct higgs_cq_info;
struct higgs_iost_info;
struct higgs_break_point_info;
struct higgs_itct_info;
struct higgs_device;
struct higgs_card;

struct sal_msg;
struct higgs_cmd_table;
struct higgs_sge_entry;

/* temporary add by chenqilin */
#define DRV_LOG_INFO  3       

/* #define HIGGS_CARD0_REG_BASE (0xb10000000ULL) */

#define HIGGS_MAX_CARD_NUM  (4)	/* modify for arm server(2cpu) */
#define HIGGS_MAX_PHY_NUM   (8)
#define HIGGS_MAX_PORT_NUM  (8)
#define HIGGS_MAX_DQ_NUM    (4)
#define HIGGS_MAX_CQ_NUM    (HIGGS_MAX_DQ_NUM)	/*cq is equal to dq */

#if defined(EVB_VERSION_TEST)
#define HIGGS_MAX_REQ_NUM   (1024)
#define HIGGS_MAX_DEV_NUM   (512)
#elif defined(FPGA_VERSION_TEST)
#define HIGGS_MAX_REQ_NUM   (512)
#define HIGGS_MAX_DEV_NUM   (128)
#elif defined(C05_VERSION_TEST)
#define HIGGS_MAX_REQ_NUM   (1024)
#define HIGGS_MAX_DEV_NUM   (512)
#elif defined(PV660_ARM_SERVER)
#define HIGGS_MAX_REQ_NUM   (1024)
#define HIGGS_MAX_DEV_NUM   (512)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)	/* linux 3.19 */
#define MAX_INTERRUPT_TABLE_INDEX   (80 + 32 + 2)
#endif

#define HIGGS_MAX_REQ_LIST_NUM   (1)

#define HIGGS_MAX_IOST_CACHE_NUM   (64)

#define HIGGS_REG_ENABLE (1)
#define HIGGS_REG_DISABLE (0)

#define HIGGS_DEVICE_VALID  (1)
#define HIGGS_DEVICE_INVALID    (0)
#define HIGGS_REF(a) ((a) = (a))
#define HIGGS_CHIP_PORT_ID_TO_DEV_MASK 0xff

#define HIGGS_INVALID_PORT_ID      0xff	/*invalid port id for init */
#define HIGGS_INVALID_PHY_ID       0xff	/*invalid port id for init */
#define HIGGS_INVALID_CHIP_PORT_ID 0xf	/* invalid chip port id  */

#define CFG_PHY_BIT_NUM          4
#define HIGGS_ADDR_IDENTIFY_LEN        6

/* register phy speed */
#define HIGGS_PHY_RATE_1_5_G    0x08
#define HIGGS_PHY_RATE_3_0_G    0x09
#define HIGGS_PHY_RATE_6_0_G    0x0a
#define HIGGS_PHY_RATE_12_0_G   0x0b

/* interrupt related */
#define HIGGS_MAX_INTRNAME_LEN  64

/* address related */
#define HIGGS_ADDRESS_LO_GET(addr) ((u32)(((u64)addr)&0xffffffff))
#define HIGGS_ADDRESS_HI_GET(addr) ((u32)((((u64)addr)>>32)&0xffffffff))

enum msi_phy_int {
	MSI_PHY_CTRL_RDY = 0,
	MSI_PHY_DMA_RESP_ERR,
	MSI_PHY_HOTPLUG_TOUT,
	MSI_PHY_BCAST_ACK,
	MSI_PHY_OOB_RESTART,
	MSI_PHY_RX_HARDRST,
	MSI_PHY_STATUS_CHG,
	MSI_PHY_SL_PHY_ENABLED,
	MSI_PHY_INT_REG0,
	MSI_PHY_INT_REG1,
	MSI_PHY_BUTT
};

#define  GET_BIT(data, pos)     ((data) &  (0x1 << (pos)))
#define  SET_BIT(data, pos)     ((data) |  (0x1 << (pos)))
#define  CLEAR_BIT(data, pos)   ((data) & ~(0x1 << (pos)))

#define  PBIT0 (0x01)
#define  PBIT1 (0x02)
#define  PBIT2 (0x04)
#define  PBIT3 (0x08)
#define  PBIT4 (0x10)
#define  PBIT5 (0x20)
#define  PBIT6 (0x40)
#define  PBIT7 (0x80)

/* mask define */
#define  NBIT0 (0xfe)
#define  NBIT1 (0xfd)
#define  NBIT2 (0xfb)
#define  NBIT3 (0xf7)
#define  NBIT4 (0xef)
#define  NBIT5 (0xdf)
#define  NBIT6 (0xbf)
#define  NBIT7 (0x7f)

/* bit mask */
#define  GET_BIT0(data)  (data & 0x0001)
#define  GET_BIT1(data)  (data & 0x0002)
#define  GET_BIT2(data)  (data & 0x0004)
#define  GET_BIT3(data)  (data & 0x0008)
#define  GET_BIT4(data)  (data & 0x0010)
#define  GET_BIT5(data)  (data & 0x0020)
#define  GET_BIT6(data)  (data & 0x0040)
#define  GET_BIT7(data)  (data & 0x0080)
#define  GET_BIT8(data)  (data & 0x0100)
#define  GET_BIT9(data)  (data & 0x0200)
#define  GET_BIT10(data) (data & 0x0400)
#define  GET_BIT11(data) (data & 0x0800)
#define  GET_BIT12(data) (data & 0x1000)
#define  GET_BIT13(data) (data & 0x2000)
#define  GET_BIT14(data) (data & 0x4000)
#define  GET_BIT15(data) (data & 0x8000)
#define  GET_BIT28(data) (data & 0x10000000)
#define  GET_BIT29(data) (data & 0x20000000)

#define HIGGS_ARRAY_ELEMENT_COUNT(array) (sizeof(array)/sizeof(array[0]))
#define OFFSET_OF_FIELD( type, field )    (((u64)&(((type*)1024)->field)) - 1024)
#define HIGGS_TRUE                1	/*true */
#define HIGGS_FALSE               0	/*false */
#define HIGGS_SWAP_32(x)\
	( ((u32)((u8)(x))) << 24 | \
	  ((u32)((u8)((x) >> 8))) << 16 | \
	  ((u32)((u8)((x) >> 16))) << 8 | \
	  ((u32)((u8)((x) >> 24))) \
	)

#define HIGGS_SWAP_16(x) \
	( (((u16)((u8)(x))) << 8) | \
	  (u16)(u8)((x) >> 8) \
	)

#define HIGGS_SWAP_64(x) \
    (   \
        ((u64)(HIGGS_SWAP_32((u32)(x)))) << 32 | \
        ((u64)(HIGGS_SWAP_32((u32)((x) >> 32)))) \
    )

struct higgs_timer_callback_param {
	u32 card_id;
	u32 ll_portid;

	struct osal_timer_list plug_in_timer;
};

#if defined(PV660_ARM_SERVER)
enum higgs_led_opcode {
	LED_DISK_PRESENT = 0,	/* disk present */
	LED_DISK_NORMAL_IO = 0x4,	/* disk normal io */
	LED_DISK_LOCATE = 0x6,	/*disk Locate operaion */
	LED_DISK_REBUILD = 0x7,	/*disk Rebuild operation */
	LED_DISK_FAULT = 0x5,	/*disk fault operation */
	LED_DISK_BUFF
};

enum higgs_hilink_type {
	HIGGS_HILINK_TYPE_DSAF,
	HIGGS_HILINK_TYPE_PCIE,
	HIGGS_HILINK_TYPE_BUTT
};

struct higgs_led_event_notify {
	struct list_head notify_list;
	u32 phy_id;
	enum higgs_led_opcode event_val;	/*light event */
};

#endif

struct higgs_config_info {
	u32 id;
	u32 work_mode;
	u32 smp_tmo;
	u32 jam_tmo;
	u32 numinboundqueues;
	u32 numoutboundqueues;
	u32 ibqueue_num_elements;
	u32 obqueue_num_elements;
	u32 dev_queue_depth;
	u32 max_targets;	/*max support device number */
	u32 max_canqueue;
	u32 max_active_io;
	u32 hwint_coal_timer;
	u32 hwint_coal_count;
	u32 ncq_switch;
	u32 phy_link_rate[HIGGS_MAX_PHY_NUM];
	u32 phy_addr_high[HIGGS_MAX_PHY_NUM];
	u32 phy_addr_low[HIGGS_MAX_PHY_NUM];
	u32 biterr_stop_threshold;
	u32 biterr_interval_threshold;
	u32 biterr_routine_time;
	u32 biterr_routine_count;
	u32 port_bitmap[HIGGS_MAX_PORT_NUM];
	u32 phy_g3_cfg[HIGGS_MAX_PHY_NUM];
	u32 port_mini_id[HIGGS_MAX_PORT_NUM];
/* begin: remove configFile.ini dep */
	u32 cpu_node_id;
	u32 phy_useage;
    u32 phy_num;
    u32 port_num;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)	/* linux 3.19 */
	s32 intr_table[MAX_INTERRUPT_TABLE_INDEX];
	enum higgs_hilink_type hilink_type;
#endif
/* end:remove configFile.ini dep  */    
};

struct higgs_dqueue {
	struct higgs_dq_info *queue_base;
	OSP_DMA queue_dma_base;
	u32 dq_depth;
	/*current change come from write pointer */
	u32 wpointer;
};

struct higgs_cqueue {
	struct higgs_cq_info *queue_base;
	OSP_DMA queue_dma_base;
	u32 cq_depth;
	u32 rpointer;
};

// IO error waiting print time
#define IO_ERROR_PRINT_SILENCE_S	(1)

/*itct manage */
/**LL device address infomation**/
struct higgs_device {
	struct list_head list_node;
	void *up_dev;	     /**SAL Dev pointer**/
	u32 dev_type;
	u32 dev_id;
	u32 valid;		   /** node valid flag **/
	u32 resv;
	/******************    record error info          ******************/
	u32 err_info0;
	u32 err_info1;
	u32 err_info2;
	u32 err_info3;
	unsigned long last_print_err_jiff;	/*the time of printing error before*/
};

struct higgs_struct_info {
	struct higgs_dqueue delivery_queue[HIGGS_MAX_DQ_NUM];
	struct higgs_cqueue complete_queue[HIGGS_MAX_CQ_NUM];

	u32 entry_use_per_dq;
	u32 entry_use_per_cq;
	/*free memory used */

	void *all_dq_base;
	OSP_DMA all_dq_dma_base;

	void *all_cq_base;
	OSP_DMA all_cq_dma_base;
	/*itct */
	struct higgs_itct_info *itct_base;
	OSP_DMA itct_dma_base;
	u32 itct_size;
	/*iost */
	struct higgs_iost_info *iost_base;
	OSP_DMA iost_dma_base;
	u32 iost_size;

	/*break point */
	struct higgs_break_point_info *break_point;
	OSP_DMA dbreak_point;
	u32 break_point_size;

};

struct higgs_diag_data {
	u32 result;

};

struct higgs_phy {
	struct sal_phy *up_ll_phy;
	struct sas_identify sas_identify;    /**< the SAS identify of the phy */
	u32 phy_id;	      /**< the Id of the phy */
	struct higgs_diag_data diag_result;
	struct osal_timer_list phy_run_serdes_fw_timer;	/*phy timer */
	bool run_serdes_fw_timer_en;	/*timer enable flag */

#if defined(PV660_ARM_SERVER)
	struct osal_timer_list phy_dma_status_timer;	/*phy dma link status */   
	bool phy_is_idle;	/*phy idle flag*/
	bool last_time_idle;
#endif

	bool to_notify_phy_stop_rsp;	/* whether notify Phy Stop Rsp event */
};

enum higgs_port_status {
	HIGGS_PORT_FREE,
	HIGGS_PORT_UP,
	HIGGS_PORT_DOWN,
	HIGGS_PORT_INVALID,
	HIGGS_PORT_BUTT
};

struct higgs_port {

	u8 phy[HIGGS_MAX_PHY_NUM];	/* Boolean arrar: the Phys included in the port. */
	u32 index;		/*for debug */
	u8 resv[3];
	u32 status;		/* port state */
	u32 port_id;		/* Port Id from SPCV */
	u32 type;		/* port down type */
	u32 last_phy;		/*record the index of last phy down for firmware */
	u32 phy_bitmap;		/* store phy bitmap */
	struct sal_port *up_ll_port;
	bool to_notify_wire_hotplug;
};

struct higgs_req;
/*protocol dispatch, SSP SMP and internal abort */
struct higgs_prot_dispatch {
	s32(*command_verify) (struct higgs_req * ll_req);
	enum sal_cmnd_exec_result (*command_prepare) (struct higgs_req *
						      ll_req);
	enum sal_cmnd_exec_result (*command_send) (struct higgs_req * ll_req);
	 s32(*command_rsp) (struct higgs_req * ll_req);
	 s32(*command_error) (struct higgs_req * ll_req);
};

enum higgs_abort_flag {
	HIGGS_ABORT_SINGLE_FLAG = (0x0),
	HIGGS_ABORT_DEV_FLAG = (0x1),
	HIGGS_ABORT_BUTT_FLAG = (0xFF)
};

enum higgs_inter_cmd_type {
	INTER_CMD_TYPE_NONE = 0U,
	INTER_CMD_TYPE_ABORT,
	INTER_CMD_TYPE_TASK,
	INTER_CMD_TYPE_BUTT
};

struct higgs_req_context {
	enum higgs_inter_cmd_type cmd_type;	/*TM or internal abort */
	enum higgs_abort_flag inter_type;	/*abort single, abort dev */
	u16 abort_tag;
	u16 abort_dev_id;
	u8 tm_function;
	u8 reserved;
	u64 sas_addr;
};

enum higgs_req_state {
	HIGGS_REQ_STATE_FREE,
	HIGGS_REQ_STATE_RUNNING,
	HIGGS_REQ_STATE_SUSPECT,
	HIGGS_REQ_STATE_BUTT
};

enum higgs_req_event {
	HIGGS_REQ_EVENT_GET,
	HIGGS_REQ_EVENT_COMPLETE,	/* io success */
	HIGGS_REQ_EVENT_HALFDONE,	/* io not complete */
	HIGGS_REQ_EVENT_SUSP_TMO,	/* io timeout */
	HIGGS_REQ_EVENT_ABORT_ST,
	HIGGS_REQ_EVENT_TM_OK = 5,	/*ABORT tm success*/  
	HIGGS_REQ_EVENT_ABORTCHIP_OK,	/*abortsigleio success */
	HIGGS_REQ_EVENT_BUTT
};

struct higgs_req {
	struct list_head pool_entry;	/*link to the free pool */
	void (*completion) (struct higgs_req *, u32);	/*LL REQ complete callback */
	u32 iptt;		/*LL REQ tag, also it is ssp ini tag */
	u32 cmpl_entry;
	struct sal_msg *sal_msg;	/*point to the SAL MSG */
	struct higgs_prot_dispatch *proto_dispatch;	/*ssp,smp,abort handler */
	/*FIXME: the struct defined other place */
	struct higgs_device *ll_dev;	/*LL DEV pointer, get device id */
	struct higgs_card *higgs_card;	/*for find easy */
	u32 queue_id;
	void *dq_info;

	enum higgs_req_state req_state;	/* req state */

	OSP_DMA cmd_table_dma;
	void *cmd_table;

	struct higgs_req_context ctx;
	u8 cmd;
	u8 reserved[3];
	void *sge_info;
	u32 err_info;		/*parse the hardware error info to fill it */
	bool valid_and_in_use;
	bool abort_io_in_hdd_ok;	/*abort io in hdd OK*/
	bool abort_io_in_chip_ok;
	unsigned long req_time_start;
};

#define HIGGS_IPTT_FIFO_TYPE   1
struct higgs_iptt_stack {
	u32 *stack;
	u32 size;
	u32 top;
	u32 ptr_out;
	u8 stack_type;
	u8 reserved[3];

};
struct higgs_req_manager {
	struct list_head req_list;
	void *req_mem;
	u32 req_cnt;
	SAS_SPINLOCK_T req_lock;
	struct higgs_iptt_stack iptt_stack;
};

struct higgs_command_table_pool {
	struct {
		struct higgs_cmd_table *table_addr;
		OSP_DMA table_dma_addr;
	} table_entry[HIGGS_MAX_REQ_NUM];
	u32 malloc_size;
};

#define HIGGS_MAX_DMA_SEGS		(SAL_MAX_DMA_SEGS)

struct higgs_sge_pool {
	struct {
		struct higgs_sge_entry *sge_addr;
		OSP_DMA sge_dma_addr;
		u32 use_count;
		u32 total_cnt;
	} sge_entry[HIGGS_MAX_REQ_NUM];

	u32 sge_cnt;

};

struct higgs_int_info {
	u32 irq_num;
	char intr_vec_name[HIGGS_MAX_INTRNAME_LEN];
	 irqreturn_t(*int_handler) (s32 irq, void *dev_id);
	bool reg_succ;
};

/* chip error stat*/
#define    HIGGS_DPH_ERR_NUM    4
struct higgs_chip_err_fatal_stat {
	u32 sas_mrx_err_cnt;	/*SASMRX received incorrect xfer_rdy */
	u32 hgc_iost_ram_1bit_ecc_err_cnt;	/*1Bit ECC of IOST HGC RAM */
	u32 hgc_iost_ram_mul_bit_ecc_err_cnt;	/*multi-Bit ECC error stat of iost current */
	u32 hgc_iost_ram_mul_bit_ecc_err_rct;	/*multi-Bit ECC error stat of iost before */
	u32 hgc_dqe_ram_1bit_ecc_err_cnt;	/*1Bit ECC error of DQ */
	u32 hgc_dqe_ram_mul_bit_ecc_err_cnt;	/*multi-Bit ECC error stat of dq current */
	u32 hgc_dqe_ram_mul_bit_ecc_err_rct;	/*multi-Bit ECC error stat of dq before */
	u32 hgc_itct_ram_1bit_ecc_err_cnt;	/*1Bit ECC of itct HGC RAM */
	u32 hgc_itct_ram_mul_bit_ecc_err_cnt;	/*multi-Bit ECC error stat of itct current */
	u32 hgc_itct_ram_mul_bit_ecc_err_rct;	/*multi-Bit ECC error stat of itct before */
	u32 hgc_axi_int_err_cnt;	/*AXI interrupt error */
	u32 hgc_fifo_ovfld_int_err_cnt;	/*FIFO overflow interrupt error */
	u32 hgc_1bit_ecc_err_cnt;	/*HGC 1bit ECC error */
	u32 emac_1bit_ecc_err_cnt;	/*DMAC 1bit ECC error */
	u32 dmac_mul_bit_ecc_err_cnt;	/*DMAC multi-bit ECC error */
	u32 dmac_bus_int_err_cnt;	/*DMAC bus interrupt error */

	u32 send_iden_addr_fr_cfg_err_cnt;	/*IDENTIFY address frame config error*/
	u32 rcv_open_addr_fr_err_cnt;	/*receive Identify address frame error */
	u32 stmt_link_rxbuf_ovfld_err_cnt;	/*ST/MT link result in Rx Buffer overflow error */
	u32 link_layer_ela_buf_ovfld_err_cnt;	/*Link level Elasticity Buffer overflow */
	u32 speed_nego_fail_cnt;	/*speed negotiate fail */
	u32 tx_train_pat_lock_lost_tout_cnt;	/*TX-TRAIN£¬Pattern Lock Lost Timeout */
	u32 ps_req_fail_cnt;	/*PS_REQ fail */

	u32 default_err_cnt;

};

/**MSI interrupt error type**/
enum msi_chip_fatal_int {
	MSI_CHIP_FATAL_HGC_ECC = 0,
	MSI_CHIP_FATAL_HGC_AXI,
	MSI_CHIP_FATAL_BUTT
};

struct higgs_iost_cache_info {
	unsigned long save_cache_iost_jiff;
	u32 iost_cache_array[HIGGS_MAX_IOST_CACHE_NUM];
};

struct higgs_card {
	u32 card_id;

	SAS_SPINLOCK_T card_lock;
	u64 card_reg_base;
	u64 hilink_base;
	u64 sp804_timer_base;	/* simulate 8072 interrupt SP804 Timer register base address */

	struct sal_card *sal_card;
	struct platform_device *plat_form_dev;

	struct higgs_config_info card_cfg; /* preserve all cfg info */
	u32 higgs_can_dq;
	u32 higgs_can_cq;
	u32 higgs_can_dev;
	u32 higgs_can_io;

	SAS_SPINLOCK_T card_free_dev_lock;	/*free device list lock*/
	struct list_head card_free_dev_list;	/*free device list */
	void *dev_mem;

	struct higgs_struct_info io_hw_struct_info;
	struct higgs_command_table_pool cmd_table_pool;
	struct higgs_sge_pool sge_pool;

	struct higgs_req_manager higgs_req_manager;
	SAS_SPINLOCK_T dq_lock[HIGGS_MAX_DQ_NUM];

	struct higgs_phy phy[HIGGS_MAX_PHY_NUM];
	struct higgs_port port[HIGGS_MAX_PORT_NUM];

	struct higgs_req *running_req[HIGGS_MAX_REQ_NUM];	/*send to disk, hasn't come back */

	struct higgs_chip_err_fatal_stat chip_err_fatal_stat;	/*chip error stat */

	/*debug use */
	u32 higgs_use_kmem;
	u32 higgs_use_dma_mem;
	struct higgs_int_info phy_irq[MSI_PHY_BUTT][HIGGS_MAX_PHY_NUM];
	struct higgs_int_info cq_irq[HIGGS_MAX_CQ_NUM];
	struct higgs_int_info chip_fatal_irq[MSI_CHIP_FATAL_BUTT];
	struct higgs_int_info sp804_timer_irq;

	/*dump use */
	u32 mem_size;		/* memory used */
	unsigned long dump_log_start_jiff;
	u32 ilegal_iptt_cnt;

	struct higgs_iost_cache_info iost_cache_info;

#if defined(PV660_ARM_SERVER)
	/*light thread */
	void __iomem *cpld_reg_base;
	struct task_struct *led_event_thread;
	struct list_head led_event_active_list;
	SAS_SPINLOCK_T led_event_ctrl_lock;
#endif
	/* END:   Added by c00257296, 2015/1/21 */
};

/* check whether error happened in chip */
#define HIGGS_INT_STAT_REG_FOUND_ERR(val)  (((val.sas_mrx_err_cnt > 0) \
	|| (val.hgc_iost_ram_mul_bit_ecc_err_cnt  > 0 ) \
	|| (val.hgc_dqe_ram_mul_bit_ecc_err_cnt   > 0 ) \
	|| (val.hgc_itct_ram_mul_bit_ecc_err_cnt  > 0 ) \
	|| (val.hgc_axi_int_err_cnt            > 0 ) \
	|| (val.hgc_fifo_ovfld_int_err_cnt      > 0 ) \
	|| (val.dmac_mul_bit_ecc_err_cnt      > 0 ) \
	|| (val.dmac_bus_int_err_cnt           > 0 ) \
	|| (val.send_iden_addr_fr_cfg_err_cnt    > 0 ) \
    || (val.rcv_open_addr_fr_err_cnt        > 0 ) \
	|| (val.stmt_link_rxbuf_ovfld_err_cnt   > 0 ) \
    || (val.link_layer_ela_buf_ovfld_err_cnt > 0 ) \
    || (val.ps_req_fail_cnt               > 0 ) \
    || (val.speed_nego_fail_cnt           > 0 ) \
    || (val.tx_train_pat_lock_lost_tout_cnt  > 0 ) \
    || (val.default_err_cnt              > 0 ) \
    ) ? 1 : 0)

/*chip fault type*/
#define HIGGS_CHIP_MRX_ERR(val) ((val.sas_mrx_err_cnt > 0) ? 1 : 0)
#define HIGGS_CHIP_HGC_IOST_RAM_1BIT_ECC_ERR(val) ((val.hgc_iost_ram_1bit_ecc_err_cnt > 0) ? 1 : 0)
#define HIGGS_CHIP_HGC_IOST_RAM_MUL_BIT_ECC_ERR(val) ((val.hgc_iost_ram_mul_bit_ecc_err_cnt > 0) ? 1 : 0)
#define HIGGS_CHIP_HGC_DQE_RAM_1BIT_ECC_ERR(val) ((val.hgc_dqe_ram_1bit_ecc_err_cnt > 0) ? 1 : 0)
#define HIGGS_CHIP_HGC_DQE_RAM_MUL_BIT_ECC_ERR(val) ((val.hgc_dqe_ram_mul_bit_ecc_err_cnt > 0) ? 1 : 0)
#define HIGGS_CHIP_HGC_ITCT_RAM_1BIT_ECC_ERR(val) ((val.hgc_itct_ram_1bit_ecc_err_cnt > 0) ? 1 : 0)
#define HIGGS_CHIP_HGC_ITCT_RAM_MUL_BIT_ECC_ERR(val) ((val.hgc_itct_ram_mul_bit_ecc_err_cnt > 0) ? 1 : 0)
#define HIGGS_CHIP_HGC_AXI_INT_ERR(val) ((val.hgc_axi_int_err_cnt > 0) ? 1 : 0)
#define HIGGS_CHIP_HGC_FIFO_OVFLD_ERR(val) ((val.hgc_fifo_ovfld_int_err_cnt > 0) ? 1 : 0)
#define HIGGS_CHIP_HGC_1BIT_ECC_ERR(val) ((val.hgc_1bit_ecc_err_cnt > 0) ? 1 : 0)
#define HIGGS_CHIP_DMAC_1BIT_ECC_ERR(val) ((val.emac_1bit_ecc_err_cnt > 0) ? 1 : 0)
#define HIGGS_CHIP_DMAC_MUL_BIT_ECC_ERR(val) ((val.dmac_mul_bit_ecc_err_cnt > 0) ? 1 : 0)
#define HIGGS_CHIP_DMAC_BUS_ERR(val) ((val.dmac_bus_int_err_cnt > 0) ? 1 : 0)
#define HIGGS_CHIP_SEND_IDEN_ADDR_FR_CFG_ERR(val) ((val.send_iden_addr_fr_cfg_err_cnt> 0) ? 1 : 0)
#define HIGGS_CHIP_RCV_OPEN_ADDR_FR_ERR(val) ((val.rcv_open_addr_fr_err_cnt > 0) ? 1 : 0)
#define HIGGS_CHIP_ST_MT_LINK_RX_BUF_OVFLD_ERR(val) ((val.stmt_link_rxbuf_ovfld_err_cnt > 0) ? 1 : 0)
#define HIGGS_CHIP_LINK_LAYER_ELA_BUF_OVFLD_ERR(val) ((val.link_layer_ela_buf_ovfld_err_cnt > 0) ? 1 : 0)
#define HIGGS_CHIP_PS_REQ_FAIL_ERR(val) ((val.ps_req_fail_cnt > 0) ? 1 : 0)
#define HIGGS_CHIP_SPEED_NEGO_FAIL_ERR(val) ((val.speed_nego_fail_cnt > 0) ? 1 : 0)
#define HIGGS_CHIP_TX_TRAIN_PAT_LOCK_LOST_TOUT_ERR(val) ((val.tx_train_pat_lock_lost_tout_cnt > 0) ? 1 : 0)
#define HIGGS_CHIP_DEFAULT_ERR(val) ((val.default_err_cnt > 0) ? 1 : 0)

/* Tx/Rx address frame status inquire register*/
#define HIGGS_SL_TXID_MASK                   0x00000040
#define HIGGS_SL_TXID_ERR                    0x00000040
#define HIGGS_SL_RXID_FAILTYPE_MASK          0x00000038
#define HIGGS_SL_RXID_AFTYPE_ERR             0x00000008
#define HIGGS_SL_RXID_RCV_SOAF_ERR           0x00000010
#define HIGGS_SL_RXID_SFM_ERR                0x00000018
#define HIGGS_SL_RXID_ATOM_ERR               0x00000020
#define HIGGS_SL_RXID_CRC_ERR                0x00000028
#define HIGGS_SL_RXID_DWORD_ERR              0x00000030
#define HIGGS_SL_RXOP_FAILTYPE_MASK          0x00000008
#define HIGGS_SL_RXOP_AFTYPE_ERR             0x00000001
#define HIGGS_SL_RXOP_RCV_SOAF_ERR           0x00000002
#define HIGGS_SL_RXOP_SFM_ERR                0x00000003
#define HIGGS_SL_RXOP_ATOM_ERR               0x00000004
#define HIGGS_SL_RXOP_CRC_ERR                0x00000005
#define HIGGS_SL_RXOP_DWORD_ERR              0x00000006
#define HIGGS_TX_RX_AF_STATUS_IS_ERR(val)  (HIGGS_SL_TXID_IS_ERR(val) | \
                                              HIGGS_SL_RXID_FAILTYPE_IS_ERR(val) | \
                                              HIGGS_SL_RXOP_FAILTYPE_IS_ERR(val))
#define HIGGS_SL_TXID_IS_ERR(val)  ((((val) & HIGGS_SL_TXID_MASK) \
                                               == HIGGS_SL_TXID_ERR) ? 1 : 0)
#define HIGGS_SL_RXID_FAILTYPE_IS_ERR(val)  (((((val) & HIGGS_SL_RXID_FAILTYPE_MASK) \
                                               == HIGGS_SL_RXID_AFTYPE_ERR) \
                                               || (((val) & HIGGS_SL_RXID_FAILTYPE_MASK) \
                                               == HIGGS_SL_RXID_RCV_SOAF_ERR) \
                                               || (((val) & HIGGS_SL_RXID_FAILTYPE_MASK) \
                                               == HIGGS_SL_RXID_SFM_ERR) \
                                               || (((val) & HIGGS_SL_RXID_FAILTYPE_MASK) \
                                               == HIGGS_SL_RXID_ATOM_ERR) \
                                               || (((val) & HIGGS_SL_RXID_FAILTYPE_MASK) \
                                               == HIGGS_SL_RXID_CRC_ERR) \
                                               || (((val) & HIGGS_SL_RXID_FAILTYPE_MASK) \
                                               == HIGGS_SL_RXID_DWORD_ERR)) ? 1 : 0)
#define HIGGS_SL_RXOP_FAILTYPE_IS_ERR(val)  (((((val) & HIGGS_SL_RXOP_FAILTYPE_MASK) \
                                               == HIGGS_SL_RXOP_AFTYPE_ERR) \
                                               || (((val) & HIGGS_SL_RXOP_FAILTYPE_MASK) \
                                               == HIGGS_SL_RXOP_RCV_SOAF_ERR) \
                                               || (((val) & HIGGS_SL_RXOP_FAILTYPE_MASK) \
                                               == HIGGS_SL_RXOP_SFM_ERR) \
                                               || (((val) & HIGGS_SL_RXOP_FAILTYPE_MASK) \
                                               == HIGGS_SL_RXOP_ATOM_ERR) \
                                               || (((val) & HIGGS_SL_RXOP_FAILTYPE_MASK) \
                                               == HIGGS_SL_RXOP_CRC_ERR) \
                                               || (((val) & HIGGS_SL_RXOP_FAILTYPE_MASK) \
                                               == HIGGS_SL_RXOP_DWORD_ERR)) ? 1 : 0)

/* chip fault type */
enum higgs_chip_err_type {
	HIGGS_CHIP_ERR_DPH_PORT_ID = 0,
	HIGGS_CHIP_ERR_DPH_PHY_PORT,	/* 1: PHY do not belong to port */
	HIGGS_CHIP_ERR_DPH_IO_TYPE,	/* 2: IO error */
	HIGGS_CHIP_ERR_DPH_IO_ITCT,	/* 3: ITCT initialization error */

	HIGGS_CHIP_ERR_SAS_MRX,	/* SASMRX receive incorrect xfer_rdy */
	HIGGS_CHIP_ERR_HGC_IOST_RAM_ECC,	/* iost ram ecc error */
	HIGGS_CHIP_ERR_HGC_DQE_RAM_ECC,	/* dq ecc error */
	HIGGS_CHIP_ERR_HGC_ITCT_RAM_ECC,	/* itct ecc error */
	HIGGS_CHIP_ERR_HGC_AXI_INT,	/* AXI error */
	HIGGS_CHIP_ERR_HGC_FIFO_OVFLD_INT,	/* FIFO overflow error */
	HIGGS_CHIP_ERR_HGC_1BIT_ECC,	/* HGC 1bit ECC error */
	HIGGS_CHIP_ERR_DMAC_1BIT_ECC,	/* DMAC 1bit ECC error */
	HIGGS_CHIP_ERR_DMAC_MULTI_BIT_ECC,	/* DMAC multi-bit ECC error */

	HIGGS_CHIP_ERR_PHY_BIST,	/* PHY BIST error */
	HIGGS_CHIP_ERR_BIST_ERROR_PRBS,	/* BIST receive PRBS error */
	HIGGS_CHIP_ERR_STMT_LINK_RX_BUF_OVFL,
	HIGGS_CHIP_ERR_LINK_LAYER_ELAS_BUF_OVFL,	/* Link level lasticity Buffer overflow */
	HIGGS_CHIP_ERR_SPEED_NEGO_FAIL,
	HIGGS_CHIP_ERR_PAT_LOCK_TIMEOUT,	/* TX-TRAINÖÐ£¬Pattern Lock Lost Timeout */
	HIGGS_CHIP_ERR_DMAC_BUS_INT,	/* DMAC bus interrupt error */
	HIGGS_CHIP_ERR_RXOP_CHK_ADDR,	/* RXOP find address error */
	HIGGS_CHIP_ERR_RXOP_CHK_SPD_DISMATCH,	/* RXOP find speed mismatch error */
	HIGGS_CHIP_ERR_RXOP_CHK_PROT,	/* protocol error */
	HIGGS_CHIP_ERR_RXOP_CHK_FEATURE,	/* FEATURE error */
	HIGGS_CHIP_ERR_RXOP_CHK_ICT,	/* ICT error */
	HIGGS_CHIP_ERR_SEND_IDENTIFY_ADDR_FR_CFG,	/* IDENTIFY address config error */
	HIGGS_CHIP_ERR_RCV_OPEN_ADDR_FR,
	HIGGS_CHIP_ERR_PS_REQ_REQ_FAIL,

	HIGGS_CHIP_ERR_BUTT
};

#define HIGGS_REG_WRITE_DWORD(reg, val) (writel((val), (reg)))
#define HIGGS_REG_READ_DWORD(reg) (readl(reg))

/* BEGIN: Added by c00257296, 2015/1/22   PN:arm_server*/
#define HIGGS_REG_WRITE_BYTE(reg, val) (writeb((val), (reg)))
#define HIGGS_REG_READ_BYTE(reg) (readb(reg))
/* END:   Added by c00257296, 2015/1/22 */

#define HIGGS_GLOBAL_REG_READ(card, offset)   \
    HIGGS_REG_READ_DWORD((void *)((card)->card_reg_base + (u64)(offset)))
#define HIGGS_GLOBAL_REG_WRITE(card, offset, val)   \
    HIGGS_REG_WRITE_DWORD(((void *)((card)->card_reg_base + (u64)(offset))), (val))

#define HIGGS_PORT_REG_READ(card, phy_id, offset)   \
    HIGGS_REG_READ_DWORD((void *)((card)->card_reg_base + (u64)(phy_id) * 0x400 + (u64)(offset)))
#define HIGGS_PORT_REG_WRITE(card, phy_id, offset, val)   \
    HIGGS_REG_WRITE_DWORD(((void *)((card)->card_reg_base + (u64)(phy_id) * 0x400 + (u64)(offset))), val)

#define HIGGS_AXIMASTERCFG_REG_READ(card, offset)   \
    HIGGS_REG_READ_DWORD((void *)((card)->card_reg_base + 0x5000 + (u64)(offset)))
#define HIGGS_AXIMASTERCFG_REG_WRITE(card, offset, val)   \
    HIGGS_REG_WRITE_DWORD(((void *)((card)->card_reg_base + 0x5000 + (u64)(offset))), (val))

#define HIGGS_CPLDLED_REG_READ(card, offset)   \
    HIGGS_REG_READ_BYTE((void *)((card)->cpld_reg_base + (u64)(offset)))

#define HIGGS_CPLDLED_REG_WRITE(card, offset, val)   \
    HIGGS_REG_WRITE_BYTE(((void *)((card)->cpld_reg_base + (u64)(offset))), (val))

#define HIGGS_PORT_REG_ADDR(card, phy_id, offset) \
    ((void *)((card)->card_reg_base + (u64)(phy_id) * 0x400 + (u64)(offset)))
#if 0
#define DS_API(lane)           ((0x1FF6c + 8*(15-(u64)lane))*2)
#endif

struct higgs_global {
	u32 higgs_log_level;
#define HIGGS_PROT_BUTT (SAL_MSG_BUTT)
	struct higgs_prot_dispatch prot_dispatcher[HIGGS_PROT_BUTT];	/*ssp and smp */
	struct higgs_prot_dispatch inter_abort_dispatch;	/*abort single and abort dev */
};
extern struct higgs_global higgs_dispatcher;

#ifdef _PCLINT_
#define HIGGS_LOG(module_id, msg_level, msg_code, X, args...)
#define HIGGS_TRACE(debug_level,id,X,args...)					printk(X,##args)
#define HIGGS_ASSERT(expr, args...)
#define HIGGS_TRACE_FRQLIMIT(debug_level,interval,id,X,args...)	printk(X,##args)
#define HIGGS_TRACE_LIMIT(debug_level,interval,count,id,X,args...) 	printk(X,##args)
#else

/* ASSERT*/
#define HIGGS_ASSERT(expr, args...) \
	do { \
		if (unlikely(!(expr))) { \
			printk(KERN_EMERG "BUG! (%s,%s,%d)%s (called from %p)\n", \
				   __FILE__, __FUNCTION__, __LINE__, \
				   # expr, __builtin_return_address(0)); \
			SAS_DUMP_STACK();   \
			args; \
		} \
	} while (0)

#define HIGGS_TRACE(debug_level,id,X,args...)  \
	do { \
		if (debug_level <= sal_global.sal_log_level)\
			DRV_LOG(0/*MID_SAS_INI*/, debug_level,id, X, ## args);	\
	} while (0)

#define HIGGS_TRACE_FRQLIMIT(debug_level,interval,id,X,args...) \
	do { \
		 static unsigned long last = 0; \
		 if ( time_after_eq(jiffies,last+(interval))) { \
		   last = jiffies; \
		   HIGGS_TRACE(debug_level,id, X,## args);\
		 } \
	} while(0)

#define HIGGS_TRACE_LIMIT(debug_level,interval,count,id,X,args...) \
	do { \
		 static unsigned long last = 0; \
		 static unsigned long local_count = 0; \
		 if(local_count<count) {	\
			local_count++;	\
			HIGGS_TRACE(debug_level,id, X,## args);\
		 }	\
		 if ( time_after_eq(jiffies,last+(interval))) { \
		   last = jiffies; \
		   local_count = 0;	\
		 } \
	} while(0)

#endif

#endif
