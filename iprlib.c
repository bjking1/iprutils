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
 * $Header: /cvsroot/iprdd/iprutils/iprlib.c,v 1.58 2004/08/11 22:17:24 bjking1 Exp $
 */

#ifndef iprlib_h
#include "iprlib.h"
#endif

#define IPR_HOTPLUG_FW_PATH "/usr/lib/hotplug/firmware/"

static void default_exit_func()
{
}

struct ipr_array_query_data *ipr_qac_data = NULL;
struct scsi_dev_data *scsi_dev_table = NULL;
u32 num_ioas = 0;
struct ipr_ioa *ipr_ioa_head = NULL;
struct ipr_ioa *ipr_ioa_tail = NULL;
int runtime = 0;
char *tool_name = NULL;
void (*exit_func) (void) = default_exit_func;
int ipr_debug = 0;
int ipr_force = 0;

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
		supported_with_min_ucode_level: true,
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
		supported_with_min_ucode_level: true,
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
		supported_with_min_ucode_level: true,
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
		supported_with_min_ucode_level: true,
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

int get_max_bus_speed(struct ipr_ioa *ioa, int bus)
{
	struct sysfs_class_device *class_device;
	struct sysfs_attribute *attr;
	const struct ses_table_entry *ste = get_ses_entry(ioa, bus);
	u32 fw_version;
	int max_xfer_rate = IPR_MAX_XFER_RATE;

	class_device = sysfs_open_class_device("scsi_host", ioa->host_name);
	attr = sysfs_get_classdev_attr(class_device, "fw_version");
	sscanf(attr->value, "%8X", &fw_version);
	sysfs_close_class_device(class_device);

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
	{.ccin = 0x5703, .scsi_id_changeable = 0,
	.msl = 0x03090059, .fw_name = "5052414D" },
	{.ccin = 0x5709, .scsi_id_changeable = 0,
	.msl = 0x030D0047, .fw_name = "5052414E" },
	{.ccin = 0x570A, .scsi_id_changeable = 0,
	.msl = 0x020A004E, .fw_name = "44415255" },
	{.ccin = 0x570B, .scsi_id_changeable = 0,
	.msl = 0x020A004E, .fw_name = "44415255" },
	{.ccin = 0x2780, .scsi_id_changeable = 0,
	.msl = 0x030E0039, .fw_name = "5349530E" },
};

static void setup_ioa_parms(struct ipr_ioa *ioa)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ioa_parms); i++) {
		if (ioa->ccin == ioa_parms[i].ccin) {
			ioa->scsi_id_changeable = ioa_parms[i].scsi_id_changeable;
			ioa->msl = ioa_parms[i].msl;
			return;
		}
	}

	ioa->scsi_id_changeable = 1;
}

void tool_init(char *name)
{
	int ccin, rc;
	struct ipr_ioa *ipr_ioa;
	struct sysfs_driver *sysfs_ipr_driver;
	struct dlist *ipr_devs;
	char *pci_address, bus[100];

	struct sysfs_class *sysfs_host_class;
	struct sysfs_class *sysfs_device_class;
	struct dlist *class_devices;
	struct sysfs_class_device *class_device;
	struct sysfs_device *sysfs_device_device;
	struct sysfs_device *sysfs_host_device;
	struct sysfs_device *sysfs_pci_device;

	struct sysfs_attribute *sysfs_model_attr;

	if (!tool_name) {
		tool_name = malloc(strlen(name)+1);
		strcpy(tool_name, name);
	}

	for (ipr_ioa = ipr_ioa_head; ipr_ioa;) {
		ipr_ioa = ipr_ioa->next;
		free(ipr_ioa_head);
		ipr_ioa_head = ipr_ioa;
	}

	free(ipr_qac_data);
	free(scsi_dev_table);
	ipr_qac_data = NULL;
	scsi_dev_table = NULL;
	ipr_ioa_tail = NULL;
	ipr_ioa_head = NULL;
	num_ioas = 0;

	rc = sysfs_find_driver_bus("ipr", bus, 100);
	if (rc) {
		syslog_dbg("Failed to find ipr driver bus. %m\n");
		return;
	}

	/* Find all the IPR IOAs attached and save away vital info about them */
	sysfs_ipr_driver = sysfs_open_driver(bus, "ipr");
	if (sysfs_ipr_driver == NULL) {
		syslog_dbg("Failed to open ipr driver bus. %m\n");
		sysfs_ipr_driver = sysfs_open_driver("ipr", bus);
		if (sysfs_ipr_driver == NULL) {
			syslog_dbg("Failed to open ipr driver bus on second attempt. %m\n");
			return;
		}
	}

	ipr_devs = sysfs_get_driver_devices(sysfs_ipr_driver);

	if (ipr_devs != NULL) {
		dlist_for_each_data(ipr_devs, pci_address, char) {

			ipr_ioa = (struct ipr_ioa*)malloc(sizeof(struct ipr_ioa));
			memset(ipr_ioa,0,sizeof(struct ipr_ioa));

			/* PCI address */
			strcpy(ipr_ioa->pci_address, pci_address);

			sysfs_host_class = sysfs_open_class("scsi_host");
			if (!sysfs_host_class)
				exit_on_error("Failed to open scsi_host class.\n");

			class_devices = sysfs_get_class_devices(sysfs_host_class);
			dlist_for_each_data(class_devices, class_device, struct sysfs_class_device) {
				sysfs_host_device = sysfs_get_classdev_device(class_device);
				sysfs_pci_device = sysfs_get_device_parent(sysfs_host_device);

				if (strcmp(pci_address, sysfs_pci_device->name) == 0) {
					strcpy(ipr_ioa->host_name, sysfs_host_device->name);
					sscanf(ipr_ioa->host_name, "host%d", &ipr_ioa->host_num);
					break;
				}
			}
			sysfs_close_class(sysfs_host_class);

			sysfs_device_class = sysfs_open_class("scsi_device");
			if (!sysfs_device_class)
				exit_on_error("Failed to open scsi_device class\n");

			class_devices = sysfs_get_class_devices(sysfs_device_class);
			dlist_for_each_data(class_devices, class_device, struct sysfs_class_device) {
				sysfs_device_device = sysfs_get_classdev_device(class_device);
				sysfs_host_device = sysfs_get_device_parent(sysfs_device_device);

				if ((strcmp(ipr_ioa->host_name, sysfs_host_device->name) == 0) && 
				    (strstr(sysfs_device_device->name, ":255:255:255"))) {

					sysfs_model_attr = sysfs_get_device_attr(sysfs_device_device, "model");
					sscanf(sysfs_model_attr->value, "%4X", &ccin);
					ipr_ioa->ccin = ccin;
					setup_ioa_parms(ipr_ioa);
					break;
				}
			}
			sysfs_close_class(sysfs_device_class);

			ipr_ioa->next = NULL;
			ipr_ioa->num_raid_cmds = 0;

			if (ipr_ioa_tail) {
				ipr_ioa_tail->next = ipr_ioa;
				ipr_ioa_tail = ipr_ioa;
			} else
				ipr_ioa_tail = ipr_ioa_head = ipr_ioa;

			num_ioas++;
		}
	}

	sysfs_close_driver(sysfs_ipr_driver);

	return;
}

void ipr_reset_adapter(struct ipr_ioa *ioa)
{
	struct sysfs_class_device *class_device;
	struct sysfs_attribute *attr;

	class_device = sysfs_open_class_device("scsi_host",
					       ioa->host_name);
	attr = sysfs_get_classdev_attr(class_device,
				       "reset_host");
	sysfs_write_attribute(attr, "1", 1);
	sysfs_close_class_device(class_device);
}

void ipr_rescan(struct ipr_ioa *ioa, int bus, int id, int lun)
{
	struct sysfs_class_device *class_device;
	struct sysfs_attribute *attr;
	char buf[100];

	sprintf(buf, "%d %d %d", bus, id, lun);

	class_device = sysfs_open_class_device("scsi_host", ioa->host_name);
	attr = sysfs_get_classdev_attr(class_device, "scan");
	sysfs_write_attribute(attr, buf, strlen(buf));
	sysfs_close_class_device(class_device);
}

