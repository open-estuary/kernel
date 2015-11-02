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
u64 hisi_armv8_pmustub_event_update(struct perf_event *,
					struct hw_perf_event *, int);
u64 hisi_armv8_ddr_update_start_value(struct perf_event *,
					struct hw_perf_event *, int);
u64 hisi_llc_event_update(struct perf_event *,
                                 struct hw_perf_event *, int);
u64 hisi_ddr_event_update(struct perf_event *,
                                 struct hw_perf_event *, int);
u64 hisi_mn_event_update(struct perf_event *,
                                 struct hw_perf_event *, int);

int hisi_pmustub_enable_counter(int);
void hisi_pmustub_disable_counter(int);
int armv8_hisi_counter_valid(int);
int hisi_pmustub_write_counter(int, u32);
u32 hisi_pmustub_read_counter(int);
void hisi_pmustub_write_evtype(int, u32);
int hisi_pmustub_enable_intens(int);
int hisi_pmustub_disable_intens(int);
int hisi_armv8_pmustub_get_event_idx(unsigned long);
int hisi_armv8_pmustub_clear_event_idx(int);
void hisi_armv8_pmustub_enable_counting(void);
void hisi_armv8_pmustub_init(void);
void hisi_set_llc_evtype(int, u32);
void hisi_set_mn_evtype(int, u32);
u32 hisi_read_llc_counter(int, u64, u32);
u32 hisi_read_mn_counter(int, u64, u32);
u64 hisi_read_ddr_counter(unsigned long);
int hisi_init_llc_hw_perf_event(struct hw_perf_event *);
int hisi_init_ddr_hw_perf_event(struct hw_perf_event *);
int hisi_init_mn_hw_perf_event(struct hw_perf_event *hwc);
u64 hisi_armv8_ddr_update_start_value(struct perf_event *,
					struct hw_perf_event *,
							int);
irqreturn_t hisi_llc_event_handle_irq(int, void *);

/*
 * ARMv8 HiSilicon LLC RAW event types.
 */
enum armv8_hisi_llc_event_types {
	ARMV8_HISI_PERFCTR_LLC_S0_TC_READ_ALLOCATE		= 0x300,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_WRITE_ALLOCATE		= 0x301,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_READ_NOALLOCATE		= 0x302,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_WRITE_NOALLOCATE		= 0x303,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_READ_HIT			= 0x304,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_WRITE_HIT			= 0x305,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_CMO_REQUEST		= 0x306,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_COPYBACK_REQ		= 0x307,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_HCCS_SNOOP_REQ		= 0x308,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_SMMU_REQ			= 0x309,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_EXCL_SUCCESS		= 0x30A,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_EXCL_FAIL			= 0x30B,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_CACHELINE_OFLOW		= 0x30C,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_RECV_ERR			= 0x30D,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_RECV_PREFETCH		= 0x30E,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_RETRY_REQ			= 0x30F,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_DGRAM_2B_ECC		= 0x310,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_TGRAM_2B_ECC		= 0x311,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_SPECULATE_SNOOP		= 0x312,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_SPECULATE_SNOOP_SUCCESS	= 0x313,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_TGRAM_1B_ECC		= 0x314,
	ARMV8_HISI_PERFCTR_LLC_S0_TC_DGRAM_1B_ECC		= 0x315,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_READ_ALLOCATE		= 0x316,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_WRITE_ALLOCATE		= 0x317,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_READ_NOALLOCATE		= 0x318,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_WRITE_NOALLOCATE		= 0x319,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_READ_HIT			= 0x31A,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_WRITE_HIT			= 0x31B,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_CMO_REQUEST		= 0x31C,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_COPYBACK_REQ		= 0x31D,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_HCCS_SNOOP_REQ		= 0x31E,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_SMMU_REQ			= 0x31F,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_EXCL_SUCCESS		= 0x320,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_EXCL_FAIL			= 0x321,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_CACHELINE_OFLOW		= 0x322,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_RECV_ERR			= 0x323,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_RECV_PREFETCH		= 0x324,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_RETRY_REQ			= 0x325,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_DGRAM_2B_ECC		= 0x326,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_TGRAM_2B_ECC		= 0x327,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_SPECULATE_SNOOP		= 0x328,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_SPECULATE_SNOOP_SUCCESS	= 0x329,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_TGRAM_1B_ECC		= 0x32A,
	ARMV8_HISI_PERFCTR_LLC_S0_TA_DGRAM_1B_ECC		= 0x32B,
};

