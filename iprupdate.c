/**
 * IBM IPR adapter microcode update utility
 *
 * (C) Copyright 2000, 2004
 * International Business Machines Corporation and others.
 * All Rights Reserved. This program and the accompanying
 * materials are made available under the terms of the
 * Common Public License v1.0 which accompanies this distribution.
 *
 */

/*
 * $Header: /cvsroot/iprdd/iprutils/iprupdate.c,v 1.15 2004/08/11 22:17:26 bjking1 Exp $
 */

#include <unistd.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <sys/stat.h>

#ifndef iprlib_h
#include "iprlib.h"
#endif

#include <sys/mman.h>

static int ioa_needs_update(struct ipr_ioa *ioa, int silent)
{
	struct sysfs_class_device *class_device;
	struct sysfs_attribute *attr;
	u32 fw_version;

	class_device = sysfs_open_class_device("scsi_host", ioa->host_name);
	attr = sysfs_get_classdev_attr(class_device, "fw_version");
	sscanf(attr->value, "%8X", &fw_version);
	sysfs_close_class_device(class_device);

	if (fw_version >= ioa->msl)
		return 0;

	if (!silent)
		ioa_info(ioa, "Adapter needs microcode update\n");
	return 1;
}

static int dev_needs_update(struct ipr_dev *dev)
{
	struct ipr_std_inq_data std_inq_data;
	struct ipr_dasd_inquiry_page3 page3_inq;
	struct unsupported_af_dasd *unsupp_af;

	memset(&std_inq_data, 0, sizeof(std_inq_data));
	if (ipr_inquiry(dev, IPR_STD_INQUIRY, &std_inq_data, sizeof(std_inq_data)))
		return 0;

	if (ipr_inquiry(dev, 3, &page3_inq, sizeof(page3_inq)))
		return 0;

	unsupp_af = get_unsupp_af(&std_inq_data, &page3_inq);
	if (!unsupp_af)
		return 0;

	if (!disk_needs_msl(unsupp_af, &std_inq_data))
		return 0;

	scsi_info(dev, "Device needs microcode update\n");
	return 1;
}

static void update_ioa_fw(struct ipr_ioa *ioa, int force)
{
	int rc;
	struct ipr_fw_images *list;

	if (!ioa_needs_update(ioa, 1))
		return;

	rc = get_ioa_firmware_image_list(ioa, &list);

	if (rc < 1) {
		syslog(LOG_ERR, "Could not find microcode file for IBM %04X. "
		       "Please download the latest microcode from "
		       "http://techsupport.services.ibm.com/server/mdownload/download.html. "
		       "SCSI speeds will be limited to %d MB/s until updated microcode is downloaded.\n",
		       ioa->ccin, IPR_SAFE_XFER_RATE);
		return;
	}

	ipr_update_ioa_fw(ioa, list, force);
	free(list);
}

static void update_disk_fw(struct ipr_dev *dev, int force)
{
	int rc;
	struct ipr_fw_images *list;

	rc = get_dasd_firmware_image_list(dev, &list);

	if (rc > 0) {
		ipr_update_disk_fw(dev, list, force);
		free(list);
	}

	return;
}

int main(int argc, char *argv[])
{
	int rc = 0;
	int i;
	int force_devs, force_ioas;
	int check_levels = 0;
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;

	openlog("iprupdate", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);

	force_devs = force_ioas = 0;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--force") == 0) {
			force_devs = force_ioas = 1;
		} else if (strcmp(argv[i], "--version") == 0) {
			printf("iprupdate: %s\n", IPR_VERSION_STR);
			exit(1);
		} else if (strcmp(argv[i], "--force-devs") == 0) {
			force_devs = 1;
		} else if (strcmp(argv[i], "--force-ioas") == 0) {
			force_ioas = 1;
		} else if (strcmp(argv[i], "--check_only") == 0) {
			check_levels = 1;
		} else if (strcmp(argv[i], "--debug") == 0) {
			ipr_debug = 1;
		} else {
			printf("Usage: iprupdate [options]\n");
			printf("  Options: --version    Print iprupdate version\n");
			exit(1);
		}
	}

	tool_init("iprupdate");

	check_current_config(false);

	for (ioa = ipr_ioa_head; ioa; ioa = ioa->next) {
		if (check_levels)
			rc |= ioa_needs_update(ioa, 0);
		else
			update_ioa_fw(ioa, force_ioas);

		for (i = 0; i < ioa->num_devices; i++) {
			dev = &ioa->dev[i];

			/* If not a DASD, ignore */
			if (!dev->scsi_dev_data ||
			    ipr_is_volume_set(dev) ||
			    (dev->scsi_dev_data->type != TYPE_DISK &&
			     dev->scsi_dev_data->type != IPR_TYPE_AF_DISK))
				continue;

			if (check_levels)
				rc |= dev_needs_update(dev);
			else
				update_disk_fw(dev, force_devs);
		}
	}

	return rc;
}
