/*
 * Huawei Pv660/Hi1610 sas controller device process
 *
 * Copyright 2015 Huawei. <chenjianmin@huawei.com>
 *
 * This file is licensed under GPLv2.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
*/
#include "higgs_common.h"
#include "higgs_pv660.h"
#include "higgs_init.h"
#include "higgs_peri.h"
#include "higgs_phy.h"
#include "higgs_port.h"
#include "higgs_dev.h"
#include "higgs_io.h"
#include "higgs_eh.h"
#include "higgs_dfm.h"
#include "higgs_misc.h"
#include "higgs_intr.h"
#include "higgs_dump.h"
#include "higgs_stub.h"

#if defined(PV660_ARM_SERVER)
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#endif


#define MAX_CFG_SETION_LEN	(64)
#define MAX_NAME_LEN (32)

/* clear goto statement  */


/*----------------------------------------------*
 * outer variable type define                                *
 *----------------------------------------------*/
extern const char compile_date[];
extern const char compile_time[];
extern const char higgs_sw_ver[];
extern char ll_sw_ver[SAL_MAX_CARD_NUM][SAL_MAX_CHIP_VER_STR_LEN];

/*----------------------------------------------*
 * outer function type define.                            *
 *----------------------------------------------*/

/*----------------------------------------------*
 * inner function type define                                      *
 *----------------------------------------------*/
static s32 higgs_wait_axi_bus_idle(struct higgs_card *ll_card);
static s32 higgs_wait_dma_tx_rx_idle(struct higgs_card *ll_card);
static s32 higgs_prepare_reset_hw(struct higgs_card *ll_card);
static s32 higgs_execute_reset_hw(struct higgs_card *ll_card);
static s32 higgs_reset_hw(struct higgs_card *ll_card);
void higgs_device_release(struct device *dev);
static s32 higgs_test_reg_access(struct higgs_card *ll_card);
//void Higgs_NotifySfpEvent(unsigned long data);
void higgs_notify_led_thread_remove(struct higgs_card *ll_card);

/* arm server debug */
int hilink_reg_init(void);
void hilink_reg_exit(void);
unsigned int hilink_sub_dsaf_init(void);
void hilink_sub_dsaf_exit(void);
unsigned int hilink_sub_pcie_init(void);
void hilink_sub_pcie_exit(void);
/* arm server debug */

void sal_remove_all_dev(u8 card_id);

/*----------------------------------------------*
 * global variable                                          *
 *----------------------------------------------*/
struct higgs_global higgs_dispatcher = {
	.higgs_log_level = OSP_DBL_MINOR,
	{{0}},
	{0},
};


u32 max_cpu_nodes;

struct higgs_reg_info {
	u64 reg_addr;
	u32 reg_value;
	u32 enable;
};

struct higgs_probe_info {
	struct platform_device *dev;
	u32 card_id;
};


#if defined(FPGA_VERSION_TEST)
static struct higgs_reg_info global_reg_cfg[] = {
    /**Delivery Queue enable register */
	{HISAS30HV100_GLOBAL_REG_DLVRY_QUEUE_ENABLE_REG,
	 (1 << HIGGS_MAX_DQ_NUM) - 1, HIGGS_REG_ENABLE},
	{HISAS30HV100_GLOBAL_REG_HGC_SAS_TXFAIL_RETRY_CTRL_REG, 0x211FF,
	 HIGGS_REG_ENABLE},
	{HISAS30HV100_GLOBAL_REG_CFG_AGING_TIME_REG, 0x7a120, HIGGS_REG_ENABLE},
	{HISAS30HV100_GLOBAL_REG_HGC_ERR_STAT_EN_REG, 0x401, HIGGS_REG_ENABLE},
	{HISAS30HV100_GLOBAL_REG_CFG_1US_TIMER_TRSH_REG, 0x64,
	 HIGGS_REG_ENABLE},
	{HISAS30HV100_GLOBAL_REG_HGC_GET_ITV_TIME_REG, 0x5, HIGGS_REG_ENABLE},
	{HISAS30HV100_GLOBAL_REG_INT_COAL_EN_REG, 0xC, HIGGS_REG_ENABLE},
								   /**interrupt coalition enable register */
	{HISAS30HV100_GLOBAL_REG_OQ_INT_COAL_TIME_REG, 0x186A0, HIGGS_REG_ENABLE},
									   /**interrupt coalition count register */
	{HISAS30HV100_GLOBAL_REG_OQ_INT_COAL_CNT_REG, 0x1, HIGGS_REG_ENABLE},
								       /**interrupt coalition time config register */
	{HISAS30HV100_GLOBAL_REG_ENT_INT_COAL_TIME_REG, 0x1, HIGGS_REG_ENABLE},

	{HISAS30HV100_GLOBAL_REG_ENT_INT_COAL_CNT_REG, 0x1, HIGGS_REG_ENABLE},

	{HISAS30HV100_GLOBAL_REG_OQ_INT_SRC_REG, 0x0, HIGGS_REG_ENABLE},
								  /**OQ interrupt source register */
	{HISAS30HV100_GLOBAL_REG_OQ_INT_SRC_MSK_REG, 0xfffffff0, HIGGS_REG_ENABLE},
									     /**OQ interrupt source mask register*/
	{HISAS30HV100_GLOBAL_REG_ENT_INT_SRC1_REG, 0xffffffff, HIGGS_REG_ENABLE},
									   /**event interrupt source register*/

	{HISAS30HV100_GLOBAL_REG_ENT_INT_SRC_MSK1_REG, 0xffffffff, HIGGS_REG_ENABLE},
									       /**event interrupt source mask register*/

	{HISAS30HV100_GLOBAL_REG_ENT_INT_SRC2_REG, 0xffffffff, HIGGS_REG_ENABLE},
									   /**event interrupt source register*/

	{HISAS30HV100_GLOBAL_REG_ENT_INT_SRC_MSK2_REG, 0xCfffffff, HIGGS_REG_ENABLE},
									       /**event interrupt source mask register*/
	{HISAS30HV100_GLOBAL_REG_SAS_ECC_INTR_MSK_REG, 0xFFFFFFC0, HIGGS_REG_ENABLE},
									       /**event interrupt source mask register*/
	{HISAS30HV100_GLOBAL_REG_AXI_AHB_CLK_CFG_REG, 0x1, HIGGS_REG_ENABLE},
	{HISAS30HV100_GLOBAL_REG_CFG_SAS_CONFIG_REG, 0x40000FF, HIGGS_REG_ENABLE}
};

static struct higgs_reg_info port_reg_cfg[] = {
	{HISAS30HV100_PORT_REG_PROG_PHY_LINK_RATE_REG, 0x00000888,
	 HIGGS_REG_ENABLE},
	{HISAS30HV100_PORT_REG_SL_TOUT_CFG_REG, 0x7D3E7D7D, HIGGS_REG_ENABLE},
	{HISAS30HV100_PORT_REG_DONE_RECEVIED_TIME_REG, 0x10, HIGGS_REG_ENABLE},
	{HISAS30HV100_PORT_REG_RXOP_CHECK_CFG_H_REG, 0x1000, HIGGS_REG_ENABLE},
	{HISAS30HV100_PORT_REG_CHL_INT0_MSK_REG, 0xfffffffe, HIGGS_REG_ENABLE},
									 /**channel interrupt register 0 mask register */
	{HISAS30HV100_PORT_REG_CHL_INT1_MSK_REG, 0xffffffff, HIGGS_REG_ENABLE},
									 /**channel interrupt register 1 mask register */
	{HISAS30HV100_PORT_REG_CHL_INT2_MSK_REG, 0xffffff2b, HIGGS_REG_ENABLE},
									 /**channel interrupt register 2 mask register */
	{HISAS30HV100_PORT_REG_CHL_INT_COAL_EN_REG, 0, HIGGS_REG_ENABLE}
								  /**interrupt coalition enable register */
};

#else
static struct higgs_reg_info global_reg_cfg[] = {
	{HISAS30HV100_GLOBAL_REG_DLVRY_QUEUE_ENABLE_REG, (1 << HIGGS_MAX_DQ_NUM) - 1, HIGGS_REG_ENABLE},
											      /**Delivery Queue enable register */
	{HISAS30HV100_GLOBAL_REG_HGC_TRANS_TASK_CNT_LIMIT_REG, 0x11,
	 HIGGS_REG_ENABLE},
	{HISAS30HV100_GLOBAL_REG_DEVICE_MSG_WORK_MODE_REG, 0x1, HIGGS_REG_ENABLE},
									      /**Device parameter config register */
	{HISAS30HV100_GLOBAL_REG_MAX_BURST_BYTES_REG, 0, HIGGS_REG_DISABLE},
								      /**Maxim Burst Size*/
	{HISAS30HV100_GLOBAL_REG_SMP_TIMEOUT_TIMER_REG, 0, HIGGS_REG_DISABLE},
									/**send SMP request, wait response time*/

	{HISAS30HV100_GLOBAL_REG_MAX_CON_TIME_LIMIT_TIME_REG, 0, HIGGS_REG_DISABLE},
									      /**maximum connect time limit timer initial value*/
	{HISAS30HV100_GLOBAL_REG_HGC_SAS_TXFAIL_RETRY_CTRL_REG, 0x1ff,
	 HIGGS_REG_ENABLE},

	{HISAS30HV100_GLOBAL_REG_HGC_ERR_STAT_EN_REG, 0x401, HIGGS_REG_ENABLE},
	{HISAS30HV100_GLOBAL_REG_CFG_1US_TIMER_TRSH_REG, 0x64,
	 HIGGS_REG_ENABLE},
	{HISAS30HV100_GLOBAL_REG_HGC_GET_ITV_TIME_REG, 0x1, HIGGS_REG_ENABLE},
	{HISAS30HV100_GLOBAL_REG_I_T_NEXUS_LOSS_TIME_REG, 0x64, HIGGS_REG_ENABLE},
									     /**I_T nexus loss timer initial value*/
	{HISAS30HV100_GLOBAL_REG_BUS_INACTIVE_LIMIT_TIME_REG, 0x2710,
	 HIGGS_REG_ENABLE},
	{HISAS30HV100_GLOBAL_REG_REJECT_TO_OPEN_LIMIT_TIME_REG, 0x1,
	 HIGGS_REG_ENABLE},

	{HISAS30HV100_GLOBAL_REG_CFG_AGING_TIME_REG, 0x7a12, HIGGS_REG_ENABLE},
	{HISAS30HV100_GLOBAL_REG_HGC_DFX_CFG_REG2_REG, 0x9c40,
	 HIGGS_REG_ENABLE},
	{HISAS30HV100_GLOBAL_REG_FIS_LIST_BADDR_L_REG, 0x2, HIGGS_REG_ENABLE},
	{HISAS30HV100_GLOBAL_REG_INT_COAL_EN_REG, 0xC, HIGGS_REG_ENABLE},
								   /**interrupt coalition enable register */
	{HISAS30HV100_GLOBAL_REG_OQ_INT_COAL_TIME_REG, 0x186A0, HIGGS_REG_ENABLE},
									    /**interrupt coalition time config register */
	{HISAS30HV100_GLOBAL_REG_OQ_INT_COAL_CNT_REG, 1, HIGGS_REG_ENABLE},
								     /**interrupt coalition count register*/
	{HISAS30HV100_GLOBAL_REG_ENT_INT_COAL_TIME_REG, 0x1, HIGGS_REG_ENABLE},
									 /**event interrupt coalition time config register */
	{HISAS30HV100_GLOBAL_REG_ENT_INT_COAL_CNT_REG, 0x1, HIGGS_REG_ENABLE},
									/**event interrupt coalition count register*/

	{HISAS30HV100_GLOBAL_REG_OQ_INT_SRC_REG, 0xffffffff, HIGGS_REG_ENABLE},
									 /**OQ interrupt source register*/


	{HISAS30HV100_GLOBAL_REG_OQ_INT_SRC_MSK_REG, 0, HIGGS_REG_ENABLE},
								    /**DQ interrupt source mask register*/

	{HISAS30HV100_GLOBAL_REG_ENT_INT_SRC1_REG, 0xffffffff, HIGGS_REG_ENABLE},
									   /**event interrupt source register*/

	{HISAS30HV100_GLOBAL_REG_ENT_INT_SRC_MSK1_REG, 0, HIGGS_REG_ENABLE},
								      /**event interrupt source mask register*/

	{HISAS30HV100_GLOBAL_REG_ENT_INT_SRC2_REG, 0xffffffff, HIGGS_REG_ENABLE},
									    /**event interrupt source register*/

	{HISAS30HV100_GLOBAL_REG_ENT_INT_SRC_MSK2_REG, 0, HIGGS_REG_ENABLE},
								       /**event interrupt source mask register*/
	{HISAS30HV100_GLOBAL_REG_SAS_ECC_INTR_MSK_REG, 0, HIGGS_REG_ENABLE},
								       /**event interrupt source mask register*/
	{HISAS30HV100_GLOBAL_REG_AXI_AHB_CLK_CFG_REG, 0x2, HIGGS_REG_ENABLE},
	{HISAS30HV100_GLOBAL_REG_CFG_SAS_CONFIG_REG, 0x22000000,
	 HIGGS_REG_ENABLE}
};

#define SAS_12_GB
static struct higgs_reg_info port_reg_cfg[] = {
    /**open HAL_ISR_SL_PHY_ENABLE  HAL_ISR_PHYCTRL_NOT_RDY  HAL_ISR_PHYCTRL_STATUS_CHG*/
#if defined(SAS_3_GB)
	{HISAS30HV100_PORT_REG_PROG_PHY_LINK_RATE_REG, 0x00000899,
	 HIGGS_REG_ENABLE},
	{HISAS30HV100_PORT_REG_PHY_RATE_NEGO_REG, 0x415E600, HIGGS_REG_ENABLE},
	{HISAS30HV100_PORT_REG_PHY_CONFIG2_REG, 0x7C080, HIGGS_REG_ENABLE},
#endif

#if defined(SAS_6_GB)
	{HISAS30HV100_PORT_REG_PROG_PHY_LINK_RATE_REG, 0x0000088A,
	 HIGGS_REG_ENABLE},
	{HISAS30HV100_PORT_REG_PHY_CONFIG2_REG, 0x7C080, HIGGS_REG_ENABLE},
	{HISAS30HV100_PORT_REG_PHY_RATE_NEGO_REG, 0x415EE00, HIGGS_REG_ENABLE},
	{HISAS30HV100_PORT_REG_PHY_PCN_REG, 0x80A80000, HIGGS_REG_ENABLE},
#endif

#if defined(SAS_12_GB)
	{HISAS30HV100_PORT_REG_PROG_PHY_LINK_RATE_REG, 0x0000088A,
	 HIGGS_REG_ENABLE},
//     {HISAS30HV100_PORT_REG_PHY_CONFIG2_REG,0x80C7C084,HIGGS_REG_ENABLE},   //heyousong 2014/11/26
	{HISAS30HV100_PORT_REG_PHY_CONFIG2_REG, 0x8087C084, HIGGS_REG_ENABLE},	//heyousong 2014/11/26

	{HISAS30HV100_PORT_REG_PHY_RATE_NEGO_REG, 0x415EE00, HIGGS_REG_ENABLE},
	{HISAS30HV100_PORT_REG_PHY_PCN_REG, 0x80AA0001, HIGGS_REG_ENABLE},
#endif

	{HISAS30HV100_PORT_REG_SL_TOUT_CFG_REG, 0x7D7D7D7D, HIGGS_REG_ENABLE},
	{HISAS30HV100_PORT_REG_DONE_RECEVIED_TIME_REG, 0x0, HIGGS_REG_ENABLE},
	{HISAS30HV100_PORT_REG_RXOP_CHECK_CFG_H_REG, 0x1000, HIGGS_REG_ENABLE},
	{HISAS30HV100_PORT_REG_DONE_RECEVIED_TIME_REG, 0, HIGGS_REG_ENABLE},
	{HISAS30HV100_PORT_REG_CON_CFG_DRIVER_REG, 0x13f0a, HIGGS_REG_ENABLE},


	{HISAS30HV100_PORT_REG_CHL_INT_COAL_EN_REG, 3, HIGGS_REG_ENABLE},
								   /**interrupt coalition enable register*/
	{HISAS30HV100_PORT_REG_DONE_RECEVIED_TIME_REG, 8, HIGGS_REG_ENABLE}

};
#endif

#if defined(PV660_ARM_SERVER)
struct higgs_led_info {
	char name[16];
	u32 card_id;
	u32 phy_id;
	u32 offset;
};

static struct higgs_led_info higgs_led_info[] = {
	{"Hilink2-0", 0, 0, 0x1e},
	{"Hilink2-1", 0, 1, 0x22},
	{"Hilink2-2", 0, 2, 0x20},
	{"Hilink2-3", 0, 3, 0x2e},
	{"Hilink2-4", 0, 4, 0x1c},
	{"Hilink2-5", 0, 5, 0x1a},
	{"Hilink2-6", 0, 6, 0x16},
	{"Hilink2-7", 0, 7, 0x12},
	{"Hilink6-1", 1, 5, 0x18},
	{"Hilink6-2", 1, 6, 0x14},
	{"Hilink6-3", 1, 7, 0x24},
};

struct device_node *cpld_dev_node = NULL;
void __iomem *cpld_reg_base = NULL;
#endif
/* END:   Added by c00257296, 2015/1/19 */

static u64 higgs_dma_mask = DMA_BIT_MASK(64);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 16, 0)
static struct platform_device higgs_dev_0 = {
	.name = "PV660_SasController",
	.id = P660_SAS_CORE_DSAF_ID,	/* sas controller DSAF */
	.dev = {
		.dma_mask = &higgs_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(64),
		.release = higgs_device_release,
		},
};

static struct platform_device higgs_dev_1 = {
	.name = "PV660_SasController",
	.id = P660_SAS_CORE_PCIE_ID,	/* sas controller PCIE */
	.dev = {
		.dma_mask = &higgs_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(64),
		},
};

static struct platform_driver higgs_drv = {
	.probe = higgs_probe_stub,
	.remove = higgs_remove_stub,
	.driver = {
		   .name = "PV660_SasController",
		   .owner = THIS_MODULE,
		   },
};

#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)

static const struct of_device_id hisas_of_match[] = {
	{.compatible = "hisilicon,p660-sas",},
	{},
};

MODULE_DEVICE_TABLE(of, hisas_of_match);

static struct platform_driver higgs_drv = {
	.probe = higgs_probe_stub,
	.remove = higgs_remove_stub,
	.driver = {
		   .name = "PV660_SasController",
		   .owner = THIS_MODULE,
		   .of_match_table = hisas_of_match,
		   },
};

#else

static const struct of_device_id hisas_of_match[] = {
	{.compatible = "pv660, sas_controller",},
	{},
};

MODULE_DEVICE_TABLE(of, hisas_of_match);

static struct platform_driver higgs_drv = {
	.probe = higgs_probe_stub,
	.remove = higgs_remove_stub,
	.driver = {
		   .name = "PV660_SasController",
		   .owner = THIS_MODULE,
		   .of_match_table = hisas_of_match,
		   },
};
#endif



void higgs_device_release(struct device *dev)
{
	HIGGS_REF(dev);
	return;
}

#if 0          
void *higgs_alloc_dma_cq_mem(struct higgs_card *ll_card, u32 size,
			     OSP_DMA * dma_addr)
{
	void *cq_mem_addr = NULL;
	HIGGS_ASSERT(NULL != ll_card, return NULL);
	HIGGS_ASSERT(NULL != ll_card->plat_form_dev, return NULL);
#if 0

	g_pstCqDmaPool[ll_card->card_id] =
	    dma_pool_create("CQ MEM", &ll_card->plat_form_dev->dev, size, 4, 0);

	HIGGS_TRACE(OSP_DBL_MAJOR, 100, "Dma pool Alloc %p",
		    g_pstCqDmaPool[ll_card->card_id]);
	if (g_pstCqDmaPool == NULL) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1, "Dma pool alloc fail!");
		return cq_mem_addr;
	}
	cq_mem_addr =
	    dma_pool_alloc(g_pstCqDmaPool[ll_card->card_id], GFP_DMA, dma_addr);
	printk("[mjdbg]%s,addr:%lx,id:%d,size:%d,dmaaddr:%lx\n", __func__,
	       cq_mem_addr, ll_card->card_id, size, *dma_addr);
	if (NULL == cq_mem_addr) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1, "Dma pool Alloc fail!");
	}
	if ((((u64) cq_mem_addr) % CACHE_LINE_SIZE)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1,
			    "cq_mem_addr = %p cache align error!", cq_mem_addr);
	}
