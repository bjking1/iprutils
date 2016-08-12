#ifndef iprlib_h
#define iprlib_h
/**
 * IBM IPR adapter utility library
 *
 * (C) Copyright 2000, 2004
 * International Business Machines Corporation and others.
 * All Rights Reserved. This program and the accompanying
 * materials are made available under the terms of the
 * Common Public License v1.0 which accompanies this distribution.
 *
 */

/*
 * $Header: /cvsroot/iprdd/iprutils/iprlib.h,v 1.107 2009/06/30 23:32:40 wboyer Exp $
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mount.h>
#include <sys/utsname.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <scsi/scsi.h>
#include <scsi/scsi_ioctl.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/file.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <ctype.h>
#include <syslog.h>
#include <term.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <asm/byteorder.h>
#include <sys/mman.h>
#include <paths.h>
#include <linux/netlink.h>
#include <time.h>
#include <limits.h>
#include <zlib.h>
#include <sys/param.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define IPR_DASD_UCODE_USRLIB                0
#define IPR_DASD_UCODE_ETC                   1

#define IPR_DASD_UCODE_NOT_FOUND             -1
#define IPR_DASD_UCODE_NO_HDR                1
#define IPR_DASD_UCODE_HDR                   2

#define UCODE_BASE_DIR                       "/usr/lib/microcode"
#define LINUX_UCODE_BASE_DIR                 "/lib/firmware"

#define FIRMWARE_HOTPLUG_DIR_TAG             "FIRMWARE_DIR"
#define FIRMWARE_HOTPLUG_CONFIG_FILE         "/etc/hotplug/firmware.agent"
#define FIRMWARE_HOTPLUG_DEFAULT_DIR         LINUX_UCODE_BASE_DIR

#define IPRDUMP_DIR "/var/log/"

#define IPR_JBOD_BLOCK_SIZE                  512
#define IPR_DEFAULT_AF_BLOCK_SIZE            522
#define IPR_JBOD_4K_BLOCK_SIZE               4096
#define IPR_AF_4K_BLOCK_SIZE                 4224
#define IOCTL_BUFFER_SIZE                    512
#define IPR_MAX_NUM_BUSES                    4
#define IPR_VENDOR_ID_LEN                    8
#define IPR_PROD_ID_LEN                      16
#define IPR_SERIAL_NUM_LEN                   8
#define ESM_SERIAL_NUM_LEN                   12
#define YL_SERIAL_NUM_LEN                    12
#define IPR_DESCRIPTION_LEN                  16
#define IPR_VPD_PLANT_CODE_LEN               4
#define IPR_VPD_CACHE_SIZE_LEN               3
#define IPR_VPD_DRAM_SIZE_LEN                3
#define IPR_VPD_PART_NUM_LEN                 12
#define IPR_CCB_CDB_LEN                      16
#define IPR_QAC_BUFFER_SIZE                  200000
#define IPR_SIS32_QAC_BUFFER_SIZE            16000
#define IPR_INVALID_ARRAY_ID                 0xFF
#define IPR_IOA_RESOURCE_HANDLE              0xffffffff
#define IPR_RECLAIM_NUM_BLOCKS_MULTIPLIER    256
#define IPR_SDB_CHECK_AND_QUIESCE            0x00
#define IPR_SDB_CHECK_ONLY                   0x40
#define IPR_SDB_CHECK_AND_QUIESCE_ENC        0x0e
#define IPR_RDB_UNQUIESCE_AND_REBALANCE      0x20
#define IPR_MAX_NUM_SUPP_INQ_PAGES           36
#define SES_MAX_NUM_SUPP_INQ_PAGES           7
#define IPR_DUMP_TRACE_ENTRY_SIZE            8192
#define IPR_MODE_SENSE_LENGTH                255
#define IPR_DEFECT_LIST_HDR_LEN              4
#define IPR_FORMAT_DATA                      0x10
#define IPR_FORMAT_IMMED                     2
#define IPR_ARRAY_CMD_TIMEOUT                (2 * 60)      /* 2 minutes */
#define IPR_INTERNAL_DEV_TIMEOUT             (2 * 60)      /* 2 minutes */
#define IPR_FORMAT_UNIT_TIMEOUT              (3 * 60 * 60) /* 3 hours */
#define IPR_INTERNAL_TIMEOUT                 (30)          /* 30 seconds */
#define IPR_SUSPEND_DEV_BUS_TIMEOUT          (35)          /* 35 seconds */
#define IPR_EVALUATE_DEVICE_TIMEOUT          (2 * 60)      /* 2 minutes */
#define IPR_WRITE_BUFFER_TIMEOUT             (8 * 60)      /* 8 minutes */
#define IPR_CACHE_RECLAIM_TIMEOUT            (10 * 60)     /* 10 minutes */
#define SET_DASD_TIMEOUTS_TIMEOUT            (2 * 60)
#define IPR_NUM_DRIVE_ELEM_STATUS_ENTRIES    50
#define IPR_DRIVE_ELEM_STATUS_EMPTY          5
#define IPR_DRIVE_ELEM_STATUS_POPULATED      1
#define IPR_DRIVE_ELEM_STATUS_UNSUPP         0
#define IPR_DRIVE_ELEM_STATUS_NO_ACCESS      8
#define IPR_TIMEOUT_SECOND_RADIX             0x0000
#define IPR_TIMEOUT_MINUTE_RADIX             0x4000
#define IPR_TIMEOUT_RADIX_MASK               0xC000
#define IPR_TIMEOUT_RADIX_IS_MINUTE(to) \
        (((to) & IPR_TIMEOUT_RADIX_MASK) == IPR_TIMEOUT_MINUTE_RADIX)
#define IPR_TIMEOUT_RADIX_IS_SECONDS(to) \
        (((to) & IPR_TIMEOUT_RADIX_MASK) == IPR_TIMEOUT_SECOND_RADIX)
#define IPR_TIMEOUT_MASK                     0x3FFF

#define IPR_IOA_DEBUG                        0xDDu
#define   IPR_IOA_DEBUG_READ_IOA_MEM         0x00u
#define   IPR_IOA_DEBUG_WRITE_IOA_MEM        0x01u
#define   IPR_IOA_DEBUG_READ_FLIT            0x03u
#define   IPR_IOA_DEBUG_ENABLE_DBG_FUNC      0x0Au
#define   IPR_IOA_DEBUG_DISABLE_DBG_FUNC     0x0Bu

#define IPR_STD_INQ_Z0_TERM_LEN              8
#define IPR_STD_INQ_Z1_TERM_LEN              12
#define IPR_STD_INQ_Z2_TERM_LEN              4
#define IPR_STD_INQ_Z3_TERM_LEN              5
#define IPR_STD_INQ_Z4_TERM_LEN              4
#define IPR_STD_INQ_Z5_TERM_LEN              2
#define IPR_STD_INQ_Z6_TERM_LEN              10
#define IPR_STD_INQ_PART_NUM_LEN             12
#define IPR_STD_INQ_EC_LEVEL_LEN             10
#define IPR_STD_INQ_FRU_NUM_LEN              12

#define IPR_START_STOP_STOP                  0x00
#define IPR_START_STOP_START                 0x01
#define IPR_READ_CAPACITY_16                 0x10
#define IPR_SERVICE_ACTION_IN                0x9E
#define IPR_MAINTENANCE_IN                   0xA3
#define IPR_QUERY_MULTI_ADAPTER_STATUS       0x01
#define IPR_MAINTENANCE_OUT                  0xA4
#define IPR_CHANGE_MULTI_ADAPTER_ASSIGNMENT  0x02
#define IPR_QUERY_RESOURCE_STATE             0xC2
#define IPR_QUERY_COMMAND_STATUS             0xCB
#define IPR_SUSPEND_DEV_BUS                  0xC8
#define IPR_RESUME_DEV_BUS                   0xC9
#define IPR_IOA_SERVICE_ACTION               0xD2
#define IPR_QUERY_RES_ADDR_ALIASES           0x10
#define IPR_QUERY_RES_PATH_ALIASES           0x20
#define IPR_DISRUPT_DEVICE                   0x11
#define IPR_QUERY_SAS_EXPANDER_INFO          0x12
#define IPR_QUERY_RES_REDUNDANCY_INFO        0x13
#define IPR_QUERY_RES_REDUNDANCY_INFO64      0x23
#define IPR_CHANGE_CACHE_PARAMETERS          0x14
#define IPR_QUERY_CACHE_PARAMETERS           0x16
#define IPR_LIVE_DUMP                        0x31
#define IPR_QUERY_IOA_DEV_PORT               0x32
#define IPR_EVALUATE_DEVICE                  0xE4
#define SKIP_READ                            0xE8
#define SKIP_WRITE                           0xEA
#define QUERY_DASD_TIMEOUTS                  0xEB
#define SET_DASD_TIMEOUTS                    0xEC
#define IPR_MIGRATE_ARRAY_PROTECTION         0xEF
#define IPR_QUERY_ARRAY_CONFIG               0xF0
#define IPR_START_ARRAY_PROTECTION           0xF1
#define IPR_STOP_ARRAY_PROTECTION            0xF2
#define IPR_RESYNC_ARRAY_PROTECTION          0xF3
#define IPR_ADD_ARRAY_DEVICE                 0xF4
#define IPR_REMOVE_ARRAY_DEVICE              0xF5
#define IPR_REBUILD_DEVICE_DATA              0xF6
#define IPR_RECLAIM_CACHE_STORE              0xF8
#define IPR_SET_ARRAY_ASYMMETRIC_ACCESS      0xFA
#define IPR_RECLAIM_ACTION                   0x68
#define IPR_RECLAIM_PERFORM                  0x00
#define IPR_RECLAIM_RESET_BATTERY_ERROR      0x08
#define IPR_RECLAIM_EXTENDED_INFO            0x10
#define IPR_RECLAIM_QUERY                    0x20
#define IPR_RECLAIM_RESET                    0x40
#define IPR_RECLAIM_FORCE_BATTERY_ERROR      0x60
#define IPR_RECLAIM_UNKNOWN_PERM             0x80
#define SET_SUPPORTED_DEVICES                0xFB
#define IPR_STD_INQUIRY                      0xFF
#ifndef REPORT_LUNS
#define REPORT_LUNS                          0xA0
#endif

#define IPR_XLATE_DEV_FMT_RC(rc)	((((rc) & 127) == 51) ? -EIO : 0)
#define IPR_TYPE_AF_DISK                     0xC
#define IPR_TYPE_ADAPTER                     0x1f
#define IPR_TYPE_EMPTY_SLOT                  0xff

#define IPR_ACTIVE_OPTIMIZED                 0x0
#define IPR_ACTIVE_NON_OPTIMIZED             0x1
#define IPR_ACTIVE_STANDBY                   0x2
#define IPR_CLEAR_ASYMMETRIC_STATE           0x0
#define IPR_PRESERVE_ASYMMETRIC_STATE        0x80

#define IPR_IOA_CACHING_DUAL_FAILURE_DISABLED  0x0
#define IPR_IOA_CACHING_DUAL_FAILURE_ENABLED   0x1
#define IPR_IOA_REQUESTED_CACHING_DEFAULT    0x0
#define IPR_IOA_REQUESTED_CACHING_DISABLED   0x1

#define IPR_IOA_CACHING_DEFAULT_DUAL_ENABLED   0x2
#define IPR_IOA_CACHING_DISABLED_DUAL_ENABLED  0x3

#define IPR_IOA_VSET_CACHE_ENABLED		0x4

#define IPR_IOA_SET_CACHING_DEFAULT          0x0
#define IPR_IOA_SET_CACHING_DISABLED         0x10
#define IPR_IOA_SET_CACHING_DUAL_DISABLED    0x20
#define IPR_IOA_SET_CACHING_DUAL_ENABLED     0x30
#define IPR_IOA_SET_VSET_CACHE_ENABLED	     0x40
#define IPR_IOA_SET_VSET_CACHE_DISABLED      0x50

#define PHYSICAL_LOCATION_LENGTH	1024

#define IPR_HDD                              0x0
#define IPR_SSD                              0x1
#define IPR_BLK_DEV_CLASS_4K                 0x4
#define IPR_RI				     0x1

#define IPR_ARRAY_VIRTUAL_BUS			0x1
#define IPR_VSET_VIRTUAL_BUS			0x2
#define IPR_IOAFP_VIRTUAL_BUS			0x3

#define IPR_SAS_STD_INQ_UCODE_ID		12
#define IPR_SAS_STD_INQ_VENDOR_UNIQ		40
#define IPR_SAS_STD_INQ_PLANT_MAN		4
#define IPR_SAS_STD_INQ_DATE_MAN		5
#define IPR_SAS_STD_INQ_FRU_COUNT		4
#define IPR_SAS_STD_INQ_FRU_FIELD_LEN		2
#define IPR_SAS_STD_INQ_FRU_PN			12
#define IPR_SAS_STD_INQ_ASM_EC_LVL		10
#define IPR_SAS_STD_INQ_ASM_PART_NUM		12
#define IPR_SAS_STD_INQ_FRU_ASM_EC		10
#define IPR_SAS_INQ_BYTES_WARRANTY_LEN		3

/* Device write cache policies. */
enum {IPR_DEV_CACHE_WRITE_THROUGH = 0, IPR_DEV_CACHE_WRITE_BACK};

/* System P Operating modes */
enum system_p_mode {POWER_VM, POWER_KVM, POWER_BAREMETAL} ;

#define  IPR_IS_DASD_DEVICE(std_inq_data) \
((((std_inq_data).peri_dev_type) == TYPE_DISK) && !((std_inq_data).removeable_medium))

