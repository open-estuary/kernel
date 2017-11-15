/*
 * HiSilicon SoC MN Hardware event counters support
 *
 * Copyright (C) 2017 Hisilicon Limited
 * Author: Shaokun Zhang <zhangshaokun@hisilicon.com>
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
#include <linux/acpi.h>
#include <linux/bitmap.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/perf_event.h>
#include "hisi_uncore_pmu.h"

/*
 * ARMv8 HiSilicon MN event types.
 */
enum armv8_hisi_mn_event_types {
	HISI_HWEVENT_MN_EO_BARR_REQ	= 0x0,
	HISI_HWEVENT_MN_EC_BARR_REQ	= 0x01,
	HISI_HWEVENT_MN_DVM_OP_REQ	= 0x02,
	HISI_HWEVENT_MN_DVM_SYNC_REQ	= 0x03,
	HISI_HWEVENT_MN_READ_REQ	= 0x04,
	HISI_HWEVENT_MN_WRITE_REQ	= 0x05,
	HISI_HWEVENT_MN_EVENT_MAX	= 0x08,
};

/*
 * ARMv8 HiSilicon Hardware counter Index.
 */
enum armv8_hisi_mn_counters {
	HISI_IDX_MN_COUNTER0	= 0x0,
	HISI_IDX_MN_COUNTER_MAX	= 0x4,
};

#define MN1_EVTYPE_REG_OFF 0x48
#define MN1_EVCTRL_REG_OFF 0x40
#define MN1_CNT0_REG_OFF 0x30
#define MN1_EVENT_EN 0x01
#define MN1_BANK_SELECT 0x01

#define GET_MODULE_ID(hwmod_data) hwmod_data->module_id

/*
 * Default timer frequency to poll and avoid counter overflow.
 * CPU speed = 2.4Ghz and number of CPU cores in a SCCL is 16.
 * For a single MN event on a CPU core consumes 200 cycles.
 * So overflow time = (2^31 * 200) / (16 * 2.4G) which is about 21 seconds
 * So on a safe side we use a timer interval of 8sec
 */
#define MN1_HRTIMER_INTERVAL (8LL * MSEC_PER_SEC)

struct hisi_mn_data {
	struct hisi_djtag_client *client;
	u32 module_id;
};

static inline int hisi_mn_counter_valid(int idx)
{
	return (idx >= HISI_IDX_MN_COUNTER0 &&
			idx < HISI_IDX_MN_COUNTER_MAX);
}

/* Select the counter register offset from the index */
static inline u32 get_counter_reg_off(int cntr_idx)
{
	return (MN1_CNT0_REG_OFF + (cntr_idx * 4));
}

static u64 hisi_mn_read_counter(struct hisi_pmu *mn_pmu, int cntr_idx)
{
	struct hisi_mn_data *mn_data = mn_pmu->hwmod_data;
	struct hisi_djtag_client *client = mn_data->client;
	u32 module_id = GET_MODULE_ID(mn_data);
	u32 reg_off, value;

	reg_off = get_counter_reg_off(cntr_idx);
	hisi_djtag_readreg(module_id, MN1_BANK_SELECT, reg_off,
			   client, &value);

	return value;
}

static void hisi_mn_set_evtype(struct hisi_pmu *mn_pmu, int idx, u32 val)
{
	struct hisi_mn_data *mn_data = mn_pmu->hwmod_data;
	struct hisi_djtag_client *client = mn_data->client;
	u32 module_id = GET_MODULE_ID(mn_data);
	u32 event_value, value = 0;

	event_value = (val - HISI_HWEVENT_MN_EO_BARR_REQ);

	/*
	 * Write the event code in event select register.
	 * Each byte in the 32 bit event select register is used
	 * to configure the event code. Each byte correspond to a
	 * counter register to use.
	 */
	val = event_value << (8 * idx);

	hisi_djtag_readreg(module_id, MN1_BANK_SELECT, MN1_EVTYPE_REG_OFF,
			   client, &value);
	value &= ~(0xff << (8 * idx));
	value |= val;
	hisi_djtag_writereg(module_id, MN1_BANK_SELECT, MN1_EVTYPE_REG_OFF,
			    value, client);
}

