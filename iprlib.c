/******************************************************************/
/* Linux IBM IPR utility library                                  */
/* Description: IBM Storage IOA Interface Specification (IPR)     */
/*              Linux utility	library                             */
/*                                                                */
/* (C) Copyright 2000, 2001                                       */
/* International Business Machines Corporation and others.        */
/* All Rights Reserved.                                           */
/******************************************************************/

/*
 * $Header: /cvsroot/iprdd/iprutils/iprlib.c,v 1.17 2004/02/18 16:28:04 bjking1 Exp $
 */

#ifndef iprlib_h
#include "iprlib.h"
#endif

struct ipr_array_query_data *ipr_qac_data = NULL;
struct scsi_dev_data *scsi_dev_table = NULL;
u32 num_ioas = 0;
struct ipr_ioa *ipr_ioa_head = NULL;
struct ipr_ioa *ipr_ioa_tail = NULL;

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

struct ses_table_entry {
	char product_id[17];
	char compare_product_id_byte[17];
	u32 max_bus_speed_limit;
};

static const struct ses_table_entry ses_table[] = {
	{"2104-DL1        ", "XXXXXXXXXXXXXXXX", 80},
	{"2104-TL1        ", "XXXXXXXXXXXXXXXX", 80},
	{"HSBP07M P U2SCSI", "XXXXXXXXXXXXXXXX", 80},
	{"HSBP05M P U2SCSI", "XXXXXXXXXXXXXXXX", 80},
	{"HSBP05M S U2SCSI", "XXXXXXXXXXXXXXXX", 80},
	{"HSBP06E ASU2SCSI", "XXXXXXXXXXXXXXXX", 80},
	{"2104-DU3        ", "XXXXXXXXXXXXXXXX", 160},
	{"2104-TU3        ", "XXXXXXXXXXXXXXXX", 160},
	{"HSBP04C RSU2SCSI", "XXXXXXX*XXXXXXXX", 160},
	{"HSBP06E RSU2SCSI", "XXXXXXX*XXXXXXXX", 160},
	{"St  V1S2        ", "XXXXXXXXXXXXXXXX", 160},
	{"HSBPD4M  PU3SCSI", "XXXXXXX*XXXXXXXX", 160},
	{"VSBPD1H   U3SCSI", "XXXXXXX*XXXXXXXX", 160}
};

static int get_max_bus_speed(struct ipr_ioa *ioa, int bus)
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
		return IPR_MAX_XFER_RATE;
	if (strncmp(dev->scsi_dev_data->vendor_id, "IBM", 3))
		return IPR_MAX_XFER_RATE;

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
			return ste->max_bus_speed_limit;
	}

	return IPR_MAX_XFER_RATE;
}

struct ioa_parms {
	int ccin;
	int scsi_id_changeable;
	u32 msl;
	char *fw_name;
};

static const struct ioa_parms ioa_parms [] = {
	{.ccin = 0x5702, .scsi_id_changeable = 1, .msl = 0, .fw_name = "44415254" },
	{.ccin = 0x5703, .scsi_id_changeable = 0, .msl = 0, .fw_name = "5052414D" },
	{.ccin = 0x5709, .scsi_id_changeable = 0, .msl = 0, .fw_name = "5052414E" },
	{.ccin = 0x570A, .scsi_id_changeable = 0, .msl = 0, .fw_name = "44415255" },
	{.ccin = 0x570B, .scsi_id_changeable = 0, .msl = 0, .fw_name = "44415255" },
	{.ccin = 0x2780, .scsi_id_changeable = 0, .msl = 0, .fw_name = "unknown" },
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

	ioa->scsi_id_changeable = 0;
}