#define IPR_SET_MODE(change_mask, cur_val, new_val)  \
{                                                       \
int mod_bits = (cur_val ^ new_val);                     \
if ((change_mask & mod_bits) == mod_bits)               \
{                                                       \
cur_val = new_val;                                      \
}                                                       \
}

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define IPR_RECORD_ID_SUPPORTED_ARRAYS       __constant_be16_to_cpu((u16)0)
#define IPR_RECORD_ID_COMP_RECORD            __constant_be16_to_cpu((u16)3)
#define IPR_RECORD_ID_ARRAY_RECORD           __constant_be16_to_cpu((u16)4)
#define IPR_RECORD_ID_DEVICE_RECORD          __constant_be16_to_cpu((u16)5)
#define IPR_RECORD_ID_ARRAY_RECORD_3         __constant_be16_to_cpu((u16)6)
#define IPR_RECORD_ID_VSET_RECORD_3          __constant_be16_to_cpu((u16)7)
#define IPR_RECORD_ID_DEVICE_RECORD_3        __constant_be16_to_cpu((u16)8)

extern struct ipr_array_query_data *ipr_array_query_data;
extern int num_ioas;
extern struct ipr_ioa *ipr_ioa_head;
extern struct ipr_ioa *ipr_ioa_tail;
extern int runtime;
extern void (*exit_func) (void);
extern int daemonize;
extern int ipr_debug;
extern int ipr_force;
extern int ipr_sg_required;
extern int polling_mode;
extern int ipr_fast;
extern int format_done;
extern char *tool_name;
extern struct sysfs_dev *head_zdev;
extern struct sysfs_dev *tail_zdev;
extern enum system_p_mode power_cur_mode;

struct sysfs_dev {
	u64 device_id;
	struct sysfs_dev *next, *prev;
};

struct ipr_phy {
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 box:3;
	u8 phy:5;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 phy:5;
	u8 box:3;
#endif
};

struct ipr_phy_2bit_hop {
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 box:2;
	u8 phy:6;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 phy:6;
	u8 box:2;
#endif
};

struct ipr_res_addr {
	u8 host;
	u8 bus;
	union {
		u8 target;
		struct ipr_phy phy;
		struct ipr_phy_2bit_hop phy_2bit_hop;
	};
	u8 lun;
#define IPR_GET_PHYSICAL_LOCATOR(res_addr) \
	(((res_addr)->bus << 16) | ((res_addr)->target << 8) | (res_addr)->lun)
};

struct ipr_res_path {
	u8 res_path_bytes[8];
};

struct ipr_std_inq_vpids {
	u8 vendor_id[IPR_VENDOR_ID_LEN];          /* Vendor ID */
	u8 product_id[IPR_PROD_ID_LEN];           /* Product ID */
};

struct ipr_common_record {
	u16 record_id;
	u16 record_len;
};

struct ipr_vset_res_state {
	u16 stripe_size;
	u8 prot_level;
	u8 num_devices_in_vset;
	u32 reserved6;
	u32 ilid;
	u32 failing_dev_ioasc;
	struct ipr_res_addr failing_dev_res_addr;
	u32 failing_dev_res_handle;
	u8 prot_level_str[8];
};

struct ipr_dasd_res_state {
	u32 data_path_width;  /* bits */
	u32 data_xfer_rate;   /* 100 KBytes/second */
	u32 ilid;
	u32 failing_dev_ioasc;
	struct ipr_res_addr failing_dev_res_addr;
	u32 failing_dev_res_handle;
};

struct ipr_gscsi_res_state {
	u32 data_path_width;  /* bits */
	u32 data_xfer_rate;   /* 100 KBytes/second */
};

struct ipr_supported_device {
	u16 data_length;
	u8 reserved;
	u8 num_records;
	struct ipr_std_inq_vpids vpids;
	u8 reserved2[16];
};

struct ipr_mode_page_hdr {
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 parms_saveable:1;
	u8 reserved1:1;
	u8 page_code:6;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 page_code:6;
	u8 reserved1:1;
	u8 parms_saveable:1;
#endif
	u8 page_length;
};

struct ipr_vendor_mode_page {
	/* Mode page 0x00 */
	struct ipr_mode_page_hdr hdr;
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 qpe:1;
	u8 uqe:1;
	u8 dwd:1;
	u8 reserved1:4;
	u8 arhes:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 arhes:1;
	u8 reserved1:4;
	u8 dwd:1;
	u8 uqe:1;
	u8 qpe:1;
#endif

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 asdpe:1;
	u8 reserved2:1;
	u8 cmdac:1;
	u8 rpfae:1;
	u8 dotf:1;
	u8 reserved3:1;
	u8 rrnde:1;
	u8 cpe:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 cpe:1;
	u8 rrnde:1;
	u8 reserved3:1;
	u8 dotf:1;
	u8 rpfae:1;
	u8 cmdac:1;
	u8 reserved2:1;
	u8 asdpe:1;
#endif

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 reserved4:6;
	u8 dwlro:1;
	u8 dlro:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 dlro:1;
	u8 dwlro:1;
	u8 reserved4:6;
#endif

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 reserved5:2;
	u8 dsn:1;
	u8 frdd:1;
	u8 dpsdp:1;
	u8 wpen:1;
	u8 caen:1;
	u8 ovple:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 ovple:1;
	u8 caen:1;
	u8 wpen:1;
	u8 dpsdp:1;
	u8 frdd:1;
	u8 dsn:1;
	u8 reserved5:2;
#endif

	u8 reserved7[2];

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 reserved8:1;
	u8 adc:1;
	u8 qemc:1;
	u8 drd:1;
	u8 led_mode:4;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 led_mode:4;
	u8 drd:1;
	u8 qemc:1;
	u8 adc:1;
	u8 reserved8:1;
#endif

	u8 temp_threshold;
	u8 cmd_aging_limit_hi;
	u8 cmd_aging_limit_lo;
	u8 qpe_read_threshold;
	u8 reserved10;

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 drrt:1;
	u8 dnr:1;
	u8 reserved11:1;
	u8 rarr:1;
	u8 ffmt:1;
	u8 reserved12:3;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 reserved12:3;
	u8 ffmt:1;
	u8 rarr:1;
	u8 reserved11:1;
	u8 dnr:1;
	u8 drrt:1;
#endif

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 rtp:1;
	u8 rrc:1;
	u8 fcert:1;
	u8 reserved13:1;
	u8 drpdv:1;
	u8 dsf:1;
	u8 irt:1;
	u8 ivr:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 ivr:1;
	u8 irt:1;
	u8 dsf:1;
	u8 drpdv:1;
	u8 reserved13:1;
	u8 fcert:1;
	u8 rrc:1;
	u8 rtp:1;
#endif
};

struct ipr_caching_parameters_page {
	/* Mode page 0x08 */
	struct ipr_mode_page_hdr hdr;
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 ic:1;
	u8 abpf:1;
	u8 cap:1;
	u8 disc:1;
	u8 size:1;
	u8 wce:1;
	u8 mf:1;
	u8 rcd:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 rcd:1;
	u8 mf:1;
	u8 wce:1;
	u8 size:1;
	u8 disc:1;
	u8 cap:1;
	u8 abpf:1;
	u8 ic:1;
#endif

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 demand_read_retention_priority:4;
	u8 write_retention_priority:4;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 write_retention_priority:4;
	u8 demand_read_retention_priority:4;
#endif

	u16 disable_prefetch_transfer_length;
	u16 minimum_prefetch;
	u16 maximum_prefetch;
	u16 maximum_prefetch_ceiling;

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 fsw:1;
	u8 lbcss:1;
	u8 dra:1;
	u8 reserved:5;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 reserved:5;
	u8 dra:1;
	u8 lbcss:1;
	u8 fsw:1;
#endif

	u8 cache_segments;
	u16 cache_segment_size;
	u8 reserved1;
	u8 non_cache_segment_size[3];
};

struct ipr_control_mode_page {
	/* Mode page 0x0A */
	struct ipr_mode_page_hdr hdr;
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 tst:3;
	u8 reserved1:3;
	u8 gltsd:1;
	u8 rlec:1;
	u8 queue_algorithm_modifier:4;
	u8 reserved2:1;
	u8 qerr:2;
	u8 dque:1;
	u8 reserved3:1;
	u8 rac:1;
	u8 reserved4:2;
	u8 swp:1;
	u8 raerp:1;
	u8 uaaerp:1;
	u8 eaerp:1;
	u8 ato:1;
	u8 tas:1;
	u8 reserved5:3;
	u8 autoload_mode:3;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 rlec:1;
	u8 gltsd:1;
	u8 reserved1:3;
	u8 tst:3;
	u8 dque:1;
	u8 qerr:2;
	u8 reserved2:1;
	u8 queue_algorithm_modifier:4;
	u8 eaerp:1;
	u8 uaaerp:1;
	u8 raerp:1;
	u8 swp:1;
	u8 reserved4:2;
	u8 rac:1;
	u8 reserved3:1;
	u8 autoload_mode:3;
	u8 reserved5:3;
	u8 tas:1;
	u8 ato:1;
#endif
	u16 ready_aen_holdoff_period;
	u16 busy_timeout_period;
	u16 reserved6;
};

struct ipr_rw_err_mode_page {
	/* Page code 0x01 */
	struct ipr_mode_page_hdr hdr;
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 awre:1;
	u8 arre:1;
	u8 tb:1;
	u8 rc:1;
	u8 eer:1;
	u8 per:1;
	u8 dte:1;
	u8 dcr:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 dcr:1;
	u8 dte:1;
	u8 per:1;
	u8 eer:1;
	u8 rc:1;
	u8 tb:1;
	u8 arre:1;
	u8 awre:1;
#endif
	u8 read_retry_count;
	u8 correction_span;
	u8 head_offset_count;
	u8 data_strobe_offset_count;
	u8 reserved1;
	u8 write_retry_count;
	u8 reserved2;
	u16 recovery_time_limit;
};

struct ipr_reclaim_query_data {
	u8 action_status;
#define IPR_ACTION_SUCCESSFUL               0
#define IPR_ACTION_NOT_REQUIRED             1
#define IPR_ACTION_NOT_PERFORMED            2

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 reclaim_known_needed:1;
	u8 reclaim_unknown_needed:1;
	u8 reserved2:2;
	u8 reclaim_known_performed:1;
	u8 reclaim_unknown_performed:1;
	u8 reserved3:1;
	u8 num_blocks_needs_multiplier:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 num_blocks_needs_multiplier:1;
	u8 reserved3:1;
	u8 reclaim_unknown_performed:1;
	u8 reclaim_known_performed:1;
	u8 reserved2:2;
	u8 reclaim_unknown_needed:1;
	u8 reclaim_known_needed:1;
#endif
	u16 num_blocks;

	u8 rechargeable_battery_type;
#define IPR_BATTERY_TYPE_NO_BATTERY          0
#define IPR_BATTERY_TYPE_NICD                1
#define IPR_BATTERY_TYPE_NIMH                2
#define IPR_BATTERY_TYPE_LIION               3

	u8 rechargeable_battery_error_state;
#define IPR_BATTERY_NO_ERROR_STATE           0
#define IPR_BATTERY_WARNING_STATE            1
#define IPR_BATTERY_ERROR_STATE              2

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 conc_maint_battery:1;
	u8 battery_replace_allowed:1;
	u8 reserved4:6;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 reserved4:6;
	u8 battery_replace_allowed:1;
	u8 conc_maint_battery:1;
#endif

	u8 reserved5;

	u16 raw_power_on_time;
	u16 adjusted_power_on_time;
	u16 estimated_time_to_battery_warning;
	u16 estimated_time_to_battery_failure;

	u8 reserved6[240];
};

struct ipr_sdt_entry {
	u32 bar_str_offset;
	u32 end_offset;
	u8  entry_byte;
	u8  reserved[3];
#if defined (__BIG_ENDIAN_BITFIELD)
	u8  endian:1;
	u8  reserved1:1;
	u8  valid_entry:1;
	u8  reserved2:5;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8  reserved2:5;
	u8  valid_entry:1;
	u8  reserved1:1;
	u8  endian:1;
#endif
	u8  resv;
	u16 priority;
};

struct ipr_query_res_state {
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 reserved1:1;
	u8 not_oper:1;
	u8 not_ready:1;
	u8 not_func:1;
	u8 reserved2:4;

	u8 read_write_prot:1;
	u8 reserved3:7;

	u8 prot_dev_failed:1;
	u8 prot_suspended:1;
	u8 prot_resuming:1;
	u8 degraded_oper:1;
	u8 service_req:1;
	u8 reserved4:3;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 reserved2:4;
	u8 not_func:1;
	u8 not_ready:1;
	u8 not_oper:1;
	u8 reserved1:1;

	u8 reserved3:7;
	u8 read_write_prot:1;

	u8 reserved4:3;
	u8 service_req:1;
	u8 degraded_oper:1;
	u8 prot_resuming:1;
	u8 prot_suspended:1;
	u8 prot_dev_failed:1;
#endif
	u8 reserved5;

	union {
		struct ipr_vset_res_state vset;
		struct ipr_dasd_res_state dasd;
		struct ipr_gscsi_res_state gscsi;
	};
};

struct ipr_query_io_port {
	u32 length;
#define IOA_DEV_PORT_ACTIVE	0x0
#define IOA_DEV_PORT_SUSPEND	0x1
#define IOA_DEV_PORT_PARTIAL_SUSPEND	0x2
#define IOA_DEV_PORT_UNKNOWN	0xFF
	u8 port_state;
	u8 reserved1;
	u8 reserved2;
	u8 reserved3;
};

struct ipr_res_addr_aliases {
	u32 length;
	struct ipr_res_addr res_addr[10];
};

#define for_each_ra_alias(ra, aliases) \
          for (ra = (aliases)->res_addr; \
               ra < ((aliases)->res_addr + (ntohl((aliases)->length) / sizeof(struct ipr_res_addr))) && \
               ra < ((aliases)->res_addr + ARRAY_SIZE((aliases)->res_addr)); \
               ra++)

struct ipr_res_path_aliases {
	u32 length;
	u32 reserved1;
	struct ipr_res_path res_path[10];
};

