
#ifndef _SAL_DEV_H_
#define _SAL_DEV_H_


#define SAL_IO_MAX_SECTORS 		1024	/*128 sgl * 4K = 512 K 512K /512 = 1K secter*/
#define SAL_MAX_SGL_SEGMENT  	4096	/*one page size*/
#define SAL_DEV_REF_COUNT_ONCE  (1)

struct sal_dev_fsm {
	enum sal_dev_state new_status;
	 s32(*act_func) (struct sal_dev *);
};

extern void sal_dev_fsm_init(void);
extern s32 sal_dev_init(struct sal_card *sal_card);
extern s32 sal_dev_remove(struct sal_card *sal_card);
extern struct sal_dev *sal_get_free_dev(struct sal_card *sal_card);
extern void sal_del_dev(struct sal_card *sal_card, struct sal_dev *sal_dev,
			bool in_list_lock);
extern s32 sal_dev_state_chg(struct sal_dev *sal_dev, enum sal_dev_event event);
extern struct sal_dev *sal_get_dev_by_sas_addr(struct sal_port *port,
					       u64 sas_addr);
extern struct sal_dev *sal_get_dev_by_sas_addr_in_dev_list_lock(struct sal_port
								*port,
								u64 sas_addr);
extern void sal_get_dev(struct sal_dev *dev);
extern void sal_put_dev(struct sal_dev *dev);
extern void sal_dev_check_put_last(struct sal_dev *dev, bool in_list_lock);
extern void sal_put_dev_in_dev_list_lock(struct sal_dev *dev);
extern void sal_abnormal_put_dev(struct sal_dev *dev);
extern enum sal_dev_state sal_get_dev_state(struct sal_dev *sal_dev);
extern struct sal_dev *sal_get_dev_by_scsi_id(struct sal_card *card,
					      u32 scsi_id);
extern s32 sal_slave_alloc(struct drv_device_address * cmd, void **device);
extern void sal_slave_destroy(struct drv_device * argv_in);

extern s32 sal_initiator_ctrl(void *argv_in, void *argv_out);
extern s32 sal_device_ctrl(void *argv_in, void *argv_out);

#endif