static void hisi_mn_clear_evtype(struct hisi_pmu *mn_pmu, int idx)
{
	struct hisi_mn_data *mn_data = mn_pmu->hwmod_data;
	struct hisi_djtag_client *client = mn_data->client;
	u32 module_id = GET_MODULE_ID(mn_data);
	u32 value;

	if (!hisi_mn_counter_valid(idx)) {
		dev_err(mn_pmu->dev, "Unsupported event index:%d!\n", idx);
		return;
	}

	/*
	 * Clear the event code in event select register by writing value 0xff.
	 * Each byte in the 32 bit event select register is used to configure
	 * the event code. Each byte correspond to a counter register to use.
	 */
	hisi_djtag_readreg(module_id, MN1_BANK_SELECT, MN1_EVTYPE_REG_OFF,
			   client, &value);
	value &= ~(0xff << (8 * idx));
	value |= (0xff << (8 * idx));
	hisi_djtag_writereg(module_id, MN1_BANK_SELECT, MN1_EVTYPE_REG_OFF,
			    value, client);
}

static void hisi_mn_write_counter(struct hisi_pmu *mn_pmu,
				  struct hw_perf_event *hwc, u32 value)
{
	struct hisi_mn_data *mn_data = mn_pmu->hwmod_data;
	struct hisi_djtag_client *client = mn_data->client;
	u32 module_id = GET_MODULE_ID(mn_data);
	u32 reg_off;
	int idx = GET_CNTR_IDX(hwc);

	reg_off = get_counter_reg_off(idx);
	hisi_djtag_writereg(module_id, MN1_BANK_SELECT, reg_off, value, client);
}

static void hisi_mn_start_counters(struct hisi_pmu *mn_pmu)
{
	struct hisi_mn_data *mn_data = mn_pmu->hwmod_data;
	struct hisi_djtag_client *client = mn_data->client;
	unsigned long *used_mask = mn_pmu->pmu_events.used_mask;
	u32 module_id = GET_MODULE_ID(mn_data);
	u32 num_counters = mn_pmu->num_counters;
	u32 value;
	int enabled = bitmap_weight(used_mask, num_counters);

	if (!enabled)
		return;

	/* Set the event_bus_en bit in MN_EVENT_CTRL to start counting */
	hisi_djtag_readreg(module_id, MN1_BANK_SELECT, MN1_EVCTRL_REG_OFF,
			   client, &value);
	value |= MN1_EVENT_EN;
	hisi_djtag_writereg(module_id, MN1_BANK_SELECT, MN1_EVCTRL_REG_OFF,
			    value, client);
}

static void hisi_mn_stop_counters(struct hisi_pmu *mn_pmu)
{
	struct hisi_mn_data *mn_data = mn_pmu->hwmod_data;
	struct hisi_djtag_client *client = mn_data->client;
	u32 module_id = GET_MODULE_ID(mn_data);
	u32 value;

	/*
	 * Clear the event_bus_en bit in MN_EVENT_CTRL
	 */
	hisi_djtag_readreg(module_id, MN1_BANK_SELECT, MN1_EVCTRL_REG_OFF,
			   client, &value);
	value &= ~(MN1_EVENT_EN);
	hisi_djtag_writereg(module_id, MN1_BANK_SELECT, MN1_EVCTRL_REG_OFF,
			    value, client);
}

static void hisi_mn_clear_event_idx(struct hisi_pmu *mn_pmu, int idx)
{
	if (!hisi_mn_counter_valid(idx)) {
		dev_err(mn_pmu->dev, "Unsupported event index:%d!\n", idx);
		return;
	}
	clear_bit(idx, mn_pmu->pmu_events.used_mask);
}

static int hisi_mn_get_event_idx(struct perf_event *event)
{
	struct hisi_pmu *mn_pmu = to_hisi_pmu(event->pmu);
	unsigned long *used_mask = mn_pmu->pmu_events.used_mask;
	u32 num_counters = mn_pmu->num_counters;
	int event_idx;

	event_idx = find_first_zero_bit(used_mask, num_counters);
	if (event_idx == num_counters)
		return -EAGAIN;

	set_bit(event_idx, used_mask);

	return event_idx;
}

static const struct of_device_id mn_of_match[] = {
	{ .compatible = "hisilicon,hip05-pmu-mn-v1", },
	{ .compatible = "hisilicon,hip06-pmu-mn-v1", },
	{ .compatible = "hisilicon,hip07-pmu-mn-v2", },
	{},
};
MODULE_DEVICE_TABLE(of, mn_of_match);

