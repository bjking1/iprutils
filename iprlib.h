#ifndef iprlib_h
#define iprlib_h
/******************************************************************/
/* Linux IBM IPR utility library                                  */
/* Description: IBM Storage IOA Interface Specification (IPR)     */
/*              Linux utility library                             */
/*                                                                */
/* (C) Copyright 2000, 2001                                       */
/* International Business Machines Corporation and others.        */
/* All Rights Reserved.                                           */
/******************************************************************/

/*
 * $Header: /cvsroot/iprdd/iprutils/iprlib.h,v 1.5 2004/01/13 21:13:32 manderso Exp $
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
#define IPR_MAX_NUM_BUSES               4
#define IPR_VENDOR_ID_LEN            8
#define IPR_PROD_ID_LEN              16
#define IPR_SERIAL_NUM_LEN           8
#define IPR_VPD_PLANT_CODE_LEN           4
#define IPR_VPD_CACHE_SIZE_LEN           3
#define IPR_VPD_DRAM_SIZE_LEN           3
#define IPR_VPD_PART_NUM_LEN             12
#define IPR_CCB_CDB_LEN      16
#define IPR_EOL                              "\n"
#define IPR_IOCTL_SEND_COMMAND          0xf1f1
#define IPR_MAX_PSERIES_LOCATION_LEN    48
#define IPR_MAX_PHYSICAL_DEVS           192
#define IPR_QAC_BUFFER_SIZE             16000
#define IPR_VSET_MODEL_NUMBER        200
#define IPR_INVALID_ARRAY_ID                 0xFFu
#define IPR_IOA_RESOURCE_HANDLE         0xffffffff
#define IPR_RECLAIM_NUM_BLOCKS_MULTIPLIER    256
#define  IPR_SDB_CHECK_AND_QUIESCE           0x00u
#define  IPR_SDB_CHECK_ONLY                  0x40u
#define IPR_MAX_NUM_SUPP_INQ_PAGES   8
#define IPR_DUMP_TRACE_ENTRY_SIZE            8192
#define IPR_MODE_SENSE_LENGTH                255
#define IPR_DEFECT_LIST_HDR_LEN      4
#define IPR_FORMAT_DATA              0x10u
#define IPR_FORMAT_IMMED             2
#define  IPR_PERI_TYPE_SES             0x0Du
#define IPR_ARRAY_CMD_TIMEOUT                (2 * 60)     /* 2 minutes */
#define IPR_INTERNAL_DEV_TIMEOUT             (2 * 60)     /* 2 minutes */
#define IPR_FORMAT_UNIT_TIMEOUT              (4 * 60 * 60) /* 4 hours */
#define IPR_INTERNAL_TIMEOUT                 (30)         /* 30 seconds */
#define IPR_EVALUATE_DEVICE_TIMEOUT          (2 * 60)     /* 2 minutes */
#define IPR_WRITE_BUFFER_TIMEOUT             (10 * 60)    /* 10 minutes */

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

#define  IPR_START_STOP_STOP                 0x00u
#define  IPR_START_STOP_START                0x01u
#define  IPR_READ_CAPACITY_16                0x10u
#define IPR_SERVICE_ACTION_IN                0x9Eu
#define IPR_QUERY_RESOURCE_STATE             0xC2u
#define IPR_QUERY_IOA_CONFIG                 0xC5u
#define IPR_QUERY_COMMAND_STATUS             0xCBu
#define IPR_EVALUATE_DEVICE                  0xE4u
#define IPR_QUERY_ARRAY_CONFIG               0xF0u
#define IPR_START_ARRAY_PROTECTION           0xF1u
#define IPR_STOP_ARRAY_PROTECTION            0xF2u
#define IPR_RESYNC_ARRAY_PROTECTION          0xF3u
#define IPR_ADD_ARRAY_DEVICE                 0xF4u
#define IPR_REMOVE_ARRAY_DEVICE              0xF5u
#define IPR_REBUILD_DEVICE_DATA              0xF6u
#define IPR_RECLAIM_CACHE_STORE              0xF8u
#define  IPR_RECLAIM_ACTION                  0x60u
#define  IPR_RECLAIM_PERFORM                 0x00u
#define  IPR_RECLAIM_EXTENDED_INFO           0x10u
#define  IPR_RECLAIM_QUERY                   0x20u
#define  IPR_RECLAIM_RESET                   0x40u
#define  IPR_RECLAIM_FORCE_BATTERY_ERROR     0x60u
#define  IPR_RECLAIM_UNKNOWN_PERM            0x80u
#define IPR_DISCARD_CACHE_DATA               0xF9u
#define  IPR_PROHIBIT_CORR_INFO_UPDATE       0x80u
#define IPR_MODE_SELECT                      0x15u
#define IPR_MODE_SENSE                       0x1Au
#define IPR_STD_INQUIRY                      0xFFu

