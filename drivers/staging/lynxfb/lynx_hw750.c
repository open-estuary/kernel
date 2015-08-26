#include<linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
#include<linux/config.h>
#endif
#include <linux/version.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/errno.h>
#include<linux/string.h>
#include<linux/mm.h>
#include<linux/slab.h>
#include<linux/delay.h>
#include<linux/fb.h>
#include<linux/ioport.h>
#include<linux/init.h>
#include<linux/pci.h>
#include<linux/vmalloc.h>
#include<linux/pagemap.h>
#include <linux/console.h>
#ifdef CONFIG_MTRR
#include <asm/mtrr.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
/* no below two header files in 2.6.9 */
#include<linux/platform_device.h>
#include<linux/screen_info.h>
#else
/* nothing by far */
#endif

#include "lynx_drv.h"
#include "lynx_hw750.h"
#include "ddk750/ddk750.h"
#include "lynx_accel.h"

int hw_sm750_map(struct lynx_share* share,struct pci_dev* pdev)
{
	int ret;
	struct sm750_share * spec_share;
	ENTER();

	spec_share = container_of(share,struct sm750_share,share);	
	ret = 0;	

	share->vidreg_start  = pci_resource_start(pdev,1);
	share->vidreg_size = MB(2);

	/* reserve the vidreg space of smi adaptor 
	 * if you do this, u need to add release region code
	 * in lynxfb_remove, or memory will not be mapped again 
	 * successfully
	 * */

#if 0
	if((ret = pci_request_region(pdev,1,_moduleName_)))
	{
		err_msg("Can not request PCI regions.\n");
		goto exit;
	}
#endif
	
	/* now map mmio and vidmem*/
	share->pvReg = ioremap_nocache(share->vidreg_start,share->vidreg_size);
	if(!share->pvReg){
		err_msg("mmio failed\n");
		ret = -EFAULT;
		goto exit;
	}

	share->accel.dprBase = share->pvReg + DE_BASE_ADDR_TYPE1;
	share->accel.dpPortBase = share->pvReg + DE_PORT_ADDR_TYPE1;

	ddk750_set_mmio(share->pvReg,share->venid,share->devid,share->revid);
	share->chiptype = getChipType();

	share->vidmem_start = pci_resource_start(pdev,0);
	/* don't use pdev_resource[x].end - resource[x].start to
	 * calculate the resource size,its only the maximum available
	 * size but not the actual size,use
	 * @hw_sm750_getVMSize function can be safe.
	 * */
	share->vidmem_size = hw_sm750_getVMSize(share);
	inf_msg("video memory size = %d mb\n",share->vidmem_size >> 20);

	/* reserve the vidmem space of smi adaptor */
#if 0
	if((ret = pci_request_region(pdev,0,_moduleName_)))
	{
		err_msg("Can not request PCI regions.\n");
		goto exit;
	}
#endif

	share->pvMem = ioremap(share->vidmem_start,
							share->vidmem_size);

	if(!share->pvMem){
		err_msg("Map video memory failed\n");
		ret = -EFAULT;
		goto exit;
	}

	inf_msg("video memory vaddr = %p\n",share->pvMem);
exit:	
	LEAVE(ret);
}



