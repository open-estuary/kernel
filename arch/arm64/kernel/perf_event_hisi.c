/*
 * HiSilicon SoC Hardware event counters support
 *
 * Copyright (C) 2015 Huawei Technologies Limited
 * Author: Anurup M <anurup.m@huawei.com>
 *
 * This code is based heavily on the ARMv7 perf event code.
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
#include <linux/module.h>
#include <linux/bitmap.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/mfd/syscon.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/perf_event.h>
#include <linux/slab.h>

#include <asm/atomic.h>
#include <asm/cputype.h>
#include <asm/hisi-djtag.h>
#include <asm/irq.h>
#include <asm/irq_regs.h>

#include "perf_event_hisi.h"

/* Map cfg_en values for LLC Banks */
const int llc_cfgen_map[] = { HISI_LLC_BANK0_CFGEN, HISI_LLC_BANK1_CFGEN,
				HISI_LLC_BANK1_CFGEN, HISI_LLC_BANK3_CFGEN
};

/* LLC information */
hisi_llc_data llc_data[MAX_DIE];

int gnum_llc;

/* MN information */
hisi_mn_data mn_data[MAX_DIE];

/* djtag read interface - Call djtag driver */
static int hisi_djtag_readreg(int module_id, int bank, u32 offset,
				struct device_node *djtag_node, u32 *pvalue)
{
	int ret;
	u32 chain_id = 0;

	while (bank != 1) {
		bank = (bank >> 0x1);
		chain_id++;
	}

	ret = djtag_readl(djtag_node, offset, module_id, chain_id, pvalue);
	if (ret)
		pr_info("Djtag:%s Read failed!\n", djtag_node->full_name);

	return ret;
}

/* djtag write interface */
static int hisi_djtag_writereg(int module_id, int bank, u32 offset, u32 value,
							struct device_node *djtag_node)
{
	int ret;

	ret = djtag_writel(djtag_node, offset, module_id, bank, value);
	if (ret)
		pr_info("Djtag:%s Write failed!\n", djtag_node->full_name);

	return ret;
}

u64 hisi_llc_event_update(struct perf_event *event,
				struct hw_perf_event *hwc, int idx)
{
	struct hisi_llc_hwc_data_info *phisi_hwc_data = hwc->perf_event_data;
	struct device_node *djtag_node;
	u64 delta, delta_ovflw, prev_raw_count, new_raw_count = 0;
	int cfg_en;
	u32 raw_event_code = hwc->config_base;
	u32 dieID = (raw_event_code & HISI_HW_DIE_MASK) >> 20;
	u32 llc_idx = dieID - 1;
	u32 ovfl_cnt = 0;
	int counter_idx;
	int cntr_idx = idx & ~(HISI_CNTR_DIE_MASK);
	int i, j = 0;

	if (!dieID || (HISI_HW_DIE_MAX < dieID)) {
		pr_err("Invalid DieID=%d in event code!\n", dieID);
		return 0;
	}

	if (cntr_idx >= ARMV8_HISI_IDX_LLC_COUNTER0 &&
		cntr_idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		counter_idx = cntr_idx - ARMV8_HISI_IDX_LLC_COUNTER0;
	} else {
		pr_err("Unsupported event index:%d!\n", cntr_idx);
		return 0;
	}

	/* Find the djtag device node of the Die */
	djtag_node = llc_data[llc_idx].djtag_node;

	for (i = 0; i < NUM_LLC_BANKS; i++, j++) {
		cfg_en = llc_data[llc_idx].bank[i].cfg_en;
again:
		prev_raw_count =
			local64_read(
			&phisi_hwc_data->hwc_prev_counters[j].prev_count);
		new_raw_count =
			hisi_read_llc_counter(counter_idx,
					djtag_node, cfg_en);

		if (local64_cmpxchg(
			&phisi_hwc_data->hwc_prev_counters[j].prev_count,
					prev_raw_count, new_raw_count) !=
								prev_raw_count)
			goto again;

		/* update the interrupt count also */
		ovfl_cnt =
		atomic_read(&llc_data[llc_idx].bank[i].cntr_ovflw[counter_idx]);
		if (0 != ovfl_cnt) {
			delta_ovflw =
				(0xFFFFFFFF - (u32)prev_raw_count) +
					 ((ovfl_cnt - 1) * 0xFFFFFFFF);
			delta = (delta_ovflw + new_raw_count) &
						HISI_ARMV8_MAX_PERIOD;
			/* Set the overflow count to 0 */
			atomic_set(
			&llc_data[llc_idx].bank[i].cntr_ovflw[counter_idx],
									0);
		} else {
			delta = (new_raw_count - prev_raw_count) &
						HISI_ARMV8_MAX_PERIOD;
		}
		pr_debug("LLC: delta for event:0x%x is %llu\n",
						raw_event_code, delta);
		local64_add(delta, &event->count);
	}

	local64_sub(delta, &hwc->period_left);

	return new_raw_count;
}

