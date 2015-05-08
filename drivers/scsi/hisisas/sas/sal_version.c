/*
 * Huawei Pv660/Hi1610 sas controller version file
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

#include "sal_common.h"

char sal_compile_date[] = __DATE__;
char sal_compile_time[] = __TIME__;
char sal_sw_ver[] = "1.03.119";

char ll_sw_ver[SAL_MAX_CARD_NUM][SAL_MAX_CHIP_VER_STR_LEN];

EXPORT_SYMBOL(ll_sw_ver);
