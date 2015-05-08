/*
 * Huawei Pv660/Hi1610 sata protocol related data structure
 *
 * Copyright 2015 Huawei.
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

#ifndef __SATPROTOCOL_H__
#define __SATPROTOCOL_H__



/******************************************************************************
                        SCSI related macros and defines
*******************************************************************************/

#define SCSIOPC_TEST_UNIT_READY     0x00
#define SCSIOPC_INQUIRY             0x12
#define SCSIOPC_MODE_SENSE_6        0x1A
#define SCSIOPC_MODE_SENSE_10       0x5A
#define SCSIOPC_MODE_SELECT_6       0x15
#define SCSIOPC_START_STOP_UNIT     0x1B
#define SCSIOPC_READ_CAPACITY_10    0x25
#define SCSIOPC_READ_CAPACITY_16    0x9E
#define SCSIOPC_READ_6              0x08
#define SCSIOPC_READ_10             0x28
#define SCSIOPC_READ_12             0xA8
#define SCSIOPC_READ_16             0x88
#define SCSIOPC_WRITE_6             0x0A
#define SCSIOPC_WRITE_10            0x2A
#define SCSIOPC_WRITE_12            0xAA
#define SCSIOPC_WRITE_16            0x8A
#define SCSIOPC_WRITE_VERIFY        0x2E
#define SCSIOPC_VERIFY_10           0x2F
#define SCSIOPC_VERIFY_12           0xAF
#define SCSIOPC_VERIFY_16           0x8F
#define SCSIOPC_REQUEST_SENSE       0x03
#define SCSIOPC_REPORT_LUN          0xA0
#define SCSIOPC_FORMAT_UNIT         0x04
#define SCSIOPC_SEND_DIAGNOSTIC     0x1D
#define SCSIOPC_WRITE_SAME_10       0x41
#define SCSIOPC_WRITE_SAME_16       0x93
#define SCSIOPC_READ_BUFFER         0x3C
#define SCSIOPC_WRITE_BUFFER        0x3B
#define SCSIOPC_LOG_SENSE           0x4D
#define SCSIOPC_MODE_SELECT_10      0x55
#define SCSIOPC_SYNCHRONIZE_CACHE_10 0x35
#define SCSIOPC_SYNCHRONIZE_CACHE_16 0x91
#define SCSIOPC_WRITE_AND_VERIFY_10 0x2E
#define SCSIOPC_WRITE_AND_VERIFY_12 0xAE
#define SCSIOPC_WRITE_AND_VERIFY_16 0x8E
#define SCSIOPC_READ_MEDIA_SERIAL_NUMBER 0xAB
#define SCSIOPC_REASSIGN_BLOCKS     0x07
#define SCSIOPC_ATA_PASSTHROUGH_16     0x85
#define SCSIOPC_ATA_PASSTHROUGH_12      0xa1

/*max scsi operation code*/
#define SCSIOPC_MAX                     0xFF

/*SCSI SENSE KEY VALUES*/
#define SCSI_SNSKEY_NO_SENSE           0x00
#define SCSI_SNSKEY_RECOVERED_ERROR    0x01
#define SCSI_SNSKEY_NOT_READY          0x02
#define SCSI_SNSKEY_MEDIUM_ERROR       0x03
#define SCSI_SNSKEY_HARDWARE_ERROR     0x04
#define SCSI_SNSKEY_ILLEGAL_REQUEST    0x05
#define SCSI_SNSKEY_UNIT_ATTENTION     0x06
#define SCSI_SNSKEY_DATA_PROTECT       0x07
#define SCSI_SNSKEY_ABORTED_COMMAND    0x0B
#define SCSI_SNSKEY_MISCOMPARE         0x0E

/*SCSI Additional Sense Codes and Qualifiers combo two-bytes*/
#define SCSI_SNSCODE_NO_ADDITIONAL_INFO                         0x0000
#define SCSI_SNSCODE_INVALID_COMMAND                            0x2000
#define SCSI_SNSCODE_LOGICAL_BLOCK_OUT                          0x2100
#define SCSI_SNSCODE_INVALID_FIELD_IN_CDB                       0x2400
#define SCSI_SNSCODE_LOGICAL_NOT_SUPPORTED                      0x2500
#define SCSI_SNSCODE_POWERON_RESET                              0x2900
#define SCSI_SNSCODE_EVERLAPPED_CMDS                            0x4e00
#define SCSI_SNSCODE_INTERNAL_TARGET_FAILURE                    0x4400
#define SCSI_SNSCODE_MEDIUM_NOT_PRESENT                         0x3a00
#define SCSI_SNSCODE_UNRECOVERED_READ_ERROR                     0x1100
#define SCSI_SNSCODE_RECORD_NOT_FOUND                           0x1401
#define SCSI_SNSCODE_NOT_READY_TO_READY_CHANGE                  0x2800
#define SCSI_SNSCODE_OPERATOR_MEDIUM_REMOVAL_REQUEST            0x5a01
#define SCSI_SNSCODE_INFORMATION_UNIT_CRC_ERROR                 0x4703
#define SCSI_SNSCODE_LOGICAL_UNIT_NOT_READY_FORMAT_IN_PROGRESS  0x0404
#define SCSI_SNSCODE_HARDWARE_IMPENDING_FAILURE                 0x5d10
#define SCSI_SNSCODE_LOW_POWER_CONDITION_ON                     0x5e00
#define SCSI_SNSCODE_LOGICAL_UNIT_NOT_READY_INIT_REQUIRED       0x0402
#define SCSI_SNSCODE_INVALID_FIELD_PARAMETER_LIST               0x2600
#define SCSI_SNSCODE_PARAMETER_LIST_LENGTH_ERROR                0x1A00
#define SCSI_SNSCODE_ATA_DEVICE_FAILED_SET_FEATURES             0x4471
#define SCSI_SNSCODE_ATA_DEVICE_FEATURE_NOT_ENABLED             0x670B
#define SCSI_SNSCODE_LOGICAL_UNIT_FAILED_SELF_TEST              0x3E03
#define SCSI_SNSCODE_COMMAND_SEQUENCE_ERROR                     0x2C00
#define SCSI_SNSCODE_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE         0x2100
#define SCSI_SNSCODE_LOGICAL_UNIT_FAILURE                       0x3E01
#define SCSI_SNSCODE_MEDIA_LOAD_OR_EJECT_FAILED                 0x5300
#define SCSI_SNSCODE_LOGICAL_UNIT_NOT_READY_INITIALIZING_COMMAND_REQUIRED 0x0402
#define SCSI_SNSCODE_LOGICAL_UNIT_NOT_READY_CAUSE_NOT_REPORTABLE          0x0400
#define SCSI_SNSCODE_LOGICAL_UNIT_DOES_NOT_RESPOND_TO_SELECTION           0x0500
#define SCSI_SNSCODE_DIAGNOSTIC_FAILURE_ON_COMPONENT_NN         0x4000
#define SCSI_SNSCODE_COMMANDS_CLEARED_BY_ANOTHER_INITIATOR      0x2F00
#define SCSI_SNSCODE_WRITE_ERROR_AUTO_REALLOCATION_FAILED       0x0C02
#define SCSI_SNSCODE_WRITE_PROTECTED                               0x2700

