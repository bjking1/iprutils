#ifndef iprlib_h
#define iprlib_h
/**
 * IBM IPR adapter utility library
 *
 * (C) Copyright 2000, 2004
 * International Business Machines Corporation and others.
 * All Rights Reserved.
 *
 */

/*
 * $Header: /cvsroot/iprdd/iprutils/iprlib.h,v 1.26 2004/03/11 02:37:43 bjking1 Exp $
 */

#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mount.h>
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
#include <ctype.h>
#include <syslog.h>
#include <term.h>
#include <pci/pci.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <sysfs/libsysfs.h>
#include <linux/byteorder/swab.h>
#include <asm/byteorder.h>
#include <sys/mman.h>

#define IPR_DASD_UCODE_USRLIB 0
#define IPR_DASD_UCODE_ETC 1

#define IPR_DASD_UCODE_NOT_FOUND -1
#define IPR_DASD_UCODE_NO_HDR 1
#define IPR_DASD_UCODE_HDR 2

#define UCODE_BASE_DIR "/usr/lib/microcode"

#define IOCTL_BUFFER_SIZE                    512
#define IPR_MAX_NUM_BUSES                    4
#define IPR_VENDOR_ID_LEN                    8
#define IPR_PROD_ID_LEN                      16
#define IPR_SERIAL_NUM_LEN                   8
#define IPR_VPD_PLANT_CODE_LEN               4
#define IPR_VPD_CACHE_SIZE_LEN               3
#define IPR_VPD_DRAM_SIZE_LEN                3
#define IPR_VPD_PART_NUM_LEN                 12
#define IPR_CCB_CDB_LEN                      16
#define IPR_QAC_BUFFER_SIZE                  16000
#define IPR_INVALID_ARRAY_ID                 0xFF
#define IPR_IOA_RESOURCE_HANDLE              0xffffffff
#define IPR_RECLAIM_NUM_BLOCKS_MULTIPLIER    256
#define  IPR_SDB_CHECK_AND_QUIESCE           0x00
#define  IPR_SDB_CHECK_ONLY                  0x40
#define IPR_MAX_NUM_SUPP_INQ_PAGES           8
#define IPR_DUMP_TRACE_ENTRY_SIZE            8192
#define IPR_MODE_SENSE_LENGTH                255
#define IPR_DEFECT_LIST_HDR_LEN              4
#define IPR_FORMAT_DATA                      0x10
#define IPR_FORMAT_IMMED                     2
#define IPR_ARRAY_CMD_TIMEOUT                (2 * 60)     /* 2 minutes */
#define IPR_INTERNAL_DEV_TIMEOUT             (2 * 60)     /* 2 minutes */
#define IPR_FORMAT_UNIT_TIMEOUT              (3 * 60 * 60) /* 3 hours */
#define IPR_INTERNAL_TIMEOUT                 (30)         /* 30 seconds */
#define IPR_SUSPEND_DEV_BUS_TIMEOUT          (35)         /* 35 seconds */
#define IPR_EVALUATE_DEVICE_TIMEOUT          (2 * 60)     /* 2 minutes */
#define IPR_WRITE_BUFFER_TIMEOUT             (10 * 60)    /* 10 minutes */
#define SET_DASD_TIMEOUTS_TIMEOUT		   (2 * 60)
#define IPR_NUM_DRIVE_ELEM_STATUS_ENTRIES    16
#define IPR_TIMEOUT_MINUTE_RADIX		0x4000

#define IPR_IOA_DEBUG                        0xDDu
#define   IPR_IOA_DEBUG_READ_IOA_MEM         0x00u
#define   IPR_IOA_DEBUG_WRITE_IOA_MEM        0x01u
#define   IPR_IOA_DEBUG_READ_FLIT            0x03u

