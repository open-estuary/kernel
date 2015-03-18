/*--------------------------------------------------------------------------------------------------------------------------*/
/*!!Warning: This is a key information asset of Huawei Tech Co.,Ltd                                                         */
/*CODEMARK:64z4jYnYa5t1KtRL8a/vnMxg4uGttU/wzF06xcyNtiEfsIe4UpyXkUSy93j7U7XZDdqx2rNx
p+25Dla32ZW7osA9Q1ovzSUNJmwD2Lwb8CS3jj1e4NXnh+7DT2iIAuYHJTrgjUqp838S0X3Y
kLe483srxy0NOL4THvTHEtjnJ71qMwcNKFQeWV8Q07Cdm5gG1IFfhliFcrjTpOPs+EjmdZ5n
fL6TlvM+GB0m6pzEXwHf9obi9JRYx9AMaHLc6IpSEtVf3xddYxmwR+sgGxoyyg==*/
/*--------------------------------------------------------------------------------------------------------------------------*/
/************************************************************************

  Hisilicon MDIO driver
  Copyright(c) 2014 - 2019 Huawei Corporation.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:TBD

************************************************************************/

#include "iware_error.h"
#include "iware_log.h"
#include "iware_mdio_hal.h"

/**
 *mdio_read_hw - read phy regs
 *@mdio_dev: mdio device
 *@phy_addr:phy addr
 *@is_c45:
 *@page:
 *@reg: reg
 *@data:regs data
 *return status
 */
static int mdio_read_hw(struct mdio_device *mdio_dev, u8 phy_addr,
			u8 is_c45, u8 page, u16 reg, u16 *data)
{
	union mdio_command_reg mdio_cmd_reg;
	union mdio_addr_reg mdio_addr_reg;
	union mdio_radata_reg mdio_rdata_reg;
	union mdio_sta_reg mdio_sta_reg;

	u32 time_cnt = MDIO_TIMEOUT;

	if (phy_addr > MDIO_MAX_PHY_ADDR) {
		log_err(mdio_dev->dev, "Wrong phy address: phy_addr(%x)\n",
			phy_addr);
		return HRD_COMMON_ERR_INPUT_INVALID;
	}

	log_dbg(mdio_dev->dev, "mdio(%d) base is %p\n",
		mdio_dev->gidx, mdio_dev->vbase);
	log_dbg(mdio_dev->dev,
		"phy_addr=%d, is_c45=%d, devad=%d, regnum=%#x!\n",
		phy_addr, is_c45, page, reg);

	/* Step 1; waitting for MDIO_COMMAND_REG 's mdio_start==0,
		after that can do read or wtrite*/
	for (time_cnt = MDIO_TIMEOUT; time_cnt; time_cnt--) {
		mdio_cmd_reg.u32 = MDIO_READ_REG(mdio_dev, MDIO_COMMAND_REG);
		if (!mdio_cmd_reg.bits.Mdio_Start)
			break;
	}
	if (mdio_cmd_reg.bits.Mdio_Start) {
		log_err(mdio_dev->dev,
			"MDIO is always busy! COMMAND_REG(%#x)\n",
			mdio_cmd_reg.u32);
		return HRD_COMMON_ERR_READ_FAIL;
	}

	if (!is_c45) {
		mdio_cmd_reg.bits.Mdio_St = MDIO_ST_CLAUSE_22;
		mdio_cmd_reg.bits.Mdio_Op = MDIO_C22_READ;
		mdio_cmd_reg.bits.Mdio_Prtad = (u16) phy_addr;
		mdio_cmd_reg.bits.Mdio_Devad = reg;
		mdio_cmd_reg.bits.Mdio_Start = 1;
		MDIO_WRITE_REG(mdio_dev, MDIO_COMMAND_REG, mdio_cmd_reg.u32);
		mdio_addr_reg.u32 = 0; 
		
		log_dbg(mdio_dev->dev, "is not c45!\n");
	} else {

		mdio_addr_reg.u32 = MDIO_READ_REG(mdio_dev, MDIO_ADDR_REG);
		mdio_addr_reg.bits.Mdio_Address = reg;
		MDIO_WRITE_REG(mdio_dev, MDIO_ADDR_REG, mdio_addr_reg.u32);

		/* Step 2; config the cmd-reg to write addr*/
		mdio_cmd_reg.bits.Mdio_St	 = MDIO_ST_CLAUSE_45;
		mdio_cmd_reg.bits.Mdio_Op	 = MDIO_C45_WRITE_ADDR;
		/* mdio_st==2'b00: is configing port addr*/
		mdio_cmd_reg.bits.Mdio_Prtad = (UINT16)phy_addr;
		/* mdio_st == 2'b00: is configing dev_addr*/
		mdio_cmd_reg.bits.Mdio_Devad = page;
		mdio_cmd_reg.bits.Mdio_Start = 1;
		MDIO_WRITE_REG(mdio_dev, MDIO_COMMAND_REG, mdio_cmd_reg.u32);

		/* Step 3; waitting for MDIO_COMMAND_REG 's mdio_start==0,
		check for read or write opt is finished */
		for (time_cnt = MDIO_TIMEOUT; time_cnt; time_cnt--) {
			mdio_cmd_reg.u32 = MDIO_READ_REG(mdio_dev, MDIO_COMMAND_REG);
			if (!mdio_cmd_reg.bits.Mdio_Start)
				break;
		}
		if (mdio_cmd_reg.bits.Mdio_Start) {
			log_err(mdio_dev->dev,
				"MDIO is always busy! COMMAND_REG(%#x)\n",
				mdio_cmd_reg.u32);
			return HRD_COMMON_ERR_READ_FAIL;
		}

		/* Step 4; config cmd-reg, send read opt */
		mdio_cmd_reg.bits.Mdio_St	 = MDIO_ST_CLAUSE_45;
		mdio_cmd_reg.bits.Mdio_Op	 = MDIO_C45_READ;
		/* mdio_st==2'b00: is configing port addr*/
		mdio_cmd_reg.bits.Mdio_Prtad = (UINT16)phy_addr;
		/* mdio_st == 2'b00: is configing dev_addr*/
		mdio_cmd_reg.bits.Mdio_Devad = page;
		mdio_cmd_reg.bits.Mdio_Start = 1;
		MDIO_WRITE_REG(mdio_dev, MDIO_COMMAND_REG, mdio_cmd_reg.u32);

		log_dbg(mdio_dev->dev, "is c45!\n");
	}

	/* Step 5; waitting for MDIO_COMMAND_REG 's mdio_start==0,
	check for read or write opt is finished */
	for (time_cnt = MDIO_TIMEOUT; time_cnt; time_cnt--) {
		mdio_cmd_reg.u32 = MDIO_READ_REG(mdio_dev, MDIO_COMMAND_REG);
		if (!mdio_cmd_reg.bits.Mdio_Start)
			break;
	}
	if (mdio_cmd_reg.bits.Mdio_Start) {
		log_err(mdio_dev->dev,
			"MDIO is always busy! COMMAND_REG(%#x) ADDR_REG(%#x)\n",
			mdio_cmd_reg.u32, mdio_addr_reg.u32);
		return HRD_COMMON_ERR_READ_FAIL;
	}

	mdio_sta_reg.u32 = MDIO_READ_REG(mdio_dev, MDIO_STA_REG);
	if (mdio_sta_reg.bits.Mdio_Sta) {
		log_err(mdio_dev->dev, " ERROR! MDIO Read failed!\n");
		return HRD_COMMON_ERR_READ_FAIL;
	}
	/* Step 6; get out data*/
	mdio_rdata_reg.u32 = MDIO_READ_REG(mdio_dev, MDIO_RDATA_REG);
	*data = mdio_rdata_reg.bits.Mdio_Rdata;

	return 0;
}

