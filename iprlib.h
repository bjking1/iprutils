#ifndef iprlib_h
#define iprlib_h
/******************************************************************/
/* Linux IBM SIS utility library                                  */
/* Description: IBM Storage IOA Interface Specification (SIS)     */
/*              Linux utility library                             */
/*                                                                */
/* (C) Copyright 2000, 2001                                       */
/* International Business Machines Corporation and others.        */
/* All Rights Reserved.                                           */
/******************************************************************/

/*
 * $Header: /cvsroot/iprdd/iprutils/iprlib.h,v 1.2 2003/10/22 22:44:25 bjking1 Exp $
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

#define IOCTL_BUFFER_SIZE 512
#define IBMSIS_MAX_NUM_BUSES               4
#define IBMSIS_VENDOR_ID_LEN            8
#define IBMSIS_PROD_ID_LEN              16
#define IBMSIS_SERIAL_NUM_LEN           8
#define IBMSIS_CCB_CDB_LEN      16
#define IBMSIS_EOL                              "\n"
#define IBMSIS_IOCTL_SEND_COMMAND          0xf1f1
#define IBMSIS_MAX_PSERIES_LOCATION_LEN    24
#define IBMSIS_MAX_PHYSICAL_DEVS           192
#define IBMSIS_QAC_BUFFER_SIZE             16000
#define IBMSIS_VSET_MODEL_NUMBER        200
#define IBMSIS_INVALID_ARRAY_ID                 0xFFu
#define IBMSIS_RECLAIM_NUM_BLOCKS_MULTIPLIER    256
#define  IBMSIS_SDB_CHECK_AND_QUIESCE           0x00u
#define  IBMSIS_SDB_CHECK_ONLY                  0x40u
#define IBMSIS_MAX_NUM_SUPP_INQ_PAGES   8
#define IBMSIS_DUMP_TRACE_ENTRY_SIZE            8192

#define IBMSIS_IOA_DEBUG                        0xDDu
#define   IBMSIS_IOA_DEBUG_READ_IOA_MEM         0x00u
#define   IBMSIS_IOA_DEBUG_WRITE_IOA_MEM        0x01u
#define   IBMSIS_IOA_DEBUG_READ_FLIT            0x03u

#define IBMSIS_STD_INQ_Z0_TERM_LEN      8
#define IBMSIS_STD_INQ_Z1_TERM_LEN      12
#define IBMSIS_STD_INQ_Z2_TERM_LEN      4
#define IBMSIS_STD_INQ_Z3_TERM_LEN      5
#define IBMSIS_STD_INQ_Z4_TERM_LEN      4
#define IBMSIS_STD_INQ_Z5_TERM_LEN      2
#define IBMSIS_STD_INQ_Z6_TERM_LEN      10
#define IBMSIS_STD_INQ_PART_NUM_LEN     12
#define IBMSIS_STD_INQ_EC_LEVEL_LEN     10
#define IBMSIS_STD_INQ_FRU_NUM_LEN      12

#define  IBMSIS_START_STOP_STOP                 0x00u
#define  IBMSIS_START_STOP_START                0x01u
#define  IBMSIS_READ_CAPACITY_16                0x10u
#define IBMSIS_SERVICE_ACTION_IN                0x9Eu
#define IBMSIS_QUERY_RESOURCE_STATE             0xC2u
#define IBMSIS_QUERY_IOA_CONFIG                 0xC5u
#define IBMSIS_QUERY_COMMAND_STATUS             0xCBu
#define IBMSIS_EVALUATE_DEVICE                  0xE4u
#define IBMSIS_QUERY_ARRAY_CONFIG               0xF0u
#define IBMSIS_START_ARRAY_PROTECTION           0xF1u
#define IBMSIS_STOP_ARRAY_PROTECTION            0xF2u
#define IBMSIS_RESYNC_ARRAY_PROTECTION          0xF3u
#define IBMSIS_ADD_ARRAY_DEVICE                 0xF4u
#define IBMSIS_REMOVE_ARRAY_DEVICE              0xF5u
#define IBMSIS_REBUILD_DEVICE_DATA              0xF6u
#define IBMSIS_RECLAIM_CACHE_STORE              0xF8u
#define  IBMSIS_RECLAIM_ACTION                  0x60u
#define  IBMSIS_RECLAIM_PERFORM                 0x00u
#define  IBMSIS_RECLAIM_EXTENDED_INFO           0x10u
#define  IBMSIS_RECLAIM_QUERY                   0x20u
#define  IBMSIS_RECLAIM_RESET                   0x40u
#define  IBMSIS_RECLAIM_FORCE_BATTERY_ERROR     0x60u
#define  IBMSIS_RECLAIM_UNKNOWN_PERM            0x80u
#define IBMSIS_DISCARD_CACHE_DATA               0xF9u
#define  IBMSIS_PROHIBIT_CORR_INFO_UPDATE       0x80u

#define IBMSIS_NO_REDUCTION             0
#define IBMSIS_HALF_REDUCTION           1
#define IBMSIS_QUARTER_REDUCTION        2
#define IBMSIS_EIGHTH_REDUCTION         4
#define IBMSIS_SIXTEENTH_REDUCTION      6
#define IBMSIS_UNKNOWN_REDUCTION        7

#define IBMSIS_SUBTYPE_AF_DASD          0x0
#define IBMSIS_SUBTYPE_GENERIC_SCSI     0x1
#define IBMSIS_SUBTYPE_VOLUME_SET       0x2

#define  IBMSIS_IS_DASD_DEVICE(std_inq_data) \
    ((((std_inq_data).peri_dev_type) == IBMSIS_PERI_TYPE_DISK) && !((std_inq_data).removeable_medium))

#define IBMSIS_GET_CAP_REDUCTION(res_flags) \
(((res_flags).capacity_reduction_hi << 1) | (res_flags).capacity_reduction_lo)

#define ibmsis_memory_copy memcpy
#define ibmsis_memory_set memset

#define IBMSIS_SET_MODE(change_mask, cur_val, new_val)  \
{                                                       \
int mod_bits = (cur_val ^ new_val);                     \
if ((change_mask & mod_bits) == mod_bits)               \
{                                                       \
cur_val = new_val;                                      \
}                                                       \
}

#define IBMSIS_CONC_MAINT                       0xC8u
#define  IBMSIS_CONC_MAINT_CHECK_AND_QUIESCE    IBMSIS_SDB_CHECK_AND_QUIESCE
#define  IBMSIS_CONC_MAINT_CHECK_ONLY           IBMSIS_SDB_CHECK_ONLY
#define  IBMSIS_CONC_MAINT_QUIESE_ONLY          IBMSIS_SDB_QUIESE_ONLY

#define  IBMSIS_CONC_MAINT_FMT_MASK             0x0Fu
#define  IBMSIS_CONC_MAINT_FMT_SHIFT            0
#define  IBMSIS_CONC_MAINT_GET_FMT(fmt) \
((fmt & IBMSIS_CONC_MAINT_FMT_MASK) >> IBMSIS_CONC_MAINT_FMT_SHIFT)
#define  IBMSIS_CONC_MAINT_DSA_FMT              0x00u
#define  IBMSIS_CONC_MAINT_FRAME_ID_FMT         0x01u
#define  IBMSIS_CONC_MAINT_PSERIES_FMT          0x02u
#define  IBMSIS_CONC_MAINT_XSERIES_FMT          0x03u

#define  IBMSIS_CONC_MAINT_TYPE_MASK            0x30u
#define  IBMSIS_CONC_MAINT_TYPE_SHIFT           4
#define  IBMSIS_CONC_MAINT_GET_TYPE(type) \
((type & IBMSIS_CONC_MAINT_TYPE_MASK) >> IBMSIS_CONC_MAINT_TYPE_SHIFT)
#define  IBMSIS_CONC_MAINT_INSERT               0x0u
#define  IBMSIS_CONC_MAINT_REMOVE               0x1u

#define IBMSIS_RECORD_ID_SUPPORTED_ARRAYS       _i16((u16)0)
#define IBMSIS_RECORD_ID_ARRAY_RECORD           _i16((u16)1)
#define IBMSIS_RECORD_ID_DEVICE_RECORD          _i16((u16)2)
#define IBMSIS_RECORD_ID_COMP_RECORD            _i16((u16)3)
#define IBMSIS_RECORD_ID_ARRAY2_RECORD          _i16((u16)4)
#define IBMSIS_RECORD_ID_DEVICE2_RECORD         _i16((u16)5)

/******************************************************************/
/* Driver Commands                                                */
/******************************************************************/
#define IBMSIS_GET_TRACE                        0xE1u
#define IBMSIS_RESET_DEV_CHANGED                0xE8u
#define IBMSIS_DUMP_IOA                         0xD7u
#define IBMSIS_MODE_SENSE_PAGE_28               0xD8u
#define IBMSIS_MODE_SELECT_PAGE_28              0xD9u
#define IBMSIS_RESET_HOST_ADAPTER               0xDAu
#define IBMSIS_READ_DRIVER_CFG                  0xDBu
#define IBMSIS_WRITE_DRIVER_CFG                 0xDCu


