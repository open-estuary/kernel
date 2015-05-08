
#include "higgs_serdes_api.h"

#define YEYDATA_BUFF (512 * 8 * 2)

static unsigned char use_ssc = 0;
//0-Normal CDR(default), 1-SSCDR
static unsigned char cdr_mode = 0;
//static unsigned char  hilink1_8g = 1;
//static enum board_type ebord_type = EM_16CORE_EVB_BOARD;

static unsigned char sds_tx_polarity[2][7][8] = { {{0}} };
static unsigned char sds_rx_polarity[2][7][8] = { {{0}} };

#define DSAF_SUB_BASE                             (0xC0000000)
#define PCIE_SUB_BASE                             (0xB0000000)

#define DSAF_SUB_BASE_SLAVE_CPU                   (0x40000000000ULL + 0xC0000000)

#define PCIE_SUB_BASE_SLAVE_CPU                   (0x40000000000ULL + 0xB0000000)

#define PCIE_SUB_BASE_SIZE                        (0x10000)
#define DSAF_SUB_BASE_SIZE                        (0x10000)

static unsigned long long macro0_virt_base = 0;
static unsigned long long macro1_virt_base = 0;
static unsigned long long macro2_virt_base = 0;
static unsigned long long macro3_virt_base = 0;
static unsigned long long macro4_virt_base = 0;
static unsigned long long macro5_virt_base = 0;
static unsigned long long macro6_virt_base = 0;

static unsigned long long slave_macro0_virt_base = 0;
static unsigned long long slave_macro1_virt_base = 0;
static unsigned long long slave_macro2_virt_base = 0;
static unsigned long long slave_macro3_virt_base = 0;
static unsigned long long slave_macro4_virt_base = 0;
static unsigned long long slave_macro5_virt_base = 0;
static unsigned long long slave_macro6_virt_base = 0;

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/
static unsigned long long sub_pcie_base_addr = 0;
static unsigned long long sub_pcie_pa_addr = 0;

//add by chenqilin
static unsigned long long sub_pcie_base_addr_slavecpu = 0;
static unsigned long long sub_pcie_pa_addr_slavecpu = 0;
//end

static unsigned long long sub_dsaf_base_addr = 0;
static unsigned long long sub_dsaf_pa_addr = 0;

//add by chenqilin
static unsigned long long sub_dsaf_base_addr_slavecpu = 0;
static unsigned long long sub_dsaf_pa_addr_slavecpu = 0;
//end

/*****************************************************************************
 函 数 名  : hilink0_reg_init
 功能描述  : hilink 寄存器映射
 输入参数  : void
 输出参数  : 无
 返 回 值  : int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年6月6日
    作    者   : z00176027
    修改内容   : 新生成函数

*****************************************************************************/
static int hilink0_reg_init(void)
{

	macro0_virt_base =
	    (unsigned long long)ioremap(HILINKMACRO0, HILINKMACRO_SIZE);
	if (!macro0_virt_base) {
		printk(KERN_ERR HILINK_NAME
		       ": could not of_iomap Hilink registers addr:0x%x,size:0x%x\n",
		       HILINKMACRO0, HILINKMACRO_SIZE);
		return -1;
	}

	/* begin:支持2P ARM SERVER, 从片CPU */
	if (max_cpu_nodes > 1) {
		slave_macro0_virt_base =
		    (unsigned long long)ioremap(HILINKMACRO0 + 0x40000000000ULL,
						HILINKMACRO_SIZE);
		if (!slave_macro0_virt_base) {
			/* 主片CPU unmap hilink addr */
			iounmap((void *)macro0_virt_base);
			printk(KERN_ERR HILINK_NAME
			       ": could not of_iomap SLAVE Hilink registers addr:0x%x,size:0x%x\n",
			       HILINKMACRO0, HILINKMACRO_SIZE);
			return -1;
		}
	}
	/* end:支持2P ARM SERVER */

	return 0;
}

/*****************************************************************************
 函 数 名  : hilink1_reg_init
 功能描述  : hilink 寄存器映射
 输入参数  : void
 输出参数  : 无
 返 回 值  : int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年6月6日
    作    者   : z00176027
    修改内容   : 新生成函数

*****************************************************************************/
static int hilink1_reg_init(void)
{

	macro1_virt_base =
	    (unsigned long long)ioremap(HILINKMACRO1, HILINKMACRO_SIZE);
	if (!macro1_virt_base) {
		printk(KERN_ERR HILINK_NAME
		       ": could not ioremap Hilink registers addr:0x%x,size:0x%x\n",
		       HILINKMACRO1, HILINKMACRO_SIZE);
		return -1;
	}
	/* begin:支持2P ARM SERVER, 从片CPU */
	if (max_cpu_nodes > 1) {
		slave_macro1_virt_base =
		    (unsigned long long)ioremap(HILINKMACRO1 + 0x40000000000ULL,
						HILINKMACRO_SIZE);
		if (!slave_macro1_virt_base) {
			/* 主片CPU unmap hilink addr */
			iounmap((void *)macro1_virt_base);
			printk(KERN_ERR HILINK_NAME
			       ": could not of_iomap SLAVE Hilink registers addr:0x%x,size:0x%x\n",
			       HILINKMACRO1, HILINKMACRO_SIZE);
			return -1;
		}
	}
	/* end:支持2P ARM SERVER */
	return 0;
}

/*****************************************************************************
 函 数 名  : hilink1_reg_init
 功能描述  : hilink 寄存器映射
 输入参数  : void
 输出参数  : 无
 返 回 值  : int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年6月6日
    作    者   : z00176027
    修改内容   : 新生成函数

*****************************************************************************/
static int hilink2_reg_init(void)
{
	//printk("hilink2_phy_addr:0x%llx,size:0x%llx\n",hilink2_phy_addr,size);

	macro2_virt_base =
	    (unsigned long long)ioremap(HILINKMACRO2, HILINKMACRO_SIZE);
	if (!macro2_virt_base) {
		printk(KERN_ERR HILINK_NAME
		       ": could not ioremap Hilink registers addr:0x%x,size:0x%x\n",
		       HILINKMACRO2, HILINKMACRO_SIZE);
		return -1;
	}

	/* begin:支持2P ARM SERVER, 从片CPU */
	if (max_cpu_nodes > 1) {
		slave_macro2_virt_base =
		    (unsigned long long)ioremap(HILINKMACRO2 + 0x40000000000ULL,
						HILINKMACRO_SIZE);
		if (!slave_macro2_virt_base) {
			/* 主片CPU unmap hilink addr */
			iounmap((void *)macro2_virt_base);
			printk(KERN_ERR HILINK_NAME
			       ": could not of_iomap SLAVE Hilink registers addr:0x%x,size:0x%x\n",
			       HILINKMACRO2, HILINKMACRO_SIZE);
			return -1;
		}
	}
	/* end:支持2P ARM SERVER */
	return 0;
}

/*****************************************************************************
 函 数 名  : hilink1_reg_init
 功能描述  : hilink 寄存器映射
 输入参数  : void
 输出参数  : 无
 返 回 值  : int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年6月6日
    作    者   : z00176027
    修改内容   : 新生成函数

*****************************************************************************/
static int hilink3_reg_init(void)
{
	//printk("hilink3_phy_addr:0x%llx,size:0x%llx\n",hilink3_phy_addr, size);

	macro3_virt_base =
	    (unsigned long long)ioremap(HILINKMACRO3, HILINKMACRO_SIZE);
	if (!macro3_virt_base) {
		printk(KERN_ERR HILINK_NAME
		       ": could not ioremap Hilink registers addr:0x%x,size:0x%x\n",
		       HILINKMACRO3, HILINKMACRO_SIZE);
		return -1;
	}
	/* begin:支持2P ARM SERVER, 从片CPU */
	if (max_cpu_nodes > 1) {
		slave_macro3_virt_base =
		    (unsigned long long)ioremap(HILINKMACRO3 + 0x40000000000ULL,
						HILINKMACRO_SIZE);
		if (!slave_macro3_virt_base) {
			/* 主片CPU unmap hilink addr */
			iounmap((void *)macro3_virt_base);
			printk(KERN_ERR HILINK_NAME
			       ": could not of_iomap SLAVE Hilink registers addr:0x%x,size:0x%x\n",
			       HILINKMACRO3, HILINKMACRO_SIZE);
			return -1;
		}
	}
	/* end:支持2P ARM SERVER */
	return 0;
}

/*****************************************************************************
 函 数 名  : hilink1_reg_init
 功能描述  : hilink 寄存器映射
 输入参数  : void
 输出参数  : 无
 返 回 值  : int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年6月6日
    作    者   : z00176027
    修改内容   : 新生成函数

*****************************************************************************/
static int hilink4_reg_init(void)
{
	//printk("hilink4_phy_addr:0x%llx,size:0x%llx\n",hilink4_phy_addr, size);

	macro4_virt_base =
	    (unsigned long long)ioremap(HILINKMACRO4, HILINKMACRO_SIZE);
	if (!macro4_virt_base) {
		printk(KERN_ERR HILINK_NAME
		       ": could not ioremap Hilink registers addr:0x%x,size:0x%x\n",
		       HILINKMACRO4, HILINKMACRO_SIZE);
		return -1;
	}
	/* begin:支持2P ARM SERVER, 从片CPU */
	if (max_cpu_nodes > 1) {
		slave_macro4_virt_base =
		    (unsigned long long)ioremap(HILINKMACRO4 + 0x40000000000ULL,
						HILINKMACRO_SIZE);
		if (!slave_macro4_virt_base) {
			/* 主片CPU unmap hilink addr */
			iounmap((void *)macro4_virt_base);
			printk(KERN_ERR HILINK_NAME
			       ": could not of_iomap SLAVE Hilink registers addr:0x%x,size:0x%x\n",
			       HILINKMACRO4, HILINKMACRO_SIZE);
			return -1;
		}
	}
	/* end:支持2P ARM SERVER */
	return 0;
}

/*****************************************************************************
 函 数 名  : hilink1_reg_init
 功能描述  : hilink 寄存器映射
 输入参数  : void
 输出参数  : 无
 返 回 值  : int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年6月6日
    作    者   : z00176027
    修改内容   : 新生成函数

*****************************************************************************/
static int hilink5_reg_init(void)
{
	//printk("hilink5_phy_addr:0x%llx,size:0x%llx\n",hilink5_phy_addr, size);

	macro5_virt_base =
	    (unsigned long long)ioremap(HILINKMACRO5, HILINKMACRO_SIZE);
	if (!macro5_virt_base) {
		printk(KERN_ERR HILINK_NAME
		       ": could not ioremap Hilink registers addr:0x%x,size:0x%x\n",
		       HILINKMACRO5, HILINKMACRO_SIZE);
		return -1;
	}
	/* begin:支持2P ARM SERVER, 从片CPU */
	if (max_cpu_nodes > 1) {
		slave_macro5_virt_base =
		    (unsigned long long)ioremap(HILINKMACRO5 + 0x40000000000ULL,
						HILINKMACRO_SIZE);
		if (!slave_macro5_virt_base) {
			/* 主片CPU unmap hilink addr */
			iounmap((void *)macro5_virt_base);
			printk(KERN_ERR HILINK_NAME
			       ": could not of_iomap SLAVE Hilink registers addr:0x%x,size:0x%x\n",
			       HILINKMACRO5, HILINKMACRO_SIZE);
			return -1;
		}
	}
	/* end:支持2P ARM SERVER */
	return 0;
}

