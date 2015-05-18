/*
 * arch/arm64/kernel/kprobes.h
 *
 * Copyright (C) 2013 Linaro Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifndef _ARM_KERNEL_KPROBES_H
#define _ARM_KERNEL_KPROBES_H

/* BRK opcodes with ESR encoding  */
#define BRK64_ESR_MASK		0xFFFF
#define BRK64_ESR_KPROBES	0x0004
#define BRK64_OPCODE_KPROBES	0xD4200080	/* "brk 0x4" */
#define ARCH64_NOP_OPCODE	0xD503201F

#define JPROBES_MAGIC_NUM	0xa5a5a5a5a5a5a5a5

/* Move this out to appropriate header file */
int fixup_exception(struct pt_regs *regs);

#endif /* _ARM_KERNEL_KPROBES_H */
