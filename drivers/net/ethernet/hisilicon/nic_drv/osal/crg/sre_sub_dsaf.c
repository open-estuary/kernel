/*--------------------------------------------------------------------------------------------------------------------------*/
/*!!Warning: This is a key information asset of Huawei Tech Co.,Ltd                                                         */
/*CODEMARK:kOyQZYzjDpyGdBAEC2GaWuVy7vy/wDnq7gJfHBOj2pBXFF9pJtpDLt9sw5WJiMsUkN5d7jr7
aK5J3kmlnl+vpQIjFDOn3HlYdNfd+O2Mso0Wn8OrHjkJn4/rA05FKgT2JMPJkXZ1ZyV4aAJ6
v59wfQSN0TIrOVdUKjC0BbCUrbJg2BMEh3GREuXXCXXdAGTFcvkJ7e6nkvLybnBN4YmU50an
8gywnUZ0sEL9LGFdkWx2WcTHxhkQVAakbASsFfJXU4Sdq7r37jhcUlOn2AxDYQ==*/
/*--------------------------------------------------------------------------------------------------------------------------*/
/******************************************************************************

                  版权所有 (C), 2011-2021, 华为技术有限公司

 ******************************************************************************
  文 件 名   : sre_crs21.c
  版 本 号   : 初稿
  作    者   : z00176027
  生成日期   : 2013年10月17日
  最近修改   :
  功能描述   : P660 子系统21 复位及去复位
  函数列表   :
  修改历史   :
  1.日    期   : 2013年10月17日
    作    者   : z00176027
    修改内容   : 创建文件

******************************************************************************/

#include "sre_sub_dsaf.h"


/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/
UINT64 sub_dsaf_base_addr = 0;
UINT64 sub_dsaf_pa_addr = 0;

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

    /* 定义Dsaf SUB的寄存器读写接口 */
#ifdef HRD_OS_LINUX
inline UINT32 DSAF_SUB_READ_REG(UINT32 pRegBase, UINT32 ulRegIndex)
{
    volatile UINT32 temp;

    temp = ioread32((void __iomem *)(sub_dsaf_base_addr + (pRegBase - DSAF_SUB_BASE) + (ulRegIndex << 2)));

    return temp;
}

#define DSAF_SUB_WRITE_REG(pRegBase, ulRegIndex, ulValue) \
   (iowrite32(ulValue, (void __iomem *)(sub_dsaf_base_addr + (pRegBase - DSAF_SUB_BASE) + (ulRegIndex << 2))))

#else
#define DSAF_SUB_READ_REG(pRegBase, ulRegIndex) \
        OS_READ_REG(pRegBase, ulRegIndex)

#define DSAF_SUB_WRITE_REG(pRegBase, ulRegIndex, ulValue) \
        OS_WRITE_REG(pRegBase, ulRegIndex, ulValue)
#endif

/*
#define HRD_DSAF0_ADDRESS DSAF_SUB_BASE
#define HRD_DSAF1_ADDRESS M3_SUB_BASE
#define HRD_DSAF2_ADDRESS PCIE_SUB_BASE
#define HRD_DSAF3_ADDRESS ALG_SUB_BASE
*/
#define HRD_DSAF0_ADDRESS 0xC0000000
#define HRD_DSAF1_ADDRESS 0xA0000000
#define HRD_DSAF2_ADDRESS 0xB0000000
#define HRD_DSAF3_ADDRESS 0xD0000000

UINT32 HRD_Dsaf_GetAddressByIndex(UINT32 ulDsafIndex)
{
    UINT32 ulAddr = 0;
    switch ( ulDsafIndex )
    {
        case 0 :
            ulAddr = HRD_DSAF0_ADDRESS;
            break;
        case 1 :
            ulAddr = HRD_DSAF1_ADDRESS;
            break;
        case 2 :
            ulAddr = HRD_DSAF2_ADDRESS;
            break;
        case 3 :
            ulAddr = HRD_DSAF3_ADDRESS;
            break;
        default:
            ulAddr = HRD_DSAF0_ADDRESS;
    }

    return ulAddr;
}