/*****************************************************************************
 函 数 名  : hilink1_reg_init
 功能描述  : hilink 寄存器映射
 输入参数  : void
 输出参数  : 无
 返 回 值  : int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年6月6日
    作    者   : z00176027
    修改内容   : 新生成函数

*****************************************************************************/
static int hilink6_reg_init(void)
{
	//printk("hilink6_phy_addr:0x%llx,size:0x%llx\n",hilink6_phy_addr, size);

	macro6_virt_base =
	    (unsigned long long)ioremap(HILINKMACRO6, HILINKMACRO_SIZE);;
	if (!macro6_virt_base) {
		printk(KERN_ERR HILINK_NAME
		       ": could not ioremap Hilink registers addr:0x%x,size:0x%x\n",
		       HILINKMACRO6, HILINKMACRO_SIZE);
		return -1;
	}
	/* begin:支持2P ARM SERVER, 从片CPU */
	if (max_cpu_nodes > 1) {
		slave_macro6_virt_base =
		    (unsigned long long)ioremap(HILINKMACRO6 + 0x40000000000ULL,
						HILINKMACRO_SIZE);
		if (!slave_macro6_virt_base) {
			/* 主片CPU unmap hilink addr */
			iounmap((void *)macro6_virt_base);
			printk(KERN_ERR HILINK_NAME
			       ": could not of_iomap SLAVE Hilink registers addr:0x%x,size:0x%x\n",
			       HILINKMACRO6, HILINKMACRO_SIZE);
			return -1;
		}
	}
	/* end:支持2P ARM SERVER */
	return 0;
}

/*****************************************************************************
 函 数 名  : hilink0_reg_exit
 功能描述  : hilink 寄存器映射释放0
 输入参数  : int index
 输出参数  : 无
 返 回 值  : int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年6月6日
    作    者   : z00176027
    修改内容   : 新生成函数

*****************************************************************************/
static int hilink0_reg_exit(void)
{
	if (macro0_virt_base > 0)
		iounmap((void *)macro0_virt_base);

	/* begin:支持2P ARM SERVER, 从片CPU */
	if (slave_macro0_virt_base > 0)
		iounmap((void *)slave_macro0_virt_base);
	/* end:支持2P ARM SERVER */

	return 0;
}

/*****************************************************************************
 函 数 名  : hilink1_reg_exit
 功能描述  : hilink 寄存器映射释放0
 输入参数  : int index
 输出参数  : 无
 返 回 值  : int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年6月6日
    作    者   : z00176027
    修改内容   : 新生成函数

*****************************************************************************/
static int hilink1_reg_exit(void)
{
	if (macro1_virt_base > 0)
		iounmap((void *)macro1_virt_base);

	/* begin:支持2P ARM SERVER, 从片CPU */
	if (slave_macro1_virt_base > 0)
		iounmap((void *)slave_macro1_virt_base);
	/* end:支持2P ARM SERVER */

	return 0;

}

/*****************************************************************************
 函 数 名  : hilink2_reg_exit
 功能描述  : hilink 寄存器映射释放0
 输入参数  : int index
 输出参数  : 无
 返 回 值  : int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年6月6日
    作    者   : z00176027
    修改内容   : 新生成函数

*****************************************************************************/
static int hilink2_reg_exit(void)
{
	if (macro2_virt_base > 0)
		iounmap((void *)macro2_virt_base);

	/* begin:支持2P ARM SERVER, 从片CPU */
	if (slave_macro2_virt_base > 0)
		iounmap((void *)slave_macro2_virt_base);
	/* end:支持2P ARM SERVER */

	return 0;
}

/*****************************************************************************
 函 数 名  : hilink3_reg_exit
 功能描述  : hilink 寄存器映射释放0
 输入参数  : int index
 输出参数  : 无
 返 回 值  : int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年6月6日
    作    者   : z00176027
    修改内容   : 新生成函数

*****************************************************************************/
static int hilink3_reg_exit(void)
{
	if (macro3_virt_base > 0)
		iounmap((void *)macro3_virt_base);

	/* begin:支持2P ARM SERVER, 从片CPU */
	if (slave_macro3_virt_base > 0)
		iounmap((void *)slave_macro3_virt_base);
	/* end:支持2P ARM SERVER */
	return 0;

}

/*****************************************************************************
 函 数 名  : hilink4_reg_exit
 功能描述  : hilink 寄存器映射释放0
 输入参数  : int index
 输出参数  : 无
 返 回 值  : int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年6月6日
    作    者   : z00176027
    修改内容   : 新生成函数

*****************************************************************************/
static int hilink4_reg_exit(void)
{
	if (macro4_virt_base > 0)
		iounmap((void *)macro4_virt_base);

	/* begin:支持2P ARM SERVER, 从片CPU */
	if (slave_macro4_virt_base > 0)
		iounmap((void *)slave_macro4_virt_base);
	/* end:支持2P ARM SERVER */
	return 0;
}

/*****************************************************************************
 函 数 名  : hilink5_reg_exit
 功能描述  : hilink 寄存器映射释放0
 输入参数  : int index
 输出参数  : 无
 返 回 值  : int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年6月6日
    作    者   : z00176027
    修改内容   : 新生成函数

*****************************************************************************/
static int hilink5_reg_exit(void)
{
	if (macro5_virt_base > 0)
		iounmap((void *)macro5_virt_base);

	/* begin:支持2P ARM SERVER, 从片CPU */
	if (slave_macro5_virt_base > 0)
		iounmap((void *)slave_macro5_virt_base);
	/* end:支持2P ARM SERVER */
	return 0;
}

/*****************************************************************************
 函 数 名  : hilink6_reg_exit
 功能描述  : hilink 寄存器映射释放0
 输入参数  : int index
 输出参数  : 无
 返 回 值  : int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年6月6日
    作    者   : z00176027
    修改内容   : 新生成函数

*****************************************************************************/
static int hilink6_reg_exit(void)
{
	if (macro6_virt_base > 0)
		iounmap((void *)macro6_virt_base);

	/* begin:支持2P ARM SERVER, 从片CPU */
	if (slave_macro6_virt_base > 0)
		iounmap((void *)slave_macro6_virt_base);
	/* end:支持2P ARM SERVER */
	return 0;
}

int hilink_reg_init(void)
{
	int ret;

	ret = hilink0_reg_init();
	if (0 != ret)
		printk("%s fail,ret:0x%x\n", __FUNCTION__, ret);

	ret = hilink1_reg_init();
	if (0 != ret) {
		printk("%s fail,ret:0x%x\n", __FUNCTION__, ret);

		(void)hilink0_reg_exit();
	}

	ret = hilink2_reg_init();
	if (0 != ret) {
		printk("%s fail,ret:0x%x\n", __FUNCTION__, ret);

		(void)hilink1_reg_exit();
		(void)hilink0_reg_exit();
	}

	ret = hilink3_reg_init();
	if (0 != ret) {
		printk("%s fail,ret:0x%x\n", __FUNCTION__, ret);

		(void)hilink2_reg_exit();
		(void)hilink1_reg_exit();
		(void)hilink0_reg_exit();
	}

	ret = hilink4_reg_init();
	if (0 != ret) {
		printk("%s fail,ret:0x%x\n", __FUNCTION__, ret);

		(void)hilink3_reg_exit();
		(void)hilink2_reg_exit();
		(void)hilink1_reg_exit();
		(void)hilink0_reg_exit();
	}

	ret = hilink5_reg_init();
	if (0 != ret) {
		printk("%s fail,ret:0x%x\n", __FUNCTION__, ret);

		(void)hilink4_reg_exit();
		(void)hilink3_reg_exit();
		(void)hilink2_reg_exit();
		(void)hilink1_reg_exit();
		(void)hilink0_reg_exit();
	}

	ret = hilink6_reg_init();
	if (0 != ret) {
		printk("%s fail,ret:0x%x\n", __FUNCTION__, ret);

		(void)hilink5_reg_exit();
		(void)hilink4_reg_exit();
		(void)hilink3_reg_exit();
		(void)hilink2_reg_exit();
		(void)hilink1_reg_exit();
		(void)hilink0_reg_exit();
	}

	return 0;
}

/*****************************************************************************
 函 数 名  : hilink_reg_exit
 功能描述  : hilink寄存器资源释放
 输入参数  : void
 输出参数  : 无
 返 回 值  : static
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年6月6日
    作    者   : z00176027
    修改内容   : 新生成函数

*****************************************************************************/
void hilink_reg_exit(void)
{
	(void)hilink6_reg_exit();
	(void)hilink5_reg_exit();
	(void)hilink4_reg_exit();
	(void)hilink3_reg_exit();
	(void)hilink2_reg_exit();
	(void)hilink1_reg_exit();
	(void)hilink0_reg_exit();
}

static void serdes_delay_us(unsigned int time)
{

	if (time >= 1000) {
		mdelay(time / 1000);
		time = (time % 1000);
	}

	if (time != 0)
		udelay(time);
}

unsigned int hilink_sub_dsaf_init(void)
{
	sub_dsaf_pa_addr = DSAF_SUB_BASE;

	/* SUB ALG申请io内存 */
/*    if (!request_mem_region(sub_dsaf_pa_addr, DSAF_SUB_BASE_SIZE, "SUB DSAF Reg"))
    {

        printk("SUB DSA region busy\n");

        return ERROR;
    }
*/
	sub_dsaf_base_addr =
	    (unsigned long long)ioremap(sub_dsaf_pa_addr, DSAF_SUB_BASE_SIZE);
	if (!sub_dsaf_base_addr) {
		//release_mem_region(sub_dsaf_pa_addr, DSAF_SUB_BASE_SIZE);
		printk("could not ioremap SUB DSA registers\n");

		return ERROR;
	}

	/* add by chenqilin, for slave cpu of arm server */
	if (max_cpu_nodes > 1) {
		sub_dsaf_pa_addr_slavecpu = DSAF_SUB_BASE_SLAVE_CPU;

		/* SUB ALG申请io内存 */
/*	    if (!request_mem_region(sub_dsaf_pa_addr_slavecpu, DSAF_SUB_BASE_SIZE, "SLAVE SUB DSAF Reg"))
	    {
	        printk("SLAVE CPU SUB DSAF region busy\n");
	        return ERROR;
	    }
*/
		sub_dsaf_base_addr_slavecpu =
		    (unsigned long long)ioremap(sub_dsaf_pa_addr_slavecpu,
						DSAF_SUB_BASE_SIZE);
		if (!sub_dsaf_base_addr_slavecpu) {
			//release_mem_region(sub_dsaf_pa_addr_slavecpu, DSAF_SUB_BASE_SIZE);
			printk
			    ("could not ioremap SLAVE CPU SUB DSAF registers\n");

			return ERROR;
		}
	}
	/* end */

	return OK;
}

