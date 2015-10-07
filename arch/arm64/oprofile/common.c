/**
 * @file common.c
 *
 * @remark Copyright 2004 Oprofile Authors
 * @remark Copyright 2010 ARM Ltd.
 * @remark Read the file COPYING
 *
 * @author Zwane Mwaikambo
 * @author Will Deacon [move to perf]
 */

#include <linux/cpumask.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/oprofile.h>
#include <linux/perf_event.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <asm/stacktrace.h>
#include <linux/uaccess.h>

#include <asm/perf_event.h>
#include <asm/ptrace.h>

#ifdef CONFIG_HW_PERF_EVENTS

char *op_name_from_perf_id(void)
{
	return perf_pmu_name();
}
#endif

static int report_trace(struct stackframe *frame, void *d)
{
	unsigned int *depth = d;

	if (*depth) {
		oprofile_add_trace(frame->pc);
		(*depth)--;
	}

	return *depth == 0;
}

/*
 * The registers we're interested in are at the end of the variable
 * length saved register structure. The fp points at the end of this
 * structure so the address of this struct is:
 * (struct frame_tail *)(xxx->fp)-1
 */
struct frame_tail {
	struct frame_tail *fp;
	unsigned long sp;
	unsigned long lr;
} __attribute__((packed));

static struct frame_tail *user_backtrace(struct frame_tail *tail)
{
	struct frame_tail buftail[2];

	/* Also check accessibility of one struct frame_tail beyond */
	if (!access_ok(VERIFY_READ, tail, sizeof(buftail)))
		return NULL;
	if (__copy_from_user_inatomic(buftail, tail, sizeof(buftail)))
		return NULL;

	oprofile_add_trace(buftail[0].lr);

	/* frame pointers should strictly progress back up the stack
	 * (towards higher addresses) */
	if (tail + 1 >= buftail[0].fp)
		return NULL;

	return buftail[0].fp-1;
}

static void arm_backtrace(struct pt_regs * const regs, unsigned int depth)
{
	struct frame_tail *tail = ((struct frame_tail *) regs->regs[29]) - 1;

	if (!user_mode(regs)) {
		struct stackframe frame;
		frame.fp = regs->regs[29];
		frame.sp = regs->sp;
		frame.lr = regs->regs[30];
		frame.pc = regs->pc;
		walk_stackframe(&frame, report_trace, &depth);
		return;
	}

	while (depth-- && tail && !((unsigned long) tail & 3))
		tail = user_backtrace(tail);
}

int __init oprofile_arch_init(struct oprofile_operations *ops)
{
	/* provide backtrace support also in timer mode: */
	ops->backtrace		= arm_backtrace;

	return oprofile_perf_init(ops);
}

void oprofile_arch_exit(void)
{
	oprofile_perf_exit();
}

