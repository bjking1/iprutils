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
 * $Header: /cvsroot/iprdd/iprutils/iprlib.c,v 1.127 2009/10/30 18:46:39 klebers Exp $
 */

#ifndef iprlib_h
#include "iprlib.h"
#endif

static void default_exit_func()
{
}

struct ipr_array_query_data *ipr_qac_data = NULL;
int num_ioas = 0;
struct ipr_ioa *ipr_ioa_head = NULL;
struct ipr_ioa *ipr_ioa_tail = NULL;
void (*exit_func) (void) = default_exit_func;
int daemonize = 0;
int ipr_debug = 0;
int ipr_force = 0;
int ipr_sg_required = 0;
int polling_mode = 0;
int ipr_fast = 0;
int format_done = 0;
static int ipr_mode5_write_buffer = 0;
static int first_time_check_zeroed_dev = 0;
int tool_init_retry = 1;

struct sysfs_dev *head_zdev = NULL;
struct sysfs_dev *tail_zdev = NULL;

static int ipr_force_polling = 0;
static int ipr_force_uevents = 0;
static char *hotplug_dir = NULL;
static struct scsi_dev_data *scsi_dev_table = NULL;

/* Current state of this machine. */
enum system_p_mode power_cur_mode = POWER_BAREMETAL;

/* This table includes both unsupported 522 disks and disks that support 
 being formatted to 522, but require a minimum microcode level. The disks
 that require a minimum level of microcode will be marked by the 
 supported_with_min_ucode_level flag set to true. */
struct unsupported_af_dasd unsupported_af[] =
{
	{
		/* 00 Mako & Hammerhead */
		vendor_id: {"IBM     "},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"DRVS            "},
		compare_product_id_byte: {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		lid: {"    "},
		lid_mask: {0, 0, 0, 0},
		supported_with_min_ucode_level: false,
		min_ucode_level: {"    "},
		min_ucode_mask: {0, 0, 0, 0}
	},
	{
		/* 01 Swordfish */
		vendor_id: {"IBM     "},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"DRHS            "},
		compare_product_id_byte: {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		lid: {"    "},
		lid_mask: {0, 0, 0, 0},
		supported_with_min_ucode_level: false,
		min_ucode_level: {"    "},
		min_ucode_mask: {0, 0, 0, 0}
	},
	{
		/* 02 Neptune */
		vendor_id: {"IBM     "},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"DNES            "},
		compare_product_id_byte: {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		lid: {"    "},
		lid_mask: {0, 0, 0, 0},
		supported_with_min_ucode_level: false,
		min_ucode_level: {"    "},
		min_ucode_mask: {0, 0, 0, 0}
	},
	{
		/* 03 Thornback, Stingray, & Manta */
		vendor_id: {"IBM     "},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"DMVS            "},
		compare_product_id_byte: {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		lid: {"    "},
		lid_mask: {0, 0, 0, 0},
		supported_with_min_ucode_level: false,
		min_ucode_level: {"    "},
		min_ucode_mask: {0, 0, 0, 0}
	},
	{
		/* 04 Discovery I */
		vendor_id: {"IBM     "},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"DDYS            "},
		compare_product_id_byte: {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		lid: {"    "},
		lid_mask: {0, 0, 0, 0},
		supported_with_min_ucode_level: false,
		min_ucode_level: {"    "},
		min_ucode_mask: {0, 0, 0, 0}
	},
	{
		/* 05 US73 (Discovery II) - fixme - are we supporting this drive? */
		vendor_id: {"IBM     "},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"IC35L     D2    "},
		compare_product_id_byte: {1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
		lid: {"    "},
		lid_mask: {0, 0, 0, 0},
		supported_with_min_ucode_level: true,
		min_ucode_level: {"S5DE"},
		min_ucode_mask: {1, 1, 1, 1}
	},
	{
		/* 06 Apollo (Ext. Scallop 9GB) - LID: SC09 */
		vendor_id: {"IBM     "},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"ST318305L       "},
		compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
		lid: {"SC09"},
		lid_mask: {1, 1, 1, 1},
		supported_with_min_ucode_level: false,
		min_ucode_level: {"C749"},
		min_ucode_mask: {1, 1, 1, 1}
	},
	{
		/* 07 Apollo (Ext. Scallop 18GB) - LID: SC18 */
		vendor_id: {"IBM     "},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"ST318305L       "},
		compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
		lid: {"SC18"},
		lid_mask: {1, 1, 1, 1},
		supported_with_min_ucode_level: false,
		min_ucode_level: {"C709"},
		min_ucode_mask: {1, 1, 1, 1}
	},
	{
		/* 08 Apollo (Ext. Scallop 36GB) - LID: SC36 */
		vendor_id: {"IBM     "},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"ST336605L       "},
		compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
		lid: {"SC36"},
		lid_mask: {1, 1, 1, 1},
		supported_with_min_ucode_level: false,
		min_ucode_level: {"C709"},
		min_ucode_mask: {1, 1, 1, 1}
	},
	{
		/* 09 Apollo (Ext. Scallop 73GB) - LID: SC73 */
		vendor_id: {"IBM     "},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"ST373405L       "},
		compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
		lid: {"SC73"},
		lid_mask: {1, 1, 1, 1},
		supported_with_min_ucode_level: false,
		min_ucode_level: {"C709"},
		min_ucode_mask: {1, 1, 1, 1}
	},
	{
		/* 10 Apollo (non-external 9GB) - LID: AT1x */
		vendor_id: {"IBM     "},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"ST318305L       "},
		compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
		lid: {"AT1 "},
		lid_mask: {1, 1, 1, 0},
		supported_with_min_ucode_level: true,
		min_ucode_level: {"C54E"},
		min_ucode_mask: {1, 1, 1, 1}
	},
	{
		/* 11 Apollo (non-external 18GB) - LID: AT0x */
		vendor_id: {"IBM     "},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"ST318305L       "},
		compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
		lid: {"AT0 "},
		lid_mask: {1, 1, 1, 0},
		supported_with_min_ucode_level: true,
		min_ucode_level: {"C50E"},
		min_ucode_mask: {1, 1, 1, 1}
	},
	{
		/* 12 Apollo (non-external 36GB) - LID: AT0x */
		vendor_id: {"IBM     "},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"ST336605L       "},
		compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
		lid: {"AT0 "},
		lid_mask: {1, 1, 1, 0},
		supported_with_min_ucode_level: true,
		min_ucode_level: {"C50E"},
		min_ucode_mask: {1, 1, 1, 1}
	},
	{
		/* 13 Apollo (non-external 73GB) - LID: AT0x */
		vendor_id: {"IBM     "},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"ST373405L       "},
		compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
		lid: {"AT0 "},
		lid_mask: {1, 1, 1, 0},
		supported_with_min_ucode_level: true,
		min_ucode_level: {"C50E"},
		min_ucode_mask: {1, 1, 1, 1}
	},
	{
		/* 14 Odyssey (18, 36, 73GB) */
		vendor_id: {"IBM     "},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"ST3   53L       "},
		compare_product_id_byte: {1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
		lid: {"    "},
		lid_mask: {0, 0, 0, 0},
		supported_with_min_ucode_level: true,
		min_ucode_level: {"C51A"},
		min_ucode_mask: {1, 1, 1, 1}
	},
	{
		/* 15 Odyssey (146+GB) - fixme - not in drive list; remove from table? */
		vendor_id: {"IBM     "},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"ST3    53L      "},
		compare_product_id_byte: {1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
		lid: {"    "},
		lid_mask: {0, 0, 0, 0},
		supported_with_min_ucode_level: true,
		min_ucode_level: {"C51A"},
		min_ucode_mask: {1, 1, 1, 1}
	},
	{
		/* 16 Gemini (18, 36, 73GB) */
		vendor_id: {"IBM     "},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"ST3   07L       "},
		compare_product_id_byte: {1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
		lid: {"    "},
		lid_mask: {0, 0, 0, 0},
		supported_with_min_ucode_level: true,
		min_ucode_level: {"C50F"},
		min_ucode_mask: {1, 1, 1, 1}
	},
	{
		/* 17 Gemini (146+GB) */
		vendor_id: {"IBM     "},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"ST3    07L      "},
		compare_product_id_byte: {1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
		lid: {"    "},
		lid_mask: {0, 0, 0, 0},
		supported_with_min_ucode_level: true,
		min_ucode_level: {"C50F"},
		min_ucode_mask: {1, 1, 1, 1}
	},
	{
		/* 18 Daytona */
		vendor_id: {"IBM     "},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"IC35L     DY    "},
		compare_product_id_byte: {1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
		lid: {"    "},
		lid_mask: {0, 0, 0, 0},
		supported_with_min_ucode_level: true,
		min_ucode_level: {"S28C"},
		min_ucode_mask: {1, 1, 1, 1}
	}
};

struct unsupported_dasd unsupported_dasd [] = {
	{	/* Piranha 18 GB, 15k RPM 160 MB/s - UCPR018 - 4322 */
		vendor_id: {"IBMAS400"},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"UCPR018         "},
		compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	},
	{	/* Piranha 18 GB, 15k RPM 320 MB/s - XCPR018 - 4325 */
		vendor_id: {"IBMAS400"},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"XCPR018         "},
		compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	}, 
	{	/* Piranha 35 GB, 15k RPM 160 MB/s - UCPR036 - 4323 */
		vendor_id: {"IBMAS400"},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"UCPR036         "},
		compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	}, 
	{	/* Piranha 35 GB, 15k RPM 320 MB/s - XCPR036 - 4326 */
		vendor_id: {"IBMAS400"},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"XCPR036         "},
		compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	},
	{	/* Monza 70 GB, 15k RPM 320 MB/s - XCPR073 - 4327 */
		vendor_id: {"IBMAS400"},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"XCPR073         "},
		compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	},
	{	/* Monza 141 GB, 15k RPM 320 MB/s - XCPR146 - 4328 */
		vendor_id: {"IBMAS400"},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"XCPR146         "},
		compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	},
	{	/* Monaco 282 GB, 15k RPM 320 MB/s - XCPR282 - 4329 */
		vendor_id: {"IBMAS400"},
		compare_vendor_id_byte: {1, 1, 1, 1, 1, 1, 1, 1},
		product_id: {"XCPR282         "},
		compare_product_id_byte: {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	}
};

struct ses_table_entry {
	char product_id[17];
	char compare_product_id_byte[17];
	u32 max_bus_speed_limit;
	int block_15k_devices;
};

static const struct ses_table_entry ses_table[] = {
	{"2104-DL1        ", "XXXXXXXXXXXXXXXX", 80, 0},
	{"2104-TL1        ", "XXXXXXXXXXXXXXXX", 80, 0},
	{"HSBP07M P U2SCSI", "XXXXXXXXXXXXXXXX", 80, 1},
	{"HSBP05M P U2SCSI", "XXXXXXXXXXXXXXXX", 80, 1},
	{"HSBP05M S U2SCSI", "XXXXXXXXXXXXXXXX", 80, 1},
	{"HSBP06E ASU2SCSI", "XXXXXXXXXXXXXXXX", 80, 1},
	{"2104-DU3        ", "XXXXXXXXXXXXXXXX", 160, 0},
	{"2104-TU3        ", "XXXXXXXXXXXXXXXX", 160, 0},
	{"HSBP04C RSU2SCSI", "XXXXXXX*XXXXXXXX", 160, 0},
	{"HSBP06E RSU2SCSI", "XXXXXXX*XXXXXXXX", 160, 0},
	{"St  V1S2        ", "XXXXXXXXXXXXXXXX", 160, 0},
	{"HSBPD4M  PU3SCSI", "XXXXXXX*XXXXXXXX", 160, 0},
	{"VSBPD1H   U3SCSI", "XXXXXXX*XXXXXXXX", 160, 0}
};


/*---------- static subroutine/function declearation starts here --------*/

static int sg_ioctl_by_name(char *, u8 *, void *, u32, u32,
			    struct sense_data_t *, u32);

/*---------- subroutine/function code starts here ---------*/

/**
 * ses_table_entry - 
 * @ioa:	ipr ioa struct
 * @bus:	
 *
 * Returns:
 *   ses_table_entry pointer if success / NULL on failure
 **/
static const struct ses_table_entry *get_ses_entry(struct ipr_ioa *ioa, int bus)
{
	int i, j, matches;
	struct ipr_dev *dev = ioa->dev;
	const struct ses_table_entry *ste = ses_table;

	for (i = 0; i < ioa->num_devices; i++, dev++) {
		if (!dev->scsi_dev_data)
			continue;
		if (dev->scsi_dev_data->channel == bus &&
		    dev->scsi_dev_data->type == TYPE_ENCLOSURE)
			break;
	}

	if (i == ioa->num_devices)
		return NULL;
	if (strncmp(dev->scsi_dev_data->vendor_id, "IBM", 3))
		return NULL;

	for (i = 0; i < ARRAY_SIZE(ses_table); i++, ste++) {
		for (j = 0, matches = 0; j < IPR_PROD_ID_LEN; j++) {
			if (ste->compare_product_id_byte[j] == 'X') {
				if (dev->scsi_dev_data->product_id[j] == ste->product_id[j])
					matches++;
				else
					break;
			} else
				matches++;
		}

		if (matches == IPR_PROD_ID_LEN)
			return ste;
	}

	return NULL;
}

/**
 * get_cap_entry - Returns a pointer to a capability entry that corresponds to
 * 		   the given raid level.
 * @supported_arrays:	ipr_supported_arrays struct
 * @raid_level:		character string representation of the raid level
 *
 * Returns:
 *   ipr_array_cap_entry pointer if success / NULL on failure
 **/
struct ipr_array_cap_entry *
get_cap_entry(struct ipr_supported_arrays *supported_arrays, char *raid_level)
{
	struct ipr_array_cap_entry *cap;

	for_each_cap_entry(cap, supported_arrays) {
		if (!strcmp((char *)cap->prot_level_str, raid_level))
			return cap;
	}

	return NULL;
}

/**
 * get_raid_cap_entry - Returns a pointer to a capability entry that corresponds
 * 			to the given protection level.
 * @supported_arrays:	ipr_supported_arrays struct
 * @prot_level:		protection level
 *
 * Returns:
 *   ipr_array_cap_entry pointer if success / NULL on failure
 **/
struct ipr_array_cap_entry *
get_raid_cap_entry(struct ipr_supported_arrays *supported_arrays, u8 prot_level)
{
	struct ipr_array_cap_entry *cap;

	if (!supported_arrays)
		return NULL;

	for_each_cap_entry(cap, supported_arrays) {
		if (cap->prot_level == prot_level)
			return cap;
	}

	return NULL;
}

/**
 * get_prot_level_str - Returns the string representation of the protection
 * 			level the given the numeric protection level.
 * @supported_arrays:	ipr_supported_arrays struct
 * @prot_level:		protection level
 *
 * Returns:
 *   protection level string if success / NULL on failure
 **/
char *get_prot_level_str(struct ipr_supported_arrays *supported_arrays,
			 int prot_level)
{
	struct ipr_array_cap_entry *cap;

	cap = get_raid_cap_entry(supported_arrays, prot_level);

	if (cap)
		return (char *)cap->prot_level_str;

	return NULL;
}

/**
 * get_vset_from_array - Given an array, return the corresponding
 *			 volume set device.
 * @ioa:		ipr ioa struct
 * @array:		ipr dev struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
struct ipr_dev *get_vset_from_array(struct ipr_ioa *ioa, struct ipr_dev *array)
{
	struct ipr_dev *vset;

	for_each_vset(ioa, vset)
		if (vset->array_id == array->array_id)
			return vset;

	return array;
}

/**
 * get_array_from_vset - Given a volume set, return the corresponding
 *			 array device.
 * @ioa:		ipr ioa struct
 * @vset:		ipr dev struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
struct ipr_dev *get_array_from_vset(struct ipr_ioa *ioa, struct ipr_dev *vset)
{
	struct ipr_dev *array;

	for_each_array(ioa, array)
		if (array->array_id == vset->array_id)
			return array;

	return vset;
}

static ssize_t sysfs_read_attr(const char *devpath, const char *attr,
			       void *value, size_t len)
{
	char attrpath[256];
	ssize_t ret;
	int fd;

	sprintf(attrpath, "%s/%s", devpath, attr);
	fd = open(attrpath, O_RDONLY);
	if (fd < 0)
		return -1;

	ret = read(fd, value, len);
	close(fd);
	if (ret < 0)
		return -1;

	return ret;
}

static ssize_t sysfs_write_attr(const char *devpath, const char *attr,
				void *value, size_t len)
{
	char attrpath[256];
	ssize_t ret;
	int fd;

	sprintf(attrpath, "%s/%s", devpath, attr);
	fd = open(attrpath, O_WRONLY);
	if (fd < 0)
		return -1;

	ret = write(fd, value, len);
	close(fd);
	if (ret < 0)
		return -1;

	return ret;
}

/**
 * ipr_find_sysfs_dev - 
 * @dev:		ipr dev struct
 * @head:		sysfs dev struct
 *
 * Returns:
 *    sysfs_dev pointer if success / NULL on failure
 **/
struct sysfs_dev * ipr_find_sysfs_dev(struct ipr_dev *dev, struct sysfs_dev *head)
{
	struct sysfs_dev *sdev;

	if (!dev->scsi_dev_data)
		return NULL;

	for (sdev = head; sdev; sdev = sdev->next) {
		if (sdev->device_id != dev->scsi_dev_data->device_id)
			continue;
		if (!memcmp(sdev->ioa_pci_addr, dev->ioa->pci_address, sizeof(sdev->ioa_pci_addr)))
			break;

	}

	return sdev;
}

/**
 * ipr_sysfs_dev_to_dev - 
 * @sdev:		sysfs dev struct
 *
 * Returns:
 *    ipr_dev pointer if success / NULL on failure
 **/
struct ipr_dev *ipr_sysfs_dev_to_dev(struct sysfs_dev *sdev)
{
	struct ipr_dev *dev;
	struct ipr_ioa *ioa;

	for_each_ioa(ioa) {
		if (memcmp(sdev->ioa_pci_addr, ioa->pci_address, sizeof(sdev->ioa_pci_addr)))
			continue;

		for_each_dev(ioa, dev) {
			if (!dev->scsi_dev_data)
				continue;
			if (sdev->device_id == dev->scsi_dev_data->device_id)
				return dev;
		}
	}

	return NULL;
}

/**
 * ipr_find_zeroed_dev -
 * @dev:		ipr dev struct
 *
 * Returns:
 *    results of call to ipr_find_sysfs_dev
 **/
struct sysfs_dev * ipr_find_zeroed_dev(struct ipr_dev *dev)
{
	return ipr_find_sysfs_dev(dev, head_zdev);
}

/**
 * ipr_device_is_zeroed -
 * @dev:		ipr dev struct
 *
 * Returns:
 *    1 if the given devices is zeroed / 0 if the device is not zeroed
 **/
int ipr_device_is_zeroed(struct ipr_dev *dev)
{
	if (ipr_find_zeroed_dev(dev))
		return 1;
	return 0;
}

/**
 * ipr_add_sysfs_dev - 
 * @dev:		ipr dev struct
 * @head:		sysfs dev struct
 * @tail:		sysfs dev struct
 *
 * Returns:
 *    nothing
 **/
void ipr_add_sysfs_dev(struct ipr_dev *dev, struct sysfs_dev **head,
		       struct sysfs_dev **tail)
{
	struct sysfs_dev *sdev = ipr_find_sysfs_dev(dev, *head);

	if (!dev->scsi_dev_data)
		return;

	if (!sdev) {
		sdev = calloc(1, sizeof(struct sysfs_dev));
		sdev->device_id = dev->scsi_dev_data->device_id;
		memcpy(sdev->ioa_pci_addr, dev->ioa->pci_address, sizeof(sdev->ioa_pci_addr));

		if (!(*head)) {
			*tail = *head = sdev;
		} else {
			(*tail)->next = sdev;
			sdev->prev = *tail;
			*tail = sdev;
		}
	}
}

/**
 * ipr_add_zeroed_dev - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *    nothing
 **/
void ipr_add_zeroed_dev(struct ipr_dev *dev)
{
	ipr_add_sysfs_dev(dev, &head_zdev, &tail_zdev);
}

/**
 * ipr_del_sysfs_dev - 
 * @dev:		ipr dev struct
 * @head:		sysfs dev struct
 * @tail:		sysfs dev struct
 *
 * Returns:
 *    nothing
 **/
void ipr_del_sysfs_dev(struct ipr_dev *dev, struct sysfs_dev **head,
		       struct sysfs_dev **tail)
{
	struct sysfs_dev *sdev = ipr_find_sysfs_dev(dev, *head);

	if (!sdev || !dev->scsi_dev_data)
		return;

	if (sdev == *head) {
		*head = (*head)->next;

		if (!(*head))
			*tail = NULL;
		else
			(*head)->prev = NULL;
	} else if (sdev == *tail) {
		*tail = (*tail)->prev;
		(*tail)->next = NULL;
	} else {
		sdev->next->prev = sdev->prev;
		sdev->prev->next = sdev->next;
	}
}

/**
 * ipr_del_zeroed_dev - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *    nothing
 **/
void ipr_del_zeroed_dev(struct ipr_dev *dev)
{
	ipr_del_sysfs_dev(dev, &head_zdev, &tail_zdev);
}

/**
 * ipr_update_qac_with_zeroed_devs
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *    nothing
 **/
void ipr_update_qac_with_zeroed_devs(struct ipr_ioa *ioa)
{
	struct sysfs_dev *zdev;
	struct ipr_dev_record *dev_rcd;
	struct ipr_mode_pages mode_pages;
	struct ipr_ioa_mode_page *page;
	int i;

	if (!ioa->qac_data)
		return;

	for (i = 0; i < ioa->num_devices; i++) {
		zdev = ipr_find_zeroed_dev(&ioa->dev[i]);
		if (!zdev && ipr_is_af_dasd_device(&ioa->dev[i])) {
			memset(&mode_pages, 0, sizeof(mode_pages));
			ipr_mode_sense(&ioa->dev[i], 0x20, &mode_pages);

			page = (struct ipr_ioa_mode_page *) (((u8 *)&mode_pages) +
				     mode_pages.hdr.block_desc_len +
				     sizeof(mode_pages.hdr));

			if (page->format_completed) {
				dev_rcd = (struct ipr_dev_record *)ioa->dev[i].qac_entry;
				dev_rcd->known_zeroed = 1;
			}
		}
		else if (zdev && ioa->dev[i].qac_entry) {
			dev_rcd = (struct ipr_dev_record *)ioa->dev[i].qac_entry;
			dev_rcd->known_zeroed = 1;
		}
	}
}

void set_devs_format_completed()
{
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;

	if (format_done) {
		for_each_ioa(ioa) {
			for_each_dev(ioa, dev) {
			if (ipr_is_af_dasd_device(dev) && ipr_device_is_zeroed(dev))
				ipr_set_format_completed_bit(dev);
			}
		}
		format_done = 0;
	}
}

/**
 * ipr_cleanup_zeroed_devs -
 *
 * Returns:
 *    nothing
 **/
void ipr_cleanup_zeroed_devs()
{
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	struct sysfs_dev *zdev;

	for_each_ioa(ioa) {
		for_each_dev(ioa, dev) {
			zdev = ipr_find_zeroed_dev(dev);
			if (!zdev)
				continue;

			if (dev->scsi_dev_data && dev->scsi_dev_data->type == TYPE_DISK)
				ipr_del_zeroed_dev(dev);
			else if (ipr_is_array_member(dev))
				ipr_del_zeroed_dev(dev);
			else if (ipr_is_hot_spare(dev))
				ipr_del_zeroed_dev(dev);
		}
	}
}

/**
 * get_max_bus_speed - 
 * @ioa:		ipr ioa struct
 * @bus:		bus number
 *
 * Returns:
 *   maximum bus speed value
 **/
int get_max_bus_speed(struct ipr_ioa *ioa, int bus)
{
	const struct ses_table_entry *ste = get_ses_entry(ioa, bus);
	u32 fw_version;
	int max_xfer_rate = IPR_MAX_XFER_RATE;
	char devpath[PATH_MAX];
	char value[16];
	ssize_t len;

	sprintf(devpath, "/sys/class/scsi_host/%s", ioa->host_name);
	len = sysfs_read_attr(devpath, "fw_version", value, 16);
	if (len < 0)
		return -1;
	sscanf(value, "%8X", &fw_version);

	if (ipr_force)
		return IPR_MAX_XFER_RATE;

	if (fw_version < ioa->msl)
		max_xfer_rate = IPR_SAFE_XFER_RATE;

	if (ste && ste->max_bus_speed_limit < max_xfer_rate)
		return ste->max_bus_speed_limit;

	return max_xfer_rate;
}

struct ioa_parms {
	int ccin;
	int scsi_id_changeable;
	u32 msl;
	char *fw_name;
};

static const struct ioa_parms ioa_parms [] = {
	{.ccin = 0x5702, .scsi_id_changeable = 1,
	.msl = 0x02080029, .fw_name = "44415254" },
	{.ccin = 0x1974, .scsi_id_changeable = 1,
	.msl = 0x02080037, .fw_name = "44415254" },
	{.ccin = 0x5703, .scsi_id_changeable = 1,
	.msl = 0x03090059, .fw_name = "5052414D" },
	{.ccin = 0x1975, .scsi_id_changeable = 1,
	.msl = 0x03090068, .fw_name = "5052414D" },
	{.ccin = 0x5709, .scsi_id_changeable = 0,
	.msl = 0x030D0047, .fw_name = "5052414E" },
	{.ccin = 0x1976, .scsi_id_changeable = 0,
	.msl = 0x030D0056, .fw_name = "5052414E" },
	{.ccin = 0x570A, .scsi_id_changeable = 0,
	.msl = 0x020A004E, .fw_name = "44415255" },
	{.ccin = 0x570B, .scsi_id_changeable = 0,
	.msl = 0x020A004E, .fw_name = "44415255" },
	{.ccin = 0x2780, .scsi_id_changeable = 0,
	.msl = 0x030E0040, .fw_name = "5349530E" },
};

struct chip_details {
	u16 vendor;
	u16 device;
	const char *desc;
};

static const struct chip_details chip_details []= {
	{ .vendor = 0x1069, .device = 0xB166, "PCI-X" }, /* Gemstone */
	{ .vendor = 0x1014, .device = 0x028C, "PCI-X" }, /* Citrine */
	{ .vendor = 0x1014, .device = 0x0180, "PCI-X" }, /* Snipe */
	{ .vendor = 0x1014, .device = 0x02BD, "PCI-X" }, /* Obsidian */
	{ .vendor = 0x1014, .device = 0x0339, "PCI-E" }, /* Obsidian-E */
	{ .vendor = 0x9005, .device = 0x0503, "PCI-X" }, /* Scamp */
	{ .vendor = 0x1014, .device = 0x033D, "PCI-E" }, /* CRoC-FPGA */
	{ .vendor = 0x1014, .device = 0x034A, "PCI-E" }, /* CRoCodile */
};

/**
 * get_chip_details - 
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   chip_details struct pointer if success / NULL on failure
 **/
static const struct chip_details *get_chip_details(struct ipr_ioa *ioa)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(chip_details); i++) {
		if (ioa->pci_vendor == chip_details[i].vendor &&
		    ioa->pci_device == chip_details[i].device)
			return &chip_details[i];
	}

	return NULL;
}

struct ioa_details {
	u16 subsystem_vendor;
	u16 subsystem_device;
	const char *ioa_desc;
	int is_spi:1;
};

static const struct ioa_details ioa_details [] = {
	{.subsystem_vendor = 0x1014,
	.subsystem_device = 0x028D,
	"PCI-X SAS RAID Adapter", .is_spi = 0}
};

/**
 * get_ioa_details - 
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   ioa_details struct pointer if success / NULL on failure
 **/
static const struct ioa_details *get_ioa_details(struct ipr_ioa *ioa)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ioa_details); i++) {
		if (ioa->subsystem_vendor == ioa_details[i].subsystem_vendor &&
		    ioa->subsystem_device == ioa_details[i].subsystem_device)
			return &ioa_details[i];
	}

	return NULL;
}

bool ipr_is_af_blk_size(struct ipr_ioa *ioa, int blk_sz)
{

	if (blk_sz == ioa->af_block_size ||
			(ioa->support_4k && blk_sz == IPR_AF_4K_BLOCK_SIZE))
		return true;
	else
		return false;
}

/**
 * ipr_improper_device_type -
 * @dev:		ipr dev struct
 *
 * Returns:
 *   1 if the device is improper / 0 if the device is not improper
 **/
int ipr_improper_device_type(struct ipr_dev *dev)
{
	if (dev->rescan)
		return 1;
	if (!dev->scsi_dev_data)
		return 0;
	if (!dev->ioa->qac_data || !dev->ioa->qac_data->num_records)
		return 0;
	if (dev->scsi_dev_data->type == IPR_TYPE_AF_DISK && !dev->qac_entry &&
	    (ipr_get_blk_size(dev) == IPR_JBOD_BLOCK_SIZE ||
			(dev->ioa->support_4k && ipr_get_blk_size(dev) == IPR_JBOD_4K_BLOCK_SIZE)))
		return 1;
	if (dev->scsi_dev_data->type == TYPE_DISK && dev->qac_entry && ipr_is_af_blk_size(dev->ioa, ipr_get_blk_size(dev)))
		return 1;
	return 0;
}

/**
 * mode_sense - 
 * @dev:		ipr dev struct
 * @page:		page number
 * @buff:		buffer
 * @sense_data		sense_data_t struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int mode_sense(struct ipr_dev *dev, u8 page, void *buff,
		      struct sense_data_t *sense_data)
{
	u8 cdb[IPR_CCB_CDB_LEN];
	int fd, rc;
	u8 length = IPR_MODE_SENSE_LENGTH; /* xxx FIXME? */
	char *name = dev->gen_name;

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		scsi_dbg(dev, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = MODE_SENSE;
	cdb[2] = page;
	cdb[4] = length;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_FROM_DEV,
		      sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	close(fd);
	return rc;
}

/**
 * ipr_tcq_mode -
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   the tcq mode type
 **/
static enum ipr_tcq_mode ioa_get_tcq_mode(struct ipr_ioa *ioa)
{
	struct ipr_mode_pages mode_pages;
	struct sense_data_t sense_data;
	int rc;

	rc = mode_sense(&ioa->ioa, 0x28, &mode_pages, &sense_data);

	if (rc && sense_data.sense_key == ILLEGAL_REQUEST)
		return IPR_TCQ_NACA;
	if (rc)
		return IPR_TCQ_DISABLE;
	return IPR_TCQ_FROZEN;
}

/**
 * __ioa_is_spi - Determine if the IOA is a SCSI Parallel Interface (SPI)
 *		  adapter.
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   1 if the IOA is an SPI adapter / 0 otherwise
 **/
int __ioa_is_spi(struct ipr_ioa *ioa)
{
	struct ipr_mode_pages mode_pages;
	struct sense_data_t sense_data;
	int rc;

	rc = mode_sense(&ioa->ioa, 0x28, &mode_pages, &sense_data);

	if (rc && sense_data.sense_key == ILLEGAL_REQUEST)
		return 0;
	return 1;
}

/**
 * ioa_is_spi - Determine if the IOA is a SCSI Parallel Interface (SPI) adapter.
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   1 if the IOA is an SPI adapter / 0 otherwise
 **/
int ioa_is_spi(struct ipr_ioa *ioa)
{
	const struct ioa_details *details = get_ioa_details(ioa);

	if (details)
		return details->is_spi;
	return __ioa_is_spi(ioa);
}

static const char *raid_desc = "SCSI RAID Adapter";
static const char *jbod_desc = "SCSI Adapter";
static const char *sas_raid_desc = "SAS RAID Adapter";
static const char *sas_jbod_desc = "SAS Adapter";
static const char *aux_cache_desc = "Aux Cache Adapter";
static const char *def_chip_desc = "PCI";

/**
 * get_bus_desc -
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   FIXME
 **/
const char *get_bus_desc(struct ipr_ioa *ioa)
{
	const struct chip_details *chip = get_chip_details(ioa);

	if (!chip)
		return def_chip_desc;
	return chip->desc;
}

/**
 * get_ioa_desc -
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   FIXME
 **/
const char *get_ioa_desc(struct ipr_ioa *ioa)
{
	const struct ioa_details *details = get_ioa_details(ioa);

	if (details)
		return details->ioa_desc;
	else if (ioa->is_aux_cache)
		return aux_cache_desc;
	else if (!ioa_is_spi(ioa)) {
		if (ioa->qac_data->num_records)
			return sas_raid_desc;
		return sas_jbod_desc;
	} else if (ioa->qac_data->num_records)
		return raid_desc;

	return jbod_desc;
}

/**
 * get_ioa_fw - 
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   pointer to ioa_parms entry / NULL on failure
 **/
static const struct ioa_parms *get_ioa_fw(struct ipr_ioa *ioa)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ioa_parms); i++) {
		if (ioa->ccin == ioa_parms[i].ccin)
			return &ioa_parms[i];
	}

	return NULL;
}

/**
 * setup_ioa_parms
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   nothing
 **/
static void setup_ioa_parms(struct ipr_ioa *ioa)
{
	const struct ioa_parms *ioa_parms = get_ioa_fw(ioa);

	ioa->scsi_id_changeable = 1;

	if (ioa_parms) {
		ioa->scsi_id_changeable = ioa_parms->scsi_id_changeable;
		ioa->msl = ioa_parms->msl;
	}
}

/**
 * find_ioa - 
 * @host_no:		host number int
 *
 * Returns:
 *   pointer to ipr_ioa /  NULL on failure
 **/
struct ipr_ioa *find_ioa(int host_no)
{
	struct ipr_ioa *ioa;

	for_each_ioa(ioa)
		if (ioa->host_num == host_no)
			return ioa;
	return NULL;
}

/**
 * _find_dev - 
 * @blk:		ipr ioa struct
 * @compare:	        pointer to a comparison function
 *
 * Returns:
 *   ipr_dev pointer if success / NULL on failure
 **/
static struct ipr_dev *_find_dev(char *blk, int (*compare) (struct ipr_dev *, char *))
{
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;
	char *name = malloc(strlen(_PATH_DEV) + strlen(blk) + 1);

	if (!name)
		return NULL;

	if (strncmp(blk, _PATH_DEV, strlen(_PATH_DEV)))
		sprintf(name, _PATH_DEV"%s", blk);
	else
		sprintf(name, "%s", blk);

	for_each_ioa(ioa) {
		if (!compare(&ioa->ioa, name)) {
			free(name);
			return &ioa->ioa;
		}

		for_each_dev(ioa, dev) {
			if (!compare(dev, name)) {
				free(name);
				return dev;
			}
		}
	}

	free(name);
	return NULL;
}

/**
 * blk_compare - 
 * @dev:		ipr ioa struct
 * @name:		character string to compare
 *
 * Returns:
 *   integer result of the strcmp()
 **/
static int blk_compare(struct ipr_dev *dev, char *name)
{
	return strcmp(dev->dev_name, name);
}

/**
 * find_blk_dev -
 * @blk:		block device name
 *
 * Returns:
 *   result of _find_dev()
 **/
struct ipr_dev *find_blk_dev(char *blk)
{
	return _find_dev(blk, blk_compare);
}

/**
 * gen_compare - 
 * @dev:		ipr dev struct
 * @name:		character string to compare
 *
 * Returns:
 *   integer result of the strcmp()
 **/
static int gen_compare(struct ipr_dev *dev, char *name)
{
	return strcmp(dev->gen_name, name);
}

/**
 * find_gen_dev - 
 * @gen:		generic device name
 *
 * Returns:
 *   result of _find_dev()
 **/
struct ipr_dev *find_gen_dev(char *gen)
{
	return _find_dev(gen, gen_compare);
}

/**
 * find_dev - 
 * @name:		device name to find
 *
 * Returns:
 *   ipr_dev pointer
 **/
struct ipr_dev *find_dev(char *name)
{
	struct ipr_dev *dev = find_blk_dev(name);

	if (!dev)
		dev = find_gen_dev(name);
	return dev;
}

/**
 * ipr_uevents_supported - indicate if uevents are supported
 *
 * Returns:
 *   1 if success / 0 on failure
 **/
