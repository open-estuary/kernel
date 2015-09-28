/*
 * Copyright (c) 2015 Linaro Ltd.
 * Copyright (c) 2015 Hisilicon Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#undef CONFIG_DEBUG_FS
#ifndef _HISI_SAS_H_
#define _HISI_SAS_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <scsi/scsi.h>
#include <scsi/libsas.h>
#include <linux/of_irq.h>

#define DRV_NAME "hisi_sas"
#define DRV_VERSION "v0.1"

#undef SAS_DIF

#define HISI_SAS_MAX_PHYS	9
#define HISI_SAS_MAX_QUEUES	32
#define HISI_SAS_QUEUE_SLOTS 512
#define HISI_SAS_MAX_ITCT_ENTRIES 4096
#define HISI_SAS_MAX_DEVICES HISI_SAS_MAX_ITCT_ENTRIES
#define HISI_SAS_COMMAND_ENTRIES 8192

#define HISI_SAS_ITCT_ENTRY_SZ (sizeof(struct hisi_sas_itct))
#define HISI_SAS_IOST_ENTRY_SZ (sizeof(struct hisi_sas_iost))
#define HISI_SAS_DQ_ENTRY_SZ (sizeof(struct hisi_sas_cmd_hdr))
#define HISI_SAS_CQ_ENTRY_SZ (hisi_hba->hba_info->cq_hdr_sz)
#define HISI_SAS_STATUS_BUF_SZ \
		(sizeof(struct hisi_sas_err_record) + 1024)
#define HISI_SAS_BREAKPOINT_ENTRY_SZ (sizeof(struct hisi_sas_breakpoint))
#define HISI_SAS_COMMAND_TABLE_SZ \
		(((sizeof(union hisi_sas_command_table)+3)/4)*4)

#define HISI_SAS_MAX_SSP_RESP_SZ (sizeof(struct ssp_frame_hdr) + 1024) /* j00310691 see table 118 */
#define HISI_SAS_MAX_SMP_RESP_SZ 1028 /* From spec 9.4.3 (1032-4 byte crc) */
#define HISI_SAS_MAX_STP_RESP_SZ 28 /* DMA setup fis length http://samfreetime.blogspot.co.uk/2012/08/dma-setup-device-to-host-or-host-to.html */

#define HISI_SAS_NAME_LEN 32
#define HISI_SAS_RESET_REG_CNT 2

enum {
	PORT_TYPE_SAS = (1U << 1),
	PORT_TYPE_SATA = (1U << 0)
};

#define DMA_ADDR_LO(addr) ((u32)(addr & 0xffffffff))
#define DMA_ADDR_HI(addr) ((u32)(addr >> 32))

#define DECLARE_INT_HANDLER(handler, idx)\
static irqreturn_t handler##idx(int irq, void *p)\
{\
	return	handler(idx, p);\
}

#define INT_HANDLER_NAME(handler, idx)\
	handler##idx

enum dev_status {
	HISI_SAS_DEV_NORMAL,
	HISI_SAS_DEV_EH
};

enum hisi_sas_dev_type {
	HISI_SAS_DEV_TYPE_STP = 0,
	HISI_SAS_DEV_TYPE_SSP,
	HISI_SAS_DEV_TYPE_SATA
};

struct hisi_sas_phy {
	struct hisi_hba	*hisi_hba;
	struct hisi_sas_port	*port;
	struct asd_sas_phy	sas_phy;
	struct sas_identify	identify;
	struct timer_list	timer;
	u64		port_id; /* from hw */
	u64		dev_sas_addr;
	u64		phy_type;
	u64		frame_rcvd_size;
	u8		frame_rcvd[32];
	u8		phy_attached;
	u8		reserved[3];
	u64		phy_event;
	int		eye_diag_done;
	enum sas_linkrate	minimum_linkrate;
	enum sas_linkrate	maximum_linkrate;
};

struct hisi_sas_port {
	struct asd_sas_port	sas_port;
	u8	port_attached;
	u8	id; /* from hw */
	struct list_head	list;
};

struct hisi_sas_device {
	enum sas_device_type	dev_type;
	struct hisi_hba		*hisi_hba;
	struct domain_device	*sas_device;
	u64 attached_phy;
	u64 device_id;
	u64 running_req;
	struct hisi_sas_itct *itct;
	u8 dev_status;
	u64 reserved;
};

