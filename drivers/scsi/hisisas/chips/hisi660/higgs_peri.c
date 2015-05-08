/******************************************************************************

                  版权所有 (C) 华为技术有限公司

******************************************************************************
     	文 件 名   : higgs_peri.c
     	版 本 号   : 初稿
     	作    者   : 
     	生成日期   : 2014年7月30日
     	最近修改   :
     	功能描述   : higgs外设的管理，访问，初始化 
     	函数列表   :
     	修改历史   :
     	1.日    期   : 2014年7月29日
     	  作    者   :  z00171046
     	  修改内容   : 创建文件

******************************************************************************/
#include "higgs_common.h"
#include "higgs_peri.h"
#include "higgs_stub.h"
#include "higgs_pv660.h"
#include "higgs_intr.h"

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

/* C05硬件平台特性  */
#define C05_BOARD_INNER_PORT_LL_ID  0x0
//#define C05_CPU_OUTER_PORT_LL_ID    0x1
#define C05_CPU_OUTER_PORT_MINI_ID  0x1
/* #define C05_EXP_OUTER_PORT_MINI_ID  0x0 */

#define CPLD_REG_BASE               (0x100)
#define CPLD_LED_MINISAS1_REG       (CPLD_REG_BASE + 0x219)
#define CPLD_LED_12G_LINK			((u8)(0x4 | 0x2 | 0x0))
#define CPLD_LED_6G_LINK            ((u8)(0x4 | 0x0 | 0x1))
#define CPLD_LED_FAULT_LINK         ((u8)(0x0 | 0x2 | 0x1))
#define CPLD_LED_NO_LINK            ((u8)(0x4 | 0x2 | 0x1))

#define CPLD_SFP_PRSNT_REG           (CPLD_REG_BASE + 0x218)
#define CPLD_SFP_PRSNT_MINISAS1_MASK ((u8)0x1)
#define CPLD_SFP_PRSNT_MINISAS1      ((u8)0x0)

#define I2C_9545_CHNL                				1
#define I2C_9545_ADDR                				0xE0
#define I2C_9545_ROUTE_TO_NONE                      0x0
#define I2C_9545_ROUTE_TO_MINISAS1_PORT             (0x1 << 0)
#define I2C_MINISAS1_PORT_ADDR						0xA0

#define I2C_9545_ROUTE_TO_MINISAS1_REDRIVER         (0x1 << 3)
#define I2C_MINISAS1_REDRIVER_LOWER_ADDR     		0xA2
#define I2C_MINISAS1_REDRIVER_UPPER_ADDR            0xA4
#define C05_MINISAS1_REDRIVER_CHNL_EQ_PARAM         ((u8)((0xC << 4) | (0x2 << 2) | (0x2 << 0)))
#define REDRIVER_CHNL_1_4_CTRL_OFFSET               0x5
#define REDRIVER_CHNL_1_4_CTRL_MASK                 0x1f
#define REDRIVER_CHNL_1_4_CTRL_ENABLE               0x10
#define REDRIVER_CHNL_1_4_CTRL_DISABLE              0x1f
#define REDRIVER_CHNL_5_8_CTRL_OFFSET               0x5
#define REDRIVER_CHNL_5_8_CTRL_MASK                 0x1f
#define REDRIVER_CHNL_5_8_CTRL_ENABLE               0x10
#define REDRIVER_CHNL_5_8_CTRL_DISABLE              0x1f
#define REDRIVER_CHNL_NUM_MAX						0x8

#define SFP_EEPROM_PAGE_SEL_OFFSET  127

int bsp_i2c_simple_read( int chan, unsigned char slave_addr, unsigned int sub_addr, char* obuffer, unsigned int rx_len , int sub_addr_len);
int bsp_i2c_simple_write(int chan, unsigned char slave_addr, unsigned int sub_addr, char* ibuffer, unsigned int tx_len , int sub_addr_len);
static s32 higgs_sfp_page_select(struct higgs_card *ll_card, u32 ll_port_id,
				 u32 page_sel);

static s32 higgs_read_sfp_eeprom(struct higgs_card *ll_card,
				 u32 ll_port_id,
				 u8 * page, u32 rd_len, u32 offset);

static s32 higgs_simulate_read_sfp_eeprom_inner_port(struct higgs_card *ll_card,
						     u32 ll_port_id,
						     u8 * page,
						     u32 rd_len, u32 offset);

static s32 higgs_sfp_on_port(struct higgs_card *ll_card, u32 ll_port_id,
			     bool * is_on_port);

static s32 higgs_led_ctrl(struct higgs_card *ll_card, u32 ll_port_id,
			  enum sal_led_op_type led_op);

static void higgs_sfp_event_handler(void *param);

static bool higgs_is_board_inner_port(struct higgs_card *ll_card,
				      u32 ll_port_id);

static u32 higgs_get_hw_version(void);

static s32 higgs_init_sas_redriver_addr(struct higgs_card *ll_card);

static s32 higgs_config_sas_redriver_channel_param(struct higgs_card *ll_card);

static s32 higgs_enable_sas_redriver_channel(struct higgs_card *ll_card);

static s32 higgs_disable_sas_redriver_channel(struct higgs_card *ll_card);

static bool higgs_ll_port_exist(struct higgs_card *ll_card, u32 ll_port_id);

static s32 higgs_open_i2c_9545_route(u32 route);

static void higgs_close_i2c_9545_route(void);

//void Higgs_NotifySfpEvent(unsigned long v_ulData);

enum higgs_i2c_offset_size {
	HIGGS_I2C_OFFSET_NONE = 0,
	HIGGS_I2C_OFFSET_ONE_BYTE = 1
/*	HIGGS_I2C_OFFSET_TWO_BYTE, */
/*	HIGGS_I2C_OFFSET_BUTT  */
};

static s32 higgs_i2c_wirte(u8 chan,
			   u8 addr,
			   u32 offset,
			   enum higgs_i2c_offset_size offset_size,
			   u8 * buff, u32 buff_size);

static s32 higgs_i2c_read(u8 chan,
			  u8 addr,
			  u32 offset,
			  enum higgs_i2c_offset_size offset_size,
			  u8 * buff, u32 buff_size);

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
static struct higgs_card *higgs_peri_cards[HIGGS_MAX_CARD_NUM];

static struct intr_event_handler sfp_event_handle = {
#if defined(C05_VERSION_TEST)
	INT_MINISASHD,
#else
	0,
#endif
	higgs_sfp_event_handler
};

static const u32 i2c_mini_sas1_redriver_chan_addr[REDRIVER_CHNL_NUM_MAX] = {
	I2C_MINISAS1_REDRIVER_LOWER_ADDR,
	I2C_MINISAS1_REDRIVER_LOWER_ADDR,
	I2C_MINISAS1_REDRIVER_LOWER_ADDR,
	I2C_MINISAS1_REDRIVER_LOWER_ADDR,
	I2C_MINISAS1_REDRIVER_UPPER_ADDR,
	I2C_MINISAS1_REDRIVER_UPPER_ADDR,
	I2C_MINISAS1_REDRIVER_UPPER_ADDR,
	I2C_MINISAS1_REDRIVER_UPPER_ADDR
};

static const u32 redriver_chan_offset[REDRIVER_CHNL_NUM_MAX] = {
	0x1, 0x2, 0x3, 0x4, 0x1, 0x2, 0x3, 0x4
};

/*----------------------------------------------*
 * 对外接口实现                                   *
 *----------------------------------------------*/

/*****************************************************************************
 函 数 名  : higgs_locate_ll_card_and_ll_port_id_by_mini_port_id
 功能描述  : 通过单板MINI SAS端口ID，定位相应的LL Card和LL端口ID
 输入参数  : u32 v_uiMiniSasPortId,
 输出参数  : struct higgs_card **ret_ll_card,
		  u32 *ret_ll_port_id
 返 回 值  : s32
*****************************************************************************/
s32 higgs_locate_ll_card_and_ll_port_id_by_mini_port_id(u32 mini_port_id,
							struct higgs_card
							**ret_ll_card,
							u32 * ret_ll_port_id)
{
	u32 card_id;
	u32 ll_port_id;
	struct higgs_card *ll_card;
	struct higgs_config_info *cfg_info;

	/* 参数检查 */
	HIGGS_ASSERT(NULL != ret_ll_card, return ERROR);
	HIGGS_ASSERT(NULL != ret_ll_port_id, return ERROR);
	HIGGS_ASSERT(HIGGS_INVALID_PORT_ID != mini_port_id, return ERROR);

	/* 遍历匹配 */
	for (card_id = 0; card_id < HIGGS_MAX_CARD_NUM; card_id++) {
		ll_card = higgs_peri_cards[card_id];
		if (NULL == ll_card)
			continue;
		cfg_info = &ll_card->card_cfg;
		for (ll_port_id = 0; ll_port_id < HIGGS_MAX_PORT_NUM;
		     ll_port_id++)
			if (((const u32)mini_port_id) ==
			    cfg_info->port_mini_id[ll_port_id]) {
				*ret_ll_card = ll_card;
				*ret_ll_port_id = ll_port_id;
				return OK;
			}
	}

	return ERROR;
}

