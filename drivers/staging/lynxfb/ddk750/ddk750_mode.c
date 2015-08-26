
#include "ddk750_help.h"
#include "ddk750_reg.h"
#include "ddk750_mode.h"
#include "ddk750_chip.h"


/*
 *  Timing parameter for modes supported by SM750LE.
 *  Note that most timings in this table is made according to standard VESA 
 *  parameters.
 *  It is a subset of gDefaultModeParamTable[], all timings are copy from it.
 */
static mode_parameter_t gSM750LEModeParamTable[] =
{
/* 800 x 600  [4:3] */
/* The first 2 commented lines below are taken from SM502, the rest timing are
   taken from the VESA Monitor Timing Standard */
/* {1062, 800, 840,128, POS, 628, 600, 601, 4, POS, 40000000, 37665, 60, NEG}, */
/* {1054, 800, 842, 64, POS, 625, 600, 601, 3, POS, 56000000, 53131, 85, NEG}, */
 {1056, 800, 840,128, POS, 628, 600, 601, 4, POS, 40000000, 37879, 60, POS},

/* 1024 x 768  [4:3] */
/* The first 2 commented lines below are taken from SM502, the rest timing are
   taken from the VESA Monitor Timing Standard */
/* {1340,1024,1060,136, NEG, 809, 768, 772, 6, NEG, 65000000, 48507, 60, NEG}, */
/* {1337,1024,1072, 96, NEG, 808, 768, 780, 3, NEG, 81000000, 60583, 75, NEG}, */
 {1344,1024,1048,136, NEG, 806, 768, 771, 6, NEG, 65000000, 48363, 60, NEG},
  
/* 1152 x 864  [4:3] -- Widescreen eXtended Graphics Array */
/* {1475,1152,1208, 96, NEG, 888, 864, 866, 3, NEG, 78600000, 53288, 60, NEG},*/
 {1475,1152,1208, 96, POS, 888, 864, 866, 3, POS, 78600000, 53288, 60, NEG},
 
/* 1280 x 720  [16:9] -- HDTV (WXGA) */
 {1664,1280,1336,136, POS, 746, 720, 721, 3, POS, 74481000, 44760, 60, NEG},

/* 1280 x 768  [5:3] -- Not a popular mode */
 {1678,1280,1350,136, POS, 795, 768, 769, 3, POS, 80000000, 47676, 60, NEG},

/* 1280 x 960  [4:3] */
/* The first commented line below are taken from SM502, the rest timing are
   taken from the VESA Monitor Timing Standard */
/* {1618,1280,1330, 96, NEG, 977, 960, 960, 2, NEG, 94500000, 59259, 60, NEG},*/
 {1800,1280,1376,112, POS,1000, 960, 961, 3, POS,108000000, 60000, 60, NEG},
    
/* 1280 x 1024 [5:4] */
#if 1
 /* GTF with C = 40, M = 600, J = 20, K = 128 */
 {1712,1280,1360,136, NEG,1060,1024,1025, 3, POS,108883200, 63600, 60, NEG},
#else
 /* VESA Standard */
 {1688,1280,1328,112, POS,1066,1024,1025, 3, POS,108000000, 63981, 60, NEG},
#endif

/* End of table. */
 { 0, 0, 0, 0, NEG, 0, 0, 0, 0, NEG, 0, 0, 0, NEG},
};

/*
 *  Timing parameter for modes supported by SM750HS.
 *  Note that most timings in this table is made according to standard VESA 
 *  parameters.
 *  It is a subset of gDefaultModeParamTable[], all timings are copy from it.
 */