/*SCSI Additional Sense Codes and Qualifiers saparate bytes*/
#define SCSI_ASC_NOTREADY_INIT_CMD_REQ    0x04
#define SCSI_ASCQ_NOTREADY_INIT_CMD_REQ   0x02

/*SCSI related bit mask*/
#define SCSI_EVPD_MASK               0x01
#define SCSI_IMMED_MASK              0x01
#define SCSI_NACA_MASK               0x04
#define SCSI_LINK_MASK               0x01
#define SCSI_PF_MASK                 0x10
#define SCSI_DEVOFFL_MASK            0x02
#define SCSI_UNITOFFL_MASK           0x01
#define SCSI_START_MASK              0x01
#define SCSI_LOEJ_MASK               0x02
#define SCSI_NM_MASK                 0x02
#define SCSI_FLUSH_CACHE_IMMED_MASK              0x02
#define SCSI_FUA_NV_MASK                         0x02
#define SCSI_VERIFY_BYTCHK_MASK                  0x02
#define SCSI_FORMAT_UNIT_IMMED_MASK              0x02
#define SCSI_FORMAT_UNIT_FOV_MASK                0x80
#define SCSI_FORMAT_UNIT_DCRT_MASK               0x20
#define SCSI_FORMAT_UNIT_IP_MASK                 0x08
#define SCSI_WRITE_SAME_LBDATA_MASK              0x02
#define SCSI_WRITE_SAME_PBDATA_MASK              0x04
#define SCSI_SYNC_CACHE_IMMED_MASK               0x02
#define SCSI_WRITE_N_VERIFY_BYTCHK_MASK          0x02
#define SCSI_SEND_DIAGNOSTIC_SELFTEST_MASK       0x04
#define SCSI_FORMAT_UNIT_DEFECT_LIST_FORMAT_MASK 0x07
#define SCSI_FORMAT_UNIT_FMTDATA_MASK            0x10
#define SCSI_FORMAT_UNIT_CMPLIST_MASK            0x08
#define SCSI_FORMAT_UNIT_LONGLIST_MASK           0x20
#define SCSI_READ10_FUA_MASK                     0x08
#define SCSI_READ12_FUA_MASK                     0x08
#define SCSI_READ16_FUA_MASK                     0x08
#define SCSI_WRITE10_FUA_MASK                    0x08
#define SCSI_WRITE12_FUA_MASK                    0x08
#define SCSI_WRITE16_FUA_MASK                    0x08
#define SCSI_READ_CAPACITY10_PMI_MASK            0x01
#define SCSI_READ_CAPACITY16_PMI_MASK            0x01
#define SCSI_MODE_SENSE6_PC_MASK                 0xC0
#define SCSI_MODE_SENSE6_PAGE_CODE_MASK          0x3F
#define SCSI_MODE_SENSE10_PC_MASK                0xC0
#define SCSI_MODE_SENSE10_LLBAA_MASK             0x10
#define SCSI_MODE_SENSE10_PAGE_CODE_MASK         0x3F
#define SCSI_SEND_DIAGNOSTIC_TEST_CODE_MASK      0xE0
#define SCSI_LOG_SENSE_PAGE_CODE_MASK            0x3F
#define SCSI_MODE_SELECT6_PF_MASK                0x10
#define SCSI_MODE_SELECT6_AWRE_MASK              0x80
#define SCSI_MODE_SELECT6_RC_MASK                0x10
#define SCSI_MODE_SELECT6_EER_MASK               0x08
#define SCSI_MODE_SELECT6_PER_MASK               0x04
#define SCSI_MODE_SELECT6_DTE_MASK               0x02
#define SCSI_MODE_SELECT6_DCR_MASK               0x01
#define SCSI_MODE_SELECT6_WCE_MASK               0x04
#define SCSI_MODE_SELECT6_DRA_MASK               0x20
#define SCSI_MODE_SELECT6_PERF_MASK              0x80
#define SCSI_MODE_SELECT6_TEST_MASK              0x04
#define SCSI_MODE_SELECT6_DEXCPT_MASK            0x08
#define SCSI_MODE_SELECT10_PF_MASK               0x10
#define SCSI_MODE_SELECT10_LONGLBA_MASK          0x01
#define SCSI_MODE_SELECT10_AWRE_MASK             0x80
#define SCSI_MODE_SELECT10_RC_MASK               0x10
#define SCSI_MODE_SELECT10_EER_MASK              0x08
#define SCSI_MODE_SELECT10_PER_MASK              0x04
#define SCSI_MODE_SELECT10_DTE_MASK              0x02
#define SCSI_MODE_SELECT10_DCR_MASK              0x01
#define SCSI_MODE_SELECT10_WCE_MASK              0x04
#define SCSI_MODE_SELECT10_DRA_MASK              0x20
#define SCSI_MODE_SELECT10_PERF_MASK             0x80
#define SCSI_MODE_SELECT10_TEST_MASK             0x04
#define SCSI_MODE_SELECT10_DEXCPT_MASK           0x08
#define SCSI_WRITE_N_VERIFY10_FUA_MASK           0x08
#define SCSI_REQUEST_SENSE_DESC_MASK             0x01
#define SCSI_READ_WRITE_BUFFER_MODE_MASK         0x1F