/*****************************************************************************
函 数 名  : HRD_Dsaf_XbarSrst
功能描述  : Xbar复位及解复位
输入参数  : UINT32 ulEn  0:复位 >=1:去复位
输出参数  : 无
返 回 值  :
调用函数  :
被调函数  :

修改历史      :
 1.日    期   : 2013年10月25日
   作    者   : z00176027
   修改内容   : 新生成函数

*****************************************************************************/
UINT32 HRD_Dsaf_XbarSrst(UINT32 ulDsafIndex,UINT32 ulEn)
{
    SC_XBAR_RESET_REQ_U o_sc_xbar_reset_req;
    SC_XBAR_RESET_DREQ_U o_sc_xbar_reset_dreq;

    UINT32 ulAddr = 0;

    ulAddr = HRD_Dsaf_GetAddressByIndex(ulDsafIndex);

    osal_printf("%d,%s:%x\r\n",__LINE__,__FUNCTION__,ulAddr);
    memset(&o_sc_xbar_reset_req,0,sizeof(SC_XBAR_RESET_REQ_U));
    memset(&o_sc_xbar_reset_dreq,0,sizeof(SC_XBAR_RESET_DREQ_U));

    if(0 == ulEn)
    {
         ulAddr += (DSAF_SUB_SC_XBAR_RESET_REQ_REG - HRD_DSAF0_ADDRESS);
         o_sc_xbar_reset_req.bits.xbar_srst_req = 1;
         DSAF_SUB_WRITE_REG(ulAddr,0,o_sc_xbar_reset_req.u32);
    }
    else
    {
        ulAddr += (DSAF_SUB_SC_XBAR_RESET_DREQ_REG - HRD_DSAF0_ADDRESS);
        o_sc_xbar_reset_dreq.bits.xbar_srst_dreq = 1;
        DSAF_SUB_WRITE_REG(ulAddr,0,o_sc_xbar_reset_dreq.u32);
    }

    return OS_SUCCESS;
}


/*****************************************************************************
函 数 名  : HRD_Dsaf_NtSrst
功能描述  : Nt复位及解复位
输入参数  : UINT32 ulEn  0:复位 >=1:去复位
输出参数  : 无
返 回 值  :
调用函数  :
被调函数  :

修改历史      :
 1.日    期   : 2013年10月25日
   作    者   : z00176027
   修改内容   : 新生成函数

*****************************************************************************/
UINT32 HRD_Dsaf_NtSrst(UINT32 ulDsafIndex,UINT32 ulEn)
{
    SC_NT_RESET_REQ_U o_sc_nt_reset_req;
    SC_NT_RESET_DREQ_U o_sc_nt_reset_dreq;
    UINT32 ulAddr = 0;

    ulAddr = HRD_Dsaf_GetAddressByIndex(ulDsafIndex);

    memset(&o_sc_nt_reset_req,0,sizeof(SC_NT_RESET_REQ_U));
    memset(&o_sc_nt_reset_dreq,0,sizeof(SC_NT_RESET_DREQ_U));

    if(0 == ulEn)
    {
         ulAddr += (DSAF_SUB_SC_NT_RESET_REQ_REG - HRD_DSAF0_ADDRESS);
         o_sc_nt_reset_req.bits.nt_srst_req = 1;
         DSAF_SUB_WRITE_REG(ulAddr,0,o_sc_nt_reset_req.u32);
    }
    else
    {
        ulAddr += (DSAF_SUB_SC_NT_RESET_DREQ_REG - HRD_DSAF0_ADDRESS);
        o_sc_nt_reset_dreq.bits.nt_srst_dreq = 1;
        DSAF_SUB_WRITE_REG(ulAddr,0,o_sc_nt_reset_dreq.u32);
    }

    return OS_SUCCESS;
}