/*****************************************************************************
 函 数 名  : HRD_SubAlgExit
 功能描述  : SUB DSAF CRG 退出
 输入参数  : void
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年11月21日
    作    者   : z00176027
    修改内容   : 新生成函数

*****************************************************************************/
void hilink_sub_dsaf_exit(void)
{
	/* 释放SUB dsaf 资源 */
	if (sub_dsaf_base_addr > 0)
		//release_mem_region(sub_dsaf_pa_addr, DSAF_SUB_BASE_SIZE);
		iounmap((void *)sub_dsaf_base_addr);

	if (sub_dsaf_base_addr_slavecpu > 0)
		//release_mem_region(sub_dsaf_pa_addr_slavecpu, DSAF_SUB_BASE_SIZE);
		iounmap((void *)sub_dsaf_base_addr_slavecpu);
}

static unsigned long long comm_sub_dsaf_get_base(unsigned int node)
{
	/* 主片node=0 */
	if (node == MASTER_CPU_NODE)
		return sub_dsaf_base_addr;
	else
		return sub_dsaf_base_addr_slavecpu;
}

/*****************************************************************************
 函 数 名  : sub_pcie_init
 功能描述  : SUB PCIE CRG linux初始化
 输入参数  : void
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年11月21日
    作    者   : z00176027
    修改内容   : 新生成函数

*****************************************************************************/
unsigned int hilink_sub_pcie_init(void)
{
	sub_pcie_pa_addr = PCIE_SUB_BASE;

	/* SUB ALG申请io内存 */
	/* if (!request_mem_region(sub_pcie_pa_addr, PCIE_SUB_BASE_SIZE, "SUB PCIE Reg"))
	   {
	   printk("SUB PCIE region busy\n");
	   return ERROR;
	   }
	 */
	sub_pcie_base_addr =
	    (unsigned long long)ioremap(sub_pcie_pa_addr, PCIE_SUB_BASE_SIZE);
	if (!sub_pcie_base_addr) {
		//release_mem_region(sub_pcie_pa_addr, PCIE_SUB_BASE_SIZE);
		printk("could not ioremap SUB PCIE registers\n");

		return ERROR;
	}

	/* add by chenqilin, for slave cpu of arm server */
	if (max_cpu_nodes > 1) {

		sub_pcie_pa_addr_slavecpu = PCIE_SUB_BASE_SLAVE_CPU;

		/* SUB ALG申请io内存 */
		/*    if (!request_mem_region(sub_pcie_pa_addr_slavecpu, PCIE_SUB_BASE_SIZE, "SLAVE SUB PCIE Reg"))
		   {
		   printk("SLAVE CPU SUB PCIE region busy\n");
		   return ERROR;
		   }
		 */
		sub_pcie_base_addr_slavecpu =
		    (unsigned long long)ioremap(sub_pcie_pa_addr_slavecpu,
						PCIE_SUB_BASE_SIZE);
		if (!sub_pcie_base_addr_slavecpu) {
			//release_mem_region(sub_pcie_pa_addr_slavecpu, PCIE_SUB_BASE_SIZE);
			printk
			    ("could not ioremap SLAVE CPU SUB PCIE registers\n");

			return ERROR;
		}
	}
	/* end */
	return OK;
}

/*****************************************************************************
 函 数 名  : sub_pcie_exit
 功能描述  : SUB PCIE CRG 退出
 输入参数  : void
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年11月21日
    作    者   : z00176027
    修改内容   : 新生成函数

*****************************************************************************/
void hilink_sub_pcie_exit(void)
{
	/* 释放POU资源 */
	if (sub_pcie_base_addr > 0)
		//release_mem_region(sub_pcie_pa_addr, PCIE_SUB_BASE_SIZE);
		iounmap((void *)sub_pcie_base_addr);

	if (sub_pcie_base_addr_slavecpu > 0)
		//release_mem_region(sub_pcie_pa_addr_slavecpu, PCIE_SUB_BASE_SIZE);
		iounmap((void *)sub_pcie_base_addr_slavecpu);
}

static unsigned long long comm_sub_pcie_get_base(unsigned int node)
{
	/* 主片node=0 */
	if (node == MASTER_CPU_NODE)
		return sub_pcie_base_addr;
	else
		return sub_pcie_base_addr_slavecpu;
}

static inline unsigned int system_reg_read(unsigned int node,
					   unsigned long long reg_pa)
{
	volatile unsigned int temp;
	unsigned long long addr = 0;
	unsigned long long addr_reg = 0;

	if ((reg_pa >= HILINKMACRO0)
	    && (reg_pa < HILINKMACRO0 + HILINK_REG_SIZE)) {
		addr =
		    ((node ==
		      MASTER_CPU_NODE) ? macro0_virt_base :
		     slave_macro0_virt_base);
		addr_reg = HILINKMACRO0;
	} else if ((reg_pa >= HILINKMACRO1)
		   && (reg_pa < HILINKMACRO1 + HILINK_REG_SIZE)) {
		addr =
		    ((node ==
		      MASTER_CPU_NODE) ? macro1_virt_base :
		     slave_macro1_virt_base);
		addr_reg = HILINKMACRO1;
	} else if ((reg_pa >= HILINKMACRO2)
		   && (reg_pa < HILINKMACRO2 + HILINK_REG_SIZE)) {
		addr =
		    ((node ==
		      MASTER_CPU_NODE) ? macro2_virt_base :
		     slave_macro2_virt_base);
		addr_reg = HILINKMACRO2;
	} else if ((reg_pa >= HILINKMACRO3)
		   && (reg_pa < HILINKMACRO3 + HILINK_REG_SIZE)) {
		addr =
		    ((node ==
		      MASTER_CPU_NODE) ? macro3_virt_base :
		     slave_macro3_virt_base);
		addr_reg = HILINKMACRO3;
	} else if ((reg_pa >= HILINKMACRO4)
		   && (reg_pa < HILINKMACRO4 + HILINK_REG_SIZE)) {
		addr =
		    ((node ==
		      MASTER_CPU_NODE) ? macro4_virt_base :
		     slave_macro4_virt_base);
		addr_reg = HILINKMACRO4;
	} else if ((reg_pa >= HILINKMACRO5)
		   && (reg_pa < HILINKMACRO5 + HILINK_REG_SIZE)) {
		addr =
		    ((node ==
		      MASTER_CPU_NODE) ? macro5_virt_base :
		     slave_macro5_virt_base);
		addr_reg = HILINKMACRO5;
	} else if ((reg_pa >= HILINKMACRO6)
		   && (reg_pa < HILINKMACRO6 + HILINK_REG_SIZE)) {
		addr_reg = HILINKMACRO6;
		addr =
		    ((node ==
		      MASTER_CPU_NODE) ? macro6_virt_base :
		     slave_macro6_virt_base);
	} else if ((reg_pa >= SRE_SAS0_DSAF_CFG_BASE)
		   && (reg_pa < SRE_SAS0_DSAF_CFG_BASE + DSAF_SUB_BASE_SIZE)) {
		addr_reg = SRE_SAS0_DSAF_CFG_BASE;
		addr = comm_sub_dsaf_get_base(node);
	} else if ((reg_pa >= SRE_SAS1_PCIE_CFG_BASE)
		   && (reg_pa < SRE_SAS1_PCIE_CFG_BASE + PCIE_SUB_BASE_SIZE)) {
		addr_reg = SRE_SAS1_PCIE_CFG_BASE;
		addr = comm_sub_pcie_get_base(node);
	} else {
		printk("reg_pa:0x%llx\n", reg_pa);
	}
	//printk("node:%d, reg_pa:0x%llx, addr:0x%llx, addr_reg:0x%llx\n",node, reg_pa, addr, addr_reg);

	temp = ioread32((void __iomem *)(addr + (reg_pa - addr_reg)));

	return temp;
}

static inline void system_reg_write(unsigned int node,
				    unsigned long long reg_pa, unsigned int val)
{
	//volatile unsigned int temp;
	unsigned long long addr = 0;
	unsigned long long addr_reg = 0;

	if ((reg_pa >= HILINKMACRO0)
	    && (reg_pa < HILINKMACRO0 + HILINK_REG_SIZE)) {
		addr =
		    ((node ==
		      MASTER_CPU_NODE) ? macro0_virt_base :
		     slave_macro0_virt_base);
		addr_reg = HILINKMACRO0;
	} else if ((reg_pa >= HILINKMACRO1)
		   && (reg_pa < HILINKMACRO1 + HILINK_REG_SIZE)) {
		addr =
		    ((node ==
		      MASTER_CPU_NODE) ? macro1_virt_base :
		     slave_macro1_virt_base);
		addr_reg = HILINKMACRO1;
	} else if ((reg_pa >= HILINKMACRO2)
		   && (reg_pa < HILINKMACRO2 + HILINK_REG_SIZE)) {
		addr =
		    ((node ==
		      MASTER_CPU_NODE) ? macro2_virt_base :
		     slave_macro2_virt_base);
		addr_reg = HILINKMACRO2;
	} else if ((reg_pa >= HILINKMACRO3)
		   && (reg_pa < HILINKMACRO3 + HILINK_REG_SIZE)) {
		addr =
		    ((node ==
		      MASTER_CPU_NODE) ? macro3_virt_base :
		     slave_macro3_virt_base);
		addr_reg = HILINKMACRO3;
	} else if ((reg_pa >= HILINKMACRO4)
		   && (reg_pa < HILINKMACRO4 + HILINK_REG_SIZE)) {
		addr =
		    ((node ==
		      MASTER_CPU_NODE) ? macro4_virt_base :
		     slave_macro4_virt_base);
		addr_reg = HILINKMACRO4;
	} else if ((reg_pa >= HILINKMACRO5)
		   && (reg_pa < HILINKMACRO5 + HILINK_REG_SIZE)) {
		addr =
		    ((node ==
		      MASTER_CPU_NODE) ? macro5_virt_base :
		     slave_macro5_virt_base);
		addr_reg = HILINKMACRO5;
	} else if ((reg_pa >= HILINKMACRO6)
		   && (reg_pa < HILINKMACRO6 + HILINK_REG_SIZE)) {
		addr_reg = HILINKMACRO6;
		addr =
		    ((node ==
		      MASTER_CPU_NODE) ? macro6_virt_base :
		     slave_macro6_virt_base);
	} else if ((reg_pa >= SRE_SAS0_DSAF_CFG_BASE)
		   && (reg_pa < SRE_SAS0_DSAF_CFG_BASE + DSAF_SUB_BASE_SIZE)) {
		addr_reg = SRE_SAS0_DSAF_CFG_BASE;
		addr = comm_sub_dsaf_get_base(node);
	} else if ((reg_pa >= SRE_SAS1_PCIE_CFG_BASE)
		   && (reg_pa < SRE_SAS1_PCIE_CFG_BASE + PCIE_SUB_BASE_SIZE)) {
		addr_reg = SRE_SAS1_PCIE_CFG_BASE;
		addr = comm_sub_pcie_get_base(node);
	} else {
		printk("reg_pa:0x%llx\n", reg_pa);
	}

	//printk("node:%d, reg_pa:0x%llx, addr:0x%llx, addr_reg:0x%llx, val:0x%x\n",node, reg_pa, addr, addr_reg, val);

	iowrite32(val, (void __iomem *)(addr + (reg_pa - addr_reg)));

	//return temp;
}

