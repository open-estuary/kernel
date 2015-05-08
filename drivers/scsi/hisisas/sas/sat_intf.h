/*
 * Huawei Pv660/Hi1610 sata driver related interface
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

#ifndef __SAT_INTF_H__
#define __SAT_INTF_H__

#include "sat_protocol.h"
#include "sat_common.h"
extern s32 sat_process_command(struct stp_req *ata_cmd,
			       struct sat_ini_cmnd *scsi_cmd,
			       struct sata_device *sata_dev);
extern s32 sat_abort_task_mgmt(struct stp_req *tm_stp_cmd,
			       struct stp_req *org_stp_cmd);

#endif
