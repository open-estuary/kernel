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
#include <linux/mbi.h>
#include <linux/mfd/syscon.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/perf_event.h>
#include <linux/slab.h>
#include <asm/cputype.h>
#include <asm/irq.h>
#include <asm/irq_regs.h>
#include <asm/pmu.h>
#include <asm/atomic.h>
#include "perf_event_hip05.h"
#include "djtag.h"

/* Map cfg_en values for LLC Banks */
const int llc_cfgen_map[] = { HISI_LLC_BANK0_CFGEN, HISI_LLC_BANK1_CFGEN,
				HISI_LLC_BANK1_CFGEN, HISI_LLC_BANK3_CFGEN
};

/* SoC Die information and Djtag ioremap addr holder */
hisi_syscon_reg_map soc_djtag_regmap[MAX_DIE];

/* LLC information */
hisi_llc_data llc_data[MAX_DIE];

/* Index for easy access to SoC regmap struct */
/* These shall be removed after we change event code
 * to contan die and socket information */
int gs0_ta_llc_idx = -1;
int gs0_tc_llc_idx = -1;
int gs1_ta_llc_idx = -1;
int gs1_tc_llc_idx = -1;
int gnum_llc = 0;

/* MN information */
hisi_mn_data mn_data[MAX_DIE];
int gs0_ta_mn_idx = -1;
int gs0_tc_mn_idx = -1;
int gs1_ta_mn_idx = -1;
int gs1_tc_mn_idx = -1;

/* DDR information */
hisi_ddr_data ddr_data[MAX_DIE];

/* Index for easy access to SoC DDR's */
/* These shall be removed after we change event code
 * to contan die and socket information */
int gnum_ddrs = 0;
int gta_ddr_idx = -1;
int gtb_ddr_idx = -1;
int gtc_ddr_idx = -1;

/* Bitmap for the supported hardware event counter index */
/* These shall be optimized after we change event code
 * to contan die and socket information */
DECLARE_BITMAP(hisi_llc_s0_tc_event_used_mask,
				HISI_ARMV8_MAX_CFG_LLC_CNTR);
DECLARE_BITMAP(hisi_llc_s0_ta_event_used_mask,
				HISI_ARMV8_MAX_CFG_LLC_CNTR);
DECLARE_BITMAP(hisi_mn_s0_tc_event_used_mask,
				HISI_ARMV8_MAX_CFG_MN_CNTR);
DECLARE_BITMAP(hisi_mn_s0_ta_event_used_mask,
				HISI_ARMV8_MAX_CFG_MN_CNTR);
DECLARE_BITMAP(hisi_ddr_event_used_mask,
				HISI_ARMV8_MAX_CFG_DDR_CNTR);

static inline u64 update_ddr_event_data(unsigned evtype,
					local64_t *pevent_start_count) {
	u64 new_raw_count;
	u64 prev_raw_count;

again:
	prev_raw_count = local64_read(pevent_start_count);
	new_raw_count = hisi_read_ddr_counter(evtype);

	if (local64_cmpxchg(pevent_start_count, prev_raw_count,
				new_raw_count) != prev_raw_count)
		goto again;

	return new_raw_count;
}

u64 hisi_armv8_ddr_update_start_value(struct perf_event *event,
					struct hw_perf_event *hwc,
								 int idx) {
	struct hisi_ddr_hwc_data_info *phisi_hwc_data = hwc->perf_event_data;
	unsigned long evtype = hwc->config_base & HISI_ARMV8_EVTYPE_EVENT;
	u64 new_raw_count = 0;
	s64 cpu_prev_start_time;
        ktime_t time_start;

	new_raw_count = update_ddr_event_data(evtype,
				 &phisi_hwc_data->event_start_count);

again:
	/* Update Start CPU time */
        time_start = ktime_get();
	cpu_prev_start_time = local64_read(&phisi_hwc_data->cpu_start_time);
        if (local64_cmpxchg(&phisi_hwc_data->cpu_start_time,
				 cpu_prev_start_time, time_start.tv64)
						 != cpu_prev_start_time)
		goto again;

	return new_raw_count;
}

u64 hisi_ddr_event_update(struct perf_event *event,
                                 struct hw_perf_event *hwc, int idx) {
	struct hisi_ddr_hwc_data_info *phisi_hwc_data = hwc->perf_event_data;
	unsigned long evtype = hwc->config_base & HISI_ARMV8_EVTYPE_EVENT;
	struct arm_pmu *armpmu = to_arm_pmu(event->pmu);
	ktime_t time_start;
	u64 delta, prev_raw_count, new_raw_count = 0;
	s64 cpu_start_time;
        s64 elapsed_ns;

	cpu_start_time = local64_read(&phisi_hwc_data->cpu_start_time);
	time_start.tv64 = cpu_start_time;

	elapsed_ns = ktime_to_ns(ktime_sub(ktime_get(), time_start));
	//pr_info("Elapsed time is %llx\n", elapsed_ns);

again:
	prev_raw_count = local64_read(&phisi_hwc_data->event_start_count);
	new_raw_count = hisi_read_ddr_counter(evtype);

        if (local64_cmpxchg(&phisi_hwc_data->event_start_count, prev_raw_count,
					new_raw_count) != prev_raw_count)
		goto again;

	delta = (new_raw_count - prev_raw_count) &
						armpmu->max_period;
	//pr_info("delta is %llu\n", delta);

	local64_add(delta, &event->count);
	local64_sub(delta, &hwc->period_left);

	return new_raw_count;
}

u64 hisi_llc_event_update(struct perf_event *event,
                                 struct hw_perf_event *hwc, int idx) {
	struct arm_pmu *armpmu = to_arm_pmu(event->pmu);
	struct hisi_llc_hwc_data_info *phisi_hwc_data = hwc->perf_event_data;
	u64 delta, delta_ovflw, prev_raw_count, new_raw_count = 0;
	u64 djtag_address;
	u32 llc_idx = 0;
	u32 cfg_en;
	u32 ovfl_cnt = 0;
	int counter_idx;
	int i, j = 0;

	/* FIXME: This function code logic need improvement.
	 * Data structure to be optimized.
	 * We need encoding in counter idx to identify socket
	 * and totem */
	if (idx >= ARMV8_HISI_IDX_LLC_S0_TA_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		/* handle for TotemA S0 */
		if (-1 != gs0_ta_llc_idx) {
			llc_idx = gs0_ta_llc_idx;
			counter_idx = idx - ARMV8_HISI_IDX_LLC_S0_TA_COUNTER0;
		}
		else
			return 0;
	}
	else if (idx >= ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_LLC_S0_TC_COUNTER_MAX) {
		/* handle for TotemC S0 */
		if (-1 != gs0_tc_llc_idx) {
			llc_idx = gs0_tc_llc_idx;
			counter_idx = idx - ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0;
		}
		else
			return 0;
	}
	else {
		pr_info("Unsupported event index:%d!\n", idx);
		return 0;
	}

	/* Find the djtag address of the Die */
	djtag_address = llc_data[llc_idx].djtag_reg_map;
//	pr_info("djtag address is 0x%llx\n", djtag_address);

	for (i = 0; i < NUM_LLC_BANKS; i++, j++) {
		cfg_en = llc_data[llc_idx].bank[i].cfg_en;
again:
		prev_raw_count =
			local64_read(
			&phisi_hwc_data->hwc_prev_counters[j].prev_count);
		new_raw_count =
			hisi_read_llc_counter(counter_idx,
					djtag_address, cfg_en);

		if (local64_cmpxchg(
			&phisi_hwc_data->hwc_prev_counters[j].prev_count,
					prev_raw_count, new_raw_count) !=
								prev_raw_count)
			goto again;

		/* update the interrupt count also */
		ovfl_cnt = atomic_read(&llc_data[llc_idx].bank[i].cntr_ovflw[counter_idx]);
		if (0 != ovfl_cnt) {
			delta_ovflw =
				(0xFFFFFFFF - prev_raw_count) +
					 ((ovfl_cnt - 1) * 0xFFFFFFFF);
			delta = (delta_ovflw + new_raw_count) &
						armpmu->max_period;
			//pr_info("delta ovflw is %llu\n", delta_ovflw);
		}
		else {
			delta = (new_raw_count - prev_raw_count) &
						armpmu->max_period;
		}

		/*pr_info("delta is %llu\n", delta);*/
		local64_add(delta, &event->count);
		local64_sub(delta, &hwc->period_left);
	}

	return new_raw_count;
}