u64 hisi_mn_event_update(struct perf_event *event,
			struct hw_perf_event *hwc, int idx)
{
	struct hisi_mn_hwc_data_info *phisi_hwc_data = hwc->perf_event_data;
	struct device_node *djtag_node;
	u64 delta, prev_raw_count, new_raw_count = 0;
	u32 raw_event_code = hwc->config_base;
	u32 dieID = (raw_event_code & HISI_HW_DIE_MASK) >> 20;
	u32 mn_idx = dieID - 1;
	int cntr_idx = idx & ~(HISI_CNTR_DIE_MASK);
	int counter_idx;

	if (cntr_idx >= ARMV8_HISI_IDX_MN_COUNTER0 &&
		cntr_idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		counter_idx = cntr_idx - ARMV8_HISI_IDX_MN_COUNTER0;
	} else {
		pr_err("Unsupported event index:%d!\n", cntr_idx);
		return 0;
	}

	/* Find the djtag device node of the Die */
	djtag_node = mn_data[mn_idx].djtag_node;

again:
	prev_raw_count =
		local64_read(&phisi_hwc_data->event_start_count);
	new_raw_count =
		hisi_read_mn_counter(counter_idx, djtag_node,
						HISI_MN_MODULE_CFGEN);

	if (local64_cmpxchg(&phisi_hwc_data->event_start_count,
				prev_raw_count, new_raw_count) !=
							prev_raw_count)
		goto again;

	delta = (new_raw_count - prev_raw_count) & HISI_ARMV8_MAX_PERIOD;
	pr_debug("MN:delta for MN event:0x%x is %llu\n",
					raw_event_code, delta);
	local64_add(delta, &event->count);
	local64_sub(delta, &hwc->period_left);

	return new_raw_count;
}

/* Read hardware counter and update the Perf counter statistics */
u64 hisi_pmu_event_update(struct perf_event *event,
				 struct hw_perf_event *hwc, int idx){
	u64 new_raw_count = 0;
	int cntr_idx = idx & ~(HISI_CNTR_DIE_MASK);

	/* 
	 * Identify Event type and read appropriate hardware counter
	 * and sum the values
	 */
	if (ARMV8_HISI_IDX_LLC_COUNTER0 <= cntr_idx &&
			 cntr_idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		 new_raw_count = hisi_llc_event_update(event, hwc, idx);
	} else if (ARMV8_HISI_IDX_MN_COUNTER0 <= cntr_idx &&
			cntr_idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		new_raw_count = hisi_mn_event_update(event, hwc, idx);
	}

	return new_raw_count;
}

static void hisi_clear_llc_event_idx(int idx)
{
	struct device_node *djtag_node;
	void *bitmap_addr;
	int counter_idx, i;
	int cfg_en;
	u32 value;
	u32 reg_offset;
	int cntr_idx = idx & ~(HISI_CNTR_DIE_MASK);
	u32 llc_idx = (((idx & HISI_CNTR_DIE_MASK) >> 8) - 1);

	if (ARMV8_HISI_IDX_LLC_COUNTER0 <= cntr_idx &&
		 cntr_idx <=	ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		counter_idx = ARMV8_HISI_IDX_LLC_COUNTER0;
	} else {
		pr_err("Unsupported event index:%d!\n", cntr_idx);
		return;
	}

	bitmap_addr = llc_data[llc_idx].hisi_llc_event_used_mask;

	__clear_bit(cntr_idx - counter_idx, bitmap_addr);

	/* Clear Counting in LLC event config register */
	if (llc_idx) {
		if ((cntr_idx - counter_idx) <= 3)
			reg_offset = HISI_LLC_EVENT_TYPE0_REG_OFF;
		else
			reg_offset = HISI_LLC_EVENT_TYPE1_REG_OFF;

		djtag_node = llc_data[llc_idx].djtag_node;

		/*
		 * Clear the event in LLC_EVENT_TYPEx Register
		 * for all LLC banks
		 */
		for (i = 0; i < NUM_LLC_BANKS; i++) {
			cfg_en = llc_data[llc_idx].bank[i].cfg_en;

			hisi_djtag_readreg(HISI_LLC_MODULE_ID,
					cfg_en,
					reg_offset,
					djtag_node, &value);

			value &= ~(0xff << (8 * counter_idx));
			value |= (0xff << (8 * counter_idx));
			hisi_djtag_writereg(HISI_LLC_MODULE_ID,
					cfg_en,
					reg_offset,
					value,
					djtag_node);
		}

	}

	return;
}

void hisi_pmu_clear_event_idx(int idx)
{
	int cntr_idx = idx & ~(HISI_CNTR_DIE_MASK);
	u32 module_idx = (((idx & HISI_CNTR_DIE_MASK) >> 8) - 1);

	if (ARMV8_HISI_IDX_LLC_COUNTER0 <= cntr_idx &&
		 cntr_idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		hisi_clear_llc_event_idx(idx);
	} else if (ARMV8_HISI_IDX_MN_COUNTER0 <= cntr_idx &&
			cntr_idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		__clear_bit(cntr_idx - ARMV8_HISI_IDX_MN_COUNTER0,
				mn_data[module_idx].hisi_mn_event_used_mask);
	}

	return;
}