#define IPR_STD_INQ_Z0_TERM_LEN      8
#define IPR_STD_INQ_Z1_TERM_LEN      12
#define IPR_STD_INQ_Z2_TERM_LEN      4
#define IPR_STD_INQ_Z3_TERM_LEN      5
#define IPR_STD_INQ_Z4_TERM_LEN      4
#define IPR_STD_INQ_Z5_TERM_LEN      2
#define IPR_STD_INQ_Z6_TERM_LEN      10
#define IPR_STD_INQ_PART_NUM_LEN     12
#define IPR_STD_INQ_EC_LEVEL_LEN     10
#define IPR_STD_INQ_FRU_NUM_LEN      12
#define IPR_STD_INQ_AS400_SERV_LVL_OFF       107

#define  IPR_START_STOP_STOP                 0x00
#define  IPR_START_STOP_START                0x01
#define  IPR_READ_CAPACITY_16                0x10
#define IPR_SERVICE_ACTION_IN                0x9E
#define IPR_QUERY_RESOURCE_STATE             0xC2
#define IPR_QUERY_COMMAND_STATUS             0xCB
#define IPR_SUSPEND_DEV_BUS                  0xC8u
#define IPR_EVALUATE_DEVICE                  0xE4
#define SKIP_READ					0xE8
#define SKIP_WRITE				0xEA
#define QUERY_DASD_TIMEOUTS			0xEB
#define SET_DASD_TIMEOUTS			0xEC
#define IPR_QUERY_ARRAY_CONFIG            0xF0
#define IPR_START_ARRAY_PROTECTION           0xF1
#define IPR_STOP_ARRAY_PROTECTION            0xF2
#define IPR_RESYNC_ARRAY_PROTECTION          0xF3
#define IPR_ADD_ARRAY_DEVICE                 0xF4
#define IPR_REMOVE_ARRAY_DEVICE              0xF5
#define IPR_REBUILD_DEVICE_DATA              0xF6
#define IPR_RECLAIM_CACHE_STORE              0xF8
#define  IPR_RECLAIM_ACTION                  0x60
#define  IPR_RECLAIM_PERFORM                 0x00
#define  IPR_RECLAIM_EXTENDED_INFO           0x10
#define  IPR_RECLAIM_QUERY                   0x20
#define  IPR_RECLAIM_RESET                   0x40
#define  IPR_RECLAIM_FORCE_BATTERY_ERROR     0x60
#define  IPR_RECLAIM_UNKNOWN_PERM            0x80
#define SET_SUPPORTED_DEVICES			0xFB
#define IPR_STD_INQUIRY                      0xFF
#ifndef REPORT_LUNS
#define REPORT_LUNS				0xA0
#endif

#define IPR_TYPE_AF_DISK             0xC
#define IPR_TYPE_SES                 TYPE_ENCLOSURE /* xxx change to TYPE_ENCLOSURE */
#define IPR_TYPE_ADAPTER             0x1f
#define IPR_TYPE_EMPTY_SLOT          0xff

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

extern struct ipr_array_query_data *ipr_array_query_data;
extern u32 num_ioas;
extern struct ipr_ioa *ipr_ioa_head;
extern struct ipr_ioa *ipr_ioa_tail;
extern int runtime;

struct ipr_res_addr {
	u8 reserved;
	u8 bus;
	u8 target;
	u8 lun;
#define IPR_GET_PHYSICAL_LOCATOR(res_addr) \
	(((res_addr)->channel << 16) | ((res_addr)->id << 8) | (res_addr)->lun)
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
#endif
	u8 reserved5;
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

	u8 reserved4[2];

	u16 raw_power_on_time;
	u16 adjusted_power_on_time;
	u16 estimated_time_to_battery_warning;
	u16 estimated_time_to_battery_failure;

	u8 reserved5[240];
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
	};
};