u64 hisi_mn_event_update(struct perf_event *event,
                                 struct hw_perf_event *hwc, int idx) {
	struct arm_pmu *armpmu = to_arm_pmu(event->pmu);
	struct hisi_mn_hwc_data_info *phisi_hwc_data = hwc->perf_event_data;
	u64 delta, prev_raw_count, new_raw_count = 0;
	u64 djtag_address;
	u32 mn_idx = 0;
	int counter_idx;

	/* FIXME: This function code logic need improvement.
	 * Data structure to be optimized.
	 * We need encoding in counter idx to identify socket
	 * and totem */
	if (idx >= ARMV8_HISI_IDX_MN_S0_TA_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		/* handle for TotemA S0 */
		if (-1 != gs0_ta_mn_idx) {
			mn_idx = gs0_ta_mn_idx;
			counter_idx = idx - ARMV8_HISI_IDX_MN_S0_TA_COUNTER0;
		}
		else
			return 0;
	}
	else if (idx >= ARMV8_HISI_IDX_MN_S0_TC_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_MN_S0_TC_COUNTER_MAX) {
		/* handle for TotemC S0 */
		if (-1 != gs0_tc_mn_idx) {
			mn_idx = gs0_tc_mn_idx;
			counter_idx = idx - ARMV8_HISI_IDX_MN_S0_TC_COUNTER0;
		}
		else
			return 0;
	}
	else {
		pr_info("Unsupported event index:%d!\n", idx);
		return 0;
	}

	djtag_address = mn_data[mn_idx].djtag_reg_map;
//	pr_info("djtag address is 0x%llx\n", djtag_address);

again:
	prev_raw_count =
		local64_read(&phisi_hwc_data->event_start_count);
	new_raw_count =
		hisi_read_mn_counter(counter_idx, djtag_address,
						HISI_MN_MODULE_CFGEN);

	if (local64_cmpxchg(&phisi_hwc_data->event_start_count,
				prev_raw_count, new_raw_count) !=
							prev_raw_count)
		goto again;

	delta = (new_raw_count - prev_raw_count) & armpmu->max_period;
	//pr_info("delta is %llu\n", delta);

	local64_add(delta, &event->count);
	local64_sub(delta, &hwc->period_left);

	return new_raw_count;
}

/* Read hardware counter and update the Perf counter statistics */
u64 hisi_armv8_pmustub_event_update(struct perf_event *event,
				 struct hw_perf_event *hwc, int idx) {
	u64 new_raw_count = 0;

	/* Identify Event type and read appropriate hardware counter
	 * and sum the values */
	if (ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0 <= idx &&
		 idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		 new_raw_count = hisi_llc_event_update(event, hwc, idx);
	}
	else if (ARMV8_HISI_IDX_MN_S0_TC_COUNTER0 <= idx &&
			idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		/*pr_info("Counter index is for MN..\n");*/
		new_raw_count = hisi_mn_event_update(event, hwc, idx);
	}
	else if (ARMV8_HISI_IDX_DDR_S0_TC_COUNTER0 <= idx &&
                        idx <= ARMV8_HISI_IDX_DDR_COUNTER_MAX) {
               new_raw_count = hisi_ddr_event_update(event, hwc, idx);
        }

	return new_raw_count;
}

#if 0
void hisi_armv8_pmustub_enable_counting(void)
{
	u32 value;
	u32 cfg_en;
	int i;

	/* Set event_bus_en bit of LLC_AUCTRL for the first event counting */
	value = __bitmap_weight(hisi_llc_event_used_mask,
					HISI_ARMV8_MAX_CFG_LLC_CNTR);
	if (0 == value) {
		/* Set the event_bus_en bit in LLC AUCNTRL to enable counting
		 * for all LLC banks */
		for (i = 0; i < 4; i++) {
			switch (i) {
			case 0:
				cfg_en = HISI_LLC_BANK0_CFGEN;
				break;
			case 1:
				cfg_en = HISI_LLC_BANK1_CFGEN;
				break;
			case 2:
				cfg_en = HISI_LLC_BANK2_CFGEN;
				break;
			case 3:
				cfg_en = HISI_LLC_BANK3_CFGEN;
				break;
			}

			hisi_djtag_readreg(HISI_LLC_BANK_MODULE_ID,
					cfg_en,
					HISI_LLC_AUCTRL_REG_OFF,
					HISI_DJTAG_BASE_ADDRESS, &value);

			value |= 0x1000000;
			hisi_djtag_writereg(HISI_LLC_BANK_MODULE_ID,
					cfg_en,
					HISI_LLC_AUCTRL_REG_OFF,
					value,
					HISI_DJTAG_BASE_ADDRESS);
		}
	}
}
#endif

int hisi_armv8_pmustub_clear_event_idx(int idx)
{
	int ret = -1;
	void *bitmap_addr;
	int counter_idx;

	/* We need encoding in counter idx to identify socket
	 * and totem */
	if (ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0 <= idx &&
		 idx <=	ARMV8_HISI_IDX_LLC_S0_TC_COUNTER_MAX) {
		bitmap_addr = hisi_llc_s0_tc_event_used_mask;
		counter_idx = ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0;
	}
	else if (ARMV8_HISI_IDX_LLC_S0_TA_COUNTER0 <= idx &&
		 idx <=	ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		bitmap_addr = hisi_llc_s0_ta_event_used_mask;
		counter_idx = ARMV8_HISI_IDX_LLC_S0_TA_COUNTER0;
	}
	else if (ARMV8_HISI_IDX_MN_S0_TA_COUNTER0 <= idx &&
			 idx <=	ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		bitmap_addr = hisi_mn_s0_ta_event_used_mask;
		counter_idx = ARMV8_HISI_IDX_MN_S0_TA_COUNTER0;
	}
	else if (ARMV8_HISI_IDX_MN_S0_TC_COUNTER0 <= idx &&
			 idx <=	ARMV8_HISI_IDX_MN_S0_TC_COUNTER_MAX) {
		bitmap_addr = hisi_mn_s0_tc_event_used_mask;
		counter_idx = ARMV8_HISI_IDX_MN_S0_TC_COUNTER0;
	}
	else if (ARMV8_HISI_IDX_DDR_S0_TC_COUNTER0 <= idx &&
			idx <= ARMV8_HISI_IDX_DDR_COUNTER_MAX) {
		bitmap_addr = hisi_ddr_event_used_mask;
		counter_idx = ARMV8_HISI_IDX_DDR_S0_TC_COUNTER0;
	}
	else
		return ret;

	__clear_bit(idx - counter_idx, bitmap_addr);

#if 0
	if ((event_idx - ARMV8_HISI_IDX_LLC_COUNTER0) <= 3)
		reg_offset = HISI_LLC_EVENT_TYPE0_REG_OFF;
	else
		reg_offset = HISI_LLC_EVENT_TYPE1_REG_OFF;

	/* Clear the event in LLC_EVENT_TYPE0 Register */
	ret = hisi_djtag_readreg(HISI_LLC_BANK0_MODULE_ID,
					HISI_LLC_BANK0_CFGEN,
					reg_offset,
					HISI_DJTAG_BASE_ADDRESS,
							 &value);
	if (0 < ret)
		pr_info("djtag_read failed!!\n");

	value &= ~(0xff << (8 * (event_idx - ARMV8_HISI_IDX_LLC_COUNTER0)));

	value |= (0xff << (8 * (event_idx - ARMV8_HISI_IDX_LLC_COUNTER0)));

	ret = hisi_djtag_writereg(HISI_LLC_BANK0_MODULE_ID, /* ModuleID  */
					HISI_LLC_BANK0_CFGEN, /* Cfg_enable */
					reg_offset, /* Register Offset */
					value,
					HISI_DJTAG_BASE_ADDRESS);
	/* Clear event_bus_en bit of LLC_AUCTRL if no event counting in
	 * progress */
	value = __bitmap_weight(hisi_llc_event_used_mask,
				HISI_ARMV8_MAX_CFG_LLC_CNTR);
	if (0 == value) {
		ret = hisi_djtag_readreg(HISI_LLC_BANK0_MODULE_ID,
				HISI_LLC_BANK0_CFGEN, /* Cfg_enable */
				HISI_LLC_AUCTRL_REG_OFF, /* Register Offset */
				HISI_DJTAG_BASE_ADDRESS, &value);

		value &= ~(HISI_LLC_AUCTRL_EVENT_BUS_EN);
		ret = hisi_djtag_writereg(HISI_LLC_BANK0_MODULE_ID,
				HISI_LLC_BANK0_CFGEN, /* Cfg_enable */
				HISI_LLC_AUCTRL_REG_OFF, /* Register Offset */
				value,
				HISI_DJTAG_BASE_ADDRESS);
	}
#endif
	return ret;
}