int hisi_pmu_get_event_idx(struct hw_perf_event *event)
{
	int event_idx = -1;
	u32 raw_event_code = event->config_base;
	unsigned long evtype = raw_event_code & HISI_ARMV8_EVTYPE_EVENT;
	u32 dieID = (raw_event_code & HISI_HW_DIE_MASK) >> 20;
	u32 module_idx = dieID - 1;
	u32 counter_idx;

	if (!dieID || (HISI_HW_DIE_MAX < dieID)) {
		pr_err("Invalid DieID=%d in event code!\n", dieID);
		return -EINVAL;
	}

	/* If event type is LLC events */
	if (evtype >= ARMV8_HISI_PERFCTR_LLC_READ_ALLOCATE &&
			evtype <= ARMV8_HISI_PERFCTR_LLC_DGRAM_1B_ECC) {
		event_idx =
			find_first_zero_bit(
				llc_data[module_idx].hisi_llc_event_used_mask,
						HISI_ARMV8_MAX_CFG_LLC_CNTR);

		if (event_idx == HISI_ARMV8_MAX_CFG_LLC_CNTR) {
			pr_err("LLC: Hardware counters not free!\n");
			return -EAGAIN;
		}

		__set_bit(event_idx,
			llc_data[module_idx].hisi_llc_event_used_mask);

		switch (module_idx + 1) {
		case HISI_SOC0_TOTEMC:
			counter_idx =
				ARMV8_HISI_IDX_LLC_COUNTER0 |
				(HISI_SOC0_TOTEMC << 8);
		break;
		case HISI_SOC0_TOTEMA:
			counter_idx =
				ARMV8_HISI_IDX_LLC_COUNTER0 |
				(HISI_SOC0_TOTEMA << 8);
		break;
		default:
			return -EINVAL;
		break;
		}

		pr_debug("LLC:event_idx=%d LLC index:%d\n",
				event_idx, module_idx);
		event_idx = event_idx + counter_idx;
	} else if (evtype >= ARMV8_HISI_PERFCTR_MN_EO_BARR_REQ &&
			evtype <= ARMV8_HISI_PERFCTR_MN_RETRY_REQ) {
		event_idx =
			find_first_zero_bit(
			mn_data[module_idx].hisi_mn_event_used_mask,
						HISI_ARMV8_MAX_CFG_MN_CNTR);

		if (event_idx == HISI_ARMV8_MAX_CFG_MN_CNTR) {
			pr_err("MN: Hardware counters not free!\n");
			return -EAGAIN;
		}

		__set_bit(event_idx,
			mn_data[module_idx].hisi_mn_event_used_mask);

		switch (module_idx + 1) {
		case HISI_SOC0_TOTEMC:
			counter_idx =
				ARMV8_HISI_IDX_MN_COUNTER0 |
					(HISI_SOC0_TOTEMC << 8);
		break;
		case HISI_SOC0_TOTEMA:
			counter_idx =
				ARMV8_HISI_IDX_MN_COUNTER0 |
					(HISI_SOC0_TOTEMA << 8);
		break;
		default:
			return -EINVAL;
		break;
		}

		pr_debug("MN:event_idx=%d MN index:%d\n",
				event_idx, module_idx);
		event_idx = event_idx + counter_idx;
	}

	return event_idx;
}

int hisi_pmu_enable_intens(int idx)
{
	return 0;
}

int hisi_pmu_disable_intens(int idx)
{
	return 0;
}

/* Create variables to store previous count based on no of
 * banks and Dies
 */
int hisi_init_llc_hw_perf_event(struct hw_perf_event *hwc)
{
	struct hisi_llc_hwc_data_info *phisi_hwc_data_info = NULL;
	struct device_node *djtag_node;
	u32 num_banks = NUM_LLC_BANKS;
	u32 raw_event_code = hwc->config_base;
	u32 value, cfg_en;
	u32 dieID = (raw_event_code & HISI_HW_DIE_MASK) >> 20;
	u32 llc_idx = dieID - 1;
	int i;

	if (!dieID || (HISI_HW_DIE_MAX < dieID)) {
		pr_err("LLC: Invalid DieID=%d in event code!\n", dieID);
		return -EINVAL;
	}

	/* Check if the LLC data is initialized for this Die */
	if (!llc_data[llc_idx].djtag_node) {
		pr_err("LLC: DieID=%d not initialized!\n", dieID);
		return -EINVAL;
	}

	/*
	 * Create event counter local variables for each bank to
	 * store the previous counter value
	 */
	phisi_hwc_data_info = kzalloc(sizeof(struct hisi_llc_hwc_data_info),
								 GFP_ATOMIC);
	if (unlikely(!phisi_hwc_data_info)) {
		pr_err("LLC: Alloc failed for hisi hwc die data!.\n");
		return -ENOMEM;
	}

	phisi_hwc_data_info->num_banks = num_banks;

	phisi_hwc_data_info->hwc_prev_counters =
				kzalloc(num_banks * sizeof(struct hisi_hwc_prev_counter),
								GFP_ATOMIC);
	if (unlikely(!phisi_hwc_data_info)) {
		pr_err("LLC: Alloc failed for hisi hwc die bank data!.\n");
		kfree(phisi_hwc_data_info);
		return -ENOMEM;
	}

	hwc->perf_event_data = phisi_hwc_data_info;

	djtag_node = llc_data[llc_idx].djtag_node;

	/* Enable interrupts for counter overlow */
	for (i = 0; i < NUM_LLC_BANKS; i++) {
		cfg_en = llc_data[llc_idx].bank[i].cfg_en;

		hisi_djtag_readreg(HISI_LLC_MODULE_ID,
				cfg_en,
				HISI_LLC_BANK_INTM,
				djtag_node, &value);
		if (value) {
			hisi_djtag_writereg(HISI_LLC_MODULE_ID,
				cfg_en,
				HISI_LLC_BANK_INTM,
				0x0,
				djtag_node);
		}
	}

	return 0;
}