#define for_each_rp_alias(rp, aliases) \
          for (rp = (aliases)->res_path; \
               rp < ((aliases)->res_path + ((ntohl((aliases)->length)- 4) / sizeof(struct ipr_res_path))) && \
               rp < ((aliases)->res_path + ARRAY_SIZE((aliases)->res_path)); \
               rp++)

struct ipr_array_cap_entry {
	u8   prot_level;
#define IPR_DEFAULT_RAID_LVL "5"

#if defined (__BIG_ENDIAN_BITFIELD)
	u8   include_allowed:1;
	u8   reserved:5;
	u8   format_overlay_type:2;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8   format_overlay_type:2;
	u8   reserved:5;
	u8   include_allowed:1;
#endif
#define IPR_FORMAT_ADD_DEVICES 1
#define IPR_FORMAT_REMOVE_DEVICES 2

	u16  reserved2;
	u8   reserved3;
	u8   max_num_array_devices;
	u8   min_num_array_devices;
	u8   min_mult_array_devices;
	u8   min_num_per_tier;
	u8   reserved4;
	u16  supported_stripe_sizes;
	u16  reserved5;
	u16  recommended_stripe_size;
	u8   prot_level_str[8];
};

#define for_each_cap_entry(cap, supp) \
        for (cap = (supp)->entry; \
             (unsigned long)cap < ((unsigned long)((supp)->entry) + (ntohs((supp)->num_entries) * ntohs((supp)->entry_length))); \
             cap = (struct ipr_array_cap_entry *)((unsigned long)cap + ntohs((supp)->entry_length)))

struct ipr_array_record {
	struct ipr_common_record common;

#if defined (__BIG_ENDIAN_BITFIELD)
	u8  issue_cmd:1;
	u8  known_zeroed:1;
	u8  reserved1:6;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8  reserved1:6;
	u8  known_zeroed:1;
	u8  issue_cmd:1;
#endif

#if defined (__BIG_ENDIAN_BITFIELD)
	u8  saved_asym_access_state:4;
	u8  current_asym_access_state:4;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8  current_asym_access_state:4;
	u8  saved_asym_access_state:4;
#endif

#if defined (__BIG_ENDIAN_BITFIELD)
	u8  established:1;
	u8  exposed:1;
	u8  non_func:1;
	u8  high_avail:1;
	u8  no_config_entry:1;
	u8  reserved3:3;

	u8  start_cand:1;
	u8  stop_cand:1;
	u8  resync_cand:1;
	u8  migrate_cand:1;
	u8  asym_access_cand:1;
	u8  reserved4:3;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8  reserved3:3;
	u8  no_config_entry:1;
	u8  high_avail:1;
	u8  non_func:1;
	u8  exposed:1;
	u8  established:1;

	u8  reserved4:3;
	u8  asym_access_cand:1;
	u8  migrate_cand:1;
	u8  resync_cand:1;
	u8  stop_cand:1;
	u8  start_cand:1;
#endif
	union {
		struct {
			u16 stripe_size;

			u8  raid_level;
			u8  array_id;
			u32 resource_handle;
			struct ipr_res_addr resource_addr;
			struct ipr_res_addr last_resource_addr;
			u8  vendor_id[IPR_VENDOR_ID_LEN];
			u8  product_id[IPR_PROD_ID_LEN];
			u8  serial_number[8];
#if defined (__BIG_ENDIAN_BITFIELD)
			u8  block_dev_class:3;
			u8  reserved51:1;
			u8  read_intensive:1;
			u8  reserved5:3;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
			u8  reserved5:3;
			u8  read_intensive:1;
			u8  reserved51:1;
			u8  block_dev_class:3;
#endif
			u8  reserved6;
			u8  reserved7;
			u8  reserved8;
		}__attribute__((packed, aligned (4))) type2;

		struct {
			u8  reserved5;
			u8  raid_level;
			u16 stripe_size;
			u8  reserved6[4];
			u8  reserved7[16];

			u8  last_func_res_path[8];

			u8  reserved8[2];

			u8  array_id;
#if defined (__BIG_ENDIAN_BITFIELD)
			u8  block_dev_class:3;
			u8  reserved91:1;
			u8  read_intensive:1;
			u8  reserved9:3;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
			u8  reserved9:3;
			u8  read_intensive:1;
			u8  reserved91:1;
			u8  block_dev_class:3;
#endif
			u32 resource_handle;
			u8  dev_id[8];
			u8  lun[8];
			u8  wwn[16];
			u8  res_path[8];
			u8  vendor_id[IPR_VENDOR_ID_LEN];
			u8  product_id[IPR_PROD_ID_LEN];
			u8  serial_number[IPR_SERIAL_NUM_LEN];
			u8  desc[IPR_DESCRIPTION_LEN];

			u8  total_arr_size[8];
			u8  total_size_inuse[8];
			u8  max_size_to_use[8];
			u8  total_size_enc[8];
			u8  total_inuse_enc[8];
			u8  max_size_enc[8];
		}__attribute__((packed, aligned (4))) type3;
	};
}__attribute__((packed, aligned (4)));

struct ipr_dev_record {
	struct ipr_common_record common;
#if defined (__BIG_ENDIAN_BITFIELD)
	u8  issue_cmd:1;
	u8  known_zeroed:1;
	u8  reserved:6;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8  reserved:6;
	u8  known_zeroed:1;
	u8  issue_cmd:1;
#endif
#if defined (__BIG_ENDIAN_BITFIELD)
	u8  saved_asym_access_state:4;
	u8  current_asym_access_state:4;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8  current_asym_access_state:4;
	u8  saved_asym_access_state:4;
#endif

#if defined (__BIG_ENDIAN_BITFIELD)
	u8  array_member:1;
	u8  has_parity:1;
	u8  is_exposed_device:1;
	u8  is_hot_spare:1;
	u8  no_cfgte_vol:1;
	u8  no_cfgte_dev:1;
	u8  reserved2:2;

	u8  start_cand:1;
	u8  parity_cand:1;
	u8  stop_cand:1;
	u8  resync_cand:1;
	u8  include_cand:1;
	u8  exclude_cand:1;
	u8  rebuild_cand:1;
	u8  zero_cand:1;

	u8  add_hot_spare_cand:1;
	u8  rmv_hot_spare_cand:1;
	u8  migrate_array_prot_cand:1;
	u8  reserved3:5;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8  reserved2:2;
	u8  no_cfgte_dev:1;
	u8  no_cfgte_vol:1;
	u8  is_hot_spare:1;
	u8  is_exposed_device:1;
	u8  has_parity:1;
	u8  array_member:1;

	u8  zero_cand:1;
	u8  rebuild_cand:1;
	u8  exclude_cand:1;
	u8  include_cand:1;
	u8  resync_cand:1;
	u8  stop_cand:1;
	u8  parity_cand:1;
	u8  start_cand:1;

	u8  reserved3:5;
	u8  migrate_array_prot_cand:1;
	u8  rmv_hot_spare_cand:1;
	u8  add_hot_spare_cand:1;
#endif
	union {
		struct {
			u8  reserved4[2];
			u8  array_id;
			u32 resource_handle;
			struct ipr_res_addr resource_addr;
			struct ipr_res_addr last_resource_addr;

			u8  vendor_id[IPR_VENDOR_ID_LEN];
			u8  product_id[IPR_PROD_ID_LEN];
			u8  serial_number[IPR_SERIAL_NUM_LEN];

#if defined (__BIG_ENDIAN_BITFIELD)
			u8  block_dev_class:3;
			u8  reserved51:1;
			u8  read_intensive:1;
			u8  reserved5:3;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
			u8  reserved5:3;
			u8  read_intensive:1;
			u8  reserved51:1;
			u8  block_dev_class:3;
#endif
			u8  reserved6;
			u8  reserved7;
			u8  reserved8;
		}__attribute__((packed, aligned (4))) type2;
		struct {
			u8  reserved4[3];
			u8  reserved5[4];
			u8  reserved6[16];
			u8  last_func_res_path[8];
			u8  reserved7[2];
			u8  array_id;

#if defined (__BIG_ENDIAN_BITFIELD)
			u8  block_dev_class:3;
			u8  reserved81:1;
			u8  read_intensive:1;
			u8  reserved8:3;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
			u8  reserved8:3;
			u8  read_intensive:1;
			u8  reserved81:1;
			u8  block_dev_class:3;
#endif
			u32 resource_handle;
			u8  dev_id[8];
			u8  lun[8];
			u8  wwn[16];
			u8  res_path[8];
			u8  vendor_id[IPR_VENDOR_ID_LEN];
			u8  product_id[IPR_PROD_ID_LEN];
			u8  serial_number[IPR_SERIAL_NUM_LEN];
			u8  desc[IPR_DESCRIPTION_LEN];
			u8  reserved9[8];
			u8  reserved10[2];
			u8  reserved11[6];
			u8  reserved12[8];
		}__attribute__((packed, aligned (4))) type3;
	};
}__attribute__((packed, aligned (4)));

#define __for_each_qac_entry(rcd, qac, type) \
      if ((qac)->num_records) \
          for (rcd = (type *)(qac)->data; \
               ((unsigned long)rcd) < ((unsigned long)((qac)->u.buf + (qac)->resp_len)) && \
               ((unsigned long)rcd) < ((unsigned long)((qac)->u.buf + IPR_QAC_BUFFER_SIZE)); \
               rcd = (type *)((unsigned long)rcd + ntohs(((struct ipr_common_record *)rcd)->record_len)))

#define for_each_supported_arrays_rcd(rcd, qac) \
      __for_each_qac_entry(rcd, qac, struct ipr_supported_arrays) \
              if (rcd->common.record_id == IPR_RECORD_ID_SUPPORTED_ARRAYS)

#define for_each_qac_entry(rcd, qac) \
      __for_each_qac_entry(rcd, qac, struct ipr_common_record)

#define for_each_dev_rcd(rcd, qac) \
      __for_each_qac_entry(rcd, qac, struct ipr_dev_record) \
              if (ipr_is_device_record(rcd->common.record_id))

#define for_each_array_rcd(rcd, qac) \
      __for_each_qac_entry(rcd, qac, struct ipr_array_record) \
              if (ipr_is_array_record(rcd->common.record_id))

struct ipr_std_inq_data {
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 peri_qual:3;
	u8 peri_dev_type:5;

	u8 removeable_medium:1;
	u8 reserved1:7;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 peri_dev_type:5;
	u8 peri_qual:3;

	u8 reserved1:7;
	u8 removeable_medium:1;
#endif

	u8 version;

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 aen:1;
	u8 obsolete1:1;
	u8 norm_aca:1;
	u8 hi_sup:1;
	u8 resp_data_fmt:4;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 resp_data_fmt:4;
	u8 hi_sup:1;
	u8 norm_aca:1;
	u8 obsolete1:1;
	u8 aen:1;
#endif

	u8 additional_len;

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 sccs:1;
	u8 reserved2:7;

	u8 bque:1;
	u8 enc_serv:1;
	u8 vs:1;
	u8 multi_port:1;
	u8 mchngr:1;
	u8 obsolete2:2;
	u8 addr16:1;

	u8 rel_adr:1;
	u8 obsolete3:1;
	u8 wbus16:1;
	u8 sync:1;
	u8 linked:1;
	u8 trans_dis:1;
	u8 cmd_que:1;
	u8 vs2:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 reserved2:7;
	u8 sccs:1;

	u8 addr16:1;
	u8 obsolete2:2;
	u8 mchngr:1;
	u8 multi_port:1;
	u8 vs:1;
	u8 enc_serv:1;
	u8 bque:1;

	u8 vs2:1;
	u8 cmd_que:1;
	u8 trans_dis:1;
	u8 linked:1;
	u8 sync:1;
	u8 wbus16:1;
	u8 obsolete3:1;
	u8 rel_adr:1;
#endif
	struct ipr_std_inq_vpids vpids;
	u8 ros_rsvd_ram_rsvd[4];
	u8 serial_num[IPR_SERIAL_NUM_LEN];
};

struct ipr_std_inq_data_long {
	struct ipr_std_inq_data std_inq_data;
	u8 z1_term[IPR_STD_INQ_Z1_TERM_LEN];
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 reserved:4;
	u8 clocking:2;
	u8 qas:1;
	u8 ius:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 ius:1;
	u8 qas:1;
	u8 clocking:2;
	u8 reserved:4;
#endif
	u8 reserved1[41];
	u8 z2_term[IPR_STD_INQ_Z2_TERM_LEN];
	u8 z3_term[IPR_STD_INQ_Z3_TERM_LEN];
	u8 as400_service_level;
	u8 z4_term[IPR_STD_INQ_Z4_TERM_LEN];
	u8 z5_term[IPR_STD_INQ_Z5_TERM_LEN];
	u8 fru_number[IPR_STD_INQ_FRU_NUM_LEN];
	u8 ec_level[IPR_STD_INQ_EC_LEVEL_LEN];
	u8 part_number[IPR_STD_INQ_PART_NUM_LEN];
	u8 z6_term[IPR_STD_INQ_Z6_TERM_LEN];
};

struct ipr_sas_std_inq_data {
	struct ipr_std_inq_data std_inq_data;
	u8 microcode_id[IPR_SAS_STD_INQ_UCODE_ID];
	u8 reserved1;
	u8 vendor_unique[IPR_SAS_STD_INQ_VENDOR_UNIQ];

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 reserved2:5;
	u8 is_ssd:1;
	u8 near_line:1;
	u8 unlock:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 unlock:1;
	u8 near_line:1;
	u8 is_ssd:1;
	u8 reserved2:5;
#endif

	u8 plant_manufacture[IPR_SAS_STD_INQ_PLANT_MAN];
	u8 date_manufacture[IPR_SAS_STD_INQ_DATE_MAN];
	u8 vendor_unique_pn;
	u8 fru_count[IPR_SAS_STD_INQ_FRU_COUNT];
	u8 fru_field_len[IPR_SAS_STD_INQ_FRU_FIELD_LEN];
	u8 fru_pn[IPR_SAS_STD_INQ_FRU_PN];
	u8 asm_ec_level[IPR_SAS_STD_INQ_ASM_EC_LVL];
	u8 asm_part_num[IPR_SAS_STD_INQ_ASM_PART_NUM];
	u8 fru_asm_ec[IPR_SAS_STD_INQ_FRU_ASM_EC];
	u8 reserved3[6];
};