static mode_parameter_t gSM750HSModeParamTable[] =
{
/* 640 x 480  [4:3] */
 { 800, 640, 656, 96, NEG, 525, 480, 490, 2, NEG, 25175000, 31469, 60, NEG},

/* 800 x 600  [4:3] */
/* The first 2 commented lines below are taken from SM502, the rest timing are 
   taken from the VESA Monitor Timing Standard */
/* {1062, 800, 840,128, POS, 628, 600, 601, 4, POS, 40000000, 37665, 60, NEG}, */
/* {1054, 800, 842, 64, POS, 625, 600, 601, 3, POS, 56000000, 53131, 85, NEG}, */
 {1056, 800, 840,128, POS, 628, 600, 601, 4, POS, 40000000, 37879, 60, POS},

/* 1024 x 768  [4:3] */
/* The first 2 commented lines below are taken from SM502, the rest timing are
   taken from the VESA Monitor Timing Standard */
/* {1340,1024,1060,136, NEG, 809, 768, 772, 6, NEG, 65000000, 48507, 60, NEG}, */
/* {1337,1024,1072, 96, NEG, 808, 768, 780, 3, NEG, 81000000, 60583, 75, NEG}, */
 {1344,1024,1048,136, NEG, 806, 768, 771, 6, NEG, 65000000, 48363, 60, NEG},
  
/* 1152 x 864  [4:3] -- Widescreen eXtended Graphics Array */
/* {1475,1152,1208, 96, NEG, 888, 864, 866, 3, NEG, 78600000, 53288, 60, NEG},*/
 {1475,1152,1208, 96, POS, 888, 864, 866, 3, POS, 78600000, 53288, 60, NEG},
 
/* 1280 x 720  [16:9] -- HDTV (WXGA) */
 {1664,1280,1336,136, POS, 746, 720, 721, 3, POS, 74481000, 44760, 60, NEG},

/* 1280 x 768  [5:3] -- Not a popular mode */
 {1678,1280,1350,136, POS, 795, 768, 769, 3, POS, 80000000, 47676, 60, NEG},

/* 1280 x 960  [4:3] */
/* The first commented line below are taken from SM502, the rest timing are
   taken from the VESA Monitor Timing Standard */
/* {1618,1280,1330, 96, NEG, 977, 960, 960, 2, NEG, 94500000, 59259, 60, NEG},*/
 {1800,1280,1376,112, POS,1000, 960, 961, 3, POS,108000000, 60000, 60, NEG},
    
/* 1280 x 1024 [5:4] */
#if 1
 /* GTF with C = 40, M = 600, J = 20, K = 128 */
 {1712,1280,1360,136, NEG,1060,1024,1025, 3, POS,108883200, 63600, 60, NEG},
#else
 /* VESA Standard */
 {1688,1280,1328,112, POS,1066,1024,1025, 3, POS,108000000, 63981, 60, NEG},
#endif

/* 1600 x 1200 [4:3]. -- Ultra eXtended Graphics Array */
 /* VESA */
 {2160,1600,1664,192, POS,1250,1200,1201, 3, POS,162000000, 75000, 60, NEG},

/* 1920 x 1080 [16:9]. This is a make-up value, need to be proven. 
   The Pixel clock is calculated based on the maximum resolution of
   "Single Link" DVI, which support a maximum 165MHz pixel clock.
   The second values are taken from:
   http://www.tek.com/Measurement/App_Notes/25_14700/eng/25W_14700_3.pdf
 */
 {2200,1920,2008, 44, NEG,1125,1080,1081, 3, POS,148500000, 67500, 60, NEG},

/* 1920 x 1200 [8:5]. -- Widescreen Ultra eXtended Graphics Array (WUXGA) */
 {2592,1920,2048,208, NEG,1242,1200,1201, 3, POS,193160000, 74522, 60, NEG},

/* End of table. */
 { 0, 0, 0, 0, NEG, 0, 0, 0, 0, NEG, 0, 0, 0, NEG},
};

/*
 *  Timing parameter for modes supported by SM750HS_F
 *  Note that most timings in this table is made according to standard VESA 
 *  parameters.
 *  It is a subset of gDefaultModeParamTable[], all timings are copy from it.
 */
