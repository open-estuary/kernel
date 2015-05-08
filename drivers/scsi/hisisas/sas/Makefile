#include sas common header directory
EXTRA_CFLAGS += -I$(srctree)/drivers/scsi/hisisas/common
EXTRA_CFLAGS += -Wno-date-time
#EXTRA_CFLAGS += -mlong-calls
EXTRA_CFLAGS += -fsigned-char

obj-$(CONFIG_SCSI_HISISAS_COMMON) += sal.o

sal-y += sal_version.o \
	sal_init.o \
	sal_event.o \
	sal_commfunc.o \
   	sadiscover.o \
	sadiscctrl.o \
	sal_io.o \
	sal_dev.o \
	sal_msg.o \
	sal_ctrl.o \
	sal_crc.o \
	sal_errhandler.o \
	sat_main.o \
	sat_protocol.o


	