/*************************************************************
Prototype    : sds_reg_read
Description  :露脕serdes脛脷虏驴录脛麓忙脝梅
Input        :      unsigned int macro_id
			        unsigned int reg_offset
Output       : 脦脼
Return Value :   unsigned int 录脛麓忙脝梅脰碌
Calls        :
Called By    :

History        :
1.Date         : 2013/10/15
 Author       :  w00244733
 Modification :  Created function

***********************************************************/
unsigned int sds_reg_read(unsigned int node, unsigned int macro_id,
			  unsigned int reg_offset)
{
	unsigned int reg_val = 0;
	unsigned long long reg_addr = 0;

	if (macro_id > MACRO_6) {
		printk("sds_reg_read]:MacroId %d error\n", macro_id);
		reg_addr = HILINKMACRO6 + reg_offset;
	}

	if (macro_id == 0)
		reg_addr = HILINKMACRO0 + reg_offset;
	if (macro_id == 1)
		reg_addr = HILINKMACRO1 + reg_offset;
	if (macro_id == 2)
		reg_addr = HILINKMACRO2 + reg_offset;
	if (macro_id == 3)
		reg_addr = HILINKMACRO3 + reg_offset;
	if (macro_id == 4)
		reg_addr = HILINKMACRO4 + reg_offset;
	if (macro_id == 5)
		reg_addr = HILINKMACRO5 + reg_offset;
	if (macro_id == 6)
		reg_addr = HILINKMACRO6 + reg_offset;

	//脪脝脰虏脨猫脪陋脤脴卤冒脳垄脪芒
	reg_val = system_reg_read(node, reg_addr);
	return reg_val;
}

static void sds_reg_write(unsigned int node, unsigned int macro_id,
			  unsigned int reg_offset, unsigned int reg_val)
{
	unsigned long long reg_addr = 0;

	if (macro_id > MACRO_6) {
		printk("sds_reg_write:MacroId %d error\n", macro_id);
		reg_addr = HILINKMACRO6 + reg_offset;
	}

	if (macro_id == 0)
		reg_addr = HILINKMACRO0 + reg_offset;
	if (macro_id == 1)
		reg_addr = HILINKMACRO1 + reg_offset;
	if (macro_id == 2)
		reg_addr = HILINKMACRO2 + reg_offset;
	if (macro_id == 3)
		reg_addr = HILINKMACRO3 + reg_offset;
	if (macro_id == 4)
		reg_addr = HILINKMACRO4 + reg_offset;
	if (macro_id == 5)
		reg_addr = HILINKMACRO5 + reg_offset;
	if (macro_id == 6)
		reg_addr = HILINKMACRO6 + reg_offset;

	//脪脝脰虏脨猫脪陋脤脴卤冒脳垄脪芒
	system_reg_write(node, reg_addr, reg_val);
	return;
}

/*************************************************************
Prototype    : sds_reg_bits_write
Description  :掳麓脦禄脨麓serdes脛脷虏驴录脛麓忙脝梅
Input        :      unsigned int   macro_id
                      unsigned int    reg_offset    录脛麓忙脝梅脝芦脪脝碌脴脰路
                       unsigned int   high_bit
                      unsigned int    low_bit
                      unsigned int    reg_val 脨麓脠毛脰碌
Output       : 脦脼
Return Value :
Calls        :
Called By    :

History        :
1.Date         : 2013/10/10
 Author       : w00244733
 Modification : adapt to hi1381

***********************************************************/
//Serdes 录脛麓忙脝梅掳麓脦禄脨麓
void sds_reg_bits_write(unsigned int node, unsigned int macro_id,
			unsigned int reg_offset, unsigned int high_bit,
			unsigned int low_bit, unsigned int reg_val)
{
	unsigned int reg_cfg_max_val;
	unsigned int orign_reg_val;
	unsigned int final_val;
	unsigned int mask;
	unsigned int add;

	if (macro_id > MACRO_6) {
		printk
		    ("sds_reg_bits_write]:Macro %d RegAddr %d [%d:%d]  Value 0x%x (Macro ID is too big!)\r\n",
		     macro_id, reg_offset, high_bit, low_bit, reg_val);
		return;
	}

	if (high_bit < low_bit) {
		high_bit ^= low_bit;
		low_bit ^= high_bit;
		high_bit ^= low_bit;
	}
	reg_cfg_max_val = (0x1 << (high_bit - low_bit + 1)) - 1;
	if (reg_val > reg_cfg_max_val) {
		printk
		    ("[sds_reg_bits_write]:Macro%d RegAddr 0x%x [%d:%d]  Value 0x%x is too big!\r\n",
		     macro_id, reg_offset, high_bit, low_bit, reg_val);
		return;
	}

	orign_reg_val = sds_reg_read(node, macro_id, reg_offset);
	mask = (~(reg_cfg_max_val << low_bit)) & 0xffff;
	orign_reg_val &= mask;
	add = reg_val << low_bit;
	final_val = orign_reg_val + add;
	sds_reg_write(node, macro_id, reg_offset, final_val);

	return;
}

/*************************************************************
Prototype    : sds_reg_bits_read
Description  :掳麓脦禄露脕serdes脛脷虏驴录脛麓忙脝梅
Input        :      unsigned int   macro_id
                      unsigned int    reg_offset    录脛麓忙脝梅脝芦脪脝碌脴脰路
                       unsigned int   high_bit
                      unsigned int    low_bit

Output       : 脦脼
Return Value :
Calls        :
Called By    :

History        :
1.Date         : 2013/10/10
 Author       : w00244733
 Modification : adapt to hi1381

***********************************************************/
unsigned int sds_reg_bits_read(unsigned int node, unsigned int macro_id,
			       unsigned int reg_offset, unsigned int high_bit,
			       unsigned int low_bit)
{
	unsigned int orign_val;
	unsigned int mask;
	unsigned int final;

	//路脜脭脷脳卯脟掳脙忙
	if (macro_id > MACRO_6) {
		printk
		    ("[sds_reg_bits_read]:Macro %d RegAddr 0x%x (Macro ID is too big!)\r\n",
		     macro_id, reg_offset);
	}

	if (high_bit < low_bit) {
		high_bit ^= low_bit;
		low_bit ^= high_bit;
		high_bit ^= low_bit;
	}
	orign_val = sds_reg_read(node, macro_id, reg_offset);
	orign_val >>= low_bit;
	mask = (0x1 << (high_bit - low_bit + 1)) - 1;
	final = (orign_val & mask);
	return final;
}

unsigned int serdes_check_param(unsigned int macro, unsigned int lane)
{
	switch (macro) {
	case MACRO_0:
	case MACRO_1:
	case MACRO_2:
		if (lane > 7) {
			printk("macro:%u, lane:%u is invalid\n", macro, lane);
			return EM_SERDES_FAIL;
		}
		break;

	case MACRO_3:
	case MACRO_4:
	case MACRO_5:
	case MACRO_6:
		if (lane > 3) {
			printk("macro:%u, lane:%u is invalid\n", macro, lane);
			return EM_SERDES_FAIL;
		}
		break;

	default:
		printk("macro is invalid:%u\n", macro);
		return EM_SERDES_FAIL;
	}

	return EM_SERDES_SUCCESS;
}