/*****************************************************************************
函 数 名  : HRD_Dsaf_XgeSrstByPort
功能描述  : Xge复位及解复位
输入参数  : UINT32 ulPort  ulPort   暂时没有区分port，随便写个port，所有port全部去复位
            UINT32 ulEn  0:复位 >=1:去复位
输出参数  : 无
返 回 值  :
调用函数  :
被调函数  :

修改历史      :
 1.日    期   : 2013年10月26日
   作    者   : z00176027
   修改内容   : 新生成函数

*****************************************************************************/
UINT32 HRD_Dsaf_XgeSrstByPort(UINT32 ulPort, UINT32 ulEn)
{
    SC_XGE_RESET_REQ_U o_sc_xge_reset_req;
    SC_XGE_RESET_DREQ_U o_sc_xge_reset_dreq;


    memset(&o_sc_xge_reset_req,0,sizeof(SC_XGE_RESET_REQ_U));
    memset(&o_sc_xge_reset_dreq,0,sizeof(SC_XGE_RESET_DREQ_U));

    if(ulPort >= PV660_XGE_NUM)
    {
        return OS_FAIL;
    }


    if(0 == ulEn)
    {
        o_sc_xge_reset_req.bits.xge_cfg_srst_req = 0x1;
        o_sc_xge_reset_req.u32 |= 0x2082082 <<ulPort;
        DSAF_SUB_WRITE_REG(DSAF_SUB_SC_XGE_RESET_REQ_REG,0,o_sc_xge_reset_req.u32);
    }
    else
    {
        o_sc_xge_reset_dreq.bits.xge_cfg_srst_dreq = 0x1;
        o_sc_xge_reset_dreq.u32 |= 0x2082082 <<ulPort;
        DSAF_SUB_WRITE_REG(DSAF_SUB_SC_XGE_RESET_DREQ_REG,0,o_sc_xge_reset_dreq.u32);
    }

    return OS_SUCCESS;
}

/*****************************************************************************
函 数 名  : HRD_Dsaf_GeSrst
功能描述  : ge复位及解复位
输入参数  : UINT32 ulPort  ulPort 暂时没有区分port，随便写个port，所有port全部去复位
            UINT32 ulEn  0:复位 >=1:去复位
输出参数  : 无
返 回 值  :
调用函数  :
被调函数  :

修改历史      :
 1.日    期   : 2013年10月26日
   作    者   : z00176027
   修改内容   : 新生成函数

*****************************************************************************/
UINT32 HRD_Dsaf_GeSrstByPort(UINT32 ulPort, UINT32 ulEn)
{
    SC_GE_RESET_REQ0_U o_sc_ge_reset_req0;
    SC_GE_RESET_DREQ0_U o_sc_ge_reset_dreq0;
    SC_GE_RESET_REQ1_U o_sc_ge_reset_req1;
    SC_GE_RESET_DREQ1_U o_sc_ge_reset_dreq1;

    /* for GE6/GE7 needed ppe rest , GE6/GE7 can vld, by lisheng***/
    SC_PPE_RESET_REQ_U o_sc_ppe_reset_req;
    SC_PPE_RESET_DREQ_U o_sc_ppe_reset_dreq;

    if(ulPort >= PV660_GE_NUM)
    {
        return OS_FAIL;
    }

    memset(&o_sc_ge_reset_req0,0,sizeof(SC_GE_RESET_REQ0_U));
    memset(&o_sc_ge_reset_dreq0,0,sizeof(SC_GE_RESET_DREQ0_U));
    memset(&o_sc_ge_reset_req1,0,sizeof(SC_GE_RESET_REQ1_U));
    memset(&o_sc_ge_reset_dreq1,0,sizeof(SC_GE_RESET_DREQ1_U));
    memset(&o_sc_ppe_reset_req,0,sizeof(SC_PPE_RESET_REQ_U));
    memset(&o_sc_ppe_reset_dreq,0,sizeof(SC_PPE_RESET_DREQ_U));

    if(ulPort < 6)
    {
        if(0 == ulEn)
        {
            /* 先复位fifo */
            o_sc_ge_reset_req1.u32 |= 0x1 <<ulPort;
            DSAF_SUB_WRITE_REG(DSAF_SUB_SC_GE_RESET_REQ1_REG,0,o_sc_ge_reset_req1.u32);

            o_sc_ge_reset_req0.u32 |= 0x1041041 <<ulPort;
            DSAF_SUB_WRITE_REG(DSAF_SUB_SC_GE_RESET_REQ0_REG,0,o_sc_ge_reset_req0.u32);
        }
        else
        {
            /* 先解复位整体 */
            o_sc_ge_reset_dreq0.u32 |= 0x1041041 <<ulPort;
            DSAF_SUB_WRITE_REG(DSAF_SUB_SC_GE_RESET_DREQ0_REG,0,o_sc_ge_reset_dreq0.u32);

            o_sc_ge_reset_dreq1.u32 |= 0x1 <<ulPort;
            DSAF_SUB_WRITE_REG(DSAF_SUB_SC_GE_RESET_DREQ1_REG,0,o_sc_ge_reset_dreq1.u32);
        }
    }
    else
    {
        if(0 == ulEn)
        {
            /* 先复位fifo */
            o_sc_ge_reset_req1.u32 |= 0x15540 <<(ulPort-6);
            DSAF_SUB_WRITE_REG(DSAF_SUB_SC_GE_RESET_REQ1_REG,0,o_sc_ge_reset_req1.u32);

            //o_sc_ge_reset_req0.u32 |= 0x1041041 <<ulPort;
            //DSAF_SUB_WRITE_REG(DSAF_SUB_SC_GE_RESET_REQ0_REG,0,o_sc_ge_reset_req0.u32);
        }
        else
        {
            /* 先解复位整体 */
            //o_sc_ge_reset_dreq0.u32 |= 0x1041041 <<ulPort;
            //DSAF_SUB_WRITE_REG(DSAF_SUB_SC_GE_RESET_DREQ0_REG,0,o_sc_ge_reset_dreq0.u32);

            o_sc_ge_reset_dreq1.u32 |= 0x15540 <<(ulPort-6);
            DSAF_SUB_WRITE_REG(DSAF_SUB_SC_GE_RESET_DREQ1_REG,0,o_sc_ge_reset_dreq1.u32);
        }

        if(0 == ulEn)
        {
            o_sc_ppe_reset_req.u32 |= (0x100 << (ulPort-6));
            DSAF_SUB_WRITE_REG(DSAF_SUB_SC_PPE_RESET_REQ_REG,0,o_sc_ppe_reset_req.u32);
        }
        else
        {
            o_sc_ppe_reset_dreq.u32 |= (0x100 << (ulPort-6));
            DSAF_SUB_WRITE_REG(DSAF_SUB_SC_PPE_RESET_DREQ_REG,0,o_sc_ppe_reset_dreq.u32);
        }

    }

    return OS_SUCCESS;
}



