/*
 * Copyright (C) 2015 Hisilicon Limited, All Rights Reserved.
 * Author: Jun Ma <majun258@huawei.com>
 * Author: Yun Wu <wuyun.wu@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/interrupt.h>
#include <linux/irqchip.h>
#include <linux/module.h>
#include <linux/msi.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

/**
 * In mbigen vector register
 * bit[31:16]:	device id
 * bit[15:0]:	start value of event ID
 */
#define IRQ_EVENT_ID_SHIFT		16
#define IRQ_EVENT_ID_MASK		0xffff

#define IRQS_PER_NORMAL_MBIGEN_NODE	64
/* offset of vector register in mbigen node */
#define REG_MBIGEN_VEC_OFFSET		0x300
#define REG_MBIGEN_EXT_VEC_OFFSET		0x320

/**
 * offset of clear register in mbigen node
 * This register is used to clear the status
 * of interrupt
 */
#define REG_MBIGEN_CLEAR_OFFSET		0x100
#define REG_MBIGEN_EXT_CLEAR_OFFSET		0x120

/**
 * offset of interrupt type register
 * This register is used to configure interrupt
 * trigger type
 */
#define REG_MBIGEN_TYPE_OFFSET		0x0
#define REG_MBIGEN_EXT_TYPE_OFFSET		0x20

/**
 * to avoid messing up with the interrupt from 1610 in ITS driver,
 * all of the hwirq number in pv660 add the offset 2048
 */
#define HW_IRQ_OFFSET	2048

/**
 * struct mbigen_device - holds the information of mbigen device.
 *
 * @pdev:		pointer to the platform device structure of mbigen chip.
 * @base:		mapped address of this mbigen chip.
 * @domain:		pointer to the irq domain
 * @dev_id:		device id of this mbigen device.
 */
struct mbigen_device {
	struct platform_device	*pdev;
	void __iomem		*base;
	struct irq_domain	*domain;
	unsigned int		dev_id;
};

/**
 * struct mbigen_irq_data - private data of each irq
 *
 * @base:		mapped address of mbigen chip
 * @pin_offset:		local pin offset of interrupt.
 * @reg_vec:		addr offset of interrupt vector register.
 * @reg_type:		addr offset of interrupt trigger type register.
 * @reg_clear:		addr offset of interrupt clear register.
 * @type:		interrupt trigger type.
 */
struct mbigen_irq_data {
	void __iomem		*base;
	unsigned int		pin_offset;
	unsigned int		reg_vec;
	unsigned int		reg_type;
	unsigned int		reg_clear;
	unsigned int		type;
};

/**
 * define the event ID start value of each mbigen node
 * in a mbigen chip
 */
static int mbigen_event_base[6] = {0, 64, 128, 192, 256, 384};

static inline int get_mbigen_vec_reg(u32 nid, u32 offset)
{
	if (nid < 4)
		return (nid * 4) + REG_MBIGEN_VEC_OFFSET;
	else
		return (nid - 4) * 4 + REG_MBIGEN_EXT_VEC_OFFSET;
}

static int get_mbigen_type_reg(u32 nid, u32 offset)
{
	int ofst;

	if (nid < 4) {
		ofst = offset / 32 * 4;
		return ofst + REG_MBIGEN_TYPE_OFFSET;
	} else {
		offset -= 4 * IRQS_PER_NORMAL_MBIGEN_NODE;
		ofst = offset / 32 * 4;
		return ofst + REG_MBIGEN_EXT_TYPE_OFFSET;
	}
}

static int get_mbigen_clear_reg(u32 nid, u32 offset)
{
	int ofst;

	if (nid < 4) {
		ofst = offset / 32 * 4;
		return ofst + REG_MBIGEN_CLEAR_OFFSET;
	} else {
		offset -= 4 * IRQS_PER_NORMAL_MBIGEN_NODE;
		ofst = offset / 32 * 4;
		return ofst + REG_MBIGEN_EXT_CLEAR_OFFSET;
	}
}


static void mbigen_ack_irq(struct irq_data *data)
{
	struct mbigen_irq_data *mgn_irq_data = irq_data_get_irq_chip_data(data);
	u32 mask;

	/* only level triggered interrupt need to clear status */
	if (mgn_irq_data->type == IRQ_TYPE_LEVEL_MASK) {
		mask = 1 << (mgn_irq_data->pin_offset % 32);
		writel_relaxed(mask, mgn_irq_data->reg_clear + mgn_irq_data->base);
	}
}


