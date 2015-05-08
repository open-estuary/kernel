#ifndef _HIGGS_DEV_H_
#define _HIGGS_DEV_H_

s32 higgs_reg_dev(struct sal_dev * sal_dev);

s32 higgs_dereg_dev(struct sal_dev *sal_dev);

void higgs_recycle_dev(struct higgs_card *ll_card, struct higgs_device *ll_dev);

s32 higgs_dereg_port(struct sal_card *sal_card, u32 port_id, u8 port_op_code);

s32 higgs_dev_operation(struct sal_dev *sal_dev, enum sal_dev_op_type dev_opt);

u64 higgs_get_dev_sas_addr_by_dev_id(struct higgs_card *ll_card, u32 dev_id);

#endif