/*****************************************************************************
函 数 名  : HRD_Dsaf_PpeSrst
功能描述  : Ppe复位及解复位
输入参数  : UINT32 ulPort  ulPort
            UINT32 ulEn  0:复位 >=1:去复位
输出参数  : 无
返 回 值  :
调用函数  :
被调函数  :

修改历史      :
 1.日    期   : 2013年10月26日
   作    者   : z00176027
   修改内容   : 新生成函数

*****************************************************************************/
UINT32 HRD_Dsaf_PpeSrstByPort(UINT32 ulPort,UINT32 ulEn)
{
    SC_PPE_RESET_REQ_U o_sc_ppe_reset_req;
    SC_PPE_RESET_DREQ_U o_sc_ppe_reset_dreq;


    if(ulPort >= PV660_XGE_NUM)
    {
        return OS_FAIL;
    }

    memset(&o_sc_ppe_reset_req,0,sizeof(SC_PPE_RESET_REQ_U));
    memset(&o_sc_ppe_reset_dreq,0,sizeof(SC_PPE_RESET_DREQ_U));


    if(0 == ulEn)
    {
        o_sc_ppe_reset_req.u32 |= (1 << ulPort);
        DSAF_SUB_WRITE_REG(DSAF_SUB_SC_PPE_RESET_REQ_REG,0,o_sc_ppe_reset_req.u32);
    }
    else
    {
        o_sc_ppe_reset_dreq.u32 |= (1 << ulPort);
        DSAF_SUB_WRITE_REG(DSAF_SUB_SC_PPE_RESET_DREQ_REG,0,o_sc_ppe_reset_dreq.u32);
    }

    return OS_SUCCESS;
}


/*****************************************************************************
函 数 名  : HRD_Dsaf_RoceSrst
功能描述  : Roce复位及解复位
输入参数  : UINT32 ulEn  0:复位 >=1:去复位
输出参数  : 无
返 回 值  :
调用函数  :
被调函数  :

修改历史      :
 1.日    期   : 2013年10月25日
   作    者   : z00176027
   修改内容   : 新生成函数

*****************************************************************************/
UINT32 HRD_Dsaf_RoceSrst(UINT32 ulEn)
{
    SC_ROCEE_RESET_REQ_U o_sc_roce_reset_req;
    SC_ROCEE_RESET_DREQ_U o_sc_roce_reset_dreq;

    memset(&o_sc_roce_reset_req,0,sizeof(SC_ROCEE_RESET_REQ_U));
    memset(&o_sc_roce_reset_dreq,0,sizeof(SC_ROCEE_RESET_DREQ_U));

    if(0 == ulEn)
    {
         o_sc_roce_reset_req.bits.rocee_srst_req = 1;
         DSAF_SUB_WRITE_REG(DSAF_SUB_SC_ROCEE_RESET_REQ_REG,0,o_sc_roce_reset_req.u32);
    }
    else
    {
        o_sc_roce_reset_dreq.bits.rocee_srst_dreq = 1;
        DSAF_SUB_WRITE_REG(DSAF_SUB_SC_ROCEE_RESET_DREQ_REG,0,o_sc_roce_reset_dreq.u32);
    }

    return OS_SUCCESS;
}