#define IPR_NO_REDUCTION             0
#define IPR_HALF_REDUCTION           1
#define IPR_QUARTER_REDUCTION        2
#define IPR_EIGHTH_REDUCTION         4
#define IPR_SIXTEENTH_REDUCTION      6
#define IPR_UNKNOWN_REDUCTION        7

#define IPR_SUBTYPE_AF_DASD          0x0
#define IPR_SUBTYPE_GENERIC_SCSI     0x1
#define IPR_SUBTYPE_VOLUME_SET       0x2

#define  IPR_IS_DASD_DEVICE(std_inq_data) \
    ((((std_inq_data).peri_dev_type) == IPR_PERI_TYPE_DISK) && !((std_inq_data).removeable_medium))

#define IPR_GET_CAP_REDUCTION(res_flags) \
(((res_flags).capacity_reduction_hi << 1) | (res_flags).capacity_reduction_lo)

#define ipr_memory_copy memcpy
#define ipr_memory_set memset

#define IPR_SET_MODE(change_mask, cur_val, new_val)  \
{                                                       \
int mod_bits = (cur_val ^ new_val);                     \
if ((change_mask & mod_bits) == mod_bits)               \
{                                                       \
cur_val = new_val;                                      \
}                                                       \
}

#define  IPR_IS_SES_DEVICE(std_inq_data) \
    (((std_inq_data).peri_dev_type) == IPR_PERI_TYPE_SES)

#define IPR_CONC_MAINT                       0xC8u
#define  IPR_CONC_MAINT_CHECK_AND_QUIESCE    IPR_SDB_CHECK_AND_QUIESCE
#define  IPR_CONC_MAINT_CHECK_ONLY           IPR_SDB_CHECK_ONLY
#define  IPR_CONC_MAINT_QUIESE_ONLY          IPR_SDB_QUIESE_ONLY

#define  IPR_CONC_MAINT_FMT_MASK             0x0Fu
#define  IPR_CONC_MAINT_FMT_SHIFT            0
#define  IPR_CONC_MAINT_GET_FMT(fmt) \
((fmt & IPR_CONC_MAINT_FMT_MASK) >> IPR_CONC_MAINT_FMT_SHIFT)
#define  IPR_CONC_MAINT_DSA_FMT              0x00u
#define  IPR_CONC_MAINT_FRAME_ID_FMT         0x01u
#define  IPR_CONC_MAINT_PSERIES_FMT          0x02u
#define  IPR_CONC_MAINT_XSERIES_FMT          0x03u

#define  IPR_CONC_MAINT_TYPE_MASK            0x30u
#define  IPR_CONC_MAINT_TYPE_SHIFT           4
#define  IPR_CONC_MAINT_GET_TYPE(type) \
((type & IPR_CONC_MAINT_TYPE_MASK) >> IPR_CONC_MAINT_TYPE_SHIFT)
#define  IPR_CONC_MAINT_INSERT               0x0u
#define  IPR_CONC_MAINT_REMOVE               0x1u

#define IPR_RECORD_ID_SUPPORTED_ARRAYS       _i16((u16)0)
#define IPR_RECORD_ID_ARRAY_RECORD           _i16((u16)1)
#define IPR_RECORD_ID_DEVICE_RECORD          _i16((u16)2)
#define IPR_RECORD_ID_COMP_RECORD            _i16((u16)3)
#define IPR_RECORD_ID_ARRAY2_RECORD          _i16((u16)4)
#define IPR_RECORD_ID_DEVICE2_RECORD         _i16((u16)5)

/******************************************************************/
/* Driver Commands                                                */
/******************************************************************/
#define IPR_GET_TRACE                        0xE1u
#define IPR_RESET_DEV_CHANGED                0xE8u
#define IPR_DUMP_IOA                         0xD7u
#define IPR_MODE_SENSE_PAGE_28               0xD8u
#define IPR_MODE_SELECT_PAGE_28              0xD9u
#define IPR_RESET_HOST_ADAPTER               0xDAu
#define IPR_READ_DRIVER_CFG                  0xDBu
#define IPR_WRITE_DRIVER_CFG                 0xDCu


#define  IPR_PERI_TYPE_DISK            0x00u