static mode_parameter_t gSM750HS_F_ModeParamTable[] =
{
/* 640 x 480  [4:3] */
 { 800, 640, 656, 96, NEG, 525, 480, 490, 2, NEG, 25175000, 31469, 60, NEG},

/* 800 x 600  [4:3] */
/* The first 2 commented lines below are taken from SM502, the rest timing are 
   taken from the VESA Monitor Timing Standard */
/* {1062, 800, 840,128, POS, 628, 600, 601, 4, POS, 40000000, 37665, 60, NEG}, */
/* {1054, 800, 842, 64, POS, 625, 600, 601, 3, POS, 56000000, 53131, 85, NEG}, */
 {1056, 800, 840,128, POS, 628, 600, 601, 4, POS, 40000000, 37879, 60, POS},

/* 1024 x 768  [4:3] */
/* The first 2 commented lines below are taken from SM502, the rest timing are
   taken from the VESA Monitor Timing Standard */
/* {1340,1024,1060,136, NEG, 809, 768, 772, 6, NEG, 65000000, 48507, 60, NEG}, */
/* {1337,1024,1072, 96, NEG, 808, 768, 780, 3, NEG, 81000000, 60583, 75, NEG}, */
 {1344,1024,1048,136, NEG, 806, 768, 771, 6, NEG, 65000000, 48363, 60, NEG},
  
/* 1152 x 864  [4:3] -- Widescreen eXtended Graphics Array */
/* {1475,1152,1208, 96, NEG, 888, 864, 866, 3, NEG, 78600000, 53288, 60, NEG},*/
 {1475,1152,1208, 96, POS, 888, 864, 866, 3, POS, 78600000, 53288, 60, NEG},
 
/* 1280 x 720  [16:9] -- HDTV (WXGA) */
 {1650,1280,1390,40, POS, 750, 720, 725, 5, POS, 74250000, 45000, 60, NEG},
 //{1664,1280,1336,136, POS, 746, 720, 721, 3, POS, 74481000, 44760, 60, NEG},

/* 1280 x 768  [5:3] -- Not a popular mode */
 {1664,1280,1344,128, POS, 798, 768, 771, 7, POS, 79500000, 47776, 60, NEG},
 //{1678,1280,1350,136, POS, 795, 768, 769, 3, POS, 80000000, 47676, 60, NEG},

/* 1280 x 960  [4:3] */
/* The first commented line below are taken from SM502, the rest timing are
   taken from the VESA Monitor Timing Standard */
/* {1618,1280,1330, 96, NEG, 977, 960, 960, 2, NEG, 94500000, 59259, 60, NEG},*/
 {1800,1280,1376,112, POS,1000, 960, 961, 3, POS,108000000, 60000, 60, NEG},
    
/* 1280 x 1024 [5:4] */
#if 1
 /* GTF with C = 40, M = 600, J = 20, K = 128 */
 {1688,1280,1328,112, NEG,1066,1024,1025, 3, POS,108000000, 63900, 60, NEG},
 //{1712,1280,1360,136, NEG,1060,1024,1025, 3, POS,108883200, 63600, 60, NEG},
#else
 /* VESA Standard */
 {1688,1280,1328,112, POS,1066,1024,1025, 3, POS,108000000, 63981, 60, NEG},
#endif

/* 1600 x 1200 [4:3]. -- Ultra eXtended Graphics Array */
 /* VESA */
 {2160,1600,1664,192, POS,1250,1200,1201, 3, POS,162000000, 75000, 60, NEG},

/* 1920 x 1080 [16:9]. This is a make-up value, need to be proven. 
   The Pixel clock is calculated based on the maximum resolution of
   "Single Link" DVI, which support a maximum 165MHz pixel clock.
   The second values are taken from:
   http://www.tek.com/Measurement/App_Notes/25_14700/eng/25W_14700_3.pdf
 */
 {2200,1920,2008, 44, POS,1125,1080,1084, 5, POS,148500000, 67500, 60, NEG},
 //{2200,1920,2008, 44, NEG,1125,1080,1081, 3, POS,148500000, 67500, 60, NEG},

/* 1920 x 1200 [8:5]. -- Widescreen Ultra eXtended Graphics Array (WUXGA) */
 {2592,1920,2048,208, NEG,1242,1200,1201, 3, POS,193160000, 74522, 60, NEG},

/* End of table. */
 { 0, 0, 0, 0, NEG, 0, 0, 0, 0, NEG, 0, 0, 0, NEG},
};


/*
	SM750LE only: 
    This function takes care extra registers and bit fields required to set
    up a mode in SM750LE

	Explanation about Display Control register:
    HW only supports 7 predefined pixel clocks, and clock select is 
    in bit 29:27 of	Display Control register.
*/
static unsigned long displayControlAdjust_SM750LE(mode_parameter_t *pModeParam, unsigned long dispControl)
{
	unsigned long x, y;

	x = pModeParam->horizontal_display_end;
	y = pModeParam->vertical_display_end;

    /* SM750LE has to set up the top-left and bottom-right
       registers as well.
       Note that normal SM750/SM718 only use those two register for
       auto-centering mode.
    */
    POKE32(CRT_AUTO_CENTERING_TL,
      FIELD_VALUE(0, CRT_AUTO_CENTERING_TL, TOP, 0)
    | FIELD_VALUE(0, CRT_AUTO_CENTERING_TL, LEFT, 0));

    POKE32(CRT_AUTO_CENTERING_BR, 
      FIELD_VALUE(0, CRT_AUTO_CENTERING_BR, BOTTOM, y-1)
    | FIELD_VALUE(0, CRT_AUTO_CENTERING_BR, RIGHT, x-1));

    /* Assume common fields in dispControl have been properly set before
       calling this function.
       This function only sets the extra fields in dispControl.
    */

	/* Clear bit 29:27 of display control register */
    dispControl &= FIELD_CLEAR(CRT_DISPLAY_CTRL, CLK);

	/* Set bit 29:27 of display control register for the right clock */
	/* Note that SM750LE only need to supported 7 resoluitons. */
	if ( x == 800 && y == 600 )
    	dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, CLK, PLL41);
	else if (x == 1024 && y == 768)
    	dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, CLK, PLL65);
	else if (x == 1152 && y == 864)
    	dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, CLK, PLL80);
	else if (x == 1280 && y == 768)
    	dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, CLK, PLL80);
	else if (x == 1280 && y == 720)
    	dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, CLK, PLL74);
	else if (x == 1280 && y == 960)
    	dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, CLK, PLL108);
	else if (x == 1280 && y == 1024)
    	dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, CLK, PLL108);
	else /* default to VGA clock */
    	dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, CLK, PLL25);

	/* Set bit 25:24 of display controller */
    dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, CRTSELECT, CRT);
    dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, RGBBIT, 24BIT);

    /* Set bit 14 of display controller */
    dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, CLOCK_PHASE, ACTIVE_LOW);

    POKE32(CRT_DISPLAY_CTRL, dispControl);

	return dispControl;
}


