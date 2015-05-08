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

#ifndef __TYPES_INTF_H__
#define __TYPES_INTF_H__

#ifdef __cplusplus
 #if __cplusplus
extern "C" {
 #endif
#endif

enum drv_factory {
    DRV_FACTOR_GENERAL,   
    DRV_FACTOR_PMC,        
    DRV_FACTOR_MAV,        
    DRV_FACTOR_LSI,        
    DRV_FACTOR_MLX,       
    DRV_FACTOR_BUTT
};

#define DRV_PORT_RESERVE_SHIFT 28
#define DRV_PORT_RESERVE_MASK 0xf0000000

#define DRV_PORT_CONTROL_ID_SHIFT 24
#define DRV_PORT_CONTROL_ID_MASK 0x0f000000

#define DRV_PORT_FACTORY_SHIFT 20
#define DRV_PORT_FACTORY_MASK 0x00f00000

#define DRV_PORT_DRIVER_SHIFT 16
#define DRV_PORT_DRIVER_MASK 0x000f0000

#define DRV_PORT_BOARD_TYPE_SHIFT 13
#define DRV_PORT_BOARD_TYPE_MASK 0x0000E000

#define DRV_PORT_BOARD_ID_SHIFT 8
#define DRV_PORT_BOARD_ID_MASK 0x00001f00

#define DRV_PORT_NO_SHIFT 0
#define DRV_PORT_NO_MASK 0x000000ff


#define DRV_BUILD_FACTORY_PORT_ID(Factory, driver_type, BoardType, BoardID, PortNo) \
((((Factory) << DRV_PORT_FACTORY_SHIFT) & DRV_PORT_FACTORY_MASK)           \
|(((driver_type) << DRV_PORT_DRIVER_SHIFT) & DRV_PORT_DRIVER_MASK)          \
|(((BoardType) << DRV_PORT_BOARD_TYPE_SHIFT) & DRV_PORT_BOARD_TYPE_MASK)   \
|(((BoardID) << DRV_PORT_BOARD_ID_SHIFT) & DRV_PORT_BOARD_ID_MASK)         \
|(((PortNo) << DRV_PORT_NO_SHIFT) & DRV_PORT_NO_MASK))

#define DRV_PORT_NOFACTORY_DRIVER_MASK 0x007f0000


#define DRV_BUILD_NOFACTORY_PORT_ID(driver_type, BoardType, BoardID, PortNo) \
((((driver_type) << DRV_PORT_DRIVER_SHIFT) & DRV_PORT_NOFACTORY_DRIVER_MASK)\
|(((BoardType) << DRV_PORT_BOARD_TYPE_SHIFT) & DRV_PORT_BOARD_TYPE_MASK)   \
|(((BoardID) << DRV_PORT_BOARD_ID_SHIFT) & DRV_PORT_BOARD_ID_MASK)         \
|(((PortNo) << DRV_PORT_NO_SHIFT) & DRV_PORT_NO_MASK))


#define DRV_BUILD_PORT_ID(Factory, driver_type, BoardType, BoardID, PortNo)\
    ((DRV_FACTOR_GENERAL == Factory)?\
    DRV_BUILD_NOFACTORY_PORT_ID(driver_type, BoardType, BoardID, PortNo):\
    DRV_BUILD_FACTORY_PORT_ID(Factory, driver_type, BoardType, BoardID, PortNo))

#ifdef __cplusplus
 #if __cplusplus
}
 #endif/**  __cpluscplus */
#endif /**  __cpluscplus */

#endif  /** __TYPES_INTF_H__ */


