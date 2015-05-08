#ifndef _HIGGS_PORT_H
#define _HIGGS_PORT_H

/****************************************************************************
***                                               MACRO/Enum define                        ***
****************************************************************************/

/****************************************************************************
***                                             struct/union define               ***
****************************************************************************/

/****************************************************************************
***                                          Function  Type Define                       ***
****************************************************************************/

extern s32 higgs_all_port_start_work(struct sal_card *sal_card);

extern s32 higgs_port_ref_count_opt(struct higgs_card *ll_card,
				    struct sal_port *sal_port,
				    enum sal_refcnt_op_type sal_ref_opt);

extern s32 higgs_port_operation(struct sal_card *sal_card,
				enum sal_port_op_type port_op, void *arg);

extern s32 higgs_del_port(struct sal_port *sal_port);

extern void higgs_init_port_src(struct higgs_card *ll_card,
				struct higgs_port *ll_port);

extern struct higgs_port *higgs_get_free_port(struct higgs_card *ll_card);

extern void higgs_notify_port_id(struct sal_card *sal_card);

extern struct higgs_port *higgs_get_port_by_port_id(struct higgs_card *ll_card,
						    u32 chip_port_id);

extern s32 higgs_set_up_port_id(struct higgs_card *ll_card,
				u32 * logic_port_id);

extern void higgs_mark_ll_port_on_phy_up(struct higgs_card *ll_card,
					 struct higgs_port *ll_port,
					 u32 phy_id, u32 chip_port_id);

extern void higgs_mark_ll_port_on_phy_down(struct higgs_card *ll_card,
					   struct higgs_port *ll_port,
					   u32 phy_id);

extern void higgs_mark_ll_port_on_stop_phy_rsp(struct higgs_card *ll_card,
					       struct higgs_port *ll_port,
					       u32 phy_id);

extern s32 higgs_notify_sal_wire_hotplug(struct higgs_card *ll_card,
					 u32 ll_port_id);

#endif
