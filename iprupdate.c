/******************************************************************/
/* Linux IBM IPR microcode update utility                         */
/* Description: IBM Storage IOA Interface Specification (IPR)     */
/*              Linux microcode update utility                    */
/*                                                                */
/* (C) Copyright 2000, 2001                                       */
/* International Business Machines Corporation and others.        */
/* All Rights Reserved.                                           */
/******************************************************************/

/*
 * $Header: /cvsroot/iprdd/iprutils/iprupdate.c,v 1.6 2004/02/18 16:28:04 bjking1 Exp $
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
	int fd, rc;
	struct sysfs_class_device *class_device;
	struct sysfs_attribute *attr;
	u32 fw_version;
	struct ipr_ioa_ucode_header *image_hdr;
	char ucode_path[100], ucode_file[100], *tmp;
	struct stat ucode_stats;

	class_device = sysfs_open_class_device("scsi_host", ioa->host_name);
	attr = sysfs_get_classdev_attr(class_device, "fw_version");
	sscanf(attr->value, "%8X", &fw_version);
	sysfs_close_class_device(class_device);

	if (fw_version > ioa->msl && !force)
		return;

	rc = find_ioa_firmware_image(ioa, ucode_path);

	if (rc) {
		syslog(LOG_ERR, "Could not find firmware file for IBM %04X.\n", ioa->ccin);
		return;
	}

	fd = open(ucode_path, O_RDONLY);

	if (fd < 0) {
		syslog(LOG_ERR, "Could not open firmware file %s.\n", ucode_path);
		return;
	}

	rc = fstat(fd, &ucode_stats);

	if (rc != 0) {
		syslog(LOG_ERR, "Error accessing IOA firmware file: %s.\n", ucode_path);
		close(fd);
		return;
	}

	image_hdr = mmap(NULL, ucode_stats.st_size, PROT_READ, MAP_SHARED, fd, 0);

	if (image_hdr == MAP_FAILED) {
		syslog(LOG_ERR, "Error mapping IOA firmware file: %s.\n", ucode_path);
		close(fd);
		return;
	}

	if (ntohl(image_hdr->rev_level) > fw_version || force) {
		ioa_info(ioa, "Updating adapter microcode from %08X to %08X.\n",
			 fw_version, ntohl(image_hdr->rev_level));

		tmp = strrchr(ucode_path, '/');
		tmp++;
		sprintf(ucode_file, "%s\n", tmp);
		class_device = sysfs_open_class_device("scsi_host", ioa->host_name);
		attr = sysfs_get_classdev_attr(class_device, "update_fw");
		rc = sysfs_write_attribute(attr, ucode_file, strlen(ucode_file));
		sysfs_close_class_device(class_device);

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
}

static void update_disk_fw(struct ipr_dev *dev, int force)
{
	int rc = 0;
	struct stat ucode_stats;
	char ucode_file[50];
	int fd, image_size, image_offset;
	struct ipr_dasd_ucode_header *dasd_image_hdr;
	struct ipr_dasd_inquiry_page3 dasd_page3_inq;
	void *buffer;
	char prefix[100], modification_level[5];

	if (memcmp(dev->scsi_dev_data->vendor_id, "IBMAS400", 8) == 0) {
		sprintf(prefix, "ibmsis%02X%02X%02X%02X.img",
			dasd_page3_inq.load_id[0], dasd_page3_inq.load_id[1],
			dasd_page3_inq.load_id[2],
			dasd_page3_inq.load_id[3]);
		image_offset = 0;
	} else if (memcmp(dev->scsi_dev_data->vendor_id, "IBM     ", 8) == 0) {
		sprintf(prefix, "%.7s.%02X%02X%02X%02X",
			dev->scsi_dev_data->product_id,
			dasd_page3_inq.load_id[0], dasd_page3_inq.load_id[1],
			dasd_page3_inq.load_id[2],
			dasd_page3_inq.load_id[3]);
		image_offset = sizeof(struct ipr_dasd_ucode_header);
	} else
		return;

	rc = ipr_inquiry(dev, 3, &dasd_page3_inq, sizeof(dasd_page3_inq));

	if (rc != 0) {
		dev_err(dev, "Inquiry failed\n");
		return;
	}

	rc = find_dasd_firmware_image(dev, prefix, ucode_file, modification_level);

	if (rc == IPR_DASD_UCODE_NOT_FOUND) {
		syslog_dbg(LOG_ERR, "Could not find firmware file %s.\n", prefix);
		return;
	} else if (rc == IPR_DASD_UCODE_NO_HDR) {
		image_offset = 0;
	}

	fd = open(ucode_file, O_RDONLY);

	if (fd < 0) {
		syslog_dbg(LOG_ERR, "Could not open firmware file %s.\n", ucode_file);
		return;
	}

	rc = fstat(fd, &ucode_stats);

	if (rc != 0) {
		syslog(LOG_ERR, "Error accessing firmware file: %s.\n", ucode_file);
		close(fd);
		return;
	}

	dasd_image_hdr = mmap(NULL, ucode_stats.st_size,
			      PROT_READ, MAP_SHARED, fd, 0);

	if (dasd_image_hdr == MAP_FAILED) {
		syslog(LOG_ERR, "Error reading firmware file: %s.\n", ucode_file);
		close(fd);
		return;
	}

	if ((memcmp(dasd_image_hdr->load_id, dasd_page3_inq.load_id, 4)) &&
	    (memcmp(dev->scsi_dev_data->vendor_id, "IBMAS400", 8) == 0)) {
		syslog(LOG_ERR, "Firmware file corrupt: %s.\n", ucode_file);
		munmap(dasd_image_hdr, ucode_stats.st_size);
		close(fd);
		return;
	}

	if (memcmp(&modification_level,
		   dasd_page3_inq.release_level, 4) > 0 || force) {
		dev_info(dev, "Updating disk microcode using %s "
			 "from %c%c%c%c to %c%c%c%c\n", ucode_file,
			 dasd_page3_inq.release_level[0],
			 dasd_page3_inq.release_level[1],
			 dasd_page3_inq.release_level[2],
			 dasd_page3_inq.release_level[3],
			 modification_level[0],
			 modification_level[1],
			 modification_level[2],
			 modification_level[3]);

		buffer = (void *)dasd_image_hdr + image_offset;
		image_size = ucode_stats.st_size - image_offset;

		rc = ipr_write_buffer(dev, buffer, image_size);

		if (rc != 0)
			dev_err(dev, "Write buffer failed\n");
	}

	if (munmap(dasd_image_hdr, ucode_stats.st_size))
		syslog(LOG_ERR, "munmap failed.\n");

	close(fd);
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
			    dev->scsi_dev_data->type != TYPE_DISK ||
			    dev->scsi_dev_data->type != IPR_TYPE_AF_DISK)
				continue;

			update_disk_fw(dev, force_devs);
		}
	}

	return rc;
}