#define SCSI_READ_WRITE_10_FUA_MASK        SCSI_READ10_FUA_MASK
#define SCSI_READ_WRITE_12_FUA_MASK        SCSI_READ12_FUA_MASK
#define SCSI_READ_WRITE_16_FUA_MASK        SCSI_READ16_FUA_MASK

#define SCSI_INQUIRY_SUPPORTED_VPD_PAGE             0x00
#define SCSI_INQUIRY_UNIT_SERIAL_NUMBER_VPD_PAGE    0x80
#define SCSI_INQUIRY_DEVICE_IDENTIFICATION_VPD_PAGE 0x83
#define SCSI_INQUIRY_ATA_INFORMATION_VPD_PAGE       0x89

#define SCSI_READ_CAPACITY16_PARA_LEN           32
#define SCSI_READ_CAPACITY10_PARA_LEN           8

#define SCSI_MODESENSE_CONTROL_PAGE                            0x0A
#define SCSI_MODESENSE_READ_WRITE_ERROR_RECOVERY_PAGE        0x01
#define SCSI_MODESENSE_CACHING                                  0x08
#define SCSI_MODESENSE_INFORMATION_EXCEPTION_CONTROL_PAGE   0x1C
#define SCSI_MODESENSE_RETURN_ALL_PAGES                        0x3F
#define SCSI_MODESENSE_VENDOR_SPECIFIC_PAGE                   0x00

#define SCSI_MODESELECT_CONTROL_PAGE                            0x0A
#define SCSI_MODESELECT_READ_WRITE_ERROR_RECOVERY_PAGE        0x01
#define SCSI_MODESELECT_CACHING                                  0x08
#define SCSI_MODESELECT_INFORMATION_EXCEPTION_CONTROL_PAGE   0x1C

#define SCSI_LOGSENSE_SUPPORTED_LOG_PAGES                      0x00
#define SCSI_LOGSENSE_SELFTEST_RESULTS_PAGE                    0x10
#define SCSI_LOGSENSE_INFORMATION_EXCEPTIONS_PAGE              0x2F

/*For internally use*/
#define SCSI_MODE_SENSE6_RETURN_ALL_PAGES_LEN                     68
#define SCSI_MODE_SENSE6_CONTROL_PAGE_LEN                          24
#define SCSI_MODE_SENSE6_READ_WRITE_ERROR_RECOVERY_PAGE_LEN      24
#define SCSI_MODE_SENSE6_CACHING_LEN                                32
#define SCSI_MODE_SENSE6_INFORMATION_EXCEPTION_CONTROL_PAGE_LEN 24
#define SCSI_MODE_SENSE6_MAX_PAGE_LEN   SCSI_MODE_SENSE6_RETURN_ALL_PAGES_LEN

#define SCSI_MODE_SENSE10_RETURN_ALL_PAGES_LEN        72
#define SCSI_MODE_SENSE10_CONTROL_PAGE_LEN             28
#define SCSI_MODE_SENSE10_READ_WRITE_ERROR_RECOVERY_PAGE_LEN 28
#define SCSI_MODE_SENSE10_CACHING_LEN                  36
#define SCSI_MODE_SENSE10_INFORMATION_EXCEPTION_CONTROL_PAGE_LEN 28

#define SCSI_MODE_SENSE10_RETURN_ALL_PAGES_LLBAA_LEN         80
#define SCSI_MODE_SENSE10_CONTROL_PAGE_LLBAA_LEN             36
#define SCSI_MODE_SENSE10_READ_WRITE_ERROR_RECOVERY_PAGE_LLBAA_LEN 36
#define SCSI_MODE_SENSE10_CACHING_LLBAA_LEN                  44
#define SCSI_MODE_SENSE10_INFORMATION_EXCEPTION_CONTROL_PAGE_LLBAA_LEN 36
#define SCSI_MODE_SENSE10_MAX_PAGE_LEN   SCSI_MODE_SENSE10_RETURN_ALL_PAGES_LLBAA_LEN

/*TMP buffer size of INQUIRY used by SAT*/
#define SCSI_MAX_INQUIRY_BUFFER_SIZE 1024

/*group number shift in operation code*/
#define SCSI_GROUP_NUMBER_SHIFT 5

#define SCSI_IO_DMA_FROM_DEV(direction)  ( PCI_DMA_FROMDEVICE== direction)
#define SCSI_IO_DMA_TO_DEV(direction)    (PCI_DMA_TODEVICE == direction)
#define SCSI_IO_DMA_NONE(direction)       (PCI_DMA_NONE == direction)
/*Sense data format definitions,support both fix format sense data and descriptor
 *format sense data*/

/*ATA identify frame size,512 byte*/
#define ATA_IDENTIFY_SIZE             512

#define ATA_READ_LOG_LEN              512

#define ATA_MAX_LOG_INDEX               19
#define ATA_MAX_SMART_READ_LOG_INDEX   21
/*max NCQ tag */
#define MAX_NCQ_TAG  31