extern struct ipr_array_query_data *p_ipr_array_query_data;
extern struct ipr_resource_table *p_res_table;
extern u32 num_ioas;
extern struct ipr_ioa *p_head_ipr_ioa;
extern struct ipr_ioa *p_last_ipr_ioa;
extern int driver_major;
extern int driver_minor;

struct ipr_res_addr
{
    u8 reserved;
    u8 bus;
    u8 target;
    u8 lun;
#define IPR_GET_PHYSICAL_LOCATOR(res_addr) \
(((res_addr).bus << 16) | ((res_addr).target << 8) | (res_addr).lun)
};

struct ipr_std_inq_vpids
{
    u8 vendor_id[IPR_VENDOR_ID_LEN];          /* Vendor ID */
    u8 product_id[IPR_PROD_ID_LEN];           /* Product ID */
};

struct ipr_record_common
{
    u16 record_id;
    u16 record_len;
};

/**********************************************************/ 
/*                                                        */
/* L I T T L E    E N D I A N                             */
/*                                                        */
/**********************************************************/
#if (defined(__KERNEL__) && defined(__LITTLE_ENDIAN)) || \
(!defined(__KERNEL__) && (__BYTE_ORDER == __LITTLE_ENDIAN))
#define htoipr16(x) htons(x)
#define htoipr32(x) htonl(x)
#define iprtoh16(x) ntohs(x)
#define iprtoh32(x) ntohl(x)

#define _i16(x) htons(x)
#define _i32(x) htonl(x)

#define IPR_BIG_ENDIAN       0
#define IPR_LITTLE_ENDIAN    1

struct ipr_mode_page_hdr
{
    u8 page_code:6;
    u8 reserved1:1;
    u8 parms_saveable:1;
    u8 page_length;
};

struct ipr_control_mode_page
{
    /* Mode page 0x0A */
    struct ipr_mode_page_hdr header;
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

struct ipr_reclaim_query_data
{
    u8 action_status;
#define IPR_ACTION_SUCCESSFUL               0
#define IPR_ACTION_NOT_REQUIRED             1
#define IPR_ACTION_NOT_PERFORMED            2
    u8 num_blocks_needs_multiplier:1;
    u8 reserved3:1;
    u8 reclaim_unknown_performed:1;
    u8 reclaim_known_performed:1;
    u8 reserved2:2;
    u8 reclaim_unknown_needed:1;
    u8 reclaim_known_needed:1;

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

/* IBM's IPR smart dump table structures */
struct ipr_sdt_entry
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

struct ipr_vset_res_state
{
    u16 stripe_size;
    u8 prot_level;
    u8 num_devices_in_vset;
    u32 reserved6;
};

struct ipr_dasd_res_state
{
    u32 data_path_width;  /* bits */
    u32 data_xfer_rate;   /* 100 KBytes/second */
};

struct ipr_query_res_state
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
        struct ipr_vset_res_state vset;
        struct ipr_dasd_res_state dasd;
    }dev;

    u32 ilid;
    u32 failing_dev_ioasc;
    struct ipr_res_addr failing_dev_res_addr;
    u32 failing_dev_res_handle;
    u8 protection_level_str[8];
};

struct ipr_array_cap_entry
{
    u8                          prot_level;
#define IPR_DEFAULT_RAID_LVL "5"
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

struct ipr_array_record
{
    struct ipr_record_common common;
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

struct ipr_array2_record
{
    struct ipr_record_common common;

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
    struct ipr_res_addr last_resource_address;
    u8  vendor_id[8];
    u8  product_id[16];
    u8  serial_number[8];
    u32 reserved;
};

struct ipr_resource_flags
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

struct ipr_device_record
{
    struct ipr_record_common common;
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
    struct ipr_resource_flags resource_flags_to_become;
    u32 user_area_size_to_become;  
};

/* 44 bytes */
struct ipr_std_inq_data{
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
    struct ipr_std_inq_vpids vpids;

    /* ROS and RAM levels */
    u8 ros_rsvd_ram_rsvd[4];

    /* Serial Number */
    u8 serial_num[IPR_SERIAL_NUM_LEN];
};

struct ipr_std_inq_data_long
{
    struct ipr_std_inq_data std_inq_data;
    u8 z1_term[IPR_STD_INQ_Z1_TERM_LEN];
    u8 ius:1;
    u8 qas:1;
    u8 clocking:2;
    u8 reserved:4;
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

struct ipr_mode_page_28_scsi_dev_bus_attr
{
    struct ipr_res_addr res_addr;