static void mbigen_eoi_irq(struct irq_data *data)
{
	struct mbigen_irq_data *mgn_irq_data = irq_data_get_irq_chip_data(data);
	u32 mask;

	/* only level triggered interrupt need to clear status */
	if (mgn_irq_data->type == IRQ_TYPE_LEVEL_MASK) {
		mask = 1 << (mgn_irq_data->pin_offset % 32);
		writel_relaxed(mask, mgn_irq_data->reg_clear + mgn_irq_data->base);
	}

	irq_chip_eoi_parent(data);
}

static int mbigen_set_type(struct irq_data *d, unsigned int type)
{
	struct mbigen_irq_data *mgn_irq_data = irq_data_get_irq_chip_data(d);
	u32 mask;
	int val;

	if (type != IRQ_TYPE_LEVEL_HIGH && type != IRQ_TYPE_EDGE_RISING)
		return -EINVAL;

	mask = 1 << (mgn_irq_data->pin_offset % 32);

	val = readl_relaxed(mgn_irq_data->reg_type + mgn_irq_data->base);

	if (type == IRQ_TYPE_LEVEL_HIGH)
		val |= mask;
	else
		val &= ~mask;

	writel_relaxed(val, mgn_irq_data->reg_type + mgn_irq_data->base);

	return 0;
}

void mbigen_unmask_irq(struct irq_data *data)
{
	mbigen_ack_irq(data);
	irq_chip_unmask_parent(data);
}


static struct irq_chip mbigen_irq_chip = {
	.name =			"mbigen-v1",
	.irq_mask =		irq_chip_mask_parent,
	.irq_unmask =		irq_chip_unmask_parent,
	.irq_eoi =		mbigen_eoi_irq,
	.irq_set_type =		mbigen_set_type,
	.irq_set_affinity =	irq_chip_set_affinity_parent,
};

static int get_mbigen_nid(unsigned int offset)
{
	int nid = 0;

	if (offset < 256)
		nid = offset / 64;
	else if (offset < 384)
		nid = 4;
	else if (offset < 640)
		nid = 5;

	return nid;
}

static void mbigen_write_msg(struct msi_desc *desc, struct msi_msg *msg)
{
	struct mbigen_irq_data *mgn_irq_data = irq_get_chip_data(desc->irq);
	struct mbigen_device *mgn_chip;
	struct irq_data *data;
	u32 newval, oldval, nid;

	data = irq_get_irq_data(desc->irq);
	mgn_chip = platform_msi_get_host_data(data->domain);

	nid = get_mbigen_nid(data->hwirq - HW_IRQ_OFFSET);

	newval = (mgn_chip->dev_id << IRQ_EVENT_ID_SHIFT) | mbigen_event_base[nid];
	oldval = readl_relaxed(mgn_irq_data->reg_vec + mgn_irq_data->base);

	if (newval != oldval)
		writel_relaxed(newval, mgn_irq_data->reg_vec + mgn_irq_data->base);
}

static struct mbigen_irq_data *set_mbigen_irq_data(int hwirq,
						unsigned int type)
{
	struct mbigen_irq_data *datap;
	unsigned int nid;

	datap = kzalloc(sizeof(*datap), GFP_KERNEL);
	if (!datap)
		return NULL;

	datap->pin_offset = hwirq - HW_IRQ_OFFSET;

	/* get the mbigen node number */
	nid = get_mbigen_nid(datap->pin_offset);

	datap->reg_vec = get_mbigen_vec_reg(nid, datap->pin_offset);
	datap->reg_type = get_mbigen_type_reg(nid, datap->pin_offset);

	/* no clear register for edge triggered interrupt */
	if (type == IRQ_TYPE_EDGE_RISING)
		datap->reg_clear = 0;
	else
		datap->reg_clear = get_mbigen_clear_reg(nid,
					datap->pin_offset);

	datap->type = type;
	return datap;
}

static int mbigen_domain_translate(struct irq_domain *d,
				    struct irq_fwspec *fwspec,
				    unsigned long *hwirq,
				    unsigned int *type)
{
	if (is_of_node(fwspec->fwnode)) {
		if (fwspec->param_count != 2)
			return -EINVAL;

		*hwirq = fwspec->param[0] + HW_IRQ_OFFSET;
		*type = fwspec->param[1] & IRQ_TYPE_SENSE_MASK;

		fwspec->param[0] = *hwirq;

		return 0;
	}
	return -EINVAL;
}