/**
 *mdio_write_hw - write phy regs
 *@mdio_dev: mdio device
 *@phy_addr:phy addr
 *@is_c45:
 *@page:
 *@reg: reg
 *@data:regs data
 *return status
 */
static int mdio_write_hw(struct mdio_device *mdio_dev, u8 phy_addr,
			 u8 is_c45, u8 page, u16 reg, u16 data)
{
	union mdio_command_reg mdio_cmd_reg;
	union mdio_wdata_reg mdio_wdata_reg;
	union mdio_addr_reg mdio_addr_reg;
	u32 time_cnt = MDIO_TIMEOUT;

	if (phy_addr > MDIO_MAX_PHY_ADDR) {
		log_err(mdio_dev->dev, "Wrong phy address: phy_addr(%x)\n",
			phy_addr);
		return HRD_COMMON_ERR_INPUT_INVALID;
	}

	
	log_dbg(mdio_dev->dev, "mdio(%d) base is %p,write data=%d\n",
		mdio_dev->gidx, mdio_dev->vbase, data);
	log_dbg(mdio_dev->dev, "phy_addr=%d, is_c45=%d, devad=%d, regnum=%#x!\n",
		phy_addr, is_c45, page, reg);

	/* Step 1; waitting for MDIO_COMMAND_REG 's mdio_start==0,
		after that can do read or wtrite*/
	for (time_cnt = MDIO_TIMEOUT; time_cnt; time_cnt--) {
		mdio_cmd_reg.u32= MDIO_READ_REG(mdio_dev, MDIO_COMMAND_REG);
		if (!mdio_cmd_reg.bits.Mdio_Start)
			break;
	}
	if (mdio_cmd_reg.bits.Mdio_Start) {
		log_err(mdio_dev->dev,
			"MDIO is always busy! COMMAND_REG(%#x)\n",
			mdio_cmd_reg.u32);
		return HRD_COMMON_ERR_WRITE_FAIL;
	}

	if (!is_c45) {
		mdio_wdata_reg.u32 = MDIO_READ_REG(mdio_dev, MDIO_WDATA_REG);
		mdio_wdata_reg.bits.Mdio_Wdata = data;
		MDIO_WRITE_REG(mdio_dev, MDIO_WDATA_REG, mdio_wdata_reg.u32);

		mdio_cmd_reg.bits.Mdio_St = MDIO_ST_CLAUSE_22;
		mdio_cmd_reg.bits.Mdio_Op = MDIO_C22_WRITE;
		mdio_cmd_reg.bits.Mdio_Prtad = (u16) phy_addr;

		mdio_cmd_reg.bits.Mdio_Devad = reg;
		mdio_cmd_reg.bits.Mdio_Start = 1;
		MDIO_WRITE_REG(mdio_dev, MDIO_COMMAND_REG, mdio_cmd_reg.u32);

		log_dbg(mdio_dev->dev, " is not c45!\n");
	} else {

		/* Step 2; config the cmd-reg to write addr*/
		mdio_addr_reg.u32 = MDIO_READ_REG(mdio_dev, MDIO_ADDR_REG);
		mdio_addr_reg.bits.Mdio_Address = reg;
		MDIO_WRITE_REG(mdio_dev, MDIO_ADDR_REG, mdio_addr_reg.u32);

		mdio_cmd_reg.bits.Mdio_St	 = MDIO_ST_CLAUSE_45;
		mdio_cmd_reg.bits.Mdio_Op	 = MDIO_C45_WRITE_ADDR;
		/* mdio_st==2'b00: is configing port addr*/
		mdio_cmd_reg.bits.Mdio_Prtad = (UINT16)phy_addr;
		/* mdio_st == 2'b00: is configing dev_addr*/
		mdio_cmd_reg.bits.Mdio_Devad = page;
		mdio_cmd_reg.bits.Mdio_Start = 1;
		MDIO_WRITE_REG(mdio_dev, MDIO_COMMAND_REG, mdio_cmd_reg.u32);

		/* Step 3; waitting for MDIO_COMMAND_REG 's mdio_start==0,
		check for read or write opt is finished */
		for (time_cnt = MDIO_TIMEOUT; time_cnt; time_cnt--) {
			mdio_cmd_reg.u32 = MDIO_READ_REG(mdio_dev, MDIO_COMMAND_REG);
			if (!mdio_cmd_reg.bits.Mdio_Start)
				break;
		}
		if (mdio_cmd_reg.bits.Mdio_Start) {
			log_err(mdio_dev->dev,
				"MDIO is always busy! COMMAND_REG(%#x)\n",
				mdio_cmd_reg.u32);
			return HRD_COMMON_ERR_WRITE_FAIL;
		}

		/* Step 4; config the data needed writing */
		mdio_wdata_reg.u32 = MDIO_READ_REG(mdio_dev, MDIO_WDATA_REG);
		mdio_wdata_reg.bits.Mdio_Wdata = data;
		MDIO_WRITE_REG(mdio_dev, MDIO_WDATA_REG, mdio_wdata_reg.u32);

		/* Step 5; config the cmd-reg for the write opt*/
		mdio_cmd_reg.bits.Mdio_St	 = MDIO_ST_CLAUSE_45;
		mdio_cmd_reg.bits.Mdio_Op	 = MDIO_C45_WRITE_DATA;
		/* mdio_st==2'b00: is configing port addr*/
		mdio_cmd_reg.bits.Mdio_Prtad = (UINT16)phy_addr;
		/* mdio_st == 2'b00: is configing dev_addr*/
		mdio_cmd_reg.bits.Mdio_Devad = page;
		mdio_cmd_reg.bits.Mdio_Start = 1;
		MDIO_WRITE_REG(mdio_dev, MDIO_COMMAND_REG, mdio_cmd_reg.u32);

		log_dbg(mdio_dev->dev, "is c45!\n");

	}

	return 0;
}

/**
 *mdio_set_ops - set mdio ops
 *@mdio_ops: mdio option
 */
void mdio_set_ops(struct mdio_ops *ops)
{
	ops->write_phy_reg = mdio_write_hw;
	ops->read_phy_reg = mdio_read_hw;
}
