#ifndef HIGGS_SERDES_API_H
#define HIGGS_SERDES_API_H

#include "higgs_common.h"

/*************************************************************
       ÐÂÔö¼ÓµÄºê¶¨Òåbegin
***********************************************************/
#define HILINK_REG_SIZE    (0x40000)

/*
static inline void serdes_delay_us (unsigned int time) 
{
	if(time >= 1000)
	{
		mdelay(time/1000);
		time = (time%1000);
	}
	if(time != 0)
	{
		udelay(time);
	}
}
*/

extern u32 max_cpu_nodes;

/*l00290354 add serdes struct
×¢Òâµ±hilink0ÎªEM_HILINK0_PCIE1_4LANE_PCIE2_4LANEÄ£Ê½
hilink2±ØÐëÎªEM_HILINK2_SAS0_8LANE
*/
enum hilink0_mode_type {
	EM_HILINK0_PCIE1_8LANE = 0,
	EM_HILINK0_PCIE1_4LANE_PCIE2_4LANE = 1,
};

enum hilink1_mode_type {
	EM_HILINK1_PCIE0_8LANE = 0,
	EM_HILINK1_HCCS_8LANE = 1,
};

enum hilink2_mode_type {
	EM_HILINK2_PCIE2_8LANE = 0,
	EM_HILINK2_SAS0_8LANE = 1,
};

enum hilink3_mode_type {
	EM_HILINK3_GE_4LANE = 0,
	EM_HILINK3_GE_2LANE_XGE_2LANE = 1,	//lane0,lane1-ge,lane2,lane3 xge
};

enum hilink4_mode_type {
	EM_HILINK4_GE_4LANE = 0,
	EM_HILINK4_XGE_4LANE = 1,
};

enum hilink5_mode_type {
	EM_HILINK5_SAS1_4LANE = 0,
	EM_HILINK5_PCIE3_4LANE = 1,
};

enum board_type {
	EM_32CORE_EVB_BOARD = 0,
	EM_16CORE_EVB_BOARD = 1,
	EM_V2R1CO5_BORAD = 2,
	EM_OTHER_BORAD		//Ã»ÓÐ¼«ÐÔ·´×ª.2P
};

struct hilink_serdes_param {
	enum hilink0_mode_type hilink0_mode;
	enum hilink1_mode_type hilink1_mode;
	enum hilink2_mode_type hilink2_mode;
	enum hilink3_mode_type hilink3_mode;
	enum hilink4_mode_type hilink4_mode;
	enum hilink5_mode_type hilink5_mode;
	//enum board_type board_type;
};

enum serdes_ret {
	EM_SERDES_SUCCESS = 0,
	EM_SERDES_FAIL = 1,
};

#define CS_CALIB_TIME 10
#define DS_CALIB_TIME 20

/*FirmwareÏà¹Ø¼Ä´æÆ÷*/
#define MCU_CFG_OFFSET_ADDR                         ((0xFFF0)*2)
#define FIRMWARE_DOWNLOAD_OFFSET_ADDR               ((0xC000)*2)
#define CUSTOMER_SPACE_START_OFFSET_ADDR            ((0xFF6C)*2)
#define CUSTOMER_SPACE_END_OFFSET_ADDR              ((0xFFEE)*2)
#define CS_DS_NUM_SEARCH_OFFSET_ADDR                ((0xFFEE)*2)
#define CS_CALIBRATION_ENABLE_OFFSET_ADDR           ((0xFFED)*2)
#define DS_CTLE_CONTROL1_OFFSET_ADDR(i)             ((0xFFE4-(i)*8)*2)
#define DS_CTLE_CONTROL0_OFFSET_ADDR(i)             ((0xFFE6-(i)*8)*2)
#define SRB_SYS_CTRL_BASE (0xf3e00000)
/*CS Ð£×¼×î´ó´ÎÊý*/
#define CS_CALIB_MAX_TIME                        (10)

/* ·µ»ØÖµºê¶¨Òå*/
#define SRE_Hilink_INIT_ERROR                  1
#define SRE_Hilink_READ_REG_ERROR         1
#define SRE_Hilink_WRITE_REG_ERROR       1
#define SRE_Hilink_PARAMETER_ERROR       1
#define SRE_Hilink_CALL_FUN_ERROR          1
#define SRE_OK                                          0
#define SRE_HILINK_CS_CALIB_ERR                   1
#define SRE_HILINK_DS_CALIB_ERR   1

