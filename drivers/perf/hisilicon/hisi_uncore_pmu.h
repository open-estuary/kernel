/*
 * HiSilicon SoC Hardware event counters support
 *
 * Copyright (C) 2017 Hisilicon Limited
 * Author: Anurup M <anurup.m@huawei.com>
 *
 * This code is based on the uncore PMUs like arm-cci and arm-ccn.
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
#ifndef __HISI_UNCORE_PMU_H__
#define __HISI_UNCORE_PMU_H__

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <asm/local64.h>
#include "djtag.h"

#undef pr_fmt
#define pr_fmt(fmt)     "hisi_pmu: " fmt

#define HISI_EVTYPE_EVENT	0xff
#define GET_EVENTID(ev)   (ev->hw.config & HISI_EVTYPE_EVENT)

#define HISI_MAX_PERIOD(nr) (BIT_ULL(nr) - 1)
#define MAX_BANKS 4

#define GET_CNTR_IDX(hwc) (hwc->idx)
#define to_hisi_pmu(c)	(container_of(c, struct hisi_pmu, pmu))

#define HISI_PMU_FORMAT_ATTR(_name, _config)		\
	(&((struct dev_ext_attribute[]) {		\
		{ .attr = __ATTR(_name, 0444,		\
				 hisi_format_sysfs_show,\
				 NULL),			\
		  .var = (void *) _config,		\
		}					\
	})[0].attr.attr)

#define HISI_PMU_EVENT_ATTR_STR(_name, _str)		\
	(&((struct perf_pmu_events_attr[]) {		\
		{ .attr = __ATTR(_name, 0444,		\
				 hisi_event_sysfs_show,	\
				 NULL),			\
		  .event_str = _str,			\
		}					\
	  })[0].attr.attr)

struct hisi_pmu;

struct hisi_uncore_ops {
	void (*set_evtype)(struct hisi_pmu *, int, u32);
	void (*clear_evtype)(struct hisi_pmu *, int);
	void (*set_event_period)(struct perf_event *);
	int (*get_event_idx)(struct perf_event *);
	void (*clear_event_idx)(struct hisi_pmu *, int);
	u64 (*event_update)(struct perf_event *);
	u64 (*read_counter)(struct hisi_pmu *, int);
	void (*write_counter)(struct hisi_pmu *, struct hw_perf_event *, u32);
	void (*enable_counter)(struct hisi_pmu *, int);
	void (*disable_counter)(struct hisi_pmu *, int);
	void (*start_counters)(struct hisi_pmu *);
	void (*stop_counters)(struct hisi_pmu *);
	void (*start_hrtimer)(struct hisi_pmu *);
	void (*stop_hrtimer)(struct hisi_pmu *);
};

struct hisi_pmu_hwevents {
	struct perf_event **hw_events;
	unsigned long *used_mask;
};

/* Generic pmu struct for different pmu types */
struct hisi_pmu {
	const char *name;
	struct pmu pmu;
	struct hisi_uncore_ops *ops;
	struct hisi_pmu_hwevents pmu_events;
	void *hwmod_data; /* Hardware module specific data */
	cpumask_t cpus;
	struct device *dev;
	struct list_head active_list; /* Active events list */
	struct hrtimer hrtimer; /* hrtimer to handle the
				 * counter overflow
				 */
	u64 hrt_duration; /* hrtimer timeout */
	u32 scl_id;
	int num_counters;
	int num_events;
	int num_active;
	int counter_bits;
};

void hisi_uncore_pmu_read(struct perf_event *event);
int hisi_uncore_pmu_add(struct perf_event *event, int flags);
void hisi_uncore_pmu_del(struct perf_event *event, int flags);
void hisi_uncore_pmu_start(struct perf_event *event, int flags);
void hisi_uncore_pmu_stop(struct perf_event *event, int flags);
void hisi_uncore_pmu_set_event_period(struct perf_event *event);
u64 hisi_uncore_pmu_event_update(struct perf_event *event);
int hisi_uncore_pmu_event_init(struct perf_event *event);
int hisi_uncore_pmu_setup(struct hisi_pmu *hisi_pmu, const char *pmu_name);
void hisi_uncore_pmu_enable(struct pmu *pmu);
void hisi_uncore_pmu_disable(struct pmu *pmu);
struct hisi_pmu *hisi_pmu_alloc(struct device *dev, u32 num_cntrs);
ssize_t hisi_event_sysfs_show(struct device *dev,
			      struct device_attribute *attr, char *buf);
ssize_t hisi_format_sysfs_show(struct device *dev,
			       struct device_attribute *attr, char *buf);
ssize_t hisi_cpumask_sysfs_show(struct device *dev,
				struct device_attribute *attr, char *buf);
void hisi_hrtimer_init(struct hisi_pmu *hisi_pmu, u64 timer_interval);
void hisi_hrtimer_start(struct hisi_pmu *hisi_pmu);
void hisi_hrtimer_stop(struct hisi_pmu *hisi_pmu);
void hisi_djtag_readreg(int module_id, int bank, u32 offset,
			struct hisi_djtag_client *client, u32 *value);
void hisi_djtag_writereg(int module_id, int bank, u32 offset,
			 u32 value, struct hisi_djtag_client *client);
#endif /* __HISI_UNCORE_PMU_H__ */