#endif
	cq_mem_addr =
	    dma_alloc_coherent(&ll_card->plat_form_dev->dev, size, dma_addr,
			       GFP_DMA);
	//printk("###[mjdbg]###1%s,addr:%lx,id:%d,size:%d,dmaaddr:%lx\n",__func__,cq_mem_addr,ll_card->card_id,size,*dma_addr);
	if (NULL == cq_mem_addr)
		HIGGS_TRACE(OSP_DBL_MAJOR, 1, "Dma Alloc fail!");

	if ((((u64) cq_mem_addr) % CACHE_LINE_SIZE))
		HIGGS_TRACE(OSP_DBL_MAJOR, 1,
			    "cq_mem_addr = %p cache align error!", cq_mem_addr);

	return cq_mem_addr;
}

void higgs_free_dma_cq_mem(struct higgs_card *ll_card, u32 size,
			   void *va, OSP_DMA dma_addr)
{
	HIGGS_ASSERT(NULL != va, return);
	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(NULL != ll_card->plat_form_dev, return);
	HIGGS_REF(ll_card);

	//dma_pool_free(g_pstCqDmaPool[ll_card->card_id], va, dma_addr);
	//dma_pool_destroy(g_pstCqDmaPool[ll_card->card_id]);
	//g_pstCqDmaPool[ll_card->card_id] = NULL;
	dma_free_coherent(&ll_card->plat_form_dev->dev, size, va, dma_addr);
	return;
}
#endif

void *higgs_alloc_dma_mem(struct higgs_card *ll_card, u32 size,
			  OSP_DMA * dma_addr)
{
	void *addr = NULL;

	HIGGS_ASSERT(NULL != ll_card, return NULL);
	HIGGS_ASSERT(NULL != ll_card->plat_form_dev, return NULL);

	addr =
	    dma_alloc_coherent(&ll_card->plat_form_dev->dev, size, dma_addr,
			       GFP_DMA);
	if (NULL == addr)
		HIGGS_TRACE(OSP_DBL_MAJOR, 1, "Dma Alloc fail!");

	if ((((u64) addr) % CACHE_LINE_SIZE))
		HIGGS_TRACE(OSP_DBL_MAJOR, 1, "addr = %p cache align error!",
			    addr);

	return addr;
}

void higgs_free_dma_mem(struct higgs_card *ll_card, u32 size,
			void *va, OSP_DMA dma_addr)
{
	HIGGS_ASSERT(NULL != va, return);
	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(NULL != ll_card->plat_form_dev, return);
	HIGGS_REF(ll_card);

	dma_free_coherent(&ll_card->plat_form_dev->dev, size, va, dma_addr);

	return;

}

OSP_DMA higgs_dma_map_single_sg(struct higgs_card * ll_card, void *sgl,
				u32 len, s32 direct)
{
	OSP_DMA ret = 0;
	HIGGS_ASSERT(NULL != sgl, return 0);
	HIGGS_REF(ll_card);
	HIGGS_REF(len);
	HIGGS_REF(direct);
#if 0
	ret =
	    dma_map_single(&(ll_card->plat_form_dev->dev), data_buff, len,
			   (enum dma_data_direction)direct);
	HIGGS_TRACE_FRQLIMIT(OSP_DBL_MAJOR, 60 * HZ, 4283,
			     "virt %llx, len %d and dma mapped addr %llx",
			     (u64) data_buff, len, (u64) ret);
#endif
	ret = sg_phys(sgl);
	HIGGS_TRACE_FRQLIMIT(OSP_DBL_MAJOR, 10 * HZ, 4283,
			     "sgl %llx virt %llx, phy addr %llx", (u64) sgl,
			     (u64) sg_virt(sgl), (u64) ret);

	return ret;
}

OSP_DMA higgs_dma_map_single_sge_buffer(struct higgs_card * ll_card,
					void *data_buff, u32 len, s32 direct)
{
	OSP_DMA ret = 0;
	HIGGS_ASSERT(NULL != data_buff, return 0);
	HIGGS_REF(ll_card);
	HIGGS_REF(len);
	HIGGS_REF(direct);

	ret =
	    dma_map_single(&(ll_card->plat_form_dev->dev), data_buff, len,
			   (enum dma_data_direction)direct);
	HIGGS_TRACE_FRQLIMIT(OSP_DBL_INFO, 60 * HZ, 4283,
			     "virt %llx, len %d and dma mapped addr %llx, direct addr is %d,dma mask is %llx",
			     (u64) data_buff, len, (u64) ret, direct,
			     ll_card->plat_form_dev->dev.coherent_dma_mask);
	return ret;
}

void higgs_use_total_kmem(struct higgs_card *ll_card, u32 size)
{
	ll_card->higgs_use_kmem += size;
}

void higgs_use_total_dma_mem(struct higgs_card *ll_card, u32 size)
{
	ll_card->higgs_use_dma_mem += size;
}

#define HIGGS_FREE_QUEUE_MEM(ll_card, name)	\
do {	\
	struct higgs_struct_info *higgs_hw_info = &ll_card->io_hw_struct_info;	\
    u32 size_per_##name = higgs_hw_info->entry_use_per_##name * (sizeof(struct higgs_##name##_info));	\
    if(NULL == higgs_hw_info->all_##name##_base) {	\
        break;	\
    }	\
    if(ll_card->higgs_can_##name > 0 && higgs_hw_info->entry_use_per_##name > 0) {	\
        higgs_free_dma_mem(ll_card, ll_card->higgs_can_##name * size_per_##name,	\
                higgs_hw_info->all_##name##_base, higgs_hw_info->all_##name##_dma_base);	\
    }	\
    higgs_hw_info->all_##name##_base = NULL;	\
}while(0)

/*----------------------------------------------*
 * DQ data structure in higgs                                      *
 *----------------------------------------------*/
static void higgs_free_dq_mem(struct higgs_card *ll_card)
{
	HIGGS_FREE_QUEUE_MEM(ll_card, dq);

	return;
}

/*----------------------------------------------*
 * DQ data structure initialize in higgs                                      *
 *----------------------------------------------*/
static u32 higgs_init_dq_mem(struct higgs_card *ll_card)
{
	struct higgs_struct_info *higgs_hw_info = &ll_card->io_hw_struct_info;
	struct higgs_dq_info *dq_info = NULL;
	OSP_DMA dq_dma_mem = 0;
	u32 queue_id = 0;
	u32 dq_queue_size = 0;
	struct higgs_dqueue *higgs_dq = NULL;

	if (ll_card->higgs_can_dq > 0 && higgs_hw_info->entry_use_per_dq > 0) {
		/*allocte memory for all the DQ */
		dq_queue_size =
		    higgs_hw_info->entry_use_per_dq * HIGGS_DQ_ENTRY_SIZE;
		dq_info =
		    higgs_alloc_dma_mem(ll_card,
					ll_card->higgs_can_dq * dq_queue_size,
					&dq_dma_mem);

		if (NULL == dq_info) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 1,
				    "Higgs Alloc DQ MemSize[%d] fail!",
				    ll_card->higgs_can_dq * dq_queue_size);
			return ERROR;
		} else {	//  所有dq长度总和
			HIGGS_TRACE(OSP_DBL_INFO, 1,
				    "higgs card %d alloc %d dq mem, dq_info %p, dq_dma_mem %llx",
				    ll_card->card_id,
				    ll_card->higgs_can_dq * dq_queue_size,
				    dq_info, dq_dma_mem);
		}

		HIGGS_TRACE(OSP_DBL_INFO, 100,
			    "maxCanDq is :%d, dq Entry Size is:%d",
			    higgs_hw_info->entry_use_per_dq,
			    (u32) HIGGS_DQ_ENTRY_SIZE);
		memset(dq_info, 0, ll_card->higgs_can_dq * dq_queue_size);
		higgs_use_total_dma_mem(ll_card,
					ll_card->higgs_can_dq * dq_queue_size);

		/*free use */
		higgs_hw_info->all_dq_base = dq_info;
		higgs_hw_info->all_dq_dma_base = dq_dma_mem;
		for (queue_id = 0; queue_id < ll_card->higgs_can_dq; queue_id++) {
			higgs_dq = &higgs_hw_info->delivery_queue[queue_id];
			higgs_dq->queue_base = dq_info;
			higgs_dq->queue_dma_base = dq_dma_mem;
			higgs_dq->dq_depth = higgs_hw_info->entry_use_per_dq;
			higgs_dq->wpointer = 0;
//            higgs_dq->uiCurPointer = 0;
			/*find next dq */
			dq_info += higgs_hw_info->entry_use_per_dq;
			dq_dma_mem += dq_queue_size;
			HIGGS_TRACE(OSP_DBL_INFO, 1,
				    "DQ %d virt addr %p , dma addr %llx, depth %d",
				    queue_id, higgs_dq->queue_base,
				    higgs_dq->queue_dma_base,
				    higgs_dq->dq_depth);
		}
	}

	return OK;
}

/**
 * higgs_free_cq_mem - free CQ data structure in higgs.
 *
 */
static void higgs_free_cq_mem(struct higgs_card *ll_card)
{
	//HIGGS_FREE_QUEUE_MEM(ll_card, CQ);
	/* modify by chenqilin */
	//HIGGS_FREE_QUEUE_CQ_MEM(ll_card, cq);
	HIGGS_FREE_QUEUE_MEM(ll_card, cq);

	return;
}

/**
 * higgs_init_cq_mem - initialize CQ data structure in higgs.
 *
 */
static u32 higgs_init_cq_mem(struct higgs_card *ll_card)
{
	struct higgs_struct_info *higgs_hw_info = &ll_card->io_hw_struct_info;
	struct higgs_cq_info *cq_info = NULL;
	OSP_DMA cq_dma_mem = 0;
	u32 queue_id = 0;
	u32 cq_queue_size = 0;
	struct higgs_cqueue *higgs_cq = NULL;

	if (ll_card->higgs_can_cq > 0 && higgs_hw_info->entry_use_per_cq > 0) {
		/*allocte memory for all the DQ */
		cq_queue_size =
		    higgs_hw_info->entry_use_per_cq * HIGGS_CQ_ENTRY_SIZE;
		cq_info =
		    higgs_alloc_dma_mem(ll_card,
					ll_card->higgs_can_cq * cq_queue_size,
					&cq_dma_mem);

		if (NULL == cq_info) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 1,
				    "Higgs Alloc CQ MemSize[%d] fail!",
				    ll_card->higgs_can_cq * cq_queue_size);
			return ERROR;
		}
		HIGGS_TRACE(OSP_DBL_INFO, 1,
			    "higgs card %d alloc %d CQ mem, cq_info %p",
			    ll_card->card_id,
			    ll_card->higgs_can_cq * cq_queue_size, cq_info);
		memset(cq_info, 0, ll_card->higgs_can_cq * cq_queue_size);
		higgs_use_total_dma_mem(ll_card,
					ll_card->higgs_can_cq * cq_queue_size);

		/*free use */
		higgs_hw_info->all_cq_base = cq_info;
		higgs_hw_info->all_cq_dma_base = cq_dma_mem;
		for (queue_id = 0; queue_id < ll_card->higgs_can_cq; queue_id++) {
			higgs_cq = &higgs_hw_info->complete_queue[queue_id];

			higgs_cq->queue_base = cq_info;
			higgs_cq->queue_dma_base = cq_dma_mem;

			higgs_cq->cq_depth = higgs_hw_info->entry_use_per_cq;
			higgs_cq->rpointer = 0;
			HIGGS_TRACE(OSP_DBL_INFO, 1,
				    "CQ %d virt addr %p , dma addr %llx, depth %d",
				    queue_id, higgs_cq->queue_base,
				    higgs_cq->queue_dma_base,
				    higgs_cq->cq_depth);

			/*find next dq */
			cq_info += higgs_hw_info->entry_use_per_cq;
			cq_dma_mem += cq_queue_size;

		}
	}

	return OK;
}

/**
 * higgs_init_hw_struct_mem - initialize DQ/CQ/ITCT/IOST data structure in higgs.
 *
 */
static s32 higgs_init_hw_struct_mem(struct higgs_card *ll_card)
{
	struct higgs_struct_info *higgs_hw_info = &ll_card->io_hw_struct_info;

	 /*DQ*/ if (OK != higgs_init_dq_mem(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1, "Higgs Alloc DQ fail!");
		goto dq_fail;
	}

	/*CQ*/ if (OK != higgs_init_cq_mem(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1, "Higgs Alloc CQ fail!");
		goto cq_fail;
	}

	/*iost */
	higgs_hw_info->iost_size =
	    HIGGS_IOST_ENTRY_SIZE * ll_card->higgs_can_io;
	higgs_hw_info->iost_base =
	    higgs_alloc_dma_mem(ll_card, higgs_hw_info->iost_size,
				&higgs_hw_info->iost_dma_base);
	if (NULL == higgs_hw_info->iost_base) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1,
			    "Higgs Alloc IOST MemSize[%d]bytes fail!",
			    higgs_hw_info->iost_size);
		goto iost_fail;
	}
	HIGGS_TRACE(OSP_DBL_INFO, 1,
		    "higgs card %d alloc %d iost mem, iost_base %p",
		    ll_card->card_id, higgs_hw_info->iost_size,
		    higgs_hw_info->iost_base);
	memset(higgs_hw_info->iost_base, 0, higgs_hw_info->iost_size);
	higgs_use_total_dma_mem(ll_card, higgs_hw_info->iost_size);

	/*itct */
	higgs_hw_info->itct_size =
	    HIGGS_ITCT_ENTRY_SIZE * ll_card->higgs_can_dev;
	higgs_hw_info->itct_base =
	    higgs_alloc_dma_mem(ll_card, higgs_hw_info->itct_size,
				&higgs_hw_info->itct_dma_base);
	if (NULL == higgs_hw_info->itct_base) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1,
			    "Higgs Alloc ITCT MemSize[%d]bytes fail!",
			    higgs_hw_info->itct_size);
		goto itct_fail;
	}
	HIGGS_TRACE(OSP_DBL_INFO, 1,
		    "higgs card %d alloc %d itct mem, itct_base%p, dmaBaseAddr %llx",
		    ll_card->card_id, higgs_hw_info->itct_size,
		    higgs_hw_info->itct_base, higgs_hw_info->itct_dma_base);
	memset(higgs_hw_info->itct_base, 0, higgs_hw_info->itct_size);
	higgs_use_total_dma_mem(ll_card, higgs_hw_info->itct_size);

	/*break point */
	higgs_hw_info->break_point_size =
	    HIGGS_BREAK_POINT_ENTRY_SIZE * ll_card->higgs_can_io;
	higgs_hw_info->break_point =
	    higgs_alloc_dma_mem(ll_card, higgs_hw_info->break_point_size,
				&higgs_hw_info->dbreak_point);
	if (NULL == higgs_hw_info->break_point) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1,
			    "Higgs Alloc BreakPoint MemSize[%d]bytes fail!",
			    higgs_hw_info->break_point_size);
		goto breakpoint_fail;
	}
	HIGGS_TRACE(OSP_DBL_INFO, 1,
		    "higgs card %d alloc %d byte break mem, break_point %p",
		    ll_card->card_id, higgs_hw_info->break_point_size,
		    higgs_hw_info->break_point);
	memset(higgs_hw_info->break_point, 0, higgs_hw_info->break_point_size);
	higgs_use_total_dma_mem(ll_card, higgs_hw_info->break_point_size);

	return OK;

      breakpoint_fail:
	higgs_free_dma_mem(ll_card, higgs_hw_info->break_point_size,
			   higgs_hw_info->break_point,
			   higgs_hw_info->dbreak_point);
	higgs_hw_info->itct_base = NULL;

      itct_fail:
	higgs_free_dma_mem(ll_card, higgs_hw_info->itct_size,
			   higgs_hw_info->itct_base,
			   higgs_hw_info->itct_dma_base);
	higgs_hw_info->itct_base = NULL;

      iost_fail:
	higgs_free_dma_mem(ll_card, higgs_hw_info->iost_size,
			   higgs_hw_info->iost_base,
			   higgs_hw_info->iost_dma_base);
	higgs_hw_info->iost_base = NULL;

      cq_fail:
	higgs_free_cq_mem(ll_card);

      dq_fail:
	higgs_free_dq_mem(ll_card);

	return ERROR;

}

/**
 * higgs_free_hw_struct_mem - de-initialize DQ/CQ/ITCT/IOST data structure in higgs.
 *
 */
static void higgs_free_hw_struct_mem(struct higgs_card *ll_card)
{
	struct higgs_struct_info *higgs_hw_info = &ll_card->io_hw_struct_info;

	/*free BreakPoint memory */
	if (higgs_hw_info->break_point != NULL) {
		higgs_free_dma_mem(ll_card, higgs_hw_info->break_point_size,
				   higgs_hw_info->break_point,
				   higgs_hw_info->dbreak_point);
		higgs_hw_info->break_point = NULL;
	}

	/*free ITCT memory */
	if (higgs_hw_info->itct_base != NULL) {
		higgs_free_dma_mem(ll_card, higgs_hw_info->itct_size,
				   higgs_hw_info->itct_base,
				   higgs_hw_info->itct_dma_base);
		higgs_hw_info->itct_base = NULL;
	}

	/*free IOST memory */
	if (higgs_hw_info->iost_base != NULL) {
		higgs_free_dma_mem(ll_card, higgs_hw_info->iost_size,
				   higgs_hw_info->iost_base,
				   higgs_hw_info->iost_dma_base);
		higgs_hw_info->iost_base = NULL;
	}

	/*free CQ memory */
	higgs_free_cq_mem(ll_card);

	/*free DQ memory */
	higgs_free_dq_mem(ll_card);

	return;

}

/**
 * higgs_init_cmd_tbl_pool -maintenance memory allocate and initialize for COMMAND TABLE POOL data structure in higgs.
 *
 */
static s32 higgs_init_cmd_tbl_pool(struct higgs_card *ll_card)
{
	u32 i = 0;
	struct higgs_cmd_table *cmd_tbl = NULL;
	OSP_DMA cmd_tbl_dma_mem = 0;
	u32 size_per_cmd_tbl = 0;
	struct higgs_command_table_pool *cmd_tbl_pool = NULL;

	cmd_tbl_pool = &ll_card->cmd_table_pool;
	/*allocte memory for all the command table */
	for (i = 0; i < ll_card->higgs_can_io; i++) {
		size_per_cmd_tbl = sizeof(struct higgs_cmd_table);
		cmd_tbl =
		    higgs_alloc_dma_mem(ll_card, size_per_cmd_tbl,
					&cmd_tbl_dma_mem);
		if (NULL == cmd_tbl) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 1,
				    "higgs command table mem size[%d] alloc fail!\n",
				    size_per_cmd_tbl);
			return ERROR;
		}
		/*init command table address */
		memset(cmd_tbl, 0, size_per_cmd_tbl);
		if ((i % 256) == 0) {
			HIGGS_TRACE(OSP_DBL_INFO, 2694,
				    "size_per_cmd_tbl %d, i %d, cmd_tbl %p, cmd_tbl_dma_mem %llx",
				    size_per_cmd_tbl, i, cmd_tbl,
				    cmd_tbl_dma_mem);
		}

		cmd_tbl_pool->table_entry[i].table_addr = (void *)cmd_tbl;
		cmd_tbl_pool->table_entry[i].table_dma_addr = cmd_tbl_dma_mem;
		/*record allocated dma mem */
		cmd_tbl_pool->malloc_size = size_per_cmd_tbl;
		higgs_use_total_dma_mem(ll_card, cmd_tbl_pool->malloc_size);
	}

	return OK;

}

/**
 * higgs_free_cmd_tbl_pool -re-initialize for COMMAND TABLE data structure in higgs.
 *
 */