#define  IBMSIS_PERI_TYPE_DISK            0x00u

enum sis_platform
{
    SIS_ISERIES,
    SIS_PSERIES,
    SIS_GENERIC,
    SIS_UNKNOWN_PLATFORM
};

extern struct ibmsis_array_query_data *p_sis_array_query_data;
extern struct ibmsis_resource_table *p_res_table;
extern u32 num_ioas;
extern struct sis_ioa *p_head_sis_ioa;
extern struct sis_ioa *p_last_sis_ioa;
extern int driver_major;
extern int driver_minor;
extern enum sis_platform platform;

struct ibmsis_res_addr
{
    u8 reserved;
    u8 bus;
    u8 target;
    u8 lun;
#define IBMSIS_GET_PHYSICAL_LOCATOR(res_addr) \
(((res_addr).bus << 16) | ((res_addr).target << 8) | (res_addr).lun)
};

struct ibmsis_std_inq_vpids
{
    u8 vendor_id[IBMSIS_VENDOR_ID_LEN];          /* Vendor ID */
    u8 product_id[IBMSIS_PROD_ID_LEN];           /* Product ID */
};

struct ibmsis_record_common
{
    u16 record_id;
    u16 record_len;
};

#if (defined(__KERNEL__) && defined(__LITTLE_ENDIAN)) || \
(!defined(__KERNEL__) && (__BYTE_ORDER == __LITTLE_ENDIAN))
#define htosis16(x) htons(x)
#define htosis32(x) htonl(x)
#define sistoh16(x) ntohs(x)
#define sistoh32(x) ntohl(x)

#define _i16(x) htons(x)
#define _i32(x) htonl(x)

#define IBMSIS_BIG_ENDIAN       0
#define IBMSIS_LITTLE_ENDIAN    1

struct ibmsis_mode_page_hdr
{
    u8 parms_saveable:1;
    u8 reserved1:1;
    u8 page_code:6;
    u8 page_length;
};

struct ibmsis_control_mode_page
{
    /* Mode page 0x0A */
    struct ibmsis_mode_page_hdr header;
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

    u8 reserved5;
    u16 ready_aen_holdoff_period;
    u16 busy_timeout_period;
    u16 reserved6;
};