int ipr_query_array_config(struct ipr_ioa *ioa,
			   bool allow_rebuild_refresh, bool set_array_id,
			   int array_id, void *buff)
{
	int fd;
	int length = sizeof(struct ipr_array_query_data);
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;

	ioa->nr_ioa_microcode = 0;

	if (strlen(ioa->ioa.gen_name) == 0)
		return -ENOENT;

	fd = open(ioa->ioa.gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", ioa->ioa.gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_QUERY_ARRAY_CONFIG;

	if (set_array_id) {
		cdb[1] = 0x01;
		cdb[2] = array_id;
	} else if (allow_rebuild_refresh)
		cdb[1] = 0;
	else
		cdb[1] = 0x80; /* Prohibit Rebuild Candidate Refresh */

	cdb[7] = (length & 0xff00) >> 8;
	cdb[8] = length & 0xff;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if (rc != 0) {
		if (sense_data.sense_key != ILLEGAL_REQUEST)
			ioa_cmd_err(ioa, &sense_data, "Query Array Config", rc);
		else if (sense_data.sense_key == NOT_READY &&
			 sense_data.add_sense_code == 0x40 &&
			 sense_data.add_sense_code_qual == 0x85)
			ioa->nr_ioa_microcode = 1;
	}

	close(fd);
	return rc;
}

static int ipr_start_array(struct ipr_ioa *ioa, char *cmd,
			   int stripe_size, int prot_level, int hot_spare)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;
	int length = ntohs(ioa->qac_data->resp_len);

	if (strlen(ioa->ioa.gen_name) == 0)
		return -ENOENT;

	fd = open(ioa->ioa.gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", ioa->ioa.gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_START_ARRAY_PROTECTION;
	if (hot_spare)
		cdb[1] = 0x01;

	cdb[4] = (u8)(stripe_size >> 8);
	cdb[5] = (u8)(stripe_size);
	cdb[6] = prot_level;
	cdb[7] = (length & 0xff00) >> 8;
	cdb[8] = length & 0xff;

	rc = sg_ioctl(fd, cdb, ioa->qac_data,
		      length, SG_DXFER_TO_DEV,
		      &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if (rc != 0)
		ioa_cmd_err(ioa, &sense_data, "Start Array Protection", rc);

	close(fd);
	return rc;
}

int ipr_start_array_protection(struct ipr_ioa *ioa,
			       int stripe_size, int prot_level)
{
	return ipr_start_array(ioa, "Start Array Protection",
			       stripe_size, prot_level, 0);
}

int ipr_add_hot_spare(struct ipr_ioa *ioa)
{
	return ipr_start_array(ioa, "Create hot spare", 0, 0, 1);
}

static int ipr_stop_array(struct ipr_ioa *ioa, char *cmd, int hot_spare)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;
	int length = ntohs(ioa->qac_data->resp_len);

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

	rc = sg_ioctl(fd, cdb, ioa->qac_data,
		      length, SG_DXFER_TO_DEV,
		      &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if (rc != 0)
		ioa_cmd_err(ioa, &sense_data, cmd, rc);

	close(fd);
	return rc;
}

int ipr_stop_array_protection(struct ipr_ioa *ioa)
{
	return ipr_stop_array(ioa, "Stop Array Protection", 0);
}

int ipr_remove_hot_spare(struct ipr_ioa *ioa)
{
	return ipr_stop_array(ioa, "Delete hot spare", 1);
}

int ipr_rebuild_device_data(struct ipr_ioa *ioa)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;
	int length = ntohs(ioa->qac_data->resp_len);

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

	rc = sg_ioctl(fd, cdb, ioa->qac_data, length,
		      SG_DXFER_TO_DEV, &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if (rc != 0)
		ioa_cmd_err(ioa, &sense_data, "Rebuild Device Data", rc);

	close(fd);
	return rc;
}

int ipr_add_array_device(struct ipr_ioa *ioa,
			 struct ipr_array_query_data *qac_data)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;
	int length = ntohs(qac_data->resp_len);

	if (strlen(ioa->ioa.gen_name) == 0)
		return -ENOENT;

	fd = open(ioa->ioa.gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", ioa->ioa.gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_ADD_ARRAY_DEVICE;
	cdb[7] = (length & 0xff00) >> 8;
	cdb[8] = length & 0xff;

	rc = sg_ioctl(fd, cdb, qac_data,
		      length, SG_DXFER_TO_DEV,
		      &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if (rc != 0)
		ioa_cmd_err(ioa, &sense_data, "Add Array Device", rc);

	close(fd);
	return rc;
}

int ipr_query_command_status(struct ipr_dev *dev, void *buff)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int length = sizeof(struct ipr_cmd_status);

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_QUERY_COMMAND_STATUS;
	cdb[7] = (length & 0xff00) >> 8;
	cdb[8] = length & 0xff;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_ARRAY_CMD_TIMEOUT);

	if ((rc != 0) && (errno != EINVAL))
		scsi_cmd_err(dev, &sense_data, "Query Command Status", rc);

	close(fd);
	return rc;
}

int ipr_query_resource_state(struct ipr_dev *dev, void *buff)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int length = sizeof(struct ipr_query_res_state);

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
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

int ipr_mode_sense(struct ipr_dev *dev, u8 page, void *buff)
{
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int fd, rc;
	u8 length = IPR_MODE_SENSE_LENGTH; /* xxx FIXME? */

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = MODE_SENSE;
	cdb[2] = page;
	cdb[4] = length;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	/* post log if error unless error is device format in progress */
	if ((rc != 0) &&
	    (((sense_data.error_code & 0x7F) != 0x70) ||
	    ((sense_data.sense_key & 0x0F) != 0x02) || /* NOT READY */
	    (sense_data.add_sense_code != 0x04) ||     /* LOGICAL UNIT NOT READY */
	    (sense_data.add_sense_code_qual != 0x04))) /* FORMAT IN PROGRESS */

		scsi_cmd_err(dev, &sense_data, "Mode Sense", rc);

	close(fd);
	return rc;
}

int ipr_mode_select(struct ipr_dev *dev, void *buff, int length)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = MODE_SELECT;
	cdb[1] = 0x10; /* PF = 1, SP = 0 */
	cdb[4] = length;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_TO_DEV,
		      &sense_data, IPR_INTERNAL_TIMEOUT);

	if (rc != 0)
		scsi_cmd_err(dev, &sense_data, "Mode Select", rc);

	close(fd);
	return rc;
}

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

int ipr_read_capacity(struct ipr_dev *dev, void *buff)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;
	int length = sizeof(struct ipr_read_cap);

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
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

int ipr_read_capacity_16(struct ipr_dev *dev, void *buff)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int length = sizeof(struct ipr_read_cap16);

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_SERVICE_ACTION_IN;
	cdb[1] = IPR_READ_CAPACITY_16;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0)
		scsi_cmd_err(dev, &sense_data, "Read Capacity", rc);

	close(fd);
	return rc;
}

int ipr_reclaim_cache_store(struct ipr_ioa *ioa, int action, void *buff)
{
	char *file = ioa->ioa.gen_name;
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;
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

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0) {
		if (sense_data.sense_key != ILLEGAL_REQUEST)
			ioa_cmd_err(ioa, &sense_data, "Reclaim Cache", rc);
	}
	else if ((action & IPR_RECLAIM_ACTION) == IPR_RECLAIM_PERFORM)
		ipr_reset_adapter(ioa);

	close(fd);
	return rc;
}

int ipr_start_stop_start(struct ipr_dev *dev)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;
	int length = 0;

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = START_STOP;
	cdb[4] = IPR_START_STOP_START;

	rc = sg_ioctl(fd, cdb, NULL,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0)
		scsi_cmd_err(dev, &sense_data, "Start Unit", rc);

	close(fd);
	return rc;
}

int ipr_start_stop_stop(struct ipr_dev *dev)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int length = 0;

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = START_STOP;
	cdb[4] = IPR_START_STOP_STOP;

	rc = sg_ioctl(fd, cdb, NULL,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0)
		scsi_cmd_err(dev, &sense_data, "Stop Unit", rc);

	close(fd);
	return rc;
}

int ipr_test_unit_ready(struct ipr_dev *dev,
			struct sense_data_t *sense_data)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	int length = 0;

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = TEST_UNIT_READY;

	rc = sg_ioctl(fd, cdb, NULL,
		      length, SG_DXFER_FROM_DEV,
		      sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0 && sense_data->sense_key != NOT_READY)
		scsi_cmd_err(dev, sense_data, "Test Unit Ready", rc);

	close(fd);
	return rc;
}

int ipr_format_unit(struct ipr_dev *dev)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	u8 *defect_list_hdr;
	int length = IPR_DEFECT_LIST_HDR_LEN;

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	defect_list_hdr = calloc(1, IPR_DEFECT_LIST_HDR_LEN);

	cdb[0] = FORMAT_UNIT;
	cdb[1] = IPR_FORMAT_DATA; /* lun = 0, fmtdata = 1, cmplst = 0, defect list format = 0 */
	cdb[4] = 1;

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

