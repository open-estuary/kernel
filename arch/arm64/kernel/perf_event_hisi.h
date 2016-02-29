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

/*
 * ARMv8 HiSilicon LLC RAW event types.
 */
enum armv8_hisi_llc_event_types {
	ARMV8_HISI_PERFCTR_LLC_READ_ALLOCATE		= 0x300,
	ARMV8_HISI_PERFCTR_LLC_WRITE_ALLOCATE		= 0x301,
	ARMV8_HISI_PERFCTR_LLC_READ_NOALLOCATE		= 0x302,
	ARMV8_HISI_PERFCTR_LLC_WRITE_NOALLOCATE		= 0x303,
	ARMV8_HISI_PERFCTR_LLC_READ_HIT				= 0x304,
	ARMV8_HISI_PERFCTR_LLC_WRITE_HIT			= 0x305,
	ARMV8_HISI_PERFCTR_LLC_CMO_REQUEST			= 0x306,
	ARMV8_HISI_PERFCTR_LLC_COPYBACK_REQ			= 0x307,
	ARMV8_HISI_PERFCTR_LLC_HCCS_SNOOP_REQ		= 0x308,
	ARMV8_HISI_PERFCTR_LLC_SMMU_REQ				= 0x309,
	ARMV8_HISI_PERFCTR_LLC_EXCL_SUCCESS			= 0x30A,
	ARMV8_HISI_PERFCTR_LLC_EXCL_FAIL			= 0x30B,
	ARMV8_HISI_PERFCTR_LLC_CACHELINE_OFLOW		= 0x30C,
	ARMV8_HISI_PERFCTR_LLC_RECV_ERR				= 0x30D,
	ARMV8_HISI_PERFCTR_LLC_RECV_PREFETCH		= 0x30E,
	ARMV8_HISI_PERFCTR_LLC_RETRY_REQ			= 0x30F,
	ARMV8_HISI_PERFCTR_LLC_DGRAM_2B_ECC			= 0x310,
	ARMV8_HISI_PERFCTR_LLC_TGRAM_2B_ECC			= 0x311,
	ARMV8_HISI_PERFCTR_LLC_SPECULATE_SNOOP		= 0x312,
	ARMV8_HISI_PERFCTR_LLC_SPECULATE_SNOOP_SUCCESS	= 0x313,
	ARMV8_HISI_PERFCTR_LLC_TGRAM_1B_ECC		= 0x314,
	ARMV8_HISI_PERFCTR_LLC_DGRAM_1B_ECC		= 0x315,
};

/*
 * ARMv8 HiSilicon MN RAW event types.
 */
enum armv8_hisi_mn_event_types {
	ARMV8_HISI_PERFCTR_MN_EO_BARR_REQ		= 0x316,
	ARMV8_HISI_PERFCTR_MN_EC_BARR_REQ		= 0x317,
	ARMV8_HISI_PERFCTR_MN_DVM_OP_REQ		= 0x318,
	ARMV8_HISI_PERFCTR_MN_DVM_SYNC_REQ		= 0x319,
	ARMV8_HISI_PERFCTR_MN_READ_REQ			= 0x31A,
	ARMV8_HISI_PERFCTR_MN_WRITE_REQ			= 0x31B,
	ARMV8_HISI_PERFCTR_MN_COPYBK_REQ		= 0x31C,
	ARMV8_HISI_PERFCTR_MN_OTHER_REQ			= 0x31D,
	ARMV8_HISI_PERFCTR_MN_RETRY_REQ			= 0x31E,
	ARMV8_HISI_PERFCTR_EVENT_MAX,
};

/*
 * ARMv8 HiSilicon Hardware counter Index.
 */
enum armv8_hisi_hw_counters {
	ARMV8_HISI_IDX_LLC_COUNTER0			= 0x30,
	ARMV8_HISI_IDX_LLC_COUNTER_MAX		= 0x37,
	ARMV8_HISI_IDX_MN_COUNTER0			= 0x38,
	ARMV8_HISI_IDX_MN_COUNTER_MAX		= 0x3B,
	ARMV8_HISI_IDX_DDRC0_COUNTER0		= 0x3C,
	ARMV8_HISI_IDX_DDRC0_COUNTER_MAX	= 0x3F,
	ARMV8_HISI_IDX_DDRC1_COUNTER0		= 0x40,
	ARMV8_HISI_IDX_DDRC1_COUNTER_MAX	= 0x43,
	ARMV8_HISI_IDX_COUNTER_MAX,
};

/*
 * ARMv8 HiSilicon SoC CPu Die Names.
 */
enum hisi_die_id {
	HISI_SOC0_TOTEMA = 1,
	HISI_SOC0_TOTEMC,
	HISI_SOC0_TOTEMB,
	HISI_SOC1_TOTEMA,
	HISI_SOC1_TOTEMC,
	HISI_SOC1_TOTEMB,
};

#define HISI_CNTR_DIE_MASK	(0xF00)

#define HISI_HW_DIE_MAX	(1 << 4)
#define HISI_HW_DIE_MASK	(0xF00000)

#define HISI_HW_MODULE_MAX	(1 << 4)
#define HISI_HW_MODULE_MASK	(0xF0000)

