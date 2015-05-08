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
#include "higgs_misc.h"
#include "higgs_peri.h"
#include "higgs_io.h"
#include "higgs_eh.h"
#include "higgs_init.h"
#include "higgs_dfm.h"
#include "higgs_dump.h"


/*----------------------------------------------*
 *  * outer variable type define                                *                                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 *    outer function type define                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * inner function type define                                *
 *----------------------------------------------*/
static s32 higgs_com_read_reg(struct higgs_card *ll_card, void *data);
static s32 higgs_com_write_reg(struct higgs_card *ll_card, void *data);
static s32 higgs_get_ll_rsc(struct sal_card *sal_card,
			    struct sal_show_rsc_option *rsc_option);
static s32 higgs_get_chip_info(struct higgs_card *ll_card);

/*----------------------------------------------*
 * global variable                                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * module  variable                                   *
 *----------------------------------------------*/




/*----------------------------------------------*
 * operate chip according to the chip type.                                   *
 *----------------------------------------------*/
s32 higgs_chip_operation(struct sal_card *sal_card,
			 enum sal_chip_op_type chip_op, void *data)
{
	s32 ret = ERROR;

	/* parameter check */
	HIGGS_ASSERT(NULL != sal_card, return ERROR);

	switch (chip_op) {
	case SAL_CHIP_OPT_SOFTRESET:
		ret = higgs_reset_chip(sal_card);
		break;
	case SAL_CHIP_OPT_ROUTE_CHECK:
		ret = higgs_chip_fatal_check(sal_card, data);
		break;
	case SAL_CHIP_OPT_LOOPBACK:
		ret = higgs_port_loopback(sal_card, data);
		break;
	case SAL_CHIP_OPT_CLOCK_CHECK:
		ret =
		    higgs_chip_clock_check(sal_card,
					   (enum sal_card_clock *)data);
		break;
	case SAL_CHIP_OPT_DUMP_LOG:
		ret = higgs_dump_info(sal_card, data);
		break;
	case SAL_CHIP_OPT_GET_VERSION:
		ret = higgs_get_chip_info(sal_card->drv_data);
		break;
	case SAL_CHIP_OPT_POWER_DOWN:
		ret = higgs_comm_pdprocess(sal_card->drv_data);
		break;
	case SAL_CHIP_OPT_MEM_TEST:
		 /*TBD*/ break;
	case SAL_CHIP_OPT_INTR_COAL:
	case SAL_CHIP_OPT_UPDATE:
	case SAL_CHIP_OPT_PCIE_LOOPBACK:
		HIGGS_TRACE(OSP_DBL_MAJOR, 4617,
			    "Card:%d Unsupported Chip opcode:%d",
			    sal_card->card_no, chip_op);
		ret = ERROR;
		break;
	case SAL_CHIP_OPT_REQ_TIMEOUTCHECK:
		higgs_req_timeout_check(sal_card);	/*detect req timeout release. */
		ret = OK;
		break;
	default:
		HIGGS_TRACE(OSP_DBL_MAJOR, 4617,
			    "Card:%d Invalid Chip opcode:%d", sal_card->card_no,
			    chip_op);
		ret = ERROR;
		break;
	}

	return ret;

}

/*----------------------------------------------*
 * read/write register interface function for low-layer .                                   *
 *----------------------------------------------*/
s32 higgs_reg_operation(struct sal_card * sal_card, enum sal_reg_op_type reg_op,
			void *data)
{
	s32 ret = ERROR;

	/* parameter check */
	HIGGS_ASSERT(NULL != sal_card, return ERROR);
	HIGGS_ASSERT(NULL != data, return ERROR);

	/* operation distribute*/
	switch (reg_op) {
	case SAL_REG_OPT_READ:
		ret = higgs_com_read_reg(sal_card->drv_data, data);
		break;
	case SAL_REG_OPT_WRITE:
		ret = higgs_com_write_reg(sal_card->drv_data, data);
		break;
	case SAL_REG_PCIE_OPT_READ:
	case SAL_REG_PCIE_OPT_WRITE:
		HIGGS_TRACE(OSP_DBL_MINOR, 4411,
			    "Card:%d Unsupported register opcode:%d",
			    sal_card->card_no, reg_op);
		ret = ERROR;
		break;
	case SAL_REG_OPT_BUTT:
	default:
		HIGGS_TRACE(OSP_DBL_MAJOR, 4411,
			    "Card:%d Invalid register opcode:%d",
			    sal_card->card_no, reg_op);
		ret = ERROR;
		break;
	}

	return ret;
}

