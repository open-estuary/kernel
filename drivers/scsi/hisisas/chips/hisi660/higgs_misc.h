#ifndef _HIGGS_MISC_H_
#define _HIGGS_MISC_H_


/****************************************************************************
***                                               MACRO/Enum define                        ***
****************************************************************************/
#define  HIGGS_BAR_GLOBAL   0xff
#define  HIGGS_PORT_REG_MAX_OFFSET		(0x800 + 0x324 + 0x4)
#define  HIGGS_GLOBAL_REG_MAX_OFFSET	(0x7d8 + 0x4)

/****************************************************************************
***                                             struct/union define               ***
****************************************************************************/

/****************************************************************************
***                                          Function  Type Define                       ***
****************************************************************************/

extern s32 higgs_chip_operation(struct sal_card *sal_card,
				enum sal_chip_op_type chip_op, void *data);

extern s32 higgs_reg_operation(struct sal_card *sal_card,
			       enum sal_reg_op_type reg_op, void *data);

extern s32 higgs_comm_ll_operation(struct sal_card *sal_card,
				   enum sal_common_op_type comm_op, void *data);

extern void higgs_reg_write_part(u64 reg, u32 low, u32 high, u32 value);

#endif /* _HIGGS_MISC_H_ */