struct ibmsis_reclaim_query_data
{
    u8 action_status;
#define IBMSIS_ACTION_SUCCESSFUL               0
#define IBMSIS_ACTION_NOT_REQUIRED             1
#define IBMSIS_ACTION_NOT_PERFORMED            2
    u8 num_blocks_needs_multiplier:1;
    u8 reserved3:1;
    u8 reclaim_unknown_performed:1;
    u8 reclaim_known_performed:1;
    u8 reserved2:2;
    u8 reclaim_unknown_needed:1;
    u8 reclaim_known_needed:1;

    u16 num_blocks;

    u8 rechargeable_battery_type;
#define IBMSIS_BATTERY_TYPE_NO_BATTERY          0
#define IBMSIS_BATTERY_TYPE_NICD                1
#define IBMSIS_BATTERY_TYPE_NIMH                2
#define IBMSIS_BATTERY_TYPE_LIION               3

    u8 rechargeable_battery_error_state;
#define IBMSIS_BATTERY_NO_ERROR_STATE           0
#define IBMSIS_BATTERY_WARNING_STATE            1
#define IBMSIS_BATTERY_ERROR_STATE              2

    u8 reserved4[2];

    u16 raw_power_on_time;
    u16 adjusted_power_on_time;
    u16 estimated_time_to_battery_warning;
    u16 estimated_time_to_battery_failure;

    u8 reserved5[240];
};

/* IBM's SIS smart dump table structures */
struct ibmsis_sdt_entry
{
    u32 bar_str_offset;
    u32 end_offset;
    u8  entry_byte;
    u8  reserved[3];
    u8  reserved2:5;
    u8  valid_entry:1;
    u8  reserved1:1;
    u8  endian:1;
    u8  resv;
    u16 priority;
};

struct ibmsis_vset_res_state
{
    u16 stripe_size;
    u8 prot_level;
    u8 num_devices_in_vset;
    u32 reserved6;
};

struct ibmsis_dasd_res_state
{
    u32 data_path_width;  /* bits */
    u32 data_xfer_rate;   /* 100 KBytes/second */
};

struct ibmsis_query_res_state
{
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

    u8 reserved5;

    union
    {
        struct ibmsis_vset_res_state vset;
        struct ibmsis_dasd_res_state dasd;
    }dev;

    u32 ilid;
    u32 failing_dev_ioasc;
    struct ibmsis_res_addr failing_dev_res_addr;
    u32 failing_dev_res_handle;
    u8 protection_level_str[8];
};

struct ibmsis_array_cap_entry
{
    u8                          prot_level;
#define IBMSIS_DEFAULT_RAID_LVL "5"
    u8                          reserved:7;
    u8                          include_allowed:1;
    u16                         reserved2;
    u8                          reserved3;
    u8                          max_num_array_devices;
    u8                          min_num_array_devices;
    u8                          min_mult_array_devices;
    u16                         reserved4;
    u16                         supported_stripe_sizes;
    u16                         reserved5;
    u16                         recommended_stripe_size;
    u8                          prot_level_str[8];
};

struct ibmsis_array_record
{
    struct ibmsis_record_common common;
    u8  reserved:6;
    u8  known_zeroed:1;
    u8  issue_cmd:1;
    u8  reserved1;

    u8  reserved2:5;
    u8  non_func:1;
    u8  exposed:1;
    u8  established:1;

    u8  reserved3:5;
    u8  resync_cand:1;
    u8  stop_cand:1;
    u8  start_cand:1;

    u8  reserved4[3];
    u8  array_id;
    u32 reserved5;
};

struct ibmsis_array2_record
{
    struct ibmsis_record_common common;

    u8  reserved1:6;
    u8  known_zeroed:1;
    u8  issue_cmd:1;

    u8  reserved2;

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

    u16 stripe_size;

    u8  raid_level;
    u8  array_id;
    u32 resource_handle;
    u32 resource_address;
    struct ibmsis_res_addr last_resource_address;
    u8  vendor_id[8];
    u8  product_id[16];
    u8  serial_number[8];
    u32 reserved;
};

struct ibmsis_resource_flags
{
    u8 capacity_reduction_hi:2;
    u8 reserved2:1;
    u8 aff:1;
    u8 reserved1:1;
    u8 is_array_member:1;
    u8 is_compressed:1;
    u8 is_ioa_resource:1;

    u8 reserved3:7;
    u8 capacity_reduction_lo:1;
};

struct ibmsis_device_record
{
    struct ibmsis_record_common common;
    u8  reserved:6;
    u8  known_zeroed:1;
    u8  issue_cmd:1;

    u8  reserved1;

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

    u8  reserved4[2];
    u8  array_id;
    u32 resource_handle;
    u16 reserved5;
    struct ibmsis_resource_flags resource_flags_to_become;
    u32 user_area_size_to_become;  
};

/* 44 bytes */
struct ibmsis_std_inq_data{
    u8 peri_dev_type:5;
    u8 peri_qual:3;

    u8 reserved1:7;
    u8 removeable_medium:1;

    u8 version;

    u8 resp_data_fmt:4;
    u8 hi_sup:1;
    u8 norm_aca:1;
    u8 obsolete1:1;
    u8 aen:1;

    u8 additional_len;

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

    /* Vendor and Product ID */
    struct ibmsis_std_inq_vpids vpids;

    /* ROS and RAM levels */
    u8 ros_rsvd_ram_rsvd[4];

    /* Serial Number */
    u8 serial_num[IBMSIS_SERIAL_NUM_LEN];
};