void tool_init(char *tool_name)
{
	int ccin;
	struct ipr_ioa *ipr_ioa;
	struct sysfs_driver *sysfs_ipr_driver;
	struct dlist *ipr_devs;
	char *pci_address;

	struct sysfs_class *sysfs_host_class;
	struct sysfs_class *sysfs_device_class;
	struct dlist *class_devices;
	struct sysfs_class_device *class_device;
	struct sysfs_device *sysfs_device_device;
	struct sysfs_device *sysfs_host_device;
	struct sysfs_device *sysfs_pci_device;

	struct sysfs_attribute *sysfs_model_attr;

	/* Find all the IPR IOAs attached and save away vital info about them */
	sysfs_ipr_driver = sysfs_open_driver("ipr", "pci");
	if (sysfs_ipr_driver == NULL) {
		syslog(LOG_ERR,"Unable to located ipr driver data: %m\n");
		return;
	}

	ipr_devs = sysfs_get_driver_devices(sysfs_ipr_driver);

	if (ipr_devs != NULL) {
		dlist_for_each_data(ipr_devs, pci_address, char) {

			ipr_ioa = (struct ipr_ioa*)malloc(sizeof(struct ipr_ioa));
			memset(ipr_ioa,0,sizeof(struct ipr_ioa));

			/* PCI address */
			strcpy(ipr_ioa->pci_address, pci_address);

			/* driver version */
			strcpy(ipr_ioa->driver_version,"2");  /* FIXME pick up version from modinfo? */

			sysfs_host_class = sysfs_open_class("scsi_host");
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

int ipr_query_array_config(struct ipr_ioa *ioa,
			   bool allow_rebuild_refresh, bool set_array_id,
			   int array_id, void *buff)
{
	int fd;
	int length = sizeof(struct ipr_array_query_data);
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;
	int rc;

	if (strlen(ioa->ioa.gen_name) == 0)
		return -ENOENT;

	fd = open(ioa->ioa.gen_name, O_RDWR);
	if (fd <= 1) {
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
		dev_cmd_err(dev, &sense_data, "Query Command Status", rc);

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
		dev_cmd_err(dev, &sense_data, "Query Resource State", rc);

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

	if (rc != 0)
		dev_cmd_err(dev, &sense_data, "Mode Sense", rc);

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
		dev_cmd_err(dev, &sense_data, "Mode Select", rc);

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
		syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
		return errno;
	}

	arg = SG_SCSI_RESET_DEVICE;
	rc = ioctl(fd, SG_SCSI_RESET, &arg);

	if (rc != 0)
		dev_err(dev, "Reset Device failed. %m\n");

	close(fd);
	return rc;
}

int ipr_re_read_partition(struct ipr_dev *dev)
{
	int fd, rc;

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
		syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
		return errno;
	}

	rc = ioctl(fd, BLKRRPART, NULL);

	if (rc != 0)
		dev_err(dev, "Re-read partition table failed. %m\n");

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
		syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = READ_CAPACITY;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0)
		dev_cmd_err(dev, &sense_data, "Read Capacity", rc);

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
		dev_cmd_err(dev, &sense_data, "Read Capacity", rc);

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
		syslog(LOG_ERR, "Could not open %s. %m\n",file);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = IPR_RECLAIM_CACHE_STORE;
	cdb[1] = (u8)action;

	rc = sg_ioctl(fd, cdb, buff,
		      length, SG_DXFER_FROM_DEV,
		      &sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0) {
		if (errno != EINVAL) {
			syslog(LOG_ERR, "Reclaim Cache Store to %s failed. %m\n", file);
		}
	}
	else if ((action & IPR_RECLAIM_PERFORM) == IPR_RECLAIM_PERFORM)
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
		dev_cmd_err(dev, &sense_data, "Start Unit", rc);

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
		dev_cmd_err(dev, &sense_data, "Stop Unit", rc);

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
		syslog(LOG_ERR, "Could not open %s. %m\n", dev->gen_name);
		return errno;
	}

	memset(cdb, 0, IPR_CCB_CDB_LEN);

	cdb[0] = TEST_UNIT_READY;

	rc = sg_ioctl(fd, cdb, NULL,
		      length, SG_DXFER_FROM_DEV,
		      sense_data, IPR_INTERNAL_DEV_TIMEOUT);

	if (rc != 0)
		dev_cmd_err(dev, sense_data, "Test Unit Ready", rc);

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
		      &sense_data, IPR_FORMAT_UNIT_TIMEOUT);

	free(defect_list_hdr);

	if (rc != 0)
		dev_cmd_err(dev, &sense_data, "Format Unit", rc);

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

	if (rc != 0)
		dev_cmd_err(dev, &sense_data, "Evaluate Device Capabilities", rc);

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
		dev_cmd_err(dev, &sense_data, "Inquiry", rc);

	close(fd);
	return rc;
}

