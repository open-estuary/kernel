/*
 * Copyright (c) 2015 Hisilicon Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/acpi.h>
#include <linux/acpi_mdio.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/phy.h>
#include <linux/phy_fixed.h>
#include <linux/platform_device.h>
#include <linux/spinlock_types.h>

static
int acpi_mdiobus_register_phy(struct mii_bus *mdio, struct fwnode_handle *child,
			      u32 addr)
{
	struct phy_device *phy;
	const char *phy_type;
	bool is_c45;
	int rc;

	rc = fwnode_property_read_string(child, "ethernet-phy", &phy_type);
	if (rc < 0)
		return rc;

	if (!strncmp(phy_type, "ethernet-phy-ieee802.3-c45",
		     sizeof("ethernet-phy-ieee802.3-c45")))
		is_c45 = 1;
	else if (!strncmp(phy_type, "ethernet-phy-ieee802.3-c22",
			  sizeof("ethernet-phy-ieee802.3-c22")))
		is_c45 = 0;
	else
		return -ENODATA;

	phy = get_phy_device(mdio, addr, is_c45);
	if (!phy || IS_ERR(phy))
		return 1;

	/* Associate the fw node with the device structure so it
	 * can be looked up later
	 */
	phy->dev.fwnode = child;

	if (mdio->irq)
		phy->irq = mdio->irq[addr];

	if (fwnode_property_read_bool(child, "broken-turn-around"))
		mdio->phy_ignore_ta_mask |= 1 << addr;

	/* All data is now stored in the phy struct;
	 * register it
	 */
	rc = phy_device_register(phy);
	if (rc) {
		phy_device_free(phy);
		return 1;
	}

	dev_dbg(&mdio->dev, "registered phy at address %i\n", addr);

	return 0;
}

int acpi_mdio_parse_addr(struct device *dev, struct fwnode_handle *fwnode)
{
	u32 addr;
	int ret;

	ret = fwnode_property_read_u32(fwnode, "phy-addr", &addr);
	if (ret < 0) {
		dev_err(dev, "has invalid PHY address ret:%d\n", ret);
		return ret;
	}

	if (addr >= PHY_MAX_ADDR) {
		dev_err(dev, "PHY address %i is too large\n", addr);
		return -EINVAL;
	}

	return addr;
}
EXPORT_SYMBOL(acpi_mdio_parse_addr);

/**
 * acpi_mdiobus_register - Register mii_bus and create PHYs
 * @mdio: pointer to mii_bus structure
 * @fwnode: pointer to framework node of MDIO bus.
 *
 * This function registers the mii_bus structure and registers a phy_device
 * for each child node of mdio device.
 */
int acpi_mdiobus_register(struct mii_bus *mdio, struct fwnode_handle *fwnode)
{
	struct fwnode_handle *child;
	struct acpi_device *adev;
	bool scanphys = false;
	int addr, rc, i;

	/* Mask out all PHYs from auto probing.  Instead the PHYs listed in
	 * the framework node are populated after the bus has been registered
	 */
	mdio->phy_mask = ~0;

	/* Clear all the IRQ properties */
	if (mdio->irq)
		for (i = 0; i < PHY_MAX_ADDR; i++)
			mdio->irq[i] = PHY_POLL;

	mdio->dev.fwnode = fwnode;

	/* Register the MDIO bus */
	rc = mdiobus_register(mdio);
	if (rc)
		return rc;

	/* Loop over the child nodes and register a phy_device for each one */
	device_for_each_child_node(&mdio->dev, child) {
		adev = to_acpi_device_node(child);
		if (!adev)
			continue;

		addr = acpi_mdio_parse_addr(&adev->dev, child);
		if (addr < 0) {
			scanphys = true;
			continue;
		}

		rc = acpi_mdiobus_register_phy(mdio, child, addr);
		dev_dbg(&mdio->dev, "acpi reg phy rc:%#x addr:%#x\n", rc, addr);
		if (rc)
			continue;
	}

	if (!scanphys)
		return 0;

	/* auto scan for PHYs with empty reg property */
	device_for_each_child_node(&mdio->dev, child) {
		/* Skip PHYs with reg property set */
		if (!fwnode_property_present(child, "reg"))
			continue;

		for (addr = 0; addr < PHY_MAX_ADDR; addr++) {
			/* skip already registered PHYs */
			if (mdio->phy_map[addr])
				continue;

			/* be noisy to encourage people to set reg property */
			dev_info(&mdio->dev, "scan phy %s at address %i\n",
				 acpi_dev_name(to_acpi_device_node(child)),
				 addr);

			rc = acpi_mdiobus_register_phy(mdio, child, addr);
			if (rc)
				continue;
		}
	}

	return 0;
}
EXPORT_SYMBOL(acpi_mdiobus_register);