struct ibmsis_std_inq_data_long
{
    struct ibmsis_std_inq_data std_inq_data;
    u8 z1_term[IBMSIS_STD_INQ_Z1_TERM_LEN];
    u8 ius:1;
    u8 qas:1;
    u8 clocking:2;
    u8 reserved:4;
    u8 reserved1[41];
    u8 z2_term[IBMSIS_STD_INQ_Z2_TERM_LEN];
    u8 z3_term[IBMSIS_STD_INQ_Z3_TERM_LEN];
    u8 reserved2;
    u8 z4_term[IBMSIS_STD_INQ_Z4_TERM_LEN];
    u8 z5_term[IBMSIS_STD_INQ_Z5_TERM_LEN];
    u8 part_number[IBMSIS_STD_INQ_PART_NUM_LEN];
    u8 ec_level[IBMSIS_STD_INQ_EC_LEVEL_LEN];
    u8 fru_number[IBMSIS_STD_INQ_FRU_NUM_LEN];
    u8 z6_term[IBMSIS_STD_INQ_Z6_TERM_LEN];
};

struct ibmsis_mode_page_28_scsi_dev_bus_attr
{
    struct ibmsis_res_addr res_addr;

    u8 reserved2:2;
    u8 lvd_to_se_transition_not_allowed:1;
    u8 target_mode_supported:1;
    u8 term_power_absent:1;
    u8 enable_target_mode:1;
    u8 qas_capability:2;
#define IBMSIS_MODEPAGE28_QAS_CAPABILITY_NO_CHANGE      0  
#define IBMSIS_MODEPAGE28_QAS_CAPABILITY_DISABLE_ALL    1        
#define IBMSIS_MODEPAGE28_QAS_CAPABILITY_ENABLE_ALL     2
/* NOTE:   Due to current operation conditions QAS should
 never be enabled so the change mask will be set to 0 */
#define IBMSIS_MODEPAGE28_QAS_CAPABILITY_CHANGE_MASK    0

    u8 scsi_id;
#define IBMSIS_MODEPAGE28_SCSI_ID_NO_CHANGE             0x80u
#define IBMSIS_MODEPAGE28_SCSI_ID_NO_ID                 0xFFu

    u8 bus_width;
#define IBMSIS_MODEPAGE28_BUS_WIDTH_NO_CHANGE           0

    u8 extended_reset_delay;
#define IBMSIS_EXTENDED_RESET_DELAY                     7

    u32 max_xfer_rate;
#define IBMSIS_MODEPAGE28_MAX_XFR_RATE_NO_CHANGE        0

    u8  min_time_delay;
#define IBMSIS_DEFAULT_SPINUP_DELAY                     0xFFu
#define IBMSIS_INIT_SPINUP_DELAY                        5
    u8  reserved3;
    u16 reserved4;
};

#elif (defined(__KERNEL__) && defined(__BIG_ENDIAN)) || \
(!defined(__KERNEL__) && (__BYTE_ORDER == __BIG_ENDIAN))
#define htosis16(x) (x)
#define htosis32(x) (x)
#define sistoh16(x) (x)
#define sistoh32(x) (x)
#define _i16(x) (x)
#define _i32(x) (x)

#define IBMSIS_BIG_ENDIAN       1
#define IBMSIS_LITTLE_ENDIAN    0

struct ibmsis_mode_page_hdr
{
    u8 page_code:6;
    u8 reserved1:1;
    u8 parms_saveable:1;
    u8 page_length;
};

struct ibmsis_control_mode_page
{
    /* Mode page 0x0A */
    struct ibmsis_mode_page_hdr header;
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
    u8 reserved5;
    u16 ready_aen_holdoff_period;
    u16 busy_timeout_period;
    u16 reserved6;
};

struct ibmsis_reclaim_query_data
{
    u8 action_status;
#define IBMSIS_ACTION_SUCCESSFUL               0
#define IBMSIS_ACTION_NOT_REQUIRED             1
#define IBMSIS_ACTION_NOT_PERFORMED            2
    u8 reclaim_known_needed:1;
    u8 reclaim_unknown_needed:1;
    u8 reserved2:2;
    u8 reclaim_known_performed:1;
    u8 reclaim_unknown_performed:1;
    u8 reserved3:1;
    u8 num_blocks_needs_multiplier:1;

    u16 num_blocks;

    u8 rechargeable_battery_type;
#define IBMSIS_BATTERY_TYPE_NO_BATTERY          0
#define IBMSIS_BATTERY_TYPE_NICD                1
#define IBMSIS_BATTERY_TYPE_NIMH                2
#define IBMSIS_BATTERY_TYPE_LIION               3

    u8 rechargeable_battery_error_state;
#define IBMSIS_BATTERY_NO_ERROR_STATE           0
#define IBMSIS_BATTERY_WARNING_STATE            1
#define IBMSIS_BATTERY_ERROR_STATE              2

    u8 reserved4[2];

    u16 raw_power_on_time;
    u16 adjusted_power_on_time;
    u16 estimated_time_to_battery_warning;
    u16 estimated_time_to_battery_failure;

    u8 reserved5[240];
};

/* IBM's SIS smart dump table structures */
struct ibmsis_sdt_entry
{
    u32 bar_str_offset;
    u32 end_offset;
    u8  entry_byte;
    u8  reserved[3];
    u8  endian:1;
    u8  reserved1:1;
    u8  valid_entry:1;
    u8  reserved2:5;
    u8  resv;
    u16 priority;
};

struct ibmsis_vset_res_state
{
    u16 stripe_size;
    u8 prot_level;
    u8 num_devices_in_vset;
    u32 reserved6;
};

struct ibmsis_dasd_res_state
{
    u32 data_path_width;  /* bits */
    u32 data_xfer_rate;   /* 100 KBytes/second */
};

struct ibmsis_query_res_state
{
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