int ipr_evaluate_device(struct ipr_dev *dev, u32 res_handle)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	u32 resource_handle;
	int length = 0;

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
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

int ipr_inquiry(struct ipr_dev *dev, u8 page, void *buff, u8 length)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
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

	if (rc != 0)
		scsi_cmd_err(dev, &sense_data, "Inquiry", rc);

	close(fd);
	return rc;
}

int ipr_write_buffer(struct ipr_dev *dev, void *buff, int length)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	struct ipr_disk_attr attr;
	int old_qdepth;

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	if ((rc = ipr_get_dev_attr(dev, &attr))) {
		scsi_dbg(dev, "Failed to get device attributes. rc=%d\n", rc);
		return rc;
	}

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

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s.\n",dev->gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = WRITE_BUFFER;
	cdb[1] = 5;
	cdb[6] = (length & 0xff0000) >> 16;
	cdb[7] = (length & 0x00ff00) >> 8;
	cdb[8] = length & 0x0000ff;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_TO_DEV,
		      &sense_data, IPR_WRITE_BUFFER_TIMEOUT);

	if (rc != 0)
		scsi_cmd_err(dev, &sense_data, "Write buffer", rc);
	else
		sleep(5);

	close(fd);
	attr.queue_depth = old_qdepth;
	rc = ipr_set_dev_attr(dev, &attr, 0);

	return rc;
}

int ipr_suspend_device_bus(struct ipr_ioa *ioa,
			   struct ipr_res_addr *res_addr, u8 option)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;
	int length = 0;

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
	cdb[3] = res_addr->bus;
	cdb[4] = res_addr->target;
	cdb[5] = res_addr->lun;

	rc = sg_ioctl(fd, cdb, NULL,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_SUSPEND_DEV_BUS_TIMEOUT);

	/* FIXME will rc != 0 if sense data */
	close(fd);
	return rc;
}

int ipr_receive_diagnostics(struct ipr_dev *dev,
			    u8 page, void *buff, int length)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
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

