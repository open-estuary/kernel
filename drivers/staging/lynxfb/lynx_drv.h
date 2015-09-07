#ifndef LYNXDRV_H_
#define LYNXDRV_H_

#include "ddk750/ddk750_chip.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,17)
#else
typedef unsigned long resource_size_t;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
#else
typedef int pm_message_t;
#endif

/* please use revision id to distinguish sm750le and sm750*/
#define SPC_SM750 	0

/* new vendor and dev id for HIS 750*/
#define PCI_VENDOR_ID_HIS	0x19e5
#define PCI_DEVID_750HS		0x1711


#define PCI_VENDOR_ID_SMI 	0x126f
#define PCI_DEVID_LYNX_EXP	0x0750
#define PCI_DEVID_LYNX_SE	0x0718

#define _moduleName_ "lynxfb"
#define PFX _moduleName_ ": "
#define err_msg(fmt,args...) printk(KERN_ERR PFX fmt, ## args)
#define war_msg(fmt,args...) printk(KERN_WARNING PFX fmt, ## args)
#define inf_msg(fmt,args...) printk(KERN_INFO PFX fmt, ## args)
/* below code also works ok,but there must be no KERN_INFO prefix */
//#define inf_msg(...) printk(__VA_ARGS__)

#if (DEBUG == 1)
/* debug level == 1 */
#define dbg_msg(fmt,args...) printk(KERN_DEBUG PFX fmt, ## args)
#define ENTER()	printk(KERN_DEBUG PFX "%*c %s\n",smi_indent++,'>',__func__)
#define LEAVE(...)	\
	do{				\
	printk(KERN_DEBUG PFX "%*c %s\n",--smi_indent,'<',__func__); \
	return __VA_ARGS__; \
	}while(0)

#elif (DEBUG == 2)
/* debug level == 2*/
#define dbg_msg(fmt,args...) printk(KERN_ERR PFX fmt, ## args)
#define ENTER()	printk(KERN_ERR PFX "%*c %s\n",smi_indent++,'>',__func__)

#define LEAVE(...)	\
	do{				\
	printk(KERN_ERR PFX "%*c %s\n",--smi_indent,'<',__func__); \
	return __VA_ARGS__; \
	}while(0)

#ifdef inf_msg
#undef inf_msg
#endif

#define inf_msg(fmt,args...) printk(KERN_ERR PFX fmt, ## args)
#else
/* no debug */
#define dbg_msg(...)
#define ENTER()
#define LEAVE(...) return __VA_ARGS__

#endif

#define MB(x) ((x)<<20)
#define MHZ(x) ((x) * 1000000)
/* align should be 2,4,8,16 */
#define PADDING(align,data) (((data)+(align)-1)&(~((align) -1)))
extern int smi_indent;


struct lynx_accel{
	/* base virtual address of DPR registers */
	volatile unsigned char __iomem * dprBase;
	/* base virtual address of de data port */
	volatile unsigned char __iomem * dpPortBase;

	/* function fointers */
	int (*de_init)(struct lynx_accel *);

	int (*de_wait)(void);/* see if hardware ready to work */

	int (*de_fillrect)(struct lynx_accel *,u32,u32,u32,
							u32,u32,u32,u32,u32,u32);

	int (*de_copyarea)(struct lynx_accel *,u32,u32,u32,u32,
						u32,u32,u32,u32,
						u32,u32,u32,u32);

	int (*de_imageblit)(struct lynx_accel *,const char *,u32,u32,u32,
						u32,u32,u32,u32,u32,u32,u32,u32,u32);					
	
};

/* 	lynx_share stands for a presentation of two frame buffer
	that use one smi adaptor , it is similar to a basic class of C++
*/
struct lynx_share{
	/* device information members */
	u16 venid;
	u16 devid;
	u8 revid;
	logical_chip_type_t chiptype;
	/* driver members */
	struct pci_dev * pdev;
	struct fb_info * fbinfo[2];
	struct lynx_accel accel;
	int accel_off;
	int dual;
#ifdef CONFIG_MTRR
		int mtrr_off;
		struct{
			int vram;
			int vram_added;
		}mtrr;
#endif	
	/* all smi graphic adaptor get below attributes */
	resource_size_t vidmem_start;
	resource_size_t vidreg_start;
	resource_size_t vidmem_size;	
	resource_size_t vidreg_size;
	volatile unsigned char __iomem * pvReg;
	unsigned char __iomem * pvMem;
	/* function pointers */
	void (*suspend)(void);
	void (*resume)(void);
};