int hw_sm750_inithw(struct lynx_share* share,struct pci_dev * pdev)
{
	struct sm750_share * spec_share;	
	struct init_status * parm;
	logical_chip_type_t chiptype = getChipType();
	ENTER();
	spec_share = container_of(share,struct sm750_share,share);	
	parm = &spec_share->state.initParm;
	if(parm->chip_clk == 0){
		if(getChipType() >= SM750LE){
			parm->chip_clk = DEFAULT_SM750LE_CHIP_CLOCK;
		}else{
			parm->chip_clk = DEFAULT_SM750_CHIP_CLOCK;
		}
	}
						
	if(parm->mem_clk == 0)
		parm->mem_clk = parm->chip_clk;
	if(parm->master_clk == 0)
		parm->master_clk = parm->chip_clk/3;
		
	ddk750_initHw((initchip_param_t *)&spec_share->state.initParm);
	/* for sm718,open pci burst */
	if(share->devid == 0x718){
		POKE32(SYSTEM_CTRL,
				FIELD_SET(PEEK32(SYSTEM_CTRL),SYSTEM_CTRL,PCI_BURST,ON));
	}

	/* sm750 use sii164, it can be setup with default value 
	 * by on power, so initDVIDisp can be skipped */
#if 0	
	ddk750_initDVIDisp();
#endif

	if(chiptype < SM750LE)
	{
		/* does user need CRT ?*/
		if(spec_share->state.nocrt){
			POKE32(MISC_CTRL,
					FIELD_SET(PEEK32(MISC_CTRL),
					MISC_CTRL,
					DAC_POWER,OFF));
			/* shut off dpms */
			POKE32(SYSTEM_CTRL,
					FIELD_SET(PEEK32(SYSTEM_CTRL),
					SYSTEM_CTRL,
					DPMS,VNHN));
		}else{
			POKE32(MISC_CTRL,
					FIELD_SET(PEEK32(MISC_CTRL),
					MISC_CTRL,
					DAC_POWER,ON));
			/* turn on dpms */
			POKE32(SYSTEM_CTRL,
					FIELD_SET(PEEK32(SYSTEM_CTRL),
					SYSTEM_CTRL,
					DPMS,VPHP));
		}

		switch (spec_share->state.pnltype){
			case sm750_doubleTFT:
			case sm750_24TFT:
			case sm750_dualTFT:
			POKE32(PANEL_DISPLAY_CTRL,
				FIELD_VALUE(PEEK32(PANEL_DISPLAY_CTRL),
							PANEL_DISPLAY_CTRL,
							TFT_DISP,
							spec_share->state.pnltype));
			break;
		}
	}else if(chiptype == SM750LE){
		
		/* for 750LE ,no DVI chip initilization makes Monitor no signal */
		/* Set up GPIO for software I2C to program DVI chip in the 
		   Xilinx SP605 board, in order to have video signal.
		 */ 
        swI2CInit(0,1);
        /* Customer may NOT use CH7301 DVI chip, which has to be 
           initialized differently.
         */
        if (swI2CReadReg(0xec, 0x4a) == 0x95)
        {
            /* The following register values for CH7301 are from 
               Chrontel app note and our experiment.
             */
			inf_msg("yes,CH7301 DVI chip found\n");
            swI2CWriteReg(0xec, 0x1d, 0x16);
            swI2CWriteReg(0xec, 0x21, 0x9);
            swI2CWriteReg(0xec, 0x49, 0xC0);
			inf_msg("okay,CH7301 DVI chip setup done\n");
        }	
	}else{
		/* for SM750HS_F SM750HS SM750HS_A nothing to do*/
	}

	/* init 2d engine */
	if(!share->accel_off){
		hw_sm750_initAccel(share);
//		share->accel.de_wait = hw_sm750_deWait;
	}

	LEAVE(0);
}


resource_size_t hw_sm750_getVMSize(struct lynx_share * share)
{
	resource_size_t ret;
	ENTER();
	ret = ddk750_getVMSize();		
	LEAVE(ret);
}



int hw_sm750_output_checkMode(struct lynxfb_output* output,struct fb_var_screeninfo* var)
{
	ENTER();
	LEAVE(0);
}


int hw_sm750_output_setMode(struct lynxfb_output* output,
									struct fb_var_screeninfo* var,struct fb_fix_screeninfo* fix)
{
	int ret;
	disp_output_t dispSet;
	int channel;
	ENTER();
	ret = 0;
	dispSet = 0;
	channel = *output->channel;
	
	
	if(getChipType() < SM750LE){
		if(channel == sm750_primary){
			inf_msg("primary channel\n");
			if(output->paths & sm750_panel)
				dispSet |= do_LCD1_PRI;
			if(output->paths & sm750_crt)	
				dispSet |= do_CRT_PRI;

		}else{		
			inf_msg("secondary channel\n");
			if(output->paths & sm750_panel)
				dispSet |= do_LCD1_SEC;
			if(output->paths & sm750_crt)
				dispSet |= do_CRT_SEC;

		}	
		ddk750_setLogicalDispOut(dispSet);
	}else{
		/* just open DISPLAY_CONTROL_750LE register bit 3:0*/
		u32 reg;
		reg = PEEK32(DISPLAY_CONTROL_750LE);
		reg |= 0xf;
		POKE32(DISPLAY_CONTROL_750LE,reg);
	}

	inf_msg("ddk setlogicdispout done \n");
	LEAVE(ret);
}

void hw_sm750_output_clear(struct lynxfb_output* output)
{
	ENTER();
	LEAVE();
}

int hw_sm750_crtc_checkMode(struct lynxfb_crtc* crtc,struct fb_var_screeninfo* var)
{
	struct lynx_share * share;
	ENTER();

	share = container_of(crtc,struct lynxfb_par,crtc)->share;

	switch (var->bits_per_pixel){
		case 8:
		case 16:
			break;
		case 32:
			if(share->revid == (unsigned char)SM750LE_REVISION_ID){
				dbg_msg("750le do not support 32bpp\n");
				LEAVE (-EINVAL);
			}
			break;
		default:
			LEAVE(-EINVAL);
			
	}	
	
	LEAVE(0);
}


