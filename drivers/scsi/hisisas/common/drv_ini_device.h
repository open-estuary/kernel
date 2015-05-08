/*
 * Huawei Pv660/Hi1610 sas controller driver macros
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

#ifndef __DRV_INI_DEVICE_H__
#define __DRV_INI_DEVICE_H__

enum drv_ini_opcode {
    DRV_INI_QUEUECMD,            /* queue command*/
    DRV_INI_EH_ABORTCMD,         /* ERR handle, abort command*/
    DRV_INI_EH_RESETDEVICE,      /* ERR handle, reset device*/
    DRV_INI_EH_RESETHOST,        /* ERR handle, reset host*/
    DRV_INI_SLAVE_ALLOC,         /* slave allocate*/
    DRV_INI_SLAVE_DESTROY,       /* slave destroy*/
    DRV_INI_CHANGE_QUEUEDEPTH,   /* change queue depth*/
    DRV_INI_CHANGE_QUEUETYPE,    /* change queue type*/
    DRV_INI_INITIATOR_CTRL,      /* initiator control*/
    DRV_INI_DEVICE_CTRL,         /* device control*/
    
    DRV_INI_OPCODE_BUTT
};

struct drv_device {
    struct drv_device_address* addr;    /*SCSI device address*/
    void* device;            /*Initiator driver device*/
};

#endif


