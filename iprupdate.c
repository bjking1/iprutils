/**
 * IBM IPR adapter microcode update utility
 *
 * (C) Copyright 2000, 2004
 * International Business Machines Corporation and others.
 * All Rights Reserved.
 *
 */

/*
 * $Header: /cvsroot/iprdd/iprutils/iprupdate.c,v 1.10 2004/02/27 20:50:33 bjking1 Exp $
 */

#include <unistd.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <sys/stat.h>

#ifndef iprlib_h
#include "iprlib.h"
#endif

#include <sys/mman.h>

static void update_ioa_fw(struct ipr_ioa *ioa, int force)
{
	int rc;
	struct ipr_fw_images *list;

	rc = get_ioa_firmware_image_list(ioa, &list);

	if (rc < 1) {
		syslog(LOG_ERR, "Could not find firmware file for IBM %04X.\n", ioa->ccin);
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
	struct ipr_ioa *ioa;
	struct ipr_dev *dev;

	openlog("iprupdate", LOG_PERROR | LOG_PID | LOG_CONS, LOG_USER);

	force_devs = force_ioas = 0;

	if (argc > 1) {
		if (strcmp(argv[1], "--force") == 0)
			force_devs = force_ioas = 1;
		else if (strcmp(argv[1], "--version") == 0) {
			printf("iprupdate: %s\n", IPR_VERSION_STR);
			exit(1);
		} else if (strcmp(argv[1], "--force-devs") == 0) {
			force_devs = 1;
		} else if (strcmp(argv[1], "--force-ioas") == 0) {
			force_ioas = 1;
		} else {
			printf("Usage: iprdate [options]\n");
			printf("  Options: --version    Print iprupdate version\n");
			exit(1);
		}
	}

	tool_init("iprupdate");

	check_current_config(false);

	for (ioa = ipr_ioa_head; ioa; ioa = ioa->next) {
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

	return rc;
}
