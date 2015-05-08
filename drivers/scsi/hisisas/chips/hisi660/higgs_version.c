/*
 * Huawei Pv660/Hi1610 sas controller driver version file
 *
 * Copyright 2015 Huawei. <chenjianmin@huawei.com>
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

#include "higgs_common.h"

const char compile_date[] = __DATE__;
const char compile_time[] = __TIME__;
#if defined(FPGA_VERSION_TEST)
const char higgs_sw_ver[] = "1.FPGA_VERSION.163";
#elif defined(EVB_VERSION_TEST)
const char higgs_sw_ver[] = "1.EVB_VERSION.163";
#elif defined(C05_VERSION_TEST)
const char higgs_sw_ver[] = "1.C05_VERSION.163";
#elif defined(PV660_ARM_SERVER)
const char higgs_sw_ver[] = "1.ARM_SERVER.K319";
#endif
