
#ifndef _SAL_ERR_HANDLE_H_
#define _SAL_ERR_HANDLE_H_
/****************************************************************************
*                                                                                                                                *
*                                         MACRO)/Enum                                                                 *
*                                                                                                                                *
****************************************************************************/
#define SAL_MAX_ERRHANDLE_ACTION_NUM    100
#define SAL_MAX_ERRHANDE_RETRIES_TIME   (3)

/*define enum of IO error process operation.*/
enum sal_err_handle_action {
	SAL_ERR_HANDLE_ACTION_UNKNOWN = 0,
	SAL_ERR_HANDLE_ACTION_RESENDMSG = 1,	/*re_send message for Jam. */
	SAL_ERR_HANDLE_ACTION_LL_RETRY = 2,	/* interface of using LL Retry msg */
	SAL_ERR_HANDLE_ACTION_HDRESET_RETRY,
	SAL_ERR_HANDLE_ACTION_ABORT_RETRY,
	SAL_ERR_HANDLE_ACTION_RESET_DEV,
	SAL_ERR_HANDLE_ACTION_RESET_HOST,
	SAL_ERR_HANDLE_ACTION_BUTT
};

/* operation object of error handle  */
enum sal_err_handle_type {
	SAL_ERR_HANDLE_CLASS_MSG = 10,
	SAL_ERR_HANDLE_CLASS_DEV,
	SAL_ERR_HANDLE_CLASS_CARD,
	SAL_ERR_HANDLE_CLASS_BUTT
};

#define SAL_DEFAULT_INNER_ERRHANDER_TMO 	(2000)	/* timeout of inner error handle, unit:ms */
#define SAL_MAX_JAM_TMO 					(10000)	/* time of maximum promise for JAM, unit:ms */

#define SAL_SRC_FROM_FRAME    0x1
#define SAL_SRC_FROM_DRIVER   0x2
/****************************************************************************
*                                                                                                                                *
*                                 struct / union                                                                          *
*                                                                                                                                *
****************************************************************************/

struct sal_eh_cmnd {
	u32 card_id;
	u32 scsi_id;
	u64 unique_id;
	u32 src_direction;
	enum sal_eh_dev_rst_type reset_type;
};

struct sal_ll_notify {
	struct list_head notify_list;
	enum sal_err_handle_action op_code;	/* opration code.*/
	u64 start_jiffies;	/* start time. */
	bool process_flag;	/* if event need to process ,TURE-need to process,false-ignore */
	void *info;		/*evaluate according to actual instance. (struct sal_msg,struct sal_dev,struct sal_card) */
	struct sal_eh_cmnd eh_info;	/*necessary information for eh £¬it is used when msg and dev pointer could not be sent into. */
};

/****************************************************************************
*                                                                                                                                *
*                                function pointer type define                                                *
*                                                                                                                                *
****************************************************************************/

typedef s32(*sal_err_io_handler_t) (void *);

/****************************************************************************
*                                                                                                                                *
*                                                  function type                                                           *
*                                                                                                                                *
****************************************************************************/
extern void sal_tm_done(struct sal_msg *sal_msg);
extern void sal_abort_single_io_done(struct sal_msg *sal_msg);
extern s32 sal_eh_abort_cmnd_process(void *argv_in, void *argv_out);
extern s32 sal_eh_abort_command(struct drv_ini_cmd * ini_cmd);
extern s32 sal_eh_device_reset(struct drv_ini_cmd * ini_cmd);
extern enum sal_eh_wait_func_ret sal_default_smp_wait_func(struct sal_msg
							   *sal_msg,
							   struct sal_port
							   *port);
extern enum sal_eh_wait_func_ret sal_inner_eh_smp_wait_func(struct sal_msg
							    *sal_msg,
							    struct sal_port
							    *port);
extern enum sal_eh_wait_func_ret sal_default_tm_wait_func(struct sal_msg
							  *sal_msg);
extern enum sal_eh_wait_func_ret sal_inner_tm_wait_func(struct sal_msg
							*sal_msg);
extern void sal_err_notify_thread_remove(struct sal_card *sal_card);
extern s32 sal_err_notify_thread_init(struct sal_card *sal_card);
extern s32 sal_err_handler(void *param);
extern void sal_init_err_handler_op(void);
extern void sal_remove_err_handler_op(void);
extern s32 sal_err_notify_remove(struct sal_card *sal_card);
extern s32 sal_err_notify_rsc_init(struct sal_card *sal_card);
extern s32 sal_add_msg_to_err_handler(struct sal_msg *sal_msg,
				      enum sal_err_handle_action op_code,
				      unsigned long jiff);
extern s32 sal_add_dev_to_err_handler(struct sal_dev *sal_dev,
				      enum sal_err_handle_action op_code);
extern s32 sal_add_card_to_err_handler(struct sal_card *sal_card,
				       enum sal_err_handle_action op_code);

extern void sal_clean_dev_io_in_err_handle(struct sal_card *sal_card,
					   struct sal_dev *sal_dev);

extern void sal_free_all_active_io(struct sal_card *sal_card);

extern void sal_free_left_io(struct sal_card *sal_card);

extern s32 sal_err_handle_fail_done_io(struct sal_msg *sal_msg);
extern void sal_recover_tmo_msg(struct sal_card *sal_card);
extern s32 sal_link_reset_smp(struct sal_dev *sal_dev,
			      struct disc_smp_req *smp_req,
			      u32 req_len,
			      struct disc_smp_req *smp_req_resp,
			      u32 resp_len, sal_eh_wait_smp_func_t wait_func);

extern s32 sal_abort_single_io(struct sal_msg *orginal_msg);
extern void sal_abort_smp_request(struct sal_card *sal_card,
				  struct sal_msg *sal_msg);
extern s32 sal_enter_err_hand_mutex_area(struct sal_card *sal_card);
extern void sal_leave_err_hand_mutex_area(struct sal_card *sal_card);

#endif
