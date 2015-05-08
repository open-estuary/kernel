
#ifndef __SADISCOVER_H__
#define __SADISCOVER_H__

#include "sadiscover_intf.h"


#define DISC_MAX_PORT_NUM                       16
#define DISC_MAX_CARD_NUM                       16

#define DISC_MAX_EXP_NUM                        128
#define DISC_MAX_RESET_TIMES                    2
#define DISC_RESULT_TRUE                        1
#define DISC_RESULT_FALSE                       0

/*in discover,smp retry times*/
#define DISC_MAX_SMP_RETRIES                    3	
#define DISC_WAIT_TIME                          25

/*Identify deive is ssp ini*/
#define DISC_DEV_IS_SSPINI                      0x08
#define DISC_PHYID_INVALID                      0xFFFF

#define SAL_LANUCH_DISC_INTERVAL   100

/*report general retry times*/
#define DISC_MAX_REPORTGEN_RETRIES              20
/*configring retry delay:jiffies 25*4 = 100ms */
#define DISC_CONFIGRING_RETRY_TIME              25

#define DISC_MAX_REDO_DISC_NUM                   5

#define DISCARD_MALLOC_DEVICE_MEM       0x00000001
#define DISCARD_MALLOC_EXPANDER_MEM     0x00000002
#define DISCARD_REGISTER_INTF_SUCESS    0x00000004

#define DISC_CHECK_PRI_PRI_ATTACHED(phy_id,topo_param) ((((phy_id) < ((topo_param)->pri_phy_id_0 + 4)) \
                                                          && ((phy_id) >= (topo_param)->pri_phy_id_0)) \
                                                          || (((phy_id) < ((topo_param)->pri_phy_id_1 + 4)) \
                                                          && ((phy_id) >= (topo_param)->pri_phy_id_1)))

#define DISC_CHECK_EXP_EXP_ATTACHED(phy_id,topo_param) ((((phy_id) < ((topo_param)->exp_phy_id_0 + 4)) \
                                                          && ((phy_id) >= (topo_param)->exp_phy_id_0)) \
                                                          || (((phy_id) < ((topo_param)->exp_phy_id_1 + 4)) \
                                                          && ((phy_id) >= (topo_param)->exp_phy_id_1)))


/*get configring bit from report general frame*/
#define DISC_GET_CONFIGURING_BIT(resp)  (((resp)->configure >> 1) & 0x01)

/*get configurable bit from report general frame*/
#define DISC_GET_CONFIGURABLE_BIT(resp) ((resp)->configure & 0x01)

/*get long response bit from report general frame*/
#define DISC_GET_LONGRESP_BIT(resp) (((resp)->long_resp >> 7) & 0x01)

/*get expander routing index from report general frame*/
#define DISC_GET_ROUTING_INDEX(resp)   \
    ((((u16)resp->exp_route_idx[0]) << 8) | resp->exp_route_idx[1])

/*get expander change count from report general frame*/
#define DISC_GET_EXP_CHANGE_COUNT(resp)  \
    (((u16)resp->exp_chg_cnt[0]) << 8 | resp->exp_chg_cnt[1])


#define DISC_GET_VIRTUAL_FROM_DISCOVERRESP(resp) \
                        ((resp)->pp_tmo_val >> 7)

#define DISC_GET_ATTACHED_SASADDR_HI(resp) \
                        ((((u64)resp->attach_sas_addr_hi[0]) << 24)| \
                         (((u64)resp->attach_sas_addr_hi[1]) << 16)| \
                         (((u64)resp->attach_sas_addr_hi[2]) << 8)| \
                          (resp->attach_sas_addr_hi[3]))


#define DISC_GET_ATTACHED_SASADDR_LO(resp) \
                        ((((u64)resp->attach_sas_addr_lo[0]) << 24)| \
                         (((u64)resp->attach_sas_addr_lo[1]) << 16)| \
                         (((u64)resp->attach_sas_addr_lo[2]) << 8)| \
                          (resp->attach_sas_addr_lo[3]))