int hisi_armv8_pmustub_get_event_idx(unsigned long evtype)
{
	int event_idx = -1;

	/* If event type is LLC events */
	if (evtype >= ARMV8_HISI_PERFCTR_LLC_S0_TC_READ_ALLOCATE &&
			evtype <= ARMV8_HISI_PERFCTR_LLC_S0_TC_DGRAM_1B_ECC) {
		if (-1 == gs0_tc_llc_idx)
			return -EOPNOTSUPP;

		event_idx =
			find_first_zero_bit(hisi_llc_s0_tc_event_used_mask,
						HISI_ARMV8_MAX_CFG_LLC_CNTR);

		if (event_idx == HISI_ARMV8_MAX_CFG_LLC_CNTR)
			return -EAGAIN;

		__set_bit(event_idx, hisi_llc_s0_tc_event_used_mask);
		event_idx = event_idx + ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0;
	}
	else if (evtype >= ARMV8_HISI_PERFCTR_LLC_S0_TA_READ_ALLOCATE &&
                        evtype <= ARMV8_HISI_PERFCTR_LLC_S0_TA_DGRAM_1B_ECC) {
		if (-1 == gs0_ta_llc_idx)
			return -EOPNOTSUPP;

                event_idx =
			find_first_zero_bit(hisi_llc_s0_ta_event_used_mask,
						HISI_ARMV8_MAX_CFG_LLC_CNTR);

                if (event_idx == HISI_ARMV8_MAX_CFG_LLC_CNTR)
                        return -EAGAIN;

                __set_bit(event_idx, hisi_llc_s0_ta_event_used_mask);
                event_idx = event_idx + ARMV8_HISI_IDX_LLC_S0_TA_COUNTER0;
        }
	/* If event type is for MN */
	else if (evtype >= ARMV8_HISI_PERFCTR_MN_S0_TC_EO_BARR_REQ &&
			evtype <= ARMV8_HISI_PERFCTR_MN_S0_TC_RETRY_REQ ) {
		if (-1 == gs0_tc_mn_idx)
			return -EOPNOTSUPP;

		event_idx =
			find_first_zero_bit(hisi_mn_s0_tc_event_used_mask,
						HISI_ARMV8_MAX_CFG_MN_CNTR);

		if (event_idx == HISI_ARMV8_MAX_CFG_MN_CNTR)
			return -EAGAIN;

		__set_bit(event_idx, hisi_mn_s0_tc_event_used_mask);
		event_idx = event_idx + ARMV8_HISI_IDX_MN_S0_TC_COUNTER0;
	}
	else if (evtype >= ARMV8_HISI_PERFCTR_MN_S0_TA_EO_BARR_REQ &&
			evtype <= ARMV8_HISI_PERFCTR_MN_S0_TA_RETRY_REQ ) {
		if (-1 == gs0_ta_mn_idx)
			return -EOPNOTSUPP;

		event_idx = find_first_zero_bit(hisi_mn_s0_ta_event_used_mask,
				HISI_ARMV8_MAX_CFG_MN_CNTR);

		if (event_idx == HISI_ARMV8_MAX_CFG_MN_CNTR)
			return -EAGAIN;

		__set_bit(event_idx, hisi_mn_s0_ta_event_used_mask);
		event_idx = event_idx + ARMV8_HISI_IDX_MN_S0_TA_COUNTER0;
	}
	else if (evtype >= ARMV8_HISI_PERFCTR_DDRC0_TC_FLUX_READ_BW &&
			evtype < ARMV8_HISI_PERFCTR_EVENT_MAX) {
                //pr_info("Event index for DDR is %lx..\n", evtype);
		event_idx = find_first_zero_bit(hisi_ddr_event_used_mask,
				HISI_ARMV8_MAX_CFG_DDR_CNTR);

		if (event_idx == HISI_ARMV8_MAX_CFG_DDR_CNTR)
			return -EAGAIN;

		__set_bit(event_idx, hisi_ddr_event_used_mask);
		event_idx = event_idx + ARMV8_HISI_IDX_DDR_S0_TC_COUNTER0;
	}

	return event_idx;
}


int hisi_pmustub_enable_intens(int idx)
{
	return 0;
}

int hisi_pmustub_disable_intens(int idx)
{
	return 0;
}

int hisi_init_ddr_hw_perf_event(struct hw_perf_event *hwc)
{
	struct hisi_ddr_hwc_data_info *hisi_ddr_hwc_data;

       /* Create event counter local variables for storing DDR prev_count
	* cpu start and end_times */
        hisi_ddr_hwc_data = (struct hisi_ddr_hwc_data_info *)kzalloc(
                                        sizeof(struct hisi_ddr_hwc_data_info),
                                                                 GFP_ATOMIC);
        if (unlikely(!hisi_ddr_hwc_data)) {
                pr_err("Alloc failed for hisi hwc die data!.\n");
                return -ENOMEM;
        }

        hwc->perf_event_data = hisi_ddr_hwc_data;
	return 0;
}

/* Create variables to store previous count based on no of
*  banks and Dies */
int hisi_init_llc_hw_perf_event(struct hw_perf_event *hwc)
{
	struct hisi_llc_hwc_data_info *phisi_hwc_data_info = NULL;
	u32 num_banks = NUM_LLC_BANKS;

	/* Create event counter local variables for each bank to
	 * store the previous counter value */
	phisi_hwc_data_info = (struct hisi_llc_hwc_data_info *)kzalloc(
					sizeof(struct hisi_llc_hwc_data_info),
								 GFP_ATOMIC);
	if (unlikely(!phisi_hwc_data_info)) {
		pr_err("Alloc failed for hisi hwc die data!.\n");
		return -ENOMEM;
	}

	phisi_hwc_data_info->num_banks = num_banks;

	phisi_hwc_data_info->hwc_prev_counters =
		(struct hisi_hwc_prev_counter *)kzalloc(num_banks *
				sizeof(struct hisi_hwc_prev_counter),
								GFP_ATOMIC);
	if (unlikely(!phisi_hwc_data_info)) {
		pr_err("Alloc failed for hisi hwc die bank data!.\n");
		kfree(phisi_hwc_data_info);
		return -ENOMEM;
	}

	hwc->perf_event_data = phisi_hwc_data_info;

	return 0;
}