/*Macro number*/
#define MACRO_0   0
#define MACRO_1   1
#define MACRO_2   2
#define MACRO_3   3
#define MACRO_4   4
#define MACRO_5   5
#define MACRO_6   6

#define SRE_OK          0
#define SRE_ERROR       0xffffffff

/* CS  number*/
#define CS0  0
#define CS1  1
#define INVAID_CS_PARA 2
/* DS number*/
#define DS0  0
#define DS1  1
#define DS2  2
#define DS3  3
#define DS4  4
#define DS5  5
#define DS_ALL 0xff
#define INVAID_DS_PARA 6

/*line rate mode */
#define  GE_1250    0
#define  GE_3125    1
#define  XGE_10312  2
#define  PCIE_2500  3
#define  PCIE_5000  4
#define  PCIE_8000  5
#define  SAS_1500   6
#define  SAS_3000   7
#define  SAS_6000   8
#define  SAS_12000  9
#define  HCCS_32    10
#define  HCCS_40    11

#define  PMA_MODE_PCIE      0
#define  PMA_MODE_SAS       1
#define  PMA_MODE_NORMAL    2

/*PRBS ÂëÁ÷*/
#define  PRBS7  0
#define  PRBS9  1
#define  PRBS10 2
#define  PRBS11 3
#define  PRBS15 4
#define  PRBS20 5
#define  PRBS23 6
#define  PRBS31 7
#define  PRBS_MODE_CUSTOM  8

/*AC or DC couple mode flag ACÎª0  DCÎª1*/
#define   AC_MODE_FLAG  0
#define   DC_MODE_FLAG  1
#define   COUPLE_MODE_FLAG   AC_MODE_FLAG

#define MACRO_0_SLICES_NUM 10
#define MACRO_1_SLICES_NUM 10
#define MACRO_2_SLICES_NUM 10
#define MACRO_3_SLICES_NUM 6
#define MACRO_4_SLICES_NUM 6
#define MACRO_5_SLICES_NUM 6
#define MACRO_6_SLICES_NUM 6

/*serdes CS/DS_CLK/TX/RX¼Ä´æÆ÷µÄ¸öÊý*/
#define MAX_CS_CSR_NUM   61
#define MAX_DSCLK_CSR_NUM  31
#define MAX_TX_CSR_NUM   62
#define MAX_RX_CSR_NUM  63
#define CP_CSR_NUM  7

#define CS_CSR(num ,reg)       (((0x0000+(reg)*0x0002+(num)*0x0200))*2)
#define DSCLK_CSR(lane,reg)    (((0x4100+(reg)*0x0002+(lane)*0x0200))*2)
#define RX_CSR(lane,reg)       (((0x4080+(reg)*0x0002+(lane)*0x0200))*2)
#define TX_CSR(lane,reg)       (((0x4000+(reg)*0x0002+(lane)*0x0200))*2)
#define CP_CSR(reg)            (((0x0FFF0+(reg)*0x0002))*2)

#define REG_BROADCAST          (0x2000 *2)
#define VERSION_OFFSET         (0x1800c *2)
#define FIRMWARE_BASE          (0x18000 *2)
#define API_BASE               (0x1FF6C *2)
#define API_END                (0x20000 *2)
#define CS_API                 (0xFFEC *2)
#define DS_API(lane)           ((0x1FF6c + 8*(15-lane))*2)
//#define Firmware_Size          (32620)
#define MCU_MASK               (0x8000 *2)

#define FULL_RATE    0x0
#define HALF_RATE    0x1
#define QUARTER_RATE 0x2

#define WIDTH_40BIT 0x3
#define WIDTH_32BIT 0x2
#define WIDTH_20BIT 0x1
#define WIDTH_16BIT 0x0

#define KA2_KB1  0x0
#define KA2_KB2  0x2
#define KA3_KB1  0x1
#define KA3_KB2  0x3

#define J_DIV_20  0x0
#define J_DIV_10  0x1
#define J_DIV_16  0x2
#define J_DIV_8   0x3

#define J_DIV_NO_PLUS2 0
#define J_DIV_PLUS2    1