int ipr_write_buffer(struct ipr_dev *dev, void *buff, int length)
{
	int fd, rc;
	u8 cdb[IPR_CCB_CDB_LEN];
	struct sense_data_t sense_data;

	if (strlen(dev->gen_name) == 0)
		return -ENOENT;

	fd = open(dev->gen_name, O_RDWR);
	if (fd <= 1) {
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
		dev_cmd_err(dev, &sense_data, "Write buffer", rc);

	close(fd);
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
		dev_cmd_err(dev, &sense_data, "Receive diagnostics", rc);

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
		dev_cmd_err(dev, &sense_data, "Send diagnostics", rc);

	close(fd);
	return rc;
}

#define IPR_MAX_XFER 0x8000
const int cdb_size[] ={6, 10, 10, 0, 16, 12, 16, 16};
int sg_ioctl(int fd, u8 cdb[IPR_CCB_CDB_LEN],
	     void *data, u32 xfer_len, u32 data_direction,
	     struct sense_data_t *sense_data,
	     u32 timeout_in_sec)
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
			iovec[i].iov_base = malloc(IPR_MAX_XFER);
			memcpy(iovec[i].iov_base, data + IPR_MAX_XFER * i, IPR_MAX_XFER);
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

	for (i = 0; i < 2; i++) {
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

		if ((rc == 0) && (io_hdr_t.masked_status == CHECK_CONDITION))
			rc = CHECK_CONDITION;

		if ((rc != CHECK_CONDITION) || (sense_data->sense_key != UNIT_ATTENTION))
			break;
	}

	if (iovec_count) {
		for (i = 0; i < iovec_count; i++)
			free(iovec[i].iov_base);
		free(iovec);
	}

	return rc;
};

#define MAX_SG_DEVS 255  //FIXME!!!
#define MAX_SD_DEVS 255
static void make_dev_name(char * fname, const char * leadin, int k, 
			  int do_numeric)
{
	char buff[64];
	int  big,little;

	strcpy(fname, leadin ? leadin : "/dev/sg");
	if (do_numeric) {
		sprintf(buff, "%d", k);
		strcat(fname, buff);
	}
	else {
		if (k < 26) {
			buff[0] = 'a' + (char)k;
			buff[1] = '\0';
			strcat(fname, buff);
		}
		else if (k <= 255) { /* assumes sequence goes x,y,z,aa,ab,ac etc */
			big    = k/26;
			little = k - (26 * big);
			big    = big - 1;

			buff[0] = 'a' + (char)big;
			buff[1] = 'a' + (char)little;
			buff[2] = '\0';
			strcat(fname, buff);
		}
		else
			strcat(fname, "xxxx");
	}
}

/* returns 0 on success, 1 on failure  to find major and minor
 number for the given device name */
int get_sd_proc_data(char *sd_name, int *major_num, int *minor_num)
{
	FILE *proc_file;
	char proc_file_name[] = "/proc/partitions";
	int rc;
	int local_major_num, local_minor_num;
	char line[100];
	char old_line[100];
	int lines_different;

	proc_file = fopen(proc_file_name, "r");

	if (proc_file == NULL)
		return 1;
	do
	{
		strcpy(old_line,line);
		fgets(line, 100, proc_file);
		lines_different = strcmp(line,old_line);
		if (strstr(line, sd_name))
		{
			sscanf(line,"%d %d",
			       &local_major_num,
			       &local_minor_num);
			*major_num = local_major_num;
			*minor_num = local_minor_num;
			fclose(proc_file);
			return 0;
		}
	}while ((rc != EOF) && (lines_different));

	fclose(proc_file);
	return 1;
}

