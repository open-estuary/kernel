/*
 * Huawei Pv660/Hi1610 sas controller driver common definition
 *
 * Copyright 2015 Huawei.
 *
 * This file is licensed under GPLv2.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
*/

#ifndef __SALCOMMON_H__
#define __SALCOMMON_H__

#include "drv_common.h"
#include <scsi/scsi_dbg.h>


#include "sal_intf.h"
#include "sadiscover.h"
#include "sal_event.h"
//#include "sasini_chgdelay.h"
#include "sal_ctrl.h"
#include "sal_commfunc.h"


extern char sal_compile_date[];
extern char sal_compile_time[];
extern char sal_sw_ver[];

extern struct sal_global_info sal_global;


#define SAL_MAX_DMA_SEGS		(128)


#define SAL_CARD_CLK_REG_DETEC(dcs_val_1,dcs_val_2,val)	((!((u32)(dcs_val_1)&0x01)) \
																||(!((u32)(dcs_val_2)&0x01)) \
																||(((u32)(val)>>14)&0x01) \
																||(((u32)(val)>>8)&0x01) \
																||(((u32)(val)>>2)&0x01))


#define SAL_TGT_STATUS(byte)     (byte)
#define SAL_MSG_STATUS(byte)     (byte << 8)
#define SAL_HOST_STATUS(byte)    (byte << 16)
#define SAL_DRV_STATUS(byte)     (byte << 24)

#define SAL_LOGIC_PORTID_TO_PORTNUM_MASK  0x1f

#define INQUIRY_MAX_CARD_NUM    6	

#define SAL_MAX_LOGIC_PORT  	(2*SAL_MAX_CARD_NUM)

#define SAL_SMP_TIMEOUT     100	/* SMP Timeout 1s */
#define SAL_SMP_RETRY_TIMES 30	

#define usdc_GET_ALL_PHY_ERR    0xff
#define SAL_GET_ALL_PHY_ERR     usdc_GET_ALL_PHY_ERR

#define SAL_INVALID_POSITION 0xDEADDEAD

#define SAL_TM_TIMEOUT       (3000)	/* 3000 ms */
#define SAL_ABORT_FW_TIMEOUT (3000)	/* 3000 ms */
#define SAL_ABORT_TIMEOUT    (200)	/* 200 ms */


#define OSP_DBL_CRITICAL    0 
#define OSP_DBL_MAJOR       1 
#define OSP_DBL_MINOR       2 
#define OSP_DBL_INFO        3 
#define OSP_DBL_DATA        4 
#define OSP_DBL_BUTT        6


#define SAL_ASSERT(expr, args...) \
    do \
    {\
        if (unlikely(!(expr)))\
        {\
            printk(KERN_EMERG "BUG! (%s,%s,%d)%s (called from %p)\n", \
                   __FILE__, __FUNCTION__, __LINE__, \
                   # expr, __builtin_return_address(0)); \
            SAS_DUMP_STACK();   \
            args; \
        } \
    } while (0)

#define SAL_TRACE(debug_level,id,X,args...)  \
    do\
    {\
        if (debug_level <= sal_global.sal_log_level)\
        {\
			DRV_LOG(0/*MID_SAS_INI*/, debug_level,id, X, ## args);	\
        } \
    }while(0)

#define SAL_TRACE_FRQLIMIT(debug_level,interval,id,X,args...) \
do \
{ \
     static unsigned long last = 0; \
     if ( time_after_eq(jiffies,last+(interval))) \
     { \
       last = jiffies; \
	   SAL_TRACE(debug_level,id, X,## args);\
     } \
} while(0)

#define SAL_TRACE_LIMIT(debug_level,interval,count,id,X,args...) \
do \
{ \
     static unsigned long last = 0; \
     static unsigned long local_count = 0; \
     if(local_count<count)	\
     {	\
	 	local_count++;	\
		SAL_TRACE(debug_level,id, X,## args);\
	 }	\
     if ( time_after_eq(jiffies,last+(interval))) \
     { \
       last = jiffies; \
       local_count = 0;	\
     } \
} while(0)



#ifndef MIN
#define MIN(a,b) ( ((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ( ((a)>(b))?(a):(b))
#endif

#define SAL_IS_CARD_READY(card)  ((card->flag & SAL_CARD_ACTIVE) \
                                && (!(card->flag & SAL_CARD_REMOVE_PROCESS)) \
                                && (!(card->flag & SAL_CARD_RESETTING)) \
                                && (!(card->flag & SAL_CARD_LOOPBACK))\
                                && (!(card->flag & SAL_CARD_FATAL))\
                                )


#define SAL_IS_CARD_NOT_ACTIVE(card) 	((card->flag & SAL_CARD_INIT_PROCESS)\
									||(card->flag & SAL_CARD_REMOVE_PROCESS)\
									||(card->flag & SAL_CARD_LOOPBACK)\
									||(card->flag & SAL_CARD_FATAL)\
									)

#define SAL_IS_RW_CMND(cdb0)	(	\
					(WRITE_6 == (cdb0)) || (WRITE_10 == (cdb0))\
					|| (WRITE_12 == (cdb0)) || (WRITE_16 == (cdb0))\
					|| (READ_6 == (cdb0)) || (READ_10 == (cdb0))\
					|| (READ_12 == (cdb0)) || (READ_16 == (cdb0)))

#define SAL_IS_READ_CMND(cdb0) ((READ_6 == (cdb0))\
							 || (READ_10 == (cdb0))\
							 || (READ_12 == (cdb0))\
							 || (READ_16 == (cdb0)))