/* ¶ÔÓ¦²»Í¬Ä£Ê½ºÍÏßËÙµÄÅäÖÃÃ¶¾ÙÁ¿*/
enum line_rate_cfg {
	LINE_RATE_GE_1250 = 0,
	LINE_RATE_GE_3125,
	LINE_RATE_XGE_10312,
	LINE_RATE_PCIE_2500,
	LINE_RATE_PCIE_5000,
	LINE_RATE_PCIE_8000,
	LINE_RATE_SAS_1500,
	LINE_RATE_SAS_3000,
	LINE_RATE_SAS_6000,
	LINE_RATE_SAS_12000,
	LINE_RATE_HCCS_12000,
	INVAID_LINERATE_CFG
};

/*ËÙÂÊÄ£Ê½½á¹¹Ìå*/
struct link_rate_cfg {
	//enum line_rate_cfg  rate;
	unsigned int number;
};

void sds_reg_bits_write(unsigned int node, unsigned int macro_id,
			unsigned int reg_offset, unsigned int high_bit,
			unsigned int low_bit, unsigned int reg_val);
unsigned int sds_reg_bits_read(unsigned int node, unsigned int macro_id,
			       unsigned int reg_offset, unsigned int high_bit,
			       unsigned int low_bit);
void serdes_cs_cfg(unsigned int node, unsigned int macro_id,
		   unsigned int cs_num, unsigned int cs_cfg);
unsigned int serdes_cs_calib(unsigned int node, unsigned int macro_id,
			     unsigned int cs_num);
void serdes_ds_cfg(unsigned int node, unsigned int macro_id,
		   unsigned int ds_num, unsigned int ds_cfg,
		   unsigned int cs_src);
unsigned int serdes_ds_calib(unsigned int node, unsigned int macro_id,
			     unsigned int ds_num, unsigned int ds_cfg);
void ds_hw_calibration_adjust(unsigned int node, unsigned int macro_id,
			      unsigned int ds_num);
void ds_hw_calibration_init(unsigned int node, unsigned int macro_id,
			    unsigned int ds_num);
unsigned int ds_hw_calibration_exec(unsigned int node, unsigned int macro_id,
				    unsigned int ds_num);
void ds_hw_calibration_adjust(unsigned int node, unsigned int macro_id,
			      unsigned int ds_num);
void ds_cfg_after_calibration(unsigned int node, unsigned int macro_id,
			      unsigned int ds_num, unsigned int ds_cfg,
			      unsigned int flag);

/* begin:Ö§³Ö2P arm server */
#define HILINK_REG_BASE_OFFSET		(0x40000000000ULL)

#define MASTER_CPU_NODE		(0)
#define SLAVE_CPU_NODE		(1)

#define HILINKMACRO_SIZE   (HILINK_REG_SIZE)
#define HILINK_NAME "hilink"

#define HILINKMACRO0       (0xB2000000)
#define HILINKMACRO1       (0xB2080000)
#define HILINKMACRO2       (0xB2100000)
#define HILINKMACRO3       (0xC2200000)
#define HILINKMACRO4       (0xC2280000)
#define HILINKMACRO5       (0xB2180000)
#define HILINKMACRO6       (0xB2200000)

#define SYS_CTRL_KERNEL    (0xF3000000+0xE00000)
#define SYS_CTRL_UP        (0x0f28f5000)
#define SERDES_CTRL0       (0x0F28F4000)
#define SERDES_CTRL1       (0x0F7802000)

#define HILINK0_MACRO_PWRDB    (0xB0002418)
#define HILINK1_MACRO_PWRDB    (0xB0002518)
#define HILINK2_MACRO_PWRDB    (0xC0002418)
#define HILINK3_MACRO_PWRDB    (0xC0002518)
#define HILINK4_MACRO_PWRDB    (0xC0002618)
#define HILINK5_MACRO_PWRDB    (0xB0002618)
#define HILINK6_MACRO_PWRDB    (0xB0002718)

#define HILINK0_MACRO_GRSTB    (0xB000241C)
#define HILINK1_MACRO_GRSTB    (0xB000251C)
#define HILINK2_MACRO_GRSTB    (0xC000241C)
#define HILINK3_MACRO_GRSTB    (0xC000251C)
#define HILINK4_MACRO_GRSTB    (0xC000261C)
#define HILINK5_MACRO_GRSTB    (0xB000261C)
#define HILINK6_MACRO_GRSTB    (0xB000271C)

#define HILINK0_MUX_CTRL        (0xB0002300)
#define HILINK1_MUX_CTRL        (0xB0002304)
#define HILINK2_MUX_CTRL        (0xB0002308)
#define HILINK5_MUX_CTRL        (0xB0002314)