/*****************************************************************************
函 数 名  : HRD_Dsaf_CpuSrst
功能描述  : Dsaf M3 CPU复位及解复位
输入参数  : UINT32 ulEn  0:复位 >=1:去复位
输出参数  : 无
返 回 值  :
调用函数  :
被调函数  :

修改历史      :
 1.日    期   : 2013年10月22日
   作    者   : z00176027
   修改内容   : 新生成函数

*****************************************************************************/
UINT32 HRD_Dsaf_CpuSrst(UINT32 ulEn)
{
    SC_CPU_RESET_REQ_U o_sc_cpu_reset_req;
    SC_CPU_RESET_DREQ_U o_sc_cpu_reset_dreq;

    memset(&o_sc_cpu_reset_req,0,sizeof(SC_CPU_RESET_REQ_U));
    memset(&o_sc_cpu_reset_dreq,0,sizeof(SC_CPU_RESET_DREQ_U));

    if(0 == ulEn)
    {
         o_sc_cpu_reset_req.bits.cpu_srst_req = 1;
         DSAF_SUB_WRITE_REG(DSAF_SUB_SC_CPU_RESET_REQ_REG,0,o_sc_cpu_reset_req.u32);
    }
    else
    {
          o_sc_cpu_reset_dreq.bits.cpu_srst_dreq = 1;
          DSAF_SUB_WRITE_REG(DSAF_SUB_SC_CPU_RESET_DREQ_REG,0,o_sc_cpu_reset_dreq.u32);
    }

    return OS_SUCCESS;
}

/*****************************************************************************
函 数 名  : HRD_Dsaf_SasSrst
功能描述  : Dsaf Sas复位及解复位
输入参数  : UINT32 ulEn  0:复位 >=1:去复位
输出参数  : 无
返 回 值  :
调用函数  :
被调函数  :

修改历史      :
 1.日    期   : 2013年10月22日
   作    者   : z00176027
   修改内容   : 新生成函数

*****************************************************************************/
UINT32 HRD_Dsaf_SasSrst(UINT32 ulEn)
{
    SC_SAS_RESET_REQ_U o_sc_sas_reset_req;
    SC_SAS_RESET_DREQ_U o_sc_sas_reset_dreq;

    memset(&o_sc_sas_reset_req,0,sizeof(SC_SAS_RESET_REQ_U));
    memset(&o_sc_sas_reset_dreq,0,sizeof(SC_SAS_RESET_DREQ_U));

    if(0 == ulEn)
    {
         o_sc_sas_reset_req.u32 |= 0x7ffff;
         DSAF_SUB_WRITE_REG(DSAF_SUB_SC_SAS_RESET_REQ_REG,0,o_sc_sas_reset_req.u32);
    }
    else
    {
          o_sc_sas_reset_dreq.u32 |= 0x7ffff;
          DSAF_SUB_WRITE_REG(DSAF_SUB_SC_SAS_RESET_DREQ_REG,0,o_sc_sas_reset_dreq.u32);
    }

    return OS_SUCCESS;
}