/*
 * ARMv8 HiSilicon MN RAW event types.
 */
enum armv8_hisi_mn_event_types {
	ARMV8_HISI_PERFCTR_MN_S0_TC_EO_BARR_REQ			= 0x32C,
	ARMV8_HISI_PERFCTR_MN_S0_TC_EC_BARR_REQ			= 0x32D,
	ARMV8_HISI_PERFCTR_MN_S0_TC_DV_MOP_REQ			= 0x32E,
	ARMV8_HISI_PERFCTR_MN_S0_TC_READ_REQ			= 0x32F,
	ARMV8_HISI_PERFCTR_MN_S0_TC_WRITE_REQ			= 0x330,
	ARMV8_HISI_PERFCTR_MN_S0_TC_COPYBK_REQ			= 0x331,
	ARMV8_HISI_PERFCTR_MN_S0_TC_OTHER_REQ			= 0x332,
	ARMV8_HISI_PERFCTR_MN_S0_TC_RETRY_REQ			= 0x333,
	ARMV8_HISI_PERFCTR_MN_S0_TA_EO_BARR_REQ			= 0x334,
	ARMV8_HISI_PERFCTR_MN_S0_TA_EC_BARR_REQ			= 0x335,
	ARMV8_HISI_PERFCTR_MN_S0_TA_DV_MOP_REQ			= 0x336,
	ARMV8_HISI_PERFCTR_MN_S0_TA_READ_REQ			= 0x337,
	ARMV8_HISI_PERFCTR_MN_S0_TA_WRITE_REQ			= 0x338,
	ARMV8_HISI_PERFCTR_MN_S0_TA_COPYBK_REQ			= 0x339,
	ARMV8_HISI_PERFCTR_MN_S0_TA_OTHER_REQ			= 0x33A,
	ARMV8_HISI_PERFCTR_MN_S0_TA_RETRY_REQ			= 0x33B,
};

/*
 * ARMv8 HiSilicon DDR RAW event types.
 */
enum armv8_hisi_ddr_event_types {
        ARMV8_HISI_PERFCTR_DDRC0_TC_FLUX_READ_BW                     = 0x33C,
        ARMV8_HISI_PERFCTR_DDRC0_TC_FLUX_WRITE_BW                    = 0x33D,
        ARMV8_HISI_PERFCTR_DDRC0_TC_FLUX_READ_LAT                    = 0x33E,
        ARMV8_HISI_PERFCTR_DDRC0_TC_FLUX_WRITE_LAT                   = 0x33F,
        ARMV8_HISI_PERFCTR_DDRC1_TC_FLUX_READ_BW                     = 0x340,
        ARMV8_HISI_PERFCTR_DDRC1_TC_FLUX_WRITE_BW                    = 0x341,
        ARMV8_HISI_PERFCTR_DDRC1_TC_FLUX_READ_LAT                    = 0x342,
        ARMV8_HISI_PERFCTR_DDRC1_TC_FLUX_WRITE_LAT                   = 0x343,
        ARMV8_HISI_PERFCTR_DDRC0_TA_FLUX_READ_BW                     = 0x344,
        ARMV8_HISI_PERFCTR_DDRC0_TA_FLUX_WRITE_BW                    = 0x345,
        ARMV8_HISI_PERFCTR_DDRC0_TA_FLUX_READ_LAT                    = 0x346,
        ARMV8_HISI_PERFCTR_DDRC0_TA_FLUX_WRITE_LAT                   = 0x347,
        ARMV8_HISI_PERFCTR_DDRC1_TA_FLUX_READ_BW                     = 0x349,
        ARMV8_HISI_PERFCTR_DDRC1_TA_FLUX_WRITE_BW                    = 0x34A,
        ARMV8_HISI_PERFCTR_DDRC1_TA_FLUX_READ_LAT                    = 0x34B,
        ARMV8_HISI_PERFCTR_DDRC1_TA_FLUX_WRITE_LAT                   = 0x34C,
	ARMV8_HISI_PERFCTR_EVENT_MAX,
};