    u8 reserved5;

    union
    {
        struct ibmsis_vset_res_state vset;
        struct ibmsis_dasd_res_state dasd;
    }dev;

    u32 ilid;
    u32 failing_dev_ioasc;
    struct ibmsis_res_addr failing_dev_res_addr;
    u32 failing_dev_res_handle;
    u8 protection_level_str[8];
};

struct ibmsis_array_cap_entry
{
    u8                          prot_level;
#define IBMSIS_DEFAULT_RAID_LVL "5"
    u8                          include_allowed:1;
    u8                          reserved:7;
    u16                         reserved2;
    u8                          reserved3;
    u8                          max_num_array_devices;
    u8                          min_num_array_devices;
    u8                          min_mult_array_devices;
    u16                         reserved4;
    u16                         supported_stripe_sizes;
    u16                         reserved5;
    u16                         recommended_stripe_size;
    u8                          prot_level_str[8];
};

struct ibmsis_array_record
{
    struct ibmsis_record_common common;
    u8  issue_cmd:1;
    u8  known_zeroed:1;
    u8  reserved:6;
    u8  reserved1;
    u8  established:1;
    u8  exposed:1;
    u8  non_func:1;
    u8  reserved2:5;
    u8  start_cand:1;
    u8  stop_cand:1;
    u8  resync_cand:1;
    u8  reserved3:5;
    u8  reserved4[3];
    u8  array_id;
    u32 reserved5;
};

struct ibmsis_array2_record
{
    struct ibmsis_record_common common;

    u8  issue_cmd:1;
    u8  known_zeroed:1;
    u8  reserved1:6;

    u8  reserved2;

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

    u16 stripe_size;

    u8  raid_level;
    u8  array_id;
    u32 resource_handle;
    u32 resource_address;
    struct ibmsis_res_addr last_resource_address;
    u8  vendor_id[8];
    u8  product_id[16];
    u8  serial_number[8];
    u32 reserved;
};

struct ibmsis_resource_flags
{
    u8 is_ioa_resource:1;
    u8 is_compressed:1;
    u8 is_array_member:1;
    u8 reserved1:1;
    u8 aff:1;
    u8 reserved2:1;
    u8 capacity_reduction_hi:2;

    u8 capacity_reduction_lo:1;
    u8 reserved3:7;
};

struct ibmsis_device_record
{
    struct ibmsis_record_common common;
    u8  issue_cmd:1;
    u8  known_zeroed:1;
    u8  reserved:6;

    u8  reserved1;

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

    u8  reserved4[2];
    u8  array_id;
    u32 resource_handle;
    u16 reserved5;
    struct ibmsis_resource_flags resource_flags_to_become;
    u32 user_area_size_to_become;  
};

/* 44 bytes */
struct ibmsis_std_inq_data{
    u8 peri_qual:3;
    u8 peri_dev_type:5;

    u8 removeable_medium:1;
    u8 reserved1:7;

    u8 version;

    u8 aen:1;
    u8 obsolete1:1;
    u8 norm_aca:1;
    u8 hi_sup:1;
    u8 resp_data_fmt:4;

    u8 additional_len;

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

    /* Vendor and Product ID */
    struct ibmsis_std_inq_vpids vpids;

    /* ROS and RAM levels */
    u8 ros_rsvd_ram_rsvd[4];

    /* Serial Number */
    u8 serial_num[IBMSIS_SERIAL_NUM_LEN];
};

struct ibmsis_std_inq_data_long
{
    struct ibmsis_std_inq_data std_inq_data;
    u8 z1_term[IBMSIS_STD_INQ_Z1_TERM_LEN];
    u8 reserved:4;
    u8 clocking:2;
    u8 qas:1;
    u8 ius:1;
    u8 reserved1[41];
    u8 z2_term[IBMSIS_STD_INQ_Z2_TERM_LEN];
    u8 z3_term[IBMSIS_STD_INQ_Z3_TERM_LEN];
    u8 reserved2;
    u8 z4_term[IBMSIS_STD_INQ_Z4_TERM_LEN];
    u8 z5_term[IBMSIS_STD_INQ_Z5_TERM_LEN];
    u8 part_number[IBMSIS_STD_INQ_PART_NUM_LEN];
    u8 ec_level[IBMSIS_STD_INQ_EC_LEVEL_LEN];
    u8 fru_number[IBMSIS_STD_INQ_FRU_NUM_LEN];
    u8 z6_term[IBMSIS_STD_INQ_Z6_TERM_LEN];
};

struct ibmsis_mode_page_28_scsi_dev_bus_attr
{
    struct ibmsis_res_addr res_addr;
    u8 qas_capability:2;
#define IBMSIS_MODEPAGE28_QAS_CAPABILITY_NO_CHANGE      0  
#define IBMSIS_MODEPAGE28_QAS_CAPABILITY_DISABLE_ALL    1        
#define IBMSIS_MODEPAGE28_QAS_CAPABILITY_ENABLE_ALL     2
/* NOTE:   Due to current operation conditions QAS should
 never be enabled so the change mask will be set to 0 */
#define IBMSIS_MODEPAGE28_QAS_CAPABILITY_CHANGE_MASK    0

    u8 enable_target_mode:1;
    u8 term_power_absent:1;
    u8 target_mode_supported:1;
    u8 lvd_to_se_transition_not_allowed:1;
    u8 reserved2:2;

    u8 scsi_id;
#define IBMSIS_MODEPAGE28_SCSI_ID_NO_CHANGE             0x80u
#define IBMSIS_MODEPAGE28_SCSI_ID_NO_ID                 0xFFu

    u8 bus_width;
#define IBMSIS_MODEPAGE28_BUS_WIDTH_NO_CHANGE           0