struct ipr_array_cap_entry {
	u8   prot_level;
#define IPR_DEFAULT_RAID_LVL "5"
#if defined (__BIG_ENDIAN_BITFIELD)
	u8   include_allowed:1;
	u8   reserved:7;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8   reserved:7;
	u8   include_allowed:1;
#endif
	u16  reserved2;
	u8   reserved3;
	u8   max_num_array_devices;
	u8   min_num_array_devices;
	u8   min_mult_array_devices;
	u16  reserved4;
	u16  supported_stripe_sizes;
	u16  reserved5;
	u16  recommended_stripe_size;
	u8   prot_level_str[8];
};

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

	u8  reserved2;

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
	u8  reserved4:5;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8  reserved3:3;
	u8  no_config_entry:1;
	u8  high_avail:1;
	u8  non_func:1;
	u8  exposed:1;
	u8  established:1;

	u8  reserved4:5;
	u8  resync_cand:1;
	u8  stop_cand:1;
	u8  start_cand:1;
#endif
	u16 stripe_size;

	u8  raid_level;
	u8  array_id;
	u32 resource_handle;
	struct ipr_res_addr resource_addr;
	struct ipr_res_addr last_resource_addr;
	u8  vendor_id[IPR_VENDOR_ID_LEN];
	u8  product_id[IPR_PROD_ID_LEN];
	u8  serial_number[8];
	u32 reserved;
};

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
	u8  reserved1;

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
	u8  reserved3:6;
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

	u8  reserved3:6;
	u8  rmv_hot_spare_cand:1;
	u8  add_hot_spare_cand:1;
#endif

	u8  reserved4[2];
	u8  array_id;
	u32 resource_handle;
	struct ipr_res_addr resource_addr;
	struct ipr_res_addr last_resource_addr;

	u8 vendor_id[IPR_VENDOR_ID_LEN];
	u8 product_id[IPR_PROD_ID_LEN];
	u8 serial_num[IPR_SERIAL_NUM_LEN];
	u32 reserved5;
};

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
	u8 reserved2;
	u8 z4_term[IPR_STD_INQ_Z4_TERM_LEN];
	u8 z5_term[IPR_STD_INQ_Z5_TERM_LEN];
	u8 part_number[IPR_STD_INQ_PART_NUM_LEN];
	u8 ec_level[IPR_STD_INQ_EC_LEVEL_LEN];
	u8 fru_number[IPR_STD_INQ_FRU_NUM_LEN];
	u8 z6_term[IPR_STD_INQ_Z6_TERM_LEN];
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

#define IPR_CATEGORY_BUS "Bus"
#define IPR_QAS_CAPABILITY "QAS_CAPABILITY"
#define IPR_HOST_SCSI_ID "HOST_SCSI_ID"
#define IPR_BUS_WIDTH "BUS_WIDTH"
#define IPR_MAX_XFER_RATE_STR "MAX_BUS_SPEED"
#define IPR_MIN_TIME_DELAY "MIN_TIME_DELAY"

#define IPR_CATEGORY_DISK "Disk"
#define IPR_QUEUE_DEPTH "QUEUE_DEPTH"
#define IPR_TCQ_ENABLED "TCQ_ENABLED"
#define IPR_FORMAT_TIMEOUT "FORMAT_UNIT_TIMEOUT"

#define IPR_MAX_XFER_RATE 320

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
	int id;
	int lun;
	int type;
	int opens;
	int qdepth;
	int busy;
	int online;
	u32 handle;
	u8 vendor_id[IPR_VENDOR_ID_LEN + 1];
	u8 product_id[IPR_PROD_ID_LEN + 1];
	u8 sysfs_device_name[16];
	char dev_name[64];
	char gen_name[64];
};

struct ipr_dev {
	char dev_name[64];
	char gen_name[64];
	u8 prot_level_str[8];
	u32 is_reclaim_cand:1;
	u32 reserved:31;
	struct scsi_dev_data *scsi_dev_data;
	struct ipr_common_record *qac_entry;
	struct ipr_ioa *ioa;
};

