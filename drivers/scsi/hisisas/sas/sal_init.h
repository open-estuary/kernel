#ifndef _SAL_INIT_H_
#define _SAL_INIT_H_

/****************************************************************************
*                                                                                                                               *
*                                               MACRO/Enum                                      *
*                                                                                                                               *
****************************************************************************/

#define SAL_GET_BOARDTYPE(param)  (((param) & 0xE0) >> 5)
#define SAL_GET_SLOTID(param) 	  ((param) & 0x1f)


/****************************************************************************
*                                                                                                                                *
*                                             struct / union define                                 *
*                                                                                                                                *
****************************************************************************/

/****************************************************************************
*                                                                                                                                *
*                                                function type                                                  *
*                                                                                                                                *
****************************************************************************/

extern u32 sal_get_card_flag(struct sal_card *sal_card);
extern void sal_complete_all_io(struct sal_card *sal_card);
extern void sal_remove_host(struct sal_card *sal_card);
extern s32 sal_get_card_num(struct pci_dev *dev, u8 * card_pos, u8 * card_no);

#endif
