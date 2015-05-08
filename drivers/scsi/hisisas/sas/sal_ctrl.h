
#ifndef _SAL_CTRL_H_
#define _SAL_CTRL_H_

/****************************************************************************
*                                                                                                                                *
*                                         MACRO/Enum define                                                        *
*                                                                                                                                *
****************************************************************************/
#define SAL_QUEUE_COUNT (256)

#define ENABLE_ADD_TO_CTRL  (0)	/*could add task. */
#define DISABLE_ADD_TO_CTRL (1)	/*could not add task. */

/****************************************************************************
*                                                                                                                                *
*                                 struct/union define                                                                   *
*                                                                                                                                *
****************************************************************************/

struct sal_ctrl_msg {
	struct list_head list;
	u32 op_code;		/* operation code */
	void *argv_in;		/* input parameter*/
	void *argv_out;		/* output parameter */
	s32 status;		/* return value*/
	bool wait;
	 s32(*raw_func) (void *, void *);	/*process raw type  */
	SAS_STRUCT_COMPLETION compl;	/* signal*/
};

struct sal_ctrl {
	u32 card_id;
	u32 can_add;
	SAS_SPINLOCK_T ctrl_list_lock;	/* lockup list. */
	struct list_head free_list;	/* free list */
	struct list_head process_list;	/*waiting for process list */
	struct sal_ctrl_msg msg[SAL_QUEUE_COUNT];
	struct task_struct *task;
};

struct sal_probe_remove_info {
	struct pci_dev *dev;
	struct pci_device_id *dev_id;
};

enum sal_ctrl_msg_type {
	SAL_CTRL_RAW = 1000,	/* raw type */
	SAL_CTRL_HOST_ADD,	/*load SAS chip */
	SAL_CTRL_HOST_REMOVE, /*unload SAS chip*/
	SAL_CTRL_BUTT            /* maximum*/
};

struct sal_async_info {
	u32 delay_time;
	 s32(*exec_func) (void *argv_in, void *argv_out);
	void *argv_in;
	void *argv_out;

};

/****************************************************************************
*                                                                                                                                 *
*                                          Function                                                                          *
*                                                                                                                                 *
****************************************************************************/

s32 sal_send_raw_ctrl_wait(u32 card_id,
			   void *argv_in,
			   void *argv_out, s32(*raw_func) (void *, void *));
void sal_send_raw_ctrl_no_wait(u32 card_id,
			       void *argv_in,
			       void *argv_out, s32(*raw_func) (void *, void *));
s32 sal_free_ctrl_msg(u32 card_id, struct sal_ctrl_msg *msg);
s32 sal_card_ctrl_init(void);
void sal_card_ctrl_remove(void);

s32 sal_send_to_ctrl_wait(u32 card_id,
			  u32 op_code,
			  void *argv_in,
			  void *argv_out, s32(*raw_func) (void *, void *));

void sal_send_to_ctrl_no_wait(u32 card_id,
			      u32 op_code,
			      void *argv_in,
			      void *argv_out, s32(*raw_func) (void *, void *));

extern s32 sal_ctrl_async_func(void *argv_in, void *argv_out);
#endif