/*****************************************************************************
 函 数 名  : higgs_locate_mini_port_id_by_ll_port_id
 功能描述  : 通过LL端口ID，定位相应的单板MINI SAS端口ID
 输入参数  : struct higgs_card *ll_card,
		   u32  ll_port_id
 输出参数  : u32 *mini_port_id
 返 回 值  : s32
 *****************************************************************************/
s32 higgs_locate_mini_port_id_by_ll_port_id(struct higgs_card * ll_card,
					    u32 ll_port_id, u32 * mini_port_id)
{
	u32 tmp_mini_port;

	/* 参数检查 */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(ll_port_id < HIGGS_MAX_PORT_NUM, return ERROR);

	/* 从配置中读取  */
	tmp_mini_port = ll_card->card_cfg.port_mini_id[ll_port_id];
	if (HIGGS_INVALID_PORT_ID != tmp_mini_port) {
		*mini_port_id = tmp_mini_port;
		return OK;
	}

	return ERROR;
}

/*****************************************************************************
 函 数 名  : higgs_locate_ll_port_id_by_phy_id
 功能描述  : 通过LL Card引用和PHY ID，定位相应的LL层端口ID
 输入参数  : struct higgs_card *ll_card,
		   u32 phy_id
 输出参数  : u32 *ret_ll_port_id
 返 回 值  : s32
*****************************************************************************/
s32 higgs_locate_ll_port_id_by_phy_id(struct higgs_card * ll_card,
				      u32 phy_id, u32 * ret_ll_port_id)
{
	u32 i;
	u32 port_bit_map;
	u32 tgt_bit_mask;

	/* 参数检查 */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(phy_id < HIGGS_MAX_PHY_NUM, return ERROR);

	/* 遍历匹配 */
	tgt_bit_mask = (0x1 << phy_id);
	for (i = 0; i < HIGGS_MAX_PORT_NUM; i++) {
		port_bit_map = ll_card->card_cfg.port_bitmap[i];
		/* PHY命中 */
		if (0 != (port_bit_map & tgt_bit_mask)) {
			*ret_ll_port_id = i;
			return OK;
		}
	}
	return ERROR;
}

/*****************************************************************************
 函 数 名  : higgs_subscribe_sfp_event
 功能描述  : 向BSP订阅SFP插拔事件
 输入参数  : 无
 输出参数  : 无
 返 回 值  : void
*****************************************************************************/
void higgs_subscribe_sfp_event(void)
{
	u32 hw_ver = higgs_get_hw_version();

	if (HIGGS_HW_VERSION_C05 != hw_ver) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Operation unsupported on platform %d", hw_ver);
		return;
	}
        if(NULL == sal_peripheral_operation.register_int_handler)
       {
             HIGGS_TRACE(OSP_DBL_MAJOR, 171,
                        "sal_peripheral_operation.register_int_handler is null.");
       }else if (OK != sal_peripheral_operation.register_int_handler(&sfp_event_handle))
		HIGGS_TRACE(OSP_DBL_MAJOR, 171,
                       "Subscribe SFP hotplug event failed");
}

/*****************************************************************************
 函 数 名  : higgs_unsubscribe_sfp_event
 功能描述  : 向BSP取消订阅SFP插拔事件
 输入参数  : 无
 输出参数  : 无
 返 回 值  : void
*****************************************************************************/
void higgs_unsubscribe_sfp_event(void)
{
	u32 hw_ver = higgs_get_hw_version();

	if (HIGGS_HW_VERSION_C05 != hw_ver) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Operation unsupported on platform %d", hw_ver);
		return;
	}
        if(NULL == sal_peripheral_operation.unregister_int_handler)
       {
               HIGGS_TRACE(OSP_DBL_MAJOR, 171,
                        "sal_peripheral_operation.unregister_int_handler is null");
        }else if (OK != sal_peripheral_operation.unregister_int_handler(sfp_event_handle.int_event))
		HIGGS_TRACE(OSP_DBL_MAJOR, 171,
			    "UnSubscribe SFP hotplug event failed");
}

/*****************************************************************************
 函 数 名  : higgs_init_peri_device
 功能描述  : 初始化外设
 输入参数  : struct higgs_card *ll_card
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
s32 higgs_init_peri_device(struct higgs_card *ll_card)
{
	u32 hw_ver = higgs_get_hw_version();

	/* 参数检查 */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(ll_card->card_id < HIGGS_MAX_CARD_NUM, return ERROR);

	/* 暂时保存LL CARD引用 */
	HIGGS_ASSERT(NULL == higgs_peri_cards[ll_card->card_id], return ERROR);
	higgs_peri_cards[ll_card->card_id] = ll_card;

	/* 版本过滤 */
	if (HIGGS_HW_VERSION_C05 != hw_ver) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Operation skipped on platform %d", hw_ver);
		return OK;
	}

	/* 初始化SAS Redriver地址 */
	if (OK != higgs_init_sas_redriver_addr(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MINOR, 171,
			    "Card:%d init redriver addr failed",
			    ll_card->card_id);
		return ERROR;
	}
	HIGGS_TRACE(OSP_DBL_INFO, 171, "Card:%d init redriver addr succeeded",
		    ll_card->card_id);

	/* 禁用SAS Redriver通道 */
	if (OK != higgs_disable_sas_redriver_channel(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MINOR, 171,
			    "Card:%d disable redriver channel failed",
			    ll_card->card_id);
		return ERROR;
	}
	HIGGS_TRACE(OSP_DBL_INFO, 171,
		    "Card:%d disable redriver channel succeeded",
		    ll_card->card_id);

	/* 配置SAS Redriver信号参数 */
	if (OK != higgs_config_sas_redriver_channel_param(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MINOR, 171,
			    "Card:%d config redriver channel param failed",
			    ll_card->card_id);
		return ERROR;
	}
	HIGGS_TRACE(OSP_DBL_INFO, 171,
		    "Card:%d config redriver channel param succeeded",
		    ll_card->card_id);

	/* 使能SAS Redriver通道 */
	if (OK != higgs_enable_sas_redriver_channel(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MINOR, 171,
			    "Card:%d enable redriver channel failed",
			    ll_card->card_id);
		return ERROR;
	}
	HIGGS_TRACE(OSP_DBL_INFO, 171,
		    "Card:%d enable redriver channel succeeded",
		    ll_card->card_id);

	return OK;
}

/*****************************************************************************
 函 数 名  : higgs_init_peri_device
 功能描述  : 反初始化外设
 输入参数  : struct higgs_card *ll_card
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
s32 higgs_uninit_peri_device(struct higgs_card * ll_card)
{
	u32 hw_ver = higgs_get_hw_version();

	/* 参数检查 */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(ll_card->card_id < HIGGS_MAX_CARD_NUM, return ERROR);

	/* 移除LL CARD引用 */
	HIGGS_ASSERT(NULL != higgs_peri_cards[ll_card->card_id], return ERROR);
	higgs_peri_cards[ll_card->card_id] = NULL;

	/* 版本过滤 */
	if (HIGGS_HW_VERSION_C05 != hw_ver) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Operation skipped on platform %d", hw_ver);
		return OK;
	}

	/* 禁用SAS Redriver通道 */
	if (OK != higgs_disable_sas_redriver_channel(ll_card)) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Card:%d disable redriver channel failed",
			    ll_card->card_id);
		return ERROR;
	}
	HIGGS_TRACE(OSP_DBL_INFO, 171,
		    "Card:%d disable redriver channel succeeded",
		    ll_card->card_id);

	return OK;
}

/*****************************************************************************
 函 数 名  : Higgs_RegOperation
 功能描述  :GPIO操作的quark层统一接口，
          PV660/Hi1610处理器的SAS模块没有隶属的GPIO管脚，因此该接口实现为空
 输入参数  :struct sal_card *sal_card,
		enum sal_led_op_type gpio_op_type,
 		void *argv_in
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
s32 higgs_gpio_operation(struct sal_card * sal_card,
			 enum sal_gpio_op_type gpio_op_type, void *argv_in)
{
	/* PV660/Hi1610平台没有单独的SAS GPIO操作 */
	HIGGS_REF(sal_card);
	HIGGS_REF(gpio_op_type);
	HIGGS_REF(argv_in);
	HIGGS_TRACE(OSP_DBL_INFO, 151,
		    "GPIO operation unsupported on higgs platform.");

	return ERROR;
}