void get_sg_ioctl_data(struct sg_map_info *sg_map_info, int num_devs)
{
	int sg_index, sd_index, fd, rc;
	int num_sg_devices = 0;
	struct sg_map_info *temp;
	char tmp_dev_name[64];
	struct
	{
		u32 dev_id;
		u32 unique_id;
	} dev_idlun;
	int sd_major, sd_minor;
	char *sd_name;
	struct stat sda_stat;
	int max_sd_devs = MAX_SD_DEVS;
	int temp_max_sd_devs;
	int used_sd[MAX_SD_DEVS];

	memset(used_sd, 0, sizeof(used_sd));

	for (sg_index = 0;
	     (sg_index < MAX_SG_DEVS) && (num_sg_devices < num_devs);
	     sg_index++)
	{
		temp = &sg_map_info[num_sg_devices];

		memset(sg_map_info[num_sg_devices].gen_name, 0, sizeof(sg_map_info[num_sg_devices].gen_name));
		make_dev_name(sg_map_info[num_sg_devices].gen_name, "/dev/sg", sg_index, 1);
		fd = open(sg_map_info[num_sg_devices].gen_name, O_RDONLY | O_NONBLOCK);

		if (fd < 0)
		{
			if (errno == ENOENT)
			{
				mknod(sg_map_info[num_sg_devices].gen_name,
				      S_IFCHR|0660,
				      makedev(21,sg_index));

				/* change permissions and owner to be identical to
				 /dev/sg0's */
				stat("/dev/sg0", &sda_stat);
				chown(sg_map_info[num_sg_devices].gen_name,
				      sda_stat.st_uid,
				      sda_stat.st_gid);
				chmod(sg_map_info[num_sg_devices].gen_name,
				      sda_stat.st_mode);

				fd = open(sg_map_info[num_sg_devices].gen_name, O_RDONLY | O_NONBLOCK);
				if (fd < 0)
					continue;
			}
			else
				continue;
		}

		rc = ioctl(fd, SG_GET_SCSI_ID, &sg_map_info[num_sg_devices].sg_dat);

		close(fd);
		if ((rc >= 0) &&
		    (sg_map_info[num_sg_devices].sg_dat.scsi_type == TYPE_DISK))
		{
			temp_max_sd_devs = 0;

			memset(sg_map_info[num_sg_devices].dev_name, 0, sizeof(sg_map_info[num_sg_devices].dev_name));
			for (sd_index = 0; sd_index < max_sd_devs; sd_index++)
			{
				if (used_sd[sd_index])
					continue;

				memset(tmp_dev_name, 0, sizeof(tmp_dev_name));
				make_dev_name(tmp_dev_name, "/dev/sd", sd_index, 0);

				fd = open(tmp_dev_name, O_RDONLY | O_NONBLOCK);
				if (fd < 0)
				{
					if (errno == ENOENT)
					{
						/* find major and minor number from /proc/partitions */
						sd_name = strstr(tmp_dev_name, "sd");
						rc = get_sd_proc_data(sd_name, &sd_major, &sd_minor);

						if (rc)
							continue;

						mknod(tmp_dev_name,
						      S_IFBLK|0660,
						      makedev(sd_major,sd_minor));

						/* change permissions and owner to be identical to
						 /dev/sda's */
						stat("/dev/sda", &sda_stat);
						chown(tmp_dev_name,
						      sda_stat.st_uid,
						      sda_stat.st_gid);
						chmod(tmp_dev_name,
						      sda_stat.st_mode);

						fd = open(tmp_dev_name, O_RDONLY | O_NONBLOCK);
						if (fd < 0)
							continue;
					}
					else
						continue;
				}
				temp_max_sd_devs = sd_index + 1;

				rc = ioctl(fd, SCSI_IOCTL_GET_IDLUN, &dev_idlun);
				close(fd);
				if (rc < 0)
				{
					continue;
				}

				if ((sg_map_info[num_sg_devices].sg_dat.host_no == ((dev_idlun.dev_id >> 24) & 0xFF)) &&
				    (sg_map_info[num_sg_devices].sg_dat.channel == ((dev_idlun.dev_id >> 16) & 0xFF)) &&
				    (sg_map_info[num_sg_devices].sg_dat.scsi_id == (dev_idlun.dev_id & 0xFF)) &&
				    (sg_map_info[num_sg_devices].sg_dat.lun     == ((dev_idlun.dev_id >>  8) & 0xFF)))
				{
					strcpy(sg_map_info[num_sg_devices].dev_name, tmp_dev_name);
					temp_max_sd_devs = max_sd_devs;
					used_sd[sd_index] = 1;
					break;
				}
			}
			max_sd_devs = temp_max_sd_devs;
			num_sg_devices++;
		} else if (rc >= 0) {
			strcpy(sg_map_info[num_sg_devices].dev_name, "");
			num_sg_devices++;
		}
	}
	return;
}

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
	class_devices = sysfs_get_class_devices(sysfs_device_class);
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
		if (sysfs_attr)
			sscanf(sysfs_attr->value, "%d", &scsi_dev_data->online);

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

		num_devs++;
	}
	sysfs_close_class(sysfs_device_class);

	*scsi_dev_ref = scsi_dev_base;
	return num_devs;
}