enum armv8_hisi_llc_counters {
	ARMV8_HISI_IDX_LLC_S0_TC_COUNTER0 	= 0x30,
	ARMV8_HISI_IDX_LLC_S0_TC_COUNTER_MAX	= 0x37,
	ARMV8_HISI_IDX_LLC_S0_TA_COUNTER0	= 0x38,
	ARMV8_HISI_IDX_LLC_COUNTER_MAX		= 0x3F,
	ARMV8_HISI_IDX_MN_S0_TC_COUNTER0	= 0x40,
	ARMV8_HISI_IDX_MN_S0_TC_COUNTER_MAX	= 0x43,
	ARMV8_HISI_IDX_MN_S0_TA_COUNTER0	= 0x44,
	ARMV8_HISI_IDX_MN_COUNTER_MAX		= 0x47,
	ARMV8_HISI_IDX_DDR_S0_TC_COUNTER0	= 0x48,
	ARMV8_HISI_IDX_DDR_S0_TC_COUNTER_MAX	= 0x55,
	ARMV8_HISI_IDX_DDR_S0_TA_COUNTER0	= 0x56,
	ARMV8_HISI_IDX_DDR_COUNTER_MAX		= 0x64,
	ARMV8_HISI_IDX_COUNTER_MAX,
};

enum hisi_die_type {
	HISI_DIE_TYPE_TOTEM,
	HISI_DIE_TYPE_NIMBUS,
};

#define HISI_HW_EVENT_MAX (1 << 12)
#define HISI_HW_EVENT_MASK (0xFFF)

#define HISI_HW_MODULE_MAX (1 << 4)
#define HISI_HW_MODULE_MASK (0xF000)

#define HISI_HW_DIE_MAX (1 << 4)
#define HISI_HW_DIE_MASK (0xF0000)

#define HISI_HW_SOCKET_MAX (1 << 4)
#define HISI_HW_SOCKET_MASK (0xF00000)

/* HW perf modules supported index */
enum armv8_hisi_hw_module_types {
	ARMV8_HISI_HWMOD_LLC,
	ARMV8_HISI_HWMOD_MN0,
	ARMV8_HISI_HWMOD_MN1,
	ARMV8_HISI_HWMOD_DDRC0,
	ARMV8_HISI_HWMOD_DDRC1,
	ARMV8_HISI_HWMOD_MAX,

};

#define HISI_LLC_BANK_MODULE_ID 0x04
#define HISI_MN1_MODULE_ID      0x0B

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

#define HISI_LLC_BANK_INT_EVENT0 (1<<0)
#define HISI_LLC_BANK_INT_EVENT1 (1<<1)
#define HISI_LLC_BANK_INT_EVENT2 (1<<2)
#define HISI_LLC_BANK_INT_EVENT3 (1<<3)
#define HISI_LLC_BANK_INT_EVENT4 (1<<4)
#define HISI_LLC_BANK_INT_EVENT5 (1<<5)
#define HISI_LLC_BANK_INT_EVENT6 (1<<6)
#define HISI_LLC_BANK_INT_EVENT7 (1<<7)

#define HISI_LLC_S0_TOTEMA_DTS_NAME "s0_ta_llc"
#define HISI_LLC_S0_TOTEMC_DTS_NAME "s0_tc_llc"
#define HISI_LLC_S1_TOTEMA_DTS_NAME "s1_ta_llc"
#define HISI_LLC_S1_TOTEMC_DTS_NAME "s1_tc_llc"