static int ipr_uevents_supported()
{
	struct ipr_ioa *ioa = ipr_ioa_head;
	char devpath[PATH_MAX];
	char value[16];
	ssize_t len;

	if (!ioa)
		return 0;

	sprintf(devpath, "/sys/class/scsi_host/%s", ioa->host_name);
	len = sysfs_read_attr(devpath, "uevent", value, 16);
	return len > 0;
}

#define NETLINK_KOBJECT_UEVENT        15

/**
 * poll_forever -
 * @poll_func:		pointer to polling function
 * @poll_delay:         integer sleep delay value
 *
 * Returns:
 *   nothing
 **/
static void poll_forever(void (*poll_func) (void), int poll_delay)
{
	while (1) {
		sleep(poll_delay);
		poll_func();
	}
}

/**
 * handle_events - 
 * @poll_func:		pointer to polling function
 * @poll_delay:         integer sleep delay value
 * @prot_level:		pointer to event handler function
 *
 * Returns:
 *   0 if poll_forever() ever returns
 **/
int handle_events(void (*poll_func) (void), int poll_delay,
		  void (*kevent_handler) (char *))
{
	struct sockaddr_nl snl;
	int sock, rc, len, flags;
	int uevent_rcvd = ipr_force_uevents;
	char buf[1024];

	if (ipr_force_polling) {
		poll_forever(poll_func, poll_delay);
		return 0;
	}

	memset(&snl, 0, sizeof(snl));
	snl.nl_family = AF_NETLINK;
	snl.nl_pid = getpid();
	snl.nl_groups = 1;

	sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if (sock == -1) {
		syslog_dbg("Failed to get socket\n");
		poll_forever(poll_func, poll_delay);
		return 0;
	}

	rc = bind(sock, (struct sockaddr *)&snl, sizeof(snl));

	if (rc < 0) {
		syslog_dbg("Failed to bind to socket\n");
		close(sock);
		poll_forever(poll_func, poll_delay);
		return 0;
	}

	if (!ipr_force_uevents && !ipr_uevents_supported()) {
		if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1) {
			syslog(LOG_ERR, "Failed to fcntl socket\n");
			close(sock);
			poll_forever(poll_func, poll_delay);
			return 0;
		}
	}

	while (1) {
		struct sockaddr_nl src_addr;
		socklen_t addrlen = sizeof(struct sockaddr_nl);

		len = recvfrom(sock, &buf, sizeof(buf), 0,
			       (struct sockaddr *) &src_addr, &addrlen);

		if (!uevent_rcvd && len < 0) {
			poll_func();
			sleep(poll_delay);
			continue;
		}

		if (len < 0) {
			syslog_dbg("kevent recv failed.\n");
			poll_func();
			sleep(poll_delay);
			continue;
		}

		if (src_addr.nl_pid != 0) {
			syslog_dbg("received packet from unknown pid %d.\n",
				   src_addr.nl_pid);
			poll_func();
			sleep(poll_delay);
			continue;
		}

		if (!uevent_rcvd) {
			flags = fcntl(sock, F_GETFL);
			if (flags == -1) {
				syslog_dbg("F_GETFL Failed\n");
				poll_func();
				sleep(poll_delay);
				continue;
			}

			if (flags & O_NONBLOCK) {
				if (fcntl(sock, F_SETFL, (flags & ~O_NONBLOCK)) == -1) {
					syslog_dbg("F_SETFL Failed\n");
					poll_func();
					sleep(poll_delay);
					continue;
				}
			}

			uevent_rcvd = 1;
		}

		kevent_handler(buf);
	}

	close(sock);
	return 0;
}

/**
 * parse_option - parse options from the command line
 * @opt:		character string option value
 *
 * Returns:
 *   1 if success / 0 if the option isn't recognized
 **/
int parse_option(char *opt)
{
	if (strcmp(opt, "--version") == 0) {
		printf("%s: %s\n", tool_name, PACKAGE_VERSION);
		exit(0);
	}

	if (strcmp(opt, "--daemon") == 0)
		daemonize = 1;
	else if (strcmp(opt, "--debug") == 0)
		ipr_debug = 1;
	else if (strcmp(opt, "--force") == 0)
		ipr_force = 1;
	else if (strcmp(opt, "--use-polling") == 0)
		ipr_force_polling = 1;
	else if (strcmp(opt, "--use-uevents") == 0)
		ipr_force_uevents = 1;
	else if (strcmp(opt, "--fast") == 0)
		ipr_fast = 1;
	else if (strcmp(opt, "--deferred-write-buffer") == 0)
		ipr_mode5_write_buffer = 0;
	else if (strcmp(opt, "--mode5-write-buffer") == 0)
		ipr_mode5_write_buffer = 1;
	else
		return 0;

	return 1;
}

/**
 * get_pci_attr - 
 * @sysfs_pci_device:	sysfs_device struct
 * @attr:		attribute string
 *
 * Returns:
 *    pci attribute value if success / 0 on failure
 **/
static int get_pci_attr(char *devpath, char *attr)
{
	int temp, fd, len;
	char path[PATH_MAX];
	char data[200];

	sprintf(path, "%s/%s", devpath, attr);

	if ((fd = open(path, O_RDONLY)) < 0) {
		syslog_dbg("Failed to open %s\n", path);
		return 0;
	}

	len = read(fd, data, 200);
	if (len < 0) {
		syslog_dbg("Failed to read %s\n", path);
		close(fd);
		return 0;
	}

	sscanf(data, "0x%4X", &temp);
	close(fd);
	return temp;
}

/**
 * get_pci_attrs - sets vendor and device information
 * @ioa:		ipr ioa struct
 * @sysfs_pci_device:	sysfs_device struct
 *
 * Returns:
 *   nothing
 **/
static void get_pci_attrs(struct ipr_ioa *ioa, char *pci_path)
{
	ioa->subsystem_vendor = get_pci_attr(pci_path, "subsystem_vendor");
	ioa->subsystem_device = get_pci_attr(pci_path, "subsystem_device");
	ioa->pci_vendor = get_pci_attr(pci_path, "vendor");
	ioa->pci_device = get_pci_attr(pci_path, "device");
}

static struct ipr_ioa *old_ioa_head;
static struct ipr_ioa *old_ioa_tail;
static int old_num_ioas;
static struct scsi_dev_data *old_scsi_dev_table;
struct ipr_array_query_data *old_qac_data;

/**
 * free_current_config - frees current configuration data structures
 *
 * Returns:
 *   nothing
 **/
static void free_current_config()
{
	struct ipr_ioa *ioa;

	for (ioa = ipr_ioa_head; ioa;) {
		ioa = ioa->next;
		free(ipr_ioa_head);
		ipr_ioa_head = ioa;
	}

	free(ipr_qac_data);
	free(scsi_dev_table);

	ipr_ioa_head = NULL;
	ipr_ioa_tail = NULL;
	num_ioas = 0;
	scsi_dev_table = NULL;
	ipr_qac_data = NULL;
}

/**
 * save_old_config - saves current configuration data structures
 *
 * Returns:
 *   nothing
 **/
static void save_old_config()
{
	if (old_ioa_head) {
		free_current_config();
		return;
	}

	old_ioa_head = ipr_ioa_head;
	old_ioa_tail = ipr_ioa_tail;
	old_num_ioas = num_ioas;
	old_scsi_dev_table = scsi_dev_table;
	old_qac_data = ipr_qac_data;

	ipr_qac_data = NULL;
	scsi_dev_table = NULL;
	ipr_ioa_head = NULL;
	ipr_ioa_tail = NULL;
	num_ioas = 0;
}

/**
 * free_old_config - frees old configuration data structures
 *
 * Returns:
 *   nothing
 **/
static void free_old_config()
{
	struct ipr_ioa *ioa;

	/* Free up all the old memory */
	for (ioa = old_ioa_head; ioa;) {
		ioa = ioa->next;
		free(old_ioa_head);
		old_ioa_head = ioa;
	}

	free(old_qac_data);
	free(old_scsi_dev_table);

	old_ioa_head = NULL;
	old_ioa_tail = NULL;
	old_num_ioas = 0;
	old_scsi_dev_table = NULL;
	old_qac_data = NULL;
}

/**
 * same_ioa - compares two ioas to determine if they are the same
 * @first:		ipr ioa struct
 * @second:		ipr ioa struct
 *
 * Returns:
 *   0 if ioas are not the same / 1 if ioas are the same
 **/
static int same_ioa(struct ipr_ioa *first, struct ipr_ioa *second)
{
	if (strcmp(first->pci_address, second->pci_address))
		return 0;
	if (strcmp(first->host_name, second->host_name))
		return 0;
	if (first->ccin != second->ccin)
		return 0;
	if (first->host_num != second->host_num)
		return 0;
	if (first->pci_vendor != second->pci_vendor)
		return 0;
	if (first->pci_device != second->pci_device)
		return 0;
	if (first->subsystem_vendor != second->subsystem_vendor)
		return 0;
	if (first->subsystem_device != second->subsystem_device)
		return 0;
	return 1;
}

/**
 * same_scsi_dev - compares two scsi devices to determin if they are the same
 * @first:		scsi_dev_data struct
 * @second:		scsi_dev_data struct
 *
 * Returns:
 *   0 if scsi devs are not the same / 1 if scsi devs are the same
 **/
static int same_scsi_dev(struct scsi_dev_data *first, struct scsi_dev_data *second)
{
	if (!first || !second)
		return 0;
	if (first->host != second->host)
		return 0;
	if (first->channel != second->channel)
		return 0;
	if (first->id != second->id)
		return 0;
	if (first->lun != second->lun)
		return 0;
	if (first->type != second->type)
		return 0;
	if (first->online != second->online)
		return 0;
	if (strcmp(first->vendor_id, second->vendor_id))
		return 0;
	if (strcmp(first->product_id, second->product_id))
		return 0;
	if (strcmp(first->sysfs_device_name, second->sysfs_device_name))
		return 0;
	if (strcmp(first->dev_name, second->dev_name))
		return 0;
	if (strcmp(first->gen_name, second->gen_name))
		return 0;
	return 1;
}

/**
 * same_dev_rcd - compares two device records to determine if they are the same
 * @first:		ipr_dev struct
 * @second:		ipr_dev struct
 *
 * Returns:
 *   0 if devices are not the same / 1 if devices are the same
 **/
static int same_dev_rcd(struct ipr_dev *first, struct ipr_dev *second)
{
	if (memcmp(&first->res_addr, &second->res_addr,
		   sizeof(first->res_addr)))
		return 0;
	if (memcmp(first->vendor_id, second->vendor_id, IPR_VENDOR_ID_LEN))
		return 0;
	if (memcmp(first->product_id, second->product_id, IPR_PROD_ID_LEN))
		return 0;
	if (memcmp(first->serial_number, second->serial_number, IPR_SERIAL_NUM_LEN))
		return 0;
	return 1;
}

/**
 * same_dev - compares two devices to determine if they are the same
 * @first:		ipr_dev struct
 * @second:		ipr_dev struct
 *
 * Returns:
 *   0 if devices are not the same / 1 if devices are the same
 **/
static int same_dev(struct ipr_dev *first, struct ipr_dev *second)
{
	if (strcmp(first->dev_name, second->dev_name))
		return 0;
	if (strcmp(first->gen_name, second->gen_name))
		return 0;
	if (!first->scsi_dev_data && !second->scsi_dev_data) {
		if (!ipr_is_af_dasd_device(first) || !ipr_is_af_dasd_device(second))
			return 0;
		if (!same_dev_rcd(first, second))
			return 0;
	} else if (!same_scsi_dev(first->scsi_dev_data, second->scsi_dev_data))
		return 0;
	return 1;
}

/**
 * dev_init_allowed - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   1 if initialization is allowed / 0 if initialization is not allowed
 **/
static int dev_init_allowed(struct ipr_dev *dev)
{
	struct ipr_query_res_state res_state;

	if (!ipr_is_af_dasd_device(dev))
		return 1;
	if (!ipr_query_resource_state(dev, &res_state)) {
		if (!res_state.read_write_prot && !res_state.prot_dev_failed)
			return 1;
	}
	return 0;
}

/**
 * resolve_dev - 
 * @new:		ipr dev struct
 * @old:		ipr dev struct
 *
 * Returns:
 *   nothing
 **/
static void resolve_dev(struct ipr_dev *new, struct ipr_dev *old)
{
	new->init_not_allowed = !dev_init_allowed(new);
	if (!old->init_not_allowed || new->init_not_allowed)
		new->should_init = 0;
	if (!new->ioa->sis64 && new->ioa->is_secondary &&
	   !old->ioa->is_secondary && ipr_is_af_dasd_device(new) &&
	   new->scsi_dev_data)
		new->rescan = 1;
	if (!new->ioa->sis64 && !new->ioa->is_secondary &&
	   old->ioa->is_secondary && ipr_is_af_dasd_device(new) &&
	   !new->scsi_dev_data)
		new->rescan = 1;
}

/**
 * resolve_ioa - 
 * @ioa:		ipr ioa struct
 * @old_ioa:		ipr ioa struct
 *
 * Returns:
 *   nothing
 **/
static void resolve_ioa(struct ipr_ioa *ioa, struct ipr_ioa *old_ioa)
{
	struct ipr_dev *dev, *old_dev;

	ioa->should_init = 0;

	for_each_dev(ioa, dev) {
		for_each_dev(old_ioa, old_dev) {
			if (!same_dev(dev, old_dev))
				continue;
			memcpy(&dev->attr, &old_dev->attr, sizeof(dev->attr));
			dev->rescan = old_dev->rescan;
			resolve_dev(dev, old_dev);
			break;
		}
	}
}

/**
 * resolve_old_config - 
 *
 * Returns:
 *   nothing
 **/
static void resolve_old_config()
{
	struct ipr_ioa *ioa, *old_ioa;
	struct ipr_dev *dev;

	for_each_ioa(ioa) {
		ioa->should_init = 1;
		for_each_dev(ioa, dev)
			dev->should_init = 1;
	}

	for_each_ioa(ioa) {
		__for_each_ioa(old_ioa, old_ioa_head) {
			if (!same_ioa(ioa, old_ioa))
				continue;
			resolve_ioa(ioa, old_ioa);
			break;
		}
	}

	free_old_config();
}

struct ipr_pci_slot {
	char slot_name[PATH_MAX];
	char physical_name[PATH_MAX];
	char pci_device[PATH_MAX];
};

static struct ipr_pci_slot *pci_slot;
static unsigned int num_pci_slots;

/**
 * ipr_select_phy_location - 
 * @dirent:		dirent struct
 *
 * Returns:
 *   1 if d_name == "phy_location"/ 0 otherwise
 **/
static int ipr_select_phy_location(const struct dirent *dirent)
{
	if (strstr(dirent->d_name, "phy_location"))
		return 1;
	return 0;
}

static int ipr_select_pci_address(const struct dirent *dirent)
{
	if (strstr(dirent->d_name, "address"))
		return 1;
	return 0;
}

/**
 * read_attr_file -
 * @path:       	path name of device/file to open
 * @out:		character array
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int read_attr_file(char *path, char *out, int size)
{
	int fd, len;

	if ((fd = open(path, O_RDONLY)) < 0) {
		syslog_dbg("Failed to open %s\n", path);
		return -EIO;
	}

	len = read(fd, out, size);
	if (len < 0) {
		syslog_dbg("Failed to read %s\n", path);
		close(fd);
		return -EIO;
	}

	if (out[strlen(out) - 1] == '\n')
		out[strlen(out) - 1] = '\0';

	close(fd);
	return 0;
}

/**
 * ipr_add_slot_location -
 * @path:		path of device/file to open
 * @name:       	name of device/file to open
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int ipr_add_slot_location(char *path, char *name)
{
	struct ipr_pci_slot *slot;
	char fpath[PATH_MAX];
	int rc;

	sprintf(fpath, "%s%s/phy_location", path, name);

	pci_slot = realloc(pci_slot, sizeof(*pci_slot) * (num_pci_slots + 1));
	slot = &pci_slot[num_pci_slots];
	memset(slot, 0, sizeof(*slot));

	strcpy(slot->slot_name, name);
	rc = read_attr_file(fpath, slot->physical_name,
			    sizeof(slot->physical_name));

	if (rc) {
		pci_slot = realloc(pci_slot, sizeof(*pci_slot) * num_pci_slots);
		return rc;
	}

	num_pci_slots++;
	return 0;
}

/**
 * ipr_add_slot_address -
 * @path:		path of device/file to open
 * @name:       	name of device/file to open
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int ipr_add_slot_address(char *path, char *name)
{
	struct ipr_pci_slot *slot;
	char fpath[PATH_MAX];
	int rc;

	sprintf(fpath, "%s%s/address", path, name);

	pci_slot = realloc(pci_slot, sizeof(*pci_slot) * (num_pci_slots + 1));
	slot = &pci_slot[num_pci_slots];
	memset(slot, 0, sizeof(*slot));

	strcpy(slot->slot_name, name);
	strcpy(slot->physical_name, name);
	rc = read_attr_file(fpath, slot->pci_device,
			    sizeof(slot->pci_device));

	if (rc) {
		pci_slot = realloc(pci_slot, sizeof(*pci_slot) * num_pci_slots);
		return rc;
	}

	strcat(slot->pci_device, ".0");

	num_pci_slots++;
	return 0;
}

/**
 * ipr_get_pci_slots -
 *
 * Returns:
 *   nothing
 **/
static void ipr_get_pci_slots()
{
	char rootslot[PATH_MAX], slot[PATH_MAX];
	char slotpath[PATH_MAX], attr[PATH_MAX];
	char devspec[PATH_MAX], locpath[PATH_MAX];
	char loc_code[1024], *last_hyphen, *prev_hyphen;
	int num_slots, i, j, rc, num_attrs;
	int loc_code_not_found = 0;
	struct dirent **slotdir, **dirent;
	struct stat statbuf;
	struct ipr_ioa *ioa;

	for_each_ioa(ioa)
		ioa->physical_location[0] = '\0';

	for_each_ioa(ioa) {
		memset(devspec, 0, sizeof(devspec));
		sprintf(attr, "/sys/bus/pci/devices/%s/devspec",
			ioa->pci_address);
		rc = read_attr_file(attr, devspec, PATH_MAX);

		if (rc)
			continue;

		memset(loc_code, 0, sizeof(loc_code));
		sprintf(locpath, "/proc/device-tree%s/ibm,loc-code",
			devspec);
		rc = read_attr_file(locpath, loc_code,
				    sizeof(loc_code));

		if (rc) {
			loc_code_not_found = 1;
			continue;
		}

		last_hyphen = strrchr(loc_code, '-');
		if (last_hyphen && last_hyphen[1] == 'T') {
			*last_hyphen = '\0';
			prev_hyphen = strrchr(loc_code, '-');
			if (prev_hyphen && prev_hyphen[1] != 'C')
					*last_hyphen = '-';
			}

		strcpy(ioa->physical_location, loc_code);
	}

	if (loc_code_not_found == 0)
		return;

	sprintf(rootslot, "/sys/bus/pci/slots/");

	num_slots = scandir(rootslot, &slotdir, NULL, alphasort);
	if (num_slots <= 0) {
		return;
	}

	for (i = 0; i < num_slots; i++) {
		sprintf(slot, "%s%s", rootslot, slotdir[i]->d_name);
		rc = scandir(slot, &dirent, ipr_select_phy_location, alphasort);
		if (rc > 0) {
			ipr_add_slot_location(rootslot, slotdir[i]->d_name);
			while (rc--)
				free(dirent[rc]);
			free(dirent);
		} else {
			rc = scandir(slot, &dirent, ipr_select_pci_address,
				     alphasort);

			if (rc <= 0)
				continue;

			ipr_add_slot_address(rootslot, slotdir[i]->d_name);
			while (rc--)
				free(dirent[rc]);
			free(dirent);
		}
	}

	while (num_slots--)
		free(slotdir[num_slots]);
	free(slotdir);

	for (i = 0; i < num_pci_slots; i++) {

		if (!pci_slot[i].pci_device)
			continue;

		sprintf(slotpath, "/sys/bus/pci/devices/%s/",
			pci_slot[i].slot_name);

		num_attrs = scandir(slotpath, &slotdir, NULL, alphasort);
		if (num_attrs <= 0)
			continue;

		for (j = 0; j < num_attrs; j++) {
			sprintf(attr, "%s/%s", slotpath, slotdir[j]->d_name);
			rc = stat(attr, &statbuf);
			if (rc || !S_ISDIR(statbuf.st_mode))
				continue;
			if (!strcmp(slotdir[j]->d_name, "."))
				continue;
			if (!strcmp(slotdir[j]->d_name, ".."))
				continue;
			strcpy(pci_slot[i].pci_device, slotdir[j]->d_name);
			break;
		}

		while (num_attrs--)
			free(slotdir[num_attrs]);
		free(slotdir);
	}

	for_each_ioa(ioa) {
		if (strlen(ioa->physical_location) == 0) {
			for (i = 0; i < num_pci_slots; i++) {
				if (strcmp(pci_slot[i].pci_device, ioa->pci_address) &&
				    strcmp(pci_slot[i].slot_name, ioa->pci_address))
					continue;
				strcpy(ioa->physical_location,
				       pci_slot[i].physical_name);
				break;
			}
		}
	}

	free(pci_slot);
	pci_slot = NULL;
	num_pci_slots = 0;
}

/**
 * load_system_p_oper_mode
 *
 * Discover and load current operating mode.
 **/
void load_system_p_oper_mode()
{
	struct stat st;

	if (stat("/proc/ppc64/lparcfg", &st) == 0)
		power_cur_mode = POWER_VM;
	else if (stat("/etc/ibm_powerkvm-release", &st) == 0)
		power_cur_mode = POWER_KVM;
	else
		power_cur_mode = POWER_BAREMETAL;
}

/**
 * tool_init -
 * @save_state:		integer flag - whether or not to save the old config
 *
 * Returns:
 *   nothing
 **/
static int __tool_init(int save_state)
{
	int temp, fw_type;
	struct ipr_ioa *ipr_ioa;
	DIR *dirfd, *host_dirfd;
	struct dirent *dent, *host_dent;
	ssize_t len;
	char queue_str[256];

	save_old_config();

	dirfd = opendir("/sys/bus/pci/drivers/ipr");
	if (!dirfd) {
		syslog_dbg("Failed to open ipr driver bus. %m\n");
		dirfd = opendir("/sys/bus/pci/drivers/ipr");
		if (!dirfd) {
			syslog_dbg("Failed to open ipr driver bus on second attempt. %m\n");
			return -ENXIO;
		}
	}
	while ((dent = readdir(dirfd)) != NULL) {
		int channel, bus, device, function;
		char devpath[PATH_MAX];
		char buff[256];

		if (sscanf(dent->d_name, "%x:%x:%x.%x",
			   &channel, &bus, &device, &function) != 4)
			continue;

		ipr_ioa = (struct ipr_ioa*)malloc(sizeof(struct ipr_ioa));
		memset(ipr_ioa,0,sizeof(struct ipr_ioa));

		/* PCI address */
		strcpy(ipr_ioa->pci_address, dent->d_name);
		ipr_ioa->host_num = -1;
		sprintf(devpath, "/sys/bus/pci/drivers/ipr/%s",
			dent->d_name);
		host_dirfd = opendir(devpath);
		if (!host_dirfd) {
			syslog_dbg("Failed to open scsi_host class.\n");
			return -EAGAIN;
		}
		while ((host_dent = readdir(host_dirfd)) != NULL) {
			char scsipath[PATH_MAX];
			char fw_str[256];

			if (strncmp(host_dent->d_name, "host", 4))
				continue;

			if (sscanf(host_dent->d_name, "host%d",
				   &ipr_ioa->host_num) != 1)
				continue;

			strcpy(ipr_ioa->host_name, host_dent->d_name);
			get_pci_attrs(ipr_ioa, devpath);

			sprintf(scsipath, "%s/%s/scsi_host/%s", devpath,
				ipr_ioa->host_name, ipr_ioa->host_name);
			len = sysfs_read_attr(scsipath, "fw_type", fw_str, 256);
			if (len < 0)
				fw_type = 0;
			else
				sscanf(fw_str, "%d", &fw_type);
			if (ipr_debug)
				syslog(LOG_INFO, "tool_init: fw_type attr = %d.\n", fw_type);

			len = sysfs_read_attr(scsipath, "can_queue", queue_str, 256);
			if (len < 0)
				ioa_dbg(ipr_ioa, "Failed to read can_queue attribute");
			sscanf(queue_str, "%d", &ipr_ioa->can_queue);
			break;
		}
		closedir(host_dirfd);
		if (ipr_ioa->host_num < 0) {
			syslog_dbg("No SCSI Host found on IPR device %s\n",
				      ipr_ioa->pci_address);
			free(ipr_ioa);
			continue;
		}
		if (fw_type == IPR_SIS64) {
			sprintf(devpath, "/sys/bus/scsi/devices/%d:%d:0:0",
				ipr_ioa->host_num, IPR_IOAFP_VIRTUAL_BUS);
			ipr_ioa->sis64 = 1;
		} else
			sprintf(devpath, "/sys/bus/scsi/devices/%d:255:255:255",
				ipr_ioa->host_num);
		len = sysfs_read_attr(devpath, "model", buff, 16);
		if (len < 0 || (sscanf(buff, "%4X", &temp) != 1)) {
			syslog_dbg("Cannot read SCSI device model.\n");
			return -EAGAIN;
		}
		ipr_ioa->ccin = temp;
		setup_ioa_parms(ipr_ioa);

		ipr_ioa->next = NULL;
		ipr_ioa->num_raid_cmds = 0;

		if (ipr_ioa_tail) {
			ipr_ioa_tail->next = ipr_ioa;
			ipr_ioa_tail = ipr_ioa;
		} else
			ipr_ioa_tail = ipr_ioa_head = ipr_ioa;

		num_ioas++;
	}
	closedir(dirfd);

	load_system_p_oper_mode();

	if (!save_state)
		free_old_config();

	ipr_get_pci_slots();
	return 0;
}

int tool_init(int save_state)
{
	int i, rc_err = -EAGAIN;

	for (i = 0; i < 4 && rc_err == -EAGAIN; i++) {
		rc_err = __tool_init(save_state);
		if (rc_err == -EAGAIN && tool_init_retry)
			sleep(2);
	}
	if (rc_err) {
		syslog_dbg("Failed to initialize adapters. Timeout reached");
		return rc_err;
	}

	return 0;
}

/**
 * ipr_reset_adapter -
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   nothing
 **/
void ipr_reset_adapter(struct ipr_ioa *ioa)
{
	char devpath[PATH_MAX];

	sprintf(devpath, "/sys/class/scsi_host/%s", ioa->host_name);
	sysfs_write_attr(devpath, "reset_host", "1", 1);
}

/**
 * ipr_read_host_attr -
 * @ioa:		ipr ioa struct
 * @name:		attribute name
 * @value:		value to be set
 * @value_len:		length of value string
 *
 * Returns:
 *   >= 0 if success / < 0 on failure
 **/
int ipr_read_host_attr(struct ipr_ioa *ioa, char *name,
		       void *value, size_t value_len)
{
	ssize_t len;
	char path[PATH_MAX];

	sprintf(path, "/sys/class/scsi_host/%s", ioa->host_name);
	len = sysfs_read_attr(path, name, value, value_len);
	if (len < 0) {
		ioa_dbg(ioa, "Failed to read %s attribute. %m\n", name);
		return -EIO;
	}
	return len;
}

/**
 * ipr_write_host_attr -
 * @ioa:		ipr ioa struct
 * @name:		attribute to be written
 * @value:		new value
 * @value_len:		length of value string
 *
 * Returns:
 *   >= 0 if success / < 0 on failure
 **/
int ipr_write_host_attr(struct ipr_ioa *ioa, char *name,
			void *value, size_t value_len)
{
	ssize_t len;
	char path[PATH_MAX];

	sprintf(path, "/sys/class/scsi_host/%s", ioa->host_name);
	len = sysfs_write_attr(path, name, value, value_len);
	if (len < 0) {
		ioa_dbg(ioa, "Failed to write %s attribute. %m\n", name);
		return -EIO;
	}
	return len;
}

/**
 * ipr_cmds_per_lun -
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   number of commands per lun if success / 6 on failure
 **/
int ipr_cmds_per_lun(struct ipr_ioa *ioa)
{
	char value[100];
	int rc;

	rc = ipr_read_host_attr(ioa, "cmd_per_lun", value, 100);

	if (rc)
		return 6;

	return strtoul(value, NULL, 10);
}

/**
 * ipr_scan -
 * @ioa:		ipr ioa struct
 * @bus:	        bus number
 * @id:		        id number
 * @lun:		lun number (RAID) level for new array
 *
 * Returns:
 *   nothing
 **/
void ipr_scan(struct ipr_ioa *ioa, int bus, int id, int lun)
{
	char buf[100];
	char devpath[PATH_MAX];

	if (lun < 0) {
		if (id < 0)
			sprintf(buf, "%d - -", bus);
		else
			sprintf(buf, "%d %d -", bus, id);
	} else {
		if (id < 0)
			sprintf(buf, "%d - %d", bus, lun);
		else
			sprintf(buf, "%d %d %d", bus, id, lun);
	}
	sprintf(devpath, "/sys/class/scsi_host/%s", ioa->host_name);
	sysfs_write_attr(devpath, "scan", buf, 100);
}

/**
 * __ipr_query_array_config - perform a query array configuration ioctl
 * @ioa:		ipr ioa struct
 * @fd:                 file descriptor
 * @allow_rebuld_refresh:	allow rebuild refresh flag
 * @set_array_id:		set array id flag
 * @array_id:		array id number
 * @buff:		data buffer
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int __ipr_query_array_config(struct ipr_ioa *ioa, int fd,
			     bool allow_rebuild_refresh, bool set_array_id,
			     bool vset_migrate_query, int array_id,
			     struct ipr_array_query_data *buff)
{
	int length = IPR_QAC_BUFFER_SIZE;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;

	if (!ioa->sis64)
		length = IPR_SIS32_QAC_BUFFER_SIZE;

	ioa->nr_ioa_microcode = 0;

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_QUERY_ARRAY_CONFIG;

	if (vset_migrate_query) {
		cdb[1] = 0x08;
		cdb[2] = array_id;
		if (ioa->sis64)
			cdb[3] = 0x80;
	} else if (set_array_id) {
		cdb[1] = 0x01;
		cdb[2] = array_id;
		if (ioa->sis64)
			cdb[3] = 0x80;
	} else if (allow_rebuild_refresh)
		cdb[1] = 0;
	else
		cdb[1] = 0x80; /* Prohibit Rebuild Candidate Refresh */

	cdb[7] = (length & 0xff00) >> 8;
	cdb[8] = length & 0xff;

	if (ioa->sis64) {
		cdb[10] = (length & 0xff000000) >> 24;
		cdb[11] = (length & 0xff0000) >> 16;
	}

	rc = sg_ioctl(fd, cdb, buff->u.buf,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if (rc != 0) {
		if (sense_data.sense_key != ILLEGAL_REQUEST || ipr_debug)
			ioa_cmd_err(ioa, &sense_data, "Query Array Config", rc);
		else if (sense_data.sense_key == NOT_READY &&
			 sense_data.add_sense_code == 0x40 &&
			 sense_data.add_sense_code_qual == 0x85)
			ioa->nr_ioa_microcode = 1;
	}

	if (ioa->sis64) {
		buff->resp_len = ntohl(buff->u.buf64.resp_len);
		buff->num_records = ntohl(buff->u.buf64.num_records);
		buff->data = buff->u.buf64.data;
		buff->hdr_len = 8;
	} else {
		buff->resp_len = ntohs(buff->u.buf32.resp_len);
		buff->num_records = ntohs(buff->u.buf32.num_records);
		buff->data = buff->u.buf32.data;
		buff->hdr_len = 4;
	}

	return rc;
}

/**
 * ipr_query_array_config - entry point to query array configuration ioctl call
 * @ioa:		ipr ioa struct
 * @allow_rebuld_refresh:	allow rebuild refresh flag
 * @set_array_id:		set array id flag
 * @array_id:		array id number
 * @buff:		data buffer
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_query_array_config(struct ipr_ioa *ioa,
			   bool allow_rebuild_refresh, bool set_array_id,
			   bool vset_migrate_query, int array_id,
			   struct ipr_array_query_data *buff)
{
	int fd, rc;

	ioa->nr_ioa_microcode = 0;

	if (strlen(ioa->ioa.gen_name) == 0) {
		scsi_err((&ioa->ioa), "Adapter sg device does not exist\n");
		return -ENOENT;
	}

	fd = open(ioa->ioa.gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", ioa->ioa.gen_name);
		return errno;
	}

	rc = flock(fd, LOCK_EX);
	if (rc) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not lock %s. %m\n", ioa->ioa.gen_name);
		close(fd);
		return errno;
	}

	rc = __ipr_query_array_config(ioa, fd, allow_rebuild_refresh,
				      set_array_id, vset_migrate_query,
				      array_id, buff);

	close(fd);
	return rc;
}

/**
 * ipr_query_multi_ioa_status -
 * @ioa:		ipr ioa struct
 * @buff:       	data buffer
 * @len:		length
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_query_multi_ioa_status(struct ipr_ioa *ioa, void *buff, u32 len)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;

	if (strlen(ioa->ioa.gen_name) == 0)
		return -ENOENT;

	fd = open(ioa->ioa.gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", ioa->ioa.gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_MAINTENANCE_IN;
	cdb[1] = IPR_QUERY_MULTI_ADAPTER_STATUS;
	cdb[6] = len >> 24;
	cdb[7] = (len >> 16) & 0xff;
	cdb[8] = (len >> 8) & 0xff;
	cdb[9] = len & 0xff;

	rc = sg_ioctl(fd, cdb, buff, len, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if (rc != 0) {
		if (sense_data.sense_key != ILLEGAL_REQUEST)
			ioa_cmd_err(ioa, &sense_data, "Query Multi Adapter Status", rc);
		else if (sense_data.sense_key == NOT_READY &&
			 sense_data.add_sense_code == 0x40 &&
			 sense_data.add_sense_code_qual == 0x85)
			ioa->nr_ioa_microcode = 1;
	}

	close(fd);
	return rc;
}

/**
 * ipr_start_array - Start array protection for an array
 * @ioa:		ipr ioa struct
 * @cmd:                char pointer
 * @stripe_size:	stripe size for new array
 * @prot_level:		protection (RAID) level for new array
 * @hot_spare:          flag to indicate hot spare usage
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int ipr_start_array(struct ipr_ioa *ioa, char *cmd,
			   int stripe_size, int prot_level, int hot_spare)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;
	int length = ioa->qac_data->resp_len;

	if (strlen(ioa->ioa.gen_name) == 0)
		return -ENOENT;

	fd = open(ioa->ioa.gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", ioa->ioa.gen_name);
		return errno;
	}

	ipr_update_qac_with_zeroed_devs(ioa);

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_START_ARRAY_PROTECTION;
	if (hot_spare)
		cdb[1] = 0x01;

	cdb[4] = (u8)(stripe_size >> 8);
	cdb[5] = (u8)(stripe_size & 0xff);
	cdb[6] = prot_level;
	cdb[7] = (length & 0xff00) >> 8;
	cdb[8] = length & 0xff;

	if (ioa->sis64) {
		cdb[10] = (length & 0xff000000) >> 24;
		cdb[11] = (length & 0xff0000) >> 16;
	}

	rc = sg_ioctl(fd, cdb, ioa->qac_data->u.buf,
		      length, SG_DXFER_TO_DEV,
		      &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if (rc != 0)
		ioa_cmd_err(ioa, &sense_data, "Start Array Protection", rc);

	close(fd);
	return rc;
}

/**
 * ipr_start_array_protection - Start array protection for an array
 * @ioa:		ipr ioa struct
 * @stripe_size:	stripe size for new array
 * @prot_level:		protection (RAID) level for new array
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_start_array_protection(struct ipr_ioa *ioa,
			       int stripe_size, int prot_level)
{
	return ipr_start_array(ioa, "Start Array Protection",
			       stripe_size, prot_level, 0);
}

/**
 * ipr_migrate_array_protection - Migrate array protection for an array
 * @ioa:		ipr ioa struct
 * @qac_data:		struct ipr_array_query_data
 * @fd:			file descriptor
 * @stripe_size:	new or existing stripe size for array
 * @prot_level:		new protection (RAID) level for array
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_migrate_array_protection(struct ipr_ioa *ioa,
				 struct ipr_array_query_data *qac_data,
				 int fd, int stripe_size, int prot_level)
{
	u8 cdb[IPR_CCB_CDB_LEN];
	char *cmd = "migrate array protection";
	struct sense_data_t sense_data;
	int rc;
	int length = qac_data->resp_len;

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_MIGRATE_ARRAY_PROTECTION;

	cdb[4] = (u8)(stripe_size >> 8);
	cdb[5] = (u8)(stripe_size & 0xff);
	cdb[6] = prot_level;
	cdb[7] = (length & 0xff00) >> 8;
	cdb[8] = length & 0xff;

	if (ioa->sis64) {
		cdb[10] = (length & 0xff000000) >> 24;
		cdb[11] = (length & 0xff0000) >> 16;
	}
	rc = sg_ioctl(fd, cdb, qac_data->u.buf,
		      length, SG_DXFER_TO_DEV,
		      &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if (rc != 0)
		ioa_cmd_err(ioa, &sense_data, cmd, rc);

	return rc;
}

/**
 * ipr_add_hot_spare - Adds a hot spare to an array
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_add_hot_spare(struct ipr_ioa *ioa)
{
	return ipr_start_array(ioa, "Create hot spare", 0, 0, 1);
}

/**
 * ipr_stop_array - Stop array protection for an array
 * @ioa:		ipr ioa struct
 * @cmd:        	command string
 * @hot_spare:		flag to indicate hot spare usage
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int ipr_stop_array(struct ipr_ioa *ioa, char *cmd, int hot_spare)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;
	int length = ioa->qac_data->resp_len;

	if (strlen(ioa->ioa.gen_name) == 0)
		return -ENOENT;

	fd = open(ioa->ioa.gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", ioa->ioa.gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_STOP_ARRAY_PROTECTION;
	if (hot_spare)
		cdb[1] = 0x01;

	cdb[7] = (length & 0xff00) >> 8;
	cdb[8] = length & 0xff;

	if (ioa->sis64) {
		cdb[10] = (length & 0xff000000) >> 24;
		cdb[11] = (length & 0xff0000) >> 16;
	}
	rc = sg_ioctl(fd, cdb, ioa->qac_data->u.buf,
		      length, SG_DXFER_TO_DEV,
		      &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if (rc != 0)
		ioa_cmd_err(ioa, &sense_data, cmd, rc);

	close(fd);
	return rc;
}

static void ipr_mpath_flush()
{
	int pid, status;

	/* flush unused multipath device maps */
	pid = fork();
	if (pid == 0) {
		execlp("multipath", "-F", NULL);
		_exit(errno);
	} else
		waitpid(pid, &status, 0);
}