    u8 extended_reset_delay;
#define IBMSIS_EXTENDED_RESET_DELAY                     7

    u32 max_xfer_rate;
#define IBMSIS_MODEPAGE28_MAX_XFR_RATE_NO_CHANGE        0

    u8  min_time_delay;
#define IBMSIS_DEFAULT_SPINUP_DELAY                     0xFFu
#define IBMSIS_INIT_SPINUP_DELAY                        5
    u8  reserved3;
    u16 reserved4;
};
#else
#error "Neither __LITTLE_ENDIAN nor __BIG_ENDIAN defined"
#endif

#define SIS_CONFIG_DIR "/etc/ibmsis/"
#define SIS_QAS_CAPABILITY "QAS_CAPABILITY"
#define SIS_HOST_SCSI_ID "HOST_SCSI_ID"
#define SIS_BUS_WIDTH "BUS_WIDTH"
#define SIS_MAX_XFER_RATE "MAX_BUS_SPEED"
#define SIS_MIN_TIME_DELAY "MIN_TIME_DELAY"
#define SIS_CATEGORY_BUS "Bus"
#define SIS_LIMITED_MAX_XFER_RATE 80
/* SIS_LIMITED_CONFIG is used to compensate for
 a set of devices which will not allow firmware
 udpates when operating at high max transfer rates.
 Selecting the LIMITED_CONFIG adjusts the max
 transfer rate allowing successfull firmware update
 operations to be performed */
#define SIS_LIMITED_CONFIG 0x7FFF
#define SIS_SAVE_LIMITED_CONFIG 0x8000
#define SIS_NORMAL_CONFIG 0

/* Internal return codes */
#define RC_SUCCESS          0
#define RC_FAILED          -1
#define RC_NOOP		   -2
#define RC_NOT_PERFORMED   -3
#define RC_IN_USE          -4
#define RC_CANCEL          12
#define RC_REFRESH          1
#define RC_EXIT             3

struct sis_device{
    char                              dev_name[64];
    char                              gen_name[64];
    int                               opens;
    u32                               is_start_cand:1;
    u32                               is_reclaim_cand:1;
    u32                               reserved:30;
    struct ibmsis_resource_entry     *p_resource_entry;
    struct ibmsis_record_common      *p_qac_entry;
};

struct sis_ioa{
    struct sis_device                 ioa;
    u16                               ccin;
    u32                               host_num;
    u32                               host_addr;
    char                              serial_num[9];
    u32                               num_raid_cmds;
    char                              driver_version[50];
    char                              firmware_version[40];
    char                              part_num[20];
    u16                               num_devices;
    struct sis_device                *dev;
    struct ibmsis_resource_table     *p_resource_table;
    struct ibmsis_array_query_data   *p_qac_data;
    struct ibmsis_supported_arrays   *p_supported_arrays;
    struct ibmsis_reclaim_query_data *p_reclaim_data;
    struct sis_ioa                   *p_next;
    struct sis_ioa                   *p_cmd_next;
};

struct sg_proc_info
{
    int    host;
    int    channel;
    int    id;
    int    lun;
    int    type;
    int    opens;
    int    qdepth;
    int    busy;
    int    online;
};

struct sg_map_info
{
    char              dev_name[64];
    char              gen_name[64];
    struct sg_scsi_id sg_dat;
};

struct ibmsis_dasd_inquiry_page3
{
    u8 peri_qual_dev_type;
    u8 page_code;
    u8 reserved1;
    u8 page_length;
    u8 ascii_len;
    u8 reserved2[3];
    u8 load_id[4];
    u8 release_level[4];
};

struct ibmsis_array_query_data
{
    u16 resp_len;
    u8  reserved;
    u8  num_records;
    u8 data[IBMSIS_QAC_BUFFER_SIZE];
};

struct ibmsis_block_desc {
    u8 num_blocks[4];
    u8 density_code;
    u8 block_length[3];
};

struct ibmsis_mode_parm_hdr
{
    u8 length;
    u8 medium_type;
    u8 device_spec_parms;
    u8 block_desc_len;
};

struct sense_data_t {
    u8                   error_code;
    u8                   segment_numb;
    u8                   sense_key;
    u8                   info[4];
    u8                   add_sense_len;
    u8                   cmd_spec_info[4];
    u8                   add_sense_code;
    u8                   add_sense_code_qual;
    u8                   field_rep_unit_code;
    u8                   sense_key_spec[3];
    u8                   add_sense_bytes[0];
};

struct ibmsis_resource_entry
{
    u8 is_ioa_resource:1;
    u8 is_compressed:1;
    u8 is_array_member:1;
    u8 format_allowed:1;
    u8 dev_changed:1;
    u8 in_init:1;
    u8 redo_init:1;
    u8 rw_protected:1;

    u8 level;
    u8 array_id;
    u8 subtype;

    struct ibmsis_res_addr resource_address;

    u16 type;

    u16 model;

    /* The following two fields are used only for DASD */
    u32 sw_load_id;

    u32 sw_release_level;

    char serial_num[IBMSIS_SERIAL_NUM_LEN+1]; /* Null terminated ascii */

    u8 is_hidden:1;
    u8 is_af:1;
    u8 is_hot_spare:1;
    u8 supports_qas:1;
    u8 nr_ioa_microcode:1;
    u8 ioa_dead:1;
    u8 reserved4:2;

    u16 host_no;

    u32 resource_handle;                /* In big endian byteorder */

