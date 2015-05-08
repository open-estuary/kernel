/*
 * copyright (C) huawei
 */

#ifndef _HIGGS_EH_H_
#define _HIGGS_EH_H_

/*
extern void Higgs_EhInit(void);
*/
extern enum sal_cmnd_exec_result higgs_eh_abort_operation(enum
							  sal_eh_abort_op_type
							  op_type,
							  void *argv_in);
extern s32 higgs_send_tm_msg(enum sal_tm_type tm_type, struct sal_msg *sal_msg);
extern enum sal_cmnd_exec_result higgs_send_abort(struct higgs_req *higgs_req);
extern enum sal_cmnd_exec_result higgs_prepare_abort(struct higgs_req
						     *higgs_req);

extern s32 higgs_process_rsp_abort(struct higgs_req *higgs_req);

/*
extern bool Higgs_FindChipFatalInReg(struct sal_card *v_pstSALCard);
extern bool Higgs_FindChipFatalInStat(struct sal_card *v_pstSALCard, struct higgs_chip_err_fatal_stat  *v_pstChipEFStat);
*/
extern s32 higgs_chip_fatal_check(struct sal_card *sal_card,
				  enum sal_chip_chk_ret *chip_chk_ret);
extern u32 higgs_look_up_iptt(struct higgs_card *ll_card,
			      struct sal_msg *abort_msg);

extern enum higgs_tm_type higgs_convert_sal_tm_type(enum sal_tm_type sal_type);

extern s32 higgs_chip_clock_check(struct sal_card *sal_card,
				  enum sal_card_clock *sas_clk_result);

extern s32 higgs_chip_fatal_chkin_tm_timeout(struct sal_card *sal_card,
					     struct sal_msg *argc_in);
extern struct higgs_req *higgs_get_req_by_iptt(struct higgs_card *ll_card,
					       u32 iptt);
extern void higgs_req_timeout_check(struct sal_card *sal_card);

s32 higgs_process_abort_cfg_efg_err_rsp(struct higgs_req *higgs_req);

#endif