int ipr_send_diagnostics(struct ipr_dev *dev, void *buff, int length)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
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

	for (i=0; i < ARRAY_SIZE(unsupported_dasd); i++) {
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

int device_supported(struct ipr_dev *dev)
{
	struct ipr_std_inq_data std_inq;

	if (ipr_inquiry(dev, IPR_STD_INQUIRY, &std_inq, sizeof(std_inq)))
		return -EIO;

	return __device_supported(dev, &std_inq);
}

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

#define IPR_MAX_XFER 0x8000
const int cdb_size[] ={6, 10, 10, 0, 16, 12, 16, 16};
static int _sg_ioctl(int fd, u8 cdb[IPR_CCB_CDB_LEN],
		     void *data, u32 xfer_len, u32 data_direction,
		     struct sense_data_t *sense_data,
		     u32 timeout_in_sec, int retries)
{
	int rc;
	sg_io_hdr_t io_hdr_t;
	sg_iovec_t *iovec;
	int iovec_count;
	int i;
	int buff_len, segment_size;
	void *dxferp;

	/* check if scatter gather should be used */
	if (xfer_len > IPR_MAX_XFER) {
		iovec_count = xfer_len/IPR_MAX_XFER + 1;
		iovec = malloc(iovec_count * sizeof(sg_iovec_t));

		buff_len = xfer_len;
		segment_size = IPR_MAX_XFER;

		for (i = 0; (i < iovec_count) && (buff_len != 0); i++) {
			iovec[i].iov_base = malloc(segment_size);
			memcpy(iovec[i].iov_base, data + IPR_MAX_XFER * i, segment_size);
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

		if (rc == 0 && io_hdr_t.masked_status == CHECK_CONDITION)
			rc = CHECK_CONDITION;
		else if (rc == 0 && (io_hdr_t.host_status || io_hdr_t.driver_status))
			rc = -EIO;

		if (rc == 0 || io_hdr_t.host_status == 1)
			break;
	}

	if (iovec_count) {
		for (i = 0; i < iovec_count; i++)
			free(iovec[i].iov_base);
		free(iovec);
	}

	return rc;
};

int sg_ioctl(int fd, u8 cdb[IPR_CCB_CDB_LEN],
	     void *data, u32 xfer_len, u32 data_direction,
	     struct sense_data_t *sense_data,
	     u32 timeout_in_sec)
{
	return _sg_ioctl(fd, cdb,
			 data, xfer_len, data_direction,
			 sense_data, timeout_in_sec, 1);
};

int sg_ioctl_noretry(int fd, u8 cdb[IPR_CCB_CDB_LEN],
		     void *data, u32 xfer_len, u32 data_direction,
		     struct sense_data_t *sense_data,
		     u32 timeout_in_sec)
{
	return _sg_ioctl(fd, cdb,
			 data, xfer_len, data_direction,
			 sense_data, timeout_in_sec, 0);
};

int get_scsi_dev_data(struct scsi_dev_data **scsi_dev_ref)
{
	int num_devs = 0;
	struct scsi_dev_data *scsi_dev_data;
	struct scsi_dev_data *scsi_dev_base = *scsi_dev_ref;
	struct sysfs_class *sysfs_device_class;
	struct dlist *class_devices;
	struct sysfs_class_device *class_device;
	struct sysfs_device *sysfs_device_device;
	struct sysfs_attribute *sysfs_attr;

	sysfs_device_class = sysfs_open_class("scsi_device");
	if (!sysfs_device_class) {
		syslog_dbg("Failed to open scsi_device class. %m\n");
		return 0;
	}

	class_devices = sysfs_get_class_devices(sysfs_device_class);
	if (!class_devices) {
		syslog_dbg("Get class devices for scsi_device failed. %m\n");
		return 0;
	}

	dlist_for_each_data(class_devices, class_device, struct sysfs_class_device) {

		scsi_dev_base = realloc(scsi_dev_base,
					sizeof(struct scsi_dev_data) * (num_devs + 1));
		scsi_dev_data = &scsi_dev_base[num_devs];

		sysfs_device_device = sysfs_get_classdev_device(class_device);
		if (!sysfs_device_device) {
			*scsi_dev_ref = scsi_dev_base;
			return -1;
		}

		sscanf(sysfs_device_device->name,"%d:%d:%d:%d",
		       &scsi_dev_data->host,
		       &scsi_dev_data->channel,
		       &scsi_dev_data->id,
		       &scsi_dev_data->lun);

		sprintf(scsi_dev_data->sysfs_device_name,
			sysfs_device_device->name);

		sysfs_attr = sysfs_get_device_attr(sysfs_device_device, "type");
		if (sysfs_attr)
			sscanf(sysfs_attr->value, "%d", &scsi_dev_data->type);

		//FIXME WHERE TO GET OPENS!!!
			/*
			 sysfs_attr = sysfs_get_device_attr(sysfs_device_device, "opens");
			 sscanf(sysfs_attr->value "%d", &scsi_dev_data->opens);
			 */      scsi_dev_data->opens = 0;

		sysfs_attr = sysfs_get_device_attr(sysfs_device_device, "online");
		if (sysfs_attr) {
			sscanf(sysfs_attr->value, "%d", &scsi_dev_data->online);
		} else {
			sysfs_attr = sysfs_get_device_attr(sysfs_device_device, "state");
			if (sysfs_attr && strstr(sysfs_attr->value, "offline"))
				scsi_dev_data->online = 0;
			else
				scsi_dev_data->online = 1;
		}

		sysfs_attr = sysfs_get_device_attr(sysfs_device_device, "queue_depth");
		if (sysfs_attr)
			sscanf(sysfs_attr->value, "%d", &scsi_dev_data->qdepth);

		sysfs_attr = sysfs_get_device_attr(sysfs_device_device, "adapter_handle");
		if (sysfs_attr)
			sscanf(sysfs_attr->value, "%X", &scsi_dev_data->handle);

		sysfs_attr = sysfs_get_device_attr(sysfs_device_device, "vendor");
		if (sysfs_attr)
			ipr_strncpy_0(scsi_dev_data->vendor_id, sysfs_attr->value, IPR_VENDOR_ID_LEN);

		sysfs_attr = sysfs_get_device_attr(sysfs_device_device, "model");
		if (sysfs_attr)
			ipr_strncpy_0(scsi_dev_data->product_id, sysfs_attr->value, IPR_PROD_ID_LEN);

		strcpy(scsi_dev_data->dev_name,"");
		strcpy(scsi_dev_data->gen_name,"");
		num_devs++;
	}
	sysfs_close_class(sysfs_device_class);

	*scsi_dev_ref = scsi_dev_base;
	return num_devs;
}

static void get_sg_names(int num_devs)
{
	int i;
	struct sysfs_class *sysfs_device_class;
	struct dlist *class_devices;
	struct sysfs_class_device *class_device;
	struct sysfs_device *sysfs_device_device;

	sysfs_device_class = sysfs_open_class("scsi_generic");
	if (!sysfs_device_class) {
		syslog_dbg("Failed to open scsi_generic class. %m\n");
		return;
	}

	class_devices = sysfs_get_class_devices(sysfs_device_class);
	if (!class_devices) {
		syslog_dbg("Failed to get class devices for scsi_generic class. %m\n");
		sysfs_close_class(sysfs_device_class);
		return;
	}

	dlist_for_each_data(class_devices, class_device, struct sysfs_class_device) {
		sysfs_device_device = sysfs_get_classdev_device(class_device);
		if (!sysfs_device_device)
			continue;

		for (i = 0; i < num_devs; i++) {
			if (!strcmp(scsi_dev_table[i].sysfs_device_name,
				    sysfs_device_device->name)) {
				sprintf(scsi_dev_table[i].gen_name, "/dev/%s",
					class_device->name);
				break;
			}
		}
	}
	sysfs_close_class(sysfs_device_class);
}

static void get_sd_names(int num_devs)
{
	int i;
	struct sysfs_class *sysfs_device_class;
	struct dlist *class_devices;
	struct sysfs_class_device *class_device;
	struct sysfs_device *sysfs_device_device;

	sysfs_device_class = sysfs_open_class("block");
	if (!sysfs_device_class) {
		syslog_dbg("Failed to open block device class. %m\n");
		return;
	}

	class_devices = sysfs_get_class_devices(sysfs_device_class);
	if (!class_devices) {
		syslog_dbg("Failed to get class devices for block device class. %m\n");
		return;
	}

	dlist_for_each_data(class_devices, class_device, struct sysfs_class_device) {
		sysfs_device_device = sysfs_get_classdev_device(class_device);
		if (!sysfs_device_device)
			continue;

		for (i = 0; i < num_devs; i++) {
			if (!strcmp(scsi_dev_table[i].sysfs_device_name,
				    sysfs_device_device->name)) {
				sprintf(scsi_dev_table[i].dev_name, "/dev/%s",
					class_device->name);
				break;
			}
		}
	}
	sysfs_close_class(sysfs_device_class);
}

static void get_ioa_name(struct ipr_ioa *cur_ioa,
			 int num_sg_devices)
{
	int i;
	int host_no;

	sscanf(cur_ioa->host_name, "host%d", &host_no);

	for (i = 0; i < num_sg_devices; i++) {
		if (scsi_dev_table[i].host == host_no &&
		    scsi_dev_table[i].channel == 255 &&
		    scsi_dev_table[i].id == 255 &&
		    scsi_dev_table[i].lun == 255 &&
		    scsi_dev_table[i].type == IPR_TYPE_ADAPTER) {
			strcpy(cur_ioa->ioa.dev_name, scsi_dev_table[i].dev_name);
			strcpy(cur_ioa->ioa.gen_name, scsi_dev_table[i].gen_name);
		}
	}
}

void check_current_config(bool allow_rebuild_refresh)
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
	int *qac_entry_ref;

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

		/* Get Query Array Config Data */
		rc = ipr_query_array_config(ioa, allow_rebuild_refresh,
					    0, 0, qac_data);

		if (rc != 0)
			qac_data->num_records = 0;

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

			if (scsi_dev_data->type == TYPE_DISK ||
			    scsi_dev_data->type == IPR_TYPE_AF_DISK ||
			    scsi_dev_data->type == TYPE_ENCLOSURE) {
				ioa->dev[device_count].ioa = ioa;
				ioa->dev[device_count].scsi_dev_data = scsi_dev_data;
				ioa->dev[device_count].qac_entry = NULL;
				strcpy(ioa->dev[device_count].dev_name,
				       scsi_dev_data->dev_name);
				strcpy(ioa->dev[device_count].gen_name,
				       scsi_dev_data->gen_name);

				/* find array config data matching resource entry */
				common_record = (struct ipr_common_record *)qac_data->data;

				for (k = 0; k < qac_data->num_records; k++) {
					if (common_record->record_id == IPR_RECORD_ID_DEVICE_RECORD) {
						device_record = (struct ipr_dev_record *)common_record;
						if (device_record->resource_addr.bus == scsi_dev_data->channel &&
						    device_record->resource_addr.target == scsi_dev_data->id &&
						    device_record->resource_addr.lun == scsi_dev_data->lun) {
							ioa->dev[device_count].qac_entry = common_record;
							qac_entry_ref[k]++;
							break;
						}
					} else if (common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD) {
						array_record = (struct ipr_array_record *)common_record;
						if (array_record->resource_addr.bus == scsi_dev_data->channel &&
						    array_record->resource_addr.target == scsi_dev_data->id &&
						    array_record->resource_addr.lun == scsi_dev_data->lun) {
							ioa->dev[device_count].qac_entry = common_record;
							qac_entry_ref[k]++;
							break;
						}
					}                    

					common_record = (struct ipr_common_record *)
						((unsigned long)common_record + ntohs(common_record->record_len));
				}

				/* Send Test Unit Ready to start device if its a volume set */
				if (ipr_is_volume_set(&ioa->dev[device_count]))
					ipr_test_unit_ready(&ioa->dev[device_count], &sense_data);

				device_count++;
			} else if (scsi_dev_data->type == IPR_TYPE_ADAPTER) {
				ioa->ioa.ioa = ioa;
				ioa->ioa.scsi_dev_data = scsi_dev_data;
			}
		}

		/* now scan qac device and array entries to see which ones have
		 not been referenced */
		common_record = (struct ipr_common_record *)qac_data->data;

		for (k = 0; k < qac_data->num_records; k++) {
			if (qac_entry_ref[k] > 1)
				syslog(LOG_ERR,
				       "Query Array Config entry referenced more than once\n");

			if (common_record->record_id == IPR_RECORD_ID_SUPPORTED_ARRAYS) {
				ioa->supported_arrays = (struct ipr_supported_arrays *)common_record;
			} else if (!qac_entry_ref[k] &&
				   (common_record->record_id == IPR_RECORD_ID_DEVICE_RECORD ||
				    common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD)) {

				array_record = (struct ipr_array_record *)common_record;
				if (common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD &&
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

			common_record = (struct ipr_common_record *)
				((unsigned long)common_record + ntohs(common_record->record_len));
		}

		ioa->num_devices = device_count;
		free(qac_entry_ref);
	}
}

/* xxx delete */
int num_device_opens(int host_num, int channel, int id, int lun)
{
	struct scsi_dev_data *scsi_dev_base = NULL;
	int opens = 0;
	int k, num_sg_devices;

	/* Get sg data via sg proc file system */
	num_sg_devices = get_scsi_dev_data(&scsi_dev_base);

	/* find usage counts in scsi_dev_data */
	for (k=0; k < num_sg_devices; k++)
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

static void ipr_save_bus_attr(struct ipr_ioa *ioa, int bus,
			      char *field, char *value, int update)
{
	char category[16];
	sprintf(category,"[%s %d]", IPR_CATEGORY_BUS, bus);
	ipr_save_attr(ioa, category, field, value, update);
}

static void ipr_save_dev_attr(struct ipr_dev *dev, char *field,
			     char *value, int update)
{
	char category[16];
	sprintf(category,"[%s %d:%d:%d]", IPR_CATEGORY_DISK,
		dev->scsi_dev_data->channel, dev->scsi_dev_data->id,
		dev->scsi_dev_data->lun);
	ipr_save_attr(dev->ioa, category, field, value, update);
}

static int ipr_get_saved_attr(struct ipr_ioa *ioa, char *category,
				  char *field, char *value)
{
	FILE *fd;
	char fname[64], line[64], *str_ptr;
	int bus_found = 0;

	sprintf(fname, "%s%x_%s", IPR_CONFIG_DIR, ioa->ccin, ioa->pci_address);

	fd = fopen(fname, "r");
	if (fd == NULL)
		return RC_FAILED;

	while (NULL != fgets(line, 64, fd)) {
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
					sprintf(value,"%s\n",str_ptr);
					fclose(fd);
					return RC_SUCCESS;
				}
			}
		}
	}

	fclose(fd);
	return RC_FAILED;
}

