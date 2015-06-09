/*
 * Huawei driver
 *
 * Copyright (C)
 * Author: Huawei majun258@huawei.com
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/kernel.h>
#include <asm/smc_call.h>
#include <linux/cpu.h>
#include <linux/slab.h>
#include <linux/sched.h>

/* This value defined by the SMC Call manual*/
#define SMC_FUNCTION_REGISTE_ID 0x83000000
#define SMC_CALLBACK_ID 0x83000001
#define SMC_NMI_STATE_ID 0x83000002
#define SMC_NMI_TIMEOUT_ID 0x83000003

void register_nmi_handler(uint64_t func)
{
	struct smc_param64 param = { 0 };
	uint64_t *sp;

	sp = kmalloc(128000, GFP_KERNEL);

	param.a0 = SMC_FUNCTION_REGISTE_ID;
	param.a1 = (uint64_t)func;
	param.a2 = (uint64_t)sp;

	pr_debug("%s--start[%llx][%llx][%llx]: cpu: %d\n", __func__,
		 param.a0, param.a1, param.a2, smp_processor_id());

	smc_call(param.a0, param.a1, param.a2);
}
EXPORT_SYMBOL(register_nmi_handler);


void smc_callback_func(void)
{
	struct smc_param64 param = { 0 };

	param.a0 = SMC_CALLBACK_ID;
	param.a1 = 0;
	param.a2 = 0;

	pr_debug("%s--start[%llx][%llx][%llx]:cpu: %d\n", __func__,
		 param.a0, param.a1, param.a2, smp_processor_id());

	smc_call(param.a0, param.a1, param.a2);
}
EXPORT_SYMBOL(smc_callback_func);

void nmi_set_active_state(int cpuid, bool state)
{
	struct smc_param64 param = { 0 };

	param.a0 = SMC_NMI_STATE_ID;
	param.a1 = cpuid;
	param.a2 = state;

	pr_debug("%s--start[%llx][%llx][%llx]:cpu: %d\n", __func__,
		 param.a0, param.a1, param.a2, smp_processor_id());

	smc_call(param.a0, param.a1, param.a2);
}
EXPORT_SYMBOL(nmi_set_active_state);

void nmi_set_timeout(int cpuid, int time)
{
	struct smc_param64 param = { 0 };

	param.a0 = SMC_NMI_TIMEOUT_ID;
	param.a1 = cpuid;
	param.a2 = time;

	pr_debug("%s--start[%llx][%llx][%llx]:cpu: %d\n", __func__,
		 param.a0, param.a1, param.a2, smp_processor_id());

	smc_call(param.a0, param.a1, param.a2);
}
EXPORT_SYMBOL(nmi_set_timeout);

void touch_nmi_watchdog(void)
{
	/* add your function here to feed watchdog */
	touch_softlockup_watchdog();
}
EXPORT_SYMBOL(touch_nmi_watchdog);