/*****************************************************************************
 函 数 名  : higgs_check_sfp_on_port
 功能描述  : 判断端口SFP是否在位
 输入参数  : struct sal_card *sal_card,
			u32 *logic_port_id
 输出参数  :
 返 回 值  : s32
*****************************************************************************/
s32 higgs_check_sfp_on_port(struct sal_card * sal_card, u32 * logic_port_id)
{
	u32 ll_port_id;
	bool is_on_port;
	struct higgs_card *ll_card;

	/* 参数检查 */
	HIGGS_ASSERT(NULL != sal_card, return ERROR);
	HIGGS_ASSERT(NULL != logic_port_id, return ERROR);

	ll_card = (struct higgs_card *)sal_card->drv_data;
	HIGGS_ASSERT(NULL != ll_card, return ERROR);

	ll_port_id = HIGGS_LOGIC_PORT_ID_TO_LL_PORT_ID(*logic_port_id);
	HIGGS_ASSERT(ll_port_id < HIGGS_MAX_PORT_NUM, return ERROR);

	/* 读取SFP在位信息  */
	if (OK != higgs_sfp_on_port(ll_card, ll_port_id, &is_on_port)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4576,
			    "Card:%d check sfp presence failed on port:%d",
			    ll_card->card_id, ll_port_id);
		return ERROR;
	}

	return is_on_port ? OK : ERROR;
}

/*****************************************************************************
 函 数 名  : higgs_get_sfp_info_intf
 功能描述  : 读取SFP EEPROM的某个Page
 输入参数  : struct sal_card *sal_card,
			void *argv_in
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
s32 higgs_get_sfp_info_intf(struct sal_card * sal_card, void *argv_in)
{
	struct higgs_card *ll_card = NULL;
	u32 page_sel = 0;
	u32 shift_offset = 0;
	u32 len = 0;
	u32 ll_port_id = 0;
	u8 *tmp_page = NULL;
	struct sal_sfp_get_info_param *sfp_get_info = NULL;

	/* 参数检查  */
	HIGGS_ASSERT(NULL != sal_card, return ERROR);
	HIGGS_ASSERT(NULL != argv_in, return ERROR);
	ll_card = (struct higgs_card *)sal_card->drv_data;

	sfp_get_info = (struct sal_sfp_get_info_param *)argv_in;
	tmp_page = (u8 *) sfp_get_info->argv_out;
	page_sel = sfp_get_info->page_sel;
	shift_offset = sfp_get_info->shift_offset;
	len = sfp_get_info->len;
	ll_port_id = HIGGS_LOGIC_PORT_ID_TO_LL_PORT_ID(sfp_get_info->port_id);

	/*写光模块第127byte，选定upper page, 用于拼接特定的光模块输出信息 */
	if (SAL_OPTICAL_CABLE ==
	    sal_card->port_curr_wire_type[ll_port_id & 0x1f])
		if (OK != higgs_sfp_page_select(ll_card, ll_port_id, page_sel)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4428,
				    "Card:%d select sfp page failed on port:0x%x!",
				    ll_card->card_id, ll_port_id);
			return ERROR;
		}

	/* 读取选定的upper page */
	if (OK != higgs_read_sfp_eeprom(ll_card,
					ll_port_id,
					tmp_page, len, shift_offset)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4133,
			    "Card:%d read sfp page failed on port:0x%x",
			    ll_card->card_id, ll_port_id);
		return ERROR;
	}

	/* 重新选中upper page 0 */
	if (SAL_OPTICAL_CABLE ==
	    sal_card->port_curr_wire_type[ll_port_id & 0x1f])
		if (OK != higgs_sfp_page_select(ll_card, ll_port_id, 0)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4428,
				    "Card:%d select sfp page failed on port:0x%x!",
				    ll_card->card_id, ll_port_id);
			return ERROR;
		}

	return OK;

}

/*****************************************************************************
 函 数 名  : higgs_read_wire_eep_intf
 功能描述  : 读取SFP EEPROM的Page 0
 输入参数  : struct sal_card *sal_card,
			void *argv_in
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
s32 higgs_read_wire_eep_intf(struct sal_card * sal_card, void *argv_in)
{
	u8 *tmp_page = NULL;
	u32 ll_port_id = 0;
	struct sal_sfp_get_info_param *sfp_get_info = NULL;
	struct higgs_card *ll_card = NULL;

	/* 参数检查 */
	HIGGS_ASSERT(NULL != sal_card, return ERROR);
	HIGGS_ASSERT(NULL != argv_in, return ERROR);

	ll_card = (struct higgs_card *)sal_card->drv_data;
	sfp_get_info = (struct sal_sfp_get_info_param *)argv_in;
	tmp_page = (u8 *) (sfp_get_info->argv_out);
	ll_port_id = HIGGS_LOGIC_PORT_ID_TO_LL_PORT_ID(sfp_get_info->port_id);

	/*光缆和电缆都一次读取256 Bytes, 由上层决定读哪一个upper page */
	if (OK != higgs_read_sfp_eeprom(ll_card,
					ll_port_id,
					tmp_page,
					HIGGS_SFP_REGISTER_PAGE_LENGEH, 0)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4133,
			    "Card:%d read sfp eeprom failed on port:0x%x",
			    sal_card->card_no, ll_port_id);
		return ERROR;
	}

	return OK;
}

/*****************************************************************************
 函 数 名  : higgs_led_operation
 功能描述  : 端口LED点灯
 输入参数  :struct sal_card * sal_card,
 	 	  struct sal_led_op_param* v_pstLEDPara
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
s32 higgs_led_operation(struct sal_card * sal_card,
			struct sal_led_op_param * led_op)
{
	u32 phy_id = 0;
	u32 ll_port_id = 0;
	struct higgs_card *ll_card = NULL;

	/* 参数检查  */
	HIGGS_ASSERT(NULL != sal_card, return ERROR);
	HIGGS_ASSERT(NULL != led_op, return ERROR);

	ll_card = (struct higgs_card *)sal_card->drv_data;
	HIGGS_ASSERT(NULL != ll_card, return ERROR);

	/* 点灯 */
	if (SAL_ALL_PORT_LED_NEED_OPT == led_op->all_port_op) {
		/* 所有端口点灯 */
		for (ll_port_id = 0; ll_port_id < HIGGS_MAX_PORT_NUM;
		     ll_port_id++) {
			/* 端口是否有效 */
			if (!higgs_ll_port_exist(ll_card, ll_port_id))
				continue;

			/* 点灯 */
			if (OK !=
			    higgs_led_ctrl(ll_card, ll_port_id,
					   led_op->
					   all_port_speed_led[ll_port_id]))
				HIGGS_TRACE(OSP_DBL_MINOR, 4576,
					    "Card:%d LED ctrl %d failed on port %d",
					    ll_card->card_id,
					    led_op->
					    all_port_speed_led[ll_port_id],
					    ll_port_id);
		}
	} else if (SAL_LED_ALL_OFF == led_op->port_speed_led) {
		/* 所有端口熄灯  */
		for (ll_port_id = 0; ll_port_id < HIGGS_MAX_PORT_NUM;
		     ll_port_id++) {
			/* 端口是否有效 */
			if (!higgs_ll_port_exist(ll_card, ll_port_id))
				continue;

			/* 熄灯 */
			if (OK !=
			    higgs_led_ctrl(ll_card, ll_port_id, SAL_LED_OFF))
				HIGGS_TRACE(OSP_DBL_MINOR, 4576,
					    "Card:%d LED ctrl %d failed on port %d",
					    ll_card->card_id,
					    led_op->
					    all_port_speed_led[ll_port_id],
					    ll_port_id);
		}
	} else {
		/* 单个端口点灯 */
		phy_id = led_op->phy_id;

		/* 定位LL Port ID */
		if (OK !=
		    higgs_locate_ll_port_id_by_phy_id(ll_card, phy_id,
						      &ll_port_id)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4576,
				    "Card:%d locate port failed by phy:0x%x",
				    ll_card->card_id, phy_id);
			return ERROR;
		}

		/* 点灯 */
		if (OK !=
		    higgs_led_ctrl(ll_card, ll_port_id,
				   led_op->port_speed_led)) {
			HIGGS_TRACE(OSP_DBL_MINOR, 4576,
				    "Card:%d LED ctrl %d failed on port:0x%x",
				    ll_card->card_id, led_op->port_speed_led,
				    ll_port_id);
			return ERROR;
		}

	}

	return OK;
}

#if 0
/*****************************************************************************
 函 数 名  : Higgs_SimulateWireEvent
 功能描述  : 模拟板内端口的线缆插入事件
 输入参数  : struct higgs_card *ll_card
 		u32 ll_port_id
		u32  v_uiTimeoutMSecs
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
static void Higgs_SimulateWireEvent(struct higgs_card *ll_card, u32 ll_port_id,
				    u32 v_uiTimeoutMSecs)
{
	unsigned long ulFlag = 0;

	/* 内部函数，由外部调用者确保参数正确 */
	ll_card->astPortTimer[ll_port_id].ll_port_id = ll_port_id;
	ll_card->astPortTimer[ll_port_id].card_id = ll_card->card_id;

	/* 设置定时器, 通知SFP事件 */
	if (0 != v_uiTimeoutMSecs) {
		spin_lock_irqsave(&ll_card->card_lock, ulFlag);
		sal_add_timer((void *)&ll_card->astPortTimer[ll_port_id].
			      plug_in_timer,
			      (unsigned long)&ll_card->astPortTimer[ll_port_id],
			      (u32) msecs_to_jiffies(v_uiTimeoutMSecs),
			      Higgs_NotifySfpEvent);
		spin_unlock_irqrestore(&ll_card->card_lock, ulFlag);
	} else {
		Higgs_NotifySfpEvent((unsigned long)&ll_card->
				     astPortTimer[ll_port_id]);
	}
}