/*************************************************************
Prototype    : serdes_cs_cfg
Description  :serdes CS  虏脦脢媒脜盲脰脙拢卢虏禄脥卢脛拢脢陆潞脥脧脽脣脵脗脢虏脦脢媒脜盲脰脙虏禄脥卢
Input        :
                       unsigned int macro_id  Macro ID:     Macro0/Macro1/Macro2
                       unsigned int cs_num    Cs Number: CS0/CS1
                       unsigned int ulCsCfg   露脭脫娄虏禄脥卢脛拢脢陆潞脥脧脽脣脵脗脢碌脛CS脜盲脰脙脙露戮脵脕驴
Output       : 脦脼
Return Value :
Calls        :
Called By    :

History        :
1.Date         : 2013/10/11
 Author       : w00244733
 Modification : adapt to hi1381

***********************************************************/
void serdes_cs_cfg(unsigned int node, unsigned int macro_id,
		   unsigned int cs_num, unsigned int cs_cfg)
{
	// Select correct clock source for Serviceslice (if this macro contains Serviceslice)
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 0), 14, 14, 0);
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 0), 13, 13, 0);
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 0), 12, 12, 0);

	// Select correct clock source for Clockslice
	if ((GE_1250 == cs_cfg) || (GE_3125 == cs_cfg) || (XGE_10312 == cs_cfg))
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 0), 11, 11,
				   0);
	else
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 0), 11, 11,
				   1);

	//sds_reg_bits_write(node, macro_id, CS_CSR(cs_num,0),10,10, 1);
	//麓脣脰碌脦陋1脪陋脭脷Firmware脰庐脟掳碌梅脫脙,DS脨拢脳录脰庐潞贸拢卢(8G脪脭脡脧脫脙Firmware,XGE虏禄脫脙)
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 0), 10, 10, 0);
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 0), 9, 9, 0);	//The highest freq is 400M.

	// Set Clockslice divider ratio
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 15, 15, 0);	// W divider setting

	switch (cs_cfg) {
	case GE_1250:
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 14, 13,
				   KA2_KB2);
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 12, 8,
				   0xA);
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 7, 4,
				   0x9);
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 1, 0, 0);
		break;
	case GE_3125:
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 14, 13,
				   KA3_KB1);
		//P虏脦脢媒S虏脦脢媒   N=2*P+S+3
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 12, 8, 0x6);	//P
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 7, 4, 0x5);	//S
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 1, 0,
				   0x1);
		break;
	case XGE_10312:
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 14, 13,
				   KA2_KB1);
		//P虏脦脢媒S虏脦脢媒   N=2*P+S+3
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 12, 8, 0xA);	//P
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 7, 4, 0xA);	//S
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 1, 0,
				   0x3);
		break;
	case PCIE_2500:
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 14, 13,
				   KA2_KB2);
		//P虏脦脢媒S虏脦脢媒   N=2*P+S+3
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 12, 8, 0x10);	//P
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 7, 4, 0xF);	//S
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 1, 0, 0);
		break;
	case PCIE_5000:
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 14, 13,
				   KA2_KB2);
		//P虏脦脢媒S虏脦脢媒   N=2*P+S+3
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 12, 8, 0x10);	//P
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 7, 4, 0xF);	//S
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 1, 0, 0);
		break;
	case PCIE_8000:
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 14, 13,
				   KA3_KB1);
		//P虏脦脢媒S虏脦脢媒   N=2*P+S+3
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 12, 8, 0xd);	//P
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 7, 4, 0xb);	//S
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 1, 0,
				   0x3);
		break;
	case SAS_1500:
		/*2014_12_3 modify for sas DTS2014121600518 */
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 14, 13, KA3_KB1);	//CMLDIV
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 12, 8, 0x9);	//P
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 7, 4, 0x9);	//S
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 1, 0, 1);	//WORDCLKDIV
		break;
	case SAS_3000:
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 14, 13,
				   KA2_KB2);
		//P虏脦脢媒S虏脦脢媒   N=2*P+S+3
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 12, 8, 0x15);	//P
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 7, 4, 0xF);	//S
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 1, 0, 0);
		break;
	case SAS_6000:
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 14, 13,
				   KA2_KB2);
		//P虏脦脢媒S虏脦脢媒   N=2*P+S+3
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 12, 8, 0x15);	//P
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 7, 4, 0xF);	//S
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 1, 0, 0);
		break;
	case SAS_12000:
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 14, 13,
				   KA2_KB1);
		//P虏脦脢媒S虏脦脢媒   N=2*P+S+3
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 12, 8, 0x15);	//P
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 7, 4, 0xF);	//S
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 1, 0,
				   0x1);
		break;
	case HCCS_32:
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 14, 13,
				   KA2_KB1);
		//P虏脦脢媒S虏脦脢媒   N=2*P+S+3
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 12, 8, 0x15);	//P
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 7, 4, 0xF);	//S
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 1, 0,
				   0x3);
		break;
	case HCCS_40:
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 14, 13,
				   KA2_KB1);
		//P虏脦脢媒S虏脦脢媒   N=2*P+S+3
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 12, 8, 0x15);	//P
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 7, 4, 0xF);	//S
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 1), 1, 0,
				   0x1);
		break;
	default:
		break;
	}

	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 2), 15, 14, 0);
	if ((cs_num == CS0) && ((cs_cfg == SAS_1500) || (cs_cfg == PCIE_2500)))
		//0x0赂酶MCU脜脺FirmWare脤谩鹿漏赂眉驴矛碌脛脢卤脰脫拢卢脤谩脡媒脨脭脛脺,DTS2014121600518 
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 2), 13, 12,
				   0x0);

	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 44), 6, 6, 0);	// V divider setting
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 45), 7, 7, 0);

	/*
	   1.HiLink15BP Programming User Guide DraftV1.11.pdf modify
	   2.露炉脤卢驴陋脝么SSC
	 */
	if (use_ssc) {
		// SSC Mode of Clock Slice ONLY prepares for the project using SSC Function    SAS脳篓脫脙
		if ((SAS_1500 == cs_cfg) || (SAS_3000 == cs_cfg)
		    || (SAS_6000 == cs_cfg)
		    || (SAS_12000 == cs_cfg)) {
			sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 56),
					   8, 0, 0x2e);
			sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 60),
					   14, 12, 0x1);
			sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 57),
					   6, 0, 0x23);
			sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 58),
					   15, 14, 0x1);
			//sds_reg_bits_write(node, macro_id, CS_CSR(cs_num,57),7,7,0x1);
			sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 58), 13, 13, 0x0);	//0 :auto mode; 1:manual mode
			sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 58), 12, 12, 0x1);	//decrease the intg/prop cp current
			//sds_reg_bits_write(node, macro_id, CS_CSR(cs_num,60),8,8,0x0);     //power on the ssc PI module
			//CPCURRENT 2014/10/29 modify
			sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 4), 9, 4, 0x2);	//decrease the prop chargepump current

			sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 57),
					   7, 7, 0x1);
		}
	} else {
		sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 58), 9, 9,
				   0x1);
	}

	return;
}

void cs_hw_calibration_option_v2_init(unsigned int node, unsigned int macro_id,
				      unsigned int cs_num)
{
	// Set charge pump related register
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 44), 4, 0, 0x5);
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 4), 12, 10, 0x0);	//CPCURRENTINT
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 11), 7, 0, 0x10);	//VREGPLLLVL

	//if it is a low frequency clock slice
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 30), 11, 7, 0xe);
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 28), 9, 5, 0xe);

	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 28), 4, 0, 0x3);

	//   Set peak level for amplitude calibration
	sds_reg_write(node, macro_id, CS_CSR(cs_num, 46), 0x22c0);	//CS_DEBUGGING_CTRL1

	// Set freq detector threshold to minimum value
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 35), 10, 1, 0x0);

	//   Active low DLF common mode active common mode circuitry power down
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 5), 8, 8, 0x1);

	// Set the LCVCO Temperature compensation ON
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 58), 10, 10, 0x1);

	// Reset clock slice calibration bit
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 20), 2, 2, 0x0);	//CS_CALIB_SOFT_RST_N
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 20), 2, 2, 0x1);	//CS_CALIB_SOFT_RST_N

	return;
}

unsigned int cs_hw_calibration_option_v2_exec(unsigned int node,
					      unsigned int macro_id,
					      unsigned int cs_num,
					      unsigned int time)
{
	unsigned int tmp_time;
	unsigned int ret = SRE_OK;

	//   Set calibration mode to 0 for auto calibration
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 13), 15, 15, 0x0);	//LCVCOCALMODE

	//   Active clock slice auto calibration by rising edge of CS_CALIB_START
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 20), 8, 8, 0x0);	//CS_CALIB_START
	sds_reg_bits_write(node, macro_id, CS_CSR(cs_num, 20), 8, 8, 0x1);	//CS_CALIB_START

	//    Check whether calibration successfully done
	tmp_time = 0;
	while (tmp_time <= CS_CALIB_TIME) {

		serdes_delay_us(1000);
		if (0x1 == sds_reg_bits_read(node, macro_id, CS_CSR(cs_num, 13), 4, 4)) {	//LCVCOCALDONE

			if (time == 0)
				printk
				    ("[cs_hw_calibration_option_v2_exec]:Macro%d CS%d LC Vco Cal done!(LCVCOCALDONE) in %dms\r\n",
				     macro_id, cs_num, tmp_time);

			if (time > 0)
				printk
				    ("[cs_hw_calibration_option_v2_exec]:Macro%d CS%d LC Vco Re-Cal done!(LCVCOCALDONE %d) in %dms\r\n",
				     macro_id, cs_num, time, tmp_time);

			break;
		}
		tmp_time++;
	}

	if (tmp_time > CS_CALIB_TIME) {
		ret = SRE_HILINK_CS_CALIB_ERR;
		if (time == 0)
			printk
			    ("[cs_hw_calibration_option_v2_exec]:Macro%d CS%d LC Vco Cal fail!(LCVCOCALDONE)\r\n",
			     macro_id, cs_num);

		if (time > 0)
			printk
			    ("[cs_hw_calibration_option_v2_exec]:Macro%d CS%d LC Vco Re-Cal fail!(LCVCOCALDONE %d)\r\n",
			     macro_id, cs_num, time);
	}

	return ret;

}

unsigned int serdes_cs_hw_calibration_option_v2(unsigned int node,
						unsigned int macro_id,
						unsigned int cs_num)
{
	unsigned int result = SRE_OK;
	unsigned int ret = SRE_OK;
	unsigned int tmp_time = 0;

	// First do initialization before calibration
	cs_hw_calibration_option_v2_init(node, macro_id, cs_num);

	//step2 : Execution function of Cs hardware calibration
	result = cs_hw_calibration_option_v2_exec(node, macro_id, cs_num, 0);
	if (result == SRE_HILINK_CS_CALIB_ERR) {
		ret = SRE_HILINK_CS_CALIB_ERR;
		return ret;
	}
	//录矛虏茅PLL Lock脳麓脤卢
	tmp_time = 0;
	while (tmp_time <= 10) {

		serdes_delay_us(1000);
		if (0x0 == sds_reg_bits_read(node, macro_id, CS_CSR(cs_num, 2), 1, 1)) {	//CS PLL LOCK

			printk
			    ("[serdes_cs_calib]:Macro%d CS%d PLL lock success!(%d ms)\r\n",
			     macro_id, cs_num, tmp_time);
			break;
		}
		tmp_time++;
	}
	if (tmp_time > 10) {
		printk("[serdes_cs_calib]:Macro%d CS%d PLL out of lock!\r\n",
		       macro_id, cs_num);
	}

	return ret;
}

unsigned int serdes_cs_calib(unsigned int node, unsigned int macro_id,
			     unsigned int cs_num)
{
	unsigned int ret;
	unsigned int i = 0;

	for (i = 0; i < 10; i++) {
		ret =
		    serdes_cs_hw_calibration_option_v2(node, macro_id, cs_num);
		if (ret == SRE_OK)
			break;
	}

	if (i >= 10)
		ret = SRE_HILINK_CS_CALIB_ERR;

	return ret;
}