#define DISC_GET_STP_SASADDR_HI(resp) \
                        ((((u64)resp->stp_sas_addr_hi[0]) << 24)| \
                         (((u64)resp->stp_sas_addr_hi[1]) << 16)| \
                         (((u64)resp->stp_sas_addr_hi[2]) << 8)| \
                          (resp->stp_sas_addr_hi[3]))

#define DISC_GET_STP_SASADDR_LO(resp)  \
                        ((((u64)resp->stp_sas_addr_lo[0]) << 24)| \
                         (((u64)resp->stp_sas_addr_lo[1]) << 16)| \
                         (((u64)resp->stp_sas_addr_lo[2]) << 8)| \
                           (resp->stp_sas_addr_lo[3]))

#define DISC_GET_STPINI_SASADDR_HI(resp)  \
                        ((((u64)resp->aff_stp_ini_sas_addr_hi[0]) << 24)| \
                         (((u64)resp->aff_stp_ini_sas_addr_hi[1]) << 16)| \
                         (((u64)resp->aff_stp_ini_sas_addr_hi[2]) << 8)| \
                          (resp->aff_stp_ini_sas_addr_hi[3]))

#define DISC_GET_STPINI_SASADDR_LO(resp) \
                        ((((u64)resp->aff_stp_ini_sas_addr_lo[0]) << 24)| \
                         (((u64)resp->aff_stp_ini_sas_addr_lo[1]) << 16)| \
                         (((u64)resp->aff_stp_ini_sas_addr_lo[2]) << 8)| \
                          (resp->aff_stp_ini_sas_addr_lo[3]))

#define DISC_GET_ATTACHED_DEVTYPE(resp) \
                                 ((resp->attach_dev_type >> 4) & 0x0f)

#define DISC_TGT_IS_NODEVICE(ucTgtTpye) (0 == ((ucTgtTpye) & 0x0f))
#define DISC_INI_IS_NODEVICE(ucIniTpye) (0 == ((ucIniTpye) & 0x0f))

#define DISC_GET_DEVTYPE_FROM_IDENTIFY(y)   ((y) >> 4)

#define DISC_GET_SASADDR_FROM_IDENTIFY(identify) \
                ((((u64)identify->sas_addr[0]) << 56)| \
                 (((u64)identify->sas_addr[1]) << 48)| \
                 (((u64)identify->sas_addr[2]) << 40)| \
                 (((u64)identify->sas_addr[3]) << 32)| \
                 (((u64)identify->sas_addr[4]) << 24)| \
                 (((u64)identify->sas_addr[5]) << 16)| \
                 (((u64)identify->sas_addr[6]) << 8)| \
                 ((identify)->sas_addr[7]))

#define DISC_GET_SASADDR_FROM_CONFIG(req)  \
                            ((((u64)req->route_sas_addr[0]) << 56)| \
                             (((u64)req->route_sas_addr[1]) << 48)| \
                             (((u64)req->route_sas_addr[2]) << 40)| \
                             (((u64)req->route_sas_addr[3]) << 32)| \
                             (((u64)req->route_sas_addr[4]) << 24)| \
                             (((u64)req->route_sas_addr[5]) << 16)| \
                             (((u64)req->route_sas_addr[6]) << 8)| \
                             ((req)->route_sas_addr[7]))

#define DISC_GET_DEVTYPE_FROM_SHORTDESC(short_resp) \
                        ((short_resp->attached >> 4) & 0x07)

#define DISC_GET_ROUTEATTR_FROM_SHORTDESC(short_resp) \
                            (short_resp->rout_attr & 0x0f)

#define DISC_GET_VIRTUAL_FROM_SHORTDESC(short_resp) \
                            ((short_resp->rout_attr >> 7) & 0x01)

#define DISC_GET_ADDRLO_FROM_SHORTDESC(short_resp) \
                            (((u64)(short_resp->attached_sas_addr_lo[0]) << 24) | \
                             ((u64)(short_resp->attached_sas_addr_lo[1]) << 16) | \
                             ((u64)(short_resp->attached_sas_addr_lo[2]) << 8) | \
                             (short_resp->attached_sas_addr_lo[3]) )

