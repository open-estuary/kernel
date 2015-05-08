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

#ifndef __DRV_COMMON_INC__
#define __DRV_COMMON_INC__
 
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/pci.h>

#include <linux/workqueue.h>
#include <linux/libata.h>
#include <scsi/scsi_device.h>

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <linux/pci.h>
#include <linux/workqueue.h>
#include <linux/mod_devicetable.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_devinfo.h>   
#include <scsi/scsi_eh.h>

#include <scsi/scsi.h>
#include <scsi/scsi_tcq.h>
#include <scsi/scsicam.h>
#include <scsi/scsi_transport.h>

 
#ifndef CONFIG_ARM64
#include <linux/mc146818rtc.h>
#endif
 
#include <linux/kernel_stat.h>
#include <linux/timex.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/param.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/vmalloc.h>
#include <linux/jiffies.h>

#include <asm/io.h>
#include <asm/sections.h>

 #include <linux/ata.h>
#include <linux/device.h>
#include <linux/acpi.h>

#ifndef CONFIG_ARM64
#include <acpi/acpi_bus.h>
#endif
 #include <linux/pm.h>
#include <linux/dma-mapping.h>
#include <linux/dmi.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/highmem.h>

#include <linux/hdreg.h>
#include <linux/uaccess.h>
#include <linux/suspend.h>
#include <scsi/scsi_driver.h>
#include <linux/io.h>
#include <linux/async.h>
#include <linux/log2.h>
#include <asm/byteorder.h>
#include <linux/cdrom.h>
#include <scsi/scsi_dbg.h>
#include <linux/hash.h>

//fdef __SUSE11_SP1__
//nclude <linux/cpufreq.h>
//#endif

#include "drv_osal_lib.h"
#include "drv_scsi_common.h"
#include "drv_sgl.h"
#include "drv_data_protection.h"
#include "drv_initiator.h"
#include "drv_ini_device.h"
#include "drv_types_intf.h"

#define REFERNCE_VAR(var) 

#endif/*__DRV_COMMON_INC__*/