static void higgs_free_cmd_tbl_pool(struct higgs_card *ll_card)
{
	struct higgs_command_table_pool *cmd_tbl_pool = NULL;
	u32 i = 0;

	HIGGS_ASSERT(ll_card != NULL, return);
	/*free command table memory */
	cmd_tbl_pool = &ll_card->cmd_table_pool;
	for (i = 0; i < ll_card->higgs_can_io; i++) {
		if (cmd_tbl_pool->table_entry[i].table_addr != NULL) {
			higgs_free_dma_mem(ll_card, cmd_tbl_pool->malloc_size,
					   cmd_tbl_pool->table_entry[i].
					   table_addr,
					   cmd_tbl_pool->table_entry[i].
					   table_dma_addr);
			cmd_tbl_pool->table_entry[i].table_addr = NULL;
		}
	}
}

/**
 * higgs_init_sge_pool -maintenance memory allocate and initialize for SGE POOL data structure in higgs.
 *
 */
static s32 higgs_init_sge_pool(struct higgs_card *ll_card)
{
	u32 page;
	u32 sge_pool_size = 0;
	struct higgs_sge_entry_page *base_sge_addr = NULL;
	OSP_DMA base_sge_dma_addr = 0;
	struct higgs_sge_pool *sge_pool = NULL;
	u32 size_per_sge = sizeof(struct higgs_sge_entry_page);	/*every scale page size */ 

	/*allocte memory for all the sge entry */
	sge_pool = &ll_card->sge_pool;
	for (page = 0; page < ll_card->higgs_can_io; page++) {
		base_sge_addr =
		    higgs_alloc_dma_mem(ll_card, size_per_sge,
					&base_sge_dma_addr);
		if (NULL == base_sge_addr) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 1,
				    "higgs sge entry mem size[%d] alloc fail!\n",
				    size_per_sge);
			return ERROR;
		}

		/*init sge entry struct member */
		memset(base_sge_addr, 0, size_per_sge);
		sge_pool->sge_entry[page].sge_addr = (void *)(base_sge_addr);
		sge_pool->sge_entry[page].sge_dma_addr =
		    (OSP_DMA) base_sge_dma_addr;
		sge_pool_size += size_per_sge;
		if ((page % 256) == 0) {
			HIGGS_TRACE(OSP_DBL_INFO, 2694,
				    "size_per_sge %d, page %d, base_sge_addr %p, base_sge_dma_addr %llx",
				    size_per_sge, page, base_sge_addr,
				    base_sge_dma_addr);
		}
	}

	/*record allocated dma memory */
	higgs_use_total_dma_mem(ll_card, sge_pool_size);
	HIGGS_TRACE(OSP_DBL_INFO, 1231,
		    "higgs card %d alloc %d sge page, sum %d byte",
		    ll_card->card_id, page, sge_pool_size);
	return OK;
}

/**
 * higgs_free_cmd_tbl_pool -re-initialize for SGE data structure in higgs.
 *
 */
static void higgs_free_sge_pool(struct higgs_card *ll_card)
{
	u32 page;
	struct higgs_sge_pool *sge_pool = NULL;
	u32 size_per_sge = sizeof(struct higgs_sge_entry_page); /*every scale page size */ 


	/*free sge memory */
	sge_pool = &ll_card->sge_pool;
	for (page = 0; page < ll_card->higgs_can_io; page++) {
		if (sge_pool->sge_entry[page].sge_addr != NULL) {
			higgs_free_dma_mem(ll_card, size_per_sge,
					   sge_pool->sge_entry[page].sge_addr,
					   sge_pool->sge_entry[page].
					   sge_dma_addr);
			sge_pool->sge_entry[page].sge_addr = NULL;
		}
	}
}

/**
 * higgs_free_cmd_tbl_pool -initialize data structure of interaction between software and hardware in higgs.
 *
 */
static s32 higgs_init_hw_mem(struct higgs_card *ll_card)
{
	if (OK != higgs_init_hw_struct_mem(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 0,
			    "Init HwStruct Fail, already DmaMemSize[%d]bytes!\n",
			    ll_card->higgs_use_dma_mem);
		goto InitHwMem_FAIL;
	}

	if (OK != higgs_init_cmd_tbl_pool(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 0,
			    "Init CmdTblPool Fail, already DmaMemSize[%d]bytes!",
			    ll_card->higgs_use_dma_mem);
		goto InitCmdTbl_FAIL;
	}

	if (OK != higgs_init_sge_pool(ll_card)) {

		HIGGS_TRACE(OSP_DBL_MAJOR, 0,
			    "Init SgePool Fail, already DmaMemSize[%d]bytes!",
			    ll_card->higgs_use_dma_mem);
		goto InitSgePool_FAIL;
	}

	return OK;

      InitSgePool_FAIL:
	higgs_free_cmd_tbl_pool(ll_card);

      InitCmdTbl_FAIL:
	higgs_free_hw_struct_mem(ll_card);

      InitHwMem_FAIL:

	return ERROR;

}

/**
 * higgs_free_hw_mem - re-initialize data structure of interaction between software and hardware in higgs.
 *
 */
static s32 higgs_free_hw_mem(struct higgs_card *ll_card)
{
	HIGGS_ASSERT(ll_card != NULL, return ERROR);

	higgs_free_sge_pool(ll_card);

	higgs_free_cmd_tbl_pool(ll_card);

	higgs_free_hw_struct_mem(ll_card);

	return OK;

}

/**
 * higgs_init_req_mem -maintenance memory allocate and initialize for REQ data structure in higgs.
 *
 */
static s32 higgs_init_req_mem(struct higgs_card *ll_card)
{
	u32 i = 0;
	u32 req_mem_use = 0;
	struct higgs_req *req = NULL;
	struct higgs_req_manager *req_mgr = NULL;

	HIGGS_ASSERT(ll_card != NULL, return ERROR);

	/*allcated memory for request */
	req_mgr = &ll_card->higgs_req_manager;
	req_mgr->req_cnt = ll_card->higgs_can_io;	/*atomic */ 

	req_mem_use = sizeof(struct higgs_req) * ll_card->higgs_can_io;
	req = HIGGS_KMALLOC(req_mem_use, GFP_KERNEL);
	if (NULL == req) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 1, "higgs req mem alloc fail!");
		return ERROR;
	}
	memset(req, 0, req_mem_use);

	/*remember the higgs request memory */
	req_mgr->req_mem = req;
	HIGGS_TRACE(OSP_DBL_INFO, 1, "higgs malloc %d req memory[0x%p]",
		    req_mem_use, req);

	/* request queue number */
	for (i = 0; i < HIGGS_MAX_REQ_LIST_NUM; i++) {
		SAS_SPIN_LOCK_INIT(&req_mgr->req_lock);
		SAS_INIT_LIST_HEAD(&req_mgr->req_list);
	}

	/* put req into free list*/
	req = (struct higgs_req *)req_mgr->req_mem;
	for (i = 0; i < ll_card->higgs_can_io; i++) {
		SAS_INIT_LIST_HEAD(&req->pool_entry);
        
#if defined(FPGA_VERSION_TEST) || defined(EVB_VERSION_TEST) || defined(PV660_ARM_SERVER)

		/* use bigger ipttt debug */ 
		SAS_LIST_ADD(&req->pool_entry, &req_mgr->req_list);
#else
		SAS_LIST_ADD_TAIL(&req->pool_entry, &req_mgr->req_list);
#endif
		req->iptt = i;
		req->req_state = HIGGS_REQ_STATE_FREE;
		req++;
	}

	higgs_use_total_kmem(ll_card, req_mem_use);

	return OK;
}

/**
 * higgs_free_req_mem - re-initialize for REQ data structure in higgs.
 *
 */
static s32 higgs_free_req_mem(struct higgs_card *ll_card)
{
	struct higgs_req_manager *req_mgr = NULL;

	HIGGS_ASSERT(ll_card != NULL, return ERROR);
	req_mgr = &ll_card->higgs_req_manager;

	if (req_mgr->req_mem) {
		SAS_LIST_DEL_INIT(&req_mgr->req_list);
		HIGGS_KFREE(req_mgr->req_mem);
		req_mgr->req_mem = NULL;
	}

	return OK;
}

/**
 * higgs_init_req_mem -maintenance memory allocate and initialize for DEV data structure in higgs.
 *
 */
static s32 higgs_init_dev_mem(struct higgs_card *ll_card)
{
	u32 len = 0;
	u32 idx = 0;
	u32 max_tgt = 0;
	struct higgs_device *dev = NULL;
	/*allocate DEV memory */
	/*driver only need to allocate memory for store device */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);

	/*chip reset */
	max_tgt = ll_card->higgs_can_dev;
	if (max_tgt > HIGGS_MAX_DEV_NUM) {
		max_tgt = HIGGS_MAX_DEV_NUM;
	}
	len = sizeof(struct higgs_device) * max_tgt;
	ll_card->dev_mem = HIGGS_KMALLOC(len, GFP_ATOMIC);
	if (NULL == ll_card->dev_mem) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 100,
			    "Card:%d alloc dev memory failed",
			    ll_card->card_id);
		return ERROR;
	}
	HIGGS_TRACE(OSP_DBL_INFO, 1, "higgs malloc %d byte dev memory[0x%p]",
		    len, ll_card->dev_mem);
	memset(ll_card->dev_mem, 0, len);
	higgs_use_total_kmem(ll_card, len);

	SAS_SPIN_LOCK_INIT(&ll_card->card_free_dev_lock);
	SAS_INIT_LIST_HEAD(&ll_card->card_free_dev_list);

	dev = (struct higgs_device *)ll_card->dev_mem;
	for (idx = 0; idx < max_tgt; idx++) {

		dev->dev_id = idx;

#if defined(FPGA_VERSION_TEST) || defined(EVB_VERSION_TEST) || defined(PV660_ARM_SERVER)

		SAS_LIST_ADD(&dev->list_node, &ll_card->card_free_dev_list);
#else
		SAS_LIST_ADD_TAIL(&dev->list_node,
				  &ll_card->card_free_dev_list);
#endif
		dev++;
	}
	return OK;
}

/**
 * higgs_free_dev_mem - re-initialize for maintenance DEV  data structure in higgs.
 *
 */
static s32 higgs_free_dev_mem(struct higgs_card *ll_card)
{

	HIGGS_ASSERT(ll_card != NULL, return ERROR);
	if (ll_card->dev_mem) {
		SAS_LIST_DEL_INIT(&ll_card->card_free_dev_list);
		HIGGS_KFREE(ll_card->dev_mem);
		ll_card->dev_mem = NULL;
	}

	return OK;
}

/**
 * higgs_init_sw_mem - initialize entrance function for maintenance REQ/DEV  data structure in higgs.
 *
 */
static s32 higgs_init_sw_mem(struct higgs_card *ll_card)
{
	if (OK != higgs_init_req_mem(ll_card)) {
		goto InitHiggsReqMem_FAIL;
	}

	if (OK != higgs_init_dev_mem(ll_card)) {
		goto InitHiggsDevMem_FAIL;
	}

	return OK;

      InitHiggsDevMem_FAIL:
	(void)higgs_free_req_mem(ll_card);

      InitHiggsReqMem_FAIL:

	return ERROR;

}

/**
 * higgs_free_sw_mem - re-initialize  REQ/DEV  data structure in higgs.
 *
 */
static s32 higgs_free_sw_mem(struct higgs_card *ll_card)
{
	HIGGS_ASSERT(ll_card != NULL, return ERROR);

	(void)higgs_free_req_mem(ll_card);

	(void)higgs_free_dev_mem(ll_card);
	return OK;

}

/**
 * higgs_init_rsc - initialize entrance function for higgs_phy and higgs_port.
 *
 */
static void higgs_init_rsc(struct higgs_card *ll_card)
{
	u32 i = 0;
	struct sal_card *sal_card = NULL;
	u64 tmp_sas_addr = 0;

	sal_card = ll_card->sal_card;

	/*initialize double controller mutex lock */
	SAS_SPIN_LOCK_INIT(&ll_card->card_lock);

	/*initialize PHY PHY */
	for (i = 0; i < HIGGS_MAX_PHY_NUM; i++) {
		ll_card->phy[i].phy_id = i;
		ll_card->phy[i].up_ll_phy = sal_card->phy[i];
		tmp_sas_addr = ll_card->card_cfg.phy_addr_high[i];
		tmp_sas_addr = (tmp_sas_addr << 32);	/*write high 32 bit address first . */
		tmp_sas_addr |= ll_card->card_cfg.phy_addr_low[i];

		sal_card->phy[i]->local_addr = tmp_sas_addr;
		memset(&sal_card->phy[i]->err_code, 0,
		       sizeof(struct sal_bit_err));
		SAS_SPIN_LOCK_INIT(&sal_card->phy[i]->err_code.err_code_lock);
		sal_card->phy[i]->err_code.bit_err_enable = true;	/*default open  */
		sal_card->phy[i]->link_status = SAL_PHY_CLOSED;
		sal_card->phy[i]->link_rate = SAL_LINK_RATE_FREE;
		sal_card->phy[i]->port_id = SAL_INVALID_PORTID;

		/* init timer heyousong for bug60 */
		ll_card->phy[i].run_serdes_fw_timer_en = false;
		(void)osal_lib_init_timer((void *)&ll_card->phy[i].
				     phy_run_serdes_fw_timer,
				     (unsigned long)30 * HZ,
				     (unsigned long)ll_card->card_id,
				     higgs_serdes_for_12g_timer_hander);

		/* phy stop rsp simulation */
		ll_card->phy[i].to_notify_phy_stop_rsp = false;
		/* BEGIN: Added by c00257296, 2015/1/22   PN:arm_server */
#if defined(PV660_ARM_SERVER)
		ll_card->phy[i].phy_is_idle = true;
		ll_card->phy[i].last_time_idle = true;


		(void)osal_lib_init_timer((void *)&ll_card->phy[i].
				     phy_dma_status_timer,
				     (unsigned long)100 * HZ,
				     (unsigned long)ll_card->card_id,
				     higgs_check_dma_txrx);
#endif
		/* END:   Added by c00257296, 2015/1/22 */
	}

	/*initialize PORT */
	for (i = 0; i < HIGGS_MAX_PORT_NUM; i++) {	// LL port
		ll_card->port[i].status = HIGGS_PORT_FREE;
		ll_card->port[i].port_id = HIGGS_INVALID_PORT_ID;
		ll_card->port[i].last_phy = HIGGS_INVALID_PHY_ID;
		//SAS_ATOMIC_SET(&ll_card->port[i].atRefCount,0);
		ll_card->port[i].index = i;
		ll_card->port[i].phy_bitmap = 0;

		ll_card->port[i].up_ll_port = sal_card->port[i];
		sal_card->port[i]->ll_port = &ll_card->port[i];


		/* port wire hotplug simulation */
		ll_card->port[i].to_notify_wire_hotplug = false;

		/* SAL */
		sal_card->port[i]->set_dev_state.print_cnt = 0;
		sal_card->port[i]->set_dev_state.last_print_time = 0;
		sal_card->port[i]->free_all_dev.print_cnt = 0;
		sal_card->port[i]->free_all_dev.last_print_time = 0;
	}

	/*initialize other software resource  */
	for (i = 0; i < HIGGS_MAX_DQ_NUM; i++)	/* DQ number */
		/* initialize DQ queue lock */  
		SAS_SPIN_LOCK_INIT(HIGGS_DQ_LOCK(ll_card, i));



	/*initialize resource for EH */
	for (i = 0; i < HIGGS_MAX_IOST_CACHE_NUM; i++)
		ll_card->iost_cache_info.iost_cache_array[i] = 0;

	ll_card->iost_cache_info.save_cache_iost_jiff = 0;
	ll_card->ilegal_iptt_cnt = 0;

	return;
}

/**
 * higgs_init_mem -entrance function of  memory allocate and initialize for data structure in higgs.
 *
 */
static s32 higgs_init_mem(struct higgs_card *ll_card)
{
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	if (OK != higgs_init_hw_mem(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 5000, "Init Hw Mem fail!");
		goto InitHwMem_FAIL;
	}

	if (OK != higgs_init_sw_mem(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 5001, "Init Soft Mem fail!");
		goto InitSoftMem_FAIL;
	}

	return OK;

      InitSoftMem_FAIL:
	(void)higgs_free_hw_mem(ll_card);

      InitHwMem_FAIL:
	return ERROR;
}

/**
 * higgs_free_mem -re-initialize for REQ/DEV data structure in higgs.
 *
 */
static s32 higgs_free_mem(struct higgs_card *ll_card)
{
	HIGGS_ASSERT(ll_card != NULL, return ERROR);

	(void)higgs_free_sw_mem(ll_card);

	(void)higgs_free_hw_mem(ll_card);

	return OK;

}

/**
 * higgs_clr_mem_after_reset -clear memory of allocation when chip reset .
 *
 */
static void higgs_clr_mem_after_reset(struct higgs_card *ll_card)
{
	struct higgs_struct_info *higgs_hw_info = &ll_card->io_hw_struct_info;
	//struct higgs_cq_info *cq_info = NULL;
	//struct higgs_dq_info *dq_info = NULL;     
	//u32 cq_queue_size = 0;
	//u32 dq_queue_size = 0;    
	u32 i = 0;
	struct higgs_cmd_table *cmd_tbl = NULL;
	struct higgs_command_table_pool *cmd_tbl_pool = NULL;
	u32 size_per_cmd_tbl = sizeof(struct higgs_cmd_table);
	u32 page;
	struct higgs_sge_entry_page *base_sge_addr = NULL;
	struct higgs_sge_pool *sge_pool = NULL;
	u32 size_per_sge = sizeof(struct higgs_sge_entry_page);

/*
    //clear all DQ memory 
    dq_info = higgs_hw_info->all_dq_base;
    dq_queue_size = higgs_hw_info->entry_use_per_dq * HIGGS_DQ_ENTRY_SIZE;
    memset(dq_info,0x0, ll_card->higgs_can_dq*dq_queue_size);	

    //clear all CQ memory
    cq_info = higgs_hw_info->all_cq_base;
    cq_queue_size = higgs_hw_info->entry_use_per_cq * HIGGS_CQ_ENTRY_SIZE;		
    memset(cq_info,0x0, ll_card->higgs_can_cq * cq_queue_size);
*/
	// clear iost,itct,breakpoint
	memset(higgs_hw_info->iost_base, 0, higgs_hw_info->iost_size);
	memset(higgs_hw_info->itct_base, 0, higgs_hw_info->itct_size);
	memset(higgs_hw_info->break_point, 0, higgs_hw_info->break_point_size);

	//clear all cmd table memory
	cmd_tbl_pool = &ll_card->cmd_table_pool;
	for (i = 0; i < ll_card->higgs_can_io; i++) {
		cmd_tbl = cmd_tbl_pool->table_entry[i].table_addr;
		memset(cmd_tbl, 0, size_per_cmd_tbl);
	}

	//clear sge pool
	sge_pool = &ll_card->sge_pool;
	for (page = 0; page < ll_card->higgs_can_io; page++) {
		base_sge_addr =
		    (struct higgs_sge_entry_page *)((void *)sge_pool->
						    sge_entry[page].sge_addr);
		memset(base_sge_addr, 0, size_per_sge);

	}

	//clear chip error count
	memset(&(ll_card->chip_err_fatal_stat), 0,
	       sizeof(struct higgs_chip_err_fatal_stat));

	/* clear illegal iptt count */
	ll_card->ilegal_iptt_cnt = 0;

	return;
}

/**
 * higgs_init_reg -register initialize function.
 *
 */