/*****************************************************************************
 函 数 名  : Higgs_SimulateSfpPlugInnerPort
 功能描述  : 模拟板内端口的线缆插入事件
 输入参数  : struct higgs_card *ll_card
		u32  v_uiTimeoutMSecs
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
void Higgs_SimulateSfpPlugInnerPort(struct higgs_card *ll_card,
				    u32 v_uiTimeoutMSecs)
{
	/* BEGIN:DTS2014122307748  反复加载卸载higgs.ko打印堆栈 chenqilin 00308265 2015.01.05 begin */
	u32 hw_ver = higgs_get_hw_version();
	u32 ll_port_id = C05_BOARD_INNER_PORT_LL_ID;

	/* 参数检查 */
	HIGGS_ASSERT(NULL != ll_card, return);

	/* 版本过滤 */
	if (HIGGS_HW_VERSION_C05 != hw_ver) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Operation unsupported on platform %d", hw_ver);
		return;
	}

	/* 控制器过滤 */
	if (P660_SAS_CORE_DSAF_ID != ll_card->card_id) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Card:%d has no board inner port, skipped",
			    ll_card->card_id);
		return;
	}

	Higgs_SimulateWireEvent(ll_card, ll_port_id, v_uiTimeoutMSecs);
	/* END:DTS2014122307748  反复加载卸载higgs.ko打印堆栈 chenqilin 00308265 2015.01.05 end */
}

/*****************************************************************************
 函 数 名  : Higgs_CheckSimulateSfpPlugMiniPort
 功能描述  : 检查MINI SAS HD1口是否线缆在位，如果在位，模拟一个线缆插入事件。用于驱动初始化流程.
 输入参数  : struct higgs_card *ll_card
		u32  v_uiTimeoutMSecs
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
void Higgs_CheckSimulateSfpPlugMiniPort(struct higgs_card *ll_card,
					u32 v_uiTimeoutMSecs)
{
	/* BEGIN:DTS2014122307748  反复加载卸载higgs.ko打印堆栈 chenqilin 00308265 2015.01.05 begin */
	u32 hw_ver = higgs_get_hw_version();
	u32 ll_port_id = C05_CPU_OUTER_PORT_LL_ID;
	bool is_sfp_on_port = false;

	/* 参数检查 */
	HIGGS_ASSERT(NULL != ll_card, return);

	/* 版本过滤 */
	if (HIGGS_HW_VERSION_C05 != hw_ver) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Operation unsupported on platform %d", hw_ver);
		return;
	}

	/* 控制器过滤 */
	if (P660_SAS_CORE_DSAF_ID != ll_card->card_id) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Card:%d has no mini sas port, skipped",
			    ll_card->card_id);
		return;
	}

	/* MINI SAS HD1口是否线缆在位  */
	if (OK != higgs_sfp_on_port(ll_card, ll_port_id, &is_sfp_on_port)) {
		return;
	}
	if (!is_sfp_on_port) {
		return;
	}

	HIGGS_TRACE(OSP_DBL_INFO, 171,
		    "Card:%d detect cable on mini sas port, to simulate plugin event",
		    ll_card->card_id);

	Higgs_SimulateWireEvent(ll_card, ll_port_id, v_uiTimeoutMSecs);
	/* END:DTS2014122307748  反复加载卸载higgs.ko打印堆栈 chenqilin 00308265 2015.01.05 end */
}
#endif

/*****************************************************************************
 函 数 名  : higgs_delay_trigger_all_port_sfp_event_for_init
 功能描述  : 用于驱动初始化流程，检查各个端口是否线缆在位，延迟触发线缆插拔事件
 输入参数  : struct higgs_card *ll_card
		u32  tm_out_ms
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
void higgs_delay_trigger_all_port_sfp_event_for_init(struct higgs_card *ll_card,
						     u32 tm_out_ms)
{
	u32 hw_ver = higgs_get_hw_version();
	u32 ll_port_id = 0;
	bool is_sfp_on_port = false;

	/* 参数检查 */
	HIGGS_ASSERT(NULL != ll_card, return);

	/* 版本过滤 */
	if (HIGGS_HW_VERSION_C05 != hw_ver) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Operation unsupported on platform %d", hw_ver);
		return;
	}

	/* 控制器过滤 */
	if (P660_SAS_CORE_DSAF_ID != ll_card->card_id) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Card:%d has no mini sas port, skipped",
			    ll_card->card_id);
		return;
	}

	/* 遍历端口，检查在位  */
	for (ll_port_id = 0; ll_port_id < HIGGS_MAX_PORT_NUM; ll_port_id++) {
		if (OK !=
		    higgs_sfp_on_port(ll_card, ll_port_id, &is_sfp_on_port))
			continue;

		if (is_sfp_on_port) {
			HIGGS_TRACE(OSP_DBL_INFO, 171,
				    "Card:%d cable detected on port %d in init",
				    ll_card->card_id, ll_port_id);
			higgs_simulate_wire_hotplug_intr(ll_card, ll_port_id,
							 tm_out_ms);
		}
	}
	return;
}

/*****************************************************************************
 函 数 名  : higgs_get_card_position
 功能描述  : 获取SAS Card在单板上的位置信息
 输入参数  :
		u32 card_id
 输出参数  :
 返 回 值  : s32
*****************************************************************************/

u8 higgs_get_card_position(u32 card_id)
{
	u8 card_position = 0;

	switch (card_id) {
	case P660_SAS_CORE_DSAF_ID:
		card_position = (0x1 << 5) | 0x0;
		break;
	case P660_SAS_CORE_PCIE_ID:
		card_position = (0x1 << 5) | 0x1;
		break;
	case P660_1_SAS_CORE_DSAF_ID:
		card_position = (0x1 << 5) | 0x2;
		break;
	case P660_1_SAS_CORE_PCIE_ID:
		card_position = (0x1 << 5) | 0x3;
		break;
	default:
		card_position = 0xFF;
		break;
	}

	return card_position;
}

/*----------------------------------------------*
 * 内部私有函数 定义                                  *
 *----------------------------------------------*/

/*****************************************************************************
 函 数 名  : higgs_get_hw_version
 功能描述  : 获取硬件版本:
 	 	 FPGA, EVB, C05
 输入参数  : HIGGSCARD_S *ll_card,
 		u32 ll_port_id
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
static u32 higgs_get_hw_version()
{
	u32 hw_ver = HIGGS_HW_VERSION_BUTT;

#if defined(FPGA_VERSION_TEST)
	hw_ver = HIGGS_HW_VERSION_FPGA;
#elif defined(EVB_VERSION_TEST)
	hw_ver = HIGGS_HW_VERSION_EVB;
#elif defined(C05_VERSION_TEST)
	hw_ver = HIGGS_HW_VERSION_C05;
#endif

	return hw_ver;
}

/*****************************************************************************
 函 数 名  : higgs_sfp_event_handler
 功能描述  : 回调函数，用于处理SFP插拔事件
 输入参数  : void * param
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
static void higgs_sfp_event_handler(void *param)
{
	struct higgs_card *ll_card = NULL;
	u32 ll_port_id = 0xff;
	u32 i = 0;
	u32 hw_ver = higgs_get_hw_version();

	/* 版本过滤 */
	if (HIGGS_HW_VERSION_C05 != hw_ver) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Operation unsupported on platform %d", hw_ver);
		return;
	}

	HIGGS_TRACE(OSP_DBL_INFO, 171, "Receive SFP hotplug event");

	HIGGS_REF(param);
	for (i = 0; i < HIGGS_MINI_SAS_PORT_COUNT; i++) {
		/* C05版本只管理CPU引出的MINI SAS口 */
		if (C05_CPU_OUTER_PORT_MINI_ID != i)
			continue;

		/* 匹配LL层Card */
		if (OK !=
		    higgs_locate_ll_card_and_ll_port_id_by_mini_port_id(i,
									&ll_card,
									&ll_port_id))
		{
			HIGGS_TRACE(OSP_DBL_MAJOR, 187,
				    "Locate LLCard and port id failed");
			continue;
		}

		/* 模拟PM8072的GPIO中断(线缆拔插)，对接兼容SAL的流程  */
		higgs_simulate_wire_hotplug_intr(ll_card, ll_port_id, 1);	/* 1ms延迟触发 */
	}
	return;
}