struct ipr_mode_page_28_scsi_dev_bus_attr {
	struct ipr_res_addr res_addr;

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 qas_capability:2;
	u8 enable_target_mode:1;
	u8 term_power_absent:1;
	u8 target_mode_supported:1;
	u8 lvd_to_se_transition_not_allowed:1;
	u8 reserved2:2;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 reserved2:2;
	u8 lvd_to_se_transition_not_allowed:1;
	u8 target_mode_supported:1;
	u8 term_power_absent:1;
	u8 enable_target_mode:1;
	u8 qas_capability:2;
#endif
#define IPR_MODEPAGE28_QAS_CAPABILITY_NO_CHANGE      0  
#define IPR_MODEPAGE28_QAS_CAPABILITY_DISABLE_ALL    1        
#define IPR_MODEPAGE28_QAS_CAPABILITY_ENABLE_ALL     2
	/* NOTE:   Due to current operation conditions QAS should
	 never be enabled so the change mask will be set to 0 */
#define IPR_MODEPAGE28_QAS_CAPABILITY_CHANGE_MASK    0

	u8 scsi_id;
#define IPR_MODEPAGE28_SCSI_ID_NO_CHANGE             0x80u
#define IPR_MODEPAGE28_SCSI_ID_NO_ID                 0xFFu

	u8 bus_width;
#define IPR_MODEPAGE28_BUS_WIDTH_NO_CHANGE           0

	u8 extended_reset_delay;
#define IPR_EXTENDED_RESET_DELAY                     7

	u32 max_xfer_rate;
#define IPR_MODEPAGE28_MAX_XFR_RATE_NO_CHANGE        0

	u8  min_time_delay;
#define IPR_DEFAULT_SPINUP_DELAY                     0xFFu
#define IPR_INIT_SPINUP_DELAY                        5
	u8  reserved3;
	u16 reserved4;
};

#define IPR_BUS_XFER_RATE_TO_THRUPUT(rate, width) \
        (((rate) * ((width) / 8)) / 10)
#define IPR_BUS_THRUPUT_TO_XFER_RATE(thruput, width) \
        (((thruput) * 10) / ((width) / 8))

#define IPR_CONFIG_DIR "/etc/ipr/"

#define IPR_CATEGORY_IOA "Adapter"
#define IPR_GSCSI_HA_ONLY "JBOD_ONLY_HA"
#define IPR_DUAL_ADAPTER_ACTIVE_ACTIVE "DUAL_ADAPTER_ACTIVE_ACTIVE"
#define IPR_ARRAY_REBUILD_RATE "ARRAY_REBUILD_RATE"
#define IPR_ARRAY_DISABLE_REBUILD_VERIFY "DISABLE_ARRAY_REBUILD_VERIFY"

#define IPR_CATEGORY_BUS "Bus"
#define IPR_QAS_CAPABILITY "QAS_CAPABILITY"
#define IPR_HOST_SCSI_ID "HOST_SCSI_ID"
#define IPR_BUS_WIDTH "BUS_WIDTH"
#define IPR_MAX_XFER_RATE_STR "MAX_BUS_SPEED"
#define IPR_MIN_TIME_DELAY "MIN_TIME_DELAY"

/* Disk is the old format which is only here for backwards compatibility */
#define IPR_CATEGORY_DISK "Disk"
/* Device is the new format which should be used for all new attributes */
#define IPR_CATEGORY_DEVICE "Device"
#define IPR_QUEUE_DEPTH "QUEUE_DEPTH"
#define IPR_TCQ_ENABLED "TCQ_ENABLED"
#define IPR_FORMAT_TIMEOUT "FORMAT_UNIT_TIMEOUT"
#define IPR_WRITE_CACHE_POLICY "WRITE_CACHE_POLICY"

#define IPR_MAX_XFER_RATE 320
#define IPR_SAFE_XFER_RATE 160

#define IPR_JBOD_SYSFS_UNBIND 0
#define IPR_JBOD_SYSFS_BIND   1

/* Internal return codes */
#define RC_SUCCESS          0
#define RC_FAILED          -1
#define RC_NOOP		   -2
#define RC_NOT_PERFORMED   -3
#define RC_IN_USE          -4
#define RC_CANCEL          12
#define RC_REFRESH          1
#define RC_EXIT             3

struct scsi_dev_data {
	int host;
	int channel;
	union {
		u8 id;
		struct ipr_phy phy;
		struct ipr_phy_2bit_hop phy_2bit_hop;
	};
	int lun;
	int type;
	int opens;
	int qdepth;
	int busy;
	int online;
	u32 handle;
	char vendor_id[IPR_VENDOR_ID_LEN + 1];
	char product_id[IPR_PROD_ID_LEN + 1];
	char sysfs_device_name[PATH_MAX];
	char dev_name[64];
	char gen_name[64];
#define IPR_MAX_RES_PATH_LEN		24
	char res_path[IPR_MAX_RES_PATH_LEN];
	u64 device_id;
};

struct ipr_path_entry {
	u8 state;
#define IPR_PATH_FUNCTIONAL		0
#define IPR_PATH_NOT_FUNCTIONAL		1
	u8 local_bus_num;
	u8 remote_bus_num;
	u8 reserved;
	u8 local_connection_id[8];
	u8 remote_connection_id[8];
};

struct ipr_path_entries {
	u32 num_path_entries;
	struct ipr_path_entry path[0]; /* variable length */
};

struct ipr_dual_ioa_entry {
	u32 length;
	u8 link_state;
#define IPR_IOA_LINK_STATE_NOT_OPER	0
#define IPR_IOA_LINK_STATE_OPER		1
	u8 cur_state;
#define IPR_IOA_STATE_NO_CHANGE 	0
#define IPR_IOA_STATE_PRIMARY		2
#define IPR_IOA_STATE_SECONDARY		3
#define IPR_IOA_STATE_NO_PREFERENCE	4
	u8 fmt;
	u8 multi_adapter_type;
#define IPR_IOA_MA_TYPE_UNDEFINED	0
#define IPR_IOA_MA_TYPE_DUAL_IOA	1
#define IPR_IOA_MA_TYPE_AUX_CACHE	2
	u8 reserved[2];
	u16 add_len;
	union {
		struct {
			u8 remote_vendor_id[IPR_VENDOR_ID_LEN];
			u8 remote_prod_id[IPR_PROD_ID_LEN];
			u8 remote_sn[IPR_SERIAL_NUM_LEN];
		} fmt0;

		struct {
			u8 remote_vendor_id[IPR_VENDOR_ID_LEN];
			u8 remote_prod_id[IPR_PROD_ID_LEN];
			u8 remote_sn[IPR_SERIAL_NUM_LEN];
			u8 wwid[8];
		} fmt1;
	};
};

struct ipr_phys_bus_entry {
	u8 bus_num;
	u8 reserved[3];
	u8 conn_id[8];
};

struct ipr_ioa_cap_entry {
	u32 length;
	u8 reserved;
	u8 preferred_role;
	u8 token;
	u8 reserved2[4];
	u8 num_bus_entries;
	struct ipr_phys_bus_entry bus[0];
};

struct ipr_multi_ioa_status {
	u32 length;
	u32 num_entries;
	struct ipr_ioa_cap_entry cap;
	u8 buf[16 * 1024];
/*
 *	struct ipr_dual_ioa_entry ioa[0];
 */
};

struct ipr_disk_attr {
	int queue_depth;
	int tcq_enabled;
	int format_timeout;
	int write_cache_policy;
};

struct ipr_vset_attr {
	int queue_depth;
};

struct ipr_ioa_attr {
	int preferred_primary;
	int gscsi_only_ha;
	int active_active;
	int caching;
	int rebuild_rate;
	int disable_rebuild_verify;
	int vset_write_cache;
};

#define IPR_DEV_MAX_PATHS	2
struct ipr_dev {
	char dev_name[64];
	char gen_name[64];
	char prot_level_str[8];
	u8  *vendor_id;
	u8  *product_id;
	u8  *serial_number;
	u8  array_id;
	u8  raid_level;
	u16 stripe_size;
	u32 resource_handle;
	u8  block_dev_class;
	u8  read_intensive;
	u32 is_reclaim_cand:1;
	u32 should_init:1;
	u32 init_not_allowed:1;
	u32 local_flag:1;
	u32 rescan:1;
	u8 active_suspend;
	u32 is_suspend_cand:1;
	u32 is_resume_cand:1;
	u8 write_cache_policy:1;
	u8 supports_4k:1;
	u8 supports_5xx:1;
	u8 read_c7:1;
	u8 locked:1;
	int lock_fd;
	u32 format_timeout;
	struct scsi_dev_data *scsi_dev_data;
	struct ipr_dev *ses[IPR_DEV_MAX_PATHS];
	struct ipr_res_addr res_addr[IPR_DEV_MAX_PATHS];
	struct ipr_res_path res_path[IPR_DEV_MAX_PATHS];
	char res_path_name[IPR_MAX_RES_PATH_LEN];
	struct ipr_disk_attr attr;
	char physical_location[PHYSICAL_LOCATION_LENGTH];
	union {
		struct ipr_common_record *qac_entry;
		struct ipr_dev_record *dev_rcd;
		struct ipr_array_record *array_rcd;
	};
	struct ipr_dev *alt_path;
	struct ipr_ioa *ioa;
};

#define for_each_ra(ra, dev) \
           for(ra = (dev)->res_addr; \
                (ra - ((dev)->res_addr)) < IPR_DEV_MAX_PATHS; ra++)

#define for_each_rp(rp, dev) \
           for(rp = (dev)->res_path; \
                (rp - ((dev)->res_path)) < IPR_DEV_MAX_PATHS; rp++)

enum ipr_tcq_mode {
	IPR_TCQ_DISABLE = 0,
	IPR_TCQ_FROZEN = 1,
	IPR_TCQ_NACA = 2
};

#define IPR_MAX_IOA_DEVICES        (IPR_MAX_NUM_BUSES * 45 * 2 + 1)
struct ipr_ioa {
	struct ipr_dev ioa;
	u16 ccin;
	u16 af_block_size;
	u32 host_addr;
	u32 num_raid_cmds;
	u32 msl;
	u16 num_devices;
	u8 ioa_dead:1;
	u8 nr_ioa_microcode:1;
	u8 scsi_id_changeable:1;
	u8 dual_raid_support:1;
	u8 is_secondary:1;
	u8 should_init:1;
	u8 is_aux_cache:1;
	u8 protect_last_bus:1;
	u8 gscsi_only_ha:1;
	u8 in_gscsi_only_ha:1;
	u8 asymmetric_access:1;
	u8 asymmetric_access_enabled:1;
	u8 has_cache:1;
	u8 sis64:1;
	u8 rebuild_rate:4;
	u8 disable_rebuild_verify:1;
	u8 configure_rebuild_verify:1;
	u8 has_vset_write_cache:1;
	u8 vset_write_cache:1;

#define IPR_SIS32		0x00
#define IPR_SIS64		0x01
	u8 support_4k:1;
	int can_queue;
	enum ipr_tcq_mode tcq_mode;
	u16 pci_vendor;
	u16 pci_device;
	u16 subsystem_vendor;
	u16 subsystem_device;
	u16 hop_count;
#define IPR_NOHOP            0x00
#define IPR_2BIT_HOP         0x01
#define IPR_3BIT_HOP         0x02
	char dual_state[16];
	char preferred_dual_state[16];
	int host_num;
	char pci_address[16];
	char host_name[16];
	char physical_location[1024];
	u8 yl_serial_num[YL_SERIAL_NUM_LEN];
	struct ipr_dev dev[IPR_MAX_IOA_DEVICES];
	struct ipr_multi_ioa_status ioa_status;
	struct ipr_array_query_data *qac_data;
	struct ipr_array_record *start_array_qac_entry;
	struct ipr_supported_arrays *supported_arrays;
	struct ipr_reclaim_query_data *reclaim_data;
	struct ipr_ioa *next;
	struct ipr_ioa *cmd_next;
};

#define __for_each_ioa(ioa, head) for (ioa = head; ioa; ioa = ioa->next)
#define for_each_ioa(ioa) __for_each_ioa(ioa, ipr_ioa_head)
#define for_each_primary_ioa(ioa) \
        __for_each_ioa(ioa, ipr_ioa_head) \
           if (!ioa->is_secondary)

#define for_each_secondary_ioa(ioa) \
        __for_each_ioa(ioa, ipr_ioa_head) \
           if (ioa->is_secondary)

#define for_each_sas_ioa(ioa) \
        __for_each_ioa(ioa, ipr_ioa_head) \
           if (!__ioa_is_spi(ioa))

#define for_each_dev(i, d) for (d = (i)->dev; (d - (i)->dev) < (i)->num_devices; d++)

#define for_each_hotplug_dev(i, d) \
      for_each_dev(i, d) \
           if (ipr_is_af_dasd_device(d) || ipr_is_gscsi(d))

#define for_each_af_dasd(i, d) \
      for_each_dev(i, d) \
           if (ipr_is_af_dasd_device(d))

#define for_each_dev_in_vset(v, d) \
      for_each_af_dasd((v)->ioa, d) \
           if (ipr_is_array_member(d) && d->array_id == (v)->array_id)

#define for_each_vset(i, d) \
      for_each_dev(i, d) \
           if (ipr_is_volume_set(d) && !d->array_rcd->start_cand)

#define for_each_array(i, d) \
      for_each_dev(i, d) \
           if (ipr_is_array(d) && !d->array_rcd->start_cand)

#define for_each_dvd_tape(i, d) \
      for_each_dev(i, d) \
           if (d->scsi_dev_data && (d->scsi_dev_data->type == TYPE_ROM || \
		d->scsi_dev_data->type == TYPE_TAPE))