/*
	set the controller's mode for @crtc charged with @var and @fix parameters
*/
int hw_sm750_crtc_setMode(struct lynxfb_crtc* crtc,
								struct fb_var_screeninfo* var,
								struct fb_fix_screeninfo* fix)
{
	int ret,fmt;
	u32 reg;
	mode_parameter_t modparm;
	clock_type_t clock;
	struct lynx_share * share;
	struct lynxfb_par * par;
	
	ENTER();
	ret = 0;
	par = container_of(crtc,struct lynxfb_par,crtc);
	share = par->share;
#if 1
	if(!share->accel_off){
		/* set 2d engine pixel format according to mode bpp */
		switch(var->bits_per_pixel){
			case 8:
				fmt = 0;
				break;
			case 16:
				fmt = 1;
				break;
			case 32:
			default:
				fmt = 2;
				break;
		}
		hw_set2dformat(&share->accel,fmt);
	}
#endif

	/* set timing */
//	modparm.pixel_clock = PS_TO_HZ(var->pixclock);
	modparm.pixel_clock = ps_to_hz(var->pixclock);
	modparm.vertical_sync_polarity = (var->sync & FB_SYNC_HOR_HIGH_ACT) ? POS:NEG;
	modparm.horizontal_sync_polarity = (var->sync & FB_SYNC_VERT_HIGH_ACT) ? POS:NEG;
	modparm.clock_phase_polarity = (var->sync& FB_SYNC_COMP_HIGH_ACT) ? POS:NEG;			
	modparm.horizontal_display_end = var->xres;
	modparm.horizontal_sync_width = var->hsync_len;
	modparm.horizontal_sync_start = var->xres + var->right_margin;
	modparm.horizontal_total = var->xres + var->left_margin + var->right_margin + var->hsync_len;
	modparm.vertical_display_end = var->yres;
	modparm.vertical_sync_height = var->vsync_len;
	modparm.vertical_sync_start = var->yres + var->lower_margin;
	modparm.vertical_total = var->yres + var->upper_margin + var->lower_margin + var->vsync_len;

	/* choose pll */
	if(crtc->channel != sm750_secondary)
		clock = PRIMARY_PLL;
	else
		clock = SECONDARY_PLL;
	if(getChipType()>=SM750LE)
		clock = SECONDARY_PLL;
	
	dbg_msg("Request pixel clock = %lu\n",modparm.pixel_clock);
	ret = ddk750_setModeTiming(&modparm,clock);
	if(ret){
		err_msg("Set mode timing failed\n");
		goto exit;
	}
	if(getChipType()>=SM750LE)
		goto sm750crt;

	if(crtc->channel != sm750_secondary){
		/* set pitch, offset ,width,start address ,etc... */
		POKE32(PANEL_FB_ADDRESS,
			FIELD_SET(0,PANEL_FB_ADDRESS,STATUS,CURRENT)|
			FIELD_SET(0,PANEL_FB_ADDRESS,EXT,LOCAL)|
			FIELD_VALUE(0,PANEL_FB_ADDRESS,ADDRESS,crtc->oScreen));

		reg = var->xres * (var->bits_per_pixel >> 3);
		/* crtc->channel is not equal to par->index on numeric,be aware of that */
		reg = PADDING(crtc->line_pad,reg);

		POKE32(PANEL_FB_WIDTH,		
			FIELD_VALUE(0,PANEL_FB_WIDTH,WIDTH,reg)|
			FIELD_VALUE(0,PANEL_FB_WIDTH,OFFSET,fix->line_length));

		POKE32(PANEL_WINDOW_WIDTH,
			FIELD_VALUE(0,PANEL_WINDOW_WIDTH,WIDTH,var->xres -1)|
			FIELD_VALUE(0,PANEL_WINDOW_WIDTH,X,var->xoffset));

		POKE32(PANEL_WINDOW_HEIGHT,
			FIELD_VALUE(0,PANEL_WINDOW_HEIGHT,HEIGHT,var->yres_virtual - 1)|
			FIELD_VALUE(0,PANEL_WINDOW_HEIGHT,Y,var->yoffset));

		POKE32(PANEL_PLANE_TL,0);
		
		POKE32(PANEL_PLANE_BR,
			FIELD_VALUE(0,PANEL_PLANE_BR,BOTTOM,var->yres - 1)|
			FIELD_VALUE(0,PANEL_PLANE_BR,RIGHT,var->xres - 1));

		/* set pixel format */
		reg = PEEK32(PANEL_DISPLAY_CTRL);
		POKE32(PANEL_DISPLAY_CTRL,
			FIELD_VALUE(reg,
			PANEL_DISPLAY_CTRL,FORMAT,
			(var->bits_per_pixel >> 4)	    
			));
	}else
sm750crt:
	{	
		/* not implemented now */
		POKE32(CRT_FB_ADDRESS,crtc->oScreen);		
		reg = var->xres * (var->bits_per_pixel >> 3);
		/* crtc->channel is not equal to par->index on numeric,be aware of that */
		reg = PADDING(crtc->line_pad,reg);
		
		POKE32(CRT_FB_WIDTH,		
			FIELD_VALUE(0,CRT_FB_WIDTH,WIDTH,reg)|
			FIELD_VALUE(0,CRT_FB_WIDTH,OFFSET,fix->line_length));
		
		/* SET PIXEL FORMAT */
		reg = PEEK32(CRT_DISPLAY_CTRL);
		reg = FIELD_VALUE(reg,CRT_DISPLAY_CTRL,FORMAT,var->bits_per_pixel >> 4);
		POKE32(CRT_DISPLAY_CTRL,reg);
		
	}
	

exit:	
	LEAVE(ret);
}

