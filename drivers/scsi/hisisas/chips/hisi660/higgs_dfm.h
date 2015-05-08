/*
 * copyright (C) huawei
 */


#ifndef _HIGGS_DFM_H_
#define _HIGGS_DFM_H_

extern s32 higgs_port_loopback(struct sal_card *sal_card, void *argc_in);

extern s32 higgs_bit_stream_operation(struct sal_card *sal_card,
				      enum sal_bitstream_op_type oper,
				      void *argv);

#endif /* _HIGGS_DFM_H_ */
