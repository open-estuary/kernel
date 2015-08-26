obj-$(CONFIG_FB_LYNXFB) += lynxfb.o

lynxfb-objs		:= lynx_drv.o lynx_hw750.o lynx_accel.o lynx_cursor.o
lynxfb-objs		+= ddk750/ddk750_chip.o ddk750/ddk750_power.o ddk750/ddk750_mode.o ddk750/ddk750_display.o ddk750/ddk750_help.o
lynxfb-objs		+= ddk750/ddk750_swi2c.o ddk750/ddk750_sii164.o ddk750/ddk750_dvi.o ddk750/ddk750_hwi2c.o

EXTRA_CFLAGS += -DUSE_DVICHIP -DUSE_HW_I2C