/*****************************************************************************
 函 数 名  : higgs_sfp_on_port
 功能描述  : 判断端口SFP是否在位
 输入参数  :
 	 struct higgs_card *ll_card,
 	 u32 ll_port_id,
 	 bool *is_on_port
 输出参数  : bool *is_on_port
 返 回 值  : s32
*****************************************************************************/
static s32 higgs_sfp_on_port(struct higgs_card *ll_card, u32 ll_port_id,
			     bool * is_on_port)
{
	u8 reg_val = 0;
	u32 mini_port_id = 0;
	u32 hw_ver = higgs_get_hw_version();

	/* 参数检查 */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(ll_port_id < HIGGS_MAX_PORT_NUM, return ERROR);
	HIGGS_ASSERT(NULL != is_on_port, return ERROR);

	/* 是否板内端口 */
	if (higgs_is_board_inner_port(ll_card, ll_port_id)) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Card:%d board inner port %d always be present",
			    ll_card->card_id, ll_port_id);
		*is_on_port = true;
		return OK;
	}

	/* 定位MINI SAS Port ID */
	if (OK !=
	    higgs_locate_mini_port_id_by_ll_port_id(ll_card, ll_port_id,
						    &mini_port_id)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4576,
			    "Card:%d locate minisas port failed by port:0x%x",
			    ll_card->card_id, ll_port_id);
		return ERROR;
	}

	/* 调试打桩 */
	if (higgs_stub_in_debug_mode(mini_port_id))
		return higgs_stub_sfp_on_port(mini_port_id, is_on_port);

	/* 版本过滤 */
	if (HIGGS_HW_VERSION_C05 != hw_ver) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Operation skipped on platform %d", hw_ver);
		return ERROR;
	}

	if (C05_CPU_OUTER_PORT_MINI_ID != mini_port_id) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 171, "Invalid minisas port id %d",
			    mini_port_id);
		return ERROR;
	}

	/* 调用BSP的接口，读取CPLD寄存器 */
       if(NULL == sal_peripheral_operation.read_logic)
       {
              return ERROR;
       }else if (OK != sal_peripheral_operation.read_logic(CPLD_SFP_PRSNT_REG, &reg_val)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 171, "Read cpld reg 0x%x failed",
			    CPLD_SFP_PRSNT_REG);
		return ERROR;
	}
	*is_on_port =
	    (CPLD_SFP_PRSNT_MINISAS1 ==
	     (reg_val & CPLD_SFP_PRSNT_MINISAS1_MASK));

	return OK;
}

/*****************************************************************************
 函 数 名  : higgs_sfp_page_select
 功能描述  : 端口SFP EEPROM翻页
 输入参数  :
 	 struct higgs_card *ll_card,
 	 u32 ll_port_id,
 	 u32 page_sel
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
static s32 higgs_sfp_page_select(struct higgs_card *ll_card, u32 ll_port_id,
				 u32 page_sel)
{
	u8 chan = 0;
	u8 addr = 0;
	u32 offset = 0;
	u8 buff[2] = { 0 };
	u32 hw_ver = higgs_get_hw_version();
	u32 mini_port_id = 0;

	/* 参数检查 */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(ll_port_id < HIGGS_MAX_PORT_NUM, return ERROR);
	HIGGS_ASSERT(page_sel < HIGGS_SFP_EEPROM_PAGE_COUNT, return ERROR);

	/* 是否板内端口 */
	if (higgs_is_board_inner_port(ll_card, ll_port_id)) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Card:%d sfp page select, skip board inner port %d",
			    ll_card->card_id, ll_port_id);
		return OK;
	}

	/* 定位MINI SAS Port ID */
	if (OK !=
	    higgs_locate_mini_port_id_by_ll_port_id(ll_card, ll_port_id,
						    &mini_port_id)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4576,
			    "Card:%d locate minisas port failed by port:%d",
			    ll_card->card_id, ll_port_id);
		return ERROR;
	}
	HIGGS_ASSERT(mini_port_id < HIGGS_MINI_SAS_PORT_COUNT, return ERROR);

	/* 调试打桩 */
	if (higgs_stub_in_debug_mode(mini_port_id))
		return higgs_stub_sfp_page_select(mini_port_id, page_sel);

	/* 版本过滤 */
	if (HIGGS_HW_VERSION_C05 != hw_ver) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Operation unsupported on platform %d", hw_ver);
		return ERROR;
	}

	/* 9545选通至MINI SAS1 */
	if (OK != higgs_open_i2c_9545_route(I2C_9545_ROUTE_TO_MINISAS1_PORT)) {
		HIGGS_TRACE(OSP_DBL_MINOR, 171,
			    "Card:%d open 9545 route to MINISAS1 SFP failed",
			    ll_card->card_id);
		return ERROR;
	}

	/* 写第127字节，选中PAGE */
	chan = I2C_9545_CHNL;
	addr = I2C_MINISAS1_PORT_ADDR;
	offset = SFP_EEPROM_PAGE_SEL_OFFSET;
	buff[0] = (u8) page_sel;
	if (OK !=
	    higgs_i2c_wirte(chan, addr, offset, HIGGS_I2C_OFFSET_ONE_BYTE, buff,
			    1)) {
		HIGGS_TRACE(OSP_DBL_MINOR, 171,
			    "I2C write channel %d addr 0x%x offset 0x%x value 0x%x failed",
			    chan, addr, offset, buff[0]);
		higgs_close_i2c_9545_route();
		return ERROR;
	}

	/* 9545取消选通 */
	higgs_close_i2c_9545_route();

	return OK;
}

/*****************************************************************************
 函 数 名  : higgs_read_sfp_eeprom
 功能描述  : 判断读取端口SFP的EEPROM信息
 输入参数  :
		struct higgs_card *ll_card,
		u32 ll_port_id,
		u8 *page,
		u32 rd_len,
		u32 offset
 输出参数  : u8 *page
 返 回 值  : s32
*****************************************************************************/
static s32 higgs_read_sfp_eeprom(struct higgs_card *ll_card,
				 u32 ll_port_id,
				 u8 * page, u32 rd_len, u32 offset)
{
	u8 chan = 0;
	u8 addr = 0;
//      u32 tmp_offset = 0;
//      u8 wr_buff[2] = {0};
	u8 buff[SAL_SFP_WHOLE_REGISTER_BYTES] = { 0 };
	u32 hw_ver = higgs_get_hw_version();
	u32 mini_port_id = 0;

	/* 参数检查  */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(ll_port_id < HIGGS_MAX_PORT_NUM, return ERROR);
	HIGGS_ASSERT(NULL != page, return ERROR);
	HIGGS_ASSERT(rd_len <= SAL_SFP_WHOLE_REGISTER_BYTES, return ERROR);
	HIGGS_ASSERT(offset < SAL_SFP_WHOLE_REGISTER_BYTES, return ERROR);
	HIGGS_ASSERT((offset + rd_len) <= SAL_SFP_WHOLE_REGISTER_BYTES,
		     return ERROR);

	/* 是否板内端口 */
	if (higgs_is_board_inner_port(ll_card, ll_port_id)) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Card:%d read eeprom, simulation for board inner port %d",
			    ll_card->card_id, ll_port_id);
		return higgs_simulate_read_sfp_eeprom_inner_port(ll_card,
								 ll_port_id,
								 page, rd_len,
								 offset);
	}

	/* 定位MINI SAS Port ID */
	if (OK !=
	    higgs_locate_mini_port_id_by_ll_port_id(ll_card, ll_port_id,
						    &mini_port_id)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4576,
			    "Card:%d locate minisas port failed by port:0x%x",
			    ll_card->card_id, ll_port_id);
		return ERROR;
	}

	/* 调试打桩 */
	if (higgs_stub_in_debug_mode(mini_port_id))
		return higgs_stub_read_sfp_eeprom(mini_port_id, page, rd_len,
						  offset);

	/* 版本过滤 */
	if (HIGGS_HW_VERSION_C05 != hw_ver) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Operation unsupported on platform %d", hw_ver);
		return ERROR;
	}

	/* 9545选通至MINI SAS1 */
	if (OK != higgs_open_i2c_9545_route(I2C_9545_ROUTE_TO_MINISAS1_PORT)) {
		HIGGS_TRACE(OSP_DBL_MINOR, 171,
			    "Card:%d open 9545 route to MINISAS1 SFP failed",
			    ll_card->card_id);
		return ERROR;
	}

	/* 从Offset处读取数据 */
	chan = I2C_9545_CHNL;
	addr = I2C_MINISAS1_PORT_ADDR;
	//tmp_offset = offset;
	memset(buff, 0, sizeof(buff));
	if (OK !=
	    higgs_i2c_read(chan, addr, offset, HIGGS_I2C_OFFSET_ONE_BYTE, buff,
			   rd_len)) {
		HIGGS_TRACE(OSP_DBL_MINOR, 171,
			    "I2C read channel %d addr 0x%x offset 0x%x failed",
			    chan, addr, offset);
		higgs_close_i2c_9545_route();
		return ERROR;
	}

	/* 拷贝数据 */
	memcpy(page, buff, rd_len);

	/* 取消9545选通 */
	higgs_close_i2c_9545_route();

	return OK;
}

