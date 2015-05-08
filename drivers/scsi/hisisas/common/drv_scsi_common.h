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

#ifndef __DRV_SCSI_COMMON_H__
#define __DRV_SCSI_COMMON_H__

#define SCSI_SENSE_DATA_LEN 96

#define DRV_SCSI_CDB_LEN 16

#define DRV_SCSI_LUN_LEN 8


enum drv_io_direction {
    DRV_IO_BIDIRECTIONAL   = 0,              
    DRV_IO_DIRECTION_WRITE = 1,               
    DRV_IO_DIRECTION_READ  = 2,             
    DRV_IO_DIRECTION_NONE  = 3,        
};

enum drv_tm_type {
    DRV_TM_ABORT_TASK         = 0,              /*ABORT TASK*/
    DRV_TM_ABORT_TASK_SET     = 1,              /*ABORT TASK SET*/
    DRV_TM_CLEAR_ACA          = 2,              /*CLEAR ACA*/
    DRV_TM_CLEAR_TASK_SET     = 3,              /*CLEAR TASK SET*/
    DRV_TM_ITNEXUS_RESET      = 4,              /*IT NEXUS RESET*/
    DRV_TM_LOGICAL_UNIT_RESET = 5,              /*LOGICAL UNIT RESET*/
    DRV_TM_QUERY_TASK         = 6,              /*QUERY TASK*/
    DRV_TM_QUERY_TASK_SET     = 7,              /*QUERY TASK SET*/
    DRV_TM_ASYNCHRONOUS_EVENT = 8,              /*ASYNCHRONOUS EVENT*/
};

struct drv_scsi_cmd_result {
    u32  status;                          
    u16  sense_data_len;                 
    u8   sense_data[SCSI_SENSE_DATA_LEN];  
};

#endif /*__DRV_SCSI_COMMON_H__*/