static int ipr_get_saved_bus_attr(struct ipr_ioa *ioa, int bus,
				  char *field, char *value)
{
	char category[16];
	sprintf(category,"[%s %d]", IPR_CATEGORY_BUS, bus);
	return ipr_get_saved_attr(ioa, category, field, value);
}

static int ipr_get_saved_dev_attr(struct ipr_dev *dev,
				  char *field, char *value)
{
	char category[30];

	if (!dev->scsi_dev_data)
		return -ENXIO;

	sprintf(category,"[%s %d:%d:%d]", IPR_CATEGORY_DISK,
		dev->scsi_dev_data->channel, dev->scsi_dev_data->id,
		dev->scsi_dev_data->lun);
	return ipr_get_saved_attr(dev->ioa, category, field, value);
}

int ipr_get_dev_attr(struct ipr_dev *dev, struct ipr_disk_attr *attr)
{
	char temp[100];

	if (ipr_read_dev_attr(dev, "queue_depth", temp))
		return -EIO;

	attr->queue_depth = strtoul(temp, NULL, 10);

	if (ipr_read_dev_attr(dev, "tcq_enable", temp))
		return -EIO;
	attr->tcq_enabled = strtoul(temp, NULL, 10);

	attr->format_timeout = IPR_FORMAT_UNIT_TIMEOUT;

	return 0;
}

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

	return 0;
}

int ipr_set_dev_attr(struct ipr_dev *dev, struct ipr_disk_attr *attr, int save)
{
	struct ipr_disk_attr old_attr;
	char temp[100];

	if (ipr_get_dev_attr(dev, &old_attr))
		return -EIO;

	if (attr->queue_depth != old_attr.queue_depth) {
		if (!ipr_is_af_dasd_device(dev)) {
			sprintf(temp, "%d", attr->queue_depth);
			if (ipr_write_dev_attr(dev, "queue_depth", temp))
				return -EIO;
			if (save)
				ipr_save_dev_attr(dev, IPR_QUEUE_DEPTH, temp, 1);
		}
	}

	if (attr->format_timeout != old_attr.format_timeout) {
		if (ipr_is_af_dasd_device(dev)) {
			sprintf(temp, "%d", attr->format_timeout);
			if (ipr_set_dasd_timeouts(dev))
				return -EIO;
			if (save)
				ipr_save_dev_attr(dev, IPR_FORMAT_TIMEOUT, temp, 1);
		}
	}

	if (attr->tcq_enabled != old_attr.tcq_enabled) {
		if (!ipr_is_af_dasd_device(dev)) {
			sprintf(temp, "%d", attr->tcq_enabled);
			if (ipr_write_dev_attr(dev, "tcq_enable", temp))
				return -EIO;
			if (save)
				ipr_save_dev_attr(dev, IPR_TCQ_ENABLED, temp, 1);
		}
	}

	return 0;
}

static const struct ipr_dasd_timeout_record ipr_dasd_timeouts[] = {
	{FORMAT_UNIT, 0, __constant_cpu_to_be16(3 * 60 * 60)},
	{TEST_UNIT_READY, 0, __constant_cpu_to_be16(30)},
	{REQUEST_SENSE, 0, __constant_cpu_to_be16(30)},
	{INQUIRY, 0, __constant_cpu_to_be16(30)},
	{MODE_SELECT, 0, __constant_cpu_to_be16(30)},
	{MODE_SENSE, 0, __constant_cpu_to_be16(30)},
	{READ_CAPACITY, 0, __constant_cpu_to_be16(30)},
	{READ_10, 0, __constant_cpu_to_be16(30)},
	{WRITE_10, 0, __constant_cpu_to_be16(30)},
	{WRITE_VERIFY, 0, __constant_cpu_to_be16(30)},
	{REASSIGN_BLOCKS, 0, __constant_cpu_to_be16(10 * 60)},
	{START_STOP, 0, __constant_cpu_to_be16(2 * 60)},
	{SEND_DIAGNOSTIC, 0, __constant_cpu_to_be16(5 * 60)},
	{VERIFY, 0, __constant_cpu_to_be16(5 * 60)},
	{WRITE_BUFFER, 0, __constant_cpu_to_be16(5 * 60)},
	{WRITE_SAME, 0, __constant_cpu_to_be16(4 * 60 * 60)},
	{LOG_SENSE, 0, __constant_cpu_to_be16(30)},
	{REPORT_LUNS, 0, __constant_cpu_to_be16(30)},
	{SKIP_READ, 0, __constant_cpu_to_be16(30)},
	{SKIP_WRITE, 0, __constant_cpu_to_be16(30)}
};

struct ipr_dasd_timeouts {
	u32 length;
	struct ipr_dasd_timeout_record record[ARRAY_SIZE(ipr_dasd_timeouts)];
};

int ipr_set_dasd_timeouts(struct ipr_dev *dev)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	struct ipr_dasd_timeouts timeouts;
	struct ipr_disk_attr attr;

	fd = open(dev->gen_name, O_RDWR);

	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n",
			       dev->ioa->ioa.gen_name);
		return errno;
	}

	memcpy(timeouts.record, ipr_dasd_timeouts, sizeof(ipr_dasd_timeouts));
	timeouts.length = htonl(sizeof(timeouts));

	if (!ipr_get_dev_attr(dev, &attr)) {
		if (attr.format_timeout >= IPR_TIMEOUT_MINUTE_RADIX) {
			timeouts.record[0].timeout =
				(attr.format_timeout / 60) & IPR_TIMEOUT_MINUTE_RADIX;
		} else {
			timeouts.record[0].timeout = attr.format_timeout;
		}
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);
	cdb[0] = SET_DASD_TIMEOUTS;
	cdb[7] = (sizeof(timeouts) >> 8) & 0xff;
	cdb[8] = sizeof(timeouts) & 0xff;

	rc = sg_ioctl(fd, cdb, &timeouts,
		      sizeof(timeouts), SG_DXFER_TO_DEV,
		      &sense_data, SET_DASD_TIMEOUTS_TIMEOUT);

	if (rc != 0)
		scsi_cmd_err(dev, &sense_data, "Set DASD timeouts", rc);

	close(fd);
	return rc;
}

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

	ipr_get_bus_attr(ioa, &old_settings);

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

struct unsupported_af_dasd *
get_unsupp_af(struct ipr_std_inq_data *inq,
	      struct ipr_dasd_inquiry_page3 *page3)
{
	int i, j;

	for (i=0; i < ARRAY_SIZE(unsupported_af); i++) {
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

bool is_af_blocked(struct ipr_dev *ipr_dev, int silent)
{
	int rc;
	struct ipr_std_inq_data std_inq_data;
	struct ipr_dasd_inquiry_page3 dasd_page3_inq;
	struct unsupported_af_dasd *unsupp_af;

	/* Zero out inquiry data */
	memset(&std_inq_data, 0, sizeof(std_inq_data));
	rc = ipr_inquiry(ipr_dev, IPR_STD_INQUIRY,
			 &std_inq_data, sizeof(std_inq_data));

	if (rc != 0)
		return false;

	/* Issue page 3 inquiry */
	memset(&dasd_page3_inq, 0, sizeof(dasd_page3_inq));
	rc = ipr_inquiry(ipr_dev, 0x03,
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
					scsi_err(ipr_dev, "Disk %s needs updated microcode "
						 "before transitioning to 522 bytes/sector "
						 "format. IGNORING SINCE --force USED!",
						 ipr_dev->gen_name);
				return false;
			} else {
				if (!silent)
					scsi_err(ipr_dev, "Disk %s needs updated microcode "
					       "before transitioning to 522 bytes/sector "
					       "format.", ipr_dev->gen_name);
				return true;
			}
		}
	} else {/* disk is not supported at all */
		if (!silent)
			syslog(LOG_ERR,"Disk %s canot be formatted to "
			       "522 bytes/sector.", ipr_dev->gen_name);
		return true;
	}

	return false;
}