int hisi_init_mn_hw_perf_event(struct hw_perf_event *hwc)
{
	struct hisi_mn_hwc_data_info *phisi_hwc_data_info = NULL;
	u32 raw_event_code = hwc->config_base;
	u32 dieID = (raw_event_code & HISI_HW_DIE_MASK) >> 20;
	u32 mn_idx = dieID - 1;

	if (!dieID || (HISI_HW_DIE_MAX < dieID)) {
		pr_err("MN: Invalid DieID=%d in event code!\n", dieID);
		return -EINVAL;
	}

	/* Check if the MN data is initialized for this Die */
	if (!mn_data[mn_idx].djtag_node) {
		pr_err("MN: DieID=%d not initialized!\n", dieID);
		return -EINVAL;
	}

	/*
	 * Create event counter local64 variables for MN to
	 * store the previous counter value
	 */
	phisi_hwc_data_info = kzalloc(sizeof(struct hisi_mn_hwc_data_info),
					GFP_ATOMIC);
	if (unlikely(!phisi_hwc_data_info)) {
		pr_err("MN: Alloc failed for hisi hwc die data!.\n");
		return -ENOMEM;
	}

	hwc->perf_event_data = phisi_hwc_data_info;
	return 0;
}

void hisi_set_llc_evtype(int idx, u32 val)
{
	struct device_node *djtag_node;
	u32 reg_offset;
	u32 value = 0;
	int cfg_en;
	u32 counter_idx;
	u32 event_value;
	int i;
	int cntr_idx = idx & ~(HISI_CNTR_DIE_MASK);
	u32 llc_idx = (((idx & HISI_CNTR_DIE_MASK) >> 8) - 1);

	if (cntr_idx >= ARMV8_HISI_IDX_LLC_COUNTER0 &&
		cntr_idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		counter_idx = cntr_idx - ARMV8_HISI_IDX_LLC_COUNTER0;
	} else {
		pr_err("Unsupported event index:%d!\n", idx);
		return;
	}

	event_value = (val -
			ARMV8_HISI_PERFCTR_LLC_READ_ALLOCATE);

	/* Select the appropriate Event select register */
	if (counter_idx <= 3)
		reg_offset = HISI_LLC_EVENT_TYPE0_REG_OFF;
	else
		reg_offset = HISI_LLC_EVENT_TYPE1_REG_OFF;

	/* Value to write to event type register */
	val = event_value << (8 * counter_idx);

	djtag_node = llc_data[llc_idx].djtag_node;

	/*
	 * Set the event in LLC_EVENT_TYPEx Register
	 * for all LLC banks
	 */
	for (i = 0; i < NUM_LLC_BANKS; i++) {
		cfg_en = llc_data[llc_idx].bank[i].cfg_en;
		hisi_djtag_readreg(HISI_LLC_MODULE_ID,
				cfg_en,
				reg_offset,
				djtag_node, &value);

		value &= ~(0xff << (8 * counter_idx));
		value |= val;

		hisi_djtag_writereg(HISI_LLC_MODULE_ID,
				cfg_en,
				reg_offset,
				value,
				djtag_node);
	}
}