/**
 * ipr_stop_array_protection - Stop array protection for an array
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_stop_array_protection(struct ipr_ioa *ioa)
{
	ipr_mpath_flush();
	return ipr_stop_array(ioa, "Stop Array Protection", 0);
}

/**
 * ipr_remove_hot_spare - remove a hot spare from an array
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_remove_hot_spare(struct ipr_ioa *ioa)
{
	return ipr_stop_array(ioa, "Delete hot spare", 1);
}

/**
 * ipr_rebuild_device_data - 
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_rebuild_device_data(struct ipr_ioa *ioa)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;
	int length = ioa->qac_data->resp_len;

	if (strlen(ioa->ioa.gen_name) == 0)
		return -ENOENT;

	fd = open(ioa->ioa.gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", ioa->ioa.gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_REBUILD_DEVICE_DATA;
	cdb[7] = (length & 0xff00) >> 8;
	cdb[8] = length & 0xff;

	if (ioa->sis64) {
		cdb[10] = (length & 0xff000000) >> 24;
		cdb[11] = (length & 0xff0000) >> 16;
	}
	rc = sg_ioctl(fd, cdb, ioa->qac_data->u.buf, length,
		      SG_DXFER_TO_DEV, &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if (rc != 0)
		ioa_cmd_err(ioa, &sense_data, "Rebuild Device Data", rc);

	close(fd);
	return rc;
}

/**
 * ipr_resync_array - 
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_resync_array(struct ipr_ioa *ioa)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;
	int length = ioa->qac_data->resp_len;

	if (strlen(ioa->ioa.gen_name) == 0)
		return -ENOENT;

	fd = open(ioa->ioa.gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", ioa->ioa.gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_RESYNC_ARRAY_PROTECTION;
	cdb[7] = (length & 0xff00) >> 8;
	cdb[8] = length & 0xff;

	if (ioa->sis64) {
		cdb[10] = (length & 0xff000000) >> 24;
		cdb[11] = (length & 0xff0000) >> 16;
	}
	rc = sg_ioctl(fd, cdb, ioa->qac_data->u.buf, length,
		      SG_DXFER_TO_DEV, &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if (rc != 0)
		ioa_cmd_err(ioa, &sense_data, "Resync Array", rc);

	close(fd);
	return rc;
}

/**
 * ipr_add_array_device - 
 * @ioa:		ipr ioa struct
 * @fd:                 device/file descriptor
 * @qac_data:		query array configuration info
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_add_array_device(struct ipr_ioa *ioa, int fd,
			 struct ipr_array_query_data *qac_data)
{
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;
	int length = qac_data->resp_len;

	ipr_update_qac_with_zeroed_devs(ioa);

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_ADD_ARRAY_DEVICE;
	cdb[7] = (length & 0xff00) >> 8;
	cdb[8] = length & 0xff;

	if (ioa->sis64) {
		cdb[10] = (length & 0xff000000) >> 24;
		cdb[11] = (length & 0xff0000) >> 16;
	}
	rc = sg_ioctl(fd, cdb, qac_data->u.buf,
		      length, SG_DXFER_TO_DEV,
		      &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if (rc != 0)
		ioa_cmd_err(ioa, &sense_data, "Add Array Device", rc);

	return rc;
}

/**
 * ipr_query_command_status - 
 * @dev:		ipr dev struct
 * @buff:		data buffer
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_query_command_status(struct ipr_dev *dev, void *buff)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int length = sizeof(struct ipr_cmd_status);
	char *name = dev->gen_name;

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_QUERY_COMMAND_STATUS;
	cdb[7] = (length & 0xff00) >> 8;
	cdb[8] = length & 0xff;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if ((rc != 0) && (errno != EINVAL || ipr_debug))
		scsi_cmd_err(dev, &sense_data, "Query Command Status", rc);

	close(fd);
	return rc;
}

/**
 * ipr_query_resource_state
 * @dev:		ipr dev struct
 * @buff:		data buffer
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_query_resource_state(struct ipr_dev *dev, void *buff)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int length = sizeof(struct ipr_query_res_state);
	char *name = dev->gen_name;

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_QUERY_RESOURCE_STATE;
	cdb[7] = (length & 0xff00) >> 8;
	cdb[8] = length & 0xff;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if (rc != 0)
		scsi_cmd_err(dev, &sense_data, "Query Resource State", rc);

	close(fd);
	return rc;
}

/**
 * ipr_mod_sense - issue a mode sense command
 * @dev:		ipr dev struct
 * @page:       	mode page
 * @buff:		data buffer
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_mode_sense(struct ipr_dev *dev, u8 page, void *buff)
{
	struct sense_data_t sense_data;
	int rc;

	memset(&sense_data, 0, sizeof(sense_data));
	rc = mode_sense(dev, page, buff, &sense_data);

	/* post log if error unless error is device format in progress */
	if ((rc != 0) && (rc != ENOENT) &&
	    (((sense_data.error_code & 0x7F) != 0x70) ||
	     ((sense_data.sense_key & 0x0F) != 0x02) || /* NOT READY */
	     (sense_data.add_sense_code != 0x04) ||     /* LOGICAL UNIT NOT READY */
	     (sense_data.add_sense_code_qual != 0x04))) /* FORMAT IN PROGRESS */
		scsi_cmd_err(dev, &sense_data, "Mode Sense", rc);

	return rc;
}

/**
 * ipr_log_sense - issue a log sense command
 * @dev:		ipr dev struct
 * @page:       	mode page
 * @buff:		data buffer
 * @length:             buffer length
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_log_sense(struct ipr_dev *dev, u8 page, void *buff, u16 length)
{
	struct sense_data_t sense_data;
	u8 cdb[IPR_CCB_CDB_LEN];
	int fd, rc;
	char *name = dev->gen_name;

	memset(&sense_data, 0, sizeof(sense_data));

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		scsi_dbg(dev, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = LOG_SENSE;
	cdb[2] = 0x40 | page;
	cdb[7] = (length >> 8) & 0xff;
	cdb[8] = length & 0xff;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	scsi_cmd_dbg(dev, &sense_data, cdb, rc);
	close(fd);
	return rc;
}

/**
 * ipr_is_log_page_supported - is the give page supported?
 * @dev:		ipr dev struct
 * @page:       	mode page
 *
 * Returns:
 *   1 if supported / 0 if not supported
 **/
int ipr_is_log_page_supported(struct ipr_dev *dev, u8 page)
{
	struct ipr_supp_log_pages pages;
	int rc, i;

	memset(&pages, 0, sizeof(pages));
	rc = ipr_log_sense(dev, 0, &pages, sizeof(pages));

	if (rc)
		return -EIO;

	for (i = 0; i < ntohs(pages.length); i++) {
		if (pages.page[i] == page)
			return 1;
	}

	return 0;
}

/** ipr_sas_log_get_param - Fetch parameter from log page.
 *
 * Iterate over an already fetched log page pointed by page and return a
 * pointer to the beginning of the parameter.
 *
 * @page: An already fetched log page.
 * @param: to be fetched
 *
 * Returns: @dst: A pointer to the original page area starting at the
 * parameter. NULL if parameter was not found.
 * @entries_cnt: output parameter, the number of entries found in the
 * page.
 **/
void *ipr_sas_log_get_param(const struct ipr_sas_log_page *page,
			    u32 param_code, int *entries_cntr)
{
	int page_length;
	int i;
	const u8 *raw_data = page->raw_data;
	u32 cur_code;
	struct log_parameter_hdr *hdr;
	void *ret = NULL;

	if (entries_cntr)
		*entries_cntr = 0;

	page_length = (page->page_length[0] << 8) | page->page_length[1];

	for(i = 0; i < page_length && i < IPR_SAS_LOG_MAX_ENTRIES;
	    i += hdr->length + sizeof(*hdr)) {
		hdr = (struct log_parameter_hdr *) &raw_data[i];
		cur_code = (hdr->parameter_code[0] << 8) | hdr->parameter_code[1];

		if (cur_code == param_code) {
			ret = hdr;
			if (!entries_cntr)
				break;
		}

		if (entries_cntr)
			*entries_cntr += 1;
	}
	return ret;
}

/**
 * ipr_get_blk_size - return the block size for the given device
 * @dev:		ipr dev struct
 *
 * Returns:
 *   block size if success / non-zero on failure
 **/
int ipr_get_blk_size(struct ipr_dev *dev)
{
	struct ipr_mode_pages mode_pages;
	struct ipr_block_desc *block_desc;
	int rc;

	rc = ipr_mode_sense(dev, 0, &mode_pages);

	if (rc)
		return rc;

	if (mode_pages.hdr.block_desc_len > 0) {
		block_desc = (struct ipr_block_desc *)mode_pages.data;
		return ((block_desc->block_length[1] << 8) | block_desc->block_length[2]);
	}

	return 0;
}

/**
 * ipr_mode_select - issue a mode select command
 * @dev:		ipr dev struct
 * @buff:	        data buffer
 * @length:		length of buffer
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_mode_select(struct ipr_dev *dev, void *buff, int length)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;
	char *name = dev->gen_name;

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = MODE_SELECT;
	cdb[1] = 0x10;  /* PF = 1, SP = 0 */
	cdb[4] = length;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_TO_DEV,
		      &sense_data, IPR_INTERNAL_TIMEOUT);

	if (rc != 0)
		scsi_cmd_err(dev, &sense_data, "Mode Select", rc);

	close(fd);
	return rc;
}

/**
 * page0x0a_setup - Start array protection for an array
 * @dev:		ipr dev struct
 *
 * Returns:
 *   1 if success / 0 on failure
 **/
int page0x0a_setup(struct ipr_dev *dev)
{
	struct ipr_mode_pages mode_pages;
	struct ipr_control_mode_page *page;

	memset(&mode_pages, 0, sizeof(mode_pages));

	if (ipr_mode_sense(dev, 0x0A, &mode_pages))
		return -EIO;

	page = (struct ipr_control_mode_page *)(((u8 *)&mode_pages) +
						mode_pages.hdr.block_desc_len +
						sizeof(mode_pages.hdr));

	if (page->queue_algorithm_modifier != 1)
		return 0;
	if (page->tst == 1 && page->qerr != 3)
		return 0;
	if (page->qerr != 1)
		return 0;
	if (page->dque != 0)
		return 0;

	return 1;
}

/**
 * ipr_setup_af_qdepth - 
 * @dev:		ipr dev struct
 * @qdepth:		queue depth
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int ipr_setup_af_qdepth(struct ipr_dev *dev, int qdepth)
{
	struct ipr_mode_pages mode_pages;
	struct ipr_ioa_mode_page *page;
	int len;

	memset(&mode_pages, 0, sizeof(mode_pages));

	if (ipr_mode_sense(dev, 0x20, &mode_pages))
		return -EIO;

	page = (struct ipr_ioa_mode_page *) (((u8 *)&mode_pages) +
					     mode_pages.hdr.block_desc_len +
					     sizeof(mode_pages.hdr));

	page->max_tcq_depth = qdepth;

	len = mode_pages.hdr.length + 1;
	mode_pages.hdr.length = 0;
	mode_pages.hdr.medium_type = 0;
	mode_pages.hdr.device_spec_parms = 0;
	page->hdr.parms_saveable = 0;

	if (ipr_mode_select(dev, &mode_pages, len))
		return -EIO;

	return 0;

}

/**
 * ipr_reset_device - issue a SG_SCSI_RESET to a device
 * @dev:		ipr dev struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_reset_device(struct ipr_dev *dev)
{
	int fd, rc, arg;

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
		return errno;
	}

	arg = SG_SCSI_RESET_DEVICE;
	rc = ioctl(fd, SG_SCSI_RESET, &arg);

	if (rc != 0)
		scsi_err(dev, "Reset Device failed. %m\n");

	close(fd);
	return rc;
}

/**
 * ipr_re_read_partition - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_re_read_partition(struct ipr_dev *dev)
{
	int fd, rc;

	if (strlen(dev->dev_name) == 0)
		return -ENOENT;

	fd = open(dev->dev_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
		return errno;
	}

	rc = ioctl(fd, BLKRRPART, NULL);

	if (rc != 0)
		scsi_err(dev, "Re-read partition table failed. %m\n");

	close(fd);
	return rc;
}

/**
 * ipr_read_capacity - issue a read capacity command to a device
 * @dev:		ipr dev struct
 * @buff:		data buffer
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_read_capacity(struct ipr_dev *dev, void *buff)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;
	int length = sizeof(struct ipr_read_cap);
	char *name = dev->gen_name;

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = READ_CAPACITY;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0)
		scsi_cmd_err(dev, &sense_data, "Read Capacity", rc);

	close(fd);
	return rc;
}

/**
 * ipr_read_capacity_16 - issue a read capacity 16 to a device
 * @dev:		ipr dev struct
 * @buff:		data buffer
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_read_capacity_16(struct ipr_dev *dev, void *buff)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int length = sizeof(struct ipr_read_cap16);
	char *name = dev->gen_name;

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_SERVICE_ACTION_IN;
	cdb[1] = IPR_READ_CAPACITY_16;

	cdb[10] = length >> 24;
	cdb[11] = length>> 16 & 0xff;
	cdb[12] = length >> 8 & 0xff;
	cdb[13] = length & 0xff;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0)
		scsi_cmd_err(dev, &sense_data, "Read Capacity", rc);

	close(fd);
	return rc;
}

/**
 * ipr_reclaim_cache_store - 
 * @ioa:		ipr ioa struct
 * @action:	        action value
 * @buff:		data buffer
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_reclaim_cache_store(struct ipr_ioa *ioa, int action, void *buff)
{
	char *file = ioa->ioa.gen_name;
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int timeout, rc;
	int length = sizeof(struct ipr_reclaim_query_data);

	if (strlen(file) == 0)
		return -ENOENT;

	fd = open(file, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n",file);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_RECLAIM_CACHE_STORE;
	cdb[1] = (u8)action;
	cdb[7] = (length & 0xff00) >> 8;
	cdb[8] = length & 0xff;

	if ((action & IPR_RECLAIM_ACTION) == IPR_RECLAIM_PERFORM)
		timeout = IPR_CACHE_RECLAIM_TIMEOUT;
	else
		timeout = IPR_INTERNAL_DEV_TIMEOUT;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, timeout);

	if (rc != 0) {
		if (sense_data.sense_key != ILLEGAL_REQUEST)
			ioa_cmd_err(ioa, &sense_data, "Reclaim Cache", rc);
	}
	else if ((action & IPR_RECLAIM_ACTION) == IPR_RECLAIM_PERFORM)
		ipr_reset_adapter(ioa);

	close(fd);
	return rc;
}

/**
 * ipr_start_stop - 
 * @dev:		ipr dev struct
 * @start:      	start or stop flag
 * @cmd:		command buffer
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int ipr_start_stop(struct ipr_dev *dev, u8 start, char *cmd)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	char *name = dev->gen_name;

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = START_STOP;
	cdb[4] = start;

	rc = sg_ioctl(fd, cdb, NULL,
		      0, SG_DXFER_NONE,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0)
		scsi_cmd_err(dev, &sense_data, cmd, rc);

	close(fd);
	return rc;
}

/**
 * ipr_start_stop_start - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_start_stop_start(struct ipr_dev *dev)
{
	return ipr_start_stop(dev, IPR_START_STOP_START, "Start Unit");
}

/**
 * ipr_check_allow_restart - check the allow_restart flag for a device
 * @dev:		ipr dev struct
 *
 * Return value:
 *   none
 **/
int ipr_check_allow_restart(struct ipr_dev *dev)
{
	char path[PATH_MAX];
	char value[8];
	ssize_t len;

	sprintf(path,"/sys/class/scsi_disk/%s",
		dev->scsi_dev_data->sysfs_device_name);
	len = sysfs_read_attr(path, "allow_restart", value, 8);
	if (len < 1) {
		syslog_dbg("Failed to open allow_restart parameter.\n");
		return -1;
	}

	return atoi(value);
}

/**
 * ipr_allow_restart - set or clear the allow_restart flag for a device
 * @dev:		ipr dev struct
 * @allow:		value to set
 *
 * Return value:
 *   none
 **/
void ipr_allow_restart(struct ipr_dev *dev, int allow)
{
	char path[PATH_MAX];
	char value_str[8];
	ssize_t len;

	sprintf(path,"/sys/class/scsi_disk/%s",
		dev->scsi_dev_data->sysfs_device_name);
	len = sysfs_read_attr(path, "allow_restart", value_str, 8);
	if (len < 1) {
		syslog_dbg("Failed to open allow_restart parameter.\n");
		return;
	}

	if (atoi(value_str) == allow) {
		return;
	}

	snprintf(value_str, 8, "%d", allow);

	if (sysfs_write_attr(path, "allow_restart", value_str, 1) < 0)
		syslog_dbg("Failed to write allow_restart parameter.\n");
}

/**
 * ipr_start_stop_stop -
 * @dev:		ipr dev struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_start_stop_stop(struct ipr_dev *dev)
{
	return ipr_start_stop(dev, IPR_START_STOP_STOP, "Stop Unit");
}

/**
 * ipr_set_dev_cache_policy
 *
 * Set the default cache policy for a device.
 *
 * @ipr_dev:            Device to change policy.
 * @policy:	        New policy
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int ipr_set_dev_wcache_policy (const struct ipr_dev *dev, int policy)
{
	char path[PATH_MAX];
	char *cache_type;
	char current_type[100];

	memset(current_type, 0, sizeof(current_type));
	sprintf(path, "/sys/class/scsi_disk/%s",
		dev->scsi_dev_data->sysfs_device_name);

	switch (policy) {
	case IPR_DEV_CACHE_WRITE_THROUGH:
		cache_type = "write through\n";
		break;
	case IPR_DEV_CACHE_WRITE_BACK:
		cache_type = "write back\n";
		break;
	default:
		return -EINVAL;
	}

	if (sysfs_write_attr(path, "cache_type", cache_type,
			     strlen(cache_type)) < 0) {
		syslog_dbg("Failed to write cache_type parameter of %s",
			   dev->dev_name);
		return -EINVAL;
	}

	if (sysfs_read_attr(path, "cache_type", current_type, 100) < 0) {
		syslog_dbg("Failed to read cache_type parameter of %s",
			   dev->dev_name);
		return -EINVAL;
	}

	if (strncmp (current_type, cache_type, 100) != 0) {
		syslog_dbg("Failed to set cache_type parameter of %s",
			   dev->dev_name);
		return -EINVAL;
	}
	return 0;
}

/**
 * ipr_dev_wcache_policy
 *
 * Check the default cache policy of a device.
 *
 * @ipr_dev:            Device to change policy.
 * @policy:	        Returned value.  New policy.
 *
 * Returns:
 *   policy = [0, 1]  / < 0 if failure.
 **/
static int ipr_dev_wcache_policy (const struct ipr_dev *dev)
{
	char path[PATH_MAX];
	char cache_type[100];

	sprintf(path, "/sys/class/scsi_disk/%s",
		dev->scsi_dev_data->sysfs_device_name);

	memset(cache_type, '\0', ARRAY_SIZE(cache_type));

	if (sysfs_read_attr(path, "cache_type", cache_type, 100) < 0) {
		syslog_dbg("Failed to read cache_type parameter of %s",
			   dev->dev_name);
		return -EINVAL;
	}

	if (strncmp (cache_type, "write through\n", 100) == 0)
		return IPR_DEV_CACHE_WRITE_THROUGH;
	else if (strncmp (cache_type, "write back\n", 100) == 0)
		return IPR_DEV_CACHE_WRITE_BACK;
	return -EINVAL;
}

/**
 * __ipr_test_unit_ready - issue a test unit ready command
 * @dev:		ipr dev struct
 * @sense_data:		sense data struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int __ipr_test_unit_ready(struct ipr_dev *dev,
			  struct sense_data_t *sense_data)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	int allow_restart = 0;
	char *name = dev->gen_name;

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = TEST_UNIT_READY;

	rc = sg_ioctl(fd, cdb, NULL,
		      0, SG_DXFER_NONE,
		      sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	close(fd);

	if (rc && sense_data->sense_key == NOT_READY &&
		   sense_data->add_sense_code == 0x4 &&
		   sense_data->add_sense_code_qual == 0x2) {
		allow_restart = ipr_check_allow_restart(dev);	
		if (allow_restart)
			ipr_start_stop(dev, IPR_START_STOP_START, "Start Unit");
	}

	return rc;
}

/**
 * ipr_test_unit_ready - issue a test unit ready command
 * @dev:		ipr dev struct
 * @sense_data:		sense data struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_test_unit_ready(struct ipr_dev *dev,
			struct sense_data_t *sense_data)
{
	int rc = __ipr_test_unit_ready(dev, sense_data);

	if (rc != 0 && sense_data->sense_key != NOT_READY &&
	    sense_data->sense_key != UNIT_ATTENTION &&
	    (!strcmp(tool_name, "iprconfig") || ipr_debug))
		scsi_cmd_err(dev, sense_data, "Test Unit Ready", rc);

	return rc;
}

static struct ipr_dev *find_multipath_jbod(struct ipr_dev *dev)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *multipath_dev;
	u64 id = dev->scsi_dev_data->device_id;

	for_each_sas_ioa(ioa) {
		if (ioa == dev->ioa)
			continue;

		for_each_dev(ioa, multipath_dev) {
			if (multipath_dev == dev)
				continue;

			if (multipath_dev->scsi_dev_data &&
			    id == multipath_dev->scsi_dev_data->device_id)
				return multipath_dev;

			}
	}

	return NULL;
}

int ipr_device_lock(struct ipr_dev *dev)
{
	int fd, rc;
	char *name = dev->gen_name;

	if (strlen(name) == 0)
		return -ENOENT;

	if (dev->locked) {
		scsi_dbg(dev, "Device already locked\n");
		return -EINVAL;
	}

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	rc = flock(fd, LOCK_EX | LOCK_NB);

	if (rc) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not lock %s. %m\n", name);
		close(fd);
		return errno;
	}

	/* Do not close the file descriptor here as we want to hold onto the lock */
	dev->locked = 1;
	dev->lock_fd = fd;
	return rc;
}

void ipr_device_unlock(struct ipr_dev *dev)
{
	if (dev->locked) {
		close(dev->lock_fd);
		dev->locked = 0;
		dev->lock_fd = 0;
	}
}

/**
 * ipr_format_unit - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_format_unit(struct ipr_dev *dev)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	u8 *defect_list_hdr;
	int length = IPR_DEFECT_LIST_HDR_LEN;
	char *name = dev->gen_name;

	if (strlen(dev->dev_name) && dev->scsi_dev_data->device_id)
		ipr_mpath_flush();

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	defect_list_hdr = calloc(1, IPR_DEFECT_LIST_HDR_LEN);

	cdb[0] = FORMAT_UNIT;
	cdb[1] = IPR_FORMAT_DATA; /* lun = 0, fmtdata = 1, cmplst = 0, defect list format = 0 */

	defect_list_hdr[1] = IPR_FORMAT_IMMED; /* FOV = 0, DPRY = 0, DCRT = 0, STPF = 0, IP = 0, DSP = 0, Immed = 1, VS = 0 */

	rc = sg_ioctl(fd, cdb, defect_list_hdr,
		      length, SG_DXFER_TO_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	free(defect_list_hdr);

	if (rc != 0)
		scsi_cmd_err(dev, &sense_data, "Format Unit", rc);

	close(fd);

	return rc;
}

/**
 * ipr_evaluate_device - 
 * @dev:		ipr dev struct
 * @res_handle:         resource handle
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_evaluate_device(struct ipr_dev *dev, u32 res_handle)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	u32 resource_handle;
	int length = 0;
	char *name = dev->gen_name;

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	resource_handle = ntohl(res_handle);

	cdb[0] = IPR_EVALUATE_DEVICE;
	cdb[2] = (u8)(resource_handle >> 24);
	cdb[3] = (u8)(resource_handle >> 16);
	cdb[4] = (u8)(resource_handle >> 8);
	cdb[5] = (u8)(resource_handle);

	rc = sg_ioctl(fd, cdb, NULL,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_EVALUATE_DEVICE_TIMEOUT);

	if (rc != 0 && sense_data.sense_key != ILLEGAL_REQUEST)
		scsi_cmd_err(dev, &sense_data, "Evaluate Device Capabilities", rc);

	close(fd);
	return rc;
}

/**
 * ipr_disrupt_device - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_disrupt_device(struct ipr_dev *dev)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	u32 res_handle;
	char *name = dev->ioa->ioa.gen_name;

	if (strlen(name) == 0)
		return -ENOENT;

	if (!dev->dev_rcd)
		return -EINVAL;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	res_handle = ntohl(dev->resource_handle);

	cdb[0] = IPR_IOA_SERVICE_ACTION;
	cdb[1] = IPR_DISRUPT_DEVICE;
	if (!ipr_force)
		cdb[2] = 0x40;
	cdb[6] = (u8)(res_handle >> 24);
	cdb[7] = (u8)(res_handle >> 16);
	cdb[8] = (u8)(res_handle >> 8);
	cdb[9] = (u8)(res_handle);

	scsi_info(dev, "Attempting to force device failure "
		  "with disrupt device command\n");
	rc = sg_ioctl(fd, cdb, NULL,
		      0, SG_DXFER_NONE,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0 && sense_data.sense_key != ILLEGAL_REQUEST)
		scsi_cmd_err(dev, &sense_data, "Disrupt Device", rc);

	close(fd);
	return rc;
}

/**
 * ipr_inquiry - issue an inquiry command
 * @dev:		ipr dev struct
 * @page:               mode page
 * @buff:		data buffer
 * @length              buffer length
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_inquiry(struct ipr_dev *dev, u8 page, void *buff, u8 length)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	char *name = dev->gen_name;

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = INQUIRY;
	if (page != IPR_STD_INQUIRY) {
		cdb[1] = 0x01;
		cdb[2] = page;
	}
	cdb[4] = length;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0 && (ipr_debug || sense_data.sense_key != ILLEGAL_REQUEST))
		scsi_cmd_err(dev, &sense_data, "Inquiry", rc);

	close(fd);
	return rc;
}

/**
 * __ipr_get_fw_version - get the firmware version
 * @dev:		ipr dev struct
 * @pag3_ing:           page 3 inquiry data
 * @release_level:	location to copy version info into
 *
 * Returns:
 *   nothing
 **/
static void __ipr_get_fw_version(struct ipr_dev *dev,
				 struct ipr_dasd_inquiry_page3 *page3_inq, u8 release_level[4])
{
	if (ipr_is_ses(dev) && !__ioa_is_spi(dev->ioa))
		memcpy(release_level, page3_inq->load_id, 4);
	else
		memcpy(release_level, page3_inq->release_level, 4);
}

/**
 * ipr_get_fw_version - get the firmware version entry point
 * @dev:		ipr dev struct
 * @release_level:	location to copy version info into
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_get_fw_version(struct ipr_dev *dev, u8 release_level[4])
{
	struct ipr_dasd_inquiry_page3 page3_inq;
	int rc;

	memset(&page3_inq, 0, sizeof(page3_inq));
	rc = ipr_inquiry(dev, 3, &page3_inq, sizeof(page3_inq));

	if (rc)
		return rc;

	__ipr_get_fw_version(dev, &page3_inq, release_level);
	return 0;
}

/**
 * ipr_init_res_addr_aliases - 
 * @aliases:		ipr_res_addr_aliases struct
 * @res_addr:		ipr_res_addr struct
 *
 * Returns:
 *   nothing
 **/
static void ipr_init_res_addr_aliases(struct ipr_res_addr_aliases *aliases,
				      struct ipr_res_addr *res_addr)
{
	struct ipr_res_addr *ra;

	aliases->length = htonl(sizeof(*res_addr) * IPR_DEV_MAX_PATHS);
	for_each_ra_alias(ra, aliases)
		memcpy(ra, res_addr, sizeof(*ra));
}

/**
 * ipr_query_res_addr_aliases - 
 * @ioa:		ipr ioa struct
 * @res_addr:		ipr_res_addr struct
 * @aliases:		ipr_res_addr_aliases struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_query_res_addr_aliases(struct ipr_ioa *ioa, struct ipr_res_addr *res_addr,
			       struct ipr_res_addr_aliases *aliases)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	char *name = ioa->ioa.gen_name;
	struct ipr_res_addr *ra;

	ipr_init_res_addr_aliases(aliases, res_addr);
	for_each_ra_alias(ra, aliases)
		ra->host = ioa->host_num;

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_IOA_SERVICE_ACTION;
	cdb[1] = IPR_QUERY_RES_ADDR_ALIASES;
	cdb[3] = res_addr->bus;
	cdb[4] = res_addr->target;
	cdb[5] = res_addr->lun;
	cdb[10] = sizeof(*aliases) >> 24;
	cdb[11] = (sizeof(*aliases) >> 16) & 0xff;
	cdb[12] = (sizeof(*aliases) >> 8) & 0xff;
	cdb[13] = sizeof(*aliases) & 0xff;

	rc = sg_ioctl(fd, cdb, aliases,
		      sizeof(*aliases), SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc || (ntohl(aliases->length) <= 4))
		ipr_init_res_addr_aliases(aliases, res_addr);
	if (rc != 0 && sense_data.sense_key != ILLEGAL_REQUEST)
		ioa_cmd_err(ioa, &sense_data, "Query Resource Address Aliases", rc);
	if (rc && sense_data.sense_key != ILLEGAL_REQUEST)
		rc = 0;

	for_each_ra_alias(ra, aliases)
		ra->host = ioa->host_num;

	close(fd);
	return rc;
}

/**
 * ipr_init_res_path_aliases -
 * @aliases:		ipr_res_path_aliases struct
 * @res_addr:		ipr_res_path
 *
 * Returns:
 *   nothing
 **/
static void ipr_init_res_path_aliases(struct ipr_res_path_aliases *aliases,
				      struct ipr_res_path *res_path)
{
	struct ipr_res_path *rp;

	aliases->length = htonl(sizeof(*res_path) * IPR_DEV_MAX_PATHS);
	for_each_rp_alias(rp, aliases) {
		memcpy(rp, res_path, sizeof(*rp));
	}
}

/**
 * ipr_query_res_path_aliases -
 * @ioa:		ipr ioa struct
 * @res_addr:		ipr_res_path struct
 * @aliases:		ipr_res_ipr_aliases struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_query_res_path_aliases(struct ipr_ioa *ioa, struct ipr_res_path *res_path,
			       struct ipr_res_path_aliases *aliases)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	char *name = ioa->ioa.gen_name;

	ipr_init_res_path_aliases(aliases, res_path);

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_IOA_SERVICE_ACTION;
	cdb[1] = IPR_QUERY_RES_PATH_ALIASES;

	memcpy(&cdb[2], res_path->res_path_bytes, sizeof(struct ipr_res_path));
	cdb[10] = sizeof(*aliases) >> 24;
	cdb[11] = (sizeof(*aliases) >> 16) & 0xff;
	cdb[12] = (sizeof(*aliases) >> 8) & 0xff;
	cdb[13] = sizeof(*aliases) & 0xff;

	rc = sg_ioctl(fd, cdb, aliases,
		      sizeof(*aliases), SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc || (ntohl(aliases->length) <= 8))
		ipr_init_res_path_aliases(aliases, res_path);
	if (rc != 0 && sense_data.sense_key != ILLEGAL_REQUEST)
		ioa_cmd_err(ioa, &sense_data, "Query Resource Path Aliases", rc);
	if (rc && sense_data.sense_key != ILLEGAL_REQUEST)
		rc = 0;

	close(fd);

	return rc;
}

/**
 * ipr_query_sas_expander_info - 
 * @ioa:		ipr ioa struct
 * @info:		ipr_query_sas_expander_info struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_query_sas_expander_info(struct ipr_ioa *ioa,
				struct ipr_query_sas_expander_info *info)
{
	char *name = ioa->ioa.gen_name;
	struct sense_data_t sense_data;
	u8 cdb[IPR_CCB_CDB_LEN];
	int fd, rc;

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_IOA_SERVICE_ACTION;
	cdb[1] = IPR_QUERY_SAS_EXPANDER_INFO;
	cdb[10] = sizeof(*info) >> 24;
	cdb[11] = (sizeof(*info) >> 16) & 0xff;
	cdb[12] = (sizeof(*info) >> 8) & 0xff;
	cdb[13] = sizeof(*info) & 0xff;

	rc = sg_ioctl(fd, cdb, info, sizeof(*info), SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0 && sense_data.sense_key != ILLEGAL_REQUEST)
		ioa_cmd_err(ioa, &sense_data, "Query SAS Expander Info", rc);

	close(fd);
	return rc;	
}

/**
 * ipr_change_cache_parameters
 * @ioa:		ipr ioa struct
 * @mode:		caching mode to set
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_change_cache_parameters(struct ipr_ioa *ioa, int mode)
{
	char *name = ioa->ioa.gen_name;
	struct sense_data_t sense_data;
	u8 cdb[IPR_CCB_CDB_LEN];
	int fd, rc;

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_IOA_SERVICE_ACTION;
	cdb[1] = IPR_CHANGE_CACHE_PARAMETERS;
	cdb[2] = mode;

	rc = sg_ioctl(fd, cdb, NULL, 0, SG_DXFER_NONE,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0 && (sense_data.sense_key != ILLEGAL_REQUEST || ipr_debug))
		ioa_cmd_err(ioa, &sense_data, "change cache params failed", rc);

	close(fd);
	return rc;
}

/**
 * ipr_query_cache_parameters -
 * @ioa:		ipr ioa struct
 * @buf:		buffer for returned cache data
 * @len:		length of buffer
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_query_cache_parameters(struct ipr_ioa *ioa, void *buf, int len)
{
	char *name = ioa->ioa.gen_name;
	struct sense_data_t sense_data;
	u8 cdb[IPR_CCB_CDB_LEN];
	int fd, rc;

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_IOA_SERVICE_ACTION;
	cdb[1] = IPR_QUERY_CACHE_PARAMETERS;
	cdb[10] = len >> 24;
	cdb[11] = len >> 16 & 0xff;
	cdb[12] = len >> 8 & 0xff;
	cdb[13] = len & 0xff;

	rc = sg_ioctl(fd, cdb, buf, len, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0 && (sense_data.sense_key != ILLEGAL_REQUEST || ipr_debug))
		ioa_cmd_err(ioa, &sense_data, "Query Cache Parameters", rc);

	close(fd);
	return rc;
}

/**
 * ipr_query_res_redundancy_info - 
 * @dev:		ipr dev struct
 * @info:		ipr_res_redundancy_info struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_query_res_redundancy_info(struct ipr_dev *dev,
				  struct ipr_res_redundancy_info *info)
{
	struct scsi_dev_data *scsi_dev_data = dev->scsi_dev_data;
	char *name = dev->ioa->ioa.gen_name;
	struct ipr_dev_record *dev_record;
	struct sense_data_t sense_data;
	u8 cdb[IPR_CCB_CDB_LEN];
	u32 res_handle;
	int fd, rc;

	if (scsi_dev_data)
		res_handle = ntohl(scsi_dev_data->handle);
	else if (ipr_is_af_dasd_device(dev)) {
		dev_record = dev->dev_rcd;
		if (dev_record->no_cfgte_dev) 
			return -EINVAL;

		res_handle = ntohl(dev->resource_handle);
	} else
		return -EINVAL;

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_IOA_SERVICE_ACTION;
	if (dev->ioa->sis64)
		cdb[1] = IPR_QUERY_RES_REDUNDANCY_INFO64;
	else
		cdb[1] = IPR_QUERY_RES_REDUNDANCY_INFO;
	cdb[2] = (u8)(res_handle >> 24);
	cdb[3] = (u8)(res_handle >> 16);
	cdb[4] = (u8)(res_handle >> 8);
	cdb[5] = (u8)(res_handle);
	cdb[10] = sizeof(*info) >> 24;
	cdb[11] = (sizeof(*info) >> 16) & 0xff;
	cdb[12] = (sizeof(*info) >> 8) & 0xff;
	cdb[13] = sizeof(*info) & 0xff;

	rc = sg_ioctl(fd, cdb, info, sizeof(*info), SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0 && sense_data.sense_key != ILLEGAL_REQUEST)
		ioa_cmd_err(dev->ioa, &sense_data, "Query Resource Redundancy Info", rc);

	close(fd);
	return rc;
}

/**
 * get_livedump_fname - 
 * @ioa:		ipr_ioa struct
 * @fname:		string to return the filename
 * @max:		max size of fname
 *
 * Returns:
 *   negative on failure / otherwise length of the file name
 *
 **/
