#ifndef _HIGGS_STUB_H_
#define _HIGGS_STUB_H_

/****************************************************************************
***                                               MACRO/Enum define                        ***
****************************************************************************/

/****************************************************************************
***                                             struct/union define               ***
****************************************************************************/

/****************************************************************************
***                                          Function  Type Define                       ***
****************************************************************************/
extern void higgs_stub_simulate_sfp_plug_mini_port(struct higgs_card *ll_card,
						   u32 ll_port_id,
						   enum sal_wire_type type,
						   u32 timeout_ms);

//extern void higgs_StubEnterDebugMode(u32 mini_port_id);

//extern void higgs_StubExitDebugMode(u32 mini_port_id);

extern bool higgs_stub_in_debug_mode(u32 mini_port_id);

extern s32 higgs_stub_sfp_on_port(u32 mini_port_id, bool * on_port);

extern s32 higgs_stub_sfp_page_select(u32 mini_port_id, u32 page_sel);

extern s32 higgs_stub_read_sfp_eeprom(u32 mini_port_id,
				      u8 * page, u32 len, u32 offset);

extern s32 higgs_stub_led_ctrl(u32 mini_port_id, enum sal_led_op_type led_op);

extern void higgs_stub_fill_default_config_info(struct higgs_card *ll_card);

extern s32 higgs_stub_read_copper_cable_eeprom(u8 * page, u32 len, u32 offset);

#endif /* _HIGGS_STUB_H_ */