static void higgs_init_reg(struct higgs_card *ll_card)
{
	u32 idx = 0;
	//u32 uiPIndex = 0;
	u32 phy_id = 0;
	u32 val = 0;
	u64 offset = 0;
	//u32 uiQueIndex = 0;
	struct higgs_dqueue *higgs_dq = NULL;
	struct higgs_cqueue *higgs_cq = NULL;
	struct higgs_struct_info *higgs_hw_info = &ll_card->io_hw_struct_info;

	HIGGS_ASSERT(NULL != ll_card, return);

	/*global reg init */
	for (idx = 0; idx < HIGGS_ARRAY_ELEMENT_COUNT(global_reg_cfg); idx++) {
		if (HIGGS_REG_ENABLE == global_reg_cfg[idx].enable) {
			offset = global_reg_cfg[idx].reg_addr;
			HIGGS_GLOBAL_REG_WRITE(ll_card, offset,
					       global_reg_cfg[idx].reg_value);
			val = HIGGS_GLOBAL_REG_READ(ll_card, offset);
			HIGGS_TRACE_LIMIT(OSP_DBL_INFO, 1 * HZ, 3, 1123,
					  "GlobalOffset[0x%llx]WriteValue[0x%x]",
					  offset, val);
		}
	}

	/*port reg init */
	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		for (idx = 0; idx < HIGGS_ARRAY_ELEMENT_COUNT(port_reg_cfg);
		     idx++) {
			if (HIGGS_REG_ENABLE == port_reg_cfg[idx].enable) {
				offset = port_reg_cfg[idx].reg_addr;
				HIGGS_PORT_REG_WRITE(ll_card, phy_id, offset,
						     port_reg_cfg[idx].
						     reg_value);
				val =
				    HIGGS_PORT_REG_READ(ll_card, phy_id,
							offset);
				HIGGS_TRACE_LIMIT(OSP_DBL_INFO, 1 * HZ, 3, 1654,
						  "PortId[%d]Offset[0x%llx]WriteValue[0x%x]",
						  phy_id, offset, val);
			}
		}
	}

	/*initialize DQ base address and depth .  */
	for (idx = 0; idx < ll_card->higgs_can_dq; idx++) {
		higgs_dq = &higgs_hw_info->delivery_queue[idx];
		offset =
		    HISAS30HV100_GLOBAL_REG_DLVRY_QUEUE_BASE_ADDRL_0_REG +
		    (u64) 20 *idx;
		HIGGS_GLOBAL_REG_WRITE(ll_card, offset,
				       HIGGS_ADDRESS_LO_GET(higgs_dq->
							    queue_dma_base));
		offset =
		    HISAS30HV100_GLOBAL_REG_DLVRY_QUEUE_BASE_ADDRU_0_REG +
		    (u64) 20 *idx;
		HIGGS_GLOBAL_REG_WRITE(ll_card, offset,
				       HIGGS_ADDRESS_HI_GET(higgs_dq->
							    queue_dma_base));
		offset =
		    HISAS30HV100_GLOBAL_REG_DLVRY_QUEUE_DEPTH_0_REG +
		    (u64) 20 *idx;
		HIGGS_GLOBAL_REG_WRITE(ll_card, offset, higgs_dq->dq_depth);
		HIGGS_DQ_WPT_WRITE(ll_card, idx, 0);
	}

	/*initialize CQ base address and depth .  */
	for (idx = 0; idx < ll_card->higgs_can_cq; idx++) {
		higgs_cq = &higgs_hw_info->complete_queue[idx];
		offset =
		    HISAS30HV100_GLOBAL_REG_CMPLTN_QUEUE_BASE_ADDRL_0_REG +
		    (u64) 20 *idx;
		HIGGS_GLOBAL_REG_WRITE(ll_card, offset,
				       HIGGS_ADDRESS_LO_GET(higgs_cq->
							    queue_dma_base));
		offset =
		    HISAS30HV100_GLOBAL_REG_CMPLTN_QUEUE_BASE_ADDRU_0_REG +
		    (u64) 20 *idx;
		HIGGS_GLOBAL_REG_WRITE(ll_card, offset,
				       HIGGS_ADDRESS_HI_GET(higgs_cq->
							    queue_dma_base));
		offset =
		    HISAS30HV100_GLOBAL_REG_CMPLTN_QUEUE_DEPTH_0_REG +
		    (u64) 20 *idx;
		HIGGS_GLOBAL_REG_WRITE(ll_card, offset, higgs_cq->cq_depth);
		HIGGS_CQ_RPT_WRITE(ll_card, idx, 0);
	}

	/*ITCT table base address config*/
	offset = HISAS30HV100_GLOBAL_REG_ITCT_BASE_ADDRL_REG;
	HIGGS_GLOBAL_REG_WRITE(ll_card, offset,
			       HIGGS_ADDRESS_LO_GET(higgs_hw_info->
						    itct_dma_base));
	offset = HISAS30HV100_GLOBAL_REG_ITCT_BASE_ADDRU_REG;
	HIGGS_GLOBAL_REG_WRITE(ll_card, offset,
			       HIGGS_ADDRESS_HI_GET(higgs_hw_info->
						    itct_dma_base));

	/*IOST table base address config */
	offset = HISAS30HV100_GLOBAL_REG_IOST_BASE_ADDRL_REG;
	HIGGS_GLOBAL_REG_WRITE(ll_card, offset,
			       HIGGS_ADDRESS_LO_GET(higgs_hw_info->
						    iost_dma_base));
	offset = HISAS30HV100_GLOBAL_REG_IOST_BASE_ADDRU_REG;
	HIGGS_GLOBAL_REG_WRITE(ll_card, offset,
			       HIGGS_ADDRESS_HI_GET(higgs_hw_info->
						    iost_dma_base));

	/*break point base address config*/
	offset = HISAS30HV100_GLOBAL_REG_IO_BROKEN_MSG_BADDRL_REG;
	HIGGS_GLOBAL_REG_WRITE(ll_card, offset,
			       HIGGS_ADDRESS_LO_GET(higgs_hw_info->
						    dbreak_point));
	offset = HISAS30HV100_GLOBAL_REG_IO_BROKEN_MSG_BADDRU_REG;
	HIGGS_GLOBAL_REG_WRITE(ll_card, offset,
			       HIGGS_ADDRESS_HI_GET(higgs_hw_info->
						    dbreak_point));

	return;
}

#if 0 /* remove configFile.ini */

/*****************************************************************************
 函 数 名  : higgs_get_cfg_param
 功能描述  : 根据配置项读取配置变量的值
 输入参数  : char *section_name,
		struct higgs_cfg_item *cfg_param,
		u32 *cfg_val,
		u32 total_items
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
static s32 higgs_get_cfg_param(char *section_name,
			       struct higgs_cfg_item *cfg_param,
			       u32 * cfg_val, u32 total_items)
{
	u32 i = 0;
	struct drv_cst_item drv_cst_item;
	u32 *p_val = NULL;
	struct higgs_cfg_item *tmp_cfg_param = NULL;
	u32 val = 0;
	s32 ret = ERROR;

	memset(&drv_cst_item, 0, sizeof(struct drv_cst_item));
	tmp_cfg_param = cfg_param;
	p_val = cfg_val;
	for (i = 0; i < total_items; i++) {
		if ((NULL == tmp_cfg_param) || (NULL == p_val)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4632,
				    "Config name or value is null");
			return ERROR;
		}

		SAS_STRNCPY(drv_cst_item.name, tmp_cfg_param->name,
			    sizeof(drv_cst_item.name) /
			    sizeof(drv_cst_item.name[0]) - 1);
		drv_cst_item.name[sizeof(drv_cst_item.name) /
				  sizeof(drv_cst_item.name[0]) - 1] = '\0';

		if (0x00 == SAS_STRCMP("End", drv_cst_item.name))
			break;

		/*获取对应项的参数 */
		ret =
		    DRV_GetItem(section_name, drv_cst_item.name,
				drv_cst_item.value);
		if (ERROR == ret
		    && strcmp(drv_cst_item.name, "max_canqueue") == 0) {
			/* 允许此项目缺失，避免出包错误的风险，后续删除此代码 */
			(void)snprintf(drv_cst_item.value,
				       DRV_ITEM_VALUE_MAX_LEN, "%d",
				       tmp_cfg_param->default_val);
			HIGGS_TRACE(OSP_DBL_MINOR, 4633,
				    "Item:%s not exist, use default value %s",
				    drv_cst_item.name, drv_cst_item.value);
			ret = OK;
		} else if (ERROR == ret) {
			HIGGS_TRACE(OSP_DBL_MINOR, 4633,
				    "Item:%s not exist,please check "
				    "config file", drv_cst_item.name);
			return ERROR;
		}

		val = 0;
		ret = sal_special_str_to_ui(drv_cst_item.value, 0, &val);
		if (ERROR == ret) {	/*非法字符 */
			HIGGS_TRACE(OSP_DBL_MAJOR, 4634,
				    "item:%s get failed, please check config file",
				    drv_cst_item.name);
			return ERROR;
		}

		if ((val > tmp_cfg_param->max_val)
		    || (val < tmp_cfg_param->min_val)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4635,
				    "item:%s value:%d exceed normal",
				    drv_cst_item.name, val);
			return ERROR;
		} else {
			HIGGS_TRACE(OSP_DBL_DATA, 4636,
				    "item:%s succeed value:0x%x ",
				    drv_cst_item.name, val);
		}

		*p_val = val;

		tmp_cfg_param++;
		p_val++;
	}

	return OK;
}

/*****************************************************************************
 函 数 名  : higgs_get_comm_cfg
 功能描述  : 获取通用配置项
 输入参数  : void
 输出参数  : 无
 返 回 值  : void
*****************************************************************************/
s32 higgs_get_comm_cfg(void)
{
	u32 total_items = 0;
	s32 ret = ERROR;
	u8 prd_type = 0;

	/* 初始化全局变量 */
	memset(&higgs_comm_cfg, 0, sizeof(higgs_comm_cfg));

	total_items =
	    sizeof(higgs_common_cfg_param) / sizeof(struct higgs_cfg_item);
	ret =
	    higgs_get_cfg_param("sascbb_common_cfg", &higgs_common_cfg_param[0],
				(u32 *) (u64) (&higgs_comm_cfg), total_items);
	if (OK == ret) {
		sal_global.sal_comm_cfg.dev_miss_dly_time =
		    higgs_comm_cfg.dev_miss_dly_time;
		sal_global.sal_comm_cfg.slow_disk_interval =
		    higgs_comm_cfg.slow_disk_interval;
		sal_global.sal_comm_cfg.slow_period =
		    higgs_comm_cfg.slow_period;
		sal_global.sal_comm_cfg.check_period =
		    higgs_comm_cfg.check_period;
		sal_global.sal_comm_cfg.slow_disk_resp_time =
		    higgs_comm_cfg.slow_disk_resp_time;
	}

	/* 初始化产品类型 */
       if(NULL == sal_peripheral_operation.get_product_type)
       {
             prd_type = SPANGEA_V2R1;
       }else if (OK != sal_peripheral_operation.get_product_type(&prd_type)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 727, "Get card product type failed");
		return ERROR;
	}
	sal_global.prd_type = prd_type;	/*全局变量记录产品类型 */

	return ret;
}

/*****************************************************************************
 函 数 名  : higgs_get_card_cfg
 功能描述  : 获取卡特定配置项
 输入参数  :struct higgs_card *ll_card
 输出参数  : 无
 返 回 值  : s32
*****************************************************************************/
s32 higgs_get_card_cfg(struct higgs_card * ll_card)
{
	char sec_name[64] = "";
	u32 total_items = 0;
	u32 id = 0;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	id = ll_card->card_id;
	total_items =
	    sizeof(higgs_card_cfg_param) / sizeof(struct higgs_cfg_item);

	memset(sec_name, 0, sizeof(sec_name));

	(void)snprintf(sec_name, MAX_CFG_SETION_LEN - 1, "higgs_card_%d", id);
	HIGGS_TRACE(OSP_DBL_INFO, 4616,
		    "Card :%d item num:%d sec name:%s",
		    id, total_items, sec_name);
#if 1
	//lint -e740
	return higgs_get_cfg_param(sec_name, &higgs_card_cfg_param[0],
				   (u32 *) & (ll_card->card_cfg),
				   total_items);

	//lint +e740
#else
	//打桩部分
	Higgs_StubFillDefaultConfigInfo(ll_card);

	return OK;
#endif
}
#endif

#if defined(PV660_ARM_SERVER)

/**
 * higgs_get_sas_controller_cfg -obtain the sas controller config information.
 *
 */
s32 higgs_get_sas_controller_cfg(struct platform_device *dev, struct higgs_config_info* card_cfg)
{
    u32 i;
    s32 irq;
    const char *hilink_type;
    s32 err;
    char name[MAX_NAME_LEN];

    if (dev->dev.of_node == NULL) {
		printk("device node is NULL\n");
        return ERROR;
    }
    
	if (of_property_read_u32(dev->dev.of_node, "node-id", &card_cfg->cpu_node_id))
		return ERROR;
    printk("node-id:%d\n", card_cfg->cpu_node_id);

    if (of_property_read_u32(dev->dev.of_node, "controller-id", &card_cfg->id))
		return ERROR;
    printk("controller-id:%d\n", card_cfg->id);

    if (of_property_read_u32(dev->dev.of_node, "phy-num", &card_cfg->phy_num))
		return ERROR;
    printk("phy-num:%d\n", card_cfg->phy_num);

    if (of_property_read_u32(dev->dev.of_node, "phy-useage", &card_cfg->phy_useage))
		return ERROR;
	printk("phy-useage:0x%x\n", card_cfg->phy_useage);
    if (of_property_read_u32(dev->dev.of_node, "port-num", &card_cfg->port_num))
		return ERROR;
    printk("port-num:%d\n", card_cfg->port_num);

	/* g3 transmit cfg and phy sas address cfg */
	for (i = 0; i < HIGGS_MAX_PHY_NUM; i++) {
        card_cfg->phy_g3_cfg[i] = 0;
        card_cfg->phy_addr_high[i] = 0;
        card_cfg->phy_addr_low[i] = 0;
        
        if (i < card_cfg->phy_num) {
		    memset(name, 0, MAX_NAME_LEN);
		    (void)snprintf(name, MAX_NAME_LEN-1, "phy%d-addr-high", i);
            
		    if (of_property_read_u32(dev->dev.of_node, name, &card_cfg->phy_addr_high[i]))
		        return ERROR;
			printk("name=%s, val=%x\n", name, card_cfg->phy_addr_high[i]);
            
	        memset(name, 0, MAX_NAME_LEN);
		    (void)snprintf(name, MAX_NAME_LEN-1, "phy%d-addr-low", i);
		    if (of_property_read_u32(dev->dev.of_node, name, &card_cfg->phy_addr_low[i]))
		        return ERROR;
            printk("name=%s, val=%x\n", name, card_cfg->phy_addr_low[i]);

	        memset(name, 0, MAX_NAME_LEN);
	        (void)snprintf(name, MAX_NAME_LEN-1, "phy%d-g3-trasmit", i);
	        if (of_property_read_u32(dev->dev.of_node, name, &card_cfg->phy_g3_cfg[i]))
	            return ERROR;
            printk("name=%s, val=%x\n", name, card_cfg->phy_g3_cfg[i]);

        }
		card_cfg->phy_link_rate[i] = 0x04;
    }
    
    /* port bit map cfg */
 	for (i = 0; i < HIGGS_MAX_PORT_NUM; i++) {
        card_cfg->port_bitmap[i] = 0;
        if (i < card_cfg->port_num) {
	        memset(name, 0, MAX_NAME_LEN);
	        (void)snprintf(name, MAX_NAME_LEN-1, "port%d-bit-map", i);
	        if (of_property_read_u32(dev->dev.of_node, name, &card_cfg->port_bitmap[i]))
	            return ERROR;
            
            printk("port%d bitmap:0x%x\n", i, card_cfg->port_bitmap[i]);
        }        
    }

  	/* virtual IRQ resource */
	for (i = 0; i < MAX_INTERRUPT_TABLE_INDEX; i++) {
		irq = irq_of_parse_and_map(dev->dev.of_node, i);
		if (!irq) {
			return ERROR;
		}

		card_cfg->intr_table[i] = irq;
	}
	err =
	    of_property_read_string(dev->dev.of_node, "hilink-type",
				    &hilink_type);
	if (err < 0) {
		return ERROR;
	}

	if (!strcasecmp(hilink_type, "dsaf")) {
		card_cfg->hilink_type = HIGGS_HILINK_TYPE_DSAF;
        printk("hilink type:dsaf\n");
	} else if (!strcasecmp(hilink_type, "pcie")) {
		card_cfg->hilink_type = HIGGS_HILINK_TYPE_PCIE;
        printk("hilink type:pcie\n");
	} else {
		return ERROR;
	}

    /* other cfg, use default is OK */
	card_cfg->work_mode = SAL_PORT_MODE_INI;
    card_cfg->smp_tmo = 3;
    card_cfg->jam_tmo = 0;
	card_cfg->numinboundqueues = 4;
	card_cfg->numoutboundqueues = 4;
	card_cfg->ibqueue_num_elements = 1024;
	card_cfg->obqueue_num_elements = 1024;
	card_cfg->dev_queue_depth = 32;
	card_cfg->max_targets = 1024;
	card_cfg->max_canqueue = 1024;
	card_cfg->max_active_io = 4096;
	card_cfg->hwint_coal_timer = 0;
	card_cfg->hwint_coal_count = 0;
	card_cfg->ncq_switch = 0;
    card_cfg->biterr_stop_threshold = 10;
	card_cfg->biterr_interval_threshold = 5;
	card_cfg->biterr_routine_time = 1;
	card_cfg->biterr_routine_count = 20;
    
    return OK;
}

#endif

/**
 * higgs_recycle_active_req_rsc -recycle REQ resource in active list .
 *
 */
void higgs_recycle_active_req_rsc(struct higgs_card *ll_card)
{
	struct higgs_req *req = NULL;
	u32 i = 0;
	struct higgs_req_manager *req_mgr = NULL;
	unsigned long flag = 0;
	HIGGS_ASSERT(NULL != ll_card, return;
	    );

	/* req lock heyousong   */
	req_mgr = &ll_card->higgs_req_manager;
	for (i = 0; i < HIGGS_MAX_REQ_NUM; i++) {
		spin_lock_irqsave(&req_mgr->req_lock, flag);
		req = ll_card->running_req[i];
		if (NULL != req) {
                   /* don't callback SAL to notify exception */ 
			HIGGS_TRACE(OSP_DBL_INFO, 100,
				    "Card:%d free req tag:%d ",
				    ll_card->card_id, req->iptt);
			/* Is sal msg set NULL first ? */
			(void)higgs_req_state_chg(req, HIGGS_REQ_EVENT_HALFDONE,
						  false);

		}
		spin_unlock_irqrestore(&req_mgr->req_lock, flag);
	}
	return;
}

/**
 * higgs_get_card_limit - must be called after higgs_get_card_cfg.
 *
 */
void higgs_get_card_limit(struct higgs_card *ll_card)
{
	struct higgs_struct_info *higgs_hw_info = &ll_card->io_hw_struct_info;

	u32 dq_elem_num = ll_card->card_cfg.ibqueue_num_elements;
	u32 cq_elem_num = ll_card->card_cfg.obqueue_num_elements;
	u32 max_dq_num =
	    MIN(HIGGS_MAX_DQ_NUM, ll_card->card_cfg.numinboundqueues);
	u32 max_cq_num =
	    MIN(HIGGS_MAX_CQ_NUM, ll_card->card_cfg.numoutboundqueues);
	u32 max_io_num =
	    MIN(HIGGS_MAX_REQ_NUM, ll_card->card_cfg.max_active_io);
	u32 max_dev_num =
	    MIN(HIGGS_MAX_DEV_NUM, ll_card->card_cfg.max_targets);

	ll_card->card_cfg.max_active_io = max_io_num;
	ll_card->higgs_can_dq = max_dq_num;
	ll_card->higgs_can_cq = max_cq_num;
	ll_card->higgs_can_io = max_io_num;
	ll_card->higgs_can_dev = max_dev_num;

	higgs_hw_info->entry_use_per_dq = dq_elem_num;
	higgs_hw_info->entry_use_per_cq = cq_elem_num;
	HIGGS_TRACE(OSP_DBL_INFO, 100,
		    "maxCanDq is :%d, maxCanCq is:%d,max_active_io is %d",
		    ll_card->higgs_can_dq, ll_card->higgs_can_cq, max_io_num);
	//below 3 variables are  evaluated much times , is it necessary? */ 
	higgs_hw_info->itct_size =
	    HIGGS_ITCT_ENTRY_SIZE * ll_card->higgs_can_dev;
	higgs_hw_info->iost_size =
	    HIGGS_IOST_ENTRY_SIZE * ll_card->higgs_can_io;
	higgs_hw_info->break_point_size =
	    HIGGS_BREAK_POINT_ENTRY_SIZE * ll_card->higgs_can_io;

	return;
}