int hisi_init_mn_hw_perf_event(struct hw_perf_event *hwc)
{
	struct hisi_mn_hwc_data_info *phisi_hwc_data_info = NULL;

	/* Create event counter local64 variables for MN to
	 * store the previous counter value */
	phisi_hwc_data_info = (struct hisi_mn_hwc_data_info *)kzalloc(
			sizeof(struct hisi_mn_hwc_data_info),
			GFP_ATOMIC);
	if (unlikely(!phisi_hwc_data_info)) {
		pr_err("Alloc failed for hisi hwc die data!.\n");
		return -ENOMEM;
	}

	hwc->perf_event_data = phisi_hwc_data_info;
	return 0;
}


void hisi_set_llc_evtype(int idx, u32 val)
{
	u64 djtag_address;
	u32 reg_offset;
	u32 value = 0;
	u32 cfg_en;
	u32 llc_idx = 0;
	u32 counter_idx;
	u32 event_value;
	int i;

	if (idx >= ARMV8_HISI_IDX_LLC_S0_TA_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		/* handle for TotemA S0 */
		if (-1 != gs0_ta_llc_idx) {
			llc_idx = gs0_ta_llc_idx;
			counter_idx = idx - ARMV8_HISI_IDX_LLC_S0_TA_COUNTER0;
			event_value = (val -
				ARMV8_HISI_PERFCTR_LLC_S0_TA_READ_ALLOCATE);
		}
		else
			return;
	}
	else if (idx >= ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_LLC_S0_TC_COUNTER_MAX) {
		/* handle for TotemC S0 */
		if (-1 != gs0_tc_llc_idx) {
			llc_idx = gs0_tc_llc_idx;
			counter_idx = idx - ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0;
			event_value = (val -
				ARMV8_HISI_PERFCTR_LLC_S0_TC_READ_ALLOCATE);
		}
		else
			return;
	}
	else {
		pr_info("Unsupported event index:%d!\n", idx);
		return;
	}

	/* Select the appropriate Event select register */
	if (counter_idx <= 3)
		reg_offset = HISI_LLC_EVENT_TYPE0_REG_OFF;
	else
		reg_offset = HISI_LLC_EVENT_TYPE1_REG_OFF;

	/* Value to write to event type register */
	val = event_value << (8 * counter_idx);

        djtag_address = llc_data[llc_idx].djtag_reg_map;

	/* Set the event in LLC_EVENT_TYPEx Register
	 * for all LLC banks */
        for (i = 0; i < NUM_LLC_BANKS; i++) {
                cfg_en = llc_data[llc_idx].bank[i].cfg_en;

		hisi_djtag_readreg(HISI_LLC_BANK_MODULE_ID,
				cfg_en,
				reg_offset,
				djtag_address, &value);

		value &= ~(0xff << (8 * counter_idx));
		value |= val;

		hisi_djtag_writereg(HISI_LLC_BANK_MODULE_ID,
				cfg_en,
				reg_offset,
				value,
				djtag_address);
	}
}

void hisi_set_mn_evtype(int idx, u32 val) {
	u32 reg_offset;
	u32 value = 0;
	u32 mn_idx = 0;
	u32 counter_idx;
	u32 event_value;
	u64 djtag_address;

	if (idx >= ARMV8_HISI_IDX_MN_S0_TA_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		/* handle for TotemA S0 */
		if (-1 != gs0_ta_mn_idx) {
			mn_idx = gs0_ta_mn_idx;
			counter_idx = idx - ARMV8_HISI_IDX_MN_S0_TA_COUNTER0;
			event_value = (val -
				ARMV8_HISI_PERFCTR_MN_S0_TA_EO_BARR_REQ);
		}
		else
			return;
	}
	else if (idx >= ARMV8_HISI_IDX_MN_S0_TC_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_MN_S0_TC_COUNTER_MAX) {
		/* handle for TotemC S0 */
		if (-1 != gs0_tc_mn_idx) {
			mn_idx = gs0_tc_mn_idx;
			counter_idx = idx - ARMV8_HISI_IDX_MN_S0_TC_COUNTER0;
			event_value = (val -
				ARMV8_HISI_PERFCTR_MN_S0_TC_EO_BARR_REQ);
		}
		else
			return;
	}
	else {
		pr_info("Unsupported event index:%d!\n", idx);
		return;
	}

	/* Value to write to event type register */
	val = event_value << (8 * counter_idx);

	/* Find the djtag address of the Die */
        djtag_address = mn_data[mn_idx].djtag_reg_map;

	/* Set the event in MN_EVENT_TYPEx Register */
	reg_offset = HISI_MN_EVENT_TYPE;

	hisi_djtag_readreg(HISI_MN1_MODULE_ID,
			HISI_MN_MODULE_CFGEN,
			reg_offset,
			djtag_address, &value);

	value &= ~(0xff << (8 * counter_idx));
	value |= val;

	hisi_djtag_writereg(HISI_MN1_MODULE_ID,
			HISI_MN_MODULE_CFGEN,
			reg_offset,
			value,
			djtag_address);

	return;
}

void hisi_pmustub_write_evtype(int idx, u32 val) {
	val &= HISI_ARMV8_EVTYPE_EVENT;

	/* Select event based on Counter Module */
	if (ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0 <= idx &&
		 idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		hisi_set_llc_evtype(idx, val);
	}
	else if (ARMV8_HISI_IDX_MN_S0_TC_COUNTER0 <= idx &&
		 idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		hisi_set_mn_evtype(idx, val);
	}
        else if (ARMV8_HISI_IDX_DDR_S0_TC_COUNTER0 <= idx &&
                 idx <= ARMV8_HISI_IDX_DDR_COUNTER_MAX) {
        }
}

inline int armv8_hisi_counter_valid(int idx) {
	return (idx >= ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0 &&
			idx < ARMV8_HISI_IDX_COUNTER_MAX);
}

int hisi_enable_llc_counter(int idx)
{
	u32 num_banks = NUM_LLC_BANKS;
	u32 value = 0;
	u32 cfg_en;
	u32 llc_idx = 0;
	u64 djtag_address;
	int i, ret = 0;

	if (idx >= ARMV8_HISI_IDX_LLC_S0_TA_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		/* handle for TotemA S0 */
		if (-1 != gs0_ta_llc_idx)
			llc_idx = gs0_ta_llc_idx;
		else
			return -EINVAL;
	}
	else if (idx >= ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_LLC_S0_TC_COUNTER_MAX) {
		/* handle for TotemC S0 */
		if (-1 != gs0_tc_llc_idx)
			llc_idx = gs0_tc_llc_idx;
		else
			return -EINVAL;
	}

        djtag_address = llc_data[llc_idx].djtag_reg_map;
//        pr_info("djtag address is 0x%llx\n", djtag_address);

	/* Set the event_bus_en bit in LLC AUCNTRL to enable counting
	 * for all LLC banks */
	for (i = 0; i < num_banks; i++) {
		cfg_en = llc_data[llc_idx].bank[i].cfg_en;

		ret = hisi_djtag_readreg(HISI_LLC_BANK_MODULE_ID,
				cfg_en,
				HISI_LLC_AUCTRL_REG_OFF,
				djtag_address, &value);

		value |= HISI_LLC_AUCTRL_EVENT_BUS_EN;
		ret = hisi_djtag_writereg(HISI_LLC_BANK_MODULE_ID,
				cfg_en,
				HISI_LLC_AUCTRL_REG_OFF,
				value,
				djtag_address);
	}

	return ret;
}

