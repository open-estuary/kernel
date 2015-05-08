/*
 * Huawei Pv660/Hi1610 sas controller driver sgl/sge definition
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


#ifndef __DRV_SGL_H__
#define __DRV_SGL_H__

#define DRV_ENTRY_PER_SGL 64

struct drv_sge {
    char*         buff;               
    void*         page_ctrl;          
    u32           len;               
    u32           offset;                 
};

struct drv_sgl {
    struct drv_sgl* next_sgl;           
    u16            num_sges_in_chain;       
    u16            num_sges_in_sgl;         
    u32            flag;             
    u64            serial_num;              
    struct drv_sge      sge[DRV_ENTRY_PER_SGL];
    struct list_head   	node;               
    u32            		cpu_id;                 
};

#endif /*__DRV_SGL_H__*/