/**
 * higgs_add_sal_card - add the card to SAL.
 *
 */
static s32 higgs_add_sal_card(struct higgs_card *ll_card)
{

	struct sal_card *sal_card = NULL;

	sal_card = ll_card->sal_card;

	/* initialize all domain before add SAL Card.  */
	sal_card->pci_device = NULL;
	sal_card->dev = &(ll_card->plat_form_dev->dev);
	sal_card->dev_id = NULL;


	sal_card->config.dev_miss_dly_time = 3;
	sal_card->config.card_pos = (u8) ll_card->card_id;
	/* copy config  */
	sal_card->config.work_mode = ll_card->card_cfg.work_mode;
	sal_card->config.port_tmo = 0;	/* NOT USED */
	sal_card->config.smp_tmo = ll_card->card_cfg.smp_tmo;
	sal_card->config.jam_tmo = ll_card->card_cfg.jam_tmo;
	sal_card->config.num_inbound_queues =
	    ll_card->card_cfg.numinboundqueues;
	sal_card->config.num_outbound_queues =
	    ll_card->card_cfg.numoutboundqueues;
	sal_card->config.ib_queue_num_elems =
	    ll_card->card_cfg.ibqueue_num_elements;
	sal_card->config.ib_queue_elem_size = sizeof(struct higgs_dq_info);	/* NOT USED */
	sal_card->config.ob_queue_num_elems =
	    ll_card->card_cfg.obqueue_num_elements;
	sal_card->config.ob_queue_elem_size = sizeof(struct higgs_cq_info);	/* NOT USED */
	sal_card->config.dev_queue_depth =
	    ll_card->card_cfg.dev_queue_depth;
	sal_card->config.max_targets = ll_card->card_cfg.max_targets;
	sal_card->config.max_can_queues = ll_card->card_cfg.max_canqueue;
	sal_card->config.max_active_io = ll_card->card_cfg.max_active_io;
	sal_card->config.hw_int_coal_timer =
	    ll_card->card_cfg.hwint_coal_timer;
	sal_card->config.hw_int_coal_control =
	    ll_card->card_cfg.hwint_coal_count;
	sal_card->config.ncq_switch = ll_card->card_cfg.ncq_switch;
	sal_card->config.dif_switch = 0;	/* Closed,       NOT USED   */

	memcpy(&sal_card->config.phy_link_rate[0],
	       &ll_card->card_cfg.phy_link_rate[0],
	       MIN(sizeof(sal_card->config.phy_link_rate),
		   sizeof(ll_card->card_cfg.phy_link_rate))
	    );

	memcpy(&sal_card->config.phy_g3_cfg[0],
	       &ll_card->card_cfg.phy_g3_cfg[0],
	       MIN(sizeof(sal_card->config.phy_g3_cfg),
		   sizeof(ll_card->card_cfg.phy_g3_cfg))
	    );

	/* copy error code parameter */
	sal_card->config.bit_err_stop_threshold =
	    ll_card->card_cfg.biterr_stop_threshold;
	sal_card->config.bit_err_interval_threshold =
	    ll_card->card_cfg.biterr_interval_threshold;
	sal_card->config.bit_err_routine_time =
	    ll_card->card_cfg.biterr_routine_time;
	sal_card->config.bit_err_routine_cnt =
	    ll_card->card_cfg.biterr_routine_count;

	sal_card->card_clk_abnormal_num = 0;
	sal_card->last_clk_chk_time = jiffies;
	sal_card->card_clk_abnormal_flag = SAL_CARD_CLK_NORMAL;
	
	/* BCT parameter , default open */
	sal_card->config.bct_enable = true;

	/* TODO: register low-layer chip operation  */
	sal_card->ll_func.chip_op = higgs_chip_operation;
	sal_card->ll_func.phy_op = higgs_phy_operation;
	sal_card->ll_func.port_op = higgs_port_operation;
	sal_card->ll_func.dev_op = higgs_dev_operation;
	sal_card->ll_func.eh_abort_op = higgs_eh_abort_operation;
	sal_card->ll_func.send_msg = higgs_send_msg;
	sal_card->ll_func.send_tm = higgs_send_tm_msg;
	sal_card->ll_func.bitstream_op = higgs_bit_stream_operation;
	sal_card->ll_func.reg_op = higgs_reg_operation;
	sal_card->ll_func.gpio_op = higgs_gpio_operation;
	sal_card->ll_func.comm_ll_op = higgs_comm_ll_operation;

	return sal_add_card(sal_card);
	//lint +e506 +e944

}

/**
 * higgs_del_register_port - register SAS port.
 *
 */
s32 higgs_del_register_port(struct higgs_card * ll_card)
{
	//DRV_EVENT_INFO_S drv_event_info;
	u32 port_id = 0;
	struct sal_card *sal_card = NULL;
	u32 port_logic_id = 0;

	/* parameter check  */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	sal_card = ll_card->sal_card;
	HIGGS_ASSERT(NULL != sal_card, return ERROR);

	/* traverse port , report SAL event . */
	//memset(&drv_event_info, 0, sizeof(DRV_EVENT_INFO_S));
	for (port_id = 0; port_id < HIGGS_MAX_PORT_NUM; port_id++) {
		if (0 == (sal_card->config.port_bitmap[port_id] & 0xffffffff))
			continue;	/* don't config port */

		port_logic_id = sal_card->config.port_logic_id[port_id];
		HIGGS_TRACE(OSP_DBL_INFO, 4614,
			    "Card:%d is going to del register port:0x%x",
			    ll_card->card_id, port_logic_id);


	}

	return OK;
}

/**
 * higgs_release_hw - logout HW.
 *
 */
s32 higgs_release_hw(struct higgs_card * ll_card)
{
	HIGGS_TRACE(OSP_DBL_INFO, 4378,
		    "Card:%d start to release HW resource...",
		    ll_card->card_id);

	/*soft reset chip  */
	if (OK != higgs_reset_hw(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4381,
			    "Card:%d reset HW failed in HDA mode.",
			    ll_card->card_id);
		return ERROR;
	}

	/* re-initialize device  */
	(void)higgs_uninit_peri_device(ll_card);

	return OK;
}

/**
 * higgs_free_all_active_req - clear Running array REQ.
 *
 */
void higgs_free_all_active_req(struct higgs_card *ll_card)
{
	HIGGS_REF(ll_card);
	
	return;
}

/**
 * higgs_reset_hw - soft reset for hard reset.
 *
 */
static s32 higgs_reset_hw(struct higgs_card *ll_card)
{
	/* ready to chip reset */
	u32 type = SAL_ERROR_INFO_ALL;

	if (OK != higgs_prepare_reset_hw(ll_card)) {

		HIGGS_TRACE(OSP_DBL_MAJOR, 100,
			    "Card:%d prepare reset hw failed!",
			    ll_card->card_id);
		(void)higgs_dump_info(ll_card->sal_card, (void *)&type);
	}

	/*execute chip reset   */
	if (OK != higgs_execute_reset_hw(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 100,
			    "Card:%d execute reset hw failed!",
			    ll_card->card_id);
		return ERROR;
	}

	return OK;
}

/**
 * higgs_init_hw - soft reset for hardware initialize.
 *
 */
s32 higgs_init_hw(struct higgs_card * ll_card)
{
	/* parameter check  */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
#if 1				/* c02 will shield the code   */

	higgs_reset_hw(ll_card);
	HIGGS_MDELAY(100);
#endif
	/* initialize chip register */
	higgs_init_reg(ll_card);

	/* initialize serdes parameter  */
	(void)higgs_init_serdes_param(ll_card);

	/* initialize identify frame */
	(void)higgs_init_identify_frame(ll_card);

	return OK;
}

#if 0
/*****************************************************************************
 函 数 名  : Higgs_DelWireEventTimer
 功能描述  : 删除模拟线缆插入事件关联timer
 输入参数  : struct higgs_card *ll_card
             u32 init_flag
 输出参数  : 无
 返 回 值  : void
*****************************************************************************/
static void Higgs_DelWireEventTimer(struct higgs_card *ll_card, u32 init_flag)
{
	unsigned long flag = 0;
	u32 i = 0;

	if (init_flag & HIGGS_INITED_RSC) {
		for (i = 0; i < HIGGS_MAX_PORT_NUM; ++i) {
			spin_lock_irqsave(&ll_card->card_lock, flag);
			sal_del_timer((void *)&ll_card->astPortTimer[i].
				      plug_in_timer);
			spin_unlock_irqrestore(&ll_card->card_lock, flag);
		}
	}
}
#endif

/**
 * higgs_release_card - release 12G card resource.
 *
 */
void higgs_release_card(struct higgs_card *ll_card, u32 init_flag, u32 logic_id)
{
	unsigned long flag = 0;
	u32 i = 0;

	HIGGS_ASSERT(NULL != ll_card, return);

	spin_lock_irqsave(&ll_card->sal_card->card_lock, flag);
	if (SAL_CARD_REMOVE_PROCESS & ll_card->sal_card->flag) {
		/*don't remove after AC down*/
		spin_unlock_irqrestore(&ll_card->sal_card->card_lock, flag);
		return;
	}

	ll_card->sal_card->flag &= ~SAL_CARD_ACTIVE;
	ll_card->sal_card->flag |= SAL_CARD_REMOVE_PROCESS;
	spin_unlock_irqrestore(&ll_card->sal_card->card_lock, flag);

	/*if there is port product, then it is necessary to logout port */ 
	if (0xFFFFFFFF != logic_id) {

		higgs_notify_port_id(ll_card->sal_card);


	}

	if (init_flag & HIGGS_INITED_PORTID)
		/* report API/Frame delete port */
		(void)higgs_del_register_port(ll_card);

	if (init_flag & HIGGS_INITED_ADDSAL)
		sal_remove_threads(ll_card->sal_card);

	if (SAL_AND((SAL_PORT_MODE_INI == ll_card->card_cfg.work_mode),
		    (init_flag & HIGGS_INITED_ADDSAL))) {

		(void)sal_abort_all_dev_io(ll_card->sal_card);

		sal_turn_off_all_led(ll_card->sal_card);
	}

	/* close interrupt and logout interrupt  */
	if (init_flag & HIGGS_INITED_INTR) {

		higgs_close_all_intr(ll_card);

		/*release interrupt */
		higgs_release_intr(ll_card);
	}


	if (init_flag & HIGGS_INITED_ADDSAL)
		/* release SAL layer resource , return io  */
		sal_remove_card(ll_card->sal_card);

	/* release REQ-SGL  in RUNNING array*/
	if (init_flag & HIGGS_INITED_RSC) {
		//higgs_free_all_active_req(ll_card);
		higgs_recycle_active_req_rsc(ll_card);

		spin_lock_irqsave(&ll_card->card_lock, flag);
		for (i = 0; i < HIGGS_MAX_PHY_NUM; ++i) {
			HIGGS_TRACE(OSP_DBL_INFO, 739,
				    "Phy:%d del phy_run_serdes_fw_timer timer!",
				    i);
			sal_del_timer((void *)&ll_card->phy[i].
				      phy_run_serdes_fw_timer);
			ll_card->phy[i].run_serdes_fw_timer_en = false;


#if defined(PV660_ARM_SERVER)
			HIGGS_TRACE(OSP_DBL_INFO, 739,
				    "Phy:%d del phy_dma_status_timer timer!",
				    i);
			sal_del_timer((void *)&ll_card->phy[i].
				      phy_dma_status_timer);
#endif

		}
		spin_unlock_irqrestore(&ll_card->card_lock, flag);

#if defined(PV660_ARM_SERVER)
		/*delete LED process thread  */
		higgs_notify_led_thread_remove(ll_card);
#endif


	}

	/* close hardware  */
	if (init_flag & HIGGS_INITED_HW) {
		(void)higgs_release_hw(ll_card);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 16, 0)
		if (NULL != (void *)ll_card->card_reg_base) {
			SAS_IOUNMAP((void *)ll_card->card_reg_base);
			ll_card->card_reg_base = (u64) NULL;
		}
#else

		if (NULL != (void *)cpld_reg_base) {
			SAS_IOUNMAP(cpld_reg_base);
			cpld_reg_base = NULL;
			cpld_dev_node = NULL;
		}
		ll_card->cpld_reg_base = NULL;

		if (NULL != (void *)ll_card->card_reg_base) {
			HIGGS_DEVM_IOUNMAP(&ll_card->plat_form_dev->dev,
					   (void *)ll_card->card_reg_base);
			ll_card->card_reg_base = (u64) NULL;
		}
#endif
		if (NULL != ((void *)ll_card->hilink_base)) {
			SAS_IOUNMAP((void *)ll_card->hilink_base);
			ll_card->hilink_base = (u64) NULL;
		}
	}

	/* release memory  */
	if (init_flag & HIGGS_INITED_MEM)
		(void)higgs_free_mem(ll_card);

	/*release sas layer memory resource  */
	if (init_flag & HIGGS_INITED_SAL)
		sal_put_card(ll_card->sal_card);

	return;
}

/**
 * higgs_dump_rsc - test and take statistics of resource size.
 *
 */
void higgs_dump_rsc(struct higgs_card *ll_card)
{
	HIGGS_TRACE(OSP_DBL_INFO, 4151, "Card:%d allocate kernel memory %d KB", ll_card->card_id, ll_card->higgs_use_kmem / 1024);	/*将BYTE转换为KBYTE，方便阅读 */
	HIGGS_TRACE(OSP_DBL_INFO, 4151, "Card:%d allocate dma memory %d KB", ll_card->card_id, ll_card->higgs_use_dma_mem / 1024);	/*将BYTE转换为KBYTE，方便阅读 */
}

/**
 * higgs_test_reg_access - test if read and write register normal 
 *
 */
static s32 higgs_test_reg_access(struct higgs_card *ll_card)
{
	u32 rd_val = 0;
	u32 wr_val = 0;
	u32 offset = 0;

	/*chip register read write test . */
	HIGGS_TRACE(OSP_DBL_INFO, 1,
		    "****************START REG TEST****************");
	rd_val = HIGGS_GLOBAL_REG_READ(ll_card, offset);
	HIGGS_TRACE(OSP_DBL_INFO, 1,
		    "Global Reg Offset[0x%llx]Read Value[0x%x]", (u64) offset,
		    rd_val);

	HIGGS_GLOBAL_REG_WRITE(ll_card, offset, wr_val);
	HIGGS_TRACE(OSP_DBL_INFO, 1,
		    "Global Reg Offset[0x%llx]Write Value[0x%x]", (u64) offset,
		    wr_val);

	rd_val = HIGGS_GLOBAL_REG_READ(ll_card, offset);
	HIGGS_TRACE(OSP_DBL_INFO, 1,
		    "Global Reg Offset[0x%llx]Read Value[0x%x]", (u64) offset,
		    rd_val);
	if (rd_val == wr_val) {
		HIGGS_TRACE(OSP_DBL_INFO, 1,
			    "****************TEST PASS!****************");
		return OK;
	} else {
		HIGGS_TRACE(OSP_DBL_INFO, 1,
			    "****************TEST FAIL!****************");
	}
	return ERROR;
}

#if defined(PV660_ARM_SERVER)

/**
 * higgs_add_led_notify_node - add msg to  processing list .
 *
 */
void higgs_add_led_notify_node(struct higgs_card *ll_card,
			       struct higgs_led_event_notify *led_event)
{
	unsigned long flag = 0;

	SAL_ASSERT(NULL != ll_card, return);
	SAL_ASSERT(NULL != led_event, return);
	//HIGGS_TRACE(OSP_DBL_MAJOR, 100, "Led Thread Add event %d opcode, phy id %d", led_event->event_val, led_event->phy_id);

	spin_lock_irqsave(&ll_card->led_event_ctrl_lock, flag);
	/*add msg to processing list  */
	SAS_LIST_ADD_TAIL(&led_event->notify_list,
			  &ll_card->led_event_active_list);
	spin_unlock_irqrestore(&ll_card->led_event_ctrl_lock, flag);

	/*wake up thread */
	(void)SAS_WAKE_UP_PROCESS(ll_card->led_event_thread);

	return;
}

/**
 * higgs_add_event_to_led_handler - add processing event  to  cable insert and pull out thread.
 *
 */
s32 higgs_add_event_to_led_handler(struct higgs_card * ll_card,
				   u32 event, u32 phy_id)
{
	struct higgs_led_event_notify *led_event = NULL;

	led_event = HIGGS_VMALLOC(sizeof(struct higgs_led_event_notify));
	if (NULL == led_event) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 2,
			    "Card:%d get wire notify event failed",
			    ll_card->card_id);
		return ERROR;
	}
	led_event->event_val = event;
	led_event->phy_id = phy_id;

	higgs_add_led_notify_node(ll_card, led_event);

	return OK;
}

/**
 * higgs_get_active_led_notify_node - get active led notify node.
 *
 */
struct higgs_led_event_notify *higgs_get_active_led_notify_node(struct
								higgs_card
								*ll_card)
{
	unsigned long flag = 0;
	struct higgs_led_event_notify *led_event = NULL;

	spin_lock_irqsave(&ll_card->led_event_ctrl_lock, flag);
	if (SAS_LIST_EMPTY(&ll_card->led_event_active_list)) {
		spin_unlock_irqrestore(&ll_card->led_event_ctrl_lock, flag);
		return NULL;
	}
	led_event =
	    SAS_LIST_ENTRY(ll_card->led_event_active_list.next,
			   struct higgs_led_event_notify, notify_list);

	SAS_LIST_DEL_INIT(&led_event->notify_list);
	spin_unlock_irqrestore(&ll_card->led_event_ctrl_lock, flag);

	return led_event;
}

/**
 * higgs_get_led_event_from_list - check if the list which  record insert and pull out event is NULL.
 *
 */
s32 higgs_get_led_event_from_list(struct higgs_card * ll_card,
				  struct higgs_led_event_notify * led_event)
{
	struct higgs_led_event_notify *active_led_event = NULL;
	HIGGS_ASSERT(NULL != ll_card, return ERROR);

	active_led_event = higgs_get_active_led_notify_node(ll_card);
	if (NULL == active_led_event) {
		return ERROR;
	}

	led_event->phy_id = active_led_event->phy_id;
	led_event->event_val = active_led_event->event_val;
	HIGGS_VFREE(active_led_event);
	active_led_event = NULL;

	return OK;
}

/**
 * higgs_pre_handle_wire_notify_event - check if the node need to be processed.
 *
 */
s32 higgs_pre_handle_wire_notify_event(struct higgs_card * ll_card,
				       struct higgs_led_event_notify *
				       led_event)
{
	unsigned long flag = 0;
	struct list_head *pos = NULL;
	struct list_head *n = NULL;
	struct higgs_led_event_notify *tmp = NULL;

	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	HIGGS_ASSERT(NULL != led_event, return ERROR);

	if (SAS_LIST_EMPTY(&ll_card->led_event_active_list)) {
		return OK;
	} else {
		spin_lock_irqsave(&ll_card->led_event_ctrl_lock, flag);
		SAS_LIST_FOR_EACH_SAFE(pos, n, &ll_card->led_event_active_list) {
			tmp =
			    SAS_LIST_ENTRY(pos, struct higgs_led_event_notify,
					   notify_list);

			if ((led_event->event_val == tmp->event_val) &&
			    (led_event->phy_id == tmp->phy_id)) {
				HIGGS_TRACE(OSP_DBL_INFO, 100,
					    "Card:%d phy id:%d with Event val:0x%x",
					    ll_card->card_id, tmp->phy_id,
					    tmp->event_val);
				spin_unlock_irqrestore(&ll_card->
						       led_event_ctrl_lock,
						       flag);
				return ERROR;
			}
		}
		spin_unlock_irqrestore(&ll_card->led_event_ctrl_lock, flag);
	}

	return OK;
}

/**
 * higgs_oper_led_by_event - function which is according to event process port operation.
 *
 */
void higgs_oper_led_by_event(struct higgs_card *ll_card,
			     struct higgs_led_event_notify *led_event)
{
	u8 op_code = 0;
	u32 offset = 0;
	u32 total_items = 0;
	u32 i = 0;
	struct higgs_led_info *led_info = NULL;

	HIGGS_ASSERT(NULL != ll_card, return);
	HIGGS_ASSERT(NULL != led_event, return);