void hw_sm750_crtc_clear(struct lynxfb_crtc* crtc)
{
	ENTER();
	LEAVE();
}

int hw_sm750_setColReg(struct lynxfb_crtc* crtc,ushort index,
								ushort red,ushort green,ushort blue)
{	
	static unsigned int add[]={PANEL_PALETTE_RAM,CRT_PALETTE_RAM};
	POKE32(add[crtc->channel] + index*4 ,(red<<16)|(green<<8)|blue);
	return 0;
}

int hw_sm750le_setBLANK(struct lynxfb_output * output,int blank){
	int dpms,crtdb;
	ENTER();
	switch(blank)
	{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
		case FB_BLANK_UNBLANK:			
#else
		case VESA_NO_BLANKING:
#endif
			dpms = CRT_DISPLAY_CTRL_DPMS_0;		
			crtdb = CRT_DISPLAY_CTRL_BLANK_OFF;
			break;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
		case FB_BLANK_NORMAL:
			dpms = CRT_DISPLAY_CTRL_DPMS_0;
			crtdb = CRT_DISPLAY_CTRL_BLANK_ON;
			break;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
		case FB_BLANK_VSYNC_SUSPEND:
#else
		case VESA_VSYNC_SUSPEND:
#endif
			dpms = CRT_DISPLAY_CTRL_DPMS_2;
			crtdb = CRT_DISPLAY_CTRL_BLANK_ON;
			break;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
		case FB_BLANK_HSYNC_SUSPEND:	
#else
		case VESA_HSYNC_SUSPEND:
#endif
			dpms = CRT_DISPLAY_CTRL_DPMS_1;
			crtdb = CRT_DISPLAY_CTRL_BLANK_ON;
			break;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
		case FB_BLANK_POWERDOWN:
#else
		case VESA_POWERDOWN:
#endif
			dpms = CRT_DISPLAY_CTRL_DPMS_3;
			crtdb = CRT_DISPLAY_CTRL_BLANK_ON;
			break;
	}
	
	if(output->paths & sm750_crt){	
		POKE32(CRT_DISPLAY_CTRL,FIELD_VALUE(PEEK32(CRT_DISPLAY_CTRL),CRT_DISPLAY_CTRL,DPMS,dpms));
		POKE32(CRT_DISPLAY_CTRL,FIELD_VALUE(PEEK32(CRT_DISPLAY_CTRL),CRT_DISPLAY_CTRL,BLANK,crtdb));
	}
	LEAVE(0);
}