/*----------------------------------------------*
 * low-layer public interface , for not classify operation .                             *
 *----------------------------------------------*/
s32 higgs_comm_ll_operation(struct sal_card * sal_card,
			    enum sal_common_op_type comm_op, void *data)
{
	s32 ret = ERROR;

	/* parameter check  */
	HIGGS_ASSERT(NULL != sal_card, return ERROR);
	HIGGS_ASSERT(NULL != data, return ERROR);

	/* operation distribute*/
	switch (comm_op) {
	case SAL_COMMON_OPT_SHOW_IO:
		break;
	case SAL_COMMON_OPT_GETLL_RSC:
		ret = higgs_get_ll_rsc(sal_card, data);
		break;
	case SAL_COMMON_OPT_DEBUG_LEVEL:
		break;
	case SAL_COMMON_OPT_GET_SFP_INFO:
		ret = higgs_get_sfp_info_intf(sal_card, data);
		break;
	case SAL_COMMON_OPT_READ_WIRE_EEPROM:
		ret = higgs_read_wire_eep_intf(sal_card, data);
		break;
	case SAL_COMMON_OPT_INQUIRY_CMDTX_STATUS:
		ret = higgs_chip_fatal_chkin_tm_timeout(sal_card, data);
		break;
	case SAL_COMMON_OPT_BUTT:
	default:
		HIGGS_TRACE(OSP_DBL_MAJOR, 4617, "Card:%d Invalid opcode:%d",
			    sal_card->card_no, comm_op);
		ret = ERROR;
		break;
	}

	return ret;
}

/*----------------------------------------------*
 * register write partially.                             *
 *----------------------------------------------*/
void higgs_reg_write_part(u64 reg, u32 low, u32 high, u32 value)
{
	//u64 real_reg = reg;
	u32 tmp = (u32) HIGGS_REG_READ_DWORD((void *)reg);
	tmp =
	    (tmp & (((1 << low) - 1))) | (tmp &
					  (~((u32) ((1 << (high + 1)) - 1))));
	tmp = tmp | (value << low);
	HIGGS_REG_WRITE_DWORD((void *)reg, tmp);
	return;
}

/*----------------------------------------------*
 * read register interface function for low-layer                             *
 *----------------------------------------------*/
static s32 higgs_com_read_reg(struct higgs_card *ll_card, void *data)
{
	struct sal_reg_op_param *reg_op;
	u32 bar;
	u32 phy_id;
	u32 reg_offset;
	u32 val;

	/* parameter check  */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	reg_op = (struct sal_reg_op_param *)data;
	HIGGS_ASSERT(NULL != reg_op, return ERROR);

	bar = reg_op->bar;
	reg_offset = reg_op->reg_offset;
	if (!(bar < HIGGS_MAX_PHY_NUM) && (HIGGS_BAR_GLOBAL != bar)) {
		HIGGS_TRACE(OSP_DBL_MAJOR, 151,
			    "Card:%d Invalid bar value:0x%x", ll_card->card_id,
			    bar);
		return ERROR;
	} else if ((reg_offset % 4) != 0) {	/* 4 bytes align */
		HIGGS_TRACE(OSP_DBL_MAJOR, 151,
			    "Card:%d Invalid register offset:0x%x",
			    ll_card->card_id, reg_offset);
		return ERROR;
	} else if ((bar < HIGGS_MAX_PHY_NUM) && !(reg_offset < HIGGS_PORT_REG_MAX_OFFSET)) {	/* port offset out of range */

		HIGGS_TRACE(OSP_DBL_MAJOR, 151,
			    "Card:%d Invalid port register offset:0x%x",
			    ll_card->card_id, reg_offset);
		return ERROR;
	} else if ((HIGGS_BAR_GLOBAL == bar) && !(reg_offset < HIGGS_GLOBAL_REG_MAX_OFFSET)) {	/* global offset out of range  */
		HIGGS_TRACE(OSP_DBL_MAJOR, 151,
			    "Card:%d Invalid global register offset:0x%x",
			    ll_card->card_id, reg_offset);
		return ERROR;
	}

	/* read register  */
	if (HIGGS_BAR_GLOBAL == bar) {	/* global register */
		val = HIGGS_GLOBAL_REG_READ(ll_card, reg_offset);
	} else {		/* port register */

		phy_id = bar;
		val =
		    HIGGS_PORT_REG_READ(ll_card, phy_id,
					(u64) HISAS30HV100_PORT_REG_BASE +
					reg_offset);
	}
	reg_op->val = val;

	return OK;

}