static int get_livedump_fname(struct ipr_ioa *ioa, char *fname, int max)
{
	time_t cur_time, rc;
	struct tm *cur_tm;
	char time_str[100];
	int len;

	fname[0] = '\0';
	rc = time(&cur_time);
	if (rc == ((time_t)-1))
		return -EIO;

	cur_tm = localtime(&cur_time);
	if (!cur_tm)
		return -EIO;
	strftime(time_str, sizeof(time_str), "%Y%m%d.%H%M", cur_tm);

	len = snprintf(fname, max, "ipr-%04X-%s-dump-%s", ioa->ccin,
		       ioa->pci_address, time_str);

	return len;
}

#define IPR_MAX_LIVE_DUMP_SIZE (16 * 1024 * 1024)

/**
 * get_scsi_max_xfer_len -
 * @fd:			IOA sg file descriptor
 *
 * Returns:
 *   -1 on failure / otherwise max scatter-gather transfer length
 *
 **/
static int get_scsi_max_xfer_len(int fd)
{
	int rc, max_tablesize, max_scsi_len;
	char elem_str[64];
	ssize_t len;

	len = sysfs_read_attr("/sys/module/sg/parameters", "scatter_elem_sz",
			      elem_str, 64);
	if (len < 0) {
		syslog_dbg("Failed to read scatter_elem_sz parameter from sg module. %m\n");
		return -1;
	}

	rc = ioctl(fd, SG_GET_SG_TABLESIZE, &max_tablesize);
	if (rc) {
		syslog_dbg("Unable to get device scatter-gather table size. %m\n");
		return -1;
	}

	max_scsi_len = max_tablesize * atoi(elem_str);

	if (max_scsi_len < IPR_MAX_LIVE_DUMP_SIZE)
		return max_scsi_len;
	else
		return IPR_MAX_LIVE_DUMP_SIZE;
}

/**
 * ipr_get_live_dump - 
 * @dev:		ipr dev struct
 * @info:		ipr_res_redundancy_info struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 *
 **/
int ipr_get_live_dump(struct ipr_ioa *ioa)
{
	char *name = ioa->ioa.gen_name;
	struct sense_data_t sense_data;
	u8 cdb[IPR_CCB_CDB_LEN], *buf;
	char dump_file[100], dump_path[100];
	int rc, fd, fd_dump, buff_len;

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	buff_len = get_scsi_max_xfer_len(fd);
	if (buff_len <= 0) {
		rc = -EPERM;
		goto out;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);
	cdb[0] = IPR_IOA_SERVICE_ACTION;
	cdb[1] = IPR_LIVE_DUMP;
	cdb[10] = buff_len >> 24;
	cdb[11] = (buff_len >> 16) & 0xff;
	cdb[12] = (buff_len >> 8) & 0xff;
	cdb[13] = buff_len & 0xff;

	buf = (u8*) malloc(buff_len);

	rc = sg_ioctl(fd, cdb, buf, buff_len, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc) {
		ioa_cmd_err(ioa, &sense_data, "Live Dump", rc);
		goto out_free;
	}

	rc = get_livedump_fname(ioa, dump_file, sizeof(dump_file));
	if (rc < 0) {
		syslog_dbg("Failed to create a live dump file name.\n");
		goto out_free;
	}

	snprintf(dump_path, sizeof(dump_path), "%s%s", IPRDUMP_DIR, dump_file);
	fd_dump = creat(dump_path, S_IRUSR);
	if (fd_dump < 0) {
		syslog(LOG_ERR, "Could not open %s. %m\n", dump_path);
		rc = fd_dump;
		goto out_free;
	}

	rc = write(fd_dump, buf, buff_len);
	if (rc != buff_len)
		syslog(LOG_ERR, "Could not write dump on file %s. %m\n", dump_path);

	close(fd_dump);

out_free:
	free(buf);
out:
	close(fd);
	return rc;
}

/**
 * ipr_set_array_asym_access -
 * @dev:		ipr_dev struct
 * @qac:		ipr_array_query_data struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_set_array_asym_access(struct ipr_ioa *ioa)
{
	char *name = ioa->ioa.gen_name;
	struct sense_data_t sense_data;
	u8 cdb[IPR_CCB_CDB_LEN];
	int fd, rc;
	int length = ioa->qac_data->resp_len;

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_SET_ARRAY_ASYMMETRIC_ACCESS;
	cdb[7] = (length >> 8) & 0xff;
	cdb[8] = length & 0xff;

	if (ioa->sis64) {
		cdb[10] = (length & 0xff000000) >> 24;
		cdb[11] = (length & 0xff0000) >> 16;
	}
	rc = sg_ioctl(fd, cdb, ioa->qac_data->u.buf, length, SG_DXFER_TO_DEV,
		      &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if (rc != 0 && (sense_data.sense_key != ILLEGAL_REQUEST || ipr_debug))
		ioa_cmd_err(ioa, &sense_data, "Set Array Asymmetric Access", rc);

	close(fd);
	return rc;
}

/**
 * get_write_buffer_dev - FIXME - probably need to remove this routine.
 * @dev:		ipr dev struct
 *
 * Returns:
 *   device generic name for now - see FIXME below
 **/
#if 0
static char *get_write_buffer_dev(struct ipr_dev *dev)
{
	struct sysfs_class_device *class_device;
	struct sysfs_attribute *dev_attr;
	int rc, val;
	char path[SYSFS_PATH_MAX];

	/* xxx FIXME */
	return dev->gen_name;

	if (!strlen(dev->dev_name) || !dev->scsi_dev_data)
		return dev->gen_name;

	class_device = sysfs_open_class_device("block",
					       &dev->dev_name[5]);
	if (!class_device) {
		scsi_dbg(dev, "Failed to open block class device for %s. %m\n",
			 dev->dev_name);
		return dev->gen_name;
	}

	sprintf(path, "%s/queue/max_sectors_kb", class_device->path);

	dev_attr = sysfs_open_attribute(path);
	if (!dev_attr) {
		scsi_dbg(dev, "Failed to get max_sectors_kb attribute. %m\n");
		goto fail;
	}

	rc = sysfs_read_attribute(dev_attr);

	if (rc) {
		sysfs_close_attribute(dev_attr);
		scsi_dbg(dev, "Failed to read max_sectors_kb attribute. %m\n");
		goto fail;
	}

	sscanf(dev_attr->value, "%d", &val);
	sysfs_close_attribute(dev_attr);
	sysfs_close_class_device(class_device);

	if (val > 256)
		return dev->dev_name;

	return dev->gen_name;	
fail:
	sysfs_close_class_device(class_device);
	return dev->gen_name;
}
#endif

/**
 * ipr_resume_device_bus - 
 * @ioa:		ipr ioa struct
 * @res_addr:		ipr_res_addr struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_resume_device_bus(struct ipr_dev *dev,
				 struct ipr_res_addr *res_addr)
{
	int fd, rc, cdb_num;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int length = 0;
	struct ipr_ioa *ioa = dev->ioa;
	char *rp, *endptr;

	if (strlen(ioa->ioa.gen_name) == 0)
		return -ENOENT;

	fd = open(ioa->ioa.gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", ioa->ioa.gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_RESUME_DEV_BUS;
	cdb[1] = IPR_RDB_UNQUIESCE_AND_REBALANCE;
	if (ioa->sis64) {
		/* convert res path string to bytes */
		cdb_num = 2;
		rp = dev->res_path_name;
		do {
			cdb[cdb_num++] = (u8)strtol(rp, &endptr, 16);
			rp = endptr+1;
		} while (*endptr != '\0' && cdb_num < 10);

		while (cdb_num < 10) cdb[cdb_num++] = 0xff;
	} else {
		cdb[3] = res_addr->bus;
		cdb[4] = res_addr->target;
		cdb[5] = res_addr->lun;
	}

	rc = sg_ioctl(fd, cdb, NULL,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_SUSPEND_DEV_BUS_TIMEOUT);

	close(fd);
	return rc;
}

/**
 * __ipr_write_buffer - Issue WRITE_BUFFER command.
 * 			In older kernels, max_sectors_kb is not big enough and
 * 			would return -EIO when using dev_name to issue
 * 			SG_IO ioctl(). In that case, we have to use gen_name.
 * 			Also, in case dev_name does not exist, we need to use
 * 			gen_name. Hence, we try dev_name first and switch to
 * 			gen_name if we get -ENOENT or -EIO.
 * @dev:		ipr dev struct
 * @mode:       	mode value
 * @buff:		data buffer
 * @length:             buffer length
 * @sense_data:         sense data struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int __ipr_write_buffer(struct ipr_dev *dev, u8 mode, void *buff,
			      int offset, int length, struct sense_data_t *sense_data)
{
	u32 direction = length ? SG_DXFER_TO_DEV : SG_DXFER_NONE;
	u8 cdb[IPR_CCB_CDB_LEN];
	int rc;

	ENTER;
	if ((strlen(dev->dev_name) == 0) && (strlen(dev->gen_name) == 0))
		return -ENOENT;

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = WRITE_BUFFER;
	cdb[1] = mode;
	cdb[3] = (offset & 0xff0000) >> 16;
	cdb[4] = (offset & 0x00ff00) >> 8;
	cdb[5] = (offset & 0x0000ff);
	cdb[6] = (length & 0xff0000) >> 16;
	cdb[7] = (length & 0x00ff00) >> 8;
	cdb[8] = length & 0x0000ff;

	rc = sg_ioctl_by_name(dev->dev_name, cdb, buff,
		      length, direction,
		      sense_data, IPR_WRITE_BUFFER_TIMEOUT);

	if (rc != 0) {
		if ((rc == -ENOENT) || (rc == -EIO) || (rc == -EINVAL)) {
			syslog_dbg("Write buffer failed on sd device %s. %m\n", dev->dev_name);
			rc = sg_ioctl_by_name(dev->gen_name, cdb, buff,
					      length, direction,
					      sense_data, IPR_WRITE_BUFFER_TIMEOUT);
			if (rc != 0) {
				scsi_cmd_err(dev, sense_data,
					     "Write buffer using sg device", rc);
				if (rc == -ENOMEM)
					syslog(LOG_ERR,
					       "Cannot get enough memory to "
					       "perform microcode download "
					       "through %s. Reduce system "
					       "memory usage and try again.\n",
					       dev->gen_name);
			}
		} else
			scsi_cmd_err(dev, sense_data,
				     "Write buffer using sd device", rc);
	}

	return rc;
}

/**
 * ipr_write_buffer - 
 * @dev:		ipr dev struct
 * @buff:	        data buffer
 * @length:		buffer length
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_write_buffer(struct ipr_dev *dev, void *buff, int length)
{
	int rc, i;
	struct sense_data_t sense_data;
	struct ipr_disk_attr attr;
	int old_qdepth;
	int sas_ses = 0;
	int sas_ses_retries = 5;
	int mode5 = 1;
	u32 write_len = 0;
	u32 offset;
	u32 write_buffer_chunk_sz = length;

	ENTER;
	if ((rc = ipr_get_dev_attr(dev, &attr))) {
		scsi_dbg(dev, "Failed to get device attributes. rc=%d\n", rc);
		return rc;
	}

	if (ipr_is_ses(dev) && !__ioa_is_spi(dev->ioa))
		sas_ses = 1;
	if (sas_ses && !ipr_mode5_write_buffer)
		mode5 = 0;

	/*
	 * Set the queue depth to 1 for the duration of the code download.
	 * This prevents us from getting I/O errors during the code update
	 */
	old_qdepth = attr.queue_depth;
	attr.queue_depth = 1;
	if ((rc = ipr_set_dev_attr(dev, &attr, 0))) {
		scsi_dbg(dev, "Failed to set queue depth to 1. rc=%d\n", rc);
		return rc;
	}

	if (sas_ses) {
		for (offset = 0; offset < length && !mode5; offset += write_len) {
			write_len = write_buffer_chunk_sz;
			if (offset + write_len > length)
				write_len = length - offset;

			for (i = 0; i < (sas_ses_retries + 1); i++) {
				rc = __ipr_write_buffer(dev, 0x0e, buff + offset, offset,
							write_len, &sense_data);
				if (!rc) {
					break;
				} else if (rc && sense_data.sense_key == UNIT_ATTENTION &&
					   sense_data.add_sense_code == 0x29 &&
					   sense_data.add_sense_code_qual == 0x00) {
					continue;
				} else if (rc && sense_data.sense_key == ILLEGAL_REQUEST) {
					if (write_buffer_chunk_sz == 4096) {
						mode5 = 1;
					} else {
						/* Attempt 4k write */
						write_buffer_chunk_sz = 4096;
						write_len = 0;
					}
					break;
				} else if (rc) {
					goto out;
				}
			}
		}

		rc = ipr_suspend_device_bus(dev, &dev->res_addr[0],
					    IPR_SDB_CHECK_AND_QUIESCE_ENC);

		if (rc) {
			scsi_dbg(dev, "Failed to quiesce the SAS port.\n");
			goto out;
		}

		for (i = 0; i < (sas_ses_retries + 1); i++) {
			if (mode5)
				rc = __ipr_write_buffer(dev, 5, buff, 0, length, &sense_data);
			else
				rc = __ipr_write_buffer(dev, 0x0f, NULL, 0, 0, &sense_data);

			if (rc && sense_data.sense_key == UNIT_ATTENTION &&
				   sense_data.add_sense_code == 0x29 &&
				   sense_data.add_sense_code_qual == 0x00) {
				continue;
			} else {
				break;
			}
		}

		if (rc) {
			scsi_dbg(dev, "Write buffer %sfailed.\n", mode5 ? "" : "activate ");
			goto out_resume;
		}
	} else {
		if ((rc = __ipr_write_buffer(dev, 5, buff, 0, length, &sense_data)))
			goto out;
	}

	scsi_dbg(dev, "Waiting for device to come ready after download.\n");
	sleep(sas_ses ? 120 : 5);

	for (i = 0, rc = -1; rc && (i < 60); i++, sleep(1))
		rc = __ipr_test_unit_ready(dev, &sense_data);

out_resume:
	if (sas_ses)
		ipr_resume_device_bus(dev, &dev->res_addr[0]);

out:
	attr.queue_depth = old_qdepth;
	ipr_set_dev_attr(dev, &attr, 0);
	LEAVE;
	return rc;
}

/**
 * ipr_suspend_device_bus - 
 * @ioa:		ipr ioa struct
 * @res_addr:	        ipr_res_addr struct
 * @option:		option value
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_suspend_device_bus(struct ipr_dev *dev,
			   struct ipr_res_addr *res_addr, u8 option)
{
	int fd, rc, cdb_num;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int length = 0;
	struct ipr_ioa *ioa = dev->ioa;
	char *rp, *endptr;

	ENTER;
	if (strlen(ioa->ioa.gen_name) == 0)
		return -ENOENT;

	fd = open(ioa->ioa.gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", ioa->ioa.gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_SUSPEND_DEV_BUS;
	cdb[1] = option;
	if (ioa->sis64) {
		/* convert res path string to bytes */
		cdb_num = 2;
		rp = dev->res_path_name;
		do {
			cdb[cdb_num++] = (u8)strtol(rp, &endptr, 16);
			rp = endptr+1;
		} while (*endptr != '\0' && cdb_num < 10);

		while (cdb_num < 10) cdb[cdb_num++] = 0xff;
	} else {
		cdb[3] = res_addr->bus;
		cdb[4] = res_addr->target;
		cdb[5] = res_addr->lun;
	}

	rc = sg_ioctl(fd, cdb, NULL,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_SUSPEND_DEV_BUS_TIMEOUT);

	close(fd);
	LEAVE;
	return rc;
}

/**
 * ipr_can_remove_device - indicate whether or not a device can be removed
 * @dev:		ipr dev struct
 *
 * Returns:
 *   1 if device can be removed / 0 if device can not be removed
 **/
int ipr_can_remove_device(struct ipr_dev *dev)
{
	struct ipr_res_addr *ra;

	for_each_ra(ra, dev) {
		if (ipr_suspend_device_bus(dev, ra, IPR_SDB_CHECK_ONLY))
			return 0;
	}

	return 1;
}

/**
 * ipr_receive_diagnostics - 
 * @dev:		ipr dev struct
 * @page:	        page number
 * @buff:		data buffer
 * @length:             buffer length
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_receive_diagnostics(struct ipr_dev *dev,
			    u8 page, void *buff, int length)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;
	char *name = dev->gen_name;

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = RECEIVE_DIAGNOSTIC;
	cdb[1] = 0x01;
	cdb[2] = page;
	cdb[3] = (length >> 8) & 0xff;
	cdb[4] = length & 0xff;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0)
		scsi_cmd_err(dev, &sense_data, "Receive diagnostics", rc);

	close(fd);
	return rc;
}

/**
 * ipr_send_diagnostics - 
 * @dev:		ipr dev struct
 * @buff:		data buffer
 * @length:             buffer length
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_send_diagnostics(struct ipr_dev *dev, void *buff, int length)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;
	char *name = dev->gen_name;

	if (strlen(name) == 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = SEND_DIAGNOSTIC;
	cdb[1] = 0x10;
	cdb[3] = (length >> 8) & 0xff;
	cdb[4] = length & 0xff;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_TO_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0)
		scsi_cmd_err(dev, &sense_data, "Send diagnostics", rc);

	close(fd);
	return rc;
}

/**
 * set_supported_devs - 
 * @dev:		ipr dev struct
 * @std_inq:		ipr_std_inq_data struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_supported_devs(struct ipr_dev *dev,
		       struct ipr_std_inq_data *std_inq)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	struct ipr_supported_device supported_dev;

	fd = open(dev->ioa->ioa.gen_name, O_RDWR);

	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n",
			       dev->ioa->ioa.gen_name);
		return errno;
	}

	memset(&supported_dev, 0, sizeof(struct ipr_supported_device));
	memcpy(&supported_dev.vpids, &std_inq->vpids, sizeof(std_inq->vpids));
	supported_dev.num_records = 1;
	supported_dev.data_length = htons(sizeof(supported_dev));
	supported_dev.reserved = 0;

	memset(cdb, 0, IPR_CCB_CDB_LEN);
	cdb[0] = SET_SUPPORTED_DEVICES;
	cdb[7] = (sizeof(supported_dev) >> 8) & 0xff;
	cdb[8] = sizeof(supported_dev) & 0xff;

	rc = sg_ioctl(fd, cdb, &supported_dev,
		      sizeof(supported_dev), SG_DXFER_TO_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0)
		ioa_cmd_err(dev->ioa, &sense_data, "Set supported devices", rc);

	close(fd);
	return rc;
}

/**
 * __device_supported - 
 * @dev:		ipr dev struct
 * @inq:		ipr_std_inq_data
 *
 * Returns:
 *   0 if device is supported / 1 if device is not supported
 **/
static int __device_supported(struct ipr_dev *dev, struct ipr_std_inq_data *inq)
{
	int i, j;
	const struct ses_table_entry *ste;

	if (!dev->scsi_dev_data)
		return 1;

	ste = get_ses_entry(dev->ioa, dev->scsi_dev_data->channel);

	if (!ste)
		return 1;
	if (!ste->block_15k_devices)
		return 1;

	for (i = 0; i < ARRAY_SIZE(unsupported_dasd); i++) {
		for (j = 0; j < IPR_VENDOR_ID_LEN; j++) {
			if (unsupported_dasd[i].compare_vendor_id_byte[j] &&
			    unsupported_dasd[i].vendor_id[j] != inq->vpids.vendor_id[j])
				break;
		}

		if (j != IPR_VENDOR_ID_LEN)
			continue;

		for (j = 0; j < IPR_PROD_ID_LEN; j++) {
			if (unsupported_dasd[i].compare_product_id_byte[j] &&
			    unsupported_dasd[i].product_id[j] != inq->vpids.product_id[j])
				break;
		}

		if (j != IPR_PROD_ID_LEN)
			continue;

		return 0;
	}
	return 1;
}

/**
 * device_supported - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   0 if device is supported / 1 if device is not supported
 **/
int device_supported(struct ipr_dev *dev)
{
	struct ipr_std_inq_data std_inq;

	if (ipr_inquiry(dev, IPR_STD_INQUIRY, &std_inq, sizeof(std_inq)))
		return -EIO;

	return __device_supported(dev, &std_inq);
}

/**
 * enable_af - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int enable_af(struct ipr_dev *dev)
{
	struct ipr_std_inq_data std_inq;

	if (ipr_inquiry(dev, IPR_STD_INQUIRY, &std_inq, sizeof(std_inq)))
		return -EIO;

	if (!__device_supported(dev, &std_inq)) {
		scsi_dbg(dev, "Unsupported device attached\n");
		return -EIO;
	}

	if (set_supported_devs(dev, &std_inq))
		return -EIO;
	return 0;
}

/**
 * get_blk_size - return the block size of the device
 * @dev:		ipr dev struct
 *
 * Returns:
 *   block size / non-zero on failure
 **/
static int get_blk_size(struct ipr_dev *dev)
{
	struct ipr_mode_pages modes;
	struct ipr_block_desc *block_desc;
	int rc;

	memset(&modes, 0, sizeof(modes));

	rc = ipr_mode_sense(dev, 0x0a, &modes);

	if (!rc) {
		if (modes.hdr.block_desc_len > 0) {
			block_desc = (struct ipr_block_desc *)(modes.data);
			return ((block_desc->block_length[1] << 8) | block_desc->block_length[2]);
		}
	}

	return -EIO;
}

/**
 * format_req - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int format_req(struct ipr_dev *dev)
{
	struct sense_data_t sense_data;
	int rc;

	rc = ipr_test_unit_ready(dev, &sense_data);

	if (rc == CHECK_CONDITION &&
	    sense_data.add_sense_code == 0x31 &&
	    sense_data.add_sense_code_qual == 0x00)
		return 1;

	rc = get_blk_size(dev);

	if (rc < 0)
		return 0;

	if (ipr_is_gscsi(dev) && (rc != 512 || rc != 4096))
		return 1;

	return 0;
}

/*
 * Scatter/gather list buffers are checked against the value returned
 * by queue_dma_alignment(), which defaults to 511 in Linux 2.6,
 * for alignment if a SG_IO ioctl request is sent through a /dev/sdX device.
 */
#define IPR_S_G_BUFF_ALIGNMENT 512
#define IPR_MAX_XFER 0x8000

const int cdb_size[] ={6, 10, 10, 0, 16, 12, 16, 16};
/**
 * _sg_ioctl - 
 * @fd: 		file descriptor
 * @cdb:        	cdb
 * @data:		data pointer
 * @xfer_len            transfer length
 * @data_direction      transfer to dev or from dev
 * @sense_data          sense data pointer
 * @timeout_in_sec      timeout value
 * @retries             number of retries
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int _sg_ioctl(int fd, u8 cdb[IPR_CCB_CDB_LEN],
		     void *data, u32 xfer_len, u32 data_direction,
		     struct sense_data_t *sense_data,
		     u32 timeout_in_sec, int retries)
{
	int rc = 0;
	sg_io_hdr_t io_hdr_t;
	sg_iovec_t *iovec = NULL;
	int iovec_count = 0;
	int i;
	int buff_len, segment_size;
	void *dxferp;
	u8 *buf;

	/* check if scatter gather should be used */
	if (xfer_len > IPR_MAX_XFER) {
		iovec_count = (xfer_len/IPR_MAX_XFER) + 1;
		iovec = malloc(iovec_count * sizeof(sg_iovec_t));

		buff_len = xfer_len;
		segment_size = IPR_MAX_XFER;

		for (i = 0; (i < iovec_count) && (buff_len != 0); i++) {
			posix_memalign(&(iovec[i].iov_base), IPR_S_G_BUFF_ALIGNMENT, segment_size);
			if (data_direction == SG_DXFER_TO_DEV)
				memcpy(iovec[i].iov_base, data + (IPR_MAX_XFER * i), segment_size);
			iovec[i].iov_len = segment_size;

			buff_len -= segment_size;
			if (buff_len < segment_size)
				segment_size = buff_len;
		}

		iovec_count = i;
		dxferp = (void *)iovec;
	} else {
		iovec_count = 0;
		dxferp = data;
	}

	for (i = 0; i < (retries + 1); i++) {
		memset(&io_hdr_t, 0, sizeof(io_hdr_t));
		memset(sense_data, 0, sizeof(struct sense_data_t));

		io_hdr_t.interface_id = 'S';
		io_hdr_t.cmd_len = cdb_size[(cdb[0] >> 5) & 0x7];
		io_hdr_t.iovec_count = iovec_count;
		io_hdr_t.flags = 0;
		io_hdr_t.pack_id = 0;
		io_hdr_t.usr_ptr = 0;
		io_hdr_t.sbp = (unsigned char *)sense_data;
		io_hdr_t.mx_sb_len = sizeof(struct sense_data_t);
		io_hdr_t.timeout = timeout_in_sec * 1000;
		io_hdr_t.cmdp = cdb;
		io_hdr_t.dxfer_direction = data_direction;
		io_hdr_t.dxfer_len = xfer_len;
		io_hdr_t.dxferp = dxferp;

		rc = ioctl(fd, SG_IO, &io_hdr_t);

		if (rc == -1 && errno == EINVAL) {
			rc = -EINVAL;
			goto out;
		}

		if (rc == 0 && io_hdr_t.masked_status == CHECK_CONDITION)
			rc = CHECK_CONDITION;
		else if (rc == 0 && (io_hdr_t.host_status || io_hdr_t.driver_status))
			rc = -EIO;

		if (rc == 0 || io_hdr_t.host_status == 1)
			break;
	}

out:
	if (iovec_count) {
		for (i = 0, buf = (u8 *)data; i < iovec_count; i++) {
			if (data_direction == SG_DXFER_FROM_DEV)
				memcpy(buf, iovec[i].iov_base, iovec[i].iov_len);
			buf += iovec[i].iov_len;
			free(iovec[i].iov_base);
		}
		free(iovec);
	}

	return rc;
};

/**
 * sg_ioctl - 
 * @fd: 		file descriptor
 * @cdb:        	cdb
 * @data:		data pointer
 * @xfer_len            transfer length
 * @data_direction      transfer to dev or from dev
 * @sense_data          sense data pointer
 * @timeout_in_sec      timeout value
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int sg_ioctl(int fd, u8 cdb[IPR_CCB_CDB_LEN],
	     void *data, u32 xfer_len, u32 data_direction,
	     struct sense_data_t *sense_data,
	     u32 timeout_in_sec)
{
	return _sg_ioctl(fd, cdb,
			 data, xfer_len, data_direction,
			 sense_data, timeout_in_sec, 1);
};

/**
 * sg_ioctl_noretry - 
 * @fd: 		file descriptor
 * @cdb:        	cdb
 * @data:		data pointer
 * @xfer_len            transfer length
 * @data_direction      transfer to dev or from dev
 * @sense_data          sense data pointer
 * @timeout_in_sec      timeout value
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int sg_ioctl_noretry(int fd, u8 cdb[IPR_CCB_CDB_LEN],
		     void *data, u32 xfer_len, u32 data_direction,
		     struct sense_data_t *sense_data,
		     u32 timeout_in_sec)
{
	return _sg_ioctl(fd, cdb,
			 data, xfer_len, data_direction,
			 sense_data, timeout_in_sec, 0);
};

/**
 * sg_ioctl_by_name - 
 * @name: 		file name
 * @cdb:        	cdb
 * @data:		data pointer
 * @xfer_len            transfer length
 * @data_direction      transfer to dev or from dev
 * @sense_data          sense data pointer
 * @timeout_in_sec      timeout value
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int sg_ioctl_by_name(char *name, u8 cdb[IPR_CCB_CDB_LEN],
			    void *data, u32 xfer_len, u32 data_direction,
			    struct sense_data_t *sense_data,
			    u32 timeout_in_sec)
{
	int fd, rc;

	if (strlen(name) <= 0)
		return -ENOENT;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s.\n", name);
		return errno;
	}

	rc = sg_ioctl(fd, cdb, data,
		      xfer_len, data_direction,
		      sense_data, timeout_in_sec);
	close(fd);

	return rc;
}

/**
 * ipr_set_ha_mode - 
 * @ioa:		ipr ioa struct
 * @gscsi_only_ha:	process gscsi only flag
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_set_ha_mode(struct ipr_ioa *ioa, int gscsi_only_ha)
{
	int rc, len;
	struct sense_data_t sense_data;
	struct ipr_config_term_hdr *hdr;
	struct ipr_subsys_config_term *term;
	struct ipr_mode_pages pages;
	struct ipr_mode_page25 *page;

	memset(&sense_data, 0, sizeof(sense_data));
	rc = mode_sense(&ioa->ioa, 0x25, &pages, &sense_data);

	if (!rc) {
		page = (struct ipr_mode_page25 *)(pages.data + pages.hdr.block_desc_len);

		for_each_page25_term(hdr, page) {
			if (hdr->term_id != IPR_SUBSYS_CONFIG_TERM_ID)
				continue;

			term = (struct ipr_subsys_config_term *)hdr;
			if (gscsi_only_ha)
				term->config = IPR_GSCSI_ONLY_HA_SUBSYS;
			else
				term->config = IPR_AFDASD_SUBSYS;

			len = pages.hdr.length + 1;
			pages.hdr.length = 0;

			return ipr_mode_select(&ioa->ioa, &pages, len);
		}

	}	

	return rc;
}

/**
 * ipr_change_multi_adapter_assignment - set the preferred primary state and/or
 *                                       the asymmetric access state
 * @ioa:		ipr ioa struct
 * @preferred_primary:	set preferred primary flag
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_change_multi_adapter_assignment(struct ipr_ioa *ioa, int preferred_primary,
					int asym_access)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	struct ipr_dev *dev = &ioa->ioa;

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_MAINTENANCE_OUT;
	cdb[1] = IPR_CHANGE_MULTI_ADAPTER_ASSIGNMENT;
	if (asym_access)
		cdb[2] = IPR_PRESERVE_ASYMMETRIC_STATE;
	if (preferred_primary)
		cdb[3] = IPR_IOA_STATE_PRIMARY;
	else
		cdb[3] = IPR_IOA_STATE_NO_PREFERENCE;

	rc = sg_ioctl(fd, cdb, NULL, 0, SG_DXFER_NONE,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0 && (sense_data.sense_key != ILLEGAL_REQUEST || ipr_debug))
		scsi_cmd_err(dev, &sense_data, "Change Multi Adapter Assignment", rc);

	close(fd);
	return rc;
}

static void ipr_save_ioa_attr(struct ipr_ioa *ioa, char *field,
			      char *value, int update);

/**
 * set_preferred_primary - set the preferred primary state
 * @ioa:		ipr ioa struct
 * @preferred_primary:	set preferred primary flag
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int set_preferred_primary(struct ipr_ioa *ioa, int preferred_primary)
{
	char temp[100];

	sprintf(temp, "%d", preferred_primary);
	if (ipr_change_multi_adapter_assignment(ioa, preferred_primary, IPR_PRESERVE_ASYMMETRIC_STATE))
		return -EIO;

	return 0;
}

/**
 * set_ha_mode - 
 * @ioa:		ipr ioa struct
 * @gscsi_only:		gscsi only flag
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int set_ha_mode(struct ipr_ioa *ioa, int gscsi_only)
{
	char temp[100];
	int reset_needed = (gscsi_only != ioa->in_gscsi_only_ha);

	sprintf(temp, "%d", gscsi_only);
	if (ipr_set_ha_mode(ioa, gscsi_only))
		return -EIO;
	ipr_save_ioa_attr(ioa, IPR_GSCSI_HA_ONLY, temp, 1);

	if (reset_needed)
		ipr_reset_adapter(ioa);

	return 0;
}

/**
 * ipr_set_array_rebuild_verify
 * @ioa: 		ipr ioa struct
 * @disable_verify:	Whether to disable Verify Rebuild (1, 0).
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_set_array_rebuild_verify(struct ipr_ioa *ioa, u8 disable_verify)
{
	int len, rc;
	struct ipr_mode_pages mode_pages;
	struct ipr_mode_page24 *page24;

	memset(&mode_pages, 0, sizeof(mode_pages));

	rc = ipr_mode_sense(&ioa->ioa, 0x24, &mode_pages);
	if (rc)
		return rc;

	page24 = ((struct ipr_mode_page24 *)
		  (((u8 *)&mode_pages)
		   + mode_pages.hdr.block_desc_len
		   + sizeof(mode_pages.hdr)));

	if (page24->rebuild_without_verify == disable_verify)
		return 0;

	len = mode_pages.hdr.length + 1;
	mode_pages.hdr.length = 0;
	page24->rebuild_without_verify = disable_verify;

	rc = ipr_mode_select(&ioa->ioa, &mode_pages, len);
	if (rc)
		return rc;

	/* Check if rebuild rate value was correctly set */
	memset(&mode_pages, 0, sizeof(mode_pages));

	rc = ipr_mode_sense(&ioa->ioa, 0x24, &mode_pages);
	if (rc)
		return rc;

	if (page24->rebuild_without_verify != disable_verify)
		return -EINVAL;

	return 0;
}