int hw_sm750_setBLANK(struct lynxfb_output* output,int blank)
{
	unsigned int dpms,pps,crtdb;
	ENTER();
	dpms = pps = crtdb = 0;

	switch (blank)
	{		
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
		case FB_BLANK_UNBLANK:	
#else
		case VESA_NO_BLANKING:
#endif
			inf_msg("flag = FB_BLANK_UNBLANK \n");
			dpms = SYSTEM_CTRL_DPMS_VPHP;
			pps = PANEL_DISPLAY_CTRL_DATA_ENABLE;
			crtdb = CRT_DISPLAY_CTRL_BLANK_OFF;
			break;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
		case FB_BLANK_NORMAL:
			inf_msg("flag = FB_BLANK_NORMAL \n");
			dpms = SYSTEM_CTRL_DPMS_VPHP;
			pps = PANEL_DISPLAY_CTRL_DATA_DISABLE;
			crtdb = CRT_DISPLAY_CTRL_BLANK_ON;			
			break;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
		case FB_BLANK_VSYNC_SUSPEND:
#else
		case VESA_VSYNC_SUSPEND:
#endif
			dpms = SYSTEM_CTRL_DPMS_VNHP;
			pps = PANEL_DISPLAY_CTRL_DATA_DISABLE;
			crtdb = CRT_DISPLAY_CTRL_BLANK_ON;			
			break;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
		case FB_BLANK_HSYNC_SUSPEND:
#else
		case VESA_HSYNC_SUSPEND:
#endif
			dpms = SYSTEM_CTRL_DPMS_VPHN;
			pps = PANEL_DISPLAY_CTRL_DATA_DISABLE;
			crtdb = CRT_DISPLAY_CTRL_BLANK_ON;
			break;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
		case FB_BLANK_POWERDOWN:
#else
		case VESA_POWERDOWN:
#endif
			dpms = SYSTEM_CTRL_DPMS_VNHN;
			pps = PANEL_DISPLAY_CTRL_DATA_DISABLE;
			crtdb = CRT_DISPLAY_CTRL_BLANK_ON;				
			break;		
	}
		
	if(output->paths & sm750_crt){
	
		POKE32(SYSTEM_CTRL,FIELD_VALUE(PEEK32(SYSTEM_CTRL),SYSTEM_CTRL,DPMS,dpms));
		POKE32(CRT_DISPLAY_CTRL,FIELD_VALUE(PEEK32(CRT_DISPLAY_CTRL),CRT_DISPLAY_CTRL,BLANK,crtdb));
	}
	
	if(output->paths & sm750_panel){
		POKE32(PANEL_DISPLAY_CTRL,FIELD_VALUE(PEEK32(PANEL_DISPLAY_CTRL),PANEL_DISPLAY_CTRL,DATA,pps));
	}
	
	LEAVE(0);
}


void hw_sm750_initAccel(struct lynx_share * share)
{
	u32 reg;
	enable2DEngine(1);

	if(getChipType() >= SM750LE){
		reg = PEEK32(DE_STATE1);
		reg = FIELD_SET(reg,DE_STATE1,DE_ABORT,ON);
		POKE32(DE_STATE1,reg);

		reg = PEEK32(DE_STATE1);
		reg = FIELD_SET(reg,DE_STATE1,DE_ABORT,OFF);
		POKE32(DE_STATE1,reg);		
		
	}else{
		/* engine reset */	
		reg = PEEK32(SYSTEM_CTRL);
	    reg = FIELD_SET(reg,SYSTEM_CTRL,DE_ABORT,ON);
		POKE32(SYSTEM_CTRL,reg);

		reg = PEEK32(SYSTEM_CTRL);
		reg = FIELD_SET(reg,SYSTEM_CTRL,DE_ABORT,OFF);
		POKE32(SYSTEM_CTRL,reg);
	}

	/* call 2d init */
	share->accel.de_init(&share->accel);
}

int hw_sm750le_deWait()
{
	int i=0x10000000;
	while(i--){
		unsigned int dwVal = PEEK32(DE_STATE2);
		if((FIELD_GET(dwVal,DE_STATE2,DE_STATUS) == DE_STATE2_DE_STATUS_IDLE) &&
			(FIELD_GET(dwVal,DE_STATE2,DE_FIFO)  == DE_STATE2_DE_FIFO_EMPTY) &&
			(FIELD_GET(dwVal,DE_STATE2,DE_MEM_FIFO) == DE_STATE2_DE_MEM_FIFO_EMPTY))
		{
			return 0;
		}		
	}
	/* timeout error */
	return -1;
}


int hw_sm750_deWait()
{
	int i=0x10000000;
	while(i--){
		unsigned int dwVal = PEEK32(SYSTEM_CTRL);
		if((FIELD_GET(dwVal,SYSTEM_CTRL,DE_STATUS) == SYSTEM_CTRL_DE_STATUS_IDLE) &&
			(FIELD_GET(dwVal,SYSTEM_CTRL,DE_FIFO)  == SYSTEM_CTRL_DE_FIFO_EMPTY) &&
			(FIELD_GET(dwVal,SYSTEM_CTRL,DE_MEM_FIFO) == SYSTEM_CTRL_DE_MEM_FIFO_EMPTY))
		{
			return 0;
		}		
	}
	/* timeout error */
	return -1;
}