/*
 * 	SM750HS SMI FPGA:
 * 	This function takes care the extra registers and bit fields required to set
 * 	up a mode in SM750HS FPGA board.
 *
 *  Explanation about Display Control register:
 * 	FPGA only supports 7 predefined pixel clocks, and clock select is 
 * 	in bit 4:0 of new register 0x802a8.
 * 	
 */

unsigned long displayControlAdjust_SM750HS(mode_parameter_t *pModeParam, unsigned long dispControl)
{
	unsigned long x, y;
	unsigned long pll1; /* bit[31:0] of PLL */
	unsigned long pll2; /* bit[63:32] of PLL */

	x = pModeParam->horizontal_display_end;
	y = pModeParam->vertical_display_end;

	/* SM750HS has to set up a new register for PLL control (CRT_PLL1_750HS & CRT_PLL2_750HS).
	   The values to set are different for ASIC and FPGA verification.
	*/
	if ( x == 800 && y == 600 )
		{ pll1 = CRT_PLL1_750HS_40MHZ; pll2 = CRT_PLL2_750HS_40MHZ;}
	else if (x == 1024 && y == 768)
		{ pll1 = CRT_PLL1_750HS_65MHZ; pll2 = CRT_PLL2_750HS_65MHZ;}
	else if (x == 1152 && y == 864)
		{ pll1 = CRT_PLL1_750HS_80MHZ; pll2 = CRT_PLL2_750HS_80MHZ;}
	else if (x == 1280 && y == 768)
		{ pll1 = CRT_PLL1_750HS_80MHZ; pll2 = CRT_PLL2_750HS_80MHZ;}
	else if (x == 1280 && y == 720)
		{ pll1 = CRT_PLL1_750HS_74MHZ; pll2 = CRT_PLL2_750HS_74MHZ;}
	else if (x == 1280 && y == 960)
		{ pll1 = CRT_PLL1_750HS_108MHZ; pll2 = CRT_PLL2_750HS_108MHZ;}
	else if (x == 1280 && y == 1024)
		{ pll1 = CRT_PLL1_750HS_108MHZ; pll2 = CRT_PLL2_750HS_108MHZ;}
	else if (x == 1600 && y == 1200)
		{ pll1 = CRT_PLL1_750HS_162MHZ; pll2 = CRT_PLL2_750HS_162MHZ;}
	else if (x == 1920 && y == 1080)
		{ pll1 = CRT_PLL1_750HS_148MHZ; pll2 = CRT_PLL2_750HS_148MHZ;}
	else if (x == 1920 && y == 1200)
		{ pll1 = CRT_PLL1_750HS_193MHZ; pll2 = CRT_PLL2_750HS_193MHZ;}
	else /* default to VGA clock */
		{ pll1 = CRT_PLL1_750HS_25MHZ; pll2 = CRT_PLL2_750HS_25MHZ;}

    POKE32(CRT_PLL1_750HS, pll1);
    POKE32(CRT_PLL2_750HS, pll2);

    /* SM750LE and HS has to set up the top-left and bottom-right
       registers as well.
       Note that normal SM750/SM718 only use those two register for
       auto-centering mode.
    */
    POKE32(CRT_AUTO_CENTERING_TL,
      FIELD_VALUE(0, CRT_AUTO_CENTERING_TL, TOP, 0)
    | FIELD_VALUE(0, CRT_AUTO_CENTERING_TL, LEFT, 0));

    POKE32(CRT_AUTO_CENTERING_BR, 
      FIELD_VALUE(0, CRT_AUTO_CENTERING_BR, BOTTOM, y-1)
    | FIELD_VALUE(0, CRT_AUTO_CENTERING_BR, RIGHT, x-1));

    /* Assume common fields in dispControl have been properly set before
       calling this function.
       This function only sets the extra fields in dispControl.
    */

	/* Set bit 25 of display controller: Select CRT or VGA clock */
    dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, CRTSELECT, CRT);

    /* Set bit 14 of display controller */
    dispControl &= FIELD_CLEAR(CRT_DISPLAY_CTRL, CLOCK_PHASE);
    if (pModeParam->clock_phase_polarity == NEG)
        dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, CLOCK_PHASE, ACTIVE_LOW);
    else
        dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, CLOCK_PHASE, ACTIVE_HIGH);

    POKE32(CRT_DISPLAY_CTRL, dispControl);

	return dispControl;
}