#define for_each_ses(i, d) \
      for_each_dev(i, d) \
           if (d->scsi_dev_data && d->scsi_dev_data->type == TYPE_ENCLOSURE)

#define for_each_hot_spare(i, d) \
      for_each_dev(i, d) \
           if (ipr_is_hot_spare(d))

#define for_each_disk(i, d) \
      for_each_dev(i, d) \
          if ((d)->scsi_dev_data && (ipr_is_af_dasd_device(d) || ipr_is_gscsi(d)))

#define __for_each_disk(i, d) \
      for_each_dev(i, d) \
          if ((d)->scsi_dev_data && ((d)->scsi_dev_data->type == TYPE_DISK || \
              (d)->scsi_dev_data->type == IPR_TYPE_AF_DISK))

#define for_each_standalone_disk(i, d) \
      for_each_disk(i, d) \
           if (!ipr_is_hot_spare(d) && !ipr_is_array_member(d))

#define __for_each_standalone_disk(i, d) \
      __for_each_disk(i, d) \
           if (!ipr_is_hot_spare(d) && !ipr_is_volume_set(d) && \
               !ipr_is_array_member(d))
           
#define for_each_standalone_af_disk(i, d) \
      for_each_standalone_disk(i, d) \
          if (ipr_is_af_dasd_device(d))

#define for_each_jbod_disk(i, d) \
      for_each_disk(i, d) \
          if (ipr_is_gscsi(d))

struct ipr_dasd_inquiry_page3 {
	u8 peri_qual_dev_type;
	u8 page_code;
	u8 reserved1;
	u8 page_length;
	u8 ascii_len;
	u8 reserved2[3];
	u8 load_id[4];
	u8 release_level[4];
	u8 ptf_number[4];
	u8 patch_number[4];
};

struct ipr_array_query32_data {
	u16 resp_len;
	u8 reserved;
	u8 num_records;
	u8 data[0];
};

struct ipr_array_query64_data {
	u32 resp_len;
	u32 num_records;
	u8 data[0];
};

struct ipr_array_query_data {
	u32 resp_len;
	u32 num_records;
	u32 hdr_len;
	u8 *data;
	union {
		u8 buf[IPR_QAC_BUFFER_SIZE];
		struct ipr_array_query32_data buf32;
		struct ipr_array_query64_data buf64;
	} u;
};

struct ipr_block_desc {
	u8 num_blocks[4];
	u8 density_code;
	u8 block_length[3];
};

struct ipr_mode_parm_hdr {
	u8 length;
	u8 medium_type;
	u8 device_spec_parms;
	u8 block_desc_len;
};

struct ipr_mode_pages {
	struct ipr_mode_parm_hdr hdr;
	u8 data[255 - sizeof(struct ipr_mode_parm_hdr)];
};

struct sense_data_t {
	u8 error_code;
	u8 segment_numb;
	u8 sense_key;
	u8 info[4];
	u8 add_sense_len;
	u8 cmd_spec_info[4];
	u8 add_sense_code;
	u8 add_sense_code_qual;
	u8 field_rep_unit_code;
	u8 sense_key_spec[3];
	u8 add_sense_bytes[0];
};

struct ipr_ioa_mode_page {
	struct ipr_mode_page_hdr hdr;
	u8 reserved;
	u8 max_tcq_depth;
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 reserved1:1;
	u8 format_completed:1;
	u8 reserved2:6;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 reserved2:6;
	u8 format_completed:1;
	u8 reserved1:1;
#endif
};

struct ipr_mode_page24 {
	struct ipr_mode_page_hdr hdr;
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 dual_adapter_af:2;
	u8 reserved:6;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 reserved:6;
	u8 dual_adapter_af:2;
#endif

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 rebuild_without_verify:1;
	u8 reserved1:3;
	u8 rebuild_rate:4;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 rebuild_rate:4;
	u8 reserved1:3;
	u8 rebuild_without_verify:1;
#endif

#define MIN_ARRAY_REBUILD_RATE		0x02

#define DISABLE_DUAL_IOA_AF		0x00
#define ENABLE_DUAL_IOA_AF		0x02
#define ENABLE_DUAL_IOA_ACTIVE_ACTIVE	0x03
};

struct ipr_config_term_hdr {
	u8 term_id;
	u8 len;
};

struct ipr_subsys_config_term {
	struct ipr_config_term_hdr hdr;
#define IPR_SUBSYS_CONFIG_TERM_ID	0x02
	u8 config;
#define IPR_AFDASD_SUBSYS		0x00
#define IPR_GSCSI_ONLY_HA_SUBSYS	0x01
	u8 reserved;
};

struct ipr_mode_page25 {
	struct ipr_mode_page_hdr hdr;
	u8 reserved[2];
	struct ipr_config_term_hdr term;
	u8 data[256];
};

#define for_each_page25_term(hdr, page) \
for (hdr = &(page)->term;	\
	(char *)hdr < ((char *)(page) + (page)->hdr.page_length + sizeof((page)->hdr));	\
	hdr = (struct ipr_config_term_hdr *)((char *)hdr + (hdr)->len + sizeof(*hdr)))

struct ipr_mode_page_28_header {
	struct ipr_mode_page_hdr header;
	u8 num_dev_entries;
	u8 dev_entry_length;
};

struct ipr_mode_page_28 {
	struct ipr_mode_page_28_header page_hdr;
	struct ipr_mode_page_28_scsi_dev_bus_attr attr[0];
};

#define for_each_bus_attr(ptr, page28, i) \
      for (i = 0, ptr = page28->attr; i < page_28->page_hdr.num_dev_entries; \
           i++, ptr = (struct ipr_mode_page_28_scsi_dev_bus_attr *) \
               ((u8 *)ptr + page_28->page_hdr.dev_entry_length))

struct ipr_scsi_bus_attr {
	u32 max_xfer_rate;
	u8 qas_capability;
	u8 scsi_id;
	u8 bus_width;
	u8 extended_reset_delay;
	u8 min_time_delay;
};

struct ipr_scsi_buses {
	int num_buses;
	struct ipr_scsi_bus_attr bus[IPR_MAX_NUM_BUSES];
};

struct ipr_dasd_timeout_record {
	u8 op_code;
	u8 reserved;
	u16 timeout;
};

struct ipr_query_dasd_timeouts {
	u32 length;
	struct ipr_dasd_timeout_record record[100];
};

struct ipr_supported_arrays {
	struct ipr_common_record common;
	u16 num_entries;
	u16 entry_length;
	struct ipr_array_cap_entry entry[0];
};

struct ipr_read_cap {
	u32 max_user_lba;
	u32 block_length;
};

struct ipr_read_cap16 {
	u32 max_user_lba_hi;
	u32 max_user_lba_lo;
	u32 block_length;
};

/* Struct for disks that are unsupported or require a minimum microcode
 level prior to formatting to 522-byte sectors. */
struct unsupported_af_dasd {
	char vendor_id[IPR_VENDOR_ID_LEN + 1];
	char compare_vendor_id_byte[IPR_VENDOR_ID_LEN];
	char product_id[IPR_PROD_ID_LEN + 1];
	char compare_product_id_byte[IPR_PROD_ID_LEN];
	char lid[5];       /* Load ID - Bytes 8-11 of Inquiry Page 3 */
	char lid_mask[4];  /* Mask for certain bytes of the LID */
	uint supported_with_min_ucode_level;
	char min_ucode_level[5];
	char min_ucode_mask[4];    /* used to mask don't cares in the ucode level */
};

struct unsupported_dasd {
	char vendor_id[IPR_VENDOR_ID_LEN + 1];
	char compare_vendor_id_byte[IPR_VENDOR_ID_LEN];
	char product_id[IPR_PROD_ID_LEN + 1];
	char compare_product_id_byte[IPR_PROD_ID_LEN];
};

struct ipr_cmd_status_record {
	u16 reserved1;
	u16 length;
	u8 array_id;
	u8 command_code;
	u8 status;
#define IPR_CMD_STATUS_SUCCESSFUL            0
#define IPR_CMD_STATUS_IN_PROGRESS           2
#define IPR_CMD_STATUS_ATTRIB_CHANGE         3
#define IPR_CMD_STATUS_FAILED                4
#define IPR_CMD_STATUS_INSUFF_DATA_MOVED     5
#define IPR_CMD_STATUS_MIXED_BLK_DEV_CLASESS 6
#define IPR_CMD_STATUS_MIXED_LOG_BLK_SIZE    7
#define IPR_CMD_STATUS_UNSUPT_REQ_BLK_DEV_CLASS    8

	u8 percent_complete;
	struct ipr_res_addr failing_dev_res_addr;
	u32 failing_dev_res_handle;
	u32 failing_dev_ioasc;
	u32 ilid;
	u32 resource_handle;
	u8 vset_id;
	u8 flags;
	u16 reserved;
	u8 failing_dev_res_path[8];
};

struct ipr_cmd_status {
	u16 resp_len;
	u16 num_records;
	struct ipr_cmd_status_record record[250];
};

#define for_each_cmd_status(r, s) for (r = (s)->record; (unsigned long)(r) < ((unsigned long)((s)->record) + ntohs((s)->resp_len) - 4); r = (void *)(r) + ntohs((r)->length))

struct ipr_inquiry_page0 {
	u8 peri_qual_dev_type;
	u8 page_code;
	u8 reserved1;
	u8 page_length;
	u8 supported_page_codes[IPR_MAX_NUM_SUPP_INQ_PAGES];
};

struct ipr_inquiry_page3 {
	u8 peri_qual_dev_type;
	u8 page_code;
	u8 reserved1;
	u8 page_length;
	u8 ascii_len;
	u8 reserved2[3];
	u8 load_id[4];
	u8 major_release;
	u8 card_type;
	u8 minor_release[2];
	u8 ptf_number[4];
	u8 patch_number[4];
};

struct ipr_inquiry_ioa_cap {
	u8 peri_qual_dev_type;
	u8 page_code;
	u8 reserved1;
	u8 page_length;
	u8 ascii_len;
	u8 reserved2;
	u8 sis_version[2];
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 dual_ioa_raid:1;
	u8 dual_ioa_wcache:1;
	u8 dual_ioa_asymmetric_access:1;
	u8 reserved:2;
	u8 disable_array_rebuild_verify:1;
	u8 reserved7:2;

	u8 can_attach_to_aux_cache:1;
	u8 is_aux_cache:1;
	u8 is_dual_wide:1;
	u8 gscsi_only_ha:1;
	u8 reserved3:4;

	u8 reserved4:1;
	u8 af_4k_support:1;
	u8 reserved5:6;

	u8 reserved6:3;
	u8 ra_id_encoding:3;
	u8 sis_format:2;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 reserved7:2;
	u8 disable_array_rebuild_verify:1;
	u8 reserved:2;
	u8 dual_ioa_asymmetric_access:1;
	u8 dual_ioa_wcache:1;
	u8 dual_ioa_raid:1;

	u8 reserved3:4;
	u8 gscsi_only_ha:1;
	u8 is_dual_wide:1;
	u8 is_aux_cache:1;
	u8 can_attach_to_aux_cache:1;

	u8 reserved4:6;
	u8 af_4k_support:1;
	u8 reserved5:1;

	u8 sis_format:2;
	u8 ra_id_encoding:3;
	u8 reserved6:3;
#endif
	u16 af_block_size;
	u16 af_ext_cap;
	u16 vset_ext_cap;
	u16 gscsi_ext_cap;
};

struct ipr_dasd_ucode_header {
	u8 length[3];
	u8 load_id[4];
	u8 modification_level[4];
	u8 ptf_number[4];
	u8 patch_number[4];
};

struct ipr_ioa_ucode_img_desc {
#define IPR_IOAF_STR   "IOAF"
#define IPR_FPGA_STR   "FPGA"
#define IPR_DRAM_STR   "AGIG"
	char fw_type[4];
#define IPR_IOAF       0
#define IPR_FPGA       1
#define IPR_DRAM       2
	u8 reserved[16];
};

struct ipr_ioa_ucode_ext_header {
       char eyecatcher[8];        /* EXTDLIMG */
       u32 image_length;
       u32 flags;
       char fw_level[8];
       u8 reserved[12];
       u32 img_desc_offset;
};

struct ipr_ioa_ucode_header {
	u32 header_length;
	u32 lid_table_offset;
	u32 rev_level;
#define LINUX_HEADER_RESERVED_BYTES 20
	u8 reserved[LINUX_HEADER_RESERVED_BYTES];
	char eyecatcher[16];        /* IBMAS400 CCIN */
	u32 num_lids;
};

struct ipr_fw_images {
	char file[100];
	u32 version;
	int has_header;
	char date[20];
};

struct ipr_ioa_vpd {
	struct ipr_std_inq_data std_inq_data;
	u8 ascii_part_num[IPR_VPD_PART_NUM_LEN];
	u8 reserved[40];
	u8 ascii_plant_code[IPR_VPD_PLANT_CODE_LEN];
};

struct ipr_cfc_vpd {
	u8 peri_dev_type;
	u8 page_code;
	u8 reserved1;
	u8 add_page_len;
	u8 ascii_len;
	u8 cache_size[IPR_VPD_CACHE_SIZE_LEN];
	struct ipr_std_inq_vpids vpids;
	u8 revision_level[4];
	u8 serial_num[IPR_SERIAL_NUM_LEN];
	u8 ascii_part_num[12];
	u8 reserved3[40];
	u8 ascii_plant_code[IPR_VPD_PLANT_CODE_LEN];
};

struct ipr_dram_vpd {
	u8 peri_dev_type;
	u8 page_code;
	u8 reserved1;
	u8 add_page_len;
	u8 ascii_len;
	u8 dram_size[IPR_VPD_DRAM_SIZE_LEN];
};

