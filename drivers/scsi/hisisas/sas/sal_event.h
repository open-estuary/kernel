
#ifndef __SAL_EVNET_H__
#define __SAL_EVNET_H__

/****************************************************************************
*                                                                                                                                 *
*                                         MACRO/Enum define                                    *
*                                                                                                                                 *
****************************************************************************/

#define SAL_INVALID_PORTID            	0xff
#define SAL_INVALID_PHYID            	0xff
#define SAL_INVALID_LOGIC_PORTID      	0xffffffff

#define SAL_TRUE                1	/*true */
#define SAL_FALSE               0	/*false */


#define SAL_PHY_ERR_TOTAL_TIME              1200000	/*ms,20 minutes */
#define SAL_MS_PER_MIN    			        (60000)	/*MS per minute*/

#define SAL_PHY_CHGAGE_SWITCH_OFF           1	/*PHY CHANGE SWITCH */
#define SAL_PHY_CHGAGE_SWITCH_ON            0	/*PHY CHANGE SWITCH */
#define SAL_GET_DEVTYPE_FORMIDFRM(identify) \
                                  (((identify)->dev_type & 0x70) >> 4)

#define SAL_MAX_HWEVENT              SAL_PHY_EVENT_BUTT	/*number of hardware event */

#define SAL_ADD_DEV_SUCCEED         0
#define SAL_ADD_DEV_FAILED          1

#if 0
#define SAL_EXEC_SUCCESS      0
#define SAL_EVENT_EXEC_FAILED       (~0)
#endif

#define SAL_PORT_TIMER_DELAY 50	/*jiffies */

#define SAL_LOGIC_PORT_WIDTH 4

#define SAL_INVALID_SCSIID      0xffffffff	/*invalid SCSI ID */

#define SAL_ADD_DEV_TIMEOUT          (3000)	/*ms */
#define SAL_ABORT_DEV_IO_TIMEOUT     (3000)	/*ms */
#define SAL_DEL_DEV_TIMEOUT          (3000)	/*ms */

#define SAL_DEVICE_ACTIVE       0x1	/*device active for io */
#define SAL_DEVICE_FLASH        0x2	/*device flash for io */
#define SAL_DEVICE_RESTING      0x4	/*device resting for io */
#define SAL_DEVICE_DEREGISTED   0x8	/*device already be deregisted */

enum sal_port_widtch {
	SAL_PORT_WIDTH = 4,
	SAL_PORT0_V2R1C00_WIDTH = 8,

	SAL_PORT_BUTT
};

/*
 *all event processed by queue
*/
enum sal_event_type {
	SAL_EVENT_ADD_DEV = 1,
	SAL_EVENT_DEL_DEV,
	SAL_EVENT_DEL_PORT_DEV,
	SAL_EVENT_DISC_DONE,
	SAL_EVENT_PORT_REPAIR,

	SAL_EVENT_BUTT
};

/*
 *SCSI ID USED STATUS
*/
enum sal_scsi_slot_status {
	SAL_SCSI_SLOT_FREE = 0,
	SAL_SCSI_SLOT_USED,
	SAL_SCSI_SLOT_BUTT
};

#define SAL_GET_WWN_MASK(dev_type)  \
                        ((SASINI_DEVICE_SES == (dev_type))? \
                        (SASINI_EXP_SASADD_MASK):(SASINI_DISK_SASADD_MASK))

#define SAL_GET_PARENT_WWN(parent_addr,mask)  \
                        ((SASINI_HBA_SASADDR == (parent_addr))? \
                        (parent_addr):(parent_addr & SASINI_EXP_SASADD_MASK))

#if 0
#define SAL_STATUS_SUCCESS  	0
#define SAL_STATUS_FAILED   	(~0)
#endif

enum sal_status {
	SAL_STATUS_SUCCESS = 0,
	SAL_STATUS_RETRY,
	SAL_STATUS_FAILED
};

#define SAL_COMMON_STRUCTURE_LEN    128

#define SAL_LOCAL_PHY_LINK_RESET                0x01
#define SAL_LOCAL_PHY_HARD_RESET                0x02
#define SAL_LOCAL_PHY_NOTIFY_SPIN_UP            0x03
#define SAL_LOCAL_PHY_BROADCAST_ASYN_EVENT      0x04
#define SAL_LOCAL_PHY_COMINIT_OOB               0x05

/****************************************************************************
*                                                                                                                                *
*                                 struct) / union define                                *
*                                                                                                                                * 
****************************************************************************/

struct sal_phy_event {
	u32 card_id;
	u32 phy_id;
	u32 fw_port_id;
	enum sal_phy_hw_event_type event;
	enum sal_link_rate link_rate;
};

struct sal_phy_rate_chk_param {
	u8 phy_rate;
	u8 phy_id;
	u32 port_id;
};

