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

#ifndef __DRV_INITIATOR_H__
#define __DRV_INITIATOR_H__


struct drv_device_address {
    u16    initiator_id;        
    u16    bus_id;              
    u16    tgt_id;           
    u16    function_id;      
};

struct drv_ini_cmd {
    struct drv_scsi_cmd_result result;                      
    void*           upper_data;                        
    void*           driver_data;                    
    u8              cdb[DRV_SCSI_CDB_LEN];       
    u8              lun[DRV_SCSI_LUN_LEN];      
    u16             cmd_len;                    
    u16             tag;                      
    enum drv_io_direction    io_direction;          
    u32             data_len;                 
    u32             underflow;                  
    u32             overflow;                   
    u32             remain_data_len;             
    u64             port_id;                      
    u64             cmd_sn;                       
    struct drv_device_address  addr;                      
    struct drv_sgl*            sgl;                     
    void*           private_info;                      
    void            (*done)(struct drv_ini_cmd*);
    struct dif_info          dif;                     
};


struct drv_ini_private_info {
    u32             driver_type;                   
    void*           driver_data;                     
};

static inline struct drv_sgl* drv_sgl_trans(void* sgl, void* cmd, 
                                             u32 flag, struct drv_sgl* drv_sgl)
{
    struct scatterlist * scsi_sgl =NULL;

    if(unlikely(((NULL == drv_sgl)||(NULL == sgl)))) {
        printk("inparam is NULL.");
        return NULL;
    }
    
    scsi_sgl =(struct scatterlist * )sgl;

    drv_sgl->next_sgl = (void *)sg_next(scsi_sgl);      
    drv_sgl->num_sges_in_chain = scsi_sg_count((struct scsi_cmnd *)cmd);
    drv_sgl->num_sges_in_sgl = 1;
   
    drv_sgl->sge[0].buff = (char *)sg_virt(scsi_sgl);   
    drv_sgl->sge[0].len = scsi_sgl->length;             
    drv_sgl->sge[0].offset = 0;                          

    return drv_sgl;

}

#endif /*__DRV_SCSI_INITIATOR_H__*/