int hisi_enable_mn_counter(int idx)
{
	u32 value = 0;
	u32 mn_idx = 0;
	u64 djtag_address;
	int ret;

	if (idx >= ARMV8_HISI_IDX_MN_S0_TA_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		/* handle for TotemA S0 */
		if (-1 != gs0_ta_mn_idx)
			mn_idx = gs0_ta_mn_idx;
		else
			return -EINVAL;
	}
	else if (idx >= ARMV8_HISI_IDX_MN_S0_TC_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_MN_S0_TC_COUNTER_MAX) {
		/* handle for TotemC S0 */
		if (-1 != gs0_tc_mn_idx)
			mn_idx = gs0_tc_mn_idx;
		else
			return -EINVAL;
	}
	else {
		pr_info("Unsupported event index:%d!\n", idx);
		return -EINVAL;
	}

        djtag_address = mn_data[mn_idx].djtag_reg_map;

	/* Set the event_en bit in MN_EVENT_CTRL to enable counting */
	ret = hisi_djtag_readreg(HISI_MN1_MODULE_ID,
			HISI_MN_MODULE_CFGEN,
			HISI_MN_EVENT_CTRL,
			djtag_address, &value);

	value |= HISI_MN_EVENT_EN;
	ret = hisi_djtag_writereg(HISI_MN1_MODULE_ID,
			HISI_MN_MODULE_CFGEN,
			HISI_MN_EVENT_CTRL,
			value,
			djtag_address);

	return ret;
}

int hisi_pmustub_enable_counter(int idx)
{
	int ret = 0;

	if (ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0 <= idx &&
			idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		ret = hisi_enable_llc_counter(idx);
	}
	else if (ARMV8_HISI_IDX_MN_S0_TC_COUNTER0 <= idx &&
			idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		ret = hisi_enable_mn_counter(idx);
	}
	else if (ARMV8_HISI_IDX_DDR_S0_TC_COUNTER0 <= idx &&
			idx <= ARMV8_HISI_IDX_DDR_COUNTER_MAX) {
		//pr_info("Enable counter for DDR idx=%d\n", idx);
	}

	/* FIXME: Comment out currently, we disable counting
	 * globally by LLC_AUCTRL as just writing 0 to counter
	 * to stop counting has some complexities */
	/*ret = hisi_pmustub_write_counter(idx, 0);*/

	return ret;
}

void hisi_disable_llc_counter(int idx)
{
	u32 num_banks = NUM_LLC_BANKS;
	u32 value = 0;
	u32 cfg_en;
	u32 llc_idx;
	u64 djtag_address;
	int i;

	if (idx >= ARMV8_HISI_IDX_LLC_S0_TA_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		/* handle for TotemA S0 */
		if (-1 != gs0_ta_llc_idx)
			llc_idx = gs0_ta_llc_idx;
		else
			return;
	}
	else if (idx >= ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_LLC_S0_TC_COUNTER_MAX) {
		/* handle for TotemC S0 */
		if (-1 != gs0_tc_llc_idx)
			llc_idx = gs0_tc_llc_idx;
		else
			return;
	}
	else {
		pr_info("Unsupported event index:%d!\n", idx);
		return;
	}

        djtag_address = llc_data[llc_idx].djtag_reg_map;
//        pr_info("djtag address is 0x%llx\n", djtag_address);

	/* Clear the event_bus_en bit in LLC AUCNTRL if no other event counting
	 * for all LLC banks */
	for (i = 0; i < num_banks; i++) {
		cfg_en = llc_data[llc_idx].bank[i].cfg_en;

		hisi_djtag_readreg(HISI_LLC_BANK_MODULE_ID,
				cfg_en,
				HISI_LLC_AUCTRL_REG_OFF,
				djtag_address, &value);

		value &= ~(HISI_LLC_AUCTRL_EVENT_BUS_EN);
		hisi_djtag_writereg(HISI_LLC_BANK_MODULE_ID,
				cfg_en,
				HISI_LLC_AUCTRL_REG_OFF,
				value,
				djtag_address);
	}
}

void hisi_disable_mn_counter(int idx)
{
	u32 value = 0;
	u32 mn_idx = 0;
	u64 djtag_address;

	if (idx >= ARMV8_HISI_IDX_MN_S0_TA_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		/* handle for TotemA S0 */
		if (-1 != gs0_ta_mn_idx)
			mn_idx = gs0_ta_mn_idx;
		else
			return;
	}
	else if (idx >= ARMV8_HISI_IDX_MN_S0_TC_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_MN_S0_TC_COUNTER_MAX) {
		/* handle for TotemC S0 */
		if (-1 != gs0_tc_mn_idx)
			mn_idx = gs0_tc_mn_idx;
		else
			return;
	}
	else {
		pr_info("Unsupported event index:%d!\n", idx);
		return;
	}

        djtag_address = mn_data[mn_idx].djtag_reg_map;

	/* Clear the event_bus_en bit in MN_EVENT_CTRL to disable counting */
	hisi_djtag_readreg(HISI_MN1_MODULE_ID,
			HISI_MN_MODULE_CFGEN,
			HISI_MN_EVENT_CTRL,
			djtag_address, &value);

	value |= ~HISI_MN_EVENT_EN;
	hisi_djtag_writereg(HISI_MN1_MODULE_ID,
			HISI_MN_MODULE_CFGEN,
			HISI_MN_EVENT_CTRL,
			value,
			djtag_address);
}


void hisi_pmustub_disable_counter(int idx) {

	/* Read the current counter value and set it for use in event_update.
	 * LLC_AUCTRL event_bus_en will not be cleared here.
	 * It will be kept ON if atleast 1 event counting is active. */
	/*	value = hisi_pmustub_read_counter(idx);*/
	/*	local64_set(&hwc->new_count, value);*/

	if (ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0 <= idx &&
		 idx <=	ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		hisi_disable_llc_counter(idx);
	}
	else if (ARMV8_HISI_IDX_MN_S0_TC_COUNTER0 <= idx &&
			idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		hisi_disable_mn_counter(idx);
	}
	else if (ARMV8_HISI_IDX_DDR_S0_TC_COUNTER0 <= idx &&
			idx <= ARMV8_HISI_IDX_DDR_COUNTER_MAX) {
		//pr_info("Disable counter for DDR idx=%d\n", idx);
	}
}

u32 hisi_read_llc_counter(int idx, u64 djtag_address, u32 cfg_en) {
	u32 reg_offset = 0;
	u32 value;

	reg_offset = HISI_LLC_COUNTER0_REG_OFF + (idx * 4);

	hisi_djtag_readreg(HISI_LLC_BANK_MODULE_ID, /* ModuleID  */
			cfg_en, /* Cfg_enable */
			reg_offset, /* Register Offset */
			djtag_address, &value);

	return value;
}

u32 hisi_read_mn_counter(int idx, u64 djtag_address, u32 cfg_en) {
	u32 reg_offset = 0;
	u32 value;

	reg_offset = HISI_MN_EVENT_CNT0 + (idx * 4);

	hisi_djtag_readreg(HISI_MN1_MODULE_ID, /* ModuleID  */
			cfg_en, /* Cfg_enable */
			reg_offset, /* Register Offset */
			djtag_address, &value);

	return value;
}