int ipr_read_dev_attr(struct ipr_dev *dev, char *attr, char *value)
{
	struct sysfs_class_device *class_device;
	struct sysfs_attribute *dev_attr;
	struct sysfs_device *device;
	char *sysfs_dev_name;
	int rc;

	if (!dev->scsi_dev_data) {
		scsi_dbg(dev, "Cannot read dev attr %s. NULL scsi data\n", attr);
		return -ENOENT;
	}

	sysfs_dev_name = dev->scsi_dev_data->sysfs_device_name;

	class_device = sysfs_open_class_device("scsi_device",
					       sysfs_dev_name);
	if (!class_device) {
		scsi_dbg(dev, "Failed to open scsi_device class device. %m\n");
		return -EIO;
	}

	device = sysfs_get_classdev_device(class_device);
	if (!device) {
		scsi_dbg(dev, "Failed to get classdev device. %m\n");
		sysfs_close_class_device(class_device);
		return -EIO;
	}

	dev_attr = sysfs_get_device_attr(device, attr);
	if (!dev_attr) {
		scsi_dbg(dev, "Failed to get %s attribute. %m\n", attr);
		sysfs_close_class_device(class_device);
		return -EIO;
	}

	rc = sysfs_read_attribute(dev_attr);

	if (rc) {
		scsi_dbg(dev, "Failed to read %s attribute. %m\n", attr);
		sysfs_close_class_device(class_device);
		return -EIO;
	}

	sprintf(value, "%s", dev_attr->value);
	value[strlen(value)-1] = '\0';
	sysfs_close_class_device(class_device);
	return 0;
}

int ipr_write_dev_attr(struct ipr_dev *dev, char *attr, char *value)
{
	struct sysfs_class_device *class_device;
	struct sysfs_attribute *dev_attr;
	struct sysfs_device *device;
	char *sysfs_dev_name;

	if (!dev->scsi_dev_data)
		return -ENOENT;

	sysfs_dev_name = dev->scsi_dev_data->sysfs_device_name;

	class_device = sysfs_open_class_device("scsi_device",
					       sysfs_dev_name);

	if (!class_device)
		return -EIO;

	device = sysfs_get_classdev_device(class_device);

	if (!device) {
		sysfs_close_class_device(class_device);
		return -EIO;
	}

	dev_attr = sysfs_get_device_attr(device, attr);
	if (sysfs_write_attribute(dev_attr, value, strlen(value))) {
		sysfs_close_class_device(class_device);
		return -EIO;
	}

	sysfs_close_class_device(class_device);
	return 0;
}

static u32 get_ioa_ucode_version(char *ucode_file)
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

/* Sort in decending order */
static int fw_compare(const void *parm1,
		      const void *parm2)
{
	struct ipr_fw_images *first = (struct ipr_fw_images *)parm1;
	struct ipr_fw_images *second = (struct ipr_fw_images *)parm2;

	return memcmp(&second->version, &first->version,
		      sizeof(second->version));
}

int get_ioa_firmware_image_list(struct ipr_ioa *ioa,
				struct ipr_fw_images **list)
{
	char etc_ucode_file[100];
	const struct ioa_parms *parms = NULL;
	struct dirent **dirent;
	struct ipr_fw_images *ret = NULL;
	struct stat stats;
	int i, rc, j = 0;

	*list = NULL;

	for (i = 0; i < ARRAY_SIZE(ioa_parms); i++) {
		if (ioa_parms[i].ccin == ioa->ccin) {
			parms = &ioa_parms[i];
			break;
		}
	}

	if (parms) {
		rc = scandir(UCODE_BASE_DIR, &dirent, NULL, alphasort);

		for (i = 0; i < rc && rc > 0; i++) {
			if (strstr(dirent[i]->d_name, parms->fw_name) ==
			    dirent[i]->d_name) {
				ret = realloc(ret, sizeof(*ret) * (j + 1));
				sprintf(ret[j].file, UCODE_BASE_DIR"/%s",
					dirent[i]->d_name);
				ret[j].version = get_ioa_ucode_version(ret[j].file);
				ret[j].has_header = 1;
				j++;
			}
		}
		for (i = 0; i < rc; i++)
			free(dirent[i]);
		if (rc > 0)
			free(dirent);
	}

	if (parms) {
		rc = scandir(HOTPLUG_BASE_DIR, &dirent, NULL, alphasort);

		for (i = 0; i < rc && rc > 0; i++) {
			if (strstr(dirent[i]->d_name, "IBM-eServer-") &&
			    strstr(dirent[i]->d_name, parms->fw_name)) {
				ret = realloc(ret, sizeof(*ret) * (j + 1));
				sprintf(ret[j].file, HOTPLUG_BASE_DIR"/%s",
					dirent[i]->d_name);
				ret[j].version = get_ioa_ucode_version(ret[j].file);
				ret[j].has_header = 1;
				j++;
			}
		}
		for (i = 0; i < rc; i++)
			free(dirent[i]);
		if (rc > 0)
			free(dirent);
	}

	sprintf(etc_ucode_file, "/etc/microcode/ibmsis%X.img", ioa->ccin);
	if (!stat(etc_ucode_file, &stats)) {
		ret = realloc(ret, sizeof(*ret) * (j + 1));
		strcpy(ret[j].file, etc_ucode_file);
		ret[j].version = get_ioa_ucode_version(ret[j].file);
		ret[j].has_header = 1;
		j++;
	}

	if (ret)
		qsort(ret, j, sizeof(*ret), fw_compare);

	*list = ret;
	return j;
}

static u32 get_dasd_ucode_version(char *ucode_file, int ftype)
{
	int fd;
	struct stat ucode_stats;
	struct ipr_dasd_ucode_header *hdr;
	char *tmp;
	u32 rc;

	fd = open(ucode_file, O_RDONLY);

	if (fd == -1)
		return 0;

	if (ftype == IPR_DASD_UCODE_ETC) {
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

		rc = (hdr->modification_level[0] << 24) |
			(hdr->modification_level[1] << 16) |
			(hdr->modification_level[2] << 8) |
			hdr->modification_level[3];

		munmap(hdr, ucode_stats.st_size);
		close(fd);
	} else {
		tmp = strrchr(ucode_file, '.');

		if (!tmp)
			return 0;

		rc = strtoul(tmp+1, NULL, 16);
	}

	return rc;
}

int get_dasd_firmware_image_list(struct ipr_dev *dev,
				 struct ipr_fw_images **list)
{
	char etc_ucode_file[100];
	char prefix[100];
	struct dirent **dirent;
	struct ipr_fw_images *ret = NULL;
	struct ipr_dasd_inquiry_page3 page3_inq;
	struct stat stats;
	int i, rc, j = 0;

	*list = NULL;

	memset(&page3_inq, 0, sizeof(page3_inq));
	rc = ipr_inquiry(dev, 3, &page3_inq, sizeof(page3_inq));

	if (rc != 0) {
		scsi_dbg(dev, "Inquiry failed\n");
		return -EIO;
	}

	if (memcmp(dev->scsi_dev_data->vendor_id, "IBMAS400", 8) == 0) {
		sprintf(prefix, "ibmsis%02X%02X%02X%02X.img",
			page3_inq.load_id[0], page3_inq.load_id[1],
			page3_inq.load_id[2], page3_inq.load_id[3]);
	} else if (memcmp(dev->scsi_dev_data->vendor_id, "IBM     ", 8) == 0) {
		sprintf(prefix, "%.7s.%02X%02X%02X%02X",
			dev->scsi_dev_data->product_id,
			page3_inq.load_id[0], page3_inq.load_id[1],
			page3_inq.load_id[2], page3_inq.load_id[3]);
	} else
		return 0;

	rc = scandir(UCODE_BASE_DIR, &dirent, NULL, alphasort);

	if (rc > 0) {
		rc--;

		for (i = rc ; i >= 0; i--) {
			if (strstr(dirent[i]->d_name, prefix) == dirent[i]->d_name) {
				ret = realloc(ret, sizeof(*ret) * (j + 1));
				sprintf(ret[j].file, UCODE_BASE_DIR"/%s",
					dirent[i]->d_name);
				ret[j].version = get_dasd_ucode_version(ret[j].file,
									IPR_DASD_UCODE_USRLIB);
				ret[j].has_header = 0;
				j++;
				break;
			}
		}
		for (i = rc; i >= 0; i--)
			free(dirent[i]);
		free(dirent);
	}

	rc = scandir(HOTPLUG_BASE_DIR, &dirent, NULL, alphasort);

