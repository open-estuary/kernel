/*
 * Copyright (c) 2015 Hisilicon Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __LINUX_ACPI_MDIO_H
#define __LINUX_ACPI_MDIO_H

#include <linux/phy.h>

#ifdef CONFIG_ACPI

int acpi_mdio_parse_addr(struct device *dev, struct fwnode_handle *fwnode);
int acpi_mdiobus_register(struct mii_bus *mdio, struct fwnode_handle *fwnode);
struct phy_device *acpi_phy_find_device(struct fwnode_handle *phy_fwnode);
struct phy_device *acpi_phy_attach(struct net_device *dev,
				   struct fwnode_handle *phy_fwnode, u32 flags,
				   phy_interface_t iface);
struct phy_device *acpi_phy_connect(struct net_device *dev,
				    struct fwnode_handle *phy_fwnode,
				    void (*hndlr)(struct net_device *),
				    u32 flags,
				    phy_interface_t iface);

#else
static inline int acpi_mdio_parse_addr(struct device *dev,
				       struct fwnode_handle *fwnode)
{
	return -ENXIO;
}

static inline int acpi_mdiobus_register(struct mii_bus *mdio,
					struct fwnode_handle *fwnode)
{
	return -ENXIO;
}

static inline
struct phy_device *acpi_phy_find_device(struct fwnode_handle *phy_fwnode)
{
	return NULL;
}

static inline
struct phy_device *acpi_phy_attach(struct net_device *dev,
				   struct fwnode_handle *phy_fwnode, u32 flags,
				   phy_interface_t iface)
{
	return NULL;
}

static inline
struct phy_device *acpi_phy_connect(struct net_device *dev,
				    struct fwnode_handle *phy_fwnode,
				    void (*hndlr)(struct net_device *),
				    u32 flags,
				    phy_interface_t iface)
{
	return NULL;
}

#endif

#endif /* __LINUX_ACPI_MDIO_H */