#define DISC_GET_ADDRHI_FROM_SHORTDESC(short_resp) \
                            (((u64)(short_resp->attached_sas_addr_hi[0]) << 24) | \
                             ((u64)(short_resp->attached_sas_addr_hi[1]) << 16) | \
                             ((u64)(short_resp->attached_sas_addr_hi[2]) << 8) | \
                             (short_resp->attached_sas_addr_hi[3]) )
                             
#define DEV_IS_STP_TARGET(dev)        ((dev)->tgt_proto & 0x04)

#define DEV_IS_SATA_TARGET(dev)       ((dev)->tgt_proto & 0x01)

#define DSIC_GET_DEV_TYPE(tgt)			((tgt) & 0x0f)

#define DSIC_GET_DEV_RATE(rate)		((rate) & 0x0f)

#define DISC_GET_SAS_ADDR(sas_addr,n) (((sas_addr) >> (8*(7 - (n)))) & 0xff)

#define DISC_IDENTIY_IS_EXP         (EXPANDER_DEVICE << 4)

#define DISC_MIN_LINK_RATE          (DISC_LINK_RATE_1_5_G << 4)

enum disc_timer_status {
	DISC_TIMER_STATUS_FREE = 1,
	DISC_TIMER_STATUS_WAITING,
	DISC_TIMER_STATUS_OUT,
	DISC_TIMER_STATUS_BUTT
};

enum disc_disc_status {
	DISC_STATUS_NOT_START = 1,
	DISC_STATUS_STARTING,
	DISC_STATUS_UPSTREAM,
	DISC_STATUS_DOWNSTREAM,
	DISC_STATUS_CONFIG_ROUTING,
	DISC_STATUS_REPORT_PHY_SATA,
	DISC_STATUS_BUTT
};

enum disc_port_status {
	PORT_STATUS_FORBID = 1,
	PORT_STATUS_PERMIT,
	PORT_STATUS_BUTT
};

enum disc_exp_status {
	DISC_EXP_NOT_PROCESS = 1,
	DISC_EXP_UP_DISCOVERING,
	DISC_EXP_DOWN_DISCOVERING,
	DISC_EXP_BUTT
};

enum disc_exp_routing {
	SAS_DIRECT_ROUTING = 0,
	SAS_SUBTRUCTIVE_ROUTING,
	SAS_TABLE_ROUTING,
	SAS_BUTT_ROUTING
};

#define SAINI_NEED_REGDEV 	0x1
#define SAINI_NOT_REGDEV  	0x2
#define SAINI_FREE_REGDEV   0x3

#define SAINI_UNKNOWN_PORTINDEX 	0xff

struct disc_abnormal {
	u32 reset_cnt;
	u64 sas_addr;
};

/*Discover port infomation*/
struct disc_port {
	struct list_head disc_device;
	struct list_head disc_expander;
	struct task_struct *disc_thread;
	u8 link_rate;
	SAS_ATOMIC ref_cnt;	
	u32 port_id;		/*port id of chip */
	u32 new_disc;
	u32 discover_status;
	u32 abnormal_dev_nums;
	u32 disc_switch;
	void *card;	
	u64 local_sas_addr;
	struct disc_abnormal abnormal[DISC_MAX_DEVICE_NUM];
	struct sas_identify_frame identify;
	struct list_head disc_head;
	SAS_SPINLOCK_T disc_lock;
	enum sas_ini_disc_type disc_type;
};

struct disc_card {
	u8 card_no;
	u32 disc_card_flag;
	u32 inited;
	void *device_mem;
	SAS_SPINLOCK_T device_lock;
	struct list_head free_device;
	void *expander_mem;
	SAS_SPINLOCK_T expander_lock;
	struct list_head free_expander;
	struct disc_port disc_port[DISC_MAX_PORT_NUM];
	struct disc_topo_param disc_topo;
};

typedef void (*sas_ini_timer_cbfn_t) (unsigned long);

struct disc_msg {
	struct list_head disc_list;
	u8 port_rsc_idx;
	bool proc_flag;
	enum sas_ini_disc_type port_disc_type;

};

extern u32 sal_get_smp_req_len(u8 func);
extern struct disc_card disc_card_table[DISC_MAX_CARD_NUM];
extern void sal_launch_to_redo_disc(struct disc_exp_data *exp_dev);
#endif