/*SATA device capabity*/
#define SAT_DEV_CAP_PIO                        0x00000000
#define SAT_DEV_CAP_NCQ                        0x00000001
#define SAT_DEV_CAP_48BIT                      0x00000002
#define SAT_DEV_CAP_DMA                        0x00000004

#define SAT_DEV_SUPPORT_NCQ             (SAT_DEV_CAP_NCQ|SAT_DEV_CAP_48BIT)
#define SAT_DEV_SUPPORT_DMA_EXT        (SAT_DEV_CAP_DMA|SAT_DEV_CAP_48BIT)
#define SAT_DEV_SUPPORT_DMA             SAT_DEV_CAP_DMA
#define SAT_DEV_SUPPORT_PIO_EXT        (SAT_DEV_CAP_PIO|SAT_DEV_CAP_48BIT)
#define SAT_DEV_SUPPORT_PIO             SAT_DEV_CAP_PIO

/*NCQ enable or disable flag*/
#define SAT_NCQ_DISABLED                       0x0
#define SAT_NCQ_ENABLED                        0x1

#define SAT_TRANSFER_MOD_PIO                   0x01
#define SAT_TRANSFER_MOD_DMA                   0x02
#define SAT_TRANSFER_MOD_PIO_EXT               0x03
#define SAT_TRANSFER_MOD_DMA_EXT               0x04
#define SAT_TRANSFER_MOD_NCQ                    0x05

#define READ_WRITE_BUFFER_DATA_MODE             0x02
#define READ_BUFFER_DESCRIPTOR_MODE            0x03

/*functional Macros*/
/*SMART self-test supported,word 84 bit 1*/
#define SAT_IS_SMART_SELFTEST_SUPPORT(identify)             \
    (0 != ((identify)->cmd_set_feature_supported_ext & 0x02))

/*SMART feature set supported,word 82 bit0*/
#define SAT_IS_SMART_FEATURESET_SUPPORT(identify)             \
    (0 != ((identify)->cmd_set_supported & 0x01))

/*SMART feature set enabled,word 85 bit 0*/
#define SAT_IS_SMART_FEATURESET_ENABLED(identify)             \
    (0 != ((identify)->cmd_set_feature_enabled & 0x01))

/*write cache enabled,word 85 bit 5*/
#define SAT_IS_WRITE_CACHE_ENABLED(identify)             \
    (0 != ((identify)->cmd_set_feature_enabled & 0x20))

/*Read look-ahead enabled,word 85 bit 6*/
#define SAT_IS_LOOK_AHEAD_ENABLED(identify)             \
    (0 != ((identify)->cmd_set_feature_enabled & 0x40))

/*Removable media,word 0,bit 0*/
#define SAT_IS_ATA_DEVICE(identify)             \
    (0 != ((identify)->rm_ata_device & 0x80))

/*64-bit world wide name supported,word 87 bit 8*/
#define SAT_IS_WWN_SUPPORTD(identify)             \
    (0 != ((identify)->cmd_set_feature_default & 0x100))
/*whether A scsi command is read/write command*/
#define SAT_IS_READ_WRITE_CMD(opcode)                                   \
    ((SCSIOPC_READ_10 == opcode)||(SCSIOPC_WRITE_10 == opcode)            \
      ||(SCSIOPC_READ_6 == opcode)||(SCSIOPC_WRITE_6 == opcode)          \
      ||(SCSIOPC_READ_16 == opcode)||(SCSIOPC_WRITE_16 == opcode))
/*ATA Fis type definition*/
#define SATA_FIS_TYPE_H2D             0x27	/*Host to Device */
#define SATA_FIS_TYPE_D2H             0x34
#define SATA_FIS_TYPE_SDB             0xa1
#define SATA_FIS_TYPE_DMASETUP       0x41
#define SATA_FIS_TYPE_BIST_ACTIVATE 0x58
#define SATA_FIS_TYPE_PIO_SETUP      0x5f
#define SATA_FIS_TYPE_DATA            0x46
/*ATA command definitions,¼ûATA8-ACS.pdf P349*/
#define FIS_COMMAND_IDENTIFY_DEVICE          0xec
#define FIS_COMMAND_DEVICE_RESET             0x08
#define FIS_COMMAND_CHECK_POWER_MODE        0xe5
#define FIS_COMMAND_DIAGNOSTIC               0x92
#define FIS_COMMAND_FLUSH_CACHE              0xe7
#define FIS_COMMAND_FLUSH_CACHE_EXT          0xea
#define FIS_COMMAND_READ_BUFFER               0xe4
#define FIS_COMMAND_READ_DMA                  0xc8
#define FIS_COMMAND_READ_DMA_EXT             0x25
#define FIS_COMMAND_READ_DMA_QUEUED          0xc7
#define FIS_COMMAND_READ_DMA_QUEUED_EXT     0x26
#define FIS_COMMAND_READ_FPDMA_QUEUED       0x60
#define FIS_COMMAND_READ_LOG_EXT             0x2f
#define FIS_COMMAND_READ_LOG_DMA_EXT        0x47
#define FIS_COMMAND_READ_SECTOR              0x20
#define FIS_COMMAND_READ_SECTOR_EXT         0x24
#define FIS_COMMAND_READ_VERIFY_SECTOR      0x40
#define FIS_COMMAND_READ_VERIFY_SECTOR_EXT  0x42
#define FIS_COMMAND_SET_FEATURES             0xef
#define FIS_COMMAND_SMART                     0xb0
#define FIS_COMMAND_STANDBY                   0xe2
#define FIS_COMMAND_STANDBY_IMMEDIATE       0xe0
#define FIS_COMMAND_WRITE_BUFFER             0xe8
#define FIS_COMMAND_WRITE_DMA                0xca
#define FIS_COMMAND_WRITE_DMA_EXT           0x35
#define FIS_COMMAND_WRITE_DMA_QUEUED        0xcc
#define FIS_COMMAND_WRITE_DMA_QUEUED_EXT   0x36
#define FIS_COMMAND_WRITE_FPDMA_QUEUED      0x61
#define FIS_COMMAND_WRITE_LOG_EXT           0x3f
#define FIS_COMMAND_WRITE_LOG_DMA_EXT       0x57
#define FIS_COMMAND_WRITE_SECTOR            0x30
#define FIS_COMMAND_WRITE_SECTOR_EXT        0x34
#define	FIS_COMMAND_READ_MULTI	            0xC4
#define	FIS_COMMAND_READ_MULTI_EXT	        0x29
#define	FIS_COMMAND_WRITE_MULTI	            0xC5
#define	FIS_COMMAND_WRITE_MULTI_EXT	        0x39
#define	FIS_COMMAND_WRITE_MULTI_FUA_EXT    0xCE
#define FIS_COMMAND_ENABLE_DISABLE_OPERATIONS   0xB0
#define FIS_COMMAND_SMART_EXECUTE_OFFLINE_IMMEDIATE 0xB0