	total_items = sizeof(higgs_led_info) / sizeof(struct higgs_led_info);

	HIGGS_TRACE(OSP_DBL_MAJOR, 100,
		    "Led Thread Add event %d opcode, phy id %d, itemNum is %d",
		    led_event->event_val, led_event->phy_id, total_items);
	for (i = 0; i < total_items; i++) {
		led_info = &higgs_led_info[i];
		if ((led_info->card_id == ll_card->card_id) &&
		    (led_info->phy_id == led_event->phy_id)) {
			offset = led_info->offset;
			break;
		}
	}

	switch (led_event->event_val) {
	case LED_DISK_PRESENT:
		op_code = 0x0;
		break;
	case LED_DISK_NORMAL_IO:
		op_code = 0x4;
		break;
	case LED_DISK_LOCATE:
		op_code = 0x6;
		break;
	case LED_DISK_REBUILD:
		op_code = 0x7;
		break;
	case LED_DISK_FAULT:
		op_code = 0x5;
		break;
	default:
		HIGGS_TRACE(OSP_DBL_MAJOR, 100,
			    "Led event opcode is not supported %d",
			    led_event->event_val);
		break;
	}

	if (offset != 0) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 100,
			    "card id %d, phy id %d offset is %x, led opcode is %d",
			    ll_card->card_id, led_event->phy_id, offset,
			    op_code);
		HIGGS_CPLDLED_REG_WRITE(ll_card, offset, op_code);
	}
	return;
}

/**
 * higgs_handle_led_notify_event - process led light event.
 *
 */
void higgs_handle_led_notify_event(struct higgs_card *ll_card)
{
	struct higgs_led_event_notify led_event;
	HIGGS_ASSERT(NULL != ll_card, return);

	memset(&led_event, 0, sizeof(struct higgs_led_event_notify));

	for (;;) {
		 /**/
		    if (OK !=
			higgs_get_led_event_from_list(ll_card, &led_event))
			break;

		if (ERROR ==
		    higgs_pre_handle_wire_notify_event(ll_card, &led_event))
			continue;

		higgs_oper_led_by_event(ll_card, &led_event);
	}
	return;
}

/**
 * higgs_led_event_handler - cable mixed insert and pull out process.
 *
 */
s32 higgs_led_event_handler(void *data)
{
	struct higgs_card *ll_card = NULL;

	ll_card = (struct higgs_card *)data;

	SAS_SET_CURRENT_STATE(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		schedule();
		SAS_SET_CURRENT_STATE(TASK_INTERRUPTIBLE);

		/*LED  light */
		higgs_handle_led_notify_event(ll_card);
	}
	__set_current_state(TASK_RUNNING);
	HIGGS_TRACE(OSP_DBL_INFO, 2, "Card:%d Led event handler thread exit",
		    ll_card->card_id);
	ll_card->led_event_thread = NULL;
	return OK;
}

/**
 * higgs_notify_led_thread_remove - remove cable insert and pull out event process module
 *
 */
void higgs_notify_led_thread_remove(struct higgs_card *ll_card)
{
	/* stop wire handle thread */
	if (ll_card->led_event_thread) {
		(void)SAS_KTHREAD_STOP(ll_card->led_event_thread);
		SAL_TRACE(OSP_DBL_INFO, 888,
			  "Card:%d Led handler thread exit...",
			  ll_card->card_id);
		ll_card->led_event_thread = NULL;
	}
	return;
}

/**
 * higgs_led_ctrl_thread_init - initialize cable insert and pull out event process .
 *
 */
s32 higgs_led_ctrl_thread_init(struct higgs_card * ll_card)
{
	ll_card->led_event_thread = kthread_run(higgs_led_event_handler,
						(void *)ll_card, "Higgs_led/%d",
						ll_card->card_id);
	if (IS_ERR(ll_card->led_event_thread)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 890,
			    "Card:%d init Led event handler thread failed, ret %ld",
			    ll_card->card_id,
			    PTR_ERR(ll_card->led_event_thread));
		ll_card->led_event_thread = NULL;
		return ERROR;
	}

	return OK;
}
#endif


/**
 * higgs_remove - higgs layer remove .
 *
 */
s32 higgs_remove(void *in, void *out)
{
	struct higgs_card *ll_card = NULL;
	u32 id = 0;
	struct sal_card *sal_card = NULL;

	SAL_REF(out);

	id = (u32) (u64) in;
	HIGGS_TRACE(OSP_DBL_INFO, 4641, "SAS Card:%d begin remove...", id);
	sal_card = sal_get_card(id);
	HIGGS_ASSERT(sal_card != NULL, return ERROR);
	ll_card = sal_card->drv_data;
	HIGGS_ASSERT(ll_card != NULL, return ERROR);
	higgs_release_card(ll_card, 0xff, 0xffffffff);
	sal_put_card(sal_card);

	HIGGS_TRACE(OSP_DBL_INFO, 4641, "SAS Card:%d was removed succeed", id);
	return OK;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 16, 0)

/**
 * higgs_probe - higgs probe function .
 *
 */
s32 higgs_probe(void *in, void *out)
{
	/*1. call sal_alloc_card allocate the struct sal_card and struct higgs_card *memory */
	struct sal_card *sal_card = NULL;
	struct higgs_card *ll_card = NULL;
	u32 logic_id = 0xFFFFFFFF;	/* used when notify fail */
	unsigned long flag = 0;
	u32 init_flag = 0;
	u32 controller_id = HIGGS_MAX_CARD_NUM;
	unsigned long start_time = jiffies;
	struct higgs_probe_info *probe_info;
	struct platform_device *dev;

	SAL_REF(out);
	probe_info = (struct higgs_probe_info *)in;
	HIGGS_ASSERT(probe_info != NULL, return ERROR);
	dev = probe_info->dev;
	HIGGS_ASSERT(dev != NULL, return ERROR);

	controller_id = (u32) dev->id;

	HIGGS_TRACE(OSP_DBL_INFO, 4620, "Card %d is going to probe.",
		    controller_id);

	HIGGS_ASSERT(controller_id < HIGGS_MAX_CARD_NUM, return ERROR);
	/* allocate card from SAL layer  */
	sal_card = sal_alloc_card((u8) controller_id,
				  (u8) controller_id, sizeof(struct higgs_card),
				  HIGGS_MAX_PHY_NUM);
	if (NULL == sal_card) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4620,
			    "Card %d Malloc sas layer card failed",
			    controller_id);
		return ERROR;
	}
	spin_lock_irqsave(&sal_card->card_lock, flag);
	sal_card->flag |= SAL_CARD_INIT_PROCESS;
	spin_unlock_irqrestore(&sal_card->card_lock, flag);

	/*2. Init part of struct higgs_card */
	ll_card = (struct higgs_card *)sal_card->drv_data;
	ll_card->sal_card = sal_card;
	ll_card->card_id = sal_card->card_no;
	ll_card->plat_form_dev = dev;

	ll_card->higgs_use_kmem = 0;
	ll_card->higgs_use_dma_mem = 0;

	if (P660_SAS_CORE_DSAF_ID == ll_card->card_id) {
		ll_card->card_reg_base
		    =
		    (u64) SAS_IOREMAP(P660_SAS_CORE_DSAF_BASE,
				      P660_SAS_CORE_RANGE);
	} else {
		ll_card->card_reg_base
		    =
		    (u64) SAS_IOREMAP(P660_SAS_CORE_PCIE_BASE,
				      P660_SAS_CORE_RANGE);
		ll_card->hilink_base =
		    (u64) SAS_IOREMAP(0xB2200000ULL, 0xFFFFF);
	}


	higgs_test_reg_access(ll_card);

	/* read config file  */
	if (OK != higgs_get_card_cfg(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4621,
			    "Card:%d get card config failed", ll_card->card_id);
		sal_global.init_failed_reason[ll_card->card_id] =
		    SAL_CONFIG_INIT_FAILED;
		goto INIT_FAILED;
	}

	/* generate port  */
	(void)higgs_set_up_port_id(ll_card, &logic_id);

	higgs_get_card_limit(ll_card);

	if (OK != higgs_init_mem(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4623, "Card:%d init mem failed",
			    ll_card->card_id);
		sal_global.init_failed_reason[ll_card->card_id] =
		    SAL_MEM_INIT_FAILED;
		goto INIT_FAILED;
	}
	init_flag |= HIGGS_INITED_MEM;

	higgs_init_rsc(ll_card);
	init_flag |= HIGGS_INITED_RSC;


	(void)higgs_init_hw(ll_card);
	init_flag |= HIGGS_INITED_HW;


	(void)higgs_init_peri_device(ll_card);

	if (OK != higgs_init_intr(ll_card)) {

		HIGGS_TRACE(OSP_DBL_MAJOR, 4627,
			    "Card:%d init interrupt failed", ll_card->card_id);
		sal_global.init_failed_reason[ll_card->card_id] =
		    SAL_INTR_INIT_FAILED;
		goto INIT_FAILED;
	}
	init_flag |= HIGGS_INITED_INTR;

	if (OK != higgs_add_sal_card(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4628, "Card:%d add SAL failed ",
			    ll_card->card_id);
		goto INIT_FAILED;
	}
	init_flag |= HIGGS_INITED_ADDSAL;

	/*initialize finish  */
	spin_lock_irqsave(&sal_card->card_lock, flag);
	sal_card->flag &= ~SAL_CARD_INIT_PROCESS;
	sal_card->flag |= SAL_CARD_ACTIVE;
	spin_unlock_irqrestore(&sal_card->card_lock, flag);
	sal_global.init_failed_reason[sal_card->card_no] = SAL_CARD_INIT_SUCC;

	higgs_dump_rsc(ll_card);

	memset(&(ll_card->chip_err_fatal_stat), 0,
	       sizeof(struct higgs_chip_err_fatal_stat));
#if 0
	/* 开工 */
	Higgs_HookUpperLayer2Work(ll_card);
#endif
	/*report port */
	higgs_notify_port_id(sal_card);
	init_flag |= HIGGS_INITED_PORTID;


	HIGGS_TRACE(OSP_DBL_INFO, 4629,
		    "12G SAS Card:%d Probe in %s mode OK! cost time:%d ms",
		    ll_card->card_id,
		    (SAL_PORT_MODE_INI ==
		     ll_card->card_cfg.work_mode) ? "INI" : "TGT",
		    jiffies_to_msecs(jiffies - start_time));

#if  defined(FPGA_VERSION_TEST)
	if (P660_SAS_CORE_PCIE_ID == controller_id)
		higgs_stub_simulate_sfp_plug_mini_port(ll_card, 0, SAL_ELEC_CABLE, 3000);	/* FPGA: PCIE PHY 0~3 */

#elif defined(EVB_VERSION_TEST)
	if (P660_SAS_CORE_PCIE_ID == controller_id)
		higgs_stub_simulate_sfp_plug_mini_port(ll_card, 1, SAL_ELEC_CABLE, 3000);	/* EVB: PCIE PHY 4~7 */
#endif
#if 0
	if (controller_id == VAR_CHECK_CARD) {
		g_cardL = (*ll_card);
#if 0
		if (OK == Higgs_CheckCardVar(ll_card)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4628, "Card:%d check ok",
				    ll_card->card_id);
		}
#endif
	}
#endif
	return OK;

      INIT_FAILED:
	spin_lock_irqsave(&sal_card->card_lock, flag);
	sal_card->flag &= ~SAL_CARD_INIT_PROCESS;
	spin_unlock_irqrestore(&sal_card->card_lock, flag);

	higgs_release_card(ll_card, init_flag, logic_id);

	HIGGS_TRACE(OSP_DBL_MAJOR, 4628, "Card:%d probe failed! ",
		    ll_card->card_id);
	return ERROR;
}

/**
 * higgs_probe_stub - SAS sub-system probe .
 *
 */
s32 higgs_probe_stub(struct platform_device * dev)
{
	s32 ret = ERROR;
	struct higgs_probe_info info;

	info.dev = dev;
	info.card_id = (u32) dev->id;
	ret =
	    sal_send_to_ctrl_wait((u32) card_no, SAL_CTRL_HOST_ADD, &info, NULL,
				  higgs_probe);

	return ret;
}

/**
 * higgs_remove_stub - remove function .
 *
 */
s32 higgs_remove_stub(struct platform_device * dev)
{
	u32 id = 0;

	HIGGS_ASSERT(dev != NULL, return ERROR);
	HIGGS_TRACE(OSP_DBL_INFO, 744, "dev id %d in remove", dev->id);

	id = (u32) dev->id;
	(void)sal_send_to_ctrl_wait(id, SAL_CTRL_HOST_REMOVE, (void *)(u64) id,
				    NULL, higgs_remove);

	return OK;
}

#else

/* BEGIN: Added by c00257296, 2015/1/21   PN:arm_server*/
#if defined(PV660_ARM_SERVER)
extern struct higgs_card *higgs_get_card_info(u32 card_id);

static void higgs_start_pcie_sas_phy(unsigned long data)
{
	struct timer_list *tmp_timer;
	u64 tmp_sas_addr = 0;
	u32 phy_id = 0;
	struct higgs_card *ll_card = NULL;
	/* traverse, trigger */
	ll_card = higgs_get_card_info(P660_SAS_CORE_PCIE_ID);

	for (phy_id = 5; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		HIGGS_PORT_REG_WRITE(ll_card, phy_id,
				     HISAS30HV100_PORT_REG_CHL_INT2_MSK_REG,
				     0x32A);
		tmp_sas_addr =
		    ((u64) ll_card->card_cfg.phy_addr_high[phy_id] << 32)
		    | ((u64) ll_card->card_cfg.phy_addr_low[phy_id]);
		higgs_start_phy(ll_card, phy_id, tmp_sas_addr,
				HIGGS_PHY_CFG_RATE_12_0_G);
	}

	/* clear timer */
	tmp_timer = (struct timer_list *)data;
	if (NULL != tmp_timer) {
		HIGGS_VFREE(tmp_timer);
		tmp_timer = NULL;
	}

	return;
}

void higgs_start_pcie_sas_phy_later(u32 timeout_ms)
{
	struct timer_list *tmp_timer = NULL;

	tmp_timer = HIGGS_VMALLOC(sizeof(struct timer_list));
	if (NULL == tmp_timer) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4576, "Card allocate timer failed");
		return;
	}

	init_timer(tmp_timer);
	tmp_timer->data = (unsigned long)tmp_timer;
	tmp_timer->expires = jiffies + msecs_to_jiffies(timeout_ms);
	tmp_timer->function = higgs_start_pcie_sas_phy;
	add_timer(tmp_timer);
}

/* begin:add by chenqilin */
static void higgs_start_sas_phy(unsigned long data)
{
	//struct timer_list *tmp_timer;
	u64 tmp_sas_addr = 0;
	u32 phy_id = 0;
	struct higgs_card *ll_card = NULL;
	u32 controller_id;

	controller_id = (u32) data;

	/* traverse, trigger */
	ll_card = higgs_get_card_info(controller_id);

	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		if ((ll_card->card_cfg.phy_useage & (1 << phy_id)) ==
		    0)
			continue;
		/*
		HIGGS_PORT_REG_WRITE(ll_card, phy_id,
				     HISAS30HV100_PORT_REG_CHL_INT2_MSK_REG,
				     0x32A);
		*/		     
		tmp_sas_addr =
		    ((u64) ll_card->card_cfg.phy_addr_high[phy_id] << 32)
		    | ((u64) ll_card->card_cfg.phy_addr_low[phy_id]);
        
		higgs_start_phy(ll_card, phy_id, tmp_sas_addr,
				HIGGS_PHY_CFG_RATE_12_0_G);
	}

	return;
}

void higgs_start_sas_phy_later(u32 timeout_ms, u32 controller_id)
{
	struct timer_list *tmp_timer = NULL;

	tmp_timer = HIGGS_VMALLOC(sizeof(struct timer_list));
	if (NULL == tmp_timer) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4576, "Card allocate timer failed");
		return;
	}

	init_timer(tmp_timer);
	tmp_timer->data = (unsigned long)controller_id;
	tmp_timer->expires = jiffies + msecs_to_jiffies(timeout_ms);
	tmp_timer->function = higgs_start_sas_phy;
	add_timer(tmp_timer);
}

/* end: add by chenqilin */
static void higgs_start_dsaf_sas_phy(unsigned long data)
{
	struct timer_list *tmp_timer;
	u64 tmp_sas_addr = 0;
	u32 phy_id = 0;
	struct higgs_card *ll_card = NULL;

	/* traverse, trigger */
	ll_card = higgs_get_card_info(P660_SAS_CORE_DSAF_ID);

	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		tmp_sas_addr =
		    ((u64) ll_card->card_cfg.phy_addr_high[phy_id] << 32)
		    | ((u64) ll_card->card_cfg.phy_addr_low[phy_id]);

		higgs_start_phy(ll_card, phy_id, tmp_sas_addr,
				HIGGS_PHY_CFG_RATE_12_0_G);
	}

	/* clear timer */
	tmp_timer = (struct timer_list *)data;
	if (NULL != tmp_timer) {
		HIGGS_VFREE(tmp_timer);
		tmp_timer = NULL;
	}
	return;
}

void higgs_start_dsaf_sas_phy_later(u32 timeout_ms)
{
	struct timer_list *tmp_timer = NULL;

	tmp_timer = HIGGS_VMALLOC(sizeof(struct timer_list));
	if (NULL == tmp_timer) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4576, "Card allocate timer failed");
		return;
	}

	init_timer(tmp_timer);
	tmp_timer->data = (unsigned long)tmp_timer;
	tmp_timer->expires = jiffies + msecs_to_jiffies(timeout_ms);
	tmp_timer->function = higgs_start_dsaf_sas_phy;
	add_timer(tmp_timer);
}

#endif

/**
 * higgs_probe_device - probe function .
 *
 */
s32 higgs_probe_device(struct platform_device *dev, u32 card_id)
{
	/*1. call sal_alloc_card allocate the struct sal_card and struct higgs_card *memory */
	struct sal_card *sal_card = NULL;
	struct higgs_card *ll_card = NULL;
	u32 logic_id = 0xFFFFFFFF;	/* used when notify fail */
	unsigned long flag = 0;
	u32 init_flag = 0;
	u32 controller_id = HIGGS_MAX_CARD_NUM;
	unsigned long start_time = jiffies;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)	/* < linux 3.19 */
	u32 dts_idx = 0;
#endif
	struct device *device = NULL;
	struct resource *res = NULL;
	u8 card_pos = 0;
	bool(*pfn_for_lint) (u32) = higgs_stub_in_debug_mode;
      u32 phy_id = 0;

	HIGGS_REF(pfn_for_lint);	/* pclint */

	HIGGS_ASSERT(dev != NULL, return ERROR);

	controller_id = (u32) card_id;

	HIGGS_TRACE(OSP_DBL_INFO, 4620, "Card %d is going to probe.",
		    controller_id);

	HIGGS_ASSERT(controller_id < HIGGS_MAX_CARD_NUM, return ERROR);
	card_pos = higgs_get_card_position(controller_id);
	sal_card = sal_alloc_card(card_pos,
				  (u8) controller_id, sizeof(struct higgs_card),
				  HIGGS_MAX_PHY_NUM);
	if (NULL == sal_card) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4620,
			    "Card %d Malloc sas layer card failed",
			    controller_id);
		return ERROR;
	}
	spin_lock_irqsave(&sal_card->card_lock, flag);
	sal_card->flag |= SAL_CARD_INIT_PROCESS;
	spin_unlock_irqrestore(&sal_card->card_lock, flag);

	/*2. Init part of struct higgs_card */
	ll_card = (struct higgs_card *)sal_card->drv_data;
	ll_card->sal_card = sal_card;
	ll_card->card_id = sal_card->card_no;
	ll_card->plat_form_dev = dev;

	ll_card->higgs_use_kmem = 0;
	ll_card->higgs_use_dma_mem = 0;

	if (OK != higgs_get_sas_controller_cfg(dev, &ll_card->card_cfg)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4621,
			    "Card:%d get Sas controller config failed",
			    controller_id);
		SAS_VFREE(sal_card);
		return ERROR;
	}

	device = &dev->dev;
	device->dma_mask = &higgs_dma_mask;
	device->coherent_dma_mask = higgs_dma_mask;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)	/* linux 3.19 */

	res = platform_get_resource(dev, IORESOURCE_MEM, 0);
	ll_card->card_reg_base = (u64) devm_ioremap_resource(device, res);
	if (IS_ERR((void *)ll_card->card_reg_base)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4620,
			    "Card %d dev_ioremap_resource failed",
			    ll_card->card_id);
		SAS_VFREE(sal_card);
		return ERROR;
	}
	HIGGS_TRACE(OSP_DBL_INFO, 171, "Card %d card_reg_base = 0x%llx",
		    ll_card->card_id, ll_card->card_reg_base);

	platform_set_drvdata(dev, ll_card);