#define SRE_SAS0_DSAF_CFG_BASE  (0xC0000000)
#define SRE_SAS1_PCIE_CFG_BASE  (0xB0000000)

#define SRE_HILINK3_CRG_CTRL0_REG              (SRE_SAS0_DSAF_CFG_BASE + 0x180)	/* ÅäÖÃHILINK CRG */
#define SRE_HILINK3_CRG_CTRL1_REG              (SRE_SAS0_DSAF_CFG_BASE + 0x184)	/* ÅäÖÃHILINK CRG */
#define SRE_HILINK3_CRG_CTRL2_REG              (SRE_SAS0_DSAF_CFG_BASE + 0x188)	/* ÅäÖÃHILINK CRG */
#define SRE_HILINK3_CRG_CTRL3_REG              (SRE_SAS0_DSAF_CFG_BASE + 0x18C)	/* ÅäÖÃHILINK CRG */
#define SRE_HILINK4_CRG_CTRL0_REG              (SRE_SAS0_DSAF_CFG_BASE + 0x190)	/* ÅäÖÃHILINK CRG */
#define SRE_HILINK4_CRG_CTRL1_REG              (SRE_SAS0_DSAF_CFG_BASE + 0x194)	/* ÅäÖÃHILINK CRG */
#define SRE_HILINK2_LRSTB_MUX_CTRL_REG         (SRE_SAS0_DSAF_CFG_BASE + 0x2340)	/* HILINK2 lrstb[7:0]µÄMUXÑ¡Ôñ¿ØÖÆ */
#define SRE_HILINK2_MACRO_SS_REFCLK_REG        (SRE_SAS0_DSAF_CFG_BASE + 0x2400)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK2_MACRO_CS_REFCLK_DIRSEL_REG (SRE_SAS0_DSAF_CFG_BASE + 0x2404)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK2_MACRO_LIFECLK2DIG_SEL_REG  (SRE_SAS0_DSAF_CFG_BASE + 0x2408)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK2_MACRO_CORE_CLK_SELEXT_REG  (SRE_SAS0_DSAF_CFG_BASE + 0x240C)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK2_MACRO_CORE_CLK_SEL_REG     (SRE_SAS0_DSAF_CFG_BASE + 0x2410)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK2_MACRO_CTRL_BUS_MODE_REG    (SRE_SAS0_DSAF_CFG_BASE + 0x2414)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK2_MACRO_MACROPWRDB_REG       (SRE_SAS0_DSAF_CFG_BASE + 0x2418)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK2_MACRO_GRSTB_REG            (SRE_SAS0_DSAF_CFG_BASE + 0x241C)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK2_MACRO_BIT_SLIP_REG         (SRE_SAS0_DSAF_CFG_BASE + 0x2420)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK2_MACRO_LRSTB_REG            (SRE_SAS0_DSAF_CFG_BASE + 0x2424)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK3_MACRO_SS_REFCLK_REG        (SRE_SAS0_DSAF_CFG_BASE + 0x2500)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK3_MACRO_CS_REFCLK_DIRSEL_REG (SRE_SAS0_DSAF_CFG_BASE + 0x2504)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK3_MACRO_LIFECLK2DIG_SEL_REG  (SRE_SAS0_DSAF_CFG_BASE + 0x2508)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK3_MACRO_CORE_CLK_SELEXT_REG  (SRE_SAS0_DSAF_CFG_BASE + 0x250C)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK3_MACRO_CORE_CLK_SEL_REG     (SRE_SAS0_DSAF_CFG_BASE + 0x2510)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK3_MACRO_CTRL_BUS_MODE_REG    (SRE_SAS0_DSAF_CFG_BASE + 0x2514)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK3_MACRO_MACROPWRDB_REG       (SRE_SAS0_DSAF_CFG_BASE + 0x2518)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK3_MACRO_GRSTB_REG            (SRE_SAS0_DSAF_CFG_BASE + 0x251C)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK3_MACRO_BIT_SLIP_REG         (SRE_SAS0_DSAF_CFG_BASE + 0x2520)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK3_MACRO_LRSTB_REG            (SRE_SAS0_DSAF_CFG_BASE + 0x2524)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK4_MACRO_SS_REFCLK_REG        (SRE_SAS0_DSAF_CFG_BASE + 0x2600)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK4_MACRO_CS_REFCLK_DIRSEL_REG (SRE_SAS0_DSAF_CFG_BASE + 0x2604)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK4_MACRO_LIFECLK2DIG_SEL_REG  (SRE_SAS0_DSAF_CFG_BASE + 0x2608)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK4_MACRO_CORE_CLK_SELEXT_REG  (SRE_SAS0_DSAF_CFG_BASE + 0x260C)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK4_MACRO_CORE_CLK_SEL_REG     (SRE_SAS0_DSAF_CFG_BASE + 0x2610)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK4_MACRO_CTRL_BUS_MODE_REG    (SRE_SAS0_DSAF_CFG_BASE + 0x2614)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK4_MACRO_MACROPWRDB_REG       (SRE_SAS0_DSAF_CFG_BASE + 0x2618)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK4_MACRO_GRSTB_REG            (SRE_SAS0_DSAF_CFG_BASE + 0x261C)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK4_MACRO_BIT_SLIP_REG         (SRE_SAS0_DSAF_CFG_BASE + 0x2620)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK4_MACRO_LRSTB_REG            (SRE_SAS0_DSAF_CFG_BASE + 0x2624)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK2_MACRO_PLLOUTOFLOCK_REG     (SRE_SAS0_DSAF_CFG_BASE + 0x6400)	/* HILINK×´Ì¬¼Ä´æÆ÷ */
#define SRE_HILINK2_MACRO_PRBS_ERR_REG         (SRE_SAS0_DSAF_CFG_BASE + 0x6404)	/* HILINK×´Ì¬¼Ä´æÆ÷ */
#define SRE_HILINK2_MACRO_LOS_REG              (SRE_SAS0_DSAF_CFG_BASE + 0x6408)	/* HILINK×´Ì¬¼Ä´æÆ÷ */
#define SRE_HILINK3_MACRO_PLLOUTOFLOCK_REG     (SRE_SAS0_DSAF_CFG_BASE + 0x6500)	/* HILINK×´Ì¬¼Ä´æÆ÷ */
#define SRE_HILINK3_MACRO_PRBS_ERR_REG         (SRE_SAS0_DSAF_CFG_BASE + 0x6504)	/* HILINK×´Ì¬¼Ä´æÆ÷ */
#define SRE_HILINK3_MACRO_LOS_REG              (SRE_SAS0_DSAF_CFG_BASE + 0x6508)	/* HILINK×´Ì¬¼Ä´æÆ÷ */
#define SRE_HILINK4_MACRO_PLLOUTOFLOCK_REG     (SRE_SAS0_DSAF_CFG_BASE + 0x6600)	/* HILINK×´Ì¬¼Ä´æÆ÷ */
#define SRE_HILINK4_MACRO_PRBS_ERR_REG         (SRE_SAS0_DSAF_CFG_BASE + 0x6604)	/* HILINK×´Ì¬¼Ä´æÆ÷ */
#define SRE_HILINK4_MACRO_LOS_REG              (SRE_SAS0_DSAF_CFG_BASE + 0x6608)	/* HILINK×´Ì¬¼Ä´æÆ÷ */