    /* NOTE: DSA/UA and frame_id/slot_label are only valid in iSeries Linux */
    /*       In other architectures, DSA/UA will be set to zero and         */
    /*       frame_id/slot_label will be initialized to ASCII spaces        */
    u32 dsa;
#define IBMSIS_SYS_IBMSIS_BUS_MASK              0xffff0000
#define IBMSIS_SYS_CARD_MASK                    0x0000ff00
#define IBMSIS_IO_ADAPTER_MASK                  0x000000ff
#define IBMSIS_GET_SYS_BUS(dsa)                                        \
    ((dsa & IBMSIS_SYS_IBMSIS_BUS_MASK) >> 16)
#define IBMSIS_GET_SYS_CARD(dsa)                                       \
    ((dsa & IBMSIS_SYS_CARD_MASK) >> 8)
#define IBMSIS_GET_IO_ADAPTER(dsa)                                     \
    (dsa & IBMSIS_IO_ADAPTER_MASK)

    u32 unit_address;
#define IBMSIS_IO_BUS_MASK                     0x0f000000
#define IBMSIS_CTL_MASK                        0x00ff0000
#define IBMSIS_DEV_MASK                        0x0000ff00
#define IBMSIS_GET_IO_BUS(ua)                                          \
    ((ua & IBMSIS_IO_BUS_MASK) >> 24)
#define IBMSIS_GET_CTL(ua)                                             \
    ((ua & IBMSIS_CTL_MASK) >> 16)
#define IBMSIS_GET_DEV(ua)                                             \
    ((ua & IBMSIS_DEV_MASK) >> 8)

    u32 pci_bus_number;
    u32 pci_slot;

    u8 frame_id[3];
    u8 slot_label[4];
    u8 pseries_location[IBMSIS_MAX_PSERIES_LOCATION_LEN+1];

    u8 part_number[IBMSIS_STD_INQ_PART_NUM_LEN+1];
    u8 ec_level[IBMSIS_STD_INQ_EC_LEVEL_LEN+1];
    u8 fru_number[IBMSIS_STD_INQ_FRU_NUM_LEN+1];
    u8 z1_term[IBMSIS_STD_INQ_Z1_TERM_LEN+1];
    u8 z2_term[IBMSIS_STD_INQ_Z2_TERM_LEN+1];
    u8 z3_term[IBMSIS_STD_INQ_Z3_TERM_LEN+1];
    u8 z4_term[IBMSIS_STD_INQ_Z4_TERM_LEN+1];
    u8 z5_term[IBMSIS_STD_INQ_Z5_TERM_LEN+1];
    u8 z6_term[IBMSIS_STD_INQ_Z6_TERM_LEN+1];

    struct ibmsis_std_inq_data std_inq_data;
};

struct ibmsis_resource_hdr
{
    u16 num_entries;
    u16 reserved;
};

struct ibmsis_resource_table
{
    struct ibmsis_resource_hdr   hdr;
    struct ibmsis_resource_entry dev[IBMSIS_MAX_PHYSICAL_DEVS];
};

struct ibmsis_mode_page_28_header
{
    struct ibmsis_mode_page_hdr header;
    u8 num_dev_entries;
    u8 dev_entry_length;
};

struct sis_page_28
{
    struct ibmsis_mode_page_28_header page_hdr;
    struct ibmsis_mode_page_28_scsi_dev_bus_attr attr[IBMSIS_MAX_NUM_BUSES];
};

struct ibmsis_page_28
{
    struct ibmsis_mode_parm_hdr parm_hdr;
    struct ibmsis_mode_page_28_header page_hdr;
    struct ibmsis_mode_page_28_scsi_dev_bus_attr attr[IBMSIS_MAX_NUM_BUSES];
};

struct ibmsis_supported_arrays
{
    struct ibmsis_record_common common;
    u16                         num_entries;
    u16                         entry_length;
    u8                          data[0];
};

struct ibmsis_read_cap
{
    u32 max_user_lba;
    u32 block_length;
};

struct ibmsis_read_cap16
{
    u32 max_user_lba_hi;
    u32 max_user_lba_lo;
    u32 block_length;
};

/* Struct for disks that are unsupported or require a minimum microcode
 level prior to formatting to 522-byte sectors. */
struct unsupported_af_dasd
{
    char vendor_id[IBMSIS_VENDOR_ID_LEN + 1];
    char compare_vendor_id_byte[IBMSIS_VENDOR_ID_LEN];
    char product_id[IBMSIS_PROD_ID_LEN + 1];
    char compare_product_id_byte[IBMSIS_PROD_ID_LEN];
    struct ibmsis_std_inq_vpids vpid;   /* vpid - meaningful bits */
    struct ibmsis_std_inq_vpids vpid_mask;  /* mask don't cares in the vpid */
    char lid[5];       /* Load ID - Bytes 8-11 of Inquiry Page 3 */
    char lid_mask[4];  /* Mask for certain bytes of the LID */
    uint supported_with_min_ucode_level;
    char min_ucode_level[5];
    char min_ucode_mask[4];    /* used to mask don't cares in the ucode level */
};

struct ibmsis_cmd_status_record
{
    u16 reserved1;
    u16 length;
    u8 array_id;
    u8 command_code;
    u8 status;
#define IBMSIS_CMD_STATUS_SUCCESSFUL            0
#define IBMSIS_CMD_STATUS_IN_PROGRESS           2
#define IBMSIS_CMD_STATUS_ATTRIB_CHANGE         3
#define IBMSIS_CMD_STATUS_FAILED                4
#define IBMSIS_CMD_STATUS_INSUFF_DATA_MOVED     5

    u8 percent_complete;
    struct ibmsis_res_addr failing_dev_res_addr;
    u32 failing_dev_res_handle;
    u32 failing_dev_ioasc;
    u32 ilid;
    u32 resource_handle;
};

struct ibmsis_cmd_status
{
    u16 resp_len;
    u8  reserved;
    u8  num_records;
    struct ibmsis_cmd_status_record record[100];
};