#else

	if (P660_SAS_CORE_DSAF_ID == ll_card->card_id)
		dts_idx = P660_SAS_CORE_DSAF_DTS_INDEX;
	else if (P660_SAS_CORE_PCIE_ID == ll_card->card_id)
		dts_idx = P660_SAS_CORE_PCIE_DTS_INDEX;
	else if (P660_1_SAS_CORE_DSAF_ID == ll_card->card_id)
		/*CPU P660-1 sas0 dts index */
		dts_idx = P660_1_SAS_CORE_DSAF_DTS_INDEX;
	else if (P660_1_SAS_CORE_PCIE_ID == ll_card->card_id)
		/* CPU P660-1 sas1 dts index */
		dts_idx = P660_1_SAS_CORE_PCIE_DTS_INDEX;
	else
		return ERROR;

	res = HIGGS_PLATFORM_GET_RESOURCE(dev, IORESOURCE_MEM, dts_idx);
	ll_card->card_reg_base = (u64) HIGGS_DEVM_IOREMAP_RESOURCE(device, res);
	if (IS_ERR((void *)ll_card->card_reg_base)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4620,
			    "Card %d dev_ioremap_resource failed",
			    ll_card->card_id);
		return ERROR;
	}
	HIGGS_TRACE(OSP_DBL_INFO, 171, "Card %d card_reg_base = 0x%llx",
		    ll_card->card_id, ll_card->card_reg_base);
#endif

#if defined(EVB_VERSION_TEST)
	if (P660_SAS_CORE_PCIE_ID == ll_card->card_id)
		/* PCIE connect Hilink6 */
		ll_card->hilink_base =
		    (u64) SAS_IOREMAP(P660_HILINK_6_BASE, P660_HILINK_RANGE);
	else if (P660_SAS_CORE_DSAF_ID == ll_card->card_id)
		/* DSAF connect Hilink2 */
		ll_card->hilink_base =
		    (u64) SAS_IOREMAP(P660_HILINK_2_BASE, P660_HILINK_RANGE);
	else
		return ERROR;

#elif defined(C05_VERSION_TEST)

	if (P660_SAS_CORE_DSAF_ID == ll_card->card_id)
		/* DSAF connect Hilink2 */
		ll_card->hilink_base =
		    (u64) SAS_IOREMAP(P660_HILINK_2_BASE, P660_HILINK_RANGE);
	else if (P660_SAS_CORE_PCIE_ID == ll_card->card_id)
		/* PCIE connect Hilink6 */
		ll_card->hilink_base =
		    (u64) SAS_IOREMAP(P660_HILINK_6_BASE, P660_HILINK_RANGE);
	else
		return ERROR;

#endif

	if (OK != higgs_test_reg_access(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 181, "Card:%d try to boot failed",
			    ll_card->card_id);
		goto INIT_FAILED;
	}

#if defined(PV660_ARM_SERVER)
	/*arm server led control */
	cpld_dev_node =
	    of_find_compatible_node(NULL, NULL, "hisilicon,p660-cpld");
	if (cpld_dev_node == NULL) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4623,
			    "Card %d can't find %s dev node", ll_card->card_id,
			    cpld_dev_node->full_name);
		return ERROR;
	}

	if (cpld_reg_base == NULL)
		cpld_reg_base = of_iomap(cpld_dev_node, 0);

	ll_card->cpld_reg_base = cpld_reg_base;
	HIGGS_TRACE(OSP_DBL_MAJOR, 4623,
		    "Card %d find %s dev node, ulCpldLedRegBase is %p",
		    ll_card->card_id, cpld_dev_node->full_name,
		    ll_card->cpld_reg_base);

	SAS_SPIN_LOCK_INIT(&ll_card->led_event_ctrl_lock);
	SAS_INIT_LIST_HEAD(&ll_card->led_event_active_list);
	ll_card->led_event_thread = NULL;
	higgs_led_ctrl_thread_init(ll_card);
#endif


	/* generate port  */
	(void)higgs_set_up_port_id(ll_card, &logic_id);

	higgs_get_card_limit(ll_card);

	if (OK != higgs_init_mem(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4623, "Card:%d init mem failed",
			    ll_card->card_id);
		sal_global.init_failed_reason[ll_card->card_id] =
		    SAL_MEM_INIT_FAILED;
		goto INIT_FAILED;
	}
	init_flag |= HIGGS_INITED_MEM;

	higgs_init_rsc(ll_card);
	init_flag |= HIGGS_INITED_RSC;

	(void)higgs_init_hw(ll_card);
	init_flag |= HIGGS_INITED_HW;

	(void)higgs_init_peri_device(ll_card);

	if (OK != higgs_init_intr(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4627,
			    "Card:%d init interrupt failed", ll_card->card_id);
		sal_global.init_failed_reason[ll_card->card_id] =
		    SAL_INTR_INIT_FAILED;
		goto INIT_FAILED;
	}

	higgs_open_all_intr(ll_card);

	init_flag |= HIGGS_INITED_INTR;

	if (OK != higgs_add_sal_card(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 4628, "Card:%d add SAL failed ",
			    ll_card->card_id);
		goto INIT_FAILED;
	}
	init_flag |= HIGGS_INITED_ADDSAL;

	spin_lock_irqsave(&sal_card->card_lock, flag);
	sal_card->flag &= ~SAL_CARD_INIT_PROCESS;
	sal_card->flag |= SAL_CARD_ACTIVE;
	spin_unlock_irqrestore(&sal_card->card_lock, flag);
	sal_global.init_failed_reason[sal_card->card_no] = SAL_CARD_INIT_SUCC;

	higgs_dump_rsc(ll_card);
#if 0
	/* 开工 */
	Higgs_HookUpperLayer2Work(ll_card);
#endif


	init_flag |= HIGGS_INITED_PORTID;


	HIGGS_TRACE(OSP_DBL_INFO, 4629,
		    "12G SAS Card:%d Probe in %s mode OK! cost time:%d ms",
		    ll_card->card_id,
		    (SAL_PORT_MODE_INI ==
		     ll_card->card_cfg.work_mode) ? "INI" : "TGT",
		    jiffies_to_msecs(jiffies - start_time));

#if defined(FPGA_VERSION_TEST)
	if (P660_SAS_CORE_PCIE_ID == ll_card->card_id)
		higgs_stub_simulate_sfp_plug_mini_port(ll_card, 0, SAL_ELEC_CABLE, 3000);	/* PCIE PHY 0~3 */

#elif defined(EVB_VERSION_TEST)
	if (P660_SAS_CORE_PCIE_ID == ll_card->card_id)
		higgs_stub_simulate_sfp_plug_mini_port(ll_card, 1, SAL_ELEC_CABLE, 3000);	/* PCIE PHY 4~7 */

#elif defined(C05_VERSION_TEST)
	if (P660_SAS_CORE_DSAF_ID == ll_card->card_id)
		higgs_delay_trigger_all_port_sfp_event_for_init(ll_card, 1000);	

#elif defined(PV660_ARM_SERVER)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)	/* linux 3.19 */

  #if 0 /* SOLVE 12G PROBLEM */
	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		if ((ll_card->card_cfg.phy_useage & (1 << phy_id)) == 0)
			continue;

		/*先屏蔽phy up中断 */
		HIGGS_PORT_REG_WRITE(ll_card, phy_id,
				     HISAS30HV100_PORT_REG_CHL_INT2_MSK_REG,
				     0x36A);
		HIGGS_TRACE(OSP_DBL_INFO, 100,
			    "PortId[%d]Offset[0x%llx]WriteValue[0x%x]", phy_id,
			    (u64) HISAS30HV100_PORT_REG_CHL_INT2_MSK_REG,
			    HIGGS_PORT_REG_READ(ll_card, phy_id,
						HISAS30HV100_PORT_REG_CHL_INT2_MSK_REG));
	}
  #endif
	/*wait 1min */
	higgs_start_sas_phy_later(5000, ll_card->card_id);

#else
	if (P660_SAS_CORE_DSAF_ID == ll_card->card_id) {
		higgs_start_dsaf_sas_phy_later(3000);
	} else if ((P660_SAS_CORE_PCIE_ID == ll_card->card_id)
		   || (P660_1_SAS_CORE_PCIE_ID == ll_card->card_id)) {
		/*shield interrupt  */
		for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
			if ((ll_card->card_cfg.phy_useage & (1 << phy_id)) == 0)
				continue;

			HIGGS_PORT_REG_WRITE(ll_card, phy_id,
					     HISAS30HV100_PORT_REG_CHL_INT2_MSK_REG,
					     0x36A);
			HIGGS_TRACE(OSP_DBL_INFO, 100,
				    "PortId[%d]Offset[0x%llx]WriteValue[0x%x]",
				    phy_id,
				    (u64)
				    HISAS30HV100_PORT_REG_CHL_INT2_MSK_REG,
				    HIGGS_PORT_REG_READ(ll_card, phy_id,
							HISAS30HV100_PORT_REG_CHL_INT2_MSK_REG));
		}
		/*wait 1min */
		higgs_start_sas_phy_later(60 * 1000, ll_card->card_id);
	} else {

		goto INIT_FAILED;
	}
#endif
	printk("probe card(%d) OK!\r\n", ll_card->card_id);
#endif

	return OK;

      INIT_FAILED:
	spin_lock_irqsave(&sal_card->card_lock, flag);
	sal_card->flag &= ~SAL_CARD_INIT_PROCESS;
	spin_unlock_irqrestore(&sal_card->card_lock, flag);

	higgs_release_card(ll_card, init_flag, logic_id);

	HIGGS_TRACE(OSP_DBL_MAJOR, 4628, "Card:%d probe failed! ",
		    ll_card->card_id);
	return ERROR;
}

/**
 * higgs_probe - 12G sas card probe function .
 *
 */
//lint -e801
s32 higgs_probe(void *in, void *out)
{
	struct higgs_probe_info *probe_info;

	SAL_REF(out);
	probe_info = (struct higgs_probe_info *)in;
	HIGGS_ASSERT(probe_info != NULL, return ERROR);
	HIGGS_ASSERT(probe_info->dev != NULL, return ERROR);

	return higgs_probe_device(probe_info->dev, probe_info->card_id);
}

/**
 * higgs_probe_device_stub - probe function .
 *
 */
s32 higgs_probe_device_stub(struct platform_device * dev, u32 controller_id)
{
	s32 ret = ERROR;
	struct higgs_probe_info info;

	info.dev = dev;
	info.card_id = controller_id;
	ret =
	    sal_send_to_ctrl_wait(controller_id, SAL_CTRL_HOST_ADD, &info, NULL,
				  higgs_probe);
	return ret;
}

/**
 * higgs_probe_stub - SAS sub-system  probe function .
 *
 */
s32 higgs_probe_stub(struct platform_device * dev)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)	/* < linux 3.19 */
	u32 controller_id = 0;
	s32 ret;
	struct higgs_sas_controller_config sas_controller_cfg;
#endif
	static bool hilink_init_ok = false;
	u32 id = 0;

	HIGGS_ASSERT(dev != NULL, return ERROR);


#if defined(PV660_ARM_SERVER)
	if (of_property_read_u32(dev->dev.of_node, "nodes", &max_cpu_nodes))
		return ERROR;

    if (max_cpu_nodes < 1)        
		return ERROR;

	if (hilink_init_ok != true) {
		hilink_init_ok = true;
		hilink_reg_init();
		hilink_sub_dsaf_init();
		hilink_sub_pcie_init();
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)	/* linux 3.19 */

	if (of_property_read_u32(dev->dev.of_node, "controller-id", &id))
		return ERROR;

	/* os 3.19 */
	if (OK != higgs_probe_device_stub(dev, id)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 171,
			    "higgs_probe_stub controller %d failed", id);
		return ERROR;
	}
#else

	ret = higgs_get_board_sas_cfg();
	if (OK != ret) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 181,
			    "Get Board SAS configuration failed");
		return ERROR;
	}

	for (controller_id = 0;
	     controller_id <
	     (board_sas_cfg.max_cpu_node *
	      board_sas_cfg.max_controller_per_cpu); controller_id++) {
		if (OK !=
		    higgs_get_sas_controller_cfg(controller_id,
						 &sas_controller_cfg)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4621,
				    "Card:%d get Sas controller config failed",
				    controller_id);
			continue;
		}

		if (sas_controller_cfg.enabled == 0) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4621,
				    "Card:%d Sas controller is disabled",
				    controller_id);
			continue;
		}

		if (OK != higgs_probe_device_stub(dev, controller_id))
			HIGGS_TRACE(OSP_DBL_MAJOR, 171,
				    "higgs_probe controller %d failed",
				    controller_id);
	}
#endif

#endif

#if defined(FPGA_VERSION_TEST)
	/* sas controller (PCIE) */
	controller_id = P660_SAS_CORE_PCIE_ID;
	if (OK != higgs_probe_device_stub(dev, controller_id))
		HIGGS_TRACE(OSP_DBL_MAJOR, 171,
			    "higgs_probe_stub controller %d failed",
			    controller_id);

#elif defined(EVB_VERSION_TEST)
	/* sas controller (DSAF) */
	controller_id = P660_SAS_CORE_DSAF_ID;
	if (OK != higgs_probe_device_stub(dev, controller_id))
		HIGGS_TRACE(OSP_DBL_MAJOR, 171,
			    "higgs_probe_stub controller %d failed",
			    controller_id);

	/* sas controller (PCIE) */
	controller_id = P660_SAS_CORE_PCIE_ID;
	if (OK != higgs_probe_device_stub(dev, controller_id))
		HIGGS_TRACE(OSP_DBL_MAJOR, 171,
			    "higgs_probe_stub controller %d failed",
			    controller_id);

#elif defined(C05_VERSION_TEST)
	/* sas controller (DSAF) */
	controller_id = P660_SAS_CORE_DSAF_ID;
	if (OK != higgs_probe_device_stub(dev, controller_id))
		HIGGS_TRACE(OSP_DBL_MAJOR, 171,
			    "higgs_probe_stub controller %d failed",
			    controller_id);

#endif

	return OK;
}

/**
 * higgs_remove_device - remove function .
 *
 */
s32 higgs_remove_device(struct platform_device * dev, u32 controller_id)
{
	SAL_REF(dev);

	(void)sal_send_to_ctrl_wait(controller_id, SAL_CTRL_HOST_REMOVE,
				    (void *)(u64) controller_id, NULL,
				    higgs_remove);

	return OK;
}

/**
 * higgs_remove_stub -SAS sub-system  remove function .
 *
 */
s32 higgs_remove_stub(struct platform_device * dev)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)	/* < linux 3.19 */
	u32 controller_id = 0;
	struct higgs_sas_controller_config sas_controller_cfg;
#endif

	static bool has_exit = false;
	struct higgs_card *ll_card = NULL;

	HIGGS_ASSERT(dev != NULL, return ERROR);

#if defined(PV660_ARM_SERVER)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)	/* >= linux 3.19 */

	ll_card = platform_get_drvdata(dev);
	if (OK != higgs_remove_device(dev, ll_card->card_id)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 171,
			    "higgs_probe controller %d failed",
			    ll_card->card_id);
		return ERROR;
	}
#else
	for (controller_id = 0;
	     controller_id <
	     (board_sas_cfg.max_cpu_node *
	      board_sas_cfg.max_controller_per_cpu); controller_id++) {
		if (OK !=
		    higgs_get_sas_controller_cfg(controller_id,
						 &sas_controller_cfg)) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4621,
				    "Card:%d get Sas controller config failed",
				    controller_id);
			continue;
		}

		if (sas_controller_cfg.enabled == 0) {
			HIGGS_TRACE(OSP_DBL_MAJOR, 4621,
				    "Card:%d Sas controller is disabled",
				    controller_id);
			continue;
		}

		if (OK != higgs_remove_device(dev, controller_id))
			HIGGS_TRACE(OSP_DBL_MAJOR, 171,
				    "higgs_probe controller %d failed",
				    controller_id);
	}
#endif

	/* hilink re-initialize */
	if (has_exit == false) {
		has_exit = true;
		hilink_reg_exit();
		hilink_sub_dsaf_exit();
		hilink_sub_pcie_exit();
	}
#endif

#if defined(FPGA_VERSION_TEST)
	/* controller 1 (PCIE) */
	controller_id = P660_SAS_CORE_PCIE_ID;
	if (OK != higgs_remove_device(dev, controller_id))
		HIGGS_TRACE(OSP_DBL_MAJOR, 171,
			    "higgs_remove_stub controller %d failed",
			    controller_id);

#elif defined(EVB_VERSION_TEST)
	/* controller (DSAF) */
	controller_id = P660_SAS_CORE_DSAF_ID;
	if (OK != higgs_remove_device(dev, controller_id))
		HIGGS_TRACE(OSP_DBL_MAJOR, 171,
			    "higgs_remove_stub controller %d failed",
			    controller_id);

	/* controller 1 (PCIE) */
	controller_id = P660_SAS_CORE_PCIE_ID;
	if (OK != higgs_remove_device(dev, controller_id))
		HIGGS_TRACE(OSP_DBL_MAJOR, 171,
			    "higgs_remove_stub controller %d failed",
			    controller_id);

#elif defined(C05_VERSION_TEST)
	/* controller (DSAF) */
	controller_id = P660_SAS_CORE_DSAF_ID;
	if (OK != higgs_remove_device(dev, controller_id))
		HIGGS_TRACE(OSP_DBL_MAJOR, 171,
			    "higgs_remove_stub controller %d failed",
			    controller_id);

#endif

	return OK;
}
#endif

extern irqreturn_t higgs_oq_core_int(u32 cq_id, void *data);
extern void higgs_oq_int_process(struct higgs_card *ll_card, u32 queue_id);
#if 0
extern void sal_send_raw_ctrl_no_wait(u32 id,
				      void *in,
				      void *out,
				      s32(*raw_func) (void *, void *));
#endif

/**
 * higgs_init -higgs moudle initialize function .
 *
 */
s32 __init higgs_init(void)
{
	s32 ret = ERROR;
	u32 i = 0;

	HIGGS_ASSERT(sizeof(struct status_buf) > 0x40F);


	/* Higgs global variable initialize */
	higgs_register_dispatcher();

	HIGGS_TRACE(OSP_DBL_INFO, 744,
		    "higgs_oq_core_int %p, higgs_oq_int_process %p, SAL_SendCtrlMsg %p",
		    higgs_oq_core_int, higgs_oq_int_process,
		    sal_send_raw_ctrl_no_wait);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 16, 0)
#if defined(FPGA_VERSION_TEST)
	HIGGS_REF(higgs_dev_0);

	ret = platform_device_register(&higgs_dev_1);
	if (OK != ret)
		return ERROR;

	ret = platform_driver_register(&higgs_drv);
	if (OK != ret) {
		platform_device_unregister(&higgs_dev_1);
		return ERROR;
	}
#elif defined(EVB_VERSION_TEST)
	ret = platform_device_register(&higgs_dev_0);
	if (OK != ret)
		return ERROR;

	ret = platform_device_register(&higgs_dev_1);
	if (OK != ret) {
		platform_device_unregister(&higgs_dev_0);
		return ERROR;
	}

	ret = platform_driver_register(&higgs_drv);
	if (OK != ret) {
		platform_device_unregister(&higgs_dev_1);
		platform_device_unregister(&higgs_dev_0);
		return ERROR;
	}
#endif
#else

	ret = platform_driver_register(&higgs_drv);
	if (OK != ret)
		return ERROR;

	higgs_subscribe_sfp_event();