	if (rc > 0) {
		rc--;

		for (i = rc ; i >= 0; i--) {
			if (strstr(dirent[i]->d_name, "IBM-eServer-") &&
			    strstr(dirent[i]->d_name, prefix)) {
				ret = realloc(ret, sizeof(*ret) * (j + 1));
				sprintf(ret[j].file, HOTPLUG_BASE_DIR"/%s",
					dirent[i]->d_name);
				ret[j].version = get_dasd_ucode_version(ret[j].file,
									IPR_DASD_UCODE_USRLIB);
				ret[j].has_header = 0;
				j++;
				break;
			}
		}
		for (i = rc; i >= 0; i--)
			free(dirent[i]);
		free(dirent);
	}

	sprintf(etc_ucode_file, "/etc/microcode/device/%s", prefix);

	if (!stat(etc_ucode_file, &stats)) {
		ret = realloc(ret, sizeof(*ret) * (j + 1));
		strcpy(ret[j].file, etc_ucode_file);
		ret[j].version = get_dasd_ucode_version(ret[j].file,
							IPR_DASD_UCODE_ETC);
		if (memcmp(dev->scsi_dev_data->vendor_id, "IBMAS400", 8) == 0)
			ret[j].has_header = 0;
		else
			ret[j].has_header = 1;
		j++;
	}

	if (ret)
		qsort(ret, j, sizeof(*ret), fw_compare);
	else
		syslog_dbg("Could not find firmware file %s.\n", prefix);

	*list = ret;
	return j;
}

void ipr_update_ioa_fw(struct ipr_ioa *ioa,
		       struct ipr_fw_images *image, int force)
{
	struct sysfs_class_device *class_device;
	struct sysfs_attribute *attr;
	struct ipr_ioa_ucode_header *image_hdr;
	struct stat ucode_stats;
	u32 fw_version;
	int fd, rc;
	char *tmp;
	char ucode_file[200];
	DIR *dir;

	class_device = sysfs_open_class_device("scsi_host", ioa->host_name);
	attr = sysfs_get_classdev_attr(class_device, "fw_version");
	sscanf(attr->value, "%8X", &fw_version);
	sysfs_close_class_device(class_device);

	if (fw_version >= ioa->msl && !force)
		return;

	fd = open(image->file, O_RDONLY);

	if (fd < 0) {
		syslog(LOG_ERR, "Could not open firmware file %s.\n", image->file);
		return;
	}

	rc = fstat(fd, &ucode_stats);

	if (rc != 0) {
		syslog(LOG_ERR, "Failed to stat IOA firmware file: %s.\n", image->file);
		close(fd);
		return;
	}

	image_hdr = mmap(NULL, ucode_stats.st_size, PROT_READ, MAP_SHARED, fd, 0);

	if (image_hdr == MAP_FAILED) {
		syslog(LOG_ERR, "Error mapping IOA firmware file: %s.\n", image->file);
		close(fd);
		return;
	}

	if (ntohl(image_hdr->rev_level) > fw_version || force) {
		ioa_info(ioa, "Updating adapter microcode from %08X to %08X.\n",
			 fw_version, ntohl(image_hdr->rev_level));

		tmp = strrchr(image->file, '/');
		tmp++;
		dir = opendir(IPR_HOTPLUG_FW_PATH);
		if (!dir) {
			syslog(LOG_ERR, "Failed to open %s\n", IPR_HOTPLUG_FW_PATH);
			munmap(image_hdr, ucode_stats.st_size);
			close(fd);
			return;
		}
		closedir(dir);
		sprintf(ucode_file, IPR_HOTPLUG_FW_PATH".%s", tmp);
		symlink(image->file, ucode_file);
		sprintf(ucode_file, ".%s\n", tmp);
		class_device = sysfs_open_class_device("scsi_host", ioa->host_name);
		attr = sysfs_get_classdev_attr(class_device, "update_fw");
		rc = sysfs_write_attribute(attr, ucode_file, strlen(ucode_file));
		sysfs_close_class_device(class_device);
		sprintf(ucode_file, IPR_HOTPLUG_FW_PATH".%s", tmp);
		unlink(ucode_file);

		if (rc != 0)
			ioa_err(ioa, "Microcode update failed. rc=%d\n", rc);
		else {
			/* Write buffer was successful. Now we need to see if all our devices
			 are available. The IOA may have been in braindead prior to the ucode
			 download, in which case the upper layer scsi device drivers do not
			 know anything about our devices. */
			check_current_config(false);
		}
	}

	munmap(image_hdr, ucode_stats.st_size);
	close(fd);
}

void ipr_update_disk_fw(struct ipr_dev *dev,
			struct ipr_fw_images *image, int force)
{
	int rc = 0;
	struct stat ucode_stats;
	int fd, img_size, img_off;
	struct ipr_dasd_ucode_header *img_hdr;
	struct ipr_dasd_inquiry_page3 page3_inq;
	struct ipr_std_inq_data std_inq_data;
	struct unsupported_af_dasd *unsupp_af;
	void *buffer;
	u32 level;

	memset(&std_inq_data, 0, sizeof(std_inq_data));
	rc = ipr_inquiry(dev, IPR_STD_INQUIRY,
			 &std_inq_data, sizeof(std_inq_data));

	if (rc != 0)
		return;

	rc = ipr_inquiry(dev, 3, &page3_inq, sizeof(page3_inq));

	if (rc != 0) {
		scsi_dbg(dev, "Inquiry failed\n");
		return;
	}

	if (!force) {
		unsupp_af = get_unsupp_af(&std_inq_data, &page3_inq);
		if (!unsupp_af)
			return;

		if (!disk_needs_msl(unsupp_af, &std_inq_data))
			return;
	}

	if (image->has_header)
		img_off = sizeof(struct ipr_dasd_ucode_header);
	else
		img_off = 0;

	fd = open(image->file, O_RDONLY);

	if (fd < 0) {
		syslog_dbg("Could not open firmware file %s.\n", image->file);
		return;
	}

	rc = fstat(fd, &ucode_stats);

	if (rc != 0) {
		syslog(LOG_ERR, "Failed to stat firmware file: %s.\n", image->file);
		close(fd);
		return;
	}

	img_hdr = mmap(NULL, ucode_stats.st_size,
		       PROT_READ, MAP_SHARED, fd, 0);

	if (img_hdr == MAP_FAILED) {
		syslog(LOG_ERR, "Error reading firmware file: %s.\n", image->file);
		close(fd);
		return;
	}

	if (img_off && memcmp(img_hdr->load_id, page3_inq.load_id, 4) &&
	    !memcmp(dev->scsi_dev_data->vendor_id, "IBMAS400", 8)) {
		syslog(LOG_ERR, "Firmware file corrupt: %s.\n", image->file);
		munmap(img_hdr, ucode_stats.st_size);
		close(fd);
		return;
	}

	level = htonl(image->version);

	if (memcmp(&level, page3_inq.release_level, 4) > 0 || force) {
		scsi_info(dev, "Updating disk microcode using %s "
			  "from %02X%02X%02X%02X (%c%c%c%c) to %08X (%c%c%c%c)\n", image->file,
			  page3_inq.release_level[0], page3_inq.release_level[1],
			  page3_inq.release_level[2], page3_inq.release_level[3],
			  page3_inq.release_level[0], page3_inq.release_level[1],
			  page3_inq.release_level[2], page3_inq.release_level[3],
			  image->version, image->version >> 24, (image->version >> 16) & 0xff,
			  (image->version >> 8) & 0xff, image->version & 0xff);

		buffer = (void *)img_hdr + img_off;
		img_size = ucode_stats.st_size - img_off;

		rc = ipr_write_buffer(dev, buffer, img_size);
	}

	if (munmap(img_hdr, ucode_stats.st_size))
		syslog(LOG_ERR, "munmap failed.\n");

	close(fd);
}

#define AS400_TCQ_DEPTH 16
#define DEFAULT_TCQ_DEPTH 64

static int get_tcq_depth(struct ipr_dev *dev)
{
	if (!dev->scsi_dev_data)
		return AS400_TCQ_DEPTH;
	if (!strncmp(dev->scsi_dev_data->vendor_id, "IBMAS400", 8))
		return AS400_TCQ_DEPTH;

	return DEFAULT_TCQ_DEPTH;
}

static int mode_select(struct ipr_dev *dev, void *buff, int length)
{
	int fd;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		if (!strcmp(tool_name, "iprconfig") || ipr_debug)
			syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
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
		syslog(LOG_ERR, "Failed to setup mode page 0x01 for %s.\n",
		       dev->scsi_dev_data->sysfs_device_name);
		return -EIO;
	}
	return 0;
}

static int page0x0a_setup(struct ipr_dev *dev)
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
	if (page->qerr != 1)
		return 0;
	if (page->dque != 0)
		return 0;

	return 1;
}

