/*
 * Synopsys DesignWare 8250 driver.
 *
 * Copyright 2011 Picochip, Jamie Iles.
 * Copyright 2013 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Synopsys DesignWare 8250 has an extra feature whereby it detects if the
 * LCR is written whilst busy.  If it is, then a busy detect interrupt is
 * raised, the LCR needs to be rewritten and the uart status register read.
 */
#include <asm/byteorder.h>
#include <linux/acpi.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include <linux/serial_8250.h>
#include <linux/serial_core.h>
#include <linux/serial_reg.h>
#include <linux/slab.h>
#include "8250.h"
#include "8250_hisi.h"
#include <linux/delay.h>
#include <linux/console.h>
#include "../../../misc/hisi-lpc/hs_lpc_pltfm.h"

/*
 * This is earlycon driver of hisi low pin count uart
*/
#define EARLY_LPC_CMD_SAMEADDR_SING		(0x00000008)
#define EARLY_LPC_CMD_SAMEADDR_INC		(0x00000000)
#define EARLY_LPC_CMD_TYPE_IO			(0x00000000)
#define EARLY_LPC_CMD_TYPE_MEM		(0x00000002)
#define EARLY_LPC_CMD_TYPE_FWH		(0x00000004)
#define EARLY_LPC_CMD_WRITE			(0x00000001)
#define EARLY_LPC_CMD_READ			(0x00000000)

#define EARLY_LPC_REG_START			(0x00)
#define EARLY_LPC_REG_OP_STA			(0x04)
#define EARLY_LPC_REG_IQR_ST			(0x08)
#define EARLY_LPC_REG_OP_LEN			(0x10)
#define EARLY_LPC_REG_CMD			(0x14)
#define EARLY_LPC_REG_ADDR			(0x20)
#define EARLY_LPC_REG_WDATA			(0x24)
#define EARLY_LPC_REG_RDATA			(0x28)

unsigned int __init hisi_early_lpc_out(unsigned char __iomem *lpc_base,
				       u8 data, unsigned long addr)
{
	unsigned int lpc_cmd_value;
	unsigned int retry = 0;

	writel(0x00000002, lpc_base + EARLY_LPC_REG_IQR_ST);
	retry = 0;
	while (0 == (readl(lpc_base + EARLY_LPC_REG_OP_STA) & 0x00000001)) {
		udelay(1);
		retry++;
		if (retry >= 10000)
			return -ETIME;
	}

	/* set lpc master write cycle type and slv access mode */
	lpc_cmd_value = EARLY_LPC_CMD_WRITE | EARLY_LPC_CMD_TYPE_IO |
			EARLY_LPC_CMD_SAMEADDR_SING;
	writel(lpc_cmd_value, lpc_base + HS_LPC_REG_CMD);
	/* set lpc op len */
	writel(1, lpc_base + EARLY_LPC_REG_OP_LEN);
	/* write 1 byte */
	writel(data, lpc_base + EARLY_LPC_REG_WDATA);
	/* set lpc addr config */
	writel(addr, lpc_base + EARLY_LPC_REG_ADDR);
	/* set lpc start work */
	writel(0x01, lpc_base + EARLY_LPC_REG_START);
	retry = 0;
	while (0 == (readl(lpc_base + EARLY_LPC_REG_IQR_ST) & 0x00000002)) {
		udelay(1);
		retry++;
		if (retry >= 10000)
			return -ETIME;
	}

	writel(0x00000002, lpc_base + EARLY_LPC_REG_IQR_ST);

	if (1 == ((readl(lpc_base + EARLY_LPC_REG_OP_STA) & 0x00000002) >> 1))
		return 0;

	/*dev_err(lpc_dev->dev, "Lpc master write failed, op_result : 0x%x\n",
	  *	lpc_op_state_value);
	  */
	return -1;
}