#define SAL_IS_WRITE_CMND(cdb0) ((WRITE_6 == (cdb0))\
							 || (WRITE_10 == (cdb0))\
							 || (WRITE_12 == (cdb0))\
							 || (WRITE_16 == (cdb0)))

#define SAL_DMA_BEBIT32_TO_BIT32(x) ((u32)(((((u32)(x))&0x000000FF)<<24)|  \
                                     ((((u32)(x))&0x0000FF00)<<8)|   \
                                     ((((u32)(x))&0x00FF0000)>>8)|   \
                                     ((((u32)(x))&0xFF000000)>>24)))

#define SAL_PHY_ERR_CODE_NO         0
#define SAL_PHY_ERR_CODE_EXIST      1

#define SAL_SHIFT1                                    1
#define SAL_SHIFT2                                    2
#define SAL_SHIFT3                                    3
#define SAL_SHIFT4                                    4
#define SAL_SHIFT5                                    5
#define SAL_SHIFT6                                    6
#define SAL_SHIFT7                                    7
#define SAL_SHIFT8                                    8
#define SAL_SHIFT9                                    9
#define SAL_SHIFT10                                   10
#define SAL_SHIFT11                                   11
#define SAL_SHIFT12                                   12
#define SAL_SHIFT13                                   13
#define SAL_SHIFT14                                   14
#define SAL_SHIFT15                                   15
#define SAL_SHIFT16                                   16
#define SAL_SHIFT17                                   17
#define SAL_SHIFT18                                   18
#define SAL_SHIFT19                                   19
#define SAL_SHIFT20                                   20
#define SAL_SHIFT21                                   21
#define SAL_SHIFT22                                   22
#define SAL_SHIFT23                                   23
#define SAL_SHIFT24                                   24
#define SAL_SHIFT25                                   25
#define SAL_SHIFT26                                   26
#define SAL_SHIFT27                                   27
#define SAL_SHIFT28                                   28
#define SAL_SHIFT29                                   29
#define SAL_SHIFT30                                   30
#define SAL_SHIFT31                                   31

#if (BITS_PER_LONG == 64)
#define SAL_LOW_32_BITS(addr)   (u32)((addr) & 0xffffffff)
#define SAL_HIGH_32_BITS(addr)  (u32)(((addr) >> 32) & 0xffffffff)
#else
#define SAL_LOW_32_BITS(addr)   (u32)((addr) & 0xffffffff)
#define SAL_HIGH_32_BITS(addr)  0
#endif

enum sal_drv_event_type {
	SAL_DRV_DEVT_IN = 0,		
	SAL_DRV_DEVT_OUT,			
	SAL_DRV_DEVT_PORT_CHIP_FAULT	   
};

#define SAL_REF(a) ((a)=(a))
#define SAL_AND(a,b) ((a)&&(b))
#define SAL_OR(a,b) ((a)||(b))

#define TOLOWER(x) ((x) | 0x20)

#define SAL_LOWERANDX(cp) ((TOLOWER(*(cp)) == 'x') && isxdigit((cp)[1]))
#define SAL_LOWERAND0(cp) (((cp)[0] == '0') && (TOLOWER((cp)[1]) == 'x'))
#if 0 /* modify by chenqilin */
/*BEGIN:add by jianghong,2013-7-4,问题单号:DTS2013051700014:打开关闭端口PHY功能未正常实现*/
#define SAL_LOGICID_TO_CARDID(ID) \
          (((SPANGEA_V2R1 == sal_global.prd_type)\
	     &&(0 == (((ID) >> DRV_PORT_BOARD_TYPE_SHIFT) & 0x7)))\
          ?((((ID)>>8)&0x1F)+1):(((ID)>>8)&0x1F))
