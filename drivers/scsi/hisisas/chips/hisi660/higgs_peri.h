#ifndef _HIGGS_PERI_H_
#define _HIGGS_PERI_H_

/****************************************************************************
***                     宏(MACRO)/枚举(Enum)定义                          ***
****************************************************************************/

/****************************************************************************
***                     结构体(struct)/ 联合(union) 定义                  ***
****************************************************************************/
#define  HIGGS_MINI_SAS_PORT_COUNT             2
#define  HIGGS_SFP_PAGE_SELECT_BYTE            127
#define  HIGGS_SFP_REGISTER_PAGE_LENGEH        256
#define  HIGGS_SFP_EEPROM_TOTAL_SIZE           640
#define  HIGGS_SFP_EEPROM_PAGE_COUNT           4
#define  HIGGS_LOGIC_PORT_ID_TO_LL_PORT_ID(x)  ((x) & SAL_LOGIC_PORTID_TO_PORTNUM_MASK)

enum higgs_hw_version {
	HIGGS_HW_VERSION_FPGA,
	HIGGS_HW_VERSION_EVB,
	HIGGS_HW_VERSION_C05,
	HIGGS_HW_VERSION_BUTT
};

/****************************************************************************
***                         函数(Function)原型声明                        ***
****************************************************************************/
extern s32 higgs_gpio_operation(struct sal_card *sal_card,
				enum sal_gpio_op_type op_type, void *argv_in);

extern s32 higgs_led_operation(struct sal_card *sal_card,
			       struct sal_led_op_param *led_op);

extern s32 higgs_check_sfp_on_port(struct sal_card *sal_card,
				   u32 * logic_port_id);

extern s32 higgs_get_sfp_info_intf(struct sal_card *sal_card, void *argv_in);

extern s32 higgs_read_wire_eep_intf(struct sal_card *sal_card, void *argv_in);

extern void higgs_subscribe_sfp_event(void);

extern void higgs_unsubscribe_sfp_event(void);

extern s32 higgs_locate_ll_card_and_ll_port_id_by_mini_port_id(u32 mini_port_id,
							       struct higgs_card
							       **ret_ll_card,
							       u32 *
							       ret_ll_port_id);

extern s32 higgs_locate_ll_port_id_by_phy_id(struct higgs_card *ll_card,
					     u32 phy_id, u32 * ll_port_id);

extern s32 higgs_locate_mini_port_id_by_ll_port_id(struct higgs_card *ll_card,
						   u32 ll_port_id,
						   u32 * ret_mini_port_id);

extern s32 higgs_init_peri_device(struct higgs_card *ll_card);

extern s32 higgs_uninit_peri_device(struct higgs_card *ll_card);

extern u8 higgs_get_card_position(u32 card_id);

//extern void Higgs_DelayTriggerSfpEventForInnerPort(struct higgs_card *ll_card, u32  v_uiTimeoutMSecs);
//
//extern void Higgs_CheckAndDelayTriggerSfpEventForMiniPort(struct higgs_card *ll_card, u32  v_uiTimeoutMSecs);
//
//extern void Higgs_DelayTriggerTimerHandle(unsigned long v_ulData);

extern void higgs_delay_trigger_all_port_sfp_event_for_init(struct higgs_card
							    *ll_card,
							    u32 timeout_ms);

extern struct higgs_card *higgs_get_card_info(u32 card_id);
#endif