#define HISI_HW_SUB_MODULE_MAX		(1 << 4)
#define HISI_HW_SUB_MODULE_MASK	(0xF000)

#define HISI_ARMV8_MAX_CFG_EVENTS_MAX (1 << 12)
#define HISI_ARMV8_MAX_CFG_EVENTS_MASK 0xfff

#define HISI_ARMV8_MAX_PERIOD ((1LLU << 32) - 1)

#define HISI_ARMV8_EVTYPE_EVENT	0x3ff
#define HISI_ARMV8_MAX_CFG_LLC_CNTR	0x08
#define HISI_ARMV8_MAX_CFG_MN_CNTR 0x04

/* HW perf modules supported index */
#define HISI_LLC_MODULE_ID	0x04
#define HISI_MN1_MODULE_ID	0x0B

#define HISI_LLC_BANK0_CFGEN  0x02
#define HISI_LLC_BANK1_CFGEN  0x04
#define HISI_LLC_BANK2_CFGEN  0x01
#define HISI_LLC_BANK3_CFGEN  0x08
#define HISI_MN_MODULE_CFGEN  0x01

#define HISI_LLC_AUCTRL_REG_OFF 0x04
#define HISI_LLC_AUCTRL_EVENT_BUS_EN 0x1000000

#define HISI_LLC_EVENT_TYPE0_REG_OFF 0x140
#define HISI_LLC_EVENT_TYPE1_REG_OFF 0x144

#define HISI_LLC_COUNTER0_REG_OFF 0x170
#define HISI_LLC_COUNTER1_REG_OFF 0x174
#define HISI_LLC_COUNTER2_REG_OFF 0x178
#define HISI_LLC_COUNTER3_REG_OFF 0x17C
#define HISI_LLC_COUNTER4_REG_OFF 0x180
#define HISI_LLC_COUNTER5_REG_OFF 0x184
#define HISI_LLC_COUNTER6_REG_OFF 0x188
#define HISI_LLC_COUNTER7_REG_OFF 0x18C

#define HISI_LLC_BANK_INTM 0x100
#define HISI_LLC_BANK_RINT 0x104
#define HISI_LLC_BANK_INTS 0x108
#define HISI_LLC_BANK_INTC 0x10C

#define HISI_LLC_MAX_EVENTS 22

#define HISI_MN_MAX_EVENTS 9
#define HISI_MN_EVENT_CNT0	0x30
#define HISI_MN_EVENT_CNT1	0x34
#define HISI_MN_EVENT_CNT2	0x38
#define HISI_MN_EVENT_CNT3	0x3c
#define HISI_MN_EVENT_CTRL	0x40
#define HISI_MN_EVENT_TYPE	0x48
#define HISI_MN_EVENT_EN	0x1

#define MAX_BANKS 8
#define NUM_LLC_BANKS 4
#define MAX_DIE 8

struct hisi_hwc_prev_counter {
	local64_t prev_count;
};

struct hisi_mn_hwc_data_info {
	local64_t event_start_count;
};
 
struct hisi_llc_hwc_data_info {
	u32 num_banks;
	struct hisi_hwc_prev_counter *hwc_prev_counters;
};

typedef struct bank_info_t {
	u32 cfg_en;
	int irq;
	/* Overflow counter for each cnt idx */
	atomic_t cntr_ovflw[HISI_ARMV8_MAX_CFG_LLC_CNTR];
} bank_info;

typedef struct hisi_llc_data_t {
	struct device_node *djtag_node;
	DECLARE_BITMAP(hisi_llc_event_used_mask,
				HISI_ARMV8_MAX_CFG_LLC_CNTR);
	u32 num_banks;
	bank_info bank[MAX_BANKS];
} hisi_llc_data;

typedef struct hisi_mn_data_t {
	struct device_node *djtag_node;
	DECLARE_BITMAP(hisi_mn_event_used_mask,
				HISI_ARMV8_MAX_CFG_MN_CNTR);
} hisi_mn_data;

u64 hisi_pmu_event_update(struct perf_event *,
					struct hw_perf_event *, int);
u64 hisi_llc_event_update(struct perf_event *,
				struct hw_perf_event *, int);
u64 hisi_mn_event_update(struct perf_event *,
				struct hw_perf_event *, int);
int hisi_pmu_enable_counter(int);
void hisi_pmu_disable_counter(int);
int armv8_hisi_counter_valid(int);
int hisi_pmu_write_counter(int, u32);
u32 hisi_pmu_read_counter(int);
void hisi_pmu_write_evtype(int, u32);
int hisi_pmu_enable_intens(int);
int hisi_pmu_disable_intens(int);
int hisi_pmu_get_event_idx(struct hw_perf_event *);
void hisi_pmu_clear_event_idx(int);
void hisi_pmu_enable_counting(void);
void hisi_set_llc_evtype(int, u32);
void hisi_set_mn_evtype(int, u32);
u32 hisi_read_llc_counter(int, struct device_node *, int);
u32 hisi_read_mn_counter(int, struct device_node *, int);
int hisi_init_llc_hw_perf_event(struct hw_perf_event *);
int hisi_init_mn_hw_perf_event(struct hw_perf_event *);
irqreturn_t hisi_llc_event_handle_irq(int, void *);
