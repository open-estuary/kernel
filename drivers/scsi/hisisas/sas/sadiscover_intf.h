#ifndef __SADISCOVER_INTF_H__
#define __SADISCOVER_INTF_H__


#define DISC_MAX_DEVICE_NUM 			2048	/*max device number per controller*/

#define DISC_MAX_EXPANDER_PHYS 			256	/*max phys number per expander*/

/*Discover operation*/
#define DISC_OPTION_FULL_OPER 			0x1	/*full discover*/
#define DISC_OPTION_INCREMENTAL_OPER 	0x2	/*incremental discover*/

/*device register result*/
#define DISC_DEV_REGISTER_SUCESS 		0x0	/*device register success*/
#define DISC_DEV_REGISTER_FAILED 		0x1	/*device register failure*/

/*Discover operation done*/
#define DISC_SUCESSED_DONE 				0x0	/*Discover success*/
#define DISC_FAILED_DONE 				0x1	/*Discover failure*/

/*port discover  status*/
#define PORT_DISC_INVALID				0xFFFFFFFF
#define PORT_DISC_START					0x1
#define PORT_DISC_DONE					0x0

/*SMP request frame type*/
#define SMP_REQUEST                     0x40
#define SMP_RESPONSE                    0x41
#define SMP_REPORT_GENERAL 				0x00
#define SMP_DISCOVER_REQUEST 			0x10
#define SMP_DISCOVER_LIST               0x20
#define SMP_CONFIG_ROUTING_INFO 		0x90
#define SMP_REPORT_PHY_SATA 			0x12
#define SMP_PHY_CONTROL                 0x91

/*SMP response result*/
#define SMP_FUNCTION_ACCEPTED 			0x00	/*SMP response success*/
#define SMP_FAIL_PHY_NOT_EXIST			0x10	/*SMP failure:Phy not exist*/
#define SMP_FAIL_PHY_VACANT				0x16	/*SMP failure:Phy vacant */
#define SMP_FAIL_INDEX_NOT_EXIST		    0x11	/*SMP failure:Index not exist*/
#define SMP_FAIL_NOT_SUPPORT_SATA		0x12	/*SMP failure:Phy not support SATA */

/*phy operation type*/
#define PHY_NO_OPERATION                0x00
#define PHY_LINK_RESET                  0x01
#define PHY_HARD_RESET                  0x02
#define PHY_DISABLE                     0x03
#define PHY_CLEAR_ERROR_LOG             0x05
#define PHY_CLEAR_AFFILIATION           0x06
#define PHY_TRANSMIT_SPS_SIGNAL         0x07
#define PHY_CLEAR_IT_NEXUS_LOSS         0x08

/*smp phy control:partial pathway timeout value*/
#define PARTIAL_PATHWAY_TMO_VAL         0x07

#define DISCLIST_DESCRIPTOR_LONG        0
#define DISCLIST_DESCRIPTOR_SHORT       1

#define SAS_DEVICE_NAME_LEN				8
#define SAS_ADDR_LENGTH                 8

#define DISCLIST_DESCRIPTOR_NUM_PER_REQ     10

enum disc_link_rate {
	DISC_LINK_RATE_FREE = 0,
	DISC_LINK_RATE_1_5_G = 0x08,
	DISC_LINK_RATE_3_0_G,
	DISC_LINK_RATE_6_0_G,
	DISC_LINK_RATE_12_0_G,
	DISC_LINK_BUTT
};

enum sas_ini_disc_type {
	SAINI_STEP_DISC = 0,
	SAINI_FULL_DISC,
	SAINI_DISC_BUTT
};

/*SAS Identify Frame*/
struct sas_identify_frame {
	/*Byte 0:
	   Bit0~3:Address Frame Type(0x0)
	   Bit4~6:Device Type
	   Bit7:Reserved */
	u8 device_type;

	/*Byte1:
	   Bit0~3:Reason
	   Bit4~7:Reserved */
	u8 reason;

	/*Byte2:
	   Bit0:Restricted
	   Bit1:SMP Initiator Port
	   Bit2:STP Initiator Port
	   Bit3:SSP Initiator Port
	   Bit4~7:Reserved
	 */
	u8 ini_proto;

	/*Byte3:
	   Bit0:Restricted
	   Bit1:SMP Target Port
	   Bit2:STP Target Port
	   Bit3:SSP Target Port
	   Bit4~7:Reserved */
	u8 tgt_proto;