void hisi_set_mn_evtype(int idx, u32 val)
{
	struct device_node *djtag_node;
	u32 reg_offset;
	u32 value = 0;
	u32 counter_idx;
	u32 event_value;

	int cntr_idx = idx & ~(HISI_CNTR_DIE_MASK);
	u32 mn_idx = (((idx & HISI_CNTR_DIE_MASK) >> 8) - 1);

	if (cntr_idx >= ARMV8_HISI_IDX_MN_COUNTER0 &&
		cntr_idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		counter_idx = cntr_idx - ARMV8_HISI_IDX_MN_COUNTER0;
		event_value = (val -
				ARMV8_HISI_PERFCTR_MN_EO_BARR_REQ);
	} else {
		pr_err("Unsupported event index:%d!\n", idx);
		return;
	}

	/* Value to write to event type register */
	val = event_value << (8 * counter_idx);

	/* Find the djtag device node of the Die */
	djtag_node = mn_data[mn_idx].djtag_node;

	/* Set the event in MN_EVENT_TYPEx Register */
	reg_offset = HISI_MN_EVENT_TYPE;

	hisi_djtag_readreg(HISI_MN1_MODULE_ID,
			HISI_MN_MODULE_CFGEN,
			reg_offset,
			djtag_node, &value);

	value &= ~(0xff << (8 * counter_idx));
	value |= val;

	hisi_djtag_writereg(HISI_MN1_MODULE_ID,
			HISI_MN_MODULE_CFGEN,
			reg_offset,
			value,
			djtag_node);
}

void hisi_pmu_write_evtype(int idx, u32 val)
{
	int cntr_idx = idx & ~(HISI_CNTR_DIE_MASK);

	val &= HISI_ARMV8_EVTYPE_EVENT;

	/* Select event based on Counter Module */
	if (ARMV8_HISI_IDX_LLC_COUNTER0 <= cntr_idx &&
		 cntr_idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		hisi_set_llc_evtype(idx, val);
	} else if (ARMV8_HISI_IDX_MN_COUNTER0 <= cntr_idx &&
			cntr_idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		hisi_set_mn_evtype(idx, val);
	}
}

inline int armv8_hisi_counter_valid(int idx)
{
	int cntr_idx = idx & ~(HISI_CNTR_DIE_MASK);

	return (cntr_idx >= ARMV8_HISI_IDX_LLC_COUNTER0 &&
			cntr_idx < ARMV8_HISI_IDX_COUNTER_MAX);
}

int hisi_enable_llc_counter(int idx)
{
	struct device_node *djtag_node;
	u32 num_banks = NUM_LLC_BANKS;
	u32 value = 0;
	int cfg_en;
	int i, ret = 0;
	u32 llc_idx = (((idx & HISI_CNTR_DIE_MASK) >> 8) - 1);

	djtag_node = llc_data[llc_idx].djtag_node;

	/*
	 * Set the event_bus_en bit in LLC AUCNTRL to enable counting
	 * for all LLC banks
	 */
	for (i = 0; i < num_banks; i++) {
		cfg_en = llc_data[llc_idx].bank[i].cfg_en;
		ret = hisi_djtag_readreg(HISI_LLC_MODULE_ID,
				cfg_en,
				HISI_LLC_AUCTRL_REG_OFF,
				djtag_node, &value);

		value |= HISI_LLC_AUCTRL_EVENT_BUS_EN;
		ret = hisi_djtag_writereg(HISI_LLC_MODULE_ID,
				cfg_en,
				HISI_LLC_AUCTRL_REG_OFF,
				value,
				djtag_node);
	}

	return ret;
}

int hisi_enable_mn_counter(int idx)
{
	struct device_node *djtag_node;
	u32 value = 0;
	int ret;
	u32 mn_idx = (((idx & HISI_CNTR_DIE_MASK) >> 8) - 1);

	/* Find the djtag device node of the Die */
	djtag_node = mn_data[mn_idx].djtag_node;

	/* Set the event_en bit in MN_EVENT_CTRL to enable counting */
	ret = hisi_djtag_readreg(HISI_MN1_MODULE_ID,
			HISI_MN_MODULE_CFGEN,
			HISI_MN_EVENT_CTRL,
			djtag_node, &value);

	value |= HISI_MN_EVENT_EN;
	ret = hisi_djtag_writereg(HISI_MN1_MODULE_ID,
			HISI_MN_MODULE_CFGEN,
			HISI_MN_EVENT_CTRL,
			value,
			djtag_node);

	return ret;
}

int hisi_pmu_enable_counter(int idx)
{
	int ret = 0;
	int cntr_idx = idx & ~(HISI_CNTR_DIE_MASK);

	if (ARMV8_HISI_IDX_LLC_COUNTER0 <= cntr_idx &&
			cntr_idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		ret = hisi_enable_llc_counter(idx);
	} else if (ARMV8_HISI_IDX_MN_COUNTER0 <= cntr_idx &&
			cntr_idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		ret = hisi_enable_mn_counter(idx);
	}

	return ret;
}