u64 hisi_read_ddr_counter(unsigned long evtype) {
	u32 value;
	u32 reg_offset = 0;
	u32 ddr_idx = 0;

	/* FIXME: This function code logic need improvement.
	 * Data structure to be optimized. */
	if (evtype >= ARMV8_HISI_PERFCTR_DDRC0_TA_FLUX_READ_BW &&
		evtype <= ARMV8_HISI_PERFCTR_DDRC0_TA_FLUX_WRITE_LAT) {
		/* handle for Totem A  DDRC0 */
		if (-1 != gta_ddr_idx)
			ddr_idx = gta_ddr_idx;
		else
			return 0;
	}
	else if (evtype >= ARMV8_HISI_PERFCTR_DDRC1_TA_FLUX_READ_BW &&
		evtype <= ARMV8_HISI_PERFCTR_DDRC1_TA_FLUX_WRITE_LAT) {
		/* handle for Totem A  DDRC1 */
		if (-1 != gta_ddr_idx)
			ddr_idx = gta_ddr_idx + 1;
		else
			return 0;
	}
	else if(evtype >= ARMV8_HISI_PERFCTR_DDRC0_TC_FLUX_READ_BW &&
		evtype <= ARMV8_HISI_PERFCTR_DDRC0_TC_FLUX_WRITE_LAT) {
		/* handle for Totem C  DDRC0 */
		if (-1 != gtc_ddr_idx)
			ddr_idx = gtc_ddr_idx;
		else
			return 0;
	}
	else if(evtype >= ARMV8_HISI_PERFCTR_DDRC1_TC_FLUX_READ_BW &&
		evtype <= ARMV8_HISI_PERFCTR_DDRC1_TC_FLUX_WRITE_LAT) {
		if (-1 != gtc_ddr_idx)
			ddr_idx = gtc_ddr_idx + 1;
		else
			return 0;
	}
	else {
		pr_err("Unknown DDR evevnt!");
		return 0;
	}

	switch (evtype) {
		case ARMV8_HISI_PERFCTR_DDRC1_TA_FLUX_READ_BW:
		case ARMV8_HISI_PERFCTR_DDRC0_TA_FLUX_READ_BW:
		case ARMV8_HISI_PERFCTR_DDRC1_TC_FLUX_READ_BW:
		case ARMV8_HISI_PERFCTR_DDRC0_TC_FLUX_READ_BW:
			reg_offset = HISI_DDRC_FLUX_RCMD;
			break;

		case ARMV8_HISI_PERFCTR_DDRC1_TA_FLUX_WRITE_BW:
		case ARMV8_HISI_PERFCTR_DDRC0_TA_FLUX_WRITE_BW:
		case ARMV8_HISI_PERFCTR_DDRC1_TC_FLUX_WRITE_BW:
		case ARMV8_HISI_PERFCTR_DDRC0_TC_FLUX_WRITE_BW:
			reg_offset = HISI_DDRC_FLUX_WCMD;
			break;

		case ARMV8_HISI_PERFCTR_DDRC1_TA_FLUX_READ_LAT:
		case ARMV8_HISI_PERFCTR_DDRC0_TA_FLUX_READ_LAT:
		case ARMV8_HISI_PERFCTR_DDRC1_TC_FLUX_READ_LAT:
		case ARMV8_HISI_PERFCTR_DDRC0_TC_FLUX_READ_LAT:
			reg_offset = HISI_DDRC_FLUX_RLAT_CNT1;
			break;

		case ARMV8_HISI_PERFCTR_DDRC1_TA_FLUX_WRITE_LAT:
		case ARMV8_HISI_PERFCTR_DDRC0_TA_FLUX_WRITE_LAT:
		case ARMV8_HISI_PERFCTR_DDRC1_TC_FLUX_WRITE_LAT:
		case ARMV8_HISI_PERFCTR_DDRC0_TC_FLUX_WRITE_LAT:
			reg_offset = HISI_DDRC_FLUX_WLATCNT1;
			break;

		default :
			pr_info("unknown DDR evevnt");
			return 0;
	}

	DDR_REG_READ(ddr_data[ddr_idx].ddrc_reg_map + reg_offset, value);
	//pr_info("-VALUE READ for evtype=%lx from ofset:%d IS %d-\n",
	//					evtype, reg_offset, value);
	return (value);
}

u32 hisi_pmustub_read_counter(int idx)
{
	int ret = 0;

        if (ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0 <= idx &&
                        idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
        }
        else if (ARMV8_HISI_IDX_MN_S0_TC_COUNTER0 <= idx &&
                        idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
        }
        else if (ARMV8_HISI_IDX_DDR_S0_TC_COUNTER0 <= idx &&
                        idx <= ARMV8_HISI_IDX_DDR_COUNTER_MAX) {
                //pr_info("Read counter for DDR idx=%d\n", idx);
        }

	return ret;
}

u32 hisi_write_llc_counter(int idx, u32 value) {
	u32 num_banks = NUM_LLC_BANKS;
        u32 cfg_en;
        u32 reg_offset = 0;
        u64 djtag_address;
	u32 llc_idx;
        int i, counter_idx, ret = 0;

	if (idx >= ARMV8_HISI_IDX_LLC_S0_TA_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		/* handle for TotemA S0 */
		if (-1 != gs0_ta_llc_idx) {
			llc_idx = gs0_ta_llc_idx;
			counter_idx = idx - ARMV8_HISI_IDX_LLC_S0_TA_COUNTER0;
		}
		else
			return 0;
	}
	else if (idx >= ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_LLC_S0_TC_COUNTER_MAX) {
		/* handle for TotemC S0 */
		if (-1 != gs0_tc_llc_idx) {
			llc_idx = gs0_tc_llc_idx;
			counter_idx = idx - ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0;
		}
		else
			return 0;
	}
	else {
		pr_info("Unsupported event index:%d!\n", idx);
		return 0;
	}

	reg_offset = HISI_LLC_COUNTER0_REG_OFF +
					(counter_idx * 4);

        djtag_address = llc_data[llc_idx].djtag_reg_map;
//        pr_info("djtag address is 0x%llx\n", djtag_address);

	for (i = 0; i < num_banks; i++) {
		cfg_en = llc_data[llc_idx].bank[i].cfg_en;
		ret =
			hisi_djtag_writereg(HISI_LLC_BANK_MODULE_ID,
					cfg_en,
					reg_offset,
					value,
					djtag_address);
	}

	return ret;
}

u32 hisi_write_mn_counter(int idx, u32 value) {
        u32 reg_offset = 0;
        u64 djtag_address;
	u32 mn_idx;
        int counter_idx, ret = 0;

	if (idx >= ARMV8_HISI_IDX_MN_S0_TA_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		/* handle for TotemA S0 */
		if (-1 != gs0_ta_mn_idx) {
			mn_idx = gs0_ta_mn_idx;
			counter_idx = idx - ARMV8_HISI_IDX_MN_S0_TA_COUNTER0;
		}
		else
			return 0;
	}
	else if (idx >= ARMV8_HISI_IDX_MN_S0_TC_COUNTER0 &&
		idx <= ARMV8_HISI_IDX_MN_S0_TC_COUNTER_MAX) {
		/* handle for TotemC S0 */
		if (-1 != gs0_tc_mn_idx) {
			mn_idx = gs0_tc_mn_idx;
			counter_idx = idx - ARMV8_HISI_IDX_MN_S0_TC_COUNTER0;
		}
		else
			return 0;
	}
	else {
		pr_info("Unsupported event index:%d!\n", idx);
		return 0;
	}

	reg_offset = HISI_MN_EVENT_CNT0 + (counter_idx * 4);

        djtag_address = mn_data[mn_idx].djtag_reg_map;

	ret =
		hisi_djtag_writereg(HISI_MN1_MODULE_ID,
				HISI_MN_MODULE_CFGEN,
				reg_offset,
				value,
				djtag_address);

	return ret;
}

int hisi_pmustub_write_counter(int idx, u32 value)
{
	if (ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0 <= idx &&
			idx <= ARMV8_HISI_IDX_LLC_COUNTER_MAX) {
		hisi_write_llc_counter(idx, value);
	}
	else if (ARMV8_HISI_IDX_MN_S0_TC_COUNTER0 <= idx &&
			idx <= ARMV8_HISI_IDX_MN_COUNTER_MAX) {
		hisi_write_mn_counter(idx, value);
	}
	else if (ARMV8_HISI_IDX_DDR_S0_TC_COUNTER0 <= idx &&
			idx <= ARMV8_HISI_IDX_DDR_COUNTER_MAX) {
		//pr_info("Write_couner for DDR idx=%d\n", idx);
	}

	return value;
}

