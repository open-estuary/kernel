#ifndef _SAL_IO_H_
#define _SAL_IO_H_

extern volatile u32 sal_simulate_io_fatal;

#define SAL_DISK_SECTOR_SIZE_512B  512	/*sector size 512 bytes*/

#define SAL_IO_STAT_REQUEST   0x1
#define SAL_IO_STAT_RESPONSE  0x2

#define SAL_SLOW_IO_NOT_IDLE  0xFFFFFFFFFFFFFFFF

#define SAL_IO_NO_SENSE  0x00

enum sal_eh_wait_func_ret {
	SAL_EH_WAITFUNC_RET_PORTDOWN = 31,	/*port down*/
	SAL_EH_WAITFUNC_RET_TIMEOUT,	/*timeout*/
	SAL_EH_WAITFUNC_RET_ABORT,	/*abort*/
	SAL_EH_WAITFUNC_RET_WAITOK,	/*success*/
	SAL_EH_WAITFUNC_RET_BUTT
};

typedef enum sal_eh_wait_func_ret (*sal_eh_wait_smp_func_t) (struct sal_msg *
							     sal_msg,
							     struct sal_port *
							     port);
typedef enum sal_eh_wait_func_ret (*sal_eh_wait_tm_func_t) (struct sal_msg *
							    sal_msg);
#define SAL_GET_ENCAP_WAITJIFF(jiff,card)	((sal_global.dly_time[(card)->card_no]) + (jiff))

extern s32 sal_abort_task_set(struct sal_card *sal_card,
			      struct sal_dev *sal_dev);
extern s32 sal_send_disc_smp(struct disc_device_data *dev_data,
			     struct disc_smp_req *smp_req, u32 req_len,
			     struct disc_smp_response *smp_resp, u32 resp_len);
extern s32 sal_queue_cmnd(struct drv_ini_cmd * ini_cmd);
extern s32 sal_send_smp(struct sal_dev *sal_dev, u8 * req, u32 req_len,
			u8 * resp, u32 resp_len1,
			sal_eh_wait_smp_func_t wait_func);
extern void sal_io_cnt_Sum(struct sal_card *sal_card, u32 io_direct);
extern void sal_set_msg_result(struct sal_msg *msg);
extern void sal_disk_io_collect_stat(struct sal_msg *msg, u8 type);
extern void sal_outer_io_stat(struct sal_msg *msg);
extern void sal_port_io_collect(struct sal_msg *msg);
/*
extern DRV_IOCNT_PORT_DIRECTION_E sal_msg_dir_to_frame_stat_dir(struct sal_msg
								*msg);
*/
extern s32 sal_start_io(struct sal_dev *dev, struct sal_msg *msg);
extern void sal_msg_done(struct sal_msg *msg);
#endif