struct ibmsis_std_inq_vpids_sn
{
    struct ibmsis_std_inq_vpids vpids;
    u8 serial_num[IBMSIS_SERIAL_NUM_LEN];
};

struct ibmsis_discard_cache_data
{
    u32 length;
    union { 
        struct ibmsis_std_inq_vpids_sn vpids_sn;
        u32 add_cmd_parms[10];
    }data;
};

struct ibmsis_software_inq_lid_info
{
    u32  load_id;
    u32  timestamp[3];
};

struct ibmsis_inquiry_page0  /* Supported Vital Product Data Pages */
{
    u8 peri_qual_dev_type;
    u8 page_code;
    u8 reserved1;
    u8 page_length;
    u8 supported_page_codes[IBMSIS_MAX_NUM_SUPP_INQ_PAGES];
};

struct ibmsis_inquiry_page3
{
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

struct ibmsis_dasd_ucode_header
{
    u8 length[3];
    u8 load_id[4];
    u8 modification_level[4];
    u8 ptf_number[4];
    u8 patch_number[4];
};

struct ibmsis_inquiry_page_cx  /* Extended Software Inquiry  */
{
    u8 peri_qual_dev_type;
    u8 page_code;
    u8 reserved1;
    u8 page_length;
    u8 ascii_length;
    u8 reserved2[3];

    struct ibmsis_software_inq_lid_info lidinfo[15];
};

struct ibmsis_driver_cfg
{
    u16 debug_level;
    u16 trace_level;
    u16 debug_level_max;
    u16 trace_level_max;
};

/* NOTE: The structure below is a shared structure with user-land tools */
/* We need to make sure we don't put pointers in here as the
 utilities could be running in 32 bit mode on a 64 bit kernel. */
struct ibmsis_ioctl_cmd_type2
{
    u32 type:8;  /* type is used to distinguish between ioctl_cmd structure formats */
#define IBMSIS_IOCTL_TYPE_2     0x03
    u32 reserved:21;
    u32 read_not_write:1; /* data direction */
    u32 device_cmd:1; /* used to pass commands to specific devices identified by resource address */
    u32 driver_cmd:1; /* used exclusively to pass commands to the device driver, 0 otherwise */
    struct ibmsis_res_addr resource_address;
#define IBMSIS_CDB_LEN          16
    u8 cdb[IBMSIS_CDB_LEN];
    u32 buffer_len;
    u8 buffer[0];
};

/* The structures below are deprecated and should not be used. Use the
 structure above to send ioctls instead */
struct ibmsis_ioctl_cmd_internal
{
    u32 read_not_write:1;
    u32 device_cmd:1;
    u32 driver_cmd:1;
    u32 reserved:29;
    struct ibmsis_res_addr resource_address;
#define IBMSIS_CDB_LEN     16
    u8 cdb[IBMSIS_CDB_LEN];
    void *buffer;
    u32 buffer_len;
};

int get_proc_string(char *proc_file_name, char *label, char *buffer);
int scan_device(struct ibmsis_res_addr resource_addr, u32 host_num);
int remove_device(struct ibmsis_res_addr resource_addr, struct sis_ioa *p_ioa);
int sis_ioctl(int fd, u32 cmd, struct ibmsis_ioctl_cmd_internal *p_ioctl_cmd);
int sg_ioctl(int fd, u8 cdb[IBMSIS_CCB_CDB_LEN],
             void *p_data, u32 xfer_len, u32 data_direction,
             struct sense_data_t *p_sense_data,
             u32 timeout_in_sec);
int get_major_version(char *proc_file_name);
int get_minor_version(char *proc_file_name);
void check_current_config(bool allow_rebuild_refresh);
void get_sg_ioctl_data(struct sg_map_info *p_sg_map_info, int num_devs);
int get_sg_proc_data(struct sg_proc_info *p_sg_proc_info);
int num_device_opens(int host_num, int channel, int id, int lun);
void tool_init();
void exit_on_error(char *, ...);
int sis_config_file_valid(char *usr_file_name);
int  sis_config_file_read(char *usr_file_name,
                          char *category,
                          char *field,
                          char *value);
void  sis_config_file_entry(char *usr_file_name,
                            char *category,
                            char *field,
                            char *value,
                            int update);
void sis_save_page_28(struct sis_ioa *p_ioa,
                      struct sis_page_28 *p_page_28_cur,
                      struct sis_page_28 *p_page_28_chg,
                      struct sis_page_28 *p_page_28_sis);
void sis_set_page_28(struct sis_ioa *p_cur_ioa,
                     int limited_config,
                     int reset_scheduled);
void sis_set_page_28_init(struct sis_ioa *p_cur_ioa,
                          int limited_config);
void sislog_location(struct ibmsis_resource_entry *);
bool is_af_blocked(struct sis_device *p_sis_device, int silent);

/*---------------------------------------------------------------------------
 * Purpose: Identify Advanced Function DASD present
 * Lock State: N/A
 * Returns: 0 if not AF DASD
 *          1 if AF DASD
 *---------------------------------------------------------------------------*/
static inline int ibmsis_is_af_dasd_device(struct ibmsis_resource_entry *p_resource_entry)
{
    if (IBMSIS_IS_DASD_DEVICE(p_resource_entry->std_inq_data) &&
        (!p_resource_entry->is_ioa_resource) &&
        (p_resource_entry->subtype == IBMSIS_SUBTYPE_AF_DASD))
        return 1;
    else
        return 0;
}

#if (IBMSIS_DEBUG > 0)
#define syslog_dbg(...) syslog(__VA_ARGS__)
#else
#define syslog_dbg(...)
#endif

#endif