/*****************************************************************************
函 数 名  : HRD_Dsaf_SllcSrstByChannel
功能描述  : Dsaf sllc复位及解复位
输入参数  : UINT32 ulChannel  ulChannel
            UINT32 ulEn  0:复位 >=1:去复位
输出参数  : 无
返 回 值  :
调用函数  :
被调函数  :

修改历史      :
 1.日    期   : 2013年10月26日
   作    者   : z00176027
   修改内容   : 新生成函数

*****************************************************************************/
UINT32 HRD_Dsaf_SllcSrstByChannel(UINT32 ulChannel,UINT32 ulEn)
{
    SC_SLLC_TSVRX_RESET_REQ_U o_sc_sllc_tsvrx_reset_req;
    SC_SLLC_TSVRX_RESET_DREQ_U o_sc_sllc_tsvrx_reset_dreq;

    if(ulChannel >= PV660_DSAF_SLLC_NUM)
    {
        return OS_FAIL;
    }

    memset(&o_sc_sllc_tsvrx_reset_req,0,sizeof(SC_SLLC_TSVRX_RESET_REQ_U));
    memset(&o_sc_sllc_tsvrx_reset_dreq,0,sizeof(SC_SLLC_TSVRX_RESET_DREQ_U));


    if(0 == ulEn)
    {
        o_sc_sllc_tsvrx_reset_req.u32 |= ulChannel;
        DSAF_SUB_WRITE_REG(DSAF_SUB_SC_SLLC_TSVRX_RESET_REQ_REG,0,o_sc_sllc_tsvrx_reset_req.u32);
    }
    else
    {
        o_sc_sllc_tsvrx_reset_dreq.u32 |= ulChannel;
        DSAF_SUB_WRITE_REG(DSAF_SUB_SC_SLLC_TSVRX_RESET_DREQ_REG,0,o_sc_sllc_tsvrx_reset_dreq.u32);
    }

    return OS_SUCCESS;
}


/*****************************************************************************
函 数 名  : HRD_Dsaf_ItsSrst
功能描述  : Its复位及解复位
输入参数  : UINT32 ulEn  0:复位 >=1:去复位
输出参数  : 无
返 回 值  :
调用函数  :
被调函数  :

修改历史      :
 1.日    期   : 2013年10月25日
   作    者   : z00176027
   修改内容   : 新生成函数

*****************************************************************************/
UINT32 HRD_Dsaf_ItsSrst(UINT32 ulEn)
{
    SC_ITS_RESET_REQ_U o_sc_its_reset_req;
    SC_ITS_RESET_DREQ_U o_sc_its_reset_dreq;

    memset(&o_sc_its_reset_req,0,sizeof(SC_ITS_RESET_REQ_U));
    memset(&o_sc_its_reset_dreq,0,sizeof(SC_ITS_RESET_DREQ_U));

    if(0 == ulEn)
    {
         o_sc_its_reset_req.bits.its_srst_req = 1;
         DSAF_SUB_WRITE_REG(DSAF_SUB_SC_ITS_RESET_REQ_REG,0,o_sc_its_reset_req.u32);
    }
    else
    {
        o_sc_its_reset_dreq.bits.its_srst_dreq = 1;
        DSAF_SUB_WRITE_REG(DSAF_SUB_SC_ITS_RESET_DREQ_REG,0,o_sc_its_reset_dreq.u32);
    }

    return OS_SUCCESS;
}


/*****************************************************************************
函 数 名  : HRD_Dsaf_SramSrst
功能描述  : DSAFM3 Sram复位及解复位
输入参数  : UINT32 ulEn  0:复位 >=1:去复位
输出参数  : 无
返 回 值  :
调用函数  :
被调函数  :

修改历史      :
 1.日    期   : 2013年10月25日
   作    者   : z00176027
   修改内容   : 新生成函数

*****************************************************************************/
UINT32 HRD_Dsaf_SramSrst(UINT32 ulEn)
{
    SC_SRAM_RESET_REQ_U o_sc_sram_reset_req;
    SC_SRAM_RESET_DREQ_U o_sc_sram_reset_dreq;

    memset(&o_sc_sram_reset_req,0,sizeof(SC_SRAM_RESET_REQ_U));
    memset(&o_sc_sram_reset_dreq,0,sizeof(SC_SRAM_RESET_DREQ_U));

    if(0 == ulEn)
    {
         o_sc_sram_reset_req.bits.sram_srst_req = 1;
         DSAF_SUB_WRITE_REG(DSAF_SUB_SC_SRAM_RESET_REQ_REG,0,o_sc_sram_reset_req.u32);
    }
    else
    {
        o_sc_sram_reset_dreq.bits.sram_srst_dreq = 1;
        DSAF_SUB_WRITE_REG(DSAF_SUB_SC_SRAM_RESET_DREQ_REG,0,o_sc_sram_reset_dreq.u32);
    }

    return OS_SUCCESS;
}