/**
 * ipr_set_array_rebuild_rate
 * @ioa:		ipr ioa struct
 * @check_rate		checkrate policy
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_set_array_rebuild_rate(struct ipr_ioa *ioa, u8 rebuild_rate)
{
	int len, rc;
	struct ipr_mode_pages mode_pages;
	struct ipr_mode_page24 *page24;

	memset(&mode_pages, 0, sizeof(mode_pages));

	rc = ipr_mode_sense(&ioa->ioa, 0x24, &mode_pages);
	if (rc)
		return rc;

	page24 = (struct ipr_mode_page24 *) (((u8 *)&mode_pages) +
					     mode_pages.hdr.block_desc_len +
					     sizeof(mode_pages.hdr));

	len = mode_pages.hdr.length + 1;
	mode_pages.hdr.length = 0;
	page24->rebuild_rate = rebuild_rate;

	rc = ipr_mode_select(&ioa->ioa, &mode_pages, len);
	if (rc)
		return rc;

	/* Verify if rebuild rate value was correctly set */
	memset(&mode_pages, 0, sizeof(mode_pages));

	rc = ipr_mode_sense(&ioa->ioa, 0x24, &mode_pages);
	if (rc)
		return rc;

	if (rebuild_rate != page24->rebuild_rate &&
	    (rebuild_rate || page24->rebuild_rate != MIN_ARRAY_REBUILD_RATE))
		return -EINVAL;

	return 0;
}

/**
 * ipr_set_active_active_mode -
 * @ioa:		ipr ioa struct
 * @mode:		enable or disable
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_set_active_active_mode(struct ipr_ioa *ioa)
{
	int len, rc;
	struct ipr_mode_pages mode_pages;
	struct ipr_mode_page24 *page24;

	memset(&mode_pages, 0, sizeof(mode_pages));
	rc = ipr_mode_sense(&ioa->ioa, 0x24, &mode_pages);
	if (!rc) {
		page24 = (struct ipr_mode_page24 *) (((u8 *)&mode_pages) +
				 mode_pages.hdr.block_desc_len +
				 sizeof(mode_pages.hdr));
		len = mode_pages.hdr.length +1;
		mode_pages.hdr.length = 0;
		page24->dual_adapter_af = ENABLE_DUAL_IOA_ACTIVE_ACTIVE;
		return ipr_mode_select(&ioa->ioa, &mode_pages, len);
	}

	return rc;
}

/**
 * set_active_active_mode -
 * @ioa:		ipr ioa struct
 * @mode:		enable or disable
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int set_active_active_mode(struct ipr_ioa *ioa, int mode)
{
	struct ipr_ioa_attr attr;
	int rc;

	/* Get the current ioa attributes. */
	rc = ipr_get_ioa_attr(ioa, &attr);

	if (rc)
		return rc;

	/* Get the saved ioa attributes from the config file. */
	rc = ipr_modify_ioa_attr(ioa, &attr);

	if (rc)
		return rc;

	if (attr.active_active == mode) {
		/* We should never get here. */
		ioa_dbg(ioa, "Saved and current asymmetric access mode are not the same.\n");
	}

	attr.active_active = mode;
	return ipr_set_ioa_attr(ioa, &attr, 1);
}


int ipr_set_ses_mode(struct ipr_dev *dev, int mode) {
	int rc;
	struct sense_data_t sense_data;
	u8 buff = (u8)mode;

	rc = __ipr_write_buffer(dev, 0x1f, &buff, 0, sizeof(buff), &sense_data);
	return rc;
}


/**
 * get_scsi_dev_data - get scsi device data
 * @scsi_dev_ref:	scsi_dev_data struct
 *
 * Returns:
 *   number of devices if success / 0 or -1 on failure
 **/
int get_scsi_dev_data(struct scsi_dev_data **scsi_dev_ref)
{
	int num_devs = 0;
	DIR *dirfd;
	struct dirent *dent;
	struct scsi_dev_data *scsi_dev_data;
	struct scsi_dev_data *scsi_dev_base = *scsi_dev_ref;

	dirfd = opendir("/sys/class/scsi_device");
	if (!dirfd) {
		syslog_dbg("Failed to open scsi_device class. %m\n");
		return 0;
	}
	while ((dent = readdir(dirfd)) != NULL) {
		int host, channel, lun;
		signed char id;
		char devpath[PATH_MAX];
		char buff[256];
		ssize_t len;

		if (sscanf(dent->d_name,"%d:%d:%hhd:%d",
			   &host, &channel, &id, &lun) != 4)
			continue;

		scsi_dev_base = realloc(scsi_dev_base,
					sizeof(struct scsi_dev_data) * (num_devs + 1));
		scsi_dev_data = &scsi_dev_base[num_devs];
		scsi_dev_data->host = host;
		scsi_dev_data->channel = channel;
		scsi_dev_data->id = id;
		scsi_dev_data->lun = lun;
		strcpy(scsi_dev_data->sysfs_device_name, dent->d_name);
		sprintf(devpath, "/sys/class/scsi_device/%s/device",
			dent->d_name);
		len = sysfs_read_attr(devpath, "type", buff, 256);
		if (len > 0)
			sscanf(buff, "%d", &scsi_dev_data->type);

		//FIXME WHERE TO GET OPENS!!!
			/*
			 sysfs_attr = sysfs_get_device_attr(sysfs_device_device, "opens");
			 sscanf(sysfs_attr->value "%d", &scsi_dev_data->opens);
			 */      scsi_dev_data->opens = 0;

		len = sysfs_read_attr(devpath, "state", buff, 256);
		if (len > 0 && strstr(buff, "offline"))
			scsi_dev_data->online = 0;
		else
			scsi_dev_data->online = 1;

		len = sysfs_read_attr(devpath, "queue_depth", buff, 256);
		if (len > 0)
			sscanf(buff, "%d", &scsi_dev_data->qdepth);

		len = sysfs_read_attr(devpath, "adapter_handle", buff, 256);
		if (len > 0)
			sscanf(buff, "%X", &scsi_dev_data->handle);

		len = sysfs_read_attr(devpath, "vendor", buff, 256);
		if (len > 0)
			ipr_strncpy_0n(scsi_dev_data->vendor_id,
				       buff, IPR_VENDOR_ID_LEN);

		len = sysfs_read_attr(devpath, "model", buff, 256);
		if (len > 0)
			ipr_strncpy_0n(scsi_dev_data->product_id,
				       buff, IPR_PROD_ID_LEN);

		len = sysfs_read_attr(devpath, "resource_path", buff, 256);
		if (len > 0)
			ipr_strncpy_0n(scsi_dev_data->res_path,
				       buff, IPR_MAX_RES_PATH_LEN);
 
		len = sysfs_read_attr(devpath, "device_id", buff, 256);
		if (len > 0)
			sscanf(buff, "%llX", &scsi_dev_data->device_id);

		strcpy(scsi_dev_data->dev_name,"");
		strcpy(scsi_dev_data->gen_name,"");
		num_devs++;
	}
	closedir(dirfd);

	*scsi_dev_ref = scsi_dev_base;
	return num_devs;
}

/**
 * wait_for_dev - wait 20 seconds for the device to become available
 * @name:		device name
 *
 * Returns:
 *   -ETIMEDOUT
 **/
static int wait_for_dev(char *name)
{
	int fd, delay;

	for (delay = 20, fd = 0; delay; delay--) {
		fd = open(name, O_RDWR);
		if (fd != -1) {
			close(fd);
			break;
		}

		syslog_dbg("Waiting for %s to show up\n", name);
		sleep(1);
	}

	if (fd == -1)
		syslog_dbg("Failed to open %s.\n", name);

	return -ETIMEDOUT;
}

int get_sg_name(struct scsi_dev_data *scsi_dev)
{
	int i, rc = -ENXIO;
	DIR *dirfd;
	struct dirent *dent;
	char devpath[PATH_MAX];

	sprintf(devpath, "/sys/class/scsi_device/%s/device/scsi_generic",
		scsi_dev->sysfs_device_name);
	dirfd = opendir(devpath);
	if (!dirfd)
		return -ENXIO;
	while((dent = readdir(dirfd)) != NULL) {
		if (dent->d_name[0] == '.')
			continue;
		if (strncmp(dent->d_name, "sg", 2))
			continue;
		sprintf(scsi_dev->gen_name, "/dev/%s",
			dent->d_name);
		rc = 0;
		break;
	}
	closedir(dirfd);
	return rc;
}

/**
 * get_sg_names - waits for sg devices to become available
 * @num_devs:		number of devices
 *
 * Returns:
 *   nothing
 **/
static void get_sg_names(int num_devs)
{
	int i;
	for (i = 0; i < num_devs; i++)
		get_sg_name(&scsi_dev_table[i]);
}

/**
 * get_sd_names - populate the scsi_dev_table dev_name entries
 * @num_devs:		number of devices
 *
 * Returns:
 *   nothing
 **/
static void get_sd_names(int num_devs)
{
	int i;
	DIR *dirfd;
	struct dirent *dent;
	char devpath[PATH_MAX];

	for (i = 0; i < num_devs; i++) {
		sprintf(devpath, "/sys/class/scsi_device/%s/device/block",
			scsi_dev_table[i].sysfs_device_name);
		dirfd = opendir(devpath);
		if (!dirfd)
			continue;
		while((dent = readdir(dirfd)) != NULL) {
			if (dent->d_name[0] == '.')
				continue;
			if (strncmp(dent->d_name, "sd", 2) && strncmp(dent->d_name, "sr", 2))
				continue;
			sprintf(scsi_dev_table[i].dev_name, "/dev/%s",
				dent->d_name);
			break;
		}
		closedir(dirfd);
	}
}

/**
 * get_ioa_name - populate adapter information in the scsi_dev_table
 * @ioa:		ipr ioa struct
 * @num_sg_devices:	number of sg devices
 *
 * Returns:
 *   nothing
 **/
static void get_ioa_name(struct ipr_ioa *ioa,
			 int num_sg_devices)
{
	int i;
	int host_no;
	int channel = IPR_IOAFP_VIRTUAL_BUS;
	int id = 0;
	int lun = 0;

	sscanf(ioa->host_name, "host%d", &host_no);

	if (!ioa->sis64)
		channel = id = lun = 255;

	for (i = 0; i < num_sg_devices; i++) {
		if (scsi_dev_table[i].host == host_no &&
		    scsi_dev_table[i].channel == channel &&
		    scsi_dev_table[i].id == id &&
		    scsi_dev_table[i].lun == lun &&
		    scsi_dev_table[i].type == IPR_TYPE_ADAPTER) {
			strcpy(ioa->ioa.dev_name, scsi_dev_table[i].dev_name);
			strcpy(ioa->ioa.gen_name, scsi_dev_table[i].gen_name);
			ioa->ioa.scsi_dev_data = &scsi_dev_table[i];
			ioa->ioa.ioa = ioa;
		}
	}
}

struct ipr_dual_ioa_state {
	u8 state;
	char *desc;
};

static struct ipr_dual_ioa_state dual_ioa_states [] = {
	{IPR_IOA_STATE_PRIMARY, "Primary"},
	{IPR_IOA_STATE_SECONDARY, "Secondary"},
	{IPR_IOA_STATE_NO_PREFERENCE, "No Preference"}
};

/**
 * print_ioa_state - copy the ioa state into buf
 * @buf:		data buffer
 * @state:	        state
 *
 * Returns:
 *   nothing
 **/
static void print_ioa_state(char *buf, u8 state)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(dual_ioa_states); i++) {
		if (dual_ioa_states[i].state == state) {
			strcpy(buf, dual_ioa_states[i].desc);
			return;
		}
	}

	sprintf(buf, "Unknown (%d)", state);
}

/**
 * get_subsys_config - 
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   nothing
 **/
static void get_subsys_config(struct ipr_ioa *ioa)
{
	int rc;
	struct ipr_mode_pages pages;
	struct ipr_mode_page25 *page;
	struct ipr_config_term_hdr *hdr;
	struct ipr_subsys_config_term *term;
	struct sense_data_t sense_data;

	ioa->in_gscsi_only_ha = 0;

	if (!ioa->gscsi_only_ha)
		return;

	memset(&sense_data, 0, sizeof(sense_data));
	rc = mode_sense(&ioa->ioa, 0x25, &pages, &sense_data);

	if (!rc) {
		page = (struct ipr_mode_page25 *)(pages.data + pages.hdr.block_desc_len);

		for_each_page25_term(hdr, page) {
			if (hdr->term_id != IPR_SUBSYS_CONFIG_TERM_ID)
				continue;

			term = (struct ipr_subsys_config_term *)hdr;
			if (term->config == IPR_GSCSI_ONLY_HA_SUBSYS)
				ioa->in_gscsi_only_ha = 1;
			return;
		} 
	}

}

/**
 * get_dual_ioa_state - set ioa->is_secondary to indicate the dual ioa state
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   nothing
 **/
static void get_dual_ioa_state(struct ipr_ioa *ioa)
{
	int rc;
	struct ipr_dual_ioa_entry *ioa_entry;

	sprintf(ioa->dual_state, "Primary");
	sprintf(ioa->preferred_dual_state, "No Preference");
	ioa->is_secondary = 0;

	if (!ioa->dual_raid_support)
		return;

	rc = ipr_query_multi_ioa_status(ioa, &ioa->ioa_status, sizeof(ioa->ioa_status));

	if (rc)
		return;

	print_ioa_state(ioa->preferred_dual_state, ioa->ioa_status.cap.preferred_role);

	if (ntohl(ioa->ioa_status.num_entries)) {
		ioa_entry = (struct ipr_dual_ioa_entry *)
			(((unsigned long)&ioa->ioa_status.cap) + ntohl(ioa->ioa_status.cap.length));
		print_ioa_state(ioa->dual_state, ioa_entry->cur_state);
		if (ioa_entry->cur_state == IPR_IOA_STATE_SECONDARY)
			ioa->is_secondary = 1;
	}
}

/**
 * get_af_block_size - return the af block size
 * @ioa_cap:		ipr_inquiry_ioa_cap struct
 *
 * Returns:
 *   the actual af_block_size or IPR_DEFAULT_AF_BLOCK_SIZE
 **/
static u16 get_af_block_size(struct ipr_inquiry_ioa_cap *ioa_cap)
{
	int sz_off = offsetof(struct ipr_inquiry_ioa_cap, af_block_size);
	int len_off = offsetof(struct ipr_inquiry_ioa_cap, page_length);

	if (ioa_cap->page_length > (sz_off - len_off))
		return ntohs(ioa_cap->af_block_size);
	return IPR_DEFAULT_AF_BLOCK_SIZE;
}

/**
 * get_ioa_cap - get the capability information for the ioa (inquiry page D0)
 *               Also, if page 01 is supported, then there is cache on the card.
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   nothing
 **/
static void get_ioa_cap(struct ipr_ioa *ioa)
{
	int rc, j;
	struct ipr_inquiry_page0 page0_inq;
	struct ipr_inquiry_ioa_cap ioa_cap;
	struct ipr_mode_page24 *page24;
	struct ipr_mode_pages mode_pages;
	struct ipr_cache_cap_vpd cc_vpd;

	memset(&mode_pages, 0, sizeof(mode_pages));

	ioa->af_block_size = IPR_DEFAULT_AF_BLOCK_SIZE;
	ioa->tcq_mode = ioa_get_tcq_mode(ioa);

	rc = ipr_inquiry(&ioa->ioa, 0, &page0_inq, sizeof(page0_inq));

	if (rc) {
		ioa->ioa_dead = 1;
		return;
	}

	for (j = 0; j < page0_inq.page_length; j++) {
		if (page0_inq.supported_page_codes[j] == 0x01) {
			ioa->has_cache = 1;
			continue;
		}
		if (page0_inq.supported_page_codes[j] == 0xC4) {
			ioa->has_cache = 1;

			memset(&cc_vpd, 0, sizeof(cc_vpd));
			ipr_inquiry(&ioa->ioa, 0xC4, &cc_vpd, sizeof(cc_vpd));

			if (htonl(cc_vpd.cache_cap) &
			    IPR_CACHE_CAP_VSET_WRITE_CACHE)
				ioa->has_vset_write_cache = 1;
			continue;
		}
		if (page0_inq.supported_page_codes[j] != 0xD0)
			continue;
		rc = ipr_inquiry(&ioa->ioa, 0xD0, &ioa_cap, sizeof(ioa_cap));
		if (rc)
			break;

		ioa->af_block_size = get_af_block_size(&ioa_cap);
		ioa->support_4k = ioa_cap.af_4k_support;

		if (ioa_cap.is_aux_cache)
			ioa->is_aux_cache = 1;
		if (ioa_cap.can_attach_to_aux_cache && ioa_cap.is_dual_wide)
			ioa->protect_last_bus = 1;
		if (ioa_cap.gscsi_only_ha)
			ioa->gscsi_only_ha = 1;

		if (ioa_cap.sis_format == IPR_SIS64)
			ioa->sis64 = 1;
		else {
			if (ioa_cap.ra_id_encoding == IPR_2BIT_HOP)
				ioa->hop_count = IPR_2BIT_HOP;
			else
				ioa->hop_count = IPR_3BIT_HOP;
		}

		rc = ipr_mode_sense(&ioa->ioa, 0x24, &mode_pages);
		if (rc)
			break;
		page24 = (struct ipr_mode_page24 *) (((u8 *)&mode_pages)
						     + mode_pages.hdr.block_desc_len +
						     sizeof(mode_pages.hdr));
		ioa->rebuild_rate = page24->rebuild_rate;

		if (ioa_cap.disable_array_rebuild_verify) {
			ioa->configure_rebuild_verify = 1;
			ioa->disable_rebuild_verify =
				page24->rebuild_without_verify;
		}

		if (ioa_cap.dual_ioa_raid || ioa_cap.dual_ioa_asymmetric_access) {
			if (ioa_cap.dual_ioa_raid && page24->dual_adapter_af == ENABLE_DUAL_IOA_AF)
				ioa->dual_raid_support = 1;

			if (ioa_cap.dual_ioa_asymmetric_access) {
				ioa->asymmetric_access = 1;
				if (page24->dual_adapter_af == ENABLE_DUAL_IOA_ACTIVE_ACTIVE) {
					ioa->dual_raid_support = 1;
					ioa->asymmetric_access_enabled = 1;
				} else {
					ioa->asymmetric_access_enabled = 0;
				}
			}
		}
	}
}

/**
 * get_prot_levels - populate the prot_level_str field for each array, for each
 *                   vset and each device in the vset
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   nothing
 **/
static void get_prot_levels(struct ipr_ioa *ioa)
{
	struct ipr_dev *array, *vset, *dev;
	char *prot_level_str;

	for_each_array(ioa, array) {
		prot_level_str = get_prot_level_str(ioa->supported_arrays,
						    array->raid_level);
		strncpy(array->prot_level_str, prot_level_str, 8);
	}

	for_each_vset(ioa, vset) {
		prot_level_str = get_prot_level_str(ioa->supported_arrays,
						    vset->raid_level);
		strncpy(vset->prot_level_str, prot_level_str, 8);
		for_each_dev_in_vset(vset, dev)
			strncpy(dev->prot_level_str, prot_level_str, 8);
	}
}

void ipr_convert_res_path_to_bytes(struct ipr_dev *dev)
{

	struct scsi_dev_data *scsi_dev_data = dev->scsi_dev_data;
	int i;
	char *startptr, *endptr;

	if (scsi_dev_data) {
		startptr = dev->res_path_name;
		i = 0;
		do {
			dev->res_path[0].res_path_bytes[i++] = (u8)strtol(startptr, &endptr, 16);
			startptr = endptr + 1;
		} while (*endptr != '\0' &&  i < 8);

		while ( i < 8 ) dev->res_path[0].res_path_bytes[i++] = 0xff;
	}
}

/**
 * get_res_addr - 
 * @dev:		ipr dev struct
 * @res_addr:		ipr_res_addr struct
 *
 * Returns:
 *   0 if success / -1 on failure
 **/
static int get_res_addr(struct ipr_dev *dev, struct ipr_res_addr *res_addr)
{
	struct ipr_dev_record *dev_record = dev->dev_rcd;
	struct ipr_array_record *array_record = dev->array_rcd;

	if (dev->scsi_dev_data) {
		res_addr->host = dev->scsi_dev_data->host;
		res_addr->bus = dev->scsi_dev_data->channel;
		res_addr->target = dev->scsi_dev_data->id;
		res_addr->lun = dev->scsi_dev_data->lun;
		if (dev->ioa->sis64) {
			strncpy(dev->res_path_name, dev->scsi_dev_data->res_path, strlen(dev->scsi_dev_data->res_path));
			ipr_convert_res_path_to_bytes(dev);
		}
	} else if (ipr_is_af_dasd_device(dev)) {
		if (dev_record && dev_record->no_cfgte_dev) {
			res_addr->host = dev->ioa->host_num;
			res_addr->bus = dev_record->type2.last_resource_addr.bus;
			res_addr->target = dev_record->type2.last_resource_addr.target;
			res_addr->lun = dev_record->type2.last_resource_addr.lun;
		} else if (dev_record) {
			res_addr->host = dev->ioa->host_num;
			res_addr->bus = dev_record->type2.resource_addr.bus;
			res_addr->target = dev_record->type2.resource_addr.target;
			res_addr->lun = dev_record->type2.resource_addr.lun;
			if (dev->ioa->sis64) {
				ipr_format_res_path(dev_record->type3.res_path, dev->res_path_name, IPR_MAX_RES_PATH_LEN);
				ipr_convert_res_path_to_bytes(dev);
			}
		} else
			return -1;
	} else if (ipr_is_volume_set(dev)) {
		if (array_record && array_record->no_config_entry) {
			res_addr->host = dev->ioa->host_num;
			res_addr->bus = array_record->type2.last_resource_addr.bus;
			res_addr->target = array_record->type2.last_resource_addr.target;
			res_addr->lun = array_record->type2.last_resource_addr.lun;
		} else if (array_record) {
			res_addr->host = dev->ioa->host_num;
			res_addr->bus = array_record->type2.resource_addr.bus;
			res_addr->target = array_record->type2.resource_addr.target;
			res_addr->lun = array_record->type2.resource_addr.lun;
		} else
			return -1;
	} else
		return -1;
	return 0;
}

/**
 * find_multipath_vset - return the other vset of a given multipath vset
 * @vset1:		ipr dev struct
 *
 * Returns:
 *   ipr_dev if success / NULL on failure
 **/
static struct ipr_dev *find_multipath_vset(struct ipr_dev *vset1)
{
	struct ipr_ioa *ioa;
	struct ipr_dev *vset2;

	for_each_ioa(ioa) {
		if (ioa == vset1->ioa)
			continue;
		for_each_vset(ioa, vset2) {
			if (memcmp(vset1->vendor_id,
				   vset2->vendor_id,
				   IPR_VENDOR_ID_LEN))
				continue;
			if (memcmp(vset1->product_id,
				   vset2->product_id,
				   IPR_PROD_ID_LEN))
				continue;
			if (memcmp(vset1->serial_number,
				   vset2->serial_number,
				   IPR_SERIAL_NUM_LEN))
				continue;
			return vset2;
		}
	}

	return NULL;
}

/**
 * link_multipath_vsets - set the alt_path entries for multipath vsets
 *
 * Returns:
 *   nothing
 **/
static void link_multipath_vsets()
{
	struct ipr_ioa *ioa;
	struct ipr_dev *vset1, *vset2;

	for_each_ioa(ioa) {
		for_each_vset(ioa, vset1) {
			vset2 = find_multipath_vset(vset1);
			if (!vset2)
				continue;
			vset1->alt_path = vset2;
			vset2->alt_path = vset1;
		}
	}
}

/**
 * ipr_format_res_path - Format the resource path into a string.
 * @res_path:	resource path
 * @buf:	buffer
 * @len:	buffer length
 *
 * Return value:
 *	none
 **/
void ipr_format_res_path(u8 *res_path, char *buffer, int len)
{
	int i;
	char *p = buffer;

	*p = '\0';
	p += snprintf(p, buffer + len - p, "%02X", res_path[0]);
	for (i = 1; res_path[i] != 0xff && ((i * 3) < len); i++)
		p += snprintf(p, buffer + len - p, "-%02X", res_path[i]);
}

/**
 * ipr_format_res_path_wo_hyphen - Format the resource path into a string.
 * @res_path:	resource path
 * @buf:	buffer
 * @len:	buffer length
 *
 * Return value:
 *	none
 **/
void ipr_format_res_path_wo_hyphen(u8 *res_path, char *buffer, int len)
{
	int i;
	char *p = buffer;

	*p = '\0';
	p += snprintf(p, buffer + len - p, "%02X", res_path[0]);
	for (i = 1; res_path[i] != 0xff && ((i * 3) < len); i++)
		p += snprintf(p, buffer + len - p, "%02X", res_path[i]);
}

/**
 * ipr_res_path_cmp - compare two resource paths
 * @dev_res_path
 * @scsi_res_path
 *
 * Returns:
 *   1 if the paths are the same, 0 otherwise
 **/
int ipr_res_path_cmp(u8 *dev_res_path, char *scsi_res_path)
{
	char buffer[IPR_MAX_RES_PATH_LEN];

	ipr_format_res_path(dev_res_path, buffer, IPR_MAX_RES_PATH_LEN);
	return !strncmp(buffer, scsi_res_path, IPR_MAX_RES_PATH_LEN);
}

/**
 * ipr_debug_dump_rcd - dump out a device record
 * @rcd:		record structure
 *
 * Returns:
 *   nothing
 */
void ipr_debug_dump_rcd(struct ipr_common_record *rcd)
{
	int i;
	u8 *rcd_ptr = (u8 *)rcd;

	dprintf("===========\n");
	dprintf("Record ID = %d, Length = %d\n", rcd->record_id, rcd->record_len);
	for (i=0; i<rcd->record_len; i++) {
		dprintf("%02x", rcd_ptr[i]);
		if ((i+1)%8 == 0)
			dprintf(" ");
		if ((i+1)%32 == 0)
			dprintf("\n");
	}
	dprintf("\n");
}

/**
 * ipr_get_logical_block_size - check the logical block size
 * @dev:		ipr dev struct
 *
 * Return value:
 *   none
 **/
int ipr_get_logical_block_size(struct ipr_dev *dev)
{
	char path[PATH_MAX], *first_hyphen;
	char buff[16];
	ssize_t len;
	int rc;

	first_hyphen = strchr(dev->dev_name, 's');
	sprintf(path, "/sys/block/%s/queue", first_hyphen);

	len = sysfs_read_attr(path, "logical_block_size", buff, 16);
	if (len < 0) {
		syslog_dbg("Failed to open logical_block_size parameter.\n");
		return -1;
	}

	rc = atoi(buff);

	return rc;
}

/**
 * init_inquiry_c7 - Page 0xC7 Inquiry to disks
 * @dev:		ipr dev struct
 *
 * Setup IBM vendor unique settings
 *
 * Returns:
 *   0 if success / other on failure
 **/
static int init_inquiry_c7(struct ipr_dev *dev)
{
	struct ipr_sas_inquiry_pageC7 inq;
	int rc;

	if (dev->read_c7)
		return 0;

	if (!dev->scsi_dev_data || strncmp(dev->scsi_dev_data->vendor_id, "IBM", 3)) {
		if (ipr_is_gscsi(dev)) {
			if (ipr_get_logical_block_size(dev) == IPR_JBOD_4K_BLOCK_SIZE) {
				dev->block_dev_class |= IPR_BLK_DEV_CLASS_4K;
				dev->supports_4k = 1;
			} else {
				dev->block_dev_class &= ~IPR_BLK_DEV_CLASS_4K;
				dev->supports_5xx = 1;
			}
		}

		dev->format_timeout = IPR_FORMAT_UNIT_TIMEOUT;
		scsi_dbg(dev, "Skipping IBM vendor settings for non IBM device.\n");
		return -EINVAL;
	}

	memset(&inq, 0, sizeof(inq));

	rc = ipr_inquiry(dev, 0xC7, &inq, sizeof(inq));

	if (rc) {
		scsi_dbg(dev, "Inquiry 0xC7 failed. rc=%d\n", rc);
		return rc;
	}

	switch (inq.support_4k_modes) {
	case ONLY_5XX_SUPPORTED:
		dev->supports_5xx = 1;
		dev->supports_4k = 0;
		dev->block_dev_class &= ~IPR_BLK_DEV_CLASS_4K;
		scsi_dbg(dev, "Only 5xx supported.\n");
		break;
	case ONLY_4K_SUPPORTED:
		dev->supports_4k = 1;
		dev->block_dev_class |= IPR_BLK_DEV_CLASS_4K;
		scsi_dbg(dev, "Only 4k supported.\n");
		break;
	case BOTH_5XXe_OR_4K_SUPPORTED:
	default:
		dev->supports_5xx = 1;
		dev->supports_4k = 1;
		dev->block_dev_class |= IPR_BLK_DEV_CLASS_4K;
		scsi_dbg(dev, "Both 4k and 5xx supported.\n");
		break;
	};

	dev->read_c7 = 1;
	dev->format_timeout = ((inq.format_timeout_hi << 8) | inq.format_timeout_lo) * 60;
	return 0;
}

/**
 * __check_current_config - populates the ioa configuration data
 * @allow_rebuild_refresh:	allow_rebuild_refresh flag
 * @device_details_only:	Skip commands not needed for show-details
 *
 * Returns:
 *   nothing
 **/
void __check_current_config(bool allow_rebuild_refresh, bool device_details_only)
{
	struct scsi_dev_data *scsi_dev_data;
	int num_sg_devices, rc, device_count, j, k;
	struct ipr_ioa *ioa;
	struct ipr_array_query_data *qac_data;
	struct ipr_common_record *common_record;
	struct ipr_dev_record *device_record;
	struct ipr_array_record *array_record;
	struct ipr_std_inq_data std_inq_data;
	struct sense_data_t sense_data;
	struct ipr_res_addr res_addr, *ra;
	struct ipr_dev *dev;
	int *qac_entry_ref;
	struct ipr_dev_identify_vpd di_vpd;
	char *pchr;
	struct ipr_mode_pages mode_pages;
	struct ipr_ioa_mode_page *page;

	if (ipr_qac_data == NULL) {
		ipr_qac_data =
			(struct ipr_array_query_data *)
			calloc(num_ioas, sizeof(struct ipr_array_query_data));
	}

	/* Get sg data via sysfs */
	num_sg_devices = get_scsi_dev_data(&scsi_dev_table);
	get_sg_names(num_sg_devices);
	get_sd_names(num_sg_devices);

	for(ioa = ipr_ioa_head, qac_data = ipr_qac_data;
	    ioa; ioa = ioa->next, qac_data++) {
		get_ioa_name(ioa, num_sg_devices);

		ioa->num_devices = 0;

		rc = ipr_inquiry(&ioa->ioa, IPR_STD_INQUIRY,
				 &std_inq_data, sizeof(std_inq_data));

		if (rc)
			ioa->ioa_dead = 1;

		get_ioa_cap(ioa);
		get_dual_ioa_state(ioa);
		get_subsys_config(ioa);

		if (ioa->has_vset_write_cache == 1 &&
		    get_ioa_caching(ioa) == IPR_IOA_VSET_CACHE_ENABLED)
			ioa->vset_write_cache = 1;

		/* Get Query Array Config Data */
		rc = ipr_query_array_config(ioa, allow_rebuild_refresh, 0, 0, 0, qac_data);

		if (rc != 0) {
			qac_data->num_records = 0;
			qac_data->resp_len = qac_data->hdr_len;
		}

		ioa->qac_data = qac_data;
		ioa->start_array_qac_entry = NULL;

		device_count = 0;
		memset(ioa->dev, 0, IPR_MAX_IOA_DEVICES * sizeof(struct ipr_dev));
		qac_entry_ref = calloc(1, sizeof(int) * qac_data->num_records);

		/* now assemble data pertaining to each individual device */
		for (j = 0, scsi_dev_data = scsi_dev_table;
		     j < num_sg_devices; j++, scsi_dev_data++) {
			if (scsi_dev_data->host != ioa->host_num)
				continue;

			if (ioa->ioa.scsi_dev_data == scsi_dev_data)
				continue;

			if (scsi_dev_data->type == TYPE_DISK ||
			    scsi_dev_data->type == IPR_TYPE_AF_DISK ||
			    scsi_dev_data->type == IPR_TYPE_ARRAY ||
			    scsi_dev_data->type == TYPE_ENCLOSURE ||
			    scsi_dev_data->type == TYPE_ROM ||
			    scsi_dev_data->type == TYPE_TAPE ||
			    scsi_dev_data->type == TYPE_PROCESSOR) {
				ioa->dev[device_count].ioa = ioa;
				ioa->dev[device_count].scsi_dev_data = scsi_dev_data;
				ioa->dev[device_count].qac_entry = NULL;
				strcpy(ioa->dev[device_count].dev_name,
				       scsi_dev_data->dev_name);
				strcpy(ioa->dev[device_count].gen_name,
				       scsi_dev_data->gen_name);

				/* find array config data matching resource entry */
				k = 0;
				for_each_qac_entry(common_record, qac_data) {
					if (common_record->record_id == IPR_RECORD_ID_DEVICE_RECORD) {
						device_record = (struct ipr_dev_record *)common_record;
						if (device_record->type2.resource_addr.bus == scsi_dev_data->channel &&
						    device_record->type2.resource_addr.target == scsi_dev_data->id &&
						    device_record->type2.resource_addr.lun == scsi_dev_data->lun) {
							ioa->dev[device_count].qac_entry = common_record;
							qac_entry_ref[k]++;
							break;
						}
					} else if (common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD) {
						array_record = (struct ipr_array_record *)common_record;
						if (array_record->type2.resource_addr.bus == scsi_dev_data->channel &&
						    array_record->type2.resource_addr.target == scsi_dev_data->id &&
						    array_record->type2.resource_addr.lun == scsi_dev_data->lun) {
							ioa->dev[device_count].qac_entry = common_record;
							qac_entry_ref[k]++;
							break;
						}
					} else if (common_record->record_id == IPR_RECORD_ID_DEVICE_RECORD_3) {
						device_record = (struct ipr_dev_record *)common_record;
						if (ipr_res_path_cmp(device_record->type3.res_path, scsi_dev_data->res_path)) {
							ioa->dev[device_count].qac_entry = common_record;
							qac_entry_ref[k]++;
							break;
						}
					} else if (common_record->record_id == IPR_RECORD_ID_VSET_RECORD_3) {
						array_record = (struct ipr_array_record *)common_record;
						if (ipr_res_path_cmp(array_record->type3.res_path, scsi_dev_data->res_path)) {
							ioa->dev[device_count].qac_entry = common_record;
							qac_entry_ref[k]++;
							break;
						}
					} else if (common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD_3) {
						array_record = (struct ipr_array_record *)common_record;
						if (ipr_res_path_cmp(array_record->type3.res_path, scsi_dev_data->res_path)) {
							ioa->dev[device_count].qac_entry = common_record;
							qac_entry_ref[k]++;
							break;
						}
					}
					k++;
				}

				/* Send Test Unit Ready to start device if its a volume set */
				/* xxx TODO try to remove this */
				if (!ipr_fast && ipr_is_volume_set(&ioa->dev[device_count]) && !device_details_only)
					__ipr_test_unit_ready(&ioa->dev[device_count], &sense_data);

				device_count++;
			}
		}

		/* now scan qac device and array entries to see which ones have
		 not been referenced */
		k = 0;
		for_each_qac_entry(common_record, qac_data) {
			if (qac_entry_ref[k] > 1)
				syslog(LOG_ERR,
				       "Query Array Config entry referenced more than once\n");

			if (common_record->record_id == IPR_RECORD_ID_SUPPORTED_ARRAYS) {
				ioa->supported_arrays = (struct ipr_supported_arrays *)common_record;
			} else if (!qac_entry_ref[k] &&
				   (ipr_is_device_record(common_record->record_id) ||
				    ipr_is_vset_record(common_record->record_id))) {
				// TODO - type 3 array records????
				array_record = (struct ipr_array_record *)common_record;
				if (ipr_is_vset_record(common_record->record_id) &&
				    array_record->start_cand) {
					ioa->start_array_qac_entry = array_record;
				} else {
					/* add phantom qac entry to ioa device list */
					ioa->dev[device_count].scsi_dev_data = NULL;
					ioa->dev[device_count].qac_entry = common_record;
					ioa->dev[device_count].ioa = ioa;
					strcpy(ioa->dev[device_count].dev_name, "");
					strcpy(ioa->dev[device_count].gen_name, "");
					device_count++;
				}
			}
			k++;
		}

		ioa->num_devices = device_count;
		free(qac_entry_ref);
	}

	for_each_ioa(ioa) {
		for_each_dev(ioa, dev) {
			get_res_addr(dev, &res_addr);
			for_each_ra(ra, dev)
				memcpy(ra, &res_addr, sizeof(*ra));

			if (ipr_is_gscsi(dev) || ipr_is_af_dasd_device(dev) && !device_details_only)
				init_inquiry_c7(dev);

			if (!dev->qac_entry)
				continue;

			if (dev->qac_entry->record_id == IPR_RECORD_ID_DEVICE_RECORD) {
				dev->vendor_id = dev->dev_rcd->type2.vendor_id;
				dev->product_id = dev->dev_rcd->type2.product_id;
				dev->serial_number = dev->dev_rcd->type2.serial_number;
				dev->array_id = dev->dev_rcd->type2.array_id;
				dev->resource_handle = dev->dev_rcd->type2.resource_handle;
				dev->block_dev_class = dev->dev_rcd->type2.block_dev_class;
				if (dev->block_dev_class & IPR_SSD)
					dev->read_intensive = dev->dev_rcd->type2.read_intensive;
			} else if (dev->qac_entry->record_id == IPR_RECORD_ID_DEVICE_RECORD_3) {
				dev->vendor_id = dev->dev_rcd->type3.vendor_id;
				dev->product_id = dev->dev_rcd->type3.product_id;
				dev->serial_number = dev->dev_rcd->type3.serial_number;
				dev->array_id = dev->dev_rcd->type3.array_id;
				dev->resource_handle = dev->dev_rcd->type3.resource_handle;
				dev->block_dev_class = dev->dev_rcd->type3.block_dev_class;
				if (dev->block_dev_class & IPR_SSD)
					dev->read_intensive = dev->dev_rcd->type3.read_intensive;
			} else if (dev->qac_entry->record_id == IPR_RECORD_ID_ARRAY_RECORD) {
				dev->vendor_id = dev->array_rcd->type2.vendor_id;
				dev->product_id = dev->array_rcd->type2.product_id;
				dev->serial_number = dev->array_rcd->type2.serial_number;
				dev->array_id = dev->array_rcd->type2.array_id;
				dev->raid_level = dev->array_rcd->type2.raid_level;
				dev->stripe_size = dev->array_rcd->type2.stripe_size;
				dev->resource_handle = dev->array_rcd->type2.resource_handle;
				dev->block_dev_class = dev->array_rcd->type2.block_dev_class;
				if (dev->block_dev_class & IPR_SSD)
					dev->read_intensive = dev->dev_rcd->type2.read_intensive;
			} else if (dev->qac_entry->record_id == IPR_RECORD_ID_VSET_RECORD_3) {
				dev->vendor_id = dev->array_rcd->type3.vendor_id;
				dev->product_id = dev->array_rcd->type3.product_id;
				dev->serial_number = dev->array_rcd->type3.serial_number;
				dev->array_id = dev->array_rcd->type3.array_id;
				dev->raid_level = dev->array_rcd->type3.raid_level;
				dev->stripe_size = dev->array_rcd->type3.stripe_size;
				dev->resource_handle = dev->array_rcd->type3.resource_handle;
				dev->block_dev_class = dev->array_rcd->type3.block_dev_class;
				if (dev->block_dev_class & IPR_SSD)
					dev->read_intensive = dev->dev_rcd->type3.read_intensive;
			} else if (dev->qac_entry->record_id == IPR_RECORD_ID_ARRAY_RECORD_3) {
				dev->vendor_id = dev->array_rcd->type3.vendor_id;
				dev->product_id = dev->array_rcd->type3.product_id;
				dev->serial_number = dev->array_rcd->type3.serial_number;
				dev->array_id = dev->array_rcd->type3.array_id;
				dev->raid_level = dev->array_rcd->type3.raid_level;
				dev->stripe_size = dev->array_rcd->type3.stripe_size;
				dev->resource_handle = dev->array_rcd->type3.resource_handle;
				dev->block_dev_class = dev->array_rcd->type3.block_dev_class;
				if (dev->block_dev_class & IPR_SSD)
					dev->read_intensive = dev->dev_rcd->type3.read_intensive;
			}
		}
		get_prot_levels(ioa);
	}

	for_each_ioa(ioa) {
		if (strlen((char *)ioa->yl_serial_num) == 0) {
			memset(&di_vpd, 0, sizeof(di_vpd));
			rc = ipr_inquiry(&ioa->ioa, 0x83, &di_vpd, sizeof(di_vpd));
			if (!rc && ntohs(di_vpd.add_page_len) > 120) {
				pchr = strstr((char *)&di_vpd.dev_identify_contxt[0],"SN");
				if (pchr)
					strncpy((char *)ioa->yl_serial_num, (pchr + 3), YL_SERIAL_NUM_LEN);
			}
		}
	}

	set_devs_format_completed();
	link_multipath_vsets();
	ipr_cleanup_zeroed_devs();
	resolve_old_config();

        if (!first_time_check_zeroed_dev) {
                for_each_ioa(ioa) {
                        for_each_dev(ioa, dev) {
                                if (ipr_is_af_dasd_device(dev)) {
                                        memset(&mode_pages, 0, sizeof(mode_pages));
                                        ipr_mode_sense(dev, 0x20, &mode_pages);

                                        page = (struct ipr_ioa_mode_page *) (((u8 *)&mode_pages) +
                                                     mode_pages.hdr.block_desc_len +
                                                     sizeof(mode_pages.hdr));

                                        if (page->format_completed)
                                                ipr_add_zeroed_dev(dev);
                                }
                        }
                }
                first_time_check_zeroed_dev = 1;
        }
}