/*****************************************************************************
 函 数 名  : higgs_led_ctrl
 功能描述  : 端口LED点灯
 输入参数  :
 	 	 struct higgs_card *ll_card,
 	 	 u32 ll_port_id,
 	 	 enum sal_led_op_type led_op
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
static s32 higgs_led_ctrl(struct higgs_card *ll_card, u32 ll_port_id,
			  enum sal_led_op_type led_op)
{
	u8 reg_val = 0;
	u32 mini_port_id = 0;
	u32 hw_ver = HIGGS_HW_VERSION_BUTT;

	/* 参数检查 */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(ll_port_id < HIGGS_MAX_PORT_NUM, return ERROR);

	HIGGS_TRACE(OSP_DBL_INFO, 4576,
		    "Card:%d To LED ctrl, port:%d opcode: 0x%x",
		    ll_card->card_id, ll_port_id, led_op);
	hw_ver = higgs_get_hw_version();

	/* 是否板内端口 */
	if (higgs_is_board_inner_port(ll_card, ll_port_id)) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Card:%d LED ctrl, skip board inner port %d",
			    ll_card->card_id, ll_port_id);
		return OK;
	}

	/* 定位MINI SAS Port ID */
	if (OK !=
	    higgs_locate_mini_port_id_by_ll_port_id(ll_card, ll_port_id,
						    &mini_port_id)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4576,
			    "Card:%d locate minisas port failed by port:%d",
			    ll_card->card_id, ll_port_id);
		return ERROR;
	}
	HIGGS_ASSERT(mini_port_id < HIGGS_MINI_SAS_PORT_COUNT, return ERROR);

	/* 调试打桩 */
	if (higgs_stub_in_debug_mode(mini_port_id))
		return higgs_stub_led_ctrl(mini_port_id, led_op);

	/* 版本过滤  */
	if (HIGGS_HW_VERSION_C05 != hw_ver) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 171,
			    "Operation unsupported platform %d", hw_ver);
		return ERROR;
	}

	/* 点灯控制  */
	if (C05_CPU_OUTER_PORT_MINI_ID != mini_port_id) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 171,
			    "Card: %d invalid minisas port %d ",
			    ll_card->card_id, mini_port_id);
		return ERROR;
	}

	switch (led_op) {
	case SAL_LED_HIGH_SPEED:
		reg_val = CPLD_LED_12G_LINK;
		break;
	case SAL_LED_LOW_SPEED:
		reg_val = CPLD_LED_6G_LINK;
		break;
	case SAL_LED_FAULT:
		reg_val = CPLD_LED_FAULT_LINK;
		break;
	case SAL_LED_OFF:
		reg_val = CPLD_LED_NO_LINK;
		break;
	case SAL_LED_ALL_OFF:
	case SAL_LED_RESTORE:
	case SAL_LED_BUTT:
	default:
		HIGGS_TRACE(OSP_DBL_MAJOR, 171, "LED ctrl, invalid  opcode %d",
			    (u32) led_op);
		return ERROR;
	}

	/* 调用BSP的接口，读写CPLD寄存器 */
        if(NULL != sal_peripheral_operation.write_logic)
       {
	     (void)sal_peripheral_operation.write_logic(CPLD_LED_MINISAS1_REG, reg_val);
       }
	return OK;
}

/*****************************************************************************
 函 数 名  : higgs_enable_sas_redriver_channel
 功能描述  : 使能MINI SAS级联口上游的SAS驱动器的通道
 输入参数  : struct higgs_card *ll_card
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
static s32 higgs_enable_sas_redriver_channel(struct higgs_card *ll_card)
{
	u8 chan = 0;
	u8 addr = 0;
	u32 offset = 0;
	u8 redriver_chan_ctrl = 0;
	u8 wr_buff[2] = { 0 };
	u8 rd_buff[2] = { 0 };
	s32 ret = ERROR;

	/* 参数检查 */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);

	/* C05版本 */
	if (P660_SAS_CORE_DSAF_ID != ll_card->card_id) {
		HIGGS_TRACE(OSP_DBL_MINOR, 4576,
			    "C05 version do not manage controller %d",
			    ll_card->card_id);
		return ERROR;
	}

	/* 9545选通至SAS Redriver */
	if (OK !=
	    higgs_open_i2c_9545_route(I2C_9545_ROUTE_TO_MINISAS1_REDRIVER)) {
		HIGGS_TRACE(OSP_DBL_MINOR, 171,
			    "Card:%d open 9545 route to MINISAS1 redriver failed",
			    ll_card->card_id);
		return ERROR;
	}

	do {
		/* Redriver通道1-4使能 */
		chan = I2C_9545_CHNL;
		addr = I2C_MINISAS1_REDRIVER_LOWER_ADDR;
		offset = REDRIVER_CHNL_1_4_CTRL_OFFSET;

		rd_buff[0] = 0;
		if (OK !=
		    higgs_i2c_read(chan, addr, offset,
				   HIGGS_I2C_OFFSET_ONE_BYTE, rd_buff, 1)) {
			HIGGS_TRACE(OSP_DBL_MINOR, 171,
				    "I2C read channel %d addr 0x%x offset 0x%x failed",
				    chan, addr, offset);
			break;
		}
		redriver_chan_ctrl = rd_buff[0];

		wr_buff[0] =
		    (u8) ((redriver_chan_ctrl & (~REDRIVER_CHNL_1_4_CTRL_MASK))
			  | REDRIVER_CHNL_1_4_CTRL_ENABLE);
		if (OK !=
		    higgs_i2c_wirte(chan, addr, offset,
				    HIGGS_I2C_OFFSET_ONE_BYTE, wr_buff, 1)) {
			HIGGS_TRACE(OSP_DBL_MINOR, 171,
				    "I2C write channel %d addr 0x%x offset 0x%x value 0x%x failed",
				    chan, addr, offset, wr_buff[0]);
			break;
		}
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Enable MINISAS1 redriver channel 1-4 succeeded");

		/* Redriver通道5-8使能 */
		chan = I2C_9545_CHNL;
		addr = I2C_MINISAS1_REDRIVER_UPPER_ADDR;
		offset = REDRIVER_CHNL_5_8_CTRL_OFFSET;

		rd_buff[0] = 0;
		if (OK !=
		    higgs_i2c_read(chan, addr, offset,
				   HIGGS_I2C_OFFSET_ONE_BYTE, rd_buff, 1)) {
			HIGGS_TRACE(OSP_DBL_MINOR, 171,
				    "I2C read channel %d addr 0x%x offset 0x%x failed",
				    chan, addr, offset);
			break;
		}
		redriver_chan_ctrl = rd_buff[0];

		wr_buff[0] =
		    (u8) ((redriver_chan_ctrl & (~REDRIVER_CHNL_5_8_CTRL_MASK))
			  | REDRIVER_CHNL_5_8_CTRL_ENABLE);
		if (OK !=
		    higgs_i2c_wirte(chan, addr, offset,
				    HIGGS_I2C_OFFSET_ONE_BYTE, wr_buff, 1)) {
			HIGGS_TRACE(OSP_DBL_MINOR, 171,
				    "I2C write channel %d addr 0x%x offset 0x%x value 0x%x failed",
				    chan, addr, offset, wr_buff[0]);
			break;
		}

		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Enable MINISAS1 redriver channel 5-8 succeeded");
		ret = OK;
	} while (0);

	/* 取消9545选通 */
	higgs_close_i2c_9545_route();

	return ret;
}

/*****************************************************************************
 函 数 名  : higgs_config_sas_redriver_channel_param
 功能描述  : 配置MINI SAS级联口上游的SAS驱动器的通道的信号参数
 输入参数  : struct higgs_card *ll_card
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
static s32 higgs_config_sas_redriver_channel_param(struct higgs_card *ll_card)
{
	u8 chan = 0;
	u8 addr = 0;
	u32 offset = 0;
	u32 redriver_chan = 0;
	u8 wr_buff[2] = { 0 };

	/* 参数检查 */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);

	/* C05版本 */
	if (P660_SAS_CORE_DSAF_ID != ll_card->card_id) {
		HIGGS_TRACE(OSP_DBL_MINOR, 4576,
			    "C05 version do not manage controller %d",
			    ll_card->card_id);
		return ERROR;
	}

	/* 9545选通至SAS Redriver */
	if (OK !=
	    higgs_open_i2c_9545_route(I2C_9545_ROUTE_TO_MINISAS1_REDRIVER)) {
		HIGGS_TRACE(OSP_DBL_MINOR, 171,
			    "Card:%d open 9545 route to MINISAS1 redriver failed",
			    ll_card->card_id);
		return ERROR;
	}

	/* Redriver通道1-8参数配置 */
	chan = I2C_9545_CHNL;
	for (redriver_chan = 0; redriver_chan < REDRIVER_CHNL_NUM_MAX;
	     redriver_chan++) {
		addr = (u8) i2c_mini_sas1_redriver_chan_addr[redriver_chan];
		offset = redriver_chan_offset[redriver_chan];
		wr_buff[0] = (u8) C05_MINISAS1_REDRIVER_CHNL_EQ_PARAM;
		if (OK !=
		    higgs_i2c_wirte(chan, addr, offset,
				    HIGGS_I2C_OFFSET_ONE_BYTE, wr_buff, 1)) {
			HIGGS_TRACE(OSP_DBL_MINOR, 171,
				    "I2C write channel %d addr 0x%x offset 0x%x value 0x%x failed",
				    chan, addr, offset, wr_buff[0]);
			higgs_close_i2c_9545_route();
			return ERROR;
		}
	}
	HIGGS_TRACE(OSP_DBL_INFO, 171,
		    "Config MINISAS1 redriver channel 1-8 param 0x%x succeeded",
		    wr_buff[0]);

	/* 9545取消选通 */
	higgs_close_i2c_9545_route();

	return OK;
}