/*
	SM750HS HIS FPGA:
    This function takes care the extra registers and bit fields required to set
    up a mode in SM750HS FPGA board.

	Explanation about Display Control register:
    FPGA only supports 7 predefined pixel clocks, and clock select is 
    in bit 4:0 of new register 0x802a8.
*/
unsigned long displayControlAdjust_SM750HS_F(mode_parameter_t *pModeParam, unsigned long dispControl)
{
	unsigned long x, y;
	unsigned long pll1; /* bit[31:0] of PLL */
	unsigned long pll2; /* bit[63:32] of PLL */

	x = pModeParam->horizontal_display_end;
	y = pModeParam->vertical_display_end;

	/* SM750HS_F has to set up a new register for PLL control (CRT_PLL1_750HS & CRT_PLL2_750HS).
	   The values to set are different for ASIC and FPGA verification.
	*/
	if ( x == 800 && y == 600 )
		{ pll1 = CRT_PLL1_750HS_F_40MHZ; pll2 = CRT_PLL2_750HS_F_40MHZ;}
	else if (x == 1024 && y == 768)
		{ pll1 = CRT_PLL1_750HS_F_65MHZ; pll2 = CRT_PLL2_750HS_F_65MHZ;}
	else if (x == 1152 && y == 864)
		{ pll1 = CRT_PLL1_750HS_F_80MHZ_1152; pll2 = CRT_PLL2_750HS_F_80MHZ;}
	else if (x == 1280 && y == 768)
		{ pll1 = CRT_PLL1_750HS_F_80MHZ; pll2 = CRT_PLL2_750HS_F_80MHZ;}
	else if (x == 1280 && y == 720)
		{ pll1 = CRT_PLL1_750HS_F_74MHZ; pll2 = CRT_PLL2_750HS_F_74MHZ;}
	else if (x == 1280 && y == 960)
		{ pll1 = CRT_PLL1_750HS_F_108MHZ; pll2 = CRT_PLL2_750HS_F_108MHZ;}
	else if (x == 1280 && y == 1024)
		{ pll1 = CRT_PLL1_750HS_F_108MHZ; pll2 = CRT_PLL2_750HS_F_108MHZ;}
	else if (x == 1600 && y == 1200)
		{ pll1 = CRT_PLL1_750HS_F_162MHZ; pll2 = CRT_PLL2_750HS_F_162MHZ;}
	else if (x == 1920 && y == 1080)
		{ pll1 = CRT_PLL1_750HS_F_148MHZ; pll2 = CRT_PLL2_750HS_F_148MHZ;}
	else if (x == 1920 && y == 1200)
		{ pll1 = CRT_PLL1_750HS_F_193MHZ; pll2 = CRT_PLL2_750HS_F_193MHZ;}
	else /* default to VGA clock */
		{ pll1 = CRT_PLL1_750HS_F_25MHZ; pll2 = CRT_PLL2_750HS_F_25MHZ;}

//    POKE32(CRT_PLL1_750HS, pll1);
    POKE32(CRT_PLL2_750HS, pll2);
    setVclock_HiSilicon(pll1);


    /* SM750LE and HS has to set up the top-left and bottom-right
       registers as well.
       Note that normal SM750/SM718 only use those two register for
       auto-centering mode.
    */
    POKE32(CRT_AUTO_CENTERING_TL,
      FIELD_VALUE(0, CRT_AUTO_CENTERING_TL, TOP, 0)
    | FIELD_VALUE(0, CRT_AUTO_CENTERING_TL, LEFT, 0));

    POKE32(CRT_AUTO_CENTERING_BR, 
      FIELD_VALUE(0, CRT_AUTO_CENTERING_BR, BOTTOM, y-1)
    | FIELD_VALUE(0, CRT_AUTO_CENTERING_BR, RIGHT, x-1));

    /* Assume common fields in dispControl have been properly set before
       calling this function.
       This function only sets the extra fields in dispControl.
    */

	/* Set bit 25 of display controller: Select CRT or VGA clock */
    dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, CRTSELECT, CRT);

    /* Set bit 14 of display controller */
    dispControl &= FIELD_CLEAR(CRT_DISPLAY_CTRL, CLOCK_PHASE);
    if (pModeParam->clock_phase_polarity == NEG)
        dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, CLOCK_PHASE, ACTIVE_LOW);
    else
        dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, CLOCK_PHASE, ACTIVE_HIGH);

    POKE32(CRT_DISPLAY_CTRL, dispControl);

	return dispControl;
}