#define HISI_MN_S0_TOTEMA_DTS_NAME "s0_ta_mn"
#define HISI_MN_S0_TOTEMC_DTS_NAME "s0_tc_mn"
#define HISI_MN_S1_TOTEMA_DTS_NAME "s1_ta_mn"
#define HISI_MN_S1_TOTEMC_DTS_NAME "s1_tc_mn"

#define HISI_LLC_MAX_EVENTS 22

#define HISI_MN_MAX_EVENTS 9

#define	HISI_ARMV8_EVTYPE_EVENT	0x3ff
#define HISI_ARMV8_MAX_CFG_LLC_EVENTS 0x08
#define HISI_ARMV8_MAX_CFG_MN_EVENTS 0x04
#define HISI_ARMV8_MAX_CFG_DDR_EVENTS 0x0D
#define HISI_ARMV8_MAX_CFG_EVENTS_MASK 0xff

#define HISI_MN_EVENT_CNT0      0x30
#define HISI_MN_EVENT_CNT1      0x34
#define HISI_MN_EVENT_CNT2      0x38
#define HISI_MN_EVENT_CNT3      0x3c
#define HISI_MN_EVENT_CTRL      0x40
#define HISI_MN_EVENT_TYPE      0x48
#define HISI_MN_EVENT_EN        0x1

#define HISI_DDRC_CTRL_PERF		0x010
#define HISI_DDRC_CFG_PERF		0x270
#define HISI_DDRC_FLUX_WR		0x380
#define HISI_DDRC_FLUX_RD		0x384
#define HISI_DDRC_FLUX_WCMD		0x388
#define HISI_DDRC_FLUX_RCMD		0x38C
#define HISI_DDRC_FLUX_WLATCNT1		0x3A4
#define HISI_DDRC_FLUX_RLAT_CNT1	0x3AC

#define HISI_DDRC_TOTEMA_DTS_NAME "s0_ta_ddrc"
#define HISI_DDRC_TOTEMB_DTS_NAME "s0_tb_ddrc"
#define HISI_DDRC_TOTEMC_DTS_NAME "s0_tc_ddrc"

#define DDR_REG_READ(addr, value) (value = *(volatile u32 *)(addr))
#define DDR_REG_WRITE(addr, value) (*(volatile u32 *)(addr) = value)

#define MAX_DDR 8
#define MAX_BANKS 8
#define NUM_LLC_BANKS 4
#define MAX_HW_MODULES 6
#define MAX_DIE 8

typedef struct bank_info_t {
	u32 cfg_en;
	int irq;
} bank_info;

struct hisi_hwc_prev_counter {
	local64_t prev_count;
};

struct hisi_llc_hwc_data_info {
	u32 num_banks;
	struct hisi_hwc_prev_counter *hwc_prev_counters;
};

struct hisi_mn_hwc_data_info {
	local64_t event_start_count;
};

struct hisi_ddr_hwc_data_info {
	local64_t cpu_start_time;
	local64_t event_start_count;
};

/* Have this for Every totem/die */
typedef struct hisi_syscon_reg_map_t {
	int die_type;
	struct regmap *djtag_reg_map;
} hisi_syscon_reg_map;

typedef struct hisi_llc_data_t {
        u64 djtag_reg_map;
        hisi_syscon_reg_map *reg_map;
	u32 num_banks;
	bank_info bank[MAX_BANKS];
} hisi_llc_data;

typedef struct hisi_mn_data_t {
    u64 djtag_reg_map;
}hisi_mn_data;

typedef struct hisi_ddr_data_t {
        u64 ddrc_reg_map;
}hisi_ddr_data;

#if 0
typedef struct hisi_hwmod_data_t {
        hisi_ddr_data llc_data[MAX_LLC];
        hisi_ddr_data mn_data[MAX_MN];
        hisi_ddr_data ddr_data[MAX_DDR];
}hisi_hwmod_data;
#endif