#define SRE_HILINK0_MUX_CTRL_REG               (SRE_SAS1_PCIE_CFG_BASE + 0x2300)	/* HILINK¸´ÓÃÑ¡Ôñ */
#define SRE_HILINK1_MUX_CTRL_REG               (SRE_SAS1_PCIE_CFG_BASE + 0x2304)	/* HILINK¸´ÓÃÑ¡Ôñ */
#define SRE_HILINK2_MUX_CTRL_REG               (SRE_SAS1_PCIE_CFG_BASE + 0x2308)	/* HILINK¸´ÓÃÑ¡Ôñ */
#define SRE_HILINK5_MUX_CTRL_REG               (SRE_SAS1_PCIE_CFG_BASE + 0x2314)	/* HILINK¸´ÓÃÑ¡Ôñ */
#define SRE_HILINK1_AHB_MUX_CTRL_REG           (SRE_SAS1_PCIE_CFG_BASE + 0x2324)	/* HILINK AHB¸´ÓÃÑ¡Ôñ */
#define SRE_HILINK2_AHB_MUX_CTRL_REG           (SRE_SAS1_PCIE_CFG_BASE + 0x2328)	/* HILINK AHB¸´ÓÃÑ¡Ôñ */
#define SRE_HILINK5_AHB_MUX_CTRL_REG           (SRE_SAS1_PCIE_CFG_BASE + 0x2334)	/* HILINK AHB¸´ÓÃÑ¡Ôñ */
#define SRE_HILINK5_LRSTB_MUX_CTRL_REG         (SRE_SAS1_PCIE_CFG_BASE + 0x2340)	/* HILINK5 lrstb[3:0]µÄMUXÑ¡Ôñ¿ØÖÆ */
#define SRE_HILINK6_LRSTB_MUX_CTRL_REG         (SRE_SAS1_PCIE_CFG_BASE + 0x2344)	/* HILINK6 lrstb[3:0]µÄMUXÑ¡Ôñ¿ØÖÆ */
#define SRE_HILINK0_MACRO_SS_REFCLK_REG        (SRE_SAS1_PCIE_CFG_BASE + 0x2400)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK0_MACRO_CS_REFCLK_DIRSEL_REG (SRE_SAS1_PCIE_CFG_BASE + 0x2404)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK0_MACRO_LIFECLK2DIG_SEL_REG  (SRE_SAS1_PCIE_CFG_BASE + 0x2408)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK0_MACRO_CORE_CLK_SELEXT_REG  (SRE_SAS1_PCIE_CFG_BASE + 0x240C)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK0_MACRO_CORE_CLK_SEL_REG     (SRE_SAS1_PCIE_CFG_BASE + 0x2410)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK0_MACRO_CTRL_BUS_MODE_REG    (SRE_SAS1_PCIE_CFG_BASE + 0x2414)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK0_MACRO_MACROPWRDB_REG       (SRE_SAS1_PCIE_CFG_BASE + 0x2418)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK0_MACRO_GRSTB_REG            (SRE_SAS1_PCIE_CFG_BASE + 0x241C)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK0_MACRO_BIT_SLIP_REG         (SRE_SAS1_PCIE_CFG_BASE + 0x2420)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK0_MACRO_LRSTB_REG            (SRE_SAS1_PCIE_CFG_BASE + 0x2424)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK1_MACRO_SS_REFCLK_REG        (SRE_SAS1_PCIE_CFG_BASE + 0x2500)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK1_MACRO_CS_REFCLK_DIRSEL_REG (SRE_SAS1_PCIE_CFG_BASE + 0x2504)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK1_MACRO_LIFECLK2DIG_SEL_REG  (SRE_SAS1_PCIE_CFG_BASE + 0x2508)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK1_MACRO_CORE_CLK_SELEXT_REG  (SRE_SAS1_PCIE_CFG_BASE + 0x250C)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK1_MACRO_CORE_CLK_SEL_REG     (SRE_SAS1_PCIE_CFG_BASE + 0x2510)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK1_MACRO_CTRL_BUS_MODE_REG    (SRE_SAS1_PCIE_CFG_BASE + 0x2514)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK1_MACRO_MACROPWRDB_REG       (SRE_SAS1_PCIE_CFG_BASE + 0x2518)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK1_MACRO_GRSTB_REG            (SRE_SAS1_PCIE_CFG_BASE + 0x251C)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK1_MACRO_BIT_SLIP_REG         (SRE_SAS1_PCIE_CFG_BASE + 0x2520)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK1_MACRO_LRSTB_REG            (SRE_SAS1_PCIE_CFG_BASE + 0x2524)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK5_MACRO_SS_REFCLK_REG        (SRE_SAS1_PCIE_CFG_BASE + 0x2600)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK5_MACRO_CS_REFCLK_DIRSEL_REG (SRE_SAS1_PCIE_CFG_BASE + 0x2604)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK5_MACRO_LIFECLK2DIG_SEL_REG  (SRE_SAS1_PCIE_CFG_BASE + 0x2608)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK5_MACRO_CORE_CLK_SELEXT_REG  (SRE_SAS1_PCIE_CFG_BASE + 0x260C)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK5_MACRO_CORE_CLK_SEL_REG     (SRE_SAS1_PCIE_CFG_BASE + 0x2610)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK5_MACRO_CTRL_BUS_MODE_REG    (SRE_SAS1_PCIE_CFG_BASE + 0x2614)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK5_MACRO_MACROPWRDB_REG       (SRE_SAS1_PCIE_CFG_BASE + 0x2618)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK5_MACRO_GRSTB_REG            (SRE_SAS1_PCIE_CFG_BASE + 0x261C)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK5_MACRO_BIT_SLIP_REG         (SRE_SAS1_PCIE_CFG_BASE + 0x2620)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK5_MACRO_LRSTB_REG            (SRE_SAS1_PCIE_CFG_BASE + 0x2624)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK6_MACRO_SS_REFCLK_REG        (SRE_SAS1_PCIE_CFG_BASE + 0x2700)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK6_MACRO_CS_REFCLK_DIRSEL_REG (SRE_SAS1_PCIE_CFG_BASE + 0x2704)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK6_MACRO_LIFECLK2DIG_SEL_REG  (SRE_SAS1_PCIE_CFG_BASE + 0x2708)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK6_MACRO_CORE_CLK_SELEXT_REG  (SRE_SAS1_PCIE_CFG_BASE + 0x270C)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK6_MACRO_CORE_CLK_SEL_REG     (SRE_SAS1_PCIE_CFG_BASE + 0x2710)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK6_MACRO_CTRL_BUS_MODE_REG    (SRE_SAS1_PCIE_CFG_BASE + 0x2714)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK6_MACRO_MACROPWRDB_REG       (SRE_SAS1_PCIE_CFG_BASE + 0x2718)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK6_MACRO_GRSTB_REG            (SRE_SAS1_PCIE_CFG_BASE + 0x271C)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK6_MACRO_BIT_SLIP_REG         (SRE_SAS1_PCIE_CFG_BASE + 0x2720)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK6_MACRO_LRSTB_REG            (SRE_SAS1_PCIE_CFG_BASE + 0x2724)	/* HILINKÅäÖÃ¼Ä´æÆ÷ */
#define SRE_HILINK0_MACRO_PLLOUTOFLOCK_REG     (SRE_SAS1_PCIE_CFG_BASE + 0x6400)	/* HILINK×´Ì¬¼Ä´æÆ÷ */
#define SRE_HILINK0_MACRO_PRBS_ERR_REG         (SRE_SAS1_PCIE_CFG_BASE + 0x6404)	/* HILINK×´Ì¬¼Ä´æÆ÷ */
#define SRE_HILINK0_MACRO_LOS_REG              (SRE_SAS1_PCIE_CFG_BASE + 0x6408)	/* HILINK×´Ì¬¼Ä´æÆ÷ */
#define SRE_HILINK1_MACRO_PLLOUTOFLOCK_REG     (SRE_SAS1_PCIE_CFG_BASE + 0x6500)	/* HILINK×´Ì¬¼Ä´æÆ÷ */
#define SRE_HILINK1_MACRO_PRBS_ERR_REG         (SRE_SAS1_PCIE_CFG_BASE + 0x6504)	/* HILINK×´Ì¬¼Ä´æÆ÷ */
#define SRE_HILINK1_MACRO_LOS_REG              (SRE_SAS1_PCIE_CFG_BASE + 0x6508)	/* HILINK×´Ì¬¼Ä´æÆ÷ */
#define SRE_HILINK5_MACRO_PLLOUTOFLOCK_REG     (SRE_SAS1_PCIE_CFG_BASE + 0x6600)	/* HILINK×´Ì¬¼Ä´æÆ÷ */
#define SRE_HILINK5_MACRO_PRBS_ERR_REG         (SRE_SAS1_PCIE_CFG_BASE + 0x6604)	/* HILINK×´Ì¬¼Ä´æÆ÷ */
#define SRE_HILINK5_MACRO_LOS_REG              (SRE_SAS1_PCIE_CFG_BASE + 0x6608)	/* HILINK×´Ì¬¼Ä´æÆ÷ */
#define SRE_HILINK6_MACRO_PLLOUTOFLOCK_REG     (SRE_SAS1_PCIE_CFG_BASE + 0x6700)	/* HILINK×´Ì¬¼Ä´æÆ÷ */
#define SRE_HILINK6_MACRO_PRBS_ERR_REG         (SRE_SAS1_PCIE_CFG_BASE + 0x6704)	/* HILINK×´Ì¬¼Ä´æÆ÷ */
#define SRE_HILINK6_MACRO_LOS_REG              (SRE_SAS1_PCIE_CFG_BASE + 0x6708)	/* HILINK×´Ì¬¼Ä´æÆ÷ */

#endif /*# HIGGS_SERDES_API_H */