/*
 *record information common information between disc module and driver
*/
struct sal_disk_info {
	struct sal_port *port;
	struct sal_card *card;
	u8 direct_attatch;	/* if direct connect */
	u8 ini_type;		/* initiate type */
	enum sal_dev_type type;	/* target type */
	u32 link_rate;
	u32 slot_id;
	u32 logic_id;
	u32 virt_phy;
	u32 *status;		/*flag that operation is success or not*/
	u64 sas_addr;
	u64 parent;
};

struct sal_port_event {
	struct sal_port *port;
};

union sal_event_info {
	struct sal_disk_info disk_add;
	struct sal_disk_info disk_del;
	struct sal_port_event port_event;

};

/*
 * all inforamtion of event
*/
struct sal_event {
	union sal_event_info info;	/*information of event */
	struct list_head list;	/*event queue list */
	unsigned long time;	/*Time of event happened;for debug */
	enum sal_event_type event;	/*event type */
	SAS_STRUCT_COMPLETION *disc_compl;	/*device disc completion */
	SAS_STRUCT_COMPLETION *event_compl;	/*device disc completion */
	char comm_str[SAL_COMMON_STRUCTURE_LEN];	/* common structure */
};

enum sal_is_err {
	SAL_PHY_OK = 0,
	SAL_PHY_ERROR,
	SAL_PHY_WARN,
	SAL_PHY_IS_ERR_BUTT
};

/****************************************************************************
*                                                                                                                                 *
*                                          Function type define                                    *
*                                                                                                                                *
****************************************************************************/

typedef s32(*sal_hw_event_cbfn_t) (struct sal_port * sal_port,
				   struct sal_phy * sal_phy,
				   struct sal_phy_event info);

extern s32 sal_all_phy_down_handle(struct sal_port *port);
extern s32 sal_event_handler(void *param);
extern s32 sal_start_work(void *argv_in, void *argv_out);
extern s32 sal_disc_add_disk(struct disc_device_data *disc_dev);
extern void sal_disc_del_disk(struct disc_device_data *disc_dev);
extern void sal_disc_done(u8 card_no, u32 port_no);
extern void sal_add_dev_cb(struct sal_dev *dev, u32 status);
extern void sal_del_dev_cb(struct sal_dev *dev, enum sal_status status);
extern void sal_abort_dev_io_cb(struct sal_dev *dev, enum sal_status status);
extern s32 sal_stop_card_disc(u32 card_no);
extern void sal_free_dev_rsc(u8 card_no, u32 scsi_id, u32 chan_id, u32 lun_id);
extern void sal_notify_report_dev_in(u8 card_no,
				     u32 scsi_id, u32 chan_id, u32 lun_id);
extern s32 sal_ctrl_reset_chip_by_card(void *argv_in, void *argv_out);
extern s32 sal_phy_event(struct sal_port *sal_port,
			 struct sal_phy *sal_phy,
			 struct sal_phy_event phy_event_info);
extern void sal_event_init(void);
extern void sal_event_exit(void);
extern void sal_routine_timer(unsigned long card_no);
extern struct sal_event *sal_get_free_event(struct sal_card *sal_card);
extern void sal_put_free_event(struct sal_card *sal_card,
			       struct sal_event *sal_event);
extern void sal_put_all_dev(u8 card_no);
extern void SAL_DiscRedo(u32 card_id, u32 port_id, u64 sas_addr);

extern s32 sal_abort_dev_io(struct sal_card *sal_card, struct sal_dev *dev);
extern void sal_add_disk_report(struct sal_card *sal_card,
				struct sal_dev *sal_dev);
extern void sal_intr_event2_reset_phy(struct sal_card *sal_card, u32 phy_id);
extern void sal_port_led_func(struct work_struct *work);
extern void sal_port_led_execute(struct sal_card *sal_card);
extern void sal_turn_off_all_led(struct sal_card *sal_card);
extern s32 sal_set_port_dev_state(struct sal_card *sal_card,
				  struct sal_port *port,
				  enum sal_dev_event event);
extern void sal_del_port_report(struct sal_card *sal_card,
				struct sal_port *port);
extern void sal_re_init_port(struct sal_card *sal_card,
			     struct sal_port *sal_port);
extern void sal_clr_err_handler_notify_by_dev(struct sal_card *sal_card,
					      struct sal_dev *sal_dev,
					      bool mutex_flag);
extern void sal_ll_port_ref_oper(struct sal_card *sal_card,
				 struct sal_port *sal_port,
				 enum sal_refcnt_op_type ref);
extern void sal_add_dev_event_thread(struct sal_card *sal_card,
				     struct sal_event *sal_event);
extern bool sal_is_all_phy_down(struct sal_port *sal_port);
extern s32 sal_handle_all_port_rsc_clean(struct sal_card *sal_card,
					 struct sal_event *sal_event);
#endif