/*****************************************************************************
 函 数 名  : Higgs_InitSasRedriverChannelAddr
 功能描述  : 初始化MINI SAS级联口上游的SAS驱动器I2C地址
 输入参数  : struct higgs_card *ll_card
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
static s32 higgs_init_sas_redriver_addr(struct higgs_card *ll_card)
{
	u8 chan = 0;
	u8 addr = 0;
	u32 offset = 0;
	u8 wr_buff[2] = { 0 };

	/* 参数检查 */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);

	/* C05版本 */
	if (P660_SAS_CORE_DSAF_ID != ll_card->card_id) {
		HIGGS_TRACE(OSP_DBL_MINOR, 4576,
			    "C05 version do not manage controller %d",
			    ll_card->card_id);
		return ERROR;
	}

	/* 9545选通至SAS Redriver */
	if (OK !=
	    higgs_open_i2c_9545_route(I2C_9545_ROUTE_TO_MINISAS1_REDRIVER)) {
		HIGGS_TRACE(OSP_DBL_MINOR, 171,
			    "Card:%d open 9545 route to MINISAS1 redriver failed",
			    ll_card->card_id);
		return ERROR;
	}

	/* Redriver地址初始化 */
	chan = I2C_9545_CHNL;
	addr = I2C_MINISAS1_REDRIVER_LOWER_ADDR;
	offset = 0x3C;
	wr_buff[0] = I2C_MINISAS1_REDRIVER_LOWER_ADDR;
	if (OK !=
	    higgs_i2c_wirte(chan, addr, offset, HIGGS_I2C_OFFSET_ONE_BYTE,
			    wr_buff, 1)) {
		HIGGS_TRACE(OSP_DBL_MINOR, 171,
			    "I2C write channel %d addr 0x%x offset 0x%x value 0x%x failed",
			    chan, addr, offset, wr_buff[0]);
		higgs_close_i2c_9545_route();
		return ERROR;
	}

	/* 9545取消选通 */
	higgs_close_i2c_9545_route();

	return OK;
}

/*****************************************************************************
 函 数 名  : higgs_disable_sas_redriver_channel
 功能描述  : 禁用MINI SAS级联口的上游SAS驱动器的通道
 输入参数  : struct higgs_card *ll_card
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
static s32 higgs_disable_sas_redriver_channel(struct higgs_card *ll_card)
{
	u8 chan = 0;
	u8 addr = 0;
	u32 offset = 0;
	u8 redriver_chan_ctrl = 0;
	u8 rd_buff[2] = { 0 };
	u8 wr_buff[2] = { 0 };
	s32 ret = ERROR;

	/* 参数检查 */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);

	/* C05版本 */
	if (P660_SAS_CORE_DSAF_ID != ll_card->card_id) {
		HIGGS_TRACE(OSP_DBL_MINOR, 4576,
			    "C05 version do not manage controller %d",
			    ll_card->card_id);
		return ERROR;
	}

	/* 9545选通至SAS Redriver */
	if (OK !=
	    higgs_open_i2c_9545_route(I2C_9545_ROUTE_TO_MINISAS1_REDRIVER)) {
		HIGGS_TRACE(OSP_DBL_MINOR, 171,
			    "Card:%d open 9545 route to MINISAS1 redriver failed",
			    ll_card->card_id);
		return ERROR;
	}

	do {
		/* Redriver通道1-4禁用 */
		chan = I2C_9545_CHNL;
		addr = I2C_MINISAS1_REDRIVER_LOWER_ADDR;
		offset = REDRIVER_CHNL_1_4_CTRL_OFFSET;

		rd_buff[0] = 0;
		if (OK !=
		    higgs_i2c_read(chan, addr, offset,
				   HIGGS_I2C_OFFSET_ONE_BYTE, rd_buff, 1)) {
			HIGGS_TRACE(OSP_DBL_MINOR, 171,
				    "I2C read channel %d addr 0x%x offset 0x%x failed",
				    chan, addr, offset);
			break;
		}
		redriver_chan_ctrl = rd_buff[0];

		wr_buff[0] =
		    (u8) ((redriver_chan_ctrl & (~REDRIVER_CHNL_1_4_CTRL_MASK))
			  | REDRIVER_CHNL_1_4_CTRL_DISABLE);
		if (OK !=
		    higgs_i2c_wirte(chan, addr, offset,
				    HIGGS_I2C_OFFSET_ONE_BYTE, wr_buff, 1)) {
			HIGGS_TRACE(OSP_DBL_MINOR, 171,
				    "I2C write channel %d addr 0x%x offset 0x%x value 0x%x failed",
				    chan, addr, offset, wr_buff[0]);
			break;
		}
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Disable MINISAS1 redriver channel 1-4 succeeded");

		/* Redriver通道5-8禁用 */
		chan = I2C_9545_CHNL;
		addr = I2C_MINISAS1_REDRIVER_UPPER_ADDR;
		offset = REDRIVER_CHNL_5_8_CTRL_OFFSET;

		rd_buff[0] = 0;
		if (OK !=
		    higgs_i2c_read(chan, addr, offset,
				   HIGGS_I2C_OFFSET_ONE_BYTE, rd_buff, 1)) {
			HIGGS_TRACE(OSP_DBL_MINOR, 171,
				    "I2C read channel %d addr 0x%x offset 0x%x failed",
				    chan, addr, offset);
			break;
		}
		redriver_chan_ctrl = rd_buff[0];

		wr_buff[0] =
		    (u8) ((redriver_chan_ctrl & (~REDRIVER_CHNL_5_8_CTRL_MASK))
			  | REDRIVER_CHNL_5_8_CTRL_DISABLE);
		if (OK !=
		    higgs_i2c_wirte(chan, addr, offset,
				    HIGGS_I2C_OFFSET_ONE_BYTE, wr_buff, 1)) {
			HIGGS_TRACE(OSP_DBL_MINOR, 171,
				    "I2C write channel %d addr 0x%x offset 0x%x value 0x%x failed",
				    chan, addr, offset, wr_buff[0]);
			break;
		}

		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Disable MINISAS1 redriver channel 5-8 succeeded");
		ret = OK;
	} while (0);

	/* 9545取消选通 */
	higgs_close_i2c_9545_route();

	return ret;
}

/*****************************************************************************
 函 数 名  : higgs_open_i2c_9545_route
 功能描述  : 打开9545路由选通
 输入参数  : 无
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
static s32 higgs_open_i2c_9545_route(u32 route)
{
	u8 chan = 0;
	u8 addr = 0;
	u32 offset = 0;
	u8 wr_buff[2] = { 0 };

	/* 9545选通路由 */
	chan = I2C_9545_CHNL;
	addr = I2C_9545_ADDR;
	offset = 0;
	wr_buff[0] = (u8) route;
	if (OK !=
	    higgs_i2c_wirte(chan, addr, offset, HIGGS_I2C_OFFSET_NONE, wr_buff,
			    1)) {
		HIGGS_TRACE(OSP_DBL_MINOR, 171,
			    "I2C write channel %d addr 0x%x value 0x%x failed",
			    chan, addr, wr_buff[0]);
		return ERROR;
	}
	HIGGS_MDELAY(2);	/* 9545 switch need some delay */
	return OK;
}

/*****************************************************************************
 函 数 名  : higgs_close_i2c_9545_route
 功能描述  : 关闭9545路由选通
 输入参数  : 无
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
static void higgs_close_i2c_9545_route(void)
{
	u8 chan = 0;
	u8 addr = 0;
	u32 offset = 0;
	u8 wr_buff[2] = { 0 };

	/* 9545取消选通 */
	chan = I2C_9545_CHNL;
	addr = I2C_9545_ADDR;
	offset = 0;
	wr_buff[0] = I2C_9545_ROUTE_TO_NONE;
	if (OK !=
	    higgs_i2c_wirte(chan, addr, offset, HIGGS_I2C_OFFSET_NONE, wr_buff,
			    1))
		HIGGS_TRACE(OSP_DBL_MINOR, 171,
			    "I2C write channel %d addr 0x%x value 0x%x failed",
			    chan, addr, wr_buff[0]);

	HIGGS_MDELAY(2);	/* 9545 switch need some delay */
	return;
}