    u8 reserved2:2;
    u8 lvd_to_se_transition_not_allowed:1;
    u8 target_mode_supported:1;
    u8 term_power_absent:1;
    u8 enable_target_mode:1;
    u8 qas_capability:2;
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

struct ipr_resource_entry
{
    u8 level;
    u8 array_id;

    u8 capacity_reduction_hi:2;
    u8 reserved2:1;
    u8 is_af:1;
    u8 is_hot_spare:1;
    u8 is_array_member:1;
    u8 is_compressed:1;
    u8 is_ioa_resource:1;

    u8 subtype:4;
    u8 reserved3:3;
    u8 capacity_reduction_lo:1;

    struct ipr_res_addr resource_address;
    u32 resource_handle;
    u32 reserved4;
    u32 reserved5;
    struct ipr_std_inq_data std_inq_data;
};

/* SIS command packet structure */
struct ipr_cmd_pkt
{
    u16 reserved;               /* Reserved by IOA */
    u8 request_type;
#define IPR_RQTYPE_SCSICDB     0x00
#define IPR_RQTYPE_IOACMD      0x01
#define IPR_RQTYPE_HCAM        0x02

    u8 luntar_luntrn;

    u8 reserved3:4;
    u8 cmd_sync_override:1;
    u8 no_underlength_checking:1;
    u8 reserved2:1;
    u8 write_not_read:1;

    u8 reserved6:1;
    u8 task_attributes:3;
    u8 reserved4:4;

    u8 cdb[16];
    u16 cmd_timeout;
};

/**********************************************************/
/*                                                        */
/* B I G    E N D I A N                                   */
/*                                                        */
/**********************************************************/
#elif (defined(__KERNEL__) && defined(__BIG_ENDIAN)) || \
(!defined(__KERNEL__) && (__BYTE_ORDER == __BIG_ENDIAN))
#define htoipr16(x) (x)
#define htoipr32(x) (x)
#define iprtoh16(x) (x)
#define iprtoh32(x) (x)
#define _i16(x) (x)
#define _i32(x) (x)

#define IPR_BIG_ENDIAN       1
#define IPR_LITTLE_ENDIAN    0

struct ipr_mode_page_hdr
{
    u8 parms_saveable:1;
    u8 reserved1:1;
    u8 page_code:6;
    u8 page_length;
};

struct ipr_control_mode_page
{
    /* Mode page 0x0A */
    struct ipr_mode_page_hdr header;
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

struct ipr_reclaim_query_data
{
    u8 action_status;
#define IPR_ACTION_SUCCESSFUL               0
#define IPR_ACTION_NOT_REQUIRED             1
#define IPR_ACTION_NOT_PERFORMED            2
    u8 reclaim_known_needed:1;
    u8 reclaim_unknown_needed:1;
    u8 reserved2:2;
    u8 reclaim_known_performed:1;
    u8 reclaim_unknown_performed:1;
    u8 reserved3:1;
    u8 num_blocks_needs_multiplier:1;

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

/* IBM's IPR smart dump table structures */
struct ipr_sdt_entry
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

struct ipr_vset_res_state
{
    u16 stripe_size;
    u8 prot_level;
    u8 num_devices_in_vset;
    u32 reserved6;
};

struct ipr_dasd_res_state
{
    u32 data_path_width;  /* bits */
    u32 data_xfer_rate;   /* 100 KBytes/second */
};

struct ipr_query_res_state
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
        struct ipr_vset_res_state vset;
        struct ipr_dasd_res_state dasd;
    }dev;

    u32 ilid;
    u32 failing_dev_ioasc;
    struct ipr_res_addr failing_dev_res_addr;
    u32 failing_dev_res_handle;
    u8 protection_level_str[8];
};

struct ipr_array_cap_entry
{
    u8                          prot_level;
#define IPR_DEFAULT_RAID_LVL "5"
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

struct ipr_array_record
{
    struct ipr_record_common common;
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

struct ipr_array2_record
{
    struct ipr_record_common common;

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
    struct ipr_res_addr last_resource_address;
    u8  vendor_id[8];
    u8  product_id[16];
    u8  serial_number[8];
    u32 reserved;
};

struct ipr_resource_flags
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

struct ipr_device_record
{
    struct ipr_record_common common;
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
    struct ipr_resource_flags resource_flags_to_become;
    u32 user_area_size_to_become;  
};

/* 44 bytes */
struct ipr_std_inq_data{
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
    struct ipr_std_inq_vpids vpids;

    /* ROS and RAM levels */
    u8 ros_rsvd_ram_rsvd[4];

