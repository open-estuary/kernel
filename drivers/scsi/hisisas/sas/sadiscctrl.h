
#ifndef __SADISCCTRL_H__
#define __SADISCCTRL_H__

struct disc_event {
	struct list_head event_list;
	u8 card_no;
	u8 port_no;		/* the port id of chip */
	u8 port_rsc_idx;	/* the index of sal port rsc */
	u8 link_rate;
	u64 local_addr;
	bool process_flag;
	enum sas_ini_disc_type disc_type;
	struct sas_identify_frame identify;
};

struct disc_event_ctrl {
	struct list_head event_head;
	SAS_SPINLOCK_T event_lock;
	u32 cnt;
};

#endif