/*----------------------------------------------*
 * write register interface function for low-layer                             *
 *----------------------------------------------*/
static s32 higgs_com_write_reg(struct higgs_card *ll_card, void *data)
{
	struct sal_reg_op_param *reg_op;
	u32 bar;
	u32 phy_id;
	u32 reg_offset;
	u32 val;

	/* parameter check  */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);
	reg_op = (struct sal_reg_op_param *)data;
	HIGGS_ASSERT(NULL != reg_op, return ERROR);

	bar = reg_op->bar;
	reg_offset = reg_op->reg_offset;
	val = reg_op->val;
	if (!(bar < HIGGS_MAX_PHY_NUM) && (HIGGS_BAR_GLOBAL != bar)) {	/* port or global */

		HIGGS_TRACE(OSP_DBL_MAJOR, 151,
			    "Card:%d Invalid bar value:0x%x", ll_card->card_id,
			    bar);
		return ERROR;
	} else if (reg_offset % 4 != 0) {	/* 4 bytes align */

		HIGGS_TRACE(OSP_DBL_MAJOR, 151,
			    "Card:%d Invalid register offset:0x%x",
			    ll_card->card_id, reg_offset);
		return ERROR;
	} else if ((bar < HIGGS_MAX_PHY_NUM) && !(reg_offset < HIGGS_PORT_REG_MAX_OFFSET)) {	/* port offset out of range  */

		HIGGS_TRACE(OSP_DBL_MAJOR, 151,
			    "Card:%d Invalid port register offset:0x%x",
			    ll_card->card_id, reg_offset);
		return ERROR;
	} else if ((HIGGS_BAR_GLOBAL == bar) && !(reg_offset < HIGGS_GLOBAL_REG_MAX_OFFSET)) {	/* global offset out of range  */

		HIGGS_TRACE(OSP_DBL_MAJOR, 151,
			    "Card:%d Invalid global register offset:0x%x",
			    ll_card->card_id, reg_offset);
		return ERROR;
	}

	/*write register*/
	if (HIGGS_BAR_GLOBAL == bar) {	/* global register */

		HIGGS_GLOBAL_REG_WRITE(ll_card, reg_offset, val);
	} else {		/* port register */

		phy_id = bar;
		HIGGS_PORT_REG_WRITE(ll_card, phy_id,
				     (u64) HISAS30HV100_PORT_REG_BASE +
				     reg_offset, val);
	}

	return OK;
}

/*----------------------------------------------*
 * obtain resource of low-layer                             *
 *----------------------------------------------*/
static s32 higgs_get_ll_rsc(struct sal_card *sal_card,
			    struct sal_show_rsc_option *rsc_option)
{
	struct higgs_card *ll_card;

	/* parameter check  */
	HIGGS_ASSERT(sal_card != NULL, return ERROR);
	HIGGS_ASSERT(rsc_option != NULL, return ERROR);
	ll_card = (struct higgs_card *)sal_card->drv_data;

	/* TODO: take statistics of free req */
	HIGGS_REF(sal_card);
	HIGGS_REF(ll_card);
	HIGGS_REF(rsc_option);

	/* TODO: take statistics of free sgl */

	return ERROR;
}

/*----------------------------------------------*
 * obtain information of chip                             *
 *----------------------------------------------*/
static s32 higgs_get_chip_info(struct higgs_card *ll_card)
{
	/* parameter check  */
	HIGGS_ASSERT(NULL != ll_card, return ERROR);

	/* TODO: obtain chip model /version  */
	HIGGS_REF(ll_card);

	return OK;
}