struct ipr_cache_cap_vpd {
#define IPR_CACHE_CAP_VSET_WRITE_CACHE	0x08000000
	u8 peri_dev_type;
	u8 page_code;
	u8 reserved1;
	u8 add_page_len;
	u32 cache_cap;
	u32 data_store_size;
	u32 write_cache_size;
	u32 comp_write_cache_size;
	u32 read_cache_size;
	u32 comp_read_cache_size;
};

struct ipr_dev_identify_vpd {
	u8 peri_dev_type;
	u8 page_code;
	u16 add_page_len;
	u8 reserved[80];
	u8 dev_identify_contxt[40];
};

#define IPR_IOA_MAX_SUPP_LOG_PAGES		64
struct ipr_supp_log_pages {
	u8 page_code;
	u8 reserved;
	u16 length;
	u8 page[IPR_IOA_MAX_SUPP_LOG_PAGES];
};

struct ipr_sas_phy_log_desc {
	u8 reserved;
	u8 phy;
	u8 reserved2[2];

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 reseved3:1;
	u8 attached_dev_type:3;
	u8 reserved4:4;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 reserved4:4;
	u8 attached_dev_type:3;
	u8 reseved3:1;
#endif

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 reserved5:4;
	u8 link_rate:4;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 link_rate:4;
	u8 reserved5:4;
#endif

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 reserved6:4;
	u8 attached_ssp_initiator_port:1;
	u8 attached_stp_initiator_port:1;
	u8 attached_smp_initiator_port:1;
	u8 reserved7:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 reserved7:1;
	u8 attached_smp_initiator_port:1;
	u8 attached_stp_initiator_port:1;
	u8 attached_ssp_initiator_port:1;
	u8 reserved6:4;
#endif

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 reserved8:4;
	u8 attached_ssp_target_port:1;
	u8 attached_stp_target_port:1;
	u8 attached_smp_target_port:1;
	u8 reserved9:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 reserved9:1;
	u8 attached_smp_target_port:1;
	u8 attached_stp_target_port:1;
	u8 attached_ssp_target_port:1;
	u8 reserved8:4;
#endif

	u8 sas_addr[8];
	u8 attached_sas_addr[8];
	u8 attached_phy_id;
	u8 reserved10[7];
	u32 invalid_dword_count;
	u32 running_disparity_error_count;
	u32 loss_of_dword_sync_count;
	u32 phy_reset_problem_count;
};

struct ipr_sas_port_log_desc {
	u16 port_num;
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 du:1;
	u8 ds:1;
	u8 tsd:1;
	u8 etc:1;
	u8 tmc:2;
	u8 lbin:1;
	u8 lp:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 lp:1;
	u8 lbin:1;
	u8 tmc:2;
	u8 etc:1;
	u8 tsd:1;
	u8 ds:1;
	u8 du:1;
#endif
	u8 length;
	u8 proto_id;
#define IPR_SAS_PROTOCOL_ID	0x06
	u8 reserved[2];
	u8 num_phys;
#define IPR_IOA_MAX_PHYS	4
	struct ipr_sas_phy_log_desc phy[IPR_IOA_MAX_PHYS];
};

#define IPR_SAS_LINK_RATE_NOT_PROGRAMMABLE	0
#define IPR_SAS_LINK_RATE_1_5_Gbps			8
#define IPR_SAS_LINK_RATE_3_0_Gbps			9

#define IPR_SAS_ROUTING_DIRECT		0
#define IPR_SAS_ROUTING_SUB_OR_DIRECT	1
#define IPR_SAS_ROUTING_TABLE_OR_DIRECT	2

struct ipr_sas_phy_info {
	u32 len;
	u8 bus;
	u8 cascade;
	u8 phy_num;
	u8 reserved;
	struct ipr_sas_phy_log_desc phy_log;
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 prog_min_link_rate:4;
	u8 hw_min_link_rate:4;

	u8 prog_max_link_rate:4;
	u8 hw_max_link_rate:4;

	u8 virtual_phy:1;
	u8 reserved2:7;

	u8 reserved3:4;
	u8 routing_attr:4;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 hw_min_link_rate:4;
	u8 prog_min_link_rate:4;

	u8 hw_max_link_rate:4;
	u8 prog_max_link_rate:4;

	u8 reserved2:7;
	u8 virtual_phy:1;

	u8 routing_attr:4;
	u8 reserved3:4;
#endif
};

struct ipr_sas_expander_info {
	u32 len;
	u8 bus;
	u8 cascade;
	u8 reserved;
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 reserved2:7;
	u8 sas_1_1_fmt:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 sas_1_1_fmt:1;
	u8 reserved2:7;
#endif
	struct ipr_std_inq_vpids vpids;
	u8 prod_rev_level[4];
	u8 component_vendor_id[8];
	u8 component_id[2];
	u8 component_rev_id;
	u8 reserved3;
	u8 vendor_spec[8];
	u8 enclosure_logical_id[8];
	u8 reserved4[3];
	u8 num_phys;
	struct ipr_sas_phy_info phy[1];
};

struct ipr_query_sas_expander_info {
	u32 len;
	u8 reserved[3];
	u8 num_expanders;
	struct ipr_sas_expander_info exp[1];
};

struct ipr_global_cache_params_term {
#define IPR_CACHE_PARAM_TERM_ID	0x10
	u8 term_id;
	u8 len;
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 enable_caching_dual_ioa_failure:1;
	u8 disable_caching_requested:1;
	u8 reserved1:2;
	u8 vset_write_cache_enabled:1;
	u8 reserved2:3;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 reserved2:3;
	u8 vset_write_cache_enabled:1;
	u8 reserved1:2;
	u8 disable_caching_requested:1;
	u8 enable_caching_dual_ioa_failure:1;
#endif
	u8 reserved3;
};

struct log_parameter_hdr {
	u8 parameter_code[2];

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 du:1;
	u8 reserved1:1;
	u8 tsd:1;
	u8 etc:1;
	u8 tmc:2;
	u8 lbin:1;
	u8 lp:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 lp:1;
	u8 lbin:1;
	u8 tmc:2;
	u8 etc:1;
	u8 tsd:1;
	u8 reserved1:1;
	u8 du:1;
#endif
	u8 length;
};

/* Log Parameter: Log Page = 0x34. Attribute template */
struct ipr_sas_log_smart_attr {
	struct log_parameter_hdr hdr;
	u8 norm_threshold_val;
	int8_t norm_worst_val;
	u8 raw_data[10];
};

/* Log Parameter: Log Page = 0x2F. Attribute Code = 0x0 */
struct ipr_sas_log_inf_except_attr {
	struct log_parameter_hdr param_hdr;
	u8 inf_except_add_sense_code;
	u8 inf_except_add_sense_code_qual;
	u8 last_temp_read;
};

struct ipr_sas_log_write_err_cnt_attr {
	struct log_parameter_hdr param_hdr;
	u8 counter;
};

struct ipr_sas_log_page {
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 reserved1:2;
	u8 page_code:6;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 page_code:6;
	u8 reserved1:2;
#endif
	u8 reserved2;
	u8 page_length[2];

#define IPR_SAS_LOG_MAX_ENTRIES		256
	u8 raw_data[IPR_SAS_LOG_MAX_ENTRIES];
};

struct ipr_query_ioa_caching_info {
	u16 len;
	u8 reserved[2];
#define IPR_MAX_TERMS           512
	struct ipr_global_cache_params_term term[IPR_MAX_TERMS];
};

#define for_each_cache_term(term, info) \
        for (term = (info)->term; \
             ((unsigned long)term) < ((unsigned long)((unsigned long)(info) + ntohs((info)->len)) + 2) && \
             ((unsigned long)term + term->len + 2 - (unsigned long)(info)->term) < sizeof((info)->term); \
             term = (struct ipr_global_cache_params_term *)(((unsigned long)term) + term->len + 2))

struct ipr_log_page18 {
	u8 page_code;
	u8 reserved;
	u16 length;
#define IPR_IOA_MAX_PORTS	32
	struct ipr_sas_port_log_desc port[IPR_IOA_MAX_PORTS];
};

#define for_each_port(port, log) \
        for (port = (log)->port; \
             (unsigned long)port < ((unsigned long)((log)->port) + ntohs(port->length)); \
             port = (struct ipr_sas_port_log_desc *)((unsigned long)(&port->length) + port->length + 1))

#define for_each_phy(phy, port) \
        for (phy = (port)->phy; \
             (phy < ((port)->phy + (port)->num_phys)) && \
             (phy < ((port)->phy + IPR_IOA_MAX_PORTS)); phy++)

struct ipr_dasd_perf_counters_log_page30 {
	u8 page_code;
	u8 reserved1;
	u16 length;
	u8 reserved2[3];
	u8 param_length;
	u16 dev_no_seeks;
	u16 dev_seeks_2_3;
	u16 dev_seeks_1_3;
	u16 dev_seeks_1_6;
	u16 dev_seeks_1_12;
	u16 dev_seeks_0;
	u32 reserved3;
	u16 dev_read_buf_overruns;
	u16 dev_write_buf_underruns;
	u32 dev_cache_read_hits;
	u32 dev_cache_partial_read_hits;
	u32 dev_cache_write_hits;
	u32 dev_cache_fast_write_hits;
	u32 reserved4;
	u32 reserved5;
	u32 ioa_dev_read_ops;
	u32 ioa_dev_write_ops;
	u32 ioa_cache_read_hits;
	u32 ioa_cache_partial_read_hits;
	u32 ioa_cache_write_hits;
	u32 ioa_cache_fast_write_hits;
	u32 ioa_cache_emu_read_hits;
	u32 ioa_idle_loop_count[2];
	u32 ioa_idle_count_value;
	u8 ioa_idle_units;
	u8 reserved6[3];
};

struct ipr_fabric_config_element {
	u8 type_status;
#define IPR_PATH_CFG_TYPE_MASK	0xF0
#define IPR_PATH_CFG_NOT_EXIST	0x00
#define IPR_PATH_CFG_IOA_PORT		0x10
#define IPR_PATH_CFG_EXP_PORT		0x20
#define IPR_PATH_CFG_DEVICE_PORT	0x30
#define IPR_PATH_CFG_DEVICE_LUN	0x40

#define IPR_PATH_CFG_STATUS_MASK	0x0F
#define IPR_PATH_CFG_NO_PROB		0x00
#define IPR_PATH_CFG_DEGRADED		0x01
#define IPR_PATH_CFG_FAILED		0x02
#define IPR_PATH_CFG_SUSPECT		0x03
#define IPR_PATH_NOT_DETECTED		0x04
#define IPR_PATH_INCORRECT_CONN	0x05

	u8 cascaded_expander;
	u8 phy;
	u8 link_rate;
#define IPR_PHY_LINK_RATE_MASK	0x0F

	u32 wwid[2];
};

struct ipr_fabric_descriptor {
	u16 length;
	u8 ioa_port;
	u8 cascaded_expander;
	u8 phy;
	u8 path_state;
#define IPR_PATH_ACTIVE_MASK		0xC0
#define IPR_PATH_NO_INFO		0x00
#define IPR_PATH_ACTIVE			0x40
#define IPR_PATH_NOT_ACTIVE		0x80

#define IPR_PATH_STATE_MASK		0x0F
#define IPR_PATH_STATE_NO_INFO		0x00
#define IPR_PATH_HEALTHY		0x01
#define IPR_PATH_DEGRADED		0x02
#define IPR_PATH_FAILED			0x03

	u16 num_entries;
	struct ipr_fabric_config_element elem[1];
};

struct ipr_fabric_config_element64 {
	u16 length;
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 cfg_elem_id:2;
	u8 reserved1:6;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 reserved1:6;
	u8 cfg_elem_id:2;
#endif
	u8 reserved2;
	u8 type_status;
	u8 reserved3[2];
	u8 link_rate;
	u8 res_path[8];
	u32 wwid[4];
};

struct ipr_fabric_descriptor64 {
	u16 length;
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 fabric_id:2;
	u8 reserved1:6;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 reserved1:6;
	u8 fabric_id:2;
#endif
	u8 reserved2[2];
	u8 path_state;
	u8 reserved3;
	u8 res_path[8];
	u32 reserved4;
	u8 reserved5[2];
	u16 num_entries;
	struct ipr_fabric_config_element64 elem[1];
};

#define for_each_fabric_cfg(fabric, cfg) \
		for (cfg = (fabric)->elem; \
			cfg < ((fabric)->elem + ntohs((fabric)->num_entries)); \
			cfg++)

#define for_each_fabric_desc(fabric, info) \
	for (fabric = (info)->u.fabric; \
	     ((unsigned long)fabric) < (((unsigned long)(info)) + ntohl((info)->len) + sizeof((info)->len)); \
	     fabric = (struct ipr_fabric_descriptor *)(((unsigned long)fabric) + ntohs(fabric->length)))

#define for_each_fabric_desc64(fabric, info) \
	for (fabric = (info)->u.fabric64; \
	     ((unsigned long)fabric) < (((unsigned long)(info)) + ntohl((info)->len) + sizeof((info)->len)); \
	     fabric = (struct ipr_fabric_descriptor64 *)(((unsigned long)fabric) + ntohs(fabric->length)))

struct ipr_res_redundancy_info {
	u32 len;
	u8 possibly_redundant_paths;
	u8 healthy_paths;
	u8 degraded_paths;
	u8 failed_paths;
	u8 active_paths;
	u8 reserved[2];
	u8 fabric_descriptors;
	union {
		struct ipr_fabric_descriptor fabric[1];
		struct ipr_fabric_descriptor64 fabric64[1];
	} u;
	u8 data[16384];
};

struct ses_inquiry_page0 {
	u8 peri_qual_dev_type;
	u8 page_code;
	u8 reserved1;
	u8 page_length;
	u8 supported_vpd_page[SES_MAX_NUM_SUPP_INQ_PAGES];
};

struct ses_serial_num_vpd {
	u8 peri_dev_type;
	u8 page_code;
	u8 reserved1;
	u8 add_page_len;
	u8 ascii_len;
	u8 serial_num_head[3];
	u8 feature_code[4];
	u8 ascii_dash;
	u8 count[3];
	u8 ascii_space;
	u8 ses_serial_num[7];
};