/*
 *  transfer length and LBA limit 2^28 See identify device data word 61:60
 *  ATA spec p125
 *  7 zeros
 */
#define SAT_TR_LBA_LIMIT                      0x10000000

#define SAT_IS_MULTI_COMMAND(fiscommand)                          \
        ((FIS_COMMAND_READ_MULTI == fiscommand)                            \
          ||(FIS_COMMAND_READ_MULTI_EXT == fiscommand)                     \
          ||(FIS_COMMAND_WRITE_MULTI == fiscommand)                        \
          ||(FIS_COMMAND_WRITE_MULTI_EXT == fiscommand)                    \
          ||(FIS_COMMAND_WRITE_MULTI_FUA_EXT == fiscommand))

#define SAT_IS_READ_COMMAND(opcode) ((SCSIOPC_READ_10 == opcode)         \
        ||(SCSIOPC_READ_12 == opcode)||(SCSIOPC_READ_6 == opcode)         \
        ||(SCSIOPC_READ_16 == opcode))
#define SAT_IS_NCQ_REQ(fis_command) \
            (((fis_command) == FIS_COMMAND_WRITE_FPDMA_QUEUED)\
              ||((fis_command) == FIS_COMMAND_READ_FPDMA_QUEUED))

#define SAT_IS_ATA_PASSTHROUGH_CMND(cdb)            \
            (((SCSIOPC_ATA_PASSTHROUGH_12 == (cdb)[0])      \
                                ||(SCSIOPC_ATA_PASSTHROUGH_16 == (cdb)[0])))
/*C bit MASK,when it is set to 0,means it is control command*/
#define SAT_H2D_CBIT_MASK                                0x80
#define SAT_IS_RESET_OR_DERESET_FIS(h2dfis)                          \
    ((0 == (h2dfis->header.pm_port & SAT_H2D_CBIT_MASK))             \
    &&((0 == h2dfis->data.control)||(4 == h2dfis->data.control)))

/* 
 * ATA Status Register Mask 
 */
#define ATA_STATUS_MASK_ERR                   0x01	/* Error/check bit  */
#define ATA_STATUS_MASK_DRQ                   0x08	/* data Request bit */
#define ATA_STATUS_MASK_DF                    0x20	/* Device Fault bit */
#define ATA_STATUS_MASK_DRDY                  0x40	/* Device Ready bit */
#define ATA_STATUS_MASK_BSY                   0x80	/* Busy bit         */

/*if DF or ERR bit in status bit is set,current command failed*/
#define SAT_IS_IO_FAILED(status) ((status)&(ATA_STATUS_MASK_ERR|ATA_STATUS_MASK_DF))

/* 
 * ATA Error Register Mask 
 */
#define ATA_ERROR_MASK_NM                     0x02	/* No media present bit         */
#define ATA_ERROR_MASK_ABRT                   0x04	/* Command aborted bit          */
#define ATA_ERROR_MASK_MCR                    0x08	/* Media change request bit     */
#define ATA_ERROR_MASK_IDNF                   0x10	/* Address not found bit        */
#define ATA_ERROR_MASK_MC                     0x20	/* Media has changed bit        */
#define ATA_ERROR_MASK_UNC                    0x40	/* Uncorrectable data error bit */
#define ATA_ERROR_MASK_ICRC                   0x80	/* Interface CRC error bit      */

#define ATA_MAX_VERIFY_LEN(command) (FIS_COMMAND_READ_VERIFY_SECTOR == command)?256:65536

#define IS_DIGNOSTIC_REQ(cmnd)    (cmnd == FIS_COMMAND_SMART_EXECUTE_OFFLINE_IMMEDIATE)

#define IS_BACKGROUND_SELF_TEST(lba) (lba == 1 || lba == 2)

#define SAT_CHECK_AND_SET(condition, varible, v_1, v_2) \
        ((condition)?(varible = v_1):(varible = v_2))

#define SAT_OR(cond_1, cond_2) ((cond_1) || (cond_2))