void check_current_config(bool allow_rebuild_refresh)
{
	__check_current_config(allow_rebuild_refresh, 0);
}

/**
 * num_devices_opens - return usage count (number of opens) for a given device
 * @host_num:		host number
 * @channel:    	channel number
 * @id: 		id number
 * @lun:		lun number
 *
 * Returns:
 *   usage count (number of opens) for a given device
 **/
/* xxx TODO delete */
int num_device_opens(int host_num, int channel, int id, int lun)
{
	struct scsi_dev_data *scsi_dev_base = NULL;
	int opens = 0;
	int k, num_sg_devices;

	/* Get sg data via sg proc file system */
	num_sg_devices = get_scsi_dev_data(&scsi_dev_base);

	/* find usage counts in scsi_dev_data */
	for (k = 0; k < num_sg_devices; k++)
	{
		if ((host_num == scsi_dev_base[k].host) &&
		    (channel  == scsi_dev_base[k].channel) &&
		    (id       == scsi_dev_base[k].id) &&
		    (lun      == scsi_dev_base[k].lun))
		{
			opens = scsi_dev_base[k].opens;
			break;
		}
	}

	free(scsi_dev_base);
	return opens;
}

/**
 * open_and_lock - Open and device file and lock it.
 * @file_name:		the name of the device to open
 *
 * Returns:
 *   file descriptor if success / -1 on failure
 **/
int open_and_lock(char *file_name)
{
	int fd;
	int rc;

	fd = open(file_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", file_name);
		return fd;
	}

	rc = flock(fd, LOCK_EX);
	if (rc) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not lock %s. %m\n", file_name);
		close(fd);
		return rc;
	}

	return fd;
}

/**
 * exit_on_error - exits program or error after cleaning up
 * @s:			string
 * @...:		arguments
 *
 * Returns:
 *   nothing - exits program on error
 **/
void exit_on_error(char *s, ...)
{
	va_list args;
	char usr_str[256];

	exit_func();

	va_start(args, s);
	vsprintf(usr_str, s, args);
	va_end(args);

	closelog();
	openlog(tool_name, LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);
	syslog(LOG_ERR,"%s",usr_str);
	closelog();

	exit(1);
}

/**
 * ipr_config_file_hdr - prints the header to the ipr config file
 * @file_name:		file name
 *
 * Returns:
 *   nothing
 **/
static void ipr_config_file_hdr(char *file_name)
{
	FILE *fd;
	char cmd_str[64];

	if (strlen(file_name) == 0)
		return;

	/* check if file currently exists */
	fd = fopen(file_name, "r");
	if (fd) {
		fclose(fd);
		return;
	}

	/* be sure directory is present */
	sprintf(cmd_str,"install -d %s",IPR_CONFIG_DIR);
	system(cmd_str);

	/* otherwise, create new file */
	fd = fopen(file_name, "w");
	if (fd == NULL) {
		syslog(LOG_ERR, "Could not open %s. %m\n", file_name);
		return;
	}

	fprintf(fd,"# DO NOT EDIT! Software generated configuration file for\n");
	fprintf(fd,"# ipr SCSI device subsystem\n\n");
	fprintf(fd,"# Use iprconfig to configure\n");
	fprintf(fd,"# See \"man iprconfig\" for more information\n\n");
	fclose(fd);
}

/**
 * ipr_save_attr -
 * @ioa:		ipr ioa struct
 * @category:		
 * @field:		
 * @value:		
 * @update:		
 *
 * Returns:
 *   nothing
 **/
static void ipr_save_attr(struct ipr_ioa *ioa, char *category,
			  char *field, char *value, int update)
{
	char fname[64];
	FILE *fd, *temp_fd;
	char temp_fname[64], line[64];
	int bus_found = 0;
	int field_found = 0;

	sprintf(fname, "%s%x_%s", IPR_CONFIG_DIR, ioa->ccin, ioa->pci_address);
	ipr_config_file_hdr(fname);

	fd = fopen(fname, "r");
	if (fd == NULL)
		return;

	sprintf(temp_fname, "%s.1", fname);
	temp_fd = fopen(temp_fname, "w");
	if (temp_fd == NULL) {
		syslog(LOG_ERR, "Could not open %s. %m\n", temp_fname);
		return;
	}

	while (fgets(line, 64, fd)) {
		if (strstr(line, category) && line[0] != '#') {
			bus_found = 1;
		} else {
			if (bus_found) {
				if (line[0] == '[') {
					bus_found = 0;
					if (!field_found) {
						if (!update)
							fprintf(temp_fd, "# ");

						fprintf(temp_fd,"%s %s\n",field,value);
					}
				}

				if (strstr(line, field)) {
					if (update)
						sprintf(line,"%s %s\n",field,value);

					field_found = 1;
				}
			}
		}

		fputs(line, temp_fd);
	} 

	if (!field_found) {
		if (!bus_found)
			fprintf(temp_fd,"\n%s\n", category);

		if (!update)
			fprintf(temp_fd, "# ");

		fprintf(temp_fd,"%s %s\n", field, value);
	}

	if (rename(temp_fname, fname))
		syslog(LOG_ERR, "Could not save %s.\n", fname);
	fclose(fd);
	fclose(temp_fd);
}

/**
 * ipr_save_bus_attr - 
 * @ioa:		ipr ioa struct
 * @bus:		bus
 * @field:		
 * @value:		
 * @update:		
 *
 * Returns:
 *   nothing
 **/
static void ipr_save_bus_attr(struct ipr_ioa *ioa, int bus,
			      char *field, char *value, int update)
{
	char category[16];
	sprintf(category,"[%s %d]", IPR_CATEGORY_BUS, bus);
	ipr_save_attr(ioa, category, field, value, update);
}

/**
 * ipr_save_dev_attr - 
 * @dev:		ipr dev struct
 * @field:		
 * @value:		
 * @update:		
 *
 * Returns:
 *   nothing
 **/
static void ipr_save_dev_attr(struct ipr_dev *dev, char *field,
			     char *value, int update)
{
	char category[100];
	struct ipr_dev *alt_dev = dev->alt_path;

	if (dev->scsi_dev_data->device_id)
		sprintf(category,"[%s %lx]", IPR_CATEGORY_DEVICE,
			dev->scsi_dev_data->device_id);
	else
		sprintf(category,"[%s %d:%d:%d]", IPR_CATEGORY_DISK,
			dev->scsi_dev_data->channel, dev->scsi_dev_data->id,
			dev->scsi_dev_data->lun);

	ipr_save_attr(dev->ioa, category, field, value, update);
	if (alt_dev)
		ipr_save_attr(alt_dev->ioa, category, field, value, update);
}

/**
 * ipr_save_ioa_attr - 
 * @ioa:		ipr ioa struct
 * @field:		
 * @value:		
 * @update:		
 *
 * Returns:
 *   nothing
 **/
static void ipr_save_ioa_attr(struct ipr_ioa *ioa, char *field,
			     char *value, int update)
{
	char category[16];
	sprintf(category,"[%s]", IPR_CATEGORY_IOA);
	ipr_save_attr(ioa, category, field, value, update);
}

/**
 * ipr_get_saved_attr - 
 * @ioa:		ipr ioa struct
 * @category:		
 * @field:		
 * @value:		
 *
 * Returns:
 *   0 if success / RC_FAILED on failure
 **/
static int ipr_get_saved_attr(struct ipr_ioa *ioa, char *category,
				  char *field, char *value)
{
	FILE *fd;
	char fname[100], line[100], *str_ptr;
	int bus_found = 0;

	sprintf(fname, "%s%x_%s", IPR_CONFIG_DIR, ioa->ccin, ioa->pci_address);

	fd = fopen(fname, "r");
	if (fd == NULL)
		return RC_FAILED;

	while (NULL != fgets(line, 100, fd)) {
		if (line[0] != '#') {
			if (strstr(line, category))
				bus_found = 1;
			else if (bus_found) {
				if (line[0] == '[')
					bus_found = 0;

				if ((str_ptr = strstr(line, field))) {
					str_ptr += strlen(field);
					while (str_ptr[0] == ' ')
						str_ptr++;
					sscanf(str_ptr, "%s\n", value);
					fclose(fd);
					return RC_SUCCESS;
				}
			}
		}
	}

	fclose(fd);
	return RC_FAILED;
}

/**
 * ipr_get_saved_bus_attr - 
 * @ioa:		ipr ioa struct
 * @bus:		
 * @field:		
 * @value:		
 *
 * Returns:
 *   0 if success / RC_FAILED on failure
 **/
static int ipr_get_saved_bus_attr(struct ipr_ioa *ioa, int bus,
				  char *field, char *value)
{
	char category[16];
	sprintf(category,"[%s %d]", IPR_CATEGORY_BUS, bus);
	return ipr_get_saved_attr(ioa, category, field, value);
}

/**
 * ipr_get_saved_dev_attr - 
 * @dev:		ipr dev struct
 * @field:		
 * @value:		
 *
 * Returns:
 *   0 if success / RC_FAILED on failure
 **/
static int ipr_get_saved_dev_attr(struct ipr_dev *dev,
				  char *field, char *value)
{
	char category[100];
	int rc = RC_FAILED;

	if (!dev->scsi_dev_data)
		return -ENXIO;

	if (dev->scsi_dev_data->device_id) {
		sprintf(category,"[%s %lx]", IPR_CATEGORY_DEVICE,
			dev->scsi_dev_data->device_id);

		rc = ipr_get_saved_attr(dev->ioa, category, field, value);

		if (rc) {
			/* Older kernels reported a byte swapped device_id, which has since
			 been changed. Check both for compatibility reasons */
			sprintf(category,"[%s %lx]", IPR_CATEGORY_DEVICE,
				htobe64(dev->scsi_dev_data->device_id));

			rc = ipr_get_saved_attr(dev->ioa, category, field, value);
		}
	}

	if (rc) {
		sprintf(category,"[%s %d:%d:%d]", IPR_CATEGORY_DISK,
			dev->scsi_dev_data->channel, dev->scsi_dev_data->id,
			dev->scsi_dev_data->lun);
		rc = ipr_get_saved_attr(dev->ioa, category, field, value);
	}

	return rc;
}

/**
 * ipr_get_saved_ioa_attr - 
 * @ioa:		ipr ioa struct
 * @field:	
 * @value:		
 *
 * Returns:
 *   0 if success / RC_FAILED on failure
 **/
static int ipr_get_saved_ioa_attr(struct ipr_ioa *ioa,
				  char *field, char *value)
{
	char category[16];
	sprintf(category,"[%s]", IPR_CATEGORY_IOA);
	return ipr_get_saved_attr(ioa, category, field, value);
}

#define GSCSI_TCQ_DEPTH 3
#define GSCSI_SAS_TCQ_DEPTH 16
#define AS400_TCQ_DEPTH 16
#define DEFAULT_TCQ_DEPTH 64

/**
 * get_tcq_depth - return the proper queue depth for the given device
 * @dev:		ipr dev struct
 *
 * Returns:
 *   GSCSI_TCQ_DEPTH, GSCSI_SAS_TCQ_DEPTH, AS400_TCQ_DEPTH or DEFAULT_TCQ_DEPTH
 **/
static int get_tcq_depth(struct ipr_dev *dev)
{
	if (ipr_is_gscsi(dev)) {
		if (ioa_is_spi(dev->ioa))
			return GSCSI_TCQ_DEPTH;
		else
			return GSCSI_SAS_TCQ_DEPTH;
	}
	if (!dev->scsi_dev_data)
		return AS400_TCQ_DEPTH;
	if (!strncmp(dev->scsi_dev_data->vendor_id, "IBMAS400", 8))
		return AS400_TCQ_DEPTH;

	return DEFAULT_TCQ_DEPTH;
}

/**
 * is_tagged -
 * @dev:		ipr dev struct
 *
 * Returns:
 *
 **/
static int is_tagged(struct ipr_dev *dev)
{
	char temp[100];

	if (!ipr_read_dev_attr(dev, "tcq_enable", temp, 100))
		return strtoul(temp, NULL, 10);
	else if (!ipr_read_dev_attr(dev, "queue_type", temp, 100))
		return (strstr(temp, "none") ? 0 : 1);

	return 0;
}

/**
 * set_tagged -
 * @dev:		ipr dev struct
 * @tcq_enabled:
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int set_tagged(struct ipr_dev *dev, int tcq_enabled)
{
	char temp[100];

	if (!ipr_read_dev_attr(dev, "tcq_enable", temp, 100)) {
		sprintf(temp, "%d", tcq_enabled);
		return ipr_write_dev_attr(dev, "tcq_enable", temp);
	}

	if (!ipr_read_dev_attr(dev, "queue_type", temp, 100)) {
		if (!tcq_enabled)
			return ipr_write_dev_attr(dev, "queue_type", "none");

		if (page0x0a_setup(dev))
			return ipr_write_dev_attr(dev, "queue_type", "ordered");
		else
			return ipr_write_dev_attr(dev, "queue_type", "simple");
	}

	return -EIO;
}

/**
 * get_format_timeout - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   timeout value
 **/
static int get_format_timeout(struct ipr_dev *dev)
{
	struct ipr_query_dasd_timeouts tos;
	int rc, i, records, timeout;
	char temp[100];

	rc = init_inquiry_c7(dev);

	if (rc && ipr_is_af_dasd_device(dev)) {
		rc = ipr_query_dasd_timeouts(dev, &tos);

		if (!rc) {
			records = (ntohl(tos.length) - sizeof(tos.length)) / sizeof(tos.record[0]);

			for (i = 0; i < records; i++) {
				if (tos.record[i].op_code != FORMAT_UNIT)
					continue;
				if (IPR_TIMEOUT_RADIX_IS_MINUTE(ntohs(tos.record[i].timeout)))
					return ((ntohs(tos.record[i].timeout) & IPR_TIMEOUT_MASK) * 60);
				if (IPR_TIMEOUT_RADIX_IS_SECONDS(ntohs(tos.record[i].timeout)))
					return ntohs(tos.record[i].timeout) & IPR_TIMEOUT_MASK;
				scsi_dbg(dev, "Unknown timeout radix: %X\n",
					 (ntohs(tos.record[i].timeout) & IPR_TIMEOUT_RADIX_MASK));
				break;
			}
		}
	}

	timeout = dev->format_timeout;
	rc = ipr_get_saved_dev_attr(dev, IPR_FORMAT_TIMEOUT, temp);
	if (rc == RC_SUCCESS)
		sscanf(temp, "%d", &timeout);

	return timeout;
}

static const struct ipr_dasd_timeout_record ipr_dasd_timeouts[] = {
	{READ_10, 0, __constant_cpu_to_be16(30)},
	{WRITE_10, 0, __constant_cpu_to_be16(30)},
	{WRITE_VERIFY, 0, __constant_cpu_to_be16(30)},
	{SKIP_READ, 0, __constant_cpu_to_be16(30)},
	{SKIP_WRITE, 0, __constant_cpu_to_be16(30)}
};

struct ipr_dasd_timeouts {
	u32 length;
	struct ipr_dasd_timeout_record record[ARRAY_SIZE(ipr_dasd_timeouts) + 1];
};

/**
 * ipr_set_dasd_timeouts - 
 * @dev:		ipr dev struct
 * @format_timeout:	
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int ipr_set_dasd_timeouts(struct ipr_dev *dev, int format_timeout)
{
	int fd, rc, len;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	struct ipr_dasd_timeouts timeouts;
	struct ipr_disk_attr attr;
	char *name = dev->gen_name;

	fd = open(name, O_RDWR);

	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memcpy(timeouts.record, ipr_dasd_timeouts, sizeof(ipr_dasd_timeouts));
	len = sizeof(timeouts) - sizeof(timeouts.record[0]);

	if (!ipr_get_dev_attr(dev, &attr)) {
		len = sizeof(timeouts);

		if (!format_timeout)
			format_timeout = attr.format_timeout;

		timeouts.record[ARRAY_SIZE(ipr_dasd_timeouts)].op_code = FORMAT_UNIT;
		if (format_timeout >= IPR_TIMEOUT_MASK) {
			timeouts.record[ARRAY_SIZE(ipr_dasd_timeouts)].timeout =
				htons((format_timeout / 60) | IPR_TIMEOUT_MINUTE_RADIX);
		} else {
			timeouts.record[ARRAY_SIZE(ipr_dasd_timeouts)].timeout =
				htons(format_timeout);
		}
	}

	timeouts.length = htonl(len);
	memset(cdb, 0, IPR_CCB_CDB_LEN);
	cdb[0] = SET_DASD_TIMEOUTS;
	cdb[7] = (len >> 8) & 0xff;
	cdb[8] = len & 0xff;

	rc = sg_ioctl(fd, cdb, &timeouts,
		      len, SG_DXFER_TO_DEV,
		      &sense_data, SET_DASD_TIMEOUTS_TIMEOUT);

	if (rc != 0)
		scsi_cmd_err(dev, &sense_data, "Set DASD timeouts", rc);

	close(fd);
	return rc;
}

/**
 * ipr_get_dev_attr -
 * @ioa:		ipr ioa struct
 * @attr:		ipr_disk_attr struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_get_dev_attr(struct ipr_dev *dev, struct ipr_disk_attr *attr)
{
	char temp[100];
	struct ipr_mode_pages mode_pages;
	struct ipr_ioa_mode_page *page;

	if (ipr_read_dev_attr(dev, "queue_depth", temp, 100))
		return -EIO;

	if (ipr_is_af_dasd_device(dev)) {
		memset(&mode_pages, 0, sizeof(mode_pages));

		if (ipr_mode_sense(dev, 0x20, &mode_pages))
			return -EIO;

		page = (struct ipr_ioa_mode_page *) (((u8 *)&mode_pages) +
						     mode_pages.hdr.block_desc_len +
						     sizeof(mode_pages.hdr));

		attr->queue_depth = page->max_tcq_depth;
	} else
		attr->queue_depth = strtoul(temp, NULL, 10);

	if (ipr_is_af_dasd_device(dev)) {
		if (attr->queue_depth < 2)
			attr->tcq_enabled = 0;
		else
			attr->tcq_enabled = 1;
	} else
		attr->tcq_enabled = is_tagged(dev);

	attr->format_timeout = get_format_timeout(dev);

	if (ipr_is_gscsi(dev) || ipr_is_volume_set(dev)) {
		if (ipr_dev_wcache_policy(dev) == IPR_DEV_CACHE_WRITE_BACK)
			attr->write_cache_policy = IPR_DEV_CACHE_WRITE_BACK;
		else
			attr->write_cache_policy = IPR_DEV_CACHE_WRITE_THROUGH;
	}

	return 0;
}

int ipr_known_zeroed_is_saved(struct ipr_dev *dev)
{
	int len;
	struct ipr_mode_pages mode_pages;
	struct ipr_ioa_mode_page *page;

	memset(&mode_pages, 0, sizeof(mode_pages));

	if (!ipr_mode_sense(dev, 0x20, &mode_pages)) {
		page = (struct ipr_ioa_mode_page *) (((u8 *)&mode_pages) +
						     mode_pages.hdr.block_desc_len +
						     sizeof(mode_pages.hdr));

		if (page->format_completed)
			return 1;
	}

	return 0;
}

int ipr_set_format_completed_bit(struct ipr_dev *dev)
{
	int len, retries = 5;
	struct ipr_mode_pages mode_pages;
	struct ipr_ioa_mode_page *page;

	scsi_dbg(dev, "Setting device formatted bit. Device ID=%lx\n", dev->scsi_dev_data->device_id);

	memset(&mode_pages, 0, sizeof(mode_pages));

	do {
		if (!ipr_mode_sense(dev, 0x20, &mode_pages))
			break;
		sleep(1);
	} while (retries--);

	if (!retries) {
		scsi_info(dev, "Page 20 mode sense failed. Device ID=%lx\n", dev->scsi_dev_data->device_id);
		return -EIO;
	}

	page = (struct ipr_ioa_mode_page *) (((u8 *)&mode_pages) +
						     mode_pages.hdr.block_desc_len +
						     sizeof(mode_pages.hdr));

	page->format_completed = 1;

	len = mode_pages.hdr.length + 1;
	mode_pages.hdr.length = 0;
	mode_pages.hdr.medium_type = 0;
	mode_pages.hdr.device_spec_parms = 0;
	page->hdr.parms_saveable = 0;

	do {
		if (!ipr_mode_select(dev, &mode_pages, len))
			break;
		sleep(1);
	} while (retries--);

	if (!retries) {
		scsi_info(dev, "Page 20 mode select failed. Device ID=%lx\n", dev->scsi_dev_data->device_id);
		return -EIO;
	}

	return 0;
}
/**
 * get_ioa_caching -
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   0
 **/
int get_ioa_caching(struct ipr_ioa *ioa)
{
	int rc;
	int found = 0;
	struct ipr_query_ioa_caching_info info;
	struct ipr_global_cache_params_term *term;
	int term_size = sizeof(struct ipr_query_ioa_caching_info);

	memset(&info, 0, term_size);

	rc = ipr_query_cache_parameters(ioa, &info, term_size);
	if (rc)
		return IPR_IOA_REQUESTED_CACHING_DEFAULT;

	for_each_cache_term(term, &info)
		if (term && term->term_id == IPR_CACHE_PARAM_TERM_ID) {
			found = 1;
			break;
		}

	if (found == 1)
		if (term->enable_caching_dual_ioa_failure == IPR_IOA_CACHING_DUAL_FAILURE_ENABLED)
			if (term->disable_caching_requested == IPR_IOA_REQUESTED_CACHING_DISABLED)
				return IPR_IOA_CACHING_DISABLED_DUAL_ENABLED;
			else
				return IPR_IOA_CACHING_DEFAULT_DUAL_ENABLED;
		else if (term->vset_write_cache_enabled)
			return IPR_IOA_VSET_CACHE_ENABLED;
		else
			if (term->disable_caching_requested == IPR_IOA_REQUESTED_CACHING_DISABLED)
				return IPR_IOA_REQUESTED_CACHING_DISABLED;
			else
				return IPR_IOA_REQUESTED_CACHING_DEFAULT;
	else
		return IPR_IOA_REQUESTED_CACHING_DEFAULT;
}

/**
 * ipr_get_ioa_attr - 
 * @ioa:		ipr ioa struct
 * @attr:		ipr_ioa_attr struct
 *
 * Returns:
 *   0
 **/
int ipr_get_ioa_attr(struct ipr_ioa *ioa, struct ipr_ioa_attr *attr)
{
	attr->preferred_primary = 0;
	attr->gscsi_only_ha = ioa->in_gscsi_only_ha;
	attr->active_active = ioa->asymmetric_access_enabled;
	attr->caching = get_ioa_caching(ioa);
	attr->rebuild_rate = ioa->rebuild_rate;
	attr->disable_rebuild_verify = ioa->disable_rebuild_verify;

	if (!ioa->dual_raid_support)
		return 0;

	if (ioa->ioa_status.cap.preferred_role == IPR_IOA_STATE_PRIMARY)
		attr->preferred_primary = 1;
	return 0;
}

/**
 * ipr_modify_dev_attr - 
 * @dev:		ipr dev struct
 * @attr:		ipr_disk_attr struct
 *
 * Returns:
 *   0
 **/
int ipr_modify_dev_attr(struct ipr_dev *dev, struct ipr_disk_attr *attr)
{
	char temp[100];
	int rc;

	rc = ipr_get_saved_dev_attr(dev, IPR_QUEUE_DEPTH, temp);
	if (rc == RC_SUCCESS)
		sscanf(temp, "%d", &attr->queue_depth);

	rc = ipr_get_saved_dev_attr(dev, IPR_TCQ_ENABLED, temp);
	if (rc == RC_SUCCESS)
		sscanf(temp, "%d", &attr->tcq_enabled);

	rc = ipr_get_saved_dev_attr(dev, IPR_FORMAT_TIMEOUT, temp);
	if (rc == RC_SUCCESS)
		sscanf(temp, "%d", &attr->format_timeout);

	rc = ipr_get_saved_dev_attr(dev, IPR_WRITE_CACHE_POLICY, temp);
	if (rc == RC_SUCCESS)
		sscanf(temp, "%d", &attr->write_cache_policy);
	else
		attr->write_cache_policy = IPR_DEV_CACHE_WRITE_BACK;

	return 0;
}

/**
 * ipr_modify_ioa_attr - 
 * @ioa:		ipr ioa struct
 * @attr:		ipr_ioa_attr struct
 *
 * Returns:
 *   0
 **/
int ipr_modify_ioa_attr(struct ipr_ioa *ioa, struct ipr_ioa_attr *attr)
{
	char temp[100];
	int rc;

	rc = ipr_get_saved_ioa_attr(ioa, IPR_GSCSI_HA_ONLY, temp);
	if (rc == RC_SUCCESS)
		sscanf(temp, "%d", &attr->gscsi_only_ha);

	rc = ipr_get_saved_ioa_attr(ioa, IPR_DUAL_ADAPTER_ACTIVE_ACTIVE, temp);
	if (rc == RC_SUCCESS)
		sscanf(temp, "%d", &attr->active_active);

	return 0;
}

/**
 * ipr_set_dev_attr - 
 * @dev:		ipr dev struct
 * @attr:		ipr_disk_attr struct
 * @save:		save flag
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_set_dev_attr(struct ipr_dev *dev, struct ipr_disk_attr *attr, int save)
{
	struct ipr_disk_attr old_attr;
	char temp[100];

	if (ipr_get_dev_attr(dev, &old_attr))
		return -EIO;

	if (attr->queue_depth != old_attr.queue_depth) {
		sprintf(temp, "%d", attr->queue_depth);
		if (ipr_is_af_dasd_device(dev)) {
			if (ipr_setup_af_qdepth(dev, attr->queue_depth))
				return -EIO;
		} else {
			if (ipr_write_dev_attr(dev, "queue_depth", temp))
				return -EIO;

			if (dev->alt_path && ipr_write_dev_attr(dev->alt_path,
								"queue_depth",
								temp))
				return -EIO;
		}
		if (save)
			ipr_save_dev_attr(dev, IPR_QUEUE_DEPTH, temp, 1);
	}

	if (attr->format_timeout != old_attr.format_timeout) {
		if (ipr_is_af_dasd_device(dev)) {
			sprintf(temp, "%d", attr->format_timeout);
			if (ipr_set_dasd_timeouts(dev, attr->format_timeout))
				return -EIO;
			if (save)
				ipr_save_dev_attr(dev, IPR_FORMAT_TIMEOUT, temp, 1);
		}
	}

	if (attr->tcq_enabled != old_attr.tcq_enabled) {
		if (!ipr_is_af_dasd_device(dev)) {
			if (set_tagged(dev, attr->tcq_enabled))
				return -EIO;
			sprintf(temp, "%d", attr->tcq_enabled);
			if (save)
				ipr_save_dev_attr(dev, IPR_TCQ_ENABLED, temp, 1);
		}
	}

	if (ipr_is_gscsi(dev) || ipr_is_volume_set(dev)) {
		if (attr->write_cache_policy != old_attr.write_cache_policy
		    || !attr->write_cache_policy) {
			ipr_set_dev_wcache_policy(dev, attr->write_cache_policy);
			if (save) {
				sprintf(temp, "%d", attr->write_cache_policy);
				ipr_save_dev_attr(dev, IPR_WRITE_CACHE_POLICY, temp, 1);
			}
		}
	}

	return 0;
}

/**
 * ipr_set_ioa_attr - 
 * @ioa:		ipr ioa struct
 * @attr:		ipr_ioa_attr struct
 * @save:		save flag
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_set_ioa_attr(struct ipr_ioa *ioa, struct ipr_ioa_attr *attr, int save)
{
	struct ipr_ioa_attr old_attr;
	char temp[100];
	int mode;
	int rc;

	if (ipr_get_ioa_attr(ioa, &old_attr))
		return -EIO;

	if (ioa->has_vset_write_cache && attr->vset_write_cache) {
		/* vset cache should not be disabled adapter-wide
		 for any reason.  So we don't save the parameter here. */
		ipr_change_cache_parameters(ioa,
					    IPR_IOA_SET_VSET_CACHE_ENABLED);
	}

	/* FIXME - preferred_primary and active_active may change at the same
	 * time.  This code may need to change.
 	 */

	if (attr->preferred_primary != old_attr.preferred_primary)
		if (ipr_change_multi_adapter_assignment(ioa, attr->preferred_primary, IPR_PRESERVE_ASYMMETRIC_STATE))
			return -EIO;

	if (attr->gscsi_only_ha != old_attr.gscsi_only_ha) {
		sprintf(temp, "%d", attr->gscsi_only_ha);

		if (ipr_set_ha_mode(ioa, attr->gscsi_only_ha))
			return -EIO;
		if (save)
			ipr_save_ioa_attr(ioa, IPR_GSCSI_HA_ONLY, temp, 1);
		ipr_reset_adapter(ioa);
	}

	if (attr->active_active != old_attr.active_active && ioa->asymmetric_access) {
		sprintf(temp, "%d", attr->active_active);

		/* If setting active/active, use mode page 24.
		 * If clearing, reset the adapter and then use
		 * Change Multi Adapter Assignment. */
		if (attr->active_active) {
			if (ipr_set_active_active_mode(ioa))
				return -EIO;
			if (save)
				ipr_save_ioa_attr(ioa, IPR_DUAL_ADAPTER_ACTIVE_ACTIVE, temp, 1);
		} else {
			if (save)
				ipr_save_ioa_attr(ioa, IPR_DUAL_ADAPTER_ACTIVE_ACTIVE, temp, 1);
			ipr_reset_adapter(ioa);
		       	if (ipr_change_multi_adapter_assignment(ioa,
							       attr->preferred_primary,
							       attr->active_active))
				return -EIO;
		}
	}

	if (attr->caching != old_attr.caching) {
		if (attr->caching == IPR_IOA_REQUESTED_CACHING_DEFAULT)
			mode = IPR_IOA_SET_CACHING_DEFAULT;
		else
			mode = IPR_IOA_SET_CACHING_DISABLED;
		ipr_change_cache_parameters(ioa, mode);
	}

	if (attr->rebuild_rate != old_attr.rebuild_rate) {
		rc = ipr_set_array_rebuild_rate(ioa, attr->rebuild_rate);
		if (rc)
			return rc;

		if (save) {
			sprintf(temp, "%d", attr->rebuild_rate);
			ipr_save_ioa_attr(ioa, IPR_ARRAY_REBUILD_RATE, temp, 1);
		}
	}

	if (attr->disable_rebuild_verify != old_attr.disable_rebuild_verify) {
		rc = ipr_set_array_rebuild_verify(ioa,
						  attr->disable_rebuild_verify);
		if (rc)
			return rc;

		if (save) {
			sprintf(temp, "%d", attr->disable_rebuild_verify);
			ipr_save_ioa_attr(ioa, IPR_ARRAY_DISABLE_REBUILD_VERIFY,
					  temp, 1);
		}
	}

	get_dual_ioa_state(ioa);	/* for preferred_primary */
	get_subsys_config(ioa);		/* for gscsi_only_ha */
	return 0;
}