#define IPR_MAX_IOA_DEVICES        (IPR_MAX_NUM_BUSES * 15 * 2 + 1)
struct ipr_ioa {
	struct ipr_dev ioa;
	u16 ccin;
	u32 host_addr;
	u32 num_raid_cmds;
	u32 msl;
	char driver_version[50];
	u16 num_devices;
	u8 rw_protected:1; /* FIXME */
	u8 ioa_dead:1;
	u8 nr_ioa_microcode:1;
	u8 scsi_id_changeable:1;
	int host_num;
	char pci_address[16];
	char host_name[16];
	struct ipr_dev dev[IPR_MAX_IOA_DEVICES];
	struct ipr_array_query_data *qac_data;
	struct ipr_array_record *start_array_qac_entry;
	struct ipr_supported_arrays *supported_arrays;
	struct ipr_reclaim_query_data *reclaim_data;
	struct ipr_ioa *next;
	struct ipr_ioa *cmd_next;
};

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

struct ipr_array_query_data {
	u16 resp_len;
	u8 reserved;
	u8 num_records;
	u8 data[IPR_QAC_BUFFER_SIZE];
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
};

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

struct ipr_disk_attr {
	int queue_depth;
	int tcq_enabled;
	int format_timeout;
};

struct ipr_vset_attr {
	int queue_depth;
};

struct ipr_dasd_timeout_record {
	u8 op_code;
	u8 reserved;
	u16 timeout;
};

struct ipr_supported_arrays {
	struct ipr_common_record common;
	u16 num_entries;
	u16 entry_length;
	u8 data[0];
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
	char vendor_id[IPR_VENDOR_ID_LEN + 1]; /* xxx char * instead? */
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
	char vendor_id[IPR_VENDOR_ID_LEN + 1]; /* xxx char * instead? */
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

	u8 percent_complete;
	struct ipr_res_addr failing_dev_res_addr;
	u32 failing_dev_res_handle;
	u32 failing_dev_ioasc;
	u32 ilid;
	u32 resource_handle;
};

struct ipr_cmd_status {
	u16 resp_len;
	u8  reserved;
	u8  num_records;
	struct ipr_cmd_status_record record[100];
};

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

struct ipr_dasd_ucode_header {
	u8 length[3];
	u8 load_id[4];
	u8 modification_level[4];
	u8 ptf_number[4];
	u8 patch_number[4];
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
	u8 model_num[3];
	u8 reserved2[9];
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

struct ipr_drive_elem_status
{
#if defined (__BIG_ENDIAN_BITFIELD)
	u8 select:1;
	u8 predictive_fault:1;
	u8 reserved:1;
	u8 swap:1;
	u8 status:4;

	u8 reserved2:4;
	u8 scsi_id:4;

	u8 reserved3:4;
	u8 insert:1;
	u8 remove:1;
	u8 reserved4:1;
	u8 identify:1;

	u8 reserved5:1;
	u8 fault_requested:1;
	u8 fault_sensed:1;
	u8 reserved6:5;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 status:4;
	u8 swap:1;
	u8 reserved:1;
	u8 predictive_fault:1;
	u8 select:1;

	u8 scsi_id:4;
	u8 reserved2:4;

	u8 identify:1;
	u8 reserved4:1;
	u8 remove:1;
	u8 insert:1;
	u8 reserved3:4;

	u8 reserved6:5;
	u8 fault_sensed:1;
	u8 fault_requested:1;
	u8 reserved5:1;
#endif
};

struct ipr_encl_status_ctl_pg
{
	u8 page_code;
	u8 health_status;
	u16 byte_count;
	u8 reserved1[4];

#if defined (__BIG_ENDIAN_BITFIELD)
	u8 overall_status_select:1;
	u8 overall_status_predictive_fault:1;
	u8 overall_status_reserved:1;
	u8 overall_status_swap:1;
	u8 overall_status_reserved2:4;

	u8 overall_status_reserved3;

	u8 overall_status_reserved4:4;
	u8 overall_status_insert:1;
	u8 overall_status_remove:1;
	u8 overall_status_reserved5:1;
	u8 overall_status_identify:1;