u8 __init hisi_early_lpc_in(unsigned char __iomem *lpc_base, unsigned int addr)
{
	unsigned int retry = 0;
	unsigned int lpc_cmd_value;

	writel(0x00000002, lpc_base + EARLY_LPC_REG_IQR_ST);

	retry = 0;
	while (0 == (readl(lpc_base + EARLY_LPC_REG_OP_STA) & 0x00000001)) {
		udelay(1);
		retry++;
		if (retry >= 10000)
			return -ETIME;
	}

	/* set lpc master read, cycle type and slv access mode */
	lpc_cmd_value = EARLY_LPC_CMD_READ | EARLY_LPC_CMD_TYPE_IO |
			EARLY_LPC_CMD_SAMEADDR_SING;
	writel(lpc_cmd_value, lpc_base + EARLY_LPC_REG_CMD);
	/* set lpc op len */
	writel(1, lpc_base + EARLY_LPC_REG_OP_LEN);
	/* set lpc addr config */
	writel(addr, lpc_base + EARLY_LPC_REG_ADDR);
	/* set lpc start work */
	writel(0x01, lpc_base + EARLY_LPC_REG_START);

	while (0 == (readl(lpc_base + EARLY_LPC_REG_IQR_ST) & 0x00000002)) {
		udelay(1);
		retry++;
		if (retry >= 10000)
			/*dev_err(lpc_dev->dev, "lpc read, time out\n");*/
			return -ETIME;
	}

	writel(0x00000002, lpc_base + EARLY_LPC_REG_IQR_ST);

	/* Get read data */
	if (1 == (readl(lpc_base + EARLY_LPC_REG_OP_STA) & 0x00000002) >> 1)
		return readb(lpc_base + EARLY_LPC_REG_RDATA);

	/*dev_err(lpc_dev->dev, "Lpc master read failed, op_result : 0x%x\n",
	 *	lpc_op_state_value);
	 */
	return 0;
}

u8 __weak __init hisilpc_8250_early_in(struct uart_port *port, int offset)
{
	return hisi_early_lpc_in(port->membase, port->iobase + offset);
}

void __weak __init hisilpc_8250_early_out(struct uart_port *port,
					  int offset, int value)
{
	hisi_early_lpc_out(port->membase, value, port->iobase + offset);
}

#define BOTH_EMPTY (UART_LSR_TEMT | UART_LSR_THRE)

static void __init wait_for_xmitr(struct uart_port *port)
{
	unsigned int status;

	for (;;) {
		status = hisilpc_8250_early_in(port, UART_LSR);
		if ((status & BOTH_EMPTY) == BOTH_EMPTY)
			return;
		cpu_relax();
	}
}

static void __init serial_putc(struct uart_port *port, int c)
{
	wait_for_xmitr(port);
	hisilpc_8250_early_out(port, UART_TX, c);
}

static void __init early_hisilpc_8250_write(struct console *console,
					    const char *s, unsigned int count)
{
	struct earlycon_device *device = console->data;
	struct uart_port *port = &device->port;
	unsigned int ier;

	/* Save the IER and disable interrupts preserving the UUE bit */
	ier = hisilpc_8250_early_in(port, UART_IER);
	if (ier)
		hisilpc_8250_early_out(port, UART_IER, ier & UART_IER_UUE);

	uart_console_write(port, s, count, serial_putc);

	/* Wait for transmitter to become empty and restore the IER */
	wait_for_xmitr(port);

	if (ier)
		hisilpc_8250_early_out(port, UART_IER, ier);
}

static void __init init_port(struct earlycon_device *device)
{
	struct uart_port *port = &device->port;
	unsigned int divisor;
	unsigned char c;
	unsigned int ier;

	hisilpc_8250_early_out(port, UART_LCR, 0x3);	/* 8n1 */
	ier = hisilpc_8250_early_in(port, UART_IER);
	hisilpc_8250_early_out(port, UART_IER, ier & UART_IER_UUE); /* no int */
	hisilpc_8250_early_out(port, UART_FCR, 0);	/* no fifo */
	hisilpc_8250_early_out(port, UART_MCR, 0x3);	/* DTR + RTS */

	divisor = DIV_ROUND_CLOSEST(port->uartclk, 16 * device->baud);
	c = hisilpc_8250_early_in(port, UART_LCR);
	hisilpc_8250_early_out(port, UART_LCR, c | UART_LCR_DLAB);
	hisilpc_8250_early_out(port, UART_DLL, divisor & 0xff);
	hisilpc_8250_early_out(port, UART_DLM, (divisor >> 8) & 0xff);
	hisilpc_8250_early_out(port, UART_LCR, c & ~UART_LCR_DLAB);
}