/*****************************************************************************
函 数 名  : HRD_Dsaf_PpeComSrst
功能描述  : PPE公共复位及解复位
输入参数  : UINT32 ulEn  0:复位 >=1:去复位
输出参数  : 无
返 回 值  :
调用函数  :
被调函数  :

修改历史      :
 1.日    期   : 2013年10月25日
   作    者   : z00176027
   修改内容   : 新生成函数

*****************************************************************************/
UINT32 HRD_Dsaf_PpeComSrst(UINT32 ulCommId, UINT32 ulEn)
{

    /**业务 ppe comm 为 0*/
    if(ulCommId < 1)
    {
        SC_RCB_PPE_COM_RESET_REQ_U o_sc_ppe_com_reset_req;
        SC_RCB_PPE_COM_RESET_DREQ_U o_sc_ppe_com_reset_dreq;

        memset(&o_sc_ppe_com_reset_req,0,sizeof(SC_RCB_PPE_COM_RESET_REQ_U));
        memset(&o_sc_ppe_com_reset_dreq,0,sizeof(SC_RCB_PPE_COM_RESET_DREQ_U));

        if(0 == ulEn)
        {
             o_sc_ppe_com_reset_req.bits.rcb_ppe_com_srst_req = 1;
             DSAF_SUB_WRITE_REG(DSAF_SUB_SC_RCB_PPE_COM_RESET_REQ_REG,0,o_sc_ppe_com_reset_req.u32);
        }
        else
        {
            o_sc_ppe_com_reset_dreq.bits.rcb_ppe_com_srst_dreq = 1;
            DSAF_SUB_WRITE_REG(DSAF_SUB_SC_RCB_PPE_COM_RESET_DREQ_REG,0,o_sc_ppe_com_reset_dreq.u32);
        }
    }
    /**调试ppe comm 编号为 1 2*/
    else
    {
        SC_PPE_RESET_REQ_U o_sc_ppe_reset_req;
        SC_PPE_RESET_DREQ_U o_sc_ppe_reset_dreq;

        memset(&o_sc_ppe_reset_req,0,sizeof(SC_PPE_RESET_REQ_U));
        memset(&o_sc_ppe_reset_dreq,0,sizeof(SC_PPE_RESET_DREQ_U));

        if(0 == ulEn)
        {
            o_sc_ppe_reset_req.u32 |= (0x100 << (ulCommId - 1));
            DSAF_SUB_WRITE_REG(DSAF_SUB_SC_PPE_RESET_REQ_REG,0,o_sc_ppe_reset_req.u32);
        }
        else
        {
            o_sc_ppe_reset_dreq.u32 |= (0x100 << (ulCommId - 1));
            DSAF_SUB_WRITE_REG(DSAF_SUB_SC_PPE_RESET_DREQ_REG,0,o_sc_ppe_reset_dreq.u32);
        }
    }

    return OS_SUCCESS;
}


/*****************************************************************************
函 数 名  : HRD_Dsaf_TimerSrst
功能描述  : Timer复位及解复位
输入参数  : UINT32 ulEn  0:复位 >=1:去复位
输出参数  : 无
返 回 值  :
调用函数  :
被调函数  :

修改历史      :
 1.日    期   : 2013年10月25日
   作    者   : z00176027
   修改内容   : 新生成函数

*****************************************************************************/
UINT32 HRD_Dsaf_TimerSrst(UINT32 ulEn)
{
    SC_TIMER_RESET_REQ_U o_sc_timer_reset_req;
    SC_TIMER_RESET_DREQ_U o_sc_timer_reset_dreq;

    memset(&o_sc_timer_reset_req,0,sizeof(SC_TIMER_RESET_REQ_U));
    memset(&o_sc_timer_reset_dreq,0,sizeof(SC_TIMER_RESET_DREQ_U));

    if(0 == ulEn)
    {
         o_sc_timer_reset_req.bits.timer_srst_req = 1;
         DSAF_SUB_WRITE_REG(DSAF_SUB_SC_TIMER_RESET_REQ_REG,0,o_sc_timer_reset_req.u32);
    }
    else
    {
        o_sc_timer_reset_dreq.bits.timer_srst_dreq = 1;
        DSAF_SUB_WRITE_REG(DSAF_SUB_SC_TIMER_RESET_DREQ_REG,0,o_sc_timer_reset_dreq.u32);
    }

    return OS_SUCCESS;
}