struct esm_serial_num_vpd {
	u8 peri_dev_type;
	u8 page_code;
	u8 reserved1;
	u8 page_len;
	u8 ascii_len;
	u8 fru_part_number[3];
	u8 frb_number[8];
	u8 reserved2;
	u8 serial_num_head[3];
	u8 esm_serial_num[12];
	u8 reserved3;
	u8 model_num_head[3];
	u8 model_num[4];
	u8 reserved4;
	u8 frb_label_head[3];
	u8 frb_label[5];
	u8 reserved5;
};

struct drive_elem_desc {
	u8 reserved1[2];
	u8 desc_length[2];
	u8 reserved2;
#define DISK_PHY_LOC_LENGTH	9
	u8 disk_physical_loc[DISK_PHY_LOC_LENGTH];
	u8 reserved3[2];
	u8 form_facter;
	u8 slot_height;
	u8 slot_bus;
	u8 phy_speed;
};

struct drive_elem_desc_pg {
	u8 page_code;
	u8 reserved1;
	u16 page_length;
	u8 reserved2[6];
	u16 desc_length;
#define DEVICE_ELEM_NUMBER	50
	struct drive_elem_desc dev_elem[DEVICE_ELEM_NUMBER];
};
struct ipr_ses_type_desc {
	u8 elem_type;
#define IPR_SES_DEVICE_ELEM	0x01
#define IPR_SES_ESM_ELEM	0x07
#define IPR_SES_ENCLOSURE_ELEM	0x0E
	u8 num_elems;
	u8 subenclosure_id;
	u8 text_len;
};

struct ipr_drive_elem_status {
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 select:1;
	u8 predictive_fault:1;
	u8 reserved:1;
	u8 swap:1;
	u8 status:4;

	u8 reserved2:1;
	u8 device_environment:2;
	u8 slot_id:5;

	u8 reserved3:4;
	u8 insert:1;
	u8 remove:1;
	u8 identify:1;
	u8 reserved4:1;

	u8 reserved5:1;
	u8 fault_requested:1;
	u8 fault_sensed:1;
	u8 reserved6:1;
	u8 enable_byp:2;
	u8 reserved7:1;
	u8 disable_resets:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 status:4;
	u8 swap:1;
	u8 reserved:1;
	u8 predictive_fault:1;
	u8 select:1;

	u8 slot_id:5;
	u8 device_environment:2;
	u8 reserved2:1;

	u8 reserved4:1;
	u8 identify:1;
	u8 remove:1;
	u8 insert:1;
	u8 reserved3:4;

	u8 disable_resets:1;
	u8 reserved7:1;
	u8 enable_byp:2;
	u8 reserved6:1;
	u8 fault_sensed:1;
	u8 fault_requested:1;
	u8 reserved5:1;
#endif
};

struct ipr_encl_status_ctl_pg {
	u8 page_code;
	u8 health_status;
	u16 byte_count;
	u8 reserved1[4];
	struct ipr_drive_elem_status elem_status[IPR_NUM_DRIVE_ELEM_STATUS_ENTRIES];
};

struct ipr_ses_config_pg {
	u8 buf[2048];
};

struct ipr_ses_inquiry_pageC3 {
	u8 perif_type;
	u8 page_code;
	u8 reserved1;
	u8 page_length;
	u8 ascii_length;
	u8 mode_keyword[3];
	u8 mode[4];
	u8 reserved2;
};

/* Page 12h - Get Time */
struct ipr_ses_diag_page12 {
	u8 page_code;
	u8 reserved1;
	u8 page_length[2];
	u8 timestamp_origin;
	u8 reserved2;
	u8 timestamp[6];
};

/* Page 13h - Set Time */
struct ipr_ses_diag_ctrl_page13 {
	u8 page_code;
	u8 reserved1;
	u8 page_length[2];
	u8 timestamp[6];
	u8 reserved[2];
};

struct ipr_sas_inquiry_pageC4 {
	u8 peri_qual_dev_type;
	u8 page_code;
	u8 reserved1;
	u8 page_length;
#define IPR_SAS_ENDURANCE_HIGH_HDD 0x20
#define IPR_SAS_ENDURANCE_LOW_EZ   0x31
#define IPR_SAS_ENDURANCE_LOW3	   0x32
#define IPR_SAS_ENDURANCE_LOW1	   0x33
	u8 endurance;
	u8 reserved[6];
	u8 revision;
	u8 serial_num_11s[8];
	u8 serial_num_supplier[8];
	u8 master_drive_part[12];
};

struct ipr_sas_inquiry_pageC7 {
	u8 peri_qual_dev_type;
	u8 page_code;
	u8 reserved1;
	u8 page_length;
	u8 ascii_len;
	u8 reserved2[38];
	u8 format_timeout_hi; /* in minutes */
	u8 format_timeout_lo;
	u8 reserved3[63];
	u8 support_4k_modes;
#define ONLY_5XX_SUPPORTED		0
#define BOTH_5XXe_OR_4K_SUPPORTED	1
#define ONLY_4K_SUPPORTED		2
	u8 reserved4[5];
	u8 total_bytes_warranty[IPR_SAS_INQ_BYTES_WARRANTY_LEN];
	u8 reserved5[43];
};

static inline int ipr_elem_offset(struct ipr_ses_config_pg *ses_cfg, u8 type)
{
	int i, off;
	struct ipr_ses_type_desc *desc;
	u8 *diag = ses_cfg->buf;

	desc = (struct ipr_ses_type_desc *)&diag[12+diag[11]];

	for (i = 0, off = 1; i < diag[10]; off += (desc->num_elems + 1), i++, desc++)
		if (desc->elem_type == type)
			return off;

	return 1;
}

static inline int ipr_dev_elem_offset(struct ipr_ses_config_pg *ses_cfg)
{
	return ipr_elem_offset(ses_cfg, IPR_SES_DEVICE_ELEM);
}

static inline int ipr_ses_elem_offset(struct ipr_ses_config_pg *ses_cfg)
{
	return ipr_elem_offset(ses_cfg, IPR_SES_ENCLOSURE_ELEM);
}

static inline int ipr_esm_elem_offset(struct ipr_ses_config_pg *ses_cfg)
{
	return ipr_elem_offset(ses_cfg, IPR_SES_ESM_ELEM);
}

static inline struct ipr_drive_elem_status *
ipr_get_overall_elem(struct ipr_encl_status_ctl_pg *ses_data,
		     struct ipr_ses_config_pg *ses_cfg)
{
	return &(ses_data->elem_status[ipr_dev_elem_offset(ses_cfg) - 1]);
}

static inline struct ipr_ses_type_desc *ipr_get_elem(struct ipr_ses_config_pg *ses_cfg, u8 type)
{
	int i;
	struct ipr_ses_type_desc *desc;
	u8 *diag = ses_cfg->buf;

	desc = (struct ipr_ses_type_desc *)&diag[12+diag[11]];

	for (i = 0; i < diag[10]; i++, desc++)
		if (desc->elem_type == type)
			return desc;

	return NULL;
}

static inline struct ipr_ses_type_desc *ipr_get_dev_elem(struct ipr_ses_config_pg *ses_cfg)
{
	return ipr_get_elem(ses_cfg, IPR_SES_DEVICE_ELEM);
}

static inline struct ipr_drive_elem_status *
ipr_get_enclosure_elem(struct ipr_encl_status_ctl_pg *ses_data,
		       struct ipr_ses_config_pg *ses_cfg)
{
	return &(ses_data->elem_status[ipr_ses_elem_offset(ses_cfg) - 1]);
}

static inline struct ipr_drive_elem_status *
ipr_get_esm_elem(struct ipr_encl_status_ctl_pg *ses_data,
		 struct ipr_ses_config_pg *ses_cfg)
{
	return &(ses_data->elem_status[ipr_esm_elem_offset(ses_cfg) - 1]);
}

static inline int ipr_max_dev_elems(struct ipr_ses_config_pg *ses_cfg)
{
	struct ipr_ses_type_desc *desc = ipr_get_dev_elem(ses_cfg);

	if (desc && desc->num_elems < IPR_NUM_DRIVE_ELEM_STATUS_ENTRIES)
		return desc->num_elems;

	return IPR_NUM_DRIVE_ELEM_STATUS_ENTRIES;
}

#define for_each_elem_status(elem, sd, sc) \
        for (elem = &((sd)->elem_status[ipr_dev_elem_offset(sc)]); \
             elem < &((sd)->elem_status[(ntohs((sd)->byte_count)-4)/sizeof((sd)->elem_status[0])]) && \
             elem < &((sd)->elem_status[ipr_dev_elem_offset(sc) + ipr_max_dev_elems(sc)]) && \
             elem < &((sd)->elem_status[IPR_NUM_DRIVE_ELEM_STATUS_ENTRIES]); elem++)

int sg_ioctl(int, u8 *, void *, u32, u32, struct sense_data_t *, u32);
int sg_ioctl_noretry(int, u8 *, void *, u32, u32, struct sense_data_t *, u32);
int ipr_change_multi_adapter_assignment(struct ipr_ioa *, int, int);
int set_ha_mode(struct ipr_ioa *, int);
int set_preferred_primary(struct ipr_ioa *, int);
void check_current_config(bool);
int num_device_opens(int, int, int, int);
int open_and_lock(char *);
int tool_init(int);
void exit_on_error(char *, ...);
bool is_af_blocked(struct ipr_dev *, int);
int ipr_query_array_config(struct ipr_ioa *, bool, bool, bool, int, struct ipr_array_query_data *);
int __ipr_query_array_config(struct ipr_ioa *, int, bool, bool, bool, int, struct ipr_array_query_data *);
int ipr_query_command_status(struct ipr_dev *, void *);
int ipr_query_io_dev_port(struct ipr_dev *, struct ipr_query_io_port *);
int ipr_mode_sense(struct ipr_dev *, u8, void *);
int ipr_mode_select(struct ipr_dev *, void *, int);
int ipr_log_sense(struct ipr_dev *, u8, void *, u16);
int ipr_is_log_page_supported(struct ipr_dev *, u8);
void *ipr_sas_log_get_param(const struct ipr_sas_log_page *page, u32 param,
			    int *entries_cnt);
int ipr_reset_device(struct ipr_dev *);
int ipr_re_read_partition(struct ipr_dev *);
int ipr_read_capacity(struct ipr_dev *, void *);
int ipr_read_capacity_16(struct ipr_dev *, void *);
int ipr_query_resource_state(struct ipr_dev *, void *);
void ipr_allow_restart(struct ipr_dev *, int);
int ipr_get_logical_block_size(struct ipr_dev *);
bool ipr_is_af_blk_size(struct ipr_ioa *, int);
void ipr_set_manage_start_stop(struct ipr_dev *);
int ipr_start_stop_start(struct ipr_dev *);
int ipr_start_stop_stop(struct ipr_dev *);
int ipr_stop_array_protection(struct ipr_ioa *);
int ipr_remove_hot_spare(struct ipr_ioa *);
int ipr_start_array_protection(struct ipr_ioa *, int, int);
int ipr_migrate_array_protection(struct ipr_ioa *,
	       			 struct ipr_array_query_data *, int, int, int);
int ipr_set_array_asym_access(struct ipr_ioa *);
int ipr_add_hot_spare(struct ipr_ioa *);
int ipr_rebuild_device_data(struct ipr_ioa *);
int ipr_resync_array(struct ipr_ioa *);
int ipr_test_unit_ready(struct ipr_dev *, struct sense_data_t *);
int ipr_format_unit(struct ipr_dev *);
void ipr_format_res_path(u8 *, char *, int);
void ipr_format_res_path_wo_hyphen(u8 *, char *, int);
int ipr_add_array_device(struct ipr_ioa *, int, struct ipr_array_query_data *);
int ipr_reclaim_cache_store(struct ipr_ioa *, int, void *);
int ipr_evaluate_device(struct ipr_dev *, u32);
int ipr_query_res_redundancy_info(struct ipr_dev *,
				  struct ipr_res_redundancy_info *);
int ipr_query_sas_expander_info(struct ipr_ioa *,
				struct ipr_query_sas_expander_info *);
int ipr_query_cache_parameters(struct ipr_ioa *, void *, int);
int ipr_disrupt_device(struct ipr_dev *);
int ipr_inquiry(struct ipr_dev *, u8, void *, u8);
int ipr_query_res_addr_aliases(struct ipr_ioa *, struct ipr_res_addr *,
			       struct ipr_res_addr_aliases *);
int ipr_query_res_path_aliases(struct ipr_ioa *, struct ipr_res_path *,
			       struct ipr_res_path_aliases *);