void get_ioa_name(struct ipr_ioa *cur_ioa,
		  struct sg_map_info *sg_map_info,
		  int num_sg_devices)
{
	int i;
	int host_no;

	sscanf(cur_ioa->host_name, "host%d", &host_no);

	for (i = 0; i < num_sg_devices; i++, sg_map_info++) {
		if ((sg_map_info->sg_dat.host_no == host_no)   &&
		    (sg_map_info->sg_dat.channel == 255) &&
		    (sg_map_info->sg_dat.scsi_id == 255) &&
		    (sg_map_info->sg_dat.lun     == 255) &&
		    (sg_map_info->sg_dat.scsi_type == IPR_TYPE_ADAPTER)) {

			strcpy(cur_ioa->ioa.dev_name, sg_map_info->dev_name);
			strcpy(cur_ioa->ioa.gen_name, sg_map_info->gen_name);
		}
	}
}

void check_current_config(bool allow_rebuild_refresh)
{
	struct sg_map_info *sg_map_info;
	struct scsi_dev_data *scsi_dev_data;
	int  num_sg_devices, rc, device_count, j, k;
	struct ipr_ioa *ioa;
	struct ipr_array_query_data *qac_data;
	struct ipr_common_record *common_record;
	struct ipr_dev_record *device_record;
	struct ipr_array_record *array_record;
	int *qac_entry_ref;

	if (ipr_qac_data == NULL) {
		ipr_qac_data =
			(struct ipr_array_query_data *)
			malloc(sizeof(struct ipr_array_query_data) * num_ioas);
	}

	/* Get sg data via sg proc file system */
	num_sg_devices = get_scsi_dev_data(&scsi_dev_table);

	sg_map_info = (struct sg_map_info *)malloc(sizeof(struct sg_map_info) * num_sg_devices);
	get_sg_ioctl_data(sg_map_info, num_sg_devices);

	for(ioa = ipr_ioa_head, qac_data = ipr_qac_data;
	    ioa; ioa = ioa->next, qac_data++) {
		get_ioa_name(ioa, sg_map_info, num_sg_devices);

		ioa->num_devices = 0;

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
		     j < num_sg_devices; j++, scsi_dev_data++)
		{
			if (scsi_dev_data->host != ioa->host_num)
				continue;

			if ((scsi_dev_data->type == TYPE_DISK) ||
			    (scsi_dev_data->type == IPR_TYPE_AF_DISK) ||
			    (scsi_dev_data->type == IPR_TYPE_SES))
			{
				ioa->dev[device_count].ioa = ioa;
				ioa->dev[device_count].scsi_dev_data = scsi_dev_data;
				ioa->dev[device_count].qac_entry = NULL;
				strcpy(ioa->dev[device_count].dev_name, "");
				strcpy(ioa->dev[device_count].gen_name, "");

				/* find array config data matching resource entry */
				common_record = (struct ipr_common_record *)qac_data->data;

				for (k = 0; k < qac_data->num_records; k++)
				{
					if (common_record->record_id == IPR_RECORD_ID_DEVICE_RECORD) {
						device_record = (struct ipr_dev_record *)common_record;
						if ((device_record->resource_addr.bus    == scsi_dev_data->channel) &&
						    (device_record->resource_addr.target == scsi_dev_data->id) &&
						    (device_record->resource_addr.lun    == scsi_dev_data->lun))
						{
							ioa->dev[device_count].qac_entry = common_record;
							qac_entry_ref[k]++;
							break;
						}
					} else if (common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD) {
						array_record = (struct ipr_array_record *)common_record;
						if ((array_record->resource_addr.bus    == scsi_dev_data->channel) &&
						    (array_record->resource_addr.target == scsi_dev_data->id) &&
						    (array_record->resource_addr.lun    == scsi_dev_data->lun))
						{
							ioa->dev[device_count].qac_entry = common_record;
							qac_entry_ref[k]++;
							break;
						}
					}                    

					common_record = (struct ipr_common_record *)
						((unsigned long)common_record + ntohs(common_record->record_len));
				}

				/* find sg_map_info matching resource entry */
				for (k = 0; k < num_sg_devices; k++)
				{
					if ((scsi_dev_data->host    == sg_map_info[k].sg_dat.host_no) &&
					    (scsi_dev_data->channel == sg_map_info[k].sg_dat.channel) &&
					    (scsi_dev_data->id      == sg_map_info[k].sg_dat.scsi_id) &&
					    (scsi_dev_data->lun     == sg_map_info[k].sg_dat.lun))
					{
						/* copy device name to main structure */
						strcpy(ioa->dev[device_count].dev_name, sg_map_info[k].dev_name);
						strcpy(ioa->dev[device_count].gen_name, sg_map_info[k].gen_name);

						break;
					}
				}

				device_count++;
			}
			else if (scsi_dev_data->type == IPR_TYPE_ADAPTER)
			{
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
			} else if ((!qac_entry_ref[k]) &&
				   ((common_record->record_id == IPR_RECORD_ID_DEVICE_RECORD) ||
				    (common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD))) {

				array_record = (struct ipr_array_record *)common_record;
				if ((common_record->record_id == IPR_RECORD_ID_ARRAY_RECORD)
				    && (array_record->start_cand))

					ioa->start_array_qac_entry = array_record;
				else {

					/* add phantom qac entry to ioa device list */
					ioa->dev[device_count].scsi_dev_data = NULL;
					ioa->dev[device_count].qac_entry = common_record;

					strcpy(ioa->dev[device_count].dev_name, "");
					strcpy(ioa->dev[device_count].gen_name, "");
					device_count++;
				}
			}

			common_record = (struct ipr_common_record *)
				((unsigned long)common_record + ntohs(common_record->record_len));
		}

		ioa->num_devices = device_count;
	}
	free(sg_map_info);
}

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

	va_start(args, s);
	vsprintf(usr_str, s, args);
	va_end(args);

	closelog();
	openlog("iprconfig", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER); /* FIXME xxx */
	syslog(LOG_ERR,"%s",usr_str);
	closelog();
	openlog("iprconfig", LOG_PID | LOG_CONS, LOG_USER);

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