/*****************************************************************************
函 数 名  : HRD_Dsaf_Sll1cSrst
功能描述  : Sllc1复位及解复位
输入参数  : UINT32 ulEn  0:复位 >=1:去复位
输出参数  : 无
返 回 值  :
调用函数  :
被调函数  :

修改历史      :
 1.日    期   : 2013年10月25日
   作    者   : z00176027
   修改内容   : 新生成函数

*****************************************************************************/
UINT32 HRD_Dsaf_Sllc1Srst(UINT32 ulEn)
{
    SC_SLLC_RESET_REQ_U o_sc_sllc1_reset_req;
    SC_SLLC_RESET_DREQ_U o_sc_sllc1_reset_dreq;

    memset(&o_sc_sllc1_reset_req,0,sizeof(SC_SLLC_RESET_REQ_U));
    memset(&o_sc_sllc1_reset_dreq,0,sizeof(SC_SLLC_RESET_DREQ_U));

    if(0 == ulEn)
    {
         o_sc_sllc1_reset_req.bits.sllc_srst_req = 1;
         DSAF_SUB_WRITE_REG(DSAF_SUB_SC_SLLC_RESET_REQ_REG,0,o_sc_sllc1_reset_req.u32);
    }
    else
    {
        o_sc_sllc1_reset_dreq.bits.sllc_srst_dreq = 1;
        DSAF_SUB_WRITE_REG(DSAF_SUB_SC_SLLC_RESET_DREQ_REG,0,o_sc_sllc1_reset_dreq.u32);
    }

    return OS_SUCCESS;
}

/*****************************************************************************
 函 数 名  : HRD_Dsaf_IntMuxSet
 功能描述  : 中断选择发送到ITS还是M3
 输入参数  : UINT32 ulValue
 输出参数  : 无
 返 回 值  : UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月23日
    作    者   : yww
    修改内容   : 新生成函数

*****************************************************************************/
UINT32 HRD_Dsaf_IntMuxSet(UINT32 ulValue)
{
    DSAF_SUB_WRITE_REG(DSAF_SUB_SC_ITS_M3_INT_MUX_SEL_REG,0,(ulValue & 0x1F));
    return OS_SUCCESS;
}

#ifdef HRD_OS_LINUX
/*****************************************************************************
 函 数 名  : HRD_SubDsafInit
 功能描述  : SUB DSAF CRG linux初始化
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
UINT32 HRD_SubDsafInit(void)
{
    sub_dsaf_pa_addr = DSAF_SUB_BASE;

    /* SUB ALG申请io内存 */
    if (!request_mem_region(sub_dsaf_pa_addr, DSAF_SUB_BASE_SIZE, "SUB DSAF Reg"))
    {

        printk("SUB DSA region busy\n");

        return OS_ERROR;
    }

    sub_dsaf_base_addr = (UINT64)ioremap(sub_dsaf_pa_addr, DSAF_SUB_BASE_SIZE);
    if (!sub_dsaf_base_addr)
    {
        release_mem_region(sub_dsaf_pa_addr, DSAF_SUB_BASE_SIZE);
        printk("could not ioremap SUB DSA registers\n");

        return OS_ERROR;
    }

    return OS_SUCCESS;
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
void HRD_SubDsafExit(void)
{
    /* 释放SUB dsaf 资源 */
    if (sub_dsaf_base_addr > 0)
    {
        release_mem_region(sub_dsaf_pa_addr, DSAF_SUB_BASE_SIZE);
        iounmap((void *)sub_dsaf_base_addr);
    }

}

UINT64 HRD_SubDsafGetBase(void)
{
	return sub_dsaf_base_addr;
}

EXPORT_SYMBOL(HRD_Dsaf_SllcSrstByChannel);
EXPORT_SYMBOL(HRD_Dsaf_GeSrstByPort);
EXPORT_SYMBOL(HRD_Dsaf_XgeSrstByPort);
EXPORT_SYMBOL(HRD_Dsaf_PpeSrstByPort);
EXPORT_SYMBOL(HRD_Dsaf_XbarSrst);
EXPORT_SYMBOL(HRD_Dsaf_NtSrst);
EXPORT_SYMBOL(HRD_Dsaf_CpuSrst);
EXPORT_SYMBOL(HRD_Dsaf_RoceSrst);
EXPORT_SYMBOL(HRD_SubDsafGetBase);
EXPORT_SYMBOL(HRD_Dsaf_PpeComSrst);

#endif


