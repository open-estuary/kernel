#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/delay.h>

#include "hs_lpc_pltfm.h"

#define LPC_REG_READ(reg, result) ((result) = readl(reg))

#define LPC_REG_WRITE(reg, data)   writel((data), (reg))

struct hs_lpc_dev *lpc_dev;

int lpc_master_write(unsigned int slv_access_mode, unsigned int cycle_type,
		     unsigned int addr, unsigned char *buf, unsigned int len)
{
	unsigned int i;
	unsigned int lpc_cmd_value;
	unsigned int lpc_irq_st_value;
	unsigned int lpc_op_state_value;
	unsigned int retry = 0;

	/* para check */
	if (!buf)
		return -EFAULT;

	if (slv_access_mode != HS_LPC_CMD_SAMEADDR_SING &&
	    slv_access_mode != HS_LPC_CMD_SAMEADDR_INC) {
		dev_err(lpc_dev->dev, "Slv access mode error\n");
		return -EINVAL;
	}

	if ((cycle_type != HS_LPC_CMD_TYPE_IO) &&
	    (cycle_type != HS_LPC_CMD_TYPE_MEM) &&
	    (cycle_type != HS_LPC_CMD_TYPE_FWH)) {
		dev_err(lpc_dev->dev, "Cycle type error\n");
		return -EINVAL;
	}

	if (len == 0) {
		dev_err(lpc_dev->dev, "Write length zero\n");
		return -EINVAL;
	}

	LPC_REG_WRITE(lpc_dev->regs + HS_LPC_REG_IQR_ST, 0x00000002);
	retry = 0;
	while (0 == (LPC_REG_READ(lpc_dev->regs + HS_LPC_REG_OP_STATUS,
				  lpc_op_state_value) & 0x00000001)) {
		udelay(1);
		retry++;
		if (retry >= 300) {
			dev_err(lpc_dev->dev, "lpc W, wait idle time out\n");
			return -ETIME;
		}
	}

	/* set lpc master write cycle type and slv access mode */
	lpc_cmd_value = HS_LPC_CMD_WRITE | cycle_type | slv_access_mode;
	LPC_REG_WRITE(lpc_dev->regs + HS_LPC_REG_CMD, lpc_cmd_value);

	/* set lpc op len */
	LPC_REG_WRITE(lpc_dev->regs + HS_LPC_REG_OP_LEN, len);

	/* Set write data */
	for (i = 0; i < len; i++)
		LPC_REG_WRITE(lpc_dev->regs + HS_LPC_REG_WDATA, buf[i]);

	/* set lpc addr config */
	LPC_REG_WRITE(lpc_dev->regs + HS_LPC_REG_ADDR, addr);

	/* set lpc start work */
	LPC_REG_WRITE(lpc_dev->regs + HS_LPC_REG_START, 0x01);

	retry = 0;
	while (0 == (LPC_REG_READ(lpc_dev->regs + HS_LPC_REG_IQR_ST,
				  lpc_irq_st_value) & 0x00000002)) {
		udelay(1);
		retry++;
		if (retry >= 100000) {
			dev_err(lpc_dev->dev, "lpc write, time out\n");
			return -ETIME;
		}
	}

	LPC_REG_WRITE(lpc_dev->regs + HS_LPC_REG_IQR_ST, 0x00000002);

	if (1 == (LPC_REG_READ(lpc_dev->regs + HS_LPC_REG_OP_STATUS,
			       lpc_op_state_value) & 0x00000002) >> 1) {
		return 0;
	}

	dev_err(lpc_dev->dev, "Lpc master write failed, op_result : 0x%x\n",
		lpc_op_state_value);
	return -1;
}

void  lpc_io_write_byte(u8 value, unsigned long addr)
{
	unsigned long flags;
	int ret;

	if (!lpc_dev) {
		return;
	}
	spin_lock_irqsave(&lpc_dev->lock, flags);
	ret = lpc_master_write(HS_LPC_CMD_SAMEADDR_SING, HS_LPC_CMD_TYPE_IO,
			       addr, &value, 1);
	spin_unlock_irqrestore(&lpc_dev->lock, flags);
}
EXPORT_SYMBOL(lpc_io_write_byte);