/**
 * ipr_query_dasd_timeouts - 
 * @dev:		ipr dev struct
 * @timeouts:		ipr_query_dasd_timesouts struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_query_dasd_timeouts(struct ipr_dev *dev,
			    struct ipr_query_dasd_timeouts *timeouts)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	char *name = dev->gen_name;

	fd = open(name, O_RDWR);

	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(timeouts, 0, sizeof(*timeouts));

	memset(cdb, 0, IPR_CCB_CDB_LEN);
	cdb[0] = QUERY_DASD_TIMEOUTS;
	cdb[7] = (sizeof(*timeouts) >> 8) & 0xff;
	cdb[8] = sizeof(*timeouts) & 0xff;

	rc = sg_ioctl(fd, cdb, timeouts,
		      sizeof(*timeouts), SG_DXFER_FROM_DEV,
		      &sense_data, SET_DASD_TIMEOUTS_TIMEOUT);

	if (rc != 0)
		scsi_cmd_err(dev, &sense_data, "Query DASD timeouts", rc);

	close(fd);
	return rc;
}

/**
 * ipr_get_bus_attr - 
 * @ioa:		ipr ioa struct
 * @sbus:		ipr_scsi_buses struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_get_bus_attr(struct ipr_ioa *ioa, struct ipr_scsi_buses *sbus)
{
	struct ipr_mode_pages mode_pages;
	struct ipr_mode_page_28 *page_28;
	struct ipr_mode_page_28_scsi_dev_bus_attr *bus;
	int rc, i, busno;

	memset(&mode_pages, 0, sizeof(mode_pages));
	memset(sbus, 0, sizeof(*sbus));

	rc = ipr_mode_sense(&ioa->ioa, 0x28, &mode_pages);

	if (rc)
		return rc;

	page_28 = (struct ipr_mode_page_28 *)(mode_pages.data + mode_pages.hdr.block_desc_len);

	for_each_bus_attr(bus, page_28, i) {
		busno = bus->res_addr.bus;
		sbus->bus[busno].max_xfer_rate = ntohl(bus->max_xfer_rate);
		sbus->bus[busno].qas_capability = bus->qas_capability;
		sbus->bus[busno].scsi_id = bus->scsi_id;
		sbus->bus[busno].bus_width = bus->bus_width;
		sbus->bus[busno].extended_reset_delay = bus->extended_reset_delay;
		sbus->bus[busno].min_time_delay = bus->min_time_delay;
		sbus->num_buses++;
	}

	return 0;
}

/**
 * ipr_set_bus_attr - 
 * @ioa:		ipr ioa struct
 * @sbus:		ipr_scsi_buses struct
 * @save:		save flag
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_set_bus_attr(struct ipr_ioa *ioa, struct ipr_scsi_buses *sbus, int save)
{
	struct ipr_mode_pages mode_pages;
	struct ipr_mode_page_28 *page_28;
	struct ipr_mode_page_28_scsi_dev_bus_attr *bus;
	struct ipr_scsi_buses old_settings;
	int rc, i, busno, len;
	int reset_needed = 0;
	char value_str[64];

	memset(&mode_pages, 0, sizeof(mode_pages));

	rc = ipr_mode_sense(&ioa->ioa, 0x28, &mode_pages);

	if (rc)
		return rc;

	rc = ipr_get_bus_attr(ioa, &old_settings);

	if (rc)
		return rc;

	page_28 = (struct ipr_mode_page_28 *)
		(mode_pages.data + mode_pages.hdr.block_desc_len);

	for_each_bus_attr(bus, page_28, i) {
		busno = bus->res_addr.bus;
		bus->bus_width = sbus->bus[busno].bus_width;
		if (save && bus->bus_width != old_settings.bus[busno].bus_width) {
			sprintf(value_str, "%d", bus->bus_width);
			ipr_save_bus_attr(ioa, i, IPR_BUS_WIDTH, value_str, 1);
		}

		bus->max_xfer_rate = htonl(sbus->bus[busno].max_xfer_rate);
		if (save && bus->max_xfer_rate != old_settings.bus[busno].max_xfer_rate) {
			sprintf(value_str, "%d",
				IPR_BUS_XFER_RATE_TO_THRUPUT(sbus->bus[busno].max_xfer_rate,
							     bus->bus_width));
			ipr_save_bus_attr(ioa, i, IPR_MAX_XFER_RATE_STR, value_str, 1);
		}

		bus->qas_capability = sbus->bus[busno].qas_capability;
		if (save && bus->qas_capability != old_settings.bus[busno].qas_capability) {
			sprintf(value_str, "%d", bus->qas_capability);
			ipr_save_bus_attr(ioa, i, IPR_QAS_CAPABILITY, value_str, 1);
		}

		if (bus->scsi_id != sbus->bus[busno].scsi_id)
			reset_needed = 1;

		bus->scsi_id = sbus->bus[busno].scsi_id;
		if (save && bus->scsi_id != old_settings.bus[busno].scsi_id) {
			sprintf(value_str, "%d", bus->scsi_id);
			ipr_save_bus_attr(ioa, i, IPR_HOST_SCSI_ID, value_str, 1);
		}

		bus->min_time_delay = sbus->bus[busno].min_time_delay;
		if (save && bus->min_time_delay != old_settings.bus[busno].min_time_delay) {
			sprintf(value_str, "%d", bus->min_time_delay);
			ipr_save_bus_attr(ioa, i, IPR_MIN_TIME_DELAY, value_str, 1);
		}

		bus->extended_reset_delay = sbus->bus[busno].extended_reset_delay;
	}

	len = mode_pages.hdr.length + 1;
	mode_pages.hdr.length = 0;

	rc = ipr_mode_select(&ioa->ioa, &mode_pages, len);

	if (reset_needed)
		ipr_reset_adapter(ioa);

	return rc;
}

/**
 * ipr_modify_bus_attr - 
 * @ioa:		ipr ioa struct
 * @sbus:		ipr_scsi_buses struct
 *
 * Returns:
 *   nothing
 **/
void ipr_modify_bus_attr(struct ipr_ioa *ioa, struct ipr_scsi_buses *sbus)
{
	int i, rc, saved_value, max_xfer_rate;
	char value_str[64];

	for (i = 0; i < sbus->num_buses; i++) {
		rc = ipr_get_saved_bus_attr(ioa, i, IPR_QAS_CAPABILITY, value_str);
		if (rc == RC_SUCCESS) {
			sscanf(value_str, "%d", &saved_value);
			sbus->bus[i].qas_capability = saved_value;
		}

		if (ioa->scsi_id_changeable) {
			rc = ipr_get_saved_bus_attr(ioa, i, IPR_HOST_SCSI_ID, value_str);
			if (rc == RC_SUCCESS) {
				sscanf(value_str, "%d", &saved_value);
				sbus->bus[i].scsi_id = saved_value;
			}
		}

		rc = ipr_get_saved_bus_attr(ioa, i, IPR_BUS_WIDTH, value_str);
		if (rc == RC_SUCCESS) {
			sscanf(value_str, "%d", &saved_value);
			sbus->bus[i].bus_width = saved_value;
		}

		max_xfer_rate = get_max_bus_speed(ioa, i);

		rc = ipr_get_saved_bus_attr(ioa, i, IPR_MAX_XFER_RATE_STR, value_str);
		if (rc == RC_SUCCESS) {
			sscanf(value_str, "%d", &saved_value);

			if (saved_value <= max_xfer_rate) {
				sbus->bus[i].max_xfer_rate =
					IPR_BUS_THRUPUT_TO_XFER_RATE(saved_value, sbus->bus[i].bus_width);
			} else {
				sbus->bus[i].max_xfer_rate =
					IPR_BUS_THRUPUT_TO_XFER_RATE(max_xfer_rate, sbus->bus[i].bus_width);
				sprintf(value_str, "%d", max_xfer_rate);
				ipr_save_bus_attr(ioa, i, IPR_MAX_XFER_RATE_STR, value_str, 1);
			}
		} else {
			sbus->bus[i].max_xfer_rate =
				IPR_BUS_THRUPUT_TO_XFER_RATE(max_xfer_rate, sbus->bus[i].bus_width);
		}

		rc = ipr_get_saved_bus_attr(ioa, i, IPR_MIN_TIME_DELAY, value_str);

		if (rc == RC_SUCCESS) {
			sscanf(value_str,"%d", &saved_value);
			sbus->bus[i].min_time_delay = saved_value;
		} else {
			sbus->bus[i].min_time_delay = IPR_INIT_SPINUP_DELAY;
		}
	}
}

/**
 * get_unsupp_af - 
 * @ing:		ipr_std_ing_data struct
 * @page3:		ipr_dasd_inquiry_page3 struct
 *
 * Returns:
 *   unsupported_af_dasd struct
 **/
struct unsupported_af_dasd *
get_unsupp_af(struct ipr_std_inq_data *inq,
	      struct ipr_dasd_inquiry_page3 *page3)
{
	int i, j;

	for (i = 0; i < ARRAY_SIZE(unsupported_af); i++) {
		for (j = 0; j < IPR_VENDOR_ID_LEN; j++) {
			if (unsupported_af[i].compare_vendor_id_byte[j] &&
			    unsupported_af[i].vendor_id[j] != inq->vpids.vendor_id[j])
				break;
		}

		if (j != IPR_VENDOR_ID_LEN)
			continue;

		for (j = 0; j < IPR_PROD_ID_LEN; j++) {
			if (unsupported_af[i].compare_product_id_byte[j] &&
			    unsupported_af[i].product_id[j] != inq->vpids.product_id[j])
				break;
		}

		if (j != IPR_PROD_ID_LEN)
			continue;

		for (j = 0; j < 4; j++) {
			if (unsupported_af[i].lid_mask[j] &&
			    unsupported_af[i].lid[j] != page3->load_id[j])
				break;
		}

		if (j != 4)
			continue;

		return &unsupported_af[i];
	}
	return NULL;
}

/**
 * disk_needs_msl - 
 * @unsupp_af:		unsupported_af_dasd struct
 * @inq:		ipr_std_inq_data struct
 *
 * Returns:
 *   true if needs msl / false otherwise
 **/
bool disk_needs_msl(struct unsupported_af_dasd *unsupp_af,
		    struct ipr_std_inq_data *inq)
{
	u32 ros_rcv_ram_rsvd, min_ucode_level;
	int j;

	if (unsupp_af->supported_with_min_ucode_level) {
		min_ucode_level = 0;
		ros_rcv_ram_rsvd = 0;

		for (j = 0; j < 4; j++) {
			if (unsupp_af->min_ucode_mask[j]) {
				min_ucode_level = (min_ucode_level << 8) |
					unsupp_af->min_ucode_level[j];
				ros_rcv_ram_rsvd = (ros_rcv_ram_rsvd << 8) |
					inq->ros_rsvd_ram_rsvd[j];
			}
		}

		if (min_ucode_level > ros_rcv_ram_rsvd)
			return true;
	}

	return false;
}

/**
 * is_af_blocked - 
 * @dev:		ipr dev struct
 * @silent:		
 *
 * Returns:
 *   true if blocked / false otherwise
 **/
bool is_af_blocked(struct ipr_dev *dev, int silent)
{
	int rc;
	struct ipr_std_inq_data std_inq_data;
	struct ipr_dasd_inquiry_page3 dasd_page3_inq;
	struct unsupported_af_dasd *unsupp_af;

	/* Zero out inquiry data */
	memset(&std_inq_data, 0, sizeof(std_inq_data));
	rc = ipr_inquiry(dev, IPR_STD_INQUIRY,
			 &std_inq_data, sizeof(std_inq_data));

	if (rc != 0)
		return false;

	/* Issue page 3 inquiry */
	memset(&dasd_page3_inq, 0, sizeof(dasd_page3_inq));
	rc = ipr_inquiry(dev, 0x03,
			 &dasd_page3_inq, sizeof(dasd_page3_inq));

	if (rc != 0)
		return false;

	unsupp_af = get_unsupp_af(&std_inq_data, &dasd_page3_inq);

	if (!unsupp_af)
		return false;

	/* If we make it this far, we have a match into the table.  Now,
	 determine if we need a certain level of microcode or if this
	 disk is not supported all together. */
	if (unsupp_af->supported_with_min_ucode_level) {
		if (disk_needs_msl(unsupp_af, &std_inq_data)) {
			if (ipr_force) {
				if (!silent)
					scsi_err(dev, "Disk %s needs updated microcode "
						 "before transitioning to 522 bytes/sector "
						 "format. IGNORING SINCE --force USED!",
						 dev->gen_name);
				return false;
			} else {
				if (!silent)
					scsi_err(dev, "Disk %s needs updated microcode "
					       "before transitioning to 522 bytes/sector "
					       "format.", dev->gen_name);
				return true;
			}
		}
	} else {/* disk is not supported at all */
		if (!silent)
			syslog(LOG_ERR,"Disk %s canot be formatted to "
			       "522 bytes/sector.", dev->gen_name);
		return true;
	}

	return false;
}

/**
 * ipr_read_dev_attr -
 * @dev:		ipr dev struct
 * @attr:
 * @value:
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_read_dev_attr(struct ipr_dev *dev, char *attr,
		      char *value, size_t value_len)
{
	char *sysfs_dev_name;
	char devpath[PATH_MAX];
	ssize_t len;

	if (!dev->scsi_dev_data) {
		scsi_dbg(dev, "Cannot read dev attr %s. NULL scsi data\n", attr);
		return -ENOENT;
	}
	sysfs_dev_name = dev->scsi_dev_data->sysfs_device_name;
	sprintf(devpath, "/sys/class/scsi_device/%s/device", sysfs_dev_name);
	len = sysfs_read_attr(devpath, attr, value, value_len);
	if (len < 0) {
		scsi_dbg(dev, "Failed to read %s attribute. %m\n", attr);
		return -EIO;
	}
	return 0;
}

/**
 * ipr_write_dev_attr -
 * @dev:		ipr dev struct
 * @attr:
 * @value:
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_write_dev_attr(struct ipr_dev *dev, char *attr, char *value)
{
	char *sysfs_dev_name;
	char devpath[PATH_MAX];

	if (!dev->scsi_dev_data)
		return -ENOENT;

	sysfs_dev_name = dev->scsi_dev_data->sysfs_device_name;
	sprintf(devpath, "/sys/class/scsi_device/%s/device", sysfs_dev_name);
	if (sysfs_write_attr(devpath, attr, value, strlen(value)) < 0) {
		scsi_dbg(dev, "Failed to write attribute: %s\n", attr);
		return -EIO;
	}
	return 0;
}

/**
 * get_ucode_date - 
 * @ucode_file:		microcode file name
 * @ucode_date:		microcode date string pointer
 * @max_size:		max size for the date field
 *
 * Returns:
 *   nothing
 **/
void get_ucode_date(char *ucode_file, char *ucode_date, int max_size)
{
	struct stat st;
	struct tm *file_tm;

	ucode_date[0] = '\0';
	if (stat(ucode_file, &st))
		return;

	file_tm = localtime(&st.st_mtime);
	if (!file_tm)
		return;

	strftime(ucode_date, max_size, "%D", file_tm);
}

/**
 * get_ioa_ucode_version - 
 * @ucode_file:		microcode file name
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
u32 get_ioa_ucode_version(char *ucode_file)
{
	int fd, rc;
	struct stat ucode_stats;
	struct ipr_ioa_ucode_header *image_hdr;

	fd = open(ucode_file, O_RDONLY);

	if (fd == -1)
		return 0;

	rc = fstat(fd, &ucode_stats);

	if (rc != 0) {
		close(fd);
		return 0;
	}

	image_hdr = mmap(NULL, ucode_stats.st_size,
			 PROT_READ, MAP_SHARED, fd, 0);

	if (image_hdr == MAP_FAILED) {
		close(fd);
		return 0;
	}

	rc = ntohl(image_hdr->rev_level);
	munmap(image_hdr, ucode_stats.st_size);
	close(fd);

	return rc;
}

/**
 * fw_compare - compare two firmware images
 * @parm1:		pointer to firmware image
 * @parm2:		pointer to firmware image
 *
 * Returns:
 *   0 if the images are the same / non-zero otherwise
 **/
/* Sort in decending order */
static int fw_compare(const void *parm1,
		      const void *parm2)
{
	struct ipr_fw_images *first = (struct ipr_fw_images *)parm1;
	struct ipr_fw_images *second = (struct ipr_fw_images *)parm2;

	if (first->version < second->version)
		return 1;
	if (second->version > first->version)
		return -1;
	return 0;
}

/**
 * ipr_get_hotplug_dir - 
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int ipr_get_hotplug_dir()
{
	FILE *file;
	char buf[100];
	char *loc, *end;

	file = fopen(FIRMWARE_HOTPLUG_CONFIG_FILE, "r");

	if (!file) {
		hotplug_dir = realloc(hotplug_dir, strlen(FIRMWARE_HOTPLUG_DEFAULT_DIR) + 1);
		if (!hotplug_dir)
			return -ENOMEM;

		strcpy(hotplug_dir, FIRMWARE_HOTPLUG_DEFAULT_DIR);
		return 0;
	}

	clearerr(file);
	do {
		if (feof(file)) {
			syslog(LOG_ERR, "Failed parsing %s. Reached end of file.\n",
			       FIRMWARE_HOTPLUG_CONFIG_FILE);
			return -EIO;
		}
		fgets(buf, 100, file);
		loc = strstr(buf, FIRMWARE_HOTPLUG_DIR_TAG);
	} while(!loc || buf[0] == '#');

	loc = strchr(buf, '/');

	fclose(file);

	if (!loc) {
		syslog(LOG_ERR, "Failed parsing %s.\n",  FIRMWARE_HOTPLUG_CONFIG_FILE);
		return -EIO;
	}

	end = strchr(loc, ' ');

	if (!end)
		end = strchr(loc, '"');
	if (end)
		*end = '\0';

	hotplug_dir = realloc(hotplug_dir, strlen(loc) + 1);

	if (!hotplug_dir)
		return -ENOMEM;

	strcpy(hotplug_dir, loc);
	end = strchr(hotplug_dir, '\n');
	if (end)
		*end = '\0';

	return 0;
}

/**
 * get_dasd_ucode_version - 
 * @ucode_file:		file name of microcode file
 * @has_hdr:		has header flag
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
u32 get_dasd_ucode_version(char *ucode_file, int has_hdr)
{
	int fd;
	unsigned int len;
	struct stat ucode_stats;
	struct ipr_dasd_ucode_header *hdr;
	char *tmp;
	u32 rc;

	if (has_hdr) {
		fd = open(ucode_file, O_RDONLY);

		if (fd == -1)
			return 0;

		rc = fstat(fd, &ucode_stats);

		if (rc != 0) {
			fprintf(stderr, "Failed to stat %s\n", ucode_file);
			close(fd);
			return 0;
		}

		hdr = mmap(NULL, ucode_stats.st_size, PROT_READ,
			   MAP_SHARED, fd, 0);

		if (hdr == MAP_FAILED) {
			fprintf(stderr, "mmap of %s failed\n", ucode_file);
			close(fd);
			return 0;
		}

		len = (hdr->length[0] << 16) | (hdr->length[1] << 8) | hdr->length[2];

		if (len == ucode_stats.st_size) {
			rc = (hdr->modification_level[0] << 24) |
				(hdr->modification_level[1] << 16) |
				(hdr->modification_level[2] << 8) |
				hdr->modification_level[3];

			munmap(hdr, ucode_stats.st_size);
			close(fd);
			return rc;
		} else {
			munmap(hdr, ucode_stats.st_size);
			close(fd);		
		}
	}

	tmp = strrchr(ucode_file, '.');

	if (!tmp)
		return 0;

	rc = strtoul(tmp+1, NULL, 16);

	return rc;
}

/**
 * get_ses_ucode_version - 
 * @ucode_file:		
 *
 * Returns:
 *   ses microcode version / 0 if failure
 **/
u32 get_ses_ucode_version(char *ucode_file)
{
	char *tmp = strrchr(ucode_file, '.');

	if (!tmp)
		return 0;
	if (strlen(tmp+1) < 4)
		return 0;

	return (tmp[1] << 24) | (tmp[2] << 16) | (tmp[3] << 8) | tmp[4];
}

/**
 * get_dev_fw_version - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   device firmware version / 0 if failure
 **/
u32 get_dev_fw_version(struct ipr_dev *dev)
{
	u8 release_level[4];
	int rc;

	rc = ipr_get_fw_version(dev, release_level);

	if (rc != 0) {
		scsi_dbg(dev, "Inquiry failed\n");
		return 0;
	}

	rc = release_level[0] << 24 | release_level[1] << 16 |
		release_level[2] << 8 | release_level[3];
	return rc;
}

/**
 * get_ioa_fw_version -
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   ioa firmware version
 **/
static u32 get_ioa_fw_version(struct ipr_ioa *ioa)
{
	char devpath[PATH_MAX];
	char value[16];
	ssize_t len;
	u32 fw_version;

	sprintf(devpath, "/sys/class/scsi_host/%s", ioa->host_name);
	len = sysfs_read_attr(devpath, "fw_version", value, 16);
	if (len < 0)
		return -1;
	sscanf(value, "%8X", &fw_version);

	return fw_version;
}

/**
 * get_fw_version - Get microcode version of device.
 *
 * @dev:		Device
 *
 * Returns:
 *   ucode version if success / 0 on failure
 **/
u32 get_fw_version(struct ipr_dev *dev)
{
	if (!dev) {
		/* FIXME: We should return -ENODEV here but old API
		returns 0 on failure and any uint > 0 can be a firmware
		level. A viable option would be writting the fw level to
		a pointer received as argument, but lets hold to the
		current API for now. */
		return 0;
	}

	if (ipr_is_ioa(dev))
		return get_ioa_fw_version(dev->ioa);

	return get_dev_fw_version(dev);
}

/**
 * get_ioa_image_type -
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   ioa image type
 **/
static u8 get_ioa_image_type(struct ipr_ioa *ioa)
{
	u32 fw_version = get_ioa_fw_version(ioa);
	return ((fw_version & 0x00ff0000) >> 16);
}

/**
 * get_ioa_fw_name -
 * @ioa:		ipr ioa struct
 * @buf:		data buffer
 *
 * Returns:
 *   nothing
 **/
static void get_ioa_fw_name(struct ipr_ioa *ioa, char *buf)
{
	const struct ioa_parms *ioa_parms = get_ioa_fw(ioa);

	if (ioa_parms)
		strcpy(buf, ioa_parms->fw_name);
	else
		sprintf(buf, "534953%02X", get_ioa_image_type(ioa));
}

/**
 * get_linux_ioa_fw_name -
 * @ioa:		ipr ioa struct
 * @buf:		data buffer
 *
 * Returns:
 *   nothing
 **/
static void get_linux_ioa_fw_name(struct ipr_ioa *ioa, char *buf)
{
	sprintf(buf, "pci.%04x%04x.%02x", ioa->pci_vendor, ioa->pci_device,
		get_ioa_image_type(ioa));
}

/**
 * get_linux_ioa_fw_name_capital -
 * @ioa:		ipr ioa struct
 * @buf:		data buffer
 *
 * Returns:
 *   nothing
 **/
static void get_linux_ioa_fw_name_capital(struct ipr_ioa *ioa, char *buf)
{
	sprintf(buf, "pci.%04X%04X.%02X", ioa->pci_vendor, ioa->pci_device,
		get_ioa_image_type(ioa));
}

/**
 * init_ioa_ucode_entry - 
 * @img:		ipr_fw_images struct
 *
 * Returns:
 *   nothing
 **/
static void init_ioa_ucode_entry(struct ipr_fw_images *img)
{
	img->version = get_ioa_ucode_version(img->file);
	img->has_header = 0;
	get_ucode_date(img->file, img->date, sizeof(img->date));
}

/**
 * init_disk_ucode_entry - 
 * @img:		ipr_fw_images
 *
 * Returns:
 *   nothing
 **/
static void init_disk_ucode_entry(struct ipr_fw_images *img)
{
	img->version = get_dasd_ucode_version(img->file, 1);
	img->has_header = 1;
	get_ucode_date(img->file, img->date, sizeof(img->date));
}

/**
 * init_disk_ucode_entry_nohdr -
 * @img:		ipr_fw_images struct
 *
 * Returns:
 *   nothing
 **/
static void init_disk_ucode_entry_nohdr(struct ipr_fw_images *img)
{
	img->version = get_dasd_ucode_version(img->file, 0);
	img->has_header = 0;
	get_ucode_date(img->file, img->date, sizeof(img->date));
}

/**
 * init_ses_ucode_entry_nohdr - 
 * @img:		ipr_fw_images struct
 *
 * Returns:
 *   nothing
 **/
static void init_ses_ucode_entry_nohdr(struct ipr_fw_images *img)
{
	img->version = get_ses_ucode_version(img->file);
	img->has_header = 0;
	get_ucode_date(img->file, img->date, sizeof(img->date));
}

/**
 * scan_fw_dir - 
 * @path:		
 * @name:		
 * @list:		
 * @len:		
 * @init		function pointer to initialization function
 *
 * Returns:
 *   int len
 **/
static int scan_fw_dir(char *path, char *name, struct ipr_fw_images **list, int len,
		       void (*init)(struct ipr_fw_images *))
{
	struct dirent **dirent;
	int rc, i;
	struct ipr_fw_images *ret = *list;

	rc = scandir(path, &dirent, NULL, alphasort);

	for (i = 0; i < rc && rc > 0; i++) {
		if (strstr(dirent[i]->d_name, name) == dirent[i]->d_name) {
			ret = realloc(ret, sizeof(*ret) * (len + 1));
			sprintf(ret[len].file, "%s/%s", path, dirent[i]->d_name);
			init(&ret[len]);
			len++;
		}
	}

	for (i = 0; i < rc; i++)
		free(dirent[i]);
	if (rc > 0)
		free(dirent);

	*list = ret;
	return len;
}

/**
 * get_ioa_firmware_image_list -
 * @ioa:		ipr ioa struct
 * @list:		ipr_fw_images struct
 *
 * Returns:
 *   length of list
 **/
int get_ioa_firmware_image_list(struct ipr_ioa *ioa,
				struct ipr_fw_images **list)
{
	char buf[100];
	struct ipr_fw_images *ret = NULL;
	int len = 0;

	*list = NULL;

	get_ioa_fw_name(ioa, buf);

	len = scan_fw_dir(UCODE_BASE_DIR, buf, &ret, len, init_ioa_ucode_entry);

	get_linux_ioa_fw_name(ioa, buf);

	len = scan_fw_dir(LINUX_UCODE_BASE_DIR, buf, &ret, len, init_ioa_ucode_entry);

	get_linux_ioa_fw_name_capital(ioa, buf);

	len = scan_fw_dir(LINUX_UCODE_BASE_DIR, buf, &ret, len, init_ioa_ucode_entry);

	sprintf(buf, "ibmsis%X.img", ioa->ccin);

	len = scan_fw_dir("/etc/microcode", buf, &ret, len, init_ioa_ucode_entry);

	sprintf(buf, "ibmsis%X.img", get_ioa_image_type(ioa));

	len = scan_fw_dir("/etc/microcode", buf, &ret, len, init_ioa_ucode_entry);

	if (ret)
		qsort(ret, len, sizeof(*ret), fw_compare);

	*list = ret;
	return len;
}

/**
 * get_dasd_firmware_image_list- 
 * @dev:		ipr dev struct
 * @list:		irp_fw_images struct
 *
 * Returns:
 *   length value if success / non-zero on failure
 **/
int get_dasd_firmware_image_list(struct ipr_dev *dev,
				 struct ipr_fw_images **list)
{
	char buf[100];
	struct ipr_fw_images *ret = NULL;
	struct ipr_dasd_inquiry_page3 page3_inq;
	int rc;
	int len = 0;

	*list = NULL;

	memset(&page3_inq, 0, sizeof(page3_inq));
	rc = ipr_inquiry(dev, 3, &page3_inq, sizeof(page3_inq));

	if (rc != 0) {
		scsi_dbg(dev, "Inquiry failed\n");
		return -EIO;
	}

	sprintf(buf, "%.7s.%02X%02X%02X%02X",
		dev->scsi_dev_data->product_id,
		page3_inq.load_id[0], page3_inq.load_id[1],
		page3_inq.load_id[2], page3_inq.load_id[3]);

	len = scan_fw_dir(UCODE_BASE_DIR, buf, &ret, len,
			  init_disk_ucode_entry_nohdr);

	if (memcmp(dev->scsi_dev_data->vendor_id, "IBMAS400", 8) == 0)
		sprintf(buf, "ibmsis%02X%02X%02X%02X.img",
			page3_inq.load_id[0], page3_inq.load_id[1],
			page3_inq.load_id[2], page3_inq.load_id[3]);

	len = scan_fw_dir("/etc/microcode/device", buf, &ret, len,
			  init_disk_ucode_entry);

	sprintf(buf, "IBM-%.7s.%02X%02X%02X%02X",
		dev->scsi_dev_data->product_id,
		page3_inq.load_id[0], page3_inq.load_id[1],
		page3_inq.load_id[2], page3_inq.load_id[3]);

	len = scan_fw_dir(LINUX_UCODE_BASE_DIR, buf, &ret, len,
			  init_disk_ucode_entry_nohdr);

	if (len)
		qsort(ret, len, sizeof(*ret), fw_compare);
	else
		scsi_dbg(dev, "Could not find device firmware file\n");

	*list = ret;
	return len;
}

/**
 * get_ses_load_id- 
 * @dev:		ipr dev struct
 * @load_id:		
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int get_ses_load_id(struct ipr_dev *dev, u8 load_id[4])
{
	struct ipr_dasd_inquiry_page3 page3_inq;
	int rc;

	memset(&page3_inq, 0, sizeof(page3_inq));
	rc = ipr_inquiry(dev, 3, &page3_inq, sizeof(page3_inq));

	if (rc)
		return rc;

	if (ipr_is_ses(dev) && !__ioa_is_spi(dev->ioa))
		memcpy(load_id, page3_inq.release_level, 4);
	else
		memcpy(load_id, page3_inq.load_id, 4);

	return 0;
}

/**
 * get_ses_firmware_image_list- 
 * @dev:		ipr dev struct
 * @list:		
 *
 * Returns:
 *   length value if success / non-zero on failure
 **/
int get_ses_firmware_image_list(struct ipr_dev *dev,
				struct ipr_fw_images **list)
{
	char buf[100];
	struct ipr_fw_images *ret = NULL;
	u8 load_id[4];
	int rc;
	int len = 0;

	*list = NULL;

	rc = get_ses_load_id(dev, load_id);

	if (rc != 0) {
		scsi_dbg(dev, "Inquiry failed\n");
		return -EIO;
	}

	sprintf(buf, "%02X%02X%02X%02X", load_id[0], load_id[1],
		load_id[2], load_id[3]);

	len = scan_fw_dir(UCODE_BASE_DIR, buf, &ret, len,
			  init_ses_ucode_entry_nohdr);

	sprintf(buf, "IBM-%02X%02X%02X%02X", load_id[0], load_id[1],
		load_id[2], load_id[3]);

	len = scan_fw_dir(LINUX_UCODE_BASE_DIR, buf, &ret, len,
			  init_ses_ucode_entry_nohdr);

	if (len)
		qsort(ret, len, sizeof(*ret), fw_compare);
	else
		scsi_dbg(dev, "Could not find device firmware file\n");

	*list = ret;
	return len;
}

/**
 * get_fw_image - Common interface to find version of the latest
 * microcode image found in the filesystem.
 *
 * @dev:		Device
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
struct ipr_fw_images *get_latest_fw_image(struct ipr_dev *dev)
{
	struct ipr_fw_images *fw = NULL;

	if (!dev)
		return NULL;

	if (ipr_is_ioa(dev))
		get_ioa_firmware_image_list(dev->ioa, &fw);
	else if (ipr_is_ses(dev))
		get_ses_firmware_image_list(dev, &fw);
	else if (ipr_is_gscsi(dev) || ipr_is_af_dasd_device(dev))
		get_dasd_firmware_image_list(dev, &fw);

	if (!fw)
		return NULL;

	return fw;
}

/**
 * get_firmware_image_list - Common interface to find version of the
 * latest microcode image found in the filesystem.
 *
 * @dev:		Device
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int get_latest_fw_image_version(struct ipr_dev *dev)
{
	struct ipr_fw_images *fw = NULL;
	u32 version = 0;

	if (!dev)
		return -ENODEV;

	if (ipr_is_ioa(dev))
		get_ioa_firmware_image_list(dev->ioa, &fw);
	else if (ipr_is_ses(dev))
		get_ses_firmware_image_list(dev, &fw);
	else if (ipr_is_gscsi(dev) || ipr_is_af_dasd_device(dev))
		get_dasd_firmware_image_list(dev, &fw);

	if (!fw)
		return -EINVAL;

	version = fw->version;
	free(fw);

	return version;
}

struct ipr_ioa_desc {
	u16 type;
	const char *desc;
};

struct ipr_ioa_desc ioa_desc [] = {
	{0x5702, "PCI-X Dual Channel Ultra320 SCSI Adapter [5702]"},
	{0x1974, "PCI-X Dual Channel Ultra320 SCSI Adapter [1974]"},
	{0x5703, "PCI-X Dual Channel Ultra320 SCSI RAID Adapter [5703]"},
	{0x1975, "PCI-X Dual Channel Ultra320 SCSI RAID Adapter [1975]"},
	{0x2780, "PCI-X Quad Channel Ultra320 SCSI RAID Adapter [2780]"},
	{0x5709, "SCSI RAID Enablement Card for PCI-X Dual Channel Ultra320 SCSI Integrated Controller [5709]"},
	{0x1976, "SCSI RAID Enablement Card for PCI-X Dual Channel Ultra320 SCSI Integrated Controller [1976]"},
	{0x570A, "PCI-X Dual Channel SCSI Integrated Controller (Adapter bus) [570A]"},
	{0x570B, "PCI-X Dual Channel SCSI Integrated Controller (Adapter bus) [570B]"}
};

/**
 * get_long_ioa_desc- 
 * @type:		
 *
 * Returns:
 *   IOA description if success / NULL on failure
 **/
static const char *get_long_ioa_desc(u16 type)
{
	int i;

	for (i = 0; i < sizeof(ioa_desc)/sizeof(ioa_desc[0]); i++) {
		if (type == ioa_desc[i].type)
			return ioa_desc[i].desc;
	}

	return NULL;
}

/**
 * ipr_log_ucode_error - log a microcode error
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   nothing
 **/
void ipr_log_ucode_error(struct ipr_ioa *ioa)
{
	const char *desc = get_long_ioa_desc(ioa->ccin);

	if (desc) {
		syslog(LOG_ERR, "Could not find required level of microcode for IBM '%s'. "
		       "Please download the latest microcode from "
		       "http://techsupport.services.ibm.com/server/mdownload/download.html. "
		       "SCSI speeds will be limited to %d MB/s until updated microcode is downloaded.\n",
		       desc, IPR_SAFE_XFER_RATE);
	} else {
		syslog(LOG_ERR, "Could not find required level of microcode for IBM %04X. "
		       "Please download the latest microcode from "
		       "http://techsupport.services.ibm.com/server/mdownload/download.html. "
		       "SCSI speeds will be limited to %d MB/s until updated microcode is downloaded.\n",
		       ioa->ccin, IPR_SAFE_XFER_RATE);
	}
}