	u8 device_name[SAS_DEVICE_NAME_LEN];
	u8 sas_addr[SAS_ADDR_LENGTH];
	u8 phy_identifier;
	u8 reserved[7];
};

/*device infomation(include Expander and end device)*/
struct disc_device_data {
	struct list_head device_link;
	u8 card_id;
	u32 port_id;		/*port id from chip */
	u8 top_phy_id;
	u8 phy_id;		/*attache device(Expander or SPC)Phy_id */
	u8 dev_type;
	u8 link_rate;
	/*initiator_ssp_stp_smp:
	   Bit0:Attached Sata Host
	   Bit1:attached smp initiator
	   Bit2:attached stp initiator
	   Bit3:attached ssp initiator */
	u8 ini_proto;
	/*target_ssp_stp_smp
	   Bit0:Attached Sata Device
	   Bit1:attached smp target
	   Bit2:attached stp target
	   Bit3:attached ssp target
	   Bit7:attached sata port selector */
	u8 tgt_proto;

	void *expander; /*end device:point to Expander;Expander:point to self*/
	u8 valid_before;
	u8 valid_now;
	u64 sas_addr;
	u64 father_sas_addr;
	u8 reg_dev_flag;
};

struct disc_exp_data {
	struct list_head exp_list;
	struct disc_device_data *device;
	u8 card_id;
	u32 port_id;		/*port id from chip */
	u8 long_resp;		/*Expander SAS 2.0 response */
	u8 phy_nums;		/*Expander PHY number */
	u8 routing_attr[DISC_MAX_EXPANDER_PHYS];
	u8 configuring;		/*0-expander is free;1-expander is busy; */
	u8 config_route_table;
	u16 routing_idx;
	u16 exp_chg_cnt;
	u64 sas_addr;

	u8 flag;		/*Expander status flag*/
	u8 retries;
	u8 discovering_phy_id;
	u8 send_smp_phy_id;
	u8 num_of_upstream_phys;
	u32 curr_upstream_phy_idx;
	u8 upstream_phy[DISC_MAX_EXPANDER_PHYS];
	u8 phy_chg_cnt[DISC_MAX_EXPANDER_PHYS];
	u64 last_sas_addr[DISC_MAX_EXPANDER_PHYS];
	u64 upstream_sas_addr;
	u8 num_of_downstream_phys;
	u8 downstream_phy[DISC_MAX_EXPANDER_PHYS];
	u32 curr_downstream_phy_idx;
	u32 smp_retry_num;
	u16 curr_phy_idx[DISC_MAX_EXPANDER_PHYS];
	u64 cfg_sas_addr;
	struct disc_exp_data *upstream_exp;
	struct disc_exp_data *downstream_exp;

	struct disc_exp_data *curr_downstream_exp;
	struct disc_exp_data *return_exp;
};

struct disc_port_context {
	u8 card_no;
	u32 port_id;		/*port id from chip */
	u8 port_rsc_idx;
	u8 link_rate;
	enum sal_disc_type disc_type;
	u64 local_sas_addr;
	struct sas_identify_frame sas_identify;	/*SAS Identify */
};

/*Report general request frame*/
struct smp_report_general {
/*null*/
};


/*Discover request frame*/
struct smp_discover {
	u8 reserved_4_8[5];	/*Byte4~8:reserved*/
	u8 phy_identifier;	/*Byte9:PHY IDENTIFIER */
	u8 reserved_10_11[2];	/*byte10~11:reserved*/
};

/*Discover_List request frame*/
struct smp_discover_list {
	u8 reserved_4_7[4];	/*Byte4~7:reserved*/
	u8 start_phy_id;	/*Byte8:STARTING PHY IDENTIFIER */
	u8 num_of_descritor;	/*Byte9:MAXIMUM NUMBER OF DISCOVER LIST DESCRIPTORS */
	/*byte10:
	   Bit0~3:PHY FILTER
	   Bit4~6:Reserved
	   Bit7:IGNORE ZONE GROUP */
	u8 phy_filter;
	u8 desc_type;		/*byte11:Bit0~3DESCRIPTOR TYPE */
	u8 reserved_12_15[4];	/*Byte12~15:reserved*/
	u8 vendor[12];		/*Byte16~27:Vendor-specific */
};