static int mbigen_irq_domain_alloc(struct irq_domain *domain,
					unsigned int virq,
					unsigned int nr_irqs,
					void *args)
{
	struct irq_fwspec *fwspec = args;
	irq_hw_number_t hwirq = fwspec->param[0];
	unsigned int type = fwspec->param[1];
	struct mbigen_device *mgn_chip;
	struct mbigen_irq_data *mgn_irq_data;
	int i, err = 0;

	/* set related information of this irq */
	mgn_irq_data = set_mbigen_irq_data(hwirq, type);
	if (!mgn_irq_data)
		return err;

	mgn_chip = platform_msi_get_host_data(domain);
	mgn_irq_data->base = mgn_chip->base;

	/*
	 * to get the irq_data->hwirq in its driver, i moved this part
	 * in front of alloc function
	 */
	for (i = 0; i < nr_irqs; i++)
		irq_domain_set_hwirq_and_chip(domain, virq + i, hwirq + i,
				      &mbigen_irq_chip, mgn_irq_data);

	err = platform_msi_domain_alloc(domain, virq, nr_irqs);
	if (err)
		return err;

	return 0;
}

static void mbigen_domain_free(struct irq_domain *domain, unsigned int virq,
			       unsigned int nr_irqs)
{
	struct irq_data *d = irq_domain_get_irq_data(domain, virq);
	struct mbigen_irq_data *mgn_irq_data = irq_data_get_irq_chip_data(d);

	kfree(mgn_irq_data);
	irq_domain_free_irqs_common(domain, virq, nr_irqs);
}

static struct irq_domain_ops mbigen_domain_ops = {
	.translate	= mbigen_domain_translate,
	.alloc		= mbigen_irq_domain_alloc,
	.free		= mbigen_domain_free,
};

static int mbigen_device_probe(struct platform_device *pdev)
{
	struct mbigen_device *mgn_chip;
	struct irq_domain *domain;
	u32 num_msis, dev_id;

	mgn_chip = devm_kzalloc(&pdev->dev, sizeof(*mgn_chip), GFP_KERNEL);
	if (!mgn_chip)
		return -ENOMEM;

	mgn_chip->pdev = pdev;
	mgn_chip->base = of_iomap(pdev->dev.of_node, 0);

	/* If there is no "num-msi" property, assume 64... */
	if (of_property_read_u32(pdev->dev.of_node, "num-msis", &num_msis) < 0)
		num_msis = 64;

	if (of_property_read_u32_index(pdev->dev.of_node, "msi-parent",
					 1, &dev_id))
		return -EINVAL;

	mgn_chip->dev_id = dev_id;

	domain = platform_msi_create_device_domain(&pdev->dev, num_msis,
							mbigen_write_msg,
							&mbigen_domain_ops,
							mgn_chip);

	if (!domain)
		return -ENOMEM;

	mgn_chip->domain = domain;

	platform_set_drvdata(pdev, mgn_chip);

	return 0;
}

static int mbigen_device_remove(struct platform_device *pdev)
{
	struct mbigen_device *mgn_chip = platform_get_drvdata(pdev);

	irq_domain_remove(mgn_chip->domain);
	iounmap(mgn_chip->base);

	return 0;
}

static const struct of_device_id mbigen_of_match[] = {
	{ .compatible = "hisilicon,mbigen-v1" },
	{ /* END */ }
};
MODULE_DEVICE_TABLE(of, mbigen_of_match);

static struct platform_driver mbigen_platform_driver = {
	.driver = {
		.name		= "Hisilicon MBIGEN-V1",
		.owner		= THIS_MODULE,
		.of_match_table	= mbigen_of_match,
	},
	.probe			= mbigen_device_probe,
	.remove			= mbigen_device_remove,
};

static int __init mbigen_device_init(void)
{
	return platform_driver_register(&mbigen_platform_driver);
}
arch_initcall(mbigen_device_init)

MODULE_AUTHOR("Jun Ma <majun258@huawei.com>");
MODULE_AUTHOR("Yun Wu <wuyun.wu@huawei.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Hisilicon MBI Generator driver");