void hisi_disable_llc_counter(int idx)
{
	struct device_node *djtag_node;
	u32 num_banks = NUM_LLC_BANKS;
	u32 value = 0;
	int cfg_en;
	int i;
	int cntr_idx = idx & ~(HISI_CNTR_DIE_MASK);
	u32 llc_idx = (((idx & HISI_CNTR_DIE_MASK) >> 8) - 1);

	if (!(cntr_idx >= ARMV8_HISI_IDX_LLC_COUNTER0 &&
		cntr_idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX)) {
		pr_err("Unsupported event index:%d!\n", idx);
		return;
	}

	djtag_node = llc_data[llc_idx].djtag_node;

	/*
	 * Clear the event_bus_en bit in LLC AUCNTRL if no other
	 * event counting for all LLC banks
	 */
	for (i = 0; i < num_banks; i++) {
		cfg_en = llc_data[llc_idx].bank[i].cfg_en;
		hisi_djtag_readreg(HISI_LLC_MODULE_ID,
				cfg_en,
				HISI_LLC_AUCTRL_REG_OFF,
				djtag_node, &value);

		value &= ~(HISI_LLC_AUCTRL_EVENT_BUS_EN);
		hisi_djtag_writereg(HISI_LLC_MODULE_ID,
				cfg_en,
				HISI_LLC_AUCTRL_REG_OFF,
				value,
				djtag_node);
	}
}

void hisi_disable_mn_counter(int idx)
{
	struct device_node *djtag_node;
	u32 value = 0;
	int cntr_idx = idx & ~(HISI_CNTR_DIE_MASK);
	u32 mn_idx = (((idx & HISI_CNTR_DIE_MASK) >> 8) - 1);

	if (!(cntr_idx >= ARMV8_HISI_IDX_MN_COUNTER0 &&
				cntr_idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX)) {
		pr_err("Unsupported event index:%d!\n", idx);
		return;
	}

	/* Find the djtag device node of the Die */
	djtag_node = mn_data[mn_idx].djtag_node;

	/* Clear the event_bus_en bit in MN_EVENT_CTRL to disable counting */
	hisi_djtag_readreg(HISI_MN1_MODULE_ID,
			HISI_MN_MODULE_CFGEN,
			HISI_MN_EVENT_CTRL,
			djtag_node, &value);

	value |= ~HISI_MN_EVENT_EN;
	hisi_djtag_writereg(HISI_MN1_MODULE_ID,
			HISI_MN_MODULE_CFGEN,
			HISI_MN_EVENT_CTRL,
			value,
			djtag_node);
}

void hisi_pmu_disable_counter(int idx)
{
	int cntr_idx = idx & ~(HISI_CNTR_DIE_MASK);

	if (ARMV8_HISI_IDX_LLC_COUNTER0 <= cntr_idx &&
		 cntr_idx <=	ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		hisi_disable_llc_counter(idx);
	} else if (ARMV8_HISI_IDX_MN_COUNTER0 <= cntr_idx &&
			cntr_idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		hisi_disable_mn_counter(idx);
	}
}

u32 hisi_read_llc_counter(int idx, struct device_node *djtag_node, int bank)
{
	u32 reg_offset = 0;
	u32 value;

	reg_offset = HISI_LLC_COUNTER0_REG_OFF + (idx * 4);

	hisi_djtag_readreg(HISI_LLC_MODULE_ID, /* ModuleID  */
			bank,
			reg_offset, /* Register Offset */
			djtag_node, &value);

	return value;
}

u32 hisi_read_mn_counter(int idx, struct device_node *djtag_node, int bank)
{
	u32 reg_offset = 0;
	u32 value;

	reg_offset = HISI_MN_EVENT_CNT0 + (idx * 4);

	hisi_djtag_readreg(HISI_MN1_MODULE_ID, /* ModuleID  */
			bank,
			reg_offset, /* Register Offset */
			djtag_node, &value);

	return value;
}

u32 hisi_pmu_read_counter(int idx)
{
	int ret = 0;
	int cntr_idx = idx & ~(HISI_CNTR_DIE_MASK);

	if (ARMV8_HISI_IDX_LLC_COUNTER0 <= cntr_idx &&
			cntr_idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
	} else if (ARMV8_HISI_IDX_MN_COUNTER0 <= cntr_idx &&
			cntr_idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
	}

	return ret;
}

u32 hisi_write_llc_counter(int idx, u32 value)
{
	struct device_node *djtag_node;
	u32 num_banks = NUM_LLC_BANKS;
	int cfg_en;
	u32 reg_offset = 0;
	int i, counter_idx, ret = 0;
	int cntr_idx = idx & ~(HISI_CNTR_DIE_MASK);
	u32 llc_idx = (((idx & HISI_CNTR_DIE_MASK) >> 8) - 1);

	if (cntr_idx >= ARMV8_HISI_IDX_LLC_COUNTER0 &&
		cntr_idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		counter_idx = cntr_idx - ARMV8_HISI_IDX_LLC_COUNTER0;
	} else {
		pr_err("Unsupported event index:%d!\n", cntr_idx);
		return 0;
	}

	reg_offset = HISI_LLC_COUNTER0_REG_OFF +
					(counter_idx * 4);

	djtag_node = llc_data[llc_idx].djtag_node;
	for (i = 0; i < num_banks; i++) {
		cfg_en = llc_data[llc_idx].bank[i].cfg_en;
		ret = hisi_djtag_writereg(HISI_LLC_MODULE_ID,
					cfg_en,
					reg_offset,
					value,
					djtag_node);
	}

	return ret;
}