/*************************************************************
Prototype    : serdes_ds_cfg
Description  :serdes DS  虏脦脢媒脜盲脰脙拢卢虏禄脥卢脛拢脢陆潞脥脧脽脣脵脗脢虏脦脢媒脜盲脰脙虏禄脥卢
Input        :
                       unsigned int macro_id  Macro ID:     Macro0/Macro1/Macro2
                       unsigned int ds_num    Ds Number
                       DS_CFG_ENUM ulCsCfg   露脭脫娄虏禄脥卢脛拢脢陆潞脥脧脽脣脵脗脢碌脛DS 脜盲脰脙脙露戮脵脕驴
                       unsigned int cs_src    脩隆脭帽CS0禄貌脮脽CS1
Output       : 脦脼
Return Value :
Calls        :
Called By    :

History        :
1.Date         : 2013/10/11
 Author       : w00244733
 Modification : adapt to hi1381

***********************************************************/
void serdes_ds_cfg(unsigned int node, unsigned int macro_id,
		   unsigned int ds_num, unsigned int ds_cfg,
		   unsigned int cs_src)
{
	unsigned int reg_val;
	unsigned long long reg_addr = 0;
	unsigned int rate_mode = 0;

	if (macro_id == 0)
		reg_addr = SRE_HILINK0_MACRO_LRSTB_REG;

	if (macro_id == 1)
		reg_addr = SRE_HILINK1_MACRO_LRSTB_REG;

	if (macro_id == 2)
		reg_addr = SRE_HILINK2_MACRO_LRSTB_REG;

	if (macro_id == 3)
		reg_addr = SRE_HILINK3_MACRO_LRSTB_REG;

	if (macro_id == 4)
		reg_addr = SRE_HILINK4_MACRO_LRSTB_REG;

	if (macro_id == 5)
		reg_addr = SRE_HILINK5_MACRO_LRSTB_REG;

	if (macro_id == 6)
		reg_addr = SRE_HILINK6_MACRO_LRSTB_REG;

	//release lane reset
	reg_val = system_reg_read(node, reg_addr);
	reg_val |= (0x1 << ds_num);
	system_reg_write(node, reg_addr, reg_val);
	serdes_delay_us(10);
	/*
	   1.HiLink15BP Programming User Guide DraftV1.11.pdf modify 碌梅脮没脣鲁脨貌
	 */
	// Power up DSCLK
	sds_reg_bits_write(node, macro_id, DSCLK_CSR(ds_num, 1), 10, 10, 0x1);
	// Power up Tx
	sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 0), 15, 15, 0x1);
	// Power up Rx
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 0), 15, 15, 0x1);
	serdes_delay_us(100);
	//disable remote EyeMetric
	//sds_reg_bits_write(node, macro_id, RX_CSR(ds_num,61),13,13,0);
	// Dsclk configuration start
	sds_reg_bits_write(node, macro_id, DSCLK_CSR(ds_num, 0), 14, 14,
			   cs_src);

	// Choose whether bypass Tx phase interpolator
	sds_reg_bits_write(node, macro_id, DSCLK_CSR(ds_num, 0), 10, 10, 0);

	// Select Tx spine clock source
	sds_reg_bits_write(node, macro_id, DSCLK_CSR(ds_num, 0), 13, 13,
			   cs_src);

	/*
	   1.HiLink15BP Programming User Guide DraftV1.11.pdf modify
	 */
	//sds_reg_bits_write(node, macro_id, DSCLK_CSR(ds_num,1),9,9,0);

	/*DTS2014091107380 */
	serdes_delay_us(200);

	if (sds_tx_polarity[node][macro_id][ds_num] == 1)
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 8, 8, 1);

#if 0
	if (ebord_type == EM_16CORE_EVB_BOARD) {
		// Set Tx output polarity. Not inverting data is default. For Phosphor660 16 core EVB Board
		if (((3 == macro_id) && (3 == ds_num))
		    || ((4 == macro_id) && ((0 == ds_num) || (2 == ds_num)))) {
			sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 8,
					   8, 1);
		}
	} else if (ebord_type == EM_32CORE_EVB_BOARD) {
		// Set Tx output polarity. Not inverting data is default. For Phosphor660 32core EVB Board
		if (((3 == macro_id) && (2 == ds_num))
		    || ((4 == macro_id)
			&& ((1 == ds_num) || (2 == ds_num) || (3 == ds_num)))) {
			sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 8,
					   8, 1);
		}
	} else if (ebord_type == EM_V2R1CO5_BORAD) {
		// Set Tx output polarity. Not inverting data is default. For Phosphor660 32core EVB Board
		if ((1 == macro_id) && ((7 == ds_num) || (0 == ds_num))) {
			sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 8,
					   8, 1);
		}
	} else {
		//do nothing
	}
#endif

	// Set Tx clock and data source
	sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 7, 6, 0x0);
	sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 5, 4, 0x0);

	// Set Tx align window dead band and Tx align mode
	sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 3, 3, 0x0);
	sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 2, 0, 0x0);

	switch (ds_cfg) {
	case GE_1250:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 11, 10, QUARTER_RATE);	//Rx脣脵脗脢
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 14, 12, WIDTH_40BIT);	//Rx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 11, 10, QUARTER_RATE);	//Tx脣脵脗脢
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 14, 12, WIDTH_40BIT);	//Tx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 15, 10,
				   0x3f);
		rate_mode = QUARTER_RATE;
		break;
	case GE_3125:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 11, 10, HALF_RATE);	//Rx脣脵脗脢
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 14, 12, WIDTH_20BIT);	//Rx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 11, 10, HALF_RATE);	//Tx脣脵脗脢
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 14, 12, WIDTH_20BIT);	//Tx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 15, 10,
				   0x3f);
		rate_mode = HALF_RATE;
		break;
	case XGE_10312:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 11, 10, FULL_RATE);	//Rx脣脵脗脢
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 14, 12, WIDTH_32BIT);	//Rx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 11, 10, FULL_RATE);	//Tx脣脵脗脢
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 14, 12, WIDTH_32BIT);	//Tx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 15, 10,
				   0x3f);
		rate_mode = FULL_RATE;
		break;
	case PCIE_2500:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 11, 10, HALF_RATE);	//Rx脣脵脗脢
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 14, 12, WIDTH_40BIT);	//Rx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 11, 10, HALF_RATE);	//Tx脣脵脗脢
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 14, 12, WIDTH_40BIT);	//Tx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 15, 10,
				   0x34);
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 9, 5,
				   0x1b);
		rate_mode = HALF_RATE;
		break;
	case PCIE_5000:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 11, 10, FULL_RATE);	//Rx脣脵脗脢
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 14, 12, WIDTH_20BIT);	//Rx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 11, 10, FULL_RATE);	//Tx脣脵脗脢
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 14, 12, WIDTH_20BIT);	//Tx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 15, 10,
				   0x34);
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 9, 5,
				   0x1b);
		rate_mode = FULL_RATE;
		break;
	case PCIE_8000:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 11, 10, FULL_RATE);	//Rx脣脵脗脢
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 14, 12, WIDTH_16BIT);	//Rx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 11, 10, FULL_RATE);	//Tx脣脵脗脢
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 14, 12, WIDTH_16BIT);	//Tx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 15, 10,
				   0x34);
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 9, 5,
				   0x1b);
		rate_mode = FULL_RATE;
		break;
	case SAS_1500:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 11, 10, QUARTER_RATE);	//Rx脣脵脗脢
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 14, 12, WIDTH_40BIT);	//Rx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 11, 10, QUARTER_RATE);	//Tx脣脵脗脢
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 14, 12, WIDTH_40BIT);	//Tx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 15, 10,
				   0x37);
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 9, 5,
				   0x18);
		rate_mode = QUARTER_RATE;
		break;
	case SAS_3000:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 11, 10, HALF_RATE);	//Rx脣脵脗脢
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 14, 12, WIDTH_16BIT);	//Rx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 11, 10, HALF_RATE);	//Tx脣脵脗脢
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 14, 12, WIDTH_16BIT);	//Tx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 15, 10,
				   0x37);
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 9, 5,
				   0x18);
		rate_mode = HALF_RATE;
		break;
	case SAS_6000:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 11, 10, FULL_RATE);	//Rx脣脵脗脢
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 14, 12, WIDTH_16BIT);	//Rx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 11, 10, FULL_RATE);	//Tx脣脵脗脢
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 14, 12, WIDTH_16BIT);	//Tx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 15, 10,
				   0x37);
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 9, 5,
				   0x18);
		rate_mode = FULL_RATE;
		break;
	case SAS_12000:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 11, 10, FULL_RATE);	//Rx脣脵脗脢
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 14, 12, WIDTH_16BIT);	//Rx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 11, 10, FULL_RATE);	//Tx脣脵脗脢
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 14, 12, WIDTH_16BIT);	//Tx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 15, 10,
				   0x37);
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 9, 5,
				   0x18);
		rate_mode = FULL_RATE;
		break;
	case HCCS_32:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 11, 10, FULL_RATE);	//Rx脣脵脗脢
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 14, 12, WIDTH_32BIT);	//Rx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 11, 10, FULL_RATE);	//Tx脣脵脗脢
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 14, 12, WIDTH_32BIT);	//Tx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 15, 10,
				   0x3f);
		rate_mode = FULL_RATE;
		break;
	case HCCS_40:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 11, 10, FULL_RATE);	//Rx脣脵脗脢
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 14, 12, WIDTH_40BIT);	//Rx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 11, 10, FULL_RATE);	//Tx脣脵脗脢
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 2), 14, 12, WIDTH_40BIT);	//Tx脦禄驴铆
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 15, 10,
				   0x3f);
		rate_mode = FULL_RATE;
		break;
	default:
		break;
	}

	// Set Tx FIR coefficients //脣霉脫脨脛拢脢陆脜盲脰脙露录脧脿脥卢拢卢鹿脢虏禄脭脵脟酶路脰
	sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 0), 9, 5, 0);
	sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 0), 4, 0, 0);
	sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 1), 4, 0, 0);

	// Set Tx tap pwrdnb according to settings of pre2/post2 tap setting
	sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 10), 7, 0, 0xf6);

	// Set Tx amplitude to 3
	sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 11), 2, 0, 0x3);

	// TX termination calibration target resist value choice
	sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 34), 1, 0, 0x2);

	if (ds_cfg == PCIE_2500 || ds_cfg == PCIE_5000 || ds_cfg == PCIE_8000)
		//select deemph from register  txdrv_map_sel,PCIE脪碌脦帽脨猫脪陋录脫碌脛
		sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 34), 4, 4,
				   0x1);

	// Set center phase offset according to rate mode
	if (FULL_RATE == rate_mode)
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 12), 15, 8,
				   0x20);
	else
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 12), 15, 8,
				   0x0);

	// Rx termination calibration target resist value choice. 0-50Ohms
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 31), 5, 4, 0x1);

	if (ds_cfg == PCIE_2500 || ds_cfg == SAS_1500 || ds_cfg == SAS_3000
	    || ds_cfg == SAS_6000 || ds_cfg == GE_1250 || ds_cfg == GE_3125)
		/*pugv1.11 脡脧脠卤脡脵录脛麓忙脝梅 ,xls 脫脨rxctlerefsel */
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 39), 5, 3,
				   0x4);

	if (sds_rx_polarity[node][macro_id][ds_num] == 1)
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 8, 8,
				   0x1);

#if 0
	if (ebord_type == EM_16CORE_EVB_BOARD) {
		// Set Rx data polarity. Not inverting data is default. For Phosphor660 16 core EVB Board
		if ((4 == macro_id)
		    && ((0 == ds_num) || (1 == ds_num) || (2 == ds_num))) {
			/*DTS2014092505690 l00290354 */
			sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10),
					   8, 8, 0x1);
		}
	} else if (ebord_type == EM_32CORE_EVB_BOARD) {
		// Set Rx data polarity. Not inverting data is default. For Phosphor660 32core EVB Board
		if ((4 == macro_id) && ((0 == ds_num) || (2 == ds_num))) {
			sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10),
					   8, 8, 0x1);
		}
	} else if (ebord_type == EM_V2R1CO5_BORAD) {
		if ((1 == macro_id) && ((0 == ds_num) || (1 == ds_num))) {
			sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10),
					   8, 8, 0x1);
		}
	} else {
		//
	}