/******************************************************************************
                        SATA related macros and defines
*******************************************************************************/
/*sata identify information*/
struct sata_identify_data {
	u16 rm_ata_device;
	/* b15-b9 :  */
	/* b8     :  ataDevice */
	/* b7-b1  : */
	/* b0     :  removableMedia */
	u16 word_1_9[9];		/**< word 1 to 9 of identify device information */
	u8 serial_num[20];	     /**< word 10 to 19 of identify device information, 20 ASCII chars */
	u16 word_20_22[3];		/**< word 20 to 22 of identify device information */
	u8 firmware_ver[8];	    /**< word 23 to 26 of identify device information, 4 ASCII chars */
	u8 model_num[40];	     /**< word 27 to 46 of identify device information, 40 ASCII chars */
	u16 word_47_48[2];		/**< word 47 to 48 of identify device information, 40 ASCII chars */
	u16 dma_lba_iod_ios_timer;
	/* b15-b14:word49_bit14_15 */
	/* b13    : standbyTimerSupported */
	/* b12    : word49_bit12 */
	/* b11    : IORDYSupported */
	/* b10     : IORDYDisabled */
	/* b9     : lbaSupported */
	/* b8     : dmaSupported */
	/* b7-b0  : retired */
	u16 word_50_52[3];		/**< word 50 to 52 of identify device information, 40 ASCII chars */
	u16 valid_w88_w70;
	/* b15-3  : word53_bit3_15 */
	/* b2     : validWord88  */
	/* b1     : validWord70_64  */
	/* b0     : word53_bit0  */
	u16 word_54_59[6];		/**< word54-59 of identify device information  */
	u16 user_addressable_sectors_lo;
				     /**< word60 of identify device information  */
	u16 user_addressable_sectors_hi;
				     /**< word61 of identify device information  */
	u16 word_62_74[13];		/**< word62-74 of identify device information  */
	u16 queue_depth;
	/* b15-5  : word75_bit5_15 */
	/* b4-0   : queueDepth */
	u16 sata_cap;
	/* b15-b11: word76_bit11_15  */
	/* b10    : phyEventCountersSupport */
	/* b9     : hostInitPowerMangment */
	/* b8     : nativeCommandQueuing */
	/* b7-b3  : word76_bit4_7 */
	/* b2     : sataGen2Supported (3.0 Gbps) */
	/* b1     : sataGen1Supported (1.5 Gbps) */
	/* b0      :word76_bit0 */
	u16 word_77;			/**< word77 of identify device information */
	u16 sata_features_supported;
	/* b15-b7 : word78_bit7_15 */
	/* b6     : softSettingPreserveSupported */
	/* b5     : word78_bit5 */
	/* b4     : inOrderDataDeliverySupported */
	/* b3     : devInitPowerManagementSupported */
	/* b2     : autoActiveDMASupported */
	/* b1     : nonZeroBufOffsetSupported */
	/* b0     : word78_bit0  */
	u16 sata_features_enabled;
	/* b15-7  : word79_bit7_15  */
	/* b6     : softSettingPreserveEnabled */
	/* b5     : word79_bit5  */
	/* b4     : inOrderDataDeliveryEnabled */
	/* b3     : devInitPowerManagementEnabled */
	/* b2     : autoActiveDMAEnabled */
	/* b1     : nonZeroBufOffsetEnabled */
	/* b0     : word79_bit0 */
	u16 major_ver_num;
	/* b15    : word80_bit15 */
	/* b14    : supportATA_ATAPI14 */
	/* b13    : supportATA_ATAPI13 */
	/* b12    : supportATA_ATAPI12 */
	/* b11    : supportATA_ATAPI11 */
	/* b10    : supportATA_ATAPI10 */
	/* b9     : supportATA_ATAPI9  */
	/* b8     : supportATA_ATAPI8  */
	/* b7     : supportATA_ATAPI7  */
	/* b6     : supportATA_ATAPI6  */
	/* b5     : supportATA_ATAPI5  */
	/* b4     : supportATA_ATAPI4 */
	/* b3     : supportATA3 */
	/* b2-0   : word80_bit0_2 */
	u16 minor_ver_num;	  /**< word81 of identify device information */
	u16 cmd_set_supported;
	/* b15    : word82_bit15 */
	/* b14    : NOPSupported */
	/* b13    : READ_BUFFERSupported */
	/* b12    : WRITE_BUFFERSupported */
	/* b11    : word82_bit11 */
	/* b10    : hostProtectedAreaSupported */
	/* b9     : DEVICE_RESETSupported */
	/* b8     : SERVICEInterruptSupported */
	/* b7     : releaseInterruptSupported */
	/* b6     : lookAheadSupported */
	/* b5     : writeCacheSupported */
	/* b4     : word82_bit4 */
	/* b3     : mandPowerManagmentSupported */
	/* b2     : removableMediaSupported */
	/* b1     : securityModeSupported */
	/* b0     : SMARTSupported */
	u16 cmd_set_supported_1;
	/* b15-b14: word83_bit14_15  */
	/* b13    : FLUSH_CACHE_EXTSupported  */
	/* b12    : mandatoryFLUSH_CACHESupported */
	/* b11    : devConfOverlaySupported */
	/* b10    : address48BitsSupported */
	/* b9     : autoAcousticManageSupported */
	/* b8     : SET_MAX_SecurityExtSupported */
	/* b7     : word83_bit7 */
	/* b6     : SET_FEATUREReqSpinupSupported */
	/* b5     : powerUpInStandyBySupported */
	/* b4     : removableMediaStNotifSupported */
	/* b3     : advanPowerManagmentSupported */
	/* b2     : CFASupported */
	/* b1     : DMAQueuedSupported */
	/* b0     : DOWNLOAD_MICROCODESupported */
	u16 cmd_set_feature_supported_ext;
	/* b15-b13: word84_bit13_15 */
	/* b12    : timeLimitRWContSupported */
	/* b11    : timeLimitRWSupported */
	/* b10    : writeURGBitSupported */
	/* b9     : readURGBitSupported */
	/* b8     : wwwNameSupported */
	/* b7     : WRITE_DMAQ_FUA_EXTSupported */
	/* b6     : WRITE_FUA_EXTSupported */
	/* b5     : generalPurposeLogSupported */
	/* b4     : streamingSupported  */
	/* b3     : mediaCardPassThroughSupported */
	/* b2     : mediaSerialNoSupported */
	/* b1     : SMARTSelfRestSupported */
	/* b0     : SMARTErrorLogSupported */
	u16 cmd_set_feature_enabled;
	/* b15    : word85_bit15 */
	/* b14    : NOPEnabled */
	/* b13    : READ_BUFFEREnabled  */
	/* b12    : WRITE_BUFFEREnabled */
	/* b11    : word85_bit11 */
	/* b10    : hostProtectedAreaEnabled  */
	/* b9     : DEVICE_RESETEnabled */
	/* b8     : SERVICEInterruptEnabled */
	/* b7     : releaseInterruptEnabled */
	/* b6     : lookAheadEnabled */
	/* b5     : writeCacheEnabled */
	/* b4     : word85_bit4 */
	/* b3     : mandPowerManagmentEnabled */
	/* b2     : removableMediaEnabled */
	/* b1     : securityModeEnabled */
	/* b0     : SMARTEnabled */
	u16 cmd_set_feature_enabled_1;
	/* b15-b14: word86_bit14_15 */
	/* b13    : FLUSH_CACHE_EXTEnabled */
	/* b12    : mandatoryFLUSH_CACHEEnabled */
	/* b11    : devConfOverlayEnabled */
	/* b10    : address48BitsEnabled */
	/* b9     : autoAcousticManageEnabled */
	/* b8     : SET_MAX_SecurityExtEnabled */
	/* b7     : word86_bit7 */
	/* b6     : SET_FEATUREReqSpinupEnabled */
	/* b5     : powerUpInStandyByEnabled  */
	/* b4     : removableMediaStNotifEnabled */
	/* b3     : advanPowerManagmentEnabled */
	/* b2     : CFAEnabled */
	/* b1     : DMAQueuedEnabled */
	/* b0     : DOWNLOAD_MICROCODEEnabled */
	u16 cmd_set_feature_default;
	/* b15-b13: word87_bit13_15 */
	/* b12    : timeLimitRWContEnabled */
	/* b11    : timeLimitRWEnabled */
	/* b10    : writeURGBitEnabled */
	/* b9     : readURGBitEnabled */
	/* b8     : wwwNameEnabled */
	/* b7     : WRITE_DMAQ_FUA_EXTEnabled */
	/* b6     : WRITE_FUA_EXTEnabled */
	/* b5     : generalPurposeLogEnabled */
	/* b4     : streamingEnabled */
	/* b3     : mediaCardPassThroughEnabled */
	/* b2     : mediaSerialNoEnabled */
	/* b1     : SMARTSelfRestEnabled */
	/* b0     : SMARTErrorLogEnabled */
	u16 ultra_dma_mode;
	/* b15    : word88_bit15 */
	/* b14    : ultraDMAMode6Selected */
	/* b13    : ultraDMAMode5Selected */
	/* b12    : ultraDMAMode4Selected */
	/* b11    : ultraDMAMode3Selected */
	/* b10    : ultraDMAMode2Selected */
	/* b9     : ultraDMAMode1Selected */
	/* b8     : ultraDMAMode0Selected */
	/* b7     : word88_bit7  */
	/* b6     : ultraDMAMode6Supported */
	/* b5     : ultraDMAMode5Supported */
	/* b4     : ultraDMAMode4Supported */
	/* b3     : ultraDMAMode3Supported */
	/* b2     : ultraDMAMode2Supported */
	/* b1     : ultraDMAMode1Supported */
	/* b0     : ultraDMAMode0Supported */
	u16 time_to_security_erase;
	u16 time_to_enhanced_security_erase;
	u16 curr_apm_value;
	u16 master_passw_rev_code;
	u16 hw_reset_result;
	/* b15-b14: word93_bit15_14 */
	/* b13    : deviceDetectedCBLIBbelow Vil */
	/* b12-b8 : device1 HardwareResetResult */
	/* b7-b0  : device0 HardwareResetResult */
	u16 curr_auto_accoustic_mgmt_val;
	/* b15-b8 : Vendor recommended value */
	/* b7-b0  : current value */
	u16 word_95_99[5];		/**< word85-99 of identify device information  */
	u16 max_lba_0_15;		 /**< word100 of identify device information  */
	u16 max_lba_16_31;		 /**< word101 of identify device information  */
	u16 max_lba_32_47;		 /**< word102 of identify device information  */
	u16 max_lba_48_63;		 /**< word103 of identify device information  */
	u16 word_104_107[4];		/**< word104-107 of identify device information  */
	u16 naming_authority;
	/* b15-b12: NAA_bit0_3  */
	/* b11-b0 : IEEE_OUI_bit12_23 */
	u16 naming_authority1;
	/* b15-b4 : IEEE_OUI_bit0_11 */
	/* b3-b0  : uniqueID_bit32_35 */
	u16 unique_id_bit_16_31;		  /**< word110 of identify device information  */
	u16 unique_id_bit_0_15;			  /**< word111 of identify device information  */
	u16 word_112_126[15];
	u16 rmable_media_stat;
	/* b15-b2 : word127_b16_2 */
	/* b1-b0  : supported set see ATAPI6 spec */
	u16 security_status;
	/* b15-b9 : word128_b15_9 */
	/* b8     : securityLevel */
	/* b7-b6  : word128_b7_6 */
	/* b5     : enhancedSecurityEraseSupported */
	/* b4     : securityCountExpired */
	/* b3     : securityFrozen */
	/* b2     : securityLocked */
	/* b1     : securityEnabled */
	/* b0     : securitySupported */
	u16 vendor_spec[31];
	u16 cfa_power_mode_1;
	/* b15    : word 160 supported */
	/* b14    : word160_b14 */
	/* b13    : cfaPowerRequired */
	/* b12    : cfaPowerModeDisabled */
	/* b11-b0 : maxCurrentInMa */
	u16 word_161_175[15];
	u16 curr_media_sn[30];
	u16 word_206_254[49];	       /**< word206-254 of identify device information  */
	u16 integrity_word;
	/* b15-b8 : cheksum */
	/* b7-b0  : signature */
};