/*SMP config routing info request frame*/
struct smp_config_routing_info {
	u8 expect_exp_chg_cnt[2];	/*Byte4~5:expect Expander change count */
	u8 expander_route_idx[2];	/*Byte6~7 */
	u8 reserved_8;		/*Byte8:reserved*/
	u8 phy_identifier;	/*Byte9 */
	u8 reserved_10_11[2];	/*byte10~11:reserved*/
	u8 disable_exp_route_entry;	/*Byte12:bit0~bit6reserved£¬bit7 used*/
	u8 reserved_13_15[3];	/*Byte13~15 */
	u8 route_sas_addr[SAS_ADDR_LENGTH];	/*Byte16~23:sas address*/
	u8 reserved_24_39[16];	/*Byte24~39:reserved*/
};

/*report phy sata request frame*/
struct smp_report_phy_sata {
	u8 reserved_4_8[5];	/*Byte4~8:reserved*/
	u8 phy_identifier;	/*Byte9 */
	u8 affiliation_context;	/*byte10 */
	u8 reserved_11;		/*byte11:reserved*/
};

/*phy control request frame*/
struct smp_phy_control {
	u8 expect_exp_chg_cnt[2];	/*Byte4~5 */
	u8 reserved_6_8[3];	/*Byte6~8:reserved*/
	u8 phy_identifier;	/*Byte9 */
	u8 phy_operation;	/*byte10 */
	u8 update_pathway_tmo_val;	/*byte11 */
	u8 reserved_12_23[12];	/*Byte12~23:reserved*/
	u8 attach_dev_name[SAS_ADDR_LENGTH];	/*Byte24~31 */
	u8 program_min_link_rate;	/*Byte32 */
	u8 program_max_link_rate;	/*Byte33 */
	u8 reserved_34_35[2];	/*Byte34~35 */
	u8 pathway_tmo_val;	/*Byte36 */
	u8 reserved_37_39[3];	/*Byte37~39 */
};

/*SMP request type*/
union smp_req_type {
	struct smp_report_general report_general;	/*0x00 */
	struct smp_discover discover_req;	/*0x10 */
	struct smp_config_routing_info cfg_route_info;	/*0x90 */
	struct smp_report_phy_sata report_phy_sata;	/*0x12 */
	struct smp_phy_control phy_control;	/*0x91 */
	struct smp_discover_list disc_list;	/*0x20 */
};


/*SMP request frame*/
struct disc_smp_req {
	u8 smp_frame_type;	/*byte0:0x40 */
	u8 smp_function;	/*byte1:SMP fuction */
	u8 resp_len;		/*byte2:Allocated Response Length */
	u8 req_len;		/*byte3:Request Length */
	union smp_req_type req_type;	/*SMP request frame*/
};

/*SMP Report general response frame*/
struct smp_report_general_rsp {
	u8 exp_chg_cnt[2];	/*Byte4~5 */
	u8 exp_route_idx[2];	/*Byte6~7 */
	/*Byte8:
	   *Bit0~6:Reserved
	   *Bit7:Long response
	 */
	u8 long_resp;
	u8 phy_nums;		/*Byte9 */
	/*byte10:
	   Bit0:externally configurable
	   Bit1:configuring
	   Bit2:configures others
	   Bit7:t-t supported */
	u8 configure;
	u8 reserved_11;		/*byte11:reserved*/
	u8 enclosure_id[SAS_DEVICE_NAME_LEN];	/*Byte12~19:Enclosure id */
	u8 reserved_20_29[10];	/*Byte20~29:reserved*/
};

/* smp discover response frame*/
struct smp_discover_rsp {
	u8 exp_chg_cnt[2];	/*Byte4~5 */
	u8 reserved_6_8[3];	/*Byte6~8 */
	u8 phy_identifier;	/*Byte9 */
	u8 reserved_10_11[2];	/*byte10~11:reserved*/
	/*Byte12:
	   Bit0~3:attached reason
	   bit4~6:attached device type */
	u8 attach_dev_type;
	/*Byte13:
	   Bit0~3:link rate */
	u8 link_rate;
	/*Byte14:
	   Bit0:Attached Sata Host
	   Bit1:attached smp initiator
	   Bit2:attached stp initiator
	   Bit3:attached ssp initiator */
	u8 attach_ini_proto;
	/*Byte15:
	   Bit0:Attached Sata Device
	   Bit1:attached smp target
	   Bit2:attached stp target
	   Bit3:attached ssp target
	   Bit7:attached sata port selector */
	u8 attach_tgt_proto;
	u8 sas_addr_hi[4];	/*Byte16~23:SAS Address */
	u8 sas_addr_lo[4];
	u8 attach_sas_addr_hi[4];	/*Byte24~31:Attached SAS Address */
	u8 attach_sas_addr_lo[4];
	u8 attach_phy_identifier;	/*Byte32:Attached Phy Identifier */
	u8 reserved_33_39[7];	/*Byte33~39:reserved*/
	/*Byte40:
	   Bit0~3:Hardware Min physical LinkRate
	   Bit4~7:Programmed Min physical LinkRate */
	u8 min_link_rate;
	/*Byte41:
	   Bit0~3:Hardware Max physical LinkRate
	   Bit4~7:Programmed Max physical LinkRate */
	u8 max_link_rate;
	u8 phy_chg_cnt;		/*Byte 42 */
	/*Byte43:
	   Bit0~3:Partial Pathway Timeout Value
	   Bit7:Virtual Phy */
	u8 pp_tmo_val;
	/*Byte44:
	   Bit0~3:Routing Attribute
	   Bit4~7:Reserved */
	u8 routing_attr;
	/*Byte45:
	   Bit0~6:Connector Type */
	u8 connector_type;
	u8 connector_ele_idx;	/*Byte46 */
	u8 connector_phy_link;	/*Byte47 */
	u8 reserved_48_49[2];	/*Byte48~49:reserved*/
	u8 vendor_spec[2];	/*Byte50~51 */
};

