#include sas common header directory
EXTRA_CFLAGS += -I$(srctree)/drivers/scsi/hisisas/common
EXTRA_CFLAGS += -I$(srctree)/drivers/scsi/hisisas/sas
EXTRA_CFLAGS += -Wno-date-time
#EXTRA_CFLAGS += -mlong-calls
EXTRA_CFLAGS += -fsigned-char

higgs-y += higgs_init.o
higgs-y += higgs_dev.o
higgs-y += higgs_intr.o
higgs-y += higgs_dump.o
higgs-y += higgs_eh.o
higgs-y += higgs_io.o
higgs-y += higgs_peri.o
higgs-y += higgs_dfm.o
higgs-y += higgs_stub.o
higgs-y += higgs_port.o
higgs-y += higgs_phy.o
higgs-y += higgs_misc.o
higgs-y += higgs_version.o
#hilink serdes api
higgs-y += higgs_serdes_api.o

obj-$(CONFIG_SCSI_HISISAS_PV660) := higgs.o