/*END:add by jianghong,2013-7-4,问题单号:DTS2013051700014*/
#endif
#define SAL_MASK_16                 0xFFFF
#define SAL_MASK_32                 0xFFFFFFFF


#define SAL_SOFT_RESET_HW           0
#define SAL_CHIP_RESET_HW           1

#define SAL_IO_STAT_ENABLE           1
#define SAL_IO_STAT_DISABLE          0


#define SAL_PCIE_AER_UIES(uies) (((uies) >> 22) & 0x1)
#define SAL_PCIE_AER_URS(uies) (((uies) >> 20) & 0x1)
#define SAL_PCIE_AER_ECRC_ERR_ST(uies) (((uies) >> 19) & 0x1)
#define SAL_PCIE_AER_MTS(uies) (((uies) >> 18) & 0x1)
#define SAL_PCIE_AER_RX_OVER_FLW_ST(uies) (((uies) >> 17) & 0x1)
#define SAL_PCIE_AER_UCS(uies) (((uies) >> 16) & 0x1)
#define SAL_PCIE_AER_CAS(uies) (((uies) >> 15) & 0x1)
#define SAL_PCIE_AER_CTS(uies) (((uies) >> 14) & 0x1)
#define SAL_PCIE_AER_FPS(uies) (((uies) >> 13) & 0x1)
#define SAL_PCIE_AER_PTS(uies) (((uies) >> 12) & 0x1)
#define SAL_PCIE_AER_DLS(uies) (((uies) >> 4) & 0x1)

#define SAL_PCIE_AER_HLOFS(uies) (((uies) >> 15) & 0x1)
#define SAL_PCIE_AER_CIES(uies) (((uies) >> 14) & 0x1)
#define SAL_PCIE_AER_ANFES(uies) (((uies) >> 13) & 0x1)
#define SAL_PCIE_AER_RTS(uies) (((uies) >> 12) & 0x1)
#define SAL_PCIE_AER_RNS(uies) (((uies) >> 8) & 0x1)
#define SAL_PCIE_AER_BDS(uies) (((uies) >> 7) & 0x1)
#define SAL_PCIE_AER_BTS(uies) (((uies) >> 6) & 0x1)
#define SAL_PCIE_AER_RXS(uies) (((uies) >> 0) & 0x1)

#define SAL_RETURN_MID_STATUS   0x1000

enum sal_chip_type {
	SAL_CHIP_TYPE_8070 = 0x8070,
	SAL_CHIP_TYPE_8072 = 0x8072
};


enum sal_upd_err_code {
	RETURN_UPDATE_FLASH_ERROR = 0X1001,
	RETURN_UPDATE_EEP_ERROR,
	RETURN_UPDATE_RESET_ERROR
};

struct gpe_handler {
    u32 gpe_event;                               
    void (*gpe_event_handler)(void * data);    
};

struct intr_event_handler {
    u32 int_event;                              
    void (*int_event_handler)(void * data);    
};

typedef struct tag_peripheral_operation_struct
{
  
    s32   (*get_product_type)(u8 *v_pucpro_type);
    s32   (*get_dev_position)( u8 bus_id, u8 *puc_dev_pos );
    s32   (*get_cmos_time)(u32 *cmos_time);
    s32   (*get_unique_dev_position)(u8 bus_id, u8 *puc_dev_pos);
    
    s32   (*get_card_type)(u8 card_id, u8 *card_type);
    s32   (*read_logic)(u32 address, u8 *puc_data);
    s32   (*write_logic)(u32 address, u8 uc_data);
    
    int   (*i2c_capi_do_send)(int bus_id, char chip_addr,
                                                   unsigned int sub_addr, char *buf, 
                                                   unsigned int size,int sub_addr_len);

    int   (*i2c_api_do_recv)(int bus_id, char chip_addr, 
                                                     unsigned int sub_addr, char *buf, 
                                                     unsigned int size,int sub_addr_len);
    int   (*i2c_simple_read)( int chan, unsigned char slave_addr,
                                                            unsigned int sub_addr, char* obuffer, 
                                                          unsigned int rx_len , int sub_addr_len);
    int  (*i2c_simple_write)(int chan, unsigned char slave_addr,
                                                          unsigned int sub_addr, char* ibuffer, 
                                                          unsigned int tx_len , int sub_addr_len);
    s32   (*register_int_handler)(struct intr_event_handler *int_handler_reg);
    s32   (*unregister_int_handler)(u32 int_event);
    u32   (*serdes_lane_reset)(u32 node, u32 macro_id,u32 ds_num,u32 ds_cfg);

    void   (*serdes_enable_ctledfe)(u32 macro, u32 lane, u32 ds_cfg);
    s32   (*register_gpe_handler)(struct gpe_handler *gpe_handler_reg);

    s32   (*gpe_interrupt_clear)(u32 gpe_event);
    s32   (*get_bsp_version)(u8 *puc_str);
    s32   (*bsp_reboot)(void);
    s32   (*read_slot_id)(u8 *pui_slot_id);
}peripheral_operation_struct;