/* Helper function for acpi_phy_find_device */
static int acpi_phy_match(struct device *dev, void *phy_fwnode)
{
	return dev->fwnode == phy_fwnode;
}

/**
 * acpi_phy_find_device - Give a PHY node, find the phy_device
 * @phy_fwnode: Pointer to the phy's framework node
 *
 * If successful, returns a pointer to the phy_device with the embedded
 * struct device refcount incremented by one, or NULL on failure.
 */
struct phy_device *acpi_phy_find_device(struct fwnode_handle *phy_fwnode)
{
	struct device *d;

	if (!phy_fwnode)
		return NULL;

	d = bus_find_device(&mdio_bus_type, NULL, phy_fwnode, acpi_phy_match);

	return d ? to_phy_device(d) : NULL;
}
EXPORT_SYMBOL(acpi_phy_find_device);

/**
 * acpi_phy_attach - Attach to a PHY without starting the state machine
 * @dev: pointer to net_device claiming the phy
 * @phy_fwnode: framework Node pointer for the PHY
 * @flags: flags to pass to the PHY
 * @iface: PHY data interface type
 *
 * If successful, returns a pointer to the phy_device with the embedded
 * struct device refcount incremented by one, or NULL on failure. The
 * refcount must be dropped by calling phy_disconnect() or phy_detach().
 */
struct phy_device *acpi_phy_attach(struct net_device *dev,
				   struct fwnode_handle *phy_fwnode, u32 flags,
				   phy_interface_t iface)
{
	struct phy_device *phy = acpi_phy_find_device(phy_fwnode);
	int ret;

	if (!phy)
		return NULL;

	ret = phy_attach_direct(dev, phy, flags, iface);

	/* refcount is held by phy_attach_direct() on success */
	put_device(&phy->dev);

	return ret ? NULL : phy;
}
EXPORT_SYMBOL(acpi_phy_attach);

/**
 * acpi_phy_connect - Connect to the phy described
 * @dev: pointer to net_device claiming the phy
 * @phy_fwnode: Pointer to framework node for the PHY
 * @hndlr: Link state callback for the network device
 * @iface: PHY data interface type
 *
 * If successful, returns a pointer to the phy_device with the embedded
 * struct device refcount incremented by one, or NULL on failure. The
 * refcount must be dropped by calling phy_disconnect() or phy_detach().
 */
struct phy_device *acpi_phy_connect(struct net_device *dev,
				    struct fwnode_handle *phy_fwnode,
				    void (*hndlr)(struct net_device *),
				    u32 flags,
				    phy_interface_t iface)
{
	struct phy_device *phy = acpi_phy_find_device(phy_fwnode);
	int ret;

	if (!phy)
		return NULL;

	phy->dev_flags = flags;

	ret = phy_connect_direct(dev, phy, hndlr, iface);

	/* refcount is held by phy_connect_direct() on success */
	put_device(&phy->dev);

	return ret ? NULL : phy;
}
EXPORT_SYMBOL(acpi_phy_connect);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huawei Tech. Co., Ltd.");