struct lynx_cursor{
	/* cursor width ,height and size */
	int w;
	int h;
	int size;
	/* hardware limitation */
	int maxW;
	int maxH;
	/* base virtual address and offset  of cursor image */
	char __iomem * vstart;
	int offset;
	/* mmio addr of hw cursor */
	volatile char __iomem * mmio;	
	/* the lynx_share of this adaptor */
	struct lynx_share * share;
	/* proc_routines */
	void (*enable)(struct lynx_cursor *);
	void (*disable)(struct lynx_cursor *);
	void (*setSize)(struct lynx_cursor *,int,int);
	void (*setPos)(struct lynx_cursor *,int,int);
	void (*setColor)(struct lynx_cursor *,u32,u32);
	void (*setData)(struct lynx_cursor *,u16,const u8*,const u8*);
};

struct lynxfb_crtc{
	unsigned char __iomem * vCursor;//virtual address of cursor
	unsigned char __iomem * vScreen;//virtual address of on_screen
	int oCursor;//cursor address offset in vidmem
	int oScreen;//onscreen address offset in vidmem
	int channel;/* which channel this crtc stands for*/
	resource_size_t vidmem_size;/* this view's video memory max size */

	/* below attributes belong to info->fix, their value depends on specific adaptor*/
	u16 line_pad;/* padding information:0,1,2,4,8,16,... */
	u16 xpanstep;
	u16 ypanstep;
	u16 ywrapstep;

	void * priv;

	int(*proc_setMode)(struct lynxfb_crtc*,
						struct fb_var_screeninfo*,
						struct fb_fix_screeninfo*);	

	int(*proc_checkMode)(struct lynxfb_crtc*,struct fb_var_screeninfo*);	
	int(*proc_setColReg)(struct lynxfb_crtc*,ushort,ushort,ushort,ushort);			
	void (*clear)(struct lynxfb_crtc*);
	/* cursor information */
	struct lynx_cursor cursor;
};

struct lynxfb_output{
	int dpms;
	int paths;
	/* 	which paths(s) this output stands for,for sm750: 
		paths=1:means output for panel paths
		paths=2:means output for crt paths
		paths=3:means output for both panel and crt paths
	*/

	int * channel;
	/* 	which channel these outputs linked with,for sm750:
		*channel=0 means primary channel
		*channel=1 means secondary channel
		output->channel ==> &crtc->channel
	*/
	void * priv;
	
	int(*proc_setMode)(struct lynxfb_output*,
						struct fb_var_screeninfo*,
						struct fb_fix_screeninfo*);	

	int(*proc_checkMode)(struct lynxfb_output*,struct fb_var_screeninfo*);	
	int(*proc_setBLANK)(struct lynxfb_output*,int);
	void  (*clear)(struct lynxfb_output*);
};

struct lynxfb_par{
	/* either 0 or 1 for dual head adaptor,0 is the older one registered */
	int index;
	unsigned int pseudo_palette[256];
	struct lynxfb_crtc crtc;
	struct lynxfb_output output;
	struct fb_info * info;
	struct lynx_share * share;
};

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#if 0
#define OUTPUT_2_SHARE(pOutput)	\
		(struct lynx_share *)	\
		((char*)container_of(pOutput,struct lynxfb_par,output) +	\
		offsetof(struct lynxfb_par,share))

#define OUTPUT_2_INFO(pOutput)	\
		(struct fb_info *)	\
		((char *)container_of(pOutput,struct lynxfb_par,output) + 	\
		offsetof(struct lynxfb_par,info))


#define CRTC_2_SHARE(pCrtc)	\
		(struct lynx_share*)	\
		((char*)container_of(pCrtc,struct lynxfb_par,crtc) +	\
		offsetof(struct lynxfb_par,share))

#define CRTC_2_INFO(pCrtc)	\
		(struct fb_info *)	\
		((char *)container_of(pCrtc,struct lynxfb_par,crtc) + 	\
		offsetof(struct lynxfb_par,info))
#endif

#define PS_TO_HZ(ps)	\
			({ 	\
			unsigned long long hz = 1000*1000*1000*1000UL;	\
			do_div(hz,ps);	\
			(unsigned long)hz;})

#if 1
static inline unsigned long ps_to_hz(unsigned int psvalue)
{
	unsigned long long numerator=1000*1000*1000*1000ULL;
	/* 10^12 / picosecond period gives frequency in Hz */
	do_div(numerator, psvalue);
	return (unsigned long)numerator;
}
#else
static inline unsigned long ps_to_hz(unsigned long psvalue)
{
	unsigned long long numerator=1000*1000*1000*1000ULL;
	/* 10^12 / picosecond period gives frequency in Hz */
	do_div(numerator, psvalue);
	return (unsigned long)numerator;
}
#endif



#endif
