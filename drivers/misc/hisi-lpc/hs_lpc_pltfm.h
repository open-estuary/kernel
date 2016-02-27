#ifndef __HS_LPC_PLTFM_H__
#define __HS_LPC_PLTFM_H__

#define HS_LPC_REG_START            (0x00)
#define HS_LPC_REG_OP_STATUS        (0x04)
#define HS_LPC_REG_IQR_ST           (0x08)
#define HS_LPC_REG_OP_LEN           (0x10)
#define HS_LPC_REG_CMD              (0x14)
#define HS_LPC_REG_FWH_ID_MSIZE     (0x18)
#define HS_LPC_REG_ADDR             (0x20)
#define HS_LPC_REG_WDATA            (0x24)
#define HS_LPC_REG_RDATA            (0x28)
#define HS_LPC_REG_LONG_CNT         (0x30)
#define HS_LPC_REG_TX_FIFO_ST       (0x50)
#define HS_LPC_REG_RX_FIFO_ST       (0x54)
#define HS_LPC_REG_TIME_OUT         (0x58)
#define HS_LPC_REG_STRQ_CTRL0       (0x80)
#define HS_LPC_REG_STRQ_CTRL1       (0x84)
#define HS_LPC_REG_STRQ_INT         (0x90)
#define HS_LPC_REG_STRQ_INT_MASK    (0x94)
#define HS_LPC_REG_STRQ_STAT        (0xa0)

#define HS_LPC_CMD_SAMEADDR_SING    (0x00000008)
#define HS_LPC_CMD_SAMEADDR_INC     (0x00000000)
#define HS_LPC_CMD_TYPE_IO          (0x00000000)
#define HS_LPC_CMD_TYPE_MEM         (0x00000002)
#define HS_LPC_CMD_TYPE_FWH         (0x00000004)
#define HS_LPC_CMD_WRITE            (0x00000001)
#define HS_LPC_CMD_READ             (0x00000000)

#define IO_MUX_BASE_ADDR  (0xa0170000)
#define REG_OFFSET_IOMG033      0X84
#define REG_OFFSET_IOMG035      0x8C
#define REG_OFFSET_IOMG036      0x90
#define REG_OFFSET_IOMG050      0xC8
#define REG_OFFSET_IOMG049      0xC4
#define REG_OFFSET_IOMG048      0xC0
#define REG_OFFSET_IOMG047      0xBC
#define REG_OFFSET_IOMG046      0xB8
#define REG_OFFSET_IOMG045      0xB4

#define REG_OFFSET_LPC_RESET         0xad8
#define REG_OFFSET_LPC_BUS_RESET     0xae0
#define REG_OFFSET_LPC_CLK_DIS       0x3a4
#define REG_OFFSET_LPC_BUS_CLK_DIS   0x3ac
#define REG_OFFSET_LPC_DE_RESET      0xadc
#define REG_OFFSET_LPC_BUS_DE_RESET  0xae4
#define REG_OFFSET_LPC_CLK_EN        0x3a0
#define REG_OFFSET_LPC_BUS_CLK_EN    0x3a8

#define LPC_FRAME_LEN (0x10)

struct hs_lpc_dev {
	spinlock_t lock;   /* spin lock */
	void __iomem  *regs;
	struct device *dev;
};

#define LPC_CURR_STATUS_IDLE    0
#define LPC_CURR_STATUS_START   1
#define LPC_CURR_STATUS_TYPE_DIR 2
#define LPC_CURR_STATUS_ADDR    3
#define LPC_CURR_STATUS_MSIZE   4
#define LPC_CURR_STATUS_WDATA   5
#define LPC_CURR_STATUS_TARHOST 6
#define LPC_CURR_STATUS_SYNC    7
#define LPC_CURR_STATUS_RDATA   8
#define LPC_CURR_STATUS_TARSLAVE 9
#define LPC_CURR_STATUS_ABORT   10

#define ALG_HCCS_SUBCTRL_BASE_ADDR (0xD0000000)
#define REG_OFFSET_RESET_REQ       0x0AD0
#define REG_OFFSET_RESET_DREQ      0x0AD4

extern u8 lpc_io_read_byte(unsigned long addr);
extern void  lpc_io_write_byte(u8 value, unsigned long addr);
#endif