int lpc_master_read(unsigned int slv_access_mode, unsigned int cycle_type,
		    unsigned int addr, unsigned char *buf, unsigned int len)
{
	unsigned int i;
	unsigned int lpc_cmd_value;
	unsigned int lpc_irq_st_value;
	unsigned int lpc_rdata_value;
	unsigned int lpc_op_state_value;
	unsigned int retry = 0;

	/* para check */
	if (!buf)
		return -EFAULT;

	if (slv_access_mode != HS_LPC_CMD_SAMEADDR_SING &&
	    slv_access_mode != HS_LPC_CMD_SAMEADDR_INC) {
		dev_err(lpc_dev->dev, "Slv access mode error\n");
		return -EINVAL;
	}

	if (cycle_type != HS_LPC_CMD_TYPE_IO &&
	    cycle_type != HS_LPC_CMD_TYPE_MEM &&
	    cycle_type != HS_LPC_CMD_TYPE_FWH) {
		dev_err(lpc_dev->dev, "Cycle type error\n");
		return -EINVAL;
	}

	LPC_REG_WRITE(lpc_dev->regs + HS_LPC_REG_IQR_ST, 0x00000002);

	retry = 0;
	while (0 == (LPC_REG_READ(lpc_dev->regs + HS_LPC_REG_OP_STATUS,
				  lpc_op_state_value) & 0x00000001)) {
		udelay(1);
		retry++;
		if (retry >= 300) {
			dev_err(lpc_dev->dev, "lpc read,wait idl timeout\n");
			return -ETIME;
		}
	}

	/* set lpc master read, cycle type and slv access mode */
	lpc_cmd_value = HS_LPC_CMD_READ | cycle_type | slv_access_mode;
	LPC_REG_WRITE(lpc_dev->regs + HS_LPC_REG_CMD, lpc_cmd_value);

	/* set lpc op len */
	LPC_REG_WRITE(lpc_dev->regs + HS_LPC_REG_OP_LEN, len);

	/* set lpc addr config */
	LPC_REG_WRITE(lpc_dev->regs + HS_LPC_REG_ADDR, addr);

	/* set lpc start work */
	LPC_REG_WRITE(lpc_dev->regs + HS_LPC_REG_START, 0x01);

	while (0 == (LPC_REG_READ(lpc_dev->regs + HS_LPC_REG_IQR_ST,
				  lpc_irq_st_value) & 0x00000002)) {
		udelay(1);
		retry++;
		if (retry >= 100000) {
			dev_err(lpc_dev->dev, "lpc read, time out\n");
			return -ETIME;
		}
	}

	LPC_REG_WRITE(lpc_dev->regs + HS_LPC_REG_IQR_ST, 0x00000002);

	/* Get read data */
	if (1 == (LPC_REG_READ(lpc_dev->regs + HS_LPC_REG_OP_STATUS,
			       lpc_op_state_value) & 0x00000002) >> 1) {
		for (i = 0; i < len; i++) {
			LPC_REG_READ(lpc_dev->regs + HS_LPC_REG_RDATA,
				     lpc_rdata_value);
			buf[i] = lpc_rdata_value;
		}
		return 0;
	}

	dev_err(lpc_dev->dev, "Lpc master read failed, op_result : 0x%x\n",
		lpc_op_state_value);

	return -1;
}

u8 lpc_io_read_byte(unsigned long addr)
{
	unsigned char value;
	unsigned long flags;

	if (!lpc_dev) {
		return 0;
	}

	spin_lock_irqsave(&lpc_dev->lock, flags);
	lpc_master_read(HS_LPC_CMD_SAMEADDR_SING,
			HS_LPC_CMD_TYPE_IO, addr, &value, 1);
	spin_unlock_irqrestore(&lpc_dev->lock, flags);
	return value;
}
EXPORT_SYMBOL(lpc_io_read_byte);