/*short type discover list response*/
struct smp_discoverlist_short {
	u8 phy_id;		/*Byte0:PHY IDENTIFIER */
	u8 func_result;		/*Byte1:FUNCTION RESULT */
	/*Byte2:
	   Bit0~3:ATTACHED REASON
	   Bit4~6:ATTACHED DEVICE TYPE
	   Bit7:Restricted for DISCOVER response byte 12 */
	u8 attached;
	/*Byte3:
	   Bit0~3:NEGOTIATED LOGICAL LINK RATE(0x8-1.5G; 0x9-3G; 0xA-6G; 0xB-12G;) 
	   Bit4~7:Restricted for DISCOVER response byte 13 */
	u8 logic_rate;
	/*Byte4:
	   Bit0:ATTACHED SATA HOST
	   Bit1:ATTACHED SMP INITIATOR
	   Bit2:ATTACHED STP INITIATOR
	   Bit3:ATTACHED SSP INITIATOR
	   Bit4~7:Restricted for DISCOVER response byte 14 */
	u8 attached_ini;
	/*Byte5:
	   Bit0:ATTACHED SATA DEVICE
	   Bit1:ATTACHED SMP TARGET
	   Bit2:ATTACHED STP TARGET
	   Bit3:ATTACHED SSP TARGET
	   Bit4~6:Restricted for DISCOVER response byte 15
	   Bit7:ATTACHED SATAPORT SELECTOR */
	u8 attached_tgt;
	/*Byte6:
	   Bit0~3:ROUTING ATTRIBUTE
	   Bit4~6:reserved
	   Bit7:VIRTUAL PHY */
	u8 rout_attr;
	/*Byte7:
	   Bit0~3:NEGOTIATED PHYSICAL LINK RATE
	   Bit4~7:REASON */
	u8 physic_rate;
	u8 zone_grp;		/*Byte8:ZONE GROUP */
	u8 zone_inside;		/*Byte9:Zone Inside*/
	u8 attached_phy_identifier;	/*byte10:ATTACHED PHY IDENTIFIER */
	u8 phy_chg_cnt;		/*byte11:PHY CHANGE COUNT */
	u8 attached_sas_addr_hi[4];	/*Byte12~19:ATTACHED SAS ADDRESS */
	u8 attached_sas_addr_lo[4];
	u8 reserved_20_23[4];	/*Byte20~23:reserved*/
};

/*Discover List Descriptor*/
union discover_list_desc_type {
	struct smp_discover_rsp disc_resp;
	struct smp_discoverlist_short disc_short_resp;
};