irqreturn_t hisi_llc_event_handle_irq(int irq_num, void *dev)
{
	u64 djtag_address;
	u32 num_banks = NUM_LLC_BANKS;
	u32 cfg_en;
	u32 value;
	u32 bit_value;
	u32 counter_idx;
	int llc_idx;
	int bit_pos;
	int i = 0;

	/* Identify the bank and the ITS Register */
	for (llc_idx = 0; llc_idx < gnum_llc; llc_idx++) {
		for (i = 0; i < num_banks; i++) {
			if (irq_num ==
					llc_data[llc_idx].bank[i].irq) {
				//pr_info("irq:%d matched with bank:%d\n",
				//				 irq_num, i);
				break;
			}
		}
		if (num_banks != i)
			break;
	}

	if (i == num_banks)
		return IRQ_NONE;

	/* Find the djtag address of the Die */
	djtag_address =
		llc_data[llc_idx].djtag_reg_map;
	//pr_info("djtag address=%llx\n", djtag_address);

	cfg_en = llc_data[llc_idx].bank[i].cfg_en;

	/* Read the staus register if any bit is set */
	hisi_djtag_readreg(HISI_LLC_BANK_MODULE_ID,
			cfg_en,
			HISI_LLC_BANK_INTS,
			djtag_address, &value);
	//pr_info("Value read for cfg_en=0x%x is 0x%x\n",
	//		cfg_en, value);

	/* Find the bits sets and clear them */
	for (bit_pos = 0; bit_pos <= HISI_ARMV8_MAX_CFG_LLC_CNTR; bit_pos++)
	{
		if(test_bit(bit_pos, &value)) {
			//pr_info("Bit %d is set\n", bit_pos);

			/* Reset the IRQ status flag */
			value &= ~bit_pos;

			hisi_djtag_writereg(HISI_LLC_BANK_MODULE_ID,
					cfg_en,
					HISI_LLC_BANK_INTC,
					value,
					djtag_address);

			/* Read the staus register to confirm */
			hisi_djtag_readreg(HISI_LLC_BANK_MODULE_ID,
					cfg_en,
					HISI_LLC_BANK_INTS,
					djtag_address, &value);
			//pr_info("Value read for cfg_en=0x%x after reset:0x%x\n",
			//		cfg_en, value);

			/* Find the counter_idx */
			if (llc_idx == gs0_tc_llc_idx)
				counter_idx = bit_pos + ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0;
			else if (llc_idx == gs0_tc_llc_idx)
				counter_idx = bit_pos + ARMV8_HISI_IDX_LLC_S0_TA_COUNTER0;
			else
				return IRQ_NONE;

			/* Update overflow times to refer in event_update */
			bit_value = atomic_read(&llc_data[llc_idx].bank[i].cntr_ovflw[bit_pos]);
			//pr_info("The counter overflow value read is %d\n", bit_value); 
			atomic_inc(&llc_data[llc_idx].bank[i].cntr_ovflw[bit_pos]);
		}
	}

        /* Event set new period */

        /* Handle event overflow updation */

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
		pr_err("Invalid IRQ numbers in dts.\n");
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
				"hisi-pmu-stub", pllc_data);
		if (ret) {
			pr_err("unable to request IRQ%d for HISI PMU" \
				"Stub counters\n", irq);
			goto err;
		}
		//pr_info("IRQ:%d assigned to bank:%d\n", irq, i);
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

static void llc_update_index(const char *name, int idx) {
	if (strstr(name, HISI_LLC_S0_TOTEMA_DTS_NAME)) {
		if (-1 == gs0_ta_llc_idx)
			gs0_ta_llc_idx = idx;
	}
	else if (strstr(name, HISI_LLC_S1_TOTEMA_DTS_NAME)) {
		if (-1 == gs1_ta_llc_idx)
			gs1_ta_llc_idx = idx;
	}
	else if(strstr(name, HISI_LLC_S0_TOTEMC_DTS_NAME)) {
		if (-1 == gs0_tc_llc_idx)
			gs0_tc_llc_idx = idx;
	}
	else if(strstr(name, HISI_LLC_S1_TOTEMC_DTS_NAME)) {
		if (-1 == gs1_tc_llc_idx)
			gs1_tc_llc_idx = idx;
	}
}

static u64 ioremap_djtag_tmp(u64 djtag_base_addr, u32 size)
{
	u64 djtag_address;

	djtag_address = (u64)ioremap_nocache(djtag_base_addr, size);
	return djtag_address;
}

static int init_hisi_llc_data(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct of_phandle_args arg;
	struct regmap *djtag_map;
	struct resource res;
	int ret, i, die, llc_idx;

	for (i = 0; i < MAX_DIE; i++) {
		ret = of_parse_phandle_with_fixed_args(node,
				"system-controller", 1, i, &arg);
		if (!ret) {
			if (arg.args[0] > 0  && arg.args[0] <= MAX_DIE ) {
				die = arg.args[0];
			}
			else
				return -EINVAL;
		}
		else
			continue;

		djtag_map = syscon_node_to_regmap(arg.np);
		if (IS_ERR(djtag_map)) {
			dev_err(dev, "Cant read djtag addr map - hisilicon,\
							hip05-sysctrl\n");
			return -EINVAL;
		}

		/* Temporary change till djtag interface changed to regmap_write */
		if (of_address_to_resource(arg.np, 0, &res)) {
			pr_err("Unable to read Djtag Address!\n");
			return -EINVAL;
		}

                //pr_info("res->name=%s res->start is 0x%llx size=0x%x ..\n",
                //                res.name, res.start, resource_size(&res));

		llc_idx = die -1;
		if (!llc_data[llc_idx].djtag_reg_map) {
			llc_data[llc_idx].djtag_reg_map =
				ioremap_djtag_tmp(res.start,
						resource_size(&res));
			soc_djtag_regmap[llc_idx].djtag_reg_map = djtag_map;
			llc_data[llc_idx].reg_map = &soc_djtag_regmap[llc_idx];

			ret = init_hisi_llc_banks(&llc_data[llc_idx], pdev);
			if (ret)
				return ret;

			llc_update_index(node->name, llc_idx);
		}
	}
	gnum_llc++;

	return 0;
}

void hisi_armv8_pmustub_init(void)
{
	/* Init All event used masks */
	memset(hisi_llc_s0_tc_event_used_mask,
			0, sizeof(hisi_llc_s0_tc_event_used_mask));
	memset(hisi_llc_s0_ta_event_used_mask, 0,
				sizeof(hisi_llc_s0_ta_event_used_mask));
	memset(hisi_mn_s0_tc_event_used_mask, 0,
				sizeof(hisi_mn_s0_tc_event_used_mask));
	memset(hisi_mn_s0_ta_event_used_mask, 0,
				sizeof(hisi_mn_s0_ta_event_used_mask));
	memset(hisi_ddr_event_used_mask, 0,
					sizeof(hisi_ddr_event_used_mask));

	/* Enable counting during Init to avoid stop counting */
	/* FIXME: comment out currently due to some complexities */
	/* hisi_armv8_pmustub_enable_counting(); */
}

static int hisi_pmu_stub_llc_dev_probe(struct platform_device *pdev) {
	int ret;

	ret = init_hisi_llc_data(pdev);
	if(ret)
		return ret;

	return 0;
}

static int hisi_pmu_stub_llc_dev_remove(struct platform_device *pdev) {
	int i;

	for (i = 0; i < MAX_DIE; i++) {
		if (llc_data[i].djtag_reg_map)
			iounmap((void *)llc_data[i].djtag_reg_map);
	}

	return 0;
}

static struct of_device_id llc_of_match[] = {
	{.compatible = "hisilicon,hip05-llc",},
	{},
};
MODULE_DEVICE_TABLE(of, llc_of_match);