static int hs_lpc_probe(struct platform_device *pdev)
{
	struct resource *regs = NULL;
	struct resource *regs_mux = NULL;
	void __iomem  *regs_mux_ = NULL;
	struct resource *regs_iowarp = NULL;
	void __iomem  *regs_iowarp_ = NULL;

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "Device OF-Node is NULL\n");
		return -EFAULT;
	}

	lpc_dev = devm_kzalloc(&pdev->dev,
			       sizeof(struct hs_lpc_dev), GFP_KERNEL);
	if (!lpc_dev)
		return -ENOMEM;

	spin_lock_init(&lpc_dev->lock);
	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	lpc_dev->regs = devm_ioremap_resource(&pdev->dev, regs);
	if (IS_ERR(lpc_dev->regs))
		return PTR_ERR(lpc_dev->regs);

	/*Io mux config*/
	regs_mux = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	regs_mux_ = ioremap_nocache(regs_mux->start, resource_size(regs_mux));
	if (IS_ERR(regs_mux_))
		return PTR_ERR(regs_mux_);

	LPC_REG_WRITE(regs_mux_ + REG_OFFSET_IOMG033, 0x3);
	LPC_REG_WRITE(regs_mux_ + REG_OFFSET_IOMG035, 0x3);
	LPC_REG_WRITE(regs_mux_ + REG_OFFSET_IOMG036, 0x3);
	LPC_REG_WRITE(regs_mux_ + REG_OFFSET_IOMG050, 0x3);
	LPC_REG_WRITE(regs_mux_ + REG_OFFSET_IOMG049, 0x3);
	LPC_REG_WRITE(regs_mux_ + REG_OFFSET_IOMG048, 0x3);
	LPC_REG_WRITE(regs_mux_ + REG_OFFSET_IOMG047, 0x3);
	LPC_REG_WRITE(regs_mux_ + REG_OFFSET_IOMG046, 0x3);
	LPC_REG_WRITE(regs_mux_ + REG_OFFSET_IOMG045, 0x3);
	iounmap(regs_mux_);
	udelay(1);

	dev_err(&pdev->dev, "hisi low pin count iomux config successfully\n");
	/*initialization*/
	regs_iowarp = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	regs_iowarp_ = ioremap_nocache(regs_iowarp->start,
				       resource_size(regs_iowarp));
	if (IS_ERR(regs_iowarp_))
		return PTR_ERR(regs_iowarp_);

	LPC_REG_WRITE(regs_iowarp_ + REG_OFFSET_LPC_RESET, 0xf);
	udelay(1);
	LPC_REG_WRITE(regs_iowarp_ + REG_OFFSET_LPC_BUS_RESET, 0xf);
	udelay(1);
	LPC_REG_WRITE(regs_iowarp_ + REG_OFFSET_LPC_CLK_DIS, 0xf);
	udelay(1);
	LPC_REG_WRITE(regs_iowarp_ + REG_OFFSET_LPC_BUS_CLK_DIS, 0xf);
	udelay(1);
	LPC_REG_WRITE(regs_iowarp_ + REG_OFFSET_LPC_DE_RESET, 0xf);
	udelay(1);
	LPC_REG_WRITE(regs_iowarp_ + REG_OFFSET_LPC_BUS_DE_RESET, 0xf);
	udelay(1);
	LPC_REG_WRITE(regs_iowarp_ + REG_OFFSET_LPC_CLK_EN, 0xf);
	udelay(1);
	LPC_REG_WRITE(regs_iowarp_ + REG_OFFSET_LPC_BUS_CLK_EN, 0xf);

	iounmap(regs_iowarp_);
	dev_err(&pdev->dev, "hisi low pin count initialized successfully\n");

	lpc_dev->dev = &pdev->dev;
	platform_set_drvdata(pdev, lpc_dev);

	    return 0;
}

static int hs_lpc_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id hs_lpc_pltfm_match[] = {
	{
		.compatible = "hisilicon,low-pin-count",
	},
	{},
};

static struct platform_driver hs_lpc_driver = {
	.driver = {
		.name           = "hs-lpc",
		.owner          = THIS_MODULE,
		.of_match_table = hs_lpc_pltfm_match,
	},
	.probe                = hs_lpc_probe,
	.remove               = hs_lpc_remove,
};

static int __init hs_lpc_init_driver(void)
{
	return platform_driver_register(&hs_lpc_driver);
}

static void __exit hs_lpc_init_exit(void)
{
	platform_driver_unregister(&hs_lpc_driver);
}

arch_initcall(hs_lpc_init_driver);
module_exit(hs_lpc_init_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Huawei Tech. Co., Ltd.");
MODULE_DESCRIPTION("LPC driver for linux");
MODULE_VERSION("v1.0");