	u8 overall_status_reserved6:1;
	u8 overall_status_fault_requested:1;
	u8 overall_status_fault_sensed:1;
	u8 overall_status_reserved7:4;
	u8 overall_status_disable_resets:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	u8 overall_status_reserved2:4;
	u8 overall_status_swap:1;
	u8 overall_status_reserved:1;
	u8 overall_status_predictive_fault:1;
	u8 overall_status_select:1;

	u8 overall_status_reserved3;

	u8 overall_status_identify:1;
	u8 overall_status_reserved5:1;
	u8 overall_status_remove:1;
	u8 overall_status_insert:1;
	u8 overall_status_reserved4:4;

	u8 overall_status_disable_resets:1;
	u8 overall_status_reserved7:4;
	u8 overall_status_fault_sensed:1;
	u8 overall_status_fault_requested:1;
	u8 overall_status_reserved6:1;
#endif
	struct ipr_drive_elem_status elem_status[IPR_NUM_DRIVE_ELEM_STATUS_ENTRIES];
};

int sg_ioctl(int, u8 *, void *, u32, u32, struct sense_data_t *, u32);
void check_current_config(bool);
int num_device_opens(int, int, int, int);
void tool_init();
void exit_on_error(char *, ...);
bool is_af_blocked(struct ipr_dev *, int);
int ipr_query_array_config(struct ipr_ioa *, bool, bool, int, void *);
int ipr_query_command_status(struct ipr_dev *, void *);
int ipr_mode_sense(struct ipr_dev *, u8, void *);
int ipr_mode_select(struct ipr_dev *, void *, int);
int ipr_reset_device(struct ipr_dev *);
int ipr_re_read_partition(struct ipr_dev *);
int ipr_read_capacity(struct ipr_dev *, void *);
int ipr_read_capacity_16(struct ipr_dev *, void *);
int ipr_query_resource_state(struct ipr_dev *, void *);
int ipr_start_stop_start(struct ipr_dev *);
int ipr_start_stop_stop(struct ipr_dev *);
int ipr_stop_array_protection(struct ipr_ioa *);
int ipr_remove_hot_spare(struct ipr_ioa *);
int ipr_start_array_protection(struct ipr_ioa *, int, int);
int ipr_add_hot_spare(struct ipr_ioa *);
int ipr_rebuild_device_data(struct ipr_ioa *);
int ipr_test_unit_ready(struct ipr_dev *, struct sense_data_t *);
int ipr_format_unit(struct ipr_dev *);
int ipr_add_array_device(struct ipr_ioa *, struct ipr_array_query_data *);
int ipr_reclaim_cache_store(struct ipr_ioa *, int, void *);
int ipr_evaluate_device(struct ipr_dev *, u32);
int ipr_inquiry(struct ipr_dev *, u8, void *, u8);
void ipr_reset_adapter(struct ipr_ioa *);
int ipr_read_dev_attr(struct ipr_dev *, char *, char *);
int ipr_write_dev_attr(struct ipr_dev *, char *, char *);
int ipr_suspend_device_bus(struct ipr_ioa *, struct ipr_res_addr *, u8);
int ipr_receive_diagnostics(struct ipr_dev *, u8, void *, int);
int ipr_send_diagnostics(struct ipr_dev *, void *, int);
int ipr_get_bus_attr(struct ipr_ioa *, struct ipr_scsi_buses *);
void ipr_modify_bus_attr(struct ipr_ioa *, struct ipr_scsi_buses *);
int ipr_set_bus_attr(struct ipr_ioa *, struct ipr_scsi_buses *, int);
int ipr_write_buffer(struct ipr_dev *, void *, int);
int enable_af(struct ipr_dev *);
int get_max_bus_speed(struct ipr_ioa *, int);
int ipr_get_dev_attr(struct ipr_dev *, struct ipr_disk_attr *);
int ipr_modify_dev_attr(struct ipr_dev *, struct ipr_disk_attr *);
int ipr_set_dev_attr(struct ipr_dev *, struct ipr_disk_attr *, int);
int ipr_set_dasd_timeouts(struct ipr_dev *);
int get_ioa_firmware_image_list(struct ipr_ioa *, struct ipr_fw_images **);
int get_dasd_firmware_image_list(struct ipr_dev *, struct ipr_fw_images **);
void ipr_update_ioa_fw(struct ipr_ioa *, struct ipr_fw_images *, int);
void ipr_update_disk_fw(struct ipr_dev *, struct ipr_fw_images *, int);
void ipr_init_dev(struct ipr_dev *);
void ipr_init_ioa(struct ipr_ioa *);
int device_supported(struct ipr_dev *);

/*---------------------------------------------------------------------------
 * Purpose: Identify Advanced Function DASD present
 * Lock State: N/A
 * Returns: 0 if not AF DASD
 *          1 if AF DASD
 *---------------------------------------------------------------------------*/
static inline int ipr_is_af_dasd_device(struct ipr_dev *device)
{
	if ((device->qac_entry != NULL) &&
	    (device->qac_entry->record_id == IPR_RECORD_ID_DEVICE_RECORD))
		return 1;
	else
		return 0;
}

static inline int ipr_is_volume_set(struct ipr_dev *device)
{       
	if ((device->qac_entry != NULL) &&
	    (device->qac_entry->record_id == IPR_RECORD_ID_ARRAY_RECORD))
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
		return 0;
}

static inline int ipr_is_hot_spare(struct ipr_dev *device)
{
	struct ipr_dev_record *dev_record =
		(struct ipr_dev_record *)device->qac_entry;

	if ((dev_record != NULL) &&
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

static inline void ipr_strncpy_0(char *dest, char *source, int length)
{
	memcpy(dest, source, length);
	dest[length] = '\0';
}

#if (IPR_DEBUG > 0)
#define syslog_dbg(...) syslog(__VA_ARGS__)
#else
#define syslog_dbg(...)
#endif

#define scsi_log(level, dev, fmt, ...) \
      syslog(level, "%d:%d:%d:%d: " fmt, dev->ioa->host_num, \
             dev->scsi_dev_data->channel, dev->scsi_dev_data->id, \
             dev->scsi_dev_data->lun, ##__VA_ARGS__)

#define scsi_dbg(dev, fmt, ...) \
      syslog_dbg(LOG_DEBUG, "%d:%d:%d:%d: " fmt, dev->ioa->host_num, \
             dev->scsi_dev_data->channel, dev->scsi_dev_data->id, \
             dev->scsi_dev_data->lun, ##__VA_ARGS__)

#define scsi_info(dev, fmt, ...) \
      scsi_log(LOG_NOTICE, dev, fmt, ##__VA_ARGS__)

#define scsi_err(dev, fmt, ...) \
      scsi_log(LOG_ERR, dev, fmt, ##__VA_ARGS__)

#define scsi_cmd_err(dev, sense, cmd, rc) \
      scsi_err(dev, "%s failed. rc=%d, SK: %X ASC: %X ASCQ: %X\n",\
              cmd, rc, (sense)->sense_key, (sense)->add_sense_code, \
              (sense)->add_sense_code_qual)

#define ioa_log(level, ioa, fmt, ...) \
      syslog(level, "%s: " fmt, ioa->pci_address, ##__VA_ARGS__)

#define ioa_info(ioa, fmt, ...) \
      ioa_log(LOG_NOTICE, ioa, fmt, ##__VA_ARGS__)

#define ioa_err(ioa, fmt, ...) \
      ioa_log(LOG_ERR, ioa, fmt, ##__VA_ARGS__)

#define ioa_cmd_err(ioa, sense, cmd, rc) \
      ioa_err(ioa, "%s failed. rc=%d, SK: %X ASC: %X ASCQ: %X\n",\
              cmd, rc, (sense)->sense_key, (sense)->add_sense_code, \
              (sense)->add_sense_code_qual)
#endif