static const struct acpi_device_id hisi_mn_pmu_acpi_match[] = {
	{ "HISI0221", },
	{ "HISI0222", },
	{},
};
MODULE_DEVICE_TABLE(acpi, hisi_mn_pmu_acpi_match);

static int hisi_mn_init_data(struct hisi_pmu *mn_pmu,
			     struct hisi_djtag_client *client)
{
	struct hisi_mn_data *mn_data;
	struct device *dev = &client->dev;
	int ret;

	mn_data = devm_kzalloc(dev, sizeof(*mn_data), GFP_KERNEL);
	if (!mn_data)
		return -ENOMEM;

	/* Set the djtag Identifier */
	mn_data->client = client;
	mn_pmu->hwmod_data = mn_data;

	if (dev->of_node) {
		const struct of_device_id *of_id;

		of_id = of_match_device(mn_of_match, dev);
		if (!of_id) {
			dev_err(dev, "DT: Match device fail!\n");
			return -EINVAL;
		}
	} else if (ACPI_COMPANION(dev)) {
		const struct acpi_device_id *acpi_id;

		acpi_id = acpi_match_device(hisi_mn_pmu_acpi_match, dev);
		if (!acpi_id) {
			dev_err(dev, "ACPI: Match device fail!\n");
			return -EINVAL;
		}
	} else
		return -EINVAL;

	ret = device_property_read_u32(dev, "hisilicon,module-id",
				       &mn_data->module_id);
	if (ret < 0) {
		dev_err(dev, "DT: Could not read module-id!\n");
		return -EINVAL;
	}

	return 0;
}

static struct attribute *hisi_mn_format_attr[] = {
	HISI_PMU_FORMAT_ATTR(event, "config:0-7"),
	NULL,
};

static const struct attribute_group hisi_mn_format_group = {
	.name = "format",
	.attrs = hisi_mn_format_attr,
};

static struct attribute *hisi_mn_events_attr[] = {
	HISI_PMU_EVENT_ATTR_STR(eo_barrier_req, "event=0x0"),
	HISI_PMU_EVENT_ATTR_STR(ec_barrier_req,	"event=0x01"),
	HISI_PMU_EVENT_ATTR_STR(dvm_op_req, "event=0x02"),
	HISI_PMU_EVENT_ATTR_STR(dvm_sync_req, "event=0x03"),
	HISI_PMU_EVENT_ATTR_STR(read_req, "event=0x04"),
	HISI_PMU_EVENT_ATTR_STR(write_req, "event=0x05"),
	NULL,
};

static const struct attribute_group hisi_mn_events_group = {
	.name = "events",
	.attrs = hisi_mn_events_attr,
};

static struct attribute *hisi_mn_attrs[] = {
	NULL,
};

static const struct attribute_group hisi_mn_attr_group = {
	.attrs = hisi_mn_attrs,
};

static DEVICE_ATTR(cpumask, 0444, hisi_cpumask_sysfs_show, NULL);

static struct attribute *hisi_mn_cpumask_attrs[] = {
	&dev_attr_cpumask.attr,
	NULL,
};

static const struct attribute_group hisi_mn_cpumask_attr_group = {
	.attrs = hisi_mn_cpumask_attrs,
};

static const struct attribute_group *hisi_mn_pmu_attr_groups[] = {
	&hisi_mn_attr_group,
	&hisi_mn_format_group,
	&hisi_mn_events_group,
	&hisi_mn_cpumask_attr_group,
	NULL,
};

static struct hisi_uncore_ops hisi_uncore_mn_ops = {
	.set_evtype = hisi_mn_set_evtype,
	.clear_evtype = hisi_mn_clear_evtype,
	.set_event_period = hisi_uncore_pmu_set_event_period,
	.get_event_idx = hisi_mn_get_event_idx,
	.clear_event_idx = hisi_mn_clear_event_idx,
	.event_update = hisi_uncore_pmu_event_update,
	.start_counters = hisi_mn_start_counters,
	.stop_counters = hisi_mn_stop_counters,
	.write_counter = hisi_mn_write_counter,
	.read_counter = hisi_mn_read_counter,
};