static struct platform_driver hisi_pmu_stub_llc_driver = {
	.driver = {
		.name = "hip05-llc",
		.of_match_table = llc_of_match,
	},
	.probe = hisi_pmu_stub_llc_dev_probe,
	.remove = hisi_pmu_stub_llc_dev_remove,
};
module_platform_driver(hisi_pmu_stub_llc_driver);

static void ddr_update_index(const char *name, int idx) {
	if (strstr(name, HISI_DDRC_TOTEMA_DTS_NAME)) {
		if (-1 == gta_ddr_idx)
			gta_ddr_idx = idx;
	}
	else if(strstr(name, HISI_DDRC_TOTEMC_DTS_NAME)) {
		if (-1 == gtc_ddr_idx)
			gtc_ddr_idx = idx;
	}
	else if(strstr(name, HISI_DDRC_TOTEMB_DTS_NAME)) {
		if (-1 == gtb_ddr_idx)
			gtb_ddr_idx = idx;
	}
}

static void init_hisi_ddr(u64 ulddrc_baseaddr) {
        u32 value;

	DDR_REG_WRITE(ulddrc_baseaddr + HISI_DDRC_CTRL_PERF, 0);
	DDR_REG_READ(ulddrc_baseaddr + HISI_DDRC_CTRL_PERF, value);
	//pr_info("-- Set DDRC0_CTRL_PERF in 0x%llx to 0x%x\n",
	//		ulddrc_baseaddr, value);

	DDR_REG_READ(ulddrc_baseaddr + HISI_DDRC_CFG_PERF, value);
	value = value & 0x2fffffff;

	DDR_REG_WRITE(ulddrc_baseaddr + HISI_DDRC_CFG_PERF, value);
	//pr_info("-- Set DDRC0_CFG_PERF to (value & 0x2fffffff) 0x%x\n", value);

	DDR_REG_WRITE(ulddrc_baseaddr + HISI_DDRC_CTRL_PERF, 1);

	DDR_REG_READ(ulddrc_baseaddr + HISI_DDRC_CTRL_PERF, value);
	//pr_info("--Set DDRC0_CTRL_PERF to 0x%x\n", value);
}

static int init_hisi_ddr_dts_data(struct platform_device *pdev) {
	struct resource *res;
	int i, num_ddrs = 0;

	/* Get the number of DDRC's supported in a Totem */
	while ((res = platform_get_resource(pdev,
					IORESOURCE_MEM, num_ddrs))) {
		num_ddrs++;
	}

	for (i = 0; i < num_ddrs; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		//pr_info("res->name=%s res->start is 0x%llx size=0x%x ..\n",
		//		res->name, res->start, resource_size(res));
		ddr_data[gnum_ddrs].ddrc_reg_map =
				(u64)ioremap_nocache(res->start,
						 resource_size(res));
		if (!ddr_data[gnum_ddrs].ddrc_reg_map) {
			pr_info("ioremap failed for DDR!\n");
			goto err;
		}

		ddr_update_index(res->name, gnum_ddrs);
		init_hisi_ddr(ddr_data[gnum_ddrs].ddrc_reg_map);
		gnum_ddrs++;
	}

	return 0;
err:
	/* FIXME: DO I need to iounmap here?
	 * I didnot update the Index for fail so may be not */
	return -1;
}

static int hisi_pmu_stub_ddr_dev_probe(struct platform_device *pdev) {
	int ret;

        ret = init_hisi_ddr_dts_data(pdev);
        if(ret)
                return ret;

        return 0;
}

static int hisi_pmu_stub_ddr_dev_remove(struct platform_device *pdev) {
	int i;

	for (i = 0; i < gnum_ddrs; i++) {
		if (ddr_data[i].ddrc_reg_map)
			iounmap((void *)ddr_data[i].ddrc_reg_map);
	}

        return 0;
}

static struct of_device_id ddr_of_match[] = {
        {.compatible = "hisilicon,hip05-ddr",},
        {},
};
MODULE_DEVICE_TABLE(of, ddr_of_match);

static struct platform_driver hisi_pmu_stub_ddr_driver = {
        .driver = {
                .name = "hip05-ddr",
                .of_match_table = ddr_of_match,
        },
        .probe = hisi_pmu_stub_ddr_dev_probe,
        .remove = hisi_pmu_stub_ddr_dev_remove,
};
module_platform_driver(hisi_pmu_stub_ddr_driver);

static void mn_update_index(const char *name, int idx) {
        if (strstr(name, HISI_MN_S0_TOTEMA_DTS_NAME)) {
                if (-1 == gs0_ta_mn_idx)
                        gs0_ta_mn_idx = idx;
        }
        else if (strstr(name, HISI_MN_S1_TOTEMA_DTS_NAME)) {
                if (-1 == gs1_ta_mn_idx)
                        gs1_ta_mn_idx = idx;
        }
        else if(strstr(name, HISI_MN_S0_TOTEMC_DTS_NAME)) {
                if (-1 == gs0_tc_mn_idx)
                        gs0_tc_mn_idx = idx;
        }
        else if(strstr(name, HISI_MN_S1_TOTEMC_DTS_NAME)) {
                if (-1 == gs1_tc_mn_idx)
                        gs0_tc_mn_idx = idx;
	}

}

static int init_hisi_mn_data(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct of_phandle_args arg;
	struct regmap *djtag_map;
	struct resource res;
	int ret, i, die, mn_idx;

	for (i = 0; i < MAX_DIE; i++) {
		ret = of_parse_phandle_with_fixed_args(node,
				"system-controller", 1, i, &arg);
		if (!ret) {
			if (arg.args[0] > 0  && arg.args[0] <= MAX_DIE )
				die = arg.args[0];
			else
				return -EINVAL;
		}
		else
			continue;

		djtag_map = syscon_node_to_regmap(arg.np);
		if (IS_ERR(djtag_map)) {
			dev_err(dev, "Cant read djtag addr map - hisilicon,\
							hip05-sysctrl\n");
			return -EINVAL;
		}

                /* Temporary change till djtag interface changed to regmap_write */
                if (of_address_to_resource(arg.np, 0, &res)) {
                        pr_err("Unable to read Djtag Address!\n");
                        return -EINVAL;
                }

                //pr_info("res->name=%s res->start is 0x%llx size=0x%x ..\n",
                //                res.name, res.start, resource_size(&res));

		mn_idx = die - 1;
		if (!mn_data[mn_idx].djtag_reg_map) {
			mn_data[mn_idx].djtag_reg_map =
				ioremap_djtag_tmp(res.start,
						resource_size(&res));
                        mn_update_index(node->name, mn_idx);
		}
	}

	return 0;
}

static int hisi_pmu_stub_mn_dev_probe(struct platform_device *pdev) {
        int ret;

        ret = init_hisi_mn_data(pdev);

        return ret;
}

static int hisi_pmu_stub_mn_dev_remove(struct platform_device *pdev) {
	int i;

	for (i = 0; i < MAX_DIE; i++) {
		if (mn_data[i].djtag_reg_map)
			iounmap((void *)mn_data[i].djtag_reg_map);
	}

        return 0;
}

static struct of_device_id mn_of_match[] = {
	{.compatible = "hisilicon,hip05-mn",},
	{},
};
MODULE_DEVICE_TABLE(of, mn_of_match);

static struct platform_driver hisi_pmu_stub_mn_driver = {
	.driver = {
		.name = "hip05-mn",
		.of_match_table = mn_of_match,
	},
	.probe = hisi_pmu_stub_mn_dev_probe,
	.remove = hisi_pmu_stub_mn_dev_remove,
};
module_platform_driver(hisi_pmu_stub_mn_driver);


MODULE_DESCRIPTION("HiSilicon PMU stub driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Anurup M");