/*****************************************************************************
 函 数 名  : higgs_i2c_wirte
 功能描述  : I2C写操作
 输入参数  :
		u8 chan,
		u8 addr,
		u32 offset,
		enum higgs_i2c_offset_size offset_size,
		u8*  buff,
		u32 buff_size
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
static s32 higgs_i2c_wirte(u8 chan,
			   u8 addr,
			   u32 offset,
			   enum higgs_i2c_offset_size offset_size,
			   u8 * buff, u32 buff_size)
{
	u8 tmp_addr = (addr >> 1);	/* 660 i2c驱动接口使用无偏移地址 */
	u32 hw_ver = higgs_get_hw_version();

	/* 版本过滤 */
	if (HIGGS_HW_VERSION_C05 != hw_ver) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Operation unsupported on platform %d", hw_ver);
		return ERROR;
	}
//      return i2c_api_do_send(chan, (char)tmp_addr, offset, (char*)buff, buff_size, (s32)offset_size);
       if(NULL == sal_peripheral_operation.i2c_simple_write)
       {
              return ERROR;
       }else
       return sal_peripheral_operation.i2c_simple_write(chan, (u8) tmp_addr, offset, (char *)buff,
				    buff_size, (s32) offset_size);
}

/*****************************************************************************
 函 数 名  : higgs_i2c_read
 功能描述  : I2C读操作
 输入参数  :
		u8 chan,
		u8 addr,
		u32 offset,
		enum higgs_i2c_offset_size offset_size,
		u8*  buff,
		u32 buff_size
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
static s32 higgs_i2c_read(u8 chan,
			  u8 addr,
			  u32 offset,
			  enum higgs_i2c_offset_size offset_size,
			  u8 * buff, u32 buff_size)
{
	u8 tmp_addr = (addr >> 1);	/* 660 i2c驱动接口使用无偏移地址 */
	u32 hw_ver = higgs_get_hw_version();

	/* 版本过滤 */
	if (HIGGS_HW_VERSION_C05 != hw_ver) {
		HIGGS_TRACE(OSP_DBL_INFO, 171,
			    "Operation unsupported on platform %d", hw_ver);
		return ERROR;
	}
//      return i2c_api_do_recv(chan, (char)tmp_addr, offset, (char*)buff, buff_size, (s32)offset_size);
       if(NULL == sal_peripheral_operation.i2c_simple_read)
       {
              return ERROR;
       }else
         return sal_peripheral_operation.i2c_simple_read(chan, (u8) tmp_addr, offset, (char *)buff,
                  buff_size, (s32) offset_size);
}

/*****************************************************************************
 函 数 名  : higgs_is_board_inner_port
 功能描述  : 判断是否CPU与板载8054的连接端口
 输入参数  : HIGGSCARD_S *ll_card,
 		u32 ll_port_id
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
static bool higgs_is_board_inner_port(struct higgs_card *ll_card,
				      u32 ll_port_id)
{
	u32 hw_ver = higgs_get_hw_version();
	bool is_inner_port = false;

	/* 参数检查 */
	HIGGS_ASSERT(NULL != ll_card, return false);
	HIGGS_ASSERT(ll_port_id < HIGGS_MAX_PORT_NUM, return false);

	/* 判断 */
	switch (hw_ver) {
	case HIGGS_HW_VERSION_C05:
		is_inner_port = ((P660_SAS_CORE_DSAF_ID == ll_card->card_id)
				 && (C05_BOARD_INNER_PORT_LL_ID == ll_port_id));
		break;
	default:
		break;
	}

	return is_inner_port;
}

#if 0
/*****************************************************************************
 函 数 名  : Higgs_NotifySfpEvent
 功能描述  : 触发端口线缆插拔事件
 输入参数  : unsigned long v_ulData
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
void Higgs_NotifySfpEvent(unsigned long v_ulData)
{
	/* BEGIN:DTS2014122307748  反复加载卸载higgs.ko打印堆栈 chenqilin 00308265 2015.01.05 begin */
	u32 ll_port_id = 0;
	u32 logic_port_id = 0;
	u32 uiReadGpioRspVal = 0;
	struct higgs_card *ll_card = NULL;
	struct sal_card *sal_card = NULL;
	struct higgs_timer_callback_param stPortTimer;
	u32 card_id = 0;

	/* 参数检查 */
	HIGGS_ASSERT(0 != v_ulData, return);
	stPortTimer = *(struct higgs_timer_callback_param *)v_ulData;
	card_id = stPortTimer.card_id;
	ll_port_id = stPortTimer.ll_port_id;

	sal_card = sal_get_card(card_id);
	HIGGS_ASSERT(NULL != sal_card, return);

	ll_card = (struct higgs_card *)sal_card->drv_data;
	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(NULL != ll_card->sal_card, return);

	/* 手动触发 */
	if (P660_SAS_CORE_DSAF_ID == ll_card->card_id) {	/* C05版本 */
		/* 调用SAL_AddEventToWireHandler通知SAL层 */
		/* DTS2014122503568: 在PV660/Hi1610平台, GPIO值虽然无意义，但是SAL层要求不同端口的值有差异，否则邻近的事件可能会被过滤 */
#if 0
		uiReadGpioRspVal = 0xFFFFFFFF;	/* 在PV660/Hi1610平台, GPIO值废弃 */
#else
		uiReadGpioRspVal = 0xFFFFFF00 + ll_port_id;
#endif
		/* DTS2014122503568 */
		logic_port_id =
		    ll_card->sal_card->config.port_logic_id[ll_port_id];
		if (OK !=
		    SAL_AddEventToWireHandler(ll_card->sal_card,
					      uiReadGpioRspVal,
					      logic_port_id)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4576,
				    "Card:%d port:0x%x add sfp hotplug event failed",
				    ll_card->card_id, logic_port_id);
		} else {
			HIGGS_TRACE(OSP_DBL_INFO, 4576,
				    "Card:%d port:0x%x add sfp hotplug event succeeded",
				    ll_card->card_id, logic_port_id);
		}
	} else {
		HIGGS_TRACE(OSP_DBL_INFO, 4576,
			    "Card:%d ignore sfp event simulation",
			    ll_card->card_id);
	}

	sal_put_card(sal_card);
	/* END:DTS2014122307748  反复加载卸载higgs.ko打印堆栈 chenqilin 00308265 2015.01.05 end */

	return;
}
#endif

/*****************************************************************************
 函 数 名  : higgs_read_sfp_eeprom
 功能描述  : 给板内SAS端口模拟SFP EEPROM信息
 输入参数  :
		struct higgs_card *ll_card,
		u32 ll_port_id,
		u8 *page,
		u32 rd_len,
		u32 offset
 输出参数  : u8 *page
 返 回 值  : s32
*****************************************************************************/
static s32 higgs_simulate_read_sfp_eeprom_inner_port(struct higgs_card *ll_card,
						     u32 ll_port_id,
						     u8 * page,
						     u32 rd_len, u32 offset)
{
	/* 参数检查 */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(ll_port_id < HIGGS_MAX_PORT_NUM, return ERROR);

	/* 是否板内端口 */
	if (!higgs_is_board_inner_port(ll_card, ll_port_id))
		return ERROR;

	return higgs_stub_read_copper_cable_eeprom(page, rd_len, offset);
}

/*****************************************************************************
 函 数 名  : higgs_ll_port_exist
 功能描述  : 判断LL PORT是否存在
 输入参数  : struct higgs_card *ll_card,
		u32 ll_port_id,
 输出参数  : 无
 返 回 值  : bool
*****************************************************************************/
static bool higgs_ll_port_exist(struct higgs_card *ll_card, u32 ll_port_id)
{
	u32 port_bit_map = 0;

	/* 参数检查 */
	HIGGS_ASSERT(NULL != ll_card, return false);
	HIGGS_ASSERT(ll_port_id < HIGGS_MAX_PORT_NUM, return false);

	port_bit_map = ll_card->card_cfg.port_bitmap[ll_port_id];

	return (0 != port_bit_map);
}

struct higgs_card *higgs_get_card_info(u32 card_id)
{
	return higgs_peri_cards[card_id];
}

/*
 * 测试用函数
 */
 /*
    s32 Higg_I2cTest(s32 v_uiCmd)
    {
    struct higgs_card *ll_card;
    s32 ret = ERROR;

    ll_card = HIGGS_VMALLOC(sizeof(struct higgs_card));
    if (NULL == ll_card)
    return ERROR;
    ll_card->card_id = P660_SAS_CORE_DSAF_ID;
    switch(v_uiCmd)
    {
    case 0:
    ret = higgs_init_sas_redriver_addr(ll_card);
    break;
    case 1:
    ret = higgs_enable_sas_redriver_channel(ll_card);
    break;
    case 2:
    ret = higgs_disable_sas_redriver_channel(ll_card);
    break;
    case 3:
    ret = higgs_config_sas_redriver_channel_param(ll_card);
    break;
    default:
    ret = ERROR;
    break;
    }

    HIGGS_VFREE(ll_card);
    return ret;
    }

    EXPORT_SYMBOL(Higg_I2cTest);
  */