#endif

	HIGGS_TRACE(OSP_DBL_INFO, 744,
		    "Higgs module (revision is:%s) init (complied at:%s %s)",
		    higgs_sw_ver, compile_date, compile_time);


	for (i = 0; i < SAL_MAX_CARD_NUM; ++i)
		SAS_STRNCPY(ll_sw_ver[i], (char *)higgs_sw_ver,
			    (s32) strlen(higgs_sw_ver) + 1);

	return OK;
}

void __exit higgs_exit(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 16, 0)
#if defined(FPGA_VERSION_TEST)
	platform_device_unregister(&higgs_dev_1);
#elif defined(EVB_VERSION_TEST)
	platform_device_unregister(&higgs_dev_1);
	platform_device_unregister(&higgs_dev_0);
#endif
#else

	higgs_unsubscribe_sfp_event();
#endif

	platform_driver_unregister(&higgs_drv);

	HIGGS_TRACE(OSP_DBL_INFO, 744, "Higgs module (revision is:%s) exit",
		    higgs_sw_ver);
	return;
}

module_init(higgs_init);
module_exit(higgs_exit);
MODULE_LICENSE("GPL");

/**
 * higgs_wait_dma_tx_rx_idle -chip reset , wait DMA TX & RX status Idle  .
 *
 */
static s32 higgs_wait_dma_tx_rx_idle(struct higgs_card *ll_card)
{
	u32 phy_id = 0;
	U_DMA_TX_STATUS dma_tx_status;
	U_DMA_RX_STATUS dma_rx_status;
	unsigned long wait_end_time = 0;

	wait_end_time = jiffies + msecs_to_jiffies(HIGGS_CHIPRESET_WAIT_IDLE);
	while (phy_id < HIGGS_MAX_PHY_NUM) {
		dma_tx_status.u32 =
		    HIGGS_PORT_REG_READ(ll_card, phy_id,
					HISAS30HV100_PORT_REG_DMA_TX_STATUS_REG);

		dma_rx_status.u32 =
		    HIGGS_PORT_REG_READ(ll_card, phy_id,
					HISAS30HV100_PORT_REG_DMA_RX_STATUS_REG);

		if (dma_tx_status.bits.dma_tx_busy
		    || dma_rx_status.bits.dma_rx_busy) {
			HIGGS_TRACE(OSP_DBL_MINOR, 100,
				    "Card:%d phy %d DMA not idle, DmaTxSts(0x%x), DmaRxSts(0x%x)",
				    ll_card->card_id, phy_id, dma_tx_status.u32,
				    dma_rx_status.u32);

			/* timeout check */
			if (time_after(jiffies, wait_end_time))
				return ERROR;

			HIGGS_MDELAY(10);
		} else {
			phy_id++;
		}
	}

	return OK;
}

/**
 * higgs_wait_axi_bus_idle -chip reset , wait AXI bus free  .
 *
 */
static s32 higgs_wait_axi_bus_idle(struct higgs_card *ll_card)
{
	u32 curr_port_status = 0;
	unsigned long wait_end_time = 0;

	wait_end_time = jiffies + msecs_to_jiffies(HIGGS_CHIPRESET_WAIT_IDLE);
	for (;;) {
		curr_port_status =
		    HIGGS_AXIMASTERCFG_REG_READ(ll_card,
						HIGGS_AXIMASTERCFG_CURR_PORT_STS_OFFSET);
		if (0 == curr_port_status)
			return OK;

		HIGGS_TRACE(OSP_DBL_MINOR, 100,
			    "Card:%d axi master bus not idle",
			    ll_card->card_id);

		/* timeout check */
		if (time_after(jiffies, wait_end_time))
			return ERROR;

		HIGGS_MDELAY(1);
	}
}

/**
 * higgs_prepare_reset_hw - ready to chip reset.
 *
 */
static s32 higgs_prepare_reset_hw(struct higgs_card *ll_card)
{
	u32 phy_id = 0;
	U_PHY_CTRL phy_ctrl;

	/* parameter check */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);

	for (phy_id = 0; phy_id < HIGGS_MAX_PHY_NUM; phy_id++) {
		phy_ctrl.u32 =
		    HIGGS_PORT_REG_READ(ll_card, phy_id,
					HISAS30HV100_PORT_REG_PHY_CTRL_REG);
		phy_ctrl.bits.cfg_reset_req = 0x1;
		HIGGS_PORT_REG_WRITE(ll_card, phy_id,
				     HISAS30HV100_PORT_REG_PHY_CTRL_REG,
				     phy_ctrl.u32);
	}

	HIGGS_UDELAY(HIGGS_CHIPRESET_WAIT_RESETREQ);

	/* wait DMA TX & RX Idle */
	if (OK != higgs_wait_dma_tx_rx_idle(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 100,
			    "Card:%d soft reset wait DMA idle failed",
			    ll_card->card_id);
		return ERROR;
	}

	/* wait AxiBus bus free */
	if (OK != higgs_wait_axi_bus_idle(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 100,
			    "Card:%d wait axi bus idle failed!",
			    ll_card->card_id);
		return ERROR;
	}

	return OK;
}

/**
 * higgs_execute_reset_hw - config system crg register , execute chip reset.
 *
 */
static s32 higgs_execute_reset_hw(struct higgs_card *ll_card)
{
	u32 reg_val = 0;
	u64 sub_ctrl_base = 0;
	u32 sub_ctrl_range = 0;
	u64 rst_reg_addr = 0;
	u64 derst_reg_addr = 0;
	u32 rst_val = 0;
	u32 derst_val = 0;
	u64 rst_stat_reg_addr = 0;

	/* parameter check */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)	/* < linux 3.19 */

	if (P660_SAS_CORE_DSAF_ID == ll_card->card_id) {
		sub_ctrl_base = HIGGS_DSAF_SUBCTL_BASE;
		sub_ctrl_range = HIGGS_DSAF_SUBCTL_RANGE;
		rst_reg_addr = HIGGS_DSAF_SUB_CTRL_RESET_OFFSET;
		derst_reg_addr = HIGGS_DSAF_SUB_CTRL_DERESET_OFFSET;
		rst_stat_reg_addr = HIGGS_DSAF_SUB_CTRL_RESET_STATUS_OFFSET;
		rst_val = HIGGS_DSAF_SUB_CTRL_RESET_VALUE;
		derst_val = HIGGS_DSAF_SUB_CTRL_DERESET_VALUE;
	} else if (P660_SAS_CORE_PCIE_ID == ll_card->card_id) {
		sub_ctrl_base = HIGGS_PCIE_SUBCTL_BASE;
		sub_ctrl_range = HIGGS_PCIE_SUBCTL_RANGE;
		rst_reg_addr = HIGGS_PCIE_SUB_CTRL_RESET_OFFSET;
		derst_reg_addr = HIGGS_PCIE_SUB_CTRL_DERESET_OFFSET;
		rst_stat_reg_addr = HIGGS_PCIE_SUB_CTRL_RESET_STATUS_OFFSET;
		rst_val = HIGGS_PCIE_SUB_CTRL_RESET_VALUE;
		derst_val = HIGGS_PCIE_SUB_CTRL_DERESET_VALUE;
	} else if (P660_1_SAS_CORE_DSAF_ID == ll_card->card_id) {
		sub_ctrl_base = HIGGS_SLAVE_DSAF_SUBCTL_BASE;
		sub_ctrl_range = HIGGS_DSAF_SUBCTL_RANGE;
		rst_reg_addr = HIGGS_DSAF_SUB_CTRL_RESET_OFFSET;
		derst_reg_addr = HIGGS_DSAF_SUB_CTRL_DERESET_OFFSET;
		rst_stat_reg_addr = HIGGS_DSAF_SUB_CTRL_RESET_STATUS_OFFSET;
		rst_val = HIGGS_DSAF_SUB_CTRL_RESET_VALUE;
		derst_val = HIGGS_DSAF_SUB_CTRL_DERESET_VALUE;
	} else if (P660_1_SAS_CORE_PCIE_ID == ll_card->card_id) {
		sub_ctrl_base = HIGGS_SLAVE_PCIE_SUBCTL_BASE;
		sub_ctrl_range = HIGGS_PCIE_SUBCTL_RANGE;
		rst_reg_addr = HIGGS_PCIE_SUB_CTRL_RESET_OFFSET;
		derst_reg_addr = HIGGS_PCIE_SUB_CTRL_DERESET_OFFSET;
		rst_stat_reg_addr = HIGGS_PCIE_SUB_CTRL_RESET_STATUS_OFFSET;
		rst_val = HIGGS_PCIE_SUB_CTRL_RESET_VALUE;
		derst_val = HIGGS_PCIE_SUB_CTRL_DERESET_VALUE;
	} else {
		return ERROR;
	}
#else

	if (HIGGS_HILINK_TYPE_DSAF == ll_card->card_cfg.hilink_type) {

        if (ll_card->card_cfg.cpu_node_id > 0) {
            sub_ctrl_base = HIGGS_SLAVE_DSAF_SUBCTL_BASE;
            sub_ctrl_range = HIGGS_DSAF_SUBCTL_RANGE;
            rst_reg_addr = HIGGS_DSAF_SUB_CTRL_RESET_OFFSET;
            derst_reg_addr = HIGGS_DSAF_SUB_CTRL_DERESET_OFFSET;
            rst_stat_reg_addr = HIGGS_DSAF_SUB_CTRL_RESET_STATUS_OFFSET;
            rst_val = HIGGS_DSAF_SUB_CTRL_RESET_VALUE;
            derst_val = HIGGS_DSAF_SUB_CTRL_DERESET_VALUE;

        } else {
		    sub_ctrl_base = HIGGS_DSAF_SUBCTL_BASE;
		    sub_ctrl_range = HIGGS_DSAF_SUBCTL_RANGE;
		    rst_reg_addr = HIGGS_DSAF_SUB_CTRL_RESET_OFFSET;
		    derst_reg_addr = HIGGS_DSAF_SUB_CTRL_DERESET_OFFSET;
		    rst_stat_reg_addr = HIGGS_DSAF_SUB_CTRL_RESET_STATUS_OFFSET;
		    rst_val = HIGGS_DSAF_SUB_CTRL_RESET_VALUE;
		    derst_val = HIGGS_DSAF_SUB_CTRL_DERESET_VALUE;
        }
            
	} else if (HIGGS_HILINK_TYPE_PCIE == ll_card->card_cfg.hilink_type) {

		if (ll_card->card_cfg.cpu_node_id > 0) {
			sub_ctrl_base = HIGGS_SLAVE_PCIE_SUBCTL_BASE;
			sub_ctrl_range = HIGGS_PCIE_SUBCTL_RANGE;
			rst_reg_addr = HIGGS_PCIE_SUB_CTRL_RESET_OFFSET;
			derst_reg_addr = HIGGS_PCIE_SUB_CTRL_DERESET_OFFSET;
			rst_stat_reg_addr = HIGGS_PCIE_SUB_CTRL_RESET_STATUS_OFFSET;
			rst_val = HIGGS_PCIE_SUB_CTRL_RESET_VALUE;
			derst_val = HIGGS_PCIE_SUB_CTRL_DERESET_VALUE;
        
        } else {    
		    sub_ctrl_base = HIGGS_PCIE_SUBCTL_BASE;
		    sub_ctrl_range = HIGGS_PCIE_SUBCTL_RANGE;
		    rst_reg_addr = HIGGS_PCIE_SUB_CTRL_RESET_OFFSET;
		    derst_reg_addr = HIGGS_PCIE_SUB_CTRL_DERESET_OFFSET;
		    rst_stat_reg_addr = HIGGS_PCIE_SUB_CTRL_RESET_STATUS_OFFSET;
		    rst_val = HIGGS_PCIE_SUB_CTRL_RESET_VALUE;
		    derst_val = HIGGS_PCIE_SUB_CTRL_DERESET_VALUE;
        }
            
	} else {
	    return ERROR;
	}

#endif
	/* io remap */
	sub_ctrl_base = (u64) SAS_IOREMAP(sub_ctrl_base, sub_ctrl_range);

	/* reset  */
	HIGGS_REG_WRITE_DWORD((void *)(sub_ctrl_base + rst_reg_addr), rst_val);
	HIGGS_MDELAY(1);
	reg_val =
	    HIGGS_REG_READ_DWORD((void *)(sub_ctrl_base + rst_stat_reg_addr));
	if (HIGGS_SAS_RESET_STATUS_RESET !=
	    (reg_val & HIGGS_SAS_RESET_STATUS_MASK)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 100, "Card:%d sas reset failed",
			    ll_card->card_id);
		return ERROR;
	}

	HIGGS_REG_WRITE_DWORD((void *)(sub_ctrl_base + derst_reg_addr),
			      derst_val);
	HIGGS_MDELAY(1);
	reg_val =
	    HIGGS_REG_READ_DWORD((void *)(sub_ctrl_base + rst_stat_reg_addr));
	if (HIGGS_SAS_RESET_STATUS_DERESET !=
	    (reg_val & HIGGS_SAS_RESET_STATUS_MASK)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 100, "Card:%d sas dereset failed",
			    ll_card->card_id);
		return ERROR;
	}

	/* io unmap */
	SAS_IOUNMAP((void *)sub_ctrl_base);

	return OK;
}

/**
 * higgs_recycle_all_dev - delete port exception may lead to dev resource leak.
 *
 */
void higgs_recycle_all_dev(struct higgs_card *ll_card)
{
	u32 i = 0;
	struct higgs_device *ll_dev = NULL;
	struct sal_dev *sal_dev = NULL;

	ll_dev = (struct higgs_device *)ll_card->dev_mem;

	for (i = 0; i < ll_card->higgs_can_dev; i++) {
		if (NULL != ll_dev->up_dev) {
			sal_dev = (struct sal_dev *)ll_dev->up_dev;
			sal_dev->ll_dev = NULL;

			higgs_recycle_dev(ll_card, ll_dev);
		}
		ll_dev++;
	}

	return;
}

/**
 * higgs_clear_dq_cq_pointer - chip reset.
 *
 */
void higgs_clear_dq_cq_pointer(struct higgs_card *ll_card)
{
	u32 i = 0;
	struct higgs_dqueue *dq_queue = NULL;
	struct higgs_cqueue *cq_queue = NULL;

	for (i = 0; i < ll_card->higgs_can_dq; i++) {
		dq_queue = &ll_card->io_hw_struct_info.delivery_queue[i];
		dq_queue->wpointer = 0;
		HIGGS_TRACE(OSP_DBL_INFO, 100,
			    "iDqNum=%d, wpointer = 0x%x,uiChipWPointer=0x%x,uiChipRPointer=0x%x",
			    i, dq_queue->wpointer,
			    HIGGS_DQ_WPT_GET(ll_card, i),
			    HIGGS_DQ_RPT_GET(ll_card, i));
	}

	for (i = 0; i < ll_card->higgs_can_cq; i++) {
		cq_queue = &ll_card->io_hw_struct_info.complete_queue[i];
		cq_queue->rpointer = 0;
		HIGGS_TRACE(OSP_DBL_INFO, 100,
			    "iCqNum=%d, rpointer = 0x%x,uiChipRPointer = 0x%x,uiChipWPointer=0x%x",
			    i, cq_queue->rpointer,
			    HIGGS_CQ_RPT_GET(ll_card, i),
			    HIGGS_CQ_WPT_GET(ll_card, i));
	}
	return;
}

/**
 * higgs_wait_chip_all_dev_Idle - chip reset.
 *
 */
void higgs_wait_chip_all_dev_Idle(struct sal_card *sal_card)
{
	u32 i = 0;
	struct sal_port *sal_port = NULL;
	static unsigned long last = 0;

	for (i = 0; i < SAL_MAX_PHY_NUM; i++) {
		sal_port = sal_card->port[i];
		while ((SAS_ATOMIC_READ(&sal_port->dev_num) != 0) &&
		       (!time_after_eq(jiffies, last + SAL_DEL_DEV_TIMEOUT))) {
			HIGGS_TRACE(OSP_DBL_INFO, 100,
				    "port id %d, dev num is %d", i,
				    SAS_ATOMIC_READ(&sal_port->dev_num));
			SAS_MSLEEP(10);
		}
	}

	return;
}

/**
 * higgs_reset_chip - chip reset.
 *
 */
s32 higgs_reset_chip(struct sal_card * sal_card)
{
	struct higgs_card *ll_card = NULL;
	u32 i = 0;
	unsigned long flag = 0;

	/* parameter check */
	HIGGS_ASSERT(NULL != sal_card, return ERROR);
	ll_card = (struct higgs_card *)sal_card->drv_data;
	HIGGS_ASSERT(NULL != ll_card, return ERROR);

	//higgs_wait_chip_all_dev_Idle(sal_card);
	/*modify PHY flag */
	for (i = 0; i < sal_card->phy_num; i++) {
		sal_card->phy[i]->link_status = SAL_PHY_CLOSED;
		sal_card->phy[i]->link_rate = SAL_LINK_RATE_FREE;
	}

	/*close interrupt */
	higgs_close_all_intr(ll_card);
#ifdef FPGA_VERSION_TEST
	/*release interrupt*/
	higgs_release_intr(ll_card);
#endif

	/* feset hardware  */
	if (OK != higgs_reset_hw(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MINOR, 100, "Card:%d reset HW failed!",
			    sal_card->card_no);
		goto RESET_FAILED;
	}

	spin_lock_irqsave(&ll_card->sal_card->card_lock, flag);
	ll_card->sal_card->flag |= SAL_CARD_RESETTING;
	spin_unlock_irqrestore(&ll_card->sal_card->card_lock, flag);

	/* recycle Active REQ  resource */
	higgs_recycle_active_req_rsc(ll_card);

	/* recycle dev resource  */
	higgs_recycle_all_dev(ll_card);

	spin_lock_irqsave(&ll_card->card_lock, flag);
	for (i = 0; i < HIGGS_MAX_PHY_NUM; ++i) {
		sal_del_timer((void *)&ll_card->phy[i].phy_run_serdes_fw_timer);
		ll_card->phy[i].run_serdes_fw_timer_en = false;
	}
	spin_unlock_irqrestore(&ll_card->card_lock, flag);

	(void)higgs_test_reg_access(ll_card);

	/*initialize hardware  */
	if (OK != higgs_init_hw(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 100,
			    "Card:%d init HW failed,fail reason is %d",
			    sal_card->card_no,
			    sal_global.init_failed_reason[ll_card->card_id]);
		goto RESET_FAILED;
	}

	higgs_clr_mem_after_reset(ll_card);

	higgs_clear_dq_cq_pointer(ll_card);

#ifdef FPGA_VERSION_TEST
	if (OK != higgs_init_intr(ll_card)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 100,
			    "Card:%d init Intr failed,fail reason is %d",
			    sal_card->card_no,
			    sal_global.init_failed_reason[ll_card->card_id]);
		goto RESET_FAILED;
	}
#endif
	higgs_open_all_intr(ll_card);

	HIGGS_TRACE(OSP_DBL_INFO, 100, "Card:%d chip soft reset succeeded",
		    ll_card->card_id);
	return OK;

      RESET_FAILED:
	spin_lock_irqsave(&sal_card->card_lock, flag);
	sal_card->flag &= ~SAL_CARD_RESETTING;
	sal_card->flag |= SAL_CARD_FATAL;
	spin_unlock_irqrestore(&sal_card->card_lock, flag);
	return RETURN_OPERATION_FAILED;
}

/**
 * higgs_comm_pdprocess - AC logout in low-layer.
 *
 */
s32 higgs_comm_pdprocess(struct higgs_card * ll_card)
{
	HIGGS_ASSERT(NULL != ll_card, return ERROR);

	/*report API/Frame delete port */
	(void)higgs_del_register_port(ll_card);


	sal_notify_disc_shut_down(ll_card->sal_card);

	higgs_close_all_phy(ll_card);

	/*clear all IO of chip  */
	if (SAL_PORT_MODE_INI == ll_card->card_cfg.work_mode)
		(void)sal_abort_all_dev_io(ll_card->sal_card);

	higgs_close_all_intr(ll_card);
	higgs_release_intr(ll_card);

	sal_remove_threads(ll_card->sal_card);

	sal_complete_all_io(ll_card->sal_card);
#if 0 /* remove chg delay */

	sasini_dev_chg_delay_exit((u8) ll_card->card_id);
#else
	sal_remove_all_dev((u8) ll_card->card_id);
#endif

	return OK;
}