    /* Serial Number */
    u8 serial_num[IPR_SERIAL_NUM_LEN];
};

struct ipr_std_inq_data_long
{
    struct ipr_std_inq_data std_inq_data;
    u8 z1_term[IPR_STD_INQ_Z1_TERM_LEN];
    u8 reserved:4;
    u8 clocking:2;
    u8 qas:1;
    u8 ius:1;
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

struct ipr_mode_page_28_scsi_dev_bus_attr
{
    struct ipr_res_addr res_addr;
    u8 qas_capability:2;
#define IPR_MODEPAGE28_QAS_CAPABILITY_NO_CHANGE      0  
#define IPR_MODEPAGE28_QAS_CAPABILITY_DISABLE_ALL    1        
#define IPR_MODEPAGE28_QAS_CAPABILITY_ENABLE_ALL     2
/* NOTE:   Due to current operation conditions QAS should
 never be enabled so the change mask will be set to 0 */
#define IPR_MODEPAGE28_QAS_CAPABILITY_CHANGE_MASK    0

    u8 enable_target_mode:1;
    u8 term_power_absent:1;
    u8 target_mode_supported:1;
    u8 lvd_to_se_transition_not_allowed:1;
    u8 reserved2:2;

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

struct ipr_resource_entry
{
    u8 level;
    u8 array_id;

    u8 is_ioa_resource:1;
    u8 is_compressed:1;
    u8 is_array_member:1;
    u8 is_hot_spare:1;
    u8 is_af:1;
    u8 reserved2:1;
    u8 capacity_reduction_hi:2;

    u8 capacity_reduction_lo:1;
    u8 reserved3:3;
    u8 subtype:4;

    struct ipr_res_addr resource_address;
    u32 resource_handle;
    u32 reserved4;
    u32 reserved5;
    struct ipr_std_inq_data std_inq_data;
};

/* SIS command packet structure */
struct ipr_cmd_pkt
{
    u16 reserved;               /* Reserved by IOA */
    u8 request_type;
#define IPR_RQTYPE_SCSICDB     0x00
#define IPR_RQTYPE_IOACMD      0x01
#define IPR_RQTYPE_HCAM        0x02

    u8 luntar_luntrn;

    u8 write_not_read:1;
    u8 reserved2:1;
    u8 no_underlength_checking:1;
    u8 cmd_sync_override:1;
    u8 reserved3:4;

    u8 reserved4:4;
    u8 task_attributes:3;
    u8 reserved6:1;