/*h2d fis header(first dword)*/
struct sata_h2d_header {
	u8 fis_type;		/* fisType, set to 27h for DeviceToHostReg */
	u8 pm_port;
	/* b7   : C_bit This bit is set to one when the register transfer is 
	   due to an update of the Command register */
	/* b6-b4: reserved */
	/* b3-b0: PM Port */
	u8 command;		/* Contains the contents of the Command register of 
				   the Shadow Command Block */
	u8 features;		/* Contains the contents of the Features register of 
				   the Shadow Command Block */
};

/*h2d fis data,4 dwords after header*/
struct sata_h2d_data {
	u8 lba_low;		/* Contains the contents of the LBA Low register of the Shadow Command Block */
	u8 lba_mid;		/* Contains the contents of the LBA Mid register of the Shadow Command Block */
	u8 lab_high;		/* Contains the contents of the LBA High register of the Shadow Command Block */
	u8 device;		/* Contains the contents of the Device register of the Shadow Command Block */

	u8 lba_low_exp;		/* Contains the contents of the expanded address field of the 
				   Shadow Command Block */
	u8 lba_mid_exp;		/* Contains the contents of the expanded address field of the 
				   Shadow Command Block */
	u8 lba_high_exp;	/* Contains the contents of the expanded address field of the 
				   Shadow Command Block */
	u8 features_exp;	/* Contains the contents of the expanded address field of the 
				   Shadow Command Block */