/*
	SM750HS HIS ASIC:
    This function takes care the extra registers and bit fields required to set
    up a mode in SM750HS FPGA board.

	Explanation about Display Control register:
    FPGA only supports 7 predefined pixel clocks, and clock select is 
    in bit 4:0 of new register 0x802a8.
*/
unsigned long displayControlAdjust_SM750HS_A(mode_parameter_t *pModeParam, unsigned long dispControl)
{
	unsigned long x, y;
	unsigned long pll1; /* bit[31:0] of PLL */
	unsigned long pll2; /* bit[63:32] of PLL */

	x = pModeParam->horizontal_display_end;
	y = pModeParam->vertical_display_end;

	/* SM750HS_A has to set up a new register for PLL control (CRT_PLL1_750HS & CRT_PLL2_750HS).
	   The values to set are different for ASIC and FPGA verification.
	*/
	if ( x == 800 && y == 600 )
		{ pll1 = CRT_PLL1_750HS_A_40MHZ; pll2 = CRT_PLL2_750HS_A_40MHZ;}
	else if (x == 1024 && y == 768)
		{ pll1 = CRT_PLL1_750HS_A_65MHZ; pll2 = CRT_PLL2_750HS_A_65MHZ;}
	else if (x == 1152 && y == 864)
		{ pll1 = CRT_PLL1_750HS_A_80MHZ; pll2 = CRT_PLL2_750HS_A_80MHZ;}
	else if (x == 1280 && y == 768)
		{ pll1 = CRT_PLL1_750HS_A_80MHZ; pll2 = CRT_PLL2_750HS_A_80MHZ;}
	else if (x == 1280 && y == 720)
		{ pll1 = CRT_PLL1_750HS_A_74MHZ; pll2 = CRT_PLL2_750HS_A_74MHZ;}
	else if (x == 1280 && y == 960)
		{ pll1 = CRT_PLL1_750HS_A_108MHZ; pll2 = CRT_PLL2_750HS_A_108MHZ;}
	else if (x == 1280 && y == 1024)
		{ pll1 = CRT_PLL1_750HS_A_108MHZ; pll2 = CRT_PLL2_750HS_A_108MHZ;}
	else if (x == 1600 && y == 1200)
		{ pll1 = CRT_PLL1_750HS_A_162MHZ; pll2 = CRT_PLL2_750HS_A_162MHZ;}
	else if (x == 1920 && y == 1080)
		{ pll1 = CRT_PLL1_750HS_A_148MHZ; pll2 = CRT_PLL2_750HS_A_148MHZ;}
	else if (x == 1920 && y == 1200)
		{ pll1 = CRT_PLL1_750HS_A_193MHZ; pll2 = CRT_PLL2_750HS_A_193MHZ;}
	else /* default to VGA clock */
		{ pll1 = CRT_PLL1_750HS_A_25MHZ; pll2 = CRT_PLL2_750HS_A_25MHZ;}

    POKE32(CRT_PLL1_750HS, pll1);
    POKE32(CRT_PLL2_750HS, pll2);

    /* SM750LE and HS has to set up the top-left and bottom-right
       registers as well.
       Note that normal SM750/SM718 only use those two register for
       auto-centering mode.
    */
    POKE32(CRT_AUTO_CENTERING_TL,
      FIELD_VALUE(0, CRT_AUTO_CENTERING_TL, TOP, 0)
    | FIELD_VALUE(0, CRT_AUTO_CENTERING_TL, LEFT, 0));

    POKE32(CRT_AUTO_CENTERING_BR, 
      FIELD_VALUE(0, CRT_AUTO_CENTERING_BR, BOTTOM, y-1)
    | FIELD_VALUE(0, CRT_AUTO_CENTERING_BR, RIGHT, x-1));

    /* Assume common fields in dispControl have been properly set before
       calling this function.
       This function only sets the extra fields in dispControl.
    */

	/* Set bit 25 of display controller: Select CRT or VGA clock */
    dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, CRTSELECT, CRT);

    /* Set bit 14 of display controller */
    dispControl &= FIELD_CLEAR(CRT_DISPLAY_CTRL, CLOCK_PHASE);
    if (pModeParam->clock_phase_polarity == NEG)
        dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, CLOCK_PHASE, ACTIVE_LOW);
    else
        dispControl = FIELD_SET(dispControl, CRT_DISPLAY_CTRL, CLOCK_PHASE, ACTIVE_HIGH);

    POKE32(CRT_DISPLAY_CTRL, dispControl);

	return dispControl;
}