struct hisi_sas_slot {
	struct list_head entry;
	struct sas_task *task;
	struct hisi_sas_port	*port;
	u64	n_elem;
	int	dlvry_queue;
	int	dlvry_queue_slot;
	int	cmplt_queue;
	int	cmplt_queue_slot;
	int	idx;
	int	tmf_idx; /* to be removed */
	void	*cmd_hdr;
	dma_addr_t cmd_hdr_dma;
	void	*status_buffer;
	dma_addr_t status_buffer_dma;
	void *command_table;
	dma_addr_t command_table_dma;
	struct hisi_sas_sge_page *sge_page;
	dma_addr_t sge_page_dma;
#ifdef SAS_DIF
	struct hisi_sas_sge_page *sge_dif_page;
	dma_addr_t sge_dif_page_dma;
#endif
};

struct hisi_sas_tmf_task {
	u8 tmf;
	u16 tag_of_task_to_be_managed;
};

struct hisi_sas_tei {
	struct sas_task	*task;
	struct hisi_sas_cmd_hdr	*hdr;
	struct hisi_sas_port	*port;
	struct hisi_sas_slot	*slot;
	int	n_elem;
	int	iptt;
};

struct hisi_hba;

struct hisi_sas_dispatch {
	int (*hw_init)(struct hisi_hba *hisi_hba);
	int (*phys_init)(struct hisi_hba *hisi_hba);
	int (*interrupt_init)(struct hisi_hba *hisi_hba);
	int (*interrupt_openall)(struct hisi_hba *hisi_hba);
	void (*setup_itct)(struct hisi_hba *hisi_hba,
			   struct hisi_sas_device *device);
	int (*get_free_slot)(struct hisi_hba *hisi_hba, int *q, int *s);
	void (*start_delivery)(struct hisi_hba *hisi_hba);
	int (*prep_ssp)(struct hisi_hba *hisi_hba,
			struct hisi_sas_tei *tei, int is_tmf,
			struct hisi_sas_tmf_task *tmf);
	int (*prep_smp)(struct hisi_hba *hisi_hba,
			struct hisi_sas_tei *tei);
	int (*prep_stp)(struct hisi_hba *hisi_hba,
			struct hisi_sas_tei *tei);
	int (*slot_complete)(struct hisi_hba *hisi_hba,
			     struct hisi_sas_slot *slot, int abort);
	void (*phy_enable)(struct hisi_hba *hisi_hba, int phy_no);
	void (*phy_disable)(struct hisi_hba *hisi_hba, int phy_no);
	void (*hard_phy_reset)(struct hisi_hba *hisi_hba, int phy_no);
	int (*free_device)(struct hisi_hba *hisi_hba,
			   struct hisi_sas_device *dev);
};

struct hisi_sas_hba_info {
	/* We only descibe sizes of structures which */
	/* are different between IP revisions to simplify */
	/* the code. */
	int cq_hdr_sz;
	const struct hisi_sas_dispatch *dispatch;
#ifdef SAS_DIF
	u32 prot_cap;
#endif
};

enum hisi_sas_wq_event {
	CONTROL_PHY,
};

struct hisi_sas_wq {
	struct work_struct	work_struct;
	struct hisi_hba *hisi_hba;
	int phy_no;
	int event;
	int data;
};

struct hisi_hba {
	spinlock_t	lock;

	struct platform_device *pdev;
	struct device *dev;
	struct device_node *np;

	void __iomem *regs;
	void __iomem *ctrl_regs;
	u32 reset_reg[HISI_SAS_RESET_REG_CNT];
	u32 reset_status_reg[HISI_SAS_RESET_REG_CNT];

	u8 sas_addr[SAS_ADDR_SIZE];

	struct hisi_sas_cmd_hdr	*cmd_hdr[HISI_SAS_MAX_QUEUES];
	dma_addr_t	cmd_hdr_dma[HISI_SAS_MAX_QUEUES];
	struct hisi_sas_complete_hdr	*complete_hdr[HISI_SAS_MAX_QUEUES];
	dma_addr_t	complete_hdr_dma[HISI_SAS_MAX_QUEUES];

	struct hisi_sas_initial_fis *initial_fis;
	dma_addr_t	initial_fis_dma;

	int	n_phy;

	int scan_finished;

	struct timer_list timer;
	struct workqueue_struct *wq;
	const struct hisi_sas_hba_info *hba_info;

	int slot_index_count;
	unsigned long *slot_index_tags;

	struct dma_pool *sge_page_pool;
#ifdef SAS_DIF
	struct dma_pool *sge_dif_page_pool;
#endif

	/* SCSI/SAS glue */
	struct sas_ha_struct *sas;
	struct Scsi_Host *shost;

	struct hisi_sas_phy phy[HISI_SAS_MAX_PHYS];
	struct hisi_sas_port port[HISI_SAS_MAX_PHYS];