u32 hisi_write_mn_counter(int idx, u32 value)
{
	struct device_node *djtag_node;
	u32 reg_offset = 0;
	int counter_idx, ret = 0;
	int cntr_idx = idx & ~(HISI_CNTR_DIE_MASK);
	u32 mn_idx = (((idx & HISI_CNTR_DIE_MASK) >> 8) - 1);

	if (cntr_idx >= ARMV8_HISI_IDX_MN_COUNTER0 &&
			cntr_idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		counter_idx = cntr_idx - ARMV8_HISI_IDX_MN_COUNTER0;
	} else {
		pr_err("Unsupported event index:%d!\n", cntr_idx);
		return 0;
	}

	reg_offset = HISI_MN_EVENT_CNT0 + (counter_idx * 4);

	/* Find the djtag device node of the Die */
	djtag_node = mn_data[mn_idx].djtag_node;

	ret = hisi_djtag_writereg(HISI_MN1_MODULE_ID,
			HISI_MN_MODULE_CFGEN,
			reg_offset,
			value,
			djtag_node);

	return ret;
}

int hisi_pmu_write_counter(int idx, u32 value)
{
	int cntr_idx = idx & ~(HISI_CNTR_DIE_MASK);

	if (ARMV8_HISI_IDX_LLC_COUNTER0 <= cntr_idx &&
			cntr_idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		hisi_write_llc_counter(idx, value);
	} else if (ARMV8_HISI_IDX_MN_COUNTER0 <= cntr_idx &&
			cntr_idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		hisi_write_mn_counter(idx, value);
	}

	return value;
}

irqreturn_t hisi_llc_event_handle_irq(int irq_num, void *dev)
{
	struct device_node *djtag_node;
	u32 num_banks = NUM_LLC_BANKS;
	int cfg_en;
	u32 value;
	u32 bit_value;
	int llc_idx;
	int bit_pos;
	int i = 0;

	/* Identify the bank and the ITS Register */
	for (llc_idx = 0; llc_idx < gnum_llc; llc_idx++) {
		for (i = 0; i < num_banks; i++) {
			if (irq_num ==
				llc_data[llc_idx].bank[i].irq) {
				pr_debug("LLC: irq:%d matched with" \
						" LLC bank:%d\n",
							 irq_num, i);
				break;
			}
		}
		if (num_banks != i)
			break;
	}

	if (i == num_banks)
		return IRQ_NONE;

	/* Find the djtag device node of the Die */
	djtag_node = llc_data[llc_idx].djtag_node;

	cfg_en = llc_data[llc_idx].bank[i].cfg_en;

	/* Read the staus register if any bit is set */
	hisi_djtag_readreg(HISI_LLC_MODULE_ID,
			cfg_en,
			HISI_LLC_BANK_INTS,
			djtag_node, &value);
	pr_debug("LLC: handle_irq bank=0x%x" \
				"ITS value=0x%x\n", i, value);

	/* Find the bits sets and clear them */
	for (bit_pos = 0; bit_pos <= HISI_ARMV8_MAX_CFG_LLC_CNTR;
							bit_pos++) {
		if (test_bit(bit_pos, (unsigned long *)&value)) {
			pr_debug("LLC:handle_irq - Bit %d is set\n",
								bit_pos);
			/* Reset the IRQ status flag */
			value &= ~bit_pos;

			hisi_djtag_writereg(HISI_LLC_MODULE_ID,
					cfg_en,
					HISI_LLC_BANK_INTC,
					value,
					djtag_node);

			/* Read the staus register to confirm */
			hisi_djtag_readreg(HISI_LLC_MODULE_ID,
					cfg_en,
					HISI_LLC_BANK_INTS,
					djtag_node, &value);
			pr_debug("LLC:handle_irq - ITS Value after" \
					" reset:0x%x\n",
					value);
			/* Update overflow times to refer in event_update */
			bit_value = atomic_read(
			&llc_data[llc_idx].bank[i].cntr_ovflw[bit_pos]);
			atomic_inc(
			&llc_data[llc_idx].bank[i].cntr_ovflw[bit_pos]);
			pr_debug("LLC: handle_irq - The counter overflow " \
				"times is 0x%x\n",
				llc_data[llc_idx].bank[i].cntr_ovflw[bit_pos]);
		}
	}

	/* FIXME: Event set new period */
	/* FIXME: Handle event overflow updation */

	return IRQ_HANDLED;
}