/* only timing related registers will be  programed */
static int programModeRegisters(mode_parameter_t * pModeParam,pll_value_t * pll)
{
	int ret = 0;
	int cnt = 0;
	unsigned int ulTmpValue,ulReg;
	logical_chip_type_t chipType;

	if(pll->clockType == SECONDARY_PLL)
	{
		/* programe secondary pixel clock */
		POKE32(CRT_PLL_CTRL,formatPllReg(pll));
        POKE32(CRT_HORIZONTAL_TOTAL,
              FIELD_VALUE(0, CRT_HORIZONTAL_TOTAL, TOTAL, pModeParam->horizontal_total - 1)
            | FIELD_VALUE(0, CRT_HORIZONTAL_TOTAL, DISPLAY_END, pModeParam->horizontal_display_end - 1));

        POKE32(CRT_HORIZONTAL_SYNC,
              FIELD_VALUE(0, CRT_HORIZONTAL_SYNC, WIDTH, pModeParam->horizontal_sync_width)
            | FIELD_VALUE(0, CRT_HORIZONTAL_SYNC, START, pModeParam->horizontal_sync_start - 1));

        POKE32(CRT_VERTICAL_TOTAL,
              FIELD_VALUE(0, CRT_VERTICAL_TOTAL, TOTAL, pModeParam->vertical_total - 1)
            | FIELD_VALUE(0, CRT_VERTICAL_TOTAL, DISPLAY_END, pModeParam->vertical_display_end - 1));

        POKE32(CRT_VERTICAL_SYNC,
              FIELD_VALUE(0, CRT_VERTICAL_SYNC, HEIGHT, pModeParam->vertical_sync_height)
            | FIELD_VALUE(0, CRT_VERTICAL_SYNC, START, pModeParam->vertical_sync_start - 1));


		ulTmpValue = FIELD_VALUE(0,CRT_DISPLAY_CTRL,VSYNC_PHASE,pModeParam->vertical_sync_polarity)|		
					  FIELD_VALUE(0,CRT_DISPLAY_CTRL,HSYNC_PHASE,pModeParam->horizontal_sync_polarity)|
					  FIELD_SET(0,CRT_DISPLAY_CTRL,TIMING,ENABLE)|		
					  FIELD_SET(0,CRT_DISPLAY_CTRL,PLANE,ENABLE);

		chipType = getChipType();	
		if(chipType == SM750LE || chipType == SM718){
			ulReg = PEEK32(CRT_DISPLAY_CTRL)
					& FIELD_CLEAR(CRT_DISPLAY_CTRL,VSYNC_PHASE)
					& FIELD_CLEAR(CRT_DISPLAY_CTRL,HSYNC_PHASE)
					& FIELD_CLEAR(CRT_DISPLAY_CTRL,TIMING)				
					& FIELD_CLEAR(CRT_DISPLAY_CTRL,PLANE);

			 POKE32(CRT_DISPLAY_CTRL,ulTmpValue|ulReg);	
		}else{
			switch(chipType){
				case SM750HS:
					displayControlAdjust_SM750HS(pModeParam,ulTmpValue);
					break;
				case SM750HS_F:
					displayControlAdjust_SM750HS_F(pModeParam, ulTmpValue);
					break;
				case SM750HS_A:
					displayControlAdjust_SM750HS_A(pModeParam, ulTmpValue);
					break;
				default:
					displayControlAdjust_SM750LE(pModeParam,ulTmpValue);
					break;
			}
		}
	}
	else if(pll->clockType == PRIMARY_PLL)
	{	
		unsigned int ulReservedBits;
		POKE32(PANEL_PLL_CTRL,formatPllReg(pll));

        POKE32(PANEL_HORIZONTAL_TOTAL,
              FIELD_VALUE(0, PANEL_HORIZONTAL_TOTAL, TOTAL, pModeParam->horizontal_total - 1)
            | FIELD_VALUE(0, PANEL_HORIZONTAL_TOTAL, DISPLAY_END, pModeParam->horizontal_display_end - 1));

        POKE32(PANEL_HORIZONTAL_SYNC,
              FIELD_VALUE(0, PANEL_HORIZONTAL_SYNC, WIDTH, pModeParam->horizontal_sync_width)
            | FIELD_VALUE(0, PANEL_HORIZONTAL_SYNC, START, pModeParam->horizontal_sync_start - 1));

        POKE32(PANEL_VERTICAL_TOTAL,
              FIELD_VALUE(0, PANEL_VERTICAL_TOTAL, TOTAL, pModeParam->vertical_total - 1)
            | FIELD_VALUE(0, PANEL_VERTICAL_TOTAL, DISPLAY_END, pModeParam->vertical_display_end - 1));

        POKE32(PANEL_VERTICAL_SYNC,
              FIELD_VALUE(0, PANEL_VERTICAL_SYNC, HEIGHT, pModeParam->vertical_sync_height)
            | FIELD_VALUE(0, PANEL_VERTICAL_SYNC, START, pModeParam->vertical_sync_start - 1));

		ulTmpValue = FIELD_VALUE(0,PANEL_DISPLAY_CTRL,VSYNC_PHASE,pModeParam->vertical_sync_polarity)|
					FIELD_VALUE(0,PANEL_DISPLAY_CTRL,HSYNC_PHASE,pModeParam->horizontal_sync_polarity)|
					FIELD_VALUE(0,PANEL_DISPLAY_CTRL,CLOCK_PHASE,pModeParam->clock_phase_polarity)|
					FIELD_SET(0,PANEL_DISPLAY_CTRL,TIMING,ENABLE)|
					FIELD_SET(0,PANEL_DISPLAY_CTRL,PLANE,ENABLE);
		
        ulReservedBits = FIELD_SET(0, PANEL_DISPLAY_CTRL, RESERVED_1_MASK, ENABLE) |
                         FIELD_SET(0, PANEL_DISPLAY_CTRL, RESERVED_2_MASK, ENABLE) |
                         FIELD_SET(0, PANEL_DISPLAY_CTRL, RESERVED_3_MASK, ENABLE)|
                         FIELD_SET(0,PANEL_DISPLAY_CTRL,VSYNC,ACTIVE_LOW);

        ulReg = (PEEK32(PANEL_DISPLAY_CTRL) & ~ulReservedBits)
              & FIELD_CLEAR(PANEL_DISPLAY_CTRL, CLOCK_PHASE)
              & FIELD_CLEAR(PANEL_DISPLAY_CTRL, VSYNC_PHASE)
              & FIELD_CLEAR(PANEL_DISPLAY_CTRL, HSYNC_PHASE)
              & FIELD_CLEAR(PANEL_DISPLAY_CTRL, TIMING)
              & FIELD_CLEAR(PANEL_DISPLAY_CTRL, PLANE);

		
		/* May a hardware bug or just my test chip (not confirmed).
		* PANEL_DISPLAY_CTRL register seems requiring few writes
		* before a value can be succesfully written in.
		* Added some masks to mask out the reserved bits.
		* Note: This problem happens by design. The hardware will wait for the
		*       next vertical sync to turn on/off the plane.
		*/

		POKE32(PANEL_DISPLAY_CTRL,ulTmpValue|ulReg);
#if 1
		while((PEEK32(PANEL_DISPLAY_CTRL) & ~ulReservedBits) != (ulTmpValue|ulReg))
		{			
			cnt++;
			if(cnt > 1000)
				break;
			POKE32(PANEL_DISPLAY_CTRL,ulTmpValue|ulReg);
		}	
#endif
	}
	else{		
		ret = -1;
	}
	return ret;
}