static int setup_page0x0a(struct ipr_dev *dev)
{
	struct ipr_mode_pages mode_pages, ch_mode_pages;
	struct ipr_control_mode_page *page, *ch_page;
	int len;

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
	IPR_SET_MODE(ch_page->qerr, page->qerr, 1);
	IPR_SET_MODE(ch_page->dque, page->dque, 0);

	if (page->queue_algorithm_modifier != 1) {
		scsi_dbg(dev, "Cannot set QAM=1\n");
		return -EIO;
	}

	if (page->qerr != 1) {
		scsi_dbg(dev, "Cannot set QERR=1\n");
			return -EIO;
	}

	if (page->dque != 0) {
		scsi_dbg(dev, "Cannot set dque=0\n");
		return -EIO;
	}

	len = mode_pages.hdr.length + 1;
	mode_pages.hdr.length = 0;
	mode_pages.hdr.medium_type = 0;
	mode_pages.hdr.device_spec_parms = 0;
	page->hdr.parms_saveable = 0;

	if (mode_select(dev, &mode_pages, len)) {
		syslog(LOG_ERR, "Failed to setup mode page 0x0A for %s.\n",
		       dev->scsi_dev_data->sysfs_device_name);
		return -EIO;
	}
	return 0;
}

static int page0x20_setup(struct ipr_dev *dev)
{
	struct ipr_mode_pages mode_pages;
	struct ipr_ioa_mode_page *page;

	memset(&mode_pages, 0, sizeof(mode_pages));

	if (ipr_mode_sense(dev, 0x20, &mode_pages))
		return 0;

	page = (struct ipr_ioa_mode_page *) (((u8 *)&mode_pages) +
					     mode_pages.hdr.block_desc_len +
					     sizeof(mode_pages.hdr));

	return (page->max_tcq_depth == get_tcq_depth(dev));
}

static int ipr_setup_page0x20(struct ipr_dev *dev)
{
	struct ipr_mode_pages mode_pages;
	struct ipr_ioa_mode_page *page;
	int len;

	if (!ipr_is_af_dasd_device(dev))
		return 0;

	memset(&mode_pages, 0, sizeof(mode_pages));

	if (ipr_mode_sense(dev, 0x20, &mode_pages))
		return -EIO;

	page = (struct ipr_ioa_mode_page *) (((u8 *)&mode_pages) +
					     mode_pages.hdr.block_desc_len +
					     sizeof(mode_pages.hdr));

	page->max_tcq_depth = get_tcq_depth(dev);

	len = mode_pages.hdr.length + 1;
	mode_pages.hdr.length = 0;
	mode_pages.hdr.medium_type = 0;
	mode_pages.hdr.device_spec_parms = 0;
	page->hdr.parms_saveable = 0;

	if (ipr_mode_select(dev, &mode_pages, len))
		return -EIO;

	return 0;
}

static int dev_init_allowed(struct ipr_dev *dev)
{
	struct ipr_query_res_state res_state;

	if (!ipr_query_resource_state(dev, &res_state)) {
		if (!res_state.read_write_prot && !res_state.prot_dev_failed)
			return 1;
	}
	return 0;
}

/*
 * VSETs:
 * 1. Adjust queue depth based on number of devices
 *
 */
static void init_vset_dev(struct ipr_dev *dev)
{
	struct ipr_query_res_state res_state;
	char q_depth[10];
	char cur_depth[100];
	int depth;

	memset(&res_state, 0, sizeof(res_state));

	if (!ipr_query_resource_state(dev, &res_state)) {
		depth = res_state.vset.num_devices_in_vset * 4;
		snprintf(q_depth, sizeof(q_depth), "%d", depth);
		q_depth[sizeof(q_depth)-1] = '\0';
		if (ipr_read_dev_attr(dev, "queue_depth", cur_depth))
			return;
		if (!strcmp(cur_depth, q_depth))
			return;
		if (ipr_write_dev_attr(dev, "queue_depth", q_depth))
			return;
	}
}

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

	if (ipr_get_dev_attr(dev, &attr))
		return;
	if (runtime && page0x0a_setup(dev) && attr.tcq_enabled)
		return;
	if (ipr_test_unit_ready(dev, &sense_data))
		return;
	if (enable_af(dev))
		return;
	if (setup_page0x0a(dev)) {
		scsi_dbg(dev, "Failed to enable TCQing.\n");
		return;
	}

	attr.queue_depth = get_tcq_depth(dev);
	attr.tcq_enabled = 1;

	if (ipr_modify_dev_attr(dev, &attr))
		return;
	if (ipr_set_dev_attr(dev, &attr, 0))
		return;
}

/*
 * AF DASD:
 * 1. Setup mode pages (pages 0x01, 0x0A, 0x20)
 * 2. Send set supported devices
 * 3. Set DASD timeouts
 */
static void init_af_dev(struct ipr_dev *dev)
{
	struct ipr_disk_attr attr;

	if (ipr_set_dasd_timeouts(dev))
		return;
	if (runtime && !dev_init_allowed(dev))
		return;
	if (runtime && page0x20_setup(dev) && page0x0a_setup(dev))
		return;
	if (setup_page0x01(dev))
		return;
	if (setup_page0x0a(dev))
		return;
	if (ipr_setup_page0x20(dev))
		return;
	if (enable_af(dev))
		return;
	if (ipr_get_dev_attr(dev, &attr))
		return;

	attr.format_timeout = IPR_FORMAT_UNIT_TIMEOUT;

	if (ipr_modify_dev_attr(dev, &attr))
		return;
	if (ipr_set_dev_attr(dev, &attr, 0))
		return;
}

/*
 * IOA:
 * 1. Load saved adapter configuration
 */
static void init_ioa_dev(struct ipr_dev *dev)
{
	struct ipr_scsi_buses buses;

	if (ipr_get_bus_attr(dev->ioa, &buses))
		return;
	ipr_modify_bus_attr(dev->ioa, &buses);
	if (ipr_set_bus_attr(dev->ioa, &buses, 0))
		return;
}

void ipr_init_dev(struct ipr_dev *dev)
{
	if (!dev->scsi_dev_data)
		return;

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
	default:
		break;
	};
}

void ipr_init_ioa(struct ipr_ioa *ioa)
{
	int i = 0;

	if (ioa->ioa_dead)
		return;

	for (i = 0; i < ioa->num_devices; i++)
		ipr_init_dev(&ioa->dev[i]);

	init_ioa_dev(&ioa->ioa);
}

struct ipr_dev *get_dev_from_addr(struct ipr_res_addr *res_addr)
{
	struct ipr_ioa *cur_ioa;
	int j;
	struct scsi_dev_data *scsi_dev_data;

	for(cur_ioa = ipr_ioa_head; cur_ioa != NULL; cur_ioa = cur_ioa->next) {
		for (j = 0; j < cur_ioa->num_devices; j++) {
			scsi_dev_data = cur_ioa->dev[j].scsi_dev_data;

			if (scsi_dev_data == NULL)
				continue;

			if (scsi_dev_data->host == res_addr->host &&
			    scsi_dev_data->channel == res_addr->bus &&
			    scsi_dev_data->id == res_addr->target &&
			    scsi_dev_data->lun == res_addr->lun)
				return &cur_ioa->dev[j];
		}
	}

	return NULL;
}

struct ipr_dev *get_dev_from_handle(u32 res_handle)
{
	struct ipr_ioa *cur_ioa;
	int j;
	struct ipr_dev_record *device_record;
	struct ipr_array_record *array_record;

	for(cur_ioa = ipr_ioa_head; cur_ioa != NULL; cur_ioa = cur_ioa->next) {
		for (j = 0; j < cur_ioa->num_devices; j++) {
			device_record = (struct ipr_dev_record *)cur_ioa->dev[j].qac_entry;
			array_record = (struct ipr_array_record *)cur_ioa->dev[j].qac_entry;

			if (device_record == NULL)
				continue;

			if (device_record->common.record_id == IPR_RECORD_ID_DEVICE_RECORD &&
			    device_record->resource_handle == res_handle)
				return &cur_ioa->dev[j];

			if (array_record->common.record_id == IPR_RECORD_ID_ARRAY_RECORD &&
			    array_record->resource_handle == res_handle)
				return &cur_ioa->dev[j];

		}
	}

	return NULL;
}

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
	open("/dev/null",O_RDONLY);
	open("/dev/null",O_WRONLY);
	open("/dev/null",O_WRONLY);
	setsid();
}