static int init_hisi_llc_banks(hisi_llc_data *pllc_data,
					struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	int i, j, ret, irq, num_irqs = 0;

	pllc_data->num_banks = NUM_LLC_BANKS;
	for (i = 0; i < NUM_LLC_BANKS; i++)
		pllc_data->bank[i].cfg_en = llc_cfgen_map[i];

	/* Get the number of IRQ's for LLC banks in a Totem */
	while ((res =
		platform_get_resource(pdev, IORESOURCE_IRQ, num_irqs))) {
		num_irqs++;
	}

	if (num_irqs > NUM_LLC_BANKS) {
		pr_err("LLC: Invalid IRQ numbers in dts.\n");
		return -EINVAL;
	}

	for (i = 0; i < num_irqs; i++) {
		irq = platform_get_irq(pdev, i);
		if (irq < 0) {
			dev_err(dev, "failed to get irq index %d\n", i);
			return -ENODEV;
		}

		ret = request_irq(irq, hisi_llc_event_handle_irq,
				IRQF_NOBALANCING,
				"hisi-pmu", pllc_data);
		if (ret) {
			pr_err("LLC: unable to request IRQ%d for HISI PMU" \
					"Stub counters\n", irq);
			goto err;
		}
		pr_debug("LLC:IRQ:%d assigned to bank:%d\n", irq, i);
		pllc_data->bank[i].irq = irq;
	}

	return 0;
err:
	for (j = 0; j < i; j++) {
		irq = platform_get_irq(pdev, j);
		if (irq < 0) {
			dev_err(dev, "failed to get irq index %d\n", i);
			continue;
		}
		free_irq(irq, NULL);
	}

	return ret;
}

static int init_hisi_llc_data(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct device_node *djtag_node;
	struct of_phandle_args arg;
	int ret, die, llc_idx;

	ret = of_parse_phandle_with_fixed_args(node,
						"djtag", 1, 0, &arg);
	if (!ret) {
		if (arg.args[0] > 0  && arg.args[0] <= MAX_DIE) {
			die = arg.args[0];
			djtag_node = arg.np;
			pr_debug("LLC: llc_device_probbe - die=%d\n", die);
		} else
			return -EINVAL;
	} else {
		pr_err("LLC: llc_device_probe - node without djtag..\n!");
		return -EINVAL;
	}

	llc_idx = die - 1;
	llc_data[llc_idx].djtag_node = djtag_node;

	ret = init_hisi_llc_banks(&llc_data[llc_idx], pdev);
	if (ret)
		return ret;

	memset(llc_data[llc_idx].hisi_llc_event_used_mask, 0,
			sizeof(llc_data[llc_idx].hisi_llc_event_used_mask));

	gnum_llc++;

	return 0;
}

static int hisi_pmu_llc_dev_probe(struct platform_device *pdev)
{
	int ret;

	ret = init_hisi_llc_data(pdev);
	if (ret)
		return ret;

	return 0;
}

static int hisi_pmu_llc_dev_remove(struct platform_device *pdev)
{
	return 0;
}

static struct of_device_id llc_of_match[] = {
	{ .compatible = "hisilicon,hip05-llc", },
	{ .compatible = "hisilicon,hip06-llc", },
	{},
};
MODULE_DEVICE_TABLE(of, llc_of_match);

static struct platform_driver hisi_pmu_llc_driver = {
	.driver = {
		.name = "hisi-llc-perf",
		.of_match_table = llc_of_match,
	},
	.probe = hisi_pmu_llc_dev_probe,
	.remove = hisi_pmu_llc_dev_remove,
};
module_platform_driver(hisi_pmu_llc_driver);

static int init_hisi_mn_data(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct device_node *djtag_node;
	struct of_phandle_args arg;
	int ret, die, mn_idx;

	ret = of_parse_phandle_with_fixed_args(node,
					"djtag", 1, 0, &arg);
	if (!ret) {
		if (arg.args[0] > 0  && arg.args[0] <= MAX_DIE) {
			die = arg.args[0];
			djtag_node = arg.np;
			pr_debug("MN: mn_device_probbe - die=%d\n", die);
		} else
			return -EINVAL;
	} else {
		pr_err("MN: mn_device_probe - node without djtag..\n!");
		return ret;
	}

	mn_idx = die - 1;
	mn_data[mn_idx].djtag_node = djtag_node;

	memset(mn_data[mn_idx].hisi_mn_event_used_mask, 0,
			sizeof(mn_data[mn_idx].hisi_mn_event_used_mask));

	return 0;
}

static int hisi_pmu_mn_dev_probe(struct platform_device *pdev)
{
	int ret;

	ret = init_hisi_mn_data(pdev);
	return ret;
}

static int hisi_pmu_mn_dev_remove(struct platform_device *pdev)
{
	return 0;
}

static struct of_device_id mn_of_match[] = {
	{ .compatible = "hisilicon,hip05-mn", },
	{ .compatible = "hisilicon,hip06-mn", },
	{},
};
MODULE_DEVICE_TABLE(of, mn_of_match);

static struct platform_driver hisi_pmu_mn_driver = {
	.driver = {
		.name = "hisi-mn-perf",
		.of_match_table = mn_of_match,
	},
	.probe = hisi_pmu_mn_dev_probe,
	.remove = hisi_pmu_mn_dev_remove,
};
module_platform_driver(hisi_pmu_mn_driver);

MODULE_DESCRIPTION("HiSilicon ARMv8 PMU driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Anurup M");