	u8 sector_cnt;		/* Contains the contents of the Sector Count register of the 
				   Shadow Command Block */
	u8 sector_cnt_exp;	/* Contains the contents of the expanded address field of 
				   the Shadow Command Block */
	u8 reserved_4;		/* Reserved */
	u8 control;		/* Contains the contents of the Device Control register of the 
				   Shadow Command Block */
	u32 reserved_5;		/* Reserved */
};

/*d2h fis define,consists of d2h header and d2h data combins*/
struct sata_h2d {
	struct sata_h2d_header header;
	struct sata_h2d_data data;
};

struct sata_d2h_header {
	u8 fis_type;		/* fisType, set to 34h for DeviceToHostReg */
	u8 pm_port;
	/* b7     : reserved */
	/* b6     : I Interrupt bit */
	/* b5-b4  : reserved */
	/* b3-b0  : PM Port */
	u8 status;		/* Contains the contents to be placed in the Status(and Alternate status) 
				   Register of the Shadow Command Block */
	u8 err;			/* Contains the contents to be placed in the Error register of the Shadow Command Block */
};

struct sata_d2h_data {
	u8 lba_low;		/* Contains the contents to be placed in the LBA Low register 
				   of the Shadow Command Block */
	u8 lba_mid;		/* Contains the contents to be placed in the LBA Mid register 
				   of the Shadow Command Block */

	u8 lab_high;		/* Contains the contents to be placed in the LBA High register 
				   of the Shadow Command Block */
	u8 device;		/* Contains the contents to be placed in the Device register of the Shadow Command Block */

	u8 lba_low_exp;		/* Contains the contents of the expanded address field 
				   of the Shadow Command Block */
	u8 lba_mid_exp;		/* Contains the contents of the expanded address field 
				   of the Shadow Command Block */
	u8 lba_high_exp;	/* Contains the contents of the expanded address field 
				   of the Shadow Command Block */
	u8 reserved_4; /** reserved */

	u8 sector_cnt;		/* Contains the contents to be placed in the Sector 
				   Count register of the Shadow Command Block */
	u8 sector_cnt_exp;	/* Contains the contents of the expanded address 
				   field of the Shadow Command Block */
	u8 reserved_5;		/* Reserved */
	u8 reserved_6;		/* Reserved */
	u32 reserved_7;		/* Reserved */
};

struct sata_d2h {
	struct sata_d2h_header header;
	struct sata_d2h_data data;
};

/*Fix format sense data*/
struct fix_fmt_sns_data {
	u8 resp_code;		/*byte 0,bit 0-6:Response Code,bit 7:Valid */
	u8 reserved;		/*byte 1,reserved */
	u8 sns_key;		/*byte 2,bit 0-3 Sense Key,bit 4:reserved,bit 5:incorrect
				   *length indicator(for read long/write long etc),bit 6: end
				   *of media,bit 7:Filemark */
	u8 info[4];		/*byte 3-6:information field */
	u8 add_sns_len;		/*byte 7:additional sense length */
	u8 cmd_spec[4];		/*byte 8-11:command specific information */
	u8 asc;			/*byte 12:Additional Sense Code */
	u8 ascq;		/*byte 13:Additional Sense Code Qualifier */
	u8 fruc;		/*byte 14:Field Replaceable Unit Code */
	u8 sks[3];		/*byte 15 bit 0-6,byte 16,byte 17:Sense Key Specific;byte
				   *15 bit 7:sense-key specific valid */
	//lint -e43
	u8 add_sns[0];		/*byte 18-n:Additional Sense Bytes */
	//lint -e43
};

/*descriptor format sense data*/
struct dsc_fmt_sns_data {
	u8 resp_code;		/*byte 0,bit 0-6:Response Code,bit 7:reserved */
	u8 sns_key;		/*byte 1,bit 0-3 Sense Key,bit 4-7:reserved, */
	u8 asc;			/*byte 2:Additional Sense Code */
	u8 ascq;		/*byte 3:Additional Sense Code Qualifier */
	u8 reserved[3];		/*byte 4-6 reserved */
	u8 add_sns_len;		/*byte 7:additional sense length */
	u8 add_sns[0];		/*byte 8-n:Sense data descriptors */
};

/*sense data union*/
union sns_data {
	struct fix_fmt_sns_data fix_fmt;
	struct dsc_fmt_sns_data dsc_fmt;
};

extern void sat_init_trans_array(void);

#endif