static void ipr_save_bus_attr(struct ipr_ioa *ioa, int bus,
			      char *field, char *value, int update)
{
	char category[16], fname[64];
	FILE *fd, *temp_fd;
	char temp_fname[64], line[64];
	int bus_found = 0;
	int field_found = 0;

	sprintf(fname, "%s%x_%s", IPR_CONFIG_DIR, ioa->ccin, ioa->pci_address);
	ipr_config_file_hdr(fname);

	sprintf(category,"[%s %d]", IPR_CATEGORY_BUS, bus);

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

static int ipr_get_saved_bus_attr(struct ipr_ioa *ioa, int bus,
				  char *field, char *value)
{
	FILE *fd;
	char fname[64], line[64], category[16], *str_ptr;
	int bus_found;

	sprintf(category,"[%s %d]", IPR_CATEGORY_BUS, bus);
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

int ipr_get_bus_attr(struct ipr_ioa *ioa, struct ipr_scsi_buses *sbus)
{
	struct ipr_mode_pages mode_pages;
	struct ipr_mode_page_28 *page_28;
	struct ipr_mode_page_28_scsi_dev_bus_attr *bus;
	int rc, i, busno;

	memset(&mode_pages, 0, sizeof(mode_pages));
	memset(sbus, 0, sizeof(*sbus));

	rc = ipr_mode_sense(&ioa->ioa, 0x28, &mode_pages);

	if (!rc)
		return rc;

	page_28 = (struct ipr_mode_page_28 *)(mode_pages.data + mode_pages.hdr.block_desc_len);

	for_each_bus_attr(bus, page_28, i) {
		busno = bus->res_addr.bus;
		sbus->bus[busno].max_xfer_rate = bus->max_xfer_rate;
		sbus->bus[busno].qas_capability = bus->qas_capability;
		sbus->bus[busno].scsi_id = bus->scsi_id;
		sbus->bus[busno].bus_width = bus->bus_width;
		sbus->bus[busno].extended_reset_delay = bus->extended_reset_delay;
		sbus->bus[busno].min_time_delay = bus->min_time_delay;
		sbus->num_buses++;
	}

	return 0;
}

int ipr_set_bus_attr(struct ipr_ioa *ioa, struct ipr_scsi_buses *sbus)
{
	struct ipr_mode_pages mode_pages;
	struct ipr_mode_page_28 *page_28;
	struct ipr_mode_page_28_scsi_dev_bus_attr *bus;
	int rc, i, busno, len;
	int reset_needed = 0;

	memset(&mode_pages, 0, sizeof(mode_pages));
	memset(sbus, 0, sizeof(*sbus));

	rc = ipr_mode_sense(&ioa->ioa, 0x28, &mode_pages);

	if (!rc)
		return rc;

	page_28 = (struct ipr_mode_page_28 *)
		(mode_pages.data + mode_pages.hdr.block_desc_len);

	for_each_bus_attr(bus, page_28, i) {
		busno = bus->res_addr.bus;
		bus->max_xfer_rate = sbus->bus[busno].max_xfer_rate;
		bus->qas_capability = sbus->bus[busno].qas_capability;
		if (bus->scsi_id != sbus->bus[busno].scsi_id)
			reset_needed = 1;
		bus->scsi_id = sbus->bus[busno].scsi_id;
		bus->bus_width = sbus->bus[busno].bus_width;
		bus->extended_reset_delay = sbus->bus[busno].extended_reset_delay;
		bus->min_time_delay = sbus->bus[busno].min_time_delay;
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
					htonl(saved_value * 10 / (sbus->bus[i].bus_width / 8));
			} else {
				sbus->bus[i].max_xfer_rate =
					htonl(max_xfer_rate * 10 / (sbus->bus[i].bus_width / 8));
				sprintf(value_str, "%d", max_xfer_rate);
				ipr_save_bus_attr(ioa, i, IPR_MAX_XFER_RATE_STR, value_str, 1);
			}
		} else {
			sbus->bus[i].max_xfer_rate =
				htonl(max_xfer_rate * 10 / (sbus->bus[i].bus_width / 8));
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

bool is_af_blocked(struct ipr_dev *ipr_dev, int silent)
{
	int i, j, rc;
	struct ipr_std_inq_data std_inq_data;
	struct ipr_dasd_inquiry_page3 dasd_page3_inq;
	u32 ros_rcv_ram_rsvd, min_ucode_level;

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

	for (i=0; i < ARRAY_SIZE(unsupported_af); i++) {
		for (j = 0; j < IPR_VENDOR_ID_LEN; j++) {
			if (unsupported_af[i].compare_vendor_id_byte[j] &&
			    unsupported_af[i].vendor_id[j] != std_inq_data.vpids.vendor_id[j])
				break;
		}

		if (j != IPR_VENDOR_ID_LEN)
			continue;

		for (j = 0; j < IPR_PROD_ID_LEN; j++) {
			if (unsupported_af[i].compare_product_id_byte[j] &&
			    unsupported_af[i].product_id[j] != std_inq_data.vpids.product_id[j])
				break;
		}

		if (j != IPR_PROD_ID_LEN)
			continue;

		for (j = 0; j < 4; j++) {
			if (unsupported_af[i].lid_mask[j] &&
			    unsupported_af[i].lid[j] != dasd_page3_inq.load_id[j])
				break;
		}

		if (j != 4)
			continue;

		/* If we make it this far, we have a match into the table.  Now,
		 determine if we need a certain level of microcode or if this
		 disk is not supported all together. */
		if (unsupported_af[i].supported_with_min_ucode_level) {
			/* Check microcode level in std inquiry data */
			/* If it's less than the minimum required level, we will
			 tell the user to upgrade microcode. */
			min_ucode_level = 0;
			ros_rcv_ram_rsvd = 0;

			for (j = 0; j < 4; j++) {
				if (unsupported_af[i].min_ucode_mask[j]) {
					min_ucode_level = (min_ucode_level << 8) |
						unsupported_af[i].min_ucode_level[j];
					ros_rcv_ram_rsvd = (ros_rcv_ram_rsvd << 8) |
						std_inq_data.ros_rsvd_ram_rsvd[j];
				}
			}

			if (min_ucode_level > ros_rcv_ram_rsvd) {
				if (!silent)
					syslog(LOG_ERR,"Disk %s needs updated microcode "
					       "before transitioning to 522 bytes/sector "
					       "format.", ipr_dev->gen_name);
				return true;
			} else
				return false;
		} else {/* disk is not supported at all */
			if (!silent)
				syslog(LOG_ERR,"Disk %s canot be formatted to "
				       "522 bytes/sector.", ipr_dev->gen_name);
			return true;
		}
	}   
	return false;
}

int ipr_write_dev_attr(struct ipr_dev *dev, char *attr, char *value)
{
	struct sysfs_class_device *class_device;
	struct sysfs_attribute *dev_attr;
	struct sysfs_device *device;
	char *sysfs_dev_name = dev->scsi_dev_data->sysfs_device_name;

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

static int select_dasd_ucode_file(const struct dirent *p_dir_entry)
{
	return 1;
}

static u32 get_dasd_ucode_version(char *ucode_file, int ftype)
{
	int fd;
	struct stat ucode_stats;
	struct ipr_dasd_ucode_header *dasd_image_hdr;
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

		dasd_image_hdr = mmap(NULL, ucode_stats.st_size,
				      PROT_READ, MAP_SHARED, fd, 0);

		if (dasd_image_hdr == MAP_FAILED) {
			fprintf(stderr, "mmap of %s failed\n", ucode_file);
			close(fd);
			return 0;
		}

		rc = (dasd_image_hdr->modification_level[0] << 24) |
			(dasd_image_hdr->modification_level[1] << 16) |
			(dasd_image_hdr->modification_level[2] << 8) |
			dasd_image_hdr->modification_level[3];

		munmap(dasd_image_hdr, ucode_stats.st_size);
		close(fd);
	} else {
		tmp = strrchr(ucode_file, '.');

		if (!tmp)
			return 0;

		rc = strtoul(tmp+1, NULL, 16);
	}

	return rc;
}

int find_dasd_firmware_image(struct ipr_dev *dev,
			     char *prefix, char *fname, char *version)
{
	char etc_ucode_file[100];
	char usr_ucode_file[100];
	struct dirent **pp_dirent;
	u32 etc_version = 0;
	u32 usr_version = 0;
	int i, rc;

	rc = scandir("/usr/lib/microcode", &pp_dirent,
		     select_dasd_ucode_file, alphasort);

	if (rc > 0) {
		rc--;

		for (i = rc ; i >= 0; i--) {
			if (strstr(pp_dirent[i]->d_name, prefix) == pp_dirent[i]->d_name) {
				sprintf(usr_ucode_file, "/usr/lib/microcode/%s",
					pp_dirent[i]->d_name);
				break;
			}
		}

		free(*pp_dirent);
	}

	sprintf(etc_ucode_file, "/etc/microcode/device/%s", prefix);

	etc_version = get_dasd_ucode_version(etc_ucode_file, IPR_DASD_UCODE_ETC);
	usr_version = get_dasd_ucode_version(usr_ucode_file, IPR_DASD_UCODE_USRLIB);

	if (etc_version > usr_version) {
		sprintf(fname, etc_ucode_file);
		memcpy(version, &etc_version, sizeof(u32));
		return IPR_DASD_UCODE_HDR;
	} else if (usr_version == 0) {
		return IPR_DASD_UCODE_NOT_FOUND;
	} else {
		sprintf(fname, usr_ucode_file);
		memcpy(version, &usr_version, sizeof(u32));
		return IPR_DASD_UCODE_NO_HDR;
	}

	return -1;
}

static u32 get_ioa_ucode_version(char *ucode_file)
{
	int fd, rc;
	struct stat ucode_stats;
	struct ipr_ioa_ucode_header *image_hdr;

	fd = open(ucode_file, O_RDONLY);

	if (fd == -1)
		return 0;

	/* Get the size of the microcode image */
	rc = fstat(fd, &ucode_stats);

	if (rc != 0) {
		close(fd);
		return 0;
	}

	/* Map the image in memory */
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

static int select_ioa_ucode_file(const struct dirent *p_dir_entry)
{
	return 1;
}

int find_ioa_firmware_image(struct ipr_ioa *ioa, char *fname)
{
	char etc_ucode_file[100];
	char usr_ucode_file[100];
	const struct ioa_parms *parms = NULL;
	struct dirent **dirent;
	u32 etc_version = 0;
	u32 usr_version = 0;
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(ioa_parms); i++) {
		if (ioa_parms[i].ccin == ioa->ccin) {
			parms = &ioa_parms[i];
			break;
		}
	}

	if (parms) {
		rc = scandir("/usr/lib/microcode", &dirent,
			     select_ioa_ucode_file, alphasort);

		if (rc > 0) {
			rc--;

			for (i = rc ; i >= 0; i--) {
				if (strstr(dirent[i]->d_name, parms->fw_name) == dirent[i]->d_name) {
					sprintf(usr_ucode_file, "/usr/lib/microcode/%s", dirent[i]->d_name);
					break;
				}
			}

			free(*dirent);
		}
	}

	sprintf(etc_ucode_file, "/etc/microcode/ibmsis%X.img", ioa->ccin);

	etc_version = get_ioa_ucode_version(etc_ucode_file);
	usr_version = get_ioa_ucode_version(usr_ucode_file);

	if (etc_version > usr_version) {
		sprintf(fname, etc_ucode_file);
		return 0;
	} else if (usr_version == 0) {
		return -1;
	} else {
		sprintf(fname, usr_ucode_file);
		return 0;
	}

	return -1;
}