/**
 * ipr_update_ioa_fw -
 * @ioa:		ipr ioa struct
 * @image:		pointer to fw image
 * @force:		force flag
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_update_ioa_fw(struct ipr_ioa *ioa,
		      struct ipr_fw_images *image, int force)
{
	struct ipr_ioa_ucode_header *image_hdr;
	struct ipr_ioa_ucode_ext_header *ext_hdr;
	struct ipr_ioa_ucode_img_desc *img_desc;
	struct stat ucode_stats;
	u32 fw_version;
	int fd, rc;
	int ioafw = 1;
	int host_num = ioa->host_num;
	char *tmp;
	char ucode_file[200];
	DIR *dir;
	char *img_file;
	char cwd[200];
	char devpath[PATH_MAX];
	ssize_t len;

	fw_version = get_ioa_fw_version(ioa);

	if (fw_version >= ioa->msl && !force)
		return 0;

	if (ipr_get_hotplug_dir())
		return 0;

	fd = open(image->file, O_RDONLY);

	if (fd < 0) {
		syslog(LOG_ERR, "Could not open firmware file %s.\n", image->file);
		return fd;
	}

	rc = fstat(fd, &ucode_stats);

	if (rc != 0) {
		syslog(LOG_ERR, "Failed to stat IOA firmware file: %s.\n", image->file);
		close(fd);
		return rc;
	}

	image_hdr = mmap(NULL, ucode_stats.st_size, PROT_READ, MAP_SHARED, fd, 0);

	if (image_hdr == MAP_FAILED) {
		syslog(LOG_ERR, "Error mapping IOA firmware file: %s.\n", image->file);
		close(fd);
		return -EIO;
	}

	ext_hdr = (void *)image_hdr + ntohl(image_hdr->header_length);
	img_desc = (void *)ext_hdr + ntohl(ext_hdr->img_desc_offset);
	if (strncmp(img_desc->fw_type, IPR_IOAF_STR, 4))
		ioafw = 0;

	if (ntohl(image_hdr->rev_level) > fw_version || force) {
		if (ioafw)
			ioa_info(ioa, "Updating microcode from %08X to %08X.\n",
				 fw_version, ntohl(image_hdr->rev_level));
		else
			ioa_info(ioa, "Updating microcode to %08X.\n",
				 ntohl(image_hdr->rev_level));

		/* Give the file name an absolute path if needed. */
		if (image->file[0] != '/') {
			getcwd(cwd, sizeof(cwd));
			strcat(cwd, "/");
			img_file = strcat(cwd, image->file);
		} else
			img_file = image->file;

		tmp = strrchr(img_file, '/');
		if (tmp)
			tmp++;
		else {
			syslog(LOG_ERR, "Failed to find image name in %s\n",
			img_file);
			return -EIO;
		}

		dir = opendir(hotplug_dir);
		if (!dir)
			mkdir(hotplug_dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

		dir = opendir(hotplug_dir);
		if (!dir) {
			syslog(LOG_ERR, "Failed to open %s. %m\n", hotplug_dir);
			munmap(image_hdr, ucode_stats.st_size);
			close(fd);
			return -EIO;
		}
		closedir(dir);
		sprintf(ucode_file, "%s/.%s", hotplug_dir, tmp);
		symlink(img_file, ucode_file);
		sprintf(ucode_file, ".%s\n", tmp);
		sprintf(devpath, "/sys/class/scsi_host/%s", ioa->host_name);
		len = sysfs_write_attr(devpath, "update_fw",
				       ucode_file, strlen(ucode_file));
		sprintf(ucode_file, "%s/.%s", hotplug_dir, tmp);
		unlink(ucode_file);

		if (len < 0)
			ioa_err(ioa, "Microcode update failed. rc=%d\n",
				(int)len);
		check_current_config(false);
		for_each_ioa(ioa) {
			if (ioa->host_num != host_num)
				continue;
			ipr_init_ioa(ioa);
			break;
		}
	} else
		ipr_log_ucode_error(ioa);

	munmap(image_hdr, ucode_stats.st_size);
	close(fd);
	return rc;
}

/**
 * ipr_update_disk_fw - update disk fw
 * @dev:		ipr dev struct
 * @image:		pointer to fw image
 * @force:		force flag
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
/* xxx TODO make a general routine to do write buffer that takes a
 struct ipr_fw_images as input */
int ipr_update_disk_fw(struct ipr_dev *dev,
		       struct ipr_fw_images *image, int force)
{
	int rc = 0;
	struct stat ucode_stats;
	int fd;
	struct ipr_dasd_ucode_header *img_hdr;
	struct ipr_dasd_inquiry_page3 page3_inq;
	struct ipr_std_inq_data std_inq_data;
	struct unsupported_af_dasd *unsupp_af;
	u32 level;
	u8 release_level[4];

	memset(&std_inq_data, 0, sizeof(std_inq_data));
	rc = ipr_inquiry(dev, IPR_STD_INQUIRY,
			 &std_inq_data, sizeof(std_inq_data));

	if (rc != 0)
		return rc;

	rc = ipr_inquiry(dev, 3, &page3_inq, sizeof(page3_inq));

	if (rc != 0) {
		scsi_dbg(dev, "Inquiry failed\n");
		return rc;
	}

	if (!force) {
		unsupp_af = get_unsupp_af(&std_inq_data, &page3_inq);
		if (!unsupp_af)
			return 0;

		if (!disk_needs_msl(unsupp_af, &std_inq_data))
			return 0;
	}

	fd = open(image->file, O_RDONLY);

	if (fd < 0) {
		syslog_dbg("Could not open firmware file %s.\n", image->file);
		return fd;
	}

	rc = fstat(fd, &ucode_stats);

	if (rc != 0) {
		syslog(LOG_ERR, "Failed to stat firmware file: %s.\n", image->file);
		close(fd);
		return rc;
	}

	img_hdr = mmap(NULL, ucode_stats.st_size,
		       PROT_READ, MAP_SHARED, fd, 0);

	if (img_hdr == MAP_FAILED) {
		syslog(LOG_ERR, "Error reading firmware file: %s.\n", image->file);
		close(fd);
		return -EIO;
	}

	level = htonl(image->version);
	__ipr_get_fw_version(dev, &page3_inq, release_level);

	if (memcmp(&level, page3_inq.release_level, 4) > 0 || force) {
		scsi_info(dev, "Updating device microcode using %s "
			  "from %02X%02X%02X%02X (%c%c%c%c) to %08X (%c%c%c%c)\n", image->file,
			  release_level[0], release_level[1], release_level[2], release_level[3],
			  release_level[0], release_level[1], release_level[2], release_level[3],
			  image->version, image->version >> 24, (image->version >> 16) & 0xff,
			  (image->version >> 8) & 0xff, image->version & 0xff);

		rc = ipr_write_buffer(dev, img_hdr, ucode_stats.st_size);
		ipr_init_dev(dev);
	}

	if (munmap(img_hdr, ucode_stats.st_size))
		syslog(LOG_ERR, "munmap failed.\n");

	close(fd);
	return rc;
}

/**
 * mode_select - issue a mode select command
 * @dev:		ipr dev struct
 * @buff:		data buffer
 * @length:		
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int mode_select(struct ipr_dev *dev, void *buff, int length)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;
	char *name = dev->gen_name;

	fd = open(name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = MODE_SELECT;
	cdb[1] = 0x11;
	cdb[4] = length;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_TO_DEV,
		      &sense_data, IPR_INTERNAL_TIMEOUT);

	if (rc)
		scsi_cmd_err(dev, &sense_data, "Mode Select", rc);

	close(fd);
	return rc;
}


int ipr_ses_get_time(struct ipr_dev *dev, u64* timestamp, int *origin)
{
	struct ipr_ses_diag_page12 get_time;
	int err;

	err = ipr_receive_diagnostics(dev, 0x12, &get_time, sizeof(get_time));
	if (err)
		return -EIO;

	*origin = !!get_time.timestamp_origin;
	*timestamp = be64toh(*((u64*) get_time.timestamp)) >> 16;
	return 0;
}

int ipr_ses_set_time(struct ipr_dev *dev, u64 timestamp)
{
	struct ipr_ses_diag_ctrl_page13 set_time;

	memset(&set_time, '\0', sizeof(set_time));

	set_time.page_code = 0x13;
	set_time.page_length[1] = 8;

	timestamp = htobe64(timestamp << 16);
	memcpy(set_time.timestamp, (char*) &
	       timestamp, 6);

	return ipr_send_diagnostics(dev, &set_time, sizeof(set_time));
}

/**
 * setup_page0x00 - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int setup_page0x00(struct ipr_dev *dev)
{
	struct ipr_mode_pages mode_pages, ch_mode_pages;
	struct ipr_vendor_mode_page *page, *ch_page;
	int len;

	if (strncmp(dev->scsi_dev_data->vendor_id, "IBM", 3)) {
		scsi_dbg(dev, "Not setting up mode page 0x00 for unknown device.\n");
		return 0;
	}

	memset(&mode_pages, 0, sizeof(mode_pages));
	memset(&ch_mode_pages, 0, sizeof(ch_mode_pages));

	if (ipr_mode_sense(dev, 0x00, &mode_pages))
		return -EIO;
	if (ipr_mode_sense(dev, 0x40, &ch_mode_pages))
		return -EIO;

	page = (struct ipr_vendor_mode_page *)(((u8 *)&mode_pages) +
					       mode_pages.hdr.block_desc_len +
					       sizeof(mode_pages.hdr));
	ch_page = (struct ipr_vendor_mode_page *)(((u8 *)&ch_mode_pages) +
						  ch_mode_pages.hdr.block_desc_len +
						  sizeof(ch_mode_pages.hdr));

	IPR_SET_MODE(ch_page->arhes, page->arhes, 1);
	IPR_SET_MODE(ch_page->cmdac, page->cmdac, 1);
	IPR_SET_MODE(ch_page->caen, page->caen, 1);

	/* Use a 3 second command aging timer - units are 50 ms */
	IPR_SET_MODE(ch_page->cmd_aging_limit_hi, page->cmd_aging_limit_hi, 0);
	IPR_SET_MODE(ch_page->cmd_aging_limit_lo, page->cmd_aging_limit_lo, 60);

	len = mode_pages.hdr.length + 1;
	mode_pages.hdr.length = 0;
	mode_pages.hdr.medium_type = 0;
	mode_pages.hdr.device_spec_parms = 0;
	page->hdr.parms_saveable = 0;

	if (mode_select(dev, &mode_pages, len)) {
		scsi_dbg(dev, "Failed to setup mode page 0x00\n");
		return -EIO;
	}
	return 0;
}

/**
 * setup_page0x01 - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int setup_page0x01(struct ipr_dev *dev)
{
	struct ipr_mode_pages mode_pages, ch_mode_pages;
	struct ipr_rw_err_mode_page *page, *ch_page;
	int len;

	memset(&mode_pages, 0, sizeof(mode_pages));
	memset(&ch_mode_pages, 0, sizeof(ch_mode_pages));

	if (ipr_mode_sense(dev, 0x01, &mode_pages))
		return -EIO;
	if (ipr_mode_sense(dev, 0x41, &ch_mode_pages))
		return -EIO;

	page = (struct ipr_rw_err_mode_page *)(((u8 *)&mode_pages) +
					       mode_pages.hdr.block_desc_len +
					       sizeof(mode_pages.hdr));
	ch_page = (struct ipr_rw_err_mode_page *)(((u8 *)&ch_mode_pages) +
						  ch_mode_pages.hdr.block_desc_len +
						  sizeof(ch_mode_pages.hdr));

	IPR_SET_MODE(ch_page->awre, page->awre, 1);
	IPR_SET_MODE(ch_page->arre, page->arre, 1);

	if (page->awre != 1)
		goto error;
	if (page->arre != 1)
		goto error;

	len = mode_pages.hdr.length + 1;
	mode_pages.hdr.length = 0;
	mode_pages.hdr.medium_type = 0;
	mode_pages.hdr.device_spec_parms = 0;
	page->hdr.parms_saveable = 0;

	if (mode_select(dev, &mode_pages, len)) {
error:
		scsi_err(dev, "Failed to setup mode page 0x01\n");
		return -EIO;
	}
	return 0;
}

/**
 * setup_page0x08 - Perform initial configuration for mode page 0x8 of
 * vset devices.
 *
 * This disables the Write Cache for vsets.
 *
 * @dev:		ipr dev struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int setup_page0x08(struct ipr_dev *dev)
{
	struct ipr_mode_pages mode_pages;
	struct ipr_caching_parameters_page *page;
	int len;

	memset(&mode_pages, 0, sizeof(mode_pages));

	if (ipr_mode_sense(dev, 0x08, &mode_pages))
		return -EIO;

	page = ((struct ipr_caching_parameters_page *)
		(((u8 *)&mode_pages)
		 + mode_pages.hdr.block_desc_len
		 + sizeof(mode_pages.hdr)));

	if (page->wce == 0) {
		/* Write cache is already disabled */
		return 0;
	}

	page->wce = 0;

	len = mode_pages.hdr.length + 1;
	mode_pages.hdr.length = 0;
	mode_pages.hdr.medium_type = 0;
	mode_pages.hdr.device_spec_parms = 0;
	page->hdr.parms_saveable = 0;

	if (mode_select(dev, &mode_pages, len)) {
		scsi_err(dev, "Failed to disable write cache.\n");
		return -EIO;
	}

	if (ipr_mode_sense(dev, 0x08, &mode_pages))
		return -EIO;

	page = ((struct ipr_caching_parameters_page *)
		(((u8 *)&mode_pages)
		 + mode_pages.hdr.block_desc_len
		 + sizeof(mode_pages.hdr)));

	if (page->wce != 0) {
		scsi_err(dev, "Failed to disable write cache.\n");
		return -EIO;
	}
	return 0;
}

/**
 * setup_page0x0a - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
static int setup_page0x0a(struct ipr_dev *dev)
{
	struct ipr_mode_pages mode_pages, ch_mode_pages;
	struct ipr_control_mode_page *page, *ch_page;
	int len;
	int rc = 0;

	memset(&mode_pages, 0, sizeof(mode_pages));
	memset(&ch_mode_pages, 0, sizeof(ch_mode_pages));

	if (ipr_mode_sense(dev, 0x0A, &mode_pages))
		return -EIO;
	if (ipr_mode_sense(dev, 0x4A, &ch_mode_pages))
		return -EIO;

	page = (struct ipr_control_mode_page *)(((u8 *)&mode_pages) +
						mode_pages.hdr.block_desc_len +
						sizeof(mode_pages.hdr));
	ch_page = (struct ipr_control_mode_page *)(((u8 *)&ch_mode_pages) +
						   ch_mode_pages.hdr.block_desc_len +
						   sizeof(ch_mode_pages.hdr));

	IPR_SET_MODE(ch_page->queue_algorithm_modifier,
		     page->queue_algorithm_modifier, 1);
	IPR_SET_MODE(ch_page->tst, page->tst, 1);
	IPR_SET_MODE(ch_page->tas, page->tas, 1);

	switch(dev->ioa->tcq_mode) {
	case IPR_TCQ_FROZEN:
		IPR_SET_MODE(ch_page->qerr, page->qerr, 3);
		if (page->qerr != 3) {
			IPR_SET_MODE(ch_page->qerr, page->qerr, 1);
			if (page->tst != 1)
				IPR_SET_MODE(ch_page->tst, page->tst, 0);
		}

		scsi_dbg(dev, "Using control mode settings: "
			 "TST=%d, QERR=%d, TAS=%d\n", page->tst, page->qerr, page->tas);
		break;
	case IPR_TCQ_NACA:
		IPR_SET_MODE(ch_page->qerr, page->qerr, 0);
		break;
	case IPR_TCQ_DISABLE:
	default:
		rc = -EIO;
		break;
	};

	IPR_SET_MODE(ch_page->dque, page->dque, 0);

	if (page->dque != 0) {
		scsi_dbg(dev, "Cannot set dque=0\n");
		return -EIO;
	}

	if (page->queue_algorithm_modifier != 1)
		scsi_dbg(dev, "Cannot set QAM=1\n");

	len = mode_pages.hdr.length + 1;
	mode_pages.hdr.length = 0;
	mode_pages.hdr.medium_type = 0;
	mode_pages.hdr.device_spec_parms = 0;
	page->hdr.parms_saveable = 0;

	if (mode_select(dev, &mode_pages, len)) {
		scsi_err(dev, "Failed to setup mode page 0x0A\n");
		return -EIO;
	}
	return rc;
}

void ipr_count_devices_in_vset(struct ipr_dev *dev, int *num_devs,
			       int *ssd_num_devs)
{
	struct ipr_dev *vset, *temp;
	int devs_cnt = 0, ssd_devs_cnt = 0;

	if (ipr_is_volume_set(dev)) {
		for_each_dev_in_vset(dev, temp) {
			devs_cnt++;
			if (temp->block_dev_class & IPR_SSD)
				ssd_devs_cnt++;
		}
	} else {
		devs_cnt++;
		if (dev->block_dev_class & IPR_SSD)
			ssd_devs_cnt++;
	}

	*num_devs = devs_cnt;
	*ssd_num_devs = ssd_devs_cnt;

}

int ipr_max_queue_depth(struct ipr_ioa *ioa, int num_devs, int num_ssd_devs)
{
	int max_qdepth;

	if (num_ssd_devs == num_devs)
		max_qdepth = MIN(num_devs * 64, ioa->can_queue);
	else if (num_ssd_devs)
		max_qdepth = MIN(num_devs * 32, ioa->can_queue);
	else
		max_qdepth = MIN(num_devs * 16, ioa->can_queue);

	return MAX(max_qdepth, 128);
}

/**
 * init_vset_dev - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   nothing
 **/
/*
 * VSETs:
 * 1. Adjust queue depth based on number of devices
 *
 */
static void init_vset_dev(struct ipr_dev *dev)
{
	struct ipr_query_res_state res_state;
	char q_depth[100];
	char cur_depth[100], saved_depth[100];
	int depth, rc, num_devs, ssd_num_devs;
	char saved_cache[100];

	memset(&res_state, 0, sizeof(res_state));

	if (polling_mode && !dev->should_init)
		return;

	if (dev->ioa->has_vset_write_cache) {
		int pol;
		rc = ipr_get_saved_dev_attr(dev, IPR_WRITE_CACHE_POLICY,
					    saved_cache);

		pol = (rc || strtoul (saved_cache, NULL, 10)) ?
			IPR_DEV_CACHE_WRITE_BACK : IPR_DEV_CACHE_WRITE_THROUGH;

		ipr_set_dev_wcache_policy(dev, pol);
	}

	if (!ipr_query_resource_state(dev, &res_state)) {
		ipr_count_devices_in_vset(dev, &num_devs, &ssd_num_devs);
		depth = ipr_max_queue_depth(dev->ioa, num_devs, ssd_num_devs);

		snprintf(q_depth, sizeof(q_depth), "%d", depth);
		if (ipr_read_dev_attr(dev, "queue_depth", cur_depth, 100))
			return;
		rc = ipr_get_saved_dev_attr(dev, IPR_QUEUE_DEPTH, saved_depth);
		if (rc == RC_SUCCESS) {
			depth = strtoul(saved_depth, NULL, 10);
			if (depth && depth <= 255)
				strcpy(q_depth, saved_depth);
		}

		if (!strcmp(cur_depth, q_depth))
			return;
		if (ipr_write_dev_attr(dev, "queue_depth", q_depth))
			return;
	}
}

/**
 * init_gpdd_dev - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   nothing
 **/
/*
 * GPDD DASD:
 * 1. Setup Mode Page 0x0A for TCQing.
 * 2. Enable TCQing
 * 3. Adjust queue depth
 *
 */
static void init_gpdd_dev(struct ipr_dev *dev)
{
	struct ipr_disk_attr attr;
	struct sense_data_t sense_data;
	int rc;

	if (polling_mode && !dev->should_init)
		return;
	if (__ipr_test_unit_ready(dev, &sense_data)) {
		if ((sense_data.sense_key != UNIT_ATTENTION) ||
		    __ipr_test_unit_ready(dev, &sense_data))
			return;
	}
	if (enable_af(dev))
		return;
	if (ipr_get_dev_attr(dev, &attr))
		return;
	if (setup_page0x00(dev))
		return;
	if ((rc = setup_page0x0a(dev))) {
		if (rc != -EINVAL) {
			scsi_dbg(dev, "Failed to enable TCQing.\n");
			return;
		}
	} else {
		attr.queue_depth = get_tcq_depth(dev);
		attr.tcq_enabled = 1;
	}

	if (ipr_modify_dev_attr(dev, &attr))
		return;
	if (ipr_set_dev_attr(dev, &attr, 0))
		return;
}

/**
 * init_af_dev - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   nothing
 **/
/*
 * AF DASD:
 * 1. Setup mode pages (pages 0x01, 0x0A, 0x20)
 * 2. Send set supported devices
 * 3. Set DASD timeouts
 */
static void init_af_dev(struct ipr_dev *dev)
{
	struct ipr_disk_attr attr;
	int rc;

	if (ipr_set_dasd_timeouts(dev, 0))
		return;
	if (polling_mode && (!dev->should_init && !memcmp(&attr, &dev->attr, sizeof(attr))))
	    return;
	if (polling_mode && !dev_init_allowed(dev))
		return;
	if (setup_page0x00(dev))
		return;
	if (setup_page0x01(dev))
		return;
	if (setup_page0x08(dev))
		return;
	if (enable_af(dev))
		return;
	if (ipr_get_dev_attr(dev, &attr))
		return;

	if ((rc = setup_page0x0a(dev))) {
		if (rc != -EINVAL) {
			scsi_dbg(dev, "Failed to enable TCQing.\n");
			return;
		}
	} else
		attr.queue_depth = get_tcq_depth(dev);

	if (ipr_modify_dev_attr(dev, &attr))
		return;
	memcpy(&dev->attr, &attr, sizeof(attr));
	if (ipr_set_dev_attr(dev, &attr, 0))
		return;
}

/**
 * init_ioa_dev - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   nothing
 **/
/*
 * IOA:
 * 1. Load saved adapter configuration
 */
static void init_ioa_dev(struct ipr_dev *dev)
{
	struct ipr_scsi_buses buses;
	struct ipr_ioa_attr attr;

	if (!dev->ioa)
		return;
	if (polling_mode && !dev->ioa->should_init)
		return;
	if (ipr_get_ioa_attr(dev->ioa, &attr))
		return;
	if (dev->ioa->asymmetric_access && dev->ioa->sis64)
		attr.active_active = 1;
	if (dev->ioa->configure_rebuild_verify)
		attr.disable_rebuild_verify = 1;
	if (dev->ioa->has_vset_write_cache)
		attr.vset_write_cache = 1;
	ipr_modify_ioa_attr(dev->ioa, &attr);
	if (ipr_set_ioa_attr(dev->ioa, &attr, 0))
		return;
	if (ipr_get_bus_attr(dev->ioa, &buses))
		return;
	ipr_modify_bus_attr(dev->ioa, &buses);
	if (ipr_set_bus_attr(dev->ioa, &buses, 0))
		return;
}

static void init_ses_dev(struct ipr_dev *dev)
{
	time_t t = time(NULL);
	if (t != ((time_t) -1)) {
		t *= 1000;
		ipr_ses_set_time(dev, t);
	}
}

/**
 * ipr_init_dev - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   0
 **/
int ipr_init_dev(struct ipr_dev *dev)
{
	if (!dev->scsi_dev_data)
		return 0;

	switch (dev->scsi_dev_data->type) {
	case TYPE_DISK:
		if (ipr_is_volume_set(dev))
			init_vset_dev(dev);
		else
			init_gpdd_dev(dev);
		break;
	case IPR_TYPE_AF_DISK:
		init_af_dev(dev);
		break;
	case IPR_TYPE_ADAPTER:
		if (ipr_is_ioa(dev))
			init_ioa_dev(dev);
		break;
	case TYPE_ENCLOSURE:
		init_ses_dev(dev);
		break;
	default:
		break;
	};

	return 0;
}

/**
 * ipr_init_new_dev - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   nothing
 **/
int ipr_init_new_dev(struct ipr_dev *dev)
{
	if (!dev->scsi_dev_data)
		return 0;

	switch (dev->scsi_dev_data->type) {
	case TYPE_DISK:
		if (!strlen(dev->dev_name))
			return 1;
		wait_for_dev(dev->dev_name);
		break;
	case IPR_TYPE_ADAPTER:
		if (!ipr_is_ioa(dev))
			break;
	case IPR_TYPE_AF_DISK:
		wait_for_dev(dev->gen_name);
		break;
	default:
		break;
	};

	ipr_init_dev(dev);
	return 0;
}

/**
 * ipr_scan_ra - 
 * @ioa:		ipr ioa struct
 * @ra:			ipr_res_addr struct
 *
 * Returns:
 *   nothing
 **/
static void ipr_scan_ra(struct ipr_ioa *ioa, struct ipr_res_addr *ra)
{
	ra_dbg(ra, "Scanning for new device\n");
	ipr_scan(ioa, ra->bus, ra->target, ra->lun);
}

/**
 * ipr_for_each_unique_ra - 
 * @dev:		ipr dev struct
 * @func:		function pointer
 *
 * Returns:
 *   nothing
 **/
static void ipr_for_each_unique_ra(struct ipr_dev *dev,
				   void (*func) (struct ipr_ioa *, struct ipr_res_addr *))
{
	signed int i, j;
	int dup;

	for (i = 0; i < ARRAY_SIZE(dev->res_addr); i++) {
		dup = 0;
		for (j = i - 1; j >= 0; j--) {
			if (!memcmp(&dev->res_addr[i], &dev->res_addr[j],
				    sizeof(struct ipr_res_addr))) {
				dup = 1;
				break;
			}
		}

		if (!dup)
			func(dev->ioa, &dev->res_addr[i]);
	}
}

/**
 * fixup_improper_devs - 
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   0 if no improper devs / number of improper devs otherwise
 **/
static int fixup_improper_devs(struct ipr_ioa *ioa)
{
	struct ipr_dev *dev;
	int improper = 0;

	for_each_dev(ioa, dev) {
		dev->local_flag = 0;
		if (ipr_improper_device_type(dev) && !ipr_device_lock(dev)) {
			improper++;
			dev->local_flag = 1;
			scsi_dbg(dev, "Deleting improper device\n");
			ipr_write_dev_attr(dev, "delete", "1");
			ipr_device_unlock(dev);
		}
	}

	if (!improper)
		return 0;

	sleep(5);
	for_each_dev(ioa, dev) {
		if (!dev->local_flag)
			continue;
		dev->local_flag = 0;
		dev->rescan = 0;
		ipr_for_each_unique_ra(dev, ipr_scan_ra);
	}

	return improper;
}

/**
 * ipr_init_ioa - 
 * @ioa:		ipr ioa struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_init_ioa(struct ipr_ioa *ioa)
{
	struct ipr_dev *dev;

	if (ioa->ioa_dead)
		return 0;
	if (fixup_improper_devs(ioa))
		return -EAGAIN;

	init_ioa_dev(&ioa->ioa);

	for_each_dev(ioa, dev)
		ipr_init_dev(dev);

	return 0;
}

/**
 * scsi_dev_kevent - 
 * @buf:		data buffer
 * @find_device:	function pointer
 * @func:		function pointer
 *
 * Returns:
 *   nothing
 **/
void scsi_dev_kevent(char *buf, struct ipr_dev *(*find_device)(char *),
		     int (*func)(struct ipr_dev *))
{
	struct ipr_dev *dev;
	char *name;
	int i, j, rc = -EAGAIN;

	name = strrchr(buf, '/');
	if (!name) {
		syslog_dbg("Failed to handle %s kevent\n", buf);
		return;
	}

	name++;
	for (i = 0; i < 2 && rc == -EAGAIN; i++) {
		for (j = 0; j < 10; j++) {
			tool_init(1);
			check_current_config(false);
			dev = find_device(name);
			if (dev)
				break;
			sleep(2);
		}

		if (!dev) {
			syslog_dbg("Failed to find ipr dev %s\n", name);
			return;
		}

		rc = func(dev);
	}
}

/**
 * scsi_host_kevent -
 * @buf:		data buffer
 * @func:		funtion pointer
 *
 * Returns:
 *   nothing
 **/
void scsi_host_kevent(char *buf, int (*func)(struct ipr_ioa *))
{
	struct ipr_ioa *ioa;
	char *c;
	int host;
	int i, j, rc = -EAGAIN;

	c = strrchr(buf, '/');
	if (!c) {
		syslog_dbg("Failed to handle %s kevent\n", buf);
		return;
	}

	c += strlen("/host");

	host = strtoul(c, NULL, 10);
	for (i = 0; i < 2 && rc == -EAGAIN; i++) {
		for (j = 0; j < 10; j++) {
			tool_init(1);
			check_current_config(false);
			ioa = find_ioa(host);
			if (ioa)
				break;
			sleep(2);
		}

		if (!ioa) {
			syslog_dbg("Failed to find ipr ioa %d\n", host);
			return;
		}

		rc = func(ioa);
	}
}

/**
 * get_dev_from_addr - 
 * @res_addr:		ipr_res_addr struct
 *
 * Returns:
 *   ipr_dev struct if success / NULL otherwise
 **/
struct ipr_dev *get_dev_from_addr(struct ipr_res_addr *res_addr)
{
	struct ipr_ioa *ioa;
	int j;
	struct scsi_dev_data *scsi_dev_data;

	for_each_ioa(ioa) {
		for (j = 0; j < ioa->num_devices; j++) {
			scsi_dev_data = ioa->dev[j].scsi_dev_data;

			if (!scsi_dev_data)
				continue;

			if (scsi_dev_data->host == res_addr->host &&
			    scsi_dev_data->channel == res_addr->bus &&
			    scsi_dev_data->id == res_addr->target &&
			    scsi_dev_data->lun == res_addr->lun)
				return &ioa->dev[j];
		}
	}

	return NULL;
}

/**
 * get_dev_from_handle - 
 * @res_handle:		
 * @ioa:		struct ipr_ioa
 *
 * Returns:
 *   ipr_dev struct if success / NULL otherwise
 **/
struct ipr_dev *get_dev_from_handle(struct ipr_ioa *ioa, u32 res_handle)
{
	int j;

	for (j = 0; j < ioa->num_devices; j++) {
		if (!ioa->dev[j].qac_entry)
			continue;

		if (ipr_is_device_record(ioa->dev[j].qac_entry->record_id) &&
		     ioa->dev[j].resource_handle == res_handle)
			return &ioa->dev[j];

		if (ipr_is_array_record(ioa->dev[j].qac_entry->record_id) &&
		    ioa->dev[j].resource_handle == res_handle)
			return &ioa->dev[j];

		if (ipr_is_vset_record(ioa->dev[j].qac_entry->record_id) &&
		    ioa->dev[j].resource_handle == res_handle)
			return &ioa->dev[j];
	}

	return NULL;
}

/**
 * ipr_daemonize - 
 *
 * Returns:
 *   nothing
 **/
void ipr_daemonize()
{
	int rc = fork();

	if (rc < 0) {
		syslog(LOG_ERR, "Failed to daemonize\n");
		exit(1);
	} else if (rc) {
		exit(0);
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	open(_PATH_DEVNULL,O_RDONLY);
	open(_PATH_DEVNULL,O_WRONLY);
	open(_PATH_DEVNULL,O_WRONLY);
	setsid();
}

/**
 * ipr_disable_qerr - 
 * @dev:		ipr dev struct
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_disable_qerr(struct ipr_dev *dev)
{
	u8 ioctl_buffer[IOCTL_BUFFER_SIZE];
	u8 ioctl_buffer2[IOCTL_BUFFER_SIZE];
	struct ipr_control_mode_page *control_mode_page;
	struct ipr_control_mode_page *control_mode_page_changeable;
	struct ipr_mode_parm_hdr *mode_parm_hdr;
	int status;
	u8 length;

	/* Issue mode sense to get the control mode page */
	status = ipr_mode_sense(dev, 0x0a, &ioctl_buffer);

	if (status)
		return -EIO;

	/* Issue mode sense to get the control mode page */
	status = ipr_mode_sense(dev, 0x4a, &ioctl_buffer2);

	if (status)
		return -EIO;

	mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer2;

	control_mode_page_changeable = (struct ipr_control_mode_page *)
		(((u8 *)(mode_parm_hdr+1)) + mode_parm_hdr->block_desc_len);

	mode_parm_hdr = (struct ipr_mode_parm_hdr *)ioctl_buffer;

	control_mode_page = (struct ipr_control_mode_page *)
		(((u8 *)(mode_parm_hdr+1)) + mode_parm_hdr->block_desc_len);

	/* Turn off QERR since some drives do not like QERR
	 and IMMED bit at the same time. */
	IPR_SET_MODE(control_mode_page_changeable->qerr,
		     control_mode_page->qerr, 0);

	/* Issue mode select to set page x0A */
	length = mode_parm_hdr->length + 1;

	mode_parm_hdr->length = 0;
	control_mode_page->hdr.parms_saveable = 0;
	mode_parm_hdr->medium_type = 0;
	mode_parm_hdr->device_spec_parms = 0;

	status = ipr_mode_select(dev, &ioctl_buffer, length);

	if (status)
		return -EIO;

	return 0;
}

void ipr_set_manage_start_stop(struct ipr_dev *dev)
{
	char path[PATH_MAX];
	ssize_t len;
	char value_str[2];
	int value;

	sprintf(path, "/sys/class/scsi_disk/%s",
		dev->scsi_dev_data->sysfs_device_name);
	len = sysfs_read_attr(path, "manage_start_stop", value_str, 2);
	if (len < 0) {
		syslog_dbg("Failed to open manage_start_stop parameter.\n");
		return;
	}

	value = atoi(value_str);

	snprintf(value_str, 2, "%d", 1);

	if (sysfs_write_attr(path, "manage_start_stop", value_str, 1) < 0)
		syslog_dbg("Failed to write manage_start_stop parameter.\n");

	len = sysfs_read_attr(path, "manage_start_stop", value_str, 2);
	value = atoi(value_str);
}

/**
 * ipr_query_ioa_device_port - 
 * @ioa:		ipr ioa struct
 * @res_addr:	        ipr_res_addr struct
 * @option:		option value
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_query_io_dev_port(struct ipr_dev *dev, struct ipr_query_io_port *io_port)
{
	int fd, rc, cdb_num;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct ipr_ioa *ioa = dev->ioa;
	struct sense_data_t sense_data;
	char *rp, *endptr;

	if (strlen(ioa->ioa.gen_name) == 0)
		return -ENOENT;

	fd = open(ioa->ioa.gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", ioa->ioa.gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_IOA_SERVICE_ACTION;
	cdb[1] = IPR_QUERY_IOA_DEV_PORT;
	if (ioa->sis64) {
		/* convert res path string to bytes */
		cdb_num = 2;
		rp = dev->res_path_name;
		do {
			cdb[cdb_num++] = (u8)strtol(rp, &endptr, 16);
			rp = endptr+1;
		} while (*endptr != '\0' && cdb_num < 10);

		while (cdb_num < 10) 
			cdb[cdb_num++] = 0xff;

		cdb[10] = sizeof(*io_port) >> 24;
		cdb[11] = (sizeof(*io_port) >> 16) & 0xff;
		cdb[12] = (sizeof(*io_port) >> 8) & 0xff;
		cdb[13] = sizeof(*io_port) & 0xff;
	}
	else {
		close(fd);
		return -1;
	}

	rc = sg_ioctl(fd, cdb, io_port,
		      sizeof(*io_port), SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_TIMEOUT);

	if (rc != 0)
		scsi_cmd_err(dev, &sense_data, "Query IO port status", rc);

	close(fd);
	return rc;
}

/**
 * ipr_jbod_sysfs_bind -
 * @dev:		ipr dev struct
 * @op:			sysfs operation
 *
 * Returns:
 *   0 if success / non-zero on failure
 **/
int ipr_jbod_sysfs_bind(struct ipr_dev *dev, u8 op)
{
	struct ipr_dev *mp_dev;
	int rc, fd, size;
	char *sysfs_device_name;

	sysfs_device_name = dev->scsi_dev_data->sysfs_device_name;
	size = strnlen(sysfs_device_name, sizeof(sysfs_device_name));

	if (op == IPR_JBOD_SYSFS_BIND) {
		fd = open("/sys/bus/scsi/drivers/sd/bind", O_WRONLY);
	} else if (op == IPR_JBOD_SYSFS_UNBIND) {
		fd = open("/sys/bus/scsi/drivers/sd/unbind", O_WRONLY);
	} else {
		fd = -1;
		errno = ENOTSUP;
	}
	if (fd < 0)
		return errno;

	rc = write(fd, sysfs_device_name, size);
	if (rc < 0) {
		close(fd);
		return errno;
	}

	mp_dev = find_multipath_jbod(dev);
	if (mp_dev) {
		sysfs_device_name = mp_dev->scsi_dev_data->sysfs_device_name;
		size = strnlen(sysfs_device_name, sizeof(sysfs_device_name));
		rc = write(fd, sysfs_device_name, size);
		if (rc < 0) {
			close(fd);
			return errno;
		}
	}

	close(fd);
	return 0;
}

int check_sg_module()
{
	DIR *sg_dirfd;
	char devpath[PATH_MAX];

	sprintf(devpath, "%s", "/sys/module/sg");

	sg_dirfd = opendir(devpath);

	if (!sg_dirfd) {
		syslog_dbg("Failed to open sg parameter.\n");
		return -1;
	}

	closedir(sg_dirfd);

	return 0;
}