	int	id;
	int	queue_count;
	char	*int_names;
	struct hisi_sas_slot	*slot_prep;

	struct hisi_sas_device	devices[HISI_SAS_MAX_DEVICES];
	struct dma_pool *command_table_pool;
	struct dma_pool *status_buffer_pool;
	struct hisi_sas_itct *itct;
	dma_addr_t itct_dma;
	struct hisi_sas_iost *iost;
	dma_addr_t iost_dma;
	struct hisi_sas_breakpoint *breakpoint;
	dma_addr_t breakpoint_dma;
	struct hisi_sas_breakpoint *sata_breakpoint;
	dma_addr_t sata_breakpoint_dma;
	struct hisi_sas_slot	*slot_info;
	struct dentry *dbg_dir;
};

#define HISI_SAS_DISP	(hisi_hba->hba_info->dispatch)

extern const struct hisi_sas_hba_info hisi_sas_hba_info_v1_hw;
extern const struct hisi_sas_hba_info hisi_sas_hba_info_v2_hw;

/* Generic HW structures */
/* Delivery queue header */
struct hisi_sas_cmd_hdr {
	/* dw0 */
	u32 dw0;

	/* dw1 */
	u32 dw1;

	/* dw2 */
	u32 dw2;

	/* dw3 */
	u32 iptt:16;
	u32 tptt:16;

	/* dw4 */
	u32 data_transfer_len;

	/* dw5 */
	u32 first_burst_num;

	/* dw6 */
	u32 dif_sg_len:16;
	u32 data_sg_len:16;

	/* dw7 */
	u32 rsvd4:15;
	u32 double_mode:1;
	u32 abort_iptt:16;

	/* dw8 */
	u32 cmd_table_addr_lo;

	/* dw9 */
	u32 cmd_table_addr_hi;

	/* dw10 */
	u32 sts_buffer_addr_lo;

	/* dw11 */
	u32 sts_buffer_addr_hi;

	/* dw12 */
	u32 prd_table_addr_lo;

	/* dw13 */
	u32 prd_table_addr_hi;

	/* dw14 */
	u32 dif_prd_table_addr_lo;

	/* dw15 */
	u32 dif_prd_table_addr_hi;
} __packed;

struct hisi_sas_itct {
	u64 data[16];
} __packed;

struct hisi_sas_iost {
	/* qw0 */
	uint64_t io_type:3;
	uint64_t io_dir:2;
	uint64_t cmd_tlr:2;
	uint64_t send_rpt:1;
	uint64_t phy_id:8;
	uint64_t target_ict:16;
	uint64_t force_phy:1;
	uint64_t tlr_cnt:4;
	uint64_t io_retry_cnt:6;
	uint64_t rsvd0:7;
	uint64_t dir_fmt:1;
	uint64_t prd_dif_src:1;
	uint64_t sgl_mode:1;
	uint64_t pir_present:1;
	uint64_t first_burst:1;
	uint64_t spp_pass_through:1;
	uint64_t io_slot_number:8;

	/* qw1 */
	uint64_t io_status:8;
	uint64_t io_ts:1;
	uint64_t io_rs:1;
	uint64_t io_ct:1;
	uint64_t max_resp_frame_len:9;
	uint64_t rsvd1:11;
	uint64_t chk_len:1;
	uint64_t xfer_tptt:16;
	uint64_t io_rt:1;
	uint64_t io_rd:1;
	uint64_t mis_cnt:8;
	uint64_t rsvd2:6;

	/* qw2 */
	uint64_t xfer_offset:32;
	uint64_t xfer_len:32;

	/* qw3 */
	uint64_t status_buffer_address;
} __packed;

struct hisi_sas_err_record {
	/* dw0 */
	u32 dma_tx_err_type:16;
	u32 dma_rx_err_type:16;

	/* dw1 */
	u32 trans_tx_fail_type;

	/* dw2 */
	u32 trans_rx_fail_type;

	/* dw3 */
	u32 rsvd;
} __packed;

struct hisi_sas_initial_fis {
	struct hisi_sas_err_record err_record;
	struct dev_to_host_fis fis;
	u32 rsvd[3];
} __packed;

struct hisi_sas_breakpoint {
	u8	data[128];	/*io128 byte*/
} __packed;

struct hisi_sas_sge {
	u32 addr_lo;
	u32 addr_hi;
	u32 page_ctrl_0;
	u32 page_ctrl_1;
	u32 data_len;
	u32 data_off;
} __packed;

