/*
 * Huawei Pv660/Hi1610 sas controller driver interface
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
#ifndef __DRV_DATA_PROTETION_H__
#define __DRV_DATA_PROTETION_H__

#define DRV_DATA_PROTECION_LEN              8

#define DRV_DIF_ACTION_NONE                 0

#define DRV_VERIFY_MASK                     (0xFF)

#define DRV_VERIFY_CRC_MASK                 (1<<1)

#define DRV_VERIFY_APP_MASK                 (1<<2)

#define DRV_VERIFY_LBA_MASK                 (1<<3)

#define DRV_REPLACE_MASK                    (0xFF<<8)

#define DRV_REPLACE_CRC_MASK                (1<<8)

#define DRV_REPLACE_APP_MASK                (1<<9)

#define DRV_REPLACE_LBA_MASK                (1<<10)

#define DRV_DIF_ACTION_MASK                 (0xFF<<16)

#define DRV_DIF_ACTION_INSERT               (0x1<<16)

#define DRV_DIF_ACTION_VERIFY_AND_DELETE    (0x2<<16)

#define DRV_DIF_ACTION_VERIFY_AND_FORWARD   (0x3<<16)

#define DRV_DIF_ACTION_VERIFY_AND_REPLACE   (0x4<<16)

#define DRV_DIF_OTHER_CONTROL_MASK          (0xFF<<24)

#define DRV_NOT_INCREACE_LBA_COUNT          (0x1<<24)

#define DRV_DIF_CRC_POS 0
#define DRV_DIF_CRC_LEN 2
#define DRV_DIF_APP_POS 2
#define DRV_DIF_APP_LEN 2
#define DRV_DIF_LBA_POS 4
#define DRV_DIF_LBA_LEN 4
#define DRV_DIF_CRC_ERR 0x1001
#define DRV_DIF_LBA_ERR 0x1002
#define DRV_DIF_APP_ERR 0x1003

struct dif_result_info {
    u8 actual_dif[DRV_DATA_PROTECION_LEN];      
    u8 expected_dif[DRV_DATA_PROTECION_LEN];    
};

struct dif_info {
    struct dif_result_info dif_result;        
    u32           protect_opcode;      
    u16           app_tag;          
    u64           start_lba;           
    struct drv_sgl*          protection_sgl;    
};

#endif /*__DRV_DATA_PROTETION_H__*/