#endif

	// Set CTLE/DFE parameters for common mode or PCIE 2.5Gbps&5Gbps or SAS/SATA 1.5Gbps&3Gbps&6Gbps or do CTLE/DFE adaptation
	// gain = 777  /boost = 753  /CMB = 222  /RMB = 222  /Zero = 222  /SQH = 222
	/*0xfaa7陆芒戮枚PCIE录矛虏芒虏禄碌陆露脭露脣碌脛脦脢脤芒 */
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 1), 15, 0, 0xbaa7);
	/*P660 Programming User Configuration.xlsx modify */
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 2), 15, 0, 0x3aa7);
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 3), 15, 0, 0x3aa7);

	sds_reg_bits_write(node, macro_id, DSCLK_CSR(ds_num, 1), 9, 9, 0x1);

}

// DS Configuration after DS calibration
void ds_cfg_after_calibration(unsigned int node, unsigned int macro_id,
			      unsigned int ds_num, unsigned int ds_cfg,
			      unsigned int flag)
{
	unsigned int reg_val;

	if (flag == AC_MODE_FLAG) {
		// Use AC couple mode (cap bypass mode), suggested mode for most customer
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 0), 13, 13,
				   0x0);
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 40), 9, 9,
				   0x0);
	} else {
		// Use DC couple mode (cap bypass mode), suggested mode for most customer
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 0), 13, 13,
				   0x1);
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 40), 9, 9,
				   0x1);

		reg_val =
		    sds_reg_bits_read(node, macro_id, RX_CSR(ds_num, 0), 2, 0);
		if (reg_val == 0)
			sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 0), 2, 0, 0x1);	// CTLEPASSGN cannot set to 0 in DC couple mode
	}

	// Rx termination floating set
	//RXTERM_FLOATING (RX_CSR27 bit[12]) = User Setting;
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 27), 12, 12, 0x1);

	// signal detection and read the ret from Pin SQUELCH_DET and ALOS
	//SIGDET_ENABLE (RX_CSR60 bit[15]) = User Setting;
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 60), 15, 15, 0x1);

	if ((SAS_1500 == ds_cfg) || (SAS_3000 == ds_cfg) || (SAS_6000 == ds_cfg)
	    || (SAS_12000 == ds_cfg)) {
		//RX_SIGDET_BLD_DLY_SEL (RX_CSR31 bit[0]) = User Setting;
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 31), 0, 0,
				   0x1);
		//SIGDET_WIN (RX_CSR60 bit[14:11])
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 60), 14, 11,
				   0);
	} else if ((PCIE_2500 == ds_cfg) || (PCIE_5000 == ds_cfg)
		   || (PCIE_8000 == ds_cfg)) {
		//RX_SIGDET_BLD_DLY_SEL (RX_CSR31 bit[0]) = User Setting;
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 31), 0, 0,
				   0x0);
		//SIGDET_WIN (RX_CSR60 bit[14:11])
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 60), 14, 11,
				   0x5);
	} else {
		//RX_SIGDET_BLD_DLY_SEL (RX_CSR31 bit[0]) = User Setting;
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 31), 0, 0,
				   0x0);
		//SIGDET_WIN (RX_CSR60 bit[14:11])
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 60), 14, 11,
				   0x6);
	}

	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 61), 10, 0, 2);

	switch (ds_cfg) {
	case GE_1250:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 60), 14, 11,
				   6);
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 62), 10, 0,
				   0x40);
		break;
	case GE_3125:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 60), 14, 11,
				   6);
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 62), 10, 0,
				   0x40);
		break;
	case XGE_10312:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 60), 14, 11,
				   6);
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 62), 10, 0,
				   0x40);
		break;
	case PCIE_2500:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 60), 14, 11,
				   5);
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 62), 10, 0,
				   0x28);
		break;
	case PCIE_5000:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 60), 14, 11,
				   5);
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 62), 10, 0,
				   0x28);
		break;
	case PCIE_8000:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 60), 14, 11,
				   5);
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 62), 10, 0,
				   0x28);
		break;
	case SAS_1500:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 60), 14, 11,
				   0);
		break;
	case SAS_3000:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 60), 14, 11,
				   0);
		break;
	case SAS_6000:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 60), 14, 11,
				   0);
		break;
	case SAS_12000:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 60), 14, 11,
				   0);
		break;
	case HCCS_32:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 60), 14, 11,
				   6);
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 62), 10, 0,
				   0x40);
	case HCCS_40:
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 60), 14, 11,
				   6);
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 62), 10, 0,
				   0x40);
		break;
	default:
		break;
	}

	// Other specified Dataslice settings for certain application added below
	// Customer should confirm with HiLink support team for extra settings
}

void ds_hw_calibration_init(unsigned int node, unsigned int macro_id,
			    unsigned int ds_num)
{
	// Power up eye monitor
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 15), 15, 15, 0x1);

	// Set RX, DSCLK and TX to auto calibration mode
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 27), 15, 15, 0x0);
	sds_reg_bits_write(node, macro_id, DSCLK_CSR(ds_num, 9), 13, 13, 0x0);
	sds_reg_bits_write(node, macro_id, DSCLK_CSR(ds_num, 9), 12, 10, 0x0);
	sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 14), 5, 4, 0x0);

	// Make sure Tx2Rx loopback is disabled
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 0), 10, 10, 0x0);

	// Set Rx to AC couple mode for calibration
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 0), 13, 13, 0x0);

	//ECOARSEALIGNSTEP (RX_CSR45 bit[6:4]) = 0x0;, pug v1.2
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 45), 6, 4, 0x0);
}

unsigned int ds_hw_calibration_exec(unsigned int node, unsigned int macro_id,
				    unsigned int ds_num)
{
	unsigned int chk_cnt;

	// Start data slice calibration by rising edge of DSCALSTART
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 27), 14, 14, 0x0);
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 27), 14, 14, 0x1);

	// Check whether calibration complete
	chk_cnt = 0;		// temp local variable for time out count of DS calibration down check
	do {
		chk_cnt++;
		serdes_delay_us(1000);
	} while (chk_cnt <= 100
		 &&
		 (sds_reg_bits_read(node, macro_id, TX_CSR(ds_num, 26), 15, 15)
		  == 0
		  || sds_reg_bits_read(node, macro_id, TX_CSR(ds_num, 26), 14,
				       14) == 0));

	if (chk_cnt > 100) {
		printk
		    ("[ds_hw_calibration_exec]:Macro%d DS%d  check count > 100  calibration not completed  \n",
		     macro_id, ds_num);
		return SRE_HILINK_DS_CALIB_ERR;	// Calibration failed
	}
	// Set the loss-of-lock detector to continuous running mode
	sds_reg_bits_write(node, macro_id, DSCLK_CSR(ds_num, 15), 0, 0, 0x0);

	// Start dsclk loss of lock detect
	sds_reg_bits_write(node, macro_id, DSCLK_CSR(ds_num, 10), 9, 9, 0x0);
	sds_reg_bits_write(node, macro_id, DSCLK_CSR(ds_num, 10), 9, 9, 0x1);
	// Calibration succeed
	return SRE_OK;

}

void ds_hw_calibration_adjust(unsigned int node, unsigned int macro_id,
			      unsigned int ds_num)
{
	int i;
	unsigned int rd_reg = 0;

	// Power down eye monitor
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 15), 15, 15, 0x0);

	// Transfer Rx auto calibration value to registers and adjust Rx termination
	for (i = 1; i <= 6; i++) {
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 27), 5, 0, i);
		rd_reg =
		    sds_reg_bits_read(node, macro_id, TX_CSR(ds_num, 42), 5, 0);
		rd_reg = rd_reg & 0x1f;

		switch (i) {
		case 1:
			sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 28),
					   14, 10, rd_reg);
			break;
		case 2:
			sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 28),
					   9, 5, rd_reg);
			break;
		case 3:
			sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 28),
					   4, 0, rd_reg);
			break;
		case 4:
			sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 29),
					   14, 10, rd_reg);
			break;
		case 5:
			sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 29),
					   9, 5, rd_reg);
			break;
		case 6:
			sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 29),
					   4, 0, rd_reg);
			break;
		default:
			break;
		}
	}

	sds_reg_bits_write(node, macro_id, DSCLK_CSR(ds_num, 9), 13, 13, 0x1);
}

/*************************************************************
Prototype    : serdes_ds_calib
Description  :serdes DS  脨拢脳录
Input        :
               unsigned int macro_id  Macro ID:     Macro0/Macro1/Macro2
               unsigned int ds_num    Ds Number
               unsigned int ds_cfg   露脭脫娄虏禄脥卢脛拢脢陆潞脥脧脽脣脵脗脢碌脛DS 脜盲脰脙
Output       : 脦脼
Return Value :
Calls        :
Called By    :

History        :
1.Date         : 2013/10/11
 Author       : w00244733
 Modification : adapt to hi1381

***********************************************************/
unsigned int serdes_ds_calib(unsigned int node, unsigned int macro_id,
			     unsigned int ds_num, unsigned int ds_cfg)
{
	unsigned int ret = SRE_OK;

	// First do initialization before calibration
	ds_hw_calibration_init(node, macro_id, ds_num);

	// Then execute Ds hardware calibration
	ret = ds_hw_calibration_exec(node, macro_id, ds_num);
	if (ret != SRE_OK) {
		printk
		    ("[serdes_ds_calib]:Macro%d DS%d  call ds_hw_calibration_exec error \n",
		     macro_id, ds_num);
		return SRE_HILINK_DS_CALIB_ERR;
	}
	// Finally do Ds hardware calibration adjustment
	ds_hw_calibration_adjust(node, macro_id, ds_num);

	// Check calibration results

	serdes_delay_us(1000);
	// Check out of lock when in ring VCO mode

	if (sds_reg_bits_read(node, macro_id, DSCLK_CSR(ds_num, 0), 1, 1) == 0) {
		return SRE_OK;	// calibration success
	} else {
		printk
		    ("[serdes_ds_calib]:Macro%d DS%d  calibration results error \n",
		     macro_id, ds_num);
		return SRE_HILINK_DS_CALIB_ERR;	// calibration false
	}

}

/*************************************************************
Prototype    : pma_init
Description  :
Input        :

                    unsigned int ulFlag  卤锚脰戮PCIE脛拢脢陆脟脨禄禄拢卢

Output       : 脦脼
Calls        :
Called By    :

History        :
1.Date         : 2013/10/11
 Author       : w00244733
 Modification : adapt to hi1381,脪脝脰虏6602

***********************************************************/
void pma_init(unsigned int node, unsigned int macro_id, unsigned int ds_num,
	      unsigned int ds_cfg, unsigned int cs_src, unsigned int pma_mode)
{

	// PMA mode configure:0-PCIe, 1-SAS, 2-Normal Mode
	sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 48), 13, 12,
			   pma_mode);

	if (PMA_MODE_SAS < pma_mode)
		return;

	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 54), 15, 13, 0x0);
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 54), 11, 8, 0x9);
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 55), 15, 8, 0xa);
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 55), 7, 0, 0x0);
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 56), 15, 8, 0xa);
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 56), 7, 0, 0x14);

	// Enable eye metric function
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 15), 15, 15, 0x1);

	// Set CDR_MODE. 0-Normal CDR(default), 1-SSCDR
	if (pma_mode == PMA_MODE_SAS) {
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 6, 6,
				   cdr_mode);
	} else if (pma_mode == PMA_MODE_PCIE) {
		sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 10), 6, 6,
				   0x0);
	} else {
		//do nothing
	}

	sds_reg_bits_write(node, macro_id, DSCLK_CSR(ds_num, 0), 9, 5, 0x1f);
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 53), 15, 11, 0x4);

	sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 48), 11, 11, 0x0);

	// Configure for pull up rxtx_status and enable pin_en
	sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 48), 14, 14, 0x1);