extern peripheral_operation_struct  sal_peripheral_operation;

enum sal_err_type {
	SAS_ERR_INVALID_DWORD = 0,
	SAS_ERR_DISPARITY_ERROR,
	SAS_ERR_LOSSDWORD_SYNCH,
	SAS_ERR_CODE_VIOLATION,
	SAS_ERR_PHY_RESET_PROBLEM,
	SAS_ERR_BUTT
};

enum sal_chip_chk_ret {
	SAL_ROUTINE_CHK_RET_OK = 0x0,
	SAL_ROUTINE_CHK_RET_CHIP_PD,
	SAL_ROUTINE_CHK_RET_CHIP_RST,
	SAL_ROUTINE_CHK_RET_BUTT
};

enum sal_init_fail_reason {
	SAL_SLOT_NO_CARD = 0,	
	SAL_SCSI_HOST_ALLOC_FAILED,
	SAL_SCSI_HOST_REGISTER_FAILED,	
	SAL_TGT_HOST_ALLOC_FAILED,	
	SAL_CONFIG_INIT_FAILED,
	SAL_INTR_INIT_FAILED,	
	SAL_PCIE_INIT_FAILED,	
	SAL_MEM_INIT_FAILED,	
	SAL_RSC_ALLOC_FAILED,
	SAL_RSC_INIT_FAILED,	
	SAL_HW_INIT_FAILED,
	SAL_IO_ROUTINE_INIT_FAILED,
	SAL_DISC_ROUTINE_INIT_FAILED,	
	SAL_EVENT_ROUTINE_INIT_FAILED,
	SAL_DEV_DELAY_INIT_FAILED,
	SAL_WIRE_ROUTINE_INIT_FAILED,	
	SAL_CARD_INIT_SUCC,	
	SAL_CARD_FAILREASON_BUTT
};

struct sal_port_wwn {
	u64 port_wwn;		/*local port wwn */
	u64 rcv_port_wwn;	/*remote port wwn */
};

#define QUARK_PCIE_DIAG_BYTES  (16 * 1024)

#define QUARK_PCIE_DIAG_WRITE_BACK           1
#define QUARK_PCIE_DIAG_WRITE_BACK_WITH_DIF  2

/* PCIE Diagnostic Command Type */
#define QUARK_CMD_TYPE_PCIE_DIAG_OPRN_PERFORM             0x00
#define QUARK_CMD_TYPE_PCIE_DIAG_OPRN_STOP                0x01
#define QUARK_CMD_TYPE_PCIE_DIAG_THRESHOLD_SPECIFY        0x02
#define QUARK_CMD_TYPE_PCIE_DIAG_RECEIVE_ENABLE       	  0x03
#define QUARK_CMD_TYPE_PCIE_DIAG_REPORT_GET               0x04
#define QUARK_CMD_TYPE_PCIE_DIAG_ERR_CNT_RESET            0x05

struct pcie_diag_exec_param {
	u32 command;
	u32 flag;
	u32 initial_io_seed;
	u32 resved;
	u32 rd_addr_lo;
	u32 rd_addr_hi;
	u32 wr_addr_lo;
	u32 wr_addr_hi;
	u32 len;
	u32 pattern;
	u32 udt_arr[6];
	u32 udr_arr[6];
};

enum sal_card_clock {
	SAL_CARD_CLK_NORMAL = 0,
	SAL_CARD_CLK_ABNORMAL,	
	SAL_CARD_CLK_BUTT
};

#endif
