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
 * $Header: /cvsroot/iprdd/iprutils/iprupdate.c,v 1.18 2005/02/23 20:57:12 bjking1 Exp $
 */

#include <unistd.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <sys/stat.h>

#ifndef iprlib_h
#include "iprlib.h"
#endif

#include <sys/mman.h>

char *tool_name = "iprupdate";

static int force_devs;
static int force_ioas;

static int ioa_needs_update(struct ipr_ioa *ioa, int silent)
{
	u32 fw_version = get_ioa_fw_version(ioa);

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
		ipr_log_ucode_error(ioa);
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

static int download_needed(struct ipr_ioa *ioa)
{
	struct ipr_dev *dev;
	int i, rc = 0;

	rc |= ioa_needs_update(ioa, 0);

	for (i = 0, dev = ioa->dev; i < ioa->num_devices; i++, dev++) {
		/* If not a DASD, ignore */
		if (!dev->scsi_dev_data ||
		    ipr_is_volume_set(dev) ||
		    (dev->scsi_dev_data->type != TYPE_DISK &&
		     dev->scsi_dev_data->type != IPR_TYPE_AF_DISK))
			continue;

		rc |= dev_needs_update(dev);
	}

	return rc;
}

static int any_download_needed()
{
	struct ipr_ioa *ioa;
	int rc = 0;

	for_each_ioa(ioa)
		rc |= download_needed(ioa);

	return rc;
}

static void update_ioa(struct ipr_ioa *ioa)
{
	struct ipr_dev *dev;
	int i;

	update_ioa_fw(ioa, force_ioas);

	for (i = 0; i < ioa->num_devices; i++) {
		dev = &ioa->dev[i];

		/* If not a DASD, ignore */
		if (!dev->scsi_dev_data ||
		    ipr_is_volume_set(dev) ||
		    (dev->scsi_dev_data->type != TYPE_DISK &&
		     dev->scsi_dev_data->type != IPR_TYPE_AF_DISK))
			continue;

		update_disk_fw(dev, force_devs);
	}
}

static void update_all()
{
	struct ipr_ioa *ioa;

	for_each_ioa(ioa)
		update_ioa(ioa);
}

static void kevent_handler(char *buf)
{
	struct ipr_ioa *ioa;
	char *c;
	int host;

	if (strncmp(buf, "change@/class/scsi_host", 23))
		return;

	c = strrchr(buf, '/');
	if (!c) {
		syslog_dbg("Failed to handle %s kevent\n", buf);
		return;
	}

	c += strlen("/host");
	host = strtoul(c, NULL, 10);

	tool_init();
	check_current_config(false);

	ioa = find_ioa(host);

	if (!ioa) {
		syslog(LOG_ERR, "Failed to find ipr ioa %d for iprdump\n", host);
		return;
	}

	update_ioa(ioa);
}

int main(int argc, char *argv[])
{
	int i;
	int check_levels = 0;

	openlog("iprupdate", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);

	for (i = 1; i < argc; i++) {
		if (parse_option(argv[i]))
			continue;
		else if (strcmp(argv[i], "--force-devs") == 0)
			force_devs = 1;
		else if (strcmp(argv[i], "--force-ioas") == 0)
			force_ioas = 1;
		else if (strcmp(argv[i], "--check_only") == 0)
			check_levels = 1;
		else {
			printf("Usage: iprupdate [options]\n");
			printf("  Options: --version    Print iprupdate version\n");
			exit(1);
		}
	}

	if (ipr_force)
		force_devs = force_ioas = 1;

	ipr_sg_required = 1;

	tool_init();

	check_current_config(false);

	if (check_levels)
		return any_download_needed();

	if (!daemonize) {
		update_all();
		return 0;
	}

	ipr_daemonize();

	return handle_events(update_all, 60, kevent_handler);
}