//    sds_reg_bits_write(node, macro_id, TX_CSR(ds_num,48),3,3, 0x1);
	sds_reg_bits_write(node, macro_id, RX_CSR(ds_num, 60), 15, 15, 0x1);
	sds_reg_bits_write(node, macro_id, TX_CSR(ds_num, 48), 3, 3, 0x1);

	return;
}

unsigned int serdes2_lane_reset(unsigned int node, unsigned int ds_num,
				unsigned int ds_cfg)
{
	unsigned int cs_src;
	unsigned int reg_val;
	unsigned int reg_addr = 0;
	unsigned int i = 0;

	//disable CTLE/DFE
	sds_reg_bits_write(node, MACRO_2, DS_API(ds_num) + 0, 6, 6, 0);
	sds_reg_bits_write(node, MACRO_2, DS_API(ds_num) + 4, 7, 4, 0);

	//CTLE and DFE adaptation reset bit11 in the PUG control0
	sds_reg_bits_write(node, MACRO_2, DS_API(ds_num) + 4, 3, 3, 1);

	//CTLE and DFE adaptation reset status
	i = 1000;
	reg_val = sds_reg_bits_read(node, MACRO_2, DS_API(ds_num) + 12, 3, 3);
	while ((1 != reg_val) && i) {
		serdes_delay_us(100);
		i--;
		reg_val =
		    sds_reg_bits_read(node, MACRO_2, DS_API(ds_num) + 12, 3, 3);
	}

	if (0 == i)
		printk("[serdes2_lane_reset]:CTLE/DFE reset timeout\n");

	//CTLE and DFE adaptation reset release bit11 in the PUG control0
	sds_reg_bits_write(node, MACRO_2, DS_API(ds_num) + 4, 3, 3, 0);

	reg_addr = SRE_HILINK2_MACRO_LRSTB_REG;
	//lane reset
	reg_val = system_reg_read(node, reg_addr);
	reg_val &= (~(0x1 << ds_num));
	system_reg_write(node, reg_addr, reg_val);

	cs_src = CS0;
	serdes_ds_cfg(node, MACRO_2, ds_num, ds_cfg, cs_src);
	if (SRE_OK != serdes_ds_calib(node, MACRO_2, ds_num, ds_cfg)) {
		printk("[serdes2_lane_reset]:Macro2 Ds%d Calibrate fail!\r\n",
		       ds_num);
		return EM_SERDES_FAIL;
	}

	ds_cfg_after_calibration(node, MACRO_2, ds_num, ds_cfg, DC_MODE_FLAG);
	pma_init(node, MACRO_2, ds_num, ds_cfg, cs_src, PMA_MODE_SAS);

	return EM_SERDES_SUCCESS;
}

unsigned int serdes5_lane_reset(unsigned int node, unsigned int ds_num,
				unsigned int ds_cfg)
{
	unsigned int cs_src;
	unsigned int reg_val;
	unsigned long long reg_addr = 0;
	unsigned int i = 0;

	//disable CTLE/DFE
	sds_reg_bits_write(node, MACRO_5, DS_API(ds_num) + 0, 6, 6, 0);
	sds_reg_bits_write(node, MACRO_5, DS_API(ds_num) + 4, 7, 4, 0);

	//CTLE and DFE adaptation reset bit11 in the PUG control0
	sds_reg_bits_write(node, MACRO_5, DS_API(ds_num) + 4, 3, 3, 1);

	//CTLE and DFE adaptation reset status
	i = 1000;
	reg_val = sds_reg_bits_read(node, MACRO_5, DS_API(ds_num) + 12, 3, 3);
	while ((1 != reg_val) && i) {
		serdes_delay_us(100);
		i--;
		reg_val =
		    sds_reg_bits_read(node, MACRO_5, DS_API(ds_num) + 12, 3, 3);
	}

	if (0 == i)
		printk("[serdes5_lane_reset]:CTLE/DFE reset timeout!\r\n");

	//CTLE and DFE adaptation reset release bit11 in the PUG control0
	sds_reg_bits_write(node, MACRO_5, DS_API(ds_num) + 4, 3, 3, 0);

	reg_addr = SRE_HILINK5_MACRO_LRSTB_REG;
	//lane reset
	reg_val = system_reg_read(node, reg_addr);
	reg_val &= (~(0x1 << ds_num));
	system_reg_write(node, reg_addr, reg_val);
	serdes_delay_us(10);

	cs_src = CS0;
	serdes_ds_cfg(node, MACRO_5, ds_num, ds_cfg, cs_src);
	if (SRE_OK != serdes_ds_calib(node, MACRO_5, ds_num, ds_cfg)) {
		printk("[serdes5_lane_reset]:Macro5 Ds%d Calibrate fail!\r\n",
		       ds_num);
		return EM_SERDES_FAIL;
	}

	ds_cfg_after_calibration(node, MACRO_5, ds_num, ds_cfg, DC_MODE_FLAG);
	pma_init(node, MACRO_5, ds_num, ds_cfg, cs_src, PMA_MODE_SAS);

	return EM_SERDES_SUCCESS;
}

static unsigned int serdes6_lane_reset(unsigned int node, unsigned int ds_num,
				       unsigned int ds_cfg)
{
	unsigned int cs_src;
	unsigned int reg_val;
	unsigned long long reg_addr = 0;
	unsigned int i = 0;

	//disable CTLE/DFE
	sds_reg_bits_write(node, MACRO_6, DS_API(ds_num) + 0, 6, 6, 0);
	sds_reg_bits_write(node, MACRO_6, DS_API(ds_num) + 4, 7, 4, 0);

	//CTLE and DFE adaptation reset bit11 in the PUG control0
	sds_reg_bits_write(node, MACRO_6, DS_API(ds_num) + 4, 3, 3, 1);

	//CTLE and DFE adaptation reset status
	i = 1000;
	reg_val = sds_reg_bits_read(node, MACRO_6, DS_API(ds_num) + 12, 3, 3);
	while ((1 != reg_val) && i) {
		serdes_delay_us(100);
		i--;
		reg_val =
		    sds_reg_bits_read(node, MACRO_6, DS_API(ds_num) + 12, 3, 3);
	}

	if (0 == i)
		printk("[serdes6_lane_reset]:CTLE/DFE reset timeout!\r\n");

	//CTLE and DFE adaptation reset release bit11 in the PUG control0
	sds_reg_bits_write(node, MACRO_6, DS_API(ds_num) + 4, 3, 3, 0);

	reg_addr = SRE_HILINK6_MACRO_LRSTB_REG;
	//lane reset
	reg_val = system_reg_read(node, reg_addr);
	reg_val &= (~(0x1 << ds_num));
	system_reg_write(node, reg_addr, reg_val);
	serdes_delay_us(10);

	cs_src = CS0;
	serdes_ds_cfg(node, MACRO_6, ds_num, ds_cfg, cs_src);
	if (SRE_OK != serdes_ds_calib(node, MACRO_6, ds_num, ds_cfg)) {
		printk("[serdes6_lane_reset]:Macro6 Ds%d Calibrate fail!\r\n",
		       ds_num);
		return EM_SERDES_FAIL;
	}

	ds_cfg_after_calibration(node, MACRO_6, ds_num, ds_cfg, DC_MODE_FLAG);
	pma_init(node, MACRO_6, ds_num, ds_cfg, cs_src, PMA_MODE_SAS);
	return EM_SERDES_SUCCESS;
}

unsigned int higgs_comm_serdes_lane_reset(unsigned int node,
					  unsigned int macro_id,
					  unsigned int ds_num,
					  unsigned int ds_cfg)
{
	unsigned int ret = EM_SERDES_SUCCESS;

	if (EM_SERDES_SUCCESS != serdes_check_param(macro_id, ds_num)) {
		printk("macro%d lane %d param invalid!\n", macro_id, ds_num);
		return EM_SERDES_FAIL;
	}

	if (ds_cfg != SAS_1500)
		printk("ds_cfg %u invalid not support!\n", ds_cfg);

	//printk( "node:%u,macro_id:%u,ds_num:%u,ds_cfg:%u\n",node,macro_id,ds_num,ds_cfg);
	switch (macro_id) {
	case MACRO_0:
	case MACRO_1:
		break;

	case MACRO_2:
		if (1 == system_reg_read(node, SRE_HILINK2_MUX_CTRL_REG))
			ret = serdes2_lane_reset(node, ds_num, ds_cfg);

		break;

	case MACRO_3:
	case MACRO_4:
		break;

	case MACRO_5:
		if (1 == system_reg_read(node, SRE_HILINK5_MUX_CTRL_REG))
			ret = serdes5_lane_reset(node, ds_num, ds_cfg);

		break;
	case MACRO_6:
		ret = serdes6_lane_reset(node, ds_num, ds_cfg);
		break;

	default:
		printk("invaild macro\n");
		break;
	}

	return ret;
}

void higgs_comm_serdes_enable_ctledfe(unsigned int node, unsigned int macro,
				      unsigned int lane, unsigned int ds_cfg)
{
	//printk( "node:%u,macro_id:%u,ds_num:%u,ds_cfg:%u\n",node,macro,lane,ds_cfg);
	if ((HCCS_32 == ds_cfg) || (HCCS_40 == ds_cfg)) {
		sds_reg_bits_write(node, macro, DS_API(lane) + 0, 15, 0,
				   0x783d);
		//control_0
		sds_reg_bits_write(node, macro, DS_API(lane) + 4, 15, 0,
				   0xc851);
	}

	if ((PCIE_8000 == ds_cfg)) {
		//step2 Enable CTLE/DFE
		//sds_reg_bits_write(node, macro,DS_API(lane)+0, 15, 0,0x7484);
		//control_0
		sds_reg_bits_write(node, macro, DS_API(lane) + 4, 15, 0,
				   0x8851);
	}

	if ((SAS_12000 == ds_cfg)) {
		//step2 Enable CTLE/DFE
		sds_reg_bits_write(node, macro, DS_API(lane) + 0, 15, 0,
				   0x5664);
		//control_0
#if 0
		sds_reg_bits_write(node, macro, DS_API(lane) + 4, 15, 0,
				   0xc851);
#else
		//sds_reg_bits_write(node, macro,DS_API(lane)+4, 15, 0,0x4851);
		sds_reg_bits_write(node, macro, DS_API(lane) + 4, 15, 0,
				   0x7851);
#endif
	}
}