void ipr_convert_res_path_to_bytes(struct ipr_dev *);
void ipr_format_res_path(u8 *, char *, int);
void ipr_reset_adapter(struct ipr_ioa *);
void ipr_scan(struct ipr_ioa *, int, int, int);
int ipr_read_host_attr(struct ipr_ioa *, char *, void *, size_t);
int ipr_write_host_attr(struct ipr_ioa *, char *, void *, size_t);
int ipr_read_dev_attr(struct ipr_dev *, char *, char *, size_t);
int ipr_write_dev_attr(struct ipr_dev *, char *, char *);
int ipr_suspend_device_bus(struct ipr_dev *, struct ipr_res_addr *, u8);
int ipr_resume_device_bus(struct ipr_dev *, struct ipr_res_addr *);
int ipr_can_remove_device(struct ipr_dev *);
int ipr_receive_diagnostics(struct ipr_dev *, u8, void *, int);
int ipr_send_diagnostics(struct ipr_dev *, void *, int);
int ipr_get_bus_attr(struct ipr_ioa *, struct ipr_scsi_buses *);
void ipr_modify_bus_attr(struct ipr_ioa *, struct ipr_scsi_buses *);
int ipr_set_bus_attr(struct ipr_ioa *, struct ipr_scsi_buses *, int);
int ipr_write_buffer(struct ipr_dev *, void *, int);
int enable_af(struct ipr_dev *);
int get_max_bus_speed(struct ipr_ioa *, int);
int ipr_get_ioa_attr(struct ipr_ioa *, struct ipr_ioa_attr *);
int ipr_get_dev_attr(struct ipr_dev *, struct ipr_disk_attr *);
int ipr_modify_dev_attr(struct ipr_dev *, struct ipr_disk_attr *);
int ipr_set_ioa_attr(struct ipr_ioa *, struct ipr_ioa_attr *, int);
int ipr_modify_ioa_attr(struct ipr_ioa *, struct ipr_ioa_attr *);
int ipr_set_dev_attr(struct ipr_dev *, struct ipr_disk_attr *, int);
int set_active_active_mode(struct ipr_ioa *, int);
int ipr_set_format_completed_bit(struct ipr_dev *);
int ipr_set_ses_mode(struct ipr_dev *, int);
int get_ioa_caching(struct ipr_ioa *);
int ipr_change_cache_parameters(struct ipr_ioa *, int);
int ipr_query_dasd_timeouts(struct ipr_dev *, struct ipr_query_dasd_timeouts *);
int get_ioa_firmware_image_list(struct ipr_ioa *, struct ipr_fw_images **);
int get_dasd_firmware_image_list(struct ipr_dev *, struct ipr_fw_images **);
int get_ses_firmware_image_list(struct ipr_dev *, struct ipr_fw_images **);
int get_latest_fw_image_version(struct ipr_dev *);
struct ipr_fw_images *get_latest_fw_image(struct ipr_dev *);
int ipr_update_ioa_fw(struct ipr_ioa *, struct ipr_fw_images *, int);
int ipr_update_disk_fw(struct ipr_dev *, struct ipr_fw_images *, int);
int ipr_init_dev(struct ipr_dev *);
int ipr_init_new_dev(struct ipr_dev *);
int ipr_init_ioa(struct ipr_ioa *);
int device_supported(struct ipr_dev *);
struct ipr_dev *get_dev_from_addr(struct ipr_res_addr *res_addr);
struct ipr_dev *get_dev_from_handle(struct ipr_ioa *ioa, u32 res_handle);
void ipr_daemonize();
struct unsupported_af_dasd *get_unsupp_af(struct ipr_std_inq_data *,
					  struct ipr_dasd_inquiry_page3 *);
bool disk_needs_msl(struct unsupported_af_dasd *,
		    struct ipr_std_inq_data *);
void ipr_add_zeroed_dev(struct ipr_dev *);
void ipr_update_qac_with_zeroed_devs(struct ipr_ioa *);
void ipr_cleanup_zeroed_devs();
void ipr_del_zeroed_dev(struct ipr_dev *);
int ipr_device_is_zeroed(struct ipr_dev *);
struct ipr_array_cap_entry *get_raid_cap_entry(struct ipr_supported_arrays *, u8 );
char *get_prot_level_str(struct ipr_supported_arrays *, int);
u32 get_fw_version(struct ipr_dev *);
int ipr_disable_qerr(struct ipr_dev *);
void ipr_log_ucode_error(struct ipr_ioa *);
u32 get_dasd_ucode_version(char *, int);
u32 get_ses_ucode_version(char *ucode_file);
const char *get_bus_desc(struct ipr_ioa *);
const char *get_ioa_desc(struct ipr_ioa *);
int ioa_is_spi(struct ipr_ioa *);
int __ioa_is_spi(struct ipr_ioa *);
int page0x0a_setup(struct ipr_dev *);
int handle_events(void (*) (void), int, void (*) (char *));
struct ipr_ioa *find_ioa(int);
int parse_option(char *);
struct ipr_dev *find_blk_dev(char *);
struct ipr_dev *find_gen_dev(char *);
struct ipr_dev *find_dev(char *);
int ipr_cmds_per_lun(struct ipr_ioa *);
void scsi_host_kevent(char *, int (*)(struct ipr_ioa *));
void scsi_dev_kevent(char *, struct ipr_dev *(*)(char *), int (*)(struct ipr_dev *));
int format_req(struct ipr_dev *);
struct ipr_dev *get_vset_from_array(struct ipr_ioa *, struct ipr_dev *);
struct ipr_dev *get_array_from_vset(struct ipr_ioa *, struct ipr_dev *);
struct sysfs_dev * ipr_find_sysfs_dev(struct ipr_dev *, struct sysfs_dev *);
void ipr_add_sysfs_dev(struct ipr_dev *, struct sysfs_dev **, struct sysfs_dev **);
void ipr_del_sysfs_dev(struct ipr_dev *, struct sysfs_dev **, struct sysfs_dev **);
struct ipr_dev *ipr_sysfs_dev_to_dev(struct sysfs_dev *);
struct ipr_array_cap_entry *get_cap_entry(struct ipr_supported_arrays *, char *);
int ipr_get_blk_size(struct ipr_dev *);
u32 get_ioa_ucode_version(char *);
int ipr_improper_device_type(struct ipr_dev *);
int ipr_get_fw_version(struct ipr_dev *, u8 release_level[4]);
int ipr_get_live_dump(struct ipr_ioa *);
int ipr_jbod_sysfs_bind(struct ipr_dev *, u8);
int ipr_max_queue_depth(struct ipr_ioa *ioa, int num_devs, int num_ssd_devs);
void ipr_count_devices_in_vset(struct ipr_dev *, int *num_devs, int *ssd_num_devs);
int ipr_known_zeroed_is_saved(struct ipr_dev *);
int get_sg_name(struct scsi_dev_data *);
int ipr_sg_inquiry(struct scsi_dev_data *, u8, void *, u8);

static inline u32 ipr_get_dev_res_handle(struct ipr_ioa *ioa, struct ipr_dev_record *dev_rcd)
{
	if (ioa->sis64)
		return dev_rcd->type3.resource_handle;
	else
		return dev_rcd->type2.resource_handle;
}

static inline u32 ipr_get_arr_res_handle(struct ipr_ioa *ioa, struct ipr_array_record *array_rcd)
{
	if (ioa->sis64)
		return array_rcd->type3.resource_handle;
	else
		return array_rcd->type2.resource_handle;
}

static inline int ipr_is_device_record(int record_id)
{
	if ((record_id == IPR_RECORD_ID_DEVICE_RECORD) ||
	    (record_id == IPR_RECORD_ID_DEVICE_RECORD_3))
		return 1;
	else
		return 0;
}

static inline int ipr_is_vset_record(int record_id)
{
	if ((record_id == IPR_RECORD_ID_ARRAY_RECORD) ||
	    (record_id == IPR_RECORD_ID_VSET_RECORD_3))
		return 1;
	else
		return 0;
}

static inline int ipr_is_array_record(int record_id)
{
	if ((record_id == IPR_RECORD_ID_ARRAY_RECORD) ||
	    (record_id == IPR_RECORD_ID_ARRAY_RECORD_3))
		return 1;
	else
		return 0;
}

static inline int ipr_is_af_dasd_device(struct ipr_dev *device)
{
	if ((device->qac_entry != NULL) &&
	    (ipr_is_device_record(device->qac_entry->record_id)))
		return 1;
	else
		return 0;
}

static inline int ipr_is_remote_af_dasd_device(struct ipr_dev *device)
{
	if ((device->qac_entry != NULL) &&
	    (ipr_is_device_record(device->qac_entry->record_id) &&
		device->dev_rcd->current_asym_access_state == IPR_ACTIVE_NON_OPTIMIZED))
		return 1;
	else
		return 0;
}

static inline int ipr_is_volume_set(struct ipr_dev *device)
{
	if ((device->qac_entry != NULL) &&
	    (ipr_is_vset_record(device->qac_entry->record_id)))
		return 1;
	else
		return 0;
}

static inline int ipr_is_array(struct ipr_dev *device)
{
	if ((device->qac_entry != NULL) &&
	    (ipr_is_array_record(device->qac_entry->record_id)))
		return 1;
	else
		return 0;
}

static inline int ipr_is_hidden(struct ipr_dev *device)
{
	if (ipr_is_af_dasd_device(device) ||
	    ((device->scsi_dev_data) &&
	     (device->scsi_dev_data->type == 3)))  //FIXME SES type.
			return 1;
	else
		return 0;}


static inline int ipr_is_hot_spare(struct ipr_dev *device)
{
	struct ipr_dev_record *dev_record =
		(struct ipr_dev_record *)device->qac_entry;

	if ((dev_record != NULL) &&
	    (ipr_is_device_record(dev_record->common.record_id)) &&
	    (dev_record->is_hot_spare))
		return 1;
	else
		return 0;
}

static inline int ipr_is_array_member(struct ipr_dev *device)
{
	struct ipr_dev_record *dev_record =
		(struct ipr_dev_record *)device->qac_entry;

	if ((dev_record != NULL) &&
	    (dev_record->array_member))
		return 1;
	else
		return 0;
}

static inline int ipr_is_af(struct ipr_dev *device)
{
	if (device->qac_entry != NULL)
		return 1;
	else
		return 0;
}

static inline int ipr_is_gscsi(struct ipr_dev *dev)
{
	if (!dev->qac_entry &&
	    dev->scsi_dev_data &&
	    dev->scsi_dev_data->type == TYPE_DISK)
		return 1;
	else
		return 0;
}

static inline int ipr_is_ses(struct ipr_dev *dev)
{
	if (dev->scsi_dev_data &&
	    dev->scsi_dev_data->type == TYPE_ENCLOSURE)
		return 1;
	else
		return 0;
}

static inline void ipr_strncpy_0(char *dest, char *source, int length)
{
	memcpy(dest, source, length);
	dest[length] = '\0';
}

static inline void ipr_strncpy_0n(char *dest, char *source, int length)
{
	char *ch;
	memcpy(dest, source, length);
	dest[length] = '\0';
	ch = strchr(dest, '\n');
	if (ch)
		*ch = '\0';
}

#define dprintf(...) \
do { \
if (ipr_debug) \
fprintf(stderr, __VA_ARGS__);\
} while(0)

#define ENTER dprintf("Entering %s\n", __FUNCTION__);
#define LEAVE dprintf("Leaving %s\n", __FUNCTION__);

#define syslog_dbg(...) \
do { \
if (ipr_debug) \
syslog(LOG_ERR, __VA_ARGS__);\
} while(0)

#define ra_dbg(ra, fmt, ...) \
      syslog_dbg("%d:%d:%d:%d: " fmt, (ra)->host, (ra)->bus, \
             (ra)->target, (ra)->lun, ##__VA_ARGS__); \

#define scsi_log(level, dev, fmt, ...) \
do { \
if (dev->scsi_dev_data && !dev->ioa->ioa_dead) { \
      syslog(level, "%d:%d:%d:%d: " fmt, dev->ioa->host_num, \
             dev->scsi_dev_data->channel, dev->scsi_dev_data->id, \
             dev->scsi_dev_data->lun, ##__VA_ARGS__); \
} \
} while (0)

#define scsi_dbg(dev, fmt, ...) \
do { \
if ((dev)->scsi_dev_data) { \
      syslog_dbg("%d:%d:%d:%d: " fmt, (dev)->ioa->host_num, \
             (dev)->scsi_dev_data->channel, (dev)->scsi_dev_data->id, \
             (dev)->scsi_dev_data->lun, ##__VA_ARGS__); \
} \
} while (0)

#define scsi_info(dev, fmt, ...) \
      scsi_log(LOG_NOTICE, dev, fmt, ##__VA_ARGS__)

#define scsi_err(dev, fmt, ...) \
      scsi_log(LOG_ERR, dev, fmt, ##__VA_ARGS__)

#define scsi_warn(dev, fmt, ...) \
      scsi_log(LOG_WARNING, dev, fmt, ##__VA_ARGS__)

#define scsi_cmd_err(dev, sense, cmd, rc) \
do { \
      if ((((sense)->error_code & 0x7F) != 0x70) || \
          (((sense)->sense_key & 0x0F) != 0x05) || ipr_debug) { \
             scsi_err(dev, "%s failed. rc=%d, SK: %X ASC: %X ASCQ: %X\n",\
                      cmd, rc, (sense)->sense_key & 0x0f, (sense)->add_sense_code, \
                      (sense)->add_sense_code_qual); \
      } \
} while (0)

#define scsi_cmd_dbg(dev, sense, cmd, rc) \
do { \
      if (rc && ipr_debug) { \
             scsi_err(dev, "%s failed. rc=%d, SK: %X ASC: %X ASCQ: %X\n",\
                      cmd, rc, (sense)->sense_key & 0x0f, (sense)->add_sense_code, \
                      (sense)->add_sense_code_qual); \
      } \
} while (0)

#define ioa_log(level, ioa, fmt, ...) \
      syslog(level, "%s: " fmt, ioa->pci_address, ##__VA_ARGS__)

#define ioa_info(ioa, fmt, ...) \
      ioa_log(LOG_NOTICE, ioa, fmt, ##__VA_ARGS__)

#define ioa_err(ioa, fmt, ...) \
      ioa_log(LOG_ERR, ioa, fmt, ##__VA_ARGS__)

#define ioa_dbg(ioa, fmt, ...) \
      syslog_dbg("%s: " fmt, ioa->pci_address, ##__VA_ARGS__)

#define ioa_cmd_err(ioa, sense, cmd, rc) \
do { \
if (!ioa->ioa_dead) { \
      if ((((sense)->error_code & 0x7F) != 0x70) || \
          (((sense)->sense_key & 0x0F) != 0x05) || ipr_debug) { \
             ioa_err(ioa, "%s failed. rc=%d, SK: %X ASC: %X ASCQ: %X\n",\
                     cmd, rc, (sense)->sense_key & 0x0f, (sense)->add_sense_code, \
                     (sense)->add_sense_code_qual); \
      } \
} \
} while (0)

#endif /* iprlib_h */
