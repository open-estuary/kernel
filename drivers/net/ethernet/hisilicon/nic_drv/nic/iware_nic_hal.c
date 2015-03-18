/*--------------------------------------------------------------------------------------------------------------------------*/
/*!!Warning: This is a key information asset of Huawei Tech Co.,Ltd														 */
/*CODEMARK:kOyQZYzjDpyGdBAEC2GaWuVy7vy/wDnq7gJfHBOj2pBXFF9pJtpDLt9sw5WJiMsUkN5d7jr7
aK5J3kmlnl+vpQIjFDOn3HlYdNfd+O2Mso0Wn8OrHjkJn4/rA05FKgT2JMPJkXZ1ZyV4aAJ6
v59wffSsy5ngGEtoBQq26yjsyQfSVYZd3vIsYB0WfSPHVJz9igKa/WmjD0r9FeqXscJRa6b7
P3AfVaAWzJC9l9NRIao+FQSxfxGkoi04M4KkdJLQRrUe5nRCza02ogcfT12E7Q==*/
/*--------------------------------------------------------------------------------------------------------------------------*/
/*******************************************************************************

  Hisilicon network interface controller driver
  Copyright(c) 2014 - 2019 Huawei Corporation.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:TBD

*******************************************************************************/

/* #include "hrd_module.h"
#include "hrd_typedef.h" */
#include "iware_error.h"
#include "iware_log.h"
#include "iware_dsaf_main.h"
#include "iware_mac_main.h"
#include "iware_nic_main.h"

/**
 * nic_drv_change_mtu - change mtu
 * @nic_device: nic dev
 * @new_mtu: new mtu
 *
 * Return 0 on success, negative on failure
 */
static int nic_drv_change_mtu(struct nic_device *nic_dev, int new_mtu)
{
	int ret;
	int max_frame = new_mtu + ETH_HLEN + ETH_FCS_LEN;
	struct mac_device *mac_dev = NULL;

	log_dbg(&nic_dev->netdev->dev, "func begin\n");

	mac_dev = nic_dev->mac_dev;
	if (NULL == mac_dev || NULL == mac_dev->change_mtu) {
		log_err(&nic_dev->netdev->dev,
			"mac_dev or change_mtu func not found!\n");
		return -ENODEV;
	}

	ret = mac_dev->change_mtu(mac_dev, max_frame);
	if (ret) {
		log_err(&nic_dev->netdev->dev, "change mtu fail, ret = %#x!\n",
			ret);
		return ret;
	}
	nic_dev->mtu = max_frame;

	return 0;
}

/**
 * nic_drv_set_mac_address - set mac address
 * @nic_device: nic dev
 * @mac_addr: mac addr
 *
 * Return 0 on success, negative on failure
 */
static int nic_drv_set_mac_address(struct nic_device *nic_dev,
				   char *mac_addr)
{
	int ret;
	struct mac_device *mac_dev = NULL;

	log_dbg(&nic_dev->netdev->dev, "func begin\n");

	mac_dev = nic_dev->mac_dev;
	if (NULL == mac_dev || NULL == mac_dev->change_addr)
		return -ENODEV;

	ret = mac_dev->change_addr(mac_dev, mac_addr);
	if (ret) {
		log_err(&nic_dev->netdev->dev,
			"mac change_addr fail, ret = %#x!\n", ret);
		return ret;
	}

	return 0;
}

/**
 * nic_set_ops - set nic ops
 * @ops: nic ops
 */
void nic_set_ops(struct nic_ops *ops)
{
	ops->change_mtu = nic_drv_change_mtu;
	ops->set_mac_address = nic_drv_set_mac_address;
}