/*Discover List response frame*/
struct smp_discover_list_rsp {
	u8 exp_chg_cnt[2];	/*Byte4~5 */
	u8 reserved_6_7[2];	/*Byte6~7:reserved*/
	u8 start_phy_id;	/*Byte8 */
	u8 num_of_desc;		/*Byte9 */
	u8 phy_filter;		/*byte10:Bit0~3PHY FILTER */
	u8 desc_type;		/*byte11:bit0~3DESCRIPTOR TYPE */
	u8 desc_len;		/*Byte12:DISCOVER LIST DESCRIPTOR LENGTH */
	u8 reserved_13_15[3];	/*Byte13~15:reserved*/
	/*Byte16:
	   Bit0:EXTERNALLY CONFIGURABLE ROUTE TABLE
	   Bit1:CONFIGURING
	   Bit2:ZONE CONFIGURING
	   Bit3:SELF CONFIGURING
	   Bit4~5:reserved
	   Bit6:ZONING ENABLED
	   Bit7:ZONING SUPPORTED */
	u8 configure;
	u8 reserved_17;		/*Byte17:reserved*/
	u8 last_self_idx[2];	/*Byte18~19:LAST SELF-CONFIGURATION STATUS DESCRIPTOR INDEX */
	u8 last_phy_event_idx[2];	/*Byte20~21:LAST PHY EVENT LIST DESCRIPTOR INDEX */
	u8 reserved_22_31[10];	/*Byte22~31:reserved*/
	u8 vendor[16];		/*Byte32~47:Vendor specific */
	union discover_list_desc_type disc_desc;
};

/*config routing info response frame*/
struct smp_config_routing_info_rsp {
	/*CRC? */
};

/*report phy sata response frame*/
struct smp_report_phy_sata_rsp {
	u8 exp_chg_cnt[2];	/*Byte4~5 */
	u8 reserved_6_8[3];	/*Byte6~8:reserved*/
	u8 phy_identifier;	/*Byte9 */
	u8 reserved_10;		/*byte10 */
	/*byte11:
	   Bit0:Affiliation Valid
	   Bit2:Affiliation Supported
	   Bit3:STP I_T NEXUSLOSS Occurred */
	u8 affiliation;
	u8 reserved_12_15[4];	/*Byte12~15 */
	u8 stp_sas_addr_hi[4];	/*Byte16~23 */
	u8 stp_sas_addr_lo[4];
	u8 reg_d2h_fis[20];	/*Byte24~43 */
	u8 reserved_44_47[4];	/*Byte44~47 */
	u8 aff_stp_ini_sas_addr_hi[4];	/*Byte48~55 */
	u8 aff_stp_ini_sas_addr_lo[4];
	u8 i_t_loss_sas_addr_hi[4];	/*Byte56~63 */
	u8 i_t_loss_sas_addr_lo[4];
	u8 reserved_64;		/*Byte64 */
	u8 affi_context;	/*Byte65 */
	u8 curr_affi_context;	/*Byte66 */
	u8 max_affi_context;	/*Byte67 */
};

struct phy_control_rsp {
/*null*/
};

union disc_smp_response_type {
	struct smp_report_general_rsp report_general_rsp;
	struct smp_discover_rsp discover_rsp;
	struct smp_config_routing_info_rsp config_routing_info_rsp;
	struct smp_report_phy_sata_rsp phy_sata_rsp;
	struct phy_control_rsp phy_ctrl_rsp;
	struct smp_discover_list_rsp disc_list_rsp;
};

struct disc_smp_response {
	u8 smp_frame_type;	/*Byte0 */
	u8 smp_function;	/*Byte1 */
	u8 smp_result;		/*Byte2 */
	u8 resp_len;		/*Byte3 */
	union disc_smp_response_type response;
};

enum smp_fail_reason {
	SMP_NO_FAIL = 0,
	SMP_FAIL_REASON_SOFTRESET_NEED,
	SMP_FAIL_REASON_OTHER,
	SMP_FAIL_REASON_BUTT
};

struct disc_topo_param {
	u8 pri_phy_id_0;
	u8 pri_phy_id_1;
	u8 exp_phy_id_0;
	u8 exp_phy_id_1;
};

extern void sal_discover(struct disc_port_context *port_context);
extern void sal_port_down_notify(u8 card_id, u32 port_id);
extern void sal_port_up_notify(u8 card_id, u32 port_id);
extern void sal_notify_to_clr_disc_port_msg(u32 card_id, u32 port_id,
					    u8 port_rsc_idx);

struct disc_event_intf {
	s32(*smp_frame_send) (struct disc_device_data * device_data,
			      struct disc_smp_req * smp_req, u32 req_len,
			      struct disc_smp_response * smp_resp,
			      u32 resp_len);
	s32(*device_arrival) (struct disc_device_data * device_data);
	void (*device_remove) (struct disc_device_data * device_data);
	void (*discover_done) (u8 card_no, u32 port_no);
};

extern s32 sal_init_disc_rsc(u8 card_no,
			     struct disc_event_intf *disc_event_intf);
extern void sal_release_disc_rsc(u8 card_no);
extern s32 sal_disc_module_init(void);
extern void sal_disc_module_remove(void);
extern void sal_set_disc_card_false(u8 card_no);
#endif
