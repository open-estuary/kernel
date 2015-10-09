/*
 * Copyright (C) 2015, Linaro Ltd.
 *	Author: Tomasz Nowicki <tomasz.nowicki@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place - Suite 330, Boston, MA 02111-1307 USA.
 */

#ifndef __IORT_H__
#define __IORT_H__

#include <linux/acpi.h>

struct pci_dev;

#ifdef CONFIG_ACPI

struct fwnode_handle;

int iort_register_domain_token(int trans_id, struct fwnode_handle *fw_node);
struct fwnode_handle *iort_find_its_domain_token(int trans_id);
struct fwnode_handle *iort_find_pci_domain_token(struct device *dev);
int iort_find_pci_id(struct pci_dev *pdev, u32 req_id, u32 *dev_id);
#else /* CONFIG_ACPI */
static inline int
iort_find_pci_id(struct pci_dev *pdev, u32 req_id, u32 *dev_id)
{ return -ENXIO; }
#endif /* CONFIG_ACPI */

#endif /* __IORT_H__ */