static int __init early_hisilpc8250_setup(struct earlycon_device *device,
					  const char *options)
{
	char *p;
	int ret;

	if (!(device->port.membase || device->port.iobase))
		return -ENODEV;

	if (device->options) {
		ret = kstrtoul(device->options, 0,
			       (unsigned long *)&device->baud);
		/*wed asume that baudrate must specified,
		*value 0 means uart have been configed
		*/
		p = strchr(device->options, ',');
		if (p)
			p++;

		else
			return -EINVAL;

		ret = kstrtoul(p, 0, (unsigned long *)&device->port.iobase);
		if (ret)
			return ret;
	} else {
		device->port.iobase = 0x2f8;
		device->baud = 0;
	}

	if (device->port.iobase == 0)
		return -EFAULT;

	if (!device->baud) {
		struct uart_port *port = &device->port;
		unsigned int ier;

		/* assume the device was initialized, only mask interrupts */
		ier = hisilpc_8250_early_in(port, UART_IER);
		hisilpc_8250_early_out(port, UART_IER, ier & UART_IER_UUE);
	} else {
		init_port(device);
	}

	device->con->write = early_hisilpc_8250_write;
	return 0;
}

EARLYCON_DECLARE(hisilpcuart, early_hisilpc8250_setup);

/*
 * below is normal uart driver.
*/
static void hs_uart_serial_out(struct uart_port *p, int offset, int value)
{
	lpc_io_write_byte(value, (unsigned long)p->membase +
			  (offset << p->regshift));
}

static unsigned int hs_uart_serial_in(struct uart_port *p, int offset)
{
	unsigned char value;

	value = lpc_io_read_byte((unsigned long)p->membase +
				 (offset << p->regshift));
	return value;
}

static int hs_uart_probe(struct platform_device *pdev)
{
	struct uart_8250_port uart = {};
	struct resource *regs;
	struct hs_uart_data *data;
	unsigned int val = 0;

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "Device OF-Node is NULL\n");
		return -EFAULT;
	}
	spin_lock_init(&uart.port.lock);
	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	uart.port.mapbase = 0;

	uart.port.membase = (unsigned char __iomem *)regs->start;
	uart.port.type = PORT_16550A;
	uart.port.flags = UPF_SHARE_IRQ | UPF_BOOT_AUTOCONF | UPF_FIXED_PORT |
		UPF_FIXED_TYPE;
	uart.port.dev = &pdev->dev;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->clk = devm_clk_get(&pdev->dev, "clk");
	if (IS_ERR(data->clk)) {
		dev_err(&pdev->dev, "unable to get clock\n");
		return PTR_ERR(data->clk);
	}

	clk_prepare_enable(data->clk);
	uart.port.uartclk = clk_get_rate(data->clk);

	uart.port.iotype = UPIO_MEM;
	uart.port.serial_in = hs_uart_serial_in;
	uart.port.serial_out = hs_uart_serial_out;
	uart.port.private_data = data;

	if (!of_property_read_u32(pdev->dev.of_node, "reg-shift", &val))
		uart.port.regshift = val;

	data->line = serial8250_register_8250_port(&uart);
	if (data->line < 0) {
		dev_err(&pdev->dev, "unable to register 8250 port\n");
		clk_disable_unprepare(data->clk);
		return data->line;
	}

	platform_set_drvdata(pdev, data);
	return 0;
}

static int hs_uart_remove(struct platform_device *pdev)
{
	struct hs_uart_data *data = platform_get_drvdata(pdev);

	serial8250_unregister_port(data->line);
	clk_disable_unprepare(data->clk);
	return 0;
}

static const struct of_device_id hs_uart_pltfm_match[] = {
	{
		.compatible = "hisilicon,16550-lpc-uart",
	},
	{},
};

static struct platform_driver hs_uart_driver = {
	.driver = {
		.name = "hs-uart",
		.owner = THIS_MODULE,
		.of_match_table	= hs_uart_pltfm_match,
	},
	.probe = hs_uart_probe,
	.remove = hs_uart_remove,
};

module_platform_driver(hs_uart_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huawei Tech. Co., Ltd.");
MODULE_DESCRIPTION("Hisi UART serial port driver over LPC");
MODULE_VERSION("v1.0");