    u8 cdb[16];
    u16 cmd_timeout;
};


#else
#error "Neither __LITTLE_ENDIAN nor __BIG_ENDIAN defined"
#endif

#define IPR_CONFIG_DIR "/etc/ipr/"
#define IPR_QAS_CAPABILITY "QAS_CAPABILITY"
#define IPR_HOST_SCSI_ID "HOST_SCSI_ID"
#define IPR_BUS_WIDTH "BUS_WIDTH"
#define IPR_MAX_XFER_RATE "MAX_BUS_SPEED"
#define IPR_MIN_TIME_DELAY "MIN_TIME_DELAY"
#define IPR_CATEGORY_BUS "Bus"
#define IPR_LIMITED_MAX_XFER_RATE 80
/* IPR_LIMITED_CONFIG is used to compensate for
 a set of devices which will not allow firmware
 udpates when operating at high max transfer rates.
 Selecting the LIMITED_CONFIG adjusts the max
 transfer rate allowing successfull firmware update
 operations to be performed */
#define IPR_LIMITED_CONFIG 0x7FFF
#define IPR_SAVE_LIMITED_CONFIG 0x8000
#define IPR_NORMAL_CONFIG 0

/* Internal return codes */
#define RC_SUCCESS          0
#define RC_FAILED          -1
#define RC_NOOP		   -2
#define RC_NOT_PERFORMED   -3
#define RC_IN_USE          -4
#define RC_CANCEL          12
#define RC_REFRESH          1
#define RC_EXIT             3

struct ipr_device{
    char                              dev_name[64];
    char                              gen_name[64];
    int                               opens;
    u32                               is_start_cand:1;
    u32                               is_reclaim_cand:1;
    u32                               reserved:30;
  u32 pci_bus_number;
  u32 pci_slot;
    struct ipr_resource_entry     *p_resource_entry;
    struct ipr_record_common      *p_qac_entry;
};

struct ipr_ioa {
    struct ipr_device                 ioa;
    u16                               ccin;
    u32                               host_num;
    u32                               host_addr;
    char                              serial_num[9];
    u32                               num_raid_cmds;
    char                              driver_version[50];
    char                              firmware_version[40];
    char                              part_num[20];
    u16                               num_devices;
    u8 rw_protected:1; /* FIXME */
    u8 ioa_dead:1;
    u8 nr_ioa_microcode:1;
    u16 host_no;
    char pci_address[16];
    struct ipr_device                *dev;
    struct ipr_resource_table     *p_resource_table;
    struct ipr_array_query_data   *p_qac_data;
    struct ipr_supported_arrays   *p_supported_arrays;
    struct ipr_reclaim_query_data *p_reclaim_data;
    struct ipr_ioa                   *p_next;
    struct ipr_ioa                   *p_cmd_next;
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

struct ipr_dasd_inquiry_page3
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

struct ipr_array_query_data
{
    u16 resp_len;
    u8  reserved;
    u8  num_records;
    u8 data[IPR_QAC_BUFFER_SIZE];
};

struct ipr_block_desc {
    u8 num_blocks[4];
    u8 density_code;
    u8 block_length[3];
};

struct ipr_mode_parm_hdr
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

struct ipr_driver_cfg {
	u16 debug_level;
	u16 debug_level_max;
}__attribute__((packed, aligned (2)));

struct ipr_dump_ioctl {
	u32 reserved;
	u32 buffer_len;
	u8 buffer[0];
}__attribute__((packed, aligned (4)));

struct ipr_bus_attributes {
	u8 bus;
	u8 qas_enabled;
	u8 bus_width;
	u8 reserved;
	u32 max_xfer_rate;
}__attribute__((packed, aligned (4)));

struct ipr_get_trace_ioctl {
	u32 reserved;
	u32 buffer_len;
	u8 buffer[0];
}__attribute__((packed, aligned (4)));

struct ipr_reclaim_cache_ioctl {
	u32 reserved;
	u32 ioasc;
	u32 buffer_len;
	u8 buffer[0];
}__attribute__((packed, aligned (4)));

struct ipr_query_config_ioctl {
	u32 reserved;
	u32 buffer_len;
	u8 buffer[0];
}__attribute__((packed, aligned (4)));

struct ipr_ucode_download_ioctl {
	u32 reserved;
	u32 ioasc;
	u32 buffer_len;
	u8 buffer[0];
}__attribute__((packed, aligned (4)));

struct ipr_passthru_ioctl {
	u32 timeout_in_sec;
	u32 ioasc;
	u32 res_handle;
	struct ipr_cmd_pkt cmd_pkt;
	u32 buffer_len;
	u8 buffer[0];
}__attribute__((packed, aligned (4)));

#define IPR_IOCTL_CODE 'i'

#define IPR_IOCTL_PASSTHRU _IOWR(IPR_IOCTL_CODE, 0x40, struct ipr_passthru_ioctl)
#define IPR_IOCTL_RUN_DIAGNOSTICS _IO(IPR_IOCTL_CODE 0x41)
#define IPR_IOCTL_DUMP_IOA _IOW(IPR_IOCTL_CODE, 0x42, struct ipr_dump_ioctl)
#define IPR_IOCTL_RESET_IOA _IO(IPR_IOCTL_CODE, 0x43)
#define IPR_IOCTL_READ_DRIVER_CFG _IOR(IPR_IOCTL_CODE, 0x44, struct ipr_driver_cfg)
#define IPR_IOCTL_WRITE_DRIVER_CFG _IOW(IPR_IOCTL_CODE, 0x45, struct ipr_driver_cfg)
#define IPR_IOCTL_GET_BUS_CAPABILITIES _IOWR(IPR_IOCTL_CODE, 0x46, struct ipr_bus_attributes)
#define IPR_IOCTL_SET_BUS_ATTRIBUTES _IOW(IPR_IOCTL_CODE, 0x47, struct ipr_bus_attributes)
#define IPR_IOCTL_GET_TRACE _IOW(IPR_IOCTL_CODE, 0x48, struct ipr_get_trace_ioctl)
#define IPR_IOCTL_RECLAIM_CACHE _IOWR(IPR_IOCTL_CODE, 0x49, struct ipr_reclaim_cache_ioctl)
#define IPR_IOCTL_QUERY_CONFIGURATION _IOW(IPR_IOCTL_CODE, 0x4A, struct ipr_query_config_ioctl)
#define IPR_IOCTL_UCODE_DOWNLOAD _IOWR(IPR_IOCTL_CODE, 0x4B,struct ipr_ucode_download_ioctl)
#define IPR_IOCTL_CHANGE_ADAPTER_ASSIGNMENT _IO(IPR_IOCTL_CODE,0x4C)

struct ipr_resource_hdr
{
    u8 num_entries;
    u8 reserved1;
    u16 reserved2;
};

struct ipr_resource_table
{
    struct ipr_resource_hdr   hdr;
    struct ipr_resource_entry dev[IPR_MAX_PHYSICAL_DEVS];
};

struct ipr_mode_page_28_header
{
    struct ipr_mode_page_hdr header;
    u8 num_dev_entries;
    u8 dev_entry_length;
};

struct ipr_page_28
{
    struct ipr_mode_page_28_header page_hdr;
    struct ipr_mode_page_28_scsi_dev_bus_attr attr[IPR_MAX_NUM_BUSES];
};

struct ipr_pagewh_28
{
    struct ipr_mode_parm_hdr parm_hdr;
    struct ipr_mode_page_28_header page_hdr;
    struct ipr_mode_page_28_scsi_dev_bus_attr attr[IPR_MAX_NUM_BUSES];
};

struct ipr_supported_arrays
{
    struct ipr_record_common common;
    u16                         num_entries;
    u16                         entry_length;
    u8                          data[0];
};

struct ipr_read_cap
{
    u32 max_user_lba;
    u32 block_length;
};

struct ipr_read_cap16
{
    u32 max_user_lba_hi;
    u32 max_user_lba_lo;
    u32 block_length;
};

/* Struct for disks that are unsupported or require a minimum microcode
 level prior to formatting to 522-byte sectors. */
struct unsupported_af_dasd
{
    char vendor_id[IPR_VENDOR_ID_LEN + 1];
    char compare_vendor_id_byte[IPR_VENDOR_ID_LEN];
    char product_id[IPR_PROD_ID_LEN + 1];
    char compare_product_id_byte[IPR_PROD_ID_LEN];
    struct ipr_std_inq_vpids vpid;   /* vpid - meaningful bits */
    struct ipr_std_inq_vpids vpid_mask;  /* mask don't cares in the vpid */
    char lid[5];       /* Load ID - Bytes 8-11 of Inquiry Page 3 */
    char lid_mask[4];  /* Mask for certain bytes of the LID */
    uint supported_with_min_ucode_level;
    char min_ucode_level[5];
    char min_ucode_mask[4];    /* used to mask don't cares in the ucode level */
};

struct ipr_cmd_status_record
{
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

struct ipr_cmd_status
{
    u16 resp_len;
    u8  reserved;
    u8  num_records;
    struct ipr_cmd_status_record record[100];
};

struct ipr_std_inq_vpids_sn
{
    struct ipr_std_inq_vpids vpids;
    u8 serial_num[IPR_SERIAL_NUM_LEN];
};

struct ipr_discard_cache_data
{
    u32 length;
    union { 
        struct ipr_std_inq_vpids_sn vpids_sn;
        u32 add_cmd_parms[10];
    }data;
};

struct ipr_software_inq_lid_info
{
    u32  load_id;
    u32  timestamp[3];
};

struct ipr_inquiry_page0  /* Supported Vital Product Data Pages */
{
    u8 peri_qual_dev_type;
    u8 page_code;
    u8 reserved1;
    u8 page_length;
    u8 supported_page_codes[IPR_MAX_NUM_SUPP_INQ_PAGES];
};

struct ipr_inquiry_page3
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

struct ipr_dasd_ucode_header
{
    u8 length[3];
    u8 load_id[4];
    u8 modification_level[4];
    u8 ptf_number[4];
    u8 patch_number[4];
};

struct ipr_inquiry_page_cx  /* Extended Software Inquiry  */
{
    u8 peri_qual_dev_type;
    u8 page_code;
    u8 reserved1;
    u8 page_length;
    u8 ascii_length;
    u8 reserved2[3];