/* j00310691 Max SMP Request frame size is 44; see table 186 */
struct hisi_sas_command_table_smp {
	u8 bytes[44];
} __packed;

struct hisi_sas_command_table_stp {
	struct	host_to_dev_fis command_fis; /* 20 */
	u8	dummy[12]; /* 12 */
	u8	atapi_cdb[ATAPI_CDB_LEN]; /* 16 */
} __packed;

#define HISI_SAS_SGE_PAGE_CNT SCSI_MAX_SG_SEGMENTS
struct hisi_sas_sge_page {
	struct hisi_sas_sge sge[HISI_SAS_SGE_PAGE_CNT];
} __packed;

#define LUN_SIZE 8

/* See SAS spec, task IU
 */
struct ssp_task_iu {
	u8     lun[LUN_SIZE];	  /* BE */
	u16    _r_a;
	u8     tmf;
	u8     _r_b;
	__be16 tag;		  /* BE */
	u8     _r_c[14];
} __packed;

/* from SAS spec 1.1; table 119 */
struct ssp_command_iu {
	u8     lun[LUN_SIZE];
	u8     _r_a;
	u8     efb_prio_attr;	  /* enable first burst, task prio & attr */
#define EFB_MASK        0x80
#define TASK_PRIO_MASK	0x78
#define TASK_ATTR_MASK  0x07

	u8    _r_b;
	u8     add_cdb_len;	  /* in dwords, since bit 0,1 are reserved */
	union {
		u8     cdb[16];
		struct {
			__le64 long_cdb_addr;	/* bus address, LE */
			__le32 long_cdb_size;	/* LE */
			u8     _r_c[3];
			u8     eol_ds;		/* eol:6,6, ds:5,4 */
		} long_cdb;	  /* sequencer extension */
	};
} __packed;

struct ssp_rdy_iu {
	__be32 requested_offset;  /* BE */
	__be32 write_data_len;	  /* BE */
	__be32 _r_a;
} __packed;

struct hisi_sas_command_table_ssp {
	struct ssp_frame_hdr hdr;
	union {
		struct {
			struct ssp_command_iu task;
			u32 prot[6];
		};
		struct ssp_task_iu ssp_task;
		struct ssp_rdy_iu xfer_rdy;
		struct ssp_response_iu ssp_res;
	} u;
	/* See Table 118 of 1.1 spec */
} __packed;

union hisi_sas_command_table {
	struct hisi_sas_command_table_ssp ssp;
	struct hisi_sas_command_table_smp smp;
	struct hisi_sas_command_table_stp stp;
};

int hisi_sas_scan_finished(struct Scsi_Host *shost, unsigned long time);
void hisi_sas_scan_start(struct Scsi_Host *shost);

void hisi_sas_slot_index_init(struct hisi_hba *hisi_hba);
void hisi_sas_phy_init(struct hisi_hba *hisi_hba, int i);
int hisi_sas_start_phy_layer(struct hisi_hba *hisi_hba);
int hisi_sas_dev_found(struct domain_device *dev);
void hisi_sas_dev_gone(struct domain_device *dev);
int hisi_sas_queue_command(struct sas_task *task, gfp_t gfp_flags);
int hisi_sas_control_phy(struct asd_sas_phy *sas_phy,
			enum phy_func func,
			void *funcdata);
int hisi_sas_abort_task(struct sas_task *task);
int hisi_sas_abort_task_set(struct domain_device *dev, u8 *lun);
int hisi_sas_clear_aca(struct domain_device *dev, u8 *lun);
int hisi_sas_clear_task_set(struct domain_device *dev, u8 *lun);
int hisi_sas_I_T_nexus_reset(struct domain_device *dev);
int hisi_sas_lu_reset(struct domain_device *dev, u8 *lun);
int hisi_sas_query_task(struct sas_task *task);
void hisi_sas_port_formed(struct asd_sas_phy *sas_phy);
void hisi_sas_port_deformed(struct asd_sas_phy *sas_phy);
void hisi_sas_phy_down(struct hisi_hba *hisi_hba, int phy_no, int rdy);
void hisi_sas_bytes_dmaed(struct hisi_hba *hisi_hba, int phy_no);
void hisi_sas_port_notify_formed(struct asd_sas_phy *sas_phy, int lock);
void hisi_sas_slot_task_free(struct hisi_hba *hisi_hba, struct sas_task *task,
			struct hisi_sas_slot *slot);
#ifdef CONFIG_DEBUG_FS
int hisi_sas_debugfs_init(struct hisi_hba *hisi_hba);
void hisi_sas_debugfs_free(struct hisi_hba *hisi_hba);
#endif
#endif