/*
 *  findModeParamFromTable
 *      This function locates the requested mode in the given parameter table
 *
 *  Input:
 *      width           - Mode width
 *      height          - Mode height
 *      refresh_rate    - Mode refresh rate
 *      index           - Index that is used for multiple search of the same mode 
 *                        that have the same width, height, and refresh rate, 
 *                        but have different timing parameters.
 *
 *  Output:
 *      Success: return a pointer to the mode_parameter_t entry.
 *      Fail: a NULL pointer.
 */
mode_parameter_t *findModeParamFromTable(
    unsigned long width, 
    unsigned long height, 
    unsigned long refresh_rate,
    unsigned short index,
    mode_parameter_t *pModeTable
)

{
    unsigned short modeIndex = 0, tempIndex = 0;
    
    /* Walk the entire mode table. */    
    while (pModeTable[modeIndex].pixel_clock != 0)
    {
        if (((width == (unsigned long)(-1)) || (pModeTable[modeIndex].horizontal_display_end == width)) &&
            ((height == (unsigned long)(-1)) || (pModeTable[modeIndex].vertical_display_end == height)) &&
            ((refresh_rate == (unsigned long)(-1)) || (pModeTable[modeIndex].vertical_frequency == refresh_rate)))
        {
            if (tempIndex < index)
                tempIndex++;
            else
                return (&pModeTable[modeIndex]);
        }
        
        /* Next entry */
        modeIndex++;
    }

    /* No match, return NULL pointer */
    return((mode_parameter_t *)0);
}


int ddk750_setModeTiming(mode_parameter_t * parm,clock_type_t clock)
{
	pll_value_t pll;
	unsigned long width;
    unsigned long height; 
    unsigned long refresh_rate;
	unsigned int uiActualPixelClk;
	pll.inputFreq = DEFAULT_INPUT_CLOCK;
	pll.clockType = clock;

	width = parm->horizontal_display_end;
	height = parm->vertical_display_end;
	refresh_rate = 60;

	switch(getChipType())
	{
		case SM750LE:
    		parm = findModeParamFromTable(width, height, refresh_rate, 0, gSM750LEModeParamTable);
			break;
		case SM750HS:
    		parm = findModeParamFromTable(width, height, refresh_rate, 0, gSM750HSModeParamTable);
			break;
		case SM750HS_F:
		case SM750HS_A:
    		parm = findModeParamFromTable(width, height, refresh_rate, 0, gSM750HS_F_ModeParamTable);
			break;
		default: /* Normal SM750/SM718 */
    		break;
	}
	
	uiActualPixelClk = calcPllValue(parm->pixel_clock,&pll);
	if(getChipType() >= SM750LE){
#if defined(__i386__) || defined( __x86_64__) //fix Segmentation fault on arm platform
		/* set graphic mode via IO method */
		outb_p(0x88,0x3d4);
		outb_p(0x06,0x3d5);
#endif
	}
	programModeRegisters(parm,&pll);
	return 0;	
}