    struct ipr_software_inq_lid_info lidinfo[15];
};

/* NOTE: The structure below is a shared structure with user-land tools */
/* We need to make sure we don't put pointers in here as the
 utilities could be running in 32 bit mode on a 64 bit kernel. */
struct ipr_ioctl_cmd_type2
{
    u32 type:8;  /* type is used to distinguish between ioctl_cmd structure formats */
#define IPR_IOCTL_TYPE_2     0x03
    u32 reserved:21;
    u32 read_not_write:1; /* data direction */
    u32 device_cmd:1; /* used to pass commands to specific devices identified by resource address */
    u32 driver_cmd:1; /* used exclusively to pass commands to the device driver, 0 otherwise */
    struct ipr_res_addr resource_address;
#define IPR_CDB_LEN          16
    u8 cdb[IPR_CDB_LEN];
    u32 buffer_len;
    u8 buffer[0];
};

/* The structures below are deprecated and should not be used. Use the
 structure above to send ioctls instead */
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

int get_proc_string(char *proc_file_name, char *label, char *buffer);
int scan_device(struct ipr_res_addr resource_addr, u32 host_num);
int remove_device(struct ipr_res_addr resource_addr, struct ipr_ioa *p_ioa);
int sg_ioctl(int fd, u8 cdb[IPR_CCB_CDB_LEN],
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
int ipr_config_file_valid(char *usr_file_name);
int  ipr_config_file_read(char *usr_file_name,
                          char *category,
                          char *field,
                          char *value);
void  ipr_config_file_entry(char *usr_file_name,
                            char *category,
                            char *field,
                            char *value,
                            int update);
void ipr_save_page_28(struct ipr_ioa *p_ioa,
                      struct ipr_page_28 *p_page_28_cur,
                      struct ipr_page_28 *p_page_28_chg,
                      struct ipr_page_28 *p_page_28_ipr);
void ipr_set_page_28(struct ipr_ioa *p_cur_ioa,
                     int limited_config,
                     int reset_scheduled);
void ipr_set_page_28_init(struct ipr_ioa *p_cur_ioa,
                          int limited_config);
void iprlog_location(struct ipr_ioa *p_ioa);
bool is_af_blocked(struct ipr_device *p_ipr_device, int silent);
int ipr_query_command_status(int fd, u32 res_handle, void *buff);
int ipr_mode_sense(int fd, u8 page, u32 res_handle, void *buff);
int ipr_mode_select(int fd, u32 res_handle, void *buff, int length);
int ipr_read_capacity_16(int fd, u32 res_handle, void *buff);
int ipr_read_capacity(int fd, u32 res_handle, void *buff);
int ipr_query_resource_state(int fd, u32 res_handle, void *buff);
int ipr_start_stop_stop(int fd, u32 res_handle);
int ipr_stop_array_protection(int fd, struct ipr_array_query_data *p_qac_data);
int ipr_remove_hot_spare(int fd, struct ipr_array_query_data *p_qac_data);
int ipr_start_array_protection(int fd, struct ipr_array_query_data *p_qac_data, int stripe_size, int prot_level);
int ipr_add_hot_spare(int fd, struct ipr_array_query_data *p_qac_data);
int ipr_rebuild_device_data(int fd, struct ipr_array_query_data *p_qac_data);
int ipr_format_unit(int fd, u32 res_handle);
int ipr_add_array_device(int fd, struct ipr_array_query_data *p_qac_data);
int ipr_reclaim_cache_store(int fd, int action, void *buff);
u32 ipr_max_xfer_rate(int fd, int bus);
int ipr_mode_sense_page_28(int fd, void *buff);
int ipr_mode_select_page_28(int fd, void *buff);
int ipr_evaluate_device(int fd, u32 res_handle);
int ipr_inquiry(int fd, u32 res_handle, u8 page, void *buff, u8 length);
int ipr_write_buffer(int fd, void *buff, u32 length);
int ipr_dev_write_buffer(int fd, u32 res_handle, void *buff, u32 length);

/*---------------------------------------------------------------------------
 * Purpose: Identify Advanced Function DASD present
 * Lock State: N/A
 * Returns: 0 if not AF DASD
 *          1 if AF DASD
 *---------------------------------------------------------------------------*/
static inline int ipr_is_af_dasd_device(struct ipr_resource_entry *p_resource_entry)
{
    if (IPR_IS_DASD_DEVICE(p_resource_entry->std_inq_data) &&
        (!p_resource_entry->is_ioa_resource) &&
        (p_resource_entry->subtype == IPR_SUBTYPE_AF_DASD))
        return 1;
    else
        return 0;
}

static inline int ipr_is_hidden(struct ipr_resource_entry *p_resource_entry)
{
    if (ipr_is_af_dasd_device(p_resource_entry) ||
        IPR_IS_SES_DEVICE(p_resource_entry->std_inq_data))
        return 1;
    else
        return 0;
}

static inline int ipr_is_af(struct ipr_resource_entry *p_resource_entry)
{
    if (IPR_IS_DASD_DEVICE(p_resource_entry->std_inq_data) &&
        (!p_resource_entry->is_ioa_resource) &&
        ((p_resource_entry->subtype == IPR_SUBTYPE_AF_DASD) ||
        (p_resource_entry->subtype == IPR_SUBTYPE_VOLUME_SET)))
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

#endif