/* Use hrtimer when no IRQ, to poll for avoiding counter overflow */
static void hisi_mn_hrtimer_init(struct hisi_pmu *mn_pmu)
{
	INIT_LIST_HEAD(&mn_pmu->active_list);
	mn_pmu->ops->start_hrtimer = hisi_hrtimer_start;
	mn_pmu->ops->stop_hrtimer = hisi_hrtimer_stop;
	hisi_hrtimer_init(mn_pmu, MN1_HRTIMER_INTERVAL);
}

static void hisi_mn_pmu_init(struct hisi_pmu *mn_pmu,
			     struct hisi_djtag_client *client)
{
	struct device *dev = &client->dev;

	mn_pmu->num_events = HISI_HWEVENT_MN_EVENT_MAX;
	mn_pmu->num_counters = HISI_IDX_MN_COUNTER_MAX;
	mn_pmu->counter_bits = 32;
	mn_pmu->num_active = 0;
	mn_pmu->scl_id = hisi_djtag_get_sclid(client);

	mn_pmu->name = kasprintf(GFP_KERNEL, "hisi_mn_%d", mn_pmu->scl_id);
	mn_pmu->ops = &hisi_uncore_mn_ops;
	mn_pmu->dev = dev;

	/* Pick one core to use for cpumask attributes */
	cpumask_set_cpu(smp_processor_id(), &mn_pmu->cpus);

	/*
	 * Use poll method to avoid counter overflow as overflow IRQ
	 * is not supported in v1, v2 hardware.
	 */
	hisi_mn_hrtimer_init(mn_pmu);
}

static int hisi_pmu_mn_dev_probe(struct hisi_djtag_client *client)
{
	struct hisi_pmu *mn_pmu;
	struct device *dev = &client->dev;
	int ret;

	mn_pmu = hisi_pmu_alloc(dev, HISI_IDX_MN_COUNTER_MAX);
	if (!mn_pmu)
		return -ENOMEM;

	ret = hisi_mn_init_data(mn_pmu, client);
	if (ret)
		return ret;

	hisi_mn_pmu_init(mn_pmu, client);

	mn_pmu->pmu = (struct pmu) {
		.name = mn_pmu->name,
		.task_ctx_nr = perf_invalid_context,
		.event_init = hisi_uncore_pmu_event_init,
		.pmu_enable = hisi_uncore_pmu_enable,
		.pmu_disable = hisi_uncore_pmu_disable,
		.add = hisi_uncore_pmu_add,
		.del = hisi_uncore_pmu_del,
		.start = hisi_uncore_pmu_start,
		.stop = hisi_uncore_pmu_stop,
		.read = hisi_uncore_pmu_read,
		.attr_groups = hisi_mn_pmu_attr_groups,
	};

	ret = hisi_uncore_pmu_setup(mn_pmu, mn_pmu->name);
	if (ret) {
		dev_err(dev, "hisi_uncore_pmu_init FAILED!!\n");
		kfree(mn_pmu->name);
		return ret;
	}

	/* Set the drv data to MN pmu */
	dev_set_drvdata(dev, mn_pmu);

	return 0;
}

static int hisi_pmu_mn_dev_remove(struct hisi_djtag_client *client)
{
	struct hisi_pmu *mn_pmu;
	struct device *dev = &client->dev;

	mn_pmu = dev_get_drvdata(dev);
	perf_pmu_unregister(&mn_pmu->pmu);
	kfree(mn_pmu->name);

	return 0;
}

static struct hisi_djtag_driver hisi_pmu_mn_driver = {
	.driver = {
		.name = "hisi-pmu-mn",
		.of_match_table = mn_of_match,
		.acpi_match_table = ACPI_PTR(hisi_mn_pmu_acpi_match),
	},
	.probe = hisi_pmu_mn_dev_probe,
	.remove = hisi_pmu_mn_dev_remove,
};

static int __init hisi_pmu_mn_init(void)
{
	int ret;

	ret = hisi_djtag_register_driver(THIS_MODULE, &hisi_pmu_mn_driver);
	if (ret < 0) {
		pr_err("hisi pmu MN init failed, ret=%d\n", ret);
		return ret;
	}

	return 0;
}
module_init(hisi_pmu_mn_init);

static void __exit hisi_pmu_mn_exit(void)
{
	hisi_djtag_unregister_driver(&hisi_pmu_mn_driver);
}
module_exit(hisi_pmu_mn_exit);

MODULE_DESCRIPTION("HiSilicon SoC HIP0x MN PMU driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Shaokun Zhang");
